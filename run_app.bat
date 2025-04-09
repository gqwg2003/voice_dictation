@echo off
REM Voice Dictation Application - C++ Version
REM Launch script for Windows

setlocal

REM Configuration
set BUILD_DIR=build_new
set APP_NAME=VoiceDictation.exe
set CMAKE_GENERATOR="MinGW Makefiles"
set CMAKE_CONFIG=Release
set QT_PATH=D:\Qt\6.9.0\mingw_64
set MINGW_PATH=D:\Qt\Tools\mingw1310_64\bin

REM Add MinGW and Qt tools to PATH
set PATH=%MINGW_PATH%;%QT_PATH%\bin;%PATH%

echo Using Qt from: %QT_PATH%
echo Using MinGW from: %MINGW_PATH%

REM Stop any running instances of the application
echo Stopping any running instances...
taskkill /F /IM %APP_NAME% 2>nul

REM Check if exe exists already
if not exist %BUILD_DIR%\%APP_NAME% (
    echo Building Voice Dictation application...
    cd %BUILD_DIR%
    cmake -G %CMAKE_GENERATOR% -DCMAKE_PREFIX_PATH=%QT_PATH% ..
    cmake --build . --config %CMAKE_CONFIG%
    cd ..
) else (
    echo Voice Dictation application found, launching...
)

REM Run the application
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
echo ----------------------------------------------------------------------

exit /b 0 