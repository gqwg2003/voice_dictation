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
      m_sampleSize(DEFAULT_SAMPLE_SIZE)
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
        // Reset audio buffer
        m_audioBuffer.clear();
        
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
    if (!m_audioDevice.isNull()) {
        gLogger->info("Using specified input device");
    } else {
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
}

std::vector<float> AudioProcessor::processRawAudioData(const QByteArray& rawData)
{
    // Process 16-bit PCM audio data into normalized float values
    std::vector<float> processedData;
    processedData.reserve(rawData.size() / 2);  // 16-bit = 2 bytes per sample
    
    const int16_t* samples = reinterpret_cast<const int16_t*>(rawData.constData());
    int sampleCount = rawData.size() / 2;
    
    // Convert to float and normalize to [-1, 1]
    for (int i = 0; i < sampleCount; ++i) {
        float normalizedSample = static_cast<float>(samples[i]) / 32768.0f;
        processedData.push_back(normalizedSample);
    }
    
    return processedData;
}

std::vector<float> AudioProcessor::calculateAudioLevels(const std::vector<float>& audioData)
{
    std::vector<float> levels(LEVEL_COUNT, 0.0f);
    
    if (audioData.empty()) {
        return levels;
    }
    
    // Divide the audio data into LEVEL_COUNT segments and calculate RMS for each
    size_t samplesPerLevel = audioData.size() / LEVEL_COUNT;
    
    if (samplesPerLevel == 0) {
        // Not enough samples, use average for all
        float sum = 0.0f;
        for (float sample : audioData) {
            sum += sample * sample;  // Square for RMS
        }
        
        float rms = std::sqrt(sum / audioData.size());
        std::fill(levels.begin(), levels.end(), rms);
    } else {
        // Calculate RMS for each segment
        for (int i = 0; i < LEVEL_COUNT; ++i) {
            size_t startIdx = i * samplesPerLevel;
            size_t endIdx = (i + 1) * samplesPerLevel;
            
            if (i == LEVEL_COUNT - 1) {
                endIdx = audioData.size();  // Last segment takes all remaining samples
            }
            
            float sum = 0.0f;
            for (size_t j = startIdx; j < endIdx; ++j) {
                sum += audioData[j] * audioData[j];  // Square for RMS
            }
            
            levels[i] = std::sqrt(sum / (endIdx - startIdx));
        }
    }
    
    return levels;
} 