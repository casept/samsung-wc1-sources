/** @file scim_socket_imengine.h
 * definition of SocketFactory related classes.
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
 * 1. Use ISE cache file
 * 2. Add new interface APIs for keyboard ISE
 *    a. select_aux (), set_prediction_allow () and set_layout ()
 *    b. update_candidate_item_layout (), update_cursor_position () and update_displayed_candidate_number ()
 *    c. candidate_more_window_show (), candidate_more_window_hide () and longpress_candidate ()
 *    d. set_imdata () and reset_option ()
 * 3. Add get_option () in SocketFactory
 *
 * $Id: scim_socket_imengine.h,v 1.13 2005/07/06 03:57:05 suzhe Exp $
 */

#if !defined (__SCIM_SOCKET_IMENGINE_H)
#define __SCIM_SOCKET_IMENGINE_H

namespace scim {

class SocketFactory;

class EAPI SocketFactory : public IMEngineFactoryBase
{
    WideString m_name;

    String     m_language;

    String     m_peer_uuid;

    String     m_icon_file;

    bool       m_ok;

    unsigned int m_option;

    friend class SocketInstance;

public:
    SocketFactory (const String &peer_uuid);

    bool valid () const { return m_ok; }

    virtual ~SocketFactory ();

    virtual WideString  get_name () const;
    virtual WideString  get_authors () const;
    virtual WideString  get_credits () const;
    virtual WideString  get_help () const;
    virtual String      get_uuid () const;
    virtual String      get_icon_file () const;
    virtual String      get_language () const;
    virtual unsigned int get_option () const;

    virtual IMEngineInstancePointer create_instance (const String& encoding, int id = -1);

private:
    int  create_peer_instance (const String &encoding);
};

class EAPI SocketInstance : public IMEngineInstanceBase
{
    SocketFactory *m_factory;
    int            m_peer_id;
    Connection     m_signal_reconnect_connection;

public:
    SocketInstance (SocketFactory *factory, const String& encoding, int id, int peer_id);
    virtual ~SocketInstance ();

    virtual bool process_key_event (const KeyEvent& key);
    virtual void move_preedit_caret (unsigned int pos);
    virtual void select_aux (unsigned int item);
    virtual void select_candidate (unsigned int item);
    virtual void update_lookup_table_page_size (unsigned int page_size);
    virtual void lookup_table_page_up ();
    virtual void lookup_table_page_down ();
    virtual void set_prediction_allow (bool allow);
    virtual void set_layout (unsigned int layout);
    virtual void reset_option ();
    virtual void reset ();
    virtual void focus_in ();
    virtual void focus_out ();
    virtual void trigger_property (const String &property);
    virtual void process_helper_event (const String &helper_uuid, const Transaction &trans);
    virtual void update_client_capabilities (unsigned int cap);
    virtual void update_candidate_item_layout (const std::vector<unsigned int> &row_items);
    virtual void update_cursor_position (unsigned int cursor_pos);
    virtual void update_displayed_candidate_number (unsigned int number);
    virtual void candidate_more_window_show (void);
    virtual void candidate_more_window_hide (void);
    virtual void longpress_candidate (unsigned int index);
    virtual void set_imdata (const char *data, unsigned int len);
    virtual void set_autocapital_type (int mode);
    virtual void set_input_hint (unsigned int input_hint);
    virtual void update_bidi_direction (unsigned int bidi_direction);

private:
    bool commit_transaction (Transaction &trans);
    bool do_transaction (Transaction &trans, bool &ret);
    void reconnect_callback (void);
};

// Forward declaration
class SocketIMEngineGlobal;

} // namespace scim

#endif
/*
vi:ts=4:nowrap:ai:expandtab
*/

