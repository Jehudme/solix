cmake_minimum_required(VERSION 3.20)
project(solix_raylib_native LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Fetch latest official Raylib release (v6.2) archive and build statically
include(FetchContent)
FetchContent_Declare(
    raylib
    URL https://github.com/raysan5/raylib/archive/refs/tags/6.2.tar.gz
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)

set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_GAMES OFF CACHE BOOL "" FORCE)
set(CUSTOMIZE_BUILD OFF CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(WITH_PIC ON CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(raylib)

# Locate Solix core include directory
if(NOT DEFINED SOLIX_CORE_INCLUDE_DIR)
    if(EXISTS "${CMAKE_SOURCE_DIR}/core/include")
        set(SOLIX_CORE_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/core/include")
    elseif(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../../core/include")
        set(SOLIX_CORE_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../../core/include")
    elseif(EXISTS "$ENV{HOME}/.local/include/solix")
        set(SOLIX_CORE_INCLUDE_DIR "$ENV{HOME}/.local/include/solix")
    else()
        set(SOLIX_CORE_INCLUDE_DIR "/usr/local/include/solix")
    endif()
endif()

add_library(solix_raylib SHARED
    src/generated_raylib.cpp
    src/register.cpp
)

target_include_directories(solix_raylib PRIVATE
    include
    ${raylib_SOURCE_DIR}/src
    ${SOLIX_CORE_INCLUDE_DIR}
    ${SOLIX_CORE_INCLUDE_DIR}/../src
)

target_link_libraries(solix_raylib PRIVATE raylib)

# Platform-specific link libraries for static raylib integration
if(WIN32)
    target_link_libraries(solix_raylib PRIVATE winmm gdi32 opengl32 shell32 user32)
elseif(APPLE)
    target_link_libraries(solix_raylib PRIVATE
        "-framework OpenGL"
        "-framework Cocoa"
        "-framework IOKit"
        "-framework CoreVideo"
    )
else()
    target_link_libraries(solix_raylib PRIVATE m pthread dl rt)
endif()

# Output compiled library directly into libs/raylib/lib/
set(RAYLIB_OUTPUT_LIB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../lib")

set_target_properties(solix_raylib PROPERTIES
    OUTPUT_NAME "solix_raylib"
    RUNTIME_OUTPUT_DIRECTORY "${RAYLIB_OUTPUT_LIB_DIR}"
    LIBRARY_OUTPUT_DIRECTORY "${RAYLIB_OUTPUT_LIB_DIR}"
    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${RAYLIB_OUTPUT_LIB_DIR}"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${RAYLIB_OUTPUT_LIB_DIR}"
    LIBRARY_OUTPUT_DIRECTORY_DEBUG "${RAYLIB_OUTPUT_LIB_DIR}"
    LIBRARY_OUTPUT_DIRECTORY_RELEASE "${RAYLIB_OUTPUT_LIB_DIR}"
)
