cmake_minimum_required(VERSION 3.16)
project(VoiceDictation VERSION 3.5.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Find required packages
# Требуем Qt 6.10.0 для устранения уязвимостей
find_package(Qt6 6.10.0 COMPONENTS Core Widgets Multimedia Network REQUIRED)

# Проверяем версию Qt для уверенности
if(Qt6_VERSION VERSION_LESS 6.10.0)
    message(FATAL_ERROR "Qt 6.10.0 или выше требуется для устранения уязвимостей безопасности. Найдена версия: ${Qt6_VERSION}")
endif()

# Используем последнюю версию nlohmann_json для исправления уязвимостей
include(FetchContent)
FetchContent_Declare(json URL https://github.com/nlohmann/json/releases/download/v3.12.0/json.tar.xz)
FetchContent_MakeAvailable(json)

# Версия whisper.cpp v1.7.5
set(WHISPER_VERSION "1.7.5")

# Try to find Whisper library
option(USE_WHISPER "Use Whisper library for speech recognition" ON)
if(USE_WHISPER)
    # Set Whisper paths - try to find in common locations
    set(WHISPER_ROOT_DIR "" CACHE PATH "Root directory for Whisper library")
    
    # Look in common locations if not specified
    if(NOT WHISPER_ROOT_DIR)
        # Windows common locations
        if(WIN32)
            list(APPEND WHISPER_SEARCH_PATHS
                "C:/Program Files/whisper"
                "C:/Program Files (x86)/whisper"
                "C:/whisper"
                "${CMAKE_SOURCE_DIR}/third_party/whisper"
            )
        endif()
        
        # Linux/macOS common locations
        if(UNIX)
            list(APPEND WHISPER_SEARCH_PATHS
                "/usr/local/include/whisper"
                "/usr/include/whisper"
                "${CMAKE_SOURCE_DIR}/third_party/whisper"
                "$ENV{HOME}/whisper"
            )
        endif()
        
        # Try to find whisper.h
        find_path(WHISPER_INCLUDE_DIR whisper.h
            PATHS ${WHISPER_SEARCH_PATHS}
            PATH_SUFFIXES include 
        )
        
        # Try to find library
        find_library(WHISPER_LIBRARY
            NAMES whisper whisper-cpp libwhisper libwhisper-cpp
            PATHS ${WHISPER_SEARCH_PATHS}
            PATH_SUFFIXES lib
        )
        
        if(WHISPER_INCLUDE_DIR AND WHISPER_LIBRARY)
            set(WHISPER_FOUND TRUE)
            message(STATUS "Found Whisper: ${WHISPER_LIBRARY}")
            message(STATUS "Whisper include: ${WHISPER_INCLUDE_DIR}")
        else()
            set(WHISPER_FOUND FALSE)
            message(STATUS "Whisper not found, speech recognition will use fallback methods")
        endif()
    endif()
    
    if(WHISPER_FOUND)
        add_definitions(-DHAVE_WHISPER)
        include_directories(${WHISPER_INCLUDE_DIR})
    endif()
endif()

# Try to find DeepSpeech library
option(USE_DEEPSPEECH "Use Mozilla DeepSpeech for speech recognition" ON)
if(USE_DEEPSPEECH)
    # Set DeepSpeech paths - try to find in common locations
    set(DEEPSPEECH_ROOT_DIR "" CACHE PATH "Root directory for DeepSpeech library")
    
    # Look in common locations if not specified
    if(NOT DEEPSPEECH_ROOT_DIR)
        # Windows common locations
        if(WIN32)
            list(APPEND DEEPSPEECH_SEARCH_PATHS
                "C:/Program Files/DeepSpeech"
                "C:/Program Files (x86)/DeepSpeech"
                "C:/DeepSpeech"
                "${CMAKE_SOURCE_DIR}/third_party/deepspeech"
            )
        endif()
        
        # Linux/macOS common locations
        if(UNIX)
            list(APPEND DEEPSPEECH_SEARCH_PATHS
                "/usr/local/include/deepspeech"
                "/usr/include/deepspeech"
                "${CMAKE_SOURCE_DIR}/third_party/deepspeech"
                "$ENV{HOME}/deepspeech"
            )
        endif()
        
        # Try to find deepspeech.h
        find_path(DEEPSPEECH_INCLUDE_DIR deepspeech.h
            PATHS ${DEEPSPEECH_SEARCH_PATHS}
            PATH_SUFFIXES include 
        )
        
        # Try to find library
        find_library(DEEPSPEECH_LIBRARY
            NAMES deepspeech
            PATHS ${DEEPSPEECH_SEARCH_PATHS}
            PATH_SUFFIXES lib
        )
        
        if(DEEPSPEECH_INCLUDE_DIR AND DEEPSPEECH_LIBRARY)
            set(DEEPSPEECH_FOUND TRUE)
            message(STATUS "Found DeepSpeech: ${DEEPSPEECH_LIBRARY}")
            message(STATUS "DeepSpeech include: ${DEEPSPEECH_INCLUDE_DIR}")
        else()
            set(DEEPSPEECH_FOUND FALSE)
            message(STATUS "DeepSpeech not found, this recognition option will be disabled")
        endif()
    endif()
    
    if(DEEPSPEECH_FOUND)
        add_definitions(-DHAVE_DEEPSPEECH)
        include_directories(${DEEPSPEECH_INCLUDE_DIR})
    endif()
endif()

# Add include directories
include_directories(${CMAKE_SOURCE_DIR})

# Recognition service files
set(RECOGNITION_SERVICES
    src/core/recognition_services/RecognitionService.cpp
    src/core/recognition_services/WhisperRecognitionService.cpp
    src/core/recognition_services/GoogleRecognitionService.cpp
    src/core/recognition_services/AzureRecognitionService.cpp
    src/core/recognition_services/YandexRecognitionService.cpp
    src/core/recognition_services/DeepSpeechRecognitionService.cpp
)

# Settings tabs files
set(SETTINGS_TABS
    # These will be implemented in the next phase
    # src/gui/settings_tabs/GeneralTab.cpp
    # src/gui/settings_tabs/AudioTab.cpp
    # src/gui/settings_tabs/LanguageTab.cpp
    # src/gui/settings_tabs/HotkeysTab.cpp
    # src/gui/settings_tabs/AdvancedTab.cpp
)

# UI components files
set(UI_COMPONENTS
    # These will be implemented in the next phase
    # src/gui/ui_components/RecordingControls.cpp
    # src/gui/ui_components/HistoryPanel.cpp
    # src/gui/ui_components/StatusIndicator.cpp
)

# Main sources
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
    ${RECOGNITION_SERVICES}
    ${SETTINGS_TABS}
    ${UI_COMPONENTS}
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
    src/core/recognition_services/RecognitionService.h
    src/core/recognition_services/WhisperRecognitionService.h
    src/core/recognition_services/GoogleRecognitionService.h
    src/core/recognition_services/AzureRecognitionService.h
    src/core/recognition_services/YandexRecognitionService.h
    src/core/recognition_services/DeepSpeechRecognitionService.h
    # These will be implemented in the next phase
    # src/gui/settings_tabs/GeneralTab.h
    # src/gui/settings_tabs/AudioTab.h
    # src/gui/settings_tabs/LanguageTab.h
    # src/gui/settings_tabs/HotkeysTab.h
    # src/gui/settings_tabs/AdvancedTab.h
    # src/gui/ui_components/RecordingControls.h
    # src/gui/ui_components/HistoryPanel.h
    # src/gui/ui_components/StatusIndicator.h
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
    Qt6::Network
    nlohmann_json::nlohmann_json
)

# Link Whisper if found
if(WHISPER_FOUND)
    target_link_libraries(${PROJECT_NAME} PRIVATE ${WHISPER_LIBRARY})
endif()

# Link DeepSpeech if found
if(DEEPSPEECH_FOUND)
    target_link_libraries(${PROJECT_NAME} PRIVATE ${DEEPSPEECH_LIBRARY})
endif()

# Install target
install(TARGETS ${PROJECT_NAME} DESTINATION bin)

# Install models directory
install(DIRECTORY ${CMAKE_SOURCE_DIR}/models DESTINATION ${CMAKE_INSTALL_PREFIX} OPTIONAL)

# Copy resources
file(COPY ${CMAKE_SOURCE_DIR}/resources DESTINATION ${CMAKE_BINARY_DIR})

# Force resource rebuild
# set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/resources/Icon/app.ico")
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/resources/resources.qrc")

# Create models directory for whisper models
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/models)

# Platform specific settings
if(WIN32)
    set_target_properties(${PROJECT_NAME} PROPERTIES WIN32_EXECUTABLE TRUE)
    target_link_libraries(${PROJECT_NAME} PRIVATE winmm)
    
    # Add Windows executable properties
    set_target_properties(${PROJECT_NAME} PROPERTIES
        VS_DPI_AWARE "PerMonitor"
        WIN32_EXECUTABLE TRUE
    )
    
    # Configure app icon for Windows
    configure_file(${CMAKE_SOURCE_DIR}/resources/icons/app.png ${CMAKE_BINARY_DIR}/app.png COPYONLY)
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

# Output configuration information
message(STATUS "Project configuration:")
message(STATUS "  CXX Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "  Qt version: ${Qt6_VERSION}")
message(STATUS "  Whisper support: ${WHISPER_FOUND}")
message(STATUS "  DeepSpeech support: ${DEEPSPEECH_FOUND}")

# Create models directory with README
if(NOT EXISTS ${CMAKE_SOURCE_DIR}/models)
    file(MAKE_DIRECTORY ${CMAKE_SOURCE_DIR}/models)
    file(WRITE ${CMAKE_SOURCE_DIR}/models/README.txt "Place models here.\n\nRecommended models:\n- ggml-base-en.bin for Whisper English\n- ggml-base-ru.bin for Whisper Russian\n- deepspeech-0.9.3-models.pbmm for DeepSpeech English\n\nWhisper models can be downloaded from: https://huggingface.co/ggerganov/whisper.cpp\nDeepSpeech models can be downloaded from: https://github.com/mozilla/DeepSpeech/releases")
endif() 