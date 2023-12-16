#include <Elementary.h>
#include "elm_priv.h"
#include "elm_widget_toolbar.h"

EAPI const char ELM_TOOLBAR_SMART_NAME[] = "elm_toolbar";

#define ELM_TOOLBAR_ITEM_FROM_INLIST(item) \
  ((item) ? EINA_INLIST_CONTAINER_GET(item, Elm_Toolbar_Item) : NULL)

static const char SIG_SCROLL[] = "scroll";
static const char SIG_SCROLL_ANIM_START[] = "scroll,anim,start";
static const char SIG_SCROLL_ANIM_STOP[] = "scroll,anim,stop";
static const char SIG_SCROLL_DRAG_START[] = "scroll,drag,start";
static const char SIG_SCROLL_DRAG_STOP[] = "scroll,drag,stop";
static const char SIG_CLICKED[] = "clicked";
static const char SIG_LONGPRESSED[] = "longpressed";
static const char SIG_CLICKED_DOUBLE[] = "clicked,double";
static const char SIG_LANG_CHANGED[] = "language,changed";
static const char SIG_ACCESS_CHANGED[] = "access,changed";
static const Evas_Smart_Cb_Description _smart_callbacks[] = {
   {SIG_SCROLL, ""},
   {SIG_SCROLL_ANIM_START, ""},
   {SIG_SCROLL_ANIM_STOP, ""},
   {SIG_SCROLL_DRAG_START, ""},
   {SIG_SCROLL_DRAG_STOP, ""},
   {SIG_CLICKED, ""},
   {SIG_LONGPRESSED, ""},
   {SIG_CLICKED_DOUBLE, ""},
   {SIG_LANG_CHANGED, ""},
   {SIG_ACCESS_CHANGED, ""},
   {NULL, NULL}
};

static const Evas_Smart_Interface *_smart_interfaces[] =
{
   (Evas_Smart_Interface *)&ELM_SCROLLABLE_IFACE, NULL
};

EVAS_SMART_SUBCLASS_IFACE_NEW
  (ELM_TOOLBAR_SMART_NAME, _elm_toolbar, Elm_Toolbar_Smart_Class,
  Elm_Widget_Smart_Class, elm_widget_smart_class_get, _smart_callbacks,
  _smart_interfaces);

static void _item_select(Elm_Toolbar_Item *it);
static void _sizing_eval(Evas_Object *obj);
static Elm_Toolbar_Item *_highlight_next_item_get(Evas_Object *obj,
                                                  Evas_Object *box,
                                                  Eina_Bool reverse);
static int
_toolbar_item_prio_compare_cb(const void *i1,
                              const void *i2)
{
   const Elm_Toolbar_Item *eti1 = i1;
   const Elm_Toolbar_Item *eti2 = i2;

   if (!eti2) return 1;
   if (!eti1) return -1;

   if (eti2->prio.priority == eti1->prio.priority)
     return -1;

   return eti2->prio.priority - eti1->prio.priority;
}

static void
_items_visibility_fix(Elm_Toolbar_Smart_Data *sd,
                      Evas_Coord *iw,
                      Evas_Coord vw,
                      Eina_Bool *more)
{
   Elm_Toolbar_Item *it, *prev;
   Evas_Coord ciw = 0, cih = 0;
   Eina_List *sorted = NULL;
   int count = 0, i = 0;

   *more = EINA_FALSE;

   EINA_INLIST_FOREACH(sd->items, it)
     {
        if (it->separator)
          {
             prev = ELM_TOOLBAR_ITEM_FROM_INLIST(EINA_INLIST_GET(it)->prev);
             if (prev) it->prio.priority = prev->prio.priority;
          }
     }

   EINA_INLIST_FOREACH(sd->items, it)
     {
        sorted = eina_list_sorted_insert
            (sorted, _toolbar_item_prio_compare_cb, it);
     }

   if (sd->more_item)
     {
        evas_object_geometry_get(sd->VIEW(more_item), NULL, NULL, &ciw, &cih);
        if (sd->vertical) *iw += cih;
        else *iw += ciw;
     }

   EINA_LIST_FREE(sorted, it)
     {
        if (it->prio.priority > sd->standard_priority)
          {
             evas_object_geometry_get(VIEW(it), NULL, NULL, &ciw, &cih);
             if (sd->vertical) *iw += cih;
             else *iw += ciw;
             it->prio.visible = (*iw <= vw);
             it->in_box = sd->bx;
             if (!it->separator) count++;
          }
        else
          {
             it->prio.visible = EINA_FALSE;
             if (!it->separator) i++;
             if (i <= (count + 1))
               it->in_box = sd->bx_more;
             else
               it->in_box = sd->bx_more2;
             *more = EINA_TRUE;
          }
     }
}

static void
_item_menu_destroy(Elm_Toolbar_Item *item)
{
   if (item->o_menu)
     {
        evas_object_del(item->o_menu);
        item->o_menu = NULL;
     }
   item->menu = EINA_FALSE;
}

static void
_item_unselect(Elm_Toolbar_Item *item)
{
   if ((!item) || (!item->selected)) return;

   ELM_TOOLBAR_DATA_GET(WIDGET(item), sd);

   item->selected = EINA_FALSE;
   sd->selected_item = NULL;
   edje_object_signal_emit(VIEW(item), "elm,state,unselected", "elm");
   elm_widget_signal_emit(item->icon, "elm,state,unselected", "elm");
}

static void
_item_unhighlight(Elm_Toolbar_Item *item)
{
   if (!item) return;

   ELM_TOOLBAR_DATA_GET(WIDGET(item), sd);

   if (!sd->highlighted_item) return;

   if (item == sd->highlighted_item)
     {
        edje_object_signal_emit(VIEW(sd->highlighted_item), "elm,highlight,off", "elm");
        evas_object_smart_callback_call(WIDGET(item), "item,unfocused", sd->highlighted_item);
        sd->highlighted_item = NULL;
     }
}

static void
_menu_hide(void *data,
           Evas *e __UNUSED__,
           Evas_Object *obj __UNUSED__,
           void *event_info __UNUSED__)
{
   Elm_Toolbar_Item *selected;
   Elm_Toolbar_Item *it = data;

   selected = (Elm_Toolbar_Item *)elm_toolbar_selected_item_get(WIDGET(it));
   _item_unselect(selected);
}

static void
_menu_del(void *data,
          Evas *e __UNUSED__,
          Evas_Object *obj,
          void *event_info __UNUSED__)
{
   // avoid hide being emitted during object deletion
   evas_object_event_callback_del_full
     (obj, EVAS_CALLBACK_HIDE, _menu_hide, data);
}

static void
_item_menu_create(Elm_Toolbar_Smart_Data *sd,
                  Elm_Toolbar_Item *item)
{
   item->o_menu = elm_menu_add(elm_widget_parent_get(WIDGET(item)));
   item->menu = EINA_TRUE;

   if (sd->menu_parent)
     elm_menu_parent_set(item->o_menu, sd->menu_parent);

   evas_object_event_callback_add
     (item->o_menu, EVAS_CALLBACK_HIDE, _menu_hide, item);
   evas_object_event_callback_add
     (item->o_menu, EVAS_CALLBACK_DEL, _menu_del, item);
}

static void
_elm_toolbar_item_menu_cb(void *data,
                          Evas_Object *obj __UNUSED__,
                          void *event_info __UNUSED__)
{
   Elm_Toolbar_Item *it = data;

   if (it->func) it->func((void *)(it->base.data), WIDGET(it), it);
}

static void
_item_show(Elm_Toolbar_Item *it)
{
   Evas_Coord x, y, w, h, bx, by;

   ELM_TOOLBAR_DATA_GET(WIDGET(it), sd);

   evas_object_geometry_get(sd->bx, &bx, &by, NULL, NULL);
   evas_object_geometry_get(VIEW(it), &x, &y, &w, &h);
   sd->s_iface->content_region_show
     (ELM_WIDGET_DATA(sd)->obj, x - bx, y - by, w, h);
}

static void
_item_mirrored_set(Evas_Object *obj __UNUSED__,
                   Elm_Toolbar_Item *it,
                   Eina_Bool mirrored)
{
   edje_object_mirrored_set(VIEW(it), mirrored);
   if (it->o_menu) elm_widget_mirrored_set(it->o_menu, mirrored);
}

static void
_mirrored_set(Evas_Object *obj,
              Eina_Bool mirrored)
{
   Elm_Toolbar_Item *it;

   ELM_TOOLBAR_DATA_GET(obj, sd);

   EINA_INLIST_FOREACH(sd->items, it)
     _item_mirrored_set(obj, it, mirrored);
   if (sd->more_item)
     _item_mirrored_set(obj, sd->more_item, mirrored);
}

static void
_items_size_fit(Evas_Object *obj, Evas_Coord *bl, Evas_Coord view)
{
   Elm_Toolbar_Item *it, *prev;
   Eina_Bool full = EINA_FALSE, more = EINA_FALSE;
   Evas_Coord min, mw, mh;
   int sumf = 0, sumb = 0, prev_min = 0;

   ELM_TOOLBAR_DATA_GET(obj, sd);

   EINA_INLIST_FOREACH(sd->items, it)
     {
        min = mw = mh = -1;
        if (it->in_box && it->in_box == sd->bx)
          {
             if (!it->separator && !it->object)
               elm_coords_finger_size_adjust(1, &mw, 1, &mh);
             edje_object_size_min_restricted_calc(VIEW(it), &mw, &mh, mw, mh);
             if (!it->separator && !it->object)
               elm_coords_finger_size_adjust(1, &mw, 1, &mh);
          }
        else if (!more)
          {
             more = EINA_TRUE;
             elm_coords_finger_size_adjust(1, &mw, 1, &mh);
             edje_object_size_min_restricted_calc(sd->VIEW(more_item), &mw, &mh, mw, mh);
             elm_coords_finger_size_adjust(1, &mw, 1, &mh);
          }

        if (mw != -1 || mh != -1)
          {
             if (sd->vertical) min = mh;
             else min = mw;

             if ((!full) && ((sumf + min) > view))
               {
                  prev = ELM_TOOLBAR_ITEM_FROM_INLIST(EINA_INLIST_GET(it)->prev);
                  if (prev && prev->separator)
                    {
                       sumf -= prev_min;
                       sumb += prev_min;
                    }
                  full = EINA_TRUE;
               }

             if (!full) sumf += min;
             else sumb += min;
             prev_min = min;
          }
     }
   if (sumf != 0) *bl = (Evas_Coord)(((sumf + sumb) * view) / sumf);
}

static Eina_Bool
_elm_toolbar_item_coordinates_calc(Elm_Toolbar_Item *item,
                                   Elm_Toolbar_Item_Scrollto_Type type,
                                   Evas_Coord *x,
                                   Evas_Coord *y,
                                   Evas_Coord *w,
                                   Evas_Coord *h)
{
   Evas_Coord ix, iy, iw, ih, bx, by, vw, vh;

   ELM_TOOLBAR_DATA_GET(WIDGET(item), sd);

   sd->s_iface->content_viewport_size_get(WIDGET(item), &vw, &vh);
   evas_object_geometry_get(sd->bx, &bx, &by, NULL, NULL);
   evas_object_geometry_get(VIEW(item), &ix, &iy, &iw, &ih);

   switch (type)
     {
      case ELM_TOOLBAR_ITEM_SCROLLTO_IN:
         *x = ix - bx;
         *y = iy - by;
         *w = iw;
         *h = ih;
         break;

      case ELM_TOOLBAR_ITEM_SCROLLTO_FIRST:
         *x = ix - bx;
         *y = iy - by;
         *w = vw;
         *h = vh;
         break;

      case ELM_TOOLBAR_ITEM_SCROLLTO_MIDDLE:
         *x = ix - bx + (iw / 2) - (vw / 2);
         *y = iy - by + (ih / 2) - (vh / 2);
         *w = vw;
         *h = vh;
         break;

      case ELM_TOOLBAR_ITEM_SCROLLTO_LAST:
         *x = ix - bx + iw - vw;
         *y = iy - by + ih - vh;
         *w = vw;
         *h = vh;
         break;

      default:
         return EINA_FALSE;
     }

   return EINA_TRUE;
}

static void
_resize_job(void *data)
{
   Evas_Object *obj = (Evas_Object *)data;
   Evas_Coord mw, mh, vw = 0, vh = 0, w = 0, h = 0;
   Elm_Toolbar_Item *it = NULL;
   Evas_Object *o = NULL; // TIZEN ONLY
   Eina_List *l = NULL; // TIZEN ONLY
   Eina_List *list = NULL;
   Eina_Bool more = EINA_FALSE;

   ELM_TOOLBAR_DATA_GET(obj, sd);

   sd->resize_job = NULL;
   sd->s_iface->content_viewport_size_get(obj, &vw, &vh);
   evas_object_size_hint_min_get(sd->bx, &mw, &mh);
   evas_object_geometry_get(sd->bx, NULL, NULL, &w, &h);

   if (sd->shrink_mode == ELM_TOOLBAR_SHRINK_MENU)
     {
        Evas_Coord iw = 0, ih = 0, more_w = 0, more_h = 0;

        if (sd->vertical)
          {
             h = vh;
             _items_visibility_fix(sd, &ih, vh, &more);
          }
        else
          {
             w = vw;
             _items_visibility_fix(sd, &iw, vw, &more);
          }
        evas_object_geometry_get
          (sd->VIEW(more_item), NULL, NULL, &more_w, &more_h);

        if (sd->vertical)
          {
             if ((ih - more_h) <= vh) ih -= more_h;
          }
        else
          {
             if ((iw - more_w) <= vw) iw -= more_w;
          }

        /* All items are removed from the box object, since removing
         * individual items won't trigger a resize. Items are be
         * readded below. */
        evas_object_box_remove_all(sd->bx, EINA_FALSE);
        if (((sd->vertical) && (ih > vh)) ||
            ((!sd->vertical) && (iw > vw)) || more)
          {
             Evas_Object *menu;

             _item_menu_destroy(sd->more_item);
             _item_menu_create(sd, sd->more_item);
             menu =
               elm_toolbar_item_menu_get((Elm_Object_Item *)sd->more_item);
             EINA_INLIST_FOREACH(sd->items, it)
               {
                  if (!it->prio.visible)
                    {
                       if (it->separator)
                         elm_menu_item_separator_add(menu, NULL);
                       else
                         {
                            Elm_Object_Item *menu_it;

                            menu_it = elm_menu_item_add
                                (menu, NULL, it->icon_str, it->label,
                                _elm_toolbar_item_menu_cb, it);
                            elm_object_item_disabled_set
                              (menu_it, elm_widget_item_disabled_get(it));
                            if (it->o_menu)
                              elm_menu_clone(it->o_menu, menu, menu_it);
                         }
                       evas_object_hide(VIEW(it));
                    }
                  else
                    {
                       evas_object_box_append(sd->bx, VIEW(it));
                       evas_object_show(VIEW(it));
                    }
               }
             evas_object_box_append(sd->bx, sd->VIEW(more_item));
             evas_object_show(sd->VIEW(more_item));
          }
        else
          {
             /* All items are visible, show them all (except for the
              * "More" button, of course). */
             EINA_INLIST_FOREACH(sd->items, it)
               {
                  evas_object_show(VIEW(it));
                  evas_object_box_append(sd->bx, VIEW(it));
               }
             evas_object_hide(sd->VIEW(more_item));
          }
     }
   else if (sd->shrink_mode == ELM_TOOLBAR_SHRINK_HIDE)
     {
        Evas_Coord iw = 0, ih = 0;

        if (sd->vertical)
          {
             h = vh;
             _items_visibility_fix(sd, &ih, vh, &more);
          }
        else
          {
             w = vw;
             _items_visibility_fix(sd, &iw, vw, &more);
          }
        evas_object_box_remove_all(sd->bx, EINA_FALSE);
        if (((sd->vertical) && (ih > vh)) ||
            ((!sd->vertical) && (iw > vw)) || more)
          {
             EINA_INLIST_FOREACH(sd->items, it)
               {
                  if (!it->prio.visible)
                    evas_object_hide(VIEW(it));
                  else
                    {
                       evas_object_box_append(sd->bx, VIEW(it));
                       evas_object_show(VIEW(it));
                    }
               }
          }
        else
          {
             /* All items are visible, show them all */
             EINA_INLIST_FOREACH(sd->items, it)
               {
                  evas_object_show(VIEW(it));
                  evas_object_box_append(sd->bx, VIEW(it));
               }
          }
     }
   else if (sd->shrink_mode == ELM_TOOLBAR_SHRINK_EXPAND)
     {
        Evas_Coord iw = 0, ih = 0;

        if (sd->vertical)
          h = (vh >= mh) ? vh : mh;
        else
          w = (vw >= mw) ? vw : mw;

        if (sd->vertical)
          _items_visibility_fix(sd, &ih, vh, &more);
        else
          _items_visibility_fix(sd, &iw, vw, &more);

        evas_object_box_remove_all(sd->bx, EINA_FALSE);
        evas_object_box_remove_all(sd->bx_more, EINA_FALSE);
        evas_object_box_remove_all(sd->bx_more2, EINA_FALSE);

        EINA_INLIST_FOREACH(sd->items, it)
          {
             if (it->in_box)
               {
                  evas_object_box_append(it->in_box, VIEW(it));
                  evas_object_show(VIEW(it));
               }
          }
        if (more)
          {
             evas_object_box_append(sd->bx, sd->VIEW(more_item));
             evas_object_show(sd->VIEW(more_item));
          }
        else
          evas_object_hide(sd->VIEW(more_item));

        if (sd->vertical)
          {
             if (h > vh) _items_size_fit(obj, &h, vh);
             if (sd->item_count - sd->separator_count > 0)
               sd->s_iface->paging_set(obj, 0.0, 0.0, 0, (h / (sd->item_count - sd->separator_count)));
          }
        else
          {
             if (w > vw) _items_size_fit(obj, &w, vw);
             if (sd->item_count - sd->separator_count > 0)
               sd->s_iface->paging_set(obj, 0.0, 0.0, (w / (sd->item_count - sd->separator_count)), 0);
          }
     }
   else
     {
        if (sd->vertical)
          h = (vh >= mh) ? vh : mh;
        else
          w = (vw >= mw) ? vw : mw;
        EINA_INLIST_FOREACH(sd->items, it)
          {
             if (it->selected)
               {
                  _item_show(it);
               }
             evas_object_show(VIEW(it));
          }
     }

   if (sd->transverse_expanded)
     {
        if (sd->vertical)
          w = vw;
        else
          h = vh;
     }

   evas_object_resize(sd->bx, w, h);

//// TIZEN ONLY
   list = evas_object_box_children_get(sd->bx);
   EINA_LIST_FOREACH(list, l, o)
     {
        if (o == eina_list_nth(list, 0))
          {
             edje_object_signal_emit(o, "elm,order,first,item", "elm");
             if (eina_list_count(list) == 1)
               edje_object_signal_emit(o, "elm,order,last,item", "elm");
          }
        else if (o == eina_list_nth(list, eina_list_count(list)-1))
          edje_object_signal_emit(o, "elm,order,last,item", "elm");
        else
          edje_object_signal_emit(o, "elm,order,default,item", "elm");
     }
   eina_list_free(list);
//

// Remove the first or last separator since it is not neccessary
   list = evas_object_box_children_get(sd->bx_more);
   EINA_INLIST_FOREACH(sd->items, it)
     {
        if (it->separator &&
            ((VIEW(it) == eina_list_data_get(list)) ||
             (VIEW(it) == eina_list_nth(list, eina_list_count(list) - 1))))
          {
             evas_object_box_remove(sd->bx_more, VIEW(it));
             evas_object_move(VIEW(it), -9999, -9999);
             evas_object_hide(VIEW(it));
          }
     }
   eina_list_free(list);

   list = evas_object_box_children_get(sd->bx_more2);
   EINA_INLIST_FOREACH(sd->items, it)
     {
        if (it->separator &&
            ((VIEW(it) == eina_list_data_get(list)) ||
             (VIEW(it) == eina_list_nth(list, eina_list_count(list) - 1))))
          {
             evas_object_box_remove(sd->bx_more2, VIEW(it));
             evas_object_move(VIEW(it), -9999, -9999);
             evas_object_hide(VIEW(it));
          }
     }
   eina_list_free(list);

   _mirrored_set(obj, elm_widget_mirrored_get(obj));
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
_elm_toolbar_smart_on_focus(Evas_Object *obj, Elm_Focus_Info *info)
{
   Elm_Toolbar_Item *it = NULL;
   Elm_Focus_Info *fi = info;
   Evas_Coord x, y, w, h;

   ELM_TOOLBAR_DATA_GET(obj, sd);

   if (elm_widget_focus_get(obj))
     {
        evas_object_focus_set(ELM_WIDGET_DATA(sd)->resize_obj, EINA_TRUE);
        // FIXME: There are applications which do not use elm_win as top widget.
        // This is workaround! Those could not use focus!
        if (_focus_enabled(obj))
          {
             if (sd->highlighted_item &&
                 (fi->dir == ELM_FOCUS_UP || fi->dir == ELM_FOCUS_DOWN))
               {
                  edje_object_signal_emit(VIEW(sd->highlighted_item), "elm,highlight,on", "elm");
                  evas_object_smart_callback_call(obj, "item,focused", sd->highlighted_item);
               }
             else
               {
                  if (fi->dir == ELM_FOCUS_NEXT || fi->dir == ELM_FOCUS_RIGHT ||
                      fi->dir == ELM_FOCUS_UP || fi->dir == ELM_FOCUS_DOWN)
                    {
                       sd->highlighted_item = NULL;
                       it = _highlight_next_item_get(obj, sd->bx, EINA_FALSE);
                    }
                  else if (fi->dir == ELM_FOCUS_LEFT || fi->dir == ELM_FOCUS_PREVIOUS)
                    {
                       sd->highlighted_item = NULL;
                       it = _highlight_next_item_get(obj, sd->bx, EINA_TRUE);
                    }
                  if(!it)
                    return EINA_FALSE;

                  sd->highlighted_item = it;
                  edje_object_signal_emit(VIEW(sd->highlighted_item), "elm,highlight,on", "elm");
                  evas_object_smart_callback_call(obj, "item,focused", sd->highlighted_item);
               }

             if (_elm_toolbar_item_coordinates_calc(
                   sd->highlighted_item, ELM_TOOLBAR_ITEM_SCROLLTO_IN, &x, &y, &w, &h))
               sd->s_iface->region_bring_in(obj, x, y, w, h);
          }
     }
   else
     {
        if (sd->highlighted_item)
          {
             edje_object_signal_emit(VIEW(sd->highlighted_item), "elm,highlight,off", "elm");
             evas_object_smart_callback_call(obj, "item,unfocused", sd->highlighted_item);
          }
     evas_object_focus_set(ELM_WIDGET_DATA(sd)->resize_obj, EINA_FALSE);
   }

   return EINA_TRUE;
}

static Elm_Toolbar_Item *
_highlight_next_item_get(Evas_Object *obj, Evas_Object *box, Eina_Bool reverse)
{
   ELM_TOOLBAR_DATA_GET(obj, sd);
   Eina_List *list = NULL;
   Elm_Toolbar_Item *it = NULL;
   Evas_Object *it_obj = NULL;

   list = evas_object_box_children_get(box);
   if (reverse)
     list = eina_list_reverse(list);

   if (sd->highlighted_item)
     {
        list = eina_list_data_find_list(list, VIEW(sd->highlighted_item));
        if (list) list = eina_list_next(list);
     }
   it_obj = eina_list_data_get(list);
   if (it_obj) it = evas_object_data_get(it_obj, "item");
   else it = NULL;

   while (it &&
          (it->separator ||
           elm_object_item_disabled_get((Elm_Object_Item *)it)))
     {
        if (list) list = eina_list_next(list);
        if (!list)
          {
             it = NULL;
             break;
          }
        it_obj = eina_list_data_get(list);
        if (it_obj) it = evas_object_data_get(it_obj, "item");
        else it = NULL;
     }
   eina_list_free(list);

   return it;
}

static Eina_Bool
_elm_toolbar_smart_event(Evas_Object *obj,
                         Evas_Object *src __UNUSED__,
                         Evas_Callback_Type type __UNUSED__,
                         void *event_info)
{
   (void) src;
   (void) type;
   Elm_Toolbar_Item *it = NULL;
   Evas_Coord x, y, w, h;

   ELM_TOOLBAR_DATA_GET(obj, sd);

   Evas_Event_Key_Down *ev = event_info;

   // TIZEN ONLY(20131221) : When access mode, focused ui is disabled.
   if (_elm_config->access_mode) return EINA_FALSE;

   if (elm_widget_disabled_get(obj)) return EINA_FALSE;
   if ((type != EVAS_CALLBACK_KEY_DOWN) && (type != EVAS_CALLBACK_KEY_UP))
     return EINA_FALSE;
   if (!sd->items) return EINA_FALSE;
   if (ev->event_flags & EVAS_EVENT_FLAG_ON_HOLD) return EINA_FALSE;
   if (!_focus_enabled(obj)) return EINA_FALSE;

   if ((!strcmp(ev->keyname, "Return")) ||
            ((!strcmp(ev->keyname, "KP_Enter")) && !ev->string))
     {
        if (type == EVAS_CALLBACK_KEY_DOWN && sd->highlighted_item)
          {
             edje_object_signal_emit(VIEW(sd->highlighted_item), "elm,action,pressed", "elm");
          }
        if (type == EVAS_CALLBACK_KEY_UP && sd->highlighted_item )
          {
             edje_object_signal_emit(VIEW(sd->highlighted_item), "elm,action,unpressed", "elm");
             _item_select(sd->highlighted_item);
          }
        ev->event_flags |= EVAS_EVENT_FLAG_ON_HOLD;
        return EINA_TRUE;
     }
#ifdef ELM_FOCUSED_UI
   else if ((!strcmp(ev->keyname, "Left")) ||
            ((!strcmp(ev->keyname, "KP_Left")) && !ev->string))
     {
        if (type != EVAS_CALLBACK_KEY_DOWN) return EINA_FALSE;
        if (!sd->vertical)
          it = _highlight_next_item_get(obj, sd->bx, EINA_TRUE);
        else
          return EINA_FALSE;
     }
   else if ((!strcmp(ev->keyname, "Right")) ||
            ((!strcmp(ev->keyname, "KP_Right")) && !ev->string))
     {
        if (type != EVAS_CALLBACK_KEY_DOWN) return EINA_FALSE;
        if (!sd->vertical)
          it = _highlight_next_item_get(obj, sd->bx, EINA_FALSE);
        else
          return EINA_FALSE;
     }
   else if ((!strcmp(ev->keyname, "Up")) ||
            ((!strcmp(ev->keyname, "KP_Up")) && !ev->string))
     {
        if (type != EVAS_CALLBACK_KEY_DOWN) return EINA_FALSE;
        if (sd->vertical)
          it = _highlight_next_item_get(obj, sd->bx, EINA_TRUE);
        else
          return EINA_FALSE;
     }
   else if ((!strcmp(ev->keyname, "Down")) ||
            ((!strcmp(ev->keyname, "KP_Down")) && !ev->string))
     {
        if (type != EVAS_CALLBACK_KEY_DOWN) return EINA_FALSE;
        if (sd->vertical)
          it = _highlight_next_item_get(obj, sd->bx, EINA_FALSE);
        else
          return EINA_FALSE;
     }
   else if (!strcmp(ev->keyname, "Tab"))
     {
        if (type != EVAS_CALLBACK_KEY_DOWN) return EINA_FALSE;
        if (evas_key_modifier_is_set(ev->modifiers, "Shift"))
          {
             if (!sd->vertical)
               it = _highlight_next_item_get(obj, sd->bx, EINA_TRUE);
             else
               return EINA_FALSE;
          }
        else
          {
             if (!sd->vertical)
               it = _highlight_next_item_get(obj, sd->bx, EINA_FALSE);
             else
               return EINA_FALSE;
          }
     }

   if (!it)
     return EINA_FALSE;

   if (sd->highlighted_item)
     {
        edje_object_signal_emit(VIEW(sd->highlighted_item), "elm,highlight,off", "elm");
        evas_object_smart_callback_call(obj, "item,unfocused", sd->highlighted_item);
     }
   sd->highlighted_item = it;
   edje_object_signal_emit(VIEW(sd->highlighted_item), "elm,highlight,on", "elm");
   evas_object_smart_callback_call(obj, "item,focused", sd->highlighted_item);

   if (_elm_toolbar_item_coordinates_calc(
         sd->highlighted_item, ELM_TOOLBAR_ITEM_SCROLLTO_IN, &x, &y, &w, &h))
     sd->s_iface->region_bring_in(obj, x, y, w, h);

   ev->event_flags |= EVAS_EVENT_FLAG_ON_HOLD;
   return EINA_TRUE;
#else
   return EINA_FALSE;
#endif
}

static void
_resizing_eval(Evas_Object *obj, Elm_Toolbar_Item *it)
{
   Evas_Coord x, y, h;
   Evas_Coord mw = -1, mh = -1;
   ELM_TOOLBAR_DATA_GET(obj, sd);

   evas_object_geometry_get(obj, &x, &y, NULL, &h);
   evas_object_move(sd->more, x, y + h);
   //calculate the size of each item
   if (it)
     {
        edje_object_size_min_restricted_calc(VIEW(it), &mw, &mh, mw, mh);
        if (!it->separator && !it->object)
          elm_coords_finger_size_adjust(1, &mw, 1, &mh);
        evas_object_size_hint_min_set(VIEW(it), mw, mh);
        evas_object_size_hint_max_set(VIEW(it), -1, -1);
     }
   else
     {
        if (sd->resize_job) ecore_job_del(sd->resize_job);
        // fit and fix the visibility
        sd->resize_job = ecore_job_add(_resize_job, obj);
     }
}

static void
_resize_cb(void *data,
           Evas *e __UNUSED__,
           Evas_Object *obj __UNUSED__,
           void *event_info __UNUSED__)
{
   _resizing_eval(data, NULL);
}

static void
_item_disable_hook(Elm_Object_Item *it)
{
   Elm_Toolbar_Item *toolbar_it = (Elm_Toolbar_Item *)it;

   if (elm_widget_item_disabled_get(toolbar_it))
     {
        edje_object_signal_emit(VIEW(toolbar_it), "elm,state,disabled", "elm");
        elm_widget_signal_emit(toolbar_it->icon, "elm,state,disabled", "elm");
     }
   else
     {
        edje_object_signal_emit(VIEW(toolbar_it), "elm,state,enabled", "elm");
        elm_widget_signal_emit(toolbar_it->icon, "elm,state,enabled", "elm");
     }

   _resizing_eval(WIDGET(toolbar_it), NULL);
}

static Eina_Bool
_item_icon_set(Evas_Object *icon_obj,
               const char *type,
               const char *icon)
{
   char icon_str[512];

   if ((!type) || (!*type)) goto end;
   if ((!icon) || (!*icon)) return EINA_FALSE;
   if ((snprintf(icon_str, sizeof(icon_str), "%s%s", type, icon) > 0)
       && (elm_icon_standard_set(icon_obj, icon_str)))
     return EINA_TRUE;
end:
   if (elm_icon_standard_set(icon_obj, icon))
     return EINA_TRUE;

   WRN("couldn't find icon definition for '%s'", icon);
   return EINA_FALSE;
}

static int
_elm_toolbar_icon_size_get(Elm_Toolbar_Smart_Data *sd)
{
   const char *icon_size = edje_object_data_get
       (ELM_WIDGET_DATA(sd)->resize_obj, "icon_size");

   if (icon_size)
     return atoi(icon_size) /
        edje_object_base_scale_get(ELM_WIDGET_DATA(sd)->resize_obj) *
        elm_config_scale_get();

   return _elm_config->icon_size;
}

static void
_menu_move_resize_cb(void *data,
                     Evas *e __UNUSED__,
                     Evas_Object *obj __UNUSED__,
                     void *event_info __UNUSED__)
{
   Elm_Toolbar_Item *it = data;
   Evas_Coord x, y, h;

   ELM_TOOLBAR_DATA_GET(WIDGET(it), sd);

   if (!sd->menu_parent) return;
   evas_object_geometry_get(VIEW(it), &x, &y, NULL, &h);
   elm_menu_move(it->o_menu, x, y + h);
}

static void
_item_select(Elm_Toolbar_Item *it)
{
   Elm_Toolbar_Item *it2;
   Evas_Object *obj2;
   Eina_Bool sel;
   Eina_List *list;

   ELM_TOOLBAR_DATA_GET(WIDGET(it), sd);

   if (elm_widget_item_disabled_get(it) || (it->separator) || (it->object))
     return;
   sel = it->selected;

   if (sd->select_mode != ELM_OBJECT_SELECT_MODE_NONE)
     {
        if (sel)
          {
             if (sd->shrink_mode == ELM_TOOLBAR_SHRINK_EXPAND)
               {
                  if (sd->more_item == it)
                    {
                       elm_layout_signal_emit
                         (sd->more, "elm,state,close", "elm");
                       _item_unselect(it);
                    }
               }
             if (sd->select_mode != ELM_OBJECT_SELECT_MODE_ALWAYS)
               _item_unselect(it);
          }
        else
          {
             it2 = (Elm_Toolbar_Item *)
               elm_toolbar_selected_item_get(WIDGET(it));
             _item_unselect(it2);

             it->selected = EINA_TRUE;
             sd->selected_item = it;
             if (sd->shrink_mode == ELM_TOOLBAR_SHRINK_EXPAND)
               {
                  if (sd->more_item == it)
                    {
                       list = evas_object_box_children_get(sd->bx_more2);
                       if (!list)
                         elm_layout_signal_emit
                           (sd->more, "elm,state,open", "elm");
                       else
                         elm_layout_signal_emit
                           (sd->more, "elm,state,open2", "elm");
                       eina_list_free(list);
                    }
                  else
                    {
                       if (it->in_box != sd->bx)
                         {
                            edje_object_signal_emit
                              (sd->VIEW(more_item), "elm,state,selected",
                              "elm");
                            elm_widget_signal_emit
                              (sd->more_item->icon, "elm,state,selected",
                              "elm");
                         }
                       else
                         {
                            edje_object_signal_emit
                              (sd->VIEW(more_item), "elm,state,unselected",
                              "elm");
                            elm_widget_signal_emit
                              (sd->more_item->icon, "elm,state,unselected",
                              "elm");
                         }
                       elm_layout_signal_emit
                         (sd->more, "elm,state,close", "elm");
                    }
               }
             edje_object_signal_emit(VIEW(it), "elm,state,selected", "elm");
             elm_widget_signal_emit(it->icon, "elm,state,selected", "elm");
             _item_show(it);
          }
     }

   obj2 = WIDGET(it);
   if (it->menu && (!sel))
     {
        evas_object_show(it->o_menu);
        evas_object_event_callback_add
          (VIEW(it), EVAS_CALLBACK_RESIZE, _menu_move_resize_cb, it);
        evas_object_event_callback_add
          (VIEW(it), EVAS_CALLBACK_MOVE, _menu_move_resize_cb, it);

        _menu_move_resize_cb(it, NULL, NULL, NULL);
     }

   if ((!sel) || (sd->select_mode == ELM_OBJECT_SELECT_MODE_ALWAYS))
     {
        if (it->func) it->func((void *)(it->base.data), WIDGET(it), it);
     }
   evas_object_smart_callback_call(obj2, SIG_CLICKED, it);
}

static void
_item_del(Elm_Toolbar_Item *it)
{
   Elm_Toolbar_Item_State *it_state;

   _item_unselect(it);
   _item_unhighlight(it);

   EINA_LIST_FREE(it->states, it_state)
     {
        if (it->icon == it_state->icon)
          it->icon = NULL;
        eina_stringshare_del(it_state->label);
        eina_stringshare_del(it_state->icon_str);
        if (it_state->icon) evas_object_del(it_state->icon);
        free(it_state);
     }

   eina_stringshare_del(it->label);
   if (it->label)
     edje_object_signal_emit(VIEW(it), "elm,state,text,hidden", "elm");
   eina_stringshare_del(it->icon_str);

   if (it->icon)
     {
        edje_object_signal_emit(VIEW(it), "elm,state,icon,hidden", "elm");
        evas_object_del(it->icon);
     }

   if (it->object) evas_object_del(it->object);
   //TODO: See if checking for sd->menu_parent is necessary before
   //deleting menu
   if (it->o_menu) evas_object_del(it->o_menu);
}

static void
_item_theme_hook(Evas_Object *obj,
                 Elm_Toolbar_Item *it,
                 double scale,
                 int icon_size)
{
   Evas_Coord mw = -1, mh = -1;
   Evas_Object *view = VIEW(it);
   const char *style;

   char buf[128];

   ELM_TOOLBAR_DATA_GET(obj, sd);

   style = elm_widget_style_get(obj);

   _item_mirrored_set(obj, it, elm_widget_mirrored_get(obj));
   edje_object_scale_set(view, scale);

   if (!it->separator && !it->object)
     {
        elm_widget_theme_object_set(obj, view, "toolbar", "item", style);
        if (it->selected)
          {
             edje_object_signal_emit(view, "elm,state,selected", "elm");
             elm_widget_signal_emit(it->icon, "elm,state,selected", "elm");
          }
        if (elm_widget_item_disabled_get(it))
          {
             edje_object_signal_emit(view, "elm,state,disabled", "elm");
             elm_widget_signal_emit(it->icon, "elm,state,disabled", "elm");
          }
        if (it->icon)
          {
             evas_object_size_hint_min_set(it->icon, icon_size, icon_size);
             evas_object_size_hint_max_set(it->icon, icon_size, icon_size);
             edje_object_part_swallow(view, "elm.swallow.icon", it->icon);
             edje_object_signal_emit
               (VIEW(it), "elm,state,icon,visible", "elm");
          }
        if (it->label)
          {
             edje_object_part_text_escaped_set(view, "elm.text", it->label);
             edje_object_signal_emit(VIEW(it), "elm,state,text,visible", "elm");
          }
     }
   else
     {
        if (!it->object)
          {
             elm_widget_theme_object_set
               (obj, view, "toolbar", "separator", style);
          }
        else
          {
             elm_widget_theme_object_set
               (obj, view, "toolbar", "object", style);
             edje_object_part_swallow(view, "elm.swallow.object", it->object);
          }
     }
   snprintf(buf, sizeof(buf), "elm,state,orient,%d", ELM_WIDGET_DATA(sd)->orient_mode);
   edje_object_signal_emit(view, buf, "elm");

    edje_object_message_signal_process(view);
    if (!it->separator && !it->object)
      elm_coords_finger_size_adjust(1, &mw, 1, &mh);
    if (sd->shrink_mode != ELM_TOOLBAR_SHRINK_EXPAND)
      {
         if (sd->vertical)
           {
              evas_object_size_hint_weight_set(view, EVAS_HINT_EXPAND, -1.0);
              evas_object_size_hint_align_set
                (view, EVAS_HINT_FILL, EVAS_HINT_FILL);
           }
         else
           {
              evas_object_size_hint_weight_set(VIEW(it), -1.0, EVAS_HINT_EXPAND);
              evas_object_size_hint_align_set
                (view, EVAS_HINT_FILL, EVAS_HINT_FILL);
           }
      }
    else
      {
         evas_object_size_hint_weight_set
           (VIEW(it), EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
         evas_object_size_hint_align_set
           (VIEW(it), EVAS_HINT_FILL, EVAS_HINT_FILL);
      }
    _resizing_eval(obj, it);
    _sizing_eval(obj);
}

static void
_inform_item_number(Evas_Object *obj)
{
   ELM_TOOLBAR_DATA_GET(obj, sd);
   Elm_Toolbar_Item *it;
   char buf[sizeof("elm,action,click,") + 3];
   static int scount = 0;
   int count = 0;

   EINA_INLIST_FOREACH(sd->items, it)
     if (!it->separator) count++;

   if (scount != count)
     {
        scount = count;
        sprintf(buf, "elm,number,item,%d", count);

        EINA_INLIST_FOREACH(sd->items, it)
          {
             if (!it->separator && !it->object)
               {
                  edje_object_signal_emit(VIEW(it), buf, "elm");
                  edje_object_message_signal_process(VIEW(it));
                  _resizing_eval(obj, it);
               }
          }
     }
}

static void
_sizing_eval(Evas_Object *obj)
{
   Evas_Coord minw = -1, minh = -1, minw_bx = -1, minh_bx = -1;
   Evas_Coord vw = 0, vh = 0;
   Evas_Coord w, h;

   ELM_TOOLBAR_DATA_GET(obj, sd);

   evas_object_smart_need_recalculate_set(sd->bx, EINA_TRUE);
   evas_object_smart_calculate(sd->bx);
   edje_object_size_min_calc(ELM_WIDGET_DATA(sd)->resize_obj, &minw, &minh);
   evas_object_geometry_get(obj, NULL, NULL, &w, &h);

   if (w < minw) w = minw;
   if (h < minh) h = minh;

   evas_object_resize(ELM_WIDGET_DATA(sd)->resize_obj, w, h);

   evas_object_size_hint_min_get(sd->bx, &minw_bx, &minh_bx);
   sd->s_iface->content_viewport_size_get(obj, &vw, &vh);

   if (sd->shrink_mode == ELM_TOOLBAR_SHRINK_NONE)
     {
        minw = minw_bx + (w - vw);
        minh = minh_bx + (h - vh);
     }
   else if (sd->shrink_mode == ELM_TOOLBAR_SHRINK_EXPAND)
     {
        if (sd->vertical)
          {
             minw = minw_bx + (w - vw);
             if (minh_bx <= vh) minh_bx = vh;
             else _items_size_fit(obj, &minh_bx, vh);
          }
        else
          {
             minh = minh_bx + (h - vh);
             if (minw_bx <= vw) minw_bx = vw;
             else _items_size_fit(obj, &minw_bx, vw);
          }
     }
   else
     {
        if (sd->vertical)
          {
             minw = minw_bx + (w - vw);
             minh = h - vh;
          }
        else
          {
             minw = w - vw;
             minh = minh_bx + (h - vh);
          }
     }

   if (sd->transverse_expanded)
     {
        if (sd->vertical)
          minw_bx = vw;
        else
          minh_bx = vh;
     }
   evas_object_resize(sd->bx, minw_bx, minh_bx);
   evas_object_resize(sd->more, minw_bx, minh_bx);
   evas_object_size_hint_min_set(obj, minw, minh);
   evas_object_size_hint_max_set(obj, -1, -1);

   _inform_item_number(obj);
}

static Eina_Bool
_elm_toolbar_smart_theme(Evas_Object *obj)
{
   Elm_Toolbar_Item *it;
   double scale = 0;
   const char *str;

   ELM_TOOLBAR_DATA_GET(obj, sd);

   if (sd->on_deletion)
     return EINA_TRUE;

   if (!ELM_WIDGET_CLASS(_elm_toolbar_parent_sc)->theme(obj))
     return EINA_FALSE;

   elm_widget_theme_object_set
     (obj, ELM_WIDGET_DATA(sd)->resize_obj, "toolbar", "base",
     elm_widget_style_get(obj));

   str = edje_object_data_get(ELM_WIDGET_DATA(sd)->resize_obj, "focus_highlight");
   if ((str) && (!strcmp(str, "on")))
     elm_widget_highlight_in_theme_set(obj, EINA_TRUE);
   else
     elm_widget_highlight_in_theme_set(obj, EINA_FALSE);

   elm_layout_theme_set
     (sd->more, "toolbar", "more", elm_widget_style_get(obj));

   _mirrored_set(obj, elm_widget_mirrored_get(obj));

   sd->theme_icon_size = _elm_toolbar_icon_size_get(sd);
   if (sd->priv_icon_size) sd->icon_size = sd->priv_icon_size;
   else sd->icon_size = sd->theme_icon_size;

   scale = (elm_widget_scale_get(obj) * elm_config_scale_get());
   EINA_INLIST_FOREACH(sd->items, it)
     _item_theme_hook(obj, it, scale, sd->icon_size);

   if (sd->more_item)
     _item_theme_hook(obj, sd->more_item, scale, sd->icon_size);

   _sizing_eval(obj);
   return EINA_TRUE;
}

static void
_elm_toolbar_item_label_update(Elm_Toolbar_Item *item)
{
   edje_object_part_text_escaped_set(VIEW(item), "elm.text", item->label);
   if (item->label)
     edje_object_signal_emit(VIEW(item), "elm,state,text,visible", "elm");
   else
     edje_object_signal_emit(VIEW(item), "elm,state,text,hidden", "elm");
}

static void
_elm_toolbar_item_label_set_cb(void *data,
                               Evas_Object *obj,
                               const char *emission,
                               const char *source)
{
   Elm_Toolbar_Item *item = data;

   _elm_toolbar_item_label_update(item);
   edje_object_signal_callback_del
     (obj, emission, source, _elm_toolbar_item_label_set_cb);
   edje_object_signal_emit(VIEW(item), "elm,state,label,reset", "elm");
}

static void
_item_label_set(Elm_Toolbar_Item *item,
                const char *label,
                const char *sig)
{
   const char *s;

   if ((label) && (item->label) && (!strcmp(label, item->label))) return;

   eina_stringshare_replace(&item->label, label);
   s = edje_object_data_get(VIEW(item), "transition_animation_on");
   if ((s) && (atoi(s)))
     {
        edje_object_part_text_escaped_set
          (VIEW(item), "elm.text_new", item->label);
        edje_object_signal_emit(VIEW(item), sig, "elm");
        edje_object_signal_callback_add
          (VIEW(item), "elm,state,label_set,done", "elm",
          _elm_toolbar_item_label_set_cb, item);
     }
   else
     _elm_toolbar_item_label_update(item);

   _resizing_eval(WIDGET(item), item);
}

static void
_item_text_set_hook(Elm_Object_Item *it,
                    const char *part,
                    const char *label)
{
   Elm_Toolbar_Item *item;
   char buf[256];
   item = (Elm_Toolbar_Item *)it;

   if ((!part) || (!strcmp(part, "default")) ||
       (!strcmp(part, "elm.text")))
     {
        _item_label_set(((Elm_Toolbar_Item *)it), label, "elm,state,label_set");
     }
   else
     {
        if (label)
          {
             snprintf(buf, sizeof(buf), "elm,state,%s,visible", part);
             edje_object_signal_emit(VIEW(item), buf, "elm");
          }
        else
          {
             snprintf(buf, sizeof(buf), "elm,state,%s,hidden", part);
             edje_object_signal_emit(VIEW(item), buf, "elm");
          }
        edje_object_part_text_escaped_set(VIEW(item), part, label);
     }
}

static const char *
_item_text_get_hook(const Elm_Object_Item *it,
                    const char *part)
{
   char buf[256];

   if (!part || !strcmp(part, "default"))
     snprintf(buf, sizeof(buf), "elm.text");
   else
     snprintf(buf, sizeof(buf), "%s", part);

   return edje_object_part_text_get(VIEW(it), buf);
}

static void
_item_content_set_hook(Elm_Object_Item *it,
                       const char *part,
                       Evas_Object *content)
{
   Elm_Toolbar_Item *item = (Elm_Toolbar_Item *)it;
   Evas_Object *obj = WIDGET(item);
   double scale;

   ELM_TOOLBAR_DATA_GET(obj, sd);

   if (part && strcmp(part, "object")) return;
   if (item->object == content) return;

   _elm_access_widget_item_unregister((Elm_Widget_Item *)item);

   if (item->object) evas_object_del(item->object);

   item->object = content;
   if (item->object)
     elm_widget_sub_object_add(obj, item->object);

   scale = (elm_widget_scale_get(obj) * elm_config_scale_get());
   _item_theme_hook(obj, item, scale, sd->icon_size);
}

static Evas_Object *
_item_content_get_hook(const Elm_Object_Item *it,
                       const char *part)
{
   if (part && strcmp(part, "object")) return NULL;
   return ((Elm_Toolbar_Item *)it)->object;
}

static Evas_Object *
_item_content_unset_hook(Elm_Object_Item *it,
                         const char *part)
{
   Elm_Toolbar_Item *item = (Elm_Toolbar_Item *)it;
   Evas_Object *obj = WIDGET(item);
   Evas_Object *o;
   double scale;

   ELM_TOOLBAR_DATA_GET(obj, sd);

   if (part && strcmp(part, "object")) return NULL;

   edje_object_part_unswallow(VIEW(it), item->object);
   elm_widget_sub_object_del(obj, item->object);
   o = item->object;
   item->object = NULL;
   scale = (elm_widget_scale_get(obj) * elm_config_scale_get());
   _item_theme_hook(obj, item, scale, sd->icon_size);

   return o;
}

static void
_item_signal_emit_hook(Elm_Object_Item *it ,
                       const char *emission,
                       const char *source)
{
   Elm_Toolbar_Item *item = (Elm_Toolbar_Item *)it;
   edje_object_signal_emit(VIEW(item), emission, source);
   edje_object_message_signal_process(VIEW(item));
}

static Eina_Bool
_elm_toolbar_smart_translate(Evas_Object *obj)
{
   ELM_TOOLBAR_DATA_GET(obj, sd);
   Elm_Toolbar_Item *it;

   EINA_INLIST_FOREACH(sd->items, it)
     elm_widget_item_translate(it);

   evas_object_smart_callback_call(obj, SIG_LANG_CHANGED, NULL);

   return EINA_TRUE;
}

static void
_elm_toolbar_smart_orientation_set(Evas_Object *obj, int rotation)
{
   ELM_TOOLBAR_DATA_GET(obj, sd);
   Elm_Toolbar_Item *it;
   char buf[128];

   snprintf(buf, sizeof(buf), "elm,state,orient,%d", rotation);
   EINA_INLIST_FOREACH(sd->items, it)
     {
        edje_object_signal_emit(VIEW(it), buf, "elm");
        edje_object_message_signal_process(VIEW(it));
        _resizing_eval(obj, it);
     }
   _sizing_eval(obj);
}

static void
_item_resize(void *data,
             Evas *e __UNUSED__,
             Evas_Object *obj __UNUSED__,
             void *event_info __UNUSED__)
{
   _sizing_eval(data);
   _resizing_eval(data, NULL);

}

static void
_move_cb(void *data,
         Evas *e __UNUSED__,
         Evas_Object *obj __UNUSED__,
         void *event_info __UNUSED__)
{
   Evas_Coord x, y, h;

   ELM_TOOLBAR_DATA_GET(data, sd);
   evas_object_geometry_get(data, &x, &y, NULL, &h);
   evas_object_move(sd->more, x, y + h);
}

// FIXME: There are applications which do not use elm_win as top widget.
// This is workaround! Those could not use focus!
static void
_highlight_off_cb(void *data __UNUSED__,
         Evas *e __UNUSED__,
         Evas_Object *obj,
         void *event_info __UNUSED__)
{
   ELM_TOOLBAR_DATA_GET(obj, sd);

   if (sd->highlighted_item)
     {
        edje_object_signal_emit(VIEW(sd->highlighted_item), "elm,highlight,off", "elm");
        evas_object_smart_callback_call(obj, "item,unfocused", sd->highlighted_item);
        sd->highlighted_item = NULL;
     }
}

static void
_select_filter_cb(Elm_Toolbar_Item *it,
                  Evas_Object *obj __UNUSED__,
                  const char *emission,
                  const char *source __UNUSED__)
{
   int button;
   char buf[sizeof("elm,action,click,") + 1];

   button = atoi(emission + sizeof("mouse,clicked,") - 1);
   if (button == 1) return;  /* regular left click event */
   snprintf(buf, sizeof(buf), "elm,action,click,%d", button);
   edje_object_signal_emit(VIEW(it), buf, "elm");
}

static void
_select_cb(void *data,
           Evas_Object *obj __UNUSED__,
           const char *emission __UNUSED__,
           const char *source __UNUSED__)
{
   Elm_Toolbar_Item *it = data;

   if ((_elm_config->access_mode == ELM_ACCESS_MODE_OFF) ||
       (_elm_access_2nd_click_timeout(VIEW(it))))
     {
        if (_elm_config->access_mode != ELM_ACCESS_MODE_OFF)
          _elm_access_say(WIDGET(it), E_("IDS_TPLATFORM_BODY_SELECTED_T_TTS"), EINA_FALSE);
        _item_select(it);
     }
}

static void
_item_move_cb(void *data,
         Evas *e __UNUSED__,
         Evas_Object *obj __UNUSED__,
         void *event_info __UNUSED__)
{
   Elm_Toolbar_Item *item = data;

   item->on_move = EINA_FALSE;

   evas_object_event_callback_del_full
     (VIEW(item), EVAS_CALLBACK_MOVE, _item_move_cb, data);
}

static void
_items_change(Elm_Toolbar_Item *reorder_from, Elm_Toolbar_Item *reorder_to)
{
   Elm_Toolbar_Item *prev = NULL, *next = NULL;
   int tmp;

   if (!reorder_from || !reorder_to) return;

   ELM_TOOLBAR_DATA_GET(WIDGET(reorder_from), sd);
   if (reorder_from == reorder_to) return;

   if ((!reorder_from->separator) && (!reorder_to->separator))
     {
        prev = ELM_TOOLBAR_ITEM_FROM_INLIST
            (EINA_INLIST_GET(reorder_from)->prev);
        if (prev == reorder_to)
          prev = reorder_from;
        if (!prev)
          next = ELM_TOOLBAR_ITEM_FROM_INLIST
              (EINA_INLIST_GET(reorder_from)->next);
        if (next == reorder_to)
          next = NULL;

        sd->items = eina_inlist_remove
            (sd->items, EINA_INLIST_GET(reorder_from));
        sd->items = eina_inlist_append_relative
            (sd->items, EINA_INLIST_GET(reorder_from),
            EINA_INLIST_GET(reorder_to));

        sd->items = eina_inlist_remove
            (sd->items, EINA_INLIST_GET(reorder_to));
        if (prev)
          sd->items = eina_inlist_append_relative
              (sd->items, EINA_INLIST_GET(reorder_to),
              EINA_INLIST_GET(prev));
        else if (next)
          sd->items = eina_inlist_prepend_relative
              (sd->items, EINA_INLIST_GET(reorder_to),
              EINA_INLIST_GET(next));
        else
          sd->items = eina_inlist_prepend
             (sd->items, EINA_INLIST_GET(reorder_to));

        evas_object_box_remove(sd->bx, VIEW(reorder_from));
        evas_object_box_insert_after(sd->bx, VIEW(reorder_from),
                                     VIEW(reorder_to));
        evas_object_box_remove(sd->bx, VIEW(reorder_to));
        if (prev)
          evas_object_box_insert_after(sd->bx, VIEW(reorder_to),
                                       VIEW(prev));
        else if (next)
          evas_object_box_insert_before(sd->bx, VIEW(reorder_to),
                                        VIEW(next));
        else
          evas_object_box_prepend(sd->bx, VIEW(reorder_to));

        tmp = reorder_from->prio.priority;
        reorder_from->prio.priority = reorder_to->prio.priority;
        reorder_to->prio.priority = tmp;

        reorder_from->on_move = EINA_TRUE;
        reorder_to->on_move = EINA_TRUE;

        evas_object_event_callback_add
           (VIEW(reorder_from), EVAS_CALLBACK_MOVE,
           _item_move_cb, reorder_from);
        evas_object_event_callback_add
           (VIEW(reorder_to), EVAS_CALLBACK_MOVE,
           _item_move_cb, reorder_to);
     }

   _resizing_eval(WIDGET(reorder_from), NULL);
}

static void
_transit_del_cb(void *data, Elm_Transit *transit __UNUSED__)
{
   Elm_Toolbar_Item *it, *item = data;
   ELM_TOOLBAR_DATA_GET(WIDGET(item), sd);

   if (item->reorder_to)
     {
        if (item->reorder_to == sd->reorder_empty)
          sd->reorder_empty = item;
        else if (item == sd->reorder_empty)
          sd->reorder_empty = item->reorder_to;

        _items_change(item->reorder_to, item);

        EINA_INLIST_FOREACH(sd->items, it)
          {
             if (it != item)
               {
                  if (it->reorder_to == item)
                    it->reorder_to = item->reorder_to;
                  else if (it->reorder_to == item->reorder_to)
                    it->reorder_to = item;
               }
          }
     }
   if (item->proxy)
     {
        evas_object_image_source_visible_set(elm_image_object_get(item->proxy), EINA_TRUE);
        evas_object_del(item->proxy);
        item->proxy = NULL;
     }
   item->trans = NULL;

   if (item->reorder_to)
     {
        EINA_INLIST_FOREACH(sd->items, it)
           if (it->trans) break;

        if (!it) sd->reorder_empty = sd->reorder_item;
     }
   item->reorder_to = NULL;
}

static void
_item_transition_start
(Elm_Toolbar_Item *it, Evas_Coord x, Evas_Coord y, Evas_Coord w, Evas_Coord h)
{
   Evas_Coord tx, ty;
   Evas_Object *obj = WIDGET(it);
   ELM_TOOLBAR_DATA_GET(obj, sd);

   it->proxy = elm_image_add(obj);
   elm_image_aspect_fixed_set(it->proxy, EINA_FALSE);
   evas_object_image_source_set(elm_image_object_get(it->proxy), VIEW(it));
   evas_object_image_source_visible_set(elm_image_object_get(it->proxy), EINA_FALSE);
   evas_object_image_source_clip_set(elm_image_object_get(it->proxy), EINA_FALSE);

   it->trans = elm_transit_add();
   elm_transit_object_add(it->trans, it->proxy);
   evas_object_geometry_get(VIEW(sd->reorder_empty), &tx, &ty, NULL, NULL);
   evas_object_move(it->proxy, x, y);
   evas_object_resize(it->proxy, w, h);
   evas_object_show(it->proxy);

   elm_transit_effect_translation_add(it->trans, 0, 0, tx - x, 0);
   elm_transit_duration_set(it->trans, 0.3);
   elm_transit_del_cb_set(it->trans, _transit_del_cb, it);
   elm_transit_go(it->trans);

   it->reorder_to = sd->reorder_empty;
}

static void
_animate_missed_items(Elm_Toolbar_Item *prev, Elm_Toolbar_Item *next)
{
   ELM_TOOLBAR_DATA_GET(WIDGET(prev), sd);
   Elm_Toolbar_Item *it, *it2;
   Eina_List *list, *l;
   Evas_Object *o;
   Eina_Bool reverse = EINA_FALSE;
   Evas_Coord fx, fy, fw, fh;

   list = evas_object_box_children_get(sd->bx);

   EINA_LIST_FOREACH(list, l, o)
     {
        if (o == VIEW(prev))
          break;
        else if (o == VIEW(next))
          reverse = EINA_TRUE;
     }

   if (!reverse)
     l = eina_list_next(l);
   else
     l = eina_list_prev(l);

   while (VIEW(next) != eina_list_data_get(l))
     {
        EINA_INLIST_FOREACH(sd->items, it)
          {
             if (VIEW(it) == eina_list_data_get(l))
               {
                  if (!it->trans && it != sd->reorder_item)
                    {
                       evas_object_geometry_get(VIEW(sd->reorder_empty), &fx, &fy, &fw, &fh);
                       _item_transition_start(it, fx, fy, fw, fh);
                       sd->reorder_empty = it;
                    }
                  EINA_INLIST_FOREACH(sd->items, it2)
                    {
                       if (it == it2->reorder_to) break;
                    }
                  if (it2)
                    {
                       it2->reorder_to = NULL;
                       evas_object_geometry_get(it2->proxy, &fx, &fy, &fw, &fh);
                       if (it2->trans) elm_transit_del(it2->trans);
                       _item_transition_start(it2, fx, fy, fw, fh);
                       sd->reorder_empty = it;
                    }
               }
          }
        if (!reverse)
          l = eina_list_next(l);
        else
          l = eina_list_prev(l);
     }
   eina_list_free(list);
}

static void
_mouse_move_reorder(Elm_Toolbar_Item *item,
                    Evas *evas __UNUSED__,
                    Evas_Object *obj __UNUSED__,
                    Evas_Event_Mouse_Move *ev)
{
   Evas_Coord x, y, w, h;
   Evas_Coord fx, fy, fw, fh;
   Elm_Toolbar_Item *it, *it2;

   ELM_TOOLBAR_DATA_GET(WIDGET(item), sd);

   evas_object_geometry_get(VIEW(item), &x, &y, &w, &h);
   if (sd->vertical)
     evas_object_move(item->proxy, x, ev->cur.canvas.y - (h / 2));
   else
     evas_object_move(item->proxy, ev->cur.canvas.x - (w / 2), y);
   evas_object_show(item->proxy);

   if (sd->reorder_empty->on_move) return;

   evas_object_geometry_get(sd->VIEW(reorder_empty), &x, &y, &w, &h);
   if (ev->cur.canvas.x < x || ev->cur.canvas.x > x + w)
     {
        EINA_INLIST_FOREACH(sd->items, it)
          {
             if (it->on_move) continue;
             evas_object_geometry_get(VIEW(it), &x, &y, &w, &h);
             if (ev->cur.canvas.x > x && ev->cur.canvas.x < x + w) break;
          }
        if (it && (it != sd->reorder_empty))
          {
             _animate_missed_items(sd->reorder_empty, it);
             if (!it->trans && it != item)
               {
                  evas_object_geometry_get(VIEW(it), &fx, &fy, &fw, &fh);
                  _item_transition_start(it, fx, fy, fw, fh);
                  sd->reorder_empty = it;
               }
             EINA_INLIST_FOREACH(sd->items, it2)
               {
                  if (it == it2->reorder_to) break;
               }
             if (it2)
               {
                  it2->reorder_to = NULL;
                  evas_object_geometry_get(it2->proxy, &fx, &fy, &fw, &fh);
                  if (it2->trans) elm_transit_del(it2->trans);
                  _item_transition_start(it2, fx, fy, fw, fh);
                  sd->reorder_empty = it;
               }
          }
     }
}

static void
_mouse_up_reorder(Elm_Toolbar_Item *it,
                  Evas *evas __UNUSED__,
                  Evas_Object *obj,
                  Evas_Event_Mouse_Up *ev __UNUSED__)
{
   ELM_TOOLBAR_DATA_GET(WIDGET(it), sd);

   evas_object_event_callback_del_full
     (obj, EVAS_CALLBACK_MOUSE_MOVE,
     (Evas_Object_Event_Cb)_mouse_move_reorder, it);
   evas_object_event_callback_del_full
     (sd->more, EVAS_CALLBACK_MOUSE_MOVE,
     (Evas_Object_Event_Cb)_mouse_move_reorder, it);
   evas_object_event_callback_del_full
     (VIEW(it), EVAS_CALLBACK_MOUSE_MOVE,
     (Evas_Object_Event_Cb)_mouse_move_reorder, it);
   evas_object_event_callback_del_full
     (obj, EVAS_CALLBACK_MOUSE_UP,
     (Evas_Object_Event_Cb)_mouse_up_reorder, it);
   evas_object_event_callback_del_full
     (sd->more, EVAS_CALLBACK_MOUSE_UP,
     (Evas_Object_Event_Cb)_mouse_up_reorder, it);

   if (it->proxy)
     {
        evas_object_image_source_visible_set(elm_image_object_get(it->proxy), EINA_TRUE);
        evas_object_del(it->proxy);
        it->proxy = NULL;
     }
   sd->s_iface->hold_set(obj, EINA_FALSE);
}

static void
_item_reorder_start(Elm_Toolbar_Item *item)
{
   Evas_Object *obj = WIDGET(item);
   Evas_Coord x, y, w, h;

   ELM_TOOLBAR_DATA_GET(obj, sd);

   sd->reorder_empty = sd->reorder_item = item;

   item->proxy = elm_image_add(obj);
   elm_image_aspect_fixed_set(item->proxy, EINA_FALSE);
   evas_object_image_source_set(elm_image_object_get(item->proxy), VIEW(item));
   evas_object_image_source_visible_set(elm_image_object_get(item->proxy), EINA_FALSE);
   evas_object_image_source_clip_set(elm_image_object_get(item->proxy), EINA_FALSE);
   evas_object_layer_set(item->proxy, 100);
   edje_object_signal_emit(VIEW(item), "elm,state,moving", "elm");

   evas_object_event_callback_add
     (obj, EVAS_CALLBACK_MOUSE_MOVE,
     (Evas_Object_Event_Cb)_mouse_move_reorder, item);

   evas_object_event_callback_add
     (sd->more, EVAS_CALLBACK_MOUSE_MOVE,
     (Evas_Object_Event_Cb)_mouse_move_reorder, item);

   evas_object_event_callback_add
     (item->proxy, EVAS_CALLBACK_MOUSE_MOVE,
     (Evas_Object_Event_Cb)_mouse_move_reorder, item);

   evas_object_event_callback_add
     (obj, EVAS_CALLBACK_MOUSE_UP,
     (Evas_Object_Event_Cb)_mouse_up_reorder, item);

   evas_object_event_callback_add
     (sd->more, EVAS_CALLBACK_MOUSE_UP,
     (Evas_Object_Event_Cb)_mouse_up_reorder, item);

   evas_object_geometry_get(VIEW(item), &x, &y, &w, &h);
   evas_object_resize(item->proxy, w, h);
   evas_object_move(item->proxy, x, y);
   evas_object_show(item->proxy);

   sd->s_iface->hold_set(WIDGET(item), EINA_TRUE);
}

static Eina_Bool
_long_press_cb(void *data)
{
   Elm_Toolbar_Item *it = data;
   ELM_TOOLBAR_DATA_GET(WIDGET(it), sd);

   sd->long_timer = NULL;
   sd->long_press = EINA_TRUE;

   if (sd->reorder_mode)
     _item_reorder_start(it);

   evas_object_smart_callback_call(WIDGET(it), SIG_LONGPRESSED, it);

   return ECORE_CALLBACK_CANCEL;
}

static void
_mouse_move_cb(Elm_Toolbar_Item *it,
               Evas *evas __UNUSED__,
               Evas_Object *obj __UNUSED__,
               Evas_Event_Mouse_Move *ev)
{
   Evas_Coord x, y, w, h;

   ELM_TOOLBAR_DATA_GET(WIDGET(it), sd);
   evas_object_geometry_get(VIEW(it), &x, &y, &w, &h);

   if ((sd->long_timer) &&
       ((x > ev->cur.canvas.x) || (ev->cur.canvas.x > x + w) ||
        (y > ev->cur.canvas.y) || (ev->cur.canvas.y > y + h)))
     {
        ecore_timer_del(sd->long_timer);
        sd->long_timer = NULL;
     }
   //******************** TIZEN Only
   if ((it->pressed) &&
       (ELM_RECTS_POINT_OUT(x, y, w, h, ev->cur.canvas.x, ev->cur.canvas.y)))
     {
        edje_object_signal_emit(VIEW(it), "elm,action,unpressed", "elm");
        it->pressed = EINA_FALSE;
     }
   //****************************
}

static void
_mouse_down_cb(Elm_Toolbar_Item *it,
               Evas *evas,
               Evas_Object *obj __UNUSED__,
               Evas_Event_Mouse_Down *ev)
{
   ELM_TOOLBAR_DATA_GET(WIDGET(it), sd);

   if (ev->button != 1) return;

   if (evas_event_down_count_get(evas) == 1)
     {
        edje_object_signal_emit(VIEW(it), "elm,action,multi,cancel", "elm");
     }
   else if (evas_event_down_count_get(evas) != 1)
     {
        edje_object_signal_emit(VIEW(it), "elm,action,multi,down", "elm");
        return;
     }
   sd->mouse_down = EINA_TRUE;

   if (ev->flags & EVAS_BUTTON_DOUBLE_CLICK)
     evas_object_smart_callback_call(WIDGET(it), SIG_CLICKED_DOUBLE, it);
   sd->long_press = EINA_FALSE;
   if (sd->long_timer)
     ecore_timer_interval_set
       (sd->long_timer, _elm_config->longpress_timeout);
   else
     sd->long_timer = ecore_timer_add
         (_elm_config->longpress_timeout, _long_press_cb, it);

   evas_object_event_callback_add(VIEW(it), EVAS_CALLBACK_MOUSE_MOVE,
                                  (Evas_Object_Event_Cb)_mouse_move_cb, it);
//******************** TIZEN Only
   it->pressed = EINA_TRUE;
//****************************
}

static void
_mouse_up_cb(Elm_Toolbar_Item *it,
             Evas *evas,
             Evas_Object *obj __UNUSED__,
             Evas_Event_Mouse_Up *ev)
{
   ELM_TOOLBAR_DATA_GET(WIDGET(it), sd);

   if (ev->button != 1) return;

   if (evas_event_down_count_get(evas) != 0)
     {
        edje_object_signal_emit(VIEW(it), "elm,action,multi,down", "elm");
     }
   if (!sd->mouse_down) return;
   sd->mouse_down = EINA_FALSE;

   if (sd->long_timer)
     {
        ecore_timer_del(sd->long_timer);
        sd->long_timer = NULL;
     }
   evas_object_event_callback_del_full
     (VIEW(it), EVAS_CALLBACK_MOUSE_MOVE,
     (Evas_Object_Event_Cb)_mouse_move_cb, it);

//******************** TIZEN Only
   it->pressed = EINA_FALSE;
//****************************
}

static void
_mouse_in_cb(void *data,
             Evas_Object *obj __UNUSED__,
             const char *emission __UNUSED__,
             const char *source __UNUSED__)
{
   Elm_Toolbar_Item *it = data;

   edje_object_signal_emit(VIEW(it), "elm,state,highlighted", "elm");
   elm_widget_signal_emit(it->icon, "elm,state,highlighted", "elm");
}

static void
_mouse_out_cb(void *data,
              Evas_Object *obj __UNUSED__,
              const char *emission __UNUSED__,
              const char *source __UNUSED__)
{
   Elm_Toolbar_Item *it = data;

   edje_object_signal_emit(VIEW(it), "elm,state,unhighlighted", "elm");
   elm_widget_signal_emit(it->icon, "elm,state,unhighlighted", "elm");
}

static void
_scroll_cb(Evas_Object *obj,
           void *data __UNUSED__)
{
   evas_object_smart_callback_call(obj, SIG_SCROLL, NULL);
}

static void
_scroll_anim_start_cb(Evas_Object *obj,
                      void *data __UNUSED__)
{
   evas_object_smart_callback_call(obj, SIG_SCROLL_ANIM_START, NULL);
}

static void
_scroll_anim_stop_cb(Evas_Object *obj,
                     void *data __UNUSED__)
{
   evas_object_smart_callback_call(obj, SIG_SCROLL_ANIM_STOP, NULL);
}

static void
_scroll_drag_start_cb(Evas_Object *obj,
                      void *data __UNUSED__)
{
  ELM_TOOLBAR_DATA_GET(obj, sd);

   if (sd->long_timer)
     {
        ecore_timer_del(sd->long_timer);
        sd->long_timer = NULL;
     }

   evas_object_smart_callback_call(obj, SIG_SCROLL_DRAG_START, NULL);
}

static void
_scroll_drag_stop_cb(Evas_Object *obj,
                     void *data __UNUSED__)
{
   evas_object_smart_callback_call(obj, SIG_SCROLL_DRAG_STOP, NULL);
}

static void
_layout(Evas_Object *o,
        Evas_Object_Box_Data *priv,
        void *data)
{
   Evas_Object *obj = (Evas_Object *)data;

   ELM_TOOLBAR_DATA_GET(obj, sd);
   _els_box_layout
     (o, priv, !sd->vertical, sd->homogeneous, elm_widget_mirrored_get(obj));
}

static char *
_access_type_cb(void *data, Evas_Object *obj __UNUSED__)
{
   const char *style = NULL;
   Elm_Toolbar_Item *it = (Elm_Toolbar_Item *)data;

   style = edje_object_data_get(VIEW(it), "widget_style");

   if (style)
     return strdup(E_(style));

   return strdup(E_("IDS_TPLATFORM_BODY_TAB_T_TTS"));
}

static char *
_access_info_cb(void *data, Evas_Object *obj __UNUSED__)
{
   Elm_Toolbar_Item *it = (Elm_Toolbar_Item *)data;
   const char *txt = ((Elm_Widget_Item *)it)->access_info;

   if (!txt) txt = it->label;
   if (txt) return strdup(txt);

   return NULL;
}

static char *
_access_state_cb(void *data, Evas_Object *obj __UNUSED__)
{
   Elm_Toolbar_Item *it = (Elm_Toolbar_Item *)data;

   if (it->separator)
     return strdup(E_("Separator"));
   else if (elm_widget_item_disabled_get(it))
     return strdup(E_("IDS_ACCS_BODY_DISABLED_TTS"));
   else if (it->selected)
     return strdup(E_("IDS_TPLATFORM_BODY_SELECTED_T_TTS"));
   else if (it->menu)
     return strdup(E_("Has menu"));

   return NULL;
}

static char *
_access_context_info_cb(void *data, Evas_Object *obj __UNUSED__)
{
   Elm_Toolbar_Item *it = (Elm_Toolbar_Item *)data;
   const char *style;
   style = elm_widget_style_get(WIDGET(it));

   if (!it->separator && !it->selected &&
       !elm_widget_item_disabled_get(it) && !strcmp(style, "tabbar"))
     return strdup(E_("IDS_TPLATFORM_BODY_DOUBLE_TAP_TO_MOVE_TO_CONTENT_T_TTS"));

   return NULL;
}

static Eina_Bool
_item_del_pre_hook(Elm_Object_Item *it)
{
   Elm_Toolbar_Item *item, *next = NULL;
   Evas_Object *obj;

   item = (Elm_Toolbar_Item *)it;

   ELM_TOOLBAR_DATA_GET(WIDGET(item), sd);

   obj = WIDGET(item);

   evas_object_box_remove(sd->bx,VIEW(it));
   //Remove the item from the box as sometimes the resize callback of sd->bx is not getting hit
   if (item != sd->more_item) /* more item does not get in the list */
     {
        if (!sd->on_deletion)
          next = ELM_TOOLBAR_ITEM_FROM_INLIST(EINA_INLIST_GET(item)->next);
        sd->items = eina_inlist_remove(sd->items, EINA_INLIST_GET(item));
        sd->item_count--;
        if (!sd->on_deletion)
          {
             if (!next) next = ELM_TOOLBAR_ITEM_FROM_INLIST(sd->items);
             if ((sd->select_mode == ELM_OBJECT_SELECT_MODE_ALWAYS) &&
                 item->selected && next) _item_select(next);
          }
     }

   _item_del(item);

   if (item != sd->more_item)
     _elm_toolbar_smart_theme(obj);

   return EINA_TRUE;
}

static void
_access_activate_cb(void *data __UNUSED__,
                    Evas_Object *part_obj __UNUSED__,
                    Elm_Object_Item *item)
{
   Elm_Toolbar_Item *it;
   it = (Elm_Toolbar_Item *)item;

   if (elm_widget_item_disabled_get(it)) return;

   if (!it->selected)
     {
        _elm_access_say(WIDGET(it), E_("IDS_TPLATFORM_BODY_SELECTED_T_TTS"), EINA_FALSE);
        _item_select(it);
     }
}

static void
_access_widget_item_register(Elm_Toolbar_Item *it)
{
   Elm_Access_Info *ai;
   _elm_access_widget_item_register((Elm_Widget_Item *)it);
   ai = _elm_access_object_get(it->base.access_obj);

   _elm_access_callback_set(ai, ELM_ACCESS_TYPE, _access_type_cb, it);
   _elm_access_callback_set(ai, ELM_ACCESS_INFO, _access_info_cb, it);
   _elm_access_callback_set(ai, ELM_ACCESS_STATE, _access_state_cb, it);
   _elm_access_callback_set(ai, ELM_ACCESS_CONTEXT_INFO, _access_context_info_cb, it);
   _elm_access_activate_callback_set(ai, _access_activate_cb, NULL);
}

static Elm_Toolbar_Item *
_item_new(Evas_Object *obj,
          const char *icon,
          const char *label,
          Evas_Smart_Cb func,
          const void *data)
{
   Evas_Object *icon_obj;
   Elm_Toolbar_Item *it;

   ELM_TOOLBAR_DATA_GET(obj, sd);

   icon_obj = elm_icon_add(obj);
   elm_icon_order_lookup_set(icon_obj, sd->lookup_order);
   if (!icon_obj) return NULL;

   it = elm_widget_item_new(obj, Elm_Toolbar_Item);
   if (!it)
     {
        evas_object_del(icon_obj);
        return NULL;
     }

   elm_widget_item_del_pre_hook_set(it, _item_del_pre_hook);
   elm_widget_item_disable_hook_set(it, _item_disable_hook);
   elm_widget_item_text_set_hook_set(it, _item_text_set_hook);
   elm_widget_item_text_get_hook_set(it, _item_text_get_hook);
   elm_widget_item_content_set_hook_set(it, _item_content_set_hook);
   elm_widget_item_content_get_hook_set(it, _item_content_get_hook);
   elm_widget_item_content_unset_hook_set(it, _item_content_unset_hook);
   elm_widget_item_signal_emit_hook_set(it, _item_signal_emit_hook);

   it->label = eina_stringshare_add(label);
   it->prio.visible = 1;
   it->prio.priority = 0;
   it->func = func;
   it->separator = EINA_FALSE;
   it->object = NULL;
   it->base.data = data;

   VIEW(it) = edje_object_add(evas_object_evas_get(obj));
   evas_object_data_set(VIEW(it), "item", it);

   if (_elm_config->access_mode == ELM_ACCESS_MODE_ON)
     _access_widget_item_register(it);

   if (_item_icon_set(icon_obj, "toolbar/", icon))
     {
        it->icon = icon_obj;
        it->icon_str = eina_stringshare_add(icon);
     }
   else
     {
        it->icon = NULL;
        it->icon_str = NULL;
        evas_object_del(icon_obj);
     }

   elm_widget_theme_object_set
     (obj, VIEW(it), "toolbar", "item", elm_widget_style_get(obj));
   edje_object_signal_callback_add
     (VIEW(it), "elm,action,click", "elm", _select_cb, it);
   edje_object_signal_callback_add
     (VIEW(it), "mouse,clicked,*", "*", (Edje_Signal_Cb)_select_filter_cb, it);
   edje_object_signal_callback_add
     (VIEW(it), "elm,mouse,in", "elm", _mouse_in_cb, it);
   edje_object_signal_callback_add
     (VIEW(it), "elm,mouse,out", "elm", _mouse_out_cb, it);
   evas_object_event_callback_add
     (VIEW(it), EVAS_CALLBACK_MOUSE_DOWN, (Evas_Object_Event_Cb)_mouse_down_cb,
     it);
   evas_object_event_callback_add
     (VIEW(it), EVAS_CALLBACK_MOUSE_UP, (Evas_Object_Event_Cb)_mouse_up_cb, it);

   elm_widget_sub_object_add(obj, VIEW(it));
   _resizing_eval(obj, it);
   evas_object_event_callback_add
     (VIEW(it), EVAS_CALLBACK_RESIZE, _item_resize, obj);

   if ((!sd->items) && (sd->select_mode == ELM_OBJECT_SELECT_MODE_ALWAYS))
     _item_select(it);
   return it;
}

static void
_elm_toolbar_item_icon_update(Elm_Toolbar_Item *item)
{
   Elm_Toolbar_Item_State *it_state;
   Evas_Object *old_icon =
     edje_object_part_swallow_get(VIEW(item), "elm.swallow.icon");
   Eina_List *l;

   elm_widget_sub_object_del(VIEW(item), old_icon);
   edje_object_part_swallow(VIEW(item), "elm.swallow.icon", item->icon);
   if (item->icon)
       edje_object_signal_emit(VIEW(item), "elm,state,icon,visible", "elm");
   else
       edje_object_signal_emit(VIEW(item), "elm,state,icon,hidden", "elm");
   evas_object_hide(old_icon);

   EINA_LIST_FOREACH(item->states, l, it_state)
     {
        if (it_state->icon == old_icon) return;
     }
   evas_object_del(old_icon);
}

static void
_elm_toolbar_item_icon_set_cb(void *data,
                              Evas_Object *obj,
                              const char *emission,
                              const char *source)
{
   Elm_Toolbar_Item *item = data;

   edje_object_part_unswallow(VIEW(item), item->icon);
   _elm_toolbar_item_icon_update(item);
   edje_object_signal_callback_del
     (obj, emission, source, _elm_toolbar_item_icon_set_cb);
   edje_object_signal_emit(VIEW(item), "elm,state,icon,reset", "elm");
}

static void
_elm_toolbar_item_icon_obj_set(Evas_Object *obj,
                               Elm_Toolbar_Item *item,
                               Evas_Object *icon_obj,
                               const char *icon_str,
                               double icon_size,
                               const char *sig)
{
   Evas_Object *old_icon;
   const char *s;

   if (icon_str)
     eina_stringshare_replace(&item->icon_str, icon_str);
   else
     {
        eina_stringshare_del(item->icon_str);
        item->icon_str = NULL;
     }
   item->icon = icon_obj;
   if (icon_obj)
     {
        evas_object_size_hint_min_set(item->icon, icon_size, icon_size);
        evas_object_size_hint_max_set(item->icon, icon_size, icon_size);
        evas_object_show(item->icon);
        elm_widget_sub_object_add(obj, item->icon);
     }
   s = edje_object_data_get(VIEW(item), "transition_animation_on");
   if ((s) && (atoi(s)))
     {
        old_icon = edje_object_part_swallow_get
            (VIEW(item), "elm.swallow.icon_new");
        if (old_icon)
          {
             elm_widget_sub_object_del(VIEW(item), old_icon);
             evas_object_hide(old_icon);
          }
        edje_object_part_swallow
          (VIEW(item), "elm.swallow.icon_new", item->icon);
        edje_object_signal_emit(VIEW(item), sig, "elm");
        edje_object_signal_callback_add
          (VIEW(item), "elm,state,icon_set,done", "elm",
          _elm_toolbar_item_icon_set_cb, item);
     }
   else
     _elm_toolbar_item_icon_update(item);
}

static void
_elm_toolbar_item_state_cb(void *data __UNUSED__,
                           Evas_Object *obj __UNUSED__,
                           void *event_info)
{
   Elm_Toolbar_Item *it = event_info;
   Elm_Toolbar_Item_State *it_state;

   it_state = eina_list_data_get(it->current_state);
   if (it_state->func)
     it_state->func((void *)it_state->data, obj, event_info);
}

static Elm_Toolbar_Item_State *
_item_state_new(const char *label,
                const char *icon_str,
                Evas_Object *icon,
                Evas_Smart_Cb func,
                const void *data)
{
   Elm_Toolbar_Item_State *it_state;

   it_state = ELM_NEW(Elm_Toolbar_Item_State);
   it_state->label = eina_stringshare_add(label);
   it_state->icon_str = eina_stringshare_add(icon_str);
   it_state->icon = icon;
   it_state->func = func;
   it_state->data = data;

   return it_state;
}

static void
_elm_toolbar_action_left_cb(void *data,
                            Evas_Object *o __UNUSED__,
                            const char *sig __UNUSED__,
                            const char *src __UNUSED__)
{
   Evas_Object *obj = data;
   Elm_Toolbar_Item *it, *it2;
   Eina_Bool done = EINA_FALSE;

   ELM_TOOLBAR_DATA_GET(obj, sd);

   EINA_INLIST_FOREACH(sd->items, it)
     {
        if (it->selected)
          {
             Eina_Bool found = EINA_FALSE;

             EINA_INLIST_REVERSE_FOREACH(sd->items, it2)
               {
                  if (elm_object_item_disabled_get((Elm_Object_Item *)it2))
                    continue;
                  if (it2 == it)
                    {
                       found = EINA_TRUE;
                       continue;
                    }
                  if (!found) continue;
                  if (it2->separator) continue;
                  _item_unselect(it);
                  _item_select(it2);
                  break;
               }
             done = EINA_TRUE;
             break;
          }
     }
   if (!done)
     {
        EINA_INLIST_FOREACH(sd->items, it)
          {
             if (elm_object_item_disabled_get((Elm_Object_Item *)it)) continue;
             if (it->separator) continue;
             _item_select(it);
             break;
          }
     }
}

static void
_elm_toolbar_action_right_cb(void *data,
                             Evas_Object *o __UNUSED__,
                             const char *sig __UNUSED__,
                             const char *src __UNUSED__)
{
   Evas_Object *obj = data;
   Elm_Toolbar_Item *it, *it2;
   Eina_Bool done = EINA_FALSE;

   ELM_TOOLBAR_DATA_GET(obj, sd);

   EINA_INLIST_FOREACH(sd->items, it)
     {
        if (it->selected)
          {
             Eina_Bool found = EINA_FALSE;

             EINA_INLIST_FOREACH(sd->items, it2)
               {
                  if (elm_object_item_disabled_get((Elm_Object_Item *)it2))
                    continue;
                  if (it2 == it)
                    {
                       found = EINA_TRUE;
                       continue;
                    }
                  if (!found) continue;
                  if (it2->separator) continue;
                  _item_unselect(it);
                  _item_select(it2);
                  break;
               }
             done = EINA_TRUE;
             break;
          }
     }
   if (!done)
     {
        EINA_INLIST_REVERSE_FOREACH(sd->items, it)
          {
             if (elm_object_item_disabled_get((Elm_Object_Item *)it)) continue;
             if (it->separator) continue;
             _item_select(it);
             break;
          }
     }
}

static void
_elm_toolbar_action_up_cb(void *data,
                          Evas_Object *o,
                          const char *sig,
                          const char *src)
{
   _elm_toolbar_action_left_cb(data, o, sig, src);
}

static void
_elm_toolbar_action_down_cb(void *data,
                            Evas_Object *o,
                            const char *sig,
                            const char *src)
{
   _elm_toolbar_action_right_cb(data, o, sig, src);
}

static void
_elm_toolbar_smart_add(Evas_Object *obj)
{
   EVAS_SMART_DATA_ALLOC(obj, Elm_Toolbar_Smart_Data);

   ELM_WIDGET_CLASS(_elm_toolbar_parent_sc)->base.add(obj);
}

static void
_elm_toolbar_smart_del(Evas_Object *obj)
{
   Elm_Toolbar_Item *it, *next;

   ELM_TOOLBAR_DATA_GET(obj, sd);

   sd->on_deletion = EINA_TRUE;

   if (sd->resize_job)
     ecore_job_del(sd->resize_job);

   sd->resize_job = NULL;

   it = ELM_TOOLBAR_ITEM_FROM_INLIST(sd->items);
   while (it)
     {
        next = ELM_TOOLBAR_ITEM_FROM_INLIST(EINA_INLIST_GET(it)->next);
        elm_widget_item_del(it);
        it = next;
     }

   sd->items = NULL;

   if (sd->more_item)
     {
        elm_widget_item_del(sd->more_item);
        sd->more_item = NULL;
     }
   if (sd->long_timer)
     {
        ecore_timer_del(sd->long_timer);
        sd->long_timer = NULL;
     }

   ELM_WIDGET_CLASS(_elm_toolbar_parent_sc)->base.del(obj);
}

static void
_elm_toolbar_smart_move(Evas_Object *obj,
                        Evas_Coord x,
                        Evas_Coord y)
{
   ELM_TOOLBAR_DATA_GET(obj, sd);

   ELM_WIDGET_CLASS(_elm_toolbar_parent_sc)->base.move(obj, x, y);

   evas_object_move(sd->hit_rect, x, y);
}

static void
_elm_toolbar_smart_resize(Evas_Object *obj,
                          Evas_Coord w,
                          Evas_Coord h)
{

   ELM_TOOLBAR_DATA_GET(obj, sd);
   ELM_WIDGET_CLASS(_elm_toolbar_parent_sc)->base.resize(obj, w, h);
   evas_object_resize(sd->hit_rect, w, h);
   /*resize the box first to avoid flickering of
     saved state of the toolbar when ratation changed but not in case of scroll*/
   if (sd->shrink_mode != ELM_TOOLBAR_SHRINK_SCROLL)
     evas_object_resize(sd->bx, w, h);
   _resizing_eval(obj, NULL);
}

static void
_elm_toolbar_smart_member_add(Evas_Object *obj,
                              Evas_Object *member)
{
   ELM_TOOLBAR_DATA_GET(obj, sd);

   ELM_WIDGET_CLASS(_elm_toolbar_parent_sc)->base.member_add(obj, member);

   if (sd->hit_rect)
     evas_object_raise(sd->hit_rect);
}

static Eina_List *
_access_item_find_append(const Evas_Object *obj,
                         Evas_Object *bx,
                         Eina_List *items)
{
   Elm_Toolbar_Item *it;
   Eina_List *list;

   ELM_TOOLBAR_DATA_GET(obj, sd);

   list = evas_object_box_children_get(bx);
   if (!list) return items;

   EINA_INLIST_FOREACH (sd->items, it)
     {
        if (it->separator) continue;
        if (eina_list_data_find(list, it->base.view))
          {
             if (it->base.access_obj)
               items = eina_list_append(items, it->base.access_obj);
             else
               items = eina_list_append(items, it->object);
          }
     }

   eina_list_free(list);

   return items;
}

static Eina_Bool
_elm_toolbar_smart_focus_next(const Evas_Object *obj,
                              Elm_Focus_Direction dir,
                              Evas_Object **next)
{
   Eina_List *items = NULL;
   Eina_List *list;

   ELM_TOOLBAR_DATA_GET(obj, sd);

   if (sd->more_item && sd->more_item->selected)
     {
        items = _access_item_find_append(obj, sd->bx_more, items);
        items = _access_item_find_append(obj, sd->bx_more2, items);
        items = eina_list_append(items, sd->more_item->base.access_obj);
     }
   else
     {
        items = _access_item_find_append(obj, sd->bx, items);
        list = evas_object_box_children_get(sd->bx);
        if (sd->more_item &&
            eina_list_data_find(list, sd->more_item->base.view))
          items = eina_list_append(items, sd->more_item->base.access_obj);
        eina_list_free(list);
     }

   return elm_widget_focus_list_next_get
            (obj, items, eina_list_data_get, dir, next);
}
static Eina_Bool
_elm_toolbar_smart_focusable_is(Evas_Object *obj, Evas_Coord x, Evas_Coord y)
{
   printf("x and y  pint x: %d   y: %d  \n", x, y);
   Evas_Coord cx, cy, cw, ch;
   Elm_Toolbar_Item *it;

   ELM_TOOLBAR_DATA_GET(obj, sd);

   EINA_INLIST_FOREACH(sd->items, it)
     {
        if (it->base.disabled)
          {
        	evas_object_geometry_get(VIEW(it), &cx, &cy, &cw, &ch);
        	if (!(ELM_RECTS_POINT_OUT(cx, cy, cw, ch, x, y))) return EINA_FALSE;
          }
     }
   return EINA_TRUE;

}
static void
_elm_toolbar_smart_focus_highlight_set(Evas_Object *obj, Eina_Bool focus_highlight)
{
   Evas_Coord x, y, w, h;
   ELM_TOOLBAR_DATA_GET(obj, sd);
   if (focus_highlight)
     {
        if (sd->highlighted_item)
          {
             edje_object_signal_emit(VIEW(sd->highlighted_item), "elm,highlight,on", "elm");
             evas_object_smart_callback_call(obj, "item,focused", sd->highlighted_item);
             if (_elm_toolbar_item_coordinates_calc(
                   sd->highlighted_item, ELM_TOOLBAR_ITEM_SCROLLTO_IN, &x, &y, &w, &h))
               sd->s_iface->region_bring_in(obj, x, y, w, h);
          }
     }
   else
     {
        if (sd->highlighted_item)
          {
             edje_object_signal_emit(VIEW(sd->highlighted_item), "elm,highlight,off", "elm");
             evas_object_smart_callback_call(obj, "item,unfocused", sd->highlighted_item);
          }
     }
}

static void
_access_obj_process(Elm_Toolbar_Smart_Data * sd, Eina_Bool is_access)
{
   Elm_Toolbar_Item *it;

   EINA_INLIST_FOREACH (sd->items, it)
     {
        if (is_access) _access_widget_item_register(it);
        else _elm_access_widget_item_unregister((Elm_Widget_Item *)it);
     }
}

static void
_elm_toolbar_smart_access(Evas_Object *obj, Eina_Bool is_access)
{
   ELM_TOOLBAR_CHECK(obj);
   ELM_TOOLBAR_DATA_GET(obj, sd);

   if (is_access)
     ELM_WIDGET_CLASS(ELM_WIDGET_DATA(sd)->api)->focus_next =
       _elm_toolbar_smart_focus_next;
   else
     ELM_WIDGET_CLASS(ELM_WIDGET_DATA(sd)->api)->focus_next = NULL;
   _access_obj_process(sd, is_access);

   evas_object_smart_callback_call(obj, SIG_ACCESS_CHANGED, NULL);
}

static void
_elm_toolbar_smart_set_user(Elm_Toolbar_Smart_Class *sc)
{
   ELM_WIDGET_CLASS(sc)->base.add = _elm_toolbar_smart_add;
   ELM_WIDGET_CLASS(sc)->base.del = _elm_toolbar_smart_del;
   ELM_WIDGET_CLASS(sc)->base.move = _elm_toolbar_smart_move;
   ELM_WIDGET_CLASS(sc)->base.resize = _elm_toolbar_smart_resize;
   ELM_WIDGET_CLASS(sc)->base.member_add = _elm_toolbar_smart_member_add;

   ELM_WIDGET_CLASS(sc)->on_focus = _elm_toolbar_smart_on_focus;
   ELM_WIDGET_CLASS(sc)->event = _elm_toolbar_smart_event;
   ELM_WIDGET_CLASS(sc)->theme = _elm_toolbar_smart_theme;
   ELM_WIDGET_CLASS(sc)->translate = _elm_toolbar_smart_translate;
   ELM_WIDGET_CLASS(sc)->orientation_set = _elm_toolbar_smart_orientation_set;
   ELM_WIDGET_CLASS(sc)->focus_highlight_set = _elm_toolbar_smart_focus_highlight_set;
   ELM_WIDGET_CLASS(sc)->focusable_is = _elm_toolbar_smart_focusable_is;

   if (_elm_config->access_mode != ELM_ACCESS_MODE_OFF)
     ELM_WIDGET_CLASS(sc)->focus_next = _elm_toolbar_smart_focus_next;

   ELM_WIDGET_CLASS(sc)->access = _elm_toolbar_smart_access;
}

EAPI const Elm_Toolbar_Smart_Class *
elm_toolbar_smart_class_get(void)
{
   static Elm_Toolbar_Smart_Class _sc =
     ELM_TOOLBAR_SMART_CLASS_INIT_NAME_VERSION(ELM_TOOLBAR_SMART_NAME);
   static const Elm_Toolbar_Smart_Class *class = NULL;
   Evas_Smart_Class *esc = (Evas_Smart_Class *)&_sc;

   if (class)
     return class;

   _elm_toolbar_smart_set(&_sc);
   esc->callbacks = _smart_callbacks;
   class = &_sc;

   return class;
}

EAPI Evas_Object *
elm_toolbar_add(Evas_Object *parent)
{
   Evas_Object *obj;

   EINA_SAFETY_ON_NULL_RETURN_VAL(parent, NULL);

   obj = elm_widget_add(_elm_toolbar_smart_class_new(), parent);
   if (!obj) return NULL;

   if (!elm_widget_sub_object_add(parent, obj))
     ERR("could not add %p as sub object of %p", obj, parent);

   Evas_Object *edje;

   edje = edje_object_add(evas_object_evas_get(obj));
   elm_widget_resize_object_set(obj, edje, EINA_TRUE);

   elm_widget_theme_object_set
     (obj, edje, "toolbar", "base", elm_widget_style_get(obj));

   ELM_TOOLBAR_DATA_GET(obj, sd);

   sd->hit_rect = evas_object_rectangle_add(evas_object_evas_get(obj));
   evas_object_smart_member_add(sd->hit_rect, obj);
   elm_widget_sub_object_add(obj, sd->hit_rect);

   /* common scroller hit rectangle setup */
   evas_object_color_set(sd->hit_rect, 0, 0, 0, 0);
   evas_object_show(sd->hit_rect);
   evas_object_repeat_events_set(sd->hit_rect, EINA_TRUE);

   elm_widget_can_focus_set(obj, EINA_TRUE);

   sd->s_iface = evas_object_smart_interface_get
       (obj, ELM_SCROLLABLE_IFACE_NAME);

   sd->s_iface->objects_set(obj, edje, sd->hit_rect);

   sd->mouse_down = EINA_FALSE;
   sd->more_item = NULL;
   sd->selected_item = NULL;
   sd->standard_priority = -99999;

   sd->s_iface->bounce_allow_set
     (obj, _elm_config->thumbscroll_bounce_enable, EINA_FALSE);
   sd->s_iface->policy_set
     (obj, ELM_SCROLLER_POLICY_AUTO, ELM_SCROLLER_POLICY_OFF);
   sd->s_iface->scroll_cb_set(obj, _scroll_cb);
   sd->s_iface->animate_start_cb_set(obj, _scroll_anim_start_cb);
   sd->s_iface->animate_stop_cb_set(obj, _scroll_anim_stop_cb);
   sd->s_iface->drag_start_cb_set(obj, _scroll_drag_start_cb);
   sd->s_iface->drag_stop_cb_set(obj, _scroll_drag_stop_cb);

   edje_object_signal_callback_add
     (edje, "elm,action,left", "elm", _elm_toolbar_action_left_cb, obj);
   edje_object_signal_callback_add
     (edje, "elm,action,right", "elm", _elm_toolbar_action_right_cb, obj);
   edje_object_signal_callback_add
     (edje, "elm,action,up", "elm", _elm_toolbar_action_up_cb, obj);
   edje_object_signal_callback_add
     (edje, "elm,action,down", "elm", _elm_toolbar_action_down_cb, obj);

   sd->shrink_mode = ELM_TOOLBAR_SHRINK_NONE;
   sd->priv_icon_size = 0; // unset
   sd->theme_icon_size = _elm_toolbar_icon_size_get(sd);
   if (sd->priv_icon_size) sd->icon_size = sd->priv_icon_size;
   else sd->icon_size = sd->theme_icon_size;

   sd->homogeneous = EINA_TRUE;
   sd->align = 0.5;

   sd->bx = evas_object_box_add(evas_object_evas_get(obj));
   evas_object_box_align_set(sd->bx, sd->align, 0.5);
   evas_object_box_layout_set(sd->bx, _layout, obj, NULL);
   elm_widget_sub_object_add(obj, sd->bx);
   sd->s_iface->content_set(obj, sd->bx);
   evas_object_show(sd->bx);

   sd->more = elm_layout_add(obj);
   elm_layout_theme_set(sd->more, "toolbar", "more", "default");
   elm_widget_sub_object_add(obj, sd->more);
   evas_object_show(sd->more);

   sd->bx_more = evas_object_box_add(evas_object_evas_get(obj));
   evas_object_box_align_set(sd->bx_more, sd->align, 0.5);
   evas_object_box_layout_set(sd->bx_more, _layout, obj, NULL);
   elm_widget_sub_object_add(obj, sd->bx_more);
   elm_layout_content_set
     (sd->more, "elm.swallow.content", sd->bx_more);
   evas_object_show(sd->bx_more);

   sd->bx_more2 = evas_object_box_add(evas_object_evas_get(obj));
   evas_object_box_align_set(sd->bx_more2, sd->align, 0.5);
   evas_object_box_layout_set(sd->bx_more2, _layout, obj, NULL);
   elm_widget_sub_object_add(obj, sd->bx_more2);
   elm_layout_content_set
     (sd->more, "elm.swallow.content2", sd->bx_more2);
   evas_object_show(sd->bx_more2);

   elm_toolbar_shrink_mode_set(obj, _elm_config->toolbar_shrink_mode);
   evas_object_event_callback_add(obj, EVAS_CALLBACK_MOVE, _move_cb, obj);
   // FIXME: There are applications which do not use elm_win as top widget.
   // This is workaround! Those could not use focus!
   evas_object_event_callback_add(obj, EVAS_CALLBACK_MOUSE_DOWN, _highlight_off_cb, obj);
   evas_object_event_callback_add
     (sd->bx, EVAS_CALLBACK_RESIZE, _resize_cb, obj);
   elm_toolbar_icon_order_lookup_set(obj, ELM_ICON_LOOKUP_THEME_FDO);

   _sizing_eval(obj);

   //Tizen Only: This should be removed when eo is applied.
   ELM_WIDGET_DATA_GET(obj, wsd);
   wsd->on_create = EINA_FALSE;
   return obj;
}

EAPI void
elm_toolbar_icon_size_set(Evas_Object *obj,
                          int icon_size)
{
   ELM_TOOLBAR_CHECK(obj);
   ELM_TOOLBAR_DATA_GET(obj, sd);

   if (sd->priv_icon_size == icon_size) return;
   sd->priv_icon_size = icon_size;

   if (sd->priv_icon_size) sd->icon_size = sd->priv_icon_size;
   else sd->icon_size = sd->theme_icon_size;

   _elm_toolbar_smart_theme(obj);
}

EAPI int
elm_toolbar_icon_size_get(const Evas_Object *obj)
{
   ELM_TOOLBAR_CHECK(obj) 0;
   ELM_TOOLBAR_DATA_GET(obj, sd);

   return sd->priv_icon_size;
}

EAPI Elm_Object_Item *
elm_toolbar_item_append(Evas_Object *obj,
                        const char *icon,
                        const char *label,
                        Evas_Smart_Cb func,
                        const void *data)
{
   Elm_Toolbar_Item *it;
   Elm_Object_Item *it_prev1, *it_prev2;
   double scale;

   ELM_TOOLBAR_CHECK(obj) NULL;
   ELM_TOOLBAR_DATA_GET(obj, sd);

   it = _item_new(obj, icon, label, func, data);
   if (!it) return NULL;
   scale = (elm_widget_scale_get(obj) * elm_config_scale_get());

   sd->items = eina_inlist_append(sd->items, EINA_INLIST_GET(it));
   evas_object_box_append(sd->bx, VIEW(it));
   _item_theme_hook(obj, it, scale, sd->icon_size);
   _sizing_eval(obj);
   sd->item_count++;

   //TIZEN ONLY
   //TO support the VI effect of Navigation style of toolbar
   if (!strcmp(elm_widget_style_get(obj), "navigationbar"))
     {
        evas_object_lower(VIEW(it));
        if (sd->item_count == 1)
          edje_object_signal_emit(ELM_WIDGET_DATA(sd)->resize_obj, "elm,order,one,item", "elm");
        else
          edje_object_signal_emit(ELM_WIDGET_DATA(sd)->resize_obj, "elm,order,stop,anim", "elm");
        elm_object_item_signal_emit(it, "elm,order,append,anim", "elm");
        it_prev1 = elm_toolbar_item_prev_get((Elm_Object_Item *)it);
        if (it_prev1)
          {
             it_prev2 = elm_toolbar_item_prev_get(it_prev1);
             if (it_prev2)
               elm_object_item_signal_emit(it_prev2, "elm,order,stop,divider,anim", "elm");
          }
        elm_object_item_signal_emit(it_prev1, "elm,order,divider,anim", "elm");
     }
   //
   return (Elm_Object_Item *)it;
}

EAPI Elm_Object_Item *
elm_toolbar_item_prepend(Evas_Object *obj,
                         const char *icon,
                         const char *label,
                         Evas_Smart_Cb func,
                         const void *data)
{
   Elm_Toolbar_Item *it;
   double scale;

   ELM_TOOLBAR_CHECK(obj) NULL;
   ELM_TOOLBAR_DATA_GET(obj, sd);

   it = _item_new(obj, icon, label, func, data);
   if (!it) return NULL;
   scale = (elm_widget_scale_get(obj) * elm_config_scale_get());

   sd->items = eina_inlist_prepend(sd->items, EINA_INLIST_GET(it));
   evas_object_box_prepend(sd->bx, VIEW(it));

   _item_theme_hook(obj, it, scale, sd->icon_size);
   _sizing_eval(obj);
   sd->item_count++;

   return (Elm_Object_Item *)it;
}

EAPI Elm_Object_Item *
elm_toolbar_item_insert_before(Evas_Object *obj,
                               Elm_Object_Item *before,
                               const char *icon,
                               const char *label,
                               Evas_Smart_Cb func,
                               const void *data)
{
   Elm_Toolbar_Item *it, *_before;
   double scale;

   ELM_TOOLBAR_CHECK(obj) NULL;
   ELM_TOOLBAR_ITEM_CHECK_OR_RETURN(before, NULL);
   ELM_TOOLBAR_DATA_GET(obj, sd);

   _before = (Elm_Toolbar_Item *)before;
   it = _item_new(obj, icon, label, func, data);
   if (!it) return NULL;
   scale = (elm_widget_scale_get(obj) * elm_config_scale_get());

   sd->items = eina_inlist_prepend_relative
       (sd->items, EINA_INLIST_GET(it), EINA_INLIST_GET(_before));
   evas_object_box_insert_before(sd->bx, VIEW(it), VIEW(_before));
   _item_theme_hook(obj, it, scale, sd->icon_size);
   _sizing_eval(obj);
   sd->item_count++;

   return (Elm_Object_Item *)it;
}

EAPI Elm_Object_Item *
elm_toolbar_item_insert_after(Evas_Object *obj,
                              Elm_Object_Item *after,
                              const char *icon,
                              const char *label,
                              Evas_Smart_Cb func,
                              const void *data)
{
   Elm_Toolbar_Item *it, *_after;
   double scale;

   ELM_TOOLBAR_CHECK(obj) NULL;
   ELM_TOOLBAR_ITEM_CHECK_OR_RETURN(after, NULL);

   ELM_TOOLBAR_DATA_GET(obj, sd);
   _after = (Elm_Toolbar_Item *)after;
   it = _item_new(obj, icon, label, func, data);
   if (!it) return NULL;
   scale = (elm_widget_scale_get(obj) * elm_config_scale_get());

   sd->items = eina_inlist_append_relative
       (sd->items, EINA_INLIST_GET(it), EINA_INLIST_GET(_after));
   evas_object_box_insert_after(sd->bx, VIEW(it), VIEW(_after));
   _item_theme_hook(obj, it, scale, sd->icon_size);
   _sizing_eval(obj);
   sd->item_count++;

   return (Elm_Object_Item *)it;
}

EAPI Elm_Object_Item *
elm_toolbar_first_item_get(const Evas_Object *obj)
{
   ELM_TOOLBAR_CHECK(obj) NULL;
   ELM_TOOLBAR_DATA_GET(obj, sd);

   if (!sd->items) return NULL;
   return (Elm_Object_Item *)ELM_TOOLBAR_ITEM_FROM_INLIST(sd->items);
}

EAPI Elm_Object_Item *
elm_toolbar_last_item_get(const Evas_Object *obj)
{
   ELM_TOOLBAR_CHECK(obj) NULL;
   ELM_TOOLBAR_DATA_GET(obj, sd);

   if (!sd->items) return NULL;

   return (Elm_Object_Item *)ELM_TOOLBAR_ITEM_FROM_INLIST(sd->items->last);
}

EAPI Elm_Object_Item *
elm_toolbar_item_next_get(const Elm_Object_Item *it)
{
   ELM_TOOLBAR_ITEM_CHECK_OR_RETURN(it, NULL);

   return (Elm_Object_Item *)ELM_TOOLBAR_ITEM_FROM_INLIST(
            EINA_INLIST_GET(((Elm_Toolbar_Item *)it))->next);
}

EAPI Elm_Object_Item *
elm_toolbar_item_prev_get(const Elm_Object_Item *it)
{
   ELM_TOOLBAR_ITEM_CHECK_OR_RETURN(it, NULL);

   return (Elm_Object_Item *)ELM_TOOLBAR_ITEM_FROM_INLIST(
            EINA_INLIST_GET(((Elm_Toolbar_Item *)it))->prev);
}

EAPI void
elm_toolbar_item_priority_set(Elm_Object_Item *it,
                              int priority)
{
   Elm_Toolbar_Item *item = (Elm_Toolbar_Item *)it;

   ELM_TOOLBAR_ITEM_CHECK_OR_RETURN(it);

   if (item->prio.priority == priority) return;
   item->prio.priority = priority;
   _resizing_eval(WIDGET(item), NULL);
}

EAPI int
elm_toolbar_item_priority_get(const Elm_Object_Item *it)
{
   ELM_TOOLBAR_ITEM_CHECK_OR_RETURN(it, 0);

   return ((Elm_Toolbar_Item *)it)->prio.priority;
}

EAPI Elm_Object_Item *
elm_toolbar_item_find_by_label(const Evas_Object *obj,
                               const char *label)
{
   Elm_Toolbar_Item *it;

   ELM_TOOLBAR_CHECK(obj) NULL;
   ELM_TOOLBAR_DATA_GET(obj, sd);

   EINA_INLIST_FOREACH(sd->items, it)
     {
        if (!strcmp(it->label, label))
          return (Elm_Object_Item *)it;
     }

   return NULL;
}

EAPI void
elm_toolbar_item_selected_set(Elm_Object_Item *it,
                              Eina_Bool selected)
{
   Elm_Toolbar_Item *item = (Elm_Toolbar_Item *)it;

   ELM_TOOLBAR_ITEM_CHECK_OR_RETURN(it);

   if (item->selected == selected) return;
   if (selected) _item_select(item);
   else _item_unselect(item);
}

EAPI Eina_Bool
elm_toolbar_item_selected_get(const Elm_Object_Item *it)
{
   ELM_TOOLBAR_ITEM_CHECK_OR_RETURN(it, EINA_FALSE);

   return ((Elm_Toolbar_Item *)it)->selected;
}

EAPI Elm_Object_Item *
elm_toolbar_selected_item_get(const Evas_Object *obj)
{
   ELM_TOOLBAR_CHECK(obj) NULL;
   ELM_TOOLBAR_DATA_GET(obj, sd);

   return (Elm_Object_Item *)sd->selected_item;
}

EAPI Elm_Object_Item *
elm_toolbar_more_item_get(const Evas_Object *obj)
{
   ELM_TOOLBAR_CHECK(obj) NULL;
   ELM_TOOLBAR_DATA_GET(obj, sd);

   return (Elm_Object_Item *)sd->more_item;
}

EAPI void
elm_toolbar_item_icon_set(Elm_Object_Item *it,
                          const char *icon)
{
   Evas_Object *obj;
   Evas_Object *icon_obj;
   Elm_Toolbar_Item *item = (Elm_Toolbar_Item *)it;

   ELM_TOOLBAR_ITEM_CHECK_OR_RETURN(it);

   obj = WIDGET(item);
   ELM_TOOLBAR_DATA_GET(obj, sd);
   if ((icon) && (item->icon_str) && (!strcmp(icon, item->icon_str))) return;

   icon_obj = elm_icon_add(obj);
   if (!icon_obj) return;
   if (_item_icon_set(icon_obj, "toolbar/", icon))
     _elm_toolbar_item_icon_obj_set
       (obj, item, icon_obj, icon, sd->icon_size, "elm,state,icon_set");
   else
     {
        _elm_toolbar_item_icon_obj_set
          (obj, item, NULL, NULL, 0, "elm,state,icon_set");
        evas_object_del(icon_obj);
     }
}

EAPI const char *
elm_toolbar_item_icon_get(const Elm_Object_Item *it)
{
   ELM_TOOLBAR_ITEM_CHECK_OR_RETURN(it, NULL);

   return ((Elm_Toolbar_Item *)it)->icon_str;
}

EAPI Evas_Object *
elm_toolbar_item_object_get(const Elm_Object_Item *it)
{
   Elm_Toolbar_Item *item = (Elm_Toolbar_Item *)it;

   ELM_TOOLBAR_ITEM_CHECK_OR_RETURN(it, NULL);

   return VIEW(item);
}

EAPI Evas_Object *
elm_toolbar_item_icon_object_get(Elm_Object_Item *it)
{
   ELM_TOOLBAR_ITEM_CHECK_OR_RETURN(it, NULL);

   return ((Elm_Toolbar_Item *)it)->icon;
}

EAPI Eina_Bool
elm_toolbar_item_icon_memfile_set(Elm_Object_Item *it,
                                  const void *img,
                                  size_t size,
                                  const char *format,
                                  const char *key)
{
   Elm_Toolbar_Item *item = (Elm_Toolbar_Item *)it;
   Evas_Object *icon_obj;
   Evas_Object *obj;
   Eina_Bool ret;

   ELM_TOOLBAR_ITEM_CHECK_OR_RETURN(it, EINA_FALSE);

   obj = WIDGET(item);
   ELM_TOOLBAR_DATA_GET(obj, sd);

   if (img && size)
     {
        icon_obj = elm_icon_add(obj);
        evas_object_repeat_events_set(icon_obj, EINA_TRUE);
        ret = elm_image_memfile_set(icon_obj, img, size, format, key);
        if (!ret)
          {
             evas_object_del(icon_obj);
             return EINA_FALSE;
          }
        _elm_toolbar_item_icon_obj_set
          (obj, item, icon_obj, NULL, sd->icon_size, "elm,state,icon_set");
     }
   else
     _elm_toolbar_item_icon_obj_set
       (obj, item, NULL, NULL, 0, "elm,state,icon_set");
   return EINA_TRUE;
}

EAPI Eina_Bool
elm_toolbar_item_icon_file_set(Elm_Object_Item *it,
                               const char *file,
                               const char *key)
{
   Elm_Toolbar_Item *item = (Elm_Toolbar_Item *)it;
   Evas_Object *icon_obj;
   Evas_Object *obj;
   Eina_Bool ret;

   ELM_TOOLBAR_ITEM_CHECK_OR_RETURN(it, EINA_FALSE);

   obj = WIDGET(item);
   ELM_TOOLBAR_DATA_GET(obj, sd);

   if (file)
     {
        icon_obj = elm_icon_add(obj);
        evas_object_repeat_events_set(icon_obj, EINA_TRUE);
        ret = elm_image_file_set(icon_obj, file, key);
        if (!ret)
          {
             evas_object_del(icon_obj);
             return EINA_FALSE;
          }
        _elm_toolbar_item_icon_obj_set
          (obj, item, icon_obj, NULL, sd->icon_size, "elm,state,icon_set");
     }
   else
     _elm_toolbar_item_icon_obj_set
       (obj, item, NULL, NULL, 0, "elm,state,icon_set");
   return EINA_TRUE;
}

EAPI void
elm_toolbar_item_separator_set(Elm_Object_Item *it,
                               Eina_Bool separator)
{
   Elm_Toolbar_Item *item = (Elm_Toolbar_Item *)it;
   Evas_Object *obj = WIDGET(item);
   double scale;

   ELM_TOOLBAR_ITEM_CHECK_OR_RETURN(it);
   ELM_TOOLBAR_DATA_GET(obj, sd);

   if (item->separator == separator) return;
   item->separator = separator;
   scale = (elm_widget_scale_get(obj) * elm_config_scale_get());
   _item_theme_hook(obj, item, scale, sd->icon_size);
   evas_object_size_hint_min_set(VIEW(item), -1, -1);
   if (separator) sd->separator_count++;
   else sd->separator_count--;
}

EAPI Eina_Bool
elm_toolbar_item_separator_get(const Elm_Object_Item *it)
{
   ELM_TOOLBAR_ITEM_CHECK_OR_RETURN(it, EINA_FALSE);

   return ((Elm_Toolbar_Item *)it)->separator;
}

EAPI void
elm_toolbar_shrink_mode_set(Evas_Object *obj,
                            Elm_Toolbar_Shrink_Mode shrink_mode)
{
   Eina_Bool bounce;

   ELM_TOOLBAR_CHECK(obj);
   ELM_TOOLBAR_DATA_GET(obj, sd);

   if (sd->shrink_mode == shrink_mode) return;
   sd->shrink_mode = shrink_mode;
   bounce = (_elm_config->thumbscroll_bounce_enable) &&
     (shrink_mode == ELM_TOOLBAR_SHRINK_SCROLL);
   sd->s_iface->bounce_allow_set(obj, bounce, EINA_FALSE);

   if (sd->more_item)
     {
        elm_widget_item_del(sd->more_item);
        sd->more_item = NULL;
     }

   if (shrink_mode == ELM_TOOLBAR_SHRINK_MENU)
     {
        elm_toolbar_homogeneous_set(obj, EINA_FALSE);
        sd->s_iface->policy_set
          (obj, ELM_SCROLLER_POLICY_OFF, ELM_SCROLLER_POLICY_OFF);
        sd->more_item = _item_new(obj, "more_menu", "More", NULL, NULL);
     }
   else if (shrink_mode == ELM_TOOLBAR_SHRINK_HIDE)
     {
        elm_toolbar_homogeneous_set(obj, EINA_FALSE);
        sd->s_iface->policy_set
          (obj, ELM_SCROLLER_POLICY_OFF, ELM_SCROLLER_POLICY_OFF);
     }
   else if (shrink_mode == ELM_TOOLBAR_SHRINK_EXPAND)
     {
        elm_toolbar_homogeneous_set(obj, EINA_FALSE);
        sd->s_iface->policy_set
          (obj, ELM_SCROLLER_POLICY_AUTO, ELM_SCROLLER_POLICY_OFF);
        sd->more_item = _item_new(obj, "more_menu", "More", NULL, NULL);
     }
   else
     sd->s_iface->policy_set
       (obj, ELM_SCROLLER_POLICY_AUTO, ELM_SCROLLER_POLICY_OFF);

   _sizing_eval(obj);
}

EAPI Elm_Toolbar_Shrink_Mode
elm_toolbar_shrink_mode_get(const Evas_Object *obj)
{
   ELM_TOOLBAR_CHECK(obj) ELM_TOOLBAR_SHRINK_NONE;
   ELM_TOOLBAR_DATA_GET(obj, sd);

   return sd->shrink_mode;
}

EAPI void
elm_toolbar_transverse_expanded_set(Evas_Object *obj, Eina_Bool transverse_expanded)
{
   ELM_TOOLBAR_CHECK(obj);
   ELM_TOOLBAR_DATA_GET(obj, sd);

   if (sd->transverse_expanded == transverse_expanded) return;
   sd->transverse_expanded = transverse_expanded;

   _sizing_eval(obj);
}

EAPI Eina_Bool
elm_toolbar_transverse_expanded_get(const Evas_Object *obj)
{
   ELM_TOOLBAR_CHECK(obj) EINA_FALSE;
   ELM_TOOLBAR_DATA_GET(obj, sd);

   return sd->transverse_expanded;
}

EAPI void
elm_toolbar_homogeneous_set(Evas_Object *obj,
                            Eina_Bool homogeneous)
{
   ELM_TOOLBAR_CHECK(obj);
   ELM_TOOLBAR_DATA_GET(obj, sd);

   homogeneous = !!homogeneous;
   if (homogeneous == sd->homogeneous) return;
   sd->homogeneous = homogeneous;
   if (homogeneous) elm_toolbar_shrink_mode_set(obj, ELM_TOOLBAR_SHRINK_NONE);
   evas_object_smart_calculate(sd->bx);
}

EAPI Eina_Bool
elm_toolbar_homogeneous_get(const Evas_Object *obj)
{
   ELM_TOOLBAR_CHECK(obj) EINA_FALSE;
   ELM_TOOLBAR_DATA_GET(obj, sd);

   return sd->homogeneous;
}

EAPI void
elm_toolbar_menu_parent_set(Evas_Object *obj,
                            Evas_Object *parent)
{
   Elm_Toolbar_Item *it;

   ELM_TOOLBAR_CHECK(obj);
   ELM_TOOLBAR_DATA_GET(obj, sd);
   EINA_SAFETY_ON_NULL_RETURN(parent);

   sd->menu_parent = parent;
   EINA_INLIST_FOREACH(sd->items, it)
     {
        if (it->o_menu)
          elm_menu_parent_set(it->o_menu, sd->menu_parent);
     }
   if ((sd->more_item) && (sd->more_item->o_menu))
     elm_menu_parent_set(sd->more_item->o_menu, sd->menu_parent);
}

EAPI Evas_Object *
elm_toolbar_menu_parent_get(const Evas_Object *obj)
{
   ELM_TOOLBAR_CHECK(obj) NULL;
   ELM_TOOLBAR_DATA_GET(obj, sd);

   return sd->menu_parent;
}

EAPI void
elm_toolbar_align_set(Evas_Object *obj,
                      double align)
{
   ELM_TOOLBAR_CHECK(obj);
   ELM_TOOLBAR_DATA_GET(obj, sd);

   if (sd->vertical)
     {
        if (sd->align != align)
          evas_object_box_align_set(sd->bx, 0.5, align);
     }
   else
     {
        if (sd->align != align)
          evas_object_box_align_set(sd->bx, align, 0.5);
     }
   sd->align = align;
}

EAPI double
elm_toolbar_align_get(const Evas_Object *obj)
{
   ELM_TOOLBAR_CHECK(obj) 0.0;
   ELM_TOOLBAR_DATA_GET(obj, sd);

   return sd->align;
}

EAPI void
elm_toolbar_item_menu_set(Elm_Object_Item *it,
                          Eina_Bool menu)
{
   Elm_Toolbar_Item *item = (Elm_Toolbar_Item *)it;

   ELM_TOOLBAR_ITEM_CHECK_OR_RETURN(it);
   ELM_TOOLBAR_DATA_GET(WIDGET(item), sd);

   if (item->menu == menu) return;
   if (menu) _item_menu_create(sd, item);
   else _item_menu_destroy(item);
}

EAPI Evas_Object *
elm_toolbar_item_menu_get(const Elm_Object_Item *it)
{
   Elm_Toolbar_Item *item = (Elm_Toolbar_Item *)it;

   ELM_TOOLBAR_ITEM_CHECK_OR_RETURN(it, NULL);

   if (!item->menu) return NULL;
   return item->o_menu;
}

EAPI Elm_Toolbar_Item_State *
elm_toolbar_item_state_add(Elm_Object_Item *it,
                           const char *icon,
                           const char *label,
                           Evas_Smart_Cb func,
                           const void *data)
{
   Elm_Toolbar_Item *item = (Elm_Toolbar_Item *)it;
   Elm_Toolbar_Item_State *it_state;
   Evas_Object *icon_obj;
   Evas_Object *obj;

   ELM_TOOLBAR_ITEM_CHECK_OR_RETURN(it, NULL);

   obj = WIDGET(item);
   ELM_TOOLBAR_DATA_GET(WIDGET(item), sd);

   if (!item->states)
     {
        it_state = _item_state_new
            (item->label, item->icon_str, item->icon, item->func,
            item->base.data);
        item->states = eina_list_append(item->states, it_state);
        item->current_state = item->states;
     }

   icon_obj = elm_icon_add(obj);
   elm_icon_order_lookup_set(icon_obj, sd->lookup_order);
   if (!icon_obj) goto error_state_add;

   if (!_item_icon_set(icon_obj, "toolbar/", icon))
     {
        evas_object_del(icon_obj);
        icon_obj = NULL;
        icon = NULL;
     }

   it_state = _item_state_new(label, icon, icon_obj, func, data);
   item->states = eina_list_append(item->states, it_state);
   item->func = _elm_toolbar_item_state_cb;
   item->base.data = NULL;

   return it_state;

error_state_add:
   if (item->states && !eina_list_next(item->states))
     {
        eina_stringshare_del(item->label);
        eina_stringshare_del(item->icon_str);
        free(eina_list_data_get(item->states));
        eina_list_free(item->states);
        item->states = NULL;
     }
   return NULL;
}

EAPI Eina_Bool
elm_toolbar_item_state_del(Elm_Object_Item *it,
                           Elm_Toolbar_Item_State *state)
{
   Elm_Toolbar_Item_State *it_state;
   Elm_Toolbar_Item *item;
   Eina_List *del_state;

   ELM_TOOLBAR_ITEM_CHECK_OR_RETURN(it, EINA_FALSE);

   if (!state) return EINA_FALSE;

   item = (Elm_Toolbar_Item *)it;
   if (!item->states) return EINA_FALSE;

   del_state = eina_list_data_find_list(item->states, state);
   if (del_state == item->states) return EINA_FALSE;
   if (del_state == item->current_state)
     elm_toolbar_item_state_unset(it);

   eina_stringshare_del(state->label);
   eina_stringshare_del(state->icon_str);
   if (state->icon) evas_object_del(state->icon);
   free(state);

   item->states = eina_list_remove_list(item->states, del_state);
   if (item->states && !eina_list_next(item->states))
     {
        it_state = eina_list_data_get(item->states);
        item->base.data = it_state->data;
        item->func = it_state->func;
        eina_stringshare_del(it_state->label);
        eina_stringshare_del(it_state->icon_str);
        free(eina_list_data_get(item->states));
        eina_list_free(item->states);
        item->states = NULL;
     }

   return EINA_TRUE;
}

EAPI Eina_Bool
elm_toolbar_item_state_set(Elm_Object_Item *it,
                           Elm_Toolbar_Item_State *state)
{
   Elm_Toolbar_Item *item = (Elm_Toolbar_Item *)it;
   Elm_Toolbar_Item_State *it_state;
   Eina_List *next_state;
   Evas_Object *obj;

   ELM_TOOLBAR_ITEM_CHECK_OR_RETURN(it, EINA_FALSE);

   obj = WIDGET(item);
   ELM_TOOLBAR_DATA_GET(obj, sd);
   if (!item->states) return EINA_FALSE;

   if (state)
     {
        next_state = eina_list_data_find_list(item->states, state);
        if (!next_state) return EINA_FALSE;
     }
   else
     next_state = item->states;

   if (next_state == item->current_state) return EINA_TRUE;

   it_state = eina_list_data_get(next_state);
   if (eina_list_data_find(item->current_state, state))
     {
        _item_label_set(item, it_state->label, "elm,state,label_set,forward");
        _elm_toolbar_item_icon_obj_set
          (obj, item, it_state->icon, it_state->icon_str,
          sd->icon_size, "elm,state,icon_set,forward");
     }
   else
     {
        _item_label_set(item, it_state->label, "elm,state,label_set,backward");
        _elm_toolbar_item_icon_obj_set
          (obj, item, it_state->icon, it_state->icon_str,
          sd->icon_size, "elm,state,icon_set,backward");
     }
   if (elm_widget_item_disabled_get(item))
     elm_widget_signal_emit(item->icon, "elm,state,disabled", "elm");
   else
     elm_widget_signal_emit(item->icon, "elm,state,enabled", "elm");

   item->current_state = next_state;

   if (_elm_config->access_mode == ELM_ACCESS_MODE_ON)
     {
        _access_widget_item_register(item);
     }

   return EINA_TRUE;
}

EAPI void
elm_toolbar_item_state_unset(Elm_Object_Item *it)
{
   elm_toolbar_item_state_set(it, NULL);
}

EAPI Elm_Toolbar_Item_State *
elm_toolbar_item_state_get(const Elm_Object_Item *it)
{
   Elm_Toolbar_Item *item = (Elm_Toolbar_Item *)it;

   ELM_TOOLBAR_ITEM_CHECK_OR_RETURN(it, NULL);

   if ((!item->states) || (!item->current_state)) return NULL;
   if (item->current_state == item->states) return NULL;

   return eina_list_data_get(item->current_state);
}

EAPI Elm_Toolbar_Item_State *
elm_toolbar_item_state_next(Elm_Object_Item *it)
{
   Eina_List *next_state;
   Elm_Toolbar_Item *item = (Elm_Toolbar_Item *)it;

   ELM_TOOLBAR_ITEM_CHECK_OR_RETURN(it, NULL);

   if (!item->states) return NULL;

   next_state = eina_list_next(item->current_state);
   if (!next_state)
     next_state = eina_list_next(item->states);
   return eina_list_data_get(next_state);
}

EAPI Elm_Toolbar_Item_State *
elm_toolbar_item_state_prev(Elm_Object_Item *it)
{
   Eina_List *prev_state;
   Elm_Toolbar_Item *item = (Elm_Toolbar_Item *)it;

   ELM_TOOLBAR_ITEM_CHECK_OR_RETURN(it, NULL);

   if (!item->states) return NULL;

   prev_state = eina_list_prev(item->current_state);
   if ((!prev_state) || (prev_state == item->states))
     prev_state = eina_list_last(item->states);
   return eina_list_data_get(prev_state);
}

EAPI void
elm_toolbar_icon_order_lookup_set(Evas_Object *obj,
                                  Elm_Icon_Lookup_Order order)
{
   Elm_Toolbar_Item *it;

   ELM_TOOLBAR_CHECK(obj);
   ELM_TOOLBAR_DATA_GET(obj, sd);

   if (sd->lookup_order == order) return;
   sd->lookup_order = order;
   EINA_INLIST_FOREACH(sd->items, it)
     elm_icon_order_lookup_set(it->icon, order);
   if (sd->more_item)
     elm_icon_order_lookup_set(sd->more_item->icon, order);
}

EAPI Elm_Icon_Lookup_Order
elm_toolbar_icon_order_lookup_get(const Evas_Object *obj)
{
   ELM_TOOLBAR_CHECK(obj) ELM_ICON_LOOKUP_THEME_FDO;
   ELM_TOOLBAR_DATA_GET(obj, sd);

   return sd->lookup_order;
}

EAPI void
elm_toolbar_horizontal_set(Evas_Object *obj,
                           Eina_Bool horizontal)
{
   ELM_TOOLBAR_CHECK(obj);
   ELM_TOOLBAR_DATA_GET(obj, sd);

   horizontal = !!horizontal;
   if (!horizontal == sd->vertical) return;
   sd->vertical = !horizontal;
   if (sd->vertical)
     evas_object_box_align_set(sd->bx, 0.5, sd->align);
   else
     evas_object_box_align_set(sd->bx, sd->align, 0.5);

   _sizing_eval(obj);
}

EAPI Eina_Bool
elm_toolbar_horizontal_get(const Evas_Object *obj)
{
   ELM_TOOLBAR_CHECK(obj) EINA_FALSE;
   ELM_TOOLBAR_DATA_GET(obj, sd);

   return !sd->vertical;
}

EAPI unsigned int
elm_toolbar_items_count(const Evas_Object *obj)
{
   ELM_TOOLBAR_CHECK(obj) 0;
   ELM_TOOLBAR_DATA_GET(obj, sd);

   return sd->item_count;
}

EAPI void
elm_toolbar_standard_priority_set(Evas_Object *obj,
                                  int priority)
{
   ELM_TOOLBAR_CHECK(obj);
   ELM_TOOLBAR_DATA_GET(obj, sd);

   if (sd->standard_priority == priority) return;
   sd->standard_priority = priority;
   _resizing_eval(obj, NULL);
}

EAPI int
elm_toolbar_standard_priority_get(const Evas_Object *obj)
{
   ELM_TOOLBAR_CHECK(obj) 0;
   ELM_TOOLBAR_DATA_GET(obj, sd);

   return sd->standard_priority;
}

EAPI void
elm_toolbar_select_mode_set(Evas_Object *obj,
                            Elm_Object_Select_Mode mode)
{
   ELM_TOOLBAR_CHECK(obj);
   ELM_TOOLBAR_DATA_GET(obj, sd);

   if (mode >= ELM_OBJECT_SELECT_MODE_MAX)
     return;

   if (sd->select_mode == mode) return;

   if ((mode == ELM_OBJECT_SELECT_MODE_ALWAYS) &&
       (sd->select_mode != ELM_OBJECT_SELECT_MODE_ALWAYS) &&
       sd->items)
     _item_select(ELM_TOOLBAR_ITEM_FROM_INLIST(sd->items));

   if (sd->select_mode != mode)
     sd->select_mode = mode;
}

EAPI Elm_Object_Select_Mode
elm_toolbar_select_mode_get(const Evas_Object *obj)
{
   ELM_TOOLBAR_CHECK(obj) ELM_OBJECT_SELECT_MODE_MAX;
   ELM_TOOLBAR_DATA_GET(obj, sd);

   return sd->select_mode;
}

EAPI void
elm_toolbar_reorder_mode_set(Evas_Object *obj,
                             Eina_Bool    reorder_mode)
{
   ELM_TOOLBAR_CHECK(obj);
   ELM_TOOLBAR_DATA_GET(obj, sd);

   sd->reorder_mode = !!reorder_mode;
}

EAPI Eina_Bool
elm_toolbar_reorder_mode_get(const Evas_Object *obj)
{
   ELM_TOOLBAR_CHECK(obj) EINA_FALSE;
   ELM_TOOLBAR_DATA_GET(obj, sd);

   return sd->reorder_mode;
}

EAPI void
elm_toolbar_item_show(Elm_Object_Item *it, Elm_Toolbar_Item_Scrollto_Type type)
{
   Evas_Coord x, y, w, h;
   Elm_Toolbar_Item *item = (Elm_Toolbar_Item *)it;

   ELM_TOOLBAR_ITEM_CHECK_OR_RETURN(it);
   ELM_TOOLBAR_DATA_GET(WIDGET(item), sd);

   if (_elm_toolbar_item_coordinates_calc(item, type, &x, &y, &w, &h))
     sd->s_iface->content_region_show(WIDGET(item), x, y, w, h);
}

EAPI void
elm_toolbar_item_bring_in(Elm_Object_Item *it, Elm_Toolbar_Item_Scrollto_Type type)
{
   Evas_Coord x, y, w, h;
   Elm_Toolbar_Item *item = (Elm_Toolbar_Item *)it;

   ELM_TOOLBAR_ITEM_CHECK_OR_RETURN(it);
   ELM_TOOLBAR_DATA_GET(WIDGET(item), sd);

   if (_elm_toolbar_item_coordinates_calc(item, type, &x, &y, &w, &h))
     sd->s_iface->region_bring_in(WIDGET(item), x, y, w, h);
}
