# Fetch Catch2
FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG        v3.5.2
)
FetchContent_MakeAvailable(Catch2)

add_library(solix_test_plugin SHARED
    runtime/fixtures/test_plugin.cpp
)
target_include_directories(solix_test_plugin PRIVATE
    ${CMAKE_SOURCE_DIR}/core/include
    ${CMAKE_SOURCE_DIR}/core/src
)

add_library(solix_test_plugin_b SHARED
    runtime/fixtures/test_plugin_b.cpp
)
target_include_directories(solix_test_plugin_b PRIVATE
    ${CMAKE_SOURCE_DIR}/core/include
    ${CMAKE_SOURCE_DIR}/core/src
)

file(GLOB_RECURSE TEST_SOURCES CONFIGURE_DEPENDS "statements/*.cpp" "commands/*.cpp" "solixlib/*.cpp")
file(GLOB TEST_SOURCES_RUNTIME CONFIGURE_DEPENDS "runtime/*.cpp")
list(APPEND TEST_SOURCES ${TEST_SOURCES_RUNTIME})

add_executable(solix_tests
    ${TEST_SOURCES}
)
add_dependencies(solix_tests solix_test_plugin solix_test_plugin_b)

target_link_libraries(solix_tests PRIVATE Catch2::Catch2WithMain solix_core solix_cli_core
    $<IF:$<TARGET_EXISTS:spdlog::spdlog>,spdlog::spdlog,spdlog>
    $<IF:$<TARGET_EXISTS:fmt::fmt>,fmt::fmt,fmt>
)

target_include_directories(solix_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/core/include
    ${CMAKE_SOURCE_DIR}/core/src
    ${CMAKE_SOURCE_DIR}/cli/src
    ${CMAKE_SOURCE_DIR}/cli/src/commands
)

target_compile_definitions(solix_tests PRIVATE
    SOLIX_PROJECT_ROOT="${CMAKE_SOURCE_DIR}"
    SOLIX_TEST_PLUGIN_PATH="${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_SHARED_LIBRARY_PREFIX}solix_test_plugin${CMAKE_SHARED_LIBRARY_SUFFIX}"
    SOLIX_TEST_PLUGIN_B_PATH="${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_SHARED_LIBRARY_PREFIX}solix_test_plugin_b${CMAKE_SHARED_LIBRARY_SUFFIX}"
)

include(Catch)
catch_discover_tests(solix_tests)
