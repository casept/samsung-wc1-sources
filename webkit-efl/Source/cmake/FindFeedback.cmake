# - Try to find feedback
# Once done, this will define
#
#  FEEDBACK_FOUND - system has feedback
#  FEEDBACK_INCLUDE_DIRS - the feedback include directories

find_package(PkgConfig)
pkg_check_modules(FEEDBACK feedback)

find_path(FEEDBACK_INCLUDE_DIR
  NAMES feedback.h
  PATHS ${FEEDBACK_PKGCONF_INCLUDE_DIRS}
)

set(FEEDBACK_PROCESS_INCLUDES FEEDBACK_INCLUDE_DIR FEEDBACK_INCLUDE_DIRS)

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(FEEDBACK DEFAULT_MSG FEEDBACK_PROCESS_INCLUDES)