# Голосовой Ввод / Voice Dictation

## 🇷🇺 Русский

Многоязычное приложение для распознавания голоса и быстрого ввода текста. Идеально подходит для общения в игровых чатах, мессенджерах и социальных сетях без необходимости отвлекаться от игрового процесса.

### 🔑 Ключевые особенности
- Мгновенное распознавание речи
- Мультиязычная поддержка (русский, английский)
- Полное управление с клавиатуры с помощью настраиваемых горячих клавиш
- Мгновенный доступ к тексту через буфер обмена
- Ненавязчивая работа в фоновом режиме через системный трей
- Улучшенная безопасность и защита данных

### 🚀 Установка и запуск

#### Для пользователей Windows:
```
# Запустите скрипт для сборки и запуска приложения
run_app.bat
```

#### Сборка из исходного кода:
```bash
# Создание директории для сборки
mkdir build && cd build

# Генерация проекта с помощью CMake
cmake ..

# Сборка проекта
cmake --build .

# Запуск приложения
./VoiceDictation
```

### 📝 Использование
| Действие | Горячая клавиша (по умолчанию) |
|----------|--------------------------------|
| Записать | Ctrl+Alt+R |
| Копировать текст | Ctrl+Alt+C |
| Очистить | Ctrl+Alt+X |

### 🧰 Режимы работы
Приложение поддерживает несколько режимов работы с разной производительностью, автоматически выбирая оптимальный режим для вашего устройства.

### 🔒 Безопасность
- Информация о безопасности доступна в файле [SECURITY.md](SECURITY.md).

### 📋 Зависимости
- Qt 6 (Core, Widgets, Multimedia)
- nlohmann-json
- C++17 совместимый компилятор

### 📄 Лицензия
MIT

---

## 🇬🇧 English

A multilingual application for voice recognition and quick text input. Perfect for communication in game chats, messengers, and social networks without distracting from the gaming process.

### 🔑 Key Features
- Instant speech recognition
- Multilingual support (Russian, English)
- Full keyboard control with customizable hotkeys
- Instant access to text via clipboard
- Unobtrusive background operation via system tray
- Enhanced security and data protection

### 🚀 Installation and Launch

#### For Windows users:
```
# Run the script to build and launch the application
run_app.bat
```

#### Building from source:
```bash
# Create build directory
mkdir build && cd build

# Generate project with CMake
cmake ..

# Build the project
cmake --build .

# Run the application
./VoiceDictation
```

### 📝 Usage
| Action | Hotkey (default) |
|--------|------------------|
| Record | Ctrl+Alt+R |
| Copy text | Ctrl+Alt+C |
| Clear | Ctrl+Alt+X |

### 🧰 Operation Modes
The application supports several operation modes with different performance levels, automatically selecting the optimal mode for your device.

### 🔒 Security
- Security information is available in the [SECURITY.md](SECURITY.md) file.

### 📋 Dependencies
- Qt 6 (Core, Widgets, Multimedia)
- nlohmann-json
- C++17 compatible compiler

### 📄 License
MIT