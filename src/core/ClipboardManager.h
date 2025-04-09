#pragma once

#include <QObject>
#include <QString>
#include <QClipboard>
#include <QTimer>
#include <QStringList>

class ClipboardManager : public QObject {
    Q_OBJECT

public:
    explicit ClipboardManager(QObject* parent = nullptr);
    ~ClipboardManager();

    // Copy text to clipboard
    void copyToClipboard(const QString& text);
    
    // Get clipboard text
    QString getClipboardText() const;
    
    // History management
    QStringList getHistory() const;
    void clearHistory();
    
    // Configuration
    void setKeepHistory(bool enable);
    bool getKeepHistory() const;
    
    void setMaxHistorySize(int size);
    int getMaxHistorySize() const;

signals:
    void clipboardTextChanged(const QString& text);
    void textCopied(const QString& text);
    void historyCleared();

private slots:
    void checkClipboardChange();

private:
    QClipboard* m_clipboard;
    QStringList m_history;
    QTimer m_clipboardCheckTimer;
    QString m_lastClipboardText;
    bool m_keepHistory;
    int m_maxHistorySize;
    
    static const int DEFAULT_MAX_HISTORY_SIZE = 20;
    static const int CLIPBOARD_CHECK_INTERVAL = 500; // ms
}; 