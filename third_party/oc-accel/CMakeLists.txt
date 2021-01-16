# OC-Accel is always built from third_party, we don't rely on external 'SNAP_ROOT's

project(oc-accel)

if (NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/oc-accel/software/include/libosnap.h)
    # We have a submodule for oc-accel. Clone it now.
    execute_process(COMMAND git submodule update --init -- oc-accel
                    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
endif()

add_library(oc-accel
    ${CMAKE_CURRENT_SOURCE_DIR}/oc-accel/software/lib/osnap.c
    ${CMAKE_CURRENT_SOURCE_DIR}/oc-accel/software/include/libosnap.h
)

target_include_directories(oc-accel
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/oc-accel/software/include
    ${OCXL_INCLUDE_DIR}

    INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/oc-accel/software/include>
    $<INSTALL_INTERFACE:include>
)

if (${BUILD_OCSE})
    target_link_libraries(oc-accel
        PRIVATE
        ocxl::ocxl
    )
else()
    target_link_libraries(oc-accel
        PRIVATE
        ${OCXL_LIBRARY}
    )
endif()

target_compile_definitions(oc-accel
    PRIVATE
    $<$<STREQUAL:${CMAKE_SYSTEM_PROCESSOR},x86_64>:_SIM_>
)

add_library(oc-accel::oc-accel ALIAS oc-accel)

install(TARGETS oc-accel EXPORT oc-accel-export DESTINATION "${CMAKE_INSTALL_LIBDIR}")

# Header files
install(DIRECTORY
    ${CMAKE_CURRENT_SOURCE_DIR}/oc-accel/software/include/ DESTINATION ${INSTALL_INCLUDE}
    COMPONENT dev
)

# CMake config
install(EXPORT oc-accel-export
    NAMESPACE   oc-accel::
    DESTINATION share/oc-accel/cmake/oc-accel
    COMPONENT   dev
)

# Export library for downstream projects
export(TARGETS oc-accel NAMESPACE oc-accel:: FILE share/oc-accel/cmake/oc-accel/oc-accel-export.cmake)

# Install cmake find script for the project
install(FILES oc-accel-config.cmake DESTINATION share/oc-accel COMPONENT dev)
