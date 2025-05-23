# Голосовая Диктовка 🎙️

[![C++17](https://img.shields.io/badge/C%2B%2B-17-blue.svg)](https://isocpp.org/std/the-standard)
[![Qt 6](https://img.shields.io/badge/Qt-6.10.0-41CD52.svg)](https://www.qt.io/)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)

Мультиязычное приложение для голосовой диктовки и распознавания речи, разработанное с использованием C++ и Qt 6. Позволяет записывать голос, мгновенно преобразовывать его в текст и сохранять результаты.

![Скриншот приложения](resources/screenshots/app_screenshot.png)

## 📋 Содержание

- [Возможности](#-возможности)
- [Технологии](#-технологии)
- [Архитектура](#-архитектура)
- [Требования](#-требования)
- [Установка и запуск](#-установка-и-запуск)
- [Сборка из исходного кода](#-сборка-из-исходного-кода)
- [Развертывание для Windows](#-развертывание-для-windows)
- [Разработка](#-разработка)
- [Безопасность](#-безопасность)
- [Лицензия](#-лицензия)

## ✨ Возможности

- **Мультиязычное распознавание**: Поддержка русского и английского языков
- **Запись в реальном времени**: Захват аудио с микрофона с регулируемыми параметрами
- **Гибкие настройки распознавания**: Выбор языка и настройка параметров
- **Визуализация аудио**: Динамический индикатор уровня громкости микрофона
- **Система уведомлений**: Интерактивные уведомления о событиях приложения
- **Управление данными**: Сохранение и экспорт распознанного текста в файл
- **Автономный режим**: Возможность работы без подключения к интернету

## 🔧 Технологии

- **C++17**: Современный, высокопроизводительный язык программирования
- **Qt 6.10.0**: Кросс-платформенный фреймворк для создания GUI-приложений
  - **Qt Core**: Базовые функции и типы данных
  - **Qt Widgets**: Элементы пользовательского интерфейса
  - **Qt Multimedia**: Работа с аудио и видео
  - **Qt Network**: Сетевое взаимодействие и API-запросы
- **whisper.cpp 1.7.5**: Оффлайн распознавание речи
- **nlohmann/json 3.12.0**: Работа с JSON-данными
- **Обработка аудио**: Запись, анализ и обработка звуковых сигналов
- **Распознавание речи**: Интеграция с различными движками распознавания речи

## 🏗️ Архитектура

Приложение построено на основе модульной многоуровневой архитектуры, обеспечивающей расширяемость и поддержку различных движков распознавания речи.

### Основные компоненты

- **MainWindow** - Главное окно приложения с основным пользовательским интерфейсом
- **SpeechRecognizer** - Ядро системы для записи и распознавания речи
- **RecognitionService** - Абстрактный интерфейс для различных движков распознавания
  - **WhisperRecognitionService** - Автономный движок на базе whisper.cpp
  - **GoogleRecognitionService** - Реализация для Google Speech API
  - **DeepSpeechRecognitionService** - Автономный движок на базе Mozilla DeepSpeech
- **AudioManager** - Управление записью и обработкой аудио
- **QmlIntegration** - Связь между C++ и QML компонентами
- **WaveProgress** - Анимированная визуализация уровня микрофона (QML)
- **NotificationManager** - Система управления уведомлениями (QML)
- **SettingsDialog** - Настройки приложения и параметров распознавания

## 📋 Требования

### Минимальные требования

- Операционная система: Windows 10/11,
- Процессор: двухъядерный, 2 ГГц
- Оперативная память: 4 ГБ
- Свободное место на диске: 150 МБ
- Микрофон: Любой стандартный аудиовход

### Требования для разработки

- **Qt 6.10.0** или выше
- **CMake 3.16** или выше
- **Компилятор** с поддержкой C++17 (MSVC 2019+, GCC 9+, Clang 10+)
- **Git** для управления версиями

## 🚀 Установка и запуск

### Windows

1. Скачайте последнюю версию приложения с [страницы релизов](https://github.com/gqwg2003/voice_dictation/releases)
2. Распакуйте архив в удобное место
3. Запустите `VoiceDictation.exe`

### Linux

```bash
# Установка зависимостей (Ubuntu/Debian)
sudo apt-get update
sudo apt-get install libqt6core6 libqt6gui6 libqt6widgets6 libqt6multimedia6 libqt6qml6

# Установка приложения
sudo dpkg -i voice-dictation_X.X.X_amd64.deb

# Запуск
voice-dictation
```

## 🔨 Сборка из исходного кода

### Подготовка

1. Установите Qt 6.10.0+ (https://www.qt.io/download)
2. Установите CMake 3.16+ (https://cmake.org/download/)
3. Установите компилятор с поддержкой C++17

### Пошаговая инструкция

1. Клонируйте репозиторий:
   ```bash
   git clone https://github.com/gqwg2003/voice_dictation.git
   cd voice_dictation
   ```

2. Создайте директорию для сборки:
   ```bash
   mkdir build
   cd build
   ```

3. Сконфигурируйте проект с помощью CMake:
   ```bash
   cmake ..
   ```

4. Соберите проект:
   ```bash
   cmake --build . --config Release
   ```

5. Запустите приложение:
   ```bash
   ./VoiceDictation    # Linux/macOS
   .\VoiceDictation.exe  # Windows
   ```

## 📦 Развертывание для Windows

Для создания автономного дистрибутива на Windows:

```bash
# В директории сборки
windeployqt --release --qmldir=../src/qml VoiceDictation.exe
```

Это создаст папку со всеми необходимыми DLL и зависимостями для запуска приложения на других компьютерах без установленного Qt.

## 🧰 Разработка

### Структура проекта

```
voice_dictation/
├── src/                    # Исходный код
│   ├── core/               # Основная логика приложения
│   ├── ui/                 # Пользовательский интерфейс
│   ├── qml/                # QML компоненты
│   ├── utils/              # Вспомогательные классы и функции
│   └── services/           # Сервисы распознавания речи
├── resources/              # Ресурсы (иконки, шрифты, стили)
├── tests/                  # Тесты
└── docs/                   # Документация
```

### Создание нового движка распознавания

Для добавления нового движка распознавания речи:

1. Создайте новый класс, наследующийся от `RecognitionService`
2. Реализуйте все необходимые методы
3. Зарегистрируйте сервис в `SpeechRecognizer`

```cpp
class YourRecognitionService : public RecognitionService {
public:
    // Реализуйте абстрактные методы
};
```

## 🔒 Безопасность

### Требования безопасности

Для обеспечения безопасности приложения используются следующие версии библиотек:

- **Qt 6.10.0** - защита от уязвимостей в сетевом стеке и обработке медиа
- **whisper.cpp 1.7.5** - устранение проблем обработки памяти и улучшение точности распознавания
- **nlohmann/json 3.12.0** - защита от уязвимостей при разборе JSON-данных

### Загрузка моделей Whisper

Для работы оффлайн-распознавания необходимо загрузить модели Whisper:

```powershell
# Windows (PowerShell)
.\scripts\download-whisper-models.ps1 base
```

Доступные модели:
- **tiny/tiny.en** - самая маленькая и быстрая модель
- **base/base.en** - сбалансированная модель (рекомендуется)
- **small/small.en** - более точная модель
- **medium/medium.en** - высокоточная модель
- **large-v2/large-v3** - наиболее точные модели

### Важные функции безопасности

- **Проверка входных данных** - все входные данные проверяются перед обработкой
- **no_speech_threshold** - защита от ложных распознаваний при отсутствии речи
- **Безопасные вызовы API** - использование защищенных соединений для облачных сервисов
- **Изоляция компонентов** - модульная архитектура для минимизации воздействия уязвимостей

Полную информацию о безопасности см. в файле [SECURITY.md](SECURITY.md).

## 📄 Лицензия

Проект распространяется под лицензией GPL-3.0 license. Подробности в файле [LICENSE](LICENSE).

---

© 2025 Voice Dictation Team 