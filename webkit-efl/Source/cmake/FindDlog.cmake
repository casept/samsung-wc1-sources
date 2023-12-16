# - Try to find Dlog
# Once done, this will define
#
#  DLOG_FOUND - system has libdlog-0
#  DLIG_INCLUDE_DIRS - the libdlog-0 include directories
#  DLOG_LIBRARIES - link these to use libdlog-0

find_package(PkgConfig)

# Use pkg-config to get hints about paths
pkg_check_modules(DLOG dlog)

# Include dir
find_path(DLOG_INCLUDE_DIR
  NAMES dlog.h
  PATHS ${DLOG_PKGCONF_INCLUDE_DIRS}
  PATH_SUFFIXES dlog
)

# Finally the library itself
find_library(DLOG_LIBRARY
  NAMES dlog
  PATHS ${DLOG_PKGCONF_LIBRARY_DIRS}
)

# Set the include dir variables and the libraries and let libfind_process do the rest.
# NOTE: Singular variables for this library, plural for libraries this this lib depends on.
set(DLOG_PROCESS_INCLUDES DLOG_INCLUDE_DIR DLOG_INCLUDE_DIRS)
set(DLOG_PROCESS_LIBS DLOG_LIBRARY DLOG_LIBRARIES)

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(DLOG DEFAULT_MSG DLOG_PROCESS_INCLUDES DLOG_PROCESS_LIBS)
