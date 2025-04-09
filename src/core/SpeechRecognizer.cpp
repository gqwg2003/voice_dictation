#include "SpeechRecognizer.h"
#include "AudioProcessor.h"
#include "../utils/Logger.h"

#include <QDebug>
#include <thread>
#include <chrono>

// External global logger
extern Logger* gLogger;

// RecognitionThread implementation
SpeechRecognizer::RecognitionThread::RecognitionThread(SpeechRecognizer* recognizer)
    : m_recognizer(recognizer),
      m_stopRequested(false)
{
}

SpeechRecognizer::RecognitionThread::~RecognitionThread()
{
    stop();
    wait();
}

void SpeechRecognizer::RecognitionThread::run()
{
    gLogger->info("Recognition thread started");
    
    while (!m_stopRequested) {
        // Process audio chunks as they become available
        m_recognizer->processAudioChunk();
        
        // Small sleep to prevent tight loop
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    gLogger->info("Recognition thread stopped");
}

void SpeechRecognizer::RecognitionThread::stop()
{
    m_stopRequested = true;
}

// SpeechRecognizer implementation
SpeechRecognizer::SpeechRecognizer(AudioProcessor* audioProcessor, QObject* parent)
    : QObject(parent),
      m_audioProcessor(audioProcessor),
      m_isRunning(false),
      m_languageCode("en-US"),
      m_sampleRate(16000),
      m_channels(1)
{
    initialize();
}

SpeechRecognizer::~SpeechRecognizer()
{
    stopRecognition();
    cleanup();
}

void SpeechRecognizer::startRecognition()
{
    if (m_isRunning) {
        gLogger->warning("Recognition already running");
        return;
    }
    
    gLogger->info("Starting speech recognition");
    
    m_isRunning = true;
    
    // Start the recognition thread
    m_recognitionThread = std::make_unique<RecognitionThread>(this);
    m_recognitionThread->start();
    
    emit recognitionStarted();
}

void SpeechRecognizer::stopRecognition()
{
    if (!m_isRunning) {
        return;
    }
    
    gLogger->info("Stopping speech recognition");
    
    m_isRunning = false;
    
    // Stop the recognition thread
    if (m_recognitionThread) {
        m_recognitionThread->stop();
        m_recognitionThread->wait();
        m_recognitionThread.reset();
    }
    
    emit recognitionStopped();
}

void SpeechRecognizer::setLanguage(const QString& languageCode)
{
    gLogger->info("Setting recognition language to: " + languageCode.toStdString());
    
    // Lock to prevent changing language during recognition
    std::lock_guard<std::mutex> lock(m_mutex);
    m_languageCode = languageCode;
}

QString SpeechRecognizer::getLanguage() const
{
    return m_languageCode;
}

bool SpeechRecognizer::isRunning() const
{
    return m_isRunning;
}

void SpeechRecognizer::processSpeech(const std::vector<float>& audioData)
{
    if (audioData.empty()) {
        return;
    }
    
    try {
        // Transcribe the audio
        QString text = transcribeAudio(audioData);
        
        if (!text.isEmpty()) {
            // Emit the recognized text
            emit speechRecognized(text);
        }
        
    } catch (const std::exception& e) {
        gLogger->error("Error processing speech: " + std::string(e.what()));
        emit recognitionError(QString("Processing error: %1").arg(e.what()));
    }
}

void SpeechRecognizer::processAudioChunk()
{
    if (!m_isRunning || !m_audioProcessor) {
        return;
    }
    
    try {
        // Get audio data from the audio processor
        auto audioData = m_audioProcessor->getAudioData();
        
        if (!audioData.empty()) {
            // Process the audio data
            processSpeech(audioData);
        }
        
    } catch (const std::exception& e) {
        gLogger->error("Error getting audio data: " + std::string(e.what()));
        emit recognitionError(QString("Audio error: %1").arg(e.what()));
    }
}

QString SpeechRecognizer::transcribeAudio(const std::vector<float>& audioData)
{
    // TODO: Implement actual transcription using a speech recognition library
    // For this skeleton implementation, just return a dummy text
    
    // In a real implementation, this would use a library like:
    // - Mozilla DeepSpeech
    // - Whisper
    // - Google Cloud Speech API
    // - Microsoft Azure Cognitive Services
    
    // Simulate processing delay
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // Return sample text based on language
    if (m_languageCode == "ru-RU") {
        return "Это тестовый текст распознавания речи";
    } else {
        return "This is a speech recognition test text";
    }
}

void SpeechRecognizer::initialize()
{
    gLogger->info("Initializing speech recognizer");
    
    // TODO: Initialize speech recognition engine/model
    
    // In a real implementation, this would:
    // 1. Load speech recognition models
    // 2. Initialize any libraries
    // 3. Set up audio processing parameters
}

void SpeechRecognizer::cleanup()
{
    gLogger->info("Cleaning up speech recognizer");
    
    // TODO: Clean up speech recognition resources
} 