macro(GENERATE_THEME _target_name _name _option)
    set(LegacyTheme_DIR "${WEBCORE_DIR}/platform/efl/WearableTheme")

    add_custom_command(
        OUTPUT ${LEGACY_THEME_BINARY_DIR}/${_name}
        COMMAND ${EDJE_CC_EXECUTABLE} -v -no-save ${_option} default.edc ${LEGACY_THEME_BINARY_DIR}/${_name}
        DEPENDS
            default.edc
            widget/button/button.edc
            widget/button/00_button_normal.png
            widget/button/00_button_focus.png
            widget/button/00_button_press.png
            widget/check/check.edc
            widget/check/00_check_enabled.png
            widget/check/00_check_pressed.png
            widget/check/00_check_disabled.png
            widget/check/00_checkbox_enabled.png
            widget/check/00_checkbox_pressed.png
            widget/check/00_checkbox_disabled.png
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
            widget/mediacontrol/playpausebutton/pausebutton.png
            widget/mediacontrol/playpausebutton/pausebutton_press.png
            widget/mediacontrol/playpausebutton/playbutton.png
            widget/mediacontrol/playpausebutton/playbutton_press.png
            widget/mediacontrol/playpausebutton/playpause_button.edc
            widget/mediacontrol/seekbackwardbutton/seekbackward_button.edc
            widget/mediacontrol/seekbackwardbutton/seekbackwardbutton.png
            widget/mediacontrol/seekforwardbutton/seekforward_button.edc
            widget/mediacontrol/seekforwardbutton/seekforwardbutton.png
            widget/mediacontrol/togglecaptionsbutton/closedcaption_disabled.png
            widget/mediacontrol/togglecaptionsbutton/closedcaption.png
            widget/mediacontrol/togglecaptionsbutton/toggle_captions_button.edc
            widget/progressbar/bt_base.png
            widget/progressbar/progressbar.edc
            widget/progressbar/shelf_inset.png
            widget/radio/radio.edc
            widget/radio/tw_btn_radio_holo_dark.png
            widget/radio/tw_btn_radiobox_holo_dark.png
            widget/radio/tw_btn_radiobox_focus_line.png
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
            widget/slider/slider.edc
            widget/slider/00_slider_h.png
            widget/slider/00_slider_fill_h.png
            widget/slider/00_slider_knob_h.png
            widget/slider/00_slider_knob_press_h.png
            widget/slider/00_slider_knob_disable_h.png
            widget/slider/slider_fill_v.png
            widget/slider/slider_knob_press_v.png
            widget/slider/slider_knob_v.png
            widget/slider/slider_v.png
            widget/spinner/sp_down_default.png
            widget/spinner/sp_down_pressed.png
            widget/spinner/spinner.edc
            widget/spinner/sp_up_default.png
            widget/spinner/sp_up_pressed.png
        WORKING_DIRECTORY ${LegacyTheme_DIR}
        VERBATIM
    )

    add_custom_target(${_target_name} ALL
        DEPENDS ${LEGACY_THEME_BINARY_DIR}/${_name}
    )
endmacro()

set(LegacyTheme_DEFINITION "")
if (ENABLE_PROGRESS_ELEMENT)
    list(APPEND LegacyTheme_DEFINITION "-DENABLE_PROGRESS_ELEMENT")
endif ()

if (ENABLE_TIZEN_SUPPORT)
    list(APPEND LegacyTheme_DEFINITION "-DENABLE_TIZEN_SUPPORT")

    # FIXME: Should we install our default theme to share/edje insted of share/ewebkit-0/themes?
    GENERATE_THEME(LegacyTheme "legacyWebkit.edj" "${LegacyTheme_DEFINITION}")
    install(FILES "${LEGACY_THEME_BINARY_DIR}/legacyWebkit.edj" DESTINATION share/edje)
else ()
    GENERATE_THEME(LegacyTheme "default.edj" "${LegacyTheme_DEFINITION}")
    install(FILES "${LEGACY_THEME_BINARY_DIR}/default.edj" DESTINATION ${DATA_INSTALL_DIR}/themes)
endif ()

if (ENABLE_API_TESTS)
    GENERATE_THEME(BigButtonLegacyTheme "big_button_theme.edj" "-DBIG_BUTTON_THEME_FOR_TESTING")
endif ()
