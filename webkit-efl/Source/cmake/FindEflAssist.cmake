# - Try to find efl-assist
# Once done, this will define
#
#  EFL_ASSIST_FOUND - system has efl-assist
#  EFL_ASSIST_INCLUDE_DIRS - the efl-assist include directories
#  EFL_ASSIST_LIBRARIES - link these to use efl-assist

INCLUDE(FindPkgConfig)

PKG_CHECK_MODULES(PC_EFL_ASSIST efl-assist)

FIND_PATH(EFL_ASSIST_INCLUDE_DIRS
    NAMES efl_assist.h
    PATHS ${PC_EFL_ASSIST_INCLUDE_DIRS}
    PATH_SUFFIXES efl-assist
)
FIND_LIBRARY(EFL_ASSIST_LIBRARIES
    NAMES efl-assist
    PATHS ${PC_EFL_ASSIST_LIBRARY_DIRS}
)

INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(EFL_ASSIST DEFAULT_MSG EFL_ASSIST_INCLUDE_DIRS EFL_ASSIST_LIBRARIES)
