#ifdef HAVE_CONFIG_H
# include "elementary_config.h"
#endif

#ifdef HAVE_EVIL
# include <Evil.h>
#endif

#include <Elementary.h>
#include "elm_priv.h"

#define _TIZEN_CHANGEABLE

EAPI int ELM_EVENT_CONFIG_ALL_CHANGED = 0;

Elm_Config *_elm_config = NULL;
char *_elm_profile = NULL;
static Eet_Data_Descriptor *_config_edd = NULL;
static Eet_Data_Descriptor *_config_font_overlay_edd = NULL;
static Eet_Data_Descriptor *_config_color_edd = NULL;
static Eet_Data_Descriptor *_config_color_palette_edd = NULL;
static Eet_Data_Descriptor *_config_color_overlay_edd = NULL;
const char *_elm_preferred_engine = NULL;
const char *_elm_preference = NULL;
const char *_elm_accel_preference = NULL;
Eina_List  *_color_overlays_del = NULL;

static Ecore_Poller *_elm_cache_flush_poller = NULL;

const char *_elm_engines[] = {
   "software_x11",
   "fb",
   "directfb",
   "software_16_x11",
   "software_8_x11",
   "xrender_x11",
   "opengl_x11",
   "software_gdi",
   "software_16_wince_gdi",
   "sdl",
   "software_16_sdl",
   "opengl_sdl",
   "buffer",
   "ews",
   "opengl_cocoa",
   "psl1ght",
   "wayland_shm",
   "wayland_egl",
   NULL
};

/* whenever you want to add a new text class support into Elementary,
   declare it both here and in the (default) theme */
static const Elm_Text_Class _elm_text_classes[] = {
   {"button", "Button"},
   {"label", "Label"},
   {"entry", "Entry"},
   {"title_bar", "Title Bar"},
   {"list_item", "List Items"},
   {"grid_item", "Grid Items"},
   {"toolbar_item", "Toolbar Items"},
   {"menu_item", "Menu Items"},
   {NULL, NULL}
};

/* whenever you want to add a new class class support into Elementary,
   declare it both here and in the (default) theme */
static const Elm_Color_Class _elm_color_classes[] = {
   {NULL, NULL}
};

static void        _desc_init(void);
static void        _desc_shutdown(void);
static void        _profile_fetch_from_conf(void);
static void        _config_free(void);
static void        _config_apply(void);
static void        _config_sub_apply(void);
static Elm_Config *_config_user_load(void);
static Elm_Config *_config_system_load(void);
static void        _config_load(void);
static void        _config_update(void);
static void        _env_get(void);
static size_t      _elm_data_dir_snprintf(char       *dst,
                                          size_t      size,
                                          const char *fmt, ...)
                                          EINA_PRINTF(3, 4);
static size_t _elm_user_dir_snprintf(char       *dst,
                                     size_t      size,
                                     const char *fmt, ...)
                                     EINA_PRINTF(3, 4);

#define ELM_CONFIG_VAL(edd, type, member, dtype) \
  EET_DATA_DESCRIPTOR_ADD_BASIC(edd, type, #member, member, dtype)
#define ELM_CONFIG_LIST(edd, type, member, eddtype) \
  EET_DATA_DESCRIPTOR_ADD_LIST(edd, type, #member, member, eddtype)

#ifdef HAVE_ELEMENTARY_X
static Ecore_Event_Handler *_prop_change_handler = NULL;
static Ecore_Timer *_prop_change_delay_timer = NULL;
static Ecore_X_Window _config_win = 0;
#define ATOM_COUNT 3
static Ecore_X_Atom _atom[ATOM_COUNT];
static Ecore_X_Atom _atom_config = 0;
static const char *_atom_names[ATOM_COUNT] =
{
   "ELM_PROFILE",
   "ELM_CONFIG",
   "ELM_CONFIG_WIN"
};
#define ATOM_E_PROFILE    0
#define ATOM_E_CONFIG     1
#define ATOM_E_CONFIG_WIN 2

static Eina_Bool _prop_config_get(void);
static void      _prop_config_set(void);
static Eina_Bool _prop_change(void *data  __UNUSED__,
                              int ev_type __UNUSED__,
                              void       *ev);
EAPI void
elm_config_font_set(const char * font_name)
{
    _elm_config->font_name = eina_stringshare_add(font_name);
    evas_font_reinit();
}

static Eina_Bool
_prop_config_get(void)
{
   int size = 0;
   Ecore_X_Atom atom;
   char buf[512];
   unsigned char *data = NULL;
   Elm_Config *config_data;
   Eina_Bool pre_access_mode;

   snprintf(buf, sizeof(buf), "ELM_CONFIG_%s", _elm_profile);
   atom = ecore_x_atom_get(buf);
   _atom_config = atom;
   if (!ecore_x_window_prop_property_get(_config_win,
                                         atom, _atom[ATOM_E_CONFIG],
                                         8, &data, &size))
     {
        if (!ecore_x_window_prop_property_get(_config_win,
                                              _atom[ATOM_E_CONFIG],
                                              _atom[ATOM_E_CONFIG],
                                              8, &data, &size))
          {
             if(data) free(data);
             ERR("ecore_x_window_prop_property_get is failed!");
             DBG("OUT::prop_config_get returns EINA_FALSE");
             return EINA_FALSE;
          }
        else
          _atom_config = _atom[ATOM_E_CONFIG];
     }
   else
     _atom_config = atom;
   if (size < 1)
     {
        free(data);
        DBG("OUT::prop_config_get returns EINA_FALSE");
        return EINA_FALSE;
     }
   config_data = eet_data_descriptor_decode(_config_edd, data, size);
   free(data);
   if (!config_data)
     {
        ERR("eet_data_descriptor_decode is failed!");
        DBG("OUT::prop_config_get returns EINA_FALSE");
        return EINA_FALSE;
     }

   /* What do we do on version mismatch when someone changes the
    * config in the rootwindow? */
   /* Most obvious case, new version and we are still linked to
    * whatever was there before, we just ignore until user restarts us */
   if (config_data->config_version > ELM_CONFIG_VERSION)
     {
        DBG("config_version mismatch!");
        DBG("OUT::prop_config_get returns EINA_TRUE");
        return EINA_TRUE;
     }

   if(config_data->font_name && _elm_config->font_name)
     {
         if(strcmp(config_data->font_name, _elm_config->font_name))
            evas_font_reinit();
     }

   /* What in the case the version is older? Do we even support those
    * cases or we only check for equality above? */
   pre_access_mode = _elm_config->access_mode;
   _config_free();
   _elm_config = config_data;
   _env_get();
   _config_apply();
   _config_sub_apply();

   _elm_config_font_overlay_apply();
#ifdef _TIZEN_CHANGEABLE
   Eina_List *eelist, *last;
   Ecore_Evas *ee;
   void *cu = NULL;

   eelist = ecore_evas_ecore_evas_list_get();
   if (eelist)
     {
        last = eina_list_last(eelist);
        ee = eina_list_data_get(last);
        cu = ecore_evas_data_get(ee, "changeable_ui");
     }
   if (!cu) _elm_config_color_overlay_apply();
   eina_list_free(eelist);
#else
   _elm_config_color_overlay_apply();
#endif
   _elm_rescale();
   _elm_recache();
   if (pre_access_mode != _elm_config->access_mode)
     _elm_win_access(_elm_config->access_mode);
   if (!_elm_config->access_mode) _elm_access_shutdown();
   ecore_event_add(ELM_EVENT_CONFIG_ALL_CHANGED, NULL, NULL, NULL);
   DBG("DONE::prop_config_get returns EINA_TRUE");
   return EINA_TRUE;
}

static void
_prop_config_set(void)
{
   unsigned char *config_data = NULL;
   int size = 0;

   config_data = eet_data_descriptor_encode(_config_edd, _elm_config, &size);
   if (config_data)
     {
        Ecore_X_Atom atom;
        char buf[512];

        snprintf(buf, sizeof(buf), "ELM_CONFIG_%s", _elm_profile);
        atom = ecore_x_atom_get(buf);
        _atom_config = atom;

        ecore_x_window_prop_property_set(_config_win, _atom_config,
                                         _atom[ATOM_E_CONFIG], 8,
                                         config_data, size);
        free(config_data);
     }
}

static Eina_Bool
_prop_change_delay_cb(void *data __UNUSED__)
{
   char *s;

   if (!getenv("ELM_PROFILE"))
     {
        s = ecore_x_window_prop_string_get(_config_win, _atom[ATOM_E_PROFILE]);
        if (s)
          {
             if (_elm_profile) free(_elm_profile);
             _elm_profile = s;
          }
     }
   _prop_config_get();
   _prop_change_delay_timer = NULL;
   return EINA_FALSE;
}

static Eina_Bool
_prop_change(void *data  __UNUSED__,
             int ev_type __UNUSED__,
             void       *ev)
{
   Ecore_X_Event_Window_Property *event = ev;

   if (event->win == _config_win)
     {
        if (event->atom == _atom[ATOM_E_PROFILE])
          {
             if (_prop_change_delay_timer) ecore_timer_del(_prop_change_delay_timer);
             _prop_change_delay_timer = ecore_timer_add(0.1, _prop_change_delay_cb, NULL);
          }
        else if (((_atom_config > 0) && (event->atom == _atom_config)) ||
                 (event->atom == _atom[ATOM_E_CONFIG]))
          {
             if (_prop_change_delay_timer) ecore_timer_del(_prop_change_delay_timer);
             _prop_change_delay_timer = ecore_timer_add(0.1, _prop_change_delay_cb, NULL);
          }
     }
   return ECORE_CALLBACK_PASS_ON;
}

#endif

static void
_desc_init(void)
{
   Eet_Data_Descriptor_Class eddc;

   EET_EINA_FILE_DATA_DESCRIPTOR_CLASS_SET(&eddc, Elm_Config);
   eddc.func.str_direct_alloc = NULL;
   eddc.func.str_direct_free = NULL;

   _config_edd = eet_data_descriptor_file_new(&eddc);
   if (!_config_edd)
     {
        printf("EEEK! eet_data_descriptor_file_new() failed\n");
        return;
     }

   memset(&eddc, 0, sizeof(eddc)); /* just in case... */
   EET_EINA_STREAM_DATA_DESCRIPTOR_CLASS_SET(&eddc, Elm_Font_Overlay);
   eddc.func.str_direct_alloc = NULL;
   eddc.func.str_direct_free = NULL;

   _config_font_overlay_edd = eet_data_descriptor_stream_new(&eddc);
   if (!_config_font_overlay_edd)
     {
        printf("EEEK! eet_data_descriptor_stream_new() failed\n");
        eet_data_descriptor_free(_config_edd);
        return;
     }

   memset(&eddc, 0, sizeof(eddc)); /* just in case... */
   EET_EINA_STREAM_DATA_DESCRIPTOR_CLASS_SET(&eddc, Elm_Color_RGBA);
   eddc.func.str_direct_alloc = NULL;
   eddc.func.str_direct_free = NULL;

   _config_color_edd = eet_data_descriptor_stream_new(&eddc);
   if (!_config_color_edd)
     {
        printf("EEEK! eet_data_descriptor_stream_new() failed\n");
        eet_data_descriptor_free(_config_edd);
        return;
     }

   memset(&eddc, 0, sizeof(eddc)); /* just in case... */
   EET_EINA_STREAM_DATA_DESCRIPTOR_CLASS_SET(&eddc, Elm_Custom_Palette);
   eddc.func.str_direct_alloc = NULL;
   eddc.func.str_direct_free = NULL;

   _config_color_palette_edd = eet_data_descriptor_stream_new(&eddc);
   if (!_config_color_palette_edd)
     {
        printf("EEEK! eet_data_descriptor_stream_new() failed\n");
        eet_data_descriptor_free(_config_edd);
        return;
     }

   memset(&eddc, 0, sizeof(eddc)); /* just in case... */
   EET_EINA_STREAM_DATA_DESCRIPTOR_CLASS_SET(&eddc, Elm_Color_Overlay);
   eddc.func.str_direct_alloc = NULL;
   eddc.func.str_direct_free = NULL;

   _config_color_overlay_edd = eet_data_descriptor_stream_new(&eddc);
   if (!_config_color_overlay_edd)
     {
        printf("EEEK! eet_data_descriptor_stream_new() failed\n");
        eet_data_descriptor_free(_config_edd);
        return;
     }

#define T_INT    EET_T_INT
#define T_DOUBLE EET_T_DOUBLE
#define T_STRING EET_T_STRING
#define T_UCHAR  EET_T_UCHAR

#define T        Elm_Font_Overlay
#define D        _config_font_overlay_edd
   ELM_CONFIG_VAL(D, T, text_class, EET_T_STRING);
   ELM_CONFIG_VAL(D, T, font, EET_T_STRING);
   ELM_CONFIG_VAL(D, T, size, EET_T_INT);
#undef T
#undef D

#define T Elm_Color_RGBA
#define D _config_color_edd
   ELM_CONFIG_VAL(D, T, r, EET_T_UINT);
   ELM_CONFIG_VAL(D, T, g, EET_T_UINT);
   ELM_CONFIG_VAL(D, T, b, EET_T_UINT);
   ELM_CONFIG_VAL(D, T, a, EET_T_UINT);
   ELM_CONFIG_VAL(D, T, color_name, EET_T_STRING);
#undef T
#undef D

#define T Elm_Custom_Palette
#define D _config_color_palette_edd
   ELM_CONFIG_VAL(D, T, palette_name, EET_T_STRING);
   ELM_CONFIG_LIST(D, T, color_list, _config_color_edd);
#undef T
#undef D

#define T Elm_Color_Overlay
#define D _config_color_overlay_edd
   ELM_CONFIG_VAL(D, T, color_class, EET_T_STRING);
   ELM_CONFIG_VAL(D, T, color.r, EET_T_UCHAR);
   ELM_CONFIG_VAL(D, T, color.g, EET_T_UCHAR);
   ELM_CONFIG_VAL(D, T, color.b, EET_T_UCHAR);
   ELM_CONFIG_VAL(D, T, color.a, EET_T_UCHAR);
   ELM_CONFIG_VAL(D, T, outline.r, EET_T_UCHAR);
   ELM_CONFIG_VAL(D, T, outline.g, EET_T_UCHAR);
   ELM_CONFIG_VAL(D, T, outline.b, EET_T_UCHAR);
   ELM_CONFIG_VAL(D, T, outline.a, EET_T_UCHAR);
   ELM_CONFIG_VAL(D, T, shadow.r, EET_T_UCHAR);
   ELM_CONFIG_VAL(D, T, shadow.g, EET_T_UCHAR);
   ELM_CONFIG_VAL(D, T, shadow.b, EET_T_UCHAR);
   ELM_CONFIG_VAL(D, T, shadow.a, EET_T_UCHAR);
#undef T
#undef D

#define T Elm_Config
#define D _config_edd
   ELM_CONFIG_VAL(D, T, config_version, T_INT);
   ELM_CONFIG_VAL(D, T, engine, T_STRING);

   //--------------------------------------------------------------
   // FIXME: For now we don't save accel & accel_override & gl depth/stencil/msaa
   // Reason is the current Settings app DOES NOT SUPPORT THEM!
   // ELM_CONFIG_VAL(D, T, accel, T_STRING);
   // ELM_CONFIG_VAL(D, T, accel_override, T_UCHAR);
   // ELM_CONFIG_VAL(D, T, gl_depth, T_INT);
   // ELM_CONFIG_VAL(D, T, gl_stencil, T_INT);
   // ELM_CONFIG_VAL(D, T, gl_msaa, T_INT);
   // -- jpeg 2014/08/27 -- wonsik 2015/01/12
   //--------------------------------------------------------------

   ELM_CONFIG_VAL(D, T, vsync, T_UCHAR);
   ELM_CONFIG_VAL(D, T, thumbscroll_enable, T_UCHAR);
   ELM_CONFIG_VAL(D, T, thumbscroll_threshold, T_INT);
   ELM_CONFIG_VAL(D, T, thumbscroll_hold_threshold, T_INT);
   ELM_CONFIG_VAL(D, T, thumbscroll_momentum_threshold, T_DOUBLE);
   ELM_CONFIG_VAL(D, T, thumbscroll_flick_distance_tolerance, T_INT);
   ELM_CONFIG_VAL(D, T, thumbscroll_friction, T_DOUBLE);
   ELM_CONFIG_VAL(D, T, thumbscroll_min_friction, T_DOUBLE);
   ELM_CONFIG_VAL(D, T, thumbscroll_friction_standard, T_DOUBLE);
   ELM_CONFIG_VAL(D, T, thumbscroll_bounce_friction, T_DOUBLE);
   ELM_CONFIG_VAL(D, T, thumbscroll_border_friction, T_DOUBLE);
   ELM_CONFIG_VAL(D, T, thumbscroll_sensitivity_friction, T_DOUBLE);
   ELM_CONFIG_VAL(D, T, thumbscroll_duration, T_DOUBLE);
   ELM_CONFIG_VAL(D, T, thumbscroll_cubic_bezier_p1_x, T_DOUBLE);
   ELM_CONFIG_VAL(D, T, thumbscroll_cubic_bezier_p1_y, T_DOUBLE);
   ELM_CONFIG_VAL(D, T, thumbscroll_cubic_bezier_p2_x, T_DOUBLE);
   ELM_CONFIG_VAL(D, T, thumbscroll_cubic_bezier_p2_y, T_DOUBLE);
   ELM_CONFIG_VAL(D, T, thumbscroll_acceleration_threshold, T_DOUBLE);
   ELM_CONFIG_VAL(D, T, thumbscroll_acceleration_time_limit, T_DOUBLE);
   ELM_CONFIG_VAL(D, T, thumbscroll_acceleration_weight, T_DOUBLE);
   ELM_CONFIG_VAL(D, T, page_scroll_friction, T_DOUBLE);
   ELM_CONFIG_VAL(D, T, bring_in_scroll_friction, T_DOUBLE);
   ELM_CONFIG_VAL(D, T, zoom_friction, T_DOUBLE);
   ELM_CONFIG_VAL(D, T, thumbscroll_bounce_enable, T_UCHAR);
   ELM_CONFIG_VAL(D, T, scroll_smooth_start_enable, T_UCHAR);
   ELM_CONFIG_VAL(D, T, scroll_smooth_time_interval, T_DOUBLE);
   ELM_CONFIG_VAL(D, T, scroll_smooth_amount, T_DOUBLE);
   ELM_CONFIG_VAL(D, T, scroll_smooth_history_weight, T_DOUBLE);
   ELM_CONFIG_VAL(D, T, scroll_smooth_future_time, T_DOUBLE);
   ELM_CONFIG_VAL(D, T, scroll_smooth_time_window, T_DOUBLE);
   ELM_CONFIG_VAL(D, T, scale, T_DOUBLE);
   ELM_CONFIG_VAL(D, T, bgpixmap, T_INT);
   ELM_CONFIG_VAL(D, T, compositing, T_INT);
   /* EET_DATA_DESCRIPTOR_ADD_LIST(D, T, "font_dirs", font_dirs, sub_edd); */
   ELM_CONFIG_LIST(D, T, font_overlays, _config_font_overlay_edd);
   ELM_CONFIG_VAL(D, T, font_hinting, T_INT);
   ELM_CONFIG_VAL(D, T, cache_flush_poll_interval, T_INT);
   ELM_CONFIG_VAL(D, T, cache_flush_enable, T_UCHAR);
   ELM_CONFIG_VAL(D, T, image_cache, T_INT);
   ELM_CONFIG_VAL(D, T, font_cache, T_INT);
   ELM_CONFIG_VAL(D, T, edje_cache, T_INT);
   ELM_CONFIG_VAL(D, T, edje_collection_cache, T_INT);
   ELM_CONFIG_VAL(D, T, finger_size, T_INT);
   ELM_CONFIG_VAL(D, T, fps, T_DOUBLE);
   ELM_CONFIG_VAL(D, T, theme, T_STRING);
   ELM_CONFIG_VAL(D, T, modules, T_STRING);
   ELM_CONFIG_VAL(D, T, tooltip_delay, T_DOUBLE);
   ELM_CONFIG_VAL(D, T, cursor_engine_only, T_UCHAR);
   ELM_CONFIG_VAL(D, T, focus_highlight_enable, T_UCHAR);
   ELM_CONFIG_VAL(D, T, focus_highlight_animate, T_UCHAR);
   ELM_CONFIG_VAL(D, T, toolbar_shrink_mode, T_INT);
   ELM_CONFIG_VAL(D, T, fileselector_expand_enable, T_UCHAR);
   ELM_CONFIG_VAL(D, T, inwin_dialogs_enable, T_UCHAR);
   ELM_CONFIG_VAL(D, T, icon_size, T_INT);
   ELM_CONFIG_VAL(D, T, longpress_timeout, T_DOUBLE);
   ELM_CONFIG_VAL(D, T, effect_enable, T_UCHAR);
   ELM_CONFIG_VAL(D, T, desktop_entry, T_UCHAR);
   ELM_CONFIG_VAL(D, T, password_show_last, T_UCHAR);
   ELM_CONFIG_VAL(D, T, password_show_last_timeout, T_DOUBLE);
   ELM_CONFIG_VAL(D, T, glayer_zoom_finger_enable, T_UCHAR);
   ELM_CONFIG_VAL(D, T, glayer_zoom_finger_factor, T_DOUBLE);
   ELM_CONFIG_VAL(D, T, glayer_zoom_wheel_factor, T_DOUBLE);
   ELM_CONFIG_VAL(D, T, glayer_zoom_distance_tolerance, T_DOUBLE);
   ELM_CONFIG_VAL(D, T, glayer_rotate_finger_enable, T_UCHAR);
   ELM_CONFIG_VAL(D, T, glayer_rotate_angular_tolerance, T_DOUBLE);
   ELM_CONFIG_VAL(D, T, glayer_line_min_length, T_DOUBLE);
   ELM_CONFIG_VAL(D, T, glayer_line_distance_tolerance, T_DOUBLE);
   ELM_CONFIG_VAL(D, T, glayer_line_angular_tolerance, T_DOUBLE);
   ELM_CONFIG_VAL(D, T, glayer_flick_time_limit_ms, T_INT);
   ELM_CONFIG_VAL(D, T, glayer_long_tap_start_timeout, T_DOUBLE);
   ELM_CONFIG_VAL(D, T, glayer_double_tap_timeout, T_DOUBLE);
   ELM_CONFIG_VAL(D, T, access_mode, T_UCHAR);
   ELM_CONFIG_VAL(D, T, access_password_read_enable, T_UCHAR);
   ELM_CONFIG_VAL(D, T, selection_clear_enable, T_UCHAR);
   ELM_CONFIG_VAL(D, T, glayer_continues_enable, T_UCHAR);
   ELM_CONFIG_VAL(D, T, week_start, T_INT);
   ELM_CONFIG_VAL(D, T, weekend_start, T_INT);
   ELM_CONFIG_VAL(D, T, weekend_len, T_INT);
   ELM_CONFIG_VAL(D, T, year_min, T_INT);
   ELM_CONFIG_VAL(D, T, year_max, T_INT);
   ELM_CONFIG_LIST(D, T, color_overlays, _config_color_overlay_edd);
   ELM_CONFIG_LIST(D, T, color_palette, _config_color_palette_edd);
   ELM_CONFIG_VAL(D, T, softcursor_mode, T_UCHAR);
   ELM_CONFIG_VAL(D, T, auto_norender_withdrawn, T_UCHAR);
   ELM_CONFIG_VAL(D, T, auto_norender_iconified_same_as_withdrawn, T_UCHAR);
   ELM_CONFIG_VAL(D, T, auto_flush_withdrawn, T_UCHAR);
   ELM_CONFIG_VAL(D, T, auto_dump_withdrawn, T_UCHAR);
   ELM_CONFIG_VAL(D, T, auto_throttle, T_UCHAR);
   ELM_CONFIG_VAL(D, T, auto_throttle_amount, T_DOUBLE);
   ELM_CONFIG_VAL(D, T, indicator_service_0, T_STRING);
   ELM_CONFIG_VAL(D, T, indicator_service_90, T_STRING);
   ELM_CONFIG_VAL(D, T, indicator_service_180, T_STRING);
   ELM_CONFIG_VAL(D, T, indicator_service_270, T_STRING);
   ELM_CONFIG_VAL(D, T, translate, T_UCHAR);
   ELM_CONFIG_VAL(D, T, genlist_animation_duration, T_DOUBLE);
   ELM_CONFIG_VAL(D, T, gengrid_reorder_duration, T_DOUBLE);
   ELM_CONFIG_VAL(D, T, gengrid_animation_duration, T_DOUBLE);
   ELM_CONFIG_VAL(D, T, scroll_item_align_enable, T_UCHAR);
   ELM_CONFIG_VAL(D, T, scroll_item_valign, T_STRING);
   ELM_CONFIG_VAL(D, T, font_name, T_STRING);
#undef T
#undef D
#undef T_INT
#undef T_DOUBLE
#undef T_STRING
#undef T_UCHAR
}

static void
_desc_shutdown(void)
{
   if (_config_edd)
     {
        eet_data_descriptor_free(_config_edd);
        _config_edd = NULL;
     }

   if (_config_font_overlay_edd)
     {
        eet_data_descriptor_free(_config_font_overlay_edd);
        _config_font_overlay_edd = NULL;
     }

   if (_config_color_edd)
     {
        eet_data_descriptor_free(_config_color_edd);
        _config_color_edd = NULL;
     }

   if (_config_color_palette_edd)
     {
        eet_data_descriptor_free(_config_color_palette_edd);
        _config_color_palette_edd = NULL;
     }

   if (_config_color_overlay_edd)
     {
        eet_data_descriptor_free(_config_color_overlay_edd);
        _config_color_overlay_edd = NULL;
     }
}

static int
_sort_files_cb(const void *f1,
               const void *f2)
{
   return strcmp(f1, f2);
}

const char *
_elm_config_current_profile_get(void)
{
   return _elm_profile;
}

static size_t
_elm_data_dir_snprintf(char       *dst,
                       size_t      size,
                       const char *fmt,
                       ...)
{
   size_t data_dir_len, off;
   va_list ap;

   data_dir_len = eina_strlcpy(dst, _elm_data_dir, size);

   off = data_dir_len + 1;
   if (off >= size)
     goto end;

   va_start(ap, fmt);
   dst[data_dir_len] = '/';

   off = off + vsnprintf(dst + off, size - off, fmt, ap);
   va_end(ap);

end:
   return off;
}

static size_t
_elm_user_dir_snprintf(char       *dst,
                       size_t      size,
                       const char *fmt,
                       ...)
{
   const char *home;
   size_t user_dir_len, off;
   va_list ap;

#ifdef _WIN32
   home = evil_homedir_get();
#else
   home = getenv("HOME");
#endif
   if (!home)
     home = "/";

   user_dir_len = eina_str_join_len(dst, size, '/', home, strlen(home),
                                    ELEMENTARY_BASE_DIR, sizeof(ELEMENTARY_BASE_DIR) - 1);

   off = user_dir_len + 1;
   if (off >= size)
     goto end;

   va_start(ap, fmt);
   dst[user_dir_len] = '/';

   off = off + vsnprintf(dst + off, size - off, fmt, ap);
   va_end(ap);

end:
   return off;
}

const char *
_elm_config_profile_dir_get(const char *prof,
                            Eina_Bool   is_user)
{
   char buf[PATH_MAX] = {0, };

   if (!is_user)
     goto not_user;

   _elm_user_dir_snprintf(buf, sizeof(buf), "config/%s", prof);

   if (ecore_file_is_dir(buf))
     return strdup(buf);

   return NULL;

not_user:
   snprintf(buf, sizeof(buf), "%s/config/%s", _elm_data_dir, prof);

   if (ecore_file_is_dir(buf))
     return strdup(buf);

   return NULL;
}

Eina_List *
_elm_config_font_overlays_list(void)
{
   return _elm_config->font_overlays;
}

Eina_Bool _elm_config_access_get(void)
{
   return _elm_config->access_mode;
}

void _elm_config_access_set(Eina_Bool is_access)
{
   is_access = !!is_access;
   if (_elm_config->access_mode == is_access) return;
   _elm_config->access_mode = is_access;
   _elm_win_access(is_access);

   if (!is_access) _elm_access_shutdown();
}

Eina_Bool _elm_config_access_password_read_enabled_get(void)
{
   return _elm_config->access_password_read_enable;
}

void _elm_config_access_password_read_enabled_set(Eina_Bool enabled)
{
   enabled = !!enabled;
   if (_elm_config->access_password_read_enable == enabled) return;
   _elm_config->access_password_read_enable = enabled;
}

Eina_Bool _elm_config_selection_unfocused_clear_get(void)
{
   return _elm_config->selection_clear_enable;
}

void _elm_config_selection_unfocused_clear_set(Eina_Bool enabled)
{
   enabled = !!enabled;
   if (_elm_config->selection_clear_enable == enabled) return;
   _elm_config->selection_clear_enable = enabled;
}

void
_elm_config_font_overlay_set(const char    *text_class,
                             const char    *font,
                             Evas_Font_Size size)
{
   Elm_Font_Overlay *efd;
   Eina_List *l;

   EINA_LIST_FOREACH(_elm_config->font_overlays, l, efd)
     {
        if (strcmp(efd->text_class, text_class))
          continue;

        if (efd->font) eina_stringshare_del(efd->font);
        efd->font = eina_stringshare_add(font);
        efd->size = size;
        _elm_config->font_overlays =
          eina_list_promote_list(_elm_config->font_overlays, l);
        return;
     }

   /* the text class doesn't exist */
   efd = calloc(1, sizeof(Elm_Font_Overlay));
   efd->text_class = eina_stringshare_add(text_class);
   efd->font = eina_stringshare_add(font);
   efd->size = size;

   _elm_config->font_overlays = eina_list_prepend(_elm_config->font_overlays,
                                                  efd);
}

void
_elm_config_font_overlay_remove(const char *text_class)
{
   Elm_Font_Overlay *efd;
   Eina_List *l;

   EINA_LIST_FOREACH(_elm_config->font_overlays, l, efd)
     {
        if (efd->text_class && strcmp(efd->text_class, text_class))
          continue;

        _elm_config->font_overlays =
          eina_list_remove_list(_elm_config->font_overlays, l);
        if (efd->text_class) eina_stringshare_del(efd->text_class);
        if (efd->font) eina_stringshare_del(efd->font);
        free(efd);

        return;
     }
}

void
_elm_config_font_overlay_apply(void)
{
   Elm_Font_Overlay *efd;
   Eina_List *l;
   int i;

   for (i = 0; _elm_text_classes[i].desc; i++)
     edje_text_class_del(_elm_text_classes[i].name);

   EINA_LIST_FOREACH(_elm_config->font_overlays, l, efd)
     edje_text_class_set(efd->text_class, efd->font, efd->size);
}

Eina_List *
_elm_config_text_classes_get(void)
{
   Eina_List *ret = NULL;
   int i;

   for (i = 0; _elm_text_classes[i].desc; i++)
     {
        Elm_Text_Class *tc;
        tc = malloc(sizeof(*tc));
        if (!tc) continue;

        *tc = _elm_text_classes[i];

        ret = eina_list_append(ret, tc);
     }

   return ret;
}

void
_elm_config_text_classes_free(Eina_List *l)
{
   Elm_Text_Class *tc;

   EINA_LIST_FREE(l, tc)
     free(tc);
}

Eina_List *
_elm_config_color_classes_get(void)
{
   Eina_List *ret = NULL;
   int i;

   for (i = 0; _elm_color_classes[i].desc; i++)
     {
        Elm_Color_Class *cc;
        cc = malloc(sizeof(*cc));
        if (!cc) continue;

        *cc = _elm_color_classes[i];

        ret = eina_list_append(ret, cc);
     }

   return ret;
}

void
_elm_config_color_classes_free(Eina_List *l)
{
   Elm_Color_Class *cc;

   EINA_LIST_FREE(l, cc)
     free(cc);
}

Eina_List *
_elm_config_color_overlays_list(void)
{
   return _elm_config->color_overlays;
}

void
_elm_config_color_overlay_set(const char *color_class,
                              int r, int g, int b, int a,
                              int r2, int g2, int b2, int a2,
                              int r3, int g3, int b3, int a3)
{
   Elm_Color_Overlay *ecd;
   Eina_List *l;

#define CHECK_COLOR_VAL(v) v = (v > 255)? 255 : (v < 0)? 0: v
   CHECK_COLOR_VAL(r);
   CHECK_COLOR_VAL(g);
   CHECK_COLOR_VAL(b);
   CHECK_COLOR_VAL(a);
   CHECK_COLOR_VAL(r2);
   CHECK_COLOR_VAL(g2);
   CHECK_COLOR_VAL(b2);
   CHECK_COLOR_VAL(a2);
   CHECK_COLOR_VAL(r3);
   CHECK_COLOR_VAL(g3);
   CHECK_COLOR_VAL(b3);
   CHECK_COLOR_VAL(a3);
#undef CHECK_COLOR_VAL

   EINA_LIST_FOREACH(_elm_config->color_overlays, l, ecd)
     {
        if (strcmp(ecd->color_class, color_class))
          continue;

        ecd->color.r = r;
        ecd->color.g = g;
        ecd->color.b = b;
        ecd->color.a = a;
        ecd->outline.r = r2;
        ecd->outline.g = g2;
        ecd->outline.b = b2;
        ecd->outline.a = a2;
        ecd->shadow.r = r3;
        ecd->shadow.g = g3;
        ecd->shadow.b = b3;
        ecd->shadow.a = a3;

        _elm_config->color_overlays =
           eina_list_promote_list(_elm_config->color_overlays, l);

        return;
     }

   /* the color class doesn't exist */
   ecd = calloc(1, sizeof(Elm_Color_Overlay));
   if (!ecd) return;

   ecd->color_class = eina_stringshare_add(color_class);
   ecd->color.r = r;
   ecd->color.g = g;
   ecd->color.b = b;
   ecd->color.a = a;
   ecd->outline.r = r2;
   ecd->outline.g = g2;
   ecd->outline.b = b2;
   ecd->outline.a = a2;
   ecd->shadow.r = r3;
   ecd->shadow.g = g3;
   ecd->shadow.b = b3;
   ecd->shadow.a = a3;

   _elm_config->color_overlays =
      eina_list_prepend(_elm_config->color_overlays, ecd);
}

void
_elm_config_color_overlay_remove(const char *color_class)
{
   Elm_Color_Overlay *ecd;
   Eina_List *l;

   EINA_LIST_FOREACH(_elm_config->color_overlays, l, ecd)
     {
        if (!ecd->color_class) continue;
        if (strcmp(ecd->color_class, color_class)) continue;

        _color_overlays_del =
           eina_list_append(_color_overlays_del,
                            eina_stringshare_add(color_class));
        _elm_config->color_overlays =
          eina_list_remove_list(_elm_config->color_overlays, l);
        eina_stringshare_del(ecd->color_class);
        free(ecd);

        return;
     }
}

void
_elm_config_color_overlay_apply(void)
{
   Elm_Color_Overlay *ecd;
   Eina_List *l;
   char *color_class;

   EINA_LIST_FREE(_color_overlays_del, color_class)
     {
        edje_color_class_del(color_class);
        eina_stringshare_del(color_class);
     }

   EINA_LIST_FOREACH(_elm_config->color_overlays, l, ecd)
     edje_color_class_set(ecd->color_class,
                ecd->color.r, ecd->color.g, ecd->color.b, ecd->color.a,
                ecd->outline.r, ecd->outline.g, ecd->outline.b, ecd->outline.a,
                ecd->shadow.r, ecd->shadow.g, ecd->shadow.b, ecd->shadow.a);
}

Eina_List *
_elm_config_color_list_get(const char *palette_name)
{
    Eina_List *plist;
    Elm_Custom_Palette *cpalette;
    EINA_LIST_FOREACH(_elm_config->color_palette, plist, cpalette)
      {
         if (strcmp(cpalette->palette_name, palette_name))
           continue;
         return cpalette->color_list;
      }
    return NULL;
}

void
_elm_config_color_set(const char *palette_name,
                      int r,
                      int g,
                      int b,
                      int a)
{
   Eina_List *plist;
   Elm_Custom_Palette *cpalette;
   Elm_Color_RGBA *color;
   EINA_LIST_FOREACH(_elm_config->color_palette, plist, cpalette)
     {
        if (strcmp(cpalette->palette_name, palette_name))
          continue;

        color = calloc(1, sizeof(Elm_Color_RGBA));
        eina_stringshare_replace(&color->color_name, "");
        color->r = r;
        color->g = g;
        color->b = b;
        color->a = a;
        cpalette->color_list = eina_list_prepend(cpalette->color_list,
                                                       color);
     }
}

void
_elm_config_colors_free(const char *palette_name)
{
   Eina_List *plist;
   Elm_Custom_Palette *cpalette;
   Elm_Color_RGBA *color;
   EINA_LIST_FOREACH(_elm_config->color_palette, plist, cpalette)
     {
        if (strcmp(cpalette->palette_name, palette_name))
          continue;

        EINA_LIST_FREE(cpalette->color_list, color)
          {
             free(color);
          }
     }
}

Eina_List *
_elm_config_profiles_list(void)
{
   Eina_File_Direct_Info *info;
   Eina_List *flist = NULL;
   Eina_Iterator *file_it;
   char buf[PATH_MAX];
   const char *dir;
   size_t len;

   len = _elm_user_dir_snprintf(buf, sizeof(buf), "config");

   file_it = eina_file_stat_ls(buf);
   if (!file_it)
     goto sys;

   buf[len] = '/';
   len++;

   len = sizeof(buf) - len;

   EINA_ITERATOR_FOREACH(file_it, info)
     {
        if (info->name_length >= len)
          continue;

        if (info->type == EINA_FILE_DIR)
          {
             flist =
               eina_list_sorted_insert(flist, _sort_files_cb,
                                       eina_stringshare_add(info->path +
                                                            info->name_start));
          }
     }

   eina_iterator_free(file_it);

sys:
   len = eina_str_join_len(buf, sizeof(buf), '/', _elm_data_dir,
                           strlen(_elm_data_dir), "config",
                           sizeof("config") - 1);

   file_it = eina_file_direct_ls(buf);
   if (!file_it)
     goto list_free;

   buf[len] = '/';
   len++;

   len = sizeof(buf) - len;
   EINA_ITERATOR_FOREACH(file_it, info)
     {
        if (info->name_length >= len)
          continue;

        switch (info->type)
          {
           case EINA_FILE_DIR:
           {
              const Eina_List *l;
              const char *tmp;

              EINA_LIST_FOREACH(flist, l, tmp)
                if (!strcmp(info->path + info->name_start, tmp))
                  break;

              if (!l)
                flist =
                  eina_list_sorted_insert(flist, _sort_files_cb,
                                          eina_stringshare_add(info->path +
                                                               info->name_start));
           }
           break;

           default:
             continue;
          }
     }
   eina_iterator_free(file_it);
   return flist;

list_free:
   EINA_LIST_FREE(flist, dir)
     eina_stringshare_del(dir);

   return NULL;
}

static void
_profile_fetch_from_conf(void)
{
   char buf[PATH_MAX], *p, *s;
   Eet_File *ef = NULL;
   int len = 0;

   _elm_profile = strdup("default");

   // if env var - use profile without question
   s = getenv("ELM_PROFILE");
   if (s)
     {
        free(_elm_profile);
        _elm_profile = strdup(s);
        return;
     }

   // user profile
   _elm_user_dir_snprintf(buf, sizeof(buf), "config/profile.cfg");
   ef = eet_open(buf, EET_FILE_MODE_READ);
   if (ef)
     {
        p = eet_read(ef, "config", &len);
        if (p)
          {
             free(_elm_profile);
             _elm_profile = malloc(len + 1);
             memcpy(_elm_profile, p, len);
             _elm_profile[len] = 0;
             free(p);
          }
        eet_close(ef);
        if (!p) ef = NULL;
     }
   if (ef) return;

   // system profile
   _elm_data_dir_snprintf(buf, sizeof(buf), "config/profile.cfg");
   ef = eet_open(buf, EET_FILE_MODE_READ);
   if (ef)
     {
        p = eet_read(ef, "config", &len);
        if (p)
          {
             free(_elm_profile);
             _elm_profile = malloc(len + 1);
             memcpy(_elm_profile, p, len);
             _elm_profile[len] = 0;
             free(p);
          }
        eet_close(ef);
     }
}

static void
_config_free(void)
{
   Elm_Font_Overlay *fo;
   const char *fontdir;
   Elm_Color_Overlay *co;
   Elm_Custom_Palette *palette;
   Elm_Color_RGBA *color;
   char *color_class;

   if (!_elm_config) return;
   EINA_LIST_FREE(_elm_config->font_dirs, fontdir)
     {
        eina_stringshare_del(fontdir);
     }
   if (_elm_config->engine) eina_stringshare_del(_elm_config->engine);
   if (_elm_config->accel) eina_stringshare_del(_elm_config->accel);
   EINA_LIST_FREE(_elm_config->font_overlays, fo)
     {
        if (fo->text_class) eina_stringshare_del(fo->text_class);
        if (fo->font) eina_stringshare_del(fo->font);
        free(fo);
     }
   EINA_LIST_FREE(_color_overlays_del, color_class)
      eina_stringshare_del(color_class);
   EINA_LIST_FREE(_elm_config->color_overlays, co)
     {
        if (co->color_class) eina_stringshare_del(co->color_class);
        free(co);
     }
   EINA_LIST_FREE(_elm_config->color_palette, palette)
     {
        if (palette->palette_name) eina_stringshare_del(palette->palette_name);
        EINA_LIST_FREE(palette->color_list, color) free(color);
        free(palette);
     }
   if (_elm_config->theme) eina_stringshare_del(_elm_config->theme);
   if (_elm_config->modules) eina_stringshare_del(_elm_config->modules);
   if (_elm_config->font_name) eina_stringshare_del(_elm_config->font_name);
   free(_elm_config);
   _elm_config = NULL;
}

static void
_config_apply(void)
{
   _elm_theme_parse(NULL, _elm_config->theme);
   ecore_animator_frametime_set(1.0 / _elm_config->fps);
}

static void
_config_sub_apply(void)
{
   edje_frametime_set(1.0 / _elm_config->fps);
   edje_scale_set(_elm_config->scale);
   edje_password_show_last_set(_elm_config->password_show_last);
   edje_password_show_last_timeout_set(_elm_config->password_show_last_timeout);
}

static Eina_Bool
_elm_cache_flush_cb(void *data __UNUSED__)
{
   elm_cache_all_flush();
   return ECORE_CALLBACK_RENEW;
}

/* kind of abusing this call right now -- shared between all of those
 * properties -- but they are not meant to be called that periodically
 * anyway */
void
_elm_recache(void)
{
   Eina_List *l;
   Evas_Object *win;

   elm_cache_all_flush();

   EINA_LIST_FOREACH(_elm_win_list, l, win)
     {
        Evas *e = evas_object_evas_get(win);
        evas_image_cache_set(e, _elm_config->image_cache * 1024);
        evas_font_cache_set(e, _elm_config->font_cache * 1024);
     }
   edje_file_cache_set(_elm_config->edje_cache);
   edje_collection_cache_set(_elm_config->edje_collection_cache);

   if (_elm_cache_flush_poller)
     {
        ecore_poller_del(_elm_cache_flush_poller);
        _elm_cache_flush_poller = NULL;
     }
   if (_elm_config->cache_flush_enable)
     {
        if (_elm_config->cache_flush_poll_interval > 0)
          {
             _elm_cache_flush_poller =
                ecore_poller_add(ECORE_POLLER_CORE,
                                 _elm_config->cache_flush_poll_interval,
                                 _elm_cache_flush_cb, NULL);
          }
     }
}

static Elm_Config *
_config_user_load(void)
{
   Elm_Config *cfg = NULL;
   Eet_File *ef;
   char buf[PATH_MAX];

   _elm_user_dir_snprintf(buf, sizeof(buf), "config/%s/base.cfg",
                          _elm_profile);

   ef = eet_open(buf, EET_FILE_MODE_READ);
   if (ef)
     {
        cfg = eet_data_read(ef, _config_edd, "config");
        eet_close(ef);
     }
   return cfg;
}

static Elm_Config *
_config_system_load(void)
{
   Elm_Config *cfg = NULL;
   Eet_File *ef;
   char buf[PATH_MAX];

   _elm_data_dir_snprintf(buf, sizeof(buf), "config/%s/base.cfg",
                          _elm_profile);

   ef = eet_open(buf, EET_FILE_MODE_READ);
   if (ef)
     {
        cfg = eet_data_read(ef, _config_edd, "config");
        eet_close(ef);
     }
   return cfg;
}

static void
_config_load(void)
{
   _elm_config = _config_user_load();
   if (_elm_config)
     {
        if (_elm_config->config_version < ELM_CONFIG_VERSION)
          _config_update();
        return;
     }

   /* no user config, fallback for system. No need to check version for
    * this one, if it's not the right one, someone screwed up at the time
    * of installing it */
   _elm_config = _config_system_load();
   if (_elm_config) return;
   /* FIXME: config load could have failed because of a non-existent
    * profile. Fallback to default before moving on */

   // config load fail - defaults
   // why are these here? well if they are, it means you can make a gui
   // config recovery app i guess...
   _elm_config = ELM_NEW(Elm_Config);
   _elm_config->config_version = ELM_CONFIG_VERSION;
   _elm_config->engine = eina_stringshare_add("software_x11");
   _elm_config->accel = NULL;
   _elm_config->accel_override = 0;
   _elm_config->vsync = 0;
   _elm_config->thumbscroll_enable = EINA_TRUE;
   _elm_config->thumbscroll_threshold = 24;
   _elm_config->thumbscroll_hold_threshold = 24;
   _elm_config->thumbscroll_momentum_threshold = 100.0;
   _elm_config->thumbscroll_flick_distance_tolerance = 1000;
   _elm_config->thumbscroll_friction = 1.0;
   _elm_config->thumbscroll_min_friction = 0.5;
   _elm_config->thumbscroll_friction_standard = 1000.0;
   _elm_config->thumbscroll_bounce_friction = 0.5;
   _elm_config->thumbscroll_bounce_enable = EINA_TRUE;
   _elm_config->thumbscroll_acceleration_threshold = 2000.0;
   _elm_config->thumbscroll_acceleration_time_limit = 0.5;
   _elm_config->thumbscroll_acceleration_weight = 1.5;
   _elm_config->page_scroll_friction = 0.5;
   _elm_config->bring_in_scroll_friction = 0.5;
   _elm_config->zoom_friction = 0.5;
   _elm_config->thumbscroll_border_friction = 0.5;
   _elm_config->thumbscroll_sensitivity_friction = 0.25; // magic number! just trial and error shows this makes it behave "nicer" and not run off at high speed all the time
   _elm_config->thumbscroll_duration = 1.0;
   _elm_config->thumbscroll_cubic_bezier_p1_x = 0.0;
   _elm_config->thumbscroll_cubic_bezier_p1_y = 0.23;
   _elm_config->thumbscroll_cubic_bezier_p2_x = 0.18;
   _elm_config->thumbscroll_cubic_bezier_p2_y = 1.0;
   _elm_config->scroll_smooth_start_enable = EINA_FALSE;
   _elm_config->scroll_smooth_time_interval = 0.008;
   _elm_config->scroll_smooth_amount = 1.0;
   _elm_config->scroll_smooth_history_weight = 0.3;
   _elm_config->scroll_smooth_future_time = 0.0;
   _elm_config->scroll_smooth_time_window = 0.2;
   _elm_config->scale = 1.0;
   _elm_config->bgpixmap = 0;
   _elm_config->compositing = 1;
   _elm_config->font_hinting = 2;
   _elm_config->cache_flush_poll_interval = 512;
   _elm_config->cache_flush_enable = EINA_TRUE;
   _elm_config->font_dirs = NULL;
   _elm_config->image_cache = 4096;
   _elm_config->font_cache = 512;
   _elm_config->edje_cache = 32;
   _elm_config->edje_collection_cache = 64;
   _elm_config->finger_size = 40;
   _elm_config->fps = 60.0;
   _elm_config->theme = eina_stringshare_add("default");
   _elm_config->modules = NULL;
   _elm_config->tooltip_delay = 1.0;
   _elm_config->cursor_engine_only = EINA_TRUE;
   _elm_config->focus_highlight_enable = EINA_FALSE;
   _elm_config->focus_highlight_animate = EINA_TRUE;
   _elm_config->toolbar_shrink_mode = 2;
   _elm_config->fileselector_expand_enable = EINA_FALSE;
   _elm_config->inwin_dialogs_enable = EINA_FALSE;
   _elm_config->icon_size = 32;
   _elm_config->longpress_timeout = 1.0;
   _elm_config->effect_enable = EINA_TRUE;
   _elm_config->desktop_entry = EINA_FALSE;
   _elm_config->is_mirrored = EINA_FALSE; /* Read sys value in env_get() */
   _elm_config->password_show_last = EINA_FALSE;
   _elm_config->password_show_last_timeout = 2.0;
   _elm_config->glayer_zoom_finger_enable = EINA_TRUE;
   _elm_config->glayer_zoom_finger_factor = 1.0;
   _elm_config->glayer_zoom_wheel_factor = 0.05;
   _elm_config->glayer_zoom_distance_tolerance = 1.0; /* 1 times elm_finger_size_get() */
   _elm_config->glayer_rotate_finger_enable = EINA_TRUE;
   _elm_config->glayer_rotate_angular_tolerance = 2.0; /* 2 DEG */
   _elm_config->glayer_line_min_length = 1.0;         /* 1 times elm_finger_size_get() */
   _elm_config->glayer_line_distance_tolerance = 3.0; /* 3 times elm_finger_size_get() */
   _elm_config->glayer_line_angular_tolerance = 20.0; /* 20 DEG */
   _elm_config->glayer_flick_time_limit_ms = 120;              /* ms to finish flick */
   _elm_config->glayer_long_tap_start_timeout = 1.2;   /* 1.2 second to start long-tap */
   _elm_config->glayer_double_tap_timeout = 0.25;   /* 0.25 seconds between 2 mouse downs of a tap. */
   _elm_config->glayer_continues_enable = EINA_TRUE;      /* Continue gestures default */
   _elm_config->access_mode = ELM_ACCESS_MODE_OFF;
   _elm_config->access_password_read_enable = EINA_FALSE;
   _elm_config->selection_clear_enable = EINA_TRUE;
   _elm_config->week_start = 1; /* monday */
   _elm_config->weekend_start = 6; /* saturday */
   _elm_config->weekend_len = 2;
   _elm_config->year_min = 2;
   _elm_config->year_max = 137;
   _elm_config->softcursor_mode = 0; /* 0 = auto, 1 = on, 2 = off */
   _elm_config->color_palette = NULL;
   _elm_config->auto_norender_withdrawn = 0;
   _elm_config->auto_norender_iconified_same_as_withdrawn = 1;
   _elm_config->auto_flush_withdrawn = 0;
   _elm_config->auto_dump_withdrawn = 0;
   _elm_config->auto_throttle = 0;
   _elm_config->auto_throttle_amount = 0.1;
   _elm_config->indicator_service_0 = eina_stringshare_add("elm_indicator_portrait");
   _elm_config->indicator_service_90 = eina_stringshare_add("elm_indicator_landscape");
   _elm_config->indicator_service_180 = eina_stringshare_add("elm_indicator_portrait");
   _elm_config->indicator_service_270 = eina_stringshare_add("elm_indicator_landscape");
   _elm_config->genlist_animation_duration = 1000.0;
   _elm_config->gengrid_reorder_duration = 800.0;
   _elm_config->gengrid_animation_duration = 800.0;
   _elm_config->item_scroll_align = 0;
   _elm_config->scroll_item_align_enable = EINA_FALSE;
   _elm_config->scroll_item_valign = eina_stringshare_add("center");
   _elm_config->gl_depth = 0;
   _elm_config->gl_stencil = 0;
   _elm_config->gl_msaa = 0;
}

static const char *
_elm_config_eet_close_error_get(Eet_File *ef,
                                char     *file)
{
   Eet_Error err;
   const char *erstr = NULL;

   err = eet_close(ef);
   switch (err)
     {
      case EET_ERROR_WRITE_ERROR:
        erstr = "An error occurred while saving Elementary's "
                "settings to disk. The error could not be "
                "deterimined. The file where the error occurred was: "
                "%s. This file has been deleted to avoid corrupt data.";
        break;

      case EET_ERROR_WRITE_ERROR_FILE_TOO_BIG:
        erstr = "Elementary's settings files are too big "
                "for the file system they are being saved to. "
                "This error is very strange as the files should "
                "be extremely small. Please check the settings "
                "for your home directory. "
                "The file where the error occurred was: %s ."
                "This file has been deleted to avoid corrupt data.";
        break;

      case EET_ERROR_WRITE_ERROR_IO_ERROR:
        erstr = "An output error occurred when writing the settings "
                "files for Elementary. Your disk is having troubles "
                "and possibly needs replacement. "
                "The file where the error occurred was: %s ."
                "This file has been deleted to avoid corrupt data.";
        break;

      case EET_ERROR_WRITE_ERROR_OUT_OF_SPACE:
        erstr = "Elementary cannot write its settings file "
                "because it ran out of space to write the file. "
                "You have either run out of disk space or have "
                "gone over your quota limit. "
                "The file where the error occurred was: %s ."
                "This file has been deleted to avoid corrupt data.";
        break;

      case EET_ERROR_WRITE_ERROR_FILE_CLOSED:
        erstr = "Elementary unexpectedly had the settings file "
                "it was writing closed on it. This is very unusual. "
                "The file where the error occurred was: %s "
                "This file has been deleted to avoid corrupt data.";
        break;

      default:
        break;
     }
   if (erstr)
     {
        /* delete any partially-written file */
         ecore_file_unlink(file);
         return strdup(erstr);
     }

   return NULL;
}

static Eina_Bool
_elm_config_profile_save(void)
{
   char buf[4096], buf2[4096];
   int ok = 0, ret;
   const char *err;
   Eet_File *ef;
   size_t len;

   len = _elm_user_dir_snprintf(buf, sizeof(buf), "config/profile.cfg");
   if (len + 1 >= sizeof(buf))
     return EINA_FALSE;

   len = _elm_user_dir_snprintf(buf2, sizeof(buf2), "config/profile.cfg.tmp");
   if (len + 1 >= sizeof(buf2))
     return EINA_FALSE;

   ef = eet_open(buf2, EET_FILE_MODE_WRITE);
   if (!ef)
     return EINA_FALSE;

   ok = eet_write(ef, "config", _elm_profile, strlen(_elm_profile), 0);
   if (!ok)
     goto err;

   err = _elm_config_eet_close_error_get(ef, buf2);
   if (err)
     {
        ERR("%s", err);
        free((void *)err);
        goto err;
     }

   ret = ecore_file_mv(buf2, buf);
   if (!ret)
     {
        ERR("Error saving Elementary's configuration profile file");
        goto err;
     }

   ecore_file_unlink(buf2);
   return EINA_TRUE;

err:
   ecore_file_unlink(buf2);
   return EINA_FALSE;
}

Eina_Bool
_elm_config_save(void)
{
   char buf[4096], buf2[4096];
   int ok = 0, ret;
   const char *err;
   Eet_File *ef;
   size_t len;

   len = _elm_user_dir_snprintf(buf, sizeof(buf), "config/%s", _elm_profile);
   if (len + 1 >= sizeof(buf))
     return EINA_FALSE;

   ok = ecore_file_mkpath(buf);
   if (!ok)
     {
        ERR("Problem accessing Elementary's user configuration directory: %s",
            buf);
        return EINA_FALSE;
     }

   if (!_elm_config_profile_save())
     return EINA_FALSE;

   buf[len] = '/';
   len++;

   if (len + sizeof("base.cfg") >= sizeof(buf) - len)
     return EINA_FALSE;

   memcpy(buf + len, "base.cfg", sizeof("base.cfg"));
   len += sizeof("base.cfg") - 1;

   if (len + sizeof(".tmp") >= sizeof(buf))
     return EINA_FALSE;

   memcpy(buf2, buf, len);
   memcpy(buf2 + len, ".tmp", sizeof(".tmp"));

   ef = eet_open(buf2, EET_FILE_MODE_WRITE);
   if (!ef)
     return EINA_FALSE;

   ok = eet_data_write(ef, _config_edd, "config", _elm_config, 1);
   if (!ok)
     goto err;

   err = _elm_config_eet_close_error_get(ef, buf2);
   if (err)
     {
        ERR("%s", err);
        free((void *)err);
        goto err;
     }

   ret = ecore_file_mv(buf2, buf);
   if (!ret)
     {
        ERR("Error saving Elementary's configuration file");
        goto err;
     }

   ecore_file_unlink(buf2);
   return EINA_TRUE;

err:
   ecore_file_unlink(buf2);
   return EINA_FALSE;
}

// TIZEN_ONLY(20150323)
// In now implementation of this feature is only genlist.
void
_elm_config_scroll_item_valign_set(const char *scroll_item_valign)
{
   if (_elm_config->scroll_item_valign && strcmp(_elm_config->scroll_item_valign, scroll_item_valign))
     eina_stringshare_del(_elm_config->scroll_item_valign);

   _elm_config->scroll_item_valign = eina_stringshare_add(scroll_item_valign);
}
//////////////////////

static void
_config_update(void)
{
   Elm_Config *tcfg;

   tcfg = _config_system_load();
   if (!tcfg)
     {
        /* weird profile or something? We should probably fill
         * with hardcoded defaults, or get from default previx */
          return;
     }
#define IFCFG(v)   if ((_elm_config->config_version & 0xffff) < (v)) {
#define IFCFGELSE } else {
#define IFCFGEND  }
#define COPYVAL(x) do {_elm_config->x = tcfg->x; } while (0)
#define COPYPTR(x) do {_elm_config->x = tcfg->x; tcfg->x = NULL; } while (0)
#define COPYSTR(x) COPYPTR(x)

     /* we also need to update for property changes in the root window
      * if needed, but that will be dependent on new properties added
      * with each version */

     IFCFG(0x0003);
     COPYVAL(longpress_timeout);
     IFCFGEND;

#undef COPYSTR
#undef COPYPTR
#undef COPYVAL
#undef IFCFGEND
#undef IFCFGELSE
#undef IFCFG

     /* after updating user config, we must save */
}

static void
_env_get(void)
{
   char *s;
   double friction;

   s = getenv("ELM_ENGINE");
   if (s)
     {
        if ((!strcasecmp(s, "x11")) ||
            (!strcasecmp(s, "x")) ||
            (!strcasecmp(s, "software-x11")) ||
            (!strcasecmp(s, "software_x11")))
          eina_stringshare_replace(&_elm_config->engine, ELM_SOFTWARE_X11);
        else if ((!strcasecmp(s, "opengl")) ||
                 (!strcasecmp(s, "gl")) ||
                 (!strcasecmp(s, "opengl-x11")) ||
                 (!strcasecmp(s, "opengl_x11")))
          eina_stringshare_replace(&_elm_config->engine, ELM_OPENGL_X11);
        else if ((!strcasecmp(s, "x11-8")) ||
                 (!strcasecmp(s, "x8")) ||
                 (!strcasecmp(s, "software-8-x11")) ||
                 (!strcasecmp(s, "software_8_x11")))
          eina_stringshare_replace(&_elm_config->engine, ELM_SOFTWARE_8_X11);
        else if ((!strcasecmp(s, "x11-16")) ||
                 (!strcasecmp(s, "x16")) ||
                 (!strcasecmp(s, "software-16-x11")) ||
                 (!strcasecmp(s, "software_16_x11")))
          eina_stringshare_replace(&_elm_config->engine, ELM_SOFTWARE_16_X11);
/*
        else if ((!strcasecmp(s, "xrender")) ||
                 (!strcasecmp(s, "xr")) ||
                 (!strcasecmp(s, "xrender-x11")) ||
                 (!strcasecmp(s, "xrender_x11")))
          eina_stringshare_replace(&_elm_config->engine, ELM_XRENDER_X11);
 */
        else if ((!strcasecmp(s, "fb")) ||
                 (!strcasecmp(s, "software-fb")) ||
                 (!strcasecmp(s, "software_fb")))
          eina_stringshare_replace(&_elm_config->engine, ELM_SOFTWARE_FB);
        else if ((!strcasecmp(s, "directfb")) ||
                 (!strcasecmp(s, "dfb")))
          eina_stringshare_replace(&_elm_config->engine, ELM_SOFTWARE_DIRECTFB);
        else if ((!strcasecmp(s, "psl1ght")))
          eina_stringshare_replace(&_elm_config->engine, ELM_SOFTWARE_PSL1GHT);
        else if ((!strcasecmp(s, "sdl")) ||
                 (!strcasecmp(s, "software-sdl")) ||
                 (!strcasecmp(s, "software_sdl")))
          eina_stringshare_replace(&_elm_config->engine, ELM_SOFTWARE_SDL);
        else if ((!strcasecmp(s, "sdl-16")) ||
                 (!strcasecmp(s, "software-16-sdl")) ||
                 (!strcasecmp(s, "software_16_sdl")))
          eina_stringshare_replace(&_elm_config->engine, ELM_SOFTWARE_16_SDL);
        else if ((!strcasecmp(s, "opengl-sdl")) ||
                 (!strcasecmp(s, "opengl_sdl")) ||
                 (!strcasecmp(s, "gl-sdl")) ||
                 (!strcasecmp(s, "gl_sdl")))
          eina_stringshare_replace(&_elm_config->engine, ELM_OPENGL_SDL);
        else if ((!strcasecmp(s, "opengl-cocoa")) ||
                 (!strcasecmp(s, "opengl_cocoa")) ||
                 (!strcasecmp(s, "gl-cocoa")) ||
                 (!strcasecmp(s, "gl_cocoa")))
          eina_stringshare_replace(&_elm_config->engine, ELM_OPENGL_COCOA);
        else if ((!strcasecmp(s, "gdi")) ||
                 (!strcasecmp(s, "software-gdi")) ||
                 (!strcasecmp(s, "software_gdi")))
          eina_stringshare_replace(&_elm_config->engine, ELM_SOFTWARE_WIN32);
        else if ((!strcasecmp(s, "wince-gdi")) ||
                 (!strcasecmp(s, "software-16-wince-gdi")) ||
                 (!strcasecmp(s, "software_16_wince_gdi")))
          eina_stringshare_replace(&_elm_config->engine, ELM_SOFTWARE_16_WINCE);
        else if (!strcasecmp(s, "buffer"))
          eina_stringshare_replace(&_elm_config->engine, ELM_BUFFER);
        else if ((!strncmp(s, "shot:", 5)))
          eina_stringshare_replace(&_elm_config->engine, s);
        else if ((!strcasecmp(s, "ews")))
          eina_stringshare_replace(&_elm_config->engine, ELM_EWS);
        else if ((!strcasecmp(s, "wayland_shm")))
          eina_stringshare_replace(&_elm_config->engine, ELM_WAYLAND_SHM);
        else if ((!strcasecmp(s, "wayland_egl")))
          eina_stringshare_replace(&_elm_config->engine, ELM_WAYLAND_EGL);
        else
          ERR("Unknown engine '%s'.", s);
     }

   s = getenv("ELM_VSYNC");
   if (s) _elm_config->vsync = !!atoi(s);

   s = getenv("ELM_THUMBSCROLL_ENABLE");
   if (s) _elm_config->thumbscroll_enable = !!atoi(s);
   s = getenv("ELM_THUMBSCROLL_THRESHOLD");
   if (s) _elm_config->thumbscroll_threshold = atoi(s);
   s = getenv("ELM_THUMBSCROLL_HOLD_THRESHOLD");
   if (s) _elm_config->thumbscroll_hold_threshold = atoi(s);
   // FIXME: floatformat locale issues here 1.0 vs 1,0 - should just be 1.0
   s = getenv("ELM_THUMBSCROLL_MOMENTUM_THRESHOLD");
   if (s) _elm_config->thumbscroll_momentum_threshold = _elm_atof(s);
   s = getenv("ELM_THUMBSCROLL_FLICK_DISTANCE_TOLERANCE");
   if (s) _elm_config->thumbscroll_flick_distance_tolerance = atoi(s);
   s = getenv("ELM_THUMBSCROLL_FRICTION");
   if (s) _elm_config->thumbscroll_friction = _elm_atof(s);
   s = getenv("ELM_THUMBSCROLL_MIN_FRICTION");
   if (s) _elm_config->thumbscroll_min_friction = _elm_atof(s);
   s = getenv("ELM_THUMBSCROLL_FRICTION_STANDARD");
   if (s) _elm_config->thumbscroll_friction_standard = _elm_atof(s);
   s = getenv("ELM_THUMBSCROLL_BOUNCE_ENABLE");
   if (s) _elm_config->thumbscroll_bounce_enable = !!atoi(s);
   s = getenv("ELM_THUMBSCROLL_BOUNCE_FRICTION");
   if (s) _elm_config->thumbscroll_bounce_friction = _elm_atof(s);
   s = getenv("ELM_THUMBSCROLL_ACCELERATION_THRESHOLD");
   if (s) _elm_config->thumbscroll_acceleration_threshold = _elm_atof(s);
   s = getenv("ELM_THUMBSCROLL_ACCELERATION_TIME_LIMIT");
   if (s) _elm_config->thumbscroll_acceleration_time_limit = _elm_atof(s);
   s = getenv("ELM_THUMBSCROLL_ACCELERATION_WEIGHT");
   if (s) _elm_config->thumbscroll_acceleration_weight = _elm_atof(s);
   s = getenv("ELM_PAGE_SCROLL_FRICTION");
   if (s) _elm_config->page_scroll_friction = _elm_atof(s);
   s = getenv("ELM_BRING_IN_SCROLL_FRICTION");
   if (s) _elm_config->bring_in_scroll_friction = _elm_atof(s);
   s = getenv("ELM_ZOOM_FRICTION");
   if (s) _elm_config->zoom_friction = _elm_atof(s);
   s = getenv("ELM_THUMBSCROLL_BORDER_FRICTION");
   if (s)
     {
        friction = _elm_atof(s);
        if (friction < 0.0)
          friction = 0.0;

        if (friction > 1.0)
          friction = 1.0;

        _elm_config->thumbscroll_border_friction = friction;
     }
   s = getenv("ELM_THUMBSCROLL_SENSITIVITY_FRICTION");
   if (s)
     {
        friction = _elm_atof(s);
        if (friction < 0.1)
          friction = 0.1;

        if (friction > 1.0)
          friction = 1.0;

        _elm_config->thumbscroll_sensitivity_friction = friction;
     }
   s = getenv("ELM_THUMBSCROLL_DURATION");
   if (s) _elm_config->thumbscroll_duration = _elm_atof(s);
   s = getenv("ELM_THUMBSCROLL_CUBIC_BEZIER_P1_X");
   if (s) _elm_config->thumbscroll_cubic_bezier_p1_x = _elm_atof(s);
   s = getenv("ELM_THUMBSCROLL_CUBIC_BEZIER_P1_Y");
   if (s) _elm_config->thumbscroll_cubic_bezier_p1_y = _elm_atof(s);
   s = getenv("ELM_THUMBSCROLL_CUBIC_BEZIER_P2_X");
   if (s) _elm_config->thumbscroll_cubic_bezier_p2_x = _elm_atof(s);
   s = getenv("ELM_THUMBSCROLL_CUBIC_BEZIER_P2_Y");
   if (s) _elm_config->thumbscroll_cubic_bezier_p2_y = _elm_atof(s);
   s = getenv("ELM_SCROLL_SMOOTH_START_ENABLE");
   if (s) _elm_config->scroll_smooth_start_enable = !!atoi(s);
   s = getenv("ELM_SCROLL_SMOOTH_TIME_INTERVAL");
   if (s) _elm_config->scroll_smooth_time_interval = atof(s);
   s = getenv("ELM_SCROLL_SMOOTH_AMOUNT");
   if (s) _elm_config->scroll_smooth_amount = _elm_atof(s);
   s = getenv("ELM_SCROLL_SMOOTH_HISTORY_WEIGHT");
   if (s) _elm_config->scroll_smooth_history_weight = _elm_atof(s);
   s = getenv("ELM_SCROLL_SMOOTH_FUTURE_TIME");
   if (s) _elm_config->scroll_smooth_future_time = _elm_atof(s);
   s = getenv("ELM_SCROLL_SMOOTH_TIME_WINDOW");
   if (s) _elm_config->scroll_smooth_time_window = _elm_atof(s);
   s = getenv("ELM_THEME");
   if (s) eina_stringshare_replace(&_elm_config->theme, s);

   s = getenv("ELM_FONT_HINTING");
   if (s)
     {
        if      (!strcasecmp(s, "none")) _elm_config->font_hinting = 0;
        else if (!strcasecmp(s, "auto"))
          _elm_config->font_hinting = 1;
        else if (!strcasecmp(s, "bytecode"))
          _elm_config->font_hinting = 2;
     }

   s = getenv("ELM_FONT_PATH");
   if (s)
     {
        const char *p, *pp;
        char *buf2;

        EINA_LIST_FREE(_elm_config->font_dirs, p)
          {
             eina_stringshare_del(p);
          }

        buf2 = alloca(strlen(s) + 1);
        p = s;
        pp = p;
        for (;; )
          {
             if ((*p == ':') || (*p == 0))
               {
                  int len;

                  len = p - pp;
                  strncpy(buf2, pp, len);
                  buf2[len] = 0;
                  _elm_config->font_dirs =
                    eina_list_append(_elm_config->font_dirs,
                                     eina_stringshare_add(buf2));
                  if (*p == 0) break;
                  p++;
                  pp = p;
               }
             else
               {
                  if (*p == 0) break;
                  p++;
               }
          }
     }

   s = getenv("ELM_IMAGE_CACHE");
   if (s) _elm_config->image_cache = atoi(s);

   s = getenv("ELM_FONT_CACHE");
   if (s) _elm_config->font_cache = atoi(s);

   s = getenv("ELM_SCALE");
   if (s) _elm_config->scale = _elm_atof(s);

   s = getenv("ELM_FINGER_SIZE");
   if (s) _elm_config->finger_size = atoi(s);

   s = getenv("ELM_PASSWORD_SHOW_LAST");
   if (s) _elm_config->password_show_last = !!atoi(s);

   s = getenv("ELM_PASSWORD_SHOW_LAST_TIMEOUT");
   if (s)
     {
        double pw_show_last_timeout = _elm_atof(s);
        if (pw_show_last_timeout >= 0.0)
          _elm_config->password_show_last_timeout = pw_show_last_timeout;
     }

   s = getenv("ELM_FPS");
   if (s) _elm_config->fps = _elm_atof(s);
   if (_elm_config->fps < 1.0) _elm_config->fps = 1.0;

   s = getenv("ELM_MODULES");
   if (s) eina_stringshare_replace(&_elm_config->modules, s);

   s = getenv("ELM_TOOLTIP_DELAY");
   if (s)
     {
        double delay = _elm_atof(s);
        if (delay >= 0.0)
          _elm_config->tooltip_delay = delay;
     }

   s = getenv("ELM_CURSOR_ENGINE_ONLY");
   if (s) _elm_config->cursor_engine_only = !!atoi(s);

   s = getenv("ELM_FOCUS_HIGHLIGHT_ENABLE");
   if (s) _elm_config->focus_highlight_enable = !!atoi(s);

   s = getenv("ELM_FOCUS_HIGHLIGHT_ANIMATE");
   if (s) _elm_config->focus_highlight_animate = !!atoi(s);

   s = getenv("ELM_TOOLBAR_SHRINK_MODE");
   if (s) _elm_config->toolbar_shrink_mode = atoi(s);

   s = getenv("ELM_FILESELECTOR_EXPAND_ENABLE");
   if (s) _elm_config->fileselector_expand_enable = !!atoi(s);

   s = getenv("ELM_INWIN_DIALOGS_ENABLE");
   if (s) _elm_config->inwin_dialogs_enable = !!atoi(s);

   s = getenv("ELM_ICON_SIZE");
   if (s) _elm_config->icon_size = atoi(s);

   s = getenv("ELM_LONGPRESS_TIMEOUT");
   if (s) _elm_config->longpress_timeout = _elm_atof(s);
   if (_elm_config->longpress_timeout < 0.0)
     _elm_config->longpress_timeout = 0.0;

   s = getenv("ELM_EFFECT_ENABLE");
   if (s) _elm_config->effect_enable = !!atoi(s);

   s = getenv("ELM_DESKTOP_ENTRY");
   if (s) _elm_config->desktop_entry = !!atoi(s);

   s = getenv("ELM_ACCESS_MODE");
   if (s) _elm_config->access_mode = ELM_ACCESS_MODE_ON;
   s = getenv("ELM_ACCESS_PASSWORD_READ_ENABLE");
   if (s) _elm_config->access_password_read_enable = !!atoi(s);

   s = getenv("ELM_SELECTION_CLEAR_ENABLE");
   if (s) _elm_config->selection_clear_enable = !!atoi(s);

   s = getenv("ELM_AUTO_THROTTLE");
   if (s) _elm_config->auto_throttle = EINA_TRUE;
   s = getenv("ELM_AUTO_THROTTLE_AMOUNT");
   if (s) _elm_config->auto_throttle_amount = _elm_atof(s);
   s = getenv("ELM_AUTO_NORENDER_WITHDRAWN");
   if (s) _elm_config->auto_norender_withdrawn = EINA_TRUE;
   s = getenv("ELM_AUTO_NORENDER_ICONIFIED_SAME_AS_WITHDRAWN");
   if (s) _elm_config->auto_norender_iconified_same_as_withdrawn = EINA_TRUE;
   s = getenv("ELM_AUTO_FLUSH_WITHDRAWN");
   if (s) _elm_config->auto_flush_withdrawn = EINA_TRUE;
   s = getenv("ELM_AUTO_DUMP_WIDTHDRAWN");
   if (s) _elm_config->auto_dump_withdrawn = EINA_TRUE;

   s = getenv("ELM_INDICATOR_SERVICE_0");
   if (s) eina_stringshare_replace(&_elm_config->indicator_service_0, s);
   s = getenv("ELM_INDICATOR_SERVICE_90");
   if (s) eina_stringshare_replace(&_elm_config->indicator_service_90, s);
   s = getenv("ELM_INDICATOR_SERVICE_180");
   if (s) eina_stringshare_replace(&_elm_config->indicator_service_180, s);
   s = getenv("ELM_INDICATOR_SERVICE_270");
   if (s) eina_stringshare_replace(&_elm_config->indicator_service_270, s);
   s = getenv("ELM_GENLIST_ANIMATION_DURATION");
   if (s) _elm_config->genlist_animation_duration = _elm_atof(s);
   s = getenv("ELM_GENGRID_REORDER_DURATION");
   if (s) _elm_config->gengrid_reorder_duration = _elm_atof(s);
   s = getenv("ELM_GENGRID_ANIMATION_DURATION");
   if (s) _elm_config->gengrid_animation_duration = _elm_atof(s);
}

EAPI Eina_Bool
elm_config_mirrored_get(void)
{
   return _elm_config->is_mirrored;
}

EAPI void
elm_config_mirrored_set(Eina_Bool mirrored)
{
   mirrored = !!mirrored;
   if (_elm_config->is_mirrored == mirrored) return;
   _elm_config->is_mirrored = mirrored;
   _elm_rescale();
}

EAPI Eina_Bool
elm_config_cursor_engine_only_get(void)
{
   return _elm_config->cursor_engine_only;
}

EAPI void
elm_config_cursor_engine_only_set(Eina_Bool engine_only)
{
   engine_only = !!engine_only;
   _elm_config->cursor_engine_only = engine_only;
}

EAPI double
elm_config_tooltip_delay_get(void)
{
   return _elm_config->tooltip_delay;
}

EAPI void
elm_config_tooltip_delay_set(double delay)
{
   if (delay < 0.0) return;
   _elm_config->tooltip_delay = delay;
}

EAPI double
elm_config_scale_get(void)
{
   return _elm_config->scale;
}

EAPI void
elm_config_scale_set(double scale)
{
   if (scale < 0.0) return;
   if (_elm_config->scale == scale) return;
   _elm_config->scale = scale;
   _elm_rescale();
}

EAPI Eina_Bool
elm_config_password_show_last_get(void)
{
   return _elm_config->password_show_last;
}

EAPI void
elm_config_password_show_last_set(Eina_Bool password_show_last)
{
   if (_elm_config->password_show_last == password_show_last) return;
   _elm_config->password_show_last = password_show_last;
   edje_password_show_last_set(_elm_config->password_show_last);
}

EAPI double
elm_config_password_show_last_timeout_get(void)
{
   return _elm_config->password_show_last_timeout;
}

EAPI void
elm_config_password_show_last_timeout_set(double password_show_last_timeout)
{
   if (password_show_last_timeout < 0.0) return;
   if (_elm_config->password_show_last_timeout == password_show_last_timeout) return;
   _elm_config->password_show_last_timeout = password_show_last_timeout;
   edje_password_show_last_timeout_set(_elm_config->password_show_last_timeout);
}

EAPI Eina_Bool
elm_config_save(void)
{
   return _elm_config_save();
}

EAPI void
elm_config_reload(void)
{
   _elm_config_reload();
}

EAPI const char *
elm_config_profile_get(void)
{
   return _elm_config_current_profile_get();
}

EAPI const char *
elm_config_profile_dir_get(const char *profile,
                    Eina_Bool   is_user)
{
   return _elm_config_profile_dir_get(profile, is_user);
}

EAPI void
elm_config_profile_dir_free(const char *p_dir)
{
   free((void *)p_dir);
}

EAPI Eina_List *
elm_config_profile_list_get(void)
{
   return _elm_config_profiles_list();
}

EAPI void
elm_config_profile_list_free(Eina_List *l)
{
   const char *dir;

   EINA_LIST_FREE(l, dir)
     eina_stringshare_del(dir);
}

EAPI void
elm_config_profile_set(const char *profile)
{
   EINA_SAFETY_ON_NULL_RETURN(profile);
   _elm_config_profile_set(profile);
}

EAPI Eina_Bool
elm_config_profile_exists(const char *profile)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(profile, EINA_FALSE);
   return _elm_config_profile_exists(profile);
}

EAPI const char *
elm_config_engine_get(void)
{
   return _elm_config->engine;
}

EAPI void
elm_config_engine_set(const char *engine)
{
   EINA_SAFETY_ON_NULL_RETURN(engine);

   _elm_config_engine_set(engine);
}

EAPI Eina_List *
elm_config_text_classes_list_get(void)
{
   return _elm_config_text_classes_get();
}

EAPI void
elm_config_text_classes_list_free(Eina_List *list)
{
   _elm_config_text_classes_free(list);
}

EAPI const Eina_List *
elm_config_font_overlay_list_get(void)
{
   return _elm_config_font_overlays_list();
}

EAPI Eina_Bool
elm_config_access_get(void)
{
   return _elm_config_access_get();
}

EAPI void
elm_config_access_set(Eina_Bool is_access)
{
   _elm_config_access_set(is_access);
}

EAPI Eina_Bool
elm_config_access_password_read_enabled_get(void)
{
   return _elm_config_access_password_read_enabled_get();
}

EAPI void
elm_config_access_password_read_enabled_set(Eina_Bool enabled)
{
   _elm_config_access_password_read_enabled_set(enabled);
}

EAPI Eina_Bool
elm_config_selection_unfocused_clear_get(void)
{
   return _elm_config_selection_unfocused_clear_get();
}

EAPI void
elm_config_selection_unfocused_clear_set(Eina_Bool enabled)
{
   _elm_config_selection_unfocused_clear_set(enabled);
}

EAPI void
elm_config_font_overlay_set(const char    *text_class,
                     const char    *font,
                     Evas_Font_Size size)
{
   EINA_SAFETY_ON_NULL_RETURN(text_class);
   _elm_config_font_overlay_set(text_class, font, size);
}

EAPI void
elm_config_font_overlay_unset(const char *text_class)
{
   EINA_SAFETY_ON_NULL_RETURN(text_class);
   _elm_config_font_overlay_remove(text_class);
}

EAPI void
elm_config_font_overlay_apply(void)
{
   _elm_config_font_overlay_apply();
   _elm_rescale();
}

EAPI Eina_List *
elm_config_color_classes_list_get(void)
{
   return _elm_config_color_classes_get();
}

EAPI void
elm_config_color_classes_list_free(Eina_List *list)
{
   _elm_config_color_classes_free(list);
}

EAPI const Eina_List *
elm_config_color_overlay_list_get(void)
{
   return _elm_config_color_overlays_list();
}

EAPI void
elm_config_color_overlay_set(const char *color_class,
                             int r, int g, int b, int a,
                             int r2, int g2, int b2, int a2,
                             int r3, int g3, int b3, int a3)
{
   EINA_SAFETY_ON_NULL_RETURN(color_class);
   _elm_config_color_overlay_set(color_class,
                                 r, g, b, a,
                                 r2, g2, b2, a2,
                                 r3, g3, b3, a3);
}

EAPI void
elm_config_color_overlay_unset(const char *color_class)
{
   EINA_SAFETY_ON_NULL_RETURN(color_class);
   _elm_config_color_overlay_remove(color_class);
}

EAPI void
elm_config_color_overlay_apply(void)
{
   _elm_config_color_overlay_apply();
   _elm_rescale();
}

EAPI Evas_Coord
elm_config_finger_size_get(void)
{
   return _elm_config->finger_size;
}

EAPI void
elm_config_finger_size_set(Evas_Coord size)
{
   if (size < 0) return;
   if (_elm_config->finger_size == size) return;
   _elm_config->finger_size = size;
   _elm_rescale();
}

EAPI int
elm_config_cache_flush_interval_get(void)
{
   return _elm_config->cache_flush_poll_interval;
}

EAPI void
elm_config_cache_flush_interval_set(int size)
{
   EINA_SAFETY_ON_FALSE_RETURN(size > 0);
   if (_elm_config->cache_flush_poll_interval == size) return;
   _elm_config->cache_flush_poll_interval = size;

   _elm_recache();
}

EAPI Eina_Bool
elm_config_cache_flush_enabled_get(void)
{
   return _elm_config->cache_flush_enable;
}

EAPI void
elm_config_cache_flush_enabled_set(Eina_Bool enabled)
{
   enabled = !!enabled;
   if (_elm_config->cache_flush_enable == enabled) return;
   _elm_config->cache_flush_enable = enabled;

   _elm_recache();
}

EAPI int
elm_config_cache_font_cache_size_get(void)
{
   return _elm_config->font_cache;
}

EAPI void
elm_config_cache_font_cache_size_set(int size)
{
   if (size < 0) return;
   if (_elm_config->font_cache == size) return;
   _elm_config->font_cache = size;

   _elm_recache();
}

EAPI int
elm_config_cache_image_cache_size_get(void)
{
   return _elm_config->image_cache;
}

EAPI void
elm_config_cache_image_cache_size_set(int size)
{
   if (size < 0) return;
   if (_elm_config->image_cache == size) return;
   _elm_config->image_cache = size;

   _elm_recache();
}

EAPI int
elm_config_cache_edje_file_cache_size_get()
{
   return _elm_config->edje_cache;
}

EAPI void
elm_config_cache_edje_file_cache_size_set(int size)
{
   if (size < 0) return;
   if (_elm_config->edje_cache == size) return;
   _elm_config->edje_cache = size;

   _elm_recache();
}

EAPI int
elm_config_cache_edje_collection_cache_size_get(void)
{
   return _elm_config->edje_collection_cache;
}

EAPI void
elm_config_cache_edje_collection_cache_size_set(int size)
{
   if (_elm_config->edje_collection_cache == size) return;
   _elm_config->edje_collection_cache = size;

   _elm_recache();
}

EAPI Eina_Bool
elm_config_focus_highlight_enabled_get(void)
{
   return _elm_config->focus_highlight_enable;
}

EAPI void
elm_config_focus_highlight_enabled_set(Eina_Bool enable)
{
   _elm_config->focus_highlight_enable = !!enable;
}

EAPI Eina_Bool
elm_config_focus_highlight_animate_get(void)
{
   return _elm_config->focus_highlight_animate;
}

EAPI void
elm_config_focus_highlight_animate_set(Eina_Bool animate)
{
   _elm_config->focus_highlight_animate = !!animate;
}

EAPI Eina_Bool
elm_config_scroll_bounce_enabled_get(void)
{
   return _elm_config->thumbscroll_bounce_enable;
}

EAPI void
elm_config_scroll_bounce_enabled_set(Eina_Bool enabled)
{
   _elm_config->thumbscroll_bounce_enable = enabled;
}

EAPI double
elm_config_scroll_bounce_friction_get(void)
{
   return _elm_config->thumbscroll_bounce_friction;
}

EAPI void
elm_config_scroll_bounce_friction_set(double friction)
{
   _elm_config->thumbscroll_bounce_friction = friction;
}

EAPI double
elm_config_scroll_page_scroll_friction_get(void)
{
   return _elm_config->page_scroll_friction;
}

EAPI void
elm_config_scroll_page_scroll_friction_set(double friction)
{
   _elm_config->page_scroll_friction = friction;
}

EAPI double
elm_config_scroll_bring_in_scroll_friction_get(void)
{
   return _elm_config->bring_in_scroll_friction;
}

EAPI void
elm_config_scroll_bring_in_scroll_friction_set(double friction)
{
   _elm_config->bring_in_scroll_friction = friction;
}

EAPI double
elm_config_scroll_zoom_friction_get(void)
{
   return _elm_config->zoom_friction;
}

EAPI void
elm_config_scroll_zoom_friction_set(double friction)
{
   _elm_config->zoom_friction = friction;
}

EAPI Eina_Bool
elm_config_scroll_thumbscroll_enabled_get(void)
{
   return _elm_config->thumbscroll_enable;
}

EAPI void
elm_config_scroll_thumbscroll_enabled_set(Eina_Bool enabled)
{
   _elm_config->thumbscroll_enable = enabled;
}

EAPI unsigned int
elm_config_scroll_thumbscroll_threshold_get(void)
{
   return _elm_config->thumbscroll_threshold;
}

EAPI void
elm_config_scroll_thumbscroll_threshold_set(unsigned int threshold)
{
   _elm_config->thumbscroll_threshold = threshold;
}

EAPI unsigned int
elm_config_scroll_thumbscroll_hold_threshold_get(void)
{
   return _elm_config->thumbscroll_hold_threshold;
}

EAPI void
elm_config_scroll_thumbscroll_hold_threshold_set(unsigned int threshold)
{
   _elm_config->thumbscroll_hold_threshold = threshold;
}

EAPI double
elm_config_scroll_thumbscroll_momentum_threshold_get(void)
{
   return _elm_config->thumbscroll_momentum_threshold;
}

EAPI void
elm_config_scroll_thumbscroll_momentum_threshold_set(double threshold)
{
   _elm_config->thumbscroll_momentum_threshold = threshold;
}

EAPI unsigned int
elm_config_scroll_thumbscroll_flick_distance_tolerance_get(void)
{
   return _elm_config->thumbscroll_flick_distance_tolerance;
}

EAPI void
elm_config_scroll_thumbscroll_flick_distance_tolerance_set(unsigned int distance)
{
   _elm_config->thumbscroll_flick_distance_tolerance = distance;
}

EAPI double
elm_config_scroll_thumbscroll_friction_get(void)
{
   return _elm_config->thumbscroll_friction;
}

EAPI void
elm_config_scroll_thumbscroll_friction_set(double friction)
{
   _elm_config->thumbscroll_friction = friction;
}

EAPI double
elm_config_scroll_thumbscroll_min_friction_get(void)
{
   return _elm_config->thumbscroll_min_friction;
}

EAPI void
elm_config_scroll_thumbscroll_min_friction_set(double friction)
{
   _elm_config->thumbscroll_min_friction = friction;
}

EAPI double
elm_config_scroll_thumbscroll_friction_standard_get(void)
{
   return _elm_config->thumbscroll_friction_standard;
}

EAPI void
elm_config_scroll_thumbscroll_friction_standard_set(double standard)
{
   _elm_config->thumbscroll_friction_standard = standard;
}

EAPI double
elm_config_scroll_thumbscroll_border_friction_get(void)
{
   return _elm_config->thumbscroll_border_friction;
}

EAPI void
elm_config_scroll_thumbscroll_border_friction_set(double friction)
{
   if (friction < 0.0) friction = 0.0;
   if (friction > 1.0) friction = 1.0;
   _elm_config->thumbscroll_border_friction = friction;
}

EAPI double
elm_config_scroll_thumbscroll_sensitivity_friction_get(void)
{
   return _elm_config->thumbscroll_sensitivity_friction;
}

EAPI void
elm_config_scroll_thumbscroll_sensitivity_friction_set(double friction)
{
   if (friction < 0.1) friction = 0.1;
   if (friction > 1.0) friction = 1.0;
   _elm_config->thumbscroll_sensitivity_friction = friction;
}

EAPI double
elm_config_scroll_thumbscroll_acceleration_threshold_get(void)
{
   return _elm_config->thumbscroll_acceleration_threshold;
}

EAPI void
elm_config_scroll_thumbscroll_acceleration_threshold_set(double threshold)
{
   _elm_config->thumbscroll_acceleration_threshold = threshold;
}

EAPI double
elm_config_scroll_thumbscroll_acceleration_time_limit_get(void)
{
   return _elm_config->thumbscroll_acceleration_time_limit;
}

EAPI void
elm_config_scroll_thumbscroll_acceleration_time_limit_set(double time_limit)
{
   _elm_config->thumbscroll_acceleration_time_limit = time_limit;
}

EAPI double
elm_config_scroll_thumbscroll_acceleration_weight_get(void)
{
   return _elm_config->thumbscroll_acceleration_weight;
}

// TIZEN_ONLY(20150323)
// In now implementation of this feature is only genlist.
EAPI void
elm_config_scroll_thumbscroll_acceleration_weight_set(double weight)
{
   _elm_config->thumbscroll_acceleration_weight = weight;
}

EAPI void
elm_config_scroll_item_align_enabled_set(Eina_Bool enable)
{
   _elm_config->scroll_item_align_enable = !!enable;
}

EAPI Eina_Bool
elm_config_scroll_item_align_enabled_get(void)
{
   return _elm_config->scroll_item_align_enable;
}

EAPI void
elm_config_scroll_item_valign_set(const char *scroll_item_valign)
{
   EINA_SAFETY_ON_NULL_RETURN(scroll_item_valign);
   _elm_config_scroll_item_valign_set(scroll_item_valign);
}

EAPI const char *
elm_config_scroll_item_valign_get(void)
{
   return _elm_config->scroll_item_valign;
}
////////////////

EAPI void
elm_config_longpress_timeout_set(double longpress_timeout)
{
   _elm_config->longpress_timeout = longpress_timeout;
}

EAPI double
elm_config_longpress_timeout_get(void)
{
   return _elm_config->longpress_timeout;
}

EAPI void
elm_config_softcursor_mode_set(Elm_Softcursor_Mode mode)
{
   _elm_config->softcursor_mode = mode;
}

EAPI Elm_Softcursor_Mode
elm_config_softcursor_mode_get(void)
{
   return _elm_config->softcursor_mode;
}

EAPI double
elm_config_glayer_long_tap_start_timeout_get(void)
{
   return _elm_config->glayer_long_tap_start_timeout;
}

EAPI void
elm_config_glayer_long_tap_start_timeout_set(double long_tap_timeout)
{
   _elm_config->glayer_long_tap_start_timeout = long_tap_timeout;
}

EAPI double
elm_config_glayer_double_tap_timeout_get(void)
{
   return _elm_config->glayer_double_tap_timeout;
}

EAPI void
elm_config_glayer_double_tap_timeout_set(double double_tap_timeout)
{
   _elm_config->glayer_double_tap_timeout = double_tap_timeout;
}

EAPI void
elm_config_all_flush(void)
{
#ifdef HAVE_ELEMENTARY_X
   _prop_config_set();
   ecore_x_window_prop_string_set(_config_win, _atom[ATOM_E_PROFILE],
                                  _elm_profile);
#endif
}

static void
_translation_init()
{
#ifdef ENABLE_NLS
   /*
   const char *cur_dom = textdomain(NULL);
   const char *trans_comment = gettext("");
   const char *msg_locale = setlocale(LC_MESSAGES, NULL);
   */

   /* How does it decide translation with current domain??
      Application could use their own text domain.
      This is insane to me. */

   /* Same concept as what glib does:
    * We shouldn't translate if there are no translations for the
    * application in the current locale + domain. (Unless locale is
    * en_/C where translating only parts of the interface make some
    * sense).
    */
   /*
      _elm_config->translate = !(strcmp (cur_dom, "messages") &&
      !*trans_comment && strncmp (msg_locale, "en_", 3) &&
      strcmp (msg_locale, "C"));
    */
   /* Get RTL orientation from system */
   if (_elm_config->translate)
     {
        bindtextdomain(PACKAGE, LOCALE_DIR);
        // TIZEN_ONLY(20150925)
        // WC1 doesn't support mirroring mode
        // _elm_config->is_mirrored = !strcmp(E_("default:LTR"), "default:RTL");
        _elm_config->is_mirrored = EINA_FALSE;
     }

#endif
}

void
_elm_config_init(void)
{
   if (!ELM_EVENT_CONFIG_ALL_CHANGED)
      ELM_EVENT_CONFIG_ALL_CHANGED = ecore_event_type_new();
   _desc_init();
   _profile_fetch_from_conf();
   _config_load();
   _env_get();
   if (_elm_preferred_engine) eina_stringshare_del(_elm_preferred_engine);
   _elm_preferred_engine = NULL;
   ELM_SAFE_FREE(_elm_preference, eina_stringshare_del);
   ELM_SAFE_FREE(_elm_accel_preference, eina_stringshare_del);
   _translation_init();
   _config_apply();
   _elm_config_font_overlay_apply();
   _elm_config_color_overlay_apply();
   _elm_recache();
}

void
_elm_config_sub_shutdown(void)
{
#ifdef HAVE_ELEMENTARY_X
   if (_prop_change_delay_timer) ecore_timer_del(_prop_change_delay_timer);
   _prop_change_delay_timer = NULL;
#endif

#define ENGINE_COMPARE(name) (!strcmp(_elm_config->engine, name))
   if (ENGINE_COMPARE(ELM_SOFTWARE_X11) ||
       ENGINE_COMPARE(ELM_SOFTWARE_16_X11) ||
       ENGINE_COMPARE(ELM_XRENDER_X11) ||
       ENGINE_COMPARE(ELM_OPENGL_X11) ||
       ENGINE_COMPARE(ELM_OPENGL_COCOA))
#undef ENGINE_COMPARE
     {
#ifdef HAVE_ELEMENTARY_X
        ecore_x_shutdown();
#endif
     }
}

void
_elm_config_sub_init(void)
{
#define ENGINE_COMPARE(name) (!strcmp(_elm_config->engine, name))
   if (ENGINE_COMPARE(ELM_SOFTWARE_X11) ||
       ENGINE_COMPARE(ELM_SOFTWARE_16_X11) ||
       ENGINE_COMPARE(ELM_XRENDER_X11) ||
       ENGINE_COMPARE(ELM_OPENGL_X11) ||
       ENGINE_COMPARE(ELM_OPENGL_COCOA))
#undef ENGINE_COMPARE
     {
#ifdef HAVE_ELEMENTARY_X
        if (ecore_x_init(NULL))
          {
             Ecore_X_Window win = 0, win2 = 0, root;

             if (!ecore_x_screen_is_composited(0))
               _elm_config->compositing = 0;

             ecore_x_atoms_get(_atom_names, ATOM_COUNT, _atom);
             root = ecore_x_window_root_first_get();
             if (ecore_x_window_prop_window_get(root,
                                                _atom[ATOM_E_CONFIG_WIN],
                                                &win, 1) == 1)
               {
                  if (ecore_x_window_prop_window_get(win,
                                                     _atom[ATOM_E_CONFIG_WIN],
                                                     &win2, 1) == 1)
                    {
                       if (win2 == win) _config_win = win;
                    }
               }
             if (_config_win == 0)
               _config_win = ecore_x_window_permanent_create
                             (root, _atom[ATOM_E_CONFIG_WIN]);

             ecore_x_event_mask_set(_config_win,
                                    ECORE_X_EVENT_MASK_WINDOW_PROPERTY);
             _prop_change_handler = ecore_event_handler_add
               (ECORE_X_EVENT_WINDOW_PROPERTY, _prop_change, NULL);
             if (!getenv("ELM_PROFILE"))
               {
                  char *s;
                  
                  s = ecore_x_window_prop_string_get(_config_win,
                                                     _atom[ATOM_E_PROFILE]);
                  if (s)
                    {
                       int changed = 0;
                       
                       if (_elm_profile)
                         {
                            if (strcmp(_elm_profile, s)) changed = 1;
                            free(_elm_profile);
                         }
                       _elm_profile = s;
                       if (changed) _prop_config_get();
                    }
               }
          }
        else
          ERR("Cannot connect to X11 display. check $DISPLAY variable");
#endif
     }
   _config_sub_apply();
   if (_elm_config->modules) _elm_module_parse(_elm_config->modules);
}

void
_elm_config_reload(void)
{
   _config_free();
   _config_load();
   _config_apply();
   _elm_config_font_overlay_apply();
   _elm_config_color_overlay_apply();
   _elm_rescale();
   _elm_recache();
   ecore_event_add(ELM_EVENT_CONFIG_ALL_CHANGED, NULL, NULL, NULL);
}

void
_elm_config_engine_set(const char *engine)
{
   if (_elm_config->engine && strcmp(_elm_config->engine, engine))
     eina_stringshare_del(_elm_config->engine);

   _elm_config->engine = eina_stringshare_add(engine);
}
EAPI const char *
elm_config_preferred_engine_get(void)
{
   return _elm_preferred_engine;
}

EAPI void
elm_config_preferred_engine_set(const char *engine)
{
   if (engine)
     eina_stringshare_replace(&(_elm_preferred_engine), engine);
   else
     {
        if (_elm_preferred_engine) eina_stringshare_del(_elm_preferred_engine);
        _elm_preferred_engine = NULL;
     }
}

EAPI const char *
elm_config_accel_preference_get(void)
{
   if (_elm_preference) return _elm_preference;
   if (_elm_accel_preference) return _elm_accel_preference;
   return _elm_config->accel;
}

EAPI void
elm_config_accel_preference_set(const char *pref)
{
   if (pref)
     {
       char **arr;
       int tokens=0, i=0;
       Eina_Bool is_hw_accel = EINA_FALSE;
       char *item_splite = ":";

       /* Accel preference's string has the egl surface configuration as a hw accel, depth, stencil and msaa.
         * The string format is   "{HW Accel}:depth{value}:stencil{value}:msaa{msaa string}"
         * Especially, msaa string is related Evas GL MSAA enum value(low, mid, high)
         * so msaa string has four types as msaa, msaa_low, msaa_mid, msaa_high
         * For instance, "opengl:depth24:stencil8:msaa_high".
         * It means that using hw accelation, window surface depth buffer's size is 24, stencil buffer's size 8 and msaa bits is the highest.
         * The other use-case is  "opengl:depth24".
         * It measn that using hw accelation, depth buffer size is 24. stencil and msaa are not used.
         * Default case is  "opengl:depth:stencil:msaa".
         * It means that depth, stencil and msaa are setted by default value(depth:24, stencil:8, msaa:low)
         */

       DBG("accel preference's string: %s",pref);
        /* full string */
        eina_stringshare_replace(&(_elm_preference), pref);
        ELM_SAFE_FREE(_elm_accel_preference, eina_stringshare_del);
        ELM_SAFE_FREE(_elm_config->accel, eina_stringshare_del);

        /* split GL items (hw accel, gl depth, gl stencil, gl msaa */
        arr = eina_str_split_full(pref, item_splite,0,&tokens);
        for (i = 0; arr[i]; i++)
          {
             if ((!strcasecmp(arr[i], "gl")) ||
                  (!strcasecmp(arr[i], "opengl")) ||
                  (!strcasecmp(arr[i], "3d")) ||
                  (!strcasecmp(arr[i], "hw")) ||
                  (!strcasecmp(arr[i], "accel")) ||
                  (!strcasecmp(arr[i], "hardware"))
                )
               {
                  eina_stringshare_replace(&(_elm_accel_preference), arr[i]);
                  eina_stringshare_replace(&(_elm_config->accel), arr[i]);
                  is_hw_accel = EINA_TRUE;
               }
             else if (!strncmp(arr[i],"depth",5))
               {
                  char *value_str = arr[i] + 5;
                  if ((value_str) && (isdigit(*value_str)))
                    _elm_config->gl_depth = atoi(value_str);
                  else
                    _elm_config->gl_depth = 24;
               }
             else if (!strncmp(arr[i],"stencil",7))
               {
                  char *value_str = arr[i] + 7;
                  if ((value_str) && (isdigit(*value_str)))
                    _elm_config->gl_stencil = atoi(value_str);
                  else
                    _elm_config->gl_stencil = 8;
               }
             else if (!strncmp(arr[i],"msaa_low",8))
               _elm_config->gl_msaa = 1;             // 1 means msaa low
             else if (!strncmp(arr[i],"msaa_mid",8))
               _elm_config->gl_msaa = 2;             // 2 means msaa mid
             else if (!strncmp(arr[i],"msaa_high",9))
               _elm_config->gl_msaa = 4;             // 4 means msaa high
             else if (!strncmp(arr[i],"msaa",4))
               _elm_config->gl_msaa = 1;            // 1 means msaa low
          }

        DBG("accel: %s",_elm_accel_preference);
        DBG("gl depth: %d",_elm_config->gl_depth);
        DBG("gl stencil: %d",_elm_config->gl_stencil);
        DBG("gl msaa: %d",_elm_config->gl_msaa);
        free(arr[0]);
        free(arr);

        if (is_hw_accel == EINA_FALSE)
          {
             ELM_SAFE_FREE(_elm_accel_preference, eina_stringshare_del);
             ELM_SAFE_FREE(_elm_config->accel, eina_stringshare_del);
          }
     }
   else
     {
        ELM_SAFE_FREE(_elm_preference, eina_stringshare_del);
        ELM_SAFE_FREE(_elm_accel_preference, eina_stringshare_del);
        ELM_SAFE_FREE(_elm_config->accel, eina_stringshare_del);
     }
}

EAPI Eina_Bool
elm_config_accel_preference_override_get(void)
{
   return _elm_config->accel_override;
}

EAPI void
elm_config_accel_preference_override_set(Eina_Bool enabled)
{
   _elm_config->accel_override = enabled;
}

EAPI const char *
elm_config_indicator_service_get(int rotation)
{
   switch (rotation)
     {
      case 0:
        return _elm_config->indicator_service_0;
      case 90:
        return _elm_config->indicator_service_90;
      case 180:
        return _elm_config->indicator_service_180;
      case 270:
        return _elm_config->indicator_service_270;
      default:
        return NULL;
     }
}

EAPI double
elm_config_fps_get(void)
{
   return _elm_config->fps;
}

EAPI void
elm_config_fps_set(double fps)
{
   _elm_config->fps = fps;
}

////JIYOUN:START We have to remove this code after cluster home application change their code
EAPI const char *
elm_config_indicator_service_0_get(void)
{
   return _elm_config->indicator_service_0;
}

EAPI const char *
elm_config_indicator_service_90_get(void)
{
   return _elm_config->indicator_service_90;
}

EAPI const char *
elm_config_indicator_service_180_get(void)
{
   return _elm_config->indicator_service_180;
}

EAPI const char *
elm_config_indicator_service_270_get(void)
{
   return _elm_config->indicator_service_270;
}
////JIYOUN:END We have to remove this code after cluster home application change their code
void
_elm_config_profile_set(const char *profile)
{
   Eina_Bool changed = EINA_FALSE;

   if (_elm_profile)
     {
        if (strcmp(_elm_profile, profile))
          changed = 1;
        free(_elm_profile);
     }

   _elm_profile = strdup(profile);

   if (changed)
     {
        _config_free();
        _config_load();
        _env_get();
        _config_apply();
        _elm_config_font_overlay_apply();
        _elm_config_color_overlay_apply();
        _elm_rescale();
        _elm_recache();
     }
}

Eina_Bool
_elm_config_profile_exists(const char *profile)
{
   Eina_List *profs = _elm_config_profiles_list();
   Eina_List *l;
   const char *p, *dir;
   Eina_Bool res = EINA_FALSE;

   EINA_LIST_FOREACH(profs, l, p)
     {
        if (!strcmp(profile, p))
          {
             res = EINA_TRUE;
             break;
          }
     }

   EINA_LIST_FREE(profs, dir)
     eina_stringshare_del(dir);

   return res;
}

void
_elm_config_shutdown(void)
{
#define ENGINE_COMPARE(name) (!strcmp(_elm_config->engine, name))
   if (ENGINE_COMPARE(ELM_SOFTWARE_X11) ||
       ENGINE_COMPARE(ELM_SOFTWARE_16_X11) ||
       ENGINE_COMPARE(ELM_XRENDER_X11) ||
       ENGINE_COMPARE(ELM_OPENGL_X11))
#undef ENGINE_COMPARE
     {
#ifdef HAVE_ELEMENTARY_X
        ecore_event_handler_del(_prop_change_handler);
        _prop_change_handler = NULL;
#endif
     }
   _config_free();
   ELM_SAFE_FREE(_elm_preferred_engine, eina_stringshare_del);
   ELM_SAFE_FREE(_elm_accel_preference, eina_stringshare_del);
   ELM_SAFE_FREE(_elm_preference, eina_stringshare_del);
   ELM_SAFE_FREE(_elm_profile, free);

   _desc_shutdown();
}

