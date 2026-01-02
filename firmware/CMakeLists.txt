# Top-level CMakeLists.txt for ESP-IDF project
# This file tells the ESP-IDF build system where to find everything

# Minimum CMake version required by ESP-IDF v5
cmake_minimum_required(VERSION 3.16)

# Include the ESP-IDF build system
# $ENV{IDF_PATH} is an environment variable pointing to your ESP-IDF installation
include($ENV{IDF_PATH}/tools/cmake/project.cmake)

# Name of your project - this becomes the name of the compiled firmware binary
project(esp32cam_firmware)


