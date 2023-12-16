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

#define Uses_SCIM_CONFIG_PATH
#define Uses_SCIM_PANEL_AGENT
#define Uses_SCIM_IMENGINE_MODULE
#define Uses_SCIM_HELPER_MODULE
#define Uses_SCIM_COMPOSE_KEY


#include <string.h>
#include "scim.h"
#include "scim_stl_map.h"
#include "isf_panel_efl.h"
#include "isf_panel_utility.h"
#include "isf_query_utility.h"
#include <sys/stat.h>
#include <unistd.h>
#include <dlog.h>
#include <errno.h>

/////////////////////////////////////////////////////////////////////////////
// Declaration of macro.
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// Declaration of global variables.
/////////////////////////////////////////////////////////////////////////////
MapStringVectorSizeT         _groups;
std::vector<String>          _uuids;
std::vector<String>          _names;
std::vector<String>          _module_names;
std::vector<String>          _langs;
std::vector<String>          _icons;
std::vector<uint32>          _options;
std::vector<TOOLBAR_MODE_T>  _modes;


/////////////////////////////////////////////////////////////////////////////
// Declaration of internal variables.
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Declaration of internal functions.
/////////////////////////////////////////////////////////////////////////////
static bool add_keyboard_ise_module (const String ise_name, const ConfigPointer &config);
static bool add_helper_ise_module   (const String ise_name);


/**
 * @brief Get all ISEs support languages.
 *
 * @param all_langs The list to store all languages.
 */
void isf_get_all_languages (std::vector<String> &all_langs)
{
    String lang_name;

    for (MapStringVectorSizeT::iterator it = _groups.begin (); it != _groups.end (); ++it) {
        lang_name = scim_get_language_name_english (it->first);
        all_langs.push_back (lang_name);
    }
}

/**
 * @brief Get all ISEs for the specific languages.
 *
 * @param lang_list The specific languages list.
 * @param uuid_list The list to store ISEs' UUID.
 * @param name_list The list to store ISEs' name.
 */
void isf_get_all_ises_in_languages (std::vector<String> lang_list, std::vector<String> &uuid_list, std::vector<String> &name_list)
{
    String lang_name;

    for (MapStringVectorSizeT::iterator it = _groups.begin (); it != _groups.end (); ++it) {
        lang_name = scim_get_language_name_english (it->first);
        if (std::find (lang_list.begin (), lang_list.end (), lang_name) != lang_list.end ()) {
            for (size_t i = 0; i < it->second.size (); i++) {
                // Avoid to add the same ISE
                if (std::find (uuid_list.begin (), uuid_list.end (), _uuids[it->second[i]]) == uuid_list.end ()) {
                    uuid_list.push_back (_uuids[it->second[i]]);
                    name_list.push_back (_names[it->second[i]]);
                }
            }
        }
    }
}

/**
 * @brief Get keyboard ISE
 *
 * @param config The config pointer for loading keyboard ISE.
 * @param ise_uuid The keyboard ISE uuid.
 * @param ise_name The keyboard ISE name.
 * @param ise_option The keyboard ISE option.
 */
void isf_get_keyboard_ise (const ConfigPointer &config, String &ise_uuid, String &ise_name, uint32 &ise_option)
{
    String uuid = config->read (String (SCIM_CONFIG_DEFAULT_IMENGINE_FACTORY) + String ("/") + String ("~other"), String (""));
    if (ise_uuid.length () > 0)
        uuid = ise_uuid;
    for (unsigned int i = 0; i < _uuids.size (); i++) {
        if (uuid == _uuids[i]) {
            ise_uuid   = _uuids[i];
            ise_name   = _names[i];
            ise_option = _options[i];
            return;
        }
    }
    ise_name = String ("");
    ise_uuid = String ("");
}

/**
 * @brief Get all keyboard ISEs for the specific languages.
 *
 * @param lang_list The specific languages list.
 * @param uuid_list The list to store keyboard ISEs' UUID.
 * @param name_list The list to store keyboard ISEs' name.
 * @param bCheckOption The flag to check whether support hardware keyboard.
 */
void isf_get_keyboard_ises_in_languages (const std::vector<String> &lang_list,
                                         std::vector<String>       &uuid_list,
                                         std::vector<String>       &name_list,
                                         bool                       bCheckOption)
{
    String lang_name;

    for (MapStringVectorSizeT::iterator it = _groups.begin (); it != _groups.end (); ++it) {
        lang_name = scim_get_language_name_english (it->first);

        if (std::find (lang_list.begin (), lang_list.end (), lang_name) != lang_list.end ()) {
            for (size_t i = 0; i < it->second.size (); i++) {
                if (_modes[it->second[i]] != TOOLBAR_KEYBOARD_MODE)
                    continue;
                if (bCheckOption && (_options[it->second[i]] & SCIM_IME_NOT_SUPPORT_HARDWARE_KEYBOARD))
                    continue;
                if (std::find (uuid_list.begin (), uuid_list.end (), _uuids[it->second[i]]) == uuid_list.end ()) {
                    uuid_list.push_back (_uuids[it->second[i]]);
                    name_list.push_back (_names[it->second[i]]);
                }
            }
        }
    }
}

/**
 * @brief Get all helper ISEs for the specific languages.
 *
 * @param lang_list The specific languages list.
 * @param uuid_list The list to store helper ISEs' UUID.
 * @param name_list The list to store helper ISEs' name.
 */
void isf_get_helper_ises_in_languages (const std::vector<String> &lang_list, std::vector<String> &uuid_list, std::vector<String> &name_list)
{
    String lang_name;

    for (MapStringVectorSizeT::iterator it = _groups.begin (); it != _groups.end (); ++it) {
        lang_name = scim_get_language_name_english (it->first);
        if (std::find (lang_list.begin (), lang_list.end (), lang_name) != lang_list.end ()) {
            for (size_t i = 0; i < it->second.size (); i++) {

                if (_modes[it->second[i]]!= TOOLBAR_HELPER_MODE)
                    continue;
                // Avoid to add the same ISE
                if (std::find (uuid_list.begin (), uuid_list.end (), _uuids[it->second[i]]) == uuid_list.end ()) {
                    uuid_list.push_back (_uuids[it->second[i]]);
                    name_list.push_back (_names[it->second[i]]);
                }
            }
        }
    }
}

/**
 * @brief Save all ISEs information into cache file.
 *
 * @return void
 */
void isf_save_ise_information (void)
{
    if (_module_names.size () <= 0)
        return;

    std::vector<ISEINFO> info_list;
    for (size_t i = 0; i < _module_names.size (); ++i) {
        ISEINFO info;
        info.name     = _names[i];
        info.uuid     = _uuids[i];
        info.module   = _module_names[i];
        info.language = _langs[i];
        info.icon     = _icons[i];
        info.mode     = _modes[i];
        info.option   = _options[i];
        info.locales  = "";
        info_list.push_back (info);
    }

    String user_file_name = String (USER_ENGINE_FILE_NAME);
    isf_write_ise_info_list (user_file_name.c_str (), info_list);
}

/**
 * @brief Load all ISEs to initialize.
 *
 * @param type The loading ISE type.
 * @param config The config pointer for loading keyboard ISE.
 * @param uuids The ISE uuid list.
 * @param names The ISE name list.
 * @param module_names The ISE module name list.
 * @param langs The ISE language list.
 * @param icons The ISE icon list.
 * @param modes The ISE type list.
 * @param options The ISE option list.
 */
void isf_get_factory_list (LOAD_ISE_TYPE  type,
                           const ConfigPointer &config,
                           std::vector<String> &uuids,
                           std::vector<String> &names,
                           std::vector<String> &module_names,
                           std::vector<String> &langs,
                           std::vector<String> &icons,
                           std::vector<TOOLBAR_MODE_T> &modes,
                           std::vector<uint32> &options)
{
    uuids.clear ();
    names.clear ();
    module_names.clear ();
    langs.clear ();
    icons.clear ();
    modes.clear ();
    options.clear ();
    _groups.clear ();

    String user_file_name = String (USER_ENGINE_FILE_NAME);
    FILE *engine_list_file = fopen (user_file_name.c_str (), "r");
    if (engine_list_file == NULL) {
        std::cerr << __func__ << " Failed to open(" << user_file_name << ")\n";
        return;
    }

    char buf[MAXLINE];
    while (fgets (buf, MAXLINE, engine_list_file) != NULL) {
        ISEINFO info;
        isf_get_ise_info_from_string (buf, info);
        if (info.mode == TOOLBAR_HELPER_MODE || type != HELPER_ONLY) {
            names.push_back (info.name);
            uuids.push_back (info.uuid);
            module_names.push_back (info.module);
            langs.push_back (info.language);
            icons.push_back (info.icon);
            modes.push_back (info.mode);
            options.push_back (info.option);
        }
    }

    fclose (engine_list_file);
}

/**
 * @brief Load all ISEs information and ISF default languages.
 *
 * @param type The load ISE type.
 * @param config The config pointer for loading keyboard ISE.
 */
void isf_load_ise_information (LOAD_ISE_TYPE type, const ConfigPointer &config)
{
    /* Load ISE engine info */
    isf_get_factory_list (type, config, _uuids, _names, _module_names, _langs, _icons, _modes, _options);

    /* Update _groups */
    std::vector<String> ise_langs;
    for (size_t i = 0; i < _uuids.size (); ++i) {
        scim_split_string_list (ise_langs, _langs[i]);
        for (size_t j = 0; j < ise_langs.size (); j++) {
            if (std::find (_groups[ise_langs[j]].begin (), _groups[ise_langs[j]].end (), i) == _groups[ise_langs[j]].end ())
                _groups[ise_langs[j]].push_back (i);
        }
        ise_langs.clear ();
    }
}

/**
 * @brief Load one keyboard ISE module.
 *
 * @param module_name The keboard ISE module name.
 * @param config The config pointer for loading keyboard ISE.
 *
 * @return true if load module is successful, otherwise return false.
 */
static bool add_keyboard_ise_module (const String module_name, const ConfigPointer &config)
{
    if (module_name.length () <= 0 || module_name == "socket")
        return false;

    IMEngineFactoryPointer factory;
    IMEngineModule         ime_module;

    String filename = String (USER_ENGINE_FILE_NAME);
    FILE *engine_list_file = fopen (filename.c_str (), "a");
    if (engine_list_file == NULL) {
        LOGW ("Failed to open %s!!!\n", filename.c_str ());
        return false;
    }

    ime_module.load (module_name, config);
    if (ime_module.valid ()) {
        for (size_t j = 0; j < ime_module.number_of_factories (); ++j) {
            try {
                factory = ime_module.create_factory (j);
            } catch (...) {
                factory.reset ();
            }

            if (!factory.null ()) {
                if (std::find (_uuids.begin (), _uuids.end (), factory->get_uuid ()) == _uuids.end ()) {
                    String uuid = factory->get_uuid ();
                    String name = utf8_wcstombs (factory->get_name ());
                    String language = isf_get_normalized_language (factory->get_language ());
                    String icon = factory->get_icon_file ();
                    char mode[12];
                    char option[12];

                    _uuids.push_back (uuid);
                    _names.push_back (name);
                    _module_names.push_back (module_name);
                    _langs.push_back (language);
                    _icons.push_back (icon);
                    _modes.push_back (TOOLBAR_KEYBOARD_MODE);
                    _options.push_back (factory->get_option ());

                    snprintf (mode, sizeof (mode), "%d", (int)TOOLBAR_KEYBOARD_MODE);
                    snprintf (option, sizeof (option), "%d", factory->get_option ());

                    String line = isf_combine_ise_info_string (name, uuid, module_name, language,
                                                               icon, String (mode), String (option), factory->get_locales ());
                    if (fputs (line.c_str (), engine_list_file) < 0) {
                        LOGW ("Failed to write (%s)!!!\n", line.c_str ());
                        break;
                    }
                }
                factory.reset ();
            }
        }
        ime_module.unload ();
    } else {
        LOGW ("Failed to load (%s)!!!", module_name.c_str ());
        fclose (engine_list_file);
        return false;
    }

    int ret = fclose (engine_list_file);
    if (ret != 0)
        LOGW ("Failed to fclose %s!!!\n", filename.c_str ());

    return true;
}

/**
 * @brief Load one helper ISE module.
 *
 * @param module_name The helper ISE module name.
 *
 * @return true if load module is successful, otherwise return false.
 */
static bool add_helper_ise_module (const String module_name)
{
    if (module_name.length () <= 0)
        return false;

    HelperModule helper_module;
    HelperInfo   helper_info;

    String filename = String (USER_ENGINE_FILE_NAME);
    FILE *engine_list_file = fopen (filename.c_str (), "a");
    if (engine_list_file == NULL) {
        LOGW ("Failed to open %s!!!\n", filename.c_str ());
        return false;
    }

    helper_module.load (module_name);
    if (helper_module.valid ()) {
        for (size_t j = 0; j < helper_module.number_of_helpers (); ++j) {
            helper_module.get_helper_info (j, helper_info);
            _uuids.push_back (helper_info.uuid);
            _names.push_back (helper_info.name);
            _langs.push_back (isf_get_normalized_language (helper_module.get_helper_lang (j)));
            _module_names.push_back (module_name);
            _icons.push_back (helper_info.icon);
            _modes.push_back (TOOLBAR_HELPER_MODE);
            _options.push_back (helper_info.option);

            char mode[12];
            char option[12];
            snprintf (mode, sizeof (mode), "%d", (int)TOOLBAR_HELPER_MODE);
            snprintf (option, sizeof (option), "%d", helper_info.option);

            String line = isf_combine_ise_info_string (helper_info.name, helper_info.uuid, module_name, isf_get_normalized_language (helper_module.get_helper_lang (j)),
                                                       helper_info.icon, String (mode), String (option), String (""));
            if (fputs (line.c_str (), engine_list_file) < 0) {
                 LOGW ("Failed to write (%s)!!!\n", line.c_str ());
                 break;
            }
        }
        helper_module.unload ();
    } else {
        LOGW ("Failed to load (%s)!!!", module_name.c_str ());
        fclose (engine_list_file);
        return false;
    }

    int ret = fclose (engine_list_file);
    if (ret != 0)
        LOGW ("Failed to fclose %s!!!\n", filename.c_str ());

    return true;
}

/**
 * @brief Update ISEs information for ISE is added or removed.
 *
 * @param type The load ISE type.
 * @param config The config pointer for loading keyboard ISE.
 *
 * @return true if ISEs list is changed, otherwise return false.
 */
bool isf_update_ise_list (LOAD_ISE_TYPE type, const ConfigPointer &config)
{
    bool ret = false;

    std::vector<String> helper_list;
    scim_get_helper_module_list (helper_list);

    std::vector<String> install_modules, uninstall_modules;

    /* Check keyboard ISEs */
    if (type != HELPER_ONLY) {
        std::vector<String> imengine_list;
        scim_get_imengine_module_list (imengine_list);
        for (size_t i = 0; i < imengine_list.size (); ++i) {
            install_modules.push_back (imengine_list [i]);
            if (std::find (_module_names.begin (), _module_names.end (), imengine_list [i]) == _module_names.end ()) {
                if (add_keyboard_ise_module (imengine_list [i], config))
                    ret = true;
            }
        }
    }

    /* Check helper ISEs */
    for (size_t i = 0; i < helper_list.size (); ++i) {
        install_modules.push_back (helper_list [i]);
        if (std::find (_module_names.begin (), _module_names.end (), helper_list [i]) == _module_names.end ()) {
            if (add_helper_ise_module (helper_list [i]))
                ret = true;
        }
    }

    /* Try to find uninstall ISEs */
    bool bFindUninstall = false;
    for (size_t i = 0; i < _module_names.size (); ++i) {
        if (std::find (install_modules.begin (), install_modules.end (), _module_names [i]) == install_modules.end ()) {
            ret = true;
            bFindUninstall = true;
            /* Avoid to add the same module */
            if (std::find (uninstall_modules.begin (), uninstall_modules.end (), _module_names [i]) == uninstall_modules.end ()) {
                uninstall_modules.push_back (_module_names [i]);
                String filename = String (USER_ENGINE_FILE_NAME);
                if (isf_remove_ise_info_from_file (filename.c_str (), _module_names [i].c_str ()) == false)
                    LOGW ("Failed to remove %s from cache file : %s!!!", _module_names [i].c_str (), filename.c_str ());
            }
        }
    }
    if (bFindUninstall) {
        std::vector<String>          tmp_uuids        = _uuids;
        std::vector<String>          tmp_names        = _names;
        std::vector<String>          tmp_module_names = _module_names;
        std::vector<String>          tmp_langs        = _langs;
        std::vector<String>          tmp_icons        = _icons;
        std::vector<uint32>          tmp_options      = _options;
        std::vector<TOOLBAR_MODE_T>  tmp_modes        = _modes;

        _uuids.clear ();
        _names.clear ();
        _module_names.clear ();
        _langs.clear ();
        _icons.clear ();
        _options.clear ();
        _modes.clear ();
        _groups.clear ();

        for (size_t i = 0; i < tmp_module_names.size (); ++i) {
            if (std::find (uninstall_modules.begin (), uninstall_modules.end (), tmp_module_names [i]) == uninstall_modules.end ()) {
                _uuids.push_back (tmp_uuids [i]);
                _names.push_back (tmp_names [i]);
                _module_names.push_back (tmp_module_names [i]);
                _langs.push_back (tmp_langs [i]);
                _icons.push_back (tmp_icons [i]);
                _options.push_back (tmp_options [i]);
                _modes.push_back (tmp_modes [i]);
            }
        }
    }

    /* Update _groups */
    if (ret) {
        std::vector<String> ise_langs;
        for (size_t i = 0; i < _uuids.size (); ++i) {
            scim_split_string_list (ise_langs, _langs[i]);
            for (size_t j = 0; j < ise_langs.size (); j++) {
                if (std::find (_groups[ise_langs[j]].begin (), _groups[ise_langs[j]].end (), i) == _groups[ise_langs[j]].end ())
                    _groups[ise_langs[j]].push_back (i);
            }
            ise_langs.clear ();
        }
    }

    return ret;
}

bool isf_remove_ise_module (const String module_name, const ConfigPointer &config)
{
    if (std::find (_module_names.begin (), _module_names.end (), module_name) == _module_names.end ()) {
        LOGW ("Cannot to find %s!!!", module_name.c_str ());
        return true;
    }

    String filename = String (USER_ENGINE_FILE_NAME);
    if (isf_remove_ise_info_from_file (filename.c_str (), module_name.c_str ())) {
        isf_get_factory_list (ALL_ISE, config, _uuids, _names, _module_names, _langs, _icons, _modes, _options);

        /* Update _groups */
        _groups.clear ();
        std::vector<String> ise_langs;
        for (size_t i = 0; i < _uuids.size (); ++i) {
            scim_split_string_list (ise_langs, _langs[i]);
            for (size_t j = 0; j < ise_langs.size (); j++) {
                if (std::find (_groups[ise_langs[j]].begin (), _groups[ise_langs[j]].end (), i) == _groups[ise_langs[j]].end ())
                    _groups[ise_langs[j]].push_back (i);
            }
            ise_langs.clear ();
        }
        return true;
    } else {
        LOGW ("Failed to remove %s from cache file : %s!!!", module_name.c_str (), filename.c_str ());
        return false;
    }
}

bool isf_update_ise_module (const String strModulePath, const ConfigPointer &config)
{
    bool ret = false;
    struct stat filestat;
    if (stat (strModulePath.c_str (), &filestat) == -1) {
        char buf_err[256];
        LOGW ("can't access : %s, reason : %s\n", strModulePath.c_str (), strerror_r (errno, buf_err, sizeof (buf_err)));
        return false;
    }

    if (!S_ISDIR (filestat.st_mode)) {
        int begin = strModulePath.find_last_of (SCIM_PATH_DELIM) + 1;
        String mod_name = strModulePath.substr (begin, strModulePath.find_last_of ('.') - begin);
        String path     = strModulePath.substr (0, strModulePath.find_last_of (SCIM_PATH_DELIM));
        LOGD ("module_name = %s, path = %s", mod_name.c_str (), path.c_str ());

        if (mod_name.length () > 0 && path.length () > 1) {
            if (isf_remove_ise_module (mod_name, config)) {
                String type = path.substr (path.find_last_of (SCIM_PATH_DELIM) + 1);
                LOGD ("type = %s", type.c_str ());
                if (type == String ("Helper")) {
                    if (add_helper_ise_module (mod_name))
                        ret = true;
                } else if (type == String ("IMEngine")) {
                    if (add_keyboard_ise_module (mod_name, config))
                        ret = true;
                }
            }
        } else {
            LOGW ("%s is not valid so file!!!", strModulePath.c_str ());
        }
    }

    /* Update _groups */
    if (ret) {
        _groups.clear ();
        std::vector<String> ise_langs;
        for (size_t i = 0; i < _uuids.size (); ++i) {
            scim_split_string_list (ise_langs, _langs[i]);
            for (size_t j = 0; j < ise_langs.size (); j++) {
                if (std::find (_groups[ise_langs[j]].begin (), _groups[ise_langs[j]].end (), i) == _groups[ise_langs[j]].end ())
                    _groups[ise_langs[j]].push_back (i);
            }
            ise_langs.clear ();
        }
    }

    return ret;
}

/**
 * @brief Get normalized language name.
 *
 * @param src_str The language name before normalized.
 *
 * @return normalized language name.
 */
String isf_get_normalized_language (String src_str)
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

bool isf_add_helper_ise (HelperInfo helper_info, String module_name)
{
    String filename = String (USER_ENGINE_FILE_NAME);
    FILE *engine_list_file = fopen (filename.c_str (), "a");
    if (engine_list_file == NULL) {
        LOGW ("Failed to open %s!!!\n", filename.c_str ());
        return false;
    }

    _uuids.push_back (helper_info.uuid);
    _names.push_back (helper_info.name);
    _langs.push_back (isf_get_normalized_language ("en_US")); // FIXME
    _module_names.push_back (module_name);
    _icons.push_back (helper_info.icon);
    _modes.push_back (TOOLBAR_HELPER_MODE);
    _options.push_back (helper_info.option);

    char mode[12];
    char option[12];
    snprintf (mode, sizeof (mode), "%d", (int)TOOLBAR_HELPER_MODE);
    snprintf (option, sizeof (option), "%d", helper_info.option);

    String line = isf_combine_ise_info_string (helper_info.name, helper_info.uuid, module_name, isf_get_normalized_language ("en_US"),
                                               helper_info.icon, String (mode), String (option), String (""));
    if (fputs (line.c_str (), engine_list_file) < 0) {
         LOGW ("Failed to write (%s)!!!\n", line.c_str ());
    }

    if (fclose (engine_list_file) != 0)
        LOGW ("Failed to fclose %s!!!\n", filename.c_str ());

    return true;
}

bool isf_remove_helper_ise (const char *uuid, const ConfigPointer &config)
{
    String filename = String (USER_ENGINE_FILE_NAME);
    if (isf_remove_ise_info_from_file_by_uuid (filename.c_str (), uuid)) {
        isf_get_factory_list (ALL_ISE, config, _uuids, _names, _module_names, _langs, _icons, _modes, _options);

        /* Update _groups */
        _groups.clear ();
        std::vector<String> ise_langs;
        for (size_t i = 0; i < _uuids.size (); ++i) {
            scim_split_string_list (ise_langs, _langs[i]);
            for (size_t j = 0; j < ise_langs.size (); j++) {
                if (std::find (_groups[ise_langs[j]].begin (), _groups[ise_langs[j]].end (), i) == _groups[ise_langs[j]].end ())
                    _groups[ise_langs[j]].push_back (i);
            }
            ise_langs.clear ();
        }
        return true;
    } else {
        LOGW ("Failed to remove uuid : %s from cache file : %s!!!", uuid, filename.c_str ());
        return false;
    }
}


/*
vi:ts=4:nowrap:ai:expandtab
*/
