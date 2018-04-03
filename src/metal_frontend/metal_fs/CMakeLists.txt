
# Required for FUSE
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D_FILE_OFFSET_BITS=64")

include_directories(
    ${FUSE_INCLUDE_DIR}
    # ${SNAP_INCLUDE_DIR}
    # ${CXL_INCLUDE_DIR}
)
SET(METAL_FS_SOURCES
    ${METAL_FS_SHARED_SOURCES}

    list/list.c
    metal_fs.c
    server.c
)

SET (METAL_FS_LIBRARIES
    ${FUSE_LIBRARIES}
    ${CMAKE_THREAD_LIBS_INIT}

    ${SNAP_LIBRARY}
    ${CXL_LIBRARY}
)

add_executable(metal_fs ${METAL_FS_SOURCES})
target_link_libraries(metal_fs metal metal_operators metal_pipeline ${METAL_FS_LIBRARIES})
