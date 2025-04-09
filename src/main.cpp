/**
 * Voice Dictation Application
 * 
 * This application allows you to dictate text using voice recognition and
 * easily copy it to the clipboard for use in games, social media, etc.
 */

#include <QApplication>
#include <QStyle>
#include <QFontDatabase>
#include <QStyleFactory>
#include <QThread>
#include <QMessageBox>
#include <QDir>
#include <QCommandLineParser>
#include <iostream>
#include <signal.h>
#include <fstream>
#include <QTranslator>
#include <QSettings>
#include <QLocale>
#include <QStandardPaths>
#include <QDateTime>

#include "gui/MainWindow.h"
#include "utils/Logger.h"
#include "utils/Version.h"

// Global variables
Logger* gLogger = nullptr;

// Initialize logger
bool initializeLogger() {
    // Create logs directory in app data location if it doesn't exist
    QString appDataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir appDataDir(appDataPath);
    if (!appDataDir.exists()) {
        appDataDir.mkpath(".");
    }
    
    QString logsPath = appDataPath + "/logs";
    QDir logsDir(logsPath);
    if (!logsDir.exists()) {
        logsDir.mkpath(".");
    }
    
    // Create log file name with current date/time
    QString logFileName = logsPath + "/voice_dictation_" + 
                          QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss") + ".log";
    
    // Initialize global logger
    gLogger = new Logger();
    
    // Apply logging settings
    QSettings settings;
    bool enableLogging = settings.value("advanced/enable_logging", false).toBool();
    QString logLevelStr = settings.value("advanced/log_level", "info").toString();
    
    LogLevel logLevel = LogLevel::Info;
    if (logLevelStr == "error") logLevel = LogLevel::Error;
    else if (logLevelStr == "warning") logLevel = LogLevel::Warning;
    else if (logLevelStr == "info") logLevel = LogLevel::Info;
    else if (logLevelStr == "debug") logLevel = LogLevel::Debug;
    else if (logLevelStr == "verbose") logLevel = LogLevel::Verbose;
    
    gLogger->setLogLevel(logLevel);
    gLogger->setEnableLogging(enableLogging);
    
    // Initialize logger with log file
    bool success = gLogger->init(logFileName.toStdString());
    if (success) {
        gLogger->info("Logger initialized successfully");
    } else {
        gLogger->error("Failed to initialize logger");
    }
    
    return success;
}

// Cleanup logger
void cleanupLogger() {
    if (gLogger) {
        gLogger->info("Application shutting down");
        delete gLogger;
        gLogger = nullptr;
    }
}

// Signal handler function for graceful shutdown
void signalHandler(int signal) {
    if (gLogger) {
        gLogger->info("Received signal " + std::to_string(signal) + ", exiting gracefully");
    }
    QApplication::quit();
}

// Debug file output function
void writeDebugToFile(const std::string& message) {
    std::ofstream debugFile("debug_startup.log", std::ios::app);
    if (debugFile.is_open()) {
        debugFile << message << std::endl;
        debugFile.close();
    }
}

// Main application entry point
int main(int argc, char *argv[]) {
    try {
        // Write debug information to file
        writeDebugToFile("Application starting...");
        writeDebugToFile("Current directory: " + QDir::currentPath().toStdString());
        
        // Initialize version information
        std::cout << "Updating version information..." << std::endl;
        writeDebugToFile("Updating version information...");
        Version version;
        auto versionData = version.forceUpdateVersion();
        std::cout << "Version: " << versionData.displayVersion << std::endl;
        std::cout << "Build Number: " << versionData.buildNumber << std::endl;
        writeDebugToFile("Version: " + versionData.displayVersion);
        writeDebugToFile("Build Number: " + versionData.buildNumber);
        
        // Create Qt application
        writeDebugToFile("Creating QApplication...");
        QApplication app(argc, argv);
        app.setApplicationName("Voice Dictation");
        app.setApplicationVersion(QString::fromStdString(versionData.displayVersion));
        app.setStyle(QStyleFactory::create("Fusion"));  // Using Fusion style for a more professional look
        writeDebugToFile("QApplication created");

        // Set up command line parser
        QCommandLineParser parser;
        parser.setApplicationDescription("Voice Dictation Application");
        parser.addHelpOption();
        parser.addVersionOption();
        
        // Add show-window option
        QCommandLineOption showWindowOption("show-window", "Show the main window on startup instead of minimizing to tray");
        parser.addOption(showWindowOption);
        
        // Process command line arguments
        parser.process(app);
        bool showWindow = parser.isSet(showWindowOption);
        writeDebugToFile("Show window flag: " + std::string(showWindow ? "true" : "false"));
        
        // Setup logging
        writeDebugToFile("Setting up logger...");
        bool loggerInitialized = initializeLogger();
        gLogger->info("Starting Voice Dictation Application v" + versionData.displayVersion);
        writeDebugToFile("Logger initialized");
        
        // Get optimal performance mode
        // TODO: Replace with actual implementation
        std::string hybridMode = "balanced";
        int performanceLevel = 2;
        gLogger->info("Running in " + hybridMode + " mode with performance level " + std::to_string(performanceLevel));
        
        // Register signal handlers for graceful shutdown
        #ifdef _WIN32
        signal(SIGINT, signalHandler);
        signal(SIGTERM, signalHandler);
        #else
        struct sigaction sa;
        sa.sa_handler = signalHandler;
        sigemptyset(&sa.sa_mask);
        sa.sa_flags = 0;
        sigaction(SIGINT, &sa, NULL);
        sigaction(SIGTERM, &sa, NULL);
        #endif
        
        // Load translator based on system locale or user preference
        QSettings settings;
        QString language = settings.value("language/current", QLocale::system().name()).toString();
        
        QTranslator translator;
        if (language != "en-US" && language != "en") {
            QString qmFile = ":/translations/voicedictation_" + language.left(2) + ".qm";
            if (translator.load(qmFile)) {
                app.installTranslator(&translator);
                gLogger->info("Loaded translation: " + qmFile.toStdString());
            } else {
                gLogger->warning("Failed to load translation: " + qmFile.toStdString());
            }
        }
        
        // Create main window
        writeDebugToFile("Creating main window...");
        MainWindow mainWindow;
        
        // Show main window if requested
        if (showWindow) {
            writeDebugToFile("Showing main window because of command line flag...");
            mainWindow.show();
            mainWindow.activateWindow();
            mainWindow.raise();
            writeDebugToFile("Main window shown and activated");
        } else {
            writeDebugToFile("Not showing main window initially (will be minimized to tray)");
        }
        
        // Start application event loop
        writeDebugToFile("Entering Qt event loop...");
        int result = app.exec();
        writeDebugToFile("Qt event loop exited with code: " + std::to_string(result));
        
        // Cleanup
        cleanupLogger();
        
        return result;
    } catch (const std::exception& e) {
        writeDebugToFile("EXCEPTION: " + std::string(e.what()));
        std::cerr << "EXCEPTION: " << e.what() << std::endl;
        
        // Show error message box if we can
        try {
            QMessageBox::critical(nullptr, "Error Starting Application", 
                QString("An error occurred while starting the application: %1").arg(e.what()));
        } catch (...) {
            // Ignore errors showing the message box
        }
        
        return 1;
    } catch (...) {
        writeDebugToFile("UNKNOWN EXCEPTION occurred");
        std::cerr << "UNKNOWN EXCEPTION occurred" << std::endl;
        return 1;
    }
} 