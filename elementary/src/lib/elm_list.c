#include <Elementary.h>
#include "elm_priv.h"
#include "elm_widget_list.h"

#define ITEM_UNHIGHLIGHT_TIMER 0.01  // This is needed for highlight effect when item is unhighlighted right after item is highlighted and selected

EAPI const char ELM_LIST_SMART_NAME[] = "elm_list";

static const char SIG_ACTIVATED[] = "activated";
static const char SIG_CLICKED_DOUBLE[] = "clicked,double";
static const char SIG_SELECTED[] = "selected";
static const char SIG_UNSELECTED[] = "unselected";
static const char SIG_LONGPRESSED[] = "longpressed";
static const char SIG_SCROLL_ANIM_START[] = "scroll,anim,start";
static const char SIG_SCROLL_ANIM_STOP[] = "scroll,anim,stop";
static const char SIG_SCROLL_DRAG_START[] = "scroll,drag,start";
static const char SIG_SCROLL_DRAG_STOP[] = "scroll,drag,stop";
static const char SIG_SCROLL[] = "scroll";
static const char SIG_EDGE_TOP[] = "edge,top";
static const char SIG_EDGE_BOTTOM[] = "edge,bottom";
static const char SIG_EDGE_LEFT[] = "edge,left";
static const char SIG_EDGE_RIGHT[] = "edge,right";
static const char SIG_LANG_CHANGED[] = "language,changed";
static const char SIG_SWIPE[] = "swipe";
static const char SIG_HIGHLIGHTED[] = "highlighted";
static const char SIG_UNHIGHLIGHTED[] = "unhighlighted";
static const char SIG_ACCESS_CHANGED[] = "access,changed";
static const Evas_Smart_Cb_Description _smart_callbacks[] = {
   {SIG_ACTIVATED, ""},
   {SIG_CLICKED_DOUBLE, ""},
   {SIG_SELECTED, ""},
   {SIG_UNSELECTED, ""},
   {SIG_LONGPRESSED, ""},
   {SIG_SCROLL, ""},
   {SIG_SCROLL_ANIM_START, ""},
   {SIG_SCROLL_ANIM_STOP, ""},
   {SIG_SCROLL_DRAG_START, ""},
   {SIG_SCROLL_DRAG_STOP, ""},
   {SIG_EDGE_TOP, ""},
   {SIG_EDGE_BOTTOM, ""},
   {SIG_EDGE_LEFT, ""},
   {SIG_EDGE_RIGHT, ""},
   {SIG_LANG_CHANGED, ""},
   {SIG_SWIPE, ""},
   {SIG_HIGHLIGHTED, ""},
   {SIG_UNHIGHLIGHTED, ""},
   {SIG_ACCESS_CHANGED, ""},
   {NULL, NULL}
};

typedef enum
{
   FOCUS_DIR_UP = 0,
   FOCUS_DIR_DOWN,
   FOCUS_DIR_LEFT,
   FOCUS_DIR_RIGHT
} Focus_Dir;

static const Evas_Smart_Interface *_smart_interfaces[] =
{
   (Evas_Smart_Interface *)&ELM_SCROLLABLE_IFACE, NULL
};

static void _size_hints_changed_cb(void *, Evas *, Evas_Object *, void *);
static void _mouse_up_cb(void *, Evas *, Evas_Object *, void *);
static void _mouse_down_cb(void *, Evas *, Evas_Object *, void *);
static void _mouse_move_cb(void *, Evas *, Evas_Object *, void *);
static Eina_Bool _select_unselect(void *data);
static Eina_Bool _unhighlight_timer(void *data);
static Eina_Bool _long_press_cb(void *data);
static void _item_highlight(Elm_List_Item *it);
static void _items_fix(Evas_Object *);
static void _item_select(Elm_List_Item *it);
static void _item_unselect(Elm_List_Item *it);
static void _highlight_timer_disable(Elm_List_Item *it);

EVAS_SMART_SUBCLASS_IFACE_NEW
  (ELM_LIST_SMART_NAME, _elm_list, Elm_List_Smart_Class,
   Elm_Layout_Smart_Class, elm_layout_smart_class_get, _smart_callbacks,
   _smart_interfaces);

static inline void
_elm_list_item_free(Elm_List_Item *it)
{
   evas_object_event_callback_del_full
     (VIEW(it), EVAS_CALLBACK_MOUSE_DOWN, _mouse_down_cb, it);
   evas_object_event_callback_del_full
     (VIEW(it), EVAS_CALLBACK_MOUSE_UP, _mouse_up_cb, it);
   evas_object_event_callback_del_full
     (VIEW(it), EVAS_CALLBACK_MOUSE_MOVE, _mouse_move_cb, it);

   if (it->unhighlight_timer)
     {
        ecore_timer_del(it->unhighlight_timer);
        it->unhighlight_timer = NULL;
     }
   _highlight_timer_disable(it);
   if ((Elm_List_Item *)it->sd->focused == it) it->sd->focused = NULL;
   if ((Elm_List_Item *)it->sd->key_down == it) it->sd->key_down = NULL;

   edje_object_message_signal_process(VIEW(it));

   if (it->icon)
     evas_object_event_callback_del_full
       (it->icon, EVAS_CALLBACK_CHANGED_SIZE_HINTS,
       _size_hints_changed_cb, WIDGET(it));

   if (it->end)
     evas_object_event_callback_del_full
       (it->end, EVAS_CALLBACK_CHANGED_SIZE_HINTS,
       _size_hints_changed_cb, WIDGET(it));

   eina_stringshare_del(it->label);

   if (it->swipe_timer) ecore_timer_del(it->swipe_timer);
   it->swipe_timer = NULL;
   if (it->long_timer) ecore_timer_del(it->long_timer);
   it->long_timer = NULL;
   if (it->icon) evas_object_del(it->icon);
   if (it->end) evas_object_del(it->end);
}

static Eina_Bool
_item_multi_select_up(Elm_List_Smart_Data *sd)
{
   Elm_Object_Item *prev;

   if (!sd->selected) return EINA_FALSE;
   if (!sd->multi) return EINA_FALSE;

   prev = elm_list_item_prev(sd->last_selected_item);
   if (!prev) return EINA_TRUE;

   if (elm_list_item_selected_get(prev))
     {
        elm_list_item_selected_set(sd->last_selected_item, EINA_FALSE);
        sd->last_selected_item = prev;
        elm_list_item_show(sd->last_selected_item);
     }
   else
     {
        elm_list_item_selected_set(prev, EINA_TRUE);
        elm_list_item_show(prev);
     }
   return EINA_TRUE;
}

static Eina_Bool
_item_multi_select_down(Elm_List_Smart_Data *sd)
{
   Elm_Object_Item *next;

   if (!sd->selected) return EINA_FALSE;
   if (!sd->multi) return EINA_FALSE;

   next = elm_list_item_next(sd->last_selected_item);
   if (!next) return EINA_TRUE;

   if (elm_list_item_selected_get(next))
     {
        elm_list_item_selected_set(sd->last_selected_item, EINA_FALSE);
        sd->last_selected_item = next;
        elm_list_item_show(sd->last_selected_item);
     }
   else
     {
        elm_list_item_selected_set(next, EINA_TRUE);
        elm_list_item_show(next);
     }
   return EINA_TRUE;
}

static Eina_Bool
_all_items_unselect(Elm_List_Smart_Data *sd)
{
   if (!sd->selected) return EINA_FALSE;

   while (sd->selected)
     elm_list_item_selected_set((Elm_Object_Item *)sd->selected->data, EINA_FALSE);

   return EINA_TRUE;
}

static Eina_Bool
_item_single_select_up(Elm_List_Smart_Data *sd)
{
   Elm_Object_Item *prev;

   if (!sd->selected) prev = eina_list_data_get(eina_list_last(sd->items));
   else
     {
        prev = elm_list_item_prev(sd->last_selected_item);
        while (prev)
          {
             if (!elm_object_item_disabled_get(prev)) break;
             prev = elm_list_item_prev(prev);
          }
     }
   if (!prev) return EINA_FALSE;

   _all_items_unselect(sd);

   elm_list_item_selected_set(prev, EINA_TRUE);
   elm_list_item_show(prev);

   return EINA_TRUE;
}

static Eina_Bool
_item_single_select_down(Elm_List_Smart_Data *sd)
{
   Elm_Object_Item *next;

   if (!sd->selected) next = eina_list_data_get(sd->items);
   else
     {
        next = elm_list_item_next(sd->last_selected_item);
        while (next)
          {
             if (!elm_object_item_disabled_get(next)) break;
             next = elm_list_item_next(next);
          }
     }
   if (!next) return EINA_FALSE;

   _all_items_unselect(sd);

   elm_list_item_selected_set(next, EINA_TRUE);
   elm_list_item_show(next);

   return EINA_TRUE;
}

// FIXME: There are applications which do not use elm_win as top widget.
// This is workaround! Those could not use focus!
static Eina_Bool
_focus_enabled(Evas_Object *obj)
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

static void
_item_focused(Elm_List_Item *it)
{
   if (!it) return;
   Elm_List_Smart_Data *sd = it->sd;
   Evas_Coord x, y, w, h, sx, sy, sw, sh;

   evas_object_geometry_get(VIEW(it), &x, &y, &w, &h);
   evas_object_geometry_get(ELM_WIDGET_DATA(sd)->obj, &sx, &sy, &sw, &sh);
   if ((x < sx) || (y < sy) || ((x + w) > (sx + sw)) || ((y + h) > (sy + sh)))
     {
        elm_list_item_bring_in((Elm_Object_Item *)it);
     }

   if (_focus_enabled(ELM_WIDGET_DATA(sd)->obj))
     edje_object_signal_emit(VIEW(it), "elm,action,focus_highlight,show", "elm");

   sd->focused = (Elm_Object_Item *)it;
   evas_object_smart_callback_call(WIDGET(it), "item,focused", it);
}

static void
_item_unfocused(Elm_List_Item *it)
{
   if (!it) return;
   Elm_List_Smart_Data *sd = it->sd;
   if (!sd->focused) return;

   if (sd->focused_content)
     {
        elm_object_focus_set(ELM_WIDGET_DATA(sd)->obj, EINA_FALSE);
        elm_object_focus_set(ELM_WIDGET_DATA(sd)->obj, EINA_TRUE);
        sd->focused_content = NULL;
     }
   edje_object_signal_emit(VIEW(sd->focused), "elm,action,focus_highlight,hide", "elm");
   if (it == (Elm_List_Item *)sd->focused)
      sd->focused = NULL;
   evas_object_smart_callback_call(WIDGET(it), "item,unfocused", it);
}

static Eina_Bool
_item_focused_next(Elm_List_Smart_Data *sd, Focus_Dir dir)
{
   Elm_List_Item *it = NULL;
   Elm_List_Item *old_focused = (Elm_List_Item *)sd->focused;

   if (dir == FOCUS_DIR_DOWN || dir == FOCUS_DIR_UP)
     {
        if (dir == FOCUS_DIR_DOWN)
          {
             if (sd->focused)
               {
                  Eina_List *l = eina_list_data_find_list(sd->items, sd->focused);
                  it = (Elm_List_Item *)eina_list_data_get(eina_list_next(l));
                  _item_unfocused((Elm_List_Item *)sd->focused);
               }
             else it = (Elm_List_Item *)eina_list_data_get(sd->items);
          }
        else if (dir == FOCUS_DIR_UP)
          {
             if (sd->focused)
               {
                  Eina_List *l = eina_list_data_find_list(sd->items, sd->focused);
                  it = (Elm_List_Item *)eina_list_data_get(eina_list_prev(l));
                  _item_unfocused((Elm_List_Item *)sd->focused);
               }
             else it = (Elm_List_Item *)eina_list_data_get(eina_list_last(sd->items));
          }

        if (!it)
          {
             _item_focused(old_focused);
             return EINA_FALSE;
          }
        _item_focused(it);
     }
   else if (dir == FOCUS_DIR_LEFT || dir == FOCUS_DIR_RIGHT)
     {
        Evas_Object *obj = sd->focused_content;

        if (!sd->focused) return EINA_FALSE;
        it = (Elm_List_Item *)sd->focused;
        if (sd->focused_content)
          {
             if ((sd->focused_content != it->icon) &&
                 (sd->focused_content != it->end))
               {
                  if (dir == FOCUS_DIR_LEFT)
                    {
                       if (it->icon) obj = it->icon;
                       else obj = it->end;
                    }
                  else if (dir == FOCUS_DIR_RIGHT)
                    {
                       if (it->end) obj = it->end;
                       else obj = it->icon;
                    }
               }
             else
               {
                  if (dir == FOCUS_DIR_LEFT)
                    {
                       if ((it->end == sd->focused_content))
                         obj = it->icon;
                    }
                  else if (dir == FOCUS_DIR_RIGHT)
                    {
                       if ((it->icon == sd->focused_content))
                         obj = it->end;
                    }
               }
          }
        if (!obj && dir == FOCUS_DIR_LEFT)
          {
             if (it->end) obj = it->icon;
             else obj = it->end;
          }
        else if (!obj && dir == FOCUS_DIR_RIGHT)
          {
             if (it->end) obj = it->end;
             else obj = it->icon;
          }

        if (obj)
          {
             sd->focused_content = obj;
             elm_object_focus_set(obj, EINA_TRUE);
          }
        else
          {
             sd->focused_content = NULL;
             return EINA_FALSE;
          }
     }
   else return EINA_FALSE;

   return EINA_TRUE;
}

static Eina_Bool
_elm_list_smart_event(Evas_Object *obj,
                      Evas_Object *src __UNUSED__,
                      Evas_Callback_Type type,
                      void *event_info)
{
   Evas_Coord x = 0;
   Evas_Coord y = 0;
   Evas_Coord v_w = 0;
   Evas_Coord v_h = 0;
   Evas_Coord i_w = 0;
   Evas_Coord i_h = 0;
   Evas_Coord bx = 0;
   Evas_Coord by = 0;
   Evas_Coord step_x = 0;
   Evas_Coord step_y = 0;
   Evas_Coord page_x = 0;
   Evas_Coord page_y = 0;
   Elm_List_Item *it = NULL;
   Evas_Event_Key_Down *ev = event_info;

   ELM_LIST_DATA_GET(obj, sd);

   // TIZEN ONLY(20131221) : When access mode, focused ui is disabled.
   if (_elm_config->access_mode) return EINA_FALSE;

   if (elm_widget_disabled_get(obj)) return EINA_FALSE;
   if (type != EVAS_CALLBACK_KEY_DOWN && type != EVAS_CALLBACK_KEY_UP)
     return EINA_FALSE;
   if (!sd->items) return EINA_FALSE;
   if (ev->event_flags & EVAS_EVENT_FLAG_ON_HOLD) return EINA_FALSE;
   if (!_focus_enabled(obj)) return EINA_FALSE;

   sd->s_iface->content_pos_get(obj, &x, &y);
   sd->s_iface->step_size_get(obj, &step_x, &step_y);
   sd->s_iface->page_size_get(obj, &page_x, &page_y);
   sd->s_iface->content_viewport_size_get(obj, &v_w, &v_h);

   /* TODO: fix logic for horizontal mode */
   if ((!strcmp(ev->keyname, "Left")) ||
       ((!strcmp(ev->keyname, "KP_Left")) && !ev->string))
     {
        if (type != EVAS_CALLBACK_KEY_DOWN) return EINA_FALSE;
        if (sd->select_on_focus_enabled)
          {
             if ((sd->h_mode) &&
                 (((evas_key_modifier_is_set(ev->modifiers, "Shift")) &&
                   (_item_multi_select_up(sd)))
                  || (_item_single_select_up(sd))))
               {
                  ev->event_flags |= EVAS_EVENT_FLAG_ON_HOLD;
                  return EINA_TRUE;
               }
             else
                x -= step_x;
          }
        else
          {
             if (_item_focused_next(sd, FOCUS_DIR_LEFT))
               {
                  ev->event_flags |= EVAS_EVENT_FLAG_ON_HOLD;
                  return EINA_TRUE;
               }
             else return EINA_FALSE;
          }
     }
   else if ((!strcmp(ev->keyname, "Right")) ||
            ((!strcmp(ev->keyname, "KP_Right")) && !ev->string))
     {
        if (type != EVAS_CALLBACK_KEY_DOWN) return EINA_FALSE;
        if (sd->select_on_focus_enabled)
          {
             if ((sd->h_mode) &&
                 (((evas_key_modifier_is_set(ev->modifiers, "Shift")) &&
                   (_item_multi_select_down(sd)))
                  || (_item_single_select_down(sd))))
               {
                  ev->event_flags |= EVAS_EVENT_FLAG_ON_HOLD;
                  return EINA_TRUE;
               }
             else
                x += step_x;
          }
        else
          {
             if ( _item_focused_next(sd, FOCUS_DIR_RIGHT))
               {
                  ev->event_flags |= EVAS_EVENT_FLAG_ON_HOLD;
                  return EINA_TRUE;
               }
             else return EINA_FALSE;
          }
     }
   else if ((!strcmp(ev->keyname, "Up")) ||
            ((!strcmp(ev->keyname, "KP_Up")) && !ev->string))
     {
        if (type != EVAS_CALLBACK_KEY_DOWN) return EINA_FALSE;
        if (sd->select_on_focus_enabled)
          {
             if ((!sd->h_mode) &&
                 (evas_key_modifier_is_set(ev->modifiers, "Shift")) &&
                 (_item_multi_select_up(sd)))
               {
                  ev->event_flags |= EVAS_EVENT_FLAG_ON_HOLD;
                  return EINA_TRUE;
               }
          }
        else
          {
             if (_item_focused_next(sd, FOCUS_DIR_UP))
               {
                  ev->event_flags |= EVAS_EVENT_FLAG_ON_HOLD;
                  return EINA_TRUE;
               }
             else
               return EINA_FALSE;
          }
     }
   else if ((!strcmp(ev->keyname, "Down")) ||
            ((!strcmp(ev->keyname, "KP_Down")) && !ev->string))
     {
        if (type != EVAS_CALLBACK_KEY_DOWN) return EINA_FALSE;
        if (sd->select_on_focus_enabled)
          {
             if ((!sd->h_mode) &&
                 (evas_key_modifier_is_set(ev->modifiers, "Shift")) &&
                 (_item_multi_select_down(sd)))
               {
                  ev->event_flags |= EVAS_EVENT_FLAG_ON_HOLD;
                  return EINA_TRUE;
               }
          }
        else
          {
             if (_item_focused_next(sd, FOCUS_DIR_DOWN))
               {
                  ev->event_flags |= EVAS_EVENT_FLAG_ON_HOLD;
                  return EINA_TRUE;
               }
             else
               return EINA_FALSE;
          }
     }
   else if ((!strcmp(ev->keyname, "Home")) ||
            ((!strcmp(ev->keyname, "KP_Home")) && !ev->string))
     {
        if (type != EVAS_CALLBACK_KEY_DOWN) return EINA_FALSE;
        if (!sd->select_on_focus_enabled)
          {
             _item_unfocused((Elm_List_Item *)sd->focused);
             _item_focused_next(sd, FOCUS_DIR_DOWN);
          }
        ev->event_flags |= EVAS_EVENT_FLAG_ON_HOLD;
        return EINA_TRUE;
     }
   else if ((!strcmp(ev->keyname, "End")) ||
            ((!strcmp(ev->keyname, "KP_End")) && !ev->string))
     {
        if (type != EVAS_CALLBACK_KEY_DOWN) return EINA_FALSE;
        if (!sd->select_on_focus_enabled)
          {
             _item_unfocused((Elm_List_Item *)sd->focused);
             _item_focused_next(sd, FOCUS_DIR_UP);
          }
        ev->event_flags |= EVAS_EVENT_FLAG_ON_HOLD;
        return EINA_TRUE;
     }
   else if ((!strcmp(ev->keyname, "Prior")) ||
            ((!strcmp(ev->keyname, "KP_Prior")) && !ev->string))
     {
        if (type != EVAS_CALLBACK_KEY_DOWN) return EINA_FALSE;

        it = (Elm_List_Item *) sd->focused;
        _item_unfocused(it);

        while (page_y <= v_h)
          {
             if (elm_list_item_prev((Elm_Object_Item *) it))
               it = (Elm_List_Item *) elm_list_item_prev((Elm_Object_Item *) it);
             else break;
             evas_object_geometry_get(VIEW(it), NULL, NULL, &i_w, &i_h);
             page_y += i_h;
          }

        evas_smart_objects_calculate(evas_object_evas_get(sd->box));
        evas_object_geometry_get(sd->box, &bx, &by, NULL, NULL);
        evas_object_geometry_get(VIEW(it), &x, &y, NULL, NULL);
        x -= bx;
        y -= by;
        sd->s_iface->region_bring_in(WIDGET(it), x, y, v_w, v_h);
        edje_object_signal_emit
          (VIEW(it), "elm,action,focus_highlight,show", "elm");
        sd->focused = (Elm_Object_Item *)it;
        ev->event_flags |= EVAS_EVENT_FLAG_ON_HOLD;
        return EINA_TRUE;
     }
   else if ((!strcmp(ev->keyname, "Next")) ||
            ((!strcmp(ev->keyname, "KP_Next")) && !ev->string))
     {
        if (type != EVAS_CALLBACK_KEY_DOWN) return EINA_FALSE;

        it = (Elm_List_Item *) sd->focused;
        _item_unfocused(it);
        evas_object_geometry_get(VIEW(it), NULL, NULL, &i_w, &i_h);
        page_y = i_h;

        while (page_y <= v_h)
          {
             if (elm_list_item_next((Elm_Object_Item *) it))
               it = (Elm_List_Item *) elm_list_item_next((Elm_Object_Item *) it);
             else break;
             evas_object_geometry_get(VIEW(it), NULL, NULL, &i_w, &i_h);
             page_y += i_h;
          }

        evas_smart_objects_calculate(evas_object_evas_get(sd->box));
        evas_object_geometry_get(sd->box, &bx, &by, NULL, NULL);
        evas_object_geometry_get(VIEW(it), &x, &y, NULL, NULL);
        x -= bx;
        y -= by;
        sd->s_iface->region_bring_in(WIDGET(it), x, y, v_w, v_h);
        edje_object_signal_emit
          (VIEW(it), "elm,action,focus_highlight,show", "elm");
        sd->focused = (Elm_Object_Item *)it;
        ev->event_flags |= EVAS_EVENT_FLAG_ON_HOLD;
        return EINA_TRUE;
     }
   else if (((!strcmp(ev->keyname, "Return")) ||
             (!strcmp(ev->keyname, "KP_Enter"))) &&
            (!sd->multi))
     {
        if (type == EVAS_CALLBACK_KEY_DOWN && !sd->key_down)
          {
             if (sd->focused)
               {
                  sd->key_down = sd->focused;
                  it = (Elm_List_Item *) sd->key_down;

                  edje_object_signal_emit
                     (VIEW(it), "elm,action,focus_highlight,hide", "elm");

                  _item_highlight(it);
                  if (it->long_timer)
                    ecore_timer_del(it->long_timer);
                  it->long_timer = ecore_timer_add(_elm_config->longpress_timeout,
                                                              _long_press_cb, it);
                  evas_object_smart_callback_call(obj, SIG_ACTIVATED, it);
                  ev->event_flags |= EVAS_EVENT_FLAG_ON_HOLD;
                  return EINA_TRUE;
               }
             else return EINA_FALSE;
          }
        else if (type == EVAS_CALLBACK_KEY_UP && sd->key_down)
          {
             it = (Elm_List_Item *) sd->key_down;

             edje_object_signal_emit
                (VIEW(it), "elm,action,focus_highlight,show", "elm");

             if (it->long_timer)
               ecore_timer_del(it->long_timer);
             it->long_timer = NULL;
             if (it->highlight_timer)
               ecore_timer_del(it->highlight_timer);
             _select_unselect(it);
             sd->key_down = NULL;
             ev->event_flags |= EVAS_EVENT_FLAG_ON_HOLD;
             return EINA_TRUE;
          }
        else return EINA_FALSE;
     }
   else if (!strcmp(ev->keyname, "Escape"))
     {
        if (type != EVAS_CALLBACK_KEY_DOWN) return EINA_FALSE;
        if (!_all_items_unselect(sd)) return EINA_FALSE;
        ev->event_flags |= EVAS_EVENT_FLAG_ON_HOLD;
        return EINA_TRUE;
     }
   else return EINA_FALSE;

   ev->event_flags |= EVAS_EVENT_FLAG_ON_HOLD;
   sd->s_iface->content_pos_set(obj, x, y, EINA_TRUE);

   return EINA_TRUE;
}

static Eina_Bool
_elm_list_smart_translate(Evas_Object *obj)
{
   ELM_LIST_DATA_GET(obj, sd);
   Elm_List_Item *it;
   Eina_List *l;

   EINA_LIST_FOREACH(sd->items, l, it)
     elm_widget_item_translate(it);

   evas_object_smart_callback_call(obj, SIG_LANG_CHANGED, NULL);

   return EINA_TRUE;
}

static void
_elm_list_deletions_process(Elm_List_Smart_Data *sd)
{
   Elm_List_Item *it;

   sd->walking++; // avoid nested deletion and also _sub_del() items_fix

   EINA_LIST_FREE (sd->to_delete, it)
     {
        sd->items = eina_list_remove_list(sd->items, it->node);

        /* issuing free because of "locking" item del pre hook */
        _elm_list_item_free(it);
        elm_widget_item_free(it);
     }

   sd->walking--;
}

static void
_elm_list_smart_sizing_eval(Evas_Object *obj)
{
   Evas_Coord vw = 0, vh = 0;
   Evas_Coord minw, minh, maxw, maxh, w, h, vmw, vmh;
   double xw, yw;

   ELM_LIST_DATA_GET(obj, sd);

   /* parent class' early call */
   if (!sd->s_iface) return;
   if (!sd->box) return;

   evas_object_size_hint_min_get(sd->box, &minw, &minh);
   evas_object_size_hint_max_get(sd->box, &maxw, &maxh);
   evas_object_size_hint_weight_get(sd->box, &xw, &yw);

   sd->s_iface->content_viewport_size_get(obj, &vw, &vh);
   if (xw > 0.0)
     {
        if ((minw > 0) && (vw < minw)) vw = minw;
        else if ((maxw > 0) && (vw > maxw))
          vw = maxw;
     }
   else if (minw > 0)
     vw = minw;
   if (yw > 0.0)
     {
        if ((minh > 0) && (vh < minh)) vh = minh;
        else if ((maxh > 0) && (vh > maxh))
          vh = maxh;
     }
   else if (minh > 0)
     vh = minh;

   evas_object_resize(sd->box, vw, vh);
   w = -1;
   h = -1;

   edje_object_size_min_calc
     (ELM_WIDGET_DATA(sd)->resize_obj, &vmw, &vmh);

   if (sd->scr_minw) w = vmw + minw;
   if (sd->scr_minh) h = vmh + minh;

   evas_object_size_hint_max_get(obj, &maxw, &maxh);
   if ((maxw > 0) && (w > maxw))
     w = maxw;
   if ((maxh > 0) && (h > maxh))
     h = maxh;

   evas_object_size_hint_min_set(obj, w, h);
}

static void
_elm_list_content_min_limit_cb(Evas_Object *obj,
                               Eina_Bool w,
                               Eina_Bool h)
{
   ELM_LIST_DATA_GET(obj, sd);

   if ((sd->mode == ELM_LIST_LIMIT) ||
       (sd->mode == ELM_LIST_EXPAND)) return;
   sd->scr_minw = !!w;
   sd->scr_minh = !!h;

   elm_layout_sizing_eval(obj);
}

static void
_elm_list_mode_set_internal(Elm_List_Smart_Data *sd)
{
   if (sd->mode == ELM_LIST_LIMIT)
     {
        if (!sd->h_mode)
          {
             sd->scr_minw = EINA_TRUE;
             sd->scr_minh = EINA_FALSE;
          }
        else
          {
             sd->scr_minw = EINA_FALSE;
             sd->scr_minh = EINA_TRUE;
          }
     }
   else if (sd->mode == ELM_LIST_EXPAND)
     {
        sd->scr_minw = EINA_TRUE;
        sd->scr_minh = EINA_TRUE;
     }

   elm_layout_sizing_eval(ELM_WIDGET_DATA(sd)->obj);
}

static inline void
_elm_list_walk(Elm_List_Smart_Data *sd)
{
   if (sd->walking < 0)
     {
        ERR("ERROR: walking was negative. fixed!\n");
        sd->walking = 0;
     }
   sd->walking++;
}

static inline void
_elm_list_unwalk(Elm_List_Smart_Data *sd)
{
   sd->walking--;
   if (sd->walking < 0)
     {
        ERR("ERROR: walking became negative. fixed!\n");
        sd->walking = 0;
     }

   if (sd->walking)
     return;

   if (sd->to_delete)
     _elm_list_deletions_process(sd);

   if (sd->fix_pending)
     {
        sd->fix_pending = EINA_FALSE;
        _items_fix(ELM_WIDGET_DATA(sd)->obj);
        elm_layout_sizing_eval(ELM_WIDGET_DATA(sd)->obj);
     }
}

static void
_items_fix(Evas_Object *obj)
{
   Evas_Coord minw[2] = { 0, 0 }, minh[2] = { 0, 0 };
   const Eina_List *l;
   Elm_List_Item *it;
   Evas_Coord mw, mh;
   int i, redo = 0;

   const char *style;
   const char *it_odd;
   const char *it_plain;
   const char *it_compress;
   const char *it_compress_odd;

   ELM_LIST_DATA_GET(obj, sd);

   style = elm_widget_style_get(obj);
   it_plain = sd->h_mode ? "h_item" : "item";
   it_odd = sd->h_mode ? "h_item_odd" : "item_odd";
   it_compress = sd->h_mode ? "h_item_compress" : "item_compress";
   it_compress_odd = sd->h_mode ? "h_item_compress_odd" : "item_compress_odd";

   if (sd->walking)
     {
        sd->fix_pending = EINA_TRUE;
        return;
     }

   evas_object_ref(obj);
   _elm_list_walk(sd); // watch out "return" before unwalk!

   EINA_LIST_FOREACH(sd->items, l, it)
     {
        if (it->deleted) continue;
        if (it->icon)
          {
             evas_object_size_hint_min_get(it->icon, &mw, &mh);
             if (mw > minw[0]) minw[0] = mw;
             if (mh > minh[0]) minh[0] = mh;
          }
        if (it->end)
          {
             evas_object_size_hint_min_get(it->end, &mw, &mh);
             if (mw > minw[1]) minw[1] = mw;
             if (mh > minh[1]) minh[1] = mh;
          }
     }

   if ((minw[0] != sd->minw[0]) || (minw[1] != sd->minw[1]) ||
       (minh[0] != sd->minh[0]) || (minh[1] != sd->minh[1]))
     {
        sd->minw[0] = minw[0];
        sd->minw[1] = minw[1];
        sd->minh[0] = minh[0];
        sd->minh[1] = minh[1];
        redo = 1;
     }

   i = 0;
   EINA_LIST_FOREACH(sd->items, l, it)
     {
        if (it->deleted)
          continue;

        it->even = i & 0x1;
        if ((it->even != it->is_even) || (!it->fixed) || (redo))
          {
             const char *stacking;

             /* FIXME: separators' themes seem to be b0rked */
             if (it->is_separator)
               elm_widget_theme_object_set
                 (obj, VIEW(it), "separator", sd->h_mode ?
                 "horizontal" : "vertical", style);
             else if (sd->mode == ELM_LIST_COMPRESS)
               {
                  if (it->even)
                    elm_widget_theme_object_set
                      (obj, VIEW(it), "list", it_compress, style);
                  else
                    elm_widget_theme_object_set
                      (obj, VIEW(it), "list", it_compress_odd, style);
               }
             else
               {
                  if (it->even)
                    elm_widget_theme_object_set
                      (obj, VIEW(it), "list", it_plain, style);
                  else
                    elm_widget_theme_object_set
                      (obj, VIEW(it), "list", it_odd, style);
               }
             stacking = edje_object_data_get(VIEW(it), "stacking");
             if (stacking)
               {
                  if (!strcmp(stacking, "below"))
                    evas_object_lower(VIEW(it));
                  else if (!strcmp(stacking, "above"))
                    evas_object_raise(VIEW(it));
               }
             edje_object_part_text_escaped_set
               (VIEW(it), "elm.text", it->label);

             if ((!it->icon) && (minh[0] > 0))
               {
                  it->icon = evas_object_rectangle_add
                      (evas_object_evas_get(VIEW(it)));
                  evas_object_color_set(it->icon, 0, 0, 0, 0);
                  it->dummy_icon = EINA_TRUE;
               }
             if ((!it->end) && (minh[1] > 0))
               {
                  it->end = evas_object_rectangle_add
                      (evas_object_evas_get(VIEW(it)));
                  evas_object_color_set(it->end, 0, 0, 0, 0);
                  it->dummy_end = EINA_TRUE;
               }
             if (it->icon)
               {
                  evas_object_size_hint_min_set(it->icon, minw[0], minh[0]);
                  evas_object_size_hint_max_set(it->icon, 99999, 99999);
                  edje_object_part_swallow
                    (VIEW(it), "elm.swallow.icon", it->icon);
               }
             if (it->end)
               {
                  evas_object_size_hint_min_set(it->end, minw[1], minh[1]);
                  evas_object_size_hint_max_set(it->end, 99999, 99999);
                  edje_object_part_swallow
                    (VIEW(it), "elm.swallow.end", it->end);
               }
             if (!it->fixed)
               {
                  // this may call up user and it may modify the list item
                  // but we're safe as we're flagged as walking.
                  // just don't process further
                  edje_object_message_signal_process(VIEW(it));
                  if (it->deleted)
                    continue;
                  mw = mh = -1;
                  elm_coords_finger_size_adjust(1, &mw, 1, &mh);
                  edje_object_size_min_restricted_calc
                    (VIEW(it), &mw, &mh, mw, mh);
                  elm_coords_finger_size_adjust(1, &mw, 1, &mh);
                  evas_object_size_hint_min_set(VIEW(it), mw, mh);
                  evas_object_show(VIEW(it));
               }
             if ((it->selected) || (it->highlighted))
               {
                  const char *select_raise;

                  // this may call up user and it may modify the list item
                  // but we're safe as we're flagged as walking.
                  // just don't process further
                  edje_object_signal_emit
                    (VIEW(it), "elm,state,selected", "elm");
                  if (it->deleted)
                    continue;

                  select_raise = edje_object_data_get(VIEW(it), "selectraise");
                  if ((select_raise) && (!strcmp(select_raise, "on")))
                    evas_object_raise(VIEW(it));
               }
             if (it->base.disabled)
               edje_object_signal_emit(VIEW(it), "elm,state,disabled", "elm");

             it->fixed = EINA_TRUE;
             it->is_even = it->even;
          }
        i++;
     }

   _elm_list_mode_set_internal(sd);
   _elm_list_unwalk(sd);

   evas_object_unref(obj);
}

static void
_size_hints_changed_cb(void *data,
                       Evas *e __UNUSED__,
                       Evas_Object *obj __UNUSED__,
                       void *event_info __UNUSED__)
{
   ELM_LIST_DATA_GET(data, sd);
   if (sd->delete_me) return;

   _items_fix(data);
   elm_layout_sizing_eval(data);
}

/* FIXME: take off later. maybe this show region coords belong in the
 * interface (new api functions, set/get)? */
static void
_show_region_hook(void *data,
                  Evas_Object *obj)
{
   Evas_Coord x, y, w, h;

   ELM_LIST_DATA_GET(data, sd);

   elm_widget_show_region_get(obj, &x, &y, &w, &h);
   sd->s_iface->content_region_set(obj, x, y, w, h);
}

static Eina_Bool
_elm_list_smart_disable(Evas_Object *obj)
{
   ELM_LIST_DATA_GET(obj, sd);

   if (!ELM_WIDGET_CLASS(_elm_list_parent_sc)->disable(obj))
     return EINA_FALSE;

   if (elm_widget_disabled_get(obj))
     {
        elm_widget_scroll_freeze_push(obj);
        elm_widget_scroll_hold_push(obj);
        /* FIXME: if we get to have a way to only un-highlight items
         * in the future, keeping them selected... */
        _all_items_unselect(sd);
     }
   else
     {
        elm_widget_scroll_freeze_pop(obj);
        elm_widget_scroll_hold_pop(obj);
     }

   return EINA_TRUE;
}

static void
_mirrored_set(Evas_Object *obj,
              Eina_Bool rtl)
{
   Elm_List_Item *it;
   Eina_List *n;

   ELM_LIST_DATA_GET(obj, sd);

   sd->s_iface->mirrored_set(obj, rtl);

   EINA_LIST_FOREACH(sd->items, n, it)
     edje_object_mirrored_set(VIEW(it), rtl);
}

static Eina_Bool
_elm_list_smart_theme(Evas_Object *obj)
{
   Elm_List_Item *it;
   Eina_List *n;

   ELM_LIST_DATA_GET(obj, sd);

   if (!ELM_WIDGET_CLASS(_elm_list_parent_sc)->theme(obj)) return EINA_FALSE;

   _mirrored_set(obj, elm_widget_mirrored_get(obj));

   EINA_LIST_FOREACH(sd->items, n, it)
     {
        edje_object_scale_set
          (VIEW(it), elm_widget_scale_get(obj) * elm_config_scale_get());
        it->fixed = 0;
     }

   _items_fix(obj);
   elm_layout_sizing_eval(obj);

   return EINA_TRUE;
}

static void
_elm_list_focus_highlight_show(void *data __UNUSED__,
                                  Evas_Object *obj,
                                  const char *emission __UNUSED__,
                                  const char *src __UNUSED__)
{
   ELM_LIST_DATA_GET(obj, sd);
   if (sd->focused) _item_focused((Elm_List_Item *) sd->focused);
}

static void
_elm_list_focus_highlight_hide(void *data __UNUSED__,
                                  Evas_Object *obj,
                                  const char *emission __UNUSED__,
                                  const char *src __UNUSED__)
{
   ELM_LIST_DATA_GET(obj, sd);
   if (sd->focused)
     {
        // Do not use _item_unfocused because focus should be remained
        edje_object_signal_emit
           (VIEW(sd->focused), "elm,action,focus_highlight,hide", "elm");
     }
}

static Eina_Bool
_elm_list_smart_on_focus(Evas_Object *obj, Elm_Focus_Info *info)
{
   ELM_LIST_DATA_GET(obj, sd);

   if (!ELM_WIDGET_CLASS(_elm_list_parent_sc)->on_focus(obj, info))
     return EINA_FALSE;

   if (elm_widget_focus_get(obj) && sd->selected && !sd->last_selected_item)
     sd->last_selected_item = eina_list_data_get(sd->selected);

   if (sd->select_on_focus_enabled) return EINA_TRUE;
   if (elm_widget_focus_get(obj))
     {
        if (_focus_enabled(obj))
          {
             if (sd->focused)
               _item_focused((Elm_List_Item *)sd->focused);
             else
               _item_focused_next(sd, FOCUS_DIR_DOWN);
          }
     }
   else
     {
        // when key down and not called key up
        // and focus is not on list, call select_unselect forcely
        if (sd->key_down)
          {
             _select_unselect(sd->key_down);
             sd->key_down = NULL;
          }

        if (sd->focused)
          edje_object_signal_emit
            (VIEW(sd->focused), "elm,action,focus_highlight,hide", "elm");
     }
   return EINA_TRUE;
}

static Eina_Bool
_elm_list_smart_sub_object_del(Evas_Object *obj,
                               Evas_Object *sobj)
{
   const Eina_List *l;
   Elm_List_Item *it;

   ELM_LIST_DATA_GET(obj, sd);

   if (!ELM_WIDGET_CLASS(_elm_list_parent_sc)->sub_object_del(obj, sobj))
     return EINA_FALSE;

   if ((sobj == sd->box) || (sobj == obj)) goto end;

   EINA_LIST_FOREACH(sd->items, l, it)
     {
        if ((sobj == it->icon) || (sobj == it->end))
          {
             if (it->icon == sobj) it->icon = NULL;
             if (it->end == sobj) it->end = NULL;
             evas_object_event_callback_del_full
               (sobj, EVAS_CALLBACK_CHANGED_SIZE_HINTS, _size_hints_changed_cb,
               obj);
             if (!sd->walking)
               {
                  _items_fix(obj);
                  elm_layout_sizing_eval(obj);
               }
             else
               sd->fix_pending = EINA_TRUE;
             break;
          }
     }

end:
   return EINA_TRUE;
}

static void
_item_highlight(Elm_List_Item *it)
{
   ELM_LIST_ITEM_CHECK_OR_RETURN(it);
   Elm_List_Smart_Data *sd = it->sd;
   Evas_Object *obj = WIDGET(it);

   if (elm_widget_item_disabled_get(it)) return;
   if (sd->select_mode == ELM_OBJECT_SELECT_MODE_NONE) return;
   if (it->highlighted) return;

   it->highlighted = EINA_TRUE;
   edje_object_signal_emit(VIEW(it), "elm,state,selected", "elm");

   evas_object_ref(obj);
   _elm_list_walk(sd);
   evas_object_smart_callback_call(obj, SIG_HIGHLIGHTED, it);
   _elm_list_unwalk(sd);
   evas_object_unref(obj);
}

static void
_item_unhighlight(Elm_List_Item *it)
{
   if (!it->highlighted) return;
   Elm_List_Smart_Data *sd = it->sd;
   Evas_Object *obj = WIDGET(it);

   it->highlighted = EINA_FALSE;

   edje_object_signal_emit(VIEW(it), "elm,state,unselected", "elm");

   evas_object_ref(obj);
   _elm_list_walk(sd);
   evas_object_smart_callback_call(WIDGET(it), SIG_UNHIGHLIGHTED, it);
   _elm_list_unwalk(sd);
   evas_object_unref(obj);

}

static void
_item_select(Elm_List_Item *it)
{
   const char *select_raise;
   ELM_LIST_ITEM_CHECK_OR_RETURN(it);

   Elm_List_Smart_Data *sd = it->sd;
   Evas_Object *obj = WIDGET(it);

   if (elm_widget_item_disabled_get(it)) return;
   if (sd->select_mode == ELM_OBJECT_SELECT_MODE_NONE) return;
   if (sd->select_mode != ELM_OBJECT_SELECT_MODE_ALWAYS &&
       it->selected)
      return;

   if (!sd->multi)
     {
        const Eina_List *l, *ll;
        Elm_List_Item *it2;
        EINA_LIST_FOREACH_SAFE(sd->selected, l, ll, it2)
          if (it2 != it) _item_unselect(it2);
     }

   if (!it->selected)
     {
        sd->selected = eina_list_append(sd->selected, it);
        it->selected = EINA_TRUE;
     }

   sd->last_selected_item = (Elm_Object_Item *)it;
   _item_highlight(it);

   select_raise = edje_object_data_get(VIEW(it), "selectraise");
   if ((select_raise) && (!strcmp(select_raise, "on")))
     evas_object_raise(VIEW(it));

   edje_object_signal_emit
      (VIEW(it), "elm,state,clicked", "elm");

   evas_object_ref(obj);
   _elm_list_walk(sd);

   if (it->func) it->func((void *)it->base.data, obj, it);
   evas_object_smart_callback_call(obj, SIG_SELECTED, it);

   _elm_list_unwalk(sd);
   evas_object_unref(obj);
}

static void
_item_unselect(Elm_List_Item *it)
{
   const char *stacking, *select_raise;
   ELM_LIST_ITEM_CHECK_OR_RETURN(it);

   Elm_List_Smart_Data *sd = it->sd;
   Evas_Object *obj = WIDGET(it);

   if (!it->selected) return;

   stacking = edje_object_data_get(VIEW(it), "stacking");
   select_raise = edje_object_data_get(VIEW(it), "selectraise");
   if ((select_raise) && (!strcmp(select_raise, "on")))
     {
        if ((stacking) && (!strcmp(stacking, "below")))
          evas_object_lower(VIEW(it));
     }

   it->selected = EINA_FALSE;
   sd->selected = eina_list_remove(sd->selected, it);

   if (it->unhighlight_timer)
     {
        ecore_timer_del(it->unhighlight_timer);
        it->unhighlight_timer = NULL;
     }
   _item_unhighlight(it);

   evas_object_ref(obj);
   _elm_list_walk(sd);
   evas_object_smart_callback_call(WIDGET(it), SIG_UNSELECTED, it);
   _elm_list_unwalk(sd);
   evas_object_unref(obj);
}

static Eina_Bool
_swipe_cancel(void *data)
{
   Elm_List_Item *it = data;

   ELM_LIST_ITEM_CHECK_OR_RETURN(it, ECORE_CALLBACK_CANCEL);
   ELM_LIST_DATA_GET(WIDGET(it), sd);

   sd->swipe = EINA_FALSE;
   sd->movements = 0;

   return ECORE_CALLBACK_RENEW;
}

static void
_scroll_animate_stop_cb(Evas_Object *obj,
                        void *data __UNUSED__)
{
   evas_object_smart_callback_call(obj, SIG_SCROLL_ANIM_STOP, NULL);
}

static void
_scroll_animate_start_cb(Evas_Object *obj,
                         void *data __UNUSED__)
{
   evas_object_smart_callback_call(obj, SIG_SCROLL_ANIM_START, NULL);
}


static void
_scroll_drag_start_cb(Evas_Object *obj,
                      void *data __UNUSED__)
{
   evas_object_smart_callback_call(obj, SIG_SCROLL_DRAG_START, NULL);
}

static void
_scroll_drag_stop_cb(Evas_Object *obj,
                     void *data __UNUSED__)
{
   evas_object_smart_callback_call(obj, SIG_SCROLL_DRAG_STOP, NULL);
}

static void
_scroll_cb(Evas_Object *obj,
           void *data __UNUSED__)
{
   evas_object_smart_callback_call(obj, SIG_SCROLL, NULL);
}

static void
_edge_left_cb(Evas_Object *obj,
              void *data __UNUSED__)
{
   evas_object_smart_callback_call(obj, SIG_EDGE_LEFT, NULL);
}

static void
_edge_right_cb(Evas_Object *obj,
               void *data __UNUSED__)
{
   evas_object_smart_callback_call(obj, SIG_EDGE_RIGHT, NULL);
}

static void
_edge_top_cb(Evas_Object *obj,
             void *data __UNUSED__)
{
   evas_object_smart_callback_call(obj, SIG_EDGE_TOP, NULL);
}

static void
_edge_bottom_cb(Evas_Object *obj,
                void *data __UNUSED__)
{
   evas_object_smart_callback_call(obj, SIG_EDGE_BOTTOM, NULL);
}

static Eina_Bool
_long_press_cb(void *data)
{
   Elm_List_Item *it = data;
   Evas_Object *obj;

   it->long_timer = NULL;
   ELM_LIST_ITEM_CHECK_OR_RETURN(it, ECORE_CALLBACK_CANCEL);
   obj = WIDGET(it);
   ELM_LIST_DATA_GET(obj, sd);

   if (elm_widget_item_disabled_get(it)) return ECORE_CALLBACK_CANCEL;

   sd->longpressed = EINA_TRUE;
   evas_object_smart_callback_call(WIDGET(it), SIG_LONGPRESSED, it);

   return ECORE_CALLBACK_CANCEL;
}

static void
_swipe_do(Elm_List_Item *it)
{
   int i, sum = 0;

   ELM_LIST_ITEM_CHECK_OR_RETURN(it);

   ELM_LIST_DATA_GET(WIDGET(it), sd);

   sd->swipe = EINA_FALSE;
   for (i = 0; i < sd->movements; i++)
     {
        sum += sd->history[i].x;
        if (abs(sd->history[0].y - sd->history[i].y) > 10) return;
     }

   sum /= sd->movements;
   if (abs(sum - sd->history[0].x) <= 10) return;

   evas_object_smart_callback_call(WIDGET(it), SIG_SWIPE, it);
}

static Eina_Bool
_highlight_timer(void *data)
{
   Elm_List_Item *it = data;
   it->highlight_timer = NULL;
   _item_highlight(it);

   return EINA_FALSE;
}

static Eina_Bool
_unhighlight_timer(void *data)
{
   Elm_List_Item *it = data;
   it->unhighlight_timer = NULL;
   _item_unhighlight(it);

   if (it->selected) _item_unselect(it);

   return EINA_FALSE;
}

static Eina_Bool
_select_unselect(void *data)
{
   Elm_List_Item *it = data;

   if (it->sd->select_mode == ELM_OBJECT_SELECT_MODE_ALWAYS)
      _item_select(it);
   else
     {
        if (!it->selected) _item_select(it);
        else _item_unselect(it);
     }

   return EINA_FALSE;
}

static void
_highlight_timer_disable(Elm_List_Item *it)
{
   if (it->highlight_timer)
     {
        ecore_timer_del(it->highlight_timer);
        it->highlight_timer = NULL;
     }
}

static void
_highlight_timer_enable(Elm_List_Item *it)
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
   Elm_List_Item *it = data;
   Evas_Event_Mouse_Move *ev = event_info;

   ELM_LIST_ITEM_CHECK_OR_RETURN(it);
   obj = WIDGET(it);
   ELM_LIST_DATA_GET(obj, sd);

   evas_object_ref(obj);
   _elm_list_walk(sd);

   if (ev->event_flags & EVAS_EVENT_FLAG_ON_HOLD)
     {
        if (it->highlight_timer)
          {
             ecore_timer_del(it->highlight_timer);
             it->highlight_timer = NULL;
          }
        else
          {
             if (it->unhighlight_timer)
               {
                  ecore_timer_del(it->unhighlight_timer);
                  it->unhighlight_timer = NULL;
               }
             _item_unhighlight(it);
          }

        if (!sd->on_hold)
          {
             sd->on_hold = EINA_TRUE;
             if (it->long_timer)
               {
                  ecore_timer_del(it->long_timer);
                  it->long_timer = NULL;
               }
             if (!sd->was_selected)
               _item_unselect(it);
          }
        if (sd->movements == ELM_LIST_SWIPE_MOVES) sd->swipe = EINA_TRUE;
        else
          {
             sd->history[sd->movements].x = ev->cur.canvas.x;
             sd->history[sd->movements].y = ev->cur.canvas.y;
             if (abs((sd->history[sd->movements].x - sd->history[0].x)) > 40)
               sd->swipe = EINA_TRUE;
             else
               sd->movements++;
          }
     }
   //******************** TIZEN Only
   else
     {
        Evas_Coord x, y, w, h;
        evas_object_geometry_get(VIEW(it), &x, &y, &w, &h);
        if (ELM_RECTS_POINT_OUT(x, y, w, h, ev->cur.canvas.x, ev->cur.canvas.y))
          {
             if (it->long_timer)
               {
                  ecore_timer_del(it->long_timer);
                  it->long_timer = NULL;
               }
             if (it->highlight_timer)
               {
                  ecore_timer_del(it->highlight_timer);
                  it->highlight_timer = NULL;
               }
             else
               {
                  if (it->unhighlight_timer)
                    {
                       ecore_timer_del(it->unhighlight_timer);
                       it->unhighlight_timer = NULL;
                    }
                  _item_unhighlight(it);
               }
          }
     }
   //*******************************

   _elm_list_unwalk(sd);
   evas_object_unref(obj);
}

static void
_mouse_down_cb(void *data,
               Evas *evas,
               Evas_Object *o __UNUSED__,
               void *event_info)
{
   Evas_Event_Mouse_Down *ev = event_info;
   Elm_List_Item *it = data;
   Evas_Object *obj;

   ELM_LIST_ITEM_CHECK_OR_RETURN(it);
   obj = WIDGET(it);
   ELM_LIST_DATA_GET(obj, sd);

   if (ev->button != 1) return;

   // mouse_down is only activated when one finger touched
   if (evas_event_down_count_get(evas) != 1) return;

   if (ev->event_flags & EVAS_EVENT_FLAG_ON_HOLD) sd->on_hold = EINA_TRUE;
   else sd->on_hold = EINA_FALSE;

   if (sd->on_hold) return;
   sd->was_selected = it->selected;

   evas_object_ref(obj);
   _elm_list_walk(sd);

   _highlight_timer_enable(it);

   sd->longpressed = EINA_FALSE;
   if (it->long_timer) ecore_timer_del(it->long_timer);
   it->long_timer = ecore_timer_add
       (_elm_config->longpress_timeout, _long_press_cb, it);
   if (it->swipe_timer) ecore_timer_del(it->swipe_timer);
   it->swipe_timer = ecore_timer_add(0.4, _swipe_cancel, it);

   /* Always call the callbacks last - the user may delete our context! */
   if (ev->flags & EVAS_BUTTON_DOUBLE_CLICK)
     {
        evas_object_smart_callback_call(WIDGET(it), SIG_CLICKED_DOUBLE, it);
        evas_object_smart_callback_call(WIDGET(it), SIG_ACTIVATED, it);
     }
   sd->swipe = EINA_FALSE;
   sd->movements = 0;

   _elm_list_unwalk(sd);
   evas_object_unref(obj);
}

static void
_mouse_up_cb(void *data,
             Evas *evas,
             Evas_Object *o __UNUSED__,
             void *event_info)
{
   Evas_Object *obj;
   Elm_List_Item *it = data;
   Evas_Event_Mouse_Up *ev = event_info;

   ELM_LIST_ITEM_CHECK_OR_RETURN(it);
   obj = WIDGET(it);
   ELM_LIST_DATA_GET(obj, sd);

   if (ev->button != 1) return;

   if (ev->event_flags & EVAS_EVENT_FLAG_ON_HOLD) sd->on_hold = EINA_TRUE;
   else sd->on_hold = EINA_FALSE;
   sd->longpressed = EINA_FALSE;

   // mouse up is only activated when no finger touched
   if (evas_event_down_count_get(evas) != 0)
     {
        if (!it->highlight_timer)
          {
             if (it->unhighlight_timer)
               {
                  ecore_timer_del(it->unhighlight_timer);
                  it->unhighlight_timer = NULL;
               }
             _item_unhighlight(it);
          }
     }
   if (it->long_timer)
     {
        ecore_timer_del(it->long_timer);
        it->long_timer = NULL;
     }
   if (it->swipe_timer)
     {
        ecore_timer_del(it->swipe_timer);
        it->swipe_timer = NULL;
     }
   if (it->highlight_timer)
     {
        ecore_timer_del(it->highlight_timer);
        it->highlight_timer = NULL;
        if (evas_event_down_count_get(evas) == 0) _item_highlight(it);
     }
   // FIXME: if highlight mode is not used, mouse move cannot disable
   // _item_select
   if (it->highlighted && (evas_event_down_count_get(evas) == 0))
     _select_unselect(it);

   if (sd->on_hold)
     {
        if (sd->swipe) _swipe_do(data);
        sd->on_hold = EINA_FALSE;
        return;
     }
   if (sd->longpressed)
     {
        if (!sd->was_selected) _item_unselect(it);
        sd->was_selected = 0;
        return;
     }
}

static void
_item_disable_hook(Elm_Object_Item *it)
{
   Elm_List_Item *item = (Elm_List_Item *)it;

   _item_unselect(item);
   if (it == item->sd->focused) _item_unfocused(item);
   if (item->highlight_timer)
     {
        ecore_timer_del(item->highlight_timer);
        item->highlight_timer = NULL;
     }

   if (elm_widget_item_disabled_get(it))
     edje_object_signal_emit(VIEW(item), "elm,state,disabled", "elm");
   else
     edje_object_signal_emit(VIEW(item), "elm,state,enabled", "elm");
}

static void
_item_content_set_hook(Elm_Object_Item *it,
                       const char *part,
                       Evas_Object *content)
{
   Elm_List_Item *item = (Elm_List_Item *)it;
   Evas_Object **icon_p = NULL;
   Eina_Bool dummy = EINA_FALSE;

   if ((!part) || (!strcmp(part, "start")))
     {
        icon_p = &(item->icon);
        dummy = item->dummy_icon;
        if (!content) item->dummy_icon = EINA_TRUE;
        else item->dummy_icon = EINA_FALSE;
     }
   else if (!strcmp(part, "end"))
     {
        icon_p = &(item->end);
        dummy = item->dummy_end;
        if (!content) item->dummy_end = EINA_TRUE;
        else item->dummy_end = EINA_FALSE;
     }
   else
     return;

   if (content == *icon_p) return;
   if ((dummy) && (!content)) return;
   if (dummy) evas_object_del(*icon_p);
   if (!content)
     {
        content =
          evas_object_rectangle_add(evas_object_evas_get(WIDGET(item)));
        evas_object_color_set(content, 0, 0, 0, 0);
     }
   if (*icon_p)
     {
        evas_object_del(*icon_p);
        *icon_p = NULL;
     }
   *icon_p = content;

   if (VIEW(item))
     edje_object_part_swallow(VIEW(item), "elm.swallow.icon", content);
}

static Evas_Object *
_item_content_get_hook(const Elm_Object_Item *it,
                       const char *part)
{
   Elm_List_Item *item = (Elm_List_Item *)it;

   if ((!part) || (!strcmp(part, "start")))
     {
        if (item->dummy_icon) return NULL;
        return item->icon;
     }
   else if (!strcmp(part, "end"))
     {
        if (item->dummy_end) return NULL;
        return item->end;
     }

   return NULL;
}

static Evas_Object *
_item_content_unset_hook(const Elm_Object_Item *it,
                         const char *part)
{
   Elm_List_Item *item = (Elm_List_Item *)it;

   if ((!part) || (!strcmp(part, "start")))
     {
        Evas_Object *obj = item->icon;
        _item_content_set_hook((Elm_Object_Item *)it, part, NULL);
        return obj;
     }
   else if (!strcmp(part, "end"))
     {
        Evas_Object *obj = item->end;
        _item_content_set_hook((Elm_Object_Item *)it, part, NULL);
        return obj;
     }

   return NULL;
}

static void
_item_text_set_hook(Elm_Object_Item *it,
                    const char *part,
                    const char *text)
{
   Elm_List_Item *list_it = (Elm_List_Item *)it;

   if (part && strcmp(part, "default")) return;
   if (!eina_stringshare_replace(&list_it->label, text)) return;
   if (VIEW(list_it))
     edje_object_part_text_escaped_set(VIEW(list_it), "elm.text", text);
}

static const char *
_item_text_get_hook(const Elm_Object_Item *it,
                    const char *part)
{
   if (part && strcmp(part, "default")) return NULL;
   return ((Elm_List_Item *)it)->label;
}

static Eina_Bool
_item_del_pre_hook(Elm_Object_Item *it)
{
   Evas_Object *obj = WIDGET(it);
   Elm_List_Item *item = (Elm_List_Item *)it;

   ELM_LIST_DATA_GET(obj, sd);

   if (item->selected) _item_unselect(item);
   if (sd->focused == it) sd->focused = NULL;
   if (sd->key_down == it) sd->key_down = NULL;
   if (item->unhighlight_timer)
     {
        ecore_timer_del(item->unhighlight_timer);
        item->unhighlight_timer = NULL;
     }
   _highlight_timer_disable((Elm_List_Item *)it);

   if (sd->walking > 0)
     {
        if (item->deleted) return EINA_FALSE;
        item->deleted = EINA_TRUE;
        sd->to_delete = eina_list_append(sd->to_delete, item);
        return EINA_FALSE;
     }

   sd->items = eina_list_remove_list(sd->items, item->node);

   evas_object_ref(obj);
   _elm_list_walk(sd);

   _elm_list_item_free(item);

   _elm_list_unwalk(sd);
   evas_object_unref(obj);

   return EINA_TRUE;
}

static void
_item_signal_emit_hook(Elm_Object_Item *it,
                       const char *emission,
                       const char *source)
{
   edje_object_signal_emit(VIEW(it), emission, source);
}

static char *
_access_info_cb(void *data, Evas_Object *obj __UNUSED__)
{
   Elm_List_Item *it = (Elm_List_Item *)data;
   const char *txt = NULL;
   if (!it) return NULL;

   if (!txt) txt = it->label;
   if (txt) return strdup(txt);

   return NULL;
}

static char *
_access_state_cb(void *data, Evas_Object *obj __UNUSED__)
{
   Elm_List_Item *it = (Elm_List_Item *)data;
   if (!it) return NULL;

   if (it->base.disabled)
     return strdup(E_("IDS_ACCS_BODY_DISABLED_TTS"));

   return NULL;
}

static void
_access_on_highlight_cb(void *data)
{
   Elm_Object_Item *it = (Elm_Object_Item *)data;
   if (!it) return;

   elm_list_item_bring_in(it);
}

static void
_access_activate_cb(void *data __UNUSED__,
                    Evas_Object *part_obj __UNUSED__,
                    Elm_Object_Item *item)
{
   _item_select((Elm_List_Item *)item);
}

static void
_access_widget_item_register(Elm_List_Item *it, Eina_Bool is_access)
{
   Elm_Access_Info *ai;

   if (!is_access) _elm_access_widget_item_unregister((Elm_Widget_Item *)it);
   else
     {
        _elm_access_widget_item_register((Elm_Widget_Item *)it);

        ai = _elm_access_object_get(it->base.access_obj);

        _elm_access_callback_set(ai, ELM_ACCESS_INFO, _access_info_cb, it);
        _elm_access_callback_set(ai, ELM_ACCESS_STATE, _access_state_cb, it);
        _elm_access_on_highlight_hook_set(ai, _access_on_highlight_cb, it);
        _elm_access_activate_callback_set(ai, _access_activate_cb, it);
     }
}

static Elm_List_Item *
_item_new(Evas_Object *obj,
          const char *label,
          Evas_Object *icon,
          Evas_Object *end,
          Evas_Smart_Cb func,
          const void *data)
{
   Elm_List_Item *it;

   ELM_LIST_DATA_GET(obj, sd);

   it = elm_widget_item_new(obj, Elm_List_Item);
   if (!it) return NULL;

   it->sd = sd;
   it->label = eina_stringshare_add(label);
   it->icon = icon;
   it->end = end;
   it->func = func;
   it->base.data = data;

   VIEW(it) = edje_object_add(evas_object_evas_get(obj));

   /* access */
   if (_elm_config->access_mode == ELM_ACCESS_MODE_ON)
     _access_widget_item_register(it, EINA_TRUE);

   edje_object_mirrored_set(VIEW(it), elm_widget_mirrored_get(obj));
   evas_object_event_callback_add
     (VIEW(it), EVAS_CALLBACK_MOUSE_DOWN, _mouse_down_cb, it);
   evas_object_event_callback_add
     (VIEW(it), EVAS_CALLBACK_MOUSE_UP, _mouse_up_cb, it);
   evas_object_event_callback_add
     (VIEW(it), EVAS_CALLBACK_MOUSE_MOVE, _mouse_move_cb, it);
   evas_object_size_hint_weight_set
     (VIEW(it), EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(VIEW(it), EVAS_HINT_FILL, EVAS_HINT_FILL);
   edje_object_mirrored_set(VIEW(it), elm_widget_mirrored_get(obj));

   if (it->icon)
     {
        elm_widget_sub_object_add(obj, it->icon);
        evas_object_event_callback_add
          (it->icon, EVAS_CALLBACK_CHANGED_SIZE_HINTS, _size_hints_changed_cb,
          obj);
     }
   if (it->end)
     {
        elm_widget_sub_object_add(obj, it->end);
        evas_object_event_callback_add
          (it->end, EVAS_CALLBACK_CHANGED_SIZE_HINTS, _size_hints_changed_cb,
          obj);
     }

   elm_widget_item_disable_hook_set(it, _item_disable_hook);
   elm_widget_item_content_set_hook_set(it, _item_content_set_hook);
   elm_widget_item_content_get_hook_set(it, _item_content_get_hook);
   elm_widget_item_content_unset_hook_set(it, _item_content_unset_hook);
   elm_widget_item_text_set_hook_set(it, _item_text_set_hook);
   elm_widget_item_text_get_hook_set(it, _item_text_get_hook);
   elm_widget_item_del_pre_hook_set(it, _item_del_pre_hook);
   elm_widget_item_signal_emit_hook_set(it, _item_signal_emit_hook);

   return it;
}

static void
_resize_cb(void *data,
           Evas *e __UNUSED__,
           Evas_Object *obj __UNUSED__,
           void *event_info __UNUSED__)
{
   elm_layout_sizing_eval(data);
}

static Eina_Bool
_elm_list_smart_focus_next(const Evas_Object *obj,
                           Elm_Focus_Direction dir,
                           Evas_Object **next)
{
   Eina_List *items = NULL;
   Eina_List *elist = NULL;
   Elm_List_Item *it;

   ELM_LIST_CHECK(obj) EINA_FALSE;
   ELM_LIST_DATA_GET(obj, sd);

   if (_elm_config->access_mode != ELM_ACCESS_MODE_ON) return EINA_FALSE;

   EINA_LIST_FOREACH(sd->items, elist, it)
     {
        items = eina_list_append(items, it->base.access_obj);
        if (it->icon) items = eina_list_append(items, it->icon);
        if (it->end) items = eina_list_append(items, it->end);
     }

   return elm_widget_focus_list_next_get
            (obj, items, eina_list_data_get, dir, next);
}

static void
_elm_list_smart_add(Evas_Object *obj)
{
   EVAS_SMART_DATA_ALLOC(obj, Elm_List_Smart_Data);

   ELM_WIDGET_CLASS(_elm_list_parent_sc)->base.add(obj);
}

static void
_elm_list_smart_del(Evas_Object *obj)
{
   const Eina_List *l;
   Elm_List_Item *it;

   ELM_LIST_DATA_GET(obj, sd);

   if (sd->walking)
     ERR("ERROR: list deleted while walking.\n");

   _item_unfocused((Elm_List_Item *)sd->focused);
   sd->delete_me = EINA_TRUE;
   EINA_LIST_FOREACH(sd->items, l, it)
     {
        if (it->icon)
          evas_object_event_callback_del
            (it->icon, EVAS_CALLBACK_CHANGED_SIZE_HINTS,
            _size_hints_changed_cb);
        if (it->end)
          evas_object_event_callback_del
            (it->end, EVAS_CALLBACK_CHANGED_SIZE_HINTS,
            _size_hints_changed_cb);
     }

   evas_object_event_callback_del
     (obj, EVAS_CALLBACK_CHANGED_SIZE_HINTS,
     _size_hints_changed_cb);
   evas_object_event_callback_del
     (sd->box, EVAS_CALLBACK_CHANGED_SIZE_HINTS, _size_hints_changed_cb);

   _elm_list_walk(sd);

   EINA_LIST_FREE (sd->items, it)
     {
        /* issuing free because of "locking" item del pre hook */
        _elm_list_item_free(it);
        elm_widget_item_free(it);
     }

   _elm_list_unwalk(sd);

   if (sd->to_delete)
     ERR("ERROR: leaking nodes!\n");

   eina_list_free(sd->selected);

   ELM_WIDGET_CLASS(_elm_list_parent_sc)->base.del(obj);
}

static void
_elm_list_smart_move(Evas_Object *obj,
                     Evas_Coord x,
                     Evas_Coord y)
{
   ELM_LIST_DATA_GET(obj, sd);

   ELM_WIDGET_CLASS(_elm_list_parent_sc)->base.move(obj, x, y);

   evas_object_move(sd->hit_rect, x, y);
}

static void
_elm_list_smart_resize(Evas_Object *obj,
                       Evas_Coord w,
                       Evas_Coord h)
{
   ELM_LIST_DATA_GET(obj, sd);

   ELM_WIDGET_CLASS(_elm_list_parent_sc)->base.resize(obj, w, h);

   evas_object_resize(sd->hit_rect, w, h);
}

static void
_elm_list_smart_member_add(Evas_Object *obj,
                           Evas_Object *member)
{
   ELM_LIST_DATA_GET(obj, sd);

   ELM_WIDGET_CLASS(_elm_list_parent_sc)->base.member_add(obj, member);

   if (sd->hit_rect)
     evas_object_raise(sd->hit_rect);
}

static void
_elm_list_smart_access(Evas_Object *obj, Eina_Bool is_access)
{
   Eina_List *elist = NULL;
   Elm_List_Item *it;

   ELM_LIST_DATA_GET(obj, sd);

   if (is_access)
     ELM_WIDGET_CLASS(ELM_WIDGET_DATA(sd)->api)->focus_next =
        _elm_list_smart_focus_next;
   else
     ELM_WIDGET_CLASS(ELM_WIDGET_DATA(sd)->api)->focus_next = NULL;

   EINA_LIST_FOREACH(sd->items, elist, it)
     _access_widget_item_register(it, is_access);

   evas_object_smart_callback_call(obj, SIG_ACCESS_CHANGED, NULL);
}

static void
_elm_list_smart_set_user(Elm_List_Smart_Class *sc)
{
   ELM_WIDGET_CLASS(sc)->base.add = _elm_list_smart_add;
   ELM_WIDGET_CLASS(sc)->base.del = _elm_list_smart_del;
   ELM_WIDGET_CLASS(sc)->base.move = _elm_list_smart_move;
   ELM_WIDGET_CLASS(sc)->base.resize = _elm_list_smart_resize;
   ELM_WIDGET_CLASS(sc)->base.member_add = _elm_list_smart_member_add;

   ELM_WIDGET_CLASS(sc)->sub_object_del = _elm_list_smart_sub_object_del;
   ELM_WIDGET_CLASS(sc)->on_focus = _elm_list_smart_on_focus;
   ELM_WIDGET_CLASS(sc)->focus_next = NULL;
   ELM_WIDGET_CLASS(sc)->focus_direction_manager_is = NULL;
   ELM_WIDGET_CLASS(sc)->focus_direction = NULL;
   ELM_WIDGET_CLASS(sc)->theme = _elm_list_smart_theme;
   ELM_WIDGET_CLASS(sc)->disable = _elm_list_smart_disable;
   ELM_WIDGET_CLASS(sc)->event = _elm_list_smart_event;
   ELM_WIDGET_CLASS(sc)->translate = _elm_list_smart_translate;
   ELM_WIDGET_CLASS(sc)->access = _elm_list_smart_access;

   ELM_LAYOUT_CLASS(sc)->sizing_eval = _elm_list_smart_sizing_eval;

   if (_elm_config->access_mode == ELM_ACCESS_MODE_ON)
     ELM_WIDGET_CLASS(sc)->focus_next = _elm_list_smart_focus_next;
}

EAPI const Elm_List_Smart_Class *
elm_list_smart_class_get(void)
{
   static Elm_List_Smart_Class _sc =
     ELM_LIST_SMART_CLASS_INIT_NAME_VERSION(ELM_LIST_SMART_NAME);
   static const Elm_List_Smart_Class *class = NULL;
   Evas_Smart_Class *esc = (Evas_Smart_Class *)&_sc;

   if (class)
     return class;

   _elm_list_smart_set(&_sc);
   esc->callbacks = _smart_callbacks;
   class = &_sc;

   return class;
}

EAPI Evas_Object *
elm_list_add(Evas_Object *parent)
{
   Evas_Object *obj;
   Evas_Coord minw, minh;

   EINA_SAFETY_ON_NULL_RETURN_VAL(parent, NULL);

   obj = elm_widget_add(_elm_list_smart_class_new(), parent);
   if (!obj) return NULL;

   if (!elm_widget_sub_object_add(parent, obj))
     ERR("could not add %p as sub object of %p", obj, parent);

   ELM_LIST_DATA_GET(obj, sd);

   elm_widget_can_focus_set(obj, EINA_TRUE);

   sd->mode = ELM_LIST_SCROLL;

   elm_layout_theme_set(obj, "list", "base", elm_widget_style_get(obj));

   sd->hit_rect = evas_object_rectangle_add(evas_object_evas_get(obj));
   evas_object_data_set(sd->hit_rect, "_elm_leaveme", obj);
   evas_object_smart_member_add(sd->hit_rect, obj);
   elm_widget_sub_object_add(obj, sd->hit_rect);

   /* common scroller hit rectangle setup */
   evas_object_color_set(sd->hit_rect, 0, 0, 0, 0);
   evas_object_show(sd->hit_rect);
   evas_object_repeat_events_set(sd->hit_rect, EINA_TRUE);

   sd->s_iface = evas_object_smart_interface_get
       (obj, ELM_SCROLLABLE_IFACE_NAME);

   sd->s_iface->animate_start_cb_set(obj, _scroll_animate_start_cb);
   sd->s_iface->animate_stop_cb_set(obj, _scroll_animate_stop_cb);
   sd->s_iface->scroll_cb_set(obj, _scroll_cb);
   sd->s_iface->drag_start_cb_set(obj, _scroll_drag_start_cb);
   sd->s_iface->drag_stop_cb_set(obj, _scroll_drag_stop_cb);
   sd->s_iface->edge_left_cb_set(obj, _edge_left_cb);
   sd->s_iface->edge_right_cb_set(obj, _edge_right_cb);
   sd->s_iface->edge_top_cb_set(obj, _edge_top_cb);
   sd->s_iface->edge_bottom_cb_set(obj, _edge_bottom_cb);

   sd->s_iface->content_min_limit_cb_set
     (obj, _elm_list_content_min_limit_cb);

   sd->s_iface->objects_set
     (obj, ELM_WIDGET_DATA(sd)->resize_obj, sd->hit_rect);

   /* the scrollable interface may set this */
   evas_object_event_callback_add
     (ELM_WIDGET_DATA(sd)->resize_obj, EVAS_CALLBACK_CHANGED_SIZE_HINTS,
     _size_hints_changed_cb, obj);

   edje_object_size_min_calc
     (ELM_WIDGET_DATA(sd)->resize_obj, &minw, &minh);
   evas_object_size_hint_min_set(obj, minw, minh);
   evas_object_event_callback_add(obj, EVAS_CALLBACK_RESIZE, _resize_cb, obj);

   sd->s_iface->bounce_allow_set
     (obj, EINA_FALSE, _elm_config->thumbscroll_bounce_enable);

   sd->box = elm_box_add(obj);
   elm_box_homogeneous_set(sd->box, EINA_TRUE);
   evas_object_size_hint_weight_set(sd->box, EVAS_HINT_EXPAND, 0.0);
   evas_object_size_hint_align_set(sd->box, EVAS_HINT_FILL, 0.0);

   /* FIXME: change this ugly code path later */
   elm_widget_on_show_region_hook_set(sd->box, _show_region_hook, obj);
   elm_widget_sub_object_add(obj, sd->box);

   sd->s_iface->content_set(obj, sd->box);
   evas_object_event_callback_add
     (sd->box, EVAS_CALLBACK_CHANGED_SIZE_HINTS,
     _size_hints_changed_cb, obj);
   const char *str = edje_object_data_get(ELM_WIDGET_DATA(sd)->resize_obj,
                                          "focus_highlight");
   if ((str) && (!strcmp(str, "on")))
      elm_widget_highlight_in_theme_set(obj, EINA_TRUE);
   else
      elm_widget_highlight_in_theme_set(obj, EINA_FALSE);
   sd->select_on_focus_enabled = EINA_FALSE;
   elm_widget_signal_callback_add(obj, "elm,action,focus_highlight,hide", "elm", _elm_list_focus_highlight_hide, obj);
   elm_widget_signal_callback_add(obj, "elm,action,focus_highlight,show", "elm", _elm_list_focus_highlight_show, obj);

   //Tizen Only: This should be removed when eo is applied.
   ELM_WIDGET_DATA_GET(obj, wsd);
   wsd->on_create = EINA_FALSE;

   return obj;
}

EAPI void
elm_list_go(Evas_Object *obj)
{
   ELM_LIST_CHECK(obj);

   _items_fix(obj);
}

EAPI void
elm_list_multi_select_set(Evas_Object *obj,
                          Eina_Bool multi)
{
   ELM_LIST_CHECK(obj);
   ELM_LIST_DATA_GET(obj, sd);

   sd->multi = !!multi;
   if (!sd->multi && sd->selected)
     {
        Eina_List *l, *ll;
        Elm_List_Item *it;
        Elm_List_Item *last = sd->selected->data;
        EINA_LIST_FOREACH_SAFE(sd->selected, l, ll, it)
           if (last != it) _item_unselect(it);
     }
}

EAPI Eina_Bool
elm_list_multi_select_get(const Evas_Object *obj)
{
   ELM_LIST_CHECK(obj) EINA_FALSE;
   ELM_LIST_DATA_GET(obj, sd);

   return sd->multi;
}

EAPI void
elm_list_mode_set(Evas_Object *obj,
                  Elm_List_Mode mode)
{
   ELM_LIST_CHECK(obj);
   ELM_LIST_DATA_GET(obj, sd);

   if (sd->mode == mode)
     return;

   sd->mode = mode;
   sd->scr_minw = EINA_FALSE;
   sd->scr_minh = EINA_FALSE;

   _elm_list_mode_set_internal(sd);
}

EAPI Elm_List_Mode
elm_list_mode_get(const Evas_Object *obj)
{
   ELM_LIST_CHECK(obj) ELM_LIST_LAST;
   ELM_LIST_DATA_GET(obj, sd);

   return sd->mode;
}

EAPI void
elm_list_horizontal_set(Evas_Object *obj,
                        Eina_Bool horizontal)
{
   Eina_Bool bounce = _elm_config->thumbscroll_bounce_enable;

   ELM_LIST_CHECK(obj);
   ELM_LIST_DATA_GET(obj, sd);

   horizontal = !!horizontal;

   if (sd->h_mode == horizontal)
     return;

   sd->h_mode = horizontal;
   elm_box_horizontal_set(sd->box, horizontal);

   if (horizontal)
     {
        evas_object_size_hint_weight_set(sd->box, 0.0, EVAS_HINT_EXPAND);
        evas_object_size_hint_align_set(sd->box, 0.0, EVAS_HINT_FILL);
        sd->s_iface->bounce_allow_set(obj, bounce, EINA_FALSE);
     }
   else
     {
        evas_object_size_hint_weight_set(sd->box, EVAS_HINT_EXPAND, 0.0);
        evas_object_size_hint_align_set(sd->box, EVAS_HINT_FILL, 0.0);
        sd->s_iface->bounce_allow_set(obj, EINA_FALSE, bounce);
     }

   _elm_list_mode_set_internal(sd);
}

EAPI Eina_Bool
elm_list_horizontal_get(const Evas_Object *obj)
{
   ELM_LIST_CHECK(obj) EINA_FALSE;
   ELM_LIST_DATA_GET(obj, sd);

   return sd->h_mode;
}

EAPI void
elm_list_select_mode_set(Evas_Object *obj,
                         Elm_Object_Select_Mode mode)
{
   ELM_LIST_CHECK(obj);
   ELM_LIST_DATA_GET(obj, sd);

   if (mode >= ELM_OBJECT_SELECT_MODE_MAX || sd->select_mode == mode)
     return;

   sd->select_mode = mode;

   if (sd->select_mode == ELM_OBJECT_SELECT_MODE_NONE)
     {
        Eina_List *l, *ll;
        Elm_List_Item *it;
        EINA_LIST_FOREACH_SAFE(sd->selected, l, ll, it)
          _item_unselect(it);
     }
}

EAPI Elm_Object_Select_Mode
elm_list_select_mode_get(const Evas_Object *obj)
{
   ELM_LIST_CHECK(obj) ELM_OBJECT_SELECT_MODE_MAX;
   ELM_LIST_DATA_GET(obj, sd);

   return sd->select_mode;
}

EAPI void
elm_list_bounce_set(Evas_Object *obj,
                    Eina_Bool h_bounce,
                    Eina_Bool v_bounce)
{
   ELM_LIST_CHECK(obj);
   ELM_LIST_DATA_GET(obj, sd);

   sd->s_iface->bounce_allow_set(obj, h_bounce, v_bounce);
}

EAPI void
elm_list_bounce_get(const Evas_Object *obj,
                    Eina_Bool *h_bounce,
                    Eina_Bool *v_bounce)
{
   ELM_LIST_CHECK(obj);
   ELM_LIST_DATA_GET(obj, sd);

   sd->s_iface->bounce_allow_get(obj, h_bounce, v_bounce);
}

EAPI void
elm_list_scroller_policy_set(Evas_Object *obj,
                             Elm_Scroller_Policy policy_h,
                             Elm_Scroller_Policy policy_v)
{
   ELM_LIST_CHECK(obj);
   ELM_LIST_DATA_GET(obj, sd);

   if ((policy_h >= ELM_SCROLLER_POLICY_LAST) ||
       (policy_v >= ELM_SCROLLER_POLICY_LAST))
     return;

   sd->s_iface->policy_set(obj, policy_h, policy_v);
}

EAPI void
elm_list_scroller_policy_get(const Evas_Object *obj,
                             Elm_Scroller_Policy *policy_h,
                             Elm_Scroller_Policy *policy_v)
{
   ELM_LIST_CHECK(obj);
   ELM_LIST_DATA_GET(obj, sd);

   sd->s_iface->policy_get(obj, policy_h, policy_v);
}

EAPI void
elm_list_clear(Evas_Object *obj)
{
   Elm_List_Item *it;

   ELM_LIST_CHECK(obj);
   ELM_LIST_DATA_GET(obj, sd);

   if (!sd->items) return;

   eina_list_free(sd->selected);
   sd->selected = NULL;

   if (sd->walking > 0)
     {
        Eina_List *n;

        EINA_LIST_FOREACH(sd->items, n, it)
          {
             if (it->deleted) continue;
             it->deleted = EINA_TRUE;
             sd->to_delete = eina_list_append(sd->to_delete, it);
          }
        return;
     }

   evas_object_ref(obj);

   _elm_list_walk(sd);

   EINA_LIST_FREE (sd->items, it)
     {
        /* issuing free because of "locking" item del pre hook */
        _elm_list_item_free(it);
        elm_widget_item_free(it);
     }

   _elm_list_unwalk(sd);

   _items_fix(obj);
   elm_layout_sizing_eval(obj);

   evas_object_unref(obj);
}

EAPI const Eina_List *
elm_list_items_get(const Evas_Object *obj)
{
   ELM_LIST_CHECK(obj) NULL;
   ELM_LIST_DATA_GET(obj, sd);

   return sd->items;
}

EAPI Elm_Object_Item *
elm_list_selected_item_get(const Evas_Object *obj)
{
   ELM_LIST_CHECK(obj) NULL;
   ELM_LIST_DATA_GET(obj, sd);

   if (sd->selected) return (Elm_Object_Item *)sd->selected->data;

   return NULL;
}

EAPI const Eina_List *
elm_list_selected_items_get(const Evas_Object *obj)
{
   ELM_LIST_CHECK(obj) NULL;
   ELM_LIST_DATA_GET(obj, sd);

   return sd->selected;
}

EAPI Elm_Object_Item *
elm_list_item_append(Evas_Object *obj,
                     const char *label,
                     Evas_Object *icon,
                     Evas_Object *end,
                     Evas_Smart_Cb func,
                     const void *data)
{
   Elm_List_Item *it;

   ELM_LIST_CHECK(obj) NULL;
   ELM_LIST_DATA_GET(obj, sd);

   it = _item_new(obj, label, icon, end, func, data);

   sd->items = eina_list_append(sd->items, it);
   it->node = eina_list_last(sd->items);
   elm_box_pack_end(sd->box, VIEW(it));

   return (Elm_Object_Item *)it;
}

EAPI Elm_Object_Item *
elm_list_item_prepend(Evas_Object *obj,
                      const char *label,
                      Evas_Object *icon,
                      Evas_Object *end,
                      Evas_Smart_Cb func,
                      const void *data)
{
   Elm_List_Item *it;

   ELM_LIST_CHECK(obj) NULL;
   ELM_LIST_DATA_GET(obj, sd);

   it = _item_new(obj, label, icon, end, func, data);

   sd->items = eina_list_prepend(sd->items, it);
   it->node = sd->items;
   elm_box_pack_start(sd->box, VIEW(it));

   return (Elm_Object_Item *)it;
}

EAPI Elm_Object_Item *
elm_list_item_insert_before(Evas_Object *obj,
                            Elm_Object_Item *before,
                            const char *label,
                            Evas_Object *icon,
                            Evas_Object *end,
                            Evas_Smart_Cb func,
                            const void *data)
{
   Elm_List_Item *it, *before_it;

   ELM_LIST_CHECK(obj) NULL;
   ELM_LIST_ITEM_CHECK_OR_RETURN(before, NULL);
   ELM_LIST_DATA_GET(obj, sd);

   before_it = (Elm_List_Item *)before;
   if (!before_it->node) return NULL;

   it = _item_new(obj, label, icon, end, func, data);
   sd->items = eina_list_prepend_relative_list(sd->items, it, before_it->node);
   it->node = before_it->node->prev;
   elm_box_pack_before(sd->box, VIEW(it), VIEW(before_it));

   return (Elm_Object_Item *)it;
}

EAPI Elm_Object_Item *
elm_list_item_insert_after(Evas_Object *obj,
                           Elm_Object_Item *after,
                           const char *label,
                           Evas_Object *icon,
                           Evas_Object *end,
                           Evas_Smart_Cb func,
                           const void *data)
{
   Elm_List_Item *it, *after_it;

   ELM_LIST_CHECK(obj) NULL;
   ELM_LIST_ITEM_CHECK_OR_RETURN(after, NULL);
   ELM_LIST_DATA_GET(obj, sd);

   after_it = (Elm_List_Item *)after;
   if (!after_it->node) return NULL;

   it = _item_new(obj, label, icon, end, func, data);
   sd->items = eina_list_append_relative_list(sd->items, it, after_it->node);
   it->node = after_it->node->next;
   elm_box_pack_after(sd->box, VIEW(it), VIEW(after_it));

   return (Elm_Object_Item *)it;
}

EAPI Elm_Object_Item *
elm_list_item_sorted_insert(Evas_Object *obj,
                            const char *label,
                            Evas_Object *icon,
                            Evas_Object *end,
                            Evas_Smart_Cb func,
                            const void *data,
                            Eina_Compare_Cb cmp_func)
{
   Eina_List *l;
   Elm_List_Item *it;

   ELM_LIST_CHECK(obj) NULL;
   ELM_LIST_DATA_GET(obj, sd);

   it = _item_new(obj, label, icon, end, func, data);

   sd->items = eina_list_sorted_insert(sd->items, cmp_func, it);
   l = eina_list_data_find_list(sd->items, it);
   l = eina_list_next(l);
   if (!l)
     {
        it->node = eina_list_last(sd->items);
        elm_box_pack_end(sd->box, VIEW(it));
     }
   else
     {
        Elm_List_Item *before = eina_list_data_get(l);

        it->node = before->node->prev;
        elm_box_pack_before(sd->box, VIEW(it), VIEW(before));
     }

   return (Elm_Object_Item *)it;
}

EAPI void
elm_list_item_separator_set(Elm_Object_Item *it,
                            Eina_Bool setting)
{
   ELM_LIST_ITEM_CHECK_OR_RETURN(it);

   ((Elm_List_Item *)it)->is_separator = !!setting;
}

EAPI Eina_Bool
elm_list_item_separator_get(const Elm_Object_Item *it)
{
   ELM_LIST_ITEM_CHECK_OR_RETURN(it, EINA_FALSE);

   return ((Elm_List_Item *)it)->is_separator;
}

EAPI void
elm_list_item_selected_set(Elm_Object_Item *it,
                           Eina_Bool selected)
{
   Elm_List_Item *item = (Elm_List_Item *)it;
   ELM_LIST_ITEM_CHECK_OR_RETURN(it);

   if (elm_widget_item_disabled_get(it)) return;

   if (item->highlight_timer)
     {
        ecore_timer_del(item->highlight_timer);
        item->highlight_timer = NULL;
     }
   if (item->unhighlight_timer)
     {
        ecore_timer_del(item->unhighlight_timer);
        item->unhighlight_timer = NULL;
     }

   // FIXME: This API has highlight/unhighlight feature also..
   if (selected) _item_highlight(item);
   else if (item->highlighted)
     {
        // FIXME: to show highlight effect long enough to show,
        // list use timer when iten uighlight.
        item->unhighlight_timer = ecore_timer_add(ITEM_UNHIGHLIGHT_TIMER,
                                                  _unhighlight_timer, item);
        return;
     }

   if (item->selected == selected) return;

   if (selected) _item_select(item);
   else _item_unselect(item);
}

EAPI Eina_Bool
elm_list_item_selected_get(const Elm_Object_Item *it)
{
   ELM_LIST_ITEM_CHECK_OR_RETURN(it, EINA_FALSE);

   return ((Elm_List_Item *)it)->selected;
}

EAPI void
elm_list_item_show(Elm_Object_Item *it)
{
   Evas_Coord bx, by, bw, bh;
   Evas_Coord x, y, w, h;

   ELM_LIST_ITEM_CHECK_OR_RETURN(it);
   ELM_LIST_DATA_GET(WIDGET(it), sd);

   evas_smart_objects_calculate(evas_object_evas_get(sd->box));
   evas_object_geometry_get(sd->box, &bx, &by, &bw, &bh);
   evas_object_geometry_get(VIEW(it), &x, &y, &w, &h);
   x -= bx;
   y -= by;

   sd->s_iface->content_region_show(WIDGET(it), x, y, w, h);
}

EAPI void
elm_list_item_bring_in(Elm_Object_Item *it)
{
   Evas_Coord bx, by, bw, bh;
   Evas_Coord x, y, w, h;

   ELM_LIST_ITEM_CHECK_OR_RETURN(it);
   ELM_LIST_DATA_GET(WIDGET(it), sd);

   evas_smart_objects_calculate(evas_object_evas_get(sd->box));
   evas_object_geometry_get(sd->box, &bx, &by, &bw, &bh);
   evas_object_geometry_get(VIEW(it), &x, &y, &w, &h);
   x -= bx;
   y -= by;

   sd->s_iface->region_bring_in(WIDGET(it), x, y, w, h);
}

EAPI Evas_Object *
elm_list_item_object_get(const Elm_Object_Item *it)
{
   ELM_LIST_ITEM_CHECK_OR_RETURN(it, NULL);

   return VIEW(it);
}

EAPI Elm_Object_Item *
elm_list_item_prev(const Elm_Object_Item *it)
{
   Elm_List_Item *item = (Elm_List_Item *)it;

   ELM_LIST_ITEM_CHECK_OR_RETURN(it, NULL);

   if (item->node->prev) return item->node->prev->data;
   else return NULL;
}

EAPI Elm_Object_Item *
elm_list_item_next(const Elm_Object_Item *it)
{
   Elm_List_Item *item = (Elm_List_Item *)it;

   ELM_LIST_ITEM_CHECK_OR_RETURN(it, NULL);

   if (item->node->next) return item->node->next->data;
   else return NULL;
}

EAPI Elm_Object_Item *
elm_list_first_item_get(const Evas_Object *obj)
{
   ELM_LIST_CHECK(obj) NULL;
   ELM_LIST_DATA_GET(obj, sd);

   if (!sd->items) return NULL;

   return eina_list_data_get(sd->items);
}

EAPI Elm_Object_Item *
elm_list_last_item_get(const Evas_Object *obj)
{
   ELM_LIST_CHECK(obj) NULL;
   ELM_LIST_DATA_GET(obj, sd);

   if (!sd->items) return NULL;

   return eina_list_data_get(eina_list_last(sd->items));
}

EAPI Elm_Object_Item *
elm_list_at_xy_item_get(const Evas_Object *obj,
                           Evas_Coord x,
                           Evas_Coord y,
                           int *posret)
{
   ELM_LIST_CHECK(obj) NULL;
   ELM_LIST_DATA_GET(obj, sd);
   Eina_List *l;
   Elm_List_Item *it;
   Evas_Coord lasty;
   evas_object_geometry_get(sd->hit_rect, &lasty, NULL, NULL, NULL);

   EINA_LIST_FOREACH(sd->items, l, it)
     {
        Evas_Coord itx, ity;
        Evas_Object *vit = VIEW(it);
        Evas_Coord vx, vy, vw, vh;
        evas_object_geometry_get(vit, &vx, &vy, &vw, &vh);

        itx = vx;
        ity = vy;
        if (ELM_RECTS_INTERSECT
              (itx, ity, vw, vh, x, y, 1, 1))
          {
             if (posret)
               {
                  if (y <= (ity + (vh / 4))) *posret = -1;
                  else if (y >= (ity + vh - (vh / 4)))
                    *posret = 1;
                  else *posret = 0;
               }

             return (Elm_Object_Item *) it;
          }

        lasty = ity + vh;
     }

   if (posret)
     {
        if (y > lasty) *posret = 1;
        else *posret = -1;
     }

   return NULL;
}
