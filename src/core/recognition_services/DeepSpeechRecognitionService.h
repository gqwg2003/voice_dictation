#pragma once

#include "RecognitionService.h"
#include <memory>
#include <string>
#include <vector>

// Условная компиляция для DeepSpeech
#ifdef HAVE_DEEPSPEECH
#include <deepspeech.h>
#endif

// Forward declarations
namespace DeepSpeech {
    class Model;
    struct StreamingState;
}

class DeepSpeechRecognitionService : public RecognitionService {
public:
    DeepSpeechRecognitionService(QObject* parent = nullptr);
    ~DeepSpeechRecognitionService() override;

    bool initialize(const QString& apiKey = QString(), bool useSharedApiKey = false) override;
    void setLanguage(const QString& languageCode) override;
    QString getLanguage() const override;
    QString transcribe(const std::vector<float>& audioData) override;
    bool isReady() const override;

private:
    bool loadModel();
    void unloadModel();
    std::string getModelPath(const QString& language) const;
    bool processSpeech(const std::vector<float>& audioData);
    bool finalizeSpeech();

private:
#ifdef HAVE_DEEPSPEECH
    std::unique_ptr<DeepSpeech::Model> m_model;
    std::unique_ptr<DeepSpeech::StreamingState> m_streamingState;
#else
    // Заглушки для компиляции без DeepSpeech
    void* m_model = nullptr;
    void* m_streamingState = nullptr;
#endif
    QString m_language;
    bool m_isInitialized;
    int m_sampleRate;
    int m_channels;
}; 