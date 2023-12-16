#ifdef HAVE_CONFIG_H
#include "elementary_config.h"
#endif

#include <Elementary.h>
#include "elm_priv.h"
#include <unicode/udat.h>
#include <unicode/ustring.h>
#include <unicode/ucnv.h>

#define DATETIME_FIELD_COUNT            6
#define FIELD_FORMAT_LEN                3
#define BUFF_SIZE                       256
#define STRUCT_TM_YEAR_BASE_VALUE       1900
#define STRUCT_TM_MONTH_BASE_VALUE      1
#define BOOST_NAME                      "ELM_TIMEPICKER_BOOST"

typedef struct _Input_Circle_Module_Data Input_Circle_Module_Data;

struct _Input_Circle_Module_Data
{
   Elm_Datetime_Module_Data mod_data;
   Evas_Object *field_obj[DATETIME_FIELD_COUNT];
   Evas_Object *radio_group;
   Eina_Bool is_timepicker;
   Eina_Bool is_am;
   Eina_Bool is_ampm_init;
   Eina_Bool is_ampm_click;
};

static void
_ampm_clicked_cb(void *data, Evas_Object *obj, void *event_info EINA_UNUSED)
{
   Input_Circle_Module_Data *circle_mod;
   struct tm t;

   circle_mod = (Input_Circle_Module_Data *)data;
   if (!circle_mod || !obj) return;

   elm_datetime_value_get(circle_mod->mod_data.base, &t);

   if (t.tm_hour < 12) t.tm_hour += 12;
   else if (t.tm_hour >= 12) t.tm_hour -= 12;

   circle_mod->is_ampm_click = EINA_TRUE;
   elm_datetime_value_set(circle_mod->mod_data.base, &t);
   circle_mod->is_ampm_click = EINA_FALSE;
}

static void
_field_title_string_set(Evas_Object *field, Elm_Datetime_Field_Type type)
{
   switch(type)
     {
      case ELM_DATETIME_YEAR:
        elm_object_domain_translatable_part_text_set(field, "elm.text.title", PACKAGE, E_("WDS_ST_OPT_YEAR_ABB"));
        break;

      case ELM_DATETIME_MONTH:
        elm_object_domain_translatable_part_text_set(field, "elm.text.title", PACKAGE, E_("WDS_ST_OPT_MONTH_ABB"));
        break;

      case ELM_DATETIME_DATE:
        elm_object_domain_translatable_part_text_set(field, "elm.text.title", PACKAGE, E_("WDS_ST_OPT_DAY_ABB"));
        break;

      case ELM_DATETIME_HOUR:
        elm_object_domain_translatable_part_text_set(field, "elm.text.title", PACKAGE, E_("WDS_ST_HEADER_HR_ABB"));
        break;

      case ELM_DATETIME_MINUTE:
        elm_object_domain_translatable_part_text_set(field, "elm.text.title", PACKAGE, E_("WDS_ST_HEADER_MIN_ABB"));
        break;

      default:
        break;
     }
}

static void
_ampm_string_set(Input_Circle_Module_Data *circle_mod, Evas_Object *field, Eina_Bool is_am)
{
   if (!circle_mod || !field) return;

   if (is_am)
     {
        if (circle_mod->is_am)
          elm_object_domain_translatable_text_set(field, PACKAGE, E_("IDS_COM_BODY_AM"));
        else
          {
             elm_object_domain_translatable_part_text_set(field, "elm.text.duplicate", PACKAGE, E_("IDS_COM_BODY_AM"));
             if (circle_mod->is_ampm_click)
               elm_object_signal_emit(field, "elm,action,am,show,animation", "elm");
             else
               elm_object_signal_emit(field, "elm,action,am,show", "elm");
          }
     }
   else
     {
        if (circle_mod->is_am)
          {
             elm_object_domain_translatable_part_text_set(field, "elm.text.duplicate", PACKAGE, E_("IDS_COM_BODY_PM"));
             if (circle_mod->is_ampm_click)
               elm_object_signal_emit(field, "elm,action,pm,show,animation", "elm");
             else
               elm_object_signal_emit(field, "elm,action,pm,show", "elm");
          }
        else
          elm_object_domain_translatable_text_set(field, PACKAGE, E_("IDS_COM_BODY_PM"));
     }
}

static void
_picker_layout_adjust(Input_Circle_Module_Data *circle_mod)
{
   int loc = -1;

   if (!circle_mod) return;

   if (circle_mod->is_timepicker)
     {
        Eina_Bool is_ampm_exist = circle_mod->mod_data.field_location_get(circle_mod->mod_data.base, ELM_DATETIME_AMPM, &loc);
        if (is_ampm_exist)
          {
             elm_layout_signal_emit(circle_mod->mod_data.base, "elm,action,ampm,show", "elm");

             if (loc == 5)
               elm_layout_signal_emit(circle_mod->mod_data.base, "elm,action,colon,front", "elm");
             else
               elm_layout_signal_emit(circle_mod->mod_data.base, "elm,action,colon,back", "elm");
          }
        else
          elm_layout_signal_emit(circle_mod->mod_data.base, "elm,action,ampm,hide", "elm");
     }
   else
     {
        char buf[BUFF_SIZE];

        circle_mod->mod_data.field_location_get(circle_mod->mod_data.base, ELM_DATETIME_YEAR, &loc);
        snprintf(buf, sizeof(buf), "elm,state,field%d,year", loc);
        elm_layout_signal_emit(circle_mod->mod_data.base, buf, "elm");

        circle_mod->mod_data.field_location_get(circle_mod->mod_data.base, ELM_DATETIME_MONTH, &loc);
        snprintf(buf, sizeof(buf), "elm,state,field%d,month", loc);
        elm_layout_signal_emit(circle_mod->mod_data.base, buf, "elm");

        circle_mod->mod_data.field_location_get(circle_mod->mod_data.base, ELM_DATETIME_DATE, &loc);
        snprintf(buf, sizeof(buf), "elm,state,field%d,date", loc);
        elm_layout_signal_emit(circle_mod->mod_data.base, buf, "elm");
     }

   edje_object_message_signal_process(elm_layout_edje_get(circle_mod->mod_data.base));
}

static Evas_Object *
_first_radio_get(Input_Circle_Module_Data *circle_mod)
{
   Elm_Datetime_Field_Type field_type;
   int idx = 0, begin, end, min_loc = ELM_DATETIME_LAST, loc;

   if (!circle_mod) return NULL;

   if (circle_mod->is_timepicker)
     {
        begin = ELM_DATETIME_HOUR;
        end = ELM_DATETIME_MINUTE;
     }
   else
     {
        begin = ELM_DATETIME_YEAR;
        end = ELM_DATETIME_DATE;
     }

   for (idx = begin; idx <= end; idx++)
     {
        if (!circle_mod->mod_data.field_location_get(circle_mod->mod_data.base, idx, &loc)) continue;

        if (loc < min_loc)
          {
             min_loc = loc;
             field_type = idx;
          }
     }

   if (min_loc == ELM_DATETIME_LAST)
     return NULL;
   else
     return circle_mod->field_obj[field_type];
}

static void
_first_field_select(Input_Circle_Module_Data *circle_mod)
{
   Evas_Object *first_obj;
   int value;

   if (!circle_mod) return;

   first_obj = _first_radio_get(circle_mod);
   if (!first_obj) return;

   value = elm_radio_state_value_get(first_obj);
   elm_radio_value_set(circle_mod->radio_group, value);

   evas_object_smart_callback_call(first_obj, "changed", circle_mod->mod_data.base);
}

static void
_theme_changed_cb(void *data, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Input_Circle_Module_Data *circle_mod;
   const char *style;

   circle_mod = (Input_Circle_Module_Data *)data;
   if (!circle_mod) return;

   style = elm_object_style_get(circle_mod->mod_data.base);

   if (!strcmp(style, "timepicker_layout") || !strcmp(style, "timepicker/circle"))
     circle_mod->is_timepicker = EINA_TRUE;
   else
     circle_mod->is_timepicker = EINA_FALSE;

   _picker_layout_adjust(circle_mod);
   _first_field_select(circle_mod);
}

static Eina_Bool
_field_access_clicked_cb(void *data, Evas_Object *obj, Elm_Access_Action_Info *action_info)
{
   Elm_Datetime_Field_Type field_type = (Elm_Datetime_Field_Type)evas_object_data_get(obj, "_field_type");

   if (field_type == ELM_DATETIME_AMPM)
     {
        _elm_access_text_set(_elm_access_object_get(obj), ELM_ACCESS_INFO,
            elm_object_part_text_get(obj, "elm.text.duplicate"));
        _elm_access_text_set(_elm_access_object_get(obj), ELM_ACCESS_TYPE, E_("WDS_TTS_TBBODY_DOUBLE_TAP_TO_CHANGE"));
     }
   else
     {
        Eina_Strbuf *buf = NULL;
        const char* field;

        _elm_access_text_set(_elm_access_object_get(obj), ELM_ACCESS_INFO, elm_object_text_get(obj));
        _elm_access_text_set(_elm_access_object_get(obj), ELM_ACCESS_TYPE, E_("WDS_TTS_TBOPT_PICKER"));

        switch(field_type)
          {
             case ELM_DATETIME_YEAR:
               field = E_("WDS_ST_OPT_YEAR_ABB");
               break;

             case ELM_DATETIME_MONTH:
               field = E_("WDS_ST_OPT_MONTH_ABB");
               break;

             case ELM_DATETIME_DATE:
               field = E_("WDS_ST_OPT_DAY_ABB");
               break;

             case ELM_DATETIME_HOUR:
               field = E_("WDS_ST_HEADER_HR_ABB");
               break;

             case ELM_DATETIME_MINUTE:
               field = E_("WDS_ST_HEADER_MIN_ABB");
               break;

             default:
               break;
          }

        buf = eina_strbuf_new();
        eina_strbuf_append_printf(buf, E_("WDS_TTS_TBBODY_ROTATE_THE_P1SS_TO_ADJUST_P2SS"), "bezel", field);//[String]Change to WDS after bezel is translated.
        _elm_access_text_set(_elm_access_object_get(obj), ELM_ACCESS_STATE, eina_strbuf_string_get(buf));
        eina_strbuf_free(buf);

        elm_radio_value_set(obj, elm_radio_state_value_get(obj));
        evas_object_smart_callback_call(obj, "changed", NULL);
     }

   return EINA_FALSE;
}

EAPI void
field_format_changed(Elm_Datetime_Module_Data *module_data, Evas_Object *obj)
{
   Input_Circle_Module_Data *circle_mod;
   Elm_Datetime_Field_Type field_type;

   circle_mod = (Input_Circle_Module_Data *)module_data;
   if (!circle_mod || !obj) return;

   field_type = (Elm_Datetime_Field_Type)evas_object_data_get(obj, "_field_type");

   switch(field_type)
     {
        case ELM_DATETIME_YEAR:
          _picker_layout_adjust(circle_mod);
          _first_field_select(circle_mod);

        case ELM_DATETIME_MONTH:
        case ELM_DATETIME_DATE:
        case ELM_DATETIME_HOUR:
        case ELM_DATETIME_MINUTE:
          _field_title_string_set(obj, field_type);
          break;

        default:
          break;
     }
}

EAPI void
field_value_display(Elm_Datetime_Module_Data *module_data, Evas_Object *obj)
{
   Input_Circle_Module_Data *circle_mod;
   Elm_Datetime_Field_Type field_type;
   struct tm tim;
   char buf[BUFF_SIZE] = {0};
   const char *fmt;
   char strbuf[BUFF_SIZE];
   char *locale_tmp;
   char *p = NULL;
   char locale[BUFF_SIZE] = {0,};
   int val;
   UDateFormat *dt_formatter = NULL;
   UErrorCode status = U_ZERO_ERROR;
   UChar str[BUFF_SIZE];
   UChar pattern[BUFF_SIZE] = {0, };
   UChar ufield[BUFF_SIZE] = {0, };
   UDate date;
   UCalendar *calendar;
   int32_t pos;

   circle_mod = (Input_Circle_Module_Data *)module_data;
   if (!circle_mod || !obj) return;

   elm_datetime_value_get(circle_mod->mod_data.base, &tim);

   field_type = (Elm_Datetime_Field_Type)evas_object_data_get(obj, "_field_type");
   fmt = circle_mod->mod_data.field_format_get(circle_mod->mod_data.base, field_type);

   locale_tmp = setlocale(LC_MESSAGES, NULL);
   strcpy(locale, locale_tmp);

   if (locale[0] != '\0')
     {
        p = strstr(locale, ".UTF-8");
        if (p) *p = 0;
     }

   switch (field_type)
     {
      case ELM_DATETIME_YEAR:
        if (circle_mod->is_timepicker) return;

        if (!strcmp(fmt, "%y"))
          u_uastrcpy(pattern, "yy");
        else
          u_uastrcpy(pattern, "yyyy");

        val = STRUCT_TM_YEAR_BASE_VALUE + tim.tm_year;
        break;

      case ELM_DATETIME_MONTH:
        if (circle_mod->is_timepicker) return;

        if (!strcmp(fmt, "%m"))
          u_uastrcpy(pattern, "MM");
        else if (!strcmp(fmt, "%B"))
          u_uastrcpy(pattern, "MMMM");
        else
          u_uastrcpy(pattern, "MMM");

        val = STRUCT_TM_MONTH_BASE_VALUE + tim.tm_mon;
        break;

      case ELM_DATETIME_DATE:
        if (circle_mod->is_timepicker) return;

        if (!strcmp(fmt, "%e"))
          u_uastrcpy(pattern, "d");
        else
          u_uastrcpy(pattern, "dd");

        val = tim.tm_mday;
        break;

      case ELM_DATETIME_HOUR:
        if (!circle_mod->is_timepicker) return;

        if (!strcmp(fmt, "%l"))
          {
             if (!strcmp(locale,"ja_JP"))
               u_uastrcpy(pattern, "K");
             else
               u_uastrcpy(pattern, "h");
          }
        else if (!strcmp(fmt, "%I"))
          {
             if (!strcmp(locale,"ja_JP"))
               u_uastrcpy(pattern, "KK");
             else
               u_uastrcpy(pattern, "hh");
          }
        else if (!strcmp(fmt, "%k"))
          u_uastrcpy(pattern, "H");
        else
          u_uastrcpy(pattern, "HH");

        val = tim.tm_hour;
        break;

      case ELM_DATETIME_MINUTE:
        if (!circle_mod->is_timepicker) return;

        u_uastrcpy(pattern, "mm");
        val = tim.tm_min;
        break;

      case ELM_DATETIME_AMPM:
        if (!circle_mod->is_timepicker) return;

        if (!circle_mod->is_ampm_init)
          {
             circle_mod->is_ampm_init = EINA_TRUE;

             if (tim.tm_hour < 12)
               circle_mod->is_am = EINA_FALSE;
             else
               circle_mod->is_am = EINA_TRUE;
          }

        if (tim.tm_hour < 12)
          {
             _ampm_string_set(circle_mod, obj, EINA_TRUE);
             circle_mod->is_am = EINA_TRUE;
          }
        else
          {
             _ampm_string_set(circle_mod, obj, EINA_FALSE);
             circle_mod->is_am = EINA_FALSE;
          }
        return;

      default:
        return;
     }

   snprintf(strbuf, sizeof(strbuf), "%d", val);
   u_uastrcpy(ufield, strbuf);
   pos = 0;

   dt_formatter = udat_open(UDAT_IGNORE, UDAT_IGNORE, locale, NULL, -1, pattern, -1, &status);
   calendar = ucal_open(NULL, -1, locale, UCAL_GREGORIAN, &status);
   ucal_clear(calendar);

   udat_parseCalendar(dt_formatter, calendar, ufield, sizeof(ufield), &pos, &status);
   date = ucal_getMillis(calendar, &status);
   ucal_close(calendar);

   udat_format(dt_formatter, date, str, sizeof(str), NULL, &status);
   u_austrcpy(buf, str);
   udat_close(dt_formatter);

   elm_object_text_set(obj, buf);
}

EAPI Evas_Object *
field_create(Elm_Datetime_Module_Data *module_data, Elm_Datetime_Field_Type field_type)
{
   Input_Circle_Module_Data *circle_mod;
   Evas_Object *field_obj = NULL;
   circle_mod = (Input_Circle_Module_Data *)module_data;
   if (!circle_mod) return NULL;

   switch(field_type)
     {
      case ELM_DATETIME_YEAR:
      case ELM_DATETIME_DATE:
        field_obj = elm_radio_add(circle_mod->mod_data.base);
        if (!evas_object_data_get(elm_object_parent_widget_get(circle_mod->mod_data.base),
            BOOST_NAME))
          elm_object_style_set(field_obj, "datepicker");

        elm_radio_state_value_set(field_obj, field_type);

        if (!circle_mod->radio_group)
          circle_mod->radio_group = field_obj;
        else
          elm_radio_group_add(field_obj, circle_mod->radio_group);
        break;

      case ELM_DATETIME_MONTH:
        field_obj = elm_radio_add(circle_mod->mod_data.base);
        if (!evas_object_data_get(elm_object_parent_widget_get(circle_mod->mod_data.base),
            BOOST_NAME))
          elm_object_style_set(field_obj, "datepicker_month");

        elm_radio_state_value_set(field_obj, field_type);

        if (!circle_mod->radio_group)
          circle_mod->radio_group = field_obj;
        else
          elm_radio_group_add(field_obj, circle_mod->radio_group);
        break;

      case ELM_DATETIME_HOUR:
      case ELM_DATETIME_MINUTE:
        field_obj = elm_radio_add(circle_mod->mod_data.base);
        elm_object_style_set(field_obj, "timepicker");

        elm_radio_state_value_set(field_obj, field_type);

        if (!circle_mod->radio_group)
          circle_mod->radio_group = field_obj;
        else
          elm_radio_group_add(field_obj, circle_mod->radio_group);
        break;

      case ELM_DATETIME_AMPM:
        field_obj = elm_button_add(circle_mod->mod_data.base);
        elm_object_style_set(field_obj, "datetime/ampm");

        evas_object_smart_callback_add(field_obj, "clicked", _ampm_clicked_cb, circle_mod);
        break;

      default:
        return NULL;
     }

   evas_object_data_set(field_obj, "_field_type", (void *)field_type);

   circle_mod->field_obj[field_type] = field_obj;

   elm_access_action_cb_set(circle_mod->field_obj[field_type], ELM_ACCESS_ACTION_HIGHLIGHT, _field_access_clicked_cb, NULL);

   return field_obj;
}

EAPI Elm_Datetime_Module_Data *
obj_hook(Evas_Object *obj)
{
   Input_Circle_Module_Data *circle_mod;
   circle_mod = ELM_NEW(Input_Circle_Module_Data);
   if (!circle_mod) return NULL;
   circle_mod->radio_group = NULL;
   circle_mod->is_timepicker = EINA_FALSE;
   circle_mod->is_am = EINA_FALSE;
   circle_mod->is_ampm_init = EINA_FALSE;
   circle_mod->is_ampm_click = EINA_FALSE;

   evas_object_smart_callback_add(obj, "theme,changed", _theme_changed_cb, circle_mod);

   return ((Elm_Datetime_Module_Data*)circle_mod);
}

EAPI void
obj_unhook(Elm_Datetime_Module_Data *module_data EINA_UNUSED)
{
   Input_Circle_Module_Data *circle_mod;

   circle_mod = (Input_Circle_Module_Data *)module_data;
   if (!circle_mod) return;

   if (circle_mod)
     {
        free(circle_mod);
        circle_mod = NULL;
     }
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
