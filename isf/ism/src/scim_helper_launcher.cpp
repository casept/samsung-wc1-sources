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
 * 1. Call set_arg_info () before run_helper ()
 *
 * $Id: scim_helper_launcher.cpp,v 1.6 2005/05/16 01:25:46 suzhe Exp $
 *
 */

#define Uses_SCIM_HELPER_MODULE
#define Uses_SCIM_GLOBAL_CONFIG
#define Uses_SCIM_CONFIG_PATH
#include <stdlib.h>
#include "scim_private.h"
#include "scim.h"
#include <privilege-control.h>
#include <unistd.h>
#include "ise_preexec.h"

using namespace scim;

int main (int argc, char *argv [])
{
    int i = 0;
    int j = 0;
    String config;
    String display;
    String helper;
    String uuid;
    bool   daemon = false;

    char *p =  getenv ("DISPLAY");
    if (p) display = String (p);

    config = scim_global_config_read (SCIM_GLOBAL_CONFIG_DEFAULT_CONFIG_MODULE, String ("simple"));

    while (i < argc) {
        if (++i >= argc) break;

        if (String ("-c") == argv [i] ||
            String ("--config") == argv [i]) {
            if (++i >= argc) {
                std::cerr << "No argument for option " << argv [i-1] << "\n";
                exit (-1);
            }
            config = argv [i];
            continue;
        }

        if (String ("-d") == argv [i] ||
            String ("--daemon") == argv [i]) {
            daemon = true;
            continue;
        }

        if (String ("--display") == argv [i]) {
            if (++i >= argc) {
                std::cerr << "No argument for option " << argv [i-1] << "\n";
                exit (-1);
            }
            display = argv [i];
            continue;
        }

        if (String ("-h") == argv [i] ||
            String ("--help") == argv [i]) {
            std::cout << "Usage: " << argv [0] << " [options] module uuid\n\n"
                      << "The options are:\n"
                      << "  -c, --config name    Use specified config module, default is \"simple\".\n"
                      << "  -d, --daemon         Run as daemon.\n"
                      << "  --display name       run setup on a specified DISPLAY.\n"
                      << "  -h, --help           Show this help message.\n"
                      << "module                 The name of the Helper module\n"
                      << "uuid                   The uuid of the Helper to be launched.\n";
            return 0;
        }

        if (String ("-v") == argv [i] ||
            String ("--verbose") == argv [i]) {
            if (++i >= argc) {
                std::cerr << "No argument for option " << argv [i-1] << "\n";
                return -1;
            }
            DebugOutput::set_verbose_level (atoi (argv [i]));
            continue;
        }

        if (String ("-m") == argv [i] ||
            String ("--mask") == argv [i]) {
            if (++i >= argc) {
                std::cerr << "No argument for option " << argv [i-1] << "\n";
                return -1;
            }
            if (String (argv [i]) != "none") {
                std::vector<String> debug_mask_list;
                scim_split_string_list (debug_mask_list, argv [i], ',');
                DebugOutput::disable_debug (SCIM_DEBUG_AllMask);
                for (size_t k=0; k<debug_mask_list.size (); k++)
                    DebugOutput::enable_debug_by_name (debug_mask_list [k]);
            }
            continue;
        }

        if (String ("-o") == argv [i] ||
            String ("--output") == argv [i]) {
            if (++i >= argc) {
                std::cerr << "No argument for option " << argv [i-1] << "\n";
                return -1;
            }
            DebugOutput::set_output (String (argv [i]));
            continue;
        }

        ++j;

        if (j == 1) {
            helper = String (argv [i]);
            continue;
        } else if (j == 2) {
            uuid = String (argv [i]);
            continue;
        }

        ISF_SAVE_LOG ("Invalid command line option: %d %s...\n", i, argv [i]);

        std::cerr << "Invalid command line option: " << argv [i] << "\n";
        return -1;
    }

    SCIM_DEBUG_MAIN(1) << "scim-helper-launcher: " << config << " " << display << " " << helper << " " << uuid << "\n";
    ISF_SAVE_LOG ("Helper ISE (%s %s) is launching...\n", helper.c_str (), uuid.c_str ());

    if (!helper.length () || !uuid.length ()) {
        std::cerr << "Module name or Helper UUID is missing.\n";
        return -1;
    }

    int ret = ise_preexec (helper.c_str (), uuid.c_str ());
    if (ret < 0) {
        ISF_SAVE_LOG ("ise_preexec failed (%s, %d)\n", helper.c_str (), ret);

        std::cerr << "ise_preexec failed(" << helper << ret << ")\n";
        return -1;
    }

    perm_app_set_privilege ("isf", NULL, NULL);

    HelperModule helper_module (helper);

    if (!helper_module.valid () || helper_module.number_of_helpers () == 0) {
        ISF_SAVE_LOG ("Unable to load helper module(%s)\n", helper.c_str ());

        std::cerr << "Unable to load helper module(" << helper << ")\n";
        return -1;
    }

    ConfigPointer config_pointer = ConfigBase::get (true, config);

    if (config_pointer.null ()) {
        config_pointer = new DummyConfig ();
    }

//    if (daemon) scim_daemon ();

    helper_module.set_arg_info (argc, argv);
    helper_module.run_helper (uuid, config_pointer, display);
    helper_module.unload ();

    if (!config_pointer.null ())
        config_pointer.reset ();
    ConfigBase::set (0);
    ISF_SAVE_LOG ("Helper ISE (%s) is destroyed!!!\n", uuid.c_str ());
}

/*
vi:ts=4:nowrap:ai:expandtab
*/
