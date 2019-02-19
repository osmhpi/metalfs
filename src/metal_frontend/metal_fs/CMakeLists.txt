
# Required for FUSE
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_FILE_OFFSET_BITS=64")

include_directories(
    ${FUSE_INCLUDE_DIR}
    # ${SNAP_INCLUDE_DIR}
    # ${CXL_INCLUDE_DIR}
    ${PROTOBUF_INCLUDE_DIRS}
)

SET(METAL_FS_SOURCES
    ${METAL_FS_SHARED_SOURCES}

    metal_fs.cpp
    server.cpp
    server.hpp
    agent_pool.hpp
    agent_pool.cpp
    registered_agent.hpp
    pipeline_builder.cpp
    pipeline_builder.hpp
    pipeline_loop.cpp
    pipeline_loop.hpp
)

SET (METAL_FS_LIBRARIES
    ${FUSE_LIBRARIES}
    ${CMAKE_THREAD_LIBS_INIT}
#    ${PROTOBUF_LIBRARIES}

    ${SNAP_LIBRARY}
    ${CXL_LIBRARY}
)

add_executable(metal_fs
    ${METAL_FS_SOURCES}
)

target_link_libraries(metal_fs metal metal_operators metal_pipeline metal_filesystem_pipeline messages ${METAL_FS_LIBRARIES})
