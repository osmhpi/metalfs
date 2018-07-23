set(
    METAL_STORAGE_TEST_SOURCES

    base_test.cpp
    base_test.hpp
    storage_test.cpp

    gtest_main.cpp
)

set(
    LIBRARIES
    gtest
)


include_directories(../../third_party/googletest/googletest/)

add_executable(metal_storage_test ${METAL_STORAGE_TEST_SOURCES})
target_link_libraries(metal_storage_test metal_storage ${LIBRARIES})
