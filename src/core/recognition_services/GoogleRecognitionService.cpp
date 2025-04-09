#include "GoogleRecognitionService.h"
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

GoogleRecognitionService::GoogleRecognitionService(QObject* parent)
    : RecognitionService(parent)
    , m_publicApiEnabled(false)
{
    // Configure the temporary file
    m_tempFile.setFileTemplate(
        QStandardPaths::writableLocation(QStandardPaths::TempLocation) + 
        "/voicedictation_XXXXXX.wav"
    );
}

GoogleRecognitionService::~GoogleRecognitionService()
{
    // Temporary file is cleaned up automatically
}

bool GoogleRecognitionService::initialize(const QString& apiKey, bool useSharedApiKey)
{
    gLogger->info("Initializing Google Cloud Speech recognition service");
    
    m_apiKey = apiKey;
    m_useSharedApiKey = useSharedApiKey;
    
    // Check settings for public API flag
    QSettings settings;
    m_publicApiEnabled = settings.value("speech/use_public_api", false).toBool();
    
    // For Google, always mark as ready since we can connect even without API key (for testing)
    m_isReady = true;
    
    return true;
}

void GoogleRecognitionService::setLanguage(const QString& languageCode)
{
    m_languageCode = languageCode;
}

QString GoogleRecognitionService::getLanguage() const
{
    return m_languageCode;
}

QString GoogleRecognitionService::transcribe(const std::vector<float>& audioData)
{
    if (audioData.empty()) {
        gLogger->warning("Empty audio data provided for transcription");
        return QString();
    }
    
    gLogger->info("Using Google Cloud Speech API");
    
    // Get API key
    QString apiKey = getActiveApiKey();
    
    // Check if we should use public API (free version)
    if (m_publicApiEnabled) {
        apiKey = "PUBLIC_API";
    }
    
    if (apiKey.isEmpty() && !m_publicApiEnabled) {
        gLogger->error("No Google API key available");
        emit recognitionError(tr("Google API key is missing"));
        return QString();
    }
    
    // Convert the audio data to a format suitable for Google Cloud Speech API
    QString audioBase64 = convertToBase64(audioData);
    
    if (audioBase64.isEmpty()) {
        gLogger->error("Failed to convert audio data to base64");
        emit recognitionError(tr("Failed to process audio data"));
        return QString();
    }
    
    // Determine if we're using OAuth or API key auth
    bool usingOAuth = apiKey.startsWith("ya29.") || apiKey.startsWith("Bearer ");
    QString auth = apiKey;
    
    // Create JSON request
    QJsonObject configObj;
    configObj["encoding"] = "LINEAR16";
    configObj["sampleRateHertz"] = 16000;
    configObj["languageCode"] = m_languageCode;
    configObj["enableWordTimeOffsets"] = false;
    
    // Add features
    QJsonObject featuresObj;
    featuresObj["enableAutomaticPunctuation"] = true;
    featuresObj["enableSpokenPunctuation"] = true;
    featuresObj["enableSpokenEmojis"] = true;
    configObj["speechContexts"] = QJsonArray::fromStringList(QStringList() << "," << "." << "!" << "?");
    
    // Add model selection
    configObj["model"] = "command_and_search"; // or "phone_call", "video", "default" depending on audio source
    
    QJsonObject audioObj;
    audioObj["content"] = QString::fromLatin1(audioBase64.toLatin1());
    
    QJsonObject requestObj;
    requestObj["config"] = configObj;
    requestObj["audio"] = audioObj;
    
    QJsonDocument requestDoc(requestObj);
    QByteArray requestData = requestDoc.toJson();
    
    // Create network request
    QUrl url;
    QNetworkRequest request;
    
    // Public API mode uses a different endpoint
    if (m_publicApiEnabled) {
        url = QUrl("https://speech-api-public.eastus.azurecontainer.io/speech/google");
        request.setUrl(url);
        request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
        
        // Add query params
        QUrlQuery query;
        query.addQueryItem("lang", m_languageCode);
        query.addQueryItem("public_access", "true");
        url.setQuery(query);
        request.setUrl(url);
    } else {
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
    }
    
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
                        
                        // If using public API, give a clear message
                        if (m_publicApiEnabled) {
                            emit recognitionError(tr("Google API error (public API): %1.\nThe public API may be unavailable. Try using your own API key.").arg(errorMsg));
                            return QString();
                        } else if (m_useSharedApiKey) {
                            emit recognitionError(tr("Google API error (shared API key): %1.\nThe shared API key may not be working. Try using your own API key.").arg(errorMsg));
                            return QString();
                        } else {
                            emit recognitionError(tr("Google API error: %1 - %2").arg(errorCode).arg(errorMsg));
                            return QString();
                        }
                    } else {
                        gLogger->error("Google API returned empty result with no error details");
                        
                        if (m_publicApiEnabled) {
                            emit recognitionError(tr("Google API returned no results (public API). The public API may not be working."));
                            return QString();
                        } else if (m_useSharedApiKey) {
                            emit recognitionError(tr("Google API returned no results (shared key). The shared API key may not be working."));
                            return QString();
                        } else {
                            emit recognitionError(tr("Google API returned no transcription results. Audio may be too noisy or unclear."));
                            return QString();
                        }
                    }
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
        gLogger->error("Google API request timed out");
        emit recognitionError(tr("Google API request timed out. Server might be busy or unavailable."));
        
        // Fallback to offline recognition if using public or shared API
        if (m_publicApiEnabled || m_useSharedApiKey) {
            gLogger->info("Falling back to offline recognition after timeout");
            return WhisperRecognitionService().transcribe(audioData);
        }
    }
    
    reply->deleteLater();
    return result;
}

bool GoogleRecognitionService::isReady() const
{
    return m_isReady;
}

bool GoogleRecognitionService::isPublicApiEnabled() const
{
    return m_publicApiEnabled;
}

void GoogleRecognitionService::setPublicApiEnabled(bool enabled)
{
    m_publicApiEnabled = enabled;
    
    // Save to settings
    QSettings settings;
    settings.setValue("speech/use_public_api", enabled);
}

QString GoogleRecognitionService::saveToTemporaryFile(const std::vector<float>& audioData)
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

QString GoogleRecognitionService::convertToBase64(const std::vector<float>& audioData)
{
    // Save to temporary WAV file first
    QString tempFilePath = saveToTemporaryFile(audioData);
    if (tempFilePath.isEmpty()) {
        return QString();
    }
    
    // Read the file and convert to base64
    QFile file(tempFilePath);
    if (!file.open(QIODevice::ReadOnly)) {
        gLogger->error("Failed to reopen temporary audio file");
        return QString();
    }
    
    QByteArray fileDataBytes = file.readAll();
    file.close();
    
    return fileDataBytes.toBase64();
}

QString GoogleRecognitionService::getActiveApiKey() const
{
    // If using a shared key, get it from environment or settings
    if (m_useSharedApiKey) {
        QString sharedKey = QString::fromLocal8Bit(qgetenv("GOOGLE_API_KEY_SHARED"));
        
        if (sharedKey.isEmpty()) {
            // Try to get from settings
            QSettings settings;
            sharedKey = settings.value("google/shared_api_key", "").toString();
        }
        
        return sharedKey;
    }
    
    // Otherwise, use the provided key
    return m_apiKey;
}

bool GoogleRecognitionService::handleApiError(int httpStatus, const QByteArray& errorData, const QString& errorString)
{
    // Handle different HTTP errors
    switch (httpStatus) {
        case 400:
            gLogger->error("Google API bad request: " + errorData.toStdString());
            emit recognitionError(tr("Invalid request to Google API. Check audio format."));
            break;
            
        case 401:
            gLogger->error("Google API unauthorized: " + errorData.toStdString());
            
            if (m_publicApiEnabled) {
                emit recognitionError(tr("Google API authorization error (public API). The public service may be unavailable."));
                return true; // Try offline
            } else if (m_useSharedApiKey) {
                emit recognitionError(tr("Google API authorization error (shared key). The shared key may be invalid."));
                return true; // Try offline
            } else {
                emit recognitionError(tr("Unauthorized access to Google API. Check your API key or credentials."));
                return false;
            }
            
        case 403:
            gLogger->error("Google API forbidden: " + errorData.toStdString());
            
            if (m_publicApiEnabled) {
                emit recognitionError(tr("Access forbidden (Google public API). Request limit may be exceeded."));
                return true; // Try offline
            } else if (m_useSharedApiKey) {
                emit recognitionError(tr("Access forbidden (Google shared key). Request limit may be exceeded."));
                return true; // Try offline
            } else {
                emit recognitionError(tr("Access to Google API forbidden. Quota may be exceeded."));
                return false;
            }
            
        case 429:
            gLogger->error("Google API rate limit: " + errorData.toStdString());
            
            if (m_publicApiEnabled) {
                emit recognitionError(tr("Request limit exceeded (Google public API)."));
                return true; // Try offline
            } else if (m_useSharedApiKey) {
                emit recognitionError(tr("Request limit exceeded (Google shared key)."));
                return true; // Try offline
            } else {
                emit recognitionError(tr("Google API rate limit exceeded. Please try again later."));
                return false;
            }
            
        case 500:
        case 501:
        case 502:
        case 503:
        case 504:
            gLogger->error("Google API server error: " + errorData.toStdString());
            
            if (m_publicApiEnabled || m_useSharedApiKey) {
                emit recognitionError(tr("Google API server error. Trying offline recognition."));
                return true; // Try offline
            } else {
                emit recognitionError(tr("Google API server error. Please try again later."));
                return false;
            }
            
        default:
            gLogger->error("Google API request failed: " + errorString.toStdString() + " - " + errorData.toStdString());
            
            if (m_publicApiEnabled) {
                emit recognitionError(tr("Google API request error (public API). The public service may not be working."));
                return true; // Try offline
            } else if (m_useSharedApiKey) {
                emit recognitionError(tr("Google API request error (shared key). The shared key may not be working."));
                return true; // Try offline
            } else {
                emit recognitionError(tr("Google API request failed: %1").arg(errorString));
                return false;
            }
    }
    
    // Default to not falling back to offline
    return false;
} 