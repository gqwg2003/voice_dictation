#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <string>
#include <vector>
#include <deque>
#include <algorithm>
#include <chrono>
#include <ctime>

#ifdef _WIN32
#include <windows.h>
#endif

namespace py = pybind11;

class ClipboardManager {
private:
    // История буфера обмена
    std::deque<std::string> clipboard_history;
    // Максимальный размер истории
    size_t max_history_size = 10;
    // Сохраняет последний скопированный текст для предотвращения дубликатов
    std::string last_clipboard_text;
    // Флаг активности отслеживания буфера обмена
    bool is_tracking = false;

public:
    ClipboardManager() = default;
    
    ~ClipboardManager() {
        stop_tracking();
    }

    // Получение текста из буфера обмена
    std::string get_clipboard_text() {
#ifdef _WIN32
        if (!OpenClipboard(NULL))
            return "";

        HANDLE hData = GetClipboardData(CF_TEXT);
        if (hData == NULL) {
            CloseClipboard();
            return "";
        }

        char* pszText = static_cast<char*>(GlobalLock(hData));
        if (pszText == NULL) {
            CloseClipboard();
            return "";
        }

        std::string text(pszText);
        GlobalUnlock(hData);
        CloseClipboard();
        return text;
#else
        // Заглушка для других ОС
        return "";
#endif
    }
    
    // Установка текста в буфер обмена
    bool set_clipboard_text(const std::string& text) {
#ifdef _WIN32
        if (!OpenClipboard(NULL))
            return false;
            
        EmptyClipboard();
        
        HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, text.size() + 1);
        if (hMem == NULL) {
            CloseClipboard();
            return false;
        }
        
        char* pszText = static_cast<char*>(GlobalLock(hMem));
        if (pszText == NULL) {
            CloseClipboard();
            return false;
        }
        
        memcpy(pszText, text.c_str(), text.size() + 1);
        GlobalUnlock(hMem);
        
        if (SetClipboardData(CF_TEXT, hMem) == NULL) {
            CloseClipboard();
            return false;
        }
        
        CloseClipboard();
        
        // Обновление внутреннего состояния
        last_clipboard_text = text;
        add_to_history(text);
        
        return true;
#else
        // Заглушка для других ОС
        return false;
#endif
    }
    
    // Добавление текста в историю буфера обмена
    void add_to_history(const std::string& text) {
        // Проверка на пустой текст
        if (text.empty())
            return;
            
        // Проверка на дубликат последнего элемента
        if (!clipboard_history.empty() && clipboard_history.front() == text)
            return;
            
        // Добавление в начало истории
        clipboard_history.push_front(text);
        
        // Удаление старых элементов, если превышен максимальный размер
        while (clipboard_history.size() > max_history_size) {
            clipboard_history.pop_back();
        }
    }
    
    // Очистка истории буфера обмена
    void clear_history() {
        clipboard_history.clear();
    }
    
    // Получение истории буфера обмена
    std::vector<std::string> get_history() const {
        return std::vector<std::string>(clipboard_history.begin(), clipboard_history.end());
    }
    
    // Установка максимального размера истории
    void set_max_history_size(size_t size) {
        max_history_size = size;
        
        // Обрезаем историю, если новый размер меньше текущего
        while (clipboard_history.size() > max_history_size) {
            clipboard_history.pop_back();
        }
    }
    
    // Получение максимального размера истории
    size_t get_max_history_size() const {
        return max_history_size;
    }
    
    // Начать отслеживание буфера обмена
    bool start_tracking() {
#ifdef _WIN32
        if (is_tracking)
            return true;
            
        // В реальной реализации здесь был бы код для запуска отслеживания
        // через Windows API (SetClipboardViewer, AddClipboardFormatListener и т.д.)
        is_tracking = true;
        
        // Начальная инициализация
        try {
            std::string current_text = get_clipboard_text();
            last_clipboard_text = current_text;
            if (!current_text.empty()) {
                add_to_history(current_text);
            }
        } catch (...) {
            // Игнорируем ошибки инициализации
        }
        
        return true;
#else
        // Заглушка для других ОС
        return false;
#endif
    }
    
    // Остановить отслеживание буфера обмена
    bool stop_tracking() {
#ifdef _WIN32
        if (!is_tracking)
            return true;
            
        // В реальной реализации здесь был бы код для остановки отслеживания
        // через Windows API (ChangeClipboardChain, RemoveClipboardFormatListener и т.д.)
        is_tracking = false;
        return true;
#else
        // Заглушка для других ОС
        return false;
#endif
    }
    
    // Проверить, активно ли отслеживание
    bool is_tracking_active() const {
        return is_tracking;
    }
    
    // Обработка события изменения буфера обмена
    // Будет вызываться из кода Qt
    void process_clipboard_change() {
        try {
            std::string current_text = get_clipboard_text();
            if (current_text != last_clipboard_text && !current_text.empty()) {
                last_clipboard_text = current_text;
                add_to_history(current_text);
            }
        } catch (...) {
            // Игнорируем ошибки обработки
        }
    }
    
    // Получить дату и время изменения буфера обмена (заглушка)
    std::string get_clipboard_timestamp() {
        auto now = std::chrono::system_clock::now();
        auto time_t_now = std::chrono::system_clock::to_time_t(now);
        
        std::tm tm_now;
#ifdef _WIN32
        localtime_s(&tm_now, &time_t_now);
#else
        tm_now = *std::localtime(&time_t_now);
#endif
        
        char buffer[80];
        std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &tm_now);
        return std::string(buffer);
    }
};

PYBIND11_MODULE(clipboard_manager, m) {
    m.doc() = "Clipboard manager module for clipboard history and operations";
    
    py::class_<ClipboardManager>(m, "ClipboardManager")
        .def(py::init<>())
        .def("get_clipboard_text", &ClipboardManager::get_clipboard_text)
        .def("set_clipboard_text", &ClipboardManager::set_clipboard_text)
        .def("add_to_history", &ClipboardManager::add_to_history)
        .def("clear_history", &ClipboardManager::clear_history)
        .def("get_history", &ClipboardManager::get_history)
        .def("set_max_history_size", &ClipboardManager::set_max_history_size)
        .def("get_max_history_size", &ClipboardManager::get_max_history_size)
        .def("start_tracking", &ClipboardManager::start_tracking)
        .def("stop_tracking", &ClipboardManager::stop_tracking)
        .def("is_tracking_active", &ClipboardManager::is_tracking_active)
        .def("process_clipboard_change", &ClipboardManager::process_clipboard_change)
        .def("get_clipboard_timestamp", &ClipboardManager::get_clipboard_timestamp);
} 