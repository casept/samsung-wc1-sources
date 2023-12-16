/** @file scim_imengine.cpp
 *  @brief Implementation of class IMEngineFactoryBase and IMEngineInstanceBase.
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
 * 1. Add new interface APIs for keyboard ISE
 *    a. select_aux (), set_prediction_allow () and set_layout ()
 *    b. update_candidate_item_layout (), update_cursor_position () and update_displayed_candidate_number ()
 *    c. candidate_more_window_show (), candidate_more_window_hide () and longpress_candidate ()
 *    d. set_imdata () and reset_option ()
 * 2. Add get_option () in IMEngineFactoryBase class
 *
 * $Id: scim_imengine.cpp,v 1.15 2005/04/08 15:24:11 suzhe Exp $
 *
 */

#define Uses_SCIM_IMENGINE
#define Uses_C_CTYPE

#include "scim_private.h"
#include "scim.h"

namespace scim {

typedef Signal1<void, IMEngineInstanceBase*>
        IMEngineSignalVoid;

typedef Signal2<void, IMEngineInstanceBase*,int>
        IMEngineSignalInt;

typedef Signal2<void, IMEngineInstanceBase*,bool>
        IMEngineSignalBool;

typedef Signal2<void, IMEngineInstanceBase*,const String&>
        IMEngineSignalString;

typedef Signal2<void, IMEngineInstanceBase*,const WideString&>
        IMEngineSignalWideString;

typedef Signal2<void, IMEngineInstanceBase*,const KeyEvent&>
        IMEngineSignalKeyEvent;

typedef Signal2<void, IMEngineInstanceBase*,const Property&>
        IMEngineSignalProperty;

typedef Signal2<void, IMEngineInstanceBase*,const PropertyList&>
        IMEngineSignalPropertyList;

typedef Signal2<void, IMEngineInstanceBase*,const LookupTable&>
        IMEngineSignalLookupTable;

typedef Signal3<void, IMEngineInstanceBase*,const String&,const Transaction&>
        IMEngineSignalStringTransaction;

typedef Signal4<void, IMEngineInstanceBase*,const WideString&,const AttributeList&,int>
        IMEngineSignalWideStringAttributeListInt;

typedef Signal3<void, IMEngineInstanceBase*,const WideString&,const AttributeList&>
        IMEngineSignalWideStringAttributeList;

typedef Signal5<bool, IMEngineInstanceBase*,WideString&,int&,int,int>
        IMEngineSignalGetSurroundingText;

typedef Signal3<bool, IMEngineInstanceBase*,int,int>
        IMEngineSignalDeleteSurroundingText;

typedef Signal2<bool, IMEngineInstanceBase*,WideString&>
        IMEngineSignalGetSelection;

typedef Signal3<bool, IMEngineInstanceBase*,int,int>
        IMEngineSignalSetSelection;

typedef Signal3<void, IMEngineInstanceBase*,ISF_CANDIDATE_PORTRAIT_LINE_T,ISF_CANDIDATE_MODE_T>
        IMEngineSignalCandidateStyle;

class IMEngineFactoryBase::IMEngineFactoryBaseImpl
{
public:
    std::vector<String> m_encoding_list;
    std::vector<String> m_locale_list;
    String              m_language;
};

class IMEngineInstanceBase::IMEngineInstanceBaseImpl
{
public:
    IMEngineFactoryPointer                m_factory;
    String                                m_encoding;

    IMEngineSignalVoid                    m_signal_show_preedit_string;
    IMEngineSignalVoid                    m_signal_show_aux_string;
    IMEngineSignalVoid                    m_signal_show_lookup_table;

    IMEngineSignalVoid                    m_signal_hide_preedit_string;
    IMEngineSignalVoid                    m_signal_hide_aux_string;
    IMEngineSignalVoid                    m_signal_hide_lookup_table;

    IMEngineSignalInt                     m_signal_update_preedit_caret;
    IMEngineSignalWideStringAttributeListInt m_signal_update_preedit_string;
    IMEngineSignalWideStringAttributeList m_signal_update_aux_string;
    IMEngineSignalWideString              m_signal_commit_string;
    IMEngineSignalLookupTable             m_signal_update_lookup_table;

    IMEngineSignalKeyEvent                m_signal_forward_key_event;

    IMEngineSignalPropertyList            m_signal_register_properties;
    IMEngineSignalProperty                m_signal_update_property;

    IMEngineSignalVoid                    m_signal_beep;

    IMEngineSignalString                  m_signal_start_helper;
    IMEngineSignalString                  m_signal_stop_helper;
    IMEngineSignalStringTransaction       m_signal_send_helper_event;

    IMEngineSignalGetSurroundingText      m_signal_get_surrounding_text;
    IMEngineSignalDeleteSurroundingText   m_signal_delete_surrounding_text;

    IMEngineSignalGetSelection            m_signal_get_selection;
    IMEngineSignalSetSelection            m_signal_set_selection;

    IMEngineSignalVoid                    m_signal_expand_candidate;
    IMEngineSignalVoid                    m_signal_contract_candidate;

    IMEngineSignalCandidateStyle          m_signal_set_candidate_style;

    IMEngineSignalString                  m_signal_send_private_command;

    int    m_id;
    void * m_frontend_data;

    IMEngineInstanceBaseImpl () : m_id (0), m_frontend_data (0) { }
};

IMEngineFactoryBase::IMEngineFactoryBase ()
    : m_impl (new IMEngineFactoryBaseImpl ())
{
}

IMEngineFactoryBase::~IMEngineFactoryBase ()
{
    delete m_impl;
}

bool
IMEngineFactoryBase::validate_encoding (const String& encoding) const
{
    if (encoding == "UTF-8")
        return true;

    for (size_t i=0; i<m_impl->m_encoding_list.size (); ++i)
        if (m_impl->m_encoding_list [i] == encoding)
            return true;

    return false;
}

bool
IMEngineFactoryBase::validate_locale (const String& locale) const
{
    for (size_t i=0; i<m_impl->m_locale_list.size (); ++i)
        if (m_impl->m_locale_list [i] == locale)
            return true;

    if (scim_get_locale_encoding (locale) == "UTF-8")
        return true;

    return false;
}

String
IMEngineFactoryBase::get_language () const
{
    return m_impl->m_language;
}

WideString
IMEngineFactoryBase::inverse_query (const WideString &str)
{
    return WideString ();
}

void
IMEngineFactoryBase::load_resource ()
{
    return ;
}

unsigned int
IMEngineFactoryBase::get_option () const
{
    return 0;
}

String
IMEngineFactoryBase::get_encodings () const
{
    return scim_combine_string_list (m_impl->m_encoding_list);
}

String
IMEngineFactoryBase::get_locales () const
{
    return scim_combine_string_list (m_impl->m_locale_list);
}

String
IMEngineFactoryBase::get_default_encoding () const
{
    if (m_impl->m_encoding_list.size ())
        return m_impl->m_encoding_list [0];
    return String ("UTF-8");
}

String
IMEngineFactoryBase::get_default_locale () const
{
    if (m_impl->m_locale_list.size ())
        return m_impl->m_locale_list [0];
    return String ("");
}

void
IMEngineFactoryBase::set_locales (const String& locales)
{
    m_impl->m_locale_list.clear ();
    m_impl->m_encoding_list.clear ();

    if (locales.size () == 0) return;

    String locale;
    std::vector <String> lc_list;

    scim_split_string_list (lc_list, locales);

    for (size_t i=0; i<lc_list.size (); ++i) {
        locale = scim_validate_locale (lc_list [i]);
        if (locale.length ()) {
            m_impl->m_locale_list.push_back (locale);
            m_impl->m_encoding_list.push_back (scim_get_locale_encoding (locale));
        }
    }

    m_impl->m_language = scim_get_locale_language (get_default_locale ());
}

void
IMEngineFactoryBase::set_languages (const String& languages)
{
    std::vector <String> lang_list;
    String locales;
    String all_locales;
    std::vector<String> valid_langlist;
    String valid_lang;

    scim_split_string_list (lang_list, languages, ',');

    for (size_t i = 0; i < lang_list.size (); ++ i) {
        locales = scim_get_language_locales (lang_list [i]);
        if (locales.length ()) {
            if (all_locales.length ())
                all_locales.push_back (',');
            all_locales += locales;
        }
    }

    if (all_locales.length ())
        set_locales (all_locales);

    if (lang_list.size ()) {
        for (size_t i = 0; i < lang_list.size (); i++) {
            valid_lang = scim_validate_language (lang_list [i]);
            valid_langlist.push_back (valid_lang);
        }
        m_impl->m_language = scim_combine_string_list (valid_langlist);
    }
}

IMEngineInstanceBase::IMEngineInstanceBase (IMEngineFactoryBase *factory,
                                            const String        &encoding,
                                            int                  id)
    : m_impl (new IMEngineInstanceBaseImpl ())
{
    m_impl->m_factory = factory;
    m_impl->m_encoding = encoding;
    m_impl->m_id = id;

    if (!m_impl->m_factory.null ()) {
        if (!m_impl->m_factory->validate_encoding (encoding)) {
            m_impl->m_encoding = m_impl->m_factory->get_default_encoding ();
        }
    } else {
        m_impl->m_encoding = String ("UTF-8");
    }
}

IMEngineInstanceBase::~IMEngineInstanceBase ()
{
    delete m_impl;
}

bool
IMEngineInstanceBase::set_encoding (const String &encoding)
{
    if (!m_impl->m_factory.null () && m_impl->m_factory->validate_encoding (encoding)) {
        m_impl->m_encoding = encoding;
        return true;
    }
    return false;
}

String
IMEngineInstanceBase::get_encoding () const
{
    return m_impl->m_encoding;
}

int
IMEngineInstanceBase::get_id () const
{
    return m_impl->m_id;
}

String
IMEngineInstanceBase::get_factory_uuid () const
{
    if (!m_impl->m_factory.null ())
        return m_impl->m_factory->get_uuid ();

    return String ();
}

void
IMEngineInstanceBase::set_frontend_data (void *data)
{
    m_impl->m_frontend_data = data;
}

void *
IMEngineInstanceBase::get_frontend_data (void)
{
    return m_impl->m_frontend_data;
}

void
IMEngineInstanceBase::move_preedit_caret (unsigned int)
{
}

void
IMEngineInstanceBase::select_aux (unsigned int)
{
}

void
IMEngineInstanceBase::select_candidate (unsigned int)
{
}

void
IMEngineInstanceBase::update_lookup_table_page_size (unsigned int)
{
}

void
IMEngineInstanceBase::lookup_table_page_up ()
{
}

void
IMEngineInstanceBase::lookup_table_page_down ()
{
}

void
IMEngineInstanceBase::set_prediction_allow (bool allow)
{
}

void
IMEngineInstanceBase::set_layout (unsigned int layout)
{
}

void
IMEngineInstanceBase::set_input_hint (unsigned int input_hint)
{
}

void
IMEngineInstanceBase::update_bidi_direction (unsigned int bidi_direction)
{
}

void
IMEngineInstanceBase::update_candidate_item_layout (const std::vector<unsigned int> &row_items)
{
}

void
IMEngineInstanceBase::update_cursor_position (unsigned int cursor_pos)
{
}

void
IMEngineInstanceBase::update_displayed_candidate_number (unsigned int number)
{
}

void
IMEngineInstanceBase::candidate_more_window_show (void)
{
}

void
IMEngineInstanceBase::candidate_more_window_hide (void)
{
}

void
IMEngineInstanceBase::longpress_candidate (unsigned int index)
{
}

void
IMEngineInstanceBase::set_imdata (const char *data, unsigned int len)
{
}

void
IMEngineInstanceBase::reset_option ()
{
}

void
IMEngineInstanceBase::reset ()
{
}

void
IMEngineInstanceBase::focus_in ()
{
}

void
IMEngineInstanceBase::focus_out ()
{
}

void
IMEngineInstanceBase::trigger_property (const String &)
{
}

void
IMEngineInstanceBase::process_helper_event (const String &, const Transaction &)
{
}

void
IMEngineInstanceBase::update_client_capabilities (unsigned int /*cap*/)
{
}

Connection
IMEngineInstanceBase::signal_connect_show_preedit_string (IMEngineSlotVoid *slot)
{
    return m_impl->m_signal_show_preedit_string.connect (slot);
}

Connection
IMEngineInstanceBase::signal_connect_show_aux_string (IMEngineSlotVoid *slot)
{
    return m_impl->m_signal_show_aux_string.connect (slot);
}

Connection
IMEngineInstanceBase::signal_connect_show_lookup_table (IMEngineSlotVoid *slot)
{
    return m_impl->m_signal_show_lookup_table.connect (slot);
}

Connection
IMEngineInstanceBase::signal_connect_hide_preedit_string (IMEngineSlotVoid *slot)
{
    return m_impl->m_signal_hide_preedit_string.connect (slot);
}

Connection
IMEngineInstanceBase::signal_connect_hide_aux_string (IMEngineSlotVoid *slot)
{
    return m_impl->m_signal_hide_aux_string.connect (slot);
}

Connection
IMEngineInstanceBase::signal_connect_hide_lookup_table (IMEngineSlotVoid *slot)
{
    return m_impl->m_signal_hide_lookup_table.connect (slot);
}

Connection
IMEngineInstanceBase::signal_connect_update_preedit_caret (IMEngineSlotInt *slot)
{
    return m_impl->m_signal_update_preedit_caret.connect (slot);
}

Connection
IMEngineInstanceBase::signal_connect_update_preedit_string (IMEngineSlotWideStringAttributeListInt *slot)
{
    return m_impl->m_signal_update_preedit_string.connect (slot);
}

Connection
IMEngineInstanceBase::signal_connect_update_aux_string (IMEngineSlotWideStringAttributeList *slot)
{
    return m_impl->m_signal_update_aux_string.connect (slot);
}

Connection
IMEngineInstanceBase::signal_connect_update_lookup_table (IMEngineSlotLookupTable *slot)
{
    return m_impl->m_signal_update_lookup_table.connect (slot);
}

Connection
IMEngineInstanceBase::signal_connect_commit_string (IMEngineSlotWideString *slot)
{
    return m_impl->m_signal_commit_string.connect (slot);
}

Connection
IMEngineInstanceBase::signal_connect_forward_key_event (IMEngineSlotKeyEvent *slot)
{
    return m_impl->m_signal_forward_key_event.connect (slot);
}

Connection
IMEngineInstanceBase::signal_connect_register_properties (IMEngineSlotPropertyList *slot)
{
    return m_impl->m_signal_register_properties.connect (slot);
}

Connection
IMEngineInstanceBase::signal_connect_update_property (IMEngineSlotProperty *slot)
{
    return m_impl->m_signal_update_property.connect (slot);
}

Connection
IMEngineInstanceBase::signal_connect_beep (IMEngineSlotVoid *slot)
{
    return m_impl->m_signal_beep.connect (slot);
}

Connection
IMEngineInstanceBase::signal_connect_start_helper (IMEngineSlotString *slot)
{
    return m_impl->m_signal_start_helper.connect (slot);
}

Connection
IMEngineInstanceBase::signal_connect_stop_helper (IMEngineSlotString *slot)
{
    return m_impl->m_signal_stop_helper.connect (slot);
}

Connection
IMEngineInstanceBase::signal_connect_send_helper_event (IMEngineSlotStringTransaction *slot)
{
    return m_impl->m_signal_send_helper_event.connect (slot);
}

Connection
IMEngineInstanceBase::signal_connect_get_surrounding_text (IMEngineSlotGetSurroundingText *slot)
{
    return m_impl->m_signal_get_surrounding_text.connect (slot);
}

Connection
IMEngineInstanceBase::signal_connect_delete_surrounding_text (IMEngineSlotDeleteSurroundingText *slot)
{
    return m_impl->m_signal_delete_surrounding_text.connect (slot);
}

Connection
IMEngineInstanceBase::signal_connect_get_selection (IMEngineSlotGetSelection *slot)
{
    return m_impl->m_signal_get_selection.connect (slot);
}

Connection
IMEngineInstanceBase::signal_connect_set_selection (IMEngineSlotSetSelection *slot)
{
    return m_impl->m_signal_set_selection.connect (slot);
}

Connection
IMEngineInstanceBase::signal_connect_expand_candidate (IMEngineSlotVoid *slot)
{
    return m_impl->m_signal_expand_candidate.connect (slot);
}

Connection
IMEngineInstanceBase::signal_connect_contract_candidate (IMEngineSlotVoid *slot)
{
    return m_impl->m_signal_contract_candidate.connect (slot);
}

Connection
IMEngineInstanceBase::signal_connect_set_candidate_style (IMEngineSlotCandidateStyle *slot)
{
    return m_impl->m_signal_set_candidate_style.connect (slot);
}

Connection
IMEngineInstanceBase::signal_connect_send_private_command (IMEngineSlotString *slot)
{
    return m_impl->m_signal_send_private_command.connect (slot);
}

void
IMEngineInstanceBase::show_preedit_string ()
{
    m_impl->m_signal_show_preedit_string (this);
}

void
IMEngineInstanceBase::show_aux_string ()
{
    m_impl->m_signal_show_aux_string (this);
}

void
IMEngineInstanceBase::show_lookup_table ()
{
    m_impl->m_signal_show_lookup_table (this);
}

void
IMEngineInstanceBase::hide_preedit_string ()
{
    m_impl->m_signal_hide_preedit_string (this);
}

void
IMEngineInstanceBase::hide_aux_string ()
{
    m_impl->m_signal_hide_aux_string (this);
}

void
IMEngineInstanceBase::hide_lookup_table ()
{
    m_impl->m_signal_hide_lookup_table (this);
}

void
IMEngineInstanceBase::update_preedit_caret (int caret)
{
    m_impl->m_signal_update_preedit_caret (this, caret);
}

void
IMEngineInstanceBase::update_preedit_string (const WideString    &str,
                                             const AttributeList &attrs)
{
    m_impl->m_signal_update_preedit_string (this, str, attrs, -1);
}

void
IMEngineInstanceBase::update_preedit_string (const WideString    &str,
                                             const AttributeList &attrs,
                                             int            caret)
{
    m_impl->m_signal_update_preedit_string (this, str, attrs, caret);
}

void
IMEngineInstanceBase::update_aux_string (const WideString    &str,
                                         const AttributeList &attrs)
{
    m_impl->m_signal_update_aux_string (this, str, attrs);
}

void
IMEngineInstanceBase::update_lookup_table (const LookupTable &table)
{
    m_impl->m_signal_update_lookup_table (this, table);
}

void
IMEngineInstanceBase::commit_string (const WideString &str)
{
    m_impl->m_signal_commit_string (this, str);
}

void
IMEngineInstanceBase::forward_key_event (const KeyEvent &key)
{
    m_impl->m_signal_forward_key_event (this, key);
}

void
IMEngineInstanceBase::register_properties (const PropertyList &properties)
{
    m_impl->m_signal_register_properties (this, properties);
}

void
IMEngineInstanceBase::update_property (const Property &property)
{
    m_impl->m_signal_update_property (this, property);
}

void
IMEngineInstanceBase::beep ()
{
    m_impl->m_signal_beep (this);
}

void
IMEngineInstanceBase::start_helper (const String &helper_uuid)
{
    m_impl->m_signal_start_helper (this, helper_uuid);
}

void
IMEngineInstanceBase::stop_helper (const String &helper_uuid)
{
    m_impl->m_signal_stop_helper (this, helper_uuid);
}

void
IMEngineInstanceBase::send_helper_event (const String &helper_uuid, const Transaction &trans)
{
    m_impl->m_signal_send_helper_event (this, helper_uuid, trans);
}

bool
IMEngineInstanceBase::get_surrounding_text (WideString &text, int &cursor, int maxlen_before, int maxlen_after)
{
    text = WideString ();
    cursor = 0;

    if (maxlen_before == 0 && maxlen_after == 0)
        return false;

    if (m_impl->m_signal_get_surrounding_text (this, text, cursor, maxlen_before, maxlen_after) && text.length ())
        return true;

    return false;
}

bool
IMEngineInstanceBase::delete_surrounding_text (int offset, int len)
{
    return m_impl->m_signal_delete_surrounding_text (this, offset, len);
}

bool
IMEngineInstanceBase::get_selection (WideString &text)
{
    if (m_impl->m_signal_get_selection (this, text))
        return true;

    return false;
}

bool
IMEngineInstanceBase::set_selection (int start, int end)
{
    return m_impl->m_signal_set_selection (this, start, end);
}

void
IMEngineInstanceBase::expand_candidate (void)
{
    m_impl->m_signal_expand_candidate (this);
}

void
IMEngineInstanceBase::contract_candidate (void)
{
    m_impl->m_signal_contract_candidate (this);
}

void
IMEngineInstanceBase::set_candidate_style (ISF_CANDIDATE_PORTRAIT_LINE_T portrait_line,
                                           ISF_CANDIDATE_MODE_T          mode)
{
    m_impl->m_signal_set_candidate_style (this, portrait_line, mode);
}

void
IMEngineInstanceBase::set_autocapital_type (int mode)
{
}

void
IMEngineInstanceBase::send_private_command (const String &command)
{
    m_impl->m_signal_send_private_command (this, command);
}

// implementation of DummyIMEngine
DummyIMEngineFactory::DummyIMEngineFactory ()
{
    set_locales ("C");
}

DummyIMEngineFactory::~DummyIMEngineFactory ()
{
}

WideString
DummyIMEngineFactory::get_name () const
{
    return utf8_mbstowcs (_("English Keyboard"));
}

WideString
DummyIMEngineFactory::get_authors () const
{
    return WideString ();
}

WideString
DummyIMEngineFactory::get_credits () const
{
    return WideString ();
}

WideString
DummyIMEngineFactory::get_help () const
{
    return WideString ();
}

String
DummyIMEngineFactory::get_uuid () const
{
    return String ("0148bcec-850d-45f2-ba95-a493bb31492e");
}

String
DummyIMEngineFactory::get_icon_file () const
{
    return String (SCIM_KEYBOARD_ICON_FILE);
}

bool
DummyIMEngineFactory::validate_encoding (const String& encoding) const
{
    return true;
}

bool
DummyIMEngineFactory::validate_locale (const String& locale) const
{
    return true;
}

IMEngineInstancePointer
DummyIMEngineFactory::create_instance (const String& encoding, int id)
{
    return new DummyIMEngineInstance (this, encoding, id);
}

DummyIMEngineInstance::DummyIMEngineInstance (DummyIMEngineFactory *factory,
                                        const String& encoding,
                                        int id)
    : IMEngineInstanceBase (factory, encoding, id)
{
}

DummyIMEngineInstance::~DummyIMEngineInstance ()
{
}

bool
DummyIMEngineInstance::process_key_event (const KeyEvent& key)
{
    return false;
}

void
DummyIMEngineInstance::focus_in ()
{
    register_properties (PropertyList ());
}

} // namespace scim

/*
vi:ts=4:nowrap:ai:expandtab
*/
