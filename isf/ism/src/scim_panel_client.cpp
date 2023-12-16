/** @file scim_panel_client.cpp
 *  @brief Implementation of class PanelClient.
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
 *    a. m_signal_select_aux, m_signal_reset_keyboard_ise
 *    b. m_signal_update_candidate_item_layout and m_signal_update_displayed_candidate_number
 *    c. m_signal_get_surrounding_text and m_signal_delete_surrounding_text
 *    d. m_signal_show_preedit_string, m_signal_hide_preedit_string, m_signal_update_preedit_string and m_signal_update_preedit_caret
 *    e. m_signal_candidate_more_window_show, m_signal_candidate_more_window_hide, m_signal_longpress_candidate
 *    f. m_signal_update_ise_input_context, m_signal_update_isf_candidate_panel
 * 2. Add new interface APIs in PanelClient class
 *    a. update_cursor_position () and update_surrounding_text ()
 *    b. expand_candidate (), contract_candidate () and set_candidate_style ()
 *    c. reset_input_context () and turn_on_log ()
 *    d. get_client_id () and register_client ()
 *
 * $Id: scim_panel_client.cpp,v 1.6 2005/06/26 16:35:33 suzhe Exp $
 *
 */

#define Uses_SCIM_TRANSACTION
#define Uses_SCIM_TRANS_COMMANDS
#define Uses_SCIM_PANEL_CLIENT
#define Uses_SCIM_SOCKET
#define Uses_SCIM_EVENT

#include "scim_private.h"
#include "scim.h"

#include <unistd.h>
#include <string.h>

namespace scim {

typedef Signal1<void, int>
        PanelClientSignalVoid;

typedef Signal2<void, int, int>
        PanelClientSignalInt;

typedef Signal3<void, int, int, int>
        PanelClientSignalIntInt;

typedef Signal2<void, int, const String &>
        PanelClientSignalString;

typedef Signal2<void, int, const WideString &>
        PanelClientSignalWideString;

typedef Signal4<void, int, const String &, const String &, const Transaction &>
        PanelClientSignalStringStringTransaction;

typedef Signal2<void, int, const KeyEvent &>
        PanelClientSignalKeyEvent;

typedef Signal4<void, int, const WideString &, const AttributeList &, int>
        PanelClientSignalStringAttrsInt;

typedef Signal2<void, int, const std::vector<uint32> &>
        PanelClientSignalUintVector;

class PanelClient::PanelClientImpl
{
    SocketClient                                m_socket;
    SocketClient                                m_socket_active;
    int                                         m_socket_timeout;
    uint32                                      m_socket_magic_key;
    uint32                                      m_socket_magic_key_active;
    Transaction                                 m_send_trans;
    Transaction                                 m_recv_trans;
    int                                         m_current_icid;
    int                                         m_send_refcount;

    PanelClientSignalVoid                       m_signal_reload_config;
    PanelClientSignalVoid                       m_signal_exit;
    PanelClientSignalUintVector                 m_signal_update_candidate_item_layout;
    PanelClientSignalInt                        m_signal_update_lookup_table_page_size;
    PanelClientSignalVoid                       m_signal_lookup_table_page_up;
    PanelClientSignalVoid                       m_signal_lookup_table_page_down;
    PanelClientSignalVoid                       m_signal_reset_option;
    PanelClientSignalString                     m_signal_trigger_property;
    PanelClientSignalStringStringTransaction    m_signal_process_helper_event;
    PanelClientSignalInt                        m_signal_move_preedit_caret;
    PanelClientSignalInt                        m_signal_update_preedit_caret;
    PanelClientSignalInt                        m_signal_select_aux;
    PanelClientSignalInt                        m_signal_select_candidate;
    PanelClientSignalKeyEvent                   m_signal_process_key_event;
    PanelClientSignalWideString                 m_signal_commit_string;
    PanelClientSignalKeyEvent                   m_signal_forward_key_event;
    PanelClientSignalVoid                       m_signal_request_help;
    PanelClientSignalVoid                       m_signal_request_factory_menu;
    PanelClientSignalString                     m_signal_change_factory;
    PanelClientSignalVoid                       m_signal_reset_keyboard_ise;
    PanelClientSignalVoid                       m_signal_update_keyboard_ise;
    PanelClientSignalVoid                       m_signal_show_preedit_string;
    PanelClientSignalVoid                       m_signal_hide_preedit_string;
    PanelClientSignalStringAttrsInt             m_signal_update_preedit_string;
    PanelClientSignalIntInt                     m_signal_get_surrounding_text;
    PanelClientSignalIntInt                     m_signal_delete_surrounding_text;
    PanelClientSignalVoid                       m_signal_get_selection;
    PanelClientSignalIntInt                     m_signal_set_selection;
    PanelClientSignalInt                        m_signal_update_displayed_candidate_number;
    PanelClientSignalVoid                       m_signal_candidate_more_window_show;
    PanelClientSignalVoid                       m_signal_candidate_more_window_hide;
    PanelClientSignalInt                        m_signal_longpress_candidate;
    PanelClientSignalIntInt                     m_signal_update_ise_input_context;
    PanelClientSignalIntInt                     m_signal_update_isf_candidate_panel;
    PanelClientSignalString                     m_signal_send_private_command;

public:
    PanelClientImpl ()
        : m_socket_timeout (scim_get_default_socket_timeout ()),
          m_socket_magic_key (0), m_socket_magic_key_active (0),
          m_current_icid (-1),
          m_send_refcount (0)
    {
    }

    int  open_connection        (const String &config, const String &display)
    {
        String panel_address = scim_get_default_panel_socket_address (display);
        SocketAddress addr (panel_address);

        if (m_socket.is_connected ()) close_connection ();

        bool ret;
        int  i, count = 0;

        /* Try three times. */
        while (1) {
            ret = m_socket.connect (addr);
            if (ret == false) {
                scim_usleep (100000);
                launch_panel (config, display);
                std::cerr << " Re-connecting to PanelAgent server.";
                ISF_LOG (" Re-connecting to PanelAgent server.\n");
                /* Make sure we are not retrying for more than 5 seconds, in total */
                for (i = 0; i < 3; ++i) {
                    if (m_socket.connect (addr)) {
                        ret = true;
                        break;
                    }
                    std::cerr << ".";
                    scim_usleep (100000);
                }
                if (ret) {
                    std::cerr << " Connected :" << i << "\n";
                    ISF_LOG ("  Connected :%d\n", i);
                }
            }

            if (ret && scim_socket_open_connection (m_socket_magic_key, String ("FrontEnd"), String ("Panel"), m_socket, m_socket_timeout)) {
                ISF_LOG (" PID=%d connect to PanelAgent (%s), connected:%d.\n", getpid (), panel_address.c_str (), count);
                if (!m_socket_active.connect (addr))
                    return -1;
                if (!scim_socket_open_connection (m_socket_magic_key_active,
                                                  String ("FrontEnd_Active"), String ("Panel"),
                                                  m_socket_active, m_socket_timeout)) {
                    m_socket_active.close ();
                    m_socket_magic_key_active = 0;
                    return -1;
                }
                break;
            } else {
                std::cerr << " PID=" << getpid () << " cannot connect to PanelAgent (" << panel_address << "), connected:" << count << ".\n";
                ISF_LOG (" PID=%d cannot connect to PanelAgent (%s), connected:%d.\n", getpid (), panel_address.c_str (), count);
            }

            m_socket.close ();

            /* No need to do this forever... */
            if (count++ >= 0) break;
        }

        return m_socket.get_id ();
    }

    void close_connection       ()
    {
        m_socket.close ();
        m_socket_active.close ();
        m_socket_magic_key        = 0;
        m_socket_magic_key_active = 0;
    }

    int  get_connection_number  () const
    {
        return m_socket.get_id ();
    }

    bool is_connected           () const
    {
        return m_socket.is_connected ();
    }

    bool has_pending_event      () const
    {
        return m_socket.is_connected () && m_socket.wait_for_data (0) > 0;
    }

    bool filter_event           ()
    {
        Transaction recv;

        if (!m_socket.is_connected () || !recv.read_from_socket (m_socket, m_socket_timeout))
            return false;

        int cmd;
        uint32 context = (uint32)(-1);

        if (!recv.get_command (cmd) || cmd != SCIM_TRANS_CMD_REPLY)
            return true;

        /* No context id available, so there will be some global command. */
        if (recv.get_data_type () == SCIM_TRANS_DATA_COMMAND) {
            while (recv.get_command (cmd)) {
                switch (cmd) {
                    case SCIM_TRANS_CMD_RELOAD_CONFIG:
                        m_signal_reload_config ((int)context);
                        break;
                    case SCIM_TRANS_CMD_EXIT:
                        m_signal_exit ((int)context);
                        break;
                    default:
                        break;
                }
            }
            return true;
        }

        /* Now for context related command. */
        if (!recv.get_data (context))
            return true;

        while (recv.get_command (cmd)) {
            switch (cmd) {
                case ISM_TRANS_CMD_UPDATE_CANDIDATE_ITEM_LAYOUT:
                    {
                        std::vector<uint32> row_items;
                        if (recv.get_data (row_items))
                            m_signal_update_candidate_item_layout ((int) context, row_items);
                    }
                    break;
                case SCIM_TRANS_CMD_UPDATE_LOOKUP_TABLE_PAGE_SIZE:
                    {
                        uint32 size;
                        if (recv.get_data (size))
                            m_signal_update_lookup_table_page_size ((int) context, (int) size);
                    }
                    break;
                case SCIM_TRANS_CMD_LOOKUP_TABLE_PAGE_UP:
                    {
                        m_signal_lookup_table_page_up ((int) context);
                    }
                    break;
                case SCIM_TRANS_CMD_LOOKUP_TABLE_PAGE_DOWN:
                    {
                        m_signal_lookup_table_page_down ((int) context);
                    }
                    break;
                case ISM_TRANS_CMD_RESET_ISE_OPTION:
                    {
                        m_signal_reset_option ((int) context);
                    }
                    break;
                case SCIM_TRANS_CMD_TRIGGER_PROPERTY:
                    {
                        String property;
                        if (recv.get_data (property))
                            m_signal_trigger_property ((int) context, property);
                    }
                    break;
                case SCIM_TRANS_CMD_PROCESS_HELPER_EVENT:
                    {
                        String target_uuid;
                        String helper_uuid;
                        Transaction trans;
                        if (recv.get_data (target_uuid) && recv.get_data (helper_uuid) && recv.get_data (trans))
                            m_signal_process_helper_event ((int) context, target_uuid, helper_uuid, trans);
                    }
                    break;
                case SCIM_TRANS_CMD_MOVE_PREEDIT_CARET:
                    {
                        uint32 caret;
                        if (recv.get_data (caret))
                            m_signal_move_preedit_caret ((int) context, (int) caret);
                    }
                    break;
                case ISM_TRANS_CMD_SELECT_AUX:
                    {
                        uint32 item;
                        if (recv.get_data (item))
                            m_signal_select_aux ((int) context, (int) item);
                    }
                    break;
                case SCIM_TRANS_CMD_SELECT_CANDIDATE:
                    {
                        uint32 item;
                        if (recv.get_data (item))
                            m_signal_select_candidate ((int) context, (int) item);
                    }
                    break;
                case SCIM_TRANS_CMD_PROCESS_KEY_EVENT:
                    {
                        KeyEvent key;
                        if (recv.get_data (key))
                            m_signal_process_key_event ((int) context, key);
                    }
                    break;
                case SCIM_TRANS_CMD_COMMIT_STRING:
                    {
                        WideString wstr;
                        if (recv.get_data (wstr))
                            m_signal_commit_string ((int) context, wstr);
                    }
                    break;
                case SCIM_TRANS_CMD_FORWARD_KEY_EVENT:
                    {
                        KeyEvent key;
                        if (recv.get_data (key))
                            m_signal_forward_key_event ((int) context, key);
                    }
                    break;
                case SCIM_TRANS_CMD_PANEL_REQUEST_HELP:
                    {
                        m_signal_request_help ((int) context);
                    }
                    break;
                case SCIM_TRANS_CMD_PANEL_REQUEST_FACTORY_MENU:
                    {
                        m_signal_request_factory_menu ((int) context);
                    }
                    break;
                case SCIM_TRANS_CMD_PANEL_CHANGE_FACTORY:
                    {
                        String sfid;
                        if (recv.get_data (sfid))
                            m_signal_change_factory ((int) context, sfid);
                    }
                    break;
                case ISM_TRANS_CMD_PANEL_RESET_KEYBOARD_ISE:
                    {
                        m_signal_reset_keyboard_ise ((int) context);
                    }
                    break;
                case ISM_TRANS_CMD_PANEL_UPDATE_KEYBOARD_ISE:
                    {
                        m_signal_update_keyboard_ise ((int) context);
                    }
                    break;
                case SCIM_TRANS_CMD_SHOW_PREEDIT_STRING:
                    {
                        m_signal_show_preedit_string ((int) context);
                    }
                    break;
                case SCIM_TRANS_CMD_HIDE_PREEDIT_STRING:
                    {
                        m_signal_hide_preedit_string ((int) context);
                    }
                    break;
                case SCIM_TRANS_CMD_UPDATE_PREEDIT_STRING:
                    {
                        WideString wstr;
                        AttributeList attrs;
                        uint32 caret;
                        if (recv.get_data (wstr) && recv.get_data (attrs)
                            && recv.get_data (caret))
                            m_signal_update_preedit_string ((int) context, wstr, attrs, (int)caret);
                    }
                    break;
                case SCIM_TRANS_CMD_UPDATE_PREEDIT_CARET:
                    {
                        uint32 caret;
                        if (recv.get_data (caret))
                            m_signal_update_preedit_caret ((int) context, (int) caret);
                    }
                    break;
                case ISM_TRANS_CMD_TURN_ON_LOG:
                    {
                        uint32 isOn;
                        if (recv.get_data (isOn)) {
                            if (isOn) {
                                DebugOutput::enable_debug (SCIM_DEBUG_AllMask);
                                DebugOutput::set_verbose_level (7);
                            } else {
                                DebugOutput::disable_debug (SCIM_DEBUG_AllMask);
                                DebugOutput::set_verbose_level (0);
                            }
                        }
                    }
                    break;
                case SCIM_TRANS_CMD_GET_SURROUNDING_TEXT:
                    {
                        uint32 maxlen_before;
                        uint32 maxlen_after;
                        if (recv.get_data (maxlen_before) && recv.get_data (maxlen_after))
                            m_signal_get_surrounding_text ((int) context, (int)maxlen_before, (int)maxlen_after);
                    }
                    break;
                case SCIM_TRANS_CMD_DELETE_SURROUNDING_TEXT:
                    {
                        uint32 offset;
                        uint32 len;
                        if (recv.get_data (offset) && recv.get_data (len))
                            m_signal_delete_surrounding_text ((int) context, (int)offset, (int)len);
                    }
                    break;
                 case SCIM_TRANS_CMD_GET_SELECTION:
                    {
                        m_signal_get_selection ((int) context);
                    }
                    break;
                case SCIM_TRANS_CMD_SET_SELECTION:
                    {
                        uint32 start;
                        uint32 end;
                        if (recv.get_data (start) && recv.get_data (end))
                            m_signal_set_selection ((int) context, (int)start, (int)end);
                    }
                    break;
                case ISM_TRANS_CMD_UPDATE_DISPLAYED_CANDIDATE:
                    {
                        uint32 number;
                        if (recv.get_data (number))
                            m_signal_update_displayed_candidate_number ((int) context, (int)number);
                    }
                    break;
                case ISM_TRANS_CMD_CANDIDATE_MORE_WINDOW_SHOW:
                    {
                        m_signal_candidate_more_window_show ((int) context);
                    }
                    break;
                case ISM_TRANS_CMD_CANDIDATE_MORE_WINDOW_HIDE:
                    {
                        m_signal_candidate_more_window_hide ((int) context);
                    }
                    break;
                case ISM_TRANS_CMD_LONGPRESS_CANDIDATE:
                    {
                        uint32 index;
                        if (recv.get_data (index))
                            m_signal_longpress_candidate ((int) context, (int)index);
                    }
                    break;
                case ISM_TRANS_CMD_UPDATE_ISE_INPUT_CONTEXT:
                    {
                        uint32 type, value;
                        if (recv.get_data (type) && recv.get_data (value))
                            m_signal_update_ise_input_context ((int) context, (int)type, (int)value);
                    }
                    break;
                case ISM_TRANS_CMD_UPDATE_ISF_CANDIDATE_PANEL:
                    {
                        uint32 type, value;
                        if (recv.get_data (type) && recv.get_data (value))
                            m_signal_update_isf_candidate_panel ((int) context, (int)type, (int)value);
                    }
                    break;
                case SCIM_TRANS_CMD_SEND_PRIVATE_COMMAND:
                    {
                        String str;
                        if (recv.get_data (str))
                            m_signal_send_private_command ((int) context, str);
                    }
                    break;
                default:
                    break;
            }
        }
        return true;
    }

    bool prepare                (int icid)
    {
        if (!m_socket_active.is_connected ()) return false;

        int cmd;
        uint32 data;

        if (m_send_refcount <= 0) {
            m_current_icid = icid;
            m_send_trans.clear ();
            m_send_trans.put_command (SCIM_TRANS_CMD_REQUEST);
            m_send_trans.put_data (m_socket_magic_key_active);
            m_send_trans.put_data ((uint32) icid);

            if (m_send_trans.get_command (cmd) &&
                m_send_trans.get_data (data) &&
                m_send_trans.get_data (data))
                m_send_refcount = 0;
            else
                return false;
        }

        if (m_current_icid == icid) {
            m_send_refcount ++;
            return true;
        }
        return false;
    }

    bool send                   ()
    {
        if (!m_socket_active.is_connected ()) return false;

        if (m_send_refcount <= 0) return false;

        m_send_refcount --;

        if (m_send_refcount > 0) return false;

        if (m_send_trans.get_data_type () != SCIM_TRANS_DATA_UNKNOWN)
            return m_send_trans.write_to_socket (m_socket_active, 0x4d494353);

        return false;
    }

public:
    void turn_on                (int icid)
    {
        if (m_send_refcount > 0 && m_current_icid == icid)
            m_send_trans.put_command (SCIM_TRANS_CMD_PANEL_TURN_ON);
    }
    void turn_off               (int icid)
    {
        if (m_send_refcount > 0 && m_current_icid == icid)
            m_send_trans.put_command (SCIM_TRANS_CMD_PANEL_TURN_OFF);
    }
    void update_screen          (int icid, int screen)
    {
        if (m_send_refcount > 0 && m_current_icid == icid) {
            m_send_trans.put_command (SCIM_TRANS_CMD_UPDATE_SCREEN);
            m_send_trans.put_data ((uint32) screen);
        }
    }
    void show_help              (int icid, const String &help)
    {
        if (m_send_refcount > 0 && m_current_icid == icid) {
            m_send_trans.put_command (SCIM_TRANS_CMD_PANEL_SHOW_HELP);
            m_send_trans.put_data (help);
        }
    }
    void show_factory_menu      (int icid, const std::vector <PanelFactoryInfo> &menu)
    {
        if (m_send_refcount > 0 && m_current_icid == icid) {
            m_send_trans.put_command (SCIM_TRANS_CMD_PANEL_SHOW_FACTORY_MENU);
            for (size_t i = 0; i < menu.size (); ++ i) {
                m_send_trans.put_data (menu [i].uuid);
                m_send_trans.put_data (menu [i].name);
                m_send_trans.put_data (menu [i].lang);
                m_send_trans.put_data (menu [i].icon);
            }
        }
    }
    void focus_in               (int icid, const String &uuid)
    {
        if (m_send_refcount > 0 && m_current_icid == icid) {
            m_send_trans.put_command (SCIM_TRANS_CMD_FOCUS_IN);
            m_send_trans.put_data (uuid);
        }
    }
    void focus_out              (int icid)
    {
        if (m_send_refcount > 0 && m_current_icid == icid)
            m_send_trans.put_command (SCIM_TRANS_CMD_FOCUS_OUT);
    }
    void update_factory_info    (int icid, const PanelFactoryInfo &info)
    {
        if (m_send_refcount > 0 && m_current_icid == icid) {
            m_send_trans.put_command (SCIM_TRANS_CMD_PANEL_UPDATE_FACTORY_INFO);
            m_send_trans.put_data (info.uuid);
            m_send_trans.put_data (info.name);
            m_send_trans.put_data (info.lang);
            m_send_trans.put_data (info.icon);
        }
    }
    void update_spot_location   (int icid, int x, int y, int top_y)
    {
        if (m_send_refcount > 0 && m_current_icid == icid) {
            m_send_trans.put_command (SCIM_TRANS_CMD_UPDATE_SPOT_LOCATION);
            m_send_trans.put_data ((uint32) x);
            m_send_trans.put_data ((uint32) y);
            m_send_trans.put_data ((uint32) top_y);
        }
    }
    void update_cursor_position (int icid, int cursor_pos)
    {
        if (m_send_refcount > 0 && m_current_icid == icid) {
            m_send_trans.put_command (ISM_TRANS_CMD_UPDATE_CURSOR_POSITION);
            m_send_trans.put_data ((uint32) cursor_pos);
        }
    }
    void update_surrounding_text (int icid, const WideString &str, int cursor)
    {
        if (m_send_refcount > 0 && m_current_icid == icid) {
            m_send_trans.put_command (ISM_TRANS_CMD_UPDATE_SURROUNDING_TEXT);
            m_send_trans.put_data (utf8_wcstombs (str));
            m_send_trans.put_data ((uint32) cursor);
        }
    }
    void update_selection (int icid, const WideString &str)
    {
        if (m_send_refcount > 0 && m_current_icid == icid) {
            m_send_trans.put_command (ISM_TRANS_CMD_UPDATE_SELECTION);
            m_send_trans.put_data (utf8_wcstombs (str));
        }
    }
    void show_preedit_string    (int icid)
    {
        if (m_send_refcount > 0 && m_current_icid == icid)
            m_send_trans.put_command (SCIM_TRANS_CMD_SHOW_PREEDIT_STRING);
    }
    void show_aux_string        (int icid)
    {
        if (m_send_refcount > 0 && m_current_icid == icid)
            m_send_trans.put_command (SCIM_TRANS_CMD_SHOW_AUX_STRING);
    }
    void show_lookup_table      (int icid)
    {
        if (m_send_refcount > 0 && m_current_icid == icid)
            m_send_trans.put_command (SCIM_TRANS_CMD_SHOW_LOOKUP_TABLE);
    }
    void hide_preedit_string    (int icid)
    {
        if (m_send_refcount > 0 && m_current_icid == icid)
            m_send_trans.put_command (SCIM_TRANS_CMD_HIDE_PREEDIT_STRING);
    }
    void hide_aux_string        (int icid)
    {
        if (m_send_refcount > 0 && m_current_icid == icid)
            m_send_trans.put_command (SCIM_TRANS_CMD_HIDE_AUX_STRING);
    }
    void hide_lookup_table      (int icid)
    {
        if (m_send_refcount > 0 && m_current_icid == icid)
            m_send_trans.put_command (SCIM_TRANS_CMD_HIDE_LOOKUP_TABLE);
    }
    void update_preedit_string  (int icid, const WideString &str, const AttributeList &attrs, int caret)
    {
        if (m_send_refcount > 0 && m_current_icid == icid) {
            m_send_trans.put_command (SCIM_TRANS_CMD_UPDATE_PREEDIT_STRING);
            m_send_trans.put_data (utf8_wcstombs (str));
            m_send_trans.put_data (attrs);
            m_send_trans.put_data ((uint32) caret);
        }
    }
    void update_preedit_caret   (int icid, int caret)
    {
        if (m_send_refcount > 0 && m_current_icid == icid) {
            m_send_trans.put_command (SCIM_TRANS_CMD_UPDATE_PREEDIT_CARET);
            m_send_trans.put_data ((uint32) caret);
        }
    }
    void update_aux_string      (int icid, const WideString &str, const AttributeList &attrs)
    {
        if (m_send_refcount > 0 && m_current_icid == icid) {
            m_send_trans.put_command (SCIM_TRANS_CMD_UPDATE_AUX_STRING);
            m_send_trans.put_data (utf8_wcstombs (str));
            m_send_trans.put_data (attrs);
        }
    }
    void update_lookup_table    (int icid, const LookupTable &table)
    {
        if (m_send_refcount > 0 && m_current_icid == icid) {
            m_send_trans.put_command (SCIM_TRANS_CMD_UPDATE_LOOKUP_TABLE);
            m_send_trans.put_data (table);
        }
    }
    void register_properties    (int icid, const PropertyList &properties)
    {
        if (m_send_refcount > 0 && m_current_icid == icid) {
            m_send_trans.put_command (SCIM_TRANS_CMD_REGISTER_PROPERTIES);
            m_send_trans.put_data (properties);
        }
    }
    void update_property        (int icid, const Property &property)
    {
        if (m_send_refcount > 0 && m_current_icid == icid) {
            m_send_trans.put_command (SCIM_TRANS_CMD_UPDATE_PROPERTY);
            m_send_trans.put_data (property);
        }
    }
    void start_helper           (int icid, const String &helper_uuid)
    {
        if (m_send_refcount > 0 && m_current_icid == icid) {
            m_send_trans.put_command (SCIM_TRANS_CMD_START_HELPER);
            m_send_trans.put_data (helper_uuid);
        }
    }
    void stop_helper            (int icid, const String &helper_uuid)
    {
        if (m_send_refcount > 0 && m_current_icid == icid) {
            m_send_trans.put_command (SCIM_TRANS_CMD_STOP_HELPER);
            m_send_trans.put_data (helper_uuid);
        }
    }
    void send_helper_event      (int icid, const String &helper_uuid, const Transaction &trans)
    {
        if (m_send_refcount > 0 && m_current_icid == icid) {
            m_send_trans.put_command (SCIM_TRANS_CMD_SEND_HELPER_EVENT);
            m_send_trans.put_data (helper_uuid);
            m_send_trans.put_data (trans);
        }
    }
    void register_input_context (int icid, const String &uuid)
    {
        if (m_send_refcount > 0 && m_current_icid == icid) {
            m_send_trans.put_command (SCIM_TRANS_CMD_PANEL_REGISTER_INPUT_CONTEXT);
            m_send_trans.put_data (uuid);
        }
    }
    void remove_input_context   (int icid)
    {
        if (m_send_refcount > 0 && m_current_icid == icid)
            m_send_trans.put_command (SCIM_TRANS_CMD_PANEL_REMOVE_INPUT_CONTEXT);
    }
    void reset_input_context    (int icid)
    {
        if (m_send_refcount > 0 && m_current_icid == icid)
            m_send_trans.put_command (SCIM_TRANS_CMD_PANEL_RESET_INPUT_CONTEXT);
    }
    void turn_on_log            (int icid, uint32 isOn)
    {
        m_send_trans.put_command (ISM_TRANS_CMD_TURN_ON_LOG);
        m_send_trans.put_data (isOn);
    }
    void expand_candidate       (int icid)
    {
        if (m_send_refcount > 0 && m_current_icid == icid)
            m_send_trans.put_command (ISM_TRANS_CMD_EXPAND_CANDIDATE);
    }
    void contract_candidate     (int icid)
    {
        if (m_send_refcount > 0 && m_current_icid == icid)
            m_send_trans.put_command (ISM_TRANS_CMD_CONTRACT_CANDIDATE);
    }
    void set_candidate_style    (int icid, ISF_CANDIDATE_PORTRAIT_LINE_T portrait_line, ISF_CANDIDATE_MODE_T mode)
    {
        if (m_send_refcount > 0 && m_current_icid == icid) {
            m_send_trans.put_command (ISM_TRANS_CMD_SET_CANDIDATE_UI);
            m_send_trans.put_data (portrait_line);
            m_send_trans.put_data (mode);
        }
    }

    bool post_prepare           (void)
    {
        if (m_socket_active.is_connected () && m_send_refcount > 0) {
            m_send_trans.clear ();
            m_send_trans.put_command (SCIM_TRANS_CMD_REQUEST);
            m_send_trans.put_data (m_socket_magic_key_active);
            m_send_trans.put_data ((uint32) m_current_icid);

            return true;
        }

        return false;
    }

    bool instant_send           (void)
    {
        if (m_socket_active.is_connected () && m_send_refcount > 0) {
            if (m_send_trans.get_data_type () != SCIM_TRANS_DATA_UNKNOWN)
                return m_send_trans.write_to_socket (m_socket_active, 0x4d494353);
        }

        return false;
    }

    void show_ise               (int client_id, int icid, void *data, int length, int *input_panel_show) {
        if (m_send_refcount > 0 && m_current_icid == icid) {
            int cmd;
            uint32 temp;
            m_send_trans.put_command (ISM_TRANS_CMD_SHOW_ISE_PANEL);
            m_send_trans.put_data ((uint32)client_id);
            m_send_trans.put_data ((uint32)icid);
            m_send_trans.put_data ((const char *)data, (size_t)length);
            instant_send ();
            if (!m_send_trans.read_from_socket (m_socket_active, m_socket_timeout))
                std::cerr << __func__ << " read_from_socket() may be timeout \n";
            if (m_send_trans.get_command (cmd) && cmd == SCIM_TRANS_CMD_REPLY &&
                    m_send_trans.get_command (cmd) && cmd == SCIM_TRANS_CMD_OK &&
                    m_send_trans.get_data (temp)) {
                if (input_panel_show)
                    *input_panel_show = temp;
            } else {
                std::cerr << __func__ << " get_command() or get_data() may fail!!!\n";
                if (input_panel_show)
                    *input_panel_show = false;
            }
            post_prepare ();
        }
    }

    void hide_ise               (int client_id, int icid) {
        if (m_send_refcount > 0 && m_current_icid == icid) {
            m_send_trans.put_command (ISM_TRANS_CMD_HIDE_ISE_PANEL);
            m_send_trans.put_data ((uint32)client_id);
            m_send_trans.put_data ((uint32)icid);
        }
    }

    void show_control_panel (void) {
        if (m_send_refcount > 0)
            m_send_trans.put_command (ISM_TRANS_CMD_SHOW_ISF_CONTROL);
    }

    void hide_control_panel (void) {
        if (m_send_refcount > 0)
            m_send_trans.put_command (ISM_TRANS_CMD_HIDE_ISF_CONTROL);
    }

    void set_imdata (const char* data, int len) {
        if (m_send_refcount > 0) {
            m_send_trans.put_command (ISM_TRANS_CMD_SET_ISE_IMDATA);
            m_send_trans.put_data (data, len);
        }
    }

    void get_imdata (char* data, int* len) {
        if (m_send_refcount > 0) {
            int cmd;
            size_t datalen = 0;
            char* data_temp = NULL;

            m_send_trans.put_command (ISM_TRANS_CMD_GET_ISE_IMDATA);
            instant_send ();
            if (!m_send_trans.read_from_socket (m_socket_active, m_socket_timeout))
                std::cerr << __func__ << " read_from_socket() may be timeout \n";

            if (m_send_trans.get_command (cmd) && cmd == SCIM_TRANS_CMD_REPLY &&
                    m_send_trans.get_command (cmd) && cmd == SCIM_TRANS_CMD_OK &&
                    m_send_trans.get_data (&data_temp, datalen)) {
                memcpy (data, data_temp, datalen);
                *len = datalen;
            } else {
                std::cerr << __func__ << " get_command() or get_data() may fail!!!\n";
            }
            if (data_temp)
                delete [] data_temp;
            post_prepare ();
        }
    }

    void get_ise_window_geometry (int* x, int* y, int* width, int* height) {
        if (m_send_refcount > 0) {
            int cmd;
            uint32 x_temp = 0;
            uint32 y_temp = 0;
            uint32 w_temp = 0;
            uint32 h_temp = 0;

            m_send_trans.put_command (ISM_TRANS_CMD_GET_ACTIVE_ISE_GEOMETRY);
            instant_send ();

            if (!m_send_trans.read_from_socket (m_socket_active, m_socket_timeout))
                std::cerr << __func__ << " read_from_socket() may be timeout \n";

            if (m_send_trans.get_command (cmd) && cmd == SCIM_TRANS_CMD_REPLY &&
                m_send_trans.get_command (cmd) && cmd == SCIM_TRANS_CMD_OK &&
                m_send_trans.get_data (x_temp) &&
                m_send_trans.get_data (y_temp) &&
                m_send_trans.get_data (w_temp) &&
                m_send_trans.get_data (h_temp)) {
                *x = x_temp;
                *y = y_temp;
                *width = w_temp;
                *height = h_temp;
            } else {
                std::cerr << __func__ << " get_command() or get_data() may fail!!!\n";
            }
            post_prepare ();
        }
    }

    void get_candidate_window_geometry (int* x, int* y, int* width, int* height) {
        if (m_send_refcount > 0) {
            int cmd;
            uint32 x_temp = 0;
            uint32 y_temp = 0;
            uint32 w_temp = 0;
            uint32 h_temp = 0;

            m_send_trans.put_command (ISM_TRANS_CMD_GET_CANDIDATE_GEOMETRY);
            instant_send ();

            if (!m_send_trans.read_from_socket (m_socket_active, m_socket_timeout))
                std::cerr << __func__ << " read_from_socket () may be timeout \n";

            if (m_send_trans.get_command (cmd) && cmd == SCIM_TRANS_CMD_REPLY &&
                m_send_trans.get_command (cmd) && cmd == SCIM_TRANS_CMD_OK &&
                m_send_trans.get_data (x_temp) &&
                m_send_trans.get_data (y_temp) &&
                m_send_trans.get_data (w_temp) &&
                m_send_trans.get_data (h_temp)) {
                *x = x_temp;
                *y = y_temp;
                *width = w_temp;
                *height = h_temp;
            } else {
                std::cerr << __func__ << " get_command () or get_data () is failed!!!\n";
            }
            post_prepare ();
        }
    }

    void get_ise_language_locale (char **locale) {
        if (m_send_refcount > 0) {
            int cmd;
            size_t datalen = 0;
            char  *data = NULL;

            m_send_trans.put_command (ISM_TRANS_CMD_GET_ISE_LANGUAGE_LOCALE);
            instant_send ();
            if (!m_send_trans.read_from_socket (m_socket_active, m_socket_timeout))
                std::cerr << __func__ << " read_from_socket () may be timeout \n";

            if (m_send_trans.get_command (cmd) && cmd == SCIM_TRANS_CMD_REPLY &&
                m_send_trans.get_command (cmd) && cmd == SCIM_TRANS_CMD_OK &&
                m_send_trans.get_data (&data, datalen)) {
                if (locale)
                    *locale = strndup (data, datalen);
            } else {
                std::cerr << __func__ << " get_command () or get_data () is failed!!!\n";
                if (locale)
                    *locale = strdup ("");
            }
            if (data)
                delete [] data;
            post_prepare ();
        }
    }

    void set_return_key_type (int type) {
        if (m_send_refcount > 0) {
            m_send_trans.put_command (ISM_TRANS_CMD_SET_RETURN_KEY_TYPE);
            m_send_trans.put_data (type);
        }
    }

    void get_return_key_type (int &type) {
        if (m_send_refcount > 0) {
            int cmd;
            uint32 temp;

            m_send_trans.put_command (ISM_TRANS_CMD_GET_RETURN_KEY_TYPE);
            instant_send ();
            if (!m_send_trans.read_from_socket (m_socket_active, m_socket_timeout))
                std::cerr << __func__ << " read_from_socket() may be timeout \n";

            if (m_send_trans.get_command (cmd) && cmd == SCIM_TRANS_CMD_REPLY &&
                    m_send_trans.get_command (cmd) && cmd == SCIM_TRANS_CMD_OK &&
                    m_send_trans.get_data (temp)) {
                type = temp;
            } else {
                std::cerr << __func__ << " get_command() or get_data() may fail!!!\n";
            }
            post_prepare ();
        }
    }

    void set_return_key_disable (int disabled) {
        if (m_send_refcount > 0) {
            m_send_trans.put_command (ISM_TRANS_CMD_SET_RETURN_KEY_DISABLE);
            m_send_trans.put_data (disabled);
        }
    }

    void get_return_key_disable (int &disabled) {
        if (m_send_refcount > 0) {
            int cmd;
            uint32 temp;

            m_send_trans.put_command (ISM_TRANS_CMD_GET_RETURN_KEY_DISABLE);
            instant_send ();
            if (!m_send_trans.read_from_socket (m_socket_active, m_socket_timeout))
                std::cerr << __func__ << " read_from_socket() may be timeout \n";

            if (m_send_trans.get_command (cmd) && cmd == SCIM_TRANS_CMD_REPLY &&
                    m_send_trans.get_command (cmd) && cmd == SCIM_TRANS_CMD_OK &&
                    m_send_trans.get_data (temp)) {
                disabled = temp;
            } else {
                std::cerr << __func__ << " get_command() or get_data() may fail!!!\n";
            }
            post_prepare ();
        }
    }

    void set_layout (int layout) {
        if (m_send_refcount > 0) {
            m_send_trans.put_command (ISM_TRANS_CMD_SET_LAYOUT);
            m_send_trans.put_data (layout);
        }
    }

    void set_input_mode (int input_mode) {
        if (m_send_refcount > 0) {
            m_send_trans.put_command (ISM_TRANS_CMD_SET_INPUT_MODE);
            m_send_trans.put_data (input_mode);
        }
    }

    void set_input_hint (int icid, int input_hint) {
        if (m_send_refcount > 0 && m_current_icid == icid) {
            m_send_trans.put_command (ISM_TRANS_CMD_SET_INPUT_HINT);
            m_send_trans.put_data (input_hint);
        }
    }

    void update_bidi_direction (int icid, int bidi_direction) {
        if (m_send_refcount > 0 && m_current_icid == icid) {
            m_send_trans.put_command (ISM_TRANS_CMD_UPDATE_BIDI_DIRECTION);
            m_send_trans.put_data (bidi_direction);
        }
    }

    void get_layout (int* layout) {
        if (m_send_refcount > 0) {
            int cmd;
            uint32 layout_temp;

            m_send_trans.put_command (ISM_TRANS_CMD_GET_LAYOUT);
            instant_send ();
            if (!m_send_trans.read_from_socket (m_socket_active, m_socket_timeout))
                std::cerr << __func__ << " read_from_socket() may be timeout \n";

            if (m_send_trans.get_command (cmd) && cmd == SCIM_TRANS_CMD_REPLY &&
                    m_send_trans.get_command (cmd) && cmd == SCIM_TRANS_CMD_OK &&
                    m_send_trans.get_data (layout_temp)) {
                *layout = layout_temp;
            } else {
                std::cerr << __func__ << " get_command() or get_data() may fail!!!\n";
            }
            post_prepare ();
        }
    }

    void get_ise_state (int &ise_state) {
        if (m_send_refcount > 0) {
            int cmd;
            uint32 temp;

            m_send_trans.put_command (ISM_TRANS_CMD_GET_ISE_STATE);
            instant_send ();
            if (!m_send_trans.read_from_socket (m_socket_active, m_socket_timeout))
                std::cerr << __func__ << " read_from_socket() may be timeout \n";

            if (m_send_trans.get_command (cmd) && cmd == SCIM_TRANS_CMD_REPLY &&
                    m_send_trans.get_command (cmd) && cmd == SCIM_TRANS_CMD_OK &&
                    m_send_trans.get_data (temp)) {
                ise_state = temp;
            } else {
                std::cerr << __func__ << " get_command() or get_data() may fail!!!\n";
            }
            post_prepare ();
        }
    }

    void set_ise_language (int language) {
        if (m_send_refcount > 0) {
            m_send_trans.put_command (ISM_TRANS_CMD_SET_ISE_LANGUAGE);
            m_send_trans.put_data (language);
        }
    }

    void set_caps_mode (int mode) {
        if (m_send_refcount > 0) {
            m_send_trans.put_command (ISM_TRANS_CMD_SET_CAPS_MODE);
            m_send_trans.put_data (mode);
        }
    }

    void send_will_show_ack (void) {
        if (m_send_refcount > 0)
            m_send_trans.put_command (ISM_TRANS_CMD_SEND_WILL_SHOW_ACK);
    }

    void send_will_hide_ack (void) {
        if (m_send_refcount > 0)
            m_send_trans.put_command (ISM_TRANS_CMD_SEND_WILL_HIDE_ACK);
    }

    void set_keyboard_mode (int mode) {
        if (m_send_refcount > 0) {
            m_send_trans.put_command (ISM_TRANS_CMD_SET_HARDWARE_KEYBOARD_MODE);
            m_send_trans.put_data (mode);
        }
    }

    void send_candidate_will_hide_ack (void) {
        if (m_send_refcount > 0)
            m_send_trans.put_command (ISM_TRANS_CMD_SEND_CANDIDATE_WILL_HIDE_ACK);
    }

    bool get_client_id (int &client_id) {
        if (!m_socket.is_connected ()) return false;

        int cmd;
        uint32 data;
        uint32 id;

        m_send_trans.clear ();
        m_send_trans.put_command (SCIM_TRANS_CMD_REQUEST);
        m_send_trans.put_data (m_socket_magic_key);
        m_send_trans.put_data (0);

        if (m_send_trans.get_command (cmd) &&
            m_send_trans.get_data (data) &&
            m_send_trans.get_data (data)) {
            m_send_trans.put_command (ISM_TRANS_CMD_GET_PANEL_CLIENT_ID);
            m_send_trans.write_to_socket (m_socket);

            if (!m_send_trans.read_from_socket (m_socket, m_socket_timeout))
                std::cerr << __func__ << " read_from_socket() may be timeout \n";

            if (m_send_trans.get_command (cmd) && cmd == SCIM_TRANS_CMD_REPLY &&
                    m_send_trans.get_command (cmd) && cmd == SCIM_TRANS_CMD_OK &&
                    m_send_trans.get_data (id)) {
                client_id = id;
                return true;
            } else {
                std::cerr << __func__ << " get_command() or get_data() may fail!!!\n";
                return false;
            }
        }

        return false;
    }

    void register_client (int client_id) {
        if (m_send_refcount > 0) {
            m_send_trans.put_command (ISM_TRANS_CMD_REGISTER_PANEL_CLIENT);
            m_send_trans.put_data (client_id);
        }
    }

    void process_key_event(KeyEvent& key, int *ret) {
        if (m_send_refcount > 0) {
            int cmd;
            uint32 ret_temp;

            m_send_trans.put_command (SCIM_TRANS_CMD_PROCESS_KEY_EVENT);
            m_send_trans.put_data(key);
            instant_send ();
            if (!m_send_trans.read_from_socket (m_socket_active, m_socket_timeout))
                std::cerr << __func__ << " read_from_socket() may be timeout \n";

            if (m_send_trans.get_command (cmd) && cmd == SCIM_TRANS_CMD_REPLY &&
                    m_send_trans.get_command (cmd) && cmd == SCIM_TRANS_CMD_OK &&
                    m_send_trans.get_data (ret_temp)) {
                *ret = ret_temp;
            } else {
                std::cerr << __func__ << " get_command() or get_data() may fail!!!\n";
            }
            post_prepare ();
        }
    }

    void get_active_helper_option(int *option) {
        if (m_send_refcount > 0) {
            int cmd;
            uint32 option_temp;

            m_send_trans.put_command (ISM_TRANS_CMD_GET_ACTIVE_HELPER_OPTION);
            instant_send ();
            if (!m_send_trans.read_from_socket (m_socket_active, m_socket_timeout))
                std::cerr << __func__ << " read_from_socket() may be timeout \n";

            if (m_send_trans.get_command (cmd) && cmd == SCIM_TRANS_CMD_REPLY &&
                    m_send_trans.get_command (cmd) && cmd == SCIM_TRANS_CMD_OK &&
                    m_send_trans.get_data (option_temp)) {
                *option = option_temp;
            } else {
                std::cerr << __func__ << " get_command() or get_data() may fail!!!\n";
            }
            post_prepare ();
        }
    }

public:
    void reset_signal_handler                               (void)
    {
        m_signal_reload_config.reset();
        m_signal_exit.reset();
        m_signal_update_candidate_item_layout.reset();
        m_signal_update_lookup_table_page_size.reset();
        m_signal_lookup_table_page_up.reset();
        m_signal_lookup_table_page_down.reset();
        m_signal_reset_option.reset();
        m_signal_trigger_property.reset();
        m_signal_process_helper_event.reset();
        m_signal_move_preedit_caret.reset();
        m_signal_update_preedit_caret.reset ();
        m_signal_select_aux.reset();
        m_signal_select_candidate.reset();
        m_signal_process_key_event.reset();
        m_signal_commit_string.reset();
        m_signal_forward_key_event.reset();
        m_signal_request_help.reset();
        m_signal_request_factory_menu.reset();
        m_signal_change_factory.reset();
        m_signal_reset_keyboard_ise.reset();
        m_signal_update_keyboard_ise.reset();
        m_signal_show_preedit_string.reset();
        m_signal_hide_preedit_string.reset();
        m_signal_update_preedit_string.reset();
        m_signal_get_surrounding_text.reset();
        m_signal_delete_surrounding_text.reset();
        m_signal_get_selection.reset();
        m_signal_set_selection.reset();
        m_signal_update_displayed_candidate_number.reset();
        m_signal_candidate_more_window_show.reset();
        m_signal_candidate_more_window_hide.reset();
        m_signal_longpress_candidate.reset();
        m_signal_update_ise_input_context.reset();
        m_signal_update_isf_candidate_panel.reset();
        m_signal_send_private_command.reset();
    }

    Connection signal_connect_reload_config                 (PanelClientSlotVoid                    *slot)
    {
        return m_signal_reload_config.connect (slot);
    }
    Connection signal_connect_exit                          (PanelClientSlotVoid                    *slot)
    {
        return m_signal_exit.connect (slot);
    }
    Connection signal_connect_update_candidate_item_layout  (PanelClientSlotUintVector              *slot)
    {
        return m_signal_update_candidate_item_layout.connect (slot);
    }
    Connection signal_connect_update_lookup_table_page_size (PanelClientSlotInt                     *slot)
    {
        return m_signal_update_lookup_table_page_size.connect (slot);
    }
    Connection signal_connect_lookup_table_page_up          (PanelClientSlotVoid                    *slot)
    {
        return m_signal_lookup_table_page_up.connect (slot);
    }
    Connection signal_connect_lookup_table_page_down        (PanelClientSlotVoid                    *slot)
    {
        return m_signal_lookup_table_page_down.connect (slot);
    }
    Connection signal_connect_reset_option                  (PanelClientSlotVoid                    *slot)
    {
        return m_signal_reset_option.connect (slot);
    }
    Connection signal_connect_trigger_property              (PanelClientSlotString                  *slot)
    {
        return m_signal_trigger_property.connect (slot);
    }
    Connection signal_connect_process_helper_event          (PanelClientSlotStringStringTransaction *slot)
    {
        return m_signal_process_helper_event.connect (slot);
    }
    Connection signal_connect_move_preedit_caret            (PanelClientSlotInt                     *slot)
    {
        return m_signal_move_preedit_caret.connect (slot);
    }
    Connection signal_connect_update_preedit_caret          (PanelClientSlotInt                     *slot)
    {
        return m_signal_update_preedit_caret.connect (slot);
    }
    Connection signal_connect_select_aux                    (PanelClientSlotInt                     *slot)
    {
        return m_signal_select_aux.connect (slot);
    }
    Connection signal_connect_select_candidate              (PanelClientSlotInt                     *slot)
    {
        return m_signal_select_candidate.connect (slot);
    }
    Connection signal_connect_process_key_event             (PanelClientSlotKeyEvent                *slot)
    {
        return m_signal_process_key_event.connect (slot);
    }
    Connection signal_connect_commit_string                 (PanelClientSlotWideString              *slot)
    {
        return m_signal_commit_string.connect (slot);
    }
    Connection signal_connect_forward_key_event             (PanelClientSlotKeyEvent                *slot)
    {
        return m_signal_forward_key_event.connect (slot);
    }
    Connection signal_connect_request_help                  (PanelClientSlotVoid                    *slot)
    {
        return m_signal_request_help.connect (slot);
    }
    Connection signal_connect_request_factory_menu          (PanelClientSlotVoid                    *slot)
    {
        return m_signal_request_factory_menu.connect (slot);
    }
    Connection signal_connect_change_factory                (PanelClientSlotString                  *slot)
    {
        return m_signal_change_factory.connect (slot);
    }

    Connection signal_connect_reset_keyboard_ise            (PanelClientSlotVoid                    *slot)
    {
        return m_signal_reset_keyboard_ise.connect (slot);
    }

    Connection signal_connect_update_keyboard_ise           (PanelClientSlotVoid                    *slot)
    {
        return m_signal_update_keyboard_ise.connect (slot);
    }

    Connection signal_connect_show_preedit_string           (PanelClientSlotVoid                    *slot)
    {
        return m_signal_show_preedit_string.connect (slot);
    }

    Connection signal_connect_hide_preedit_string           (PanelClientSlotVoid                    *slot)
    {
        return m_signal_hide_preedit_string.connect (slot);
    }

    Connection signal_connect_update_preedit_string         (PanelClientSlotStringAttrsInt          *slot)
    {
        return m_signal_update_preedit_string.connect (slot);
    }

    Connection signal_connect_get_surrounding_text          (PanelClientSlotIntInt                  *slot)
    {
        return m_signal_get_surrounding_text.connect (slot);
    }

    Connection signal_connect_delete_surrounding_text       (PanelClientSlotIntInt                  *slot)
    {
        return m_signal_delete_surrounding_text.connect (slot);
    }

    Connection signal_connect_get_selection                 (PanelClientSlotVoid                    *slot)
    {
        return m_signal_get_selection.connect (slot);
    }

    Connection signal_connect_set_selection                 (PanelClientSlotIntInt                  *slot)
    {
        return m_signal_set_selection.connect (slot);
    }

    Connection signal_connect_update_displayed_candidate_number (PanelClientSlotInt                 *slot)
    {
        return m_signal_update_displayed_candidate_number.connect (slot);
    }

    Connection signal_connect_candidate_more_window_show    (PanelClientSlotVoid                    *slot)
    {
        return m_signal_candidate_more_window_show.connect (slot);
    }

    Connection signal_connect_candidate_more_window_hide    (PanelClientSlotVoid                    *slot)
    {
        return m_signal_candidate_more_window_hide.connect (slot);
    }

    Connection signal_connect_longpress_candidate           (PanelClientSlotInt                     *slot)
    {
        return m_signal_longpress_candidate.connect (slot);
    }

    Connection signal_connect_update_ise_input_context      (PanelClientSlotIntInt                  *slot)
    {
        return m_signal_update_ise_input_context.connect (slot);
    }

    Connection signal_connect_update_isf_candidate_panel    (PanelClientSlotIntInt                  *slot)
    {
        return m_signal_update_isf_candidate_panel.connect (slot);
    }

    Connection signal_connect_send_private_command          (PanelClientSlotString                  *slot)
    {
        return m_signal_send_private_command.connect (slot);
    }

private:
    void launch_panel (const String &config, const String &display) const
    {
        scim_launch_panel (true, config, display, NULL);
    }
};

PanelClient::PanelClient ()
    : m_impl (new PanelClientImpl ())
{
}

PanelClient::~PanelClient ()
{
    delete m_impl;
}

int
PanelClient::open_connection (const String &config, const String &display)
{
    return m_impl->open_connection (config, display);
}

void
PanelClient::close_connection ()
{
    m_impl->close_connection ();
}

int
PanelClient::get_connection_number () const
{
    return m_impl->get_connection_number ();
}

bool
PanelClient::is_connected () const
{
    return m_impl->is_connected ();
}

bool
PanelClient::has_pending_event () const
{
    return m_impl->has_pending_event ();
}

bool
PanelClient::filter_event ()
{
    return m_impl->filter_event ();
}

bool
PanelClient::prepare (int icid)
{
    return m_impl->prepare (icid);
}

bool
PanelClient::send ()
{
    return m_impl->send ();
}

void
PanelClient::turn_on                (int icid)
{
    m_impl->turn_on (icid);
}
void
PanelClient::turn_off               (int icid)
{
    m_impl->turn_off (icid);
}
void
PanelClient::update_screen          (int icid, int screen)
{
    m_impl->update_screen (icid, screen);
}
void
PanelClient::show_help              (int icid, const String &help)
{
    m_impl->show_help (icid, help);
}
void
PanelClient::show_factory_menu      (int icid, const std::vector <PanelFactoryInfo> &menu)
{
    m_impl->show_factory_menu (icid, menu);
}
void
PanelClient::focus_in               (int icid, const String &uuid)
{
    m_impl->focus_in (icid, uuid);
}
void
PanelClient::focus_out              (int icid)
{
    m_impl->focus_out (icid);
}
void
PanelClient::update_factory_info    (int icid, const PanelFactoryInfo &info)
{
    m_impl->update_factory_info (icid, info);
}
void
PanelClient::update_spot_location   (int icid, int x, int y, int top_y)
{
    m_impl->update_spot_location (icid, x, y, top_y);
}
void
PanelClient::update_cursor_position (int icid, int cursor_pos)
{
    m_impl->update_cursor_position (icid, cursor_pos);
}
void
PanelClient::update_surrounding_text (int icid, const WideString &str, int cursor)
{
    m_impl->update_surrounding_text (icid, str, cursor);
}
void
PanelClient::update_selection (int icid, const WideString &str)
{
    m_impl->update_selection (icid, str);
}
void
PanelClient::show_preedit_string    (int icid)
{
    m_impl->show_preedit_string (icid);
}
void
PanelClient::show_aux_string        (int icid)
{
    m_impl->show_aux_string (icid);
}
void
PanelClient::show_lookup_table      (int icid)
{
    m_impl->show_lookup_table (icid);
}
void
PanelClient::hide_preedit_string    (int icid)
{
    m_impl->hide_preedit_string (icid);
}
void
PanelClient::hide_aux_string        (int icid)
{
    m_impl->hide_aux_string (icid);
}
void
PanelClient::hide_lookup_table      (int icid)
{
    m_impl->hide_lookup_table (icid);
}
void
PanelClient::update_preedit_string  (int icid, const WideString &str, const AttributeList &attrs, int caret)
{
    m_impl->update_preedit_string (icid, str, attrs, caret);
}
void
PanelClient::update_preedit_caret   (int icid, int caret)
{
    m_impl->update_preedit_caret (icid, caret);
}
void
PanelClient::update_aux_string      (int icid, const WideString &str, const AttributeList &attrs)
{
    m_impl->update_aux_string (icid, str, attrs);
}
void
PanelClient::update_lookup_table    (int icid, const LookupTable &table)
{
    m_impl->update_lookup_table (icid, table);
}
void
PanelClient::register_properties    (int icid, const PropertyList &properties)
{
    m_impl->register_properties (icid, properties);
}
void
PanelClient::update_property        (int icid, const Property &property)
{
    m_impl->update_property (icid, property);
}
void
PanelClient::start_helper           (int icid, const String &helper_uuid)
{
    m_impl->start_helper (icid, helper_uuid);
}

void
PanelClient::stop_helper            (int icid, const String &helper_uuid)
{
    m_impl->stop_helper (icid, helper_uuid);
}
void
PanelClient::send_helper_event      (int icid, const String &helper_uuid, const Transaction &trans)
{
    m_impl->send_helper_event (icid, helper_uuid, trans);
}
void
PanelClient::register_input_context (int icid, const String &uuid)
{
    m_impl->register_input_context (icid, uuid);
}
void
PanelClient::remove_input_context   (int icid)
{
    m_impl->remove_input_context (icid);
}
void
PanelClient::reset_input_context    (int icid)
{
    m_impl->reset_input_context (icid);
}

void
PanelClient::turn_on_log            (int icid, uint32 isOn)
{
    m_impl->turn_on_log (icid, isOn);
}

void
PanelClient::expand_candidate       (int icid)
{
    m_impl->expand_candidate (icid);
}

void
PanelClient::contract_candidate     (int icid)
{
    m_impl->contract_candidate (icid);
}

void
PanelClient::set_candidate_style    (int icid, ISF_CANDIDATE_PORTRAIT_LINE_T portrait_line, ISF_CANDIDATE_MODE_T mode)
{
    m_impl->set_candidate_style (icid, portrait_line, mode);
}

void
PanelClient::show_ise               (int client_id, int icid, void *data, int length, int *input_panel_show)
{
    m_impl->show_ise (client_id, icid, data, length, input_panel_show);
}

void
PanelClient::hide_ise               (int client_id, int icid)
{
    m_impl->hide_ise (client_id, icid);
}

void
PanelClient::show_control_panel     (void)
{
    m_impl->show_control_panel ();
}

void
PanelClient::hide_control_panel     (void)
{
    m_impl->hide_control_panel ();
}

void
PanelClient::set_imdata             (const char* data, int len)
{
    m_impl->set_imdata (data, len);
}

void
PanelClient::get_imdata             (char* data, int* len)
{
    m_impl->get_imdata (data, len);
}

void
PanelClient::get_ise_window_geometry (int* x, int* y, int* width, int* height)
{
    m_impl->get_ise_window_geometry (x, y, width, height);
}

void
PanelClient::get_candidate_window_geometry (int* x, int* y, int* width, int* height)
{
    m_impl->get_candidate_window_geometry (x, y, width, height);
}

void
PanelClient::get_ise_language_locale (char **locale)
{
    m_impl->get_ise_language_locale (locale);
}

void
PanelClient::set_return_key_type    (int type)
{
    m_impl->set_return_key_type (type);
}

void
PanelClient::get_return_key_type    (int &type)
{
    m_impl->get_return_key_type (type);
}

void
PanelClient::set_return_key_disable (int disabled)
{
    m_impl->set_return_key_disable (disabled);
}

void
PanelClient::get_return_key_disable (int &disabled)
{
    m_impl->get_return_key_disable (disabled);
}

void
PanelClient::set_layout             (int layout)
{
    m_impl->set_layout (layout);
}

void
PanelClient::get_layout             (int* layout)
{
    m_impl->get_layout (layout);
}

void
PanelClient::set_input_mode         (int input_mode)
{
    m_impl->set_input_mode (input_mode);
}

void
PanelClient::set_input_hint         (int icid, int input_hint)
{
    m_impl->set_input_hint (icid, input_hint);
}

void
PanelClient::update_bidi_direction     (int icid, int bidi_direction)
{
    m_impl->update_bidi_direction (icid, bidi_direction);
}

void
PanelClient::set_ise_language       (int language)
{
    m_impl->set_ise_language (language);
}

void
PanelClient::set_caps_mode          (int mode)
{
    m_impl->set_caps_mode (mode);
}

void
PanelClient::send_will_show_ack     (void)
{
    m_impl->send_will_show_ack ();
}

void
PanelClient::send_will_hide_ack     (void)
{
    m_impl->send_will_hide_ack ();
}

void
PanelClient::set_keyboard_mode (int mode)
{
    m_impl->set_keyboard_mode (mode);
}

void
PanelClient::send_candidate_will_hide_ack (void)
{
    m_impl->send_candidate_will_hide_ack ();
}

bool
PanelClient::get_client_id (int &client_id)
{
    return m_impl->get_client_id (client_id);
}

void
PanelClient::register_client (int client_id)
{
    m_impl->register_client (client_id);
}

void
PanelClient::get_ise_state (int &ise_state)
{
    m_impl->get_ise_state (ise_state);
}

void
PanelClient::process_key_event (KeyEvent& key, int* ret)
{
    m_impl->process_key_event (key, ret);
}

void
PanelClient::get_active_helper_option(int* option)
{
    m_impl->get_active_helper_option (option);
}

void
PanelClient::reset_signal_handler                         (void)
{
    m_impl->reset_signal_handler ();
}

Connection
PanelClient::signal_connect_reload_config                 (PanelClientSlotVoid                    *slot)
{
    return m_impl->signal_connect_reload_config (slot);
}
Connection
PanelClient::signal_connect_exit                          (PanelClientSlotVoid                    *slot)
{
    return m_impl->signal_connect_exit (slot);
}
Connection
PanelClient::signal_connect_update_candidate_item_layout  (PanelClientSlotUintVector              *slot)
{
    return m_impl->signal_connect_update_candidate_item_layout (slot);
}
Connection
PanelClient::signal_connect_update_lookup_table_page_size (PanelClientSlotInt                     *slot)
{
    return m_impl->signal_connect_update_lookup_table_page_size (slot);
}
Connection
PanelClient::signal_connect_lookup_table_page_up          (PanelClientSlotVoid                    *slot)
{
    return m_impl->signal_connect_lookup_table_page_up (slot);
}
Connection
PanelClient::signal_connect_lookup_table_page_down        (PanelClientSlotVoid                    *slot)
{
    return m_impl->signal_connect_lookup_table_page_down (slot);
}
Connection
PanelClient::signal_connect_reset_option                  (PanelClientSlotVoid                    *slot)
{
    return m_impl->signal_connect_reset_option (slot);
}
Connection
PanelClient::signal_connect_trigger_property              (PanelClientSlotString                  *slot)
{
    return m_impl->signal_connect_trigger_property (slot);
}
Connection
PanelClient::signal_connect_process_helper_event          (PanelClientSlotStringStringTransaction *slot)
{
    return m_impl->signal_connect_process_helper_event (slot);
}
Connection
PanelClient::signal_connect_move_preedit_caret            (PanelClientSlotInt                     *slot)
{
    return m_impl->signal_connect_move_preedit_caret (slot);
}
Connection
PanelClient::signal_connect_update_preedit_caret          (PanelClientSlotInt                     *slot)
{
    return m_impl->signal_connect_update_preedit_caret (slot);
}
Connection
PanelClient::signal_connect_select_aux                    (PanelClientSlotInt                     *slot)
{
    return m_impl->signal_connect_select_aux (slot);
}
Connection
PanelClient::signal_connect_select_candidate              (PanelClientSlotInt                     *slot)
{
    return m_impl->signal_connect_select_candidate (slot);
}
Connection
PanelClient::signal_connect_process_key_event             (PanelClientSlotKeyEvent                *slot)
{
    return m_impl->signal_connect_process_key_event (slot);
}
Connection
PanelClient::signal_connect_commit_string                 (PanelClientSlotWideString              *slot)
{
    return m_impl->signal_connect_commit_string (slot);
}
Connection
PanelClient::signal_connect_forward_key_event             (PanelClientSlotKeyEvent                *slot)
{
    return m_impl->signal_connect_forward_key_event (slot);
}
Connection
PanelClient::signal_connect_request_help                  (PanelClientSlotVoid                    *slot)
{
    return m_impl->signal_connect_request_help (slot);
}
Connection
PanelClient::signal_connect_request_factory_menu          (PanelClientSlotVoid                    *slot)
{
    return m_impl->signal_connect_request_factory_menu (slot);
}
Connection
PanelClient::signal_connect_change_factory                (PanelClientSlotString                  *slot)
{
    return m_impl->signal_connect_change_factory (slot);
}

Connection
PanelClient::signal_connect_reset_keyboard_ise            (PanelClientSlotVoid                    *slot)
{
    return m_impl->signal_connect_reset_keyboard_ise (slot);
}

Connection
PanelClient::signal_connect_update_keyboard_ise           (PanelClientSlotVoid                    *slot)
{
    return m_impl->signal_connect_update_keyboard_ise (slot);
}

Connection
PanelClient::signal_connect_show_preedit_string           (PanelClientSlotVoid                    *slot)
{
    return m_impl->signal_connect_show_preedit_string (slot);
}

Connection
PanelClient::signal_connect_hide_preedit_string           (PanelClientSlotVoid                    *slot)
{
    return m_impl->signal_connect_hide_preedit_string (slot);
}

Connection
PanelClient::signal_connect_update_preedit_string         (PanelClientSlotStringAttrsInt          *slot)
{
    return m_impl->signal_connect_update_preedit_string (slot);
}

Connection
PanelClient::signal_connect_get_surrounding_text          (PanelClientSlotIntInt                  *slot)
{
    return m_impl->signal_connect_get_surrounding_text (slot);
}

Connection
PanelClient::signal_connect_delete_surrounding_text       (PanelClientSlotIntInt                  *slot)
{
    return m_impl->signal_connect_delete_surrounding_text (slot);
}

Connection
PanelClient::signal_connect_get_selection                 (PanelClientSlotVoid                    *slot)
{
    return m_impl->signal_connect_get_selection (slot);
}

Connection
PanelClient::signal_connect_set_selection                 (PanelClientSlotIntInt                  *slot)
{
    return m_impl->signal_connect_set_selection (slot);
}

Connection
PanelClient::signal_connect_update_displayed_candidate_number (PanelClientSlotInt                 *slot)
{
    return m_impl->signal_connect_update_displayed_candidate_number (slot);
}

Connection
PanelClient::signal_connect_candidate_more_window_show    (PanelClientSlotVoid                    *slot)
{
    return m_impl->signal_connect_candidate_more_window_show (slot);
}

Connection
PanelClient::signal_connect_candidate_more_window_hide    (PanelClientSlotVoid                    *slot)
{
    return m_impl->signal_connect_candidate_more_window_hide (slot);
}

Connection
PanelClient::signal_connect_longpress_candidate           (PanelClientSlotInt                     *slot)
{
    return m_impl->signal_connect_longpress_candidate (slot);
}

Connection
PanelClient::signal_connect_update_ise_input_context      (PanelClientSlotIntInt                  *slot)
{
    return m_impl->signal_connect_update_ise_input_context (slot);
}

Connection
PanelClient::signal_connect_update_isf_candidate_panel    (PanelClientSlotIntInt                  *slot)
{
    return m_impl->signal_connect_update_isf_candidate_panel (slot);
}

Connection
PanelClient::signal_connect_send_private_command          (PanelClientSlotString                  *slot)
{
    return m_impl->signal_connect_send_private_command (slot);
}

} /* namespace scim */

/*
vi:ts=4:nowrap:ai:expandtab
*/

