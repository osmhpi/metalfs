cmake_minimum_required(VERSION 2.6)

project(metal_fs)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}) # Put binaries into root of build tree

set(CMAKE_C_FLAGS "-Wall -Wextra -std=gnu11")
set(CMAKE_CXX_FLAGS "-std=c++17")

find_package(FUSE REQUIRED)
find_package(Threads REQUIRED)
find_package(LMDB REQUIRED)
find_package(Protobuf REQUIRED)
find_package(jq REQUIRED)
find_package(snap REQUIRED)
find_package(cxl REQUIRED)

add_subdirectory(src)
add_subdirectory(test)

add_subdirectory(third_party/googletest)
add_subdirectory(third_party/cxxopts)
