# Fetch CLI11
FetchContent_Declare(
    CLI11
    GIT_REPOSITORY https://github.com/CLIUtils/CLI11.git
    GIT_TAG        v2.4.1
)
FetchContent_MakeAvailable(CLI11)

# Fetch nlohmann_json
FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG        v3.11.3
)
FetchContent_MakeAvailable(nlohmann_json)

add_library(solix_cli_core STATIC
    src/package_manager.cpp
    src/commands/compile.cpp
    src/commands/execute.cpp
    src/commands/build.cpp
    src/commands/new.cpp
    src/commands/package.cpp
)

add_library(solix_launcher_core ALIAS solix_cli_core)

target_link_libraries(solix_cli_core PUBLIC solix_core CLI11::CLI11 nlohmann_json::nlohmann_json)

target_include_directories(solix_cli_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/commands
)

add_executable(solix
    src/main.cpp
)

target_link_libraries(solix PRIVATE solix_cli_core)


# Provide backward-compatible binary link/alias for solix_launcher and solix_cli
if(WIN32)
    add_custom_target(solix_launcher ALL
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:solix>
            $<TARGET_FILE_DIR:solix>/solix_launcher.exe
        DEPENDS solix
    )
    add_custom_target(solix_cli ALL
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:solix>
            $<TARGET_FILE_DIR:solix>/solix_cli.exe
        DEPENDS solix
    )
else()
    add_custom_target(solix_launcher ALL
        COMMAND ${CMAKE_COMMAND} -E create_symlink
            $<TARGET_FILE_NAME:solix>
            $<TARGET_FILE_DIR:solix>/solix_launcher
        DEPENDS solix
    )
    add_custom_target(solix_cli ALL
        COMMAND ${CMAKE_COMMAND} -E create_symlink
            $<TARGET_FILE_NAME:solix>
            $<TARGET_FILE_DIR:solix>/solix_cli
        DEPENDS solix
    )
endif()

