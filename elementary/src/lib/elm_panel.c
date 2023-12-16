#include <Elementary.h>
#include "elm_priv.h"
#include "elm_widget_panel.h"

EAPI const char ELM_PANEL_SMART_NAME[] = "elm_panel";

static const char ACCESS_OUTLINE_PART[] = "access.outline";

static const char SIG_SCROLL[] = "scroll";

static const Evas_Smart_Cb_Description _smart_callbacks[] =
{
   {SIG_SCROLL, ""}
};

static const Evas_Smart_Interface *_smart_interfaces[] =
{
   (Evas_Smart_Interface *)&ELM_SCROLLABLE_IFACE, NULL
};

EVAS_SMART_SUBCLASS_IFACE_NEW
  (ELM_PANEL_SMART_NAME, _elm_panel, Elm_Panel_Smart_Class,
   Elm_Layout_Smart_Class, elm_layout_smart_class_get, _smart_callbacks,
   _smart_interfaces);

static void
_mirrored_set(Evas_Object *obj,
              Eina_Bool rtl)
{
   ELM_PANEL_DATA_GET(obj, sd);

   elm_widget_mirrored_set(sd->bx, rtl);
   elm_panel_orient_set(obj, elm_panel_orient_get(obj));
}

static void
_elm_panel_smart_sizing_eval(Evas_Object *obj)
{
   Evas_Coord mw = -1, mh = -1;

   ELM_PANEL_DATA_GET(obj, sd);

   if (sd->on_deletion) return;

   evas_object_smart_calculate(sd->bx);
   edje_object_size_min_calc(ELM_WIDGET_DATA(sd)->resize_obj, &mw, &mh);
   evas_object_size_hint_min_set(obj, mw, mh);
   evas_object_size_hint_max_set(obj, -1, -1);
}

static char *
_access_state_cb(void *data, Evas_Object *obj __UNUSED__)
{
   ELM_PANEL_DATA_GET(data, sd);

   if (!sd->hidden) return strdup(E_("state: opened"));//[string:error] not included in po file.
   else return strdup(E_("state: closed"));//[string:error] not included in po file.

   return NULL;
}

static Evas_Object *
_access_object_get(const Evas_Object *obj, const char *part)
{
   Evas_Object *po, *ao;
   ELM_PANEL_DATA_GET(obj, sd);

   po = (Evas_Object *)edje_object_part_object_get
      (elm_layout_edje_get(sd->scr_ly), part);
   ao = evas_object_data_get(po, "_part_access_obj");

   return ao;
}

static void
_access_activate_cb(void *data,
                    Evas_Object *part_obj __UNUSED__,
                    Elm_Object_Item *item __UNUSED__)
{
   elm_panel_hidden_set(data, EINA_TRUE);
}

#ifdef ELM_FEATURE_WEARABLE_C1
static char *
_cue_access_info_cb(void *data, Evas_Object *obj)
{
   char text[1024];

   snprintf(text, sizeof(text), "%s, %s", E_("WDS_TTS_TBOPT_MORE_OPTIONS"), E_("IDS_ACCS_BODY_BUTTON_TTS"));

   return strdup(text);
}

static void
_cue_access_activate_cb(void * data, Evas_Object *part_obj, Elm_Object_Item *item)
{
   ELM_PANEL_DATA_GET(data, sd);

   if (sd->hidden) elm_panel_toggle(data);
}
#endif

static void
_access_obj_process(Evas_Object *obj, Eina_Bool is_access)
{
   Evas_Object *ao = NULL;
   ELM_PANEL_DATA_GET(obj, sd);

   /* drawer access */
   if (sd->scrollable)
     {
        if (is_access)
          {
             ao = _access_object_get(obj, ACCESS_OUTLINE_PART);
             if (!ao)
               {
                  ao = _elm_access_edje_object_part_object_register(obj, elm_layout_edje_get(sd->scr_ly), ACCESS_OUTLINE_PART);
                  _elm_access_text_set(_elm_access_object_get(ao), ELM_ACCESS_TYPE, E_("IDS_TPLATFORM_BODY_A_DRAWER_IS_OPEN_T_TTS"));
                  _elm_access_text_set(_elm_access_object_get(ao), ELM_ACCESS_CONTEXT_INFO, E_("IDS_ACCS_BODY_DOUBLE_TAP_TO_CLOSE_DRAWER_MENU_TTS"));
                  _elm_access_activate_callback_set(_elm_access_object_get(ao), _access_activate_cb, obj);
               }
          }
        else
          {
             _elm_access_edje_object_part_object_unregister(obj, elm_layout_edje_get(sd->scr_ly), ACCESS_OUTLINE_PART);
          }
     }
   else
     {
        if (is_access)
          {
#ifdef ELM_FEATURE_WEARABLE_C1
             Evas_Object *cue_access = NULL;
             cue_access = _elm_access_edje_object_part_object_register(obj, ELM_WIDGET_DATA(sd)->resize_obj, "cue.event");
             if (cue_access)
               {
                  elm_access_info_cb_set(cue_access, ELM_ACCESS_INFO, _cue_access_info_cb, obj);
                  elm_access_activate_cb_set(cue_access, _cue_access_activate_cb, obj);
               }
#endif
             _elm_access_edje_object_part_object_register(obj, ELM_WIDGET_DATA(sd)->resize_obj, "btn_icon");
             _elm_access_text_set(_elm_access_object_get(ao),ELM_ACCESS_TYPE, E_("panel button"));//[string:error] not included in po file.
             _elm_access_callback_set(_elm_access_object_get(ao), ELM_ACCESS_STATE, _access_state_cb, obj);
          }
        else
          {
#ifdef ELM_FEATURE_WEARABLE_C1
             _elm_access_edje_object_part_object_unregister(obj, ELM_WIDGET_DATA(sd)->resize_obj, "cue.event");
#endif
             _elm_access_edje_object_part_object_unregister(obj, ELM_WIDGET_DATA(sd)->resize_obj, "btn_icon");
          }
     }
}

static void
_orient_set_do(Evas_Object *obj)
{
   ELM_PANEL_DATA_GET(obj, sd);

   switch (sd->orient)
     {
      case ELM_PANEL_ORIENT_TOP:
        elm_layout_theme_set(obj, "panel", "top", elm_widget_style_get(obj));
        break;

      case ELM_PANEL_ORIENT_BOTTOM:
        elm_layout_theme_set
          (obj, "panel", "bottom", elm_widget_style_get(obj));
        break;

      case ELM_PANEL_ORIENT_LEFT:
        if (!elm_widget_mirrored_get(obj))
          elm_layout_theme_set
            (obj, "panel", "left", elm_widget_style_get(obj));
        else
          elm_layout_theme_set
            (obj, "panel", "right", elm_widget_style_get(obj));
        break;

      case ELM_PANEL_ORIENT_RIGHT:
        if (!elm_widget_mirrored_get(obj))
          elm_layout_theme_set
            (obj, "panel", "right", elm_widget_style_get(obj));
        else
          elm_layout_theme_set
            (obj, "panel", "left", elm_widget_style_get(obj));
        break;
     }

   /* access */
   if (_elm_config->access_mode == ELM_ACCESS_MODE_ON)
     _access_obj_process(obj, EINA_TRUE);
}

static void
_scrollable_layout_theme_set(Evas_Object *obj, Elm_Panel_Smart_Data *sd)
{
   switch (sd->orient)
     {
      case ELM_PANEL_ORIENT_TOP:
         if (!elm_layout_theme_set(sd->scr_ly, "scroller", "panel/top",
                                   elm_widget_style_get(obj)))
           CRITICAL("Failed to set layout!");
         if (sd->freeze)
            sd->s_iface->movement_block_set(obj, ELM_SCROLLER_MOVEMENT_BLOCK_VERTICAL);
         else
            sd->s_iface->movement_block_set(obj, ELM_SCROLLER_MOVEMENT_NO_BLOCK);
         break;
      case ELM_PANEL_ORIENT_BOTTOM:
         if (!elm_layout_theme_set(sd->scr_ly, "scroller", "panel/bottom",
                                   elm_widget_style_get(obj)))
           CRITICAL("Failed to set layout!");
         if (sd->freeze)
            sd->s_iface->movement_block_set(obj, ELM_SCROLLER_MOVEMENT_BLOCK_VERTICAL);
         else
            sd->s_iface->movement_block_set(obj, ELM_SCROLLER_MOVEMENT_NO_BLOCK);
         break;
      case ELM_PANEL_ORIENT_LEFT:
         if (!elm_layout_theme_set(sd->scr_ly, "scroller", "panel/left",
                                   elm_widget_style_get(obj)))
           CRITICAL("Failed to set layout!");
         if (sd->freeze)
            sd->s_iface->movement_block_set(obj, ELM_SCROLLER_MOVEMENT_BLOCK_HORIZONTAL);
         else
            sd->s_iface->movement_block_set(obj, ELM_SCROLLER_MOVEMENT_NO_BLOCK);
         break;
      case ELM_PANEL_ORIENT_RIGHT:
         if (!elm_layout_theme_set(sd->scr_ly, "scroller", "panel/right",
                                   elm_widget_style_get(obj)))
           CRITICAL("Failed to set layout!");
         if (sd->freeze)
            sd->s_iface->movement_block_set(obj, ELM_SCROLLER_MOVEMENT_BLOCK_HORIZONTAL);
         else
            sd->s_iface->movement_block_set(obj, ELM_SCROLLER_MOVEMENT_NO_BLOCK);
         break;
     }

   /* access */
   if (_elm_config->access_mode == ELM_ACCESS_MODE_ON)
     _access_obj_process(obj, EINA_TRUE);
}

static Eina_Bool
_elm_panel_smart_theme(Evas_Object *obj)
{
   const char *str;

   ELM_PANEL_DATA_GET(obj, sd);

   if (!ELM_WIDGET_CLASS(_elm_panel_parent_sc)->theme(obj))
     return EINA_FALSE;

   _mirrored_set(obj, elm_widget_mirrored_get(obj));

   if (sd->scrollable)
     {
        const char *handler_size;
        elm_widget_theme_object_set(obj, sd->scr_edje, "scroller", "panel",
                                    elm_widget_style_get(obj));
        _scrollable_layout_theme_set(obj, sd);

        handler_size = edje_object_data_get(sd->scr_edje, "handler_size");
        if (handler_size)
          sd->handler_size = (Evas_Coord) (elm_object_scale_get(obj)) * (atoi(handler_size));
     }
   else
     {
        str = edje_object_data_get
           (ELM_WIDGET_DATA(sd)->resize_obj, "focus_highlight");
        if ((str) && (!strcmp(str, "on")))
          elm_widget_highlight_in_theme_set(obj, EINA_TRUE);
        else
          elm_widget_highlight_in_theme_set(obj, EINA_FALSE);

        _orient_set_do(obj);
     }

   elm_layout_sizing_eval(obj);

   return EINA_TRUE;
}

static Eina_Bool
_elm_panel_smart_focus_next(const Evas_Object *obj,
                            Elm_Focus_Direction dir,
                            Evas_Object **next)
{
   Evas_Object *cur;
   Eina_List *items = NULL;
   Eina_Bool ret = EINA_FALSE;

   ELM_PANEL_DATA_GET(obj, sd);

   if (!sd->content)
     return EINA_FALSE;

   if (sd->scrollable)
     {
        if (sd->hidden) return EINA_FALSE;

        if (_elm_config->access_mode == ELM_ACCESS_MODE_ON)
          {
             Evas_Object *ao = _access_object_get(obj, ACCESS_OUTLINE_PART);
             if (ao) items = eina_list_append(items, ao);
             items = eina_list_append(items, sd->content);

             ret = elm_widget_focus_list_next_get
                (obj, items, eina_list_data_get, dir, next);
             eina_list_free(items);

             return ret;
          }

        return elm_widget_focus_next_get(sd->content, dir, next);
     }

   cur = sd->content;

   /* Try to Focus cycle in subitem */
   if (!sd->hidden)
     return elm_widget_focus_next_get(cur, dir, next);

   /* access */
   if (_elm_config->access_mode != ELM_ACCESS_MODE_OFF)
     {
        Evas_Object *ao, *po;
        po = (Evas_Object *)edje_object_part_object_get
               (ELM_WIDGET_DATA(sd)->resize_obj, "btn_icon");
        ao = evas_object_data_get(po, "_part_access_obj");
        _elm_access_highlight_set(ao, EINA_FALSE);
     }

   /* Return */
   *next = (Evas_Object *)obj;
   return !elm_widget_focus_get(obj);
}

static void
_box_layout_cb(Evas_Object *o,
               Evas_Object_Box_Data *priv,
               void *data __UNUSED__)
{
   _els_box_layout(o, priv, EINA_TRUE, EINA_FALSE, EINA_FALSE);
}

static void
_handler_open(Evas_Object *obj, Evas_Coord w, Evas_Coord h)
{
   ELM_PANEL_DATA_GET(obj, sd);

   if (sd->handler_size == 0) return;

   switch (sd->orient)
     {
      case ELM_PANEL_ORIENT_TOP:
         sd->s_iface->content_region_show
            (obj, 0, (h * sd->content_size_ratio) - sd->handler_size, w, h);
         break;
      case ELM_PANEL_ORIENT_BOTTOM:
         sd->s_iface->content_region_show
            (obj, 0, sd->handler_size, w, h);
         break;
      case ELM_PANEL_ORIENT_LEFT:
         sd->s_iface->content_region_show
            (obj, (w * sd->content_size_ratio) - sd->handler_size, 0, w, h);
         break;
      case ELM_PANEL_ORIENT_RIGHT:
         sd->s_iface->region_bring_in
            (obj, sd->handler_size, 0, w, h);
         sd->s_iface->freeze_set(obj, EINA_TRUE);
         sd->s_iface->movement_block_set(obj, ELM_SCROLLER_MOVEMENT_BLOCK_HORIZONTAL);
         sd->freeze = TRUE;
         break;
     }
}

static void
_drawer_open(Evas_Object *obj, Evas_Coord w, Evas_Coord h, Eina_Bool anim)
{
   ELM_PANEL_DATA_GET(obj, sd);
   Evas_Coord x = 0, y = 0;

   if (sd->freeze)
     {
        sd->s_iface->movement_block_set(obj, ELM_SCROLLER_MOVEMENT_NO_BLOCK);
        sd->freeze = EINA_FALSE;
        elm_layout_signal_emit(sd->scr_ly, "elm,state,content,visible", "elm");
        elm_object_signal_emit(obj, "elm,state,unhold", "elm");
     }

   switch (sd->orient)
     {
      case ELM_PANEL_ORIENT_TOP:
      case ELM_PANEL_ORIENT_LEFT:
         break;

      case ELM_PANEL_ORIENT_BOTTOM:
         y = h * sd->content_size_ratio;
         break;

      case ELM_PANEL_ORIENT_RIGHT:
         x = w * sd->content_size_ratio;
         break;
     }

   if (anim) sd->s_iface->region_bring_in(obj, x, y, w, h);
   else sd->s_iface->content_region_show(obj, x, y, w, h);
}

static void
_drawer_close(Evas_Object *obj, Evas_Coord w, Evas_Coord h, Eina_Bool anim)
{
   ELM_PANEL_DATA_GET(obj, sd);
   Evas_Coord x = 0, y = 0;
   Eina_Bool horizontal = EINA_FALSE;

   switch (sd->orient)
     {
      case ELM_PANEL_ORIENT_TOP:
         y = h * sd->content_size_ratio;
         break;

      case ELM_PANEL_ORIENT_LEFT:
         x = w * sd->content_size_ratio;
         horizontal = EINA_TRUE;
         break;

      case ELM_PANEL_ORIENT_BOTTOM:
         break;
      case ELM_PANEL_ORIENT_RIGHT:
         horizontal = EINA_TRUE;
         break;
     }

   //do not need to drawer_close.
   //cause panel size is zero.
   if (w == 0 && h == 0) return;

   if (anim)
     {
        if (sd->freeze)
          {
             sd->s_iface->movement_block_set(obj, ELM_SCROLLER_MOVEMENT_NO_BLOCK);
             sd->freeze = EINA_FALSE;
             elm_layout_signal_emit(sd->scr_ly, "elm,state,content,visible", "elm");
             elm_object_signal_emit(obj, "elm,state,unhold", "elm");
          }
        sd->s_iface->region_bring_in(obj, x, y, w, h);
        sd->panel_close = EINA_TRUE;
     }
   else
     {
        sd->s_iface->content_region_show(obj, x, y, w, h);
        if (!sd->freeze)
          {
             if (horizontal)
               sd->s_iface->movement_block_set(obj, ELM_SCROLLER_MOVEMENT_BLOCK_HORIZONTAL);
             else
               sd->s_iface->movement_block_set(obj, ELM_SCROLLER_MOVEMENT_BLOCK_VERTICAL);
             sd->freeze = EINA_TRUE;
             elm_layout_signal_emit(sd->scr_ly, "elm,state,content,hidden", "elm");
             elm_object_signal_emit(obj, "elm,state,hold", "elm");
          }
     }
}

static void
_panel_toggle(void *data __UNUSED__,
              Evas_Object *obj,
              const char *emission __UNUSED__,
              const char *source __UNUSED__)
{
   ELM_PANEL_DATA_GET(obj, sd);

   Evas_Coord w, h;

   if (sd->scrollable)
     {
        if (elm_widget_disabled_get(obj)) return;

        evas_object_geometry_get(obj, NULL, NULL, &w, &h);
        if (sd->hidden)
          {
             sd->hidden = EINA_FALSE;
             _drawer_open(obj, w, h, EINA_TRUE);
             elm_object_signal_emit(obj, "elm,state,active", "elm");
             evas_object_repeat_events_set(obj, EINA_FALSE);
          }
        else
          {
             sd->hidden = EINA_TRUE;
             _drawer_close(obj, w, h, EINA_TRUE);
             elm_object_signal_emit(obj, "elm,state,inactive", "elm");
             evas_object_repeat_events_set(obj, EINA_TRUE);
          }
     }
   else
     {
        if (sd->hidden)
          {
             elm_layout_signal_emit(obj, "elm,action,show", "elm");
             sd->hidden = EINA_FALSE;
          }
        else
          {
             elm_layout_signal_emit(obj, "elm,action,hide", "elm");
             sd->hidden = EINA_TRUE;
             if (elm_widget_focus_get(sd->content))
               {
                  elm_widget_focused_object_clear(obj);
                  elm_widget_focus_steal(obj);
               }
          }

        edje_object_message_signal_process(ELM_WIDGET_DATA(sd)->resize_obj);
     }
}

static Eina_Bool
_state_sync(Evas_Object *obj)
{
   ELM_PANEL_DATA_GET(obj, sd);
   Evas_Object *ao;
   Evas_Coord pos, panel_size, w, h;
   Eina_Bool open = EINA_FALSE, horizontal = EINA_FALSE;
   evas_object_geometry_get(obj, NULL, NULL, &w, &h);

   switch (sd->orient)
     {
      case ELM_PANEL_ORIENT_TOP:
         panel_size = h * sd->content_size_ratio;
         sd->s_iface->content_pos_get(obj, NULL, &pos);

         if (pos == 0) open = EINA_TRUE;
         else if (pos == panel_size) open = EINA_FALSE;
         else return EINA_FALSE;
         break;

      case ELM_PANEL_ORIENT_BOTTOM:
         panel_size = h * sd->content_size_ratio;
         sd->s_iface->content_pos_get(obj, NULL, &pos);

         if (pos == panel_size) open = EINA_TRUE;
         else if (pos == 0) open = EINA_FALSE;
         else return EINA_FALSE;
         break;

      case ELM_PANEL_ORIENT_LEFT:
         panel_size = w * sd->content_size_ratio;
         sd->s_iface->content_pos_get(obj, &pos, NULL);
         horizontal = EINA_TRUE;

         if (pos == 0) open = EINA_TRUE;
         else if (pos == panel_size) open = EINA_FALSE;
         else return EINA_FALSE;
         break;

      case ELM_PANEL_ORIENT_RIGHT:
         panel_size = w * sd->content_size_ratio;
         Evas_Coord pos_y;
         sd->s_iface->content_pos_get(obj, &pos, &pos_y);
         horizontal = EINA_TRUE;

         if (pos == panel_size) open = EINA_TRUE;
         else if (pos == 0) open = EINA_FALSE;
         else return EINA_FALSE;
         break;
     }

   if (open)
     {
        if (sd->hidden)
          {
             sd->hidden = EINA_FALSE;
             elm_object_signal_emit(obj, "elm,state,active", "elm");
             evas_object_repeat_events_set(obj, EINA_FALSE);
          }
        sd->s_iface->single_direction_set(obj, ELM_SCROLLER_SINGLE_DIRECTION_HARD);

        //focus & access
        elm_object_tree_focus_allow_set(obj, EINA_TRUE);
        if (_elm_config->access_mode == ELM_ACCESS_MODE_ON)
          {
             ao = _access_object_get(obj, ACCESS_OUTLINE_PART);
             evas_object_show(ao);
             _elm_access_highlight_set(ao, EINA_FALSE);
          }
        else
          elm_object_focus_set(obj, EINA_TRUE);
     }
   else
     {
        if (!sd->hidden)
          {
             sd->hidden = EINA_TRUE;
             elm_object_signal_emit(obj, "elm,state,inactive", "elm");
             evas_object_repeat_events_set(obj, EINA_TRUE);
          }
        if (horizontal)
          sd->s_iface->movement_block_set(obj, ELM_SCROLLER_MOVEMENT_BLOCK_HORIZONTAL);
        else
          sd->s_iface->movement_block_set(obj, ELM_SCROLLER_MOVEMENT_BLOCK_VERTICAL);
        sd->freeze = EINA_TRUE;
        elm_layout_signal_emit(sd->scr_ly, "elm,state,content,hidden", "elm");
        elm_object_signal_emit(obj, "elm,state,hold", "elm");

        sd->s_iface->single_direction_set(obj, ELM_SCROLLER_SINGLE_DIRECTION_NONE);

        //focus & access
        elm_object_tree_focus_allow_set(obj, EINA_FALSE);
        if (_elm_config->access_mode == ELM_ACCESS_MODE_ON)
          {
             ao = _access_object_get(obj, ACCESS_OUTLINE_PART);
             evas_object_hide(ao);
          }
     }

   return EINA_TRUE;
}

static Eina_Bool
_timer_cb(void *data)
{
   ELM_PANEL_DATA_GET(data, sd);
   Evas_Object *obj = data;
   Evas_Coord w, h;

   sd->timer = NULL;

   if (sd->freeze)
     {
        sd->s_iface->movement_block_set(obj, ELM_SCROLLER_MOVEMENT_NO_BLOCK);
        sd->freeze = EINA_FALSE;
        elm_layout_signal_emit(sd->scr_ly, "elm,state,content,visible", "elm");
        elm_object_signal_emit(obj, "elm,state,unhold", "elm");
        evas_object_geometry_get(obj, NULL, NULL, &w, &h);
        _handler_open(obj, w, h);
     }

   return ECORE_CALLBACK_CANCEL;
}

static void
_event_mouse_up(void *data,
                Evas *e EINA_UNUSED,
                Evas_Object *obj EINA_UNUSED,
                void *event_info)
{
   ELM_PANEL_DATA_GET(data, sd);
   Evas_Event_Mouse_Up *ev = event_info;
   Evas_Coord x, y, up_x, up_y, minw = 0, minh = 0;
   evas_object_geometry_get(data, &x, &y, NULL, NULL);

   up_x = ev->output.x - x;
   up_y = ev->output.y - y;

   elm_coords_finger_size_adjust(1, &minw, 1, &minh);

   if ((!sd->hidden) && (up_x == sd->down_x) && (up_y == sd->down_y))
     elm_panel_hidden_set(data, EINA_TRUE);
}

static void
_on_mouse_down(void *data,
               Evas *e EINA_UNUSED,
               Evas_Object *obj,
               void *event_info)
{
   Elm_Panel_Smart_Data *sd = data;
   Evas_Event_Mouse_Down *ev = event_info;
   Evas_Coord finger_size = elm_config_finger_size_get();
   Evas_Coord x, y, w, h;
   evas_object_geometry_get(obj, &x, &y, &w, &h);
   const char *timer_duration;
   double timer_duration_num;
   const char *event_area;
   double event_area_size;

   sd->down_x = ev->output.x - x;
   sd->down_y = ev->output.y - y;
   sd->prev_x = sd->down_x;
   sd->prev_y = sd->down_y;

   sd->down = EINA_TRUE;

   // if freeze state & mouse down on the edge
   // then set timer for un-freeze
   timer_duration = edje_object_data_get(sd->scr_edje, "timer_duration");
   if (timer_duration) timer_duration_num = atof(timer_duration);
   else timer_duration_num = 0.2;

   event_area = edje_object_data_get(sd->scr_edje, "event_area");
   if (event_area) event_area_size = atof(event_area);
   else event_area_size = finger_size;

   switch (sd->orient)
     {
      case ELM_PANEL_ORIENT_TOP:
         if ((sd->freeze) && (sd->down_y >= 0) && (sd->down_y < event_area_size))
           {
              if (sd->timer) ecore_timer_del(sd->timer);
              sd->timer = ecore_timer_add(timer_duration_num, _timer_cb, obj);
           }
         break;
      case ELM_PANEL_ORIENT_BOTTOM:
         if ((sd->freeze) && (sd->down_y <= h) && (sd->down_y > (h - event_area_size)))
           {
              if (sd->timer) ecore_timer_del(sd->timer);
              sd->timer = ecore_timer_add(timer_duration_num, _timer_cb, obj);
           }
         break;
      case ELM_PANEL_ORIENT_LEFT:
         if ((sd->freeze) && (sd->down_x >= 0) && (sd->down_x < event_area_size))
           {
              if (sd->timer) ecore_timer_del(sd->timer);
              sd->timer = ecore_timer_add(timer_duration_num, _timer_cb, obj);
           }
         break;
      case ELM_PANEL_ORIENT_RIGHT:
         if ((sd->freeze) && (sd->down_x <= w) && (sd->down_x > (w - event_area_size)))
           {
              if (sd->timer) ecore_timer_del(sd->timer);
              sd->timer = ecore_timer_add(timer_duration_num, _timer_cb, obj);
              sd->handler_bringin = EINA_TRUE;
           }
         break;
     }
}

static void
_on_mouse_move(void *data,
               Evas *e EINA_UNUSED,
               Evas_Object *obj,
               void *event_info)
{
   Elm_Panel_Smart_Data *sd = data;
   Evas_Event_Mouse_Move *ev = event_info;
   Evas_Coord x, y, w, h, cur_x, cur_y, finger_size;
   evas_object_geometry_get(obj, &x, &y, &w, &h);
   finger_size = elm_config_finger_size_get();
   const char *event_area;
   double event_area_size;

   event_area = edje_object_data_get(sd->scr_edje, "event_area");
   if (event_area) event_area_size = atof(event_area);
   else event_area_size = finger_size;

   if (!sd->down) return;

   cur_x = ev->cur.canvas.x - x;
   cur_y = ev->cur.canvas.y - y;

   sd->s_iface->content_pos_get(obj, &x, &y);

   if (sd->handler_bringin)
     {
        switch (sd->orient)
          {
             case ELM_PANEL_ORIENT_TOP:
                break;
             case ELM_PANEL_ORIENT_BOTTOM:
                break;
             case ELM_PANEL_ORIENT_LEFT:
                break;
             case ELM_PANEL_ORIENT_RIGHT:
                if (x < sd->handler_size && abs(sd->prev_x - cur_x) < finger_size) return;
                else
                  {
                     sd->handler_bringin = EINA_FALSE;
                     sd->s_iface->movement_block_set(obj, ELM_SCROLLER_MOVEMENT_NO_BLOCK);
                     sd->s_iface->freeze_set(obj, EINA_FALSE);
                     if (sd->freeze == EINA_TRUE)
                       {
                          sd->freeze = EINA_FALSE;
                          elm_layout_signal_emit(sd->scr_ly, "elm,state,content,visible", "elm");
                          elm_object_signal_emit(obj, "elm,state,unhold", "elm");
                       }
                  }
                break;
          }
     }

   // if mouse down on the edge (it means sd->timer is not null)
   //    and move more than finger size
   // then un-freeze
   switch (sd->orient)
     {
      case ELM_PANEL_ORIENT_TOP:
         if (sd->timer && ((cur_y - sd->down_y) > event_area_size))
           {
              sd->s_iface->movement_block_set(obj, ELM_SCROLLER_MOVEMENT_NO_BLOCK);
              sd->freeze = EINA_FALSE;
              elm_layout_signal_emit(sd->scr_ly, "elm,state,content,visible", "elm");
              elm_object_signal_emit(obj, "elm,state,unhold", "elm");
           }
         break;
      case ELM_PANEL_ORIENT_BOTTOM:
         if (sd->timer && ((sd->down_y - cur_y) > event_area_size))
           {
              sd->s_iface->movement_block_set(obj, ELM_SCROLLER_MOVEMENT_NO_BLOCK);
              sd->freeze = EINA_FALSE;
              elm_layout_signal_emit(sd->scr_ly, "elm,state,content,visible", "elm");
              elm_object_signal_emit(obj, "elm,state,unhold", "elm");
           }
         break;
      case ELM_PANEL_ORIENT_LEFT:
         if (sd->timer && ((cur_x - sd->down_x) > event_area_size))
           {
              sd->s_iface->movement_block_set(obj, ELM_SCROLLER_MOVEMENT_NO_BLOCK);
              sd->freeze = EINA_FALSE;
              elm_layout_signal_emit(sd->scr_ly, "elm,state,content,visible", "elm");
              elm_object_signal_emit(obj, "elm,state,unhold", "elm");
           }
         break;
      case ELM_PANEL_ORIENT_RIGHT:
         if (sd->timer && ((sd->down_x - cur_x) > event_area_size))
           {
              sd->s_iface->movement_block_set(obj, ELM_SCROLLER_MOVEMENT_NO_BLOCK);
              sd->freeze = EINA_FALSE;
              elm_layout_signal_emit(sd->scr_ly, "elm,state,content,visible", "elm");
              elm_object_signal_emit(obj, "elm,state,unhold", "elm");
           }
         break;
     }

   if (!sd->freeze && sd->hidden)
     ev->event_flags |= EVAS_EVENT_FLAG_ON_HOLD;

   sd->prev_x = cur_x; sd->prev_y = cur_y;
}

static void
_on_mouse_up(void *data,
             Evas *e EINA_UNUSED,
             Evas_Object *obj,
             void *event_info)
{
   Elm_Panel_Smart_Data *sd = data;
   Evas_Event_Mouse_Up *ev = event_info;
   Evas_Coord panel_size, open_threshold, close_threshold, pos, w, h;
   const char *open_threshold_data, *close_threshold_data;

   evas_object_geometry_get(obj, NULL, NULL, &w, &h);

   ELM_SAFE_FREE(sd->timer, ecore_timer_del);

   sd->down = EINA_FALSE;

   if (sd->hidden) sd->s_iface->freeze_set(obj, EINA_FALSE);

   if (sd->animator) return;

   if (_state_sync(obj)) return;

   if (sd->orient == ELM_PANEL_ORIENT_TOP || sd->orient == ELM_PANEL_ORIENT_BOTTOM)
     panel_size = h * sd->content_size_ratio;
   else panel_size = w * sd->content_size_ratio;

   open_threshold_data = edje_object_data_get(sd->scr_edje, "open_threshold");
   if (open_threshold_data) open_threshold = atoi(open_threshold_data);
   else open_threshold = panel_size / 4;

   close_threshold_data = edje_object_data_get(sd->scr_edje, "close_threshold");
   if (close_threshold_data)
     {
        close_threshold = atoi(close_threshold_data);
        if (sd->orient == ELM_PANEL_ORIENT_BOTTOM || sd->orient == ELM_PANEL_ORIENT_RIGHT)
          close_threshold = panel_size - close_threshold;
     }
   else
     {
        close_threshold = panel_size - open_threshold;
        if (sd->orient == ELM_PANEL_ORIENT_TOP || sd->orient == ELM_PANEL_ORIENT_LEFT)
          close_threshold = open_threshold;
     }

   if (sd->orient == ELM_PANEL_ORIENT_TOP || sd->orient == ELM_PANEL_ORIENT_LEFT)
     open_threshold = panel_size - open_threshold;

   switch (sd->orient)
     {
      case ELM_PANEL_ORIENT_TOP:
         sd->s_iface->content_pos_get(obj, NULL, &pos);

         if (sd->hidden)
           {
              if (pos < open_threshold) _drawer_open(obj, w, h, EINA_TRUE);
              else _drawer_close(obj, w, h, EINA_TRUE);
           }
         else
           {
              if (pos < close_threshold) _drawer_open(obj, w, h, EINA_TRUE);
              else _drawer_close(obj, w, h, EINA_TRUE);
           }
         break;

      case ELM_PANEL_ORIENT_BOTTOM:
         sd->s_iface->content_pos_get(obj, NULL, &pos);

         if (sd->hidden)
           {
              if (pos > open_threshold) _drawer_open(obj, w, h, EINA_TRUE);
              else _drawer_close(obj, w, h, EINA_TRUE);
           }
         else
           {
              if (pos > close_threshold) _drawer_open(obj, w, h, EINA_TRUE);
              else _drawer_close(obj, w, h, EINA_TRUE);
           }
         break;

      case ELM_PANEL_ORIENT_LEFT:
         sd->s_iface->content_pos_get(obj, &pos, NULL);

         if (sd->hidden)
           {
              if (pos < open_threshold) _drawer_open(obj, w, h, EINA_TRUE);
              else _drawer_close(obj, w, h, EINA_TRUE);
           }
         else
           {
              if (pos < close_threshold) _drawer_open(obj, w, h, EINA_TRUE);
              else _drawer_close(obj, w, h, EINA_TRUE);
           }
         break;

      case ELM_PANEL_ORIENT_RIGHT:
         sd->s_iface->content_pos_get(obj, &pos, NULL);

         if (sd->hidden)
           {
              if (pos > open_threshold) _drawer_open(obj, w, h, EINA_TRUE);
              else _drawer_close(obj, w, h, EINA_TRUE);
           }
         else
           {
              if (pos > close_threshold) _drawer_open(obj, w, h, EINA_TRUE);
              else _drawer_close(obj, w, h, EINA_TRUE);
           }
         break;
     }

   if (!sd->freeze && sd->hidden)
     ev->event_flags |= EVAS_EVENT_FLAG_ON_HOLD;
}

static Eina_Bool
_elm_panel_smart_event(Evas_Object *obj,
                       Evas_Object *src,
                       Evas_Callback_Type type,
                       void *event_info)
{
   // TIZEN ONLY (2013.11.27): when panel is scrollable, ignore smart event
   ELM_PANEL_DATA_GET(obj, sd);
   if (sd->scrollable) return EINA_FALSE;
   // TIZEN ONLY

   Evas_Event_Key_Down *ev = event_info;

   if (elm_widget_disabled_get(obj)) return EINA_FALSE;
   if ((src != obj) || (type != EVAS_CALLBACK_KEY_DOWN)) return EINA_FALSE;
   if (ev->event_flags & EVAS_EVENT_FLAG_ON_HOLD) return EINA_FALSE;

   if ((strcmp(ev->keyname, "Return")) &&
       (strcmp(ev->keyname, "KP_Enter")) &&
       (strcmp(ev->keyname, "space")))
     return EINA_FALSE;

   _panel_toggle(NULL, obj, NULL, NULL);

   ev->event_flags |= EVAS_EVENT_FLAG_ON_HOLD;
   return EINA_TRUE;
}

static Eina_Bool
_elm_panel_smart_content_set(Evas_Object *obj,
                             const char *part,
                             Evas_Object *content)
{
   ELM_PANEL_DATA_GET(obj, sd);

   if (part && strcmp(part, "default"))
     return ELM_CONTAINER_CLASS(_elm_panel_parent_sc)->content_set
              (obj, part, content);

   if (sd->content == content) return EINA_TRUE;
   if (sd->content)
     evas_object_box_remove_all(sd->bx, EINA_TRUE);
   sd->content = content;
   if (content)
     {
        evas_object_box_append(sd->bx, sd->content);
        evas_object_show(sd->content);
     }

   elm_layout_sizing_eval(obj);

   return EINA_TRUE;
}

static Evas_Object *
_elm_panel_smart_content_get(const Evas_Object *obj,
                             const char *part)
{
   ELM_PANEL_DATA_GET(obj, sd);

   if (part && strcmp(part, "default"))
     return ELM_CONTAINER_CLASS(_elm_panel_parent_sc)->content_get(obj, part);

   return sd->content;
}

static Evas_Object *
_elm_panel_smart_content_unset(Evas_Object *obj,
                               const char *part)
{
   Evas_Object *content;

   ELM_PANEL_DATA_GET(obj, sd);

   if (part && strcmp(part, "default"))
     return ELM_CONTAINER_CLASS
              (_elm_panel_parent_sc)->content_unset(obj, part);

   if (!sd->content) return NULL;
   content = sd->content;

   evas_object_box_remove_all(sd->bx, EINA_FALSE);
   sd->content = NULL;

   return content;
}

static void
_elm_panel_smart_add(Evas_Object *obj)
{
   EVAS_SMART_DATA_ALLOC(obj, Elm_Panel_Smart_Data);

   ELM_WIDGET_CLASS(_elm_panel_parent_sc)->base.add(obj);
}

static void
_elm_panel_smart_del(Evas_Object *obj)
{
   Evas_Object *child;
   Eina_List *l;

   ELM_PANEL_DATA_GET(obj, sd);

   sd->on_deletion = EINA_TRUE;

   ELM_SAFE_FREE(sd->timer, ecore_timer_del);
   ELM_SAFE_FREE(sd->animator, ecore_animator_del);

   /* let's make our box object the *last* to be processed, since it
    * may (smart) parent other sub objects here */
   EINA_LIST_FOREACH(ELM_WIDGET_DATA(sd)->subobjs, l, child)
     {
        if (child == sd->bx)
          {
             ELM_WIDGET_DATA(sd)->subobjs =
               eina_list_demote_list(ELM_WIDGET_DATA(sd)->subobjs, l);
             break;
          }
     }

   ELM_WIDGET_CLASS(_elm_panel_parent_sc)->base.del(obj);
}

static void
_elm_panel_smart_orientation_set(Evas_Object *obj, int rotation)
{
    ELM_PANEL_DATA_GET(obj, sd);

    if(rotation == 90 || rotation == 270)
      sd->content_size_ratio = 0.45;
    else
      sd->content_size_ratio = 0.75;
}

static void
_elm_panel_smart_move(Evas_Object *obj, Evas_Coord x, Evas_Coord y)
{
   ELM_PANEL_DATA_GET(obj, sd);

   ELM_WIDGET_CLASS(_elm_panel_parent_sc)->base.move(obj, x, y);

   evas_object_move(sd->hit_rect, x, y);
}

static Eina_Bool
_elm_panel_anim_cb(void *data)
{
   Evas_Object *obj = data;
   ELM_PANEL_DATA_GET(obj, sd);
   int w, h;

   evas_object_geometry_get(obj, NULL, NULL, &w, &h);

   if (sd->hidden) _drawer_close(obj, w, h, EINA_FALSE);
   else _drawer_open(obj, w, h, EINA_FALSE);

   sd->animator = NULL;

   return ECORE_CALLBACK_CANCEL;
}

static void
_elm_panel_smart_resize(Evas_Object *obj, Evas_Coord w, Evas_Coord h)
{
   ELM_PANEL_DATA_GET(obj, sd);
   ELM_WIDGET_CLASS(_elm_panel_parent_sc)->base.resize(obj, w, h);

   if (!sd->scrollable) return;

   evas_object_resize(sd->hit_rect, w, h);
   switch (sd->orient)
     {
      case ELM_PANEL_ORIENT_TOP:
      case ELM_PANEL_ORIENT_BOTTOM:
         // vertical
         evas_object_resize(sd->scr_ly, w, (1 + sd->content_size_ratio) * h);
         evas_object_size_hint_min_set(sd->scr_panel, w, (sd->content_size_ratio * h));
         evas_object_size_hint_min_set(sd->scr_event, w, h);
         break;
      case ELM_PANEL_ORIENT_LEFT:
      case ELM_PANEL_ORIENT_RIGHT:
         // horizontal
         evas_object_resize(sd->scr_ly, (1 + sd->content_size_ratio) * w, h);
         evas_object_size_hint_min_set(sd->scr_panel, (sd->content_size_ratio * w), h);
         evas_object_size_hint_min_set(sd->scr_event, w, h);
         break;
     }

   //Close/Open the drawer after sizing calculation is finished.
   if (sd->animator) ecore_animator_del(sd->animator);
   sd->animator = ecore_animator_add(_elm_panel_anim_cb, obj);
}

static void
_elm_panel_smart_member_add(Evas_Object *obj, Evas_Object *member)
{
   ELM_PANEL_DATA_GET(obj, sd);
   ELM_WIDGET_CLASS(_elm_panel_parent_sc)->base.member_add(obj, member);

   if (sd->hit_rect) evas_object_raise(sd->hit_rect);
}

static void
_elm_panel_smart_access(Evas_Object *obj, Eina_Bool is_access)
{
   ELM_PANEL_CHECK(obj);
   ELM_PANEL_DATA_GET(obj, sd);

   _access_obj_process(obj, is_access);

   return;
}

static Evas_Object *
_elm_panel_smart_access_object_get(Evas_Object *obj, char *part)
{
   ELM_PANEL_CHECK(obj) NULL;
   ELM_PANEL_DATA_GET(obj, sd);

   if (sd->scrollable && sd->scr_ly)
     return _access_object_get(obj, part);

   return NULL;
}

static Eina_Bool
_elm_panel_smart_disable(Evas_Object *obj)
{
   ELM_PANEL_DATA_GET(obj, sd);

   if (sd->scrollable)
     {
        if (elm_widget_disabled_get(obj))
          {
             evas_object_event_callback_del(obj, EVAS_CALLBACK_MOUSE_DOWN,
                                            _on_mouse_down);
             evas_object_event_callback_del(obj, EVAS_CALLBACK_MOUSE_MOVE,
                                            _on_mouse_move);
             evas_object_event_callback_del(obj, EVAS_CALLBACK_MOUSE_UP,
                                            _on_mouse_up);
             evas_object_event_callback_del(sd->scr_event, EVAS_CALLBACK_MOUSE_UP,
                                            _event_mouse_up);
          }
        else
          {
             evas_object_event_callback_add(obj, EVAS_CALLBACK_MOUSE_DOWN,
                                            _on_mouse_down, sd);
             evas_object_event_callback_add(obj, EVAS_CALLBACK_MOUSE_MOVE,
                                            _on_mouse_move, sd);
             evas_object_event_callback_add(obj, EVAS_CALLBACK_MOUSE_UP,
                                            _on_mouse_up, sd);
             evas_object_event_callback_add(sd->scr_event, EVAS_CALLBACK_MOUSE_UP,
                                            _event_mouse_up, obj);
          }
     }

   return EINA_TRUE;
}

static void
_elm_panel_smart_set_user(Elm_Panel_Smart_Class *sc)
{
   ELM_WIDGET_CLASS(sc)->base.add = _elm_panel_smart_add;
   ELM_WIDGET_CLASS(sc)->base.del = _elm_panel_smart_del;
   ELM_WIDGET_CLASS(sc)->base.move = _elm_panel_smart_move;
   ELM_WIDGET_CLASS(sc)->base.resize = _elm_panel_smart_resize;
   ELM_WIDGET_CLASS(sc)->base.member_add = _elm_panel_smart_member_add;

   ELM_WIDGET_CLASS(sc)->focus_next = _elm_panel_smart_focus_next;
   ELM_WIDGET_CLASS(sc)->theme = _elm_panel_smart_theme;
   ELM_WIDGET_CLASS(sc)->event = _elm_panel_smart_event;
   ELM_WIDGET_CLASS(sc)->access = _elm_panel_smart_access;
   ELM_WIDGET_CLASS(sc)->access_object_get = _elm_panel_smart_access_object_get;
   ELM_WIDGET_CLASS(sc)->disable = _elm_panel_smart_disable;
   ELM_WIDGET_CLASS(sc)->orientation_set = _elm_panel_smart_orientation_set;

   ELM_CONTAINER_CLASS(sc)->content_set = _elm_panel_smart_content_set;
   ELM_CONTAINER_CLASS(sc)->content_get = _elm_panel_smart_content_get;
   ELM_CONTAINER_CLASS(sc)->content_unset = _elm_panel_smart_content_unset;

   ELM_LAYOUT_CLASS(sc)->sizing_eval = _elm_panel_smart_sizing_eval;
}

static void
_anim_stop_cb(Evas_Object *obj, void *data EINA_UNUSED)
{
   ELM_PANEL_DATA_GET(obj, sd);

   if (elm_widget_disabled_get(obj)) return;

   if (sd->handler_bringin)
     {
        sd->handler_bringin = EINA_FALSE;
        sd->s_iface->movement_block_set(obj, ELM_SCROLLER_MOVEMENT_NO_BLOCK);
        sd->s_iface->freeze_set(obj, EINA_FALSE);
        sd->freeze = EINA_FALSE;
     }

   if (sd->down) return;

   if (sd->panel_close)
     {
        elm_object_signal_emit(obj, "elm,state,inactive,finished", "elm");
        sd->panel_close = EINA_FALSE;
     }

   if (_state_sync(obj)) return;

   Evas_Coord panel_size, open_threshold, close_threshold, pos, w, h;
   const char *open_threshold_data, *close_threshold_data;

   evas_object_geometry_get(obj, NULL, NULL, &w, &h);

   if (sd->orient == ELM_PANEL_ORIENT_TOP || sd->orient == ELM_PANEL_ORIENT_BOTTOM)
     panel_size = h * sd->content_size_ratio;
   else panel_size = w * sd->content_size_ratio;

   open_threshold_data = edje_object_data_get(sd->scr_edje, "open_threshold");
   if (open_threshold_data) open_threshold = atoi(open_threshold_data);
   else open_threshold = panel_size / 4;

   close_threshold_data = edje_object_data_get(sd->scr_edje, "close_threshold");
   if (close_threshold_data)
     {
        close_threshold = atoi(close_threshold_data);
        if (sd->orient == ELM_PANEL_ORIENT_BOTTOM || sd->orient == ELM_PANEL_ORIENT_RIGHT)
          close_threshold = panel_size - close_threshold;
     }
   else
     {
        close_threshold = panel_size - open_threshold;
        if (sd->orient == ELM_PANEL_ORIENT_TOP || sd->orient == ELM_PANEL_ORIENT_LEFT)
          close_threshold = open_threshold;
     }

   if (sd->orient == ELM_PANEL_ORIENT_TOP || sd->orient == ELM_PANEL_ORIENT_LEFT)
     open_threshold = panel_size - open_threshold;

   switch (sd->orient)
     {
      case ELM_PANEL_ORIENT_TOP:
         sd->s_iface->content_pos_get(obj, NULL, &pos);

         if (sd->hidden)
           {
              if (pos < open_threshold) _drawer_open(obj, w, h, EINA_TRUE);
              else _drawer_close(obj, w, h, EINA_TRUE);
           }
         else
           {
              if (pos < close_threshold) _drawer_open(obj, w, h, EINA_TRUE);
              else _drawer_close(obj, w, h, EINA_TRUE);
           }
         break;

      case ELM_PANEL_ORIENT_BOTTOM:
         sd->s_iface->content_pos_get(obj, NULL, &pos);

         if (sd->hidden)
           {
              if (pos > open_threshold) _drawer_open(obj, w, h, EINA_TRUE);
              else _drawer_close(obj, w, h, EINA_TRUE);
           }
         else
           {
              if (pos > close_threshold) _drawer_open(obj, w, h, EINA_TRUE);
              else _drawer_close(obj, w, h, EINA_TRUE);
           }
         break;

      case ELM_PANEL_ORIENT_LEFT:
         sd->s_iface->content_pos_get(obj, &pos, NULL);

         if (sd->hidden)
           {
              if (pos < open_threshold) _drawer_open(obj, w, h, EINA_TRUE);
              else _drawer_close(obj, w, h, EINA_TRUE);
           }
         else
           {
              if (pos < close_threshold) _drawer_open(obj, w, h, EINA_TRUE);
              else _drawer_close(obj, w, h, EINA_TRUE);
           }
         break;

      case ELM_PANEL_ORIENT_RIGHT:
         sd->s_iface->content_pos_get(obj, &pos, NULL);

         if (sd->hidden)
           {
              if (pos > open_threshold) _drawer_open(obj, w, h, EINA_TRUE);
              else _drawer_close(obj, w, h, EINA_TRUE);
           }
         else
           {
              if (pos > close_threshold) _drawer_open(obj, w, h, EINA_TRUE);
              else _drawer_close(obj, w, h, EINA_TRUE);
           }
         break;
     }
}

static void
_scroll_cb(Evas_Object *obj, void *data EINA_UNUSED)
{
   ELM_PANEL_DATA_GET(obj, sd);
   Elm_Panel_Scroll_Info event;
   Evas_Coord x, y, w, h;

   if (elm_widget_disabled_get(obj)) return;
   // in the case of
   // freeze_set(FALSE) -> mouse_up -> freeze_set(TRUE) -> scroll
   if (sd->freeze)
     {
        sd->s_iface->movement_block_set(obj, ELM_SCROLLER_MOVEMENT_NO_BLOCK);
        sd->freeze = EINA_FALSE;
        elm_layout_signal_emit(sd->scr_ly, "elm,state,content,visible", "elm");
        elm_object_signal_emit(obj, "elm,state,unhold", "elm");
     }

   sd->s_iface->content_pos_get(obj, &x, &y);
   evas_object_geometry_get(obj, NULL, NULL, &w, &h);

   switch (sd->orient)
     {
      case ELM_PANEL_ORIENT_TOP:
         event.rel_x = 1;
         event.rel_y = 1 - ((double) y / (double) ((sd->content_size_ratio) * h));
        break;
      case ELM_PANEL_ORIENT_BOTTOM:
         event.rel_x = 1;
         event.rel_y = (double) y / (double) ((sd->content_size_ratio) * h);
        break;
      case ELM_PANEL_ORIENT_LEFT:
         event.rel_x = 1 - ((double) x / (double) ((sd->content_size_ratio) * w));
         event.rel_y = 1;
        break;
      case ELM_PANEL_ORIENT_RIGHT:
         event.rel_x = (double) x / (double) ((sd->content_size_ratio) * w);
         event.rel_y = 1;
        break;
     }
   evas_object_smart_callback_call(obj, SIG_SCROLL, (void *) &event);
}

EAPI const Elm_Panel_Smart_Class *
elm_panel_smart_class_get(void)
{
   static Elm_Panel_Smart_Class _sc =
     ELM_PANEL_SMART_CLASS_INIT_NAME_VERSION(ELM_PANEL_SMART_NAME);
   static const Elm_Panel_Smart_Class *class = NULL;

   if (class) return class;

   _elm_panel_smart_set(&_sc);
   class = &_sc;

   return class;
}

EAPI Evas_Object *
elm_panel_add(Evas_Object *parent)
{
   Evas_Object *obj;

   EINA_SAFETY_ON_NULL_RETURN_VAL(parent, NULL);

   obj = elm_widget_add(_elm_panel_smart_class_new(), parent);
   if (!obj) return NULL;

   if (!elm_widget_sub_object_add(parent, obj))
     ERR("could not add %p as sub object of %p", obj, parent);

   ELM_PANEL_DATA_GET(obj, sd);

   elm_widget_can_focus_set(obj, EINA_TRUE);

   /* just to bootstrap and have theme hook to work */
   elm_layout_theme_set(obj, "panel", "top", elm_widget_style_get(obj));

   _elm_panel_smart_theme(obj);

   elm_widget_can_focus_set(obj, EINA_TRUE);

   sd->hidden = EINA_FALSE;

   sd->bx = evas_object_box_add(evas_object_evas_get(obj));
   evas_object_box_layout_set(sd->bx, _box_layout_cb, sd, NULL);
   elm_layout_content_set(obj, "elm.swallow.content", sd->bx);
   evas_object_show(sd->bx);

   elm_layout_signal_callback_add
     (obj, "elm,action,panel,toggle", "*", _panel_toggle, obj);

   _mirrored_set(obj, elm_widget_mirrored_get(obj));

   elm_layout_sizing_eval(obj);

   //Tizen Only: This should be removed when eo is applied.
   ELM_WIDGET_DATA_GET(obj, wsd);
   wsd->on_create = EINA_FALSE;
   wsd->highlight_root = EINA_TRUE;

   sd->panel_edje = wsd->resize_obj;

   return obj;
}

EAPI void
elm_panel_orient_set(Evas_Object *obj,
                     Elm_Panel_Orient orient)
{
   ELM_PANEL_CHECK(obj);
   ELM_PANEL_DATA_GET(obj, sd);

   if (sd->orient == orient) return;
   sd->orient = orient;

   if (sd->scrollable) _scrollable_layout_theme_set(obj, sd);
   else _orient_set_do(obj);

   elm_layout_sizing_eval(obj);
}

EAPI Elm_Panel_Orient
elm_panel_orient_get(const Evas_Object *obj)
{
   ELM_PANEL_CHECK(obj) ELM_PANEL_ORIENT_LEFT;
   ELM_PANEL_DATA_GET(obj, sd);

   return sd->orient;
}

EAPI void
elm_panel_hidden_set(Evas_Object *obj,
                     Eina_Bool hidden)
{
   ELM_PANEL_CHECK(obj);
   ELM_PANEL_DATA_GET(obj, sd);

   if (sd->hidden == hidden)
     {
        if (sd->hidden && !sd->freeze)
          {
             Evas_Coord w, h;
             evas_object_geometry_get(obj, NULL, NULL, &w, &h);
             _drawer_close(obj, w, h, EINA_TRUE);
          }
        return;
     }

   _panel_toggle(NULL, obj, NULL, NULL);
}

EAPI Eina_Bool
elm_panel_hidden_get(const Evas_Object *obj)
{
   ELM_PANEL_CHECK(obj) EINA_FALSE;
   ELM_PANEL_DATA_GET(obj, sd);

   return sd->hidden;
}

EAPI void
elm_panel_toggle(Evas_Object *obj)
{
   ELM_PANEL_CHECK(obj);

   _panel_toggle(NULL, obj, NULL, NULL);
}

EAPI void
elm_panel_scrollable_set(Evas_Object *obj, Eina_Bool scrollable)
{
   ELM_PANEL_DATA_GET(obj, sd);

   scrollable = !!scrollable;
   if (sd->scrollable == scrollable) return;
   sd->scrollable = scrollable;

   if (scrollable)
     {
        elm_layout_content_unset(obj, "elm.swallow.content");

        elm_widget_resize_object_set(obj, NULL, EINA_TRUE);
        elm_widget_sub_object_add(obj, sd->panel_edje);

        if (!sd->scr_edje)
          {
             const char *handler_size;

             sd->scr_edje = edje_object_add(evas_object_evas_get(obj));
             elm_widget_theme_object_set(obj, sd->scr_edje, "scroller", "panel",
                                         elm_widget_style_get(obj));
             evas_object_size_hint_weight_set
                (sd->scr_edje, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
             evas_object_size_hint_align_set
                (sd->scr_edje, EVAS_HINT_FILL, EVAS_HINT_FILL);

             handler_size = edje_object_data_get(sd->scr_edje, "handler_size");
             if (handler_size)
               sd->handler_size = (Evas_Coord) (elm_object_scale_get(obj)) * (atoi(handler_size));
          }

        elm_widget_resize_object_set(obj, sd->scr_edje, EINA_TRUE);

        if (!sd->hit_rect)
          {
             sd->s_iface = evas_object_smart_interface_get
                (obj, ELM_SCROLLABLE_IFACE_NAME);

             sd->hit_rect = evas_object_rectangle_add(evas_object_evas_get(obj));
             evas_object_smart_member_add(sd->hit_rect, obj);
             elm_widget_sub_object_add(obj, sd->hit_rect);
             evas_object_color_set(sd->hit_rect, 0, 0, 0, 0);
             evas_object_show(sd->hit_rect);
             evas_object_repeat_events_set(sd->hit_rect, EINA_TRUE);

             sd->s_iface->objects_set(obj, sd->scr_edje, sd->hit_rect);
             sd->s_iface->animate_stop_cb_set(obj, _anim_stop_cb);
             sd->s_iface->scroll_cb_set(obj, _scroll_cb);
             sd->s_iface->set_auto_scroll_enabled(obj, EINA_FALSE);
             sd->s_iface->momentum_animator_disabled_set(obj, EINA_TRUE);
          }

        if (!sd->scr_ly)
          {
             sd->scr_ly = elm_layout_add(obj);
             evas_object_smart_member_add(sd->scr_ly, obj);
             elm_widget_sub_object_add(obj, sd->scr_ly);
             _scrollable_layout_theme_set(obj, sd);

             sd->scr_panel = evas_object_rectangle_add(evas_object_evas_get(obj));
             evas_object_color_set(sd->scr_panel, 0, 0, 0, 0);
             elm_widget_sub_object_add(obj, sd->scr_panel);
             elm_layout_content_set(sd->scr_ly, "panel_area", sd->scr_panel);

             sd->scr_event = evas_object_rectangle_add(evas_object_evas_get(obj));
             evas_object_color_set(sd->scr_event, 0, 0, 0, 0);
             elm_widget_sub_object_add(obj, sd->scr_event);
             elm_layout_content_set(sd->scr_ly, "event_area", sd->scr_event);
          }

        sd->s_iface->content_set(obj, sd->scr_ly);
        sd->freeze = EINA_TRUE;
        elm_layout_content_set(sd->scr_ly, "elm.swallow.content", sd->bx);

        switch (sd->orient)
          {
           case ELM_PANEL_ORIENT_TOP:
           case ELM_PANEL_ORIENT_BOTTOM:
              sd->s_iface->movement_block_set(obj, ELM_SCROLLER_MOVEMENT_BLOCK_VERTICAL);
              break;
           case ELM_PANEL_ORIENT_LEFT:
           case ELM_PANEL_ORIENT_RIGHT:
              sd->s_iface->movement_block_set(obj, ELM_SCROLLER_MOVEMENT_BLOCK_HORIZONTAL);
              break;
          }

        sd->s_iface->single_direction_set(obj, ELM_SCROLLER_SINGLE_DIRECTION_NONE);

        if (!elm_widget_disabled_get(obj))
          {
             evas_object_event_callback_add(obj, EVAS_CALLBACK_MOUSE_DOWN,
                                            _on_mouse_down, sd);
             evas_object_event_callback_add(obj, EVAS_CALLBACK_MOUSE_MOVE,
                                            _on_mouse_move, sd);
             evas_object_event_callback_add(obj, EVAS_CALLBACK_MOUSE_UP,
                                            _on_mouse_up, sd);
             evas_object_event_callback_add(sd->scr_event, EVAS_CALLBACK_MOUSE_UP,
                                            _event_mouse_up, obj);
          }
     }
   else
     {
        sd->s_iface->content_set(obj, NULL);

        evas_object_event_callback_del(obj, EVAS_CALLBACK_MOUSE_DOWN, _on_mouse_down);
        evas_object_event_callback_del(obj, EVAS_CALLBACK_MOUSE_MOVE, _on_mouse_move);
        evas_object_event_callback_del(obj, EVAS_CALLBACK_MOUSE_UP, _on_mouse_up);
        evas_object_event_callback_del(sd->scr_event, EVAS_CALLBACK_MOUSE_UP,
                                       _event_mouse_up);

        elm_widget_resize_object_set(obj, NULL, EINA_TRUE);
        elm_widget_sub_object_add(obj, sd->scr_edje);

        elm_widget_resize_object_set(obj, sd->panel_edje, EINA_TRUE);

        elm_layout_content_unset(sd->scr_ly, "elm.swallow.content");
        elm_layout_content_set(obj, "elm.swallow.content", sd->bx);
        _orient_set_do(obj);
        if (!sd->hidden)
          {
             elm_layout_signal_emit(obj, "elm,action,show", "elm");
          }
        else
          {
             elm_layout_signal_emit(obj, "elm,action,hide", "elm");
             if (elm_widget_focus_get(sd->content))
               {
                  elm_widget_focused_object_clear(obj);
                  elm_widget_focus_steal(obj);
               }
          }
        if (sd->scr_ly)
          {
             evas_object_hide(sd->scr_ly);
          }
     }
}

EAPI Eina_Bool
elm_panel_scrollable_get(const Evas_Object *obj)
{
   ELM_PANEL_DATA_GET(obj, sd);

   return sd->scrollable;
}

EAPI void
elm_panel_scrollable_content_size_set(Evas_Object *obj, double ratio)
{
   Evas_Coord w, h;
   ELM_PANEL_DATA_GET(obj, sd);

   sd->content_size_ratio = ratio;
   evas_object_geometry_get(obj, NULL, NULL, &w, &h);

   switch (sd->orient)
     {
      case ELM_PANEL_ORIENT_TOP:
      case ELM_PANEL_ORIENT_BOTTOM:
         // vertical
         evas_object_resize(sd->scr_ly, w, (1 + sd->content_size_ratio) * h);
         evas_object_size_hint_min_set(sd->scr_panel, w, (sd->content_size_ratio * h));
         evas_object_size_hint_min_set(sd->scr_event, w, h);
         break;
      case ELM_PANEL_ORIENT_LEFT:
      case ELM_PANEL_ORIENT_RIGHT:
         // horizontal
         evas_object_resize(sd->scr_ly, (1 + sd->content_size_ratio) * w, h);
         evas_object_size_hint_min_set(sd->scr_panel, (sd->content_size_ratio * w), h);
         evas_object_size_hint_min_set(sd->scr_event, w, h);
         break;
     }

   if (sd->animator) ecore_animator_del(sd->animator);
   sd->animator = ecore_animator_add(_elm_panel_anim_cb, obj);
}
