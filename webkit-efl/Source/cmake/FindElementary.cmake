# - Try to find Elementary
# Once done, this will define
#
#  ELEMENTARY_FOUND - system has Elementary installed.
#  ELEMENTARY_INCLUDE_DIRS - directories which contain the Elementary headers.
#  ELEMENTARY_LIBRARIES - libraries required to link against Elementary.
#
# Copyright (C) 2012 Intel Corporation. All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 1.  Redistributions of source code must retain the above copyright
#     notice, this list of conditions and the following disclaimer.
# 2.  Redistributions in binary form must reproduce the above copyright
#     notice, this list of conditions and the following disclaimer in the
#     documentation and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDER AND ITS CONTRIBUTORS ``AS
# IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
# THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
# PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR ITS
# CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
# EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
# OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
# WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
# OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
# ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

include(FindPkgConfig)

pkg_check_modules(PC_LIBELEMENTARY elementary)
find_library(ELEMENTARY_LIBRARY
    NAMES ${PC_LIBELEMENTARY_LIBRARIES}
    HINTS ${PC_LIBELEMENTARY_LIBDIR} ${PC_LIBELEMENTARY_LIBRARY_DIRS} )

set(ELEMENTARY_DEFINITIONS ${PC_LIBELEMENTARY_CFLAGS_OTHER})
set(ELEMENTARY_LIBRARIES ${ELEMENTARY_LIBRARY})
set(ELEMENTARY_INCLUDE_DIRS ${PC_LIBELEMENTARY_INCLUDE_DIRS})

# Ecore_Con is required by not WebKit/Efl but Elementary.
FIND_EFL_LIBRARY(ECORE_CON
    HEADERS Ecore_Con.h
    HEADER_PREFIXES ecore-1 ecore-con-1
    LIBRARY ecore_con
)

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(Elementary REQUIRED_VARS ELEMENTARY_INCLUDE_DIRS ELEMENTARY_LIBRARIES ECORE_CON_INCLUDE_DIRS ECORE_CON_LIBRARIES
                                             VERSION_VAR   ELM_VERSION)
