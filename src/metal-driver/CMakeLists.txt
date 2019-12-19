
#
# External dependencies
#

find_package(FUSE REQUIRED)

#
# Executable name and options
#

# Target name
set(target metal-driver)

# Exit here if required dependencies are not met
message(STATUS "Executable ${target}")


#
# Sources
#

set(sources
    agent_pool.cpp
    agent_pool.hpp
    metal_fs.cpp
    metal_fuse_operations.cpp
    metal_fuse_operations.hpp
    pipeline_builder.cpp
    pipeline_builder.hpp
    pipeline_loop.cpp
    pipeline_loop.hpp
    registered_agent.hpp
    server.cpp
    server.hpp
)


#
# Create executable
#

# Build executable
add_executable(${target}
    MACOSX_BUNDLE
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
    ${PROJECT_SOURCE_DIR}/targets/SNAP/_internal/include
    ${PROJECT_SOURCE_DIR}/third_party/spdlog
    ${PROJECT_SOURCE_DIR}/third_party/cxxopts/include
)


#
# Libraries
#

target_link_libraries(${target}
    PRIVATE
    ${DEFAULT_LIBRARIES}
    ${FUSE_LIBRARIES}
    ${META_PROJECT_NAME}::metal-pipeline
    ${META_PROJECT_NAME}::metal-filesystem
    ${META_PROJECT_NAME}::metal-filesystem-pipeline
    ${META_PROJECT_NAME}::metal-driver-messages
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
    -D_FILE_OFFSET_BITS=64
)


#
# Linker options
#

target_link_libraries(${target}
    PRIVATE
    ${DEFAULT_LINKER_OPTIONS}
)

# Build dependency on metal-driver-placeholder
add_dependencies(${target} ${META_PROJECT_NAME}::metal-driver-placeholder)

#
# Deployment
#

# Executable
install(TARGETS ${target}
    RUNTIME DESTINATION ${INSTALL_BIN} COMPONENT runtime
    BUNDLE  DESTINATION ${INSTALL_BIN} COMPONENT runtime
)
