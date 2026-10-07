add_library(solix_lsp_core STATIC
    src/protocol.cpp
    src/transport.cpp
    src/server.cpp
)

target_include_directories(solix_lsp_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/core/include
    ${CMAKE_SOURCE_DIR}/core/src
    ${CMAKE_SOURCE_DIR}/cli/src
)

target_link_libraries(solix_lsp_core PUBLIC
    solix_core
    solix_cli_core
    nlohmann_json::nlohmann_json
)

set_target_properties(solix_lsp_core PROPERTIES POSITION_INDEPENDENT_CODE ON)

add_executable(solix-lsp
    src/main.cpp
)

target_link_libraries(solix-lsp PRIVATE solix_lsp_core)

install(
    TARGETS solix-lsp
    RUNTIME DESTINATION bin
)
