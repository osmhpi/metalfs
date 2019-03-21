set(
    SOURCES
    extent.c
    extent.h
    inode.c
    inode.h
    meta.c
    meta.h
    metal.c
    metal.h
    heap.c
    heap.h
    storage.h
    storage_in_memory.c
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
