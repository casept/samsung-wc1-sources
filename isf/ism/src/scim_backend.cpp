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
 * 1. Create and use ISE cache file
 * 2. Dynamic load keyboard ISE
 *
 * $Id: scim_backend.cpp,v 1.38.2.1 2006/09/24 16:00:52 suzhe Exp $
 *
 */

#define Uses_SCIM_FILTER_MANAGER
#define Uses_SCIM_BACKEND
#define Uses_SCIM_IMENGINE
#define Uses_SCIM_IMENGINE_MODULE
#define Uses_SCIM_CONFIG_PATH
#define Uses_STL_ALGORITHM
#define Uses_SCIM_PANEL_AGENT


#include <malloc.h>
#include <string.h>
#include <unistd.h>
#include <dlog.h>
#include "scim_private.h"
#include "scim.h"
#include "scim_stl_map.h"
#include "isf_query_utility.h"


#ifdef LOG_TAG
# undef LOG_TAG
#endif
#define LOG_TAG                                         "ISF_BACKEND"


namespace scim {

#if SCIM_USE_STL_EXT_HASH_MAP
typedef __gnu_cxx::hash_map <String, IMEngineFactoryPointer, scim_hash_string >     IMEngineFactoryRepository;
#elif SCIM_USE_STL_HASH_MAP
typedef std::hash_map <String, IMEngineFactoryPointer, scim_hash_string >           IMEngineFactoryRepository;
#else
typedef std::map <String, IMEngineFactoryPointer>                                   IMEngineFactoryRepository;
#endif

typedef std::vector <IMEngineFactoryPointer>                                        IMEngineFactoryPointerVector;


class LocaleEqual
{
    String m_lhs;
public:
    LocaleEqual (const String &lhs) : m_lhs (lhs) { }

    bool operator () (const String &rhs) const {
        if (m_lhs == rhs) return true;
        if (scim_get_locale_language (m_lhs) == scim_get_locale_language (rhs) &&
            scim_get_locale_encoding (m_lhs) == scim_get_locale_encoding (rhs) &&
            m_lhs.find ('.') != String::npos && rhs.find ('.') != String::npos)
            return true;
        return false;
    }
};

class IMEngineFactoryPointerLess
{
public:
    bool operator () (const IMEngineFactoryPointer &lhs, const IMEngineFactoryPointer &rhs) const {
        return (lhs->get_language () < rhs->get_language ()) ||
               (lhs->get_language () == rhs->get_language () && lhs->get_name () < rhs->get_name ());
    }
};

class BackEndBase::BackEndBaseImpl
{
    IMEngineFactoryRepository    m_factory_repository;
    String                       m_supported_unicode_locales;
    ConfigPointer                m_config;

public:
    BackEndBaseImpl (const ConfigPointer &config)
        : m_config (config)
    {
        String locales;

        /* Set the default supported locales. */
        locales = scim_global_config_read (SCIM_GLOBAL_CONFIG_SUPPORTED_UNICODE_LOCALES, String ("en_US.UTF-8"));

        std::vector <String> locale_list;
        std::vector <String> real_list;

        scim_split_string_list (locale_list, locales);

        for (std::vector <String>::iterator i = locale_list.begin (); i!= locale_list.end (); ++i) {
            *i = scim_validate_locale (*i);
            if (i->length () && scim_get_locale_encoding (*i) == "UTF-8" &&
                std::find_if (real_list.begin (), real_list.end (), LocaleEqual (*i)) == real_list.end ())
                real_list.push_back (*i);
        }

        m_supported_unicode_locales = scim_combine_string_list (real_list);
    }

    void clear ()
    {
        m_factory_repository.clear ();
    }

    void dump_factories ()
    {
        IMEngineFactoryRepository::const_iterator it;
        std::cout << "Factories in backend:" << std::endl;
        for (it = m_factory_repository.begin (); it != m_factory_repository.end (); ++it) {
            if (it->second.null ())
                std::cout << "\t" << it->first << ": null" << std::endl;
            else
                std::cout << "\t" << it->first << ": valid" << std::endl;
        }
    }

    String get_all_locales () const
    {
        String locale;

        std::vector <String> locale_list;
        std::vector <String> real_list;

        IMEngineFactoryRepository::const_iterator it;

        for (it = m_factory_repository.begin (); it != m_factory_repository.end (); ++it) {
            if (locale.length () == 0)
                locale += it->second->get_locales ();
            else
                locale += (String (",") + it->second->get_locales ());
        }

        if (m_supported_unicode_locales.length ())
            locale += (String (",") + m_supported_unicode_locales);

        scim_split_string_list (locale_list, locale);

        for (std::vector <String>::iterator i = locale_list.begin (); i!= locale_list.end (); i++) {
            locale = scim_validate_locale (*i);
            if (locale.length () &&
                std::find_if (real_list.begin (), real_list.end (), LocaleEqual (locale)) == real_list.end ())
                real_list.push_back (locale);
        }

        return scim_combine_string_list (real_list);
    }

    IMEngineFactoryPointer get_factory (const String &uuid) const
    {
        IMEngineFactoryRepository::const_iterator it = m_factory_repository.find (uuid);

        if (it != m_factory_repository.end ())
            return it->second;

        return IMEngineFactoryPointer (0);
    }

    uint32 get_factories_for_encoding (std::vector<IMEngineFactoryPointer> &factories,
                                       const String                        &encoding)  const
    {
        IMEngineFactoryRepository::const_iterator it;

        factories.clear ();

        for (it = m_factory_repository.begin (); it != m_factory_repository.end (); ++it) {
            if ((encoding.length () == 0 || it->second->validate_encoding (encoding)))
                factories.push_back (it->second);
        }

        sort_factories (factories);

        return factories.size ();
    }

    uint32 get_factories_for_language (std::vector<IMEngineFactoryPointer> &factories,
                                       const String                        &language)  const
    {
        IMEngineFactoryRepository::const_iterator it;

        factories.clear ();

        for (it = m_factory_repository.begin (); it != m_factory_repository.end (); ++it) {
            if ((language.length () == 0 || it->second->get_language () == language))
                factories.push_back (it->second);
        }

        sort_factories (factories);

        return factories.size ();
    }

    uint32 get_factory_list (std::vector<String> &uuids) const
    {
        IMEngineFactoryRepository::const_iterator it;
        for (it = m_factory_repository.begin (); it != m_factory_repository.end (); ++it)
            uuids.push_back(it->first);

        return m_factory_repository.size ();
    }

    IMEngineFactoryPointer get_default_factory (const String &language, const String &encoding) const
    {
        if (!language.length ()) return IMEngineFactoryPointer ();

        IMEngineFactoryPointerVector factories;

        if (get_factories_for_encoding (factories, encoding) > 0) {
            IMEngineFactoryPointer lang_first;
            IMEngineFactoryPointerVector::iterator it;

            String def_uuid;

            def_uuid = m_config->read (String (SCIM_CONFIG_DEFAULT_IMENGINE_FACTORY) +
                                       String ("/") + String ("~other"),
                                       String (""));

            /* Match by Normalized language exactly. */
            for (it = factories.begin (); it != factories.end (); ++it) {
                if (scim_get_normalized_language ((*it)->get_language ()) == language && lang_first.null ())
                    lang_first = *it;

                if ((*it)->get_uuid () == def_uuid)
                    return *it;
            }

            if (!lang_first.null ())
                return lang_first;

            /* Match by short language name. */
            for (it = factories.begin (); it != factories.end (); ++it)
                if ((*it)->get_language () == language.substr (0,2))
                    return *it;

            return factories [0];
        }

        return IMEngineFactoryPointer ();
    }

    void set_default_factory (const String &language, const String &uuid)
    {
        if (!language.length () || !uuid.length ()) return;

        IMEngineFactoryPointerVector factories;
        if (get_factories_for_encoding (factories, "") > 0) {
            IMEngineFactoryPointerVector::iterator it;
            for (it = factories.begin (); it != factories.end (); ++it) {
                if ((*it)->get_uuid () == uuid) {
                    m_config->write (String (SCIM_CONFIG_DEFAULT_IMENGINE_FACTORY) + String ("/") + String ("~other"), uuid);
                    m_config->flush ();
                    m_config->reload ();
                    return;
                }
            }
        }
    }

    IMEngineFactoryPointer get_next_factory (const String &language, const String &encoding, const String &cur_uuid) const
    {
        IMEngineFactoryPointerVector factories;

        if (get_factories_for_encoding (factories, encoding) > 0) {
            IMEngineFactoryPointer lang_first;
            IMEngineFactoryPointerVector::iterator it, itl;

            for (it = factories.begin (); it != factories.end (); ++it) {
                uint32 option = (*it)->get_option ();
                if ((language.length () == 0 || (*it)->get_language () == language) && lang_first.null () &&
                    !(option & SCIM_IME_NOT_SUPPORT_HARDWARE_KEYBOARD))
                    lang_first = *it;

                if ((*it)->get_uuid () == cur_uuid) {
                    for (itl = it + 1; itl != factories.end (); ++itl) {
                        option = (*itl)->get_option ();
                        if ((language.length () == 0 || (*itl)->get_language () == language) &&
                            !(option & SCIM_IME_NOT_SUPPORT_HARDWARE_KEYBOARD))
                            return *itl;
                    }
                    if (!lang_first.null ()) return lang_first;

                    return factories [0];
                }
            }
        }

        return IMEngineFactoryPointer ();
    }

    IMEngineFactoryPointer get_previous_factory (const String &language, const String &encoding, const String &cur_uuid) const
    {
        IMEngineFactoryPointerVector factories;

        if (get_factories_for_encoding (factories, encoding) > 0) {
            IMEngineFactoryPointer lang_last;
            IMEngineFactoryPointerVector::iterator it, itl;

            uint32 option = 0;
            for (it = factories.begin (); it != factories.end (); ++it) {
                option = (*it)->get_option ();
                if ((language.length () == 0 || (*it)->get_language () == language) &&
                    !(option & SCIM_IME_NOT_SUPPORT_HARDWARE_KEYBOARD))
                    lang_last = *it;
            }

            for (it = factories.begin (); it != factories.end (); ++it) {
                if ((*it)->get_uuid () == cur_uuid) {
                    for (itl = it; itl != factories.begin (); --itl) {
                        option = (*(itl-1))->get_option ();
                        if ((language.length () == 0 || (*(itl-1))->get_language () == language) &&
                            !(option & SCIM_IME_NOT_SUPPORT_HARDWARE_KEYBOARD))
                            return *(itl-1);
                    }

                    if (!lang_last.null ()) return lang_last;

                    return factories [factories.size () - 1];
                }
            }
        }

        return IMEngineFactoryPointer ();
    }

    bool add_factory (const IMEngineFactoryPointer &factory)
    {
        if (!factory.null ()) {
            String uuid = factory->get_uuid ();

            if (uuid.length ()) {
                m_factory_repository [uuid] = factory;
                return true;
            }
        }

        return false;
    }

    void add_specific_factory (const String &uuid, const IMEngineFactoryPointer &factory)
    {
        m_factory_repository [uuid] = factory;
        return;
    }

private:
    void sort_factories (std::vector<IMEngineFactoryPointer> &factories) const
    {
        std::sort (factories.begin (), factories.end (), IMEngineFactoryPointerLess ());
    }
};

BackEndBase::BackEndBase (const ConfigPointer &config)
    : m_impl (new BackEndBase::BackEndBaseImpl (config))
{
}

BackEndBase::~BackEndBase ()
{
    delete m_impl;
}

String
BackEndBase::get_all_locales () const
{
    return m_impl->get_all_locales ();
}

IMEngineFactoryPointer
BackEndBase::get_factory (const String &uuid) const
{
    return m_impl->get_factory (uuid);
}

uint32
BackEndBase::get_factories_for_encoding (std::vector<IMEngineFactoryPointer> &factories, const String &encoding) const
{
    return m_impl->get_factories_for_encoding (factories, encoding);
}

uint32
BackEndBase::get_factories_for_language (std::vector<IMEngineFactoryPointer> &factories, const String &language) const
{
    return m_impl->get_factories_for_language (factories, language);
}

uint32
BackEndBase::get_factory_list (std::vector<String> &uuids) const
{
    return m_impl->get_factory_list (uuids);
}

IMEngineFactoryPointer
BackEndBase::get_default_factory (const String &language, const String &encoding) const
{
    return m_impl->get_default_factory (language, encoding);
}

void
BackEndBase::set_default_factory (const String &language, const String &uuid)
{
    m_impl->set_default_factory (language, uuid);
}

IMEngineFactoryPointer
BackEndBase::get_next_factory (const String &language, const String &encoding, const String &cur_uuid) const
{
    return m_impl->get_next_factory (language, encoding, cur_uuid);
}

IMEngineFactoryPointer
BackEndBase::get_previous_factory (const String &language, const String &encoding, const String &cur_uuid) const
{
    return m_impl->get_previous_factory (language, encoding, cur_uuid);
}

bool
BackEndBase::add_factory (const IMEngineFactoryPointer &factory)
{
    return m_impl->add_factory (factory);
}

void
BackEndBase::add_specific_factory (const String &uuid,
                    const IMEngineFactoryPointer &factory)
{
    m_impl->add_specific_factory (uuid, factory);
    return;
}

void
BackEndBase::clear ()
{
    m_impl->clear ();
}

void
BackEndBase::dump_factories ()
{
    m_impl->dump_factories ();
}

/* Implementation of CommonBackEnd. */
struct CommonBackEnd::CommonBackEndImpl {
    std::vector<IMEngineModule *> m_engine_modules;
    FilterManager       *m_filter_manager;
    std::vector<String>  m_disabled_factories;
    std::vector<String>  m_modules;
    std::map <String, String> m_factory_module_repository;

    CommonBackEndImpl () : m_filter_manager (0) { }
};

CommonBackEnd::CommonBackEnd (const ConfigPointer       &config,
                              const std::vector<String> &modules)
    : BackEndBase (config),
      m_impl (new CommonBackEndImpl)
{
}

CommonBackEnd::~CommonBackEnd ()
{
    clear ();

    for (unsigned int i = 0; i < m_impl->m_engine_modules.size (); i++) {
        if (m_impl->m_engine_modules [i]) {
            m_impl->m_engine_modules [i]->unload ();
            delete m_impl->m_engine_modules [i];
            m_impl->m_engine_modules [i] = NULL;
        }
    }
    delete m_impl->m_filter_manager;
    delete m_impl;
}

void
CommonBackEnd::initialize (const ConfigPointer       &config,
                           const std::vector<String> &modules,
                           const bool                is_load_resource,
                           const bool                is_load_info)
{
    SCIM_DEBUG_BACKEND (1) << __FUNCTION__ << "...\n";

    std::vector<String>    new_modules = modules;
    IMEngineFactoryPointer factory;
    int all_factories_count = 0;
    int module_factories_count = 0;

    if (config.null ()) return;

    /* Get disabled factories list. */
    m_impl->m_disabled_factories = scim_global_config_read (SCIM_GLOBAL_CONFIG_DISABLED_IMENGINE_FACTORIES, m_impl->m_disabled_factories);

    /* Put socket module to the end of list. */
    if (new_modules.size () > 1) {
        for (std::vector<String>::iterator it = new_modules.begin (); it != new_modules.end (); ++it) {
            if (*it == "socket") {
                new_modules.erase (it);
                new_modules.push_back ("socket");
                break;
            }
        }
    }

    /* Try to load all IMEngine modules */
    try {
        m_impl->m_filter_manager = new FilterManager (config);
    } catch (const std::exception & err) {
        std::cerr << err.what () << "\n";
        return;
    }

    if (is_load_info) {
        /*for (size_t i = 0; i < new_modules.size (); ++i)
            add_module_info (config, new_modules [i]);*/
        add_module_info_from_cache_file (config, new_modules);
        return;
    }

    /* load IMEngine modules */
    for (size_t i = 0; i < new_modules.size (); ++i) {
        module_factories_count = add_module (config, new_modules [i], is_load_resource);
        all_factories_count += module_factories_count;
    }

    factory = new ComposeKeyFactory ();
    if (all_factories_count == 0 ||
        std::find (m_impl->m_disabled_factories.begin (),
                    m_impl->m_disabled_factories.end (),
                    factory->get_uuid ()) == m_impl->m_disabled_factories.end ()) {
        factory = m_impl->m_filter_manager->attach_filters_to_factory (factory);
        add_factory (factory);
    }
}

int
CommonBackEnd::add_module (const ConfigPointer &config,
                           const String         module,
                           bool                 is_load_resource)
{
    SCIM_DEBUG_BACKEND (1) << __FUNCTION__ << " : " << module << "\n";

    IMEngineFactoryPointer  factory;
    IMEngineModule         *engine_module = NULL;
    int  module_factories_count = 0;
    bool first_load = false;

    if (config.null ()) return 0;

    if (module.length () > 0) {
        /* Check if the module is already loaded */
        if (std::find (m_impl->m_modules.begin (), m_impl->m_modules.end (), module) == m_impl->m_modules.end ()) {
            engine_module = new IMEngineModule;
            m_impl->m_engine_modules.push_back (engine_module);
            m_impl->m_modules.push_back (module);
            engine_module->load (module, config);
            first_load = true;
        } else {
            for (size_t i = 0; i < m_impl->m_modules.size (); i++) {
                if (m_impl->m_modules [i] == module) {
                    engine_module = m_impl->m_engine_modules [i];
                    break;
                }
            }
        }

        if (engine_module && engine_module->valid ()) {
            size_t number = engine_module->number_of_factories ();
            for (size_t j = 0; j < number; ++j) {

                /* Try to load a IMEngine Factory. */
                try {
                    factory = engine_module->create_factory (j);
                } catch (const std::exception & err) {
                    std::cerr << err.what () << "\n";
                    factory.reset ();
                }

                if (!factory.null ()) {
                    /* Check if it's disabled. */
                    if (std::find (m_impl->m_disabled_factories.begin (),
                                   m_impl->m_disabled_factories.end (),
                                   factory->get_uuid ()) == m_impl->m_disabled_factories.end ()) {

                        /* Add it into disabled list to prevent from loading again. */
                        /*m_impl->m_disabled_factories.push_back (factory->get_uuid ());*/

                        if (is_load_resource)
                            factory->load_resource ();

                        /* Only load filter for none socket IMEngines. */
                        if (module != "socket")
                            factory = m_impl->m_filter_manager->attach_filters_to_factory (factory);

                        add_factory (factory);

                        module_factories_count ++;

                        SCIM_DEBUG_BACKEND (1) << "    Loading IMEngine Factory " << j << " : " << "OK\n";
                    } else {
                        SCIM_DEBUG_BACKEND (1) << "    Loading IMEngine Factory " << j << " : " << "Disabled\n";
                        factory.reset ();
                    }
                } else {
                    std::cerr << "    Loading IMEngine Factory " << module << "(" << j << ") is Failed!!!\n";
                }
            }
            if (module_factories_count) {
                SCIM_DEBUG_BACKEND (1) << module << " IMEngine module is successfully loaded.\n";
            } else {
                std::cerr << "No Factory loaded from " << module << " IMEngine module!!!\n";
                if (first_load)
                    engine_module->unload ();
            }
        } else {
            std::cerr << __func__ << ": Failed to load " << module << " IMEngine module!!!\n";
        }
    }

    return module_factories_count;
}

void
CommonBackEnd::add_module_info (const ConfigPointer &config, const String module_name)
{
    SCIM_DEBUG_BACKEND (1) << __FUNCTION__ << " : " << module_name << "\n";

    if (module_name.length () <= 0 || module_name == "socket")
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
                if (m_impl->m_factory_module_repository.find (factory->get_uuid ())
                        == m_impl->m_factory_module_repository.end ()) {
                    add_specific_factory (factory->get_uuid (), IMEngineFactoryPointer (0));
                    m_impl->m_factory_module_repository[factory->get_uuid ()] = module_name;
                }

                factory.reset ();
            }
        }
    } else {
        std::cerr << __func__ << ": Failed to load " << module_name << " IMEngine module!!!\n";
    }

    ime_module.unload ();
}

void
CommonBackEnd::add_factory_by_uuid (const ConfigPointer &config, const String uuid)
{
    SCIM_DEBUG_BACKEND (1) << __FUNCTION__ << " : " << uuid << "\n";

    if (m_impl->m_factory_module_repository.find (uuid) == m_impl->m_factory_module_repository.end ())
        return;

    String module = m_impl->m_factory_module_repository[uuid];
    add_module (config, module, true);
}

void
CommonBackEnd::release_module (const std::vector<String> &use_uuids, const String del_uuid)
{
    SCIM_DEBUG_BACKEND (1) << __FUNCTION__ << " : " << del_uuid << "\n";

    if (m_impl->m_factory_module_repository.find (del_uuid) == m_impl->m_factory_module_repository.end ())
        return;

    String module = m_impl->m_factory_module_repository[del_uuid];
    std::vector<String> uuids;
    std::map <String, String>::iterator it = m_impl->m_factory_module_repository.begin ();

    uuids.clear ();
    for (; it != m_impl->m_factory_module_repository.end (); it++) {
        if (it->second == module)
            uuids.push_back (it->first);
    }

    for (unsigned int i = 0; i < uuids.size (); i++) {
        for (unsigned int j = 0; j < use_uuids.size (); j++) {
            if (uuids[i] == use_uuids[j])
                return;
        }
    }

    /* unload the module */
    std::vector<String>::iterator it2 = uuids.begin ();
    for (; it2 != uuids.end (); it2++)
        add_specific_factory (*it2, IMEngineFactoryPointer (0));

    std::vector<String>::iterator it3 = m_impl->m_modules.begin ();
    for (; it3 != m_impl->m_modules.end (); it3++) {
        if (*it3 == module) {
            m_impl->m_modules.erase (it3);
            break;
        }
    }

    std::vector<IMEngineModule *>::iterator it4 = m_impl->m_engine_modules.begin ();
    for (; it4 != m_impl->m_engine_modules.end (); it4++) {
        if ((*it4)->get_module_name () == module) {
            SCIM_DEBUG_BACKEND (1) << __FUNCTION__ << " : " << module << "\n";
            (*it4)->unload ();
            delete (*it4);
            m_impl->m_engine_modules.erase (it4);
            break;
        }
    }
    malloc_trim (0);
}

void
CommonBackEnd::add_module_info_from_cache_file (const ConfigPointer &config, std::vector<String> &modules)
{
    SCIM_DEBUG_BACKEND (1) << __FUNCTION__ << "...\n";

    FILE *engine_list_file = NULL;
    FILE *user_engine_file = NULL;
    char buf[MAXLINE];
    bool isFirst = false;
    std::vector<String> current_modules;

    current_modules.clear ();

    String user_file_name = String (USER_ENGINE_FILE_NAME);
    String sys_file_name  = String (SYS_ENGINE_FILE_NAME);

    engine_list_file = fopen (user_file_name.c_str (), "r");
    if (engine_list_file == NULL) {
        std::cerr <<  user_file_name << " doesn't exist.\n";
        isFirst = true;

        engine_list_file = fopen (sys_file_name.c_str (), "r");
        if (engine_list_file == NULL) {
            std::cerr <<  sys_file_name << " doesn't exist.\n";
            goto failed_open;
        }
    }

    /*
     * If we start the system firstly, the engine list file in user directory
     * doesn't exit, we should create it.
     */
    if (isFirst) {
        user_engine_file = fopen (user_file_name.c_str (), "a");
        if (user_engine_file == NULL) {
            std::cerr << __func__ << " Failed to open(" << user_file_name << ")\n";
        }
    }

    while (fgets (buf, MAXLINE, engine_list_file) != NULL) {
        if (isFirst && user_engine_file != NULL) {
            if (fputs (buf, user_engine_file) == EOF)
                std::cerr << "failed to write " << user_file_name << "\n";
        }

        ISEINFO info;
        isf_get_ise_info_from_string (buf, info);
        std::vector<String>::iterator iter = std::find (modules.begin (), modules.end (), info.module);
        if (info.mode == TOOLBAR_KEYBOARD_MODE && iter != modules.end ()) {
            if (m_impl->m_factory_module_repository.find (info.uuid) == m_impl->m_factory_module_repository.end ()) {
                add_specific_factory (info.uuid, IMEngineFactoryPointer (0));
                m_impl->m_factory_module_repository[info.uuid] = info.module;
            }

            std::vector<String>::iterator iter2 = std::find (current_modules.begin (), current_modules.end (), info.module);
            if (iter2 == current_modules.end ())
                current_modules.push_back (info.module);
        }
    }

    if (isFirst && user_engine_file != NULL)
        fclose (user_engine_file);
    fclose (engine_list_file);

failed_open:
    update_module_info (config, current_modules, modules);
}

void
CommonBackEnd::update_module_info (const ConfigPointer &config,
                                   std::vector<String> &current_modules,
                                   std::vector<String> &modules)
{
    SCIM_DEBUG_BACKEND (1) << __FUNCTION__ << "...\n";

    std::vector<String> imengine_list;
    scim_get_imengine_module_list (imengine_list);
    for (size_t i = 0; i < imengine_list.size (); ++i) {
        if (std::find (modules.begin (), modules.end (), imengine_list [i]) != modules.end ()
                && std::find (current_modules.begin (), current_modules.end (), imengine_list [i]) == current_modules.end ())
                add_imengine_module_info (imengine_list [i], config);
    }
}

void
CommonBackEnd::add_imengine_module_info (const String module_name, const ConfigPointer &config)
{
    SCIM_DEBUG_BACKEND (1) << __FUNCTION__ << " : " << module_name << "\n";

    if (module_name.length () <= 0 || module_name == "socket")
        return;

    IMEngineFactoryPointer factory;
    IMEngineModule         ime_module;

    String filename = String (USER_ENGINE_FILE_NAME);
    FILE *engine_list_file = fopen (filename.c_str (), "a");
    if (engine_list_file == NULL) {
        LOGW ("Failed to open %s!!!\n", filename.c_str ());
        return;
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
                String uuid = factory->get_uuid ();
                String name = utf8_wcstombs (factory->get_name ());
                String language = isf_get_normalized_language (factory->get_language ());
                String icon = factory->get_icon_file ();
                char mode[12];
                char option[12];

                snprintf (mode, sizeof (mode), "%d", (int)TOOLBAR_KEYBOARD_MODE);
                snprintf (option, sizeof (option), "%d", factory->get_option ());

                String line = isf_combine_ise_info_string (name, uuid, module_name, language,
                                                           icon, String (mode), String (option), factory->get_locales ());
                if (fputs (line.c_str (), engine_list_file) < 0) {
                    LOGW ("Failed to write (%s)!!!\n", line.c_str ());
                    break;
                }

                add_specific_factory (uuid, IMEngineFactoryPointer (0));
                m_impl->m_factory_module_repository[uuid] = module_name;

                factory.reset ();
            }
        }
    } else {
        std::cerr << __func__ << ": Failed to load " << module_name << " IMEngine module!!!\n";
    }

    ime_module.unload ();
    int ret = fclose (engine_list_file);
    if (ret != 0)
        LOGW ("Failed to fclose %s!!!\n", filename.c_str ());
}


} /* namespace scim */

/*
vi:ts=4:nowrap:ai:expandtab
*/
