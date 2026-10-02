# Fetch Catch2
FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG        v3.5.2
)
FetchContent_MakeAvailable(Catch2)

file(GLOB_RECURSE TEST_SOURCES CONFIGURE_DEPENDS "statements/*.cpp" "commands/*.cpp")

add_executable(solix_tests
    ${TEST_SOURCES}
)

target_link_libraries(solix_tests PRIVATE Catch2::Catch2WithMain solix_language solix_launcher_core
    $<IF:$<TARGET_EXISTS:spdlog::spdlog>,spdlog::spdlog,spdlog>
    $<IF:$<TARGET_EXISTS:fmt::fmt>,fmt::fmt,fmt>
)

target_include_directories(solix_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/language/include
    ${CMAKE_SOURCE_DIR}/language/src
    ${CMAKE_SOURCE_DIR}/launcher/src
    ${CMAKE_SOURCE_DIR}/launcher/src/commands
)

target_compile_definitions(solix_tests PRIVATE
    SOLIX_PROJECT_ROOT="${CMAKE_SOURCE_DIR}"
)

if(WIN32)
    if(MSVC)
        target_link_options(solix_tests PRIVATE "/STACK:16777216")
    else()
        target_link_options(solix_tests PRIVATE "-Wl,--stack,16777216")
    endif()
endif()

include(Catch)
catch_discover_tests(solix_tests)
