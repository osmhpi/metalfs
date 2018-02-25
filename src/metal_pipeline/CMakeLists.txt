set(
    SOURCES
    
    pipeline.c
    pipeline.h
)

set(
    METAL_PIPELINE_LIBRARIES
)

include_directories(
    
)

add_library(metal_pipeline STATIC ${SOURCES})
target_link_libraries(metal_pipeline ${METAL_PIPELINE_LIBRARIES})
