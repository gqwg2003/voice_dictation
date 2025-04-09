#pragma once

#include <QDialog>
#include <QTabWidget>
#include <QCheckBox>
#include <QSpinBox>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>
#include <QGroupBox>
#include <QKeySequenceEdit>
#include <QMap>
#include <QLineEdit>

class SettingsDialog : public QDialog {
    Q_OBJECT

public:
    explicit SettingsDialog(QWidget* parent = nullptr);
    ~SettingsDialog();

signals:
    void settingsApplied();

private slots:
    void onAccept();
    void onReject();
    void onRestoreDefaults();
    void handleRecognitionServiceChanged(int index);
    void onUsePublicApiToggled(bool checked);

private:
    // UI setup
    void setupUi();
    void createGeneralTab();
    void createHotkeysTab();
    void createAudioTab();
    void createLanguageTab();
    void createAdvancedTab();
    
    // Settings
    void loadSettings();
    void saveSettings();
    void applySettings();
    void restoreDefaults();
    
    // Hotkey handling
    void updateHotkeyLabels();
    void onHotkeyChanged();

private:
    QTabWidget* m_tabWidget;
    
    // General tab
    QCheckBox* m_startMinimizedCheckBox;
    QCheckBox* m_keepHistoryCheckBox;
    QSpinBox* m_maxHistorySizeSpinBox;
    
    // Hotkeys tab
    struct HotkeyWidgets {
        QLabel* label;
        QKeySequenceEdit* editor;
        QPushButton* resetButton;
    };
    QMap<QString, HotkeyWidgets> m_hotkeyWidgets;
    
    // Audio tab
    QComboBox* m_inputDeviceComboBox;
    QComboBox* m_sampleRateComboBox;
    QComboBox* m_channelsComboBox;
    
    // Language tab
    QCheckBox* m_autoCorrectCheckBox;
    QCheckBox* m_capitalizeFirstCheckBox;
    QCheckBox* m_addPunctuationCheckBox;
    QComboBox* m_recognitionServiceComboBox;
    QLineEdit* m_apiKeyEdit;
    QLineEdit* m_azureRegionEdit;
    QCheckBox* m_usePublicApiCheckBox;
    
    // Advanced tab
    QCheckBox* m_debugModeCheckBox;
    QSpinBox* m_recognitionThresholdSpinBox;
    QLineEdit* m_customModelPathEdit;
    QCheckBox* m_enableLoggingCheckBox;
    QComboBox* m_logLevelComboBox;
    
    // Dialog buttons
    QPushButton* m_okButton;
    QPushButton* m_cancelButton;
    QPushButton* m_applyButton;
    QPushButton* m_restoreDefaultsButton;
}; 