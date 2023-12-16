# - Try to find DBus-1
# Once done, this will define
#
#  DBUS-1_FOUND - system has dbus-1
#  DBUS-1_INCLUDE_DIRS - the dbus-1 include directories
#  DBUS-1_LIBRARIES - link these to use dbus-1

find_package(PkgConfig)

# Use pkg-config to get hints about paths
pkg_check_modules(DBUS-1 dbus-1)

# Include dir
find_path(DBUS-1_MAIN_INCLUDE_DIR
  NAMES dbus/dbus.h
  PATHS ${DBUS-1_PKGCONF_INCLUDE_DIRS}
)

find_path(DBUS-1_ARCH_INCLUDE_DIR
  NAMES dbus/dbus-arch-deps.h
  PATHS ${DBUS-1_PKGCONF_INCLUDE_DIRS}
)

SET(DBUS-1_INCLUDE_DIR
  ${DBUS-1_MAIN_INCLUDE_DIR}
  ${DBUS-1_ARCH_INCLUDE_DIR}
)

# Finally the library itself
find_library(DBUS-1_LIBRARY
NAMES dbus-1
PATHS ${DBUS-1_PKGCONF_LIBRARY_DIRS}
)

# Set the include dir variables and the libraries and let libfind_process do the rest.
# NOTE: Singular variables for this library, plural for libraries this this lib depends on.
set(DBUS-1_PROCESS_INCLUDES DBUS-1_INCLUDE_DIR DBUS-1_INCLUDE_DIRS)
set(DBUS-1_PROCESS_LIBS DBUS-1_LIBRARY DBUS-1_LIBRARIES)

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(DBUS-1 DEFAULT_MSG DBUS-1_PROCESS_INCLUDES DBUS-1_PROCESS_LIBS)

