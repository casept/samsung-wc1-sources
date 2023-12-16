macro(GENERATE_THEME _target_name _name _option)
    set(DefaultTheme_DIR "${WEBCORE_DIR}/platform/efl/DefaultTheme")

    add_custom_command(
        OUTPUT ${THEME_BINARY_DIR}/${_name}
        COMMAND ${EDJE_CC_EXECUTABLE} -v -no-save ${_option} default.edc ${THEME_BINARY_DIR}/${_name}
        DEPENDS
            default.edc
#if ENABLE_TIZEN_WEARABLE_PROFILE
            widget/button/wearable_button.edc
            widget/button/00_button_normal.png
            widget/button/00_button_focus.png
            widget/button/00_button_press.png
#else
            widget/button/button.edc
            widget/button/00_button_01.png
            widget/button/00_button_01_dim.png
            widget/button/00_button_01_focus.png
#endif
            widget/check/check.edc
            widget/check/00_check_activated.png
            widget/check/00_check_activated_dim.png
            widget/check/00_check_bg.png
            widget/check/00_check_dim_bg.png
            widget/check/00_check_focus.png
            widget/combo/combo.edc
            widget/combo/00_combo_focus_border.png
            widget/combo/00_combo_focus_button.png
            widget/combo/00_combo_focus.png
            widget/combo/00_combo_border.png
            widget/combo/00_combo_normal_button.png
            widget/combo/00_combo_normal.png
            widget/combo/00_combo_press_button.png
            widget/combo/00_combo_press.png
            widget/combo/00_icon.png
            widget/entry/entry.edc
            widget/entry/00_focused.png
            widget/entry/00_normal.png
            widget/mediacontrol/fullscreenbutton/enterfullscreenbutton.png
            widget/mediacontrol/fullscreenbutton/exitfullscreenbutton.png
            widget/mediacontrol/fullscreenbutton/fullscreen_button.edc
            widget/mediacontrol/mutebutton/mute_button.edc
            widget/mediacontrol/mutebutton/mutebutton.png
            widget/mediacontrol/mutebutton/unmutebutton.png
#if ENABLE_TIZEN_WEARABLE_PROFILE
            widget/mediacontrol/playpausebutton/wearable_pausebutton.png
            widget/mediacontrol/playpausebutton/wearable_pausebutton_press.png
            widget/mediacontrol/playpausebutton/wearable_playbutton.png
            widget/mediacontrol/playpausebutton/wearable_playbutton_press.png
            widget/mediacontrol/playpausebutton/wearable_playpause_button.edc
#else
            widget/mediacontrol/playpausebutton/pausebutton.png
            widget/mediacontrol/playpausebutton/playbutton.png
            widget/mediacontrol/playpausebutton/playpause_button.edc
#endif
            widget/mediacontrol/seekbackwardbutton/seekbackward_button.edc
            widget/mediacontrol/seekbackwardbutton/seekbackwardbutton.png
            widget/mediacontrol/seekforwardbutton/seekforward_button.edc
            widget/mediacontrol/seekforwardbutton/seekforwardbutton.png
            widget/mediacontrol/togglecaptionsbutton/closedcaption_disabled.png
            widget/mediacontrol/togglecaptionsbutton/closedcaption.png
            widget/mediacontrol/togglecaptionsbutton/toggle_captions_button.edc
#if ENABLE_TIZEN_VIDEO
            widget/mediacontrol/sliderthumb/sliderthumb.png
            widget/mediacontrol/sliderthumb/sliderthumb_press.png
            widget/mediacontrol/sliderthumb/slider_thumb.edc
#endif
            widget/progressbar/bt_base.png
            widget/progressbar/progressbar.edc
            widget/progressbar/shelf_inset.png
            widget/radio/radio.edc
            widget/radio/00_button_radio_activated.png
            widget/radio/00_button_radio_activated_dim.png
            widget/radio/00_button_radio_bg.png
            widget/radio/00_button_radio_dim_bg.png
            widget/radio/00_button_radio_focus.png
            widget/scrollbar/scrollbar.edc
            widget/scrollbar/scrollbar_h.png
            widget/scrollbar/scrollbar_knob_h.png
            widget/scrollbar/scrollbar_knob_v.png
            widget/scrollbar/scrollbar_v.png
            widget/search/cancel/cancel_normal_button2.png
            widget/search/cancel/cancel_normal_button.png
            widget/search/cancel/search_cancel.edc
            widget/search/decoration/decoration_normal_button.png
            widget/search/decoration/search_decoration.edc
            widget/search/field/field_focused.png
            widget/search/field/field_normal.png
            widget/search/field/search_field.edc
#if ENABLE_TIZEN_WEARABLE_PROFILE
            widget/slider/wearable_slider.edc
            widget/slider/00_slider_h.png
            widget/slider/00_slider_fill_h.png
            widget/slider/00_slider_knob_h.png
            widget/slider/00_slider_knob_press_h.png
            widget/slider/00_slider_knob_disable_h.png
#else
            widget/slider/slider.edc
            widget/slider/slider_h.png
            widget/slider/slider_fill_h.png
            widget/slider/slider_knob_h.png
            widget/slider/slider_knob_press_h.png
#endif
            widget/slider/slider_fill_v.png
            widget/slider/slider_knob_press_v.png
            widget/slider/slider_knob_v.png
            widget/slider/slider_v.png
            widget/spinner/sp_down_default.png
            widget/spinner/sp_down_pressed.png
            widget/spinner/spinner.edc
            widget/spinner/sp_up_default.png
            widget/spinner/sp_up_pressed.png
        WORKING_DIRECTORY ${DefaultTheme_DIR}
        VERBATIM
    )

    add_custom_target(${_target_name} ALL
        DEPENDS ${THEME_BINARY_DIR}/${_name}
    )
endmacro()

set(DefaultTheme_DEFINITION "")
if (ENABLE_PROGRESS_ELEMENT)
    list(APPEND DefaultTheme_DEFINITION "-DENABLE_PROGRESS_ELEMENT")
endif ()

if (ENABLE_TIZEN_WEARABLE_PROFILE)
    list(APPEND DefaultTheme_DEFINITION "-DENABLE_TIZEN_WEARABLE_PROFILE")
endif ()

if (ENABLE_TIZEN_SUPPORT)
    if (NOT ENABLE_TIZEN_TV_PROFILE)
        list(APPEND DefaultTheme_DEFINITION "-DENABLE_TIZEN_NO_CURSOR")
    endif ()

    # FIXME: Should we install our default theme to share/edje insted of share/ewebkit-0/themes?
    GENERATE_THEME(DefaultTheme "webkit.edj" "${DefaultTheme_DEFINITION}")
    install(FILES "${THEME_BINARY_DIR}/webkit.edj" DESTINATION share/edje)
else ()
    GENERATE_THEME(DefaultTheme "default.edj" "${DefaultTheme_DEFINITION}")
    install(FILES "${THEME_BINARY_DIR}/default.edj" DESTINATION ${DATA_INSTALL_DIR}/themes)
endif ()

if (ENABLE_API_TESTS)
    GENERATE_THEME(BigButtonTheme "big_button_theme.edj" "-DBIG_BUTTON_THEME_FOR_TESTING")
endif ()
