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
#include <QTreeWidget>
#include <QProgressBar>
#include <QListWidget>
#include <QNetworkAccessManager>
#include <QNetworkReply>

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
    void onLanguageFilterChanged(int index);
    void onResourceSearchTextChanged(const QString& text);
    void onDownloadButtonClicked();
    void onDownloadProgress(qint64 bytesReceived, qint64 bytesTotal);
    void onDownloadFinished();

private:
    // UI setup
    void setupUi();
    void createGeneralTab();
    void createHotkeysTab();
    void createAudioTab();
    void createLanguageTab();
    void createAdvancedTab();
    void createResourcesTab();
    
    // Settings
    void loadSettings();
    void saveSettings();
    void applySettings();
    void restoreDefaults();
    
    // Hotkey handling
    void updateHotkeyLabels();
    void onHotkeyChanged();
    
    // Resource management
    void populateResourcesList(const QString& languageFilter = QString(), const QString& searchText = QString());
    void downloadResource(const QString& resourceId, const QString& destPath);
    bool isResourceInstalled(const QString& resourceId);

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
    
    // Resources tab
    QComboBox* m_resourceLanguageComboBox;
    QLineEdit* m_resourceSearchEdit;
    QTreeWidget* m_resourcesTreeWidget;
    QPushButton* m_downloadButton;
    QProgressBar* m_downloadProgressBar;
    QLabel* m_downloadStatusLabel;
    
    // Network
    QNetworkAccessManager* m_networkManager;
    QNetworkReply* m_currentDownload;
    
    // Dialog buttons
    QPushButton* m_okButton;
    QPushButton* m_cancelButton;
    QPushButton* m_applyButton;
    QPushButton* m_restoreDefaultsButton;
}; 