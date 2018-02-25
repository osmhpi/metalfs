set(
    SOURCES

    afu_read_mem/afu.c
    afu_read_mem/afu.h
    afu_uppercase/afu.c
    afu_uppercase/afu.h
    afu_write_mem/afu.c
    afu_write_mem/afu.h

    afus.h
)

set(
    METAL_AFUS_LIBRARIES
)

include_directories(
    
)

add_library(metal_afus STATIC ${SOURCES})
target_link_libraries(metal_afus ${METAL_AFUS_LIBRARIES})
