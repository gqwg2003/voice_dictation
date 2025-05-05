#include "speechrecognizer.h"
#include <QAudioFormat>
#include <QAudioDevice>
#include <QMediaDevices>
#include <QDebug>
#include <QRandomGenerator>

SpeechRecognizer::SpeechRecognizer(QObject *parent)
    : QObject(parent)
    , m_audioSource(nullptr)
    , m_audioBuffer(nullptr)
    , m_processingTimer(nullptr)
    , m_levelTimer(nullptr)
    , m_isRecording(false)
{
    // Инициализируем таймеры
    m_processingTimer = new QTimer(this);
    connect(m_processingTimer, &QTimer::timeout, this, &SpeechRecognizer::processRecordedAudio);
    
    m_levelTimer = new QTimer(this);
    connect(m_levelTimer, &QTimer::timeout, this, &SpeechRecognizer::updateMicrophoneLevel);
    
    m_audioBuffer = new QBuffer(this);
}

SpeechRecognizer::~SpeechRecognizer()
{
    if (m_isRecording) {
        stopRecording();
    }
    
    cleanupAudio();
}

bool SpeechRecognizer::startRecording(const QString& language)
{
    if (m_isRecording) {
        return true; // Уже записываем
    }
    
    m_currentLanguage = language;
    
    // Инициализируем аудио захват
    initAudio();
    
    if (!m_audioSource) {
        qWarning() << "Не удалось создать источник аудио";
        return false;
    }
    
    // Подготавливаем буфер к записи
    m_audioBuffer->open(QIODevice::WriteOnly);
    m_audioBuffer->buffer().clear();
    
    // Начинаем запись
    m_audioSource->start(m_audioBuffer);
    
    // Запускаем таймеры
    m_processingTimer->start(2000); // Каждые 2 секунды обрабатываем аудио
    m_levelTimer->start(200); // 5 раз в секунду обновляем уровень звука
    
    m_isRecording = true;
    emit recordingStatusChanged(true);
    
    return true;
}

void SpeechRecognizer::stopRecording()
{
    if (!m_isRecording) {
        return;
    }
    
    // Останавливаем таймеры
    m_processingTimer->stop();
    m_levelTimer->stop();
    
    // Останавливаем запись
    if (m_audioSource) {
        m_audioSource->stop();
    }
    
    // Закрываем буфер
    if (m_audioBuffer && m_audioBuffer->isOpen()) {
        m_audioBuffer->close();
        
        // Обрабатываем последний фрагмент
        processRecordedAudio();
    }
    
    m_isRecording = false;
    emit recordingStatusChanged(false);
    
    cleanupAudio();
}

bool SpeechRecognizer::isRecording() const
{
    return m_isRecording;
}

void SpeechRecognizer::processRecordedAudio()
{
    if (!m_isRecording || !m_audioBuffer) {
        return;
    }
    
    // Получаем данные буфера
    const QByteArray &audioData = m_audioBuffer->buffer();
    
    if (audioData.size() > 0) {
        // Преобразуем речь в текст
        QString recognizedText = convertSpeechToText(audioData, m_currentLanguage);
        
        if (!recognizedText.isEmpty()) {
            emit textRecognized(recognizedText);
        }
        
        // Очищаем буфер для следующей части
        m_audioBuffer->buffer().clear();
    }
}

void SpeechRecognizer::updateMicrophoneLevel()
{
    if (!m_isRecording || !m_audioSource) {
        return;
    }
    
    // В реальном приложении здесь будет логика определения уровня громкости
    // На данный момент просто эмулируем случайный уровень для демонстрации
    const int level = QRandomGenerator::global()->bounded(40, 80);
    emit microphoneLevelChanged(level);
}

QString SpeechRecognizer::convertSpeechToText(const QByteArray& audioData, const QString& language)
{
    // В настоящем приложении здесь будет использована реальная API для распознавания речи
    // Например, Google Speech API, Yandex SpeechKit, Whisper, CMU Sphinx и т.д.
    
    // Пока что эмулируем ответ для демонстрации
    Q_UNUSED(audioData);
    
    static const QStringList russianWords = {
        "Привет", "мир", "это", "тест", "распознавания", "речи", 
        "на", "русском", "языке", "в", "приложении"
    };
    
    static const QStringList englishWords = {
        "Hello", "world", "this", "is", "speech", "recognition", 
        "test", "in", "English", "for", "demo", "application"
    };
    
    const QStringList &words = (language == "ru-RU") ? russianWords : englishWords;
    
    int wordCount = QRandomGenerator::global()->bounded(3, 7);
    QStringList resultWords;
    
    for(int i = 0; i < wordCount; ++i) {
        int wordIndex = QRandomGenerator::global()->bounded(words.size());
        resultWords.append(words[wordIndex]);
    }
    
    return resultWords.join(' ') + ".";
}

void SpeechRecognizer::initAudio()
{
    // Очищаем существующие ресурсы, если они есть
    cleanupAudio();
    
    // Выбираем входное аудио устройство (микрофон)
    const QAudioDevice inputDevice = QMediaDevices::defaultAudioInput();
    
    if (!inputDevice.isNull()) {
        // Настраиваем формат аудио
        QAudioFormat format;
        format.setSampleRate(16000); // 16kHz
        format.setChannelCount(1);   // Моно
        format.setSampleFormat(QAudioFormat::Int16); // 16-bit PCM
        
        if (inputDevice.isFormatSupported(format)) {
            m_audioSource = new QAudioSource(inputDevice, format, this);
            m_audioSource->setBufferSize(8192);
        } else {
            qWarning() << "Аудио формат не поддерживается устройством";
        }
    } else {
        qWarning() << "Не найдено аудио устройство ввода";
    }
}

void SpeechRecognizer::cleanupAudio()
{
    if (m_audioSource) {
        delete m_audioSource;
        m_audioSource = nullptr;
    }
} 