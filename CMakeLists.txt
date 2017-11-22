cmake_minimum_required(VERSION 2.6)

project(accfs)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")

set(CMAKE_C_FLAGS "-Wall -Wextra -std=c11")

find_package(FUSE REQUIRED)
find_package (Threads REQUIRED)

add_executable(afu_agent src/afu_agent/afu_agent.c  src/common/buffer.c)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D_FILE_OFFSET_BITS=64")

include_directories(${FUSE_INCLUDE_DIR})
add_executable(accfs src/fs/accfs.c src/fs/server.c src/fs/list/list.c src/common/buffer.c src/fs/afu.c)
target_link_libraries(accfs ${FUSE_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
