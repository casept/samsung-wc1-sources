/**
 * @file scim_socket_frontend.h
 * @brief definition of SocketFrontEnd related classes.
 */

/* ISF is based on SCIM 1.4.7 and extended for supporting more mobile fitable. */

/*
 * Smart Common Input Method
 *
 * Copyright (c) 2002-2005 James Su <suzhe@tsinghua.org.cn>
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
 * 1. Add helper ISE management
 *    a. Load helper ISE module
 *    b. Launch helper ISE
 * 2. Dynamic load keyboard ISE
 * 3. Add new interface APIs for keyboard ISE
 *    a. expand_candidate (), contract_candidate () and set_candidate_style ()
 *    b. socket_select_aux (), socket_set_prediction_allow () and socket_set_layout ()
 *    c. socket_update_candidate_item_layout (), socket_update_cursor_position () and socket_update_displayed_candidate_number ()
 *    d. socket_candidate_more_window_show (), socket_candidate_more_window_hide () and socket_longpress_candidate ()
 *    e. socket_set_imdata () and socket_reset_option ()
 *
 * $Id: scim_socket_frontend.h,v 1.26 2005/04/14 17:01:56 suzhe Exp $
 */

#if !defined (__SCIM_SOCKET_FRONTEND_H)
#define __SCIM_SOCKET_FRONTEND_H

#include "scim_stl_map.h"
#include <Ecore.h>

using namespace scim;

class EAPI SocketFrontEnd : public FrontEndBase
{
    enum ClientType {
        NONE_CLIENT             = -1, //Client does not find in SocketClientRepository
        UNKNOWN_CLIENT          =  0, //Connecting client, never receive any data
        IMENGINE_CLIENT,
        CONFIG_CLIENT,
        HELPER_MANAGER_CLIENT
    };

    struct ClientInfo {
        uint32     key;
        ClientType type;
        Ecore_Fd_Handler *handler;
    };

    enum ContinueMark {
        STATUS_NONE,
        STATUS_CONTINUE,
        STATUS_DISCONNECT
    };

    /**
     * ::first = socket id, ::second = instance id.
     */
    typedef std::vector <std::pair <int, int> > SocketInstanceRepository;

#if SCIM_USE_STL_EXT_HASH_MAP
    typedef __gnu_cxx::hash_map <int, ClientInfo, __gnu_cxx::hash <int> >   SocketClientRepository;
#elif SCIM_USE_STL_HASH_MAP
    typedef std::hash_map <int, ClientInfo, std::hash <int> >               SocketClientRepository;
#else
    typedef std::map <int, ClientInfo>                                      SocketClientRepository;
#endif

    ConfigPointer     m_config;
    String            m_display;

    SocketServer      m_socket_server;

    Transaction       m_send_trans;
    Transaction       m_receive_trans;
    Transaction       m_temp_trans;
    ContinueMark      m_continue_mark;

    SocketInstanceRepository m_socket_instance_repository;

    SocketClientRepository   m_socket_client_repository;

    bool   m_stay;

    bool   m_config_readonly;

    int    m_socket_timeout;

    int    m_current_instance;

    int    m_current_socket_client;

    int    m_preload_keyboard_ise_id;

    uint32 m_current_socket_client_key;

public:
    SocketFrontEnd (const BackEndPointer &backend,
                    const ConfigPointer  &config);

    virtual ~SocketFrontEnd ();

protected:
    virtual void show_preedit_string     (int id);
    virtual void show_aux_string         (int id);
    virtual void show_lookup_table       (int id);

    virtual void hide_preedit_string     (int id);
    virtual void hide_aux_string         (int id);
    virtual void hide_lookup_table       (int id);

    virtual void update_preedit_caret    (int id, int caret);
    virtual void update_preedit_string   (int id, const WideString & str, const AttributeList & attrs, int caret);
    virtual void update_aux_string       (int id, const WideString & str, const AttributeList & attrs);
    virtual void commit_string           (int id, const WideString & str);
    virtual void forward_key_event       (int id, const KeyEvent & key);
    virtual void update_lookup_table     (int id, const LookupTable & table);

    virtual void register_properties     (int id, const PropertyList & properties);
    virtual void update_property         (int id, const Property & property);

    virtual void beep                    (int id);

    virtual void start_helper            (int id, const String &helper_uuid);
    virtual void stop_helper             (int id, const String &helper_uuid);
    virtual void send_helper_event       (int id, const String &helper_uuid, const Transaction &trans);

    virtual bool get_surrounding_text    (int id, WideString &text, int &cursor, int maxlen_before, int maxlen_after);
    virtual bool delete_surrounding_text (int id, int offset, int len);

    virtual bool get_selection           (int id, WideString &text);
    virtual bool set_selection           (int id, int start, int end);

    virtual void expand_candidate        (int id);
    virtual void contract_candidate      (int id);
    virtual void set_candidate_style     (int                           id,
                                          ISF_CANDIDATE_PORTRAIT_LINE_T portrait_line,
                                          ISF_CANDIDATE_MODE_T          mode);

    virtual void send_private_command    (int id, const String &command);

public:
    virtual void init (int argc, char **argv);
    virtual void run ();

private:
    void load_helper_modules (const std::vector<String> &load_engine_list);
    void get_helper_list (const Socket &client);
    void run_helper (const Socket &client);

    void get_active_ise_list (int client_id);
    void preload_keyboard_ise (const String &uuid);
    void unregister_helper ();

    uint32 generate_key () const;
    ContinueMark get_continue_mark();

    bool check_client_connection (const Socket &client) const;

    void socket_accept_callback    (SocketServer *server, const Socket &client);
    void socket_receive_callback   (SocketServer *server, const Socket &client);
    void socket_exception_callback (SocketServer *server, const Socket &client);

    bool socket_open_connection    (SocketServer *server, const Socket &client);
    void socket_close_connection   (SocketServer *server, const Socket &client);
    ClientInfo socket_get_client_info (const Socket &client);

    //client_id is client's socket id
    void socket_get_factory_list            (int client_id);
    void socket_get_factory_name            (int client_id);
    void socket_get_factory_authors         (int client_id);
    void socket_get_factory_credits         (int client_id);
    void socket_get_factory_help            (int client_id);
    void socket_get_factory_locales         (int client_id);
    void socket_get_factory_icon_file       (int client_id);
    void socket_get_factory_language        (int client_id);

    void socket_new_instance                (int client_id);
    void socket_delete_instance             (int client_id);
    void socket_delete_all_instances        (int client_id);

    void socket_process_key_event           (int client_id);
    void socket_move_preedit_caret          (int client_id);
    void socket_select_aux                  (int client_id);
    void socket_select_candidate            (int client_id);
    void socket_update_lookup_table_page_size (int client_id);
    void socket_lookup_table_page_up        (int client_id);
    void socket_lookup_table_page_down      (int client_id);
    void socket_set_prediction_allow        (int client_id);
    void socket_set_layout                  (int client_id);
    void socket_reset_option                (int client_id);
    void socket_reset                       (int client_id);
    void socket_focus_in                    (int client_id);
    void socket_focus_out                   (int client_id);
    void socket_trigger_property            (int client_id);
    void socket_process_helper_event        (int client_id);
    void socket_update_client_capabilities  (int client_id);

    void socket_flush_config                (int client_id);
    void socket_erase_config                (int client_id);
    void socket_get_config_string           (int client_id);
    void socket_set_config_string           (int client_id);
    void socket_get_config_int              (int client_id);
    void socket_set_config_int              (int client_id);
    void socket_get_config_bool             (int client_id);
    void socket_set_config_bool             (int client_id);
    void socket_get_config_double           (int client_id);
    void socket_set_config_double           (int client_id);
    void socket_get_config_vector_string    (int client_id);
    void socket_set_config_vector_string    (int client_id);
    void socket_get_config_vector_int       (int client_id);
    void socket_set_config_vector_int       (int client_id);
    void socket_reload_config               (int client_id);
    void socket_set_display_name            (int client_id);
    void socket_update_ise_list             (int client_id);

    void socket_load_file                   (int client_id);
    void socket_turn_on_log                 (int client_id);

    void reload_config_callback (const ConfigPointer &config);

    void socket_update_candidate_item_layout (int client_id);
    void socket_update_cursor_position      (int client_id);
    void socket_update_displayed_candidate_number (int client_id);
    void socket_candidate_more_window_show  (int client_id);
    void socket_candidate_more_window_hide  (int client_id);
    void socket_longpress_candidate         (int client_id);
    void socket_set_imdata                  (int client_id);
    void socket_set_autocapital_type        (int client_id);
    void socket_set_input_hint              (int client_id);
    void socket_update_bidi_direction       (int client_id);
};

#endif

/*
vi:ts=4:nowrap:ai:expandtab
*/
