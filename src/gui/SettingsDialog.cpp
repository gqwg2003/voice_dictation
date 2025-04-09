#include "SettingsDialog.h"
#include "../utils/Logger.h"
#include "../core/HotkeyManager.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QDialogButtonBox>
#include <QSettings>
#include <QMessageBox>
#include <QMediaDevices>
#include <QAudioDevice>

// External global logger
extern Logger* gLogger;

SettingsDialog::SettingsDialog(QWidget* parent)
    : QDialog(parent)
{
    setupUi();
    loadSettings();
    
    setWindowTitle(tr("Settings"));
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
    
    // Text processing settings
    QGroupBox* textGroup = new QGroupBox(tr("Text Processing"), tab);
    QVBoxLayout* textLayout = new QVBoxLayout(textGroup);
    
    m_autoCorrectCheckBox = new QCheckBox(tr("Auto-correct common spelling errors"), textGroup);
    m_capitalizeFirstCheckBox = new QCheckBox(tr("Capitalize first letter of sentences"), textGroup);
    m_addPunctuationCheckBox = new QCheckBox(tr("Add punctuation marks if missing"), textGroup);
    
    textLayout->addWidget(m_autoCorrectCheckBox);
    textLayout->addWidget(m_capitalizeFirstCheckBox);
    textLayout->addWidget(m_addPunctuationCheckBox);
    
    // Add groups to tab
    layout->addWidget(textGroup);
    layout->addStretch();
    
    // Add tab to tab widget
    m_tabWidget->addTab(tab, tr("Language"));
}

void SettingsDialog::loadSettings()
{
    QSettings settings;
    
    // General settings
    m_startMinimizedCheckBox->setChecked(settings.value("general/start_minimized", false).toBool());
    m_keepHistoryCheckBox->setChecked(settings.value("general/keep_history", true).toBool());
    m_maxHistorySizeSpinBox->setValue(settings.value("general/max_history_size", 20).toInt());
    
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
    m_autoCorrectCheckBox->setChecked(settings.value("language/auto_correct", true).toBool());
    m_capitalizeFirstCheckBox->setChecked(settings.value("language/capitalize_first", true).toBool());
    m_addPunctuationCheckBox->setChecked(settings.value("language/add_punctuation", true).toBool());
}

void SettingsDialog::saveSettings()
{
    QSettings settings;
    
    // General settings
    settings.setValue("general/start_minimized", m_startMinimizedCheckBox->isChecked());
    settings.setValue("general/keep_history", m_keepHistoryCheckBox->isChecked());
    settings.setValue("general/max_history_size", m_maxHistorySizeSpinBox->value());
    
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
    settings.setValue("language/auto_correct", m_autoCorrectCheckBox->isChecked());
    settings.setValue("language/capitalize_first", m_capitalizeFirstCheckBox->isChecked());
    settings.setValue("language/add_punctuation", m_addPunctuationCheckBox->isChecked());
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