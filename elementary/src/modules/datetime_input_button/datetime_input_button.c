#include <Elementary.h>
#include "elm_priv.h"
#include <unicode/udat.h>
#include <unicode/ustring.h>
#include <unicode/ucnv.h>


#define DATETIME_FIELD_COUNT            6
#define BUFF_SIZE                       1024
#define STRUCT_TM_YEAR_BASE_VALUE       1900
#define STRUCT_TM_TIME_12HRS_MAX_VALUE  12
#define STRUCT_TM_TIME_24HRS_MAX_VALUE  23
#define PICKER_FIELD_COUNT              3
#define ROTATE_TIME_NORMAL              0.2

typedef struct _Button_Module_Data Button_Module_Data;

struct _Button_Module_Data
{
   Elm_Datetime_Module_Data mod_data;
   Evas_Object *datetime_field[DATETIME_FIELD_COUNT];
   Evas_Object *datetime_entry_field[DATETIME_FIELD_COUNT];
   Evas_Object *top_button[DATETIME_FIELD_COUNT];
   Evas_Object *foot_button[DATETIME_FIELD_COUNT];
   Evas_Object *entry_layout, *parent_layout;
   Evas_Object *bg, *win, *parent_nf;
   Evas_Object *group;
   Ecore_Timer *longpress_timer;
   Ecore_Timer *spin_timer;
   int field_location[DATETIME_FIELD_COUNT];
   int last_value;
   int focused_field;
   struct tm set_time;
   Eina_Bool time_12hr_fmt;
   Eina_Bool weekday_show;
   Eina_Bool weekday_loc_first;
   Eina_Bool timepicker;
   Eina_Bool field_entry_created;
   Eina_Bool is_toggled;
   Eina_Bool is_am;
 };

EAPI void obj_format_hook(Elm_Datetime_Module_Data *module_data);
static void _field_value_display(Button_Module_Data *button_mod, Evas_Object *obj);
static void _field_access_register(Button_Module_Data *button_mod, Evas_Object *obj,
                                   Elm_Datetime_Field_Type field_type);
static int  _string_to_integer(const char *str);
static void _field_entry_create(void* data);

static void
_set_AM_PM_string(Button_Module_Data *button_mod, Eina_Bool is_am)
{
    if (is_am)
      {
         button_mod->is_am = EINA_TRUE;
         elm_object_domain_translatable_text_set(button_mod->datetime_field[ELM_DATETIME_AMPM], PACKAGE, "IDS_COM_BODY_AM");
         elm_object_domain_translatable_text_set(button_mod->datetime_entry_field[ELM_DATETIME_AMPM], PACKAGE, "IDS_COM_BODY_AM");
      }
    else
      {
         button_mod->is_am = EINA_FALSE;
         elm_object_domain_translatable_text_set(button_mod->datetime_field[ELM_DATETIME_AMPM], PACKAGE, "IDS_COM_BODY_PM");
         elm_object_domain_translatable_text_set(button_mod->datetime_entry_field[ELM_DATETIME_AMPM], PACKAGE, "IDS_COM_BODY_PM");
      }
}

static int
_string_to_integer(const char *str)
{
    char *locale = NULL;
    char *locale_tmp = NULL;
    char *p = NULL;
    int num;
    UNumberFormat *num_formatter = NULL;
    UErrorCode status = U_ZERO_ERROR;
    UChar ustr[256];
    UConverter *cnv;

    locale_tmp = setlocale(LC_ALL, NULL);
    if (!locale_tmp) return -1;
    locale = (char*)malloc(sizeof(char)*(strlen(locale_tmp)+1));
    strcpy(locale, locale_tmp);
    if (locale[0] != '\0')
      {
         p = strstr(locale, ".UTF-8");
         if (p) *p = 0;
      }
   cnv = ucnv_open( NULL,&status);
   ucnv_toUChars(cnv, ustr, sizeof(ustr), str, strlen(str)+1, &status);
   num_formatter = unum_open(UNUM_DEFAULT, NULL, -1, locale, NULL, &status);
   num = unum_parse(num_formatter,  ustr, sizeof(ustr), 0, &status);
   ucnv_close(cnv);
   unum_close(num_formatter);

   free(locale);
   return num;
}

static void
_access_increment_decrement_info_say(Evas_Object *btn, Evas_Object *field_obj,
                                     Eina_Bool is_incremented)
{
   char *text;
   Eina_Strbuf *buf;

    buf = eina_strbuf_new();
    if (is_incremented)
      {
         elm_object_signal_emit(btn, "elm,action,anim,activate", "elm");
         eina_strbuf_append(buf, E_("incremented"));
      }
    else
      {
         elm_object_signal_emit(btn, "elm,action,anim,activate", "elm");
         eina_strbuf_append(buf, E_("decremented"));
      }

   eina_strbuf_append_printf
      (buf, "%s", elm_object_text_get(field_obj));

   text = eina_strbuf_string_steal(buf);
   eina_strbuf_free(buf);
   _elm_access_say(field_obj, text, EINA_FALSE);
}

static void
_am_pm_value_set(Button_Module_Data *button_mod)
{
   struct tm min_time, max_time;

   Evas_Object *datetime = button_mod->mod_data.base;

   elm_datetime_value_get(datetime, &(button_mod->set_time));
   button_mod->mod_data.fields_min_max_get(datetime, &(button_mod->set_time), &min_time, &max_time);

   if (button_mod->set_time.tm_hour >= min_time.tm_hour &&
        button_mod->set_time.tm_hour <= max_time.tm_hour)
     {
        if (button_mod->is_am && button_mod->set_time.tm_hour == STRUCT_TM_TIME_12HRS_MAX_VALUE)
          button_mod->set_time.tm_hour = STRUCT_TM_TIME_12HRS_MAX_VALUE;
        else if (button_mod->is_am)
          button_mod->set_time.tm_hour += STRUCT_TM_TIME_12HRS_MAX_VALUE;
        else if (button_mod->set_time.tm_hour == STRUCT_TM_TIME_12HRS_MAX_VALUE)
          button_mod->set_time.tm_hour = 0;
        else
          button_mod->set_time.tm_hour -= STRUCT_TM_TIME_12HRS_MAX_VALUE;
        button_mod->is_toggled = EINA_FALSE;
        elm_datetime_value_set(datetime, &(button_mod->set_time));
     }
   if (button_mod->set_time.tm_hour < STRUCT_TM_TIME_12HRS_MAX_VALUE)
     _set_AM_PM_string(button_mod, EINA_TRUE);
   else
     _set_AM_PM_string(button_mod, EINA_FALSE);
}

static void
_field_state_set(Button_Module_Data *button_mod, Evas_Object *obj, const char *type)
{
   Elm_Datetime_Field_Type field_type;
   Evas_Object *radio;
   int value;
   if (!button_mod) return;

   field_type = (Elm_Datetime_Field_Type)evas_object_data_get(obj, type);
   radio = button_mod->datetime_field[field_type];
   value = elm_radio_state_value_get(radio);
   elm_radio_value_set(button_mod->group, value);
}

static void
_adjust_layout(Button_Module_Data *button_mod, Evas_Object *obj)
{
   Evas_Object *datetime;
   char buf[BUFF_SIZE];
   int idx, loc;
   Eina_Bool field_exist;

   if (!button_mod) return;
   datetime = button_mod->mod_data.base;

   for (idx = 0; idx <= ELM_DATETIME_AMPM; idx++)
     {
        field_exist = button_mod->mod_data.field_location_get(datetime, idx, &loc);
        switch (idx)
          {
           case ELM_DATETIME_YEAR:
              snprintf(buf, sizeof(buf), "elm,state,field%d,year", loc);
              elm_layout_signal_emit(obj, buf, "");
              break;
           case ELM_DATETIME_MONTH:
              snprintf(buf, sizeof(buf), "elm,state,field%d,month", loc);
              elm_layout_signal_emit(obj, buf, "");
              break;
           case ELM_DATETIME_DATE:
              snprintf(buf, sizeof(buf), "elm,state,field%d,date", loc);
              elm_layout_signal_emit(obj, buf, "");
              break;
           case ELM_DATETIME_HOUR:
              snprintf(buf, sizeof(buf), "elm,state,field%d,hour", loc);
              elm_layout_signal_emit(obj, buf, "");
              break;
           case ELM_DATETIME_MINUTE:
              snprintf(buf, sizeof(buf), "elm,state,field%d,minute", loc);
              elm_layout_signal_emit(obj, buf, "");
              break;
           case ELM_DATETIME_AMPM:
              snprintf(buf, sizeof(buf), "elm,state,field%d,ampm", loc);
              elm_layout_signal_emit(obj, buf, "");
              break;
          }
     }

   field_exist = button_mod->mod_data.field_location_get(datetime, ELM_DATETIME_AMPM, &loc);

   if (!field_exist)
     {
        snprintf(buf, sizeof(buf), "elm,state,field%d,ampm,hide", loc);
        elm_layout_signal_emit(obj, buf, "");
     }
   edje_object_message_signal_process(elm_layout_edje_get(obj));
}

static void
_radio_clicked_cb(void *data, Evas_Object *obj,
                  const char *emission EINA_UNUSED,
                  const char *source EINA_UNUSED)
{
   Elm_Datetime_Field_Type field_type;
   Button_Module_Data *button_mod;
   int idx;
   button_mod = (Button_Module_Data *)data;
   if (!button_mod) return;

   field_type = (Elm_Datetime_Field_Type)evas_object_data_get(obj, "_field_type");
   if (field_type == ELM_DATETIME_AMPM)
     {
        _am_pm_value_set(button_mod);
        return;
     }

   if (button_mod->field_entry_created == EINA_FALSE)
     _field_entry_create(button_mod);

   for (idx = 0; idx < DATETIME_FIELD_COUNT; idx++)
      elm_object_text_set(button_mod->datetime_entry_field[idx],elm_object_text_get(button_mod->datetime_field[idx]));
   evas_object_show(button_mod->win);
   evas_object_show(button_mod->bg);
   evas_object_show(button_mod->parent_layout);
   evas_object_show(button_mod->entry_layout);
   elm_object_focus_set(button_mod->datetime_entry_field[field_type], EINA_TRUE);
   elm_entry_select_all(button_mod->datetime_entry_field[field_type]);
}

static void
_entry_focused_cb(void *data, Evas_Object *obj, void *event_info EINA_UNUSED)
{
   Button_Module_Data *button_mod;
   Elm_Datetime_Field_Type field_type;

   button_mod = (Button_Module_Data *)data;
   if (!button_mod) return;
   field_type = (Elm_Datetime_Field_Type)evas_object_data_get(obj, "_entry_field_type");
   button_mod->focused_field = field_type;
   button_mod->last_value = -1;
}

static void
_entry_double_clicked_cb(void *data, Evas_Object *obj, void *event_info EINA_UNUSED)
{
   Button_Module_Data *button_mod;
   button_mod = (Button_Module_Data *)data;
   if (!button_mod) return;
   button_mod->last_value = _string_to_integer(elm_object_text_get(obj));
}

static void
_entry_layout_hide(Button_Module_Data *button_mod)
{
   if (_elm_config->access_mode)
     {
        if (button_mod->parent_nf)
          elm_object_tree_focus_allow_set(button_mod->parent_nf, EINA_TRUE);
        _elm_access_object_hilight_disable(evas_object_evas_get(button_mod->entry_layout));
        elm_access_highlight_set(button_mod->datetime_field[button_mod->focused_field]);
     }
   evas_object_hide(button_mod->win);
}

static void
_entry_clicked_cb(void *data, Evas_Object *obj, void *event_info EINA_UNUSED)
{
   Button_Module_Data *button_mod;
   Elm_Datetime_Field_Type field_type;
   int idx;
   button_mod = (Button_Module_Data *)data;
   if (!button_mod) return;

   field_type = (Elm_Datetime_Field_Type)evas_object_data_get(obj, "_entry_field_type");
   if (field_type != ELM_DATETIME_AMPM)
     //button_mod->last_value = _string_to_integer(elm_object_text_get(obj));
     elm_entry_select_all(obj);
   else
     {
        button_mod->focused_field = field_type;
       //Commit the entry value first
        for (idx = 0; idx < DATETIME_FIELD_COUNT; idx++)
           elm_object_focus_set(button_mod->datetime_entry_field[idx], EINA_FALSE);
        _am_pm_value_set(button_mod);
     }
}

static Evas_Object *
_entry_layout_create(Button_Module_Data *button_mod)
{
   Evas_Object *win, *bg;
   Evas_Object *layout, *parent, *parent2;
   Evas_Object *child = NULL, *nf;
   Evas_Coord w = 0, h = 0;

   nf = elm_widget_parent_get(button_mod->mod_data.base);
   parent2 = nf;
   while (nf)
     {
        if (!strcmp(elm_widget_type_get(nf), "elm_naviframe"))
          {
             child = nf;
             break;
          }
        parent2 = nf;
        nf = elm_widget_parent_get(parent2);
     }
   if (child)
     button_mod->parent_nf = child;
   ecore_x_window_size_get(ecore_x_window_root_first_get(), &w, &h);
   win = elm_win_add(NULL, NULL, ELM_WIN_UTILITY);
   evas_object_resize(win, w, h);
   if (!win) return NULL;

   elm_win_autodel_set(win, EINA_TRUE);

   if (elm_win_wm_rotation_supported_get(win))
     {
        int rots[4] = { 0, 90, 180, 270 };
        elm_win_wm_rotation_available_rotations_set(win, (const int*)(&rots), 4);
     }

   bg = elm_bg_add(win);
   elm_object_style_set(bg, "datetime");
   evas_object_size_hint_weight_set(bg, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_win_resize_object_add(win, bg);

   parent = elm_layout_add(bg);
   elm_layout_theme_set(parent, "layout", "application", "default");
   evas_object_size_hint_weight_set(parent, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);

   layout = elm_layout_add(parent);
   elm_layout_theme_set(layout, "datetime", "datepicker_layout", "entry");
   evas_object_size_hint_weight_set(layout, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);

   elm_object_part_content_set(parent, "elm.swallow.content", layout);
   elm_object_part_content_set(bg, "elm.swallow.content", parent);

   button_mod->parent_layout = parent;
   button_mod->bg = bg;
   button_mod->win = win;
   return layout;
}

static void
_datetime_entry_value_set(Button_Module_Data *button_mod, Elm_Datetime_Field_Type field_type, int value)
{
   Evas_Object *datetime = button_mod->mod_data.base;

   elm_datetime_value_get(datetime, &(button_mod->set_time));

   switch (field_type)
    {
      case ELM_DATETIME_YEAR:
         button_mod->set_time.tm_year = value;
         break;
      case ELM_DATETIME_MONTH:
         button_mod->set_time.tm_mon = value;
         break;
      case ELM_DATETIME_DATE:
         button_mod->set_time.tm_mday = value;
         break;
      case ELM_DATETIME_HOUR:
         if (button_mod->time_12hr_fmt)
           {
              if (button_mod->is_am && value == STRUCT_TM_TIME_12HRS_MAX_VALUE)
                button_mod->set_time.tm_hour = 0;
              else
                button_mod->set_time.tm_hour = value;
           }
         else button_mod->set_time.tm_hour = value;
         break;
      case ELM_DATETIME_MINUTE:
         button_mod->set_time.tm_min = value;
         break;
      case ELM_DATETIME_AMPM:
      default:
         break;
    }
   button_mod->is_toggled = EINA_FALSE;
   elm_datetime_value_set(datetime, &(button_mod->set_time));
}

static int
_picker_nextfield_location_get(Button_Module_Data *button_mod, int curr)
{
   int idx, next_idx;

   for (idx = 0; idx <= ELM_DATETIME_DATE; idx++)
     if (button_mod->field_location[curr] == idx) break;

   if (idx >= ELM_DATETIME_DATE) return ELM_DATETIME_LAST;

   for (next_idx = 0; next_idx <= ELM_DATETIME_DATE; next_idx++)
     if (button_mod->field_location[next_idx] == idx+1) return next_idx;

   return ELM_DATETIME_LAST;
}

char *
_text_insert(const char *text, const char *input, int pos)
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
   Button_Module_Data *button_mod;
   char *new_str = NULL;
   int min = 0, max = 0, val = 0, len = 0, next_idx = 0;
   char *insert;
   const char *curr_str;
   const int min_text_len = 3;

   if (!(*text) || !strcmp(*text, "")) return;
   button_mod = (Button_Module_Data *)data;
   if (!button_mod) return;

   insert = *text;
   curr_str = elm_object_text_get(obj);
   if (curr_str)
     len = strlen(curr_str);
   if (len < min_text_len) return;

   if (curr_str) new_str = _text_insert(curr_str, insert, elm_entry_cursor_pos_get(obj));
   if (new_str) val = _string_to_integer(new_str);

   button_mod->mod_data.field_limit_get(button_mod->mod_data.base, ELM_DATETIME_YEAR, &min, &max);

   min += STRUCT_TM_YEAR_BASE_VALUE;
   max += STRUCT_TM_YEAR_BASE_VALUE;
   if (val == 0)
     {
        val = min;
        sprintf(new_str, "%d", val);
     }
   if (val >= min && val <= max)
     {
        elm_object_text_set(obj, new_str);
        elm_entry_cursor_end_set(obj);
        next_idx = _picker_nextfield_location_get(button_mod, ELM_DATETIME_YEAR);
        if (next_idx != ELM_DATETIME_LAST)
          elm_object_focus_set(button_mod->datetime_entry_field[next_idx], EINA_TRUE);
        _datetime_entry_value_set(button_mod, ELM_DATETIME_YEAR, (val - STRUCT_TM_YEAR_BASE_VALUE));
        if (next_idx != ELM_DATETIME_LAST)
          elm_entry_select_all(button_mod->datetime_entry_field[next_idx]);
        elm_entry_cursor_end_set(obj);
     }

   *insert = 0;
   free((void *)new_str);
   new_str = NULL;
}

static void
_month_validity_checking_filter(void *data, Evas_Object *obj, char **text)
{
   Button_Module_Data *button_mod;
   char *new_str = NULL;
   int min = 0, max = 0;
   int val = 0;
   char *insert;
   const char *curr_str;
   int next_idx = 0;
   int len = 0;
   const int min_text_len = 1;
   const char *fmt;

  if (!(*text) || !strcmp(*text, "")) return;

   button_mod = (Button_Module_Data *)data;
   if (!button_mod) return;

   fmt = button_mod->mod_data.field_format_get(button_mod->mod_data.base, ELM_DATETIME_MONTH);
   if (!strcmp(fmt, "%b"))
     elm_entry_entry_set(obj, ""); //clear entry first in case of abbreviated month name
   insert = *text;
   curr_str = elm_object_text_get(obj);
   if (curr_str) new_str = _text_insert(curr_str, insert, elm_entry_cursor_pos_get(obj));
   if (new_str) val = _string_to_integer(new_str) - 1;

   if (strcmp(fmt, "%b"))
     {
         if (curr_str && (!strcmp(*text, "1") || !strcmp(*text, "0")))
           len = strlen(curr_str);
         else
           len = strlen(*text);
         if (len < min_text_len) return;

         if (new_str)
           {
               val = _string_to_integer(new_str);
               if (val == 0)
                 val = STRUCT_TM_TIME_12HRS_MAX_VALUE - 1;
               else val -= 1;
            }
     }

   button_mod->mod_data.field_limit_get(button_mod->mod_data.base, ELM_DATETIME_MONTH, &min, &max);

   if ((val >= min) && (val <= max))
     {
        elm_entry_cursor_end_set(obj);
        next_idx = _picker_nextfield_location_get(button_mod, ELM_DATETIME_MONTH);
        if (next_idx != ELM_DATETIME_LAST)
          {
             elm_object_focus_set(button_mod->datetime_entry_field[next_idx], EINA_TRUE);
             _datetime_entry_value_set(button_mod, ELM_DATETIME_MONTH, val);
             elm_entry_select_all(button_mod->datetime_entry_field[next_idx]);
          }
     }
   *insert = 0;
   if (new_str) free((void *)new_str);
   new_str = NULL;
}

static void
_date_validity_checking_filter(void *data, Evas_Object *obj, char **text)
{
   Button_Module_Data *button_mod;
   char *new_str = NULL;
   int val = 0, len = 0;
   const char *curr_str;
   char *insert;
   int min = 0, max = 0;
   int next_idx = 0;
   const int min_text_len = 1;

   if (!(*text) || !strcmp(*text, "")) return;
   button_mod = (Button_Module_Data *)data;
   if (!button_mod) return;

   insert = *text;
   curr_str = elm_object_text_get(obj);
   if (curr_str)
     len = strlen(curr_str);
   if (len < min_text_len) return;

   if (curr_str) new_str = _text_insert(curr_str, insert, elm_entry_cursor_pos_get(obj));
   if (new_str) val = _string_to_integer(new_str);

   button_mod->mod_data.field_limit_get(button_mod->mod_data.base, ELM_DATETIME_DATE, &min, &max);

   if (val == 0)
     {
        val = max;
        sprintf(new_str, "%d", val);
     }

   if ((val >= min) && (val <= max))
     {
        elm_object_text_set(obj, new_str);

        next_idx = _picker_nextfield_location_get(button_mod, ELM_DATETIME_DATE);
        if (next_idx != ELM_DATETIME_LAST)
          elm_object_focus_set(button_mod->datetime_entry_field[next_idx], EINA_TRUE);
        _datetime_entry_value_set(button_mod, ELM_DATETIME_DATE, val);
        elm_entry_cursor_end_set(obj);
        if (next_idx != ELM_DATETIME_LAST)
          elm_entry_select_all(button_mod->datetime_entry_field[next_idx]);
     }

   *insert = 0;
   free((void *)new_str);
   new_str = NULL;
}

static void
_hour_validity_checking_filter(void *data, Evas_Object *obj, char **text)
{
   Button_Module_Data *button_mod;
   char *new_str = NULL;
   int val = 0, len = 0, min = 0, max = 0;
   char *insert;
   const char *curr_str;
   const int min_text_len = 1, max_text_len = 2;
   char *locale_tmp;
   char locale[32] = {0,};
   char *p = NULL;

   if (!(*text) || !strcmp(*text, "")) return;

   locale_tmp = setlocale(LC_ALL, NULL);
   strcpy(locale, locale_tmp);

   if (locale[0] != '\0')
     {
        p = strstr(locale, ".UTF-8");
        if (p) *p = 0;
     }

   button_mod = (Button_Module_Data *)data;
   if (!button_mod) return;

   insert = *text;
   curr_str = elm_object_text_get(obj);
   if (curr_str)
     len = strlen(curr_str);
   if (len < min_text_len) return;

   if (curr_str) new_str = _text_insert(curr_str, insert, elm_entry_cursor_pos_get(obj));
   if (new_str) val = _string_to_integer(new_str);

   elm_datetime_value_get(button_mod->mod_data.base, &(button_mod->set_time));
   button_mod->mod_data.field_limit_get(button_mod->mod_data.base, ELM_DATETIME_HOUR, &min, &max);

   if (button_mod->time_12hr_fmt && !strcmp(locale,"ja_JP"))
     {
        if (val > STRUCT_TM_TIME_12HRS_MAX_VALUE)
           goto error;
        else if (val == STRUCT_TM_TIME_12HRS_MAX_VALUE)
          {
             sprintf(new_str, "%d", 0);
             val = 0;
          }
        if ((button_mod->set_time.tm_hour >= STRUCT_TM_TIME_12HRS_MAX_VALUE) && (val != STRUCT_TM_TIME_12HRS_MAX_VALUE))
          val += STRUCT_TM_TIME_12HRS_MAX_VALUE;
     }

   else if (button_mod->time_12hr_fmt && strcmp(locale,"ja_JP"))
     {
        if (val > STRUCT_TM_TIME_12HRS_MAX_VALUE)
          goto error;
        else if (val == 0)
          {
             sprintf(new_str, "%d", STRUCT_TM_TIME_12HRS_MAX_VALUE);
             val = STRUCT_TM_TIME_12HRS_MAX_VALUE;
          }
        if ((button_mod->set_time.tm_hour >= STRUCT_TM_TIME_12HRS_MAX_VALUE) && (val != STRUCT_TM_TIME_12HRS_MAX_VALUE))
          val += STRUCT_TM_TIME_12HRS_MAX_VALUE;
     }

   if ((val >= min) && (val <= max))
     {
       elm_object_text_set(obj, new_str);
       elm_entry_cursor_end_set(obj);
       //if (len == max_text_len)
         elm_object_focus_set(button_mod->datetime_entry_field[ELM_DATETIME_MINUTE], EINA_TRUE);
       _datetime_entry_value_set(button_mod, ELM_DATETIME_HOUR, val);
       elm_entry_select_all(button_mod->datetime_entry_field[ELM_DATETIME_MINUTE]);
     }

error:
    *insert = 0;
    free((void *)new_str);
    new_str = NULL;
}

static void
_minute_validity_checking_filter(void *data, Evas_Object *obj, char **text)
{
   Button_Module_Data *button_mod;
   char *new_str = NULL;
   int val = 0, len = 0, min = 0, max = 0;
   char *insert;
   const char *curr_str;
   const int min_text_len = 1;

   if (!(*text) || !strcmp(*text, "")) return;
   button_mod = (Button_Module_Data *)data;
   if (!button_mod) return;

   insert = *text;
   curr_str = elm_object_text_get(obj);
   if (curr_str)
     len = strlen(curr_str);
   if (len < min_text_len) return;

   if (curr_str) new_str = _text_insert(curr_str, insert, elm_entry_cursor_pos_get(obj));
   if (new_str) val = _string_to_integer(new_str);

   button_mod->mod_data.field_limit_get(button_mod->mod_data.base, ELM_DATETIME_MINUTE, &min, &max);

   if ((val >= min) && (val <= max))
     {
        elm_object_text_set(obj, new_str);
        _datetime_entry_value_set(button_mod, ELM_DATETIME_MINUTE, val);
        elm_entry_cursor_end_set(obj);
     }
   *insert = 0;
   free((void *)new_str);
   new_str = NULL;
}

static void
_set_datepicker_entry_filter(Button_Module_Data *button_mod)
{
   Evas_Object *entry;
   //Elm_Entry_Filter_Accept_Set digits_filter_data;
   Elm_Entry_Filter_Limit_Size limit_filter_data;
   int idx;
   const char *fmt;
   //digits_filter_data.accepted = "0123456789";
   //digits_filter_data.rejected = NULL;

   limit_filter_data.max_byte_count = 0;
   fmt = button_mod->mod_data.field_format_get(button_mod->mod_data.base, ELM_DATETIME_MONTH);
   for (idx = 0; idx <= ELM_DATETIME_DATE; idx++)
     {
         entry = button_mod->datetime_entry_field[idx];
         elm_entry_context_menu_disabled_set(entry, EINA_TRUE);

         if (idx == ELM_DATETIME_YEAR)
           limit_filter_data.max_char_count = 4;
         else if (idx == ELM_DATETIME_MONTH && !strcmp(fmt, "%b"))
         /* For ASCII character Month's string (3) + (2)(max digits) = 5
          * For UNICODE character, libicu will display upto length 3 month string which contains
          * 3 main character and 3 supplement character = 6 ,so entry has 6 character and 6 to be entered = 12
          * */
           limit_filter_data.max_char_count = 12;
         else if (idx == ELM_DATETIME_MONTH)
           limit_filter_data.max_char_count = 2;
         else
           limit_filter_data.max_char_count = 2;
         elm_entry_markup_filter_append(entry, elm_entry_filter_limit_size, &limit_filter_data);

         if (idx == ELM_DATETIME_YEAR)
           elm_entry_markup_filter_append(entry, _year_validity_checking_filter, button_mod);
         else if (idx == ELM_DATETIME_MONTH)
           elm_entry_markup_filter_append(entry, _month_validity_checking_filter, button_mod);
         else if (idx == ELM_DATETIME_DATE)
           elm_entry_markup_filter_append(entry, _date_validity_checking_filter, button_mod);
     }
}

static void
_set_timepicker_entry_filter(Button_Module_Data *button_mod)
{
   Evas_Object *entry;
   Elm_Entry_Filter_Accept_Set digits_filter_data;
   Elm_Entry_Filter_Limit_Size limit_filter_data;
   int idx;

   digits_filter_data.accepted = "0123456789";
   digits_filter_data.rejected = NULL;

   limit_filter_data.max_byte_count = 0;

   for (idx = ELM_DATETIME_HOUR; idx < ELM_DATETIME_AMPM; idx++)
      {
         entry = button_mod->datetime_entry_field[idx];
         elm_entry_context_menu_disabled_set(entry, EINA_TRUE);

         limit_filter_data.max_char_count = 2;
         elm_entry_markup_filter_append(entry, elm_entry_filter_limit_size, &limit_filter_data);
         elm_entry_markup_filter_append(entry, elm_entry_filter_accept_set, &digits_filter_data);

         if (idx == ELM_DATETIME_HOUR)
           elm_entry_markup_filter_append(entry, _hour_validity_checking_filter, button_mod);
         else if (idx == ELM_DATETIME_MINUTE)
           elm_entry_markup_filter_append(entry, _minute_validity_checking_filter, button_mod);
      }
}

static void
_input_panel_event_callback(void *data, Ecore_IMF_Context *imf_context EINA_UNUSED, int value)
{
   Button_Module_Data *button_mod;

   button_mod = (Button_Module_Data *)data;
   if (!button_mod) return;

   switch (value)
     {
       case ECORE_IMF_INPUT_PANEL_STATE_SHOW:
          break;
       case ECORE_IMF_INPUT_PANEL_STATE_HIDE:
          _field_state_set(button_mod, button_mod->datetime_field[button_mod->focused_field], "_field_type");
          _entry_layout_hide(button_mod);
          break;
     }
}

static void
_input_panel_context_set(Button_Module_Data *button_mod, Elm_Datetime_Field_Type field_type)
{
   Ecore_IMF_Context *imf_context = elm_entry_imf_context_get(button_mod->datetime_entry_field[field_type]);
   ecore_imf_context_input_panel_event_callback_add(imf_context, ECORE_IMF_INPUT_PANEL_STATE_EVENT,
                                                     _input_panel_event_callback, button_mod);
}

static void
_entry_value_commit(Button_Module_Data *button_mod, Elm_Datetime_Field_Type field_type)
{
   int curr_value, val, min, max;

   if(field_type == ELM_DATETIME_YEAR)
     {
        if (_string_to_integer(elm_object_text_get(button_mod->datetime_entry_field[field_type])) == 0)
          {
             button_mod->mod_data.field_limit_get(button_mod->mod_data.base, field_type, &min, &max);
             _datetime_entry_value_set(button_mod, field_type, min);
          }
        else if (_string_to_integer(elm_object_text_get(button_mod->datetime_entry_field[field_type])) < STRUCT_TM_YEAR_BASE_VALUE)
          _datetime_entry_value_set(button_mod, field_type,
            _string_to_integer(elm_object_text_get(button_mod->datetime_field[field_type])) - STRUCT_TM_YEAR_BASE_VALUE);
        else
          _datetime_entry_value_set(button_mod, field_type,
           _string_to_integer(elm_object_text_get(button_mod->datetime_entry_field[field_type])) - STRUCT_TM_YEAR_BASE_VALUE);
     }
   else if (field_type == ELM_DATETIME_HOUR)
     {
        elm_datetime_value_get(button_mod->mod_data.base, &(button_mod->set_time));
        if (button_mod->time_12hr_fmt)
          {
             if (button_mod->set_time.tm_hour >= STRUCT_TM_TIME_12HRS_MAX_VALUE)
               {
                  if (button_mod->last_value == -1)
                    {
                       if (elm_entry_is_empty(button_mod->datetime_entry_field[field_type]))
                         curr_value = _string_to_integer(elm_object_text_get(button_mod->datetime_field[field_type])) + STRUCT_TM_TIME_12HRS_MAX_VALUE;
                      else
                         curr_value = _string_to_integer(elm_object_text_get(button_mod->datetime_entry_field[field_type])) + STRUCT_TM_TIME_12HRS_MAX_VALUE;
                    }
                  else
                    curr_value = button_mod->last_value + STRUCT_TM_TIME_12HRS_MAX_VALUE;

                  if (curr_value == 2 * STRUCT_TM_TIME_12HRS_MAX_VALUE)
                    _datetime_entry_value_set(button_mod, field_type, STRUCT_TM_TIME_12HRS_MAX_VALUE);
                  else
                   _datetime_entry_value_set(button_mod, field_type, curr_value);
               }
             else
               {
                  if (button_mod->last_value == -1)
                    {
                       if (elm_entry_is_empty(button_mod->datetime_entry_field[field_type]))
                         curr_value = _string_to_integer(elm_object_text_get(button_mod->datetime_field[field_type]));
                       else
                         curr_value = _string_to_integer(elm_object_text_get(button_mod->datetime_entry_field[field_type]));
                    }
                 else
                   curr_value = button_mod->last_value;
                 if (curr_value == STRUCT_TM_TIME_12HRS_MAX_VALUE)
                   curr_value = 0;
                 _datetime_entry_value_set(button_mod, field_type, curr_value);
               }
          }
        else
          _datetime_entry_value_set(button_mod, field_type, _string_to_integer(elm_object_text_get(button_mod->datetime_entry_field[field_type])));
     }
   else if (field_type == ELM_DATETIME_MONTH)
     {
        val = _string_to_integer(elm_object_text_get(button_mod->datetime_entry_field[field_type]));
        if (!val)
          val = STRUCT_TM_TIME_12HRS_MAX_VALUE;
        _datetime_entry_value_set(button_mod, field_type, val - 1);
     }
   else if (field_type == ELM_DATETIME_DATE)
     {
        val = _string_to_integer(elm_object_text_get(button_mod->datetime_entry_field[field_type]));
        button_mod->mod_data.field_limit_get(button_mod->mod_data.base, field_type, &min, &max);
        if (!val)
          val = max;
        _datetime_entry_value_set(button_mod, field_type, val);
     }
   else
      _datetime_entry_value_set(button_mod, field_type, _string_to_integer(elm_object_text_get(button_mod->datetime_entry_field[field_type])));
}

static void
_activated_value_set(Button_Module_Data *button_mod, Elm_Datetime_Field_Type field_type)
{
   int curr_value = 0;

   if (!strcmp(elm_object_text_get(button_mod->datetime_entry_field[field_type]), "<br/>"))
     {
        if (field_type == ELM_DATETIME_YEAR)
          {
             if (button_mod->last_value == -1)
               _datetime_entry_value_set(button_mod, field_type,
                 _string_to_integer(elm_object_text_get(button_mod->datetime_field[field_type])) - STRUCT_TM_YEAR_BASE_VALUE);
             else if (button_mod->last_value > STRUCT_TM_YEAR_BASE_VALUE)
               _datetime_entry_value_set(button_mod, field_type, button_mod->last_value - STRUCT_TM_YEAR_BASE_VALUE);
             else
               _datetime_entry_value_set(button_mod, field_type,
                              _string_to_integer(elm_object_text_get(button_mod->datetime_field[field_type])) - STRUCT_TM_YEAR_BASE_VALUE);
          }
        else if (field_type == ELM_DATETIME_HOUR)
          {
             if (button_mod->last_value == -1)
               curr_value = _string_to_integer(elm_object_text_get(button_mod->datetime_field[field_type]));
             else
               curr_value = button_mod->last_value;
             elm_datetime_value_get(button_mod->mod_data.base, &(button_mod->set_time));
             if ((button_mod->time_12hr_fmt) && (button_mod->set_time.tm_hour >= STRUCT_TM_TIME_12HRS_MAX_VALUE)
                   && (curr_value != STRUCT_TM_TIME_12HRS_MAX_VALUE))
               curr_value += STRUCT_TM_TIME_12HRS_MAX_VALUE;
             else if ((button_mod->time_12hr_fmt) && (button_mod->set_time.tm_hour >= STRUCT_TM_TIME_12HRS_MAX_VALUE)
                     && (curr_value == STRUCT_TM_TIME_12HRS_MAX_VALUE))
               curr_value = 0;
             _datetime_entry_value_set(button_mod, field_type, curr_value);
          }
        else
          {
             if (button_mod->last_value == -1)
               _datetime_entry_value_set(button_mod, field_type,
                 _string_to_integer(elm_object_text_get(button_mod->datetime_field[field_type])));
             else
              _datetime_entry_value_set(button_mod, field_type, button_mod->last_value);
          }
     }

}

static void
_entry_activated_cb(void *data, Evas_Object *obj, void *event_info EINA_UNUSED)
{
   Button_Module_Data *button_mod;
   Elm_Datetime_Field_Type field_type;
   int next_idx;
   button_mod = (Button_Module_Data *)data;
   if (!button_mod) return;

   field_type = (Elm_Datetime_Field_Type)evas_object_data_get(obj, "_entry_field_type");
   if (button_mod->timepicker)
     {
        if (field_type == ELM_DATETIME_HOUR)
          {
             _activated_value_set(button_mod, ELM_DATETIME_HOUR);
             elm_object_focus_set(button_mod->datetime_entry_field[ELM_DATETIME_MINUTE], EINA_TRUE);
             elm_entry_select_all(button_mod->datetime_entry_field[ELM_DATETIME_MINUTE]);
          }
        else
          {
             _activated_value_set(button_mod, ELM_DATETIME_MINUTE);
             elm_object_focus_set(button_mod->datetime_entry_field[ELM_DATETIME_MINUTE], EINA_FALSE);
          }
     }
   else
     {
        if (field_type == ELM_DATETIME_YEAR)
          _activated_value_set(button_mod, ELM_DATETIME_YEAR);
        else
          _activated_value_set(button_mod, field_type);

        next_idx = _picker_nextfield_location_get(button_mod, field_type);

        if (next_idx != ELM_DATETIME_LAST)
          {
             elm_object_focus_set(button_mod->datetime_entry_field[next_idx], EINA_TRUE);
             elm_entry_select_all(button_mod->datetime_entry_field[next_idx]);
          }
        else
          elm_object_focus_set(button_mod->datetime_entry_field[field_type], EINA_FALSE);
     }
}

static void
_entry_unfocused_cb(void *data, Evas_Object *obj, void *event_info EINA_UNUSED)
{
   Button_Module_Data *button_mod;
   Elm_Datetime_Field_Type field_type;
   const char *fmt;

   button_mod = (Button_Module_Data *)data;
   if (!button_mod) return;

   field_type = (Elm_Datetime_Field_Type)evas_object_data_get(obj, "_entry_field_type");
   fmt = button_mod->mod_data.field_format_get(button_mod->mod_data.base, ELM_DATETIME_MONTH);
   if (!strcmp(fmt, "%b") && field_type == ELM_DATETIME_MONTH) return;
   _entry_value_commit(button_mod, field_type);
}

static void
_field_list_arrange(Button_Module_Data *button_mod)
{
   Evas_Object *obj;
   char buf[BUFF_SIZE];
   int idx, loc;
   Elm_Datetime_Field_Type field_type;
   if (!button_mod) return;
   obj = button_mod->mod_data.base;

   for (idx = 0; idx < DATETIME_FIELD_COUNT; idx++)
     {
        field_type = (Elm_Datetime_Field_Type)evas_object_data_get(button_mod->datetime_field[idx], "_field_type");
        button_mod->mod_data.field_location_get(obj, field_type, &loc);
        snprintf(buf, sizeof(buf), "field%d", loc);
        evas_object_hide(elm_layout_content_unset(obj, buf));
        snprintf(buf, sizeof(buf), "up_btn_field%d.padding", loc);
        evas_object_hide(elm_layout_content_unset(obj, buf));
        snprintf(buf, sizeof(buf), "down_btn_field%d.padding", loc);
        evas_object_hide(elm_layout_content_unset(obj, buf));
        if (button_mod->entry_layout)
          evas_object_hide(elm_layout_content_unset(button_mod->entry_layout, buf));
     }

   for (idx = 0; idx < DATETIME_FIELD_COUNT; idx++)
     {
        field_type = (Elm_Datetime_Field_Type)evas_object_data_get(button_mod->datetime_field[idx], "_field_type");
        button_mod->mod_data.field_location_get(obj, field_type, &loc);

        if (elm_datetime_field_visible_get(obj,field_type))
          {
             snprintf(buf, sizeof(buf), "field%d", loc);
             elm_layout_content_set(obj, buf, button_mod->datetime_field[idx]);
             if (button_mod->entry_layout)
               elm_layout_content_set(button_mod->entry_layout, buf, button_mod->datetime_entry_field[idx]);
             snprintf(buf, sizeof(buf), "up_btn_field%d.padding", loc);
             elm_layout_content_set(obj, buf, button_mod->top_button[idx]);
             snprintf(buf, sizeof(buf), "down_btn_field%d.padding", loc);
             elm_layout_content_set(obj, buf, button_mod->foot_button[idx]);
          }
        if(button_mod->field_entry_created == EINA_TRUE)
        {
            if (button_mod->timepicker)
              {
                 if (loc == ELM_DATETIME_HOUR)
                   elm_entry_input_panel_return_key_type_set(button_mod->datetime_entry_field[loc], ELM_INPUT_PANEL_RETURN_KEY_TYPE_NEXT);
                 if (loc == ELM_DATETIME_MINUTE)
                   elm_entry_input_panel_return_key_type_set(button_mod->datetime_entry_field[loc], ELM_INPUT_PANEL_RETURN_KEY_TYPE_DONE);
              }
            else
              {
                 if (loc == (DATETIME_FIELD_COUNT - 1) || loc == (PICKER_FIELD_COUNT - 1))
                   elm_entry_input_panel_return_key_type_set(button_mod->datetime_entry_field[idx], ELM_INPUT_PANEL_RETURN_KEY_TYPE_DONE);
                 else
                   elm_entry_input_panel_return_key_type_set(button_mod->datetime_entry_field[idx], ELM_INPUT_PANEL_RETURN_KEY_TYPE_NEXT);
              }
        }
            button_mod->field_location[idx] = loc;
   }

   if(button_mod->field_entry_created == EINA_TRUE)
   {
       if (button_mod->timepicker)
         _set_timepicker_entry_filter(button_mod);
       else
         _set_datepicker_entry_filter(button_mod);
   }

   elm_layout_sizing_eval(obj);
}

static void
_fields_visible_set(Button_Module_Data *button_mod)
{
   Evas_Object *datetime;

   if (!button_mod) return;
   datetime = button_mod->mod_data.base;

   if (button_mod->timepicker)
     {
        elm_datetime_field_visible_set(datetime, ELM_DATETIME_YEAR, EINA_FALSE);
        elm_datetime_field_visible_set(datetime, ELM_DATETIME_MONTH, EINA_FALSE);
        elm_datetime_field_visible_set(datetime, ELM_DATETIME_DATE, EINA_FALSE);

        elm_datetime_field_visible_set(datetime, ELM_DATETIME_HOUR, EINA_TRUE);
        elm_datetime_field_visible_set(datetime, ELM_DATETIME_MINUTE, EINA_TRUE);
        elm_datetime_field_visible_set(datetime, ELM_DATETIME_AMPM, EINA_TRUE);

        evas_object_hide(button_mod->datetime_entry_field[ELM_DATETIME_YEAR]);
        evas_object_hide(button_mod->datetime_entry_field[ELM_DATETIME_MONTH]);
        evas_object_hide(button_mod->datetime_entry_field[ELM_DATETIME_DATE]);

        evas_object_show(button_mod->datetime_entry_field[ELM_DATETIME_HOUR]);
        evas_object_show(button_mod->datetime_entry_field[ELM_DATETIME_MINUTE]);
        evas_object_show(button_mod->datetime_entry_field[ELM_DATETIME_AMPM]);
     }
   else
     {
        elm_datetime_field_visible_set(datetime, ELM_DATETIME_YEAR, EINA_TRUE);
        elm_datetime_field_visible_set(datetime, ELM_DATETIME_MONTH, EINA_TRUE);
        elm_datetime_field_visible_set(datetime, ELM_DATETIME_DATE, EINA_TRUE);

        elm_datetime_field_visible_set(datetime, ELM_DATETIME_HOUR, EINA_FALSE);
        elm_datetime_field_visible_set(datetime, ELM_DATETIME_MINUTE, EINA_FALSE);
        elm_datetime_field_visible_set(datetime, ELM_DATETIME_AMPM, EINA_FALSE);

        evas_object_show(button_mod->datetime_entry_field[ELM_DATETIME_YEAR]);
        evas_object_show(button_mod->datetime_entry_field[ELM_DATETIME_MONTH]);
        evas_object_show(button_mod->datetime_entry_field[ELM_DATETIME_DATE]);

        evas_object_hide(button_mod->datetime_entry_field[ELM_DATETIME_HOUR]);
        evas_object_hide(button_mod->datetime_entry_field[ELM_DATETIME_MINUTE]);
        evas_object_hide(button_mod->datetime_entry_field[ELM_DATETIME_AMPM]);
     }
}

static void
_select_first_field(Button_Module_Data *button_mod)
{
   Evas_Object *datetime;
   Evas_Object *group;
   Evas_Object *first_obj;
   int value;

   if (!button_mod) return;
   datetime = button_mod->mod_data.base;
   group = button_mod->group;

   if (!button_mod->timepicker)
     {
        first_obj = elm_object_part_content_get(datetime, "field0");
        value = elm_radio_state_value_get(first_obj);
        elm_radio_value_set(group, value);
     }
   else
     {
        first_obj = elm_object_part_content_get(datetime, "field3");
        value = elm_radio_state_value_get(first_obj);
        elm_radio_value_set(group, value);
     }
}

static Eina_Bool
_value_increment_cb(void *data)
{
   Button_Module_Data *button_mod = (Button_Module_Data *)data;
   Evas_Object *datetime = button_mod->mod_data.base;
   struct tm min_time, max_time;
   int on_field = elm_radio_value_get(button_mod->group);

   elm_datetime_value_get(datetime, &(button_mod->set_time));
   button_mod->mod_data.fields_min_max_get(datetime, &(button_mod->set_time), &min_time, &max_time);

   switch (on_field)
     {
      case ELM_DATETIME_YEAR:
         button_mod->set_time.tm_year++;
         if (button_mod->set_time.tm_year > max_time.tm_year)
           button_mod->set_time.tm_year = min_time.tm_year;
         break;
      case ELM_DATETIME_MONTH:
         button_mod->set_time.tm_mon++;
         if (button_mod->set_time.tm_mon > max_time.tm_mon)
           button_mod->set_time.tm_mon = min_time.tm_mon;
         break;
      case ELM_DATETIME_DATE:
         button_mod->set_time.tm_mday++;
         if (button_mod->set_time.tm_mday > max_time.tm_mday)
           button_mod->set_time.tm_mday = min_time.tm_mday;
         break;
      case ELM_DATETIME_HOUR:
         button_mod->set_time.tm_hour++;
         if (button_mod->set_time.tm_hour > max_time.tm_hour)
           button_mod->set_time.tm_hour = min_time.tm_hour;
         if (button_mod->set_time.tm_hour < STRUCT_TM_TIME_12HRS_MAX_VALUE)
           _set_AM_PM_string(button_mod, EINA_TRUE);
         else
           _set_AM_PM_string(button_mod, EINA_FALSE);
         break;
      case ELM_DATETIME_MINUTE:
         button_mod->set_time.tm_min++;
         if (button_mod->set_time.tm_min > max_time.tm_min)
           button_mod->set_time.tm_min = min_time.tm_min;
         break;
      case ELM_DATETIME_AMPM:
      default:
         break;
     }
   button_mod->is_toggled = EINA_FALSE;
   elm_datetime_value_set(datetime, &(button_mod->set_time));
   return ECORE_CALLBACK_RENEW;
}

static Eina_Bool
_value_decrement_cb(void *data)
{
   Button_Module_Data *button_mod = (Button_Module_Data *)data;
   Evas_Object *datetime = button_mod->mod_data.base;
   struct tm min_time, max_time;

   int on_field = elm_radio_value_get(button_mod->group);

   elm_datetime_value_get(datetime, &(button_mod->set_time));
   button_mod->mod_data.fields_min_max_get(datetime, &(button_mod->set_time), &min_time, &max_time);
   switch (on_field)
     {
      case ELM_DATETIME_YEAR:
         button_mod->set_time.tm_year--;
         if (button_mod->set_time.tm_year < min_time.tm_year)
           button_mod->set_time.tm_year = max_time.tm_year;
         break;
      case ELM_DATETIME_MONTH:
         button_mod->set_time.tm_mon--;
         if (button_mod->set_time.tm_mon < min_time.tm_mon)
           button_mod->set_time.tm_mon = max_time.tm_mon;
         break;
      case ELM_DATETIME_DATE:
         button_mod->set_time.tm_mday--;
         if (button_mod->set_time.tm_mday < min_time.tm_mday)
           button_mod->set_time.tm_mday = max_time.tm_mday;
         break;
      case ELM_DATETIME_HOUR:
         button_mod->set_time.tm_hour--;
         if (button_mod->set_time.tm_hour < min_time.tm_hour)
           button_mod->set_time.tm_hour = max_time.tm_hour;
         if (button_mod->set_time.tm_hour < STRUCT_TM_TIME_12HRS_MAX_VALUE)
           _set_AM_PM_string(button_mod, EINA_TRUE);
         else
           _set_AM_PM_string(button_mod, EINA_FALSE);
         break;
      case ELM_DATETIME_MINUTE:
         button_mod->set_time.tm_min--;
         if (button_mod->set_time.tm_min < min_time.tm_min)
           button_mod->set_time.tm_min = max_time.tm_min;
         break;
      case ELM_DATETIME_AMPM:
      default:
         break;
     }
   button_mod->is_toggled = EINA_FALSE;
   elm_datetime_value_set(datetime, &(button_mod->set_time));
   return ECORE_CALLBACK_RENEW;
}

static Eina_Bool
_longpress_top_button_timer_cb(void *data)
{
   Button_Module_Data *button_mod = (Button_Module_Data *)data;

   button_mod->longpress_timer = NULL;
   if (button_mod->spin_timer) ecore_timer_del(button_mod->spin_timer);

   button_mod->spin_timer = ecore_timer_add(ROTATE_TIME_NORMAL, _value_increment_cb, button_mod);
   return ECORE_CALLBACK_CANCEL;
}

static Eina_Bool
_longpress_foot_button_timer_cb(void *data)
{
   Button_Module_Data *button_mod = (Button_Module_Data *)data;

   button_mod->longpress_timer = NULL;
   if (button_mod->spin_timer) ecore_timer_del(button_mod->spin_timer);

   button_mod->spin_timer = ecore_timer_add(ROTATE_TIME_NORMAL, _value_decrement_cb, button_mod);
   return ECORE_CALLBACK_CANCEL;
}

static void
_button_top_pressed_cb(void *data,
             Evas_Object *obj,
             void *event_info EINA_UNUSED)
{
   Button_Module_Data *button_mod = (Button_Module_Data *)data;
   _field_state_set(button_mod, obj, "_field_type_top_btn");
   if (button_mod->longpress_timer) ecore_timer_del(button_mod->longpress_timer);

   button_mod->longpress_timer = ecore_timer_add(_elm_config->longpress_timeout, _longpress_top_button_timer_cb, button_mod);
}

static void
_button_foot_pressed_cb(void *data,
             Evas_Object *obj,
             void *event_info EINA_UNUSED)
{
   Button_Module_Data *button_mod = (Button_Module_Data *)data;
   _field_state_set(button_mod, obj, "_field_type_foot_btn");
   if (button_mod->longpress_timer) ecore_timer_del(button_mod->longpress_timer);

   button_mod->longpress_timer = ecore_timer_add(_elm_config->longpress_timeout, _longpress_foot_button_timer_cb, button_mod);
}

static void
_button_top_foot_unpressed_cb(void *data,
             Evas_Object *obj EINA_UNUSED,
             void *event_info EINA_UNUSED)
{
   Button_Module_Data *button_mod = (Button_Module_Data *)data;
   if (button_mod->longpress_timer)
     {
        ecore_timer_del(button_mod->longpress_timer);
        button_mod->longpress_timer = NULL;
     }
   if (button_mod->spin_timer)
     {
        ecore_timer_del(button_mod->spin_timer);
        button_mod->spin_timer = NULL;
     }
}

static void
_button_foot_clicked_cb(void *data,
                  Evas_Object *obj,
                  void *event_info EINA_UNUSED)
{
   Elm_Datetime_Field_Type field_type;
   Button_Module_Data *button_mod;
   button_mod = (Button_Module_Data *)data;
   _value_decrement_cb(button_mod);

   field_type = (Elm_Datetime_Field_Type)evas_object_data_get(obj, "_field_type_foot_btn");
   if (_elm_config->access_mode)
     _access_increment_decrement_info_say(obj, button_mod->datetime_field[field_type], EINA_FALSE);
}

static void
_button_top_clicked_cb(void *data,
                  Evas_Object *obj,
                  void *event_info EINA_UNUSED)
{
   Elm_Datetime_Field_Type field_type;
   Button_Module_Data *button_mod;
   button_mod = (Button_Module_Data *)data;
   _value_increment_cb(data);

   field_type = (Elm_Datetime_Field_Type)evas_object_data_get(obj, "_field_type_top_btn");
   if (_elm_config->access_mode)
     _access_increment_decrement_info_say(obj, button_mod->datetime_field[field_type], EINA_TRUE);
}

static Evas_Object *
_top_button_field_create(Button_Module_Data *button_mod)
{
   Evas_Object *top;
   Evas_Object *obj = button_mod->mod_data.base;

   top = elm_button_add(obj);
   elm_object_style_set(top, "datetime/top_btn");
   evas_object_size_hint_align_set(top, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_size_hint_weight_set(top, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_smart_callback_add(top, "clicked", _button_top_clicked_cb, button_mod);
   evas_object_smart_callback_add(top, "unpressed", _button_top_foot_unpressed_cb, button_mod);
   evas_object_smart_callback_add(top, "pressed", _button_top_pressed_cb, button_mod);
   return top;
}

static Evas_Object *
_foot_button_field_create(Button_Module_Data *button_mod)
{
   Evas_Object *foot;
   Evas_Object *obj = button_mod->mod_data.base;

   foot = elm_button_add(obj);
   elm_object_style_set(foot, "datetime/foot_btn");
   evas_object_size_hint_align_set(foot, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_size_hint_weight_set(foot, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_smart_callback_add(foot, "clicked", _button_foot_clicked_cb, button_mod);
   evas_object_smart_callback_add(foot, "unpressed", _button_top_foot_unpressed_cb, button_mod);
   evas_object_smart_callback_add(foot, "pressed", _button_foot_pressed_cb, button_mod);
   return foot;
}

static void
_weekday_loc_update(Button_Module_Data *button_mod)
{
   char *fmt;

   if (!button_mod) return;

   fmt = nl_langinfo(D_T_FMT);
   if (!strncmp(fmt, "%a", 2) || !strncmp(fmt, "%A", 2))
     button_mod->weekday_loc_first = EINA_TRUE;
   else
     button_mod->weekday_loc_first = EINA_FALSE;
}

static void
_module_format_change(Button_Module_Data *button_mod)
{
   const char *fmt;
   if (!button_mod) return;

   fmt = button_mod->mod_data.field_format_get(button_mod->mod_data.base, ELM_DATETIME_MONTH);
   if (!strcmp(fmt, "%b"))
      elm_entry_input_panel_layout_set(button_mod->datetime_entry_field[ELM_DATETIME_MONTH], ELM_INPUT_PANEL_LAYOUT_MONTH);
   else
      elm_entry_input_panel_layout_set(button_mod->datetime_entry_field[ELM_DATETIME_MONTH], ELM_INPUT_PANEL_LAYOUT_DATETIME);

   _adjust_layout(button_mod, button_mod->mod_data.base);
   if (button_mod->entry_layout)
     _adjust_layout(button_mod, button_mod->entry_layout);
   _select_first_field(button_mod);
   _weekday_loc_update(button_mod);
}

static void
_module_language_changed_cb(void *data,
                            Evas_Object *obj EINA_UNUSED,
                            void *event_info EINA_UNUSED)
{
   Button_Module_Data *button_mod;
   button_mod = (Button_Module_Data *)data;
   if (!button_mod) return;
}

static void
_weekday_show_cb(void *data,
                 Evas_Object *obj EINA_UNUSED,
                 const char *emission EINA_UNUSED,
                 const char *source EINA_UNUSED)
{
   Button_Module_Data *button_mod;
   int idx, loc, weekday_loc;

   button_mod = (Button_Module_Data *)data;
   if (!button_mod) return;

   button_mod->weekday_show = EINA_TRUE;

   weekday_loc = (button_mod->weekday_loc_first) ? 0 : 2;
   for (idx = 0; idx <= ELM_DATETIME_DATE; idx++)
     {
        button_mod->mod_data.field_location_get(button_mod->mod_data.base, idx, &loc);
        if (loc == weekday_loc)
         {
            _field_value_display(button_mod, button_mod->datetime_field[idx]);
            break;
         }
     }
}

static void
_weekday_hide_cb(void *data,
                 Evas_Object *obj EINA_UNUSED,
                 const char *emission EINA_UNUSED,
                 const char *source EINA_UNUSED)
{
   Button_Module_Data *button_mod;
   int idx, loc, weekday_loc;

   button_mod = (Button_Module_Data *)data;
   if (!button_mod) return;

   button_mod->weekday_show = EINA_FALSE;
   weekday_loc = (button_mod->weekday_loc_first) ? 0 : 2;
   for (idx = 0; idx <= ELM_DATETIME_DATE; idx++)
     {
        button_mod->mod_data.field_location_get(button_mod->mod_data.base, idx, &loc);
        if (loc == weekday_loc)
         {
            _field_value_display(button_mod, button_mod->datetime_field[idx]);
            break;
         }
     }
}

static void
_field_value_display(Button_Module_Data *button_mod, Evas_Object *obj)
{
   Elm_Datetime_Field_Type  field_type;
   struct tm curr_time;
   char buf[BUFF_SIZE] = {0,};
   char weekday[BUFF_SIZE], label[BUFF_SIZE];
   char field[BUFF_SIZE] = {0,};
   const char *fmt;
   char *locale_tmp;
   char locale[32] = {0,};
   char *p = NULL;
   int loc, weekday_loc;
   Eina_Bool is_weekday_shown;

   UDateFormat *dt_formatter = NULL;
   UErrorCode status = U_ZERO_ERROR;
   UChar str[256];
   UChar Pattern[256] = {0, };
   UChar Ufield[256] = {0, };
   UDate date;
   UCalendar *calendar;
   int32_t pos;

   elm_datetime_value_get(button_mod->mod_data.base, &curr_time);
   field_type = (Elm_Datetime_Field_Type )evas_object_data_get(obj, "_field_type");
   fmt = button_mod->mod_data.field_format_get(button_mod->mod_data.base, field_type);
   button_mod->time_12hr_fmt = button_mod->mod_data.field_location_get(button_mod->mod_data.base, ELM_DATETIME_AMPM, NULL);

   locale_tmp = setlocale(LC_ALL, NULL);
   strcpy(locale, locale_tmp);

   if (locale[0] != '\0')
     {
        p = strstr(locale, ".UTF-8");
        if (p) *p = 0;
     }

   switch (field_type)
     {
      case ELM_DATETIME_YEAR:
        if (!strcmp(fmt, "%y"))
          u_uastrncpy(Pattern, "yy", strlen("yy"));
        else
          u_uastrncpy(Pattern, "yyyy", strlen("yyyy"));
        snprintf(field, sizeof(field), "%d", (curr_time.tm_year + STRUCT_TM_YEAR_BASE_VALUE));
        u_uastrncpy(Ufield, field, strlen(field));
        break;

      case ELM_DATETIME_MONTH:
        if (!strcmp(fmt, "%B"))
          u_uastrncpy(Pattern, "MMMM", strlen("MMMM"));
        else if (!strcmp(fmt, "%m"))
          u_uastrncpy(Pattern, "MM", strlen("MM"));
        else
          u_uastrncpy(Pattern, "MMM", strlen("MMM"));
        snprintf(field, sizeof(field), "%d", curr_time.tm_mon + 1);
        u_uastrncpy(Ufield, field, strlen(field));
        break;
      case ELM_DATETIME_DATE:
        if (!strcmp(fmt, "%e"))
          u_uastrncpy(Pattern, "d", strlen("d"));
        else
          u_uastrncpy(Pattern, "dd", strlen("dd"));
        snprintf(field, sizeof(field), "%d", curr_time.tm_mday);
        u_uastrncpy(Ufield, field, strlen(field));
         break;
      case ELM_DATETIME_HOUR:
        if (button_mod->time_12hr_fmt && !strcmp(locale,"ja_JP"))
          u_uastrncpy(Pattern, "KK", strlen("KK"));
        else if(button_mod->time_12hr_fmt)
          u_uastrncpy(Pattern, "hh", strlen("hh"));
        else
          u_uastrncpy(Pattern, "HH", strlen("HH"));
        snprintf(field, sizeof(field), "%d", curr_time.tm_hour);
        u_uastrncpy(Ufield, field, strlen(field));
        break;
      case ELM_DATETIME_MINUTE:
        u_uastrncpy(Pattern, "mm", strlen("mm"));
        snprintf(field, sizeof(field), "%d", curr_time.tm_min);
        u_uastrncpy(Ufield, field, strlen(field));
        break;
      case ELM_DATETIME_AMPM:
        if (!button_mod->is_toggled)
          return;
        break;
      default:
        break;
     }

   if (field_type != ELM_DATETIME_AMPM)
     {
        dt_formatter = udat_open(UDAT_IGNORE, UDAT_IGNORE, locale, NULL, -1, Pattern, -1, &status);
        calendar = ucal_open(NULL, 0, locale, UCAL_GREGORIAN, &status);
        ucal_clear(calendar);
        pos = 0;
        udat_parseCalendar(dt_formatter, calendar, Ufield, sizeof(Ufield), &pos,&status);
        date = ucal_getMillis(calendar, &status);
        udat_format(dt_formatter, date, str, sizeof(str), NULL, &status);
        u_austrcpy(buf, str);
        udat_close(dt_formatter);
     }
   if ((!buf[0]) && ((!strcmp(fmt, "%p")) || (!strcmp(fmt, "%P"))))
     {
        if (curr_time.tm_hour < STRUCT_TM_TIME_12HRS_MAX_VALUE)
         _set_AM_PM_string(button_mod, EINA_TRUE);
        else
         _set_AM_PM_string(button_mod, EINA_FALSE);
     }
   else if (!button_mod->weekday_show)
     {
        elm_object_text_set(obj, buf);
        elm_object_text_set(button_mod->datetime_entry_field[field_type], buf);
     }
   else
     {
        /* FIXME: To restrict month wrapping because of summer time in some locales,
         * ignore day light saving mode in mktime(). */
        curr_time.tm_isdst = -1;

        mktime(&curr_time);
        strftime(weekday, sizeof(weekday), "%a", &curr_time);
        weekday_loc = (button_mod->weekday_loc_first) ? 0 : 2;
        is_weekday_shown = button_mod->mod_data.field_location_get(
                           button_mod->mod_data.base, field_type, &loc);
        if (!is_weekday_shown || (loc != weekday_loc))
          {
             elm_object_text_set(obj, buf);
             elm_object_text_set(button_mod->datetime_entry_field[field_type], buf);
          }
        else if (loc == 0)
          {
             snprintf(label, sizeof(label), "%s %s", weekday, buf);
             elm_object_text_set(obj, label);
             elm_object_text_set(button_mod->datetime_entry_field[field_type], label);
          }
        else
          {
             snprintf(label, sizeof(label), "%s %s", buf, weekday);
             elm_object_text_set(obj, label);
             elm_object_text_set(button_mod->datetime_entry_field[field_type], label);
          }
     }
   _field_access_register(button_mod, obj, field_type);
}

static char *
_access_info_top_btn_cb(void *data, Evas_Object *obj)
{
    Button_Module_Data *button_mod;
    button_mod = (Button_Module_Data *)data;
   _field_state_set(button_mod, obj, "_field_type_top_btn");
    return NULL;
}

static char *
_access_info_foot_btn_cb(void *data, Evas_Object *obj)
{
    Button_Module_Data *button_mod;
    button_mod = (Button_Module_Data *)data;
   _field_state_set(button_mod, obj, "_field_type_foot_btn");
    return NULL;
}

static char *
_access_info_field_cb(void *data, Evas_Object *obj)
{
    Button_Module_Data *button_mod;
    button_mod = (Button_Module_Data *)data;
   _field_state_set(button_mod, obj, "_field_type");
    return NULL;
}

static void
_field_access_clicked_cb(void *data, Evas_Object *obj, void *event_info EINA_UNUSED)
{
   Elm_Datetime_Field_Type field_type;
   Button_Module_Data *button_mod;
   int idx;
   button_mod = (Button_Module_Data *)data;
   if (!button_mod) return;

   field_type = (Elm_Datetime_Field_Type)evas_object_data_get(obj, "_field_type");
   if (field_type == ELM_DATETIME_AMPM)
     {
        _am_pm_value_set(button_mod);
        _elm_access_say(obj, elm_object_text_get(obj), EINA_FALSE);
        return;
     }
   if (button_mod->field_entry_created == EINA_FALSE)
     _field_entry_create(button_mod);
   for (idx = 0; idx < DATETIME_FIELD_COUNT; idx++)
      elm_object_text_set(button_mod->datetime_entry_field[idx],elm_object_text_get(button_mod->datetime_field[idx]));

   _field_access_register(button_mod, button_mod->datetime_field[field_type], field_type);
   if (button_mod->parent_nf)
     elm_object_tree_focus_allow_set(button_mod->parent_nf, EINA_FALSE);

   evas_object_show(button_mod->win);
   evas_object_show(button_mod->bg);
   evas_object_show(button_mod->parent_layout);
   evas_object_show(button_mod->entry_layout);
   elm_object_focus_set(button_mod->datetime_entry_field[field_type], EINA_TRUE);
   elm_entry_select_all(button_mod->datetime_entry_field[field_type]);
}

static void
_field_access_register(Button_Module_Data *button_mod, Evas_Object *obj, Elm_Datetime_Field_Type field_type)
{
   const char* type = E_("IDS_IDLE_BODY_TAP_TO_CHANGE");
   Evas_Object *ao;

   _elm_access_text_set(_elm_access_object_get(obj), ELM_ACCESS_INFO, elm_object_text_get(obj));

   if (field_type == ELM_DATETIME_AMPM)
     _elm_access_text_set(_elm_access_object_get(obj), ELM_ACCESS_TYPE, type);
   else
     _elm_access_text_set(_elm_access_object_get(obj), ELM_ACCESS_TYPE, NULL);

   ao = elm_access_object_get(elm_entry_textblock_get(button_mod->datetime_entry_field[field_type]));
   _elm_access_text_set(_elm_access_object_get(ao), ELM_ACCESS_INFO,
   elm_object_text_get(button_mod->datetime_entry_field[field_type]));

   if (field_type == ELM_DATETIME_AMPM)
     {
        _elm_access_text_set(_elm_access_object_get(ao), ELM_ACCESS_TYPE, type);
        _elm_access_callback_set(_elm_access_object_get(ao), ELM_ACCESS_STATE, NULL, NULL);
     }
   else
     _elm_access_text_set(_elm_access_object_get(ao), ELM_ACCESS_TYPE, NULL);
}

EAPI Eina_List*
focus_object_list_get(Elm_Datetime_Module_Data *module_data)
{
   Evas_Object *obj;
   Button_Module_Data *button_mod;
   Elm_Datetime_Field_Type field_type;
   Eina_List *items = NULL;
   int loc = 0, loc2;
   unsigned int idx;
   Eina_Bool visible[DATETIME_FIELD_COUNT];

   button_mod = (Button_Module_Data *)module_data;
   if (!button_mod) return NULL;

   obj = button_mod->mod_data.base;

   for (idx = 0; idx < DATETIME_FIELD_COUNT; idx++)
     {
         field_type = (Elm_Datetime_Field_Type)evas_object_data_get(button_mod->datetime_field[idx], "_field_type");
         if (elm_datetime_field_visible_get(obj, field_type)) visible[idx] = EINA_TRUE;
         else visible[idx] = EINA_FALSE;
     }
   for (loc = 0; loc < DATETIME_FIELD_COUNT; loc++)
     {
         for (idx = 0; idx < DATETIME_FIELD_COUNT; idx++)
           {
               field_type = (Elm_Datetime_Field_Type)evas_object_data_get(button_mod->datetime_field[idx], "_field_type");
               button_mod->mod_data.field_location_get(obj, field_type, &loc2);
               if ((loc2 == loc) && (visible[idx]))
                 {
                    items = eina_list_append(items, button_mod->top_button[idx]);
                    items = eina_list_append(items, button_mod->datetime_field[idx]);
                    items = eina_list_append(items, button_mod->foot_button[idx]);
                 }
            }
      }
   return items;
}

EAPI void
access_register(Elm_Datetime_Module_Data *module_data)
{
   const char* type = E_("IDS_ACCS_BODY_BUTTON_TTS");
   Button_Module_Data *button_mod;
   Elm_Access_Info *ai_top, *ai_foot;
   int idx;

   button_mod = (Button_Module_Data *)module_data;
   if (!button_mod) return;

   for (idx = 0; idx < DATETIME_FIELD_COUNT; idx++)
     {
        ai_top = _elm_access_object_get(button_mod->top_button[idx]);
        ai_foot = _elm_access_object_get(button_mod->foot_button[idx]);

        switch (idx)
          {
             case ELM_DATETIME_YEAR:
               _elm_access_text_set(ai_top , ELM_ACCESS_TYPE,  E_("IDS_COM_BODY_INCREASE_YEAR"));
               _elm_access_text_set(ai_foot , ELM_ACCESS_TYPE,  E_("IDS_COM_BODY_DECREASE_YEAR"));
               break;

             case ELM_DATETIME_MONTH:
               _elm_access_text_set(ai_top , ELM_ACCESS_TYPE,  E_("IDS_COM_BODY_INCREASE_MONTH"));
               _elm_access_text_set(ai_foot , ELM_ACCESS_TYPE,  E_("IDS_COM_BODY_DECREASE_MONTH"));
               break;

             case ELM_DATETIME_DATE:
               _elm_access_text_set(ai_top , ELM_ACCESS_TYPE,  E_("IDS_COM_BODY_INCREASE_DAY"));
               _elm_access_text_set(ai_foot , ELM_ACCESS_TYPE,  E_("IDS_COM_BODY_DECREASE_DAY"));
               break;

             case ELM_DATETIME_HOUR:
               _elm_access_text_set(ai_top , ELM_ACCESS_TYPE,  E_("IDS_COM_BODY_INCREASE_HOUR"));
               _elm_access_text_set(ai_foot , ELM_ACCESS_TYPE,  E_("IDS_COM_BODY_DECREASE_HOUR"));
               break;

             case ELM_DATETIME_MINUTE:
               _elm_access_text_set(ai_top , ELM_ACCESS_TYPE,  E_("IDS_COM_BODY_INCREASE_MINUTE"));
               _elm_access_text_set(ai_foot , ELM_ACCESS_TYPE,  E_("IDS_COM_BODY_DECREASE_MINUTE"));
               break;

             default:
               break;
          }

        _elm_access_text_set(ai_top , ELM_ACCESS_CONTEXT_INFO, type);
        _elm_access_callback_set(ai_top, ELM_ACCESS_INFO, _access_info_top_btn_cb, button_mod);
        _elm_access_text_set(ai_foot , ELM_ACCESS_CONTEXT_INFO, type);
        _elm_access_callback_set(ai_foot, ELM_ACCESS_INFO, _access_info_foot_btn_cb, button_mod);

        evas_object_smart_callback_add(button_mod->datetime_field[idx], "clicked", _field_access_clicked_cb, button_mod);
        _elm_access_callback_set(_elm_access_object_get(button_mod->datetime_field[idx]),
                                  ELM_ACCESS_STATE, _access_info_field_cb, button_mod);
     }
}

// module functions for the specific module type

EAPI void
display_fields(Elm_Datetime_Module_Data *module_data)
{
   Evas_Object *obj;
   Button_Module_Data *button_mod;
   Elm_Datetime_Field_Type field_type;
   int idx;

   button_mod = (Button_Module_Data *)module_data;
   if (!button_mod) return;

   obj = button_mod->mod_data.base;
   for (idx = 0; idx < DATETIME_FIELD_COUNT; idx++)
     {
         field_type = (Elm_Datetime_Field_Type)evas_object_data_get(button_mod->datetime_field[idx], "_field_type");
         if (elm_datetime_field_visible_get(obj, field_type))
           _field_value_display(button_mod, button_mod->datetime_field[idx]);
     }
   button_mod->is_toggled = EINA_TRUE;
}

static
void _field_entry_create(void* data)
{
    Button_Module_Data *button_mod;
    Evas_Object *entry_field_obj, *obj;
    Elm_Datetime_Field_Type field_type;
    button_mod = (Button_Module_Data *)data;
    int idx, loc;
    char buf[BUFF_SIZE];

    if (!button_mod) return;

    for (idx = 0; idx < DATETIME_FIELD_COUNT; idx++)
      {
        entry_field_obj = elm_entry_add(button_mod->entry_layout);

        elm_entry_scrollable_set(entry_field_obj, EINA_FALSE);
        elm_entry_single_line_set(entry_field_obj, EINA_TRUE);
        elm_entry_select_allow_set(entry_field_obj, EINA_TRUE);
        elm_entry_cnp_mode_set(entry_field_obj, ELM_CNP_MODE_PLAINTEXT);
        elm_entry_magnifier_disabled_set(entry_field_obj, EINA_TRUE);
        elm_entry_cursor_handler_disabled_set(entry_field_obj, EINA_TRUE);

        evas_object_smart_callback_add(entry_field_obj, "focused", _entry_focused_cb, button_mod);
        evas_object_smart_callback_add(entry_field_obj, "clicked", _entry_clicked_cb, button_mod);
        evas_object_smart_callback_add(entry_field_obj, "clicked,double", _entry_double_clicked_cb, button_mod);

        switch (idx)
          {
             case ELM_DATETIME_YEAR:
                snprintf(buf, sizeof(buf), "datetime/style%d", 1);
                break;
             case ELM_DATETIME_MONTH:
                snprintf(buf, sizeof(buf), "datetime/style%d", 2);
                break;
             case ELM_DATETIME_DATE:
                snprintf(buf, sizeof(buf), "datetime/style%d", 1);
                break;
             case ELM_DATETIME_HOUR:
                snprintf(buf, sizeof(buf), "datetime/style%d", 1);
                break;
             case ELM_DATETIME_MINUTE:
                snprintf(buf, sizeof(buf), "datetime/style%d", 1);
                break;
             case ELM_DATETIME_AMPM:
                snprintf(buf, sizeof(buf), "datetime/style%d", 3);
                break;
          }

        evas_object_data_set(entry_field_obj, "_entry_field_type", (void *)idx);
        button_mod->datetime_entry_field[idx] = entry_field_obj;
        button_mod->field_entry_created = EINA_TRUE;

        if (idx != ELM_DATETIME_AMPM)
          {
             elm_object_style_set(entry_field_obj, buf);
             evas_object_smart_callback_add(entry_field_obj, "activated", _entry_activated_cb, button_mod);
             evas_object_smart_callback_add(entry_field_obj, "unfocused", _entry_unfocused_cb, button_mod);
             elm_entry_input_panel_layout_set(entry_field_obj, ELM_INPUT_PANEL_LAYOUT_DATETIME);
             _input_panel_context_set(button_mod, idx);
          }
        else
          {
             elm_object_style_set(entry_field_obj, "datetime/AM_PM");
             elm_entry_input_panel_enabled_set(entry_field_obj, EINA_FALSE);
             elm_entry_magnifier_disabled_set(entry_field_obj, EINA_TRUE);
             elm_object_focus_allow_set(entry_field_obj, EINA_FALSE);
          }
      }

    obj = button_mod->mod_data.base;
    for (idx = 0; idx < DATETIME_FIELD_COUNT; idx++)
      {
         field_type = (Elm_Datetime_Field_Type)evas_object_data_get(button_mod->datetime_field[idx], "_field_type");
         button_mod->mod_data.field_location_get(obj, field_type, &loc);

         if (button_mod->timepicker)
           {
              if (loc == ELM_DATETIME_HOUR)
                elm_entry_input_panel_return_key_type_set(button_mod->datetime_entry_field[loc], ELM_INPUT_PANEL_RETURN_KEY_TYPE_NEXT);
              if (loc == ELM_DATETIME_MINUTE)
                elm_entry_input_panel_return_key_type_set(button_mod->datetime_entry_field[loc], ELM_INPUT_PANEL_RETURN_KEY_TYPE_DONE);
           }
         else
           {
              if (loc == (DATETIME_FIELD_COUNT - 1) || loc == (PICKER_FIELD_COUNT - 1))
                elm_entry_input_panel_return_key_type_set(button_mod->datetime_entry_field[idx], ELM_INPUT_PANEL_RETURN_KEY_TYPE_DONE);
              else
                elm_entry_input_panel_return_key_type_set(button_mod->datetime_entry_field[idx], ELM_INPUT_PANEL_RETURN_KEY_TYPE_NEXT);
           }
         button_mod->field_location[idx] = loc;
      }

    if (button_mod->timepicker)
      _set_timepicker_entry_filter(button_mod);
    else
      _set_datepicker_entry_filter(button_mod);

    obj_format_hook(button_mod);
}

EAPI void
create_fields(Elm_Datetime_Module_Data *module_data)
{
   Button_Module_Data *button_mod;
   Evas_Object *field_obj, *top_foot_btn;
   int idx;
   char buf[BUFF_SIZE];

   button_mod = (Button_Module_Data *)module_data;
   if (!button_mod) return;

   button_mod->entry_layout = _entry_layout_create(button_mod);

   for (idx = 0; idx < DATETIME_FIELD_COUNT; idx++)
     {
        field_obj = elm_radio_add(button_mod->mod_data.base);
        elm_layout_signal_callback_add(field_obj, "elm,action,radio,clicked", "", _radio_clicked_cb, button_mod);

        switch (idx)
          {
             case ELM_DATETIME_YEAR:
                snprintf(buf, sizeof(buf), "datetime/style%d", 1);
                break;
             case ELM_DATETIME_MONTH:
                snprintf(buf, sizeof(buf), "datetime/style%d", 2);
                break;
             case ELM_DATETIME_DATE:
                snprintf(buf, sizeof(buf), "datetime/style%d", 1);
                break;
             case ELM_DATETIME_HOUR:
                snprintf(buf, sizeof(buf), "datetime/style%d", 1);
                break;
             case ELM_DATETIME_MINUTE:
                snprintf(buf, sizeof(buf), "datetime/style%d", 1);
                break;
             case ELM_DATETIME_AMPM:
                snprintf(buf, sizeof(buf), "datetime/style%d", 3);
                break;
          }

        elm_object_style_set(field_obj, buf);

        elm_radio_state_value_set(field_obj, idx);
        evas_object_data_set(field_obj, "_field_type", (void *)idx);
        button_mod->datetime_field[idx] = field_obj;

        if (!button_mod->group)
          {
             button_mod->group = field_obj;
             elm_radio_value_set(button_mod->group, 0);
          }
        else
          elm_radio_group_add(field_obj, button_mod->group);

        if (idx != ELM_DATETIME_AMPM)
          {
             top_foot_btn = _top_button_field_create(button_mod);
             button_mod->top_button[idx] = top_foot_btn;
             evas_object_data_set(top_foot_btn, "_field_type_top_btn", (void *)idx);
             top_foot_btn = _foot_button_field_create(button_mod);
             button_mod->foot_button[idx] = top_foot_btn;
             evas_object_data_set(top_foot_btn, "_field_type_foot_btn", (void *)idx);
          }
     }

   _field_list_arrange(button_mod);
}


EAPI Elm_Datetime_Module_Data *
obj_hook(Evas_Object *obj)
{
   Button_Module_Data *button_mod;

   button_mod = ELM_NEW(Button_Module_Data);
   if (!button_mod) return NULL;

   evas_object_smart_callback_add(obj, "language,changed",
                                  _module_language_changed_cb, button_mod);
   elm_object_signal_callback_add(obj, "weekday,show", "",
                                  _weekday_show_cb, button_mod);
   elm_object_signal_callback_add(obj, "weekday,hide", "",
                                  _weekday_hide_cb, button_mod);

   button_mod->weekday_show = EINA_FALSE;
   button_mod->weekday_loc_first = EINA_TRUE;
   button_mod->group = NULL;
   button_mod->longpress_timer = NULL;
   button_mod->spin_timer = NULL;
   button_mod->field_entry_created = EINA_FALSE;
   button_mod->is_toggled = EINA_TRUE;

   return ((Elm_Datetime_Module_Data*)button_mod);
}

EAPI void
obj_unhook(Elm_Datetime_Module_Data *module_data)
{
   Button_Module_Data *button_mod;

   button_mod = (Button_Module_Data *)module_data;
   if (!button_mod) return;

   if (button_mod)
     {
        if (button_mod->longpress_timer)
          ecore_timer_del(button_mod->longpress_timer);
        if (button_mod->spin_timer)
          ecore_timer_del(button_mod->spin_timer);

        evas_object_del(button_mod->entry_layout);
        evas_object_del(button_mod->parent_layout);
        evas_object_del(button_mod->bg);
        evas_object_del(button_mod->win);

        free(button_mod);
        button_mod = NULL;
     }
}

EAPI void
obj_format_hook(Elm_Datetime_Module_Data *module_data)
{
   Button_Module_Data *button_mod;
   Eina_Bool visible = EINA_FALSE;
   const char *fmt;
   struct tm curr_time;

   button_mod = (Button_Module_Data *)module_data;
   if (!button_mod) return;
   visible = evas_object_visible_get(button_mod->win);
   if (visible)
     elm_object_tree_focus_allow_set(button_mod->win, EINA_FALSE);
   _field_list_arrange(button_mod);
   _module_format_change(button_mod);
   _fields_visible_set(button_mod);
   if (visible)
     {
        elm_object_tree_focus_allow_set(button_mod->win, EINA_TRUE);
        elm_object_focus_set(button_mod->datetime_entry_field[button_mod->focused_field], EINA_TRUE);
     }
   elm_datetime_value_get(button_mod->mod_data.base, &curr_time);
   fmt = button_mod->mod_data.field_format_get(button_mod->mod_data.base, ELM_DATETIME_AMPM);
   if (((!strcmp(fmt, "%p")) || (!strcmp(fmt, "%P"))))
     {
       if (curr_time.tm_hour < STRUCT_TM_TIME_12HRS_MAX_VALUE)
         _set_AM_PM_string(button_mod, EINA_TRUE);
       else
         _set_AM_PM_string(button_mod, EINA_FALSE);
     }
}

EAPI void
obj_theme_hook(Elm_Datetime_Module_Data *module_data)
{
   Button_Module_Data *button_mod;
   Evas_Object *datetime;
   const char *style;

   button_mod = (Button_Module_Data *)module_data;
   if (!button_mod) return;
   datetime = button_mod->mod_data.base;
   display_fields((Elm_Datetime_Module_Data *)button_mod);

   style = elm_object_style_get(datetime);

   if (style)
     {
        if (!strcmp(style, "timepicker_layout"))
          {
             button_mod->timepicker = EINA_TRUE;
             elm_layout_theme_set(button_mod->entry_layout, "datetime", "timepicker_layout", "entry");
          }
        else
          {
             button_mod->timepicker = EINA_FALSE;
             elm_layout_theme_set(button_mod->entry_layout, "datetime", "datepicker_layout", "entry");
          }
      }

   _fields_visible_set(button_mod);
   _adjust_layout(button_mod, button_mod->mod_data.base);
   if (button_mod->entry_layout)
     _adjust_layout(button_mod, button_mod->entry_layout);
   _select_first_field(button_mod);
}

EAPI void
sizing_eval_hook(Elm_Datetime_Module_Data *module_data)
{
   Button_Module_Data *button_mod;
   Evas_Object *obj;
   Evas_Coord minw = -1, minh = -1;
   unsigned int idx, field_count = 0;
   Elm_Datetime_Field_Type field_type;

   button_mod = (Button_Module_Data *)module_data;
   if (!button_mod) return;

   obj = button_mod->mod_data.base;
   for (idx = 0; idx < DATETIME_FIELD_COUNT; idx++)
     {
         field_type = (Elm_Datetime_Field_Type)evas_object_data_get(button_mod->datetime_field[idx], "_field_type");
         if (elm_datetime_field_visible_get(obj,field_type)) field_count++;
     }

   if (field_count)
     elm_coords_finger_size_adjust(field_count, &minw, 1, &minh);
   edje_object_size_min_restricted_calc
     (elm_layout_edje_get(obj), &minw, &minh, minw, minh);
   evas_object_size_hint_min_set(obj, minw, minh);
   evas_object_size_hint_max_set(obj, -1, -1);
}

EAPI void
obj_focus_hook(Elm_Datetime_Module_Data *module_data EINA_UNUSED)
{
   return;
}

// module api funcs needed
EAPI int
elm_modapi_init(void *m EINA_UNUSED)
{
   return 1; // succeed always
}

EAPI int
elm_modapi_shutdown(void *m EINA_UNUSED)
{
   return 1; // succeed always
}
