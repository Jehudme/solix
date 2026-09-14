add_library(solix_language
    src/lexer.cpp
    src/parser.cpp
    src/semantic.cpp
    src/compiler.cpp
    
)

target_include_directories(solix_language
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)