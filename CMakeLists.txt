# Make sure we use the correct vesion
cmake_minimum_required(VERSION 3.14)

# Add the "src" directory as include directory to allow including all "hal-xxx" modules using include.
include_directories(src)

# Add the common data to the project.
add_subdirectory(src/hal-common)
add_subdirectory(src/hal-feather-m0)
add_subdirectory(src/hal-lcd-character)
add_subdirectory(src/hal-lcd-hitachi)
add_subdirectory(src/hal-mcp230xx)

# Add the toolchain projects.
add_subdirectory(hal-toolchain/arduino-core)
add_subdirectory(hal-toolchain/arduino-link)

# Name the project.
project(lcd-demo)

# Set the upload port
set(UPLOAD_PORT "/dev/cu.usbmodem141201")

# Define the library to build.
add_library(lcd-demo STATIC src/Application.cpp src/Application.hpp src/main.cpp src/Console.cpp src/Console.hpp)
target_link_libraries(lcd-demo HAL-common HAL-feather-m0 HAL-lcd-hitachi HAL-mcp230xx)

# Link the firmware
arduino_link(lcd-demo)
