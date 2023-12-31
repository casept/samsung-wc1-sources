#ifndef ELM_PRIV_H
#define ELM_PRIV_H
#ifdef HAVE_CONFIG_H
#include "elementary_config.h"
#endif
#ifdef HAVE_ELEMENTARY_X
#include <Ecore_X.h>
#endif
#ifdef SDB_ENABLE
#include <Ecore_Ipc.h>
#endif
#ifdef HAVE_ELEMENTARY_FB
#include <Ecore_Fb.h>
#endif
#ifdef HAVE_ELEMENTARY_WINCE
#include <Ecore_WinCE.h>
#endif
#ifdef HAVE_ELEMENTARY_WAYLAND
#include <Ecore_Wayland.h>
#endif

#ifdef HAVE_EIO
# include <Eio.h>
#endif

#ifdef EAPI
# undef EAPI
#endif

#ifdef _WIN32
# ifdef ELEMENTARY_BUILD
#  ifdef DLL_EXPORT
#   define EAPI __declspec(dllexport)
#  else
#   define EAPI
#  endif /* ! DLL_EXPORT */
# else
#  define EAPI __declspec(dllimport)
# endif /* ! EFL_EVAS_BUILD */
#else
# ifdef __GNUC__
#  if __GNUC__ >= 4
#   define EAPI __attribute__ ((visibility("default")))
#  else
#   define EAPI
#  endif
# else
#  define EAPI
# endif
#endif /* ! _WIN32 */

#include "elm_widget.h"

#ifdef HAVE_LANGINFO_H
# include <langinfo.h>
#endif

#define CRITICAL(...) EINA_LOG_DOM_CRIT(_elm_log_dom, __VA_ARGS__)
#define ERR(...)      EINA_LOG_DOM_ERR(_elm_log_dom, __VA_ARGS__)
#define WRN(...)      EINA_LOG_DOM_WARN(_elm_log_dom, __VA_ARGS__)
#define INF(...)      EINA_LOG_DOM_INFO(_elm_log_dom, __VA_ARGS__)
#define DBG(...)      EINA_LOG_DOM_DBG(_elm_log_dom, __VA_ARGS__)

#ifdef ENABLE_NLS
# include <libintl.h>
# define E_(string)    _elm_dgettext(string)
#else
# ifndef setlocale
#  define setlocale(c, l)
# endif
# ifndef libintl_setlocale
#  define libintl_setlocale(c, l)
# endif
# ifndef bindtextdomain
#  define bindtextdomain(domain, dir)
# endif
# ifndef libintl_bindtextdomain
#  define libintl_bindtextdomain(domain, dir)
# endif
# define E_(string) (string)
#endif
#define N_(string) (string)

typedef struct _Edje_Signal_Data         Edje_Signal_Data;
typedef struct _Elm_Config               Elm_Config;
typedef struct _Elm_Module               Elm_Module;
typedef struct _Elm_Datetime_Module_Data Elm_Datetime_Module_Data;

struct _Edje_Signal_Data
{
   Evas_Object   *obj;
   Edje_Signal_Cb func;
   const char    *emission;
   const char    *source;
   void          *data;
};

struct _Elm_Theme
{
   Eina_List  *overlay;
   Eina_List  *themes;
   Eina_List  *extension;
   Eina_Hash  *cache;
   Eina_Hash  *cache_data;
   Elm_Theme  *ref_theme;
   Eina_List  *referrers;
   const char *theme;
   int         ref;
};

/* increment this whenever we change config enough that you need new
 * defaults for elm to work.
 */
#define ELM_CONFIG_EPOCH           0x0001
/* increment this whenever a new set of config values are added but
 * the users config doesn't need to be wiped - simply new values need
 * to be put in
 */
#define ELM_CONFIG_FILE_GENERATION 0x0003
#define ELM_CONFIG_VERSION         ((ELM_CONFIG_EPOCH << 16) | \
                                    ELM_CONFIG_FILE_GENERATION)
/* NB: profile configuration files (.src) must have their
 * "config_version" entry's value up-to-date with ELM_CONFIG_VERSION
 * (in decimal)!! */

/* note: always remember to sync it with elm_config.c */
extern const char *_elm_engines[];

#define ELM_SOFTWARE_X11      (_elm_engines[0])
#define ELM_SOFTWARE_FB       (_elm_engines[1])
#define ELM_SOFTWARE_DIRECTFB (_elm_engines[2])
#define ELM_SOFTWARE_16_X11   (_elm_engines[3])
#define ELM_SOFTWARE_8_X11    (_elm_engines[4])
#define ELM_XRENDER_X11       (_elm_engines[5])
#define ELM_OPENGL_X11        (_elm_engines[6])
#define ELM_SOFTWARE_WIN32    (_elm_engines[7])
#define ELM_SOFTWARE_16_WINCE (_elm_engines[8])
#define ELM_SOFTWARE_SDL      (_elm_engines[9])
#define ELM_SOFTWARE_16_SDL   (_elm_engines[10])
#define ELM_OPENGL_SDL        (_elm_engines[11])
#define ELM_BUFFER            (_elm_engines[12])
#define ELM_EWS               (_elm_engines[13])
#define ELM_OPENGL_COCOA      (_elm_engines[14])
#define ELM_SOFTWARE_PSL1GHT  (_elm_engines[15])
#define ELM_WAYLAND_SHM       (_elm_engines[16])
#define ELM_WAYLAND_EGL       (_elm_engines[17])

#define ELM_FONT_TOKEN_STYLE  ":style="

#define ELM_ACCESS_MODE_OFF   EINA_FALSE
#define ELM_ACCESS_MODE_ON    EINA_TRUE

#undef MIN
#define MIN(x, y) (((x) < (y)) ? (x) : (y))
#undef MAX
#define MAX(x, y) (((x) > (y)) ? (x) : (y))

#define ELM_SAFE_FREE(_h, _fn) do { if (_h) { _fn((void*)_h); _h = NULL; } } while (0)

struct _Elm_Config
{
   int           config_version;
   const char   *engine;
   const char   *accel;
   unsigned char accel_override;
   unsigned char vsync;
   unsigned char thumbscroll_enable;
   int           thumbscroll_threshold;
   int           thumbscroll_hold_threshold;
   double        thumbscroll_momentum_threshold;
   int           thumbscroll_flick_distance_tolerance;
   double        thumbscroll_friction;
   double        thumbscroll_min_friction;
   double        thumbscroll_friction_standard;
   double        thumbscroll_bounce_friction;
   double        thumbscroll_acceleration_threshold;
   double        thumbscroll_acceleration_time_limit;
   double        thumbscroll_acceleration_weight;
   double        page_scroll_friction;
   double        bring_in_scroll_friction;
   double        zoom_friction;
   unsigned char thumbscroll_bounce_enable;
   double        thumbscroll_border_friction;
   double        thumbscroll_sensitivity_friction;
   double        thumbscroll_duration;
   double        thumbscroll_cubic_bezier_p1_x;
   double        thumbscroll_cubic_bezier_p1_y;
   double        thumbscroll_cubic_bezier_p2_x;
   double        thumbscroll_cubic_bezier_p2_y;
   unsigned char scroll_smooth_start_enable;
   double        scroll_smooth_time_interval;
   double        scroll_smooth_amount;
   double        scroll_smooth_history_weight;
   double        scroll_smooth_future_time;
   double        scroll_smooth_time_window;
   double        scale;
   int           bgpixmap;
   int           compositing;
   Eina_List    *font_dirs;
   Eina_List    *font_overlays;
   int           font_hinting;
   int           cache_flush_poll_interval;
   unsigned char cache_flush_enable;
   int           image_cache;
   int           font_cache;
   int           edje_cache;
   int           edje_collection_cache;
   int           finger_size;
   double        fps;
   const char   *theme;
   const char   *modules;
   double        tooltip_delay;
   unsigned char cursor_engine_only;
   unsigned char focus_highlight_enable;
   unsigned char focus_highlight_animate;
   int           toolbar_shrink_mode;
   unsigned char fileselector_expand_enable;
   unsigned char inwin_dialogs_enable;
   int           icon_size;
   double        longpress_timeout;
   unsigned char effect_enable;
   unsigned char desktop_entry;
   unsigned char password_show_last;
   double        password_show_last_timeout;
   unsigned char glayer_zoom_finger_enable;
   double        glayer_zoom_finger_factor;
   double        glayer_zoom_wheel_factor;
   double        glayer_zoom_distance_tolerance;
   double        glayer_rotate_finger_enable;
   double        glayer_rotate_angular_tolerance;
   double        glayer_line_min_length;
   double        glayer_line_distance_tolerance;
   double        glayer_line_angular_tolerance;
   unsigned int  glayer_flick_time_limit_ms;
   double        glayer_long_tap_start_timeout;
   double        glayer_double_tap_timeout;
   Eina_Bool     access_mode;
   Eina_Bool     access_password_read_enable;
   unsigned char glayer_continues_enable;
   int           week_start;
   int           weekend_start;
   int           weekend_len;
   int           year_min;
   int           year_max;
   Eina_List    *color_overlays;
   Eina_List    *color_palette;
   unsigned char softcursor_mode;
   unsigned char auto_norender_withdrawn;
   unsigned char auto_norender_iconified_same_as_withdrawn;
   unsigned char auto_flush_withdrawn;
   unsigned char auto_dump_withdrawn;
   unsigned char auto_throttle;
   double        auto_throttle_amount;
   const char   *indicator_service_0;
   const char   *indicator_service_90;
   const char   *indicator_service_180;
   const char   *indicator_service_270;
   unsigned char selection_clear_enable;
   unsigned char translate;
   double        genlist_animation_duration;
   double        gengrid_reorder_duration;
   double        gengrid_animation_duration;
   int           item_scroll_align;
   Eina_Bool     scroll_item_align_enable;
   const char   *scroll_item_valign;
   int            gl_depth;
   int            gl_stencil;
   int            gl_msaa;
   const char   *font_name;
   /* Not part of the EET file */
   Eina_Bool     is_mirrored : 1;
};

struct _Elm_Module
{
   int          version;
   const char  *name;
   const char  *as;
   const char  *so_path;
   const char  *data_dir;
   const char  *bin_dir;
   Eina_Module *module;
   void        *data;
   void        *api;
   int          (*init_func)(Elm_Module *m);
   int          (*shutdown_func)(Elm_Module *m);
   int          references;
};

struct _Elm_Datetime_Module_Data
{
   Evas_Object *base;
   const char  *(*field_format_get)(Evas_Object * obj,
                                    Elm_Datetime_Field_Type field_type);
   Eina_Bool    (*field_location_get)(Evas_Object *obj,
                                      Elm_Datetime_Field_Type field_type,
                                      int *loc);
   void         (*field_limit_get)(Evas_Object *obj,
                                   Elm_Datetime_Field_Type field_type,
                                   int *range_min,
                                   int *range_max);
   void         (*fields_min_max_get)(Evas_Object *obj,
                                      struct tm *set_value,
                                      struct tm *min_value,
                                      struct tm *max_value);
   Eina_List *(*fields_sorted_get)(Evas_Object *obj);
};

void                 _elm_emotion_init(void);
void                 _elm_emotion_shutdown(void);

int                  _elm_ews_wm_init(void);
void                 _elm_ews_wm_shutdown(void);
void                 _elm_ews_wm_rescale(Elm_Theme *th,
                                         Eina_Bool use_theme);

void                 _elm_win_shutdown(void);
void                 _elm_win_rescale(Elm_Theme *th,
                                      Eina_Bool use_theme);
void                 _elm_win_access(Eina_Bool is_access);
void                 _elm_win_translate(void);

#ifdef ELM_FEATURE_POWER_SAVE
typedef enum
{
   ELM_SCREEN_STATUS_ON,
   ELM_SCREEN_STATUS_DIM,
   ELM_SCREEN_STATUS_OFF
} Elm_Screen_Status;

Elm_Screen_Status    _elm_screen_status_get(void);
Eina_Bool            _elm_win_list_visibility_get(void);
Eina_Bool            _elm_power_save_enabled_get(void);
#endif

Ecore_X_Window       _elm_ee_xwin_get(const Ecore_Evas *ee);

Eina_Bool            _elm_theme_object_set(Evas_Object *parent,
                                           Evas_Object *o,
                                           const char *clas,
                                           const char *group,
                                           const char *style);
Eina_Bool            _elm_theme_object_icon_set(Evas_Object *o,
                                                const char *group,
                                                const char *style);
Eina_Bool             _elm_theme_set(Elm_Theme *th,
                                    Evas_Object *o,
                                    const char *clas,
                                    const char *group,
                                    const char *style);
Eina_Bool            _elm_theme_icon_set(Elm_Theme *th,
                                         Evas_Object *o,
                                         const char *group,
                                         const char *style);
Eina_Bool            _elm_theme_parse(Elm_Theme *th,
                                      const char *theme);
void                 _elm_theme_shutdown(void);

void                 _elm_module_init(void);
void                 _elm_module_shutdown(void);
void                 _elm_module_parse(const char *s);
Elm_Module          *_elm_module_find_as(const char *as);
Elm_Module          *_elm_module_add(const char *name,
                                     const char *as);
void                 _elm_module_del(Elm_Module *m);
Eina_Bool            _elm_module_load(Elm_Module *m);
void                 _elm_module_unload(Elm_Module *m);
const void          *_elm_module_symbol_get(Elm_Module *m,
                                            const char *name);

void                 _elm_widget_top_win_focused_set(Evas_Object *obj,
                                                     Eina_Bool top_win_focused);
Eina_Bool            _elm_widget_top_win_focused_get(const Evas_Object *obj);

// TIZEN ONLY (20131211) : For broadcasting the state of focus highlight.
void                 _elm_widget_focus_highlighted_set(Evas_Object *obj,
                                                       Eina_Bool focus_highlighted);
Eina_Bool            _elm_widget_focus_highlighted_get(const Evas_Object *obj);

// TIZEN ONLY: for color class setting //
void                 _elm_widget_color_overlay_set(Evas_Object *obj, Evas_Object *sobj);
void                 _elm_widget_font_overlay_set(Evas_Object *obj, Evas_Object *sobj);
//

void                 _elm_unneed_ethumb(void);
void                 _elm_unneed_web(void);

void                 _elm_rescale(void);

void                 _elm_config_init(void);
void                 _elm_config_sub_init(void);
void                 _elm_config_shutdown(void);
void                 _elm_config_sub_shutdown(void);
Eina_Bool            _elm_config_save(void);
void                 _elm_config_reload(void);

void                 _elm_recache(void);

const char          *_elm_config_current_profile_get(void);
const char          *_elm_config_profile_dir_get(const char *prof,
                                                 Eina_Bool is_user);
Eina_List           *_elm_config_profiles_list(void);
void                 _elm_config_all_update(void);
void                 _elm_config_profile_set(const char *profile);
Eina_Bool            _elm_config_profile_exists(const char *profile);

void                 _elm_config_engine_set(const char *engine);

Eina_List           *_elm_config_font_overlays_list(void);
void                 _elm_config_font_overlay_set(const char *text_class,
                                                  const char *font,
                                                  Evas_Font_Size size);
void                 _elm_config_font_overlay_remove(const char *text_class);
void                 _elm_config_font_overlay_apply(void);
Eina_List           *_elm_config_text_classes_get(void);
void                 _elm_config_text_classes_free(Eina_List *l);

Eina_List           *_elm_config_color_classes_get(void);
void                 _elm_config_color_classes_free(Eina_List *l);
Eina_List           *_elm_config_color_overlays_list(void);
void                 _elm_config_color_overlay_set(const char *color_class,
                                                   int r, int g, int b, int a,
                                                   int r2, int g2, int b2, int a2,
                                                   int r3, int g3, int b3, int a3);
void                 _elm_config_color_overlay_remove(const char *color_class);
void                 _elm_config_color_overlay_apply(void);

Eina_Bool            _elm_config_access_get(void);
void                 _elm_config_access_set(Eina_Bool is_access);
Eina_Bool            _elm_config_access_password_read_enabled_get(void);
void                 _elm_config_access_password_read_enabled_set(Eina_Bool enabled);

Elm_Font_Properties *_elm_font_properties_get(Eina_Hash **font_hash,
                                              const char *font);
Eina_Hash           *_elm_font_available_hash_add(Eina_Hash *font_hash,
                                                  const char *full_name);
void                 _elm_font_available_hash_del(Eina_Hash *hash);

void                 elm_tooltip_theme(Elm_Tooltip *tt);
void                 elm_object_sub_tooltip_content_cb_set(Evas_Object *eventarea,
                                                           Evas_Object *owner,
                                                           Elm_Tooltip_Content_Cb func,
                                                           const void *data,
                                                           Evas_Smart_Cb del_cb);
void                 elm_cursor_theme(Elm_Cursor *cur);
void                 elm_object_sub_cursor_set(Evas_Object *eventarea,
                                               Evas_Object *owner,
                                               const char *cursor);

void                 elm_menu_clone(Evas_Object *from_menu,
                                    Evas_Object *to_menu,
                                    Elm_Object_Item *parent);

char                *_elm_util_mkup_to_text(const char *mkup);
char                *_elm_util_text_to_mkup(const char *text);
Eina_Bool            _elm_util_freeze_events_get(const Evas_Object *obj);


Eina_Bool            _elm_video_check(Evas_Object *video);

Eina_List           *_elm_config_color_list_get(const char *palette_name);
void                 _elm_config_color_set(const char *palette_name,
                                           int r,
                                           int g,
                                           int b,
                                           int a);
void                 _elm_config_colors_free(const char *palette_name);

EAPI void            elm_video_audio_mute_set(Evas_Object *obj, Eina_Bool mute);
EAPI Eina_Bool       elm_video_audio_mute_get(const Evas_Object *obj);

/* DEPRECATED, will be removed on next release */
void                 _elm_icon_signal_emit(Evas_Object *obj,
                                           const char *emission,
                                           const char *source);
void                 _elm_icon_signal_callback_add(Evas_Object *obj,
                                                   const char *emission,
                                                   const char *source,
                                                   Edje_Signal_Cb func_cb,
                                                   void *data);
void                *_elm_icon_signal_callback_del(Evas_Object *obj,
                                                   const char *emission,
                                                   const char *source,
                                                   Edje_Signal_Cb func_cb);
/* end of DEPRECATED */

extern char *_elm_appname;
extern Elm_Config *_elm_config;
extern const char *_elm_data_dir;
extern const char *_elm_lib_dir;
extern int _elm_log_dom;
extern Eina_List *_elm_win_list;
extern int _elm_win_deferred_free;
extern const char *_elm_preferred_engine;
extern const char *_elm_accel_preference;

#ifdef ENABLE_NLS
/* Our gettext wrapper, used to disable translation of elm if the app
 * is not translated. */
static inline const char *
_elm_dgettext(const char *string)
{
   if (EINA_UNLIKELY(_elm_config->translate == EINA_FALSE))
     {
        return string;
     }

   return dgettext(PACKAGE, string);
}

#endif

/* Used by the paste handler */
void   _elm_entry_entry_paste(Evas_Object *obj, const char *entry);

double _elm_atof(const char *s);

#endif
