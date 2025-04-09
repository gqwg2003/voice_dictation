#pragma once

#include <QObject>
#include <QString>
#include <QThread>
#include <memory>
#include <vector>
#include <atomic>
#include <mutex>
#include <condition_variable>

// Forward declarations
class AudioProcessor;
class RecognitionService;

// Speech recognition service types
enum class RecognitionServiceType {
    Offline,  // Whisper
    Google,
    Yandex,
    Azure
};

class SpeechRecognizer : public QObject {
    Q_OBJECT

public:
    explicit SpeechRecognizer(AudioProcessor* audioProcessor, QObject* parent = nullptr);
    ~SpeechRecognizer();

    // Control methods
    void startRecognition();
    void stopRecognition();
    
    // Configuration
    void setLanguage(const QString& languageCode);
    QString getLanguage() const;
    
    // Cloud API configuration
    void setRecognitionService(RecognitionServiceType service);
    void setApiKey(const QString& apiKey);
    void setUseSharedApiKey(bool useShared);
    void setUsePublicApi(bool usePublic);
    void setAzureRegion(const QString& region);
    
    // State getters
    RecognitionServiceType getRecognitionService() const;
    bool isUsingSharedApiKey() const;
    bool isUsingPublicApi() const;
    QString getAzureRegion() const;
    bool isRunning() const;
    
    // Public method for processing audio data
    void processSpeech(const std::vector<float>& audioData);

signals:
    void speechRecognized(const QString& text);
    void recognitionStarted();
    void recognitionStopped();
    void recognitionError(const QString& errorMessage);

private:
    // Recognition worker thread
    class RecognitionThread : public QThread {
    public:
        RecognitionThread(SpeechRecognizer* recognizer);
        ~RecognitionThread();
        
        void run() override;
        void stop();
        
    private:
        SpeechRecognizer* m_recognizer;
        std::atomic<bool> m_stopRequested;
    };
    
    // Recognition methods
    void processAudioChunk();
    QString transcribeAudio(const std::vector<float>& audioData);
    
    // Service management
    void createRecognitionService();
    
    // Internal helpers
    void initialize();
    void cleanup();

private:
    // Dependencies
    AudioProcessor* m_audioProcessor;
    
    // Recognition service
    std::unique_ptr<RecognitionService> m_recognitionService;
    
    // State
    std::atomic<bool> m_isRunning;
    QString m_languageCode;
    RecognitionServiceType m_recognitionServiceType;
    QString m_apiKey;
    bool m_useSharedApiKey;
    bool m_usePublicApi;
    QString m_azureRegion;
    
    // Threading
    std::unique_ptr<RecognitionThread> m_recognitionThread;
    std::mutex m_mutex;
    std::condition_variable m_condVar;
    
    // Recognition parameters
    int m_sampleRate;
    int m_channels;
}; 