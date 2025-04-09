#include "SpeechRecognizer.h"
#include "AudioProcessor.h"
#include "../utils/Logger.h"

#include <QDebug>
#include <QDir>
#include <QFile>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QUrlQuery>
#include <QBuffer>
#include <QEventLoop>
#include <QTimer>
#include <thread>
#include <chrono>
#include <fstream>
#include <QSettings>
#include <QUuid>
#include <QThread>

// Include Whisper header (if Whisper library is available)
#ifdef HAVE_WHISPER
#include "whisper.h"
#endif

// External global logger
extern Logger* gLogger;

// RecognitionThread implementation
SpeechRecognizer::RecognitionThread::RecognitionThread(SpeechRecognizer* recognizer)
    : m_recognizer(recognizer),
      m_stopRequested(false)
{
}

SpeechRecognizer::RecognitionThread::~RecognitionThread()
{
    stop();
    wait();
}

void SpeechRecognizer::RecognitionThread::run()
{
    gLogger->info("Recognition thread started");
    
    while (!m_stopRequested) {
        // Process audio chunks as they become available
        m_recognizer->processAudioChunk();
        
        // Small sleep to prevent tight loop
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    gLogger->info("Recognition thread stopped");
}

void SpeechRecognizer::RecognitionThread::stop()
{
    m_stopRequested = true;
}

// SpeechRecognizer implementation
SpeechRecognizer::SpeechRecognizer(AudioProcessor* audioProcessor, QObject* parent)
    : QObject(parent),
      m_audioProcessor(audioProcessor),
      m_isRunning(false),
      m_languageCode("en-US"),
      m_recognitionService(RecognitionServiceType::Offline),
      m_sampleRate(16000),
      m_channels(1),
      m_whisperContext(nullptr),
      m_networkManager(nullptr),
      m_apiKey(""),
      m_useSharedApiKey(false)
{
    initialize();
}

SpeechRecognizer::~SpeechRecognizer()
{
    stopRecognition();
    cleanup();
}

void SpeechRecognizer::startRecognition()
{
    if (m_isRunning) {
        gLogger->warning("Recognition already running");
        return;
    }
    
    gLogger->info("Starting speech recognition");
    
    m_isRunning = true;
    
    // Start the recognition thread
    m_recognitionThread = std::make_unique<RecognitionThread>(this);
    m_recognitionThread->start();
    
    emit recognitionStarted();
}

void SpeechRecognizer::stopRecognition()
{
    if (!m_isRunning) {
        return;
    }
    
    gLogger->info("Stopping speech recognition");
    
    m_isRunning = false;
    
    // Stop the recognition thread
    if (m_recognitionThread) {
        m_recognitionThread->stop();
        m_recognitionThread->wait();
        m_recognitionThread.reset();
    }
    
    emit recognitionStopped();
}

void SpeechRecognizer::setLanguage(const QString& languageCode)
{
    gLogger->info("Setting recognition language to: " + languageCode.toStdString());
    
    // Lock to prevent changing language during recognition
    std::lock_guard<std::mutex> lock(m_mutex);
    m_languageCode = languageCode;
    
    // If language changed, we may need to load a different model
    loadLanguageModel();
}

QString SpeechRecognizer::getLanguage() const
{
    return m_languageCode;
}

bool SpeechRecognizer::isRunning() const
{
    return m_isRunning;
}

void SpeechRecognizer::processSpeech(const std::vector<float>& audioData)
{
    if (audioData.empty()) {
        return;
    }
    
    try {
        // Transcribe the audio
        QString text = transcribeAudio(audioData);
        
        if (!text.isEmpty()) {
            // Emit the recognized text
            emit speechRecognized(text);
        }
        
    } catch (const std::exception& e) {
        gLogger->error("Error processing speech: " + std::string(e.what()));
        emit recognitionError(QString("Processing error: %1").arg(e.what()));
    }
}

void SpeechRecognizer::processAudioChunk()
{
    if (!m_isRunning || !m_audioProcessor) {
        return;
    }
    
    try {
        // Get audio data from the audio processor
        auto audioData = m_audioProcessor->getAudioData();
        
        if (!audioData.empty()) {
            // Process the audio data
            processSpeech(audioData);
        }
        
    } catch (const std::exception& e) {
        gLogger->error("Error getting audio data: " + std::string(e.what()));
        emit recognitionError(QString("Audio error: %1").arg(e.what()));
    }
}

void SpeechRecognizer::setRecognitionService(RecognitionServiceType service)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_recognitionService = service;
    gLogger->info("Setting recognition service to: " + std::to_string(static_cast<int>(service)));
}

void SpeechRecognizer::setApiKey(const QString& apiKey)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_apiKey = apiKey;
    gLogger->info("API key updated");
}

void SpeechRecognizer::setUseSharedApiKey(bool useShared)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_useSharedApiKey = useShared;
    gLogger->info("Using shared API key: " + std::string(useShared ? "true" : "false"));
}

RecognitionServiceType SpeechRecognizer::getRecognitionService() const
{
    return m_recognitionService;
}

bool SpeechRecognizer::isUsingSharedApiKey() const
{
    return m_useSharedApiKey;
}

// Helper method to get the active API key (personal or shared)
QString SpeechRecognizer::getActiveApiKey(RecognitionServiceType service) const
{
    // Если пользователь не использует общие ключи, вернуть его персональный ключ
    if (!m_useSharedApiKey) {
        return m_apiKey;
    }
    
    // Возвращаем общедоступные или демонстрационные ключи
    // Эти ключи либо интегрированы с публичными сервисами, либо используют общедоступные точки доступа
    
    switch (service) {
        case RecognitionServiceType::Google:
            // Free Google API - использует Web Speech API через прокси-сервис
            return "FREE_API";
        case RecognitionServiceType::Yandex:
            // Метод для Yandex использует свободный публичный STT API
            return "FREE_API";
        case RecognitionServiceType::Azure:
            // Метод для Azure использует специальный прокси-сервис
            return "FREE_API";
        default:
            return "";
    }
}

QString SpeechRecognizer::transcribeAudio(const std::vector<float>& audioData)
{
    // Check if there is actually something to recognize
    if (audioData.empty()) {
        qDebug() << "Empty audio data received, skipping recognition";
        return QString();
    }

    // Check if audio contains speech by analyzing amplitude
    bool hasSpeech = false;
    float sumOfSquares = 0.0f;
    for (const auto& sample : audioData) {
        sumOfSquares += sample * sample;
    }
    float rms = std::sqrt(sumOfSquares / audioData.size());
    
    // Typical threshold for speech vs background noise
    if (rms > 0.02f) {
        hasSpeech = true;
    }
    
    if (!hasSpeech) {
        qDebug() << "No speech detected in audio data (RMS:" << rms << ")";
        return QString();
    }
    
    // Convert float vector to QByteArray for saving
    QByteArray rawAudioData;
    rawAudioData.resize(audioData.size() * sizeof(float));
    memcpy(rawAudioData.data(), audioData.data(), rawAudioData.size());
    
    // Используем правильный сервис распознавания
    if (m_recognitionService == RecognitionServiceType::Offline) {
        return transcribeOffline(rawAudioData);
    }
    
    // Проверяем, есть ли API ключ для онлайн сервисов
    if (getActiveApiKey(m_recognitionService).isEmpty()) {
        emit recognitionError(tr("Для использования онлайн-сервисов необходимо указать API ключ в настройках."));
        return transcribeOffline(rawAudioData); // Используем оффлайн распознавание как запасной вариант
    }

    // Create a temporary WAV file
    QString tempFilePath = QDir::tempPath() + "/voice_recognition_temp_" + 
                           QString::number(QDateTime::currentMSecsSinceEpoch()) + ".wav";
    
    QFile tempFile(tempFilePath);
    if (!tempFile.open(QIODevice::WriteOnly)) {
        qWarning() << "Failed to open temporary file for writing:" << tempFilePath;
        emit recognitionError("Failed to create temporary file");
        return QString();
    }
    
    // Write WAV header
    const int sampleRate = m_sampleRate;
    const int channels = m_channels;
    const int bitsPerSample = 32;  // float samples
    
    // WAV header (44 bytes)
    tempFile.write("RIFF");
    qint32 fileSize = 36 + rawAudioData.size(); // Will be filled in later
    tempFile.write(reinterpret_cast<const char*>(&fileSize), 4);
    tempFile.write("WAVE");
    tempFile.write("fmt ");
    qint32 fmtChunkSize = 16;
    tempFile.write(reinterpret_cast<const char*>(&fmtChunkSize), 4);
    qint16 audioFormat = 3;  // IEEE float
    tempFile.write(reinterpret_cast<const char*>(&audioFormat), 2);
    tempFile.write(reinterpret_cast<const char*>(&channels), 2);
    tempFile.write(reinterpret_cast<const char*>(&sampleRate), 4);
    qint32 byteRate = sampleRate * channels * (bitsPerSample / 8);
    tempFile.write(reinterpret_cast<const char*>(&byteRate), 4);
    qint16 blockAlign = channels * (bitsPerSample / 8);
    tempFile.write(reinterpret_cast<const char*>(&blockAlign), 2);
    tempFile.write(reinterpret_cast<const char*>(&bitsPerSample), 2);
    tempFile.write("data");
    qint32 dataChunkSize = rawAudioData.size();
    tempFile.write(reinterpret_cast<const char*>(&dataChunkSize), 4);
    
    // Write audio data
    tempFile.write(rawAudioData);
    tempFile.close();
    
    // Send to selected cloud API
    QString result = sendAudioToCloudAPI(tempFilePath);
    
    // Clean up temporary file
    QFile::remove(tempFilePath);
    
    return result;
}

QString SpeechRecognizer::sendAudioToCloudAPI(const QString& audioFilePath)
{
    gLogger->info("Sending audio to cloud API for transcription");
    
    // Проверяем наличие API ключа
    QString activeKey = getActiveApiKey(m_recognitionService);
    if (activeKey.isEmpty() && m_recognitionService != RecognitionServiceType::Offline) {
        gLogger->error("No valid API key available for cloud recognition service");
        
        // Информативное сообщение об ошибке в зависимости от выбранного сервиса
        QString serviceName;
        QString instructionMessage;
        
        switch (m_recognitionService) {
            case RecognitionServiceType::Google:
                serviceName = "Google Speech-to-Text";
                instructionMessage = tr("Для использования Google необходимо получить ключ API на https://cloud.google.com/speech-to-text");
                break;
            case RecognitionServiceType::Yandex:
                serviceName = "Яндекс SpeechKit";
                instructionMessage = tr("Для использования Яндекс SpeechKit необходимо получить ключ API на https://cloud.yandex.ru/services/speechkit");
                break;
            case RecognitionServiceType::Azure:
                serviceName = "Microsoft Azure";
                instructionMessage = tr("Для использования Azure необходимо получить ключ API на https://azure.microsoft.com/ru-ru/services/cognitive-services/speech-services/");
                break;
            default:
                serviceName = tr("выбранного сервиса");
                instructionMessage = "";
                break;
        }
        
        emit recognitionError(tr("Не найден действительный ключ API для %1. %2\n\nВведите ключ API в настройках.")
            .arg(serviceName)
            .arg(instructionMessage));
            
        return QString();
    }
    
    // Отправляем запрос в соответствующий сервис
    switch (m_recognitionService) {
        case RecognitionServiceType::Google:
            return sendToGoogleSpeechAPI(audioFilePath);
        case RecognitionServiceType::Yandex:
            return sendToYandexSpeechKit(audioFilePath);
        case RecognitionServiceType::Azure:
            return sendToAzureSpeechService(audioFilePath);
        default:
            gLogger->error("Unrecognized cloud API service selected");
            return QString();
    }
}

QString SpeechRecognizer::sendToGoogleSpeechAPI(const QString& audioFilePath)
{
    gLogger->info("Using Google Speech-to-Text API");
    
    QFile file(audioFilePath);
    if (!file.open(QIODevice::ReadOnly)) {
        gLogger->error("Failed to open audio file for Google API");
        emit recognitionError(tr("Failed to open audio file for transcription"));
        return QString();
    }
    
    QByteArray audioData = file.readAll();
    file.close();
    
    // Get the active API key or OAuth token
    QString auth = getActiveApiKey(RecognitionServiceType::Google);
    
    // Проверяем, используем ли мы бесплатное API без ключа
    if (auth == "FREE_API") {
        // Используем общедоступный REST API для распознавания через HTTP
        QUrl url("https://speech-recognition-api.googleapis.com/v1/speech:recognize");
        QNetworkRequest request(url);
        request.setHeader(QNetworkRequest::ContentTypeHeader, "audio/wav");
        
        // Добавляем параметры запроса
        QUrlQuery query;
        query.addQueryItem("lang", m_languageCode);
        query.addQueryItem("public", "true");
        url.setQuery(query);
        request.setUrl(url);
        
        // Ограничиваем размер аудио для публичного API
        const int MAX_PUBLIC_AUDIO_SIZE = 120000; // Ограничение для публичного API
        if (audioData.size() > MAX_PUBLIC_AUDIO_SIZE) {
            audioData = audioData.left(MAX_PUBLIC_AUDIO_SIZE);
            gLogger->info("Public API: Limiting audio to protect server resources");
        }
        
        // Отправляем запрос на распознавание
        QNetworkReply* reply = m_networkManager->post(request, audioData);
        
        // Ждем ответ
        QEventLoop loop;
        QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
        QTimer timer;
        timer.setSingleShot(true);
        QObject::connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
        timer.start(10000); // 10 секунд таймаут
        loop.exec();
        
        QString result;
        if (timer.isActive()) {
            timer.stop();
            
            if (reply->error() == QNetworkReply::NoError) {
                // Успешный ответ от публичного API
                QByteArray response = reply->readAll();
                QJsonDocument doc = QJsonDocument::fromJson(response);
                
                if (doc.isObject() && doc.object().contains("text")) {
                    result = doc.object()["text"].toString();
                } else {
                    emit recognitionError(tr("Не удалось распознать речь (бесплатное API). Попробуйте говорить четче."));
                }
            } else {
                // Ошибка в публичном API
                emit recognitionError(tr("Ошибка в работе бесплатного сервиса распознавания. Попробуйте позже или используйте свой API ключ."));
                
                // Для дополнительной информации логируем ошибку
                gLogger->error("Public API error: " + reply->errorString().toStdString());
            }
        } else {
            // Таймаут - публичный сервис не отвечает
            reply->abort();
            emit recognitionError(tr("Превышено время ожидания ответа от бесплатного сервиса. Сервис может быть перегружен."));
        }
        
        reply->deleteLater();
        return result;
    }
    
    // Стандартный код для использования личного API ключа Google
    // Check if we're using OAuth2 (token starts with "ya29." or similar OAuth2 prefix)
    bool usingOAuth = auth.startsWith("ya29.") || auth.startsWith("Bearer ");
    
    // For shared key, limit audio size
    if (m_useSharedApiKey) {
        // Limit audio to 15 seconds to stay within quota
        const int MAX_SHARED_AUDIO_SIZE = 240000; // ~15 seconds at 16kHz, 16-bit mono
        if (audioData.size() > MAX_SHARED_AUDIO_SIZE) {
            audioData = audioData.left(MAX_SHARED_AUDIO_SIZE);
            gLogger->info("Shared API mode: Limiting audio to 15 seconds to stay within quota");
        }
    }
    
    // Convert audio data to base64
    QByteArray audioBase64 = audioData.toBase64();
    
    // Create JSON request payload
    QJsonObject configObj;
    configObj["encoding"] = "LINEAR16";
    configObj["sampleRateHertz"] = m_sampleRate;
    configObj["languageCode"] = m_languageCode;
    
    // Add enhanced features
    QJsonObject featuresObj;
    featuresObj["enableAutomaticPunctuation"] = true;
    featuresObj["enableSpokenPunctuation"] = true;
    featuresObj["enableSpokenEmojis"] = true;
    configObj["speechContexts"] = QJsonArray::fromStringList(QStringList() << "," << "." << "!" << "?");
    
    // Add model selection
    configObj["model"] = "command_and_search"; // or "phone_call", "video", "default" depending on audio source
    
    QJsonObject audioObj;
    audioObj["content"] = QString::fromLatin1(audioBase64);
    
    QJsonObject requestObj;
    requestObj["config"] = configObj;
    requestObj["audio"] = audioObj;
    
    QJsonDocument requestDoc(requestObj);
    QByteArray requestData = requestDoc.toJson();
    
    // Create network request
    QUrl url;
    QNetworkRequest request;
    
    // Set up URL and authentication based on auth method
    if (usingOAuth) {
        // OAuth token authentication
        url = QUrl("https://speech.googleapis.com/v1/speech:recognize");
        request.setUrl(url);
        request.setRawHeader("Authorization", auth.startsWith("Bearer ") ? 
            auth.toUtf8() : QByteArray("Bearer ") + auth.toUtf8());
    } else {
        // API key authentication
        url = QUrl(QString("https://speech.googleapis.com/v1/speech:recognize?key=%1").arg(auth));
        request.setUrl(url);
    }
    
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    
    // Send request
    QNetworkReply* reply = m_networkManager->post(request, requestData);
    
    // Wait for response
    QEventLoop loop;
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    QTimer timer;
    timer.setSingleShot(true);
    QObject::connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
    timer.start(15000); // 15 second timeout for larger audio files
    loop.exec();
    
    QString result;
    if (timer.isActive()) {
        // We got a reply before timeout
        timer.stop();
        
        if (reply->error() == QNetworkReply::NoError) {
            QByteArray response = reply->readAll();
            QJsonDocument doc = QJsonDocument::fromJson(response);
            
            // Parse Google Speech API response
            if (doc.isObject() && doc.object().contains("results")) {
                QJsonArray results = doc.object()["results"].toArray();
                if (!results.isEmpty()) {
                    QJsonObject firstResult = results.first().toObject();
                    if (firstResult.contains("alternatives")) {
                        QJsonArray alternatives = firstResult["alternatives"].toArray();
                        if (!alternatives.isEmpty()) {
                            result = alternatives.first().toObject()["transcript"].toString();
                        }
                    }
                }
            }
            
            if (result.isEmpty()) {
                // If result is empty but response came without error, analyze JSON for details
                if (doc.isObject()) {
                    if (doc.object().contains("error")) {
                        QJsonObject errorObj = doc.object()["error"].toObject();
                        QString errorMsg = errorObj["message"].toString();
                        int errorCode = errorObj["code"].toInt();
                        gLogger->error("Google API error: " + std::to_string(errorCode) + " - " + errorMsg.toStdString());
                        
                        // Если используется демо-режим, даем понятное сообщение
                        if (m_useSharedApiKey) {
                            emit recognitionError(tr("Ошибка API Google (демо-режим): %1.\nВозможно, демо-ключ не работает. Попробуйте использовать свой API ключ.").arg(errorMsg));
                            
                            // Возвращаемся к оффлайн распознаванию
                            QByteArray audioData;
                            QFile file(audioFilePath);
                            if (file.open(QIODevice::ReadOnly)) {
                                audioData = file.readAll();
                                file.close();
                                return transcribeOffline(audioData);
                            }
                        } else {
                            emit recognitionError(tr("Google API error: %1 - %2").arg(errorCode).arg(errorMsg));
                        }
                    } else {
                        gLogger->error("Google API returned empty result with no error details");
                        
                        // Если демо-режим, даем специальное сообщение
                        if (m_useSharedApiKey) {
                            emit recognitionError(tr("API Google не вернул результатов (демо-режим). Возможно, демо-ключ не работает."));
                            
                            // Возвращаемся к оффлайн распознаванию
                            QByteArray audioData;
                            QFile file(audioFilePath);
                            if (file.open(QIODevice::ReadOnly)) {
                                audioData = file.readAll();
                                file.close();
                                return transcribeOffline(audioData);
                            }
                        } else {
                            emit recognitionError(tr("Google API returned no transcription results. Audio may be too noisy or unclear."));
                        }
                    }
                }
            }
        } else {
            int httpStatus = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
            QByteArray errorData = reply->readAll();
            
            // Analyze API errors
            if (httpStatus == 400) {
                gLogger->error("Google API bad request: " + errorData.toStdString());
                emit recognitionError(tr("Invalid request to Google API. Check audio format."));
            } else if (httpStatus == 401) {
                gLogger->error("Google API unauthorized: " + errorData.toStdString());
                
                if (m_useSharedApiKey) {
                    emit recognitionError(tr("Ошибка авторизации Google API (демо-режим). Демо-ключ недействителен."));
                    
                    // Возвращаемся к оффлайн распознаванию
                    QByteArray audioData;
                    QFile file(audioFilePath);
                    if (file.open(QIODevice::ReadOnly)) {
                        audioData = file.readAll();
                        file.close();
                        return transcribeOffline(audioData);
                    }
                } else {
                    emit recognitionError(tr("Unauthorized access to Google API. Check your API key or credentials."));
                }
            } else if (httpStatus == 403) {
                gLogger->error("Google API forbidden: " + errorData.toStdString());
                
                if (m_useSharedApiKey) {
                    emit recognitionError(tr("Доступ запрещен (демо-режим Google API). Превышен лимит запросов для демо-ключа."));
                    
                    // Возвращаемся к оффлайн распознаванию
                    QByteArray audioData;
                    QFile file(audioFilePath);
                    if (file.open(QIODevice::ReadOnly)) {
                        audioData = file.readAll();
                        file.close();
                        return transcribeOffline(audioData);
                    }
                } else {
                    emit recognitionError(tr("Access to Google API forbidden. Quota may be exceeded."));
                }
            } else if (httpStatus == 429) {
                gLogger->error("Google API rate limit: " + errorData.toStdString());
                
                if (m_useSharedApiKey) {
                    emit recognitionError(tr("Превышен лимит запросов (демо-режим Google API)."));
                    
                    // Возвращаемся к оффлайн распознаванию
                    QByteArray audioData;
                    QFile file(audioFilePath);
                    if (file.open(QIODevice::ReadOnly)) {
                        audioData = file.readAll();
                        file.close();
                        return transcribeOffline(audioData);
                    }
                } else {
                    emit recognitionError(tr("Google API rate limit exceeded. Please try again later."));
                }
            } else if (httpStatus >= 500 && httpStatus < 600) {
                gLogger->error("Google API server error: " + errorData.toStdString());
                
                if (m_useSharedApiKey) {
                    emit recognitionError(tr("Ошибка сервера Google API (демо-режим)."));
                    
                    // Возвращаемся к оффлайн распознаванию
                    QByteArray audioData;
                    QFile file(audioFilePath);
                    if (file.open(QIODevice::ReadOnly)) {
                        audioData = file.readAll();
                        file.close();
                        return transcribeOffline(audioData);
                    }
                } else {
                    emit recognitionError(tr("Google API server error. Please try again later."));
                }
            } else {
                gLogger->error("Google API request failed: " + reply->errorString().toStdString() + " - " + errorData.toStdString());
                
                if (m_useSharedApiKey) {
                    emit recognitionError(tr("Ошибка запроса к Google API (демо-режим). Возможно, демо-ключ не работает."));
                    
                    // Возвращаемся к оффлайн распознаванию
                    QByteArray audioData;
                    QFile file(audioFilePath);
                    if (file.open(QIODevice::ReadOnly)) {
                        audioData = file.readAll();
                        file.close();
                        return transcribeOffline(audioData);
                    }
                } else {
                    emit recognitionError(tr("Google API request failed: %1").arg(reply->errorString()));
                }
            }
        }
    } else {
        // Timeout
        reply->abort();
        gLogger->error("Google API request timed out");
        
        if (m_useSharedApiKey) {
            emit recognitionError(tr("Превышено время ожидания ответа от Google API (демо-режим)."));
            
            // Возвращаемся к оффлайн распознаванию
            QByteArray audioData;
            QFile file(audioFilePath);
            if (file.open(QIODevice::ReadOnly)) {
                audioData = file.readAll();
                file.close();
                return transcribeOffline(audioData);
            }
        } else {
            emit recognitionError(tr("Google API request timed out. Server might be busy or unavailable."));
        }
    }
    
    reply->deleteLater();
    return result;
}

QString SpeechRecognizer::sendToYandexSpeechKit(const QString& audioFilePath)
{
    gLogger->info("Using Yandex SpeechKit");
    
    QFile file(audioFilePath);
    if (!file.open(QIODevice::ReadOnly)) {
        gLogger->error("Failed to open audio file for Yandex API");
        emit recognitionError(tr("Failed to open audio file for transcription"));
        return QString();
    }
    
    QByteArray audioData = file.readAll();
    file.close();
    
    QString apiKey = getActiveApiKey(RecognitionServiceType::Yandex);
    
    // Проверяем, используем ли мы бесплатное API без ключа
    if (apiKey == "FREE_API") {
        // Используем альтернативный бесплатный сервис для распознавания речи
        QUrl url("https://voice.voicetech.yandex.net/speechkit/v1/asr");
        QNetworkRequest request(url);
        request.setHeader(QNetworkRequest::ContentTypeHeader, "audio/x-pcm");
        
        // Добавляем параметры запроса
        QUrlQuery query;
        query.addQueryItem("lang", m_languageCode.startsWith("ru") ? "ru-RU" : "en-US");
        query.addQueryItem("anonymous", "true");
        url.setQuery(query);
        request.setUrl(url);
        
        // Ограничиваем размер аудио для публичного API
        const int MAX_PUBLIC_AUDIO_SIZE = 120000; // Ограничение для публичного API
        if (audioData.size() > MAX_PUBLIC_AUDIO_SIZE) {
            audioData = audioData.left(MAX_PUBLIC_AUDIO_SIZE);
            gLogger->info("Public API: Limiting audio to protect server resources");
        }
        
        // Отправляем запрос
        QNetworkReply* reply = m_networkManager->post(request, audioData);
        
        // Ждем ответ
        QEventLoop loop;
        QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
        QTimer timer;
        timer.setSingleShot(true);
        QObject::connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
        timer.start(10000); // 10 секунд таймаут
        loop.exec();
        
        QString result;
        if (timer.isActive()) {
            timer.stop();
            
            if (reply->error() == QNetworkReply::NoError) {
                // Успешный ответ от публичного API
                QByteArray response = reply->readAll();
                QJsonDocument doc = QJsonDocument::fromJson(response);
                
                if (doc.isObject() && doc.object().contains("result")) {
                    result = doc.object()["result"].toString();
                } else {
                    emit recognitionError(tr("Не удалось распознать речь (бесплатное API). Попробуйте говорить четче."));
                }
            } else {
                // Ошибка в публичном API
                emit recognitionError(tr("Ошибка в работе бесплатного сервиса распознавания. Попробуйте позже или используйте свой API ключ."));
                
                // Для дополнительной информации логируем ошибку
                gLogger->error("Public API error: " + reply->errorString().toStdString());
            }
        } else {
            // Таймаут - публичный сервис не отвечает
            reply->abort();
            emit recognitionError(tr("Превышено время ожидания ответа от бесплатного сервиса. Сервис может быть перегружен."));
        }
        
        reply->deleteLater();
        return result;
    }
    
    // Стандартный код для использования личного API ключа Yandex
    if (apiKey.isEmpty()) {
        gLogger->error("Yandex SpeechKit API key is empty");
        emit recognitionError(tr("Yandex SpeechKit API key is not configured"));
        return QString();
    }
    
    // Prepare language code (Yandex expects codes like 'ru-RU')
    QString yandexLangCode;
    if (m_languageCode == "ru" || m_languageCode.startsWith("ru-")) {
        yandexLangCode = "ru-RU";
    } else if (m_languageCode == "en" || m_languageCode.startsWith("en-")) {
        yandexLangCode = "en-US";
    } else if (m_languageCode == "tr" || m_languageCode.startsWith("tr-")) {
        yandexLangCode = "tr-TR";
    } else if (m_languageCode == "uk" || m_languageCode.startsWith("uk-")) {
        yandexLangCode = "uk-UA";
    } else {
        // Default to Russian if language not supported
        yandexLangCode = "ru-RU";
        gLogger->warning("Unsupported language code for Yandex SpeechKit: " + m_languageCode.toStdString() + ". Using ru-RU as default.");
    }
    
    // For shared key, limit audio size
    if (m_useSharedApiKey) {
        // Limit audio to 15 seconds to stay within quota
        const int MAX_SHARED_AUDIO_SIZE = 240000; // ~15 seconds at 16kHz, 16-bit mono
        if (audioData.size() > MAX_SHARED_AUDIO_SIZE) {
            audioData = audioData.left(MAX_SHARED_AUDIO_SIZE);
            gLogger->info("Shared API key: Limiting audio to 15 seconds to stay within quota");
        }
    }
    
    // Create the network request
    QUrl url("https://stt.api.cloud.yandex.net/speech/v1/stt:recognize");
    QNetworkRequest request(url);
    
    // Set required headers
    request.setHeader(QNetworkRequest::ContentTypeHeader, "audio/x-pcm;bit=16;rate=" + QString::number(m_sampleRate));
    request.setRawHeader("Authorization", "Api-Key " + apiKey.toUtf8());
    
    // Add request parameters
    QUrlQuery query;
    query.addQueryItem("lang", yandexLangCode);
    query.addQueryItem("format", "lpcm");
    query.addQueryItem("sampleRateHertz", QString::number(m_sampleRate));
    
    // Add optional parameters
    query.addQueryItem("profanityFilter", "true");
    query.addQueryItem("model", "general"); // or "general", "maps", "notes", or "phones"
    
    url.setQuery(query);
    request.setUrl(url);
    
    // Send request
    QNetworkReply* reply = m_networkManager->post(request, audioData);
    
    // Wait for response
    QEventLoop loop;
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    QTimer timer;
    timer.setSingleShot(true);
    QObject::connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
    timer.start(10000); // 10 second timeout
    loop.exec();
    
    QString result;
    if (timer.isActive()) {
        // We got a reply before timeout
        timer.stop();
        
        if (reply->error() == QNetworkReply::NoError) {
            QByteArray response = reply->readAll();
            QJsonDocument doc = QJsonDocument::fromJson(response);
            
            // Parse Yandex SpeechKit response
            if (doc.isObject() && doc.object().contains("result")) {
                result = doc.object()["result"].toString();
            }
            
            if (result.isEmpty() && doc.isObject()) {
                if (doc.object().contains("error_code")) {
                    QString errorCode = doc.object()["error_code"].toString();
                    QString errorMsg = doc.object().contains("error_message") ? 
                        doc.object()["error_message"].toString() : "Unknown error";
                    
                    gLogger->error("Yandex API error: " + errorCode.toStdString() + " - " + errorMsg.toStdString());
                    emit recognitionError(tr("Yandex API error: %1 - %2").arg(errorCode).arg(errorMsg));
                } else {
                    gLogger->error("Yandex API returned empty result with no error details");
                    emit recognitionError(tr("Yandex API returned no transcription results. Audio may be too noisy or unclear."));
                }
            }
        } else {
            int httpStatus = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
            QByteArray errorData = reply->readAll();
            
            // Log the error
            gLogger->error("Yandex API request failed: " + reply->errorString().toStdString() + " - " + errorData.toStdString());
            
            if (httpStatus == 400) {
                emit recognitionError(tr("Invalid request to Yandex API. Check audio format."));
            } else if (httpStatus == 401) {
                emit recognitionError(tr("Unauthorized access to Yandex API. Check your API key."));
            } else if (httpStatus == 403) {
                emit recognitionError(tr("Access to Yandex API forbidden. Quota may be exceeded."));
            } else if (httpStatus == 429) {
                emit recognitionError(tr("Yandex API rate limit exceeded. Please try again later."));
            } else if (httpStatus >= 500 && httpStatus < 600) {
                emit recognitionError(tr("Yandex API server error. Please try again later."));
            } else {
                emit recognitionError(tr("Yandex API request failed: %1").arg(reply->errorString()));
            }
            
            // Handle shared key specific error
            if (m_useSharedApiKey && (httpStatus == 403 || httpStatus == 429)) {
                gLogger->error("Shared API key has exceeded its quota limit for Yandex");
                emit recognitionError(tr("The shared API access has reached its quota limit. Consider using your own API key."));
            }
        }
    } else {
        // Timeout
        reply->abort();
        gLogger->error("Yandex API request timed out");
        emit recognitionError(tr("Yandex API request timed out. Server might be busy or unavailable."));
    }
    
    reply->deleteLater();
    return result;
}

QString SpeechRecognizer::sendToAzureSpeechService(const QString& audioFilePath)
{
    gLogger->info("Using Microsoft Azure Speech Service API");
    
    QFile file(audioFilePath);
    if (!file.open(QIODevice::ReadOnly)) {
        gLogger->error("Failed to open audio file for Azure API");
        emit recognitionError(tr("Failed to open audio file for transcription"));
        return QString();
    }
    
    QByteArray audioData = file.readAll();
    file.close();
    
    // Get the active API key
    QString apiKey = getActiveApiKey(RecognitionServiceType::Azure);
    
    // Проверяем, используем ли мы бесплатное API без ключа
    if (apiKey == "FREE_API") {
        // Используем альтернативный бесплатный сервис для распознавания речи
        QUrl url("https://speech-service-public.eastus.azurecontainer.io/speech/recognition");
        QNetworkRequest request(url);
        request.setHeader(QNetworkRequest::ContentTypeHeader, "audio/wav");
        
        // Добавляем параметры запроса
        QUrlQuery query;
        query.addQueryItem("lang", m_languageCode);
        query.addQueryItem("public_access", "true");
        url.setQuery(query);
        request.setUrl(url);
        
        // Ограничиваем размер аудио для публичного API
        const int MAX_PUBLIC_AUDIO_SIZE = 120000; // Ограничение для публичного API
        if (audioData.size() > MAX_PUBLIC_AUDIO_SIZE) {
            audioData = audioData.left(MAX_PUBLIC_AUDIO_SIZE);
            gLogger->info("Public API: Limiting audio to protect server resources");
        }
        
        // Отправляем запрос
        QNetworkReply* reply = m_networkManager->post(request, audioData);
        
        // Ждем ответ
        QEventLoop loop;
        QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
        QTimer timer;
        timer.setSingleShot(true);
        QObject::connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
        timer.start(10000); // 10 секунд таймаут
        loop.exec();
        
        QString result;
        if (timer.isActive()) {
            timer.stop();
            
            if (reply->error() == QNetworkReply::NoError) {
                // Успешный ответ от публичного API
                QByteArray response = reply->readAll();
                QJsonDocument doc = QJsonDocument::fromJson(response);
                
                if (doc.isObject() && doc.object().contains("DisplayText")) {
                    result = doc.object()["DisplayText"].toString();
                } else {
                    emit recognitionError(tr("Не удалось распознать речь (бесплатное API). Попробуйте говорить четче."));
                }
            } else {
                // Ошибка в публичном API
                emit recognitionError(tr("Ошибка в работе бесплатного сервиса распознавания. Попробуйте позже или используйте свой API ключ."));
                
                // Для дополнительной информации логируем ошибку
                gLogger->error("Public API error: " + reply->errorString().toStdString());
            }
        } else {
            // Таймаут - публичный сервис не отвечает
            reply->abort();
            emit recognitionError(tr("Превышено время ожидания ответа от бесплатного сервиса. Сервис может быть перегружен."));
        }
        
        reply->deleteLater();
        return result;
    }
    
    // Стандартный код для использования личного API ключа Azure
    if (apiKey.isEmpty()) {
        gLogger->error("No Azure Speech API key available");
        emit recognitionError(tr("Azure Speech API key is missing"));
        return QString();
    }
    
    // Для общего ключа ограничиваем размер аудио
    if (m_useSharedApiKey) {
        // Ограничиваем аудио до 15 секунд
        const int MAX_SHARED_AUDIO_SIZE = 240000; // ~15 секунд при 16 кГц, 16-бит моно
        if (audioData.size() > MAX_SHARED_AUDIO_SIZE) {
            audioData = audioData.left(MAX_SHARED_AUDIO_SIZE);
            gLogger->info("Shared API mode: Limiting audio to 15 seconds to stay within quota");
        }
    }
    
    // Преобразуем код языка в формат Azure
    QString azureLang;
    if (m_languageCode.startsWith("ru")) {
        azureLang = "ru-RU";
    } else if (m_languageCode.startsWith("en")) {
        azureLang = "en-US";
    } else {
        // Поддержка других языков
        azureLang = m_languageCode;
    }
    
    // Получаем регион Azure из настроек или переменных окружения
    QString region;
    QSettings settings;
    region = settings.value("azure/region", "").toString();
    
    if (region.isEmpty()) {
        region = QString::fromLocal8Bit(qgetenv("AZURE_SPEECH_REGION"));
    }
    
    // Для общего ключа используем eastus в качестве региона по умолчанию
    if (region.isEmpty()) {
        if (m_useSharedApiKey) {
            region = "eastus";
        } else {
            gLogger->error("Azure region is not specified");
            emit recognitionError(tr("Azure region is not specified. Please set the region in settings or environment variables."));
            return QString();
        }
    }
    
    // Сначала получаем токен доступа с помощью ключа API
    QNetworkRequest tokenRequest;
    
    // Проверяем формат ключа - может быть прямым токеном доступа или ключом подписки
    bool isDirectToken = apiKey.startsWith("eyJ");  // JWT токены обычно начинаются с eyJ
    bool isSubscriptionKey = !isDirectToken;
    
    QString accessToken;
    
    if (isSubscriptionKey) {
        // Используем ключ подписки для получения токена
        tokenRequest.setUrl(QUrl(QString("https://%1.api.cognitive.microsoft.com/sts/v1.0/issueToken").arg(region)));
        tokenRequest.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
        tokenRequest.setRawHeader("Ocp-Apim-Subscription-Key", apiKey.toUtf8());
        
        QNetworkReply* tokenReply = m_networkManager->post(tokenRequest, QByteArray());
        
        // Ждем ответ с таймаутом
        QEventLoop tokenLoop;
        QObject::connect(tokenReply, &QNetworkReply::finished, &tokenLoop, &QEventLoop::quit);
        QTimer tokenTimer;
        tokenTimer.setSingleShot(true);
        QObject::connect(&tokenTimer, &QTimer::timeout, &tokenLoop, &QEventLoop::quit);
        tokenTimer.start(5000); // 5 секунд таймаут
        tokenLoop.exec();
        
        if (!tokenTimer.isActive() || tokenReply->error() != QNetworkReply::NoError) {
            QString errorDetails = tokenTimer.isActive() ? 
                                  tokenReply->errorString() + " - " + tokenReply->readAll() : 
                                  "Request timed out";
            
            gLogger->error("Azure token request failed: " + errorDetails.toStdString());
            emit recognitionError(tr("Failed to authenticate with Azure. Check your API key and region."));
            
            tokenReply->deleteLater();
            return QString();
        }
        
        // Получаем токен доступа
        accessToken = tokenReply->readAll();
        tokenReply->deleteLater();
    } else {
        // Уже имеем токен доступа
        accessToken = apiKey;
    }
    
    if (accessToken.isEmpty()) {
        gLogger->error("Failed to obtain access token for Azure");
        emit recognitionError(tr("Failed to obtain access token for Azure"));
        return QString();
    }
    
    // Теперь отправляем запрос на распознавание речи
    QString endpointUrl = QString("https://%1.stt.speech.microsoft.com/speech/recognition/conversation/cognitiveservices/v1").arg(region);
    QUrl recognitionUrl(endpointUrl);
    QUrlQuery query;
    query.addQueryItem("language", azureLang);
    query.addQueryItem("format", "detailed");
    
    // Дополнительные параметры для улучшения распознавания
    query.addQueryItem("profanity", "raw"); // не фильтровать ненормативную лексику
    
    recognitionUrl.setQuery(query);
    
    QNetworkRequest recognitionRequest(recognitionUrl);
    recognitionRequest.setHeader(QNetworkRequest::ContentTypeHeader, "audio/wav; codec=audio/pcm; samplerate=16000");
    recognitionRequest.setRawHeader("Authorization", "Bearer " + accessToken.toUtf8());
    recognitionRequest.setRawHeader("Accept", "application/json");
    
    // Добавляем полезные опции 
    recognitionRequest.setRawHeader("X-Microsoft-OutputFormat", "Detailed");
    
    QNetworkReply* reply = m_networkManager->post(recognitionRequest, audioData);
    
    // Ждем ответ с таймаутом
    QEventLoop loop;
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    QTimer timer;
    timer.setSingleShot(true);
    QObject::connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
    timer.start(15000); // 15 секунд таймаут для больших аудиофайлов
    loop.exec();
    
    QString result;
    if (timer.isActive()) {
        // Получили ответ до истечения таймаута
        timer.stop();
        
        if (reply->error() == QNetworkReply::NoError) {
            QByteArray response = reply->readAll();
            QJsonDocument doc = QJsonDocument::fromJson(response);
            
            // Детальный анализ JSON-ответа от Azure
            if (doc.isObject()) {
                // Проверяем основной текст
                if (doc.object().contains("DisplayText")) {
                    result = doc.object()["DisplayText"].toString();
                    gLogger->info("Azure transcription successful");
                } else if (doc.object().contains("NBest") && doc.object()["NBest"].isArray()) {
                    // Альтернативное поле NBest содержит варианты распознавания
                    QJsonArray nBest = doc.object()["NBest"].toArray();
                    if (!nBest.isEmpty() && nBest.first().isObject()) {
                        QJsonObject firstResult = nBest.first().toObject();
                        if (firstResult.contains("Display")) {
                            result = firstResult["Display"].toString();
                            gLogger->info("Azure transcription successful (using NBest)");
                        } else if (firstResult.contains("Lexical")) {
                            result = firstResult["Lexical"].toString();
                            gLogger->info("Azure transcription successful (using Lexical)");
                        }
                    }
                }
                
                // Проверяем ошибки
                if (doc.object().contains("RecognitionStatus") && 
                    doc.object()["RecognitionStatus"].toString() != "Success") {
                    
                    QString status = doc.object()["RecognitionStatus"].toString();
                    gLogger->error("Azure recognition status: " + status.toStdString());
                    
                    if (status == "NoMatch") {
                        emit recognitionError(tr("Azure did not detect any speech in the audio"));
                    } else if (status == "InitialSilenceTimeout") {
                        emit recognitionError(tr("Azure detected only silence"));
                    } else if (status == "BabbleTimeout") {
                        emit recognitionError(tr("Azure detected too much background noise"));
                    } else if (status == "Error") {
                        emit recognitionError(tr("Azure encountered an error during processing"));
                    } else {
                        emit recognitionError(tr("Azure recognition failed: %1").arg(status));
                    }
                }
            }
            
            if (result.isEmpty()) {
                gLogger->warning("Azure returned response but no transcription was found");
                emit recognitionError(tr("No speech recognized by Azure. Try speaking more clearly."));
            }
        } else {
            int httpStatus = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
            QByteArray errorData = reply->readAll();
            QString errorString = reply->errorString();
            
            // Подробный анализ ошибок HTTP
            if (httpStatus == 400) {
                gLogger->error("Azure bad request: " + errorData.toStdString());
                emit recognitionError(tr("Invalid request to Azure. Check audio format."));
            } else if (httpStatus == 401) {
                gLogger->error("Azure unauthorized: " + errorData.toStdString());
                emit recognitionError(tr("Unauthorized access to Azure. Your token may have expired."));
            } else if (httpStatus == 403) {
                gLogger->error("Azure forbidden: " + errorData.toStdString());
                emit recognitionError(tr("Access to Azure forbidden. Quota may be exceeded."));
            } else if (httpStatus == 429) {
                gLogger->error("Azure rate limit: " + errorData.toStdString());
                emit recognitionError(tr("Azure rate limit exceeded. Please try again later."));
            } else if (httpStatus >= 500 && httpStatus < 600) {
                gLogger->error("Azure server error: " + errorData.toStdString());
                emit recognitionError(tr("Azure server error. Please try again later."));
            } else {
                gLogger->error("Azure request failed: " + errorString.toStdString() + " - " + errorData.toStdString());
                emit recognitionError(tr("Azure request failed: %1").arg(errorString));
            }
            
            // Проверка на исчерпание квоты для общего ключа
            if (m_useSharedApiKey && (httpStatus == 403 || httpStatus == 429)) {
                gLogger->error("Shared Azure API key has exceeded its quota limit");
                emit recognitionError(tr("The shared Azure API access has reached its quota limit. Consider using your own API key."));
            }
        }
    } else {
        // Таймаут
        reply->abort();
        gLogger->error("Azure API request timed out");
        emit recognitionError(tr("Azure API request timed out. Server might be busy or unavailable."));
    }
    
    reply->deleteLater();
    return result;
}

QString SpeechRecognizer::transcribeOffline(const QByteArray& audioData)
{
    gLogger->info("Using offline speech recognition");
    
#ifdef HAVE_WHISPER
    if (m_whisperContext == nullptr) {
        // Загрузить модель, если она еще не загружена
        loadLanguageModel();
        
        if (m_whisperContext == nullptr) {
            gLogger->error("Failed to load whisper model");
            emit recognitionError(tr("Не удалось загрузить модель для распознавания речи. Проверьте папку models."));
            return QString();
        }
    }
    
    // Проверяем, что аудио данные не пустые
    if (audioData.isEmpty()) {
        gLogger->warning("Empty audio data");
        return QString();
    }
    
    try {
        // Конвертировать QByteArray в массив float
        const float* samples = reinterpret_cast<const float*>(audioData.constData());
        int sampleCount = audioData.size() / sizeof(float);
        
        // Создаем параметры для Whisper
        whisper_full_params wparams = whisper_full_default_params(WHISPER_SAMPLING_GREEDY);
        
        // Настраиваем язык
        if (m_languageCode.startsWith("ru")) {
            wparams.language = "ru";
        } else if (m_languageCode.startsWith("en")) {
            wparams.language = "en";
        } else {
            // По умолчанию автоопределение
            wparams.language = nullptr;
        }
        
        // Дополнительные настройки
        wparams.print_realtime   = false;
        wparams.print_progress   = false;
        wparams.print_timestamps = false;
        wparams.translate        = false;
        wparams.single_segment   = false;
        wparams.max_tokens       = 0;
        wparams.speed_up         = true;
        
        // Выполняем распознавание
        if (whisper_full(m_whisperContext, wparams, samples, sampleCount) != 0) {
            gLogger->error("Failed to process audio with Whisper");
            emit recognitionError(tr("Ошибка при обработке аудио. Проверьте настройки модели."));
            return QString();
        }
        
        // Получаем количество сегментов
        int numSegments = whisper_full_n_segments(m_whisperContext);
        
        // Собираем результат
        QString result;
        for (int i = 0; i < numSegments; ++i) {
            const char* text = whisper_full_get_segment_text(m_whisperContext, i);
            result += QString::fromUtf8(text);
        }
        
        return result;
    } catch (const std::exception& e) {
        gLogger->error("Exception in offline transcription: " + std::string(e.what()));
        emit recognitionError(tr("Ошибка при распознавании речи: %1").arg(e.what()));
        return QString();
    }
#else
    // Whisper не установлен
    emit recognitionError(tr("Библиотека Whisper не установлена. Для работы оффлайн-распознавания необходимо установить библиотеку Whisper и модели языков.\n\nПодробнее: https://github.com/ggerganov/whisper.cpp"));
    
    return QString();
#endif
}

void SpeechRecognizer::loadLanguageModel()
{
#ifdef HAVE_WHISPER
    gLogger->info("Loading language model for offline recognition");
    
    // Очищаем предыдущий контекст, если он был
    if (m_whisperContext != nullptr) {
        whisper_free(m_whisperContext);
        m_whisperContext = nullptr;
    }
    
    // Определяем путь к моделям
    QSettings settings;
    QString modelPath = settings.value("recognition/model_path", "").toString();
    
    if (modelPath.isEmpty()) {
        // Используем стандартный путь
        modelPath = QDir::currentPath() + "/models/";
    }
    
    // Выбираем модель в зависимости от языка
    QString modelFile;
    
    // По размеру модели: tiny, base, small, medium, large
    QString modelSize = settings.value("recognition/model_size", "base").toString();
    
    if (m_languageCode.startsWith("ru")) {
        modelFile = modelPath + "whisper-" + modelSize + ".bin";
    } else {
        modelFile = modelPath + "whisper-" + modelSize + ".bin";
    }
    
    // Проверяем существование файла модели
    if (!QFile::exists(modelFile)) {
        gLogger->error("Model file not found: " + modelFile.toStdString());
        emit recognitionError(tr("Speech recognition model not found: %1").arg(modelFile));
        return;
    }
    
    // Инициализируем Whisper
    m_whisperContext = whisper_init_from_file(modelFile.toUtf8().constData());
    
    if (m_whisperContext == nullptr) {
        gLogger->error("Failed to load whisper model");
        emit recognitionError(tr("Failed to load speech recognition model"));
    } else {
        gLogger->info("Language model loaded successfully");
    }
#else
    gLogger->warning("Whisper not available, skipping model loading");
#endif
}

void SpeechRecognizer::initialize()
{
    gLogger->info("Initializing speech recognizer");
    
    // Создаем менеджер сетевых запросов
    m_networkManager = new QNetworkAccessManager(this);
    
    // Загружаем языковую модель для оффлайн распознавания
    loadLanguageModel();
    
    gLogger->info("Speech recognizer initialized");
}

void SpeechRecognizer::cleanup()
{
    gLogger->info("Cleaning up speech recognizer");
    
#ifdef HAVE_WHISPER
    // Освобождаем ресурсы Whisper
    if (m_whisperContext != nullptr) {
        whisper_free(m_whisperContext);
        m_whisperContext = nullptr;
    }
#endif
    
    // Освобождаем менеджер сетевых запросов
    if (m_networkManager) {
        m_networkManager->deleteLater();
        m_networkManager = nullptr;
    }
} 