set(
    SOURCES

    pipeline.c
    pipeline.h
)

set(
    METAL_PIPELINE_LIBRARIES

    ${SNAP_LIBRARY}
    ${CXL_LIBRARY}
)

include_directories(
    ${SNAP_INCLUDE_DIR}
    ${CMAKE_SOURCE_DIR}/include
)

add_library(metal_pipeline STATIC ${SOURCES})
target_link_libraries(metal_pipeline ${METAL_PIPELINE_LIBRARIES})
