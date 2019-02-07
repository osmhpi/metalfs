set(
        SOURCES

        filesystem_pipeline_runner.cpp
        file_data_sink.cpp
        file_data_source.cpp
        filesystem_pipeline_runner.hpp
        file_data_sink.hpp
        file_data_source.hpp
)

set(
        METAL_FILESYSTEM_PIPELINE_LIBRARIES

)

include_directories(

        ${CMAKE_SOURCE_DIR}/include
)

add_library(metal_filesystem_pipeline STATIC ${SOURCES})
target_link_libraries(metal_filesystem_pipeline metal ${METAL_PIPELINE_LIBRARIES})
