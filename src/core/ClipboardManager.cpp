#include "ClipboardManager.h"
#include "../utils/Logger.h"

#include <QApplication>
#include <QDebug>

// External global logger
extern Logger* gLogger;

ClipboardManager::ClipboardManager(QObject* parent)
    : QObject(parent),
      m_keepHistory(true),
      m_maxHistorySize(DEFAULT_MAX_HISTORY_SIZE)
{
    // Get system clipboard
    m_clipboard = QApplication::clipboard();
    
    // Initialize last clipboard text
    m_lastClipboardText = m_clipboard->text();
    
    // Setup timer to check for clipboard changes
    m_clipboardCheckTimer.setInterval(CLIPBOARD_CHECK_INTERVAL);
    connect(&m_clipboardCheckTimer, &QTimer::timeout, this, &ClipboardManager::checkClipboardChange);
    m_clipboardCheckTimer.start();
    
    // Connect to clipboard dataChanged signal for immediate notification
    connect(m_clipboard, &QClipboard::dataChanged, this, &ClipboardManager::checkClipboardChange);
    
    gLogger->info("Clipboard manager initialized");
}

ClipboardManager::~ClipboardManager()
{
    m_clipboardCheckTimer.stop();
}

void ClipboardManager::copyToClipboard(const QString& text)
{
    if (text.isEmpty()) {
        return;
    }
    
    gLogger->info("Copying text to clipboard");
    
    // Set clipboard text
    m_clipboard->setText(text);
    
    // Add to history if enabled
    if (m_keepHistory) {
        // Remove if already in history to avoid duplicates
        m_history.removeAll(text);
        
        // Add to beginning of history
        m_history.prepend(text);
        
        // Trim history if exceeds max size
        while (m_history.size() > m_maxHistorySize) {
            m_history.removeLast();
        }
    }
    
    // Update last clipboard text
    m_lastClipboardText = text;
    
    // Emit signal
    emit textCopied(text);
}

QString ClipboardManager::getClipboardText() const
{
    return m_clipboard->text();
}

QStringList ClipboardManager::getHistory() const
{
    return m_history;
}

void ClipboardManager::clearHistory()
{
    m_history.clear();
    emit historyCleared();
}

void ClipboardManager::setKeepHistory(bool enable)
{
    m_keepHistory = enable;
    
    if (!enable) {
        clearHistory();
    }
}

bool ClipboardManager::getKeepHistory() const
{
    return m_keepHistory;
}

void ClipboardManager::setMaxHistorySize(int size)
{
    if (size > 0) {
        m_maxHistorySize = size;
        
        // Trim history if needed
        while (m_history.size() > m_maxHistorySize) {
            m_history.removeLast();
        }
    }
}

int ClipboardManager::getMaxHistorySize() const
{
    return m_maxHistorySize;
}

void ClipboardManager::checkClipboardChange()
{
    QString currentText = m_clipboard->text();
    
    // Check if clipboard text has changed
    if (currentText != m_lastClipboardText) {
        gLogger->info("Clipboard text changed");
        
        m_lastClipboardText = currentText;
        
        // Add to history if enabled and not empty
        if (m_keepHistory && !currentText.isEmpty()) {
            // Remove if already in history to avoid duplicates
            m_history.removeAll(currentText);
            
            // Add to beginning of history
            m_history.prepend(currentText);
            
            // Trim history if exceeds max size
            while (m_history.size() > m_maxHistorySize) {
                m_history.removeLast();
            }
        }
        
        // Emit signal
        emit clipboardTextChanged(currentText);
    }
} 