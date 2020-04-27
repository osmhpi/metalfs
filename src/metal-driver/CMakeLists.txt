
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

add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/metal-driver-placeholder.hex
    DEPENDS $<TARGET_FILE:${META_PROJECT_NAME}::metal-driver-placeholder>
    COMMAND cat $<TARGET_FILE:${META_PROJECT_NAME}::metal-driver-placeholder> | xxd -i - ${CMAKE_CURRENT_BINARY_DIR}/metal-driver-placeholder.hex
    COMMENT "Generating binary include from ${META_PROJECT_NAME}::metal-driver-placeholder"
)

#
# Sources
#

set(sources
    ${CMAKE_CURRENT_BINARY_DIR}/metal-driver-placeholder.hex
    agent_data_sink_context.cpp
    agent_data_sink_context.hpp
    agent_data_source_context.cpp
    agent_data_source_context.hpp
    agent_pool.cpp
    agent_pool.hpp 
    client_error.hpp
    combined_fuse_handler.cpp
    combined_fuse_handler.hpp
    configured_pipeline.hpp
    filesystem_fuse_handler.cpp
    filesystem_fuse_handler.hpp
    fuse_handler.hpp
    metal_fs.cpp
    metal_fuse_operations.cpp
    metal_fuse_operations.hpp 
    operator_agent.cpp
    operator_agent.hpp
    operator_fuse_handler.cpp
    operator_fuse_handler.hpp
    pipeline_builder.cpp
    pipeline_builder.hpp
    pipeline_loop.cpp
    pipeline_loop.hpp
    pseudo_operators.cpp
    pseudo_operators.hpp
    server.cpp
    server.hpp 
    socket_fuse_handler.cpp
    socket_fuse_handler.hpp
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
    ${CMAKE_CURRENT_BINARY_DIR}
    ${PROJECT_BINARY_DIR}/src/include
    ${PROJECT_SOURCE_DIR}/targets/SNAP/_internal/include
)


#
# Libraries
#

target_link_libraries(${target}
    PRIVATE
    ${DEFAULT_LIBRARIES}
    ${FUSE_LIBRARIES}
    cxxopts
    spdlog::spdlog
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
