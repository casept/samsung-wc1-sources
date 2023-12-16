# - Try to find E-DBus
# Once done, this will define
#
#  EDBUS_FOUND - system has edbus
#  EDBUS_INCLUDE_DIRS - the edbus include directories
#  EDBUS_LIBRARIES - link these to use edbus

find_package(PkgConfig)

# Use pkg-config to get hints about paths
pkg_check_modules(EDBUS edbus)

# Include dir
find_path(EDBUS_INCLUDE_DIR
  NAMES E_DBus.h
  PATHS ${EDBUS_PKGCONF_INCLUDE_DIRS}
)

# Finally the library itself
find_library(EDBUS_LIBRARY
  NAMES edbus
  PATHS ${EDBUS_PKGCONF_LIBRARY_DIRS}
)

# Set the include dir variables and the libraries and let libfind_process do the rest.
# NOTE: Singular variables for this library, plural for libraries this this lib depends on.
set(EDBUS_PROCESS_INCLUDES EDBUS_INCLUDE_DIR EDBUS_INCLUDE_DIRS)
set(EDBUS_PROCESS_LIBS EDBUS_LIBRARY EDBUS_LIBRARIES)

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(EDBUS DEFAULT_MSG EDBUS_PROCESS_INCLUDES EDBUS_PROCESS_LIBS)
