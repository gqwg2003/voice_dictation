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

// External global logger
extern Logger* gLogger;

SettingsDialog::SettingsDialog(QWidget* parent)
    : QDialog(parent)
    , m_networkManager(nullptr)
    , m_currentDownload(nullptr)
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
    
    m_downloadButton = new QPushButton(tr("Скачать выбранный ресурс"), actionsGroup);
    m_downloadButton->setEnabled(false);
    
    m_downloadProgressBar = new QProgressBar(actionsGroup);
    m_downloadProgressBar->setRange(0, 100);
    m_downloadProgressBar->setValue(0);
    m_downloadProgressBar->setVisible(false);
    
    m_downloadStatusLabel = new QLabel(actionsGroup);
    m_downloadStatusLabel->setVisible(false);
    
    actionsLayout->addWidget(m_downloadButton);
    actionsLayout->addWidget(m_downloadProgressBar);
    actionsLayout->addWidget(m_downloadStatusLabel);
    
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
                m_downloadButton->setEnabled(!m_resourcesTreeWidget->selectedItems().isEmpty());
            });
    connect(m_downloadButton, &QPushButton::clicked, 
            this, &SettingsDialog::onDownloadButtonClicked);
            
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
    if (selectedItems.isEmpty()) {
        return;
    }
    
    QTreeWidgetItem* item = selectedItems.first();
    QString resourceId = item->data(0, Qt::UserRole).toString();
    QString resourceUrl = item->data(1, Qt::UserRole).toString();
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
    QString fileName = QFileInfo(resourceUrl).fileName();
    if (fileName.isEmpty()) {
        fileName = resourceId;
    }
    
    // Добавляем префикс языка, если ресурс языкозависимый
    if (!resourceLang.isEmpty() && resourceLang != "multi") {
        if (!fileName.contains(resourceLang)) {
            fileName = resourceLang + "_" + fileName;
        }
    }
    
    QString fullPath = destPath + fileName;
    
    // Начинаем загрузку
    downloadResource(resourceUrl, fullPath);
    
    // Обновляем UI
    m_downloadButton->setEnabled(false);
    m_downloadProgressBar->setValue(0);
    m_downloadProgressBar->setVisible(true);
    m_downloadStatusLabel->setText(tr("Загрузка %1...").arg(fileName));
    m_downloadStatusLabel->setVisible(true);
}

void SettingsDialog::onDownloadProgress(qint64 bytesReceived, qint64 bytesTotal)
{
    if (bytesTotal <= 0) {
        m_downloadProgressBar->setRange(0, 0); // Бесконечный прогресс
        return;
    }
    
    int percent = static_cast<int>((bytesReceived * 100) / bytesTotal);
    m_downloadProgressBar->setRange(0, 100);
    m_downloadProgressBar->setValue(percent);
    
    m_downloadStatusLabel->setText(tr("Загрузка: %1 / %2 МБ (%3%)")
        .arg(bytesReceived / 1024.0 / 1024.0, 0, 'f', 1)
        .arg(bytesTotal / 1024.0 / 1024.0, 0, 'f', 1)
        .arg(percent));
}

void SettingsDialog::onDownloadFinished()
{
    if (!m_currentDownload) {
        return;
    }
    
    if (m_currentDownload->error() == QNetworkReply::NoError) {
        // Успешная загрузка
        QUrl redirectUrl = m_currentDownload->attribute(QNetworkRequest::RedirectionTargetAttribute).toUrl();
        if (!redirectUrl.isEmpty()) {
            // Если это редирект, продолжаем загрузку по новому URL
            QUrl newUrl = m_currentDownload->url().resolved(redirectUrl);
            m_currentDownload->deleteLater();
            
            QNetworkRequest request(newUrl);
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
        if (file.open(QIODevice::WriteOnly)) {
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
    
    m_downloadButton->setEnabled(true);
    m_downloadProgressBar->setValue(100);
    
    // Обновляем список ресурсов
    QString languageFilter = m_resourceLanguageComboBox->currentData().toString();
    if (languageFilter == "all") {
        languageFilter = QString();
    }
    populateResourcesList(languageFilter, m_resourceSearchEdit->text());
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
    m_downloadButton->setEnabled(!m_resourcesTreeWidget->selectedItems().isEmpty());
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
    QNetworkRequest request(QUrl(resourceUrl));
    
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