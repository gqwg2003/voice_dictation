#include "MainWindow.h"
#include "AudioVisualizer.h"
#include "SettingsDialog.h"
#include "../core/SpeechRecognizer.h"
#include "../core/AudioProcessor.h"
#include "../core/TextProcessor.h"
#include "../core/ClipboardManager.h"
#include "../core/HotkeyManager.h"
#include "../utils/Logger.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QCloseEvent>
#include <QMessageBox>
#include <QSettings>
#include <QDir>
#include <QStandardPaths>
#include <QApplication>
#include <QScreen>
#include <QStatusBar>

// External global logger
extern Logger* gLogger;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
      m_isRecording(false)
{
    gLogger->info("Initializing main window");
    
    // Initialize core components
    m_audioProcessor = std::make_unique<AudioProcessor>();
    m_speechRecognizer = std::make_unique<SpeechRecognizer>(m_audioProcessor.get());
    m_textProcessor = std::make_unique<TextProcessor>();
    m_clipboardManager = std::make_unique<ClipboardManager>();
    m_hotkeyManager = std::make_unique<HotkeyManager>();
    
    // Setup UI
    setupUi();
    createActions();
    createTrayIcon();
    createConnections();
    
    // Load settings
    loadSettings();
    
    // Set window properties
    setMinimumSize(400, 500);
    setWindowTitle(tr("Voice Dictation"));
    setWindowIcon(QIcon(":/icons/app.png"));
    
    // Center window on screen
    QRect screenGeometry = QApplication::primaryScreen()->geometry();
    int x = (screenGeometry.width() - width()) / 2;
    int y = (screenGeometry.height() - height()) / 2;
    move(x, y);
    
    // Setup visualization timer
    m_visualizationTimer.setInterval(VISUALIZATION_UPDATE_INTERVAL);
    connect(&m_visualizationTimer, &QTimer::timeout, this, &MainWindow::updateAudioVisualization);
    
    gLogger->info("Main window initialized");
}

MainWindow::~MainWindow()
{
    gLogger->info("Cleaning up main window");
    
    // Save settings before closing
    saveSettings();
    
    // Stop recording if active
    if (m_isRecording) {
        onStopRecording();
    }
    
    // Stop timer
    m_visualizationTimer.stop();
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (m_trayIcon->isVisible()) {
        gLogger->info("Minimizing to tray instead of closing");
        hide();
        event->ignore();
    } else {
        event->accept();
    }
}

void MainWindow::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::LanguageChange) {
        retranslateUi();
    }
    QMainWindow::changeEvent(event);
}

void MainWindow::setupUi()
{
    QWidget *centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);
    
    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);
    
    // Text area
    m_textEdit = new QTextEdit(this);
    m_textEdit->setReadOnly(false);
    m_textEdit->setPlaceholderText(tr("Recognized text will appear here..."));
    mainLayout->addWidget(m_textEdit, 1);
    
    // Audio visualizer
    m_audioVisualizer = new AudioVisualizer(this);
    m_audioVisualizer->setMinimumHeight(80);
    mainLayout->addWidget(m_audioVisualizer);
    
    // Controls
    QHBoxLayout *controlsLayout = new QHBoxLayout();
    
    m_recordButton = new QPushButton(tr("Record"), this);
    m_recordButton->setIcon(QIcon(":/icons/record.png"));
    m_stopButton = new QPushButton(tr("Stop"), this);
    m_stopButton->setIcon(QIcon(":/icons/stop.png"));
    m_stopButton->setEnabled(false);
    m_clearButton = new QPushButton(tr("Clear"), this);
    m_clearButton->setIcon(QIcon(":/icons/clear.png"));
    m_copyButton = new QPushButton(tr("Copy"), this);
    m_copyButton->setIcon(QIcon(":/icons/copy.png"));
    m_settingsButton = new QPushButton(tr("Settings"), this);
    m_settingsButton->setIcon(QIcon(":/icons/settings.png"));
    
    controlsLayout->addWidget(m_recordButton);
    controlsLayout->addWidget(m_stopButton);
    controlsLayout->addWidget(m_clearButton);
    controlsLayout->addWidget(m_copyButton);
    controlsLayout->addWidget(m_settingsButton);
    
    mainLayout->addLayout(controlsLayout);
    
    // Language selection
    QHBoxLayout *languageLayout = new QHBoxLayout();
    QLabel *languageLabel = new QLabel(tr("Language:"), this);
    m_languageComboBox = new QComboBox(this);
    m_languageComboBox->addItem(tr("Russian"), "ru-RU");
    m_languageComboBox->addItem(tr("English"), "en-US");
    
    languageLayout->addWidget(languageLabel);
    languageLayout->addWidget(m_languageComboBox, 1);
    
    mainLayout->addLayout(languageLayout);
    
    // Status bar
    m_statusLabel = new QLabel(tr("Ready"), this);
    statusBar()->addWidget(m_statusLabel, 1);
}

void MainWindow::createActions()
{
    m_showAction = new QAction(tr("Show"), this);
    connect(m_showAction, &QAction::triggered, this, &MainWindow::show);
    
    m_settingsAction = new QAction(tr("Settings"), this);
    connect(m_settingsAction, &QAction::triggered, this, &MainWindow::onSettingsAction);
    
    m_aboutAction = new QAction(tr("About"), this);
    connect(m_aboutAction, &QAction::triggered, this, &MainWindow::onAboutAction);
    
    m_exitAction = new QAction(tr("Exit"), this);
    connect(m_exitAction, &QAction::triggered, this, &MainWindow::onExitAction);
}

void MainWindow::createTrayIcon()
{
    m_trayMenu = new QMenu(this);
    m_trayMenu->addAction(m_showAction);
    m_trayMenu->addAction(m_settingsAction);
    m_trayMenu->addSeparator();
    m_trayMenu->addAction(m_aboutAction);
    m_trayMenu->addSeparator();
    m_trayMenu->addAction(m_exitAction);
    
    m_trayIcon = new QSystemTrayIcon(this);
    m_trayIcon->setContextMenu(m_trayMenu);
    m_trayIcon->setIcon(QIcon(":/icons/app.png"));
    m_trayIcon->setToolTip(tr("Voice Dictation"));
    m_trayIcon->show();
    
    connect(m_trayIcon, &QSystemTrayIcon::activated, this, &MainWindow::onTrayActivated);
}

void MainWindow::createConnections()
{
    // Button connections
    connect(m_recordButton, &QPushButton::clicked, this, &MainWindow::onStartRecording);
    connect(m_stopButton, &QPushButton::clicked, this, &MainWindow::onStopRecording);
    connect(m_clearButton, &QPushButton::clicked, this, &MainWindow::onClearText);
    connect(m_copyButton, &QPushButton::clicked, this, &MainWindow::onCopyText);
    connect(m_settingsButton, &QPushButton::clicked, this, &MainWindow::onSettingsAction);
    
    // ComboBox connections
    connect(m_languageComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), 
            this, &MainWindow::onLanguageChanged);
    
    // Speech recognizer connections
    connect(m_speechRecognizer.get(), &SpeechRecognizer::speechRecognized,
            this, &MainWindow::onSpeechRecognized);
    connect(m_speechRecognizer.get(), &SpeechRecognizer::recognitionError,
            this, &MainWindow::onRecognitionError);
    connect(m_speechRecognizer.get(), &SpeechRecognizer::recognitionStarted,
            this, [this]() { m_statusLabel->setText(tr("Recognition started")); });
    connect(m_speechRecognizer.get(), &SpeechRecognizer::recognitionStopped,
            this, [this]() { m_statusLabel->setText(tr("Recognition stopped")); });
    
    // Audio processor connections
    connect(m_audioProcessor.get(), &AudioProcessor::audioDataReady,
            m_speechRecognizer.get(), &SpeechRecognizer::processSpeech);
    connect(m_audioProcessor.get(), &AudioProcessor::errorOccurred,
            this, &MainWindow::onAudioError);
    
    // Hotkey connections
    connect(m_hotkeyManager.get(), &HotkeyManager::hotkeyPressed,
            this, &MainWindow::onHotkeyPressed);
}

void MainWindow::loadSettings()
{
    QSettings settings;
    
    // Load window geometry
    if (settings.contains("window/geometry")) {
        restoreGeometry(settings.value("window/geometry").toByteArray());
    } else {
        // Default size
        resize(800, 600);
    }
    
    // Load window state
    if (settings.contains("window/state")) {
        restoreState(settings.value("window/state").toByteArray());
    }
    
    // Start minimized?
    bool startMinimized = settings.value("general/start_minimized", false).toBool();
    if (startMinimized) {
        QTimer::singleShot(0, this, &MainWindow::hide);
    }
    
    // Configure speech recognizer
    if (m_speechRecognizer) {
        // Set language
        QString language = settings.value("language/current", "en-US").toString();
        m_speechRecognizer->setLanguage(language);
        
        // Set recognition service
        QString service = settings.value("speech/recognition_service", "offline").toString();
        if (service == "google") {
            m_speechRecognizer->setRecognitionService(RecognitionServiceType::Google);
        } else if (service == "yandex") {
            m_speechRecognizer->setRecognitionService(RecognitionServiceType::Yandex);
        } else if (service == "azure") {
            m_speechRecognizer->setRecognitionService(RecognitionServiceType::Azure);
        } else {
            m_speechRecognizer->setRecognitionService(RecognitionServiceType::Offline);
        }
        
        // Set shared API key usage
        bool useSharedApiKey = settings.value("speech/use_shared_api_key", false).toBool();
        m_speechRecognizer->setUseSharedApiKey(useSharedApiKey);
        
        // Set public API usage
        bool usePublicApi = settings.value("speech/use_public_api", false).toBool();
        m_speechRecognizer->setUsePublicApi(usePublicApi);
        
        // Set API key (used only if not using shared key)
        QString apiKey = settings.value("speech/api_key", "").toString();
        m_speechRecognizer->setApiKey(apiKey);
    }
}

void MainWindow::saveSettings()
{
    QSettings settings;
    
    // Save window geometry
    settings.setValue("window/geometry", saveGeometry());
    
    // Save window state
    settings.setValue("window/state", saveState());
    
    // Save language
    settings.setValue("language/current", m_currentLanguage);
}

void MainWindow::switchLanguage(const QString &language)
{
    if (m_currentLanguage == language) {
        return;
    }
    
    m_currentLanguage = language;
    
    // Load translator for the selected language
    QCoreApplication::removeTranslator(&m_translator);
    
    if (language != "en-US") {
        QString qmPath = QString(":/translations/voicedictation_%1.qm").arg(language.left(2));
        if (m_translator.load(qmPath)) {
            QCoreApplication::installTranslator(&m_translator);
            gLogger->info("Loaded translations for " + language.toStdString());
        } else {
            gLogger->warning("Failed to load translations for " + language.toStdString());
        }
    }
    
    // Update speech recognizer language
    m_speechRecognizer->setLanguage(language);
    
    // Retranslate UI
    retranslateUi();
}

void MainWindow::retranslateUi()
{
    // Update UI text elements
    setWindowTitle(tr("Voice Dictation"));
    m_recordButton->setText(tr("Record"));
    m_stopButton->setText(tr("Stop"));
    m_clearButton->setText(tr("Clear"));
    m_copyButton->setText(tr("Copy"));
    m_settingsButton->setText(tr("Settings"));
    m_textEdit->setPlaceholderText(tr("Recognized text will appear here..."));
    m_statusLabel->setText(tr("Ready"));
    
    // Update actions
    m_showAction->setText(tr("Show"));
    m_settingsAction->setText(tr("Settings"));
    m_aboutAction->setText(tr("About"));
    m_exitAction->setText(tr("Exit"));
    
    // Update tray icon tooltip
    m_trayIcon->setToolTip(tr("Voice Dictation"));
}

void MainWindow::onStartRecording()
{
    gLogger->info("Starting recording");
    
    m_textEdit->setReadOnly(true);
    m_audioProcessor->startRecording();
    m_speechRecognizer->startRecognition();
    m_visualizationTimer.start();
    
    m_isRecording = true;
    m_recordButton->setEnabled(false);
    m_stopButton->setEnabled(true);
    m_statusLabel->setText(tr("Recording..."));
}

void MainWindow::onStopRecording()
{
    gLogger->info("Stopping recording");
    
    m_audioProcessor->stopRecording();
    m_speechRecognizer->stopRecognition();
    m_visualizationTimer.stop();
    
    m_isRecording = false;
    m_recordButton->setEnabled(true);
    m_stopButton->setEnabled(false);
    m_statusLabel->setText(tr("Ready"));
    m_textEdit->setReadOnly(false);
    
    // Reset visualizer
    m_audioVisualizer->clear();
}

void MainWindow::onClearText()
{
    m_textEdit->clear();
}

void MainWindow::onCopyText()
{
    QString text = m_textEdit->toPlainText();
    if (!text.isEmpty()) {
        m_clipboardManager->copyToClipboard(text);
        m_statusLabel->setText(tr("Text copied to clipboard"));
    }
}

void MainWindow::onLanguageChanged(int index)
{
    QString language = m_languageComboBox->itemData(index).toString();
    gLogger->info("Language changed to " + language.toStdString());
    switchLanguage(language);
}

void MainWindow::onSettingsAction()
{
    if (!m_settingsDialog) {
        m_settingsDialog = std::make_unique<SettingsDialog>(this);
        connect(m_settingsDialog.get(), &SettingsDialog::settingsApplied, 
                this, &MainWindow::applySettings);
    }
    
    m_settingsDialog->show();
    m_settingsDialog->raise();
    m_settingsDialog->activateWindow();
}

void MainWindow::onTrayActivated(QSystemTrayIcon::ActivationReason reason)
{
    if (reason == QSystemTrayIcon::DoubleClick) {
        if (isVisible()) {
            hide();
        } else {
            show();
            activateWindow();
        }
    }
}

void MainWindow::onAboutAction()
{
    QMessageBox::about(this, tr("About Voice Dictation"),
                      tr("<h2>Voice Dictation</h2>"
                         "<p>Version: %1</p>"
                         "<p>A multilingual application for voice recognition and quick text input.</p>"
                         "<p>© 2025 Voice Dictation Team</p>")
                      .arg(QApplication::applicationVersion()));
}

void MainWindow::onExitAction()
{
    // Save settings before exiting
    saveSettings();
    
    // Ensure tray icon is hidden before exit to prevent hanging
    m_trayIcon->hide();
    
    QApplication::quit();
}

void MainWindow::updateAudioVisualization()
{
    if (m_isRecording) {
        std::vector<float> audioLevels = m_audioProcessor->getAudioLevels();
        m_audioVisualizer->updateLevels(audioLevels);
    }
}

void MainWindow::onSpeechRecognized(const QString &text)
{
    if (!text.isEmpty()) {
        gLogger->info("Speech recognized: " + text.toStdString());
        
        // Process text with text processor (for formatting, etc.)
        QString processedText = m_textProcessor->processText(text, m_currentLanguage);
        
        // Append to text edit
        m_textEdit->append(processedText);
        
        // Scroll to bottom
        m_textEdit->moveCursor(QTextCursor::End);
        m_textEdit->ensureCursorVisible();
        
        // Update status
        m_statusLabel->setText(tr("Text recognized"));
    }
}

void MainWindow::onRecognitionError(const QString &errorMessage)
{
    gLogger->error("Recognition error: " + errorMessage.toStdString());
    m_statusLabel->setText(tr("Recognition error: %1").arg(errorMessage));
}

void MainWindow::onAudioError(const QString &errorMessage)
{
    gLogger->error("Audio error: " + errorMessage.toStdString());
    m_statusLabel->setText(tr("Audio error: %1").arg(errorMessage));
    
    // Если произошла ошибка во время записи, останавливаем запись
    if (m_isRecording) {
        onStopRecording();
    }
}

void MainWindow::onHotkeyPressed(const QString &action)
{
    gLogger->info("Hotkey pressed: " + action.toStdString());
    
    if (action == "record") {
        if (!m_isRecording) {
            onStartRecording();
        } else {
            onStopRecording();
        }
    } else if (action == "copy") {
        onCopyText();
    } else if (action == "clear") {
        onClearText();
    }
}

void MainWindow::applySettings()
{
    gLogger->info("Applying settings");
    
    // Загружаем настройки из QSettings
    QSettings settings;
    
    // Применяем настройки языка
    QString language = settings.value("language/current", "en-US").toString();
    if (language != m_currentLanguage) {
        // Найдем индекс в комбобоксе для этого языка
        int langIndex = m_languageComboBox->findData(language);
        if (langIndex >= 0) {
            m_languageComboBox->setCurrentIndex(langIndex);
        }
        switchLanguage(language);
    }
    
    // Применяем настройки сервиса распознавания
    if (m_speechRecognizer) {
        // Устанавливаем сервис распознавания
        QString service = settings.value("speech/recognition_service", "offline").toString();
        RecognitionServiceType serviceType = RecognitionServiceType::Offline;
        
        if (service == "google") {
            serviceType = RecognitionServiceType::Google;
        } else if (service == "yandex") {
            serviceType = RecognitionServiceType::Yandex;
        } else if (service == "azure") {
            serviceType = RecognitionServiceType::Azure;
        }
        
        m_speechRecognizer->setRecognitionService(serviceType);
        
        // Устанавливаем общий ключ API
        bool useSharedApiKey = settings.value("speech/use_shared_api_key", false).toBool();
        m_speechRecognizer->setUseSharedApiKey(useSharedApiKey);
        
        // Устанавливаем публичный API
        bool usePublicApi = settings.value("speech/use_public_api", false).toBool();
        m_speechRecognizer->setUsePublicApi(usePublicApi);
        
        // Устанавливаем ключ API (используется только если не используется общий ключ)
        QString apiKey = settings.value("speech/api_key", "").toString();
        m_speechRecognizer->setApiKey(apiKey);
    }
    
    // Обновляем настройки аудиопроцессора
    if (m_audioProcessor) {
        // Устанавливаем частоту дискретизации
        int sampleRate = settings.value("audio/sample_rate", 16000).toInt();
        m_audioProcessor->setSampleRate(sampleRate);
        
        // Устанавливаем количество каналов
        int channels = settings.value("audio/channels", 1).toInt();
        m_audioProcessor->setChannelCount(channels);
    }
    
    // Перезагружаем горячие клавиши
    m_hotkeyManager->reloadHotkeys();
    
    // Обновляем статус
    m_statusLabel->setText(tr("Settings applied"));
    
    gLogger->info("Settings applied successfully");
} 