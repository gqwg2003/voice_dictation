#pragma once

#include <QObject>
#include <QAudioInput>
#include <QMediaCaptureSession>
#include <QAudioSource>
#include <QBuffer>
#include <QTimer>

class SpeechRecognizer : public QObject
{
    Q_OBJECT

public:
    explicit SpeechRecognizer(QObject *parent = nullptr);
    ~SpeechRecognizer();

    // Методы для управления записью
    bool startRecording(const QString& language);
    void stopRecording();
    bool isRecording() const;

signals:
    // Сигнал, отправляемый когда текст распознан
    void textRecognized(const QString& text);
    
    // Сигнал изменения статуса записи
    void recordingStatusChanged(bool isRecording);
    
    // Сигнал обновления уровня микрофона (0-100)
    void microphoneLevelChanged(int level);

private slots:
    void processRecordedAudio();
    void updateMicrophoneLevel();

private:
    // Преобразует аудио данные в текст
    QString convertSpeechToText(const QByteArray& audioData, const QString& language);
    
    // Инициализация и очистка ресурсов
    void initAudio();
    void cleanupAudio();
    
    // Аудио захват
    QAudioSource* m_audioSource;
    QBuffer* m_audioBuffer;
    
    // Таймеры
    QTimer* m_processingTimer;
    QTimer* m_levelTimer;
    
    // Состояние
    bool m_isRecording;
    QString m_currentLanguage;
}; 