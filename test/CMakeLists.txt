
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
)

add_subdirectory(metal_test)
add_subdirectory(metal_pipeline_test)
add_subdirectory(metal_storage_test)
