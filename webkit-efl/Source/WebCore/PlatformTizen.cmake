# Please do not use ENABLE_TIZEN_FOO macro in Foo.cmake file. We generally use it inside file directly.

list(APPEND WebCore_INCLUDE_DIRECTORIES
    "${WEBCORE_DIR}/Modules/mediastream/tizen"
    "${WEBCORE_DIR}/page/scrolling/coordinatedgraphics/tizen"
    "${WEBCORE_DIR}/platform/audio/gstreamer/tizen"
    "${WEBCORE_DIR}/platform/efl/tizen"
    "${WEBCORE_DIR}/platform/graphics/efl/tizen"
    "${WEBCORE_DIR}/platform/graphics/gstreamer/tizen"
    "${WEBCORE_DIR}/platform/graphics/texmap/coordinated/efl/tizen"
    "${WEBCORE_DIR}/platform/graphics/texmap/efl/tizen"
    "${WEBCORE_DIR}/platform/mediastream/"
    "${WEBCORE_DIR}/platform/mediastream/tizen"
    "${WTF_DIR}/wtf/efl/tizen"
    ${ASM_INCLUDE_DIRS}
    ${CAPI_INCLUDE_DIRS}
    ${DRI2_INCLUDE_DIRS}
    ${GSTREAMER_INTERFACES_INCLUDE_DIRS}
    ${LEVELDB_INCLUDE_DIRS}
    ${MMSOUND_INCLUDE_DIRS}
    ${OPENSSL_INCLUDE_DIR}
    ${SESSION_INCLUDE_DIRS}
    ${STT_INCLUDE_DIRS}
    ${TBM_INCLUDE_DIRS}
    ${TTS_INCLUDE_DIRS}
    ${VConf_INCLUDE_DIRS}
    ${feedback_INCLUDE_DIR}
    ${DBUS-1_INCLUDE_DIRS}
    ${E_DBUS_INCLUDE_DIRS}
)

list(APPEND WebCore_LIBRARIES
    ${ASM_LIBRARIES}
    ${CAPI_LIBRARIES}
    ${DRI2_LIBRARIES}
    ${GSTREAMER_INTERFACES_LIBRARIES}
    ${LEVELDB_LIBRARIES}
    ${MMSOUND_LIBRARIES}
    ${OPENSSL_LIBRARIES}
    ${SESSION_LIBRARIES}
    ${STT_LIBRARIES}
    ${TBM_LIBRARIES}
    ${TTS_LIBRARIES}
    ${VConf_LIBRARIES}
)

# Replace EFL port files with Tizen's.
list(REMOVE_ITEM WebCore_SOURCES

    platform/efl/LocalizedStringsEfl.cpp
    platform/efl/PasteboardEfl.cpp
    platform/efl/ScrollbarThemeEfl.cpp

    platform/mediastream/gstreamer/MediaStreamCenterGStreamer.cpp

    platform/network/soup/ResourceHandleSoup.cpp
    platform/network/soup/SocketStreamHandleSoup.cpp
)

list(APPEND WebCore_IDL_FILES
    Modules/mediastream/tizen/BlobCallback.idl
    Modules/mediastream/tizen/CameraControl.idl
    Modules/mediastream/tizen/CameraCapabilities.idl
    Modules/mediastream/tizen/CameraError.idl
    Modules/mediastream/tizen/CameraErrorCallback.idl
    Modules/mediastream/tizen/CameraImageCapture.idl
    Modules/mediastream/tizen/CameraMediaRecorder.idl
    Modules/mediastream/tizen/CameraSettingErrors.idl
    Modules/mediastream/tizen/CameraSettingErrorCallback.idl
    Modules/mediastream/tizen/CameraSuccessCallback.idl
    Modules/mediastream/tizen/CameraSize.idl
    Modules/mediastream/tizen/CreateCameraSuccessCallback.idl
    Modules/mediastream/tizen/MediaStreamRecorder.idl
    Modules/mediastream/tizen/NavigatorTizenCamera.idl
    Modules/mediastream/tizen/TizenCamera.idl
)

list(APPEND WebCore_SOURCES

    Modules/mediastream/tizen/CameraControl.cpp
    Modules/mediastream/tizen/CameraCapabilities.cpp
    Modules/mediastream/tizen/CameraImageCapture.cpp
    Modules/mediastream/tizen/CameraMediaRecorder.cpp
    Modules/mediastream/tizen/CameraShootingSound.cpp
    Modules/mediastream/tizen/CameraSize.cpp
    Modules/mediastream/tizen/MediaStreamRecorder.cpp
    Modules/mediastream/tizen/NavigatorTizenCamera.cpp
    Modules/mediastream/tizen/TizenCamera.cpp

    Modules/webaudio/MediaStreamAudioDestinationNode.cpp

    page/scrolling/coordinatedgraphics/tizen/ScrollThumbLayerCoordinator.cpp

    platform/audio/gstreamer/tizen/AudioSessionManagerGStreamerTizen.cpp

    platform/efl/tizen/ClipboardTizen.cpp
    platform/efl/tizen/DataObjectTizen.cpp
    platform/efl/tizen/DeviceMotionClientTizen.cpp
    platform/efl/tizen/DeviceOrientationClientTizen.cpp
    platform/efl/tizen/DeviceOrientationProviderTizen.cpp
    platform/efl/tizen/DeviceMotionProviderTizen.cpp
    platform/efl/tizen/LocalizedStringsTizen.cpp
    platform/efl/tizen/PasteboardTizen.cpp
    platform/efl/tizen/PlatformSpeechSynthesizerTizen.cpp
    platform/efl/tizen/PlatformSynthesisProviderTizen.cpp
    platform/efl/tizen/ScrollbarThemeTizen.cpp
    platform/efl/tizen/SpeechRecognitionProviderTizen.cpp
    platform/efl/tizen/SSLKeyGeneratorTizen.cpp
    platform/efl/tizen/SSLPrivateKeyStoreTizen.cpp
    platform/efl/tizen/STTProviderTizen.cpp
    platform/efl/tizen/TizenExtensibleAPI.cpp
    platform/efl/tizen/TizenLinkEffect.cpp
    platform/efl/tizen/TizenProfiler.cpp

    platform/mediastream/tizen/CameraCapabilitiesImpl.cpp
    platform/mediastream/tizen/CameraControlImpl.cpp
    platform/mediastream/tizen/CameraImageCaptureImpl.cpp
    platform/mediastream/tizen/LocalMediaServer.cpp
    platform/mediastream/tizen/MediaStreamCenterTizen.cpp
    platform/mediastream/tizen/MediaStreamManager.cpp
    platform/mediastream/tizen/MediaRecorder.cpp
    platform/mediastream/tizen/MediaRecorderPrivateImpl.cpp

    platform/network/soup/tizen/ResourceHandleSoupTizen.cpp
    platform/network/soup/tizen/SocketStreamHandleSoupTizen.cpp

    platform/graphics/efl/tizen/Canvas2DLayerTizen.cpp
    platform/graphics/efl/tizen/CoordinatedBackingStoreTizen.cpp
    platform/graphics/efl/tizen/PlatformSurfacePoolTizen.cpp
    platform/graphics/efl/tizen/PlatformSurfaceTexturePoolTizen.cpp
    platform/graphics/efl/tizen/SharedPlatformSurfaceTizen.cpp
    platform/graphics/efl/tizen/SharedVideoPlatformSurfaceTizen.cpp
    platform/graphics/efl/tizen/VideoLayerTizen.cpp
    platform/graphics/gpu/tizen/DrawingBufferTizen.cpp
    platform/graphics/gstreamer/FullscreenVideoControllerGStreamer.cpp
    platform/graphics/gstreamer/URIUtils.cpp
    platform/graphics/gstreamer/MP4Parser.cpp
    platform/graphics/gstreamer/GSTParse.cpp
    platform/graphics/gstreamer/tizen/WebKitCameraSourceGStreamerTizen.cpp
    platform/graphics/gstreamer/tizen/CDMPrivateClearKeyTizen.cpp
    platform/graphics/gstreamer/tizen/CDMPrivatePlayReadyTizen.cpp
    platform/graphics/texmap/coordinated/efl/tizen/CoordinatedTileTizen.cpp
    platform/graphics/texmap/coordinated/efl/tizen/EdgeEffectController.cpp
    platform/graphics/texmap/coordinated/efl/tizen/ScrollbarFadeAnimationController.cpp
    platform/graphics/texmap/efl/tizen/PlatformSurfaceTextureGL.cpp
)

if (ENABLE_TIZEN_ALL_IN_ONE)
    list(APPEND WebCore_SOURCES
        editing/EditingAllInOne.cpp
        inspector/InspectorAllInOne.cpp
        rendering/svg/RenderSVGAllInOne.cpp
        svg/SVGAllInOne.cpp
    )
endif ()

if (WTF_USE_3D_GRAPHICS)
    if (WTF_USE_EGL)
        list(REMOVE_ITEM WebCore_SOURCES
            platform/graphics/surfaces/efl/GraphicsSurfaceCommon.cpp
        )
        list(APPEND WebCore_SOURCES
            platform/graphics/surfaces/efl/GraphicsSurfaceEfl.cpp # syt
            platform/graphics/surfaces/efl/tizen/GraphicsSurfaceTizen.cpp
        )
    endif ()
endif ()

if (ENABLE_TIZEN_TV_PROFILE)
    INCLUDE_IF_EXISTS(${WEBCORE_DIR}/PlatformTizenTV.cmake)
endif ()

# Define directory of images.
ADD_DEFINITIONS("-DIMAGE_DIR=\"${CMAKE_INSTALL_PREFIX}/share/${WebKit2_OUTPUT_NAME}-${PROJECT_VERSION_MAJOR}/images/\"")
