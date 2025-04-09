@echo off
REM Voice Dictation Application - C++ Version
REM Launch script for Windows

setlocal

REM Configuration
set BUILD_DIR=build_new
set APP_NAME=VoiceDictation.exe
set CMAKE_GENERATOR="MinGW Makefiles"
set CMAKE_CONFIG=Release
set QT_PATH=C:\Qt\6.9.0\mingw_64
set MINGW_PATH=C:\Qt\Tools\mingw1310_64\bin

REM Add MinGW and Qt tools to PATH
set PATH=%MINGW_PATH%;%QT_PATH%\bin;%PATH%

echo Using Qt from: %QT_PATH%
echo Using MinGW from: %MINGW_PATH%

REM Check if build directory exists
if not exist %BUILD_DIR% (
    echo Building Voice Dictation application for the first time...
    mkdir %BUILD_DIR%
    cd %BUILD_DIR%
    cmake -G %CMAKE_GENERATOR% -DCMAKE_PREFIX_PATH=%QT_PATH% ..
    if errorlevel 1 (
        echo CMake generation failed!
        exit /b 1
    )
    cmake --build . --config %CMAKE_CONFIG%
    if errorlevel 1 (
        echo Build failed!
        exit /b 1
    )
    cd ..
) else (
    echo Checking for updates to the application...
    cd %BUILD_DIR%
    cmake -G %CMAKE_GENERATOR% -DCMAKE_PREFIX_PATH=%QT_PATH% ..
    cmake --build . --config %CMAKE_CONFIG%
    if errorlevel 1 (
        echo Build failed!
        exit /b 1
    )
    cd ..
)

REM Stop any running instances of the application
echo Stopping any running instances...
taskkill /F /IM VoiceDictation.exe 2>nul

REM Deploy Qt dependencies
echo Deploying Qt dependencies...
windeployqt %BUILD_DIR%\%APP_NAME% --no-translations

REM Run the application with show-window parameter
echo Starting Voice Dictation application...
start "" %BUILD_DIR%\%APP_NAME% --show-window

echo.
echo ----------------------------------------------------------------------
echo Voice Dictation application has been started!
echo.
echo NOTE: The application runs in the system tray.
echo - Look for the Voice Dictation icon in the system tray (near the clock)
echo - Double-click the tray icon to show/hide the main window
echo - Right-click the tray icon for additional options
echo.
echo If you close the main window, the application will continue
echo running in the background.
echo ----------------------------------------------------------------------

exit /b 0 