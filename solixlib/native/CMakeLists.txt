add_library(solixlib_native SHARED
    src/register.cpp
    src/console.cpp
    src/string.cpp
    src/primitives.cpp
    src/math.cpp
    src/time.cpp
    src/io_fs.cpp
    src/io_stream.cpp
    src/system.cpp
    src/crypto.cpp
)




target_include_directories(solixlib_native PRIVATE
    ${CMAKE_SOURCE_DIR}/core/include
    ${CMAKE_SOURCE_DIR}/core/src
)

target_link_libraries(solixlib_native PRIVATE solix_core)

set(SOLIXLIB_OUTPUT_LIB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../project/lib")

# Output compiled library directly into solixlib/project/lib/
set_target_properties(solixlib_native PROPERTIES
    OUTPUT_NAME "solixlib_native"
    RUNTIME_OUTPUT_DIRECTORY "${SOLIXLIB_OUTPUT_LIB_DIR}"
    LIBRARY_OUTPUT_DIRECTORY "${SOLIXLIB_OUTPUT_LIB_DIR}"
    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${SOLIXLIB_OUTPUT_LIB_DIR}"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${SOLIXLIB_OUTPUT_LIB_DIR}"
    LIBRARY_OUTPUT_DIRECTORY_DEBUG "${SOLIXLIB_OUTPUT_LIB_DIR}"
    LIBRARY_OUTPUT_DIRECTORY_RELEASE "${SOLIXLIB_OUTPUT_LIB_DIR}"
)
