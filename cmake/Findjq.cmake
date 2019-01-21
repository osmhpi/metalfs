find_path(JQ_INCLUDE_DIR NAMES  jq.h PATHS "$ENV{JQ_DIR}/include")
find_library(JQ_LIBRARIES NAMES jq   PATHS "$ENV{JQ_DIR}/lib" )

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(jq DEFAULT_MSG JQ_INCLUDE_DIR JQ_LIBRARIES)

if(JQ_FOUND)
    message(STATUS "Found jq    (include: ${JQ_INCLUDE_DIR}, library: ${JQ_LIBRARIES})")
    mark_as_advanced(JQ_INCLUDE_DIR JQ_LIBRARIES)
endif()
