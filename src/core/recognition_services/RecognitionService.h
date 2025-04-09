#pragma once

#include <QObject>
#include <QString>
#include <vector>

// Forward declaration
class QNetworkAccessManager;

/**
 * @brief Base class for all speech recognition services
 * 
 * This abstract class defines the interface for all speech recognition services.
 * Each service (Google, Azure, Whisper, etc.) should implement this interface.
 */
class RecognitionService : public QObject {
    Q_OBJECT

public:
    explicit RecognitionService(QObject* parent = nullptr);
    virtual ~RecognitionService();

    /**
     * @brief Initialize the service with required parameters
     * @param apiKey API key for the service (if required)
     * @param useSharedApiKey Whether to use a shared API key
     * @return True if initialization was successful
     */
    virtual bool initialize(const QString& apiKey = QString(), bool useSharedApiKey = false) = 0;

    /**
     * @brief Set the language for speech recognition
     * @param languageCode ISO language code (e.g., "en-US", "ru-RU")
     */
    virtual void setLanguage(const QString& languageCode) = 0;

    /**
     * @brief Get the current language code
     * @return Current language code
     */
    virtual QString getLanguage() const = 0;

    /**
     * @brief Transcribe audio data to text
     * @param audioData The audio data to transcribe
     * @return Transcribed text
     */
    virtual QString transcribe(const std::vector<float>& audioData) = 0;

    /**
     * @brief Check if the service is ready for transcription
     * @return True if the service is ready
     */
    virtual bool isReady() const = 0;

signals:
    /**
     * @brief Signal emitted when an error occurs during recognition
     * @param errorMessage The error message
     */
    void recognitionError(const QString& errorMessage);

protected:
    QNetworkAccessManager* m_networkManager;
    QString m_languageCode;
    QString m_apiKey;
    bool m_useSharedApiKey;
    bool m_isReady;
}; 