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

#define Uses_SCIM_HELPER_MANAGER
#define Uses_SCIM_PANEL_CLIENT

#include "scim.h"
#include "scim_private.h"

#undef GDK_WINDOWING_X11

#if GDK_MULTIHEAD_SAFE
#include <gdk/gdk.h>
#include <gdk/gdkx.h>
#include <gtk/gtk.h>
#endif

#include <string.h>
#include <list>


using namespace scim;

static void _enable_panel_agent_log (bool _enable_debug)
{
    String display_name;
    String config_name ("socket");
    int id = 0;

    {
#if GDK_MULTIHEAD_SAFE
        const char *p = gdk_display_get_name (gdk_display_get_default ());
#else
        const char *p = getenv ("DISPLAY");
#endif
        if (p) display_name = String (p);
    }

    String address = scim_get_default_panel_socket_address (display_name);

    SocketAddress m_socket_address;
    SocketClient  m_socket_client;
    m_socket_address.set_address (address);

    if (!m_socket_address.valid ()) {
        printf ("m_socket_address is not valid!!!\n");
        return;
    }

    PanelClient panel_client;
    if (panel_client.open_connection (config_name, display_name) >= 0) {
        panel_client.prepare (id);
        panel_client.turn_on_log (id, _enable_debug);
        panel_client.send ();
    }
}

static void _enable_frontend_log (bool _enable_debug)
{
    HelperManager m_helper_manager;

    m_helper_manager.turn_on_log (_enable_debug);
}

int main (int argc, char **argv)
{
    bool _enable_debug;

    if (argc != 2) {
        printf ("usage: isf_log [on/off]\n");
        return -1;
    }

    if (!strcmp (argv[1], "on"))
        _enable_debug = true;
    else if (!strcmp (argv[1], "off"))
        _enable_debug = false;
    else {
        printf ("usage: isf_log [on/off]\n");
        return -1;
    }

#if GDK_MULTIHEAD_SAFE
    gtk_init (&argc, &argv);
#endif

    try {
        _enable_panel_agent_log (_enable_debug);
        _enable_frontend_log (_enable_debug);
    } catch (...) {
        printf ("exception happen...\n");
    }
    return 0;
}

/*
vi:ts=4:nowrap:ai:expandtab
*/
