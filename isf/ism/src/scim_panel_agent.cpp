/** @file scim_panel.cpp
 *  @brief Implementation of class PanelAgent.
 */

/* ISF is based on SCIM 1.4.7 and extended for supporting more mobile fitable. */

/*
 * Smart Common Input Method
 *
 * Copyright (c) 2005 James Su <suzhe@tsinghua.org.cn>
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
 * $Id: scim_panel_agent.cpp,v 1.8.2.1 2006/01/09 14:32:18 suzhe Exp $
 *
 */

#define Uses_SCIM_TRANSACTION
#define Uses_SCIM_TRANS_COMMANDS
#define Uses_SCIM_PANEL_AGENT
#define Uses_SCIM_HELPER
#define Uses_SCIM_SOCKET
#define Uses_SCIM_EVENT
#define Uses_SCIM_CONFIG
#define Uses_SCIM_CONFIG_MODULE
#define Uses_SCIM_CONFIG_PATH
#define Uses_SCIM_UTILITY

#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include "scim_private.h"
#include "scim.h"
#include "scim_stl_map.h"
#include "security-server.h"


EAPI scim::CommonLookupTable g_isf_candidate_table;


namespace scim {

typedef Signal0<void>
        PanelAgentSignalVoid;

typedef Signal1<void, int>
        PanelAgentSignalInt;

typedef Signal1<void, int &>
        PanelAgentSignalInt2;

typedef Signal1<void, const String &>
        PanelAgentSignalString;

typedef Signal2<void, const String &, bool>
        PanelAgentSignalStringBool;

typedef Signal2<void, String &, String &>
        PanelAgentSignalString2;

typedef Signal2<void, int, const String &>
        PanelAgentSignalIntString;

typedef Signal1<void, const PanelFactoryInfo &>
        PanelAgentSignalFactoryInfo;

typedef Signal1<void, const std::vector <PanelFactoryInfo> &>
        PanelAgentSignalFactoryInfoVector;

typedef Signal1<void, const LookupTable &>
        PanelAgentSignalLookupTable;

typedef Signal1<void, const Property &>
        PanelAgentSignalProperty;

typedef Signal1<void, const PropertyList &>
        PanelAgentSignalPropertyList;

typedef Signal2<void, int, int>
        PanelAgentSignalIntInt;

typedef Signal2<void, int &, int &>
        PanelAgentSignalIntInt2;

typedef Signal3<void, int, int, int>
        PanelAgentSignalIntIntInt;

typedef Signal4<void, int, int, int, int>
        PanelAgentSignalIntIntIntInt;

typedef Signal2<void, int, const Property &>
        PanelAgentSignalIntProperty;

typedef Signal2<void, int, const PropertyList &>
        PanelAgentSignalIntPropertyList;

typedef Signal2<void, int, const HelperInfo &>
        PanelAgentSignalIntHelperInfo;

typedef Signal3<void, const String &, const AttributeList &, int>
        PanelAgentSignalAttributeStringInt;

typedef Signal2<void, const String &, const AttributeList &>
        PanelAgentSignalAttributeString;

typedef Signal1<void, std::vector <String> &>
        PanelAgentSignalStringVector;

typedef Signal1<bool, std::vector <String> &>
        PanelAgentSignalBoolStringVector;

typedef Signal2<void, char *, std::vector <String> &>
        PanelAgentSignalStrStringVector;

typedef Signal2<bool, const String &, ISE_INFO &>
        PanelAgentSignalStringISEINFO;

typedef Signal1<void, const KeyEvent &>
        PanelAgentSignalKeyEvent;

typedef Signal1<void, struct rectinfo &>
        PanelAgentSignalRect;

typedef Signal6<bool, String, String &, String &, int &, int &, String &>
        PanelAgentSignalBoolString4int2;

enum ClientType {
    UNKNOWN_CLIENT,
    FRONTEND_CLIENT,
    FRONTEND_ACT_CLIENT,
    HELPER_CLIENT,
    HELPER_ACT_CLIENT,
    IMCONTROL_ACT_CLIENT,
    IMCONTROL_CLIENT
};

struct ClientInfo {
    uint32       key;
    ClientType   type;
};

struct HelperClientStub {
    int id;
    int ref;

    HelperClientStub (int i = 0, int r = 0) : id (i), ref (r) { }
};

struct IMControlStub {
    std::vector<ISE_INFO> info;
    std::vector<int> count;
};

static  int _id_count = -4;

#define DEFAULT_CONTEXT_VALUE 0xfff

#if SCIM_USE_STL_EXT_HASH_MAP
typedef __gnu_cxx::hash_map <int, ClientInfo, __gnu_cxx::hash <int> >       ClientRepository;
typedef __gnu_cxx::hash_map <int, HelperInfo, __gnu_cxx::hash <int> >       HelperInfoRepository;
typedef __gnu_cxx::hash_map <uint32, String, __gnu_cxx::hash <unsigned int> > ClientContextUUIDRepository;
typedef __gnu_cxx::hash_map <String, HelperClientStub, scim_hash_string>    HelperClientIndex;
typedef __gnu_cxx::hash_map <String, std::vector < std::pair <uint32, String> >, scim_hash_string>    StartHelperICIndex;
#elif SCIM_USE_STL_HASH_MAP
typedef std::hash_map <int, ClientInfo, std::hash <int> >                   ClientRepository;
typedef std::hash_map <int, HelperInfo, std::hash <int> >                   HelperInfoRepository;
typedef std::hash_map <uint32, String, std::hash <unsigned int> >           ClientContextUUIDRepository;
typedef std::hash_map <String, HelperClientStub, scim_hash_string>          HelperClientIndex;
typedef std::hash_map <String, std::vector < std::pair <uint32, String> >, scim_hash_string>          StartHelperICIndex;
#else
typedef std::map <int, ClientInfo>                                          ClientRepository;
typedef std::map <int, HelperInfo>                                          HelperInfoRepository;
typedef std::map <uint32, String>                                           ClientContextUUIDRepository;
typedef std::map <String, HelperClientStub>                                 HelperClientIndex;
typedef std::map <String, std::vector < std::pair <uint32, String> > >                                StartHelperICIndex;
#endif

typedef std::map <String, uint32>              UUIDCountRepository;
typedef std::map <String, enum HelperState>    UUIDStateRepository;
typedef std::map <String, int>                 StringIntRepository;
typedef std::map <int, struct IMControlStub>   IMControlRepository;
typedef std::map <int, int>                    IntIntRepository;

static uint32
get_helper_ic (int client, uint32 context)
{
    return (uint32) (client & 0xFFFF) | ((context & 0x7FFF) << 16);
}

static void
get_imengine_client_context (uint32 helper_ic, int &client, uint32 &context)
{
    client   = (int) (helper_ic & 0xFFFF);
    context  = ((helper_ic >> 16) & 0x7FFF);
}

//==================================== PanelAgent ===========================
class PanelAgent::PanelAgentImpl
{
    bool                                m_should_exit;
    bool                                m_should_resident;

    int                                 m_current_screen;

    String                              m_config_name;
    String                              m_display_name;

    int                                 m_socket_timeout;
    String                              m_socket_address;
    SocketServer                        m_socket_server;

    Transaction                         m_send_trans;
    Transaction                         m_recv_trans;
    Transaction                         m_nest_trans;

    int                                 m_current_socket_client;
    uint32                              m_current_client_context;
    String                              m_current_context_uuid;
    TOOLBAR_MODE_T                      m_current_toolbar_mode;
    uint32                              m_current_helper_option;
    String                              m_current_helper_uuid;
    String                              m_last_helper_uuid;
    String                              m_current_ise_name;
    int                                 m_pending_active_imcontrol_id;
    int                                 m_show_request_client_id;
    int                                 m_active_client_id;
    IntIntRepository                    m_panel_client_map;
    IntIntRepository                    m_imcontrol_map;
    bool                                m_should_shared_ise;
    bool                                m_ise_exiting;
    bool                                m_is_imengine_aux;
    bool                                m_is_imengine_candidate;

    int                                 m_last_socket_client;
    uint32                              m_last_client_context;
    String                              m_last_context_uuid;

    char                               *m_ise_context_buffer;
    size_t                              m_ise_context_length;

    ClientRepository                    m_client_repository;
    /*
    * Each Helper ISE has two socket connect between PanelAgent and HelperAgent.
    * m_helper_info_repository records the active connection.
    * m_helper_active_info_repository records the passive connection.
    */
    HelperInfoRepository                m_helper_info_repository;
    HelperInfoRepository                m_helper_active_info_repository;
    HelperClientIndex                   m_helper_client_index;

    /* when helper register, notify imcontrol client */
    StringIntRepository                 m_ise_pending_repository;
    IMControlRepository                 m_imcontrol_repository;

    StartHelperICIndex                  m_start_helper_ic_index;

    /* Keyboard ISE */
    ClientContextUUIDRepository         m_client_context_uuids;

    /* Helper ISE */
    ClientContextUUIDRepository         m_client_context_helper;
    UUIDCountRepository                 m_helper_uuid_count;

    HelperManager                       m_helper_manager;

    PanelAgentSignalVoid                m_signal_reload_config;
    PanelAgentSignalVoid                m_signal_turn_on;
    PanelAgentSignalVoid                m_signal_turn_off;
    PanelAgentSignalVoid                m_signal_show_panel;
    PanelAgentSignalVoid                m_signal_hide_panel;
    PanelAgentSignalInt                 m_signal_update_screen;
    PanelAgentSignalIntIntInt           m_signal_update_spot_location;
    PanelAgentSignalFactoryInfo         m_signal_update_factory_info;
    PanelAgentSignalVoid                m_signal_start_default_ise;
    PanelAgentSignalVoid                m_signal_stop_default_ise;
    PanelAgentSignalIntInt              m_signal_update_input_context;
    PanelAgentSignalIntInt              m_signal_set_candidate_ui;
    PanelAgentSignalIntInt2             m_signal_get_candidate_ui;
    PanelAgentSignalIntInt              m_signal_set_candidate_position;
    PanelAgentSignalRect                m_signal_get_candidate_geometry;
    PanelAgentSignalRect                m_signal_get_input_panel_geometry;
    PanelAgentSignalString              m_signal_set_keyboard_ise;
    PanelAgentSignalString2             m_signal_get_keyboard_ise;
    PanelAgentSignalString              m_signal_show_help;
    PanelAgentSignalFactoryInfoVector   m_signal_show_factory_menu;
    PanelAgentSignalVoid                m_signal_show_preedit_string;
    PanelAgentSignalVoid                m_signal_show_aux_string;
    PanelAgentSignalVoid                m_signal_show_lookup_table;
    PanelAgentSignalVoid                m_signal_show_associate_table;
    PanelAgentSignalVoid                m_signal_hide_preedit_string;
    PanelAgentSignalVoid                m_signal_hide_aux_string;
    PanelAgentSignalVoid                m_signal_hide_lookup_table;
    PanelAgentSignalVoid                m_signal_hide_associate_table;
    PanelAgentSignalAttributeStringInt  m_signal_update_preedit_string;
    PanelAgentSignalInt                 m_signal_update_preedit_caret;
    PanelAgentSignalAttributeString     m_signal_update_aux_string;
    PanelAgentSignalLookupTable         m_signal_update_lookup_table;
    PanelAgentSignalLookupTable         m_signal_update_associate_table;
    PanelAgentSignalPropertyList        m_signal_register_properties;
    PanelAgentSignalProperty            m_signal_update_property;
    PanelAgentSignalIntPropertyList     m_signal_register_helper_properties;
    PanelAgentSignalIntProperty         m_signal_update_helper_property;
    PanelAgentSignalIntHelperInfo       m_signal_register_helper;
    PanelAgentSignalInt                 m_signal_remove_helper;
    PanelAgentSignalStringBool          m_signal_set_active_ise_by_uuid;
    PanelAgentSignalVoid                m_signal_focus_in;
    PanelAgentSignalVoid                m_signal_focus_out;
    PanelAgentSignalVoid                m_signal_expand_candidate;
    PanelAgentSignalVoid                m_signal_contract_candidate;
    PanelAgentSignalInt                 m_signal_select_candidate;
    PanelAgentSignalBoolStringVector    m_signal_get_ise_list;
    PanelAgentSignalBoolString4int2     m_signal_get_ise_information;
    PanelAgentSignalBoolStringVector    m_signal_get_keyboard_ise_list;
    PanelAgentSignalIntIntIntInt        m_signal_update_ise_geometry;
    PanelAgentSignalStringVector        m_signal_get_language_list;
    PanelAgentSignalStringVector        m_signal_get_all_language;
    PanelAgentSignalStrStringVector     m_signal_get_ise_language;
    PanelAgentSignalStringISEINFO       m_signal_get_ise_info_by_uuid;
    PanelAgentSignalKeyEvent            m_signal_send_key_event;

    PanelAgentSignalInt                 m_signal_accept_connection;
    PanelAgentSignalInt                 m_signal_close_connection;
    PanelAgentSignalVoid                m_signal_exit;

    PanelAgentSignalVoid                m_signal_transaction_start;
    PanelAgentSignalVoid                m_signal_transaction_end;

    PanelAgentSignalVoid                m_signal_lock;
    PanelAgentSignalVoid                m_signal_unlock;

    PanelAgentSignalVoid                m_signal_show_ise;
    PanelAgentSignalVoid                m_signal_hide_ise;

    PanelAgentSignalVoid                m_signal_will_show_ack;
    PanelAgentSignalVoid                m_signal_will_hide_ack;

    PanelAgentSignalInt                 m_signal_set_keyboard_mode;

    PanelAgentSignalVoid                m_signal_candidate_will_hide_ack;
    PanelAgentSignalInt2                m_signal_get_ise_state;
public:
    PanelAgentImpl ()
        : m_should_exit (false),
          m_should_resident (false),
          m_current_screen (0),
          m_socket_timeout (scim_get_default_socket_timeout ()),
          m_current_socket_client (-1), m_current_client_context (0),
          m_current_toolbar_mode (TOOLBAR_KEYBOARD_MODE),
          m_current_helper_option (0),
          m_pending_active_imcontrol_id (-1),
          m_show_request_client_id (-1),
          m_active_client_id (-1),
          m_should_shared_ise (false),
          m_ise_exiting (false), m_is_imengine_aux (false), m_is_imengine_candidate (false),
          m_last_socket_client (-1), m_last_client_context (0),
          m_ise_context_buffer (NULL), m_ise_context_length (0)
    {
        m_current_ise_name = String (_("English Keyboard"));
        m_imcontrol_repository.clear ();
        m_imcontrol_map.clear ();
        m_panel_client_map.clear ();
        m_socket_server.signal_connect_accept (slot (this, &PanelAgentImpl::socket_accept_callback));
        m_socket_server.signal_connect_receive (slot (this, &PanelAgentImpl::socket_receive_callback));
        m_socket_server.signal_connect_exception (slot (this, &PanelAgentImpl::socket_exception_callback));
    }

    ~PanelAgentImpl ()
    {
        delete_ise_context_buffer ();
    }

    void delete_ise_context_buffer (void)
    {
        if (m_ise_context_buffer != NULL) {
            delete[] m_ise_context_buffer;
            m_ise_context_buffer = NULL;
            m_ise_context_length = 0;
        }
    }

    bool initialize (const String &config, const String &display, bool resident)
    {
        m_config_name = config;
        m_display_name = display;
        m_should_resident = resident;

        m_socket_address = scim_get_default_panel_socket_address (display);

        m_socket_server.shutdown ();

        /* If our helper manager could not connect to the HelperManager process,
           this panel agent's initialization has failed - since we are assuming
           the helper manager process should be launched beforehand */
        if (m_helper_manager.get_connection_number() == -1)
            return false;

        return m_socket_server.create (SocketAddress (m_socket_address));
    }

    bool valid (void) const
    {
        return m_socket_server.valid ();
    }

public:
    bool run (void)
    {
        SCIM_DEBUG_MAIN (1) << "PanelAgent::run ()\n";

        return m_socket_server.run ();
    }

    void stop (void)
    {
        SCIM_DEBUG_MAIN(1) << "PanelAgent::stop ()\n";

        lock ();
        m_should_exit = true;
        unlock ();

        SocketClient  client;

        if (client.connect (SocketAddress (m_socket_address))) {
            client.close ();
        }

        if (TOOLBAR_HELPER_MODE == m_current_toolbar_mode || m_current_helper_option & ISM_HELPER_PROCESS_KEYBOARD_KEYEVENT) {
            String helper_uuid = get_current_helper_uuid ();
            hide_helper (helper_uuid);
            stop_helper (helper_uuid, -2, 0);
        }
    }

    int get_helper_list (std::vector <HelperInfo> & helpers) const
    {
        SCIM_DEBUG_MAIN (1) << "PanelAgent::get_helper_list ()\n";

        helpers.clear ();

        m_helper_manager.get_helper_list ();
        unsigned int num = m_helper_manager.number_of_helpers ();
        HelperInfo info;

        SCIM_DEBUG_MAIN (2) << "Found " << num << " Helper objects\n";

        for (unsigned int i = 0; i < num; ++i) {
            if (m_helper_manager.get_helper_info (i, info) && info.uuid.length ()
                && (info.option & SCIM_HELPER_STAND_ALONE))
                helpers.push_back (info);

            SCIM_DEBUG_MAIN (3) << "Helper " << i << " : " << info.uuid << " : " << info.name << " : "
                                << ((info.option & SCIM_HELPER_STAND_ALONE) ? "SA " : "")
                                << ((info.option & SCIM_HELPER_AUTO_START) ? "AS " : "")
                                << ((info.option & SCIM_HELPER_AUTO_RESTART) ? "AR " : "") << "\n";
        }

        return (int)(helpers.size ());
    }

    TOOLBAR_MODE_T get_current_toolbar_mode () const
    {
        return m_current_toolbar_mode;
    }

    String get_current_ise_name () const
    {
        return m_current_ise_name;
    }

    String get_current_helper_uuid () const
    {
        return m_current_helper_uuid;
    }

    String get_current_helper_name () const
    {
        std::vector<HelperInfo> helpers;

        get_helper_list (helpers);

        std::vector<HelperInfo>::iterator iter;

        for (iter = helpers.begin (); iter != helpers.end (); iter++) {
            if (iter->uuid == m_current_helper_uuid)
                return iter->name;
        }

        return String ("");
    }

    uint32 get_current_helper_option () const
    {
        return m_current_helper_option;
    }

    void set_current_ise_name (String &name)
    {
        m_current_ise_name = name;
    }

    void set_current_toolbar_mode (TOOLBAR_MODE_T mode)
    {
        m_current_toolbar_mode = mode;
    }

    void set_current_helper_option (uint32 option)
    {
        m_current_helper_option = option;
    }

    void update_panel_event (int cmd, uint32 nType, uint32 nValue)
    {
        int    focused_client;
        uint32 focused_context;
        get_focused_context (focused_client, focused_context);
        if (focused_client == -1 && m_active_client_id != -1) {
            focused_client  = m_panel_client_map[m_active_client_id];
            focused_context = 0;
        }

        SCIM_DEBUG_MAIN(1) << __func__ << " (" << nType << ", " << nValue << "), client=" << focused_client << "\n";

        ClientInfo client_info = socket_get_client_info (focused_client);
        if (client_info.type == FRONTEND_CLIENT) {
            Socket client_socket (focused_client);
            m_send_trans.clear ();
            m_send_trans.put_command (SCIM_TRANS_CMD_REPLY);
            m_send_trans.put_data (focused_context);
            m_send_trans.put_command (cmd);
            m_send_trans.put_data (nType);
            m_send_trans.put_data (nValue);
            m_send_trans.write_to_socket (client_socket);
        } else {
            std::cerr << __func__ << " focused client is not existed!!!" << "\n";
        }

        if (m_panel_client_map[m_show_request_client_id] != focused_client) {
            client_info = socket_get_client_info (m_panel_client_map[m_show_request_client_id]);
            if (client_info.type == FRONTEND_CLIENT) {
                Socket client_socket (m_panel_client_map[m_show_request_client_id]);
                m_send_trans.clear ();
                m_send_trans.put_command (SCIM_TRANS_CMD_REPLY);
                m_send_trans.put_data (0);
                m_send_trans.put_command (cmd);
                m_send_trans.put_data (nType);
                m_send_trans.put_data (nValue);
                m_send_trans.write_to_socket (client_socket);
                std::cerr << __func__ << " show request client=" << m_panel_client_map[m_show_request_client_id] << "\n";
            } else {
                std::cerr << __func__ << " show request client is not existed!!!" << "\n";
            }
        }
    }

    bool move_preedit_caret (uint32 position)
    {
        SCIM_DEBUG_MAIN(1) << "PanelAgent::move_preedit_caret (" << position << ")\n";

        int client;
        uint32 context;

        lock ();

        get_focused_context (client, context);

        if (client >= 0) {
            Socket client_socket (client);
            m_send_trans.clear ();
            m_send_trans.put_command (SCIM_TRANS_CMD_REPLY);
            m_send_trans.put_data ((uint32) context);
            m_send_trans.put_command (SCIM_TRANS_CMD_MOVE_PREEDIT_CARET);
            m_send_trans.put_data ((uint32) position);
            m_send_trans.write_to_socket (client_socket);
        }

        unlock ();

        return client >= 0;
    }

    bool request_help (void)
    {
        SCIM_DEBUG_MAIN(1) << "PanelAgent::request_help ()\n";

        int client;
        uint32 context;

        lock ();

        get_focused_context (client, context);

        if (client >= 0) {
            Socket client_socket (client);
            m_send_trans.clear ();
            m_send_trans.put_command (SCIM_TRANS_CMD_REPLY);
            m_send_trans.put_data ((uint32) context);
            m_send_trans.put_command (SCIM_TRANS_CMD_PANEL_REQUEST_HELP);
            m_send_trans.write_to_socket (client_socket);
        }

        unlock ();

        return client >= 0;
    }

    bool request_factory_menu (void)
    {
        SCIM_DEBUG_MAIN(1) << "PanelAgent::request_factory_menu ()\n";

        int client;
        uint32 context;

        lock ();

        get_focused_context (client, context);

        if (client >= 0) {
            Socket client_socket (client);
            m_send_trans.clear ();
            m_send_trans.put_command (SCIM_TRANS_CMD_REPLY);
            m_send_trans.put_data ((uint32) context);
            m_send_trans.put_command (SCIM_TRANS_CMD_PANEL_REQUEST_FACTORY_MENU);
            m_send_trans.write_to_socket (client_socket);
        }

        unlock ();

        return client >= 0;
    }

    bool reset_keyboard_ise (void)
    {
        SCIM_DEBUG_MAIN(1) << "PanelAgent::reset_keyboard_ise ()\n";
        int    client = -1;
        uint32 context = 0;

        lock ();

        get_focused_context (client, context);
        if (client >= 0) {
            Socket client_socket (client);
            m_send_trans.clear ();
            m_send_trans.put_command (SCIM_TRANS_CMD_REPLY);
            m_send_trans.put_data ((uint32) context);
            m_send_trans.put_command (ISM_TRANS_CMD_PANEL_RESET_KEYBOARD_ISE);
            m_send_trans.write_to_socket (client_socket);
        }

        unlock ();

        return client >= 0;
    }

    bool update_keyboard_ise_list (void)
    {
        SCIM_DEBUG_MAIN(1) << "PanelAgent::update_keyboard_ise_list ()\n";
        int    client = -1;
        uint32 context = 0;

        lock ();

        get_focused_context (client, context);
        if (client >= 0) {
            Socket client_socket (client);
            m_send_trans.clear ();
            m_send_trans.put_command (SCIM_TRANS_CMD_REPLY);
            m_send_trans.put_data ((uint32) context);
            m_send_trans.put_command (ISM_TRANS_CMD_PANEL_UPDATE_KEYBOARD_ISE);
            m_send_trans.write_to_socket (client_socket);
        }

        unlock ();

        return client >= 0;
    }

    bool change_factory (const String  &uuid)
    {
        SCIM_DEBUG_MAIN(1) << "PanelAgent::change_factory (" << uuid << ")\n";

        int client;
        uint32 context;
        if (scim_global_config_read (SCIM_GLOBAL_CONFIG_PRELOAD_KEYBOARD_ISE, false))
            m_helper_manager.preload_keyboard_ise (uuid);
        lock ();

        get_focused_context (client, context);

        if (client >= 0) {
            Socket client_socket (client);
            m_send_trans.clear ();
            m_send_trans.put_command (SCIM_TRANS_CMD_REPLY);
            m_send_trans.put_data ((uint32) context);
            m_send_trans.put_command (SCIM_TRANS_CMD_PANEL_CHANGE_FACTORY);
            m_send_trans.put_data (uuid);
            m_send_trans.write_to_socket (client_socket);
        }

        unlock ();

        return client >= 0;
    }

    bool helper_candidate_show (void)
    {
        SCIM_DEBUG_MAIN(4) << __FUNCTION__ << "...\n";

        int    client;
        uint32 context;

        get_focused_context (client, context);

        if (TOOLBAR_HELPER_MODE == m_current_toolbar_mode || m_current_helper_option & ISM_HELPER_PROCESS_KEYBOARD_KEYEVENT) {
            HelperClientIndex::iterator it = m_helper_client_index.find (m_current_helper_uuid);

            if (it != m_helper_client_index.end ()) {
                Socket client_socket (it->second.id);
                uint32 ctx = get_helper_ic (client, context);

                m_send_trans.clear ();
                m_send_trans.put_command (SCIM_TRANS_CMD_REPLY);
                m_send_trans.put_data (ctx);
                m_send_trans.put_data (m_current_helper_uuid);
                m_send_trans.put_command (ISM_TRANS_CMD_CANDIDATE_SHOW);
                m_send_trans.write_to_socket (client_socket);

                return true;
            }
        }

        return false;
    }

    bool helper_candidate_hide (void)
    {
        SCIM_DEBUG_MAIN(4) << __FUNCTION__ << "...\n";

        int    client;
        uint32 context;

        get_focused_context (client, context);

        if (TOOLBAR_HELPER_MODE == m_current_toolbar_mode || m_current_helper_option & ISM_HELPER_PROCESS_KEYBOARD_KEYEVENT) {
            HelperClientIndex::iterator it = m_helper_client_index.find (m_current_helper_uuid);

            if (it != m_helper_client_index.end ()) {
                Socket client_socket (it->second.id);
                uint32 ctx = get_helper_ic (client, context);

                m_send_trans.clear ();
                m_send_trans.put_command (SCIM_TRANS_CMD_REPLY);
                m_send_trans.put_data (ctx);
                m_send_trans.put_data (m_current_helper_uuid);
                m_send_trans.put_command (ISM_TRANS_CMD_CANDIDATE_HIDE);
                m_send_trans.write_to_socket (client_socket);

                return true;
            }
        }

        return false;
    }

    bool candidate_more_window_show (void)
    {
        SCIM_DEBUG_MAIN(4) << __FUNCTION__ << "...\n";

        int    client;
        uint32 context;

        get_focused_context (client, context);
        if (m_is_imengine_candidate) {
            if (client >= 0) {
                Socket client_socket (client);
                m_send_trans.clear ();
                m_send_trans.put_command (SCIM_TRANS_CMD_REPLY);
                m_send_trans.put_data ((uint32) context);
                m_send_trans.put_command (ISM_TRANS_CMD_CANDIDATE_MORE_WINDOW_SHOW);
                m_send_trans.write_to_socket (client_socket);
                return true;
            }
        } else {
            if (TOOLBAR_HELPER_MODE == m_current_toolbar_mode || m_current_helper_option & ISM_HELPER_PROCESS_KEYBOARD_KEYEVENT) {
                HelperClientIndex::iterator it = m_helper_client_index.find (m_current_helper_uuid);

                if (it != m_helper_client_index.end ()) {
                    Socket client_socket (it->second.id);
                    uint32 ctx = get_helper_ic (client, context);

                    m_send_trans.clear ();
                    m_send_trans.put_command (SCIM_TRANS_CMD_REPLY);
                    m_send_trans.put_data (ctx);
                    m_send_trans.put_data (m_current_helper_uuid);
                    m_send_trans.put_command (ISM_TRANS_CMD_CANDIDATE_MORE_WINDOW_SHOW);
                    m_send_trans.write_to_socket (client_socket);

                    return true;
                }
            }
        }

        return false;
    }

    bool candidate_more_window_hide (void)
    {
        SCIM_DEBUG_MAIN(4) << __FUNCTION__ << "...\n";

        int    client;
        uint32 context;

        get_focused_context (client, context);
        if (m_is_imengine_candidate) {
            if (client >= 0) {
                Socket client_socket (client);
                m_send_trans.clear ();
                m_send_trans.put_command (SCIM_TRANS_CMD_REPLY);
                m_send_trans.put_data ((uint32) context);
                m_send_trans.put_command (ISM_TRANS_CMD_CANDIDATE_MORE_WINDOW_HIDE);
                m_send_trans.write_to_socket (client_socket);
                return true;
            }
        } else {
            if (TOOLBAR_HELPER_MODE == m_current_toolbar_mode || m_current_helper_option & ISM_HELPER_PROCESS_KEYBOARD_KEYEVENT) {
                HelperClientIndex::iterator it = m_helper_client_index.find (m_current_helper_uuid);

                if (it != m_helper_client_index.end ()) {
                    Socket client_socket (it->second.id);
                    uint32 ctx = get_helper_ic (client, context);

                    m_send_trans.clear ();
                    m_send_trans.put_command (SCIM_TRANS_CMD_REPLY);
                    m_send_trans.put_data (ctx);
                    m_send_trans.put_data (m_current_helper_uuid);
                    m_send_trans.put_command (ISM_TRANS_CMD_CANDIDATE_MORE_WINDOW_HIDE);
                    m_send_trans.write_to_socket (client_socket);

                    return true;
                }
            }
        }

        return false;
    }

    bool update_helper_lookup_table (const LookupTable &table)
    {
        SCIM_DEBUG_MAIN(4) << __FUNCTION__ << "...\n";

        int    client;
        uint32 context;

        if (TOOLBAR_HELPER_MODE == m_current_toolbar_mode || m_current_helper_option & ISM_HELPER_PROCESS_KEYBOARD_KEYEVENT) {
            HelperClientIndex::iterator it = m_helper_client_index.find (m_current_helper_uuid);

            if (it != m_helper_client_index.end ()) {
                Socket client_socket (it->second.id);
                uint32 ctx;

                get_focused_context (client, context);
                ctx = get_helper_ic (client, context);

                m_send_trans.clear ();
                m_send_trans.put_command (SCIM_TRANS_CMD_REPLY);
                m_send_trans.put_data (ctx);
                m_send_trans.put_data (m_current_helper_uuid);
                m_send_trans.put_command (ISM_TRANS_CMD_UPDATE_LOOKUP_TABLE);
                m_send_trans.put_data (table);
                m_send_trans.write_to_socket (client_socket);

                return true;
            }
        }

        return false;
    }

    bool select_aux (uint32 item)
    {
        SCIM_DEBUG_MAIN(1) << "PanelAgent::select_aux (" << item << ")\n";

        int client;
        uint32 context;

        lock ();

        get_focused_context (client, context);

        if (m_is_imengine_aux) {
            if (client >= 0) {
                Socket client_socket (client);
                m_send_trans.clear ();
                m_send_trans.put_command (SCIM_TRANS_CMD_REPLY);
                m_send_trans.put_data ((uint32) context);
                m_send_trans.put_command (ISM_TRANS_CMD_SELECT_AUX);
                m_send_trans.put_data ((uint32)item);
                m_send_trans.write_to_socket (client_socket);
            }
        } else {
            helper_select_aux (item);
        }

        unlock ();

        return client >= 0;
    }

    bool select_candidate (uint32 item)
    {
        SCIM_DEBUG_MAIN(1) << "PanelAgent::select_candidate (" << item << ")\n";

        int client;
        uint32 context;

        lock ();

        get_focused_context (client, context);

        if (m_is_imengine_candidate) {
            if (client >= 0) {
                Socket client_socket (client);
                m_send_trans.clear ();
                m_send_trans.put_command (SCIM_TRANS_CMD_REPLY);
                m_send_trans.put_data ((uint32) context);
                m_send_trans.put_command (SCIM_TRANS_CMD_SELECT_CANDIDATE);
                m_send_trans.put_data ((uint32)item);
                m_send_trans.write_to_socket (client_socket);
            }
        } else {
            helper_select_candidate (item);
        }

        unlock ();

        return client >= 0;
    }

    bool lookup_table_page_up (void)
    {
        SCIM_DEBUG_MAIN(1) << "PanelAgent::lookup_table_page_up ()\n";

        int client;
        uint32 context;

        lock ();

        get_focused_context (client, context);

        if (m_is_imengine_candidate) {
            if (client >= 0) {
                Socket client_socket (client);
                m_send_trans.clear ();
                m_send_trans.put_command (SCIM_TRANS_CMD_REPLY);
                m_send_trans.put_data ((uint32) context);
                m_send_trans.put_command (SCIM_TRANS_CMD_LOOKUP_TABLE_PAGE_UP);
                m_send_trans.write_to_socket (client_socket);
            }
        } else {
            helper_lookup_table_page_up ();
        }

        unlock ();

        return client >= 0;
    }

    bool lookup_table_page_down (void)
    {
        SCIM_DEBUG_MAIN(1) << "PanelAgent::lookup_table_page_down ()\n";

        int client;
        uint32 context;

        lock ();

        get_focused_context (client, context);

        if (m_is_imengine_candidate) {
            if (client >= 0) {
                Socket client_socket (client);
                m_send_trans.clear ();
                m_send_trans.put_command (SCIM_TRANS_CMD_REPLY);
                m_send_trans.put_data ((uint32) context);
                m_send_trans.put_command (SCIM_TRANS_CMD_LOOKUP_TABLE_PAGE_DOWN);
                m_send_trans.write_to_socket (client_socket);
            }
        } else {
            helper_lookup_table_page_down ();
        }

        unlock ();

        return client >= 0;
    }

    bool update_lookup_table_page_size (uint32 size)
    {
        SCIM_DEBUG_MAIN(1) << "PanelAgent::update_lookup_table_page_size (" << size << ")\n";

        int client;
        uint32 context;

        lock ();

        get_focused_context (client, context);

        if (m_is_imengine_candidate) {
            if (client >= 0) {
                Socket client_socket (client);
                m_send_trans.clear ();
                m_send_trans.put_command (SCIM_TRANS_CMD_REPLY);
                m_send_trans.put_data ((uint32) context);
                m_send_trans.put_command (SCIM_TRANS_CMD_UPDATE_LOOKUP_TABLE_PAGE_SIZE);
                m_send_trans.put_data (size);
                m_send_trans.write_to_socket (client_socket);
            }
        } else {
            helper_update_lookup_table_page_size (size);
        }

        unlock ();

        return client >= 0;
    }

    bool update_candidate_item_layout (const std::vector<uint32> &row_items)
    {
        SCIM_DEBUG_MAIN(1) << __func__ << " (" << row_items.size () << ")\n";

        int client;
        uint32 context;

        lock ();

        get_focused_context (client, context);

        if (m_is_imengine_candidate) {
            if (client >= 0) {
                Socket client_socket (client);
                m_send_trans.clear ();
                m_send_trans.put_command (SCIM_TRANS_CMD_REPLY);
                m_send_trans.put_data ((uint32) context);
                m_send_trans.put_command (ISM_TRANS_CMD_UPDATE_CANDIDATE_ITEM_LAYOUT);
                m_send_trans.put_data (row_items);
                m_send_trans.write_to_socket (client_socket);
            }
        } else {
            helper_update_candidate_item_layout (row_items);
        }

        unlock ();

        return client >= 0;
    }

    bool select_associate (uint32 item)
    {
        SCIM_DEBUG_MAIN(1) << "PanelAgent::select_associate (" << item << ")\n";

        int client;
        uint32 context;

        lock ();

        get_focused_context (client, context);

        if (client >= 0) {
            Socket client_socket (client);
            m_send_trans.clear ();
            m_send_trans.put_command (SCIM_TRANS_CMD_REPLY);
            m_send_trans.put_data ((uint32) context);
            m_send_trans.put_command (ISM_TRANS_CMD_SELECT_ASSOCIATE);
            m_send_trans.put_data ((uint32)item);
            m_send_trans.write_to_socket (client_socket);
        }

        unlock ();

        helper_select_associate (item);

        return client >= 0;
    }

    bool associate_table_page_up (void)
    {
        SCIM_DEBUG_MAIN(1) << "PanelAgent::associate_table_page_up ()\n";

        int client;
        uint32 context;

        lock ();

        get_focused_context (client, context);

        if (client >= 0) {
            Socket client_socket (client);
            m_send_trans.clear ();
            m_send_trans.put_command (SCIM_TRANS_CMD_REPLY);
            m_send_trans.put_data ((uint32) context);
            m_send_trans.put_command (ISM_TRANS_CMD_ASSOCIATE_TABLE_PAGE_UP);
            m_send_trans.write_to_socket (client_socket);
        }

        unlock ();

        helper_associate_table_page_up ();

        return client >= 0;
    }

    bool associate_table_page_down (void)
    {
        SCIM_DEBUG_MAIN(1) << "PanelAgent::associate_table_page_down ()\n";

        int client;
        uint32 context;

        lock ();

        get_focused_context (client, context);

        if (client >= 0) {
            Socket client_socket (client);
            m_send_trans.clear ();
            m_send_trans.put_command (SCIM_TRANS_CMD_REPLY);
            m_send_trans.put_data ((uint32) context);
            m_send_trans.put_command (ISM_TRANS_CMD_ASSOCIATE_TABLE_PAGE_DOWN);
            m_send_trans.write_to_socket (client_socket);
        }

        unlock ();

        helper_associate_table_page_down ();

        return client >= 0;
    }

    bool update_associate_table_page_size (uint32 size)
    {
        SCIM_DEBUG_MAIN(1) << "PanelAgent::update_associate_table_page_size (" << size << ")\n";

        int client;
        uint32 context;

        lock ();

        get_focused_context (client, context);

        if (client >= 0) {
            Socket client_socket (client);
            m_send_trans.clear ();
            m_send_trans.put_command (SCIM_TRANS_CMD_REPLY);
            m_send_trans.put_data ((uint32) context);
            m_send_trans.put_command (ISM_TRANS_CMD_UPDATE_ASSOCIATE_TABLE_PAGE_SIZE);
            m_send_trans.put_data (size);
            m_send_trans.write_to_socket (client_socket);
        }

        unlock ();

        helper_update_associate_table_page_size (size);

        return client >= 0;
    }

    bool update_displayed_candidate_number (uint32 size)
    {
        SCIM_DEBUG_MAIN(1) << __func__ << " (" << size << ")\n";

        int client;
        uint32 context;

        lock ();
        get_focused_context (client, context);

        if (m_is_imengine_candidate) {
            if (client >= 0) {
                Socket client_socket (client);
                m_send_trans.clear ();
                m_send_trans.put_command (SCIM_TRANS_CMD_REPLY);
                m_send_trans.put_data ((uint32) context);
                m_send_trans.put_command (ISM_TRANS_CMD_UPDATE_DISPLAYED_CANDIDATE);
                m_send_trans.put_data (size);
                m_send_trans.write_to_socket (client_socket);
            }
        } else {
            helper_update_displayed_candidate_number (size);
        }
        unlock ();

        return client >= 0;
    }

    void send_longpress_event (int type, int index)
    {
        SCIM_DEBUG_MAIN(1) << __func__ << " (" << type << ", " << index << ")\n";

        int    client;
        uint32 context;

        get_focused_context (client, context);
        if (m_is_imengine_candidate) {
            if (client >= 0) {
                Socket client_socket (client);
                m_send_trans.clear ();
                m_send_trans.put_command (SCIM_TRANS_CMD_REPLY);
                m_send_trans.put_data ((uint32) context);
                m_send_trans.put_command (ISM_TRANS_CMD_LONGPRESS_CANDIDATE);
                m_send_trans.put_data (index);
                m_send_trans.write_to_socket (client_socket);
            }
        } else {
            helper_longpress_candidate (index);
        }
    }

    bool trigger_property (const String  &property)
    {
        SCIM_DEBUG_MAIN(1) << "PanelAgent::trigger_property (" << property << ")\n";

        int client;
        uint32 context;

        lock ();

        get_focused_context (client, context);

        if (client >= 0) {
            Socket client_socket (client);
            m_send_trans.clear ();
            m_send_trans.put_command (SCIM_TRANS_CMD_REPLY);
            m_send_trans.put_data ((uint32) context);
            m_send_trans.put_command (SCIM_TRANS_CMD_TRIGGER_PROPERTY);
            m_send_trans.put_data (property);
            m_send_trans.write_to_socket (client_socket);
        }

        unlock ();

        return client >= 0;
    }

    bool trigger_helper_property (int            client,
                                  const String  &property)
    {
        SCIM_DEBUG_MAIN(1) << "PanelAgent::trigger_helper_property (" << client << "," << property << ")\n";

        lock ();

        ClientInfo info = socket_get_client_info (client);

        if (client >= 0 && info.type == HELPER_CLIENT) {
            int fe_client;
            uint32 fe_context;
            String fe_uuid;

            fe_uuid = get_focused_context (fe_client, fe_context);

            Socket client_socket (client);
            m_send_trans.clear ();
            m_send_trans.put_command (SCIM_TRANS_CMD_REPLY);

            /* FIXME: We presume that client and context are both less than 65536.
             * Hopefully, it should be true in any UNIXs.
             * So it's ok to combine client and context into one uint32.*/
            m_send_trans.put_data (get_helper_ic (fe_client, fe_context));
            m_send_trans.put_data (fe_uuid);
            m_send_trans.put_command (SCIM_TRANS_CMD_TRIGGER_PROPERTY);
            m_send_trans.put_data (property);
            m_send_trans.write_to_socket (client_socket);
        }

        unlock ();

        return client >= 0 && info.type == HELPER_CLIENT;
    }

    bool start_helper (const String  &uuid, int client, uint32 context)
    {
        SCIM_DEBUG_MAIN(1) << "PanelAgent::start_helper (" << uuid << ")\n";
        ISF_SAVE_LOG ("start ISE(%s)\n", uuid.c_str ());

        if (uuid.length () <= 0)
            return false;

        lock ();

        /*if (m_current_toolbar_mode != TOOLBAR_HELPER_MODE || m_current_helper_uuid.compare (uuid) != 0)*/ {
            SCIM_DEBUG_MAIN(1) << "Run_helper\n";
            m_helper_manager.run_helper (uuid, m_config_name, m_display_name);
        }
        m_current_helper_uuid = uuid;

        unlock ();

        return true;
    }

    bool stop_helper (const String &uuid, int client, uint32 context)
    {
        char buf[256] = {0};
        snprintf (buf, sizeof (buf), "time:%ld  pid:%d  %s  %s  prepare to stop ISE(%s)\n",
            time (0), getpid (), __FILE__, __func__, uuid.c_str ());
        ISF_SAVE_LOG ("prepare to stop ISE(%s)\n", uuid.c_str ());

        SCIM_DEBUG_MAIN(1) << "PanelAgent::stop_helper (" << uuid << ")\n";
        if (uuid.length () <= 0)
            return false;

        lock ();

        uint32 ctx = get_helper_ic (client, context);
        HelperClientIndex::iterator it = m_helper_client_index.find (uuid);
        if (it != m_helper_client_index.end ())
        {
            Socket client_socket (it->second.id);
            m_send_trans.clear ();
            m_send_trans.put_command (SCIM_TRANS_CMD_REPLY);
            m_send_trans.put_data (ctx);
            m_send_trans.put_data (uuid);

            m_ise_exiting = true;
            m_current_helper_uuid = String ("");
            m_send_trans.put_command (SCIM_TRANS_CMD_EXIT);
            m_send_trans.write_to_socket (client_socket);
            SCIM_DEBUG_MAIN(1) << "Stop helper\n";
            ISF_SAVE_LOG ("send SCIM_TRANS_CMD_EXIT message\n");
        }

        unlock ();

        return true;
    }

    void focus_out_helper (const String &uuid, int client, uint32 context)
    {
        HelperClientIndex::iterator it = m_helper_client_index.find (m_current_helper_uuid);

        if (it != m_helper_client_index.end ()) {
            Socket client_socket (it->second.id);
            uint32 ctx = get_helper_ic (client, context);

            m_send_trans.clear ();
            m_send_trans.put_command (SCIM_TRANS_CMD_REPLY);
            m_send_trans.put_data (ctx);
            m_send_trans.put_data (uuid);
            m_send_trans.put_command (SCIM_TRANS_CMD_FOCUS_OUT);
            m_send_trans.write_to_socket (client_socket);
        }
    }

    void focus_in_helper (const String &uuid, int client, uint32 context)
    {
        HelperClientIndex::iterator it = m_helper_client_index.find (m_current_helper_uuid);

        if (it != m_helper_client_index.end ()) {
            Socket client_socket (it->second.id);
            uint32 ctx = get_helper_ic (client, context);

            m_send_trans.clear ();
            m_send_trans.put_command (SCIM_TRANS_CMD_REPLY);
            m_send_trans.put_data (ctx);
            m_send_trans.put_data (uuid);
            m_send_trans.put_command (SCIM_TRANS_CMD_FOCUS_IN);
            m_send_trans.write_to_socket (client_socket);
        }
    }

    bool show_helper (const String &uuid, char *data, size_t &len, uint32 ctx)
    {
        HelperClientIndex::iterator it = m_helper_client_index.find (m_current_helper_uuid);

        if (it != m_helper_client_index.end ()) {
            Socket client_socket (it->second.id);

            m_send_trans.clear ();
            m_send_trans.put_command (SCIM_TRANS_CMD_REPLY);
            m_send_trans.put_data (ctx);
            m_send_trans.put_data (uuid);
            m_send_trans.put_command (ISM_TRANS_CMD_SHOW_ISE_PANEL);
            m_send_trans.put_data (data, len);
            m_send_trans.write_to_socket (client_socket);

            ISF_SAVE_LOG ("Send ISM_TRANS_CMD_SHOW_ISE_PANEL message\n");

            return true;
        }
        return false;
    }

    void hide_helper (const String &uuid, uint32 ctx = 0)
    {
        HelperClientIndex::iterator it = m_helper_client_index.find (m_current_helper_uuid);

        if (it != m_helper_client_index.end ()) {
            int client;
            uint32 context;
            Socket client_socket (it->second.id);

            if (ctx == 0) {
                get_focused_context (client, context);
                ctx = get_helper_ic (client, context);
            }

            m_send_trans.clear ();
            m_send_trans.put_command (SCIM_TRANS_CMD_REPLY);
            m_send_trans.put_data (ctx);
            m_send_trans.put_data (uuid);
            m_send_trans.put_command (ISM_TRANS_CMD_HIDE_ISE_PANEL);
            m_send_trans.write_to_socket (client_socket);

            ISF_SAVE_LOG ("Send ISM_TRANS_CMD_HIDE_ISE_PANEL message\n");
        }
    }

    bool set_helper_mode (const String &uuid, uint32 &mode)
    {
        HelperClientIndex::iterator it = m_helper_client_index.find (m_current_helper_uuid);

        if (it != m_helper_client_index.end ()) {
            int client;
            uint32 context;
            Socket client_socket (it->second.id);
            uint32 ctx;

            get_focused_context (client, context);
            ctx = get_helper_ic (client, context);

            m_send_trans.clear ();
            m_send_trans.put_command (SCIM_TRANS_CMD_REPLY);
            m_send_trans.put_data (ctx);
            m_send_trans.put_data (uuid);
            m_send_trans.put_command (ISM_TRANS_CMD_SET_ISE_MODE);
            m_send_trans.put_data (mode);
            m_send_trans.write_to_socket (client_socket);
            return true;
        }

        return false;
    }

    bool set_helper_language (const String &uuid, uint32 &language)
    {
        HelperClientIndex::iterator it = m_helper_client_index.find (m_current_helper_uuid);

        if (it != m_helper_client_index.end ()) {
            int client     = -1;
            uint32 context = 0;
            Socket client_socket (it->second.id);
            uint32 ctx;

            get_focused_context (client, context);
            ctx = get_helper_ic (client, context);

            m_send_trans.clear ();
            m_send_trans.put_command (SCIM_TRANS_CMD_REPLY);
            m_send_trans.put_data (ctx);
            m_send_trans.put_data (uuid);
            m_send_trans.put_command (ISM_TRANS_CMD_SET_ISE_LANGUAGE);
            m_send_trans.put_data (language);
            m_send_trans.write_to_socket (client_socket);
            return true;
        }
        return false;
    }

    bool set_helper_imdata (const String &uuid, char *imdata, size_t &len)
    {
        HelperClientIndex::iterator it = m_helper_client_index.find (m_current_helper_uuid);

        if (it != m_helper_client_index.end ()) {
            int client     = -1;
            uint32 context = 0;
            Socket client_socket (it->second.id);
            uint32 ctx;

            get_focused_context (client, context);
            ctx = get_helper_ic (client, context);

            m_send_trans.clear ();
            m_send_trans.put_command (SCIM_TRANS_CMD_REPLY);
            m_send_trans.put_data (ctx);
            m_send_trans.put_data (uuid);
            m_send_trans.put_command (ISM_TRANS_CMD_SET_ISE_IMDATA);
            m_send_trans.put_data (imdata, len);
            m_send_trans.write_to_socket (client_socket);
            return true;
        }
        return false;
    }

    bool set_helper_return_key_type (const String &uuid, int type)
    {
        HelperClientIndex::iterator it = m_helper_client_index.find (m_current_helper_uuid);

        if (it != m_helper_client_index.end ()) {
            int client     = -1;
            uint32 context = 0;
            Socket client_socket (it->second.id);
            uint32 ctx;

            get_focused_context (client, context);
            ctx = get_helper_ic (client, context);

            m_send_trans.clear ();
            m_send_trans.put_command (SCIM_TRANS_CMD_REPLY);
            m_send_trans.put_data (ctx);
            m_send_trans.put_data (uuid);
            m_send_trans.put_command (ISM_TRANS_CMD_SET_RETURN_KEY_TYPE);
            m_send_trans.put_data (type);
            m_send_trans.write_to_socket (client_socket);

            return true;
        }

        return false;
    }

    bool get_helper_return_key_type (const String &uuid, uint32 &type)
    {
        HelperClientIndex::iterator it = m_helper_client_index.find (m_current_helper_uuid);

        if (it != m_helper_client_index.end ()) {

            int    client;
            uint32 context;
            Socket client_socket (it->second.id);
            uint32 ctx;
            Transaction trans;

            get_focused_context (client, context);
            ctx = get_helper_ic (client, context);

            trans.clear ();
            trans.put_command (SCIM_TRANS_CMD_REPLY);
            trans.put_data (ctx);
            trans.put_data (uuid);
            trans.put_command (ISM_TRANS_CMD_GET_RETURN_KEY_TYPE);

            int cmd;
            if (trans.write_to_socket (client_socket)
                && trans.read_from_socket (client_socket)
                && trans.get_command(cmd) && cmd == SCIM_TRANS_CMD_REPLY
                && trans.get_data (type)) {
                SCIM_DEBUG_MAIN (1) << __func__ << " success\n";
                return true;
            } else {
                std::cerr << __func__ << " failed\n";
            }
        }
        return false;
    }

    bool set_helper_return_key_disable (const String &uuid, bool disabled)
    {
        HelperClientIndex::iterator it = m_helper_client_index.find (m_current_helper_uuid);

        if (it != m_helper_client_index.end ()) {
            int client     = -1;
            uint32 context = 0;
            Socket client_socket (it->second.id);
            uint32 ctx;

            get_focused_context (client, context);
            ctx = get_helper_ic (client, context);

            m_send_trans.clear ();
            m_send_trans.put_command (SCIM_TRANS_CMD_REPLY);
            m_send_trans.put_data (ctx);
            m_send_trans.put_data (uuid);
            m_send_trans.put_command (ISM_TRANS_CMD_SET_RETURN_KEY_DISABLE);
            m_send_trans.put_data (disabled);
            m_send_trans.write_to_socket (client_socket);

            return true;
        }

        return false;
    }

    bool get_helper_return_key_disable (const String &uuid, uint32 &disabled)
    {
        HelperClientIndex::iterator it = m_helper_client_index.find (m_current_helper_uuid);

        if (it != m_helper_client_index.end ()) {
            int    client;
            uint32 context;
            Socket client_socket (it->second.id);
            uint32 ctx;
            Transaction trans;

            get_focused_context (client, context);
            ctx = get_helper_ic (client, context);

            trans.clear ();
            trans.put_command (SCIM_TRANS_CMD_REPLY);
            trans.put_data (ctx);
            trans.put_data (uuid);
            trans.put_command (ISM_TRANS_CMD_GET_RETURN_KEY_DISABLE);

            int cmd;
            if (trans.write_to_socket (client_socket)
                && trans.read_from_socket (client_socket)
                && trans.get_command(cmd) && cmd == SCIM_TRANS_CMD_REPLY
                && trans.get_data (disabled)) {
                SCIM_DEBUG_MAIN (1) << __func__ << " success\n";
                return true;
            } else {
                std::cerr << __func__ << " failed\n";
            }
        }
        return false;
    }

    bool set_helper_layout (const String &uuid, uint32 &layout)
    {
        HelperClientIndex::iterator it = m_helper_client_index.find (m_current_helper_uuid);

        if (it != m_helper_client_index.end ()) {
            int client;
            uint32 context;
            Socket client_socket (it->second.id);
            uint32 ctx;

            get_focused_context (client, context);
            ctx = get_helper_ic (client, context);

            m_send_trans.clear ();
            m_send_trans.put_command (SCIM_TRANS_CMD_REPLY);
            m_send_trans.put_data (ctx);
            m_send_trans.put_data (uuid);
            m_send_trans.put_command (ISM_TRANS_CMD_SET_LAYOUT);
            m_send_trans.put_data (layout);
            m_send_trans.write_to_socket (client_socket);
            return true;
        }

        return false;
    }

    bool set_helper_input_mode (const String &uuid, uint32 &mode)
    {
        HelperClientIndex::iterator it = m_helper_client_index.find (m_current_helper_uuid);

        if (it != m_helper_client_index.end ()) {
            int client;
            uint32 context;
            Socket client_socket (it->second.id);
            uint32 ctx;

            get_focused_context (client, context);
            ctx = get_helper_ic (client, context);

            m_send_trans.clear ();
            m_send_trans.put_command (SCIM_TRANS_CMD_REPLY);
            m_send_trans.put_data (ctx);
            m_send_trans.put_data (uuid);
            m_send_trans.put_command (ISM_TRANS_CMD_SET_INPUT_MODE);
            m_send_trans.put_data (mode);
            m_send_trans.write_to_socket (client_socket);
            return true;
        }

        return false;
    }

    bool set_helper_input_hint (const String &uuid, uint32 &hint)
    {
        HelperClientIndex::iterator it = m_helper_client_index.find (m_current_helper_uuid);

        if (it != m_helper_client_index.end ()) {
            int client;
            uint32 context;
            Socket client_socket (it->second.id);
            uint32 ctx;

            get_focused_context (client, context);
            ctx = get_helper_ic (client, context);

            m_send_trans.clear ();
            m_send_trans.put_command (SCIM_TRANS_CMD_REPLY);
            m_send_trans.put_data (ctx);
            m_send_trans.put_data (uuid);
            m_send_trans.put_command (ISM_TRANS_CMD_SET_INPUT_HINT);
            m_send_trans.put_data (hint);
            m_send_trans.write_to_socket (client_socket);
            return true;
        }

        return false;
    }

    bool set_helper_bidi_direction (const String &uuid, uint32 &direction)
    {
        HelperClientIndex::iterator it = m_helper_client_index.find (m_current_helper_uuid);

        if (it != m_helper_client_index.end ()) {
            int client;
            uint32 context;
            Socket client_socket (it->second.id);
            uint32 ctx;

            get_focused_context (client, context);
            ctx = get_helper_ic (client, context);

            m_send_trans.clear ();
            m_send_trans.put_command (SCIM_TRANS_CMD_REPLY);
            m_send_trans.put_data (ctx);
            m_send_trans.put_data (uuid);
            m_send_trans.put_command (ISM_TRANS_CMD_UPDATE_BIDI_DIRECTION);
            m_send_trans.put_data (direction);
            m_send_trans.write_to_socket (client_socket);
            return true;
        }

        return false;
    }

    bool set_helper_caps_mode (const String &uuid, uint32 &mode)
    {
        HelperClientIndex::iterator it = m_helper_client_index.find (m_current_helper_uuid);

        if (it != m_helper_client_index.end ()) {
            int client;
            uint32 context;
            Socket client_socket (it->second.id);
            uint32 ctx;

            get_focused_context (client, context);
            ctx = get_helper_ic (client, context);

            m_send_trans.clear ();
            m_send_trans.put_command (SCIM_TRANS_CMD_REPLY);
            m_send_trans.put_data (ctx);
            m_send_trans.put_data (uuid);
            m_send_trans.put_command (ISM_TRANS_CMD_SET_CAPS_MODE);
            m_send_trans.put_data (mode);
            m_send_trans.write_to_socket (client_socket);
            return true;
        }

        return false;
    }

    bool show_helper_option_window (const String &uuid)
    {
        HelperClientIndex::iterator it = m_helper_client_index.find (m_current_helper_uuid);

        if (it != m_helper_client_index.end ()) {
            int client;
            uint32 context;
            Socket client_socket (it->second.id);
            uint32 ctx;

            get_focused_context (client, context);
            ctx = get_helper_ic (client, context);

            m_send_trans.clear ();
            m_send_trans.put_command (SCIM_TRANS_CMD_REPLY);
            m_send_trans.put_data (ctx);
            m_send_trans.put_data (uuid);
            m_send_trans.put_command (ISM_TRANS_CMD_SHOW_ISE_OPTION_WINDOW);
            m_send_trans.write_to_socket (client_socket);

            ISF_SAVE_LOG ("Send ISM_TRANS_CMD_SHOW_ISE_OPTION_WINDOW message\n");

            return true;
        }
        return false;
    }

    void show_isf_panel (int client_id)
    {
        SCIM_DEBUG_MAIN(4) << "PanelAgent::show_isf_panel ()\n";
        m_signal_show_panel ();
    }

    void hide_isf_panel (int client_id)
    {
        SCIM_DEBUG_MAIN(4) << "PanelAgent::hide_isf_panel ()\n";
        m_signal_hide_panel ();
    }

    void show_ise_panel (int client_id)
    {
        SCIM_DEBUG_MAIN(4) << "PanelAgent::show_ise_panel ()\n";

        String initial_uuid = scim_global_config_read (String (SCIM_GLOBAL_CONFIG_INITIAL_ISE_UUID), String (""));
        String default_uuid = scim_global_config_read (String (SCIM_GLOBAL_CONFIG_DEFAULT_ISE_UUID), String (""));

        ISF_SAVE_LOG ("prepare to show ISE %d [%s] [%s]\n", client_id, initial_uuid.c_str(), default_uuid.c_str());

        char   *data = NULL;
        size_t  len;
        bool ret = false;
        Transaction trans;
        Socket client_socket (client_id);
        m_show_request_client_id = client_id;
        m_active_client_id = client_id;

        uint32 client;
        uint32 context;
        if (m_recv_trans.get_data (client) && m_recv_trans.get_data (context) && m_recv_trans.get_data (&data, len)) {
            SCIM_DEBUG_MAIN(4) << __func__ << " (client:" << client << " context:" << context << ")\n";
            if (TOOLBAR_HELPER_MODE == m_current_toolbar_mode) {
                int    focused_client;
                uint32 focused_context;
                get_focused_context (focused_client, focused_context);
                if (focused_client == -1 && m_active_client_id != -1) {
                    focused_client  = m_panel_client_map[m_active_client_id];
                    focused_context = 0;
                }

                uint32 ctx = get_helper_ic (focused_client, focused_context);
                ret = show_helper (m_current_helper_uuid, data, len, ctx);
            }
            /* Save ISE context for ISE panel re-showing */
            if (data && len > 0) {
                delete_ise_context_buffer ();
                m_ise_context_buffer = new char [len];
                if (m_ise_context_buffer) {
                    m_ise_context_length = len;
                    memcpy (m_ise_context_buffer, data, m_ise_context_length);
                }
            }
        }

        trans.clear ();
        trans.put_command (SCIM_TRANS_CMD_REPLY);
        trans.put_command (SCIM_TRANS_CMD_OK);
        trans.put_data (ret);
        trans.write_to_socket (client_socket);
        if (data != NULL)
            delete[] data;

        if (ret) {
            m_signal_show_ise ();
        } else {
            m_signal_start_default_ise ();
        }
    }

    void hide_ise_panel (int client_id)
    {
        SCIM_DEBUG_MAIN(4) << "PanelAgent::hide_ise_panel ()\n";
        ISF_SAVE_LOG ("prepare to hide ISE, %d %d\n", client_id, m_show_request_client_id);

        uint32 client;
        uint32 context;
        if (m_recv_trans.get_data (client) && m_recv_trans.get_data (context)) {
            SCIM_DEBUG_MAIN(4) << __func__ << " (client:" << client << " context:" << context << ")\n";
            if ((m_panel_client_map[client_id] == m_current_socket_client || client_id == m_show_request_client_id)
                && (TOOLBAR_HELPER_MODE == m_current_toolbar_mode || m_current_helper_option & ISM_HELPER_PROCESS_KEYBOARD_KEYEVENT)) {
                int    focused_client;
                uint32 focused_context;
                get_focused_context (focused_client, focused_context);
                if (focused_client == -1 && m_active_client_id != -1) {
                    focused_client  = m_panel_client_map[m_active_client_id];
                    focused_context = 0;
                }

                m_signal_hide_ise ();
            }
            /* Release ISE context buffer */
            delete_ise_context_buffer ();
        }
    }

    void set_default_ise (const DEFAULT_ISE_T &ise)
    {
        scim_global_config_write (String (SCIM_GLOBAL_CONFIG_DEFAULT_ISE_UUID), ise.uuid);
        scim_global_config_flush ();
    }

    void set_should_shared_ise (const bool should_shared_ise)
    {
        m_should_shared_ise = should_shared_ise;
    }

    bool process_key_event (const String &uuid, KeyEvent& key, uint32 &result)
    {
        HelperClientIndex::iterator it = m_helper_client_index.find (m_current_helper_uuid);

        if (it != m_helper_client_index.end ()) {
            int    client;
            uint32 context;
            Socket client_socket (it->second.id);
            uint32 ctx;
            Transaction trans;

            get_focused_context (client, context);
            ctx = get_helper_ic (client, context);

            trans.clear ();
            trans.put_command (SCIM_TRANS_CMD_REPLY);
            trans.put_data (ctx);
            trans.put_data (uuid);
            trans.put_command (SCIM_TRANS_CMD_PROCESS_KEY_EVENT);
            trans.put_data (key);
            int cmd;
            if (trans.write_to_socket (client_socket)
                && trans.read_from_socket (client_socket)
                && trans.get_command (cmd) && cmd == SCIM_TRANS_CMD_REPLY
                && trans.get_data (result)) {
                SCIM_DEBUG_MAIN (1) << __func__ << " success\n";
                return true;
            } else {
                std::cerr << __func__ << " failed\n";
            }
        }
        return false;
    }

    bool get_helper_geometry (String &uuid, struct rectinfo &info)
    {
        HelperClientIndex::iterator it = m_helper_client_index.find (m_current_helper_uuid);

        if (it != m_helper_client_index.end ()) {
            int    client;
            uint32 context;
            Socket client_socket (it->second.id);
            uint32 ctx;
            Transaction trans;

            get_focused_context (client, context);
            ctx = get_helper_ic (client, context);

            trans.clear ();
            trans.put_command (SCIM_TRANS_CMD_REPLY);
            trans.put_data (ctx);
            trans.put_data (uuid);
            trans.put_command (ISM_TRANS_CMD_GET_ACTIVE_ISE_GEOMETRY);

            if (trans.write_to_socket (client_socket)) {
                int cmd;

                trans.clear ();
                if (trans.read_from_socket (client_socket)
                    && trans.get_command(cmd) && cmd == SCIM_TRANS_CMD_REPLY
                    && trans.get_data (info.pos_x)
                    && trans.get_data (info.pos_y)
                    && trans.get_data (info.width)
                    && trans.get_data (info.height)) {
                    SCIM_DEBUG_MAIN (1) << __func__ << " is successful\n";
                    return true;
                } else {
                    std::cerr << __func__ << " is failed!!!\n";
                    return false;
                }
            }
        }
        return false;
    }

    bool get_helper_imdata (String &uuid, char **imdata, size_t &len)
    {
        HelperClientIndex::iterator it = m_helper_client_index.find (m_current_helper_uuid);

        if (it != m_helper_client_index.end ()) {
            int    client;
            uint32 context;
            Socket client_socket (it->second.id);
            uint32 ctx;
            Transaction trans;

            get_focused_context (client, context);
            ctx = get_helper_ic (client, context);

            trans.clear ();
            trans.put_command (SCIM_TRANS_CMD_REPLY);
            trans.put_data (ctx);
            trans.put_data (uuid);
            trans.put_command (ISM_TRANS_CMD_GET_ISE_IMDATA);

            int cmd;
            if (trans.write_to_socket (client_socket)
                && trans.read_from_socket (client_socket)
                && trans.get_command(cmd) && cmd == SCIM_TRANS_CMD_REPLY
                && trans.get_data (imdata, len)) {
                SCIM_DEBUG_MAIN (1) << "get_helper_imdata success\n";
                return true;
            } else {
                std::cerr << "get_helper_imdata failed\n";
            }
        }
        return false;
    }

    bool get_helper_layout (String &uuid, uint32 &layout)
    {
        HelperClientIndex::iterator it = m_helper_client_index.find (m_current_helper_uuid);

        if (it != m_helper_client_index.end ()) {
            int    client;
            uint32 context;
            Socket client_socket (it->second.id);
            uint32 ctx;
            Transaction trans;

            get_focused_context (client, context);
            ctx = get_helper_ic (client, context);

            trans.clear ();
            trans.put_command (SCIM_TRANS_CMD_REPLY);
            trans.put_data (ctx);
            trans.put_data (uuid);
            trans.put_command (ISM_TRANS_CMD_GET_LAYOUT);

            int cmd;
            if (trans.write_to_socket (client_socket)
                && trans.read_from_socket (client_socket)
                && trans.get_command(cmd) && cmd == SCIM_TRANS_CMD_REPLY
                && trans.get_data (layout)) {
                SCIM_DEBUG_MAIN (1) << "get_helper_layout success\n";
                return true;
            } else {
                std::cerr << "get_helper_layout failed\n";
            }
        }
        return false;
    }

    void get_input_panel_geometry (int client_id)
    {
        SCIM_DEBUG_MAIN(4) << __func__ << "\n";

        struct rectinfo info = {0, 0, 0, 0};
        m_signal_get_input_panel_geometry (info);

        Transaction trans;
        Socket client_socket (client_id);

        trans.clear ();
        trans.put_command (SCIM_TRANS_CMD_REPLY);
        trans.put_command (SCIM_TRANS_CMD_OK);
        trans.put_data (info.pos_x);
        trans.put_data (info.pos_y);
        trans.put_data (info.width);
        trans.put_data (info.height);
        trans.write_to_socket (client_socket);
    }

    void get_candidate_window_geometry (int client_id)
    {
        SCIM_DEBUG_MAIN(4) << __func__ << "\n";

        struct rectinfo info = {0, 0, 0, 0};
        m_signal_get_candidate_geometry (info);

        Transaction trans;
        Socket client_socket (client_id);

        trans.clear ();
        trans.put_command (SCIM_TRANS_CMD_REPLY);
        trans.put_command (SCIM_TRANS_CMD_OK);
        trans.put_data (info.pos_x);
        trans.put_data (info.pos_y);
        trans.put_data (info.width);
        trans.put_data (info.height);
        trans.write_to_socket (client_socket);
    }

    void get_ise_language_locale (int client_id)
    {
        SCIM_DEBUG_MAIN(4) << __func__ << "\n";

        size_t  len;
        char   *data = NULL;
        Transaction trans;

        if (TOOLBAR_HELPER_MODE == m_current_toolbar_mode || m_current_helper_option & ISM_HELPER_PROCESS_KEYBOARD_KEYEVENT) {
            HelperClientIndex::iterator it = m_helper_client_index.find (m_current_helper_uuid);

            if (it != m_helper_client_index.end ()) {
                int    client;
                uint32 context;
                get_focused_context (client, context);

                uint32 ctx = get_helper_ic (client, context);

                trans.clear ();
                trans.put_command (SCIM_TRANS_CMD_REPLY);
                trans.put_data (ctx);
                trans.put_data (m_current_helper_uuid);
                trans.put_command (ISM_TRANS_CMD_GET_ISE_LANGUAGE_LOCALE);

                int cmd;
                Socket client_socket (it->second.id);
                if (trans.write_to_socket (client_socket)
                    && trans.read_from_socket (client_socket)
                    && trans.get_command (cmd) && cmd == SCIM_TRANS_CMD_REPLY
                    && trans.get_data (&data, len)) {
                } else {
                    std::cerr << "Get ISE language locale is failed!!!\n";
                }
            }
        }

        trans.clear ();
        trans.put_command (SCIM_TRANS_CMD_REPLY);
        if (data != NULL && len > 0) {
            trans.put_command (SCIM_TRANS_CMD_OK);
            trans.put_data (data, len);
        } else {
            trans.put_command (SCIM_TRANS_CMD_FAIL);
        }

        Socket client_socket (client_id);
        trans.write_to_socket (client_socket);

        if (NULL != data)
            delete [] data;
    }

    void get_current_ise_geometry (rectinfo &rect)
    {
        SCIM_DEBUG_MAIN(4) << __func__ << " \n";
        bool           ret  = false;

        if (TOOLBAR_HELPER_MODE == m_current_toolbar_mode || m_current_helper_option & ISM_HELPER_PROCESS_KEYBOARD_KEYEVENT)
            ret = get_helper_geometry (m_current_helper_uuid, rect);

        if (!ret) {
            rect.pos_x  = 0;
            rect.pos_y  = 0;
            rect.width  = 0;
            rect.height = 0;
        }
    }

    void set_ise_mode (int client_id)
    {
        SCIM_DEBUG_MAIN(4) << "PanelAgent::set_ise_mode ()\n";
        uint32 mode;

        if (m_recv_trans.get_data (mode)) {
            if (TOOLBAR_HELPER_MODE == m_current_toolbar_mode || m_current_helper_option & ISM_HELPER_PROCESS_KEYBOARD_KEYEVENT)
                set_helper_mode (m_current_helper_uuid, mode);
        }
    }

    void set_ise_layout (int client_id)
    {
        SCIM_DEBUG_MAIN(4) << "PanelAgent::set_ise_layout ()\n";
        uint32 layout;

        if (m_recv_trans.get_data (layout)) {
            if (TOOLBAR_HELPER_MODE == m_current_toolbar_mode || m_current_helper_option & ISM_HELPER_PROCESS_KEYBOARD_KEYEVENT)
                set_helper_layout (m_current_helper_uuid, layout);
        }
    }

    void set_ise_input_mode (int client_id)
    {
        SCIM_DEBUG_MAIN(4) << "PanelAgent::set_ise_input_mode ()\n";
        uint32 input_mode;

        if (m_recv_trans.get_data (input_mode)) {
            if (TOOLBAR_HELPER_MODE == m_current_toolbar_mode || m_current_helper_option & ISM_HELPER_PROCESS_KEYBOARD_KEYEVENT)
                set_helper_input_mode (m_current_helper_uuid, input_mode);
        }
    }

    void set_ise_input_hint (int client_id)
    {
        SCIM_DEBUG_MAIN(4) << "PanelAgent::set_ise_input_hint ()\n";
        uint32 input_hint;

        if (m_recv_trans.get_data (input_hint)) {
            if (TOOLBAR_HELPER_MODE == m_current_toolbar_mode || m_current_helper_option & ISM_HELPER_PROCESS_KEYBOARD_KEYEVENT)
                set_helper_input_hint (m_current_helper_uuid, input_hint);
        }
    }

    void update_ise_bidi_direction (int client_id)
    {
        SCIM_DEBUG_MAIN(4) << "PanelAgent::update_ise_bidi_direction ()\n";
        uint32 bidi_direction;

        if (m_recv_trans.get_data (bidi_direction)) {
            if (TOOLBAR_HELPER_MODE == m_current_toolbar_mode || m_current_helper_option & ISM_HELPER_PROCESS_KEYBOARD_KEYEVENT)
                set_helper_bidi_direction (m_current_helper_uuid, bidi_direction);
        }
    }

    void set_ise_language (int client_id)
    {
        SCIM_DEBUG_MAIN(4) << "PanelAgent::set_ise_language ()\n";
        uint32 language;

        if (m_recv_trans.get_data (language)) {
            if (TOOLBAR_HELPER_MODE == m_current_toolbar_mode || m_current_helper_option & ISM_HELPER_PROCESS_KEYBOARD_KEYEVENT)
                set_helper_language (m_current_helper_uuid, language);
        }
    }

    void set_ise_imdata (int client_id)
    {
        SCIM_DEBUG_MAIN(4) << "PanelAgent::set_ise_imdata ()\n";
        char   *imdata = NULL;
        size_t  len;

        if (m_recv_trans.get_data (&imdata, len)) {
            if (TOOLBAR_HELPER_MODE == m_current_toolbar_mode || m_current_helper_option & ISM_HELPER_PROCESS_KEYBOARD_KEYEVENT)
                set_helper_imdata (m_current_helper_uuid, imdata, len);
        }

        if (NULL != imdata)
            delete [] imdata;
    }

    void get_ise_imdata (int client_id)
    {
        SCIM_DEBUG_MAIN(4) << "PanelAgent::get_ise_imdata ()\n";
        char   *imdata = NULL;
        size_t  len;
        bool    ret    = false;

        if (TOOLBAR_HELPER_MODE == m_current_toolbar_mode || m_current_helper_option & ISM_HELPER_PROCESS_KEYBOARD_KEYEVENT)
            ret = get_helper_imdata (m_current_helper_uuid, &imdata, len);

        Transaction trans;
        Socket client_socket (client_id);

        trans.clear ();
        trans.put_command (SCIM_TRANS_CMD_REPLY);
        if (ret) {
            trans.put_command (SCIM_TRANS_CMD_OK);
            trans.put_data (imdata, len);
        } else {
            trans.put_command (SCIM_TRANS_CMD_FAIL);
        }

        trans.write_to_socket (client_socket);

        if (NULL != imdata)
            delete [] imdata;
    }

    void get_ise_layout (int client_id)
    {
        SCIM_DEBUG_MAIN(4) << "PanelAgent::get_ise_layout ()\n";
        uint32 layout;
        bool   ret = false;

        if (TOOLBAR_HELPER_MODE == m_current_toolbar_mode || m_current_helper_option & ISM_HELPER_PROCESS_KEYBOARD_KEYEVENT)
            ret = get_helper_layout (m_current_helper_uuid, layout);

        Transaction trans;
        Socket client_socket (client_id);

        trans.clear ();
        trans.put_command (SCIM_TRANS_CMD_REPLY);
        if (ret) {
            trans.put_command (SCIM_TRANS_CMD_OK);
            trans.put_data (layout);
        } else {
            trans.put_command (SCIM_TRANS_CMD_FAIL);
        }

        trans.write_to_socket (client_socket);
    }

    void get_ise_state (int client_id)
    {
        SCIM_DEBUG_MAIN(4) << __func__ << "\n";
        int state = 0;
        m_signal_get_ise_state (state);

        Transaction trans;
        Socket client_socket (client_id);

        trans.clear ();
        trans.put_command (SCIM_TRANS_CMD_REPLY);
        trans.put_command (SCIM_TRANS_CMD_OK);
        trans.put_data (state);
        trans.write_to_socket (client_socket);
    }

    void get_active_ise (int client_id)
    {
        SCIM_DEBUG_MAIN(4) << __func__ << "\n";
        Transaction trans;
        Socket client_socket (client_id);
        String default_uuid = scim_global_config_read (String (SCIM_GLOBAL_CONFIG_DEFAULT_ISE_UUID), String (""));

        trans.clear ();
        trans.put_command (SCIM_TRANS_CMD_REPLY);
        trans.put_command (SCIM_TRANS_CMD_OK);
        trans.put_data (default_uuid);
        trans.write_to_socket (client_socket);
    }

    void get_ise_list (int client_id)
    {
        SCIM_DEBUG_MAIN(4) << "PanelAgent::get_ise_list ()\n";
        std::vector<String> strlist;
        m_signal_get_ise_list (strlist);

        Transaction trans;
        Socket client_socket (client_id);
        char *buf = NULL;
        size_t len;
        uint32 num;

        trans.clear ();
        trans.put_command (SCIM_TRANS_CMD_REPLY);
        trans.put_command (SCIM_TRANS_CMD_OK);

        num = strlist.size ();
        trans.put_data (num);
        for (unsigned int i = 0; i < num; i++) {
            buf = const_cast<char *>(strlist[i].c_str ());
            len = strlen (buf) + 1;
            trans.put_data (buf, len);
        }

        trans.write_to_socket (client_socket);
    }

    void get_ise_information (int client_id)
    {
        SCIM_DEBUG_MAIN(4) << __func__ << "\n";

        String strUuid, strName, strLanguage, strModuleName;
        int nType   = 0;
        int nOption = 0;
        if (m_recv_trans.get_data (strUuid)) {
            m_signal_get_ise_information (strUuid, strName, strLanguage, nType, nOption, strModuleName);
        }

        Transaction trans;
        Socket client_socket (client_id);
        trans.clear ();
        trans.put_command (SCIM_TRANS_CMD_REPLY);
        trans.put_command (SCIM_TRANS_CMD_OK);
        trans.put_data (strName);
        trans.put_data (strLanguage);
        trans.put_data (nType);
        trans.put_data (nOption);
        trans.put_data (strModuleName);
        trans.write_to_socket (client_socket);
    }

    void get_language_list (int client_id)
    {
        SCIM_DEBUG_MAIN(4) << "PanelAgent::get_language_list ()\n";
        std::vector<String> strlist;

        m_signal_get_language_list (strlist);

        Transaction trans;
        Socket client_socket (client_id);
        char *buf = NULL;
        size_t len;
        uint32 num;

        trans.clear ();
        trans.put_command (SCIM_TRANS_CMD_REPLY);
        trans.put_command (SCIM_TRANS_CMD_OK);

        num = strlist.size ();
        trans.put_data (num);
        for (unsigned int i = 0; i < num; i++) {
            buf = const_cast<char *>(strlist[i].c_str ());
            len = strlen (buf) + 1;
            trans.put_data (buf, len);
        }

        trans.write_to_socket (client_socket);
    }

    void get_all_language (int client_id)
    {
        SCIM_DEBUG_MAIN(4) << "PanelAgent::get_all_language ()\n";
        std::vector<String> strlist;

        m_signal_get_all_language (strlist);

        Transaction trans;
        Socket  client_socket (client_id);
        char   *buf = NULL;
        size_t  len;
        uint32  num;

        trans.clear ();
        trans.put_command (SCIM_TRANS_CMD_REPLY);
        trans.put_command (SCIM_TRANS_CMD_OK);

        num = strlist.size ();
        trans.put_data (num);
        for (unsigned int i = 0; i < num; i++) {
            buf = const_cast<char *>(strlist[i].c_str ());
            len = strlen (buf) + 1;
            trans.put_data (buf, len);
        }

        trans.write_to_socket (client_socket);
    }

    void get_ise_language (int client_id)
    {
        SCIM_DEBUG_MAIN(4) << "PanelAgent::get_ise_language ()\n";
        std::vector<String> strlist;
        char   *buf = NULL;
        size_t  len;
        Transaction trans;
        Socket client_socket (client_id);

        if (!(m_recv_trans.get_data (&buf, len))) {
            trans.clear ();
            trans.put_command (SCIM_TRANS_CMD_REPLY);
            trans.put_command (SCIM_TRANS_CMD_FAIL);
            trans.write_to_socket (client_socket);
            if (NULL != buf)
                delete[] buf;
            return;
        }

        m_signal_get_ise_language (buf, strlist);

        if (buf) {
            delete [] buf;
            buf = NULL;
        }

        trans.clear ();
        trans.put_command (SCIM_TRANS_CMD_REPLY);
        trans.put_command (SCIM_TRANS_CMD_OK);

        uint32 num = strlist.size ();
        trans.put_data (num);
        for (unsigned int i = 0; i < num; i++) {
            buf = const_cast<char *>(strlist[i].c_str ());
            len = strlen (buf) + 1;
            trans.put_data (buf, len);
        }

        trans.write_to_socket (client_socket);
    }

    bool reset_ise_option (int client_id)
    {
        SCIM_DEBUG_MAIN(1) << "PanelAgent::resect_ise_option ()\n";

        int    client = -1;
        uint32 context;

        lock ();

        ClientContextUUIDRepository::iterator it = m_client_context_uuids.begin ();
        if (it != m_client_context_uuids.end ()) {
            get_imengine_client_context (it->first, client, context);
        }

        if (client >= 0) {
            Socket client_socket (client);
            m_send_trans.clear ();
            m_send_trans.put_command (SCIM_TRANS_CMD_REPLY);
            m_send_trans.put_data ((uint32) context);
            m_send_trans.put_command (ISM_TRANS_CMD_RESET_ISE_OPTION);
            m_send_trans.write_to_socket (client_socket);
        }

        unlock ();

        Transaction trans;
        trans.clear ();
        trans.put_command (SCIM_TRANS_CMD_REPLY);
        trans.put_command (SCIM_TRANS_CMD_OK);
        Socket client_socket (client_id);
        trans.write_to_socket (client_socket);

        return client >= 0;
    }

    bool find_active_ise_by_uuid (String uuid)
    {
        HelperInfoRepository::iterator iter = m_helper_info_repository.begin ();
        for (; iter != m_helper_info_repository.end (); iter++) {
            if (!uuid.compare (iter->second.uuid))
                return true;
        }

        return false;
    }

    void set_active_ise_by_uuid (int client_id)
    {
        SCIM_DEBUG_MAIN(4) << "PanelAgent::set_active_ise_by_uuid ()\n";
        char   *buf = NULL;
        size_t  len;
        Transaction trans;
        Socket client_socket (client_id);

        trans.clear ();
        trans.put_command (SCIM_TRANS_CMD_REPLY);
        if (!(m_recv_trans.get_data (&buf, len))) {
            trans.put_command (SCIM_TRANS_CMD_FAIL);
            trans.write_to_socket (client_socket);
            if (NULL != buf)
                delete[] buf;
            return;
        }

        if (buf == NULL) {
            trans.put_command (SCIM_TRANS_CMD_FAIL);
            trans.write_to_socket (client_socket);
            return;
        }

        String uuid (buf);
        ISE_INFO info;

        if (!m_signal_get_ise_info_by_uuid (uuid, info)) {
            trans.put_command (SCIM_TRANS_CMD_FAIL);
            trans.write_to_socket (client_socket);
            if (NULL != buf)
                delete[] buf;
            return;
        }

        if (info.type == TOOLBAR_KEYBOARD_MODE) {
            m_signal_set_active_ise_by_uuid (uuid, 1);
            trans.put_command (SCIM_TRANS_CMD_OK);
            trans.write_to_socket (client_socket);
            if (NULL != buf)
                delete[] buf;
            return;
        } else if (info.option & ISM_ISE_HIDE_IN_CONTROL_PANEL) {
            int count = _id_count--;
            if (info.type == TOOLBAR_HELPER_MODE) {
                m_current_toolbar_mode = TOOLBAR_HELPER_MODE;
                if (uuid != m_current_helper_uuid)
                    m_last_helper_uuid = m_current_helper_uuid;
                start_helper (uuid, count, DEFAULT_CONTEXT_VALUE);
                IMControlRepository::iterator iter = m_imcontrol_repository.find (client_id);
                if (iter == m_imcontrol_repository.end ()) {
                    struct IMControlStub stub;
                    stub.count.clear ();
                    stub.info.clear ();
                    stub.info.push_back (info);
                    stub.count.push_back (count);
                    m_imcontrol_repository[client_id] = stub;
                } else {
                    iter->second.info.push_back (info);
                    iter->second.count.push_back (count);
                }
            }
        } else {
            m_signal_set_active_ise_by_uuid (uuid, 1);
        }

        sync();

        if (find_active_ise_by_uuid (uuid)) {
            trans.put_command (SCIM_TRANS_CMD_OK);
            trans.write_to_socket (client_socket);
        } else {
            m_ise_pending_repository[uuid] = client_id;
        }

        if (NULL != buf)
            delete[] buf;
    }

    void set_initial_ise_by_uuid (int client_id)
    {
        SCIM_DEBUG_MAIN(4) << "PanelAgent::set_initial_ise_by_uuid ()\n";
        char   *buf = NULL;
        size_t  len;
        Transaction trans;
        Socket client_socket (client_id);

        trans.clear ();
        trans.put_command (SCIM_TRANS_CMD_REPLY);
        if (!(m_recv_trans.get_data (&buf, len))) {
            trans.put_command (SCIM_TRANS_CMD_FAIL);
            trans.write_to_socket (client_socket);
            if (NULL != buf)
                delete[] buf;
            return;
        }

        if (buf == NULL) {
            trans.put_command (SCIM_TRANS_CMD_FAIL);
            trans.write_to_socket (client_socket);
            return;
        }

        String uuid (buf);

        scim_global_config_write (String (SCIM_GLOBAL_CONFIG_INITIAL_ISE_UUID), String (uuid));
        scim_global_config_flush ();

        trans.put_command (SCIM_TRANS_CMD_OK);
        trans.write_to_socket (client_socket);

        if (NULL != buf)
            delete[] buf;
    }

    void set_ise_return_key_type (int client_id)
    {
        SCIM_DEBUG_MAIN(4) << __func__ << "\n";
        uint32 type;

        if (m_recv_trans.get_data (type)) {
            if (TOOLBAR_HELPER_MODE == m_current_toolbar_mode || m_current_helper_option & ISM_HELPER_PROCESS_KEYBOARD_KEYEVENT)
                set_helper_return_key_type (m_current_helper_uuid, type);
        }
    }

    void get_ise_return_key_type (int client_id)
    {
        SCIM_DEBUG_MAIN(4) << __func__ << "\n";
        uint32 type = 0;
        bool   ret  = false;

        if (TOOLBAR_HELPER_MODE == m_current_toolbar_mode || m_current_helper_option & ISM_HELPER_PROCESS_KEYBOARD_KEYEVENT)
            ret = get_helper_return_key_type (m_current_helper_uuid, type);

        Transaction trans;
        Socket client_socket (client_id);

        trans.clear ();
        trans.put_command (SCIM_TRANS_CMD_REPLY);
        if (ret) {
            trans.put_command (SCIM_TRANS_CMD_OK);
            trans.put_data (type);
        } else {
            trans.put_command (SCIM_TRANS_CMD_FAIL);
        }

        trans.write_to_socket (client_socket);
    }

    void set_ise_return_key_disable (int client_id)
    {
        SCIM_DEBUG_MAIN(4) << __func__ << "\n";
        uint32 disabled;

        if (m_recv_trans.get_data (disabled)) {
            if (TOOLBAR_HELPER_MODE == m_current_toolbar_mode || m_current_helper_option & ISM_HELPER_PROCESS_KEYBOARD_KEYEVENT)
                set_helper_return_key_disable (m_current_helper_uuid, disabled);
        }
    }

    void get_ise_return_key_disable (int client_id)
    {
        SCIM_DEBUG_MAIN(4) << __func__ << "\n";
        uint32 disabled = 0;
        bool   ret      = false;

        if (TOOLBAR_HELPER_MODE == m_current_toolbar_mode || m_current_helper_option & ISM_HELPER_PROCESS_KEYBOARD_KEYEVENT)
            ret = get_helper_return_key_disable (m_current_helper_uuid, disabled);

        Transaction trans;
        Socket client_socket (client_id);

        trans.clear ();
        trans.put_command (SCIM_TRANS_CMD_REPLY);
        if (ret) {
            trans.put_command (SCIM_TRANS_CMD_OK);
            trans.put_data (disabled);
        } else {
            trans.put_command (SCIM_TRANS_CMD_FAIL);
        }

        trans.write_to_socket (client_socket);
    }

    int get_active_ise_list (std::vector<String> &strlist)
    {
        strlist.clear ();
        m_helper_manager.get_active_ise_list (strlist);
        return (int)(strlist.size ());
    }

    int get_helper_manager_id (void)
    {
        return m_helper_manager.get_connection_number ();
    }

    bool has_helper_manager_pending_event (void)
    {
        return m_helper_manager.has_pending_event ();
    }

    bool filter_helper_manager_event (void)
    {
        return m_helper_manager.filter_event ();
    }

    void reset_helper_context (const String &uuid)
    {
        HelperClientIndex::iterator it = m_helper_client_index.find (m_current_helper_uuid);

        if (it != m_helper_client_index.end ()) {
            int client;
            uint32 context;
            Socket client_socket (it->second.id);
            uint32 ctx;

            get_focused_context (client, context);
            ctx = get_helper_ic (client, context);

            m_send_trans.clear ();
            m_send_trans.put_command (SCIM_TRANS_CMD_REPLY);
            m_send_trans.put_data (ctx);
            m_send_trans.put_data (uuid);
            m_send_trans.put_command (ISM_TRANS_CMD_RESET_ISE_CONTEXT);
            m_send_trans.write_to_socket (client_socket);
        }
    }

    void reset_ise_context (int client_id)
    {
        SCIM_DEBUG_MAIN(4) << "PanelAgent::reset_ise_context ()\n";

        if (TOOLBAR_HELPER_MODE == m_current_toolbar_mode || m_current_helper_option & ISM_HELPER_PROCESS_KEYBOARD_KEYEVENT)
            reset_helper_context (m_current_helper_uuid);
    }

    void set_ise_caps_mode (int client_id)
    {
        SCIM_DEBUG_MAIN(4) << "PanelAgent::set_ise_caps_mode ()\n";
        uint32 mode;
        if (m_recv_trans.get_data (mode)) {
            if (TOOLBAR_HELPER_MODE == m_current_toolbar_mode || m_current_helper_option & ISM_HELPER_PROCESS_KEYBOARD_KEYEVENT)
                set_helper_caps_mode (m_current_helper_uuid, mode);
        }
    }

    int send_display_name (String &name)
    {
        return m_helper_manager.send_display_name (name);
    }

    bool reload_config (void)
    {
        SCIM_DEBUG_MAIN(1) << "PanelAgent::reload_config ()\n";

        lock ();

        m_send_trans.clear ();
        m_send_trans.put_command (SCIM_TRANS_CMD_REPLY);
        m_send_trans.put_command (SCIM_TRANS_CMD_RELOAD_CONFIG);

        for (ClientRepository::iterator it = m_client_repository.begin (); it != m_client_repository.end (); ++it) {
            if (it->second.type == FRONTEND_CLIENT || it->second.type == HELPER_CLIENT) {
                Socket client_socket (it->first);
                m_send_trans.write_to_socket (client_socket);
            }
        }

        unlock ();
        return true;
    }

    bool exit (void)
    {
        SCIM_DEBUG_MAIN(1) << "PanelAgent::exit ()\n";

        lock ();

        m_send_trans.clear ();
        m_send_trans.put_command (SCIM_TRANS_CMD_REPLY);
        m_send_trans.put_command (SCIM_TRANS_CMD_EXIT);

        for (ClientRepository::iterator it = m_client_repository.begin (); it != m_client_repository.end (); ++it) {
            Socket client_socket (it->first);
            m_send_trans.write_to_socket (client_socket);
        }

        unlock ();

        stop ();

        return true;
    }

    bool filter_event (int fd)
    {
        SCIM_DEBUG_MAIN (1) << "PanelAgent::filter_event ()\n";

        return m_socket_server.filter_event (fd);
    }

    bool filter_exception_event (int fd)
    {
        SCIM_DEBUG_MAIN (1) << "PanelAgent::filter_exception_event ()\n";

        return m_socket_server.filter_exception_event (fd);
    }

    int get_server_id ()
    {
        SCIM_DEBUG_MAIN (1) << "PanelAgent::get_server_id ()\n";

        return m_socket_server.get_id ();
    }

    void update_ise_list (std::vector<String> &strList)
    {
        /* send ise list to frontend */
        String dst_str = scim_combine_string_list (strList);
        m_helper_manager.send_ise_list (dst_str);

        /* request PanelClient to update keyboard ise list */
        update_keyboard_ise_list ();
    }

    void will_show_ack (int client_id)
    {
        SCIM_DEBUG_MAIN(4) << "PanelAgent::will_show_ack ()\n";

        m_signal_will_show_ack ();
    }

    void will_hide_ack (int client_id)
    {
        SCIM_DEBUG_MAIN(4) << "PanelAgent::will_hide_ack ()\n";

        m_signal_will_hide_ack ();
    }

    void reset_default_ise (int client_id)
    {
        SCIM_DEBUG_MAIN(4) << __func__ << "\n";

        String initial_ise = scim_global_config_read (String (SCIM_GLOBAL_CONFIG_INITIAL_ISE_UUID), String (""));
        if (initial_ise.length () > 0)
            m_signal_set_active_ise_by_uuid (initial_ise.c_str (), 1);
        else
            std::cerr << "Read SCIM_GLOBAL_CONFIG_INITIAL_ISE_UUID is failed!!!\n";
    }

    void set_keyboard_mode (int client_id)
    {
        SCIM_DEBUG_MAIN(4) << "PanelAgent::set_keyboard_mode ()\n";
        uint32 mode;
        if (m_recv_trans.get_data (mode)) {
            m_signal_set_keyboard_mode (mode);
        }
    }

    void candidate_will_hide_ack (int client_id)
    {
        SCIM_DEBUG_MAIN(4) << __FUNCTION__ << "\n";

        m_signal_candidate_will_hide_ack ();
    }

    void process_key_event (int client_id)
    {
        SCIM_DEBUG_MAIN(4) << __func__ << "\n";
        uint32 result = 0;
        bool   ret      = false;

        KeyEvent key;
        if (m_recv_trans.get_data (key)) {
            ret = process_key_event (m_current_helper_uuid, key, result);
        }

        Transaction trans;
        Socket client_socket (client_id);

        trans.clear ();
        trans.put_command (SCIM_TRANS_CMD_REPLY);
        if (ret) {
            trans.put_command (SCIM_TRANS_CMD_OK);
            trans.put_data (result);
        } else {
            trans.put_command (SCIM_TRANS_CMD_FAIL);
        }

        trans.write_to_socket (client_socket);
    }

    void get_active_helper_option (int client_id)
    {
        SCIM_DEBUG_MAIN(4) << __func__ << "\n";
        uint32 option = get_current_helper_option ();

        Transaction trans;
        Socket client_socket (client_id);

        trans.clear ();
        trans.put_command (SCIM_TRANS_CMD_REPLY);
        trans.put_command (SCIM_TRANS_CMD_OK);
        trans.put_data (option);
        SCIM_DEBUG_MAIN(4) << __func__ << " option " << option << "\n";
        trans.write_to_socket (client_socket);
    }

    void show_ise_option_window (int client_id)
    {
        SCIM_DEBUG_MAIN(4) << "PanelAgent::show_ise_panel ()\n";

        String initial_uuid = scim_global_config_read (String (SCIM_GLOBAL_CONFIG_INITIAL_ISE_UUID), String (""));
        String default_uuid = scim_global_config_read (String (SCIM_GLOBAL_CONFIG_DEFAULT_ISE_UUID), String (""));

        ISF_SAVE_LOG ("prepare to show ISE option window %d [%s] [%s]\n", client_id, initial_uuid.c_str(), default_uuid.c_str());

        if (TOOLBAR_HELPER_MODE == m_current_toolbar_mode) {
            show_helper_option_window (m_current_helper_uuid);
        }
    }

    Connection signal_connect_reload_config              (PanelAgentSlotVoid                *slot)
    {
        return m_signal_reload_config.connect (slot);
    }

    Connection signal_connect_turn_on                    (PanelAgentSlotVoid                *slot)
    {
        return m_signal_turn_on.connect (slot);
    }

    Connection signal_connect_turn_off                   (PanelAgentSlotVoid                *slot)
    {
        return m_signal_turn_off.connect (slot);
    }

    Connection signal_connect_show_panel                 (PanelAgentSlotVoid                *slot)
    {
        return m_signal_show_panel.connect (slot);
    }

    Connection signal_connect_hide_panel                 (PanelAgentSlotVoid                *slot)
    {
        return m_signal_hide_panel.connect (slot);
    }

    Connection signal_connect_update_screen              (PanelAgentSlotInt                 *slot)
    {
        return m_signal_update_screen.connect (slot);
    }

    Connection signal_connect_update_spot_location       (PanelAgentSlotIntIntInt           *slot)
    {
        return m_signal_update_spot_location.connect (slot);
    }

    Connection signal_connect_update_factory_info        (PanelAgentSlotFactoryInfo         *slot)
    {
        return m_signal_update_factory_info.connect (slot);
    }

    Connection signal_connect_start_default_ise          (PanelAgentSlotVoid                *slot)
    {
        return m_signal_start_default_ise.connect (slot);
    }

    Connection signal_connect_stop_default_ise           (PanelAgentSlotVoid                *slot)
    {
        return m_signal_stop_default_ise.connect (slot);
    }

    Connection signal_connect_set_candidate_ui           (PanelAgentSlotIntInt              *slot)
    {
        return m_signal_set_candidate_ui.connect (slot);
    }

    Connection signal_connect_get_candidate_ui           (PanelAgentSlotIntInt2             *slot)
    {
        return m_signal_get_candidate_ui.connect (slot);
    }

    Connection signal_connect_set_candidate_position     (PanelAgentSlotIntInt              *slot)
    {
        return m_signal_set_candidate_position.connect (slot);
    }

    Connection signal_connect_get_candidate_geometry     (PanelAgentSlotRect                *slot)
    {
        return m_signal_get_candidate_geometry.connect (slot);
    }

    Connection signal_connect_get_input_panel_geometry   (PanelAgentSlotRect                *slot)
    {
        return m_signal_get_input_panel_geometry.connect (slot);
    }

    Connection signal_connect_set_keyboard_ise           (PanelAgentSlotString              *slot)
    {
        return m_signal_set_keyboard_ise.connect (slot);
    }

    Connection signal_connect_get_keyboard_ise           (PanelAgentSlotString2             *slot)
    {
        return m_signal_get_keyboard_ise.connect (slot);
    }

    Connection signal_connect_show_help                  (PanelAgentSlotString              *slot)
    {
        return m_signal_show_help.connect (slot);
    }

    Connection signal_connect_show_factory_menu          (PanelAgentSlotFactoryInfoVector   *slot)
    {
        return m_signal_show_factory_menu.connect (slot);
    }

    Connection signal_connect_show_preedit_string        (PanelAgentSlotVoid                *slot)
    {
        return m_signal_show_preedit_string.connect (slot);
    }

    Connection signal_connect_show_aux_string            (PanelAgentSlotVoid                *slot)
    {
        return m_signal_show_aux_string.connect (slot);
    }

    Connection signal_connect_show_lookup_table          (PanelAgentSlotVoid                *slot)
    {
        return m_signal_show_lookup_table.connect (slot);
    }

    Connection signal_connect_show_associate_table       (PanelAgentSlotVoid                *slot)
    {
        return m_signal_show_associate_table.connect (slot);
    }

    Connection signal_connect_hide_preedit_string        (PanelAgentSlotVoid                *slot)
    {
        return m_signal_hide_preedit_string.connect (slot);
    }

    Connection signal_connect_hide_aux_string            (PanelAgentSlotVoid                *slot)
    {
        return m_signal_hide_aux_string.connect (slot);
    }

    Connection signal_connect_hide_lookup_table          (PanelAgentSlotVoid                *slot)
    {
        return m_signal_hide_lookup_table.connect (slot);
    }

    Connection signal_connect_hide_associate_table       (PanelAgentSlotVoid                *slot)
    {
        return m_signal_hide_associate_table.connect (slot);
    }

    Connection signal_connect_update_preedit_string      (PanelAgentSlotAttributeStringInt  *slot)
    {
        return m_signal_update_preedit_string.connect (slot);
    }

    Connection signal_connect_update_preedit_caret       (PanelAgentSlotInt                 *slot)
    {
        return m_signal_update_preedit_caret.connect (slot);
    }

    Connection signal_connect_update_aux_string          (PanelAgentSlotAttributeString     *slot)
    {
        return m_signal_update_aux_string.connect (slot);
    }

    Connection signal_connect_update_lookup_table        (PanelAgentSlotLookupTable         *slot)
    {
        return m_signal_update_lookup_table.connect (slot);
    }

    Connection signal_connect_update_associate_table     (PanelAgentSlotLookupTable         *slot)
    {
        return m_signal_update_associate_table.connect (slot);
    }

    Connection signal_connect_register_properties        (PanelAgentSlotPropertyList        *slot)
    {
        return m_signal_register_properties.connect (slot);
    }

    Connection signal_connect_update_property            (PanelAgentSlotProperty            *slot)
    {
        return m_signal_update_property.connect (slot);
    }

    Connection signal_connect_register_helper_properties (PanelAgentSlotIntPropertyList     *slot)
    {
        return m_signal_register_helper_properties.connect (slot);
    }

    Connection signal_connect_update_helper_property     (PanelAgentSlotIntProperty         *slot)
    {
        return m_signal_update_helper_property.connect (slot);
    }

    Connection signal_connect_register_helper            (PanelAgentSlotIntHelperInfo       *slot)
    {
        return m_signal_register_helper.connect (slot);
    }

    Connection signal_connect_remove_helper              (PanelAgentSlotInt                 *slot)
    {
        return m_signal_remove_helper.connect (slot);
    }

    Connection signal_connect_set_active_ise_by_uuid     (PanelAgentSlotStringBool          *slot)
    {
        return m_signal_set_active_ise_by_uuid.connect (slot);
    }

    Connection signal_connect_focus_in                   (PanelAgentSlotVoid                   *slot)
    {
        return m_signal_focus_in.connect (slot);
    }

    Connection signal_connect_focus_out                  (PanelAgentSlotVoid                   *slot)
    {
        return m_signal_focus_out.connect (slot);
    }

    Connection signal_connect_expand_candidate           (PanelAgentSlotVoid                   *slot)
    {
        return m_signal_expand_candidate.connect (slot);
    }

    Connection signal_connect_contract_candidate         (PanelAgentSlotVoid                   *slot)
    {
        return m_signal_contract_candidate.connect (slot);
    }

    Connection signal_connect_select_candidate           (PanelAgentSlotInt                    *slot)
    {
        return m_signal_select_candidate.connect (slot);
    }

    Connection signal_connect_get_ise_list               (PanelAgentSlotBoolStringVector       *slot)
    {
        return m_signal_get_ise_list.connect (slot);
    }

    Connection signal_connect_get_ise_information        (PanelAgentSlotBoolString4int2        *slot)
    {
        return m_signal_get_ise_information.connect (slot);
    }

    Connection signal_connect_get_keyboard_ise_list      (PanelAgentSlotBoolStringVector       *slot)
    {
        return m_signal_get_keyboard_ise_list.connect (slot);
    }

    Connection signal_connect_update_ise_geometry        (PanelAgentSlotIntIntIntInt           *slot)
    {
        return m_signal_update_ise_geometry.connect (slot);
    }

    Connection signal_connect_get_language_list          (PanelAgentSlotStringVector           *slot)
    {
        return m_signal_get_language_list.connect (slot);
    }

    Connection signal_connect_get_all_language           (PanelAgentSlotStringVector           *slot)
    {
        return m_signal_get_all_language.connect (slot);
    }

    Connection signal_connect_get_ise_language           (PanelAgentSlotStrStringVector        *slot)
    {
        return m_signal_get_ise_language.connect (slot);
    }

    Connection signal_connect_get_ise_info_by_uuid       (PanelAgentSlotStringISEINFO          *slot)
    {
        return m_signal_get_ise_info_by_uuid.connect (slot);
    }

    Connection signal_connect_send_key_event             (PanelAgentSlotKeyEvent               *slot)
    {
        return m_signal_send_key_event.connect (slot);
    }

    Connection signal_connect_accept_connection          (PanelAgentSlotInt                    *slot)
    {
        return m_signal_accept_connection.connect (slot);
    }

    Connection signal_connect_close_connection           (PanelAgentSlotInt                    *slot)
    {
        return m_signal_close_connection.connect (slot);
    }

    Connection signal_connect_exit                       (PanelAgentSlotVoid                   *slot)
    {
        return m_signal_exit.connect (slot);
    }

    Connection signal_connect_transaction_start          (PanelAgentSlotVoid                   *slot)
    {
        return m_signal_transaction_start.connect (slot);
    }

    Connection signal_connect_transaction_end            (PanelAgentSlotVoid                   *slot)
    {
        return m_signal_transaction_end.connect (slot);
    }

    Connection signal_connect_lock                       (PanelAgentSlotVoid                   *slot)
    {
        return m_signal_lock.connect (slot);
    }

    Connection signal_connect_unlock                     (PanelAgentSlotVoid                   *slot)
    {
        return m_signal_unlock.connect (slot);
    }

    Connection signal_connect_update_input_context       (PanelAgentSlotIntInt                 *slot)
    {
        return m_signal_update_input_context.connect (slot);
    }

    Connection signal_connect_show_ise                   (PanelAgentSlotVoid                *slot)
    {
        return m_signal_show_ise.connect (slot);
    }

    Connection signal_connect_hide_ise                   (PanelAgentSlotVoid                *slot)
    {
        return m_signal_hide_ise.connect (slot);
    }

    Connection signal_connect_will_show_ack              (PanelAgentSlotVoid                *slot)
    {
        return m_signal_will_show_ack.connect (slot);
    }

    Connection signal_connect_will_hide_ack              (PanelAgentSlotVoid                *slot)
    {
        return m_signal_will_hide_ack.connect (slot);
    }

    Connection signal_connect_set_keyboard_mode (PanelAgentSlotInt                *slot)
    {
        return m_signal_set_keyboard_mode.connect (slot);
    }

    Connection signal_connect_candidate_will_hide_ack    (PanelAgentSlotVoid                *slot)
    {
        return m_signal_candidate_will_hide_ack.connect (slot);
    }

    Connection signal_connect_get_ise_state              (PanelAgentSlotInt2                *slot)
    {
        return m_signal_get_ise_state.connect (slot);
    }

private:
    bool socket_check_client_connection (const Socket &client)
    {
        SCIM_DEBUG_MAIN (3) << "PanelAgent::socket_check_client_connection (" << client.get_id () << ")\n";

        unsigned char buf [sizeof(uint32)];

        int nbytes = client.read_with_timeout (buf, sizeof(uint32), m_socket_timeout);

        if (nbytes == sizeof (uint32))
            return true;

        if (nbytes < 0) {
            SCIM_DEBUG_MAIN (4) << "Error occurred when reading socket: " << client.get_error_message () << ".\n";
        } else {
            SCIM_DEBUG_MAIN (4) << "Timeout when reading socket.\n";
        }

        return false;
    }

    void socket_accept_callback                 (SocketServer   *server,
                                                 const Socket   &client)
    {
        SCIM_DEBUG_MAIN (2) << "PanelAgent::socket_accept_callback (" << client.get_id () << ")\n";

        lock ();
        if (m_should_exit) {
            SCIM_DEBUG_MAIN (3) << "Exit Socket Server Thread.\n";
            server->shutdown ();
        } else {
            m_signal_accept_connection (client.get_id ());
        }
        unlock ();
    }

    void socket_receive_callback                (SocketServer   *server,
                                                 const Socket   &client)
    {
        int     client_id = client.get_id ();
        int     cmd     = 0;
        uint32  key     = 0;
        uint32  context = 0;
        String  uuid;
        bool    current = false;
        bool    last    = false;

        ClientInfo client_info;

        SCIM_DEBUG_MAIN (1) << "PanelAgent::socket_receive_callback (" << client_id << ")\n";

        /* If the connection is closed then close this client. */
        if (!socket_check_client_connection (client)) {
            socket_close_connection (server, client);
            return;
        }

        client_info = socket_get_client_info (client_id);

        /* If it's a new client, then request to open the connection first. */
        if (client_info.type == UNKNOWN_CLIENT) {
            socket_open_connection (server, client);
            return;
        }

        /* If can not read the transaction,
         * or the transaction is not started with SCIM_TRANS_CMD_REQUEST,
         * or the key is mismatch,
         * just return. */
        if (!m_recv_trans.read_from_socket (client, m_socket_timeout) ||
            !m_recv_trans.get_command (cmd) || cmd != SCIM_TRANS_CMD_REQUEST ||
            !m_recv_trans.get_data (key)    || key != (uint32) client_info.key)
            return;

        if (client_info.type == FRONTEND_ACT_CLIENT) {
            if (m_recv_trans.get_data (context)) {
                SCIM_DEBUG_MAIN (1) << "PanelAgent::FrontEnd Client, context = " << context << "\n";
                socket_transaction_start();
                while (m_recv_trans.get_command (cmd)) {
                    SCIM_DEBUG_MAIN (3) << "PanelAgent::cmd = " << cmd << "\n";

                    if (cmd == ISM_TRANS_CMD_REGISTER_PANEL_CLIENT) {
                        uint32 id = 0;
                        if (m_recv_trans.get_data (id)) {
                            SCIM_DEBUG_MAIN(4) << "    ISM_TRANS_CMD_REGISTER_PANEL_CLIENT (" << client_id << "," << context << "," << id << ")\n";
                            m_panel_client_map [client_id] = (int)id;
                        }
                        continue;
                    }

                    if (cmd == SCIM_TRANS_CMD_PANEL_REGISTER_INPUT_CONTEXT) {
                        if (m_recv_trans.get_data (uuid)) {
                            SCIM_DEBUG_MAIN (2) << "PanelAgent::register_input_context (" << client_id << "," << context << "," << uuid << ")\n";
                            uint32 ctx = get_helper_ic (m_panel_client_map[client_id], context);
                            m_client_context_uuids [ctx] = uuid;
                        }
                        continue;
                    }

                    if (cmd == SCIM_TRANS_CMD_PANEL_REMOVE_INPUT_CONTEXT) {
                        SCIM_DEBUG_MAIN(4) << "    SCIM_TRANS_CMD_PANEL_REMOVE_INPUT_CONTEXT (" << "client:" << client_id << " context:" << context << ")\n";
                        uint32 ctx = get_helper_ic (m_panel_client_map[client_id], context);
                        m_client_context_uuids.erase (ctx);
                        if (ctx == get_helper_ic (m_current_socket_client, m_current_client_context)) {
                            lock ();
                            m_current_socket_client  = m_last_socket_client;
                            m_current_client_context = m_last_client_context;
                            m_current_context_uuid   = m_last_context_uuid;
                            m_last_socket_client     = -1;
                            m_last_client_context    = 0;
                            m_last_context_uuid      = String ("");
                            if (m_current_socket_client == -1) {
                                unlock ();
                                socket_update_control_panel ();
                            } else {
                                unlock ();
                            }
                        } else if (ctx == get_helper_ic (m_last_socket_client, m_last_client_context)) {
                            lock ();
                            m_last_socket_client  = -1;
                            m_last_client_context = 0;
                            m_last_context_uuid   = String ("");
                            unlock ();
                        }
                        if (m_client_context_uuids.size () == 0)
                            m_signal_stop_default_ise ();
                        continue;
                    }

                    if (cmd == SCIM_TRANS_CMD_PANEL_RESET_INPUT_CONTEXT) {
                        socket_reset_input_context (m_panel_client_map[client_id], context);
                        continue;
                    }

                    if (cmd == SCIM_TRANS_CMD_FOCUS_IN) {
                        SCIM_DEBUG_MAIN(4) << "    SCIM_TRANS_CMD_FOCUS_IN (" << "client:" << client_id << " context:" << context << ")\n";
                        m_signal_focus_in ();
                        focus_in_helper (m_current_helper_uuid, m_panel_client_map[client_id], context);
                        if (m_recv_trans.get_data (uuid)) {
                            SCIM_DEBUG_MAIN (2) << "PanelAgent::focus_in (" << client_id << "," << context << "," << uuid << ")\n";
                            m_active_client_id = client_id;
                            lock ();
                            if (m_current_socket_client >= 0) {
                                m_last_socket_client  = m_current_socket_client;
                                m_last_client_context = m_current_client_context;
                                m_last_context_uuid   = m_current_context_uuid;
                            }
                            m_current_socket_client  = m_panel_client_map[client_id];
                            m_current_client_context = context;
                            m_current_context_uuid   = uuid;
                            unlock ();
                        }
                        continue;
                    }

                    if (cmd == ISM_TRANS_CMD_TURN_ON_LOG) {
                        socket_turn_on_log ();
                        continue;
                    }

                    if (cmd == ISM_TRANS_CMD_SHOW_ISF_CONTROL) {
                        show_isf_panel (client_id);
                        continue;
                    } else if (cmd == ISM_TRANS_CMD_HIDE_ISF_CONTROL) {
                        hide_isf_panel (client_id);
                        continue;
                    } else if (cmd == ISM_TRANS_CMD_SHOW_ISE_PANEL) {
                        show_ise_panel (client_id);
                        continue;
                    } else if (cmd == ISM_TRANS_CMD_HIDE_ISE_PANEL) {
                        hide_ise_panel (client_id);
                        continue;
                    } else if (cmd == ISM_TRANS_CMD_GET_ACTIVE_ISE_GEOMETRY) {
                        get_input_panel_geometry (client_id);
                        continue;
                    } else if (cmd == ISM_TRANS_CMD_GET_CANDIDATE_GEOMETRY) {
                        get_candidate_window_geometry (client_id);
                        continue;
                    } else if (cmd == ISM_TRANS_CMD_GET_ISE_LANGUAGE_LOCALE) {
                        get_ise_language_locale (client_id);
                        continue;
                    } else if (cmd == ISM_TRANS_CMD_SET_ISE_LANGUAGE) {
                        set_ise_language (client_id);
                        continue;
                    } else if (cmd == ISM_TRANS_CMD_SET_ISE_IMDATA) {
                        set_ise_imdata (client_id);
                        continue;
                    } else if (cmd == ISM_TRANS_CMD_GET_ISE_IMDATA) {
                        get_ise_imdata (client_id);
                        continue;
                    } else if (cmd == ISM_TRANS_CMD_SET_RETURN_KEY_TYPE) {
                        set_ise_return_key_type (client_id);
                        continue;
                    } else if (cmd == ISM_TRANS_CMD_GET_RETURN_KEY_TYPE) {
                        get_ise_return_key_type (client_id);
                        continue;
                    } else if (cmd == ISM_TRANS_CMD_SET_RETURN_KEY_DISABLE) {
                        set_ise_return_key_disable (client_id);
                        continue;
                    } else if (cmd == ISM_TRANS_CMD_GET_RETURN_KEY_DISABLE) {
                        get_ise_return_key_disable (client_id);
                        continue;
                    } else if (cmd == ISM_TRANS_CMD_GET_LAYOUT) {
                        get_ise_layout (client_id);
                        continue;
                    } else if (cmd == ISM_TRANS_CMD_SET_LAYOUT) {
                        set_ise_layout (client_id);
                        continue;
                    } else if (cmd == ISM_TRANS_CMD_SET_CAPS_MODE) {
                        set_ise_caps_mode (client_id);
                        continue;
                    } else if (cmd == ISM_TRANS_CMD_SEND_WILL_SHOW_ACK) {
                        will_show_ack (client_id);
                        continue;
                    } else if (cmd == ISM_TRANS_CMD_SEND_WILL_HIDE_ACK) {
                        will_hide_ack (client_id);
                        continue;
                    } else if (cmd == ISM_TRANS_CMD_SET_HARDWARE_KEYBOARD_MODE) {
                        set_keyboard_mode (client_id);
                        continue;
                    } else if (cmd == ISM_TRANS_CMD_SEND_CANDIDATE_WILL_HIDE_ACK) {
                        candidate_will_hide_ack (client_id);
                        continue;
                    } else if (cmd == SCIM_TRANS_CMD_PROCESS_KEY_EVENT) {
                        process_key_event (client_id);
                        continue;
                    } else if (cmd == ISM_TRANS_CMD_GET_ACTIVE_HELPER_OPTION) {
                        get_active_helper_option (client_id);
                        continue;
                    } else if (cmd == ISM_TRANS_CMD_GET_ISE_STATE) {
                        get_ise_state (client_id);
                        continue;
                    } else if (cmd == ISM_TRANS_CMD_SET_INPUT_MODE) {
                        set_ise_input_mode (client_id);
                        continue;
                    } else if (cmd == ISM_TRANS_CMD_SET_INPUT_HINT) {
                        set_ise_input_hint (client_id);
                        continue;
                    } else if (cmd == ISM_TRANS_CMD_UPDATE_BIDI_DIRECTION) {
                        update_ise_bidi_direction (client_id);
                        continue;
                    } else if (cmd == ISM_TRANS_CMD_SHOW_ISE_OPTION_WINDOW) {
                        show_ise_option_window (client_id);
                        continue;
                    }

                    current = last = false;
                    uuid.clear ();

                    /* Get the context uuid from the client context registration table. */
                    {
                        ClientContextUUIDRepository::iterator it = m_client_context_uuids.find (get_helper_ic (m_panel_client_map[client_id], context));
                        if (it != m_client_context_uuids.end ())
                            uuid = it->second;
                    }

                    if (m_current_socket_client == m_panel_client_map[client_id] && m_current_client_context == context) {
                        current = true;
                        if (!uuid.length ()) uuid = m_current_context_uuid;
                    } else if (m_last_socket_client == m_panel_client_map[client_id] && m_last_client_context == context) {
                        last = true;
                        if (!uuid.length ()) uuid = m_last_context_uuid;
                    }

                    /* Skip to the next command and continue, if it's not current or last focused. */
                    if (!uuid.length ()) {
                        SCIM_DEBUG_MAIN (3) << "PanelAgent:: Couldn't find context uuid.\n";
                        while (m_recv_trans.get_data_type () != SCIM_TRANS_DATA_COMMAND && m_recv_trans.get_data_type () != SCIM_TRANS_DATA_UNKNOWN)
                            m_recv_trans.skip_data ();
                        continue;
                    }

                    if (cmd == SCIM_TRANS_CMD_START_HELPER) {
                        socket_start_helper (m_panel_client_map[client_id], context, uuid);
                        continue;
                    } else if (cmd == SCIM_TRANS_CMD_SEND_HELPER_EVENT) {
                        socket_send_helper_event (m_panel_client_map[client_id], context, uuid);
                        continue;
                    } else if (cmd == SCIM_TRANS_CMD_STOP_HELPER) {
                        socket_stop_helper (m_panel_client_map[client_id], context, uuid);
                        continue;
                    }

                    /* If it's not focused, just continue. */
                    if ((!current && !last) || (last && m_current_socket_client >= 0)) {
                        SCIM_DEBUG_MAIN (3) << "PanelAgent::Not current focused.\n";
                        while (m_recv_trans.get_data_type () != SCIM_TRANS_DATA_COMMAND && m_recv_trans.get_data_type () != SCIM_TRANS_DATA_UNKNOWN)
                            m_recv_trans.skip_data ();
                        continue;
                    }

                    /* Client must focus in before do any other things. */
                    if (cmd == SCIM_TRANS_CMD_PANEL_TURN_ON)
                        socket_turn_on ();
                    else if (cmd == SCIM_TRANS_CMD_PANEL_TURN_OFF)
                        socket_turn_off ();
                    else if (cmd == SCIM_TRANS_CMD_UPDATE_SCREEN)
                        socket_update_screen ();
                    else if (cmd == SCIM_TRANS_CMD_UPDATE_SPOT_LOCATION)
                        socket_update_spot_location ();
                    else if (cmd == ISM_TRANS_CMD_UPDATE_CURSOR_POSITION)
                        socket_update_cursor_position ();
                    else if (cmd == ISM_TRANS_CMD_UPDATE_SURROUNDING_TEXT)
                        socket_update_surrounding_text ();
                    else if (cmd == ISM_TRANS_CMD_UPDATE_SELECTION)
                        socket_update_selection ();
                    else if (cmd == ISM_TRANS_CMD_EXPAND_CANDIDATE)
                        m_signal_expand_candidate ();
                    else if (cmd == ISM_TRANS_CMD_CONTRACT_CANDIDATE)
                        m_signal_contract_candidate ();
                    else if (cmd == ISM_TRANS_CMD_SET_CANDIDATE_UI)
                        socket_set_candidate_ui ();
                    else if (cmd == SCIM_TRANS_CMD_PANEL_UPDATE_FACTORY_INFO)
                        socket_update_factory_info ();
                    else if (cmd == SCIM_TRANS_CMD_SHOW_PREEDIT_STRING)
                        socket_show_preedit_string ();
                    else if (cmd == SCIM_TRANS_CMD_SHOW_AUX_STRING)
                        socket_show_aux_string ();
                    else if (cmd == SCIM_TRANS_CMD_SHOW_LOOKUP_TABLE)
                        socket_show_lookup_table ();
                    else if (cmd == ISM_TRANS_CMD_SHOW_ASSOCIATE_TABLE)
                        socket_show_associate_table ();
                    else if (cmd == SCIM_TRANS_CMD_HIDE_PREEDIT_STRING)
                        socket_hide_preedit_string ();
                    else if (cmd == SCIM_TRANS_CMD_HIDE_AUX_STRING)
                        socket_hide_aux_string ();
                    else if (cmd == SCIM_TRANS_CMD_HIDE_LOOKUP_TABLE)
                        socket_hide_lookup_table ();
                    else if (cmd == ISM_TRANS_CMD_HIDE_ASSOCIATE_TABLE)
                        socket_hide_associate_table ();
                    else if (cmd == SCIM_TRANS_CMD_UPDATE_PREEDIT_STRING)
                        socket_update_preedit_string ();
                    else if (cmd == SCIM_TRANS_CMD_UPDATE_PREEDIT_CARET)
                        socket_update_preedit_caret ();
                    else if (cmd == SCIM_TRANS_CMD_UPDATE_AUX_STRING) {
                        socket_update_aux_string ();
                        m_is_imengine_aux = true;
                    } else if (cmd == SCIM_TRANS_CMD_UPDATE_LOOKUP_TABLE) {
                        socket_update_lookup_table ();
                        m_is_imengine_candidate = true;
                    } else if (cmd == ISM_TRANS_CMD_UPDATE_ASSOCIATE_TABLE)
                        socket_update_associate_table ();
                    else if (cmd == SCIM_TRANS_CMD_REGISTER_PROPERTIES)
                        socket_register_properties ();
                    else if (cmd == SCIM_TRANS_CMD_UPDATE_PROPERTY)
                        socket_update_property ();
                    else if (cmd == SCIM_TRANS_CMD_PANEL_SHOW_HELP)
                        socket_show_help ();
                    else if (cmd == SCIM_TRANS_CMD_PANEL_SHOW_FACTORY_MENU)
                        socket_show_factory_menu ();
                    else if (cmd == SCIM_TRANS_CMD_FOCUS_OUT) {
                        m_signal_focus_out ();
                        lock ();
                        focus_out_helper (m_current_helper_uuid, m_panel_client_map[client_id], context);
                        if (m_current_socket_client >= 0) {
                            m_last_socket_client  = m_current_socket_client;
                            m_last_client_context = m_current_client_context;
                            m_last_context_uuid   = m_current_context_uuid;
                        }
                        m_current_socket_client  = -1;
                        m_current_client_context = 0;
                        m_current_context_uuid   = String ("");
                        unlock ();
                    }
                }
                socket_transaction_end ();
            }
        } else if (client_info.type == FRONTEND_CLIENT) {
            if (m_recv_trans.get_data (context)) {
                SCIM_DEBUG_MAIN (1) << "client_info.type == FRONTEND_CLIENT\n";
                socket_transaction_start();
                while (m_recv_trans.get_command (cmd)) {
                    SCIM_DEBUG_MAIN (3) << "PanelAgent::cmd = " << cmd << "\n";

                    if (cmd == ISM_TRANS_CMD_GET_PANEL_CLIENT_ID) {
                        Socket client_socket (client_id);

                        Transaction trans;
                        trans.clear ();
                        trans.put_command (SCIM_TRANS_CMD_REPLY);
                        trans.put_command (SCIM_TRANS_CMD_OK);
                        trans.put_data (client_id);
                        trans.write_to_socket (client_socket);
                        continue;
                    }
                }
                socket_transaction_end ();
            }
        } else if (client_info.type == HELPER_CLIENT) {
            socket_transaction_start ();
            while (m_recv_trans.get_command (cmd)) {
                if (cmd == SCIM_TRANS_CMD_PANEL_REGISTER_HELPER) {
                    socket_helper_register_helper (client_id);
                }
            }
            socket_transaction_end ();
        } else if (client_info.type == HELPER_ACT_CLIENT) {
            socket_transaction_start ();
            while (m_recv_trans.get_command (cmd)) {
                if (cmd == SCIM_TRANS_CMD_PANEL_REGISTER_ACTIVE_HELPER) {
                    socket_helper_register_helper_passive (client_id);
                } else if (cmd == SCIM_TRANS_CMD_COMMIT_STRING) {
                    socket_helper_commit_string (client_id);
                } else if (cmd == SCIM_TRANS_CMD_SHOW_PREEDIT_STRING) {
                    socket_helper_show_preedit_string (client_id);
                } else if (cmd == SCIM_TRANS_CMD_SHOW_AUX_STRING) {
                    socket_show_aux_string ();
                } else if (cmd == SCIM_TRANS_CMD_SHOW_LOOKUP_TABLE) {
                    socket_show_lookup_table ();
                } else if (cmd == ISM_TRANS_CMD_SHOW_ASSOCIATE_TABLE) {
                    socket_show_associate_table ();
                } else if (cmd == SCIM_TRANS_CMD_HIDE_PREEDIT_STRING) {
                    socket_helper_hide_preedit_string (client_id);
                } else if (cmd == SCIM_TRANS_CMD_HIDE_AUX_STRING) {
                    socket_hide_aux_string ();
                } else if (cmd == SCIM_TRANS_CMD_HIDE_LOOKUP_TABLE) {
                    socket_hide_lookup_table ();
                } else if (cmd == ISM_TRANS_CMD_HIDE_ASSOCIATE_TABLE) {
                    socket_hide_associate_table ();
                } else if (cmd == SCIM_TRANS_CMD_UPDATE_PREEDIT_STRING) {
                    socket_helper_update_preedit_string (client_id);
                } else if (cmd == SCIM_TRANS_CMD_UPDATE_PREEDIT_CARET) {
                    socket_helper_update_preedit_caret (client_id);
                } else if (cmd == SCIM_TRANS_CMD_UPDATE_AUX_STRING) {
                    socket_update_aux_string ();
                    m_is_imengine_aux = false;
                } else if (cmd == SCIM_TRANS_CMD_UPDATE_LOOKUP_TABLE) {
                    socket_update_lookup_table ();
                    m_is_imengine_candidate = false;
                } else if (cmd == ISM_TRANS_CMD_UPDATE_ASSOCIATE_TABLE) {
                    socket_update_associate_table ();
                } else if (cmd == SCIM_TRANS_CMD_PROCESS_KEY_EVENT ||
                           cmd == SCIM_TRANS_CMD_PANEL_SEND_KEY_EVENT) {
                    socket_helper_send_key_event (client_id);
                } else if (cmd == SCIM_TRANS_CMD_FORWARD_KEY_EVENT) {
                    socket_helper_forward_key_event (client_id);
                } else if (cmd == SCIM_TRANS_CMD_PANEL_SEND_IMENGINE_EVENT) {
                    socket_helper_send_imengine_event (client_id);
                } else if (cmd == SCIM_TRANS_CMD_REGISTER_PROPERTIES) {
                    socket_helper_register_properties (client_id);

                    /* Check whether application is already focus_in */
                    if (m_current_socket_client != -1) {
                        SCIM_DEBUG_MAIN (2) << "Application is already focus_in!\n";
                        focus_in_helper (m_current_helper_uuid, m_current_socket_client, m_current_client_context);
                        reset_keyboard_ise ();
                    }
                    /* Check whether ISE panel is already shown */
                    if (TOOLBAR_HELPER_MODE == m_current_toolbar_mode && m_ise_context_length > 0) {
                        SCIM_DEBUG_MAIN (2) << "Re-show ISE panel!\n";
                        int    focused_client;
                        uint32 focused_context;
                        get_focused_context (focused_client, focused_context);
                        if (focused_client == -1 && m_active_client_id != -1) {
                            focused_client  = m_panel_client_map[m_active_client_id];
                            focused_context = 0;
                        }

                        uint32 ctx = get_helper_ic (focused_client, focused_context);
                        bool ret = show_helper (m_current_helper_uuid, m_ise_context_buffer, m_ise_context_length, ctx);
                        if (ret)
                            m_signal_show_ise ();
                    }
                } else if (cmd == SCIM_TRANS_CMD_UPDATE_PROPERTY) {
                    socket_helper_update_property (client_id);
                } else if (cmd == SCIM_TRANS_CMD_RELOAD_CONFIG) {
                    reload_config ();
                    m_signal_reload_config ();
                } else if (cmd == ISM_TRANS_CMD_UPDATE_ISE_INPUT_CONTEXT) {
                    socket_helper_update_input_context (client_id);
                } else if (cmd == ISM_TRANS_CMD_GET_KEYBOARD_ISE_LIST) {
                    socket_get_keyboard_ise_list ();
                } else if (cmd == ISM_TRANS_CMD_SET_CANDIDATE_UI) {
                    socket_set_candidate_ui ();
                } else if (cmd == ISM_TRANS_CMD_GET_CANDIDATE_UI) {
                    socket_get_candidate_ui ();
                } else if (cmd == ISM_TRANS_CMD_SET_CANDIDATE_POSITION) {
                    socket_set_candidate_position ();
                } else if (cmd == ISM_TRANS_CMD_HIDE_CANDIDATE) {
                    socket_hide_candidate ();
                } else if (cmd == ISM_TRANS_CMD_GET_CANDIDATE_GEOMETRY) {
                    socket_get_candidate_geometry ();
                } else if (cmd == ISM_TRANS_CMD_PANEL_RESET_KEYBOARD_ISE) {
                    reset_keyboard_ise ();
                } else if (cmd == ISM_TRANS_CMD_SET_KEYBOARD_ISE_BY_UUID) {
                    socket_set_keyboard_ise ();
                } else if (cmd == ISM_TRANS_CMD_GET_KEYBOARD_ISE) {
                    socket_get_keyboard_ise ();
                } else if (cmd == ISM_TRANS_CMD_UPDATE_ISE_GEOMETRY) {
                    socket_helper_update_ise_geometry (client_id);
                } else if (cmd == ISM_TRANS_CMD_EXPAND_CANDIDATE) {
                    socket_helper_expand_candidate (client_id);
                } else if (cmd == ISM_TRANS_CMD_CONTRACT_CANDIDATE) {
                    socket_helper_contract_candidate (client_id);
                } else if (cmd == ISM_TRANS_CMD_SELECT_CANDIDATE) {
                    socket_helper_select_candidate ();
                } else if (cmd == SCIM_TRANS_CMD_GET_SURROUNDING_TEXT) {
                    socket_helper_get_surrounding_text (client_id);
                } else if (cmd == SCIM_TRANS_CMD_DELETE_SURROUNDING_TEXT) {
                    socket_helper_delete_surrounding_text (client_id);
                } else if (cmd == SCIM_TRANS_CMD_GET_SELECTION) {
                    socket_helper_get_selection (client_id);
                } else if (cmd == SCIM_TRANS_CMD_SET_SELECTION) {
                    socket_helper_set_selection (client_id);
                } else if (cmd == SCIM_TRANS_CMD_SEND_PRIVATE_COMMAND) {
                    socket_helper_send_private_command (client_id);
                } else if (cmd == ISM_TRANS_CMD_UPDATE_ISE_EXIT) {
                    HelperInfoRepository::iterator hiit = m_helper_active_info_repository.find (client.get_id ());
                    if (hiit != m_helper_active_info_repository.end ()) {
                        String l_uuid = hiit->second.uuid;
                        HelperClientIndex::iterator it = m_helper_client_index.find (l_uuid);
                        if (it != m_helper_client_index.end ()) {
                            Socket client_socket (it->second.id);
                            socket_close_connection (server, client_socket);
                        }
                    }
                    socket_close_connection (server, client);
                }
            }
            socket_transaction_end ();
        } else if (client_info.type == IMCONTROL_ACT_CLIENT) {
            socket_transaction_start ();

            while (m_recv_trans.get_command (cmd)) {
                if (cmd == ISM_TRANS_CMD_GET_ACTIVE_ISE)
                    get_active_ise (client_id);
                else if (cmd == ISM_TRANS_CMD_SET_ACTIVE_ISE_BY_UUID) {
                    ISF_SAVE_LOG ("checking sockfd privilege...\n");
                    int ret = security_server_check_privilege_by_sockfd (client_id, "isf::manager", "w");
                    if (ret == SECURITY_SERVER_API_ERROR_ACCESS_DENIED) {
                        SCIM_DEBUG_MAIN (2) <<"Security server api error. Access denied\n";
                    } else {
                        SCIM_DEBUG_MAIN (2) <<"Security server api success\n";
                    }
                    ISF_SAVE_LOG ("setting active ise\n");
                    set_active_ise_by_uuid (client_id);
                }
                else if (cmd == ISM_TRANS_CMD_SET_INITIAL_ISE_BY_UUID) {
                    ISF_SAVE_LOG ("checking sockfd privilege...\n");
                    int ret = security_server_check_privilege_by_sockfd (client_id, "isf::manager", "w");
                    if (ret == SECURITY_SERVER_API_ERROR_ACCESS_DENIED) {
                        SCIM_DEBUG_MAIN (2) <<"Security server api error. Access denied\n";
                    } else {
                        SCIM_DEBUG_MAIN (2) <<"Security server api success\n";
                    }
                    ISF_SAVE_LOG ("setting initial ise\n");
                    set_initial_ise_by_uuid (client_id);
                }
                else if (cmd == ISM_TRANS_CMD_GET_ISE_LIST)
                    get_ise_list (client_id);
                else if (cmd == ISM_TRANS_CMD_GET_ISE_INFORMATION)
                    get_ise_information (client_id);
                else if (cmd == ISM_TRANS_CMD_RESET_ISE_OPTION)
                    reset_ise_option (client_id);
                else if (cmd == ISM_TRANS_CMD_RESET_DEFAULT_ISE)
                    reset_default_ise (client_id);
                else if (cmd == ISM_TRANS_CMD_SHOW_ISF_CONTROL)
                    show_isf_panel (client_id);
                else if (cmd == ISM_TRANS_CMD_SHOW_ISE_OPTION_WINDOW)
                    show_ise_option_window (client_id);
            }

            socket_transaction_end ();
        }
    }

    void socket_exception_callback              (SocketServer   *server,
                                                 const Socket   &client)
    {
        SCIM_DEBUG_MAIN (2) << "PanelAgent::socket_exception_callback (" << client.get_id () << ")\n";

        socket_close_connection (server, client);
    }

    bool socket_open_connection                 (SocketServer   *server,
                                                 const Socket   &client)
    {
        SCIM_DEBUG_MAIN (3) << "PanelAgent::socket_open_connection (" << client.get_id () << ")\n";

        uint32 key;
        String type = scim_socket_accept_connection (key,
                                                     String ("Panel"),
                                                     String ("FrontEnd,FrontEnd_Active,Helper,Helper_Active,IMControl_Active,IMControl_Passive"),
                                                     client,
                                                     m_socket_timeout);

        if (type.length ()) {
            ClientInfo info;
            info.key = key;
            info.type = ((type == "FrontEnd") ? FRONTEND_CLIENT :
                        ((type == "FrontEnd_Active") ? FRONTEND_ACT_CLIENT :
                        ((type == "IMControl_Active") ? IMCONTROL_ACT_CLIENT :
                        ((type == "Helper_Active") ? HELPER_ACT_CLIENT :
                        ((type == "IMControl_Passive") ? IMCONTROL_CLIENT : HELPER_CLIENT)))));

            SCIM_DEBUG_MAIN (4) << "Add client to repository. Type=" << type << " key=" << key << "\n";
            lock ();
            m_client_repository [client.get_id ()] = info;

            if (info.type == IMCONTROL_ACT_CLIENT) {
                m_pending_active_imcontrol_id = client.get_id ();
            } else if (info.type == IMCONTROL_CLIENT) {
                m_imcontrol_map [m_pending_active_imcontrol_id] = client.get_id ();
                m_pending_active_imcontrol_id = -1;
            }

            unlock ();
            return true;
        }

        SCIM_DEBUG_MAIN (4) << "Close client connection " << client.get_id () << "\n";
        server->close_connection (client);
        return false;
    }

    void socket_close_connection                (SocketServer   *server,
                                                 const Socket   &client)
    {
        SCIM_DEBUG_MAIN (3) << "PanelAgent::socket_close_connection (" << client.get_id () << ")\n";

        lock ();

        m_signal_close_connection (client.get_id ());

        ClientInfo client_info = socket_get_client_info (client.get_id ());

        m_client_repository.erase (client.get_id ());

        server->close_connection (client);

        /* Exit panel if there is no connected client anymore. */
        if (client_info.type != UNKNOWN_CLIENT && m_client_repository.size () == 0 && !m_should_resident) {
            SCIM_DEBUG_MAIN (4) << "Exit Socket Server Thread.\n";
            server->shutdown ();
            m_signal_exit.emit ();
        }

        unlock ();

        if (client_info.type == FRONTEND_CLIENT) {
            SCIM_DEBUG_MAIN(4) << "It's a FrontEnd client.\n";
            /* The focused client is closed. */
            if (m_current_socket_client == client.get_id ()) {
                if (TOOLBAR_HELPER_MODE == m_current_toolbar_mode || m_current_helper_option & ISM_HELPER_PROCESS_KEYBOARD_KEYEVENT)
                    hide_helper (m_current_helper_uuid);

                lock ();
                m_current_socket_client = -1;
                m_current_client_context = 0;
                m_current_context_uuid = String ("");
                unlock ();

                socket_transaction_start ();
                socket_turn_off ();
                socket_transaction_end ();
            }

            if (m_last_socket_client == client.get_id ()) {
                lock ();
                m_last_socket_client = -1;
                m_last_client_context = 0;
                m_last_context_uuid = String ("");
                unlock ();
            }

            /* Erase all associated Client Context UUIDs. */
            std::vector <uint32> ctx_list;
            ClientContextUUIDRepository::iterator it = m_client_context_uuids.begin ();
            for (; it != m_client_context_uuids.end (); ++it) {
                if ((it->first & 0xFFFF) == (client.get_id () & 0xFFFF))
                    ctx_list.push_back (it->first);
            }

            for (size_t i = 0; i < ctx_list.size (); ++i)
                m_client_context_uuids.erase (ctx_list [i]);

            int client_id = client.get_id ();

            /* Erase all helperise info associated with the client */
            ctx_list.clear ();
            it = m_client_context_helper.begin ();
            for (; it != m_client_context_helper.end (); ++it) {
                if ((it->first & 0xFFFF) == (client_id & 0xFFFF)) {
                    ctx_list.push_back (it->first);

                    /* similar to stop_helper except that it will not call get_focused_context() */
                    String uuid = it->second;
                    if (m_helper_uuid_count.find (uuid) != m_helper_uuid_count.end ()) {
                        uint32 count = m_helper_uuid_count[uuid];
                        if (1 == count) {
                            m_helper_uuid_count.erase (uuid);

                            HelperClientIndex::iterator pise = m_helper_client_index.find (uuid);
                            if (pise != m_helper_client_index.end ()) {
                                m_send_trans.clear ();
                                m_send_trans.put_command (SCIM_TRANS_CMD_REPLY);
                                m_send_trans.put_data (it->first);
                                m_send_trans.put_data (uuid);
                                m_send_trans.put_command (SCIM_TRANS_CMD_EXIT);
                                m_send_trans.write_to_socket (pise->second.id);
                            }
                            SCIM_DEBUG_MAIN(1) << "Stop HelperISE " << uuid << " ...\n";
                        } else {
                            m_helper_uuid_count[uuid] = count - 1;
                            focus_out_helper (uuid, (it->first & 0xFFFF), ((it->first >> 16) & 0x7FFF));
                            SCIM_DEBUG_MAIN(1) << "Decrement usage count of HelperISE " << uuid
                                    << " to " << m_helper_uuid_count[uuid] << "\n";
                        }
                    }
                }
            }

            for (size_t i = 0; i < ctx_list.size (); ++i)
                 m_client_context_helper.erase (ctx_list [i]);

            HelperInfoRepository::iterator iter = m_helper_info_repository.begin ();
            for (; iter != m_helper_info_repository.end (); iter++) {
                if (!m_current_helper_uuid.compare (iter->second.uuid))
                    if (!(iter->second.option & ISM_ISE_HIDE_IN_CONTROL_PANEL))
                        socket_update_control_panel ();
            }
        } else if (client_info.type == FRONTEND_ACT_CLIENT) {
            SCIM_DEBUG_MAIN(4) << "It's a FRONTEND_ACT_CLIENT client.\n";
            IntIntRepository::iterator iter2 = m_panel_client_map.find (client.get_id ());
            if (iter2 != m_panel_client_map.end ())
                m_panel_client_map.erase (iter2);
        } else if (client_info.type == HELPER_CLIENT) {
            SCIM_DEBUG_MAIN(4) << "It's a Helper client.\n";

            lock ();

            HelperInfoRepository::iterator hiit = m_helper_info_repository.find (client.get_id ());

            if (hiit != m_helper_info_repository.end ()) {
                bool restart = false;
                String uuid = hiit->second.uuid;

                HelperClientIndex::iterator it = m_helper_client_index.find (uuid);
                if ((hiit->second.option & SCIM_HELPER_AUTO_RESTART) &&
                    (it != m_helper_client_index.end () && it->second.ref > 0))
                    restart = true;

                m_helper_manager.stop_helper (hiit->second.name);

                m_helper_client_index.erase (uuid);
                m_helper_info_repository.erase (hiit);

                if (restart && !m_ise_exiting) {
                    m_helper_manager.run_helper (uuid, m_config_name, m_display_name);
                    std::cerr << "Auto restart soft ISE:" << uuid << "\n";
                }
            }

            m_ise_exiting = false;
            unlock ();

            socket_transaction_start ();
            m_signal_remove_helper (client.get_id ());
            socket_transaction_end ();
        } else if (client_info.type == HELPER_ACT_CLIENT) {
            SCIM_DEBUG_MAIN(4) << "It's a Helper passive client.\n";

            lock ();

            HelperInfoRepository::iterator hiit = m_helper_active_info_repository.find (client.get_id ());
            if (hiit != m_helper_active_info_repository.end ())
                m_helper_active_info_repository.erase (hiit);

            unlock ();
        } else if (client_info.type == IMCONTROL_ACT_CLIENT) {
            SCIM_DEBUG_MAIN(4) << "It's a IMCONTROL_ACT_CLIENT client.\n";
            int client_id = client.get_id ();

            IMControlRepository::iterator iter = m_imcontrol_repository.find (client_id);
            if (iter != m_imcontrol_repository.end ()) {
                int size = iter->second.info.size ();
                int i = 0;
                while (i < size) {
                    stop_helper ((iter->second.info)[i].uuid, (iter->second.count)[i], DEFAULT_CONTEXT_VALUE);
                    if ((iter->second.info)[i].option & ISM_ISE_HIDE_IN_CONTROL_PANEL)
                        m_current_helper_uuid = m_last_helper_uuid;
                    i++;
                }
                m_imcontrol_repository.erase (iter);
            }

            IntIntRepository::iterator iter2 = m_imcontrol_map.find (client_id);
            if (iter2 != m_imcontrol_map.end ())
                m_imcontrol_map.erase (iter2);
        }
    }

    const ClientInfo & socket_get_client_info   (int client)
    {
        static ClientInfo null_client = { 0, UNKNOWN_CLIENT };

        ClientRepository::iterator it = m_client_repository.find (client);

        if (it != m_client_repository.end ())
            return it->second;

        return null_client;
    }

private:
    void socket_turn_on                         (void)
    {
        SCIM_DEBUG_MAIN(4) << "PanelAgent::socket_turn_on ()\n";

        m_signal_turn_on ();
    }

    void socket_turn_off                        (void)
    {
        SCIM_DEBUG_MAIN(4) << "PanelAgent::socket_turn_off ()\n";

        m_signal_turn_off ();
    }

    void socket_update_screen                   (void)
    {
        SCIM_DEBUG_MAIN(4) << "PanelAgent::socket_update_screen ()\n";

        uint32 num;
        if (m_recv_trans.get_data (num) && ((int) num) != m_current_screen) {
            SCIM_DEBUG_MAIN(4) << "New Screen number = " << num << "\n";
            m_signal_update_screen ((int) num);
            helper_all_update_screen ((int) num);
            m_current_screen = (num);
        }
    }

    void socket_update_spot_location            (void)
    {
        SCIM_DEBUG_MAIN(4) << "PanelAgent::socket_update_spot_location ()\n";

        uint32 x, y, top_y;
        if (m_recv_trans.get_data (x) && m_recv_trans.get_data (y) && m_recv_trans.get_data (top_y)) {
            SCIM_DEBUG_MAIN(4) << "New Spot location x=" << x << " y=" << y << "\n";
            m_signal_update_spot_location ((int)x, (int)y, (int)top_y);
            helper_all_update_spot_location ((int)x, (int)y);
        }
    }

    void socket_update_cursor_position          (void)
    {
        SCIM_DEBUG_MAIN(4) << "PanelAgent::socket_update_cursor_position ()\n";

        uint32 cursor_pos;
        if (m_recv_trans.get_data (cursor_pos)) {
            SCIM_DEBUG_MAIN(4) << "New cursor position pos=" << cursor_pos << "\n";
            helper_all_update_cursor_position ((int)cursor_pos);
        }
    }

    void socket_update_surrounding_text         (void)
    {
        SCIM_DEBUG_MAIN(4) << __FUNCTION__ << "...\n";

        String text;
        uint32 cursor;
        if (m_recv_trans.get_data (text) && m_recv_trans.get_data (cursor)) {
            HelperClientIndex::iterator it = m_helper_client_index.find (m_current_helper_uuid);

            if (it != m_helper_client_index.end ()) {
                int    client;
                uint32 context;
                Socket client_socket (it->second.id);
                uint32 ctx;

                lock ();

                get_focused_context (client, context);
                ctx = get_helper_ic (client, context);

                m_send_trans.clear ();
                m_send_trans.put_command (SCIM_TRANS_CMD_REPLY);
                m_send_trans.put_data (ctx);
                m_send_trans.put_data (m_current_helper_uuid);
                m_send_trans.put_command (ISM_TRANS_CMD_UPDATE_SURROUNDING_TEXT);
                m_send_trans.put_data (text);
                m_send_trans.put_data (cursor);
                m_send_trans.write_to_socket (client_socket);

                unlock ();
            }
        }
    }

    void socket_update_selection         (void)
    {
        SCIM_DEBUG_MAIN(4) << __FUNCTION__ << "...\n";

        String text;
        if (m_recv_trans.get_data (text)) {
            HelperClientIndex::iterator it = m_helper_client_index.find (m_current_helper_uuid);

            if (it != m_helper_client_index.end ()) {
                int    client;
                uint32 context;
                Socket client_socket (it->second.id);
                uint32 ctx;

                lock ();

                get_focused_context (client, context);
                ctx = get_helper_ic (client, context);

                m_send_trans.clear ();
                m_send_trans.put_command (SCIM_TRANS_CMD_REPLY);
                m_send_trans.put_data (ctx);
                m_send_trans.put_data (m_current_helper_uuid);
                m_send_trans.put_command (ISM_TRANS_CMD_UPDATE_SELECTION);
                m_send_trans.put_data (text);
                m_send_trans.write_to_socket (client_socket);

                unlock ();
            }
        }
    }

    void socket_update_factory_info             (void)
    {
        SCIM_DEBUG_MAIN(4) << "PanelAgent::socket_update_factory_info ()\n";

        PanelFactoryInfo info;
        if (m_recv_trans.get_data (info.uuid) && m_recv_trans.get_data (info.name) &&
            m_recv_trans.get_data (info.lang) && m_recv_trans.get_data (info.icon)) {
            SCIM_DEBUG_MAIN(4) << "New Factory info uuid=" << info.uuid << " name=" << info.name << "\n";
            info.lang = scim_get_normalized_language (info.lang);
            m_signal_update_factory_info (info);
        }
    }

    void socket_show_help                       (void)
    {
        SCIM_DEBUG_MAIN(4) << "PanelAgent::socket_show_help ()\n";

        String help;
        if (m_recv_trans.get_data (help))
            m_signal_show_help (help);
    }

    void socket_show_factory_menu               (void)
    {
        SCIM_DEBUG_MAIN(4) << "PanelAgent::socket_show_factory_menu ()\n";

        PanelFactoryInfo info;
        std::vector <PanelFactoryInfo> vec;

        while (m_recv_trans.get_data (info.uuid) && m_recv_trans.get_data (info.name) &&
               m_recv_trans.get_data (info.lang) && m_recv_trans.get_data (info.icon)) {
            info.lang = scim_get_normalized_language (info.lang);
            vec.push_back (info);
        }

        if (vec.size ())
            m_signal_show_factory_menu (vec);
    }

    void socket_turn_on_log                      (void)
    {
        uint32 isOn;
        if (m_recv_trans.get_data (isOn)) {
            if (isOn) {
                DebugOutput::enable_debug (SCIM_DEBUG_AllMask);
                DebugOutput::set_verbose_level (7);
                SCIM_DEBUG_MAIN(4) << __func__ << " on\n";
            } else {
                SCIM_DEBUG_MAIN(4) << __func__ << " off\n";
                DebugOutput::disable_debug (SCIM_DEBUG_AllMask);
                DebugOutput::set_verbose_level (0);
            }

            int     focused_client;
            uint32  focused_context;

            get_focused_context (focused_client, focused_context);

            if (TOOLBAR_HELPER_MODE == m_current_toolbar_mode || m_current_helper_option & ISM_HELPER_PROCESS_KEYBOARD_KEYEVENT) {
                HelperClientIndex::iterator it = m_helper_client_index.find (m_current_helper_uuid);

                if (it != m_helper_client_index.end ()) {
                    Socket client_socket (it->second.id);
                    uint32 ctx;

                    ctx = get_helper_ic (focused_client, focused_context);

                    m_send_trans.clear ();
                    m_send_trans.put_command (SCIM_TRANS_CMD_REPLY);
                    m_send_trans.put_data (ctx);
                    m_send_trans.put_data (m_current_helper_uuid);
                    m_send_trans.put_command (ISM_TRANS_CMD_TURN_ON_LOG);
                    m_send_trans.put_data (isOn);
                    m_send_trans.write_to_socket (client_socket);
                }
            }

            if (focused_client == -1) {
                std::cerr << __func__ << " get_focused_context is failed!!!\n";
                return;
            }

            ClientInfo client_info = socket_get_client_info (focused_client);
            if (client_info.type == FRONTEND_CLIENT) {
                Socket socket_client (focused_client);
                lock ();
                m_send_trans.clear ();
                m_send_trans.put_command (SCIM_TRANS_CMD_REPLY);
                m_send_trans.put_data (focused_context);
                m_send_trans.put_command (ISM_TRANS_CMD_TURN_ON_LOG);
                m_send_trans.put_data (isOn);
                m_send_trans.write_to_socket (socket_client);
                unlock ();
            }
        }
    }

    void socket_show_preedit_string             (void)
    {
        SCIM_DEBUG_MAIN(4) << "PanelAgent::socket_show_preedit_string ()\n";
        m_signal_show_preedit_string ();
    }

    void socket_show_aux_string                 (void)
    {
        SCIM_DEBUG_MAIN(4) << "PanelAgent::socket_show_aux_string ()\n";
        m_signal_show_aux_string ();
    }

    void socket_show_lookup_table               (void)
    {
        SCIM_DEBUG_MAIN(4) << "PanelAgent::socket_show_lookup_table ()\n";
        m_signal_show_lookup_table ();
    }

    void socket_show_associate_table            (void)
    {
        SCIM_DEBUG_MAIN(4) << "PanelAgent::socket_show_associate_table ()\n";
        m_signal_show_associate_table ();
    }

    void socket_hide_preedit_string             (void)
    {
        SCIM_DEBUG_MAIN(4) << "PanelAgent::socket_hide_preedit_string ()\n";
        m_signal_hide_preedit_string ();
    }

    void socket_hide_aux_string                 (void)
    {
        SCIM_DEBUG_MAIN(4) << "PanelAgent::socket_hide_aux_string ()\n";
        m_signal_hide_aux_string ();
    }

    void socket_hide_lookup_table               (void)
    {
        SCIM_DEBUG_MAIN(4) << "PanelAgent::socket_hide_lookup_table ()\n";
        m_signal_hide_lookup_table ();
    }

    void socket_hide_associate_table            (void)
    {
        SCIM_DEBUG_MAIN(4) << "PanelAgent::socket_hide_associate_table ()\n";
        m_signal_hide_associate_table ();
    }

    void socket_update_preedit_string           (void)
    {
        SCIM_DEBUG_MAIN(4) << "PanelAgent::socket_update_preedit_string ()\n";

        String str;
        AttributeList attrs;
        uint32 caret;
        if (m_recv_trans.get_data (str) && m_recv_trans.get_data (attrs) && m_recv_trans.get_data (caret))
            m_signal_update_preedit_string (str, attrs, (int) caret);
    }

    void socket_update_preedit_caret            (void)
    {
        SCIM_DEBUG_MAIN(4) << "PanelAgent::socket_update_preedit_caret ()\n";

        uint32 caret;
        if (m_recv_trans.get_data (caret))
            m_signal_update_preedit_caret ((int) caret);
    }

    void socket_update_aux_string               (void)
    {
        SCIM_DEBUG_MAIN(4) << "PanelAgent::socket_update_aux_string ()\n";

        String str;
        AttributeList attrs;
        if (m_recv_trans.get_data (str) && m_recv_trans.get_data (attrs))
            m_signal_update_aux_string (str, attrs);
    }

    void socket_update_lookup_table             (void)
    {
        SCIM_DEBUG_MAIN(4) << "PanelAgent::socket_update_lookup_table ()\n";

        lock ();
        if (m_recv_trans.get_data (g_isf_candidate_table)) {
            unlock ();
            m_signal_update_lookup_table (g_isf_candidate_table);
        } else {
            unlock ();
        }
    }

    void socket_update_associate_table          (void)
    {
        SCIM_DEBUG_MAIN(4) << "PanelAgent::socket_update_associate_table ()\n";

        CommonLookupTable table;
        if (m_recv_trans.get_data (table))
            m_signal_update_associate_table (table);
    }

    void socket_update_control_panel            (void)
    {
        SCIM_DEBUG_MAIN(4) << "PanelAgent::socket_update_control_panel ()\n";

        String name, uuid;
        m_signal_get_keyboard_ise (name, uuid);

        PanelFactoryInfo info;
        if (name.length () > 0)
            info = PanelFactoryInfo (uuid, name, String (""), String (""));
        else
            info = PanelFactoryInfo (String (""), String (_("English Keyboard")), String ("C"), String (SCIM_KEYBOARD_ICON_FILE));
        m_signal_update_factory_info (info);
    }

    void socket_register_properties             (void)
    {
        SCIM_DEBUG_MAIN(4) << "PanelAgent::socket_register_properties ()\n";

        PropertyList properties;

        if (m_recv_trans.get_data (properties))
            m_signal_register_properties (properties);
    }

    void socket_update_property                 (void)
    {
        SCIM_DEBUG_MAIN(4) << "PanelAgent::socket_update_property ()\n";

        Property property;
        if (m_recv_trans.get_data (property))
            m_signal_update_property (property);
    }

    void socket_get_keyboard_ise_list           (void)
    {
        SCIM_DEBUG_MAIN(4) << "PanelAgent::socket_get_keyboard_ise_list ()\n";

        std::vector<String> list;
        list.clear ();
        m_signal_get_keyboard_ise_list (list);

        String uuid;
        if (m_recv_trans.get_data (uuid)) {
            HelperClientIndex::iterator it = m_helper_client_index.find (uuid);
            if (it != m_helper_client_index.end ()) {
                int    client;
                uint32 context;
                get_focused_context (client, context);
                uint32 ctx = get_helper_ic (client, context);
                Socket socket_client (it->second.id);
                m_send_trans.clear ();
                m_send_trans.put_command (SCIM_TRANS_CMD_REPLY);
                m_send_trans.put_data (ctx);
                m_send_trans.put_data (uuid);
                m_send_trans.put_command (ISM_TRANS_CMD_UPDATE_KEYBOARD_ISE_LIST);
                m_send_trans.put_data (list.size ());
                for (unsigned int i = 0; i < list.size (); i++)
                    m_send_trans.put_data (list[i]);
                m_send_trans.write_to_socket (socket_client);
            }
        }
    }

    void socket_set_candidate_ui                (void)
    {
        SCIM_DEBUG_MAIN(4) << __func__ << " \n";

        uint32 portrait_line, mode;
        if (m_recv_trans.get_data (portrait_line) && m_recv_trans.get_data (mode))
            m_signal_set_candidate_ui (portrait_line, mode);
    }

    void socket_get_candidate_ui                (void)
    {
        SCIM_DEBUG_MAIN(4) << "PanelAgent::socket_get_candidate_ui ()\n";

        int style = 0, mode = 0;
        m_signal_get_candidate_ui (style, mode);

        String uuid;
        if (m_recv_trans.get_data (uuid)) {
            HelperClientIndex::iterator it = m_helper_client_index.find (uuid);
            if (it != m_helper_client_index.end ()) {
                int    client;
                uint32 context;
                get_focused_context (client, context);
                uint32 ctx = get_helper_ic (client, context);
                Socket socket_client (it->second.id);
                m_send_trans.clear ();
                m_send_trans.put_command (SCIM_TRANS_CMD_REPLY);
                m_send_trans.put_data (ctx);
                m_send_trans.put_data (uuid);
                m_send_trans.put_command (ISM_TRANS_CMD_UPDATE_CANDIDATE_UI);
                m_send_trans.put_data (style);
                m_send_trans.put_data (mode);
                m_send_trans.write_to_socket (socket_client);
            }
        }
    }

    void socket_set_candidate_position          (void)
    {
        SCIM_DEBUG_MAIN(4) << "PanelAgent::socket_set_candidate_position ()\n";

        uint32 left, top;
        if (m_recv_trans.get_data (left) && m_recv_trans.get_data (top))
            m_signal_set_candidate_position (left, top);
    }

    void socket_hide_candidate                  (void)
    {
        SCIM_DEBUG_MAIN(4) << "PanelAgent::socket_hide_candidate ()\n";

        m_signal_hide_preedit_string ();
        m_signal_hide_aux_string ();
        m_signal_hide_lookup_table ();
        m_signal_hide_associate_table ();
    }

    void socket_get_candidate_geometry          (void)
    {
        SCIM_DEBUG_MAIN(4) << __func__ << " \n";

        String uuid;
        if (m_recv_trans.get_data (uuid)) {
            HelperClientIndex::iterator it = m_helper_client_index.find (uuid);
            if (it != m_helper_client_index.end ()) {
                struct rectinfo info = {0, 0, 0, 0};
                m_signal_get_candidate_geometry (info);

                int    client;
                uint32 context;
                get_focused_context (client, context);
                uint32 ctx = get_helper_ic (client, context);
                Socket socket_client (it->second.id);
                m_send_trans.clear ();
                m_send_trans.put_command (SCIM_TRANS_CMD_REPLY);
                m_send_trans.put_data (ctx);
                m_send_trans.put_data (uuid);
                m_send_trans.put_command (ISM_TRANS_CMD_UPDATE_CANDIDATE_GEOMETRY);
                m_send_trans.put_data (info.pos_x);
                m_send_trans.put_data (info.pos_y);
                m_send_trans.put_data (info.width);
                m_send_trans.put_data (info.height);
                m_send_trans.write_to_socket (socket_client);
            }
        }
    }

    void socket_set_keyboard_ise (void)
    {
        SCIM_DEBUG_MAIN(4) << "PanelAgent::socket_set_keyboard_ise ()\n";

        String uuid;
        if (m_recv_trans.get_data (uuid))
            m_signal_set_keyboard_ise (uuid);
    }

    void socket_helper_select_candidate (void)
    {
        SCIM_DEBUG_MAIN(4) << "PanelAgent::socket_set_keyboard_ise ()\n";

        uint32 index;
        if (m_recv_trans.get_data (index))
            m_signal_select_candidate (index);
    }

    void socket_helper_update_ise_geometry (int client)
    {
        SCIM_DEBUG_MAIN(4) << __func__ << "\n";

        uint32 x, y, width, height;
        if (m_recv_trans.get_data (x) && m_recv_trans.get_data (y) &&
            m_recv_trans.get_data (width) && m_recv_trans.get_data (height)) {
            HelperInfoRepository::iterator it = m_helper_active_info_repository.find (client);
            if (it != m_helper_active_info_repository.end ()) {
                if (it->second.uuid == m_current_helper_uuid)
                    m_signal_update_ise_geometry (x, y, width, height);
            }
        }
    }

    void socket_helper_expand_candidate (int client)
    {
        SCIM_DEBUG_MAIN(4) << __func__ << "\n";

        HelperInfoRepository::iterator it = m_helper_active_info_repository.find (client);
        if (it != m_helper_active_info_repository.end ()) {
            if (it->second.uuid == m_current_helper_uuid)
                m_signal_expand_candidate ();
        }
    }

    void socket_helper_contract_candidate (int client)
    {
        SCIM_DEBUG_MAIN(4) << __func__ << "\n";

        HelperInfoRepository::iterator it = m_helper_active_info_repository.find (client);
        if (it != m_helper_active_info_repository.end ()) {
            if (it->second.uuid == m_current_helper_uuid)
                m_signal_contract_candidate ();
        }
    }

    void socket_get_keyboard_ise                (void)
    {
        SCIM_DEBUG_MAIN(4) << "PanelAgent::socket_get_keyboard_ise ()\n";

        String ise_name, ise_uuid;
        int    client  = -1;
        uint32 context = 0;
        uint32 ctx     = 0;

        get_focused_context (client, context);
        ctx = get_helper_ic (client, context);

        if (m_client_context_uuids.find (ctx) != m_client_context_uuids.end ())
            ise_uuid = m_client_context_uuids[ctx];
        m_signal_get_keyboard_ise (ise_name, ise_uuid);

        String uuid;
        if (m_recv_trans.get_data (uuid)) {
            HelperClientIndex::iterator it = m_helper_client_index.find (uuid);
            if (it != m_helper_client_index.end ()) {
                get_focused_context (client, context);
                ctx = get_helper_ic (client, context);
                Socket socket_client (it->second.id);
                m_send_trans.clear ();
                m_send_trans.put_command (SCIM_TRANS_CMD_REPLY);
                m_send_trans.put_data (ctx);
                m_send_trans.put_data (uuid);
                m_send_trans.put_command (ISM_TRANS_CMD_UPDATE_KEYBOARD_ISE);
                m_send_trans.put_data (ise_name);
                m_send_trans.put_data (ise_uuid);
                m_send_trans.write_to_socket (socket_client);
            }
        }
    }

    void socket_start_helper                    (int client, uint32 context, const String &ic_uuid)
    {
        SCIM_DEBUG_MAIN(4) << "PanelAgent::socket_start_helper ()\n";

        String uuid;
        if (m_recv_trans.get_data (uuid) && uuid.length ()) {
            HelperClientIndex::iterator it = m_helper_client_index.find (uuid);

            lock ();

            uint32 ic = get_helper_ic (client, context);

            SCIM_DEBUG_MAIN(5) << "Helper UUID =" << uuid << "  IC UUID =" << ic_uuid <<"\n";

            if (it == m_helper_client_index.end ()) {
                SCIM_DEBUG_MAIN(5) << "Run this Helper.\n";
                m_start_helper_ic_index [uuid].push_back (std::make_pair (ic, ic_uuid));
                m_helper_manager.run_helper (uuid, m_config_name, m_display_name);
            } else {
                SCIM_DEBUG_MAIN(5) << "Increase the Reference count.\n";
                Socket client_socket (it->second.id);
                m_send_trans.clear ();
                m_send_trans.put_command (SCIM_TRANS_CMD_REPLY);
                m_send_trans.put_data (ic);
                m_send_trans.put_data (ic_uuid);
                m_send_trans.put_command (SCIM_TRANS_CMD_HELPER_ATTACH_INPUT_CONTEXT);
                m_send_trans.write_to_socket (client_socket);
                ++ it->second.ref;
            }

            unlock ();
        }
    }

    void socket_stop_helper                     (int client, uint32 context, const String &ic_uuid)
    {
        SCIM_DEBUG_MAIN(4) << "PanelAgent::socket_stop_helper ()\n";

        String uuid;
        if (m_recv_trans.get_data (uuid) && uuid.length ()) {
            HelperClientIndex::iterator it = m_helper_client_index.find (uuid);

            lock ();

            uint32 ic = get_helper_ic (client, context);

            SCIM_DEBUG_MAIN(5) << "Helper UUID =" << uuid << "  IC UUID =" << ic_uuid <<"\n";

            if (it != m_helper_client_index.end ()) {
                SCIM_DEBUG_MAIN(5) << "Decrase the Reference count.\n";
                -- it->second.ref;
                Socket client_socket (it->second.id);
                m_send_trans.clear ();
                m_send_trans.put_command (SCIM_TRANS_CMD_REPLY);
                m_send_trans.put_data (ic);
                m_send_trans.put_data (ic_uuid);
                m_send_trans.put_command (SCIM_TRANS_CMD_HELPER_DETACH_INPUT_CONTEXT);
                if (it->second.ref <= 0)
                    m_send_trans.put_command (SCIM_TRANS_CMD_EXIT);
                m_send_trans.write_to_socket (client_socket);
            }

            unlock ();
        }
    }

    void socket_send_helper_event               (int client, uint32 context, const String &ic_uuid)
    {
        SCIM_DEBUG_MAIN(4) << "PanelAgent::socket_send_helper_event ()\n";

        String uuid;
        if (m_recv_trans.get_data (uuid) && m_recv_trans.get_data (m_nest_trans) &&
            uuid.length () && m_nest_trans.valid ()) {
            HelperClientIndex::iterator it = m_helper_client_index.find (uuid);
            if (it != m_helper_client_index.end ()) {
                Socket client_socket (it->second.id);

                lock ();

                m_send_trans.clear ();
                m_send_trans.put_command (SCIM_TRANS_CMD_REPLY);

                /* FIXME: We presume that client and context are both less than 65536.
                 * Hopefully, it should be true in any UNIXs.
                 * So it's ok to combine client and context into one uint32. */
                m_send_trans.put_data (get_helper_ic (client, context));
                m_send_trans.put_data (ic_uuid);
                m_send_trans.put_command (SCIM_TRANS_CMD_HELPER_PROCESS_IMENGINE_EVENT);
                m_send_trans.put_data (m_nest_trans);
                m_send_trans.write_to_socket (client_socket);

                unlock ();
            }
        }
    }

    void socket_helper_register_properties      (int client)
    {
        SCIM_DEBUG_MAIN(4) << "PanelAgent::socket_helper_register_properties (" << client << ")\n";

        PropertyList properties;
        if (m_recv_trans.get_data (properties))
            m_signal_register_helper_properties (client, properties);
    }

    void socket_helper_update_property          (int client)
    {
        SCIM_DEBUG_MAIN(4) << "PanelAgent::socket_helper_update_property (" << client << ")\n";

        Property property;
        if (m_recv_trans.get_data (property))
            m_signal_update_helper_property (client, property);
    }

    void socket_helper_send_imengine_event      (int client)
    {
        SCIM_DEBUG_MAIN(4) << "PanelAgent::socket_helper_send_imengine_event (" << client << ")\n";

        uint32 target_ic;
        String target_uuid;

        HelperInfoRepository::iterator hiit = m_helper_active_info_repository.find (client);

        if (m_recv_trans.get_data (target_ic)    &&
            m_recv_trans.get_data (target_uuid)  &&
            m_recv_trans.get_data (m_nest_trans) &&
            m_nest_trans.valid ()                &&
            hiit != m_helper_active_info_repository.end ()) {

            int     target_client;
            uint32  target_context;

            get_imengine_client_context (target_ic, target_client, target_context);

            int     focused_client;
            uint32  focused_context;
            String  focused_uuid;

            focused_uuid = get_focused_context (focused_client, focused_context);

            if (target_ic == (uint32) (-1)) {
                target_client  = focused_client;
                target_context = focused_context;
            }

            if (target_uuid.length () == 0)
                target_uuid = focused_uuid;

            ClientInfo  client_info = socket_get_client_info (target_client);

            SCIM_DEBUG_MAIN(5) << "Target UUID = " << target_uuid << "  Focused UUId = " << focused_uuid << "\nTarget Client = " << target_client << "\n";

            if (client_info.type == FRONTEND_CLIENT) {
                Socket socket_client (target_client);
                lock ();
                m_send_trans.clear ();
                m_send_trans.put_command (SCIM_TRANS_CMD_REPLY);
                m_send_trans.put_data (target_context);
                m_send_trans.put_command (SCIM_TRANS_CMD_PROCESS_HELPER_EVENT);
                m_send_trans.put_data (target_uuid);
                m_send_trans.put_data (hiit->second.uuid);
                m_send_trans.put_data (m_nest_trans);
                m_send_trans.write_to_socket (socket_client);
                unlock ();
            }
        }
    }

    void socket_helper_key_event_op (int client, int cmd)
    {
        uint32 target_ic;
        String target_uuid;
        KeyEvent key;

        if (m_recv_trans.get_data (target_ic)    &&
            m_recv_trans.get_data (target_uuid)  &&
            m_recv_trans.get_data (key)          &&
            !key.empty ()) {

            int     target_client;
            uint32  target_context;

            get_imengine_client_context (target_ic, target_client, target_context);

            int     focused_client;
            uint32  focused_context;
            String  focused_uuid;

            focused_uuid = get_focused_context (focused_client, focused_context);

            if (target_ic == (uint32) (-1)) {
                target_client  = focused_client;
                target_context = focused_context;
            }

            if (target_uuid.length () == 0)
                target_uuid = focused_uuid;

            if (target_client == -1) {
                /* FIXUP: monitor 'Invalid Window' error */
                std::cerr << "focused target client is NULL" << "\n";
            } else if (target_client == focused_client && target_context == focused_context && target_uuid == focused_uuid) {
                ClientInfo client_info = socket_get_client_info (target_client);
                if (client_info.type == FRONTEND_CLIENT) {
                    Socket socket_client (target_client);
                    lock ();
                    m_send_trans.clear ();
                    m_send_trans.put_command (SCIM_TRANS_CMD_REPLY);
                    m_send_trans.put_data (target_context);
                    m_send_trans.put_command (cmd);
                    m_send_trans.put_data (key);
                    m_send_trans.write_to_socket (socket_client);
                    unlock ();
                }
            }
        }
    }

    void socket_helper_send_key_event (int client)
    {
        SCIM_DEBUG_MAIN(4) << "PanelAgent::socket_helper_send_key_event (" << client << ")\n";
        ISF_PROF_DEBUG("first message")

        socket_helper_key_event_op (client, SCIM_TRANS_CMD_PROCESS_KEY_EVENT);
    }

    void socket_helper_forward_key_event        (int client)
    {
        SCIM_DEBUG_MAIN(4) << "PanelAgent::socket_helper_forward_key_event (" << client << ")\n";

        socket_helper_key_event_op (client, SCIM_TRANS_CMD_FORWARD_KEY_EVENT);
    }

    void socket_helper_commit_string            (int client)
    {
        SCIM_DEBUG_MAIN(4) << "PanelAgent::socket_helper_commit_string (" << client << ")\n";

        uint32 target_ic;
        String target_uuid;
        WideString wstr;

        if (m_recv_trans.get_data (target_ic)    &&
            m_recv_trans.get_data (target_uuid)  &&
            m_recv_trans.get_data (wstr)         &&
            wstr.length ()) {

            int     target_client;
            uint32  target_context;

            get_imengine_client_context (target_ic, target_client, target_context);

            int     focused_client;
            uint32  focused_context;
            String  focused_uuid;

            focused_uuid = get_focused_context (focused_client, focused_context);

            if (target_ic == (uint32) (-1)) {
                target_client  = focused_client;
                target_context = focused_context;
            }

            if (target_uuid.length () == 0)
                target_uuid = focused_uuid;

            if (target_client  == focused_client &&
                target_context == focused_context &&
                target_uuid    == focused_uuid) {
                ClientInfo client_info = socket_get_client_info (target_client);
                if (client_info.type == FRONTEND_CLIENT) {
                    Socket socket_client (target_client);
                    lock ();
                    m_send_trans.clear ();
                    m_send_trans.put_command (SCIM_TRANS_CMD_REPLY);
                    m_send_trans.put_data (target_context);
                    m_send_trans.put_command (SCIM_TRANS_CMD_COMMIT_STRING);
                    m_send_trans.put_data (wstr);
                    m_send_trans.write_to_socket (socket_client);
                    unlock ();
                } else {
                    std::cerr << "target client is not existed!!!" << "\n";
                }
            }
        }
    }

    void socket_helper_get_surrounding_text     (int client)
    {
        SCIM_DEBUG_MAIN(4) << __FUNCTION__ << " (" << client << ")\n";

        String uuid;
        uint32 maxlen_before;
        uint32 maxlen_after;

        if (m_recv_trans.get_data (uuid) &&
            m_recv_trans.get_data (maxlen_before) &&
            m_recv_trans.get_data (maxlen_after)) {

            int     focused_client;
            uint32  focused_context;
            String  focused_uuid = get_focused_context (focused_client, focused_context);

            ClientInfo client_info = socket_get_client_info (focused_client);
            if (client_info.type == FRONTEND_CLIENT) {
                Socket socket_client (focused_client);
                lock ();
                m_send_trans.clear ();
                m_send_trans.put_command (SCIM_TRANS_CMD_REPLY);
                m_send_trans.put_data (focused_context);
                m_send_trans.put_command (SCIM_TRANS_CMD_GET_SURROUNDING_TEXT);
                m_send_trans.put_data (maxlen_before);
                m_send_trans.put_data (maxlen_after);
                m_send_trans.write_to_socket (socket_client);
                unlock ();
            }
        }
    }

    void socket_helper_delete_surrounding_text  (int client)
    {
        SCIM_DEBUG_MAIN(4) << __FUNCTION__ << " (" << client << ")\n";

        uint32 offset;
        uint32 len;

        if (m_recv_trans.get_data (offset) && m_recv_trans.get_data (len)) {

            int     focused_client;
            uint32  focused_context;
            String  focused_uuid = get_focused_context (focused_client, focused_context);

            ClientInfo client_info = socket_get_client_info (focused_client);
            if (client_info.type == FRONTEND_CLIENT) {
                Socket socket_client (focused_client);
                lock ();
                m_send_trans.clear ();
                m_send_trans.put_command (SCIM_TRANS_CMD_REPLY);
                m_send_trans.put_data (focused_context);
                m_send_trans.put_command (SCIM_TRANS_CMD_DELETE_SURROUNDING_TEXT);
                m_send_trans.put_data (offset);
                m_send_trans.put_data (len);
                m_send_trans.write_to_socket (socket_client);
                unlock ();
            }
        }
    }

    void socket_helper_get_selection     (int client)
    {
        SCIM_DEBUG_MAIN(4) << __FUNCTION__ << " (" << client << ")\n";

        String uuid;

        if (m_recv_trans.get_data (uuid)) {

            int     focused_client;
            uint32  focused_context;
            String  focused_uuid = get_focused_context (focused_client, focused_context);

            ClientInfo client_info = socket_get_client_info (focused_client);
            if (client_info.type == FRONTEND_CLIENT) {
                Socket socket_client (focused_client);
                lock ();
                m_send_trans.clear ();
                m_send_trans.put_command (SCIM_TRANS_CMD_REPLY);
                m_send_trans.put_data (focused_context);
                m_send_trans.put_command (SCIM_TRANS_CMD_GET_SELECTION);
                m_send_trans.write_to_socket (socket_client);
                unlock ();
            }
        }
    }

    void socket_helper_set_selection  (int client)
    {
        SCIM_DEBUG_MAIN(4) << __FUNCTION__ << " (" << client << ")\n";

        uint32 start;
        uint32 end;

        if (m_recv_trans.get_data (start) && m_recv_trans.get_data (end)) {

            int     focused_client;
            uint32  focused_context;
            String  focused_uuid = get_focused_context (focused_client, focused_context);

            ClientInfo client_info = socket_get_client_info (focused_client);
            if (client_info.type == FRONTEND_CLIENT) {
                Socket socket_client (focused_client);
                lock ();
                m_send_trans.clear ();
                m_send_trans.put_command (SCIM_TRANS_CMD_REPLY);
                m_send_trans.put_data (focused_context);
                m_send_trans.put_command (SCIM_TRANS_CMD_SET_SELECTION);
                m_send_trans.put_data (start);
                m_send_trans.put_data (end);
                m_send_trans.write_to_socket (socket_client);
                unlock ();
            }
        }
    }

    void socket_helper_show_preedit_string            (int client)
    {
        SCIM_DEBUG_MAIN(4) << "PanelAgent::socket_helper_show_preedit_string (" << client << ")\n";

        uint32 target_ic;
        String target_uuid;

        if (m_recv_trans.get_data (target_ic) && m_recv_trans.get_data (target_uuid)) {
            int     target_client;
            uint32  target_context;

            get_imengine_client_context (target_ic, target_client, target_context);

            int     focused_client;
            uint32  focused_context;
            String  focused_uuid = get_focused_context (focused_client, focused_context);

            if (target_ic == (uint32) (-1)) {
                target_client  = focused_client;
                target_context = focused_context;
            }

            if (target_uuid.length () == 0)
                target_uuid = focused_uuid;

            if (target_client  == focused_client &&
                target_context == focused_context &&
                target_uuid    == focused_uuid) {
                ClientInfo client_info = socket_get_client_info (target_client);
                if (client_info.type == FRONTEND_CLIENT) {
                    Socket socket_client (target_client);
                    lock ();
                    m_send_trans.clear ();
                    m_send_trans.put_command (SCIM_TRANS_CMD_REPLY);
                    m_send_trans.put_data (target_context);
                    m_send_trans.put_command (SCIM_TRANS_CMD_SHOW_PREEDIT_STRING);
                    m_send_trans.write_to_socket (socket_client);
                    unlock ();
                }
            }
        }
    }

    void socket_helper_hide_preedit_string            (int client)
    {
        SCIM_DEBUG_MAIN(4) << "PanelAgent::socket_helper_hide_preedit_string (" << client << ")\n";

        uint32 target_ic;
        String target_uuid;

        if (m_recv_trans.get_data (target_ic) && m_recv_trans.get_data (target_uuid)) {
            int     target_client;
            uint32  target_context;

            get_imengine_client_context (target_ic, target_client, target_context);

            int     focused_client;
            uint32  focused_context;
            String  focused_uuid = get_focused_context (focused_client, focused_context);

            if (target_ic == (uint32) (-1)) {
                target_client  = focused_client;
                target_context = focused_context;
            }

            if (target_uuid.length () == 0)
                target_uuid = focused_uuid;

            if (target_client  == focused_client &&
                target_context == focused_context &&
                target_uuid    == focused_uuid) {
                ClientInfo client_info = socket_get_client_info (target_client);
                if (client_info.type == FRONTEND_CLIENT) {
                    Socket socket_client (target_client);
                    lock ();
                    m_send_trans.clear ();
                    m_send_trans.put_command (SCIM_TRANS_CMD_REPLY);
                    m_send_trans.put_data (target_context);
                    m_send_trans.put_command (SCIM_TRANS_CMD_HIDE_PREEDIT_STRING);
                    m_send_trans.write_to_socket (socket_client);
                    unlock ();
                }
            }
        }
    }

    void socket_helper_update_preedit_string            (int client)
    {
        SCIM_DEBUG_MAIN(4) << "PanelAgent::socket_helper_update_preedit_string (" << client << ")\n";

        uint32 target_ic;
        String target_uuid;
        WideString wstr;
        AttributeList attrs;
        uint32 caret;

        if (m_recv_trans.get_data (target_ic)    &&
            m_recv_trans.get_data (target_uuid)  &&
            m_recv_trans.get_data (wstr) &&
            m_recv_trans.get_data (attrs) &&
            m_recv_trans.get_data (caret)) {

            int     target_client;
            uint32  target_context;

            get_imengine_client_context (target_ic, target_client, target_context);

            int     focused_client;
            uint32  focused_context;
            String  focused_uuid;

            focused_uuid = get_focused_context (focused_client, focused_context);

            if (target_ic == (uint32) (-1)) {
                target_client  = focused_client;
                target_context = focused_context;
            }

            if (target_uuid.length () == 0)
                target_uuid = focused_uuid;

            if (target_client  == focused_client &&
                target_context == focused_context &&
                target_uuid    == focused_uuid) {
                ClientInfo client_info = socket_get_client_info (target_client);
                if (client_info.type == FRONTEND_CLIENT) {
                    Socket socket_client (target_client);
                    lock ();
                    m_send_trans.clear ();
                    m_send_trans.put_command (SCIM_TRANS_CMD_REPLY);
                    m_send_trans.put_data (target_context);
                    m_send_trans.put_command (SCIM_TRANS_CMD_UPDATE_PREEDIT_STRING);
                    m_send_trans.put_data (wstr);
                    m_send_trans.put_data (attrs);
                    m_send_trans.put_data (caret);
                    m_send_trans.write_to_socket (socket_client);
                    unlock ();
                }
            }
        }
    }

    void socket_helper_update_preedit_caret (int client)
    {
        SCIM_DEBUG_MAIN(4) << __func__ << " (" << client << ")\n";

        uint32 caret;

        if (m_recv_trans.get_data (caret)) {
            int     focused_client;
            uint32  focused_context;
            String  focused_uuid;
            focused_uuid = get_focused_context (focused_client, focused_context);

            ClientInfo client_info = socket_get_client_info (focused_client);
            if (client_info.type == FRONTEND_CLIENT) {
                Socket socket_client (focused_client);
                lock ();
                m_send_trans.clear ();
                m_send_trans.put_command (SCIM_TRANS_CMD_REPLY);
                m_send_trans.put_data (focused_context);
                m_send_trans.put_command (SCIM_TRANS_CMD_UPDATE_PREEDIT_CARET);
                m_send_trans.put_data (caret);
                m_send_trans.write_to_socket (socket_client);
                unlock ();
            }
        }
    }

    void socket_helper_register_helper          (int client)
    {
        SCIM_DEBUG_MAIN(4) << "PanelAgent::socket_helper_register_helper (" << client << ")\n";

        HelperInfo info;

        bool result = false;

        lock ();

        Socket socket_client (client);
        m_send_trans.clear ();
        m_send_trans.put_command (SCIM_TRANS_CMD_REPLY);

        if (m_recv_trans.get_data (info.uuid) &&
            m_recv_trans.get_data (info.name) &&
            m_recv_trans.get_data (info.icon) &&
            m_recv_trans.get_data (info.description) &&
            m_recv_trans.get_data (info.option) &&
            info.uuid.length () &&
            info.name.length ()) {

            SCIM_DEBUG_MAIN(4) << "New Helper uuid=" << info.uuid << " name=" << info.name << "\n";

            HelperClientIndex::iterator it = m_helper_client_index.find (info.uuid);

            if (it == m_helper_client_index.end ()) {
                m_helper_info_repository [client] = info;
                m_helper_client_index [info.uuid] = HelperClientStub (client, 1);
                m_send_trans.put_command (SCIM_TRANS_CMD_OK);

                StartHelperICIndex::iterator icit = m_start_helper_ic_index.find (info.uuid);

                if (icit != m_start_helper_ic_index.end ()) {
                    m_send_trans.put_command (SCIM_TRANS_CMD_HELPER_ATTACH_INPUT_CONTEXT);
                    for (size_t i = 0; i < icit->second.size (); ++i) {
                        m_send_trans.put_data (icit->second [i].first);
                        m_send_trans.put_data (icit->second [i].second);
                    }
                    m_start_helper_ic_index.erase (icit);
                }

                m_send_trans.put_command (SCIM_TRANS_CMD_UPDATE_SCREEN);
                m_send_trans.put_data ((uint32)m_current_screen);

                result = true;
            } else {
                m_send_trans.put_command (SCIM_TRANS_CMD_FAIL);
            }
        }

        m_send_trans.write_to_socket (socket_client);

        unlock ();

        if (result)
            m_signal_register_helper (client, info);
    }

    void socket_helper_register_helper_passive          (int client)
    {
        SCIM_DEBUG_MAIN(4) << "PanelAgent::socket_helper_register_helper_passive (" << client << ")\n";

        HelperInfo info;

        lock ();

        if (m_recv_trans.get_data (info.uuid) &&
            m_recv_trans.get_data (info.name) &&
            m_recv_trans.get_data (info.icon) &&
            m_recv_trans.get_data (info.description) &&
            m_recv_trans.get_data (info.option) &&
            info.uuid.length () &&
            info.name.length ()) {

            SCIM_DEBUG_MAIN(4) << "New Helper Passive uuid=" << info.uuid << " name=" << info.name << "\n";

            HelperInfoRepository::iterator it = m_helper_active_info_repository.find (client);
            if (it == m_helper_active_info_repository.end ()) {
                m_helper_active_info_repository[client] =  info;
            }

            StringIntRepository::iterator iter = m_ise_pending_repository.find (info.uuid);
            if (iter != m_ise_pending_repository.end ()) {
                Transaction trans;
                Socket client_socket (iter->second);
                trans.clear ();
                trans.put_command (SCIM_TRANS_CMD_REPLY);
                trans.put_command (SCIM_TRANS_CMD_OK);
                trans.write_to_socket (client_socket);
                m_ise_pending_repository.erase (iter);
            }

            iter = m_ise_pending_repository.find (info.name);
            if (iter != m_ise_pending_repository.end ()) {
                Transaction trans;
                Socket client_socket (iter->second);
                trans.clear ();
                trans.put_command (SCIM_TRANS_CMD_REPLY);
                trans.put_command (SCIM_TRANS_CMD_OK);
                trans.write_to_socket (client_socket);
                m_ise_pending_repository.erase (iter);
            }
        }

        unlock ();
    }

    void socket_helper_update_input_context          (int client)
    {
        SCIM_DEBUG_MAIN(4) << "PanelAgent::socket_helper_update_input_context (" << client << ")\n";

        uint32 type;
        uint32 value;

        if (m_recv_trans.get_data (type) && m_recv_trans.get_data (value)) {
            m_signal_update_input_context ((int)type, (int)value);

            int    focused_client;
            uint32 focused_context;
            get_focused_context (focused_client, focused_context);

            ClientInfo client_info = socket_get_client_info (focused_client);
            if (client_info.type == FRONTEND_CLIENT) {
                Socket client_socket (focused_client);
                m_send_trans.clear ();
                m_send_trans.put_command (SCIM_TRANS_CMD_REPLY);
                m_send_trans.put_data (focused_context);
                m_send_trans.put_command (ISM_TRANS_CMD_UPDATE_ISE_INPUT_CONTEXT);
                m_send_trans.put_data (type);
                m_send_trans.put_data (value);
                m_send_trans.write_to_socket (client_socket);
            } else {
                std::cerr << "focused client is not existed!!!" << "\n";
            }
        }
    }

    void socket_helper_send_private_command  (int client)
    {
        SCIM_DEBUG_MAIN(4) << __FUNCTION__ << " (" << client << ")\n";

        String command;

        if (m_recv_trans.get_data (command)) {
            int     focused_client;
            uint32  focused_context;
            String  focused_uuid = get_focused_context (focused_client, focused_context);

            ClientInfo client_info = socket_get_client_info (focused_client);
            if (client_info.type == FRONTEND_CLIENT) {
                Socket socket_client (focused_client);
                lock ();
                m_send_trans.clear ();
                m_send_trans.put_command (SCIM_TRANS_CMD_REPLY);
                m_send_trans.put_data (focused_context);
                m_send_trans.put_command (SCIM_TRANS_CMD_SEND_PRIVATE_COMMAND);
                m_send_trans.put_data (command);
                m_send_trans.write_to_socket (socket_client);
                unlock ();
            }
        }
    }

    void socket_reset_helper_input_context (const String &uuid, int client, uint32 context)
    {
        HelperClientIndex::iterator it = m_helper_client_index.find (m_current_helper_uuid);

        if (it != m_helper_client_index.end ()) {
            Socket client_socket (it->second.id);
            uint32 ctx = get_helper_ic (client, context);

            m_send_trans.clear ();
            m_send_trans.put_command (SCIM_TRANS_CMD_REPLY);
            m_send_trans.put_data (ctx);
            m_send_trans.put_data (uuid);
            m_send_trans.put_command (SCIM_TRANS_CMD_PANEL_RESET_INPUT_CONTEXT);
            m_send_trans.write_to_socket (client_socket);
        }
    }

    void socket_reset_input_context (int client, uint32 context)
    {
        SCIM_DEBUG_MAIN(4) << "PanelAgent::socket_reset_input_context \n";

        if (TOOLBAR_HELPER_MODE == m_current_toolbar_mode || m_current_helper_option & ISM_HELPER_PROCESS_KEYBOARD_KEYEVENT)
            socket_reset_helper_input_context (m_current_helper_uuid, client, context);
    }

    bool helper_select_aux (uint32 item)
    {
        SCIM_DEBUG_MAIN(4) << "PanelAgent::helper_select_aux \n";

        if (TOOLBAR_HELPER_MODE == m_current_toolbar_mode || m_current_helper_option & ISM_HELPER_PROCESS_KEYBOARD_KEYEVENT) {
            HelperClientIndex::iterator it = m_helper_client_index.find (m_current_helper_uuid);

            if (it != m_helper_client_index.end ()) {
                int    client;
                uint32 context;
                Socket client_socket (it->second.id);
                uint32 ctx;

                get_focused_context (client, context);
                ctx = get_helper_ic (client, context);

                m_send_trans.clear ();
                m_send_trans.put_command (SCIM_TRANS_CMD_REPLY);
                m_send_trans.put_data (ctx);
                m_send_trans.put_data (m_current_helper_uuid);
                m_send_trans.put_command (ISM_TRANS_CMD_SELECT_AUX);
                m_send_trans.put_data ((uint32)item);
                m_send_trans.write_to_socket (client_socket);

                return true;
            }
        }

        return false;
    }

    bool helper_select_candidate (uint32 item)
    {
        SCIM_DEBUG_MAIN(4) << "PanelAgent::helper_select_candidate \n";

        if (TOOLBAR_HELPER_MODE == m_current_toolbar_mode || m_current_helper_option & ISM_HELPER_PROCESS_KEYBOARD_KEYEVENT) {
            HelperClientIndex::iterator it = m_helper_client_index.find (m_current_helper_uuid);

            if (it != m_helper_client_index.end ()) {
                int    client;
                uint32 context;
                Socket client_socket (it->second.id);
                uint32 ctx;

                get_focused_context (client, context);
                ctx = get_helper_ic (client, context);

                m_send_trans.clear ();
                m_send_trans.put_command (SCIM_TRANS_CMD_REPLY);
                m_send_trans.put_data (ctx);
                m_send_trans.put_data (m_current_helper_uuid);
                m_send_trans.put_command (SCIM_TRANS_CMD_SELECT_CANDIDATE);
                m_send_trans.put_data ((uint32)item);
                m_send_trans.write_to_socket (client_socket);

                return true;
            }
        }

        return false;
    }

    bool helper_lookup_table_page_up (void)
    {
        SCIM_DEBUG_MAIN(4) << "PanelAgent::helper_lookup_table_page_up \n";

        if (TOOLBAR_HELPER_MODE == m_current_toolbar_mode || m_current_helper_option & ISM_HELPER_PROCESS_KEYBOARD_KEYEVENT) {
            HelperClientIndex::iterator it = m_helper_client_index.find (m_current_helper_uuid);

            if (it != m_helper_client_index.end ()) {
                int    client;
                uint32 context;
                Socket client_socket (it->second.id);
                uint32 ctx;

                get_focused_context (client, context);
                ctx = get_helper_ic (client, context);

                m_send_trans.clear ();
                m_send_trans.put_command (SCIM_TRANS_CMD_REPLY);
                m_send_trans.put_data (ctx);
                m_send_trans.put_data (m_current_helper_uuid);
                m_send_trans.put_command (SCIM_TRANS_CMD_LOOKUP_TABLE_PAGE_UP);
                m_send_trans.write_to_socket (client_socket);

                return true;
            }
        }

        return false;
    }

    bool helper_lookup_table_page_down (void)
    {
        SCIM_DEBUG_MAIN(4) << "PanelAgent::helper_lookup_table_page_down \n";

        if (TOOLBAR_HELPER_MODE == m_current_toolbar_mode || m_current_helper_option & ISM_HELPER_PROCESS_KEYBOARD_KEYEVENT) {
            HelperClientIndex::iterator it = m_helper_client_index.find (m_current_helper_uuid);

            if (it != m_helper_client_index.end ()) {
                int    client;
                uint32 context;
                Socket client_socket (it->second.id);
                uint32 ctx;

                get_focused_context (client, context);
                ctx = get_helper_ic (client, context);

                m_send_trans.clear ();
                m_send_trans.put_command (SCIM_TRANS_CMD_REPLY);
                m_send_trans.put_data (ctx);
                m_send_trans.put_data (m_current_helper_uuid);
                m_send_trans.put_command (SCIM_TRANS_CMD_LOOKUP_TABLE_PAGE_DOWN);
                m_send_trans.write_to_socket (client_socket);

                return true;
            }
        }

        return false;
    }

    bool helper_update_lookup_table_page_size (uint32 size)
    {
        SCIM_DEBUG_MAIN(4) << "PanelAgent::helper_update_lookup_table_page_size \n";

        if (TOOLBAR_HELPER_MODE == m_current_toolbar_mode || m_current_helper_option & ISM_HELPER_PROCESS_KEYBOARD_KEYEVENT) {
            HelperClientIndex::iterator it = m_helper_client_index.find (m_current_helper_uuid);

            if (it != m_helper_client_index.end ()) {
                int    client;
                uint32 context;
                Socket client_socket (it->second.id);
                uint32 ctx;

                get_focused_context (client, context);
                ctx = get_helper_ic (client, context);

                m_send_trans.clear ();
                m_send_trans.put_command (SCIM_TRANS_CMD_REPLY);
                m_send_trans.put_data (ctx);
                m_send_trans.put_data (m_current_helper_uuid);
                m_send_trans.put_command (SCIM_TRANS_CMD_UPDATE_LOOKUP_TABLE_PAGE_SIZE);
                m_send_trans.put_data (size);
                m_send_trans.write_to_socket (client_socket);

                return true;
            }
        }

        return false;
    }

    bool helper_update_candidate_item_layout (const std::vector<uint32> &row_items)
    {
        SCIM_DEBUG_MAIN(4) << __func__ << "\n";

        if (TOOLBAR_HELPER_MODE == m_current_toolbar_mode || m_current_helper_option & ISM_HELPER_PROCESS_KEYBOARD_KEYEVENT) {
            HelperClientIndex::iterator it = m_helper_client_index.find (m_current_helper_uuid);

            if (it != m_helper_client_index.end ()) {
                int    client;
                uint32 context;
                Socket client_socket (it->second.id);
                uint32 ctx;

                get_focused_context (client, context);
                ctx = get_helper_ic (client, context);

                m_send_trans.clear ();
                m_send_trans.put_command (SCIM_TRANS_CMD_REPLY);
                m_send_trans.put_data (ctx);
                m_send_trans.put_data (m_current_helper_uuid);
                m_send_trans.put_command (ISM_TRANS_CMD_UPDATE_CANDIDATE_ITEM_LAYOUT);
                m_send_trans.put_data (row_items);
                m_send_trans.write_to_socket (client_socket);

                return true;
            }
        }

        return false;
    }

    bool helper_select_associate (uint32 item)
    {
        SCIM_DEBUG_MAIN(4) << "PanelAgent::helper_select_associate \n";

        if (TOOLBAR_HELPER_MODE == m_current_toolbar_mode || m_current_helper_option & ISM_HELPER_PROCESS_KEYBOARD_KEYEVENT) {
            HelperClientIndex::iterator it = m_helper_client_index.find (m_current_helper_uuid);

            if (it != m_helper_client_index.end ()) {
                int    client;
                uint32 context;
                Socket client_socket (it->second.id);
                uint32 ctx;

                get_focused_context (client, context);
                ctx = get_helper_ic (client, context);

                m_send_trans.clear ();
                m_send_trans.put_command (SCIM_TRANS_CMD_REPLY);
                m_send_trans.put_data (ctx);
                m_send_trans.put_data (m_current_helper_uuid);
                m_send_trans.put_command (ISM_TRANS_CMD_SELECT_ASSOCIATE);
                m_send_trans.put_data ((uint32)item);
                m_send_trans.write_to_socket (client_socket);

                return true;
            }
        }

        return false;
    }

    bool helper_associate_table_page_up (void)
    {
        SCIM_DEBUG_MAIN(4) << "PanelAgent::helper_associate_table_page_up \n";

        if (TOOLBAR_HELPER_MODE == m_current_toolbar_mode || m_current_helper_option & ISM_HELPER_PROCESS_KEYBOARD_KEYEVENT) {
            HelperClientIndex::iterator it = m_helper_client_index.find (m_current_helper_uuid);

            if (it != m_helper_client_index.end ()) {
                int    client;
                uint32 context;
                Socket client_socket (it->second.id);
                uint32 ctx;

                get_focused_context (client, context);
                ctx = get_helper_ic (client, context);

                m_send_trans.clear ();
                m_send_trans.put_command (SCIM_TRANS_CMD_REPLY);
                m_send_trans.put_data (ctx);
                m_send_trans.put_data (m_current_helper_uuid);
                m_send_trans.put_command (ISM_TRANS_CMD_ASSOCIATE_TABLE_PAGE_UP);
                m_send_trans.write_to_socket (client_socket);

                return true;
            }
        }

        return false;
    }

    bool helper_associate_table_page_down (void)
    {
        SCIM_DEBUG_MAIN(4) << "PanelAgent::helper_associate_table_page_down \n";

        if (TOOLBAR_HELPER_MODE == m_current_toolbar_mode || m_current_helper_option & ISM_HELPER_PROCESS_KEYBOARD_KEYEVENT) {
            HelperClientIndex::iterator it = m_helper_client_index.find (m_current_helper_uuid);

            if (it != m_helper_client_index.end ()) {
                int    client;
                uint32 context;
                Socket client_socket (it->second.id);
                uint32 ctx;

                get_focused_context (client, context);
                ctx = get_helper_ic (client, context);

                m_send_trans.clear ();
                m_send_trans.put_command (SCIM_TRANS_CMD_REPLY);
                m_send_trans.put_data (ctx);
                m_send_trans.put_data (m_current_helper_uuid);
                m_send_trans.put_command (ISM_TRANS_CMD_ASSOCIATE_TABLE_PAGE_DOWN);
                m_send_trans.write_to_socket (client_socket);

                return true;
            }
        }

        return false;
    }

    bool helper_update_associate_table_page_size (uint32         size)
    {
        SCIM_DEBUG_MAIN(4) << "PanelAgent::helper_update_associate_table_page_size \n";

        if (TOOLBAR_HELPER_MODE == m_current_toolbar_mode || m_current_helper_option & ISM_HELPER_PROCESS_KEYBOARD_KEYEVENT) {
            HelperClientIndex::iterator it = m_helper_client_index.find (m_current_helper_uuid);

            if (it != m_helper_client_index.end ()) {
                int    client;
                uint32 context;
                Socket client_socket (it->second.id);
                uint32 ctx;

                get_focused_context (client, context);
                ctx = get_helper_ic (client, context);

                m_send_trans.clear ();
                m_send_trans.put_command (SCIM_TRANS_CMD_REPLY);
                m_send_trans.put_data (ctx);
                m_send_trans.put_data (m_current_helper_uuid);
                m_send_trans.put_command (ISM_TRANS_CMD_UPDATE_ASSOCIATE_TABLE_PAGE_SIZE);
                m_send_trans.put_data (size);
                m_send_trans.write_to_socket (client_socket);

                return true;
            }
        }

        return false;
    }

    bool helper_update_displayed_candidate_number (uint32 size)
    {
        SCIM_DEBUG_MAIN(4) << __func__ << "\n";

        if (TOOLBAR_HELPER_MODE == m_current_toolbar_mode || m_current_helper_option & ISM_HELPER_PROCESS_KEYBOARD_KEYEVENT) {
            HelperClientIndex::iterator it = m_helper_client_index.find (m_current_helper_uuid);

            if (it != m_helper_client_index.end ()) {
                int    client;
                uint32 context;
                Socket client_socket (it->second.id);
                uint32 ctx;

                get_focused_context (client, context);
                ctx = get_helper_ic (client, context);

                m_send_trans.clear ();
                m_send_trans.put_command (SCIM_TRANS_CMD_REPLY);
                m_send_trans.put_data (ctx);
                m_send_trans.put_data (m_current_helper_uuid);
                m_send_trans.put_command (ISM_TRANS_CMD_UPDATE_DISPLAYED_CANDIDATE);
                m_send_trans.put_data (size);
                m_send_trans.write_to_socket (client_socket);

                return true;
            }
        }

        return false;
    }

    bool helper_longpress_candidate (uint32 index)
    {
        SCIM_DEBUG_MAIN(4) << __func__ << "\n";

        if (TOOLBAR_HELPER_MODE == m_current_toolbar_mode || m_current_helper_option & ISM_HELPER_PROCESS_KEYBOARD_KEYEVENT) {
            HelperClientIndex::iterator it = m_helper_client_index.find (m_current_helper_uuid);

            if (it != m_helper_client_index.end ()) {
                int    client;
                uint32 context;
                Socket client_socket (it->second.id);
                uint32 ctx;

                get_focused_context (client, context);
                ctx = get_helper_ic (client, context);

                m_send_trans.clear ();
                m_send_trans.put_command (SCIM_TRANS_CMD_REPLY);
                m_send_trans.put_data (ctx);
                m_send_trans.put_data (m_current_helper_uuid);
                m_send_trans.put_command (ISM_TRANS_CMD_LONGPRESS_CANDIDATE);
                m_send_trans.put_data (index);
                m_send_trans.write_to_socket (client_socket);

                return true;
            }
        }

        std::cerr << __func__ << " is failed!!!\n";
        return false;
    }

    void helper_all_update_spot_location (int x, int y)
    {
        SCIM_DEBUG_MAIN (5) << "PanelAgent::helper_all_update_spot_location (" << x << "," << y << ")\n";

        HelperInfoRepository::iterator hiit = m_helper_info_repository.begin ();

        int    client;
        uint32 context;
        String uuid = get_focused_context (client, context);

        lock ();

        m_send_trans.clear ();
        m_send_trans.put_command (SCIM_TRANS_CMD_REPLY);

        /* FIXME: We presume that client and context are both less than 65536.
         * Hopefully, it should be true in any UNIXs.
         * So it's ok to combine client and context into one uint32. */
        m_send_trans.put_data (get_helper_ic (client, context));
        m_send_trans.put_data (uuid);
        m_send_trans.put_command (SCIM_TRANS_CMD_UPDATE_SPOT_LOCATION);
        m_send_trans.put_data ((uint32) x);
        m_send_trans.put_data ((uint32) y);

        for (; hiit != m_helper_info_repository.end (); ++ hiit) {
            if (hiit->second.option & SCIM_HELPER_NEED_SPOT_LOCATION_INFO) {
                Socket client_socket (hiit->first);
                m_send_trans.write_to_socket (client_socket);
            }
        }

        unlock ();
    }

    void helper_all_update_cursor_position      (int cursor_pos)
    {
        SCIM_DEBUG_MAIN (5) << "PanelAgent::helper_all_update_cursor_position (" << cursor_pos << ")\n";

        HelperInfoRepository::iterator hiit = m_helper_info_repository.begin ();

        int    client;
        uint32 context;
        String uuid = get_focused_context (client, context);

        lock ();

        m_send_trans.clear ();
        m_send_trans.put_command (SCIM_TRANS_CMD_REPLY);
        m_send_trans.put_data (get_helper_ic (client, context));
        m_send_trans.put_data (uuid);
        m_send_trans.put_command (ISM_TRANS_CMD_UPDATE_CURSOR_POSITION);
        m_send_trans.put_data ((uint32) cursor_pos);

        for (; hiit != m_helper_info_repository.end (); ++ hiit) {
            Socket client_socket (hiit->first);
            m_send_trans.write_to_socket (client_socket);
        }

        unlock ();
    }

    void helper_all_update_screen               (int screen)
    {
        SCIM_DEBUG_MAIN (5) << "PanelAgent::helper_all_update_screen (" << screen << ")\n";

        HelperInfoRepository::iterator hiit = m_helper_info_repository.begin ();

        int    client;
        uint32 context;
        String uuid;

        lock ();

        uuid = get_focused_context (client, context);

        m_send_trans.clear ();
        m_send_trans.put_command (SCIM_TRANS_CMD_REPLY);

        /* FIXME: We presume that client and context are both less than 65536.
         * Hopefully, it should be true in any UNIXs.
         * So it's ok to combine client and context into one uint32. */
        m_send_trans.put_data (get_helper_ic (client, context));
        m_send_trans.put_data (uuid);
        m_send_trans.put_command (SCIM_TRANS_CMD_UPDATE_SCREEN);
        m_send_trans.put_data ((uint32) screen);

        for (; hiit != m_helper_info_repository.end (); ++ hiit) {
            if (hiit->second.option & SCIM_HELPER_NEED_SCREEN_INFO) {
                Socket client_socket (hiit->first);
                m_send_trans.write_to_socket (client_socket);
            }
        }

        unlock ();
    }

    const String & get_focused_context (int &client, uint32 &context, bool force_last_context = false) const
    {
        if (m_current_socket_client >= 0) {
            client  = m_current_socket_client;
            context = m_current_client_context;
            return m_current_context_uuid;
        } else {
            client  = m_last_socket_client;
            context = m_last_client_context;
            return m_last_context_uuid;
        }
    }

private:
    void socket_transaction_start (void)
    {
        m_signal_transaction_start ();
    }

    void socket_transaction_end (void)
    {
        m_signal_transaction_end ();
    }

    void lock (void)
    {
        m_signal_lock ();
    }
    void unlock (void)
    {
        m_signal_unlock ();
    }
};

PanelAgent::PanelAgent ()
    : m_impl (new PanelAgentImpl ())
{
}

PanelAgent::~PanelAgent ()
{
    delete m_impl;
}

bool
PanelAgent::initialize (const String &config, const String &display, bool resident)
{
    return m_impl->initialize (config, display, resident);
}

bool
PanelAgent::valid (void) const
{
    return m_impl->valid ();
}

bool
PanelAgent::run (void)
{
    return m_impl->run ();
}

void
PanelAgent::stop (void)
{
    if (m_impl != NULL)
        m_impl->stop ();
}

int
PanelAgent::get_helper_list (std::vector <HelperInfo> & helpers) const
{
    return m_impl->get_helper_list (helpers);
}

void PanelAgent::hide_helper (const String &uuid)
{
    m_impl->hide_helper (uuid);
}
TOOLBAR_MODE_T
PanelAgent::get_current_toolbar_mode () const
{
    return m_impl->get_current_toolbar_mode ();
}

void
PanelAgent::get_current_ise_geometry (rectinfo &rect)
{
    m_impl->get_current_ise_geometry (rect);
}

String
PanelAgent::get_current_helper_uuid () const
{
    return m_impl->get_current_helper_uuid ();
}

String
PanelAgent::get_current_helper_name () const
{
    return m_impl->get_current_helper_name ();
}

String
PanelAgent::get_current_ise_name () const
{
    return m_impl->get_current_ise_name ();
}

void
PanelAgent::set_current_toolbar_mode (TOOLBAR_MODE_T mode)
{
    m_impl->set_current_toolbar_mode (mode);
}

void
PanelAgent::set_current_ise_name (String &name)
{
    m_impl->set_current_ise_name (name);
}

void
PanelAgent::set_current_helper_option (uint32 option)
{
    m_impl->set_current_helper_option (option);
}

void
PanelAgent::update_candidate_panel_event (uint32 nType, uint32 nValue)
{
    m_impl->update_panel_event (ISM_TRANS_CMD_UPDATE_ISF_CANDIDATE_PANEL, nType, nValue);
}

void
PanelAgent::update_input_panel_event (uint32 nType, uint32 nValue)
{
    m_impl->update_panel_event (ISM_TRANS_CMD_UPDATE_ISE_INPUT_CONTEXT, nType, nValue);
}

bool
PanelAgent::move_preedit_caret             (uint32         position)
{
    return m_impl->move_preedit_caret (position);
}

bool
PanelAgent::request_help                   (void)
{
    return m_impl->request_help ();
}

bool
PanelAgent::request_factory_menu           (void)
{
    return m_impl->request_factory_menu ();
}

bool
PanelAgent::change_factory                 (const String  &uuid)
{
    return m_impl->change_factory (uuid);
}

bool
PanelAgent::helper_candidate_show          (void)
{
    return m_impl->helper_candidate_show ();
}

bool
PanelAgent::helper_candidate_hide          (void)
{
    return m_impl->helper_candidate_hide ();
}

bool
PanelAgent::candidate_more_window_show     (void)
{
    return m_impl->candidate_more_window_show ();
}

bool
PanelAgent::candidate_more_window_hide     (void)
{
    return m_impl->candidate_more_window_hide ();
}

bool
PanelAgent::update_helper_lookup_table     (const LookupTable &table)
{
    return m_impl->update_helper_lookup_table (table);
}

bool
PanelAgent::select_aux                     (uint32         item)
{
    return m_impl->select_aux (item);
}

bool
PanelAgent::select_candidate               (uint32         item)
{
    return m_impl->select_candidate (item);
}

bool
PanelAgent::lookup_table_page_up           (void)
{
    return m_impl->lookup_table_page_up ();
}

bool
PanelAgent::lookup_table_page_down         (void)
{
    return m_impl->lookup_table_page_down ();
}

bool
PanelAgent::update_lookup_table_page_size  (uint32         size)
{
    return m_impl->update_lookup_table_page_size (size);
}

bool
PanelAgent::update_candidate_item_layout   (const std::vector<uint32> &row_items)
{
    return m_impl->update_candidate_item_layout (row_items);
}

bool
PanelAgent::select_associate               (uint32         item)
{
    return m_impl->select_associate (item);
}

bool
PanelAgent::associate_table_page_up        (void)
{
    return m_impl->associate_table_page_up ();
}

bool
PanelAgent::associate_table_page_down      (void)
{
    return m_impl->associate_table_page_down ();
}

bool
PanelAgent::update_associate_table_page_size (uint32         size)
{
    return m_impl->update_associate_table_page_size (size);
}

bool
PanelAgent::update_displayed_candidate_number (uint32        size)
{
    return m_impl->update_displayed_candidate_number (size);
}

void
PanelAgent::send_longpress_event           (int type, int index)
{
    m_impl->send_longpress_event (type, index);
}

bool
PanelAgent::trigger_property               (const String  &property)
{
    return m_impl->trigger_property (property);
}

bool
PanelAgent::trigger_helper_property        (int            client,
                                            const String  &property)
{
    return m_impl->trigger_helper_property (client, property);
}

bool
PanelAgent::start_helper                   (const String  &uuid)
{
    return m_impl->start_helper (uuid, -2, 0);
}

bool
PanelAgent::stop_helper                    (const String  &uuid)
{
    return m_impl->stop_helper (uuid, -2, 0);
}

void
PanelAgent::set_default_ise                (const DEFAULT_ISE_T  &ise)
{
    m_impl->set_default_ise (ise);
}

void
PanelAgent::set_should_shared_ise          (const bool should_shared_ise)
{
    m_impl->set_should_shared_ise (should_shared_ise);
}

bool
PanelAgent::reset_keyboard_ise             (void) const
{
    return m_impl->reset_keyboard_ise ();
}

int
PanelAgent::get_active_ise_list            (std::vector<String> &strlist)
{
    return m_impl->get_active_ise_list (strlist);
}

int
PanelAgent::get_helper_manager_id          (void)
{
    return m_impl->get_helper_manager_id ();
}

bool
PanelAgent::has_helper_manager_pending_event (void)
{
    return m_impl->has_helper_manager_pending_event ();
}

bool
PanelAgent::filter_helper_manager_event    (void)
{
    return m_impl->filter_helper_manager_event ();
}

int
PanelAgent::send_display_name              (String &name)
{
    return m_impl->send_display_name (name);
}

bool
PanelAgent::reload_config                  (void)
{
    return m_impl->reload_config ();
}

bool
PanelAgent::exit                           (void)
{
    return m_impl->exit ();
}

bool
PanelAgent::filter_event (int fd)
{
    return m_impl->filter_event (fd);
}

bool
PanelAgent::filter_exception_event (int fd)
{
    return m_impl->filter_exception_event (fd);
}

int
PanelAgent::get_server_id (void)
{
    return m_impl->get_server_id ();
}

void
PanelAgent::update_ise_list (std::vector<String> &strList)
{
    m_impl->update_ise_list (strList);
}

Connection
PanelAgent::signal_connect_reload_config              (PanelAgentSlotVoid                *slot)
{
    return m_impl->signal_connect_reload_config (slot);
}

Connection
PanelAgent::signal_connect_turn_on                    (PanelAgentSlotVoid                *slot)
{
    return m_impl->signal_connect_turn_on (slot);
}

Connection
PanelAgent::signal_connect_turn_off                   (PanelAgentSlotVoid                *slot)
{
    return m_impl->signal_connect_turn_off (slot);
}

Connection
PanelAgent::signal_connect_show_panel                 (PanelAgentSlotVoid                *slot)
{
    return m_impl->signal_connect_show_panel (slot);
}

Connection
PanelAgent::signal_connect_hide_panel                 (PanelAgentSlotVoid                *slot)
{
    return m_impl->signal_connect_hide_panel (slot);
}

Connection
PanelAgent::signal_connect_update_screen              (PanelAgentSlotInt                 *slot)
{
    return m_impl->signal_connect_update_screen (slot);
}

Connection
PanelAgent::signal_connect_update_spot_location       (PanelAgentSlotIntIntInt           *slot)
{
    return m_impl->signal_connect_update_spot_location (slot);
}

Connection
PanelAgent::signal_connect_update_factory_info        (PanelAgentSlotFactoryInfo         *slot)
{
    return m_impl->signal_connect_update_factory_info (slot);
}

Connection
PanelAgent::signal_connect_start_default_ise          (PanelAgentSlotVoid                *slot)
{
    return m_impl->signal_connect_start_default_ise (slot);
}

Connection
PanelAgent::signal_connect_stop_default_ise           (PanelAgentSlotVoid                *slot)
{
    return m_impl->signal_connect_stop_default_ise (slot);
}

Connection
PanelAgent::signal_connect_set_candidate_ui           (PanelAgentSlotIntInt              *slot)
{
    return m_impl->signal_connect_set_candidate_ui (slot);
}

Connection
PanelAgent::signal_connect_get_candidate_ui           (PanelAgentSlotIntInt2             *slot)
{
    return m_impl->signal_connect_get_candidate_ui (slot);
}

Connection
PanelAgent::signal_connect_set_candidate_position     (PanelAgentSlotIntInt              *slot)
{
    return m_impl->signal_connect_set_candidate_position (slot);
}

Connection
PanelAgent::signal_connect_get_candidate_geometry     (PanelAgentSlotRect                *slot)
{
    return m_impl->signal_connect_get_candidate_geometry (slot);
}

Connection
PanelAgent::signal_connect_get_input_panel_geometry   (PanelAgentSlotRect                *slot)
{
    return m_impl->signal_connect_get_input_panel_geometry (slot);
}

Connection
PanelAgent::signal_connect_set_keyboard_ise           (PanelAgentSlotString              *slot)
{
    return m_impl->signal_connect_set_keyboard_ise (slot);
}

Connection
PanelAgent::signal_connect_get_keyboard_ise           (PanelAgentSlotString2             *slot)
{
    return m_impl->signal_connect_get_keyboard_ise (slot);
}

Connection
PanelAgent::signal_connect_show_help                  (PanelAgentSlotString              *slot)
{
    return m_impl->signal_connect_show_help (slot);
}

Connection
PanelAgent::signal_connect_show_factory_menu          (PanelAgentSlotFactoryInfoVector   *slot)
{
    return m_impl->signal_connect_show_factory_menu (slot);
}

Connection
PanelAgent::signal_connect_show_preedit_string        (PanelAgentSlotVoid                *slot)
{
    return m_impl->signal_connect_show_preedit_string (slot);
}

Connection
PanelAgent::signal_connect_show_aux_string            (PanelAgentSlotVoid                *slot)
{
    return m_impl->signal_connect_show_aux_string (slot);
}

Connection
PanelAgent::signal_connect_show_lookup_table          (PanelAgentSlotVoid                *slot)
{
    return m_impl->signal_connect_show_lookup_table (slot);
}

Connection
PanelAgent::signal_connect_show_associate_table       (PanelAgentSlotVoid                *slot)
{
    return m_impl->signal_connect_show_associate_table (slot);
}

Connection
PanelAgent::signal_connect_hide_preedit_string        (PanelAgentSlotVoid                *slot)
{
    return m_impl->signal_connect_hide_preedit_string (slot);
}

Connection
PanelAgent::signal_connect_hide_aux_string            (PanelAgentSlotVoid                *slot)
{
    return m_impl->signal_connect_hide_aux_string (slot);
}

Connection
PanelAgent::signal_connect_hide_lookup_table          (PanelAgentSlotVoid                *slot)
{
    return m_impl->signal_connect_hide_lookup_table (slot);
}

Connection
PanelAgent::signal_connect_hide_associate_table       (PanelAgentSlotVoid                *slot)
{
    return m_impl->signal_connect_hide_associate_table (slot);
}

Connection
PanelAgent::signal_connect_update_preedit_string      (PanelAgentSlotAttributeStringInt  *slot)
{
    return m_impl->signal_connect_update_preedit_string (slot);
}

Connection
PanelAgent::signal_connect_update_preedit_caret       (PanelAgentSlotInt                 *slot)
{
    return m_impl->signal_connect_update_preedit_caret (slot);
}

Connection
PanelAgent::signal_connect_update_aux_string          (PanelAgentSlotAttributeString     *slot)
{
    return m_impl->signal_connect_update_aux_string (slot);
}

Connection
PanelAgent::signal_connect_update_lookup_table        (PanelAgentSlotLookupTable         *slot)
{
    return m_impl->signal_connect_update_lookup_table (slot);
}

Connection
PanelAgent::signal_connect_update_associate_table     (PanelAgentSlotLookupTable         *slot)
{
    return m_impl->signal_connect_update_associate_table (slot);
}

Connection
PanelAgent::signal_connect_register_properties        (PanelAgentSlotPropertyList        *slot)
{
    return m_impl->signal_connect_register_properties (slot);
}

Connection
PanelAgent::signal_connect_update_property            (PanelAgentSlotProperty            *slot)
{
    return m_impl->signal_connect_update_property (slot);
}

Connection
PanelAgent::signal_connect_register_helper_properties (PanelAgentSlotIntPropertyList     *slot)
{
    return m_impl->signal_connect_register_helper_properties (slot);
}

Connection
PanelAgent::signal_connect_update_helper_property     (PanelAgentSlotIntProperty         *slot)
{
    return m_impl->signal_connect_update_helper_property (slot);
}

Connection
PanelAgent::signal_connect_register_helper            (PanelAgentSlotIntHelperInfo       *slot)
{
    return m_impl->signal_connect_register_helper (slot);
}

Connection
PanelAgent::signal_connect_remove_helper              (PanelAgentSlotInt                 *slot)
{
    return m_impl->signal_connect_remove_helper (slot);
}

Connection
PanelAgent::signal_connect_set_active_ise_by_uuid     (PanelAgentSlotStringBool              *slot)
{
    return m_impl->signal_connect_set_active_ise_by_uuid (slot);
}

Connection
PanelAgent::signal_connect_focus_in                   (PanelAgentSlotVoid                *slot)
{
    return m_impl->signal_connect_focus_in (slot);
}

Connection
PanelAgent::signal_connect_focus_out                  (PanelAgentSlotVoid                *slot)
{
    return m_impl->signal_connect_focus_out (slot);
}

Connection
PanelAgent::signal_connect_expand_candidate           (PanelAgentSlotVoid                *slot)
{
    return m_impl->signal_connect_expand_candidate (slot);
}

Connection
PanelAgent::signal_connect_contract_candidate         (PanelAgentSlotVoid                *slot)
{
    return m_impl->signal_connect_contract_candidate (slot);
}

Connection
PanelAgent::signal_connect_select_candidate           (PanelAgentSlotInt                 *slot)
{
    return m_impl->signal_connect_select_candidate (slot);
}

Connection
PanelAgent::signal_connect_get_ise_list               (PanelAgentSlotBoolStringVector    *slot)
{
    return m_impl->signal_connect_get_ise_list (slot);
}

Connection
PanelAgent::signal_connect_get_ise_information        (PanelAgentSlotBoolString4int2     *slot)
{
    return m_impl->signal_connect_get_ise_information (slot);
}

Connection
PanelAgent::signal_connect_get_keyboard_ise_list      (PanelAgentSlotBoolStringVector    *slot)
{
    return m_impl->signal_connect_get_keyboard_ise_list (slot);
}

Connection
PanelAgent::signal_connect_update_ise_geometry        (PanelAgentSlotIntIntIntInt        *slot)
{
    return m_impl->signal_connect_update_ise_geometry (slot);
}

Connection
PanelAgent::signal_connect_get_language_list          (PanelAgentSlotStringVector        *slot)
{
    return m_impl->signal_connect_get_language_list (slot);
}

Connection
PanelAgent::signal_connect_get_all_language           (PanelAgentSlotStringVector        *slot)
{
    return m_impl->signal_connect_get_all_language (slot);
}

Connection
PanelAgent::signal_connect_get_ise_language           (PanelAgentSlotStrStringVector     *slot)
{
    return m_impl->signal_connect_get_ise_language (slot);
}

Connection
PanelAgent::signal_connect_get_ise_info_by_uuid       (PanelAgentSlotStringISEINFO       *slot)
{
    return m_impl->signal_connect_get_ise_info_by_uuid (slot);
}

Connection
PanelAgent::signal_connect_send_key_event             (PanelAgentSlotKeyEvent            *slot)
{
    return m_impl->signal_connect_send_key_event (slot);
}

Connection
PanelAgent::signal_connect_accept_connection          (PanelAgentSlotInt                 *slot)
{
    return m_impl->signal_connect_accept_connection (slot);
}

Connection
PanelAgent::signal_connect_close_connection           (PanelAgentSlotInt                 *slot)
{
    return m_impl->signal_connect_close_connection (slot);
}

Connection
PanelAgent::signal_connect_exit                       (PanelAgentSlotVoid                *slot)
{
    return m_impl->signal_connect_exit (slot);
}

Connection
PanelAgent::signal_connect_transaction_start          (PanelAgentSlotVoid                *slot)
{
    return m_impl->signal_connect_transaction_start (slot);
}

Connection
PanelAgent::signal_connect_transaction_end            (PanelAgentSlotVoid                *slot)
{
    return m_impl->signal_connect_transaction_end (slot);
}

Connection
PanelAgent::signal_connect_lock                       (PanelAgentSlotVoid                *slot)
{
    return m_impl->signal_connect_lock (slot);
}

Connection
PanelAgent::signal_connect_unlock                     (PanelAgentSlotVoid                *slot)
{
    return m_impl->signal_connect_unlock (slot);
}

Connection
PanelAgent::signal_connect_update_input_context       (PanelAgentSlotIntInt              *slot)
{
    return m_impl->signal_connect_update_input_context (slot);
}

Connection
PanelAgent::signal_connect_show_ise                   (PanelAgentSlotVoid                *slot)
{
    return m_impl->signal_connect_show_ise (slot);
}

Connection
PanelAgent::signal_connect_hide_ise                   (PanelAgentSlotVoid                *slot)
{
    return m_impl->signal_connect_hide_ise (slot);
}

Connection
PanelAgent::signal_connect_will_show_ack              (PanelAgentSlotVoid                *slot)
{
    return m_impl->signal_connect_will_show_ack (slot);
}

Connection
PanelAgent::signal_connect_will_hide_ack              (PanelAgentSlotVoid                *slot)
{
    return m_impl->signal_connect_will_hide_ack (slot);
}

Connection
PanelAgent::signal_connect_set_keyboard_mode (PanelAgentSlotInt                *slot)
{
    return m_impl->signal_connect_set_keyboard_mode (slot);
}

Connection
PanelAgent::signal_connect_candidate_will_hide_ack    (PanelAgentSlotVoid                *slot)
{
    return m_impl->signal_connect_candidate_will_hide_ack (slot);
}

Connection
PanelAgent::signal_connect_get_ise_state              (PanelAgentSlotInt2                *slot)
{
    return m_impl->signal_connect_get_ise_state (slot);
}

} /* namespace scim */

/*
vi:ts=4:nowrap:ai:expandtab
*/

