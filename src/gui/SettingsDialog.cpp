#include "SettingsDialog.h"
#include "../utils/Logger.h"
#include "../core/HotkeyManager.h"
#include "../core/SpeechRecognizer.h"
#include "MainWindow.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QDialogButtonBox>
#include <QSettings>
#include <QMessageBox>
#include <QMediaDevices>
#include <QAudioDevice>
#include <QFileDialog>
#include <QIcon>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QTreeWidgetItem>
#include <QDir>
#include <QFileInfo>
#include <QCoreApplication>
#include <QFile>
#include <QMutex>
#include <QTemporaryFile>

// External global logger
extern Logger* gLogger;

SettingsDialog::SettingsDialog(QWidget* parent)
    : QDialog(parent)
    , m_networkManager(nullptr)
    , m_currentDownload(nullptr)
    , m_isPaused(false)
    , m_isDownloading(false)
    , m_resumePosition(0)
    , m_completedSegments(0)
    , m_useMultiThreaded(false)
    , m_currentResourceId("")
    , m_currentResourceUrl("")
    , m_currentDestPath("")
{
    setWindowTitle(tr("Settings"));
    //setWindowIcon(QIcon(":/Icon/app.ico"));
    
    setupUi();
    loadSettings();
    
    setMinimumSize(500, 400);
}

SettingsDialog::~SettingsDialog()
{
    // Прерываем текущую загрузку, если такая есть
    if (m_currentDownload) {
        m_currentDownload->abort();
        m_currentDownload->deleteLater();
        m_currentDownload = nullptr;
    }
    
    // Освобождаем ресурсы сегментированной загрузки
    for (auto segment : m_segments) {
        if (segment->reply) {
            segment->reply->abort();
            segment->reply->deleteLater();
        }
        if (segment->tempFile) {
            segment->tempFile->close();
            delete segment->tempFile;
        }
        delete segment;
    }
    m_segments.clear();
}

void SettingsDialog::setupUi()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    
    // Create tab widget
    m_tabWidget = new QTabWidget(this);
    
    // Create tabs
    createGeneralTab();
    createHotkeysTab();
    createAudioTab();
    createLanguageTab();
    createAdvancedTab();
    createResourcesTab();
    
    // Create buttons
    QDialogButtonBox* buttonBox = new QDialogButtonBox(this);
    m_okButton = buttonBox->addButton(QDialogButtonBox::Ok);
    m_cancelButton = buttonBox->addButton(QDialogButtonBox::Cancel);
    m_applyButton = buttonBox->addButton(QDialogButtonBox::Apply);
    m_restoreDefaultsButton = buttonBox->addButton(QDialogButtonBox::RestoreDefaults);
    
    // Connect signals
    connect(m_okButton, &QPushButton::clicked, this, &SettingsDialog::onAccept);
    connect(m_cancelButton, &QPushButton::clicked, this, &SettingsDialog::onReject);
    connect(m_applyButton, &QPushButton::clicked, this, &SettingsDialog::applySettings);
    connect(m_restoreDefaultsButton, &QPushButton::clicked, this, &SettingsDialog::onRestoreDefaults);
    
    // Add widgets to layout
    mainLayout->addWidget(m_tabWidget);
    mainLayout->addWidget(buttonBox);
}

void SettingsDialog::createGeneralTab()
{
    QWidget* tab = new QWidget(this);
    QVBoxLayout* layout = new QVBoxLayout(tab);
    
    // Startup settings
    QGroupBox* startupGroup = new QGroupBox(tr("Startup"), tab);
    QVBoxLayout* startupLayout = new QVBoxLayout(startupGroup);
    
    m_startMinimizedCheckBox = new QCheckBox(tr("Start minimized to tray"), startupGroup);
    startupLayout->addWidget(m_startMinimizedCheckBox);
    
    // History settings
    QGroupBox* historyGroup = new QGroupBox(tr("Clipboard History"), tab);
    QFormLayout* historyLayout = new QFormLayout(historyGroup);
    
    m_keepHistoryCheckBox = new QCheckBox(tr("Keep clipboard history"), historyGroup);
    m_maxHistorySizeSpinBox = new QSpinBox(historyGroup);
    m_maxHistorySizeSpinBox->setRange(1, 100);
    m_maxHistorySizeSpinBox->setSingleStep(1);
    m_maxHistorySizeSpinBox->setValue(20);
    
    historyLayout->addRow("", m_keepHistoryCheckBox);
    historyLayout->addRow(tr("Maximum history size:"), m_maxHistorySizeSpinBox);
    
    // Add groups to tab
    layout->addWidget(startupGroup);
    layout->addWidget(historyGroup);
    layout->addStretch();
    
    // Add tab to tab widget
    m_tabWidget->addTab(tab, tr("General"));
}

void SettingsDialog::createHotkeysTab()
{
    QWidget* tab = new QWidget(this);
    QVBoxLayout* layout = new QVBoxLayout(tab);
    
    // Hotkeys group
    QGroupBox* hotkeysGroup = new QGroupBox(tr("Global Hotkeys"), tab);
    QFormLayout* hotkeysLayout = new QFormLayout(hotkeysGroup);
    
    // Create hotkey editors
    QStringList actions = { "record", "copy", "clear" };
    QStringList labels = { tr("Record/Stop"), tr("Copy Text"), tr("Clear Text") };
    
    for (int i = 0; i < actions.size(); ++i) {
        HotkeyWidgets widgets;
        widgets.label = new QLabel(labels[i], hotkeysGroup);
        widgets.editor = new QKeySequenceEdit(hotkeysGroup);
        widgets.resetButton = new QPushButton(tr("Reset"), hotkeysGroup);
        
        // Create layout for editor and reset button
        QHBoxLayout* editorLayout = new QHBoxLayout();
        editorLayout->addWidget(widgets.editor);
        editorLayout->addWidget(widgets.resetButton);
        
        // Add to form layout
        hotkeysLayout->addRow(widgets.label, editorLayout);
        
        // Store widgets
        m_hotkeyWidgets[actions[i]] = widgets;
        
        // Connect signals
        connect(widgets.editor, &QKeySequenceEdit::keySequenceChanged, this, &SettingsDialog::onHotkeyChanged);
        connect(widgets.resetButton, &QPushButton::clicked, [this, action = actions[i]]() {
            QSettings settings;
            QString defaultHotkey;
            
            if (action == "record") defaultHotkey = "Ctrl+Alt+R";
            else if (action == "copy") defaultHotkey = "Ctrl+Alt+C";
            else if (action == "clear") defaultHotkey = "Ctrl+Alt+X";
            
            m_hotkeyWidgets[action].editor->setKeySequence(QKeySequence(defaultHotkey));
        });
    }
    
    // Add group to tab
    layout->addWidget(hotkeysGroup);
    layout->addStretch();
    
    // Add tab to tab widget
    m_tabWidget->addTab(tab, tr("Hotkeys"));
}

void SettingsDialog::createAudioTab()
{
    QWidget* tab = new QWidget(this);
    QVBoxLayout* layout = new QVBoxLayout(tab);
    
    // Audio input settings
    QGroupBox* inputGroup = new QGroupBox(tr("Audio Input"), tab);
    QFormLayout* inputLayout = new QFormLayout(inputGroup);
    
    m_inputDeviceComboBox = new QComboBox(inputGroup);
    
    // Populate input devices
    const QList<QAudioDevice> devices = QMediaDevices::audioInputs();
    for (const QAudioDevice& device : devices) {
        m_inputDeviceComboBox->addItem(device.description(), QVariant::fromValue(device));
    }
    
    inputLayout->addRow(tr("Input device:"), m_inputDeviceComboBox);
    
    // Audio format settings
    QGroupBox* formatGroup = new QGroupBox(tr("Audio Format"), tab);
    QFormLayout* formatLayout = new QFormLayout(formatGroup);
    
    m_sampleRateComboBox = new QComboBox(formatGroup);
    m_sampleRateComboBox->addItem("8000 Hz", 8000);
    m_sampleRateComboBox->addItem("16000 Hz", 16000);
    m_sampleRateComboBox->addItem("22050 Hz", 22050);
    m_sampleRateComboBox->addItem("44100 Hz", 44100);
    m_sampleRateComboBox->addItem("48000 Hz", 48000);
    
    m_channelsComboBox = new QComboBox(formatGroup);
    m_channelsComboBox->addItem(tr("Mono"), 1);
    m_channelsComboBox->addItem(tr("Stereo"), 2);
    
    formatLayout->addRow(tr("Sample rate:"), m_sampleRateComboBox);
    formatLayout->addRow(tr("Channels:"), m_channelsComboBox);
    
    // Add groups to tab
    layout->addWidget(inputGroup);
    layout->addWidget(formatGroup);
    layout->addStretch();
    
    // Add tab to tab widget
    m_tabWidget->addTab(tab, tr("Audio"));
}

void SettingsDialog::createLanguageTab()
{
    QWidget* tab = new QWidget(this);
    QVBoxLayout* layout = new QVBoxLayout(tab);
    
    // Language options group
    QGroupBox* languageGroup = new QGroupBox(tr("Language Options"), tab);
    QVBoxLayout* languageLayout = new QVBoxLayout(languageGroup);
    
    m_autoCorrectCheckBox = new QCheckBox(tr("Auto-correct text"), languageGroup);
    m_capitalizeFirstCheckBox = new QCheckBox(tr("Capitalize first letter of sentences"), languageGroup);
    m_addPunctuationCheckBox = new QCheckBox(tr("Add punctuation automatically"), languageGroup);
    
    languageLayout->addWidget(m_autoCorrectCheckBox);
    languageLayout->addWidget(m_capitalizeFirstCheckBox);
    languageLayout->addWidget(m_addPunctuationCheckBox);
    
    // Recognition service group
    QGroupBox* serviceGroup = new QGroupBox(tr("Recognition Service"), tab);
    QVBoxLayout* serviceLayout = new QVBoxLayout(serviceGroup);
    
    QFormLayout* formLayout = new QFormLayout();
    m_recognitionServiceComboBox = new QComboBox(serviceGroup);
    m_recognitionServiceComboBox->addItem("Whisper Local", "whisper");
    m_recognitionServiceComboBox->addItem("DeepSpeech Local", "deepspeech");
    m_recognitionServiceComboBox->addItem("Google Speech API", "google");
    m_recognitionServiceComboBox->addItem("Microsoft Azure", "azure");
    m_recognitionServiceComboBox->addItem("Yandex SpeechKit", "yandex");
    
    formLayout->addRow(tr("Service:"), m_recognitionServiceComboBox);
    
    m_apiKeyEdit = new QLineEdit(serviceGroup);
    m_apiKeyEdit->setEchoMode(QLineEdit::Password);
    formLayout->addRow(tr("API Key:"), m_apiKeyEdit);
    
    m_azureRegionEdit = new QLineEdit(serviceGroup);
    m_azureRegionEdit->setPlaceholderText(tr("e.g. westeurope, eastus"));
    formLayout->addRow(tr("Azure Region:"), m_azureRegionEdit);
    
    m_usePublicApiCheckBox = new QCheckBox(tr("Use public API (no key required)"), serviceGroup);
    m_usePublicApiCheckBox->setToolTip(tr("Use public API endpoints with limited functionality"));
    
    serviceLayout->addLayout(formLayout);
    serviceLayout->addWidget(m_usePublicApiCheckBox);
    
    layout->addWidget(languageGroup);
    layout->addWidget(serviceGroup);
    layout->addStretch();
    
    // Add tab to tab widget
    m_tabWidget->addTab(tab, tr("Language"));
    
    // Connect signals
    connect(m_recognitionServiceComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), 
            this, &SettingsDialog::handleRecognitionServiceChanged);
    connect(m_usePublicApiCheckBox, &QCheckBox::toggled, this, &SettingsDialog::onUsePublicApiToggled);
    
    // Initialize visibility based on current selection
    handleRecognitionServiceChanged(m_recognitionServiceComboBox->currentIndex());
}

void SettingsDialog::createAdvancedTab()
{
    QWidget* tab = new QWidget(this);
    QVBoxLayout* layout = new QVBoxLayout(tab);
    
    // Debug settings
    QGroupBox* debugGroup = new QGroupBox(tr("Debug Options"), tab);
    QVBoxLayout* debugLayout = new QVBoxLayout(debugGroup);
    
    m_debugModeCheckBox = new QCheckBox(tr("Enable debug mode"), debugGroup);
    debugLayout->addWidget(m_debugModeCheckBox);
    
    // Recognition settings
    QGroupBox* recognitionGroup = new QGroupBox(tr("Recognition Settings"), tab);
    QFormLayout* recognitionLayout = new QFormLayout(recognitionGroup);
    
    m_recognitionThresholdSpinBox = new QSpinBox(recognitionGroup);
    m_recognitionThresholdSpinBox->setRange(0, 100);
    m_recognitionThresholdSpinBox->setSingleStep(1);
    m_recognitionThresholdSpinBox->setValue(50);
    m_recognitionThresholdSpinBox->setSuffix("%");
    recognitionLayout->addRow(tr("Voice activation threshold:"), m_recognitionThresholdSpinBox);
    
    m_customModelPathEdit = new QLineEdit(recognitionGroup);
    m_customModelPathEdit->setPlaceholderText(tr("Leave empty for default model location"));
    QPushButton* browseButton = new QPushButton(tr("Browse..."), recognitionGroup);
    QHBoxLayout* modelPathLayout = new QHBoxLayout();
    modelPathLayout->addWidget(m_customModelPathEdit);
    modelPathLayout->addWidget(browseButton);
    recognitionLayout->addRow(tr("Custom model path:"), modelPathLayout);
    
    connect(browseButton, &QPushButton::clicked, [this]() {
        QString modelPath = QFileDialog::getExistingDirectory(this, 
            tr("Select Model Directory"), 
            QString(), 
            QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
        
        if (!modelPath.isEmpty()) {
            m_customModelPathEdit->setText(modelPath);
        }
    });
    
    // Logging settings
    QGroupBox* loggingGroup = new QGroupBox(tr("Logging"), tab);
    QVBoxLayout* loggingLayout = new QVBoxLayout(loggingGroup);
    
    m_enableLoggingCheckBox = new QCheckBox(tr("Enable detailed logging"), loggingGroup);
    loggingLayout->addWidget(m_enableLoggingCheckBox);
    
    QFormLayout* logLevelForm = new QFormLayout();
    m_logLevelComboBox = new QComboBox(loggingGroup);
    m_logLevelComboBox->addItem(tr("Error"), "error");
    m_logLevelComboBox->addItem(tr("Warning"), "warning");
    m_logLevelComboBox->addItem(tr("Info"), "info");
    m_logLevelComboBox->addItem(tr("Debug"), "debug");
    m_logLevelComboBox->addItem(tr("Verbose"), "verbose");
    logLevelForm->addRow(tr("Log level:"), m_logLevelComboBox);
    
    loggingLayout->addLayout(logLevelForm);
    
    // Connect checkbox to enable/disable log level selection
    connect(m_enableLoggingCheckBox, &QCheckBox::toggled, m_logLevelComboBox, &QComboBox::setEnabled);
    
    // Add groups to tab
    layout->addWidget(debugGroup);
    layout->addWidget(recognitionGroup);
    layout->addWidget(loggingGroup);
    layout->addStretch();
    
    // Add tab to tab widget
    m_tabWidget->addTab(tab, tr("Advanced"));
    
    // Initial state
    m_logLevelComboBox->setEnabled(false);
}

void SettingsDialog::createResourcesTab()
{
    QWidget* tab = new QWidget(this);
    QVBoxLayout* layout = new QVBoxLayout(tab);
    
    // Создаем сетевой менеджер для загрузки ресурсов
    m_networkManager = new QNetworkAccessManager(this);
    m_currentDownload = nullptr;
    
    // Группа фильтрации и поиска
    QGroupBox* filterGroup = new QGroupBox(tr("Поиск и фильтрация"), tab);
    QHBoxLayout* filterLayout = new QHBoxLayout(filterGroup);
    
    // Фильтр по языку
    QLabel* languageLabel = new QLabel(tr("Язык:"), filterGroup);
    m_resourceLanguageComboBox = new QComboBox(filterGroup);
    m_resourceLanguageComboBox->addItem(tr("Все языки"), "all");
    m_resourceLanguageComboBox->addItem(tr("Английский"), "en");
    m_resourceLanguageComboBox->addItem(tr("Русский"), "ru");
    m_resourceLanguageComboBox->addItem(tr("Немецкий"), "de");
    m_resourceLanguageComboBox->addItem(tr("Французский"), "fr");
    m_resourceLanguageComboBox->addItem(tr("Испанский"), "es");
    m_resourceLanguageComboBox->addItem(tr("Итальянский"), "it");
    m_resourceLanguageComboBox->addItem(tr("Китайский"), "zh");
    m_resourceLanguageComboBox->addItem(tr("Японский"), "ja");
    
    // Поиск по названию
    QLabel* searchLabel = new QLabel(tr("Поиск:"), filterGroup);
    m_resourceSearchEdit = new QLineEdit(filterGroup);
    m_resourceSearchEdit->setPlaceholderText(tr("Введите текст для поиска"));
    
    // Добавляем в layout группы фильтрации
    filterLayout->addWidget(languageLabel);
    filterLayout->addWidget(m_resourceLanguageComboBox);
    filterLayout->addWidget(searchLabel);
    filterLayout->addWidget(m_resourceSearchEdit);
    
    // Список ресурсов
    QGroupBox* resourcesGroup = new QGroupBox(tr("Доступные ресурсы"), tab);
    QVBoxLayout* resourcesLayout = new QVBoxLayout(resourcesGroup);
    
    m_resourcesTreeWidget = new QTreeWidget(resourcesGroup);
    m_resourcesTreeWidget->setColumnCount(5);
    m_resourcesTreeWidget->setHeaderLabels({
        tr("Название"), 
        tr("Тип"), 
        tr("Язык"), 
        tr("Размер"),
        tr("Статус")
    });
    m_resourcesTreeWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    m_resourcesTreeWidget->setRootIsDecorated(false);
    m_resourcesTreeWidget->setSortingEnabled(true);
    m_resourcesTreeWidget->sortByColumn(0, Qt::AscendingOrder);
    
    resourcesLayout->addWidget(m_resourcesTreeWidget);
    
    // Кнопки действий и прогресс загрузки
    QGroupBox* actionsGroup = new QGroupBox(tr("Действия"), tab);
    QVBoxLayout* actionsLayout = new QVBoxLayout(actionsGroup);
    
    // Создаем горизонтальный layout для кнопок
    QHBoxLayout* buttonsLayout = new QHBoxLayout();
    
    // Кнопки управления загрузкой
    m_downloadButton = new QPushButton(tr("Скачать выбранный ресурс"), actionsGroup);
    m_downloadButton->setEnabled(false);
    
    m_pauseResumeButton = new QPushButton(tr("Пауза"), actionsGroup);
    m_pauseResumeButton->setEnabled(false);
    
    m_cancelDownloadButton = new QPushButton(tr("Отмена"), actionsGroup);
    m_cancelDownloadButton->setEnabled(false);
    
    // Добавляем кнопки в горизонтальный layout
    buttonsLayout->addWidget(m_downloadButton);
    buttonsLayout->addWidget(m_pauseResumeButton);
    buttonsLayout->addWidget(m_cancelDownloadButton);
    
    // Прогресс загрузки
    m_downloadProgressBar = new QProgressBar(actionsGroup);
    m_downloadProgressBar->setRange(0, 100);
    m_downloadProgressBar->setValue(0);
    m_downloadProgressBar->setVisible(false);
    
    m_downloadStatusLabel = new QLabel(actionsGroup);
    m_downloadStatusLabel->setVisible(false);
    
    // Опции многопоточной загрузки
    QGroupBox* downloadOptionsGroup = new QGroupBox(tr("Настройки загрузки"), actionsGroup);
    QFormLayout* downloadOptionsLayout = new QFormLayout(downloadOptionsGroup);
    
    QCheckBox* useMultiThreadedCheckBox = new QCheckBox(tr("Многопоточная загрузка"), downloadOptionsGroup);
    m_threadCountSpinBox = new QSpinBox(downloadOptionsGroup);
    m_threadCountSpinBox->setRange(2, 8);
    m_threadCountSpinBox->setValue(4);
    m_threadCountSpinBox->setEnabled(false);
    
    downloadOptionsLayout->addRow(useMultiThreadedCheckBox);
    downloadOptionsLayout->addRow(tr("Количество потоков:"), m_threadCountSpinBox);
    
    // Соединяем сигнал переключателя с состоянием спиннера
    connect(useMultiThreadedCheckBox, &QCheckBox::toggled, [this](bool checked) {
        m_useMultiThreaded = checked;
        m_threadCountSpinBox->setEnabled(checked);
    });
    
    // Группа настроек прокси
    m_proxyGroup = new QGroupBox(tr("Настройки прокси/VPN"), actionsGroup);
    QFormLayout* proxyLayout = new QFormLayout(m_proxyGroup);
    
    m_useProxyCheckBox = new QCheckBox(tr("Использовать прокси"), m_proxyGroup);
    
    m_proxyTypeComboBox = new QComboBox(m_proxyGroup);
    m_proxyTypeComboBox->addItem(tr("HTTP"), QNetworkProxy::HttpProxy);
    m_proxyTypeComboBox->addItem(tr("SOCKS5"), QNetworkProxy::Socks5Proxy);
    m_proxyTypeComboBox->addItem(tr("Системный"), QNetworkProxy::DefaultProxy);
    m_proxyTypeComboBox->setEnabled(false);
    
    m_proxyHostEdit = new QLineEdit(m_proxyGroup);
    m_proxyHostEdit->setEnabled(false);
    m_proxyHostEdit->setPlaceholderText("proxy.example.com");
    
    m_proxyPortSpinBox = new QSpinBox(m_proxyGroup);
    m_proxyPortSpinBox->setRange(1, 65535);
    m_proxyPortSpinBox->setValue(8080);
    m_proxyPortSpinBox->setEnabled(false);
    
    m_proxyUserEdit = new QLineEdit(m_proxyGroup);
    m_proxyUserEdit->setEnabled(false);
    m_proxyUserEdit->setPlaceholderText(tr("Имя пользователя (необязательно)"));
    
    m_proxyPasswordEdit = new QLineEdit(m_proxyGroup);
    m_proxyPasswordEdit->setEnabled(false);
    m_proxyPasswordEdit->setEchoMode(QLineEdit::Password);
    m_proxyPasswordEdit->setPlaceholderText(tr("Пароль (необязательно)"));
    
    proxyLayout->addRow("", m_useProxyCheckBox);
    proxyLayout->addRow(tr("Тип прокси:"), m_proxyTypeComboBox);
    proxyLayout->addRow(tr("Хост:"), m_proxyHostEdit);
    proxyLayout->addRow(tr("Порт:"), m_proxyPortSpinBox);
    proxyLayout->addRow(tr("Пользователь:"), m_proxyUserEdit);
    proxyLayout->addRow(tr("Пароль:"), m_proxyPasswordEdit);
    
    // Соединяем сигнал переключателя прокси с состоянием элементов управления
    connect(m_useProxyCheckBox, &QCheckBox::toggled, [this](bool checked) {
        m_proxyTypeComboBox->setEnabled(checked);
        m_proxyHostEdit->setEnabled(checked);
        m_proxyPortSpinBox->setEnabled(checked);
        m_proxyUserEdit->setEnabled(checked);
        m_proxyPasswordEdit->setEnabled(checked);
        
        if (checked) {
            setupProxy();
        } else {
            // Сбрасываем прокси на прямое соединение
            m_networkProxy.setType(QNetworkProxy::NoProxy);
            m_networkManager->setProxy(m_networkProxy);
        }
    });
    
    // Соединяем сигнал изменения типа прокси
    connect(m_proxyTypeComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), 
            this, &SettingsDialog::onProxyTypeChanged);
    
    // Добавляем элементы в layout группы действий
    actionsLayout->addLayout(buttonsLayout);
    actionsLayout->addWidget(m_downloadProgressBar);
    actionsLayout->addWidget(m_downloadStatusLabel);
    actionsLayout->addWidget(downloadOptionsGroup);
    actionsLayout->addWidget(m_proxyGroup);
    
    // Добавляем группы в основной layout
    layout->addWidget(filterGroup);
    layout->addWidget(resourcesGroup);
    layout->addWidget(actionsGroup);
    
    // Добавляем вкладку
    m_tabWidget->addTab(tab, tr("Ресурсы"));
    
    // Подключаем сигналы
    connect(m_resourceLanguageComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), 
            this, &SettingsDialog::onLanguageFilterChanged);
    connect(m_resourceSearchEdit, &QLineEdit::textChanged, 
            this, &SettingsDialog::onResourceSearchTextChanged);
    connect(m_resourcesTreeWidget, &QTreeWidget::itemSelectionChanged, 
            [this]() {
                m_downloadButton->setEnabled(!m_resourcesTreeWidget->selectedItems().isEmpty() && !m_isDownloading);
            });
    connect(m_downloadButton, &QPushButton::clicked, 
            this, &SettingsDialog::onDownloadButtonClicked);
    connect(m_pauseResumeButton, &QPushButton::clicked,
            this, &SettingsDialog::onPauseResumeButtonClicked);
    connect(m_cancelDownloadButton, &QPushButton::clicked,
            this, &SettingsDialog::onCancelButtonClicked);
            
    // Заполняем список ресурсов начальными данными
    populateResourcesList();
}

void SettingsDialog::loadSettings()
{
    QSettings settings;
    
    // General settings
    m_startMinimizedCheckBox->setChecked(settings.value("general/startMinimized", false).toBool());
    m_keepHistoryCheckBox->setChecked(settings.value("general/keepHistory", true).toBool());
    m_maxHistorySizeSpinBox->setValue(settings.value("general/maxHistorySize", 100).toInt());
    
    // Hotkey settings
    settings.beginGroup("hotkeys");
    for (const QString& action : m_hotkeyWidgets.keys()) {
        QString defaultHotkey;
        if (action == "record") defaultHotkey = "Ctrl+Alt+R";
        else if (action == "copy") defaultHotkey = "Ctrl+Alt+C";
        else if (action == "clear") defaultHotkey = "Ctrl+Alt+X";
        
        QString hotkeyStr = settings.value(action, defaultHotkey).toString();
        m_hotkeyWidgets[action].editor->setKeySequence(QKeySequence(hotkeyStr));
    }
    settings.endGroup();
    
    // Audio settings
    QString inputDeviceName = settings.value("audio/input_device", "").toString();
    if (!inputDeviceName.isEmpty()) {
        int index = m_inputDeviceComboBox->findText(inputDeviceName);
        if (index >= 0) {
            m_inputDeviceComboBox->setCurrentIndex(index);
        }
    }
    
    int sampleRate = settings.value("audio/sample_rate", 16000).toInt();
    int index = m_sampleRateComboBox->findData(sampleRate);
    if (index >= 0) {
        m_sampleRateComboBox->setCurrentIndex(index);
    }
    
    int channels = settings.value("audio/channels", 1).toInt();
    index = m_channelsComboBox->findData(channels);
    if (index >= 0) {
        m_channelsComboBox->setCurrentIndex(index);
    }
    
    // Language settings
    m_autoCorrectCheckBox->setChecked(settings.value("language/autoCorrect", true).toBool());
    m_capitalizeFirstCheckBox->setChecked(settings.value("language/capitalizeFirst", true).toBool());
    m_addPunctuationCheckBox->setChecked(settings.value("language/addPunctuation", true).toBool());
    
    // Speech recognition service settings
    QString recognitionService = settings.value("recognition/service", "whisper").toString();
    int serviceIndex = m_recognitionServiceComboBox->findData(recognitionService);
    if (serviceIndex >= 0) {
        m_recognitionServiceComboBox->setCurrentIndex(serviceIndex);
    }
    
    m_apiKeyEdit->setText(settings.value("recognition/apiKey", "").toString());
    m_apiKeyEdit->setEnabled(recognitionService != "whisper");
    
    // Загружаем значение региона Azure
    m_azureRegionEdit->setText(settings.value("recognition/azureRegion", "westeurope").toString());
    bool isAzure = (recognitionService == "azure");
    m_azureRegionEdit->setVisible(isAzure);
    m_azureRegionEdit->setEnabled(isAzure);
    
    // Advanced settings
    m_debugModeCheckBox->setChecked(settings.value("advanced/debug_mode", false).toBool());
    m_recognitionThresholdSpinBox->setValue(settings.value("advanced/recognition_threshold", 50).toInt());
    m_customModelPathEdit->setText(settings.value("advanced/custom_model_path", "").toString());
    m_enableLoggingCheckBox->setChecked(settings.value("advanced/enable_logging", false).toBool());
    
    QString logLevel = settings.value("advanced/log_level", "info").toString();
    index = m_logLevelComboBox->findData(logLevel);
    if (index >= 0) {
        m_logLevelComboBox->setCurrentIndex(index);
    }
    m_logLevelComboBox->setEnabled(m_enableLoggingCheckBox->isChecked());
    
    // Public API settings
    m_usePublicApiCheckBox->setChecked(settings.value("speech/use_public_api", false).toBool());
    m_usePublicApiCheckBox->setEnabled(recognitionService != "whisper");
}

void SettingsDialog::saveSettings()
{
    QSettings settings;
    
    // General settings
    settings.setValue("general/startMinimized", m_startMinimizedCheckBox->isChecked());
    settings.setValue("general/keepHistory", m_keepHistoryCheckBox->isChecked());
    settings.setValue("general/maxHistorySize", m_maxHistorySizeSpinBox->value());
    
    // Hotkey settings
    settings.beginGroup("hotkeys");
    for (const QString& action : m_hotkeyWidgets.keys()) {
        settings.setValue(action, m_hotkeyWidgets[action].editor->keySequence().toString());
    }
    settings.endGroup();
    
    // Audio settings
    int inputDeviceIndex = m_inputDeviceComboBox->currentIndex();
    if (inputDeviceIndex >= 0) {
        settings.setValue("audio/input_device", m_inputDeviceComboBox->itemText(inputDeviceIndex));
    }
    
    settings.setValue("audio/sample_rate", m_sampleRateComboBox->currentData().toInt());
    settings.setValue("audio/channels", m_channelsComboBox->currentData().toInt());
    
    // Language settings
    settings.setValue("language/autoCorrect", m_autoCorrectCheckBox->isChecked());
    settings.setValue("language/capitalizeFirst", m_capitalizeFirstCheckBox->isChecked());
    settings.setValue("language/addPunctuation", m_addPunctuationCheckBox->isChecked());
    
    // Speech recognition service settings
    int serviceIndex = m_recognitionServiceComboBox->currentIndex();
    if (serviceIndex >= 0) {
        settings.setValue("recognition/service", m_recognitionServiceComboBox->itemData(serviceIndex).toString());
    }
    
    settings.setValue("recognition/apiKey", m_apiKeyEdit->text());
    settings.setValue("speech/use_public_api", m_usePublicApiCheckBox->isChecked());
    
    // Сохраняем регион Azure
    settings.setValue("recognition/azureRegion", m_azureRegionEdit->text());
    
    // Advanced settings
    settings.setValue("advanced/debug_mode", m_debugModeCheckBox->isChecked());
    settings.setValue("advanced/recognition_threshold", m_recognitionThresholdSpinBox->value());
    settings.setValue("advanced/custom_model_path", m_customModelPathEdit->text());
    settings.setValue("advanced/enable_logging", m_enableLoggingCheckBox->isChecked());
    
    int logLevelIndex = m_logLevelComboBox->currentIndex();
    if (logLevelIndex >= 0) {
        settings.setValue("advanced/log_level", m_logLevelComboBox->itemData(logLevelIndex).toString());
    }
}

void SettingsDialog::applySettings()
{
    gLogger->info("Applying settings");
    
    // Save settings
    saveSettings();
    
    // Notify that settings were applied
    emit settingsApplied();
}

void SettingsDialog::restoreDefaults()
{
    gLogger->info("Restoring default settings");
    
    // General settings
    m_startMinimizedCheckBox->setChecked(false);
    m_keepHistoryCheckBox->setChecked(true);
    m_maxHistorySizeSpinBox->setValue(20);
    
    // Hotkey settings
    m_hotkeyWidgets["record"].editor->setKeySequence(QKeySequence("Ctrl+Alt+R"));
    m_hotkeyWidgets["copy"].editor->setKeySequence(QKeySequence("Ctrl+Alt+C"));
    m_hotkeyWidgets["clear"].editor->setKeySequence(QKeySequence("Ctrl+Alt+X"));
    
    // Audio settings
    m_sampleRateComboBox->setCurrentText("16000 Hz");
    m_channelsComboBox->setCurrentText("Mono");
    
    // Language settings
    m_autoCorrectCheckBox->setChecked(true);
    m_capitalizeFirstCheckBox->setChecked(true);
    m_addPunctuationCheckBox->setChecked(true);
    
    // Recognition service
    m_recognitionServiceComboBox->setCurrentIndex(m_recognitionServiceComboBox->findData("whisper"));
    m_apiKeyEdit->setText("");
    m_apiKeyEdit->setEnabled(false);
    
    // Advanced settings
    m_debugModeCheckBox->setChecked(false);
    m_recognitionThresholdSpinBox->setValue(50);
    m_customModelPathEdit->setText("");
    m_enableLoggingCheckBox->setChecked(false);
    m_logLevelComboBox->setCurrentIndex(m_logLevelComboBox->findData("info"));
    m_logLevelComboBox->setEnabled(false);
    
    // Public API settings
    m_usePublicApiCheckBox->setChecked(false);
    m_usePublicApiCheckBox->setEnabled(false);
}

void SettingsDialog::onAccept()
{
    applySettings();
    accept();
}

void SettingsDialog::onReject()
{
    reject();
}

void SettingsDialog::onRestoreDefaults()
{
    QMessageBox::StandardButton result = QMessageBox::question(this,
        tr("Restore Defaults"),
        tr("Are you sure you want to restore all settings to their default values?"),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No);
    
    if (result == QMessageBox::Yes) {
        restoreDefaults();
    }
}

void SettingsDialog::onHotkeyChanged()
{
    // This method would handle conflicts between hotkeys
    // For this simplified implementation, we don't implement conflict resolution
}

void SettingsDialog::updateHotkeyLabels()
{
    // This method would update the hotkey labels
    // For this simplified implementation, we don't implement dynamic label updating
}

void SettingsDialog::handleRecognitionServiceChanged(int index)
{
    QString service = m_recognitionServiceComboBox->itemData(index).toString();
    
    // Show/hide Azure region input based on service
    bool isAzure = (service == "azure");
    m_azureRegionEdit->setVisible(isAzure);
    m_tabWidget->parentWidget()->findChild<QLabel*>()->setVisible(isAzure);
    
    // Show/hide API key input based on service
    bool needsApiKey = (service != "whisper" && service != "deepspeech");
    m_apiKeyEdit->setVisible(needsApiKey);
    
    // Update labels and tooltips
    if (service == "google") {
        m_apiKeyEdit->setPlaceholderText(tr("Google Cloud API Key"));
        m_usePublicApiCheckBox->setEnabled(true);
    } else if (service == "azure") {
        m_apiKeyEdit->setPlaceholderText(tr("Azure Speech Service Key"));
        m_usePublicApiCheckBox->setEnabled(true);
    } else if (service == "yandex") {
        m_apiKeyEdit->setPlaceholderText(tr("Yandex SpeechKit API Key"));
        m_usePublicApiCheckBox->setEnabled(true);
    } else {
        m_usePublicApiCheckBox->setEnabled(false);
    }
    
    // Update UI based on public API checkbox
    onUsePublicApiToggled(m_usePublicApiCheckBox->isChecked());
}

void SettingsDialog::onUsePublicApiToggled(bool checked) {
    if (checked) {
        m_apiKeyEdit->setEnabled(false);
        if (m_recognitionServiceComboBox->currentData().toString() == "azure") {
            m_azureRegionEdit->setEnabled(false);
        }
    } else {
        m_apiKeyEdit->setEnabled(true);
        m_azureRegionEdit->setEnabled(true);
    }
}

void SettingsDialog::onLanguageFilterChanged(int index)
{
    QString languageFilter = m_resourceLanguageComboBox->itemData(index).toString();
    if (languageFilter == "all") {
        languageFilter = QString(); // пустая строка для "всех языков"
    }
    
    populateResourcesList(languageFilter, m_resourceSearchEdit->text());
}

void SettingsDialog::onResourceSearchTextChanged(const QString& text)
{
    QString languageFilter = m_resourceLanguageComboBox->currentData().toString();
    if (languageFilter == "all") {
        languageFilter = QString();
    }
    
    populateResourcesList(languageFilter, text);
}

void SettingsDialog::onDownloadButtonClicked()
{
    QList<QTreeWidgetItem*> selectedItems = m_resourcesTreeWidget->selectedItems();
    if (selectedItems.isEmpty() || m_isDownloading) {
        return;
    }
    
    QTreeWidgetItem* item = selectedItems.first();
    m_currentResourceId = item->data(0, Qt::UserRole).toString();
    m_currentResourceUrl = item->data(1, Qt::UserRole).toString();
    QString resourceType = item->text(1);
    QString resourceLang = item->text(2);
    
    // Определяем путь для сохранения ресурса
    QString destPath;
    if (resourceType == tr("Модель")) {
        destPath = QCoreApplication::applicationDirPath() + "/models/";
    } 
    else if (resourceType == tr("Словарь")) {
        destPath = QCoreApplication::applicationDirPath() + "/dictionaries/";
    }
    else if (resourceType == tr("Библиотека")) {
        destPath = QCoreApplication::applicationDirPath() + "/lib/";
    }
    else {
        destPath = QCoreApplication::applicationDirPath() + "/resources/";
    }
    
    // Создаем директорию, если не существует
    QDir dir(destPath);
    if (!dir.exists()) {
        dir.mkpath(".");
    }
    
    // Формируем полный путь файла
    QString fileName = QFileInfo(m_currentResourceUrl).fileName();
    if (fileName.isEmpty()) {
        fileName = m_currentResourceId;
    }
    
    // Добавляем префикс языка, если ресурс языкозависимый
    if (!resourceLang.isEmpty() && resourceLang != "multi") {
        if (!fileName.contains(resourceLang)) {
            fileName = resourceLang + "_" + fileName;
        }
    }
    
    m_currentDestPath = destPath + fileName;
    
    // Устанавливаем прокси, если настроен
    if (m_useProxyCheckBox->isChecked()) {
        setupProxy();
    }
    
    // Сбрасываем состояние загрузки
    m_resumePosition = 0;
    m_isPaused = false;
    
    // Запускаем таймер для отслеживания скорости
    m_downloadTimer.start();
    
    // Начинаем загрузку
    if (m_useMultiThreaded) {
        // Используем многопоточную загрузку
        int threadCount = m_threadCountSpinBox->value();
        startSegmentedDownload(m_currentResourceUrl, m_currentDestPath, threadCount);
    } else {
        // Используем обычную загрузку
        downloadResource(m_currentResourceUrl, m_currentDestPath);
    }
    
    // Обновляем UI
    updateDownloadControls(true);
    m_downloadProgressBar->setValue(0);
    m_downloadStatusLabel->setText(tr("Загрузка %1...").arg(fileName));
}

void SettingsDialog::onDownloadProgress(qint64 bytesReceived, qint64 bytesTotal)
{
    if (!m_isDownloading || m_isPaused) {
        return;
    }
    
    // Прогресс для обычной загрузки
    if (!m_useMultiThreaded) {
        if (bytesTotal <= 0) {
            m_downloadProgressBar->setRange(0, 0); // Бесконечный прогресс
            return;
        }
        
        // Учитываем ранее загруженные байты при возобновлении
        qint64 totalReceived = m_resumePosition + bytesReceived;
        int percent = static_cast<int>((totalReceived * 100) / (m_resumePosition + bytesTotal));
        m_downloadProgressBar->setRange(0, 100);
        m_downloadProgressBar->setValue(percent);
        
        // Рассчитываем скорость загрузки
        double elapsedSecs = m_downloadTimer.elapsed() / 1000.0;
        double speed = 0.0;
        if (elapsedSecs > 0) {
            speed = bytesReceived / elapsedSecs / 1024.0; // KB/s
        }
        
        m_downloadStatusLabel->setText(tr("Загрузка: %1 / %2 МБ (%3%) - %4 КБ/с")
            .arg(totalReceived / 1024.0 / 1024.0, 0, 'f', 1)
            .arg((m_resumePosition + bytesTotal) / 1024.0 / 1024.0, 0, 'f', 1)
            .arg(percent)
            .arg(speed, 0, 'f', 1));
    } else {
        // Для многопоточной загрузки прогресс обновляется отдельно
    }
}

void SettingsDialog::onDownloadFinished()
{
    if (!m_currentDownload || m_isPaused) {
        return;
    }
    
    if (m_currentDownload->error() == QNetworkReply::NoError) {
        // Успешная загрузка
        QUrl redirectUrl = m_currentDownload->attribute(QNetworkRequest::RedirectionTargetAttribute).toUrl();
        if (!redirectUrl.isEmpty()) {
            // Если это редирект, продолжаем загрузку по новому URL
            QUrl newUrl = m_currentDownload->url().resolved(redirectUrl);
            m_currentDownload->deleteLater();
            
            QNetworkRequest request;
            request.setUrl(newUrl);
            
            m_currentDownload = m_networkManager->get(request);
            connect(m_currentDownload, &QNetworkReply::downloadProgress, 
                    this, &SettingsDialog::onDownloadProgress);
            connect(m_currentDownload, &QNetworkReply::finished, 
                    this, &SettingsDialog::onDownloadFinished);
                    
            return;
        }
        
        // Сохраняем файл
        QString filePath = m_currentDownload->property("destinationPath").toString();
        QFile file(filePath);
        
        // Открываем файл в режиме добавления или записи в зависимости от наличия m_resumePosition
        QIODevice::OpenMode mode = m_resumePosition > 0 ? QIODevice::Append : QIODevice::WriteOnly;
        if (file.open(mode)) {
            file.write(m_currentDownload->readAll());
            file.close();
            
            m_downloadStatusLabel->setText(tr("Загрузка завершена"));
            
            // Обновляем статус ресурса в списке
            QList<QTreeWidgetItem*> selectedItems = m_resourcesTreeWidget->selectedItems();
            if (!selectedItems.isEmpty()) {
                selectedItems.first()->setText(4, tr("Установлен"));
            }
        } 
        else {
            m_downloadStatusLabel->setText(tr("Ошибка сохранения файла"));
        }
    } 
    else {
        // Ошибка загрузки
        m_downloadStatusLabel->setText(tr("Ошибка загрузки: %1").arg(m_currentDownload->errorString()));
    }
    
    m_currentDownload->deleteLater();
    m_currentDownload = nullptr;
    
    // Сбрасываем состояние загрузки
    m_isDownloading = false;
    m_isPaused = false;
    m_resumePosition = 0;
    
    // Обновляем UI
    updateDownloadControls(false);
    
    // Обновляем список ресурсов
    QString languageFilter = m_resourceLanguageComboBox->currentData().toString();
    if (languageFilter == "all") {
        languageFilter = QString();
    }
    populateResourcesList(languageFilter, m_resourceSearchEdit->text());
}

void SettingsDialog::startSegmentedDownload(const QString& url, const QString& destPath, int segmentCount)
{
    // Получаем размер файла для segmented download
    QNetworkRequest headRequest;
    headRequest.setUrl(QUrl(url));
    
    QNetworkReply* headReply = m_networkManager->head(headRequest);
    connect(headReply, &QNetworkReply::finished, [this, url, destPath, segmentCount, headReply]() {
        if (headReply->error() != QNetworkReply::NoError) {
            // Ошибка получения информации о файле, переходим к обычной загрузке
            gLogger->warning("Failed to get file size, using single-threaded download");
            downloadResource(url, destPath);
            headReply->deleteLater();
            return;
        }
        
        // Получаем размер файла
        qint64 fileSize = headReply->header(QNetworkRequest::ContentLengthHeader).toLongLong();
        headReply->deleteLater();
        
        if (fileSize <= 0) {
            // Размер неизвестен или ноль, переходим к обычной загрузке
            gLogger->warning("Unknown file size, using single-threaded download");
            downloadResource(url, destPath);
            return;
        }
        
        // Проверяем поддержку Range requests
        bool supportsRange = headReply->hasRawHeader("Accept-Ranges") && 
                            QString(headReply->rawHeader("Accept-Ranges")) != "none";
                           
        if (!supportsRange) {
            // Сервер не поддерживает частичные загрузки, переходим к обычной загрузке
            gLogger->warning("Server does not support range requests, using single-threaded download");
            downloadResource(url, destPath);
            return;
        }
        
        // Очищаем предыдущие сегменты, если они есть
        for (auto segment : m_segments) {
            if (segment->reply) {
                segment->reply->abort();
                segment->reply->deleteLater();
            }
            if (segment->tempFile) {
                segment->tempFile->close();
                delete segment->tempFile;
            }
            delete segment;
        }
        m_segments.clear();
        m_completedSegments = 0;
        
        // Вычисляем размер каждого сегмента
        qint64 segmentSize = fileSize / segmentCount;
        
        // Создаем и инициализируем структуры для каждого сегмента
        for (int i = 0; i < segmentCount; ++i) {
            DownloadSegment* segment = new DownloadSegment();
            segment->index = i;
            segment->startByte = i * segmentSize;
            segment->endByte = (i == segmentCount - 1) ? fileSize - 1 : ((i + 1) * segmentSize - 1);
            segment->bytesReceived = 0;
            segment->bytesTotal = segment->endByte - segment->startByte + 1;
            segment->completed = false;
            segment->reply = nullptr;
            
            // Создаем временный файл для сегмента
            QString tempFileName = destPath + QString(".part%1").arg(i);
            segment->tempFile = new QTemporaryFile(tempFileName);
            segment->tempFile->open();
            
            m_segments.append(segment);
        }
        
        // Запускаем загрузку всех сегментов
        for (auto segment : m_segments) {
            downloadSegment(url, destPath, segment->index, segment->startByte, segment->endByte);
        }
        
        // Отображаем информацию о многопоточной загрузке
        m_downloadStatusLabel->setText(tr("Многопоточная загрузка: %1 сегментов").arg(segmentCount));
    });
}

void SettingsDialog::downloadSegment(const QString& url, const QString& destPath, int segmentIndex, qint64 startByte, qint64 endByte)
{
    // Находим соответствующий сегмент
    if (segmentIndex >= m_segments.size()) {
        return;
    }
    
    DownloadSegment* segment = m_segments[segmentIndex];
    
    // Создаем запрос с Range header
    QNetworkRequest request;
    request.setUrl(QUrl(url));
    
    // Устанавливаем Range header
    QString rangeHeader = QString("bytes=%1-%2").arg(startByte).arg(endByte);
    request.setRawHeader("Range", rangeHeader.toUtf8());
    
    // Запускаем загрузку сегмента
    segment->reply = m_networkManager->get(request);
    
    // Сохраняем информацию о пути и индексе сегмента
    segment->reply->setProperty("segmentIndex", segmentIndex);
    segment->reply->setProperty("destinationPath", destPath);
    
    // Подключаем сигналы
    connect(segment->reply, &QNetworkReply::downloadProgress, 
            [this, segmentIndex](qint64 bytesReceived, qint64 bytesTotal) {
        
        QMutexLocker locker(&m_segmentsMutex);
        
        if (segmentIndex >= m_segments.size()) {
            return;
        }
        
        // Обновляем прогресс сегмента
        DownloadSegment* segment = m_segments[segmentIndex];
        segment->bytesReceived = bytesReceived;
        
        // Вычисляем общий прогресс всех сегментов
        qint64 totalReceived = 0;
        qint64 totalSize = 0;
        
        for (auto s : m_segments) {
            totalReceived += s->bytesReceived;
            totalSize += s->bytesTotal;
        }
        
        // Обновляем прогресс-бар
        if (totalSize > 0) {
            int percent = static_cast<int>((totalReceived * 100) / totalSize);
            m_downloadProgressBar->setValue(percent);
            
            // Рассчитываем скорость загрузки
            double elapsedSecs = m_downloadTimer.elapsed() / 1000.0;
            double speed = 0.0;
            if (elapsedSecs > 0) {
                speed = totalReceived / elapsedSecs / 1024.0; // KB/s
            }
            
            m_downloadStatusLabel->setText(tr("Многопоточная загрузка: %1 / %2 МБ (%3%) - %4 КБ/с - Сегменты: %5/%6")
                .arg(totalReceived / 1024.0 / 1024.0, 0, 'f', 1)
                .arg(totalSize / 1024.0 / 1024.0, 0, 'f', 1)
                .arg(percent)
                .arg(speed, 0, 'f', 1)
                .arg(m_completedSegments)
                .arg(m_segments.size()));
        }
    });
    
    connect(segment->reply, &QNetworkReply::finished, this, &SettingsDialog::onSegmentDownloadFinished);
}

void SettingsDialog::onSegmentDownloadFinished()
{
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) {
        return;
    }
    
    // Получаем индекс сегмента и путь назначения
    int segmentIndex = reply->property("segmentIndex").toInt();
    QString destPath = reply->property("destinationPath").toString();
    
    QMutexLocker locker(&m_segmentsMutex);
    
    if (segmentIndex >= m_segments.size()) {
        reply->deleteLater();
        return;
    }
    
    DownloadSegment* segment = m_segments[segmentIndex];
    
    if (reply->error() == QNetworkReply::NoError) {
        // Записываем полученные данные во временный файл
        segment->tempFile->write(reply->readAll());
        segment->tempFile->flush();
        
        // Помечаем сегмент как завершенный
        segment->completed = true;
        m_completedSegments++;
        
        // Если все сегменты загружены, объединяем их
        if (m_completedSegments == m_segments.size()) {
            combineSegments(destPath, m_segments.size());
        }
    } 
    else {
        // Ошибка загрузки сегмента
        gLogger->error("Segment " + std::to_string(segmentIndex) + " download failed: " + 
                     reply->errorString().toStdString());
                     
        // Пробуем перезапустить загрузку сегмента
        downloadSegment(reply->url().toString(), destPath, segmentIndex, 
                      segment->startByte, segment->endByte);
    }
    
    reply->deleteLater();
    segment->reply = nullptr;
}

void SettingsDialog::combineSegments(const QString& destPath, int segmentCount)
{
    // Создаем итоговый файл
    QFile destFile(destPath);
    if (!destFile.open(QIODevice::WriteOnly)) {
        gLogger->error("Failed to open destination file: " + destPath.toStdString());
        m_downloadStatusLabel->setText(tr("Ошибка: не удалось открыть файл назначения"));
        
        // Сбрасываем состояние загрузки
        m_isDownloading = false;
        updateDownloadControls(false);
        return;
    }
    
    // Объединяем все сегменты
    for (int i = 0; i < segmentCount; ++i) {
        if (i >= m_segments.size()) {
            break;
        }
        
        DownloadSegment* segment = m_segments[i];
        segment->tempFile->seek(0);
        
        // Копируем содержимое временного файла в итоговый
        while (!segment->tempFile->atEnd()) {
            QByteArray buffer = segment->tempFile->read(1024 * 1024); // 1MB chunks
            destFile.write(buffer);
        }
    }
    
    destFile.close();
    
    // Очищаем временные файлы и сегменты
    for (auto segment : m_segments) {
        if (segment->reply) {
            segment->reply->abort();
            segment->reply->deleteLater();
        }
        if (segment->tempFile) {
            segment->tempFile->close();
            delete segment->tempFile;
        }
        delete segment;
    }
    m_segments.clear();
    m_completedSegments = 0;
    
    // Обновляем UI
    m_downloadStatusLabel->setText(tr("Загрузка завершена"));
    m_downloadProgressBar->setValue(100);
    
    // Обновляем статус ресурса в списке
    QList<QTreeWidgetItem*> selectedItems = m_resourcesTreeWidget->selectedItems();
    if (!selectedItems.isEmpty()) {
        selectedItems.first()->setText(4, tr("Установлен"));
    }
    
    // Сбрасываем состояние загрузки
    m_isDownloading = false;
    updateDownloadControls(false);
    
    // Обновляем список ресурсов
    QString languageFilter = m_resourceLanguageComboBox->currentData().toString();
    if (languageFilter == "all") {
        languageFilter = QString();
    }
    populateResourcesList(languageFilter, m_resourceSearchEdit->text());
}

void SettingsDialog::onProxyTypeChanged(int index)
{
    setupProxy();
}

void SettingsDialog::setupProxy()
{
    if (!m_useProxyCheckBox->isChecked()) {
        return;
    }
    
    // Получаем тип прокси
    QNetworkProxy::ProxyType proxyType = static_cast<QNetworkProxy::ProxyType>(
        m_proxyTypeComboBox->currentData().toInt());
    
    // Настраиваем прокси
    m_networkProxy.setType(proxyType);
    
    if (proxyType != QNetworkProxy::DefaultProxy) {
        m_networkProxy.setHostName(m_proxyHostEdit->text());
        m_networkProxy.setPort(m_proxyPortSpinBox->value());
        
        // Устанавливаем аутентификацию, если указаны имя пользователя и пароль
        if (!m_proxyUserEdit->text().isEmpty()) {
            m_networkProxy.setUser(m_proxyUserEdit->text());
        }
        if (!m_proxyPasswordEdit->text().isEmpty()) {
            m_networkProxy.setPassword(m_proxyPasswordEdit->text());
        }
    }
    
    // Применяем прокси к сетевому менеджеру
    m_networkManager->setProxy(m_networkProxy);
    
    gLogger->info("Proxy configured: " + m_proxyHostEdit->text().toStdString() + ":" 
                 + std::to_string(m_proxyPortSpinBox->value()));
}

void SettingsDialog::updateDownloadControls(bool isDownloading, bool isPaused)
{
    m_isDownloading = isDownloading;
    m_isPaused = isPaused;
    
    // Обновляем состояние кнопок
    m_downloadButton->setEnabled(!isDownloading && !m_resourcesTreeWidget->selectedItems().isEmpty());
    m_pauseResumeButton->setEnabled(isDownloading);
    m_cancelDownloadButton->setEnabled(isDownloading);
    
    // Обновляем текст кнопки паузы/возобновления
    m_pauseResumeButton->setText(isPaused ? tr("Продолжить") : tr("Пауза"));
    
    // Показываем/скрываем прогресс загрузки
    m_downloadProgressBar->setVisible(isDownloading);
    m_downloadStatusLabel->setVisible(isDownloading);
    
    // Сбрасываем прогресс, если загрузка не активна
    if (!isDownloading) {
        m_downloadProgressBar->setValue(0);
    }
}

void SettingsDialog::downloadResource(const QString& resourceUrl, const QString& destPath)
{
    // Прерываем текущую загрузку, если она есть
    if (m_currentDownload) {
        m_currentDownload->abort();
        m_currentDownload->deleteLater();
        m_currentDownload = nullptr;
    }
    
    // Создаем запрос
    QNetworkRequest request;
    request.setUrl(QUrl(resourceUrl));
    
    // Начинаем загрузку
    m_currentDownload = m_networkManager->get(request);
    m_currentDownload->setProperty("destinationPath", destPath);
    
    // Подключаем сигналы
    connect(m_currentDownload, &QNetworkReply::downloadProgress, 
            this, &SettingsDialog::onDownloadProgress);
    connect(m_currentDownload, &QNetworkReply::finished, 
            this, &SettingsDialog::onDownloadFinished);
}

bool SettingsDialog::isResourceInstalled(const QString& resourceId)
{
    // Определяем пути и имена файлов для разных типов ресурсов
    QString appDir = QCoreApplication::applicationDirPath();
    
    // Структура данных: {ид_ресурса -> {путь, имя_файла}}
    QMap<QString, QPair<QString, QString>> resourcePaths = {
        // Модели Whisper
        {"whisper-tiny-en", {appDir + "/models/", "ggml-tiny.en.bin"}},
        {"whisper-base-en", {appDir + "/models/", "ggml-base.en.bin"}},
        {"whisper-small-en", {appDir + "/models/", "ggml-small.en.bin"}},
        {"whisper-tiny-ru", {appDir + "/models/", "ru_ggml-tiny.bin"}},
        {"whisper-base-ru", {appDir + "/models/", "ru_ggml-base.bin"}},
        
        // Модели DeepSpeech
        {"deepspeech-en", {appDir + "/models/", "deepspeech-0.9.3-models.pbmm"}},
        {"deepspeech-ru", {appDir + "/models/", "ru_deepspeech-0.9.3-models-ru.pbmm"}},
        
        // Словари
        {"dictionary-en", {appDir + "/dictionaries/", "en_words_alpha.txt"}},
        {"dictionary-ru", {appDir + "/dictionaries/", "ru_russian.txt"}},
        
        // Библиотеки
        {"deepspeech-lib-win-x64", {appDir + "/lib/", "libdeepspeech.dll"}},
        {"deepspeech-lib-linux-x64", {appDir + "/lib/", "libdeepspeech.so"}}
    };
    
    // Проверяем существование файла
    if (resourcePaths.contains(resourceId)) {
        const auto& pathInfo = resourcePaths[resourceId];
        QString fullPath = pathInfo.first + pathInfo.second;
        return QFile::exists(fullPath);
    }
    
    return false;
}

void SettingsDialog::populateResourcesList(const QString& languageFilter, const QString& searchText)
{
    // Сохраняем выбранный элемент
    QString selectedResourceId;
    QList<QTreeWidgetItem*> selectedItems = m_resourcesTreeWidget->selectedItems();
    if (!selectedItems.isEmpty()) {
        selectedResourceId = selectedItems.first()->data(0, Qt::UserRole).toString();
    }
    
    // Очищаем список
    m_resourcesTreeWidget->clear();
    
    // Формируем данные о доступных ресурсах
    struct ResourceInfo {
        QString id;         // Уникальный идентификатор
        QString name;       // Отображаемое имя
        QString type;       // Тип (модель, словарь и т.д.)
        QString language;   // Язык (en, ru, multi и т.д.)
        QString size;       // Размер в МБ
        QString url;        // URL для скачивания
    };
    
    // Список всех доступных ресурсов
    // В реальном приложении эти данные должны загружаться из сети или из локального файла конфигурации
    QList<ResourceInfo> availableResources = {
        // Модели Whisper для разных языков
        {"whisper-tiny-en", "Whisper Tiny", tr("Модель"), "en", "75 MB", 
            "https://huggingface.co/ggerganov/whisper.cpp/resolve/main/ggml-tiny.en.bin"},
        {"whisper-base-en", "Whisper Base", tr("Модель"), "en", "142 MB", 
            "https://huggingface.co/ggerganov/whisper.cpp/resolve/main/ggml-base.en.bin"},
        {"whisper-small-en", "Whisper Small", tr("Модель"), "en", "466 MB", 
            "https://huggingface.co/ggerganov/whisper.cpp/resolve/main/ggml-small.en.bin"},
        {"whisper-tiny-ru", "Whisper Tiny", tr("Модель"), "ru", "75 MB", 
            "https://huggingface.co/ggerganov/whisper.cpp/resolve/main/ggml-tiny.bin"},
        {"whisper-base-ru", "Whisper Base", tr("Модель"), "ru", "142 MB", 
            "https://huggingface.co/ggerganov/whisper.cpp/resolve/main/ggml-base.bin"},
        
        // Модели DeepSpeech для разных языков
        {"deepspeech-en", "DeepSpeech English", tr("Модель"), "en", "188 MB", 
            "https://github.com/mozilla/DeepSpeech/releases/download/v0.9.3/deepspeech-0.9.3-models.pbmm"},
        {"deepspeech-ru", "DeepSpeech Russian", tr("Модель"), "ru", "45 MB", 
            "https://github.com/alphacep/vosk-model/releases/download/small-ru/vosk-model-small-ru-0.22.zip"},
        
        // Словари
        {"dictionary-en", "English Dictionary", tr("Словарь"), "en", "5 MB", 
            "https://raw.githubusercontent.com/dwyl/english-words/master/words_alpha.txt"},
        {"dictionary-ru", "Russian Dictionary", tr("Словарь"), "ru", "7 MB", 
            "https://raw.githubusercontent.com/danakt/russian-words/master/russian.txt"},
        
        // Библиотеки
        {"deepspeech-lib-win-x64", "DeepSpeech Windows x64", tr("Библиотека"), "multi", "2.5 MB", 
            "https://github.com/mozilla/DeepSpeech/releases/download/v0.9.3/native_client.amd64.win.tar.xz"},
        {"deepspeech-lib-linux-x64", "DeepSpeech Linux x64", tr("Библиотека"), "multi", "2.8 MB", 
            "https://github.com/mozilla/DeepSpeech/releases/download/v0.9.3/native_client.amd64.linux.tar.xz"}
    };
    
    // Фильтрация списка по языку и тексту поиска
    QList<ResourceInfo> filteredResources;
    for (const ResourceInfo& info : availableResources) {
        bool matchesLanguage = languageFilter.isEmpty() || info.language == languageFilter || info.language == "multi";
        bool matchesSearch = searchText.isEmpty() || 
                             info.name.contains(searchText, Qt::CaseInsensitive) || 
                             info.type.contains(searchText, Qt::CaseInsensitive);
        
        if (matchesLanguage && matchesSearch) {
            filteredResources.append(info);
        }
    }
    
    // Заполняем список отфильтрованными ресурсами
    for (const ResourceInfo& info : filteredResources) {
        QTreeWidgetItem* item = new QTreeWidgetItem(m_resourcesTreeWidget);
        item->setText(0, info.name);
        item->setText(1, info.type);
        item->setText(2, info.language);
        item->setText(3, info.size);
        
        // Определяем статус ресурса
        if (isResourceInstalled(info.id)) {
            item->setText(4, tr("Установлен"));
        } else {
            item->setText(4, tr("Не установлен"));
        }
        
        // Сохраняем дополнительные данные
        item->setData(0, Qt::UserRole, info.id);
        item->setData(1, Qt::UserRole, info.url);
        
        // Восстанавливаем выделение
        if (info.id == selectedResourceId) {
            m_resourcesTreeWidget->setCurrentItem(item);
        }
    }
    
    // Подгоняем размер колонок
    for (int i = 0; i < m_resourcesTreeWidget->columnCount(); ++i) {
        m_resourcesTreeWidget->resizeColumnToContents(i);
    }
    
    // Обновляем состояние кнопки загрузки
    m_downloadButton->setEnabled(!m_resourcesTreeWidget->selectedItems().isEmpty() && !m_isDownloading);
}

void SettingsDialog::onPauseResumeButtonClicked()
{
    if (!m_isDownloading || !m_currentDownload) {
        return;
    }
    
    if (m_isPaused) {
        // Возобновляем загрузку
        m_isPaused = false;
        m_pauseResumeButton->setText(tr("Пауза"));
        m_downloadStatusLabel->setText(tr("Возобновление загрузки..."));
        
        // Создаем новый запрос с Range header для продолжения с позиции
        QNetworkRequest request;
        request.setUrl(QUrl(m_currentResourceUrl));
        
        // Устанавливаем Range header для продолжения с текущей позиции
        request.setRawHeader("Range", QString("bytes=%1-").arg(m_resumePosition).toUtf8());
        
        // Прерываем текущую загрузку
        if (m_currentDownload) {
            m_currentDownload->abort();
            m_currentDownload->deleteLater();
        }
        
        // Начинаем новую загрузку с указанной позиции
        m_currentDownload = m_networkManager->get(request);
        m_currentDownload->setProperty("destinationPath", m_currentDestPath);
        
        // Подключаем сигналы
        connect(m_currentDownload, &QNetworkReply::downloadProgress, 
                this, &SettingsDialog::onDownloadProgress);
        connect(m_currentDownload, &QNetworkReply::finished, 
                this, &SettingsDialog::onDownloadFinished);
                
        // Перезапускаем таймер
        m_downloadTimer.start();
    } 
    else {
        // Приостанавливаем загрузку
        m_isPaused = true;
        m_pauseResumeButton->setText(tr("Продолжить"));
        m_downloadStatusLabel->setText(tr("Загрузка приостановлена"));
        
        // Сохраняем текущую позицию для последующего возобновления
        m_resumePosition += m_currentDownload->bytesAvailable();
        
        // Приостанавливаем текущую загрузку
        if (m_currentDownload) {
            m_currentDownload->abort();
        }
    }
}

void SettingsDialog::onCancelButtonClicked()
{
    if (!m_isDownloading) {
        return;
    }
    
    // Прерываем текущую загрузку
    if (m_currentDownload) {
        m_currentDownload->abort();
        m_currentDownload->deleteLater();
        m_currentDownload = nullptr;
    }
    
    // Очищаем сегменты многопоточной загрузки
    for (auto segment : m_segments) {
        if (segment->reply) {
            segment->reply->abort();
            segment->reply->deleteLater();
        }
        if (segment->tempFile) {
            segment->tempFile->close();
            delete segment->tempFile;
        }
        delete segment;
    }
    m_segments.clear();
    m_completedSegments = 0;
    
    // Сбрасываем состояние загрузки
    m_isDownloading = false;
    m_isPaused = false;
    m_resumePosition = 0;
    m_downloadStatusLabel->setText(tr("Загрузка отменена"));
    
    // Обновляем UI
    updateDownloadControls(false);
} 