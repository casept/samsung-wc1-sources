#include <Elementary.h>
#include "elm_priv.h"
#include "elm_widget_calendar.h"

EAPI const char ELM_CALENDAR_SMART_NAME[] = "elm_calendar";

static const char SIG_CHANGED[] = "changed";
static const char SIG_DISPLAY_CHANGED[] = "display,changed";

static const Evas_Smart_Cb_Description _smart_callbacks[] = {
   {SIG_CHANGED, ""},
   {SIG_DISPLAY_CHANGED, ""},
   {NULL, NULL}
};

/* Should not be translated, it's used if we failed
 * getting from locale. */
static const char *_days_abbrev[] =
{
   "Sun", "Mon", "Tue", "Wed",
   "Thu", "Fri", "Sat"
};

static int _days_in_month[2][12] =
{
   {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31},
   {31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31}
};

EVAS_SMART_SUBCLASS_NEW
  (ELM_CALENDAR_SMART_NAME, _elm_calendar, Elm_Calendar_Smart_Class,
  Elm_Layout_Smart_Class, elm_layout_smart_class_get, _smart_callbacks);

static Elm_Calendar_Mark *
_mark_new(Evas_Object *obj,
          const char *mark_type,
          struct tm *mark_time,
          Elm_Calendar_Mark_Repeat_Type repeat)
{
   Elm_Calendar_Mark *mark;

   mark = calloc(1, sizeof(Elm_Calendar_Mark));
   if (!mark) return NULL;
   mark->obj = obj;
   mark->mark_type = eina_stringshare_add(mark_type);
   mark->mark_time = *mark_time;
   mark->repeat = repeat;

   return mark;
}

static inline void
_mark_free(Elm_Calendar_Mark *mark)
{
   eina_stringshare_del(mark->mark_type);
   free(mark);
}

static void
_elm_calendar_smart_sizing_eval(Evas_Object *obj)
{
   Evas_Coord minw = -1, minh = -1;

   ELM_CALENDAR_DATA_GET(obj, sd);

   elm_coords_finger_size_adjust(8, &minw, ELM_DAY_LAST, &minh);
   edje_object_size_min_restricted_calc
     (ELM_WIDGET_DATA(sd)->resize_obj, &minw, &minh, minw, minh);
   evas_object_size_hint_min_set(obj, minw, minh);
   evas_object_size_hint_max_set(obj, -1, -1);
}

static inline int
_maxdays_get(struct tm *selected_time)
{
   int month, year;

   month = selected_time->tm_mon;
   year = selected_time->tm_year + 1900;

   return _days_in_month
          [((!(year % 4)) && ((!(year % 400)) || (year % 100)))][month];
}

static inline void
_unselect(Evas_Object *obj,
          int selected)
{
   char emission[32];

   snprintf(emission, sizeof(emission), "cit_%i,unselected", selected);
   elm_layout_signal_emit(obj, emission, "elm");
}

static inline void
_select(Evas_Object *obj,
        int selected)
{
   char emission[32];

   ELM_CALENDAR_DATA_GET(obj, sd);

   sd->selected_it = selected;
   snprintf(emission, sizeof(emission), "cit_%i,selected", selected);
   elm_layout_signal_emit(obj, emission, "elm");
}

static inline void
_not_today(Elm_Calendar_Smart_Data *sd)
{
   char emission[32];

   snprintf(emission, sizeof(emission), "cit_%i,not_today", sd->today_it);
   elm_layout_signal_emit(ELM_WIDGET_DATA(sd)->obj, emission, "elm");
   sd->today_it = -1;
}

static inline void
_today(Elm_Calendar_Smart_Data *sd,
       int it)
{
   char emission[32];

   snprintf(emission, sizeof(emission), "cit_%i,today", it);
   elm_layout_signal_emit(ELM_WIDGET_DATA(sd)->obj, emission, "elm");
   sd->today_it = it;
}

static char *
_format_month_year(struct tm *selected_time)
{
   char buf[32];

   if (!strftime(buf, sizeof(buf), E_("%B %Y"), selected_time)) return NULL;
   return strdup(buf);
}

static inline void
_cit_mark(Evas_Object *cal,
          int cit,
          const char *mtype)
{
   char sign[64];

   snprintf(sign, sizeof(sign), "cit_%i,%s", cit, mtype);
   elm_layout_signal_emit(cal, sign, "elm");
}

static inline int
_weekday_get(int first_week_day,
             int day)
{
   return (day + first_week_day - 1) % ELM_DAY_LAST;
}

// EINA_DEPRECATED
static void
_text_day_color_update(Elm_Calendar_Smart_Data *sd,
                       int pos)
{
   char emission[32];

   switch (sd->day_color[pos])
     {
      case DAY_WEEKDAY:
        snprintf(emission, sizeof(emission), "cit_%i,weekday", pos);
        break;

      case DAY_SATURDAY:
        snprintf(emission, sizeof(emission), "cit_%i,saturday", pos);
        break;

      case DAY_SUNDAY:
        snprintf(emission, sizeof(emission), "cit_%i,sunday", pos);
        break;

      default:
        return;
     }

   elm_layout_signal_emit(ELM_WIDGET_DATA(sd)->obj, emission, "elm");
}

static void
_set_month_year(Elm_Calendar_Smart_Data *sd)
{
   char *buf;

   /* Set selected month */
   buf = sd->format_func(&sd->shown_time);
   if (buf)
     {
        elm_layout_text_set(ELM_WIDGET_DATA(sd)->obj, "month_text", buf);
        free(buf);
     }
   else elm_layout_text_set(ELM_WIDGET_DATA(sd)->obj, "month_text", "");
}

static char *
_access_info_cb(void *data __UNUSED__, Evas_Object *obj)
{
   char *ret;
   Eina_Strbuf *buf;
   buf = eina_strbuf_new();

   eina_strbuf_append_printf(buf, "day %s", elm_widget_access_info_get(obj));

   ret = eina_strbuf_string_steal(buf);
   eina_strbuf_free(buf);
   return ret;
}

static void
_access_calendar_item_register(Evas_Object *obj)
{
   int maxdays, day, i;
   char day_s[3] = {0, }, pname[14];
   Evas_Object *ao;

   ELM_CALENDAR_DATA_GET(obj, sd);

   day = 0;
   maxdays = _maxdays_get(&sd->shown_time);
   for (i = 0; i < 42; i++)
     {
        if ((!day) && (i == sd->first_day_it)) day = 1;
        if ((day) && (day <= maxdays))
          {
             snprintf(pname, sizeof(pname), "cit_%i.access", i);

             ao = _elm_access_edje_object_part_object_register
                        (obj, elm_layout_edje_get(obj), pname);
             _elm_access_text_set(_elm_access_object_get(ao),
                         ELM_ACCESS_TYPE, E_("calendar item"));
             _elm_access_callback_set(_elm_access_object_get(ao),
                           ELM_ACCESS_INFO, _access_info_cb, NULL);

             snprintf(day_s, sizeof(day_s), "%i", day++);
             elm_widget_access_info_set(ao, (const char*)day_s);
          }
        else
          {
             snprintf(pname, sizeof(pname), "cit_%i.access", i);
             _elm_access_edje_object_part_object_unregister
                     (obj, elm_layout_edje_get(obj), pname);
          }
     }
}

static void
_access_calendar_spinner_register(Evas_Object *obj)
{
   Evas_Object *po;
   Elm_Access_Info *ai;
   ELM_CALENDAR_DATA_GET(obj, sd);

   // decrement button
   sd->dec_btn_access = _elm_access_edje_object_part_object_register
                            (obj, elm_layout_edje_get(obj), "left_bt");
   ai = _elm_access_object_get(sd->dec_btn_access);
   _elm_access_text_set(ai, ELM_ACCESS_TYPE, E_("calendar decrement button"));

   // increment button
   sd->inc_btn_access = _elm_access_edje_object_part_object_register
                            (obj, elm_layout_edje_get(obj), "right_bt");
   ai = _elm_access_object_get(sd->inc_btn_access);
   _elm_access_text_set(ai, ELM_ACCESS_TYPE, E_("calendar increment button"));

   // month text
   sd->month_access = _elm_access_edje_object_part_object_register
                          (obj, elm_layout_edje_get(obj), "month_text");
   ai = _elm_access_object_get(sd->month_access);
   _elm_access_text_set(ai, ELM_ACCESS_TYPE, E_("calendar month"));

   po = (Evas_Object *)edje_object_part_object_get
          (elm_layout_edje_get(obj), "month_text");
   evas_object_pass_events_set(po, EINA_FALSE);

}

static void
_access_calendar_register(Evas_Object *obj)
{
   _access_calendar_spinner_register(obj);
   _access_calendar_item_register(obj);
}

static void
_populate(Evas_Object *obj)
{
   int maxdays, day, mon, yr, i;
   Elm_Calendar_Mark *mark;
   char part[12], day_s[3] = {0, };
   struct tm first_day;
   Eina_List *l;
   Eina_Bool last_row = EINA_TRUE;

   ELM_CALENDAR_DATA_GET(obj, sd);

   elm_layout_freeze(obj);

   if (sd->today_it > 0) _not_today(sd);

   maxdays = _maxdays_get(&sd->shown_time);
   mon = sd->shown_time.tm_mon;
   yr = sd->shown_time.tm_year;

   _set_month_year(sd);

   /* Set days */
   day = 0;
   first_day = sd->shown_time;
   first_day.tm_mday = 1;
   mktime(&first_day);

   // Layout of the calendar is changed for removing the unfilled last row.
   if (first_day.tm_wday < (int)sd->first_week_day)
     sd->first_day_it = first_day.tm_wday + ELM_DAY_LAST - sd->first_week_day;
   else
     sd->first_day_it = first_day.tm_wday - sd->first_week_day;

   if ((35 - sd->first_day_it) > (maxdays - 1)) last_row = EINA_FALSE;

   if (!last_row)
     {
        char emission[32];

        for (i = 0; i < 5; i++)
          {
             snprintf(emission, sizeof(emission), "cseph_%i,row_hide", i);
             elm_layout_signal_emit(obj, emission, "elm");
          }
        snprintf(emission, sizeof(emission), "cseph_%i,row_invisible", 5);
        elm_layout_signal_emit(obj, emission, "elm");
        for (i = 0; i < 35; i++)
          {
             snprintf(emission, sizeof(emission), "cit_%i,cell_expanded", i);
             elm_layout_signal_emit(obj, emission, "elm");
          }
        for (i = 35; i < 42; i++)
          {
             snprintf(emission, sizeof(emission), "cit_%i,cell_invisible", i);
             elm_layout_signal_emit(obj, emission, "elm");
          }
     }
   else
     {
        char emission[32];

        for (i = 0; i < 6; i++)
          {
             snprintf(emission, sizeof(emission), "cseph_%i,row_show", i);
             elm_layout_signal_emit(obj, emission, "elm");
          }
        for (i = 0; i < 42; i++)
          {
             snprintf(emission, sizeof(emission), "cit_%i,cell_default", i);
             elm_layout_signal_emit(obj, emission, "elm");
          }
     }

   for (i = 0; i < 42; i++)
     {
        _text_day_color_update(sd, i); // EINA_DEPRECATED
        if ((!day) && (i == sd->first_day_it)) day = 1;

        if ((day == sd->current_time.tm_mday)
            && (mon == sd->current_time.tm_mon)
            && (yr == sd->current_time.tm_year))
          _today(sd, i);

        if (day == sd->selected_time.tm_mday)
          {
             if ((sd->selected_it > -1) && (sd->selected_it != i))
               _unselect(obj, sd->selected_it);

             if (sd->select_mode == ELM_CALENDAR_SELECT_MODE_ONDEMAND)
               {
                  if ((mon == sd->selected_time.tm_mon)
                      && (yr == sd->selected_time.tm_year)
                      && (sd->selected))
                    {
                       _select(obj, i);
                    }
               }
             else if (sd->select_mode != ELM_CALENDAR_SELECT_MODE_NONE)
               {
                  _select(obj, i);
               }
          }

        if ((day) && (day <= maxdays))
          snprintf(day_s, sizeof(day_s), "%i", day++);
        else
          day_s[0] = 0;

        snprintf(part, sizeof(part), "cit_%i.text", i);
        elm_layout_text_set(obj, part, day_s);

        /* Clear previous marks */
        _cit_mark(obj, i, "clear");
     }

   // ACCESS
   if ((_elm_config->access_mode != ELM_ACCESS_MODE_OFF))
     _access_calendar_item_register(obj);

   /* Set marks */
   EINA_LIST_FOREACH(sd->marks, l, mark)
     {
        struct tm *mtime = &mark->mark_time;
        int month = sd->shown_time.tm_mon;
        int year = sd->shown_time.tm_year;
        int mday_it = mtime->tm_mday + sd->first_day_it - 1;

        switch (mark->repeat)
          {
           case ELM_CALENDAR_UNIQUE:
             if ((mtime->tm_mon == month) && (mtime->tm_year == year))
               _cit_mark(obj, mday_it, mark->mark_type);
             break;

           case ELM_CALENDAR_DAILY:
             if (((mtime->tm_year == year) && (mtime->tm_mon < month)) ||
                 (mtime->tm_year < year))
               day = 1;
             else if ((mtime->tm_year == year) && (mtime->tm_mon == month))
               day = mtime->tm_mday;
             else
               break;
             for (; day <= maxdays; day++)
               _cit_mark(obj, day + sd->first_day_it - 1,
                         mark->mark_type);
             break;

           case ELM_CALENDAR_WEEKLY:
             if (((mtime->tm_year == year) && (mtime->tm_mon < month)) ||
                 (mtime->tm_year < year))
               day = 1;
             else if ((mtime->tm_year == year) && (mtime->tm_mon == month))
               day = mtime->tm_mday;
             else
               break;
             for (; day <= maxdays; day++)
               if (mtime->tm_wday == _weekday_get(sd->first_day_it, day))
                 _cit_mark(obj, day + sd->first_day_it - 1,
                           mark->mark_type);
             break;

           case ELM_CALENDAR_MONTHLY:
             if (((mtime->tm_year < year) ||
                  ((mtime->tm_year == year) && (mtime->tm_mon <= month))) &&
                 (mtime->tm_mday <= maxdays))
               _cit_mark(obj, mday_it, mark->mark_type);
             break;

           case ELM_CALENDAR_ANNUALLY:
             if ((mtime->tm_year <= year) && (mtime->tm_mon == month) &&
                 (mtime->tm_mday <= maxdays))
               _cit_mark(obj, mday_it, mark->mark_type);
             break;

           case ELM_CALENDAR_LAST_DAY_OF_MONTH:
             if (((mtime->tm_year < year) ||
                  ((mtime->tm_year == year) && (mtime->tm_mon <= month))))
               _cit_mark(obj, maxdays + sd->first_day_it - 1, mark->mark_type);
             break;
          }
     }

   elm_layout_thaw(obj);
}

static void
_set_headers(Evas_Object *obj)
{
   static char part[] = "ch_0.text";
   int i;
   ELM_CALENDAR_DATA_GET(obj, sd);

   elm_layout_freeze(obj);

   for (i = 0; i < ELM_DAY_LAST; i++)
     {
        part[3] = i + '0';
        elm_layout_text_set
          (obj, part, sd->weekdays[(i + sd->first_week_day) % ELM_DAY_LAST]);
     }

   elm_layout_thaw(obj);
}

static Eina_Bool
_elm_calendar_smart_theme(Evas_Object *obj)
{
   if (!ELM_WIDGET_CLASS(_elm_calendar_parent_sc)->theme(obj))
     return EINA_FALSE;

   evas_object_smart_changed(obj);

   return EINA_TRUE;
}

/* Set correct tm_wday and tm_yday after other fields changes*/
static inline void
_fix_selected_time(Elm_Calendar_Smart_Data *sd)
{
   if (sd->selected_time.tm_mon != sd->shown_time.tm_mon)
     sd->selected_time.tm_mon = sd->shown_time.tm_mon;
   if (sd->selected_time.tm_year != sd->shown_time.tm_year)
     sd->selected_time.tm_year = sd->shown_time.tm_year;
   mktime(&sd->selected_time);
}

static Eina_Bool
_update_month(Evas_Object *obj,
              int delta)
{
   struct tm time_check;
   int maxdays;

   ELM_CALENDAR_DATA_GET(obj, sd);

   /* check if it's a valid time. for 32 bits, year greater than 2037 is not */
   time_check = sd->shown_time;
   time_check.tm_mon += delta;
   if (mktime(&time_check) == -1)
     return EINA_FALSE;

   sd->shown_time.tm_mon += delta;
   if (sd->shown_time.tm_mon < 0)
     {
        if (sd->shown_time.tm_year == sd->year_min)
          {
             sd->shown_time.tm_mon++;
             return EINA_FALSE;
          }
        sd->shown_time.tm_mon = 11;
        sd->shown_time.tm_year--;
     }
   else if (sd->shown_time.tm_mon > 11)
     {
        if (sd->shown_time.tm_year == sd->year_max)
          {
             sd->shown_time.tm_mon--;
             return EINA_FALSE;
          }
        sd->shown_time.tm_mon = 0;
        sd->shown_time.tm_year++;
     }

   if ((sd->select_mode != ELM_CALENDAR_SELECT_MODE_ONDEMAND)
       && (sd->select_mode != ELM_CALENDAR_SELECT_MODE_NONE))
     {
        maxdays = _maxdays_get(&sd->shown_time);
        if (sd->selected_time.tm_mday > maxdays)
          sd->selected_time.tm_mday = maxdays;

        _fix_selected_time(sd);
        evas_object_smart_callback_call(obj, SIG_CHANGED, NULL);
     }
   evas_object_smart_callback_call(obj, SIG_DISPLAY_CHANGED, NULL);

   return EINA_TRUE;
}

static Eina_Bool
_spin_value(void *data)
{
   ELM_CALENDAR_DATA_GET(data, sd);

   if (_update_month(data, sd->spin_speed))
     evas_object_smart_changed(data);

   sd->interval = sd->interval / 1.05;
   ecore_timer_interval_set(sd->spin, sd->interval);

   return ECORE_CALLBACK_RENEW;
}

static void
_button_inc_start(void *data,
                  Evas_Object *obj __UNUSED__,
                  const char *emission __UNUSED__,
                  const char *source __UNUSED__)
{
   ELM_CALENDAR_DATA_GET(data, sd);

   sd->interval = sd->first_interval;
   sd->spin_speed = 1;
   if (sd->spin) ecore_timer_del(sd->spin);
   sd->spin = ecore_timer_add(sd->interval, _spin_value, data);

   _spin_value(data);
}

static void
_button_dec_start(void *data,
                  Evas_Object *obj __UNUSED__,
                  const char *emission __UNUSED__,
                  const char *source __UNUSED__)
{
   ELM_CALENDAR_DATA_GET(data, sd);

   sd->interval = sd->first_interval;
   sd->spin_speed = -1;
   if (sd->spin) ecore_timer_del(sd->spin);
   sd->spin = ecore_timer_add(sd->interval, _spin_value, data);

   _spin_value(data);
}

static void
_button_stop(void *data,
             Evas_Object *obj __UNUSED__,
             const char *emission __UNUSED__,
             const char *source __UNUSED__)
{
   ELM_CALENDAR_DATA_GET(data, sd);

   sd->interval = sd->first_interval;
   if (sd->spin) ecore_timer_del(sd->spin);
   sd->spin = NULL;
}

static int
_get_item_day(Evas_Object *obj,
              int selected_it)
{
   int day;

   ELM_CALENDAR_DATA_GET(obj, sd);

   day = selected_it - sd->first_day_it + 1;
   if ((day < 0) || (day > _maxdays_get(&sd->shown_time)))
     return 0;

   return day;
}

static void
_update_sel_it(Evas_Object *obj,
               int sel_it)
{
   int day;

   ELM_CALENDAR_DATA_GET(obj, sd);

   if (sd->select_mode == ELM_CALENDAR_SELECT_MODE_NONE)
     return;

   day = _get_item_day(obj, sel_it);
   if (!day)
     return;

   _unselect(obj, sd->selected_it);
   if (!sd->selected)
     sd->selected = EINA_TRUE;

   sd->selected_time.tm_mday = day;
   _fix_selected_time(sd);
   _select(obj, sel_it);
   evas_object_smart_callback_call(obj, SIG_CHANGED, NULL);
}

static void
_day_selected(void *data,
              Evas_Object *obj __UNUSED__,
              const char *emission __UNUSED__,
              const char *source)
{
   int sel_it;

   ELM_CALENDAR_DATA_GET(data, sd);

   if (sd->select_mode == ELM_CALENDAR_SELECT_MODE_NONE)
     return;

   sel_it = atoi(source);

   _update_sel_it(data, sel_it);
}

static inline int
_time_to_next_day(struct tm *t)
{
   return ((((24 - t->tm_hour) * 60) - t->tm_min) * 60) - t->tm_sec;
}

static Eina_Bool
_update_cur_date(void *data)
{
   time_t current_time;
   int t, day;
   ELM_CALENDAR_DATA_GET(data, sd);

   if (sd->today_it > 0) _not_today(sd);

   current_time = time(NULL);
   localtime_r(&current_time, &sd->current_time);
   t = _time_to_next_day(&sd->current_time);
   ecore_timer_interval_set(sd->update_timer, t);

   if ((sd->current_time.tm_mon != sd->shown_time.tm_mon) ||
       (sd->current_time.tm_year != sd->shown_time.tm_year))
     return ECORE_CALLBACK_RENEW;

   day = sd->current_time.tm_mday + sd->first_day_it - 1;
   _today(sd, day);

   return ECORE_CALLBACK_RENEW;
}

static Eina_Bool
_elm_calendar_smart_event(Evas_Object *obj,
                          Evas_Object *src __UNUSED__,
                          Evas_Callback_Type type,
                          void *event_info)
{
   Evas_Event_Key_Down *ev = event_info;

   ELM_CALENDAR_DATA_GET(obj, sd);

   // TIZEN ONLY(20131221) : When access mode, focused ui is disabled.
   if (_elm_config->access_mode) return EINA_FALSE;

   if (type != EVAS_CALLBACK_KEY_DOWN) return EINA_FALSE;
   if (elm_widget_disabled_get(obj)) return EINA_FALSE;

   if ((!strcmp(ev->keyname, "Prior")) ||
       ((!strcmp(ev->keyname, "KP_Prior")) && (!ev->string)))
     {
        if (_update_month(obj, -1)) _populate(obj);
     }
   else if ((!strcmp(ev->keyname, "Next")) ||
            ((!strcmp(ev->keyname, "KP_Next")) && (!ev->string)))
     {
        if (_update_month(obj, 1)) _populate(obj);
     }
   else if ((sd->select_mode != ELM_CALENDAR_SELECT_MODE_NONE)
            && ((sd->select_mode != ELM_CALENDAR_SELECT_MODE_ONDEMAND)
                || (sd->selected)))
     {
        if ((!strcmp(ev->keyname, "Left")) ||
            ((!strcmp(ev->keyname, "KP_Left")) && (!ev->string)))
          {
             if ((sd->select_mode != ELM_CALENDAR_SELECT_MODE_ONDEMAND)
                 || ((sd->shown_time.tm_year == sd->selected_time.tm_year)
                     && (sd->shown_time.tm_mon == sd->selected_time.tm_mon)))
               _update_sel_it(obj, sd->selected_it - 1);
          }
        else if ((!strcmp(ev->keyname, "Right")) ||
                 ((!strcmp(ev->keyname, "KP_Right")) && (!ev->string)))
          {
             if ((sd->select_mode != ELM_CALENDAR_SELECT_MODE_ONDEMAND)
                 || ((sd->shown_time.tm_year == sd->selected_time.tm_year)
                     && (sd->shown_time.tm_mon == sd->selected_time.tm_mon)))
               _update_sel_it(obj, sd->selected_it + 1);
          }
        else if ((!strcmp(ev->keyname, "Up")) ||
                 ((!strcmp(ev->keyname, "KP_Up")) && (!ev->string)))
          {
             if ((sd->select_mode != ELM_CALENDAR_SELECT_MODE_ONDEMAND)
                 || ((sd->shown_time.tm_year == sd->selected_time.tm_year)
                     && (sd->shown_time.tm_mon == sd->selected_time.tm_mon)))
               _update_sel_it(obj, sd->selected_it - ELM_DAY_LAST);
          }
        else if ((!strcmp(ev->keyname, "Down")) ||
                 ((!strcmp(ev->keyname, "KP_Down")) && (!ev->string)))
          {
             if ((sd->select_mode != ELM_CALENDAR_SELECT_MODE_ONDEMAND)
                 || ((sd->shown_time.tm_year == sd->selected_time.tm_year)
                     && (sd->shown_time.tm_mon == sd->selected_time.tm_mon)))
               _update_sel_it(obj, sd->selected_it + ELM_DAY_LAST);
          }
        else return EINA_FALSE;
     }
   else return EINA_FALSE;

   return EINA_TRUE;
}

static void
_elm_calendar_smart_calculate(Evas_Object *obj)
{
   elm_layout_freeze(obj);

   _set_headers(obj);
   _populate(obj);

   elm_layout_thaw(obj);   
}

static void
_elm_calendar_smart_add(Evas_Object *obj)
{
   EVAS_SMART_DATA_ALLOC(obj, Elm_Calendar_Smart_Data);

   ELM_WIDGET_CLASS(_elm_calendar_parent_sc)->base.add(obj);
}

static void
_elm_calendar_smart_del(Evas_Object *obj)
{
   int i;
   Elm_Calendar_Mark *mark;
   ELM_CALENDAR_DATA_GET(obj, sd);

   if (sd->spin) ecore_timer_del(sd->spin);
   if (sd->update_timer) ecore_timer_del(sd->update_timer);

   if (sd->marks)
     {
        EINA_LIST_FREE (sd->marks, mark)
          {
             _mark_free(mark);
          }
     }

   for (i = 0; i < ELM_DAY_LAST; i++)
     eina_stringshare_del(sd->weekdays[i]);

   ELM_WIDGET_CLASS(_elm_calendar_parent_sc)->base.del(obj);
}

static Eina_Bool
_elm_calendar_smart_focus_next(const Evas_Object *obj,
                               Elm_Focus_Direction dir,
                               Evas_Object **next)
{
   int maxdays, day, i;
   Eina_List *items = NULL;
   Evas_Object *ao;
   Evas_Object *po;

   ELM_CALENDAR_CHECK(obj) EINA_FALSE;
   ELM_CALENDAR_DATA_GET(obj, sd);

   items = eina_list_append(items, sd->month_access);
   items = eina_list_append(items, sd->dec_btn_access);
   items = eina_list_append(items, sd->inc_btn_access);

   day = 0;
   maxdays = _maxdays_get(&sd->shown_time);
   for (i = 0; i < 42; i++)
     {
        if ((!day) && (i == sd->first_day_it)) day = 1;
        if ((day) && (day <= maxdays))
          {
             char pname[14];
             snprintf(pname, sizeof(pname), "cit_%i.access", i);

             po = (Evas_Object *)edje_object_part_object_get
                           (elm_layout_edje_get(obj), pname);
             ao = evas_object_data_get(po, "_part_access_obj");
             items = eina_list_append(items, ao);
          }
     }

   return elm_widget_focus_list_next_get
            (obj, items, eina_list_data_get, dir, next);
}

static void
_access_obj_process(Evas_Object *obj, Eina_Bool is_access)
{
   int maxdays, day, i;

   ELM_CALENDAR_DATA_GET(obj, sd);

   if (is_access)
     _access_calendar_register(obj);
   else
     {
        day = 0;
        maxdays = _maxdays_get(&sd->shown_time);
        for (i = 0; i < 42; i++)
          {
             if ((!day) && (i == sd->first_day_it)) day = 1;
             if ((day) && (day <= maxdays))
               {
                  char pname[14];
                  snprintf(pname, sizeof(pname), "cit_%i.access", i);

                  _elm_access_edje_object_part_object_unregister
                          (obj, elm_layout_edje_get(obj), pname);
               }
          }

        if (sd->dec_btn_access)
          _elm_access_edje_object_part_object_unregister
            (obj, elm_layout_edje_get(obj), "left_bt");
        if (sd->inc_btn_access)
          _elm_access_edje_object_part_object_unregister
            (obj, elm_layout_edje_get(obj), "right_bt");
        if (sd->month_access)
          _elm_access_edje_object_part_object_unregister
            (obj, elm_layout_edje_get(obj), "month_text");
     }
}

static void
_access_hook(Evas_Object *obj, Eina_Bool is_access)
{
   ELM_CALENDAR_CHECK(obj);
   ELM_CALENDAR_DATA_GET(obj, sd);

   if (is_access)
     ELM_WIDGET_CLASS(ELM_WIDGET_DATA(sd)->api)->focus_next =
     _elm_calendar_smart_focus_next;
   else
     ELM_WIDGET_CLASS(ELM_WIDGET_DATA(sd)->api)->focus_next = NULL;
   _access_obj_process(obj, is_access);
}

static void
_elm_calendar_smart_set_user(Elm_Calendar_Smart_Class *sc)
{
   ELM_WIDGET_CLASS(sc)->base.add = _elm_calendar_smart_add;
   ELM_WIDGET_CLASS(sc)->base.del = _elm_calendar_smart_del;
   ELM_WIDGET_CLASS(sc)->base.calculate = _elm_calendar_smart_calculate;

   ELM_WIDGET_CLASS(sc)->theme = _elm_calendar_smart_theme;
   ELM_WIDGET_CLASS(sc)->event = _elm_calendar_smart_event;

   /* not a 'focus chain manager' */
   ELM_WIDGET_CLASS(sc)->focus_next = NULL;
   ELM_WIDGET_CLASS(sc)->focus_direction_manager_is = NULL;
   ELM_WIDGET_CLASS(sc)->focus_direction = NULL;

   ELM_LAYOUT_CLASS(sc)->sizing_eval = _elm_calendar_smart_sizing_eval;

   // ACCESS
   if (_elm_config->access_mode != ELM_ACCESS_MODE_OFF)
     ELM_WIDGET_CLASS(sc)->focus_next = _elm_calendar_smart_focus_next;

   ELM_WIDGET_CLASS(sc)->access = _access_hook;
}

EAPI const Elm_Calendar_Smart_Class *
elm_calendar_smart_class_get(void)
{
   static Elm_Calendar_Smart_Class _sc =
     ELM_CALENDAR_SMART_CLASS_INIT_NAME_VERSION(ELM_CALENDAR_SMART_NAME);
   static const Elm_Calendar_Smart_Class *class = NULL;
   Evas_Smart_Class *esc = (Evas_Smart_Class *)&_sc;

   if (class)
     return class;

   _elm_calendar_smart_set(&_sc);
   esc->callbacks = _smart_callbacks;
   class = &_sc;

   return class;
}

EAPI Evas_Object *
elm_calendar_add(Evas_Object *parent)
{
   Evas_Object *obj;
   time_t weekday = 259200; /* Just the first sunday since epoch */
   time_t current_time;
   int i, t;

   EINA_SAFETY_ON_NULL_RETURN_VAL(parent, NULL);

   obj = elm_widget_add(_elm_calendar_smart_class_new(), parent);
   if (!obj) return NULL;

   if (!elm_widget_sub_object_add(parent, obj))
     ERR("could not add %p as sub object of %p", obj, parent);

   ELM_CALENDAR_DATA_GET(obj, sd);

   sd->first_interval = 0.85;
   sd->year_min = 2;
   sd->year_max = -1;
   sd->today_it = -1;
   sd->selected_it = -1;
   sd->first_day_it = -1;
   sd->format_func = _format_month_year;
   sd->marks = NULL;
   sd->selectable = (~(ELM_CALENDAR_SELECTABLE_NONE));

   edje_object_signal_callback_add
     (ELM_WIDGET_DATA(sd)->resize_obj, "elm,action,increment,start", "*",
     _button_inc_start, obj);
   edje_object_signal_callback_add
     (ELM_WIDGET_DATA(sd)->resize_obj, "elm,action,decrement,start", "*",
     _button_dec_start, obj);
   edje_object_signal_callback_add
     (ELM_WIDGET_DATA(sd)->resize_obj, "elm,action,stop", "*",
     _button_stop, obj);
   edje_object_signal_callback_add
     (ELM_WIDGET_DATA(sd)->resize_obj, "elm,action,selected", "*",
     _day_selected, obj);

   for (i = 0; i < ELM_DAY_LAST; i++)
     {
        /* FIXME: I'm not aware of a known max, so if it fails,
         * just make it larger. :| */
        char buf[20];
        /* I don't know of a better way of doing it */
        if (strftime(buf, sizeof(buf), "%a", gmtime(&weekday)))
          {
             sd->weekdays[i] = eina_stringshare_add(buf);
          }
        else
          {
             /* If we failed getting day, get a default value */
             sd->weekdays[i] = _days_abbrev[i];
             WRN("Failed getting weekday name for '%s' from locale.",
                 _days_abbrev[i]);
          }
        weekday += 86400; /* Advance by a day */
     }

   current_time = time(NULL);
   localtime_r(&current_time, &sd->shown_time);
   sd->current_time = sd->shown_time;
   sd->selected_time = sd->shown_time;
   t = _time_to_next_day(&sd->current_time);
   sd->update_timer = ecore_timer_add(t, _update_cur_date, obj);

   elm_widget_can_focus_set(obj, EINA_TRUE);

   elm_layout_theme_set(obj, "calendar", "base", elm_object_style_get(obj));
   evas_object_smart_changed(obj);

   // ACCESS
   if ((_elm_config->access_mode != ELM_ACCESS_MODE_OFF))
      _access_calendar_spinner_register(obj);

   //Tizen Only: This should be removed when eo is applied.
   ELM_WIDGET_DATA_GET(obj, wsd);
   wsd->on_create = EINA_FALSE;

   return obj;
}

EAPI void
elm_calendar_weekdays_names_set(Evas_Object *obj,
                                const char *weekdays[])
{
   int i;

   ELM_CALENDAR_CHECK(obj);
   ELM_CALENDAR_DATA_GET(obj, sd);
   EINA_SAFETY_ON_NULL_RETURN(weekdays);

   for (i = 0; i < ELM_DAY_LAST; i++)
     {
        eina_stringshare_replace(&sd->weekdays[i], weekdays[i]);
     }

   evas_object_smart_changed(obj);
}

EAPI const char **
elm_calendar_weekdays_names_get(const Evas_Object *obj)
{
   ELM_CALENDAR_CHECK(obj) NULL;
   ELM_CALENDAR_DATA_GET_OR_RETURN_VAL(obj, sd, NULL);

   return sd->weekdays;
}

EAPI void
elm_calendar_interval_set(Evas_Object *obj,
                          double interval)
{
   ELM_CALENDAR_CHECK(obj);
   ELM_CALENDAR_DATA_GET(obj, sd);

   sd->first_interval = interval;
}

EAPI double
elm_calendar_interval_get(const Evas_Object *obj)
{
   ELM_CALENDAR_CHECK(obj) 0.0;
   ELM_CALENDAR_DATA_GET_OR_RETURN_VAL(obj, sd, 0.0);

   return sd->first_interval;
}

EAPI void
elm_calendar_min_max_year_set(Evas_Object *obj,
                              int min,
                              int max)
{
   ELM_CALENDAR_CHECK(obj);
   ELM_CALENDAR_DATA_GET(obj, sd);

   min -= 1900;
   max -= 1900;
   if ((sd->year_min == min) && (sd->year_max == max)) return;
   sd->year_min = min > 2 ? min : 2;
   if (max > sd->year_min)
     sd->year_max = max;
   else
     sd->year_max = sd->year_min;
   if (sd->shown_time.tm_year > sd->year_max)
     sd->shown_time.tm_year = sd->year_max;
   if (sd->shown_time.tm_year < sd->year_min)
     sd->shown_time.tm_year = sd->year_min;
   evas_object_smart_changed(obj);
}

EAPI void
elm_calendar_min_max_year_get(const Evas_Object *obj,
                              int *min,
                              int *max)
{
   ELM_CALENDAR_CHECK(obj);
   ELM_CALENDAR_DATA_GET(obj, sd);

   if (min) *min = sd->year_min + 1900;
   if (max) *max = sd->year_max + 1900;
}

EINA_DEPRECATED EAPI void
elm_calendar_day_selection_disabled_set(Evas_Object *obj,
                                        Eina_Bool disabled)
{
   ELM_CALENDAR_CHECK(obj);

   if (disabled)
     elm_calendar_select_mode_set(obj, ELM_CALENDAR_SELECT_MODE_NONE);
   else
     elm_calendar_select_mode_set(obj, ELM_CALENDAR_SELECT_MODE_DEFAULT);
}

EINA_DEPRECATED EAPI Eina_Bool
elm_calendar_day_selection_disabled_get(const Evas_Object *obj)
{
   ELM_CALENDAR_CHECK(obj) EINA_FALSE;
   ELM_CALENDAR_DATA_GET_OR_RETURN_VAL(obj, sd, EINA_FALSE);

   return !!(sd->select_mode == ELM_CALENDAR_SELECT_MODE_NONE);
}

EAPI void
elm_calendar_selected_time_set(Evas_Object *obj,
                               struct tm *selected_time)
{
   ELM_CALENDAR_CHECK(obj);
   ELM_CALENDAR_DATA_GET(obj, sd);
   EINA_SAFETY_ON_NULL_RETURN(selected_time);

   if (sd->selectable & ELM_CALENDAR_SELECTABLE_YEAR)
     sd->selected_time.tm_year = selected_time->tm_year;
   if (sd->selectable & ELM_CALENDAR_SELECTABLE_MONTH)
     sd->selected_time.tm_mon = selected_time->tm_mon;
   if (sd->selectable & ELM_CALENDAR_SELECTABLE_DAY)
       {
          sd->selected_time.tm_mday = selected_time->tm_mday;
          if (!sd->selected)
            sd->selected = EINA_TRUE;
       }
   else if (sd->select_mode != ELM_CALENDAR_SELECT_MODE_ONDEMAND)
     {
        if (!sd->selected)
          sd->selected = EINA_TRUE;
     }
   if (sd->selected_time.tm_year != sd->shown_time.tm_year)
     sd->shown_time.tm_year = sd->selected_time.tm_year;
   if (sd->selected_time.tm_mon != sd->shown_time.tm_mon)
     sd->shown_time.tm_mon = sd->selected_time.tm_mon;

   _fix_selected_time(sd);

   evas_object_smart_changed(obj);
}

EAPI Eina_Bool
elm_calendar_selected_time_get(const Evas_Object *obj,
                               struct tm *selected_time)
{
   ELM_CALENDAR_CHECK(obj) EINA_FALSE;
   ELM_CALENDAR_DATA_GET_OR_RETURN_VAL(obj, sd, EINA_FALSE);
   EINA_SAFETY_ON_NULL_RETURN_VAL(selected_time, EINA_FALSE);

   if ((sd->select_mode == ELM_CALENDAR_SELECT_MODE_ONDEMAND)
       && (!sd->selected))
     return EINA_FALSE;
   *selected_time = sd->selected_time;

   return EINA_TRUE;
}

EAPI void
elm_calendar_format_function_set(Evas_Object *obj,
                                 Elm_Calendar_Format_Cb format_function)
{
   ELM_CALENDAR_CHECK(obj);
   ELM_CALENDAR_DATA_GET(obj, sd);

   sd->format_func = format_function;
   _set_month_year(sd);
}

EAPI Elm_Calendar_Mark *
elm_calendar_mark_add(Evas_Object *obj,
                      const char *mark_type,
                      struct tm *mark_time,
                      Elm_Calendar_Mark_Repeat_Type repeat)
{
   ELM_CALENDAR_CHECK(obj) NULL;
   ELM_CALENDAR_DATA_GET_OR_RETURN_VAL(obj, sd, NULL);

   Elm_Calendar_Mark *mark;

   mark = _mark_new(obj, mark_type, mark_time, repeat);
   sd->marks = eina_list_append(sd->marks, mark);
   mark->node = eina_list_last(sd->marks);

   return mark;
}

EAPI void
elm_calendar_mark_del(Elm_Calendar_Mark *mark)
{
   EINA_SAFETY_ON_NULL_RETURN(mark);
   ELM_CALENDAR_CHECK(mark->obj);
   ELM_CALENDAR_DATA_GET(mark->obj, sd);

   sd->marks = eina_list_remove_list(sd->marks, mark->node);
   _mark_free(mark);
}

EAPI void
elm_calendar_marks_clear(Evas_Object *obj)
{
   ELM_CALENDAR_CHECK(obj);
   ELM_CALENDAR_DATA_GET(obj, sd);

   Elm_Calendar_Mark *mark;

   EINA_LIST_FREE (sd->marks, mark)
     _mark_free(mark);
}

EAPI const Eina_List *
elm_calendar_marks_get(const Evas_Object *obj)
{
   ELM_CALENDAR_CHECK(obj) NULL;
   ELM_CALENDAR_DATA_GET(obj, sd);

   return sd->marks;
}

EAPI void
elm_calendar_marks_draw(Evas_Object *obj)
{
   ELM_CALENDAR_CHECK(obj);

   evas_object_smart_changed(obj);
}

EAPI void
elm_calendar_first_day_of_week_set(Evas_Object *obj,
                                   Elm_Calendar_Weekday day)
{
   ELM_CALENDAR_CHECK(obj);
   ELM_CALENDAR_DATA_GET(obj, sd);

   if (day >= ELM_DAY_LAST) return;
   if (sd->first_week_day != day)
     {
        sd->first_week_day = day;
        evas_object_smart_changed(obj);
     }
}

EAPI Elm_Calendar_Weekday
elm_calendar_first_day_of_week_get(const Evas_Object *obj)
{
   ELM_CALENDAR_CHECK(obj) - 1;
   ELM_CALENDAR_DATA_GET(obj, sd);

   return sd->first_week_day;
}

EAPI void
elm_calendar_select_mode_set(Evas_Object *obj,
                             Elm_Calendar_Select_Mode mode)
{
   ELM_CALENDAR_CHECK(obj);
   ELM_CALENDAR_DATA_GET(obj, sd);

   if ((mode <= ELM_CALENDAR_SELECT_MODE_ONDEMAND)
       && (sd->select_mode != mode))
     {
        sd->select_mode = mode;
        if (sd->select_mode == ELM_CALENDAR_SELECT_MODE_ONDEMAND)
          sd->selected = EINA_FALSE;
        if ((sd->select_mode == ELM_CALENDAR_SELECT_MODE_ALWAYS)
            || (sd->select_mode == ELM_CALENDAR_SELECT_MODE_DEFAULT))
          _select(obj, sd->selected_it);
        else
          _unselect(obj, sd->selected_it);
     }
}

EAPI Elm_Calendar_Select_Mode
elm_calendar_select_mode_get(const Evas_Object *obj)
{
   ELM_CALENDAR_CHECK(obj) - 1;
   ELM_CALENDAR_DATA_GET(obj, sd);

   return sd->select_mode;
}

EAPI void
elm_calendar_selectable_set(Evas_Object *obj, Elm_Calendar_Selectable selectable)
{
   ELM_CALENDAR_CHECK(obj);
   ELM_CALENDAR_DATA_GET(obj, sd);

   sd->selectable = selectable;
}

EAPI Elm_Calendar_Selectable
elm_calendar_selectable_get(const Evas_Object *obj)
{
   ELM_CALENDAR_CHECK(obj) -1;
   ELM_CALENDAR_DATA_GET_OR_RETURN_VAL(obj, sd, -1);

   return sd->selectable;
}

EAPI Eina_Bool
elm_calendar_displayed_time_get(const Evas_Object *obj, struct tm *displayed_time)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(displayed_time, EINA_FALSE);
   ELM_CALENDAR_CHECK(obj) EINA_FALSE;
   ELM_CALENDAR_DATA_GET_OR_RETURN_VAL(obj, sd, EINA_FALSE);

   *displayed_time = sd->shown_time;
   return EINA_TRUE;
}

