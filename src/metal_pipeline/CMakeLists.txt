set(
    SOURCES

    abstract_operator.cpp
    abstract_operator.hpp
    data_sink.cpp
    data_sink.hpp
    data_source.cpp
    data_source.hpp
    operator_argument.hpp
    operator_registry.cpp
    operator_registry.hpp
    pipeline_definition.cpp
    pipeline_definition.hpp
    pipeline_runner.cpp
    pipeline_runner.hpp
    snap_action.cpp
    snap_action.hpp
    user_operator.cpp
    user_operator.hpp
)

set(
    METAL_PIPELINE_LIBRARIES

    ${JQ_LIBRARIES}
    ${SNAP_LIBRARY}
    ${CXL_LIBRARY}
)

include_directories(
    ${JQ_INCLUDE_DIR}
    ${SNAP_INCLUDE_DIR}
    ${CMAKE_SOURCE_DIR}/include
)

add_library(metal_pipeline STATIC ${SOURCES})
target_link_libraries(metal_pipeline ${METAL_PIPELINE_LIBRARIES})
