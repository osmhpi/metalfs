set(
    EXAMPLE_TEST_SOURCES

    base_test.cpp
    base_test.hpp
    blowfish_pipeline_test.cpp
    changecase_pipeline_test.cpp
    colorfilter_pipeline_test.cpp
    readwrite_pipeline_test.cpp
    profiling_test.cpp

    gtest_main.cpp
)

set(
    LIBRARIES
    metal_pipeline
    gtest

    ${JQ_LIBRARIES}
    ${SNAP_LIBRARY}
    ${CXL_LIBRARY}
)


include_directories(
    ${PROJECT_SOURCE_DIR}/third_party/googletest/googletest/
    ${PROJECT_SOURCE_DIR}/third_party/spdlog
    ${PROJECT_SOURCE_DIR}/src

    ${SNAP_INCLUDE_DIR}
)

add_executable(example_test ${EXAMPLE_TEST_SOURCES})
target_link_libraries(example_test ${LIBRARIES})
