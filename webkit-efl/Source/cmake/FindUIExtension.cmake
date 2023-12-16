# - Try to find ui-extension
# Once done, this will define
#
#  UIExtension_FOUND - system has ui-extension
#  UIExtension_INCLUDE_DIRS - the ui-extension include directories
#  UIExtension_LIBRARIES - link these to use ui-extension

INCLUDE(FindPkgConfig)

PKG_CHECK_MODULES(PC_UIExtension ui-extension)

FIND_PATH(UIExtension_INCLUDE_DIRS NAMES ui_extension.h
    HINTS ${PC_UIExtension_INCLUDE_DIRS} ${PC_UIExtension_INCLUDEDIR}
)

FIND_LIBRARY(UIExtension_LIBRARIES NAMES ui-extension
    HINTS ${PC_UIExtension_LIBRARY_DIRS} ${PC_UIExtension_LIBDIR}
)

INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(UIExtension DEFAULT_MSG UIExtension_INCLUDE_DIRS UIExtension_LIBRARIES)
