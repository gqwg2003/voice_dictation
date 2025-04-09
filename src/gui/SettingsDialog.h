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

private:
    // UI setup
    void setupUi();
    void createGeneralTab();
    void createHotkeysTab();
    void createAudioTab();
    void createLanguageTab();
    
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
    
    // Dialog buttons
    QPushButton* m_okButton;
    QPushButton* m_cancelButton;
    QPushButton* m_applyButton;
    QPushButton* m_restoreDefaultsButton;
}; 