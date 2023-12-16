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
#define Uses_SCIM_IMENGINE
#define Uses_SCIM_IMENGINE_MODULE
#define Uses_SCIM_HELPER_MODULE
#define Uses_SCIM_CONFIG
#define Uses_SCIM_CONFIG_MODULE
#define Uses_SCIM_CONFIG_PATH
#define Uses_C_LOCALE
#define Uses_SCIM_UTILITY
#define Uses_SCIM_PANEL_AGENT


#include <sys/stat.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <dlog.h>
#include "scim_private.h"
#include "scim.h"
#include "isf_query_utility.h"


using namespace scim;


/////////////////////////////////////////////////////////////////////////////
// Declaration of macro.
/////////////////////////////////////////////////////////////////////////////
#ifdef LOG_TAG
# undef LOG_TAG
#endif
#define LOG_TAG                                         "ISF_QUERY"


/**
 * @brief Get normalized language name.
 *
 * @param src_str The language name before normalized.
 *
 * @return normalized language name.
 */
EAPI String isf_get_normalized_language (String src_str)
{
    if (src_str.length () == 0)
        return String ("en");

    std::vector<String> str_list, dst_list;
    scim_split_string_list (str_list, src_str);

    for (size_t i = 0; i < str_list.size (); i++)
        dst_list.push_back (scim_get_normalized_language (str_list[i]));

    String dst_str =  scim_combine_string_list (dst_list);

    return dst_str;
}

EAPI void isf_get_ise_info_from_string (const char *str, ISEINFO &info)
{
    if (str == NULL || strlen (str) == 0)
        return;

    char *buf = strdup (str);
    if (buf == NULL)
        return;

    int len = strlen (buf);
    for (int i = 0; i < len; i++) {
        if (buf[i] == ':' || buf[i] == '\n')
            buf[i] = '\0';
    }

    int j = 0;
    info.name = buf + j;
    j += strlen (buf + j) + 1;
    info.uuid = buf + j;
    j += strlen (buf + j) + 1;
    info.module = buf + j;
    j += strlen (buf + j) + 1;
    info.language = buf + j;
    j += strlen (buf + j) + 1;
    info.icon = buf + j;
    j += strlen (buf + j) + 1;
    info.mode = (TOOLBAR_MODE_T)atoi (buf + j);
    j += strlen (buf + j) + 1;
    info.option = (uint32)atoi (buf + j);
    j += strlen (buf + j) + 1;
    info.locales = buf + j;

    free (buf);
}

EAPI String isf_combine_ise_info_string (String name, String uuid, String module, String language,
                                    String icon, String mode, String option, String locales)
{
        String line = name + String (":") + uuid + String (":") + module + String (":") + language + String (":") +
                      icon + String (":") + mode + String (":") + option + String (":") + locales + String ("\n");
        return line;
}

EAPI bool isf_read_ise_info_list (const char *filename, std::vector<ISEINFO> &info_list)
{
    info_list.clear ();
    FILE *engine_list_file = fopen (filename, "r");
    if (engine_list_file == NULL) {
        std::cerr << "failed to open " << filename << "\n";
        return false;
    }

    char buf[MAXLINE];
    while (fgets (buf, MAXLINE, engine_list_file) != NULL && strlen (buf) > 0) {
        ISEINFO info;
        isf_get_ise_info_from_string (buf, info);

        info_list.push_back (info);
    }

    fclose (engine_list_file);
    return true;
}

EAPI bool isf_write_ise_info_list (const char *filename, std::vector<ISEINFO> &info_list)
{
    LOGD ("Enter");
    if (info_list.size () <= 0)
        return false;

    /* In order to avoid file writing failed, firstly save ISE information to temporary file */
    bool   bSuccess = true;
    String strTempFile = String (filename) + String (".tmp");

    FILE *engine_list_file = fopen (strTempFile.c_str (), "w+");
    if (engine_list_file == NULL) {
        LOGW ("Failed to open %s!!!\n", strTempFile.c_str ());
        return false;
    }

    std::vector<ISEINFO>::iterator iter;
    for (iter = info_list.begin (); iter != info_list.end (); iter++) {
        char mode[12];
        char option[12];
        snprintf (mode, sizeof (mode), "%d", (int)iter->mode);
        snprintf (option, sizeof (option), "%d", iter->option);

        String line = isf_combine_ise_info_string (iter->name, iter->uuid, iter->module, iter->language,
                                                   iter->icon, String (mode), String (option), iter->locales);
        if (fputs (line.c_str (), engine_list_file) < 0) {
            bSuccess = false;
            LOGW ("Failed to write (%s)!!!\n", line.c_str ());
            break;
        }
    }

    int ret = fclose (engine_list_file);
    if (ret != 0) {
        bSuccess = false;
        LOGW ("Failed to fclose %s!!!\n", strTempFile.c_str ());
    }

    if (bSuccess) {
        if (rename (strTempFile.c_str (), filename) != 0) {
            bSuccess = false;
            LOGW ("Failed to rename %s!!!\n", filename);
        }
    }

    LOGD ("Exit");
    return bSuccess;
}

static void add_keyboard_info_to_list (std::vector<ISEINFO> &info_list, const char *module_name, const ConfigPointer &config)
{
    if (module_name == NULL)
        return;

    IMEngineFactoryPointer factory;
    IMEngineModule         ime_module;
    ime_module.load (module_name, config);
    if (ime_module.valid ()) {
        for (size_t j = 0; j < ime_module.number_of_factories (); ++j) {
            try {
                factory = ime_module.create_factory (j);
            } catch (...) {
                factory.reset ();
            }

            if (!factory.null ()) {
                ISEINFO info;

                info.name = utf8_wcstombs (factory->get_name ());
                info.uuid = factory->get_uuid ();
                info.module = module_name;
                info.language = isf_get_normalized_language (factory->get_language ());
                info.icon = factory->get_icon_file ();
                info.mode = TOOLBAR_KEYBOARD_MODE;
                info.option = factory->get_option ();
                info.locales = factory->get_locales ();

                info_list.push_back (info);
                factory.reset ();
            }
        }
        ime_module.unload ();
    }
}

static void add_helper_info_to_list (std::vector<ISEINFO> &info_list, const char *module_name)
{
    if (module_name == NULL)
        return;

    HelperModule helper_module;
    HelperInfo   helper_info;
    helper_module.load (module_name);
    if (helper_module.valid ()) {
        for (size_t j = 0; j < helper_module.number_of_helpers (); ++j) {
            ISEINFO info;
            helper_module.get_helper_info (j, helper_info);
            info.name = helper_info.name;
            info.uuid = helper_info.uuid;
            info.module = module_name;
            info.language = isf_get_normalized_language (helper_module.get_helper_lang (j));
            info.icon = helper_info.icon;
            info.mode = TOOLBAR_HELPER_MODE;
            info.option = helper_info.option;
            info.locales = String ("");

            info_list.push_back (info);
        }
        helper_module.unload ();
    }
}

static void remove_ise_info_from_list (std::vector<ISEINFO> &info_list, const char *module_name)
{
    if (module_name == NULL)
        return;

    std::vector<ISEINFO>::iterator iter;
    while (info_list.size () > 0) {
        for (iter = info_list.begin (); iter != info_list.end (); iter++) {
            if (iter->module == module_name)
                break;
        }

        if (iter !=  info_list.end ())
            info_list.erase (iter);
        else
            break;
    }
}

static void remove_ise_info_from_list_by_uuid (std::vector<ISEINFO> &info_list, const char *uuid)
{
    if (uuid == NULL)
        return;

    std::vector<ISEINFO>::iterator iter;
    while (info_list.size () > 0) {
        for (iter = info_list.begin (); iter != info_list.end (); iter++) {
            if (iter->uuid == uuid)
                break;
        }

        if (iter !=  info_list.end ())
            info_list.erase (iter);
        else
            break;
    }
}

EAPI bool isf_add_keyboard_info_to_file (const char *filename, const char *module_name, const ConfigPointer &config)
{
    std::vector<ISEINFO> info_list;
    std::vector<ISEINFO>::iterator iter;
    isf_read_ise_info_list (filename, info_list);

    /* Firstly, remove the info of the specified modules from info_list */
    remove_ise_info_from_list (info_list, module_name);

    add_keyboard_info_to_list (info_list, module_name, config);

    return isf_write_ise_info_list (filename, info_list);
}

EAPI bool isf_add_helper_info_to_file (const char *filename, const char *module_name)
{
    std::vector<ISEINFO> info_list;
    std::vector<ISEINFO>::iterator iter;
    isf_read_ise_info_list (filename, info_list);

    /* Firstly, remove the info of the specified modules from info_list */
    remove_ise_info_from_list (info_list, module_name);

    add_helper_info_to_list (info_list, module_name);

    return isf_write_ise_info_list (filename, info_list);
}

EAPI bool isf_remove_ise_info_from_file (const char *filename, const char *module_name)
{
    std::vector<ISEINFO> info_list;
    std::vector<ISEINFO>::iterator iter;

    if (isf_read_ise_info_list (filename, info_list)) {

        remove_ise_info_from_list (info_list, module_name);

        return isf_write_ise_info_list (filename, info_list);
    }
    return false;
}

EAPI bool isf_remove_ise_info_from_file_by_uuid (const char *filename, const char *uuid)
{
    std::vector<ISEINFO> info_list;
    std::vector<ISEINFO>::iterator iter;

    if (isf_read_ise_info_list (filename, info_list)) {

        remove_ise_info_from_list_by_uuid (info_list, uuid);

        return isf_write_ise_info_list (filename, info_list);
    }
    return false;
}

EAPI void isf_update_ise_info_to_file (const char *filename, const ConfigPointer &config)
{
    if (filename == NULL)
        return;

    std::vector<ISEINFO> info_list;

    std::vector<String> imengine_list;
    scim_get_imengine_module_list (imengine_list);
    for (size_t i = 0; i < imengine_list.size (); ++i) {
        if (imengine_list[i] != String ("socket"))
            add_keyboard_info_to_list (info_list, imengine_list[i].c_str (), config);
    }

    std::vector<String> helper_list;
    scim_get_helper_module_list (helper_list);
    for (size_t i = 0; i < helper_list.size (); ++i) {
        add_helper_info_to_list (info_list, helper_list[i].c_str ());
    }

    isf_write_ise_info_list (filename, info_list);
}

/*
vi:ts=4:nowrap:ai:expandtab
*/
