#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/functional.h>
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>

#ifdef _WIN32
#include <windows.h>
#endif

namespace py = pybind11;

// Глобальный счетчик для уникальных ID горячих клавиш
static int g_hotkey_counter = 0;

// Хранилище функций обратного вызова для горячих клавиш
static std::unordered_map<int, std::function<void()>> g_hotkey_callbacks;

class HotkeyManager {
private:
    struct Hotkey {
        int id;                  // Уникальный ID горячей клавиши
        std::string key_combo;   // Комбинация клавиш в текстовом формате
        int key_code;            // Код клавиши
        int modifiers;           // Модификаторы (Ctrl, Alt, Shift)
    };

    std::vector<Hotkey> registered_hotkeys;
    bool active;

#ifdef _WIN32
    // Преобразование строкового представления горячей клавиши в коды клавиш
    std::pair<int, int> parse_key_combo(const std::string& key_combo) {
        int key_code = 0;
        int modifiers = 0;

        // Парсим модификаторы
        if (key_combo.find("ctrl") != std::string::npos) 
            modifiers |= MOD_CONTROL;
        if (key_combo.find("alt") != std::string::npos) 
            modifiers |= MOD_ALT;
        if (key_combo.find("shift") != std::string::npos) 
            modifiers |= MOD_SHIFT;
        if (key_combo.find("win") != std::string::npos) 
            modifiers |= MOD_WIN;

        // Найдем основную клавишу (последний +)
        size_t last_plus = key_combo.find_last_of("+");
        std::string main_key;
        
        if (last_plus != std::string::npos && last_plus < key_combo.size() - 1) {
            main_key = key_combo.substr(last_plus + 1);
        } else {
            main_key = key_combo;
        }

        // Удаляем пробелы
        main_key.erase(remove_if(main_key.begin(), main_key.end(), isspace), main_key.end());

        // Преобразование строковых клавиш в коды Virtual Key
        if (main_key.size() == 1 && isalpha(main_key[0])) {
            // Для букв A-Z
            key_code = toupper(main_key[0]);
        } else if (main_key.size() == 1 && isdigit(main_key[0])) {
            // Для цифр 0-9
            key_code = main_key[0];
        } else if (main_key == "f1") key_code = VK_F1;
        else if (main_key == "f2") key_code = VK_F2;
        else if (main_key == "f3") key_code = VK_F3;
        else if (main_key == "f4") key_code = VK_F4;
        else if (main_key == "f5") key_code = VK_F5;
        else if (main_key == "f6") key_code = VK_F6;
        else if (main_key == "f7") key_code = VK_F7;
        else if (main_key == "f8") key_code = VK_F8;
        else if (main_key == "f9") key_code = VK_F9;
        else if (main_key == "f10") key_code = VK_F10;
        else if (main_key == "f11") key_code = VK_F11;
        else if (main_key == "f12") key_code = VK_F12;
        else if (main_key == "space") key_code = VK_SPACE;
        else if (main_key == "enter") key_code = VK_RETURN;
        else if (main_key == "tab") key_code = VK_TAB;
        else if (main_key == "escape" || main_key == "esc") key_code = VK_ESCAPE;
        else if (main_key == "backspace") key_code = VK_BACK;
        else if (main_key == "insert") key_code = VK_INSERT;
        else if (main_key == "delete") key_code = VK_DELETE;
        else if (main_key == "home") key_code = VK_HOME;
        else if (main_key == "end") key_code = VK_END;
        else if (main_key == "pageup") key_code = VK_PRIOR;
        else if (main_key == "pagedown") key_code = VK_NEXT;
        else if (main_key == "up") key_code = VK_UP;
        else if (main_key == "down") key_code = VK_DOWN;
        else if (main_key == "left") key_code = VK_LEFT;
        else if (main_key == "right") key_code = VK_RIGHT;
        else if (main_key == "printscreen") key_code = VK_SNAPSHOT;
        else if (main_key == "scrolllock") key_code = VK_SCROLL;
        else if (main_key == "pause") key_code = VK_PAUSE;
        else if (main_key == "numlock") key_code = VK_NUMLOCK;

        return {key_code, modifiers};
    }
#else
    // Заглушка для других ОС
    std::pair<int, int> parse_key_combo(const std::string& key_combo) {
        return {0, 0};
    }
#endif

public:
    HotkeyManager() : active(false) {}

    ~HotkeyManager() {
        unregister_all_hotkeys();
    }

    // Обработчик горячих клавиш для Windows
#ifdef _WIN32
    static LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
        if (nCode == HC_ACTION && (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN)) {
            KBDLLHOOKSTRUCT* kbStruct = reinterpret_cast<KBDLLHOOKSTRUCT*>(lParam);
            // Обработка события клавиатуры...
        }
        return CallNextHookEx(NULL, nCode, wParam, lParam);
    }
#endif

    // Регистрация горячей клавиши
    bool register_hotkey(const std::string& key_combo, const std::function<void()>& callback) {
#ifdef _WIN32
        auto [key_code, modifiers] = parse_key_combo(key_combo);
        if (key_code == 0) {
            return false;
        }

        int id = ++g_hotkey_counter;
        if (RegisterHotKey(NULL, id, modifiers, key_code)) {
            Hotkey hotkey;
            hotkey.id = id;
            hotkey.key_combo = key_combo;
            hotkey.key_code = key_code;
            hotkey.modifiers = modifiers;
            registered_hotkeys.push_back(hotkey);
            
            // Сохраняем callback
            g_hotkey_callbacks[id] = callback;
            
            return true;
        }
#endif
        return false;
    }

    // Отмена регистрации горячей клавиши
    bool unregister_hotkey(const std::string& key_combo) {
#ifdef _WIN32
        for (auto it = registered_hotkeys.begin(); it != registered_hotkeys.end(); ++it) {
            if (it->key_combo == key_combo) {
                UnregisterHotKey(NULL, it->id);
                g_hotkey_callbacks.erase(it->id);
                registered_hotkeys.erase(it);
                return true;
            }
        }
#endif
        return false;
    }

    // Отмена регистрации всех горячих клавиш
    void unregister_all_hotkeys() {
#ifdef _WIN32
        for (const auto& hotkey : registered_hotkeys) {
            UnregisterHotKey(NULL, hotkey.id);
            g_hotkey_callbacks.erase(hotkey.id);
        }
        registered_hotkeys.clear();
#endif
    }

    // Активация прослушивания горячих клавиш
    bool start_listening() {
#ifdef _WIN32
        active = true;
        // Создание сообщений для горячих клавиш будет обрабатываться в основном цикле Qt
        return true;
#else
        return false;
#endif
    }

    // Деактивация прослушивания горячих клавиш
    bool stop_listening() {
#ifdef _WIN32
        active = false;
        return true;
#else
        return false;
#endif
    }

    // Проверка зарегистрирована ли горячая клавиша
    bool is_hotkey_registered(const std::string& key_combo) {
        for (const auto& hotkey : registered_hotkeys) {
            if (hotkey.key_combo == key_combo) {
                return true;
            }
        }
        return false;
    }

    // Получение списка зарегистрированных горячих клавиш
    std::vector<std::string> get_registered_hotkeys() {
        std::vector<std::string> result;
        for (const auto& hotkey : registered_hotkeys) {
            result.push_back(hotkey.key_combo);
        }
        return result;
    }

    // Преобразование строки в клавишу (для UI)
    static std::string key_to_string(int key_code) {
#ifdef _WIN32
        switch(key_code) {
            case VK_F1: return "F1";
            case VK_F2: return "F2";
            case VK_F3: return "F3";
            case VK_F4: return "F4";
            case VK_F5: return "F5";
            case VK_F6: return "F6";
            case VK_F7: return "F7";
            case VK_F8: return "F8";
            case VK_F9: return "F9";
            case VK_F10: return "F10";
            case VK_F11: return "F11";
            case VK_F12: return "F12";
            case VK_SPACE: return "Space";
            case VK_RETURN: return "Enter";
            case VK_TAB: return "Tab";
            case VK_ESCAPE: return "Esc";
            // и т.д.
            default:
                if (key_code >= 'A' && key_code <= 'Z') {
                    char c = static_cast<char>(key_code);
                    return std::string(1, c);
                }
                if (key_code >= '0' && key_code <= '9') {
                    char c = static_cast<char>(key_code);
                    return std::string(1, c);
                }
                return "Unknown";
        }
#else
        return "Unknown";
#endif
    }
};

// Модуль Python для горячих клавиш
PYBIND11_MODULE(hotkey_manager, m) {
    m.doc() = "Hotkey manager module for global keyboard shortcuts";
    
    py::class_<HotkeyManager>(m, "HotkeyManager")
        .def(py::init<>())
        .def("register_hotkey", &HotkeyManager::register_hotkey)
        .def("unregister_hotkey", &HotkeyManager::unregister_hotkey)
        .def("unregister_all_hotkeys", &HotkeyManager::unregister_all_hotkeys)
        .def("start_listening", &HotkeyManager::start_listening)
        .def("stop_listening", &HotkeyManager::stop_listening)
        .def("is_hotkey_registered", &HotkeyManager::is_hotkey_registered)
        .def("get_registered_hotkeys", &HotkeyManager::get_registered_hotkeys)
        .def_static("key_to_string", &HotkeyManager::key_to_string);
} 