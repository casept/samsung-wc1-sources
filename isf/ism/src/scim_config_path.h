/**
 * @file scim_config_path.h
 * @brief This file defines some common used configuration keys.
 */

/* ISF is based on SCIM 1.4.7 and extended for supporting more mobile fitable. */

/*
 * Smart Common Input Method
 *
 * Copyright (c) 2002-2005 James Su <suzhe@tsinghua.org.cn>
 *
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA  02111-1307  USA
 *
 * $Id: scim_config_path.h,v 1.35 2005/06/26 16:35:33 suzhe Exp $
 */

#ifndef __SCIM_CONFIG_PATH_H
#define __SCIM_CONFIG_PATH_H

namespace scim {
/**
 * @addtogroup Config
 * @ingroup InputServiceFramework
 * @{
 */

#define SCIM_CONFIG_UPDATE_TIMESTAMP                                "/UpdateTimeStamp"

#define SCIM_CONFIG_DEFAULT_IMENGINE_FACTORY                        "/DefaultIMEngineFactory"
#define SCIM_CONFIG_DEFAULT_HELPER_ISE                              "/DefaultHelperISE"

#define SCIM_CONFIG_FRONTEND_ON_THE_SPOT                            "/FrontEnd/OnTheSpot"
#define SCIM_CONFIG_FRONTEND_SHARED_INPUT_METHOD                    "/FrontEnd/SharedInputMethod"
#define SCIM_CONFIG_FRONTEND_IM_OPENED_BY_DEFAULT                   "/FrontEnd/IMOpenedByDefault"

#define SCIM_CONFIG_HOTKEYS_FRONTEND                                "/Hotkeys/FrontEnd"
#define SCIM_CONFIG_HOTKEYS_FRONTEND_TRIGGER                        "/Hotkeys/FrontEnd/Trigger"
#define SCIM_CONFIG_HOTKEYS_FRONTEND_ON                             "/Hotkeys/FrontEnd/On"
#define SCIM_CONFIG_HOTKEYS_FRONTEND_OFF                            "/Hotkeys/FrontEnd/Off"
#define SCIM_CONFIG_HOTKEYS_FRONTEND_NEXT_FACTORY                   "/Hotkeys/FrontEnd/NextFactory"
#define SCIM_CONFIG_HOTKEYS_FRONTEND_PREVIOUS_FACTORY               "/Hotkeys/FrontEnd/PreviousFactory"
#define SCIM_CONFIG_HOTKEYS_FRONTEND_SHOW_FACTORY_MENU              "/Hotkeys/FrontEnd/ShowFactoryMenu"
#define SCIM_CONFIG_HOTKEYS_FRONTEND_VALID_KEY_MASK                 "/Hotkeys/FrontEnd/ValidKeyMask"
#define SCIM_CONFIG_HOTKEYS_FRONTEND_HIDE_ISE                       "/Hotkeys/FrontEnd/HideISE"
#define SCIM_CONFIG_HOTKEYS_FRONTEND_IGNORE_KEY                     "/Hotkeys/FrontEnd/IgnoreKey"

#define SCIM_CONFIG_HOTKEYS_IMENGINE                                "/Hotkeys/IMEngine"
#define SCIM_CONFIG_HOTKEYS_IMENGINE_LIST                           "/Hotkeys/IMEngine/List"
#define SCIM_CONFIG_HOTKEYS_HELPER                                  "/Hotkeys/Helper"
#define SCIM_CONFIG_HOTKEYS_HELPER_LIST                             "/Hotkeys/Helper/List"

#define SCIM_CONFIG_FILTER_FILTERED_IMENGINES                       "/Filter/FilteredIMEngines"
#define SCIM_CONFIG_FILTER_FILTERED_IMENGINES_LIST                  "/Filter/FilteredIMEngines/List"

#define SCIM_GLOBAL_CONFIG_DISABLED_IMENGINE_FACTORIES              "/DisabledIMEngineFactories"
#define SCIM_GLOBAL_CONFIG_SUPPORTED_UNICODE_LOCALES                "/SupportedUnicodeLocales"
#define SCIM_GLOBAL_CONFIG_DEFAULT_KEYBOARD_LAYOUT                  "/DefaultKeyboardLayout"
#define SCIM_GLOBAL_CONFIG_DEFAULT_PANEL_PROGRAM                    "/DefaultPanelProgram"
#define SCIM_GLOBAL_CONFIG_DEFAULT_CONFIG_MODULE                    "/DefaultConfigModule"
#define SCIM_GLOBAL_CONFIG_DEFAULT_SOCKET_FRONTEND_ADDRESS          "/DefaultSocketFrontEndAddress"
#define SCIM_GLOBAL_CONFIG_DEFAULT_SOCKET_IMENGINE_ADDRESS          "/DefaultSocketIMEngineAddress"
#define SCIM_GLOBAL_CONFIG_DEFAULT_SOCKET_CONFIG_ADDRESS            "/DefaultSocketConfigAddress"
#define SCIM_GLOBAL_CONFIG_DEFAULT_PANEL_SOCKET_ADDRESS             "/DefaultPanelSocketAddress"
#define SCIM_GLOBAL_CONFIG_DEFAULT_HELPER_MANAGER_SOCKET_ADDRESS    "/DefaultHelperManagerSocketAddress"
#define SCIM_GLOBAL_CONFIG_DEFAULT_SOCKET_TIMEOUT                   "/DefaultSocketTimeout"
#define SCIM_GLOBAL_CONFIG_PANEL_SOUND_FEEDBACK                     "/Panel/CandidateSoundFeedback"
#define SCIM_GLOBAL_CONFIG_PANEL_VIBRATION_FEEDBACK                 "/Panel/CandidateVibrationFeedback"

#define SCIM_GLOBAL_CONFIG_INITIAL_ISE_TYPE                         "/InitialIseType"
#define SCIM_GLOBAL_CONFIG_INITIAL_ISE_UUID                         "/InitialIseUuid"
#define SCIM_GLOBAL_CONFIG_DEFAULT_ISE_UUID                         "/DefaultIseUuid"
#define SCIM_GLOBAL_CONFIG_PRELOAD_KEYBOARD_ISE                     "/PreloadKeyboardIse"
#define SCIM_GLOBAL_CONFIG_LAUNCH_ISE_ON_REQUEST                    "/LaunchIseOnRequest"
#define SCIM_GLOBAL_CONFIG_CHANGE_KEYBOARD_MODE_BY_TOUCH            "/ChangeKeyboardModeByTouch"
#define SCIM_GLOBAL_CONFIG_CHANGE_KEYBOARD_MODE_BY_FOCUS_MOVE       "/ChangeKeyboardModeByFocusMove"
#define SCIM_GLOBAL_CONFIG_HIDE_ISE_BASED_ON_FOCUS                  "/HideIseBasedOnFocus"
#define SCIM_GLOBAL_CONFIG_SUPPORT_HW_KEYBOARD_MODE                 "/SupportHWKeyboardMode"

#define ISF_CONFIG_HARDWARE_KEYBOARD_DETECT                         "/isf/hw_keyboard_detect"

/** @} */

} // namespace scim

#endif //__SCIM_CONFIG_PATH_H
/*
vi:ts=4:nowrap:ai:expandtab
*/

