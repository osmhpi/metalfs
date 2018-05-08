include_directories(
    ${FUSE_INCLUDE_DIR}
    ${SNAP_INCLUDE_DIR}
    ${CXL_INCLUDE_DIR}
    "${CMAKE_SOURCE_DIR}/include"
)

set(
    SOURCES

    # in_memory.c
    # metal_fpga.c
    metal_pipeline.c
    # hdl_example.c

    storage.h
)

set(
    METAL_STORAGE_LIBRARIES
    ${SNAP_LIBRARY}
    ${CXL_LIBRARY}
)

include_directories(
    ${SNAP_INCLUDE_DIR}
    ${CXL_INCLUDE_DIR}
)

add_library(metal_storage STATIC ${SOURCES})
target_link_libraries(metal_storage metal_operators metal_pipeline ${METAL_STORAGE_LIBRARIES})
