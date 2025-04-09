#pragma once

#include "RecognitionService.h"
#include <QString>
#include <QSettings>

// Forward declaration for Whisper
#ifdef HAVE_WHISPER
struct whisper_context;
#endif

/**
 * @brief Implementation of offline speech recognition using Whisper
 */
class WhisperRecognitionService : public RecognitionService {
    Q_OBJECT

public:
    explicit WhisperRecognitionService(QObject* parent = nullptr);
    ~WhisperRecognitionService() override;

    bool initialize(const QString& apiKey = QString(), bool useSharedApiKey = false) override;
    void setLanguage(const QString& languageCode) override;
    QString getLanguage() const override;
    QString transcribe(const std::vector<float>& audioData) override;
    bool isReady() const override;

private:
    /**
     * @brief Load the language model for offline recognition
     * @return True if model was loaded successfully
     */
    bool loadLanguageModel();

    /**
     * @brief Get the path to the model file
     * @return Path to the model file
     */
    QString getModelPath() const;

private:
#ifdef HAVE_WHISPER
    whisper_context* m_whisperContext;
#else
    void* m_whisperContext; // Placeholder when Whisper is not available
#endif
    QString m_modelPath;
    QString m_modelSize;
}; 