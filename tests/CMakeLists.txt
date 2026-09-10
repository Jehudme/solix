# Fetch Catch2
FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG        v3.5.2
)
FetchContent_MakeAvailable(Catch2)

file(GLOB_RECURSE TEST_SOURCES "src/lexer/*.cpp" "src/parser/*.cpp" "src/semantic/*.cpp")

add_executable(solix_tests
    ${TEST_SOURCES}
)

target_link_libraries(solix_tests PRIVATE Catch2::Catch2WithMain solix_language)

include(Catch)
catch_discover_tests(solix_tests)
