# - Try to find ASM
# Once done, this will define
#
#  ASM_FOUND - system has audio-session-mgr
#  ASM_INCLUDE_DIRS - the audio-session-mgr include directories
#  ASM_LIBRARIES - link these to use audio-session-mgr

INCLUDE(FindPkgConfig)

PKG_CHECK_MODULES(ASM
    audio-session-mgr
)

FIND_PATH(ASM_INCLUDE_DIRS NAMES audio-session-manager.h
    HINTS ${ASM_INCLUDE_DIRS} ${ASM_INCLUDEDIR}
)
FIND_LIBRARY(ASM_LIBRARIES NAMES audio-session-mgr
    HINTS ${ASM_LIBRARY_DIRS} ${ASM_LIBDIR}
)

SET(ASM_INCLUDE_DIRS
    ${ASM_INCLUDE_DIRS}
)
SET(ASM_LIBRARIES
    ${ASM_LIBRARIES}
)

INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(ASM DEFAULT_MSG ASM_INCLUDE_DIRS ASM_LIBRARIES)
