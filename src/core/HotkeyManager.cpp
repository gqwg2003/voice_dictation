#include "HotkeyManager.h"
#include "../utils/Logger.h"

#include <QSettings>
#include <QApplication>
#include <QDebug>

// External global logger
extern Logger* gLogger;

// Define the platform-specific implementation class
class HotkeyManagerImpl {
public:
    HotkeyManagerImpl() {}
    ~HotkeyManagerImpl() {}
    
    // Add any platform-specific methods here
    bool registerHotkey(const QString& /*action*/, const QKeySequence& /*keySequence*/) { return true; }
    bool unregisterHotkey(const QString& /*action*/) { return true; }
    void clearHotkeys() {}
};

// Simplified implementation without actual hotkey registration
// In a real implementation, this would be platform-specific code

HotkeyManager::HotkeyManager(QObject* parent)
    : QObject(parent),
      m_impl(std::make_unique<HotkeyManagerImpl>()),
      m_enabled(true)
{
    gLogger->info("Initializing hotkey manager");
    
    // Load default hotkeys
    loadDefaultHotkeys();
    
    // TODO: Initialize platform-specific implementation
    // For this simplified version, we don't actually register global hotkeys
    
    gLogger->info("Hotkey manager initialized");
}

HotkeyManager::~HotkeyManager()
{
    clearHotkeys();
}

bool HotkeyManager::registerHotkey(const QString& action, const QKeySequence& keySequence)
{
    if (!m_enabled || action.isEmpty() || keySequence.isEmpty()) {
        return false;
    }
    
    // Check if hotkey is already registered for another action
    if (hasHotkey(keySequence)) {
        gLogger->warning("Hotkey already registered: " + keySequence.toString().toStdString());
        return false;
    }
    
    // Unregister previous hotkey for this action if exists
    if (hasAction(action)) {
        unregisterHotkey(action);
    }
    
    gLogger->info("Registering hotkey " + keySequence.toString().toStdString() + " for action: " + action.toStdString());
    
    // In a real implementation, we would register the hotkey with the system here
    // For this simplified version, we just store it in our map
    m_hotkeys[action] = keySequence;
    
    // Save to settings
    QSettings settings;
    settings.setValue("hotkeys/" + action, keySequence.toString());
    
    emit hotkeyRegistered(action, keySequence);
    
    return true;
}

bool HotkeyManager::unregisterHotkey(const QString& action)
{
    if (!hasAction(action)) {
        return false;
    }
    
    gLogger->info("Unregistering hotkey for action: " + action.toStdString());
    
    // In a real implementation, we would unregister the hotkey from the system here
    // For this simplified version, we just remove it from our map
    m_hotkeys.remove(action);
    
    // Remove from settings
    QSettings settings;
    settings.remove("hotkeys/" + action);
    
    emit hotkeyUnregistered(action);
    
    return true;
}

QMap<QString, QKeySequence> HotkeyManager::getHotkeys() const
{
    return m_hotkeys;
}

QKeySequence HotkeyManager::getHotkey(const QString& action) const
{
    return m_hotkeys.value(action);
}

bool HotkeyManager::hasAction(const QString& action) const
{
    return m_hotkeys.contains(action);
}

bool HotkeyManager::hasHotkey(const QKeySequence& keySequence) const
{
    return m_hotkeys.values().contains(keySequence);
}

void HotkeyManager::clearHotkeys()
{
    gLogger->info("Clearing all hotkeys");
    
    // In a real implementation, we would unregister all hotkeys from the system here
    // For this simplified version, we just clear our map
    m_hotkeys.clear();
    
    // Clear settings
    QSettings settings;
    settings.beginGroup("hotkeys");
    settings.remove("");
    settings.endGroup();
    
    emit hotkeysCleared();
}

void HotkeyManager::reloadHotkeys()
{
    gLogger->info("Reloading hotkeys from settings");
    
    // Clear current hotkeys
    clearHotkeys();
    
    // Load from settings
    QSettings settings;
    settings.beginGroup("hotkeys");
    
    QStringList keys = settings.childKeys();
    for (const QString& action : keys) {
        QString keyStr = settings.value(action).toString();
        QKeySequence keySequence(keyStr);
        
        if (!keySequence.isEmpty()) {
            // Don't emit signals during reload
            m_hotkeys[action] = keySequence;
        }
    }
    
    settings.endGroup();
    
    // If no hotkeys were loaded, use defaults
    if (m_hotkeys.isEmpty()) {
        loadDefaultHotkeys();
    }
}

void HotkeyManager::setEnabled(bool enabled)
{
    m_enabled = enabled;
}

bool HotkeyManager::isEnabled() const
{
    return m_enabled;
}

void HotkeyManager::loadDefaultHotkeys()
{
    gLogger->info("Loading default hotkeys");
    
    // Default hotkeys
    registerHotkey("record", QKeySequence("Ctrl+Alt+R"));
    registerHotkey("copy", QKeySequence("Ctrl+Alt+C"));
    registerHotkey("clear", QKeySequence("Ctrl+Alt+X"));
}

// For testing purposes, simulate a hotkey press
// In a real app, this would be triggered by the platform-specific implementation
void simulateHotkeyPress(HotkeyManager* manager, const QString& action)
{
    if (manager && manager->hasAction(action) && manager->isEnabled()) {
        emit manager->hotkeyPressed(action);
    }
} 