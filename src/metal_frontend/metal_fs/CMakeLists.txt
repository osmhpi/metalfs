
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
)

# if(WITH_SNAP)
#     SET(METAL_FS_LIBRARIES
#         ${METAL_FS_LIBRARIES}
#         ${SNAP_LIBRARY}
#         ${CXL_LIBRARY}
#     )
# endif()

add_executable(metal_fs ${METAL_FS_SOURCES})
target_link_libraries(metal_fs metal metal_afus metal_pipeline ${METAL_FS_LIBRARIES})
