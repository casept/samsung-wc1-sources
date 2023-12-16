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
 * $Id: scim_launcher.cpp,v 1.9 2005/06/15 00:19:08 suzhe Exp $
 *
 */

#define Uses_SCIM_FRONTEND_MODULE
#define Uses_SCIM_IMENGINE_MODULE
#define Uses_SCIM_BACKEND
#define Uses_SCIM_CONFIG_PATH
#define Uses_SCIM_CONFIG
#define Uses_C_LOCALE
#define Uses_SCIM_UTILITY
#define Uses_SCIM_PANEL_AGENT

#include "scim_private.h"
#include "scim.h"
#include <sys/types.h>
#include <sys/times.h>
#include <unistd.h>
#include <signal.h>
#include <privilege-control.h>
#include <vconf.h>
#include "isf_query_utility.h"

using namespace scim;

static FrontEndModule *frontend_module = 0;
static ConfigModule   *config_module = 0;
static ConfigPointer   config;

void signalhandler (int sig)
{
    if (!config.null ())
    {
        config->flush ();
        config.reset ();
    }

    if (frontend_module)
    {
        delete frontend_module;
        frontend_module = 0;
    }

    if (config_module)
    {
        delete config_module;
        config_module = 0;
    }

    std::cerr << "Successfully exited.\n";

    exit (0);
}

int main (int argc, char *argv [])
{
    struct tms tiks_buf;
    clock_t clock_start = times (&tiks_buf);

    BackEndPointer      backend;

    std::vector<String> engine_list;

    String config_name   ("simple");
    String frontend_name ("socket");

    int   new_argc = 0;
    char *new_argv [40];

    int i = 0;
    bool daemon = false;

    new_argv [new_argc ++] = argv [0];

    ISF_SAVE_LOG ("Start scim-launcher\n");

    perm_app_set_privilege ("isf", NULL, NULL);

    while (i < argc) {
        if (++i >= argc) break;

        if (String ("-f") == argv [i] ||
            String ("--frontend") == argv [i]) {
            if (++i >= argc) {
                std::cerr << "No argument for option " << argv [i-1] << "\n";
                return -1;
            }
            frontend_name = argv [i];
            continue;
        }

        if (String ("-c") == argv [i] ||
            String ("--config") == argv [i]) {
            if (++i >= argc) {
                std::cerr << "No argument for option " << argv [i-1] << "\n";
                return -1;
            }
            config_name = argv [i];
            continue;
        }

        if (String ("-d") == argv [i] ||
            String ("--daemon") == argv [i]) {
            daemon = true;
            continue;
        }

        if (String ("-e") == argv [i] ||
            String ("--engines") == argv [i]) {
            if (++i >= argc) {
                std::cerr << "No argument for option " << argv [i-1] << "\n";
                return -1;
            }
            if (String (argv [i]) == "all") {
                scim_get_imengine_module_list (engine_list);
                for (size_t j = 0; j < engine_list.size (); ++j) {
                    if (engine_list [j] == "socket") {
                        engine_list.erase (engine_list.begin () + j);
                        break;
                    }
                }
            } else if (String (argv [i]) != "none") {
                scim_split_string_list (engine_list, String (argv [i]), ',');
            }
            new_argv [new_argc ++] = const_cast <char *> ("-e");
            new_argv [new_argc ++] = argv [i];
            continue;
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
                for (size_t j=0; j<debug_mask_list.size (); j++)
                    DebugOutput::enable_debug_by_name (debug_mask_list [j]);
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

        if (String ("--") == argv [i])
            break;

        new_argv [new_argc ++] = argv [i];
    } /* End of command line parsing. */

    /* Construct new argv array for FrontEnd. */
    new_argv [new_argc ++] = const_cast <char *> ("-c");
    new_argv [new_argc ++] = const_cast <char *> (config_name.c_str ());

    /* Store the rest argvs into new_argv. */
    for (++i; i < argc && new_argc < 39; ++i) {
        new_argv [new_argc ++] = argv [i];
    }

    new_argv [new_argc] = 0;

    try {
        /* Try to load config module */
        if (config_name != "dummy") {
            /* load config module */
            config_module = new ConfigModule (config_name);

            if (!config_module->valid ()) {
                std::cerr << "Can not load " << config_name << " Config module. Using dummy module instead.\n";
                delete config_module;
                config_module = 0;
            }
        }

        if (config_module) {
            config = config_module->create_config ();
        } else {
            config = new DummyConfig ();
        }

        if (config.null ()) {
            std::cerr << "Can not create Config Object!\n";
            return 1;
        }
        gettime (clock_start, "Create Config");

        // Create folder for saving engine list
        scim_make_dir (USER_ENGINE_LIST_PATH);

        char *lang_str = vconf_get_str (VCONFKEY_LANGSET);
        if (lang_str) {
            setenv ("LANG", lang_str, 1);
            setlocale (LC_MESSAGES, lang_str);
            free (lang_str);
        } else {
            setenv ("LANG", "en_US.utf8", 1);
            setlocale (LC_MESSAGES, "en_US.utf8");
        }

        /* create backend */
        backend = new CommonBackEnd (config, engine_list);
        gettime (clock_start, "Create backend");

        /* load FrontEnd module */
        frontend_module = new FrontEndModule (frontend_name, backend, config, new_argc, new_argv);
        gettime (clock_start, "Create frontend");

        if (!frontend_module || !frontend_module->valid ()) {
            std::cerr << "Failed to load " << frontend_name << " FrontEnd module.\n";
            return 1;
        }

        signal (SIGQUIT, signalhandler);
        signal (SIGTERM, signalhandler);
        signal (SIGINT,  signalhandler);
        signal (SIGHUP,  signalhandler);

        if (daemon) {
            gettime (clock_start, "Starting as daemon ...");
            scim_daemon ();
        } else {
            std::cerr << "Starting ...\n";
        }

        bool is_load_info = true;
        if (engine_list.size () == 1 && engine_list[0] == "socket")
            is_load_info = false;
        backend->initialize (config, engine_list, true, is_load_info);
        gettime (clock_start, "backend->initialize");

        /* reset backend pointer, in order to destroy backend automatically. */
        backend.reset ();

        ISF_SAVE_LOG ("now running frontend...\n");

        frontend_module->run ();
    } catch (const std::exception & err) {
        ISF_SAVE_LOG ("caught an exception! : %s\n", err.what());

        std::cerr << err.what () << "\n";
        return 1;
    }

    return 0;
}

/*
vi:ts=4:ai:nowrap:expandtab
*/
