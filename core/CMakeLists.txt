add_library(solix_core
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
    src/runtime/shared_library.cpp
    src/runtime/native_registry.cpp
)

add_library(solix_language ALIAS solix_core)

target_include_directories(solix_core
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(solix_core PUBLIC
    $<IF:$<TARGET_EXISTS:spdlog::spdlog>,spdlog::spdlog,spdlog>
    $<IF:$<TARGET_EXISTS:fmt::fmt>,fmt::fmt,fmt>
    ${CMAKE_DL_LIBS}
)

set_target_properties(solix_core PROPERTIES POSITION_INDEPENDENT_CODE ON)


