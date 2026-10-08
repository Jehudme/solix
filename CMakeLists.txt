cmake_minimum_required(VERSION 3.20)
project(solix VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(WIN32)
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS NOMINMAX WIN32_LEAN_AND_MEAN NOGDI)
endif()

add_compile_definitions(SOLIX_VERSION="${PROJECT_VERSION}")


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

add_subdirectory(core)
add_subdirectory(lsp)
add_subdirectory(cli)
add_subdirectory(solixlib/native)
add_subdirectory(tests)

# Install standard library into share/solix/solixlib
install(
    DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/solixlib/project/"
    DESTINATION share/solix/solixlib
    COMPONENT solixlib
    FILES_MATCHING
    PATTERN "*.slx"
    PATTERN "solix.json"
    PATTERN "lib/*"
)

# Install VS Code extension bundle if present
file(GLOB SOLIX_VSIX_FILES "${CMAKE_CURRENT_SOURCE_DIR}/editors/vscode/solix-*.vsix")
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
set(CPACK_PACKAGE_VERSION_MAJOR 1)
set(CPACK_PACKAGE_VERSION_MINOR 0)
set(CPACK_PACKAGE_VERSION_PATCH 0)
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_HOMEPAGE_URL "https://github.com/Jehudme/solix")
set(CPACK_PACKAGE_CONTACT "Jehudme <info@solix-lang.org>")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
set(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/README.md")

if(WIN32)
    set(CPACK_GENERATOR "ZIP;NSIS")
    set(CPACK_NSIS_MODIFY_PATH ON)
    set(CPACK_NSIS_DISPLAY_NAME "Solix Programming Language")
    set(CPACK_NSIS_PACKAGE_NAME "Solix")
    set(CPACK_NSIS_HELP_LINK "${CPACK_PACKAGE_HOMEPAGE_URL}")
    set(CPACK_NSIS_URL_INFO_ABOUT "${CPACK_PACKAGE_HOMEPAGE_URL}")
elseif(APPLE)
    set(CPACK_GENERATOR "TGZ;productbuild")
    set(CPACK_PACKAGE_FILE_NAME "solix-${PROJECT_VERSION}-Darwin")
else()
    set(CPACK_GENERATOR "TGZ;DEB;RPM")
    # Debian package settings
    set(CPACK_DEBIAN_PACKAGE_MAINTAINER "${CPACK_PACKAGE_CONTACT}")
    set(CPACK_DEBIAN_PACKAGE_SECTION "devel")
    set(CPACK_DEBIAN_PACKAGE_HOMEPAGE "${CPACK_PACKAGE_HOMEPAGE_URL}")
    set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS OFF)
    # RPM package settings
    set(CPACK_RPM_PACKAGE_LICENSE "MIT")
    set(CPACK_RPM_PACKAGE_GROUP "Development/Languages")
    set(CPACK_RPM_PACKAGE_URL "${CPACK_PACKAGE_HOMEPAGE_URL}")
endif()

include(CPack)
