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
    lmdb
)


include_directories(
    ${LMDB_INCLUDE_DIR}
)

add_library(libmetal STATIC ${SOURCES})
target_link_libraries(libmetal ${LIBRARIES})
