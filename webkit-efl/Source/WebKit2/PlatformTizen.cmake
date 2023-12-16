# Please do not use ENABLE_TIZEN_FOO macro in Foo.cmake file. We generally use it inside file directly.

list(APPEND WebKit2_INCLUDE_DIRECTORIES
    "${WEBCORE_DIR}/Modules/filesystem"
    "${WEBCORE_DIR}/Modules/geolocation"
    "${WEBCORE_DIR}/platform/audio"
    "${WEBCORE_DIR}/platform/efl/tizen"
    "${WEBCORE_DIR}/platform/graphics/efl/tizen"
    "${WEBCORE_DIR}/platform/graphics/texmap/coordinated"
    "${WEBCORE_DIR}/platform/graphics/texmap/efl/tizen"
    "${WEBCORE_DIR}/platform/mediastream"
    "${WEBKIT2_DIR}/Platform/efl/tizen/AboutData"
    "${WEBKIT2_DIR}/Shared/API/c"
    "${WEBKIT2_DIR}/Shared/API/c/tizen"
    "${WEBKIT2_DIR}/Shared/efl/tizen"
    "${WEBKIT2_DIR}/Shared/tizen"
    "${WEBKIT2_DIR}/Shared"
    "${WEBKIT2_DIR}/UIProcess/API/C/efl/tizen"
    "${WEBKIT2_DIR}/UIProcess/API/efl/tizen"
    "${WEBKIT2_DIR}/UIProcess/LocalFileSystem"
    "${WEBKIT2_DIR}/UIProcess/MediaStream"
    "${WEBKIT2_DIR}/UIProcess/efl/tizen"
    "${WEBKIT2_DIR}/WebProcess/LocalFileSystem"
    "${WEBKIT2_DIR}/WebProcess/MediaStream"
    "${WEBKIT2_DIR}/WebProcess/WebCoreSupport/efl/tizen"
    "${WEBKIT2_DIR}/WebProcess/WebPage/efl/tizen"
    "${WEBKIT2_DIR}/WebProcess/WebSpeech"
    ${CAPI_INCLUDE_DIRS}
    ${DBUS-1_INCLUDE_DIRS}
    ${EFL_ASSIST_INCLUDE_DIRS}
    ${EFL_EXTENSION_INCLUDE_DIRS}
    ${ELEMENTARY_INCLUDE_DIRS}
    ${E_DBUS_INCLUDE_DIRS}
    ${Tizen-Location-Manager_INCLUDE_DIRS}
    ${LIBSMACK_INCLUDE_DIRS}
    ${LIBCAP_INCLUDE_DIRS}
    ${UIExtension_INCLUDE_DIRS}
)

list(APPEND WebKit2_LIBRARIES
    ${CAPI_LIBRARIES}
    ${EFL_ASSIST_LIBRARIES}
    ${EFL_EXTENSION_LIBRARIES}
    ${ELEMENTARY_LIBRARIES}
    ${E_DBUS_LIBRARIES}
    ${DBUS-1_LIBRARIES}
    ${Tizen-Location-Manager_LIBRARIES}
    ${LIBSMACK_LIBRARIES}
    ${LIBCAP_LIBRARIES}
    ${UIExtension_LIBRARIES}
)

# Remove trunk files to replace Tizen specific files.
list(REMOVE_ITEM WebKit2_SOURCES
    WebProcess/WebCoreSupport/efl/WebErrorsEfl.cpp
)

list(APPEND WebKit2_SOURCES
    Platform/efl/tizen/AboutData/AboutDataTizen.cpp

    Shared/WebError.cpp
    Shared/API/c/tizen/WKURLResponseTizen.cpp
    Shared/efl/tizen/WebSubresourceTizen.cpp
    Shared/tizen/ArgumentCodersTizen.cpp
    Shared/tizen/WebURLResponseTizen.cpp
    Shared/tizen/ProcessSmackLabel.cpp

    UIProcess/API/C/efl/tizen/WKPageTizen.cpp
    UIProcess/API/C/efl/tizen/WKPreferencesTizen.cpp
    UIProcess/API/C/efl/tizen/WKUserMediaPermissionRequest.cpp
    UIProcess/API/C/efl/tizen/WKWebSpeechPermissionRequest.cpp

    UIProcess/API/efl/tizen/Drag.cpp
    UIProcess/API/efl/tizen/DragHandle.cpp
    UIProcess/API/efl/tizen/FocusRing.cpp
    UIProcess/API/efl/tizen/GeolocationProviderTizen.cpp
    UIProcess/API/efl/tizen/InputPicker.cpp
    UIProcess/API/efl/tizen/JavaScriptPopup.cpp
    UIProcess/API/efl/tizen/OfflinePageSave.cpp
    UIProcess/API/efl/tizen/TextSelection.cpp
    UIProcess/API/efl/tizen/TextSelectionHandle.cpp
    UIProcess/API/efl/tizen/TextSelectionMagnifier.cpp
    UIProcess/API/efl/tizen/ewk_certificate.cpp
    UIProcess/API/efl/tizen/ewk_console_message.cpp
    UIProcess/API/efl/tizen/ewk_custom_handlers.cpp
    UIProcess/API/efl/tizen/ewk_geolocation_permission_request.cpp
    UIProcess/API/efl/tizen/ewk_hit_test.cpp
    UIProcess/API/efl/tizen/ewk_notification.cpp
    UIProcess/API/efl/tizen/ewk_policy_decision.cpp
    UIProcess/API/efl/tizen/ewk_popup_picker.cpp
    UIProcess/API/efl/tizen/ewk_text_style.cpp
    UIProcess/API/efl/tizen/ewk_user_media.cpp
    UIProcess/API/efl/tizen/ewk_util.cpp
    UIProcess/API/efl/tizen/ewk_view_notification_provider.cpp
    UIProcess/API/efl/tizen/ewk_view_tizen_client.cpp
    UIProcess/API/efl/tizen/ewk_web_application_icon_data.cpp
    UIProcess/API/efl/tizen/ewk_webspeech_permission_request.cpp

    UIProcess/MediaStream/UserMediaPermissionRequest.cpp
    UIProcess/MediaStream/UserMediaPermissionRequestManagerProxy.cpp

    UIProcess/efl/tizen/MainFrameScrollbarTizen.cpp
    UIProcess/efl/tizen/WebContextMenuProxyTizen.cpp
    UIProcess/efl/tizen/WebNavigatorContentUtilsProxyTizen.cpp
    UIProcess/efl/tizen/WebSpeechPermissionRequestProxyTizen.cpp
    UIProcess/efl/tizen/WebSpeechPermissionRequestManagerProxyTizen.cpp
    UIProcess/efl/tizen/WebTizenClient.cpp

    WebProcess/MediaStream/UserMediaPermissionRequestManager.cpp

    WebProcess/WebCoreSupport/efl/tizen/WebDragClientTizen.cpp
    WebProcess/WebCoreSupport/efl/tizen/WebErrorsTizen.cpp
    WebProcess/WebCoreSupport/efl/tizen/WebNavigatorContentUtilsClientTizen.cpp
    WebProcess/WebCoreSupport/efl/tizen/WebUserMediaClient.cpp

    WebProcess/WebPage/efl/tizen/WebPageSerializerTizen.cpp

    WebProcess/WebSpeech/WebSpeechPermissionRequestManager.cpp
)

list(APPEND WebKit2_MESSAGES_IN_FILES
    UIProcess/efl/tizen/WebNavigatorContentUtilsProxyTizen.messages.in
)

SET(THEME_DIR ${CMAKE_BINARY_DIR}/theme)
FILE(MAKE_DIRECTORY ${THEME_DIR})

set(CONTROL_THEME ${THEME_DIR}/control.edj)

add_custom_command(
    OUTPUT ${CONTROL_THEME}
    COMMAND ${EDJE_CC_EXECUTABLE} -no-save -id ${WEBKIT2_DIR}/UIProcess/API/efl/tizen/images ${WEBKIT2_DIR}/UIProcess/API/efl/tizen/control.edc ${CONTROL_THEME}
    DEPENDS
        ${WEBKIT2_DIR}/UIProcess/API/efl/tizen/control.edc
)
list(APPEND WebKit2_SOURCES ${CONTROL_THEME})

INSTALL(FILES ${CONTROL_THEME}
DESTINATION share/${WebKit2_OUTPUT_NAME}-${PROJECT_VERSION_MAJOR}/themes)

file(GLOB EWebKit2ForTizen_HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/UIProcess/API/efl/CAPI/*.h")
install(FILES ${EWebKit2ForTizen_HEADERS} DESTINATION include/${WebKit2_OUTPUT_NAME}-${PROJECT_VERSION_MAJOR})

# Install Tizen error files.
add_definitions("-DWEBKIT_HTML_DIR=\"${CMAKE_INSTALL_PREFIX}/share/${WebKit2_OUTPUT_NAME}-${PROJECT_VERSION_MAJOR}/html\"")
set(WEBKIT2_EFL_ERROR_PAGE_DIR share/${WebKit2_OUTPUT_NAME}-${PROJECT_VERSION_MAJOR}/html)
install(FILES ${WEBKIT2_DIR}/UIProcess/efl/htmlfiles/errorPage.html DESTINATION ${WEBKIT2_EFL_ERROR_PAGE_DIR})

# Install Tizen text translation files.
add_definitions("-DWEBKIT_TEXT_DIR=\"${CMAKE_INSTALL_PREFIX}/share/locale/\"")
INCLUDE_IF_EXISTS(${WEBKIT2_DIR}/UIProcess/efl/po_tizen/CMakeLists.txt)
add_definitions("-DEDJE_DIR=\"${CMAKE_INSTALL_PREFIX}/share/${WebKit2_OUTPUT_NAME}-${PROJECT_VERSION_MAJOR}/themes\"")
set(MAGNIFIER_THEME ${THEME_DIR}/Magnifier.edj)
ADD_CUSTOM_COMMAND(
    OUTPUT ${MAGNIFIER_THEME}
    COMMAND  ${EDJE_CC_EXECUTABLE} -no-save -id ${WEBKIT2_DIR}/UIProcess/API/efl/tizen/images ${WEBKIT2_DIR}/UIProcess/API/efl/tizen/Magnifier.edc ${MAGNIFIER_THEME}
    DEPENDS
        ${WEBKIT2_DIR}/UIProcess/API/efl/tizen/Magnifier.edc
)
list(APPEND WebKit2_SOURCES ${MAGNIFIER_THEME})
install(FILES ${MAGNIFIER_THEME} DESTINATION share/${WebKit2_OUTPUT_NAME}-${PROJECT_VERSION_MAJOR}/themes)

# Install Tizen drag files
if (ENABLE_TIZEN_DRAG_SUPPORT)
    set(DRAG_THEME ${THEME_DIR}/Drag.edj)
    add_custom_command(
        OUTPUT ${DRAG_THEME}
        COMMAND  ${EDJE_CC_EXECUTABLE} -no-save -id ${WEBKIT2_DIR}/UIProcess/API/efl/tizen/images ${WEBKIT2_DIR}/UIProcess/API/efl/tizen/Drag.edc ${DRAG_THEME}
        DEPENDS
            ${WEBKIT2_DIR}/UIProcess/API/efl/tizen/Drag.edc
    )
    list(APPEND WebKit2_SOURCES ${DRAG_THEME})
endif ()

# Install edge effect images.
FILE(GLOB EDGE_EFFECT_IMAGES "${WEBKIT2_DIR}/UIProcess/API/efl/tizen/images/overscrolling/bouncing_top_0*.png")
INSTALL(FILES ${EDGE_EFFECT_IMAGES}
    DESTINATION share/${WebKit2_OUTPUT_NAME}-${PROJECT_VERSION_MAJOR}/images)

FILE(GLOB JS_POPUP_IMAGES "${WEBKIT2_DIR}/UIProcess/API/efl/tizen/images/popup_btn_*.png")
INSTALL(FILES ${JS_POPUP_IMAGES}
    DESTINATION share/${WebKit2_OUTPUT_NAME}-${PROJECT_VERSION_MAJOR}/images)
add_definitions(-DJS_POPUP_IMAGE_PATH="/usr/share/${WebKit2_OUTPUT_NAME}-${PROJECT_VERSION_MAJOR}/images")

if (ENABLE_TIZEN_SPATIAL_NAVIGATION)
    set(SPATIAL_NAVIGATION_FOCUS_RING_IMAGE_PATH ${CMAKE_INSTALL_PREFIX}/share/${WebKit2_OUTPUT_NAME}-${PROJECT_VERSION_MAJOR}/images)
    add_definitions(-DSPATIAL_NAVIGATION_FOCUS_RING_IMAGE_PATH="${SPATIAL_NAVIGATION_FOCUS_RING_IMAGE_PATH}/spatialNavigationFocusRing.png")
    install(FILES ${WEBKIT2_DIR}/UIProcess/API/efl/tizen/images/spatialNavigationFocusRing.png DESTINATION ${SPATIAL_NAVIGATION_FOCUS_RING_IMAGE_PATH})
endif ()


if (ENABLE_TIZEN_CIRCLE_DISPLAY)
    LIST(APPEND WebKit2_INCLUDE_DIRECTORIES
        ${EFLExtension_INCLUDE_DIRS}
    )
    LIST(APPEND WebKit2_LIBRARIES
        ${EFLExtension_LIBRARIES}
    )
ENDIF ()

if (ENABLE_TIZEN_WEBPROCESS_MEM_TRACK)
    LIST(APPEND WebKit2_INCLUDE_DIRECTORIES
        ${AUL_INCLUDE_DIRS}
    )
    LIST(APPEND WebKit2_LIBRARIES
        ${AUL_LIBRARIES}
    )
endif ()

# Install image files for broken image and text area
FILE(GLOB MISSING_IMAGES "${WEBCORE_DIR}/Resources/missingImage*.png")
FILE(GLOB TEXT_AREA_RESIZE_CORNER_IMAGES "${WEBCORE_DIR}/Resources/textAreaResizeCorner*.png")
INSTALL(FILES ${MISSING_IMAGES} ${TEXT_AREA_RESIZE_CORNER_IMAGES}
    DESTINATION share/${WebKit2_OUTPUT_NAME}-${PROJECT_VERSION_MAJOR}/images)
