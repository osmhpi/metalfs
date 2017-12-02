cmake_minimum_required(VERSION 2.6)

project(accfs)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")

set(CMAKE_C_FLAGS "-Wall -Wextra -std=c11")

find_package(FUSE REQUIRED)
find_package(Threads REQUIRED)

option(WITH_SNAP "Build with snap" ON)

if(WITH_SNAP)
    add_definitions(-DWITH_SNAP)
    find_package(snap REQUIRED)
    find_package(cxl REQUIRED)
endif()

SET(AFU_AGENT_SOURCES
    src/common/buffer.c

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

    src/fs/accfs.c
    src/fs/server.c
    src/fs/afu.c

    src/fs/list/list.c
)

if(WITH_SNAP)
    SET(ACCFS_SOURCES
        {ACCFS_SOURCES}
        
        src/fs/actions/blowfish/action_blowfish.c
        src/fs/actions/blowfish/snap_blowfish.c
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
target_link_libraries(accfs ${ACCFS_LIBRARIES})
