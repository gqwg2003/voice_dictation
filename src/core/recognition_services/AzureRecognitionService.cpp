#include "AzureRecognitionService.h"
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

AzureRecognitionService::AzureRecognitionService(QObject* parent)
    : RecognitionService(parent)
    , m_region("westeurope")
    , m_publicApiEnabled(false)
{
    // Configure the temporary file
    m_tempFile.setFileTemplate(
        QStandardPaths::writableLocation(QStandardPaths::TempLocation) + 
        "/voicedictation_XXXXXX.wav"
    );
}

AzureRecognitionService::~AzureRecognitionService()
{
    // Temporary file is cleaned up automatically
}

bool AzureRecognitionService::initialize(const QString& apiKey, bool useSharedApiKey)
{
    gLogger->info("Initializing Azure Speech Service");
    
    m_apiKey = apiKey;
    m_useSharedApiKey = useSharedApiKey;
    
    // Load settings
    QSettings settings;
    m_publicApiEnabled = settings.value("speech/use_public_api", false).toBool();
    
    // Load region from settings
    QString region = settings.value("recognition/azureRegion", "westeurope").toString();
    if (!region.isEmpty()) {
        m_region = region;
    }
    
    // For Azure, we can use the public API even without a key
    m_isReady = !m_apiKey.isEmpty() || m_publicApiEnabled || m_useSharedApiKey;
    
    return m_isReady;
}

void AzureRecognitionService::setLanguage(const QString& languageCode)
{
    m_languageCode = languageCode;
}

QString AzureRecognitionService::getLanguage() const
{
    return m_languageCode;
}

QString AzureRecognitionService::transcribe(const std::vector<float>& audioData)
{
    if (audioData.empty()) {
        gLogger->warning("Empty audio data provided for transcription");
        return QString();
    }
    
    gLogger->info("Using Microsoft Azure Speech Service");
    
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
        apiKey = "FREE_API";
    }
    
    // If using public API free service
    if (apiKey == "FREE_API") {
        // Use alternative free service for speech recognition
        QUrl url("https://speech-service-public.eastus.azurecontainer.io/speech/recognition");
        QNetworkRequest request(url);
        request.setHeader(QNetworkRequest::ContentTypeHeader, "audio/wav");
        
        // Add query parameters
        QUrlQuery query;
        query.addQueryItem("lang", convertLanguageCodeToAzureFormat(m_languageCode));
        query.addQueryItem("public_access", "true");
        url.setQuery(query);
        request.setUrl(url);
        
        // Read the audio file
        QFile file(audioFilePath);
        if (!file.open(QIODevice::ReadOnly)) {
            gLogger->error("Failed to open audio file for Azure API");
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
                
                if (doc.isObject() && doc.object().contains("DisplayText")) {
                    result = doc.object()["DisplayText"].toString();
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
        gLogger->error("No Azure Speech API key available");
        emit recognitionError(tr("Azure Speech API key is missing"));
        return QString();
    }
    
    // For shared key, limit audio size
    if (m_useSharedApiKey) {
        // Limit audio to 15 seconds
        QFile file(audioFilePath);
        if (file.open(QIODevice::ReadOnly)) {
            QByteArray audioDataBytes = file.readAll();
            file.close();
            
            const int MAX_SHARED_AUDIO_SIZE = 240000; // ~15 seconds at 16 kHz, 16-bit mono
            if (audioDataBytes.size() > MAX_SHARED_AUDIO_SIZE) {
                if (file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
                    file.write(audioDataBytes.left(MAX_SHARED_AUDIO_SIZE));
                    file.close();
                    gLogger->info("Shared API mode: Limiting audio to 15 seconds to stay within quota");
                }
            }
        }
    }
    
    // Get region from settings or environment
    QString region = m_region;
    if (region.isEmpty()) {
        region = QString::fromLocal8Bit(qgetenv("AZURE_SPEECH_REGION"));
    }
    
    // For shared key, use eastus as default region
    if (region.isEmpty()) {
        if (m_useSharedApiKey) {
            region = "eastus";
        } else {
            gLogger->error("Azure region is not specified");
            emit recognitionError(tr("Azure region is not specified. Please set the region in settings."));
            return QString();
        }
    }
    
    // First get access token using API key
    QNetworkRequest tokenRequest;
    
    // Check key format - can be direct access token or subscription key
    bool isDirectToken = apiKey.startsWith("eyJ");  // JWT tokens usually start with eyJ
    bool isSubscriptionKey = !isDirectToken;
    
    QString accessToken;
    
    if (isSubscriptionKey) {
        // Use subscription key to get token
        tokenRequest.setUrl(QUrl(QString("https://%1.api.cognitive.microsoft.com/sts/v1.0/issueToken").arg(region)));
        tokenRequest.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
        tokenRequest.setRawHeader("Ocp-Apim-Subscription-Key", apiKey.toUtf8());
        
        QNetworkReply* tokenReply = m_networkManager->post(tokenRequest, QByteArray());
        
        // Wait for response with timeout
        QEventLoop tokenLoop;
        QObject::connect(tokenReply, &QNetworkReply::finished, &tokenLoop, &QEventLoop::quit);
        QTimer tokenTimer;
        tokenTimer.setSingleShot(true);
        QObject::connect(&tokenTimer, &QTimer::timeout, &tokenLoop, &QEventLoop::quit);
        tokenTimer.start(5000); // 5 second timeout
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
        
        // Get access token
        accessToken = tokenReply->readAll();
        tokenReply->deleteLater();
    } else {
        // Already have access token
        accessToken = apiKey;
    }
    
    if (accessToken.isEmpty()) {
        gLogger->error("Failed to obtain access token for Azure");
        emit recognitionError(tr("Failed to obtain access token for Azure"));
        return QString();
    }
    
    // Now send request for speech recognition
    QString endpointUrl = QString("https://%1.stt.speech.microsoft.com/speech/recognition/conversation/cognitiveservices/v1").arg(region);
    QUrl recognitionUrl(endpointUrl);
    QUrlQuery query;
    query.addQueryItem("language", convertLanguageCodeToAzureFormat(m_languageCode));
    query.addQueryItem("format", "detailed");
    
    // Additional parameters for better recognition
    query.addQueryItem("profanity", "raw"); // Don't filter profanity
    
    recognitionUrl.setQuery(query);
    
    QNetworkRequest recognitionRequest(recognitionUrl);
    recognitionRequest.setHeader(QNetworkRequest::ContentTypeHeader, "audio/wav; codec=audio/pcm; samplerate=16000");
    recognitionRequest.setRawHeader("Authorization", "Bearer " + accessToken.toUtf8());
    recognitionRequest.setRawHeader("Accept", "application/json");
    
    // Add useful options
    recognitionRequest.setRawHeader("X-Microsoft-OutputFormat", "Detailed");
    
    // Read the audio file
    QFile file(audioFilePath);
    if (!file.open(QIODevice::ReadOnly)) {
        gLogger->error("Failed to open audio file for Azure API");
        emit recognitionError(tr("Failed to open audio file for transcription"));
        return QString();
    }
    
    QByteArray audioDataBytes = file.readAll();
    file.close();
    
    QNetworkReply* reply = m_networkManager->post(recognitionRequest, audioDataBytes);
    
    // Wait for response with timeout
    QEventLoop loop;
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    QTimer timer;
    timer.setSingleShot(true);
    QObject::connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
    timer.start(15000); // 15 second timeout for larger audio files
    loop.exec();
    
    QString result;
    if (timer.isActive()) {
        // Got a response before timeout
        timer.stop();
        
        if (reply->error() == QNetworkReply::NoError) {
            QByteArray response = reply->readAll();
            QJsonDocument doc = QJsonDocument::fromJson(response);
            
            // Detailed analysis of JSON response from Azure
            if (doc.isObject()) {
                // Check main text
                if (doc.object().contains("DisplayText")) {
                    result = doc.object()["DisplayText"].toString();
                    gLogger->info("Azure transcription successful");
                } else if (doc.object().contains("NBest") && doc.object()["NBest"].isArray()) {
                    // Alternative NBest field contains recognition variants
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
                
                // Check for errors
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
                
                // Fallback to offline recognition if using public or shared API
                if (m_publicApiEnabled || m_useSharedApiKey) {
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
        gLogger->error("Azure API request timed out");
        emit recognitionError(tr("Azure API request timed out. Server might be busy or unavailable."));
        
        // Fallback to offline recognition if using public or shared API
        if (m_publicApiEnabled || m_useSharedApiKey) {
            gLogger->info("Falling back to offline recognition after timeout");
            return WhisperRecognitionService().transcribe(audioData);
        }
    }
    
    reply->deleteLater();
    return result;
}

bool AzureRecognitionService::isReady() const
{
    return m_isReady;
}

void AzureRecognitionService::setRegion(const QString& region)
{
    m_region = region;
    
    // Save to settings
    QSettings settings;
    settings.setValue("recognition/azureRegion", region);
}

QString AzureRecognitionService::getRegion() const
{
    return m_region;
}

bool AzureRecognitionService::isPublicApiEnabled() const
{
    return m_publicApiEnabled;
}

void AzureRecognitionService::setPublicApiEnabled(bool enabled)
{
    m_publicApiEnabled = enabled;
    
    // Save to settings
    QSettings settings;
    settings.setValue("speech/use_public_api", enabled);
}

QString AzureRecognitionService::saveToTemporaryFile(const std::vector<float>& audioData)
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

QString AzureRecognitionService::getActiveApiKey() const
{
    // If using a shared key, get it from environment or settings
    if (m_useSharedApiKey) {
        QString sharedKey = QString::fromLocal8Bit(qgetenv("AZURE_API_KEY_SHARED"));
        
        if (sharedKey.isEmpty()) {
            // Try to get from settings
            QSettings settings;
            sharedKey = settings.value("azure/shared_api_key", "").toString();
        }
        
        return sharedKey;
    }
    
    // Otherwise, use the provided key
    return m_apiKey;
}

bool AzureRecognitionService::handleApiError(int httpStatus, const QByteArray& errorData, const QString& errorString)
{
    // Handle different HTTP errors
    switch (httpStatus) {
        case 400:
            gLogger->error("Azure bad request: " + errorData.toStdString());
            emit recognitionError(tr("Invalid request to Azure. Check audio format."));
            break;
            
        case 401:
            gLogger->error("Azure unauthorized: " + errorData.toStdString());
            
            if (m_publicApiEnabled) {
                emit recognitionError(tr("Azure authorization error (public API). The public service may be unavailable."));
                return true; // Try offline
            } else if (m_useSharedApiKey) {
                emit recognitionError(tr("Azure authorization error (shared key). The token may have expired."));
                return true; // Try offline
            } else {
                emit recognitionError(tr("Unauthorized access to Azure. Your token may have expired."));
                return false;
            }
            
        case 403:
            gLogger->error("Azure forbidden: " + errorData.toStdString());
            
            if (m_publicApiEnabled) {
                emit recognitionError(tr("Access forbidden (Azure public API). Request limit may be exceeded."));
                return true; // Try offline
            } else if (m_useSharedApiKey) {
                emit recognitionError(tr("Access forbidden (Azure shared key). Request limit may be exceeded."));
                return true; // Try offline
            } else {
                emit recognitionError(tr("Access to Azure forbidden. Quota may be exceeded."));
                return false;
            }
            
        case 429:
            gLogger->error("Azure rate limit: " + errorData.toStdString());
            
            if (m_publicApiEnabled) {
                emit recognitionError(tr("Request limit exceeded (Azure public API)."));
                return true; // Try offline
            } else if (m_useSharedApiKey) {
                emit recognitionError(tr("Request limit exceeded (Azure shared key)."));
                return true; // Try offline
            } else {
                emit recognitionError(tr("Azure rate limit exceeded. Please try again later."));
                return false;
            }
            
        case 500:
        case 501:
        case 502:
        case 503:
        case 504:
            gLogger->error("Azure server error: " + errorData.toStdString());
            
            if (m_publicApiEnabled || m_useSharedApiKey) {
                emit recognitionError(tr("Azure server error. Trying offline recognition."));
                return true; // Try offline
            } else {
                emit recognitionError(tr("Azure server error. Please try again later."));
                return false;
            }
            
        default:
            gLogger->error("Azure request failed: " + errorString.toStdString() + " - " + errorData.toStdString());
            
            if (m_publicApiEnabled) {
                emit recognitionError(tr("Azure request error (public API). The public service may not be working."));
                return true; // Try offline
            } else if (m_useSharedApiKey) {
                emit recognitionError(tr("Azure request error (shared key). The shared key may not be working."));
                return true; // Try offline
            } else {
                emit recognitionError(tr("Azure request failed: %1").arg(errorString));
                return false;
            }
    }
    
    // Default to not falling back to offline
    return false;
}

QString AzureRecognitionService::convertLanguageCodeToAzureFormat(const QString& languageCode) const
{
    // Convert language code to Azure format
    if (languageCode.startsWith("ru")) {
        return "ru-RU";
    } else if (languageCode.startsWith("en")) {
        return "en-US";
    } else {
        // Support for other languages
        return languageCode;
    }
} 