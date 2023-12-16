#include <Elementary.h>
#include "elm_priv.h"
#include "elm_widget_popup.h"
#include "elm_interface_scrollable.h"

#define _TIZEN_

#define ELM_POPUP_ACTION_BUTTON_MAX 3

EAPI const char ELM_POPUP_SMART_NAME[] = "elm_popup";

static void _button_remove(Evas_Object *, int, Eina_Bool);

static const char ACCESS_TITLE_PART[] = "access.title";
static const char ACCESS_BODY_PART[] = "access.body";
static const char ACCESS_OUTLINE_PART[] = "access.outline";

static const char SIG_DISMISSED[] = "dismissed";
static const char SIG_SHOW_FINISHED[] = "show,finished";
static const char SIG_BLOCK_CLICKED[] = "block,clicked";
static const char SIG_TIMEOUT[] = "timeout";
static const char SIG_ACCESS_CHANGED[] = "access,changed";
static const char SIG_LANG_CHANGED[] = "language,changed";

static const Evas_Smart_Cb_Description _smart_callbacks[] = {
   {SIG_DISMISSED, ""},
   {SIG_SHOW_FINISHED, ""},
   {SIG_BLOCK_CLICKED, ""},
   {SIG_TIMEOUT, ""},
   {SIG_ACCESS_CHANGED, ""},
   {SIG_LANG_CHANGED, ""},
   {NULL, NULL}
};

EVAS_SMART_SUBCLASS_NEW
  (ELM_POPUP_SMART_NAME, _elm_popup, Elm_Popup_Smart_Class,
  Elm_Layout_Smart_Class, elm_layout_smart_class_get, _smart_callbacks);

static void _notify_resize_cb(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void  _on_content_del(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _mouse_up_cb(void *, Evas *, Evas_Object *, void *);
static void _mouse_down_cb(void *, Evas *, Evas_Object *, void *);
static void _mouse_move_cb(void *, Evas *, Evas_Object *, void *);
static void _item_highlight(Elm_Popup_Item *it);
static void _item_unselect(Elm_Popup_Item *it);
static void _highlight_timer_enable(Elm_Popup_Item *it);
static Eina_Bool _focus_enabled(Evas_Object *obj);

static Eina_Bool
_elm_popup_smart_translate(Evas_Object *obj)
{
   ELM_POPUP_DATA_GET(obj, sd);
   Elm_Popup_Item *it;
   Eina_List *l;

   EINA_LIST_FOREACH(sd->items, l, it)
     elm_widget_item_translate(it);

   evas_object_smart_callback_call(obj, SIG_LANG_CHANGED, NULL);

   return EINA_TRUE;
}

static void
_show_finished_cb(void *data,
                  Evas_Object *obj __UNUSED__,
                  void *event_info __UNUSED__)
{
   evas_object_smart_callback_call(data, SIG_SHOW_FINISHED, NULL);
}

static void
_hide_effect_finished_cb(void *data,
                  Evas_Object *obj __UNUSED__,
                  void *event_info __UNUSED__)
{
   evas_object_smart_callback_call(data, SIG_DISMISSED, NULL);
}

#ifdef ELM_FEATURE_WEARABLE_C1
static char *
_popup_outline_access_info_cb(void *data, Evas_Object *obj)
{
   char *info;

   ELM_POPUP_DATA_GET(data, sd);

   info = elm_object_part_text_get(sd->main_layout, "elm.text");

   if (info) return strdup(info);
   return NULL;
}

static char *
_popup_title_access_info_cb(void *data, Evas_Object *obj)
{
   char *info, *str = NULL;
   Eina_Strbuf *buf = NULL;

   info = elm_object_part_text_get(data, "elm.text.title");

   if (info)
     {
        buf = eina_strbuf_new();
        eina_strbuf_append(buf, E_("WDS_TTS_TBOPT_ALERT"));
        eina_strbuf_append(buf, ", ");
        eina_strbuf_append(buf, info);
        str = eina_strbuf_string_steal(buf);
        eina_strbuf_free(buf);
        return strdup(str);
     }
   return NULL;
}

static char *
_popup_body_access_info_cb(void *data, Evas_Object *obj)
{
   char *info;

   info = elm_object_part_text_get(data, "elm.text");
   if (info) return strdup(info);

   return NULL;
}

static void
_highlight_chain_set(Evas_Object *prev, Evas_Object *next)
{
   elm_access_chain_end_set(next, ELM_HIGHLIGHT_DIR_NEXT);
   elm_access_highlight_next_set(next, ELM_HIGHLIGHT_DIR_NEXT, prev);
   elm_access_highlight_next_set(prev, ELM_HIGHLIGHT_DIR_PREVIOUS, next);
}

static Eina_Bool
_create_popup_access_chain(Evas_Object *obj)
{
   ELM_POPUP_DATA_GET(obj, sd);

   Evas_Object *layout, *ao_title = NULL, *ao_body = NULL, *btn1 = NULL, *btn2 = NULL;
   char *group = NULL, *title_text = NULL, *body_text = NULL;

   if (!_elm_config->access_mode) return ECORE_CALLBACK_CANCEL;

   if (!strcmp(elm_widget_style_get(obj), "circle") && sd->content_area)
     {
        layout = elm_layout_content_get(sd->content_area, "elm.swallow.content");
        if (layout && !strcmp(elm_widget_type_get(layout), "elm_layout"))
          {

             edje_object_file_get(elm_layout_edje_get(layout), NULL, &group);
             if (group && (!strncmp(group, "elm/layout/popup/content/circle", strlen("elm/layout/popup/content/circle"))))
               {
                  title_text = elm_object_part_text_get(layout, "elm.text.title");
                  if (title_text && strlen(title_text))
                    ao_title = elm_object_part_access_register(layout, "elm.text.title");

                  body_text = elm_object_part_text_get(layout, "elm.text");
                  if (body_text && strlen(body_text))
                    ao_body = elm_object_part_access_register(layout, "elm.text");

                  switch (sd->button_count)
                    {
                       case 1:
                         btn1 = elm_object_part_content_get(sd->action_area, "actionbtn1");
                         break;
                       case 2:
                         btn1 = elm_object_part_content_get(sd->action_area, "actionbtn1");
                         btn2 = elm_object_part_content_get(sd->action_area, "actionbtn2");
                         break;
                       default:
                         break;
                    }

                  if (ao_title)
                    {
                       elm_access_info_cb_set(ao_title, ELM_ACCESS_INFO, _popup_title_access_info_cb, layout);
                       elm_access_chain_end_set(ao_title, ELM_HIGHLIGHT_DIR_PREVIOUS);
                       _elm_access_highlight_set(ao_title, EINA_TRUE);
                       if (ao_body)
                         {
                            elm_access_info_cb_set(ao_body, ELM_ACCESS_INFO, _popup_body_access_info_cb, layout);
                            elm_access_highlight_next_set(ao_title, ELM_HIGHLIGHT_DIR_NEXT, ao_body);
                            elm_access_highlight_next_set(ao_body, ELM_HIGHLIGHT_DIR_PREVIOUS, ao_title);
                            if (btn1)
                              {
                                 elm_access_highlight_next_set(btn1, ELM_HIGHLIGHT_DIR_PREVIOUS, ao_body);
                                 if (btn2) _highlight_chain_set(ao_title, btn2);
                                 else _highlight_chain_set(ao_title, btn1);
                              }
                            else _highlight_chain_set(ao_title, ao_body);
                         }
                       else
                         {
                            if (btn1)
                              {
                                 if (btn2) _highlight_chain_set(ao_title, btn2);
                                 else _highlight_chain_set(ao_title, btn1);
                              }
                            else elm_access_chain_end_set(ao_body, ELM_HIGHLIGHT_DIR_NEXT);
                         }
                    }
                  else
                    {
                       if (ao_body)
                         {
                            elm_access_info_cb_set(ao_body, ELM_ACCESS_INFO, _popup_body_access_info_cb, layout);
                            elm_access_chain_end_set(ao_body, ELM_HIGHLIGHT_DIR_PREVIOUS);
                            _elm_access_highlight_set(ao_body, EINA_TRUE);
                            if (btn1)
                              {
                                 elm_access_highlight_next_set(btn1, ELM_HIGHLIGHT_DIR_PREVIOUS, ao_body);
                                 if (btn2) _highlight_chain_set(ao_body, btn2);
                                 else _highlight_chain_set(ao_body, btn1);
                              }
                            else elm_access_chain_end_set(ao_body, ELM_HIGHLIGHT_DIR_NEXT);
                         }
                    }
               }
          }
     }
   return ECORE_CALLBACK_CANCEL;
}
#endif

static void
_visuals_set(Evas_Object *obj)
{
   ELM_POPUP_DATA_GET(obj, sd);

   if (!sd->title_text && !sd->title_icon)
     elm_layout_signal_emit(sd->main_layout, "elm,state,title_area,hidden", "elm");
   else
     elm_layout_signal_emit(sd->main_layout, "elm,state,title_area,visible", "elm");

   if (sd->button_count)
     elm_layout_signal_emit(sd->main_layout, "elm,state,action_area,visible", "elm");
   else
     elm_layout_signal_emit(sd->main_layout, "elm,state,action_area,hidden", "elm");

   edje_object_message_signal_process(elm_layout_edje_get(sd->main_layout));
}

static void
_item_update(Elm_Popup_Item *item)
{
   if (item->label)
     {
        edje_object_part_text_escaped_set
           (VIEW(item), "elm.text", item->label);
        edje_object_signal_emit
           (VIEW(item), "elm,state,item,text,visible", "elm");
     }
   if (item->icon)
      edje_object_signal_emit
         (VIEW(item), "elm,state,item,icon,visible", "elm");

#ifdef _TIZEN_
   if (item->bottom_line)
      edje_object_signal_emit
         (VIEW(item), "elm,state,item,bottomline,hide", "elm");
#endif
   evas_object_show(VIEW(item));
   edje_object_message_signal_process(VIEW(item));
}

static void
_block_clicked_cb(void *data,
                  Evas_Object *obj __UNUSED__,
                  void *event_info __UNUSED__)
{
   evas_object_smart_callback_call(data, SIG_BLOCK_CLICKED, NULL);
}

static void
_timeout_cb(void *data,
            Evas_Object *obj __UNUSED__,
            void *event_info __UNUSED__)
{
   //TIZEN_ONLY(20150903): prevent popup hiding when the layout has "hide_finished_signal" data item
   //evas_object_hide(data);
   const char *hide_signal = NULL;

   ELM_POPUP_DATA_GET(data, sd);
   hide_signal = edje_object_data_get(elm_layout_edje_get(sd->main_layout), "hide_finished_signal");

   if ((hide_signal) && (!strcmp(hide_signal, "on")))
     elm_layout_signal_emit(sd->main_layout, "elm,state,hide", "elm");
   else
     evas_object_hide(data);
   //
   evas_object_smart_callback_call(data, SIG_TIMEOUT, NULL);
}

static Evas_Object *
_access_object_get(const Evas_Object *obj, const char* part)
{
   Evas_Object *po, *ao;
   ELM_POPUP_DATA_GET(obj, sd);

   po = (Evas_Object *)edje_object_part_object_get
      (elm_layout_edje_get(sd->main_layout), part);
   ao = evas_object_data_get(po, "_part_access_obj");

   return ao;
}

static void
_item_highlight(Elm_Popup_Item *it)
{
   ELM_POPUP_ITEM_CHECK_OR_RETURN(it);

   if (elm_widget_item_disabled_get(it)) return;
   edje_object_signal_emit(VIEW(it), "elm,state,selected", "elm");
}

static void
_item_unhighlight(Elm_Popup_Item *it)
{
   ELM_POPUP_ITEM_CHECK_OR_RETURN(it);

   edje_object_signal_emit(VIEW(it), "elm,state,unselected", "elm");
}

static void
_item_unselect(Elm_Popup_Item *it)
{
   ELM_POPUP_ITEM_CHECK_OR_RETURN(it);

   if (!it->selected) return;

   it->selected = EINA_FALSE;
   _item_unhighlight(it);
}

static Eina_Bool
_highlight_timer(void *data)
{
   Elm_Popup_Item *it = data;
   it->highlight_timer = NULL;
   _item_highlight(it);

   return ECORE_CALLBACK_CANCEL;
}

static void
_highlight_timer_enable(Elm_Popup_Item *it)
{
   if (it->highlight_timer) ecore_timer_del(it->highlight_timer);
   it->highlight_timer =
      ecore_timer_add(ELM_ITEM_HIGHLIGHT_TIMER, _highlight_timer, it);
}

static void
_mouse_move_cb(void *data,
               Evas *evas __UNUSED__,
               Evas_Object *o __UNUSED__,
               void *event_info)
{
   Evas_Object *obj;
   Elm_Popup_Item *it = data;
   Evas_Event_Mouse_Move *ev = event_info;

   ELM_POPUP_ITEM_CHECK_OR_RETURN(it);
   obj = WIDGET(it);
   ELM_POPUP_DATA_GET(obj, sd);

   evas_object_ref(obj);

   if(it->selected) _item_unselect(it);

   if (ev->event_flags & EVAS_EVENT_FLAG_ON_HOLD)
     {
        if (it->highlight_timer)
          {
             ecore_timer_del(it->highlight_timer);
             it->highlight_timer = NULL;
          }
        else _item_unhighlight(it);
        if (!sd->on_hold)
          {
             sd->on_hold = EINA_TRUE;
             if (!sd->was_selected)
               _item_unselect(it);
          }
     }
   evas_object_unref(obj);
}

static void
_mouse_down_cb(void *data,
               Evas *evas,
               Evas_Object *o __UNUSED__,
               void *event_info)
{
   Evas_Event_Mouse_Down *ev = event_info;
   Elm_Popup_Item *it = data;
   Evas_Object *obj;

   ELM_POPUP_ITEM_CHECK_OR_RETURN(it);
   obj = WIDGET(it);
   ELM_POPUP_DATA_GET(obj, sd);

   if (ev->button != 1) return;

   // mouse_down is only activated when one finger touched
   if (evas_event_down_count_get(evas) != 1) return;

   if (ev->event_flags & EVAS_EVENT_FLAG_ON_HOLD) sd->on_hold = EINA_TRUE;
   else sd->on_hold = EINA_FALSE;

   if (sd->on_hold) return;
   sd->was_selected = it->selected;

   evas_object_ref(obj);

   _highlight_timer_enable(it);

   evas_object_unref(obj);
}

static void
_mouse_up_cb(void *data,
             Evas *evas,
             Evas_Object *o __UNUSED__,
             void *event_info)
{
   Evas_Object *obj;
   Elm_Popup_Item *it = data;
   Evas_Event_Mouse_Up *ev = event_info;

   ELM_POPUP_ITEM_CHECK_OR_RETURN(it);
   obj = WIDGET(it);
   ELM_POPUP_DATA_GET(obj, sd);

   if (ev->button != 1) return;

   if (ev->event_flags & EVAS_EVENT_FLAG_ON_HOLD) sd->on_hold = EINA_TRUE;
   else sd->on_hold = EINA_FALSE;

   // mouse up is only activated when no finger touched
   if (evas_event_down_count_get(evas) != 0)
     {
        if (!it->highlight_timer)  return;
     }
   if (it->highlight_timer)
     {
        ecore_timer_del(it->highlight_timer);
        it->highlight_timer = NULL;
        if (evas_event_down_count_get(evas) == 0) _item_unhighlight(it);
     }

   if (sd->on_hold) sd->on_hold = EINA_FALSE;

}

static void
_on_show(void *data __UNUSED__,
         Evas *e __UNUSED__,
         Evas_Object *obj,
         void *event_info __UNUSED__)
{
   ELM_POPUP_DATA_GET(obj, sd);

   elm_object_focus_set(obj, EINA_TRUE);
   sd->access_first = EINA_TRUE;

#ifdef ELM_FEATURE_WEARABLE_C1
   _create_popup_access_chain(obj);
#endif
}

static void
_scroller_size_calc(Evas_Object *obj)
{

   Evas_Coord h = 0;
   Evas_Coord y_notify, h_notify;
   Evas_Coord wy, wh;
   Evas_Coord h_title = 0;
   Evas_Coord h_action_area = 0;
   Evas_Coord h_pad = 0;
   const char *str;

   ELM_POPUP_DATA_GET(obj, sd);
   ELM_WIDGET_DATA_GET(obj, wsd);

   // Popup maximum height
   if (wsd->orient_mode == 90 || wsd->orient_mode == 270)
     str = edje_object_data_get(elm_layout_edje_get(sd->main_layout),
                                "max_landscape_height");
   else
     str = edje_object_data_get(elm_layout_edje_get(sd->main_layout),
                                "max_portrait_height");

   if (str) h = (int)(atoi(str)
                      / edje_object_base_scale_get(elm_layout_edje_get(sd->main_layout))
                      * elm_config_scale_get()
                      * elm_object_scale_get(obj));

   // Notify height
   evas_object_geometry_get(sd->notify, NULL, &y_notify, NULL, &h_notify);
   evas_object_geometry_get(elm_widget_top_get(sd->notify), NULL, &wy, NULL, &wh);
   h_notify += (y_notify - wy);

   // Title area height
   if (sd->title_text || sd->title_icon)
     edje_object_part_geometry_get(elm_layout_edje_get(sd->main_layout), "elm.bg.title", NULL, NULL, NULL, &h_title);

   // Action area height
   if (sd->action_area)
     {
        str = edje_object_data_get(elm_layout_edje_get(sd->action_area),
                                   "action_area_height");
        if (str) h_action_area = (int)(atoi(str)
                                       / edje_object_base_scale_get(elm_layout_edje_get(sd->action_area))
                                       * elm_config_scale_get()
                                       * elm_object_scale_get(obj));
     }

   /* Tizen Only: UX dependency: outside padding is not a fixed concept.
    if height of notify is smaller than maximum height of popup,
    ouside padding doesn't have to be considered. */
   if (h < h_notify)
     {
        // Padding
        str = edje_object_data_get(elm_layout_edje_get(sd->main_layout),
                                   "popup_outside_pad");
        if (str) h_pad = (int)(atoi(str)
                               / edje_object_base_scale_get(elm_layout_edje_get(sd->main_layout))
                               * elm_config_scale_get()
                               * elm_object_scale_get(obj));
     }

   // Scroll area height
   sd->max_sc_h = h_notify - (h_title + h_action_area + h_pad);
}

static void
_size_hints_changed_cb(void *data,
                       Evas *e __UNUSED__,
                       Evas_Object *obj __UNUSED__,
                       void *event_info __UNUSED__)
{
   elm_layout_sizing_eval(data);
}

static void
_list_del(Elm_Popup_Smart_Data *sd)
{
   if (!sd->scr) return;

   evas_object_event_callback_del
     (sd->scr, EVAS_CALLBACK_CHANGED_SIZE_HINTS, _size_hints_changed_cb);

   evas_object_del(sd->tbl);
   sd->scr = NULL;
   sd->box = NULL;
   sd->spacer = NULL;
   sd->tbl = NULL;
}

static void
_item_callback_delete(Elm_Popup_Item *item)
{
   evas_object_event_callback_del_full
     (VIEW(item), EVAS_CALLBACK_MOUSE_DOWN, _mouse_down_cb, item);
   evas_object_event_callback_del_full
     (VIEW(item), EVAS_CALLBACK_MOUSE_UP, _mouse_up_cb, item);
   evas_object_event_callback_del_full
     (VIEW(item), EVAS_CALLBACK_MOUSE_MOVE, _mouse_move_cb, item);
}

static void
_items_remove(Elm_Popup_Smart_Data *sd)
{
   Elm_Popup_Item *item;

   if (!sd->items) return;

   EINA_LIST_FREE (sd->items, item) {
      _item_callback_delete(item);
      elm_widget_item_del(item);
   }

   sd->items = NULL;
}

static void
_on_button_del(void *data,
               Evas *e __UNUSED__,
               Evas_Object *obj,
               void *event_info __UNUSED__)
{
   int i;

   ELM_POPUP_DATA_GET(data, sd);

   for (i = 0; i < ELM_POPUP_ACTION_BUTTON_MAX; i++)
     {
        if (sd->buttons[i] && obj == sd->buttons[i]->btn &&
            sd->buttons[i]->delete_me == EINA_TRUE)
          {
             _button_remove(data, i, EINA_FALSE);
             break;
          }
     }
}

static void
_elm_popup_smart_del(Evas_Object *obj)
{
   unsigned int i;

   ELM_POPUP_DATA_GET(obj, sd);

   evas_object_smart_callback_del
     (sd->notify, "show,finished", _show_finished_cb);
   evas_object_smart_callback_del
     (sd->notify, "block,clicked", _block_clicked_cb);
   evas_object_smart_callback_del(sd->notify, "timeout", _timeout_cb);
   evas_object_event_callback_del
     (sd->content, EVAS_CALLBACK_DEL, _on_content_del);
   evas_object_event_callback_del(obj, EVAS_CALLBACK_SHOW, _on_show);
   evas_object_event_callback_del
      (sd->notify, EVAS_CALLBACK_RESIZE, _notify_resize_cb);
   sd->button_count = 0;

   for (i = 0; i < ELM_POPUP_ACTION_BUTTON_MAX; i++)
     {
        if (sd->buttons[i])
          {
             evas_object_event_callback_del
                (sd->buttons[i]->btn, EVAS_CALLBACK_DEL, _on_button_del);
             evas_object_del(sd->buttons[i]->btn);
             free(sd->buttons[i]);
             sd->buttons[i] = NULL;
          }
     }
   if (sd->items)
     {
        _items_remove(sd);
        _list_del(sd);
     }
   sd->allow_eval = EINA_FALSE;

   // XXX? delete other objects? just to be sure.
   ELM_SAFE_FREE(sd->content_area, evas_object_del);
   ELM_SAFE_FREE(sd->notify, evas_object_del);
   ELM_SAFE_FREE(sd->main_layout, evas_object_del);
   ELM_SAFE_FREE(sd->title_icon, evas_object_del);
   ELM_SAFE_FREE(sd->text_content_obj, evas_object_del);
   ELM_SAFE_FREE(sd->action_area, evas_object_del);
   ELM_SAFE_FREE(sd->box, evas_object_del);
   ELM_SAFE_FREE(sd->tbl, evas_object_del);
   ELM_SAFE_FREE(sd->spacer, evas_object_del);
   ELM_SAFE_FREE(sd->scr, evas_object_del);
   ELM_SAFE_FREE(sd->content, evas_object_del);

   ELM_SAFE_FREE(sd->title_text, eina_stringshare_del);
   ELM_SAFE_FREE(sd->subtitle_text, eina_stringshare_del);

   ELM_WIDGET_CLASS(_elm_popup_parent_sc)->base.del(obj);
}

static void
_mirrored_set(Evas_Object *obj,
              Eina_Bool rtl)
{
   Eina_List *elist;
   Elm_Popup_Item *item;

   ELM_POPUP_DATA_GET(obj, sd);

   elm_object_mirrored_set(sd->notify, rtl);
   if (sd->items)
     EINA_LIST_FOREACH(sd->items, elist, item)
       edje_object_mirrored_set(VIEW(item), rtl);
}

static void
_access_base_activate_cb(void *data,
                        Evas_Object *part_obj __UNUSED__,
                        Elm_Object_Item *item __UNUSED__)
{
   evas_object_smart_callback_call(data, SIG_BLOCK_CLICKED, NULL);
}

static void
_access_title_read_stop_cb(void *data,
                          Evas_Object *obj __UNUSED__,
                          void *event_info __UNUSED__)
{
   ELM_POPUP_DATA_GET(data, sd);

   if (sd->access_first)
     {
        Evas_Object *ao = _access_object_get(data, ACCESS_BODY_PART);

        if (ao)
          _elm_access_highlight_set(ao, EINA_TRUE);
     }
   sd->access_first = EINA_FALSE;
}

static void
_access_title_read_cancel_cb(void *data,
                            Evas_Object *obj __UNUSED__,
                            void *event_info __UNUSED__)
{
   ELM_POPUP_DATA_GET(data, sd);

   sd->access_first = EINA_FALSE;
}

static void
_access_obj_process(Evas_Object *obj, Eina_Bool is_access)
{
   Evas_Object *ao;

   ELM_POPUP_DATA_GET(obj, sd);

   if (is_access)
     {
        if (sd->title_text)
          {
             ao = _elm_access_edje_object_part_object_register
                    (obj, elm_layout_edje_get(sd->main_layout), ACCESS_TITLE_PART);
             _elm_access_text_set(_elm_access_object_get(ao),
                                  ELM_ACCESS_INFO, sd->title_text);
             _elm_access_highlight_set(ao, EINA_TRUE);
             evas_object_smart_callback_add(ao, "access,read,stop", _access_title_read_stop_cb, obj);
             evas_object_smart_callback_add(ao, "access,read,cancel", _access_title_read_cancel_cb, obj);
          }

        if (sd->text_content_obj)
          {
             ao = _elm_access_edje_object_part_object_register
                    (obj, elm_layout_edje_get(sd->main_layout), ACCESS_BODY_PART);
             _elm_access_text_set(_elm_access_object_get(ao),
               ELM_ACCESS_INFO, elm_object_text_get(sd->text_content_obj));
             if (!sd->button_count)
                _elm_access_activate_callback_set
                  (_elm_access_object_get(ao), _access_base_activate_cb, obj);
             if (!sd->title_text && sd->button_count)
                _elm_access_highlight_set(ao, EINA_TRUE);
          }

        /* register outline */
        if (!sd->button_count)
          {
             ao = _access_object_get(obj, ACCESS_OUTLINE_PART);
             if (!ao)
               {
                  ao = _elm_access_edje_object_part_object_register
                     (obj, elm_layout_edje_get(sd->main_layout), ACCESS_OUTLINE_PART);
                  _elm_access_text_set(_elm_access_object_get(ao),
                                       ELM_ACCESS_TYPE,
                                       E_("IDS_SCR_OPT_CENTRAL_POP_UP_TTS"));
                  _elm_access_text_set(_elm_access_object_get(ao),
                                       ELM_ACCESS_CONTEXT_INFO,
                                       E_("WDS_TTS_TBBODY_DOUBLE_TAP_TO_CLOSE_THE_POP_UP"));
#ifdef ELM_FEATURE_WEARABLE_C1
                  _elm_access_callback_set(_elm_access_object_get(ao), ELM_ACCESS_INFO, _popup_outline_access_info_cb, obj);
#endif
                  _elm_access_activate_callback_set
                     (_elm_access_object_get(ao), _access_base_activate_cb, obj);
                  if (!sd->title_text)
                    _elm_access_highlight_set(ao, EINA_TRUE);
               }
             if (sd->title_text)
               _elm_access_text_set(_elm_access_object_get(ao),
                                    ELM_ACCESS_INFO,
                                    sd->title_text);
          }
     }
   else
     {
        if (sd->title_text)
          {
             _elm_access_edje_object_part_object_unregister
                    (obj, elm_layout_edje_get(sd->main_layout), ACCESS_TITLE_PART);
          }

        if (sd->text_content_obj)
          {
             _elm_access_edje_object_part_object_unregister
                    (obj, elm_layout_edje_get(sd->main_layout), ACCESS_BODY_PART);
          }

        /* unregister outline */
        if (!sd->button_count)
          _elm_access_edje_object_part_object_unregister
                 (obj, elm_layout_edje_get(sd->main_layout), ACCESS_OUTLINE_PART);
     }
}

static Eina_Bool
_elm_popup_smart_theme(Evas_Object *obj)
{
   Elm_Popup_Item *item;
   Eina_List *elist;
   char buf[128];
   const char *style, *str;
   Evas_Coord x = -1;

   ELM_POPUP_DATA_GET(obj, sd);
   ELM_WIDGET_DATA_GET(obj, wsd);

   _mirrored_set(obj, elm_widget_mirrored_get(obj));

   style = elm_widget_style_get(obj);

   if (!strcmp(style, "default"))
      elm_widget_style_set(sd->notify, "popup");
   else
      elm_widget_style_set(sd->notify, style);

   if (!elm_layout_theme_set(sd->main_layout, "popup", "base", style))
     CRITICAL("Failed to set layout!");

   if (sd->button_count)
     {
        snprintf(buf, sizeof(buf), "buttons%u", sd->button_count);
        if (!elm_layout_theme_set(sd->action_area, "popup", buf, style))
          CRITICAL("Failed to set layout!");
     }

   if (sd->content_area)
     {
        if(!elm_layout_theme_set(sd->content_area, "popup", "content", style))
          CRITICAL("Failed to set layout!");
     }

   if (sd->text_content_obj)
     {
        snprintf(buf, sizeof(buf), "popup/%s", style);
        elm_object_style_set(sd->text_content_obj, buf);
     }
   if (sd->items)
     {
        evas_object_geometry_get(elm_layout_edje_get(sd->main_layout), &x, NULL, NULL, NULL);
        evas_object_move(obj, x, -1);
        if (_focus_enabled(obj))
          {
             str = edje_object_data_get(elm_layout_edje_get(sd->main_layout), "focus_highlight");
             if ((str) && (!strcmp(str, "on")))
               elm_widget_highlight_in_theme_set(obj, EINA_TRUE);
             else
               elm_widget_highlight_in_theme_set(obj, EINA_FALSE);
          }

        EINA_LIST_FOREACH(sd->items, elist, item)
          {
             if (wsd->orient_mode == 90 || wsd->orient_mode == 270)
               {
                  elm_widget_theme_object_set
                     (obj, VIEW(item), "popup", "item/landscape", elm_widget_style_get(obj));
               }
             else
               {
                  elm_widget_theme_object_set
                     (obj, VIEW(item), "popup", "item", elm_widget_style_get(obj));
               }
             _item_update(item);
          }

     }
   if (sd->title_text)
     {
        elm_layout_text_set(sd->main_layout, "elm.text.title", sd->title_text);
        elm_layout_signal_emit(sd->main_layout, "elm,state,title,text,visible", "elm");
     }
   if (sd->title_icon)
     elm_layout_signal_emit(sd->main_layout, "elm,state,title,icon,visible", "elm");
   if (sd->subtitle_text)
     {
        elm_layout_text_set(sd->main_layout, "elm.text.subtitle", sd->subtitle_text);
        elm_layout_signal_emit(sd->main_layout, "elm,state,subtitle,text,visible", "elm");
     }

   _visuals_set(obj);
   _scroller_size_calc(obj);
   elm_layout_sizing_eval(obj);

   /* access */
   if (_elm_config->access_mode) _access_obj_process(obj, EINA_TRUE);

   return EINA_TRUE;
}

static void
_item_sizing_eval(Elm_Popup_Item *item)
{
   Evas_Coord min_w = -1, min_h = -1, max_w = -1, max_h = -1;

   edje_object_size_min_restricted_calc
     (VIEW(item), &min_w, &min_h, min_w, min_h);
   evas_object_size_hint_min_set(VIEW(item), min_w, min_h);
   evas_object_size_hint_max_set(VIEW(item), max_w, max_h);
}

static void
_on_table_del(void *data,
              Evas *e __UNUSED__,
              Evas_Object *obj __UNUSED__,
              void *event_info __UNUSED__)
{
   ELM_POPUP_DATA_GET(data, sd);

   sd->tbl = NULL;
   sd->spacer = NULL;
   sd->scr = NULL;
   sd->box = NULL;
   elm_layout_sizing_eval(data);
}

static void
_create_scroller(Evas_Object *obj)
{
   ELM_POPUP_DATA_GET(obj, sd);

   //table
   sd->tbl = elm_table_add(sd->main_layout);
   evas_object_event_callback_add(sd->tbl, EVAS_CALLBACK_DEL,
                                  _on_table_del, obj);

   //spacer
   sd->spacer = evas_object_rectangle_add(evas_object_evas_get(obj));
   evas_object_color_set(sd->spacer, 0, 0, 0, 0);
   elm_table_pack(sd->tbl, sd->spacer, 0, 0, 1, 1);

   //Scroller
   sd->scr = elm_scroller_add(obj);
   elm_object_style_set(sd->scr, "effect");
   elm_scroller_policy_set(sd->scr, ELM_SCROLLER_POLICY_OFF, ELM_SCROLLER_POLICY_OFF);
   evas_object_size_hint_weight_set(sd->scr, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(sd->scr, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_scroller_content_min_limit(sd->scr, EINA_TRUE, EINA_FALSE);
   elm_scroller_bounce_set(sd->scr, EINA_FALSE, EINA_TRUE);
   evas_object_event_callback_add(sd->scr, EVAS_CALLBACK_CHANGED_SIZE_HINTS,
                                  _size_hints_changed_cb, obj);
   ELM_SCROLLABLE_IFACE_GET(sd->scr, s_iface);
   s_iface->set_auto_scroll_enabled(sd->scr, EINA_FALSE);
   elm_table_pack(sd->tbl, sd->scr, 0, 0, 1, 1);
   evas_object_show(sd->scr);
}

static void
_elm_popup_smart_sizing_eval(Evas_Object *obj)
{
   Eina_List *elist;
   Elm_Popup_Item *item;
   Evas_Coord h_box = 0, minh_box = 0;
   Evas_Coord minw = -1, minh = -1;

   ELM_POPUP_DATA_GET(obj, sd);

   if (!sd->allow_eval) return;

   sd->allow_eval = EINA_FALSE;
   if (sd->items)
     {
        EINA_LIST_FOREACH(sd->items, elist, item)
          {
             _item_sizing_eval(item);
             evas_object_size_hint_min_get(VIEW(item), NULL, &minh_box);
             if (minh_box != -1) h_box += minh_box;
          }

        evas_object_size_hint_min_set(sd->spacer, 0, MIN(h_box, sd->max_sc_h));
        evas_object_size_hint_max_set(sd->spacer, -1, sd->max_sc_h);
     }
   else if (sd->content_area)
     {
        double horizontal, vertical;
        Evas_Coord w, h;
        int w_content_area = 9999;
        int rotation = -1;
        const char *str;

        edje_object_message_signal_process(elm_layout_edje_get(sd->content_area));

        elm_popup_align_get(obj, &horizontal, &vertical);
        evas_object_geometry_get(elm_popup_parent_get(obj), NULL, NULL, &w, &h);

        if (horizontal == ELM_NOTIFY_ALIGN_FILL)
          minw = w;
        if (vertical == ELM_NOTIFY_ALIGN_FILL)
          minh = h;

        //Tizen Only: This has an UX dependency. If UX is changed this should be removed.
        rotation = elm_win_rotation_get(elm_widget_top_get(obj));
        if ((rotation == 90 || rotation == 270) && (horizontal == ELM_NOTIFY_ALIGN_FILL))
          {
             str = edje_object_data_get(elm_layout_edje_get(sd->content_area),
                                        "content_area_width");
             if (str) w_content_area = (int)(atoi(str)
                                             / edje_object_base_scale_get(elm_layout_edje_get(sd->content_area))
                                             * elm_config_scale_get()
                                             * elm_object_scale_get(obj));
          }

        if (minw > w_content_area) minw = w_content_area;

        edje_object_size_min_restricted_calc(elm_layout_edje_get(sd->content_area),
                                             &minw, &minh, minw, minh);
        evas_object_size_hint_min_set(sd->content_area, minw, minh);

        if (minh > sd->max_sc_h)
          evas_object_size_hint_min_set(sd->spacer, minw, sd->max_sc_h);
        else if (minh >= 0)
          evas_object_size_hint_min_set(sd->spacer, minw, minh);
     }

   if (sd->main_layout)
     edje_object_size_min_calc(elm_layout_edje_get(sd->main_layout), &minw, &minh);

   evas_object_size_hint_min_set(sd->main_layout, minw, minh);
   sd->allow_eval = EINA_TRUE;
}

static Eina_Bool
_elm_popup_smart_sub_object_del(Evas_Object *obj,
                                Evas_Object *sobj)
{
   Elm_Popup_Item *item;

   ELM_POPUP_DATA_GET(obj, sd);

   if (!ELM_WIDGET_CLASS(_elm_popup_parent_sc)->sub_object_del(obj, sobj))
     return EINA_FALSE;

   if (sobj == sd->title_icon)
     {
        elm_layout_signal_emit(sd->main_layout, "elm,state,title,icon,hidden", "elm");
        sd->title_icon = NULL;
     }
   else if ((item =
               evas_object_data_get(sobj, "_popup_icon_parent_item")) != NULL)
     {
        if (sobj == item->icon)
          {
             edje_object_signal_emit
               (VIEW(item), "elm,state,item,icon,hidden", "elm");
             item->icon = NULL;
          }
     }

   return EINA_TRUE;
}

static void
_on_content_del(void *data,
                Evas *e __UNUSED__,
                Evas_Object *obj __UNUSED__,
                void *event_info __UNUSED__)
{
   ELM_POPUP_DATA_GET(data, sd);

   sd->content = NULL;
   elm_layout_sizing_eval(data);
}

static void
_on_text_content_del(void *data,
                     Evas *e __UNUSED__,
                     Evas_Object *obj __UNUSED__,
                     void *event_info __UNUSED__)
{
   ELM_POPUP_DATA_GET(data, sd);

   sd->text_content_obj = NULL;
   elm_layout_sizing_eval(data);
}

static void
_button_remove(Evas_Object *obj,
               int pos,
               Eina_Bool delete)
{
   int i = 0;
   char buf[128];

   ELM_POPUP_DATA_GET(obj, sd);

   if (!sd->button_count) return;

   if (!sd->buttons[pos]) return;

   if (delete) evas_object_del(sd->buttons[pos]->btn);

   evas_object_event_callback_del
     (sd->buttons[pos]->btn, EVAS_CALLBACK_DEL, _on_button_del);
   free(sd->buttons[pos]);

   sd->buttons[pos] = NULL;
   sd->button_count -= 1;

   if (!sd->no_shift)
     {
        /* shift left the remaining buttons */
        for (i = pos; i < ELM_POPUP_ACTION_BUTTON_MAX - 1; i++)
          {
             sd->buttons[i] = sd->buttons[i + 1];

             snprintf(buf, sizeof(buf), "actionbtn%d", pos + 1);
             elm_object_part_content_unset(sd->action_area, buf);
             snprintf(buf, sizeof(buf), "actionbtn%d", pos);
             elm_object_part_content_set
               (sd->action_area, buf, sd->buttons[i]->btn);
          }
     }

   if (!sd->button_count)
     {
        _visuals_set(obj);
        elm_layout_content_unset(sd->main_layout, "elm.swallow.action_area");
        evas_object_hide(sd->action_area);
     }
   else
     {
        snprintf(buf, sizeof(buf), "buttons%u", sd->button_count);
        elm_layout_theme_set
          (sd->action_area, "popup", buf, elm_widget_style_get(obj));
     }
}

static void
_layout_change_cb(void *data,
                  Evas_Object *obj __UNUSED__,
                  const char *emission __UNUSED__,
                  const char *source __UNUSED__)
{
   elm_layout_sizing_eval(data);
}

static void
_restack_cb(void *data __UNUSED__,
            Evas *e __UNUSED__,
            Evas_Object *obj,
            void *event_info __UNUSED__)
{
   ELM_POPUP_DATA_GET(obj, sd);

   evas_object_raise(sd->notify);
}

static void
_list_add(Evas_Object *obj)
{
   ELM_POPUP_DATA_GET(obj, sd);

   sd->box = elm_box_add(sd->scr);
   evas_object_size_hint_weight_set(sd->box, EVAS_HINT_EXPAND, 0.0);
   evas_object_size_hint_align_set(sd->box, EVAS_HINT_FILL, 0.0);
   elm_object_content_set(sd->scr, sd->box);
}

static void
_item_select_cb(void *data,
                Evas_Object *obj __UNUSED__,
                const char *emission __UNUSED__,
                const char *source __UNUSED__)
{
   Elm_Popup_Item *item = data;

   if (!item || item->disabled) return;
   if (item->func)
     item->func((void *)item->base.data, WIDGET(item), data);
}

#ifdef _TIZEN_
static void
_item_bottomline_hide_cb(void *data,
                Evas_Object *obj __UNUSED__,
                const char *emission __UNUSED__,
                const char *source __UNUSED__)
{
   Elm_Popup_Item *item = data;

   if (!item || item->disabled) return;
   item->bottom_line = EINA_TRUE;
   edje_object_signal_emit
     (VIEW(item), "elm,state,item,bottomline,hide", "elm");
}

static void
_item_bottomline_show_cb(void *data,
                Evas_Object *obj __UNUSED__,
                const char *emission __UNUSED__,
                const char *source __UNUSED__)
{
   Elm_Popup_Item *item = data;

   if (!item || item->disabled) return;
   item->bottom_line = EINA_FALSE;
   edje_object_signal_emit
     (VIEW(item), "elm,state,item,bottomline,show", "elm");
}
#endif

static void
_item_text_set(Elm_Popup_Item *item,
               const char *label)
{
   if (!eina_stringshare_replace(&item->label, label)) return;

   edje_object_part_text_escaped_set(VIEW(item), "elm.text", label);

   if (item->label)
     edje_object_signal_emit
       (VIEW(item), "elm,state,item,text,visible", "elm");
   else
     edje_object_signal_emit
       (VIEW(item), "elm,state,item,text,hidden", "elm");

   edje_object_message_signal_process(VIEW(item));
}

static void
_item_text_set_hook(Elm_Object_Item *it,
                    const char *part,
                    const char *label)
{
   Elm_Popup_Item *item = (Elm_Popup_Item *)it;

   ELM_POPUP_ITEM_CHECK_OR_RETURN(it);

   if ((!part) || (!strcmp(part, "default")))
     {
        _item_text_set(item, label);
        return;
     }

   WRN("The part name is invalid! : popup=%p", WIDGET(item));
}

static const char *
_item_text_get_hook(const Elm_Object_Item *it,
                    const char *part)
{
   Elm_Popup_Item *item = (Elm_Popup_Item *)it;

   ELM_POPUP_ITEM_CHECK_OR_RETURN(it, NULL);

   if ((!part) || (!strcmp(part, "default")))
     return item->label;

   WRN("The part name is invalid! : popup=%p", WIDGET(item));

   return NULL;
}

static void
_item_icon_set(Elm_Popup_Item *item,
               Evas_Object *icon)
{
   if (item->icon == icon) return;

   if (item->icon)
     evas_object_del(item->icon);

   item->icon = icon;
   if (item->icon)
     {
        elm_widget_sub_object_add(WIDGET(item), item->icon);
        evas_object_data_set(item->icon, "_popup_icon_parent_item", item);
        edje_object_part_swallow
          (VIEW(item), "elm.swallow.content", item->icon);
        edje_object_signal_emit
          (VIEW(item), "elm,state,item,icon,visible", "elm");
     }
   else
     edje_object_signal_emit(VIEW(item), "elm,state,item,icon,hidden", "elm");

   edje_object_message_signal_process(item->base.view);
}

static void
_item_content_set_hook(Elm_Object_Item *it,
                       const char *part,
                       Evas_Object *content)
{
   Elm_Popup_Item *item = (Elm_Popup_Item *)it;

   ELM_POPUP_ITEM_CHECK_OR_RETURN(it);

   if ((!(part)) || (!strcmp(part, "default")))
     _item_icon_set(item, content);
   else
     WRN("The part name is invalid! : popup=%p", WIDGET(item));
}

static Evas_Object *
_item_content_get_hook(const Elm_Object_Item *it,
                       const char *part)
{
   Elm_Popup_Item *item = (Elm_Popup_Item *)it;

   ELM_POPUP_ITEM_CHECK_OR_RETURN(it, NULL);

   if ((!(part)) || (!strcmp(part, "default")))
     return item->icon;

   WRN("The part name is invalid! : popup=%p", WIDGET(item));

   return NULL;
}

static Evas_Object *
_item_icon_unset(Elm_Popup_Item *item)
{
   Evas_Object *icon = item->icon;

   if (!item->icon) return NULL;
   elm_widget_sub_object_del(WIDGET(item), icon);
   evas_object_data_del(icon, "_popup_icon_parent_item");
   edje_object_part_unswallow(item->base.view, icon);
   edje_object_signal_emit(VIEW(item), "elm,state,item,icon,hidden", "elm");
   item->icon = NULL;

   return icon;
}

static Evas_Object *
_item_content_unset_hook(const Elm_Object_Item *it,
                         const char *part)
{
   Evas_Object *content = NULL;
   Elm_Popup_Item *item = (Elm_Popup_Item *)it;

   ELM_POPUP_ITEM_CHECK_OR_RETURN(it, NULL);

   if ((!(part)) || (!strcmp(part, "default")))
     content = _item_icon_unset(item);
   else
     WRN("The part name is invalid! : popup=%p", WIDGET(item));

   return content;
}

static void
_item_disable_hook(Elm_Object_Item *it)
{
   Elm_Popup_Item *item = (Elm_Popup_Item *)it;

   ELM_POPUP_ITEM_CHECK_OR_RETURN(it);

   if (elm_widget_item_disabled_get(it))
     edje_object_signal_emit(VIEW(item), "elm,state,item,disabled", "elm");
   else
     edje_object_signal_emit(VIEW(item), "elm,state,item,enabled", "elm");
}

static Eina_Bool
_item_del_pre_hook(Elm_Object_Item *it)
{
   Elm_Popup_Item *item = (Elm_Popup_Item *)it;

   ELM_POPUP_ITEM_CHECK_OR_RETURN(it, EINA_FALSE);

#ifdef _TIZEN_
   item->bottom_line = EINA_FALSE;
#endif
   if (item->icon)
     evas_object_del(item->icon);

   eina_stringshare_del(item->label);

   return EINA_TRUE;
}

static void
_item_signal_emit_hook(Elm_Object_Item *it,
                       const char *emission,
                       const char *source)
{
   Elm_Popup_Item *item = (Elm_Popup_Item *)it;

   ELM_POPUP_ITEM_CHECK_OR_RETURN(it);

   edje_object_signal_emit(VIEW(item), emission, source);
}

static void
_access_activate_cb(void *data __UNUSED__,
                    Evas_Object *part_obj __UNUSED__,
                    Elm_Object_Item *item)
{
   _item_select_cb(item, NULL, NULL, NULL);
}

static char *
_access_state_cb(void *data, Evas_Object *obj __UNUSED__)
{
   Elm_Popup_Item *it = (Elm_Popup_Item *)data;
   if (!it) return NULL;

   if (it->base.disabled)
     return strdup(E_("IDS_ACCS_BODY_DISABLED_TTS"));

   return NULL;
}

static char *
_access_info_cb(void *data, Evas_Object *obj __UNUSED__)
{
   Elm_Popup_Item *it = (Elm_Popup_Item *)data;
   const char *txt = NULL;
   Evas_Object *icon = NULL;
   Eina_Strbuf *buf = NULL;
   char *str = NULL;

   if (!it) return NULL;

   txt = it->label;
   icon = it->icon;

   if (txt && icon)
     {
        buf = eina_strbuf_new();
        eina_strbuf_append(buf, E_("IDS_TPLATFORM_BODY_ICON"));
        eina_strbuf_append(buf, txt);
        str = eina_strbuf_string_steal(buf);
        eina_strbuf_free(buf);
        return str;
     }
   else if ((!txt) && icon) return strdup(E_("IDS_TPLATFORM_BODY_ICON"));
   else if (txt && (!icon)) return strdup(txt);

   return NULL;
}

static void
_access_widget_item_register(Elm_Popup_Item *it)
{
   Elm_Access_Info *ao;

   _elm_access_widget_item_register((Elm_Widget_Item *)it);
   ao = _elm_access_object_get(it->base.access_obj);
   _elm_access_callback_set(ao, ELM_ACCESS_INFO, _access_info_cb, it);
   _elm_access_callback_set(ao, ELM_ACCESS_STATE, _access_state_cb, it);
   _elm_access_text_set(ao, ELM_ACCESS_TYPE, E_("Popup_list"));
   _elm_access_activate_callback_set(ao, _access_activate_cb, it);

}

static void
_item_new(Elm_Popup_Item *item)
{
   int orientation = -1;

   ELM_POPUP_DATA_GET(WIDGET(item), sd);

   elm_widget_item_text_set_hook_set(item, _item_text_set_hook);
   elm_widget_item_text_get_hook_set(item, _item_text_get_hook);
   elm_widget_item_content_set_hook_set(item, _item_content_set_hook);
   elm_widget_item_content_get_hook_set(item, _item_content_get_hook);
   elm_widget_item_content_unset_hook_set(item, _item_content_unset_hook);
   elm_widget_item_disable_hook_set(item, _item_disable_hook);
   elm_widget_item_del_pre_hook_set(item, _item_del_pre_hook);
   elm_widget_item_signal_emit_hook_set(item, _item_signal_emit_hook);
   VIEW(item) = edje_object_add
       (evas_object_evas_get(ELM_WIDGET_DATA(sd)->obj));

   orientation = sd->orientation;
   if (orientation == 90 || orientation == 270)
      elm_widget_theme_object_set
         (WIDGET(item), VIEW(item), "popup", "item/landscape",
          elm_widget_style_get(WIDGET(item)));
   else
      elm_widget_theme_object_set
         (WIDGET(item), VIEW(item), "popup", "item", elm_widget_style_get(WIDGET(item)));
   edje_object_mirrored_set(VIEW(item), elm_widget_mirrored_get(WIDGET(item)));
   edje_object_signal_callback_add
     (VIEW(item), "elm,action,click", "", _item_select_cb, item);
   evas_object_size_hint_align_set
     (VIEW(item), EVAS_HINT_FILL, EVAS_HINT_FILL);
#ifdef _TIZEN_
   edje_object_signal_callback_add
     (VIEW(item), "elm,action,bottomline,show", "", _item_bottomline_show_cb, item);
   edje_object_signal_callback_add
     (VIEW(item), "elm,action,bottomline,hide", "", _item_bottomline_hide_cb, item);

#endif
   evas_object_event_callback_add
     (VIEW(item), EVAS_CALLBACK_MOUSE_DOWN, _mouse_down_cb, item);
   evas_object_event_callback_add
     (VIEW(item), EVAS_CALLBACK_MOUSE_UP, _mouse_up_cb, item);

   evas_object_event_callback_add
     (VIEW(item), EVAS_CALLBACK_MOUSE_MOVE, _mouse_move_cb, item);

   /* access */
   if (_elm_config->access_mode) _access_widget_item_register(item);

   evas_object_show(VIEW(item));
}

static Eina_Bool
_subtitle_text_set(Evas_Object *obj,
                const char *text)
{
   ELM_POPUP_DATA_GET(obj, sd);

   if (sd->subtitle_text == text) return EINA_TRUE;

   eina_stringshare_replace(&sd->subtitle_text, text);

   //bare edje here because we're inside the hook, already
   edje_object_part_text_escaped_set
     (elm_layout_edje_get(sd->main_layout), "elm.text.subtitle", text);

   if (sd->subtitle_text)
     elm_object_signal_emit(sd->main_layout, "elm,state,subtitle,text,visible", "elm");
   else
     elm_object_signal_emit(sd->main_layout, "elm,state,subtitle,text,hidden", "elm");

   edje_object_message_signal_process(elm_layout_edje_get(sd->main_layout));
   _scroller_size_calc(obj);
   elm_layout_sizing_eval(obj);

   return EINA_TRUE;
}

static Eina_Bool
_title_text_set(Evas_Object *obj,
                const char *text)
{
   Evas_Object *ao;
   char *mkup_text = NULL;

   Eina_Bool title_visibility_old, title_visibility_current;

   ELM_POPUP_DATA_GET(obj, sd);

   if (sd->title_text == text) return EINA_TRUE;

   title_visibility_old = (sd->title_text) || (sd->title_icon);
   eina_stringshare_replace(&sd->title_text, text);

   elm_layout_text_set(sd->main_layout, "elm.text.title", text);

   /* access */
   if (_elm_config->access_mode)
     {
        mkup_text = _elm_util_mkup_to_text(text);
        if (!mkup_text) return EINA_FALSE;

        ao = _access_object_get(obj, ACCESS_TITLE_PART);
        if (!ao)
          {

             ao = _elm_access_edje_object_part_object_register
                    (obj, elm_layout_edje_get(sd->main_layout), ACCESS_TITLE_PART);
             evas_object_smart_callback_add(ao, "access,read,stop", _access_title_read_stop_cb, obj);
             evas_object_smart_callback_add(ao, "access,read,cancel", _access_title_read_cancel_cb, obj);
          }
        _elm_access_text_set(_elm_access_object_get(ao), ELM_ACCESS_INFO, mkup_text);

        free(mkup_text);
        mkup_text = NULL;

        ao = _access_object_get(obj, ACCESS_OUTLINE_PART);
        if (ao)
          _elm_access_text_set(_elm_access_object_get(ao),
                               ELM_ACCESS_INFO, sd->title_text);
     }

   if (sd->title_text)
     elm_layout_signal_emit(sd->main_layout, "elm,state,title,text,visible", "elm");
   else
     elm_layout_signal_emit(sd->main_layout, "elm,state,title,text,hidden", "elm");

   title_visibility_current = (sd->title_text) || (sd->title_icon);

   if (title_visibility_old != title_visibility_current)
     _visuals_set(obj);

   _scroller_size_calc(obj);
   elm_layout_sizing_eval(obj);

   return EINA_TRUE;
}

static Eina_Bool
_content_text_set(Evas_Object *obj,
                  const char *text)
{
   Evas_Object *prev_content, *ao;
   char buf[128];

   ELM_POPUP_DATA_GET(obj, sd);

   if (sd->items) _items_remove(sd);

   if (!sd->content_area)
     {
        sd->content_area = elm_layout_add(sd->main_layout);
        if (!elm_layout_theme_set(sd->content_area, "popup", "content",
                                  elm_widget_style_get(obj)))
          CRITICAL("Failed to set layout!");
        else
          evas_object_event_callback_add
             (sd->content_area, EVAS_CALLBACK_CHANGED_SIZE_HINTS,
              _size_hints_changed_cb, obj);
     }

   prev_content = elm_layout_content_get(sd->content_area, "elm.swallow.content");
   if (prev_content) evas_object_del(prev_content);

   if (!text) goto end;

   elm_object_content_set(sd->scr, sd->content_area);

   sd->text_content_obj = elm_label_add(obj);
   snprintf(buf, sizeof(buf), "popup/%s", elm_widget_style_get(obj));
   elm_object_style_set(sd->text_content_obj, buf);

   evas_object_event_callback_add
     (sd->text_content_obj, EVAS_CALLBACK_DEL, _on_text_content_del, obj);

   elm_label_line_wrap_set(sd->text_content_obj, sd->content_text_wrap_type);
   elm_object_text_set(sd->text_content_obj, text);
   evas_object_size_hint_weight_set
     (sd->text_content_obj, EVAS_HINT_EXPAND, 0.0);
   evas_object_size_hint_align_set
     (sd->text_content_obj, EVAS_HINT_FILL, EVAS_HINT_FILL);

   elm_layout_content_set
     (sd->content_area, "elm.swallow.content", sd->text_content_obj);

   _elm_access_edje_object_part_object_unregister
      (obj, elm_layout_edje_get(sd->main_layout), ACCESS_OUTLINE_PART);

   /* access */
   if (_elm_config->access_mode)
     {
        /* unregister label, ACCESS_BODY_PART will register */
        elm_access_object_unregister(sd->text_content_obj);
        char *mkup_text = _elm_util_mkup_to_text(text);
        if (!mkup_text) return EINA_FALSE;

        ao = _access_object_get(obj, ACCESS_BODY_PART);
        if (!ao)
          {
             ao = _elm_access_edje_object_part_object_register
                    (obj, elm_layout_edje_get(sd->main_layout), ACCESS_BODY_PART);
          }
        _elm_access_activate_callback_set
          (_elm_access_object_get(ao), _access_base_activate_cb, obj);
        _elm_access_text_set(_elm_access_object_get(ao), ELM_ACCESS_INFO, mkup_text);
        free(mkup_text);
        mkup_text = NULL;
     }

end:
   elm_layout_sizing_eval(obj);

   return EINA_TRUE;
}

static Eina_Bool
_elm_popup_smart_text_set(Evas_Object *obj,
                          const char *part,
                          const char *label)
{
   ELM_POPUP_DATA_GET(obj, sd);

   if (!part || !strcmp(part, "default"))
     return _content_text_set(obj, label);
   else if (!strcmp(part, "title,text"))
     return _title_text_set(obj, label);
   else if (!strcmp(part, "subtitle,text"))
     return _subtitle_text_set(obj, label);
   else
     return elm_layout_text_set(sd->main_layout, part, label);
}

static const char *
_subtitle_text_get(const Evas_Object *obj)
{
   ELM_POPUP_DATA_GET(obj, sd);

   return sd->subtitle_text;
}

static const char *
_title_text_get(const Evas_Object *obj)
{
   ELM_POPUP_DATA_GET(obj, sd);

   return sd->title_text;
}

static const char *
_content_text_get(const Evas_Object *obj)
{
   const char *str = NULL;

   ELM_POPUP_DATA_GET(obj, sd);

   if (sd->text_content_obj)
     str = elm_object_text_get(sd->text_content_obj);

   return str;
}

static const char *
_elm_popup_smart_text_get(const Evas_Object *obj,
                          const char *part)
{
   const char *str = NULL;
   ELM_POPUP_DATA_GET(obj, sd);

   if (!part || !strcmp(part, "default"))
     str = _content_text_get(obj);
   else if (!strcmp(part, "title,text"))
     str = _title_text_get(obj);
   else if (!strcmp(part, "subtitle,text"))
     str = _subtitle_text_get(obj);
   else
     str = elm_layout_text_get(sd->main_layout, part);

   return str;
}

static Eina_Bool
_title_icon_set(Evas_Object *obj,
                Evas_Object *icon)
{
   Eina_Bool title_visibility_old, title_visibility_current;

   ELM_POPUP_DATA_GET(obj, sd);

   if (sd->title_icon == icon) return EINA_TRUE;
   title_visibility_old = (sd->title_text) || (sd->title_icon);
   if (sd->title_icon) evas_object_del(sd->title_icon);

   sd->title_icon = icon;
   title_visibility_current = (sd->title_text) || (sd->title_icon);

   elm_layout_content_set
     (sd->main_layout, "elm.swallow.title.icon", sd->title_icon);

   if (sd->title_icon)
     elm_layout_signal_emit(sd->main_layout, "elm,state,title,icon,visible", "elm");
   if (title_visibility_old != title_visibility_current) _visuals_set(obj);

   _scroller_size_calc(obj);
   elm_layout_sizing_eval(obj);

   return EINA_TRUE;
}

static Eina_Bool
_content_set(Evas_Object *obj,
             Evas_Object *content)
{
   Evas_Object *prev_content;

   ELM_POPUP_DATA_GET(obj, sd);

   if (sd->content && sd->content == content) return EINA_TRUE;
   if (sd->items) _items_remove(sd);

   if (!sd->content_area)
     {
        sd->content_area = elm_layout_add(sd->main_layout);
        if (!elm_layout_theme_set(sd->content_area, "popup", "content",
                                  elm_widget_style_get(obj)))
          CRITICAL("Failed to set layout!");
        else
          evas_object_event_callback_add
             (sd->content_area, EVAS_CALLBACK_CHANGED_SIZE_HINTS,
              _size_hints_changed_cb, obj);
     }

   prev_content = elm_layout_content_get(sd->content_area, "elm.swallow.content");
   if (prev_content) evas_object_del(prev_content);

   sd->content = content;
   if (content)
     {
        evas_object_show(sd->content_area);
        elm_object_content_set(sd->scr, sd->content_area);

        elm_layout_content_set
          (sd->content_area, "elm.swallow.content", content);
        evas_object_show(content);

        evas_object_event_callback_add
          (content, EVAS_CALLBACK_DEL, _on_content_del, obj);
     }
   elm_layout_sizing_eval(obj);

   return EINA_TRUE;
}

static void
_action_button_set(Evas_Object *obj,
                   Evas_Object *btn,
                   unsigned int idx)
{
   Action_Area_Data *adata;
   char buf[128];
   Evas_Object *ao;

   ELM_POPUP_DATA_GET(obj, sd);

   if (idx >= ELM_POPUP_ACTION_BUTTON_MAX) return;

   if (!btn)
     {
        _button_remove(obj, idx, EINA_TRUE);
        return;
     }

   if (!sd->buttons[idx]) sd->button_count++;
   else
     {
        sd->no_shift = EINA_TRUE;
        evas_object_del(sd->buttons[idx]->btn);
        ELM_SAFE_FREE(sd->buttons[idx], free);
        sd->no_shift = EINA_FALSE;
     }

   if (_elm_config->access_mode)
     {
        /* if popup has a button, popup should be closed by the button
           so outline(ACCESS_OUTLINE_PART) is not necessary any more */
        ao = _access_object_get(obj, ACCESS_OUTLINE_PART);

        if (ao && sd->button_count)
          _elm_access_edje_object_part_object_unregister
            (obj, elm_layout_edje_get(sd->main_layout), ACCESS_OUTLINE_PART);

        ao = _access_object_get(obj, ACCESS_BODY_PART);
        if (ao && sd->button_count)
        _elm_access_activate_callback_set
          (_elm_access_object_get(ao), NULL, obj);
     }

   if (!sd->action_area)
     {
        sd->action_area = elm_layout_add(sd->main_layout);
        evas_object_event_callback_add
           (sd->action_area, EVAS_CALLBACK_CHANGED_SIZE_HINTS,
            _size_hints_changed_cb, obj);
     }

   snprintf(buf, sizeof(buf), "buttons%u", sd->button_count);
   elm_layout_theme_set
     (sd->action_area, "popup", buf, elm_widget_style_get(obj));

   adata = ELM_NEW(Action_Area_Data);
   adata->obj = obj;
   adata->btn = btn;

   evas_object_event_callback_add
     (btn, EVAS_CALLBACK_DEL, _on_button_del, obj);

   sd->buttons[idx] = adata;

   snprintf(buf, sizeof(buf), "actionbtn%u", idx + 1);
   elm_object_part_content_set
     (sd->action_area, buf, sd->buttons[idx]->btn);
   evas_object_show(sd->buttons[idx]->btn);

   elm_layout_content_set
     (sd->main_layout, "elm.swallow.action_area", sd->action_area);
   evas_object_show(sd->action_area);
   if (sd->button_count == 1) _visuals_set(obj);

   _scroller_size_calc(obj);

   elm_layout_sizing_eval(obj);
}

static Eina_Bool
_elm_popup_smart_content_set(Evas_Object *obj,
                             const char *part,
                             Evas_Object *content)
{
   ELM_POPUP_DATA_GET(obj, sd);
   unsigned int i;

   if (!part || !strcmp(part, "default"))
     return _content_set(obj, content);
   else if (!strcmp(part, "title,icon"))
     return _title_icon_set(obj, content);
   else if (!strncmp(part, "button", 6))
     {
        i = atoi(part + 6) - 1;

        if (i >= ELM_POPUP_ACTION_BUTTON_MAX)
          goto err;

        _action_button_set(obj, content, i);

        return EINA_TRUE;
     }
#ifdef ELM_FEATURE_WEARABLE_B3
   else if (!strcmp(part, "toast,icon"))
     elm_layout_content_set(sd->main_layout, part, content);
#endif
   else elm_layout_content_set(sd->main_layout, part, content);

#ifdef ELM_FEATURE_WEARABLE_C1
   if (evas_object_visible_get(obj))
     _create_popup_access_chain(obj);
#endif

err:
   ERR("The part name is invalid! : popup=%p", obj);

   return EINA_FALSE;
}

static Evas_Object *
_title_icon_get(const Evas_Object *obj)
{
   ELM_POPUP_DATA_GET(obj, sd);

   return sd->title_icon;
}

static Evas_Object *
_content_get(const Evas_Object *obj)
{
   ELM_POPUP_DATA_GET(obj, sd);

   return sd->content;
}

static Evas_Object *
_action_button_get(const Evas_Object *obj,
                   unsigned int idx)
{
   Evas_Object *button = NULL;

   ELM_POPUP_DATA_GET(obj, sd);
   if (!sd->button_count) return NULL;

   if (sd->buttons[idx])
     button = sd->buttons[idx]->btn;

   return button;
}

static Evas_Object *
_elm_popup_smart_content_get(const Evas_Object *obj,
                             const char *part)
{
   ELM_POPUP_DATA_GET(obj, sd);
   Evas_Object *content = NULL;
   unsigned int i;

   if (!part || !strcmp(part, "default"))
     content = _content_get(obj);
   else if (!strcmp(part, "title,text"))
     content = _title_icon_get(obj);
   else if (!strncmp(part, "button", 6))
     {
        i = atoi(part + 6) - 1;

        if (i >= ELM_POPUP_ACTION_BUTTON_MAX)
          goto err;

        content = _action_button_get(obj, i);
     }
   else
     content = elm_layout_content_get(sd->main_layout, part);

   return content;

err:
   WRN("The part name is invalid! : popup=%p", obj);
   return content;
}

static Evas_Object *
_content_unset(Evas_Object *obj)
{
   Evas_Object *content;

   ELM_POPUP_DATA_GET(obj, sd);

   if (!sd->content) return NULL;

   evas_object_event_callback_del
     (sd->content, EVAS_CALLBACK_DEL, _on_content_del);

   content = elm_layout_content_unset(sd->content_area, "elm.swallow.content");
   sd->content = NULL;

   elm_layout_sizing_eval(obj);

   return content;
}

static Evas_Object *
_title_icon_unset(Evas_Object *obj)
{
   Evas_Object *icon;

   ELM_POPUP_DATA_GET(obj, sd);

   if (!sd->title_icon) return NULL;

   icon = sd->title_icon;
   elm_layout_content_unset(sd->main_layout, "elm.swallow.title.icon");
   sd->title_icon = NULL;

   return icon;
}

static Evas_Object *
_elm_popup_smart_content_unset(Evas_Object *obj,
                               const char *part)
{
   Evas_Object *content = NULL;
   unsigned int i;

   if (!part || !strcmp(part, "default"))
     content = _content_unset(obj);
   else if (!strcmp(part, "title,icon"))
     content = _title_icon_unset(obj);
   else if (!strncmp(part, "button", 6))
     {
        i = atoi(part + 6) - 1;

        if (i >= ELM_POPUP_ACTION_BUTTON_MAX)
          goto err;

        _button_remove(obj, i, EINA_FALSE);
     }
   else
     goto err;

   return content;

err:
   ERR("The part name is invalid! : popup=%p", obj);

   return NULL;
}

static Eina_Bool
_elm_popup_smart_focus_next(const Evas_Object *obj,
                            Elm_Focus_Direction dir,
                            Evas_Object **next)
{
   Evas_Object *ao;
   Eina_List *items = NULL;
   Elm_Popup_Item * it = NULL;
   Eina_List *l;
   Eina_Bool ret = EINA_FALSE;

   ELM_POPUP_DATA_GET(obj, sd);

   /* access */
   if (_elm_config->access_mode)
     {
        ao = _access_object_get(obj, ACCESS_OUTLINE_PART);
        if (ao) items = eina_list_append(items, ao);

        if (sd->title_text)
          {
             ao = _access_object_get(obj, ACCESS_TITLE_PART);
             items = eina_list_append(items, ao);
          }

        ao = _access_object_get(obj, ACCESS_BODY_PART);
        if (ao) items = eina_list_append(items, ao);

        EINA_LIST_FOREACH(sd->items, l, it)
          items = eina_list_append(items, it->base.access_obj);
     }

   /* content area */
   if (sd->content) items = eina_list_append(items, sd->tbl);

   /* action area */
   if (sd->button_count) items = eina_list_append(items, sd->action_area);

   if (_elm_config->access_mode)
     {
        _elm_access_auto_highlight_set(EINA_TRUE);
        ret = elm_widget_focus_list_next_get
           (sd->main_layout, items, eina_list_data_get, dir, next);
        _elm_access_auto_highlight_set(EINA_FALSE);
        eina_list_free(items);
        return ret;
     }

   if (!elm_widget_focus_list_next_get
       (sd->main_layout, items, eina_list_data_get, dir, next))
     *next = (Evas_Object *)obj;
   eina_list_free(items);

   return EINA_TRUE;
}

static Eina_Bool
_elm_popup_smart_focus_direction_manager_is(const Evas_Object *obj __UNUSED__)
{
   return EINA_TRUE;
}

static Eina_Bool
_elm_popup_smart_focus_direction(const Evas_Object *obj,
                                 const Evas_Object *base,
                                 double degree,
                                 Evas_Object **direction,
                                 double *weight)
{
   Evas_Object *ao;
   Eina_List *items = NULL;
   Elm_Popup_Item * it = NULL;
   Eina_List *l;

   ELM_POPUP_DATA_GET(obj, sd);

   /* access */
   if (_elm_config->access_mode)
     {
        ao = _access_object_get(obj, ACCESS_OUTLINE_PART);
        if (ao) items = eina_list_append(items, ao);

        if (sd->title_text)
          {
             ao = _access_object_get(obj, ACCESS_TITLE_PART);
             items = eina_list_append(items, ao);
          }

        ao = _access_object_get(obj, ACCESS_BODY_PART);
        if (ao) items = eina_list_append(items, ao);

        EINA_LIST_FOREACH(sd->items, l, it)
          items = eina_list_append(items, it->base.access_obj);
     }

   /* content area */
   if (sd->content) items = eina_list_append(items, sd->content_area);

   /* action area */
   if (sd->button_count) items = eina_list_append(items, sd->action_area);

   elm_widget_focus_list_direction_get
     (sd->main_layout, base, items, eina_list_data_get, degree, direction, weight);

   return EINA_TRUE;
}

// FIXME: There are applications which do not use elm_win as top widget.
// This is workaround! Those could not use focus!
static Eina_Bool _focus_enabled(Evas_Object *obj)
{
   if (!elm_widget_focus_get(obj)) return EINA_FALSE;

   const Evas_Object *win = elm_widget_top_get(obj);
   const char *type = evas_object_type_get(win);

   if (type && !strcmp(type, "elm_win"))
     {
        return elm_win_focus_highlight_enabled_get(win);
     }
   return EINA_FALSE;
}

static Eina_Bool
_elm_popup_smart_on_focus(Evas_Object *obj, Elm_Focus_Info *info)
{
   Elm_Popup_Item *item;
   ELM_POPUP_DATA_GET(obj, sd);

   if (!ELM_WIDGET_CLASS(_elm_popup_parent_sc)->on_focus(obj, info))
     return EINA_FALSE;

   if (sd->items)
     {
        if (elm_widget_focus_get(obj))
          {
             if (_focus_enabled(obj))
               {
                  if (!sd->focused) sd->focused = sd->items;
                  item = eina_list_data_get(sd->focused);
                  edje_object_signal_emit(VIEW(item), "elm,action,focus_highlight,show", "elm");
                  evas_object_smart_callback_call(obj, "item,focused", item);
               }
          }
        else
          {
             if (sd->focused)
               {
                  item = eina_list_data_get(sd->focused);
                  edje_object_signal_emit(VIEW(item), "elm,action,focus_highlight,hide", "elm");
                  evas_object_smart_callback_call(obj, "item,unfocused", item);
               }
          }
     }
   return EINA_TRUE;
}

static void _rotation_changed_cb(void *data,
                               Evas_Object *o __UNUSED__,
                               const char *emission __UNUSED__,
                               const char *source __UNUSED__)
{
   int rotation = -1;
   Eina_List *elist;
   Evas_Object *popup = data;
   Elm_Popup_Item *item;

   ELM_POPUP_CHECK(popup);
   ELM_POPUP_DATA_GET(popup, sd);

   rotation = ELM_WIDGET_DATA(sd)->orient_mode;

   if (sd->orientation == rotation) return;

   sd->orientation = rotation;

   if (sd->items)
     {
        EINA_LIST_FOREACH(sd->items, elist, item)
        {
           if (rotation == 90 || rotation == 270)
             {
                elm_widget_theme_object_set
                   (WIDGET(item), VIEW(item), "popup", "item/landscape",
                    elm_widget_style_get(WIDGET(item)));
             }
           else
             {
                elm_widget_theme_object_set
                   (WIDGET(item), VIEW(item), "popup", "item",
                    elm_widget_style_get(WIDGET(item)));
             }
           _item_update(item);
           if (elm_widget_item_disabled_get(item))
              edje_object_signal_emit
                 (VIEW(item), "elm,state,item,disabled", "elm");
        }
      if (sd->focused)
        {
           item = eina_list_data_get(sd->focused);
           edje_object_signal_emit(VIEW(item), "elm,action,focus_highlight,show", "elm");
        }
   }

   _scroller_size_calc(popup);
   elm_layout_sizing_eval(popup);
   elm_layout_sizing_eval(sd->content);
}

static void
_notify_resize_cb(void *data,
                  Evas *e EINA_UNUSED,
                  Evas_Object *obj EINA_UNUSED,
                  void *event_info EINA_UNUSED)
{
   Evas_Object *popup = data;

   _scroller_size_calc(popup);
   elm_layout_sizing_eval(popup);
}

static void
_elm_popup_smart_add(Evas_Object *obj)
{
   EVAS_SMART_DATA_ALLOC(obj, Elm_Popup_Smart_Data);

   ELM_WIDGET_CLASS(_elm_popup_parent_sc)->base.add(obj);

}

static void
_elm_popup_smart_parent_set(Evas_Object *obj,
                            Evas_Object *parent)
{
   ELM_POPUP_DATA_GET(obj, sd);

   if (sd->notify)
     elm_notify_parent_set(sd->notify, parent);
}

static void
_elm_popup_smart_access(Evas_Object *obj, Eina_Bool is_access)
{
   _access_obj_process(obj, is_access);

   evas_object_smart_callback_call(obj, SIG_ACCESS_CHANGED, NULL);
}

static Evas_Object *
_elm_popup_smart_access_object_get(Evas_Object *obj, char *part)
{
   ELM_POPUP_CHECK(obj) NULL;

   return _access_object_get(obj, part);
}

static void
_elm_popup_item_coordinates_calc(Elm_Popup_Item *item,
                                   Evas_Coord *x,
                                   Evas_Coord *y,
                                   Evas_Coord *w,
                                   Evas_Coord *h)
{
   Evas_Coord ix, iy, iw, ih, bx, by, vw, vh;

   ELM_POPUP_DATA_GET(WIDGET(item), sd);
   elm_scroller_region_get(sd->scr, NULL, NULL, &vw, &vh);
   evas_object_geometry_get(sd->box, &bx, &by, NULL, NULL);
   evas_object_geometry_get(VIEW(item), &ix, &iy, &iw, &ih);
   *x = ix - bx;
   *y = iy - by;
   *w = iw;
   *h = ih;
}

static Eina_Bool
_elm_popup_smart_event(Evas_Object *obj,
                       Evas_Object *src __UNUSED__,
                       Evas_Callback_Type type,
                       void *event_info)
{
   Evas_Event_Key_Down *ev = event_info;
   ELM_POPUP_DATA_GET(obj, sd);
   Elm_Popup_Item *item = NULL;
   Elm_Popup_Item *temp_item = NULL;
   Eina_List *list = NULL;
   Evas_Coord x, y, w, h;

   // TIZEN ONLY(20131221) : When access mode, focused ui is disabled.
   if (_elm_config->access_mode) return EINA_FALSE;

   if (elm_widget_disabled_get(obj)) return EINA_FALSE;
   if(!_focus_enabled(obj)) return EINA_FALSE;
   if (type != EVAS_CALLBACK_KEY_DOWN) return EINA_FALSE;
   if (ev->event_flags & EVAS_EVENT_FLAG_ON_HOLD) return EINA_FALSE;
   if (!sd->focused) sd->focused = sd->items;
   temp_item = eina_list_data_get(sd->focused);

   if (!strcmp(ev->keyname, "Tab"))
     {
        if (evas_key_modifier_is_set(ev->modifiers, "Shift"))
          elm_widget_focus_cycle(obj, ELM_FOCUS_PREVIOUS);
        else
          elm_widget_focus_cycle(obj, ELM_FOCUS_NEXT);

        goto success;
     }
   if ((!strcmp(ev->keyname, "Return")) ||
            ((!strcmp(ev->keyname, "KP_Enter")) && !ev->string))
     {
        if (sd->focused)
          {
             item = eina_list_data_get(sd->focused);
             if (!elm_widget_item_disabled_get(item))
               edje_object_signal_emit(VIEW(item), "elm,anim,activate", "elm");
          }
        goto success;
     }
   else if ((!strcmp(ev->keyname, "Left")) ||
            ((!strcmp(ev->keyname, "KP_Left")) && (!ev->string)))
     {
        elm_widget_focus_direction_go(obj, 270.0);
        goto success;
     }
   else if ((!strcmp(ev->keyname, "Right")) ||
            ((!strcmp(ev->keyname, "KP_Right")) && (!ev->string)))
     {
        elm_widget_focus_direction_go(obj, 90.0);
        goto success;
     }
   else if ((!strcmp(ev->keyname, "Up")) ||
            ((!strcmp(ev->keyname, "KP_Up")) && (!ev->string)))
     {
        if (sd->focused)
          list = eina_list_prev(sd->focused);
        elm_widget_focus_direction_go(obj, 0.0);
        goto success;
     }
   else if ((!strcmp(ev->keyname, "Down")) ||
            ((!strcmp(ev->keyname, "KP_Down")) && (!ev->string)))
     {
        if (sd->focused)
          list = eina_list_next(sd->focused);
        elm_widget_focus_direction_go(obj, 180.0);
        goto success;
     }

   return EINA_FALSE;

success:
   if (list)
     {
        item = eina_list_data_get(list);
        edje_object_signal_emit
           (VIEW(item), "elm,action,focus_highlight,show", "elm");
        evas_object_smart_callback_call(obj, "item,focused", item);
        if (temp_item && (temp_item != item))
          {
             edje_object_signal_emit(VIEW(temp_item), "elm,action,focus_highlight,hide", "elm");
             evas_object_smart_callback_call(obj, "item,unfocused", temp_item);
          }
        sd->focused = list;
        _elm_popup_item_coordinates_calc(item, &x, &y, &w, &h);
        elm_scroller_region_bring_in(sd->scr, x, y, w, h);
     }
   ev->event_flags |= EVAS_EVENT_FLAG_ON_HOLD;
   return EINA_TRUE;
}

static void
_elm_popup_smart_set_user(Elm_Popup_Smart_Class *sc)
{
   ELM_WIDGET_CLASS(sc)->base.add = _elm_popup_smart_add;
   ELM_WIDGET_CLASS(sc)->base.del = _elm_popup_smart_del;

   ELM_WIDGET_CLASS(sc)->parent_set = _elm_popup_smart_parent_set;
   ELM_WIDGET_CLASS(sc)->event = _elm_popup_smart_event;
   ELM_WIDGET_CLASS(sc)->theme = _elm_popup_smart_theme;
   ELM_WIDGET_CLASS(sc)->translate = _elm_popup_smart_translate;
   ELM_WIDGET_CLASS(sc)->focus_next = _elm_popup_smart_focus_next;
   ELM_WIDGET_CLASS(sc)->on_focus = _elm_popup_smart_on_focus;
   ELM_WIDGET_CLASS(sc)->access = _elm_popup_smart_access;
   ELM_WIDGET_CLASS(sc)->access_object_get = _elm_popup_smart_access_object_get;
   ELM_WIDGET_CLASS(sc)->focus_direction_manager_is =
      _elm_popup_smart_focus_direction_manager_is;
   ELM_WIDGET_CLASS(sc)->focus_direction = _elm_popup_smart_focus_direction;
   ELM_WIDGET_CLASS(sc)->sub_object_del = _elm_popup_smart_sub_object_del;

   ELM_CONTAINER_CLASS(sc)->content_set = _elm_popup_smart_content_set;
   ELM_CONTAINER_CLASS(sc)->content_get = _elm_popup_smart_content_get;
   ELM_CONTAINER_CLASS(sc)->content_unset = _elm_popup_smart_content_unset;

   ELM_LAYOUT_CLASS(sc)->text_set = _elm_popup_smart_text_set;
   ELM_LAYOUT_CLASS(sc)->text_get = _elm_popup_smart_text_get;
   ELM_LAYOUT_CLASS(sc)->sizing_eval = _elm_popup_smart_sizing_eval;
}

EAPI const Elm_Popup_Smart_Class *
elm_popup_smart_class_get(void)
{
   static Elm_Popup_Smart_Class _sc =
     ELM_POPUP_SMART_CLASS_INIT_NAME_VERSION(ELM_POPUP_SMART_NAME);
   static const Elm_Popup_Smart_Class *class = NULL;
   Evas_Smart_Class *esc = (Evas_Smart_Class *)&_sc;

   if (class)
     return class;

   _elm_popup_smart_set(&_sc);
   esc->callbacks = _smart_callbacks;
   class = &_sc;

   return class;
}

EAPI Evas_Object *
elm_popup_add(Evas_Object *parent)
{
   Evas_Object *obj, *ao;
   const char *str;
   int orient = ELM_POPUP_ORIENT_CENTER;

   EINA_SAFETY_ON_NULL_RETURN_VAL(parent, NULL);

   obj = elm_widget_add(_elm_popup_smart_class_new(), parent);
   if (!obj) return NULL;

   if (!elm_widget_sub_object_add(parent, obj))
     ERR("could not add %p as sub object of %p", obj, parent);

   ELM_WIDGET_DATA_GET(obj, wd);
   ELM_POPUP_DATA_GET(obj, sd);

   sd->allow_eval = EINA_TRUE;

   // Notify
   sd->notify = elm_notify_add(obj);
   elm_object_style_set(sd->notify, "popup");
   elm_notify_align_set(sd->notify, 0.5, 0.5);
   elm_notify_allow_events_set(sd->notify, EINA_FALSE);
   evas_object_size_hint_weight_set
     (sd->notify, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set
     (sd->notify, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_widget_resize_object_set(obj, sd->notify, EINA_TRUE);
   elm_notify_parent_set(sd->notify, parent);

   evas_object_smart_callback_add(sd->notify, "block,clicked", _block_clicked_cb, obj);
   evas_object_smart_callback_add(sd->notify, "timeout", _timeout_cb, obj);
   evas_object_smart_callback_add(sd->notify, "show,finished", _show_finished_cb, obj);
   evas_object_smart_callback_add(sd->notify, "dismissed", _hide_effect_finished_cb, obj);
   evas_object_event_callback_add
     (sd->notify, EVAS_CALLBACK_RESIZE, _notify_resize_cb, obj);

   // Main layout
   sd->main_layout = elm_layout_add(sd->notify);
   if (!elm_layout_theme_set(sd->main_layout, "popup", "base",
                             elm_widget_style_get(obj)))
     CRITICAL("Failed to set layout!");

   _create_scroller(obj);
   elm_layout_content_set(sd->main_layout, "elm.swallow.content", sd->tbl);

   _scroller_size_calc(obj);

   evas_object_event_callback_add(obj, EVAS_CALLBACK_SHOW, _on_show, NULL);
   evas_object_event_callback_add
     (obj, EVAS_CALLBACK_RESTACK, _restack_cb, NULL);

   elm_layout_signal_callback_add
     (sd->main_layout, "elm,state,title_area,visible", "elm", _layout_change_cb, obj);
   elm_layout_signal_callback_add
     (sd->main_layout, "elm,state,title_area,hidden", "elm", _layout_change_cb, obj);
   elm_layout_signal_callback_add
     (sd->main_layout, "elm,state,action_area,visible", "elm", _layout_change_cb, obj);
   elm_layout_signal_callback_add
     (sd->main_layout, "elm,state,action_area,hidden", "elm", _layout_change_cb, obj);

   elm_layout_signal_callback_add
     (sd->main_layout, "elm,state,orientation,changed", "", _rotation_changed_cb, obj);

   wd->highlight_root = EINA_TRUE;
   sd->orientation = wd->orient_mode;
   sd->content_text_wrap_type = ELM_WRAP_MIXED;

   _elm_widget_orient_signal_emit(obj);

   elm_widget_can_focus_set(obj, EINA_TRUE);

   //Tizen Only: This should be removed when eo is applied.
   wd->on_create = EINA_FALSE;

   /* access */
   if (_elm_config->access_mode)
     {
        ao = _access_object_get(obj, ACCESS_OUTLINE_PART);
        if (!ao)
          {
             ao = _elm_access_edje_object_part_object_register
                (obj, elm_layout_edje_get(sd->main_layout), ACCESS_OUTLINE_PART);
             _elm_access_text_set(_elm_access_object_get(ao),
                                  ELM_ACCESS_TYPE,
                                  E_("IDS_SCR_OPT_CENTRAL_POP_UP_TTS"));
             _elm_access_text_set(_elm_access_object_get(ao),
                                  ELM_ACCESS_CONTEXT_INFO,
                                  E_("WDS_TTS_TBBODY_DOUBLE_TAP_TO_CLOSE_THE_POP_UP"));
             _elm_access_activate_callback_set
                (_elm_access_object_get(ao), _access_base_activate_cb, obj);
          }
     }

   _visuals_set(obj);

   str = edje_object_data_get(elm_layout_edje_get(sd->main_layout), "default_orient");
   if (str) orient = atoi(str);
   elm_popup_orient_set(obj, orient);
   elm_object_content_set(sd->notify, sd->main_layout);

   return obj;
}

EAPI void
elm_popup_content_text_wrap_type_set(Evas_Object *obj,
                                     Elm_Wrap_Type wrap)
{
   ELM_POPUP_CHECK(obj);
   ELM_POPUP_DATA_GET(obj, sd);

   //Need to wrap the content text, so not allowing ELM_WRAP_NONE
   if (sd->content_text_wrap_type == ELM_WRAP_NONE) return;

   sd->content_text_wrap_type = wrap;
   if (sd->text_content_obj)
     elm_label_line_wrap_set(sd->text_content_obj, wrap);
}

EAPI Elm_Wrap_Type
elm_popup_content_text_wrap_type_get(const Evas_Object *obj)
{
   ELM_POPUP_CHECK(obj) ELM_WRAP_LAST;
   ELM_POPUP_DATA_GET(obj, sd);

   return sd->content_text_wrap_type;
}

EAPI void
elm_popup_align_set(Evas_Object *obj,
                    double horizontal, double vertical)
{
   ELM_POPUP_CHECK(obj);
   ELM_POPUP_DATA_GET(obj, sd);

   elm_notify_align_set(sd->notify, horizontal, vertical);
}

EAPI void
elm_popup_align_get(const Evas_Object *obj,
                    double *horizontal, double *vertical)
{
   ELM_POPUP_CHECK(obj);
   ELM_POPUP_DATA_GET(obj, sd);

   elm_notify_align_get(sd->notify, horizontal, vertical);
}

EAPI void
elm_popup_orient_set(Evas_Object *obj,
                     Elm_Popup_Orient orient)
{
   ELM_POPUP_CHECK(obj);
   ELM_POPUP_DATA_GET(obj, sd);

   if (orient >= ELM_POPUP_ORIENT_LAST) return;
   double horizontal = 0, vertical = 0;

      switch (orient)
        {
         case ELM_POPUP_ORIENT_TOP:
            horizontal = 0.5; vertical = 0.0;
           break;

         case ELM_POPUP_ORIENT_CENTER:
            horizontal = 0.5; vertical = 0.5;
           break;

         case ELM_POPUP_ORIENT_BOTTOM:
            horizontal = 0.5; vertical = 1.0;
           break;

         case ELM_POPUP_ORIENT_LEFT:
            horizontal = 0.0; vertical = 0.5;
           break;

         case ELM_POPUP_ORIENT_RIGHT:
            horizontal = 1.0; vertical = 0.5;
           break;

         case ELM_POPUP_ORIENT_TOP_LEFT:
            horizontal = 0.0; vertical = 0.0;
           break;

         case ELM_POPUP_ORIENT_TOP_RIGHT:
            horizontal = 1.0; vertical = 0.0;
           break;

         case ELM_POPUP_ORIENT_BOTTOM_LEFT:
            horizontal = 0.0; vertical = 1.0;
           break;

         case ELM_POPUP_ORIENT_BOTTOM_RIGHT:
            horizontal = 1.0; vertical = 1.0;
           break;

         default:
           break;
        }
   elm_notify_align_set(sd->notify, horizontal, vertical);
}

static Elm_Notify_Orient
_elm_notify_orient_get(const Evas_Object *obj)
{
   Elm_Notify_Orient orient;
   double horizontal, vertical;

   elm_notify_align_get(obj, &horizontal, &vertical);

   if ((horizontal == 0.5) && (vertical == 0.0))
     orient = ELM_NOTIFY_ORIENT_TOP;
   else if ((horizontal == 0.5) && (vertical == 0.5))
     orient = ELM_NOTIFY_ORIENT_CENTER;
   else if ((horizontal == 0.5) && (vertical == 1.0))
     orient = ELM_NOTIFY_ORIENT_BOTTOM;
   else if ((horizontal == 0.0) && (vertical == 0.5))
     orient = ELM_NOTIFY_ORIENT_LEFT;
   else if ((horizontal == 1.0) && (vertical == 0.5))
     orient = ELM_NOTIFY_ORIENT_RIGHT;
   else if ((horizontal == 0.0) && (vertical == 0.0))
     orient = ELM_NOTIFY_ORIENT_TOP_LEFT;
   else if ((horizontal == 1.0) && (vertical == 0.0))
     orient = ELM_NOTIFY_ORIENT_TOP_RIGHT;
   else if ((horizontal == 0.0) && (vertical == 1.0))
     orient = ELM_NOTIFY_ORIENT_BOTTOM_LEFT;
   else if ((horizontal == 1.0) && (vertical == 1.0))
     orient = ELM_NOTIFY_ORIENT_BOTTOM_RIGHT;
   else
     orient = ELM_NOTIFY_ORIENT_TOP;
   return orient;
}

EAPI Elm_Popup_Orient
elm_popup_orient_get(const Evas_Object *obj)
{
   ELM_POPUP_CHECK(obj) - 1;
   ELM_POPUP_DATA_GET(obj, sd);

   return (Elm_Popup_Orient)_elm_notify_orient_get(sd->notify);
}

EAPI void
elm_popup_timeout_set(Evas_Object *obj,
                      double timeout)
{
   ELM_POPUP_CHECK(obj);
   ELM_POPUP_DATA_GET(obj, sd);

   elm_notify_timeout_set(sd->notify, timeout);
}

EAPI double
elm_popup_timeout_get(const Evas_Object *obj)
{
   ELM_POPUP_CHECK(obj) 0.0;
   ELM_POPUP_DATA_GET(obj, sd);

   return elm_notify_timeout_get(sd->notify);
}

EAPI void
elm_popup_allow_events_set(Evas_Object *obj,
                           Eina_Bool allow)
{
   Eina_Bool allow_events = !!allow;

   ELM_POPUP_CHECK(obj);
   ELM_POPUP_DATA_GET(obj, sd);

   elm_notify_allow_events_set(sd->notify, allow_events);
}

EAPI Eina_Bool
elm_popup_allow_events_get(const Evas_Object *obj)
{
   ELM_POPUP_CHECK(obj) EINA_FALSE;
   ELM_POPUP_DATA_GET(obj, sd);

   return elm_notify_allow_events_get(sd->notify);
}

EAPI Elm_Object_Item *
elm_popup_item_append(Evas_Object *obj,
                      const char *label,
                      Evas_Object *icon,
                      Evas_Smart_Cb func,
                      const void *data)
{
   Evas_Object *prev_content;
   Elm_Popup_Item *item;

   ELM_POPUP_CHECK(obj) NULL;
   ELM_POPUP_DATA_GET(obj, sd);

   item = elm_widget_item_new(obj, Elm_Popup_Item);
   if (!item) return NULL;
   if (sd->content || sd->text_content_obj)
     {
        prev_content = elm_layout_content_get
            (sd->content_area, "elm.swallow.content");
        if (prev_content)
          evas_object_del(prev_content);
     }

   //The first item is appended.
   if (!sd->items)
     _list_add(obj);

   item->func = func;
   item->base.data = data;

   _item_new(item);
   _item_icon_set(item, icon);
   _item_text_set(item, label);

   elm_box_pack_end(sd->box, VIEW(item));
   sd->items = eina_list_append(sd->items, item);

   elm_layout_sizing_eval(obj);

   return (Elm_Object_Item *)item;
}

EAPI void
elm_popup_parent_set(Evas_Object *obj,
                           Evas_Object *parent)
  {
     ELM_POPUP_CHECK(obj);
     ELM_POPUP_DATA_GET(obj, sd);

     elm_notify_parent_set(sd->notify, parent);
  }

EAPI Evas_Object *
elm_popup_parent_get(Evas_Object *obj)
  {
     ELM_POPUP_CHECK(obj) NULL;
     ELM_POPUP_DATA_GET(obj, sd);

     return elm_notify_parent_get(sd->notify);
  }

EAPI void
elm_popup_dismiss(Evas_Object *obj)
{
   ELM_POPUP_CHECK(obj);
   ELM_POPUP_DATA_GET(obj, sd);

#ifdef ELM_FEATURE_WEARABLE_C1
   Evas_Object *highlight_obj;

   highlight_obj = evas_object_name_find(evas_object_evas_get(obj),
                                         "_elm_access_disp");

   if (highlight_obj) evas_object_hide(highlight_obj);
#endif

   elm_layout_signal_emit(sd->main_layout, "elm,state,hide", "elm");
   elm_notify_dismiss(sd->notify);
}

#undef _TIZEN_
