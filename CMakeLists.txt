cmake_minimum_required(VERSION 3.20)
project(solix VERSION 2.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(WIN32)
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS NOMINMAX WIN32_LEAN_AND_MEAN)
endif()

add_compile_definitions(SOLIX_VERSION="${PROJECT_VERSION}")

# Compiler warning options
if(MSVC)
    add_compile_options(/W4 /permissive-)
else()
    add_compile_options(-Wall -Wextra)
endif()

# Optional Sanitizers
option(ENABLE_SANITIZERS "Enable Address and UndefinedBehavior Sanitizers" OFF)
if(ENABLE_SANITIZERS)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang|AppleClang")
        message(STATUS "Enabling AddressSanitizer and UndefinedBehaviorSanitizer")
        add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer)
        add_link_options(-fsanitize=address,undefined)
    endif()
endif()


include(FetchContent)

# Enable testing
enable_testing()

find_package(fmt QUIET)
if(NOT TARGET fmt AND NOT TARGET fmt::fmt)
    FetchContent_Declare(
        fmt
        GIT_REPOSITORY https://github.com/fmtlib/fmt.git
        GIT_TAG        11.1.4
    )
    FetchContent_MakeAvailable(fmt)
endif()

set(SPDLOG_FMT_EXTERNAL ON CACHE BOOL "Use external fmt" FORCE)

find_package(spdlog QUIET)
if(NOT TARGET spdlog AND NOT TARGET spdlog::spdlog)
    FetchContent_Declare(
        spdlog
        GIT_REPOSITORY https://github.com/gabime/spdlog.git
        GIT_TAG        v1.15.1
    )
    FetchContent_MakeAvailable(spdlog)
endif()

# Fetch miniz for cross-platform ZIP handling (.slxpac)
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_FUZZERS OFF CACHE BOOL "" FORCE)
set(INSTALL_PROJECT OFF CACHE BOOL "" FORCE)
set(AMALGAMATE_SOURCES OFF CACHE BOOL "" FORCE)

find_package(miniz QUIET)
if(NOT TARGET miniz AND NOT TARGET miniz::miniz)
    FetchContent_Declare(
        miniz
        GIT_REPOSITORY https://github.com/richgel999/miniz.git
        GIT_TAG        3.1.2
    )
    FetchContent_MakeAvailable(miniz)
endif()

if(TARGET miniz)
    set_target_properties(miniz PROPERTIES POSITION_INDEPENDENT_CODE ON)
    if(MSVC)
        target_compile_options(miniz PRIVATE /W0)
    else()
        target_compile_options(miniz PRIVATE -w)
    endif()
endif()

add_subdirectory(core)
add_subdirectory(tools/lsp)
add_subdirectory(tools/cli)
add_subdirectory(libraries/solixlib/native)
add_subdirectory(libraries/raylib/native)
if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    add_subdirectory(tests)
endif()

# Install standard library into share/solix/solixlib
install(
    DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/libraries/solixlib/"
    DESTINATION share/solix/solixlib
    COMPONENT solixlib
    FILES_MATCHING
    PATTERN "*.slx"
    PATTERN "manifest.json"
    PATTERN "solix.json"
    PATTERN "lib/*"
    PATTERN "resources/*"
    PATTERN "native" EXCLUDE
    PATTERN "build" EXCLUDE
    PATTERN "scripts" EXCLUDE
)

install(
    TARGETS solixlib_native
    LIBRARY DESTINATION share/solix/solixlib/lib
    RUNTIME DESTINATION share/solix/solixlib/lib
    ARCHIVE DESTINATION share/solix/solixlib/lib
    COMPONENT solixlib
)

install(
    TARGETS solix_raylib
    LIBRARY DESTINATION share/solix/raylib/lib
    RUNTIME DESTINATION share/solix/raylib/lib
    ARCHIVE DESTINATION share/solix/raylib/lib
    COMPONENT raylib
)

# Install official raylib companion library into share/solix/raylib
install(
    DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/libraries/raylib/"
    DESTINATION share/solix/raylib
    COMPONENT raylib
    FILES_MATCHING
    PATTERN "*.slx"
    PATTERN "manifest.json"
    PATTERN "lib/*"
    PATTERN "resources/*"
    PATTERN "native" EXCLUDE
    PATTERN "build" EXCLUDE
    PATTERN "scripts" EXCLUDE
    PATTERN "examples" EXCLUDE
)

# Install VS Code extension bundle if present
file(GLOB SOLIX_VSIX_FILES "${CMAKE_CURRENT_SOURCE_DIR}/intergrations/vscode/solix-*.vsix")
if(SOLIX_VSIX_FILES)
    install(
        FILES ${SOLIX_VSIX_FILES}
        DESTINATION share/solix/vscode
        COMPONENT ide
    )
endif()

# Documentation and License
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
    install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE" DESTINATION share/doc/solix COMPONENT doc)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/README.md")
    install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/README.md" DESTINATION share/doc/solix COMPONENT doc)
endif()

# CPack Packaging Configuration
set(CPACK_PACKAGE_NAME "solix")
set(CPACK_PACKAGE_VENDOR "Solix Team")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Solix Programming Language Compiler, Runtime, and Toolchain")
set(CPACK_PACKAGE_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${PROJECT_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${PROJECT_VERSION_PATCH})
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_HOMEPAGE_URL "https://github.com/Jehudme/solix")
set(CPACK_PACKAGE_CONTACT "Jehudme <info@solix-lang.org>")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE.txt")
set(CPACK_MONOLITHIC_INSTALL ON)

if(WIN32)
    set(CPACK_GENERATOR "ZIP;NSIS")
    set(CPACK_NSIS_MODIFY_PATH ON)
    set(CPACK_NSIS_DISPLAY_NAME "Solix Programming Language")
    set(CPACK_NSIS_PACKAGE_NAME "Solix")
    set(CPACK_NSIS_HELP_LINK "${CPACK_PACKAGE_HOMEPAGE_URL}")
    set(CPACK_NSIS_URL_INFO_ABOUT "${CPACK_PACKAGE_HOMEPAGE_URL}")
    set(CPACK_NSIS_EXTRA_INSTALL_COMMANDS "
        ExecWait '\\\"$INSTDIR\\\\bin\\\\solix.exe\\\" install --archive \\\"$INSTDIR\\\\share\\\\solix\\\\solixlib\\\"'
        ExecWait '\\\"$INSTDIR\\\\bin\\\\solix.exe\\\" install --archive \\\"$INSTDIR\\\\share\\\\solix\\\\raylib\\\"'
    ")
elseif(APPLE)
    set(CPACK_GENERATOR "TGZ;productbuild")
    set(CPACK_PACKAGE_FILE_NAME "solix-${PROJECT_VERSION}-Darwin")
else()
    set(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/README.md")
    set(CPACK_GENERATOR "TGZ;DEB;RPM")
    # Debian package settings
    set(CPACK_DEBIAN_PACKAGE_MAINTAINER "${CPACK_PACKAGE_CONTACT}")
    set(CPACK_DEBIAN_PACKAGE_SECTION "devel")
    set(CPACK_DEBIAN_PACKAGE_HOMEPAGE "${CPACK_PACKAGE_HOMEPAGE_URL}")
    set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS OFF)
    # Generate Debian postinst script to auto-install companion libraries via solix install --archive
    file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/cpack_debian_postinst"
"#!/bin/sh
set -e
if command -v solix >/dev/null 2>&1; then
    solix install --archive /usr/share/solix/solixlib || true
    solix install --archive /usr/share/solix/raylib || true
fi
exit 0
")
    set(CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA "${CMAKE_CURRENT_BINARY_DIR}/cpack_debian_postinst")

    # RPM package settings
    set(CPACK_RPM_PACKAGE_LICENSE "MIT")
    set(CPACK_RPM_PACKAGE_GROUP "Development/Languages")
    set(CPACK_RPM_PACKAGE_URL "${CPACK_PACKAGE_HOMEPAGE_URL}")
    file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/cpack_rpm_postinst"
"if command -v solix >/dev/null 2>&1; then
    solix install --archive /usr/share/solix/solixlib || true
    solix install --archive /usr/share/solix/raylib || true
fi
")
    set(CPACK_RPM_POST_INSTALL_SCRIPT_FILE "${CMAKE_CURRENT_BINARY_DIR}/cpack_rpm_postinst")
endif()

include(CPack)
