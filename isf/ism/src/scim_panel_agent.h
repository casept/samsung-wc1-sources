/**
 * @file scim_panel_agent.h
 * @brief Defines scim::PanelAgent and their related types.
 *
 * scim::PanelAgent is a class used to write Panel daemons.
 * It acts like a Socket Server and handles all socket clients
 * issues.
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
 * 1. Add new signals
 *    a. m_signal_set_keyboard_ise and m_signal_get_keyboard_ise
 *    b. m_signal_focus_in and m_signal_focus_out
 *    c. m_signal_expand_candidate, m_signal_contract_candidate and m_signal_set_candidate_ui
 *    d. m_signal_get_ise_list, m_signal_get_keyboard_ise_list, m_signal_update_ise_geometry and m_signal_get_ise_information
 *    e. m_signal_set_active_ise_by_uuid and m_signal_get_ise_info_by_uuid
 *    f. m_signal_accept_connection, m_signal_close_connection and m_signal_exit
 * 2. Add new interface APIs in PanelClient class
 *    a. get_helper_manager_id (), has_helper_manager_pending_event () and filter_helper_manager_event ()
 *    b. update_candidate_panel_event (), update_input_panel_event () and select_aux ()
 *    c. candidate_more_window_show () and candidate_more_window_hide ()
 *    d. update_displayed_candidate_number () and update_candidate_item_layout ()
 *    e. stop_helper (), send_longpress_event () and update_ise_list ()
 *    f. filter_event (), filter_exception_event () and get_server_id ()
 * 3. Donot use thread to receive message
 * 4. Monitor socket frontend for self-recovery function
 *
 * $Id: scim_panel_agent.h,v 1.2 2005/06/11 14:50:31 suzhe Exp $
 */

#ifndef __SCIM_PANEL_AGENT_H
#define __SCIM_PANEL_AGENT_H

#include <scim_panel_common.h>

namespace scim {

/**
 * @addtogroup Panel
 * @ingroup InputServiceFramework
 * The accessory classes to help develop Panel daemons and FrontEnds
 * which need to communicate with Panel daemons.
 * @{
 */

typedef struct _ISE_INFO
{
    String uuid;
    String name;
    String lang;
    String icon;
    uint32 option;
    TOOLBAR_MODE_T type;
} ISE_INFO;

typedef Slot0<void>
        PanelAgentSlotVoid;

typedef Slot1<void, int>
        PanelAgentSlotInt;

typedef Slot1<void, int &>
        PanelAgentSlotInt2;

typedef Slot1<void, const String &>
        PanelAgentSlotString;

typedef Slot2<void, String &, String &>
        PanelAgentSlotString2;

typedef Slot2<void, int, const String &>
        PanelAgentSlotIntString;

typedef Slot1<void, const PanelFactoryInfo &>
        PanelAgentSlotFactoryInfo;

typedef Slot1<void, const std::vector <PanelFactoryInfo> &>
        PanelAgentSlotFactoryInfoVector;

typedef Slot1<void, const LookupTable &>
        PanelAgentSlotLookupTable;

typedef Slot1<void, const Property &>
        PanelAgentSlotProperty;

typedef Slot1<void, const PropertyList &>
        PanelAgentSlotPropertyList;

typedef Slot2<void, int, int>
        PanelAgentSlotIntInt;

typedef Slot2<void, int &, int &>
        PanelAgentSlotIntInt2;

typedef Slot3<void, int, int, int>
        PanelAgentSlotIntIntInt;

typedef Slot4<void, int, int, int, int>
        PanelAgentSlotIntIntIntInt;

typedef Slot2<void, int, const Property &>
        PanelAgentSlotIntProperty;

typedef Slot2<void, int, const PropertyList &>
        PanelAgentSlotIntPropertyList;

typedef Slot2<void, int, const HelperInfo &>
        PanelAgentSlotIntHelperInfo;

typedef Slot2<void, const String &, const AttributeList &>
        PanelAgentSlotAttributeString;

typedef Slot3<void, const String &, const AttributeList &, int>
        PanelAgentSlotAttributeStringInt;

typedef Slot1<void, std::vector<String> &>
        PanelAgentSlotStringVector;

typedef Slot1<bool, std::vector<String> &>
        PanelAgentSlotBoolStringVector;

typedef Slot2<void, char *, std::vector<String> &>
        PanelAgentSlotStrStringVector;

typedef Slot2<bool, const String &, ISE_INFO &>
        PanelAgentSlotStringISEINFO;

typedef Slot1<void, const KeyEvent &>
        PanelAgentSlotKeyEvent;

typedef Slot1<void, struct rectinfo &>
        PanelAgentSlotRect;

typedef Slot2<void, const String &, bool>
        PanelAgentSlotStringBool;

typedef Slot6<bool, String, String &, String &, int &, int &, String &>
        PanelAgentSlotBoolString4int2;

typedef struct DefaultIse
{
    TOOLBAR_MODE_T type;
    String         uuid;
    String         name;
    DefaultIse () : type (TOOLBAR_KEYBOARD_MODE), uuid (""), name ("") { }
} DEFAULT_ISE_T;

/**
 * @brief The class to implement all socket protocol in Panel.
 *
 * This class acts like a stand alone SocketServer.
 * It has its own dedicated main loop, and will be blocked when run () is called.
 * So run () must be called within a separated thread, in order to not block
 * the main loop of the Panel program itself.
 *
 * Before calling run (), the panel must hook the callback functions to the
 * corresponding signals.
 *
 * Note that, there are two special signals: lock(void) and unlock(void). These
 * two signals are used to provide a thread lock to PanelAgent, so that PanelAgent
 * can run correctly within a multi-threading Panel program.
 */
class EAPI PanelAgent
{
    class PanelAgentImpl;
    PanelAgentImpl *m_impl;

    PanelAgent (const HelperAgent &);
    const PanelAgent & operator = (const HelperAgent &);

public:
    PanelAgent ();
    ~PanelAgent ();

    /**
     * @brief Initialize this PanelAgent.
     *
     * @param config The name of the config module to be used by Helpers.
     * @param display The name of display, on which the Panel should run.
     * @param resident If this is true then this PanelAgent will keep running
     *                 even if there is no more client connected.
     *
     * @return true if the PanelAgent is initialized correctly and ready to run.
     */
    bool initialize (const String &config, const String &display, bool resident = false);

    /**
     * @brief Check if this PanelAgent is initialized correctly and ready to run.
     *
     * @return true if this PanelAgent is ready to run.
     */
    bool valid (void) const;

    /**
     * @brief Run this PanelAgent.
     *
     * This method has its own dedicated main loop,
     * so it should be run in a separated thread.
     *
     * @return false if the Panel SocketServer encountered an error when running.
     *               Otherwise, it won't return until the server exits.
     */
    bool run (void);

    /**
     * @brief Stop this PanelAgent.
     */
    void stop (void);

public:

    /**
     * @brief Get the list of all helpers.
     *
     * Panel program should provide a menu which contains
     * all stand alone but not auto start Helpers, so that users can activate
     * the Helpers by clicking in the menu.
     *
     * All auto start Helpers should be started by Panel after running PanelAgent
     * by calling PanelAgent::start_helper().
     *
     * @param helpers A list contains information of all Helpers.
     */
    int get_helper_list (std::vector <HelperInfo> & helpers) const;

    /**
     * @brief Get the list of active ISEs.
     *
     * @param strlist A list contains information of active ISEs.
     *
     * @return the list size.
     */
    int get_active_ise_list (std::vector<String> &strlist);

    /**
     * @brief Get the helper manager connection id.
     *
     * @return the connection id
     */
    int get_helper_manager_id (void);

    /**
     * @brief Check if there are any events available to be processed.
     *
     * If it returns true then HelperManager object should call
     * HelperManager::filter_event () to process them.
     *
     * @return true if there are any events available.
     */
    bool has_helper_manager_pending_event (void);

    /**
     * @brief Filter the events received from helper manager.
     *
     * Corresponding signal will be emitted in this method.
     *
     * @return true if the command was sent correctly, otherwise return false.
     */
    bool filter_helper_manager_event (void);

    /**
     * @brief Send display name to FrontEnd.
     *
     * @param name The display name.
     *
     * @return zero if this operation is successful, otherwise return -1.
     */
    int send_display_name (String &name);


    /**
     * @brief Get current ISE type.
     *
     * @return the current ISE type.
     */
    TOOLBAR_MODE_T get_current_toolbar_mode (void) const;

    /**
     * @brief Get current helper ISE uuid.
     *
     * @return the current helper ISE uuid.
     */
   String get_current_helper_uuid (void) const;

    /**
     * @brief Get current helper ISE name.
     *
     * @return the current helper ISE name.
     */
    String get_current_helper_name (void) const;

    /**
     * @brief Get current ISE name.
     *
     * @return the current ISE name.
     */
    String get_current_ise_name (void) const;

    /**
     * @brief Set current ISE name.
     *
     * @param name The current ISE name.
     */
    void set_current_ise_name (String &name);

    /**
     * @brief Set current ISE type.
     *
     * @param mode The current ISE type.
     */
    void set_current_toolbar_mode (TOOLBAR_MODE_T mode);

    /**
     * @brief Set current helper ISE option.
     *
     * @param mode The current helper ISE option.
     */
    void set_current_helper_option (uint32 option);
    /**
     * @brief Get current ISE size and position.
     *
     * @param rect It contains ISE size and position.
     */
    void get_current_ise_geometry (rectinfo &rect);

    /**
     * @brief Send candidate panel event to IM Control.
     *
     * @param nType  The candidate panel event type.
     * @param nValue The candidate panel event value.
     */
    void update_candidate_panel_event (uint32 nType, uint32 nValue);

    /**
     * @brief Send input panel event to IM Control.
     *
     * @param nType  The input panel event type.
     * @param nValue The input panel event value.
     */
    void update_input_panel_event (uint32 nType, uint32 nValue);

    /**
     * @brief Notice helper ISE to focus out.
     *
     * @param uuid The helper ISE uuid.
     */
    void focus_out_helper (const String &uuid);

    /**
     * @brief Notice helper ISE to focus in.
     *
     * @param uuid The helper ISE uuid.
     */
    void focus_in_helper (const String &uuid);

    /**
     * @brief Notice helper ISE to show window.
     *
     * @param uuid The helper ISE uuid.
     */
    bool show_helper (const String &uuid);

    /**
     * @brief Notice helper ISE to hide window.
     *
     * @param uuid The helper ISE uuid.
     */
    void hide_helper (const String &uuid);


    /**
     * @brief Set default ISE.
     *
     * @param ise The variable contains the information of default ISE.
     */
    void set_default_ise (const DEFAULT_ISE_T &ise);

    /**
     * @brief Set whether shared ISE is for all applications.
     *
     * @param should_shared_ise The indicator for shared ISE.
     */
    void set_should_shared_ise (const bool should_shared_ise);

    /**
     * @brief Reset keyboard ISE.
     *
     * @return true if this operation is successful, otherwise return false.
     */
    bool reset_keyboard_ise (void) const;

public:
    /**
     * @brief Let the focused IMEngineInstance object move the preedit caret.
     *
     * @param position The new preedit caret position.
     * @return true if the command was sent correctly.
     */
    bool move_preedit_caret             (uint32         position);

    /**
     * @brief Request help information from the focused IMEngineInstance object.
     * @return true if the command was sent correctly.
     */
    bool request_help                   (void);

    /**
     * @brief Request factory menu from the focused FrontEnd.
     * @return true if the command was sent correctly.
     */
    bool request_factory_menu           (void);

    /**
     * @brief Change the factory used by the focused IMEngineInstance object.
     *
     * @param uuid The uuid of the new factory.
     * @return true if the command was sent correctly.
     */
    bool change_factory                 (const String  &uuid);

    /**
     * @brief Notice Helper ISE that candidate more window is showed.
     * @return true if the command was sent correctly.
     */
    bool candidate_more_window_show     (void);

    /**
     * @brief Notice Helper ISE that candidate more window is hidden.
     * @return true if the command was sent correctly.
     */
    bool candidate_more_window_hide     (void);

    /**
     * @brief Notice Helper ISE that show candidate.
     * @return true if the command was sent correctly.
     */
    bool helper_candidate_show     (void);

    /**
     * @brief Notice Helper ISE that hide candidate.
     * @return true if the command was sent correctly.
     */
    bool helper_candidate_hide     (void);

    /**
     * @brief Update helper lookup table.
     * @return true if the command was sent correctly.
     */
    bool update_helper_lookup_table     (const LookupTable &table);

    /**
     * @brief Let the focused IMEngineInstance object
     *        select a aux in current aux string.
     *
     * @param item The index of the selected aux.
     * @return true if the command was sent correctly.
     */
    bool select_aux                     (uint32         item);

    /**
     * @brief Let the focused IMEngineInstance object
     *        select a candidate in current lookup table.
     *
     * @param item The index of the selected candidate.
     * @return true if the command was sent correctly.
     */
    bool select_candidate               (uint32         item);

    /**
     * @brief Let the focused IMEngineInstance object
     *        flip the LookupTable to previous page.
     * @return true if the command was sent correctly.
     */
    bool lookup_table_page_up           (void);

    /**
     * @brief Let the focused IMEngineInstance object
     *        flip the LookupTable to next page.
     * @return true if the command was sent correctly.
     */
    bool lookup_table_page_down         (void);

    /**
     * @brief Let the focused IMEngineInstance object
     *        update the page size of the LookupTable.
     *
     * @param size The new page size.
     * @return true if the command was sent correctly.
     */
    bool update_lookup_table_page_size  (uint32         size);

    /**
     * @brief Let the focused IMEngineInstance object
     *        update candidate items layout.
     *
     * @param row_items The items of each row.
     * @return true if the command was sent correctly.
     */
    bool update_candidate_item_layout   (const std::vector<uint32> &row_items);

    /**
     * @brief Let the focused IMEngineInstance object
     *        select a associate in current associate table.
     *
     * @param item The index of the selected associate.
     * @return true if the command was sent correctly.
     */
    bool select_associate               (uint32         item);

    /**
     * @brief Let the focused IMEngineInstance object
     *        flip the AssociateTable to previous page.
     * @return true if the command was sent correctly.
     */
    bool associate_table_page_up        (void);

    /**
     * @brief Let the focused IMEngineInstance object
     *        flip the AssociateTable to next page.
     * @return true if the command was sent correctly.
     */
    bool associate_table_page_down      (void);

    /**
     * @brief Let the focused IMEngineInstance object
     *        update the page size of the AssociateTable.
     *
     * @param size The new page size.
     * @return true if the command was sent correctly.
     */
    bool update_associate_table_page_size (uint32       size);

    /**
     * @brief Inform helper ISE to update displayed candidate number.
     *
     * @param size The displayed candidate number.
     * @return true if the command was sent correctly.
     */
    bool update_displayed_candidate_number (uint32      size);

    /**
     * @brief Trigger a property of the focused IMEngineInstance object.
     *
     * @param property The property key to be triggered.
     * @return true if the command was sent correctly.
     */
    bool trigger_property               (const String  &property);

    /**
     * @brief Trigger a property of a Helper object.
     *
     * @param client The client id of the Helper object.
     * @param property The property key to be triggered.
     * @return true if the command was sent correctly.
     */
    bool trigger_helper_property        (int            client,
                                         const String  &property);

    /**
     * @brief Start a stand alone helper.
     *
     * @param uuid The uuid of the Helper object to be started.
     * @return true if the command was sent correctly.
     */
    bool start_helper                   (const String  &uuid);

    /**
     * @brief Stop a stand alone helper.
     *
     * @param uuid The uuid of the Helper object to be stopped.
     * @return true if the command was sent correctly.
     */
    bool stop_helper                    (const String  &uuid);

    /**
     * @brief Let all FrontEnds and Helpers reload configuration.
     * @return true if the command was sent correctly.
     */
    bool reload_config                  (void);

    /**
     * @brief Let all FrontEnds, Helpers and this Panel exit.
     * @return true if the command was sent correctly.
     */
    bool exit                           (void);

    /**
     * @brief Filter the events received from panel client.
     *
     * Corresponding signal will be emitted in this method.
     *
     * @param fd The file description for connection.
     *
     * @return true if the command was sent correctly, otherwise return false.
     */
    bool filter_event                   (int fd);

    /**
     * @brief Filter the exception events received from panel client.
     *
     * Corresponding signal will be emitted in this method.
     *
     * @param fd The file description for connection.
     *
     * @return true if the command was sent correctly, otherwise return false.
     */
    bool filter_exception_event         (int fd);

    /**
     * @brief Get PanelAgent server fd.
     *
     * @return the PanelAgent server fd.
     */
    int get_server_id                   (void);

    /**
     * @brief Send candidate longpress event to ISE.
     *
     * @param type The candidate object type.
     * @param index The candidate object index.
     *
     * @return none.
     */
    void send_longpress_event           (int type, int index);

    /**
     * @brief Request to update ISE list.
     *
     * @return none.
     */
    void update_ise_list                (std::vector<String> &strList);

public:
    /**
     * @brief Signal: Reload configuration.
     *
     * When a Helper object send a RELOAD_CONFIG event to this Panel,
     * this signal will be emitted. Panel should reload all configuration here.
     */
    Connection signal_connect_reload_config              (PanelAgentSlotVoid                *slot);

    /**
     * @brief Signal: Turn on.
     *
     * slot prototype: void turn_on (void);
     */
    Connection signal_connect_turn_on                    (PanelAgentSlotVoid                *slot);

    /**
     * @brief Signal: Turn off.
     *
     * slot prototype: void turn_off (void);
     */
    Connection signal_connect_turn_off                   (PanelAgentSlotVoid                *slot);

    /**
     * @brief Signal: Show panel.
     *
     * slot prototype: void show_panel (void);
     */
    Connection signal_connect_show_panel                 (PanelAgentSlotVoid                *slot);

    /**
     * @brief Signal: Hide panel.
     *
     * slot prototype: void hide_panel (void);
     */
    Connection signal_connect_hide_panel                 (PanelAgentSlotVoid                *slot);

    /**
     * @brief Signal: Update screen.
     *
     * slot prototype: void update_screen (int screen);
     */
    Connection signal_connect_update_screen              (PanelAgentSlotInt                 *slot);

    /**
     * @brief Signal: Update spot location.
     *
     * slot prototype: void update_spot_location (int x, int y, int top_y);
     */
    Connection signal_connect_update_spot_location       (PanelAgentSlotIntIntInt           *slot);

    /**
     * @brief Signal: Update factory information.
     *
     * slot prototype: void update_factory_info (const PanelFactoryInfo &info);
     */
    Connection signal_connect_update_factory_info        (PanelAgentSlotFactoryInfo         *slot);

    /**
     * @brief Signal: start default ise.
     *
     * slot prototype: void start_default_ise (void);
     */
    Connection signal_connect_start_default_ise          (PanelAgentSlotVoid                *slot);

    /**
     * @brief Signal: stop default ise.
     *
     * slot prototype: void stop_default_ise (void);
     */
    Connection signal_connect_stop_default_ise           (PanelAgentSlotVoid                *slot);

    /**
     * @brief Signal: Get the list of keyboard ise name.
     *
     * slot prototype: bool get_keyboard_ise_list (std::vector<String> &);
     */
    Connection signal_connect_get_keyboard_ise_list      (PanelAgentSlotBoolStringVector    *slot);

    /**
     * @brief Signal: Set candidate ui.
     *
     * slot prototype: void set_candidate_ui (int style, int mode);
     */
    Connection signal_connect_set_candidate_ui           (PanelAgentSlotIntInt              *slot);

    /**
     * @brief Signal: Get candidate ui.
     *
     * slot prototype: void get_candidate_ui (int &style, int &mode);
     */
    Connection signal_connect_get_candidate_ui           (PanelAgentSlotIntInt2             *slot);

    /**
     * @brief Signal: Get candidate window geometry information.
     *
     * slot prototype: void get_candidate_geometry (rectinfo &info);
     */
    Connection signal_connect_get_candidate_geometry     (PanelAgentSlotRect                *slot);

    /**
     * @brief Signal: Get input panel geometry information.
     *
     * slot prototype: void get_input_panel_geometry (rectinfo &info);
     */
    Connection signal_connect_get_input_panel_geometry   (PanelAgentSlotRect                *slot);

    /**
     * @brief Signal: Set candidate position.
     *
     * slot prototype: void set_candidate_position (int left, int top);
     */
    Connection signal_connect_set_candidate_position     (PanelAgentSlotIntInt              *slot);

    /**
     * @brief Signal: Set keyboard ise.
     *
     * slot prototype: void set_keyboard_ise (const String &uuid);
     */
    Connection signal_connect_set_keyboard_ise           (PanelAgentSlotString              *slot);

    /**
     * @brief Signal: Get keyboard ise.
     *
     * slot prototype: void get_keyboard_ise (String &ise_name);
     */
    Connection signal_connect_get_keyboard_ise           (PanelAgentSlotString2             *slot);
    /**
     * @brief Signal: Update ise geometry.
     *
     * slot prototype: void update_ise_geometry (int x, int y, int width, int height);
     */
    Connection signal_connect_update_ise_geometry        (PanelAgentSlotIntIntIntInt        *slot);

    /**
     * @brief Signal: Show help.
     *
     * slot prototype: void show_help (const String &help);
     */
    Connection signal_connect_show_help                  (PanelAgentSlotString              *slot);

    /**
     * @brief Signal: Show factory menu.
     *
     * slot prototype: void show_factory_menu (const std::vector <PanelFactoryInfo> &menu);
     */
    Connection signal_connect_show_factory_menu          (PanelAgentSlotFactoryInfoVector   *slot);

    /**
     * @brief Signal: Show preedit string.
     *
     * slot prototype: void show_preedit_string (void):
     */
    Connection signal_connect_show_preedit_string        (PanelAgentSlotVoid                *slot);

    /**
     * @brief Signal: Show aux string.
     *
     * slot prototype: void show_aux_string (void):
     */
    Connection signal_connect_show_aux_string            (PanelAgentSlotVoid                *slot);

    /**
     * @brief Signal: Show lookup table.
     *
     * slot prototype: void show_lookup_table (void):
     */
    Connection signal_connect_show_lookup_table          (PanelAgentSlotVoid                *slot);

    /**
     * @brief Signal: Show associate table.
     *
     * slot prototype: void show_associate_table (void):
     */
    Connection signal_connect_show_associate_table       (PanelAgentSlotVoid                *slot);

    /**
     * @brief Signal: Hide preedit string.
     *
     * slot prototype: void hide_preedit_string (void);
     */
    Connection signal_connect_hide_preedit_string        (PanelAgentSlotVoid                *slot);

    /**
     * @brief Signal: Hide aux string.
     *
     * slot prototype: void hide_aux_string (void);
     */
    Connection signal_connect_hide_aux_string            (PanelAgentSlotVoid                *slot);

    /**
     * @brief Signal: Hide lookup table.
     *
     * slot prototype: void hide_lookup_table (void);
     */
    Connection signal_connect_hide_lookup_table          (PanelAgentSlotVoid                *slot);

    /**
     * @brief Signal: Hide associate table.
     *
     * slot prototype: void hide_associate_table (void);
     */
    Connection signal_connect_hide_associate_table       (PanelAgentSlotVoid                *slot);

    /**
     * @brief Signal: Update preedit string.
     *
     * slot prototype: void update_preedit_string (const String &str, const AttributeList &attrs, int caret);
     * - str   -- The UTF-8 encoded string to be displayed in preedit area.
     * - attrs -- The attributes of the string.
     */
    Connection signal_connect_update_preedit_string      (PanelAgentSlotAttributeStringInt  *slot);

    /**
     * @brief Signal: Update preedit caret.
     *
     * slot prototype: void update_preedit_caret (int caret);
     */
    Connection signal_connect_update_preedit_caret       (PanelAgentSlotInt                 *slot);

    /**
     * @brief Signal: Update aux string.
     *
     * slot prototype: void update_aux_string (const String &str, const AttributeList &attrs);
     * - str   -- The UTF-8 encoded string to be displayed in aux area.
     * - attrs -- The attributes of the string.
     */
    Connection signal_connect_update_aux_string          (PanelAgentSlotAttributeString     *slot);

    /**
     * @brief Signal: Update lookup table.
     *
     * slot prototype: void update_lookup_table (const LookupTable &table);
     * - table -- The new LookupTable object.
     */
    Connection signal_connect_update_lookup_table        (PanelAgentSlotLookupTable         *slot);

    /**
     * @brief Signal: Update associate table.
     *
     * slot prototype: void update_associate_table (const LookupTable &table);
     * - table -- The new LookupTable object.
     */
    Connection signal_connect_update_associate_table     (PanelAgentSlotLookupTable         *slot);

    /**
     * @brief Signal: Register properties.
     *
     * Register properties of the focused instance.
     *
     * slot prototype: void register_properties (const PropertyList &props);
     * - props -- The properties to be registered.
     */
    Connection signal_connect_register_properties        (PanelAgentSlotPropertyList        *slot);

    /**
     * @brief Signal: Update property.
     *
     * Update a property of the focused instance.
     *
     * slot prototype: void update_property (const Property &prop);
     * - prop -- The property to be updated.
     */
    Connection signal_connect_update_property            (PanelAgentSlotProperty            *slot);

    /**
     * @brief Signal: Register properties of a helper.
     *
     * slot prototype: void register_helper_properties (int id, const PropertyList &props);
     * - id    -- The client id of the helper object.
     * - props -- The properties to be registered.
     */
    Connection signal_connect_register_helper_properties (PanelAgentSlotIntPropertyList     *slot);

    /**
     * @brief Signal: Update helper property.
     *
     * slot prototype: void update_helper_property (int id, const Property &prop);
     * - id   -- The client id of the helper object.
     * - prop -- The property to be updated.
     */
    Connection signal_connect_update_helper_property     (PanelAgentSlotIntProperty         *slot);

    /**
     * @brief Signal: Register a helper object.
     *
     * A newly started helper object will send this event to Panel.
     *
     * slot prototype: register_helper (int id, const HelperInfo &helper);
     * - id     -- The client id of the helper object.
     * - helper -- The information of the helper object.
     */
    Connection signal_connect_register_helper            (PanelAgentSlotIntHelperInfo       *slot);

    /**
     * @brief Signal: Remove a helper object.
     *
     * If a running helper close its connection to Panel, then this signal will be triggered to
     * tell Panel to remove all data associated to this helper.
     *
     * slot prototype: void remove_helper (int id);
     * - id -- The client id of the helper object to be removed.
     */
    Connection signal_connect_remove_helper              (PanelAgentSlotInt                 *slot);

    /**
     * @brief Signal: Start an ise with the speficied uuid
     *
     * slot prototype: void set_active_ise_by_uuid (const String& uuid);
     * - uuid -- the uuid of the ise object
     */
    Connection signal_connect_set_active_ise_by_uuid     (PanelAgentSlotStringBool          *slot);

    /**
     * @brief Signal: Focus in panel.
     *
     * slot prototype: void focus_in (void);
     */
    Connection signal_connect_focus_in                   (PanelAgentSlotVoid                *slot);

    /**
     * @brief Signal: Focus out panel.
     *
     * slot prototype: void focus_out (void);
     */
    Connection signal_connect_focus_out                  (PanelAgentSlotVoid                *slot);

    /**
     * @brief Signal: Expand candidate panel.
     *
     * slot prototype: void expand_candidate (void);
     */
    Connection signal_connect_expand_candidate           (PanelAgentSlotVoid                *slot);

    /**
     * @brief Signal: Contract candidate panel.
     *
     * slot prototype: void contract_candidate (void);
     */
    Connection signal_connect_contract_candidate         (PanelAgentSlotVoid                *slot);

    /**
     * @brief Signal: Select candidate string index.
     *
     * slot prototype: void select_candidate (int index);
     */
    Connection signal_connect_select_candidate           (PanelAgentSlotInt                 *slot);

    /**
     * @brief Signal: Get the list of ise name.
     *
     * slot prototype: bool get_ise_list (std::vector<String> &);
     */
    Connection signal_connect_get_ise_list               (PanelAgentSlotBoolStringVector    *slot);

    /**
     * @brief Signal: Get the ISE information according to UUID.
     *
     * slot prototype: bool get_ise_information (String, String &, String &, int &, int &);
     */
    Connection signal_connect_get_ise_information        (PanelAgentSlotBoolString4int2     *slot);

    /**
     * @brief Signal: Get the list of selected language name.
     *
     * slot prototype: void get_language_list (std::vector<String> &);
     */
    Connection signal_connect_get_language_list          (PanelAgentSlotStringVector        *slot);

    /**
     * @brief Signal: Get the all languages name.
     *
     * slot prototype: void get_all_language (std::vector<String> &);
     */
    Connection signal_connect_get_all_language           (PanelAgentSlotStringVector        *slot);
    /**
     * @brief Signal: Get the language list of a specified ise.
     *
     * slot prototype: void get_ise_language (char *, std::vector<String> &);
     */
    Connection signal_connect_get_ise_language           (PanelAgentSlotStrStringVector     *slot);

    /**
     * @brief Signal: Get the ise information by uuid.
     *
     * slot prototype: bool get_ise_info_by_uuid (const String &, ISE_INFO &);
     */
    Connection signal_connect_get_ise_info_by_uuid       (PanelAgentSlotStringISEINFO       *slot);

    /**
     * @brief Signal: send key event in panel.
     *
     * slot prototype: void send_key_event (const KeyEvent &);
     */
    Connection signal_connect_send_key_event             (PanelAgentSlotKeyEvent            *slot);

    /**
     * @brief Signal: A transaction is started.
     *
     * This signal infers that the Panel should disable update before this transaction finishes.
     *
     * slot prototype: void signal_connect_transaction_start (void);
     */
    Connection signal_connect_transaction_start          (PanelAgentSlotVoid                *slot);

    /**
     * @brief Signal: Accept connection for this PanelAgent.
     *
     * slot prototype: void accept_connection (int fd);
     * - fd -- the file description for connection
     */
    Connection signal_connect_accept_connection          (PanelAgentSlotInt                 *slot);

    /**
     * @brief Signal: Close connection for this PanelAgent.
     *
     * slot prototype: void close_connection (int fd);
     * - fd -- the file description for connection
     */
    Connection signal_connect_close_connection           (PanelAgentSlotInt                 *slot);

    /**
     * @brief Signal: Exit application for this PanelAgent.
     *
     * slot prototype: void app_exit (void);
     */
    Connection signal_connect_exit                       (PanelAgentSlotVoid                *slot);

    /**
     * @brief Signal: A transaction is finished.
     *
     * This signal will get emitted when a transaction is finished. This implys to re-enable
     * panel GUI update
     *
     * slot prototype: void signal_connect_transaction_end (void);
     */
    Connection signal_connect_transaction_end            (PanelAgentSlotVoid                *slot);

    /**
     * @brief Signal: Lock the exclusive lock for this PanelAgent.
     *
     * The panel program should provide a thread lock and hook a slot into this signal to lock it.
     * PanelAgent will use this lock to ensure the data integrity.
     *
     * slot prototype: void lock (void);
     */
    Connection signal_connect_lock                       (PanelAgentSlotVoid                *slot);

    /**
     * @brief Signal: Unlock the exclusive lock for this PanelAgent.
     *
     * slot prototype: void unlock (void);
     */
    Connection signal_connect_unlock                     (PanelAgentSlotVoid                *slot);

    /**
     * @brief Signal: The input context of ISE is changed.
     *
     * slot prototype: void update_input_context (int type, int value);
     */
    Connection signal_connect_update_input_context       (PanelAgentSlotIntInt              *slot);

    /**
     * @brief Signal: Show ISE.
     *
     * slot prototype: void show_ise (void);
     */
    Connection signal_connect_show_ise                   (PanelAgentSlotVoid                *slot);

    /**
     * @brief Signal: Hide ISE.
     *
     * slot prototype: void hide_ise (void);
     */
    Connection signal_connect_hide_ise                   (PanelAgentSlotVoid                *slot);

    /**
     * @brief Signal: Notifies the client finished handling WILL_SHOW event
     *
     * slot prototype: void will_show_ack (void);
     */
    Connection signal_connect_will_show_ack              (PanelAgentSlotVoid                *slot);

    /**
     * @brief Signal: Notifies the client finished handling WILL_HIDE event
     *
     * slot prototype: void will_hide_ack (void);
     */
    Connection signal_connect_will_hide_ack              (PanelAgentSlotVoid                *slot);

    /**
     * @brief Signal: Set hardware mode
     *
     * slot prototype: void set_keyboard_mode (void);
     */
    Connection signal_connect_set_keyboard_mode (PanelAgentSlotInt                *slot);

    /**
     * @brief Signal: Notifies the client finished handling WILL_HIDE event for candidate
     *
     * slot prototype: void candidate_will_hide_ack (void);
     */
    Connection signal_connect_candidate_will_hide_ack    (PanelAgentSlotVoid                *slot);

    /**
     * @brief Signal: Get ISE state.
     *
     * slot prototype: void get_ise_state (int &state);
     */
    Connection signal_connect_get_ise_state              (PanelAgentSlotInt2                *slot);

};

/**  @} */

} /* namespace scim */

#endif /* __SCIM_PANEL_AGENT_H */

/*
vi:ts=4:nowrap:ai:expandtab
*/

