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

// External global logger
extern Logger* gLogger;

SettingsDialog::SettingsDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Settings"));
    //setWindowIcon(QIcon(":/Icon/app.ico"));
    
    setupUi();
    loadSettings();
    
    setMinimumSize(500, 400);
}

SettingsDialog::~SettingsDialog()
{
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
    bool needsApiKey = (service != "whisper");
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