#pragma once

#include <QMainWindow>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QAction>
#include <QLabel>
#include <QTextEdit>
#include <QPushButton>
#include <QComboBox>
#include <QTimer>
#include <QTranslator>
#include <memory>

// Forward declarations
class AudioVisualizer;
class SpeechRecognizer;
class AudioProcessor;
class TextProcessor;
class ClipboardManager;
class HotkeyManager;
class SettingsDialog;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void closeEvent(QCloseEvent *event) override;
    void changeEvent(QEvent *event) override;

private slots:
    void onStartRecording();
    void onStopRecording();
    void onClearText();
    void onCopyText();
    void onLanguageChanged(int index);
    void onSettingsAction();
    void onTrayActivated(QSystemTrayIcon::ActivationReason reason);
    void onAboutAction();
    void onExitAction();
    void updateAudioVisualization();
    void onSpeechRecognized(const QString &text);
    void onHotkeyPressed(const QString &action);
    void applySettings();

private:
    void setupUi();
    void createActions();
    void createTrayIcon();
    void createConnections();
    void loadSettings();
    void saveSettings();
    void switchLanguage(const QString &language);
    void retranslateUi();

private:
    // UI elements
    QTextEdit *m_textEdit;
    QPushButton *m_recordButton;
    QPushButton *m_stopButton;
    QPushButton *m_clearButton;
    QPushButton *m_copyButton;
    QComboBox *m_languageComboBox;
    QLabel *m_statusLabel;
    AudioVisualizer *m_audioVisualizer;
    
    // Tray icon
    QSystemTrayIcon *m_trayIcon;
    QMenu *m_trayMenu;
    QAction *m_showAction;
    QAction *m_settingsAction;
    QAction *m_aboutAction;
    QAction *m_exitAction;
    
    // Core components
    std::unique_ptr<SpeechRecognizer> m_speechRecognizer;
    std::unique_ptr<AudioProcessor> m_audioProcessor;
    std::unique_ptr<TextProcessor> m_textProcessor;
    std::unique_ptr<ClipboardManager> m_clipboardManager;
    std::unique_ptr<HotkeyManager> m_hotkeyManager;
    std::unique_ptr<SettingsDialog> m_settingsDialog;
    
    // State
    bool m_isRecording;
    QTimer m_visualizationTimer;
    QTranslator m_translator;
    QString m_currentLanguage;
    
    // Constants
    const int VISUALIZATION_UPDATE_INTERVAL = 50; // ms
}; 