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

#ifndef __ISF_IMCONTROL_CLIENT_H
#define __ISF_IMCONTROL_CLIENT_H

namespace scim
{

typedef Slot1<void, int> IMControlClientSlotVoid;


class EAPI IMControlClient
{
    class IMControlClientImpl;
    IMControlClientImpl *m_impl;

    IMControlClient (const IMControlClient &);
    const IMControlClient & operator = (const IMControlClient &);

public:
    IMControlClient ();
    ~IMControlClient ();

    int  open_connection        (void);
    void close_connection       (void);
    bool is_connected           (void) const;
    int  get_panel2imclient_connection_number (void) const;
    bool prepare                (void);
    bool send                   (void);

    void set_active_ise_by_uuid (const char* uuid);
    void get_active_ise (String &uuid);
    void get_ise_list (int* count, char*** iselist);
    void get_ise_info (const char* uuid, String &name, String &language, int &type, int &option, String &module_name);
    void reset_ise_option (void);
    void set_active_ise_to_default (void);
    void set_initial_ise_by_uuid (const char* uuid);
    void show_ise_selector ();
    void show_ise_option_window ();
};

}

#endif /* __ISF_IMCONTROL_CLIENT_H */

/*
vi:ts=4:nowrap:ai:expandtab
*/
