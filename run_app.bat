@echo off
REM Voice Dictation Application - C++ Version
REM Launch script for Windows

setlocal

REM Configuration
set BUILD_DIR=build_new
set APP_NAME=VoiceDictation.exe
set CMAKE_GENERATOR="MinGW Makefiles"
set CMAKE_CONFIG=Release

REM Check if the user has set QT_DIR and MINGW_DIR environment variables
if not defined QT_DIR (
    echo WARNING: QT_DIR environment variable not set. Using PATH for Qt tools.
    set QT_DIR=
) else (
    echo Using Qt from: %QT_DIR%
    set PATH=%QT_DIR%\bin;%PATH%
)

if not defined MINGW_DIR (
    echo WARNING: MINGW_DIR environment variable not set. Using PATH for MinGW tools.
    set MINGW_DIR=
) else (
    echo Using MinGW from: %MINGW_DIR%
    set PATH=%MINGW_DIR%;%PATH%
)

REM Stop any running instances of the application
echo Stopping any running instances...
taskkill /F /IM %APP_NAME% 2>nul

REM Create build directory if it doesn't exist
if not exist %BUILD_DIR% (
    echo Creating build directory...
    mkdir %BUILD_DIR%
)

REM Check if we have the necessary tools in PATH
where cmake >nul 2>&1
if %ERRORLEVEL% neq 0 (
    echo ERROR: cmake not found in PATH. Please install CMake or add it to PATH.
    exit /b 1
)

where g++ >nul 2>&1
if %ERRORLEVEL% neq 0 (
    echo ERROR: g++ not found in PATH. Please install MinGW or add it to PATH.
    exit /b 1
)

REM Check if exe exists already
if not exist %BUILD_DIR%\%APP_NAME% (
    echo Building Voice Dictation application...
    cd %BUILD_DIR%
    
    echo Running CMake configuration...
    if defined QT_DIR (
        cmake -G %CMAKE_GENERATOR% -DCMAKE_PREFIX_PATH=%QT_DIR% ..
    ) else (
        cmake -G %CMAKE_GENERATOR% ..
    )
    
    if %ERRORLEVEL% neq 0 (
        echo ERROR: CMake configuration failed.
        cd ..
        exit /b 1
    )
    
    echo Building application...
    cmake --build . --config %CMAKE_CONFIG%
    if %ERRORLEVEL% neq 0 (
        echo ERROR: Build failed.
        cd ..
        exit /b 1
    )
    
    cd ..
) else (
    echo Voice Dictation application found, launching...
)

REM Check if the executable was built successfully
if not exist %BUILD_DIR%\%APP_NAME% (
    echo ERROR: Application executable not found after build.
    exit /b 1
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