@echo off
setlocal enabledelayedexpansion

echo =====================================
echo Voice Dictation C++ Extensions Builder
echo =====================================
echo.

REM Определение текущей директории
set CURRENT_DIR=%~dp0
set CURRENT_DIR=%CURRENT_DIR:~0,-1%
set PROJECT_ROOT=%CURRENT_DIR%\..\..\
set BUILD_DIR=%CURRENT_DIR%\build\Release

REM Проверка наличия инструментов для сборки
echo Checking build requirements...

REM Проверка наличия CMake
where cmake >nul 2>nul
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: CMake not found! Please install CMake first.
    echo Download from: https://cmake.org/download/
    exit /b 1
)

REM Проверка наличия компилятора C++
echo Checking C++ compiler...
where cl >nul 2>nul
if %ERRORLEVEL% NEQ 0 (
    echo WARNING: Microsoft Visual C++ compiler not found in PATH.
    echo Make sure you have Visual Studio installed with C++ support.
    echo Attempting to proceed with build anyway...
) else (
    echo Visual C++ compiler found.
)

REM Создание build директории, если не существует
if not exist build mkdir build
cd build

REM Удаляем CMakeCache.txt, если он существует, чтобы избежать конфликтов генераторов
if exist CMakeCache.txt (
    echo Removing existing CMakeCache.txt to avoid generator conflicts...
    del /F /Q CMakeCache.txt
)
if exist CMakeFiles (
    echo Removing CMakeFiles directory...
    rmdir /S /Q CMakeFiles
)

REM Определение Python и импортирование pybind11
echo Configuring Python for build...
python -c "import pybind11; print('pybind11 found at:', pybind11.get_include())" >nul 2>nul
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: pybind11 not found! Installing via pip...
    pip install pybind11
    if %ERRORLEVEL% NEQ 0 (
        echo ERROR: Failed to install pybind11. Please install manually with:
        echo pip install pybind11
        exit /b 1
    )
)

REM Настройка проекта с помощью CMake
echo.
echo Configuring CMake build...

REM Сначала пытаемся использовать Visual Studio 2022
cmake .. -G "Visual Studio 17 2022" -A x64 -DCMAKE_BUILD_TYPE=Release
if %ERRORLEVEL% NEQ 0 (
    echo.
    echo WARNING: Failed with VS2022, trying with VS2019...
    cmake .. -G "Visual Studio 16 2019" -A x64 -DCMAKE_BUILD_TYPE=Release
    if %ERRORLEVEL% NEQ 0 (
        echo.
        echo WARNING: Failed with VS2019, trying with VS2017...
        cmake .. -G "Visual Studio 15 2017" -A x64 -DCMAKE_BUILD_TYPE=Release
        if %ERRORLEVEL% NEQ 0 (
            echo.
            echo WARNING: Failed with VS2017, trying with default generator...
            cmake .. -DCMAKE_BUILD_TYPE=Release
            if %ERRORLEVEL% NEQ 0 (
                echo.
                echo ERROR: CMake configuration failed. Please check your build environment.
                exit /b 1
            )
        )
    )
)

REM Сборка проекта
echo.
echo Building C++ extensions...
cmake --build . --config Release
if %ERRORLEVEL% NEQ 0 (
    echo.
    echo ERROR: Build failed! Please check error messages above.
    exit /b 1
)

echo.
echo Creating release directory...
if not exist Release mkdir Release

REM Дополнительные директории для копирования
set ADDITIONAL_DIR1=%CURRENT_DIR%\Release
set ADDITIONAL_DIR2=%CURRENT_DIR%\..\Release

REM Создаем дополнительные директории, если они не существуют
if not exist "%ADDITIONAL_DIR1%" mkdir "%ADDITIONAL_DIR1%"
if not exist "%ADDITIONAL_DIR2%" mkdir "%ADDITIONAL_DIR2%"

REM Список модулей
set MODULES=audio_processor text_processor speech_preprocessor speech_recognizer hotkey_manager clipboard_manager

REM Информация о собранных модулях
echo.
echo Extensions successfully built:
for %%m in (%MODULES%) do (
    if exist Release\%%m.pyd (
        echo - %%m.pyd
    ) else (
        echo WARNING: %%m.pyd not found!
    )
)

REM Копирование в дополнительные места для совместимости
echo.
echo Copying extensions to additional locations for compatibility...
for %%m in (%MODULES%) do (
    if exist Release\%%m.pyd (
        echo - Copying %%m.pyd to %ADDITIONAL_DIR1%
        copy /Y Release\%%m.pyd "%ADDITIONAL_DIR1%\"
    )
)

REM Возвращаемся в исходную директорию
cd %CURRENT_DIR%

echo.
echo =====================================
echo Build completed successfully!
echo Extensions have been installed to: %BUILD_DIR%
echo Additional copies in: %ADDITIONAL_DIR1%
echo =====================================

REM Статус модулей
echo.
echo Running hybrid module check...
cd %PROJECT_ROOT%
python -c "import os; os.environ['PYTHONPATH'] = os.getcwd(); from core.hybrid_manager import get_mode_info; mode = get_mode_info(); print('Active mode:', mode['mode']); print('Extensions available:', [ext for ext, status in mode['extensions_available'].items() if status]); print('Performance level:', mode['performance_level'], 'of 5')"

echo.
echo To test the extensions, run your application now.
echo You should see improved performance with C++ modules.
echo.

endlocal 