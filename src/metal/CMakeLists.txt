set(
    SOURCES
    extent.c
    inode.c
    meta.c
    metal.c
    hollow_heap.c
)

set(
    LIBRARIES
    m  # C Math
    lmdb
)


include_directories(
    ${LMDB_INCLUDE_DIR}
)

add_library(metal STATIC ${SOURCES})
target_link_libraries(metal ${LIBRARIES})
