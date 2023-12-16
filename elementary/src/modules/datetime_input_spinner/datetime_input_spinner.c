#ifdef HAVE_CONFIG_H
#include "elementary_config.h"
#endif

#include <Elementary.h>
#include "elm_priv.h"
#include <unicode/udat.h>
#include <unicode/ustring.h>

#define DATETIME_FIELD_COUNT            6
#define FIELD_FORMAT_LEN                3
#define BUFF_SIZE                       100
#define STRUCT_TM_YEAR_BASE_VALUE       1900
#define STRUCT_TM_TIME_12HRS_MAX_VALUE  12
#define DATETIME_LOCALE_LENGTH_MAX 32

typedef struct _Input_Spinner_Module_Data Input_Spinner_Module_Data;

struct _Input_Spinner_Module_Data
{
   Elm_Datetime_Module_Data mod_data;
   Evas_Object *field_obj[DATETIME_FIELD_COUNT];
   Evas_Object *am, *pm, *ampm;
   Eina_Bool field_status[DATETIME_FIELD_COUNT];
   Eina_Bool time_12hr_fmt;
   Eina_Bool is_pm;
   Eina_Bool is_timepicker;
   Eina_Bool is_init;
   Eina_Bool is_month_changed;
   int pre_decimal_month;
   const char *pre_str_value;
   char user_locale[DATETIME_LOCALE_LENGTH_MAX];
};

static void _am_changed_cb(void *data, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED);
static void _pm_changed_cb(void *data, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED);

static void
_field_value_set(struct tm *tim, Elm_Datetime_Field_Type  field_type, int val)
{
   if (field_type >= DATETIME_FIELD_COUNT - 1) return;

   int *timearr[]= { &tim->tm_year, &tim->tm_mon, &tim->tm_mday, &tim->tm_hour, &tim->tm_min };
   *timearr[field_type] = val;
}

static void
_fields_visible_set(Input_Spinner_Module_Data *layout_mod)
{
   Evas_Object *datetime;

   if (!layout_mod) return;
   datetime = layout_mod->mod_data.base;

   if (layout_mod->is_timepicker)
     {
        elm_datetime_field_visible_set(datetime, ELM_DATETIME_YEAR, EINA_FALSE);
        elm_datetime_field_visible_set(datetime, ELM_DATETIME_MONTH, EINA_FALSE);
        elm_datetime_field_visible_set(datetime, ELM_DATETIME_DATE, EINA_FALSE);

        elm_datetime_field_visible_set(datetime, ELM_DATETIME_HOUR, EINA_TRUE);
        elm_datetime_field_visible_set(datetime, ELM_DATETIME_MINUTE, EINA_TRUE);
        elm_datetime_field_visible_set(datetime, ELM_DATETIME_AMPM, EINA_TRUE);
     }
   else
     {
        elm_datetime_field_visible_set(datetime, ELM_DATETIME_YEAR, EINA_TRUE);
        elm_datetime_field_visible_set(datetime, ELM_DATETIME_MONTH, EINA_TRUE);
        elm_datetime_field_visible_set(datetime, ELM_DATETIME_DATE, EINA_TRUE);

        elm_datetime_field_visible_set(datetime, ELM_DATETIME_HOUR, EINA_FALSE);
        elm_datetime_field_visible_set(datetime, ELM_DATETIME_MINUTE, EINA_FALSE);
        elm_datetime_field_visible_set(datetime, ELM_DATETIME_AMPM, EINA_FALSE);
     }
}

static void
_field_toggle_to_next_location(Input_Spinner_Module_Data *layout_mod, Evas_Object *spinner)
{
   Evas_Object *spinner_next = NULL;
   Eina_List *fields, *l;

   fields = layout_mod->mod_data.fields_sorted_get(layout_mod->mod_data.base);
   l = eina_list_data_find_list(fields, spinner);
   if (l)
     {
        if (eina_list_next(l))
          spinner_next = eina_list_data_get(eina_list_next(l));
        else
          spinner_next =  eina_list_data_get(fields);
     }

   if (spinner_next)
     {
        elm_layout_signal_emit(spinner, "elm,action,entry,toggle", "elm");
        edje_object_message_signal_process(elm_layout_edje_get(spinner));
        elm_layout_signal_emit(spinner_next, "elm,action,entry,toggle", "elm");
     }
}

char *
_text_insert(const char *text, const char *input, int pos)
{
   char *result;
   int text_len, input_len;

   text_len = strlen(text);
   input_len = strlen(input);
   result = (char *)calloc(text_len + input_len + 1,  sizeof(char));

   strncpy(result, text, pos);
   strcpy(result + pos, input);
   strcpy(result + pos + input_len, text + pos);

   return result;
}

static void
_year_validity_checking_filter(void *data, Evas_Object *obj, char **text)
{
   Input_Spinner_Module_Data *layout_mod;
   char *new_str, *insert;
   int min, max, val;
   const char *curr_str, *fmt;

   layout_mod = (Input_Spinner_Module_Data *)data;
   if (!layout_mod) return;

   if (!(*text) || !strcmp(*text, "")) return;

   curr_str = elm_object_text_get(obj);
   if (!curr_str || strlen(curr_str) < 3) return;

   insert = *text;
   new_str = _text_insert(curr_str, insert, elm_entry_cursor_pos_get(obj));
   if (!new_str) return;

   val = atoi(new_str);

   layout_mod->mod_data.field_limit_get(layout_mod->mod_data.base, ELM_DATETIME_YEAR, &min, &max);

   min += STRUCT_TM_YEAR_BASE_VALUE;
   max += STRUCT_TM_YEAR_BASE_VALUE;

   if (((val >= min) && (val <= max)) || val == 0)
     elm_entry_cursor_end_set(obj);
   else
     *insert = 0;

   free(new_str);
}

static void
_month_validity_checking_filter(void *data, Evas_Object *obj, char **text)
{
   Input_Spinner_Module_Data *layout_mod;
   char *new_str, *insert;
   const char *fmt;
   double min, max;
   int val, len, loc = 0;
   const char *curr_str;

   layout_mod = (Input_Spinner_Module_Data *)data;
   if (!layout_mod) return;

   if (!(*text) || !strcmp(*text, "")) return;

   fmt = layout_mod->mod_data.field_format_get(layout_mod->mod_data.base, ELM_DATETIME_MONTH);

   if (strcmp(fmt, "%m"))
     {
        elm_entry_entry_set(obj, ""); //clear entry first in case of abbreviated month name
        curr_str = elm_object_text_get(obj);
        if (!curr_str) return;
     }
   else
     {
        curr_str = elm_object_text_get(obj);
        if (!curr_str) return;

         if (!strcmp(*text, "1") || !strcmp(*text, "0"))
           len = strlen(curr_str);
         else
           len = strlen(*text);

         if (len < 1) return;
     }

   insert = *text;
   new_str = _text_insert(curr_str, insert, elm_entry_cursor_pos_get(obj));
   if (!new_str) return;

   val = atoi(new_str);

   elm_spinner_min_max_get(layout_mod->field_obj[ELM_DATETIME_MONTH], &min, &max);

   if (((val >= min) && (val <= max)) || val == 0)
     {
        layout_mod->is_month_changed = EINA_TRUE;

        elm_entry_entry_set(obj, new_str);
        elm_entry_cursor_end_set(obj);

        layout_mod->mod_data.field_location_get(layout_mod->mod_data.base, ELM_DATETIME_MONTH, &loc);
        if (loc != 2)
          _field_toggle_to_next_location(layout_mod, layout_mod->field_obj[ELM_DATETIME_MONTH]);
     }

   *insert = 0;

   free(new_str);
}

static void
_date_validity_checking_filter(void *data, Evas_Object *obj, char **text)
{
   Input_Spinner_Module_Data *layout_mod;
   char *new_str, *insert;
   int val;
   const char *curr_str;
   double min, max;

   layout_mod = (Input_Spinner_Module_Data *)data;
   if (!layout_mod) return;

   if (!(*text) || !strcmp(*text, "")) return;

   curr_str = elm_object_text_get(obj);
   if (!curr_str || strlen(curr_str) < 1) return;

   insert = *text;
   new_str = _text_insert(curr_str, insert, elm_entry_cursor_pos_get(obj));
   if (!new_str) return;

   val = atoi(new_str);

   elm_spinner_min_max_get(layout_mod->field_obj[ELM_DATETIME_DATE], &min, &max);

   if (((val >= min) && (val <= max)) || val == 0)
     elm_entry_cursor_end_set(obj);
   else
     *insert = 0;

   free(new_str);
}

static void
_hour_validity_checking_filter(void *data, Evas_Object *obj, char **text)
{
   Input_Spinner_Module_Data *layout_mod;
   char *new_str, *insert;
   int val, len;
   const char *curr_str;
   double min, max;

   layout_mod = (Input_Spinner_Module_Data *)data;
   if (!layout_mod) return;

   if (!(*text) || !strcmp(*text, "")) return;

   curr_str = elm_object_text_get(obj);
   if (!curr_str) return;

   if ((layout_mod->time_12hr_fmt && (!strcmp(*text, "1") || !strcmp(*text, "0"))) ||
       (!layout_mod->time_12hr_fmt && (!strcmp(*text, "1") || !strcmp(*text, "2") || !strcmp(*text, "0"))))
     len = strlen(curr_str);
   else
     len = strlen(*text);

   if (len < 1) return;

   insert = *text;
   new_str = _text_insert(curr_str, insert, elm_entry_cursor_pos_get(obj));
   if (!new_str) return;

   val = atoi(new_str);

   if (layout_mod->time_12hr_fmt)
     {
        if (val > STRUCT_TM_TIME_12HRS_MAX_VALUE)
          {
             *insert = 0;
             free(new_str);
             return;
          }
        else if (val == 0)
          sprintf(new_str, "%d", STRUCT_TM_TIME_12HRS_MAX_VALUE);
     }

   elm_spinner_min_max_get(layout_mod->field_obj[ELM_DATETIME_HOUR], &min, &max);

   if ((val >= min) && (val <= max))
     {
        elm_entry_entry_set(obj, new_str);
        elm_entry_cursor_end_set(obj);

        _field_toggle_to_next_location(layout_mod, layout_mod->field_obj[ELM_DATETIME_HOUR]);
     }

   *insert = 0;

    free(new_str);
}

static void
_minute_validity_checking_filter(void *data, Evas_Object *obj, char **text)
{
   Input_Spinner_Module_Data *layout_mod;
   char *new_str, *insert;
   int val;
   double min, max;
   const char *curr_str;

   layout_mod = (Input_Spinner_Module_Data *)data;
   if (!layout_mod) return;

   if (!(*text) || !strcmp(*text, "")) return;

   curr_str = elm_object_text_get(obj);
   if (!curr_str || strlen(curr_str) < 1) return;

   insert = *text;
   new_str = _text_insert(curr_str, insert, elm_entry_cursor_pos_get(obj));
   if (!new_str) return;

   val = atoi(new_str);

   elm_spinner_min_max_get(layout_mod->field_obj[ELM_DATETIME_MINUTE], &min, &max);

   if ((val >= min) && (val <= max))
     elm_entry_cursor_end_set(obj);
   else
     *insert = 0;

   free(new_str);
}

static void
_widget_theme_changed_cb(void *data, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Input_Spinner_Module_Data *layout_mod;
   Evas_Object *widget, *field_obj, *button, *am_btn = NULL, *pm_btn = NULL, *ampm_btn;
   const char *style;
   int idx, loc = 0;

   layout_mod = (Input_Spinner_Module_Data *)data;
   if (!layout_mod) return;

   widget = layout_mod->mod_data.base;
   style = elm_object_style_get(widget);

   if (!strcmp(style, "time_layout") || !strcmp(style, "time_layout_24hr") || !strcmp(style, "time_layout_clock"))
     {
        layout_mod->is_timepicker = EINA_TRUE;

        for (idx = ELM_DATETIME_HOUR; idx < ELM_DATETIME_AMPM; idx++)
          elm_object_style_set(layout_mod->field_obj[idx], "time_picker");

        field_obj = layout_mod->field_obj[ELM_DATETIME_AMPM];

        button = elm_layout_content_unset(field_obj, "am.radio");
        if (button) evas_object_hide(button);

        button = elm_layout_content_unset(field_obj, "pm.radio");
        if (button) evas_object_hide(button);

        button = elm_layout_content_unset(field_obj, "ampm.btn");
        if (button) evas_object_hide(button);

        if (!strcmp(style, "time_layout_clock") || !strcmp(style, "time_layout"))
          {
             layout_mod->time_12hr_fmt = EINA_TRUE;

             elm_layout_theme_set(field_obj, "layout", "datetime", "clock");

             if (layout_mod->am)
               elm_layout_content_set(field_obj, "am.radio", layout_mod->am);
             else
               {
                  am_btn = elm_radio_add(field_obj);
                  layout_mod->am = am_btn;
                  elm_object_style_set(am_btn, "datetime/ampm");
                  elm_radio_state_value_set(am_btn, 0);
                  elm_object_part_content_set(field_obj, "am.radio", am_btn);
                  evas_object_smart_callback_add(am_btn, "changed", _am_changed_cb, layout_mod);
               }

             if (layout_mod->pm)
               elm_layout_content_set(field_obj, "pm.radio", layout_mod->pm);
             else
               {
                  pm_btn = elm_radio_add(field_obj);
                  layout_mod->pm = pm_btn;
                  elm_object_style_set(pm_btn, "datetime/ampm");
                  elm_radio_state_value_set(pm_btn, 1);
                  elm_radio_group_add(pm_btn, layout_mod->am);
                  elm_object_part_content_set(field_obj, "pm.radio", pm_btn);
                  evas_object_smart_callback_add(pm_btn, "changed", _pm_changed_cb, layout_mod);
               }
          }
        else
          layout_mod->time_12hr_fmt = EINA_FALSE;

        layout_mod->mod_data.field_location_get(widget, ELM_DATETIME_AMPM, &loc);
        if (loc == 5)
          {
             elm_object_signal_emit(widget, "state,colon,visible,field3", "elm");
             elm_object_signal_emit(widget, "state,colon,invisible,field4", "elm");
          }
        else
          {
             elm_object_signal_emit(widget, "state,colon,visible,field4", "elm");
             elm_object_signal_emit(widget, "state,colon,invisible,field3", "elm");
          }

        edje_object_message_signal_process(elm_layout_edje_get(widget));
     }
   else
     {
       layout_mod->is_timepicker = EINA_FALSE;

       for (idx = ELM_DATETIME_YEAR; idx <= ELM_DATETIME_DATE; idx++)
          elm_object_style_set(layout_mod->field_obj[idx], "date_picker");
     }

   _fields_visible_set(layout_mod);
}

static void
_spinner_changed_cb(void *data, Evas_Object *obj, void *event_info EINA_UNUSED)
{
   Input_Spinner_Module_Data *layout_mod;
   Elm_Datetime_Field_Type  field_type;
   Evas_Object *entry;
   struct tm tim;
   int value;
   const char *fmt;

   layout_mod = (Input_Spinner_Module_Data *)data;
   if (!layout_mod) return;
   if (!layout_mod->is_init) return;

   field_type = (Elm_Datetime_Field_Type )evas_object_data_get(obj, "_field_type");

   value =  elm_spinner_value_get(obj);
   elm_datetime_value_get(layout_mod->mod_data.base, &tim);

   if (field_type == ELM_DATETIME_YEAR)
     value -=  1900;
   else if (field_type == ELM_DATETIME_MONTH)
     value -= 1;
   else if (field_type == ELM_DATETIME_HOUR && layout_mod->time_12hr_fmt)
     {
        entry = elm_object_part_content_get(obj, "elm.swallow.entry");
        if (elm_widget_focus_get(entry))
          {
             if (layout_mod->is_pm && value >= 0 && value < STRUCT_TM_TIME_12HRS_MAX_VALUE)
               value += STRUCT_TM_TIME_12HRS_MAX_VALUE;
             else if  (!layout_mod->is_pm && value == STRUCT_TM_TIME_12HRS_MAX_VALUE)
               value = 0;
          }
     }

   _field_value_set(&tim, field_type, value);

   elm_datetime_value_set(layout_mod->mod_data.base, &tim);
}

static void
_spinner_entry_changed_cb(void *data, Evas_Object *obj, void *event_info)
{
   Input_Spinner_Module_Data *layout_mod;
   char *str;
   int len, max_len, loc = 0;
   double min, max;
   Elm_Datetime_Field_Type  field_type;

   layout_mod = (Input_Spinner_Module_Data *)data;
   if (!layout_mod) return;

   str = event_info;
   len = strlen(str);
   field_type = (Elm_Datetime_Field_Type )evas_object_data_get(obj, "_field_type");

   if (field_type == ELM_DATETIME_MONTH || field_type == ELM_DATETIME_HOUR) return;
   elm_spinner_min_max_get(obj, &min, &max);
   max_len = log10(abs(max)) + 1;

   if (max_len == len)
     {
        if (!layout_mod->is_timepicker)
          {
             layout_mod->mod_data.field_location_get(layout_mod->mod_data.base, field_type, &loc);
             if (loc != 2)
               _field_toggle_to_next_location(layout_mod, obj);
          }
     }
}

static void
_spinner_entry_apply_cb(void *data, Evas_Object *obj, void *event_info)
{
   Input_Spinner_Module_Data *layout_mod;
   Elm_Datetime_Field_Type  field_type;
   Evas_Object *entry;
   double *value;
   const char *curr_str, *fmt;

   layout_mod = (Input_Spinner_Module_Data *)data;
   if (!layout_mod) return;

   value = event_info;
   field_type = (Elm_Datetime_Field_Type )evas_object_data_get(obj, "_field_type");

   entry = elm_object_part_content_get(obj, "elm.swallow.entry");
   curr_str = elm_object_text_get(entry);
   if (!curr_str || strlen(curr_str) < 1)
     {
        if (field_type == ELM_DATETIME_MONTH)
          *value = layout_mod->pre_decimal_month;
        else
          *value = atoi(layout_mod->pre_str_value);
     }
   else if (field_type == ELM_DATETIME_MONTH && !layout_mod->is_month_changed)
     {
        fmt = layout_mod->mod_data.field_format_get(layout_mod->mod_data.base, ELM_DATETIME_MONTH);
        if (strcmp(fmt, "%m"))
          *value = layout_mod->pre_decimal_month;
     }
}

static void
_entry_focused_cb(void *data, Evas_Object *obj, void *event_info EINA_UNUSED)
{
   Input_Spinner_Module_Data *layout_mod;
   Evas_Object *entry;
   int value;
   const char *str_value, *fmt;

   layout_mod = (Input_Spinner_Module_Data *)data;
   if (!layout_mod) return;

   entry = elm_object_part_content_get(layout_mod->field_obj[ELM_DATETIME_MONTH],
                                       "elm.swallow.entry");
   if (obj == entry)
     {
        layout_mod->is_month_changed = EINA_FALSE;

        value = (int)elm_spinner_value_get(layout_mod->field_obj[ELM_DATETIME_MONTH]);
        str_value = elm_spinner_special_value_get(layout_mod->field_obj[ELM_DATETIME_MONTH], value);
        if (str_value)
          elm_object_text_set(obj, str_value);

        layout_mod->pre_decimal_month = value;
     }

   entry = elm_object_part_content_get(layout_mod->field_obj[ELM_DATETIME_HOUR],
                                       "elm.swallow.entry");
   if (obj == entry)
     {
        value = (int)elm_spinner_value_get(layout_mod->field_obj[ELM_DATETIME_HOUR]);
        str_value = elm_spinner_special_value_get(layout_mod->field_obj[ELM_DATETIME_HOUR], value);
        if (str_value)
          elm_object_text_set(obj, str_value);
     }

   layout_mod->pre_str_value = elm_object_text_get(obj);
}

static void
_entry_unfocused_cb(void *data, Evas_Object *obj, void *event_info EINA_UNUSED)
{
   Input_Spinner_Module_Data *layout_mod;
   Evas_Object *entry;
   int value;
   char buf[BUFF_SIZE];
   const char *curr_str, *fmt;
   const int hour12_min_value = 1;

   layout_mod = (Input_Spinner_Module_Data *)data;
   if (!layout_mod) return;

   entry = elm_object_part_content_get(layout_mod->field_obj[ELM_DATETIME_MONTH],
                                       "elm.swallow.entry");
   if (obj == entry && layout_mod->is_month_changed)
     {
        fmt = layout_mod->mod_data.field_format_get(layout_mod->mod_data.base, ELM_DATETIME_MONTH);
        if (strcmp(fmt, "%m"))
          {
             value = (int)elm_spinner_value_get(layout_mod->field_obj[ELM_DATETIME_MONTH]);
             snprintf(buf, sizeof(buf), "%d", value);
             elm_object_text_set(obj, buf);
          }
     }

   entry = elm_object_part_content_get(layout_mod->field_obj[ELM_DATETIME_HOUR],
                                         "elm.swallow.entry");
   // Below code is to prevent automatic toggling of am/pm.
   if (obj == entry && layout_mod->time_12hr_fmt)
     {
        value = atoi(elm_object_text_get(obj));
        if (value != hour12_min_value)
          {
             value = (int)elm_spinner_value_get(layout_mod->field_obj[ELM_DATETIME_HOUR]);
             snprintf(buf, sizeof(buf), "%d", value);
             elm_object_text_set(obj, buf);
          }
        else
          {
             // Special handling when the hour in entry is 1
             if (layout_mod->is_pm)
               {
                  value += STRUCT_TM_TIME_12HRS_MAX_VALUE;
                  snprintf(buf, sizeof(buf), "%d", value);
                  elm_object_text_set(obj, buf);
               }
          }
     }
}

static void
_entry_activated_cb(void *data, Evas_Object *obj, void *event_info EINA_UNUSED)
{
   Input_Spinner_Module_Data *layout_mod;
   Evas_Object *entry;
   int idx;

   layout_mod = (Input_Spinner_Module_Data *)data;
   if (!layout_mod) return;

   if (layout_mod->is_timepicker)
     {
        entry = elm_object_part_content_get(layout_mod->field_obj[ELM_DATETIME_HOUR], "elm.swallow.entry");
        if (obj == entry)
          _field_toggle_to_next_location(layout_mod, layout_mod->field_obj[ELM_DATETIME_HOUR]);
     }
   else
     {
        for (idx = 0; idx <= ELM_DATETIME_DATE; idx++)
         {
            entry = elm_object_part_content_get(layout_mod->field_obj[idx], "elm.swallow.entry");
            if (obj == entry)
              {
                 if (elm_entry_input_panel_return_key_type_get(obj) == ELM_INPUT_PANEL_RETURN_KEY_TYPE_NEXT)
                   _field_toggle_to_next_location(layout_mod, layout_mod->field_obj[idx]);

                 return;
              }
         }
     }
}

static void
_spinner_entry_show_cb(void *data, Evas_Object *obj, void *event_info EINA_UNUSED)
{
   Input_Spinner_Module_Data *layout_mod;
   Elm_Datetime_Field_Type  field_type;
   Evas_Object *entry;
   static Elm_Entry_Filter_Accept_Set digits_filter_data;
   static Elm_Entry_Filter_Limit_Size limit_filter_data;
   int loc = 0;
   const char *fmt;

   layout_mod = (Input_Spinner_Module_Data *)data;
   if (!layout_mod) return;

   entry = elm_object_part_content_get(obj, "elm.swallow.entry");
   if (!entry) return;

   field_type = (Elm_Datetime_Field_Type )evas_object_data_get(obj, "_field_type");

   /*Please note that only date time format is supported not time date as per config.*/
   if (!layout_mod->field_status[field_type])
     {
        evas_object_smart_callback_add(entry, "activated", _entry_activated_cb, layout_mod);
        evas_object_smart_callback_add(entry, "focused", _entry_focused_cb, layout_mod);
        evas_object_smart_callback_add(entry, "unfocused", _entry_unfocused_cb, layout_mod);

        switch (field_type)
          {
             case ELM_DATETIME_YEAR:
               elm_entry_input_panel_layout_set(entry, ELM_INPUT_PANEL_LAYOUT_DATETIME);
               elm_entry_markup_filter_append(entry, _year_validity_checking_filter, layout_mod);
               break;

             case ELM_DATETIME_MONTH:
               digits_filter_data.accepted = "0123456789";
               digits_filter_data.rejected = NULL;
               limit_filter_data.max_char_count = 5; //Month's string(3) + (2)(max digits)
               limit_filter_data.max_byte_count = 0;
               elm_entry_markup_filter_prepend(entry, elm_entry_filter_accept_set, &digits_filter_data);
               elm_entry_markup_filter_prepend(entry, elm_entry_filter_limit_size, &limit_filter_data);
               elm_entry_markup_filter_prepend(entry, _month_validity_checking_filter, layout_mod);
               break;

             case ELM_DATETIME_DATE:
               elm_entry_input_panel_layout_set(entry, ELM_INPUT_PANEL_LAYOUT_DATETIME);
               elm_entry_markup_filter_append(entry, _date_validity_checking_filter, layout_mod);
               break;

             case ELM_DATETIME_HOUR:
               elm_entry_markup_filter_append(entry, _hour_validity_checking_filter, layout_mod);
               elm_entry_input_panel_layout_set(entry, ELM_INPUT_PANEL_LAYOUT_DATETIME);
               elm_entry_input_panel_return_key_type_set(entry, ELM_INPUT_PANEL_RETURN_KEY_TYPE_NEXT);
               break;

             case ELM_DATETIME_MINUTE:
               elm_entry_markup_filter_append(entry, _minute_validity_checking_filter, layout_mod);
               elm_entry_input_panel_layout_set(entry, ELM_INPUT_PANEL_LAYOUT_DATETIME);
               elm_entry_input_panel_return_key_type_set(entry, ELM_INPUT_PANEL_RETURN_KEY_TYPE_DONE);
               break;

             default:
               ERR("the default case is not allowed");
               break;
          }

        layout_mod->field_status[field_type] = EINA_TRUE;
     }

   layout_mod->mod_data.field_location_get(layout_mod->mod_data.base, field_type, &loc);

   switch (field_type)
     {
        case ELM_DATETIME_YEAR:
           if (loc == 2)
             elm_entry_input_panel_return_key_type_set(entry, ELM_INPUT_PANEL_RETURN_KEY_TYPE_DONE);
           else
             elm_entry_input_panel_return_key_type_set(entry, ELM_INPUT_PANEL_RETURN_KEY_TYPE_NEXT);
           break;

         case ELM_DATETIME_MONTH:
           fmt = layout_mod->mod_data.field_format_get(layout_mod->mod_data.base, field_type);
           if (strcmp(fmt, "%m"))
             elm_entry_input_panel_layout_set(entry, ELM_INPUT_PANEL_LAYOUT_MONTH);
           else
             {
                elm_entry_input_panel_layout_set(entry, ELM_INPUT_PANEL_LAYOUT_DATETIME);

                if (loc == 2)
                  elm_entry_input_panel_return_key_type_set(entry, ELM_INPUT_PANEL_RETURN_KEY_TYPE_DONE);
                else
                  elm_entry_input_panel_return_key_type_set(entry, ELM_INPUT_PANEL_RETURN_KEY_TYPE_NEXT);
             }
           break;

         case ELM_DATETIME_DATE:
           if (loc == 2)
             elm_entry_input_panel_return_key_type_set(entry, ELM_INPUT_PANEL_RETURN_KEY_TYPE_DONE);
           else
             elm_entry_input_panel_return_key_type_set(entry, ELM_INPUT_PANEL_RETURN_KEY_TYPE_NEXT);
           break;

         default:
           break;
      }
}

static void
_am_changed_cb(void *data, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Input_Spinner_Module_Data *layout_mod;
   struct tm curr_time;
   Evas_Object *am_btn, *pm_btn;

   layout_mod = (Input_Spinner_Module_Data *)data;
   if (!layout_mod) return;

   am_btn = elm_object_part_content_get(layout_mod->field_obj[ELM_DATETIME_AMPM], "am.radio");
   pm_btn = elm_object_part_content_get(layout_mod->field_obj[ELM_DATETIME_AMPM], "pm.radio");

   elm_datetime_value_get(layout_mod->mod_data.base, &curr_time);

   if (curr_time.tm_hour >= 12)
     curr_time.tm_hour -= 12;
   else
     return;

   elm_datetime_value_set(layout_mod->mod_data.base, &curr_time);
}

static void
_pm_changed_cb(void *data, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Input_Spinner_Module_Data *layout_mod;
   struct tm curr_time;
   Evas_Object *am_btn, *pm_btn;

   layout_mod = (Input_Spinner_Module_Data *)data;
   if (!layout_mod) return;

   am_btn = elm_object_part_content_get(layout_mod->field_obj[ELM_DATETIME_AMPM], "am.radio");
   pm_btn = elm_object_part_content_get(layout_mod->field_obj[ELM_DATETIME_AMPM], "pm.radio");

   elm_datetime_value_get(layout_mod->mod_data.base, &curr_time);

   if (curr_time.tm_hour < 12)
     curr_time.tm_hour += 12;
   else
     return;


   elm_datetime_value_set(layout_mod->mod_data.base, &curr_time);
}

static void
_access_set(Evas_Object *ob EINA_UNUSED, Elm_Datetime_Field_Type field_type EINA_UNUSED)
{
/*
   char *type = NULL;

   switch (field_type)
     {
        case ELM_DATETIME_YEAR:
          type = "datetime field, year";
          break;

        case ELM_DATETIME_MONTH:
          type = "datetime field, month";
          break;

        case ELM_DATETIME_DATE:
          type = "datetime field, date";
          break;

        case ELM_DATETIME_HOUR:
          type = "datetime field, hour";
          break;

        case ELM_DATETIME_MINUTE:
          type = "datetime field, minute";
          break;

        case ELM_DATETIME_AMPM:
          type = "datetime field, AM/PM";
          break;

        default:
          break;
     }

   _elm_access_text_set(_elm_access_info_get(obj), ELM_ACCESS_TYPE, type);
   _elm_access_callback_set(_elm_access_info_get(obj), ELM_ACCESS_STATE, NULL, NULL);

*/
}

static void
_set_month_special_values(Input_Spinner_Module_Data *layout_mod)
{
   struct tm curr_time;
   const char *fmt;
   int month;
   UDateFormat *dt_formatter;
   UErrorCode status = U_ZERO_ERROR;
   UChar str[32] = {0, };
   char locale[DATETIME_LOCALE_LENGTH_MAX] = {0,};
   char *locale_tmp, *p;
   UChar pattern[256] = {0, };
   time_t time_val;
   UDate date;
   char buf[32] = {0,};
   int min, max;

   if (!layout_mod) return;

   elm_datetime_value_get(layout_mod->mod_data.base, &curr_time);
   fmt = layout_mod->mod_data.field_format_get(layout_mod->mod_data.base, ELM_DATETIME_MONTH);
   layout_mod->mod_data.field_limit_get(layout_mod->mod_data.base, ELM_DATETIME_MONTH, &min, &max);

   if (strlen(layout_mod->user_locale) == 0)
     locale_tmp = setlocale(LC_MESSAGES, NULL);
   else
     locale_tmp = layout_mod->user_locale;

   strcpy(locale, locale_tmp);
   if (locale[0] != '\0')
     {
        p = strstr(locale, ".UTF-8");
        if (p) *p = 0;
     }

   if (!strcmp(fmt, "%B"))
     u_uastrcpy(pattern, "MMMM");
   else if (!strcmp(fmt, "%m"))
     u_uastrcpy(pattern, "MM");
   else
     u_uastrcpy(pattern, "MMM");

   curr_time.tm_mday = 1;
   for (month = min; month <= max; month++)
     {
         curr_time.tm_mon = month;
         curr_time.tm_isdst = -1;
         time_val = (time_t)mktime(&curr_time);
         date = (UDate)time_val * 1000;

         dt_formatter = udat_open(UDAT_IGNORE, UDAT_IGNORE, locale, NULL, -1, pattern, -1, &status);
         udat_format(dt_formatter, date, str, sizeof(str), NULL, &status);
         u_austrcpy(buf, str);
         udat_close(dt_formatter);
         elm_spinner_special_value_add(layout_mod->field_obj[ELM_DATETIME_MONTH],
                                       month + 1, buf);
     }
}

static void
_set_hour_special_values(Input_Spinner_Module_Data *layout_mod)
{
   struct tm tim = {0};
   const char *fmt;
   int hour;
   char buf[32] = {0,};
   int min, max;

   if (!layout_mod) return;

   fmt = layout_mod->mod_data.field_format_get(layout_mod->mod_data.base, ELM_DATETIME_HOUR);
   layout_mod->mod_data.field_limit_get(layout_mod->mod_data.base, ELM_DATETIME_HOUR, &min, &max);

   for (hour = min; hour <= max; hour++)
     {
        tim.tm_hour = hour;
        strftime(buf, sizeof(buf), fmt, &tim);
        elm_spinner_special_value_add(layout_mod->field_obj[ELM_DATETIME_HOUR], hour, buf);
     }
}

static void
_set_am_pm_value(Input_Spinner_Module_Data *layout_mod)
{
   Evas_Object *widget, *am_btn, *pm_btn;
   struct tm curr_time;
   UDateFormat *dt_formatter;
   UErrorCode status = U_ZERO_ERROR;
   UChar str[32] = {0, };
   char locale[32] = {0,};
   char *locale_tmp, *p;
   UChar pattern[12] = {0, };
   time_t time_val;
   UDate date;
   char buf[BUFF_SIZE] = {0};
   const char *style;

   if (!layout_mod) return;

   if (strlen(layout_mod->user_locale) == 0)
     locale_tmp = setlocale(LC_MESSAGES, NULL);
   else
     locale_tmp = layout_mod->user_locale;

   strcpy(locale, locale_tmp);
   if (locale[0] != '\0')
    {
       p = strstr(locale, ".UTF-8");
       if (p) *p = 0;
    }

   widget = layout_mod->mod_data.base;
   style = elm_object_style_get(widget);

   elm_datetime_value_get(layout_mod->mod_data.base, &curr_time);
   curr_time.tm_isdst = -1;
   curr_time.tm_hour = 6;
   time_val = (time_t)mktime(&curr_time);
   date = (UDate)time_val * 1000;
   u_uastrcpy(pattern, "a");
   dt_formatter = udat_open(UDAT_IGNORE, UDAT_IGNORE, locale, NULL, -1, pattern, -1, &status);
   udat_format(dt_formatter, date, str, sizeof(str), NULL, &status);
   u_austrcpy(buf, str);
   udat_close(dt_formatter);

   am_btn = elm_object_part_content_get(layout_mod->field_obj[ELM_DATETIME_AMPM], "am.radio");
   if (!am_btn) return;

   elm_object_text_set(am_btn, buf);

   curr_time.tm_isdst = -1;
   curr_time.tm_hour = 18;
   time_val = (time_t)mktime(&curr_time);
   date = (UDate)time_val * 1000;
   u_uastrcpy(pattern, "a");
   dt_formatter = udat_open(UDAT_IGNORE, UDAT_IGNORE, locale, NULL, -1, pattern, -1, &status);
   udat_format(dt_formatter, date, str, sizeof(str), NULL, &status);
   u_austrcpy(buf, str);
   udat_close(dt_formatter);

   pm_btn = elm_object_part_content_get(layout_mod->field_obj[ELM_DATETIME_AMPM], "pm.radio");
   if (!pm_btn) return;

   elm_object_text_set(pm_btn, buf);

}

static void
_locale_changed_cb(void *data, Evas_Object *obj, void *event_info)
{
   Input_Spinner_Module_Data *layout_mod;
   char *locale = NULL;

   layout_mod = (Input_Spinner_Module_Data *)data;
   if (!layout_mod) return;

   locale = event_info;
   strcpy(layout_mod->user_locale, locale);

   _set_month_special_values(layout_mod);
   _set_am_pm_value(layout_mod);
}

// module fucns for the specific module type
EAPI void
field_format_changed(Elm_Datetime_Module_Data *module_data, Evas_Object *obj)
{
   Input_Spinner_Module_Data *layout_mod;
   Elm_Datetime_Field_Type  field_type;
   Evas_Object *widget;
   Evas_Object *entry;
   int loc = 0;
   char buf[BUFF_SIZE] = {0};
   const char *fmt;

   layout_mod = (Input_Spinner_Module_Data *)module_data;
   if (!layout_mod || !obj) return;

   if (!layout_mod->is_init) layout_mod->is_init = EINA_TRUE;

   widget = layout_mod->mod_data.base;
   field_type = (Elm_Datetime_Field_Type )evas_object_data_get(obj, "_field_type");

   switch (field_type)
     {
        case ELM_DATETIME_MONTH:
          _set_month_special_values(layout_mod);

          fmt = layout_mod->mod_data.field_format_get(layout_mod->mod_data.base, field_type);
          layout_mod->mod_data.field_location_get(widget, ELM_DATETIME_AMPM, &loc);
          snprintf(buf, sizeof(buf), "field%d", loc);

          entry = elm_object_part_content_get(layout_mod->field_obj[ELM_DATETIME_MONTH], buf);
          if (entry)
            {
               if (strcmp(fmt, "%m"))
                 elm_entry_input_panel_layout_set(entry, ELM_INPUT_PANEL_LAYOUT_MONTH);
               else
                 {
                    elm_entry_input_panel_layout_set(entry, ELM_INPUT_PANEL_LAYOUT_DATETIME);
                    if (loc == 2)
                      elm_entry_input_panel_return_key_type_set(entry, ELM_INPUT_PANEL_RETURN_KEY_TYPE_DONE);
                    else
                      elm_entry_input_panel_return_key_type_set(entry, ELM_INPUT_PANEL_RETURN_KEY_TYPE_NEXT);
                 }
            }
          break;

        case ELM_DATETIME_HOUR:
          _set_hour_special_values(layout_mod);
          break;

        case ELM_DATETIME_AMPM:
          layout_mod->mod_data.field_location_get(widget, ELM_DATETIME_AMPM, &loc);
          if (loc == 5)
            {
               elm_object_signal_emit(widget, "state,colon,visible,field3", "elm");
               elm_object_signal_emit(widget, "state,colon,invisible,field4", "elm");
            }
          else
            {
               elm_object_signal_emit(widget, "state,colon,visible,field4", "elm");
               elm_object_signal_emit(widget, "state,colon,invisible,field3", "elm");
            }

          edje_object_message_signal_process(elm_layout_edje_get(widget));
          break;

        default:
          break;
     }
}

EAPI void
field_value_display(Elm_Datetime_Module_Data *module_data, Evas_Object *obj)
{
   Input_Spinner_Module_Data *layout_mod;
   Elm_Datetime_Field_Type  field_type;
   Evas_Object *widget, *am_btn, *pm_btn;
   int min, max;
   struct tm tim;
   char buf[BUFF_SIZE] = {0};
   const char *fmt;

   layout_mod = (Input_Spinner_Module_Data *)module_data;
   if (!layout_mod || !obj) return;

   widget = layout_mod->mod_data.base;
   elm_datetime_value_get(widget, &tim);

   field_type = (Elm_Datetime_Field_Type )evas_object_data_get(obj, "_field_type");
   fmt = layout_mod->mod_data.field_format_get(widget, field_type);

   switch (field_type)
     {
        case ELM_DATETIME_YEAR:
          layout_mod->mod_data.field_limit_get(widget, field_type, &min, &max);
          elm_spinner_min_max_set(obj, 1900 + min, 1900 + max);
          elm_spinner_value_set(obj, 1900 + tim.tm_year);
          break;

        case ELM_DATETIME_MONTH:
          layout_mod->mod_data.field_limit_get(widget, field_type, &min, &max);
          elm_spinner_min_max_set(obj, 1 + min, 1 + max);
          elm_spinner_value_set(obj, 1 + tim.tm_mon);
          break;

        case ELM_DATETIME_HOUR:
          layout_mod->mod_data.field_limit_get(widget, field_type, &min, &max);
          elm_spinner_min_max_set(obj, min, max);
          elm_spinner_value_set(obj, tim.tm_hour);
          break;

        case ELM_DATETIME_AMPM:
          if ((tim.tm_hour >= 0) && (tim.tm_hour < 12))
            layout_mod->is_pm = EINA_FALSE;
          else
            layout_mod->is_pm = EINA_TRUE;

          _set_am_pm_value(layout_mod);

          if (layout_mod->is_pm)
            {
               pm_btn = elm_object_part_content_get(layout_mod->field_obj[ELM_DATETIME_AMPM], "pm.radio");
               if (!pm_btn) return;

               elm_radio_value_set(pm_btn, 1);
            }
          else
            {
               am_btn = elm_object_part_content_get(layout_mod->field_obj[ELM_DATETIME_AMPM], "am.radio");
               if (!am_btn) return;

               elm_radio_value_set(am_btn, 0);
            }

          break;

        case ELM_DATETIME_DATE:
        case ELM_DATETIME_MINUTE:
          strftime(buf, sizeof(buf), fmt, &tim);
          layout_mod->mod_data.field_limit_get(widget, field_type, &min, &max);
          elm_spinner_min_max_set(obj, min, max);
          elm_spinner_value_set(obj, atoi(buf));
          break;

        default:
          ERR("the default case is not allowed");
          break;
     }
}

EAPI Evas_Object *
field_create(Elm_Datetime_Module_Data *module_data, Elm_Datetime_Field_Type  field_type)
{
   Input_Spinner_Module_Data *layout_mod;
   Evas_Object *field_obj;

   layout_mod = (Input_Spinner_Module_Data *)module_data;
   if (!layout_mod) return NULL;

   if (field_type == ELM_DATETIME_AMPM)
     field_obj = elm_layout_add(layout_mod->mod_data.base);
   else
     {
       field_obj = elm_spinner_add(layout_mod->mod_data.base);

       if (field_type < ELM_DATETIME_HOUR)
         elm_object_style_set(field_obj, "date_picker");

       elm_spinner_editable_set(field_obj, EINA_TRUE);
       elm_spinner_step_set(field_obj, 1);
       elm_spinner_label_format_set(field_obj, "%02.0f");

       switch (field_type)
         {
            case ELM_DATETIME_YEAR:
              elm_spinner_wrap_set(field_obj, EINA_FALSE);
              elm_spinner_interval_set(field_obj, 0.1);
              break;

            case ELM_DATETIME_MONTH:
            case ELM_DATETIME_HOUR:
              elm_spinner_wrap_set(field_obj, EINA_TRUE);
              elm_spinner_interval_set(field_obj, 0.2);
              break;

            case ELM_DATETIME_DATE:
            case ELM_DATETIME_MINUTE:
              elm_spinner_wrap_set(field_obj, EINA_TRUE);
              elm_spinner_interval_set(field_obj, 0.1);
              break;

            default:
              ERR("the default case is not allowed");
              break;
         }

       evas_object_smart_callback_add(field_obj, "changed", _spinner_changed_cb, layout_mod);
       evas_object_smart_callback_add(field_obj, "entry,changed", _spinner_entry_changed_cb, layout_mod);
       evas_object_smart_callback_add(field_obj, "entry,show", _spinner_entry_show_cb, layout_mod);
       evas_object_smart_callback_add(field_obj, "entry,apply", _spinner_entry_apply_cb, layout_mod);
     }

   layout_mod->field_obj[field_type] = field_obj;
   evas_object_data_set(field_obj, "_field_type", (void *)field_type);

   _access_set(field_obj, field_type);

   return field_obj;
}

EAPI Elm_Datetime_Module_Data *
obj_hook(Evas_Object *obj)
{
   Input_Spinner_Module_Data *layout_mod;
   int idx;

   layout_mod = ELM_NEW(Input_Spinner_Module_Data);
   if (!layout_mod) return NULL;

   layout_mod->am = NULL;
   layout_mod->pm = NULL;
   layout_mod->ampm = NULL;

   for (idx = 0; idx < DATETIME_FIELD_COUNT; idx++)
     layout_mod->field_status[idx] = EINA_FALSE;

   layout_mod->time_12hr_fmt = EINA_FALSE;
   layout_mod->is_pm = EINA_FALSE;
   layout_mod->is_timepicker = EINA_FALSE;
   layout_mod->is_init = EINA_FALSE;
   layout_mod->is_month_changed = EINA_FALSE;
   layout_mod->pre_decimal_month = 0;
   layout_mod->pre_str_value = NULL;
   memset(layout_mod->user_locale, 0, DATETIME_LOCALE_LENGTH_MAX);

   evas_object_smart_callback_add(obj, "theme,changed", _widget_theme_changed_cb, layout_mod);
   evas_object_smart_callback_add(obj, "locale,changed", _locale_changed_cb, layout_mod);

   return (Elm_Datetime_Module_Data*)layout_mod;
}

EAPI void
obj_unhook(Elm_Datetime_Module_Data *module_data)
{
   Input_Spinner_Module_Data *layout_mod;

   layout_mod = (Input_Spinner_Module_Data *)module_data;
   if (!layout_mod) return;

   if (layout_mod->am) evas_object_del(layout_mod->am);
   if (layout_mod->pm) evas_object_del(layout_mod->pm);
   if (layout_mod->ampm) evas_object_del(layout_mod->ampm);

   free(layout_mod);
   layout_mod = NULL;
}

EAPI void
obj_hide(Elm_Datetime_Module_Data *module_data EINA_UNUSED)
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
