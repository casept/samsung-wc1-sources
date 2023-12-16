# - Try to find Tizen-Location-Manager
# Once done, this will define
#
#  Tizen-Location-Manager_FOUND - system has location-manager
#  Tizen-Location-Manager_INCLUDE_DIRS - the location-manager include directories
#  Tizen-Location-Manager_LIBRARIES - link these to use location-manager

INCLUDE(FindPkgConfig)

# Use pkg-config to get hints about paths
pkg_check_modules(PC_TIZEN-LOCATION-MANAGER capi-location-manager)

# Include dir
find_path(Tizen-Location-Manager_INCLUDE_DIRS
  NAMES locations.h
  PATHS ${PC_TIZEN-LOCATION-MANAGER_INCLUDE_DIRS}
  PATH_SUFFIXES location
)

# Finally the library itself
find_library(Tizen-Location-Manager_LIBRARIES
  NAMES capi-location-manager
  PATHS ${PC_TIZEN-LOCATION-MANAGER_LIBRARY_DIRS}
)

INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(Tizen-Location-Manager DEFAULT_MSG Tizen-Location-Manager_INCLUDE_DIRS Tizen-Location-Manager_LIBRARIES)
