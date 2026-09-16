add_library(solix_language
    src/lexer.cpp
    src/parser.cpp
    src/semantic.cpp
    src/compiler.cpp
    src/new/utilities/diagnostic.cpp
    src/compilation.cpp
    src/process.cpp
    src/new/processes/lexer.cpp
    src/new/processes/parser.cpp
    src/new/processes/binder.cpp
    src/runtime.cpp
)

target_include_directories(solix_language
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(solix_language PUBLIC spdlog fmt)
