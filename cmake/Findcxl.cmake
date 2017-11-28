# CXL_FOUND
# CXL_INCLUDE_DIR
# CXL_LIBRARY

include(FindPackageHandleStandardArgs)

FIND_PATH(CXL_INCLUDE_DIR libcxl.h

    PATHS
    $ENV{CXL_DIR}
    /usr
    /usr/local
    /sw
    /opt/local

	PATH_SUFFIXES
    /include

    DOC "The directory where libcxl.h resides.")

FIND_LIBRARY(CXL_LIBRARY cxl
    PATHS
    $ENV{CXL_DIR}
    /usr
    /usr/local
    /sw
    /opt/local

    DOC "The cxl library")

find_package_handle_standard_args(CXL REQUIRED_VARS CXL_INCLUDE_DIR CXL_LIBRARY)

mark_as_advanced(CXL_INCLUDE_DIR CXL_LIBRARY)
