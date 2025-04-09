#include "AudioProcessor.h"
#include "../utils/Logger.h"

#include <QDebug>
#include <QDateTime>
#include <cmath>
#include <algorithm>

// External global logger
extern Logger* gLogger;

AudioProcessor::AudioProcessor(QObject* parent)
    : QObject(parent),
      m_audioInputDevice(nullptr),
      m_isRecording(false),
      m_sampleRate(DEFAULT_SAMPLE_RATE),
      m_channelCount(DEFAULT_CHANNEL_COUNT),
      m_sampleSize(DEFAULT_SAMPLE_SIZE),
      m_audioDataReady(false)
{
    initialize();
}

AudioProcessor::~AudioProcessor()
{
    if (m_isRecording) {
        stopRecording();
    }
}

void AudioProcessor::startRecording()
{
    if (m_isRecording) {
        gLogger->warning("Recording already in progress");
        return;
    }
    
    gLogger->info("Starting audio recording");
    
    try {
        // Reset audio buffer and audio data ready flag
        m_audioBuffer.clear();
        m_audioDataReady = false;
        
        // Create and start audio source
        m_audioSource = std::make_unique<QAudioSource>(m_audioDevice, m_audioFormat);
        
        // Start capturing audio
        m_audioInputDevice = m_audioSource->start();
        
        if (m_audioInputDevice) {
            // Connect readyRead signal to process incoming audio
            connect(m_audioInputDevice, &QIODevice::readyRead,
                    this, &AudioProcessor::processAudioInput);
            
            m_isRecording = true;
            emit recordingStarted();
        } else {
            QString error = "Failed to start audio input device";
            gLogger->error(error.toStdString());
            emit errorOccurred(error);
        }
    } catch (const std::exception& e) {
        QString error = QString("Exception in startRecording: %1").arg(e.what());
        gLogger->error(error.toStdString());
        emit errorOccurred(error);
    }
}

void AudioProcessor::stopRecording()
{
    if (!m_isRecording) {
        return;
    }
    
    gLogger->info("Stopping audio recording");
    
    try {
        // Disconnect signals
        if (m_audioInputDevice) {
            disconnect(m_audioInputDevice, &QIODevice::readyRead,
                      this, &AudioProcessor::processAudioInput);
        }
        
        // Stop audio source
        if (m_audioSource) {
            m_audioSource->stop();
            m_audioSource.reset();
        }
        
        m_audioInputDevice = nullptr;
        m_isRecording = false;
        
        // Clear levels
        m_currentLevels.clear();
        m_currentLevels.resize(LEVEL_COUNT, 0.0f);
        
        emit recordingStopped();
        emit audioLevelsChanged(m_currentLevels);
        
    } catch (const std::exception& e) {
        QString error = QString("Exception in stopRecording: %1").arg(e.what());
        gLogger->error(error.toStdString());
        emit errorOccurred(error);
    }
}

std::vector<float> AudioProcessor::getAudioData()
{
    std::lock_guard<std::mutex> lock(m_audioMutex);
    return m_processedAudioData;
}

std::vector<float> AudioProcessor::getAudioLevels()
{
    std::lock_guard<std::mutex> lock(m_audioMutex);
    return m_currentLevels;
}

bool AudioProcessor::isRecording() const
{
    return m_isRecording;
}

void AudioProcessor::setInputDevice(const QAudioDevice& device)
{
    if (m_isRecording) {
        stopRecording();
    }
    
    m_audioDevice = device;
    initialize();
}

QAudioDevice AudioProcessor::getInputDevice() const
{
    return m_audioDevice;
}

void AudioProcessor::setSampleRate(int sampleRate)
{
    if (m_isRecording) {
        stopRecording();
    }
    
    m_sampleRate = sampleRate;
    initialize();
}

int AudioProcessor::getSampleRate() const
{
    return m_sampleRate;
}

void AudioProcessor::setChannelCount(int channels)
{
    if (m_isRecording) {
        stopRecording();
    }
    
    m_channelCount = channels;
    initialize();
}

int AudioProcessor::getChannelCount() const
{
    return m_channelCount;
}

void AudioProcessor::processAudioInput()
{
    if (!m_isRecording || !m_audioInputDevice) {
        return;
    }
    
    try {
        // Read available audio data
        QByteArray data = m_audioInputDevice->readAll();
        
        if (data.isEmpty()) {
            return;
        }
        
        // Process the raw audio data
        std::vector<float> processedData;
        {
            std::lock_guard<std::mutex> lock(m_audioMutex);
            
            // Append new data to buffer
            m_audioBuffer.append(data);
            
            // Process buffer if enough data is available
            if (m_audioBuffer.size() >= BUFFER_SIZE) {
                processedData = processRawAudioData(m_audioBuffer);
                m_processedAudioData = processedData;
                
                // Calculate audio levels
                m_currentLevels = calculateAudioLevels(processedData);
                
                // Clear buffer after processing
                m_audioBuffer.clear();
                
                // Mark that audio data is ready and notify waiters
                m_audioDataReady = true;
                m_audioDataCondition.notify_all();
            }
        }
        
        // Emit signals outside of lock
        if (!processedData.empty()) {
            emit audioDataReady(processedData);
            emit audioLevelsChanged(m_currentLevels);
        }
        
    } catch (const std::exception& e) {
        QString error = QString("Error processing audio input: %1").arg(e.what());
        gLogger->error(error.toStdString());
        emit errorOccurred(error);
    }
}

void AudioProcessor::initialize()
{
    gLogger->info("Initializing audio processor");
    
    // Setup audio format
    m_audioFormat.setSampleRate(m_sampleRate);
    m_audioFormat.setChannelCount(m_channelCount);
    m_audioFormat.setSampleFormat(QAudioFormat::Int16);
    
    // Find default input device if not specified
    if (m_audioDevice.isNull()) {
        m_audioDevice = QMediaDevices::defaultAudioInput();
        if (m_audioDevice.isNull()) {
            gLogger->error("No audio input device found");
            emit errorOccurred("No audio input device found");
        } else {
            gLogger->info("Using default input device: " + m_audioDevice.description().toStdString());
        }
    }
    
    // Initialize empty level data
    m_currentLevels.resize(LEVEL_COUNT, 0.0f);
    
    // Убедимся, что буфер чист
    m_processedAudioData.clear();
    m_audioBuffer.clear();
    
    gLogger->info("Audio processor initialized");
}

std::vector<float> AudioProcessor::processRawAudioData(const QByteArray& rawData)
{
    // Process 16-bit PCM audio data into normalized float values
    std::vector<float> processedData;
    processedData.reserve(rawData.size() / 2);  // 16-bit = 2 bytes per sample
    
    const int16_t* samples = reinterpret_cast<const int16_t*>(rawData.constData());
    int sampleCount = rawData.size() / 2;
    
    // Convert to normalized float values [-1.0, 1.0]
    float normalizationFactor = 1.0f / 32768.0f;  // For 16-bit audio
    
    for (int i = 0; i < sampleCount; ++i) {
        float normalizedSample = static_cast<float>(samples[i]) * normalizationFactor;
        processedData.push_back(normalizedSample);
    }
    
    gLogger->debug("Processed " + std::to_string(sampleCount) + " audio samples");
    
    return processedData;
}

std::vector<float> AudioProcessor::calculateAudioLevels(const std::vector<float>& audioData)
{
    std::vector<float> levels(LEVEL_COUNT, 0.0f);
    
    if (audioData.empty()) {
        return levels;
    }
    
    // Calculate RMS levels for different frequency bands
    // For simplicity, we'll just calculate the overall RMS level and create a visualizer
    
    float sumOfSquares = 0.0f;
    for (const auto& sample : audioData) {
        sumOfSquares += sample * sample;
    }
    
    float rms = std::sqrt(sumOfSquares / audioData.size());
    
    // Create a simple visual "spectrum" effect
    // Центральные полосы выше, края ниже (для визуального эффекта)
    for (int i = 0; i < LEVEL_COUNT; ++i) {
        float position = static_cast<float>(i) / LEVEL_COUNT;
        float amplitude = std::sin(position * 3.14159f);  // 0 to 1 to 0
        levels[i] = rms * amplitude * 5.0f; // Усиливаем для лучшего отображения
        
        // Ограничиваем значения
        levels[i] = std::max(0.0f, std::min(1.0f, levels[i]));
    }
    
    return levels;
}

void AudioProcessor::start()
{
    startRecording();
}

void AudioProcessor::stop()
{
    stopRecording();
}

bool AudioProcessor::waitForAudioData()
{
    std::unique_lock<std::mutex> lock(m_audioMutex);
    
    // If we already have audio data ready, return immediately
    if (m_audioDataReady) {
        return true;
    }
    
    // Wait indefinitely for condition variable to be notified
    m_audioDataCondition.wait(lock, [this] { return m_audioDataReady.load(); });
    return true;
} 