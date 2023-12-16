/** @file scim_helper_manager.cpp
 *  @brief Implementation of class HelperManager.
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
 * 1. Launch scim-launcher process when connection is failed
 * 2. Replace helper manager server with socket frontend
 * 3. Add some new interface APIs in HelperManager class
 *
 * $Id: scim_helper_manager.cpp,v 1.2 2005/01/05 13:41:10 suzhe Exp $
 *
 */

#define Uses_SCIM_IMENGINE_MODULE
#define Uses_SCIM_HELPER_MODULE
#define Uses_SCIM_HELPER_MANAGER
#define Uses_SCIM_SOCKET
#define Uses_SCIM_TRANSACTION
#define Uses_SCIM_TRANS_COMMAND
#define Uses_C_STDLIB

#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include "scim_private.h"
#include "scim.h"
#include "scim_utility.h"


namespace scim {


#ifndef SCIM_HELPER_MANAGER_PROGRAM
  #define SCIM_HELPER_MANAGER_PROGRAM  (SCIM_LIBEXECDIR "/scim-helper-manager")
#endif


class HelperManager::HelperManagerImpl
{
    std::vector <HelperInfo> m_helpers;

    uint32                   m_socket_key;

public:
    SocketClient             m_socket_client;
    int                      m_socket_timeout;

public:
    HelperManagerImpl ()
        : m_socket_key (0),
          m_socket_timeout (scim_get_default_socket_timeout ())
    {
        if (!open_connection ())
            std::cerr << __func__ << " open_connection () is failed!!!\n";
    }

    ~HelperManagerImpl ()
    {
        m_socket_client.close ();
    }

    unsigned int number_of_helpers (void) const
    {
        return m_helpers.size ();
    }

    bool get_helper_info (unsigned int idx, HelperInfo &info) const
    {
        if (idx < m_helpers.size ()) {
            info = m_helpers [idx];
            return true;
        }
        return false;
    }

    void run_helper (const String &uuid, const String &config_name, const String &display)
    {
        SCIM_DEBUG_MAIN(1) << __FUNCTION__ << "...\n";
        if (!uuid.length () || !m_helpers.size ())
            return;
        if (!m_socket_client.is_connected () && !open_connection ())
            return;

        Transaction trans;
        for (int i = 0; i < 3; ++i) {
            trans.clear ();
            trans.put_command (SCIM_TRANS_CMD_REQUEST);
            trans.put_data (m_socket_key);
            trans.put_command (SCIM_TRANS_CMD_HELPER_MANAGER_RUN_HELPER);
            trans.put_data (uuid);
            trans.put_data (config_name);
            trans.put_data (display);

            if (trans.write_to_socket (m_socket_client))
                return;

            m_socket_client.close ();
            if (!open_connection ())
                break;
        }
    }

    void stop_helper (const String &name)
    {
        SCIM_DEBUG_MAIN(1) << __FUNCTION__ << "...\n";
        if (!name.length ())
            return;
        if (!m_socket_client.is_connected () && !open_connection ())
            return;

        Transaction trans;
        for (int i = 0; i < 3; ++i) {
            trans.clear ();
            trans.put_command (SCIM_TRANS_CMD_REQUEST);
            trans.put_data (m_socket_key);
            trans.put_command (SCIM_TRANS_CMD_HELPER_MANAGER_STOP_HELPER);
            trans.put_data (name);

            int cmd;
            if (trans.write_to_socket (m_socket_client) &&
                trans.read_from_socket (m_socket_client) &&
                trans.get_command (cmd) && cmd == SCIM_TRANS_CMD_REPLY &&
                trans.get_command (cmd) && cmd == SCIM_TRANS_CMD_OK)
                return;

            m_socket_client.close ();
            if (!open_connection ())
                break;
        }
    }

    int get_active_ise_list (std::vector<String> &strlist)
    {
        SCIM_DEBUG_MAIN(1) << __FUNCTION__ << "...\n";
        if (!m_socket_client.is_connected () && !open_connection ())
            return -1;

        strlist.clear ();

        Transaction trans;
        for (int i = 0; i < 3; ++i) {
            trans.clear ();
            trans.put_command (SCIM_TRANS_CMD_REQUEST);
            trans.put_data (m_socket_key);
            trans.put_command (ISM_TRANS_CMD_GET_ACTIVE_ISE_LIST);

            int cmd;
            if (trans.write_to_socket (m_socket_client) &&
                trans.read_from_socket (m_socket_client) &&
                trans.get_command (cmd) && cmd == SCIM_TRANS_CMD_REPLY &&
                trans.get_data (strlist))
                return (int)(strlist.size ());

            m_socket_client.close ();
            if (!open_connection ())
                break;
        }

        return -1;
    }

    int send_display_name (String &name)
    {
        SCIM_DEBUG_MAIN(1) << __FUNCTION__ << "...\n";
        if (!m_socket_client.is_connected () && !open_connection ())
            return -1;

        Transaction trans;
        for (int i = 0; i < 3; ++i) {
            trans.clear ();
            trans.put_command (SCIM_TRANS_CMD_REQUEST);
            trans.put_data (m_socket_key);
            trans.put_command (SCIM_TRANS_CMD_HELPER_MANAGER_SEND_DISPLAY);
            trans.put_data (name);

            int cmd;
            if (trans.write_to_socket (m_socket_client) &&
                trans.read_from_socket (m_socket_client) &&
                trans.get_command (cmd) && cmd == SCIM_TRANS_CMD_REPLY &&
                trans.get_command (cmd) && cmd == SCIM_TRANS_CMD_OK)
                return 0;

            m_socket_client.close ();
            if (!open_connection ())
                break;
        }

        return -1;
    }

    int send_ise_list (String &ise_list)
    {
        SCIM_DEBUG_MAIN(1) << __FUNCTION__ << "...\n";
        if (!m_socket_client.is_connected () && !open_connection ())
            return -1;

        Transaction trans;
        for (int i = 0; i < 3; ++i) {
            trans.clear ();
            trans.put_command (SCIM_TRANS_CMD_REQUEST);
            trans.put_data (m_socket_key);
            trans.put_command (SCIM_TRANS_CMD_HELPER_MANAGER_SEND_ISE_LIST);
            trans.put_data (ise_list);

            if (trans.write_to_socket (m_socket_client))
                return 0;

            m_socket_client.close ();
            if (!open_connection ())
                break;
        }
        return -1;
    }

    int turn_on_log (uint32 isOn)
    {
        SCIM_DEBUG_MAIN(1) << __FUNCTION__ << "...\n";
        if (!m_socket_client.is_connected () && !open_connection ())
            return -1;

        Transaction trans;
        for (int i = 0; i < 3; ++i) {
            trans.clear ();
            trans.put_command (SCIM_TRANS_CMD_REQUEST);
            trans.put_data (m_socket_key);
            trans.put_command (ISM_TRANS_CMD_TURN_ON_LOG);
            trans.put_data (isOn);

            int cmd;
            if (trans.write_to_socket (m_socket_client) &&
                trans.read_from_socket (m_socket_client) &&
                trans.get_command (cmd) && cmd == SCIM_TRANS_CMD_REPLY &&
                trans.get_command (cmd) && cmd == SCIM_TRANS_CMD_OK)
                return 0;

            m_socket_client.close ();
            if (!open_connection ())
                break;
        }

        return -1;
    }

    void preload_keyboard_ise (const String &uuid)
    {
        SCIM_DEBUG_MAIN(1) << __FUNCTION__ << "...\n";
        if (!uuid.length ())
            return;
        if (!m_socket_client.is_connected () && !open_connection ())
            return;

        Transaction trans;
        for (int i = 0; i < 3; ++i) {
            trans.clear ();
            trans.put_command (SCIM_TRANS_CMD_REQUEST);
            trans.put_data (m_socket_key);
            trans.put_command (ISM_TRANS_CMD_PRELOAD_KEYBOARD_ISE);
            trans.put_data (uuid);

            int cmd;
            if (trans.write_to_socket (m_socket_client) &&
                trans.read_from_socket (m_socket_client) &&
                trans.get_command (cmd) && cmd == SCIM_TRANS_CMD_REPLY &&
                trans.get_command (cmd) && cmd == SCIM_TRANS_CMD_OK)
                return;

            m_socket_client.close ();
            if (!open_connection ())
                break;
        }
    }

    bool open_connection (void)
    {
        SCIM_DEBUG_MAIN(1) << __FUNCTION__ << "...\n";
        if (m_socket_client.is_connected ())
            return true;

        SocketAddress address (scim_get_default_helper_manager_socket_address ());

        if (address.valid ()) {
            if (!m_socket_client.connect (address)) {
                int  i = 0;
                bool bConnected = false;

                std::cerr << " Connecting to ISF(scim) server.";

                /* Make sure we are not waiting for more than 10 second */
                for (i = 0; i < 100; ++i) {
                    if (m_socket_client.connect (address)) {
                        bConnected = true;
                        break;
                    }
                    scim_usleep (100000);
                    std::cerr << ".";
                }
                std::cerr << " Connected :" << i << "\n";

                /* Do not launch SocketFrontEnd process here, let the immodule to create SocketFrontEnd process */
                if (false && !bConnected) {
                    /* Get modules list */
                    std::vector<String> engine_list;
                    std::vector<String> helper_list;
                    std::vector<String> load_engine_list;
                    scim_get_imengine_module_list (engine_list);
                    scim_get_helper_module_list (helper_list);

                    std::vector<String>::iterator it;
                    for (it = engine_list.begin (); it != engine_list.end (); it++) {
                        if (*it != "socket")
                            load_engine_list.push_back (*it);
                    }
                    for (it = helper_list.begin (); it != helper_list.end (); it++)
                        load_engine_list.push_back (*it);

                    std::cerr << "Launching a ISF daemon with Socket FrontEnd...\n";
                    const char *new_argv [] = { "--stay", 0 };
                    scim_launch (true,
                                 "simple",
                                 (load_engine_list.size () ? scim_combine_string_list (load_engine_list, ',') : "none"),
                                 "socket",
                                 const_cast<char**>(new_argv));

                    std::cerr << " Reconnecting to ISF(scim) server.";
                    for (i = 0; i < 100; ++i) {
                        if (m_socket_client.connect (address))
                            break;
                        scim_usleep (100000);
                        std::cerr << ".";
                    }
                    std::cerr << " Reconnected :" << i << "\n";
                }
            }
        }

        if (m_socket_client.is_connected () &&
            scim_socket_open_connection (m_socket_key,
                                         "HelperManager",
                                         "HelperLauncher",
                                         m_socket_client,
                                         m_socket_timeout)) {
            get_helper_list ();
            return true;
        }

        m_socket_client.close ();
        std::cerr << " helper_manager open_connection failed!\n";
        return false;
    }

    void get_helper_list (void)
    {
        SCIM_DEBUG_MAIN(1) << __FUNCTION__ << "...\n";
        if (!m_socket_client.is_connected () && !open_connection ())
            return;

        m_helpers.clear ();

        Transaction trans;
        for (int i = 0; i < 3; ++i) {
            trans.clear ();
            trans.put_command (SCIM_TRANS_CMD_REQUEST);
            trans.put_data (m_socket_key);
            trans.put_command (SCIM_TRANS_CMD_HELPER_MANAGER_GET_HELPER_LIST);

            int    cmd;
            uint32 num;
            if (trans.write_to_socket (m_socket_client) &&
                trans.read_from_socket (m_socket_client, m_socket_timeout) &&
                trans.get_command (cmd) && cmd == SCIM_TRANS_CMD_REPLY &&
                trans.get_data (num)) {
                for (uint32 j = 0; j < num; j++) {
                    HelperInfo info;
                    if (trans.get_data (info.uuid) &&
                        trans.get_data (info.name) &&
                        trans.get_data (info.icon) &&
                        trans.get_data (info.description) &&
                        trans.get_data (info.option)) {
                        m_helpers.push_back (info);
                    }
                }
                return;
            }
        }
    }
};

HelperManager::HelperManager ()
    : m_impl (new HelperManagerImpl ())
{
}

HelperManager::~HelperManager ()
{
    delete m_impl;
}

unsigned int
HelperManager::number_of_helpers (void) const
{
    return m_impl->number_of_helpers ();
}

bool
HelperManager::get_helper_info (unsigned int idx, HelperInfo &info) const
{
    return m_impl->get_helper_info (idx, info);
}

int
HelperManager::get_connection_number (void) const
{
    if (m_impl->m_socket_client.is_connected ())
        return m_impl->m_socket_client.get_id ();
    return -1;
}

bool
HelperManager::has_pending_event (void) const
{
    if (m_impl->m_socket_client.is_connected () && m_impl->m_socket_client.wait_for_data (0) > 0)
        return true;

    return false;
}

bool
HelperManager::filter_event (void)
{
    Transaction recv;
    if (!m_impl->m_socket_client.is_connected () || !recv.read_from_socket (m_impl->m_socket_client, m_impl->m_socket_timeout))
        return false;

    return true;
}

void
HelperManager::run_helper (const String &uuid, const String &config_name, const String &display) const
{
    m_impl->run_helper (uuid, config_name, display);
}

void
HelperManager::stop_helper (const String &name) const
{
    m_impl->stop_helper (name);
}

void
HelperManager::get_helper_list (void) const
{
    m_impl->get_helper_list ();
}

int
HelperManager::get_active_ise_list (std::vector<String> &strlist) const
{
    return m_impl->get_active_ise_list (strlist);
}

int
HelperManager::send_display_name (String &name) const
{
    return m_impl->send_display_name (name);
}

int
HelperManager::send_ise_list (String &ise_list) const
{
    return m_impl->send_ise_list (ise_list);
}

int
HelperManager::turn_on_log (uint32 isOn) const
{
    return m_impl->turn_on_log (isOn);
}

void
HelperManager::preload_keyboard_ise (const String &uuid) const
{
    m_impl->preload_keyboard_ise (uuid);
}

} // namespace scim

/*
vi:ts=4:nowrap:ai:expandtab
*/

