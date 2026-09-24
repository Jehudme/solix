# Fetch Catch2
FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG        v3.5.2
)
FetchContent_MakeAvailable(Catch2)

file(GLOB_RECURSE TEST_SOURCES "src/*.cpp")

add_executable(solix_tests
    ${TEST_SOURCES}
)

target_link_libraries(solix_tests PRIVATE Catch2::Catch2WithMain solix_language
    $<IF:$<TARGET_EXISTS:spdlog::spdlog>,spdlog::spdlog,spdlog>
    $<IF:$<TARGET_EXISTS:fmt::fmt>,fmt::fmt,fmt>
)

include(Catch)
catch_discover_tests(solix_tests)
target_sources(solix_tests PRIVATE src/test_templates.cpp)
target_sources(solix_tests PRIVATE src/test_phase20.cpp)
target_sources(solix_tests PRIVATE src/test_phase21.cpp)
target_sources(solix_tests PRIVATE src/test_phase22.cpp)
target_sources(solix_tests PRIVATE src/test_phase23.cpp)
target_sources(solix_tests PRIVATE src/test_phase24.cpp)
target_sources(solix_tests PRIVATE src/test_phase25.cpp)
target_sources(solix_tests PRIVATE src/test_phase26.cpp)
target_sources(solix_tests PRIVATE src/test_phase27.cpp)
target_sources(solix_tests PRIVATE src/test_phase28.cpp)
target_sources(solix_tests PRIVATE src/test_phase29.cpp)
target_sources(solix_tests PRIVATE src/test_phase30.cpp)
target_sources(solix_tests PRIVATE src/test_phase31.cpp)
target_sources(solix_tests PRIVATE src/test_phase32.cpp)
target_sources(solix_tests PRIVATE src/test_phase33.cpp)
target_sources(solix_tests PRIVATE src/test_phase34.cpp)
