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
class QNetworkAccessManager;

// Forward declaration for Whisper
#ifdef HAVE_WHISPER
struct whisper_context;
#endif

// Speech recognition service types
enum class RecognitionServiceType {
    Offline,
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
    RecognitionServiceType getRecognitionService() const;
    bool isUsingSharedApiKey() const;
    
    // State
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
    QString transcribeOffline(const QByteArray& audioData);
    
    // Cloud API integration
    QString sendAudioToCloudAPI(const QString& audioFilePath);
    QString sendToGoogleSpeechAPI(const QString& audioFilePath);
    QString sendToYandexSpeechKit(const QString& audioFilePath);
    QString sendToAzureSpeechService(const QString& audioFilePath);
    
    // Get API key (personal or shared)
    QString getActiveApiKey(RecognitionServiceType service) const;
    
    // Model management
    void loadLanguageModel();
    
    // Internal helpers
    void initialize();
    void cleanup();

private:
    // Dependencies
    AudioProcessor* m_audioProcessor;
    
    // State
    std::atomic<bool> m_isRunning;
    QString m_languageCode;
    RecognitionServiceType m_recognitionService;
    QString m_apiKey;
    bool m_useSharedApiKey;
    
    // Network for cloud API
    QNetworkAccessManager* m_networkManager;
    
    // Speech recognition model
#ifdef HAVE_WHISPER
    whisper_context* m_whisperContext;
#else
    void* m_whisperContext; // Placeholder when Whisper is not available
#endif
    
    // Threading
    std::unique_ptr<RecognitionThread> m_recognitionThread;
    std::mutex m_mutex;
    std::condition_variable m_condVar;
    
    // Recognition parameters
    int m_sampleRate;
    int m_channels;
}; 