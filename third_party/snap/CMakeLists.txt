# SNAP is always built from third_party, we don't rely on external 'SNAP_ROOT's

project(snap)

if (NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/snap/software/include/libsnap.h)
    # We have a submodule for snap. Clone it now.
    execute_process(COMMAND git submodule update --init -- snap
                    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
endif()

add_library(snap
    ${CMAKE_CURRENT_SOURCE_DIR}/snap/software/lib/snap.c
    ${CMAKE_CURRENT_SOURCE_DIR}/snap/software/include/libsnap.h
)

target_include_directories(snap
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/snap/software/include
    ${CXL_INCLUDE_DIR}

    INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/snap/software/include>
    $<INSTALL_INTERFACE:include>
)

if (${BUILD_PSLSE})
    target_link_libraries(snap
        PRIVATE
        cxl::cxl
    )
else()
    target_link_libraries(snap
        PRIVATE
        ${CXL_LIBRARY}
    )
endif()

target_compile_definitions(snap
    PRIVATE
    $<$<STREQUAL:${CMAKE_SYSTEM_PROCESSOR},x86_64>:_SIM_>
)

add_library(snap::snap ALIAS snap)

install(TARGETS snap EXPORT snap-export DESTINATION "${CMAKE_INSTALL_LIBDIR}")

# CMake config
install(EXPORT snap-export
    NAMESPACE   snap::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/snap
    COMPONENT   dev
)

# Export library for downstream projects
export(TARGETS snap NAMESPACE snap:: FILE share/snap/cmake/snap/snap-export.cmake)

# Install cmake find script for the project
install(FILES snap-config.cmake DESTINATION share/snap COMPONENT dev)
