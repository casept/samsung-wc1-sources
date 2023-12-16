#include <Elementary.h>
#include "elm_priv.h"

#define DATETIME_FIELD_COUNT            6
#define BUFF_SIZE                       1024
#define MONTH_STRING_MAX_SIZE           32
#define TOTAL_NUMBER_OF_MONTHS          12
#define STRUCT_TM_YEAR_BASE_VALUE       1900
#define STRUCT_TM_TIME_12HRS_MAX_VALUE  12
#define STRUCT_TM_TIME_24HRS_MAX_VALUE  23
#define PICKER_POPUP_FIELD_COUNT        3

/* struct tm does not define the fields in the order year, month,
 * date, hour, minute. values are reassigned to an array for easy
 * handling.
 */
#define DATETIME_MODULE_TM_ARRAY(intptr, tmptr) \
  int *intptr[] = {                             \
     &(tmptr)->tm_year,                         \
     &(tmptr)->tm_mon,                          \
     &(tmptr)->tm_mday,                         \
     &(tmptr)->tm_hour,                         \
     &(tmptr)->tm_min}

static const char *field_styles[] = {
                         "year", "month", "date", "hour", "minute", "ampm" };

static char month_arr[TOTAL_NUMBER_OF_MONTHS][MONTH_STRING_MAX_SIZE];

static const char SIG_EDIT_START[] = "edit,start";
static const char SIG_EDIT_END[] = "edit,end";
static const char SIG_PICKER_VALUE_SET[] = "picker,value,set";

/*Below signals are added to support the feature of custom Datetime picker popup*/
static const char SIG_DATETIME_PICKER_LAYOUT_CREATE[] = "datetime,picker,layout,create";
static const char SIG_DATE_PICKER_LAYOUT_GET[] = "picker,date,layout,get";
static const char SIG_TIME_PICKER_LAYOUT_GET[] = "picker,time,layout,get";
static const char SIG_CUSTOM_PICKER_SET_BTN_CLICKED[] = "custom,picker,value,set";
static const char SIG_CENTER_POPUP_WIN_GET[] = "custom,picker,win,get";

typedef struct _Popup_Module_Data Popup_Module_Data;

struct _Popup_Module_Data
{
   Elm_Datetime_Module_Data mod_data;
   Evas_Object *date_btn, *time_btn;
   Evas_Object *popup;
   Evas_Object *datepicker_layout, *timepicker_layout;
   Evas_Object *popup_field[DATETIME_FIELD_COUNT];
   Evas_Object *center_popup_win;
   Evas_Object *key_grab_rect;
   struct tm set_time;
   char weekday[BUFF_SIZE];
   int field_location[PICKER_POPUP_FIELD_COUNT];
   Eina_Bool time_12hr_fmt;
   Eina_Bool is_pm;
   Eina_Bool weekday_show;
   Eina_Bool weekday_loc_first;
   Eina_Bool is_center_popup;
   Eina_Bool is_custom_popup;
   Eina_Bool dateBtn_is_visible;
   Eina_Bool timeBtn_is_visible;
};

#ifdef ELM_FEATURE_MULTI_WINDOW

typedef struct _Center_Popup_Data Center_Popup_Data;
struct _Center_Popup_Data
{
   Evas_Object *parent;
   Evas_Object *parent_win;
   Evas_Object *block_events;
   Evas_Object *popup_win;
   Evas_Object *popup;
   Ecore_Job   *job;
};

/*Back and escape key implementation in case of multiwindow starts*/

static void _popup_ungrab_back_key(Popup_Module_Data *popup_mod);
const char *KEY_BACK = "XF86Stop";
const char *KEY_ESCAPE = "Escape";

static void
_popup_key_grab_rect_key_up_cb(void *data , Evas *e __UNUSED__, Evas_Object *obj __UNUSED__,
                                void *event_info)
{
   Evas_Event_Key_Down *ev = event_info;
   Popup_Module_Data *popup_mod = (Popup_Module_Data *)data;
   if (!popup_mod) return;

   if (!strcmp(ev->keyname, KEY_BACK) || !strcmp(ev->keyname, KEY_ESCAPE))
   {
      evas_object_hide(popup_mod->popup);
      evas_object_smart_callback_call(popup_mod->mod_data.base, SIG_EDIT_END, NULL);
      _popup_ungrab_back_key(popup_mod);

   }
}

static void
_popup_ungrab_back_key(Popup_Module_Data *popup_mod)
{
   evas_object_key_ungrab(popup_mod->key_grab_rect, KEY_BACK, 0, 0);
   evas_object_key_ungrab(popup_mod->key_grab_rect, KEY_ESCAPE, 0, 0);
   evas_object_event_callback_del_full(popup_mod->key_grab_rect, EVAS_CALLBACK_KEY_UP,
                                         _popup_key_grab_rect_key_up_cb, popup_mod);
   evas_object_del(popup_mod->key_grab_rect);
}

static void
_popup_grab_back_key(Popup_Module_Data *popup_mod)
{
   Evas *canvas = evas_object_evas_get(popup_mod->popup);
   popup_mod->key_grab_rect = evas_object_rectangle_add(canvas);

   if (!evas_object_key_grab(popup_mod->key_grab_rect, KEY_BACK, 0, 0, EINA_FALSE))
      return ;

   if (!evas_object_key_grab(popup_mod->key_grab_rect, KEY_ESCAPE, 0, 0, EINA_FALSE))
      return ;

   evas_object_event_callback_add(popup_mod->key_grab_rect, EVAS_CALLBACK_KEY_UP,
                                  _popup_key_grab_rect_key_up_cb, popup_mod);
}
/*Back Key and escape key implementation Ends*/

static void
_center_popup_resize_cb(void *data, Evas * e __UNUSED__, Evas_Object * obj, void *event_info __UNUSED__)
{
   Center_Popup_Data *cp_data = data;
   Evas_Coord ox, oy, ow, oh;
   evas_object_geometry_get(obj, &ox, &oy, &ow, &oh);
   evas_object_resize(cp_data->popup_win, ow, oh);
   evas_output_viewport_set(evas_object_evas_get(obj), ox, oy, ow, oh);
   evas_output_size_set(evas_object_evas_get(obj), ow, oh);
}

static void
_center_popup_parent_del_cb(void *data, Evas * e __UNUSED__, Evas_Object * obj __UNUSED__, void *event_info __UNUSED__)
{
   Evas_Object *object = data;
   evas_object_del(object);
}

static void
_center_popup_job_cb(void *data)
{
   Center_Popup_Data *cp_data = data;
   cp_data->job = NULL;
   evas_object_event_callback_add(cp_data->popup, EVAS_CALLBACK_RESIZE, _center_popup_resize_cb, cp_data);

   evas_object_show(cp_data->block_events);
   evas_object_show(cp_data->popup_win);
}

static void
_block_area_clicked_cb(void *data, Evas_Object *obj __UNUSED__, const char *emission __UNUSED__, const char *source __UNUSED__)
{
   evas_object_smart_callback_call(data, "block,clicked", NULL);
}

static void
_center_popup_hide_cb(void *data, Evas * e __UNUSED__, Evas_Object * obj __UNUSED__, void *event_info __UNUSED__)
{
   Center_Popup_Data *cp_data = data;
   evas_object_hide(cp_data->block_events);
   evas_object_hide(cp_data->popup_win);
}

static void
_center_popup_show_cb(void *data, Evas * e __UNUSED__, Evas_Object * obj __UNUSED__, void *event_info __UNUSED__)
{
   Center_Popup_Data *cp_data = data;
   if (cp_data->job)
     {
        ecore_job_del(cp_data->job);
        cp_data->job = NULL;
     }
   cp_data->job = ecore_job_add(_center_popup_job_cb, cp_data);
}

static void
_center_popup_del_cb(void *data, Evas * e __UNUSED__, Evas_Object * obj, void *event_info __UNUSED__)
{
   Center_Popup_Data *cp_data = data;
   if (!cp_data) return;
   elm_layout_signal_callback_del(cp_data->block_events, "elm,action,click", "elm", _block_area_clicked_cb);
   evas_object_event_callback_del_full(obj, EVAS_CALLBACK_DEL, _center_popup_del_cb, cp_data);
   evas_object_event_callback_del_full(cp_data->parent, EVAS_CALLBACK_DEL, _center_popup_parent_del_cb, obj);
   evas_object_event_callback_del_full(obj, EVAS_CALLBACK_SHOW, _center_popup_show_cb, cp_data);
   evas_object_event_callback_del_full(obj, EVAS_CALLBACK_HIDE, _center_popup_hide_cb, cp_data);
   evas_object_event_callback_del_full(obj, EVAS_CALLBACK_RESIZE, _center_popup_resize_cb, cp_data);

   if (cp_data->job)
     {
        ecore_job_del(cp_data->job);
        cp_data->job = NULL;
     }
   elm_win_resize_object_del(cp_data->parent_win, cp_data->block_events);
   evas_object_del(cp_data->block_events);
   evas_object_del(cp_data->popup_win);
   free(cp_data);
}

static void
_popup_win_del_cb(void *data, Evas * e __UNUSED__, Evas_Object * obj, void *event_info __UNUSED__)
{
   Evas_Object *popup = data;
   elm_win_resize_object_del(obj, popup);
}

static Evas_Object *
_center_popup_add(Evas_Object *parent)
{
   Center_Popup_Data *cp_data;
   cp_data = calloc(1, sizeof(Center_Popup_Data));
   cp_data->parent = parent;

   if (!strcmp(evas_object_type_get(parent), "elm_win"))
     cp_data->parent_win = parent;
   else
     cp_data->parent_win = elm_widget_top_get(parent);
   // Create popup window
   cp_data->popup_win = elm_win_add(cp_data->parent_win, "Center Popup", ELM_WIN_DIALOG_BASIC);

   if (!cp_data->popup_win) goto error;
   elm_win_alpha_set(cp_data->popup_win, EINA_TRUE);
   elm_win_title_set(cp_data->popup_win, "Center Popup");
   elm_win_modal_set(cp_data->popup_win, EINA_TRUE);
   if (elm_win_wm_rotation_supported_get(cp_data->popup_win))
     {
        int rots[4] = { 0, 90, 180, 270 };
        elm_win_wm_rotation_available_rotations_set(cp_data->popup_win, (const int*)(&rots), 4);
     }

   // Create Dimming object
   cp_data->block_events = elm_layout_add(cp_data->parent_win);
   elm_layout_theme_set(cp_data->block_events, "notify", "block_events", "center_popup");
   evas_object_size_hint_weight_set(cp_data->block_events, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_win_resize_object_add(cp_data->parent_win, cp_data->block_events);
   evas_object_repeat_events_set(cp_data->block_events, EINA_FALSE);

   cp_data->popup = elm_popup_add(cp_data->popup_win);
   if (!cp_data->popup) goto error;

   // for object color class setting //
   Evas_Object *edje;

   edje = evas_object_data_get(parent, "color_overlay");
   if (edje) evas_object_data_set(cp_data->popup, "color_overlay", edje);

   edje = evas_object_data_get(parent, "font_overlay");
   if (edje) evas_object_data_set(cp_data->popup, "font_overlay", edje);
   ////////////////////////////////////

   elm_object_style_set(cp_data->popup, "center_popup");

   elm_popup_allow_events_set(cp_data->popup, EINA_TRUE);
   evas_object_size_hint_weight_set(cp_data->popup, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_win_resize_object_add(cp_data->popup_win, cp_data->popup);

   elm_layout_signal_callback_add(cp_data->block_events, "elm,action,click", "elm", _block_area_clicked_cb, cp_data->popup);
   evas_object_event_callback_add(cp_data->popup, EVAS_CALLBACK_SHOW, _center_popup_show_cb, cp_data);
   evas_object_event_callback_add(cp_data->popup, EVAS_CALLBACK_HIDE, _center_popup_hide_cb, cp_data);
   evas_object_event_callback_priority_add(cp_data->popup, EVAS_CALLBACK_DEL, EVAS_CALLBACK_PRIORITY_BEFORE, _center_popup_del_cb, cp_data);
   evas_object_event_callback_add(parent, EVAS_CALLBACK_DEL, _center_popup_parent_del_cb, cp_data->popup);
   evas_object_event_callback_add(cp_data->popup_win, EVAS_CALLBACK_DEL, _popup_win_del_cb, cp_data->popup);

   return cp_data->popup;

error:
   if (cp_data->block_events)
     {
        elm_win_resize_object_del(cp_data->parent_win, cp_data->block_events);
        evas_object_del(cp_data->block_events);
     }
   if (cp_data->popup_win) evas_object_del(cp_data->popup_win);
   free(cp_data);
   return NULL;
}
#endif

static int
_picker_nextfield_location_get(void *data, int curr)
{
   int idx, next_idx;
   Popup_Module_Data *popup_mod;
   popup_mod = (Popup_Module_Data *)data;
   if (!popup_mod) return ELM_DATETIME_LAST;

   for (idx = 0; idx <= ELM_DATETIME_DATE; idx++)
     if (popup_mod->field_location[curr] == idx) break;

   if (idx >= ELM_DATETIME_DATE) return ELM_DATETIME_LAST;

   for (next_idx = 0; next_idx <= ELM_DATETIME_DATE; next_idx++)
     if (popup_mod->field_location[next_idx] == idx+1) return next_idx;

   return ELM_DATETIME_LAST;
}

static void
_picker_hide_cb(void *data,
                Evas_Object *obj __UNUSED__,
                const char *emission __UNUSED__,
                const char *source __UNUSED__)
{
   Popup_Module_Data *popup_mod;
   popup_mod = (Popup_Module_Data *)data;
   if (!popup_mod) return;

   evas_object_smart_callback_call(obj, SIG_EDIT_END, NULL);

   evas_object_hide(popup_mod->popup);
#ifdef ELM_FEATURE_MULTI_WINDOW
   _popup_ungrab_back_key(popup_mod);
#endif
}

static void
_set_datepicker_value(Popup_Module_Data *popup_mod)
{
   struct tm set_time;

   if (!popup_mod) return;

   elm_datetime_value_get(popup_mod->mod_data.base, &set_time);
   set_time.tm_year = (int)elm_spinner_value_get(popup_mod->popup_field[ELM_DATETIME_YEAR]) - STRUCT_TM_YEAR_BASE_VALUE;
   set_time.tm_mon = (int)elm_spinner_value_get(popup_mod->popup_field[ELM_DATETIME_MONTH]) - 1;
   set_time.tm_mday = (int)elm_spinner_value_get(popup_mod->popup_field[ELM_DATETIME_DATE]);
   elm_datetime_value_set(popup_mod->mod_data.base, &set_time);
}

static void
_set_timepicker_value(Popup_Module_Data *popup_mod)
{
   struct tm set_time;

   if (!popup_mod) return;

   elm_datetime_value_get(popup_mod->mod_data.base, &set_time);

   set_time.tm_hour = (int)elm_spinner_value_get(popup_mod->popup_field[ELM_DATETIME_HOUR]);
   set_time.tm_min = (int)elm_spinner_value_get(popup_mod->popup_field[ELM_DATETIME_MINUTE]);
   elm_datetime_value_set(popup_mod->mod_data.base, &set_time);
}

static void
_custom_popup_win_get_cb(void *data, Evas_Object *obj __UNUSED__, void *event_info)
{
   Popup_Module_Data *popup_mod;

   popup_mod = (Popup_Module_Data *)data;
   if (!popup_mod) return;
   if (event_info)
     {
        popup_mod->center_popup_win = (Evas_Object*)event_info;
        popup_mod->is_center_popup = EINA_TRUE;
     }
   popup_mod->is_custom_popup = EINA_TRUE;
}

static void
_custom_popup_set_btn_clicked_cb(void *data, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   Popup_Module_Data *popup_mod;

   popup_mod = (Popup_Module_Data *)data;
   if (!popup_mod) return;

   if (popup_mod->datepicker_layout)
     _set_datepicker_value(popup_mod);
   if (popup_mod->timepicker_layout)
     _set_timepicker_value(popup_mod);
   evas_object_smart_callback_call(popup_mod->mod_data.base, SIG_PICKER_VALUE_SET, NULL);
}

static void
_popup_set_btn_clicked_cb(void *data, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   Popup_Module_Data *popup_mod;
   Evas_Object *content, *widget;
   int idx = 0;
   Evas_Object *spinner, *entry;

   popup_mod = (Popup_Module_Data *)data;
   if (!popup_mod) return;

   widget = popup_mod->mod_data.base;

   if (widget)
     evas_object_smart_callback_call(widget, SIG_EDIT_END, NULL);

   evas_object_hide(popup_mod->popup);

   content = elm_object_content_get(popup_mod->popup);
   if (content == popup_mod->datepicker_layout)
     {
        for (idx = 0; idx <= ELM_DATETIME_DATE; idx++)
          {
             spinner = popup_mod->popup_field[idx];
             entry = elm_object_part_content_get(spinner, "elm.swallow.entry");
             if (!entry) continue;
             if (elm_object_focus_get(entry))
               {
                  elm_layout_signal_emit(spinner, "elm,action,entry,toggle", "elm");
                  edje_object_message_signal_process(elm_layout_edje_get(spinner));
               }
          }
        _set_datepicker_value(popup_mod);
     }
   else if (content == popup_mod->timepicker_layout)
     {
        for (idx = ELM_DATETIME_HOUR; idx < ELM_DATETIME_AMPM; idx++)
          {
             spinner = popup_mod->popup_field[idx];
             entry = elm_object_part_content_get(spinner, "elm.swallow.entry");
             if (!entry) continue;
             if (elm_object_focus_get(entry))
               {
                  elm_layout_signal_emit(spinner, "elm,action,entry,toggle", "elm");
                  edje_object_message_signal_process(elm_layout_edje_get(spinner));
               }
          }
        _set_timepicker_value(popup_mod);
     }

   evas_object_smart_callback_call(popup_mod->mod_data.base, SIG_PICKER_VALUE_SET, NULL);
#ifdef ELM_FEATURE_MULTI_WINDOW
   _popup_grab_back_key(popup_mod);
#endif
}

static void
_popup_cancel_btn_clicked_cb(void *data, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   Popup_Module_Data *popup_mod;
   Evas_Object *content, *widget;
   int idx = 0;
   Evas_Object *spinner, *entry;

   popup_mod = (Popup_Module_Data *)data;
   if (!popup_mod) return;

   widget = popup_mod->mod_data.base;

   if (widget)
     evas_object_smart_callback_call(widget, SIG_EDIT_END, NULL);

   evas_object_hide(popup_mod->popup);

   content = elm_object_content_get(popup_mod->popup);
   if (content == popup_mod->datepicker_layout)
     {
        for (idx = 0; idx <= ELM_DATETIME_DATE; idx++)
          {
             spinner = popup_mod->popup_field[idx];
             entry = elm_object_part_content_get(spinner, "elm.swallow.entry");
             if (!entry) continue;
             if (elm_object_focus_get(entry))
               {
                  elm_layout_signal_emit(spinner, "elm,action,entry,toggle", "elm");
               }
          }
     }
   else if (content == popup_mod->timepicker_layout)
     {
        for (idx = ELM_DATETIME_HOUR; idx < ELM_DATETIME_AMPM; idx++)
          {
             spinner = popup_mod->popup_field[idx];
             entry = elm_object_part_content_get(spinner, "elm.swallow.entry");
             if (!entry) continue;
             if (elm_object_focus_get(entry))
               {
                  elm_layout_signal_emit(spinner, "elm,action,entry,toggle", "elm");
               }
          }
     }
#ifdef ELM_FEATURE_MULTI_WINDOW
   _popup_ungrab_back_key(popup_mod);
#endif
}

static void
_entry_activated_cb(void *data, Evas_Object *obj, void *event_info __UNUSED__)
{
   Popup_Module_Data *popup_mod;
   Evas_Object *entry;
   int idx, next_idx;

   popup_mod = (Popup_Module_Data *)data;
   if (!popup_mod) return;

   for (idx = 0; idx <= ELM_DATETIME_DATE; idx++)
     {
        entry = elm_object_part_content_get(popup_mod->popup_field[idx], "elm.swallow.entry");
        if (obj == entry)
          {
             next_idx = _picker_nextfield_location_get(popup_mod, idx);
             if (next_idx != ELM_DATETIME_LAST)
               elm_layout_signal_emit(popup_mod->popup_field[next_idx], "elm,action,entry,toggle", "elm");
             return;
          }
     }

   entry = elm_object_part_content_get(popup_mod->popup_field[ELM_DATETIME_HOUR], "elm.swallow.entry");
   if (obj == entry)
      elm_layout_signal_emit(popup_mod->popup_field[ELM_DATETIME_MINUTE], "elm,action,entry,toggle", "elm");
}

static void
_entry_clicked_cb(void *data, Evas_Object *obj, void *event_info __UNUSED__)
{
   Popup_Module_Data *popup_mod;

   popup_mod = (Popup_Module_Data *)data;
   if (!popup_mod || !obj) return;
}

static void
_entry_focused_cb(void *data, Evas_Object *obj, void *event_info __UNUSED__)
{
   Popup_Module_Data *popup_mod;
   Evas_Object *entry;
   int idx, value;
   const char *hour_value = NULL;

   popup_mod = (Popup_Module_Data *)data;
   if (!popup_mod) return;

   entry = elm_object_part_content_get(popup_mod->popup_field[ELM_DATETIME_MONTH],
                                       "elm.swallow.entry");
   if (obj == entry)
     {
        value = (int)elm_spinner_value_get(popup_mod->popup_field[ELM_DATETIME_MONTH]) - 1;
        elm_object_text_set(obj, month_arr[value]);
     }
   entry = elm_object_part_content_get(popup_mod->popup_field[ELM_DATETIME_HOUR],
                                       "elm.swallow.entry");
   if (obj == entry)
     {
        value = elm_spinner_value_get(popup_mod->popup_field[ELM_DATETIME_HOUR]);
        hour_value = elm_spinner_special_value_get(popup_mod->popup_field[ELM_DATETIME_HOUR], value);
        if ((popup_mod->time_12hr_fmt) && hour_value)
          elm_object_text_set(obj, hour_value);
     }
   for (idx = 0; idx < DATETIME_FIELD_COUNT -1; idx++)
     {
        if (!popup_mod->popup_field[idx]) continue;

        entry = elm_object_part_content_get(popup_mod->popup_field[idx], "elm.swallow.entry");
        if ((obj != entry) && elm_object_focus_get(entry))
          {
             elm_layout_signal_emit(popup_mod->popup_field[idx],
                                    "elm,action,entry,toggle", "elm");
             return;
          }
     }
}

static void
_entry_unfocused_cb(void *data, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   Popup_Module_Data *popup_mod;

   popup_mod = (Popup_Module_Data *)data;
   if (!popup_mod) return;

   // TODO: entry_unfocused code
}

void
_set_datepicker_popup_title_text(Popup_Module_Data *popup_mod)
{
   struct tm set_time;
   time_t t;
   if (!popup_mod) return;

   t = time(NULL);
   localtime_r(&t, &set_time);
   set_time.tm_year = (popup_mod->set_time).tm_year;
   set_time.tm_mon = (popup_mod->set_time).tm_mon;
   set_time.tm_mday = (popup_mod->set_time).tm_mday;
   /* FIXME: To restrict month wrapping because of summer time in some locales,
    * ignore day light saving mode in mktime(). */
   set_time.tm_isdst = -1;
   mktime(&set_time);
   strftime(popup_mod->weekday, BUFF_SIZE, "%a", &set_time);
   elm_object_part_text_set(popup_mod->popup, "elm.text.title2", popup_mod->weekday);

   elm_object_domain_translatable_part_text_set(popup_mod->popup, "title,text", PACKAGE, E_("IDS_TPLATFORM_HEADER_SET_DATE"));
}

void
_set_timepicker_popup_title_text(Popup_Module_Data *popup_mod)
{
   if (!popup_mod) return;

   elm_object_domain_translatable_part_text_set(popup_mod->popup, "title,text", PACKAGE, E_("IDS_TPLATFORM_HEADER_SET_TIME"));
   elm_object_part_text_set(popup_mod->popup, "elm.text.title2", "");
}

static void
_set_ampm_value(Popup_Module_Data *popup_mod)
{
   char ampm_str[BUFF_SIZE] = {0,};
   const char *fmt = NULL;

   if (!popup_mod || !popup_mod->time_12hr_fmt) return;

   fmt = popup_mod->mod_data.field_format_get(popup_mod->mod_data.base,
                                              ELM_DATETIME_AMPM);
   strftime(ampm_str, BUFF_SIZE, fmt, &(popup_mod->set_time));
   if (ampm_str[0])
     elm_object_text_set(popup_mod->popup_field[ELM_DATETIME_AMPM], ampm_str);
   else if (popup_mod->set_time.tm_hour < STRUCT_TM_TIME_12HRS_MAX_VALUE)
     elm_object_domain_translatable_text_set(popup_mod->popup_field[ELM_DATETIME_AMPM],
                                             PACKAGE, E_("IDS_COM_BODY_AM"));
   else
     elm_object_domain_translatable_text_set(popup_mod->popup_field[ELM_DATETIME_AMPM],
                                             PACKAGE, E_("IDS_COM_BODY_PM"));
}

static Evas_Object *
_access_object_get(const Evas_Object *obj, const char* part)
{
   Evas_Object *eo, *po, *ao;

   eo = elm_layout_edje_get(obj);

   po = (Evas_Object *)edje_object_part_object_get(eo, part);
   ao = evas_object_data_get(po, "_part_access_obj");

   return ao;
}

static void
_datepicker_value_changed_cb(void *data, Evas_Object *obj, void *event_info __UNUSED__)
{
   Popup_Module_Data *popup_mod;
   struct tm min_values, max_values;
   int idx, field_idx, min, max;

   popup_mod = (Popup_Module_Data *)data;
   if (!popup_mod) return;

   for (idx = 0; idx < DATETIME_FIELD_COUNT; idx++)
     if ((obj == popup_mod->popup_field[idx])) break;

   if (idx > ELM_DATETIME_DATE) return;

   field_idx = idx;
   DATETIME_MODULE_TM_ARRAY(set_val_arr, &popup_mod->set_time);
   if (field_idx == ELM_DATETIME_YEAR)
     *set_val_arr[field_idx] = (int)elm_spinner_value_get(obj) - STRUCT_TM_YEAR_BASE_VALUE;
   else if (field_idx == ELM_DATETIME_MONTH)
     *set_val_arr[field_idx] = (int)elm_spinner_value_get(obj) - 1;
   else
     *set_val_arr[field_idx] = (int)elm_spinner_value_get(obj);

   popup_mod->mod_data.fields_min_max_get(popup_mod->mod_data.base,
                       &(popup_mod->set_time), &min_values, &max_values);

   DATETIME_MODULE_TM_ARRAY(min_val_arr, &min_values);
   DATETIME_MODULE_TM_ARRAY(max_val_arr, &max_values);

   for (idx = field_idx; idx <= ELM_DATETIME_DATE; idx++)
     {
        min = *min_val_arr[idx];
        max = *max_val_arr[idx];
        if (idx == ELM_DATETIME_YEAR)
          {
             min += STRUCT_TM_YEAR_BASE_VALUE;
             max += STRUCT_TM_YEAR_BASE_VALUE;
          }
        else if (idx == ELM_DATETIME_MONTH)
          {
             min += 1;
             max += 1;
          }
        elm_spinner_min_max_set(popup_mod->popup_field[idx], min, max);
     }
   for (idx = field_idx; idx <= ELM_DATETIME_DATE; idx++)
     {
        if (idx == ELM_DATETIME_YEAR)
          *set_val_arr[idx] = (int)elm_spinner_value_get(popup_mod->popup_field[idx]) - STRUCT_TM_YEAR_BASE_VALUE;
        else if (idx == ELM_DATETIME_MONTH)
          *set_val_arr[idx] = (int)elm_spinner_value_get(popup_mod->popup_field[idx]) - 1;
        else
          *set_val_arr[idx] = (int)elm_spinner_value_get(popup_mod->popup_field[idx]);
     }

   _set_datepicker_popup_title_text(popup_mod);
}

static void
_timepicker_value_changed_cb(void *data, Evas_Object *obj, void *event_info __UNUSED__)
{
   Popup_Module_Data *popup_mod;
   struct tm min_values, max_values;

   popup_mod = (Popup_Module_Data *)data;
   if (!popup_mod) return;

   if (obj == popup_mod->popup_field[ELM_DATETIME_HOUR])
     {
        (popup_mod->set_time).tm_hour = (int)elm_spinner_value_get(obj);;
        popup_mod->mod_data.fields_min_max_get(popup_mod->mod_data.base,
                            &(popup_mod->set_time), &min_values, &max_values);
        elm_spinner_min_max_set(popup_mod->popup_field[ELM_DATETIME_MINUTE],
                                min_values.tm_min, max_values.tm_min);
        (popup_mod->set_time).tm_min = (int)elm_spinner_value_get(
                                 popup_mod->popup_field[ELM_DATETIME_MINUTE]);

        popup_mod->is_pm = ((popup_mod->set_time).tm_hour < STRUCT_TM_TIME_12HRS_MAX_VALUE) ? 0 : 1;
        _set_ampm_value(popup_mod);
     }
   else if (obj == popup_mod->popup_field[ELM_DATETIME_MINUTE])
     (popup_mod->set_time).tm_min = (int)elm_spinner_value_get(obj);

}

static void
_hour_validity_check(void *data, Evas_Object *obj, void *event_info)
{
    Popup_Module_Data *popup_mod;

    popup_mod = (Popup_Module_Data *)data;
    if (!popup_mod) return;

    Evas_Object* entry = elm_object_part_content_get(popup_mod->popup_field[ELM_DATETIME_HOUR], "elm.swallow.entry");

    if (popup_mod->popup_field[ELM_DATETIME_HOUR] != obj || !entry)
      return;
    double* val = (double *)event_info;

    if (popup_mod->time_12hr_fmt)
      {
         if ((popup_mod->is_pm) && (*val < STRUCT_TM_TIME_12HRS_MAX_VALUE))
           *val += STRUCT_TM_TIME_12HRS_MAX_VALUE;
         if (!(popup_mod->is_pm) && (*val == STRUCT_TM_TIME_12HRS_MAX_VALUE))
           *val = 0;
      }
}

static void
_ampm_clicked_cb(void *data, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   Popup_Module_Data *popup_mod;
   double hour, hour_min, hour_max;

   popup_mod = (Popup_Module_Data *)data;
   if (!popup_mod || !popup_mod->time_12hr_fmt) return;

   hour = (int)elm_spinner_value_get(popup_mod->popup_field[ELM_DATETIME_HOUR]);

   if (hour < STRUCT_TM_TIME_12HRS_MAX_VALUE)
     hour += STRUCT_TM_TIME_12HRS_MAX_VALUE;
   else
     hour -= STRUCT_TM_TIME_12HRS_MAX_VALUE;

   elm_spinner_min_max_get(popup_mod->popup_field[ELM_DATETIME_HOUR], &hour_min, &hour_max);
   if ((hour >= hour_min) && (hour <= hour_max))
     {
        (popup_mod->set_time).tm_hour = hour;
        elm_spinner_value_set(popup_mod->popup_field[ELM_DATETIME_HOUR], hour);
        popup_mod->is_pm = (hour < STRUCT_TM_TIME_12HRS_MAX_VALUE) ? 0 : 1;
        _set_ampm_value(popup_mod);
     }
}

char *
_text_insert(const char *text, char *input, int pos)
{
   char *result;
   int text_len, input_len;

   text_len = strlen(text);
   input_len = strlen(input);
   result = (char *)malloc(sizeof(char) * (text_len + input_len) + 1);
   memset(result, 0, sizeof(char) * (text_len + input_len) + 1);

   strncpy(result, text, pos);
   strcpy(result + pos, input);
   strcpy(result + pos + input_len, text + pos);

   return result;
}

static void
_year_validity_checking_filter(void *data, Evas_Object *obj, char **text)
{
   Popup_Module_Data *popup_mod;
   Evas_Object *entry;
   char *new_str = NULL;
   int min, max, val = 0, len;
   char *insert;
   const char *curr_str;
   int next_idx = 0;

   EINA_SAFETY_ON_NULL_RETURN(text);
   popup_mod = (Popup_Module_Data *)data;
   if (!popup_mod) return;

   insert = *text;
   len = strlen(elm_object_text_get(obj));
   if (len < 3) return;

   curr_str = elm_object_text_get(obj);
   if (curr_str) new_str = _text_insert(curr_str, insert, elm_entry_cursor_pos_get(obj));
   if (new_str) val = atoi(new_str);

   popup_mod->mod_data.field_limit_get(popup_mod->mod_data.base, ELM_DATETIME_YEAR, &min, &max);
   min += STRUCT_TM_YEAR_BASE_VALUE;
   max += STRUCT_TM_YEAR_BASE_VALUE;

   if (val <= max)
     {
        elm_entry_entry_set(obj, new_str);
        elm_entry_cursor_end_set(obj);
        next_idx = _picker_nextfield_location_get(popup_mod, ELM_DATETIME_YEAR);
        if (next_idx != ELM_DATETIME_LAST)
          {
             elm_layout_signal_emit(popup_mod->popup_field[ELM_DATETIME_YEAR],
                                    "elm,action,entry,toggle", "elm");
             entry = elm_object_part_content_get(popup_mod->popup_field[next_idx],
                                                "elm.swallow.entry");
             if (!elm_object_focus_get(entry))
               elm_layout_signal_emit(popup_mod->popup_field[next_idx],
                                      "elm,action,entry,toggle", "elm");
         }
     }

   *insert = 0;
   free((void *)new_str);
   new_str = NULL;
}

static void
_month_validity_checking_filter(void *data, Evas_Object *obj, char **text)
{
   Popup_Module_Data *popup_mod;
   Evas_Object *entry;
   char *new_str = NULL;
   double min, max;
   int val = 0, len, max_digits;
   char *insert;
   const char *curr_str;
   int next_idx = 0;

   EINA_SAFETY_ON_NULL_RETURN(text);
   popup_mod = (Popup_Module_Data *)data;
   if (!popup_mod) return;

   insert = *text;
   len = strlen(elm_object_text_get(obj));
   curr_str = elm_object_text_get(obj);
   if (curr_str) new_str = _text_insert(curr_str, insert, elm_entry_cursor_pos_get(obj));
   if (new_str) val = atoi(new_str) - 1;

   elm_spinner_min_max_get(popup_mod->popup_field[ELM_DATETIME_MONTH], &min, &max);
   min -= 1;
   max -= 1;

   max_digits = (max >= 10 ? 2 : 1);
   if (len < 1 && max_digits > 1 && val < 1)
     {
        free((void*)new_str);
        new_str = NULL;
        return;
     }

   if ((val >= min) && (val <= max))
     {
        elm_entry_entry_set(obj, new_str);
        elm_entry_cursor_end_set(obj);
        next_idx = _picker_nextfield_location_get(popup_mod, ELM_DATETIME_MONTH);
        if (next_idx != DATETIME_FIELD_COUNT)
          {
             elm_layout_signal_emit(popup_mod->popup_field[ELM_DATETIME_MONTH],
                                    "elm,action,entry,toggle", "elm");
             entry = elm_object_part_content_get(popup_mod->popup_field[next_idx],
                                                 "elm.swallow.entry");
             if (!elm_object_focus_get(entry))
               elm_layout_signal_emit(popup_mod->popup_field[next_idx],
                                      "elm,action,entry,toggle", "elm");
         }
     }
   *insert = 0;
   free((void *)new_str);
   new_str = NULL;
}

static void
_hour_validity_checking_filter(void *data, Evas_Object *obj, char **text)
{
   Popup_Module_Data *popup_mod;
   Evas_Object *entry;
   char *new_str = NULL;
   int val = 0, len;
   char *insert;
   const char *curr_str;
   double min, max;

   EINA_SAFETY_ON_NULL_RETURN(text);

   popup_mod = (Popup_Module_Data *)data;
   if (!popup_mod) return;

   insert = *text;
   len = strlen(elm_object_text_get(obj));
   if (len < 1) return;

   curr_str = elm_object_text_get(obj);
   if (curr_str) new_str = _text_insert(curr_str, insert, elm_entry_cursor_pos_get(obj));
   if (new_str) val = atoi(new_str);

   if (popup_mod->time_12hr_fmt)
     {
        if ((val == 0) || (val > STRUCT_TM_TIME_12HRS_MAX_VALUE))
          {
             *insert = 0;
             free((void *)new_str);
             new_str = NULL;
             return;
          }

        if ((popup_mod->is_pm) && (val != STRUCT_TM_TIME_12HRS_MAX_VALUE))
          val += STRUCT_TM_TIME_12HRS_MAX_VALUE;
     }

   elm_spinner_min_max_get(popup_mod->popup_field[ELM_DATETIME_HOUR], &min, &max);
   if ((val >= min) && (val <= max))
     {
       elm_entry_entry_set(obj, new_str);
       elm_entry_cursor_end_set(obj);
       elm_layout_signal_emit(popup_mod->popup_field[ELM_DATETIME_HOUR],
                              "elm,action,entry,toggle", "elm");
       edje_object_message_signal_process(elm_layout_edje_get(popup_mod->popup_field[ELM_DATETIME_HOUR]));
       elm_spinner_value_set(popup_mod->popup_field[ELM_DATETIME_HOUR], val);

       entry = elm_object_part_content_get(popup_mod->popup_field[ELM_DATETIME_MINUTE],
                                                     "elm.swallow.entry");
       if (!elm_object_focus_get(entry))
         {
            elm_layout_signal_emit(popup_mod->popup_field[ELM_DATETIME_MINUTE],
                                            "elm,action,entry,toggle", "elm");
         }
     }

    *insert = 0;
    free((void *)new_str);
    new_str = NULL;
}

static void
_date_validity_checking_filter(void *data, Evas_Object *obj, char **text)
{
   Popup_Module_Data *popup_mod;
   Evas_Object *entry;
   char *new_str = NULL;
   int val = 0, len;
   const char *curr_str;
   char *insert;
   double min, max;
   int next_idx = 0;

   EINA_SAFETY_ON_NULL_RETURN(text);
   popup_mod = (Popup_Module_Data *)data;
   if (!popup_mod) return;

   insert = *text;
   len = strlen(elm_object_text_get(obj));
   if (len < 1) return;

   curr_str = elm_object_text_get(obj);
   if (curr_str) new_str = _text_insert(curr_str, insert, elm_entry_cursor_pos_get(obj));
   if (new_str) val = atoi(new_str);
   elm_spinner_min_max_get(popup_mod->popup_field[ELM_DATETIME_DATE], &min, &max);

   if ((val >= min) && (val <= max))
     {
       elm_entry_entry_set(obj, new_str);
       elm_entry_cursor_end_set(obj);
       next_idx = _picker_nextfield_location_get(popup_mod, ELM_DATETIME_DATE);
       if (next_idx != ELM_DATETIME_LAST)
         {
            elm_layout_signal_emit(popup_mod->popup_field[ELM_DATETIME_DATE],
                                   "elm,action,entry,toggle", "elm");
            entry = elm_object_part_content_get(popup_mod->popup_field[next_idx],
                                                "elm.swallow.entry");
            if (!elm_object_focus_get(entry))
              elm_layout_signal_emit(popup_mod->popup_field[next_idx],
                                        "elm,action,entry,toggle", "elm");
         }
     }
   *insert = 0;
   free((void *)new_str);
   new_str = NULL;
}

static void
_minute_validity_checking_filter(void *data, Evas_Object *obj, char **text)
{
   Popup_Module_Data *popup_mod;
   char *new_str = NULL;
   int val = 0, len;
   double min, max;
   char *insert;
   const char *curr_str;

   EINA_SAFETY_ON_NULL_RETURN(text);
   popup_mod = (Popup_Module_Data *)data;
   if (!popup_mod) return;

   insert = *text;
   len = strlen(elm_object_text_get(obj));
   if (len < 1) return;

   curr_str = elm_object_text_get(obj);
   if (curr_str) new_str = _text_insert(curr_str, insert, elm_entry_cursor_pos_get(obj));
   if (new_str) val = atoi(new_str);

   elm_spinner_min_max_get(popup_mod->popup_field[ELM_DATETIME_MINUTE], &min, &max);

   if ((val >= min) && (val <= max))
     {
       elm_entry_entry_set(obj, new_str);
       elm_entry_cursor_end_set(obj);
     }
   *insert = 0;
   free((void *)new_str);
   new_str = NULL;
}

static void
_set_datepicker_entry_filter(Popup_Module_Data *popup_mod)
{
   Evas_Object *spinner, *entry;
   static Elm_Entry_Filter_Accept_Set digits_filter_data;
   static Elm_Entry_Filter_Limit_Size limit_filter_data;
   char buf[BUFF_SIZE];
   int idx, value;

   if (!popup_mod) return;

   digits_filter_data.accepted = "0123456789";
   digits_filter_data.rejected = NULL;

   limit_filter_data.max_byte_count = 0;

   for (idx = 0; idx <= ELM_DATETIME_DATE; idx++)
     {
       spinner = popup_mod->popup_field[idx];
       entry = elm_object_part_content_get(spinner, "elm.swallow.entry");
       if (!entry) continue;

       snprintf(buf, sizeof(buf), "datetime_popup/%s", field_styles[idx]);
       elm_object_style_set(entry, buf);
       elm_entry_magnifier_disabled_set(entry, EINA_TRUE);
       elm_entry_context_menu_disabled_set(entry, EINA_TRUE);

       if (idx == ELM_DATETIME_MONTH)
         {
            value = (int)elm_spinner_value_get(popup_mod->popup_field[ELM_DATETIME_MONTH]) - 1;
            elm_object_text_set(entry, month_arr[value]);
         }

       elm_entry_markup_filter_append(entry, elm_entry_filter_accept_set, &digits_filter_data);

       if (idx == ELM_DATETIME_YEAR)
         limit_filter_data.max_char_count = 4;
       else
         limit_filter_data.max_char_count = 2;

       elm_entry_markup_filter_append(entry, elm_entry_filter_limit_size, &limit_filter_data);

       evas_object_smart_callback_add(entry, "focused", _entry_focused_cb, popup_mod);
       evas_object_smart_callback_add(entry, "unfocused", _entry_unfocused_cb, popup_mod);
       evas_object_smart_callback_add(entry, "activated", _entry_activated_cb, popup_mod);
       evas_object_smart_callback_add(entry, "clicked", _entry_clicked_cb, popup_mod);

       if (idx == ELM_DATETIME_YEAR)
         elm_entry_markup_filter_append(entry, _year_validity_checking_filter, popup_mod);
       else if (idx == ELM_DATETIME_MONTH)
         elm_entry_markup_filter_append(entry, _month_validity_checking_filter, popup_mod);
       else if (idx == ELM_DATETIME_DATE)
         elm_entry_markup_filter_append(entry, _date_validity_checking_filter, popup_mod);
     }
}

static void
_set_timepicker_entry_filter(Popup_Module_Data *popup_mod)
{
   Evas_Object *spinner, *entry;
   static Elm_Entry_Filter_Accept_Set digits_filter_data;
   static Elm_Entry_Filter_Limit_Size limit_filter_data;
   char buf[BUFF_SIZE];
   int idx;

   if (!popup_mod) return;

   digits_filter_data.accepted = "0123456789";
   digits_filter_data.rejected = NULL;

   limit_filter_data.max_byte_count = 0;

   for (idx = ELM_DATETIME_HOUR; idx < ELM_DATETIME_AMPM; idx++)
     {
       spinner = popup_mod->popup_field[idx];
       entry = elm_object_part_content_get(spinner, "elm.swallow.entry");
       if (!entry) continue;

       snprintf(buf, sizeof(buf), "datetime_popup/%s", field_styles[idx]);
       elm_object_style_set(entry, buf);
       elm_entry_magnifier_disabled_set(entry, EINA_TRUE);
       elm_entry_context_menu_disabled_set(entry, EINA_TRUE);
       elm_entry_markup_filter_append(entry, elm_entry_filter_accept_set, &digits_filter_data);

       limit_filter_data.max_char_count = 2;
       elm_entry_markup_filter_append(entry, elm_entry_filter_limit_size, &limit_filter_data);

       evas_object_smart_callback_add(entry, "focused", _entry_focused_cb, popup_mod);
       evas_object_smart_callback_add(entry, "unfocused", _entry_unfocused_cb, popup_mod);
       evas_object_smart_callback_add(entry, "activated", _entry_activated_cb, popup_mod);
       evas_object_smart_callback_add(entry, "clicked", _entry_clicked_cb, popup_mod);

       if (idx == ELM_DATETIME_HOUR)
         elm_entry_markup_filter_append(entry, _hour_validity_checking_filter, popup_mod);
       else if (idx == ELM_DATETIME_MINUTE)
         elm_entry_markup_filter_append(entry, _minute_validity_checking_filter, popup_mod);
     }
}

static void
_show_datepicker_layout(Popup_Module_Data *popup_mod)
{
   Evas_Object *content;
   struct tm curr_time;
   int idx, year, month, min, max;
   Elm_Datetime_Field_Type  field_type = ELM_DATETIME_YEAR;

   if (!popup_mod || !popup_mod->mod_data.base) return;

   content = elm_object_content_get(popup_mod->popup);
   if (content != popup_mod->datepicker_layout)
     {
         elm_object_content_unset(popup_mod->popup);
         if (content) evas_object_hide(content);
         elm_object_content_set(popup_mod->popup, popup_mod->datepicker_layout);
         evas_object_show(popup_mod->datepicker_layout);
     }
   elm_datetime_value_get(popup_mod->mod_data.base, &(popup_mod->set_time));
   _set_datepicker_popup_title_text(popup_mod);

   elm_datetime_value_get(popup_mod->mod_data.base, &curr_time);
   year = curr_time.tm_year + STRUCT_TM_YEAR_BASE_VALUE;
   elm_spinner_value_set(popup_mod->popup_field[ELM_DATETIME_YEAR], year);
   month = curr_time.tm_mon + 1;
   elm_spinner_value_set(popup_mod->popup_field[ELM_DATETIME_MONTH], month);
   elm_spinner_value_set(popup_mod->popup_field[ELM_DATETIME_DATE], curr_time.tm_mday);

   for (idx = 0; idx <= ELM_DATETIME_DATE; idx++)
     {
         field_type = (Elm_Datetime_Field_Type)idx;
         popup_mod->mod_data.field_limit_get(popup_mod->mod_data.base, field_type, &min, &max);
         if (idx == ELM_DATETIME_YEAR)
           elm_spinner_min_max_set(popup_mod->popup_field[idx], (STRUCT_TM_YEAR_BASE_VALUE + min), (STRUCT_TM_YEAR_BASE_VALUE + max));
         else if (idx == ELM_DATETIME_MONTH)
           elm_spinner_min_max_set(popup_mod->popup_field[idx], (1+min), (1+max));
         else
           elm_spinner_min_max_set(popup_mod->popup_field[idx], min, max);
     }
}

static void
_show_timepicker_layout(Popup_Module_Data *popup_mod)
{
   Evas_Object *content;
   struct tm curr_time, min_time, max_time;

   if (!popup_mod || !popup_mod->mod_data.base) return;

   content = elm_object_content_get(popup_mod->popup);
   if (content != popup_mod->timepicker_layout)
     {
         elm_object_content_unset(popup_mod->popup);
         if (content) evas_object_hide(content);
         elm_object_content_set(popup_mod->popup, popup_mod->timepicker_layout);
         evas_object_show(popup_mod->timepicker_layout);
     }
   elm_datetime_value_get(popup_mod->mod_data.base, &(popup_mod->set_time));
   _set_timepicker_popup_title_text(popup_mod);

   elm_datetime_value_get(popup_mod->mod_data.base, &curr_time);
   popup_mod->is_pm = (curr_time.tm_hour < STRUCT_TM_TIME_12HRS_MAX_VALUE) ? 0 : 1;

   popup_mod->mod_data.fields_min_max_get(popup_mod->mod_data.base, &curr_time, &min_time, &max_time);
   elm_spinner_min_max_set(popup_mod->popup_field[ELM_DATETIME_HOUR],
                            min_time.tm_hour, max_time.tm_hour);
   elm_spinner_min_max_set(popup_mod->popup_field[ELM_DATETIME_MINUTE],
                            min_time.tm_min, max_time.tm_min);

   elm_spinner_value_set(popup_mod->popup_field[ELM_DATETIME_HOUR], curr_time.tm_hour);
   elm_spinner_value_set(popup_mod->popup_field[ELM_DATETIME_MINUTE], curr_time.tm_min);

   if (popup_mod->time_12hr_fmt)
     _set_ampm_value(popup_mod);
}

static void
_set_month_special_values(Popup_Module_Data *popup_mod)
{
   struct tm curr_time;
   const char *fmt = NULL;
   int month;

   if (!popup_mod) return;

   elm_datetime_value_get(popup_mod->mod_data.base, &curr_time);
   fmt = popup_mod->mod_data.field_format_get(popup_mod->mod_data.base, ELM_DATETIME_MONTH);
   for (month = 0; month < TOTAL_NUMBER_OF_MONTHS; month++)
     {
         curr_time.tm_mon = month;
         strftime(month_arr[month], MONTH_STRING_MAX_SIZE, fmt, &curr_time);
     }
   for (month = 0; month < 12; month++)
     elm_spinner_special_value_add(popup_mod->popup_field[ELM_DATETIME_MONTH],
                                   month + 1, month_arr[month]);
}

static void
_set_hour_12hrfmt_special_values(Popup_Module_Data *popup_mod)
{
   char buf[BUFF_SIZE];
   double hour;

   if (!popup_mod) return;

   for (hour = STRUCT_TM_TIME_12HRS_MAX_VALUE +1; hour <= STRUCT_TM_TIME_24HRS_MAX_VALUE; hour++)
     {
         snprintf(buf, sizeof(buf), "%02.0f", hour-12);
         elm_spinner_special_value_add(popup_mod->popup_field[ELM_DATETIME_HOUR], hour, buf);
     }

   elm_spinner_special_value_add(popup_mod->popup_field[ELM_DATETIME_HOUR], 0, "12");
}

static void
_unset_hour_12hrfmt_special_values(Popup_Module_Data *popup_mod)
{
   int hour;
   if (!popup_mod) return;

   for (hour = STRUCT_TM_TIME_12HRS_MAX_VALUE +1; hour <= STRUCT_TM_TIME_24HRS_MAX_VALUE; hour++)
     elm_spinner_special_value_del(popup_mod->popup_field[ELM_DATETIME_HOUR], hour);

   elm_spinner_special_value_del(popup_mod->popup_field[ELM_DATETIME_HOUR], 0);
}

static void
_create_datetime_popup(Popup_Module_Data *popup_mod)
{
   Evas_Object *set_btn, *cancel_btn;
   Evas_Object *parent, *widget, *naviframe = NULL;
   const char *widget_type;

   if (!popup_mod) return;

   // add popup to the content of elm_conformant
   widget = popup_mod->mod_data.base;
   while(widget != NULL)
     {
        parent = elm_widget_parent_get(widget);
        widget_type = elm_widget_type_get(widget);
        if (!strcmp(widget_type, "elm_naviframe"))
          {
             naviframe = widget;
             break;
          }
        widget = parent;
     }

   if (popup_mod->is_custom_popup)
     {
       /*In case of custom center popup, module popup should be added to the same canvas*/
       if (popup_mod->is_center_popup)
         popup_mod->popup = elm_popup_add(popup_mod->center_popup_win);
       else
         {
            popup_mod->popup = elm_popup_add(popup_mod->mod_data.base);
            if (naviframe)
              elm_popup_parent_set(popup_mod->popup, naviframe);
            else
              elm_popup_parent_set(popup_mod->popup, elm_widget_top_get(popup_mod->mod_data.base));
            evas_object_size_hint_weight_set(popup_mod->popup, EVAS_HINT_EXPAND,
                                   EVAS_HINT_EXPAND);
            evas_object_size_hint_align_set(popup_mod->popup, EVAS_HINT_FILL, 0.5);
         }
     }
   else
     {
        #ifdef ELM_FEATURE_MULTI_WINDOW
             popup_mod->popup = _center_popup_add(popup_mod->mod_data.base);
        #else
             popup_mod->popup = elm_popup_add(popup_mod->mod_data.base);
             if (naviframe)
               elm_popup_parent_set(popup_mod->popup, naviframe);
             else
               elm_popup_parent_set(popup_mod->popup, elm_widget_top_get(popup_mod->mod_data.base));
             evas_object_size_hint_weight_set(popup_mod->popup, EVAS_HINT_EXPAND,
                                     EVAS_HINT_EXPAND);
             evas_object_size_hint_align_set(popup_mod->popup, EVAS_HINT_FILL, 0.5);
        #endif
     }

   Evas_Object *ao = (Evas_Object*)edje_object_part_object_get(elm_layout_edje_get(popup_mod->popup), "access.title");
   elm_access_object_register(ao, popup_mod->popup);
   elm_access_info_cb_set(ao, ELM_ACCESS_TYPE, NULL, NULL);

   cancel_btn = elm_button_add(popup_mod->popup);
   elm_object_style_set(cancel_btn, "popup");
   elm_object_domain_translatable_text_set(cancel_btn, PACKAGE, "IDS_COM_SK_CANCEL");
   elm_object_part_content_set(popup_mod->popup, "button1", cancel_btn);
   evas_object_smart_callback_add(cancel_btn, "clicked", _popup_cancel_btn_clicked_cb, popup_mod);

   set_btn = elm_button_add(popup_mod->popup);
   elm_object_style_set(set_btn, "popup");
   elm_object_domain_translatable_text_set(set_btn, PACKAGE, "IDS_COM_POP_SET");
   elm_object_part_content_set(popup_mod->popup, "button2", set_btn);
   evas_object_smart_callback_add(set_btn, "clicked", _popup_set_btn_clicked_cb, popup_mod);
}

static char *
_access_date_picker_state_cb(void *data, Evas_Object *obj __UNUSED__)
{
   char buf[BUFF_SIZE];
   int curr = (int)data;
   switch(curr)
     {
        case ELM_DATETIME_YEAR:
          sprintf(buf, E_("IDS_SCR_BODY_FLICK_UP_AND_DOWN_TO_ADJUST_PS_OR_DOUBLE_TAP_TO_OPEN_KEYBOARD_T_TTS"), E_("WDS_ST_OPT_YEAR_ABB"));
          return strdup(buf);
        case ELM_DATETIME_MONTH:
          sprintf(buf, E_("IDS_SCR_BODY_FLICK_UP_AND_DOWN_TO_ADJUST_PS_OR_DOUBLE_TAP_TO_OPEN_KEYBOARD_T_TTS"), E_("WDS_ST_OPT_MONTH_ABB"));
          return strdup(buf);
        case ELM_DATETIME_DATE:
          sprintf(buf, E_("IDS_SCR_BODY_FLICK_UP_AND_DOWN_TO_ADJUST_PS_OR_DOUBLE_TAP_TO_OPEN_KEYBOARD_T_TTS"), E_("IDS_ACCS_BODY_DATE_TTS"));
          return strdup(buf);
        default:
          break;
      }
   return NULL;
}

static char *
_access_time_picker_state_cb(void *data, Evas_Object *obj __UNUSED__)
{
   char buf[BUFF_SIZE];
   int curr = (int)data;
   switch(curr)
     {
       case ELM_DATETIME_HOUR:
          sprintf(buf, E_("IDS_SCR_BODY_FLICK_UP_AND_DOWN_TO_ADJUST_PS_OR_DOUBLE_TAP_TO_OPEN_KEYBOARD_T_TTS"), E_("IDS_TPLATFORM_BODY_HOUR_LC"));
          return strdup(buf);
       case ELM_DATETIME_MINUTE:
          sprintf(buf, E_("IDS_SCR_BODY_FLICK_UP_AND_DOWN_TO_ADJUST_PS_OR_DOUBLE_TAP_TO_OPEN_KEYBOARD_T_TTS"), E_("IDS_COM_BODY_MINUTE"));
          return strdup(buf);
       default:
          break;
     }
   return NULL;
}

static void
_date_picker_layout_del_cb(void *data, Evas * e __UNUSED__, Evas_Object * obj __UNUSED__, void *event_info __UNUSED__)
{
   Popup_Module_Data *popup_mod;

   popup_mod = (Popup_Module_Data *)data;
   if (!popup_mod) return;

   if (popup_mod->datepicker_layout)
     popup_mod->datepicker_layout = NULL;

   if (popup_mod->popup)
     {
       evas_object_del(popup_mod->popup);
       popup_mod->popup = NULL;
     }
}

static void
_time_picker_layout_del_cb(void *data, Evas * e __UNUSED__, Evas_Object * obj __UNUSED__, void *event_info __UNUSED__)
{
   Popup_Module_Data *popup_mod;

   popup_mod = (Popup_Module_Data *)data;
   if (!popup_mod) return;

   if (popup_mod->timepicker_layout)
     popup_mod->timepicker_layout = NULL;

   if (popup_mod->popup)
     {
       evas_object_del(popup_mod->popup);
       popup_mod->popup = NULL;
     }
}

static void
_create_datepicker_layout(Popup_Module_Data *popup_mod)
{
   Evas_Object *spinner, *entry, *ao;
   char buf[BUFF_SIZE];
   int idx, loc;

   if (!popup_mod || !popup_mod->popup) return;

   popup_mod->datepicker_layout = elm_layout_add(popup_mod->popup);
   elm_layout_theme_set(popup_mod->datepicker_layout, "layout", "datetime_popup", "date_layout");

   int loc_content[ELM_DATETIME_DATE + 1];
   for (idx = 0; idx <= ELM_DATETIME_DATE; idx++)
     {
        popup_mod->mod_data.field_location_get(popup_mod->mod_data.base, idx, &loc);
        loc_content[loc] = idx;
     }

   for (loc = 0; loc <= ELM_DATETIME_DATE; loc++)
     {
        idx = loc_content[loc];
        spinner = elm_spinner_add(popup_mod->popup);
        elm_spinner_editable_set(spinner, EINA_TRUE);
        elm_object_style_set(spinner, "vertical");
        if (_elm_config->access_mode == ELM_ACCESS_MODE_ON)
          {
             ao = _access_object_get(spinner, "access");
             _elm_access_callback_set(_elm_access_object_get(ao), ELM_ACCESS_STATE, _access_date_picker_state_cb, (void *)idx);
          }
        elm_spinner_step_set(spinner, 1);
        elm_spinner_wrap_set(spinner, EINA_TRUE);
        elm_spinner_label_format_set(spinner, "%02.0f");
        if (idx == ELM_DATETIME_MONTH)
          elm_spinner_interval_set(spinner, 0.2);
        else
          elm_spinner_interval_set(spinner, 0.1);
        snprintf(buf, sizeof(buf), "field%d", loc);
        popup_mod->field_location[idx] = loc;
        elm_object_part_content_set(popup_mod->datepicker_layout, buf, spinner);

        entry = elm_object_part_content_get(spinner, "elm.swallow.entry");
        elm_entry_input_panel_layout_set(entry, ELM_INPUT_PANEL_LAYOUT_DATETIME);
        elm_entry_drag_disabled_set(entry, EINA_TRUE);
        if (loc == (PICKER_POPUP_FIELD_COUNT - 1))
          elm_entry_input_panel_return_key_type_set(entry, ELM_INPUT_PANEL_RETURN_KEY_TYPE_DONE);
        else
          elm_entry_input_panel_return_key_type_set(entry, ELM_INPUT_PANEL_RETURN_KEY_TYPE_NEXT);

        if (idx == ELM_DATETIME_YEAR)
          elm_spinner_min_max_set(spinner, 1902, 2037);
        else if (idx == ELM_DATETIME_MONTH)
          elm_spinner_min_max_set(spinner, 1, 12);
        else if (idx == ELM_DATETIME_DATE)
          elm_spinner_min_max_set(spinner, 1, 31);

        evas_object_smart_callback_add(spinner, "changed", _datepicker_value_changed_cb, popup_mod);
        popup_mod->popup_field[idx] = spinner;
     }

   _set_month_special_values(popup_mod);
   _set_datepicker_entry_filter(popup_mod);

   if (popup_mod->is_custom_popup)
     evas_object_event_callback_add(popup_mod->datepicker_layout, EVAS_CALLBACK_DEL, _date_picker_layout_del_cb, popup_mod);
}

static char *
_access_ampm_info_cb(void *data __UNUSED__, Evas_Object *obj __UNUSED__)
{
   return strdup(E_("WDS_TTS_TBBODY_DOUBLE_TAP_TO_CHANGE"));
}

static void
_create_timepicker_layout(Popup_Module_Data *popup_mod)
{
   Evas_Object *spinner, *ampm, *entry, *ao;
   char buf[BUFF_SIZE];
   int idx;

   if (!popup_mod || !popup_mod->popup) return;

   popup_mod->timepicker_layout = elm_layout_add(popup_mod->popup);
   popup_mod->time_12hr_fmt = popup_mod->mod_data.field_location_get(popup_mod->mod_data.base, ELM_DATETIME_AMPM, NULL);

   if (popup_mod->time_12hr_fmt)
     elm_layout_theme_set(popup_mod->timepicker_layout, "layout", "datetime_popup", "time_layout");
   else
     elm_layout_theme_set(popup_mod->timepicker_layout, "layout", "datetime_popup", "time_layout_24hr");

   for (idx = ELM_DATETIME_HOUR; idx < ELM_DATETIME_AMPM; idx++)
     {
       spinner = elm_spinner_add(popup_mod->popup);
       elm_spinner_editable_set(spinner, EINA_TRUE);
       elm_object_style_set(spinner, "vertical");
       if (_elm_config->access_mode == ELM_ACCESS_MODE_ON)
         {
            ao = _access_object_get(spinner, "access");
            _elm_access_callback_set(_elm_access_object_get(ao), ELM_ACCESS_STATE, _access_time_picker_state_cb, (void *)idx);
         }
       elm_spinner_step_set(spinner, 1);
       elm_spinner_wrap_set(spinner, EINA_TRUE);
       elm_spinner_label_format_set(spinner, "%02.0f");
       if (idx == ELM_DATETIME_HOUR)
         elm_spinner_interval_set(spinner, 0.2);
       else
         elm_spinner_interval_set(spinner, 0.1);
       snprintf(buf, sizeof(buf), "field%d", (idx - ELM_DATETIME_HOUR));
       elm_object_part_content_set(popup_mod->timepicker_layout, buf, spinner);
       if (idx == ELM_DATETIME_HOUR)
         evas_object_smart_callback_add(spinner, "entry,apply", _hour_validity_check, popup_mod);
       evas_object_smart_callback_add(spinner, "changed", _timepicker_value_changed_cb, popup_mod);
       popup_mod->popup_field[idx] = spinner;
       entry = elm_object_part_content_get(spinner, "elm.swallow.entry");
       elm_entry_input_panel_layout_set(entry, ELM_INPUT_PANEL_LAYOUT_DATETIME);
       elm_entry_drag_disabled_set(entry, EINA_TRUE);
       if (idx == ELM_DATETIME_MINUTE)
         elm_entry_input_panel_return_key_type_set(entry, ELM_INPUT_PANEL_RETURN_KEY_TYPE_DONE);
       else
         elm_entry_input_panel_return_key_type_set(entry, ELM_INPUT_PANEL_RETURN_KEY_TYPE_NEXT);
    }

   ampm = elm_button_add(popup_mod->popup);
   elm_object_style_set(ampm, "datetime/ampm");
   evas_object_smart_callback_add(ampm, "clicked", _ampm_clicked_cb, popup_mod);
   elm_object_part_content_set(popup_mod->timepicker_layout, "field2",ampm);
   elm_access_info_cb_set(ampm, ELM_ACCESS_TYPE,
                          _access_ampm_info_cb, NULL);
   popup_mod->popup_field[ELM_DATETIME_AMPM] = ampm;

   elm_spinner_min_max_set(popup_mod->popup_field[ELM_DATETIME_HOUR], 0, 23);
   elm_spinner_min_max_set(popup_mod->popup_field[ELM_DATETIME_MINUTE], 0, 59);

   if (popup_mod->time_12hr_fmt)
     _set_hour_12hrfmt_special_values(popup_mod);

   _set_timepicker_entry_filter(popup_mod);

   if (popup_mod->is_custom_popup)
     evas_object_event_callback_add(popup_mod->timepicker_layout, EVAS_CALLBACK_DEL, _time_picker_layout_del_cb, popup_mod);
}

static void
_weekday_loc_update(Popup_Module_Data *popup_mod)
{
   char *fmt;

   if (!popup_mod) return;

   fmt = nl_langinfo(D_T_FMT);
   if (!strncmp(fmt, "%a", 2) || !strncmp(fmt, "%A", 2))
     popup_mod->weekday_loc_first = EINA_TRUE;
   else
     popup_mod->weekday_loc_first = EINA_FALSE;
}

static void
_launch_datepicker(void *data, Evas_Object * obj __UNUSED__, void *event_info __UNUSED__)
{
   Popup_Module_Data *popup_mod;

   popup_mod = (Popup_Module_Data *)data;
   if (!popup_mod) return;

   if (!popup_mod->popup)
     _create_datetime_popup(popup_mod);

   if (!popup_mod->datepicker_layout)
     _create_datepicker_layout(popup_mod);

   _show_datepicker_layout(popup_mod);
   if (!popup_mod->is_custom_popup)
     {
        evas_object_show(popup_mod->popup);
        evas_object_smart_callback_call(popup_mod->mod_data.base, SIG_EDIT_START, NULL);
     }
#ifdef ELM_FEATURE_MULTI_WINDOW
   _popup_grab_back_key(popup_mod);
#endif
}

static void
_launch_timepicker(void *data, Evas_Object * obj __UNUSED__, void *event_info __UNUSED__)
{
   Popup_Module_Data *popup_mod;

   popup_mod = (Popup_Module_Data *)data;
   if (!popup_mod) return;

   if (!popup_mod->popup)
     _create_datetime_popup(popup_mod);

   if (!popup_mod->timepicker_layout)
     _create_timepicker_layout(popup_mod);

   _show_timepicker_layout(popup_mod);
   if (!popup_mod->is_custom_popup)
     {
        evas_object_show(popup_mod->popup);
        evas_object_smart_callback_call(popup_mod->mod_data.base, SIG_EDIT_START, NULL);
     }
#ifdef ELM_FEATURE_MULTI_WINDOW
   _popup_grab_back_key(popup_mod);
#endif
}

static void
_custom_picker_layout_create_cb(void *data, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   Popup_Module_Data *popup_mod;
   Evas_Object *datetime;
   popup_mod = (Popup_Module_Data *)data;
   if (!popup_mod) return;

   datetime = popup_mod->mod_data.base;
   if (elm_object_part_content_get(datetime, "date.btn"))
     {
        _launch_datepicker(popup_mod, NULL, NULL);
        evas_object_smart_callback_call(datetime, SIG_DATE_PICKER_LAYOUT_GET, elm_object_content_get(popup_mod->popup));
     }
   if (elm_object_part_content_get(datetime, "time.btn"))
     {
        _launch_timepicker(popup_mod, NULL, NULL);
        evas_object_smart_callback_call(datetime, SIG_TIME_PICKER_LAYOUT_GET, elm_object_content_get(popup_mod->popup));
     }
}

static void
_module_format_change(Popup_Module_Data *popup_mod)
{
   Evas_Object *datetime, *spinner, *entry;
   int field_loc[PICKER_POPUP_FIELD_COUNT];
   char buf[BUFF_SIZE];
   int idx, loc;
   Eina_Bool show_date_btn = EINA_FALSE;
   Eina_Bool show_time_btn = EINA_FALSE;
   Eina_Bool time_12hr;

   if (!popup_mod) return;

   datetime = popup_mod->mod_data.base;
   if (popup_mod->datepicker_layout)
     {
        for (idx = 0; idx <= ELM_DATETIME_DATE; idx++)
          {
             popup_mod->mod_data.field_location_get(datetime, idx, &loc);
             snprintf(buf, sizeof(buf), "field%d", popup_mod->field_location[idx]);
             elm_object_part_content_unset(popup_mod->datepicker_layout, buf);
             field_loc[idx] = loc;
          }
        for (idx = 0; idx <= ELM_DATETIME_DATE; idx++)
          {
             snprintf(buf, sizeof(buf), "field%d", field_loc[idx]);
             elm_object_part_content_set(popup_mod->datepicker_layout, buf, popup_mod->popup_field[idx]);
             popup_mod->field_location[idx] = field_loc[idx];
          }
        for (idx = 0; idx <= ELM_DATETIME_DATE; idx++)
          {
             entry = elm_object_part_content_get(popup_mod->popup_field[idx], "elm.swallow.entry");
             if (popup_mod->field_location[idx] == (PICKER_POPUP_FIELD_COUNT - 1))
               elm_entry_input_panel_return_key_type_set(entry, ELM_INPUT_PANEL_RETURN_KEY_TYPE_DONE);
             else
               elm_entry_input_panel_return_key_type_set(entry, ELM_INPUT_PANEL_RETURN_KEY_TYPE_NEXT);
          }
     }
   for (idx = 0; idx <= ELM_DATETIME_DATE; idx++)
     show_date_btn |= popup_mod->mod_data.field_location_get(datetime, idx, NULL);
   if (show_date_btn)
     {
        elm_object_part_content_set(datetime, "date.btn", popup_mod->date_btn);
        elm_layout_signal_emit(datetime, "datepicker,show", "elm");
        popup_mod->dateBtn_is_visible = EINA_TRUE;
     }
   else
     {
        elm_object_part_content_unset(datetime, "date.btn");
        evas_object_hide(popup_mod->date_btn);
        elm_layout_signal_emit(datetime, "datepicker,hide", "elm");
        popup_mod->dateBtn_is_visible = EINA_FALSE;
     }

   for (idx = ELM_DATETIME_HOUR; idx < ELM_DATETIME_AMPM; idx++)
     show_time_btn |= popup_mod->mod_data.field_location_get(datetime, idx, NULL);
   if (show_time_btn)
     {
        elm_object_part_content_set(datetime, "time.btn", popup_mod->time_btn);
        elm_layout_signal_emit(datetime, "timepicker,show", "elm");
        popup_mod->timeBtn_is_visible = EINA_TRUE;
     }
   else
     {
        elm_object_part_content_unset(datetime, "time.btn");
        evas_object_hide(popup_mod->time_btn);
        elm_layout_signal_emit(datetime, "timepicker,hide", "elm");
        popup_mod->timeBtn_is_visible = EINA_FALSE;
     }

   time_12hr = popup_mod->mod_data.field_location_get(datetime, ELM_DATETIME_AMPM, NULL);
   if ((popup_mod->time_12hr_fmt != time_12hr) && (popup_mod->timepicker_layout))
     {
       for (idx = ELM_DATETIME_HOUR; idx <= ELM_DATETIME_AMPM; idx++)
         {
            snprintf(buf, sizeof(buf), "field%d", (idx - ELM_DATETIME_HOUR));
            spinner = elm_object_part_content_unset(popup_mod->timepicker_layout, buf);
            if (spinner) evas_object_hide(spinner);
         }
        if (time_12hr)
          {
             elm_layout_theme_set(popup_mod->timepicker_layout, "layout", "datetime_popup", "time_layout");
             for (idx = ELM_DATETIME_HOUR; idx <= ELM_DATETIME_AMPM; idx++)
               {
                  snprintf(buf, sizeof(buf), "field%d", (idx - ELM_DATETIME_HOUR));
                  elm_object_part_content_set(popup_mod->timepicker_layout, buf, popup_mod->popup_field[idx]);
               }
             _set_hour_12hrfmt_special_values(popup_mod);
          }
        else
          {
             elm_layout_theme_set(popup_mod->timepicker_layout, "layout", "datetime_popup", "time_layout_24hr");
             for (idx = ELM_DATETIME_HOUR; idx <= ELM_DATETIME_MINUTE; idx++)
               {
                  snprintf(buf, sizeof(buf), "field%d", (idx - ELM_DATETIME_HOUR));
                  elm_object_part_content_set(popup_mod->timepicker_layout, buf, popup_mod->popup_field[idx]);
               }
             _unset_hour_12hrfmt_special_values(popup_mod);
          }
     }
   popup_mod->time_12hr_fmt = time_12hr;
   _weekday_loc_update(popup_mod);
   elm_layout_sizing_eval(popup_mod->mod_data.base);
}

// module functions for the specific module type

EAPI void
display_fields(Elm_Datetime_Module_Data *module_data)
{
   Popup_Module_Data *popup_mod;
   struct tm curr_time;
   char date_str[BUFF_SIZE] = {0,};
   char time_str[BUFF_SIZE] = {0,};
   char buf[BUFF_SIZE] = {0,};
   const char *fmt;
   int i, idx, loc;
   Eina_Bool is_visible;

   popup_mod = (Popup_Module_Data *)module_data;
   if (!popup_mod || !popup_mod->date_btn || !popup_mod->time_btn) return;

   elm_datetime_value_get(popup_mod->mod_data.base, &curr_time);
   if (popup_mod->weekday_show && popup_mod->weekday_loc_first)
     {
        is_visible = popup_mod->mod_data.field_location_get
                           (popup_mod->mod_data.base, ELM_DATETIME_DATE, NULL);
        if (is_visible)
          {
             /* FIXME: To restrict month wrapping because of summer time in some locales,
              * ignore day light saving mode in mktime(). */
             curr_time.tm_isdst = -1;
             mktime(&curr_time);
             strftime(buf, sizeof(buf), "%a", &curr_time);
             strcat(date_str, buf);
             strcat(date_str, ", ");
          }
     }
   for (i = 0; i <= ELM_DATETIME_DATE; i++)
     {
        for(idx = 0; idx <= ELM_DATETIME_DATE; idx++)
          {
             is_visible = popup_mod->mod_data.field_location_get
                                   (popup_mod->mod_data.base, idx, &loc);
             if (is_visible && (loc == i))
               {
                  fmt = popup_mod->mod_data.field_format_get
                                 (popup_mod->mod_data.base, idx);
                  /* FIXME: To restrict month wrapping because of summer time in some locales,
                   * ignore day light saving mode in mktime(). */
                  curr_time.tm_isdst = -1;
                  mktime(&curr_time);
                  strftime(buf, sizeof(buf), fmt, &curr_time);
                  if (loc != 0) strcat(date_str, "/");
                  strcat(date_str, buf);
                  break;
               }
          }
     }
   if (popup_mod->weekday_show && !popup_mod->weekday_loc_first)
     {
        is_visible = popup_mod->mod_data.field_location_get
                           (popup_mod->mod_data.base, ELM_DATETIME_DATE, NULL);
        if (is_visible)
          {
             /* FIXME: To restrict month wrapping because of summer time in some locales,
              * ignore day light saving mode in mktime(). */
             curr_time.tm_isdst = -1;
             mktime(&curr_time);
             strftime(buf, sizeof(buf), "%a", &curr_time);
             strcat(date_str, ", ");
             strcat(date_str, buf);
          }
     }

   is_visible = popup_mod->mod_data.field_location_get(popup_mod->mod_data.base, ELM_DATETIME_AMPM, &loc);
   /* AM/PM is comes before hr:min */
   if (is_visible && ((loc == 0) || (loc == 3)))
     {
        fmt = popup_mod->mod_data.field_format_get(popup_mod->mod_data.base, ELM_DATETIME_AMPM);
        curr_time.tm_isdst = -1;
        mktime(&curr_time);
        strftime(buf, sizeof(buf), fmt, &curr_time);
        if (!buf[0] || !strcmp(buf, ""))
          {
             if (curr_time.tm_hour < 12) strcat(time_str, E_("IDS_COM_BODY_AM"));
             else strcat(time_str, E_("IDS_COM_BODY_PM"));
          }
        else
          strcat(time_str, buf);

        strcat(time_str, " ");
     }
   for (idx = ELM_DATETIME_HOUR; idx <= ELM_DATETIME_MINUTE; idx++)
     {
        is_visible = popup_mod->mod_data.field_location_get
                              (popup_mod->mod_data.base, idx, NULL);
        if (is_visible)
          {
             fmt = popup_mod->mod_data.field_format_get
                            (popup_mod->mod_data.base, idx);
             /* FIXME: To restrict month wrapping because of summer time in some locales,
              * ignore day light saving mode in mktime(). */
             curr_time.tm_isdst = -1;
             mktime(&curr_time);
             strftime(buf, sizeof(buf), fmt, &curr_time);
             if (idx == ELM_DATETIME_MINUTE)
               strcat(time_str, ":");

             strcat(time_str, buf);
          }
     }
   is_visible = popup_mod->mod_data.field_location_get(popup_mod->mod_data.base, ELM_DATETIME_AMPM, &loc);
   /* AM/PM is comes after hr:min */
   if (is_visible && (loc != 0) && (loc != 3))
     {
        strcat(time_str, " ");
        fmt = popup_mod->mod_data.field_format_get(popup_mod->mod_data.base, ELM_DATETIME_AMPM);
        curr_time.tm_isdst = -1;
        mktime(&curr_time);
        strftime(buf, sizeof(buf), fmt, &curr_time);
        if (!buf[0] || !strcmp(buf, ""))
          {
             if (curr_time.tm_hour < 12) strcat(time_str, E_("IDS_COM_BODY_AM"));
             else strcat(time_str, E_("IDS_COM_BODY_PM"));
          }
        else
          strcat(time_str, buf);
     }
   elm_object_text_set(popup_mod->date_btn, date_str);
   elm_object_text_set(popup_mod->time_btn, time_str);
}



static void
_datepicker_show_cb(void *data,
                   Evas_Object *obj __UNUSED__,
                   const char *emission __UNUSED__,
                   const char *source __UNUSED__)
{
   _launch_datepicker(data, NULL, NULL);
}

static void
_timepicker_show_cb(void *data,
                   Evas_Object *obj __UNUSED__,
                   const char *emission __UNUSED__,
                   const char *source __UNUSED__)
{
   _launch_timepicker(data, NULL, NULL);
}

static void
_module_language_changed_cb(void *data,
                            Evas_Object *obj __UNUSED__,
                            void *event_info __UNUSED__)
{
   Popup_Module_Data *popup_mod;
   Evas_Object *content;
   popup_mod = (Popup_Module_Data *)data;
   if (!popup_mod) return;

   _weekday_loc_update(popup_mod);
   if (!popup_mod->popup) return;

   content = elm_object_content_get(popup_mod->popup);
   if (content == popup_mod->datepicker_layout)
     _set_datepicker_popup_title_text(popup_mod);
   else if (content == popup_mod->timepicker_layout)
     _set_timepicker_popup_title_text(popup_mod);

   _set_month_special_values(popup_mod);
   _set_ampm_value(popup_mod);
}

static void
_weekday_show_cb(void *data,
                 Evas_Object *obj __UNUSED__,
                 const char *emission __UNUSED__,
                 const char *source __UNUSED__)
{
   Popup_Module_Data *popup_mod;

   popup_mod = (Popup_Module_Data *)data;
   if (!popup_mod) return;

   popup_mod->weekday_show = EINA_TRUE;
   display_fields((Elm_Datetime_Module_Data *)popup_mod);
}

static void
_weekday_hide_cb(void *data,
                 Evas_Object *obj __UNUSED__,
                 const char *emission __UNUSED__,
                 const char *source __UNUSED__)
{
   Popup_Module_Data *popup_mod;

   popup_mod = (Popup_Module_Data *)data;
   if (!popup_mod) return;

   popup_mod->weekday_show = EINA_FALSE;
   display_fields((Elm_Datetime_Module_Data *)popup_mod);
}

static char *
_picker_location_text_get(int curr, struct tm *set_date)
{
   char buf[BUFF_SIZE] = {0,};
   switch(curr)
     {
       case 0:
          strftime(buf, BUFF_SIZE, E_("IDS_ACCS_BODY_PD_M_YEAR"), set_date);
          return strdup(buf);
       case 1:
          strftime(buf, BUFF_SIZE, "%B", set_date);
          return strdup(buf);
       case 2:
          strftime(buf, BUFF_SIZE, E_("IDS_ACCS_BODY_PD_M_DAY"), set_date);
          return strdup(buf);
       default:
          break;
     }

   return NULL;
}

static char *
_access_date_info_cb(void *data, Evas_Object *obj __UNUSED__)
{
   int idx = 0, loc = 0;
   char *ret = NULL;
   Eina_Strbuf *buf = NULL;
   Popup_Module_Data *popup_mod;
   popup_mod = (Popup_Module_Data *)data;
   if (!popup_mod) return NULL;
   struct tm set_date;

   elm_datetime_value_get(popup_mod->mod_data.base, &set_date);
   buf = eina_strbuf_new();
   set_date.tm_isdst = -1;
   mktime(&set_date);
   for (idx = 0; idx <= ELM_DATETIME_DATE; idx++)
     {
        popup_mod->mod_data.field_location_get(popup_mod->mod_data.base, idx, &loc);
        if (loc > ELM_DATETIME_DATE) loc  = loc - PICKER_POPUP_FIELD_COUNT;
        popup_mod->field_location[loc] = idx;
     }
   if (popup_mod->weekday_show && popup_mod->weekday_loc_first)
     {
        strftime(popup_mod->weekday, BUFF_SIZE, "%A ", &set_date);
        eina_strbuf_append(buf, popup_mod->weekday);
     }
   for (idx = 0; idx <= ELM_DATETIME_DATE; idx++)
     {
        eina_strbuf_append(buf,_picker_location_text_get(popup_mod->field_location[idx], &set_date));
        if (idx < ELM_DATETIME_DATE) eina_strbuf_append(buf, " ");
     }
   if (popup_mod->weekday_show && !popup_mod->weekday_loc_first)
     {
        strftime(popup_mod->weekday, BUFF_SIZE, " %A", &set_date);
        eina_strbuf_append(buf, popup_mod->weekday);
     }
   ret = eina_strbuf_string_steal(buf);
   eina_strbuf_free(buf);
   return ret;
}

static char *
_access_time_info_cb(void *data, Evas_Object *obj __UNUSED__)
{
   char *ret = NULL;
   Eina_Strbuf *buf;
   Popup_Module_Data *popup_mod;
   struct tm set_time;
   char buffer[BUFF_SIZE] = {0,};
   int loc;
   Eina_Bool is_visible;

   popup_mod = (Popup_Module_Data *)data;
   if (!popup_mod) return NULL;

   elm_datetime_value_get(popup_mod->mod_data.base, &set_time);
   buf = eina_strbuf_new();
   if (popup_mod->time_12hr_fmt)
     {
        is_visible = popup_mod->mod_data.field_location_get(popup_mod->mod_data.base,
                                                            ELM_DATETIME_AMPM, &loc);
        if (is_visible && ((loc == 0) || (loc == 3)))
          {
             strftime(buffer, BUFF_SIZE, "%p ", &set_time);
             eina_strbuf_append_printf(buf, "%s", buffer);
          }
        strftime(buffer, BUFF_SIZE, E_("IDS_ACCS_BODY_PD_M_12HOUR"), &set_time);
        eina_strbuf_append_printf(buf, "%s", buffer);
        if (set_time.tm_min)
          {
             eina_strbuf_append_printf(buf, ":");
             strftime(buffer, BUFF_SIZE, E_("IDS_ACCS_BODY_PD_M_MINUTES"), &set_time);
             eina_strbuf_append_printf(buf, "%s", buffer);
          }
        if (is_visible && (loc != 0) && (loc != 3))
          {
             strftime(buffer, BUFF_SIZE, " %p", &set_time);
             eina_strbuf_append_printf(buf, "%s", buffer);
          }
     }
   else
     {
        strftime(buffer, BUFF_SIZE, E_("IDS_ACCS_BODY_PD_M_24HOUR"), &set_time);
        eina_strbuf_append_printf(buf, "%s", buffer);
        if (set_time.tm_min)
          {
             eina_strbuf_append_printf(buf, ":");
             strftime(buffer, BUFF_SIZE, E_("IDS_ACCS_BODY_PD_M_MINUTES"), &set_time);
             eina_strbuf_append_printf(buf, "%s", buffer);
          }
     }
   ret = eina_strbuf_string_steal(buf);
   eina_strbuf_free(buf);
   return ret;
}

EAPI Eina_List*
focus_object_list_get(Elm_Datetime_Module_Data *module_data, Evas_Object *obj __UNUSED__)
{
   Popup_Module_Data *popup_mod;
   Eina_List *items = NULL;

   popup_mod = (Popup_Module_Data *)module_data;
   if (!popup_mod) return NULL;

   items = eina_list_append(items, popup_mod->date_btn);
   items = eina_list_append(items, popup_mod->time_btn);

   return items;
}

EAPI void
access_register(Elm_Datetime_Module_Data *module_data)
{
   Popup_Module_Data *popup_mod;

   popup_mod = (Popup_Module_Data *)module_data;
   if (!popup_mod) return;

   elm_access_info_cb_set(popup_mod->date_btn, ELM_ACCESS_INFO,
                          _access_date_info_cb, popup_mod);
   elm_access_info_cb_set(popup_mod->time_btn, ELM_ACCESS_INFO,
                          _access_time_info_cb, popup_mod);
}

EAPI void
create_fields(Elm_Datetime_Module_Data *mdata)
{
   Popup_Module_Data *popup_mod;
   Evas_Object *obj;

   popup_mod = (Popup_Module_Data *)mdata;
   if (!popup_mod) return;

   obj = popup_mod->mod_data.base;
   popup_mod->date_btn = elm_button_add(obj);
   elm_object_style_set(popup_mod->date_btn, "style1/auto_expand");
   elm_object_part_content_set(obj, "date.btn", popup_mod->date_btn);
   evas_object_smart_callback_add(popup_mod->date_btn, "clicked",
                                  _launch_datepicker, popup_mod);

   popup_mod->time_btn = elm_button_add(obj);
   elm_object_style_set(popup_mod->time_btn, "style1/auto_expand");
   elm_object_part_content_set(obj, "time.btn", popup_mod->time_btn);
   evas_object_smart_callback_add(popup_mod->time_btn, "clicked",
                                  _launch_timepicker, popup_mod);

   elm_object_signal_callback_add(obj, "datepicker,show", "",
                                  _datepicker_show_cb, popup_mod);
   elm_object_signal_callback_add(obj, "timepicker,show", "",
                                  _timepicker_show_cb, popup_mod);
   elm_object_signal_callback_add(obj, "picker,hide", "",
                                  _picker_hide_cb, popup_mod);
   evas_object_smart_callback_add(obj, "language,changed",
                                  _module_language_changed_cb, popup_mod);
   elm_object_signal_callback_add(obj, "weekday,show", "",
                                  _weekday_show_cb, popup_mod);
   elm_object_signal_callback_add(obj, "weekday,hide", "",
                                  _weekday_hide_cb, popup_mod);
}

EAPI Elm_Datetime_Module_Data *
obj_hook(Evas_Object *obj __UNUSED__)
{
   Popup_Module_Data *popup_mod;

   popup_mod = ELM_NEW(Popup_Module_Data);
   if (!popup_mod) return NULL;

   popup_mod->date_btn = NULL;
   popup_mod->time_btn = NULL;
   popup_mod->popup = NULL;
   popup_mod->center_popup_win = NULL;
   popup_mod->datepicker_layout = NULL;
   popup_mod->timepicker_layout = NULL;
   popup_mod->weekday_show = EINA_FALSE;
   popup_mod->weekday_loc_first = EINA_TRUE;
   popup_mod->is_center_popup = EINA_FALSE;
   popup_mod->is_custom_popup = EINA_FALSE;
   popup_mod->dateBtn_is_visible = EINA_FALSE;
   popup_mod->timeBtn_is_visible = EINA_FALSE;

   return ((Elm_Datetime_Module_Data*)popup_mod);
}

EAPI void
obj_unhook(Elm_Datetime_Module_Data *module_data)
{
   Popup_Module_Data *popup_mod;

   popup_mod = (Popup_Module_Data *)module_data;
   if (!popup_mod) return;

   if (popup_mod->date_btn)
     evas_object_del(popup_mod->date_btn);
   if (popup_mod->time_btn)
     evas_object_del(popup_mod->time_btn);
   if (popup_mod->datepicker_layout)
     evas_object_del(popup_mod->datepicker_layout);
   if (popup_mod->timepicker_layout)
     evas_object_del(popup_mod->timepicker_layout);
   if (popup_mod->popup)
     evas_object_del(popup_mod->popup);

   if (popup_mod)
     {
        free(popup_mod);
        popup_mod = NULL;
     }
}

EAPI void
obj_format_hook(Elm_Datetime_Module_Data *module_data)
{
   Popup_Module_Data *popup_mod;

   popup_mod = (Popup_Module_Data *)module_data;
   if (!popup_mod) return;

   _module_format_change(popup_mod);
}

EAPI void
obj_theme_hook(Elm_Datetime_Module_Data *module_data)
{
   Popup_Module_Data *popup_mod;
   Evas_Object *obj;

   popup_mod = (Popup_Module_Data *)module_data;
   if (!popup_mod) return;

   obj = popup_mod->mod_data.base;
   display_fields((Elm_Datetime_Module_Data *)popup_mod);
   evas_object_smart_callback_add(obj, SIG_CUSTOM_PICKER_SET_BTN_CLICKED, _custom_popup_set_btn_clicked_cb, popup_mod);
   evas_object_smart_callback_add(obj, SIG_CENTER_POPUP_WIN_GET, _custom_popup_win_get_cb, popup_mod);
   evas_object_smart_callback_add(obj, SIG_DATETIME_PICKER_LAYOUT_CREATE, _custom_picker_layout_create_cb, popup_mod);
}

EAPI void
sizing_eval_hook(Elm_Datetime_Module_Data *module_data)
{
   Popup_Module_Data *popup_mod;
   Evas_Object *obj;
   Evas_Coord dw = 0, dh = 0;
   Evas_Coord tw = 0, th = 0;
   Evas_Coord center_paddingw = 0, center_paddingh = 0;
   int minw = 0;

   popup_mod = (Popup_Module_Data *)module_data;
   if (!popup_mod) return;

   obj = popup_mod->mod_data.base;
   edje_object_part_geometry_get(elm_layout_edje_get(obj), "center.padding", NULL, NULL, &center_paddingw, &center_paddingh);
   if (popup_mod->dateBtn_is_visible)
      evas_object_size_hint_min_get(popup_mod->date_btn, &dw, &dh);

   if (popup_mod->timeBtn_is_visible)
      evas_object_size_hint_min_get(popup_mod->time_btn, &tw, &th);

   minw = center_paddingw + dw + tw;
   evas_object_size_hint_min_set(obj, minw, center_paddingh);
   evas_object_size_hint_max_set(obj, -1, -1);

}

EAPI void
obj_focus_hook(Elm_Datetime_Module_Data *module_data __UNUSED__)
{
   // TODO: Default focus - enhance this func. for obj_show/obj_hide like below
#if 0
   Popup_Module_Data *popup_mod;

   popup_mod = (Popup_Module_Data *)module_data;
   if (!popup_mod || popup_mod->popup) return;

   if (elm_widget_focus_get(obj))
     evas_object_show(popup_mod->popup);
   else
     evas_object_hide(popup_mod->popup);
#endif
}

// module api funcs needed
EAPI int
elm_modapi_init(void *m __UNUSED__)
{
   return 1; // succeed always
}

EAPI int
elm_modapi_shutdown(void *m __UNUSED__)
{
   return 1; // succeed always
}
