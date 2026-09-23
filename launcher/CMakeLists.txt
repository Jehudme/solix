# Fetch CLI11
FetchContent_Declare(
    CLI11
    GIT_REPOSITORY https://github.com/CLIUtils/CLI11.git
    GIT_TAG        v2.4.1
)
FetchContent_MakeAvailable(CLI11)

add_executable(solix_launcher
    src/main.cpp
    src/commands/compile.cpp
    src/commands/execute.cpp
    src/commands/install.cpp
)

target_link_libraries(solix_launcher PRIVATE solix_language CLI11::CLI11)

add_custom_target(copy_stdlib ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/rsc/lib
        $<TARGET_FILE_DIR:solix_launcher>/lib
    COMMENT "Copying Solix library folder to launcher binary directory"
)

add_dependencies(solix_launcher copy_stdlib)
