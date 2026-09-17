add_library(solix_language
    src/utilities/diagnostic.cpp
    src/processes/lexer.cpp
    src/processes/parser.cpp
    src/processes/binder.cpp
    src/processes/assembler.cpp
    src/processes/runtime.cpp
)

target_include_directories(solix_language
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(solix_language PUBLIC spdlog fmt)
