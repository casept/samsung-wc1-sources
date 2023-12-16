# - Try to find efl-extension
# Once done, this will define
#
#  EFLExtension_FOUND - system has efl-extension
#  EFLExtension_INCLUDE_DIRS - the efl-extension include directories
#  EFLExtension_LIBRARIES - link these to use efl-extension

INCLUDE(FindPkgConfig)

PKG_CHECK_MODULES(PC_EFLExtension efl-extension)

FIND_PATH(EFLExtension_INCLUDE_DIRS NAMES efl_extension.h
    HINTS ${PC_EFLExtension_INCLUDE_DIRS} ${PC_EFLExtension_INCLUDEDIR}
)

FIND_LIBRARY(EFLExtension_LIBRARIES NAMES efl-extension
    HINTS ${PC_EFLExtension_LIBRARY_DIRS} ${PC_EFLExtension_LIBDIR}
)

INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(EFLExtension DEFAULT_MSG EFLExtension_INCLUDE_DIRS EFLExtension_LIBRARIES)
