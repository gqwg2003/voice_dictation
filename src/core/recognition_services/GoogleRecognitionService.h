#pragma once

#include "RecognitionService.h"
#include <QTemporaryFile>

/**
 * @brief Implementation of Google Cloud Speech API for speech recognition
 */
class GoogleRecognitionService : public RecognitionService {
    Q_OBJECT

public:
    explicit GoogleRecognitionService(QObject* parent = nullptr);
    ~GoogleRecognitionService() override;

    bool initialize(const QString& apiKey = QString(), bool useSharedApiKey = false) override;
    void setLanguage(const QString& languageCode) override;
    QString getLanguage() const override;
    QString transcribe(const std::vector<float>& audioData) override;
    bool isReady() const override;
    
    /**
     * @brief Get the public API version (free version) status
     * @return True if public API mode is enabled
     */
    bool isPublicApiEnabled() const;
    
    /**
     * @brief Toggle the use of public API (free version with limitations)
     * @param enabled True to enable public API usage
     */
    void setPublicApiEnabled(bool enabled);

private:
    /**
     * @brief Save audio data to a temporary WAV file
     * @param audioData The audio data to save
     * @return Path to the temporary file or empty string on failure
     */
    QString saveToTemporaryFile(const std::vector<float>& audioData);
    
    /**
     * @brief Convert audio data to base64 format
     * @param audioData The audio data to convert
     * @return Base64-encoded audio data
     */
    QString convertToBase64(const std::vector<float>& audioData);
    
    /**
     * @brief Get the active API key (personal or shared)
     * @return API key to use for requests
     */
    QString getActiveApiKey() const;
    
    /**
     * @brief Handle errors from the API response
     * @param httpStatus HTTP status code
     * @param errorData Error data from the response
     * @param errorString Error string from the network reply
     * @return Whether to fallback to offline recognition
     */
    bool handleApiError(int httpStatus, const QByteArray& errorData, const QString& errorString);

private:
    bool m_publicApiEnabled;
    QTemporaryFile m_tempFile;
}; 