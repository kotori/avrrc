cmake_minimum_required(VERSION 3.16)
project(AVRRC_Ensign VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Enable Qt's automatic resource/UI compilers
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets SerialPort Sql Network)

# --- 1. GIT VERSIONING (Cross-Platform) ---
find_package(Git QUIET)
if(GIT_FOUND)
    execute_process(
        COMMAND ${GIT_EXECUTABLE} describe --always --tags
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        OUTPUT_VARIABLE GIT_VERSION
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
endif()

if(NOT GIT_VERSION)
    set(GIT_VERSION ${PROJECT_VERSION})
endif()
add_definitions(-DAPP_VERSION="${GIT_VERSION}")

message(STATUS "AVRRC Ensign Build Version: ${GIT_VERSION}")

# --- 2. WINDOWS SPECIFIC RESOURCES ---
if(WIN32)
    enable_language(RC)
    # Only include if the files exist in your repo
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/resources/app.rc")
        set(WIN32_RESOURCES resources/app.rc)
    endif()
endif()

set(PROJECT_SOURCES
    src/main.cpp
    src/mainwindow.cpp
    src/mainwindow.h
    src/mainwindow.ui
    src/fleetmanager.cpp
    src/fleetmanager.h
    src/model_settings.h
    ${WIN32_RESOURCES}
)

# --- 3. CREATE EXECUTABLE ---
# WIN32 flag is ignored on Linux, but required for GUI apps on Windows
add_executable(AVRRC_Ensign WIN32 ${PROJECT_SOURCES})

target_link_libraries(AVRRC_Ensign PRIVATE Qt6::Widgets Qt6::SerialPort Qt6::Sql Qt6::Network)
target_include_directories(AVRRC_Ensign PRIVATE src)

# --- 4. WINDOWS DEPLOYMENT (DLL Bundling) ---
if(WIN32)
    find_program(WINDEPLOYQT_EXECUTABLE windeployqt HINTS "${Qt6_DIR}/../../../bin")
    if(WINDEPLOYQT_EXECUTABLE)
        add_custom_command(TARGET AVRRC_Ensign POST_BUILD
            COMMAND "${WINDEPLOYQT_EXECUTABLE}" "$<TARGET_FILE:AVRRC_Ensign>"
            COMMENT "Bundling Windows DLLs..."
        )
    endif()
endif()

# --- 5. LINUX INSTALLATION ---
if(UNIX AND NOT APPLE)
    install(TARGETS AVRRC_Ensign RUNTIME DESTINATION bin)
endif()
