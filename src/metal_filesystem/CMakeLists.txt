set(
    SOURCES
    extent.c
    inode.c
    meta.c
    metal.c
    heap.c
)

set(
    LIBRARIES
    m  # C Math
    lmdb
)

include_directories(
    ${LMDB_INCLUDE_DIR}
)

add_library(metal_filesystem STATIC ${SOURCES})
target_link_libraries(metal_filesystem ${LIBRARIES})
