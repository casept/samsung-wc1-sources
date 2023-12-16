# - Try to find LevelDB
# Once done, this will define
#
#  LEVELDB_FOUND - system has LevelDB
#  LEVELDB_INCLUDE_DIRS - the LevelDB include directories
#  LEVELDB_LIBRARIES - link these to use LevelDB
include(FindPkgConfig)

pkg_check_modules(PC_LEVELDB leveldb)

find_path(LEVELDB_INCLUDE_DIRS NAMES leveldb/db.h
    HINTS ${PC_LEVELDB_INCLUDE_DIRS}
)

find_library(LEVELDB_LIBRARY NAMES leveldb
  PATHS ${PC_LEVELDB_LIBRARIY_DIRS}
)

find_library(LEVELDB_MEMENV_LIBRARY NAMES memenv
  PATHS ${PC_LEVELDB_LIBRARIY_DIRS}
)

set(LEVELDB_LIBRARIES
    ${LEVELDB_LIBRARY}
    ${LEVELDB_MEMENV_LIBRARY}
)

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(LEVELDB DEFAULT_MSG LEVELDB_INCLUDE_DIRS LEVELDB_LIBRARIES)
