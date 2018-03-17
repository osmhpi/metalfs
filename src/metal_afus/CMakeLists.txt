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
    ${SNAP_LIBRARY}
)

include_directories(
    ${SNAP_INCLUDE_DIR}
    ${CMAKE_SOURCE_DIR}/include
)

add_library(metal_afus STATIC ${SOURCES})
target_link_libraries(metal_afus ${METAL_AFUS_LIBRARIES})
