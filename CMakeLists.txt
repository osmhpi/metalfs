cmake_minimum_required(VERSION 2.6)

project(accfs)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")

set(CMAKE_C_FLAGS "-Wall -Wextra -std=c11")

find_package(FUSE REQUIRED)

add_executable(afu_agent src/afu_agent/afu_agent.c)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D_FILE_OFFSET_BITS=64")

include_directories(${FUSE_INCLUDE_DIR})
add_executable(accfs src/fs/accfs.c)
target_link_libraries(accfs ${FUSE_LIBRARIES})
