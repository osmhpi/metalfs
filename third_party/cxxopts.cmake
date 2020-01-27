find_package(cxxopts QUIET)

if ((NOT CXXOPTS_INCLUDE_DIR) OR (NOT EXISTS ${CXXOPTS_INCLUDE_DIR}))
    message("Unable to find cxxopts. Including local cxxopts.")

    # We have a submodule for cxxopts. Clone it now.
    execute_process(COMMAND git submodule update --init -- cxxopts
                    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

    # set(CXL_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/pslse/libcxl
    #     CACHE PATH "pslse (libcxl) include directory" FORCE)

    add_subdirectory(cxxopts EXCLUDE_FROM_ALL)
endif()
