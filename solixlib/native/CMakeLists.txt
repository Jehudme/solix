add_library(solixlib_native SHARED
    src/register.cpp
)

target_include_directories(solixlib_native PRIVATE
    ${CMAKE_SOURCE_DIR}/language/include
    ${CMAKE_SOURCE_DIR}/language/src
)

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
