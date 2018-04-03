
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
)

add_subdirectory(metal)
add_subdirectory(metal_operators)
add_subdirectory(metal_frontend)
add_subdirectory(metal_pipeline)
add_subdirectory(metal_storage)
add_subdirectory(metal_test)
