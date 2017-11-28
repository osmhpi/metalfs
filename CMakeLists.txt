cmake_minimum_required(VERSION 2.6)

project(accfs)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")

set(CMAKE_C_FLAGS "-Wall -Wextra -std=c11")

find_package(FUSE REQUIRED)
find_package(Threads REQUIRED)
find_package(snap REQUIRED)
find_package(cxl REQUIRED)

SET(AFU_AGENT_SOURCES
    src/common/buffer.c
    # src/common/afus.c

    src/afu_agent/afu_agent.c
)
add_executable(afu_agent ${AFU_AGENT_SOURCES})

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D_FILE_OFFSET_BITS=64")

include_directories(
    ${FUSE_INCLUDE_DIR}
    ${SNAP_INCLUDE_DIR}
    ${CXL_INCLUDE_DIR}
)
SET(ACCFS_SOURCES
    src/common/buffer.c
    # src/common/afus.c

    src/fs/accfs.c
    src/fs/server.c
    src/fs/afu.c

    src/fs/list/list.c

    src/fs/actions/blowfish/action_blowfish.c
    src/fs/actions/blowfish/snap_blowfish.c
)
add_executable(accfs ${ACCFS_SOURCES})
target_link_libraries(accfs ${FUSE_LIBRARIES} ${SNAP_LIBRARY} ${CXL_LIBRARY} ${CMAKE_THREAD_LIBS_INIT})
