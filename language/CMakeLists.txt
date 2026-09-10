# Fetch spdlog
FetchContent_Declare(
    spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG        v1.13.0
)
FetchContent_MakeAvailable(spdlog)

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

target_link_libraries(solix_language PUBLIC spdlog::spdlog)
