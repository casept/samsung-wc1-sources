#PlatformTizenTV.cmake is to include Tizen TV specific files in Tizen TV build.

list(REMOVE_ITEM WebCore_SOURCES
    platform/efl/GamepadsEfl.cpp
    platform/efl/PasteboardEfl.cpp
)

list(APPEND WebCore_IDL_FILES
    Modules/gamepad/WebKitGamepadEvent.idl
    Modules/speech/DOMWindowSpeech.idl
)

list(APPEND WebCore_SOURCES
    Modules/gamepad/GamepadEventController.cpp
    Modules/gamepad/WebKitGamepadEvent.cpp

    platform/efl/tizen/GamepadsProviderTizen.cpp
    platform/efl/tizen/PlatformSpeechSynthesizerTizen.cpp
    platform/efl/tizen/PlatformSynthesisProviderTizen.cpp
    platform/efl/tizen/STTProviderTizen.cpp
    platform/efl/tizen/SpeechRecognitionProviderTizen.cpp
    platform/efl/tizen/PasteboardTizen.cpp
)

list(APPEND WebCore_INCLUDE_DIRECTORIES
    ${STT_INCLUDE_DIRS}
    ${TTS_INCLUDE_DIRS}
)
list(APPEND WebCore_LIBRARIES
    ${STT_LIBRARIES}
    ${TTS_LIBRARIES}
)
