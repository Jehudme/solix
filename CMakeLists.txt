cmake_minimum_required(VERSION 3.20)
project(solix VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(FetchContent)

# Enable testing
enable_testing()

find_package(fmt QUIET)
if(NOT TARGET fmt AND NOT TARGET fmt::fmt)
    FetchContent_Declare(
        fmt
        GIT_REPOSITORY https://github.com/fmtlib/fmt.git
        GIT_TAG        10.2.1
    )
    FetchContent_MakeAvailable(fmt)
endif()

set(SPDLOG_FMT_EXTERNAL ON CACHE BOOL "Use external fmt" FORCE)
set(SPDLOG_NO_COMPILED_STRING ON CACHE BOOL "Disable compiled string in spdlog" FORCE)
add_compile_definitions(SPDLOG_NO_COMPILED_STRING)

find_package(spdlog QUIET)
if(NOT TARGET spdlog AND NOT TARGET spdlog::spdlog)
    FetchContent_Declare(
        spdlog
        GIT_REPOSITORY https://github.com/gabime/spdlog.git
        GIT_TAG        v1.13.0
    )
    FetchContent_MakeAvailable(spdlog)
endif()

add_subdirectory(language)
add_subdirectory(launcher)
add_subdirectory(tests)
