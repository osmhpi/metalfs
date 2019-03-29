set(
    METAL_FILESYSTEM_TEST_SOURCES

    base_test.cpp
    base_test.hpp
    heap_test.cpp
    metal_test.cpp
    extent_test.cpp

    gtest_main.cpp
)

set(
    LIBRARIES
    gtest
)


include_directories(../../third_party/googletest/googletest/)

add_executable(metal_filesystem_test ${METAL_FILESYSTEM_TEST_SOURCES})
target_link_libraries(metal_filesystem_test metal_filesystem ${LIBRARIES})
