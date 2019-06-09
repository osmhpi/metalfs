
# Required for FUSE
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_FILE_OFFSET_BITS=64")

include_directories(
    ${FUSE_INCLUDE_DIR}
    # ${SNAP_INCLUDE_DIR}
    # ${CXL_INCLUDE_DIR}
    ${PROTOBUF_INCLUDE_DIRS}
    ${PROJECT_SOURCE_DIR}/third_party/spdlog
    ${PROJECT_SOURCE_DIR}/third_party/cxxopts/include
)

SET(METAL_FS_SOURCES

    metal_fs.cpp
    metal_fuse_operations.cpp
    metal_fuse_operations.hpp
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

    ${SNAP_LIBRARY}
    ${CXL_LIBRARY}
)

add_executable(metal_fs
    ${METAL_FS_SOURCES}
)

target_link_libraries(metal_fs metal_filesystem metal_pipeline metal_filesystem_pipeline messages ${METAL_FS_LIBRARIES})

add_dependencies(metal_fs metal_pipeline_test operator_agent)
