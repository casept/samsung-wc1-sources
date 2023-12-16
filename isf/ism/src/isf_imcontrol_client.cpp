/*
 * ISF(Input Service Framework)
 *
 * ISF is based on SCIM 1.4.7 and extended for supporting more mobile fitable.
 * Copyright (c) 2012-2014 Samsung Electronics Co., Ltd.
 *
 * Contact: Haifeng Deng <haifeng.deng@samsung.com>, Hengliang Luo <hl.luo@samsung.com>
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the
 * Free Software Foundation; either version 2.1 of the License, or (at your option)
 * any later version.
 *
 * This library is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public
 * License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library; if not, write to the Free Software Foundation, Inc., 51
 * Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 *
 */

#define Uses_SCIM_TRANSACTION
#define Uses_ISF_IMCONTROL_CLIENT


#include <string.h>
#include <vconf.h>
#include "scim.h"


namespace scim
{

typedef Signal1<void, int> IMControlClientSignalVoid;

class IMControlClient::IMControlClientImpl
{
    SocketClient                                m_socket_imclient2panel;
    SocketClient                                m_socket_panel2imclient;
    int                                         m_socket_timeout;
    uint32                                      m_socket_i2p_magic_key;
    uint32                                      m_socket_p2i_magic_key;
    Transaction                                 m_trans;

    IMControlClientSignalVoid                   m_signal_show_ise;
    IMControlClientSignalVoid                   m_signal_hide_ise;

public:
    IMControlClientImpl ()
          : m_socket_timeout (scim_get_default_socket_timeout ()),
            m_socket_i2p_magic_key (0),
            m_socket_p2i_magic_key (0) {
    }

    int open_connection (void) {
        String config = "";
        const char *p = getenv ("DISPLAY");
        String display;
        if (p) display = String (p);

        SocketAddress addr (scim_get_default_panel_socket_address (display));

        if (m_socket_imclient2panel.is_connected ()) close_connection ();

        bool ret=false, ret2=false;
        int count = 0;

        /* Try three times. */
        while (1) {
            ret = m_socket_imclient2panel.connect (addr);
            ret2 = m_socket_panel2imclient.connect (addr);
            if (!ret) {
                scim_usleep (100000);
                /* Do not launch panel process here, let the SocketFrontend to create panel process */
                /*
                scim_launch_panel (true, config, display, NULL);
                */
                for (int i = 0; i < 200; ++i) {
                    if (m_socket_imclient2panel.connect (addr)) {
                        ret = true;
                        break;
                    }
                    scim_usleep (100000);
                }
            }

            if (ret && scim_socket_open_connection (m_socket_i2p_magic_key, String ("IMControl_Active"), String ("Panel"), m_socket_imclient2panel, m_socket_timeout)) {
                if (ret2 && scim_socket_open_connection (m_socket_p2i_magic_key, String ("IMControl_Passive"), String ("Panel"), m_socket_panel2imclient, m_socket_timeout))
                    break;
            }
            m_socket_imclient2panel.close ();
            m_socket_panel2imclient.close ();

            if (count++ >= 3) break;

            scim_usleep (100000);
        }

        return m_socket_imclient2panel.get_id ();
    }

    void close_connection (void) {
        m_socket_imclient2panel.close ();
        m_socket_panel2imclient.close ();
        m_socket_i2p_magic_key = 0;
        m_socket_p2i_magic_key = 0;
    }

    bool is_connected (void) const {
        return (m_socket_imclient2panel.is_connected () && m_socket_panel2imclient.is_connected ());
    }

    int  get_panel2imclient_connection_number  (void) const {
        return m_socket_panel2imclient.get_id ();
    }

    bool prepare (void) {
        if (!m_socket_imclient2panel.is_connected ()) return false;

        m_trans.clear ();
        m_trans.put_command (SCIM_TRANS_CMD_REQUEST);
        m_trans.put_data (m_socket_i2p_magic_key);

        return true;
    }

    bool send (void) {
        if (!m_socket_imclient2panel.is_connected ()) return false;
        if (m_trans.get_data_type () != SCIM_TRANS_DATA_UNKNOWN)
            return m_trans.write_to_socket (m_socket_imclient2panel, 0x4d494353);
        return false;
    }

    void set_active_ise_by_uuid (const char* uuid) {
        int cmd;
        m_trans.put_command (ISM_TRANS_CMD_SET_ACTIVE_ISE_BY_UUID);
        m_trans.put_data (uuid, strlen (uuid)+1);
        m_trans.write_to_socket (m_socket_imclient2panel);
        if (!m_trans.read_from_socket (m_socket_imclient2panel, m_socket_timeout))
            std::cerr << __func__ << " read_from_socket() may be timeout \n";

        if (m_trans.get_command (cmd) && cmd == SCIM_TRANS_CMD_REPLY &&
                m_trans.get_command (cmd) && cmd == SCIM_TRANS_CMD_OK) {
        } else {
            std::cerr << __func__ << " get_command() or get_data() may fail!!!\n";
        }
    }

    void set_initial_ise_by_uuid (const char* uuid) {
        int cmd;
        m_trans.put_command (ISM_TRANS_CMD_SET_INITIAL_ISE_BY_UUID);
        m_trans.put_data (uuid, strlen (uuid)+1);
        m_trans.write_to_socket (m_socket_imclient2panel);
        if (!m_trans.read_from_socket (m_socket_imclient2panel, m_socket_timeout))
            std::cerr << __func__ << " read_from_socket() may be timeout \n";

        if (m_trans.get_command (cmd) && cmd == SCIM_TRANS_CMD_REPLY &&
                m_trans.get_command (cmd) && cmd == SCIM_TRANS_CMD_OK) {
        } else {
            std::cerr << __func__ << " get_command() or get_data() may fail!!!\n";
        }

        vconf_set_str ("db/isf/csc_initial_uuid", uuid);
    }

    void get_active_ise (String &uuid) {
        int    cmd;
        String strTemp;

        m_trans.put_command (ISM_TRANS_CMD_GET_ACTIVE_ISE);
        m_trans.write_to_socket (m_socket_imclient2panel);
        if (!m_trans.read_from_socket (m_socket_imclient2panel, m_socket_timeout))
            std::cerr << __func__ << " read_from_socket() may be timeout \n";

        if (m_trans.get_command (cmd) && cmd == SCIM_TRANS_CMD_REPLY &&
                m_trans.get_command (cmd) && cmd == SCIM_TRANS_CMD_OK &&
                m_trans.get_data (strTemp)) {
            uuid = strTemp;
        } else {
            std::cerr << __func__ << " get_command() or get_data() may fail!!!\n";
        }
    }

    void get_ise_list (int* count, char*** iselist) {
        int cmd;
        uint32 count_temp = 0;
        char **buf = NULL;
        size_t len;
        char *buf_temp = NULL;

        m_trans.put_command (ISM_TRANS_CMD_GET_ISE_LIST);
        m_trans.write_to_socket (m_socket_imclient2panel);
        if (!m_trans.read_from_socket (m_socket_imclient2panel, m_socket_timeout))
            std::cerr << __func__ << " read_from_socket() may be timeout \n";

        if (m_trans.get_command (cmd) && cmd == SCIM_TRANS_CMD_REPLY &&
                m_trans.get_command (cmd) && cmd == SCIM_TRANS_CMD_OK &&
                m_trans.get_data (count_temp) ) {
            if (count)
                *count = count_temp;
        } else {
            if (count)
                *count = 0;
            std::cerr << __func__ << " get_command() or get_data() may fail!!!\n";
        }

        if (iselist) {
            if (count_temp > 0) {
                buf = (char**)calloc (1, count_temp * sizeof (char*));
                if (buf) {
                    for (uint32 i = 0; i < count_temp; i++) {
                        if (m_trans.get_data (&buf_temp, len)) {
                            if (buf_temp) {
                                buf[i] = strdup (buf_temp);
                                delete [] buf_temp;
                            }
                        }
                    }
                }
            }

            *iselist = buf;
        }
    }

    void get_ise_info (const char* uuid, String &name, String &language, int &type, int &option, String &module_name)
    {
        int    cmd;
        uint32 tmp_type, tmp_option;
        String tmp_name, tmp_language, tmp_module_name;

        m_trans.put_command (ISM_TRANS_CMD_GET_ISE_INFORMATION);
        m_trans.put_data (String (uuid));
        m_trans.write_to_socket (m_socket_imclient2panel);
        if (!m_trans.read_from_socket (m_socket_imclient2panel, m_socket_timeout))
            std::cerr << __func__ << " read_from_socket() may be timeout \n";

        if (m_trans.get_command (cmd) && cmd == SCIM_TRANS_CMD_REPLY &&
                m_trans.get_command (cmd) && cmd == SCIM_TRANS_CMD_OK &&
                m_trans.get_data (tmp_name) && m_trans.get_data (tmp_language) &&
                m_trans.get_data (tmp_type) && m_trans.get_data (tmp_option) && m_trans.get_data (tmp_module_name)) {
            name     = tmp_name;
            language = tmp_language;
            type     = tmp_type;
            option   = tmp_option;
            module_name = tmp_module_name;
        } else {
            std::cerr << __func__ << " get_command() or get_data() may fail!!!\n";
        }
    }

    void reset_ise_option (void) {
        int cmd;

        m_trans.put_command (ISM_TRANS_CMD_RESET_ISE_OPTION);
        m_trans.write_to_socket (m_socket_imclient2panel);
        if (!m_trans.read_from_socket (m_socket_imclient2panel, m_socket_timeout))
            std::cerr << __func__ << " read_from_socket() may be timeout \n";

        if (m_trans.get_command (cmd) && cmd == SCIM_TRANS_CMD_REPLY &&
                m_trans.get_command (cmd) && cmd == SCIM_TRANS_CMD_OK) {
            ;
        } else {
            std::cerr << __func__ << " get_command() is failed!!!\n";
        }
    }

    void set_active_ise_to_default (void) {
        m_trans.put_command (ISM_TRANS_CMD_RESET_DEFAULT_ISE);
    }

    void show_ise_selector (void) {
        m_trans.put_command (ISM_TRANS_CMD_SHOW_ISF_CONTROL);
    }

    void show_ise_option_window (void) {
        m_trans.put_command (ISM_TRANS_CMD_SHOW_ISE_OPTION_WINDOW);
    }
};

IMControlClient::IMControlClient ()
        : m_impl (new IMControlClientImpl ())
{
}

IMControlClient::~IMControlClient ()
{
    delete m_impl;
}

int
IMControlClient::open_connection (void)
{
    return m_impl->open_connection ();
}

void
IMControlClient::close_connection (void)
{
    m_impl->close_connection ();
}

bool IMControlClient::is_connected (void) const
{
    return m_impl->is_connected ();
}

int IMControlClient::get_panel2imclient_connection_number (void) const
{
    return m_impl->get_panel2imclient_connection_number ();
}

bool
IMControlClient::prepare (void)
{
    return m_impl->prepare ();
}

bool
IMControlClient::send (void)
{
    return m_impl->send ();
}

void IMControlClient::set_active_ise_by_uuid (const char* uuid)
{
    m_impl->set_active_ise_by_uuid (uuid);
}

void IMControlClient::set_initial_ise_by_uuid (const char* uuid)
{
    m_impl->set_initial_ise_by_uuid (uuid);
}

void IMControlClient::get_active_ise (String &uuid)
{
    m_impl->get_active_ise (uuid);
}

void IMControlClient::get_ise_list (int* count, char*** iselist)
{
    m_impl->get_ise_list (count, iselist);
}

void IMControlClient::get_ise_info (const char* uuid, String &name, String &language, int &type, int &option, String &module_name)
{
    m_impl->get_ise_info (uuid, name, language, type, option, module_name);
}

void IMControlClient::reset_ise_option (void)
{
    m_impl->reset_ise_option ();
}

void IMControlClient::set_active_ise_to_default (void)
{
    m_impl->set_active_ise_to_default ();
}

void IMControlClient::show_ise_selector (void)
{
    m_impl->show_ise_selector ();
}

void IMControlClient::show_ise_option_window (void)
{
    m_impl->show_ise_option_window ();
}

};

/*
vi:ts=4:nowrap:ai:expandtab
*/
