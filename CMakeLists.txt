cmake_minimum_required(VERSION 3.16)
project(VoiceDictation VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Find required packages
find_package(Qt6 COMPONENTS Core Widgets Multimedia REQUIRED)

# Always download nlohmann_json
include(FetchContent)
FetchContent_Declare(json URL https://github.com/nlohmann/json/releases/download/v3.11.2/json.tar.xz)
FetchContent_MakeAvailable(json)

# Add include directories
include_directories(${CMAKE_SOURCE_DIR})

# Sources
set(SOURCES
    src/main.cpp
    src/gui/MainWindow.cpp
    src/gui/AudioVisualizer.cpp
    src/gui/SettingsDialog.cpp
    src/core/SpeechRecognizer.cpp
    src/core/AudioProcessor.cpp
    src/core/TextProcessor.cpp
    src/core/ClipboardManager.cpp
    src/core/HotkeyManager.cpp
    src/utils/Logger.cpp
    src/utils/Version.cpp
)

# Headers
set(HEADERS
    src/gui/MainWindow.h
    src/gui/AudioVisualizer.h
    src/gui/SettingsDialog.h
    src/core/SpeechRecognizer.h
    src/core/AudioProcessor.h
    src/core/TextProcessor.h
    src/core/ClipboardManager.h
    src/core/HotkeyManager.h
    src/utils/Logger.h
    src/utils/Version.h
)

# Resources
set(RESOURCES
    resources/resources.qrc
)

# Define the executable
add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS} ${RESOURCES})

# Link libraries
target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt6::Core
    Qt6::Widgets
    Qt6::Multimedia
    nlohmann_json::nlohmann_json
)

# Install target
install(TARGETS ${PROJECT_NAME} DESTINATION bin)

# Copy resources
file(COPY ${CMAKE_SOURCE_DIR}/resources DESTINATION ${CMAKE_BINARY_DIR})

# Platform specific settings
if(WIN32)
    set_target_properties(${PROJECT_NAME} PROPERTIES WIN32_EXECUTABLE TRUE)
    target_link_libraries(${PROJECT_NAME} PRIVATE winmm)
endif()

if(UNIX AND NOT APPLE)
    target_link_libraries(${PROJECT_NAME} PRIVATE X11 pthread pulse pulse-simple)
endif()

if(APPLE)
    set_target_properties(${PROJECT_NAME} PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_GUI_IDENTIFIER "com.voicedictation.app"
        MACOSX_BUNDLE_BUNDLE_NAME "Voice Dictation"
    )
endif() 