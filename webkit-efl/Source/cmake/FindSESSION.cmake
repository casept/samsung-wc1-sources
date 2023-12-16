# - Try to find SESSION
# Once done, this will define
#
#  SESSION_FOUND - system has mm_session
#  SESSION_INCLUDE_DIRS - the mm_session include directories
#  SESSION_LIBRARIES - link these to use mm_session

INCLUDE(FindPkgConfig)

PKG_CHECK_MODULES(SESSION
    mm-session
)

FIND_PATH(SESSION_INCLUDE_DIRS NAMES mm_session.h
    HINTS ${SESSION_INCLUDE_DIRS} ${SESSION_INCLUDEDIR}
)
FIND_LIBRARY(SESSION_LIBRARIES NAMES mm-session
    HINTS ${SESSION_LIBRARY_DIRS} ${SESSION_LIBDIR}
)

SET(SESSION_INCLUDE_DIRS
    ${SESSION_INCLUDE_DIRS}
)
SET(SESSION_LIBRARIES
    ${SESSION_LIBRARIES}
)

INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(SESSION DEFAULT_MSG SESSION_INCLUDE_DIRS SESSION_LIBRARIES)
