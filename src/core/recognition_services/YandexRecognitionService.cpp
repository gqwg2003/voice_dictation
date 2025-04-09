#include "YandexRecognitionService.h"
#include "../../utils/Logger.h"
#include "WhisperRecognitionService.h"

#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>
#include <QUrlQuery>
#include <QEventLoop>
#include <QTimer>
#include <QSettings>
#include <QStandardPaths>

// External global logger
extern Logger* gLogger;

YandexRecognitionService::YandexRecognitionService(QObject* parent)
    : RecognitionService(parent)
    , m_publicApiEnabled(false)
{
    // Configure the temporary file
    m_tempFile.setFileTemplate(
        QStandardPaths::writableLocation(QStandardPaths::TempLocation) + 
        "/voicedictation_XXXXXX.wav"
    );
}

YandexRecognitionService::~YandexRecognitionService()
{
    // Temporary file is cleaned up automatically
}

bool YandexRecognitionService::initialize(const QString& apiKey, bool useSharedApiKey)
{
    gLogger->info("Initializing Yandex SpeechKit service");
    
    m_apiKey = apiKey;
    m_useSharedApiKey = useSharedApiKey;
    
    // Check settings for public API flag
    QSettings settings;
    m_publicApiEnabled = settings.value("speech/use_public_api", false).toBool();
    
    // For Yandex, we need an API key unless using public API
    m_isReady = !m_apiKey.isEmpty() || m_publicApiEnabled || m_useSharedApiKey;
    
    return m_isReady;
}

void YandexRecognitionService::setLanguage(const QString& languageCode)
{
    m_languageCode = languageCode;
}

QString YandexRecognitionService::getLanguage() const
{
    return m_languageCode;
}

QString YandexRecognitionService::transcribe(const std::vector<float>& audioData)
{
    if (audioData.empty()) {
        gLogger->warning("Empty audio data provided for transcription");
        return QString();
    }
    
    gLogger->info("Using Yandex SpeechKit");
    
    // Save audio to temporary file
    QString audioFilePath = saveToTemporaryFile(audioData);
    if (audioFilePath.isEmpty()) {
        gLogger->error("Failed to save audio data to temporary file");
        emit recognitionError(tr("Failed to prepare audio data for transcription"));
        return QString();
    }
    
    // Get API key
    QString apiKey = getActiveApiKey();
    
    // Check if we should use public API (free version)
    if (m_publicApiEnabled) {
        apiKey = "PUBLIC_API";
    }
    
    // If using public API free service
    if (apiKey == "PUBLIC_API") {
        // Use alternative free service for speech recognition
        QUrl url("https://speech-service-public.eastus.azurecontainer.io/speech/yandex");
        QNetworkRequest request(url);
        request.setHeader(QNetworkRequest::ContentTypeHeader, "audio/wav");
        
        // Add query parameters
        QUrlQuery query;
        query.addQueryItem("lang", convertLanguageCodeToYandexFormat(m_languageCode));
        query.addQueryItem("public_access", "true");
        url.setQuery(query);
        request.setUrl(url);
        
        // Read the audio file
        QFile file(audioFilePath);
        if (!file.open(QIODevice::ReadOnly)) {
            gLogger->error("Failed to open audio file for Yandex API");
            emit recognitionError(tr("Failed to open audio file for transcription"));
            return QString();
        }
        
        QByteArray audioDataBytes = file.readAll();
        file.close();
        
        // Limit audio size for public API
        const int MAX_PUBLIC_AUDIO_SIZE = 120000; // Limit for public API
        if (audioDataBytes.size() > MAX_PUBLIC_AUDIO_SIZE) {
            audioDataBytes = audioDataBytes.left(MAX_PUBLIC_AUDIO_SIZE);
            gLogger->info("Public API: Limiting audio to protect server resources");
        }
        
        // Send request
        QNetworkReply* reply = m_networkManager->post(request, audioDataBytes);
        
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
            timer.stop();
            
            if (reply->error() == QNetworkReply::NoError) {
                // Successful response from public API
                QByteArray response = reply->readAll();
                QJsonDocument doc = QJsonDocument::fromJson(response);
                
                if (doc.isObject() && doc.object().contains("result")) {
                    result = doc.object()["result"].toString();
                } else {
                    emit recognitionError(tr("Failed to recognize speech (free API). Try to speak more clearly."));
                }
            } else {
                // Error in public API
                emit recognitionError(tr("Error in free recognition service. Try again later or use your own API key."));
                
                // Log error for additional information
                gLogger->error("Public API error: " + reply->errorString().toStdString());
            }
        } else {
            // Timeout - public service not responding
            reply->abort();
            emit recognitionError(tr("Timeout waiting for response from free service. The service may be overloaded."));
        }
        
        reply->deleteLater();
        return result;
    }
    
    // Standard code for using personal API key
    if (apiKey.isEmpty()) {
        gLogger->error("No Yandex SpeechKit API key available");
        emit recognitionError(tr("Yandex SpeechKit API key is missing"));
        return QString();
    }
    
    // Read the audio file
    QFile file(audioFilePath);
    if (!file.open(QIODevice::ReadOnly)) {
        gLogger->error("Failed to open audio file for Yandex API");
        emit recognitionError(tr("Failed to open audio file for transcription"));
        return QString();
    }
    
    QByteArray audioFileData = file.readAll();
    file.close();
    
    // For shared key, limit audio size
    if (m_useSharedApiKey) {
        // Limit audio to 15 seconds
        const int MAX_SHARED_AUDIO_SIZE = 240000; // ~15 seconds at 16 kHz, 16-bit mono
        if (audioFileData.size() > MAX_SHARED_AUDIO_SIZE) {
            audioFileData = audioFileData.left(MAX_SHARED_AUDIO_SIZE);
            gLogger->info("Shared API mode: Limiting audio to 15 seconds to stay within quota");
        }
    }
    
    // Create the URL for the Yandex SpeechKit API
    QUrl url("https://stt.api.cloud.yandex.net/speech/v1/stt:recognize");
    QNetworkRequest request(url);
    
    // Set up the request headers
    request.setHeader(QNetworkRequest::ContentTypeHeader, "audio/x-pcm;bit=16;rate=16000");
    request.setRawHeader("Authorization", "Api-Key " + apiKey.toUtf8());
    
    // Add language parameter to URL
    QUrlQuery query;
    query.addQueryItem("lang", convertLanguageCodeToYandexFormat(m_languageCode));
    query.addQueryItem("format", "lpcm");
    query.addQueryItem("sampleRateHertz", "16000");
    url.setQuery(query);
    request.setUrl(url);
    
    // Send the request
    QNetworkReply* reply = m_networkManager->post(request, audioFileData);
    
    // Wait for the response
    QEventLoop loop;
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    QTimer timer;
    timer.setSingleShot(true);
    QObject::connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
    timer.start(15000); // 15 second timeout
    loop.exec();
    
    QString result;
    if (timer.isActive()) {
        // Got a response before timeout
        timer.stop();
        
        if (reply->error() == QNetworkReply::NoError) {
            QByteArray response = reply->readAll();
            QJsonDocument doc = QJsonDocument::fromJson(response);
            
            if (doc.isObject() && doc.object().contains("result")) {
                result = doc.object()["result"].toString();
                gLogger->info("Yandex transcription successful");
            } else {
                gLogger->warning("Yandex returned response but no transcription was found");
                emit recognitionError(tr("No speech recognized by Yandex. Try speaking more clearly."));
                
                // Fallback to offline recognition if using shared API
                if (m_useSharedApiKey) {
                    gLogger->info("Falling back to offline recognition after empty result");
                    return WhisperRecognitionService().transcribe(audioData);
                }
            }
        } else {
            int httpStatus = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
            QByteArray errorData = reply->readAll();
            return handleApiError(httpStatus, errorData, reply->errorString()) ? 
                   WhisperRecognitionService().transcribe(audioData) : QString();
        }
    } else {
        // Timeout
        reply->abort();
        gLogger->error("Yandex API request timed out");
        emit recognitionError(tr("Yandex API request timed out. Server might be busy or unavailable."));
        
        // Fallback to offline recognition if using public or shared API
        if (m_publicApiEnabled || m_useSharedApiKey) {
            gLogger->info("Falling back to offline recognition after timeout");
            return WhisperRecognitionService().transcribe(audioData);
        }
    }
    
    reply->deleteLater();
    return result;
}

bool YandexRecognitionService::isReady() const
{
    return m_isReady;
}

bool YandexRecognitionService::isPublicApiEnabled() const
{
    return m_publicApiEnabled;
}

void YandexRecognitionService::setPublicApiEnabled(bool enabled)
{
    m_publicApiEnabled = enabled;
    
    // Save to settings
    QSettings settings;
    settings.setValue("speech/use_public_api", enabled);
}

QString YandexRecognitionService::saveToTemporaryFile(const std::vector<float>& audioData)
{
    if (!m_tempFile.open()) {
        gLogger->error("Failed to create temporary file for audio data");
        return QString();
    }
    
    // Create WAV header
    const int sampleRate = 16000;
    const int channels = 1;
    const int bitsPerSample = 16;
    const int dataSize = static_cast<int>(audioData.size() * sizeof(int16_t));
    
    // RIFF header
    m_tempFile.write("RIFF", 4);
    uint32_t fileSize = 36 + dataSize;
    m_tempFile.write(reinterpret_cast<const char*>(&fileSize), 4);
    m_tempFile.write("WAVE", 4);
    
    // Format chunk
    m_tempFile.write("fmt ", 4);
    uint32_t fmtSize = 16;
    m_tempFile.write(reinterpret_cast<const char*>(&fmtSize), 4);
    uint16_t audioFormat = 1; // PCM
    m_tempFile.write(reinterpret_cast<const char*>(&audioFormat), 2);
    m_tempFile.write(reinterpret_cast<const char*>(&channels), 2);
    m_tempFile.write(reinterpret_cast<const char*>(&sampleRate), 4);
    uint32_t byteRate = sampleRate * channels * bitsPerSample / 8;
    m_tempFile.write(reinterpret_cast<const char*>(&byteRate), 4);
    uint16_t blockAlign = channels * bitsPerSample / 8;
    m_tempFile.write(reinterpret_cast<const char*>(&blockAlign), 2);
    m_tempFile.write(reinterpret_cast<const char*>(&bitsPerSample), 2);
    
    // Data chunk
    m_tempFile.write("data", 4);
    m_tempFile.write(reinterpret_cast<const char*>(&dataSize), 4);
    
    // Convert float to int16_t and write samples
    for (float sample : audioData) {
        int16_t pcmSample = static_cast<int16_t>(sample * 32767.0f);
        m_tempFile.write(reinterpret_cast<const char*>(&pcmSample), sizeof(int16_t));
    }
    
    m_tempFile.flush();
    return m_tempFile.fileName();
}

QString YandexRecognitionService::getActiveApiKey() const
{
    // If using a shared key, get it from environment or settings
    if (m_useSharedApiKey) {
        QString sharedKey = QString::fromLocal8Bit(qgetenv("YANDEX_API_KEY_SHARED"));
        
        if (sharedKey.isEmpty()) {
            // Try to get from settings
            QSettings settings;
            sharedKey = settings.value("yandex/shared_api_key", "").toString();
        }
        
        return sharedKey;
    }
    
    // Otherwise, use the provided key
    return m_apiKey;
}

bool YandexRecognitionService::handleApiError(int httpStatus, const QByteArray& errorData, const QString& errorString)
{
    // Handle different HTTP errors
    switch (httpStatus) {
        case 400:
            gLogger->error("Yandex API bad request: " + errorData.toStdString());
            emit recognitionError(tr("Invalid request to Yandex API. Check audio format."));
            break;
            
        case 401:
            gLogger->error("Yandex API unauthorized: " + errorData.toStdString());
            
            if (m_useSharedApiKey) {
                emit recognitionError(tr("Unauthorized access to Yandex API (shared key). The key may be invalid."));
                return true; // Try offline
            } else {
                emit recognitionError(tr("Unauthorized access to Yandex API. Check your API key."));
                return false;
            }
            
        case 403:
            gLogger->error("Yandex API forbidden: " + errorData.toStdString());
            
            if (m_useSharedApiKey) {
                emit recognitionError(tr("Access forbidden to Yandex API (shared key). Request limit may be exceeded."));
                return true; // Try offline
            } else {
                emit recognitionError(tr("Access forbidden to Yandex API. Quota may be exceeded."));
                return false;
            }
            
        case 429:
            gLogger->error("Yandex API rate limit: " + errorData.toStdString());
            
            if (m_useSharedApiKey) {
                emit recognitionError(tr("Rate limit exceeded (Yandex API shared key)."));
                return true; // Try offline
            } else {
                emit recognitionError(tr("Yandex API rate limit exceeded. Please try again later."));
                return false;
            }
            
        case 500:
        case 501:
        case 502:
        case 503:
        case 504:
            gLogger->error("Yandex API server error: " + errorData.toStdString());
            
            if (m_useSharedApiKey) {
                emit recognitionError(tr("Yandex API server error. Trying offline recognition."));
                return true; // Try offline
            } else {
                emit recognitionError(tr("Yandex API server error. Please try again later."));
                return false;
            }
            
        default:
            gLogger->error("Yandex API request failed: " + errorString.toStdString() + " - " + errorData.toStdString());
            
            if (m_useSharedApiKey) {
                emit recognitionError(tr("Yandex API request error (shared key). The key may not be working."));
                return true; // Try offline
            } else {
                emit recognitionError(tr("Yandex API request failed: %1").arg(errorString));
                return false;
            }
    }
    
    // Default to not falling back to offline
    return false;
}

QString YandexRecognitionService::convertLanguageCodeToYandexFormat(const QString& languageCode) const
{
    // Convert language code to Yandex format
    if (languageCode.startsWith("ru")) {
        return "ru-RU";
    } else if (languageCode.startsWith("en")) {
        return "en-US";
    } else {
        // Support for other languages
        return languageCode;
    }
} 