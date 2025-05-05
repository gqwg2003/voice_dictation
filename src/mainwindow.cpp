#include "mainwindow.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QIcon>
#include <QApplication>
#include <QSettings>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_isRecording(false)
{
    setWindowTitle(tr("Голосовая диктовка"));
    resize(800, 600);
    
    createUI();
    createActions();
    createMenus();
    createToolbar();
    setupConnections();
    
    // Создаем распознаватель речи
    m_recognizer = new SpeechRecognizer(this);
    
    // Подключаем сигналы распознавателя к слотам
    connect(m_recognizer, &SpeechRecognizer::textRecognized, 
            this, &MainWindow::textRecognized);
    connect(m_recognizer, &SpeechRecognizer::recordingStatusChanged, 
            this, &MainWindow::recordingStatusChanged);
    connect(m_recognizer, &SpeechRecognizer::microphoneLevelChanged, 
            this, &MainWindow::updateMicrophoneLevel);
    
    // Устанавливаем начальное состояние
    recordingStatusChanged(false);
    
    statusBar()->showMessage(tr("Готов к работе"));
}

MainWindow::~MainWindow()
{
    // Если запись все еще идет, останавливаем ее
    if (m_isRecording) {
        m_recognizer->stopRecording();
    }
}

void MainWindow::createUI()
{
    // Создаем центральный виджет
    QWidget* centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);
    
    // Основной вертикальный лейаут
    QVBoxLayout* mainLayout = new QVBoxLayout(centralWidget);
    
    // Группа для кнопок управления
    QGroupBox* controlsGroupBox = new QGroupBox(tr("Управление"), this);
    QHBoxLayout* controlsLayout = new QHBoxLayout(controlsGroupBox);
    
    // Кнопки управления
    m_startButton = new QPushButton(tr("Начать диктовку"), this);
    // Временно отключаем иконки
    //m_startButton->setIcon(QIcon::fromTheme("media-record"));
    
    m_stopButton = new QPushButton(tr("Остановить"), this);
    //m_stopButton->setIcon(QIcon::fromTheme("media-playback-stop"));
    
    m_clearButton = new QPushButton(tr("Очистить"), this);
    //m_clearButton->setIcon(QIcon::fromTheme("edit-clear"));
    
    m_saveButton = new QPushButton(tr("Сохранить"), this);
    //m_saveButton->setIcon(QIcon::fromTheme("document-save"));
    
    // Выбор языка
    QLabel* languageLabel = new QLabel(tr("Язык:"), this);
    m_languageComboBox = new QComboBox(this);
    m_languageComboBox->addItem(tr("Русский"), "ru-RU");
    m_languageComboBox->addItem(tr("Английский"), "en-US");
    
    // Индикатор громкости микрофона
    QLabel* volumeLabel = new QLabel(tr("Громкость:"), this);
    m_volumeIndicator = new QSlider(Qt::Horizontal, this);
    m_volumeIndicator->setRange(0, 100);
    m_volumeIndicator->setValue(0);
    m_volumeIndicator->setEnabled(false); // Только для отображения
    
    // Добавляем элементы в лейаут управления
    controlsLayout->addWidget(m_startButton);
    controlsLayout->addWidget(m_stopButton);
    controlsLayout->addWidget(m_clearButton);
    controlsLayout->addWidget(m_saveButton);
    controlsLayout->addSpacing(20);
    controlsLayout->addWidget(languageLabel);
    controlsLayout->addWidget(m_languageComboBox);
    controlsLayout->addSpacing(20);
    controlsLayout->addWidget(volumeLabel);
    controlsLayout->addWidget(m_volumeIndicator);
    
    // Текстовый редактор
    m_textEdit = new QTextEdit(this);
    m_textEdit->setPlaceholderText(tr("Здесь появится распознанный текст..."));
    
    // Статус
    m_statusLabel = new QLabel(tr("Статус: Ожидание"), this);
    
    // Добавляем все виджеты в основной лейаут
    mainLayout->addWidget(controlsGroupBox);
    mainLayout->addWidget(m_textEdit, 1); // Растягиваем по вертикали
    mainLayout->addWidget(m_statusLabel);
}

void MainWindow::createActions()
{
    m_startAction = new QAction(tr("Начать диктовку"), this);
    //m_startAction->setIcon(QIcon::fromTheme("media-record"));
    m_startAction->setShortcut(QKeySequence("Ctrl+R"));
    
    m_stopAction = new QAction(tr("Остановить"), this);
    //m_stopAction->setIcon(QIcon::fromTheme("media-playback-stop"));
    m_stopAction->setShortcut(QKeySequence("Ctrl+S"));
    
    m_clearAction = new QAction(tr("Очистить"), this);
    //m_clearAction->setIcon(QIcon::fromTheme("edit-clear"));
    m_clearAction->setShortcut(QKeySequence("Ctrl+L"));
    
    m_saveAction = new QAction(tr("Сохранить в файл"), this);
    //m_saveAction->setIcon(QIcon::fromTheme("document-save"));
    m_saveAction->setShortcut(QKeySequence("Ctrl+S"));
    
    m_settingsAction = new QAction(tr("Настройки"), this);
    //m_settingsAction->setIcon(QIcon::fromTheme("preferences-system"));
    
    m_exitAction = new QAction(tr("Выход"), this);
    //m_exitAction->setIcon(QIcon::fromTheme("application-exit"));
    m_exitAction->setShortcut(QKeySequence("Alt+F4"));
}

void MainWindow::createMenus()
{
    m_fileMenu = menuBar()->addMenu(tr("Файл"));
    m_fileMenu->addAction(m_saveAction);
    m_fileMenu->addSeparator();
    m_fileMenu->addAction(m_exitAction);
    
    m_editMenu = menuBar()->addMenu(tr("Правка"));
    m_editMenu->addAction(m_clearAction);
    
    m_toolsMenu = menuBar()->addMenu(tr("Инструменты"));
    m_toolsMenu->addAction(m_startAction);
    m_toolsMenu->addAction(m_stopAction);
    m_toolsMenu->addSeparator();
    m_toolsMenu->addAction(m_settingsAction);
    
    m_helpMenu = menuBar()->addMenu(tr("Справка"));
    m_helpMenu->addAction(tr("О программе"));
}

void MainWindow::createToolbar()
{
    m_toolbar = addToolBar(tr("Панель инструментов"));
    m_toolbar->addAction(m_startAction);
    m_toolbar->addAction(m_stopAction);
    m_toolbar->addAction(m_clearAction);
    m_toolbar->addAction(m_saveAction);
    m_toolbar->addSeparator();
    m_toolbar->addAction(m_settingsAction);
}

void MainWindow::setupConnections()
{
    // Подключаем кнопки к слотам
    connect(m_startButton, &QPushButton::clicked, this, &MainWindow::startRecording);
    connect(m_stopButton, &QPushButton::clicked, this, &MainWindow::stopRecording);
    connect(m_clearButton, &QPushButton::clicked, this, &MainWindow::clearText);
    connect(m_saveButton, &QPushButton::clicked, this, &MainWindow::saveText);
    
    // Подключаем действия к слотам
    connect(m_startAction, &QAction::triggered, this, &MainWindow::startRecording);
    connect(m_stopAction, &QAction::triggered, this, &MainWindow::stopRecording);
    connect(m_clearAction, &QAction::triggered, this, &MainWindow::clearText);
    connect(m_saveAction, &QAction::triggered, this, &MainWindow::saveText);
    connect(m_exitAction, &QAction::triggered, this, &QWidget::close);
}

void MainWindow::startRecording()
{
    if (!m_isRecording) {
        QString language = m_languageComboBox->currentData().toString();
        if (m_recognizer->startRecording(language)) {
            m_statusLabel->setText(tr("Статус: Идет запись..."));
            m_isRecording = true;
        } else {
            QMessageBox::warning(this, tr("Ошибка"), 
                                tr("Не удалось начать запись. Проверьте микрофон."));
        }
    }
}

void MainWindow::stopRecording()
{
    if (m_isRecording) {
        m_recognizer->stopRecording();
        m_statusLabel->setText(tr("Статус: Ожидание"));
        m_isRecording = false;
    }
}

void MainWindow::clearText()
{
    m_textEdit->clear();
}

void MainWindow::saveText()
{
    QString fileName = QFileDialog::getSaveFileName(this,
        tr("Сохранить текст"), QDir::homePath(), tr("Текстовые файлы (*.txt)"));
    
    if (!fileName.isEmpty()) {
        QFile file(fileName);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream stream(&file);
            stream << m_textEdit->toPlainText();
            file.close();
            statusBar()->showMessage(tr("Файл сохранен"), 3000);
        } else {
            QMessageBox::warning(this, tr("Ошибка"), 
                                tr("Не удалось сохранить файл."));
        }
    }
}

void MainWindow::textRecognized(const QString& text)
{
    // Добавляем распознанный текст в конец текущего 
    m_textEdit->append(text);
}

void MainWindow::recordingStatusChanged(bool isRecording)
{
    m_isRecording = isRecording;
    m_startButton->setEnabled(!isRecording);
    m_stopButton->setEnabled(isRecording);
    m_startAction->setEnabled(!isRecording);
    m_stopAction->setEnabled(isRecording);
    
    if (isRecording) {
        m_statusLabel->setText(tr("Статус: Идет запись..."));
    } else {
        m_statusLabel->setText(tr("Статус: Ожидание"));
    }
}

void MainWindow::updateMicrophoneLevel(int level)
{
    m_volumeIndicator->setValue(level);
} 