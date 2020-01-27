find_package(googletest QUIET)

if ((NOT GOOGLETEST_LIBRARY) OR (NOT EXISTS ${GOOGLETEST_LIBRARY}))
    message("Unable to find googletest. Building googletest from source.")

    # We have a submodule for googletest. Clone it now.
    execute_process(COMMAND git submodule update --init -- googletest
                    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

    add_subdirectory(googletest EXCLUDE_FROM_ALL)
endif()
