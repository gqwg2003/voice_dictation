#pragma once

#include <QObject>
#include <QString>
#include <QMap>
#include <QKeySequence>
#include <QAbstractNativeEventFilter>
#include <memory>

// Forward declarations for platform-specific implementations
class HotkeyManagerImpl;

#ifdef Q_OS_WIN
class HotkeyManager : public QObject, public QAbstractNativeEventFilter {
#else
class HotkeyManager : public QObject {
#endif
    Q_OBJECT

public:
    explicit HotkeyManager(QObject* parent = nullptr);
    ~HotkeyManager();

    // Register a hotkey
    bool registerHotkey(const QString& action, const QKeySequence& keySequence);
    
    // Unregister a hotkey
    bool unregisterHotkey(const QString& action);
    
    // Get registered hotkeys
    QMap<QString, QKeySequence> getHotkeys() const;
    
    // Get hotkey for action
    QKeySequence getHotkey(const QString& action) const;
    
    // Check if action is registered
    bool hasAction(const QString& action) const;
    
    // Check if hotkey is registered
    bool hasHotkey(const QKeySequence& keySequence) const;
    
    // Clear all hotkeys
    void clearHotkeys();
    
    // Reload hotkeys from settings
    void reloadHotkeys();
    
    // Temporarily enable/disable hotkeys
    void setEnabled(bool enabled);
    bool isEnabled() const;

#ifdef Q_OS_WIN
    // Windows native event filter (from QAbstractNativeEventFilter)
    bool nativeEventFilter(const QByteArray& eventType, void* message, qintptr* result) override;
#endif

signals:
    void hotkeyPressed(const QString& action);
    void hotkeyRegistered(const QString& action, const QKeySequence& keySequence);
    void hotkeyUnregistered(const QString& action);
    void hotkeysCleared();

private:
    // Platform-specific implementation
    std::unique_ptr<HotkeyManagerImpl> m_impl;
    
    // Registered hotkeys
    QMap<QString, QKeySequence> m_hotkeys;
    
    // State
    bool m_enabled;
    
    // Load default hotkeys
    void loadDefaultHotkeys();

    // Make m_impl accessible to nativeEventFilter
    friend class HotkeyManagerImpl;
}; 