#pragma once

#include <QMainWindow>
#include <QTextEdit>
#include <QPushButton>
#include <QLabel>
#include <QComboBox>
#include <QStatusBar>
#include <QToolBar>
#include <QAction>
#include <QMenu>
#include <QMenuBar>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QSlider>
#include "speechrecognizer.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void startRecording();
    void stopRecording();
    void clearText();
    void saveText();
    void textRecognized(const QString& text);
    void recordingStatusChanged(bool isRecording);
    void updateMicrophoneLevel(int level);

private:
    void createUI();
    void createActions();
    void createMenus();
    void createToolbar();
    void setupConnections();

    // UI Components
    QTextEdit* m_textEdit;
    QPushButton* m_startButton;
    QPushButton* m_stopButton;
    QPushButton* m_clearButton;
    QPushButton* m_saveButton;
    QComboBox* m_languageComboBox;
    QLabel* m_statusLabel;
    QSlider* m_volumeIndicator;
    
    // Toolbar and actions
    QToolBar* m_toolbar;
    QAction* m_startAction;
    QAction* m_stopAction;
    QAction* m_clearAction;
    QAction* m_saveAction;
    QAction* m_settingsAction;
    QAction* m_exitAction;
    
    // Menus
    QMenu* m_fileMenu;
    QMenu* m_editMenu;
    QMenu* m_toolsMenu;
    QMenu* m_helpMenu;
    
    // Speech recognizer
    SpeechRecognizer* m_recognizer;
    
    // State
    bool m_isRecording;
}; 