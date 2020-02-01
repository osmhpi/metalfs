find_package(nlohmann_json 3.2.0 QUIET)

if ((NOT NLOHMANN_JSON_INCLUDE_DIR) OR (NOT EXISTS ${NLOHMANN_JSON_INCLUDE_DIR}))
    message("Unable to find json. Including local json.")

    # We have a submodule for json. Clone it now.
    execute_process(COMMAND git submodule update --init -- json
                    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

    set(JSON_BuildTests OFF CACHE INTERNAL "")
    add_subdirectory(json EXCLUDE_FROM_ALL)
endif()
