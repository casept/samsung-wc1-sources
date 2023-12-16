#include <Elementary.h>
#include "elm_priv.h"
#include "elm_widget_ctxpopup.h"

#define TTS_STR_MENU_POPUP dgettext(PACKAGE, "IDS_SCR_OPT_MENU_POP_UP_TTS")
#define TTS_STR_MENU_CLOSE dgettext(PACKAGE, "IDS_ST_BODY_DOUBLE_TAP_TO_CLOSE_THE_MENU_T_TTS")

EAPI const char ELM_CTXPOPUP_SMART_NAME[] = "elm_ctxpopup";

static const char ACCESS_OUTLINE_PART[] = "access.outline";

static const char SIG_DISMISSED[] = "dismissed";
static const char SIG_LANG_CHANGED[] = "language,changed";
static const char SIG_ACCESS_CHANGED[] = "access,changed";

static const Evas_Smart_Cb_Description _smart_callbacks[] = {
   {SIG_DISMISSED, ""},
   {SIG_LANG_CHANGED, ""},
   {SIG_ACCESS_CHANGED, ""},
   {NULL, NULL}
};

static void _on_move(void *data __UNUSED__, Evas *e __UNUSED__, Evas_Object *obj, void *event_info __UNUSED__);
static void _item_sizing_eval(Elm_Ctxpopup_Item *item);


EVAS_SMART_SUBCLASS_NEW
  (ELM_CTXPOPUP_SMART_NAME, _elm_ctxpopup, Elm_Ctxpopup_Smart_Class,
   Elm_Layout_Smart_Class, elm_layout_smart_class_get, _smart_callbacks);

static Eina_Bool
_elm_ctxpopup_smart_translate(Evas_Object *obj)
{
   ELM_CTXPOPUP_DATA_GET(obj, sd);
   Elm_Ctxpopup_Item *it;
   Eina_List *l;

   if (sd->auto_hide)
     evas_object_hide(obj);

   EINA_LIST_FOREACH(sd->items, l, it)
     elm_widget_item_translate(it);

   evas_object_smart_callback_call(obj, SIG_LANG_CHANGED, NULL);

   return EINA_TRUE;
}

static Evas_Object *
_access_object_get(const Evas_Object *obj, const char* part)
{
   Evas_Object *po, *ao;
   ELM_CTXPOPUP_DATA_GET(obj, sd);

   po = (Evas_Object *)edje_object_part_object_get(ELM_WIDGET_DATA(sd)->resize_obj, part);
   ao = evas_object_data_get(po, "_part_access_obj");

   return ao;
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
_elm_ctxpopup_smart_focus_next(const Evas_Object *obj,
                               Elm_Focus_Direction dir,
                               Evas_Object **next)
{
   Eina_List *items = NULL;
   Evas_Object *ao;

   ELM_CTXPOPUP_DATA_GET(obj, sd);

   if (!sd)
     return EINA_FALSE;

   if (_elm_config->access_mode)
     {
        Eina_Bool ret;

        ao = _access_object_get(obj, ACCESS_OUTLINE_PART);
        if (ao) items = eina_list_append(items, ao);

        /* scroller exists when ctxpopup has an item */
        if (sd->scr)
           items = eina_list_append(items, sd->scr);
        else
           items = eina_list_append(items, sd->box);

        _elm_access_auto_highlight_set(EINA_TRUE);
        ret = elm_widget_focus_list_next_get
                 (obj, items, eina_list_data_get, dir, next);
        _elm_access_auto_highlight_set(EINA_FALSE);
        return ret;
     }
   else
     {
        elm_widget_focus_next_get(sd->box, dir, next);
        if (!*next) *next = (Evas_Object *)obj;
        return EINA_TRUE;
     }
}

static Eina_Bool
_elm_ctxpopup_smart_focus_direction_manager_is(const Evas_Object *obj __UNUSED__)
{
   return EINA_TRUE;
}

static Eina_Bool
_elm_ctxpopup_smart_focus_direction(const Evas_Object *obj,
                                    const Evas_Object *base,
                                    double degree,
                                    Evas_Object **direction,
                                    double *weight)
{
   Eina_Bool ret;
   Eina_List *l = NULL;
   void *(*list_data_get)(const Eina_List *list);

   ELM_CTXPOPUP_DATA_GET(obj, sd);

   if (!sd)
     return EINA_FALSE;

   list_data_get = eina_list_data_get;

   l = eina_list_append(l, sd->box);
   ret = elm_widget_focus_list_direction_get
      (obj, base, l, list_data_get, degree, direction, weight);
   eina_list_free(l);

   return ret;
}

static void
_x_pos_adjust(Evas_Coord_Point *pos,
              Evas_Coord_Point *base_size,
              Evas_Coord_Rectangle *hover_area)
{
   pos->x -= (base_size->x / 2);

   if (pos->x < hover_area->x)
     pos->x = hover_area->x;
   else if ((pos->x + base_size->x) > (hover_area->x + hover_area->w))
     pos->x = (hover_area->x + hover_area->w) - base_size->x;

   if (base_size->x > hover_area->w)
     base_size->x -= (base_size->x - hover_area->w);

   if (pos->x < hover_area->x)
     pos->x = hover_area->x;
}

static void
_y_pos_adjust(Evas_Coord_Point *pos,
              Evas_Coord_Point *base_size,
              Evas_Coord_Rectangle *hover_area)
{
   pos->y -= (base_size->y / 2);

   if (pos->y < hover_area->y)
     pos->y = hover_area->y;
   else if ((pos->y + base_size->y) > (hover_area->y + hover_area->h))
     pos->y = hover_area->y + hover_area->h - base_size->y;

   if (base_size->y > hover_area->h)
     base_size->y -= (base_size->y - hover_area->h);

   if (pos->y < hover_area->y)
     pos->y = hover_area->y;
}

static void
_item_select_cb(void *data,
                Evas_Object *obj __UNUSED__,
                void *event_info __UNUSED__)
{
   Elm_Ctxpopup_Item *item = data;

   if (!item) return;
   if (elm_widget_item_disabled_get(item))
     {
        if (_elm_config->access_mode)
          elm_access_say(E_("IDS_ACCS_BODY_DISABLED_TTS"));
        return;
     }

   if (item->func)
     item->func((void*)item->base.data, WIDGET(item), data);
}

static char *
_access_info_cb(void *data, Evas_Object *obj __UNUSED__)
{
   Elm_Ctxpopup_Item *it = (Elm_Ctxpopup_Item *)data;
   const char *txt = NULL;
   Evas_Object *icon = NULL;

   if (!it) return NULL;

   txt = it->label;
   icon = it->icon;

   if (txt) return strdup(txt);
   if (icon) return strdup(E_("IDS_TPLATFORM_BODY_ICON"));
   return NULL;
}

static char *
_access_state_cb(void *data, Evas_Object *obj __UNUSED__)
{
   Elm_Ctxpopup_Item *it = (Elm_Ctxpopup_Item *)data;
   if (!it) return NULL;

   if (it->base.disabled)
     return strdup(E_("IDS_ACCS_BODY_DISABLED_TTS"));

   return NULL;
}

static void
_access_focusable_button_register(Evas_Object *obj, Elm_Ctxpopup_Item *it)
{
   Elm_Access_Info *ai;

   ai = _elm_access_object_get(obj);

   _elm_access_callback_set(ai, ELM_ACCESS_INFO, _access_info_cb, it);
   _elm_access_callback_set(ai, ELM_ACCESS_STATE, _access_state_cb, it);
   _elm_access_callback_set(ai, ELM_ACCESS_TYPE, NULL, NULL);

   ((Elm_Widget_Item *)it)->access_obj = obj;
}

static void
_mouse_down_cb(Elm_Ctxpopup_Item *it,
               Evas *evas __UNUSED__,
               Evas_Object *obj __UNUSED__,
               Evas_Event_Mouse_Down *ev)
{
   ELM_CTXPOPUP_DATA_GET(WIDGET(it), sd);

   if (ev->button != 1) return;

   //If counter is not zero, it means all other multi down is not released.
   if (sd->multi_down != 0) return;
   sd->mouse_down = EINA_TRUE;
//******************** TIZEN Only
   sd->pressed = EINA_TRUE;
//****************************
}

//******************** TIZEN Only
static void
_mouse_move_cb(Elm_Ctxpopup_Item *it,
               Evas *evas __UNUSED__,
               Evas_Object *obj,
               void *event_info)
{
   ELM_CTXPOPUP_DATA_GET(WIDGET(it), sd);
   Evas_Event_Mouse_Move *ev = event_info;

   if (ev->event_flags & EVAS_EVENT_FLAG_ON_HOLD)
     {
        if (sd->pressed)
          {
             edje_object_signal_emit(obj,"elm,action,unpressed", "elm");
             sd->pressed = EINA_FALSE;
          }
     }
   else
     {
        Evas_Coord x, y, w, h;
        evas_object_geometry_get(obj, &x, &y, &w, &h);
        if ((sd->pressed) && (ELM_RECTS_POINT_OUT(x, y, w, h, ev->cur.canvas.x, ev->cur.canvas.y)))
          {
             edje_object_signal_emit(obj,"elm,action,unpressed", "elm");
             sd->pressed = EINA_FALSE;
          }
     }
}
//****************************

static void
_mouse_up_cb(Elm_Ctxpopup_Item *it,
             Evas *evas __UNUSED__,
             Evas_Object *obj __UNUSED__,
             Evas_Event_Mouse_Up *ev)
{
   ELM_CTXPOPUP_DATA_GET(WIDGET(it), sd);

   if (ev->button != 1) return;

   if (!sd->mouse_down) return;
   sd->mouse_down = EINA_FALSE;
//******************** TIZEN Only
   sd->pressed = EINA_FALSE;
//****************************
}

static void
_multi_down_cb(void *data,
                    Evas *evas __UNUSED__,
                    Evas_Object *obj __UNUSED__,
                    void *event_info __UNUSED__)
{
   Elm_Ctxpopup_Item *it = data;
   ELM_CTXPOPUP_DATA_GET(WIDGET(it), sd);

   //Emitting signal to inform edje about multi down event.
   edje_object_signal_emit(VIEW(it), "elm,action,multi,down", "elm");
   sd->multi_down++;
}

static void
_multi_up_cb(void *data,
                    Evas *evas __UNUSED__,
                    Evas_Object *obj __UNUSED__,
                    void *event_info __UNUSED__)
{
   Elm_Ctxpopup_Item *it = data;
   ELM_CTXPOPUP_DATA_GET(WIDGET(it), sd);

   sd->multi_down--;
   if(sd->multi_down == 0)
     {
        //Emitting signal to inform edje about last multi up event.
        edje_object_signal_emit(VIEW(it), "elm,action,multi,up", "elm");
     }
}

static void
_item_new(Elm_Ctxpopup_Item *item,
          char *group_name)
{
   ELM_CTXPOPUP_DATA_GET(WIDGET(item), sd);
   if (!sd) return;

   VIEW(item) = edje_object_add(evas_object_evas_get(sd->box));
   edje_object_mirrored_set(VIEW(item), elm_widget_mirrored_get(WIDGET(item)));
   _elm_theme_object_set(WIDGET(item), VIEW(item), "ctxpopup", group_name,
                         elm_widget_style_get(WIDGET(item)));
   evas_object_size_hint_align_set(VIEW(item), EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_show(VIEW(item));

   evas_object_event_callback_add
     (VIEW(item), EVAS_CALLBACK_MOUSE_DOWN, (Evas_Object_Event_Cb)_mouse_down_cb,
     item);
//******************** TIZEN Only
   evas_object_event_callback_add
     (VIEW(item), EVAS_CALLBACK_MOUSE_MOVE, (Evas_Object_Event_Cb)_mouse_move_cb,
     item);
//****************************
   evas_object_event_callback_add
     (VIEW(item), EVAS_CALLBACK_MOUSE_UP, (Evas_Object_Event_Cb)_mouse_up_cb, item);

   /*Registering Multi down/up events to ignore mouse down/up events untill all
     multi down/up events are released.*/
   evas_object_event_callback_add
     (VIEW(item), EVAS_CALLBACK_MULTI_DOWN, (Evas_Object_Event_Cb)_multi_down_cb,
     item);
   evas_object_event_callback_add
     (VIEW(item), EVAS_CALLBACK_MULTI_UP, (Evas_Object_Event_Cb)_multi_up_cb,
     item);
}

static void
_item_icon_set(Elm_Ctxpopup_Item *item,
               Evas_Object *icon)
{
   if (item->icon)
     evas_object_del(item->icon);

   item->icon = icon;
   if (!icon) return;

   edje_object_part_swallow(VIEW(item), "elm.swallow.icon", item->icon);
}

static void
_item_label_set(Elm_Ctxpopup_Item *item,
                const char *label)
{
   if (!eina_stringshare_replace(&item->label, label))
     return;

   ELM_CTXPOPUP_DATA_GET(WIDGET(item), sd);

   edje_object_part_text_escaped_set(VIEW(item), "elm.text", label);
   if (sd->visible) _item_sizing_eval(item);
}

static Evas_Object *
_item_in_focusable_button(Elm_Ctxpopup_Item *item)
{
   Evas_Object *bt;

   bt = elm_button_add(WIDGET(item));
   elm_object_style_set(bt, "focus");
   evas_object_size_hint_align_set(bt, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_object_part_content_set(bt, "elm.swallow.content", VIEW(item));
   evas_object_smart_callback_add(bt, "clicked", _item_select_cb, item);
   evas_object_show(bt);

   return bt;
}

static Eina_Bool
_item_del_pre_hook(Elm_Object_Item *it)
{
   Elm_Ctxpopup_Item *ctxpopup_it = (Elm_Ctxpopup_Item *)it;
   Evas_Object *btn;

   ELM_CTXPOPUP_DATA_GET(WIDGET(ctxpopup_it), sd);
   if (!sd) return EINA_FALSE;

   btn = ctxpopup_it->btn;
   elm_box_unpack(sd->box, btn);
   evas_object_smart_callback_del_full(btn, "clicked", _item_select_cb, ctxpopup_it);
   evas_object_del(btn);

   sd->dir = ELM_CTXPOPUP_DIRECTION_UNKNOWN;

   if (ctxpopup_it->icon)
     evas_object_del(ctxpopup_it->icon);
   if (VIEW(ctxpopup_it))
     evas_object_del(VIEW(ctxpopup_it));

   eina_stringshare_del(ctxpopup_it->label);
   sd->items = eina_list_remove(sd->items, ctxpopup_it);

   if (eina_list_count(sd->items) < 1)
     {
        evas_object_hide(WIDGET(ctxpopup_it));
        return EINA_TRUE;
     }
   elm_layout_sizing_eval(WIDGET(ctxpopup_it));

#ifndef ELM_FEATURE_WEARABLE
   if (ctxpopup_it == sd->top_drawn_item) sd->top_drawn_item = NULL;
#endif
   return EINA_TRUE;
}

static void
_items_remove(Elm_Ctxpopup_Smart_Data *sd)
{
   Eina_List *l, *l_next;
   Elm_Ctxpopup_Item *item;

   if (!sd->items) return;

   EINA_LIST_FOREACH_SAFE(sd->items, l, l_next, item)
     elm_widget_item_del(item);

   sd->items = NULL;
}

static void
_item_sizing_eval(Elm_Ctxpopup_Item *item)
{
   Evas_Coord min_w = -1, min_h = -1, max_w = -1, max_h = -1;

   if (!item) return;

   edje_object_signal_emit(VIEW(item), "elm,state,text,default", "elm");
   edje_object_message_signal_process(VIEW(item));
   edje_object_size_min_restricted_calc(VIEW(item), &min_w, &min_h, min_w,
                                        min_h);
   evas_object_size_hint_min_set(VIEW(item), min_w, min_h);
   evas_object_size_hint_max_set(VIEW(item), max_w, max_h);
}

static Elm_Ctxpopup_Direction
_base_geometry_calc(Evas_Object *obj,
                    Evas_Coord_Rectangle *rect)
{
   Elm_Ctxpopup_Direction dir = ELM_CTXPOPUP_DIRECTION_UNKNOWN;
   Evas_Coord_Rectangle hover_area;
   Evas_Coord_Point pos = {0, 0};
   Evas_Coord_Point arrow_size;
   Evas_Coord_Point base_size;
   Evas_Coord_Point max_size;
   Evas_Coord_Point min_size;
   Evas_Coord_Point temp;
   int idx;
   const char *str;

   ELM_CTXPOPUP_DATA_GET(obj, sd);

   if (!rect) return ELM_CTXPOPUP_DIRECTION_DOWN;

   edje_object_part_geometry_get
     (sd->arrow, "ctxpopup_arrow", NULL, NULL, &arrow_size.x, &arrow_size.y);
   evas_object_resize(sd->arrow, arrow_size.x, arrow_size.y);

   //Initialize Area Rectangle.
   evas_object_geometry_get
     (sd->parent, &hover_area.x, &hover_area.y, &hover_area.w,
     &hover_area.h);

   evas_object_geometry_get(obj, &pos.x, &pos.y, NULL, NULL);

   //recalc the edje
   edje_object_size_min_calc
     (ELM_WIDGET_DATA(sd)->resize_obj, &base_size.x, &base_size.y);
   evas_object_smart_calculate(ELM_WIDGET_DATA(sd)->resize_obj);

   //Limit to Max Size
   evas_object_size_hint_max_get(obj, &max_size.x, &max_size.y);
   if ((max_size.x == -1) || (max_size.y == -1))
     {
        str = edje_object_data_get(sd->layout, "visible_maxw");
        if (str)
          max_size.x = atoi(str) / edje_object_base_scale_get(sd->layout) * elm_widget_scale_get(obj) * elm_config_scale_get();
        str = edje_object_data_get(sd->layout, "visible_maxh");
        if (str)
          max_size.y = atoi(str) / edje_object_base_scale_get(sd->layout) * elm_widget_scale_get(obj) * elm_config_scale_get();
     }

   if ((max_size.y > 0) && (base_size.y > max_size.y))
     base_size.y = max_size.y;

   if ((max_size.x > 0) && (base_size.x > max_size.x))
     base_size.x = max_size.x;

   //Limit to Min Size
   evas_object_size_hint_min_get(obj, &min_size.x, &min_size.y);
   if ((min_size.x == 0) || (min_size.y == 0))
     edje_object_size_min_get(sd->layout, &min_size.x, &min_size.y);

   if ((min_size.y > 0) && (base_size.y < min_size.y))
     base_size.y = min_size.y;

   if ((min_size.x > 0) && (base_size.x < min_size.x))
     base_size.x = min_size.x;

   //Check the Which direction is available.
   //If find a avaialble direction, it adjusts position and size.
   for (idx = 0; idx < 4; idx++)
     {
        switch (sd->dir_priority[idx])
          {
           case ELM_CTXPOPUP_DIRECTION_UNKNOWN:

           case ELM_CTXPOPUP_DIRECTION_UP:
             temp.y = (pos.y - base_size.y);
             if ((temp.y - arrow_size.y) < hover_area.y)
               continue;

             //Tizen Only(20141013): dropdown/label style's x position should follow app's evas_object_move position
             if (!strncmp(elm_object_style_get(obj), "dropdown/label", strlen("dropdown/label")))
               pos.x += base_size.x / 2;
             //Tizen Only end
             _x_pos_adjust(&pos, &base_size, &hover_area);
             pos.y -= base_size.y;
             dir = ELM_CTXPOPUP_DIRECTION_UP;
             break;

           case ELM_CTXPOPUP_DIRECTION_LEFT:
             temp.x = (pos.x - base_size.x);
             if ((temp.x - arrow_size.x) < hover_area.x)
               continue;

             _y_pos_adjust(&pos, &base_size, &hover_area);
             pos.x -= base_size.x;
             dir = ELM_CTXPOPUP_DIRECTION_LEFT;
             break;

           case ELM_CTXPOPUP_DIRECTION_RIGHT:
             temp.x = (pos.x + base_size.x);
             if ((temp.x + arrow_size.x) >
                 (hover_area.x + hover_area.w))
               continue;

             _y_pos_adjust(&pos, &base_size, &hover_area);
             dir = ELM_CTXPOPUP_DIRECTION_RIGHT;
             break;

           case ELM_CTXPOPUP_DIRECTION_DOWN:
             temp.y = (pos.y + base_size.y);
             if ((temp.y + arrow_size.y) >
                 (hover_area.y + hover_area.h))
               continue;

             //Tizen Only(20141013): dropdown/label style's x position should follow app's evas_object_move position
             if (!strncmp(elm_object_style_get(obj), "dropdown/label", strlen("dropdown/label")))
               pos.x += base_size.x / 2;
             //Tizen Only end
             _x_pos_adjust(&pos, &base_size, &hover_area);
             dir = ELM_CTXPOPUP_DIRECTION_DOWN;
             break;

           default:
             continue;
          }
        break;
     }

   //In this case, all directions are invalid because of lack of space.
   if (idx == 4)
     {
        Evas_Coord length[2];

        if (!sd->horizontal)
          {
             length[0] = pos.y - hover_area.y;
             length[1] = (hover_area.y + hover_area.h) - pos.y;

             // ELM_CTXPOPUP_DIRECTION_UP
             if (length[0] > length[1])
               {
                  //Tizen Only(20141013): dropdown/label style's x position should follow app's evas_object_move position
                  if (!strncmp(elm_object_style_get(obj), "dropdown/label", strlen("dropdown/label")))
                    pos.x += base_size.x / 2;
                  //Tizen Only end
                  _x_pos_adjust(&pos, &base_size, &hover_area);
                  pos.y -= base_size.y;
                  dir = ELM_CTXPOPUP_DIRECTION_UP;
                  if (pos.y < (hover_area.y + arrow_size.y))
                    {
                       base_size.y -= ((hover_area.y + arrow_size.y) - pos.y);
                       pos.y = hover_area.y + arrow_size.y;
                    }
               }
             //ELM_CTXPOPUP_DIRECTION_DOWN
             else
               {
                  //Tizen Only(20141013): dropdown/label style's x position should follow app's evas_object_move position
                  if (!strncmp(elm_object_style_get(obj), "dropdown/label", strlen("dropdown/label")))
                    pos.x += base_size.x / 2;
                  //Tizen Only end
                  _x_pos_adjust(&pos, &base_size, &hover_area);
                  dir = ELM_CTXPOPUP_DIRECTION_DOWN;
                  if ((pos.y + arrow_size.y + base_size.y) >
                      (hover_area.y + hover_area.h))
                    base_size.y -=
                      ((pos.y + arrow_size.y + base_size.y) -
                       (hover_area.y + hover_area.h));
               }
          }
        else
          {
             length[0] = pos.x - hover_area.x;
             length[1] = (hover_area.x + hover_area.w) - pos.x;

             //ELM_CTXPOPUP_DIRECTION_LEFT
             if (length[0] > length[1])
               {
                  _y_pos_adjust(&pos, &base_size, &hover_area);
                  pos.x -= base_size.x;
                  dir = ELM_CTXPOPUP_DIRECTION_LEFT;
                  if (pos.x < (hover_area.x + arrow_size.x))
                    {
                       base_size.x -= ((hover_area.x + arrow_size.x) - pos.x);
                       pos.x = hover_area.x + arrow_size.x;
                    }
               }
             //ELM_CTXPOPUP_DIRECTION_RIGHT
             else
               {
                  _y_pos_adjust(&pos, &base_size, &hover_area);
                  dir = ELM_CTXPOPUP_DIRECTION_RIGHT;
                  if (pos.x + (arrow_size.x + base_size.x) >
                      hover_area.x + hover_area.w)
                    base_size.x -=
                      ((pos.x + arrow_size.x + base_size.x) -
                       (hover_area.x + hover_area.w));
               }
          }
     }

   //Final position and size.
   rect->x = pos.x;
   rect->y = pos.y;
   rect->w = base_size.x;
   rect->h = base_size.y;

   return dir;
}

static void
_arrow_update(Evas_Object *obj,
              Elm_Ctxpopup_Direction dir,
              Evas_Coord_Rectangle base_size)
{
   Evas_Coord_Rectangle arrow_size;
   Evas_Coord x, y;
   double drag;
   Evas_Coord_Rectangle shadow_left_top, shadow_right_bottom, arrow_padding;

   ELM_CTXPOPUP_DATA_GET(obj, sd);

   evas_object_geometry_get(obj, &x, &y, NULL, NULL);
   evas_object_geometry_get
     (sd->arrow, NULL, NULL, &arrow_size.w, &arrow_size.h);

   /* tizen only : since ctxpopup of tizen has shadow, start and end padding of arrow, it should be put together when updating arrow
    * so there are some differences between open source and tizen */
   edje_object_part_geometry_get(ELM_WIDGET_DATA(sd)->resize_obj, "frame_shadow_left_top_padding", NULL, NULL, &shadow_left_top.w, &shadow_left_top.h);
   edje_object_part_geometry_get(ELM_WIDGET_DATA(sd)->resize_obj, "frame_shadow_right_bottom_padding", NULL, NULL, &shadow_right_bottom.w, &shadow_right_bottom.h);
   edje_object_part_geometry_get(ELM_WIDGET_DATA(sd)->resize_obj, "ctxpopup_frame_left_top", NULL, NULL, &arrow_padding.w, &arrow_padding.h);

   /* arrow is not being kept as sub-object on purpose, here. the
    * design of the widget does not help with the contrary */

   switch (dir)
     {
      case ELM_CTXPOPUP_DIRECTION_RIGHT:
        edje_object_signal_emit(sd->arrow, "elm,state,left", "elm");
        edje_object_part_swallow
           (ELM_WIDGET_DATA(sd)->resize_obj,
            (elm_widget_mirrored_get(obj) ? "elm.swallow.arrow_right" :
             "elm.swallow.arrow_left"), sd->arrow);

        if (base_size.h > 0)
          {
             if (y <= ((arrow_size.h * 0.5) + base_size.y + shadow_left_top.h + arrow_padding.h))
               y = 0;
             else if (y >= (base_size.y + base_size.h - ((arrow_size.h * 0.5) + shadow_right_bottom.h + arrow_padding.h)))
               y = base_size.h - (arrow_size.h + shadow_right_bottom.h + shadow_left_top.h + (arrow_padding.h * 2));
             else
               y = y - base_size.y - ((arrow_size.h * 0.5) + shadow_left_top.h + arrow_padding.h);
             drag = (double)(y) / (double)(base_size.h - (arrow_size.h + shadow_right_bottom.h + shadow_left_top.h + (arrow_padding.h * 2)));
             edje_object_part_drag_value_set
                (ELM_WIDGET_DATA(sd)->resize_obj,
                 (elm_widget_mirrored_get(obj) ? "elm.swallow.arrow_right" :
                  "elm.swallow.arrow_left"), 1, drag);
          }
        break;

      case ELM_CTXPOPUP_DIRECTION_LEFT:
        edje_object_signal_emit(sd->arrow, "elm,state,right", "elm");
        edje_object_part_swallow
           (ELM_WIDGET_DATA(sd)->resize_obj,
            (elm_widget_mirrored_get(obj) ? "elm.swallow.arrow_left" :
             "elm.swallow.arrow_right"), sd->arrow);

        if (base_size.h > 0)
          {
             if (y <= ((arrow_size.h * 0.5) + base_size.y + shadow_left_top.h + arrow_padding.h))
               y = 0;
             else if (y >= (base_size.y + base_size.h - ((arrow_size.h * 0.5) + shadow_right_bottom.h + arrow_padding.h)))
               y = base_size.h - (arrow_size.h + shadow_right_bottom.h + shadow_left_top.h + (arrow_padding.h * 2));
             else
               y = y - base_size.y - ((arrow_size.h * 0.5) + shadow_left_top.h + arrow_padding.h);
             drag = (double)(y) / (double)(base_size.h - (arrow_size.h + shadow_right_bottom.h + shadow_left_top.h + (arrow_padding.h * 2)));
             edje_object_part_drag_value_set
                (ELM_WIDGET_DATA(sd)->resize_obj,
                 (elm_widget_mirrored_get(obj) ? "elm.swallow.arrow_left" :
                  "elm.swallow.arrow_right"), 0, drag);
          }
        break;

      case ELM_CTXPOPUP_DIRECTION_DOWN:
        edje_object_signal_emit(sd->arrow, "elm,state,top", "elm");
        edje_object_part_swallow
          (ELM_WIDGET_DATA(sd)->resize_obj, "elm.swallow.arrow_up",
          sd->arrow);

        if (base_size.w > 0)
          {
             if (x <= ((arrow_size.w * 0.5) + base_size.x + shadow_left_top.w + arrow_padding.w))
               x = 0;
             else if (x >= (base_size.x + base_size.w - ((arrow_size.w * 0.5) + shadow_right_bottom.w + arrow_padding.w)))
               x = base_size.w - (arrow_size.w + shadow_right_bottom.w + shadow_left_top.w + (arrow_padding.w * 2));
             else
               x = x - base_size.x - ((arrow_size.w * 0.5) + shadow_left_top.w + arrow_padding.w);
             drag = (double)(x) / (double)(base_size.w - (arrow_size.w + shadow_right_bottom.w + shadow_left_top.w + (arrow_padding.w * 2)));
             edje_object_part_drag_value_set
               (ELM_WIDGET_DATA(sd)->resize_obj, "elm.swallow.arrow_up",
               drag, 1);
          }
        break;

      case ELM_CTXPOPUP_DIRECTION_UP:
        edje_object_signal_emit(sd->arrow, "elm,state,bottom", "elm");
        edje_object_part_swallow
          (ELM_WIDGET_DATA(sd)->resize_obj, "elm.swallow.arrow_down",
          sd->arrow);

        if (base_size.w > 0)
          {
             if (x <= ((arrow_size.w * 0.5) + base_size.x + shadow_left_top.w + arrow_padding.w))
               x = 0;
             else if (x >= (base_size.x + base_size.w - ((arrow_size.w * 0.5) + shadow_right_bottom.w + arrow_padding.w)))
               x = base_size.w - (arrow_size.w + shadow_right_bottom.w + shadow_left_top.w + (arrow_padding.w * 2));
             else
               x = x - base_size.x - ((arrow_size.w * 0.5) + shadow_left_top.w + arrow_padding.w);
             drag = (double)(x) / (double)(base_size.w - (arrow_size.w + shadow_right_bottom.w + shadow_left_top.w + (arrow_padding.w * 2)));
             edje_object_part_drag_value_set
               (ELM_WIDGET_DATA(sd)->resize_obj, "elm.swallow.arrow_down",
               drag, 0);
          }
        break;

      default:
        break;
     }

   //should be here for getting accurate geometry value
   evas_object_smart_calculate(ELM_WIDGET_DATA(sd)->resize_obj);
}

static void
_show_signals_emit(Evas_Object *obj,
                   Elm_Ctxpopup_Direction dir)
{
   ELM_CTXPOPUP_DATA_GET(obj, sd);

   if (!sd->visible) return;

   switch (dir)
     {
      case ELM_CTXPOPUP_DIRECTION_UP:
        edje_object_signal_emit(sd->layout, "elm,state,show,up", "elm");
        break;

      case ELM_CTXPOPUP_DIRECTION_LEFT:
        edje_object_signal_emit(sd->layout, (elm_widget_mirrored_get(obj) ? "elm,state,show,right" :
               "elm,state,show,left"), "elm");
        break;

      case ELM_CTXPOPUP_DIRECTION_RIGHT:
        edje_object_signal_emit(sd->layout, (elm_widget_mirrored_get(obj) ? "elm,state,show,left" :
               "elm,state,show,right"), "elm");
        break;

      case ELM_CTXPOPUP_DIRECTION_DOWN:
        edje_object_signal_emit(sd->layout, "elm,state,show,down", "elm");
        break;

      default:
        break;
     }

   edje_object_signal_emit(sd->bg, "elm,state,show", "elm");
}

static void
_hide_signals_emit(Evas_Object *obj,
                   Elm_Ctxpopup_Direction dir)
{
   ELM_CTXPOPUP_DATA_GET(obj, sd);

   if (!sd->visible) return;

   switch (dir)
     {
      case ELM_CTXPOPUP_DIRECTION_UP:
        edje_object_signal_emit(sd->layout, "elm,state,hide,up", "elm");
        break;

      case ELM_CTXPOPUP_DIRECTION_LEFT:
        edje_object_signal_emit(sd->layout, (elm_widget_mirrored_get(obj) ? "elm,state,hide,right" :
               "elm,state,hide,left"), "elm");
        break;

      case ELM_CTXPOPUP_DIRECTION_RIGHT:
        edje_object_signal_emit(sd->layout, (elm_widget_mirrored_get(obj) ? "elm,state,hide,left" :
               "elm,state,hide,right"), "elm");
        break;

      case ELM_CTXPOPUP_DIRECTION_DOWN:
        edje_object_signal_emit(sd->layout, "elm,state,hide,down", "elm");
        break;

      default:
        break;
     }

   edje_object_signal_emit(sd->bg, "elm,state,hide", "elm");
}

static void
_base_shift_by_arrow(Evas_Object *obj,
                     Evas_Object *arrow,
                     Elm_Ctxpopup_Direction dir,
                     Evas_Coord_Rectangle *rect)
{
   ELM_CTXPOPUP_DATA_GET(obj, sd);

   Evas_Coord arrow_w, arrow_h, diff_w, diff_h;
   Evas_Coord_Rectangle shadow_left_top, shadow_right_bottom;

   evas_object_geometry_get(arrow, NULL, NULL, &arrow_w, &arrow_h);
   /* tizen only: since ctxpopup of tizen has shadow parts, start and end padding of arrow, it should be put together when shifting ctxpopup by arrow
    * so there are some differences between opensource and tizen*/
   edje_object_part_geometry_get(ELM_WIDGET_DATA(sd)->resize_obj, "frame_shadow_left_top_padding", NULL, NULL, &shadow_left_top.w, &shadow_left_top.h);
   edje_object_part_geometry_get(ELM_WIDGET_DATA(sd)->resize_obj, "frame_shadow_right_bottom_padding", NULL, NULL, &shadow_right_bottom.w, &shadow_right_bottom.h);
   //

   switch (dir)
     {
      case ELM_CTXPOPUP_DIRECTION_RIGHT:
        diff_w = arrow_w - shadow_right_bottom.w;
        rect->x += diff_w;
        break;

      case ELM_CTXPOPUP_DIRECTION_LEFT:
        diff_w = arrow_w - shadow_left_top.w;
        rect->x -= diff_w;
        break;

      case ELM_CTXPOPUP_DIRECTION_DOWN:
        diff_h = arrow_h - shadow_left_top.h;
        rect->y += diff_h;
        break;

      case ELM_CTXPOPUP_DIRECTION_UP:
        diff_h = arrow_h - shadow_right_bottom.h;
        rect->y -= diff_h;
        break;

      default:
         break;
     }
}

static Eina_Bool
_elm_ctxpopup_smart_sub_object_add(Evas_Object *obj,
                                   Evas_Object *sobj)
{
   Elm_Widget_Smart_Class *parent_parent;

   parent_parent = (Elm_Widget_Smart_Class *)((Evas_Smart_Class *)
                                              _elm_ctxpopup_parent_sc)->parent;

   /* skipping layout's code, which registers size hint changing
    * callback on sub objects. a hack to make ctxpopup live, as it is,
    * on the new classing schema. this widget needs a total
    * rewrite. */
   if (!parent_parent->sub_object_add(obj, sobj))
     return EINA_FALSE;

   return EINA_TRUE;
}

static void
_elm_ctxpopup_smart_sizing_eval(Evas_Object *obj)
{
   Eina_List *elist;
   Elm_Ctxpopup_Item *item;
   Evas_Coord_Rectangle rect = { 0, 0, 1, 1 };
   Evas_Coord_Point box_size = { 0, 0 };
   Evas_Coord_Point _box_size = { 0, 0 };
   Evas_Coord maxw = 0;
   Evas_Coord x, y, w, h;
   const char *str;

   ELM_CTXPOPUP_DATA_GET(obj, sd);

   if (!sd->parent || !(sd->items || sd->content)) return;
   if (!sd->visible && !sd->dir_requested) return;

   //Box, Scroller
   EINA_LIST_FOREACH(sd->items, elist, item)
     {
        _item_sizing_eval(item);
        evas_object_size_hint_min_get(VIEW(item), &_box_size.x, &_box_size.y);
#ifndef ELM_FEATURE_WEARABLE
   //Kiran only
   if (!sd->color_calculated)
     {
        int item_r, item_g, item_b, item_a = 0;
        int bg_r, bg_g, bg_b, bg_a = 0;
        int item_banded_a ,bg_banded_a = 0;
        const char *item_bg_color = NULL;
        const char *list_bg_color = NULL;
        int i = 0;
        item_bg_color = edje_object_data_get(VIEW(item), "bg_color");
        list_bg_color = edje_object_data_get(sd->bg, "personalized_color");
        if (item_bg_color != NULL && list_bg_color != NULL )
          {
             edje_object_color_class_get (ELM_WIDGET_DATA(sd)->resize_obj, item_bg_color, &item_r, &item_g, &item_b, &item_a, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
             edje_object_color_class_get (ELM_WIDGET_DATA(sd)->resize_obj, list_bg_color, &bg_r, &bg_g, &bg_b, &bg_a, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
             for (i = 0; i < MAX_ITEMS_PER_VIEWPORT; i++)
                {
                   item_banded_a = 255 - (i+1)*10;
                   bg_banded_a = 255 - item_banded_a;
                   sd->item_color[i][0] = ((item_r * item_banded_a) >> 8) + ((bg_r * bg_banded_a) >> 8);
                   sd->item_color[i][1] = ((item_g * item_banded_a) >> 8) + ((bg_g * bg_banded_a) >> 8);
                   sd->item_color[i][2] = ((item_b * item_banded_a) >> 8) + ((bg_b * bg_banded_a) >> 8);
                }
             sd->color_calculated = EINA_TRUE;
          }
     }
#endif
        str = edje_object_data_get(VIEW(item), "item_max_size");
        if (str)
          {
             maxw = atoi(str);
             maxw = maxw / edje_object_base_scale_get(VIEW(item)) * elm_widget_scale_get(obj) * elm_config_scale_get();

             if (_box_size.x > maxw)
               {
                  edje_object_signal_emit(VIEW(item), "elm,state,text,ellipsis", "elm");
                  edje_object_message_signal_process(VIEW(item));
               }
          }

        if (!sd->horizontal)
          {
             if (_box_size.x > box_size.x)
               box_size.x = _box_size.x;
             if (_box_size.y != -1)
               box_size.y += _box_size.y;
          }
        else
          {
             if (_box_size.x != -1)
               box_size.x += _box_size.x;
             if (_box_size.y > box_size.y)
               box_size.y = _box_size.y;
          }
     }

//   if (!sd->arrow) return;  /* simple way to flag "under deletion" */

   if ((!sd->content) && (sd->scr))
     {
        evas_object_size_hint_min_set(sd->box, box_size.x, box_size.y);
        elm_scroller_content_min_limit(sd->scr, EINA_TRUE, EINA_TRUE);
        evas_object_size_hint_min_set(sd->scr, box_size.x, box_size.y);
     }

   //Base
   sd->dir = _base_geometry_calc(obj, &rect);

   _arrow_update(obj, sd->dir, rect);
   _base_shift_by_arrow(obj, sd->arrow, sd->dir, &rect);

   //resize scroller according to final size
   if ((!sd->content) && (sd->scr))
     {
        elm_scroller_content_min_limit(sd->scr, EINA_FALSE, EINA_FALSE);
        evas_object_smart_calculate(sd->scr);
     }

   evas_object_size_hint_min_set(ELM_WIDGET_DATA(sd)->resize_obj, rect.w, rect.h);
   evas_object_resize(ELM_WIDGET_DATA(sd)->resize_obj, rect.w, rect.h);

   evas_object_resize(sd->layout, rect.w, rect.h);
   evas_object_move(sd->layout, rect.x, rect.y);

   evas_object_geometry_get(sd->parent, &x, &y, &w, &h);
   evas_object_move(sd->bg, x, y);
   evas_object_resize(sd->bg, w, h);
}

static void
_on_parent_del(void *data,
               Evas *e __UNUSED__,
               Evas_Object *obj __UNUSED__,
               void *event_info __UNUSED__)
{
   evas_object_del(data);
}

static void
_on_parent_move(void *data,
                Evas *e __UNUSED__,
                Evas_Object *obj __UNUSED__,
                void *event_info __UNUSED__)
{
   ELM_CTXPOPUP_DATA_GET(data, sd);


   sd->dir = ELM_CTXPOPUP_DIRECTION_UNKNOWN;
   elm_layout_sizing_eval(data);
}

static void
_on_parent_resize(void *data,
                  Evas *e __UNUSED__,
                  Evas_Object *obj __UNUSED__,
                  void *event_info __UNUSED__)
{
   ELM_CTXPOPUP_DATA_GET(data, sd);
   ELM_WIDGET_DATA_GET(data, wsd);
   Edje_Message_Int msg;

   if (sd->auto_hide)
     {
        _hide_signals_emit(data, sd->dir);

        sd->dir = ELM_CTXPOPUP_DIRECTION_UNKNOWN;

        evas_object_hide(data);
        evas_object_smart_callback_call(data, SIG_DISMISSED, NULL);
     }
   else
     {
        if (wsd->orient_mode == 90 || wsd->orient_mode == 270)
         elm_widget_theme_object_set
           (data, sd->layout, "ctxpopup", "layout/landscape", elm_widget_style_get(data));
        else
         elm_widget_theme_object_set
           (data, sd->layout, "ctxpopup", "layout", elm_widget_style_get(data));

        elm_layout_sizing_eval(data);

        msg.val = 1;
        edje_object_message_send(sd->layout, EDJE_MESSAGE_INT, 0, &msg);
        _show_signals_emit(data, sd->dir);
     }
}

static void
_parent_detach(Evas_Object *obj)
{
   ELM_CTXPOPUP_DATA_GET(obj, sd);

   if (!sd->parent) return;

   evas_object_event_callback_del_full
     (sd->parent, EVAS_CALLBACK_DEL, _on_parent_del, obj);
   evas_object_event_callback_del_full
     (sd->parent, EVAS_CALLBACK_MOVE, _on_parent_move, obj);
   evas_object_event_callback_del_full
     (sd->parent, EVAS_CALLBACK_RESIZE, _on_parent_resize, obj);
}

static void
_on_content_resized(void *data,
                    Evas *e __UNUSED__,
                    Evas_Object *obj __UNUSED__,
                    void *event_info __UNUSED__)
{
   elm_layout_sizing_eval(data);
}

static void
_access_outline_activate_cb(void *data,
                        Evas_Object *part_obj __UNUSED__,
                        Elm_Object_Item *item __UNUSED__)
{
   evas_object_hide(data);
   evas_object_smart_callback_call(data, SIG_DISMISSED, NULL);
}

static void
_access_obj_process(Evas_Object *obj, Eina_Bool is_access)
{
   Evas_Object *ao;
   ELM_CTXPOPUP_DATA_GET(obj, sd);

   if (is_access)
     {
        ao = _access_object_get(obj, ACCESS_OUTLINE_PART);
        if (!ao)
          {
             ao = _elm_access_edje_object_part_object_register
                (obj, ELM_WIDGET_DATA(sd)->resize_obj, ACCESS_OUTLINE_PART);

             const char *style = elm_widget_style_get(obj);
             if (!strcmp(style, "more/default"))
               {
                  elm_access_info_set(ao, ELM_ACCESS_TYPE, TTS_STR_MENU_POPUP);
                  elm_access_info_set(ao, ELM_ACCESS_CONTEXT_INFO, TTS_STR_MENU_CLOSE);
               }
             else
               {
                  elm_access_info_set(ao, ELM_ACCESS_TYPE, E_("IDS_TPLATFORM_BODY_CONTEXTUAL_POP_UP_T_TTS"));
                  elm_access_info_set(ao, ELM_ACCESS_CONTEXT_INFO, E_("WDS_TTS_TBBODY_DOUBLE_TAP_TO_CLOSE_THE_POP_UP"));
               }
             _elm_access_activate_callback_set
                (_elm_access_object_get(ao), _access_outline_activate_cb, obj);
          }
     }
   else
     {
        _elm_access_edje_object_part_object_unregister
               (obj, ELM_WIDGET_DATA(sd)->resize_obj, ACCESS_OUTLINE_PART);
     }
}

static void
_mirrored_set(Evas_Object *obj, Eina_Bool rtl)
{
   ELM_CTXPOPUP_DATA_GET(obj, sd);

   edje_object_mirrored_set(sd->layout, rtl);
   edje_object_mirrored_set(sd->arrow, rtl);
   edje_object_mirrored_set(ELM_WIDGET_DATA(sd)->resize_obj, rtl);
}

static Eina_Bool
_elm_ctxpopup_smart_event(Evas_Object *obj,
                          Evas_Object *src __UNUSED__,
                          Evas_Callback_Type type,
                          void *event_info)
{
   Evas_Event_Key_Down *ev = event_info;

   ELM_CTXPOPUP_DATA_GET(obj, sd);

   // TIZEN ONLY(20131221) : When access mode, focused ui is disabled.
   if (_elm_config->access_mode) return EINA_FALSE;

   if (elm_widget_disabled_get(obj)) return EINA_FALSE;
   if (type != EVAS_CALLBACK_KEY_DOWN) return EINA_FALSE;
   if (ev->event_flags & EVAS_EVENT_FLAG_ON_HOLD) return EINA_FALSE;
   if (!_focus_enabled(obj)) return EINA_FALSE;

   //FIXME: for this key event, _elm_ctxpopup_smart_focus_next should be done first
   if ((!strcmp(ev->keyname, "Tab")) ||
       (!strcmp(ev->keyname, "ISO_Left_Tab")))
     {
        ev->event_flags |= EVAS_EVENT_FLAG_ON_HOLD;
        return EINA_TRUE;
     }
   /////
   if (((!strcmp(ev->keyname, "Left")) ||
        (!strcmp(ev->keyname, "KP_Left")) ||
        (!strcmp(ev->keyname, "Right")) ||
        (!strcmp(ev->keyname, "KP_Right")) ||
        (!strcmp(ev->keyname, "Up")) ||
        (!strcmp(ev->keyname, "KP_Up")) ||
        (!strcmp(ev->keyname, "Down")) ||
        (!strcmp(ev->keyname, "KP_Down"))) && (!ev->string))
     {
        double degree = 0.0;

        if ((!strcmp(ev->keyname, "Left")) ||
            (!strcmp(ev->keyname, "KP_Left")))
          degree = 270.0;
        else if ((!strcmp(ev->keyname, "Right")) ||
                 (!strcmp(ev->keyname, "KP_Right")))
          degree = 90.0;
        else if ((!strcmp(ev->keyname, "Up")) ||
                 (!strcmp(ev->keyname, "KP_Up")))
          degree = 0.0;
        else if ((!strcmp(ev->keyname, "Down")) ||
                 (!strcmp(ev->keyname, "KP_Down")))
          degree = 180.0;

        elm_widget_focus_direction_go(sd->box, degree);
        ev->event_flags |= EVAS_EVENT_FLAG_ON_HOLD;
        return EINA_TRUE;
     }
   else if (((!strcmp(ev->keyname, "Home")) ||
             (!strcmp(ev->keyname, "KP_Home")) ||
             (!strcmp(ev->keyname, "Prior")) ||
             (!strcmp(ev->keyname, "KP_Prior"))) && (!ev->string))
     {
        Elm_Ctxpopup_Item *it = eina_list_data_get(sd->items);
        Evas_Object *btn = it->btn;
        elm_object_focus_set(btn, EINA_TRUE);
        ev->event_flags |= EVAS_EVENT_FLAG_ON_HOLD;
        return EINA_TRUE;
     }
   else if (((!strcmp(ev->keyname, "End")) ||
             (!strcmp(ev->keyname, "KP_End")) ||
             (!strcmp(ev->keyname, "Next")) ||
             (!strcmp(ev->keyname, "KP_Next"))) && (!ev->string))
     {
        Elm_Ctxpopup_Item *it = eina_list_data_get(eina_list_last(sd->items));
        Evas_Object *btn = it->btn;
        elm_object_focus_set(btn, EINA_TRUE);
        ev->event_flags |= EVAS_EVENT_FLAG_ON_HOLD;
        return EINA_TRUE;
     }

   // TIZEN ONLY : 20130530 : ctxpopup will be dismissed by user
   //if (strcmp(ev->keyname, "Escape")) return EINA_FALSE;
   return EINA_FALSE;

/*
   _hide_signals_emit(obj, sd->dir);

   ev->event_flags |= EVAS_EVENT_FLAG_ON_HOLD;
   return EINA_TRUE;
*/
}

//FIXME: lost the content size when theme hook is called.
static Eina_Bool
_elm_ctxpopup_smart_theme(Evas_Object *obj)
{
   Eina_List *elist;
   Elm_Ctxpopup_Item *item;
   int idx = 0;
   Eina_Bool rtl;

   ELM_CTXPOPUP_DATA_GET(obj, sd);
   ELM_WIDGET_DATA_GET(obj, wsd);

   if (!ELM_WIDGET_CLASS(_elm_ctxpopup_parent_sc)->theme(obj))
     return EINA_FALSE;

   rtl = elm_widget_mirrored_get(obj);

   elm_widget_theme_object_set
     (obj, sd->bg, "ctxpopup", "bg", elm_widget_style_get(obj));

   elm_widget_theme_object_set
     (obj, sd->arrow, "ctxpopup", "arrow", elm_widget_style_get(obj));

   if (wsd->orient_mode == 90 || wsd->orient_mode == 270)
     elm_widget_theme_object_set
       (obj, sd->layout, "ctxpopup", "layout/landscape", elm_widget_style_get(obj));
   else
     elm_widget_theme_object_set
       (obj, sd->layout, "ctxpopup", "layout", elm_widget_style_get(obj));

   _mirrored_set(obj, rtl);

   //Items
   EINA_LIST_FOREACH(sd->items, elist, item)
     {
        edje_object_mirrored_set(VIEW(item), rtl);

        if (item->label && item->icon)
          _elm_theme_object_set(obj, VIEW(item), "ctxpopup",
                                "icon_text_style_item",
                                elm_widget_style_get(obj));
        else if (item->label)
          {
             if(!sd->horizontal)
               _elm_theme_object_set(obj, VIEW(item), "ctxpopup",
                                     "text_style_item",
                                     elm_widget_style_get(obj));
             else
               _elm_theme_object_set(obj, VIEW(item), "ctxpopup",
                                     "text_style_item_horizontal",
                                     elm_widget_style_get(obj));
          }
        else if (item->icon)
          _elm_theme_object_set
             (obj, VIEW(item), "ctxpopup", "icon_style_item",
              elm_widget_style_get(obj));
        if (item->label)
          edje_object_part_text_escaped_set(VIEW(item), "elm.text", item->label);

        if (elm_widget_item_disabled_get(item))
          edje_object_signal_emit(VIEW(item), "elm,state,disabled", "elm");

       /*
        *  For separator, if the first item has visible separator,
        *  then it should be aligned with edge of the base part.
        *  In some cases, it gives improper display. Ex) rounded corner
        *  So the first item separator should be invisible.
        */
        if ((idx++) == 0)
          edje_object_signal_emit(VIEW(item), "elm,state,default", "elm");
        else
          edje_object_signal_emit(VIEW(item), "elm,state,separator", "elm");

        // reset state of text to be default
        edje_object_signal_emit(VIEW(item), "elm,state,text,default", "elm");
        edje_object_message_signal_process(VIEW(item));
     }

   if (evas_object_visible_get(sd->bg))
     edje_object_signal_emit(sd->bg, "elm,state,show", "elm");

   if (sd->scr)
     {
        elm_object_style_set(sd->scr, "effect");

        if (sd->horizontal)
          elm_scroller_policy_set(sd->scr, ELM_SCROLLER_POLICY_AUTO, ELM_SCROLLER_POLICY_OFF);
        else
          elm_scroller_policy_set(sd->scr, ELM_SCROLLER_POLICY_OFF, ELM_SCROLLER_POLICY_AUTO);
     }

   sd->dir = ELM_CTXPOPUP_DIRECTION_UNKNOWN;

   _elm_ctxpopup_smart_sizing_eval(obj);

   /* access */
  if (_elm_config->access_mode) _access_obj_process(obj, EINA_TRUE);

   return EINA_TRUE;
}

/* kind of a big and tricky override here: an internal box will hold
 * the actual content. content aliases won't be of much help here */
static Eina_Bool
_elm_ctxpopup_smart_content_set(Evas_Object *obj,
                                const char *part,
                                Evas_Object *content)
{
   Evas_Coord min_w = -1, min_h = -1;

   ELM_CTXPOPUP_DATA_GET(obj, sd);

   if ((part) && (strcmp(part, "default")))
     return ELM_CONTAINER_CLASS(_elm_ctxpopup_parent_sc)->content_set
              (obj, part, content);

   if (!content) return EINA_FALSE;

   if (content == sd->content) return EINA_TRUE;

   if (sd->items) elm_ctxpopup_clear(obj);
   if (sd->content) evas_object_del(sd->content);

   evas_object_event_callback_add
      (sd->box, EVAS_CALLBACK_RESIZE, _on_content_resized, obj);
   edje_object_part_swallow(ELM_WIDGET_DATA(sd)->resize_obj, "elm.swallow.content", sd->box);

   evas_object_size_hint_weight_set
     (content, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_fill_set
     (content, EVAS_HINT_FILL, EVAS_HINT_FILL);

   /* since it's going to be a box content, not a layout's... */
   evas_object_show(content);

   evas_object_size_hint_min_get(content, &min_w, &min_h);
   evas_object_size_hint_min_set(sd->box, min_w, min_h);
   elm_box_pack_end(sd->box, content);

   sd->content = content;
   sd->dir = ELM_CTXPOPUP_DIRECTION_UNKNOWN;

   elm_layout_sizing_eval(obj);

   return EINA_TRUE;
}

static Evas_Object *
_elm_ctxpopup_smart_content_get(const Evas_Object *obj,
                                const char *part)
{
   ELM_CTXPOPUP_DATA_GET(obj, sd);

   if ((part) && (strcmp(part, "default")))
     return ELM_CONTAINER_CLASS(_elm_ctxpopup_parent_sc)->content_get
              (obj, part);

   return sd->content;
}

static Evas_Object *
_elm_ctxpopup_smart_content_unset(Evas_Object *obj,
                                  const char *part)
{
   Evas_Object *content;

   ELM_CTXPOPUP_DATA_GET(obj, sd);

   if ((part) && (strcmp(part, "default")))
     return ELM_CONTAINER_CLASS(_elm_ctxpopup_parent_sc)->content_unset
              (obj, part);

   content = sd->content;
   if (!content) return NULL;

   elm_box_unpack(sd->box, sd->content);

   sd->content = NULL;
   sd->dir = ELM_CTXPOPUP_DIRECTION_UNKNOWN;

   elm_layout_sizing_eval(obj);

   return content;
}

static void
_item_text_set_hook(Elm_Object_Item *it,
                    const char *part,
                    const char *label)
{
   Elm_Ctxpopup_Item *ctxpopup_it;

   if ((part) && (strcmp(part, "default"))) return;

   ctxpopup_it = (Elm_Ctxpopup_Item *)it;

   _item_label_set(ctxpopup_it, label);
}

static const char *
_item_text_get_hook(const Elm_Object_Item *it,
                    const char *part)
{
   Elm_Ctxpopup_Item *ctxpopup_it;

   if (part && strcmp(part, "default")) return NULL;

   ctxpopup_it = (Elm_Ctxpopup_Item *)it;
   return ctxpopup_it->label;
}

static void
_item_content_set_hook(Elm_Object_Item *it,
                       const char *part,
                       Evas_Object *content)
{
   Elm_Ctxpopup_Item *ctxpopup_it;

   if ((part) && (strcmp(part, "icon"))) return;

   ctxpopup_it = (Elm_Ctxpopup_Item *)it;

   ELM_CTXPOPUP_DATA_GET(WIDGET(ctxpopup_it), sd);

   _item_icon_set(ctxpopup_it, content);
   sd->dir = ELM_CTXPOPUP_DIRECTION_UNKNOWN;

   _elm_ctxpopup_smart_sizing_eval(WIDGET(ctxpopup_it));
}

static Evas_Object *
_item_content_get_hook(const Elm_Object_Item *it,
                       const char *part)
{
   Elm_Ctxpopup_Item *ctxpopup_it;

   if (part && strcmp(part, "icon")) return NULL;

   ctxpopup_it = (Elm_Ctxpopup_Item *)it;
   return ctxpopup_it->icon;
}

static void
_item_disable_hook(Elm_Object_Item *it)
{
   Elm_Ctxpopup_Item *ctxpopup_it = (Elm_Ctxpopup_Item *)it;

   ELM_CTXPOPUP_DATA_GET(WIDGET(it), sd);
   if (!sd) return;

   if (elm_widget_item_disabled_get(it))
     edje_object_signal_emit(VIEW(ctxpopup_it), "elm,state,disabled", "elm");
   else
     edje_object_signal_emit(VIEW(ctxpopup_it), "elm,state,enabled", "elm");
}

static void
_item_signal_emit_hook(Elm_Object_Item *it,
                       const char *emission,
                       const char *source)
{
   Elm_Ctxpopup_Item *ctxpopup_it = (Elm_Ctxpopup_Item *)it;

   edje_object_signal_emit(VIEW(ctxpopup_it), emission, source);
}

static void
_item_style_set_hook(Elm_Object_Item *it,
                     const char *style)
{
   Elm_Ctxpopup_Item *item = (Elm_Ctxpopup_Item *)it;
   ELM_CTXPOPUP_DATA_GET(WIDGET(item), sd);

   if (item->icon && item->label)
      _elm_theme_object_set(WIDGET(item), VIEW(item), "ctxpopup", "icon_text_style_item", style);
   else if (item->label)
     {
        if (sd->horizontal)
          _elm_theme_object_set(WIDGET(item), VIEW(item), "ctxpopup", "text_style_item_horizontal", style);
        else
          _elm_theme_object_set(WIDGET(item), VIEW(item), "ctxpopup", "text_style_item", style);
     }
   else
     _elm_theme_object_set(WIDGET(item), VIEW(item), "ctxpopup", "icon_style_item", style);

   elm_layout_sizing_eval(WIDGET(item));
}

static void
_bg_clicked_cb(void *data,
               Evas_Object *obj __UNUSED__,
               const char *emission __UNUSED__,
               const char *source __UNUSED__)
{
   ELM_CTXPOPUP_DATA_GET(data, sd);

   _hide_signals_emit(data, sd->dir);
}

static void
_on_show(void *data __UNUSED__,
         Evas *e __UNUSED__,
         Evas_Object *obj,
         void *event_info __UNUSED__)
{
   Eina_List *elist;
   Elm_Ctxpopup_Item *item;
   int idx = 0;

   ELM_CTXPOPUP_DATA_GET(obj, sd);
   ELM_WIDGET_DATA_GET(obj, wsd);

   if ((!sd->items) && (!sd->content)) return;

   sd->visible = EINA_TRUE;

   elm_layout_signal_emit(obj, "elm,state,show", "elm");

   if (wsd->orient_mode == 90 || wsd->orient_mode == 270)
     elm_widget_theme_object_set
       (obj, sd->layout, "ctxpopup", "layout/landscape", elm_widget_style_get(obj));
   else
     elm_widget_theme_object_set
       (obj, sd->layout, "ctxpopup", "layout", elm_widget_style_get(obj));

   EINA_LIST_FOREACH(sd->items, elist, item)
     {
        if (item->label && !item->icon)
          {
             if(!sd->horizontal)
               _elm_theme_object_set(obj, VIEW(item), "ctxpopup",
                                     "text_style_item",
                                     elm_widget_style_get(obj));
             else
               _elm_theme_object_set(obj, VIEW(item), "ctxpopup",
                                     "text_style_item_horizontal",
                                     elm_widget_style_get(obj));
          }

        if (idx++ == 0)
          edje_object_signal_emit(VIEW(item), "elm,state,default", "elm");
        else
          edje_object_signal_emit(VIEW(item), "elm,state,separator", "elm");
     }

   elm_layout_sizing_eval(obj);

#ifndef ELM_FEATURE_WEARABLE
   const char *color = NULL;
   int i = 0;
   Elm_Ctxpopup_Item *it;

   EINA_LIST_FOREACH (sd->items, elist, it)
     {
        if (i < MAX_ITEMS_PER_VIEWPORT)
          {
             color = edje_object_data_get(VIEW(it), "bg_color");
             edje_object_color_class_set(VIEW(it), color,
                                        sd->item_color[i][0], sd->item_color[i][1], sd->item_color[i][2], 255,
                                        255, 255, 255, 255, 255, 255, 255, 255);
             i++;
          }
     }
#endif

   elm_object_focus_set(obj, EINA_TRUE);
   _show_signals_emit(obj, sd->dir);
}

static void
_on_hide(void *data __UNUSED__,
         Evas *e __UNUSED__,
         Evas_Object *obj,
         void *event_info __UNUSED__)
{
   ELM_CTXPOPUP_DATA_GET(obj, sd);

   if (!sd->visible) return;

   sd->visible = EINA_FALSE;
}

static void
_on_move(void *data __UNUSED__,
         Evas *e __UNUSED__,
         Evas_Object *obj,
         void *event_info __UNUSED__)
{
   ELM_CTXPOPUP_DATA_GET(obj, sd);

   _elm_ctxpopup_smart_sizing_eval(obj);
}

static void
_hide_finished_cb(void *data,
                  Evas_Object *obj __UNUSED__,
                  const char *emission __UNUSED__,
                  const char *source __UNUSED__)
{
   evas_object_hide(data);
   evas_object_smart_callback_call(data, SIG_DISMISSED, NULL);
}

static void
_list_del(Elm_Ctxpopup_Smart_Data *sd)
{
   if (!sd->scr) return;

   edje_object_part_unswallow(ELM_WIDGET_DATA(sd)->resize_obj, sd->scr);
   evas_object_del(sd->scr);
   sd->scr = NULL;
   evas_object_del(sd->box);
   sd->box = NULL;
}

static void
scroll_up_down_cb(void *data, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
#ifndef ELM_FEATURE_WEARABLE
   ELM_CTXPOPUP_DATA_GET(data, sd);
   if (!sd) return;
   Evas_Coord x, y, w, h;
   Evas_Coord x2, y2, w2, h2;
   const char *color = NULL;
   int i = 0;
   Eina_List *elist;
   Elm_Ctxpopup_Item *item , *top_drawn_item = NULL;

   evas_object_geometry_get(ELM_WIDGET_DATA(sd)->resize_obj, &x2, &y2, &w2, &h2);
   EINA_LIST_FOREACH(sd->items, elist, item) {
     evas_object_geometry_get(VIEW(item), &x, &y, &w, &h);
     if ( y <= y2 && ELM_RECTS_INTERSECT(x,y,w,h,x2,y2,w2,h2))
       top_drawn_item = item;
     if( top_drawn_item && ( top_drawn_item != sd->top_drawn_item  )) {
       if( i < MAX_ITEMS_PER_VIEWPORT ) {
          color = edje_object_data_get(VIEW(item), "bg_color");
          edje_object_color_class_set(VIEW(item), color, sd->item_color[i][0], sd->item_color[i][1], sd->item_color[i][2], 255, 255, 255, 255, 255, 255, 255, 255, 255);
          i++;
       }
     }
   }
   sd->top_drawn_item = top_drawn_item;
#endif
}

static void
_list_new(Evas_Object *obj)
{
   ELM_CTXPOPUP_DATA_GET(obj, sd);
   if (!sd) return;

   //scroller
   sd->scr = elm_scroller_add(obj);
   elm_object_style_set(sd->scr, "effect");
   evas_object_size_hint_align_set(sd->scr, EVAS_HINT_FILL, EVAS_HINT_FILL);
#ifndef ELM_FEATURE_WEARABLE
   evas_object_smart_callback_add(sd->scr, "scroll,up", scroll_up_down_cb, obj);
   evas_object_smart_callback_add(sd->scr, "scroll,down", scroll_up_down_cb, obj);
#endif
   if (sd->horizontal)
     elm_scroller_policy_set(sd->scr, ELM_SCROLLER_POLICY_AUTO, ELM_SCROLLER_POLICY_OFF);
   else
     elm_scroller_policy_set(sd->scr, ELM_SCROLLER_POLICY_OFF, ELM_SCROLLER_POLICY_AUTO);

   edje_object_part_swallow(ELM_WIDGET_DATA(sd)->resize_obj, "elm.swallow.content", sd->scr);

   elm_object_content_set(sd->scr, sd->box);
   elm_ctxpopup_horizontal_set(obj, sd->horizontal);
}

static Eina_Bool
_elm_ctxpopup_smart_disable(Evas_Object *obj)
{
   Eina_List *l;
   Elm_Object_Item *it;

   ELM_CTXPOPUP_DATA_GET(obj, sd);

   if (!ELM_WIDGET_CLASS(_elm_ctxpopup_parent_sc)->disable(obj))
     return EINA_FALSE;

   EINA_LIST_FOREACH(sd->items, l, it)
     elm_object_item_disabled_set(it, elm_widget_disabled_get(obj));

   return EINA_TRUE;
}

static void
_elm_ctxpopup_smart_add(Evas_Object *obj)
{
   EVAS_SMART_DATA_ALLOC(obj, Elm_Ctxpopup_Smart_Data);

   ELM_WIDGET_CLASS(_elm_ctxpopup_parent_sc)->base.add(obj);

}

static void
_elm_ctxpopup_smart_del(Evas_Object *obj)
{
   ELM_CTXPOPUP_DATA_GET(obj, sd);

   evas_object_event_callback_del_full
     (sd->box, EVAS_CALLBACK_RESIZE, _on_content_resized, obj);
   _parent_detach(obj);

   if (sd->items)
     {
        _items_remove(sd);
        _list_del(sd);
     }
   else
     {
        evas_object_del(sd->box);
        sd->box = NULL;
     }

   evas_object_del(sd->arrow);
   sd->arrow = NULL; /* stops _sizing_eval() from going on on deletion */

   evas_object_del(sd->bg);
   sd->bg = NULL;

   evas_object_del(sd->layout);
   sd->layout = NULL;

   ELM_WIDGET_CLASS(_elm_ctxpopup_parent_sc)->base.del(obj);
}

static void
_elm_ctxpopup_smart_parent_set(Evas_Object *obj,
                               Evas_Object *parent)
{
   //default parent is to be hover parent
   elm_ctxpopup_hover_parent_set(obj, parent);
}

static void
_elm_ctxpopup_smart_access(Evas_Object *obj, Eina_Bool is_access)
{
   ELM_CTXPOPUP_CHECK(obj);

   _access_obj_process(obj, is_access);

   evas_object_smart_callback_call(obj, SIG_ACCESS_CHANGED, NULL);
}

static Evas_Object *
_elm_ctxpopup_smart_access_object_get(Evas_Object *obj, char *part)
{
   ELM_CTXPOPUP_CHECK(obj) NULL;

   return _access_object_get(obj, part);
}

static void
_elm_ctxpopup_smart_set_user(Elm_Ctxpopup_Smart_Class *sc)
{
   ELM_WIDGET_CLASS(sc)->base.add = _elm_ctxpopup_smart_add;
   ELM_WIDGET_CLASS(sc)->base.del = _elm_ctxpopup_smart_del;

   ELM_WIDGET_CLASS(sc)->parent_set = _elm_ctxpopup_smart_parent_set;
   ELM_WIDGET_CLASS(sc)->disable = _elm_ctxpopup_smart_disable;
   ELM_WIDGET_CLASS(sc)->event = _elm_ctxpopup_smart_event;
   ELM_WIDGET_CLASS(sc)->theme = _elm_ctxpopup_smart_theme;
   ELM_WIDGET_CLASS(sc)->translate = _elm_ctxpopup_smart_translate;
   ELM_WIDGET_CLASS(sc)->sub_object_add = _elm_ctxpopup_smart_sub_object_add;
   ELM_WIDGET_CLASS(sc)->focus_next = _elm_ctxpopup_smart_focus_next;
   ELM_WIDGET_CLASS(sc)->focus_direction_manager_is =
      _elm_ctxpopup_smart_focus_direction_manager_is;
   ELM_WIDGET_CLASS(sc)->focus_direction = _elm_ctxpopup_smart_focus_direction;

   ELM_CONTAINER_CLASS(sc)->content_get = _elm_ctxpopup_smart_content_get;
   ELM_CONTAINER_CLASS(sc)->content_set = _elm_ctxpopup_smart_content_set;
   ELM_CONTAINER_CLASS(sc)->content_unset = _elm_ctxpopup_smart_content_unset;

   ELM_LAYOUT_CLASS(sc)->sizing_eval = _elm_ctxpopup_smart_sizing_eval;

   ELM_WIDGET_CLASS(sc)->access = _elm_ctxpopup_smart_access;
   ELM_WIDGET_CLASS(sc)->access_object_get = _elm_ctxpopup_smart_access_object_get;
}

EAPI const Elm_Ctxpopup_Smart_Class *
elm_ctxpopup_smart_class_get(void)
{
   static Elm_Ctxpopup_Smart_Class _sc =
     ELM_CTXPOPUP_SMART_CLASS_INIT_NAME_VERSION(ELM_CTXPOPUP_SMART_NAME);
   static const Elm_Ctxpopup_Smart_Class *class = NULL;
   Evas_Smart_Class *esc = (Evas_Smart_Class *)&_sc;

   if (class)
     return class;

   _elm_ctxpopup_smart_set(&_sc);
   esc->callbacks = _smart_callbacks;
   class = &_sc;

   return class;
}

EAPI Evas_Object *
elm_ctxpopup_add(Evas_Object *parent)
{
   Evas_Object *obj;

   EINA_SAFETY_ON_NULL_RETURN_VAL(parent, NULL);

   obj = elm_widget_add(_elm_ctxpopup_smart_class_new(), parent);
   if (!obj) return NULL;

   if (!elm_widget_sub_object_add(parent, obj))
     ERR("could not add %p as sub object of %p", obj, parent);

   elm_layout_theme_set(obj, "ctxpopup", "base", elm_widget_style_get(obj));
   elm_layout_signal_callback_add
     (obj, "elm,action,hide,finished", "", _hide_finished_cb, obj);

   ELM_CTXPOPUP_DATA_GET(obj, sd);

   //Background
   sd->bg = edje_object_add(evas_object_evas_get(obj));
   elm_widget_theme_object_set(obj, sd->bg, "ctxpopup", "bg", "default");
   edje_object_signal_callback_add
     (sd->bg, "elm,action,click", "", _bg_clicked_cb, obj);
   evas_object_smart_member_add(sd->bg, obj);
   evas_object_stack_below(sd->bg, ELM_WIDGET_DATA(sd)->resize_obj);
#ifndef ELM_FEATURE_WEARABLE
   sd->color_calculated = EINA_FALSE;
   sd->top_drawn_item = NULL;
#endif
   //Arrow
   sd->arrow = edje_object_add(evas_object_evas_get(obj));
   elm_widget_theme_object_set
     (obj, sd->arrow, "ctxpopup", "arrow", "default");

   sd->dir_priority[0] = ELM_CTXPOPUP_DIRECTION_UP;
   sd->dir_priority[1] = ELM_CTXPOPUP_DIRECTION_LEFT;
   sd->dir_priority[2] = ELM_CTXPOPUP_DIRECTION_RIGHT;
   sd->dir_priority[3] = ELM_CTXPOPUP_DIRECTION_DOWN;
   sd->dir = ELM_CTXPOPUP_DIRECTION_UNKNOWN;

   sd->auto_hide = EINA_TRUE;
   sd->mouse_down = EINA_FALSE;
   sd->multi_down = 0;

   sd->box = elm_box_add(obj);
   evas_object_size_hint_weight_set
     (sd->box, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);

   ELM_WIDGET_DATA_GET(obj, wsd);

   sd->layout = edje_object_add(evas_object_evas_get(obj));
   if (wsd->orient_mode == 90 || wsd->orient_mode == 270)
     elm_widget_theme_object_set(obj, sd->layout, "ctxpopup", "layout/landscape", "default");
   else
     elm_widget_theme_object_set(obj, sd->layout, "ctxpopup", "layout", "default");
   evas_object_smart_member_add(sd->layout, obj);

   edje_object_signal_callback_add
     (sd->layout, "elm,action,hide,finished", "", _hide_finished_cb, obj);
   edje_object_part_swallow(sd->layout, "swallow", ELM_WIDGET_DATA(sd)->resize_obj);
   evas_object_size_hint_weight_set
     (sd->layout, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);

   evas_object_event_callback_add(obj, EVAS_CALLBACK_SHOW, _on_show, NULL);
   evas_object_event_callback_add(obj, EVAS_CALLBACK_HIDE, _on_hide, NULL);
   evas_object_event_callback_add(obj, EVAS_CALLBACK_MOVE, _on_move, NULL);

   _mirrored_set(obj, elm_widget_mirrored_get(obj));

   elm_widget_can_focus_set(obj, EINA_TRUE);
   elm_ctxpopup_hover_parent_set(obj, parent);
   /* access */
   if (_elm_config->access_mode) _access_obj_process(obj, EINA_TRUE);

   /* access: parent could be any object such as elm_list which does
      not know elc_ctxpopup as its child object in the focus_next(); */

   wsd->highlight_root = EINA_TRUE;

   //Tizen Only: This should be removed when eo is applied.
   wsd->on_create = EINA_FALSE;

   return obj;
}

EAPI void
elm_ctxpopup_hover_parent_set(Evas_Object *obj,
                              Evas_Object *parent)
{
   Evas_Coord x, y, w, h;

   ELM_CTXPOPUP_CHECK(obj);
   ELM_CTXPOPUP_DATA_GET(obj, sd);

   if (!parent) return;

   _parent_detach(obj);

   evas_object_event_callback_add
     (parent, EVAS_CALLBACK_DEL, _on_parent_del, obj);
   evas_object_event_callback_add
     (parent, EVAS_CALLBACK_MOVE, _on_parent_move, obj);
   evas_object_event_callback_add
     (parent, EVAS_CALLBACK_RESIZE, _on_parent_resize, obj);

   sd->parent = parent;

   //Update Background
   evas_object_geometry_get(parent, &x, &y, &w, &h);
   evas_object_move(sd->bg, x, y);
   evas_object_resize(sd->bg, w, h);

   elm_layout_sizing_eval(obj);
}

EAPI Evas_Object *
elm_ctxpopup_hover_parent_get(const Evas_Object *obj)
{
   ELM_CTXPOPUP_CHECK(obj) NULL;
   ELM_CTXPOPUP_DATA_GET(obj, sd);

   return sd->parent;
}

EAPI void
elm_ctxpopup_clear(Evas_Object *obj)
{
   ELM_CTXPOPUP_CHECK(obj);
   ELM_CTXPOPUP_DATA_GET(obj, sd);

   _items_remove(sd);

   elm_object_content_unset(sd->scr);
   edje_object_part_unswallow(ELM_WIDGET_DATA(sd)->resize_obj, sd->scr);
   evas_object_del(sd->scr);
   sd->scr = NULL;
   sd->dir = ELM_CTXPOPUP_DIRECTION_UNKNOWN;
}

EAPI void
elm_ctxpopup_horizontal_set(Evas_Object *obj,
                            Eina_Bool horizontal)
{
   Eina_List *elist;
   Elm_Ctxpopup_Item *item;
   int idx = 0;

   ELM_CTXPOPUP_CHECK(obj);
   ELM_CTXPOPUP_DATA_GET(obj, sd);

   sd->horizontal = !!horizontal;

   if (!sd->scr)
      return;

  if (!horizontal)
     {
        elm_box_horizontal_set(sd->box, EINA_FALSE);
        elm_scroller_bounce_set(sd->scr, EINA_FALSE, EINA_TRUE);
        elm_scroller_policy_set(sd->scr, ELM_SCROLLER_POLICY_OFF, ELM_SCROLLER_POLICY_AUTO);
     }
   else
     {
        elm_box_horizontal_set(sd->box, EINA_TRUE);
        elm_scroller_bounce_set(sd->scr, EINA_TRUE, EINA_FALSE);
        elm_scroller_policy_set(sd->scr, ELM_SCROLLER_POLICY_AUTO, ELM_SCROLLER_POLICY_OFF);
     }

   EINA_LIST_FOREACH(sd->items, elist, item)
     {
        if (item->label && !item->icon && !horizontal)
          _elm_theme_object_set(obj, VIEW(item), "ctxpopup", "text_style_item",
                                elm_widget_style_get(obj));
        else if (item->label && !item->icon && horizontal)
          _elm_theme_object_set(obj, VIEW(item), "ctxpopup",
                                "text_style_item_horizontal",
                                elm_widget_style_get(obj));

        if (idx++ == 0)
          edje_object_signal_emit(VIEW(item), "elm,state,default", "elm");
        else
          edje_object_signal_emit(VIEW(item), "elm,state,separator", "elm");

        _item_disable_hook((Elm_Object_Item *)item);
     }

   sd->dir = ELM_CTXPOPUP_DIRECTION_UNKNOWN;

   elm_layout_sizing_eval(obj);
}

EAPI Eina_Bool
elm_ctxpopup_horizontal_get(const Evas_Object *obj)
{
   ELM_CTXPOPUP_CHECK(obj) EINA_FALSE;
   ELM_CTXPOPUP_DATA_GET(obj, sd);

   return sd->horizontal;
}

EAPI Elm_Object_Item *
elm_ctxpopup_item_append(Evas_Object *obj,
                         const char *label,
                         Evas_Object *icon,
                         Evas_Smart_Cb func,
                         const void *data)
{
   Elm_Ctxpopup_Item *item, *it;
   Evas_Object *content, *focus_bt;
   int idx = 0;
   Eina_List *elist;

   ELM_CTXPOPUP_CHECK(obj) NULL;
   ELM_CTXPOPUP_DATA_GET(obj, sd);

   item = elm_widget_item_new(obj, Elm_Ctxpopup_Item);
   if (!item) return NULL;

   elm_widget_item_del_pre_hook_set(item, _item_del_pre_hook);
   elm_widget_item_disable_hook_set(item, _item_disable_hook);
   elm_widget_item_text_set_hook_set(item, _item_text_set_hook);
   elm_widget_item_text_get_hook_set(item, _item_text_get_hook);
   elm_widget_item_content_set_hook_set(item, _item_content_set_hook);
   elm_widget_item_content_get_hook_set(item, _item_content_get_hook);
   elm_widget_item_signal_emit_hook_set(item, _item_signal_emit_hook);
   elm_widget_item_style_set_hook_set(item, _item_style_set_hook);

   //The first item is appended.
   content = elm_object_content_unset(obj);
   if (content) evas_object_del(content);

   if (!sd->items)
     _list_new(obj);

   item->func = func;
   item->base.data = data;

   if (icon && label)
     _item_new(item, "icon_text_style_item");
   else if (label)
     {
        if (!sd->horizontal)
          _item_new(item, "text_style_item");
        else
          _item_new(item, "text_style_item_horizontal");
     }
   else
     _item_new(item, "icon_style_item");

   _item_icon_set(item, icon);
   _item_label_set(item, label);
   focus_bt = _item_in_focusable_button(item);
   elm_box_pack_end(sd->box, focus_bt);
   sd->items = eina_list_append(sd->items, item);
   item->btn = focus_bt;

   sd->dir = ELM_CTXPOPUP_DIRECTION_UNKNOWN;
   if (sd->visible)
     {
        EINA_LIST_FOREACH(sd->items, elist, it)
          {
             if (idx++ == 0)
               edje_object_signal_emit(VIEW(it), "elm,state,default", "elm");
             else
               edje_object_signal_emit(VIEW(it), "elm,state,separator", "elm");
          }
        _elm_ctxpopup_smart_sizing_eval(obj);
     }

   if (_elm_config->access_mode) _access_focusable_button_register(focus_bt, item);

   return (Elm_Object_Item *)item;
}

EAPI void
elm_ctxpopup_direction_priority_set(Evas_Object *obj,
                                    Elm_Ctxpopup_Direction first,
                                    Elm_Ctxpopup_Direction second,
                                    Elm_Ctxpopup_Direction third,
                                    Elm_Ctxpopup_Direction fourth)
{
   ELM_CTXPOPUP_CHECK(obj);
   ELM_CTXPOPUP_DATA_GET(obj, sd);

   sd->dir_priority[0] = first;
   sd->dir_priority[1] = second;
   sd->dir_priority[2] = third;
   sd->dir_priority[3] = fourth;

   elm_layout_sizing_eval(obj);
}

EAPI void
elm_ctxpopup_direction_priority_get(Evas_Object *obj,
                                    Elm_Ctxpopup_Direction *first,
                                    Elm_Ctxpopup_Direction *second,
                                    Elm_Ctxpopup_Direction *third,
                                    Elm_Ctxpopup_Direction *fourth)
{
   ELM_CTXPOPUP_CHECK(obj);
   ELM_CTXPOPUP_DATA_GET(obj, sd);

   if (first) *first = sd->dir_priority[0];
   if (second) *second = sd->dir_priority[1];
   if (third) *third = sd->dir_priority[2];
   if (fourth) *fourth = sd->dir_priority[3];
}

EAPI Elm_Ctxpopup_Direction
elm_ctxpopup_direction_get(const Evas_Object *obj)
{
   ELM_CTXPOPUP_CHECK(obj) ELM_CTXPOPUP_DIRECTION_UNKNOWN;
   ELM_CTXPOPUP_DATA_GET(obj, sd);

   return sd->dir;
}

EAPI Eina_Bool
elm_ctxpopup_direction_available_get(Evas_Object *obj, Elm_Ctxpopup_Direction direction)
{
   ELM_CTXPOPUP_CHECK(obj) EINA_FALSE;
   ELM_CTXPOPUP_DATA_GET(obj, sd);

   // TEMPORARY
   sd->dir_requested = EINA_TRUE;
   elm_layout_sizing_eval(obj);
   sd->dir_requested = EINA_FALSE;

   if (sd->dir == direction) return EINA_TRUE;
   return EINA_FALSE;
}

EAPI void
elm_ctxpopup_dismiss(Evas_Object *obj)
{
   ELM_CTXPOPUP_CHECK(obj);
   ELM_CTXPOPUP_DATA_GET(obj, sd);

   _hide_signals_emit(obj, sd->dir);
}

EAPI void
elm_ctxpopup_auto_hide_disabled_set(Evas_Object *obj, Eina_Bool disabled)
{
   ELM_CTXPOPUP_CHECK(obj);
   ELM_CTXPOPUP_DATA_GET(obj, sd);

   disabled = !!disabled;

   if (sd->auto_hide == !disabled) return;
   sd->auto_hide = !disabled;
}
