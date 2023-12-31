/**
 * @file scim_trans_commands.h
 * @brief Transaction commands.
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
 * $Id: scim_trans_commands.h,v 1.9 2005/06/26 16:35:33 suzhe Exp $
 */

#ifndef __SCIM_TRANS_COMMANDS_H
#define __SCIM_TRANS_COMMANDS_H

namespace scim {

/**
 * @brief Transaction command types used by Socket Frontend/IMEngine/Config, Helper and Panel.
 *
 * This commands are used in communication protocols of SocketFrontEnd, SocketIMEngine, SocketConfig, Helper and Panel.
 *
 * There are mainly four major protocols used in the communications among each part of SCIM:
 * - between SocketFrontEnd and SocketIMEngine (SocketFrontEnd is server)
 * - between SocketFrontEnd and SocketConfig (SocketFrontEnd is server)
 * - between Panel and FrontEnds (eg. X11 FrontEnd, Gtk IMModule and QT IMModule. Panel is server)
 * - between Panel and Helper (Panel is server).
 *
 * As soon as the socket to the server is established, the client must call function
 * scim_socket_open_connection() to create the connection and get the magic key for later communication.
 *
 * At the same time, the server must call function scim_socket_accept_connection() to
 * accept the connection and get the same magic key for later client verification.
 *
 * The valid types of servers are:
 * - "SocketFrontEnd"\n
 *   The socket FrontEnd server provides remote IMEngine and Config services.
 *   It accepts "SocketIMEngine" and "SocketConfig" clients.
 * - "Panel"\n
 *   The Panel server provides GUI and Helper management services.
 *   It accepts "FrontEnd" and "Helper" clients.
 *
 * The valid types of clients are:
 * - "SocketIMEngine"\n
 *   The socket IMEngine client acts as a proxy IMEngine forwarding all requests to SocketFrontEnd.
 *   It can only connect to "SocketFrontEnd" server.
 * - "SocketConfig"\n
 *   The socket Config client acts as a proxy Config forwarding all request  to SocketFrontEnd.
 *   It can only connect to "SocketFrontEnd" server.
 * - "FrontEnd"\n
 *   If a FrontEnd needs a Panel GUI services, it'll be a "FrontEnd" client of the Panel.
 *   It can only connect to "Panel" server.
 * - "Helper"\n
 *   All Helper objects should be "Helper" clients of a Panel.
 *   It can only connect to "Panel" server.
 *
 * Then the client and the server can communicate with each other via the socket by sending transactions.
 *
 * Multiple commands and their data may be put into one transaction with a restricted order.
 * The data of a command must be put into the transaction just follow the command itself.
 *
 * A transaction sent from a socket client to a socket server (eg. SocketIMEngine to SocketFrontEnd)
 * must be started with a SCIM_TRANS_CMD_REQUEST command followed by an uint32 magic key of the client
 * (returned by scim_socket_open_connection() function.
 *
 * A transaction sent back to a socket client from a socket server must be started with a
 * SCIM_TRANS_CMD_REPLY command.
 *
 * So for example, the layout of a transaction sent from SocketIMEngine to SocketFrontEnd may look like:
 * - #SCIM_TRANS_CMD_REQUEST
 * - an uint32 data (the magic key of a client)
 * - #SCIM_TRANS_CMD_PROCESS_KEY_EVENT
 * - an uint32 data (the id of the IMEngineInstance object used to process the KeyEvent)
 * - a scim::KeyEvent data (the KeyEvent to be processed)
 *
 * Some commands may be used in more than one protocols for similar purpose, but they may have different
 * data in different protocol.
 *
 * <b>Brief introduction of communication protocols used in SCIM:</b>
 *
 * Please refer to the descriptions of each Transaction commands for details.
 *
 * -# <b>Protocol used between SocketIMEngine and SocketFrontEnd</b>\n
 *   In this protocol, SocketFrontEnd is socket server, SocketIMEngine is client.
 *   - <b>from SocketIMEngine to SocketFrontEnd:</b>\n
 *     The Transaction sent from SocketIMEngine to SocketFrontEnd must
 *     start with #SCIM_TRANS_CMD_REQUEST and followed by an uint32 magic
 *     key which was returned by scim_socket_open_connection() and
 *     scim_socket_accept_connection().\n
 *     Before parsing the Transaction,
 *     SocketFrontEnd must verify if the magic key is matched.
 *     If the magic key is not matched, then SocketFrontEnd should just
 *     discard this transaction.\n
 *     There can be one or more commands and corresponding data right after the
 *     magic key.\n
 *     The valid commands which can be used here are:
 *     - #SCIM_TRANS_CMD_NEW_INSTANCE
 *     - #SCIM_TRANS_CMD_DELETE_INSTANCE
 *     - #SCIM_TRANS_CMD_DELETE_ALL_INSTANCES
 *     - #SCIM_TRANS_CMD_GET_FACTORY_LIST
 *     - #SCIM_TRANS_CMD_GET_FACTORY_NAME
 *     - #SCIM_TRANS_CMD_GET_FACTORY_AUTHORS
 *     - #SCIM_TRANS_CMD_GET_FACTORY_CREDITS
 *     - #SCIM_TRANS_CMD_GET_FACTORY_HELP
 *     - #SCIM_TRANS_CMD_GET_FACTORY_LOCALES
 *     - #SCIM_TRANS_CMD_GET_FACTORY_ICON_FILE
 *     - #SCIM_TRANS_CMD_GET_FACTORY_LANGUAGE
 *     - #SCIM_TRANS_CMD_PROCESS_KEY_EVENT
 *     - #SCIM_TRANS_CMD_MOVE_PREEDIT_CARET
 *     - #SCIM_TRANS_CMD_SELECT_CANDIDATE
 *     - #SCIM_TRANS_CMD_UPDATE_LOOKUP_TABLE_PAGE_SIZE
 *     - #SCIM_TRANS_CMD_LOOKUP_TABLE_PAGE_UP
 *     - #SCIM_TRANS_CMD_LOOKUP_TABLE_PAGE_DOWN
 *     - #SCIM_TRANS_CMD_RESET
 *     - #SCIM_TRANS_CMD_FOCUS_IN
 *     - #SCIM_TRANS_CMD_FOCUS_OUT
 *     - #SCIM_TRANS_CMD_TRIGGER_PROPERTY
 *     - #SCIM_TRANS_CMD_PROCESS_HELPER_EVENT
 *     - #SCIM_TRANS_CMD_UPDATE_CLIENT_CAPABILITIES
 *     - #SCIM_TRANS_CMD_LOAD_FILE
 *     - #SCIM_TRANS_CMD_CLOSE_CONNECTION
 *   - <b>from SocketFrontEnd to SocketIMEngine:</b>\n
 *     The Transaction sent back from SocketFrontEnd to SocketIMEngine must
 *     start with #SCIM_TRANS_CMD_REPLY and end with #SCIM_TRANS_CMD_OK or
 *     #SCIM_TRANS_CMD_FAIL to indicate if the request previously sent by
 *     SocketIMEngine was executed successfully.\n
 *     For some requests, like SCIM_TRANS_CMD_GET_FACTORY_LIST, etc.
 *     only some result data will be returned between #SCIM_TRANS_CMD_REPLY and #SCIM_TRANS_CMD_OK.\n
 *     For some requests, like SCIM_TRANS_CMD_PROCESS_KEY_EVENT, etc.
 *     one or more following commands and corresponding data may be returned between
 *     #SCIM_TRANS_CMD_REPLY and #SCIM_TRANS_CMD_OK commands.\n
 *     The valid commands can be used here are:
 *     - #SCIM_TRANS_CMD_SHOW_PREEDIT_STRING
 *     - #SCIM_TRANS_CMD_SHOW_AUX_STRING
 *     - #SCIM_TRANS_CMD_SHOW_LOOKUP_TABLE
 *     - #SCIM_TRANS_CMD_HIDE_PREEDIT_STRING
 *     - #SCIM_TRANS_CMD_HIDE_AUX_STRING
 *     - #SCIM_TRANS_CMD_HIDE_LOOKUP_TABLE
 *     - #SCIM_TRANS_CMD_UPDATE_PREEDIT_CARET
 *     - #SCIM_TRANS_CMD_UPDATE_PREEDIT_STRING
 *     - #SCIM_TRANS_CMD_UPDATE_AUX_STRING
 *     - #SCIM_TRANS_CMD_UPDATE_LOOKUP_TABLE
 *     - #SCIM_TRANS_CMD_COMMIT_STRING
 *     - #SCIM_TRANS_CMD_FORWARD_KEY_EVENT
 *     - #SCIM_TRANS_CMD_REGISTER_PROPERTIES
 *     - #SCIM_TRANS_CMD_UPDATE_PROPERTY
 *     - #SCIM_TRANS_CMD_BEEP
 *     - #SCIM_TRANS_CMD_START_HELPER
 *     - #SCIM_TRANS_CMD_STOP_HELPER
 *     - #SCIM_TRANS_CMD_SEND_HELPER_EVENT
 * -# <b>Protocol used between SocketConfig and SocketFrontEnd</b>\n
 *   In this protocol, SocketFrontEnd is socket server, SocketConfig is client.
 *   - <b>from SocketConfig to SocketFrontEnd:</b>\n
 *     The Transaction sent from SocketConfig to SocketFrontEnd must
 *     start with #SCIM_TRANS_CMD_REQUEST and followed by an uint32 magic
 *     key which was returned by scim_socket_open_connection() and
 *     scim_socket_accept_connection().\n
 *     Before parsing the Transaction,
 *     SocketFrontEnd must verify if the magic key is matched.
 *     If the magic key is not matched, then SocketFrontEnd should just
 *     discard this transaction.\n
 *     There can be one or more commands and corresponding data right after the
 *     magic key.\n
 *     The valid commands which can be used here are:
 *     - #SCIM_TRANS_CMD_FLUSH_CONFIG
 *     - #SCIM_TRANS_CMD_ERASE_CONFIG
 *     - #SCIM_TRANS_CMD_GET_CONFIG_STRING
 *     - #SCIM_TRANS_CMD_SET_CONFIG_STRING
 *     - #SCIM_TRANS_CMD_GET_CONFIG_INT
 *     - #SCIM_TRANS_CMD_SET_CONFIG_INT
 *     - #SCIM_TRANS_CMD_GET_CONFIG_BOOL
 *     - #SCIM_TRANS_CMD_SET_CONFIG_BOOL
 *     - #SCIM_TRANS_CMD_GET_CONFIG_DOUBLE
 *     - #SCIM_TRANS_CMD_SET_CONFIG_DOUBLE
 *     - #SCIM_TRANS_CMD_GET_CONFIG_VECTOR_STRING
 *     - #SCIM_TRANS_CMD_SET_CONFIG_VECTOR_STRING
 *     - #SCIM_TRANS_CMD_GET_CONFIG_VECTOR_INT
 *     - #SCIM_TRANS_CMD_SET_CONFIG_VECTOR_INT
 *     - #SCIM_TRANS_CMD_RELOAD_CONFIG
 *     - #SCIM_TRANS_CMD_LOAD_FILE
 *     - #SCIM_TRANS_CMD_CLOSE_CONNECTION
 *   - <b>from SocketFrontEnd to SocketConfig:</b>\n
 *     The Transaction sent back from SocketFrontEnd to SocketConfig must
 *     start with #SCIM_TRANS_CMD_REPLY and end with #SCIM_TRANS_CMD_OK or
 *     #SCIM_TRANS_CMD_FAIL to indicate if the request previously sent by
 *     SocketConfig was executed successfully.\n
 *     For some requests, like SCIM_TRANS_CMD_FLUSH_CONFIG, etc.
 *     no result data will be returned.\n
 *     For some requests, like SCIM_TRANS_CMD_GET_CONFIG_STRING, etc.
 *     the corresponding data will be returned between
 *     #SCIM_TRANS_CMD_REPLY and #SCIM_TRANS_CMD_OK commands.\n
 * -# <b>Protocol used between FrontEnds and Panel</b>\n
 *   In this protocol, Panel (eg. scim-panel-gtk or scim-panel-kde) is socket server, FrontEnds are clients.
 *   - <b>from FrontEnds to Panel:</b>\n
 *     The Transaction sent from FrontEnds to Panel must
 *     start with #SCIM_TRANS_CMD_REQUEST and followed by an uint32 magic
 *     key which was returned by scim_socket_open_connection() and
 *     scim_socket_accept_connection(). Then there must be an uint32 id
 *     for current focused input context right after the magic key.\n
 *     Before parsing the Transaction,
 *     Panel must verify if the magic key is matched.
 *     If the magic key is not matched, then Panel should just
 *     discard this transaction.\n
 *     There can be one or more commands and corresponding data right after the
 *     magic key.\n
 *     The valid commands which can be used here are:
 *     - #SCIM_TRANS_CMD_UPDATE_SCREEN
 *     - #SCIM_TRANS_CMD_UPDATE_SPOT_LOCATION
 *     - #SCIM_TRANS_CMD_PANEL_TURN_ON
 *     - #SCIM_TRANS_CMD_PANEL_TURN_OFF
 *     - #SCIM_TRANS_CMD_PANEL_UPDATE_FACTORY_INFO
 *     - #SCIM_TRANS_CMD_PANEL_SHOW_HELP
 *     - #SCIM_TRANS_CMD_PANEL_SHOW_FACTORY_MENU
 *     - #SCIM_TRANS_CMD_SHOW_PREEDIT_STRING
 *     - #SCIM_TRANS_CMD_SHOW_AUX_STRING
 *     - #SCIM_TRANS_CMD_SHOW_LOOKUP_TABLE
 *     - #SCIM_TRANS_CMD_HIDE_PREEDIT_STRING
 *     - #SCIM_TRANS_CMD_HIDE_AUX_STRING
 *     - #SCIM_TRANS_CMD_HIDE_LOOKUP_TABLE
 *     - #SCIM_TRANS_CMD_UPDATE_PREEDIT_CARET
 *     - #SCIM_TRANS_CMD_UPDATE_PREEDIT_STRING
 *     - #SCIM_TRANS_CMD_UPDATE_AUX_STRING
 *     - #SCIM_TRANS_CMD_UPDATE_LOOKUP_TABLE
 *     - #SCIM_TRANS_CMD_REGISTER_PROPERTIES
 *     - #SCIM_TRANS_CMD_UPDATE_PROPERTY
 *     - #SCIM_TRANS_CMD_START_HELPER
 *     - #SCIM_TRANS_CMD_STOP_HELPER
 *     - #SCIM_TRANS_CMD_SEND_HELPER_EVENT
 *   - <b>from Panel to FrontEnds:</b>\n
 *     The Transaction sent from Panel to FrontEnds must
 *     start with #SCIM_TRANS_CMD_REPLY.
 *     For the following commands except
 *     #SCIM_TRANS_CMD_RELOAD_CONFIG and #SCIM_TRANS_CMD_EXIT,
 *     there must be an uint32 id of the currently focused input context
 *     right after the #SCIM_TRANS_CMD_REPLY command.
 *     Then there can be one or more commands and corresponding data following.\n
 *     The valid commands which can be used here are:
 *     - #SCIM_TRANS_CMD_RELOAD_CONFIG
 *     - #SCIM_TRANS_CMD_EXIT
 *     - #SCIM_TRANS_CMD_UPDATE_LOOKUP_TABLE_PAGE_SIZE
 *     - #SCIM_TRANS_CMD_LOOKUP_TABLE_PAGE_UP
 *     - #SCIM_TRANS_CMD_LOOKUP_TABLE_PAGE_DOWN
 *     - #SCIM_TRANS_CMD_PROCESS_KEY_EVENT
 *     - #SCIM_TRANS_CMD_MOVE_PREEDIT_CARET
 *     - #SCIM_TRANS_CMD_SELECT_CANDIDATE
 *     - #SCIM_TRANS_CMD_TRIGGER_PROPERTY
 *     - #SCIM_TRANS_CMD_PROCESS_HELPER_EVENT
 *     - #SCIM_TRANS_CMD_COMMIT_STRING
 *     - #SCIM_TRANS_CMD_FORWARD_KEY_EVENT
 *     - #SCIM_TRANS_CMD_PANEL_REQUEST_HELP
 *     - #SCIM_TRANS_CMD_PANEL_REQUEST_FACTORY_MENU
 *     - #SCIM_TRANS_CMD_PANEL_CHANGE_FACTORY
 * -# <b>Protocol used between Helper and Panel</b>\n
 *   In this protocol, Panel (eg. scim-panel-gtk or scim-panel-kde) is socket server, Helper is client.
 *   - <b>from Helper to Panel:</b>\n
 *     The Transaction sent from Helper to Panel must
 *     start with #SCIM_TRANS_CMD_REQUEST and followed by an uint32 magic
 *     key which was returned by scim_socket_open_connection() and
 *     scim_socket_accept_connection().
 *     Before parsing the Transaction,
 *     Panel must verify if the magic key is matched.
 *     If the magic key is not matched, then Panel should just
 *     discard this transaction.\n
 *     There can be one or more commands and corresponding data right after the
 *     magic key.\n
 *     The valid commands which can be used here are:
 *     - #SCIM_TRANS_CMD_PANEL_REGISTER_HELPER
 *     - #SCIM_TRANS_CMD_PANEL_SEND_IMENGINE_EVENT
 *     - #SCIM_TRANS_CMD_PANEL_SEND_KEY_EVENT
 *     - #SCIM_TRANS_CMD_REGISTER_PROPERTIES
 *     - #SCIM_TRANS_CMD_UPDATE_PROPERTY
 *     - #SCIM_TRANS_CMD_FORWARD_KEY_EVENT
 *     - #SCIM_TRANS_CMD_COMMIT_STRING
 *   - <b>from Panel to Helper:</b>\n
 *     The Transaction sent from Panel to Helper must
 *     start with #SCIM_TRANS_CMD_REPLY and followed by an
 *     uint32 input context id and a scim::String input context UUID.
 *     Then there can be one or more commands and corresponding data just after the UUID.\n
 *     The valid commands which can be used here are:
 *     - #SCIM_TRANS_CMD_EXIT
 *     - #SCIM_TRANS_CMD_UPDATE_SCREEN
 *     - #SCIM_TRANS_CMD_UPDATE_SPOT_LOCATION
 *     - #SCIM_TRANS_CMD_TRIGGER_PROPERTY
 *     - #SCIM_TRANS_CMD_HELPER_PROCESS_IMENGINE_EVENT
 *
 * @addtogroup TransactionCommands
 * @ingroup InputServiceFramework
 * @{
 */

/// Unknown command. No use.
#ifndef SCIM_TRANS_CMD_UNKNOWN
    #define SCIM_TRANS_CMD_UNKNOWN                           0
#endif

// Common Commands

/**
 * @brief It's the first command which should be put into the Transaction
 *        sending from a socket client to a socket server.
 *
 * The corresponding data for this command is an uint32 magic key
 * which is returned by scim_socket_open_connection() function.
 *
 */
#ifndef SCIM_TRANS_CMD_REQUEST
    #define SCIM_TRANS_CMD_REQUEST                           1
#endif

/**
 * @brief It's the first command which should be put into the Transaction
 *        sending from a socket server to a socket client.
 *
 * The corresponding data for this command is different in
 * each protocol. Please refer to the previous protocol notes for details.
 *
 */
#ifndef SCIM_TRANS_CMD_REPLY
    #define SCIM_TRANS_CMD_REPLY                             2
#endif

/**
 * @brief This command is usually used in the Transaction sending from
 *        a socket server to a socket client to indicate that the request
 *        previously sent from the client was executed successfully.
 *
 * There is no data for this command.
 *
 */
#ifndef SCIM_TRANS_CMD_OK
    #define SCIM_TRANS_CMD_OK                                3
#endif

/**
 * @brief This command is usually used in the Transaction sending from
 *        a socket server to a socket client to indicate that the request
 *        previously sent from the client was failed to be executed.
 *
 * There is no data for this command.
 *
 */
#ifndef SCIM_TRANS_CMD_FAIL
    #define SCIM_TRANS_CMD_FAIL                              4
#endif

/**
 * @brief This command is used internally by scim_socket_open_connection() and
 *        scim_socket_accept_connection().
 *
 * It's sent from a socket client to a socket server to
 * request the server to create the connection.
 *
 * The corresponding data are:
 *   - (scim::String) a version string (the binary version of SCIM).
 *   - (scim::String) type of the client, eg. "SocketIMEngine", "FrontEnd", "Helper" etc.
 *
 * If the socket server accept the connection request, it must send back a Transaction with
 * following content:
 *   - #SCIM_TRANS_CMD_REPLY
 *   - (scim::String) a comma separated server types which are supported by the server, eg. "SocketFrontEnd" etc.
 *   - (uint32) a magic key used to validate the communication later.
 *
 * Then if the client accept the result too, it must send the following content back to the
 * socket server:
 *   - #SCIM_TRANS_CMD_REPLY
 *   - #SCIM_TRANS_CMD_OK
 *
 * Otherwise, the client must return:
 *   - #SCIM_TRANS_CMD_REPLY
 *   - #SCIM_TRANS_CMD_FAIL
 *
 * If the socket server do not accept the connection in the first stage, it should discard the request and send
 * nothing back.
 *
 */
#ifndef SCIM_TRANS_CMD_OPEN_CONNECTION
    #define SCIM_TRANS_CMD_OPEN_CONNECTION                   5
#endif

/**
 * @brief It's used to request the socket server to close the connection forcedly.
 *
 * It's currently not used at all.
 *
 */
#ifndef SCIM_TRANS_CMD_CLOSE_CONNECTION
    #define SCIM_TRANS_CMD_CLOSE_CONNECTION                  6
#endif

/**
 * @brief Request the socket server to load and send a file to the client.
 *
 * The corresponding data is:
 *   - (scim::String) the full file path to be loaded.
 *
 * If the file is loaded successfully, then the server should send back:
 *   - #SCIM_TRANS_CMD_REPLY
 *   - (raw buffer) the buffer which holds the file content.
 *   - #SCIM_TRANS_CMD_OK
 *
 * Otherwise it should send back:
 *   - #SCIM_TRANS_CMD_REPLY
 *   - #SCIM_TRANS_CMD_FAIL
 *
 * This command is only supported by SocketFrontEnd.
 *
 */
#ifndef SCIM_TRANS_CMD_LOAD_FILE
    #define SCIM_TRANS_CMD_LOAD_FILE                         7
#endif

/**
 * @brief Request the socket server to save a buffer into a file.
 *
 * The corresponding data is:
 *   - (scim::String) the full file path to be used to save the buffer.
 *   - (raw buffer) the buffer to be saved.
 *
 * If the file is saved successfully, then the server should return:
 *   - #SCIM_TRANS_CMD_REPLY
 *   - #SCIM_TRANS_CMD_OK
 *
 * Otherwise it should return:
 *   - #SCIM_TRANS_CMD_REPLY
 *   - #SCIM_TRANS_CMD_FAIL
 *
 * This command is currently not supported by any servers.
 *
 */
#ifndef SCIM_TRANS_CMD_SAVE_FILE
    #define SCIM_TRANS_CMD_SAVE_FILE                         8
#endif

/**
 * @brief This command should be sent from a socket server to its clients to let them exit.
 *
 * No data is associated to this command.
 *
 * This command is currently only used by Panel server.
 *
 */
#ifndef SCIM_TRANS_CMD_EXIT
    #define SCIM_TRANS_CMD_EXIT                              99
#endif

// Socket IMEngine to Socket FrontEnd

/**
 * @brief This command is used in SocketIMEngine to SocketFrontEnd and
 *        Panel to FrontEnd protocols to send a KeyEvent to an IMEngineInstance.
 *
 * When used in SocketIMEngine to SocketFrontEnd protocol,
 * the corresponding data is:
 *   - (uint32) the id of the IMEngineInstance to process the KeyEvent.
 *   - (KeyEvent) the KeyEvent object to be processed.
 *
 * The Transaction returned from SocketFrontEnd should contain:
 *   - #SCIM_TRANS_CMD_REPLY
 *   - (any valid commands and their corresponding data)
 *   - #SCIM_TRANS_CMD_OK or #SCIM_TRANS_CMD_FAIL to indicate
 *     that if the KeyEvent was processed successfully.
 *
 * When used in Panel to FrontEnds protocol, the corresponding data is:
 *   - (KeyEvent) the KeyEvent object to be processed.
 */
const int SCIM_TRANS_CMD_PROCESS_KEY_EVENT                = 100;
const int SCIM_TRANS_CMD_MOVE_PREEDIT_CARET               = 101;
const int SCIM_TRANS_CMD_SELECT_CANDIDATE                 = 102;
const int SCIM_TRANS_CMD_UPDATE_LOOKUP_TABLE_PAGE_SIZE    = 103;
const int SCIM_TRANS_CMD_LOOKUP_TABLE_PAGE_UP             = 104;
const int SCIM_TRANS_CMD_LOOKUP_TABLE_PAGE_DOWN           = 105;
const int SCIM_TRANS_CMD_RESET                            = 106;
const int SCIM_TRANS_CMD_FOCUS_IN                         = 107;
const int SCIM_TRANS_CMD_FOCUS_OUT                        = 108;
const int SCIM_TRANS_CMD_TRIGGER_PROPERTY                 = 109;
const int SCIM_TRANS_CMD_PROCESS_HELPER_EVENT             = 110;
const int SCIM_TRANS_CMD_UPDATE_CLIENT_CAPABILITIES       = 111;
const int ISM_TRANS_CMD_SELECT_ASSOCIATE                  = 112;
const int ISM_TRANS_CMD_UPDATE_ASSOCIATE_TABLE_PAGE_SIZE  = 113;
const int ISM_TRANS_CMD_ASSOCIATE_TABLE_PAGE_UP           = 114;
const int ISM_TRANS_CMD_ASSOCIATE_TABLE_PAGE_DOWN         = 115;
const int ISM_TRANS_CMD_SELECT_AUX                        = 116;
const int ISM_TRANS_CMD_SET_PREDICTION_ALLOW              = 117;
const int ISM_TRANS_CMD_UPDATE_CANDIDATE_ITEM_LAYOUT      = 118;
const int SCIM_TRANS_CMD_SET_AUTOCAPITAL_TYPE             = 119;

// Socket FrontEnd to Socket IMEngine
// FrontEnds to Panel
const int SCIM_TRANS_CMD_SHOW_PREEDIT_STRING              = 150;
const int SCIM_TRANS_CMD_SHOW_AUX_STRING                  = 151;
const int SCIM_TRANS_CMD_SHOW_LOOKUP_TABLE                = 152;
const int SCIM_TRANS_CMD_HIDE_PREEDIT_STRING              = 153;
const int SCIM_TRANS_CMD_HIDE_AUX_STRING                  = 154;
const int SCIM_TRANS_CMD_HIDE_LOOKUP_TABLE                = 155;
const int SCIM_TRANS_CMD_UPDATE_PREEDIT_CARET             = 156;
const int SCIM_TRANS_CMD_UPDATE_PREEDIT_STRING            = 157;
const int SCIM_TRANS_CMD_UPDATE_AUX_STRING                = 158;
const int SCIM_TRANS_CMD_UPDATE_LOOKUP_TABLE              = 159;
const int SCIM_TRANS_CMD_COMMIT_STRING                    = 160;
const int SCIM_TRANS_CMD_FORWARD_KEY_EVENT                = 161;
const int SCIM_TRANS_CMD_REGISTER_PROPERTIES              = 162;
const int SCIM_TRANS_CMD_UPDATE_PROPERTY                  = 163;
const int SCIM_TRANS_CMD_BEEP                             = 164;
const int SCIM_TRANS_CMD_START_HELPER                     = 165;
const int SCIM_TRANS_CMD_STOP_HELPER                      = 166;
const int SCIM_TRANS_CMD_SEND_HELPER_EVENT                = 167;
const int SCIM_TRANS_CMD_GET_SURROUNDING_TEXT             = 168;
const int SCIM_TRANS_CMD_DELETE_SURROUNDING_TEXT          = 169;
const int ISM_TRANS_CMD_SHOW_ASSOCIATE_TABLE              = 170;
const int ISM_TRANS_CMD_HIDE_ASSOCIATE_TABLE              = 171;
const int ISM_TRANS_CMD_UPDATE_ASSOCIATE_TABLE            = 172;
const int ISM_TRANS_CMD_TRANSACTION_CONTINUE              = 173;
const int SCIM_TRANS_CMD_GET_SELECTION                    = 174;
const int SCIM_TRANS_CMD_SET_SELECTION                    = 175;
const int SCIM_TRANS_CMD_SEND_PRIVATE_COMMAND             = 176;

// Socket IMEngine to Socket FrontEnd
const int SCIM_TRANS_CMD_NEW_INSTANCE                     = 200;
const int SCIM_TRANS_CMD_DELETE_INSTANCE                  = 201;
const int SCIM_TRANS_CMD_DELETE_ALL_INSTANCES             = 202;

const int SCIM_TRANS_CMD_GET_FACTORY_LIST                 = 203;
const int SCIM_TRANS_CMD_GET_FACTORY_NAME                 = 204;
const int SCIM_TRANS_CMD_GET_FACTORY_AUTHORS              = 205;
const int SCIM_TRANS_CMD_GET_FACTORY_CREDITS              = 206;
const int SCIM_TRANS_CMD_GET_FACTORY_HELP                 = 207;
const int SCIM_TRANS_CMD_GET_FACTORY_LOCALES              = 208;
const int SCIM_TRANS_CMD_GET_FACTORY_ICON_FILE            = 209;
const int SCIM_TRANS_CMD_GET_FACTORY_LANGUAGE             = 210;

// Socket Config to Socket FrontEnd
const int SCIM_TRANS_CMD_FLUSH_CONFIG                     = 300;
const int SCIM_TRANS_CMD_ERASE_CONFIG                     = 301;
const int SCIM_TRANS_CMD_GET_CONFIG_STRING                = 302;
const int SCIM_TRANS_CMD_SET_CONFIG_STRING                = 303;
const int SCIM_TRANS_CMD_GET_CONFIG_INT                   = 304;
const int SCIM_TRANS_CMD_SET_CONFIG_INT                   = 305;
const int SCIM_TRANS_CMD_GET_CONFIG_BOOL                  = 306;
const int SCIM_TRANS_CMD_SET_CONFIG_BOOL                  = 307;
const int SCIM_TRANS_CMD_GET_CONFIG_DOUBLE                = 308;
const int SCIM_TRANS_CMD_SET_CONFIG_DOUBLE                = 309;
const int SCIM_TRANS_CMD_GET_CONFIG_VECTOR_STRING         = 310;
const int SCIM_TRANS_CMD_SET_CONFIG_VECTOR_STRING         = 311;
const int SCIM_TRANS_CMD_GET_CONFIG_VECTOR_INT            = 312;
const int SCIM_TRANS_CMD_SET_CONFIG_VECTOR_INT            = 313;
const int SCIM_TRANS_CMD_RELOAD_CONFIG                    = 314;

// Used by Panel and Helper
const int SCIM_TRANS_CMD_UPDATE_SCREEN                    = 400;
const int SCIM_TRANS_CMD_UPDATE_SPOT_LOCATION             = 401;
const int ISM_TRANS_CMD_UPDATE_CURSOR_POSITION            = 402;
const int ISM_TRANS_CMD_UPDATE_SURROUNDING_TEXT           = 403;
const int ISM_TRANS_CMD_UPDATE_SELECTION                  = 404;

// Privately used by panel.
const int SCIM_TRANS_CMD_PANEL_EXIT                       = 500;

// FrontEnd Client to Panel
const int SCIM_TRANS_CMD_PANEL_TURN_ON                    = 501;
const int SCIM_TRANS_CMD_PANEL_TURN_OFF                   = 502;
const int SCIM_TRANS_CMD_PANEL_UPDATE_FACTORY_INFO        = 503;
const int SCIM_TRANS_CMD_PANEL_SHOW_HELP                  = 504;
const int SCIM_TRANS_CMD_PANEL_SHOW_FACTORY_MENU          = 505;
const int SCIM_TRANS_CMD_PANEL_REGISTER_INPUT_CONTEXT     = 506;
const int SCIM_TRANS_CMD_PANEL_REMOVE_INPUT_CONTEXT       = 507;
const int SCIM_TRANS_CMD_PANEL_RESET_INPUT_CONTEXT        = 508;

// Panel to FrontEnd Client
const int SCIM_TRANS_CMD_PANEL_REQUEST_HELP               = 520;
const int SCIM_TRANS_CMD_PANEL_REQUEST_FACTORY_MENU       = 521;
const int SCIM_TRANS_CMD_PANEL_CHANGE_FACTORY             = 522;
const int ISM_TRANS_CMD_PANEL_RESET_KEYBOARD_ISE          = 523;
const int ISM_TRANS_CMD_PANEL_UPDATE_KEYBOARD_ISE         = 524;

// Helper Client To Panel
const int SCIM_TRANS_CMD_PANEL_REGISTER_HELPER            = 540;
const int SCIM_TRANS_CMD_PANEL_SEND_IMENGINE_EVENT        = 541;
const int SCIM_TRANS_CMD_PANEL_SEND_KEY_EVENT             = 542;
const int SCIM_TRANS_CMD_PANEL_REGISTER_ACTIVE_HELPER     = 543;

// Panel to Helper Client
const int SCIM_TRANS_CMD_HELPER_PROCESS_IMENGINE_EVENT    = 602;
const int SCIM_TRANS_CMD_HELPER_ATTACH_INPUT_CONTEXT      = 603;
const int SCIM_TRANS_CMD_HELPER_DETACH_INPUT_CONTEXT      = 604;

// HelperManager Commands
const int SCIM_TRANS_CMD_HELPER_MANAGER_GET_HELPER_LIST   = 700;
const int SCIM_TRANS_CMD_HELPER_MANAGER_RUN_HELPER        = 701;
const int SCIM_TRANS_CMD_HELPER_MANAGER_STOP_HELPER       = 702;
const int SCIM_TRANS_CMD_HELPER_MANAGER_SEND_DISPLAY      = 703;
const int SCIM_TRANS_CMD_HELPER_MANAGER_SEND_ISE_LIST     = 704;
const int ISM_TRANS_CMD_GET_ACTIVE_ISE_LIST               = 705;
const int ISM_TRANS_CMD_PRELOAD_KEYBOARD_ISE              = 706;

/* IMControl to Panel */
const int ISM_TRANS_CMD_SHOW_ISE_PANEL                    = 1000;
const int ISM_TRANS_CMD_HIDE_ISE_PANEL                    = 1001;
const int ISM_TRANS_CMD_SHOW_ISF_CONTROL                  = 1002;
const int ISM_TRANS_CMD_HIDE_ISF_CONTROL                  = 1003;
const int ISM_TRANS_CMD_RESET_ISE_OPTION                  = 1004;
const int ISM_TRANS_CMD_SET_ACTIVE_ISE_BY_UUID            = 1005;
const int ISM_TRANS_CMD_GET_ACTIVE_ISE                    = 1006;
const int ISM_TRANS_CMD_GET_ISE_LIST                      = 1007;
const int ISM_TRANS_CMD_GET_ISE_INFORMATION               = 1008;
const int ISM_TRANS_CMD_SEND_WILL_SHOW_ACK                = 1009;
const int ISM_TRANS_CMD_SEND_WILL_HIDE_ACK                = 1010;
const int ISM_TRANS_CMD_RESET_DEFAULT_ISE                 = 1011;
const int ISM_TRANS_CMD_SET_HARDWARE_KEYBOARD_MODE        = 1012;
const int ISM_TRANS_CMD_SEND_CANDIDATE_WILL_HIDE_ACK      = 1013;
const int ISM_TRANS_CMD_GET_PANEL_CLIENT_ID               = 1014;
const int ISM_TRANS_CMD_REGISTER_PANEL_CLIENT             = 1015;
const int ISM_TRANS_CMD_GET_ISE_STATE                     = 1016;
const int ISM_TRANS_CMD_GET_ACTIVE_HELPER_OPTION          = 1017;
const int ISM_TRANS_CMD_SET_INITIAL_ISE_BY_UUID           = 1018;

/* IMControl to ISE */
const int ISM_TRANS_CMD_SET_ISE_MODE                      = 1108;
const int ISM_TRANS_CMD_SET_ISE_LANGUAGE                  = 1109;
const int ISM_TRANS_CMD_SET_ISE_IMDATA                    = 1110;
const int ISM_TRANS_CMD_GET_ISE_IMDATA                    = 1111;
const int ISM_TRANS_CMD_GET_ACTIVE_ISE_GEOMETRY           = 1112;
const int ISM_TRANS_CMD_SET_LAYOUT                        = 1113;
const int ISM_TRANS_CMD_GET_LAYOUT                        = 1114;
const int ISM_TRANS_CMD_SET_CAPS_MODE                     = 1115;
const int ISM_TRANS_CMD_GET_ISE_LANGUAGE_LOCALE           = 1116;
const int ISM_TRANS_CMD_RESET_ISE_CONTEXT                 = 1117;
const int ISM_TRANS_CMD_SET_RETURN_KEY_TYPE               = 1118;
const int ISM_TRANS_CMD_GET_RETURN_KEY_TYPE               = 1119;
const int ISM_TRANS_CMD_SET_RETURN_KEY_DISABLE            = 1120;
const int ISM_TRANS_CMD_GET_RETURN_KEY_DISABLE            = 1121;
const int ISM_TRANS_CMD_SET_INPUT_MODE                    = 1122;
const int ISM_TRANS_CMD_SET_INPUT_HINT                    = 1123;
const int ISM_TRANS_CMD_UPDATE_BIDI_DIRECTION             = 1124;
const int ISM_TRANS_CMD_SHOW_ISE_OPTION_WINDOW            = 1125;

/* ISE/Panel to IMControl */
const int ISM_TRANS_CMD_UPDATE_ISE_INPUT_CONTEXT          = 1151;
const int ISM_TRANS_CMD_UPDATE_ISF_CANDIDATE_PANEL        = 1152;


/* ISE to Panel */
const int ISM_TRANS_CMD_SET_CANDIDATE_UI                  = 1201;
const int ISM_TRANS_CMD_GET_CANDIDATE_UI                  = 1202;
const int ISM_TRANS_CMD_UPDATE_CANDIDATE_UI               = 1203;
const int ISM_TRANS_CMD_GET_KEYBOARD_ISE_LIST             = 1204;
const int ISM_TRANS_CMD_UPDATE_KEYBOARD_ISE_LIST          = 1205;
const int ISM_TRANS_CMD_SET_KEYBOARD_ISE_BY_UUID          = 1206;
const int ISM_TRANS_CMD_GET_KEYBOARD_ISE                  = 1207;
const int ISM_TRANS_CMD_UPDATE_KEYBOARD_ISE               = 1208;
const int ISM_TRANS_CMD_SET_CANDIDATE_POSITION            = 1209;
const int ISM_TRANS_CMD_HIDE_CANDIDATE                    = 1210;
const int ISM_TRANS_CMD_GET_CANDIDATE_GEOMETRY            = 1211;
const int ISM_TRANS_CMD_UPDATE_CANDIDATE_GEOMETRY         = 1212;
const int ISM_TRANS_CMD_UPDATE_ISE_GEOMETRY               = 1213;
const int ISM_TRANS_CMD_EXPAND_CANDIDATE                  = 1214;
const int ISM_TRANS_CMD_CONTRACT_CANDIDATE                = 1215;
const int ISM_TRANS_CMD_UPDATE_ISE_EXIT                   = 1216;
const int ISM_TRANS_CMD_SELECT_CANDIDATE                  = 1217;

/* Panel to ISE */
const int ISM_TRANS_CMD_CANDIDATE_MORE_WINDOW_SHOW        = 1251;
const int ISM_TRANS_CMD_CANDIDATE_MORE_WINDOW_HIDE        = 1252;
const int ISM_TRANS_CMD_UPDATE_DISPLAYED_CANDIDATE        = 1253;
const int ISM_TRANS_CMD_LONGPRESS_CANDIDATE               = 1254;
const int ISM_TRANS_CMD_UPDATE_LOOKUP_TABLE               = 1255;
const int ISM_TRANS_CMD_CANDIDATE_SHOW                    = 1256;
const int ISM_TRANS_CMD_CANDIDATE_HIDE                    = 1257;

const int ISM_TRANS_CMD_TURN_ON_LOG                       = 1301;

const int SCIM_TRANS_CMD_USER_DEFINED                     = 10000;
/**
 * @}
 */

} // namespace scim

#endif //__SCIM_TRANS_COMMANDS_H

/*
vi:ts=4:nowrap:ai:expandtab
*/

