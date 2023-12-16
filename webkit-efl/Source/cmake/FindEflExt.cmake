# - Try to find efl-extension
# Once done, this will define
#
#  EFL_EXTENSION_FOUND - system has efl-extension
#  EFL_EXTENSION_INCLUDE_DIRS - the efl-extension include directories
#  EFL_EXTENSION_LIBRARIES - link these to use efl-extension

INCLUDE(FindPkgConfig)

PKG_CHECK_MODULES(PC_EFL_EXTENSION efl-extension)

FIND_PATH(EFL_EXTENSION_INCLUDE_DIRS
    NAMES efl_extension.h
    PATHS ${PC_EFL_EXTENSION_INCLUDE_DIRS}
    PATH_SUFFIXES efl-extension
)
FIND_LIBRARY(EFL_EXTENSION_LIBRARIES
    NAMES efl-extension
    PATHS ${PC_EFL_EXTENSION_LIBRARY_DIRS}
)

INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(EFL_EXTENSION DEFAULT_MSG EFL_EXTENSION_INCLUDE_DIRS EFL_EXTENSION_LIBRARIES)
