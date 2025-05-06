#include "HotkeyManager.h"
#include "../utils/Logger.h"

#include <QSettings>
#include <QApplication>
#include <QDebug>

#ifdef Q_OS_WIN
#include <Windows.h>
#endif

// External global logger
extern Logger* gLogger;

// Define the platform-specific implementation class
class HotkeyManagerImpl {
public:
    HotkeyManagerImpl() {}
    ~HotkeyManagerImpl() { clearHotkeys(); }
    
    #ifdef Q_OS_WIN
    // Windows-specific hotkey implementation
    bool registerHotkey(const QString& action, const QKeySequence& keySequence) {
        // Parse the key sequence
        Qt::KeyboardModifiers modifiers = Qt::NoModifier;
        int keyCode = 0;
        
        if (keySequence.isEmpty()) {
            return false;
        }
        
        // Extract modifiers and key from the sequence
        QKeyCombination keyCombination = keySequence[0];
        
        if (keyCombination.keyboardModifiers() & Qt::ControlModifier) {
            modifiers |= Qt::ControlModifier;
        }
        if (keyCombination.keyboardModifiers() & Qt::AltModifier) {
            modifiers |= Qt::AltModifier;
        }
        if (keyCombination.keyboardModifiers() & Qt::ShiftModifier) {
            modifiers |= Qt::ShiftModifier;
        }
        if (keyCombination.keyboardModifiers() & Qt::MetaModifier) {
            modifiers |= Qt::MetaModifier;
        }
        
        keyCode = keyCombination.key();
        
        // Convert Qt key code to Windows virtual key code
        int vk = qtKeyToVirtualKey(keyCode);
        if (vk == 0) {
            gLogger->error("Failed to convert Qt key to Windows virtual key");
            return false;
        }
        
        // Generate a unique ID for this hotkey
        static int hotkeyId = 1;
        int id = hotkeyId++;
        
        // Convert Qt modifiers to Windows modifiers
        UINT winModifiers = 0;
        if (modifiers & Qt::ControlModifier) winModifiers |= MOD_CONTROL;
        if (modifiers & Qt::AltModifier) winModifiers |= MOD_ALT;
        if (modifiers & Qt::ShiftModifier) winModifiers |= MOD_SHIFT;
        if (modifiers & Qt::MetaModifier) winModifiers |= MOD_WIN;
        
        // Register the hotkey with Windows
        if (RegisterHotKey(NULL, id, winModifiers, vk)) {
            m_hotkeyIds[action] = id;
            return true;
        } else {
            gLogger->error("Failed to register Windows hotkey. Error code: " + std::to_string(GetLastError()));
            return false;
        }
    }
    
    bool unregisterHotkey(const QString& action) {
        if (!m_hotkeyIds.contains(action)) {
            return false;
        }
        
        int id = m_hotkeyIds[action];
        
        // Unregister the hotkey with Windows
        if (UnregisterHotKey(NULL, id)) {
            m_hotkeyIds.remove(action);
            return true;
        } else {
            gLogger->error("Failed to unregister Windows hotkey. Error code: " + std::to_string(GetLastError()));
            return false;
        }
    }
    
    void clearHotkeys() {
        // Unregister all hotkeys
        for (auto it = m_hotkeyIds.begin(); it != m_hotkeyIds.end(); ++it) {
            UnregisterHotKey(NULL, it.value());
        }
        m_hotkeyIds.clear();
    }
    
    // Convert Qt key to Windows virtual key
    int qtKeyToVirtualKey(int qtKey) {
        // Handle letters and numbers
        if (qtKey >= Qt::Key_A && qtKey <= Qt::Key_Z) {
            return 'A' + (qtKey - Qt::Key_A);
        }
        
        if (qtKey >= Qt::Key_0 && qtKey <= Qt::Key_9) {
            return '0' + (qtKey - Qt::Key_0);
        }
        
        // Handle function keys
        if (qtKey >= Qt::Key_F1 && qtKey <= Qt::Key_F24) {
            return VK_F1 + (qtKey - Qt::Key_F1);
        }
        
        // Handle special keys
        switch (qtKey) {
            case Qt::Key_Space: return VK_SPACE;
            case Qt::Key_Return: return VK_RETURN;
            case Qt::Key_Tab: return VK_TAB;
            case Qt::Key_Escape: return VK_ESCAPE;
            case Qt::Key_Backspace: return VK_BACK;
            case Qt::Key_CapsLock: return VK_CAPITAL;
            case Qt::Key_Insert: return VK_INSERT;
            case Qt::Key_Delete: return VK_DELETE;
            case Qt::Key_Home: return VK_HOME;
            case Qt::Key_End: return VK_END;
            case Qt::Key_PageUp: return VK_PRIOR;
            case Qt::Key_PageDown: return VK_NEXT;
            case Qt::Key_Left: return VK_LEFT;
            case Qt::Key_Right: return VK_RIGHT;
            case Qt::Key_Up: return VK_UP;
            case Qt::Key_Down: return VK_DOWN;
            default: return 0;  // Unsupported key
        }
    }
    
private:
    QMap<QString, int> m_hotkeyIds;  // Maps actions to Windows hotkey IDs
    
    // Делаем HotkeyManager другом, чтобы он мог получить доступ к приватным полям
    friend class HotkeyManager;
    
#else
    // Dummy implementation for non-Windows platforms
    bool registerHotkey(const QString& /*action*/, const QKeySequence& /*keySequence*/) { return true; }
    bool unregisterHotkey(const QString& /*action*/) { return true; }
    void clearHotkeys() {}
    
    // Делаем HotkeyManager другом, чтобы он мог получить доступ к приватным полям
    friend class HotkeyManager;
#endif
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
    
    // Initialize platform-specific event filter for hotkeys
#ifdef Q_OS_WIN
    qApp->installNativeEventFilter(this);
#endif
    
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

#ifdef Q_OS_WIN
bool HotkeyManager::nativeEventFilter(const QByteArray& eventType, void* message, qintptr* result)
{
    Q_UNUSED(eventType)
    Q_UNUSED(result)
    
    if (!m_enabled) {
        return false;
    }

    MSG* msg = static_cast<MSG*>(message);
    
    if (msg->message == WM_HOTKEY) {
        int hotkeyId = msg->wParam;
        
        // Find the action associated with this hotkey ID
        for (auto it = m_impl->m_hotkeyIds.begin(); it != m_impl->m_hotkeyIds.end(); ++it) {
            if (it.value() == hotkeyId) {
                // Emit signal for the corresponding action
                emit hotkeyPressed(it.key());
                return true;
            }
        }
    }
    
    return false;  // Let Qt handle the event
}
#endif 