/** @file scim_socket_frontend.cpp
 * implementation of class SocketFrontEnd.
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
 * $Id: scim_socket_frontend.cpp,v 1.37 2005/07/03 08:36:42 suzhe Exp $
 *
 */

#define Uses_SCIM_CONFIG_PATH
#define Uses_SCIM_FRONTEND
#define Uses_SCIM_SOCKET
#define Uses_SCIM_TRANSACTION
#define Uses_STL_UTILITY
#define Uses_C_STDIO
#define Uses_C_STDLIB
#define Uses_SCIM_CONFIG_BASE
#define Uses_SCIM_UTILITY
#define Uses_SCIM_IMENGINE_MODULE
#define Uses_SCIM_PANEL_AGENT

#include <string.h>

#include <limits.h>
#include "scim_private.h"
#include "scim.h"
#include "scim_socket_frontend.h"
#include "scim_helper.h"
#include "scim_helper_module.h"
#include "isf_query_utility.h"
#include <sys/time.h>
#include <sys/times.h>
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>
#include <dlog.h>
#if HAVE_PKGMGR_INFO
#include <pkgmgr-info.h>
#endif

#ifdef LOG_TAG
# undef LOG_TAG
#endif
#define LOG_TAG                                        "ISF_SOCKET_FRONTEND"


#define scim_module_init socket_LTX_scim_module_init
#define scim_module_exit socket_LTX_scim_module_exit
#define scim_frontend_module_init socket_LTX_scim_frontend_module_init
#define scim_frontend_module_run socket_LTX_scim_frontend_module_run

#define SCIM_CONFIG_FRONTEND_SOCKET_CONFIG_READONLY    "/FrontEnd/Socket/ConfigReadOnly"
#define SCIM_CONFIG_FRONTEND_SOCKET_MAXCLIENTS         "/FrontEnd/Socket/MaxClients"


using namespace scim;

static Pointer <SocketFrontEnd> _scim_frontend (0);

static int      _argc;
static char   **_argv = NULL;

//Module Interface
extern "C" {
    EAPI void scim_module_init (void)
    {
        SCIM_DEBUG_FRONTEND (1) << "Initializing Socket FrontEnd module...\n";
    }

    EAPI void scim_module_exit (void)
    {
        SCIM_DEBUG_FRONTEND (1) << "Exiting Socket FrontEnd module...\n";
        _scim_frontend.reset ();
    }

    EAPI void scim_frontend_module_init (const BackEndPointer &backend,
                                    const ConfigPointer &config,
                                    int argc,
                                    char **argv)
    {
        if (_scim_frontend.null ()) {
            SCIM_DEBUG_FRONTEND (1) << "Initializing Socket FrontEnd module (more)...\n";
            _scim_frontend = new SocketFrontEnd (backend, config);
            _argc = argc;
            _argv = argv;
        }
    }

    EAPI void scim_frontend_module_run (void)
    {
        struct tms tiks_buf;
        clock_t start = times (&tiks_buf);
        if (!_scim_frontend.null ()) {
            SCIM_DEBUG_FRONTEND (1) << "Starting Socket FrontEnd module...\n";
            _scim_frontend->init (_argc, _argv);
            gettime (start, "Init socket frontend");
            _scim_frontend->run ();
        }
    }
}

/**
 * To reduce the number of the servers, we merge the function of
 * HelperManager process into the SocketFrontEnd.
*/

typedef std::vector < std::pair <HelperInfo, String> >                      HelperRepository;

static HelperRepository     __helpers;
static std::vector<String>  __load_engine_list;

void SocketFrontEnd::load_helper_modules (const std::vector<String> &load_engine_list)
{
    SCIM_DEBUG_MAIN (1) << "load_helper_modules ()\n";

    size_t i = 0;

    __load_engine_list.clear ();
    for (i = 0; i < load_engine_list.size (); ++i)
        __load_engine_list.push_back (load_engine_list [i]);

    std::vector <String> mod_list;
    std::vector <String> all_helper_list;

    scim_get_helper_module_list (all_helper_list);

    // Get the helper module list which should be loaded.
    if (load_engine_list.size () && all_helper_list.size ()) {
        for (i = 0; i < all_helper_list.size (); ++i) {
            if (std::find (load_engine_list.begin (),
                           load_engine_list.end (),
                           all_helper_list [i]) != load_engine_list.end ())
                mod_list.push_back (all_helper_list [i]);
        }
    }

    if (mod_list.size ()) {
        HelperInfo           info;
        std::vector<ISEINFO> info_list;
        std::vector<String>  tmp_list;
        bool ret = isf_read_ise_info_list (USER_ENGINE_FILE_NAME, info_list);
        if (!ret) {
            std::cerr << __func__ << " Failed to read(" << USER_ENGINE_FILE_NAME << ")\n";
        }
        for (i = 0; i < info_list.size (); ++i) {
            if (info_list [i].mode != TOOLBAR_HELPER_MODE)
                continue;
            if (std::find (mod_list.begin (), mod_list.end (), info_list [i].module) != mod_list.end ()) {
                info.uuid   = info_list [i].uuid;
                info.name   = info_list [i].name;
                info.icon   = info_list [i].icon;
                info.option = info_list [i].option;
                SCIM_DEBUG_MAIN (3) << "  " << info.uuid << ": " << info.name << "\n";
                __helpers.push_back (std::make_pair (info, info_list [i].module));
                tmp_list.push_back (info_list [i].module);
            }
        }

        HelperModule module;
        String filename = String (USER_ENGINE_FILE_NAME);
        FILE *engine_list_file = fopen (filename.c_str (), "a");
        if (!engine_list_file) {
            LOGW ("Failed to open %s!!!\n", filename.c_str ());
        }

        for (i = 0; i < mod_list.size (); ++i) {
            if (std::find (tmp_list.begin (), tmp_list.end (), mod_list [i]) != tmp_list.end ())
                continue;

            SCIM_DEBUG_MAIN (2) << " Load module: " << mod_list [i] << "\n";

            if (module.load (mod_list [i]) && module.valid ()) {
                size_t num = module.number_of_helpers ();

                SCIM_DEBUG_MAIN (2) << " Find " << num << " Helpers:\n";

                for (size_t j = 0; j < num; ++j) {
                    if (module.get_helper_info (j, info)) {
                        SCIM_DEBUG_MAIN (3) << "  " << info.uuid << ": " << info.name << "\n";
                        __helpers.push_back (std::make_pair (info, mod_list [i]));

                        if (engine_list_file) {
                            char mode[12];
                            char option[12];
                            snprintf (mode, sizeof (mode), "%d", (int)TOOLBAR_HELPER_MODE);
                            snprintf (option, sizeof (option), "%d", info.option);

                            String line = isf_combine_ise_info_string (info.name, info.uuid, mod_list [i], isf_get_normalized_language (module.get_helper_lang (j)),
                                                                       info.icon, String (mode), String (option), String (""));
                            if (fputs (line.c_str (), engine_list_file) < 0)
                                LOGW ("Failed to write (%s)!!!\n", line.c_str ());
                        }
                    }
                }
            }

            module.unload ();
        }
        if (engine_list_file) {
            int iret = fclose (engine_list_file);
            if (iret != 0)
                LOGW ("Failed to fclose %s!!!\n", filename.c_str ());
        }
    }
}

void SocketFrontEnd::get_helper_list (const Socket &client)
{
    HelperRepository::iterator it = __helpers.begin ();

    m_send_trans.clear ();
    m_send_trans.put_command (SCIM_TRANS_CMD_REPLY);
    m_send_trans.put_data ((uint32)__helpers.size ());

    for (; it != __helpers.end (); ++it) {
        m_send_trans.put_data (it->first.uuid);
        m_send_trans.put_data (it->first.name);
        m_send_trans.put_data (it->first.icon);
        m_send_trans.put_data (it->first.description);
        m_send_trans.put_data (it->first.option);
    }
}

#ifndef SCIM_HELPER_LAUNCHER_PROGRAM
  #define SCIM_HELPER_LAUNCHER_PROGRAM  (SCIM_LIBEXECDIR "/scim-helper-launcher")
#endif
static std::vector<String>  __active_helpers;

void SocketFrontEnd::run_helper (const Socket &client)
{
    String uuid;
    String config;
    String display;
    if (!(m_receive_trans.get_data (uuid) && uuid.length ()
             && m_receive_trans.get_data (config)
             && m_receive_trans.get_data (display)))
    {
        m_send_trans.put_command (SCIM_TRANS_CMD_FAIL);
        return;
    }
    ISF_SAVE_LOG ("uuid(%s)\n", uuid.c_str ());

    for (size_t i = 0; i < __helpers.size (); ++i) {
        if (__helpers [i].first.uuid == uuid && __helpers [i].second.length ()) {

            __active_helpers.push_back (__helpers [i].first.name);

            int pid;

            pid = fork ();

            if (pid < 0) return;

            if (pid == 0) {

                const char *argv [] = { SCIM_HELPER_LAUNCHER_PROGRAM,
                                   "--daemon",
                                   "--config", const_cast<char*> (config.c_str ()),
                                   "--display", const_cast<char*> (display.c_str ()),
                                   const_cast<char*> (__helpers [i].second.c_str ()),
                                   const_cast<char*> (__helpers [i].first.uuid.c_str ()),
                                   0};

                SCIM_DEBUG_MAIN (2) << " Call scim-helper-launcher.\n";
                ISF_SAVE_LOG ("Exec scim_helper_launcher(%s)\n", __helpers [i].second.c_str ());

                execv (SCIM_HELPER_LAUNCHER_PROGRAM, const_cast<char **>(argv));
                exit (-1);
            }

            //int status;
            //waitpid (pid, &status, 0);

            break;
        } else {
            ISF_SAVE_LOG ("Can't find and exec scim_helper_launcher uuid : %s\n", uuid.c_str ());
        }
    }

    m_send_trans.put_command (SCIM_TRANS_CMD_OK);
    SCIM_DEBUG_MAIN (2) << " exit run_helper ().\n";
}

/**
 * get_active_ise_list
 * Get the active ise list. we can control the loading of ISEs through
 * the command option "-e" of scim process.
 */
void
SocketFrontEnd::get_active_ise_list (int clientid)
{
    m_send_trans.put_data (__load_engine_list);
}

void
SocketFrontEnd::preload_keyboard_ise (const String &uuid)
{
    SCIM_DEBUG_MAIN (1) << "preload_keyboard_ise ()\n";

    if (uuid.length () == 0)
        return;

    if (m_preload_keyboard_ise_id != -1)
    {
        if (get_instance_uuid (m_preload_keyboard_ise_id) == uuid)
            return;
        delete_instance (m_preload_keyboard_ise_id);
    }

    m_preload_keyboard_ise_id = new_instance (m_config, uuid, "UTF-8");
}

void
SocketFrontEnd::unregister_helper ()
{
    String name;
    if (m_receive_trans.get_data (name) && name.length ())
    {
        std::vector<String>::iterator iter = __active_helpers.begin ();
        for (; iter != __active_helpers.end (); iter++)
        {
            if (!name.compare (*iter))
            {
                __active_helpers.erase (iter);
                break;
            }
        }
    }

    m_send_trans.put_command (SCIM_TRANS_CMD_OK);
}

SocketFrontEnd::SocketFrontEnd (const BackEndPointer &backend,
                                const ConfigPointer  &config)
    : FrontEndBase (backend),
      m_config (config),
      m_continue_mark (STATUS_NONE),
      m_stay (true),
      m_config_readonly (false),
      m_socket_timeout (scim_get_default_socket_timeout ()),
      m_current_instance (-1),
      m_current_socket_client (-1),
      m_preload_keyboard_ise_id (-1),
      m_current_socket_client_key (0)
{
    SCIM_DEBUG_FRONTEND (2) << " Constructing SocketFrontEnd object...\n";
}

SocketFrontEnd::~SocketFrontEnd ()
{
    SCIM_DEBUG_FRONTEND (2) << " Destructing SocketFrontEnd object...\n";
    if (m_socket_server.is_running ())
        m_socket_server.shutdown ();
}

void
SocketFrontEnd::show_preedit_string (int id)
{
    if (m_current_instance == id)
        m_send_trans.put_command (SCIM_TRANS_CMD_SHOW_PREEDIT_STRING);
}

void
SocketFrontEnd::show_aux_string (int id)
{
    if (m_current_instance == id)
        m_send_trans.put_command (SCIM_TRANS_CMD_SHOW_AUX_STRING);
}

void
SocketFrontEnd::show_lookup_table (int id)
{
    if (m_current_instance == id)
        m_send_trans.put_command (SCIM_TRANS_CMD_SHOW_LOOKUP_TABLE);
}

void
SocketFrontEnd::hide_preedit_string (int id)
{
    if (m_current_instance == id)
        m_send_trans.put_command (SCIM_TRANS_CMD_HIDE_PREEDIT_STRING);
}

void
SocketFrontEnd::hide_aux_string (int id)
{
    if (m_current_instance == id)
        m_send_trans.put_command (SCIM_TRANS_CMD_HIDE_AUX_STRING);
}

void
SocketFrontEnd::hide_lookup_table (int id)
{
    if (m_current_instance == id)
        m_send_trans.put_command (SCIM_TRANS_CMD_HIDE_LOOKUP_TABLE);
}

void
SocketFrontEnd::update_preedit_caret (int id, int caret)
{
    if (m_current_instance == id) {
        m_send_trans.put_command (SCIM_TRANS_CMD_UPDATE_PREEDIT_CARET);
        m_send_trans.put_data ((uint32) caret);
    }
}

void
SocketFrontEnd::update_preedit_string (int id,
                                       const WideString & str,
                                       const AttributeList & attrs,
                                       int caret)
{
    if (m_current_instance == id) {
        m_send_trans.put_command (SCIM_TRANS_CMD_UPDATE_PREEDIT_STRING);
        m_send_trans.put_data (str);
        m_send_trans.put_data (attrs);
        m_send_trans.put_data ((uint32) caret);
    }
}

void
SocketFrontEnd::update_aux_string (int id,
                                   const WideString & str,
                                   const AttributeList & attrs)
{
    if (m_current_instance == id) {
        m_send_trans.put_command (SCIM_TRANS_CMD_UPDATE_AUX_STRING);
        m_send_trans.put_data (str);
        m_send_trans.put_data (attrs);
    }
}

void
SocketFrontEnd::commit_string (int id, const WideString & str)
{
    if (m_current_instance == id) {
        m_send_trans.put_command (SCIM_TRANS_CMD_COMMIT_STRING);
        m_send_trans.put_data (str);
    }
}

void
SocketFrontEnd::forward_key_event (int id, const KeyEvent & key)
{
    if (m_current_instance == id) {
        m_send_trans.put_command (SCIM_TRANS_CMD_FORWARD_KEY_EVENT);
        m_send_trans.put_data (key);
    }
}

void
SocketFrontEnd::update_lookup_table (int id, const LookupTable & table)
{
    if (m_current_instance == id) {
        m_send_trans.put_command (SCIM_TRANS_CMD_UPDATE_LOOKUP_TABLE);
        m_send_trans.put_data (table);
    }
}

void
SocketFrontEnd::register_properties (int id, const PropertyList &properties)
{
    if (m_current_instance == id) {
        m_send_trans.put_command (SCIM_TRANS_CMD_REGISTER_PROPERTIES);
        m_send_trans.put_data (properties);
    }
}

void
SocketFrontEnd::update_property (int id, const Property &property)
{
    if (m_current_instance == id) {
        m_send_trans.put_command (SCIM_TRANS_CMD_UPDATE_PROPERTY);
        m_send_trans.put_data (property);
    }
}

void
SocketFrontEnd::beep (int id)
{
    if (m_current_instance == id) {
        m_send_trans.put_command (SCIM_TRANS_CMD_BEEP);
    }
}

void
SocketFrontEnd::start_helper (int id, const String &helper_uuid)
{
    SCIM_DEBUG_FRONTEND (2) << "start_helper (" << helper_uuid << ")\n";
    if (m_current_instance == id) {
        m_send_trans.put_command (SCIM_TRANS_CMD_START_HELPER);
        m_send_trans.put_data (helper_uuid);
    }
}

void
SocketFrontEnd::stop_helper (int id, const String &helper_uuid)
{
    SCIM_DEBUG_FRONTEND (2) << "stop_helper (" << helper_uuid << ")\n";

    if (m_current_instance == id) {
        m_send_trans.put_command (SCIM_TRANS_CMD_STOP_HELPER);
        m_send_trans.put_data (helper_uuid);
    }
}

void
SocketFrontEnd::send_helper_event (int id, const String &helper_uuid, const Transaction &trans)
{
    if (m_current_instance == id) {
        m_send_trans.put_command (SCIM_TRANS_CMD_SEND_HELPER_EVENT);
        m_send_trans.put_data (helper_uuid);
        m_send_trans.put_data (trans);
    }
}

bool
SocketFrontEnd::get_surrounding_text (int id, WideString &text, int &cursor, int maxlen_before, int maxlen_after)
{
    bool ret = false;
    text.clear ();
    cursor = 0;

    if (m_current_instance == id && m_current_socket_client >= 0 && (maxlen_before != 0 || maxlen_after != 0)) {
        bool cont = false;
        if (maxlen_before < 0) maxlen_before = -1;
        if (maxlen_after < 0) maxlen_after = -1;
        Socket socket_client (m_current_socket_client);
        if (m_send_trans.get_data_type () != SCIM_TRANS_DATA_UNKNOWN) {
            m_send_trans.put_command (ISM_TRANS_CMD_TRANSACTION_CONTINUE);
            m_send_trans.write_to_socket (socket_client);
            cont = true;
            /**
             * After flushing transaction to APP, frontend maybe receives some Requests,
             * receive and deal with this Requests until get ISM_TRANS_CMD_TRANSACTION_CONTINUE reply
             */
            while (true) {
                /**
                 * socket_receive_callback() will overwrite m_receive_trans,
                 * but in previous frame of stack, m_receive_trans has not finished reading,
                 */
                Transaction _tran;
                _tran = m_receive_trans;
                m_continue_mark = STATUS_NONE;
                socket_receive_callback (&m_socket_server, socket_client);
                m_receive_trans = _tran;
                ContinueMark status = get_continue_mark ();
                if (status == STATUS_DISCONNECT)
                    return false;
                else if (status == STATUS_CONTINUE)
                    break;
            }
        }

        m_temp_trans.clear ();
        m_temp_trans.put_command (SCIM_TRANS_CMD_REPLY);
        m_temp_trans.put_command (SCIM_TRANS_CMD_GET_SURROUNDING_TEXT);
        m_temp_trans.put_data ((uint32) maxlen_before);
        m_temp_trans.put_data ((uint32) maxlen_after);

        if (m_temp_trans.write_to_socket (socket_client) &&
            m_temp_trans.read_from_socket (socket_client, m_socket_timeout)) {

            int cmd;
            uint32 key;
            uint32 cur;

            if (m_temp_trans.get_command (cmd) && cmd == SCIM_TRANS_CMD_REQUEST &&
                m_temp_trans.get_data (key) && key == m_current_socket_client_key &&
                m_temp_trans.get_command (cmd) && cmd == SCIM_TRANS_CMD_GET_SURROUNDING_TEXT &&
                m_temp_trans.get_data (text) && m_temp_trans.get_data (cur)) {
                cursor = (int) cur;
                ret = true;
            }
        }
        if (cont) {
            int cmd;
            m_send_trans.clear ();
            m_send_trans.put_command (SCIM_TRANS_CMD_REPLY);

            // Move the read ptr to the end.
            if (!m_send_trans.get_command (cmd))
                SCIM_DEBUG_FRONTEND (1) << __func__ << " Get command is failed!!!\n";
        }
    }
    return ret;

}

bool
SocketFrontEnd::delete_surrounding_text (int id, int offset, int len)
{
    if (m_current_instance == id && m_current_socket_client >= 0 && len > 0) {
        m_send_trans.put_command (SCIM_TRANS_CMD_DELETE_SURROUNDING_TEXT);
        m_send_trans.put_data ((uint32) offset);
        m_send_trans.put_data ((uint32) len);
        return true;
    }
    return false;
}

bool
SocketFrontEnd::get_selection (int id, WideString &text)
{
    bool ret = false;
    text.clear ();

    if (m_current_instance == id && m_current_socket_client >= 0) {
        bool cont = false;
        Socket socket_client (m_current_socket_client);
        if (m_send_trans.get_data_type () != SCIM_TRANS_DATA_UNKNOWN) {
            m_send_trans.put_command (ISM_TRANS_CMD_TRANSACTION_CONTINUE);
            m_send_trans.write_to_socket (socket_client);
            cont = true;
            while (true) {
                Transaction _tran;
                _tran = m_receive_trans;
                m_continue_mark = STATUS_NONE;
                socket_receive_callback (&m_socket_server, socket_client);
                m_receive_trans = _tran;
                ContinueMark status = get_continue_mark ();
                if (status == STATUS_DISCONNECT)
                    return false;
                else if (status == STATUS_CONTINUE)
                    break;
            }
        }

        m_temp_trans.clear ();
        m_temp_trans.put_command (SCIM_TRANS_CMD_REPLY);
        m_temp_trans.put_command (SCIM_TRANS_CMD_GET_SELECTION);

        if (m_temp_trans.write_to_socket (socket_client) &&
            m_temp_trans.read_from_socket (socket_client, m_socket_timeout)) {

            int cmd;
            uint32 key;

            if (m_temp_trans.get_command (cmd) && cmd == SCIM_TRANS_CMD_REQUEST &&
                m_temp_trans.get_data (key) && key == m_current_socket_client_key &&
                m_temp_trans.get_command (cmd) && cmd == SCIM_TRANS_CMD_GET_SELECTION &&
                m_temp_trans.get_data (text)) {
                ret = true;
            }
        }
        if (cont) {
            int cmd;
            m_send_trans.clear ();
            m_send_trans.put_command (SCIM_TRANS_CMD_REPLY);

            // Move the read ptr to the end.
            if (!m_send_trans.get_command (cmd))
                SCIM_DEBUG_FRONTEND (1) << __func__ << " Get command is failed!!!\n";
        }
    }
    return ret;

}

bool
SocketFrontEnd::set_selection (int id, int start, int end)
{
    if (m_current_instance == id && m_current_socket_client >= 0) {
        m_send_trans.put_command (SCIM_TRANS_CMD_SET_SELECTION);
        m_send_trans.put_data ((uint32) start);
        m_send_trans.put_data ((uint32) end);
        return true;
    }
    return false;
}

void
SocketFrontEnd::send_private_command (int id, const String &command)
{
    if (m_current_instance == id) {
        m_send_trans.put_command (SCIM_TRANS_CMD_SEND_PRIVATE_COMMAND);
        m_temp_trans.put_data (command);
    }
}

void
SocketFrontEnd::expand_candidate (int id)
{
    if (m_current_instance == id)
        m_send_trans.put_command (ISM_TRANS_CMD_EXPAND_CANDIDATE);
}

void
SocketFrontEnd::contract_candidate (int id)
{
    if (m_current_instance == id)
        m_send_trans.put_command (ISM_TRANS_CMD_CONTRACT_CANDIDATE);
}

void
SocketFrontEnd::set_candidate_style (int                           id,
                                     ISF_CANDIDATE_PORTRAIT_LINE_T portrait_line,
                                     ISF_CANDIDATE_MODE_T          mode)
{
    if (m_current_instance == id) {
        m_send_trans.put_command (ISM_TRANS_CMD_SET_CANDIDATE_UI);
        m_send_trans.put_data ((uint32) portrait_line);
        m_send_trans.put_data ((uint32) mode);
    }
}

void
SocketFrontEnd::init (int argc, char **argv)
{
    int max_clients = -1;

    std::vector<String> engine_list;

    if (!m_config.null ()) {
        String str;

        m_config_readonly = m_config->read (String (SCIM_CONFIG_FRONTEND_SOCKET_CONFIG_READONLY), false);

        max_clients = m_config->read (String (SCIM_CONFIG_FRONTEND_SOCKET_MAXCLIENTS), -1);

        m_config->signal_connect_reload (slot (this, &SocketFrontEnd::reload_config_callback));
    } else {
        m_config_readonly = false;
        max_clients = -1;
    }

    if (!m_socket_server.create (scim_get_default_socket_frontend_address ()))
        throw FrontEndError ("SocketFrontEnd -- Cannot create SocketServer.");

    m_socket_server.set_max_clients (max_clients);

    m_socket_server.signal_connect_accept (
        slot (this, &SocketFrontEnd::socket_accept_callback));

    m_socket_server.signal_connect_receive (
        slot (this, &SocketFrontEnd::socket_receive_callback));

    m_socket_server.signal_connect_exception (
        slot (this, &SocketFrontEnd::socket_exception_callback));

    if (argv && argc > 1) {
        for (int i = 1; i < argc && argv [i]; ++i) {
            if (String ("-e") == argv [i] || String ("--engines") == argv [i]) {
                if (++i >= argc) {
                    std::cerr << "No argument for option " << argv [i-1] << "\n";
                    break;
                }
                if (String (argv [i]) != "none") {
                    scim_split_string_list (engine_list, String (argv [i]), ',');
                }
                continue;
            }
            if (String ("--no-stay") == argv [i])
                m_stay = false;
        }
    }

    load_helper_modules (engine_list);
    if (scim_global_config_read (SCIM_GLOBAL_CONFIG_PRELOAD_KEYBOARD_ISE, false)) {
        String def_uuid = m_config->read (String (SCIM_CONFIG_DEFAULT_IMENGINE_FACTORY) +
                                       String ("/") + String ("~other"),
                                       String (""));
        preload_keyboard_ise (def_uuid);
    }

    /**
     * initialize the random number generator.
     */
    srand (time (0));

    signal (SIGCHLD, SIG_IGN);
}

/**
 * @brief Callback function for ecore fd handler.
 *
 * @param data The data to pass to this callback.
 * @param fd_handler The ecore fd handler.
 *
 * @return ECORE_CALLBACK_RENEW
 */
static Eina_Bool
frontend_handler (void *data, Ecore_Fd_Handler *fd_handler)
{
    if (data == NULL || fd_handler == NULL)
        return ECORE_CALLBACK_RENEW;
    SocketServer *_socket_server = (SocketServer*)data;
    int _fd = ecore_main_fd_handler_fd_get (fd_handler);
    SCIM_DEBUG_FRONTEND (1) << "frontend_handler (" << _fd << ").\n";
    if (!_socket_server->filter_event (_fd)) {
        _socket_server->filter_exception_event (_fd);
    }
    return ECORE_CALLBACK_RENEW;
}

void
SocketFrontEnd::run ()
{
    if (m_socket_server.valid ())
    {
        ecore_init ();
        ecore_main_loop_glib_integrate ();
        Ecore_Fd_Handler *_server_read_handler = ecore_main_fd_handler_add (m_socket_server.get_id (),
                ECORE_FD_READ, frontend_handler, &m_socket_server, NULL, NULL);
        if (_server_read_handler != NULL) {
            ecore_main_loop_begin ();
            ecore_main_fd_handler_del (_server_read_handler);
        } else {
            SCIM_DEBUG_FRONTEND (1) << "Error occurred when calling ecore_main_fd_handler_add(server_fd)" << "\n";
        }
        ecore_shutdown ();
    }
}

uint32
SocketFrontEnd::generate_key () const
{
    unsigned int seed = (unsigned int)time (NULL);

    return (uint32)rand_r (&seed);
}

SocketFrontEnd::ContinueMark
SocketFrontEnd::get_continue_mark()
{
    ContinueMark _is_continue = m_continue_mark;
    m_continue_mark = STATUS_NONE;
    return _is_continue;
}

bool
SocketFrontEnd::check_client_connection (const Socket &client) const
{
    SCIM_DEBUG_FRONTEND (1) << "check_client_connection (" << client.get_id () << ").\n";

    unsigned char buf [sizeof (uint32)];

    int nbytes = client.read_with_timeout (buf, sizeof (uint32), m_socket_timeout);

    if (nbytes == sizeof (uint32))
        return true;

    if (nbytes < 0) {
        SCIM_DEBUG_FRONTEND (2) << " Error occurred when reading socket (" << client.get_id ()
            << "):" << client.get_error_message () << "\n";
    } else {
        SCIM_DEBUG_FRONTEND (2) << " Timeout when reading socket (" << client.get_id ()
            << ").\n";
    }

    return false;
}

void
SocketFrontEnd::socket_accept_callback (SocketServer *server, const Socket &client)
{
    SCIM_DEBUG_FRONTEND (1) << "socket_accept_callback (" << client.get_id () << ").\n";
    ClientInfo client_info = socket_get_client_info (client);

    // If it's a new client, then request to open the connection first.
    if (client_info.type == NONE_CLIENT) {
        client_info.type = UNKNOWN_CLIENT;
        client_info.handler = ecore_main_fd_handler_add (client.get_id (),
                ECORE_FD_READ, frontend_handler, &m_socket_server, NULL, NULL);
        if (client_info.handler == NULL) {
            SCIM_DEBUG_FRONTEND (1) << "Error occurred when calling ecore_main_fd_handler_add("
                << client.get_id () << ")\n";
            server->close_connection (client);
        } else {
            //Insert new ClientInfo into repository with type UNKNOWN_CLIENT.
            m_socket_client_repository [client.get_id ()] = client_info;
        }
    }
}

void
SocketFrontEnd::socket_receive_callback (SocketServer *server, const Socket &client)
{
    int id = client.get_id ();
    int cmd;
    uint32 key;
    bool reply = true;

    ClientInfo client_info;

    SCIM_DEBUG_FRONTEND (1) << "socket_receive_callback (" << id << ").\n";

    // Check if the client is closed.
    if (!check_client_connection (client)) {
        SCIM_DEBUG_FRONTEND (2) << " closing client connection.\n";
        socket_close_connection (server, client);
        m_continue_mark = STATUS_DISCONNECT;
        return;
    }

    client_info = socket_get_client_info (client);

    // If it's a new client, then request to open the connection first.
    if (client_info.type == UNKNOWN_CLIENT) {
        socket_open_connection (server, client);
        return;
    }

    // If can not read the transaction,
    // or the transaction is not started with SCIM_TRANS_CMD_REQUEST,
    // or the key is mismatch,
    // just return.
    if (!m_receive_trans.read_from_socket (client, m_socket_timeout) ||
        !m_receive_trans.get_command (cmd) || cmd != SCIM_TRANS_CMD_REQUEST ||
        !m_receive_trans.get_data (key) || key != (uint32) client_info.key)
        return;

    m_current_socket_client     = id;
    m_current_socket_client_key = key;

    m_send_trans.clear ();
    m_send_trans.put_command (SCIM_TRANS_CMD_REPLY);

    // Move the read ptr to the end.
    if (!m_send_trans.get_command (cmd))
        return;

    while (m_receive_trans.get_command (cmd)) {
        if (cmd == ISM_TRANS_CMD_TRANSACTION_CONTINUE) {
            m_continue_mark = STATUS_CONTINUE;
            return;
        }
        else if (cmd == SCIM_TRANS_CMD_PROCESS_KEY_EVENT)
            socket_process_key_event (id);
        else if (cmd == SCIM_TRANS_CMD_MOVE_PREEDIT_CARET)
            socket_move_preedit_caret (id);
        else if (cmd == ISM_TRANS_CMD_SELECT_AUX)
            socket_select_aux (id);
        else if (cmd == SCIM_TRANS_CMD_SELECT_CANDIDATE)
            socket_select_candidate (id);
        else if (cmd == SCIM_TRANS_CMD_UPDATE_LOOKUP_TABLE_PAGE_SIZE)
            socket_update_lookup_table_page_size (id);
        else if (cmd == SCIM_TRANS_CMD_LOOKUP_TABLE_PAGE_UP)
            socket_lookup_table_page_up (id);
        else if (cmd == SCIM_TRANS_CMD_LOOKUP_TABLE_PAGE_DOWN)
            socket_lookup_table_page_down (id);
        else if (cmd == ISM_TRANS_CMD_SET_PREDICTION_ALLOW)
            socket_set_prediction_allow (id);
        else if (cmd == ISM_TRANS_CMD_UPDATE_CANDIDATE_ITEM_LAYOUT)
            socket_update_candidate_item_layout (id);
        else if (cmd == ISM_TRANS_CMD_UPDATE_CURSOR_POSITION)
            socket_update_cursor_position (id);
        else if (cmd == ISM_TRANS_CMD_UPDATE_DISPLAYED_CANDIDATE)
            socket_update_displayed_candidate_number (id);
        else if (cmd == ISM_TRANS_CMD_CANDIDATE_MORE_WINDOW_SHOW)
            socket_candidate_more_window_show (id);
        else if (cmd == ISM_TRANS_CMD_CANDIDATE_MORE_WINDOW_HIDE)
            socket_candidate_more_window_hide (id);
        else if (cmd == ISM_TRANS_CMD_LONGPRESS_CANDIDATE)
            socket_longpress_candidate (id);
        else if (cmd == ISM_TRANS_CMD_SET_ISE_IMDATA)
            socket_set_imdata (id);
        else if (cmd == SCIM_TRANS_CMD_SET_AUTOCAPITAL_TYPE)
            socket_set_autocapital_type (id);
        else if (cmd == ISM_TRANS_CMD_SET_LAYOUT)
            socket_set_layout (id);
        else if (cmd == ISM_TRANS_CMD_SET_INPUT_HINT)
            socket_set_input_hint (id);
        else if (cmd == ISM_TRANS_CMD_UPDATE_BIDI_DIRECTION)
            socket_update_bidi_direction (id);
        else if (cmd == ISM_TRANS_CMD_RESET_ISE_OPTION)
            socket_reset_option (id);
        else if (cmd == SCIM_TRANS_CMD_RESET)
            socket_reset (id);
        else if (cmd == SCIM_TRANS_CMD_FOCUS_IN)
            socket_focus_in (id);
        else if (cmd == SCIM_TRANS_CMD_FOCUS_OUT)
            socket_focus_out (id);
        else if (cmd == SCIM_TRANS_CMD_TRIGGER_PROPERTY)
            socket_trigger_property (id);
        else if (cmd == SCIM_TRANS_CMD_PROCESS_HELPER_EVENT)
            socket_process_helper_event (id);
        else if (cmd == SCIM_TRANS_CMD_UPDATE_CLIENT_CAPABILITIES)
            socket_update_client_capabilities (id);
        else if (cmd == SCIM_TRANS_CMD_GET_FACTORY_LIST)
            socket_get_factory_list (id);
        else if (cmd == SCIM_TRANS_CMD_GET_FACTORY_NAME)
            socket_get_factory_name (id);
        else if (cmd == SCIM_TRANS_CMD_GET_FACTORY_AUTHORS)
            socket_get_factory_authors (id);
        else if (cmd == SCIM_TRANS_CMD_GET_FACTORY_CREDITS)
            socket_get_factory_credits (id);
        else if (cmd == SCIM_TRANS_CMD_GET_FACTORY_HELP)
            socket_get_factory_help (id);
        else if (cmd == SCIM_TRANS_CMD_GET_FACTORY_LOCALES)
            socket_get_factory_locales (id);
        else if (cmd == SCIM_TRANS_CMD_GET_FACTORY_ICON_FILE)
            socket_get_factory_icon_file (id);
        else if (cmd == SCIM_TRANS_CMD_GET_FACTORY_LANGUAGE)
            socket_get_factory_language (id);
        else if (cmd == SCIM_TRANS_CMD_NEW_INSTANCE)
            socket_new_instance (id);
        else if (cmd == SCIM_TRANS_CMD_DELETE_INSTANCE)
            socket_delete_instance (id);
        else if (cmd == SCIM_TRANS_CMD_DELETE_ALL_INSTANCES)
            socket_delete_all_instances (id);
        else if (cmd == SCIM_TRANS_CMD_FLUSH_CONFIG)
            socket_flush_config (id);
        else if (cmd == SCIM_TRANS_CMD_ERASE_CONFIG)
            socket_erase_config (id);
        else if (cmd == SCIM_TRANS_CMD_RELOAD_CONFIG)
            socket_reload_config (id);
        else if (cmd == SCIM_TRANS_CMD_GET_CONFIG_STRING)
            socket_get_config_string (id);
        else if (cmd == SCIM_TRANS_CMD_SET_CONFIG_STRING)
            socket_set_config_string (id);
        else if (cmd == SCIM_TRANS_CMD_GET_CONFIG_INT)
            socket_get_config_int (id);
        else if (cmd == SCIM_TRANS_CMD_SET_CONFIG_INT)
            socket_set_config_int (id);
        else if (cmd == SCIM_TRANS_CMD_GET_CONFIG_BOOL)
            socket_get_config_bool (id);
        else if (cmd == SCIM_TRANS_CMD_SET_CONFIG_BOOL)
            socket_set_config_bool (id);
        else if (cmd == SCIM_TRANS_CMD_GET_CONFIG_DOUBLE)
            socket_get_config_double (id);
        else if (cmd == SCIM_TRANS_CMD_SET_CONFIG_DOUBLE)
            socket_set_config_double (id);
        else if (cmd == SCIM_TRANS_CMD_GET_CONFIG_VECTOR_STRING)
            socket_get_config_vector_string (id);
        else if (cmd == SCIM_TRANS_CMD_SET_CONFIG_VECTOR_STRING)
            socket_set_config_vector_string (id);
        else if (cmd == SCIM_TRANS_CMD_GET_CONFIG_VECTOR_INT)
            socket_get_config_vector_int (id);
        else if (cmd == SCIM_TRANS_CMD_SET_CONFIG_VECTOR_INT)
            socket_set_config_vector_int (id);
        else if (cmd == ISM_TRANS_CMD_GET_ACTIVE_ISE_LIST)
            get_active_ise_list (id);
        else if (cmd == SCIM_TRANS_CMD_LOAD_FILE)
            socket_load_file (id);
        else if (cmd == SCIM_TRANS_CMD_CLOSE_CONNECTION) {
            socket_close_connection (server, client);
            m_current_socket_client     = -1;
            m_current_socket_client_key = 0;
            return;
        } else if (cmd == SCIM_TRANS_CMD_HELPER_MANAGER_GET_HELPER_LIST) {
            SCIM_DEBUG_FRONTEND (1) << "receive cmd SCIM_TRANS_CMD_HELPER_MANAGER_GET_HELPER_LIST\n";
            get_helper_list (client);
        } else if (cmd == SCIM_TRANS_CMD_HELPER_MANAGER_RUN_HELPER) {
            SCIM_DEBUG_FRONTEND (1) << "receive cmd SCIM_TRANS_CMD_HELPER_MANAGER_RUN_HELPER\n";
            reply = false;
            run_helper (client);
        } else if (cmd == SCIM_TRANS_CMD_HELPER_MANAGER_STOP_HELPER) {
            SCIM_DEBUG_FRONTEND (1) << "receive cmd SCIM_TRANS_CMD_HELPER_MANAGER_STOP_HELPER\n";
            unregister_helper ();
        } else if (cmd == SCIM_TRANS_CMD_HELPER_MANAGER_SEND_DISPLAY) {
            SCIM_DEBUG_FRONTEND (1) << "receive cmd SCIM_TRANS_CMD_HELPER_MANAGER_SEND_DISPLAY\n";
            socket_set_display_name (id);
        } else if (cmd == SCIM_TRANS_CMD_HELPER_MANAGER_SEND_ISE_LIST) {
            SCIM_DEBUG_FRONTEND (1) << "receive cmd SCIM_TRANS_CMD_HELPER_MANAGER_SEND_ISE_LIST\n";
            reply = false;
            socket_update_ise_list (id);
        } else if (cmd == ISM_TRANS_CMD_PRELOAD_KEYBOARD_ISE) {
            SCIM_DEBUG_FRONTEND (1) << "receive cmd ISM_TRANS_CMD_PRELOAD_KEYBOARD_ISE\n";
            String uuid;
            if (m_receive_trans.get_data (uuid) && uuid.length ())
                preload_keyboard_ise (uuid);
            m_send_trans.put_command (SCIM_TRANS_CMD_OK);
        } else if (cmd == ISM_TRANS_CMD_TURN_ON_LOG) {
            SCIM_DEBUG_FRONTEND (1) << "receive cmd ISM_TRANS_CMD_TURN_ON_LOG\n";
            socket_turn_on_log (id);
        }
    }

    // Send reply to client
    if (m_send_trans.get_data_type () == SCIM_TRANS_DATA_UNKNOWN)
        m_send_trans.put_command (SCIM_TRANS_CMD_FAIL);

    if (reply)
        m_send_trans.write_to_socket (client);

    m_current_socket_client     = -1;
    m_current_socket_client_key = 0;

    SCIM_DEBUG_FRONTEND (1) << "End of socket_receive_callback (" << id << ").\n";
}

bool
SocketFrontEnd::socket_open_connection (SocketServer *server, const Socket &client)
{
    SCIM_DEBUG_FRONTEND (2) << " Open socket connection for client " << client.get_id () 
        << "  number of clients=" << m_socket_client_repository.size () << ".\n";

    uint32 key;
    String type = scim_socket_accept_connection (key,
                                                 String ("SocketFrontEnd,HelperLauncher"),
                                                 String ("SocketIMEngine,SocketConfig,HelperManager"),
                                                 client,
                                                 m_socket_timeout);
    ClientInfo info = socket_get_client_info (client);

    if (type.length () && info.type == UNKNOWN_CLIENT) {
        info.key = key;
        info.type = ((type == "SocketIMEngine") ? IMENGINE_CLIENT
                        : ((type == "SocketConfig") ? CONFIG_CLIENT : HELPER_MANAGER_CLIENT));
        SCIM_DEBUG_MAIN (2) << " Add client to repository. Type=" << type << " key=" << key << "\n";
        // Overwrite ClientInfo
        m_socket_client_repository [client.get_id ()] = info;
        return true;
    }

    // Client did not pass the registration process, close it.
    SCIM_DEBUG_FRONTEND (2) << " Failed to create new connection. type(" << type.c_str () << "," << info.type << ")\n";
    socket_close_connection (server, client);
    return false;
}

void
SocketFrontEnd::socket_close_connection (SocketServer *server, const Socket &client)
{
    SCIM_DEBUG_FRONTEND (2) << " Close client connection " << client.get_id () << " number of clients="
        << m_socket_client_repository.size () << ".\n";

    ClientInfo client_info = socket_get_client_info (client);

    server->close_connection (client);

    if (client_info.type != NONE_CLIENT) {
        ecore_main_fd_handler_del (client_info.handler);
        m_socket_client_repository.erase (client.get_id ());

        if (client_info.type == IMENGINE_CLIENT)
            socket_delete_all_instances (client.get_id ());

        if (!m_socket_client_repository.size () && !m_stay) {
            SCIM_DEBUG_FRONTEND (1) << "All clients are close, FrontEnd is exiting." << "\n";
            server->shutdown ();
            ecore_main_loop_quit ();
        }
    }
}

SocketFrontEnd::ClientInfo
SocketFrontEnd::socket_get_client_info (const Socket &client)
{
    static ClientInfo null_client = { 0, NONE_CLIENT, 0};
    SocketClientRepository::iterator it = m_socket_client_repository.find (client.get_id ());

    if (it != m_socket_client_repository.end ())
        return it->second;

    return null_client;
}

void
SocketFrontEnd::socket_exception_callback (SocketServer *server, const Socket &client)
{
    SCIM_DEBUG_FRONTEND (1) << "socket_exception_callback (" << client.get_id () << ").\n";

    socket_close_connection (server, client);
}

//client_id is client's socket id
void
SocketFrontEnd::socket_get_factory_list (int /*client_id*/)
{
    String encoding;

    SCIM_DEBUG_FRONTEND (2) << " socket_get_factory_list.\n";

    if (m_receive_trans.get_data (encoding)) {
        std::vector<String> uuids;

        if (encoding == "")
            get_factory_list (uuids);
        else
            get_factory_list_for_encoding (uuids, encoding);

        SCIM_DEBUG_FRONTEND (3) << "  Encoding (" << encoding
            << ") Num(" << uuids.size () << ").\n";

        m_send_trans.put_data (uuids);
        m_send_trans.put_command (SCIM_TRANS_CMD_OK);
    }
}

void
SocketFrontEnd::socket_get_factory_name (int /*client_id*/)
{
    String sfid;

    SCIM_DEBUG_FRONTEND (2) << " socket_get_factory_name.\n";

    if (m_receive_trans.get_data (sfid)) {
        WideString name = get_factory_name (sfid);

        m_send_trans.put_data (name);
        m_send_trans.put_command (SCIM_TRANS_CMD_OK);
    }
}

void
SocketFrontEnd::socket_get_factory_authors (int /*client_id*/)
{
    String sfid;

    SCIM_DEBUG_FRONTEND (2) << " socket_get_factory_authors.\n";

    if (m_receive_trans.get_data (sfid)) {
        WideString authors = get_factory_authors (sfid);

        m_send_trans.put_data (authors);
        m_send_trans.put_command (SCIM_TRANS_CMD_OK);
    }
}

void
SocketFrontEnd::socket_get_factory_credits (int /*client_id*/)
{
    String sfid;

    SCIM_DEBUG_FRONTEND (2) << " socket_get_factory_credits.\n";

    if (m_receive_trans.get_data (sfid)) {
        WideString credits = get_factory_credits (sfid);

        m_send_trans.put_data (credits);
        m_send_trans.put_command (SCIM_TRANS_CMD_OK);
    }
}

void
SocketFrontEnd::socket_get_factory_help (int /*client_id*/)
{
    String sfid;

    SCIM_DEBUG_FRONTEND (2) << " socket_get_factory_help.\n";

    if (m_receive_trans.get_data (sfid)) {
        WideString help = get_factory_help (sfid);

        m_send_trans.put_data (help);
        m_send_trans.put_command (SCIM_TRANS_CMD_OK);
    }
}

void
SocketFrontEnd::socket_get_factory_locales (int /*client_id*/)
{
    String sfid;

    SCIM_DEBUG_FRONTEND (2) << " socket_get_factory_locales.\n";

    if (m_receive_trans.get_data (sfid)) {
        String locales = get_factory_locales (sfid);

        SCIM_DEBUG_FRONTEND (3) << "  Locales (" << locales << ").\n";

        m_send_trans.put_data (locales);
        m_send_trans.put_command (SCIM_TRANS_CMD_OK);
    }
}

void
SocketFrontEnd::socket_get_factory_icon_file (int /*client_id*/)
{
    String sfid;

    SCIM_DEBUG_FRONTEND (2) << " socket_get_factory_icon_file.\n";

    if (m_receive_trans.get_data (sfid)) {
        String iconfile = get_factory_icon_file (sfid);

        SCIM_DEBUG_FRONTEND (3) << "  ICON File (" << iconfile << ").\n";

        m_send_trans.put_data (iconfile);
        m_send_trans.put_command (SCIM_TRANS_CMD_OK);
    }
}

void
SocketFrontEnd::socket_get_factory_language (int /*client_id*/)
{
    String sfid;

    SCIM_DEBUG_FRONTEND (2) << " socket_get_factory_language.\n";

    if (m_receive_trans.get_data (sfid)) {
        String language = get_factory_language (sfid);

        SCIM_DEBUG_FRONTEND (3) << "  Language (" << language << ").\n";

        m_send_trans.put_data (language);
        m_send_trans.put_command (SCIM_TRANS_CMD_OK);
    }
}

void
SocketFrontEnd::socket_new_instance (int client_id)
{
    String sfid;
    String encoding;

    SCIM_DEBUG_FRONTEND (2) << " socket_new_instance.\n";

    if (m_receive_trans.get_data (sfid) &&
        m_receive_trans.get_data (encoding)) {
        int siid = new_instance (m_config, sfid, encoding);

        // Instance created OK.
        if (siid >= 0) {
            SocketInstanceRepository::iterator it =
                std::lower_bound (m_socket_instance_repository.begin (),
                                  m_socket_instance_repository.end (),
                                  std::pair <int, int> (client_id, siid));

            if (it == m_socket_instance_repository.end ())
                m_socket_instance_repository.push_back (std::pair <int, int> (client_id, siid));
            else
                m_socket_instance_repository.insert (it, std::pair <int, int> (client_id, siid));

            SCIM_DEBUG_FRONTEND (3) << "  InstanceID (" << siid << ").\n";

            m_send_trans.put_data ((uint32)siid);
            m_send_trans.put_command (SCIM_TRANS_CMD_OK);
        }
    }
}

void
SocketFrontEnd::socket_delete_instance (int client_id)
{
    uint32 siid;

    SCIM_DEBUG_FRONTEND (2) << " socket_delete_instance.\n";

    if (m_receive_trans.get_data (siid)) {

        SCIM_DEBUG_FRONTEND (3) << "  InstanceID (" << siid << ").\n";

        m_current_instance = (int) siid;

        delete_instance ((int) siid);

        m_current_instance = -1;

        SocketInstanceRepository::iterator it =
            std::lower_bound (m_socket_instance_repository.begin (),
                              m_socket_instance_repository.end (),
                              std::pair <int, int> (client_id, siid));

        if (it != m_socket_instance_repository.end () &&
            *it == std::pair <int, int> (client_id, siid))
            m_socket_instance_repository.erase (it);

        m_send_trans.put_command (SCIM_TRANS_CMD_OK);
    }
}

void
SocketFrontEnd::socket_delete_all_instances (int client_id)
{
    SCIM_DEBUG_FRONTEND (2) << " socket_delete_all_instances.\n";

    SocketInstanceRepository::iterator it;

    SocketInstanceRepository::iterator lit =
        std::lower_bound (m_socket_instance_repository.begin (),
                          m_socket_instance_repository.end (),
                          std::pair <int, int> (client_id, 0));

    SocketInstanceRepository::iterator uit =
        std::upper_bound (m_socket_instance_repository.begin (),
                          m_socket_instance_repository.end (),
                          std::pair <int, int> (client_id, INT_MAX));

    if (lit != uit) {
        for (it = lit; it != uit; ++it) {
            m_current_instance = it->second;
            delete_instance (it->second);
        }
        m_current_instance = -1;
        m_socket_instance_repository.erase (lit, uit);
        m_send_trans.put_command (SCIM_TRANS_CMD_OK);
    }
}

void
SocketFrontEnd::socket_process_key_event (int /*client_id*/)
{
    uint32   siid;
    KeyEvent event;

    SCIM_DEBUG_FRONTEND (2) << " socket_process_key_event.\n";

    if (m_receive_trans.get_data (siid) &&
        m_receive_trans.get_data (event)) {

        SCIM_DEBUG_FRONTEND (3) << "  SI (" << siid << ") KeyEvent ("
            << event.code << "," << event.mask << ").\n";

        m_current_instance = (int) siid;

        if (process_key_event ((int) siid, event))
            m_send_trans.put_command (SCIM_TRANS_CMD_OK);
        else
            m_send_trans.put_command (SCIM_TRANS_CMD_FAIL);

        m_current_instance = -1;
    }
}

void
SocketFrontEnd::socket_move_preedit_caret (int /*client_id*/)
{
    uint32 siid;
    uint32 caret;

    SCIM_DEBUG_FRONTEND (2) << " socket_move_preedit_caret.\n";

    if (m_receive_trans.get_data (siid) &&
        m_receive_trans.get_data (caret)) {

        SCIM_DEBUG_FRONTEND (3) << "  SI (" << siid
            << ") Caret (" << caret << ").\n";

        m_current_instance = (int) siid;

        move_preedit_caret ((int) siid, caret);
        m_send_trans.put_command (SCIM_TRANS_CMD_OK);

        m_current_instance = -1;
    }
}

void
SocketFrontEnd::socket_select_aux (int /*client_id*/)
{
    uint32 siid;
    uint32 item;

    SCIM_DEBUG_FRONTEND (2) << " socket_select_aux.\n";

    if (m_receive_trans.get_data (siid) &&
        m_receive_trans.get_data (item)) {

        SCIM_DEBUG_FRONTEND (3) << "  SI (" << siid << ") Item (" << item << ").\n";

        m_current_instance = (int) siid;

        select_aux ((int) siid, item);
        m_send_trans.put_command (SCIM_TRANS_CMD_OK);

        m_current_instance = -1;
    }
}

void
SocketFrontEnd::socket_select_candidate (int /*client_id*/)
{
    uint32 siid;
    uint32 item;

    SCIM_DEBUG_FRONTEND (2) << " socket_select_candidate.\n";

    if (m_receive_trans.get_data (siid) &&
        m_receive_trans.get_data (item)) {

        SCIM_DEBUG_FRONTEND (3) << "  SI (" << siid << ") Item (" << item << ").\n";

        m_current_instance = (int) siid;

        select_candidate ((int) siid, item);
        m_send_trans.put_command (SCIM_TRANS_CMD_OK);

        m_current_instance = -1;
    }
}

void
SocketFrontEnd::socket_update_lookup_table_page_size (int /*client_id*/)
{
    uint32 siid;
    uint32 size;

    SCIM_DEBUG_FRONTEND (2) << " socket_update_lookup_table_page_size.\n";

    if (m_receive_trans.get_data (siid) &&
        m_receive_trans.get_data (size)) {

        SCIM_DEBUG_FRONTEND (3) << "  SI (" << siid << ") PageSize (" << size << ").\n";

        m_current_instance = (int) siid;

        update_lookup_table_page_size ((int) siid, size);
        m_send_trans.put_command (SCIM_TRANS_CMD_OK);

        m_current_instance = -1;
    }
}

void
SocketFrontEnd::socket_lookup_table_page_up (int /*client_id*/)
{
    uint32 siid;

    SCIM_DEBUG_FRONTEND (2) << " socket_lookup_table_page_up.\n";

    if (m_receive_trans.get_data (siid)) {

        SCIM_DEBUG_FRONTEND (3) << "  SI (" << siid << ").\n";

        m_current_instance = (int) siid;

        lookup_table_page_up ((int) siid);
        m_send_trans.put_command (SCIM_TRANS_CMD_OK);

        m_current_instance = -1;
    }
}

void
SocketFrontEnd::socket_lookup_table_page_down (int /*client_id*/)
{
    uint32 siid;

    SCIM_DEBUG_FRONTEND (2) << " socket_lookup_table_page_down.\n";

    if (m_receive_trans.get_data (siid)) {

        SCIM_DEBUG_FRONTEND (3) << "  SI (" << siid << ").\n";

        m_current_instance = (int) siid;

        lookup_table_page_down ((int) siid);
        m_send_trans.put_command (SCIM_TRANS_CMD_OK);

        m_current_instance = -1;
    }
}

void
SocketFrontEnd::socket_set_prediction_allow (int /*client_id*/)
{
    uint32 siid;
    uint32 allow;

    SCIM_DEBUG_FRONTEND (2) << __func__ << "\n";

    if (m_receive_trans.get_data (siid) &&
        m_receive_trans.get_data (allow)) {

        SCIM_DEBUG_FRONTEND (3) << "  SI (" << siid << ").\n";

        m_current_instance = (int) siid;

        set_prediction_allow ((int) siid, (bool) allow);
        m_send_trans.put_command (SCIM_TRANS_CMD_OK);

        m_current_instance = -1;
    }
}

void
SocketFrontEnd::socket_set_layout (int /*client_id*/)
{
    uint32 siid;
    uint32 layout;

    SCIM_DEBUG_FRONTEND (2) << __func__ << "\n";

    if (m_receive_trans.get_data (siid) &&
        m_receive_trans.get_data (layout)) {

        SCIM_DEBUG_FRONTEND (3) << "  SI (" << siid << ").\n";

        m_current_instance = (int) siid;

        set_layout ((int) siid, layout);
        m_send_trans.put_command (SCIM_TRANS_CMD_OK);

        m_current_instance = -1;
    }
}

void
SocketFrontEnd::socket_set_input_hint (int /*client_id*/)
{
    uint32 siid;
    uint32 input_hint;

    SCIM_DEBUG_FRONTEND (2) << __func__ << "\n";

    if (m_receive_trans.get_data (siid) &&
        m_receive_trans.get_data (input_hint)) {

        SCIM_DEBUG_FRONTEND (3) << "  SI (" << siid << ").\n";

        m_current_instance = (int) siid;

        set_input_hint ((int) siid, input_hint);
        m_send_trans.put_command (SCIM_TRANS_CMD_OK);

        m_current_instance = -1;
    }
}

void
SocketFrontEnd::socket_update_bidi_direction (int /*client_id*/)
{
    uint32 siid;
    uint32 bidi_direction;

    SCIM_DEBUG_FRONTEND (2) << __func__ << "\n";

    if (m_receive_trans.get_data (siid) &&
        m_receive_trans.get_data (bidi_direction)) {

        SCIM_DEBUG_FRONTEND (3) << "  SI (" << siid << ").\n";

        m_current_instance = (int) siid;

        update_bidi_direction ((int) siid, bidi_direction);
        m_send_trans.put_command (SCIM_TRANS_CMD_OK);

        m_current_instance = -1;
    }
}

void
SocketFrontEnd::socket_update_candidate_item_layout (int /*client_id*/)
{
    uint32 siid;
    std::vector<unsigned int> row_items;

    SCIM_DEBUG_FRONTEND (2) << __func__ << "\n";

    if (m_receive_trans.get_data (siid) &&
        m_receive_trans.get_data (row_items)) {

        SCIM_DEBUG_FRONTEND (3) << "  SI (" << siid << ") RowSize (" << row_items.size () << ").\n";

        m_current_instance = (int) siid;

        update_candidate_item_layout ((int) siid, row_items);
        m_send_trans.put_command (SCIM_TRANS_CMD_OK);

        m_current_instance = -1;
    }
}

void
SocketFrontEnd::socket_update_cursor_position (int /*client_id*/)
{
    uint32 siid;
    unsigned int cursor_pos;

    SCIM_DEBUG_FRONTEND (2) << __func__ << "\n";

    if (m_receive_trans.get_data (siid) &&
        m_receive_trans.get_data (cursor_pos)) {

        SCIM_DEBUG_FRONTEND (3) << "  SI (" << siid << ") New cursor (" << cursor_pos << ").\n";

        m_current_instance = (int) siid;

        update_cursor_position ((int) siid, cursor_pos);
        m_send_trans.put_command (SCIM_TRANS_CMD_OK);

        m_current_instance = -1;
    }
}

void
SocketFrontEnd::socket_update_displayed_candidate_number (int /*client_id*/)
{
    uint32 siid;
    unsigned int number;

    SCIM_DEBUG_FRONTEND (2) << __func__ << "\n";

    if (m_receive_trans.get_data (siid) &&
        m_receive_trans.get_data (number)) {

        SCIM_DEBUG_FRONTEND (3) << "  SI (" << siid << ") displayed candidate number (" << number << ").\n";

        m_current_instance = (int) siid;

        update_displayed_candidate_number ((int) siid, number);
        m_send_trans.put_command (SCIM_TRANS_CMD_OK);

        m_current_instance = -1;
    }
}

void
SocketFrontEnd::socket_candidate_more_window_show (int /*client_id*/)
{
    uint32 siid;

    SCIM_DEBUG_FRONTEND (2) << __func__ << "\n";

    if (m_receive_trans.get_data (siid)) {

        SCIM_DEBUG_FRONTEND (3) << "  SI (" << siid << ").\n";

        m_current_instance = (int) siid;

        candidate_more_window_show ((int) siid);
        m_send_trans.put_command (SCIM_TRANS_CMD_OK);

        m_current_instance = -1;
    }
}

void
SocketFrontEnd::socket_candidate_more_window_hide (int /*client_id*/)
{
    uint32 siid;

    SCIM_DEBUG_FRONTEND (2) << __func__ << "\n";

    if (m_receive_trans.get_data (siid)) {

        SCIM_DEBUG_FRONTEND (3) << "  SI (" << siid << ").\n";

        m_current_instance = (int) siid;

        candidate_more_window_hide ((int) siid);
        m_send_trans.put_command (SCIM_TRANS_CMD_OK);

        m_current_instance = -1;
    }
}

void
SocketFrontEnd::socket_longpress_candidate (int /*client_id*/)
{
    uint32 siid;
    unsigned int index;

    SCIM_DEBUG_FRONTEND (2) << __func__ << "\n";

    if (m_receive_trans.get_data (siid) &&
        m_receive_trans.get_data (index)) {

        SCIM_DEBUG_FRONTEND (3) << "  SI (" << siid << ") index (" << index << ").\n";

        m_current_instance = (int) siid;

        longpress_candidate ((int) siid, index);
        m_send_trans.put_command (SCIM_TRANS_CMD_OK);

        m_current_instance = -1;
    }
}

void
SocketFrontEnd::socket_set_imdata (int /*client_id*/)
{
    uint32 siid;
    char *imdata = NULL;
    size_t length;

    SCIM_DEBUG_FRONTEND (2) << __func__ << "\n";

    if (m_receive_trans.get_data (siid) &&
        m_receive_trans.get_data (&imdata, length)) {

        SCIM_DEBUG_FRONTEND (3) << "  SI (" << siid << ") length (" << length << ").\n";

        m_current_instance = (int) siid;

        set_imdata ((int) siid, imdata, length);
        m_send_trans.put_command (SCIM_TRANS_CMD_OK);

        m_current_instance = -1;
    }

    if (NULL != imdata)
        delete [] imdata;
}

void
SocketFrontEnd::socket_set_autocapital_type (int /*client_id*/)
{
    uint32 siid;
    uint32 mode;

    SCIM_DEBUG_FRONTEND (2) << __func__ << "\n";

    if (m_receive_trans.get_data (siid) &&
        m_receive_trans.get_data (mode)) {

        SCIM_DEBUG_FRONTEND (3) << "  SI (" << siid << ").\n";

        m_current_instance = (int) siid;

        set_autocapital_type ((int) siid, mode);
        m_send_trans.put_command (SCIM_TRANS_CMD_OK);

        m_current_instance = -1;
    }
}

void
SocketFrontEnd::socket_reset_option (int /*client_id*/)
{
    uint32 siid;

    SCIM_DEBUG_FRONTEND (2) << " socket_reset_option.\n";

    if (m_receive_trans.get_data (siid)) {

        SCIM_DEBUG_FRONTEND (3) << "  SI (" << siid << ").\n";

        m_current_instance = (int) siid;

        reset_option ((int) siid);
        m_send_trans.put_command (SCIM_TRANS_CMD_OK);

        m_current_instance = -1;
    }
}

void
SocketFrontEnd::socket_reset (int /*client_id*/)
{
    uint32 siid;

    SCIM_DEBUG_FRONTEND (2) << " socket_reset.\n";

    if (m_receive_trans.get_data (siid)) {

        SCIM_DEBUG_FRONTEND (3) << "  SI (" << siid << ").\n";

        m_current_instance = (int) siid;

        reset ((int) siid);
        m_send_trans.put_command (SCIM_TRANS_CMD_OK);

        m_current_instance = -1;
    }
}

void
SocketFrontEnd::socket_focus_in (int /*client_id*/)
{
    uint32 siid;

    SCIM_DEBUG_FRONTEND (2) << " socket_focus_in.\n";

    if (m_receive_trans.get_data (siid)) {

        SCIM_DEBUG_FRONTEND (3) << "  SI (" << siid << ").\n";

        m_current_instance = (int) siid;

        focus_in ((int) siid);
        m_send_trans.put_command (SCIM_TRANS_CMD_OK);

        m_current_instance = -1;
    }
}

void
SocketFrontEnd::socket_focus_out (int /*client_id*/)
{
    uint32 siid;

    SCIM_DEBUG_FRONTEND (2) << " socket_focus_out.\n";

    if (m_receive_trans.get_data (siid)) {

        SCIM_DEBUG_FRONTEND (3) << "  SI (" << siid << ").\n";

        m_current_instance = (int) siid;

        focus_out ((int) siid);
        m_send_trans.put_command (SCIM_TRANS_CMD_OK);

        m_current_instance = -1;
    }
}

void
SocketFrontEnd::socket_trigger_property (int /*client_id*/)
{
    uint32 siid;
    String property;

    SCIM_DEBUG_FRONTEND (2) << " socket_trigger_property.\n";

    if (m_receive_trans.get_data (siid) &&
        m_receive_trans.get_data (property)) {

        SCIM_DEBUG_FRONTEND (3) << "  SI (" << siid << ").\n";

        m_current_instance = (int) siid;

        trigger_property ((int) siid, property);
        m_send_trans.put_command (SCIM_TRANS_CMD_OK);

        m_current_instance = -1;
    }
}

void
SocketFrontEnd::socket_process_helper_event (int /*client_id*/)
{
    uint32 siid;
    String helper_uuid;
    Transaction trans;

    SCIM_DEBUG_FRONTEND (2) << " socket_process_helper_event.\n";

    if (m_receive_trans.get_data (siid) &&
        m_receive_trans.get_data (helper_uuid) &&
        m_receive_trans.get_data (trans)) {

        SCIM_DEBUG_FRONTEND (3) << "  SI (" << siid << ").\n";

        m_current_instance = (int) siid;

        process_helper_event ((int) siid, helper_uuid, trans);
        m_send_trans.put_command (SCIM_TRANS_CMD_OK);

        m_current_instance = -1;
    }
}

void
SocketFrontEnd::socket_update_client_capabilities (int /*client_id*/)
{
    uint32 siid;
    uint32 cap;

    SCIM_DEBUG_FRONTEND (2) << " socket_update_client_capabilities.\n";

    if (m_receive_trans.get_data (siid) && m_receive_trans.get_data (cap)) {

        SCIM_DEBUG_FRONTEND (3) << "  SI (" << siid << ").\n";

        m_current_instance = (int) siid;

        update_client_capabilities ((int) siid, cap);

        m_send_trans.put_command (SCIM_TRANS_CMD_OK);

        m_current_instance = -1;
    }
}


void
SocketFrontEnd::socket_flush_config (int /*client_id*/)
{
    if (m_config_readonly || m_config.null ())
        return;

    SCIM_DEBUG_FRONTEND (2) << " socket_flush_config.\n";

    if (m_config->flush ())
        m_send_trans.put_command (SCIM_TRANS_CMD_OK);
}

void
SocketFrontEnd::socket_erase_config (int /*client_id*/)
{
    if (m_config_readonly || m_config.null ())
        return;

    String key;

    SCIM_DEBUG_FRONTEND (2) << " socket_erase_config.\n";

    if (m_receive_trans.get_data (key)) {

        SCIM_DEBUG_FRONTEND (3) << "  Key   (" << key << ").\n";

        if (m_config->erase (key))
            m_send_trans.put_command (SCIM_TRANS_CMD_OK);
    }
}

void
SocketFrontEnd::socket_reload_config (int /*client_id*/)
{
    static timeval last_timestamp = {0, 0};

    if (m_config.null ())
        return;

    SCIM_DEBUG_FRONTEND (2) << " socket_reload_config.\n";

    timeval timestamp;

    gettimeofday (&timestamp, 0);

    if (timestamp.tv_sec >= last_timestamp.tv_sec)
        m_config->reload ();

    gettimeofday (&last_timestamp, 0);

    m_send_trans.put_command (SCIM_TRANS_CMD_OK);
}

void
SocketFrontEnd::socket_set_display_name (int /*client_id*/)
{
    String name;

    if (m_receive_trans.get_data (name) && name.length () > 0)
        m_display = name;

    m_send_trans.put_command (SCIM_TRANS_CMD_OK);
}

#if HAVE_PKGMGR_INFO
int app_list_cb (pkgmgrinfo_appinfo_h handle, void *user_data)
{
    int ret;
    char *app_id = NULL;
    char *pkg_id = NULL, *pkg_label = NULL, *pkg_type = NULL, *pkg_icon_path = NULL;
    pkgmgrinfo_appinfo_h appinfo_handle;
    pkgmgrinfo_pkginfo_h pkginfo_handle;
    HelperInfo helper_info;
    size_t i;

    /* Get appid */
    ret = pkgmgrinfo_appinfo_get_appid (handle, &app_id);
    if (ret != PMINFO_R_OK)
        return 0;

    ret = pkgmgrinfo_appinfo_get_appinfo (app_id, &appinfo_handle);
    if (ret != PMINFO_R_OK)
        return 0;

    /* Get package id */
    ret = pkgmgrinfo_appinfo_get_pkgname (appinfo_handle, &pkg_id);
    if (ret != PMINFO_R_OK) {
        pkgmgrinfo_appinfo_destroy_appinfo (appinfo_handle);
        return 0;
    }

    /* Get package info handle */
    ret = pkgmgrinfo_pkginfo_get_pkginfo (pkg_id, &pkginfo_handle);
    if (ret != PMINFO_R_OK) {
        pkgmgrinfo_appinfo_destroy_appinfo (appinfo_handle);
        return 0;
    }

    /* Get the type of package */
    ret = pkgmgrinfo_pkginfo_get_type (pkginfo_handle, &pkg_type);
    if (ret == PMINFO_R_OK && pkg_type && !strncmp (pkg_type, "wgt", 3)) {
        /* Get the label of package */
        if (pkgmgrinfo_pkginfo_get_label (pkginfo_handle, &pkg_label) == PMINFO_R_OK)
            helper_info.name = (pkg_label ? scim::String (pkg_label) : scim::String (""));

        /* Get the icon path of package */
        if (pkgmgrinfo_pkginfo_get_icon (pkginfo_handle, &pkg_icon_path) == PMINFO_R_OK)
            helper_info.icon = (pkg_icon_path ? scim::String (pkg_icon_path) : scim::String (""));

        // FIXME : need to get UUID
        helper_info.uuid = (app_id ? scim::String (app_id) : scim::String (""));

        helper_info.option = scim::SCIM_HELPER_STAND_ALONE | scim::SCIM_HELPER_NEED_SCREEN_INFO |
            scim::SCIM_HELPER_NEED_SPOT_LOCATION_INFO | scim::SCIM_HELPER_AUTO_RESTART;

        for (i = 0; i < __helpers.size (); ++i) {
            if (__helpers [i].first.uuid == helper_info.uuid)
                break;
        }

        if (i == __helpers.size ())
            __helpers.push_back (std::make_pair (helper_info, String ("ise-web-helper-agent")));
    }

    pkgmgrinfo_pkginfo_destroy_pkginfo (pkginfo_handle);
    pkgmgrinfo_appinfo_destroy_appinfo (appinfo_handle);

    return 0;
}
#endif

void
SocketFrontEnd::socket_update_ise_list (int /*client_id*/)
{
    String strName;
    std::vector<String> install_modules;
    std::vector<String> imengine_list;
    std::vector<String> helper_list;
    size_t i = 0, j = 0;

    if (m_receive_trans.get_data (strName) && strName.length () > 0) {
        //std::cout << "ISE name list:" << strName << "\n";
        //scim_split_string_list (name_list, strName);

        scim_get_imengine_module_list (imengine_list);
        scim_get_helper_module_list (helper_list);

        for (i = 0; i < imengine_list.size (); ++i) {
            install_modules.push_back (imengine_list [i]);
            if (std::find (__load_engine_list.begin (), __load_engine_list.end (), imengine_list [i]) == __load_engine_list.end ()) {
                SCIM_DEBUG_FRONTEND (3) << "add_module " << imengine_list [i]  << " in " << __FUNCTION__ << "\n";
                //add_module (m_config, imengine_list [i], true);
                add_module_info (m_config, imengine_list [i]);
                __load_engine_list.push_back (imengine_list [i]);
            }
        }

        HelperModule module;
        HelperInfo   info;
        for (i = 0; i < helper_list.size (); ++i) {
            install_modules.push_back (helper_list [i]);
            if (std::find (__load_engine_list.begin (), __load_engine_list.end (), helper_list [i]) == __load_engine_list.end ()) {
                if (module.load (helper_list [i]) && module.valid ()) {
                    size_t num = module.number_of_helpers ();
                    for (j = 0; j < num; ++j) {
                        if (module.get_helper_info (j, info))
                            __helpers.push_back (std::make_pair (info, helper_list [i]));
                    }
                    __load_engine_list.push_back (helper_list [i]);
                }
                module.unload ();
            }
        }

        /* Try to find uninstall ISEs */
        for (i = 0; i < __load_engine_list.size (); ++i) {
            if (std::find (install_modules.begin (), install_modules.end (), __load_engine_list [i]) == install_modules.end ()) {
                HelperRepository tmp_helpers = __helpers;
                __helpers.clear ();
                for (j = 0; j < tmp_helpers.size (); ++j) {
                    if (std::find (install_modules.begin (), install_modules.end (), tmp_helpers [j].second) != install_modules.end ())
                        __helpers.push_back (tmp_helpers [j]);
                }
                __load_engine_list = install_modules;
                break;
            }
        }

        /* Get the information of Web IMEs */
#if HAVE_PKGMGR_INFO
        int ret;
        pkgmgrinfo_appinfo_filter_h handle;
        ret = pkgmgrinfo_appinfo_filter_create (&handle);
        if (ret != PMINFO_R_OK)
            return;

        ret = pkgmgrinfo_appinfo_filter_add_string (handle, PMINFO_APPINFO_PROP_APP_CATEGORY, "http://tizen.org/category/ime");
        if (ret == PMINFO_R_OK) {
            pkgmgrinfo_appinfo_filter_foreach_appinfo (handle, app_list_cb, NULL);
        }

        pkgmgrinfo_appinfo_filter_destroy (handle);
#endif
    }

    m_send_trans.put_command (SCIM_TRANS_CMD_OK);
}

void
SocketFrontEnd::socket_get_config_string (int /*client_id*/)
{
    if (m_config.null ()) return;

    String key;

    SCIM_DEBUG_FRONTEND (2) << " socket_get_config_string.\n";

    if (m_receive_trans.get_data (key)) {
        String value;

        SCIM_DEBUG_FRONTEND (3) << "  Key (" << key << ").\n";

        if (m_config->read (key, &value)) {
            m_send_trans.put_data (value);
            m_send_trans.put_command (SCIM_TRANS_CMD_OK);
        }
    }
}

void
SocketFrontEnd::socket_set_config_string (int /*client_id*/)
{
    if (m_config_readonly || m_config.null ())
        return;

    String key;
    String value;

    SCIM_DEBUG_FRONTEND (2) << " socket_set_config_string.\n";

    if (m_receive_trans.get_data (key) &&
        m_receive_trans.get_data (value)) {

        SCIM_DEBUG_FRONTEND (3) << "  Key   (" << key << ").\n";
        SCIM_DEBUG_FRONTEND (3) << "  Value (" << value << ").\n";

        if (m_config->write (key, value))
            m_send_trans.put_command (SCIM_TRANS_CMD_OK);
    }
}

void
SocketFrontEnd::socket_get_config_int (int /*client_id*/)
{
    if (m_config.null ()) return;

    String key;

    SCIM_DEBUG_FRONTEND (2) << " socket_get_config_int.\n";

    if (m_receive_trans.get_data (key)) {

        SCIM_DEBUG_FRONTEND (3) << "  Key (" << key << ").\n";

        int value;
        if (m_config->read (key, &value)) {
            m_send_trans.put_data ((uint32) value);
            m_send_trans.put_command (SCIM_TRANS_CMD_OK);
        }
    }
}

void
SocketFrontEnd::socket_set_config_int (int /*client_id*/)
{
    if (m_config_readonly || m_config.null ())
        return;

    String key;
    uint32 value;

    SCIM_DEBUG_FRONTEND (2) << " socket_set_config_int.\n";

    if (m_receive_trans.get_data (key) &&
        m_receive_trans.get_data (value)) {

        SCIM_DEBUG_FRONTEND (3) << "  Key   (" << key << ").\n";
        SCIM_DEBUG_FRONTEND (3) << "  Value (" << value << ").\n";

        if (m_config->write (key, (int) value))
            m_send_trans.put_command (SCIM_TRANS_CMD_OK);
    }
}

void
SocketFrontEnd::socket_get_config_bool (int /*client_id*/)
{
    if (m_config.null ()) return;

    String key;

    SCIM_DEBUG_FRONTEND (2) << " socket_get_config_bool.\n";

    if (m_receive_trans.get_data (key)) {
        bool value;

        SCIM_DEBUG_FRONTEND (3) << "  Key (" << key << ").\n";

        if (m_config->read (key, &value)) {
            m_send_trans.put_data ((uint32) value);
            m_send_trans.put_command (SCIM_TRANS_CMD_OK);
        }
    }
}

void
SocketFrontEnd::socket_set_config_bool (int /*client_id*/)
{
    if (m_config_readonly || m_config.null ())
        return;

    String key;
    uint32 value;

    SCIM_DEBUG_FRONTEND (2) << " socket_set_config_bool.\n";

    if (m_receive_trans.get_data (key) &&
        m_receive_trans.get_data (value)) {

        SCIM_DEBUG_FRONTEND (3) << "  Key   (" << key << ").\n";
        SCIM_DEBUG_FRONTEND (3) << "  Value (" << value << ").\n";

        if (m_config->write (key, (bool) value))
            m_send_trans.put_command (SCIM_TRANS_CMD_OK);
    }
}

void
SocketFrontEnd::socket_get_config_double (int /*client_id*/)
{
    if (m_config.null ()) return;

    String key;

    SCIM_DEBUG_FRONTEND (2) << " socket_get_config_double.\n";

    if (m_receive_trans.get_data (key)) {
        double value;

        SCIM_DEBUG_FRONTEND (3) << "  Key (" << key << ").\n";

        if (m_config->read (key, &value)) {
            char buf [80];
            snprintf (buf, 79, "%lE", value);
            m_send_trans.put_data (String (buf));
            m_send_trans.put_command (SCIM_TRANS_CMD_OK);
        }
    }
}

void
SocketFrontEnd::socket_set_config_double (int /*client_id*/)
{
    if (m_config_readonly || m_config.null ())
        return;

    String key;
    String str;

    SCIM_DEBUG_FRONTEND (2) << " socket_set_config_double.\n";

    if (m_receive_trans.get_data (key) &&
        m_receive_trans.get_data (str)) {
        double value;
        sscanf (str.c_str (), "%lE", &value);

        SCIM_DEBUG_FRONTEND (3) << "  Key   (" << key << ").\n";
        SCIM_DEBUG_FRONTEND (3) << "  Value (" << value << ").\n";

        if (m_config->write (key, value))
            m_send_trans.put_command (SCIM_TRANS_CMD_OK);
    }
}

void
SocketFrontEnd::socket_get_config_vector_string (int /*client_id*/)
{
    if (m_config.null ()) return;

    String key;

    SCIM_DEBUG_FRONTEND (2) << " socket_get_config_vector_string.\n";

    if (m_receive_trans.get_data (key)) {
        std::vector <String> vec;

        SCIM_DEBUG_FRONTEND (3) << "  Key (" << key << ").\n";

        if (m_config->read (key, &vec)) {
            m_send_trans.put_data (vec);
            m_send_trans.put_command (SCIM_TRANS_CMD_OK);
        }
    }
}

void
SocketFrontEnd::socket_set_config_vector_string (int /*client_id*/)
{
    if (m_config_readonly || m_config.null ())
        return;

    String key;
    std::vector<String> vec;

    SCIM_DEBUG_FRONTEND (2) << " socket_set_config_vector_string.\n";

    if (m_receive_trans.get_data (key) &&
        m_receive_trans.get_data (vec)) {

        SCIM_DEBUG_FRONTEND (3) << "  Key (" << key << ").\n";

        if (m_config->write (key, vec))
            m_send_trans.put_command (SCIM_TRANS_CMD_OK);
    }
}

void
SocketFrontEnd::socket_get_config_vector_int (int /*client_id*/)
{
    if (m_config.null ()) return;

    String key;

    SCIM_DEBUG_FRONTEND (2) << " socket_get_config_vector_int.\n";

    if (m_receive_trans.get_data (key)) {
        std::vector <int> vec;

        SCIM_DEBUG_FRONTEND (3) << "  Key (" << key << ").\n";

        if (m_config->read (key, &vec)) {
            std::vector <uint32> reply;

            for (uint32 i=0; i<vec.size (); ++i)
                reply.push_back ((uint32) vec[i]);

            m_send_trans.put_data (reply);
            m_send_trans.put_command (SCIM_TRANS_CMD_OK);
        }
    }
}

void
SocketFrontEnd::socket_set_config_vector_int (int /*client_id*/)
{
    if (m_config_readonly || m_config.null ())
        return;

    String key;
    std::vector<uint32> vec;

    SCIM_DEBUG_FRONTEND (2) << " socket_set_config_vector_int.\n";

    if (m_receive_trans.get_data (key) &&
        m_receive_trans.get_data (vec)) {
        std::vector<int> req;

        SCIM_DEBUG_FRONTEND (3) << "  Key (" << key << ").\n";

        for (uint32 i=0; i<vec.size (); ++i)
            req.push_back ((int) vec[i]);

        if (m_config->write (key, req))
            m_send_trans.put_command (SCIM_TRANS_CMD_OK);
    }
}

void
SocketFrontEnd::socket_load_file (int /*client_id*/)
{
    String filename;
    char *bufptr = 0;
    size_t filesize = 0;

    SCIM_DEBUG_FRONTEND (2) << " socket_load_file.\n";

    if (m_receive_trans.get_data (filename)) {
        SCIM_DEBUG_FRONTEND (3) << "  File (" << filename << ").\n";

        filesize = scim_load_file (filename, &bufptr);
        if (filesize > 0) {
            m_send_trans.put_data (bufptr, filesize);
            m_send_trans.put_command (SCIM_TRANS_CMD_OK);
        }

        delete [] bufptr;
    }
}

void
SocketFrontEnd::socket_turn_on_log (int /*client_id*/)
{
    uint32 isOn;
    SCIM_DEBUG_FRONTEND (2) << " socket_turn_on_log.\n";

    if (m_receive_trans.get_data (isOn)) {
        if (isOn) {
            DebugOutput::enable_debug (SCIM_DEBUG_AllMask);
            DebugOutput::set_verbose_level (7);
        } else {
            DebugOutput::disable_debug (SCIM_DEBUG_AllMask);
            DebugOutput::set_verbose_level (0);
        }
        m_send_trans.put_command (SCIM_TRANS_CMD_OK);
        return;
    }

    m_send_trans.put_command (SCIM_TRANS_CMD_FAIL);
    return;
}

void
SocketFrontEnd::reload_config_callback (const ConfigPointer &config)
{
    SCIM_DEBUG_FRONTEND (1) << "Reload configuration.\n";

    int max_clients = -1;

    m_config_readonly = config->read (String (SCIM_CONFIG_FRONTEND_SOCKET_CONFIG_READONLY), false);
    max_clients = config->read (String (SCIM_CONFIG_FRONTEND_SOCKET_MAXCLIENTS), -1);

    m_socket_server.set_max_clients (max_clients);
}

/*
vi:ts=4:nowrap:expandtab
*/
