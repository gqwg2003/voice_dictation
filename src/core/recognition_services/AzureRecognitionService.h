#pragma once

#include "RecognitionService.h"
#include <QString>
#include <QTemporaryFile>

/**
 * @brief Implementation of Microsoft Azure Speech Service for speech recognition
 */
class AzureRecognitionService : public RecognitionService {
    Q_OBJECT

public:
    explicit AzureRecognitionService(QObject* parent = nullptr);
    ~AzureRecognitionService() override;

    bool initialize(const QString& apiKey = QString(), bool useSharedApiKey = false) override;
    void setLanguage(const QString& languageCode) override;
    QString getLanguage() const override;
    QString transcribe(const std::vector<float>& audioData) override;
    bool isReady() const override;
    
    /**
     * @brief Set the Azure region for the service
     * @param region The Azure region (e.g., "westeurope", "eastus")
     */
    void setRegion(const QString& region);
    
    /**
     * @brief Get the current Azure region
     * @return The current Azure region
     */
    QString getRegion() const;
    
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
    
    /**
     * @brief Convert the language code to Azure format
     * @param languageCode Original language code
     * @return Azure format language code
     */
    QString convertLanguageCodeToAzureFormat(const QString& languageCode) const;

private:
    QString m_region;
    bool m_publicApiEnabled;
    QTemporaryFile m_tempFile;
}; 