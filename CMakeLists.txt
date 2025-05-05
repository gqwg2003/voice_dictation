cmake_minimum_required(VERSION 3.14)
project(VoiceDictation VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 COMPONENTS Core Widgets Multimedia REQUIRED)

add_executable(VoiceDictation
    src/main.cpp
    src/mainwindow.cpp
    src/mainwindow.h
    src/speechrecognizer.cpp
    src/speechrecognizer.h
    resources/resources.qrc
)

target_link_libraries(VoiceDictation PRIVATE
    Qt6::Core
    Qt6::Widgets
    Qt6::Multimedia
)

target_include_directories(VoiceDictation PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

if(WIN32)
    set_target_properties(VoiceDictation PROPERTIES WIN32_EXECUTABLE TRUE)
endif() 