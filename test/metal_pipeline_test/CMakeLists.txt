set(
    METAL_PIPELINE_TEST_SOURCES

    operator_registry_test.cpp
    user_operator_test.cpp

    gtest_main.cpp
)

set(
    LIBRARIES
    gtest

    ${JQ_LIBRARIES}
    ${SNAP_LIBRARY}
    ${CXL_LIBRARY}
)


include_directories(
    ${PROJECT_SOURCE_DIR}/third_party/googletest/googletest/
    ${PROJECT_SOURCE_DIR}/third_party/spdlog

    ${SNAP_INCLUDE_DIR}
)

add_executable(metal_pipeline_test ${METAL_PIPELINE_TEST_SOURCES})
target_link_libraries(metal_pipeline_test metal_pipeline ${LIBRARIES})

add_custom_command(
        TARGET metal_pipeline_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/operators/
        COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/operators/*.json ${CMAKE_CURRENT_BINARY_DIR}/operators/
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/operators/*.json)
