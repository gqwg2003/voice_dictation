@echo off
setlocal enabledelayedexpansion

echo ========================================
echo Сборка C++ расширений для приложения
echo ========================================

REM Проверка наличия CMake
where cmake >nul 2>nul
if %ERRORLEVEL% NEQ 0 (
    echo Ошибка: CMake не найден. Пожалуйста, установите CMake.
    echo Вы можете скачать CMake с https://cmake.org/download/
    pause
    exit /b 1
)

REM Проверка наличия компилятора C++
where cl >nul 2>nul
if %ERRORLEVEL% NEQ 0 (
    echo Предупреждение: Компилятор MSVC не найден.
    echo Для установки, скачайте Visual Studio с инструментами C++.
    echo Попытаемся продолжить, но сборка может завершиться с ошибкой.
    pause
)

REM Установка рабочего каталога
set WORKSPACE_DIR=%~dp0
set WORKSPACE_DIR=%WORKSPACE_DIR:~0,-1%
echo Рабочий каталог: %WORKSPACE_DIR%

REM Создание директории сборки, если она не существует
if not exist "%WORKSPACE_DIR%\build" (
    echo Создание директории сборки...
    mkdir "%WORKSPACE_DIR%\build"
)

REM Конфигурация и сборка
echo Конфигурация проекта с помощью CMake...
cd "%WORKSPACE_DIR%\build"
cmake -DCMAKE_BUILD_TYPE=Release ..
if %ERRORLEVEL% NEQ 0 (
    echo Ошибка: Не удалось сконфигурировать проект с помощью CMake.
    pause
    exit /b 1
)

echo Сборка проекта...
cmake --build . --config Release
if %ERRORLEVEL% NEQ 0 (
    echo Ошибка: Не удалось собрать проект.
    pause
    exit /b 1
)

REM Создание целевой директории, если она не существует
if not exist "%WORKSPACE_DIR%\build\Release" (
    echo Создание директории для бинарных файлов...
    mkdir "%WORKSPACE_DIR%\build\Release"
)

REM Проверка, что файлы были созданы
echo Проверка скомпилированных файлов...
if not exist "%WORKSPACE_DIR%\build\Release\audio_processor.pyd" (
    echo Ошибка: Файл audio_processor.pyd не был создан.
    pause
    exit /b 1
)

if not exist "%WORKSPACE_DIR%\build\Release\text_processor.pyd" (
    echo Ошибка: Файл text_processor.pyd не был создан.
    pause
    exit /b 1
)

echo.
echo ========================================
echo C++ расширения успешно собраны!
echo ========================================
echo Расположение файлов: %WORKSPACE_DIR%\build\Release
echo.
echo Используемые расширения:
echo - audio_processor: расширенная обработка аудио
echo - text_processor: улучшенная обработка текста
echo.
echo Примечание: Эти файлы автоматически используются приложением
echo.

pause 