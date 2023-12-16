#include <Elementary.h>
#include "elm_priv.h"
#include "elm_widget_spinner.h"

EAPI const char ELM_SPINNER_SMART_NAME[] = "elm_spinner";

static const char SIG_CHANGED[] = "changed";
static const char SIG_DELAY_CHANGED[] = "delay,changed";
static const char SIG_LANG_CHANGED[] = "language,changed";
static const char SIG_MAX_VAL[] = "max,reached";
static const char SIG_MIN_VAL[] = "min,reached";
static const char SIG_ENTRY_CHANGED[] = "entry,changed";
static const char SIG_ENTRY_SHOW[] = "entry,show";
/*Tizen only datetime module needed this signal to avoid
 double changed in AM_PM field */
static const char SIG_ENTRY_VAL_APPLY[] = "entry,apply";

static const Evas_Smart_Cb_Description _smart_callbacks[] = {
   {SIG_CHANGED, ""},
   {SIG_DELAY_CHANGED, ""},
   {SIG_LANG_CHANGED, ""},
   {SIG_MAX_VAL, ""},
   {SIG_MIN_VAL, ""},
   {SIG_ENTRY_VAL_APPLY, ""},
   {SIG_ENTRY_CHANGED, ""},
   {SIG_ENTRY_SHOW, ""},
   {NULL, NULL}
};

EVAS_SMART_SUBCLASS_NEW
  (ELM_SPINNER_SMART_NAME, _elm_spinner, Elm_Spinner_Smart_Class,
  Elm_Layout_Smart_Class, elm_layout_smart_class_get, _smart_callbacks);

static void _access_increment_decrement_info_say(Evas_Object *obj,
                                                 Eina_Bool is_incremented);


static void
_format_parse(const char *str, char *fmt)
{
   const char *start = strchr(str, '%');
   while (start)
    {
       /* handle %% */
       if (start[1] != '%')
         break;
       else
         start = strchr(start + 2, '%');
    }

   if (start)
    {
       const char *itr, *end = NULL;
       for (itr = start + 1; *itr != '\0'; itr++)
         {
            /* allowing '%d' is quite dangerous, remove it? */
            if ((*itr == 'd') || (*itr == 'f'))
              {
                 end = itr + 1;
                 break;
              }
         }

       if (end)
         {
            memcpy(fmt, start, end - start);
            fmt[end - start] = '\0';
         }
    }
}

/*Added support for %d format also since in some language
  number format is not appropriate.*/
static Eina_Bool
_check_label_format(const char *fmt)
{
   const char *start = fmt;
   const char *itr;
   for (itr = start + 1; *itr != '\0'; itr++)
     {
        if ((*itr == 'd'))
          return EINA_FALSE;
        else if((*itr == 'f'))
          return EINA_TRUE;
     }
   return EINA_TRUE;
}

static void
_entry_show(Elm_Spinner_Smart_Data *sd)
{
   char buf[32], fmt[32] = "%0.f";

   /* try to construct just the format from given label
    * completely ignoring pre/post words
    */
   if (sd->label)
     {
        _format_parse(sd->label, fmt);
     }
   if (_check_label_format(fmt))
     snprintf(buf, sizeof(buf), fmt, sd->val);
   else
     snprintf(buf, sizeof(buf), fmt, (int)sd->val);
   elm_object_text_set(sd->ent, buf);
}

static void
_label_write(Evas_Object *obj)
{
   Eina_List *l;
   char buf[1024];
   Elm_Spinner_Special_Value *sv;

   ELM_SPINNER_DATA_GET(obj, sd);

   EINA_LIST_FOREACH(sd->special_values, l, sv)
     {
        if (sv->value == sd->val)
          {
             snprintf(buf, sizeof(buf), "%s", sv->label);
             goto apply;
          }
     }

   if (sd->label)
     {
        if (_check_label_format(sd->label))
          snprintf(buf, sizeof(buf), sd->label, sd->val);
        else
          snprintf(buf, sizeof(buf), sd->label, (int)sd->val);
     }
   else
     snprintf(buf, sizeof(buf), "%.0f", sd->val);

apply:
   elm_object_text_set(sd->text_button, buf);
   if (sd->entry_visible) _entry_show(sd);
}

static Eina_Bool
_elm_spinner_smart_translate(Evas_Object *obj)
{
   // TIZEN_ONLY(20140312): Fixed to translate value in spinner when system language is changed.
   _label_write(obj);
   //
   evas_object_smart_callback_call(obj, SIG_LANG_CHANGED, NULL);
   return EINA_TRUE;
}

static Eina_Bool
_delay_change(void *data)
{
   ELM_SPINNER_DATA_GET(data, sd);

   sd->delay = NULL;
   evas_object_smart_callback_call(data, SIG_DELAY_CHANGED, NULL);

   return ECORE_CALLBACK_CANCEL;
}

static Eina_Bool
_value_set(Evas_Object *obj,
           double new_val)
{
   ELM_SPINNER_DATA_GET(obj, sd);

   if (sd->round > 0)
     new_val = sd->val_base +
       (double)((((int)(new_val - sd->val_base)) / sd->round) * sd->round);

   if (sd->wrap)
     {
        if (new_val < sd->val_min)
          new_val = sd->val_max;
        else if (new_val > sd->val_max)
          new_val = sd->val_min;
     }
   else
     {
        if (new_val < sd->val_min)
          {
             new_val = sd->val_min;
             evas_object_smart_callback_call(obj, SIG_MIN_VAL, NULL);
          }
        else if (new_val > sd->val_max)
          {
             new_val = sd->val_max;
             evas_object_smart_callback_call(obj, SIG_MAX_VAL, NULL);
          }
     }

   if (new_val == sd->val) return EINA_FALSE;
   sd->val = new_val;

   evas_object_smart_callback_call(obj, SIG_CHANGED, NULL);
   if (sd->delay) ecore_timer_del(sd->delay);
   sd->delay = ecore_timer_add(0.2, _delay_change, obj);

   return EINA_TRUE;
}

static void
_drag_val_set(Evas_Object *obj)
{
   double pos = 0.0;

   ELM_SPINNER_DATA_GET(obj, sd);

   if (sd->val_max > sd->val_min)
     pos = ((sd->val - sd->val_min) / (sd->val_max - sd->val_min));
   if (pos < 0.0) pos = 0.0;
   else if (pos > 1.0)
     pos = 1.0;
   edje_object_part_drag_value_set
     (ELM_WIDGET_DATA(sd)->resize_obj, "elm.dragable.slider", pos, pos);
}

static void
_drag_cb(void *data,
         Evas_Object *_obj __UNUSED__,
         const char *emission __UNUSED__,
         const char *source __UNUSED__)
{
   double pos = 0.0, offset, delta;
   Evas_Object *obj = data;

   ELM_SPINNER_DATA_GET(obj, sd);

   if (sd->entry_visible) return;
   if (!strcmp(elm_widget_style_get(obj), "vertical"))
     edje_object_part_drag_value_get
        (ELM_WIDGET_DATA(sd)->resize_obj, "elm.dragable.slider", NULL, &pos);
   else
     edje_object_part_drag_value_get
        (ELM_WIDGET_DATA(sd)->resize_obj, "elm.dragable.slider", &pos, NULL);

   offset = sd->step * _elm_config->scale;
   delta = (pos - sd->drag_start_pos) * offset;
   /* If we are on rtl mode, change the delta to be negative on such changes */
   if (elm_widget_mirrored_get(obj)) delta *= -1;
   if (_value_set(data, sd->drag_start_pos + delta)) _label_write(data);
   sd->dragging = 1;
}

static void
_drag_start_cb(void *data,
               Evas_Object *obj __UNUSED__,
               const char *emission __UNUSED__,
               const char *source __UNUSED__)
{
   double pos;

   ELM_SPINNER_DATA_GET(data, sd);

   if (!strcmp(elm_widget_style_get(obj), "vertical"))
     edje_object_part_drag_value_get
        (ELM_WIDGET_DATA(sd)->resize_obj, "elm.dragable.slider", NULL, &pos);
   else
     edje_object_part_drag_value_get
        (ELM_WIDGET_DATA(sd)->resize_obj, "elm.dragable.slider", &pos, NULL);
   sd->drag_start_pos = pos;
}

static void
_drag_stop_cb(void *data,
              Evas_Object *obj __UNUSED__,
              const char *emission __UNUSED__,
              const char *source __UNUSED__)
{
   ELM_SPINNER_DATA_GET(data, sd);

   sd->drag_start_pos = 0;
   edje_object_part_drag_value_set
     (ELM_WIDGET_DATA(sd)->resize_obj, "elm.dragable.slider", 0.0, 0.0);
}

static void
_entry_hide(Evas_Object *obj)
{
   ELM_SPINNER_DATA_GET(obj, sd);

   elm_layout_signal_emit(obj, "elm,state,inactive", "elm");
   sd->entry_visible = EINA_FALSE;
}

static void
_entry_value_apply(Evas_Object *obj)
{
   const char *str;
   double val;
   char *end;
   Elm_Spinner_Special_Value *sv;
   Eina_List *l;
   Eina_Bool is_special = EINA_FALSE;

   ELM_SPINNER_DATA_GET(obj, sd);

   if (!sd->entry_visible) return;

   _entry_hide(obj);
   str = elm_object_text_get(sd->ent);
   if (!str) return;

   val = strtod(str, &end);
   evas_object_smart_callback_call(obj, SIG_ENTRY_VAL_APPLY, &val);
   elm_spinner_value_set(obj, val);
}

static void _create_entry(Evas_Object *obj);

static void
_toggle_entry(Evas_Object *obj)
{
   ELM_SPINNER_DATA_GET(obj, sd);

   _create_entry(obj);

   if (sd->dragging)
     {
        sd->dragging = 0;
        return;
     }
   if (elm_widget_disabled_get(obj)) return;
   if (!sd->editable) return;
   if (sd->entry_visible) _entry_value_apply(obj);
   else
     elm_layout_signal_emit(obj, "elm,state,active", "elm");
}

static void
_text_button_clicked_cb(void *data,
                        Evas_Object *obj __UNUSED__,
                        void *event_info __UNUSED__)
{
   _toggle_entry(data);
}

static void
_entry_toggle_cb(void *data,
                 Evas_Object *obj __UNUSED__,
                 const char *emission __UNUSED__,
                 const char *source __UNUSED__)
{
   _toggle_entry(data);
}

static void
_entry_show_cb(void *data,
               Evas *e __UNUSED__,
               Evas_Object *obj,
               void *event_info __UNUSED__)
{
   ELM_SPINNER_DATA_GET(data, sd);
   evas_object_smart_callback_call(data, SIG_ENTRY_SHOW, NULL);
   _entry_show(sd);
   elm_object_focus_set(obj, EINA_TRUE);
   elm_entry_select_all(obj);
   sd->entry_visible = EINA_TRUE;
}

static Eina_Bool
_spin_value(void *data)
{
   ELM_SPINNER_DATA_GET(data, sd);
   double real_speed = sd->spin_speed;

   /* Sanity check: our step size should be at least as
    * large as our rounding value
    */
   if ((sd->spin_speed != 0.0) && (abs(sd->spin_speed) < sd->round))
     {
        WRN("The spinning step is smaller than the rounding value, please check your code");
        real_speed = sd->spin_speed > 0 ? sd->round : -sd->round;
     }

   if (_value_set(data, sd->val + real_speed)) _label_write(data);

   return ECORE_CALLBACK_RENEW;
}

static Eina_Bool
_val_inc_start(void *data)
{
   ELM_SPINNER_DATA_GET(data, sd);

   sd->spin_speed = sd->step;
   sd->longpress_timer = NULL;
   elm_layout_signal_emit(data, "elm,action,longpress", "elm");
   if (sd->spin) ecore_timer_del(sd->spin);
   sd->spin = ecore_timer_add(sd->interval, _spin_value, data);
   _spin_value(data);
   return ECORE_CALLBACK_CANCEL;
}

static void
_val_inc_stop(Evas_Object *obj)
{
   ELM_SPINNER_DATA_GET(obj, sd);

   sd->spin_speed = 0;
   if (sd->spin) ecore_timer_del(sd->spin);
   sd->spin = NULL;
}

static Eina_Bool
_val_dec_start(void *data)
{
   ELM_SPINNER_DATA_GET(data, sd);

   sd->spin_speed = -sd->step;
   sd->longpress_timer = NULL;
   elm_layout_signal_emit(data, "elm,action,longpress", "elm");
   if (sd->spin) ecore_timer_del(sd->spin);
   sd->spin = ecore_timer_add(sd->interval, _spin_value, data);
   _spin_value(data);
   return ECORE_CALLBACK_CANCEL;
}

static void
_val_dec_stop(Evas_Object *obj)
{
   ELM_SPINNER_DATA_GET(obj, sd);

   sd->spin_speed = 0;
   if (sd->spin) ecore_timer_del(sd->spin);
   sd->spin = NULL;
}

static void
_inc_button_clicked_cb(void *data,
                       Evas_Object *obj __UNUSED__,
                       void *event_info __UNUSED__)
{
   ELM_SPINNER_DATA_GET(data, sd);

   _val_inc_stop(data);
   sd->spin_speed = sd->step;
   _spin_value(data);

   if (sd->entry_visible) _entry_value_apply(data);
   if (_elm_config->access_mode)
     _access_increment_decrement_info_say(data, EINA_TRUE);
}

static void
_inc_button_pressed_cb(void *data,
                       Evas_Object *obj __UNUSED__,
                       void *event_info __UNUSED__)
{
   ELM_SPINNER_DATA_GET(data, sd);

   if (sd->longpress_timer) ecore_timer_del(sd->longpress_timer);
   sd->longpress_timer = ecore_timer_add
                           (_elm_config->longpress_timeout,
                            _val_inc_start, data);

   if (sd->entry_visible) _entry_value_apply(data);
}

static void
_inc_button_unpressed_cb(void *data,
                         Evas_Object *obj __UNUSED__,
                         void *event_info __UNUSED__)
{
   ELM_SPINNER_DATA_GET(data, sd);

   if (sd->longpress_timer)
     {
        ecore_timer_del(sd->longpress_timer);
        sd->longpress_timer = NULL;
     }
   _val_inc_stop(data);
}

static void
_dec_button_clicked_cb(void *data,
                       Evas_Object *obj __UNUSED__,
                       void *event_info __UNUSED__)
{
   ELM_SPINNER_DATA_GET(data, sd);

   _val_dec_stop(data);
   sd->spin_speed = -sd->step;
   _spin_value(data);

   if (sd->entry_visible) _entry_value_apply(data);

   if (_elm_config->access_mode)
     _access_increment_decrement_info_say(data, EINA_FALSE);
}

static void
_dec_button_pressed_cb(void *data,
                       Evas_Object *obj __UNUSED__,
                       void *event_info __UNUSED__)
{
   ELM_SPINNER_DATA_GET(data, sd);

   if (sd->longpress_timer) ecore_timer_del(sd->longpress_timer);
   sd->longpress_timer = ecore_timer_add
                           (_elm_config->longpress_timeout,
                            _val_dec_start, data);

   if (sd->entry_visible) _entry_value_apply(data);
}

static void
_dec_button_unpressed_cb(void *data,
                         Evas_Object *obj __UNUSED__,
                         void *event_info __UNUSED__)
{
   ELM_SPINNER_DATA_GET(data, sd);

   if (sd->longpress_timer)
     {
        ecore_timer_del(sd->longpress_timer);
        sd->longpress_timer = NULL;
     }
   _val_dec_stop(data);
}

static void
_entry_activated_cb(void *data,
                    Evas_Object *obj __UNUSED__,
                    void *event_info __UNUSED__)
{
   ELM_SPINNER_DATA_GET(data, sd);

   _entry_value_apply(data);
   evas_object_smart_callback_call(data, SIG_CHANGED, NULL);
   if (sd->delay) ecore_timer_del(sd->delay);
   sd->delay = ecore_timer_add(0.2, _delay_change, data);
}

static void
_elm_spinner_smart_sizing_eval(Evas_Object *obj)
{
   Evas_Coord minw = -1, minh = -1;

   ELM_SPINNER_DATA_GET(obj, sd);

   elm_coords_finger_size_adjust(1, &minw, 1, &minh);
   edje_object_size_min_restricted_calc
     (ELM_WIDGET_DATA(sd)->resize_obj, &minw, &minh, minw, minh);
   elm_coords_finger_size_adjust(1, &minw, 1, &minh);
   evas_object_size_hint_min_set(obj, minw, minh);
   evas_object_size_hint_max_set(obj, -1, -1);
}

static Eina_Bool
_elm_spinner_smart_on_focus(Evas_Object *obj, Elm_Focus_Info *info)
{
   ELM_SPINNER_DATA_GET(obj, sd);

   if (!ELM_WIDGET_CLASS(_elm_spinner_parent_sc)->on_focus(obj, info))
     return EINA_FALSE;

   if (!elm_widget_focus_get(obj))
     {
        if (sd->delay)
          {
             ecore_timer_del(sd->delay);
             sd->delay = NULL;
          }
        if (sd->spin)
          {
             ecore_timer_del(sd->spin);
             sd->spin = NULL;
          }

        if (sd->entry_visible && !evas_focus_state_get(evas_object_evas_get(obj)))
          sd->entry_was_visible = EINA_TRUE;

        _entry_value_apply(obj);
        //When changed spinner focus state, then text_button changed focus state same.
        elm_layout_signal_emit(sd->text_button, "elm,state,unselected", "elm");
     }
   else
     {
        elm_layout_signal_emit(sd->text_button, "elm,state,selected", "elm");

        if (sd->entry_was_visible)
          {
             _toggle_entry(obj);
             sd->entry_was_visible = EINA_FALSE;
          }
     }

   return EINA_TRUE;
}

static char *
_access_info_cb(void *data, Evas_Object *obj __UNUSED__)
{
   Evas_Object *spinner;
   const char *txt = NULL;

   spinner = (Evas_Object *)(data);
   ELM_SPINNER_DATA_GET(spinner, sd);

   if (sd->entry_visible)
     txt = elm_object_text_get(sd->ent);
   else
     txt = elm_object_text_get(sd->text_button);
   if (txt) return strdup(txt);

   return NULL;
}

static char *
_access_state_cb(void *data, Evas_Object *obj __UNUSED__)
{
   if (elm_widget_disabled_get(data))
     return strdup(E_("IDS_ACCS_BODY_DISABLED_TTS"));

   return NULL;
}

static void
_access_activate_spinner_cb(void *data,
                            Evas_Object *part_obj __UNUSED__,
                            Elm_Object_Item *item __UNUSED__)
{
   ELM_SPINNER_DATA_GET(data, sd);

   if (!sd->entry_visible)
     _toggle_entry(data);
}

static void
_access_increment_decrement_info_say(Evas_Object *obj,
                                     Eina_Bool is_incremented)
{
   char *text;
   Eina_Strbuf *buf;

   ELM_SPINNER_DATA_GET(obj, sd);

   buf = eina_strbuf_new();
   if (!buf) return;
   if (is_incremented)
     {
         elm_object_signal_emit
            (sd->inc_button, "elm,action,anim,activate", "elm");
         eina_strbuf_append(buf, E_("incremented"));
     }
   else
     {
         elm_object_signal_emit
            (sd->dec_button, "elm,action,anim,activate", "elm");
         eina_strbuf_append(buf, E_("decremented"));
     }

   eina_strbuf_append_printf
      (buf, "%s", elm_object_text_get(sd->text_button));

   text = eina_strbuf_string_steal(buf);
   eina_strbuf_free(buf);
   _elm_access_say(obj, text, EINA_FALSE);
}

static void
_access_spinner_register(Evas_Object *obj, Eina_Bool is_access)
{
   Evas_Object *ao;
   Elm_Access_Info *ai;

   ELM_SPINNER_DATA_GET(obj, sd);

   if (!is_access)
     {
        /* unregister access */
       _elm_access_edje_object_part_object_unregister
         (obj, elm_layout_edje_get(obj), "access");
        elm_layout_signal_emit(obj, "elm,state,access,inactive", "elm");
        return;
     }
   elm_layout_signal_emit(obj, "elm,state,access,active", "elm");
   ao = _elm_access_edje_object_part_object_register
          (obj, elm_layout_edje_get(obj), "access");

   ai = _elm_access_object_get(ao);
   _elm_access_text_set(ai, ELM_ACCESS_TYPE, E_("IDS_CLOCK_BUTTON_SPINNER"));
   _elm_access_callback_set(ai, ELM_ACCESS_STATE, _access_state_cb, obj);
   _elm_access_activate_callback_set(ai, _access_activate_spinner_cb, obj);

   /*Do not register spinner buttons if widget is disabled*/
   if (!elm_widget_disabled_get(obj))
     {
        ai = _elm_access_object_get(sd->inc_button);
        _elm_access_text_set(ai, ELM_ACCESS_TYPE,
                             E_("spinner increment button"));

        ai = _elm_access_object_get(sd->dec_button);
        _elm_access_text_set(ai, ELM_ACCESS_TYPE,
                             E_("spinner decrement button"));

        ai = _elm_access_object_get(sd->text_button);
        _elm_access_text_set(ai, ELM_ACCESS_TYPE, E_("spinner text"));//[string:error] not included in po file.
        _elm_access_callback_set(ai, ELM_ACCESS_INFO, _access_info_cb, obj);
     }
}

static Eina_Bool
_elm_spinner_smart_disable(Evas_Object *obj)
{
   if (!ELM_WIDGET_CLASS(_elm_spinner_parent_sc)->disable(obj))
     return EINA_FALSE;
   if (_elm_config->access_mode)
     _access_spinner_register(obj, EINA_TRUE);

   return EINA_TRUE;
}

static void
_elm_spinner_smart_add(Evas_Object *obj)
{
   EVAS_SMART_DATA_ALLOC(obj, Elm_Spinner_Smart_Data);

   ELM_WIDGET_CLASS(_elm_spinner_parent_sc)->base.add(obj);
}

static void
_elm_spinner_smart_del(Evas_Object *obj)
{
   Elm_Spinner_Special_Value *sv;

   ELM_SPINNER_DATA_GET(obj, sd);

   if (sd->label) eina_stringshare_del(sd->label);
   if (sd->longpress_timer) ecore_timer_del(sd->longpress_timer);
   if (sd->delay) ecore_timer_del(sd->delay);
   if (sd->spin) ecore_timer_del(sd->spin);
   if (sd->special_values)
     {
        EINA_LIST_FREE (sd->special_values, sv)
          {
             eina_stringshare_del(sv->label);
             free(sv);
          }
     }

   ELM_WIDGET_CLASS(_elm_spinner_parent_sc)->base.del(obj);
}

static Eina_Bool
_elm_spinner_smart_theme(Evas_Object *obj)
{
   Eina_Bool int_ret;

   ELM_SPINNER_DATA_GET(obj, sd);

   int_ret = elm_layout_theme_set(obj, "spinner", "base",
                              elm_widget_style_get(obj));

   if (!int_ret) ERR("Failed to set layout!");

   if (sd->ent)
     {
        Eina_Strbuf *buf = eina_strbuf_new();
        eina_strbuf_append_printf(buf, "spinner/%s", elm_widget_style_get(obj));
        elm_widget_style_set(sd->ent, eina_strbuf_string_get(buf));
        eina_strbuf_free(buf);
     }

   if (sd->inc_button)
     {
        Eina_Strbuf *buf = eina_strbuf_new();
        eina_strbuf_append_printf(buf, "spinner/increase/%s", elm_widget_style_get(obj));
        elm_widget_style_set(sd->inc_button, eina_strbuf_string_get(buf));
        eina_strbuf_free(buf);
     }

   if (sd->text_button)
     {
        Eina_Strbuf *buf = eina_strbuf_new();
        eina_strbuf_append_printf(buf, "spinner/%s", elm_widget_style_get(obj));
        elm_widget_style_set(sd->text_button, eina_strbuf_string_get(buf));
        eina_strbuf_free(buf);
     }

   if (sd->dec_button)
     {
        Eina_Strbuf *buf = eina_strbuf_new();
        eina_strbuf_append_printf(buf, "spinner/decrease/%s", elm_widget_style_get(obj));
        elm_widget_style_set(sd->dec_button, eina_strbuf_string_get(buf));
        eina_strbuf_free(buf);
     }

   if (_elm_config->access_mode)
     _access_spinner_register(obj, EINA_TRUE);

   elm_layout_sizing_eval(obj);
   return int_ret;
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

static Eina_Bool
_elm_spinner_smart_focus_next(const Evas_Object *obj,
                           Elm_Focus_Direction dir,
                           Evas_Object **next)
{
   Evas_Object *ao;
   Eina_List *items = NULL;

   ELM_SPINNER_CHECK(obj) EINA_FALSE;
   ELM_SPINNER_DATA_GET(obj, sd);

   if (_elm_config->access_mode)
     {
        ao = _access_object_get(obj, "access");
        items = eina_list_append(items, ao);
     }
   if (!elm_widget_disabled_get(obj))
     {
        items = eina_list_append(items, sd->inc_button);
        if (sd->entry_visible)
          items = eina_list_append(items, sd->ent);
        else
          items = eina_list_append(items, sd->text_button);
        items = eina_list_append(items, sd->dec_button);
     }
   return elm_widget_focus_list_next_get
            (obj, items, eina_list_data_get, dir, next);
}

static void
_elm_spinner_smart_access(Evas_Object *obj, Eina_Bool is_access)
{
   ELM_SPINNER_CHECK(obj);

   _access_spinner_register(obj, is_access);
}

static Eina_Bool
_elm_spinner_smart_activate(Evas_Object *obj, Elm_Activate act)
{
   ELM_SPINNER_DATA_GET(obj, sd);

   if ((elm_widget_disabled_get(obj)) ||
       (act == ELM_ACTIVATE_DEFAULT)) return EINA_FALSE;

   if ((act == ELM_ACTIVATE_UP) ||
       (act == ELM_ACTIVATE_RIGHT))
     {
        if (sd->entry_visible)
          {
             _entry_value_apply(obj);
             if ((sd->val_updated) && (sd->val == sd->val_min))
               return EINA_TRUE;
          }
        _val_inc_start(obj);
        _access_increment_decrement_info_say(obj, EINA_TRUE);
        _val_inc_stop(obj);
     }
   else if ((act == ELM_ACTIVATE_DOWN) ||
            (act == ELM_ACTIVATE_LEFT))
     {
        if (sd->entry_visible)
          {
             _entry_value_apply(obj);
             if ((sd->val_updated) && (sd->val == sd->val_max))
               return EINA_TRUE;
          }
        _val_dec_start(obj);
        _access_increment_decrement_info_say(obj, EINA_FALSE);
        _val_dec_stop(obj);
     }
   return EINA_TRUE;
}

static Eina_Bool
_elm_spinner_smart_focus_direction_manager_is(const Evas_Object *obj __UNUSED__)
{
   return EINA_TRUE;
}

static Eina_Bool
_elm_spinner_smart_focus_direction(const Evas_Object *obj,
                                   const Evas_Object *base,
                                   double degree,
                                   Evas_Object **direction,
                                   double *weight)
{
   Eina_Bool ret;
   Eina_List *items = NULL;
   void *(*list_data_get)(const Eina_List *list);

   ELM_SPINNER_DATA_GET(obj, sd);

   if (!sd)
     return EINA_FALSE;

   list_data_get = eina_list_data_get;
   items = eina_list_append(items, sd->inc_button);
   if (sd->entry_visible)
     items = eina_list_append(items, sd->ent);
   else
     items = eina_list_append(items, sd->text_button);
   items = eina_list_append(items, sd->dec_button);

   ret = elm_widget_focus_list_direction_get
        (obj, base, items, list_data_get, degree, direction, weight);
   eina_list_free(items);

   return ret;
}

static void
_elm_spinner_smart_set_user(Elm_Spinner_Smart_Class *sc)
{
   ELM_WIDGET_CLASS(sc)->base.add = _elm_spinner_smart_add;
   ELM_WIDGET_CLASS(sc)->base.del = _elm_spinner_smart_del;

   ELM_WIDGET_CLASS(sc)->focus_next = _elm_spinner_smart_focus_next;
   ELM_WIDGET_CLASS(sc)->focus_direction_manager_is = _elm_spinner_smart_focus_direction_manager_is;
   ELM_WIDGET_CLASS(sc)->focus_direction = _elm_spinner_smart_focus_direction;

   ELM_WIDGET_CLASS(sc)->on_focus = _elm_spinner_smart_on_focus;
   ELM_WIDGET_CLASS(sc)->disable = _elm_spinner_smart_disable;
   ELM_LAYOUT_CLASS(sc)->sizing_eval = _elm_spinner_smart_sizing_eval;

   ELM_WIDGET_CLASS(sc)->theme = _elm_spinner_smart_theme;
   ELM_WIDGET_CLASS(sc)->translate = _elm_spinner_smart_translate;
   ELM_WIDGET_CLASS(sc)->activate = _elm_spinner_smart_activate;

   ELM_WIDGET_CLASS(sc)->access = _elm_spinner_smart_access;
}

static void
_entry_changed_user_cb(void *data,
                  Evas_Object *obj,
                  void *event_info EINA_UNUSED)
{
   Evas_Object *spinner;
   const char *str;
   int len, max_len;
   double min, max, val;

   spinner = data;
   ELM_SPINNER_DATA_GET(spinner, sd);
   str = elm_object_text_get(obj);
   len = strlen(str);
   if (len < 1) return;
   val = atof(str);
   elm_spinner_min_max_get(spinner, &min, &max);
   max_len = log10(abs(max)) + 1 + sd->decimal_ponts + 1;
   if (max_len == len)
     {
        val = (val < min) ? min : (val > max ? max : val);
        elm_spinner_value_set(spinner, val);
        _label_write(spinner);
        str = elm_object_text_get(obj);
     }

   evas_object_smart_callback_call(spinner, SIG_ENTRY_CHANGED, (void *)str);
}

static void
_entry_clicked_cb(void *data EINA_UNUSED,
                  Evas_Object *obj,
                  void *event_info EINA_UNUSED)
{
   elm_entry_select_all(obj);
}

static int
_decimal_points_get(const char *label)
{
   char fmt[32];
   int decimal_point= 0;
   _format_parse(label, fmt);
   char * start = strchr(fmt, '.');
   if (start)
   {
      start = start + 1;
      const char *itr, *end = NULL;
      for (itr = start; *itr != '\0'; itr++)
        {

           if ((*itr == 'd') || (*itr == 'f'))
             {
                end = itr;
                break;
             }
        }

    if (end)
      {
         memcpy(fmt, start, end - start);
         fmt[end - start] = '\0';
      }
     decimal_point = atoi(fmt);
   }
   return decimal_point;
}


static void
_entry_filter_add(Evas_Object *obj)
{
   ELM_SPINNER_DATA_GET(obj, sd);
   static Elm_Entry_Filter_Accept_Set digits_filter_data;
   static Elm_Entry_Filter_Limit_Size limit_filter_data;

   if (!sd->ent) return;

   elm_entry_markup_filter_remove(sd->ent, elm_entry_filter_accept_set, &digits_filter_data);
   elm_entry_markup_filter_remove(sd->ent, elm_entry_filter_limit_size, &limit_filter_data);

   int num_digit = log10(abs(sd->val_max)) + 1;
   limit_filter_data.max_byte_count = 0;

   digits_filter_data.accepted = "0123456789";
   digits_filter_data.rejected = NULL;

   if (sd->decimal_ponts > 0)
     {
        digits_filter_data.accepted = ".0123456789";
        num_digit += (sd->decimal_ponts + 1);
     }

   limit_filter_data.max_char_count = num_digit;

   elm_entry_markup_filter_append(sd->ent, elm_entry_filter_accept_set, &digits_filter_data);

   elm_entry_markup_filter_append(sd->ent, elm_entry_filter_limit_size, &limit_filter_data);
}

EAPI const Elm_Spinner_Smart_Class *
elm_spinner_smart_class_get(void)
{
   static Elm_Spinner_Smart_Class _sc =
     ELM_SPINNER_SMART_CLASS_INIT_NAME_VERSION(ELM_SPINNER_SMART_NAME);
   static const Elm_Spinner_Smart_Class *class = NULL;
   Evas_Smart_Class *esc = (Evas_Smart_Class *)&_sc;

   if (class) return class;

   _elm_spinner_smart_set(&_sc);
   esc->callbacks = _smart_callbacks;
   class = &_sc;

   return class;
}

static void
_create_entry(Evas_Object *obj)
{
  ELM_SPINNER_DATA_GET(obj, sd);
  if(!sd->ent)
    {
       sd->ent = elm_entry_add(obj);
       elm_entry_single_line_set(sd->ent, EINA_TRUE);
       Eina_Strbuf *buf = eina_strbuf_new();
       eina_strbuf_append_printf(buf, "spinner/%s", elm_widget_style_get(obj));
       elm_widget_style_set(sd->ent, eina_strbuf_string_get(buf));
       eina_strbuf_free(buf);
       elm_entry_cnp_mode_set(sd->ent, ELM_CNP_MODE_PLAINTEXT);
       elm_entry_input_panel_layout_set(sd->ent, ELM_INPUT_PANEL_LAYOUT_NUMBERONLY);
       elm_entry_select_allow_set(sd->ent, EINA_TRUE);
       elm_entry_prediction_allow_set(sd->ent, EINA_FALSE);
       elm_entry_drag_disabled_set(sd->ent, EINA_TRUE);
       elm_entry_magnifier_disabled_set(sd->ent, EINA_TRUE);
       elm_entry_context_menu_disabled_set(sd->ent, EINA_TRUE);
       elm_entry_cursor_handler_disabled_set(sd->ent, EINA_TRUE);
       evas_object_smart_callback_add(sd->ent,
                                  "activated", _entry_activated_cb, obj);
       evas_object_smart_callback_add(sd->ent,
                                  "changed,user", _entry_changed_user_cb, obj);
       evas_object_smart_callback_add(sd->ent,
                                  "clicked", _entry_clicked_cb, obj);
       _entry_filter_add(obj);
       elm_layout_content_set(obj, "elm.swallow.entry", sd->ent);
       evas_object_event_callback_add(sd->ent, EVAS_CALLBACK_SHOW,
                                      _entry_show_cb, obj);
    }
}

EAPI Evas_Object *
elm_spinner_add(Evas_Object *parent)
{
   Evas_Object *obj;

   EINA_SAFETY_ON_NULL_RETURN_VAL(parent, NULL);

   obj = elm_widget_add(_elm_spinner_smart_class_new(), parent);
   if (!obj) return NULL;

   if (!elm_widget_sub_object_add(parent, obj))
     ERR("could not add %p as sub object of %p", obj, parent);

   ELM_SPINNER_DATA_GET(obj, sd);

   sd->val = 0.0;
   sd->val_min = 0.0;
   sd->val_max = 100.0;
   sd->wrap = 0;
   sd->step = 1.0;
   sd->interval = 0.85;
   sd->entry_visible = EINA_FALSE;
   sd->editable = EINA_TRUE;

   elm_layout_theme_set(obj, "spinner", "base", elm_widget_style_get(obj));
   elm_layout_signal_callback_add(obj, "drag", "*", _drag_cb, obj);
   elm_layout_signal_callback_add(obj, "drag,start", "*", _drag_start_cb, obj);
   elm_layout_signal_callback_add(obj, "drag,stop", "*", _drag_stop_cb, obj);
   elm_layout_signal_callback_add(obj, "drag,step", "*", _drag_stop_cb, obj);
   elm_layout_signal_callback_add(obj, "drag,page", "*", _drag_stop_cb, obj);

#ifdef ELM_FEATURE_WEARABLE_C1
/*
   sd->inc_button = elm_button_add(obj);
   elm_object_style_set(sd->inc_button, "spinner/increase/default");

   evas_object_smart_callback_add
     (sd->inc_button, "clicked", _inc_button_clicked_cb, obj);
   evas_object_smart_callback_add
     (sd->inc_button, "pressed", _inc_button_pressed_cb, obj);
   evas_object_smart_callback_add
     (sd->inc_button, "unpressed", _inc_button_unpressed_cb, obj);

   elm_layout_content_set(obj, "elm.swallow.inc_button", sd->inc_button);
   elm_widget_sub_object_add(obj, sd->inc_button);
*/
   sd->text_button = elm_button_add(obj);
   elm_object_style_set(sd->text_button, "spinner/default");

//   evas_object_smart_callback_add
//     (sd->text_button, "clicked", _text_button_clicked_cb, obj);

   elm_layout_content_set(obj, "elm.swallow.text_button", sd->text_button);
   elm_widget_sub_object_add(obj, sd->text_button);

/*
   sd->dec_button = elm_button_add(obj);
   elm_object_style_set(sd->dec_button, "spinner/decrease/default");

   evas_object_smart_callback_add
     (sd->dec_button, "clicked", _dec_button_clicked_cb, obj);
   evas_object_smart_callback_add
     (sd->dec_button, "pressed", _dec_button_pressed_cb, obj);
   evas_object_smart_callback_add
     (sd->dec_button, "unpressed", _dec_button_unpressed_cb, obj);

   elm_layout_content_set(obj, "elm.swallow.dec_button", sd->dec_button);
   elm_widget_sub_object_add(obj, sd->dec_button);
*/
#else
   sd->inc_button = elm_button_add(obj);
   elm_object_style_set(sd->inc_button, "spinner/increase/default");

   evas_object_smart_callback_add
   (sd->inc_button, "clicked", _inc_button_clicked_cb, obj);
   evas_object_smart_callback_add
   (sd->inc_button, "pressed", _inc_button_pressed_cb, obj);
   evas_object_smart_callback_add
   (sd->inc_button, "unpressed", _inc_button_unpressed_cb, obj);

   elm_layout_content_set(obj, "elm.swallow.inc_button", sd->inc_button);
   elm_widget_sub_object_add(obj, sd->inc_button);

   sd->text_button = elm_button_add(obj);
   elm_object_style_set(sd->text_button, "spinner/default");

   evas_object_smart_callback_add
     (sd->text_button, "clicked", _text_button_clicked_cb, obj);

   elm_layout_content_set(obj, "elm.swallow.text_button", sd->text_button);
   elm_widget_sub_object_add(obj, sd->text_button);

   sd->dec_button = elm_button_add(obj);
   elm_object_style_set(sd->dec_button, "spinner/decrease/default");

   evas_object_smart_callback_add
   (sd->dec_button, "clicked", _dec_button_clicked_cb, obj);
   evas_object_smart_callback_add
   (sd->dec_button, "pressed", _dec_button_pressed_cb, obj);
   evas_object_smart_callback_add
   (sd->dec_button, "unpressed", _dec_button_unpressed_cb, obj);

   elm_layout_content_set(obj, "elm.swallow.dec_button", sd->dec_button);
   elm_widget_sub_object_add(obj, sd->dec_button);
#endif

   edje_object_part_drag_value_set
     (ELM_WIDGET_DATA(sd)->resize_obj, "elm.dragable.slider", 0.0, 0.0);
   elm_layout_signal_callback_add
     (obj, "elm,action,entry,toggle", "*", _entry_toggle_cb, obj);

   _label_write(obj);
   elm_widget_can_focus_set(obj, EINA_TRUE);

   elm_layout_sizing_eval(obj);

   /* access */
   if (_elm_config->access_mode)
     _access_spinner_register(obj, EINA_TRUE);

   //Tizen Only: This should be removed when eo is applied.
   ELM_WIDGET_DATA_GET(obj, wsd);
   wsd->on_create = EINA_FALSE;

   return obj;
}

EAPI void
elm_spinner_label_format_set(Evas_Object *obj,
                             const char *fmt)
{
   ELM_SPINNER_CHECK(obj);
   ELM_SPINNER_DATA_GET(obj, sd);

   eina_stringshare_replace(&sd->label, fmt);
   if (!fmt)
     sd->decimal_ponts = 0;
   else
     sd->decimal_ponts = _decimal_points_get(sd->label);

   _label_write(obj);
   elm_layout_sizing_eval(obj);
   _entry_filter_add(obj);
}

EAPI const char *
elm_spinner_label_format_get(const Evas_Object *obj)
{
   ELM_SPINNER_CHECK(obj) NULL;
   ELM_SPINNER_DATA_GET(obj, sd);

   return sd->label;
}

EAPI void
elm_spinner_min_max_set(Evas_Object *obj,
                        double min,
                        double max)
{
   ELM_SPINNER_CHECK(obj);
   ELM_SPINNER_DATA_GET(obj, sd);

   if ((sd->val_min == min) && (sd->val_max == max)) return;
   sd->val_min = min;
   sd->val_max = max;
   if (sd->val >= sd->val_min && sd->val <= sd->val_max) return;
   if (sd->val < sd->val_min) sd->val = sd->val_min;
   if (sd->val > sd->val_max) sd->val = sd->val_max;
   _drag_val_set(obj);
   _label_write(obj);
   evas_object_smart_callback_call(obj, SIG_CHANGED, NULL);
   _entry_filter_add(obj);
}

EAPI void
elm_spinner_min_max_get(const Evas_Object *obj,
                        double *min,
                        double *max)
{
   if (min) *min = 0.0;
   if (max) *max = 0.0;

   ELM_SPINNER_CHECK(obj);
   ELM_SPINNER_DATA_GET(obj, sd);

   if (min) *min = sd->val_min;
   if (max) *max = sd->val_max;
}

EAPI void
elm_spinner_step_set(Evas_Object *obj,
                     double step)
{
   ELM_SPINNER_CHECK(obj);
   ELM_SPINNER_DATA_GET(obj, sd);

   sd->step = step;
}

EAPI double
elm_spinner_step_get(const Evas_Object *obj)
{
   ELM_SPINNER_CHECK(obj) 0.0;
   ELM_SPINNER_DATA_GET(obj, sd);

   return sd->step;
}

EAPI void
elm_spinner_value_set(Evas_Object *obj,
                      double val)
{
   ELM_SPINNER_CHECK(obj);
   ELM_SPINNER_DATA_GET(obj, sd);

   if ((sd->val == val) && !sd->entry_visible) return;
   sd->val = val;
   sd->val_updated = EINA_FALSE;
   if (sd->val < sd->val_min)
     {
        sd->val = sd->val_min;
        sd->val_updated = EINA_TRUE;
     }
   if (sd->val > sd->val_max)
     {
        sd->val = sd->val_max;
        sd->val_updated = EINA_TRUE;
     }
   _drag_val_set(obj);
   _label_write(obj);
   evas_object_smart_callback_call(obj, SIG_CHANGED, NULL);
}

EAPI double
elm_spinner_value_get(const Evas_Object *obj)
{
   ELM_SPINNER_CHECK(obj) 0.0;
   ELM_SPINNER_DATA_GET(obj, sd);

   return sd->val;
}

EAPI void
elm_spinner_wrap_set(Evas_Object *obj,
                     Eina_Bool wrap)
{
   ELM_SPINNER_CHECK(obj);
   ELM_SPINNER_DATA_GET(obj, sd);

   sd->wrap = wrap;
}

EAPI Eina_Bool
elm_spinner_wrap_get(const Evas_Object *obj)
{
   ELM_SPINNER_CHECK(obj) EINA_FALSE;
   ELM_SPINNER_DATA_GET(obj, sd);

   return sd->wrap;
}

EAPI void
elm_spinner_special_value_add(Evas_Object *obj,
                              double value,
                              const char *label)
{
   Elm_Spinner_Special_Value *sv;
   Eina_List *l;

   ELM_SPINNER_CHECK(obj);
   ELM_SPINNER_DATA_GET(obj, sd);

   EINA_LIST_FOREACH(sd->special_values, l, sv)
     {
        if (sv->value != value)
          continue;

        eina_stringshare_replace(&sv->label, label);
        _label_write(obj);
        return;
     }

   sv = calloc(1, sizeof(*sv));
   if (!sv) return;
   sv->value = value;
   sv->label = eina_stringshare_add(label);

   sd->special_values = eina_list_append(sd->special_values, sv);
   _label_write(obj);
}

EAPI void
elm_spinner_special_value_del(Evas_Object *obj,
                              double value)
{
   Elm_Spinner_Special_Value *sv;
   Eina_List *l;

   ELM_SPINNER_CHECK(obj);
   ELM_SPINNER_DATA_GET(obj, sd);

   EINA_LIST_FOREACH(sd->special_values, l, sv)
     {
        if (sv->value != value)
          continue;

        sd->special_values = eina_list_remove_list(sd->special_values, l);
        eina_stringshare_del(sv->label);
        free(sv);
        _label_write(obj);
        return;
     }
}

EAPI const char *
elm_spinner_special_value_get(Evas_Object *obj,
                              double value)
{
   Elm_Spinner_Special_Value *sv;
   Eina_List *l;

   ELM_SPINNER_CHECK(obj) NULL;
   ELM_SPINNER_DATA_GET(obj, sd);

   EINA_LIST_FOREACH(sd->special_values, l, sv)
     {
        if (sv->value == value)
          return sv->label;
     }

   return NULL;
}

EAPI void
elm_spinner_editable_set(Evas_Object *obj,
                         Eina_Bool editable)
{
   ELM_SPINNER_CHECK(obj);
   ELM_SPINNER_DATA_GET(obj, sd);

   sd->editable = editable;
}

EAPI Eina_Bool
elm_spinner_editable_get(const Evas_Object *obj)
{
   ELM_SPINNER_CHECK(obj) EINA_FALSE;
   ELM_SPINNER_DATA_GET(obj, sd);

   return sd->editable;
}

EAPI void
elm_spinner_interval_set(Evas_Object *obj,
                         double interval)
{
   ELM_SPINNER_CHECK(obj);
   ELM_SPINNER_DATA_GET(obj, sd);

   sd->interval = interval;
}

EAPI double
elm_spinner_interval_get(const Evas_Object *obj)
{
   ELM_SPINNER_CHECK(obj) 0.0;
   ELM_SPINNER_DATA_GET(obj, sd);

   return sd->interval;
}

EAPI void
elm_spinner_base_set(Evas_Object *obj,
                     double base)
{
   ELM_SPINNER_CHECK(obj);
   ELM_SPINNER_DATA_GET(obj, sd);

   sd->val_base = base;
}

EAPI double
elm_spinner_base_get(const Evas_Object *obj)
{
   ELM_SPINNER_CHECK(obj) 0.0;
   ELM_SPINNER_DATA_GET(obj, sd);

   return sd->val_base;
}

EAPI void
elm_spinner_round_set(Evas_Object *obj,
                      int rnd)
{
   ELM_SPINNER_CHECK(obj);
   ELM_SPINNER_DATA_GET(obj, sd);

   sd->round = rnd;
}

EAPI int
elm_spinner_round_get(const Evas_Object *obj)
{
   ELM_SPINNER_CHECK(obj) 0;
   ELM_SPINNER_DATA_GET(obj, sd);

   return sd->round;
}
