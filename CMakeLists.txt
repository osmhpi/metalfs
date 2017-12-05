cmake_minimum_required(VERSION 2.6)

project(accfs)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")

set(CMAKE_C_FLAGS "-Wall -Wextra -std=c11")

find_package(FUSE REQUIRED)
find_package(Threads REQUIRED)
find_package(sqlite3 REQUIRED)

option(WITH_SNAP "Build with snap" ON)

if(WITH_SNAP)
    add_definitions(-DWITH_SNAP)
    find_package(snap REQUIRED)
    find_package(cxl REQUIRED)
endif()

add_subdirectory(src)
