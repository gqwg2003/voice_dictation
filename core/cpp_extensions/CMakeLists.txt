cmake_minimum_required(VERSION 3.12)
project(voice_dictation_cpp)

# Установка флагов компиляции для оптимизации и поддержки C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Флаги оптимизации в релизной сборке
if(CMAKE_BUILD_TYPE MATCHES "Release")
    if(MSVC)
        set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /O2")
    else()
        set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")
    endif()
endif()

# Find Python and PyBind11
find_package(Python COMPONENTS Interpreter Development REQUIRED)
find_package(pybind11 REQUIRED)

# Получение абсолютного пути к рабочему пространству
get_filename_component(WORKSPACE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../.." ABSOLUTE)
message(STATUS "Workspace directory: ${WORKSPACE_DIR}")

# Исходные файлы C++
add_library(audio_processor MODULE audio_processor.cpp)
add_library(text_processor MODULE text_processor.cpp)
add_library(speech_preprocessor MODULE speech_preprocessor.cpp)

# Настройка выходных данных для соответствия структуре модулей Python
set_target_properties(audio_processor PROPERTIES
    PREFIX ""
    OUTPUT_NAME "audio_processor"
    SUFFIX ".pyd"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
)

set_target_properties(text_processor PROPERTIES
    PREFIX ""
    OUTPUT_NAME "text_processor"
    SUFFIX ".pyd"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
)

set_target_properties(speech_preprocessor PROPERTIES
    PREFIX ""
    OUTPUT_NAME "speech_preprocessor"
    SUFFIX ".pyd"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
)

# Настройки для Unix-систем
if(UNIX)
    set_target_properties(audio_processor PROPERTIES SUFFIX ".so")
    set_target_properties(text_processor PROPERTIES SUFFIX ".so")
    set_target_properties(speech_preprocessor PROPERTIES SUFFIX ".so")
endif()

# Линковка с PyBind11
target_link_libraries(audio_processor PRIVATE pybind11::module)
target_link_libraries(text_processor PRIVATE pybind11::module)
target_link_libraries(speech_preprocessor PRIVATE pybind11::module)

# Добавление директорий с заголовочными файлами Python
target_include_directories(audio_processor PRIVATE ${Python_INCLUDE_DIRS})
target_include_directories(text_processor PRIVATE ${Python_INCLUDE_DIRS})
target_include_directories(speech_preprocessor PRIVATE ${Python_INCLUDE_DIRS})

# Указание warning уровня
if(MSVC)
    target_compile_options(audio_processor PRIVATE /W3)
    target_compile_options(text_processor PRIVATE /W3)
    target_compile_options(speech_preprocessor PRIVATE /W3)
else()
    target_compile_options(audio_processor PRIVATE -Wall -Wextra)
    target_compile_options(text_processor PRIVATE -Wall -Wextra)
    target_compile_options(speech_preprocessor PRIVATE -Wall -Wextra)
endif()

# Копирование собранных модулей
add_custom_command(TARGET audio_processor POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory ${WORKSPACE_DIR}/core/cpp_extensions/build/Release
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:audio_processor> ${WORKSPACE_DIR}/core/cpp_extensions/build/Release/
)

add_custom_command(TARGET text_processor POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory ${WORKSPACE_DIR}/core/cpp_extensions/build/Release
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:text_processor> ${WORKSPACE_DIR}/core/cpp_extensions/build/Release/
)

add_custom_command(TARGET speech_preprocessor POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory ${WORKSPACE_DIR}/core/cpp_extensions/build/Release
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:speech_preprocessor> ${WORKSPACE_DIR}/core/cpp_extensions/build/Release/
)

message(STATUS "CMake configuration completed for C++ extensions") 