/**
 * @file scim_helper.h
 * @brief Defines scim::HelperAgent and it's related types.
 *
 * scim::HelperAgent is a class used to write Client Helper modules.
 *
 */

/* ISF is based on SCIM 1.4.7 and extended for supporting more mobile fitable. */

/*
 * Smart Common Input Method
 *
 * Copyright (c) 2004-2005 James Su <suzhe@tsinghua.org.cn>
 * Copyright (c) 2012-2014 Samsung Electronics Co., Ltd.
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
 * Modifications by Samsung Electronics Co., Ltd.
 * 1. Add new interface APIs for keyboard ISE
 *    a. expand_candidate (), contract_candidate () and set_candidate_style ()
 *    b. set_keyboard_ise_by_uuid () and reset_keyboard_ise ()
 *    c. get_surrounding_text () and delete_surrounding_text ()
 *    d. show_preedit_string (), hide_preedit_string (), update_preedit_string () and update_preedit_caret ()
 *    e. show_candidate_string (), hide_candidate_string () and update_candidate_string ()
 *
 * $Id: scim_helper.h,v 1.16 2005/05/24 12:22:51 suzhe Exp $
 */

#ifndef __SCIM_HELPER_H
#define __SCIM_HELPER_H

#include "scim_utility.h"

namespace scim {

/**
 * @addtogroup Helper
 * @ingroup InputServiceFramework
 * The accessory classes to help develop and manage Client Helper objects.
 * @{
 */
class EAPI HelperError: public Exception
{
public:
    HelperError (const String& what_arg)
        : Exception (String("scim::Helper: ") + what_arg) { }
};

/**
 * @brief Helper option indicates that it's a stand alone Helper.
 *
 * Stand alone Helper has no corresponding IMEngine Factory,
 * Such Helper can not be started by IMEngine Factory.
 * So Panel must provide a menu, or something else, which contains
 * all stand alone Helper items, so that user can start them by
 * clicking the items.
 */
const uint32 SCIM_HELPER_STAND_ALONE             = 1;

/**
 * @brief Helper option indicates that it must be started
 * automatically when Panel starts.
 *
 * If Helper objects want to start itself as soon as
 * the Panel starts, set this option.
 */
const uint32 SCIM_HELPER_AUTO_START              = (1<<1);

/**
 * @brief Helper option indicates that it should be restarted
 * when it exits abnormally.
 *
 * This option should not be used with #SCIM_HELPER_STAND_ALONE.
 */
const uint32 SCIM_HELPER_AUTO_RESTART            = (1<<2);

/**
 * @brief Helper option indicates that it needs the screen update
 * information.
 *
 * Helper object with this option will receive the UPDATE_SCREEN event
 * when the screen of focused ic is changed.
 */
const uint32 SCIM_HELPER_NEED_SCREEN_INFO        = (1<<3);

/**
 * @brief Helper option indicates that it needs the spot location
 * information.
 *
 * Helper object with this option will receive the SPOT_LOCATION_INFO event
 * when the spot location of focused ic is changed.
 */
const uint32 SCIM_HELPER_NEED_SPOT_LOCATION_INFO = (1<<4);

/**
 * @brief ISE option indicates whether helper ISE handles the keyboard keyevent
 */
const uint32 ISM_HELPER_PROCESS_KEYBOARD_KEYEVENT = (1<<16);

/**
 * @brief ISE option indicates whether it should be hidden in control panel.
 */
const uint32 ISM_ISE_HIDE_IN_CONTROL_PANEL       = (1<<17);


/**
 * @brief Structure to hold the information of a Helper object.
 */
struct HelperInfo
{
    String uuid;            /**< The UUID of this Helper object */
    String name;            /**< The Name of this Helper object, UTF-8 encoding. */
    String icon;            /**< The Icon file path of this Helper object. */
    String description;     /**< The short description of this Helper object. */
    uint32 option;          /**< The options of this Helper object. @sa #SCIM_HELPER_STAND_ALONE etc.*/

    HelperInfo (const String &puuid = String (),
                const String &pname = String (),
                const String &picon = String (),
                const String &pdesc = String (),
                uint32 opt = 0)
        : uuid (puuid),
          name (pname),
          icon (picon),
          description (pdesc),
          option (opt) {
    }
};

enum HelperState
{
    HELPER_SHOWED,
    HELPER_HIDED,
    HELPER_INVALID_STATE
};

class HelperAgent;

typedef Slot3<void, const HelperAgent *, int, const String &>
        HelperAgentSlotVoid;

typedef Slot4<void, const HelperAgent *, int, const String &, const String &>
        HelperAgentSlotString;

typedef Slot4<void, const HelperAgent *, int, const String &, const std::vector<String> &>
        HelperAgentSlotStringVector;

typedef Slot5<void, const HelperAgent *, int, const String &, const String &, const String &>
        HelperAgentSlotString2;

typedef Slot4<void, const HelperAgent *, int, const String &, int>
        HelperAgentSlotInt;

typedef Slot5<void, const HelperAgent *, int, const String &, int, int>
        HelperAgentSlotIntInt;

typedef Slot4<void, const HelperAgent *, int, const String &, const Transaction &>
        HelperAgentSlotTransaction;

typedef Slot4<void, const HelperAgent *, int, const String &, const rectinfo &>
        HelperAgentSlotRect;

typedef Slot2<void, const HelperAgent *, struct rectinfo &>
        HelperAgentSlotSize;

typedef Slot2<void, const HelperAgent *, uint32 &>
        HelperAgentSlotUintVoid;

typedef Slot3<void, const HelperAgent *, int, uint32 &>
        HelperAgentSlotIntUint;

typedef Slot3<void, const HelperAgent *, char *, size_t &>
        HelperAgentSlotRawVoid;

typedef Slot3<void, const HelperAgent *, char **, size_t &>
        HelperAgentSlotGetRawVoid;

typedef Slot4<void, const HelperAgent *, int, char *, size_t &>
        HelperAgentSlotIntRawVoid;

typedef Slot3<void, const HelperAgent *, int, char **>
        HelperAgentSlotIntGetStringVoid;

typedef Slot2<void, const HelperAgent *, const std::vector<uint32> &>
        HelperAgentSlotUintVector;

typedef Slot2<void, const HelperAgent *, LookupTable &>
        HelperAgentSlotLookupTable;
typedef Slot3<void, const HelperAgent *, KeyEvent &, uint32 &>
        HelperAgentSlotKeyEventUint;

/**
 * @brief The accessory class to write a Helper object.
 *
 * This class implements all Socket Transaction protocol between
 * Helper object and Panel.
 */
class EAPI HelperAgent
{
    class HelperAgentImpl;
    HelperAgentImpl *m_impl;

    HelperAgent (const HelperAgent &);
    const HelperAgent & operator = (const HelperAgent &);

public:
    HelperAgent  ();
    ~HelperAgent ();

    /**
     * @brief Open socket connection to the Panel.
     *
     * Helper objects and Panel communicate with each other via the Socket
     * created by Panel, just same as the Socket between FrontEnds and Panel.
     *
     * Helper object can select/poll on the connection id returned by this function
     * to see if there are any data available to be read. If any data are available,
     * Helper object should call HelperAgent::filter_event() to process the data.
     *
     * This method should be called after the necessary signal-slots are connected.
     * If this Helper is started by an IMEngine Instance, then signal attach_input_context
     * will be emitted during this call.
     *
     * Signal update_screen will be emitted during this call as well to set the startup
     * screen of this Helper. The ic and ic_uuid parameters are invalid here.
     *
     * @param info The information of this Helper object.
     * @param display The display which this Helper object should run on.
     *
     * @return The connection socket id. -1 means failed to create
     *         the connection.
     */
    int  open_connection        (const HelperInfo   &info,
                                 const String       &display);

    /**
     * @brief Close the socket connection to Panel.
     */
    void close_connection       ();

    /**
     * @brief Get the connection id previously returned by open_connection().
     *
     * @return the connection id
     */
    int  get_connection_number  () const;

    /**
     * @brief Check whether this HelperAgent has been connected to a Panel.
     *
     * Return true when it is connected to panel, otherwise return false.
     */
    bool is_connected           () const;

    /**
     * @brief Check if there are any events available to be processed.
     *
     * If it returns true then Helper object should call
     * HelperAgent::filter_event() to process them.
     *
     * @return true if there are any events available.
     */
    bool has_pending_event      () const;

    /**
     * @brief Process the pending events.
     *
     * This function will emit the corresponding signals according
     * to the events.
     *
     * @return false if the connection is broken, otherwise return true.
     */
    bool filter_event           ();

    /**
     * @brief Request SCIM to reload all configuration.
     *
     * This function should only by used by Setup Helper to request
     * scim's reloading the configuration.
     */
    void reload_config          () const;

    /**
     * @brief Register some properties into Panel.
     *
     * This function send the request to Panel to register a list
     * of Properties.
     *
     * @param properties The list of Properties to be registered into Panel.
     *
     * @sa scim::Property.
     */
    void register_properties    (const PropertyList &properties) const;

    /**
     * @brief Update a registered property.
     *
     * @param property The property to be updated.
     */
    void update_property        (const Property     &property) const;

    /**
     * @brief Send a set of events to an IMEngineInstance.
     *
     * All events should be put into a Transaction.
     * And the events can only be received by one IMEngineInstance object.
     *
     * @param ic The handle of the Input Context to receive the events.
     * @param ic_uuid The UUID of the Input Context.
     * @param trans The Transaction object holds the events.
     */
    void send_imengine_event    (int                 ic,
                                 const String       &ic_uuid,
                                 const Transaction  &trans) const;

    /**
     * @brief Send a KeyEvent to an IMEngineInstance.
     *
     * @param ic The handle of the IMEngineInstance to receive the event.
     *        -1 means the currently focused IMEngineInstance.
     * @param ic_uuid The UUID of the IMEngineInstance. Empty means don't match.
     * @param key The KeyEvent to be sent.
     */
    void send_key_event         (int                 ic,
                                 const String       &ic_uuid,
                                 const KeyEvent     &key) const;

    /**
     * @brief Forward a KeyEvent to client application directly.
     *
     * @param ic The handle of the client Input Context to receive the event.
     *        -1 means the currently focused Input Context.
     * @param ic_uuid The UUID of the IMEngine used by the Input Context.
     *        Empty means don't match.
     * @param key The KeyEvent to be forwarded.
     */
    void forward_key_event      (int                 ic,
                                 const String       &ic_uuid,
                                 const KeyEvent     &key) const;

    /**
     * @brief Commit a WideString to client application directly.
     *
     * @param ic The handle of the client Input Context to receive the WideString.
     *        -1 means the currently focused Input Context.
     * @param ic_uuid The UUID of the IMEngine used by the Input Context.
     *        Empty means don't match.
     * @param wstr The WideString to be committed.
     */
    void commit_string          (int                 ic,
                                 const String       &ic_uuid,
                                 const WideString   &wstr) const;

    /**
     * @brief Commit a UTF-8 String to client application directly.
     *
     * @param ic The handle of the client Input Context to receive the commit string.
     *        -1 means the currently focused Input Context.
     * @param ic_uuid The UUID of the IMEngine used by the Input Context.
     *        Empty means don't match.
     * @param buf The byte array of UTF-8 string to be committed.
     * @param buflen The buf size in bytes.
     */
    void commit_string          (int                 ic,
                                 const String       &ic_uuid,
                                 const char         *buf,
                                 int                 buflen) const;

    /**
     * @brief Request to show preedit string.
     *
     * @param ic The handle of the client Input Context to receive the request.
     *        -1 means the currently focused Input Context.
     * @param ic_uuid The UUID of the IMEngine used by the Input Context.
     *        Empty means don't match.
     */
    void show_preedit_string    (int                 ic,
                                 const String       &ic_uuid) const;

    /**
     * @brief Request to show aux string.
     */
    void show_aux_string        (void) const;

    /**
     * @brief Request to show candidate string.
     */
    void show_candidate_string  (void) const;

    /**
     * @brief Request to show associate string.
     */
    void show_associate_string  (void) const;

    /**
     * @brief Request to hide preedit string.
     *
     * @param ic The handle of the client Input Context to receive the request.
     *        -1 means the currently focused Input Context.
     * @param ic_uuid The UUID of the IMEngine used by the Input Context.
     *        Empty means don't match.
     */
    void hide_preedit_string    (int                 ic,
                                 const String       &ic_uuid) const;

    /**
     * @brief Request to hide aux string.
     */
    void hide_aux_string        (void) const;

    /**
     * @brief Request to hide candidate string.
     */
    void hide_candidate_string  (void) const;

    /**
     * @brief Request to hide associate string.
     */
    void hide_associate_string  (void) const;

    /**
     * @brief Update a new WideString for preedit.
     *
     * @param ic The handle of the client Input Context to receive the WideString.
     *        -1 means the currently focused Input Context.
     * @param ic_uuid The UUID of the IMEngine used by the Input Context.
     *        Empty means don't match.
     * @param wstr The WideString to be updated.
     * @param attrs The attribute list for preedit string.
     */
    void update_preedit_string  (int                 ic,
                                 const String       &ic_uuid,
                                 const WideString   &wstr,
                                 const AttributeList &attrs) const;

    /**
     * @brief Update a new UTF-8 string for preedit.
     *
     * @param ic The handle of the client Input Context to receive the UTF-8 String.
     *        -1 means the currently focused Input Context.
     * @param ic_uuid The UUID of the IMEngine used by the Input Context.
     *        Empty means don't match.
     * @param buf The byte array of UTF-8 string to be updated.
     * @param buflen The buf size in bytes.
     * @param attrs The attribute list for preedit string.
     */
    void update_preedit_string  (int                 ic,
                                 const String       &ic_uuid,
                                 const char         *buf,
                                 int                 buflen,
                                 const AttributeList &attrs) const;

    /**
     * @brief Update a new WideString and caret for preedit.
     *
     * @param ic The handle of the client Input Context to receive the WideString.
     *        -1 means the currently focused Input Context.
     * @param ic_uuid The UUID of the IMEngine used by the Input Context.
     *        Empty means don't match.
     * @param wstr The WideString to be updated.
     * @param attrs The attribute list for preedit string.
     * @param caret The caret position in preedit string.
     */
    void update_preedit_string  (int                 ic,
                                 const String       &ic_uuid,
                                 const WideString   &wstr,
                                 const AttributeList &attrs,
                                 int                 caret) const;

    /**
     * @brief Update a new UTF-8 string and caret for preedit.
     *
     * @param ic The handle of the client Input Context to receive the UTF-8 String.
     *        -1 means the currently focused Input Context.
     * @param ic_uuid The UUID of the IMEngine used by the Input Context.
     *        Empty means don't match.
     * @param buf The byte array of UTF-8 string to be updated.
     * @param buflen The buf size in bytes.
     * @param attrs The attribute list for preedit string.
     * @param caret The caret position in preedit string.
     */
    void update_preedit_string  (int                 ic,
                                 const String       &ic_uuid,
                                 const char         *buf,
                                 int                 buflen,
                                 const AttributeList &attrs,
                                 int                 caret) const;

    /**
     * @brief Update a new string for aux.
     *
     * @param str The string to be updated.
     * @param attrs The attribute list for aux string.
     */
    void update_aux_string      (const String       &str,
                                 const AttributeList &attrs) const;

    /**
     * @brief Request to update candidate.
     *
     * @param table The lookup table for candidate.
     */
    void update_candidate_string (const LookupTable &table) const;

    /**
     * @brief Request to update associate.
     *
     * @param table The lookup table for associate.
     */
    void update_associate_string (const LookupTable &table) const;

    /**
     * @brief When the input context of ISE is changed,
     *         ISE can call this function to notify application
     *
     * @param type  type of event.
     * @param value value of event.
     */
    void update_input_context     (uint32                       type,
                                   uint32                       value) const;

    /**
     * @brief Request to get surrounding text.
     *
     * @param uuid The helper ISE UUID.
     * @param maxlen_before The max length of before.
     * @param maxlen_after The max length of after.
     */
    void get_surrounding_text     (const String                &uuid,
                                   int                          maxlen_before,
                                   int                          maxlen_after) const;

    /**
     * @brief Request to delete surrounding text.
     *
     * @param offset The offset for cursor position.
     * @param len The length for delete text.
     */
    void delete_surrounding_text  (int                          offset,
                                   int                          len) const;

    /**
     * @brief Request to get selection.
     *
     * @param uuid The helper ISE UUID.
     */
    void get_selection       (const String                &uuid) const;

    /**
     * @brief Request to selected text.
     *
     * @param start The start position in text.
     * @param end The end position in text.
     */
    void set_selection       (int                          start,
                              int                          end) const;

    /**
     * @brief Set candidate position in screen.
     *
     * @param left The x position in screen.
     * @param top The y position in screen.
     */
    void set_candidate_position   (int                          left,
                                   int                          top) const;

    /**
     * @brief Request to hide candidate window.
     */
    void candidate_hide           (void) const;

    /**
     * @brief Request to get candidate window size and position.
     *
     * @param uuid The helper ISE UUID.
     */
    void get_candidate_window_geometry (const String           &uuid) const;

    /**
     * @brief Set current keyboard ISE.
     *
     * @param uuid The keyboard ISE UUID.
     */
    void set_keyboard_ise_by_uuid (const String                &uuid) const;

    /**
     * @brief Request to get current keyboard ISE information.
     *
     * @param uuid The helper ISE UUID.
     */
    void get_keyboard_ise         (const String                &uuid) const;

    /**
     * @brief Request to get uuid list of all keyboard ISEs.
     *
     * @param uuid The helper ISE UUID.
     */
    void get_keyboard_ise_list    (const String                &uuid) const;

    /**
     * @brief Update ISE window geometry.
     *
     * @param x      The x position in screen.
     * @param y      The y position in screen.
     * @param width  The ISE window width.
     * @param height The ISE window height.
     */
    void update_geometry          (int                          x,
                                   int                          y,
                                   int                          width,
                                   int                          height) const;

    /**
     * @brief Request to expand candidate window.
     */
    void expand_candidate         (void) const;

    /**
     * @brief Request to contract candidate window.
     */
    void contract_candidate       (void) const;

    /**
     * @brief Send selected candidate string index number.
     */
    void select_candidate         (int index) const;

    /**
     * @brief Update ise exit status
     */
    void update_ise_exit          (void) const;

    /**
     * @brief Update the preedit caret position in the preedit string.
     *
     * @param caret - the new position of the preedit caret.
     */
    void update_preedit_caret     (int                          caret) const;

    /**
     * @brief Set candidate style.
     *
     * @param portrait_line - the displayed line number for portrait.
     * @param mode          - candidate window mode.
     */
    void set_candidate_style      (ISF_CANDIDATE_PORTRAIT_LINE_T portrait_line = ONE_LINE_CANDIDATE,
                                   ISF_CANDIDATE_MODE_T          mode = FIXED_CANDIDATE_WINDOW) const;

    /**
     * @brief Request to reset keyboard ISE.
     */
    void reset_keyboard_ise       (void) const;

    /**
     * @brief Send a private command to an application
     *
     * @param command The private command sent from IME.
     */
    void send_private_command     (const String                &command) const;

public:
    /**
     * @brief Connect a slot to Helper exit signal.
     *
     * This signal is used to let the Helper exit.
     *
     * The prototype of the slot is:
     *
     * void exit (const HelperAgent *agent, int ic, const String &ic_uuid);
     *
     * Parameters:
     * - agent    The pointer to the HelperAgent object which emits this signal.
     * - ic       An opaque handle of the currently focused input context.
     * - ic_uuid  The UUID of the IMEngineInstance associated with the focused input context.
     */
    Connection signal_connect_exit                   (HelperAgentSlotVoid        *slot);

    /**
     * @brief Connect a slot to Helper attach input context signal.
     *
     * This signal is used to attach an input context to this helper.
     *
     * When an input context requst to start this helper, then this
     * signal will be emitted as soon as the helper is started.
     *
     * When an input context want to start an already started helper,
     * this signal will also be emitted.
     *
     * Helper can send some events back to the IMEngineInstance in this
     * signal-slot, to inform that it has been started sccessfully.
     *
     * The prototype of the slot is:
     *
     * void attach_input_context (const HelperAgent *agent, int ic, const String &ic_uuid);
     */
    Connection signal_connect_attach_input_context   (HelperAgentSlotVoid        *slot);

    /**
     * @brief Connect a slot to Helper detach input context signal.
     *
     * This signal is used to detach an input context from this helper.
     *
     * When an input context requst to stop this helper, then this
     * signal will be emitted.
     *
     * Helper shouldn't send any event back to the IMEngineInstance, because
     * the IMEngineInstance attached to the ic should have been destroyed.
     *
     * The prototype of the slot is:
     *
     * void detach_input_context (const HelperAgent *agent, int ic, const String &ic_uuid);
     */
    Connection signal_connect_detach_input_context   (HelperAgentSlotVoid        *slot);

    /**
     * @brief Connect a slot to Helper reload config signal.
     *
     * This signal is used to let the Helper reload configuration.
     *
     * The prototype of the slot is:
     *
     * void reload_config (const HelperAgent *agent, int ic, const String &ic_uuid);
     */
    Connection signal_connect_reload_config          (HelperAgentSlotVoid        *slot);

    /**
     * @brief Connect a slot to Helper update screen signal.
     *
     * This signal is used to let the Helper move its GUI to another screen.
     * It can only be emitted when SCIM_HELPER_NEED_SCREEN_INFO is set in HelperInfo.option.
     *
     * The prototype of the slot is:
     *
     * void update_screen (const HelperAgent *agent, int ic, const String &ic_uuid, int screen_number);
     */
    Connection signal_connect_update_screen          (HelperAgentSlotInt         *slot);

    /**
     * @brief Connect a slot to Helper update spot location signal.
     *
     * This signal is used to let the Helper move its GUI according to the current spot location.
     * It can only be emitted when SCIM_HELPER_NEED_SPOT_LOCATION_INFO is set in HelperInfo.option.
     *
     * The prototype of the slot is:
     * void update_spot_location (const HelperAgent *agent, int ic, const String &ic_uuid, int x, int y);
     */
    Connection signal_connect_update_spot_location   (HelperAgentSlotIntInt      *slot);

    /**
     * @brief Connect a slot to Helper update cursor position signal.
     *
     * This signal is used to let the Helper get the cursor position information.
     *
     * The prototype of the slot is:
     * void update_cursor_position (const HelperAgent *agent, int ic, const String &ic_uuid, int cursor_pos);
     */
    Connection signal_connect_update_cursor_position (HelperAgentSlotInt         *slot);

    /**
     * @brief Connect a slot to Helper update surrounding text signal.
     *
     * This signal is used to let the Helper get the surrounding text.
     *
     * The prototype of the slot is:
     * void update_surrounding_text (const HelperAgent *agent, int ic, const String &text, int cursor);
     */
    Connection signal_connect_update_surrounding_text (HelperAgentSlotInt        *slot);

    /**
     * @brief Connect a slot to Helper update selection signal.
     *
     * This signal is used to let the Helper get the selection.
     *
     * The prototype of the slot is:
     * void update_selection (const HelperAgent *agent, int ic, const String &text);
     */
    Connection signal_connect_update_selection (HelperAgentSlotVoid        *slot);

    /**
     * @brief Connect a slot to Helper trigger property signal.
     *
     * This signal is used to trigger a property registered by this Helper.
     * A property will be triggered when user clicks on it.
     *
     * The prototype of the slot is:
     * void trigger_property (const HelperAgent *agent, int ic, const String &ic_uuid, const String &property);
     */
    Connection signal_connect_trigger_property       (HelperAgentSlotString      *slot);

    /**
     * @brief Connect a slot to Helper process imengine event signal.
     *
     * This signal is used to deliver the events sent from IMEngine to Helper.
     *
     * The prototype of the slot is:
     * void process_imengine_event (const HelperAgent *agent, int ic, const String &ic_uuid, const Transaction &transaction);
     */
    Connection signal_connect_process_imengine_event (HelperAgentSlotTransaction *slot);

    /**
     * @brief Connect a slot to Helper focus out signal.
     *
     * This signal is used to do something when input context is focus out.
     *
     * The prototype of the slot is:
     * void focus_out (const HelperAgent *agent, int ic, const String &ic_uuid);
     */
    Connection signal_connect_focus_out                         (HelperAgentSlotVoid                *slot);

    /**
     * @brief Connect a slot to Helper focus in signal.
     *
     * This signal is used to do something when input context is focus in.
     *
     * The prototype of the slot is:
     * void focus_in (const HelperAgent *agent, int ic, const String &ic_uuid);
     */
    Connection signal_connect_focus_in                          (HelperAgentSlotVoid                *slot);

    /**
     * @brief Connect a slot to Helper show signal.
     *
     * This signal is used to show Helper ISE window.
     *
     * The prototype of the slot is:
     * void ise_show (const HelperAgent *agent, int ic, char *buf, size_t &len);
     */
    Connection signal_connect_ise_show                          (HelperAgentSlotIntRawVoid          *slot);

    /**
     * @brief Connect a slot to Helper hide signal.
     *
     * This signal is used to hide Helper ISE window.
     *
     * The prototype of the slot is:
     * void ise_hide (const HelperAgent *agent, int ic, const String &ic_uuid);
     */
    Connection signal_connect_ise_hide                          (HelperAgentSlotVoid                *slot);

    /**
     * @brief Connect a slot to Helper get ISE window geometry signal.
     *
     * This signal is used to get Helper ISE window size and position.
     *
     * The prototype of the slot is:
     * void get_geometry (const HelperAgent *agent, struct rectinfo &info);
     */
    Connection signal_connect_get_geometry                      (HelperAgentSlotSize                *slot);

    /**
     * @brief Connect a slot to Helper set mode signal.
     *
     * This signal is used to set Helper ISE mode.
     *
     * The prototype of the slot is:
     * void set_mode (const HelperAgent *agent, uint32 &mode);
     */
    Connection signal_connect_set_mode                          (HelperAgentSlotUintVoid            *slot);

    /**
     * @brief Connect a slot to Helper set language signal.
     *
     * This signal is used to set Helper ISE language.
     *
     * The prototype of the slot is:
     * void set_language (const HelperAgent *agent, uint32 &language);
     */
    Connection signal_connect_set_language                      (HelperAgentSlotUintVoid            *slot);

    /**
     * @brief Connect a slot to Helper set im data signal.
     *
     * This signal is used to send im data to Helper ISE.
     *
     * The prototype of the slot is:
     * void set_imdata (const HelperAgent *agent, char *buf, size_t &len);
     */
    Connection signal_connect_set_imdata                        (HelperAgentSlotRawVoid             *slot);

    /**
     * @brief Connect a slot to Helper get im data signal.
     *
     * This signal is used to get im data from Helper ISE.
     *
     * The prototype of the slot is:
     * void get_imdata (const HelperAgent *, char **buf, size_t &len);
     */
    Connection signal_connect_get_imdata                        (HelperAgentSlotGetRawVoid          *slot);

    /**
     * @brief Connect a slot to Helper get language locale.
     *
     * This signal is used to get language locale from Helper ISE.
     *
     * The prototype of the slot is:
     * void get_language_locale (const HelperAgent *, int ic, char **locale);
     */
    Connection signal_connect_get_language_locale               (HelperAgentSlotIntGetStringVoid    *slot);

    /**
     * @brief Connect a slot to Helper set return key type signal.
     *
     * This signal is used to set return key type to Helper ISE.
     *
     * The prototype of the slot is:
     * void set_return_key_type (const HelperAgent *agent, uint32 &type);
     */
    Connection signal_connect_set_return_key_type               (HelperAgentSlotUintVoid            *slot);

    /**
     * @brief Connect a slot to Helper get return key type signal.
     *
     * This signal is used to get return key type from Helper ISE.
     *
     * The prototype of the slot is:
     * void get_return_key_type (const HelperAgent *agent, uint32 &type);
     */
    Connection signal_connect_get_return_key_type               (HelperAgentSlotUintVoid            *slot);

    /**
     * @brief Connect a slot to Helper set return key disable signal.
     *
     * This signal is used to set return key disable to Helper ISE.
     *
     * The prototype of the slot is:
     * void set_return_key_disable (const HelperAgent *agent, uint32 &disabled);
     */
    Connection signal_connect_set_return_key_disable            (HelperAgentSlotUintVoid            *slot);

    /**
     * @brief Connect a slot to Helper get return key disable signal.
     *
     * This signal is used to get return key disable from Helper ISE.
     *
     * The prototype of the slot is:
     * void get_return_key_disable (const HelperAgent *agent, uint32 &disabled);
     */
    Connection signal_connect_get_return_key_disable            (HelperAgentSlotUintVoid            *slot);

    /**
     * @brief Connect a slot to Helper get layout signal.
     *
     * This signal is used to get Helper ISE layout.
     *
     * The prototype of the slot is:
     * void get_layout (const HelperAgent *agent, uint32 &layout);
     */
    Connection signal_connect_get_layout                        (HelperAgentSlotUintVoid            *slot);

    /**
     * @brief Connect a slot to Helper set layout signal.
     *
     * This signal is used to set Helper ISE layout.
     *
     * The prototype of the slot is:
     * void set_layout (const HelperAgent *agent, uint32 &layout);
     */
    Connection signal_connect_set_layout                        (HelperAgentSlotUintVoid            *slot);

    /**
     * @brief Connect a slot to Helper set shift mode signal.
     *
     * This signal is used to set Helper shift mode.
     *
     * The prototype of the slot is:
     * void set_caps_mode (const HelperAgent *agent, uint32 &mode);
     */
    Connection signal_connect_set_caps_mode                     (HelperAgentSlotUintVoid            *slot);

    /**
     * @brief Connect a slot to Helper reset input context signal.
     *
     * This signal is used to reset Helper ISE input context.
     *
     * The prototype of the slot is:
     * void reset_input_context (const HelperAgent *agent, int ic, const String &uuid);
     */
    Connection signal_connect_reset_input_context               (HelperAgentSlotVoid                *slot);

    /**
     * @brief Connect a slot to Helper update candidate window geometry signal.
     *
     * This signal is used to get candidate window size and position.
     *
     * The prototype of the slot is:
     * void update_candidate_geometry (const HelperAgent *agent, int ic, const String &uuid, const rectinfo &info);
     */
    Connection signal_connect_update_candidate_geometry         (HelperAgentSlotRect                *slot);

    /**
     * @brief Connect a slot to Helper update keyboard ISE signal.
     *
     * This signal is used to get current keyboard ISE name and uuid.
     *
     * The prototype of the slot is:
     * void update_keyboard_ise (const HelperAgent *agent, int ic, const String &uuid,
     *                           const String &ise_name, const String &ise_uuid);
     */
    Connection signal_connect_update_keyboard_ise               (HelperAgentSlotString2             *slot);

    /**
     * @brief Connect a slot to Helper update keyboard ISE list signal.
     *
     * This signal is used to get uuid list of all keyboard ISEs.
     *
     * The prototype of the slot is:
     * void update_keyboard_ise_list (const HelperAgent *agent, int ic, const String &uuid,
     *                                const std::vector<String> &ise_list);
     */
    Connection signal_connect_update_keyboard_ise_list          (HelperAgentSlotStringVector        *slot);

    /**
     * @brief Connect a slot to Helper candidate more window show signal.
     *
     * This signal is used to do someting when candidate more window is showed.
     *
     * The prototype of the slot is:
     * void candidate_more_window_show (const HelperAgent *agent, int ic, const String &uuid);
     */
    Connection signal_connect_candidate_more_window_show        (HelperAgentSlotVoid                *slot);

    /**
     * @brief Connect a slot to Helper candidate more window hide signal.
     *
     * This signal is used to do someting when candidate more window is hidden.
     *
     * The prototype of the slot is:
     * void candidate_more_window_hide (const HelperAgent *agent, int ic, const String &uuid);
     */
    Connection signal_connect_candidate_more_window_hide        (HelperAgentSlotVoid                *slot);

    /**
     * @brief Connect a slot to Helper candidate show signal.
     *
     * This signal is used to do candidate show.
     *
     * The prototype of the slot is:
     * void candidate_show (const HelperAgent *agent, int ic, const String &uuid);
     */
    Connection signal_connect_candidate_show                    (HelperAgentSlotVoid                *slot);

    /**
     * @brief Connect a slot to Helper candidate hide signal.
     *
     * This signal is used to do candidate hide.
     *
     * The prototype of the slot is:
     * void candidate_hide (const HelperAgent *agent,int ic, const String &uuid);
     */
    Connection signal_connect_candidate_hide                    (HelperAgentSlotVoid                *slot);

    /**
     * @brief Connect a slot to Helper update lookup table signal.
     *
     * This signal is used to do someting when update lookup table.
     *
     * The prototype of the slot is:
     * void update_lookup_table (const HelperAgent *agent, int ic, const String &uuid, LookupTable &Table);
     */
    Connection signal_connect_update_lookup_table               (HelperAgentSlotLookupTable          *slot);

    /**
     * @brief Connect a slot to Helper select aux signal.
     *
     * This signal is used to do something when aux is selected.
     *
     * The prototype of the slot is:
     * void select_aux (const HelperAgent *agent, int ic, const String &uuid, int index);
     */
    Connection signal_connect_select_aux                        (HelperAgentSlotInt                 *slot);

    /**
     * @brief Connect a slot to Helper select candidate signal.
     *
     * This signal is used to do something when candidate is selected.
     *
     * The prototype of the slot is:
     * void select_candidate (const HelperAgent *agent, int ic, const String &uuid, int index);
     */
    Connection signal_connect_select_candidate                  (HelperAgentSlotInt                 *slot);

    /**
     * @brief Connect a slot to Helper candidate table page up signal.
     *
     * This signal is used to do something when candidate table is paged up.
     *
     * The prototype of the slot is:
     * void candidate_table_page_up (const HelperAgent *agent, int ic, const String &uuid);
     */
    Connection signal_connect_candidate_table_page_up           (HelperAgentSlotVoid                *slot);

    /**
     * @brief Connect a slot to Helper candidate table page down signal.
     *
     * This signal is used to do something when candidate table is paged down.
     *
     * The prototype of the slot is:
     * void candidate_table_page_down (const HelperAgent *agent, int ic, const String &uuid);
     */
    Connection signal_connect_candidate_table_page_down         (HelperAgentSlotVoid                *slot);

    /**
     * @brief Connect a slot to Helper update candidate table page size signal.
     *
     * This signal is used to do something when candidate table page size is changed.
     *
     * The prototype of the slot is:
     * void update_candidate_table_page_size (const HelperAgent *, int ic, const String &uuid, int page_size);
     */
    Connection signal_connect_update_candidate_table_page_size  (HelperAgentSlotInt                 *slot);

    /**
     * @brief Connect a slot to Helper select associate signal.
     *
     * This signal is used to do something when associate is selected.
     *
     * The prototype of the slot is:
     * void select_associate (const HelperAgent *agent, int ic, const String &uuid, int index);
     */
    Connection signal_connect_select_associate                  (HelperAgentSlotInt                 *slot);

    /**
     * @brief Connect a slot to Helper associate table page up signal.
     *
     * This signal is used to do something when associate table is paged up.
     *
     * The prototype of the slot is:
     * void associate_table_page_up (const HelperAgent *agent, int ic, const String &uuid);
     */
    Connection signal_connect_associate_table_page_up           (HelperAgentSlotVoid                *slot);

    /**
     * @brief Connect a slot to Helper associate table page down signal.
     *
     * This signal is used to do something when associate table is paged down.
     *
     * The prototype of the slot is:
     * void associate_table_page_down (const HelperAgent *agent, int ic, const String &uuid);
     */
    Connection signal_connect_associate_table_page_down         (HelperAgentSlotVoid                *slot);

    /**
     * @brief Connect a slot to Helper update associate table page size signal.
     *
     * This signal is used to do something when associate table page size is changed.
     *
     * The prototype of the slot is:
     * void update_associate_table_page_size (const HelperAgent *, int ic, const String &uuid, int page_size);
     */
    Connection signal_connect_update_associate_table_page_size  (HelperAgentSlotInt                 *slot);

    /**
     * @brief Connect a slot to Helper turn on log signal.
     *
     * This signal is used to turn on Helper ISE debug information.
     *
     * The prototype of the slot is:
     * void turn_on_log (const HelperAgent *agent, uint32 &on);
     */
    Connection signal_connect_turn_on_log                       (HelperAgentSlotUintVoid            *slot);

    /**
     * @brief Connect a slot to Helper update displayed candidate number signal.
     *
     * This signal is used to inform helper ISE displayed candidate number.
     *
     * The prototype of the slot is:
     * void update_displayed_candidate_number (const HelperAgent *, int ic, const String &uuid, int number);
     */
    Connection signal_connect_update_displayed_candidate_number (HelperAgentSlotInt                 *slot);

    /**
     * @brief Connect a slot to Helper longpress candidate signal.
     *
     * This signal is used to do something when candidate is longpress.
     *
     * The prototype of the slot is:
     * void longpress_candidate (const HelperAgent *agent, int ic, const String &uuid, int index);
     */
    Connection signal_connect_longpress_candidate               (HelperAgentSlotInt                 *slot);

    /**
     * @brief Connect a slot to Helper update candidate item layout signal.
     *
     * The prototype of the slot is:
     * void update_candidate_item_layout (const HelperAgent *, const std::vector<uint32> &row_items);
     */
    Connection signal_connect_update_candidate_item_layout      (HelperAgentSlotUintVector          *slot);

     /**
     * @brief Connect a slot to Helper process key event signal.
     *
     * The prototype of the slot is:
     * void process_key_event (const HelperAgent *, KeyEvent &key, uint32 &ret);
     */
    Connection signal_connect_process_key_event (HelperAgentSlotKeyEventUint *slot);

    /**
     * @brief Connect a slot to Helper set input mode signal.
     *
     * This signal is used to set Helper ISE input mode.
     *
     * The prototype of the slot is:
     * void set_input_mode (const HelperAgent *agent, uint32 &input_mode);
     */
    Connection signal_connect_set_input_mode                        (HelperAgentSlotUintVoid            *slot);

    /**
     * @brief Connect a slot to Helper set input hint signal.
     *
     * This signal is used to set Helper ISE input hint.
     *
     * The prototype of the slot is:
     * void set_input_hint (const HelperAgent *agent, uint32 &input_hint);
     */
    Connection signal_connect_set_input_hint                        (HelperAgentSlotUintVoid            *slot);

    /**
     * @brief Connect a slot to Helper update bidi direction signal.
     *
     * This signal is used to update Helper ISE bidi direction.
     *
     * The prototype of the slot is:
     * void update_bidi_direction (const HelperAgent *agent, uint32 &bidi_direction);
     */
    Connection signal_connect_update_bidi_direction                 (HelperAgentSlotUintVoid            *slot);

    /**
     * @brief Connect a slot to Helper show option window.
     *
     * This signal is used to do request the ISE to show option window.
     *
     * The prototype of the slot is:
     * void show_option_window (const HelperAgent *agent, int ic, const String &uuid);
     */
    Connection signal_connect_show_option_window                    (HelperAgentSlotVoid                *slot);
};

/**  @} */

} /* namespace scim */

struct ArgInfo
{
    char **argv;
    int    argc;
};

#endif /* __SCIM_HELPER_H */

/*
vi:ts=4:nowrap:ai:expandtab
*/

