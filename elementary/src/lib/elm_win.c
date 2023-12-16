#include <Elementary.h>
#include <Elementary_Cursor.h>
#include "elm_priv.h"

static const char WIN_SMART_NAME[] = "elm_win";

static const Elm_Win_Trap *trap = NULL;

extern void ecore_animator_low_power_mode_set(Eina_Bool enable);

#define TRAP(sd, name, ...)                                             \
  do                                                                    \
    {                                                                   \
       if ((!trap) || (!trap->name) ||                                  \
           ((trap->name) &&                                             \
            (trap->name(sd->trap_data, sd->base.obj, ## __VA_ARGS__)))) \
         ecore_evas_##name(sd->ee, ##__VA_ARGS__);                      \
    }                                                                   \
  while (0)

#define ELM_WIN_DATA_GET(o, sd) \
  Elm_Win_Smart_Data * sd = evas_object_smart_data_get(o)

#define ELM_WIN_DATA_GET_OR_RETURN(o, ptr)           \
  ELM_WIN_DATA_GET(o, ptr);                          \
  if (!ptr)                                          \
    {                                                \
       CRITICAL("No widget data for object %p (%s)", \
                o, evas_object_type_get(o));         \
       return;                                       \
    }

#define ELM_WIN_DATA_GET_OR_RETURN_VAL(o, ptr, val)  \
  ELM_WIN_DATA_GET(o, ptr);                          \
  if (!ptr)                                          \
    {                                                \
       CRITICAL("No widget data for object %p (%s)", \
                o, evas_object_type_get(o));         \
       return val;                                   \
    }

#define ELM_WIN_CHECK(obj)                                             \
  if (!obj || !elm_widget_type_check((obj), WIN_SMART_NAME, __func__)) \
    return

// NOTE: Tizen 2.3 only! If accel is requested, use opengl_x11
// Full accel+engine selection mechanism is not backported from upstream
#define ENGINE_GET() (_accel_is_gl() ? "opengl_x11" : (_elm_preferred_engine ? _elm_preferred_engine : (_elm_config->engine ? _elm_config->engine : "")))
#define ENGINE_COMPARE(name) (!strcmp(ENGINE_GET(), name))
#define EE_ENGINE_COMPARE(ee, name) (!strcmp(ecore_evas_engine_name_get(ee), name))

#ifdef SDB_ENABLE
typedef struct _Obj_Dump_Mode_Api Obj_Dump_Mode_Api;

struct _Obj_Dump_Mode_Api
{
   Eina_List *(*tree_create) (Evas_Object *root);
   void (*tree_free) (Eina_List *tree);
   Eina_List *(*tree_string_get) (Eina_List *tree);
   Eina_List *(*tree_string_get_for_sdb) (Eina_List *tree);
   char *(*command_for_sdb) (Eina_List *tree, char *data);
};
#endif

typedef struct _Elm_Win_Smart_Data Elm_Win_Smart_Data;

struct _Elm_Win_Smart_Data
{
   Elm_Widget_Smart_Data base; /* base widget smart data as
                                * first member obligatory, as
                                * we're inheriting from it */

   Ecore_Evas           *ee;
   Evas                 *evas;
   Evas_Object          *parent; /* parent *window* object*/
   Evas_Object          *img_obj, *frame_obj;
   Eina_List            *resize_objs; /* a window may have
                                       * *multiple* resize
                                       * objects */
#ifdef HAVE_ELEMENTARY_X
   struct
   {
      Ecore_X_Window       xwin;
      Ecore_Event_Handler *client_message_handler;
      Ecore_Event_Handler *property_handler;
#ifdef ELM_FOCUSED_UI
      // TIZEN ONLY (20130422) : For automating default focused UI.
      Ecore_Event_Handler *mouse_down_handler;
      Ecore_Event_Handler *key_down_handler;
      //
#endif
   } x;
#endif
#ifdef SDB_ENABLE
   Elm_Module *od_mod;
   Ecore_Ipc_Server *sdb_server;
   Ecore_Event_Handler *sdb_server_data_handler, *sdb_server_del_handler;
#endif

#ifdef UTILITY_MODULE_ENABLE
   Elm_Module *utility_mod;
#endif

#ifdef UI_ANALYZER_ENABLE
   Elm_Module *ui_analyzer_mod;
#endif

#ifdef HAVE_ELEMENTARY_WAYLAND
   struct
   {
      Ecore_Wl_Window *win;
   } wl;
#endif

   Ecore_Job                     *deferred_resize_job;
   Ecore_Job                     *deferred_child_eval_job;
   Ecore_Job                     *deferred_sizing_eval_job; //Tizen Only

   Elm_Win_Type                   type;
   Elm_Win_Keyboard_Mode          kbdmode;
   Elm_Win_Indicator_Mode         indmode;
   Elm_Win_Indicator_Opacity_Mode ind_o_mode;
   Elm_Win_Indicator_Type_Mode ind_t_mode;
   struct
   {
      const char  *info;
      Ecore_Timer *timer;
      int          repeat_count;
      int          shot_counter;
   } shot;
   int                            resize_location;
   int                           *autodel_clear, rot;
   struct
   {
      int x, y;
   } screen;
   struct
   {
      Ecore_Evas  *ee;
      Evas        *evas;
      Evas_Object *obj, *hot_obj;
      int          hot_x, hot_y;
   } pointer;
   struct
   {
      Evas_Object *top;

      struct
      {
         Evas_Object *target;
         Eina_Bool    visible : 1;
         Eina_Bool    handled : 1;
      } cur, prev;

      const char  *style;
      Ecore_Job   *reconf_job;

      Eina_Bool    enabled : 1;
      Eina_Bool    changed_theme : 1;
      Eina_Bool    top_animate : 1;
      Eina_Bool    geometry_changed : 1;
   } focus_highlight;
   struct
   {
      const char  *name;
      Ecore_Timer *timer;
      Eina_List   *names;
   } profile;
   struct
   {
      int          preferred_rot; // specified by app
      int         *rots;          // available rotations
      unsigned int count;         // number of elements in available rotations
      Eina_Bool    wm_supported : 1;
      Eina_Bool    use : 1;
   } wm_rot;

   Evas_Object *icon;
   const char  *title;
   const char  *icon_name;
   const char  *role;

   void *trap_data;

   double       aspect;
   int          size_base_w, size_base_h;
   int          size_step_w, size_step_h;
   int          norender;
   Eina_Bool    urgent : 1;
   Eina_Bool    modal : 1;
   Eina_Bool    demand_attention : 1;
   Eina_Bool    autodel : 1;
   Eina_Bool    constrain : 1;
   Eina_Bool    resizing : 1;
   Eina_Bool    iconified : 1;
   Eina_Bool    withdrawn : 1;
   Eina_Bool    sticky : 1;
   Eina_Bool    fullscreen : 1;
   Eina_Bool    maximized : 1;
   Eina_Bool    skip_focus : 1;
   Eina_Bool    floating : 1;
#ifdef ELM_FOCUSED_UI
   // TIZEN ONLY (20130422) : For automating default focused UI.
   Eina_Bool    keyboard_attached : 1;
   Eina_Bool    first_key_down : 1;
   //
   Eina_Bool    fixed_style : 1;
#endif
};

static const char SIG_DELETE_REQUEST[] = "delete,request";
static const char SIG_FOCUS_OUT[] = "focus,out";
static const char SIG_FOCUS_IN[] = "focus,in";
static const char SIG_MOVED[] = "moved";
static const char SIG_WITHDRAWN[] = "withdrawn";
static const char SIG_ICONIFIED[] = "iconified";
static const char SIG_NORMAL[] = "normal";
static const char SIG_STICK[] = "stick";
static const char SIG_UNSTICK[] = "unstick";
static const char SIG_FULLSCREEN[] = "fullscreen";
static const char SIG_UNFULLSCREEN[] = "unfullscreen";
static const char SIG_MAXIMIZED[] = "maximized";
static const char SIG_UNMAXIMIZED[] = "unmaximized";
static const char SIG_IOERR[] = "ioerr";
static const char SIG_INDICATOR_PROP_CHANGED[] = "indicator,prop,changed";
static const char SIG_ROTATION_CHANGED[] = "rotation,changed";
static const char SIG_INDICATOR_FLICK_DONE[] = "indicator,flick,done";
static const char SIG_QUICKPANEL_STATE_ON[] = "quickpanel,state,on";
static const char SIG_QUICKPANEL_STATE_OFF[] = "quickpanel,state,off";
static const char SIG_DESKTOP_LAYOUT[] = "desktop,layout";
static const char SIG_PROFILE_CHANGED[] = "profile,changed";
static const char SIG_WM_ROTATION_CHANGED[] = "wm,rotation,changed";
static const char SIG_AUX_HINT_ALLOWED[] = "aux,hint,allowed";
static const char SIG_ACCESS_CHANGED[] = "access,changed";
//TIZEN ONLY(20131204) : Supporting highlight change timing.
static const char SIG_HIGHLIGHT_ENABLED[] = "highlight,enabled";
static const char SIG_HIGHLIGHT_DISABLED[] = "highlight,disabled";
//

static const Evas_Smart_Cb_Description _smart_callbacks[] = {
   {SIG_DELETE_REQUEST, ""},
   {SIG_FOCUS_OUT, ""},
   {SIG_FOCUS_IN, ""},
   {SIG_MOVED, ""},
   {SIG_WITHDRAWN, ""},
   {SIG_ICONIFIED, ""},
   {SIG_NORMAL, ""},
   {SIG_STICK, ""},
   {SIG_UNSTICK, ""},
   {SIG_FULLSCREEN, ""},
   {SIG_UNFULLSCREEN, ""},
   {SIG_MAXIMIZED, ""},
   {SIG_UNMAXIMIZED, ""},
   {SIG_IOERR, ""},
   {SIG_INDICATOR_PROP_CHANGED, ""},
   {SIG_ROTATION_CHANGED, ""},
   {SIG_PROFILE_CHANGED, ""},
   {SIG_WM_ROTATION_CHANGED, ""},
   {SIG_AUX_HINT_ALLOWED, ""},
   {SIG_ACCESS_CHANGED, ""},
   {NULL, NULL}
};

EVAS_SMART_SUBCLASS_NEW
  (WIN_SMART_NAME, _elm_win, Elm_Widget_Smart_Class,
  Elm_Widget_Smart_Class, elm_widget_smart_class_get, _smart_callbacks);

Eina_List *_elm_win_list = NULL;
int _elm_win_deferred_free = 0;

static int _elm_win_count = 0;

static Eina_Bool _elm_win_auto_throttled = EINA_FALSE;

static Ecore_Job *_elm_win_state_eval_job = NULL;

static void _elm_win_resize_objects_eval(Evas_Object *obj);
#ifdef SDB_ENABLE
static void _elm_win_sdb_server_del(Elm_Win_Smart_Data *sd);
#endif

#ifdef HAVE_ELEMENTARY_X
static void _elm_win_xwin_update(Elm_Win_Smart_Data *sd);
static void _elm_win_xwin_type_set(Elm_Win_Smart_Data *sd);
#endif

static void
_elm_win_obj_intercept_show(void *data, Evas_Object *obj)
{
   Elm_Win_Smart_Data *sd = data;
   // this is called to make sure all smart containers have calculated their
   // sizes BEFORE we show the window to make sure it initially appears at
   // our desired size (ie min size is known first)
   evas_smart_objects_calculate(evas_object_evas_get(obj));
   if (sd->frame_obj)
     {
        evas_object_show(sd->frame_obj);
     }
   else if (sd->img_obj)
     {
        evas_object_show(sd->img_obj);
     }
   if (sd->pointer.obj)
     {
        ecore_evas_show(sd->pointer.ee);
        evas_object_show(sd->pointer.obj);
        /* ecore_evas_wayland_pointer_set(win->pointer.ee, 10, 10); */
     }
   evas_object_show(obj);
}

#ifdef ELM_FEATURE_POWER_SAVE
static Eina_Bool _elm_win_list_visible = EINA_FALSE;
Eina_Bool
_elm_win_list_visibility_get(void)
{
   return _elm_win_list_visible;
}
#endif

static void
_elm_win_state_eval(void *data __UNUSED__)
{
   Eina_List *l;
   Evas_Object *obj;
   int _elm_win_count_shown = 0;
   int _elm_win_count_iconified = 0;
   int _elm_win_count_withdrawn = 0;

   _elm_win_state_eval_job = NULL;

   if (_elm_config->auto_norender_withdrawn)
     {
        EINA_LIST_FOREACH(_elm_win_list, l, obj)
          {
             if ((elm_win_withdrawn_get(obj)) ||
                 ((elm_win_iconified_get(obj) &&
                   (_elm_config->auto_norender_iconified_same_as_withdrawn))))
               {
                  if (!evas_object_data_get(obj, "__win_auto_norender"))
                    {
                       Evas *evas = evas_object_evas_get(obj);

                       elm_win_norender_push(obj);
                       evas_object_data_set(obj, "__win_auto_norender", obj);

                       if (_elm_config->auto_flush_withdrawn)
                         {
                            edje_file_cache_flush();
                            edje_collection_cache_flush();
                            evas_image_cache_flush(evas);
                            evas_font_cache_flush(evas);
                         }
                       if (_elm_config->auto_dump_withdrawn)
                         {
                            evas_render_dump(evas);
                         }
                    }
               }
             else
               {
                  if (evas_object_data_get(obj, "__win_auto_norender"))
                    {
                       elm_win_norender_pop(obj);
                       evas_object_data_del(obj, "__win_auto_norender");
                    }
               }
          }
     }

   if (_elm_win_count != 0)
     {
        EINA_LIST_FOREACH(_elm_win_list, l, obj)
          {
             if (elm_win_withdrawn_get(obj))
               _elm_win_count_withdrawn++;
             else if (elm_win_iconified_get(obj))
               _elm_win_count_iconified++;
             else if (evas_object_visible_get(obj))
               _elm_win_count_shown++;
          }
     }

   if (((_elm_config->auto_throttle) &&
        (elm_policy_get(ELM_POLICY_THROTTLE) != ELM_POLICY_THROTTLE_NEVER)) ||
        (elm_policy_get(ELM_POLICY_THROTTLE) == ELM_POLICY_THROTTLE_HIDDEN_ALWAYS))
     {
        if (_elm_win_count == 0)
          {
             if (_elm_win_auto_throttled)
               {
                  ecore_throttle_adjust(-_elm_config->auto_throttle_amount);
                  _elm_win_auto_throttled = EINA_FALSE;
               }
          }
        else
          {
             if (_elm_win_count_shown <= 0)
               {
                  if (!_elm_win_auto_throttled)
                    {
                       ecore_throttle_adjust(_elm_config->auto_throttle_amount);
                       _elm_win_auto_throttled = EINA_TRUE;
                    }
               }
             else
               {
                  if (_elm_win_auto_throttled)
                    {
                       ecore_throttle_adjust(-_elm_config->auto_throttle_amount);
                       _elm_win_auto_throttled = EINA_FALSE;
                    }
               }
          }
     }

#ifdef ELM_FEATURE_POWER_SAVE
   if (_elm_power_save_enabled_get())
     {
        if (_elm_win_count_shown <= 0)
          {
             _elm_win_list_visible = EINA_FALSE;
             DBG("(anim trace) *** win list not visible ***");
             ecore_animator_low_power_mode_set(EINA_TRUE);
          }
        else
          {
             _elm_win_list_visible = EINA_TRUE;
             DBG("(anim trace) *** win list visible ***");
             if (_elm_screen_status_get() != ELM_SCREEN_STATUS_OFF)
               {
                  DBG("(anim trace) win list visible && Screen ON");
                  ecore_animator_low_power_mode_set(EINA_FALSE);
               }
          }
     }
#endif
}

static void
_elm_win_state_eval_queue(void)
{
   if (_elm_win_state_eval_job) ecore_job_del(_elm_win_state_eval_job);
   _elm_win_state_eval_job = ecore_job_add(_elm_win_state_eval, NULL);
}

// example shot spec (wait 0.1 sec then save as my-window.png):
// ELM_ENGINE="shot:delay=0.1:file=my-window.png"

static double
_shot_delay_get(Elm_Win_Smart_Data *sd)
{
   char *p, *pd;
   char *d = strdup(sd->shot.info);

   if (!d) return 0.5;
   for (p = (char *)sd->shot.info; *p; p++)
     {
        if (!strncmp(p, "delay=", 6))
          {
             double v;

             for (pd = d, p += 6; (*p) && (*p != ':'); p++, pd++)
               {
                  *pd = *p;
               }
             *pd = 0;
             v = _elm_atof(d);
             free(d);
             return v;
          }
     }
   free(d);

   return 0.5;
}

static char *
_shot_file_get(Elm_Win_Smart_Data *sd)
{
   char *p;
   char *tmp = strdup(sd->shot.info);
   char *repname = NULL;

   if (!tmp) return NULL;

   for (p = (char *)sd->shot.info; *p; p++)
     {
        if (!strncmp(p, "file=", 5))
          {
             strcpy(tmp, p + 5);
             if (!sd->shot.repeat_count) return tmp;
             else
               {
                  char *dotptr = strrchr(tmp, '.');
                  if (dotptr)
                    {
                       size_t size = sizeof(char) * (strlen(tmp) + 16);
                       repname = malloc(size);
                       strncpy(repname, tmp, dotptr - tmp);
                       snprintf(repname + (dotptr - tmp), size -
                                (dotptr - tmp), "%03i",
                                sd->shot.shot_counter + 1);
                       strcat(repname, dotptr);
                       free(tmp);
                       return repname;
                    }
               }
          }
     }
   free(tmp);
   if (!sd->shot.repeat_count) return strdup("out.png");

   repname = malloc(sizeof(char) * 24);
   snprintf(repname, sizeof(char) * 24, "out%03i.png",
            sd->shot.shot_counter + 1);

   return repname;
}

static int
_shot_repeat_count_get(Elm_Win_Smart_Data *sd)
{
   char *p, *pd;
   char *d = strdup(sd->shot.info);

   if (!d) return 0;
   for (p = (char *)sd->shot.info; *p; p++)
     {
        if (!strncmp(p, "repeat=", 7))
          {
             int v;

             for (pd = d, p += 7; (*p) && (*p != ':'); p++, pd++)
               {
                  *pd = *p;
               }
             *pd = 0;
             v = atoi(d);
             if (v < 0) v = 0;
             if (v > 1000) v = 999;
             free(d);
             return v;
          }
     }
   free(d);

   return 0;
}

static char *
_shot_key_get(Elm_Win_Smart_Data *sd __UNUSED__)
{
   return NULL;
}

static char *
_shot_flags_get(Elm_Win_Smart_Data *sd __UNUSED__)
{
   return NULL;
}

static void
_shot_do(Elm_Win_Smart_Data *sd)
{
   Ecore_Evas *ee;
   Evas_Object *o;
   unsigned int *pixels;
   int w, h;
   char *file, *key, *flags;

   ecore_evas_manual_render(sd->ee);
   pixels = (void *)ecore_evas_buffer_pixels_get(sd->ee);
   if (!pixels) return;

   ecore_evas_geometry_get(sd->ee, NULL, NULL, &w, &h);
   if ((w < 1) || (h < 1)) return;

   file = _shot_file_get(sd);
   if (!file) return;

   key = _shot_key_get(sd);
   flags = _shot_flags_get(sd);
   ee = ecore_evas_buffer_new(1, 1);
   o = evas_object_image_add(ecore_evas_get(ee));
   evas_object_image_alpha_set(o, ecore_evas_alpha_get(sd->ee));
   evas_object_image_size_set(o, w, h);
   evas_object_image_data_set(o, pixels);
   if (!evas_object_image_save(o, file, key, flags))
     {
        ERR("Cannot save window to '%s' (key '%s', flags '%s')",
            file, key, flags);
     }
   free(file);
   if (key) free(key);
   if (flags) free(flags);
   ecore_evas_free(ee);
   if (sd->shot.repeat_count) sd->shot.shot_counter++;
}

static Eina_Bool
_shot_delay(void *data)
{
   Elm_Win_Smart_Data *sd = data;

   _shot_do(sd);
   if (sd->shot.repeat_count)
     {
        int remainshot = (sd->shot.repeat_count - sd->shot.shot_counter);
        if (remainshot > 0) return EINA_TRUE;
     }
   sd->shot.timer = NULL;
   elm_exit();

   return EINA_FALSE;
}

static void
_shot_init(Elm_Win_Smart_Data *sd)
{
   if (!sd->shot.info) return;

   sd->shot.repeat_count = _shot_repeat_count_get(sd);
   sd->shot.shot_counter = 0;
}

static void
_shot_handle(Elm_Win_Smart_Data *sd)
{
   if (!sd->shot.info) return;

   if (!sd->shot.timer)
     sd->shot.timer = ecore_timer_add(_shot_delay_get(sd), _shot_delay, sd);
}

static void
_rotation_say(Evas_Object *obj, int rotation)
{
   if (!_elm_config->access_mode) return;

   if ((rotation == 90)||(rotation == 270))
     _elm_access_say(obj, E_("IDS_SCR_BODY_LANDSCAPE_MODE"), EINA_FALSE);
   else
     _elm_access_say(obj, E_("IDS_SCR_BODY_PORTRAIT_MODE_TTS"), EINA_FALSE);
}

/* elm-win specific associate, does the trap while ecore_evas_object_associate()
 * does not.
 */
static Elm_Win_Smart_Data *
_elm_win_associate_get(const Ecore_Evas *ee)
{
   return ecore_evas_data_get(ee, "elm_win");
}

/* Interceptors Callbacks */
static void
_elm_win_obj_intercept_raise(void *data, Evas_Object *obj __UNUSED__)
{
   Elm_Win_Smart_Data *sd = data;
   TRAP(sd, raise);
}

static void
_elm_win_obj_intercept_lower(void *data, Evas_Object *obj __UNUSED__)
{
   Elm_Win_Smart_Data *sd = data;
   TRAP(sd, lower);
}

static void
_elm_win_obj_intercept_stack_above(void *data __UNUSED__, Evas_Object *obj __UNUSED__, Evas_Object *above __UNUSED__)
{
   INF("TODO: %s", __FUNCTION__);
}

static void
_elm_win_obj_intercept_stack_below(void *data __UNUSED__, Evas_Object *obj __UNUSED__, Evas_Object *below __UNUSED__)
{
   INF("TODO: %s", __FUNCTION__);
}

static void
_elm_win_obj_intercept_layer_set(void *data, Evas_Object *obj __UNUSED__, int l)
{
   Elm_Win_Smart_Data *sd = data;
   TRAP(sd, layer_set, l);
}

static void
_ecore_evas_move_(void *data, Evas_Object *obj __UNUSED__, int a, int b)
{
   printf("[%s][%d] a=%d, b=%d\n", __FUNCTION__, __LINE__, a, b);
   Elm_Win_Smart_Data *sd = data;
   TRAP(sd, move, a, b);
}

/* Event Callbacks */

static void
_elm_win_obj_callback_changed_size_hints(void *data, Evas *e __UNUSED__, Evas_Object *obj, void *event_info __UNUSED__)
{
   Elm_Win_Smart_Data *sd = data;
   Evas_Coord w, h;

   evas_object_size_hint_min_get(obj, &w, &h);
   TRAP(sd, size_min_set, w, h);

   evas_object_size_hint_max_get(obj, &w, &h);
   if (w < 1) w = -1;
   if (h < 1) h = -1;
   TRAP(sd, size_max_set, w, h);
}
/* end of elm-win specific associate */


static void
_elm_win_move(Ecore_Evas *ee)
{
   Elm_Win_Smart_Data *sd = _elm_win_associate_get(ee);
   int x, y;

   EINA_SAFETY_ON_NULL_RETURN(sd);

   ecore_evas_geometry_get(ee, &x, &y, NULL, NULL);
   sd->screen.x = x;
   sd->screen.y = y;
   evas_object_smart_callback_call(ELM_WIDGET_DATA(sd)->obj, SIG_MOVED, NULL);
}

static void
_elm_win_resize_job(void *data)
{
   Elm_Win_Smart_Data *sd = data;
   const Eina_List *l;
   Evas_Object *obj;
   int w, h;

   sd->deferred_resize_job = NULL;
   ecore_evas_request_geometry_get(sd->ee, NULL, NULL, &w, &h);
   if (sd->constrain)
     {
        int sw, sh;
        ecore_evas_screen_geometry_get(sd->ee, NULL, NULL, &sw, &sh);
        w = MIN(w, sw);
        h = MIN(h, sh);
     }

   if (sd->frame_obj)
     {
        int fw, fh;

        evas_output_framespace_get(sd->evas, NULL, NULL, &fw, &fh);
        evas_object_resize(sd->frame_obj, w + fw, h + fh);
     }

   /* if (sd->img_obj) */
   /*   { */
   /*   } */

   evas_object_resize(ELM_WIDGET_DATA(sd)->obj, w, h);
   EINA_LIST_FOREACH(sd->resize_objs, l, obj)
     {
        evas_object_move(obj, 0, 0);
        evas_object_resize(obj, w, h);
     }
}

static void
_elm_win_resize(Ecore_Evas *ee)
{
   Elm_Win_Smart_Data *sd = _elm_win_associate_get(ee);
   EINA_SAFETY_ON_NULL_RETURN(sd);

   if (sd->deferred_resize_job) ecore_job_del(sd->deferred_resize_job);
   sd->deferred_resize_job = ecore_job_add(_elm_win_resize_job, sd);
}

static void
_elm_win_mouse_in(Ecore_Evas *ee)
{
   Elm_Win_Smart_Data *sd = _elm_win_associate_get(ee);
   EINA_SAFETY_ON_NULL_RETURN(sd);

   if (sd->resizing) sd->resizing = EINA_FALSE;
}

static void
_elm_win_focus_highlight_reconfigure_job_stop(Elm_Win_Smart_Data *sd)
{
   if (sd->focus_highlight.reconf_job)
     ecore_job_del(sd->focus_highlight.reconf_job);
   sd->focus_highlight.reconf_job = NULL;
}

static void
_elm_win_focus_highlight_visible_set(Elm_Win_Smart_Data *sd,
                                     Eina_Bool visible)
{
   Evas_Object *top;

   top = sd->focus_highlight.top;
   if (visible)
     {
        if (top)
          {
             DBG("[WIN_FOCUS:%p] top show", ELM_WIDGET_DATA(sd)->obj);
             evas_object_show(top);
             edje_object_signal_emit(top, "elm,action,focus,show", "elm");
          }
     }
   else
     {
        if (top)
          {
             DBG("[WIN_FOCUS:%p] top hide", ELM_WIDGET_DATA(sd)->obj);
             edje_object_signal_emit(top, "elm,action,focus,hide", "elm");
          }
     }
}

static void
_elm_win_focus_highlight_anim_setup(Elm_Win_Smart_Data *sd,
                                    Evas_Object *obj)
{
   Evas_Coord tx, ty, tw, th;
   Evas_Coord w, h, px, py, pw, ph;
   Edje_Message_Int_Set *m;
   Evas_Object *previous = sd->focus_highlight.prev.target;
   Evas_Object *target = sd->focus_highlight.cur.target;

   evas_object_geometry_get(ELM_WIDGET_DATA(sd)->obj, NULL, NULL, &w, &h);
   evas_object_geometry_get(target, &tx, &ty, &tw, &th);
   evas_object_geometry_get(previous, &px, &py, &pw, &ph);
   evas_object_move(obj, tx, ty);
   evas_object_resize(obj, tw, th);
   evas_object_clip_unset(obj);

   m = alloca(sizeof(*m) + (sizeof(int) * 8));
   m->count = 8;
   m->val[0] = px - tx;
   m->val[1] = py - ty;
   m->val[2] = pw;
   m->val[3] = ph;
   m->val[4] = 0;
   m->val[5] = 0;
   m->val[6] = tw;
   m->val[7] = th;
   edje_object_message_send(obj, EDJE_MESSAGE_INT_SET, 1, m);
}

static void
_elm_win_focus_highlight_simple_setup(Elm_Win_Smart_Data *sd,
                                      Evas_Object *obj)
{
   Evas_Object *clip, *target = sd->focus_highlight.cur.target;
   Evas_Coord x, y, w, h;

   clip = evas_object_clip_get(target);
   evas_object_geometry_get(target, &x, &y, &w, &h);

   evas_object_move(obj, x, y);
   evas_object_resize(obj, w, h);
   evas_object_clip_set(obj, clip);
}

static void
_elm_win_target_focus_highlight_set(Evas_Object *target, Eina_Bool focus_highlight)
{
   ELM_WIDGET_DATA_GET(target, tsd);
   if (tsd)
     {
        if (tsd->api->focus_highlight_set)
          {
             tsd->api->focus_highlight_set(target, focus_highlight);
          }
     }
   if (focus_highlight)
     elm_widget_signal_emit
        (target, "elm,action,focus_highlight,show", "elm");
   else
     elm_widget_signal_emit
        (target, "elm,action,focus_highlight,hide", "elm");

}

static void
_elm_win_focus_prev_target_del(void *data,
                               Evas *e EINA_UNUSED,
                               Evas_Object *obj EINA_UNUSED,
                               void *event_info EINA_UNUSED)
{
   Elm_Win_Smart_Data *sd = data;
   if (sd)
     sd->focus_highlight.prev.target = NULL;
}

static void
_elm_win_focus_highlight_reconfigure(Elm_Win_Smart_Data *sd)
{
   Evas_Object *target = sd->focus_highlight.cur.target;
   Evas_Object *previous = sd->focus_highlight.prev.target;
   Evas_Object *top = sd->focus_highlight.top;
   Eina_Bool visible_changed;
   Eina_Bool common_visible;
   const char *sig = NULL;

   _elm_win_focus_highlight_reconfigure_job_stop(sd);

   visible_changed = (sd->focus_highlight.cur.visible !=
                      sd->focus_highlight.prev.visible);

   if ((target == previous) && (!visible_changed) &&
       (!sd->focus_highlight.geometry_changed) &&
       (!sd->focus_highlight.changed_theme))
     return;

   if ((previous) && (sd->focus_highlight.prev.handled))
     {
        evas_object_event_callback_del_full
           (previous, EVAS_CALLBACK_DEL, _elm_win_focus_prev_target_del, sd);
        _elm_win_target_focus_highlight_set(previous, EINA_FALSE);
     }

   DBG("[WIN_FOCUS:%p] target(%p:%s) handled(%d) visible(%d)",
       ELM_WIDGET_DATA(sd)->obj, target, elm_widget_type_get(target),
       sd->focus_highlight.cur.handled, sd->focus_highlight.cur.visible);

   if (!target)
     common_visible = EINA_FALSE;
   else if (sd->focus_highlight.cur.handled)
     {
        common_visible = EINA_FALSE;
        if (sd->focus_highlight.cur.visible)
          sig = "elm,action,focus_highlight,show";
        else
          sig = "elm,action,focus_highlight,hide";
     }
   else
     common_visible = sd->focus_highlight.cur.visible;

   _elm_win_focus_highlight_visible_set(sd, common_visible);
   if (sig)
     {
        DBG("[WIN_FOCUS:%p] focus highlight to target(%p:%s) sig:%s",
            ELM_WIDGET_DATA(sd)->obj, target,
            elm_widget_type_get(target), sig);
        if (!strcmp(sig, "elm,action,focus_highlight,show"))
          _elm_win_target_focus_highlight_set(target, EINA_TRUE);
        else
          _elm_win_target_focus_highlight_set(target, EINA_FALSE);
     }

   if ((!target) || (!common_visible) || (sd->focus_highlight.cur.handled))
     goto the_end;

   if (sd->focus_highlight.changed_theme)
     {
        const char *str;
        if (sd->focus_highlight.style)
          str = sd->focus_highlight.style;
        else
          str = "default";
        elm_widget_theme_object_set
          (ELM_WIDGET_DATA(sd)->obj, top, "focus_highlight", "top", str);
        sd->focus_highlight.changed_theme = EINA_FALSE;
        // TIZEN ONLY (20131211) : For broadcasting the state of focus highlight.
        _elm_win_focus_highlight_visible_set(sd, common_visible);

        if (_elm_config->focus_highlight_animate)
          {
             str = edje_object_data_get(sd->focus_highlight.top, "animate");
             sd->focus_highlight.top_animate = ((str) && (!strcmp(str, "on")));
          }
     }

   if ((sd->focus_highlight.top_animate) && (previous) &&
       (!sd->focus_highlight.prev.handled))
     _elm_win_focus_highlight_anim_setup(sd, top);
   else
     _elm_win_focus_highlight_simple_setup(sd, top);
   evas_object_raise(top);

the_end:
   sd->focus_highlight.geometry_changed = EINA_FALSE;
   sd->focus_highlight.prev = sd->focus_highlight.cur;
   evas_object_event_callback_add
     (sd->focus_highlight.prev.target,
      EVAS_CALLBACK_DEL, _elm_win_focus_prev_target_del, sd);
}

static void
_elm_win_focus_highlight_reconfigure_job(void *data)
{
   _elm_win_focus_highlight_reconfigure((Elm_Win_Smart_Data *)data);
}

static void
_elm_win_focus_highlight_reconfigure_job_start(Elm_Win_Smart_Data *sd)
{
   if (sd->focus_highlight.reconf_job)
     ecore_job_del(sd->focus_highlight.reconf_job);

   sd->focus_highlight.reconf_job = ecore_job_add(
       _elm_win_focus_highlight_reconfigure_job, sd);
}

static void
_elm_win_focus_in(Ecore_Evas *ee)
{
   Elm_Win_Smart_Data *sd = _elm_win_associate_get(ee);
   Evas_Object *obj;
   unsigned int order = 0;

   EINA_SAFETY_ON_NULL_RETURN(sd);

   obj = ELM_WIDGET_DATA(sd)->obj;

   _elm_widget_top_win_focused_set(obj, EINA_TRUE);

   if (!elm_widget_focus_order_get(obj) ||
       (obj == elm_widget_newest_focus_order_get(obj, &order, EINA_TRUE)))
     {
        if (_elm_config->access_mode)
          elm_widget_focus_restore(obj);
        elm_widget_focus_steal(obj);
     }
   else
     elm_widget_focus_restore(obj);

   evas_object_smart_callback_call(obj, SIG_FOCUS_IN, NULL);
#ifdef ELM_FOCUSED_UI
   // TIZEN_ONLY (20130713) : For supporting Focused UI of TIZEN.
   if (sd->keyboard_attached)
     {
        sd->focus_highlight.cur.visible = EINA_TRUE;
        DBG("[WIN_FOCUS:%p] highlight_reconfigure_start visible(1)", obj);
        _elm_win_focus_highlight_reconfigure_job_start(sd);
     }
#endif
   if (sd->frame_obj)
     {
        edje_object_signal_emit(sd->frame_obj, "elm,action,focus", "elm");
     }
   /* do nothing */
   /* else if (sd->img_obj) */
   /*   { */
   /*   } */
}

static void
_elm_win_focus_out(Ecore_Evas *ee)
{
   Elm_Win_Smart_Data *sd = _elm_win_associate_get(ee);
   Evas_Object *obj;
   Evas_Object *access_highlight;

   EINA_SAFETY_ON_NULL_RETURN(sd);

   obj = ELM_WIDGET_DATA(sd)->obj;

   elm_object_focus_set(obj, EINA_FALSE);
   _elm_widget_top_win_focused_set(obj, EINA_FALSE);
   evas_object_smart_callback_call(obj, SIG_FOCUS_OUT, NULL);
   sd->focus_highlight.cur.visible = EINA_FALSE;
   DBG("[WIN_FOCUS:%p] highlight_reconfigure_start visible(0)", obj);
   _elm_win_focus_highlight_reconfigure_job_start(sd);
   if (sd->frame_obj)
     {
        edje_object_signal_emit(sd->frame_obj, "elm,action,unfocus", "elm");
     }

   /* access */
   if (_elm_config->access_mode)
     {
        access_highlight = evas_object_name_find(evas_object_evas_get(obj),
                                                 "_elm_access_disp");
        if (access_highlight) evas_object_hide(access_highlight);
     }

   /* do nothing */
   /* if (sd->img_obj) */
   /*   { */
   /*   } */
#ifdef SDB_ENABLE
   if (sd->sdb_server)
     _elm_win_sdb_server_del(sd);
#endif
}

static void
_elm_win_profile_update(Ecore_Evas *ee)
{
   Elm_Win_Smart_Data *sd = _elm_win_associate_get(ee);
   Evas_Object *obj;

   EINA_SAFETY_ON_NULL_RETURN(sd);

   obj = ELM_WIDGET_DATA(sd)->obj;
   if (!obj) return;

   if (sd->profile.timer)
     {
        ecore_timer_del(sd->profile.timer);
        sd->profile.timer = NULL;
     }

   /* It should be replaced per-window ELM profile later. */
   _elm_config_profile_set(sd->profile.name);

   evas_object_smart_callback_call(obj, SIG_PROFILE_CHANGED, NULL);
}

static Eina_Bool
_elm_win_profile_change_delay(void *data)
{
   Elm_Win_Smart_Data *sd = data;
   const char *profile;
   Eina_Bool changed = EINA_FALSE;

   profile = eina_list_nth(sd->profile.names, 0);
   if (profile)
     {
        if (sd->profile.name)
          {
             if (strcmp(sd->profile.name, profile))
               {
                  eina_stringshare_replace(&(sd->profile.name), profile);
                  changed = EINA_TRUE;
               }
          }
        else
          {
             sd->profile.name = eina_stringshare_add(profile);
             changed = EINA_TRUE;
          }
     }
   sd->profile.timer = NULL;
   if (changed) _elm_win_profile_update(sd->ee);
   return EINA_FALSE;
}

static void
_elm_win_sizing_eval_job(void *data)
{
   ELM_WIN_DATA_GET(data, sd);
   sd->deferred_sizing_eval_job = NULL;
   _elm_win_resize_objects_eval(data);
 }

static void
_elm_win_state_change(Ecore_Evas *ee)
{
   Elm_Win_Smart_Data *sd = _elm_win_associate_get(ee);
   Evas_Object *obj;
   Eina_Bool ch_withdrawn = EINA_FALSE;
   Eina_Bool ch_sticky = EINA_FALSE;
   Eina_Bool ch_iconified = EINA_FALSE;
   Eina_Bool ch_fullscreen = EINA_FALSE;
   Eina_Bool ch_maximized = EINA_FALSE;
   Eina_Bool ch_profile = EINA_FALSE;
   Eina_Bool ch_wm_rotation = EINA_FALSE;
   Eina_Bool ch_aux_hint = EINA_FALSE;
   const char *profile;
   Eina_List *aux_hints = NULL;

   EINA_SAFETY_ON_NULL_RETURN(sd);

   obj = ELM_WIDGET_DATA(sd)->obj;

   if (sd->withdrawn != ecore_evas_withdrawn_get(sd->ee))
     {
        sd->withdrawn = ecore_evas_withdrawn_get(sd->ee);
        ch_withdrawn = EINA_TRUE;
     }
   if (sd->sticky != ecore_evas_sticky_get(sd->ee))
     {
        sd->sticky = ecore_evas_sticky_get(sd->ee);
        ch_sticky = EINA_TRUE;
     }
   if (sd->iconified != ecore_evas_iconified_get(sd->ee))
     {
        sd->iconified = ecore_evas_iconified_get(sd->ee);
        ch_iconified = EINA_TRUE;
     }
   if (sd->fullscreen != ecore_evas_fullscreen_get(sd->ee))
     {
        sd->fullscreen = ecore_evas_fullscreen_get(sd->ee);
        ch_fullscreen = EINA_TRUE;
     }
   if (sd->maximized != ecore_evas_maximized_get(sd->ee))
     {
        sd->maximized = ecore_evas_maximized_get(sd->ee);
        ch_maximized = EINA_TRUE;
     }
   profile = ecore_evas_profile_get(sd->ee);
   if ((profile) &&
       _elm_config_profile_exists(profile))
     {
        if (sd->profile.name)
          {
             if (strcmp(sd->profile.name, profile))
               {
                  eina_stringshare_replace(&(sd->profile.name), profile);
                  ch_profile = EINA_TRUE;
               }
          }
        else
          {
             sd->profile.name = eina_stringshare_add(profile);
             ch_profile = EINA_TRUE;
          }
     }
   if (sd->wm_rot.use)
     {
        if (sd->rot != ecore_evas_rotation_get(sd->ee))
          {
             sd->rot = ecore_evas_rotation_get(sd->ee);
             ch_wm_rotation = EINA_TRUE;
          }
     }
   aux_hints = ecore_evas_aux_hints_allowed_get(sd->ee);
   if (aux_hints)
     {
        ch_aux_hint = EINA_TRUE;
     }

   _elm_win_state_eval_queue();

   if ((ch_withdrawn) || (ch_iconified))
     {
        if (sd->withdrawn)
          evas_object_smart_callback_call(obj, SIG_WITHDRAWN, NULL);
        else if (sd->iconified)
          evas_object_smart_callback_call(obj, SIG_ICONIFIED, NULL);
        else
          evas_object_smart_callback_call(obj, SIG_NORMAL, NULL);
     }
   if (ch_sticky)
     {
        if (sd->sticky)
          evas_object_smart_callback_call(obj, SIG_STICK, NULL);
        else
          evas_object_smart_callback_call(obj, SIG_UNSTICK, NULL);
     }
   if (ch_fullscreen)
     {
        if (sd->fullscreen)
          evas_object_smart_callback_call(obj, SIG_FULLSCREEN, NULL);
        else
          evas_object_smart_callback_call(obj, SIG_UNFULLSCREEN, NULL);
     }
   if (ch_maximized)
     {
        if (sd->maximized)
          evas_object_smart_callback_call(obj, SIG_MAXIMIZED, NULL);
        else
          evas_object_smart_callback_call(obj, SIG_UNMAXIMIZED, NULL);
     }
   if (ch_profile)
     {
        _elm_win_profile_update(ee);
     }
   if (ch_wm_rotation)
     {
        evas_object_size_hint_min_set(obj, -1, -1);
        evas_object_size_hint_max_set(obj, -1, -1);
        if (sd->deferred_sizing_eval_job)
          ecore_job_del(sd->deferred_sizing_eval_job);
        sd->deferred_sizing_eval_job =
           ecore_job_add(_elm_win_sizing_eval_job, obj);
#ifdef HAVE_ELEMENTARY_X
        _elm_win_xwin_update(sd);
#endif
        _rotation_say(obj, sd->rot);
        elm_widget_orientation_set(obj, sd->rot);
        evas_object_smart_callback_call(obj, SIG_ROTATION_CHANGED, NULL);
        evas_object_smart_callback_call(obj, SIG_WM_ROTATION_CHANGED, NULL);
     }
   if (ch_aux_hint)
     {
        void *id;
        Eina_List *l;
        EINA_LIST_FOREACH(aux_hints, l, id)
          evas_object_smart_callback_call(obj, SIG_AUX_HINT_ALLOWED, id);
        eina_list_free(aux_hints);
     }
}

static Eina_Bool
_elm_win_smart_focus_next(const Evas_Object *obj,
                          Elm_Focus_Direction dir,
                          Evas_Object **next)
{
   Eina_Bool ret;
   ELM_WIN_DATA_GET(obj, sd);

   const Eina_List *items;
   void *(*list_data_get)(const Eina_List *list);

   /* Focus chain */
   if (ELM_WIDGET_DATA(sd)->subobjs)
     {
        if (!(items = elm_widget_focus_custom_chain_get(obj)))
          {
             items = ELM_WIDGET_DATA(sd)->subobjs;
             if (!items)
               return EINA_FALSE;
          }
        list_data_get = eina_list_data_get;

        ret = elm_widget_focus_list_next_get(obj, items, list_data_get, dir, next);

        /* the result is used on the auto read mode. it would meet end of the
           widget tree if it return EINA_FALSE. stop the auto read if ret is EINA_FALSE */
        if (_elm_config->access_mode && *next)
          return ret;

        if (*next)
          return EINA_TRUE;
     }
   *next = (Evas_Object *)obj;
   return EINA_FALSE;
}

static Eina_Bool
_elm_win_smart_focus_direction_manager_is(const Evas_Object *obj __UNUSED__)
{
   return EINA_TRUE;
}

static Eina_Bool
_elm_win_smart_focus_direction(const Evas_Object *obj,
                               const Evas_Object *base,
                               double degree,
                               Evas_Object **direction,
                               double *weight)
{
   const Eina_List *items;
   void *(*list_data_get)(const Eina_List *list);

   ELM_WIN_DATA_GET(obj, sd);

   /* Focus chain */
   if (ELM_WIDGET_DATA(sd)->subobjs)
     {
        if (!(items = elm_widget_focus_custom_chain_get(obj)))
          items = ELM_WIDGET_DATA(sd)->subobjs;

        list_data_get = eina_list_data_get;

        return elm_widget_focus_list_direction_get
                 (obj, base, items, list_data_get, degree, direction, weight);
     }

   return EINA_FALSE;
}

static Eina_Bool
_elm_win_smart_on_focus(Evas_Object *obj, Elm_Focus_Info *info __UNUSED__)
{
   ELM_WIN_DATA_GET(obj, sd);

   if (sd->img_obj)
     evas_object_focus_set(sd->img_obj, elm_widget_focus_get(obj));
   else
     evas_object_focus_set(obj, elm_widget_focus_get(obj));

   return EINA_TRUE;
}

static Eina_Bool
_elm_win_smart_event(Evas_Object *obj,
                     Evas_Object *src __UNUSED__,
                     Evas_Callback_Type type,
                     void *event_info __UNUSED__)
{
   Evas_Event_Key_Down *ev = event_info;

   if (elm_widget_disabled_get(obj)) return EINA_FALSE;

   if (type != EVAS_CALLBACK_KEY_DOWN)
     return EINA_FALSE;

#ifdef ELM_FOCUSED_UI
   // TIZEN_ONLY (20130713) : For supporting Focused UI of TIZEN.
   if (!_elm_config->access_mode &&
       !elm_win_focus_highlight_enabled_get(obj))
     return EINA_FALSE;
   ////

   // TIZEN ONLY(20131221) : When access mode, focused ui is disabled.
   if ((!strcmp(ev->keyname, "Return")) ||
       ((!strcmp(ev->keyname, "KP_Enter")) && !ev->string))
     {
        if (_elm_config->access_mode)
          {
             _elm_access_highlight_object_activate(obj, ELM_ACTIVATE_DEFAULT);
             goto success;
          }
     }
   else if ((!strcmp(ev->keyname, "Tab")) ||
       (!strcmp(ev->keyname, "ISO_Left_Tab")))
     {
        // TIZEN ONLY(20131221) : When access mode, focused ui is disabled.
        if (!_elm_config->access_mode)
          {
             if (evas_key_modifier_is_set(ev->modifiers, "Control") ||
                 evas_key_modifier_is_set(ev->modifiers, "Alt"))
               return EINA_FALSE;
             if (evas_key_modifier_is_set(ev->modifiers, "Shift"))
               elm_widget_focus_cycle(obj, ELM_FOCUS_PREVIOUS);
             else
               elm_widget_focus_cycle(obj, ELM_FOCUS_NEXT);

             goto success;
          }
     }
   else if ((!strcmp(ev->keyname, "Left")) ||
            ((!strcmp(ev->keyname, "KP_Left")) && (!ev->string)))
     {
        // TIZEN ONLY(20131221) : When access mode, focused ui is disabled.
        if (_elm_config->access_mode)
          _elm_access_highlight_cycle(obj, ELM_ACCESS_ACTION_HIGHLIGHT_PREV);
        else
          elm_widget_focus_cycle(obj, ELM_FOCUS_LEFT);

        goto success;
     }
   else if ((!strcmp(ev->keyname, "Right")) ||
            ((!strcmp(ev->keyname, "KP_Right")) && (!ev->string)))
     {
        // TIZEN ONLY(20131221) : When access mode, focused ui is disabled.
        if (_elm_config->access_mode)
          _elm_access_highlight_cycle(obj, ELM_ACCESS_ACTION_HIGHLIGHT_NEXT);
        else
          elm_widget_focus_cycle(obj, ELM_FOCUS_RIGHT);

        goto success;
     }
   else if ((!strcmp(ev->keyname, "Up")) ||
            ((!strcmp(ev->keyname, "KP_Up")) && (!ev->string)))
     {
        // TIZEN ONLY(20131221) : When access mode, focused ui is disabled.
        if (_elm_config->access_mode)
          _elm_access_highlight_object_activate(obj, ELM_ACTIVATE_UP);
        else
          elm_widget_focus_cycle(obj, ELM_FOCUS_UP);

        goto success;
     }
   else if ((!strcmp(ev->keyname, "Down")) ||
            ((!strcmp(ev->keyname, "KP_Down")) && (!ev->string)))
     {
        // TIZEN ONLY(20131221) : When access mode, focused ui is disabled.
        if (_elm_config->access_mode)
          _elm_access_highlight_object_activate(obj, ELM_ACTIVATE_DOWN);
        else
          elm_widget_focus_cycle(obj, ELM_FOCUS_DOWN);

        goto success;
     }
#endif

   return EINA_FALSE;

#ifdef ELM_FOCUSED_UI
success:
   ev->event_flags |= EVAS_EVENT_FLAG_ON_HOLD;
   return EINA_TRUE;
#endif
}

static void
_deferred_ecore_evas_free(void *data)
{
   Eina_List *l;
   Ecore_Evas *ee;
   Ecore_Evas *ee_data = (Ecore_Evas*)data;
   EINA_LIST_FOREACH(ecore_evas_ecore_evas_list_get(), l, ee)
   {
      if( ee == ee_data )
         ecore_evas_free(ee_data);
   }
   _elm_win_deferred_free--;
}



static void
_elm_win_smart_show(Evas_Object *obj)
{
   ELM_WIN_DATA_GET(obj, sd);

   if (!evas_object_visible_get(obj))
     _elm_win_state_eval_queue();
   _elm_win_parent_sc->base.show(obj);

   TRAP(sd, show);

   if (sd->shot.info) _shot_handle(sd);
}

static void
_elm_win_smart_hide(Evas_Object *obj)
{
   ELM_WIN_DATA_GET(obj, sd);

   if (evas_object_visible_get(obj))
     _elm_win_state_eval_queue();
   _elm_win_parent_sc->base.hide(obj);

   TRAP(sd, hide);

   if (sd->frame_obj)
     {
        evas_object_hide(sd->frame_obj);
     }
   if (sd->img_obj)
     {
        evas_object_hide(sd->img_obj);
     }
   if (sd->pointer.obj)
     {
        evas_object_hide(sd->pointer.obj);
        ecore_evas_hide(sd->pointer.ee);
     }
}

static void
_elm_win_on_parent_del(void *data,
                       Evas *e __UNUSED__,
                       Evas_Object *obj,
                       void *event_info __UNUSED__)
{
   Elm_Win_Smart_Data *sd = data;

   if (obj == sd->parent) sd->parent = NULL;
}

static void
_elm_win_focus_target_move(void *data,
                           Evas *e __UNUSED__,
                           Evas_Object *obj __UNUSED__,
                           void *event_info __UNUSED__)
{
   Elm_Win_Smart_Data *sd = data;

   sd->focus_highlight.geometry_changed = EINA_TRUE;
   DBG("[WIN_FOCUS:%p] highlight_reconfigure_start geometry_changed",
       ELM_WIDGET_DATA(sd)->obj);
   _elm_win_focus_highlight_reconfigure_job_start(sd);
}

static void
_elm_win_focus_target_resize(void *data,
                             Evas *e __UNUSED__,
                             Evas_Object *obj __UNUSED__,
                             void *event_info __UNUSED__)
{
   Elm_Win_Smart_Data *sd = data;

   sd->focus_highlight.geometry_changed = EINA_TRUE;
   DBG("[WIN_FOCUS:%p] highlight_reconfigure_start geometry_changed",
       ELM_WIDGET_DATA(sd)->obj);
   _elm_win_focus_highlight_reconfigure_job_start(sd);
}

static void
_elm_win_focus_target_del(void *data,
                          Evas *e __UNUSED__,
                          Evas_Object *obj __UNUSED__,
                          void *event_info __UNUSED__)
{
   Elm_Win_Smart_Data *sd = data;

   DBG("[WIN_FOCUS:%p] highlight_reconfigure_start target_del",
       ELM_WIDGET_DATA(sd)->obj);
   sd->focus_highlight.cur.target = NULL;
   _elm_win_focus_highlight_reconfigure_job_start(sd);
}

static Evas_Object *
_elm_win_focus_target_get(Evas_Object *obj)
{
   Evas_Object *o = obj;

   do
     {
        if (elm_widget_is(o))
          {
             if (!elm_widget_highlight_ignore_get(o))
               break;
             o = elm_widget_parent_get(o);
             if (!o)
               o = evas_object_smart_parent_get(o);
          }
        else
          {
             o = elm_widget_parent_widget_get(o);
             if (!o)
               o = evas_object_smart_parent_get(o);
          }
     }
   while (o);

   return o;
}

static void
_elm_win_focus_target_callbacks_add(Elm_Win_Smart_Data *sd)
{
   Evas_Object *obj = sd->focus_highlight.cur.target;

   evas_object_event_callback_add
     (obj, EVAS_CALLBACK_MOVE, _elm_win_focus_target_move, sd);
   evas_object_event_callback_add
     (obj, EVAS_CALLBACK_RESIZE, _elm_win_focus_target_resize, sd);
}

static void
_elm_win_focus_target_callbacks_del(Elm_Win_Smart_Data *sd)
{
   Evas_Object *obj = sd->focus_highlight.cur.target;

   evas_object_event_callback_del_full
     (obj, EVAS_CALLBACK_MOVE, _elm_win_focus_target_move, sd);
   evas_object_event_callback_del_full
     (obj, EVAS_CALLBACK_RESIZE, _elm_win_focus_target_resize, sd);
}

static void
_elm_win_object_focus_in(void *data,
                         Evas *e __UNUSED__,
                         void *event_info)
{
   Evas_Object *obj = event_info, *target;
   Elm_Win_Smart_Data *sd = data;

   if (sd->focus_highlight.cur.target == obj)
     return;

   target = _elm_win_focus_target_get(obj);
   sd->focus_highlight.cur.target = target;
   if (elm_widget_highlight_in_theme_get(target))
     sd->focus_highlight.cur.handled = EINA_TRUE;
   else
     _elm_win_focus_target_callbacks_add(sd);

   evas_object_event_callback_add
     (target, EVAS_CALLBACK_DEL, _elm_win_focus_target_del, sd);

   DBG("[WIN_FOCUS:%p] highlight_reconfigure_start obj focus_in",
       ELM_WIDGET_DATA(sd)->obj);
   _elm_win_focus_highlight_reconfigure_job_start(sd);
}

static void
_elm_win_object_focus_out(void *data,
                          Evas *e __UNUSED__,
                          void *event_info __UNUSED__)
{
   Elm_Win_Smart_Data *sd = data;

   if (!sd->focus_highlight.cur.target)
     return;

   if (!sd->focus_highlight.cur.handled)
     _elm_win_focus_target_callbacks_del(sd);

   evas_object_event_callback_del_full
     (sd->focus_highlight.cur.target, EVAS_CALLBACK_DEL,
      _elm_win_focus_target_del, sd);

   sd->focus_highlight.cur.target = NULL;
   sd->focus_highlight.cur.handled = EINA_FALSE;

   DBG("[WIN_FOCUS:%p] highlight_reconfigure_start obj focus_out",
       ELM_WIDGET_DATA(sd)->obj);
   _elm_win_focus_highlight_reconfigure_job_start(sd);
}

static void
_elm_win_focus_highlight_shutdown(Elm_Win_Smart_Data *sd)
{
   _elm_win_focus_highlight_reconfigure_job_stop(sd);
   if (sd->focus_highlight.cur.target)
     {
        _elm_win_target_focus_highlight_set(sd->focus_highlight.cur.target, EINA_FALSE);
        _elm_win_focus_target_callbacks_del(sd);
        evas_object_event_callback_del_full
           (sd->focus_highlight.cur.target, EVAS_CALLBACK_DEL,
            _elm_win_focus_target_del, sd);
        sd->focus_highlight.cur.target = NULL;
     }
   if (sd->focus_highlight.top)
     {
        evas_object_del(sd->focus_highlight.top);
        sd->focus_highlight.top = NULL;
     }

   evas_event_callback_del_full
     (sd->evas, EVAS_CALLBACK_CANVAS_OBJECT_FOCUS_IN,
     _elm_win_object_focus_in, sd);
   evas_event_callback_del_full
     (sd->evas, EVAS_CALLBACK_CANVAS_OBJECT_FOCUS_OUT,
     _elm_win_object_focus_out, sd);
}

static void
_elm_win_on_img_obj_del(void *data,
                        Evas *e __UNUSED__,
                        Evas_Object *obj __UNUSED__,
                        void *event_info __UNUSED__)
{
   Elm_Win_Smart_Data *sd = data;
   sd->img_obj = NULL;
}

static void
_elm_win_resize_objects_eval(Evas_Object *obj)
{
   const Eina_List *l;
   const Evas_Object *child;

   ELM_WIN_DATA_GET(obj, sd);
   Evas_Coord w, h, minw = -1, minh = -1, maxw = -1, maxh = -1;
   int xx = 1, xy = 1;
   double wx, wy;

   EINA_LIST_FOREACH(sd->resize_objs, l, child)
     {
        evas_object_size_hint_weight_get(child, &wx, &wy);
        if (wx == 0.0) xx = 0;
        if (wy == 0.0) xy = 0;

        evas_object_size_hint_min_get(child, &w, &h);
        if (w < 1) w = 1;
        if (h < 1) h = 1;
        if (w > minw) minw = w;
        if (h > minh) minh = h;

        evas_object_size_hint_max_get(child, &w, &h);
        if (w < 1) w = -1;
        if (h < 1) h = -1;
        if (maxw == -1) maxw = w;
        else if ((w > 0) && (w < maxw))
          maxw = w;
        if (maxh == -1) maxh = h;
        else if ((h > 0) && (h < maxh))
          maxh = h;
     }
   if (!xx) maxw = minw;
   else maxw = 32767;
   if (!xy) maxh = minh;
   else maxh = 32767;
   evas_object_size_hint_min_set(obj, minw, minh);
   evas_object_size_hint_max_set(obj, maxw, maxh);
   evas_object_geometry_get(obj, NULL, NULL, &w, &h);
   if (w < minw) w = minw;
   if (h < minh) h = minh;
   if ((maxw >= 0) && (w > maxw)) w = maxw;
   if ((maxh >= 0) && (h > maxh)) h = maxh;
   evas_object_resize(obj, w, h);
}

static void
_elm_win_on_resize_obj_del(void *data,
                           Evas *e __UNUSED__,
                           Evas_Object *obj __UNUSED__,
                           void *event_info __UNUSED__)
{
   ELM_WIN_DATA_GET(data, sd);

   sd->resize_objs = eina_list_remove(sd->resize_objs, obj);

   _elm_win_resize_objects_eval(data);
}

static void
_elm_win_on_resize_obj_changed_size_hints(void *data,
                                          Evas *e __UNUSED__,
                                          Evas_Object *obj __UNUSED__,
                                          void *event_info __UNUSED__)
{
   _elm_win_resize_objects_eval(data);
}

static void
_elm_win_smart_del(Evas_Object *obj)
{
   const char *str;
   const Eina_List *l;
   Evas_Object *child = NULL;

   ELM_WIN_DATA_GET(obj, sd);

   /* NB: child deletion handled by parent's smart del */

   if ((trap) && (trap->del))
     trap->del(sd->trap_data, obj);

   if (sd->parent)
     {
        evas_object_event_callback_del_full
          (sd->parent, EVAS_CALLBACK_DEL, _elm_win_on_parent_del, sd);
        sd->parent = NULL;
     }

   EINA_LIST_FOREACH(sd->resize_objs, l, child)
     {
        evas_object_event_callback_del_full
           (child, EVAS_CALLBACK_CHANGED_SIZE_HINTS,
            _elm_win_on_resize_obj_changed_size_hints, obj);
        evas_object_event_callback_del_full
           (child, EVAS_CALLBACK_DEL, _elm_win_on_resize_obj_del, obj);
     }

   if (sd->autodel_clear) *(sd->autodel_clear) = -1;

   _elm_win_list = eina_list_remove(_elm_win_list, obj);
   _elm_win_count--;
   _elm_win_state_eval_queue();

   if (sd->ee)
     {
        ecore_evas_callback_delete_request_set(sd->ee, NULL);
        ecore_evas_callback_resize_set(sd->ee, NULL);
     }
   if (sd->deferred_sizing_eval_job)
     ecore_job_del(sd->deferred_sizing_eval_job);
   if (sd->deferred_resize_job) ecore_job_del(sd->deferred_resize_job);
   if (sd->deferred_child_eval_job) ecore_job_del(sd->deferred_child_eval_job);
   if (sd->shot.info) eina_stringshare_del(sd->shot.info);
   if (sd->shot.timer) ecore_timer_del(sd->shot.timer);

#ifdef HAVE_ELEMENTARY_X
   if (sd->x.client_message_handler)
     ecore_event_handler_del(sd->x.client_message_handler);
   if (sd->x.property_handler)
     ecore_event_handler_del(sd->x.property_handler);
#ifdef ELM_FOCUSED_UI
   // TIZEN ONLY (20130422) : For automating default focused UI.
   if (sd->x.mouse_down_handler)
     ecore_event_handler_del(sd->x.mouse_down_handler);
   if (sd->x.key_down_handler)
     ecore_event_handler_del(sd->x.key_down_handler);
   //
#endif
#endif
#ifdef SDB_ENABLE
   if (sd->sdb_server)
     _elm_win_sdb_server_del(sd);
#endif

#ifdef UTILITY_MODULE_ENABLE
   if (sd->utility_mod && sd->utility_mod->shutdown_func)
      sd->utility_mod->shutdown_func(sd->utility_mod);
#endif

#ifdef UI_ANALYZER_ENABLE
   if (sd->ui_analyzer_mod && sd->ui_analyzer_mod->shutdown_func)
      sd->ui_analyzer_mod->shutdown_func(sd->ui_analyzer_mod);
#endif

   if (sd->img_obj)
     {
        evas_object_event_callback_del_full
           (sd->img_obj, EVAS_CALLBACK_DEL, _elm_win_on_img_obj_del, sd);
        sd->img_obj = NULL;
     }
   else
     {
        if (sd->ee)
          {
             ecore_job_add(_deferred_ecore_evas_free, sd->ee);
             _elm_win_deferred_free++;
          }
     }

   _elm_win_focus_highlight_shutdown(sd);
   eina_stringshare_del(sd->focus_highlight.style);

   if (sd->title) eina_stringshare_del(sd->title);
   if (sd->icon_name) eina_stringshare_del(sd->icon_name);
   if (sd->role) eina_stringshare_del(sd->role);
   if (sd->icon) evas_object_del(sd->icon);

   EINA_LIST_FREE(sd->profile.names, str) eina_stringshare_del(str);
   if (sd->profile.name) eina_stringshare_del(sd->profile.name);
   if (sd->profile.timer) ecore_timer_del(sd->profile.timer);

   if (sd->wm_rot.rots) free(sd->wm_rot.rots);
   sd->wm_rot.rots = NULL;

   /* Don't let callback in the air that point to sd */
   ecore_evas_callback_delete_request_set(sd->ee, NULL);
   ecore_evas_callback_resize_set(sd->ee, NULL);
   ecore_evas_callback_mouse_in_set(sd->ee, NULL);
   ecore_evas_callback_focus_in_set(sd->ee, NULL);
   ecore_evas_callback_focus_out_set(sd->ee, NULL);
   ecore_evas_callback_move_set(sd->ee, NULL);
   ecore_evas_callback_state_change_set(sd->ee, NULL);

   _elm_win_parent_sc->base.del(obj); /* handles freeing sd */

   if ((!_elm_win_list) &&
       (elm_policy_get(ELM_POLICY_QUIT) == ELM_POLICY_QUIT_LAST_WINDOW_CLOSED))
     {
        edje_file_cache_flush();
        edje_collection_cache_flush();
        evas_image_cache_flush(evas_object_evas_get(obj));
        evas_font_cache_flush(evas_object_evas_get(obj));
        elm_exit();
     }
}


static void
_elm_win_smart_move(Evas_Object *obj,
                    Evas_Coord x,
                    Evas_Coord y)
{
   ELM_WIN_DATA_GET(obj, sd);

   if (sd->img_obj)
     {
        if ((x != sd->screen.x) || (y != sd->screen.y))
          {
             sd->screen.x = x;
             sd->screen.y = y;
             evas_object_smart_callback_call(obj, SIG_MOVED, NULL);
          }
        return;
     }
   else
     {
        TRAP(sd, move, x, y);
        if (!ecore_evas_override_get(sd->ee))  return;
     }

   _elm_win_parent_sc->base.move(obj, x, y);

   if (ecore_evas_override_get(sd->ee))
     {
        sd->screen.x = x;
        sd->screen.y = y;
        evas_object_smart_callback_call(obj, SIG_MOVED, NULL);
     }
   if (sd->frame_obj)
     {
        /* FIXME: We should update ecore_wl_window_location here !! */
        sd->screen.x = x;
        sd->screen.y = y;
     }
   if (sd->img_obj)
     {
        sd->screen.x = x;
        sd->screen.y = y;
     }
}

static void
_elm_win_smart_resize(Evas_Object *obj,
                      Evas_Coord w,
                      Evas_Coord h)
{
   ELM_WIN_DATA_GET(obj, sd);

   _elm_win_parent_sc->base.resize(obj, w, h);

   if (sd->img_obj)
     {
        if (sd->constrain)
          {
             int sw, sh;

             ecore_evas_screen_geometry_get(sd->ee, NULL, NULL, &sw, &sh);
             w = MIN(w, sw);
             h = MIN(h, sh);
          }
        if (w < 1) w = 1;
        if (h < 1) h = 1;

        evas_object_image_size_set(sd->img_obj, w, h);
     }

   TRAP(sd, resize, w, h);
}

static void
_elm_win_delete_request(Ecore_Evas *ee)
{
   Elm_Win_Smart_Data *sd = _elm_win_associate_get(ee);
   Evas_Object *obj;

   EINA_SAFETY_ON_NULL_RETURN(sd);

   obj = ELM_WIDGET_DATA(sd)->obj;

   int autodel = sd->autodel;
   sd->autodel_clear = &autodel;
   evas_object_ref(obj);
   evas_object_smart_callback_call(obj, SIG_DELETE_REQUEST, NULL);
   // FIXME: if above callback deletes - then the below will be invalid
   if (autodel) evas_object_del(obj);
   else sd->autodel_clear = NULL;
   evas_object_unref(obj);
}

Ecore_X_Window
_elm_ee_xwin_get(const Ecore_Evas *ee)
{
#ifdef HAVE_ELEMENTARY_X
   Ecore_X_Window xwin = 0;

   if (!ee) return 0;
   if (EE_ENGINE_COMPARE(ee, ELM_SOFTWARE_X11))
     {
        if (ee) xwin = ecore_evas_software_x11_window_get(ee);
     }
   else if (EE_ENGINE_COMPARE(ee, ELM_SOFTWARE_FB) ||
            EE_ENGINE_COMPARE(ee, ELM_SOFTWARE_16_WINCE) ||
            EE_ENGINE_COMPARE(ee, ELM_SOFTWARE_SDL) ||
            EE_ENGINE_COMPARE(ee, ELM_SOFTWARE_16_SDL) ||
            EE_ENGINE_COMPARE(ee, ELM_OPENGL_SDL) ||
            EE_ENGINE_COMPARE(ee, ELM_OPENGL_COCOA))
     {
     }
   else if (EE_ENGINE_COMPARE(ee, ELM_SOFTWARE_16_X11))
     {
        if (ee) xwin = ecore_evas_software_x11_16_window_get(ee);
     }
   else if (EE_ENGINE_COMPARE(ee, ELM_SOFTWARE_8_X11))
     {
        if (ee) xwin = ecore_evas_software_x11_8_window_get(ee);
     }
   else if (EE_ENGINE_COMPARE(ee, ELM_OPENGL_X11))
     {
        if (ee) xwin = ecore_evas_gl_x11_window_get(ee);
     }
   else if (EE_ENGINE_COMPARE(ee, ELM_SOFTWARE_WIN32))
     {
        if (ee) xwin = (long)ecore_evas_win32_window_get(ee);
     }
   return xwin;

#endif
   return 0;
}

#ifdef HAVE_ELEMENTARY_X
static void
_elm_win_xwindow_get(Elm_Win_Smart_Data *sd)
{
   sd->x.xwin = _elm_ee_xwin_get(sd->ee);
}

#endif

#ifdef HAVE_ELEMENTARY_X
static void
_elm_win_xwin_update(Elm_Win_Smart_Data *sd)
{
   const char *s;

   _elm_win_xwindow_get(sd);
   if (sd->parent)
     {
        ELM_WIN_DATA_GET(sd->parent, sdp);
        if (sdp)
          {
             if (sd->x.xwin)
               ecore_x_icccm_transient_for_set(sd->x.xwin, sdp->x.xwin);
          }
     }

   if (!sd->x.xwin) return;  /* nothing more to do */

   s = sd->title;
   if (!s) s = _elm_appname;
   if (!s) s = "";
   if (sd->icon_name) s = sd->icon_name;
   ecore_x_icccm_icon_name_set(sd->x.xwin, s);
   ecore_x_netwm_icon_name_set(sd->x.xwin, s);

   s = sd->role;
   if (s) ecore_x_icccm_window_role_set(sd->x.xwin, s);

   // set window icon
   if (sd->icon)
     {
        void *data;

        data = evas_object_image_data_get(sd->icon, EINA_FALSE);
        if (data)
          {
             Ecore_X_Icon ic;
             int w = 0, h = 0, stride, x, y;
             unsigned char *p;
             unsigned int *p2;

             evas_object_image_size_get(sd->icon, &w, &h);
             stride = evas_object_image_stride_get(sd->icon);
             if ((w > 0) && (h > 0) &&
                 (stride >= (int)(w * sizeof(unsigned int))))
               {
                  ic.width = w;
                  ic.height = h;
                  ic.data = malloc(w * h * sizeof(unsigned int));

                  if (ic.data)
                    {
                       p = (unsigned char *)data;
                       p2 = (unsigned int *)ic.data;
                       for (y = 0; y < h; y++)
                         {
                            for (x = 0; x < w; x++)
                              {
                                 *p2 = *((unsigned int *)p);
                                 p += sizeof(unsigned int);
                                 p2++;
                              }
                            p += (stride - (w * sizeof(unsigned int)));
                         }
                       ecore_x_netwm_icons_set(sd->x.xwin, &ic, 1);
                       free(ic.data);
                    }
               }
             evas_object_image_data_set(sd->icon, data);
          }
     }

   ecore_x_e_virtual_keyboard_state_set
     (sd->x.xwin, (Ecore_X_Virtual_Keyboard_State)sd->kbdmode);
   if (sd->indmode == ELM_WIN_INDICATOR_SHOW)
     ecore_x_e_illume_indicator_state_set
       (sd->x.xwin, ECORE_X_ILLUME_INDICATOR_STATE_ON);
   else if (sd->indmode == ELM_WIN_INDICATOR_HIDE)
     ecore_x_e_illume_indicator_state_set
       (sd->x.xwin, ECORE_X_ILLUME_INDICATOR_STATE_OFF);
}

static void
_elm_win_xwin_type_set(Elm_Win_Smart_Data *sd)
{
   _elm_win_xwindow_get(sd);
   if (!sd->x.xwin) return;  /* nothing more to do */

   switch (sd->type)
     {
      case ELM_WIN_BASIC:
        ecore_x_netwm_window_type_set(sd->x.xwin, ECORE_X_WINDOW_TYPE_NORMAL);
        break;

      case ELM_WIN_DIALOG_BASIC:
        ecore_x_netwm_window_type_set(sd->x.xwin, ECORE_X_WINDOW_TYPE_DIALOG);
        break;

      case ELM_WIN_DESKTOP:
        ecore_x_netwm_window_type_set(sd->x.xwin, ECORE_X_WINDOW_TYPE_DESKTOP);
        break;

      case ELM_WIN_DOCK:
        ecore_x_netwm_window_type_set(sd->x.xwin, ECORE_X_WINDOW_TYPE_DOCK);
        break;

      case ELM_WIN_TOOLBAR:
        ecore_x_netwm_window_type_set(sd->x.xwin, ECORE_X_WINDOW_TYPE_TOOLBAR);
        break;

      case ELM_WIN_MENU:
        ecore_x_netwm_window_type_set(sd->x.xwin, ECORE_X_WINDOW_TYPE_MENU);
        break;

      case ELM_WIN_UTILITY:
        ecore_x_netwm_window_type_set(sd->x.xwin, ECORE_X_WINDOW_TYPE_UTILITY);
        break;

      case ELM_WIN_SPLASH:
        ecore_x_netwm_window_type_set(sd->x.xwin, ECORE_X_WINDOW_TYPE_SPLASH);
        break;

      case ELM_WIN_DROPDOWN_MENU:
        ecore_x_netwm_window_type_set
          (sd->x.xwin, ECORE_X_WINDOW_TYPE_DROPDOWN_MENU);
        break;

      case ELM_WIN_POPUP_MENU:
        ecore_x_netwm_window_type_set
          (sd->x.xwin, ECORE_X_WINDOW_TYPE_POPUP_MENU);
        break;

      case ELM_WIN_TOOLTIP:
        ecore_x_netwm_window_type_set(sd->x.xwin, ECORE_X_WINDOW_TYPE_TOOLTIP);
        break;

      case ELM_WIN_NOTIFICATION:
        ecore_x_netwm_window_type_set
          (sd->x.xwin, ECORE_X_WINDOW_TYPE_NOTIFICATION);
        break;

      case ELM_WIN_COMBO:
        ecore_x_netwm_window_type_set(sd->x.xwin, ECORE_X_WINDOW_TYPE_COMBO);
        break;

      case ELM_WIN_DND:
        ecore_x_netwm_window_type_set(sd->x.xwin, ECORE_X_WINDOW_TYPE_DND);
        break;

      default:
        break;
     }

   return;
}
#endif


void
_elm_win_shutdown(void)
{
   while (_elm_win_list) evas_object_del(_elm_win_list->data);
   if (_elm_win_state_eval_job)
     {
        ecore_job_del(_elm_win_state_eval_job);
        _elm_win_state_eval_job = NULL;
     }
}

void
_elm_win_rescale(Elm_Theme *th,
                 Eina_Bool use_theme)
{
   const Eina_List *l;
   Evas_Object *obj;

   if (!use_theme)
     {
        EINA_LIST_FOREACH(_elm_win_list, l, obj)
          elm_widget_theme(obj);
     }
   else
     {
        EINA_LIST_FOREACH(_elm_win_list, l, obj)
          elm_widget_theme_specific(obj, th, EINA_FALSE);
     }
}

void
_elm_win_access(Eina_Bool is_access)
{
   Evas *evas;
   const Eina_List *l;
   Evas_Object *obj;
   Evas_Object *fobj;

   EINA_LIST_FOREACH(_elm_win_list, l, obj)
     {
        evas_object_smart_callback_call(obj, SIG_ACCESS_CHANGED, NULL);
        elm_widget_access(obj, is_access);

         /* floating orphan object. if there are A, B, C objects and user does
            as below, then there would be floating orphan objects.

              1. elm_object_content_set(layout, A);
              2. elm_object_content_set(layout, B);
              3. elm_object_content_set(layout, C);

            now, the object A and B are floating orphyan objects */

        fobj = obj;
        for (;;)
          {
             fobj = evas_object_below_get(fobj);
             if (!fobj) break;

             if (elm_widget_is(fobj) && !elm_widget_parent_get(fobj))
               {
                  elm_widget_access(fobj, is_access);
               }
          }

        if (!is_access)
          {
             evas = evas_object_evas_get(obj);
            if (evas) _elm_access_object_hilight_disable(evas);
          }
        else
          {
             // REDWOOD ONLY (20131230) : when access is enabled,
             //                           focus highlight is disabled.
             elm_win_focus_highlight_enabled_set(obj, EINA_FALSE);
          }
     }
}

void
_elm_win_paragraph_direction_set(void)
{
   const Eina_List *l;
   Evas_Object *obj;

   EINA_LIST_FOREACH(_elm_win_list, l, obj)
     {
        // TIZEN_ONLY(20140822): Apply evas_bidi_direction_hint_set according to locale.
        if (!strcmp(E_("default:LTR"), "default:RTL"))
          {
             evas_bidi_direction_hint_set(evas_object_evas_get(obj), EVAS_BIDI_DIRECTION_RTL);
          }
        else
          {
             evas_bidi_direction_hint_set(evas_object_evas_get(obj), EVAS_BIDI_DIRECTION_LTR);
          }

     }
}

void
_elm_win_translate(void)
{
   const Eina_List *l;
   Evas_Object *obj;

   EINA_LIST_FOREACH(_elm_win_list, l, obj)
     {
        // TIZEN_ONLY(20140822): Apply evas_bidi_direction_hint_set according to locale.
        if (!strcmp(E_("default:LTR"), "default:RTL"))
          {
             evas_bidi_direction_hint_set(evas_object_evas_get(obj), EVAS_BIDI_DIRECTION_RTL);
          }
        else
          {
             evas_bidi_direction_hint_set(evas_object_evas_get(obj), EVAS_BIDI_DIRECTION_LTR);
          }
        //
        elm_widget_translate(obj);
     }
}

#ifdef SDB_ENABLE
static int
_elm_win_send_message_to_sdb(Elm_Win_Smart_Data *sd, const char *msg)
{
   if (sd->sdb_server && msg)
     return ecore_ipc_server_send(sd->sdb_server, 0, 0, 0, 0, 0, msg, strlen(msg)+1);

   return 0;
}

static Eina_Bool
_elm_win_sdb_server_sent_cb(void *data, int type __UNUSED__, void *event)
{
   Elm_Win_Smart_Data *sd = data;
   Ecore_Ipc_Event_Server_Data *e;
   Obj_Dump_Mode_Api *mod_api = NULL;
   Eina_List *tree = NULL, *bufs = NULL;
   char *msg, *command, *text = NULL;
   const char *buf_text = NULL;

   // osp connection
   if (!evas_object_data_get(ELM_WIDGET_DATA(sd)->obj, "sdb_server"))
      return ECORE_CALLBACK_PASS_ON;

   e = (Ecore_Ipc_Event_Server_Data *) event;
   if (!e || !e->data) return ECORE_CALLBACK_PASS_ON;
   if (sd->sdb_server != e->server) return ECORE_CALLBACK_PASS_ON;

   if (sd->od_mod->api) mod_api = sd->od_mod->api;
   if (!mod_api) return ECORE_CALLBACK_PASS_ON;

   msg = strdup((char *)e->data);
   if (!msg) return ECORE_CALLBACK_PASS_ON;

   command = strtok(msg,"=");
   if (!command)
     {
        free(msg);
        return ECORE_CALLBACK_PASS_ON;
     }

   // create object dump tree
   if (mod_api->tree_create)
      tree = mod_api->tree_create(ELM_WIDGET_DATA(sd)->obj);

   if (tree)
     {
        // call object_dump module functions
        if (!strcmp(command, "AT+DUMPWND"))
          {
             if (mod_api->tree_string_get_for_sdb)
                bufs = mod_api->tree_string_get_for_sdb(tree);
          }
        else if (!strcmp(command, "AT+DUMPALL"))
          {
             if (mod_api->tree_string_get)
                bufs = mod_api->tree_string_get(tree);
          }
        else if (!strcmp(command, "AT+GETPARAM") ||
                 !strcmp(command, "AT+GETOTEXT") ||
                 !strcmp(command, "AT+CLROTEXT") ||
                 !strcmp(command, "AT+SETOTEXT"))
          {
             if (mod_api->command_for_sdb)
                text = mod_api->command_for_sdb(tree, e->data);
          }
        else
           text = strdup("Invaild Command!!");

        // send response to server
        if (bufs)
          {
             Eina_Strbuf *strbuf;

             EINA_LIST_FREE(bufs, strbuf)
               {
                  buf_text = eina_strbuf_string_get(strbuf);
                  _elm_win_send_message_to_sdb(sd, buf_text);
                  eina_strbuf_free(strbuf);
               }
          }
        else if (text)
          {
             _elm_win_send_message_to_sdb(sd, text);
             free(text);
          }

        // free object dump tree
        if (mod_api->tree_free) mod_api->tree_free(tree);
     }
   else
     {
        free(msg);
        return ECORE_CALLBACK_PASS_ON;
     }

   free(msg);
   return ECORE_CALLBACK_PASS_ON;
}

static void
_elm_win_sdb_server_del(Elm_Win_Smart_Data *sd)
{
   if (!sd || !sd->sdb_server) return;

   ELM_SAFE_FREE(sd->od_mod->api, free);
   ELM_SAFE_FREE(sd->sdb_server_data_handler, ecore_event_handler_del);
   ELM_SAFE_FREE(sd->sdb_server_del_handler, ecore_event_handler_del);
   ELM_SAFE_FREE(sd->sdb_server, ecore_ipc_server_del);
   evas_object_data_set(ELM_WIDGET_DATA(sd)->obj, "sdb_server", NULL);
   ecore_ipc_shutdown();
}

static Eina_Bool
_elm_win_sdb_server_del_cb(void *data, int type __UNUSED__, void *event)
{
   Ecore_Ipc_Event_Server_Data *e = event;
   Elm_Win_Smart_Data *sd = data;

   if (!sd || !sd->sdb_server || !e) return ECORE_CALLBACK_PASS_ON;
   if (sd->sdb_server != e->server) return ECORE_CALLBACK_PASS_ON;

   _elm_win_sdb_server_del(sd);

   return ECORE_CALLBACK_PASS_ON;
}

/**
  * note that this _object_dump_mod_init() should return (Elm_Module *) not
  * (Obj_Dump_Mode_Api *).
  * mod->api should be set to NULL when it's freed or mod->api will be freed
  * during module unload.
  */
static Elm_Module *
_object_dump_mod_init()
{
   Elm_Module *mod = NULL;
   mod = _elm_module_find_as("win/api");
   if (!mod) return NULL;

   mod->api = malloc(sizeof(Obj_Dump_Mode_Api));
   if (!mod->api) return NULL;

   ((Obj_Dump_Mode_Api *)(mod->api))->tree_create = _elm_module_symbol_get(mod, "tree_create");
   ((Obj_Dump_Mode_Api *)(mod->api))->tree_free = _elm_module_symbol_get(mod, "tree_free");
   ((Obj_Dump_Mode_Api *)(mod->api))->tree_string_get = _elm_module_symbol_get(mod, "tree_string_get");
   ((Obj_Dump_Mode_Api *)(mod->api))->tree_string_get_for_sdb = _elm_module_symbol_get(mod, "tree_string_get_for_sdb");
   ((Obj_Dump_Mode_Api *)(mod->api))->command_for_sdb = _elm_module_symbol_get(mod, "command_for_sdb");

   return mod;
}
#endif

#ifdef UTILITY_MODULE_ENABLE
static Elm_Module *
_utility_mod_init()
{
   Elm_Module *mod = NULL;
   mod = _elm_module_find_as("utility/api");
   if (!mod) return NULL;
   mod->init_func(mod);

   return mod;
}

static Eina_Bool
_utility_mod_shutdown(void *data)
{
   Elm_Win_Smart_Data *sd = data;

   if (!sd || !sd->utility_mod) return EINA_FALSE;

   if (sd->utility_mod->shutdown_func)
     sd->utility_mod->shutdown_func(sd->utility_mod);
   sd->utility_mod = NULL;

   return EINA_TRUE;
}
#endif // UTILITY_MODULE_ENABLE

#ifdef UI_ANALYZER_ENABLE
static Elm_Module *
_ui_analyzer_mod_init()
{
   Elm_Module *mod = NULL;
   mod = _elm_module_find_as("ui_analyzer/api");
   if (!mod) return NULL;
   mod->init_func(mod);

   return mod;
}

static Eina_Bool
_ui_analyzer_mod_shutdown(void *data)
{
   Elm_Win_Smart_Data *sd = data;

   if ((!sd) || (!sd->ui_analyzer_mod)) return EINA_FALSE;

   if (sd->ui_analyzer_mod->shutdown_func)
     sd->ui_analyzer_mod->shutdown_func(sd->ui_analyzer_mod);
   sd->ui_analyzer_mod = NULL;

   return EINA_TRUE;
}
#endif

#ifdef HAVE_ELEMENTARY_X
static Eina_Bool
_elm_win_client_message(void *data,
                        int type __UNUSED__,
                        void *event)
{
   Elm_Access_Action_Info *a;
   Elm_Win_Smart_Data *sd = data;
   Ecore_X_Event_Client_Message *e = event;

   if (e->format != 32) return ECORE_CALLBACK_PASS_ON;
   if (e->message_type == ECORE_X_ATOM_E_COMP_FLUSH)
     {
        if ((unsigned int)e->data.l[0] == sd->x.xwin)
          {
             Evas *evas = evas_object_evas_get(ELM_WIDGET_DATA(sd)->obj);
             if (evas)
               {
                  edje_file_cache_flush();
                  edje_collection_cache_flush();
                  evas_image_cache_flush(evas);
                  evas_font_cache_flush(evas);
               }
          }
     }
   else if (e->message_type == ECORE_X_ATOM_E_COMP_DUMP)
     {
        if ((unsigned int)e->data.l[0] == sd->x.xwin)
          {
             Evas *evas = evas_object_evas_get(ELM_WIDGET_DATA(sd)->obj);
             if (evas)
               {
                  edje_file_cache_flush();
                  edje_collection_cache_flush();
                  evas_image_cache_flush(evas);
                  evas_font_cache_flush(evas);
                  evas_render_dump(evas);
               }
          }
     }
   else if (e->message_type == ECORE_X_ATOM_E_ILLUME_ACCESS_CONTROL)
     {
        if ((unsigned int)e->data.l[0] == sd->x.xwin)
          {
             if ((unsigned int)e->data.l[1] ==
                 ECORE_X_ATOM_E_ILLUME_ACCESS_ACTION_NEXT)
               {
                  // XXX: call right access func
               }
             else if ((unsigned int)e->data.l[1] ==
                      ECORE_X_ATOM_E_ILLUME_ACCESS_ACTION_PREV)
               {
                  // XXX: call right access func
               }
             else if ((unsigned int)e->data.l[1] ==
                      ECORE_X_ATOM_E_ILLUME_ACCESS_ACTION_ACTIVATE)
               {
                 a = calloc(1, sizeof(Elm_Access_Action_Info));
                 if (!a) return ECORE_CALLBACK_PASS_ON;

                 a->x = e->data.l[3];
                 a->y = e->data.l[4];
                 elm_access_action(ELM_WIDGET_DATA(sd)->obj,
                                    ELM_ACCESS_ACTION_ACTIVATE, a);
                 free(a);
               }
             else if ((unsigned int)e->data.l[1] ==
                      ECORE_X_ATOM_E_ILLUME_ACCESS_ACTION_READ)
               {
                  a = calloc(1, sizeof(Elm_Access_Action_Info));
                  if (!a) return ECORE_CALLBACK_PASS_ON;

                  a->x = e->data.l[2];
                  a->y = e->data.l[3];
                  elm_access_action(ELM_WIDGET_DATA(sd)->obj, ELM_ACCESS_ACTION_HIGHLIGHT, a);
                  free(a);
               }
             else if ((unsigned int)e->data.l[1] ==
                      ECORE_X_ATOM_E_ILLUME_ACCESS_ACTION_OVER)
               {
                  a = calloc(1, sizeof(Elm_Access_Action_Info));
                  if (!a) return ECORE_CALLBACK_PASS_ON;

                  a->mouse_type = e->data.l[2];
                  a->x = e->data.l[3];
                  a->y = e->data.l[4];
                  elm_access_action(ELM_WIDGET_DATA(sd)->obj, ELM_ACCESS_ACTION_OVER, a);
                  free(a);
               }
             else if ((unsigned int)e->data.l[1] ==
                      ECORE_X_ATOM_E_ILLUME_ACCESS_ACTION_READ_NEXT)
               {
                  a = calloc(1, sizeof(Elm_Access_Action_Info));
                  if (!a) return ECORE_CALLBACK_PASS_ON;

                  a->highlight_cycle = EINA_TRUE;
                  elm_access_action(ELM_WIDGET_DATA(sd)->obj,
                                    ELM_ACCESS_ACTION_HIGHLIGHT_NEXT, a);
                  free(a);
               }
             else if ((unsigned int)e->data.l[1] ==
                      ECORE_X_ATOM_E_ILLUME_ACCESS_ACTION_READ_PREV)
               {
                  a = calloc(1, sizeof(Elm_Access_Action_Info));
                  if (!a) return ECORE_CALLBACK_PASS_ON;

                  a->highlight_cycle = EINA_TRUE;
                  elm_access_action(ELM_WIDGET_DATA(sd)->obj,
                                    ELM_ACCESS_ACTION_HIGHLIGHT_PREV, a);
                  free(a);
               }
             else if ((unsigned int)e->data.l[1] ==
                      ECORE_X_ATOM_E_ILLUME_ACCESS_ACTION_UP)
               {
                  elm_access_action(ELM_WIDGET_DATA(sd)->obj,
                                    ELM_ACCESS_ACTION_UP, NULL);
               }
             else if ((unsigned int)e->data.l[1] ==
                      ECORE_X_ATOM_E_ILLUME_ACCESS_ACTION_DOWN)
               {
                  elm_access_action(ELM_WIDGET_DATA(sd)->obj,
                                    ELM_ACCESS_ACTION_DOWN, NULL);
               }
             else if ((unsigned int)e->data.l[1] ==
                      ECORE_X_ATOM_E_ILLUME_ACCESS_ACTION_SCROLL)
               {
                  a = calloc(1, sizeof(Elm_Access_Action_Info));
                  if (!a) return ECORE_CALLBACK_PASS_ON;

                  a->mouse_type = e->data.l[2];
                  a->x = e->data.l[3];
                  a->y = e->data.l[4];
                  elm_access_action(ELM_WIDGET_DATA(sd)->obj,
                                    ELM_ACCESS_ACTION_SCROLL, a);
                  free(a);
               }
             else if ((unsigned int)e->data.l[1] ==
                      ECORE_X_ATOM_E_ILLUME_ACCESS_ACTION_ZOOM)
               {
                  a = calloc(1, sizeof(Elm_Access_Action_Info));
                  if (!a) return ECORE_CALLBACK_PASS_ON;

                  a->mouse_type = e->data.l[2];
                  a->zoom = (double)(e->data.l[3] / 1000.0);
                  a->angle = (double)(e->data.l[4] / 1000.0);
                  elm_access_action(ELM_WIDGET_DATA(sd)->obj,
                                    ELM_ACCESS_ACTION_ZOOM, a);
                  free(a);
               }
             else if ((unsigned int)e->data.l[1] ==
                      ECORE_X_ATOM_E_ILLUME_ACCESS_ACTION_MOUSE)
               {
                  a = calloc(1, sizeof(Elm_Access_Action_Info));
                  if (!a) return ECORE_CALLBACK_PASS_ON;

                  a->mouse_type = e->data.l[2];
                  a->x = e->data.l[3];
                  a->y = e->data.l[4];
                  elm_access_action(ELM_WIDGET_DATA(sd)->obj,
                                    ELM_ACCESS_ACTION_MOUSE, a);
                  free(a);
               }
             else if ((unsigned int)e->data.l[1] ==
                      ECORE_X_ATOM_E_ILLUME_ACCESS_ACTION_ENABLE)
               {
                  elm_access_action(ELM_WIDGET_DATA(sd)->obj,
                                    ELM_ACCESS_ACTION_ENABLE, NULL);
               }
             else if ((unsigned int)e->data.l[1] ==
                      ECORE_X_ATOM_E_ILLUME_ACCESS_ACTION_DISABLE)
               {
                  elm_access_action(ELM_WIDGET_DATA(sd)->obj,
                                    ELM_ACCESS_ACTION_DISABLE, NULL);
               }
          }
     }
   else if (e->message_type == ECORE_X_ATOM_E_INDICATOR_FLICK_DONE)
     {
        evas_object_smart_callback_call(ELM_WIDGET_DATA(sd)->obj, SIG_INDICATOR_FLICK_DONE, NULL);
     }
   else if (e->message_type == ECORE_X_ATOM_E_ILLUME_QUICKPANEL_STATE)
     {
        if ((unsigned int)e->data.l[0] == ECORE_X_ATOM_E_ILLUME_QUICKPANEL_OFF)
          evas_object_smart_callback_call(ELM_WIDGET_DATA(sd)->obj, SIG_QUICKPANEL_STATE_OFF, NULL);
        else if ((unsigned int)e->data.l[0] == ECORE_X_ATOM_E_ILLUME_QUICKPANEL_ON)
          evas_object_smart_callback_call(ELM_WIDGET_DATA(sd)->obj, SIG_QUICKPANEL_STATE_ON, NULL);
     }
   else if (e->message_type == ECORE_X_ATOM_E_MOVE_QUICKPANEL_STATE)
     {
        if ((unsigned int)e->data.l[0] == 0)
          evas_object_smart_callback_call(ELM_WIDGET_DATA(sd)->obj, SIG_QUICKPANEL_STATE_OFF, NULL);
        else if ((unsigned int)e->data.l[0] == 1)
          evas_object_smart_callback_call(ELM_WIDGET_DATA(sd)->obj, SIG_QUICKPANEL_STATE_ON, NULL);
     }
#ifdef SDB_ENABLE
   else if (e->message_type == ECORE_X_ATOM_SDB_SERVER_CONNECT)
     {
#ifdef UTILITY_MODULE_ENABLE
        if (((unsigned)e->data.l[0] == 1) &&
            ((unsigned)e->data.l[1] == sd->x.xwin))
          sd->utility_mod = _utility_mod_init();

        else if (((unsigned)e->data.l[0] == 0) &&
                 ((unsigned)e->data.l[1] == sd->x.xwin))
          _utility_mod_shutdown(sd);

#endif // UTILITY_MODULE_ENABLE

#ifdef UI_ANALYZER_ENABLE
        if (((unsigned)e->data.l[0] == 1) &&
            ((unsigned)e->data.l[1] == sd->x.xwin))
          sd->ui_analyzer_mod = _ui_analyzer_mod_init();
#endif
        if ((unsigned)e->data.l[0] == sd->x.xwin)
          {
             sd->od_mod = _object_dump_mod_init();
             if (sd->od_mod && !sd->sdb_server)
               {
                  ecore_ipc_init();

                  sd->sdb_server =
                     ecore_ipc_server_connect(ECORE_IPC_LOCAL_SYSTEM,
                                              "sdb", 0, NULL);
                  if (sd->sdb_server)
                    {
                       evas_object_data_set(ELM_WIDGET_DATA(sd)->obj,
                                            "sdb_server", sd->sdb_server);

                       sd->sdb_server_data_handler =
                          ecore_event_handler_add(ECORE_IPC_EVENT_SERVER_DATA,
                                                  _elm_win_sdb_server_sent_cb,
                                                  sd);
                       sd->sdb_server_del_handler =
                          ecore_event_handler_add(ECORE_IPC_EVENT_SERVER_DEL,
                                                  _elm_win_sdb_server_del_cb,
                                                  sd);
                    }
                  else
                    {
                       if (sd->od_mod)
                         ELM_SAFE_FREE(sd->od_mod->api, free);
                       ecore_ipc_shutdown();
                    }
               }
          }
     }
   else if (e->message_type == ECORE_X_ATOM_SDB_SERVER_DISCONNECT)
     {
#ifdef UI_ANALYZER_ENABLE
        if (((unsigned)e->data.l[0] == 1) &&
            ((unsigned)e->data.l[1] == sd->x.xwin))
          _ui_analyzer_mod_shutdown(sd);
#endif
        if ((unsigned)e->data.l[0] == sd->x.xwin)
          _elm_win_sdb_server_del(sd);
     }
#endif
   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_elm_win_property_change(void *data,
                         int type __UNUSED__,
                         void *event)
{
   Elm_Win_Smart_Data *sd = data;
   Ecore_X_Event_Window_Property *e = event;
   unsigned int _win_desktop_layout = -1;

   if (e->atom == ECORE_X_ATOM_E_WINDOW_DESKTOP_LAYOUT)
     {
        if (ecore_x_window_prop_card32_get(e->win,
                                       ECORE_X_ATOM_E_WINDOW_DESKTOP_LAYOUT,
                                       &_win_desktop_layout, 1))
          evas_object_smart_callback_call(ELM_WIDGET_DATA(sd)->obj, SIG_DESKTOP_LAYOUT,
                                          (void *)_win_desktop_layout);
     }
   if (e->atom == ECORE_X_ATOM_E_ILLUME_INDICATOR_STATE)
     {
        if (e->win == sd->x.xwin)
          {
             sd->indmode = ecore_x_e_illume_indicator_state_get(e->win);
             evas_object_smart_callback_call(ELM_WIDGET_DATA(sd)->obj, SIG_INDICATOR_PROP_CHANGED, NULL);
          }
     }
   if (e->atom == ECORE_X_ATOM_E_VIRTUAL_KEYBOARD_STATE)
     {
        if (e->win == sd->x.xwin)
          {
             sd->kbdmode = ecore_x_e_virtual_keyboard_state_get(e->win);
          }
     }
   return ECORE_CALLBACK_PASS_ON;
}

#ifdef ELM_FOCUSED_UI
// TIZEN ONLY (20130422) : For automating default focused UI.
static Eina_Bool
_elm_win_mouse_button_down(void *data,
                           int type __UNUSED__,
                           void *event __UNUSED__)
{

   ELM_WIN_DATA_GET_OR_RETURN_VAL(data, sd, ECORE_CALLBACK_PASS_ON);

   if (sd->keyboard_attached)
     {
        if (elm_win_focus_highlight_enabled_get(data))
          {
             DBG("-[WIN_FOCUS] highlight_disabled");
             elm_win_focus_highlight_enabled_set(data, EINA_FALSE);
             sd->focus_highlight.prev.visible = EINA_FALSE;
          }
     }

   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_elm_win_key_down(void *data,
                  int type __UNUSED__,
                  void *event)
{
   ELM_WIN_DATA_GET_OR_RETURN_VAL(data, sd, ECORE_CALLBACK_PASS_ON);
   Ecore_Event_Key *ev = (Ecore_Event_Key *)event;

   DBG("[WIN_FOCUS] win(%p) enabled(%d)",
       data, elm_win_focus_highlight_enabled_get(data));

   if (_elm_config->access_mode ||
       !_elm_widget_top_win_focused_get(data) ||
       elm_win_focus_highlight_enabled_get(data))
     return ECORE_CALLBACK_PASS_ON;

   if (strcmp(ev->keyname, "Tab") && strcmp(ev->keyname, "ISO_Left_Tab") &&
       strcmp(ev->keyname, "Left") && strcmp(ev->keyname, "KP_Left") &&
       strcmp(ev->keyname, "Right") && strcmp(ev->keyname, "KP_Right") &&
       strcmp(ev->keyname, "Up") && strcmp(ev->keyname, "KP_Up") &&
       strcmp(ev->keyname, "Down") && strcmp(ev->keyname, "KP_Down") &&
       strcmp(ev->keyname, "Return") && strcmp(ev->keyname, "KP_Enter") &&
       strcmp(ev->keyname, "space"))
     return ECORE_CALLBACK_PASS_ON;

   if (!sd->first_key_down)
     {
        if (ev->timestamp)
          DBG("-[WIN_FOCUS] timestamp(%d)", ev->timestamp);
        if (ev->timestamp && ev->timestamp > 1)
          {
             sd->keyboard_attached = EINA_TRUE;
             sd->focus_highlight.cur.visible = EINA_TRUE;
             DBG("-[WIN_FOCUS] highlight_enabled");
             elm_win_focus_highlight_enabled_set(data, EINA_TRUE);
             sd->first_key_down = EINA_TRUE;
          }
     }
   else if (sd->keyboard_attached)
     {
        DBG("-[WIN_FOCUS] hightlight_enabled");
        sd->focus_highlight.cur.visible = EINA_TRUE;
        elm_win_focus_highlight_enabled_set(data, EINA_TRUE);
     }

   return ECORE_CALLBACK_PASS_ON;
}
///////////////////////////
#endif
#endif

static void
_elm_win_focus_highlight_hide(void *data __UNUSED__,
                              Evas_Object *obj,
                              const char *emission __UNUSED__,
                              const char *source __UNUSED__)
{
   evas_object_hide(obj);
}

static void
_elm_win_focus_highlight_anim_end(void *data,
                                  Evas_Object *obj,
                                  const char *emission __UNUSED__,
                                  const char *source __UNUSED__)
{
   Elm_Win_Smart_Data *sd = data;

   _elm_win_focus_highlight_simple_setup(sd, obj);
}

static void
_elm_win_focus_highlight_init(Elm_Win_Smart_Data *sd)
{
   evas_event_callback_add(sd->evas, EVAS_CALLBACK_CANVAS_OBJECT_FOCUS_IN,
                           _elm_win_object_focus_in, sd);
   evas_event_callback_add(sd->evas,
                           EVAS_CALLBACK_CANVAS_OBJECT_FOCUS_OUT,
                           _elm_win_object_focus_out, sd);
#ifdef ELM_FOCUSED_UI
   // TIZEN_ONLY (20130713) : For supporting Focused UI of TIZEN.
   //sd->focus_highlight.cur.target = evas_focus_get(sd->evas);
   sd->focus_highlight.cur.target = _elm_win_focus_target_get(evas_focus_get(sd->evas));
   if (elm_widget_highlight_in_theme_get(sd->focus_highlight.cur.target))
     sd->focus_highlight.cur.handled = EINA_TRUE;
   //////
#endif

   sd->focus_highlight.top = edje_object_add(sd->evas);
   sd->focus_highlight.changed_theme = EINA_TRUE;
   edje_object_signal_callback_add(sd->focus_highlight.top,
                                   "elm,action,focus,hide,end", "",
                                   _elm_win_focus_highlight_hide, NULL);
   edje_object_signal_callback_add(sd->focus_highlight.top,
                                   "elm,action,focus,anim,end", "",
                                   _elm_win_focus_highlight_anim_end, sd);
   DBG("[WIN_FOCUS:%p] highlight_reconfigure_start INIT",
       ELM_WIDGET_DATA(sd)->obj);
   _elm_win_focus_highlight_reconfigure_job_start(sd);
}

static void
_elm_win_frame_cb_move_start(void *data,
                             Evas_Object *obj __UNUSED__,
                             const char *sig __UNUSED__,
                             const char *source __UNUSED__)
{
   Elm_Win_Smart_Data *sd;

   if (!(sd = data)) return;
   /* FIXME: Change mouse pointer */

   /* NB: Wayland handles moving surfaces by itself so we cannot
    * specify a specific x/y we want. Instead, we will pass in the
    * existing x/y values so they can be recorded as 'previous'
    * position. The new position will get updated automatically when
    * the move is finished */

   ecore_evas_wayland_move(sd->ee, sd->screen.x, sd->screen.y);
}

static void
_elm_win_frame_cb_resize_show(void *data,
                              Evas_Object *obj __UNUSED__,
                              const char *sig __UNUSED__,
                              const char *source)
{
   Elm_Win_Smart_Data *sd;

   if (!(sd = data)) return;
   if (sd->resizing) return;

#ifdef HAVE_ELEMENTARY_WAYLAND
   if (!strcmp(source, "elm.event.resize.t"))
     ecore_wl_window_cursor_from_name_set(sd->wl.win, ELM_CURSOR_TOP_SIDE);
   else if (!strcmp(source, "elm.event.resize.b"))
     ecore_wl_window_cursor_from_name_set(sd->wl.win, ELM_CURSOR_BOTTOM_SIDE);
   else if (!strcmp(source, "elm.event.resize.l"))
     ecore_wl_window_cursor_from_name_set(sd->wl.win, ELM_CURSOR_LEFT_SIDE);
   else if (!strcmp(source, "elm.event.resize.r"))
     ecore_wl_window_cursor_from_name_set(sd->wl.win, ELM_CURSOR_RIGHT_SIDE);
   else if (!strcmp(source, "elm.event.resize.tl"))
     ecore_wl_window_cursor_from_name_set(sd->wl.win,
                                          ELM_CURSOR_TOP_LEFT_CORNER);
   else if (!strcmp(source, "elm.event.resize.tr"))
     ecore_wl_window_cursor_from_name_set(sd->wl.win,
                                          ELM_CURSOR_TOP_RIGHT_CORNER);
   else if (!strcmp(source, "elm.event.resize.bl"))
     ecore_wl_window_cursor_from_name_set(sd->wl.win,
                                          ELM_CURSOR_BOTTOM_LEFT_CORNER);
   else if (!strcmp(source, "elm.event.resize.br"))
     ecore_wl_window_cursor_from_name_set(sd->wl.win,
                                          ELM_CURSOR_BOTTOM_RIGHT_CORNER);
   else
     ecore_wl_window_cursor_default_restore(sd->wl.win);
#else
   (void)source;
#endif
}

static void
_elm_win_frame_cb_resize_hide(void *data,
                              Evas_Object *obj __UNUSED__,
                              const char *sig __UNUSED__,
                              const char *source __UNUSED__)
{
   Elm_Win_Smart_Data *sd;

   if (!(sd = data)) return;
   if (sd->resizing) return;

#ifdef HAVE_ELEMENTARY_WAYLAND
   ecore_wl_window_cursor_default_restore(sd->wl.win);
#endif
}

static void
_elm_win_frame_cb_resize_start(void *data,
                               Evas_Object *obj __UNUSED__,
                               const char *sig __UNUSED__,
                               const char *source)
{
   Elm_Win_Smart_Data *sd;

   if (!(sd = data)) return;
   if (sd->resizing) return;

   sd->resizing = EINA_TRUE;

   if (!strcmp(source, "elm.event.resize.t"))
     sd->resize_location = 1;
   else if (!strcmp(source, "elm.event.resize.b"))
     sd->resize_location = 2;
   else if (!strcmp(source, "elm.event.resize.l"))
     sd->resize_location = 4;
   else if (!strcmp(source, "elm.event.resize.r"))
     sd->resize_location = 8;
   else if (!strcmp(source, "elm.event.resize.tl"))
     sd->resize_location = 5;
   else if (!strcmp(source, "elm.event.resize.tr"))
     sd->resize_location = 9;
   else if (!strcmp(source, "elm.event.resize.bl"))
     sd->resize_location = 6;
   else if (!strcmp(source, "elm.event.resize.br"))
     sd->resize_location = 10;
   else
     sd->resize_location = 0;

   if (sd->resize_location > 0)
     ecore_evas_wayland_resize(sd->ee, sd->resize_location);
}

static void
_elm_win_frame_cb_minimize(void *data,
                           Evas_Object *obj __UNUSED__,
                           const char *sig __UNUSED__,
                           const char *source __UNUSED__)
{
   Elm_Win_Smart_Data *sd;

   if (!(sd = data)) return;
// sd->iconified = EINA_TRUE;
   TRAP(sd, iconified_set, EINA_TRUE);
}

static void
_elm_win_frame_cb_maximize(void *data,
                           Evas_Object *obj __UNUSED__,
                           const char *sig __UNUSED__,
                           const char *source __UNUSED__)
{
   Elm_Win_Smart_Data *sd;

   if (!(sd = data)) return;
   if (sd->maximized) sd->maximized = EINA_FALSE;
   else sd->maximized = EINA_TRUE;
   TRAP(sd, maximized_set, sd->maximized);
}

static void
_elm_win_frame_cb_close(void *data,
                        Evas_Object *obj __UNUSED__,
                        const char *sig __UNUSED__,
                        const char *source __UNUSED__)
{
   Elm_Win_Smart_Data *sd;
   Evas_Object *win;

   /* FIXME: After the current freeze, this should be handled differently.
    *
    * Ideally, we would want to mimic the X11 backend and use something
    * like ECORE_WL_EVENT_WINDOW_DELETE and handle the delete_request
    * inside of ecore_evas. That would be the 'proper' way, but since we are
    * in a freeze right now, I cannot add a new event value, or a new
    * event structure to ecore_wayland.
    *
    * So yes, this is a temporary 'stop-gap' solution which will be fixed
    * when the freeze is over, but it does fix a trac bug for now, and in a
    * way which does not break API or the freeze. - dh
    */

   if (!(sd = data)) return;

   win = ELM_WIDGET_DATA(sd)->obj;

   int autodel = sd->autodel;
   sd->autodel_clear = &autodel;
   evas_object_ref(win);
   evas_object_smart_callback_call(win, SIG_DELETE_REQUEST, NULL);
   // FIXME: if above callback deletes - then the below will be invalid
   if (autodel) evas_object_del(win);
   else sd->autodel_clear = NULL;
   evas_object_unref(win);
}

static void
_elm_win_frame_add(Elm_Win_Smart_Data *sd,
                   const char *style)
{
   evas_output_framespace_set(sd->evas, 0, 22, 0, 26);

   sd->frame_obj = edje_object_add(sd->evas);
   elm_widget_theme_object_set
     (ELM_WIDGET_DATA(sd)->obj, sd->frame_obj, "border", "base", style);

   evas_object_is_frame_object_set(sd->frame_obj, EINA_TRUE);
   evas_object_move(sd->frame_obj, 0, 0);
   evas_object_resize(sd->frame_obj, 1, 1);

   edje_object_signal_callback_add
     (sd->frame_obj, "elm,action,move,start", "elm",
     _elm_win_frame_cb_move_start, sd);
   edje_object_signal_callback_add
     (sd->frame_obj, "elm,action,resize,show", "*",
     _elm_win_frame_cb_resize_show, sd);
   edje_object_signal_callback_add
     (sd->frame_obj, "elm,action,resize,hide", "*",
     _elm_win_frame_cb_resize_hide, sd);
   edje_object_signal_callback_add
     (sd->frame_obj, "elm,action,resize,start", "*",
     _elm_win_frame_cb_resize_start, sd);
   edje_object_signal_callback_add
     (sd->frame_obj, "elm,action,minimize", "elm",
     _elm_win_frame_cb_minimize, sd);
   edje_object_signal_callback_add
     (sd->frame_obj, "elm,action,maximize", "elm",
     _elm_win_frame_cb_maximize, sd);
   edje_object_signal_callback_add
     (sd->frame_obj, "elm,action,close", "elm", _elm_win_frame_cb_close, sd);

   if (sd->title)
     {
        edje_object_part_text_escaped_set
          (sd->frame_obj, "elm.text.title", sd->title);
     }
}

static void
_elm_win_frame_del(Elm_Win_Smart_Data *sd)
{
   if (sd->frame_obj)
     {
        edje_object_signal_callback_del
          (sd->frame_obj, "elm,action,move,start", "elm",
              _elm_win_frame_cb_move_start);
        edje_object_signal_callback_del
          (sd->frame_obj, "elm,action,resize,show", "*",
              _elm_win_frame_cb_resize_show);
        edje_object_signal_callback_del
          (sd->frame_obj, "elm,action,resize,hide", "*",
              _elm_win_frame_cb_resize_hide);
        edje_object_signal_callback_del
          (sd->frame_obj, "elm,action,resize,start", "*",
              _elm_win_frame_cb_resize_start);
        edje_object_signal_callback_del
          (sd->frame_obj, "elm,action,minimize", "elm",
              _elm_win_frame_cb_minimize);
        edje_object_signal_callback_del
          (sd->frame_obj, "elm,action,maximize", "elm",
              _elm_win_frame_cb_maximize);
        edje_object_signal_callback_del
          (sd->frame_obj, "elm,action,close", "elm",
              _elm_win_frame_cb_close);

        evas_object_del(sd->frame_obj);
        sd->frame_obj = NULL;
     }

   evas_output_framespace_set(sd->evas, 0, 0, 0, 0);
}

#ifdef ELM_DEBUG
static void
_debug_key_down(void *data __UNUSED__,
                Evas *e __UNUSED__,
                Evas_Object *obj,
                void *event_info)
{
   Evas_Event_Key_Down *ev = event_info;

   if (ev->event_flags & EVAS_EVENT_FLAG_ON_HOLD)
     return;

   if ((strcmp(ev->keyname, "F12")) ||
       (!evas_key_modifier_is_set(ev->modifiers, "Control")))
     return;

   printf("Tree graph generated.\n");
   elm_object_tree_dot_dump(obj, "./dump.dot");
}

#endif

static void
_win_img_hide(void *data,
              Evas *e __UNUSED__,
              Evas_Object *obj __UNUSED__,
              void *event_info __UNUSED__)
{
   Elm_Win_Smart_Data *sd = data;

   elm_widget_focus_hide_handle(ELM_WIDGET_DATA(sd)->obj);
}

static void
_win_img_mouse_up(void *data,
                  Evas *e __UNUSED__,
                  Evas_Object *obj __UNUSED__,
                  void *event_info)
{
   Elm_Win_Smart_Data *sd = data;
   Evas_Event_Mouse_Up *ev = event_info;
   if (!(ev->event_flags & EVAS_EVENT_FLAG_ON_HOLD))
     elm_widget_focus_mouse_up_handle(ELM_WIDGET_DATA(sd)->obj);
}

static void
_win_img_focus_in(void *data,
                  Evas *e __UNUSED__,
                  Evas_Object *obj __UNUSED__,
                  void *event_info __UNUSED__)
{
   Elm_Win_Smart_Data *sd = data;
   elm_widget_focus_steal(ELM_WIDGET_DATA(sd)->obj);
}

static void
_win_img_focus_out(void *data,
                   Evas *e __UNUSED__,
                   Evas_Object *obj __UNUSED__,
                   void *event_info __UNUSED__)
{
   Elm_Win_Smart_Data *sd = data;
   elm_widget_focused_object_clear(ELM_WIDGET_DATA(sd)->obj);
}

static void
_win_inlined_image_set(Elm_Win_Smart_Data *sd)
{
   evas_object_image_alpha_set(sd->img_obj, EINA_FALSE);
   evas_object_image_filled_set(sd->img_obj, EINA_TRUE);

   evas_object_event_callback_add
     (sd->img_obj, EVAS_CALLBACK_DEL, _elm_win_on_img_obj_del, sd);
   evas_object_event_callback_add
     (sd->img_obj, EVAS_CALLBACK_HIDE, _win_img_hide, sd);
   evas_object_event_callback_add
     (sd->img_obj, EVAS_CALLBACK_MOUSE_UP, _win_img_mouse_up, sd);
   evas_object_event_callback_add
     (sd->img_obj, EVAS_CALLBACK_FOCUS_IN, _win_img_focus_in, sd);
   evas_object_event_callback_add
     (sd->img_obj, EVAS_CALLBACK_FOCUS_OUT, _win_img_focus_out, sd);
}

static void
_elm_win_on_icon_del(void *data,
                     Evas *e __UNUSED__,
                     Evas_Object *obj,
                     void *event_info __UNUSED__)
{
   Elm_Win_Smart_Data *sd = data;

   if (sd->icon == obj) sd->icon = NULL;
}

static void
_elm_win_smart_add(Evas_Object *obj)
{
   EVAS_SMART_DATA_ALLOC(obj, Elm_Win_Smart_Data);

   _elm_win_parent_sc->base.add(obj);

   elm_widget_can_focus_set(obj, EINA_TRUE);

   elm_widget_highlight_ignore_set(obj, EINA_TRUE);
}

static void
_elm_win_smart_set_user(Elm_Widget_Smart_Class *sc)
{
   sc->base.add = _elm_win_smart_add;
   sc->base.del = _elm_win_smart_del;
   sc->base.show = _elm_win_smart_show;
   sc->base.hide = _elm_win_smart_hide;
   sc->base.move = _elm_win_smart_move;
   sc->base.resize = _elm_win_smart_resize;

   sc->focus_next = _elm_win_smart_focus_next;
   sc->focus_direction_manager_is = _elm_win_smart_focus_direction_manager_is;
   sc->focus_direction = _elm_win_smart_focus_direction;
   sc->on_focus = _elm_win_smart_on_focus;
   sc->event = _elm_win_smart_event;
}

#ifdef HAVE_ELEMENTARY_X
static void
_elm_x_io_err(void *data __UNUSED__)
{
   Eina_List *l;
   Evas_Object *obj;

   EINA_LIST_FOREACH(_elm_win_list, l, obj)
     evas_object_smart_callback_call(obj, SIG_IOERR, NULL);
   elm_exit();
}
#endif

static void
_elm_win_cb_hide(void *data __UNUSED__,
                 Evas *e __UNUSED__,
                 Evas_Object *obj __UNUSED__,
                 void *event_info __UNUSED__)
{
   _elm_win_state_eval_queue();
}

static void
_elm_win_cb_show(void *data __UNUSED__,
                 Evas *e __UNUSED__,
                 Evas_Object *obj __UNUSED__,
                 void *event_info __UNUSED__)
{
   _elm_win_state_eval_queue();
}

static Eina_Bool
_accel_is_gl(void)
{
   const char *env = NULL;
   const char *str = NULL;

   if (_elm_config->accel) str = _elm_config->accel;
   if (_elm_accel_preference) str = _elm_accel_preference;
   if ((_elm_config->accel_override) && (_elm_config->accel))
     str = _elm_config->accel;
   env = getenv("ELM_ACCEL");
   if (env) str = env;
   if ((str) &&
       ((!strcasecmp(str, "gl")) ||
        (!strcasecmp(str, "opengl")) ||
        (!strcasecmp(str, "3d")) ||
        (!strcasecmp(str, "hw")) ||
        (!strcasecmp(str, "accel")) ||
        (!strcasecmp(str, "hardware"))
       ))
     return EINA_TRUE;
   return EINA_FALSE;
}

EAPI Evas_Object *
elm_win_add(Evas_Object *parent,
            const char *name,
            Elm_Win_Type type)
{
   Evas *e;
   Evas_Object *obj;
   const Eina_List *l;
   const char *fontpath, *fallback = NULL;

   Elm_Win_Smart_Data tmp_sd;

   /* just to store some data while trying out to create a canvas */
   memset(&tmp_sd, 0, sizeof(Elm_Win_Smart_Data));

#define FALLBACK_TRY(engine)                                      \
  if (!tmp_sd.ee) {                                               \
     CRITICAL(engine " engine creation failed. Trying default."); \
     tmp_sd.ee = ecore_evas_new(NULL, 0, 0, 1, 1, NULL);          \
     if (tmp_sd.ee)                                               \
       elm_config_preferred_engine_set(ecore_evas_engine_name_get(tmp_sd.ee)); \
  } while (0)
#define FALLBACK_STORE(engine)                               \
   if (tmp_sd.ee)                                            \
   {                                                         \
      CRITICAL(engine "Fallback to %s successful.", engine); \
      fallback = engine;                                     \
   }

   switch (type)
     {
      case ELM_WIN_TIZEN_WIDGET:
        // Tizen Only(20131219): Dynamic Box is only supported in the Tizen
        if (!parent) return NULL;
          {
             e = evas_object_evas_get(parent);
             if (!e) return NULL;
             tmp_sd.ee = ecore_evas_ecore_evas_get(e);
             // Do not make any relations between this and the parent object.
             // It is just used for getting the "Ecore_Evas *"
             parent = NULL;
          }
        break;
      case ELM_WIN_INLINED_IMAGE:
        if (!parent) break;
        {
           e = evas_object_evas_get(parent);
           Ecore_Evas *ee;

           if (!e) break;

           ee = ecore_evas_ecore_evas_get(e);
           if (!ee) break;

           tmp_sd.img_obj = ecore_evas_object_image_new(ee);
           if (!tmp_sd.img_obj) break;

           tmp_sd.ee = ecore_evas_object_ecore_evas_get(tmp_sd.img_obj);
           if (!tmp_sd.ee)
             {
                evas_object_del(tmp_sd.img_obj);
                tmp_sd.img_obj = NULL;
             }
        }
        break;

      case ELM_WIN_SOCKET_IMAGE:
        tmp_sd.ee = ecore_evas_extn_socket_new(1, 1);
        break;

      default:
        if (ENGINE_COMPARE(ELM_SOFTWARE_X11))
          {
             tmp_sd.ee = ecore_evas_software_x11_new(NULL, 0, 0, 0, 1, 1);
             FALLBACK_TRY("Software X11");
             if (!tmp_sd.ee)
               {
                  tmp_sd.ee = ecore_evas_fb_new(NULL, 0, 1, 1);
                  FALLBACK_STORE("Software FB");
               }
          }
        else if (ENGINE_COMPARE(ELM_SOFTWARE_FB))
          {
             tmp_sd.ee = ecore_evas_fb_new(NULL, 0, 1, 1);
             FALLBACK_TRY("Software FB");
             if (!tmp_sd.ee)
               {
                  tmp_sd.ee = ecore_evas_software_x11_new(NULL, 0, 0, 0, 1, 1);
                  FALLBACK_STORE("Software X11");
               }
          }
        else if (ENGINE_COMPARE(ELM_SOFTWARE_DIRECTFB))
          {
             tmp_sd.ee = ecore_evas_directfb_new(NULL, 1, 0, 0, 1, 1);
             FALLBACK_TRY("Software DirectFB");
             if (!tmp_sd.ee)
               {
                  tmp_sd.ee = ecore_evas_software_x11_new(NULL, 0, 0, 0, 1, 1);
                  FALLBACK_STORE("Software X11");
                  if (!tmp_sd.ee)
                    {
                       tmp_sd.ee = ecore_evas_fb_new(NULL, 0, 1, 1);
                       FALLBACK_STORE("Software FB");
                    }
               }
          }
        else if (ENGINE_COMPARE(ELM_SOFTWARE_16_X11))
          {
             tmp_sd.ee = ecore_evas_software_x11_16_new(NULL, 0, 0, 0, 1, 1);
             FALLBACK_TRY("Software-16");
             if (!tmp_sd.ee)
               {
                  tmp_sd.ee = ecore_evas_software_x11_new(NULL, 0, 0, 0, 1, 1);
                  FALLBACK_STORE("Software X11");
                  if (!tmp_sd.ee)
                    {
                       tmp_sd.ee = ecore_evas_fb_new(NULL, 0, 1, 1);
                       FALLBACK_STORE("Software FB");
                    }
               }
          }
        else if (ENGINE_COMPARE(ELM_SOFTWARE_8_X11))
          {
             tmp_sd.ee = ecore_evas_software_x11_8_new(NULL, 0, 0, 0, 1, 1);
             FALLBACK_TRY("Software-8");
             if (!tmp_sd.ee)
               {
                  tmp_sd.ee = ecore_evas_software_x11_new(NULL, 0, 0, 0, 1, 1);
                  FALLBACK_STORE("Software X11");
                  if (!tmp_sd.ee)
                    {
                       tmp_sd.ee = ecore_evas_fb_new(NULL, 0, 1, 1);
                       FALLBACK_STORE("Software FB");
                    }
               }
          }
        else if (ENGINE_COMPARE(ELM_OPENGL_X11))
          {
             int opt[20];
             int opt_i = 0;

             if (_elm_config->vsync)
               {
                  opt[opt_i] = ECORE_EVAS_GL_X11_OPT_VSYNC;
                  opt_i++;
                  opt[opt_i] = 1;
                  opt_i++;
               }

             if (_elm_config->gl_depth)
               {
                  opt[opt_i] = ECORE_EVAS_GL_X11_OPT_GL_DEPTH;
                  opt_i++;
                  opt[opt_i] = _elm_config->gl_depth;
                  opt_i++;
               }

             if (_elm_config->gl_stencil)
               {
                  opt[opt_i] = ECORE_EVAS_GL_X11_OPT_GL_STENCIL;
                  opt_i++;
                  opt[opt_i] = _elm_config->gl_stencil;
                  opt_i++;
               }

             if (_elm_config->gl_msaa)
               {
                  opt[opt_i] = ECORE_EVAS_GL_X11_OPT_GL_MSAA;
                  opt_i++;
                  opt[opt_i] = _elm_config->gl_msaa;
                  opt_i++;
               }

             opt[opt_i] = 0;

             if (opt_i > 0)
               tmp_sd.ee = ecore_evas_gl_x11_options_new
                   (NULL, 0, 0, 0, 1, 1, opt);
             else
               tmp_sd.ee = ecore_evas_gl_x11_new(NULL, 0, 0, 0, 1, 1);
             FALLBACK_TRY("OpenGL");
             if (!tmp_sd.ee)
               {
                  tmp_sd.ee = ecore_evas_software_x11_new(NULL, 0, 0, 0, 1, 1);
                  FALLBACK_STORE("Software X11");
                  if (!tmp_sd.ee)
                    {
                       tmp_sd.ee = ecore_evas_fb_new(NULL, 0, 1, 1);
                       FALLBACK_STORE("Software FB");
                    }
               }
          }
        else if (ENGINE_COMPARE(ELM_SOFTWARE_WIN32))
          {
             tmp_sd.ee = ecore_evas_software_gdi_new(NULL, 0, 0, 1, 1);
             FALLBACK_TRY("Software Win32");
          }
        else if (ENGINE_COMPARE(ELM_SOFTWARE_16_WINCE))
          {
             tmp_sd.ee = ecore_evas_software_wince_gdi_new(NULL, 0, 0, 1, 1);
             FALLBACK_TRY("Software-16-WinCE");
          }
        else if (ENGINE_COMPARE(ELM_SOFTWARE_PSL1GHT))
          {
             tmp_sd.ee = ecore_evas_psl1ght_new(NULL, 1, 1);
             FALLBACK_TRY("PSL1GHT");
          }
        else if (ENGINE_COMPARE(ELM_SOFTWARE_SDL))
          {
             tmp_sd.ee = ecore_evas_sdl_new(NULL, 0, 0, 0, 0, 0, 1);
             FALLBACK_TRY("Software SDL");
             if (!tmp_sd.ee)
               {
                  tmp_sd.ee = ecore_evas_software_x11_new(NULL, 0, 0, 0, 1, 1);
                  FALLBACK_STORE("Software X11");
                  if (!tmp_sd.ee)
                    {
                       tmp_sd.ee = ecore_evas_fb_new(NULL, 0, 1, 1);
                       FALLBACK_STORE("Software FB");
                    }
               }
          }
        else if (ENGINE_COMPARE(ELM_SOFTWARE_16_SDL))
          {
             tmp_sd.ee = ecore_evas_sdl16_new(NULL, 0, 0, 0, 0, 0, 1);
             FALLBACK_TRY("Software-16-SDL");
             if (!tmp_sd.ee)
               {
                  tmp_sd.ee = ecore_evas_software_x11_new(NULL, 0, 0, 0, 1, 1);
                  FALLBACK_STORE("Software X11");
                  if (!tmp_sd.ee)
                    {
                       tmp_sd.ee = ecore_evas_fb_new(NULL, 0, 1, 1);
                       FALLBACK_STORE("Software FB");
                    }
               }
          }
        else if (ENGINE_COMPARE(ELM_OPENGL_SDL))
          {
             tmp_sd.ee = ecore_evas_gl_sdl_new(NULL, 1, 1, 0, 0);
             FALLBACK_TRY("OpenGL SDL");
             if (!tmp_sd.ee)
               {
                  tmp_sd.ee = ecore_evas_software_x11_new(NULL, 0, 0, 0, 1, 1);
                  FALLBACK_STORE("Software X11");
                  if (!tmp_sd.ee)
                    {
                       tmp_sd.ee = ecore_evas_fb_new(NULL, 0, 1, 1);
                       FALLBACK_STORE("Software FB");
                    }
               }
          }
        else if (ENGINE_COMPARE(ELM_OPENGL_COCOA))
          {
             tmp_sd.ee = ecore_evas_cocoa_new(NULL, 1, 1, 0, 0);
             FALLBACK_TRY("OpenGL Cocoa");
          }
        else if (ENGINE_COMPARE(ELM_BUFFER))
          {
             tmp_sd.ee = ecore_evas_buffer_new(1, 1);
          }
        else if (ENGINE_COMPARE(ELM_EWS))
          {
             tmp_sd.ee = ecore_evas_ews_new(0, 0, 1, 1);
          }
        else if (ENGINE_COMPARE(ELM_WAYLAND_SHM))
          {
             tmp_sd.ee = ecore_evas_wayland_shm_new(NULL, 0, 0, 0, 1, 1, 0);
          }
        else if (ENGINE_COMPARE(ELM_WAYLAND_EGL))
          {
             tmp_sd.ee = ecore_evas_wayland_egl_new(NULL, 0, 0, 0, 1, 1, 0);
          }
        else if (!strncmp(ENGINE_GET(), "shot:", 5))
          {
             tmp_sd.ee = ecore_evas_buffer_new(1, 1);
             ecore_evas_manual_render_set(tmp_sd.ee, EINA_TRUE);
             tmp_sd.shot.info = eina_stringshare_add
                 (ENGINE_GET() + 5);
          }
#undef FALLBACK_TRY
        break;
     }

   if (!tmp_sd.ee)
     {
        ERR("Cannot create window.");
        return NULL;
     }

   obj = evas_object_smart_add
       (ecore_evas_get(tmp_sd.ee), _elm_win_smart_class_new());

   // Tizen Only(20131219): Dynamic Box is only supported in the Tizen
   if (type == ELM_WIN_TIZEN_WIDGET)
     ecore_evas_data_set(tmp_sd.ee, "dynamic,box,win", obj);

   ELM_WIN_DATA_GET(obj, sd);

#ifdef ELM_FOCUSED_UI
   // TIZEN ONLY (20130422) : For automating default focused UI.
   sd->keyboard_attached = EINA_FALSE;
   sd->first_key_down = EINA_FALSE;
   //
#endif

   /* copying possibly altered fields back */
#define SD_CPY(_field)             \
  do                               \
    {                              \
       sd->_field = tmp_sd._field; \
    } while (0)

   SD_CPY(ee);
   SD_CPY(img_obj);
   SD_CPY(shot.info);
#undef SD_CPY

   if ((trap) && (trap->add))
     sd->trap_data = trap->add(obj);

   /* complementary actions, which depend on final smart data
    * pointer */
   if (type == ELM_WIN_INLINED_IMAGE)
     _win_inlined_image_set(sd);

#ifdef HAVE_ELEMENTARY_X
    else if (ENGINE_COMPARE(ELM_SOFTWARE_X11) ||
            ENGINE_COMPARE(ELM_SOFTWARE_16_X11) ||
            ENGINE_COMPARE(ELM_SOFTWARE_8_X11) ||
            ENGINE_COMPARE(ELM_OPENGL_X11))
     {
#ifdef ELM_FOCUSED_UI
        sd->x.mouse_down_handler = ecore_event_handler_add
            (ECORE_EVENT_MOUSE_BUTTON_DOWN, _elm_win_mouse_button_down, obj);
        sd->x.key_down_handler = ecore_event_handler_add
            (ECORE_EVENT_KEY_DOWN, _elm_win_key_down, obj);
        /////////////
#endif
        sd->x.client_message_handler = ecore_event_handler_add
            (ECORE_X_EVENT_CLIENT_MESSAGE, _elm_win_client_message, sd);
        sd->x.property_handler = ecore_event_handler_add
            (ECORE_X_EVENT_WINDOW_PROPERTY, _elm_win_property_change, sd);
     }
#endif

   else if (!strncmp(ENGINE_GET(), "shot:", 5))
     _shot_init(sd);

   sd->kbdmode = ELM_WIN_KEYBOARD_UNKNOWN;
   sd->indmode = ELM_WIN_INDICATOR_UNKNOWN;

#ifdef HAVE_ELEMENTARY_X
   _elm_win_xwindow_get(sd);
   if (sd->x.xwin)
     {
        ecore_x_io_error_handler_set(_elm_x_io_err, NULL);
     }
#endif

#ifdef HAVE_ELEMENTARY_WAYLAND
   sd->wl.win = ecore_evas_wayland_window_get(sd->ee);
#endif

   if ((_elm_config->bgpixmap)
#ifdef HAVE_ELEMENTARY_X
       &&
       (((sd->x.xwin) && (!ecore_x_screen_is_composited(0))) ||
           (!sd->x.xwin))
#endif
      )
     TRAP(sd, avoid_damage_set, ECORE_EVAS_AVOID_DAMAGE_EXPOSE);
   // bg pixmap done by x - has other issues like can be redrawn by x before it
   // is filled/ready by app
   //     TRAP(sd, avoid_damage_set, ECORE_EVAS_AVOID_DAMAGE_BUILT_IN);

   sd->type = type;
   sd->parent = parent;

   if (sd->parent)
     evas_object_event_callback_add
       (sd->parent, EVAS_CALLBACK_DEL, _elm_win_on_parent_del, sd);

   sd->evas = ecore_evas_get(sd->ee);

   evas_object_color_set(obj, 0, 0, 0, 0);
   evas_object_move(obj, 0, 0);
   evas_object_resize(obj, 1, 1);
   evas_object_layer_set(obj, 50);
   evas_object_pass_events_set(obj, EINA_TRUE);

   if (type == ELM_WIN_INLINED_IMAGE)
     elm_widget_parent2_set(obj, parent);

   /* use own version of ecore_evas_object_associate() that does TRAP() */
   ecore_evas_data_set(sd->ee, "elm_win", sd);

   evas_object_event_callback_add
     (obj, EVAS_CALLBACK_CHANGED_SIZE_HINTS,
      _elm_win_obj_callback_changed_size_hints, sd);

   evas_object_intercept_raise_callback_add
     (obj, _elm_win_obj_intercept_raise, sd);
   evas_object_intercept_lower_callback_add
     (obj, _elm_win_obj_intercept_lower, sd);
   evas_object_intercept_stack_above_callback_add
     (obj, _elm_win_obj_intercept_stack_above, sd);
   evas_object_intercept_stack_below_callback_add
     (obj, _elm_win_obj_intercept_stack_below, sd);
   evas_object_intercept_layer_set_callback_add
     (obj, _elm_win_obj_intercept_layer_set, sd);
   evas_object_intercept_show_callback_add
     (obj, _elm_win_obj_intercept_show, sd);
   evas_object_intercept_move_callback_add
     (obj, _ecore_evas_move_, sd);

   TRAP(sd, name_class_set, name, _elm_appname);
   ecore_evas_callback_delete_request_set(sd->ee, _elm_win_delete_request);
   ecore_evas_callback_resize_set(sd->ee, _elm_win_resize);
   ecore_evas_callback_mouse_in_set(sd->ee, _elm_win_mouse_in);
   ecore_evas_callback_focus_in_set(sd->ee, _elm_win_focus_in);
   ecore_evas_callback_focus_out_set(sd->ee, _elm_win_focus_out);
   ecore_evas_callback_move_set(sd->ee, _elm_win_move);
   ecore_evas_callback_state_change_set(sd->ee, _elm_win_state_change);
   evas_object_event_callback_add(obj, EVAS_CALLBACK_HIDE, _elm_win_cb_hide, sd);
   evas_object_event_callback_add(obj, EVAS_CALLBACK_SHOW, _elm_win_cb_show, sd);

   evas_image_cache_set(sd->evas, (_elm_config->image_cache * 1024));
   evas_font_cache_set(sd->evas, (_elm_config->font_cache * 1024));

   EINA_LIST_FOREACH(_elm_config->font_dirs, l, fontpath)
     evas_font_path_append(sd->evas, fontpath);

   if (!_elm_config->font_hinting)
     evas_font_hinting_set(sd->evas, EVAS_FONT_HINTING_NONE);
   else if (_elm_config->font_hinting == 1)
     evas_font_hinting_set(sd->evas, EVAS_FONT_HINTING_AUTO);
   else if (_elm_config->font_hinting == 2)
     evas_font_hinting_set(sd->evas, EVAS_FONT_HINTING_BYTECODE);

#ifdef HAVE_ELEMENTARY_X
   _elm_win_xwin_type_set(sd);
   _elm_win_xwin_update(sd);
#endif

   _elm_win_list = eina_list_append(_elm_win_list, obj);
   _elm_win_count++;

   if (((fallback) && (!strcmp(fallback, "Software FB"))) ||
       ((!fallback) && (ENGINE_COMPARE(ELM_SOFTWARE_FB))))
     {
        TRAP(sd, fullscreen_set, 1);
     }
   else if ((type != ELM_WIN_INLINED_IMAGE) &&
            (ENGINE_COMPARE(ELM_WAYLAND_SHM) ||
             ENGINE_COMPARE(ELM_WAYLAND_EGL)))
     _elm_win_frame_add(sd, "default");

   if (sd->parent)
     {
        if (_elm_widget_focus_highlighted_get(sd->parent))
          {
             DBG("-[WIN_FOCUS:%p] highlight_enabled", obj);
#ifdef ELM_FOCUSED_UI
             sd->keyboard_attached = EINA_TRUE;
#endif
             sd->focus_highlight.cur.visible = EINA_TRUE;
             elm_win_focus_highlight_enabled_set(obj, EINA_TRUE);
          }
     }
   else if (_elm_config->focus_highlight_enable)
     {
        DBG("-[WIN_FOCUS:%p] highlight_enabled", obj);
        elm_win_focus_highlight_enabled_set(obj, EINA_TRUE);
     }

#ifdef ELM_DEBUG
   Evas_Modifier_Mask mask = evas_key_modifier_mask_get(sd->evas, "Control");
   evas_object_event_callback_add
     (obj, EVAS_CALLBACK_KEY_DOWN, _debug_key_down, sd);

   if (evas_object_key_grab(obj, "F12", mask, 0, EINA_TRUE))
     INF("Ctrl+F12 key combination exclusive for dot tree generation\n");
   else
     ERR("failed to grab F12 key to elm widgets (dot) tree generation");
#endif

   if ((_elm_config->softcursor_mode == ELM_SOFTCURSOR_MODE_ON) ||
       ((_elm_config->softcursor_mode == ELM_SOFTCURSOR_MODE_AUTO) &&
           (((fallback) && (!strcmp(fallback, "Software FB"))) ||
               ((!fallback) && (ENGINE_COMPARE(ELM_SOFTWARE_FB))))))
     {
        Evas_Object *o;
        Evas_Coord mw = 1, mh = 1, hx = 0, hy = 0;

        sd->pointer.obj = o = edje_object_add(ecore_evas_get(tmp_sd.ee));
        _elm_theme_object_set(obj, o, "pointer", "base", "default");
        edje_object_size_min_calc(o, &mw, &mh);
        evas_object_resize(o, mw, mh);
        edje_object_part_geometry_get(o, "elm.swallow.hotspot",
                                      &hx, &hy, NULL, NULL);
        sd->pointer.hot_x = hx;
        sd->pointer.hot_y = hy;
        evas_object_show(o);
        ecore_evas_object_cursor_set(tmp_sd.ee, o, EVAS_LAYER_MAX, hx, hy);
     }
   else if (_elm_config->softcursor_mode == ELM_SOFTCURSOR_MODE_OFF)
     {
        // do nothing
     }

   sd->wm_rot.wm_supported = ecore_evas_wm_rotation_supported_get(sd->ee);
   sd->wm_rot.preferred_rot = -1; // it means that elm_win doesn't use preferred rotation.

   //Tizen Only: This should be removed when eo is applied.
   ELM_WIDGET_DATA_GET(obj, wd);
   wd->on_create = EINA_FALSE;

   // Tizen Only(20131219): Dynamic Box is only supported in the Tizen
   if (type == ELM_WIN_TIZEN_WIDGET)
      _elm_win_focus_in(tmp_sd.ee);

   // TIZEN_ONLY(20140822): Apply evas_bidi_direction_hint_set according to locale.
   if (!strcmp(E_("default:LTR"), "default:RTL"))
     {
        evas_bidi_direction_hint_set(evas_object_evas_get(obj), EVAS_BIDI_DIRECTION_RTL);
     }
   else
     {
        evas_bidi_direction_hint_set(evas_object_evas_get(obj), EVAS_BIDI_DIRECTION_LTR);
     }
   //

   return obj;
}

EAPI Elm_Win_Type
elm_win_type_get(Evas_Object *obj)
{
   ELM_WIN_CHECK(obj) ELM_WIN_UNKNOWN;
   ELM_WIN_DATA_GET_OR_RETURN_VAL(obj, sd, ELM_WIN_UNKNOWN);

   return sd->type;
}

EAPI Evas_Object *
elm_win_util_standard_add(const char *name,
                          const char *title)
{
   Evas_Object *win, *bg;

   win = elm_win_add(NULL, name, ELM_WIN_BASIC);
   if (!win) return NULL;

   elm_win_title_set(win, title);
   bg = elm_bg_add(win);
   if (!bg)
     {
        evas_object_del(win);
        return NULL;
     }
   evas_object_size_hint_weight_set(bg, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_win_resize_object_add(win, bg);
   evas_object_show(bg);

   return win;
}

EAPI void
elm_win_resize_object_add(Evas_Object *obj,
                          Evas_Object *subobj)
{
   Evas_Coord w, h;

   ELM_WIN_CHECK(obj);
   ELM_WIN_DATA_GET_OR_RETURN(obj, sd);

   if (eina_list_data_find(sd->resize_objs, subobj)) return;

   if (!ELM_WIDGET_DATA(sd)->api->sub_object_add(obj, subobj))
     ERR("could not add %p as sub object of %p", subobj, obj);

   sd->resize_objs = eina_list_append(sd->resize_objs, subobj);

   evas_object_event_callback_add
     (subobj, EVAS_CALLBACK_DEL, _elm_win_on_resize_obj_del, obj);
   evas_object_event_callback_add
     (subobj, EVAS_CALLBACK_CHANGED_SIZE_HINTS,
     _elm_win_on_resize_obj_changed_size_hints, obj);

   evas_object_geometry_get(obj, NULL, NULL, &w, &h);
   evas_object_move(subobj, 0, 0);
   evas_object_resize(subobj, w, h);

   _elm_win_resize_objects_eval(obj);
}

EAPI void
elm_win_resize_object_del(Evas_Object *obj,
                          Evas_Object *subobj)
{
   ELM_WIN_CHECK(obj);
   ELM_WIN_DATA_GET_OR_RETURN(obj, sd);

   if (!ELM_WIDGET_DATA(sd)->api->sub_object_del(obj, subobj))
     ERR("could not remove sub object %p from %p", subobj, obj);

   sd->resize_objs = eina_list_remove(sd->resize_objs, subobj);

   evas_object_event_callback_del_full
     (subobj, EVAS_CALLBACK_CHANGED_SIZE_HINTS,
     _elm_win_on_resize_obj_changed_size_hints, obj);
   evas_object_event_callback_del_full
     (subobj, EVAS_CALLBACK_DEL, _elm_win_on_resize_obj_del, obj);

   _elm_win_resize_objects_eval(obj);
}

EAPI void
elm_win_title_set(Evas_Object *obj,
                  const char *title)
{
   ELM_WIN_CHECK(obj);
   ELM_WIN_DATA_GET_OR_RETURN(obj, sd);

   if (!title) return;
   eina_stringshare_replace(&(sd->title), title);
   TRAP(sd, title_set, sd->title);
   if (sd->frame_obj)
     edje_object_part_text_escaped_set
       (sd->frame_obj, "elm.text.title", sd->title);
}

EAPI const char *
elm_win_title_get(const Evas_Object *obj)
{
   ELM_WIN_CHECK(obj) NULL;
   ELM_WIN_DATA_GET_OR_RETURN_VAL(obj, sd, NULL);

   return sd->title;
}

EAPI void
elm_win_icon_name_set(Evas_Object *obj,
                      const char *icon_name)
{
   ELM_WIN_CHECK(obj);
   ELM_WIN_DATA_GET_OR_RETURN(obj, sd);

   if (!icon_name) return;
   eina_stringshare_replace(&(sd->icon_name), icon_name);
#ifdef HAVE_ELEMENTARY_X
   _elm_win_xwin_update(sd);
#endif
}

EAPI const char *
elm_win_icon_name_get(const Evas_Object *obj)
{
   ELM_WIN_CHECK(obj) NULL;
   ELM_WIN_DATA_GET_OR_RETURN_VAL(obj, sd, NULL);

   return sd->icon_name;
}

EAPI void
elm_win_role_set(Evas_Object *obj, const char *role)
{
   ELM_WIN_CHECK(obj);
   ELM_WIN_DATA_GET_OR_RETURN(obj, sd);

   if (!role) return;
   eina_stringshare_replace(&(sd->role), role);
#ifdef HAVE_ELEMENTARY_X
   _elm_win_xwin_update(sd);
#endif
}

EAPI const char *
elm_win_role_get(const Evas_Object *obj)
{
   ELM_WIN_CHECK(obj) NULL;
   ELM_WIN_DATA_GET_OR_RETURN_VAL(obj, sd, NULL);

   return sd->role;
}

EAPI void
elm_win_icon_object_set(Evas_Object *obj,
                        Evas_Object *icon)
{
   ELM_WIN_CHECK(obj);
   ELM_WIN_DATA_GET_OR_RETURN(obj, sd);

   if (sd->icon)
     evas_object_event_callback_del_full
       (sd->icon, EVAS_CALLBACK_DEL, _elm_win_on_icon_del, sd);
   sd->icon = icon;
   if (sd->icon)
     evas_object_event_callback_add
       (sd->icon, EVAS_CALLBACK_DEL, _elm_win_on_icon_del, sd);
#ifdef HAVE_ELEMENTARY_X
   _elm_win_xwin_update(sd);
#endif
}

EAPI const Evas_Object *
elm_win_icon_object_get(const Evas_Object *obj)
{
   ELM_WIN_CHECK(obj) NULL;
   ELM_WIN_DATA_GET_OR_RETURN_VAL(obj, sd, NULL);

   return sd->icon;
}

EAPI void
elm_win_autodel_set(Evas_Object *obj,
                    Eina_Bool autodel)
{
   ELM_WIN_CHECK(obj);
   ELM_WIN_DATA_GET_OR_RETURN(obj, sd);

   sd->autodel = autodel;
}

EAPI Eina_Bool
elm_win_autodel_get(const Evas_Object *obj)
{
   ELM_WIN_CHECK(obj) EINA_FALSE;
   ELM_WIN_DATA_GET_OR_RETURN_VAL(obj, sd, EINA_FALSE);

   return sd->autodel;
}

EAPI void
elm_win_activate(Evas_Object *obj)
{
   ELM_WIN_CHECK(obj);
   ELM_WIN_DATA_GET_OR_RETURN(obj, sd);

   TRAP(sd, activate);
}

EAPI void
elm_win_lower(Evas_Object *obj)
{
   ELM_WIN_CHECK(obj);
   ELM_WIN_DATA_GET_OR_RETURN(obj, sd);

   TRAP(sd, lower);
}

EAPI void
elm_win_raise(Evas_Object *obj)
{
   ELM_WIN_CHECK(obj);
   ELM_WIN_DATA_GET_OR_RETURN(obj, sd);

   TRAP(sd, raise);
}

EAPI void
elm_win_center(Evas_Object *obj,
               Eina_Bool h,
               Eina_Bool v)
{
   int win_w, win_h, screen_w, screen_h, nx, ny;

   ELM_WIN_CHECK(obj);
   ELM_WIN_DATA_GET_OR_RETURN(obj, sd);

   if ((trap) && (trap->center) && (!trap->center(sd->trap_data, obj)))
     return;

   ecore_evas_screen_geometry_get(sd->ee, NULL, NULL, &screen_w, &screen_h);
   if ((!screen_w) || (!screen_h)) return;

   evas_object_geometry_get(obj, NULL, NULL, &win_w, &win_h);
   if ((!win_w) || (!win_h)) return;

   if (h) nx = win_w >= screen_w ? 0 : (screen_w / 2) - (win_w / 2);
   else nx = sd->screen.x;
   if (v) ny = win_h >= screen_h ? 0 : (screen_h / 2) - (win_h / 2);
   else ny = sd->screen.y;
   if (nx < 0) nx = 0;
   if (ny < 0) ny = 0;

   evas_object_move(obj, nx, ny);
}

EAPI void
elm_win_borderless_set(Evas_Object *obj,
                       Eina_Bool borderless)
{
   ELM_WIN_CHECK(obj);
   ELM_WIN_DATA_GET_OR_RETURN(obj, sd);

   TRAP(sd, borderless_set, borderless);
#ifdef HAVE_ELEMENTARY_X
   _elm_win_xwin_update(sd);
#endif
}

EAPI Eina_Bool
elm_win_borderless_get(const Evas_Object *obj)
{
   ELM_WIN_CHECK(obj) EINA_FALSE;
   ELM_WIN_DATA_GET_OR_RETURN_VAL(obj, sd, EINA_FALSE);

   return ecore_evas_borderless_get(sd->ee);
}

EAPI void
elm_win_shaped_set(Evas_Object *obj,
                   Eina_Bool shaped)
{
   ELM_WIN_CHECK(obj);
   ELM_WIN_DATA_GET_OR_RETURN(obj, sd);

   TRAP(sd, shaped_set, shaped);
#ifdef HAVE_ELEMENTARY_X
   _elm_win_xwin_update(sd);
#endif
}

EAPI Eina_Bool
elm_win_shaped_get(const Evas_Object *obj)
{
   ELM_WIN_CHECK(obj) EINA_FALSE;
   ELM_WIN_DATA_GET_OR_RETURN_VAL(obj, sd, EINA_FALSE);

   return ecore_evas_shaped_get(sd->ee);
}

EAPI void
elm_win_alpha_set(Evas_Object *obj,
                  Eina_Bool alpha)
{
   ELM_WIN_CHECK(obj);
   ELM_WIN_DATA_GET_OR_RETURN(obj, sd);

   if (sd->img_obj)
     {
        evas_object_image_alpha_set(sd->img_obj, alpha);
        ecore_evas_alpha_set(sd->ee, alpha);
     }
   else
     {
#ifdef HAVE_ELEMENTARY_X
        if (sd->x.xwin)
          {
             if (alpha)
               {
                  if (!ecore_x_screen_is_composited(0))
                    elm_win_shaped_set(obj, alpha);
                  else
                    TRAP(sd, alpha_set, alpha);
               }
             else
               TRAP(sd, alpha_set, alpha);
             _elm_win_xwin_type_set(sd);
             _elm_win_xwin_update(sd);
          }
        else
#endif
          TRAP(sd, alpha_set, alpha);
     }
}

EAPI Eina_Bool
elm_win_alpha_get(const Evas_Object *obj)
{
   ELM_WIN_CHECK(obj) EINA_FALSE;
   ELM_WIN_DATA_GET_OR_RETURN_VAL(obj, sd, EINA_FALSE);

   if (sd->img_obj)
     {
        return evas_object_image_alpha_get(sd->img_obj);
     }

   return ecore_evas_alpha_get(sd->ee);
}

EAPI void
elm_win_override_set(Evas_Object *obj,
                     Eina_Bool override)
{
   ELM_WIN_CHECK(obj);
   ELM_WIN_DATA_GET_OR_RETURN(obj, sd);

   TRAP(sd, override_set, override);
#ifdef HAVE_ELEMENTARY_X
   _elm_win_xwin_update(sd);
#endif
}

EAPI Eina_Bool
elm_win_override_get(const Evas_Object *obj)
{
   ELM_WIN_CHECK(obj) EINA_FALSE;
   ELM_WIN_DATA_GET_OR_RETURN_VAL(obj, sd, EINA_FALSE);

   return ecore_evas_override_get(sd->ee);
}

EAPI void
elm_win_fullscreen_set(Evas_Object *obj,
                       Eina_Bool fullscreen)
{
   ELM_WIN_CHECK(obj);
   ELM_WIN_DATA_GET_OR_RETURN(obj, sd);
   // YYY: handle if sd->img_obj
   if (ENGINE_COMPARE(ELM_SOFTWARE_FB) ||
       ENGINE_COMPARE(ELM_SOFTWARE_16_WINCE))
     {
        // these engines... can ONLY be fullscreen
        return;
     }
   else
     {
//        sd->fullscreen = fullscreen;

        if (fullscreen)
          {
             if (ENGINE_COMPARE(ELM_WAYLAND_SHM) ||
                 ENGINE_COMPARE(ELM_WAYLAND_EGL))
               _elm_win_frame_del(sd);
          }
        else
          {
             if (ENGINE_COMPARE(ELM_WAYLAND_SHM) ||
                 ENGINE_COMPARE(ELM_WAYLAND_EGL))
               _elm_win_frame_add(sd, "default");

             evas_object_show(sd->frame_obj);
          }

        TRAP(sd, fullscreen_set, fullscreen);
#ifdef HAVE_ELEMENTARY_X
        _elm_win_xwin_update(sd);
#endif
     }
}

EAPI Eina_Bool
elm_win_fullscreen_get(const Evas_Object *obj)
{
   ELM_WIN_CHECK(obj) EINA_FALSE;
   ELM_WIN_DATA_GET_OR_RETURN_VAL(obj, sd, EINA_FALSE);

   if (ENGINE_COMPARE(ELM_SOFTWARE_FB) ||
       ENGINE_COMPARE(ELM_SOFTWARE_16_WINCE))
     {
        // these engines... can ONLY be fullscreen
        return EINA_TRUE;
     }
   else
     {
        return sd->fullscreen;
     }
}

EAPI void
elm_win_maximized_set(Evas_Object *obj,
                      Eina_Bool maximized)
{
   ELM_WIN_CHECK(obj);
   ELM_WIN_DATA_GET_OR_RETURN(obj, sd);

//   sd->maximized = maximized;
   // YYY: handle if sd->img_obj
   TRAP(sd, maximized_set, maximized);
#ifdef HAVE_ELEMENTARY_X
   _elm_win_xwin_update(sd);
#endif
}

EAPI Eina_Bool
elm_win_maximized_get(const Evas_Object *obj)
{
   ELM_WIN_CHECK(obj) EINA_FALSE;
   ELM_WIN_DATA_GET_OR_RETURN_VAL(obj, sd, EINA_FALSE);

   return sd->maximized;
}

EAPI void
elm_win_iconified_set(Evas_Object *obj,
                      Eina_Bool iconified)
{
   ELM_WIN_CHECK(obj);
   ELM_WIN_DATA_GET_OR_RETURN(obj, sd);

//   sd->iconified = iconified;
   TRAP(sd, iconified_set, iconified);
#ifdef HAVE_ELEMENTARY_X
   _elm_win_xwin_update(sd);
#endif
}

EAPI Eina_Bool
elm_win_iconified_get(const Evas_Object *obj)
{
   ELM_WIN_CHECK(obj) EINA_FALSE;
   ELM_WIN_DATA_GET_OR_RETURN_VAL(obj, sd, EINA_FALSE);

   return sd->iconified;
}

EAPI void
elm_win_withdrawn_set(Evas_Object *obj,
                      Eina_Bool withdrawn)
{
   ELM_WIN_CHECK(obj);
   ELM_WIN_DATA_GET_OR_RETURN(obj, sd);

//   sd->withdrawn = withdrawn;
   TRAP(sd, withdrawn_set, withdrawn);
#ifdef HAVE_ELEMENTARY_X
   _elm_win_xwin_update(sd);
#endif
}

EAPI Eina_Bool
elm_win_withdrawn_get(const Evas_Object *obj)
{
   ELM_WIN_CHECK(obj) EINA_FALSE;
   ELM_WIN_DATA_GET_OR_RETURN_VAL(obj, sd, EINA_FALSE);

   return sd->withdrawn;
}

EAPI void
elm_win_profiles_set(Evas_Object  *obj,
                     const char  **profiles,
                     unsigned int  num_profiles)
{
   char **profiles_int;
   const char *str;
   unsigned int i, num;
   Eina_List *l;

   ELM_WIN_CHECK(obj);
   ELM_WIN_DATA_GET_OR_RETURN(obj, sd);

   if (!profiles) return;

   if (sd->profile.timer) ecore_timer_del(sd->profile.timer);
   sd->profile.timer = ecore_timer_add(0.1, _elm_win_profile_change_delay, sd);
   EINA_LIST_FREE(sd->profile.names, str) eina_stringshare_del(str);

   for (i = 0; i < num_profiles; i++)
     {
        if ((profiles[i]) &&
            _elm_config_profile_exists(profiles[i]))
          {
             str = eina_stringshare_add(profiles[i]);
             sd->profile.names = eina_list_append(sd->profile.names, str);
          }
     }

   num = eina_list_count(sd->profile.names);
   profiles_int = alloca(num * sizeof(char *));

   if (profiles_int)
     {
        i = 0;
        EINA_LIST_FOREACH(sd->profile.names, l, str)
          {
             if (str)
               profiles_int[i] = strdup(str);
             else
               profiles_int[i] = NULL;
             i++;
          }
        ecore_evas_profiles_set(sd->ee, (const char **)profiles_int, i);
        for (i = 0; i < num; i++)
          {
             if (profiles_int[i]) free(profiles_int[i]);
          }
     }
   else
     ecore_evas_profiles_set(sd->ee, profiles, num_profiles);
}

EAPI const char *
elm_win_profile_get(const Evas_Object *obj)
{
   ELM_WIN_CHECK(obj) NULL;
   ELM_WIN_DATA_GET_OR_RETURN_VAL(obj, sd, NULL);

   return sd->profile.name;
}

EAPI void
elm_win_urgent_set(Evas_Object *obj,
                   Eina_Bool urgent)
{
   ELM_WIN_CHECK(obj);
   ELM_WIN_DATA_GET_OR_RETURN(obj, sd);

   sd->urgent = urgent;
   TRAP(sd, urgent_set, urgent);
#ifdef HAVE_ELEMENTARY_X
   _elm_win_xwin_update(sd);
#endif
}

EAPI Eina_Bool
elm_win_urgent_get(const Evas_Object *obj)
{
   ELM_WIN_CHECK(obj) EINA_FALSE;
   ELM_WIN_DATA_GET_OR_RETURN_VAL(obj, sd, EINA_FALSE);

   return sd->urgent;
}

EAPI void
elm_win_demand_attention_set(Evas_Object *obj,
                             Eina_Bool demand_attention)
{
   ELM_WIN_CHECK(obj);
   ELM_WIN_DATA_GET_OR_RETURN(obj, sd);

   sd->demand_attention = demand_attention;
   TRAP(sd, demand_attention_set, demand_attention);
#ifdef HAVE_ELEMENTARY_X
   _elm_win_xwin_update(sd);
#endif
}

EAPI Eina_Bool
elm_win_demand_attention_get(const Evas_Object *obj)
{
   ELM_WIN_CHECK(obj) EINA_FALSE;
   ELM_WIN_DATA_GET_OR_RETURN_VAL(obj, sd, EINA_FALSE);

   return sd->demand_attention;
}

EAPI void
elm_win_modal_set(Evas_Object *obj,
                  Eina_Bool modal)
{
   ELM_WIN_CHECK(obj);
   ELM_WIN_DATA_GET_OR_RETURN(obj, sd);

   sd->modal = modal;
   TRAP(sd, modal_set, modal);
#ifdef HAVE_ELEMENTARY_X
   _elm_win_xwin_update(sd);
#endif
}

EAPI Eina_Bool
elm_win_modal_get(const Evas_Object *obj)
{
   ELM_WIN_CHECK(obj) EINA_FALSE;
   ELM_WIN_DATA_GET_OR_RETURN_VAL(obj, sd, EINA_FALSE);

   return sd->modal;
}

EAPI void
elm_win_aspect_set(Evas_Object *obj,
                   double aspect)
{
   ELM_WIN_CHECK(obj);
   ELM_WIN_DATA_GET_OR_RETURN(obj, sd);

   sd->aspect = aspect;
   TRAP(sd, aspect_set, aspect);
#ifdef HAVE_ELEMENTARY_X
   _elm_win_xwin_update(sd);
#endif
}

EAPI double
elm_win_aspect_get(const Evas_Object *obj)
{
   ELM_WIN_CHECK(obj) 0.0;
   ELM_WIN_DATA_GET_OR_RETURN_VAL(obj, sd, 0.0);

   return sd->aspect;
}

EAPI void
elm_win_size_base_set(Evas_Object *obj, int w, int h)
{
   ELM_WIN_CHECK(obj);
   ELM_WIN_DATA_GET_OR_RETURN(obj, sd);
   sd->size_base_w = w;
   sd->size_base_h = h;
   TRAP(sd, size_base_set, w, h);
#ifdef HAVE_ELEMENTARY_X
   _elm_win_xwin_update(sd);
#endif
}

EAPI void
elm_win_size_base_get(Evas_Object *obj, int *w, int *h)
{
   ELM_WIN_CHECK(obj);
   ELM_WIN_DATA_GET_OR_RETURN(obj, sd);
   if (w) *w = sd->size_base_w;
   if (h) *h = sd->size_base_h;
}

EAPI void
elm_win_size_step_set(Evas_Object *obj, int w, int h)
{
   ELM_WIN_CHECK(obj);
   ELM_WIN_DATA_GET_OR_RETURN(obj, sd);
   sd->size_step_w = w;
   sd->size_step_h = h;
   TRAP(sd, size_step_set, w, h);
#ifdef HAVE_ELEMENTARY_X
   _elm_win_xwin_update(sd);
#endif
}

EAPI void
elm_win_size_step_get(Evas_Object *obj, int *w, int *h)
{
   ELM_WIN_CHECK(obj);
   ELM_WIN_DATA_GET_OR_RETURN(obj, sd);
   if (w) *w = sd->size_step_w;
   if (h) *h = sd->size_step_h;
}

EAPI void
elm_win_layer_set(Evas_Object *obj,
                  int layer)
{
   ELM_WIN_CHECK(obj);
   ELM_WIN_DATA_GET_OR_RETURN(obj, sd);

   TRAP(sd, layer_set, layer);
#ifdef HAVE_ELEMENTARY_X
   _elm_win_xwin_update(sd);
#endif
}

EAPI int
elm_win_layer_get(const Evas_Object *obj)
{
   ELM_WIN_CHECK(obj) - 1;
   ELM_WIN_DATA_GET_OR_RETURN_VAL(obj, sd, -1);

   return ecore_evas_layer_get(sd->ee);
}

EAPI void
elm_win_norender_push(Evas_Object *obj)
{
   ELM_WIN_CHECK(obj);
   ELM_WIN_DATA_GET_OR_RETURN(obj, sd);

   sd->norender++;
   if (sd->norender == 1) ecore_evas_manual_render_set(sd->ee, EINA_TRUE);
}

EAPI void
elm_win_norender_pop(Evas_Object *obj)
{
   ELM_WIN_CHECK(obj);
   ELM_WIN_DATA_GET_OR_RETURN(obj, sd);

   if (sd->norender <= 0) return;
   sd->norender--;
   if (sd->norender == 0) ecore_evas_manual_render_set(sd->ee, EINA_FALSE);
}

EAPI int
elm_win_norender_get(Evas_Object *obj)
{
   ELM_WIN_CHECK(obj) - 1;
   ELM_WIN_DATA_GET_OR_RETURN_VAL(obj, sd, -1);
   return sd->norender;
}

EAPI void
elm_win_render(Evas_Object *obj)
{
   ELM_WIN_CHECK(obj);
   ELM_WIN_DATA_GET_OR_RETURN(obj, sd);
   ecore_evas_manual_render(sd->ee);
}

static int
_win_rotation_degree_check(int rotation)
{
   if ((rotation > 360) || (rotation < 0))
     {
        WRN("Rotation degree should be 0 ~ 360 (passed degree: %d)", rotation);
        rotation %= 360;
        if (rotation < 0) rotation += 360;
     }
   return rotation;
}

static void
_win_rotate(Evas_Object *obj, Elm_Win_Smart_Data *sd, int rotation, Eina_Bool resize)
{
   rotation = _win_rotation_degree_check(rotation);
   if (sd->rot == rotation) return;
   sd->rot = rotation;
   if (resize) TRAP(sd, rotation_with_resize_set, rotation);
   else TRAP(sd, rotation_set, rotation);
   evas_object_size_hint_min_set(obj, -1, -1);
   evas_object_size_hint_max_set(obj, -1, -1);
   _elm_win_resize_objects_eval(obj);
#ifdef HAVE_ELEMENTARY_X
   _elm_win_xwin_update(sd);
#endif
   _rotation_say(obj, rotation);
   elm_widget_orientation_set(obj, rotation);
   evas_object_smart_callback_call(obj, SIG_ROTATION_CHANGED, NULL);
}

EAPI void
elm_win_rotation_set(Evas_Object *obj,
                     int rotation)
{
   ELM_WIN_CHECK(obj);
   ELM_WIN_DATA_GET_OR_RETURN(obj, sd);
   _win_rotate(obj, sd, rotation, EINA_FALSE);
}

EAPI void
elm_win_rotation_with_resize_set(Evas_Object *obj,
                                 int rotation)
{
   ELM_WIN_CHECK(obj);
   ELM_WIN_DATA_GET_OR_RETURN(obj, sd);
   _win_rotate(obj, sd, rotation, EINA_TRUE);
}

EAPI int
elm_win_rotation_get(const Evas_Object *obj)
{
   ELM_WIN_CHECK(obj) - 1;
   ELM_WIN_DATA_GET_OR_RETURN_VAL(obj, sd, -1);

   return sd->rot;
}

EAPI Eina_Bool
elm_win_wm_rotation_supported_get(const Evas_Object *obj)
{
   ELM_WIN_CHECK(obj) EINA_FALSE;
   ELM_WIN_DATA_GET_OR_RETURN_VAL(obj, sd, EINA_FALSE);
   return sd->wm_rot.wm_supported;
}

/* This will unset a preferred rotation, if given preferred rotation is '-1'.
 */
EAPI void
elm_win_wm_rotation_preferred_rotation_set(Evas_Object *obj,
                                           const int rotation)
{
   int rot;

   ELM_WIN_CHECK(obj);
   ELM_WIN_DATA_GET_OR_RETURN(obj, sd);

   if (!sd->wm_rot.use)
     sd->wm_rot.use = EINA_TRUE;

   // '-1' means that elm_win doesn't use preferred rotation.
   if (rotation == -1)
     rot = -1;
   else
     rot = _win_rotation_degree_check(rotation);

   if (sd->wm_rot.preferred_rot == rot) return;
   sd->wm_rot.preferred_rot = rot;

   ecore_evas_wm_rotation_preferred_rotation_set(sd->ee, rot);
}

EAPI int
elm_win_wm_rotation_preferred_rotation_get(const Evas_Object *obj)
{
   ELM_WIN_CHECK(obj) -1;
   ELM_WIN_DATA_GET_OR_RETURN_VAL(obj, sd, -1);
   if (!sd->wm_rot.use) return -1;
   return sd->wm_rot.preferred_rot;
}

EAPI void
elm_win_wm_rotation_available_rotations_set(Evas_Object *obj,
                                            const int   *rotations,
                                            unsigned int count)
{
   unsigned int i;
   int r;

   ELM_WIN_CHECK(obj);
   ELM_WIN_DATA_GET_OR_RETURN(obj, sd);

   if (!sd->wm_rot.use)
     sd->wm_rot.use = EINA_TRUE;

   if (sd->wm_rot.rots) free(sd->wm_rot.rots);

   sd->wm_rot.rots = NULL;
   sd->wm_rot.count = 0;

   if (count > 0)
     {
        sd->wm_rot.rots = calloc(count, sizeof(int));
        if (!sd->wm_rot.rots) return;
        for (i = 0; i < count; i++)
          {
             r = _win_rotation_degree_check(rotations[i]);
             sd->wm_rot.rots[i] = r;
          }
     }

   sd->wm_rot.count = count;

   ecore_evas_wm_rotation_available_rotations_set(sd->ee,
                                                  sd->wm_rot.rots,
                                                  sd->wm_rot.count);
}

EAPI Eina_Bool
elm_win_wm_rotation_available_rotations_get(const Evas_Object *obj,
                                            int              **rotations,
                                            unsigned int      *count)
{
   ELM_WIN_CHECK(obj) EINA_FALSE;
   ELM_WIN_DATA_GET_OR_RETURN_VAL(obj, sd, EINA_FALSE);
   if (!sd->wm_rot.use) return EINA_FALSE;

   if (sd->wm_rot.count > 0)
     {
        if (rotations)
          {
             *rotations = calloc(sd->wm_rot.count, sizeof(int));
             if (*rotations)
               {
                  memcpy(*rotations,
                         sd->wm_rot.rots,
                         sizeof(int) * sd->wm_rot.count);
               }
          }
     }

   if (count) *count = sd->wm_rot.count;

   return EINA_TRUE;
}

EAPI void
elm_win_wm_rotation_manual_rotation_done_set(Evas_Object *obj,
                                             Eina_Bool    set)
{
   ELM_WIN_CHECK(obj);
   ELM_WIN_DATA_GET_OR_RETURN(obj, sd);

   if (!sd->wm_rot.use) return;

   ecore_evas_wm_rotation_manual_rotation_done_set(sd->ee, set);
}

EAPI Eina_Bool
elm_win_wm_rotation_manual_rotation_done_get(const Evas_Object *obj)
{
   ELM_WIN_CHECK(obj) EINA_FALSE;
   ELM_WIN_DATA_GET_OR_RETURN_VAL(obj, sd, EINA_FALSE);

   if (!sd->wm_rot.use) return EINA_FALSE;

   return ecore_evas_wm_rotation_manual_rotation_done_get(sd->ee);
}

EAPI void
elm_win_wm_rotation_manual_rotation_done(Evas_Object *obj)
{
   ELM_WIN_CHECK(obj);
   ELM_WIN_DATA_GET_OR_RETURN(obj, sd);

   if (!sd->wm_rot.use) return;

   ecore_evas_wm_rotation_manual_rotation_done(sd->ee);
}

EAPI void
elm_win_sticky_set(Evas_Object *obj,
                   Eina_Bool sticky)
{
   ELM_WIN_CHECK(obj);
   ELM_WIN_DATA_GET_OR_RETURN(obj, sd);

//   sd->sticky = sticky;
   TRAP(sd, sticky_set, sticky);
#ifdef HAVE_ELEMENTARY_X
   _elm_win_xwin_update(sd);
#endif
}

EAPI Eina_Bool
elm_win_sticky_get(const Evas_Object *obj)
{
   ELM_WIN_CHECK(obj) EINA_FALSE;
   ELM_WIN_DATA_GET_OR_RETURN_VAL(obj, sd, EINA_FALSE);

   return sd->sticky;
}

EAPI void
elm_win_keyboard_mode_set(Evas_Object *obj,
                          Elm_Win_Keyboard_Mode mode)
{
   ELM_WIN_CHECK(obj);
   ELM_WIN_DATA_GET_OR_RETURN(obj, sd);

   if (mode == sd->kbdmode) return;
#ifdef HAVE_ELEMENTARY_X
   _elm_win_xwindow_get(sd);
#endif
   sd->kbdmode = mode;
#ifdef HAVE_ELEMENTARY_X
   if (sd->x.xwin)
     ecore_x_e_virtual_keyboard_state_set
       (sd->x.xwin, (Ecore_X_Virtual_Keyboard_State)sd->kbdmode);
#endif
}

EAPI Elm_Win_Keyboard_Mode
elm_win_keyboard_mode_get(const Evas_Object *obj)
{
   ELM_WIN_CHECK(obj) ELM_WIN_KEYBOARD_UNKNOWN;
   ELM_WIN_DATA_GET_OR_RETURN_VAL(obj, sd, ELM_WIN_KEYBOARD_UNKNOWN);

   return sd->kbdmode;
}

EAPI void
elm_win_keyboard_win_set(Evas_Object *obj,
                         Eina_Bool is_keyboard)
{
   ELM_WIN_CHECK(obj);
   ELM_WIN_DATA_GET_OR_RETURN(obj, sd);

#ifdef HAVE_ELEMENTARY_X
   _elm_win_xwindow_get(sd);
   if (sd->x.xwin)
     ecore_x_e_virtual_keyboard_set(sd->x.xwin, is_keyboard);
#else
   (void)is_keyboard;
#endif
}

EAPI Eina_Bool
elm_win_keyboard_win_get(const Evas_Object *obj)
{
   ELM_WIN_CHECK(obj) EINA_FALSE;
   ELM_WIN_DATA_GET_OR_RETURN_VAL(obj, sd, EINA_FALSE);

#ifdef HAVE_ELEMENTARY_X
   _elm_win_xwindow_get(sd);
   if (sd->x.xwin)
     return ecore_x_e_virtual_keyboard_get(sd->x.xwin);
#endif
   return EINA_FALSE;
}

EAPI void
elm_win_indicator_mode_set(Evas_Object *obj,
                           Elm_Win_Indicator_Mode mode)
{
   ELM_WIN_CHECK(obj);
   ELM_WIN_DATA_GET_OR_RETURN(obj, sd);

   if (mode == sd->indmode) return;
#ifdef HAVE_ELEMENTARY_X
   _elm_win_xwindow_get(sd);
#endif
   sd->indmode = mode;
#ifdef HAVE_ELEMENTARY_X
   if (sd->x.xwin)
     {
        if (sd->indmode == ELM_WIN_INDICATOR_SHOW)
          ecore_x_e_illume_indicator_state_set
            (sd->x.xwin, ECORE_X_ILLUME_INDICATOR_STATE_ON);
        else if (sd->indmode == ELM_WIN_INDICATOR_HIDE)
          ecore_x_e_illume_indicator_state_set
            (sd->x.xwin, ECORE_X_ILLUME_INDICATOR_STATE_OFF);
     }
#endif
   evas_object_smart_callback_call(obj, SIG_INDICATOR_PROP_CHANGED, NULL);
}

EAPI Elm_Win_Indicator_Mode
elm_win_indicator_mode_get(const Evas_Object *obj)
{
   ELM_WIN_CHECK(obj) ELM_WIN_INDICATOR_UNKNOWN;
   ELM_WIN_DATA_GET_OR_RETURN_VAL(obj, sd, ELM_WIN_INDICATOR_UNKNOWN);

   return sd->indmode;
}

EAPI void
elm_win_indicator_opacity_set(Evas_Object *obj,
                              Elm_Win_Indicator_Opacity_Mode mode)
{
   ELM_WIN_CHECK(obj);
   ELM_WIN_DATA_GET_OR_RETURN(obj, sd);

   if (mode == sd->ind_o_mode) return;
   sd->ind_o_mode = mode;
#ifdef HAVE_ELEMENTARY_X
   _elm_win_xwindow_get(sd);
   if (sd->x.xwin)
     {
        if (sd->ind_o_mode == ELM_WIN_INDICATOR_OPAQUE)
          ecore_x_e_illume_indicator_opacity_set
            (sd->x.xwin, ECORE_X_ILLUME_INDICATOR_OPAQUE);
        else if (sd->ind_o_mode == ELM_WIN_INDICATOR_TRANSLUCENT)
          ecore_x_e_illume_indicator_opacity_set
            (sd->x.xwin, ECORE_X_ILLUME_INDICATOR_TRANSLUCENT);
        else if (sd->ind_o_mode == ELM_WIN_INDICATOR_TRANSPARENT)
          ecore_x_e_illume_indicator_opacity_set
            (sd->x.xwin, ECORE_X_ILLUME_INDICATOR_TRANSPARENT);
        else if (sd->ind_o_mode == ELM_WIN_INDICATOR_BG_TRANSPARENT)
          ecore_x_e_illume_indicator_opacity_set
            (sd->x.xwin, ECORE_X_ILLUME_INDICATOR_BG_TRANSPARENT);
     }
#endif
   evas_object_smart_callback_call(obj, SIG_INDICATOR_PROP_CHANGED, NULL);
}

EAPI Elm_Win_Indicator_Opacity_Mode
elm_win_indicator_opacity_get(const Evas_Object *obj)
{
   ELM_WIN_CHECK(obj) ELM_WIN_INDICATOR_OPACITY_UNKNOWN;
   ELM_WIN_DATA_GET_OR_RETURN_VAL(obj, sd, ELM_WIN_INDICATOR_OPACITY_UNKNOWN);

   return sd->ind_o_mode;
}

EAPI void
elm_win_indicator_type_set(Evas_Object *obj,
                              Elm_Win_Indicator_Type_Mode mode)
{
   ELM_WIN_CHECK(obj);
   ELM_WIN_DATA_GET_OR_RETURN(obj, sd);

   if (mode == sd->ind_t_mode) return;
   sd->ind_t_mode = mode;
#ifdef HAVE_ELEMENTARY_X
   _elm_win_xwindow_get(sd);
   if (sd->x.xwin)
     {
        if (sd->ind_t_mode == ELM_WIN_INDICATOR_TYPE_1)
          ecore_x_e_illume_indicator_type_set
            (sd->x.xwin, ECORE_X_ILLUME_INDICATOR_TYPE_1);
        else if (sd->ind_t_mode == ELM_WIN_INDICATOR_TYPE_2)
          ecore_x_e_illume_indicator_type_set
            (sd->x.xwin, ECORE_X_ILLUME_INDICATOR_TYPE_2);
     }
#endif
   //evas_object_smart_callback_call(obj, SIG_INDICATOR_PROP_CHANGED, NULL);
}

EAPI Elm_Win_Indicator_Type_Mode
elm_win_indicator_type_get(const Evas_Object *obj)
{
   ELM_WIN_CHECK(obj) ELM_WIN_INDICATOR_TYPE_UNKNOWN;
   ELM_WIN_DATA_GET_OR_RETURN_VAL(obj, sd, ELM_WIN_INDICATOR_TYPE_UNKNOWN);

   return sd->ind_t_mode;
}

EAPI void
elm_win_indicator_fixed_style_set(Evas_Object *obj, Eina_Bool enabled)
{
#ifdef ELM_FOCUSED_UI
   ELM_WIN_CHECK(obj);
   ELM_WIN_DATA_GET_OR_RETURN(obj, sd);

   sd->fixed_style = enabled;
#else
   (void) obj;
   (void) enabled;
#endif
}

EAPI Eina_Bool
elm_win_indicator_fixed_style_get(const Evas_Object *obj)
{
#ifdef ELM_FOCUSED_UI
   ELM_WIN_CHECK(obj) EINA_FALSE;
   ELM_WIN_DATA_GET_OR_RETURN_VAL(obj, sd, EINA_FALSE);

   return sd->fixed_style;
#else
   (void) obj;
   return EINA_FALSE;
#endif
}

EAPI void
elm_win_screen_position_get(const Evas_Object *obj,
                            int *x,
                            int *y)
{
   ELM_WIN_CHECK(obj);
   ELM_WIN_DATA_GET_OR_RETURN(obj, sd);

   if (x) *x = sd->screen.x;
   if (y) *y = sd->screen.y;
}

EAPI Eina_Bool
elm_win_focus_get(const Evas_Object *obj)
{
   ELM_WIN_CHECK(obj) EINA_FALSE;
   ELM_WIN_DATA_GET_OR_RETURN_VAL(obj, sd, EINA_FALSE);

   return ecore_evas_focus_get(sd->ee);
}

EAPI void
elm_win_screen_constrain_set(Evas_Object *obj,
                             Eina_Bool constrain)
{
   ELM_WIN_CHECK(obj);
   ELM_WIN_DATA_GET_OR_RETURN(obj, sd);

   sd->constrain = !!constrain;
}

EAPI Eina_Bool
elm_win_screen_constrain_get(Evas_Object *obj)
{
   ELM_WIN_CHECK(obj) EINA_FALSE;
   ELM_WIN_DATA_GET_OR_RETURN_VAL(obj, sd, EINA_FALSE);

   return sd->constrain;
}

EAPI void
elm_win_screen_size_get(const Evas_Object *obj,
                        int *x,
                        int *y,
                        int *w,
                        int *h)
{
   ELM_WIN_CHECK(obj);
   ELM_WIN_DATA_GET_OR_RETURN(obj, sd);

   ecore_evas_screen_geometry_get(sd->ee, x, y, w, h);
}

EAPI void
elm_win_screen_dpi_get(const Evas_Object *obj,
                       int *xdpi,
                       int *ydpi)
{
   ELM_WIN_CHECK(obj);
   ELM_WIN_DATA_GET_OR_RETURN(obj, sd);

   ecore_evas_screen_dpi_get(sd->ee, xdpi, ydpi);
}

EAPI void
elm_win_conformant_set(Evas_Object *obj,
                       Eina_Bool conformant)
{
   ELM_WIN_CHECK(obj);
   ELM_WIN_DATA_GET_OR_RETURN(obj, sd);

#ifdef HAVE_ELEMENTARY_X
   _elm_win_xwindow_get(sd);
   if (sd->x.xwin)
     ecore_x_e_illume_conformant_set(sd->x.xwin, conformant);
#else
   (void)conformant;
#endif
}

EAPI Eina_Bool
elm_win_conformant_get(const Evas_Object *obj)
{
   ELM_WIN_CHECK(obj) EINA_FALSE;
   ELM_WIN_DATA_GET_OR_RETURN_VAL(obj, sd, EINA_FALSE);

#ifdef HAVE_ELEMENTARY_X
   _elm_win_xwindow_get(sd);
   if (sd->x.xwin)
     return ecore_x_e_illume_conformant_get(sd->x.xwin);
#endif
   return EINA_FALSE;
}

EAPI void
elm_win_quickpanel_set(Evas_Object *obj,
                       Eina_Bool quickpanel)
{
   ELM_WIN_CHECK(obj);
   ELM_WIN_DATA_GET_OR_RETURN(obj, sd);
#ifdef HAVE_ELEMENTARY_X
   _elm_win_xwindow_get(sd);
   if (sd->x.xwin)
     {
        ecore_x_e_illume_quickpanel_set(sd->x.xwin, quickpanel);
        if (quickpanel)
          {
             Ecore_X_Window_State states[2];

             states[0] = ECORE_X_WINDOW_STATE_SKIP_TASKBAR;
             states[1] = ECORE_X_WINDOW_STATE_SKIP_PAGER;
             ecore_x_netwm_window_state_set(sd->x.xwin, states, 2);
             ecore_x_icccm_hints_set(sd->x.xwin, 0, 0, 0, 0, 0, 0, 0);
          }
     }
#else
   (void)quickpanel;
#endif
}

EAPI Eina_Bool
elm_win_quickpanel_get(const Evas_Object *obj)
{
   ELM_WIN_CHECK(obj) EINA_FALSE;
   ELM_WIN_DATA_GET_OR_RETURN_VAL(obj, sd, EINA_FALSE);

#ifdef HAVE_ELEMENTARY_X
   _elm_win_xwindow_get(sd);
   if (sd->x.xwin)
     return ecore_x_e_illume_quickpanel_get(sd->x.xwin);
#endif
   return EINA_FALSE;
}

EAPI void
elm_win_quickpanel_priority_major_set(Evas_Object *obj,
                                      int priority)
{
   ELM_WIN_CHECK(obj);
   ELM_WIN_DATA_GET_OR_RETURN(obj, sd);

#ifdef HAVE_ELEMENTARY_X
   _elm_win_xwindow_get(sd);
   if (sd->x.xwin)
     ecore_x_e_illume_quickpanel_priority_major_set(sd->x.xwin, priority);
#else
   (void)priority;
#endif
}

EAPI int
elm_win_quickpanel_priority_major_get(const Evas_Object *obj)
{
   ELM_WIN_CHECK(obj) - 1;
   ELM_WIN_DATA_GET_OR_RETURN_VAL(obj, sd, -1);

#ifdef HAVE_ELEMENTARY_X
   _elm_win_xwindow_get(sd);
   if (sd->x.xwin)
     return ecore_x_e_illume_quickpanel_priority_major_get(sd->x.xwin);
#endif
   return -1;
}

EAPI void
elm_win_quickpanel_priority_minor_set(Evas_Object *obj,
                                      int priority)
{
   ELM_WIN_CHECK(obj);
   ELM_WIN_DATA_GET_OR_RETURN(obj, sd);

#ifdef HAVE_ELEMENTARY_X
   _elm_win_xwindow_get(sd);
   if (sd->x.xwin)
     ecore_x_e_illume_quickpanel_priority_minor_set(sd->x.xwin, priority);
#else
   (void)priority;
#endif
}

EAPI int
elm_win_quickpanel_priority_minor_get(const Evas_Object *obj)
{
   ELM_WIN_CHECK(obj) - 1;
   ELM_WIN_DATA_GET_OR_RETURN_VAL(obj, sd, -1);

#ifdef HAVE_ELEMENTARY_X
   _elm_win_xwindow_get(sd);
   if (sd->x.xwin)
     return ecore_x_e_illume_quickpanel_priority_minor_get(sd->x.xwin);
#endif
   return -1;
}

EAPI void
elm_win_quickpanel_zone_set(Evas_Object *obj,
                            int zone)
{
   ELM_WIN_CHECK(obj);
   ELM_WIN_DATA_GET_OR_RETURN(obj, sd);

#ifdef HAVE_ELEMENTARY_X
   _elm_win_xwindow_get(sd);
   if (sd->x.xwin)
     ecore_x_e_illume_quickpanel_zone_set(sd->x.xwin, zone);
#else
   (void)zone;
#endif
}

EAPI int
elm_win_quickpanel_zone_get(const Evas_Object *obj)
{
   ELM_WIN_CHECK(obj) 0;
   ELM_WIN_DATA_GET_OR_RETURN_VAL(obj, sd, 0);

#ifdef HAVE_ELEMENTARY_X
   _elm_win_xwindow_get(sd);
   if (sd->x.xwin)
     return ecore_x_e_illume_quickpanel_zone_get(sd->x.xwin);
#endif
   return 0;
}

EAPI void
elm_win_prop_focus_skip_set(Evas_Object *obj,
                            Eina_Bool skip)
{
   ELM_WIN_CHECK(obj);
   ELM_WIN_DATA_GET_OR_RETURN(obj, sd);

   sd->skip_focus = skip;
   TRAP(sd, focus_skip_set, skip);
}

EAPI void
elm_win_illume_command_send(Evas_Object *obj,
                            Elm_Illume_Command command,
                            void *params __UNUSED__)
{
   ELM_WIN_CHECK(obj);
   ELM_WIN_DATA_GET_OR_RETURN(obj, sd);

#ifdef HAVE_ELEMENTARY_X
   _elm_win_xwindow_get(sd);
   if (sd->x.xwin)
     {
        switch (command)
          {
           case ELM_ILLUME_COMMAND_FOCUS_BACK:
             ecore_x_e_illume_focus_back_send(sd->x.xwin);
             break;

           case ELM_ILLUME_COMMAND_FOCUS_FORWARD:
             ecore_x_e_illume_focus_forward_send(sd->x.xwin);
             break;

           case ELM_ILLUME_COMMAND_FOCUS_HOME:
             ecore_x_e_illume_focus_home_send(sd->x.xwin);
             break;

           case ELM_ILLUME_COMMAND_CLOSE:
             ecore_x_e_illume_close_send(sd->x.xwin);
             break;

           default:
             break;
          }
     }
#else
   (void)command;
#endif
}

EAPI Evas_Object *
elm_win_inlined_image_object_get(Evas_Object *obj)
{
   ELM_WIN_CHECK(obj) NULL;
   ELM_WIN_DATA_GET_OR_RETURN_VAL(obj, sd, NULL);

   return sd->img_obj;
}

EAPI void
elm_win_focus_highlight_enabled_set(Evas_Object *obj,
                                    Eina_Bool enabled)
{
   ELM_WIN_CHECK(obj);
   ELM_WIN_DATA_GET_OR_RETURN(obj, sd);

   enabled = !!enabled;
   if (sd->focus_highlight.enabled == enabled)
     return;

   sd->focus_highlight.enabled = enabled;

   if (sd->focus_highlight.enabled)
     {
        _elm_win_focus_highlight_init(sd);
        // TIZEN ONLY (20131211) : For broadcasting the state of focus highlight.
        _elm_widget_focus_highlighted_set(obj, EINA_TRUE);
        //TIZEN ONLY(20131204) : Supporting highlight change timing.
        evas_object_smart_callback_call(obj, SIG_HIGHLIGHT_ENABLED, NULL);
        //
     }
   else
     {
        _elm_win_focus_highlight_shutdown(sd);
#ifdef ELM_FOCUSED_UI
        sd->keyboard_attached = EINA_FALSE;
        sd->first_key_down = EINA_FALSE;
#endif
        // TIZEN ONLY (20131211) : For broadcasting the state of focus highlight.
        _elm_widget_focus_highlighted_set(obj, EINA_FALSE);
        //TIZEN ONLY(20131204) : Supporting highlight change timing.
        evas_object_smart_callback_call(obj, SIG_HIGHLIGHT_DISABLED, NULL);
        //
     }
}

EAPI Eina_Bool
elm_win_focus_highlight_enabled_get(const Evas_Object *obj)
{
   ELM_WIN_CHECK(obj) EINA_FALSE;
   ELM_WIN_DATA_GET_OR_RETURN_VAL(obj, sd, EINA_FALSE);

   return sd->focus_highlight.enabled;
}

EAPI void
elm_win_focus_highlight_style_set(Evas_Object *obj,
                                  const char *style)
{
   ELM_WIN_CHECK(obj);
   ELM_WIN_DATA_GET_OR_RETURN(obj, sd);

   eina_stringshare_replace(&sd->focus_highlight.style, style);
   sd->focus_highlight.changed_theme = EINA_TRUE;
   DBG("[WIN_FOCUS:%p] highlight_reconfigure_start theme_changed", obj);
   _elm_win_focus_highlight_reconfigure_job_start(sd);
}

EAPI const char *
elm_win_focus_highlight_style_get(const Evas_Object *obj)
{
   ELM_WIN_CHECK(obj) NULL;
   ELM_WIN_DATA_GET_OR_RETURN_VAL(obj, sd, NULL);

   return sd->focus_highlight.style;
}

EAPI Eina_Bool
elm_win_socket_listen(Evas_Object *obj,
                      const char *svcname,
                      int svcnum,
                      Eina_Bool svcsys)
{
   ELM_WIN_CHECK(obj) EINA_FALSE;
   ELM_WIN_DATA_GET_OR_RETURN_VAL(obj, sd, EINA_FALSE);

   if (!sd->ee) return EINA_FALSE;

   if (!ecore_evas_extn_socket_listen(sd->ee, svcname, svcnum, svcsys))
     return EINA_FALSE;

   return EINA_TRUE;
}

/* windowing specific calls - shall we do this differently? */

EAPI Ecore_X_Window
elm_win_xwindow_get(const Evas_Object *obj)
{
   if (!obj) return 0;

   if (!evas_object_smart_type_check_ptr(obj, WIN_SMART_NAME))
     {
        Ecore_Evas *ee = ecore_evas_ecore_evas_get(evas_object_evas_get(obj));
        return _elm_ee_xwin_get(ee);
     }

   ELM_WIN_CHECK(obj) 0;
   ELM_WIN_DATA_GET_OR_RETURN_VAL(obj, sd, 0);

#ifdef HAVE_ELEMENTARY_X
   if (sd->x.xwin) return sd->x.xwin;
   if (sd->parent) return elm_win_xwindow_get(sd->parent);
#endif
   return 0;
}

EAPI Ecore_Wl_Window *
elm_win_wl_window_get(const Evas_Object *obj)
{
   if (!obj) return NULL;

   if (!evas_object_smart_type_check_ptr(obj, WIN_SMART_NAME))
     {
        Ecore_Evas *ee = ecore_evas_ecore_evas_get(evas_object_evas_get(obj));
        return ecore_evas_wayland_window_get(ee);
     }

   ELM_WIN_CHECK(obj) NULL;
   ELM_WIN_DATA_GET_OR_RETURN_VAL(obj, sd, NULL);
#if HAVE_ELEMENTARY_WAYLAND
   if (sd->wl.win) return sd->wl.win;
   if (sd->parent) return elm_win_wl_window_get(sd->parent);
#endif
   return NULL;
}

EAPI Eina_Bool
elm_win_trap_set(const Elm_Win_Trap *t)
{
   DBG("old %p, new %p", trap, t);

   if ((t) && (t->version != ELM_WIN_TRAP_VERSION))
     {
        CRITICAL("trying to set a trap version %lu while %lu was expected!",
                 t->version, ELM_WIN_TRAP_VERSION);
        return EINA_FALSE;
     }

   trap = t;
   return EINA_TRUE;
}

EAPI void
elm_win_floating_mode_set(Evas_Object *obj, Eina_Bool floating)
{
   ELM_WIN_CHECK(obj);
   ELM_WIN_DATA_GET_OR_RETURN(obj, sd);

   if (floating == sd->floating) return;
   sd->floating = floating;
#ifdef HAVE_ELEMENTARY_X
   _elm_win_xwindow_get(sd);
   if (sd->x.xwin)
     {
        if (sd->floating)
          ecore_x_e_illume_window_state_set
             (sd->x.xwin, ECORE_X_ILLUME_WINDOW_STATE_FLOATING);
        else
          ecore_x_e_illume_window_state_set
             (sd->x.xwin, ECORE_X_ILLUME_WINDOW_STATE_NORMAL);
     }
#endif
}

EAPI Eina_Bool
elm_win_floating_mode_get(const Evas_Object *obj)
{
   ELM_WIN_CHECK(obj) EINA_FALSE;
   ELM_WIN_DATA_GET_OR_RETURN_VAL(obj, sd, EINA_FALSE);

   return sd->floating;
}

EAPI void
elm_win_wm_desktop_layout_support_set(Evas_Object    *obj,
                                      const Eina_Bool support)
{
   Ecore_X_Atom E_ATOM_WINDOW_DESKTOP_LAYOUT_SUPPORTED = 0;
   unsigned int val = 0;

   ELM_WIN_CHECK(obj);
   ELM_WIN_DATA_GET_OR_RETURN(obj, sd);

   E_ATOM_WINDOW_DESKTOP_LAYOUT_SUPPORTED = ecore_x_atom_get("_E_ATOM_WINDOW_DESKTOP_LAYOUT_SUPPORTED");
   if (!E_ATOM_WINDOW_DESKTOP_LAYOUT_SUPPORTED) return;

   if (support) val = 1;

   ecore_x_window_prop_card32_set(sd->x.xwin, E_ATOM_WINDOW_DESKTOP_LAYOUT_SUPPORTED, &val, 1);
}

// TINEN ONLY(20131207) : For making unfocusable elm_window to be focusable.
EAPI void
elm_win_focus_allow_set(Evas_Object *obj, Eina_Bool enable)
{
   ELM_WIN_CHECK(obj);
   _elm_widget_top_win_focused_set(obj, enable);
}

EAPI Eina_Bool
elm_win_focus_allow_get(const Evas_Object *obj)
{
   ELM_WIN_CHECK(obj) EINA_FALSE;
   return _elm_widget_top_win_focused_get(obj);
}
///////////////

EAPI const Eina_List *
elm_win_aux_hints_supported_get(const Evas_Object *obj)
{
   ELM_WIN_CHECK(obj) NULL;
   ELM_WIN_DATA_GET_OR_RETURN_VAL(obj, sd, NULL);
   return ecore_evas_aux_hints_supported_get(sd->ee);
}

EAPI int
elm_win_aux_hint_add(Evas_Object *obj,
                     const char  *hint,
                     const char  *val)
{
   ELM_WIN_CHECK(obj) -1;
   ELM_WIN_DATA_GET_OR_RETURN_VAL(obj, sd, -1);
   return ecore_evas_aux_hint_add(sd->ee, hint, val);
}

EAPI Eina_Bool
elm_win_aux_hint_del(Evas_Object *obj,
                     const int    id)
{
   ELM_WIN_CHECK(obj) EINA_FALSE;
   ELM_WIN_DATA_GET_OR_RETURN_VAL(obj, sd, EINA_FALSE);
   return ecore_evas_aux_hint_del(sd->ee, id);
}

EAPI Eina_Bool
elm_win_aux_hint_val_set(Evas_Object *obj,
                         const int    id,
                         const char  *val)
{
   ELM_WIN_CHECK(obj) EINA_FALSE;
   ELM_WIN_DATA_GET_OR_RETURN_VAL(obj, sd, EINA_FALSE);
   return ecore_evas_aux_hint_val_set(sd->ee, id, val);
}
