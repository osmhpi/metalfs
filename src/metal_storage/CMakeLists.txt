include_directories(
    ${FUSE_INCLUDE_DIR}
    ${SNAP_INCLUDE_DIR}
    ${CXL_INCLUDE_DIR}
    "${CMAKE_SOURCE_DIR}/include"
)

set(
    SOURCES
    storage.h
)

option(METAL_USE_INMEMORY_STORAGE "Use in-memory storage backend" OFF)

if (METAL_USE_INMEMORY_STORAGE)
    set(SOURCES ${SOURCES} in_memory.c)
else()
    set(SOURCES ${SOURCES}
    
        # Choose one of the available backends
        # metal_fpga.c
        metal_pipeline.c
        # hdl_example.c
    )
endif()

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
