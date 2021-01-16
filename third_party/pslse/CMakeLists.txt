# Depending on the platform (amd64 - Simulation, or ppc64le - Runtime),
# libcxl may come from the OS or from pslse
find_package(cxl QUIET)

if ((NOT CXL_INCLUDE_DIR) OR (NOT EXISTS ${CXL_INCLUDE_DIR}))
    message("Unable to find libcxl. Building pslse from source.")

    # We have a submodule for pslse (libcxl). Clone it now.
    execute_process(COMMAND git submodule update --init -- pslse
                    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

    set(CXL_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/pslse/libcxl
        CACHE PATH "pslse (libcxl) include directory" FORCE)

    set(BUILD_PSLSE ON CACHE BOOL "Build pslse from source." FORCE)
endif()

if (${BUILD_PSLSE})
    project(cxl)

    set(PSL_VERSIONS PSL8 PSL9)
    set(PSL_VERSION PSL8 CACHE STRING "Target version of PSL")
    set_property(CACHE PSL_VERSION PROPERTY STRINGS ${PSL_VERSIONS})

    CHECK_INCLUDE_FILE(misc/cxl.h HAVE_MISC_CXL_H)
    IF(NOT HAVE_MISC_CXL_H)
        file(DOWNLOAD
            https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git/plain/include/uapi/misc/cxl.h
            ${CMAKE_CURRENT_SOURCE_DIR}/pslse/common/misc/cxl.h
        )
    ENDIF()

    add_library(cxl
        ${CMAKE_CURRENT_SOURCE_DIR}/pslse/common/debug.c
        ${CMAKE_CURRENT_SOURCE_DIR}/pslse/common/utils.c
        ${CMAKE_CURRENT_SOURCE_DIR}/pslse/libcxl/libcxl.c
        ${CMAKE_CURRENT_SOURCE_DIR}/pslse/libcxl/libcxl.h
    )

    list(FIND PSL_VERSIONS ${PSL_VERSION} index)
    if(index EQUAL -1)
        message(FATAL_ERROR "PSL_VERSION must be one of ${PSL_VERSIONS}")
    endif()

    target_compile_definitions(cxl
        PRIVATE
        ${PSL_VERSION}
    )

    target_include_directories(cxl
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/pslse/common
        ${CMAKE_CURRENT_SOURCE_DIR}/pslse/libcxl

        INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/pslse/common>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/pslse/libcxl>
        $<INSTALL_INTERFACE:include>
    )

    target_link_options(cxl
        PRIVATE
        -Wl,--version-script ${CMAKE_CURRENT_SOURCE_DIR}/pslse/libcxl/symver.map
    )

    add_library(cxl::cxl ALIAS cxl)

    # need to export target as well
    install(TARGETS cxl EXPORT cxl-export DESTINATION "${CMAKE_INSTALL_LIBDIR}")

    # CMake config
    install(EXPORT cxl-export
        NAMESPACE   cxl::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/cxl
        COMPONENT   dev
    )

    # Export library for downstream projects
    export(TARGETS cxl NAMESPACE cxl:: FILE share/cxl/cmake/cxl/cxl-export.cmake)

    # Install cmake find script for the project
    install(FILES cxl-config.cmake DESTINATION share/cxl COMPONENT dev)
endif()
