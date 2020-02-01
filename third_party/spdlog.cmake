find_package(spdlog QUIET)

if ((NOT SPDLOG_LIBRARY) OR (NOT EXISTS ${SPDLOG_LIBRARY}))
    message("Unable to find spdlog. Building spdlog from source.")

    # We have a submodule for spdlog. Clone it now.
    execute_process(COMMAND git submodule update --init -- spdlog
                    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

    set(SPDLOG_BUILD_SHARED ON CACHE BOOL "Build spdlog shared library" FORCE)
    set(SPDLOG_INSTALL ON CACHE BOOL "Install spdlog library" FORCE)
    add_subdirectory(spdlog)
    # Export library for downstream projects
    export(TARGETS spdlog NAMESPACE spdlog:: FILE ${PROJECT_BINARY_DIR}/cmake/spdlog/spdlog-export.cmake)
endif()
