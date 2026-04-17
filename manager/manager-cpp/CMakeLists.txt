cmake_minimum_required(VERSION 3.16)

project(AVRRC_Manager VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Enable Qt's automatic resource/UI compilers
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# Find necessary Qt6 packages
find_package(Qt6 REQUIRED COMPONENTS Widgets SerialPort)

# Define source files relative to the src directory
set(PROJECT_SOURCES
    src/main.cpp
    src/mainwindow.cpp
    src/mainwindow.h
    src/mainwindow.ui
    src/fleetmanager.cpp
    src/fleetmanager.h
    src/model_settings.h
)

# Create the executable binary
add_executable(AVRRC_Manager ${PROJECT_SOURCES})

# Link the Qt libraries
target_link_libraries(AVRRC_Manager PRIVATE Qt6::Widgets Qt6::SerialPort)

# Include the src directory so headers can find each other
target_include_directories(AVRRC_Manager PRIVATE src)

# Optional: Install rule for Linux
install(TARGETS AVRRC_Manager RUNTIME DESTINATION bin)
