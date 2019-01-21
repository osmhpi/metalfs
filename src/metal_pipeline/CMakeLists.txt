set(
    SOURCES

    abstract_operator.hpp
    user_operator.hpp
    user_operator.cpp
    pipeline_builder.cpp
    pipeline_builder.hpp
    # pipeline.cpp
    # pipeline.hpp
    operator_template.hpp
    operator_registry.hpp
    operator_registry.cpp
    snap_action.cpp
    snap_action.hpp
    data_source.hpp
    data_sink.hpp
    operator_argument.hpp
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
