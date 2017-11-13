cmake_minimum_required(VERSION 2.6)

project(accfs)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")

set(CMAKE_C_FLAGS "-Wall -Wextra -std=c11")

find_package(Fuse REQUIRED)

add_executable(accfs src/fs/accfs.c)
add_executable(afu_agent src/afu_agent/afu_agent.c)
