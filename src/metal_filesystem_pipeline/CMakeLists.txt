set(
        SOURCES

        file_data_sink.cpp
        file_data_source.cpp
        file_data_sink.hpp
        file_data_source.hpp
        metal_pipeline_storage.cpp
        metal_pipeline_storage.hpp
)

set(
        METAL_FILESYSTEM_PIPELINE_LIBRARIES

)

include_directories(

        ${SNAP_INCLUDE_DIR}
        ${CMAKE_SOURCE_DIR}/include
        ${PROJECT_SOURCE_DIR}/third_party/spdlog
)

add_library(metal_filesystem_pipeline STATIC ${SOURCES})
target_link_libraries(metal_filesystem_pipeline metal_filesystem metal_pipeline ${METAL_FILESYSTEM_PIPELINE_LIBRARIES})
