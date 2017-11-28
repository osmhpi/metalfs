# SNAP_FOUND
# SNAP_INCLUDE_DIR
# SNAP_LIBRARY

include(FindPackageHandleStandardArgs)

FIND_PATH(SNAP_INCLUDE_DIR libsnap.h

    PATHS
    $ENV{SNAP_DIR}
    /usr
    /usr/local
    /sw
    /opt/local

	PATH_SUFFIXES
    /include

    DOC "The directory where libsnap.h resides.")

FIND_LIBRARY(SNAP_LIBRARY snap
    PATHS
    $ENV{SNAP_DIR}
    /usr
    /usr/local
    /sw
    /opt/local

    DOC "The snap library")

find_package_handle_standard_args(SNAP REQUIRED_VARS SNAP_INCLUDE_DIR SNAP_LIBRARY)

mark_as_advanced(SNAP_INCLUDE_DIR SNAP_LIBRARY)
