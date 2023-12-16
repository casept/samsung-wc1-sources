# - Try to find VConf
# Once done, this will define
#
#  VConf_FOUND - system has libvconf-0
#  VConf_INCLUDE_DIRS - the libvconf-0 include directories
#  VConf_LIBRARIES - link these to use libvconf-0

INCLUDE(FindPkgConfig)

# Use pkg-config to get hints about paths
PKG_CHECK_MODULES(PC_VCONF vconf)

# Include dir
find_path(VConf_INCLUDE_DIRS
  NAMES vconf.h
  PATHS ${PC_VCONF_INCLUDE_DIRS}
  PATH_SUFFIXES vconf
)

# Finally the library itself
find_library(VConf_LIBRARIES
  NAMES vconf
  PATHS ${PC_VCONF_LIBRARY_DIRS}
)

INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(VConf DEFAULT_MSG VConf_INCLUDE_DIRS VConf_LIBRARIES)
