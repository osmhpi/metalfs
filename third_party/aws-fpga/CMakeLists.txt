project(fpga_mgmt)

if (NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/aws-fpga/sdk/userspace/include/fpga_mgmt.h)
    # We have a submodule for aws-fpga. Clone it now.
    execute_process(COMMAND git submodule update --init -- aws-fpga
                    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
endif()

add_library(fpga_mgmt
    ${CMAKE_CURRENT_SOURCE_DIR}/aws-fpga/sdk/userspace/fpga_libs/fpga_mgmt/afi_cmd_api.h
    ${CMAKE_CURRENT_SOURCE_DIR}/aws-fpga/sdk/userspace/fpga_libs/fpga_mgmt/fpga_hal_mbox_regs.h
    ${CMAKE_CURRENT_SOURCE_DIR}/aws-fpga/sdk/userspace/fpga_libs/fpga_mgmt/fpga_hal_mbox.h
    ${CMAKE_CURRENT_SOURCE_DIR}/aws-fpga/sdk/userspace/fpga_libs/fpga_mgmt/fpga_mgmt_internal.h

    ${CMAKE_CURRENT_SOURCE_DIR}/aws-fpga/sdk/userspace/fpga_libs/fpga_mgmt/fpga_hal_mbox.c
    ${CMAKE_CURRENT_SOURCE_DIR}/aws-fpga/sdk/userspace/fpga_libs/fpga_mgmt/fpga_mgmt_cmd.c
    ${CMAKE_CURRENT_SOURCE_DIR}/aws-fpga/sdk/userspace/fpga_libs/fpga_mgmt/fpga_mgmt.c
)

target_include_directories(fpga_mgmt
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/aws-fpga/sdk/userspace/include
    ${CMAKE_CURRENT_SOURCE_DIR}/aws-fpga/sdk/userspace/fpga_libs/fpga_mgmt

    INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/aws-fpga/sdk/userspace/include>
    $<INSTALL_INTERFACE:include>
)

# target_link_libraries(fpga_mgmt
#     PRIVATE

#     PUBLIC
#     ${DEFAULT_LIBRARIES}
#     pthread
#     rt

#     INTERFACE
# )

add_library(fpga_mgmt::fpga_mgmt ALIAS fpga_mgmt)

install(TARGETS fpga_mgmt EXPORT fpga_mgmt-export DESTINATION "${CMAKE_INSTALL_LIBDIR}")

# Header files
install(DIRECTORY
    ${CMAKE_CURRENT_SOURCE_DIR}/aws-fpga/software/include/ DESTINATION ${INSTALL_INCLUDE}
    COMPONENT dev
)

# CMake config
install(EXPORT fpga_mgmt-export
    NAMESPACE   fpga_mgmt::
    DESTINATION share/fpga_mgmt/cmake/fpga_mgmt
    COMPONENT   dev
)

# Export library for downstream projects
export(TARGETS fpga_mgmt NAMESPACE fpga_mgmt:: FILE share/fpga_mgmt/cmake/fpga_mgmt/fpga_mgmt-export.cmake)

# Install cmake find script for the project
install(FILES fpga_mgmt-config.cmake DESTINATION share/fpga_mgmt COMPONENT dev)
