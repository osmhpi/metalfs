
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D_FILE_OFFSET_BITS=64")

include_directories(
    ${FUSE_INCLUDE_DIR}
    ${SNAP_INCLUDE_DIR}
    ${CXL_INCLUDE_DIR}
)
SET(ACCFS_SOURCES
    ../common/buffer.c

    accfs.c
    server.c
    afu.c
    list/list.c
)

if(WITH_SNAP)
    SET(ACCFS_SOURCES
        ${ACCFS_SOURCES}

        actions/blowfish/action_blowfish.c
        actions/blowfish/snap_blowfish.c
    )
endif()

SET (ACCFS_LIBRARIES
    ${FUSE_LIBRARIES}
    ${CMAKE_THREAD_LIBS_INIT}
)

if(WITH_SNAP)
    SET(ACCFS_LIBRARIES
        ${ACCFS_LIBRARIES}
        ${SNAP_LIBRARY}
        ${CXL_LIBRARY}
    )
endif()

add_executable(accfs ${ACCFS_SOURCES})
target_link_libraries(accfs metal ${ACCFS_LIBRARIES})
