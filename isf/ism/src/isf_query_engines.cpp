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

#define Uses_SCIM_DEBUG
#define Uses_SCIM_CONFIG
#define Uses_SCIM_CONFIG_MODULE
#define Uses_SCIM_CONFIG_PATH
#define Uses_SCIM_UTILITY
#define Uses_SCIM_PANEL_AGENT


#include <sys/stat.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <vconf.h>
#include "scim_private.h"
#include "scim.h"
#include "isf_query_utility.h"
#include <privilege-control.h>


using namespace scim;


static void print_help (void)
{
    std::cout << "Usage: isf-query-engines [option] ISE_MODULE_NAME\n\n"
         << "The options are: \n"
         << "  -u, --uninstall        If this option is not specified, the default action is to install.\n"
         << "  -t, --type isetype     If this option is not specified, the default type is 0(keyboardISE).\n"
         << "  -h, --help             Show this help message.\n";
}

int main (int argc, char *argv[])
{
    char *isename = NULL;
    int isetype = 0;
    int uninstall = 0;

    perm_app_set_privilege ("isf", NULL, NULL);

    int i = 1;
    while (i < argc) {
        if (String ("-t") == String (argv[i]) || String ("--type") == String (argv[i])) {
            if (++i >= argc) {
                std::cerr << "No argument for option " << argv [i-1] << "\n";
                return -1;
            }
            isetype = atoi (argv[i]);
            i++;
            continue;
        }

        if (String ("-u") == String (argv[i]) || String ("--uninstall") == String (argv[i])) {
            i++;
            uninstall = 1;
            continue;
        }

        if (String ("-h") == String (argv[i]) || String ("--help") == String (argv[i])) {
            print_help ();
            return 0;
        }

        isename = argv[i];
        i++;
    }

    String sys_file_name  = String (SYS_ENGINE_FILE_NAME);
    String user_file_name = String (USER_ENGINE_FILE_NAME);
    String engine_file_name = sys_file_name;

    if (access (engine_file_name.c_str (), F_OK | W_OK) != 0) {
        FILE *filp = fopen (engine_file_name.c_str (), "a");
        if (filp == NULL) {
            engine_file_name = user_file_name;
            // Create folder for saving engine list
            scim_make_dir (USER_ENGINE_LIST_PATH);
        } else {
            fclose (filp);
        }
    }

    if (uninstall == 1) {
        isf_remove_ise_info_from_file (engine_file_name.c_str (), isename);
        return 0;
    }

    ConfigModule *_config_module = 0;
    ConfigPointer _config;

    _config_module = new ConfigModule ("simple");
    if (_config_module != NULL && _config_module->valid ())
        _config = _config_module->create_config ();
    if (_config.null ()) {
        std::cerr << "Config module cannot be loaded, using dummy Config.\n";

        if (_config_module) delete _config_module;
        _config_module = NULL;

        _config = new DummyConfig ();
    }

    char *lang_str = vconf_get_str (VCONFKEY_LANGSET);

    if (lang_str && strlen (lang_str)) {
        setenv ("LANG", lang_str, 1);
        setlocale (LC_MESSAGES, lang_str);
        free (lang_str);
    } else {
        setenv ("LANG", "en_US.utf8", 1);
        setlocale (LC_MESSAGES, "en_US.utf8");
    }

    if (argc == 1) {
        isf_update_ise_info_to_file (engine_file_name.c_str (), _config);
        return 0;
    }

    if (isetype == 0)
        isf_add_keyboard_info_to_file (engine_file_name.c_str (), isename, _config);
    else
        isf_add_helper_info_to_file (engine_file_name.c_str (), isename);

    return 0;
}

/*
vi:ts=4:nowrap:ai:expandtab
*/
