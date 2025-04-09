#pragma once

#include <QObject>
#include <QAudioSource>
#include <QIODevice>
#include <QBuffer>
#include <QAudioFormat>
#include <QAudioDevice>
#include <QMediaDevices>
#include <vector>
#include <mutex>
#include <atomic>
#include <memory>

class AudioProcessor : public QObject {
    Q_OBJECT

public:
    explicit AudioProcessor(QObject* parent = nullptr);
    ~AudioProcessor();

    // Control methods
    void startRecording();
    void stopRecording();
    
    // Data access
    std::vector<float> getAudioData();
    std::vector<float> getAudioLevels();
    
    // State
    bool isRecording() const;
    
    // Configuration
    void setInputDevice(const QAudioDevice& device);
    QAudioDevice getInputDevice() const;
    
    void setSampleRate(int sampleRate);
    int getSampleRate() const;
    
    void setChannelCount(int channels);
    int getChannelCount() const;

signals:
    void recordingStarted();
    void recordingStopped();
    void audioLevelsChanged(const std::vector<float>& levels);
    void audioDataReady(const std::vector<float>& audioData);
    void errorOccurred(const QString& error);

private slots:
    void processAudioInput();

private:
    // Initialize audio system
    void initialize();
    
    // Process raw audio data
    std::vector<float> processRawAudioData(const QByteArray& rawData);
    
    // Calculate audio levels from raw data
    std::vector<float> calculateAudioLevels(const std::vector<float>& audioData);

private:
    // Audio input components
    QAudioFormat m_audioFormat;
    QAudioDevice m_audioDevice;
    std::unique_ptr<QAudioSource> m_audioSource;
    QIODevice* m_audioInputDevice;
    QByteArray m_audioBuffer;
    
    // Processing state
    std::atomic<bool> m_isRecording;
    std::vector<float> m_currentLevels;
    std::vector<float> m_processedAudioData;
    std::mutex m_audioMutex;
    
    // Configuration
    int m_sampleRate;
    int m_channelCount;
    int m_sampleSize;
    
    // Constants
    static const int DEFAULT_SAMPLE_RATE = 16000;
    static const int DEFAULT_CHANNEL_COUNT = 1;
    static const int DEFAULT_SAMPLE_SIZE = 16;
    static const int LEVEL_COUNT = 32;
    static const int BUFFER_SIZE = 8192;
}; 