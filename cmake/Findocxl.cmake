# OCXL_FOUND
# OCXL_INCLUDE_DIR
# OCXL_LIBRARY

include(FindPackageHandleStandardArgs)

FIND_PATH(OCXL_INCLUDE_DIR libcxl.h

    PATHS
    $ENV{OCXL_DIR}
    $ENV{OCSE_ROOT}/libcxl
    /usr
    /usr/local
    /sw
    /opt/local

    PATH_SUFFIXES
    /include

    DOC "The directory where libocxl.h resides.")

FIND_LIBRARY(OCXL_LIBRARY cxl
    PATHS
    $ENV{OCXL_DIR}
    $ENV{OCSE_ROOT}/libcxl
    /usr
    /usr/local
    /sw
    /opt/local

    DOC "The cxl library")

find_package_handle_standard_args(CXL REQUIRED_VARS OCXL_INCLUDE_DIR OCXL_LIBRARY)

mark_as_advanced(OCXL_INCLUDE_DIR OCXL_LIBRARY)
