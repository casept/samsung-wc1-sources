/** @file scim_frontend.h
 *  @brief Defines scim::FrontEndBase interface.
 *
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
 * 1. Dynamic load keyboard ISE
 * 2. Add new interface APIs for keyboard ISE
 *    a. expand_candidate (), contract_candidate () and set_candidate_style ()
 *    b. select_aux (), set_prediction_allow () and set_layout ()
 *    c. update_candidate_item_layout (), update_cursor_position () and update_displayed_candidate_number ()
 *    d. candidate_more_window_show (), candidate_more_window_hide () and longpress_candidate ()
 *    e. set_imdata () and reset_option ()
 *
 * $Id: scim_frontend.h,v 1.42 2005/10/06 18:02:06 liuspider Exp $
 */

#ifndef __SCIM_FRONTEND_H
#define __SCIM_FRONTEND_H

namespace scim {

/**
 * @addtogroup FrontEnd
 * @ingroup InputServiceFramework
 * The base classes for FrontEnd modules.
 * @{
 */

/**
 * @brief An exception class to hold FrontEnd related errors.
 *
 * scim::FrontEndBase and its derived classes must throw
 * scim::FrontEndError object when error.
 */
class EAPI FrontEndError: public Exception
{
public:
    FrontEndError (const String& what_arg)
        : Exception (String("scim::FrontEnd: ") + what_arg) { }
};

class FrontEndBase;

/**
 * @typedef typedef Pointer <FrontEndBase> FrontEndPointer;
 *
 * A smart pointer for scim::FrontEndBase and its derived classes.
 */
typedef Pointer <FrontEndBase> FrontEndPointer;

/**
 * @brief The base class to implement the FrontEnd objects.
 *
 * FrontEnd is an interface between IMEngineFactory/IMEngineInstance objects
 * and the user applications. It forward the user requests to
 * IMEngineFactory/IMEngineInstance objects, and handle the requests sent back.
 */
class EAPI FrontEndBase : public ReferencedObject
{
    class FrontEndBaseImpl;

    FrontEndBaseImpl *m_impl;

    friend class FrontEndBaseImpl;

public:
    /**
     * @brief Constructor.
     * @param backend A BackEnd object which holds all IMEngineFactory objects.
     */
    FrontEndBase (const BackEndPointer &backend);

    /**
     * @brief Virtual destructor.
     */
    virtual ~FrontEndBase ();

protected:
    /**
     * @name functions can be used by derived classes.
     *
     * @{
     */

    /**
     * @brief Add one IMEngine module in BackEnd.
     *
     *
     * @return void.
     */
    void add_module (const ConfigPointer &config, const String module, bool is_load_resource) const;

    /**
     * @brief Add one IMEngine module info in BackEnd.
     *
     *
     * @return void.
     */
    void add_module_info (const ConfigPointer &config, const String module) const;

    /**
     * @brief Get the IMEngine factories list for specific encoding.
     *
     * @param uuids    the vector to store the factories' uuids which
     *                 support the encoding.
     * @param encoding the encoding to be queried. If empty,
     *                 all IMEngine factories will be returned.
     *
     * @return the number of IMEngine factories found.
     */
    uint32 get_factory_list_for_encoding (std::vector<String> &uuids, const String &encoding) const;

    /**
     * @brief Get the IMEngine factories list for specific language.
     *
     * @param uuids    the vector to store the factories' uuids which
     *                 support the language.
     * @param language the language to be queried. If empty,
     *                 all IMEngine factories will be returned.
     *
     * @return the number of IMEngine factories found.
     */
    uint32 get_factory_list_for_language (std::vector<String> &uuids, const String &language) const;

    /**
     * @brief Get the IMEngine factories list.
     *
     * @param uuids the vector to store the existing factories' uuids.
     *
     * @return the number of IMEngine factories found.
     */
    uint32 get_factory_list (std::vector<String> &uuids) const;

    /**
     * @brief Get the default IMEngineFactory UUID for a specific language and encoding.
     *
     * @param language the language to be queried.
     * @param encoding the encoding to be queried, if empty then don't match encoding.
     *
     * @return the UUID of the default IMEngineFactory for this language.
     */
    String get_default_factory (const String &language, const String &encoding) const;

    /**
     * @brief Set the default IMEngineFactory for a specific language.
     *
     * @param language the language to be set.
     * @param uuid the uuid of the default IMEngineFactory for this language.
     */
    void set_default_factory (const String &language, const String &uuid);

    /**
     * @brief Get the next IMEngineFactory for a specific language and encoding.
     *
     * @param language the language to be queried, if empty then don't match language.
     * @param encoding the encoding to be queried, if empty then don't match encoding.
     * @param cur_uuid the UUID of current IMEngineFactory.
     *
     * @return the UUID of the next IMEngineFactory for this language and encoding
     *         corresponding to the current IMEngineFactory.
     */
    String get_next_factory (const String &language, const String &encoding, const String &cur_uuid) const;

    /**
     * @brief Get the previous IMEngineFactory for a specific language and encoding.
     *
     * @param language the language to be queried, if empty then don't match language.
     * @param encoding the encoding to be queried, if empty then don't match encoding.
     * @param cur_uuid the UUID of current IMEngineFactory.
     *
     * @return the UUID of the previous IMEngineFactory for this language and encoding
     *         corresponding to the current IMEngineFactory.
     */
    String get_previous_factory (const String &language, const String &encoding, const String &cur_uuid) const;

    /**
     * @brief get the name of an IMEngine factory.
     *
     * @param uuid the uuid of the IMEngine factory
     * @return the name of the IMEngine factory.
     */
    WideString get_factory_name (const String &uuid) const;

    /**
     * @brief get the authors info of an IMEngine factory.
     * @param uuid the uuid of the IMEngine factory
     * @return the authors info of the IMEngine factory.
     */
    WideString get_factory_authors (const String &uuid) const;

    /**
     * @brief get the credits info of an IMEngine factory.
     * @param uuid the uuid of the IMEngine factory
     * @return the credits info of the IMEngine factory.
     */
    WideString get_factory_credits (const String &uuid) const;

    /**
     * @brief get the help info of an IMEngine factory.
     * @param uuid the uuid of the IMEngine factory
     * @return the help info of the IMEngine factory.
     */
    WideString get_factory_help (const String &uuid) const;

    /**
     * @brief get the icon file of an IMEngine factory.
     * @param uuid the uuid of the IMEngine factory
     * @return the icon file name of the IMEngine factory.
     */
    String get_factory_icon_file (const String &uuid) const;

    /**
     * @brief get the supported locales of an IMEngine  factory.
     * @param uuid the uuid of the IMEngine factory
     * @return a comma separated list of the supported locales.
     */
    String get_factory_locales (const String &uuid) const;

    /**
     * @brief get the language of an IMEngine factory.
     * @param uuid the uuid of the IMEngine factory
     * @return the language of this IMEngine factory.
     */
    String get_factory_language (const String &uuid) const;

    /**
     * @brief Check if an IMEngine factory is valid and the given encoding is supported by it.
     *
     * @param uuid The uuid of the IMEngine factory to be checked.
     * @param encoding The encoding should be supported by the factory.
     * @return true if the factory is valid and the given encoding is supported.
     */
    bool validate_factory (const String &uuid, const String &encoding = String ("")) const;

    /**
     * @brief get all locales supported by BackEnd.
     * @return a comman separated list of all supported locales.
     */
    String get_all_locales () const;

    // IMEngine instance related functions.

    /**
     * @brief create a new IMEngine instance for specific encoding.
     *
     * @param sf_uuid the IMEngineFactory UUID.
     * @param encoding the encoding to be used.
     *
     * @return the newly created IMEngine instance id, -1 means error occurred.
     */
    int  new_instance (const ConfigPointer &config, const String &sf_uuid, const String &encoding);

    /**
     * @brief replace an IMEngine  instance by a new instance created by another factory.
     *
     * This function is used to change the input method for an input context on the fly.
     *
     * @param si_id the IMEngine instance to be replaced.
     * @param sf_uuid the new IMEngine factory to be used.
     * @return true if success.
     */
    bool replace_instance (int si_id, const String &sf_uuid);

    /**
     * @brief delete an IMEngine instance according to its id.
     * @param id the id of the IMEngine instance to be deleted.
     * @return true if success, false if there is no such instance.
     */
    bool delete_instance (int id);

    /**
     * @brief delete all IMEngine instances.
     *
     * This function should be called just before quitting the FrontEnd.
     */
    void delete_all_instances ();

    /**
     * @brief get the factory uuid of this instance.
     * @param id the IMEngine instance id.
     * @return the factory uuid of this instance.
     */
    String get_instance_uuid (int id) const;

    /**
     * @brief get the working encoding of an IMEngine instance.
     * @param id the IMEngine instance id.
     * @return the working encoding of this IMEngine instance.
     */
    String get_instance_encoding (int id) const;

    /**
     * @brief get the name of an IMEngine instance.
     * @param id the IMEngine instance id.
     * @return the name of this IMEngine instance,
     *         aka. the name of its factory.
     */
    WideString get_instance_name (int id) const;

    /**
     * @brief get the authors info of an IMEngine instance.
     * @param id the IMEngine instance id.
     * @return the authors info of this IMEngine instance,
     *         aka. the authors of its factory.
     */
    WideString get_instance_authors (int id) const;

    /**
     * @brief get the credits info of an IMEngine instance.
     * @param id the IMEngine instance id.
     * @return the credits info of this IMEngine instance,
     *         aka. the credits of its factory.
     */
    WideString get_instance_credits (int id) const;

    /**
     * @brief get the help of an IMEngine instance.
     * @param id the IMEngine instance id.
     * @return the help of this IMEngine instance,
     *         aka. the help of its factory.
     */
    WideString get_instance_help (int id) const;

    /**
     * @brief get the icon file of an IMEngine instance.
     * @param id the IMEngine instance id.
     * @return the icon file name of this IMEngine instance.
     */
    String get_instance_icon_file (int id) const;

    /**
     * @brief get the name list of existing IMEngine instances.
     * @param vec the vector to store the name of existing IMEngine instances,
     *         aka. the name of its factory.
     */
    void get_instance_list(std::vector<String> &vec) const;

    /**
     * @brief output uuid of existing IMEngine instances to scim debug.
     */
    void dump_instances ();

    /**
     * @brief process a key event using specific IMEngine instance.
     * @param id the IMEngine instance id.
     * @param key the key event to be processed.
     * @return true if the event was processed successfully,
     *         false if the event was not processed and should
     *         be forward to the client application.
     */
    bool process_key_event (int id, const KeyEvent& key) const;

    /**
     * @brief let a specific IMEngine instance move its preedit caret.
     * @param id the IMEngine instance id.
     * @param pos the new preedit caret position.
     */
    void move_preedit_caret (int id, unsigned int pos) const;

    /**
     * @brief let a specific IMEngine instance select a aux in its current aux string.
     * @param id the IMEngine instance id.
     * @param index the aux index in current aux string to be selected.
     */
    void select_aux (int id, unsigned int index) const;

    /**
     * @brief let a specific IMEngine instance select a candidate in its current lookup table.
     * @param id the IMEngine instance id.
     * @param index the candidate index in current lookup table page to be selected.
     */
    void select_candidate (int id, unsigned int index) const;

    /**
     * @brief update the page size of a specific IMEngine instance's lookup table.
     * @param id the IMEngine instance id.
     * @param page_size the new page size to be used.
     */
    void update_lookup_table_page_size (int id, unsigned int page_size) const;

    /**
     * @brief Let a specific IMEngine instance flip its lookup table to the previous page.
     */
    void lookup_table_page_up (int id) const;

    /**
     * @brief Let a specific IMEngine instance flip its lookup table to the previous page.
     */
    void lookup_table_page_down (int id) const;

    /**
     * @brief Set prediction allow.
     * @param id the IMEngine instance id.
     * @param allow - the flag of prediction allow.
     */
    void set_prediction_allow (int id, bool allow) const;

    /**
     * @brief Set layout.
     * @param id the IMEngine instance id.
     * @param layout - the layout.
     */
    void set_layout (int id, unsigned int layout) const;

    /**
     * @brief reset a specific IMEngine option.
     * @param id the id of the IMEngine instance to be reset.
     */
    void reset_option (int id) const;

    /**
     * @brief reset a specific IMEngine instance.
     * @param id the id of the IMEngine instance to be reset.
     */
    void reset (int id) const;

    /**
     * @brief focus in a specific IMEngine instance.
     * @param id the id of the IMEngine instance to be focused in.
     */
    void focus_in (int id) const;

    /**
     * @brief focus out a specific IMEngine instance.
     * @param id the id of the IMEngine instance to be focused out.
     */
    void focus_out (int id) const;

    /**
     * @brief trigger a property of a specific IMEngine instance.
     * @param id the id of the IMEngine instance.
     * @param property the key of the property to be triggered.
     */
    void trigger_property (int id, const String &property) const;

    /**
     * @brief let a specific IMEngine instance to process the events sent from a Helper process.
     * @param id the id of the IMEngine instance.
     * @param helper_uuid the uuid of the Helper process.
     * @param trans the transaction which contains the events.
     */
    void process_helper_event (int id, const String &helper_uuid, const Transaction &trans) const;

    /**
     * @brief let a specific IMEngine instance to update itself according to the capabilities of the client application.
     * @param id the id of the IMEngine instance.
     * @param cap the bitset of the capabilities which are supported by client.
     */
    void update_client_capabilities (int id, unsigned int cap) const;

    /**
     * @brief update candidate items layout.
     * @param id the IMEngine instance id.
     * @param row_items the items of each row.
     */
    void update_candidate_item_layout (int id, const std::vector<unsigned int> &row_items) const;

    /**
     * @brief update cursor position.
     * @param id the IMEngine instance id.
     * @param cursor_pos the new cursor position.
     */
    void update_cursor_position (int id, unsigned int cursor_pos) const;

    /**
     * @brief Update displayed candidate number
     * @param id the IMEngine instance id.
     * @param number - the number of displayed candidates.
     */
    void update_displayed_candidate_number (int id, unsigned int number) const;

    /**
     * @brief Candidate more window is shown
     * @param id the IMEngine instance id.
     */
    void candidate_more_window_show (int id) const;

    /**
     * @brief Candidate more window is hidden
     * @param id the IMEngine instance id.
     */
    void candidate_more_window_hide (int id) const;

    /**
     * @brief let a specific IMEngine instance longpress a candidate in its current lookup table.
     * @param id the IMEngine instance id.
     * @param index - the index in current page of the lookup table.
     */
    void longpress_candidate (int id, unsigned int index) const;

    /**
     * @brief Set IM data.
     * @param id the IMEngine instance id.
     * @param data - the buffer of data.
     * @param len  - the length of data.
     */
    void set_imdata (int id, const char *data, unsigned int len) const;

    /**
     * @brief Set autocapital type
     * @param id the IMEngine instance id.
     * @param mode autocapital type
     */
    void set_autocapital_type (int id, int mode) const;

    /**
     * @brief Set input hint.
     * @param id the IMEngine instance id.
     * @param input_hint - the input hint.
     */
    void set_input_hint (int id, unsigned int input_hint) const;

    /**
     * @brief Update BiDi direction. (Neutral / LTR / RTL)
     * @param id the IMEngine instance id.
     * @param bidi_direction - the BiDi direction.
     */
    void update_bidi_direction (int id, unsigned int bidi_direction) const;

    /**
     * @}
     */

protected:
    /**
     * @name Virtual protected methods.
     *
     * The following methods should be implemented by derivation classes.
     * these functions handle the real things.
     *
     * @{
     */

    /**
     * @brief show preedit string area for an IMEngine instance.
     * @param id the id of the IMEngine instance. It must have been focused in.
     */
    virtual void show_preedit_string (int id);

    /**
     * @brief show aux string area for an IMEngine instance.
     * @param id the id of the IMEngine instance. It must have been focused in.
     */
    virtual void show_aux_string     (int id);

    /**
     * @brief show lookup table area for an IMEngine instance.
     * @param id the id of the IMEngine instance. It must have been focused in.
     */
    virtual void show_lookup_table   (int id);

    /**
     * @brief hide preedit string area for an IMEngine instance.
     * @param id the id of the IMEngine instance. It must have been focused in.
     */
    virtual void hide_preedit_string (int id);

    /**
     * @brief hide aux string area for an IMEngine instance.
     * @param id the id of the IMEngine instance. It must have been focused in.
     */
    virtual void hide_aux_string     (int id);

    /**
     * @brief hide lookup table area for an IMEngine instance.
     * @param id the id of the IMEngine instance. It must have been focused in.
     */
    virtual void hide_lookup_table   (int id);

    /**
     * @brief update the position of preedit caret for an IMEngine instance.
     * @param id the id of the IMEngine instance. It must have been focused in.
     * @param caret the new caret position.
     */
    virtual void update_preedit_caret  (int id, int caret);

    /**
     * @brief update the content of preedit string for an IMEngine instance.
     * @param id the id of the IMEngine instance. It must have been focused in.
     * @param str the new content of preedit string.
     * @param attrs the string attributes.
     */
    virtual void update_preedit_string (int id, const WideString & str, const AttributeList & attrs, int caret);

    /**
     * @brief update the content of aux string for an IMEngine instance.
     * @param id the id of the IMEngine instance. It must have been focused in.
     * @param str the new content of aux string.
     * @param attrs the string attributes.
     */
    virtual void update_aux_string     (int id, const WideString & str, const AttributeList & attrs);

    /**
     * @brief update the content of lookup table for an IMEngine instance.
     * @param id the id of the IMEngine instance. It must have been focused in.
     * @param table the new lookup table.
     */
    virtual void update_lookup_table   (int id, const LookupTable & table);

    /**
     * @brief commit a string to client for an IMEngine instance.
     * @param id the id of the IMEngine instance to commit the string.
     * @param str the string to be committed.
     */
    virtual void commit_string         (int id, const WideString & str);

    /**
     * @brief forward a keyevent to the client of an IMEngine instance.
     * @param id the id of the IMEngine instance.
     * @param key the key event to be forwarded.
     */
    virtual void forward_key_event      (int id, const KeyEvent & key);

    /**
     * @brief register all the properties of an IMEngine instance into this FrontEnd.
     * @param id the id of the IMEngine instance. It must have been focused in.
     * @param properties the PropertyList contains all the properties of this IMEngine instance.
     */
    virtual void register_properties   (int id, const PropertyList & properties);

    /**
     * @brief update a property of an IMEngine instance.
     * @param id the id of the IMEngine instance. It must have been focused in.
     * @param property the Property to be updated.
     */
    virtual void update_property       (int id, const Property & property);

    /**
     * @brief generate a short beep.
     * @param id the id of the IMEngine instance. It must have been focused in.
     */
    virtual void beep                  (int id);

    /**
     * @brief start a Client Helper process.
     *
     * @param id the id of the IMEngine instance. It must have been focused in.
     * @param helper_uuid The UUID of the Helper object.
     */
    virtual void start_helper          (int id, const String &helper_uuid);

    /**
     * @brief stop a Client Helper process.
     *
     * @param id the id of the IMEngine instance. It must have been focused in.
     * @param helper_uuid The UUID of the Helper object.
     */
    virtual void stop_helper           (int id, const String &helper_uuid);

    /**
     * @brief send an events transaction to a Client Helper process.
     *
     * @param id the id of the IMEngine instance. It must have been focused in.
     * @param helper_uuid The UUID of the Helper object.
     * @param trans the transaction which contains events.
     */
    virtual void send_helper_event     (int id, const String &helper_uuid, const Transaction &trans);

    /**
     * @brief Retrieves context around the insertion point.
     *
     * Input methods typically want context in order to constrain
     * input text based on existing text;
     * this is important for languages such as Thai where
     * only some sequences of characters are allowed.
     *
     * Unlike other signal activation actions, this action will return the result
     * immediately.
     *
     * @param id            the id of the IMEngine instance. It must have been focused in.
     * @param text          location to store the context string around the insertion point.
     * @param cursor        location to store index of the insertion cursor within @text.
     * @param maxlen_before the maxmium length of context string to be retrieved
     *                      before the cursor; -1 means unlimited.
     * @param maxlen_after  the maxmium length of context string to be retrieved
     *                      after the cursor; -1 means unlimited.
     *
     * @return true if surrounding text was provided.
     */
    virtual bool get_surrounding_text  (int id, WideString &text, int &cursor, int maxlen_before, int maxlen_after);

    /**
     * @brief Ask the client to delete characters around the cursor position.
     *
     * In order to use this function, you should first call
     * get_surrounding_text () to get the current context, and
     * call this function immediately afterwards to make sure that you
     * know what you are deleting. You should also account for the fact
     * that even if the signal was handled, the input context might not
     * have deleted all the characters that were requested to be deleted.
     *
     * @param id     the id of the IMEngine instance. It must have been focused in.
     * @param offset offset from cursor position in chars;
     *               a negative value means start before the cursor.
     * @param len    number of characters to delete.
     *
     * @return true if the signal was handled.
     */
    virtual bool delete_surrounding_text  (int id, int offset, int len);

    /**
     * @brief Retrieves selection text.
     *
     * @param id            the id of the IMEngine instance. It must have been focused in.
     * @param text          location to store the context string selected.
     *
     * @return true if selection text was provided.
     */

    virtual bool get_selection  (int id, WideString &text);

    /**
     * @brief Ask the client to select around the cursor position.
     *
     * @param id     the id of the IMEngine instance. It must have been focused in.
     * @param start  start position in chars;
     * @param end    end position in chars.
     *
     * @return true if the signal was handled.
     */
    virtual bool set_selection  (int id, int start, int end);

    /**
     * @brief Request to expand candidate window.
     *
     * @param id the id of the IMEngine instance. It must have been focused in.
     */
    virtual void expand_candidate (int id);

    /**
     * @brief Request to contract candidate window.
     *
     * @param id the id of the IMEngine instance. It must have been focused in.
     */
    virtual void contract_candidate (int id);

    /**
     * @brief Request to set candidate style.
     *
     * @param id            the id of the IMEngine instance. It must have been focused in.
     * @param portrait_line the displayed line number for portrait.
     * @param mode          candidate window mode.
     */
    virtual void set_candidate_style (int                           id,
                                      ISF_CANDIDATE_PORTRAIT_LINE_T portrait_line,
                                      ISF_CANDIDATE_MODE_T          mode);

    /**
     * @brief Send a private command to an application.
     *
     * @param id     the id of the IMEngine instance. It must have been focused in.
     * @param command The private command
     */
    virtual void send_private_command  (int id, const String &command);

    /**
     * @}
     */

public:
    /**
     * @brief init the frontend.
     *
     * This method must be implemented by derivation classes.
     */
    virtual void init (int argc, char **argv) = 0;

    /**
     * @brief run the frontend.
     *
     * This method must be implemented by derivation classes.
     */
    virtual void run () = 0;
};

/** @} */

} // namespace scim

#endif //__SCIM_FRONTEND_H

/*
vi:ts=4:nowrap:ai:expandtab
*/
