#include "SpeechRecognizer.h"
#include "AudioProcessor.h"
#include "recognition_services/RecognitionService.h"
#include "recognition_services/WhisperRecognitionService.h"
#include "recognition_services/GoogleRecognitionService.h"
#include "recognition_services/AzureRecognitionService.h"
#include "recognition_services/YandexRecognitionService.h"
#include "recognition_services/DeepSpeechRecognitionService.h"
#include "../utils/Logger.h"

#include <QDebug>
#include <QDir>
#include <QFile>
#include <QNetworkRequest>
#include <QUrl>
#include <QEventLoop>
#include <QTimer>
#include <QSettings>
#include <QThread>
#include <thread>
#include <chrono>
#include <mutex>
#include <condition_variable>

// External global logger
extern Logger* gLogger;

// Thread implementation
SpeechRecognizer::RecognitionThread::RecognitionThread(SpeechRecognizer* recognizer)
    : m_recognizer(recognizer)
    , m_stopRequested(false)
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
    
    m_stopRequested = false;
    
    while (!m_stopRequested) {
        try {
            // Проверяем флаг остановки ещё раз перед обработкой
            if (m_stopRequested) break;
            
            m_recognizer->processAudioChunk();
            
            // Небольшая пауза, чтобы избежать интенсивного потребления ресурсов
            // если аудиопроцессор не генерирует данных
            QThread::msleep(10);
            
        } catch (const std::exception& e) {
            gLogger->error("Exception in recognition thread: " + std::string(e.what()));
            emit m_recognizer->recognitionError(QString("Error in recognition thread: %1").arg(e.what()));
            
            // Sleep a bit to avoid spinning too fast in case of repeated errors
            QThread::msleep(500);
        }
    }
    
    gLogger->info("Recognition thread stopped");
}

void SpeechRecognizer::RecognitionThread::stop()
{
    m_stopRequested = true;
    
    // Wake up the thread if it's waiting for audio
    {
        std::lock_guard<std::mutex> lock(m_recognizer->m_mutex);
        m_recognizer->m_condVar.notify_one();
    }
}

// Main SpeechRecognizer class implementation
SpeechRecognizer::SpeechRecognizer(AudioProcessor* audioProcessor, QObject* parent)
    : QObject(parent)
    , m_audioProcessor(audioProcessor)
    , m_recognitionService(nullptr)
    , m_isRunning(false)
    , m_languageCode("en-US")
    , m_recognitionServiceType(RecognitionServiceType::Offline)
    , m_apiKey("")
    , m_useSharedApiKey(false)
    , m_usePublicApi(false)
    , m_azureRegion("westeurope")
    , m_recognitionThread(nullptr)
    , m_sampleRate(16000)
    , m_channels(1)
{
    initialize();
}

SpeechRecognizer::~SpeechRecognizer()
{
    cleanup();
}

void SpeechRecognizer::initialize()
{
    gLogger->info("Initializing speech recognizer");
    
    // Create the appropriate recognition service
    createRecognitionService();
    
    gLogger->info("Speech recognizer initialized");
}

void SpeechRecognizer::cleanup()
{
    gLogger->info("Cleaning up speech recognizer");
    
    stopRecognition();
    
    // Service will be cleaned up by unique_ptr
    m_recognitionService.reset();
}

void SpeechRecognizer::startRecognition()
{
    if (m_isRunning) {
        gLogger->warning("Recognition already running, ignoring start request");
        return;
    }
    
    // Ensure we have a recognition service
    if (!m_recognitionService) {
        createRecognitionService();
    }
    
    if (!m_recognitionService || !m_recognitionService->isReady()) {
        gLogger->error("Recognition service not ready, cannot start recognition");
        emit recognitionError(tr("Speech recognition service is not ready. Check settings."));
        return;
    }
    
    gLogger->info("Starting speech recognition");
    
    // Start the audio processor
    m_audioProcessor->start();
    
    // Create and start the recognition thread
    m_recognitionThread = std::make_unique<RecognitionThread>(this);
    
    // Connect recognition service errors
    connect(m_recognitionService.get(), &RecognitionService::recognitionError, 
            this, &SpeechRecognizer::recognitionError, Qt::DirectConnection);
    
    m_isRunning = true;
    m_recognitionThread->start();
    
    emit recognitionStarted();
}

void SpeechRecognizer::stopRecognition()
{
    if (!m_isRunning) {
        gLogger->warning("Recognition not running, ignoring stop request");
        return;
    }
    
    gLogger->info("Stopping speech recognition");
    
    // Stop the audio processor
    m_audioProcessor->stop();
    
    // Stop and wait for the recognition thread
    if (m_recognitionThread) {
        m_recognitionThread->stop();
        m_recognitionThread->wait();
        m_recognitionThread.reset();
    }
    
    m_isRunning = false;
    
    emit recognitionStopped();
}

void SpeechRecognizer::setLanguage(const QString& languageCode)
{
    m_languageCode = languageCode;
    
    if (m_recognitionService) {
        m_recognitionService->setLanguage(languageCode);
    }
}

QString SpeechRecognizer::getLanguage() const
{
    return m_languageCode;
}

void SpeechRecognizer::setRecognitionService(RecognitionServiceType service)
{
    if (m_recognitionServiceType != service) {
        m_recognitionServiceType = service;
        
        // Create new service
        createRecognitionService();
    }
}

void SpeechRecognizer::setApiKey(const QString& apiKey)
{
    m_apiKey = apiKey;
    
    // Reinitialize with new key
    createRecognitionService();
}

void SpeechRecognizer::setUseSharedApiKey(bool useShared)
{
    m_useSharedApiKey = useShared;
    
    // Reinitialize
    createRecognitionService();
}

void SpeechRecognizer::setUsePublicApi(bool usePublic)
{
    m_usePublicApi = usePublic;
    
    // Update the current service
    if (m_recognitionService) {
        switch (m_recognitionServiceType) {
            case RecognitionServiceType::Google:
                static_cast<GoogleRecognitionService*>(m_recognitionService.get())->setPublicApiEnabled(usePublic);
                break;
                
            case RecognitionServiceType::Yandex:
                static_cast<YandexRecognitionService*>(m_recognitionService.get())->setPublicApiEnabled(usePublic);
                break;
                
            case RecognitionServiceType::Azure:
                static_cast<AzureRecognitionService*>(m_recognitionService.get())->setPublicApiEnabled(usePublic);
                break;
                
            case RecognitionServiceType::Offline:
            case RecognitionServiceType::DeepSpeech:
                // No public API for offline services
                break;
        }
    }
}

void SpeechRecognizer::setAzureRegion(const QString& region)
{
    m_azureRegion = region;
    
    // Update Azure service if active
    if (m_recognitionServiceType == RecognitionServiceType::Azure && m_recognitionService) {
        static_cast<AzureRecognitionService*>(m_recognitionService.get())->setRegion(region);
    }
}

RecognitionServiceType SpeechRecognizer::getRecognitionService() const
{
    return m_recognitionServiceType;
}

bool SpeechRecognizer::isUsingSharedApiKey() const
{
    return m_useSharedApiKey;
}

bool SpeechRecognizer::isUsingPublicApi() const
{
    return m_usePublicApi;
}

QString SpeechRecognizer::getAzureRegion() const
{
    return m_azureRegion;
}

bool SpeechRecognizer::isRunning() const
{
    return m_isRunning;
}

void SpeechRecognizer::processSpeech(const std::vector<float>& audioData)
{
    if (!m_isRunning || audioData.empty()) {
        return;
    }
    
    try {
        QString text = transcribeAudio(audioData);
        
        if (!text.isEmpty()) {
            emit speechRecognized(text);
        }
    } catch (const std::exception& e) {
        gLogger->error("Error processing speech: " + std::string(e.what()));
        emit recognitionError(QString("Error processing speech: %1").arg(e.what()));
    }
}

void SpeechRecognizer::processAudioChunk()
{
    // Wait for the audio processor to signal that audio is ready
    if (!m_audioProcessor->waitForAudioData()) {
        // Если waitForAudioData вернул false, значит запись остановлена
        gLogger->debug("Audio processing stopped or no data available");
        return;
    }
    
    // Get the audio data
    auto audioData = m_audioProcessor->getAudioData();
    
    // Проверяем, что данные не пустые
    if (audioData.empty()) {
        gLogger->debug("Empty audio data received, skipping processing");
        return;
    }
    
    // Process the audio data
    processSpeech(audioData);
}

QString SpeechRecognizer::transcribeAudio(const std::vector<float>& audioData)
{
    // Make sure we have a recognition service
    if (!m_recognitionService) {
        createRecognitionService();
    }
    
    if (!m_recognitionService || !m_recognitionService->isReady()) {
        gLogger->error("Recognition service not ready");
        emit recognitionError(tr("Speech recognition service not ready. Check your settings."));
        return QString();
    }
    
    // Use the recognition service to transcribe
    return m_recognitionService->transcribe(audioData);
}

void SpeechRecognizer::createRecognitionService()
{
    // Clean up existing service
    if (m_recognitionService) {
        if (m_recognitionService->parent() == this) {
            // Disconnect signals
            disconnect(m_recognitionService.get(), &RecognitionService::recognitionError, 
                     this, &SpeechRecognizer::recognitionError);
        }
        
        m_recognitionService.reset();
    }
    
    // Create new service based on service type
    switch (m_recognitionServiceType) {
        case RecognitionServiceType::Offline:
            m_recognitionService = std::make_unique<WhisperRecognitionService>(this);
            break;
            
        case RecognitionServiceType::Google:
            m_recognitionService = std::make_unique<GoogleRecognitionService>(this);
            static_cast<GoogleRecognitionService*>(m_recognitionService.get())->setPublicApiEnabled(m_usePublicApi);
            break;
            
        case RecognitionServiceType::Azure:
            m_recognitionService = std::make_unique<AzureRecognitionService>(this);
            static_cast<AzureRecognitionService*>(m_recognitionService.get())->setPublicApiEnabled(m_usePublicApi);
            static_cast<AzureRecognitionService*>(m_recognitionService.get())->setRegion(m_azureRegion);
            break;
            
        case RecognitionServiceType::Yandex:
            m_recognitionService = std::make_unique<YandexRecognitionService>(this);
            static_cast<YandexRecognitionService*>(m_recognitionService.get())->setPublicApiEnabled(m_usePublicApi);
            break;
            
        case RecognitionServiceType::DeepSpeech:
            m_recognitionService = std::make_unique<DeepSpeechRecognitionService>(this);
            break;
    }
    
    // Initialize the service
    if (m_recognitionService) {
        m_recognitionService->initialize(m_apiKey, m_useSharedApiKey);
        m_recognitionService->setLanguage(m_languageCode);
        
        // Connect signals
        connect(m_recognitionService.get(), &RecognitionService::recognitionError, 
                this, &SpeechRecognizer::recognitionError, Qt::DirectConnection);
        
        gLogger->info("Created recognition service: " + std::to_string(static_cast<int>(m_recognitionServiceType)));
    } else {
        gLogger->error("Failed to create recognition service");
        emit recognitionError(tr("Failed to create speech recognition service"));
    }
} 