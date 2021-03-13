# Depending on the platform (amd64 - Simulation, or ppc64le - Runtime),
# libocxl may come from the OS or from ocse
find_package(ocxl QUIET)

if ((NOT OCXL_INCLUDE_DIR) OR (NOT EXISTS ${OCXL_INCLUDE_DIR}))
    message("Unable to find libocxl. Building ocse from source.")

    # We have a submodule for ocse (libocxl). Clone it now.
    execute_process(COMMAND git submodule update --init -- ocse
                    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

    set(OCXL_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/ocse/libocxl
        CACHE PATH "ocse (libocxl) include directory" FORCE)

    set(BUILD_OCSE ON CACHE BOOL "Build ocse from source." FORCE)
endif()

if (${BUILD_OCSE})
    project(ocxl)

    CHECK_INCLUDE_FILE(misc/ocxl.h HAVE_MISC_OCXL_H)
    IF(NOT HAVE_MISC_OCXL_H)
        file(DOWNLOAD
            https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git/plain/include/uapi/misc/ocxl.h
            ${CMAKE_CURRENT_SOURCE_DIR}/ocse/common/misc/ocxl.h
        )
    ENDIF()

    add_library(ocxl
        ${CMAKE_CURRENT_SOURCE_DIR}/ocse/common/debug.c
        ${CMAKE_CURRENT_SOURCE_DIR}/ocse/common/debug.h
        ${CMAKE_CURRENT_SOURCE_DIR}/ocse/common/utils.c
        ${CMAKE_CURRENT_SOURCE_DIR}/ocse/common/utils.h
        ${CMAKE_CURRENT_SOURCE_DIR}/ocse/libocxl/libocxl.c
        ${CMAKE_CURRENT_SOURCE_DIR}/ocse/libocxl/libocxl.h
    )

    target_include_directories(ocxl
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/ocse/common
        ${CMAKE_CURRENT_SOURCE_DIR}/ocse/libocxl

        INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/ocse/common>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/ocse/libocxl>
        $<INSTALL_INTERFACE:include>
    )

    target_link_options(ocxl
        PRIVATE
        -Wl,--version-script ${CMAKE_CURRENT_SOURCE_DIR}/ocse/libocxl/symver.map
    )

    add_library(ocxl::ocxl ALIAS ocxl)

    # need to export target as well
    install(TARGETS ocxl EXPORT ocxl-export DESTINATION "${CMAKE_INSTALL_LIBDIR}")

    # CMake config
    install(EXPORT ocxl-export
        NAMESPACE   ocxl::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/ocxl
        COMPONENT   dev
    )

    # Export library for downstream projects
    export(TARGETS ocxl NAMESPACE ocxl:: FILE share/ocxl/cmake/ocxl/ocxl-export.cmake)

    # Install cmake find script for the project
    install(FILES ocxl-config.cmake DESTINATION share/ocxl COMPONENT dev)
endif()
