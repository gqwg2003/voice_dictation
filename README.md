# Голосовой Ввод / Voice Dictation

## 🇷🇺 Русский

Многоязычное приложение для распознавания голоса и быстрого ввода текста. Идеально подходит для общения в игровых чатах, мессенджерах и социальных сетях без необходимости отвлекаться от игрового процесса.

### 🔑 Ключевые особенности
- Мгновенное распознавание речи через Google Speech Recognition API
- Мультиязычная поддержка (русский, английский)
- Полное управление с клавиатуры с помощью настраиваемых горячих клавиш
- Мгновенный доступ к тексту через буфер обмена
- Ненавязчивая работа в фоновом режиме через системный трей
- Улучшенная безопасность и защита данных
- **Чистая гибридная архитектура:** автоматическое переключение между C++ и Python компонентами
- **Адаптивная производительность:** оптимальная работа на различных устройствах
- Чистая Python-реализация без внешних нативных зависимостей

### 📋 Технические требования
- Python 3.6 или новее
- Основные зависимости: PyQt6, SpeechRecognition, pyaudio, pyperclip, pynput
- **Опционально:** CMake и компилятор C++ для сборки расширений (повышает производительность)

### 🚀 Установка и запуск
```bash
# Клонирование репозитория
git clone https://github.com/gqwg2003/voice_dictation.git

# Установка зависимостей
pip install -r requirements.txt

# Запуск приложения
python app.py
```

### 📝 Использование
| Действие | Горячая клавиша (по умолчанию) |
|----------|--------------------------------|
| Записать | Ctrl+Alt+R |
| Копировать текст | Ctrl+Alt+C |
| Очистить | Ctrl+Alt+X |

### 🧹 Обслуживание
Для очистки временных файлов и логов используйте встроенный скрипт очистки:
```bash
# Показать, какие файлы будут удалены, без фактического удаления
python scripts/cleanup.py --dry-run

# Удалить все временные файлы и логи старше 30 дней
python scripts/cleanup.py

# Удалить все логи независимо от их возраста
python scripts/cleanup.py --all-logs
```

### 📦 Версионирование
Проект использует [Семантическое Версионирование](https://semver.org/lang/ru/) (MAJOR.MINOR.PATCH). Для управления версиями используются автоматические инструменты на основе Conventional Commits.

### 🔒 Безопасность
- Информация о безопасности и процедуре сообщения об уязвимостях доступна в файле [SECURITY.md](SECURITY.md).
- Подробное руководство по безопасности для пользователей и разработчиков можно найти в [SECURITY_GUIDE.md](SECURITY_GUIDE.md).

### 📄 Лицензия
MIT

---

## 🇬🇧 English

A multilingual application for voice recognition and quick text input. Perfect for communication in game chats, messengers, and social networks without distracting from the gaming process.

### 🔑 Key Features
- Instant speech recognition via Google Speech Recognition API
- Multilingual support (Russian, English)
- Full keyboard control with customizable hotkeys
- Instant access to text via clipboard
- Unobtrusive background operation via system tray
- Enhanced security and data protection
- **Pure hybrid architecture:** automatic switching between C++ and Python components
- **Adaptive performance:** optimal operation on various devices
- Pure Python implementation without external native dependencies

### 📋 Technical Requirements
- Python 3.6 or newer
- Main dependencies: PyQt6, SpeechRecognition, pyaudio, pyperclip, pynput
- **Optional:** CMake and C++ compiler for building extensions (increases performance)

### 🚀 Installation and Launch
```bash
# Clone repository
git clone https://github.com/gqwg2003/voice_dictation.git

# Install dependencies
pip install -r requirements.txt

# Run application
python app.py
```

### 📝 Usage
| Action | Hotkey (default) |
|--------|------------------|
| Record | Ctrl+Alt+R |
| Copy text | Ctrl+Alt+C |
| Clear | Ctrl+Alt+X |

### 🧹 Maintenance
To clean temporary files and logs, use the built-in cleanup script:
```bash
# Show what files would be deleted without actually deleting them
python scripts/cleanup.py --dry-run

# Delete all temporary files and logs older than 30 days
python scripts/cleanup.py

# Delete all logs regardless of their age
python scripts/cleanup.py --all-logs
```

### 📦 Versioning
The project uses [Semantic Versioning](https://semver.org/) (MAJOR.MINOR.PATCH). Version management is handled through automated tools based on Conventional Commits.

### 🔒 Security
- Security information and vulnerability reporting procedures are available in the [SECURITY.md](SECURITY.md) file.
- A detailed security guide for users and developers can be found in [SECURITY_GUIDE.md](SECURITY_GUIDE.md).

### 📄 License
MIT

### 🧰 Operation Modes
The application supports several operation modes:

| Mode | Description | Performance |
|------|-------------|-------------|
| **Full hybrid** | All C++ components available | Maximum |
| **Partial hybrid** | Some C++ components available | High |
| **Pure Python** | Python implementations only | Medium |
| **Light mode** | Minimal functionality for mobile devices | Basic |

The application automatically determines the optimal operation mode based on available components and system characteristics. 