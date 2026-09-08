cmake_minimum_required(VERSION 3.20)
project(solix VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)

# Enable testing
enable_testing()

add_subdirectory(language)
add_subdirectory(launcher)
add_subdirectory(tests)
