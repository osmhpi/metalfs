# Try to find libfuse
#
# once done this will define
# FUSE_FOUND - System has libfuse
# FUSE_INCLUDE_DIRS - The libfuse include directories
# FUSE_LIBRARIES - The libraries needed to use libfuse
# FUSE_DEFINITIONS - Compiler switches required for using libfuse

### DEBUGGING ###
# to activate run cmake ../src -DFUSE_DEBUG

function(fusedebug _varname)
if(FUSE_DEBUG)
    message("${_varname} = ${${_varname}}")
endif(FUSE_DEBUG)
endfunction(fusedebug)

### NO-DEBUGGING ###

# did we have pkg-config?
find_package(PkgConfig)

# using pkg-config for fuse
pkg_check_modules(PC_FUSE REQUIRED fuse)

if(PC_FUSE_FOUND)
fusedebug(PC_FUSE_LIBRARIES)
fusedebug(PC_FUSE_LIBRARY_DIRS)
fusedebug(PC_FUSE_LDFLAGS)
fusedebug(PC_FUSE_LDFLAGS_OTHER)
fusedebug(PC_FUSE_INCLUDE_DIRS)
fusedebug(PC_FUSE_CFLAGS)
fusedebug(PC_FUSE_CFLAGS_OTHER)
endif(PC_FUSE_FOUND)

# search for headers
find_path(FUSE_INCLUDE_DIR fuse.h HINTS ${PC_FUSE_INCLUDEDIR}
${PC_FUSE_INCLUDE_DIRS} PATH_SUFFIXES fuse libfuse libfuse-dev )

# search for libs
find_library(FUSE_LIBRARIES NAMES fuse libfuse
HINTS ${PC_FUSE_LIBDIR} ${PC_FUSE_LIBRARY_DIRS} )

# set FUSE_DEFINITIONS to cflags
set(FUSE_DEFINITIONS ${PC_FUSE_CFLAGS_OTHER} )

# set libs
set(FUSE_LIBRARIES ${FUSE_LIBRARY})

# set headers
set(FUSE_INCLUDE_DIRS ${FUSE_INCLUDE_DIR})

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(FUSE FUSE_LIBRARY FUSE_INCLUDE_DIR)

mark_as_advanced(FUSE_INCLUDE_DIR FUSE_LIBRARY)