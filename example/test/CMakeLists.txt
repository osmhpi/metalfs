
#
# External dependencies
#

#
# Executable name and options
#

# Target name
set(target example-test)
message(STATUS "Test ${target}")


#
# Sources
#

set(sources
    gtest_main.cpp

    base_test.cpp
    base_test.hpp
    blowfish_pipeline_test.cpp
    changecase_pipeline_test.cpp
    colorfilter_pipeline_test.cpp
    readwrite_pipeline_test.cpp
    nvme_pipeline_test.cpp
    dram_pipeline_test.cpp
    profiling_test.cpp
)


#
# Create executable
#

# Build executable
add_executable(${target}
    ${sources}
)

# Create namespaced alias
add_executable(${META_PROJECT_NAME}::${target} ALIAS ${target})


#
# Project options
#

set_target_properties(${target}
    PROPERTIES
    ${DEFAULT_PROJECT_OPTIONS}
    FOLDER "${IDE_FOLDER}"
)


#
# Include directories
#

target_include_directories(${target}
    PRIVATE
    ${DEFAULT_INCLUDE_DIRECTORIES}
    ${PROJECT_BINARY_DIR}/src/include
    ${PROJECT_SOURCE_DIR}/third_party/spdlog
    ${PROJECT_SOURCE_DIR}/targets/SNAP/_internal/include # todo: this does not belong here, should be public include from metal-pipeline
)


#
# Libraries
#

target_link_libraries(${target}
    PRIVATE
    ${DEFAULT_LIBRARIES}
    ${META_PROJECT_NAME}::metal-pipeline
    ${META_PROJECT_NAME}::metal-filesystem
    ${META_PROJECT_NAME}::metal-filesystem-pipeline
    ${SPDLOG_LIBRARIES}
    gtest
)


#
# Compile definitions
#

target_compile_definitions(${target}
    PRIVATE
    ${DEFAULT_COMPILE_DEFINITIONS}
)


#
# Compile options
#

target_compile_options(${target}
    PRIVATE
    ${DEFAULT_COMPILE_OPTIONS}
)


#
# Linker options
#

target_link_libraries(${target}
    PRIVATE
    ${DEFAULT_LINKER_OPTIONS}
)
