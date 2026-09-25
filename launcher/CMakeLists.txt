# Fetch CLI11
FetchContent_Declare(
    CLI11
    GIT_REPOSITORY https://github.com/CLIUtils/CLI11.git
    GIT_TAG        v2.4.1
)
FetchContent_MakeAvailable(CLI11)

add_executable(solix
    src/main.cpp
    src/commands/compile.cpp
    src/commands/execute.cpp
    src/commands/install.cpp
)

target_link_libraries(solix PRIVATE solix_language CLI11::CLI11)

add_custom_target(copy_stdlib ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/rsc/lib
        $<TARGET_FILE_DIR:solix>/lib
    COMMENT "Copying Solix library folder to launcher binary directory"
)

add_dependencies(solix copy_stdlib)

# Provide backward-compatible binary link/alias for solix_launcher
if(WIN32)
    add_custom_target(solix_launcher ALL
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:solix>
            $<TARGET_FILE_DIR:solix>/solix_launcher.exe
        DEPENDS solix
    )
else()
    add_custom_target(solix_launcher ALL
        COMMAND ${CMAKE_COMMAND} -E create_symlink
            $<TARGET_FILE_NAME:solix>
            $<TARGET_FILE_DIR:solix>/solix_launcher
        DEPENDS solix
    )
endif()
