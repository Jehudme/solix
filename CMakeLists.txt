cmake_minimum_required(VERSION 3.20)
project(solix VERSION 0.1.0 LANGUAGES CXX)

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

# Execute `solix install` on solixlib during cmake --install
install(CODE "
    # Resolve the installed solix executable path
    set(SOLIX_BIN \"\$ENV{DESTDIR}\${CMAKE_INSTALL_PREFIX}/bin/solix\")
    if(NOT EXISTS \"\${SOLIX_BIN}\")
        set(SOLIX_BIN \"$<TARGET_FILE:solix>\")
    endif()

    set(SOLIXLIB_PATH \"${CMAKE_CURRENT_SOURCE_DIR}/solixlib/project\")

    message(STATUS \"Installing standard library into Solix registry from: \${SOLIXLIB_PATH}\")
    execute_process(
        COMMAND \${SOLIX_BIN} install \${SOLIXLIB_PATH} --force
        RESULT_VARIABLE RES
    )
    if(NOT RES EQUAL 0)
        message(WARNING \"Failed to auto-install solixlib package (exit code: \${RES})\")
    endif()
")
