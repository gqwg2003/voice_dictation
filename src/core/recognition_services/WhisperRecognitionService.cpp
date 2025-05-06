#include "WhisperRecognitionService.h"
#include "../../utils/Logger.h"

#include <QDir>
#include <QFile>
#include <QSettings>

// External global logger
extern Logger* gLogger;

#ifdef HAVE_WHISPER
#include <whisper.h>
#endif

WhisperRecognitionService::WhisperRecognitionService(QObject* parent)
    : RecognitionService(parent)
    , m_whisperContext(nullptr)
    , m_modelPath("")
    , m_modelSize("base")
    , m_noSpeechThreshold(0.6f)
{
    // Model will be loaded during initialization
}

WhisperRecognitionService::~WhisperRecognitionService()
{
#ifdef HAVE_WHISPER
    // Free whisper context
    if (m_whisperContext) {
        whisper_free(m_whisperContext);
        m_whisperContext = nullptr;
    }
#endif
}

bool WhisperRecognitionService::initialize(const QString& apiKey, bool useSharedApiKey)
{
    // API key is not used for Whisper offline recognition
    Q_UNUSED(apiKey);
    Q_UNUSED(useSharedApiKey);
    
    QSettings settings;
    m_modelSize = settings.value("recognition/model_size", "base").toString();
    m_noSpeechThreshold = settings.value("recognition/no_speech_threshold", 0.6f).toFloat();
    
    // Load the language model
    bool result = loadLanguageModel();
    m_isReady = result;
    return result;
}

void WhisperRecognitionService::setLanguage(const QString& languageCode)
{
    m_languageCode = languageCode;
    
    // Reload model if needed (in case different language models are used)
    if (m_isReady && m_languageCode != m_languageCode) {
        loadLanguageModel();
    }
}

QString WhisperRecognitionService::getLanguage() const
{
    return m_languageCode;
}

QString WhisperRecognitionService::transcribe(const std::vector<float>& audioData)
{
    gLogger->info("Using offline speech recognition with Whisper");
    
    if (!m_isReady) {
        if (!loadLanguageModel()) {
            emit recognitionError(tr("Failed to load Whisper model for speech recognition"));
            return QString();
        }
    }
    
    // Check if audio data is empty
    if (audioData.empty()) {
        gLogger->warning("Empty audio data provided for transcription");
        return QString();
    }
    
#ifdef HAVE_WHISPER
    try {
        // Get audio data pointer and sample count
        const float* samples = audioData.data();
        int sampleCount = static_cast<int>(audioData.size());
        
        // Create parameters for Whisper
        whisper_full_params wparams = whisper_full_default_params(WHISPER_SAMPLING_GREEDY);
        
        // Configure language
        if (m_languageCode.startsWith("ru")) {
            wparams.language = "ru";
        } else if (m_languageCode.startsWith("en")) {
            wparams.language = "en";
        } else {
            // Auto-detect language
            wparams.language = nullptr;
        }
        
        // Additional settings
        wparams.print_realtime   = false;
        wparams.print_progress   = false;
        wparams.print_timestamps = false;
        wparams.translate        = false;
        wparams.single_segment   = false;
        wparams.max_tokens       = 0;
        wparams.speed_up         = true;
        wparams.no_speech_thold  = m_noSpeechThreshold;
        
        // Perform recognition
        if (whisper_full(m_whisperContext, wparams, samples, sampleCount) != 0) {
            gLogger->error("Failed to process audio with Whisper");
            emit recognitionError(tr("Error processing audio with Whisper"));
            return QString();
        }
        
        // Get number of segments
        int numSegments = whisper_full_n_segments(m_whisperContext);
        
        // Collect result
        QString result;
        for (int i = 0; i < numSegments; ++i) {
            const char* text = whisper_full_get_segment_text(m_whisperContext, i);
            
            if (whisper_full_get_segment_no_speech_prob(m_whisperContext, i) < m_noSpeechThreshold) {
                result += QString::fromUtf8(text);
            }
        }
        
        return result;
    } catch (const std::exception& e) {
        gLogger->error("Exception in Whisper transcription: " + std::string(e.what()));
        emit recognitionError(tr("Error during speech recognition: %1").arg(e.what()));
        return QString();
    }
#else
    // Whisper not installed
    emit recognitionError(tr("Whisper library is not installed. Offline speech recognition is not available."));
    return QString();
#endif
}

bool WhisperRecognitionService::isReady() const
{
    return m_isReady;
}

bool WhisperRecognitionService::loadLanguageModel()
{
#ifdef HAVE_WHISPER
    gLogger->info("Loading language model for offline recognition");
    
    // Clear previous context if it exists
    if (m_whisperContext) {
        whisper_free(m_whisperContext);
        m_whisperContext = nullptr;
    }
    
    // Get model path
    QString modelPath = getModelPath();
    
    // Choose model based on language
    QString modelFile;
    
    if (m_languageCode.startsWith("ru")) {
        modelFile = modelPath + "whisper-" + m_modelSize + ".bin";
    } else {
        modelFile = modelPath + "whisper-" + m_modelSize + ".bin";
    }
    
    // Check if model file exists
    if (!QFile::exists(modelFile)) {
        gLogger->error("Model file not found: " + modelFile.toStdString());
        emit recognitionError(tr("Speech recognition model not found: %1").arg(modelFile));
        return false;
    }
    
    // Initialize Whisper
    m_whisperContext = whisper_init_from_file(modelFile.toUtf8().constData());
    
    if (m_whisperContext == nullptr) {
        gLogger->error("Failed to load whisper model");
        emit recognitionError(tr("Failed to load speech recognition model"));
        return false;
    } else {
        gLogger->info("Language model loaded successfully");
        return true;
    }
#else
    gLogger->warning("Whisper not available, skipping model loading");
    emit recognitionError(tr("Whisper library is not installed. For offline recognition, please install the Whisper library."));
    return false;
#endif
}

QString WhisperRecognitionService::getModelPath() const
{
    QSettings settings;
    QString modelPath = settings.value("recognition/model_path", "").toString();
    
    if (modelPath.isEmpty()) {
        // Use default path
        modelPath = QDir::currentPath() + "/models/";
    }
    
    // Ensure path ends with slash
    if (!modelPath.endsWith('/') && !modelPath.endsWith('\\')) {
        modelPath += "/";
    }
    
    return modelPath;
}

void WhisperRecognitionService::setNoSpeechThreshold(float threshold)
{
    if (threshold >= 0.0f && threshold <= 1.0f) {
        m_noSpeechThreshold = threshold;
        
        QSettings settings;
        settings.setValue("recognition/no_speech_threshold", m_noSpeechThreshold);
    }
}

float WhisperRecognitionService::getNoSpeechThreshold() const
{
    return m_noSpeechThreshold;
} 