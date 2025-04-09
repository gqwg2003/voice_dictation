@echo off
setlocal enabledelayedexpansion

REM Установка переменной окружения для запуска в режиме разработки
SET VOICE_DICTATION_DEV=1

REM Переход в каталог приложения
cd /d "%~dp0"

REM Проверка наличия Python
python --version >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo Python не установлен или не добавлен в PATH
    echo Установите Python и добавьте его в PATH
    pause
    exit /b 1
)

REM Запуск приложения
python main.py

REM Если приложение завершилось с ошибкой
if %ERRORLEVEL% NEQ 0 (
    echo.
    echo Приложение завершилось с ошибкой (код %ERRORLEVEL%)
    echo Для продолжения нажмите любую клавишу...
    pause > nul
) 