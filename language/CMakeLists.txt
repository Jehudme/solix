add_library(solix_language
    src/utilities/diagnostic.cpp
    src/compilation.cpp
    src/utilities/ast_clone.cpp
    src/processes/template_substitution.cpp
    src/processes/template_deduction.cpp
    src/processes/process.cpp
    src/processes/lexer.cpp
    src/processes/parser.cpp
    src/processes/binder.cpp
    src/processes/assembler.cpp
    src/runtime.cpp
    src/natives/natives.cpp
    src/natives/console.cpp
    src/natives/utilities.cpp
)

target_include_directories(solix_language
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(solix_language PUBLIC
    $<IF:$<TARGET_EXISTS:spdlog::spdlog>,spdlog::spdlog,spdlog>
    $<IF:$<TARGET_EXISTS:fmt::fmt>,fmt::fmt,fmt>
)
