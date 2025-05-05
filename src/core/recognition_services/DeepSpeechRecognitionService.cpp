#include "DeepSpeechRecognitionService.h"
#include "../../utils/Logger.h"

#include <QDir>
#include <QStandardPaths>
#include <QCoreApplication>

// Условная компиляция - если определен HAVE_DEEPSPEECH, используем реальный DeepSpeech API
#ifdef HAVE_DEEPSPEECH
#include <deepspeech.h>
#endif

// Внешний глобальный логгер
extern Logger* gLogger;

DeepSpeechRecognitionService::DeepSpeechRecognitionService(QObject* parent)
    : RecognitionService(parent)
    , m_isInitialized(false)
    , m_sampleRate(16000)
    , m_channels(1)
    , m_language("en-US")
{
    gLogger->info("DeepSpeech Recognition Service created");
}

DeepSpeechRecognitionService::~DeepSpeechRecognitionService()
{
    unloadModel();
    gLogger->info("DeepSpeech Recognition Service destroyed");
}

bool DeepSpeechRecognitionService::initialize(const QString& apiKey, bool useSharedApiKey)
{
#ifdef HAVE_DEEPSPEECH
    if (m_isReady) {
        return true;
    }

    // Сохраняем ключи (хотя DeepSpeech их не использует)
    m_apiKey = apiKey;
    m_useSharedApiKey = useSharedApiKey;

    // Загружаем модель
    if (!loadModel()) {
        gLogger->error("Failed to load DeepSpeech model");
        return false;
    }

    m_isReady = true;
    m_isInitialized = true;
    gLogger->info("DeepSpeech Recognition Service initialized");
    return true;
#else
    gLogger->warning("DeepSpeech support is not compiled in");
    return false;
#endif
}

QString DeepSpeechRecognitionService::getLanguage() const
{
    return m_language;
}

bool DeepSpeechRecognitionService::isReady() const
{
    return m_isReady;
}

QString DeepSpeechRecognitionService::transcribe(const std::vector<float>& audioData)
{
#ifdef HAVE_DEEPSPEECH
    if (!isReady() || !m_model) {
        gLogger->error("DeepSpeech model not initialized or not ready");
        return QString();
    }

    try {
        // Создаем потоковое состояние
        m_streamingState.reset(DS_CreateStream(m_model.get()));
        
        // Обрабатываем аудиоданные
        if (!processSpeech(audioData)) {
            gLogger->error("Failed to process speech with DeepSpeech");
            return QString();
        }
        
        // Получаем результат
        std::string text;
        if (finalizeSpeech()) {
            try {
                // Получаем окончательный результат распознавания
                char* result = DS_FinishStream(m_model.get(), m_streamingState.get());
                if (result) {
                    text = std::string(result);
                    DS_FreeString(result);
                }
            }
            catch (const std::exception& e) {
                gLogger->error(std::string("DeepSpeech finalization error: ") + e.what());
                emit recognitionError(QString("DeepSpeech error: %1").arg(e.what()));
                return QString();
            }
        }
        
        // Возвращаем результат
        return QString::fromStdString(text);
    }
    catch (const std::exception& e) {
        gLogger->error(std::string("DeepSpeech transcription error: ") + e.what());
        emit recognitionError(QString("DeepSpeech error: %1").arg(e.what()));
        return QString();
    }
#else
    gLogger->warning("DeepSpeech support is not compiled in");
    emit recognitionError(QString("DeepSpeech support is not compiled in"));
    return QString();
#endif
}

bool DeepSpeechRecognitionService::processSpeech(const std::vector<float>& audioData)
{
#ifdef HAVE_DEEPSPEECH
    if (!m_isInitialized || !m_model || !m_streamingState) {
        gLogger->error("DeepSpeech model not initialized");
        return false;
    }

    if (audioData.empty()) {
        return true;
    }

    try {
        // Преобразуем float в int16 (формат, который ожидает DeepSpeech)
        std::vector<int16_t> buffer(audioData.size());
        for (size_t i = 0; i < audioData.size(); ++i) {
            // Нормализация float [-1.0, 1.0] в int16_t [-32768, 32767]
            buffer[i] = static_cast<int16_t>(audioData[i] * 32767.0f);
        }

        // Обработка аудиоданных
        DS_FeedAudioContent(m_model.get(), m_streamingState.get(), buffer.data(), buffer.size());
        
        // Промежуточные результаты можно получить, но мы их используем только в finalizeSpeech
        return true;
    }
    catch (const std::exception& e) {
        gLogger->error(std::string("DeepSpeech processing error: ") + e.what());
        return false;
    }
#else
    gLogger->warning("DeepSpeech support is not compiled in");
    return false;
#endif
}

bool DeepSpeechRecognitionService::finalizeSpeech()
{
#ifdef HAVE_DEEPSPEECH
    if (!m_isInitialized || !m_model || !m_streamingState) {
        gLogger->error("DeepSpeech model not initialized or no streaming state");
        return false;
    }

    // Мы НЕ финализируем поток здесь, это делается в методе transcribe
    return true;
#else
    gLogger->warning("DeepSpeech support is not compiled in");
    return false;
#endif
}

void DeepSpeechRecognitionService::setLanguage(const QString& languageCode)
{
    if (m_language != languageCode) {
        m_language = languageCode;
        m_languageCode = languageCode;
        
        // Если модель уже загружена, перезагружаем для нового языка
        if (m_isInitialized) {
            unloadModel();
            loadModel();
        }
    }
}

bool DeepSpeechRecognitionService::loadModel()
{
#ifdef HAVE_DEEPSPEECH
    try {
        std::string modelPath = getModelPath(m_language);
        if (modelPath.empty()) {
            gLogger->error("DeepSpeech model file not found");
            return false;
        }

        gLogger->info("Loading DeepSpeech model from: " + modelPath);
        
        // Создаем модель
        ModelState* model;
        int result = DS_CreateModel(modelPath.c_str(), &model);
        if (result != 0) {
            gLogger->error("Failed to create DeepSpeech model: error code " + std::to_string(result));
            return false;
        }
        m_model.reset(model);
        
        // Устанавливаем параметры модели
        std::string scorerPath = modelPath + ".scorer";
        if (QFile::exists(QString::fromStdString(scorerPath))) {
            result = DS_EnableExternalScorer(m_model.get(), scorerPath.c_str());
            if (result != 0) {
                gLogger->warning("Failed to enable external scorer: error code " + std::to_string(result));
            }
        }
        
        gLogger->info("DeepSpeech model loaded successfully");
        return true;
    }
    catch (const std::exception& e) {
        gLogger->error(std::string("DeepSpeech model loading error: ") + e.what());
        m_model.reset();
        return false;
    }
#else
    gLogger->warning("DeepSpeech support is not compiled in");
    return false;
#endif
}

void DeepSpeechRecognitionService::unloadModel()
{
#ifdef HAVE_DEEPSPEECH
    if (m_streamingState) {
        m_streamingState.reset();
    }
    
    if (m_model) {
        m_model.reset();
        gLogger->info("DeepSpeech model unloaded");
    }
    
    m_isInitialized = false;
    m_isReady = false;
#endif
}

std::string DeepSpeechRecognitionService::getModelPath(const QString& language) const
{
    // Получаем язык в формате "en" из "en-US"
    QString shortLang = language.left(2).toLower();
    
    // Список возможных путей для поиска модели
    QStringList searchPaths;
    
    // 1. Папка моделей рядом с приложением
    searchPaths << QCoreApplication::applicationDirPath() + "/models";
    
    // 2. Папка пользователя
    searchPaths << QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/models";
    
    // 3. Системные пути (в зависимости от ОС)
#ifdef Q_OS_WIN
    searchPaths << "C:/Program Files/DeepSpeech/models";
    searchPaths << "C:/DeepSpeech/models";
#else
    searchPaths << "/usr/local/share/deepspeech/models";
    searchPaths << "/usr/share/deepspeech/models";
    searchPaths << QDir::homePath() + "/.local/share/deepspeech/models";
#endif

    // Имена файлов моделей для разных языков
    QString modelFileName;
    if (shortLang == "en") {
        modelFileName = "deepspeech-0.9.3-models.pbmm";
    }
    else if (shortLang == "ru") {
        modelFileName = "deepspeech-0.9.3-models-ru.pbmm";
    }
    else {
        // Для других языков пробуем универсальное имя
        modelFileName = "deepspeech-0.9.3-models-" + shortLang + ".pbmm";
    }
    
    // Ищем модель в каждом из путей
    for (const QString& path : searchPaths) {
        QString fullPath = path + "/" + modelFileName;
        if (QFile::exists(fullPath)) {
            return fullPath.toStdString();
        }
    }
    
    gLogger->warning("DeepSpeech model not found for language: " + shortLang.toStdString());
    return "";
} 