find_package(PkgConfig)
pkg_check_modules(PC_EAIL eail)

find_path(EAIL_INCLUDE_DIRS
    NAMES eail.h
    HINTS ${PC_EAIL_INCLUDEDIR}
          ${PC_EAIL_INCLUDE_DIRS}
    PATH_SUFFIXES eail
)

find_library(EAIL_LIBRARIES
    NAMES eail-1.0.0
    HINTS ${PC_EAIL_LIBRARY_DIRS}
          ${PC_EAIL_LIBDIR}
)

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(EAIL REQUIRED_VARS EAIL_INCLUDE_DIRS EAIL_LIBRARIES)
