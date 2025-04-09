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
    
    // State
    bool isRunning() const;

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
    void processSpeech(const std::vector<float>& audioData);
    void processAudioChunk();
    QString transcribeAudio(const std::vector<float>& audioData);
    
    // Internal helpers
    void initialize();
    void cleanup();

private:
    // Dependencies
    AudioProcessor* m_audioProcessor;
    
    // State
    std::atomic<bool> m_isRunning;
    QString m_languageCode;
    
    // Threading
    std::unique_ptr<RecognitionThread> m_recognitionThread;
    std::mutex m_mutex;
    std::condition_variable m_condVar;
    
    // Recognition parameters
    int m_sampleRate;
    int m_channels;
}; 