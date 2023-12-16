/*
 * ISF(Input Service Framework)
 *
 * ISF is based on SCIM 1.4.7 and extended for supporting more mobile fitable.
 * Copyright (c) 2012-2014 Samsung Electronics Co., Ltd.
 *
 * Contact: Jihoon Kim <jihoon48.kim@samsung.com>, Haifeng Deng <haifeng.deng@samsung.com>
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

#define Uses_SCIM_BACKEND
#define Uses_SCIM_IMENGINE_MODULE
#define Uses_SCIM_HELPER_MODULE
#define Uses_SCIM_HOTKEY
#define Uses_SCIM_PANEL_CLIENT

#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/times.h>
#include <pthread.h>
#include <langinfo.h>
#include <unistd.h>
#include <wctype.h>

#include <Evas.h>
#include <Ecore_Evas.h>
#include <Ecore_Input.h>
#include <Ecore_X.h>
#include <X11/XKBlib.h>
#include <X11/keysym.h>
#include <X11/Xutil.h>
#include <glib.h>
#include <utilX.h>
#include <vconf.h>
#include <vconf-keys.h>

#include "scim_private.h"
#include "scim.h"
#include "isf_imf_context.h"
#include "isf_imf_control_ui.h"

#ifndef CODESET
# define CODESET "INVALID"
#endif

#define SHIFT_MODE_OFF  0xffe1
#define SHIFT_MODE_ON   0xffe2
#define SHIFT_MODE_LOCK 0xffe6
#define SHIFT_MODE_ENABLE 0x9fe7
#define SHIFT_MODE_DISABLE 0x9fe8

#define E_PROP_DEVICEMGR_INPUTWIN                       "DeviceMgr Input Window"

using namespace scim;

struct _EcoreIMFContextISFImpl {
    EcoreIMFContextISF      *parent;
    IMEngineInstancePointer  si;
    Ecore_X_Window           client_window;
    Evas                    *client_canvas;
    Ecore_IMF_Input_Mode     input_mode;
    WideString               preedit_string;
    AttributeList            preedit_attrlist;
    Ecore_IMF_Autocapital_Type autocapital_type;
    Ecore_IMF_Input_Hints    input_hint;
    Ecore_IMF_BiDi_Direction bidi_direction;
    void                    *imdata;
    int                      imdata_size;
    int                      preedit_caret;
    int                      cursor_x;
    int                      cursor_y;
    int                      cursor_top_y;
    int                      cursor_pos;
    bool                     use_preedit;
    bool                     is_on;
    bool                     shared_si;
    bool                     preedit_started;
    bool                     preedit_updating;
    bool                     need_commit_preedit;
    bool                     prediction_allow;
    int                      next_shift_status;
    int                      shift_mode_enabled;

    EcoreIMFContextISFImpl  *next;

    /* Constructor */
    _EcoreIMFContextISFImpl() : parent(NULL),
                                client_window(0),
                                client_canvas(NULL),
                                input_mode(ECORE_IMF_INPUT_MODE_FULL),
                                autocapital_type(ECORE_IMF_AUTOCAPITAL_TYPE_SENTENCE),
                                input_hint(ECORE_IMF_INPUT_HINT_NONE),
                                bidi_direction(ECORE_IMF_BIDI_DIRECTION_NEUTRAL),
                                imdata(NULL),
                                imdata_size(0),
                                preedit_caret(0),
                                cursor_x(0),
                                cursor_y(0),
                                cursor_top_y(0),
                                cursor_pos(-1),
                                use_preedit(true),
                                is_on(true),
                                shared_si(false),
                                preedit_started(false),
                                preedit_updating(false),
                                need_commit_preedit(false),
                                prediction_allow(false),
                                next_shift_status(0),
                                shift_mode_enabled(0),
                                next(NULL)
    {
    }
};

typedef enum {
    INPUT_LANG_URDU,
    INPUT_LANG_HINDI,
    INPUT_LANG_BENGALI_IN,
    INPUT_LANG_BENGALI_BD,
    INPUT_LANG_ASSAMESE,
    INPUT_LANG_PUNJABI,
    INPUT_LANG_NEPALI,
    INPUT_LANG_ORIYA,
    INPUT_LANG_MAITHILI,
    INPUT_LANG_ARMENIAN,
    INPUT_LANG_CN,
    INPUT_LANG_CN_HK,
    INPUT_LANG_CN_TW,
    INPUT_LANG_JAPANESE,
    INPUT_LANG_KHMER,
    INPUT_LANG_BURMESE,
    INPUT_LANG_OTHER
} Input_Language;

struct __Punctuations {
    const char *code;
    Input_Language lang;
    wchar_t punc_code;
};

static __Punctuations __punctuations [] = {
    { "ur_PK",  INPUT_LANG_URDU,        0x06D4 },
    { "hi_IN",  INPUT_LANG_HINDI,       0x0964 },
    { "bn_IN",  INPUT_LANG_BENGALI_IN,  0x0964 },
    { "bn_BD",  INPUT_LANG_BENGALI_BD,  0x0964 },
    { "as_IN",  INPUT_LANG_ASSAMESE,    0x0964 },
    { "pa_IN",  INPUT_LANG_PUNJABI,     0x0964 },
    { "ne_NP",  INPUT_LANG_NEPALI,      0x0964 },
    { "or_IN",  INPUT_LANG_ORIYA,       0x0964 },
    { "mai_IN", INPUT_LANG_MAITHILI,    0x0964 },
    { "hy_AM",  INPUT_LANG_ARMENIAN,    0x0589 },
    { "zh_CN",  INPUT_LANG_CN,          0x3002 },
    { "zh_HK",  INPUT_LANG_CN_HK,       0x3002 },
    { "zh_TW",  INPUT_LANG_CN_TW,       0x3002 },
    { "ja_JP",  INPUT_LANG_JAPANESE,    0x3002 },
    { "km_KH",  INPUT_LANG_KHMER,       0x17D4 },
    { "z1_MM",  INPUT_LANG_BURMESE,     0x104A },
};

/* Input Context handling functions. */
static EcoreIMFContextISFImpl *new_ic_impl              (EcoreIMFContextISF     *parent);
static void                    delete_ic_impl           (EcoreIMFContextISFImpl *impl);
static void                    delete_all_ic_impl       (void);

static EcoreIMFContextISF     *find_ic                  (int                     id);


/* private functions */
static void     panel_slot_reload_config                (int                     context);
static void     panel_slot_exit                         (int                     context);
static void     panel_slot_update_candidate_item_layout (int                     context,
                                                         const std::vector<uint32> &row_items);
static void     panel_slot_update_lookup_table_page_size(int                     context,
                                                         int                     page_size);
static void     panel_slot_lookup_table_page_up         (int                     context);
static void     panel_slot_lookup_table_page_down       (int                     context);
static void     panel_slot_trigger_property             (int                     context,
                                                         const String           &property);
static void     panel_slot_process_helper_event         (int                     context,
                                                         const String           &target_uuid,
                                                         const String           &helper_uuid,
                                                         const Transaction      &trans);
static void     panel_slot_move_preedit_caret           (int                     context,
                                                         int                     caret_pos);
static void     panel_slot_update_preedit_caret         (int                     context,
                                                         int                     caret);
static void     panel_slot_select_aux                   (int                     context,
                                                         int                     aux_index);
static void     panel_slot_select_candidate             (int                     context,
                                                         int                     cand_index);
static void     panel_slot_process_key_event            (int                     context,
                                                         const KeyEvent         &key);
static void     panel_slot_commit_string                (int                     context,
                                                         const WideString       &wstr);
static void     panel_slot_forward_key_event            (int                     context,
                                                         const KeyEvent         &key);
static void     panel_slot_request_help                 (int                     context);
static void     panel_slot_request_factory_menu         (int                     context);
static void     panel_slot_change_factory               (int                     context,
                                                         const String           &uuid);
static void     panel_slot_reset_keyboard_ise           (int                     context);
static void     panel_slot_update_keyboard_ise          (int                     context);
static void     panel_slot_show_preedit_string          (int                     context);
static void     panel_slot_hide_preedit_string          (int                     context);
static void     panel_slot_update_preedit_string        (int                     context,
                                                         const WideString       &str,
                                                         const AttributeList    &attrs,
                                                         int               caret);
static void     panel_slot_get_surrounding_text         (int                     context,
                                                         int                     maxlen_before,
                                                         int                     maxlen_after);
static void     panel_slot_delete_surrounding_text      (int                     context,
                                                         int                     offset,
                                                         int                     len);
static void     panel_slot_get_selection                (int                     context);
static void     panel_slot_set_selection                (int                     context,
                                                         int                     start,
                                                         int                     end);
static void     panel_slot_send_private_command         (int                     context,
                                                         const String           &command);
static void     panel_req_focus_in                      (EcoreIMFContextISF     *ic);
static void     panel_req_update_factory_info           (EcoreIMFContextISF     *ic);
static void     panel_req_update_spot_location          (EcoreIMFContextISF     *ic);
static void     panel_req_update_cursor_position        (EcoreIMFContextISF     *ic, int cursor_pos);
static void     panel_req_show_help                     (EcoreIMFContextISF     *ic);
static void     panel_req_show_factory_menu             (EcoreIMFContextISF     *ic);
static void     panel_req_set_input_hint                (EcoreIMFContextISF     *ic, int hint);
static void     panel_req_update_bidi_direction         (EcoreIMFContextISF     *ic, int bidi_direction);

/* Panel iochannel handler*/
static bool     panel_initialize                        (void);
static void     panel_finalize                          (void);
static Eina_Bool panel_iochannel_handler                (void                   *data,
                                                         Ecore_Fd_Handler       *fd_handler);

/* utility functions */
static bool     filter_hotkeys                          (EcoreIMFContextISF     *ic,
                                                         const KeyEvent         &key);
static bool     filter_keys                             (const char             *keyname,
                                                         std::vector <String>   &keys);

static void     turn_on_ic                              (EcoreIMFContextISF     *ic);
static void     turn_off_ic                             (EcoreIMFContextISF     *ic);
static void     set_ic_capabilities                     (EcoreIMFContextISF     *ic);

static void     initialize                              (void);
static void     finalize                                (void);

static void     open_next_factory                       (EcoreIMFContextISF     *ic);
static void     open_previous_factory                   (EcoreIMFContextISF     *ic);
static void     open_specific_factory                   (EcoreIMFContextISF     *ic,
                                                         const String           &uuid);
static void     initialize_modifier_bits                (Display                *display);
static unsigned int scim_x11_keymask_scim_to_x11        (Display                *display,
                                                         uint16                  scimkeymask);
static XKeyEvent createKeyEvent                         (bool                    press,
                                                         int                     keycode,
                                                         int                     modifiers,
                                                         bool                    fake);
static void     send_x_key_event                        (const KeyEvent         &key,
                                                         bool                    fake);
static int      _keyname_to_keycode                     (const char             *keyname);
static void     _hide_preedit_string                    (int                     context,
                                                         bool                    update_preedit);

static void     attach_instance                         (const IMEngineInstancePointer &si);

/* slot functions */
static void     slot_show_preedit_string                (IMEngineInstanceBase   *si);
static void     slot_show_aux_string                    (IMEngineInstanceBase   *si);
static void     slot_show_lookup_table                  (IMEngineInstanceBase   *si);

static void     slot_hide_preedit_string                (IMEngineInstanceBase   *si);
static void     slot_hide_aux_string                    (IMEngineInstanceBase   *si);
static void     slot_hide_lookup_table                  (IMEngineInstanceBase   *si);

static void     slot_update_preedit_caret               (IMEngineInstanceBase   *si,
                                                         int                     caret);
static void     slot_update_preedit_string              (IMEngineInstanceBase   *si,
                                                         const WideString       &str,
                                                         const AttributeList    &attrs,
                                                         int               caret);
static void     slot_update_aux_string                  (IMEngineInstanceBase   *si,
                                                         const WideString       &str,
                                                         const AttributeList    &attrs);
static void     slot_commit_string                      (IMEngineInstanceBase   *si,
                                                         const WideString       &str);
static void     slot_forward_key_event                  (IMEngineInstanceBase   *si,
                                                         const KeyEvent         &key);
static void     slot_update_lookup_table                (IMEngineInstanceBase   *si,
                                                         const LookupTable      &table);

static void     slot_register_properties                (IMEngineInstanceBase   *si,
                                                         const PropertyList     &properties);
static void     slot_update_property                    (IMEngineInstanceBase   *si,
                                                         const Property         &property);
static void     slot_beep                               (IMEngineInstanceBase   *si);
static void     slot_start_helper                       (IMEngineInstanceBase   *si,
                                                         const String           &helper_uuid);
static void     slot_stop_helper                        (IMEngineInstanceBase   *si,
                                                         const String           &helper_uuid);
static void     slot_send_helper_event                  (IMEngineInstanceBase   *si,
                                                         const String           &helper_uuid,
                                                         const Transaction      &trans);
static bool     slot_get_surrounding_text               (IMEngineInstanceBase   *si,
                                                         WideString             &text,
                                                         int                    &cursor,
                                                         int                     maxlen_before,
                                                         int                     maxlen_after);
static bool     slot_delete_surrounding_text            (IMEngineInstanceBase   *si,
                                                         int                     offset,
                                                         int                     len);
static bool     slot_get_selection                      (IMEngineInstanceBase   *si,
                                                         WideString             &text);
static bool     slot_set_selection                      (IMEngineInstanceBase   *si,
                                                         int                     start,
                                                         int                     end);
static void     slot_expand_candidate                   (IMEngineInstanceBase   *si);
static void     slot_contract_candidate                 (IMEngineInstanceBase   *si);

static void     slot_set_candidate_style                (IMEngineInstanceBase   *si,
                                                         ISF_CANDIDATE_PORTRAIT_LINE_T portrait_line,
                                                         ISF_CANDIDATE_MODE_T    mode);
static void     slot_send_private_command               (IMEngineInstanceBase   *si,
                                                         const String           &command);

static void     reload_config_callback                  (const ConfigPointer    &config);

static void     fallback_commit_string_cb               (IMEngineInstanceBase   *si,
                                                         const WideString       &str);

/* Local variables declaration */
static String                                           _language;
static EcoreIMFContextISFImpl                          *_used_ic_impl_list          = 0;
static EcoreIMFContextISFImpl                          *_free_ic_impl_list          = 0;
static EcoreIMFContextISF                              *_ic_list                    = 0;

static KeyboardLayout                                   _keyboard_layout            = SCIM_KEYBOARD_Default;
static int                                              _valid_key_mask             = SCIM_KEY_AllMasks;

static FrontEndHotkeyMatcher                            _frontend_hotkey_matcher;
static IMEngineHotkeyMatcher                            _imengine_hotkey_matcher;

static IMEngineInstancePointer                          _default_instance;

static ConfigPointer                                    _config;
static Connection                                       _config_connection;
static BackEndPointer                                   _backend;

static EcoreIMFContextISF                              *_focused_ic                 = 0;

static bool                                             _scim_initialized           = false;

static int                                              _instance_count             = 0;
static int                                              _context_count              = 0;

static IMEngineFactoryPointer                           _fallback_factory;
static IMEngineInstancePointer                          _fallback_instance;
PanelClient                                             _panel_client;
static int                                              _panel_client_id            = 0;

static bool                                             _panel_initialized          = false;
static int                                              _active_helper_option       = 0;

static Ecore_Fd_Handler                                *_panel_iochannel_read_handler = 0;
static Ecore_Fd_Handler                                *_panel_iochannel_err_handler  = 0;

static Ecore_X_Window                                   _input_win                  = 0;
static Ecore_X_Window                                   _client_window              = 0;
static Ecore_Event_Filter                              *_ecore_event_filter_handler = NULL;

static bool                                             _on_the_spot                = true;
static bool                                             _shared_input_method        = false;
static bool                                             _change_keyboard_mode_by_touch = false;
static bool                                             _change_keyboard_mode_by_focus_move = false;
static bool                                             _hide_ise_based_on_focus    = false;
static bool                                             _support_hw_keyboard_mode   = false;

static double                                           space_key_time              = 0.0;

static Eina_Bool                                        autoperiod_allow            = EINA_FALSE;
static Eina_Bool                                        autocap_allow               = EINA_FALSE;

static bool                                             _x_key_event_is_valid       = false;

static Ecore_Timer                                     *_click_timer                = NULL;

static Input_Language                                   input_lang                  = INPUT_LANG_OTHER;

static Display                                         *__current_display      = 0;
static int                                              __current_alt_mask     = Mod1Mask;
static int                                              __current_meta_mask    = 0;
static int                                              __current_super_mask   = 0;
static int                                              __current_hyper_mask   = 0;
static int                                              __current_numlock_mask = Mod2Mask;

static std::vector <String>                             hide_ise_keys;
static std::vector <String>                             ignore_keys;

extern Ecore_IMF_Input_Panel_State                      input_panel_state;
extern Ecore_IMF_Input_Panel_State                      notified_state;
extern Ecore_IMF_Context                               *input_panel_ctx;

// A hack to shutdown the immodule cleanly even if im_module_exit () is not called when exiting.
class FinalizeHandler
{
public:
    FinalizeHandler () {
        SCIM_DEBUG_FRONTEND(1) << "FinalizeHandler::FinalizeHandler ()\n";
    }
    ~FinalizeHandler () {
        SCIM_DEBUG_FRONTEND(1) << "FinalizeHandler::~FinalizeHandler ()\n";
        isf_imf_context_shutdown ();
    }
};

static FinalizeHandler                                  _finalize_handler;

EAPI EcoreIMFContextISF *
get_focused_ic ()
{
    return _focused_ic;
}

EAPI int
get_panel_client_id (void)
{
    return _panel_client_id;
}

/* Function Implementations */
static EcoreIMFContextISFImpl *
new_ic_impl (EcoreIMFContextISF *parent)
{
    EcoreIMFContextISFImpl *impl = NULL;

    if (_free_ic_impl_list != NULL) {
        impl = _free_ic_impl_list;
        _free_ic_impl_list = _free_ic_impl_list->next;
    } else {
        impl = new EcoreIMFContextISFImpl;
        if (impl == NULL)
            return NULL;
    }

    impl->next = _used_ic_impl_list;
    _used_ic_impl_list = impl;

    impl->parent = parent;

    return impl;
}

static void
delete_ic_impl (EcoreIMFContextISFImpl *impl)
{
    EcoreIMFContextISFImpl *rec = _used_ic_impl_list, *last = 0;

    for (; rec != 0; last = rec, rec = rec->next) {
        if (rec == impl) {
            if (last != 0)
                last->next = rec->next;
            else
                _used_ic_impl_list = rec->next;

            rec->next = _free_ic_impl_list;
            _free_ic_impl_list = rec;

            if (rec->imdata) {
                free (rec->imdata);
                rec->imdata = NULL;
            }

            rec->imdata_size = 0;
            rec->parent = 0;
            rec->si.reset ();
            rec->client_window = 0;
            rec->preedit_string = WideString ();
            rec->preedit_attrlist.clear ();

            return;
        }
    }
}

static void
delete_all_ic_impl (void)
{
    EcoreIMFContextISFImpl *it = _used_ic_impl_list;

    while (it != 0) {
        _used_ic_impl_list = it->next;
        delete it;
        it = _used_ic_impl_list;
    }

    it = _free_ic_impl_list;
    while (it != 0) {
        _free_ic_impl_list = it->next;
        delete it;
        it = _free_ic_impl_list;
    }
}

static EcoreIMFContextISF *
find_ic (int id)
{
    EcoreIMFContextISFImpl *rec = _used_ic_impl_list;

    while (rec != 0) {
        if (rec->parent && rec->parent->id == id)
            return rec->parent;
        rec = rec->next;
    }

    return 0;
}

static bool
check_valid_ic (EcoreIMFContextISF * ic)
{
    if (ic && ic->impl && ic->ctx)
        return true;
    else
        return false;
}

static Eina_Bool
_panel_show (void *data)
{
    Ecore_IMF_Context *active_ctx = get_using_ic (ECORE_IMF_INPUT_PANEL_STATE_EVENT, ECORE_IMF_INPUT_PANEL_STATE_SHOW);

    if (!active_ctx)
        return ECORE_CALLBACK_CANCEL;

    ecore_imf_context_input_panel_show (active_ctx);

    return ECORE_CALLBACK_CANCEL;
}

static Eina_Bool
_click_check (void *data)
{
    _click_timer = NULL;
    return ECORE_CALLBACK_CANCEL;
}

static Eina_Bool
_key_down_cb (void *data, int type, void *event)
{
    SCIM_DEBUG_FRONTEND(1) << __FUNCTION__ << "...\n";

    Eina_Bool ret = EINA_FALSE;
    Ecore_Event_Key *ev = (Ecore_Event_Key *)event;
    Ecore_IMF_Context *active_ctx = get_using_ic (ECORE_IMF_INPUT_PANEL_STATE_EVENT, ECORE_IMF_INPUT_PANEL_STATE_SHOW);
    if (!ev || !ev->keyname || !active_ctx) return EINA_TRUE;

    EcoreIMFContextISF *ic = (EcoreIMFContextISF*) ecore_imf_context_data_get (active_ctx);
    if (!ic) return EINA_TRUE;

    Ecore_X_Window client_win = client_window_id_get (active_ctx);
    Ecore_X_Window focus_win = ecore_x_window_focus_get ();

    if ((client_win == focus_win) && ((_hide_ise_based_on_focus) ? (_focused_ic != 0):(true))) {
        if (filter_keys (ev->keyname, hide_ise_keys)) {
            if (get_keyboard_mode () == TOOLBAR_HELPER_MODE) {
                if (ecore_imf_context_input_panel_state_get (active_ctx) == ECORE_IMF_INPUT_PANEL_STATE_HIDE)
                    return EINA_TRUE;
            }
            LOGD ("%s key is pressed.\n", ev->keyname);
            if (_active_helper_option & ISM_HELPER_PROCESS_KEYBOARD_KEYEVENT) {
                KeyEvent key;
                scim_string_to_key (key, ev->key);
                LOGD ("process hide_ise_key_event to handle it in the helper: %s", ev->keyname);
                void *pvoid = &ret;
                _panel_client.prepare (ic->id);
                _panel_client.process_key_event (key, (int*) pvoid);
                _panel_client.send ();
            }
            if (ret) {
                return EINA_FALSE;
            }
            else if (ecore_imf_context_input_panel_state_get (active_ctx) == ECORE_IMF_INPUT_PANEL_STATE_SHOW) {
                return ECORE_CALLBACK_DONE;
            }
        }
    }
    return EINA_TRUE;
}

static Eina_Bool
_key_up_cb (void *data, int type, void *event)
{
    SCIM_DEBUG_FRONTEND(1) << __FUNCTION__ << "...\n";

    unsigned int val = 0;
    Eina_Bool ret = EINA_FALSE;
    Ecore_Event_Key *ev = (Ecore_Event_Key *)event;
    Ecore_IMF_Context *active_ctx = get_using_ic (ECORE_IMF_INPUT_PANEL_STATE_EVENT, ECORE_IMF_INPUT_PANEL_STATE_SHOW);
    if (!ev || !ev->keyname || !active_ctx) return EINA_TRUE;

    EcoreIMFContextISF *ic = (EcoreIMFContextISF*) ecore_imf_context_data_get (active_ctx);
    if (!ic) return EINA_TRUE;

    Ecore_X_Window client_win = client_window_id_get (active_ctx);
    Ecore_X_Window focus_win = ecore_x_window_focus_get ();

    if ((client_win == focus_win) && ((_hide_ise_based_on_focus) ? (_focused_ic != 0):(true))) {
        if (filter_keys (ev->keyname, hide_ise_keys)) {
            if (get_keyboard_mode () == TOOLBAR_HELPER_MODE) {
                if (ecore_imf_context_input_panel_state_get (active_ctx) == ECORE_IMF_INPUT_PANEL_STATE_HIDE)
                    return EINA_TRUE;
            }
            LOGD ("%s key is released.\n", ev->keyname);
            if (_active_helper_option & ISM_HELPER_PROCESS_KEYBOARD_KEYEVENT) {
                KeyEvent key;
                scim_string_to_key (key, ev->key);
                key.mask = SCIM_KEY_ReleaseMask;
                key.mask &= _valid_key_mask;
                LOGD ("process hide_ise_key_event to handle it in the helper: %s", ev->keyname);
                void *pvoid = &ret;
                _panel_client.prepare (ic->id);
                _panel_client.process_key_event (key, (int*) pvoid);
                _panel_client.send ();
            }
            if (ret) {
                return EINA_FALSE;
            }
            else if (ecore_imf_context_input_panel_state_get (active_ctx) == ECORE_IMF_INPUT_PANEL_STATE_SHOW) {
                isf_imf_context_reset (active_ctx);
                isf_imf_context_input_panel_instant_hide (active_ctx);
                return ECORE_CALLBACK_DONE;
            }
        }
    }

    if (_support_hw_keyboard_mode && !strcmp (ev->keyname, "XF86MenuKB")) {
        if (ecore_imf_context_input_panel_state_get (active_ctx) == ECORE_IMF_INPUT_PANEL_STATE_SHOW) {
            ecore_imf_context_input_panel_hide (active_ctx);
        } else {
            if (_click_timer == NULL) {
                if (get_keyboard_mode () == TOOLBAR_KEYBOARD_MODE) {
                    ecore_x_window_prop_card32_set (_input_win, ecore_x_atom_get (PROP_X_EXT_KEYBOARD_EXIST), &val, 1);
                    ecore_timer_add (0.1, _panel_show, NULL);
                } else {
                    ecore_imf_context_input_panel_show (active_ctx);
                }
                _click_timer = ecore_timer_add (0.4, _click_check, NULL);
            } else {
                LOGD ("Skip toggle key input");
                ecore_timer_del (_click_timer);
                _click_timer = ecore_timer_add (0.4, _click_check, NULL);
            }
        }
    }

    return EINA_TRUE;
}

static Eina_Bool
_ecore_event_filter_cb (void *data, void *loop_data, int type, void *event)
{
    if (type == ECORE_EVENT_KEY_DOWN) {
        return _key_down_cb (data, type, event);
    }
    else if (type == ECORE_EVENT_KEY_UP) {
        return _key_up_cb (data, type, event);
    }

    return EINA_TRUE;
}

void
register_key_handler ()
{
    SCIM_DEBUG_FRONTEND(1) << __FUNCTION__ << "...\n";

    if (!_ecore_event_filter_handler)
        _ecore_event_filter_handler = ecore_event_filter_add (NULL, _ecore_event_filter_cb, NULL, NULL);
}

void
unregister_key_handler ()
{
    SCIM_DEBUG_FRONTEND(1) << __FUNCTION__ << "...\n";

    if (_ecore_event_filter_handler) {
        ecore_event_filter_del (_ecore_event_filter_handler);
        _ecore_event_filter_handler = NULL;
    }
}

static void
set_prediction_allow (Ecore_IMF_Context *ctx, bool prediction)
{
    EcoreIMFContextISF *context_scim = (EcoreIMFContextISF *)ecore_imf_context_data_get (ctx);

    if (context_scim && context_scim->impl && context_scim == _focused_ic) {
        _panel_client.prepare (context_scim->id);
        context_scim->impl->si->set_prediction_allow (prediction);
        _panel_client.send ();
    }
}

static Eina_Bool
check_symbol (Eina_Unicode ucode, Eina_Unicode symbols[], int symbol_num)
{
    for (int i = 0; i < symbol_num; i++) {
        // Check symbol
        if (ucode == symbols[i])
            return EINA_TRUE;
    }

    return EINA_FALSE;
}

static Eina_Bool
check_except_autocapital (Eina_Unicode *ustr, int cursor_pos)
{
    const char *except_str[] = {"e.g.", "E.g."};
    unsigned int i = 0, j = 0, len = 0;
    for (i = 0; i < (sizeof (except_str) / sizeof (except_str[0])); i++) {
        len = strlen (except_str[i]);
        if (cursor_pos < (int)len)
            continue;

        for (j = len; j > 0; j--) {
            if (ustr[cursor_pos-j] != except_str[i][len-j])
                break;
        }

        if (j == 0) return EINA_TRUE;
    }

    return EINA_FALSE;
}

static Eina_Bool
check_space_symbol (Eina_Unicode uchar)
{
    Eina_Unicode space_symbols[] = {' ', 0x00A0 /* no-break space */, 0x3000 /* ideographic space */};
    const int symbol_num = sizeof (space_symbols) / sizeof (space_symbols[0]);

    return check_symbol (uchar, space_symbols, symbol_num);
}

static void
autoperiod_insert (Ecore_IMF_Context *ctx)
{
    char *plain_str = NULL;
    int cursor_pos = 0;
    Eina_Unicode *ustr = NULL;
    Ecore_IMF_Event_Delete_Surrounding ev;
    char *fullstop_mark = NULL;
    int del_chars = 0;

    if (autoperiod_allow == EINA_FALSE)
        return;

    if (!ctx) return;

    Ecore_IMF_Input_Panel_Layout layout = ecore_imf_context_input_panel_layout_get (ctx);
    if (layout != ECORE_IMF_INPUT_PANEL_LAYOUT_NORMAL)
        return;

    if ((ecore_time_get () - space_key_time) > DOUBLE_SPACE_INTERVAL)
        goto done;

    ecore_imf_context_surrounding_get (ctx, &plain_str, &cursor_pos);
    if (!plain_str) goto done;

    // Convert string from UTF-8 to unicode
    ustr = eina_unicode_utf8_to_unicode (plain_str, NULL);
    if (!ustr) goto done;

    if (cursor_pos < 2) goto done;

    if (check_space_symbol (ustr[cursor_pos-1])) {
        // any character + press space key twice in short time
        if (!(iswpunct (ustr[cursor_pos-2]) || check_space_symbol (ustr[cursor_pos-2]))) {
            del_chars = 1;
        }
        // any character + space + press space key twice in short time
        else if (cursor_pos >= 3 &&
                check_space_symbol (ustr[cursor_pos-2]) &&
                !(iswpunct (ustr[cursor_pos-3]) || check_space_symbol (ustr[cursor_pos-3]))) {
            del_chars = 2;
        }

        if (del_chars > 0) {
            ev.ctx = ctx;
            ev.n_chars = del_chars;
            ev.offset = del_chars * -1;
            ecore_imf_context_delete_surrounding_event_add (ctx, ev.offset, ev.n_chars);
            ecore_imf_context_event_callback_call (ctx, ECORE_IMF_CALLBACK_DELETE_SURROUNDING, &ev);

            // insert regional fullstop mark
            if (input_lang == INPUT_LANG_OTHER) {
                fullstop_mark = strdup (".");
            }
            else {
                wchar_t wbuf[2] = {0};
                wbuf[0] = __punctuations[input_lang].punc_code;

                WideString wstr = WideString (wbuf);
                fullstop_mark = strdup (utf8_wcstombs (wstr).c_str ());
            }

            ecore_imf_context_commit_event_add (ctx, fullstop_mark);
            ecore_imf_context_event_callback_call (ctx, ECORE_IMF_CALLBACK_COMMIT, (void *)fullstop_mark);

            if (fullstop_mark) {
                free (fullstop_mark);
                fullstop_mark = NULL;
            }
        }
    }

done:
    if (plain_str) free (plain_str);
    if (ustr) free (ustr);
    space_key_time = ecore_time_get ();
}

static Eina_Bool
analyze_surrounding_text (Ecore_IMF_Context *ctx)
{
    char *plain_str = NULL;
    Eina_Unicode puncs[] = {'\n','.', '!', '?', 0x00BF /* ¿ */, 0x00A1 /* ¡ */,
                            0x3002 /* 。 */, 0x06D4 /* Urdu */, 0x0964 /* Hindi */,
                            0x0589 /* Armenian */, 0x17D4 /* Khmer */, 0x104A /* Myanmar */};
    Eina_Unicode *ustr = NULL;
    Eina_Bool ret = EINA_FALSE;
    Eina_Bool detect_space = EINA_FALSE;
    int cursor_pos = 0;
    int i = 0;
    const int punc_num = sizeof (puncs) / sizeof (puncs[0]);
    EcoreIMFContextISF *context_scim;

    if (!ctx) return EINA_FALSE;
    context_scim = (EcoreIMFContextISF *)ecore_imf_context_data_get (ctx);
    if (!context_scim || !context_scim->impl) return EINA_FALSE;

    switch (context_scim->impl->autocapital_type) {
        case ECORE_IMF_AUTOCAPITAL_TYPE_NONE:
            return EINA_FALSE;
        case ECORE_IMF_AUTOCAPITAL_TYPE_ALLCHARACTER:
            return EINA_TRUE;
        default:
            break;
    }

    if (context_scim->impl->cursor_pos == 0)
        return EINA_TRUE;

    if (context_scim->impl->preedit_updating)
        return EINA_FALSE;

    ecore_imf_context_surrounding_get (ctx, &plain_str, &cursor_pos);
    if (!plain_str) goto done;

    if (cursor_pos == 0) {
        ret = EINA_TRUE;
        goto done;
    }

    // Convert string from UTF-8 to unicode
    ustr = eina_unicode_utf8_to_unicode (plain_str, NULL);
    if (!ustr) goto done;

    if (eina_unicode_strlen (ustr) < (size_t)cursor_pos) goto done;

    if (cursor_pos >= 1) {
        if (context_scim->impl->autocapital_type == ECORE_IMF_AUTOCAPITAL_TYPE_WORD) {
            // Check space or no-break space
            if (check_space_symbol (ustr[cursor_pos-1])) {
                ret = EINA_TRUE;
                goto done;
            }
        }

        // Check paragraph separator <PS> or carriage return  <br>
        if ((ustr[cursor_pos-1] == 0x2029) || (ustr[cursor_pos-1] == '\n')) {
            ret = EINA_TRUE;
            goto done;
        }

        for (i = cursor_pos; i > 0; i--) {
            // Check space or no-break space
            if (check_space_symbol (ustr[i-1])) {
                detect_space = EINA_TRUE;
                continue;
            }

            // Check punctuation and following the continuous space(s)
            if (detect_space && check_symbol (ustr[i-1], puncs, punc_num)) {
                if (check_except_autocapital (ustr, i))
                    ret = EINA_FALSE;
                else
                    ret = EINA_TRUE;

                goto done;
            }
            else {
                ret = EINA_FALSE;
                goto done;
            }
        }

        if ((i == 0) && (detect_space == EINA_TRUE)) {
            // continuous space(s) without any character
            ret = EINA_TRUE;
            goto done;
        }
    }

done:
    if (ustr) free (ustr);
    if (plain_str) free (plain_str);

    return ret;
}

EAPI Eina_Bool
caps_mode_check (Ecore_IMF_Context *ctx, Eina_Bool force, Eina_Bool noti)
{
    Eina_Bool uppercase;
    EcoreIMFContextISF *context_scim;

    if (get_keyboard_mode () == TOOLBAR_KEYBOARD_MODE) return EINA_FALSE;

    if (!ctx) return EINA_FALSE;
    context_scim = (EcoreIMFContextISF *)ecore_imf_context_data_get (ctx);

    if (!context_scim || !context_scim->impl)
        return EINA_FALSE;

    if (context_scim->impl->next_shift_status == SHIFT_MODE_LOCK) return EINA_TRUE;

    Ecore_IMF_Input_Panel_Layout layout = ecore_imf_context_input_panel_layout_get (ctx);
    if (layout != ECORE_IMF_INPUT_PANEL_LAYOUT_NORMAL)
        return EINA_FALSE;

    // Check autocapital type
    if (ecore_imf_context_input_panel_caps_lock_mode_get (ctx)) {
        uppercase = EINA_TRUE;
    } else {
        if (autocap_allow == EINA_FALSE)
            return EINA_FALSE;

        if (analyze_surrounding_text (ctx)) {
            uppercase = EINA_TRUE;
        } else {
            uppercase = EINA_FALSE;
        }
    }

    if (force) {
        context_scim->impl->next_shift_status = uppercase ? SHIFT_MODE_ON : SHIFT_MODE_OFF;
        if (noti)
            isf_imf_context_input_panel_caps_mode_set (ctx, uppercase);
    } else {
        if (context_scim->impl->next_shift_status != (uppercase ? SHIFT_MODE_ON : SHIFT_MODE_OFF)) {
            context_scim->impl->next_shift_status = uppercase ? SHIFT_MODE_ON : SHIFT_MODE_OFF;
            if (noti)
                isf_imf_context_input_panel_caps_mode_set (ctx, uppercase);
        }
    }

    return uppercase;
}

static void
window_to_screen_geometry_get (Ecore_X_Window client_win, int *x, int *y)
{
    Ecore_X_Window root_window, win;
    int win_x, win_y;
    int sum_x = 0, sum_y = 0;

    root_window = ecore_x_window_root_get (client_win);
    win = client_win;

    while (root_window != win) {
        ecore_x_window_geometry_get (win, &win_x, &win_y, NULL, NULL);
        sum_x += win_x;
        sum_y += win_y;
        win = ecore_x_window_parent_get (win);
    }

    if (x)
        *x = sum_x;
    if (y)
        *y = sum_y;
}

static unsigned int
_ecore_imf_modifier_to_scim_mask (unsigned int modifiers)
{
   unsigned int mask = 0;

   /**< "Control" is pressed */
   if (modifiers & ECORE_IMF_KEYBOARD_MODIFIER_CTRL)
     mask |= SCIM_KEY_ControlMask;

   /**< "Alt" is pressed */
   if (modifiers & ECORE_IMF_KEYBOARD_MODIFIER_ALT)
     mask |= SCIM_KEY_AltMask;

   /**< "Shift" is pressed */
   if (modifiers & ECORE_IMF_KEYBOARD_MODIFIER_SHIFT)
     mask |= SCIM_KEY_ShiftMask;

   /**< "Win" (between "Ctrl" and "Alt") is pressed */
   if (modifiers & ECORE_IMF_KEYBOARD_MODIFIER_WIN)
     mask |= SCIM_KEY_SuperMask;

   /**< "AltGr" is pressed */
   if (modifiers & ECORE_IMF_KEYBOARD_MODIFIER_ALTGR)
     mask |= SCIM_KEY_Mod5Mask;

   return mask;
}

static unsigned int
_ecore_imf_lock_to_scim_mask (unsigned int locks)
{
   unsigned int mask = 0;

   if (locks & ECORE_IMF_KEYBOARD_LOCK_CAPS)
     mask |= SCIM_KEY_CapsLockMask;

   if (locks & ECORE_IMF_KEYBOARD_LOCK_NUM)
     mask |= SCIM_KEY_NumLockMask;

   return mask;
}

static void
get_input_language ()
{
    char *input_lang_str = vconf_get_str (VCONFKEY_ISF_INPUT_LANGUAGE);
    if (!input_lang_str) return;

    input_lang = INPUT_LANG_OTHER;

    for (unsigned int i = 0; i < (sizeof (__punctuations) / sizeof (__punctuations[0])); i++) {
        if (strcmp (input_lang_str, __punctuations[i].code) == 0) {
            input_lang = __punctuations[i].lang;
            break;
        }
    }

    free (input_lang_str);
}

static void autoperiod_allow_changed_cb (keynode_t *key, void* data)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";

    autoperiod_allow = vconf_keynode_get_bool (key);
}

static void autocapital_allow_changed_cb (keynode_t *key, void* data)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";

    autocap_allow = vconf_keynode_get_bool (key);
}

static void input_language_changed_cb (keynode_t *key, void* data)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";

    get_input_language ();
}

EAPI void context_scim_imdata_get (Ecore_IMF_Context *ctx, void* data, int* length)
{
    EcoreIMFContextISF *context_scim = (EcoreIMFContextISF *)ecore_imf_context_data_get (ctx);

    SCIM_DEBUG_FRONTEND(1) << __FUNCTION__ << "...\n";

    if (context_scim && context_scim->impl) {
        if (data && context_scim->impl->imdata)
            memcpy (data, context_scim->impl->imdata, context_scim->impl->imdata_size);

        if (length)
            *length = context_scim->impl->imdata_size;
    }
}

EAPI void
imengine_layout_set (Ecore_IMF_Context *ctx, Ecore_IMF_Input_Panel_Layout layout)
{
    EcoreIMFContextISF *context_scim = (EcoreIMFContextISF *)ecore_imf_context_data_get (ctx);

    if (context_scim && context_scim->impl && context_scim == _focused_ic) {
        _panel_client.prepare (context_scim->id);
        context_scim->impl->si->set_layout (layout);
        _panel_client.send ();
    }
}

/* Public functions */
/**
 * isf_imf_context_new
 *
 * This function will be called by Ecore IMF.
 * Create a instance of type EcoreIMFContextISF.
 *
 * Return value: A pointer to the newly created EcoreIMFContextISF instance
 */
EAPI EcoreIMFContextISF *
isf_imf_context_new (void)
{
    SCIM_DEBUG_FRONTEND(1) << __FUNCTION__ << "...\n";

    int val;

    EcoreIMFContextISF *context_scim = new EcoreIMFContextISF;
    if (context_scim == NULL) {
        std::cerr << "memory allocation failed in " << __FUNCTION__ << "\n";
        return NULL;
    }

    if (_context_count == 0) {
        _context_count = getpid () % 50000;
    }
    context_scim->id = _context_count++;

    if (!_scim_initialized) {
        ecore_x_init (NULL);
        initialize ();
        _scim_initialized = true;
        isf_imf_input_panel_init ();

        /* get autoperiod allow vconf value */
        if (vconf_get_bool (VCONFKEY_AUTOPERIOD_ALLOW_BOOL, &val) == 0) {
            if (val == EINA_TRUE)
                autoperiod_allow = EINA_TRUE;
        }

        vconf_notify_key_changed (VCONFKEY_AUTOPERIOD_ALLOW_BOOL, autoperiod_allow_changed_cb, NULL);

        /* get autocapital allow vconf value */
        if (vconf_get_bool (VCONFKEY_AUTOCAPITAL_ALLOW_BOOL, &val) == 0) {
            if (val == EINA_TRUE)
                autocap_allow = EINA_TRUE;
        }

        vconf_notify_key_changed (VCONFKEY_AUTOCAPITAL_ALLOW_BOOL, autocapital_allow_changed_cb, NULL);

        /* get input language vconf value */
        get_input_language ();

        vconf_notify_key_changed (VCONFKEY_ISF_INPUT_LANGUAGE, input_language_changed_cb, NULL);

        scim_split_string_list (hide_ise_keys, _config->read (String (SCIM_CONFIG_HOTKEYS_FRONTEND_HIDE_ISE), String ("")), ',');

        scim_split_string_list (ignore_keys, _config->read (String (SCIM_CONFIG_HOTKEYS_FRONTEND_IGNORE_KEY), String ("")), ',');
    }

    return context_scim;
}

/**
 * isf_imf_context_shutdown
 *
 * It will be called when the scim im module is unloaded by ecore. It will do some
 * cleanup job.
 */
EAPI void
isf_imf_context_shutdown (void)
{
    SCIM_DEBUG_FRONTEND(1) << __FUNCTION__ << "...\n";
    ConfigBase::set (0);
    _default_instance.reset();
    if (_scim_initialized) {
        _scim_initialized = false;

        LOGD ("immodule shutdown\n");

        vconf_ignore_key_changed (VCONFKEY_AUTOPERIOD_ALLOW_BOOL, autoperiod_allow_changed_cb);
        vconf_ignore_key_changed (VCONFKEY_AUTOCAPITAL_ALLOW_BOOL, autocapital_allow_changed_cb);
        vconf_ignore_key_changed (VCONFKEY_ISF_INPUT_LANGUAGE, input_language_changed_cb);

        isf_imf_input_panel_shutdown ();
        finalize ();
        ecore_x_shutdown ();
    }
}

EAPI void
isf_imf_context_add (Ecore_IMF_Context *ctx)
{
    SCIM_DEBUG_FRONTEND(1) << __FUNCTION__ << "...\n";

    EcoreIMFContextISF *context_scim = (EcoreIMFContextISF*)ecore_imf_context_data_get (ctx);

    if (!context_scim) return;

    context_scim->impl = NULL;

    if (_backend.null ())
        return;

    IMEngineInstancePointer si;

    // Use the default instance if "shared input method" mode is enabled.
    if (_shared_input_method && !_default_instance.null ()) {
        si = _default_instance;
        SCIM_DEBUG_FRONTEND(2) << "use default instance: " << si->get_id () << " " << si->get_factory_uuid () << "\n";
    }

    // Not in "shared input method" mode, or no default instance, create an instance.
    if (si.null ()) {
        IMEngineFactoryPointer factory = _backend->get_default_factory (_language, "UTF-8");
        if (factory.null ()) return;
        si = factory->create_instance ("UTF-8", _instance_count++);
        if (si.null ()) return;
        LOGD ("create_instance: %s",si->get_factory_uuid ().c_str ());
        attach_instance (si);
        SCIM_DEBUG_FRONTEND(2) << "create new instance: " << si->get_id () << " " << si->get_factory_uuid () << "\n";
    }

    // If "shared input method" mode is enabled, and there is no default instance,
    // then store this instance as default one.
    if (_shared_input_method && _default_instance.null ()) {
        SCIM_DEBUG_FRONTEND(2) << "update default instance.\n";
        _default_instance = si;
    }

    context_scim->ctx                       = ctx;
    context_scim->impl                      = new_ic_impl (context_scim);
    if (context_scim->impl == NULL) {
        std::cerr << "memory allocation failed in " << __FUNCTION__ << "\n";
        return;
    }

    context_scim->impl->si                  = si;
    context_scim->impl->client_window       = 0;
    context_scim->impl->client_canvas       = NULL;
    context_scim->impl->preedit_caret       = 0;
    context_scim->impl->cursor_x            = 0;
    context_scim->impl->cursor_y            = 0;
    context_scim->impl->cursor_pos          = -1;
    context_scim->impl->cursor_top_y        = 0;
    context_scim->impl->is_on               = true;
    context_scim->impl->shared_si           = _shared_input_method;
    context_scim->impl->use_preedit         = _on_the_spot;
    context_scim->impl->preedit_started     = false;
    context_scim->impl->preedit_updating    = false;
    context_scim->impl->need_commit_preedit = false;
    context_scim->impl->prediction_allow    = true;

    if (!_ic_list)
        context_scim->next = NULL;
    else
        context_scim->next = _ic_list;
    _ic_list = context_scim;

    if (_shared_input_method)
        context_scim->impl->is_on = _config->read (String (SCIM_CONFIG_FRONTEND_IM_OPENED_BY_DEFAULT), context_scim->impl->is_on);

    _panel_client.prepare (context_scim->id);
    _panel_client.register_input_context (context_scim->id, si->get_factory_uuid ());
    set_ic_capabilities (context_scim);
    _panel_client.send ();

    SCIM_DEBUG_FRONTEND(2) << "input context created: id = " << context_scim->id << "\n";
}

EAPI void
isf_imf_context_del (Ecore_IMF_Context *ctx)
{
    SCIM_DEBUG_FRONTEND(1) << __FUNCTION__ << "...\n";

    if (!_ic_list) return;

    EcoreIMFContextISF *context_scim = (EcoreIMFContextISF*)ecore_imf_context_data_get (ctx);
    Ecore_IMF_Input_Panel_State l_input_panel_state = ecore_imf_context_input_panel_state_get (ctx);

    if (context_scim) {
        if (context_scim->id != _ic_list->id) {
            EcoreIMFContextISF * pre = _ic_list;
            EcoreIMFContextISF * cur = _ic_list->next;
            while (cur != NULL) {
                if (cur->id == context_scim->id) {
                    pre->next = cur->next;
                    break;
                }
                pre = cur;
                cur = cur->next;
            }
        } else {
            _ic_list = _ic_list->next;
        }
    }

    if (context_scim && context_scim->impl) {
        _panel_client.prepare (context_scim->id);

        if (context_scim == _focused_ic)
            context_scim->impl->si->focus_out ();

        if (input_panel_ctx == ctx && _scim_initialized) {
            LOGD ("ctx : %p\n", ctx);
            if (l_input_panel_state == ECORE_IMF_INPUT_PANEL_STATE_WILL_SHOW ||
                l_input_panel_state == ECORE_IMF_INPUT_PANEL_STATE_SHOW) {
                ecore_imf_context_input_panel_hide (ctx);
                isf_imf_context_input_panel_send_will_hide_ack (ctx);
            }

            if (notified_state == ECORE_IMF_INPUT_PANEL_STATE_WILL_SHOW ||
                notified_state == ECORE_IMF_INPUT_PANEL_STATE_SHOW) {
                input_panel_event_callback_call (ECORE_IMF_INPUT_PANEL_STATE_EVENT, ECORE_IMF_INPUT_PANEL_STATE_HIDE);
            }
        }

        // Delete the instance.
        // FIXME:
        // In case the instance send out some helper event,
        // and this context has been focused out,
        // we need set the focused_ic to this context temporary.
        EcoreIMFContextISF *old_focused = _focused_ic;
        _focused_ic = context_scim;
        context_scim->impl->si.reset ();
        _focused_ic = old_focused;

        if (context_scim == _focused_ic) {
            _panel_client.turn_off (context_scim->id);
            _panel_client.focus_out (context_scim->id);
        }

        _panel_client.remove_input_context (context_scim->id);
        _panel_client.send ();

        if (context_scim->impl->client_window)
            isf_imf_context_client_window_set (ctx, NULL);

        if (context_scim->impl) {
            delete_ic_impl (context_scim->impl);
            context_scim->impl = 0;
        }
    }

    isf_imf_context_input_panel_event_callback_clear (ctx);

    if (context_scim == _focused_ic)
        _focused_ic = 0;

    if (context_scim) {
        delete context_scim;
        context_scim = 0;
    }
}

/**
 * isf_imf_context_client_canvas_set
 * @ctx: a #Ecore_IMF_Context
 * @canvas: the client canvas
 *
 * This function will be called by Ecore IMF.
 *
 * Set the client canvas for the Input Method Context; this is the canvas
 * in which the input appears.
 *
 * The canvas type can be determined by using the context canvas type.
 * Actually only canvas with type "evas" (Evas *) is supported. This canvas
 * may be used in order to correctly position status windows, and may also
 * be used for purposes internal to the Input Method Context.
 */
EAPI void
isf_imf_context_client_canvas_set (Ecore_IMF_Context *ctx, void *canvas)
{
    SCIM_DEBUG_FRONTEND(1) << __FUNCTION__ << "...\n";

    EcoreIMFContextISF *context_scim = (EcoreIMFContextISF *)ecore_imf_context_data_get (ctx);

    if (context_scim && context_scim->impl && context_scim->impl->client_canvas != (Evas*) canvas) {
        context_scim->impl->client_canvas = (Evas*)canvas;
    }
}

/**
 * isf_imf_context_client_window_set
 * @ctx: a #Ecore_IMF_Context
 * @window: the client window
 *
 * This function will be called by Ecore IMF.
 *
 * Set the client window for the Input Method Context; this is the Ecore_X_Window
 * when using X11, Ecore_Win32_Window when using Win32, etc.
 *
 * This window is used in order to correctly position status windows,
 * and may also be used for purposes internal to the Input Method Context.
 */
EAPI void
isf_imf_context_client_window_set (Ecore_IMF_Context *ctx, void *window)
{
    SCIM_DEBUG_FRONTEND(1) << __FUNCTION__ << "...\n";

    EcoreIMFContextISF *context_scim = (EcoreIMFContextISF *)ecore_imf_context_data_get (ctx);

    if (context_scim && context_scim->impl && context_scim->impl->client_window != (Ecore_X_Window)((Ecore_Window)window)) {
        context_scim->impl->client_window = (Ecore_X_Window)((Ecore_Window)window);

        if ((context_scim->impl->client_window != 0) &&
                (context_scim->impl->client_window != _client_window)) {
            _client_window = context_scim->impl->client_window;
        }
    }
}

/**
 * isf_imf_context_focus_in
 * @ctx: a #Ecore_IMF_Context
 *
 * This function will be called by Ecore IMF.
 *
 * Notify the Input Method Context that the widget to which its correspond has gained focus.
 */
EAPI void
isf_imf_context_focus_in (Ecore_IMF_Context *ctx)
{
    EcoreIMFContextISF *context_scim = (EcoreIMFContextISF *)ecore_imf_context_data_get (ctx);

    if (!context_scim)
        return;

    if (!_panel_initialized)
        panel_initialize ();

    SCIM_DEBUG_FRONTEND(1) << __FUNCTION__<< "(" << context_scim->id << ")...\n";

    if (_focused_ic) {
        if (_focused_ic == context_scim) {
            SCIM_DEBUG_FRONTEND(1) << "It's already focused.\n";
            return;
        }
        SCIM_DEBUG_FRONTEND(1) << "Focus out previous IC first: " << _focused_ic->id << "\n";
        if (_focused_ic->ctx)
            isf_imf_context_focus_out (_focused_ic->ctx);
    }

    if (_change_keyboard_mode_by_focus_move) {
        TOOLBAR_MODE_T kbd_mode = TOOLBAR_HELPER_MODE;
        unsigned int val = 0;
        //To get the current keyboard mode
        if (ecore_x_window_prop_card32_get (ecore_x_window_root_first_get (), ecore_x_atom_get (PROP_X_EXT_KEYBOARD_INPUT_DETECTED), &val, 1) > 0) {
            if (val == 1) {
                kbd_mode = TOOLBAR_KEYBOARD_MODE;
            } else {
                kbd_mode = TOOLBAR_HELPER_MODE;
            }
        } else {
            kbd_mode = TOOLBAR_HELPER_MODE;
        }

        //if h/w keyboard mode, keyboard mode will be changed to s/w mode when the entry get the focus.
        if (kbd_mode == TOOLBAR_KEYBOARD_MODE) {
            LOGD ("Keyboard mode is changed H/W->S/W because of focus_in.\n");
            isf_imf_context_set_keyboard_mode (ctx, TOOLBAR_HELPER_MODE);
        }
    }

    bool need_cap   = false;
    bool need_reset = false;
    bool need_reg   = false;

    if (context_scim && context_scim->impl) {
        _focused_ic = context_scim;

        _panel_client.prepare (context_scim->id);

        // Handle the "Shared Input Method" mode.
        if (_shared_input_method) {
            SCIM_DEBUG_FRONTEND(2) << "shared input method.\n";
            IMEngineFactoryPointer factory = _backend->get_default_factory (_language, "UTF-8");
            if (!factory.null ()) {
                if (_default_instance.null () || _default_instance->get_factory_uuid () != factory->get_uuid ()) {
                    _default_instance = factory->create_instance ("UTF-8", _default_instance.null () ? _instance_count++ : _default_instance->get_id ());
                    attach_instance (_default_instance);
                    SCIM_DEBUG_FRONTEND(2) << "create new default instance: " << _default_instance->get_id () << " " << _default_instance->get_factory_uuid () << "\n";
                }

                context_scim->impl->shared_si = true;
                context_scim->impl->si = _default_instance;

                context_scim->impl->is_on = _config->read (String (SCIM_CONFIG_FRONTEND_IM_OPENED_BY_DEFAULT), context_scim->impl->is_on);
                context_scim->impl->preedit_string.clear ();
                context_scim->impl->preedit_attrlist.clear ();
                context_scim->impl->preedit_caret = 0;
                context_scim->impl->preedit_started = false;
                need_cap = true;
                need_reset = true;
                need_reg = true;
            }
        } else if (context_scim->impl->shared_si) {
            SCIM_DEBUG_FRONTEND(2) << "exit shared input method.\n";
            IMEngineFactoryPointer factory = _backend->get_default_factory (_language, "UTF-8");
            if (!factory.null ()) {
                context_scim->impl->si = factory->create_instance ("UTF-8", _instance_count++);
                context_scim->impl->preedit_string.clear ();
                context_scim->impl->preedit_attrlist.clear ();
                context_scim->impl->preedit_caret = 0;
                context_scim->impl->preedit_started = false;
                attach_instance (context_scim->impl->si);
                need_cap = true;
                need_reg = true;
                context_scim->impl->shared_si = false;
                SCIM_DEBUG_FRONTEND(2) << "create new instance: " << context_scim->impl->si->get_id () << " " << context_scim->impl->si->get_factory_uuid () << "\n";
            }
        }

        context_scim->impl->si->set_frontend_data (static_cast <void*> (context_scim));

        if (need_reg) _panel_client.register_input_context (context_scim->id, context_scim->impl->si->get_factory_uuid ());
        if (need_cap) set_ic_capabilities (context_scim);

        save_current_xid (ctx);

        panel_req_focus_in (context_scim);
//        panel_req_update_spot_location (context_scim);
//        panel_req_update_factory_info (context_scim);

        if (need_reset) context_scim->impl->si->reset ();
        if (context_scim->impl->is_on) {
            _panel_client.turn_on (context_scim->id);
//            _panel_client.hide_preedit_string (context_scim->id);
//            _panel_client.hide_aux_string (context_scim->id);
//            _panel_client.hide_lookup_table (context_scim->id);
            context_scim->impl->si->focus_in ();
            context_scim->impl->si->set_layout (ecore_imf_context_input_panel_layout_get (ctx));
            context_scim->impl->si->set_prediction_allow (context_scim->impl->prediction_allow);
            LOGD ("ctx : %p. set autocapital type : %d\n", ctx, context_scim->impl->autocapital_type);
            context_scim->impl->si->set_autocapital_type (context_scim->impl->autocapital_type);
            context_scim->impl->si->set_input_hint (context_scim->impl->input_hint);
            context_scim->impl->si->update_bidi_direction (context_scim->impl->bidi_direction);
            if (context_scim->impl->imdata)
                context_scim->impl->si->set_imdata ((const char *)context_scim->impl->imdata, context_scim->impl->imdata_size);
        } else {
            _panel_client.turn_off (context_scim->id);
        }

        _panel_client.send ();
        context_scim->impl->next_shift_status = 0;
        context_scim->impl->shift_mode_enabled = 0;
    }

    LOGD ("ctx : %p. on demand : %d\n", ctx, ecore_imf_context_input_panel_show_on_demand_get (ctx));

    if (ecore_imf_context_input_panel_enabled_get (ctx)) {
        if (!ecore_imf_context_input_panel_show_on_demand_get (ctx))
            ecore_imf_context_input_panel_show (ctx);
        else
            LOGD ("ctx : %p input panel on demand mode : TRUE\n", ctx);
    }
    else
        LOGD ("ctx : %p input panel enable : FALSE\n", ctx);

    if (get_keyboard_mode () == TOOLBAR_KEYBOARD_MODE)
        clear_hide_request ();
}

/**
 * isf_imf_context_focus_out
 * @ctx: a #Ecore_IMF_Context
 *
 * This function will be called by Ecore IMF.
 *
 * Notify the Input Method Context that the widget to which its correspond has lost focus.
 */
EAPI void
isf_imf_context_focus_out (Ecore_IMF_Context *ctx)
{
    EcoreIMFContextISF *context_scim = (EcoreIMFContextISF *)ecore_imf_context_data_get (ctx);

    if (!context_scim) return;

    SCIM_DEBUG_FRONTEND(1) << __FUNCTION__ << "(" << context_scim->id << ")...\n";

    if (context_scim && context_scim->impl && context_scim == _focused_ic) {

        WideString wstr = context_scim->impl->preedit_string;

        LOGD ("ctx : %p\n", ctx);

        if (ecore_imf_context_input_panel_enabled_get (ctx)) {
            if (!check_focus_out_by_popup_win (ctx))
                ecore_imf_context_input_panel_hide (ctx);
        }

        if (context_scim->impl->need_commit_preedit) {
            _hide_preedit_string (context_scim->id, false);

            if (wstr.length ()) {
                ecore_imf_context_commit_event_add (context_scim->ctx, utf8_wcstombs (wstr).c_str ());
                ecore_imf_context_event_callback_call (context_scim->ctx, ECORE_IMF_CALLBACK_COMMIT, const_cast<char*>(utf8_wcstombs (wstr).c_str ()));
            }
            _panel_client.prepare (context_scim->id);
            _panel_client.reset_input_context (context_scim->id);
            _panel_client.send ();
        }

        if (context_scim->impl->si) {
            _panel_client.prepare (context_scim->id);

            context_scim->impl->si->focus_out ();
            context_scim->impl->si->reset ();
            context_scim->impl->cursor_pos = -1;

            //          if (context_scim->impl->shared_si) context_scim->impl->si->reset ();

            _panel_client.focus_out (context_scim->id);
            _panel_client.send ();
        }
        _focused_ic = 0;
    }
    _x_key_event_is_valid = false;
}

/**
 * isf_imf_context_reset
 * @ctx: a #Ecore_IMF_Context
 *
 * This function will be called by Ecore IMF.
 *
 * Notify the Input Method Context that a change such as a change in cursor
 * position has been made. This will typically cause the Input Method Context
 * to clear the preedit state.
 */
EAPI void
isf_imf_context_reset (Ecore_IMF_Context *ctx)
{
    SCIM_DEBUG_FRONTEND(1) << __FUNCTION__ << "...\n";

    EcoreIMFContextISF *context_scim = (EcoreIMFContextISF *)ecore_imf_context_data_get (ctx);

    if (context_scim && context_scim->impl && context_scim == _focused_ic) {
        WideString wstr = context_scim->impl->preedit_string;

        if (context_scim->impl->si) {
            _panel_client.prepare (context_scim->id);
            context_scim->impl->si->reset ();
            _panel_client.reset_input_context (context_scim->id);
            _panel_client.send ();
        }

        if (context_scim->impl->need_commit_preedit) {
            _hide_preedit_string (context_scim->id, false);

            if (wstr.length ()) {
                ecore_imf_context_commit_event_add (context_scim->ctx, utf8_wcstombs (wstr).c_str ());
                ecore_imf_context_event_callback_call (context_scim->ctx, ECORE_IMF_CALLBACK_COMMIT, const_cast<char*>(utf8_wcstombs (wstr).c_str ()));
            }
        }
    }
}

/**
 * isf_imf_context_cursor_position_set
 * @ctx: a #Ecore_IMF_Context
 * @cursor_pos: New cursor position in characters.
 *
 * This function will be called by Ecore IMF.
 *
 * Notify the Input Method Context that a change in the cursor position has been made.
 */
EAPI void
isf_imf_context_cursor_position_set (Ecore_IMF_Context *ctx, int cursor_pos)
{
    SCIM_DEBUG_FRONTEND(1) << __FUNCTION__ << "...\n";

    EcoreIMFContextISF *context_scim = (EcoreIMFContextISF *)ecore_imf_context_data_get (ctx);

    if (context_scim && context_scim->impl && context_scim == _focused_ic) {
        if (context_scim->impl->cursor_pos != cursor_pos) {
            LOGD ("ctx : %p, cursor pos : %d\n", ctx, cursor_pos);
            context_scim->impl->cursor_pos = cursor_pos;

            caps_mode_check (ctx, EINA_FALSE, EINA_TRUE);

            if (context_scim->impl->preedit_updating)
                return;

            if (context_scim->impl->si) {
                _panel_client.prepare (context_scim->id);
                context_scim->impl->si->update_cursor_position (cursor_pos);
                panel_req_update_cursor_position (context_scim, cursor_pos);
                _panel_client.send ();
            }
        }
    }
}

/**
 * isf_imf_context_cursor_location_set
 * @ctx: a #Ecore_IMF_Context
 * @x: x position of New cursor.
 * @y: y position of New cursor.
 * @w: the width of New cursor.
 * @h: the height of New cursor.
 *
 * This function will be called by Ecore IMF.
 *
 * Notify the Input Method Context that a change in the cursor location has been made.
 */
EAPI void
isf_imf_context_cursor_location_set (Ecore_IMF_Context *ctx, int cx, int cy, int cw, int ch)
{
    SCIM_DEBUG_FRONTEND(1) << __FUNCTION__ << "...\n";

    EcoreIMFContextISF *context_scim = (EcoreIMFContextISF *)ecore_imf_context_data_get (ctx);
    Ecore_Evas *ee;
    int canvas_x, canvas_y;
    int new_cursor_x, new_cursor_y;

    if (cw == 0 && ch == 0)
        return;

    if (context_scim && context_scim->impl && context_scim == _focused_ic) {
        if (context_scim->impl->client_canvas) {
            ee = ecore_evas_ecore_evas_get (context_scim->impl->client_canvas);
            if (!ee) return;

            ecore_evas_geometry_get (ee, &canvas_x, &canvas_y, NULL, NULL);
        }
        else {
            if (context_scim->impl->client_window)
                window_to_screen_geometry_get (context_scim->impl->client_window, &canvas_x, &canvas_y);
            else
                return;
        }

        new_cursor_x = canvas_x + cx;
        new_cursor_y = canvas_y + cy + ch;

        // Don't update spot location while updating preedit string.
        if (context_scim->impl->preedit_updating && (context_scim->impl->cursor_y == new_cursor_y))
            return;

        if (context_scim->impl->cursor_x != new_cursor_x || context_scim->impl->cursor_y != new_cursor_y) {
            context_scim->impl->cursor_x     = new_cursor_x;
            context_scim->impl->cursor_y     = new_cursor_y;
            context_scim->impl->cursor_top_y = canvas_y + cy;
            _panel_client.prepare (context_scim->id);
            panel_req_update_spot_location (context_scim);
            _panel_client.send ();
            SCIM_DEBUG_FRONTEND(2) << "new cursor location = " << context_scim->impl->cursor_x << "," << context_scim->impl->cursor_y << "\n";
        }
    }
}

/**
 * isf_imf_context_input_mode_set
 * @ctx: a #Ecore_IMF_Context
 * @input_mode: the input mode
 *
 * This function will be called by Ecore IMF.
 *
 * To set the input mode of input method. The definition of Ecore_IMF_Input_Mode
 * is in Ecore_IMF.h.
 */
EAPI void
isf_imf_context_input_mode_set (Ecore_IMF_Context *ctx, Ecore_IMF_Input_Mode input_mode)
{
    SCIM_DEBUG_FRONTEND(1) << __FUNCTION__ << "...\n";

    EcoreIMFContextISF *context_scim = (EcoreIMFContextISF*)ecore_imf_context_data_get (ctx);
    if (context_scim && context_scim->impl) {
        if (context_scim->impl->input_mode != input_mode) {
            context_scim->impl->input_mode = input_mode;

            isf_imf_context_input_panel_input_mode_set (ctx, input_mode);
        }
    }
}

/**
 * isf_imf_context_preedit_string_get
 * @ctx: a #Ecore_IMF_Context
 * @str: the preedit string
 * @cursor_pos: the cursor position
 *
 * This function will be called by Ecore IMF.
 *
 * To get the preedit string of the input method.
 */
EAPI void
isf_imf_context_preedit_string_get (Ecore_IMF_Context *ctx, char** str, int *cursor_pos)
{
    SCIM_DEBUG_FRONTEND(1) << __FUNCTION__ << "...\n";

    EcoreIMFContextISF *context_scim = (EcoreIMFContextISF*)ecore_imf_context_data_get (ctx);

    if (context_scim && context_scim->impl && context_scim->impl->is_on) {
        String mbs = utf8_wcstombs (context_scim->impl->preedit_string);

        if (str) {
            if (mbs.length ())
                *str = strdup (mbs.c_str ());
            else
                *str = strdup ("");
        }

        if (cursor_pos) {
            *cursor_pos = context_scim->impl->preedit_caret;
        }
    } else {
        if (str)
            *str = strdup ("");

        if (cursor_pos)
            *cursor_pos = 0;
    }
}

EAPI void
isf_imf_context_preedit_string_with_attributes_get (Ecore_IMF_Context *ctx, char** str, Eina_List **attrs, int *cursor_pos)
{
    SCIM_DEBUG_FRONTEND(1) << __FUNCTION__ << "...\n";

    EcoreIMFContextISF *context_scim = (EcoreIMFContextISF*)ecore_imf_context_data_get (ctx);

    if (context_scim && context_scim->impl && context_scim->impl->is_on) {
        String mbs = utf8_wcstombs (context_scim->impl->preedit_string);

        if (str) {
            if (mbs.length ())
                *str = strdup (mbs.c_str ());
            else
                *str = strdup ("");
        }

        if (cursor_pos) {
            *cursor_pos = context_scim->impl->preedit_caret;
        }

        if (attrs) {
            if (mbs.length ()) {
                int start_index, end_index;
                int wlen = context_scim->impl->preedit_string.length ();
                Ecore_IMF_Preedit_Attr *attr = NULL;
                AttributeList::const_iterator i;
                bool *attrs_flag = new bool [mbs.length ()];
                memset (attrs_flag, 0, mbs.length () * sizeof (bool));
                for (i = context_scim->impl->preedit_attrlist.begin ();
                    i != context_scim->impl->preedit_attrlist.end (); ++i) {
                    start_index = i->get_start ();
                    end_index = i->get_end ();
                    if (end_index <= wlen && start_index < end_index && i->get_type () != SCIM_ATTR_DECORATE_NONE) {
                        start_index = g_utf8_offset_to_pointer (mbs.c_str (), i->get_start ()) - mbs.c_str ();
                        end_index = g_utf8_offset_to_pointer (mbs.c_str (), i->get_end ()) - mbs.c_str ();
                        if (i->get_type () == SCIM_ATTR_DECORATE) {
                            attr = (Ecore_IMF_Preedit_Attr *)calloc (1, sizeof (Ecore_IMF_Preedit_Attr));
                            if (attr == NULL)
                                continue;
                            attr->start_index = start_index;
                            attr->end_index = end_index;

                            if (i->get_value () == SCIM_ATTR_DECORATE_UNDERLINE) {
                                attr->preedit_type = ECORE_IMF_PREEDIT_TYPE_SUB1;
                                *attrs = eina_list_append (*attrs, (void *)attr);
                            } else if (i->get_value () == SCIM_ATTR_DECORATE_REVERSE) {
                                attr->preedit_type = ECORE_IMF_PREEDIT_TYPE_SUB2;
                                *attrs = eina_list_append (*attrs, (void *)attr);
                            } else if (i->get_value () == SCIM_ATTR_DECORATE_HIGHLIGHT) {
                                attr->preedit_type = ECORE_IMF_PREEDIT_TYPE_SUB3;
                                *attrs = eina_list_append (*attrs, (void *)attr);
                            } else if (i->get_value () == SCIM_ATTR_DECORATE_BGCOLOR1) {
                                attr->preedit_type = ECORE_IMF_PREEDIT_TYPE_SUB4;
                                *attrs = eina_list_append (*attrs, (void *)attr);
                            } else if (i->get_value () == SCIM_ATTR_DECORATE_BGCOLOR2) {
                                attr->preedit_type = ECORE_IMF_PREEDIT_TYPE_SUB5;
                                *attrs = eina_list_append (*attrs, (void *)attr);
                            } else if (i->get_value () == SCIM_ATTR_DECORATE_BGCOLOR3) {
                                attr->preedit_type = ECORE_IMF_PREEDIT_TYPE_SUB6;
                                *attrs = eina_list_append (*attrs, (void *)attr);
                            } else if (i->get_value () == SCIM_ATTR_DECORATE_BGCOLOR4) {
                                attr->preedit_type = ECORE_IMF_PREEDIT_TYPE_SUB7;
                                *attrs = eina_list_append (*attrs, (void *)attr);
                            } else {
                                free (attr);
                            }
                            switch (i->get_value ())
                            {
                                case SCIM_ATTR_DECORATE_UNDERLINE:
                                case SCIM_ATTR_DECORATE_REVERSE:
                                case SCIM_ATTR_DECORATE_HIGHLIGHT:
                                case SCIM_ATTR_DECORATE_BGCOLOR1:
                                case SCIM_ATTR_DECORATE_BGCOLOR2:
                                case SCIM_ATTR_DECORATE_BGCOLOR3:
                                case SCIM_ATTR_DECORATE_BGCOLOR4:
                                    // Record which character has attribute.
                                    for (int pos = start_index; pos < end_index; ++pos)
                                        attrs_flag [pos] = 1;
                                    break;
                                default:
                                    break;
                            }
                        } else if (i->get_type () == SCIM_ATTR_FOREGROUND) {
                            SCIM_DEBUG_FRONTEND(4) << "SCIM_ATTR_FOREGROUND\n";
                        } else if (i->get_type () == SCIM_ATTR_BACKGROUND) {
                            SCIM_DEBUG_FRONTEND(4) << "SCIM_ATTR_BACKGROUND\n";
                        }
                    }
                }
                // Add underline for all characters which don't have attribute.
                for (unsigned int pos = 0; pos < mbs.length (); ++pos) {
                    if (!attrs_flag [pos]) {
                        int begin_pos = pos;
                        while (pos < mbs.length () && !attrs_flag [pos])
                            ++pos;
                        // use REVERSE style as default
                        attr = (Ecore_IMF_Preedit_Attr *)calloc (1, sizeof (Ecore_IMF_Preedit_Attr));
                        if (attr == NULL)
                            continue;
                        attr->preedit_type = ECORE_IMF_PREEDIT_TYPE_SUB2;
                        attr->start_index = begin_pos;
                        attr->end_index = pos;
                        *attrs = eina_list_append (*attrs, (void *)attr);
                    }
                }
                delete [] attrs_flag;
            }
        }
    } else {
        if (str)
            *str = strdup ("");

        if (cursor_pos)
            *cursor_pos = 0;

        if (attrs)
            *attrs = NULL;
    }
}

/**
 * isf_imf_context_use_preedit_set
 * @ctx: a #Ecore_IMF_Context
 * @use_preedit: Whether the IM context should use the preedit string.
 *
 * This function will be called by Ecore IMF.
 *
 * Set whether the IM context should use the preedit string to display feedback.
 * If is 0 (default is 1), then the IM context may use some other method to
 * display feedback, such as displaying it in a child of the root window.
 */
EAPI void
isf_imf_context_use_preedit_set (Ecore_IMF_Context* ctx, Eina_Bool use_preedit)
{
    SCIM_DEBUG_FRONTEND(1) << __FUNCTION__ << " = " << (use_preedit == EINA_TRUE ? "true" : "false") << "...\n";

    EcoreIMFContextISF *context_scim = (EcoreIMFContextISF*)ecore_imf_context_data_get (ctx);

    if (!_on_the_spot) return;

    if (context_scim && context_scim->impl) {
        bool old = context_scim->impl->use_preedit;
        context_scim->impl->use_preedit = use_preedit;
        if (context_scim == _focused_ic) {
            _panel_client.prepare (context_scim->id);

            if (old != use_preedit)
                set_ic_capabilities (context_scim);

            if (context_scim->impl->preedit_string.length ())
                slot_show_preedit_string (context_scim->impl->si);

            _panel_client.send ();
        }
    }
}

/**
 * isf_imf_context_prediction_allow_set
 * @ctx: a #Ecore_IMF_Context
 * @prediction: Whether the IM context should use the prediction.
 *
 * This function will be called by Ecore IMF.
 *
 * Set whether the IM context should use the prediction.
 */
EAPI void
isf_imf_context_prediction_allow_set (Ecore_IMF_Context* ctx, Eina_Bool prediction)
{
    SCIM_DEBUG_FRONTEND(1) << __FUNCTION__ << " = " << (prediction == EINA_TRUE ? "true" : "false") << "...\n";

    EcoreIMFContextISF *context_scim = (EcoreIMFContextISF *)ecore_imf_context_data_get (ctx);

    if (context_scim && context_scim->impl && context_scim->impl->prediction_allow != prediction) {
        context_scim->impl->prediction_allow = prediction;
        set_prediction_allow (ctx, prediction);
    }
}

/**
 * isf_imf_context_prediction_allow_get
 * @ctx: a #Ecore_IMF_Context
 *
 * This function will be called by Ecore IMF.
 *
 * To get prediction allow flag for the IM context.
 *
 * Return value: the prediction allow flag for the IM context
 */
EAPI Eina_Bool
isf_imf_context_prediction_allow_get (Ecore_IMF_Context* ctx)
{
    SCIM_DEBUG_FRONTEND(1) << __FUNCTION__ << "...\n";

    EcoreIMFContextISF *context_scim = (EcoreIMFContextISF *)ecore_imf_context_data_get (ctx);

    Eina_Bool ret = EINA_FALSE;
    if (context_scim && context_scim->impl) {
        ret = context_scim->impl->prediction_allow;
    } else {
        std::cerr << __FUNCTION__ << " failed!!!\n";
    }
    return ret;
}

/**
 * isf_imf_context_autocapital_type_set
 * @ctx: a #Ecore_IMF_Context
 * @autocapital_type: the autocapital type for the IM context.
 *
 * This function will be called by Ecore IMF.
 *
 * Set autocapital type for the IM context.
 */
EAPI void
isf_imf_context_autocapital_type_set (Ecore_IMF_Context* ctx, Ecore_IMF_Autocapital_Type autocapital_type)
{
    SCIM_DEBUG_FRONTEND(1) << __FUNCTION__ << " = " << autocapital_type << "...\n";

    EcoreIMFContextISF *context_scim = (EcoreIMFContextISF *)ecore_imf_context_data_get (ctx);

    if (context_scim && context_scim->impl && context_scim->impl->autocapital_type != autocapital_type) {
        context_scim->impl->autocapital_type = autocapital_type;

        if (context_scim->impl->si && context_scim == _focused_ic) {
            LOGD ("ctx : %p. set autocapital type : %d\n", ctx, autocapital_type);
            _panel_client.prepare (context_scim->id);
            context_scim->impl->si->set_autocapital_type (autocapital_type);
            _panel_client.send ();
        }
    }
}

/**
 * isf_imf_context_filter_event
 * @ctx: a #Ecore_IMF_Context
 * @type: The type of event defined by Ecore_IMF_Event_Type.
 * @event: The event itself.
 * Return value: %TRUE if the input method handled the key event.
 *
 * This function will be called by Ecore IMF.
 *
 * Allow an Ecore Input Context to internally handle an event. If this function
 * returns 1, then no further processing should be done for this event. Input
 * methods must be able to accept all types of events (simply returning 0 if
 * the event was not handled), but there is no obligation of any events to be
 * submitted to this function.
 */
EAPI Eina_Bool
isf_imf_context_filter_event (Ecore_IMF_Context *ctx, Ecore_IMF_Event_Type type, Ecore_IMF_Event *event)
{
    SCIM_DEBUG_FRONTEND(1) << __FUNCTION__ << "...\n";

    EcoreIMFContextISF *ic = (EcoreIMFContextISF*)ecore_imf_context_data_get (ctx);
    Eina_Bool ret = EINA_FALSE;

    if (ic == NULL || ic->impl == NULL) {
        LOGW ("ic is NULL\n");
        return ret;
    }
    KeyEvent key;
    unsigned int timestamp = 0;

    if (type == ECORE_IMF_EVENT_KEY_DOWN) {
        Ecore_IMF_Event_Key_Down *ev = (Ecore_IMF_Event_Key_Down *)event;
        timestamp = ev->timestamp;

        if (filter_keys (ev->keyname, ignore_keys))
            return EINA_TRUE;

        /* Hardware input detect code */
        if (ev->timestamp > 1 && get_keyboard_mode () == TOOLBAR_HELPER_MODE && _support_hw_keyboard_mode &&
            scim_string_to_key (key, ev->key) &&
            key.code != 0xFF69 /* Cancel (Power + Volume down) key */) {
            isf_imf_context_set_keyboard_mode (ctx, TOOLBAR_KEYBOARD_MODE);
            _panel_client.prepare (ic->id);
            _panel_client.get_active_helper_option (&_active_helper_option);
            _panel_client.send ();
            ISF_SAVE_LOG ("Changed keyboard mode from S/W to H/W (code: %x, name: %s)\n", key.code, ev->keyname);
            LOGD ("Hardware keyboard mode, active helper option: %d", _active_helper_option);
        }
    }
    else if (type == ECORE_IMF_EVENT_KEY_UP) {
        Ecore_IMF_Event_Key_Up *ev = (Ecore_IMF_Event_Key_Up *)event;
        timestamp = ev->timestamp;

        if (filter_keys (ev->keyname, ignore_keys))
            return EINA_TRUE;
    }

    if (type == ECORE_IMF_EVENT_KEY_DOWN || type == ECORE_IMF_EVENT_KEY_UP) {
        if ((timestamp == 0 || timestamp == 1) && !_x_key_event_is_valid) {
            std::cerr << "    S/W key event is not valid!!!\n";
            LOGW ("S/W key event is not valid\n");
            return EINA_TRUE;
        }
    }

    if (type == ECORE_IMF_EVENT_KEY_DOWN) {
        Ecore_IMF_Event_Key_Down *ev = (Ecore_IMF_Event_Key_Down *)event;
        timestamp = ev->timestamp;
        scim_string_to_key (key, ev->key);
        key.mask |= _ecore_imf_modifier_to_scim_mask (ev->modifiers);
        key.mask |= _ecore_imf_lock_to_scim_mask (ev->locks);
    } else if (type == ECORE_IMF_EVENT_KEY_UP) {
        Ecore_IMF_Event_Key_Up *ev = (Ecore_IMF_Event_Key_Up *)event;
        timestamp = ev->timestamp;
        scim_string_to_key (key, ev->key);
        key.mask = SCIM_KEY_ReleaseMask;
        key.mask |= _ecore_imf_modifier_to_scim_mask (ev->modifiers);
        key.mask |= _ecore_imf_lock_to_scim_mask (ev->locks);
    } else if (type == ECORE_IMF_EVENT_MOUSE_UP) {
        if (ecore_imf_context_input_panel_enabled_get (ctx)) {
            LOGD ("[Mouse-up event] ctx : %p\n", ctx);
            if (ic == _focused_ic) {
                if (_change_keyboard_mode_by_touch && get_keyboard_mode () == TOOLBAR_KEYBOARD_MODE) {
                    isf_imf_context_set_keyboard_mode (ctx, TOOLBAR_HELPER_MODE);
                    LOGD("S/W keyboard mode by enabling ChangeKeyboardModeByTouch");
                } else {
                    ecore_imf_context_input_panel_show (ctx);
                }
            }
            else
                LOGE ("Can't show IME because there is no focus. ctx : %p\n", ctx);
        }
        return EINA_FALSE;
    } else {
        return ret;
    }

    key.mask &= _valid_key_mask;

    _panel_client.prepare (ic->id);

    ret = EINA_TRUE;
    if (!filter_hotkeys (ic, key)) {
        if (timestamp == 0) {
            ret = EINA_FALSE;
            // in case of generated event
            if (type == ECORE_IMF_EVENT_KEY_DOWN) {
                char code = key.get_ascii_code ();
                if (isgraph (code)) {
                    char string[2] = {0};
                    snprintf (string, sizeof (string), "%c", code);

                    if (strlen (string) != 0) {
                        ecore_imf_context_commit_event_add (ic->ctx, string);
                        ecore_imf_context_event_callback_call (ic->ctx, ECORE_IMF_CALLBACK_COMMIT, (void *)string);
                        caps_mode_check (ctx, EINA_FALSE, EINA_TRUE);
                        ret = EINA_TRUE;
                    }
                } else {
                    if (key.code == SCIM_KEY_space ||
                        key.code == SCIM_KEY_KP_Space)
                        autoperiod_insert (ctx);
                }
            }
            _panel_client.send ();
            return ret;
        }
        if (!_focused_ic || !_focused_ic->impl || !_focused_ic->impl->is_on) {
            ret = EINA_FALSE;
        } else if (get_keyboard_mode () == TOOLBAR_KEYBOARD_MODE && (_active_helper_option & ISM_HELPER_PROCESS_KEYBOARD_KEYEVENT)) {
            void *pvoid = &ret;
            _panel_client.process_key_event (key, (int*)pvoid);
        } else {
            ret = _focused_ic->impl->si->process_key_event (key);
        }

        if (ret == EINA_FALSE) {
            if (type == ECORE_IMF_EVENT_KEY_DOWN) {
                if (key.code == SCIM_KEY_space ||
                    key.code == SCIM_KEY_KP_Space)
                    autoperiod_insert (ctx);
            }
        }
    }

    _panel_client.send ();

    return ret;
}

/**
 * Set up an ISE specific data
 *
 * @param[in] ctx a #Ecore_IMF_Context
 * @param[in] data pointer of data to sets up to ISE
 * @param[in] length length of data
 */
EAPI void isf_imf_context_imdata_set (Ecore_IMF_Context *ctx, const void* data, int length)
{
    SCIM_DEBUG_FRONTEND(1) << __FUNCTION__ << " data length ( " << length << ") ...\n";
    EcoreIMFContextISF *context_scim = (EcoreIMFContextISF *)ecore_imf_context_data_get (ctx);

    if (context_scim == NULL || data == NULL || length <= 0)
        return;

    if (context_scim && context_scim->impl) {
        if (context_scim->impl->imdata)
            free (context_scim->impl->imdata);

        context_scim->impl->imdata = calloc (1, length);
        memcpy (context_scim->impl->imdata, data, length);
        context_scim->impl->imdata_size = length;

        if (context_scim->impl->si && _focused_ic == context_scim) {
            _panel_client.prepare (context_scim->id);
            context_scim->impl->si->set_imdata ((const char *)data, length);
            _panel_client.send ();
        }
    }

    isf_imf_context_input_panel_imdata_set (ctx, data, length);
}

/**
 * Get the ISE specific data from ISE
 *
 * @param[in] ctx a #Ecore_IMF_Context
 * @param[out] data pointer of data to return
 * @param[out] length length of data
 */
EAPI void isf_imf_context_imdata_get (Ecore_IMF_Context *ctx, void* data, int* length)
{
    SCIM_DEBUG_FRONTEND(1) << __FUNCTION__ << "...\n";

    isf_imf_context_input_panel_imdata_get (ctx, data, length);
}

EAPI void
isf_imf_context_input_hint_set (Ecore_IMF_Context *ctx, Ecore_IMF_Input_Hints hint)
{
    SCIM_DEBUG_FRONTEND(1) << __FUNCTION__ << "...\n";

    EcoreIMFContextISF *context_scim = (EcoreIMFContextISF *)ecore_imf_context_data_get (ctx);

    if (context_scim && context_scim->impl) {
        if (context_scim->impl->input_hint != hint) {
            LOGD ("ctx : %p, input hint : %#x\n", ctx, hint);
            context_scim->impl->input_hint = hint;

            if (context_scim->impl->si && context_scim == _focused_ic) {
                _panel_client.prepare (context_scim->id);
                context_scim->impl->si->set_input_hint (hint);
                panel_req_set_input_hint (context_scim, hint);
                _panel_client.send ();
            }
        }
    }
}

EAPI void
isf_imf_context_bidi_direction_set (Ecore_IMF_Context *ctx, Ecore_IMF_BiDi_Direction direction)
{
    SCIM_DEBUG_FRONTEND(1) << __FUNCTION__ << "...\n";

    EcoreIMFContextISF *context_scim = (EcoreIMFContextISF *)ecore_imf_context_data_get (ctx);

    if (context_scim && context_scim->impl) {
        if (context_scim->impl->bidi_direction != direction) {
            LOGD ("ctx : %p, bidi direction : %#x\n", ctx, direction);
            context_scim->impl->bidi_direction = direction;

            if (context_scim->impl->si && context_scim == _focused_ic) {
                _panel_client.prepare (context_scim->id);
                context_scim->impl->si->update_bidi_direction (direction);
                panel_req_update_bidi_direction (context_scim, direction);
                _panel_client.send ();
            }
        }
    }
}

/* Panel Slot functions */
static void
panel_slot_reload_config (int context)
{
    SCIM_DEBUG_FRONTEND(1) << __FUNCTION__ << "...\n";
}

static void
panel_slot_exit (int /* context */)
{
    SCIM_DEBUG_FRONTEND(1) << __FUNCTION__ << "...\n";

    finalize ();
}

static void
panel_slot_update_candidate_item_layout (int context, const std::vector<uint32> &row_items)
{
    EcoreIMFContextISF *ic = find_ic (context);
    SCIM_DEBUG_FRONTEND(1) << __FUNCTION__ << " context=" << context << " row size=" << row_items.size () << " ic=" << ic << "\n";
    if (ic && ic->impl && ic->impl->si && _focused_ic == ic) {
        _panel_client.prepare (ic->id);
        ic->impl->si->update_candidate_item_layout (row_items);
        _panel_client.send ();
    }
}

static void
panel_slot_update_lookup_table_page_size (int context, int page_size)
{
    EcoreIMFContextISF *ic = find_ic (context);
    SCIM_DEBUG_FRONTEND(1) << __FUNCTION__ << " context=" << context << " page_size=" << page_size << " ic=" << ic << "\n";
    if (ic && ic->impl && ic->impl->si && _focused_ic == ic) {
        _panel_client.prepare (ic->id);
        ic->impl->si->update_lookup_table_page_size (page_size);
        _panel_client.send ();
    }
}

static void
panel_slot_lookup_table_page_up (int context)
{
    EcoreIMFContextISF *ic = find_ic (context);
    SCIM_DEBUG_FRONTEND(1) << __FUNCTION__ << " context=" << context << " ic=" << ic << "\n";
    if (ic && ic->impl && ic->impl->si && _focused_ic == ic) {
        _panel_client.prepare (ic->id);
        ic->impl->si->lookup_table_page_up ();
        _panel_client.send ();
    }
}

static void
panel_slot_lookup_table_page_down (int context)
{
    EcoreIMFContextISF *ic = find_ic (context);
    SCIM_DEBUG_FRONTEND(1) << __FUNCTION__ << " context=" << context << " ic=" << ic << "\n";
    if (ic && ic->impl && ic->impl->si && _focused_ic == ic) {
        _panel_client.prepare (ic->id);
        ic->impl->si->lookup_table_page_down ();
        _panel_client.send ();
    }
}

static void
panel_slot_trigger_property (int context, const String &property)
{
    EcoreIMFContextISF *ic = find_ic (context);
    SCIM_DEBUG_FRONTEND(1) << __FUNCTION__ << " context=" << context << " property=" << property << " ic=" << ic << "\n";
    if (ic && ic->impl && ic->impl->si) {
        _panel_client.prepare (ic->id);
        ic->impl->si->trigger_property (property);
        _panel_client.send ();
    }
}

static void
panel_slot_process_helper_event (int context, const String &target_uuid, const String &helper_uuid, const Transaction &trans)
{
    EcoreIMFContextISF *ic = find_ic (context);
    SCIM_DEBUG_FRONTEND(1) << __FUNCTION__ << " context=" << context << " target=" << target_uuid
                           << " helper=" << helper_uuid << " ic=" << ic << " ic->impl=" << (ic != NULL ? ic->impl : 0) << " ic-uuid="
                           << ((ic && ic->impl && ic->impl->si) ? ic->impl->si->get_factory_uuid () : "" ) << " _focused_ic=" << _focused_ic << "\n";
    if (ic && ic->impl && _focused_ic == ic && ic->impl->is_on && ic->impl->si &&
        ic->impl->si->get_factory_uuid () == target_uuid) {
        _panel_client.prepare (ic->id);
        SCIM_DEBUG_FRONTEND(2) << "call process_helper_event\n";
        ic->impl->si->process_helper_event (helper_uuid, trans);
        _panel_client.send ();
    }
}

static void
panel_slot_move_preedit_caret (int context, int caret_pos)
{
    EcoreIMFContextISF *ic = find_ic (context);
    SCIM_DEBUG_FRONTEND(1) << __FUNCTION__ << " context=" << context << " caret=" << caret_pos << " ic=" << ic << "\n";
    if (ic && ic->impl && ic->impl->si && _focused_ic == ic) {
        _panel_client.prepare (ic->id);
        ic->impl->si->move_preedit_caret (caret_pos);
        _panel_client.send ();
    }
}

static void
panel_slot_update_preedit_caret (int context, int caret)
{
    EcoreIMFContextISF *ic = find_ic (context);
    SCIM_DEBUG_FRONTEND(1) << __FUNCTION__ << " context=" << context << " caret=" << caret << " ic=" << ic << "\n";

    if (ic && ic->impl && _focused_ic == ic && ic->impl->preedit_caret != caret) {
        ic->impl->preedit_caret = caret;
        if (ic->impl->use_preedit) {
            if (!ic->impl->preedit_started) {
                ecore_imf_context_preedit_start_event_add (ic->ctx);
                ecore_imf_context_event_callback_call (ic->ctx, ECORE_IMF_CALLBACK_PREEDIT_START, NULL);
                if (check_valid_ic (ic))
                    ic->impl->preedit_started = true;
            }
            ecore_imf_context_preedit_changed_event_add (ic->ctx);
            ecore_imf_context_event_callback_call (ic->ctx, ECORE_IMF_CALLBACK_PREEDIT_CHANGED, NULL);
        } else {
            _panel_client.prepare (ic->id);
            _panel_client.update_preedit_caret (ic->id, caret);
            _panel_client.send ();
        }
    }
}

static void
panel_slot_select_aux (int context, int aux_index)
{
    EcoreIMFContextISF *ic = find_ic (context);
    SCIM_DEBUG_FRONTEND(1) << __FUNCTION__ << " context=" << context << " aux=" << aux_index << " ic=" << ic << "\n";
    if (ic && ic->impl && ic->impl->si && _focused_ic == ic) {
        _panel_client.prepare (ic->id);
        ic->impl->si->select_aux (aux_index);
        _panel_client.send ();
    }
}

static void
panel_slot_select_candidate (int context, int cand_index)
{
    EcoreIMFContextISF *ic = find_ic (context);
    SCIM_DEBUG_FRONTEND(1) << __FUNCTION__ << " context=" << context << " candidate=" << cand_index << " ic=" << ic << "\n";
    if (ic && ic->impl && ic->impl->si && _focused_ic == ic) {
        _panel_client.prepare (ic->id);
        ic->impl->si->select_candidate (cand_index);
        _panel_client.send ();
    }
}

static int
_keyname_to_keycode (const char *keyname)
{
    int keycode = 0;
    int keysym;
    Display *display = (Display *)ecore_x_display_get ();

    keysym = XStringToKeysym (keyname);

    if (!strncmp (keyname, "Keycode-", 8)) {
        keycode = atoi (keyname + 8);
    } else {
        keycode = XKeysymToKeycode (display, keysym);
    }

    return keycode;
}

static Eina_Bool
feed_key_event (EcoreIMFContextISF *ic, const KeyEvent &key, bool fake)
{
    SCIM_DEBUG_FRONTEND(1) << __FUNCTION__ << "...\n";

    if (key.code <= 0x7F ||
        (key.code >= SCIM_KEY_BackSpace && key.code <= SCIM_KEY_Delete) ||
        (key.code >= SCIM_KEY_Home && key.code <= SCIM_KEY_Hyper_R)) {
        // ascii code and function keys
        send_x_key_event (key, fake);
        return EINA_TRUE;
    } else {
        LOGW ("Unknown key code : %d\n", key.code);
        return EINA_FALSE;
    }
}

static void
panel_slot_process_key_event (int context, const KeyEvent &key)
{
    EcoreIMFContextISF *ic = find_ic (context);

    SCIM_DEBUG_FRONTEND(1) << __FUNCTION__ << " context=" << context << " key=" << key.get_key_string () << " ic=" << ic << "\n";

    if (!(ic && ic->impl))
        return;

    if ((_focused_ic != NULL) && (_focused_ic != ic)) {
        LOGW ("focused_ic : %p, ic : %p\n", _focused_ic, ic);
        return;
    }

    KeyEvent _key = key;
    if (key.is_key_press () &&
        ecore_imf_context_input_panel_layout_get (ic->ctx) == ECORE_IMF_INPUT_PANEL_LAYOUT_NORMAL) {
        if (key.code == SHIFT_MODE_OFF ||
            key.code == SHIFT_MODE_ON ||
            key.code == SHIFT_MODE_LOCK) {
            ic->impl->next_shift_status = _key.code;
        } else if (key.code == SHIFT_MODE_ENABLE) {
            ic->impl->shift_mode_enabled = true;
            caps_mode_check (ic->ctx, EINA_TRUE, EINA_TRUE);
        } else if (key.code == SHIFT_MODE_DISABLE) {
            ic->impl->shift_mode_enabled = false;
        }
    }

    if (key.code != SHIFT_MODE_OFF &&
        key.code != SHIFT_MODE_ON &&
        key.code != SHIFT_MODE_LOCK &&
        key.code != SHIFT_MODE_ENABLE &&
        key.code != SHIFT_MODE_DISABLE) {
        if (feed_key_event (ic, _key, false)) return;
        _panel_client.prepare (ic->id);
        if (!filter_hotkeys (ic, _key)) {
            if (!_focused_ic || !_focused_ic->impl->is_on ||
                    !_focused_ic->impl->si->process_key_event (_key)) {
                _fallback_instance->process_key_event (_key);
            }
        }
        _panel_client.send ();
    }
}

static void
panel_slot_commit_string (int context, const WideString &wstr)
{
    EcoreIMFContextISF *ic = find_ic (context);
    SCIM_DEBUG_FRONTEND(1) << __FUNCTION__ << " context=" << context << " str=" << utf8_wcstombs (wstr) << " ic=" << ic << "\n";

    if (ic && ic->impl) {
        if (_focused_ic != ic) {
            LOGW ("focused_ic is different from ic\n");
            return;
        }

        if (utf8_wcstombs (wstr) == String (" ") || utf8_wcstombs (wstr) == String ("　"))
            autoperiod_insert (ic->ctx);

        if (ic->impl->need_commit_preedit)
            _hide_preedit_string (ic->id, false);
        ecore_imf_context_commit_event_add (ic->ctx, utf8_wcstombs (wstr).c_str ());
        ecore_imf_context_event_callback_call (ic->ctx, ECORE_IMF_CALLBACK_COMMIT, const_cast<char*>(utf8_wcstombs (wstr).c_str ()));
    } else {
        LOGW ("No ic\n");
    }
}

static void
panel_slot_forward_key_event (int context, const KeyEvent &key)
{
    EcoreIMFContextISF *ic = find_ic (context);
    SCIM_DEBUG_FRONTEND(1) << __FUNCTION__ << " context=" << context << " key=" << key.get_key_string () << " ic=" << ic << "\n";
    LOGD ("forward key event requested\n");

    if (!(ic && ic->impl)) {
        LOGW ("No ic\n");
        return;
    }

    if ((_focused_ic != NULL) && (_focused_ic != ic)) {
        LOGW ("focused_ic : %p, ic : %p\n", _focused_ic, ic);
        return;
    }

    if (strlen (key.get_key_string ().c_str ()) >= 116) {
        LOGW ("the length of key string is too long\n");
        return;
    }

    feed_key_event (ic, key, true);
}

static void
panel_slot_request_help (int context)
{
    EcoreIMFContextISF *ic = find_ic (context);
    SCIM_DEBUG_FRONTEND(1) << __FUNCTION__ << " context=" << context << " ic=" << ic << "\n";
    if (ic && ic->impl) {
        _panel_client.prepare (ic->id);
        panel_req_show_help (ic);
        _panel_client.send ();
    }
}

static void
panel_slot_request_factory_menu (int context)
{
    EcoreIMFContextISF *ic = find_ic (context);
    SCIM_DEBUG_FRONTEND(1) << __FUNCTION__ << " context=" << context << " ic=" << ic << "\n";
    if (ic && ic->impl) {
        _panel_client.prepare (ic->id);
        panel_req_show_factory_menu (ic);
        _panel_client.send ();
    }
}

static void
panel_slot_change_factory (int context, const String &uuid)
{
    EcoreIMFContextISF *ic = find_ic (context);
    SCIM_DEBUG_FRONTEND(1) << __FUNCTION__ << " context=" << context << " factory=" << uuid << " ic=" << ic << "\n";
    if (ic && ic->impl && ic->impl->si) {
        _panel_client.prepare (ic->id);
        ic->impl->si->reset ();
        open_specific_factory (ic, uuid);
        _panel_client.send ();
    }
}

static void
panel_slot_reset_keyboard_ise (int context)
{
    EcoreIMFContextISF *ic = find_ic (context);
    SCIM_DEBUG_FRONTEND(1) << __FUNCTION__ << " context=" << context << " ic=" << ic << "\n";
    if (ic && ic->impl) {
        WideString wstr = ic->impl->preedit_string;
        if (ic->impl->need_commit_preedit) {
            _hide_preedit_string (ic->id, false);

            if (wstr.length ()) {
                ecore_imf_context_commit_event_add (ic->ctx, utf8_wcstombs (wstr).c_str ());
                ecore_imf_context_event_callback_call (ic->ctx, ECORE_IMF_CALLBACK_COMMIT, const_cast<char*>(utf8_wcstombs (wstr).c_str ()));
                if (!check_valid_ic (ic))
                    return;
            }
        }
        _panel_client.prepare (ic->id);
        ic->impl->si->reset ();
        _panel_client.send ();
    }
}

static void
panel_slot_update_keyboard_ise (int context)
{
    EcoreIMFContextISF *ic = find_ic (context);
    SCIM_DEBUG_FRONTEND(1) << __FUNCTION__ << " context=" << context << " ic=" << ic << "\n";

    _backend->add_module (_config, "socket", false);
}

static void
panel_slot_show_preedit_string (int context)
{
    EcoreIMFContextISF *ic = find_ic (context);
    SCIM_DEBUG_FRONTEND(1) << __FUNCTION__ << " context=" << context << "\n";

    if (ic && ic->impl && _focused_ic == ic) {
        if (!ic->impl->is_on)
            ic->impl->is_on = true;

        if (ic->impl->use_preedit) {
            if (!ic->impl->preedit_started) {
                ecore_imf_context_preedit_start_event_add (_focused_ic->ctx);
                ecore_imf_context_event_callback_call (_focused_ic->ctx, ECORE_IMF_CALLBACK_PREEDIT_START, NULL);
                if (check_valid_ic (ic)) {
                    ic->impl->preedit_started     = true;
                    ic->impl->need_commit_preedit = true;
                }
            }
        } else {
            _panel_client.prepare (ic->id);
            _panel_client.show_preedit_string (ic->id);
            _panel_client.send ();
        }
    }
}

static void
_hide_preedit_string (int context, bool update_preedit)
{
    SCIM_DEBUG_FRONTEND(1) << __FUNCTION__ << "...\n";

    EcoreIMFContextISF *ic = find_ic (context);

    if (ic && ic->impl && _focused_ic == ic) {
        if (!ic->impl->is_on)
            ic->impl->is_on = true;

        bool emit = false;
        if (ic->impl->preedit_string.length ()) {
            ic->impl->preedit_string = WideString ();
            ic->impl->preedit_caret  = 0;
            ic->impl->preedit_attrlist.clear ();
            emit = true;
        }
        if (ic->impl->use_preedit) {
            if (update_preedit && emit) {
                ecore_imf_context_preedit_changed_event_add (ic->ctx);
                ecore_imf_context_event_callback_call (ic->ctx, ECORE_IMF_CALLBACK_PREEDIT_CHANGED, NULL);
                if (!check_valid_ic (ic))
                    return;
            }
            if (ic->impl->preedit_started) {
                ecore_imf_context_preedit_end_event_add (ic->ctx);
                ecore_imf_context_event_callback_call (ic->ctx, ECORE_IMF_CALLBACK_PREEDIT_END, NULL);
                if (check_valid_ic (ic)) {
                    ic->impl->preedit_started     = false;
                    ic->impl->need_commit_preedit = false;
                }
            }
        } else {
            _panel_client.prepare (ic->id);
            _panel_client.hide_preedit_string (ic->id);
            _panel_client.send ();
        }
    }
}

static void
panel_slot_hide_preedit_string (int context)
{
    SCIM_DEBUG_FRONTEND(1) << __FUNCTION__ << "...\n";

    _hide_preedit_string (context, true);
}

static void
panel_slot_update_preedit_string (int context,
                                  const WideString    &str,
                                  const AttributeList &attrs,
                                  int caret)
{
    SCIM_DEBUG_FRONTEND(1) << __FUNCTION__ << "...\n";

    EcoreIMFContextISF *ic = find_ic (context);

    if (ic && ic->impl && _focused_ic == ic) {
        if (!ic->impl->is_on)
            ic->impl->is_on = true;

        if (ic->impl->preedit_string != str || str.length ()) {
            ic->impl->preedit_string   = str;
            ic->impl->preedit_attrlist = attrs;

            if (ic->impl->use_preedit) {
                if (!ic->impl->preedit_started) {
                    ecore_imf_context_preedit_start_event_add (ic->ctx);
                    ecore_imf_context_event_callback_call (ic->ctx, ECORE_IMF_CALLBACK_PREEDIT_START, NULL);
                    if (!check_valid_ic (ic))
                        return;

                    ic->impl->preedit_started = true;
                    ic->impl->need_commit_preedit = true;
                }
                if (caret >= 0 && caret <= (int)str.length ())
                    ic->impl->preedit_caret    = caret;
                else
                    ic->impl->preedit_caret    = str.length ();
                ic->impl->preedit_updating = true;
                ecore_imf_context_preedit_changed_event_add (ic->ctx);
                ecore_imf_context_event_callback_call (ic->ctx, ECORE_IMF_CALLBACK_PREEDIT_CHANGED, NULL);
                if (check_valid_ic (ic))
                    ic->impl->preedit_updating = false;
            } else {
                _panel_client.prepare (ic->id);
                _panel_client.update_preedit_string (ic->id, str, attrs, caret);
                _panel_client.send ();
            }
        }
    }
}

static void
panel_slot_get_surrounding_text (int context, int maxlen_before, int maxlen_after)
{
    SCIM_DEBUG_FRONTEND(1) << __FUNCTION__ << "...\n";

    EcoreIMFContextISF *ic = find_ic (context);

    if (ic && ic->impl && ic->impl->si && _focused_ic == ic) {
        int cursor = 0;
        WideString text = WideString ();
        slot_get_surrounding_text (ic->impl->si, text, cursor, maxlen_before, maxlen_after);
        _panel_client.prepare (ic->id);
        _panel_client.update_surrounding_text (ic->id, text, cursor);
        _panel_client.send ();
    }
}

static void
panel_slot_delete_surrounding_text (int context, int offset, int len)
{
    SCIM_DEBUG_FRONTEND(1) << __FUNCTION__ << "...\n";

    EcoreIMFContextISF *ic = find_ic (context);

    if (ic && ic->impl && ic->impl->si && _focused_ic == ic)
        slot_delete_surrounding_text (ic->impl->si, offset, len);
}

static void
panel_slot_get_selection (int context)
{
    SCIM_DEBUG_FRONTEND(1) << __FUNCTION__ << "...\n";

    EcoreIMFContextISF *ic = find_ic (context);

    if (ic && ic->impl && _focused_ic == ic && ic->impl->si) {
        WideString text = WideString ();
        slot_get_selection (ic->impl->si, text);
        _panel_client.prepare (ic->id);
        _panel_client.update_selection (ic->id, text);
        _panel_client.send ();
    }
}

static void
panel_slot_set_selection (int context, int start, int end)
{
    SCIM_DEBUG_FRONTEND(1) << __FUNCTION__ << "...\n";

    EcoreIMFContextISF *ic = find_ic (context);

    if (ic && ic->impl && ic->impl->si && _focused_ic == ic)
        slot_set_selection (ic->impl->si, start, end);
}

static void
panel_slot_update_displayed_candidate_number (int context, int number)
{
    EcoreIMFContextISF *ic = find_ic (context);
    SCIM_DEBUG_FRONTEND(1) << __FUNCTION__ << " context=" << context << " number=" << number << " ic=" << ic << "\n";
    if (ic && ic->impl && ic->impl->si && _focused_ic == ic) {
        _panel_client.prepare (ic->id);
        ic->impl->si->update_displayed_candidate_number (number);
        _panel_client.send ();
    }
}

static void
panel_slot_candidate_more_window_show (int context)
{
    EcoreIMFContextISF *ic = find_ic (context);
    SCIM_DEBUG_FRONTEND(1) << __FUNCTION__ << " context=" << context << " ic=" << ic << "\n";
    if (ic && ic->impl && ic->impl->si && _focused_ic == ic) {
        _panel_client.prepare (ic->id);
        ic->impl->si->candidate_more_window_show ();
        _panel_client.send ();
    }
}

static void
panel_slot_candidate_more_window_hide (int context)
{
    EcoreIMFContextISF *ic = find_ic (context);
    SCIM_DEBUG_FRONTEND(1) << __FUNCTION__ << " context=" << context << " ic=" << ic << "\n";
    if (ic && ic->impl && ic->impl->si && _focused_ic == ic) {
        _panel_client.prepare (ic->id);
        ic->impl->si->candidate_more_window_hide ();
        _panel_client.send ();
    }
}

static void
panel_slot_longpress_candidate (int context, int index)
{
    EcoreIMFContextISF *ic = find_ic (context);
    SCIM_DEBUG_FRONTEND(1) << __FUNCTION__ << " context=" << context << " index=" << index << " ic=" << ic << "\n";
    if (ic && ic->impl && ic->impl->si && _focused_ic == ic) {
        _panel_client.prepare (ic->id);
        ic->impl->si->longpress_candidate (index);
        _panel_client.send ();
    }
}

static void
panel_slot_update_ise_input_context (int context, int type, int value)
{
    SCIM_DEBUG_FRONTEND(1) << __FUNCTION__ << "...\n";

    process_update_input_context (type, value);
}

static void
panel_slot_update_isf_candidate_panel (int context, int type, int value)
{
    SCIM_DEBUG_FRONTEND(1) << __FUNCTION__ << "...\n";

    process_update_input_context (type, value);
}

static void
panel_slot_send_private_command (int context, const String &command)
{
    SCIM_DEBUG_FRONTEND(1) << __FUNCTION__ << "...\n";

    EcoreIMFContextISF *ic = find_ic (context);

    if (ic && ic->impl && ic->impl->si && _focused_ic == ic)
        slot_send_private_command (ic->impl->si, command);
}

/* Panel Requestion functions. */
static void
panel_req_show_help (EcoreIMFContextISF *ic)
{
    SCIM_DEBUG_FRONTEND(1) << __FUNCTION__ << "...\n";

    String help;

    help =  String (_("Smart Common Input Method platform ")) +
            String (SCIM_VERSION) +
            String (_("\n(C) 2002-2005 James Su <suzhe@tsinghua.org.cn>\n\n"));

    if (ic && ic->impl && ic->impl->si) {
        IMEngineFactoryPointer sf = _backend->get_factory (ic->impl->si->get_factory_uuid ());
        if (sf) {
            help += utf8_wcstombs (sf->get_name ());
            help += String (_(":\n\n"));

            help += utf8_wcstombs (sf->get_help ());
            help += String (_("\n\n"));

            help += utf8_wcstombs (sf->get_credits ());
        }
        _panel_client.show_help (ic->id, help);
    }
}

static void
panel_req_show_factory_menu (EcoreIMFContextISF *ic)
{
    SCIM_DEBUG_FRONTEND(1) << __FUNCTION__ << "...\n";

    std::vector<IMEngineFactoryPointer> factories;
    std::vector <PanelFactoryInfo> menu;

    _backend->get_factories_for_encoding (factories, "UTF-8");

    for (size_t i = 0; i < factories.size (); ++ i) {
        menu.push_back (PanelFactoryInfo (
                            factories [i]->get_uuid (),
                            utf8_wcstombs (factories [i]->get_name ()),
                            factories [i]->get_language (),
                            factories [i]->get_icon_file ()));
    }

    if (menu.size ())
        _panel_client.show_factory_menu (ic->id, menu);
}

static void
panel_req_update_factory_info (EcoreIMFContextISF *ic)
{
    SCIM_DEBUG_FRONTEND(1) << __FUNCTION__ << "...\n";

    if (ic && ic->impl && ic->impl->si && ic == _focused_ic) {
        PanelFactoryInfo info;
        if (ic->impl->is_on) {
            IMEngineFactoryPointer sf = _backend->get_factory (ic->impl->si->get_factory_uuid ());
            if (sf)
                info = PanelFactoryInfo (sf->get_uuid (), utf8_wcstombs (sf->get_name ()), sf->get_language (), sf->get_icon_file ());
        } else {
            info = PanelFactoryInfo (String (""), String (_("English Keyboard")), String ("C"), String (SCIM_KEYBOARD_ICON_FILE));
        }
        _panel_client.update_factory_info (ic->id, info);
    }
}

static void
panel_req_focus_in (EcoreIMFContextISF *ic)
{
    SCIM_DEBUG_FRONTEND(1) << __FUNCTION__ << "...\n";

    if (ic && ic->impl && ic->impl->si)
        _panel_client.focus_in (ic->id, ic->impl->si->get_factory_uuid ());
}

static void
panel_req_update_spot_location (EcoreIMFContextISF *ic)
{
    SCIM_DEBUG_FRONTEND(1) << __FUNCTION__ << "...\n";

    if (ic && ic->impl)
        _panel_client.update_spot_location (ic->id, ic->impl->cursor_x, ic->impl->cursor_y, ic->impl->cursor_top_y);
}

static void
panel_req_update_cursor_position (EcoreIMFContextISF *ic, int cursor_pos)
{
    SCIM_DEBUG_FRONTEND(1) << __FUNCTION__ << "...\n";

    if (ic)
        _panel_client.update_cursor_position (ic->id, cursor_pos);
}

static void
panel_req_set_input_hint (EcoreIMFContextISF *ic, int hint)
{
    SCIM_DEBUG_FRONTEND(1) << __FUNCTION__ << "...\n";

    if (ic)
        _panel_client.set_input_hint (ic->id, hint);
}

static void
panel_req_update_bidi_direction (EcoreIMFContextISF *ic, int direction)
{
    SCIM_DEBUG_FRONTEND(1) << __FUNCTION__ << "...\n";

    if (ic)
        _panel_client.update_bidi_direction (ic->id, direction);
}

static bool
filter_hotkeys (EcoreIMFContextISF *ic, const KeyEvent &key)
{
    SCIM_DEBUG_FRONTEND(1) << __FUNCTION__ << "...\n";

    bool ret = false;

    if (!check_valid_ic (ic))
        return ret;

    _frontend_hotkey_matcher.push_key_event (key);
    _imengine_hotkey_matcher.push_key_event (key);

    FrontEndHotkeyAction hotkey_action = _frontend_hotkey_matcher.get_match_result ();

    if (hotkey_action == SCIM_FRONTEND_HOTKEY_TRIGGER) {
        IMEngineFactoryPointer sf = _backend->get_factory (ic->impl->si->get_factory_uuid ());
        if (sf && (sf->get_option () & SCIM_IME_SUPPORT_LANGUAGE_TOGGLE_KEY)) {
            ret = false;
        } else {
            if (!ic->impl->is_on) {
                turn_on_ic (ic);
            } else {
                turn_off_ic (ic);
                _panel_client.hide_aux_string (ic->id);
                _panel_client.hide_lookup_table (ic->id);
            }

            ret = true;
        }
    } else if (hotkey_action == SCIM_FRONTEND_HOTKEY_ON) {
        if (!ic->impl->is_on) {
            turn_on_ic (ic);
        }
        ret = true;
    } else if (hotkey_action == SCIM_FRONTEND_HOTKEY_OFF) {
        if (ic->impl->is_on) {
            turn_off_ic (ic);
        }
        ret = true;
    } else if (hotkey_action == SCIM_FRONTEND_HOTKEY_NEXT_FACTORY) {
        open_next_factory (ic);
        ret = true;
    } else if (hotkey_action == SCIM_FRONTEND_HOTKEY_PREVIOUS_FACTORY) {
        open_previous_factory (ic);
        ret = true;
    } else if (hotkey_action == SCIM_FRONTEND_HOTKEY_SHOW_FACTORY_MENU) {
        panel_req_show_factory_menu (ic);
        ret = true;
    } else if (_imengine_hotkey_matcher.is_matched ()) {
        ISEInfo info = _imengine_hotkey_matcher.get_match_result ();
        ISE_TYPE type = info.type;
        if (type == IMENGINE_T)
            open_specific_factory (ic, info.uuid);
        else if (type == HELPER_T)
            _panel_client.start_helper (ic->id, info.uuid);
        ret = true;
    }
    return ret;
}

static bool
filter_keys (const char *keyname, std::vector <String> &keys)
{
    SCIM_DEBUG_FRONTEND(1) << __FUNCTION__ << "...\n";

    if (!keyname)
        return false;

    for (unsigned int i = 0; i < keys.size (); ++i) {
        if (!strcmp (keyname, keys [i].c_str ())) {
            return true;
        }
    }

    return false;
}

static bool
check_socket_frontend (void)
{
    SCIM_DEBUG_FRONTEND(1) << __FUNCTION__ << "...\n";

    SocketAddress address;
    SocketClient client;

    uint32 magic;

    address.set_address (scim_get_default_socket_frontend_address ());

    if (!client.connect (address)) {
        LOGW ("check_socket_frontend's connect () failed.\n");
        return false;
    }

    if (!scim_socket_open_connection (magic,
                                      String ("ConnectionTester"),
                                      String ("SocketFrontEnd"),
                                      client,
                                      500)) {
        LOGW ("check_socket_frontend's scim_socket_open_connection () failed.\n");
        return false;
    }

    return true;
}

static int
launch_socket_frontend ()
{
    SCIM_DEBUG_FRONTEND(1) << __FUNCTION__ << "...\n";

    std::vector<String>     engine_list;
    std::vector<String>     helper_list;
    std::vector<String>     load_engine_list;

    std::vector<String>::iterator it;

    std::cerr << "Launching a ISF daemon with Socket FrontEnd...\n";
    //get modules list
    scim_get_imengine_module_list (engine_list);
    scim_get_helper_module_list (helper_list);

    for (it = engine_list.begin (); it != engine_list.end (); it++) {
        if (*it != "socket")
            load_engine_list.push_back (*it);
    }
    for (it = helper_list.begin (); it != helper_list.end (); it++)
        load_engine_list.push_back (*it);

    return scim_launch (true,
        "simple",
        (load_engine_list.size () > 0 ? scim_combine_string_list (load_engine_list, ',') : "none"),
        "socket",
        NULL);
}

static bool
panel_initialize (void)
{
    SCIM_DEBUG_FRONTEND(1) << __FUNCTION__ << "...\n";

    /* Before initializing panel client, make sure the socket frontend is running */
    if (!check_socket_frontend ()) {
        bool connected = false;
        /* Make sure we are not retrying for more than 5 seconds, in total */
        for (int i = 0; i < 3; ++i) {
            if (check_socket_frontend ()) {
                connected = true;
                break;
            }
            scim_usleep (100000);
        }
        if (!connected) {
            LOGD ("Socket FrontEnd does not exist, launching new one\n");
            launch_socket_frontend ();
        }
    }

    String display_name;
    {
        const char *p = getenv ("DISPLAY");
        if (p) display_name = String (p);
    }

    if (_panel_client.open_connection (_config->get_name (), display_name) >= 0) {
        if (_panel_client.get_client_id (_panel_client_id)) {
            _panel_client.prepare (0);
            _panel_client.register_client (_panel_client_id);
            _panel_client.send ();
        }

        int fd = _panel_client.get_connection_number ();

        _panel_iochannel_read_handler = ecore_main_fd_handler_add (fd, ECORE_FD_READ, panel_iochannel_handler, NULL, NULL, NULL);
//        _panel_iochannel_err_handler  = ecore_main_fd_handler_add (fd, ECORE_FD_ERROR, panel_iochannel_handler, NULL, NULL, NULL);

        SCIM_DEBUG_FRONTEND(2) << " Panel FD= " << fd << "\n";

        EcoreIMFContextISF *context_scim = _ic_list;
        while (context_scim != NULL) {
            if (context_scim->impl && context_scim->impl->si) {
                _panel_client.prepare (context_scim->id);
                _panel_client.register_input_context (context_scim->id, context_scim->impl->si->get_factory_uuid ());
                _panel_client.send ();
            }
            context_scim = context_scim->next;
        }

        if (_focused_ic) {
            scim_usleep (2000000);  // Wait for ISE ready
            Ecore_IMF_Context *ctx = _focused_ic->ctx;
            _focused_ic = 0;
            if (input_panel_state == ECORE_IMF_INPUT_PANEL_STATE_SHOW) {
                input_panel_state = ECORE_IMF_INPUT_PANEL_STATE_HIDE;
                isf_imf_context_focus_in (ctx);
                ecore_imf_context_input_panel_show (ctx);
            } else {
                isf_imf_context_focus_in (ctx);
            }
        }

        _panel_initialized = true;

        return true;
    }
    std::cerr << "panel_initialize () failed!!!\n";
    return false;
}

static void
panel_finalize (void)
{
    SCIM_DEBUG_FRONTEND(1) << __FUNCTION__ << "...\n";

    _panel_initialized = false;
    _panel_client.close_connection ();

    if (_panel_iochannel_read_handler) {
        ecore_main_fd_handler_del (_panel_iochannel_read_handler);
        _panel_iochannel_read_handler = 0;
    }
    if (_panel_iochannel_err_handler) {
        ecore_main_fd_handler_del (_panel_iochannel_err_handler);
        _panel_iochannel_err_handler = 0;
    }
}

static Eina_Bool
panel_iochannel_handler (void *data, Ecore_Fd_Handler *fd_handler)
{
    SCIM_DEBUG_FRONTEND(1) << __FUNCTION__ << "...\n";

    if (fd_handler == _panel_iochannel_read_handler) {
        if (_panel_client.has_pending_event () && !_panel_client.filter_event ()) {
            panel_finalize ();
            return ECORE_CALLBACK_CANCEL;
        }
    } else if (fd_handler == _panel_iochannel_err_handler) {
        panel_finalize ();
        return ECORE_CALLBACK_CANCEL;
    }
    return ECORE_CALLBACK_RENEW;
}

static void
turn_on_ic (EcoreIMFContextISF *ic)
{
    SCIM_DEBUG_FRONTEND(1) << __FUNCTION__ << "...\n";

    if (ic && ic->impl && ic->impl->si && !ic->impl->is_on) {
        ic->impl->is_on = true;

        if (ic == _focused_ic) {
            panel_req_focus_in (ic);
//            panel_req_update_spot_location (ic);
            panel_req_update_factory_info (ic);
            _panel_client.turn_on (ic->id);
//            _panel_client.hide_preedit_string (ic->id);
//            _panel_client.hide_aux_string (ic->id);
//            _panel_client.hide_lookup_table (ic->id);
            ic->impl->si->focus_in ();
            ic->impl->si->set_layout (ecore_imf_context_input_panel_layout_get (ic->ctx));
            ic->impl->si->set_prediction_allow (ic->impl->prediction_allow);
            LOGD ("ctx : %p. set autocapital type : %d\n", ic->ctx, ic->impl->autocapital_type);
            ic->impl->si->set_autocapital_type (ic->impl->autocapital_type);
            ic->impl->si->set_input_hint (ic->impl->input_hint);
            ic->impl->si->update_bidi_direction (ic->impl->bidi_direction);
        }

        //Record the IC on/off status
        if (_shared_input_method) {
            _config->write (String (SCIM_CONFIG_FRONTEND_IM_OPENED_BY_DEFAULT), true);
            _config->flush ();
        }

        if (ic->impl->use_preedit && ic->impl->preedit_string.length ()) {
            ecore_imf_context_preedit_start_event_add (ic->ctx);
            ecore_imf_context_event_callback_call (ic->ctx, ECORE_IMF_CALLBACK_PREEDIT_START, NULL);
            if (!check_valid_ic (ic))
                return;

            ecore_imf_context_preedit_changed_event_add (ic->ctx);
            ecore_imf_context_event_callback_call (ic->ctx, ECORE_IMF_CALLBACK_PREEDIT_CHANGED, NULL);

            if (check_valid_ic (ic))
                ic->impl->preedit_started = true;
        }
    }
}

static void
turn_off_ic (EcoreIMFContextISF *ic)
{
    SCIM_DEBUG_FRONTEND(1) << __FUNCTION__ << "...\n";

    if (ic && ic->impl && ic->impl->is_on) {
        ic->impl->is_on = false;

        if (ic == _focused_ic) {
            if (ic->impl->si)
                ic->impl->si->focus_out ();

//            panel_req_update_factory_info (ic);
            _panel_client.turn_off (ic->id);
            _panel_client.hide_aux_string (ic->id);
            _panel_client.hide_lookup_table (ic->id);
        }

        //Record the IC on/off status
        if (_shared_input_method) {
            _config->write (String (SCIM_CONFIG_FRONTEND_IM_OPENED_BY_DEFAULT), false);
            _config->flush ();
        }

        if (ic->impl->use_preedit && ic->impl->preedit_string.length ()) {
            ecore_imf_context_preedit_changed_event_add (ic->ctx);
            ecore_imf_context_event_callback_call (ic->ctx, ECORE_IMF_CALLBACK_PREEDIT_CHANGED, NULL);
            if (!check_valid_ic (ic))
                return;

            ecore_imf_context_preedit_end_event_add (ic->ctx);
            ecore_imf_context_event_callback_call (ic->ctx, ECORE_IMF_CALLBACK_PREEDIT_END, NULL);

            if (check_valid_ic (ic))
                ic->impl->preedit_started = false;
        }
    }
}

static void
set_ic_capabilities (EcoreIMFContextISF *ic)
{
    SCIM_DEBUG_FRONTEND(1) << __FUNCTION__ << "...\n";

    if (ic && ic->impl) {
        unsigned int cap = SCIM_CLIENT_CAP_ALL_CAPABILITIES;

        if (!_on_the_spot || !ic->impl->use_preedit)
            cap -= SCIM_CLIENT_CAP_ONTHESPOT_PREEDIT;

        if (ic->impl->si)
            ic->impl->si->update_client_capabilities (cap);
    }
}

void
initialize (void)
{
    std::vector<String>     config_list;
    std::vector<String>     engine_list;
    std::vector<String>     helper_list;
    std::vector<String>     load_engine_list;

    std::vector<String>::iterator it;

    bool                    manual = false;
    bool                    socket = true;
    String                  config_module_name = "simple";
    int                     ret    = -1;

    LOGD ("Initializing Ecore ISF IMModule...\n");

    // Get system language.
    _language = scim_get_locale_language (scim_get_current_locale ());

    if (socket) {
        // If no Socket FrontEnd is running, then launch one.
        // And set manual to false.
        bool check_result = check_socket_frontend ();

        /* Make sure we are not retrying for more than 5 seconds, in total */
        for (int i = 0; i < 3 && !check_result; ++i) {
            check_result = check_socket_frontend ();
            scim_usleep (100000);
        }

        if (!check_result) {
            //get modules list
            scim_get_imengine_module_list (engine_list);
            scim_get_helper_module_list (helper_list);

            for (it = engine_list.begin (); it != engine_list.end (); it++) {
                if (*it != "socket")
                    load_engine_list.push_back (*it);
            }
            for (it = helper_list.begin (); it != helper_list.end (); it++)
                load_engine_list.push_back (*it);

            launch_socket_frontend ();
            manual = false;
        }

        // If there is one Socket FrontEnd running and it's not manual mode,
        // then just use this Socket Frontend.
        if (!manual) {
            for (int i = 0; i < 3; ++i) {
                if (check_result) {
                    config_module_name = "socket";
                    load_engine_list.clear ();
                    load_engine_list.push_back ("socket");
                    break;
                }
                scim_usleep (100000);
                check_result = check_socket_frontend ();
            }
        }
    }

    if (config_module_name != "dummy") {
        _config = ConfigBase::get (true, config_module_name);
    }

    if (_config.null ()) {
        LOGW ("Config module cannot be loaded, using dummy Config.\n");
        _config = new DummyConfig ();
        config_module_name = "dummy";
    }

    reload_config_callback (_config);
    _config_connection = _config->signal_connect_reload (slot (reload_config_callback));

    // create backend
    _backend = new CommonBackEnd (_config, load_engine_list.size () > 0 ? load_engine_list : engine_list);

    if (_backend.null ()) {
        std::cerr << "Cannot create BackEnd Object!\n";
    } else {
        _backend->initialize (_config, load_engine_list.size () > 0 ? load_engine_list : engine_list, false, false);
        _fallback_factory = _backend->get_factory (SCIM_COMPOSE_KEY_FACTORY_UUID);
    }

    if (_fallback_factory.null ())
        _fallback_factory = new DummyIMEngineFactory ();

    _fallback_instance = _fallback_factory->create_instance (String ("UTF-8"), 0);
    _fallback_instance->signal_connect_commit_string (slot (fallback_commit_string_cb));

    /* get the input window */
    Ecore_X_Atom atom = ecore_x_atom_get (E_PROP_DEVICEMGR_INPUTWIN);
    ret = ecore_x_window_prop_xid_get (ecore_x_window_root_first_get (), atom, ECORE_X_ATOM_WINDOW, &_input_win, 1);
    if (_input_win == 0 || ret < 1)
    {
        LOGW ("Input window is NULL!\n");
    }
    // Attach Panel Client signal.
    _panel_client.signal_connect_reload_config                 (slot (panel_slot_reload_config));
    _panel_client.signal_connect_exit                          (slot (panel_slot_exit));
    _panel_client.signal_connect_update_candidate_item_layout  (slot (panel_slot_update_candidate_item_layout));
    _panel_client.signal_connect_update_lookup_table_page_size (slot (panel_slot_update_lookup_table_page_size));
    _panel_client.signal_connect_lookup_table_page_up          (slot (panel_slot_lookup_table_page_up));
    _panel_client.signal_connect_lookup_table_page_down        (slot (panel_slot_lookup_table_page_down));
    _panel_client.signal_connect_trigger_property              (slot (panel_slot_trigger_property));
    _panel_client.signal_connect_process_helper_event          (slot (panel_slot_process_helper_event));
    _panel_client.signal_connect_move_preedit_caret            (slot (panel_slot_move_preedit_caret));
    _panel_client.signal_connect_update_preedit_caret          (slot (panel_slot_update_preedit_caret));
    _panel_client.signal_connect_select_aux                    (slot (panel_slot_select_aux));
    _panel_client.signal_connect_select_candidate              (slot (panel_slot_select_candidate));
    _panel_client.signal_connect_process_key_event             (slot (panel_slot_process_key_event));
    _panel_client.signal_connect_commit_string                 (slot (panel_slot_commit_string));
    _panel_client.signal_connect_forward_key_event             (slot (panel_slot_forward_key_event));
    _panel_client.signal_connect_request_help                  (slot (panel_slot_request_help));
    _panel_client.signal_connect_request_factory_menu          (slot (panel_slot_request_factory_menu));
    _panel_client.signal_connect_change_factory                (slot (panel_slot_change_factory));
    _panel_client.signal_connect_reset_keyboard_ise            (slot (panel_slot_reset_keyboard_ise));
    _panel_client.signal_connect_update_keyboard_ise           (slot (panel_slot_update_keyboard_ise));
    _panel_client.signal_connect_show_preedit_string           (slot (panel_slot_show_preedit_string));
    _panel_client.signal_connect_hide_preedit_string           (slot (panel_slot_hide_preedit_string));
    _panel_client.signal_connect_update_preedit_string         (slot (panel_slot_update_preedit_string));
    _panel_client.signal_connect_get_surrounding_text          (slot (panel_slot_get_surrounding_text));
    _panel_client.signal_connect_delete_surrounding_text       (slot (panel_slot_delete_surrounding_text));
    _panel_client.signal_connect_get_selection                 (slot (panel_slot_get_selection));
    _panel_client.signal_connect_set_selection                 (slot (panel_slot_set_selection));
    _panel_client.signal_connect_update_displayed_candidate_number (slot (panel_slot_update_displayed_candidate_number));
    _panel_client.signal_connect_candidate_more_window_show    (slot (panel_slot_candidate_more_window_show));
    _panel_client.signal_connect_candidate_more_window_hide    (slot (panel_slot_candidate_more_window_hide));
    _panel_client.signal_connect_longpress_candidate           (slot (panel_slot_longpress_candidate));
    _panel_client.signal_connect_update_ise_input_context      (slot (panel_slot_update_ise_input_context));
    _panel_client.signal_connect_update_isf_candidate_panel    (slot (panel_slot_update_isf_candidate_panel));
    _panel_client.signal_connect_send_private_command          (slot (panel_slot_send_private_command));

    if (!panel_initialize ()) {
        std::cerr << "Ecore IM Module: Cannot connect to Panel!\n";
    }
}

static void
finalize (void)
{
    LOGD ("Finalizing Ecore ISF IMModule...\n");

    // Reset this first so that the shared instance could be released correctly afterwards.
    _default_instance.reset ();

    SCIM_DEBUG_FRONTEND(2) << "Finalize all IC partially.\n";
    while (_used_ic_impl_list) {
        // In case in "shared input method" mode,
        // all contexts share only one instance,
        // so we need point the reference pointer correctly before finalizing.
        _used_ic_impl_list->si->set_frontend_data (static_cast <void*> (_used_ic_impl_list->parent));
        isf_imf_context_del (_used_ic_impl_list->parent->ctx);
    }

    delete_all_ic_impl ();

    _fallback_instance.reset ();
    _fallback_factory.reset ();

    SCIM_DEBUG_FRONTEND(2) << " Releasing BackEnd...\n";
    _backend.reset ();

    SCIM_DEBUG_FRONTEND(2) << " Releasing Config...\n";
    _config.reset ();
    _config_connection.disconnect ();
    ConfigBase::set (0);
    _focused_ic = NULL;
    _ic_list = NULL;

    _scim_initialized = false;

    _panel_client.reset_signal_handler ();
    panel_finalize ();
}

static void
open_next_factory (EcoreIMFContextISF *ic)
{
    if (!check_valid_ic (ic))
        return;

    SCIM_DEBUG_FRONTEND(2) << __FUNCTION__ << " context=" << ic->id << "\n";
    IMEngineFactoryPointer sf = _backend->get_next_factory ("", "UTF-8", ic->impl->si->get_factory_uuid ());

    if (!sf.null ()) {
        turn_off_ic (ic);
        ic->impl->si = sf->create_instance ("UTF-8", ic->impl->si->get_id ());
        ic->impl->si->set_frontend_data (static_cast <void*> (ic));
        ic->impl->preedit_string = WideString ();
        ic->impl->preedit_caret = 0;
        attach_instance (ic->impl->si);
        _backend->set_default_factory (_language, sf->get_uuid ());
        _panel_client.register_input_context (ic->id, sf->get_uuid ());
        _panel_client.set_candidate_style (ic->id, ONE_LINE_CANDIDATE, FIXED_CANDIDATE_WINDOW);
        set_ic_capabilities (ic);
        turn_on_ic (ic);

        if (_shared_input_method) {
            _default_instance = ic->impl->si;
            ic->impl->shared_si = true;
        }
    }
}

static void
open_previous_factory (EcoreIMFContextISF *ic)
{
    if (!check_valid_ic (ic))
        return;

    SCIM_DEBUG_FRONTEND(2) << __FUNCTION__ << " context=" << ic->id << "\n";
    IMEngineFactoryPointer sf = _backend->get_previous_factory ("", "UTF-8", ic->impl->si->get_factory_uuid ());

    if (!sf.null ()) {
        turn_off_ic (ic);
        ic->impl->si = sf->create_instance ("UTF-8", ic->impl->si->get_id ());
        ic->impl->si->set_frontend_data (static_cast <void*> (ic));
        ic->impl->preedit_string = WideString ();
        ic->impl->preedit_caret = 0;
        attach_instance (ic->impl->si);
        _backend->set_default_factory (_language, sf->get_uuid ());
        _panel_client.register_input_context (ic->id, sf->get_uuid ());
        _panel_client.set_candidate_style (ic->id, ONE_LINE_CANDIDATE, FIXED_CANDIDATE_WINDOW);
        set_ic_capabilities (ic);
        turn_on_ic (ic);

        if (_shared_input_method) {
            _default_instance = ic->impl->si;
            ic->impl->shared_si = true;
        }
    }
}

static void
open_specific_factory (EcoreIMFContextISF *ic,
                       const String     &uuid)
{
    if (!check_valid_ic (ic))
        return;

    SCIM_DEBUG_FRONTEND(2) << __FUNCTION__ << " context=" << ic->id << "\n";

    // The same input method is selected, just turn on the IC.
    if (ic->impl->si && (ic->impl->si->get_factory_uuid () == uuid)) {
        turn_on_ic (ic);
        return;
    }

    IMEngineFactoryPointer sf = _backend->get_factory (uuid);

    if (uuid.length () && !sf.null ()) {
        turn_off_ic (ic);
        ic->impl->si = sf->create_instance ("UTF-8", ic->impl->si->get_id ());
        ic->impl->si->set_frontend_data (static_cast <void*> (ic));
        ic->impl->preedit_string = WideString ();
        ic->impl->preedit_caret = 0;
        attach_instance (ic->impl->si);
        _backend->set_default_factory (_language, sf->get_uuid ());
        _panel_client.register_input_context (ic->id, sf->get_uuid ());
        set_ic_capabilities (ic);
        turn_on_ic (ic);

        if (_shared_input_method) {
            _default_instance = ic->impl->si;
            ic->impl->shared_si = true;
        }
    } else {
        std::cerr << "open_specific_factory () is failed!!!!!!\n";
        LOGW ("open_specific_factory () is failed. uuid : %s", uuid.c_str ());

        // turn_off_ic comment out panel_req_update_factory_info ()
        //turn_off_ic (ic);
        if (ic->impl->is_on) {
            ic->impl->is_on = false;

            if (ic == _focused_ic) {
                ic->impl->si->focus_out ();

                panel_req_update_factory_info (ic);
                _panel_client.turn_off (ic->id);
            }

            //Record the IC on/off status
            if (_shared_input_method) {
                _config->write (String (SCIM_CONFIG_FRONTEND_IM_OPENED_BY_DEFAULT), false);
                _config->flush ();
            }

            if (ic->impl->use_preedit && ic->impl->preedit_string.length ()) {
                ecore_imf_context_preedit_changed_event_add (ic->ctx);
                ecore_imf_context_event_callback_call (ic->ctx, ECORE_IMF_CALLBACK_PREEDIT_CHANGED, NULL);
                if (!check_valid_ic (ic))
                    return;

                ecore_imf_context_preedit_end_event_add (ic->ctx);
                ecore_imf_context_event_callback_call (ic->ctx, ECORE_IMF_CALLBACK_PREEDIT_END, NULL);

                if (check_valid_ic (ic))
                    ic->impl->preedit_started = false;
            }
        }
    }
}

static void initialize_modifier_bits (Display *display)
{
    SCIM_DEBUG_FRONTEND(1) << __FUNCTION__ << "...\n";

    if (__current_display == display)
        return;

    __current_display = display;

    if (display == 0) {
        __current_alt_mask     = Mod1Mask;
        __current_meta_mask    = ShiftMask | Mod1Mask;
        __current_super_mask   = 0;
        __current_hyper_mask   = 0;
        __current_numlock_mask = Mod2Mask;
        return;
    }

    XModifierKeymap *mods = NULL;

    ::KeyCode ctrl_l  = XKeysymToKeycode (display, XK_Control_L);
    ::KeyCode ctrl_r  = XKeysymToKeycode (display, XK_Control_R);
    ::KeyCode meta_l  = XKeysymToKeycode (display, XK_Meta_L);
    ::KeyCode meta_r  = XKeysymToKeycode (display, XK_Meta_R);
    ::KeyCode alt_l   = XKeysymToKeycode (display, XK_Alt_L);
    ::KeyCode alt_r   = XKeysymToKeycode (display, XK_Alt_R);
    ::KeyCode super_l = XKeysymToKeycode (display, XK_Super_L);
    ::KeyCode super_r = XKeysymToKeycode (display, XK_Super_R);
    ::KeyCode hyper_l = XKeysymToKeycode (display, XK_Hyper_L);
    ::KeyCode hyper_r = XKeysymToKeycode (display, XK_Hyper_R);
    ::KeyCode numlock = XKeysymToKeycode (display, XK_Num_Lock);

    int i, j;

    mods = XGetModifierMapping (display);
    if (mods == NULL)
        return;

    __current_alt_mask     = 0;
    __current_meta_mask    = 0;
    __current_super_mask   = 0;
    __current_hyper_mask   = 0;
    __current_numlock_mask = 0;

    /* We skip the first three sets for Shift, Lock, and Control.  The
        remaining sets are for Mod1, Mod2, Mod3, Mod4, and Mod5.  */
    for (i = 3; i < 8; i++) {
        for (j = 0; j < mods->max_keypermod; j++) {
            ::KeyCode code = mods->modifiermap [i * mods->max_keypermod + j];
            if (! code) continue;
            if (code == alt_l || code == alt_r)
                __current_alt_mask |= (1 << i);
            else if (code == meta_l || code == meta_r)
                __current_meta_mask |= (1 << i);
            else if (code == super_l || code == super_r)
                __current_super_mask |= (1 << i);
            else if (code == hyper_l || code == hyper_r)
                __current_hyper_mask |= (1 << i);
            else if (code == numlock)
                __current_numlock_mask |= (1 << i);
        }
    }

    /* Check whether there is a combine keys mapped to Meta */
    if (__current_meta_mask == 0) {
        char buf [32];
        XKeyEvent xkey;
        KeySym keysym_l, keysym_r;

        xkey.type = KeyPress;
        xkey.display = display;
        xkey.serial = 0L;
        xkey.send_event = False;
        xkey.x = xkey.y = xkey.x_root = xkey.y_root = 0;
        xkey.time = 0;
        xkey.same_screen = False;
        xkey.subwindow = None;
        xkey.window = None;
        xkey.root = DefaultRootWindow (display);
        xkey.state = ShiftMask;

        xkey.keycode = meta_l;
        XLookupString (&xkey, buf, 32, &keysym_l, 0);
        xkey.keycode = meta_r;
        XLookupString (&xkey, buf, 32, &keysym_r, 0);

        if ((meta_l == alt_l && keysym_l == XK_Meta_L) || (meta_r == alt_r && keysym_r == XK_Meta_R))
            __current_meta_mask = ShiftMask + __current_alt_mask;
        else if ((meta_l == ctrl_l && keysym_l == XK_Meta_L) || (meta_r == ctrl_r && keysym_r == XK_Meta_R))
            __current_meta_mask = ShiftMask + ControlMask;
    }

    XFreeModifiermap (mods);
}

static unsigned int scim_x11_keymask_scim_to_x11 (Display *display, uint16 scimkeymask)
{
    SCIM_DEBUG_FRONTEND(1) << __FUNCTION__ << "...\n";

    unsigned int state = 0;

    initialize_modifier_bits (display);

    if (scimkeymask & SCIM_KEY_ShiftMask)    state |= ShiftMask;
    if (scimkeymask & SCIM_KEY_CapsLockMask) state |= LockMask;
    if (scimkeymask & SCIM_KEY_ControlMask)  state |= ControlMask;
    if (scimkeymask & SCIM_KEY_AltMask)      state |= __current_alt_mask;
    if (scimkeymask & SCIM_KEY_MetaMask)     state |= __current_meta_mask;
    if (scimkeymask & SCIM_KEY_SuperMask)    state |= __current_super_mask;
    if (scimkeymask & SCIM_KEY_HyperMask)    state |= __current_hyper_mask;
    if (scimkeymask & SCIM_KEY_NumLockMask)  state |= __current_numlock_mask;

    return state;
}

static XKeyEvent createKeyEvent (bool press, int keycode, int modifiers, bool fake)
{
    SCIM_DEBUG_FRONTEND(1) << __FUNCTION__ << "...\n";

    XKeyEvent event;
    Window focus_win;
    Display *display = (Display *)ecore_x_display_get ();
    int revert = RevertToParent;

    XGetInputFocus (display, &focus_win, &revert);

    event.display     = display;
    event.window      = focus_win;
    event.root        = DefaultRootWindow (display);
    event.subwindow   = None;
    if (fake)
        event.time    = 0;
    else
        event.time    = 1;

    event.x           = 1;
    event.y           = 1;
    event.x_root      = 1;
    event.y_root      = 1;
    event.same_screen = True;
    event.state       = modifiers;
    event.keycode     = keycode;
    if (press)
        event.type = KeyPress;
    else
        event.type = KeyRelease;
    event.send_event  = False;
    event.serial      = 0;

    return event;
}

static void send_x_key_event (const KeyEvent &key, bool fake)
{
    SCIM_DEBUG_FRONTEND(1) << __FUNCTION__ << "...\n";

    ::KeyCode keycode = 0;
    ::KeySym keysym = 0;
    bool shift = false;
    char key_string[256] = {0};
    char keysym_str[256] = {0};
    Window focus_win;
    int revert = RevertToParent;

    // Obtain the X11 display.
    Display *display = (Display *)ecore_x_display_get ();
    if (display == NULL) {
        std::cerr << "ecore_x_display_get () failed\n";
        LOGW ("ecore_x_display_get () failed\n");
        return;
    }

    // Check focus window
    XGetInputFocus (display, &focus_win, &revert);
    if (focus_win == None) {
        LOGW ("Input focus window is None\n");
        return;
    }

    if (strncmp (key.get_key_string ().c_str (), "KeyRelease+", 11) == 0) {
        snprintf (key_string, sizeof (key_string), "%s", key.get_key_string ().c_str () + 11);
    } else {
        snprintf (key_string, sizeof (key_string), "%s", key.get_key_string ().c_str ());
    }

    if (strncmp (key_string, "Shift+", 6) == 0) {
        snprintf (keysym_str, sizeof (keysym_str), "%s", key_string + 6);
    } else {
        snprintf (keysym_str, sizeof (keysym_str), "%s", key_string);
    }

    // get x keysym, keycode, keyname, and key
    keysym = XStringToKeysym (keysym_str);
    if (keysym == NoSymbol) {
        SECURE_LOGW ("NoSymbol\n");
        return;
    }

    keycode = _keyname_to_keycode (keysym_str);
    if (XkbKeycodeToKeysym (display, keycode, 0, 0) != keysym) {
        if (XkbKeycodeToKeysym (display, keycode, 0, 1) == keysym)
            shift = true;
        else
            keycode = 0;
    } else {
        shift = false;
    }

    if (keycode == 0) {
        static int mod = 0;
        KeySym *keysyms;
        int keycode_min, keycode_max, keycode_num;
        int i;

        XDisplayKeycodes (display, &keycode_min, &keycode_max);
        keysyms = XGetKeyboardMapping (display, keycode_min,
                keycode_max - keycode_min + 1,
                &keycode_num);

        if (keysyms) {
            mod = (mod + 1) & 0x7;
            i = (keycode_max - keycode_min - mod - 1) * keycode_num;

            keysyms[i] = keysym;
            XChangeKeyboardMapping (display, keycode_min, keycode_num,
                    keysyms, (keycode_max - keycode_min));
            XFree (keysyms);
            XSync (display, False);
            keycode = keycode_max - mod - 1;
        }

        if (XkbKeycodeToKeysym (display, keycode, 0, 0) != keysym) {
            if (XkbKeycodeToKeysym (display, keycode, 0, 1) == keysym)
                shift = true;
        } else {
            shift = false;
        }
    }

    unsigned int modifier = scim_x11_keymask_scim_to_x11 (display, key.mask);

    if (shift)
        modifier |= ShiftMask;

    XKeyEvent event;
    if (key.is_key_press ()) {
        if (shift) {
            event = createKeyEvent (true, XKeysymToKeycode (display, XK_Shift_L), modifier, fake);
            XSendEvent (event.display, event.window, True, KeyPressMask, (XEvent *)&event);
        }

        event = createKeyEvent (true, keycode, modifier, fake);
        XSendEvent (event.display, event.window, True, KeyPressMask, (XEvent *)&event);
    } else {
        event = createKeyEvent (false, keycode, modifier, fake);
        XSendEvent (event.display, event.window, True, KeyReleaseMask, (XEvent *)&event);

        if (shift) {
            event = createKeyEvent (false, XKeysymToKeycode (display, XK_Shift_L), modifier, fake);
            XSendEvent (event.display, event.window, True, KeyReleaseMask, (XEvent *)&event);
        }
    }
    _x_key_event_is_valid = true;
}

static void
attach_instance (const IMEngineInstancePointer &si)
{
    si->signal_connect_show_preedit_string (
        slot (slot_show_preedit_string));
    si->signal_connect_show_aux_string (
        slot (slot_show_aux_string));
    si->signal_connect_show_lookup_table (
        slot (slot_show_lookup_table));

    si->signal_connect_hide_preedit_string (
        slot (slot_hide_preedit_string));
    si->signal_connect_hide_aux_string (
        slot (slot_hide_aux_string));
    si->signal_connect_hide_lookup_table (
        slot (slot_hide_lookup_table));

    si->signal_connect_update_preedit_caret (
        slot (slot_update_preedit_caret));
    si->signal_connect_update_preedit_string (
        slot (slot_update_preedit_string));
    si->signal_connect_update_aux_string (
        slot (slot_update_aux_string));
    si->signal_connect_update_lookup_table (
        slot (slot_update_lookup_table));

    si->signal_connect_commit_string (
        slot (slot_commit_string));

    si->signal_connect_forward_key_event (
        slot (slot_forward_key_event));

    si->signal_connect_register_properties (
        slot (slot_register_properties));

    si->signal_connect_update_property (
        slot (slot_update_property));

    si->signal_connect_beep (
        slot (slot_beep));

    si->signal_connect_start_helper (
        slot (slot_start_helper));

    si->signal_connect_stop_helper (
        slot (slot_stop_helper));

    si->signal_connect_send_helper_event (
        slot (slot_send_helper_event));

    si->signal_connect_get_surrounding_text (
        slot (slot_get_surrounding_text));

    si->signal_connect_delete_surrounding_text (
        slot (slot_delete_surrounding_text));

    si->signal_connect_get_selection (
        slot (slot_get_selection));

    si->signal_connect_set_selection (
        slot (slot_set_selection));

    si->signal_connect_expand_candidate (
        slot (slot_expand_candidate));
    si->signal_connect_contract_candidate (
        slot (slot_contract_candidate));

    si->signal_connect_set_candidate_style (
        slot (slot_set_candidate_style));

    si->signal_connect_send_private_command (
        slot (slot_send_private_command));
}

// Implementation of slot functions
static void
slot_show_preedit_string (IMEngineInstanceBase *si)
{
    SCIM_DEBUG_FRONTEND(1) << __FUNCTION__ << "...\n";

    EcoreIMFContextISF *ic = static_cast<EcoreIMFContextISF *> (si->get_frontend_data ());

    if (ic && ic->impl && _focused_ic == ic) {
        if (ic->impl->use_preedit) {
            if (!ic->impl->preedit_started) {
                ecore_imf_context_preedit_start_event_add (_focused_ic->ctx);
                ecore_imf_context_event_callback_call (_focused_ic->ctx, ECORE_IMF_CALLBACK_PREEDIT_START, NULL);
                if (check_valid_ic (ic))
                    ic->impl->preedit_started = true;
            }
            //if (ic->impl->preedit_string.length ())
            //    ecore_imf_context_preedit_changed_event_add (_focused_ic->ctx);
        } else {
            _panel_client.show_preedit_string (ic->id);
        }
    }
}

static void
slot_show_aux_string (IMEngineInstanceBase *si)
{
    SCIM_DEBUG_FRONTEND(1) << __FUNCTION__ << "...\n";

    EcoreIMFContextISF *ic = static_cast<EcoreIMFContextISF *> (si->get_frontend_data ());

    if (ic && ic->impl && _focused_ic == ic)
        _panel_client.show_aux_string (ic->id);
}

static void
slot_show_lookup_table (IMEngineInstanceBase *si)
{
    SCIM_DEBUG_FRONTEND(1) << __FUNCTION__ << "...\n";

    EcoreIMFContextISF *ic = static_cast<EcoreIMFContextISF *> (si->get_frontend_data ());

    if (ic && ic->impl && _focused_ic == ic)
        _panel_client.show_lookup_table (ic->id);
}

static void
slot_hide_preedit_string (IMEngineInstanceBase *si)
{
    SCIM_DEBUG_FRONTEND(1) << __FUNCTION__ << "...\n";

    EcoreIMFContextISF *ic = static_cast<EcoreIMFContextISF *> (si->get_frontend_data ());

    if (ic && ic->impl && _focused_ic == ic) {
        bool emit = false;
        if (ic->impl->preedit_string.length ()) {
            ic->impl->preedit_string = WideString ();
            ic->impl->preedit_caret = 0;
            ic->impl->preedit_attrlist.clear ();
            emit = true;
        }
        if (ic->impl->use_preedit) {
            if (emit) {
                ecore_imf_context_preedit_changed_event_add (ic->ctx);
                ecore_imf_context_event_callback_call (ic->ctx, ECORE_IMF_CALLBACK_PREEDIT_CHANGED, NULL);
            }
            if (check_valid_ic (ic) && ic->impl->preedit_started) {
                ecore_imf_context_preedit_end_event_add (ic->ctx);
                ecore_imf_context_event_callback_call (ic->ctx, ECORE_IMF_CALLBACK_PREEDIT_END, NULL);
                if (check_valid_ic (ic))
                    ic->impl->preedit_started = false;
            }
        } else {
            _panel_client.hide_preedit_string (ic->id);
        }
    }
}

static void
slot_hide_aux_string (IMEngineInstanceBase *si)
{
    SCIM_DEBUG_FRONTEND(1) << __FUNCTION__ << "...\n";

    EcoreIMFContextISF *ic = static_cast<EcoreIMFContextISF *> (si->get_frontend_data ());

    if (ic && ic->impl && _focused_ic == ic)
        _panel_client.hide_aux_string (ic->id);
}

static void
slot_hide_lookup_table (IMEngineInstanceBase *si)
{
    SCIM_DEBUG_FRONTEND(1) << __FUNCTION__ << "...\n";

    EcoreIMFContextISF *ic = static_cast<EcoreIMFContextISF *> (si->get_frontend_data ());

    if (ic && ic->impl && _focused_ic == ic)
        _panel_client.hide_lookup_table (ic->id);
}

static void
slot_update_preedit_caret (IMEngineInstanceBase *si, int caret)
{
    SCIM_DEBUG_FRONTEND(1) << __FUNCTION__ << "...\n";

    EcoreIMFContextISF *ic = static_cast<EcoreIMFContextISF *> (si->get_frontend_data ());

    if (ic && ic->impl && _focused_ic == ic && ic->impl->preedit_caret != caret) {
        ic->impl->preedit_caret = caret;
        if (ic->impl->use_preedit) {
            if (!ic->impl->preedit_started) {
                ecore_imf_context_preedit_start_event_add (ic->ctx);
                ecore_imf_context_event_callback_call (ic->ctx, ECORE_IMF_CALLBACK_PREEDIT_START, NULL);
                if (!check_valid_ic (ic))
                    return;

                ic->impl->preedit_started = true;
            }
            ecore_imf_context_preedit_changed_event_add (ic->ctx);
            ecore_imf_context_event_callback_call (ic->ctx, ECORE_IMF_CALLBACK_PREEDIT_CHANGED, NULL);
        } else {
            _panel_client.update_preedit_caret (ic->id, caret);
        }
    }
}

static void
slot_update_preedit_string (IMEngineInstanceBase *si,
                            const WideString & str,
                            const AttributeList & attrs,
                            int caret)
{
    SCIM_DEBUG_FRONTEND(1) << __FUNCTION__ << "...\n";

    EcoreIMFContextISF *ic = static_cast<EcoreIMFContextISF *> (si->get_frontend_data ());

    if (ic && ic->impl && _focused_ic == ic && (ic->impl->preedit_string != str || str.length ())) {
        ic->impl->preedit_string   = str;
        ic->impl->preedit_attrlist = attrs;
        if (ic->impl->use_preedit) {
            if (!ic->impl->preedit_started) {
                ecore_imf_context_preedit_start_event_add (_focused_ic->ctx);
                ecore_imf_context_event_callback_call (_focused_ic->ctx, ECORE_IMF_CALLBACK_PREEDIT_START, NULL);
                if (!check_valid_ic (ic))
                    return;

                ic->impl->preedit_started = true;
            }
            if (caret >= 0 && caret <= (int)str.length ())
                ic->impl->preedit_caret = caret;
            else
                ic->impl->preedit_caret = str.length ();
            ic->impl->preedit_updating = true;
            ecore_imf_context_preedit_changed_event_add (ic->ctx);
            ecore_imf_context_event_callback_call (ic->ctx, ECORE_IMF_CALLBACK_PREEDIT_CHANGED, NULL);
            if (check_valid_ic (ic))
                ic->impl->preedit_updating = false;
        } else {
            _panel_client.update_preedit_string (ic->id, str, attrs, caret);
        }
    }
}

static void
slot_update_aux_string (IMEngineInstanceBase *si,
                        const WideString & str,
                        const AttributeList & attrs)
{
    SCIM_DEBUG_FRONTEND(1) << __FUNCTION__ << "...\n";

    EcoreIMFContextISF *ic = static_cast<EcoreIMFContextISF *> (si->get_frontend_data ());

    if (ic && ic->impl && _focused_ic == ic)
        _panel_client.update_aux_string (ic->id, str, attrs);
}

static void
slot_commit_string (IMEngineInstanceBase *si,
                    const WideString & str)
{
    SCIM_DEBUG_FRONTEND(1) << __FUNCTION__ << "...\n";

    EcoreIMFContextISF *ic = static_cast<EcoreIMFContextISF *> (si->get_frontend_data ());

    if (ic && ic->ctx) {
        if (utf8_wcstombs (str) == String (" ") || utf8_wcstombs (str) == String ("　"))
            autoperiod_insert (ic->ctx);

        Eina_Bool auto_capitalized = EINA_FALSE;

        if (ic->impl) {
            if (ecore_imf_context_input_panel_layout_get (ic->ctx) == ECORE_IMF_INPUT_PANEL_LAYOUT_NORMAL &&
                ic->impl->shift_mode_enabled &&
                ic->impl->autocapital_type != ECORE_IMF_AUTOCAPITAL_TYPE_NONE &&
                get_keyboard_mode () == TOOLBAR_HELPER_MODE) {
                char converted[2] = {'\0'};
                if (utf8_wcstombs (str).length () == 1) {
                    Eina_Bool uppercase;
                    switch (ic->impl->next_shift_status) {
                        case 0:
                            uppercase = caps_mode_check (ic->ctx, EINA_FALSE, EINA_FALSE);
                            break;
                        case SHIFT_MODE_OFF:
                            uppercase = EINA_FALSE;
                            ic->impl->next_shift_status = 0;
                            break;
                        case SHIFT_MODE_ON:
                            uppercase = EINA_TRUE;
                            ic->impl->next_shift_status = 0;
                            break;
                        case SHIFT_MODE_LOCK:
                            uppercase = EINA_TRUE;
                            break;
                        default:
                            uppercase = EINA_FALSE;
                    }
                    converted[0] = utf8_wcstombs (str).at (0);
                    if (uppercase) {
                        if (converted[0] >= 'a' && converted[0] <= 'z')
                            converted[0] -= 32;
                    } else {
                        if (converted[0] >= 'A' && converted[0] <= 'Z')
                            converted[0] += 32;
                    }

                    ecore_imf_context_commit_event_add (ic->ctx, converted);
                    ecore_imf_context_event_callback_call (ic->ctx, ECORE_IMF_CALLBACK_COMMIT, (void *)converted);

                    auto_capitalized = EINA_TRUE;
                }
            }
        }

        if (!auto_capitalized) {
            ecore_imf_context_commit_event_add (ic->ctx, utf8_wcstombs (str).c_str ());
            ecore_imf_context_event_callback_call (ic->ctx, ECORE_IMF_CALLBACK_COMMIT, const_cast<char*>(utf8_wcstombs (str).c_str ()));
        }
    }
}

static void
slot_forward_key_event (IMEngineInstanceBase *si,
                        const KeyEvent & key)
{
    SCIM_DEBUG_FRONTEND(1) << __FUNCTION__ << "...\n";

    EcoreIMFContextISF *ic = static_cast<EcoreIMFContextISF *> (si->get_frontend_data ());

    if (ic && _focused_ic == ic) {
        if (!_fallback_instance->process_key_event (key)) {
            feed_key_event (ic, key, true);
        }
    }
}

static void
slot_update_lookup_table (IMEngineInstanceBase *si,
                          const LookupTable & table)
{
    SCIM_DEBUG_FRONTEND(1) << __FUNCTION__ << "...\n";

    EcoreIMFContextISF *ic = static_cast<EcoreIMFContextISF *> (si->get_frontend_data ());

    if (ic && ic->impl && _focused_ic == ic)
        _panel_client.update_lookup_table (ic->id, table);
}

static void
slot_register_properties (IMEngineInstanceBase *si,
                          const PropertyList & properties)
{
    SCIM_DEBUG_FRONTEND(1) << __FUNCTION__ << "...\n";

    EcoreIMFContextISF *ic = static_cast<EcoreIMFContextISF *> (si->get_frontend_data ());

    if (ic && ic->impl && _focused_ic == ic)
        _panel_client.register_properties (ic->id, properties);
}

static void
slot_update_property (IMEngineInstanceBase *si,
                      const Property & property)
{
    SCIM_DEBUG_FRONTEND(1) << __FUNCTION__ << "...\n";

    EcoreIMFContextISF *ic = static_cast<EcoreIMFContextISF *> (si->get_frontend_data ());

    if (ic && ic->impl && _focused_ic == ic)
        _panel_client.update_property (ic->id, property);
}

static void
slot_beep (IMEngineInstanceBase *si)
{
    SCIM_DEBUG_FRONTEND(1) << __FUNCTION__ << "...\n";

    EcoreIMFContextISF *ic = static_cast<EcoreIMFContextISF *> (si->get_frontend_data ());

    if (ic && ic->impl && _focused_ic == ic)
        ecore_x_bell (0);
}

static void
slot_start_helper (IMEngineInstanceBase *si,
                   const String &helper_uuid)
{
    EcoreIMFContextISF *ic = static_cast<EcoreIMFContextISF *> (si->get_frontend_data ());

    SCIM_DEBUG_FRONTEND(1) << __FUNCTION__ << " helper= " << helper_uuid << " context="
                           << (ic != NULL ? ic->id : -1) << " ic=" << ic
                           << " ic-uuid=" << ((ic && ic->impl && ic->impl->si) ? ic->impl->si->get_factory_uuid () : "") << "...\n";

    if (ic && ic->impl)
        _panel_client.start_helper (ic->id, helper_uuid);
}

static void
slot_stop_helper (IMEngineInstanceBase *si,
                  const String &helper_uuid)
{
    EcoreIMFContextISF *ic = static_cast<EcoreIMFContextISF *> (si->get_frontend_data ());

    SCIM_DEBUG_FRONTEND(1) << __FUNCTION__ << " helper= " << helper_uuid << " context=" << (ic != NULL ? ic->id : -1) << " ic=" << ic << "...\n";

    if (ic && ic->impl)
        _panel_client.stop_helper (ic->id, helper_uuid);
}

static void
slot_send_helper_event (IMEngineInstanceBase *si,
                        const String      &helper_uuid,
                        const Transaction &trans)
{
    EcoreIMFContextISF *ic = static_cast<EcoreIMFContextISF *> (si->get_frontend_data ());

    SCIM_DEBUG_FRONTEND(1) << __FUNCTION__ << " helper= " << helper_uuid << " context="
                           << (ic != NULL ? ic->id : -1) << " ic=" << ic
                           << " ic-uuid=" << ((ic && ic->impl && ic->impl->si) ? ic->impl->si->get_factory_uuid () : "") << "...\n";

    if (ic && ic->impl)
        _panel_client.send_helper_event (ic->id, helper_uuid, trans);
}

static bool
slot_get_surrounding_text (IMEngineInstanceBase *si,
                           WideString            &text,
                           int                   &cursor,
                           int                    maxlen_before,
                           int                    maxlen_after)
{
    SCIM_DEBUG_FRONTEND(1) << __FUNCTION__ << "...\n";

    EcoreIMFContextISF *ic = static_cast<EcoreIMFContextISF *> (si->get_frontend_data ());

    if (ic && ic->impl && _focused_ic == ic) {
        char *surrounding = NULL;
        int   cursor_index;
        if (ecore_imf_context_surrounding_get (_focused_ic->ctx, &surrounding, &cursor_index)) {
            SCIM_DEBUG_FRONTEND(2) << "Surrounding text: " << surrounding <<"\n";
            SCIM_DEBUG_FRONTEND(2) << "Cursor Index    : " << cursor_index <<"\n";

            if (!surrounding)
                return false;

            if (cursor_index < 0) {
                free (surrounding);
                surrounding = NULL;
                return false;
            }

            WideString before = utf8_mbstowcs (String (surrounding));
            free (surrounding);
            surrounding = NULL;

            if (cursor_index > (int)before.length ())
                return false;
            WideString after = before;
            before = before.substr (0, cursor_index);
            after =  after.substr (cursor_index, after.length () - cursor_index);
            if (maxlen_before > 0 && ((unsigned int)maxlen_before) < before.length ())
                before = WideString (before.begin () + (before.length () - maxlen_before), before.end ());
            else if (maxlen_before == 0)
                before = WideString ();
            if (maxlen_after > 0 && ((unsigned int)maxlen_after) < after.length ())
                after = WideString (after.begin (), after.begin () + maxlen_after);
            else if (maxlen_after == 0)
                after = WideString ();
            text = before + after;
            cursor = before.length ();
            return true;
        }
    }
    return false;
}

static bool
slot_delete_surrounding_text (IMEngineInstanceBase *si,
                              int                   offset,
                              int                   len)
{
    SCIM_DEBUG_FRONTEND(1) << __FUNCTION__ << "...\n";

    EcoreIMFContextISF *ic = static_cast<EcoreIMFContextISF *> (si->get_frontend_data ());

    if (_focused_ic && _focused_ic == ic) {
        Ecore_IMF_Event_Delete_Surrounding ev;
        ev.ctx = _focused_ic->ctx;
        ev.n_chars = len;
        ev.offset = offset;
        ecore_imf_context_delete_surrounding_event_add (_focused_ic->ctx, offset, len);
        ecore_imf_context_event_callback_call (_focused_ic->ctx, ECORE_IMF_CALLBACK_DELETE_SURROUNDING, &ev);
        return true;
    }
    return false;
}

static bool
slot_get_selection (IMEngineInstanceBase *si,
                    WideString            &text)
{
    SCIM_DEBUG_FRONTEND(1) << __FUNCTION__ << "...\n";

    EcoreIMFContextISF *ic = static_cast<EcoreIMFContextISF *> (si->get_frontend_data ());

    if (_focused_ic && _focused_ic == ic) {
        char *selection = NULL;
        if (ecore_imf_context_selection_get (_focused_ic->ctx, &selection)) {
            SCIM_DEBUG_FRONTEND(2) << "Selection: " << selection <<"\n";

            if (!selection)
                return false;

            text = utf8_mbstowcs (String (selection));
            free (selection);
            selection = NULL;
            return true;
        }
    }
    return false;
}

static bool
slot_set_selection (IMEngineInstanceBase *si,
                    int              start,
                    int              end)
{
    SCIM_DEBUG_FRONTEND(1) << __FUNCTION__ << "...\n";

    EcoreIMFContextISF *ic = static_cast<EcoreIMFContextISF *> (si->get_frontend_data ());

    if (_focused_ic && _focused_ic == ic) {
        Ecore_IMF_Event_Selection ev;
        ev.ctx = _focused_ic->ctx;
        ev.start = start;
        ev.end = end;
        ecore_imf_context_event_callback_call (_focused_ic->ctx, ECORE_IMF_CALLBACK_SELECTION_SET, &ev);
        return true;
    }
    return false;
}

static void
slot_expand_candidate (IMEngineInstanceBase *si)
{
    SCIM_DEBUG_FRONTEND(1) << __FUNCTION__ << "...\n";

    EcoreIMFContextISF *ic = static_cast<EcoreIMFContextISF *> (si->get_frontend_data ());

    if (ic && ic->impl && _focused_ic == ic)
        _panel_client.expand_candidate (ic->id);
}

static void
slot_contract_candidate (IMEngineInstanceBase *si)
{
    SCIM_DEBUG_FRONTEND(1) << __FUNCTION__ << "...\n";

    EcoreIMFContextISF *ic = static_cast<EcoreIMFContextISF *> (si->get_frontend_data ());

    if (ic && ic->impl && _focused_ic == ic)
        _panel_client.contract_candidate (ic->id);
}

static void
slot_set_candidate_style (IMEngineInstanceBase *si, ISF_CANDIDATE_PORTRAIT_LINE_T portrait_line, ISF_CANDIDATE_MODE_T mode)
{
    SCIM_DEBUG_FRONTEND(1) << __FUNCTION__ << "...\n";

    EcoreIMFContextISF *ic = static_cast<EcoreIMFContextISF *> (si->get_frontend_data ());

    if (ic && ic->impl && _focused_ic == ic)
        _panel_client.set_candidate_style (ic->id, portrait_line, mode);
}

static void
slot_send_private_command (IMEngineInstanceBase *si,
                           const String &command)
{
    SCIM_DEBUG_FRONTEND(1) << __FUNCTION__ << "...\n";

    EcoreIMFContextISF *ic = static_cast<EcoreIMFContextISF *> (si->get_frontend_data ());

    if (_focused_ic && _focused_ic == ic) {
        ecore_imf_context_event_callback_call (_focused_ic->ctx, ECORE_IMF_CALLBACK_PRIVATE_COMMAND_SEND, (void *)command.c_str ());
    }
}

static void
reload_config_callback (const ConfigPointer &config)
{
    SCIM_DEBUG_FRONTEND(1) << __FUNCTION__ << "...\n";

    _frontend_hotkey_matcher.load_hotkeys (config);
    _imengine_hotkey_matcher.load_hotkeys (config);

    KeyEvent key;
    scim_string_to_key (key,
                        config->read (String (SCIM_CONFIG_HOTKEYS_FRONTEND_VALID_KEY_MASK),
                                      String ("Shift+Control+Alt+Lock")));

    _valid_key_mask = (key.mask > 0) ? (key.mask) : 0xFFFF;
    _valid_key_mask |= SCIM_KEY_ReleaseMask;
    // Special treatment for two backslash keys on jp106 keyboard.
    _valid_key_mask |= SCIM_KEY_QuirkKanaRoMask;

    _on_the_spot = config->read (String (SCIM_CONFIG_FRONTEND_ON_THE_SPOT), _on_the_spot);
    _shared_input_method = config->read (String (SCIM_CONFIG_FRONTEND_SHARED_INPUT_METHOD), _shared_input_method);
    _change_keyboard_mode_by_touch = scim_global_config_read (String (SCIM_GLOBAL_CONFIG_CHANGE_KEYBOARD_MODE_BY_TOUCH), _change_keyboard_mode_by_touch);
    _change_keyboard_mode_by_focus_move = scim_global_config_read (String (SCIM_GLOBAL_CONFIG_CHANGE_KEYBOARD_MODE_BY_FOCUS_MOVE), _change_keyboard_mode_by_focus_move);
    _hide_ise_based_on_focus = scim_global_config_read (String (SCIM_GLOBAL_CONFIG_HIDE_ISE_BASED_ON_FOCUS), _hide_ise_based_on_focus);
    _support_hw_keyboard_mode = scim_global_config_read (String (SCIM_GLOBAL_CONFIG_SUPPORT_HW_KEYBOARD_MODE), _support_hw_keyboard_mode);

    // Get keyboard layout setting
    // Flush the global config first, in order to load the new configs from disk.
    scim_global_config_flush ();

    _keyboard_layout = scim_get_default_keyboard_layout ();
}

static void
fallback_commit_string_cb (IMEngineInstanceBase  *si,
                           const WideString      &str)
{
    SCIM_DEBUG_FRONTEND(1) << __FUNCTION__ << "...\n";

    if (_focused_ic && _focused_ic->impl) {
        ecore_imf_context_commit_event_add (_focused_ic->ctx, utf8_wcstombs (str).c_str ());
        ecore_imf_context_event_callback_call (_focused_ic->ctx, ECORE_IMF_CALLBACK_COMMIT, const_cast<char*>(utf8_wcstombs (str).c_str ()));
    }
}

/*
vi:ts=4:expandtab:nowrap
*/
