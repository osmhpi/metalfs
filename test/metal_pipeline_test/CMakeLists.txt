set(
    METAL_PIPELINE_TEST_SOURCES

    base_test.cpp
    base_test.hpp
    blowfish_pipeline_test.cpp
    readwrite_pipeline_test.cpp

    gtest_main.cpp
)

set(
    LIBRARIES
    gtest

    ${SNAP_LIBRARY}
    ${CXL_LIBRARY}
)


include_directories(../../third_party/googletest/googletest/)

add_executable(metal_pipeline_test ${METAL_PIPELINE_TEST_SOURCES})
target_link_libraries(metal_pipeline_test metal_pipeline metal_operators ${LIBRARIES})
