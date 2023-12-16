#include <Elementary.h>
#include "elm_priv.h"
#include <unicode/udat.h>
#include <unicode/ustring.h>

#define DATETIME_FIELD_COUNT            6
#define BUFF_SIZE                       1024
#define MONTH_STRING_MAX_SIZE           32
#define TOTAL_NUMBER_OF_MONTHS          12
#define STRUCT_TM_YEAR_BASE_VALUE       1900
#define STRUCT_TM_TIME_12HRS_MAX_VALUE  12
#define STRUCT_TM_TIME_24HRS_MAX_VALUE  23
#define PICKER_layout_field_COUNT        3

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
/* remove warning for not use method (2014 / 10 / 17 )
static const char *field_styles[] = {
                         "year", "month", "date", "hour", "minute", "ampm" };
*/
static char month_arr[TOTAL_NUMBER_OF_MONTHS][MONTH_STRING_MAX_SIZE];

static const char SIG_DATE_PICKER_BTN_CLICKED[] = "date,btn,clicked";
static const char SIG_TIME_PICKER_BTN_CLICKED[] = "time,btn,clicked";
static const char SIG_PICKER_VALUE_SET[] = "picker,value,set";

typedef struct _Layout_Module_Data Layout_Module_Data;

struct _Layout_Module_Data
{
   Elm_Datetime_Module_Data mod_data;
   Evas_Object *date_btn, *time_btn;
   Evas_Object *datepicker_layout, *timepicker_layout;
   Evas_Object *layout_field[DATETIME_FIELD_COUNT];
   struct tm set_time;
   char weekday[BUFF_SIZE];
   int field_location[PICKER_layout_field_COUNT];
   Eina_Bool time_12hr_fmt;
   Eina_Bool is_pm;
   Eina_Bool weekday_show;
   Eina_Bool weekday_loc_first;
   Eina_Bool dateBtn_is_visible;
   Eina_Bool timeBtn_is_visible;
};


static int
_picker_nextfield_location_get(void *data, int curr)
{
   int idx, next_idx;
   Layout_Module_Data *layout_mod;
   layout_mod = (Layout_Module_Data *)data;
   if (!layout_mod) return ELM_DATETIME_LAST;

   for (idx = 0; idx <= ELM_DATETIME_DATE; idx++)
     if (layout_mod->field_location[curr] == idx) break;

   if (idx >= ELM_DATETIME_DATE) return ELM_DATETIME_LAST;

   for (next_idx = 0; next_idx <= ELM_DATETIME_DATE; next_idx++)
     if (layout_mod->field_location[next_idx] == idx+1) return next_idx;

   return ELM_DATETIME_LAST;
}

static void
_set_datepicker_value(Layout_Module_Data *layout_mod)
{
   struct tm set_time;

   if (!layout_mod) return;

   elm_datetime_value_get(layout_mod->mod_data.base, &set_time);
   set_time.tm_year = (int)elm_spinner_value_get(layout_mod->layout_field[ELM_DATETIME_YEAR]) - STRUCT_TM_YEAR_BASE_VALUE;
   set_time.tm_mon = (int)elm_spinner_value_get(layout_mod->layout_field[ELM_DATETIME_MONTH]) - 1;
   set_time.tm_mday = (int)elm_spinner_value_get(layout_mod->layout_field[ELM_DATETIME_DATE]);
   elm_datetime_value_set(layout_mod->mod_data.base, &set_time);
}

static void
_set_timepicker_value(Layout_Module_Data *layout_mod)
{
   struct tm set_time;

   if (!layout_mod) return;

   elm_datetime_value_get(layout_mod->mod_data.base, &set_time);

   set_time.tm_hour = (int)elm_spinner_value_get(layout_mod->layout_field[ELM_DATETIME_HOUR]);
   set_time.tm_min = (int)elm_spinner_value_get(layout_mod->layout_field[ELM_DATETIME_MINUTE]);
   elm_datetime_value_set(layout_mod->mod_data.base, &set_time);
}

static void
_layout_picker_set_cb(void *data,
                      Evas_Object *obj EINA_UNUSED,
                      const char *emission EINA_UNUSED,
                      const char *source EINA_UNUSED)
{
   Layout_Module_Data *layout_mod;

   layout_mod = (Layout_Module_Data *)data;
   if (!layout_mod) return;

   if (layout_mod->datepicker_layout)
     _set_datepicker_value(layout_mod);
   if (layout_mod->timepicker_layout)
     _set_timepicker_value(layout_mod);
   evas_object_smart_callback_call(layout_mod->mod_data.base, SIG_PICKER_VALUE_SET, NULL);
}

static void
_entry_activated_cb(void *data, Evas_Object *obj, void *event_info EINA_UNUSED)
{
   Layout_Module_Data *layout_mod;
   Evas_Object *entry;
   int idx, next_idx;

   layout_mod = (Layout_Module_Data *)data;
   if (!layout_mod) return;

   for (idx = 0; idx <= ELM_DATETIME_DATE; idx++)
     {
        entry = elm_object_part_content_get(layout_mod->layout_field[idx], "elm.swallow.entry");
        if (obj == entry)
          {
             next_idx = _picker_nextfield_location_get(layout_mod, idx);
             if (next_idx != ELM_DATETIME_LAST)
               elm_layout_signal_emit(layout_mod->layout_field[next_idx], "elm,action,entry,toggle", "elm");
             return;
          }
     }

   entry = elm_object_part_content_get(layout_mod->layout_field[ELM_DATETIME_HOUR], "elm.swallow.entry");
   if (obj == entry)
      elm_layout_signal_emit(layout_mod->layout_field[ELM_DATETIME_MINUTE], "elm,action,entry,toggle", "elm");
}

static void
_entry_focused_cb(void *data, Evas_Object *obj, void *event_info EINA_UNUSED)
{
   Layout_Module_Data *layout_mod;
   Evas_Object *entry;
   int value;
   const char *hour_value = NULL;

   layout_mod = (Layout_Module_Data *)data;
   if (!layout_mod) return;
   elm_entry_cursor_handler_disabled_set(obj, EINA_TRUE);

   entry = elm_object_part_content_get(layout_mod->layout_field[ELM_DATETIME_MONTH],
                                       "elm.swallow.entry");
   if (obj == entry)
     {
        value = (int)elm_spinner_value_get(layout_mod->layout_field[ELM_DATETIME_MONTH]) - 1;
        elm_object_text_set(obj, month_arr[value]);
     }
   entry = elm_object_part_content_get(layout_mod->layout_field[ELM_DATETIME_HOUR],
                                       "elm.swallow.entry");
   if (obj == entry)
     {
        value = elm_spinner_value_get(layout_mod->layout_field[ELM_DATETIME_HOUR]);
        hour_value = elm_spinner_special_value_get(layout_mod->layout_field[ELM_DATETIME_HOUR], value);
        if ((layout_mod->time_12hr_fmt) && hour_value)
          elm_object_text_set(obj, hour_value);
     }
}

static void
_set_ampm_value(Layout_Module_Data *layout_mod)
{
   if (!layout_mod || !layout_mod->time_12hr_fmt) return;

   if (layout_mod->set_time.tm_hour < STRUCT_TM_TIME_12HRS_MAX_VALUE)
     elm_object_domain_translatable_text_set(layout_mod->layout_field[ELM_DATETIME_AMPM],
                                             PACKAGE, E_("IDS_COM_BODY_AM"));
   else
     elm_object_domain_translatable_text_set(layout_mod->layout_field[ELM_DATETIME_AMPM],
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
_datepicker_value_changed_cb(void *data, Evas_Object *obj, void *event_info EINA_UNUSED)
{
   Layout_Module_Data *layout_mod;
   struct tm min_values, max_values;
   int idx, field_idx, min, max;

   layout_mod = (Layout_Module_Data *)data;
   if (!layout_mod) return;

   for (idx = 0; idx < DATETIME_FIELD_COUNT; idx++)
     if ((obj == layout_mod->layout_field[idx])) break;

   if (idx > ELM_DATETIME_DATE) return;

   field_idx = idx;
   DATETIME_MODULE_TM_ARRAY(set_val_arr, &layout_mod->set_time);
   if (field_idx == ELM_DATETIME_YEAR)
     *set_val_arr[field_idx] = (int)elm_spinner_value_get(obj) - STRUCT_TM_YEAR_BASE_VALUE;
   else if (field_idx == ELM_DATETIME_MONTH)
     *set_val_arr[field_idx] = (int)elm_spinner_value_get(obj) - 1;
   else
     *set_val_arr[field_idx] = (int)elm_spinner_value_get(obj);

   layout_mod->mod_data.fields_min_max_get(layout_mod->mod_data.base,
                       &(layout_mod->set_time), &min_values, &max_values);

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
        elm_spinner_min_max_set(layout_mod->layout_field[idx], min, max);
     }
   for (idx = field_idx; idx <= ELM_DATETIME_DATE; idx++)
     {
        if (idx == ELM_DATETIME_YEAR)
          *set_val_arr[idx] = (int)elm_spinner_value_get(layout_mod->layout_field[idx]) - STRUCT_TM_YEAR_BASE_VALUE;
        else if (idx == ELM_DATETIME_MONTH)
          *set_val_arr[idx] = (int)elm_spinner_value_get(layout_mod->layout_field[idx]) - 1;
        else
          *set_val_arr[idx] = (int)elm_spinner_value_get(layout_mod->layout_field[idx]);
     }
}

static void
_timepicker_value_changed_cb(void *data, Evas_Object *obj, void *event_info EINA_UNUSED)
{
   Layout_Module_Data *layout_mod;
   struct tm min_values, max_values;

   layout_mod = (Layout_Module_Data *)data;
   if (!layout_mod) return;

   if (obj == layout_mod->layout_field[ELM_DATETIME_HOUR])
     {
        (layout_mod->set_time).tm_hour = (int)elm_spinner_value_get(obj);;
        layout_mod->mod_data.fields_min_max_get(layout_mod->mod_data.base,
                            &(layout_mod->set_time), &min_values, &max_values);
        elm_spinner_min_max_set(layout_mod->layout_field[ELM_DATETIME_MINUTE],
                                min_values.tm_min, max_values.tm_min);
        (layout_mod->set_time).tm_min = (int)elm_spinner_value_get(
                                 layout_mod->layout_field[ELM_DATETIME_MINUTE]);

        layout_mod->is_pm = ((layout_mod->set_time).tm_hour < STRUCT_TM_TIME_12HRS_MAX_VALUE) ? 0 : 1;
        _set_ampm_value(layout_mod);
     }
   else if (obj == layout_mod->layout_field[ELM_DATETIME_MINUTE])
     (layout_mod->set_time).tm_min = (int)elm_spinner_value_get(obj);

}

static void
_ampm_clicked_cb(void *data, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Layout_Module_Data *layout_mod;
   double hour, hour_min, hour_max;

   layout_mod = (Layout_Module_Data *)data;
   if (!layout_mod || !layout_mod->time_12hr_fmt) return;

   hour = (int)elm_spinner_value_get(layout_mod->layout_field[ELM_DATETIME_HOUR]);

   if (hour < STRUCT_TM_TIME_12HRS_MAX_VALUE)
     hour += STRUCT_TM_TIME_12HRS_MAX_VALUE;
   else
     hour -= STRUCT_TM_TIME_12HRS_MAX_VALUE;

   elm_spinner_min_max_get(layout_mod->layout_field[ELM_DATETIME_HOUR], &hour_min, &hour_max);
   if ((hour >= hour_min) && (hour <= hour_max))
     {
        (layout_mod->set_time).tm_hour = hour;
        elm_spinner_value_set(layout_mod->layout_field[ELM_DATETIME_HOUR], hour);
        layout_mod->is_pm = (hour < STRUCT_TM_TIME_12HRS_MAX_VALUE) ? 0 : 1;
        _set_ampm_value(layout_mod);
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
   Layout_Module_Data *layout_mod;
   Evas_Object *entry;
   char *new_str = NULL;
   int min, max, val = 0, len;
   char *insert;
   const char *curr_str;
   int next_idx = 0;

   layout_mod = (Layout_Module_Data *)data;
   if (!layout_mod) return;

   if (!(*text) || !strcmp(*text, "")) return;

   insert = *text;
   len = strlen(elm_object_text_get(obj));
   if (len < 3) return;

   curr_str = elm_object_text_get(obj);
   if (curr_str) new_str = _text_insert(curr_str, insert, elm_entry_cursor_pos_get(obj));
   if (new_str) val = atoi(new_str);

   layout_mod->mod_data.field_limit_get(layout_mod->mod_data.base, ELM_DATETIME_YEAR, &min, &max);
   min += STRUCT_TM_YEAR_BASE_VALUE;
   max += STRUCT_TM_YEAR_BASE_VALUE;

   if (val <= max)
     {
        elm_entry_entry_set(obj, new_str);
        elm_entry_cursor_end_set(obj);
        next_idx = _picker_nextfield_location_get(layout_mod, ELM_DATETIME_YEAR);
        if (next_idx != ELM_DATETIME_LAST)
          {
             elm_layout_signal_emit(layout_mod->layout_field[ELM_DATETIME_YEAR],
                                    "elm,action,entry,toggle", "elm");
             entry = elm_object_part_content_get(layout_mod->layout_field[next_idx],
                                                "elm.swallow.entry");
             if (!elm_object_focus_get(entry))
               elm_layout_signal_emit(layout_mod->layout_field[next_idx],
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
   Layout_Module_Data *layout_mod;
   Evas_Object *entry;
   char *new_str = NULL;
   double min, max;
   int val = 0;
   char *insert;
   const char *curr_str;
   int next_idx = 0;

   layout_mod = (Layout_Module_Data *)data;
   if (!layout_mod) return;

   if (!(*text) || !strcmp(*text, "")) return;

   insert = *text;
   elm_entry_entry_set(obj, "");
   curr_str = elm_object_text_get(obj);
   if (curr_str) new_str = _text_insert(curr_str, insert, elm_entry_cursor_pos_get(obj));
   if (new_str) val = atoi(new_str) - 1;

   elm_spinner_min_max_get(layout_mod->layout_field[ELM_DATETIME_MONTH], &min, &max);
   min -= 1;
   max -= 1;

   if ((val >= min) && (val <= max))
     {
        elm_entry_entry_set(obj, new_str);
        elm_entry_cursor_end_set(obj);
        next_idx = _picker_nextfield_location_get(layout_mod, ELM_DATETIME_MONTH);
        if (next_idx != DATETIME_FIELD_COUNT)
          {
             elm_layout_signal_emit(layout_mod->layout_field[ELM_DATETIME_MONTH],
                                    "elm,action,entry,toggle", "elm");
             entry = elm_object_part_content_get(layout_mod->layout_field[next_idx],
                                                 "elm.swallow.entry");
             if (!elm_object_focus_get(entry))
               elm_layout_signal_emit(layout_mod->layout_field[next_idx],
                                      "elm,action,entry,toggle", "elm");
          }
     }
   *insert = 0;
   free((void *)new_str);
   new_str = NULL;
}

static void
_hour_validity_check(void *data, Evas_Object *obj, void *event_info)
{
    Layout_Module_Data *layout_mod;

    layout_mod = (Layout_Module_Data *)data;
    if (!layout_mod) return;

    Evas_Object* entry = elm_object_part_content_get(layout_mod->layout_field[ELM_DATETIME_HOUR], "elm.swallow.entry");

    if (layout_mod->layout_field[ELM_DATETIME_HOUR] != obj || !entry)
      return;
    double* val = (double *)event_info;

    if (layout_mod->time_12hr_fmt)
      {
         if ((layout_mod->is_pm) && (*val < STRUCT_TM_TIME_12HRS_MAX_VALUE))
           *val += STRUCT_TM_TIME_12HRS_MAX_VALUE;
         if (!(layout_mod->is_pm) && (*val == STRUCT_TM_TIME_12HRS_MAX_VALUE))
           *val = 0;
      }
}

static void
_hour_validity_checking_filter(void *data, Evas_Object *obj, char **text)
{
   Layout_Module_Data *layout_mod;
   Evas_Object *entry;
   char *new_str = NULL;
   int val = 0, len;
   char *insert;
   const char *curr_str;
   double min, max;

   layout_mod = (Layout_Module_Data *)data;
   if (!layout_mod) return;

   if (!(*text) || !strcmp(*text, "")) return;

   insert = *text;
   len = strlen(elm_object_text_get(obj));
   if (len < 1) return;

   curr_str = elm_object_text_get(obj);
   if (curr_str) new_str = _text_insert(curr_str, insert, elm_entry_cursor_pos_get(obj));
   if (new_str) val = atoi(new_str);

   if (layout_mod->time_12hr_fmt)
     {
        if (val > STRUCT_TM_TIME_12HRS_MAX_VALUE)
          {
             *insert = 0;
             free((void *)new_str);
             new_str = NULL;
             return;
          }

        if (val == 0)
          {
             sprintf(new_str, "%d", STRUCT_TM_TIME_12HRS_MAX_VALUE);
             val = STRUCT_TM_TIME_12HRS_MAX_VALUE;
          }

        if ((layout_mod->is_pm) && (val != STRUCT_TM_TIME_12HRS_MAX_VALUE))
          val += STRUCT_TM_TIME_12HRS_MAX_VALUE;
     }

   elm_spinner_min_max_get(layout_mod->layout_field[ELM_DATETIME_HOUR], &min, &max);
   if ((val >= min) && (val <= max))
     {
       elm_entry_entry_set(obj, new_str);
       elm_entry_cursor_end_set(obj);
       elm_layout_signal_emit(layout_mod->layout_field[ELM_DATETIME_HOUR],
                              "elm,action,entry,toggle", "elm");
       edje_object_message_signal_process(elm_layout_edje_get(layout_mod->layout_field[ELM_DATETIME_HOUR]));
       elm_spinner_value_set(layout_mod->layout_field[ELM_DATETIME_HOUR], val);

       entry = elm_object_part_content_get(layout_mod->layout_field[ELM_DATETIME_MINUTE],
                                                     "elm.swallow.entry");
       if (!elm_object_focus_get(entry))
         {
            elm_layout_signal_emit(layout_mod->layout_field[ELM_DATETIME_MINUTE],
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
   Layout_Module_Data *layout_mod;
   Evas_Object *entry;
   char *new_str = NULL;
   int val = 0, len;
   const char *curr_str;
   char *insert;
   double min, max;
   int next_idx = 0;

   layout_mod = (Layout_Module_Data *)data;
   if (!layout_mod) return;

   if (!(*text) || !strcmp(*text, "")) return;

   insert = *text;
   len = strlen(elm_object_text_get(obj));
   if (len < 1) return;

   curr_str = elm_object_text_get(obj);
   if (curr_str) new_str = _text_insert(curr_str, insert, elm_entry_cursor_pos_get(obj));
   if (new_str) val = atoi(new_str);
   elm_spinner_min_max_get(layout_mod->layout_field[ELM_DATETIME_DATE], &min, &max);

   if ((val >= min) && (val <= max))
     {
       elm_entry_entry_set(obj, new_str);
       elm_entry_cursor_end_set(obj);
       next_idx = _picker_nextfield_location_get(layout_mod, ELM_DATETIME_DATE);
       if (next_idx != ELM_DATETIME_LAST)
         {
            elm_layout_signal_emit(layout_mod->layout_field[ELM_DATETIME_DATE],
                                   "elm,action,entry,toggle", "elm");
            entry = elm_object_part_content_get(layout_mod->layout_field[next_idx],
                                                "elm.swallow.entry");
            if (!elm_object_focus_get(entry))
              elm_layout_signal_emit(layout_mod->layout_field[next_idx],
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
   Layout_Module_Data *layout_mod;
   char *new_str = NULL;
   int val = 0, len;
   double min, max;
   char *insert;
   const char *curr_str;

   layout_mod = (Layout_Module_Data *)data;
   if (!layout_mod) return;

   if (!(*text) || !strcmp(*text, "")) return;

   insert = *text;
   len = strlen(elm_object_text_get(obj));
   if (len < 1) return;

   curr_str = elm_object_text_get(obj);
   if (curr_str) new_str = _text_insert(curr_str, insert, elm_entry_cursor_pos_get(obj));
   if (new_str) val = atoi(new_str);

   elm_spinner_min_max_get(layout_mod->layout_field[ELM_DATETIME_MINUTE], &min, &max);

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
_set_datepicker_entry_filter(Layout_Module_Data *layout_mod)
{
   Evas_Object *spinner, *entry;
   static Elm_Entry_Filter_Accept_Set digits_filter_data;
   static Elm_Entry_Filter_Limit_Size limit_filter_data;
   int idx, value;

   if (!layout_mod) return;

   digits_filter_data.accepted = "0123456789";
   digits_filter_data.rejected = NULL;

   limit_filter_data.max_byte_count = 0;

   for (idx = 0; idx <= ELM_DATETIME_DATE; idx++)
     {
       spinner = layout_mod->layout_field[idx];
       entry = elm_object_part_content_get(spinner, "elm.swallow.entry");
       if (!entry) continue;

       elm_entry_magnifier_disabled_set(entry, EINA_TRUE);
       elm_entry_context_menu_disabled_set(entry, EINA_TRUE);

       if (idx == ELM_DATETIME_MONTH)
         {
            value = (int)elm_spinner_value_get(layout_mod->layout_field[ELM_DATETIME_MONTH]) - 1;
            elm_object_text_set(entry, month_arr[value]);
         }

       elm_entry_markup_filter_append(entry, elm_entry_filter_accept_set, &digits_filter_data);

       if (idx == ELM_DATETIME_YEAR)
         limit_filter_data.max_char_count = 4;
       else if (idx == ELM_DATETIME_MONTH)
         limit_filter_data.max_char_count = 5; //Month's string(3) + (2)(max digits)
       else
         limit_filter_data.max_char_count = 2;

       elm_entry_markup_filter_append(entry, elm_entry_filter_limit_size, &limit_filter_data);

       evas_object_smart_callback_add(entry, "focused", _entry_focused_cb, layout_mod);
       evas_object_smart_callback_add(entry, "activated", _entry_activated_cb, layout_mod);

       if (idx == ELM_DATETIME_YEAR)
         elm_entry_markup_filter_append(entry, _year_validity_checking_filter, layout_mod);
       else if (idx == ELM_DATETIME_MONTH)
         elm_entry_markup_filter_append(entry, _month_validity_checking_filter, layout_mod);
       else if (idx == ELM_DATETIME_DATE)
         elm_entry_markup_filter_append(entry, _date_validity_checking_filter, layout_mod);
     }
}

static void
_set_timepicker_entry_filter(Layout_Module_Data *layout_mod)
{
   Evas_Object *spinner, *entry;
   static Elm_Entry_Filter_Accept_Set digits_filter_data;
   static Elm_Entry_Filter_Limit_Size limit_filter_data;
   int idx;

   if (!layout_mod) return;

   digits_filter_data.accepted = "0123456789";
   digits_filter_data.rejected = NULL;

   limit_filter_data.max_byte_count = 0;

   for (idx = ELM_DATETIME_HOUR; idx < ELM_DATETIME_AMPM; idx++)
     {
       spinner = layout_mod->layout_field[idx];
       entry = elm_object_part_content_get(spinner, "elm.swallow.entry");
       if (!entry) continue;

       elm_entry_magnifier_disabled_set(entry, EINA_TRUE);
       elm_entry_context_menu_disabled_set(entry, EINA_TRUE);
       elm_entry_markup_filter_append(entry, elm_entry_filter_accept_set, &digits_filter_data);

       limit_filter_data.max_char_count = 2;
       elm_entry_markup_filter_append(entry, elm_entry_filter_limit_size, &limit_filter_data);

       evas_object_smart_callback_add(entry, "focused", _entry_focused_cb, layout_mod);
       evas_object_smart_callback_add(entry, "activated", _entry_activated_cb, layout_mod);

       if (idx == ELM_DATETIME_HOUR)
         elm_entry_markup_filter_append(entry, _hour_validity_checking_filter, layout_mod);
       else if (idx == ELM_DATETIME_MINUTE)
         elm_entry_markup_filter_append(entry, _minute_validity_checking_filter, layout_mod);
     }
}

static void
_show_datepicker_layout(Layout_Module_Data *layout_mod)
{
   struct tm curr_time;
   int idx, year, month, min, max;
   Elm_Datetime_Field_Type  field_type = ELM_DATETIME_YEAR;

   if (!layout_mod || !layout_mod->mod_data.base) return;

   elm_datetime_value_get(layout_mod->mod_data.base, &(layout_mod->set_time));

   elm_datetime_value_get(layout_mod->mod_data.base, &curr_time);
   year = curr_time.tm_year + STRUCT_TM_YEAR_BASE_VALUE;
   elm_spinner_value_set(layout_mod->layout_field[ELM_DATETIME_YEAR], year);
   month = curr_time.tm_mon + 1;
   elm_spinner_value_set(layout_mod->layout_field[ELM_DATETIME_MONTH], month);
   elm_spinner_value_set(layout_mod->layout_field[ELM_DATETIME_DATE], curr_time.tm_mday);

   for (idx = 0; idx <= ELM_DATETIME_DATE; idx++)
     {
         field_type = (Elm_Datetime_Field_Type)idx;
         layout_mod->mod_data.field_limit_get(layout_mod->mod_data.base, field_type, &min, &max);
         if (idx == ELM_DATETIME_YEAR)
           elm_spinner_min_max_set(layout_mod->layout_field[idx], (STRUCT_TM_YEAR_BASE_VALUE + min), (STRUCT_TM_YEAR_BASE_VALUE + max));
         else if (idx == ELM_DATETIME_MONTH)
           elm_spinner_min_max_set(layout_mod->layout_field[idx], (1+min), (1+max));
         else
           elm_spinner_min_max_set(layout_mod->layout_field[idx], min, max);
     }
}

static void
_show_timepicker_layout(Layout_Module_Data *layout_mod)
{
   struct tm curr_time, min_time, max_time;

   if (!layout_mod || !layout_mod->mod_data.base) return;

   elm_datetime_value_get(layout_mod->mod_data.base, &(layout_mod->set_time));

   elm_datetime_value_get(layout_mod->mod_data.base, &curr_time);
   layout_mod->is_pm = (curr_time.tm_hour < STRUCT_TM_TIME_12HRS_MAX_VALUE) ? 0 : 1;

   layout_mod->mod_data.fields_min_max_get(layout_mod->mod_data.base, &curr_time, &min_time, &max_time);
   elm_spinner_min_max_set(layout_mod->layout_field[ELM_DATETIME_HOUR],
                            min_time.tm_hour, max_time.tm_hour);
   elm_spinner_min_max_set(layout_mod->layout_field[ELM_DATETIME_MINUTE],
                            min_time.tm_min, max_time.tm_min);

   elm_spinner_value_set(layout_mod->layout_field[ELM_DATETIME_HOUR], curr_time.tm_hour);
   elm_spinner_value_set(layout_mod->layout_field[ELM_DATETIME_MINUTE], curr_time.tm_min);

   if (layout_mod->time_12hr_fmt)
     _set_ampm_value(layout_mod);
}

static void
_set_month_special_values(Layout_Module_Data *layout_mod)
{
   struct tm curr_time;
   const char *fmt = NULL;
   int month;
   UDateFormat *dt_formatter = NULL;
   UErrorCode status = U_ZERO_ERROR;
   UChar str[MONTH_STRING_MAX_SIZE];
   char locale[32] = {0,};
   char *locale_tmp;
   char *p = NULL;
   UChar Pattern[256] = {0, };
   time_t time_val;
   UDate date;
   char buf[MONTH_STRING_MAX_SIZE] = {0,};


   if (!layout_mod) return;

   elm_datetime_value_get(layout_mod->mod_data.base, &curr_time);
   fmt = layout_mod->mod_data.field_format_get(layout_mod->mod_data.base, ELM_DATETIME_MONTH);

   locale_tmp = setlocale(LC_MESSAGES, NULL);
   strcpy(locale, locale_tmp);

   if (locale[0] != '\0')
     {
        p = strstr(locale, ".UTF-8");
        if (p) *p = 0;
     }

   for (month = 0; month < TOTAL_NUMBER_OF_MONTHS; month++)
     {
         curr_time.tm_mon = month;
         curr_time.tm_isdst = -1;
         time_val = (time_t)mktime(&curr_time);
          date = (UDate)time_val * 1000;

         if (!strcmp(fmt, "%B"))
           u_uastrncpy(Pattern, "MMMM", strlen("MMMM"));
         else if (!strcmp(fmt, "%m"))
           u_uastrncpy(Pattern, "MM", strlen("MM"));
         else
           u_uastrncpy(Pattern, "MMM", strlen("MMM"));
         dt_formatter = udat_open(UDAT_IGNORE, UDAT_IGNORE, locale, NULL, -1, Pattern, -1, &status);
         udat_format(dt_formatter, date, str, sizeof(str), NULL, &status);
         u_austrcpy(buf, str);
         udat_close(dt_formatter);
         dt_formatter = udat_open(UDAT_IGNORE, UDAT_IGNORE, locale, NULL, -1, Pattern, -1, &status);
         udat_format(dt_formatter, date, str, sizeof(str), NULL, &status);
         u_austrcpy(buf, str);
         udat_close(dt_formatter);
         strcpy(month_arr[month],buf);
     }
   for (month = 0; month < 12; month++)
     elm_spinner_special_value_add(layout_mod->layout_field[ELM_DATETIME_MONTH],
                                   month + 1, month_arr[month]);
}

static void
_set_hour_12hrfmt_special_values(Layout_Module_Data *layout_mod)
{
   char buf[BUFF_SIZE];
   double hour;

   if (!layout_mod) return;

   for (hour = STRUCT_TM_TIME_12HRS_MAX_VALUE +1; hour <= STRUCT_TM_TIME_24HRS_MAX_VALUE; hour++)
     {
         snprintf(buf, sizeof(buf), "%02.0f", hour-12);
         elm_spinner_special_value_add(layout_mod->layout_field[ELM_DATETIME_HOUR], hour, buf);
     }

   elm_spinner_special_value_add(layout_mod->layout_field[ELM_DATETIME_HOUR], 0, "12");
}

static void
_unset_hour_12hrfmt_special_values(Layout_Module_Data *layout_mod)
{
   int hour;
   if (!layout_mod) return;

   for (hour = STRUCT_TM_TIME_12HRS_MAX_VALUE +1; hour <= STRUCT_TM_TIME_24HRS_MAX_VALUE; hour++)
     elm_spinner_special_value_del(layout_mod->layout_field[ELM_DATETIME_HOUR], hour);

   elm_spinner_special_value_del(layout_mod->layout_field[ELM_DATETIME_HOUR], 0);
}

static char *
_access_date_picker_state_cb(void *data, Evas_Object *obj EINA_UNUSED)
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
_access_time_picker_state_cb(void *data, Evas_Object *obj EINA_UNUSED)
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
_date_picker_layout_del_cb(void *data, Evas * e EINA_UNUSED, Evas_Object * obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Layout_Module_Data *layout_mod;

   layout_mod = (Layout_Module_Data *)data;
   if (!layout_mod) return;

   if (layout_mod->datepicker_layout)
     layout_mod->datepicker_layout = NULL;
}

static void
_time_picker_layout_del_cb(void *data, Evas * e EINA_UNUSED, Evas_Object * obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Layout_Module_Data *layout_mod;

   layout_mod = (Layout_Module_Data *)data;
   if (!layout_mod) return;

   if (layout_mod->timepicker_layout)
     layout_mod->timepicker_layout = NULL;
}

static void
_create_datepicker_layout(Layout_Module_Data *layout_mod)
{
   Evas_Object *spinner, *entry, *ao;
   char buf[BUFF_SIZE];
   int idx, loc;

   if (!layout_mod || !layout_mod->mod_data.base) return;

   layout_mod->datepicker_layout = elm_layout_add(layout_mod->mod_data.base);
   elm_layout_theme_set(layout_mod->datepicker_layout, "layout", "datetime", "date_layout");

   for (idx = 0; idx <= ELM_DATETIME_DATE; idx++)
     {
        spinner = elm_spinner_add(layout_mod->datepicker_layout);
        elm_spinner_editable_set(spinner, EINA_TRUE);
        layout_mod->mod_data.field_location_get(layout_mod->mod_data.base, idx, &loc);
        elm_object_style_set(spinner, "date_picker");
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
        if (loc > ELM_DATETIME_DATE) loc  = loc - PICKER_layout_field_COUNT;
        snprintf(buf, sizeof(buf), "field%d", loc);
        layout_mod->field_location[idx] = loc;
        elm_object_part_content_set(layout_mod->datepicker_layout, buf, spinner);

        entry = elm_object_part_content_get(spinner, "elm.swallow.entry");
        if (idx == ELM_DATETIME_MONTH)
          elm_entry_input_panel_layout_set(entry, ELM_INPUT_PANEL_LAYOUT_MONTH);
        else
           elm_entry_input_panel_layout_set(entry, ELM_INPUT_PANEL_LAYOUT_DATETIME);
        elm_entry_drag_disabled_set(entry, EINA_TRUE);
        if (loc == (PICKER_layout_field_COUNT - 1))
          elm_entry_input_panel_return_key_type_set(entry, ELM_INPUT_PANEL_RETURN_KEY_TYPE_DONE);
        else
          elm_entry_input_panel_return_key_type_set(entry, ELM_INPUT_PANEL_RETURN_KEY_TYPE_NEXT);

        if (idx == ELM_DATETIME_YEAR)
          elm_spinner_min_max_set(spinner, 1902, 2037);
        else if (idx == ELM_DATETIME_MONTH)
          elm_spinner_min_max_set(spinner, 1, 12);
        else if (idx == ELM_DATETIME_DATE)
          elm_spinner_min_max_set(spinner, 1, 31);

        evas_object_smart_callback_add(spinner, "changed", _datepicker_value_changed_cb, layout_mod);
        layout_mod->layout_field[idx] = spinner;
     }

   _set_month_special_values(layout_mod);
   _set_datepicker_entry_filter(layout_mod);
   evas_object_event_callback_add(layout_mod->datepicker_layout, EVAS_CALLBACK_DEL, _date_picker_layout_del_cb, layout_mod);
}

static char *
_access_ampm_info_cb(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED)
{
   return strdup(E_("WDS_TTS_TBBODY_DOUBLE_TAP_TO_CHANGE"));
}

static void
_create_timepicker_layout(Layout_Module_Data *layout_mod)
{
   Evas_Object *spinner, *ampm, *entry, *ao;
   char buf[BUFF_SIZE];
   int idx;

   if (!layout_mod || !layout_mod->mod_data.base) return;

   layout_mod->timepicker_layout = elm_layout_add(layout_mod->mod_data.base);
   layout_mod->time_12hr_fmt = layout_mod->mod_data.field_location_get(layout_mod->mod_data.base, ELM_DATETIME_AMPM, NULL);

   if (layout_mod->time_12hr_fmt)
     elm_layout_theme_set(layout_mod->timepicker_layout, "layout", "datetime", "time_layout");
   else
     elm_layout_theme_set(layout_mod->timepicker_layout, "layout", "datetime", "time_layout_24hr");

   for (idx = ELM_DATETIME_HOUR; idx < ELM_DATETIME_AMPM; idx++)
     {
       spinner = elm_spinner_add(layout_mod->timepicker_layout);
       elm_spinner_editable_set(spinner, EINA_TRUE);
       elm_object_style_set(spinner, "time_picker");
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
       elm_object_part_content_set(layout_mod->timepicker_layout, buf, spinner);
       if (idx == ELM_DATETIME_HOUR)
         evas_object_smart_callback_add(spinner, "entry,apply", _hour_validity_check, layout_mod);
       evas_object_smart_callback_add(spinner, "changed", _timepicker_value_changed_cb, layout_mod);
       layout_mod->layout_field[idx] = spinner;
       entry = elm_object_part_content_get(spinner, "elm.swallow.entry");
       elm_entry_input_panel_layout_set(entry, ELM_INPUT_PANEL_LAYOUT_DATETIME);
       elm_entry_drag_disabled_set(entry, EINA_TRUE);
       if (idx == ELM_DATETIME_MINUTE)
         elm_entry_input_panel_return_key_type_set(entry, ELM_INPUT_PANEL_RETURN_KEY_TYPE_DONE);
       else
         elm_entry_input_panel_return_key_type_set(entry, ELM_INPUT_PANEL_RETURN_KEY_TYPE_NEXT);
    }

   ampm = elm_button_add(layout_mod->timepicker_layout);
   elm_object_style_set(ampm, "datetime/ampm");
   evas_object_smart_callback_add(ampm, "clicked", _ampm_clicked_cb, layout_mod);
   elm_object_part_content_set(layout_mod->timepicker_layout, "field2",ampm);
   elm_access_info_cb_set(ampm, ELM_ACCESS_TYPE,
                          _access_ampm_info_cb, NULL);
   layout_mod->layout_field[ELM_DATETIME_AMPM] = ampm;

   elm_spinner_min_max_set(layout_mod->layout_field[ELM_DATETIME_HOUR], 0, 23);
   elm_spinner_min_max_set(layout_mod->layout_field[ELM_DATETIME_MINUTE], 0, 59);

   if (layout_mod->time_12hr_fmt)
     _set_hour_12hrfmt_special_values(layout_mod);

   _set_timepicker_entry_filter(layout_mod);
   evas_object_event_callback_add(layout_mod->timepicker_layout, EVAS_CALLBACK_DEL, _time_picker_layout_del_cb, layout_mod);
}

static void
_weekday_loc_update(Layout_Module_Data *layout_mod)
{
   char *fmt;

   if (!layout_mod) return;

   fmt = nl_langinfo(D_T_FMT);
   if (!strncmp(fmt, "%a", 2) || !strncmp(fmt, "%A", 2))
     layout_mod->weekday_loc_first = EINA_TRUE;
   else
     layout_mod->weekday_loc_first = EINA_FALSE;
}

static void
_launch_datepicker(void *data, Evas_Object * obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Layout_Module_Data *layout_mod;

   layout_mod = (Layout_Module_Data *)data;
   if (!layout_mod) return;

   if (!layout_mod->datepicker_layout)
     _create_datepicker_layout(layout_mod);

   _show_datepicker_layout(layout_mod);
   evas_object_smart_callback_call(layout_mod->mod_data.base, SIG_DATE_PICKER_BTN_CLICKED, NULL);
}

static void
_launch_timepicker(void *data, Evas_Object * obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Layout_Module_Data *layout_mod;

   layout_mod = (Layout_Module_Data *)data;
   if (!layout_mod) return;

   if (!layout_mod->timepicker_layout)
     _create_timepicker_layout(layout_mod);

   _show_timepicker_layout(layout_mod);
   evas_object_smart_callback_call(layout_mod->mod_data.base, SIG_TIME_PICKER_BTN_CLICKED, NULL);
}

static void
_module_format_change(Layout_Module_Data *layout_mod)
{
   Evas_Object *datetime, *spinner, *entry;
   int field_loc[PICKER_layout_field_COUNT];
   char buf[BUFF_SIZE];
   int idx, loc;
   Eina_Bool show_date_btn = EINA_FALSE;
   Eina_Bool show_time_btn = EINA_FALSE;
   Eina_Bool time_12hr;

   if (!layout_mod) return;

   datetime = layout_mod->mod_data.base;
   if (layout_mod->datepicker_layout)
     {
        for (idx = 0; idx <= ELM_DATETIME_DATE; idx++)
          {
             layout_mod->mod_data.field_location_get(datetime, idx, &loc);
             snprintf(buf, sizeof(buf), "field%d", layout_mod->field_location[idx]);
             elm_object_part_content_unset(layout_mod->datepicker_layout, buf);
             field_loc[idx] = loc;
          }
        for (idx = 0; idx <= ELM_DATETIME_DATE; idx++)
          {
             snprintf(buf, sizeof(buf), "field%d", field_loc[idx]);
             elm_object_part_content_set(layout_mod->datepicker_layout, buf, layout_mod->layout_field[idx]);
             layout_mod->field_location[idx] = field_loc[idx];
          }
        for (idx = 0; idx <= ELM_DATETIME_DATE; idx++)
          {
             entry = elm_object_part_content_get(layout_mod->layout_field[idx], "elm.swallow.entry");
             if (layout_mod->field_location[idx] == (PICKER_layout_field_COUNT - 1))
               elm_entry_input_panel_return_key_type_set(entry, ELM_INPUT_PANEL_RETURN_KEY_TYPE_DONE);
             else
               elm_entry_input_panel_return_key_type_set(entry, ELM_INPUT_PANEL_RETURN_KEY_TYPE_NEXT);
          }
        _set_month_special_values(layout_mod);
     }
   for (idx = 0; idx <= ELM_DATETIME_DATE; idx++)
     show_date_btn |= layout_mod->mod_data.field_location_get(datetime, idx, NULL);
   if (show_date_btn)
     {
        elm_object_part_content_set(datetime, "date.btn", layout_mod->date_btn);
        elm_layout_signal_emit(datetime, "datepicker,show", "elm");
        layout_mod->dateBtn_is_visible = EINA_TRUE;
     }
   else
     {
        elm_object_part_content_unset(datetime, "date.btn");
        evas_object_hide(layout_mod->date_btn);
        elm_layout_signal_emit(datetime, "datepicker,hide", "elm");
        layout_mod->dateBtn_is_visible = EINA_FALSE;
     }

   for (idx = ELM_DATETIME_HOUR; idx < ELM_DATETIME_AMPM; idx++)
     show_time_btn |= layout_mod->mod_data.field_location_get(datetime, idx, NULL);
   if (show_time_btn)
     {
        elm_object_part_content_set(datetime, "time.btn", layout_mod->time_btn);
        elm_layout_signal_emit(datetime, "timepicker,show", "elm");
        layout_mod->timeBtn_is_visible = EINA_TRUE;
     }
   else
     {
        elm_object_part_content_unset(datetime, "time.btn");
        evas_object_hide(layout_mod->time_btn);
        elm_layout_signal_emit(datetime, "timepicker,hide", "elm");
        layout_mod->timeBtn_is_visible = EINA_FALSE;
     }

   time_12hr = layout_mod->mod_data.field_location_get(datetime, ELM_DATETIME_AMPM, NULL);
   if ((layout_mod->time_12hr_fmt != time_12hr) && (layout_mod->timepicker_layout))
     {
       for (idx = ELM_DATETIME_HOUR; idx <= ELM_DATETIME_AMPM; idx++)
         {
            snprintf(buf, sizeof(buf), "field%d", (idx - ELM_DATETIME_HOUR));
            spinner = elm_object_part_content_unset(layout_mod->timepicker_layout, buf);
            if (spinner) evas_object_hide(spinner);
         }
        if (time_12hr)
          {
             elm_layout_theme_set(layout_mod->timepicker_layout, "layout", "datetime", "time_layout");
             for (idx = ELM_DATETIME_HOUR; idx <= ELM_DATETIME_AMPM; idx++)
               {
                  snprintf(buf, sizeof(buf), "field%d", (idx - ELM_DATETIME_HOUR));
                  elm_object_part_content_set(layout_mod->timepicker_layout, buf, layout_mod->layout_field[idx]);
               }
             _set_hour_12hrfmt_special_values(layout_mod);
          }
        else
          {
             elm_layout_theme_set(layout_mod->timepicker_layout, "layout", "datetime", "time_layout_24hr");
             for (idx = ELM_DATETIME_HOUR; idx <= ELM_DATETIME_MINUTE; idx++)
               {
                  snprintf(buf, sizeof(buf), "field%d", (idx - ELM_DATETIME_HOUR));
                  elm_object_part_content_set(layout_mod->timepicker_layout, buf, layout_mod->layout_field[idx]);
               }
             _unset_hour_12hrfmt_special_values(layout_mod);
          }
     }
   layout_mod->time_12hr_fmt = time_12hr;
   _weekday_loc_update(layout_mod);
   if (layout_mod->time_12hr_fmt)
     _set_ampm_value(layout_mod);
   elm_layout_sizing_eval(layout_mod->mod_data.base);
}

// module functions for the specific module type

EAPI void
display_fields(Elm_Datetime_Module_Data *module_data)
{
   Layout_Module_Data *layout_mod;
   struct tm curr_time;
   char date_str[BUFF_SIZE] = {0,};
   char time_str[BUFF_SIZE] = {0,};
   char buf[BUFF_SIZE] = {0,};
   const char *fmt;
   int i, idx, loc;
   Eina_Bool is_visible;
   UDateFormat *dt_formatter = NULL;
   UErrorCode status = U_ZERO_ERROR;
   UChar str[256];
   char locale[32] = {0,};
   char *locale_tmp;
   char *p = NULL;
   UChar Pattern[256] = {0, };
   time_t time_val;
   UDate date;

   layout_mod = (Layout_Module_Data *)module_data;
   if (!layout_mod || !layout_mod->time_btn || !layout_mod->time_btn) return;

   elm_datetime_value_get(layout_mod->mod_data.base, &curr_time);
   locale_tmp = setlocale(LC_MESSAGES, NULL);
   strcpy(locale, locale_tmp);
   if (locale[0] != '\0')
     {
        p = strstr(locale, ".UTF-8");
        if (p) *p = 0;
     }
   curr_time.tm_isdst = -1;
   time_val = (time_t)mktime(&curr_time);
   date = (UDate)time_val * 1000;
   if (layout_mod->weekday_show && layout_mod->weekday_loc_first)
     {
        is_visible = layout_mod->mod_data.field_location_get
                           (layout_mod->mod_data.base, ELM_DATETIME_DATE, NULL);
        if (is_visible)
          {
             u_uastrncpy(Pattern, "EEE", strlen("EEE"));
             dt_formatter = udat_open(UDAT_IGNORE, UDAT_IGNORE, locale, NULL, -1, Pattern, -1, &status);
             udat_format(dt_formatter, date, str, sizeof(str), NULL, &status);
             u_austrcpy(buf, str);
             udat_close(dt_formatter);
             strcat(buf, ", ");
             strcat(date_str, buf);
          }
     }
   for (i = 0; i <= ELM_DATETIME_DATE; i++)
     {
        for(idx = 0; idx <= ELM_DATETIME_DATE; idx++)
          {
             is_visible = layout_mod->mod_data.field_location_get
                                   (layout_mod->mod_data.base, idx, &loc);
             if (is_visible && (loc == i))
               {
                  fmt = layout_mod->mod_data.field_format_get
                                 (layout_mod->mod_data.base, idx);
                  if( idx == ELM_DATETIME_MONTH )
                     {
                        if (!strcmp(fmt, "%B"))
                          u_uastrncpy(Pattern, "MMMM", strlen("MMMM"));
                        else if (!strcmp(fmt, "%m"))
                          u_uastrncpy(Pattern, "MM", strlen("MM"));
                        else
                          u_uastrncpy(Pattern, "MMM", strlen("MMM"));

                        dt_formatter = udat_open(UDAT_IGNORE, UDAT_IGNORE, locale, NULL, -1, Pattern, -1, &status);
                        udat_format(dt_formatter, date, str, sizeof(str), NULL, &status);
                        u_austrcpy(buf, str);
                        udat_close(dt_formatter);
                        if (loc != 0) strcat(date_str, "/");
                        strcat(date_str, buf);
                        break;
                     }
                  else
                    {
                       curr_time.tm_isdst = -1;
                       mktime(&curr_time);
                       strftime(buf, sizeof(buf), fmt, &curr_time);
                       if (loc != 0) strcat(date_str, "/");
                       strcat(date_str, buf);
                       break;
                    }
               }
           }
       }
   if (layout_mod->weekday_show && !layout_mod->weekday_loc_first)
     {
        is_visible = layout_mod->mod_data.field_location_get
                           (layout_mod->mod_data.base, ELM_DATETIME_DATE, NULL);
        if (is_visible)
          {
             u_uastrncpy(Pattern, "EEE", strlen("EEE"));
             dt_formatter = udat_open(UDAT_IGNORE, UDAT_IGNORE, locale, NULL, -1, Pattern, -1, &status);
             udat_format(dt_formatter, date, str, sizeof(str), NULL, &status);
             u_austrcpy(buf, str);
             udat_close(dt_formatter);
             strcat(date_str, " ,");
             strcat(date_str, buf);
          }
     }

   is_visible = layout_mod->mod_data.field_location_get(layout_mod->mod_data.base, ELM_DATETIME_AMPM, &loc);
   /* AM/PM is comes before hr:min */
   if (is_visible && ((loc == 0) || (loc == 3)))
     {
        curr_time.tm_isdst = -1;
        mktime(&curr_time);

        if (curr_time.tm_hour < 12) strcat(time_str, E_("IDS_COM_BODY_AM"));
        else strcat(time_str, E_("IDS_COM_BODY_PM"));

        strcat(time_str, " ");
     }
   for (idx = ELM_DATETIME_HOUR; idx <= ELM_DATETIME_MINUTE; idx++)
     {
        is_visible = layout_mod->mod_data.field_location_get
                              (layout_mod->mod_data.base, idx, NULL);
        if (is_visible)
          {
             fmt = layout_mod->mod_data.field_format_get
                            (layout_mod->mod_data.base, idx);
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
   is_visible = layout_mod->mod_data.field_location_get(layout_mod->mod_data.base, ELM_DATETIME_AMPM, &loc);
   /* AM/PM is comes after hr:min */
   if (is_visible && (loc != 0) && (loc != 3))
     {
        strcat(time_str, " ");
        curr_time.tm_isdst = -1;
        mktime(&curr_time);

        if (curr_time.tm_hour < 12) strcat(time_str, E_("IDS_COM_BODY_AM"));
        else strcat(time_str, E_("IDS_COM_BODY_PM"));

     }
   elm_object_text_set(layout_mod->date_btn, date_str);
   elm_object_text_set(layout_mod->time_btn, time_str);
}

static void
_datepicker_show_cb(void *data,
                   Evas_Object *obj EINA_UNUSED,
                   const char *emission EINA_UNUSED,
                   const char *source EINA_UNUSED)
{
   _launch_datepicker(data, NULL, NULL);
}

static void
_timepicker_show_cb(void *data,
                   Evas_Object *obj EINA_UNUSED,
                   const char *emission EINA_UNUSED,
                   const char *source EINA_UNUSED)
{
   _launch_timepicker(data, NULL, NULL);
}

static void
_module_language_changed_cb(void *data,
                            Evas_Object *obj EINA_UNUSED,
                            void *event_info EINA_UNUSED)
{
   Layout_Module_Data *layout_mod;
   layout_mod = (Layout_Module_Data *)data;
   if (!layout_mod) return;

   _weekday_loc_update(layout_mod);

   _set_month_special_values(layout_mod);
   _set_ampm_value(layout_mod);
}

static void
_weekday_show_cb(void *data,
                 Evas_Object *obj EINA_UNUSED,
                 const char *emission EINA_UNUSED,
                 const char *source EINA_UNUSED)
{
   Layout_Module_Data *layout_mod;

   layout_mod = (Layout_Module_Data *)data;
   if (!layout_mod) return;

   layout_mod->weekday_show = EINA_TRUE;
   display_fields((Elm_Datetime_Module_Data *)layout_mod);
}

static void
_weekday_hide_cb(void *data,
                 Evas_Object *obj EINA_UNUSED,
                 const char *emission EINA_UNUSED,
                 const char *source EINA_UNUSED)
{
   Layout_Module_Data *layout_mod;

   layout_mod = (Layout_Module_Data *)data;
   if (!layout_mod) return;

   layout_mod->weekday_show = EINA_FALSE;
   display_fields((Elm_Datetime_Module_Data *)layout_mod);
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
_access_date_info_cb(void *data, Evas_Object *obj EINA_UNUSED)
{
   int idx = 0, loc = 0;
   char *ret = NULL;
   Eina_Strbuf *buf = NULL;
   Layout_Module_Data *layout_mod;
   layout_mod = (Layout_Module_Data *)data;
   if (!layout_mod) return NULL;
   struct tm set_date;

   elm_datetime_value_get(layout_mod->mod_data.base, &set_date);
   buf = eina_strbuf_new();
   set_date.tm_isdst = -1;
   mktime(&set_date);
   for (idx = 0; idx <= ELM_DATETIME_DATE; idx++)
     {
        layout_mod->mod_data.field_location_get(layout_mod->mod_data.base, idx, &loc);
        if (loc > ELM_DATETIME_DATE) loc  = loc - PICKER_layout_field_COUNT;
        layout_mod->field_location[loc] = idx;
     }
   if (layout_mod->weekday_show && layout_mod->weekday_loc_first)
     {
        strftime(layout_mod->weekday, BUFF_SIZE, "%A ", &set_date);
        eina_strbuf_append(buf, layout_mod->weekday);
     }
   for (idx = 0; idx <= ELM_DATETIME_DATE; idx++)
     {
        eina_strbuf_append(buf,_picker_location_text_get(layout_mod->field_location[idx], &set_date));
        if (idx < ELM_DATETIME_DATE) eina_strbuf_append(buf, " ");
     }
   if (layout_mod->weekday_show && !layout_mod->weekday_loc_first)
     {
        strftime(layout_mod->weekday, BUFF_SIZE, " %A", &set_date);
        eina_strbuf_append(buf, layout_mod->weekday);
     }
   ret = eina_strbuf_string_steal(buf);
   eina_strbuf_free(buf);
   return ret;
}

static char *
_access_time_info_cb(void *data, Evas_Object *obj EINA_UNUSED)
{
   char *ret = NULL;
   Eina_Strbuf *buf;
   Layout_Module_Data *layout_mod;
   struct tm set_time;
   char buffer[BUFF_SIZE] = {0,};
   int loc;
   Eina_Bool is_visible;

   layout_mod = (Layout_Module_Data *)data;
   if (!layout_mod) return NULL;

   elm_datetime_value_get(layout_mod->mod_data.base, &set_time);
   buf = eina_strbuf_new();
   if (layout_mod->time_12hr_fmt)
     {
        is_visible = layout_mod->mod_data.field_location_get(layout_mod->mod_data.base,
                                                            ELM_DATETIME_AMPM, &loc);
        if (is_visible && ((loc == 0) || (loc == 3)))
          {
             strftime(buffer, BUFF_SIZE, "%p ", &set_time);
             eina_strbuf_append_printf(buf, "%s", buffer);
          }
        strftime(buffer, BUFF_SIZE, E_("IDS_ACCS_BODY_PD_M_HOUR"), &set_time);
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
        strftime(buffer, BUFF_SIZE, E_("IDS_ACCS_BODY_PD_M_HOUR"), &set_time);
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
focus_object_list_get(Elm_Datetime_Module_Data *module_data, Evas_Object *obj EINA_UNUSED)
{
   Layout_Module_Data *layout_mod;
   Eina_List *items = NULL;

   layout_mod = (Layout_Module_Data *)module_data;
   if (!layout_mod) return NULL;

   items = eina_list_append(items, layout_mod->date_btn);
   items = eina_list_append(items, layout_mod->time_btn);

   return items;
}

EAPI void
access_register(Elm_Datetime_Module_Data *module_data)
{
   Layout_Module_Data *layout_mod;

   layout_mod = (Layout_Module_Data *)module_data;
   if (!layout_mod) return;

   elm_access_info_cb_set(layout_mod->date_btn, ELM_ACCESS_INFO,
                          _access_date_info_cb, layout_mod);
   elm_access_info_cb_set(layout_mod->time_btn, ELM_ACCESS_INFO,
                          _access_time_info_cb, layout_mod);
}

EAPI void
create_fields(Elm_Datetime_Module_Data *mdata)
{
   Layout_Module_Data *layout_mod;
   Evas_Object *obj;

   layout_mod = (Layout_Module_Data *)mdata;
   if (!layout_mod) return;

   obj = layout_mod->mod_data.base;
   layout_mod->date_btn = elm_button_add(obj);
   elm_object_style_set(layout_mod->date_btn, "datetime");
   elm_object_part_content_set(obj, "date.btn", layout_mod->date_btn);
   evas_object_smart_callback_add(layout_mod->date_btn, "clicked",
                                  _launch_datepicker, layout_mod);

   layout_mod->time_btn = elm_button_add(obj);
   elm_object_style_set(layout_mod->time_btn, "datetime");
   elm_object_part_content_set(obj, "time.btn", layout_mod->time_btn);
   evas_object_smart_callback_add(layout_mod->time_btn, "clicked",
                                  _launch_timepicker, layout_mod);
   elm_object_signal_callback_add(obj, "datepicker,show", "",
                                  _datepicker_show_cb, layout_mod);
   elm_object_signal_callback_add(obj, "timepicker,show", "",
                                  _timepicker_show_cb, layout_mod);
   evas_object_smart_callback_add(obj, "language,changed",
                                  _module_language_changed_cb, layout_mod);
   elm_object_signal_callback_add(obj, "weekday,show", "",
                                  _weekday_show_cb, layout_mod);
   elm_object_signal_callback_add(obj, "weekday,hide", "",
                                  _weekday_hide_cb, layout_mod);
   elm_object_signal_callback_add(obj, "picker,action,value,set", "",
                                  _layout_picker_set_cb, layout_mod);
}

EAPI Elm_Datetime_Module_Data *
obj_hook(Evas_Object *obj EINA_UNUSED)
{
   Layout_Module_Data *layout_mod;

   layout_mod = ELM_NEW(Layout_Module_Data);
   if (!layout_mod) return NULL;

   layout_mod->date_btn = NULL;
   layout_mod->time_btn = NULL;
   layout_mod->datepicker_layout = NULL;
   layout_mod->timepicker_layout = NULL;
   layout_mod->weekday_show = EINA_FALSE;
   layout_mod->weekday_loc_first = EINA_TRUE;
   layout_mod->dateBtn_is_visible = EINA_FALSE;
   layout_mod->timeBtn_is_visible = EINA_FALSE;

   return ((Elm_Datetime_Module_Data*)layout_mod);
}

EAPI void
obj_unhook(Elm_Datetime_Module_Data *module_data)
{
   Layout_Module_Data *layout_mod;

   layout_mod = (Layout_Module_Data *)module_data;
   if (!layout_mod) return;

   if (layout_mod->date_btn)
     evas_object_del(layout_mod->date_btn);
   if (layout_mod->time_btn)
     evas_object_del(layout_mod->time_btn);
   if (layout_mod->datepicker_layout)
     evas_object_del(layout_mod->datepicker_layout);
   if (layout_mod->timepicker_layout)
     evas_object_del(layout_mod->timepicker_layout);

   if (layout_mod)
     {
        free(layout_mod);
        layout_mod = NULL;
     }
}

EAPI void
obj_format_hook(Elm_Datetime_Module_Data *module_data)
{
   Layout_Module_Data *layout_mod;

   layout_mod = (Layout_Module_Data *)module_data;
   if (!layout_mod) return;

   _module_format_change(layout_mod);
}

EAPI void
obj_theme_hook(Elm_Datetime_Module_Data *module_data)
{
   Layout_Module_Data *layout_mod;

   layout_mod = (Layout_Module_Data *)module_data;
   if (!layout_mod) return;
   display_fields((Elm_Datetime_Module_Data *)layout_mod);
}

EAPI void
sizing_eval_hook(Elm_Datetime_Module_Data *module_data)
{
   Layout_Module_Data *layout_mod;
   Evas_Object *obj;
   Evas_Coord dw = 0, dh = 0;
   Evas_Coord tw = 0, th = 0;
   Evas_Coord center_paddingw = 0, center_paddingh = 0;
   int minw = 0;

   layout_mod = (Layout_Module_Data *)module_data;
   if (!layout_mod) return;

   obj = layout_mod->mod_data.base;
   edje_object_part_geometry_get(elm_layout_edje_get(obj), "center.padding", NULL, NULL, &center_paddingw, &center_paddingh);
   if (layout_mod->dateBtn_is_visible)
      evas_object_size_hint_min_get(layout_mod->date_btn, &dw, &dh);

   if (layout_mod->timeBtn_is_visible)
      evas_object_size_hint_min_get(layout_mod->time_btn, &tw, &th);

   minw = center_paddingw + dw + tw;
   evas_object_size_hint_min_set(obj, minw, center_paddingh);
   evas_object_size_hint_max_set(obj, -1, -1);

}
EAPI Evas_Object *
picker_layout_get(Elm_Datetime_Module_Data *mdata, Elm_Datetime_Picker_Type type)
{
   Layout_Module_Data *layout_mod;
   Evas_Object *layout = NULL;
   layout_mod = (Layout_Module_Data *)mdata;
   if (!layout_mod) return NULL;

   if (type == ELM_DATETIME_DATE_PICKER)
     {
        if (!layout_mod->datepicker_layout)
          _create_datepicker_layout(layout_mod);
        _show_datepicker_layout(layout_mod);
        layout = layout_mod->datepicker_layout;
     }
   else
     {
        if (!layout_mod->timepicker_layout)
          _create_timepicker_layout(layout_mod);
        _show_timepicker_layout(layout_mod);
        layout = layout_mod->timepicker_layout;
     }
   return layout;
}

EAPI void
obj_focus_hook(Elm_Datetime_Module_Data *module_data EINA_UNUSED)
{
   // TODO: Default focus for obj_show/obj_hide
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
