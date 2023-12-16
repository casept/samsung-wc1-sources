# Copyright (c) 2012, Samsung Electronics
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
# * Redistributions of source code must retain the above copyright notice, this
#   list of conditions and the following disclaimer.
# * Redistributions in binary form must reproduce the above copyright notice,
#   this list of conditions and the following disclaimer in the documentation
#   and/or other materials provided with the distribution.
# * Neither the name of Intel Corporation nor the names of its contributors may
#   be used to endorse or promote products derived from this software without
#   specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
# LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.
#
# Try to find CAPI include and library directories.
#
# After successful discovery, this will set for inclusion where needed:
#
#  CAPI_INCLUDE_DIRS
#  CAPI_LIBRARIES

INCLUDE(FindPkgConfig)

PKG_CHECK_MODULES(PC_CAPI
    capi-appfw-app-common
    capi-appfw-app-control
    capi-appfw-application
    capi-system-sensor
    capi-system-power
    capi-system-info
    capi-media-camera
)

FIND_LIBRARY(APPFW_APP_COMMON_LIBRARIES NAMES capi-appfw-app-common
    HINTS ${PC_CAPI_LIBRARY_DIRS} ${PC_CAPI_LIBDIR}
)
FIND_LIBRARY(APPFW_APP_CONTROL_LIBRARIES NAMES capi-appfw-app-control
    HINTS ${PC_CAPI_LIBRARY_DIRS} ${PC_CAPI_LIBDIR}
)
FIND_PATH(APPFW_APPLICATION_INCLUDE_DIRS NAMES app.h
    HINTS ${PC_CAPI_INCLUDE_DIRS} ${PC_CAPI_INCLUDEDIR}
)
FIND_LIBRARY(APPFW_APPLICATION_LIBRARIES NAMES capi-appfw-application
    HINTS ${PC_CAPI_LIBRARY_DIRS} ${PC_CAPI_LIBDIR}
)

list(FIND CAPI_FIND_COMPONENTS NETWORK_CONNECTION RET)
if (${RET} GREATER -1)
    PKG_CHECK_MODULES(PC_CAPI_NETWORK_CONNECTION capi-network-connection)
    FIND_PATH(NETWORK_CONNECTION_INCLUDE_DIRS NAMES net_connection.h
        HINTS ${PC_CAPI_NETWORK_CONNECTION_INCLUDE_DIRS} ${PC_CAPI_NETWORK_CONNECTION_INCLUDEDIR}
    )
    FIND_LIBRARY(NETWORK_CONNECTION_LIBRARIES NAMES capi-network-connection
        HINTS ${PC_CAPI_NETWORK_CONNECTION_LIBRARY_DIRS} ${PC_CAPI_NETWORK_CONNECTION_LIBDIR}
    )
endif ()

FIND_PATH(SYSTEM_SENSOR_INCLUDE_DIRS NAMES sensor.h
    HINTS ${PC_CAPI_INCLUDE_DIRS} ${PC_CAPI_INCLUDEDIR}
)
FIND_LIBRARY(SYSTEM_SENSOR_LIBRARIES NAMES capi-system-sensor
    HINTS ${PC_CAPI_LIBRARY_DIRS} ${PC_CAPI_LIBDIR}
)

list(FIND CAPI_FIND_COMPONENTS TELEPHONY_NETWORK_INFO RET)
if (${RET} GREATER -1)
    PKG_CHECK_MODULES(PC_CAPI_TELEPHONY_NETWORK_INFO capi-telephony-network-info)
    FIND_PATH(TELEPHONY_NETWORK_INCLUDE_DIRS NAMES telephony_network.h
        HINTS ${PC_CAPI_TELEPHONY_NETWORK_INFO_INCLUDE_DIRS} ${PC_CAPI_TELEPHONY_NETWORK_INFO_INCLUDEDIR}
    )
    FIND_LIBRARY(TELEPHONY_NETWORK_LIBRARIES NAMES capi-telephony-network-info
        HINTS ${PC_CAPI_TELEPHONY_NETWORK_LIBRARY_DIRS} ${PC_CAPI_TELEPHONY_NETWORK_INFO_LIBDIR}
    )
endif ()

FIND_PATH(SYSTEM_POWER_INCLUDE_DIRS NAMES power.h
    HINTS ${PC_CAPI_INCLUDE_DIRS} ${PC_CAPI_INCLUDEDIR}
)
FIND_LIBRARY(SYSTEM_POWER_LIBRARIES NAMES capi-system-power
    HINTS ${PC_CAPI_LIBRARY_DIRS} ${PC_CAPI_LIBDIR}
)

FIND_PATH(SYSTEM_INFO_INCLUDE_DIRS NAMES system_info.h
    HINTS ${PC_CAPI_INCLUDE_DIRS} ${PC_CAPI_INCLUDEDIR}
)
FIND_LIBRARY(SYSTEM_INFO_LIBRARIES NAMES capi-system-info
    HINTS ${PC_CAPI_LIBRARY_DIRS} ${PC_CAPI_LIBDIR}
)

FIND_PATH(MEDIA_CAMERA_INCLUDE_DIRS NAMES camera.h
    HINTS ${PC_CAPI_INCLUDE_DIRS} ${PC_CAPI_INCLUDEDIR}
)

FIND_LIBRARY(MEDIA_CAMERA_LIBRARIES NAMES capi-media-camera
    HINTS ${PC_CAPI_LIBRARY_DIRS} ${PC_CAPI_LIBDIR}
)

SET(CAPI_INCLUDE_DIRS
    ${APPFW_APPLICATION_INCLUDE_DIRS}
    ${NETWORK_CONNECTION_INCLUDE_DIRS}
    ${SYSTEM_DEVICE_INCLUDE_DIRS}
    ${SYSTEM_SENSOR_INCLUDE_DIRS}
    ${TELEPHONY_NETWORK_INCLUDE_DIRS}
    ${SYSTEM_POWER_INCLUDE_DIRS}
    ${SYSTEM_INFO_INCLUDE_DIRS}
    ${MEDIA_CAMERA_INCLUDE_DIRS}
)
SET(CAPI_LIBRARIES
    ${APPFW_APP_COMMON_LIBRARIES}
    ${APPFW_APP_CONTROL_LIBRARIES}
    ${APPFW_APPLICATION_LIBRARIES}
    ${NETWORK_CONNECTION_LIBRARIES}
    ${SYSTEM_DEVICE_LIBRARIES}
    ${SYSTEM_SENSOR_LIBRARIES}
    ${TELEPHONY_NETWORK_LIBRARIES}
    ${SYSTEM_POWER_LIBRARIES}
    ${SYSTEM_INFO_LIBRARIES}
    ${MEDIA_CAMERA_LIBRARIES}
)

INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(CAPI DEFAULT_MSG CAPI_INCLUDE_DIRS CAPI_LIBRARIES)
