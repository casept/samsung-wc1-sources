#include <Elementary.h>
#include "elm_priv.h"
#include "els_box.h"
#include "elm_widget_index.h"

EAPI const char ELM_INDEX_SMART_NAME[] = "elm_index";

#define INDEX_DELAY_CHANGE_TIME 0.2

static const char SIG_CHANGED[] = "changed";
static const char SIG_DELAY_CHANGED[] = "delay,changed";
static const char SIG_SELECTED[] = "selected";
static const char SIG_LEVEL_UP[] = "level,up";
static const char SIG_LEVEL_DOWN[] = "level,down";
static const char SIG_LANG_CHANGED[] = "language,changed";

static const Evas_Smart_Cb_Description _smart_callbacks[] = {
   {SIG_CHANGED, ""},
   {SIG_DELAY_CHANGED, ""},
   {SIG_SELECTED, ""},
   {SIG_LEVEL_UP, ""},
   {SIG_LEVEL_DOWN, ""},
   {SIG_LANG_CHANGED, ""},
   {NULL, NULL}
};

EVAS_SMART_SUBCLASS_NEW
  (ELM_INDEX_SMART_NAME, _elm_index, Elm_Index_Smart_Class,
  Elm_Layout_Smart_Class, elm_layout_smart_class_get, _smart_callbacks);

static Eina_Bool
_elm_index_smart_translate(Evas_Object *obj)
{
   evas_object_smart_callback_call(obj, SIG_LANG_CHANGED, NULL);
   return EINA_TRUE;
}

static void
_item_free(Elm_Index_Item *it)
{
   ELM_INDEX_DATA_GET(WIDGET(it), sd);

   sd->items = eina_list_remove(sd->items, it);

   if (it->omitted)
     it->omitted = eina_list_free(it->omitted);

   if (it->letter)
     eina_stringshare_del(it->letter);
   if (it->icon)
     evas_object_del(it->icon);
}

static void
_box_custom_layout(Evas_Object *o,
                   Evas_Object_Box_Data *priv,
                   void *data)
{
   Elm_Index_Smart_Data *sd = data;

   _els_box_layout(o, priv, sd->horizontal, 0, 0);
}

static void
_index_box_clear(Evas_Object *obj,
                 Evas_Object *box,
                 int level)
{
   Eina_List *l;
   Elm_Index_Item *it;

   ELM_INDEX_DATA_GET(obj, sd);

   if (!sd->level_active[level]) return;

   EINA_LIST_FOREACH(sd->items, l, it)
     {
        if (!VIEW(it)) continue;
        if (it->level != level) continue;

        evas_object_del(VIEW(it));
        VIEW(it) = NULL;
     }

   evas_object_smart_calculate(box);
   sd->level_active[level] = 0;
}

static char *
_access_info_cb(void *data, Evas_Object *obj __UNUSED__)
{
   const char *txt = NULL;

   Elm_Index_Item *it = (Elm_Index_Item *)data;
   ELM_INDEX_ITEM_CHECK_OR_RETURN(it, NULL);

   txt = elm_widget_access_info_get(obj);
   if (!txt) txt = it->letter;
   if (txt) return strdup(txt);

   return NULL;
}

static void
_access_widget_item_activate_cb(void *data __UNUSED__,
                                Evas_Object *part_obj __UNUSED__,
                                Elm_Object_Item *it)
{
   evas_object_smart_callback_call(WIDGET(it), SIG_SELECTED, it);
}

static void
_access_widget_item_register(Elm_Index_Item *it)
{
   Elm_Access_Info *ai;
   _elm_access_widget_item_register((Elm_Widget_Item *)it);

   ai = _elm_access_object_get(it->base.access_obj);

   _elm_access_text_set(ai, ELM_ACCESS_TYPE, E_("Index Item"));//[string:error] not included in po file.
   _elm_access_callback_set(ai, ELM_ACCESS_INFO, _access_info_cb, it);
   _elm_access_activate_callback_set
     (ai, _access_widget_item_activate_cb, NULL);
}

static void
_omit_calc(void *data, int num_of_items, int max_num_of_items)
{
   Elm_Index_Smart_Data *sd = data;
   int max_group_num, num_of_extra_items, i, g, size, sum, start;
   int *group_pos, *omit_info;
   Elm_Index_Omit *o;

   if ((max_num_of_items < 3) || (num_of_items <= max_num_of_items)) return;

   if (sd->group_num > 0)
     start = sd->show_group + sd->default_num;
   else start = 0;
   max_group_num = (max_num_of_items - 1) / 2;
   num_of_extra_items = num_of_items - max_num_of_items;

   group_pos = (int *)malloc(sizeof(int) * max_group_num);
   omit_info = (int *)malloc(sizeof(int) * max_num_of_items);

   if (num_of_extra_items >= max_group_num)
     {
        g = 1;
        for (i = 0; i < max_group_num; i++)
          {
             group_pos[i] = g;
             g += 2;
          }
     }
   else
     {
        size = max_num_of_items / (num_of_extra_items + 1);
        g = size;
        for (i = 0; i < num_of_extra_items; i++)
          {
             group_pos[i] = g;
             g += size;
          }
     }
   for (i = 0; i < max_num_of_items; i++)
     omit_info[i] = 1;
   for (i = 0; i < num_of_extra_items; i++)
     omit_info[group_pos[i % max_group_num]]++;

   sum = 0;
   for (i = 0; i < max_num_of_items; i++)
     {
        if (omit_info[i] > 1)
          {
             o = (Elm_Index_Omit *)malloc(sizeof(Elm_Index_Omit));
             o->offset = sum + start;
             o->count = omit_info[i];
             sd->omit = eina_list_append(sd->omit, o);
          }
        sum += omit_info[i];
     }

   free(group_pos);
   free(omit_info);
}

// FIXME: always have index filled
static void
_index_box_auto_fill(Evas_Object *obj,
                     Evas_Object *box,
                     int level)
{
   int i = 0, max_num_of_items = 0, num_of_items = 0, g = 0, skip = 0;
   char buf[128];
   Eina_List *l;
   Eina_Bool rtl;
   Elm_Index_Item *it, *head = NULL, *last_it = NULL, *next_it = NULL;
   Evas_Coord mw, mh, ih;
   Evas_Object *o;
   Elm_Index_Omit *om;

   ELM_INDEX_DATA_GET(obj, sd);

   if (sd->level_active[level]) return;

   edje_object_part_geometry_get(ELM_WIDGET_DATA(sd)->resize_obj, "elm.swallow.index.0", NULL, NULL, NULL, &ih);

   rtl = elm_widget_mirrored_get(obj);

   EINA_LIST_FREE(sd->omit, om)
     free(om);

   EINA_LIST_FOREACH(sd->items, l, it)
     {
       if (it->level != level) continue;
       if (it->omitted)
          it->omitted = eina_list_free(it->omitted);
        if (it->head) it->head = NULL;
     }

   if (sd->omit_enabled)
     {
        o = edje_object_add(evas_object_evas_get(obj));

        snprintf(buf, sizeof(buf), "item%d/vertical", level + 1);
        elm_widget_theme_object_set
          (obj, o, "index", buf, elm_widget_style_get(obj));

        edje_object_size_min_restricted_calc(o, NULL, &mh, 0, 0);

        evas_object_del(o);
        if(level == 0)
          {
             EINA_LIST_FOREACH(sd->items, l, it)
                if (it->level == level && it->priority == sd->show_group)
                  num_of_items++;
          }
        else
          {
             EINA_LIST_FOREACH(sd->items, l, it)
                if (it->level == level )
                  num_of_items++;
          }

        if (mh != 0)
          max_num_of_items = ih / mh;
        if (sd->group_num)
          max_num_of_items -= (sd->group_num + sd->default_num - 1);

        _omit_calc(sd, num_of_items, max_num_of_items);
     }

   om = eina_list_nth(sd->omit, g);
   EINA_LIST_FOREACH(sd->items, l, it)
     {
        const char *stacking;

        if (it->level != level) continue;

        /** when index has more than one group,
          * one group is shown completely and other groups are represented by one item
          */
        if (it->priority != -1)
          {
             // for groups of higher priority, the first item represents the group
             if (it->priority < sd->show_group)
               {
                  if (last_it && (last_it->priority == it->priority)) continue;
               }
             // for groups of lower priority, the last item represents the group
             else if (it->priority > sd->show_group)
               {
                  l = eina_list_next(l);
                  if (l)
                    {
                       next_it = eina_list_data_get(l);
                       l = eina_list_prev(l);
                       if (next_it->priority == it->priority) continue;
                    }
               }
          }

        if ((om) && (i == om->offset))
          {
             skip = om->count;
             skip--;
             head = it;
             it->head = head;
             head->omitted = eina_list_append(head->omitted, it);
             om = eina_list_nth(sd->omit, ++g);
          }
        else if (skip > 0)
          {
             skip--;
             i++;
             if (head)
               {
                  it->head = head;
                  head->omitted = eina_list_append(head->omitted, it);
                  // if it is selected but omitted, send signal to it's head
                  if (it->selected)
                    edje_object_signal_emit(VIEW(it->head), "elm,state,active", "elm");
               }
             continue;
          }

        o = edje_object_add(evas_object_evas_get(obj));
        VIEW(it) = o;
        edje_object_mirrored_set(VIEW(it), rtl);


        if (!it->style)
         {
           if (sd->horizontal)
             {
                if (i & 0x1)
                  elm_widget_theme_object_set
                    (obj, o, "index", "item_odd/horizontal",
                    elm_widget_style_get(obj));
                else
                  elm_widget_theme_object_set
                    (obj, o, "index", "item/horizontal",
                    elm_widget_style_get(obj));
             }
           else
             {
                if (i & 0x1) snprintf(buf, sizeof(buf), "item%d_odd/vertical", level + 1);
                else snprintf(buf, sizeof(buf), "item%d/vertical", level + 1);
                elm_widget_theme_object_set
                  (obj, o, "index", buf, elm_widget_style_get(obj));
             }
         }
        else
           elm_widget_theme_object_set(obj, o, "index", it->style, elm_widget_style_get(obj));

        if (it->icon)
          edje_object_part_swallow(o, "elm.swallow.icon", it->icon);

        if (skip > 0)
          edje_object_part_text_escaped_set(o, "elm.text", "∗");
        else
          edje_object_part_text_escaped_set(o, "elm.text", it->letter);
        edje_object_size_min_restricted_calc(o, &mw, &mh, 0, 0);
        evas_object_size_hint_min_set(o, mw, mh);
        evas_object_size_hint_weight_set(o, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
        evas_object_size_hint_align_set(o, EVAS_HINT_FILL, EVAS_HINT_FILL);
        elm_widget_sub_object_add(obj, o);
        evas_object_box_append(box, o);
        stacking = edje_object_data_get(o, "stacking");

        if (it->selected)
          edje_object_signal_emit(o, "elm,state,active", "elm");

        if (stacking)
          {
             if (!strcmp(stacking, "below")) evas_object_lower(o);
             else if (!strcmp(stacking, "above"))
               evas_object_raise(o);
          }

        evas_object_show(o);

        i++;

        // ACCESS
        if ((it->level == 0) && (_elm_config->access_mode))
          _access_widget_item_register(it);

        last_it = it;
     }

   // TIZEN ONLY adjust the last item's theme according to winset gui
   Elm_Object_Item *first_it = eina_list_data_get(sd->items);
   if (first_it)
     edje_object_signal_emit(VIEW(first_it), "elm,first,item", "elm");
   if (last_it)
     edje_object_signal_emit(VIEW(last_it), "elm,last,item", "elm");
   // TIZEN ONLY

   evas_object_smart_calculate(box);
   sd->level_active[level] = 1;
}

static void
_priority_change_job(void *data)
{
   ELM_INDEX_DATA_GET(data, sd);
   Elm_Object_Item *selected_it;

   sd->show_group = sd->next_group;
   _index_box_clear(data, sd->bx[0], 0);
   _index_box_auto_fill(data, sd->bx[0], 0);

   selected_it = elm_index_selected_item_get(data, sd->level);
   if (selected_it)
     {
        elm_index_item_selected_set(selected_it, EINA_FALSE);
        if (sd->items && !sd->indicator_disabled)
          elm_layout_signal_emit(data, "elm,indicator,state,active", "elm");
     }
}

static void
_priority_up_cb(void *data)
{
   _priority_change_job(data);
   elm_layout_signal_emit(data, "elm,priority,up", "elm");
}

static void
_priority_down_cb(void *data)
{
   _priority_change_job(data);
   elm_layout_signal_emit(data, "elm,priority,down", "elm");
}

static void
_index_priority_change(void *data, Elm_Index_Item *it)
{
   ELM_INDEX_DATA_GET(data, sd);

   if ((it->priority != -1) && (it->priority != sd->show_group))
     {
        sd->next_group = it->priority;
        if (it->priority < sd->show_group)
          _priority_up_cb(data);
        else
          _priority_down_cb(data);
     }
}

static void
_mirrored_set(Evas_Object *obj, Eina_Bool rtl)
{
   ELM_INDEX_DATA_GET(obj, sd);

   if (!sd->horizontal)
     edje_object_mirrored_set(ELM_WIDGET_DATA(sd)->resize_obj, rtl);
}

static void
_access_activate_cb(void *data,
                    Evas_Object *part_obj __UNUSED__,
                    Elm_Object_Item *item __UNUSED__)
{
   Elm_Index_Item *it;
   ELM_INDEX_DATA_GET(data, sd);
   it = eina_list_nth(sd->items, 0);
   _elm_access_highlight_set(it->base.access_obj, EINA_FALSE);
   sd->index_focus = EINA_TRUE;
}

static void
_access_index_register(Evas_Object *obj)
{
   Evas_Object *ao;
   elm_widget_can_focus_set(obj, EINA_TRUE);

   ao = _elm_access_edje_object_part_object_register
              (obj, elm_layout_edje_get(obj), "access");

   // TIZEN ONLY (2013.12.09): read type only when used as fastscroll
   if (!strcmp((elm_object_style_get(obj)), "default"))
     {
        _elm_access_text_set
           (_elm_access_object_get(ao), ELM_ACCESS_TYPE, E_("IDS_TPLATFORM_BODY_FAST_SCROLL_BAR_T_TTS"));
        _elm_access_text_set
           (_elm_access_object_get(ao), ELM_ACCESS_CONTEXT_INFO,
            E_("IDS_TPLATFORM_BODY_DOUBLE_TAP_TO_ENABLE_FAST_SCROLLING_T_TTS"));
     }
   _elm_access_activate_callback_set(_elm_access_object_get(ao), _access_activate_cb, obj);
   // TIZEN ONLY
}

static Eina_Bool
_elm_index_smart_theme(Evas_Object *obj)
{
   Evas_Coord minw = 0, minh = 0;
   Elm_Index_Item *it;

   ELM_INDEX_DATA_GET(obj, sd);

   _index_box_clear(obj, sd->bx[0], 0);
   _index_box_clear(obj, sd->bx[1], 1);

   if (sd->horizontal)
     eina_stringshare_replace(&ELM_LAYOUT_DATA(sd)->group, "base/horizontal");
   else
     {
        eina_stringshare_replace(&ELM_LAYOUT_DATA(sd)->group, "base/vertical");
        _mirrored_set(obj, elm_widget_mirrored_get(obj));
     }

   if (!ELM_WIDGET_CLASS(_elm_index_parent_sc)->theme(obj)) return EINA_FALSE;

   elm_coords_finger_size_adjust(1, &minw, 1, &minh);
   evas_object_size_hint_min_set(sd->event[0], minw, minh);

   if (edje_object_part_exists
         (ELM_WIDGET_DATA(sd)->resize_obj, "elm.swallow.index.1"))
     {
        if (!sd->bx[1])
          {
             sd->bx[1] = evas_object_box_add(evas_object_evas_get(obj));
             evas_object_box_layout_set
               (sd->bx[1], _box_custom_layout, sd, NULL);
             elm_widget_sub_object_add(obj, sd->bx[1]);
          }
        elm_layout_content_set(obj, "elm.swallow.index.1", sd->bx[1]);
     }
   else if (sd->bx[1])
     {
        evas_object_del(sd->bx[1]);
        sd->bx[1] = NULL;
     }
   if (edje_object_part_exists
         (ELM_WIDGET_DATA(sd)->resize_obj, "elm.swallow.event.1"))
     {
        if (!sd->event[1])
          {
             sd->event[1] =
               evas_object_rectangle_add(evas_object_evas_get(obj));
             evas_object_color_set(sd->event[1], 0, 0, 0, 0);
             elm_widget_sub_object_add(obj, sd->event[1]);
          }
        elm_layout_content_set(obj, "elm.swallow.event.1", sd->event[1]);
        evas_object_size_hint_min_set(sd->event[1], minw, minh);
     }
   else if (sd->event[1])
     {
        evas_object_del(sd->event[1]);
        sd->event[1] = NULL;
     }
   edje_object_message_signal_process(ELM_WIDGET_DATA(sd)->resize_obj);

   elm_layout_sizing_eval(obj);
   _index_box_auto_fill(obj, sd->bx[0], 0);

   if (sd->autohide_disabled)
     {
        if (sd->level == 1) _index_box_auto_fill(obj, sd->bx[1], 1);
        elm_layout_signal_emit(obj, "elm,state,active", "elm");
     }
   else elm_layout_signal_emit(obj, "elm,state,inactive", "elm");

   it = (Elm_Index_Item *)elm_index_selected_item_get(obj, sd->level);
   if (it)
     {
        if (it->head)
          edje_object_signal_emit(VIEW(it->head), "elm,state,active", "elm");
        else
          edje_object_signal_emit(VIEW(it), "elm,state,active", "elm");
     }

   /* access */
   if (_elm_config->access_mode)
     {
        elm_index_autohide_disabled_set(obj, EINA_TRUE);
        elm_layout_signal_emit(obj, "elm,access,state,active", "elm");
        _access_index_register(obj);
     }

   return EINA_TRUE;
}

static void
_elm_index_smart_sizing_eval(Evas_Object *obj)
{
   Evas_Coord minw = -1, minh = -1, maxw = -1, maxh = -1;

   ELM_INDEX_DATA_GET(obj, sd);

   edje_object_size_min_calc(ELM_WIDGET_DATA(sd)->resize_obj, &minw, &minh);
   evas_object_size_hint_min_set(obj, minw, minh);
   evas_object_size_hint_max_set(obj, maxw, maxh);
}

static void
_item_style_set_hook(Elm_Object_Item *it, const char *style)
{
   Elm_Index_Item *item = NULL;

   ELM_INDEX_ITEM_CHECK(it);
   item = it;

   item->style = style;

   if (style)
     elm_widget_theme_object_set(WIDGET(it), VIEW(it), "index", style, elm_widget_style_get(WIDGET(it)));

   if (item->icon)
     edje_object_part_swallow(VIEW(it), "elm.swallow.icon", item->icon);
}

static const char *
_item_style_get_hook(Elm_Object_Item *it)
{
   ELM_INDEX_ITEM_CHECK_OR_RETURN(it, NULL);

   return(((Elm_Index_Item *)it)->style);
}

static Eina_Bool
_item_del_pre_hook(Elm_Object_Item *it)
{
   ELM_INDEX_DATA_GET(WIDGET(it), sd);

   _item_free((Elm_Index_Item *)it);
   _index_box_clear(WIDGET(it), sd->bx[sd->level], sd->level);

   return EINA_TRUE;
}

static void
_item_text_set_hook(Elm_Object_Item *it,
                    const char *part,
                    const char *label)
{
   Elm_Index_Item *index_it = (Elm_Index_Item *)it;
   char buf[256];

   ELM_INDEX_ITEM_CHECK_OR_RETURN(index_it);
   ELM_INDEX_DATA_GET(WIDGET(index_it), sd);

   if (!part || !strcmp(part, "default") || !strcmp(part, "elm.text"))
     snprintf(buf, sizeof(buf), "elm.text");
   else
     snprintf(buf, sizeof(buf), "%s", part);

   if (label)
     {
        if (index_it->letter)
          eina_stringshare_del(index_it->letter);
        index_it->letter = eina_stringshare_add(label);

        edje_object_part_text_escaped_set(VIEW(index_it), buf, index_it->letter);

        _index_box_clear(WIDGET(index_it), sd->bx[sd->level], sd->level);
        _index_box_auto_fill(WIDGET(index_it), sd->bx[sd->level], sd->level);
     }
}

static const char *
_item_text_get_hook(Elm_Object_Item *it,
                    const char *part)
{
   Elm_Index_Item *index_it = (Elm_Index_Item *)it;

   ELM_INDEX_ITEM_CHECK_OR_RETURN(index_it, NULL);

   /* FIXME: This function should be fixed with edje_object_part_text_get.
   Since VIEW(it) is made and then deleted everytime when index is
   going to calculated, edje function cannot be used now.
   After fixing this behavior, please use edje instead of elm_entry. */

   return elm_entry_markup_to_utf8(index_it->letter);
}

static void
_item_content_set_hook(Elm_Object_Item *it,
                       const char *part,
                       Evas_Object *content)
{
   Elm_Index_Item *item = (Elm_Index_Item *)it;

   if ((part) && (strcmp(part, "icon"))) return;
   item->icon = content;
}

static Evas_Object *
_item_content_get_hook(const Elm_Object_Item *it,
                       const char *part)
{
   Elm_Index_Item *item = (Elm_Index_Item *)it;

   if ((part) && (strcmp(part, "icon"))) return NULL;
   return item->icon;
}

static Evas_Object *
_item_content_unset_hook(const Elm_Object_Item *it,
                         const char *part)
{
   Elm_Index_Item *item = (Elm_Index_Item *)it;

   if ((part) && (strcmp(part, "icon"))) return NULL;

   Evas_Object *icon = item->icon;
   item->icon = NULL;
   return icon;
}

static Elm_Index_Item *
_item_new(Evas_Object *obj,
          const char *letter,
          Evas_Smart_Cb func,
          const void *data)
{
   Elm_Index_Item *it;

   ELM_INDEX_DATA_GET(obj, sd);

   it = elm_widget_item_new(obj, Elm_Index_Item);
   if (!it) return NULL;

   elm_widget_item_style_set_hook_set(it, _item_style_set_hook);
   elm_widget_item_style_get_hook_set(it, _item_style_get_hook);
   elm_widget_item_del_pre_hook_set(it, _item_del_pre_hook);
   elm_widget_item_text_set_hook_set(it, _item_text_set_hook);
   elm_widget_item_text_get_hook_set(it, _item_text_get_hook);
   elm_widget_item_content_set_hook_set(it, _item_content_set_hook);
   elm_widget_item_content_get_hook_set(it, _item_content_get_hook);
   elm_widget_item_content_unset_hook_set(it, _item_content_unset_hook);

   if (letter) it->letter = eina_stringshare_add(letter);
   it->func = func;
   it->base.data = data;
   it->level = sd->level;
   it->priority = -1;
   it->style = NULL;

   return it;
}

static Elm_Index_Item *
_item_find(Evas_Object *obj,
           const void *data)
{
   Eina_List *l;
   Elm_Index_Item *it;

   ELM_INDEX_DATA_GET(obj, sd);

   EINA_LIST_FOREACH(sd->items, l, it)
     if (it->base.data == data) return it;

   return NULL;
}

static Eina_Bool
_delay_change_cb(void *data)
{
   Elm_Object_Item *item;

   ELM_INDEX_DATA_GET(data, sd);

   sd->delay = NULL;
   item = elm_index_selected_item_get(data, sd->level);
   if (item)
     {
        evas_object_smart_callback_call(data, SIG_DELAY_CHANGED, item);
        _index_priority_change(data, (Elm_Index_Item *)item);
     }

   return ECORE_CALLBACK_CANCEL;
}

static void
_sel_eval(Evas_Object *obj,
          Evas_Coord evx,
          Evas_Coord evy)
{
   Evas_Coord x, y, w, h, bx, by, bw, bh, xx, yy;
   Elm_Index_Item *it, *it_closest = NULL, *it_last = NULL, *om_closest;
   char *label = NULL, *last = NULL;
   double cdv = 0.5;
   Evas_Coord dist;
   Eina_List *l;
   int i, j, size, dh, dx, dy;

   ELM_INDEX_DATA_GET(obj, sd);

   for (i = 0; i <= sd->level; i++)
     {
        it_last = NULL;
        it_closest = NULL;
        om_closest = NULL;
        dist = 0x7fffffff;
        evas_object_geometry_get(sd->bx[i], &bx, &by, &bw, &bh);

        EINA_LIST_FOREACH(sd->items, l, it)
          {
             if (it->level != i) continue;
             if (it->level != sd->level)
               {
                  if (it->selected)
                    {
                       it_closest = it;
                       break;
                    }
                  continue;
               }
             if (it->selected)
               {
                  it_last = it;
                  it->selected = 0;
               }
             if (VIEW(it))
               {
                  evas_object_geometry_get(VIEW(it), &x, &y, &w, &h);
                  xx = x + (w / 2);
                  yy = y + (h / 2);
                  x = evx - xx;
                  y = evy - yy;
                  x = (x * x) + (y * y);
                  if ((x < dist) || (!it_closest))
                    {
                       if (sd->horizontal)
                         cdv = (double)(xx - bx) / (double)bw;
                       else
                         cdv = (double)(yy - by) / (double)bh;
                       it_closest = it;
                       dist = x;
                    }
               }
          }
        if ((i == 0) && (sd->level == 0))
          edje_object_part_drag_value_set
            (ELM_WIDGET_DATA(sd)->resize_obj, "elm.dragable.index.1", cdv, cdv);

        if (it_closest && it_closest->omitted)
          {
             it = it_closest;
             size = eina_list_count(it->omitted);
             evas_object_geometry_get(VIEW(it), &x, &y, &w, &h);
             dist = 0x7fffffff;
             dh = h / size;
             if (dh == 0)
               printf("too many index items to omit\n"); //FIXME
             else
               {
                  for (j = 0; j < size; j++)
                    {
                       xx = x + (w / 2);
                       yy = y + (dh * j) + dh;
                       dx = evx - xx;
                       dy = evy - yy;
                       dx = (dx * dx) + (dy * dy);
                       if ((dx < dist) || (!om_closest))
                         {
                            om_closest = eina_list_nth(it->omitted, j);
                            dist = dx;
                         }
                    }
               }
          }

        if (om_closest) om_closest->selected = 1;
        else if (it_closest) it_closest->selected = 1;

        if (it_closest != it_last)
          {
             if (it_last)
               {
                  const char *stacking, *selectraise;

                  it = it_last;
                  if (it->head)
                    {
                       if (it->head != it_closest) it = it->head;
                       else it = NULL;
                    }
                  if (it)
                    {
                       edje_object_signal_emit
                          (VIEW(it), "elm,state,inactive", "elm");
                       stacking = edje_object_data_get(VIEW(it), "stacking");
                       selectraise = edje_object_data_get(VIEW(it), "selectraise");
                       if ((selectraise) && (!strcmp(selectraise, "on")))
                         {
                            if ((stacking) && (!strcmp(stacking, "below")))
                              evas_object_lower(VIEW(it));
                         }
                    }
               }
             if (it_closest)
               {
                  const char *selectraise;

                  it = it_closest;

                  if (!((it_last) && (it_last->head) && (it_last->head == it_closest)))
                    {
                       edje_object_signal_emit(VIEW(it), "elm,state,active", "elm");
                       selectraise = edje_object_data_get(VIEW(it), "selectraise");
                       if ((selectraise) && (!strcmp(selectraise, "on")))
                         evas_object_raise(VIEW(it));
                    }

                  if (om_closest && (om_closest->level == sd->level))
                    evas_object_smart_callback_call
                       (obj, SIG_CHANGED, om_closest);
                  else if(it->level == sd->level)
                    evas_object_smart_callback_call
                       (obj, SIG_CHANGED, it);
                  if (sd->delay) ecore_timer_del(sd->delay);
                  sd->delay = ecore_timer_add(sd->delay_change_time,
                                              _delay_change_cb, obj);
               }
          }
        if (it_closest)
          {
             if (om_closest && (om_closest->level == sd->level)) it = om_closest;
             else it = it_closest;
             if (!last && it->letter) last = strdup(it->letter);
             else
               {
                  if (!label && last) label = strdup(last);
                  else
                    {
                       if (label && last)
                         {
                            label = realloc(label, strlen(label) +
                                            strlen(last) + 1);
                            if (!label)
                              {
                                 free(last);
                                 return;
                              }
                            strcat(label, last);
                         }
                    }
                  free(last);
                  last = NULL;
                  if (it->letter) last = strdup(it->letter);
               }
          }
     }
   if (!label) label = strdup("");
   if (!last) last = strdup("");
   if ((sd->level) && it_last)
     {
        //Tizen only as alignment needs to change for popup text when 2nd depth text is set
        edje_object_signal_emit(ELM_WIDGET_DATA(sd)->resize_obj,"index1.text.enable", "");
        elm_layout_text_set(obj, "elm.text", label);
     }
   if(it_closest)
     elm_layout_text_set(obj, "elm.text.1", last);
   elm_layout_text_set(obj, "elm.text.body", label);

   free(label);
   free(last);
}

static void
_on_mouse_wheel(void *data __UNUSED__,
                Evas *e __UNUSED__,
                Evas_Object *o __UNUSED__,
                void *event_info __UNUSED__)
{
}

static void
_on_mouse_down(void *data,
               Evas *e __UNUSED__,
               Evas_Object *o __UNUSED__,
               void *event_info)
{
   Evas_Event_Mouse_Down *ev = event_info;
   Evas_Coord x, y, w;

   ELM_INDEX_DATA_GET(data, sd);

   if (ev->button != 1) return;
   sd->down = 1;
   evas_object_geometry_get(ELM_WIDGET_DATA(sd)->resize_obj, &x, &y, &w, NULL);
   sd->dx = ev->canvas.x - x;
   sd->dy = ev->canvas.y - y;
   if (!sd->autohide_disabled)
     {
        _index_box_clear(data, sd->bx[1], 1);
        elm_layout_signal_emit(data, "elm,state,active", "elm");
     }
   _sel_eval(data, ev->canvas.x, ev->canvas.y);
   edje_object_part_drag_value_set
     (ELM_WIDGET_DATA(sd)->resize_obj, "elm.dragable.pointer",
     (!elm_object_mirrored_get(data)) ? sd->dx : (sd->dx - w), sd->dy);
   if (sd->items && !sd->indicator_disabled)
     elm_layout_signal_emit(data, "elm,indicator,state,active", "elm");
}

static void
_on_mouse_up(void *data,
             Evas *e __UNUSED__,
             Evas_Object *o __UNUSED__,
             void *event_info)
{
   Evas_Event_Mouse_Up *ev = event_info;
   Elm_Object_Item *item, *prev_item;
   Elm_Index_Item *id_item;
   int i;
   char buf[32];

   ELM_INDEX_DATA_GET(data, sd);

   if (ev->button != 1) return;
   sd->down = 0;
   item = elm_index_selected_item_get(data, sd->level);
   i = sd->level - 1;
   while(!item && i >= 0)
     {
        item = elm_index_selected_item_get(data, i);
        i--;
     }

   if (item)
     {
        evas_object_smart_callback_call(data, SIG_SELECTED, item);
        id_item = (Elm_Index_Item *)item;
        if (id_item->func)
          id_item->func((void *)id_item->base.data, WIDGET(id_item), id_item);
     }
   if (!sd->autohide_disabled)
     elm_layout_signal_emit(data, "elm,state,inactive", "elm");

   prev_item = elm_index_selected_item_get(data, 0);
   if (prev_item)
     elm_index_item_selected_set(prev_item, EINA_FALSE);

   elm_layout_signal_emit(data, "elm,state,level,0", "elm");
   if (sd->items && !sd->indicator_disabled)
     elm_layout_signal_emit(data, "elm,indicator,state,inactive", "elm");

   if(sd->index_level > 0)
     {
        i = sd->index_level;
        while(i > 0)
          {
             snprintf(buf, sizeof(buf), "elm.swallow.event.%d", i);
             elm_layout_content_unset(data, buf);
             evas_object_hide(sd->event[i]);
             i--;
          }
     }
}

static void
_on_mouse_move(void *data,
               Evas *e __UNUSED__,
               Evas_Object *o __UNUSED__,
               void *event_info)
{
   Evas_Event_Mouse_Move *ev = event_info;
   Evas_Coord minw = 0, minh = 0, x, y, dx, adx, w, ox, oy, ow, oh;
   char buf[1024];

   ELM_INDEX_DATA_GET(data, sd);

   if (!sd->down) return;

   // TIZEN ONLY return if mouse pointer is out of index0 area and index 1 is not enabled
   evas_object_geometry_get(o, &ox, &oy, &ow, &oh);
   if ((!sd->index_level) && (!ELM_RECTS_INTERSECT
     (ev->cur.canvas.x, ev->cur.canvas.y, 1, 1, ox, oy, ow, oh))) return;
   // TIZEN ONLY

   elm_coords_finger_size_adjust(1, &minw, 1, &minh);
   evas_object_geometry_get(ELM_WIDGET_DATA(sd)->resize_obj, &x, &y, &w, NULL);
   x = ev->cur.canvas.x - x;
   y = ev->cur.canvas.y - y;
   dx = x - sd->dx;
   adx = dx;
   if (adx < 0) adx = -dx;
   edje_object_part_drag_value_set
     (ELM_WIDGET_DATA(sd)->resize_obj, "elm.dragable.pointer",
     (!edje_object_mirrored_get(ELM_WIDGET_DATA(sd)->resize_obj)) ?
     x : (x - w), y);
   if (!sd->horizontal && sd->index_level)
     {
        if (adx > minw)
          {
             if (!sd->level)
               {
                  sd->level = 1;
                  snprintf(buf, sizeof(buf), "elm,state,level,%i", sd->level);
                  elm_layout_signal_emit(data, buf, "elm");
                  evas_object_smart_callback_call(data, SIG_LEVEL_UP, NULL);
               }
          }
        else
          {
             if (sd->level == 1)
               {
                  elm_index_item_clear(data);
                  sd->level = 0;
                  snprintf(buf, sizeof(buf), "elm,state,level,%i", sd->level);
                  elm_layout_signal_emit(data, buf, "elm");
                  evas_object_smart_callback_call(data, SIG_LEVEL_DOWN, NULL);
               }
          }
     }
   _sel_eval(data, ev->cur.canvas.x, ev->cur.canvas.y);
}

static void
_index_resize_cb(void *data,
                 Evas *e __UNUSED__,
                 Evas_Object *obj __UNUSED__,
                 void *event_info __UNUSED__)
{
   ELM_INDEX_DATA_GET_OR_RETURN(data, sd);

   if (!sd->omit_enabled) return;

   Elm_Index_Item *it;
   int i;

   for (i = 0; i <= sd->level; i++)
     {
        _index_box_clear(data, sd->bx[i], i);
        _index_box_auto_fill(data, sd->bx[i], i);
     }

   it = (Elm_Index_Item *)elm_index_selected_item_get(obj, sd->level);
   if (it)
     {
        if (it->head)
          edje_object_signal_emit(VIEW(it->head), "elm,state,active", "elm");
        else
          edje_object_signal_emit(VIEW(it), "elm,state,active", "elm");
     }
}

static void
_elm_index_smart_add(Evas_Object *obj)
{
   EVAS_SMART_DATA_ALLOC(obj, Elm_Index_Smart_Data);

   ELM_WIDGET_CLASS(_elm_index_parent_sc)->base.add(obj);
}

static void
_elm_index_smart_del(Evas_Object *obj)
{
   Elm_Index_Item *it;
   Elm_Index_Omit *o;

   ELM_INDEX_DATA_GET(obj, sd);

   while (sd->items)
     {
        it = sd->items->data;
        elm_widget_item_del(it);
     }

   EINA_LIST_FREE(sd->omit, o)
     free(o);

   if (sd->delay) ecore_timer_del(sd->delay);
   if (sd->event[0]) evas_object_del(sd->event[0]);
   if (sd->event[1]) evas_object_del(sd->event[1]);

   ELM_WIDGET_CLASS(_elm_index_parent_sc)->base.del(obj);
}

static Eina_Bool
_elm_index_smart_focus_next(const Evas_Object *obj,
                            Elm_Focus_Direction dir,
                            Evas_Object **next)
{
   Eina_List *items = NULL;
   Eina_List *l = NULL;
   Elm_Index_Item *it;
   Evas_Object *ao;
   Evas_Object *po;
   Eina_Bool ret;

   ELM_INDEX_CHECK(obj) EINA_FALSE;
   ELM_INDEX_DATA_GET(obj, sd);

   if (!sd->autohide_disabled)
     elm_layout_signal_emit((Evas_Object *)obj, "elm,state,active", "elm");

   po = (Evas_Object *)edje_object_part_object_get
              (elm_layout_edje_get(obj), "access");
   ao = evas_object_data_get(po, "_part_access_obj");
   items = eina_list_append(items, ao);

   if (sd->index_focus)
     {
      EINA_LIST_FOREACH(sd->items, l, it)
        {
           if (it->level != 0) continue;
           items = eina_list_append(items, it->base.access_obj);
        }
     }

   ret = elm_widget_focus_list_next_get
            (obj, items, eina_list_data_get, dir, next);

   if (!ret)
     {
        sd->index_focus = EINA_FALSE;

        Evas_Object *it_access_obj = eina_list_nth(items, eina_list_count(items));

        items = eina_list_free(items);
        items = eina_list_append(items, it_access_obj);
        items = eina_list_append(items, ao);

        ret = elm_widget_focus_list_next_get(obj, items, eina_list_data_get, dir, next);

        // to hide index item, if there is nothing to focus on autohide disalbe mode
        if (!sd->autohide_disabled)
          elm_layout_signal_emit((Evas_Object *)obj, "elm,state,inactive", "elm");
     }

   return ret;
}

static void
_access_obj_process(Evas_Object *obj, Eina_Bool is_access)
{
   Eina_List *l;
   Elm_Index_Item *it;

   ELM_INDEX_DATA_GET(obj, sd);

   EINA_LIST_FOREACH(sd->items, l, it)
     {
        if (it->level != 0) continue;
        if (is_access) _access_widget_item_register(it);
        else _elm_access_widget_item_unregister((Elm_Widget_Item *)it);
     }

   if (is_access)
     {
        elm_index_autohide_disabled_set(obj, EINA_TRUE);
        elm_layout_signal_emit(obj, "elm,access,state,active", "elm");
        _access_index_register(obj);
     }
   else
     {
        // opposition of  _access_index_register();
        if (!sd->autohide_disabled)
          elm_index_autohide_disabled_set(obj, EINA_FALSE);
        elm_layout_signal_emit(obj, "elm,access,state,inactive", "elm");
        elm_widget_can_focus_set(obj, EINA_FALSE);
        _elm_access_edje_object_part_object_unregister
             (obj, elm_layout_edje_get(obj), "access");
     }
}

static void
_access_hook(Evas_Object *obj, Eina_Bool is_access)
{
   ELM_INDEX_CHECK(obj);
   ELM_INDEX_DATA_GET(obj, sd);

   if (is_access)
     ELM_WIDGET_CLASS(ELM_WIDGET_DATA(sd)->api)->focus_next =
     _elm_index_smart_focus_next;
   else
     ELM_WIDGET_CLASS(ELM_WIDGET_DATA(sd)->api)->focus_next = NULL;
   _access_obj_process(obj, is_access);
}

static void
_elm_index_smart_orientation_set(Evas_Object *obj, int rotation EINA_UNUSED)
{
   Elm_Index_Item *item;
   int i;

   ELM_INDEX_DATA_GET(obj, sd);

   i = sd->level;
   while(i >= 0)
     {
        item = (Elm_Index_Item *)elm_index_selected_item_get((const Evas_Object *)obj, i);
        if (item)
          elm_index_item_selected_set((Elm_Object_Item *)item, EINA_FALSE);
        i--;
     }
   sd->down = 0;
}

static void
_elm_index_smart_set_user(Elm_Index_Smart_Class *sc)
{
   ELM_WIDGET_CLASS(sc)->base.add = _elm_index_smart_add;
   ELM_WIDGET_CLASS(sc)->base.del = _elm_index_smart_del;

   ELM_WIDGET_CLASS(sc)->theme = _elm_index_smart_theme;
   ELM_WIDGET_CLASS(sc)->translate = _elm_index_smart_translate;
   ELM_WIDGET_CLASS(sc)->orientation_set = _elm_index_smart_orientation_set;

   /* not a 'focus chain manager' */
   ELM_WIDGET_CLASS(sc)->focus_next = NULL;
   ELM_WIDGET_CLASS(sc)->focus_direction_manager_is = NULL;
   ELM_WIDGET_CLASS(sc)->focus_direction = NULL;

   ELM_LAYOUT_CLASS(sc)->sizing_eval = _elm_index_smart_sizing_eval;

   if (_elm_config->access_mode)
     ELM_WIDGET_CLASS(sc)->focus_next = _elm_index_smart_focus_next;

   ELM_WIDGET_CLASS(sc)->access = _access_hook;
}

EAPI const Elm_Index_Smart_Class *
elm_index_smart_class_get(void)
{
   static Elm_Index_Smart_Class _sc =
     ELM_INDEX_SMART_CLASS_INIT_NAME_VERSION(ELM_INDEX_SMART_NAME);
   static const Elm_Index_Smart_Class *class = NULL;
   Evas_Smart_Class *esc = (Evas_Smart_Class *)&_sc;

   if (class)
     return class;

   _elm_index_smart_set(&_sc);
   esc->callbacks = _smart_callbacks;
   class = &_sc;

   return class;
}

EAPI Evas_Object *
elm_index_add(Evas_Object *parent)
{
   Evas_Object *obj;
   Evas_Object *o;
   Evas_Coord minw, minh;

   EINA_SAFETY_ON_NULL_RETURN_VAL(parent, NULL);

   obj = elm_widget_add(_elm_index_smart_class_new(), parent);
   if (!obj) return NULL;

   if (!elm_widget_sub_object_add(parent, obj))
     ERR("could not add %p as sub object of %p", obj, parent);

   ELM_INDEX_DATA_GET(obj, sd);

   elm_layout_theme_set
     (obj, "index", "base/vertical", elm_widget_style_get(obj));

   o = evas_object_rectangle_add(evas_object_evas_get(obj));
   sd->event[0] = o;
   evas_object_color_set(o, 0, 0, 0, 0);
   minw = minh = 0;
   elm_coords_finger_size_adjust(1, &minw, 1, &minh);
   evas_object_size_hint_min_set(o, minw, minh);
   elm_widget_sub_object_add(obj, o);

   evas_object_event_callback_add
     (obj, EVAS_CALLBACK_RESIZE, _index_resize_cb, obj);
   evas_object_event_callback_add
     (o, EVAS_CALLBACK_MOUSE_WHEEL, _on_mouse_wheel, obj);
   evas_object_event_callback_add
     (o, EVAS_CALLBACK_MOUSE_DOWN, _on_mouse_down, obj);
   evas_object_event_callback_add
     (o, EVAS_CALLBACK_MOUSE_UP, _on_mouse_up, obj);
   evas_object_event_callback_add
     (o, EVAS_CALLBACK_MOUSE_MOVE, _on_mouse_move, obj);

   if (edje_object_part_exists
         (ELM_WIDGET_DATA(sd)->resize_obj, "elm.swallow.event.1"))
     {
        o = evas_object_rectangle_add(evas_object_evas_get(obj));
        sd->event[1] = o;
        evas_object_color_set(o, 0, 0, 0, 0);
        evas_object_size_hint_min_set(o, minw, minh);
        elm_widget_sub_object_add(obj, o);
     }

   sd->bx[0] = evas_object_box_add(evas_object_evas_get(obj));
   evas_object_box_layout_set(sd->bx[0], _box_custom_layout, sd, NULL);
   elm_widget_sub_object_add(obj, sd->bx[0]);
   elm_layout_content_set(obj, "elm.swallow.index.0", sd->bx[0]);
   evas_object_show(sd->bx[0]);

   sd->delay_change_time = INDEX_DELAY_CHANGE_TIME;
   sd->omit_enabled = 1;
   sd->index_level = 0;

   if (edje_object_part_exists
         (ELM_WIDGET_DATA(sd)->resize_obj, "elm.swallow.index.1"))
     {
        sd->bx[1] = evas_object_box_add(evas_object_evas_get(obj));
        evas_object_box_layout_set
          (sd->bx[1], _box_custom_layout, sd, NULL);
        elm_widget_sub_object_add(obj, sd->bx[1]);
        elm_layout_content_set(obj, "elm.swallow.index.1", sd->bx[1]);
        evas_object_show(sd->bx[1]);
     }

   _mirrored_set(obj, elm_widget_mirrored_get(obj));
   elm_layout_sizing_eval(obj);
   elm_widget_can_focus_set(obj, EINA_FALSE);

   // ACCESS
   if (_elm_config->access_mode)
     {
        elm_index_autohide_disabled_set(obj, EINA_TRUE);
        elm_layout_signal_emit(obj, "elm,access,state,active", "elm");
        _access_index_register(obj);
     }

   //Tizen Only: This should be removed when eo is applied.
   ELM_WIDGET_DATA_GET(obj, wsd);
   wsd->on_create = EINA_FALSE;

   return obj;
}

EAPI void
elm_index_autohide_disabled_set(Evas_Object *obj,
                                Eina_Bool disabled)
{
   ELM_INDEX_CHECK(obj);
   ELM_INDEX_DATA_GET(obj, sd);

   disabled = !!disabled;
   if (sd->autohide_disabled == disabled) return;
   sd->autohide_disabled = disabled;
   sd->level = 0;
   if (sd->autohide_disabled)
     {
        _index_box_clear(obj, sd->bx[1], 1);
        elm_layout_signal_emit(obj, "elm,state,active", "elm");
     }
   else
     elm_layout_signal_emit(obj, "elm,state,inactive", "elm");

   //FIXME: Should be update indicator based on the indicator visiblility
}

EAPI Eina_Bool
elm_index_autohide_disabled_get(const Evas_Object *obj)
{
   ELM_INDEX_CHECK(obj) EINA_FALSE;
   ELM_INDEX_DATA_GET(obj, sd);

   return sd->autohide_disabled;
}

EAPI void
elm_index_item_level_set(Evas_Object *obj,
                         int level)
{
   ELM_INDEX_CHECK(obj);
   ELM_INDEX_DATA_GET(obj, sd);

   if (sd->level == level) return;
   //TIZEN ONLY : For supporting second-depth index, "index_level" stores
   // maximum level, and "level" stores current state.
   sd->index_level = level;

   // TIZEN ONLY
}

EAPI int
elm_index_item_level_get(const Evas_Object *obj)
{
   ELM_INDEX_CHECK(obj) 0;
   ELM_INDEX_DATA_GET(obj, sd);

   return sd->level;
}

//FIXME: Should update indicator based on the autohidden status & indicator visiblility
EAPI void
elm_index_item_selected_set(Elm_Object_Item *it,
                            Eina_Bool selected)
{
   ELM_INDEX_ITEM_CHECK_OR_RETURN(it);

   Elm_Index_Item *it_sel, *it_last, *it_inactive, *it_active;
   Evas_Object *obj = WIDGET(it);

   ELM_INDEX_DATA_GET(obj, sd);

   selected = !!selected;
   it_sel = (Elm_Index_Item *)it;
   if (it_sel->selected == selected) return;

   if (selected)
     {
        it_last = (Elm_Index_Item *)elm_index_selected_item_get(obj, sd->level);

        if (it_last)
          {
             it_last->selected = 0;
             if (it_last->head)
               it_inactive = it_last->head;
             else
               it_inactive = it_last;

             edje_object_signal_emit(VIEW(it_inactive),
                                     "elm,state,inactive", "elm");
             edje_object_message_signal_process(VIEW(it_inactive));
          }
        it_sel->selected = 1;
        if (it_sel->head)
          it_active = it_sel->head;
        else
          it_active = it_sel;

        edje_object_signal_emit(VIEW(it_active), "elm,state,active", "elm");
        edje_object_message_signal_process(VIEW(it_active));

        evas_object_smart_callback_call
           (obj, SIG_CHANGED, it);
        evas_object_smart_callback_call
           (obj, SIG_SELECTED, it);
        if (sd->delay) ecore_timer_del(sd->delay);
        sd->delay = ecore_timer_add(sd->delay_change_time,
                                    _delay_change_cb, obj);
     }
   else
     {
        it_sel->selected = 0;
        if (it_sel->head)
          it_inactive = it_sel->head;
        else
          it_inactive = it_sel;

        edje_object_signal_emit(VIEW(it_inactive), "elm,state,inactive", "elm");
        edje_object_message_signal_process(VIEW(it_inactive));

        // for the case in which the selected item is unselected before mouse up
        elm_layout_signal_emit(obj, "elm,indicator,state,inactive", "elm");
     }
}

EAPI Elm_Object_Item *
elm_index_selected_item_get(const Evas_Object *obj,
                            int level)
{
   Eina_List *l;
   Elm_Index_Item *it;

   ELM_INDEX_CHECK(obj) NULL;
   ELM_INDEX_DATA_GET(obj, sd);

   EINA_LIST_FOREACH(sd->items, l, it)
     {
        if ((it->selected) && (it->level == level))
          return (Elm_Object_Item *)it;
     }

   return NULL;
}

EAPI Elm_Object_Item *
elm_index_item_append(Evas_Object *obj,
                      const char *letter,
                      Evas_Smart_Cb func,
                      const void *data)
{
   Elm_Index_Item *it;

   ELM_INDEX_CHECK(obj) NULL;
   ELM_INDEX_DATA_GET(obj, sd);

   it = _item_new(obj, letter, func, data);
   if (!it) return NULL;

   sd->items = eina_list_append(sd->items, it);
   _index_box_clear(obj, sd->bx[sd->level], sd->level);

   return (Elm_Object_Item *)it;
}

EAPI Elm_Object_Item *
elm_index_item_prepend(Evas_Object *obj,
                       const char *letter,
                       Evas_Smart_Cb func,
                       const void *data)
{
   Elm_Index_Item *it;

   ELM_INDEX_CHECK(obj) NULL;
   ELM_INDEX_DATA_GET(obj, sd);

   it = _item_new(obj, letter, func, data);
   if (!it) return NULL;

   sd->items = eina_list_prepend(sd->items, it);
   _index_box_clear(obj, sd->bx[sd->level], sd->level);

   return (Elm_Object_Item *)it;
}

EINA_DEPRECATED EAPI Elm_Object_Item *
elm_index_item_prepend_relative(Evas_Object *obj,
                                const char *letter,
                                const void *item,
                                const Elm_Object_Item *relative)
{
   return elm_index_item_insert_before
            (obj, (Elm_Object_Item *)relative, letter, NULL, item);
}

EAPI Elm_Object_Item *
elm_index_item_insert_after(Evas_Object *obj,
                            Elm_Object_Item *after,
                            const char *letter,
                            Evas_Smart_Cb func,
                            const void *data)
{
   Elm_Index_Item *it;

   ELM_INDEX_CHECK(obj) NULL;
   ELM_INDEX_DATA_GET(obj, sd);

   if (!after) return elm_index_item_append(obj, letter, func, data);

   it = _item_new(obj, letter, func, data);
   if (!it) return NULL;

   sd->items = eina_list_append_relative(sd->items, it, after);
   _index_box_clear(obj, sd->bx[sd->level], sd->level);

   return (Elm_Object_Item *)it;
}

EAPI Elm_Object_Item *
elm_index_item_insert_before(Evas_Object *obj,
                             Elm_Object_Item *before,
                             const char *letter,
                             Evas_Smart_Cb func,
                             const void *data)
{
   Elm_Index_Item *it;

   ELM_INDEX_CHECK(obj) NULL;
   ELM_INDEX_DATA_GET(obj, sd);

   if (!before) return elm_index_item_prepend(obj, letter, func, data);

   it = _item_new(obj, letter, func, data);
   if (!it) return NULL;

   sd->items = eina_list_prepend_relative(sd->items, it, before);
   _index_box_clear(obj, sd->bx[sd->level], sd->level);

   return (Elm_Object_Item *)it;
}

EAPI Elm_Object_Item *
elm_index_item_sorted_insert(Evas_Object *obj,
                             const char *letter,
                             Evas_Smart_Cb func,
                             const void *data,
                             Eina_Compare_Cb cmp_func,
                             Eina_Compare_Cb cmp_data_func)
{
   Elm_Index_Item *it;
   Eina_List *lnear;
   int cmp;

   ELM_INDEX_CHECK(obj) NULL;
   ELM_INDEX_DATA_GET(obj, sd);

   if (!(sd->items)) return elm_index_item_append(obj, letter, func, data);

   it = _item_new(obj, letter, func, data);
   if (!it) return NULL;

   lnear = eina_list_search_sorted_near_list(sd->items, cmp_func, it, &cmp);
   if (cmp < 0)
     sd->items = eina_list_append_relative_list(sd->items, it, lnear);
   else if (cmp > 0)
     sd->items = eina_list_prepend_relative_list(sd->items, it, lnear);
   else
     {
        /* If cmp_data_func is not provided, append a duplicated item */
        if (!cmp_data_func)
          sd->items = eina_list_append_relative_list(sd->items, it, lnear);
        else
          {
             Elm_Index_Item *p_it = eina_list_data_get(lnear);
             if (cmp_data_func(p_it->base.data, it->base.data) >= 0)
               p_it->base.data = it->base.data;
             elm_widget_item_del(it);
             it = NULL;
          }
     }
   _index_box_clear(obj, sd->bx[sd->level], sd->level);

   if (!it) return NULL;
   return (Elm_Object_Item *)it;
}

EAPI Elm_Object_Item *
elm_index_item_find(Evas_Object *obj,
                    const void *data)
{
   ELM_INDEX_CHECK(obj) NULL;

   return (Elm_Object_Item *)_item_find(obj, data);
}

EAPI void
elm_index_item_clear(Evas_Object *obj)
{
   Elm_Index_Item *it;
   Eina_List *l, *clear = NULL;
   char buf[32];
   ELM_INDEX_CHECK(obj);
   ELM_INDEX_DATA_GET(obj, sd);

   _index_box_clear(obj, sd->bx[sd->level], sd->level);
   EINA_LIST_FOREACH(sd->items, l, it)
     {
        if (it->level != sd->level) continue;
        clear = eina_list_append(clear, it);
     }
   EINA_LIST_FREE (clear, it)
     elm_widget_item_del(it);

   snprintf(buf, sizeof(buf), "elm.swallow.event.%d", sd->level);
   elm_layout_content_unset(obj, buf);
   evas_object_hide(sd->event[sd->level]);
}

int
_sort_cb(const void *d1, const void *d2)
{
   Elm_Index_Item *it1 = (Elm_Index_Item *)d1, *it2 = (Elm_Index_Item *)d2;
   if (it1->priority <= it2->priority) return -1;
   else return 1;
}

EAPI void
elm_index_level_go(Evas_Object *obj,
                   int level )
{
   ELM_INDEX_CHECK(obj);
   ELM_INDEX_DATA_GET(obj, sd);

   Elm_Index_Item *it;
   Eina_List *l;
   int prev;
   char buf[20];

   sd->items = eina_list_sort(sd->items, 0, EINA_COMPARE_CB(_sort_cb));

   if(level == 0)
     {
        sd->default_num = 0;
        sd->group_num = 0;
        sd->show_group = -1;
        prev = -1;
        EINA_LIST_FOREACH(sd->items, l, it)
          {
             if (it->priority == -1) sd->default_num++;
             if (it->priority != prev)
               {
                  if (prev == -1) sd->show_group = it->priority;
                  sd->group_num++;
                  prev = it->priority;
               }
          }
        snprintf(buf, sizeof(buf), "elm.swallow.event.%d", level );
        elm_layout_content_set(obj, buf, sd->event[level]);
        evas_object_show(sd->event[level]);
     }

   _index_box_clear(obj, sd->bx[level], level);
   _index_box_auto_fill(obj, sd->bx[level], level);

   if (edje_object_part_exists
         (ELM_WIDGET_DATA(sd)->resize_obj, "elm.swallow.index.1") && level)
     {
        snprintf(buf, sizeof(buf), "elm.swallow.event.%d", level );
        elm_layout_content_set(obj, buf, sd->event[level]);
        evas_object_show(sd->event[level]);
     }
}

EAPI void
elm_index_indicator_disabled_set(Evas_Object *obj,
                                 Eina_Bool disabled)
{
   ELM_INDEX_CHECK(obj);
   ELM_INDEX_DATA_GET(obj, sd);

   disabled = !!disabled;
   if (sd->indicator_disabled == disabled) return;
   sd->indicator_disabled = disabled;
   if (!sd->items) return;
   if (disabled)
     elm_layout_signal_emit(obj, "elm,indicator,state,inactive", "elm");
   else
     elm_layout_signal_emit(obj, "elm,indicator,state,active", "elm");
}

EAPI Eina_Bool
elm_index_indicator_disabled_get(const Evas_Object *obj)
{
   ELM_INDEX_CHECK(obj) EINA_FALSE;
   ELM_INDEX_DATA_GET(obj, sd);

   return sd->indicator_disabled;
}

EAPI const char *
elm_index_item_letter_get(const Elm_Object_Item *it)
{
   ELM_INDEX_ITEM_CHECK_OR_RETURN(it, NULL);

   return ((Elm_Index_Item *)it)->letter;
}

EAPI void
elm_index_horizontal_set(Evas_Object *obj,
                         Eina_Bool horizontal)
{
   ELM_INDEX_CHECK(obj);
   ELM_INDEX_DATA_GET(obj, sd);

   horizontal = !!horizontal;
   if (horizontal == sd->horizontal) return;

   sd->horizontal = horizontal;
   if (horizontal)
     sd->omit_enabled = EINA_FALSE;
   _elm_index_smart_theme(obj);
}

EAPI Eina_Bool
elm_index_horizontal_get(const Evas_Object *obj)
{
   ELM_INDEX_CHECK(obj) EINA_FALSE;
   ELM_INDEX_DATA_GET(obj, sd);

   return sd->horizontal;
}

EAPI void
elm_index_delay_change_time_set(Evas_Object *obj, double delay_change_time)
{
   ELM_INDEX_CHECK(obj);
   ELM_INDEX_DATA_GET(obj, sd);

   sd->delay_change_time = delay_change_time;
}

EAPI double
elm_index_delay_change_time_get(const Evas_Object *obj)
{
   ELM_INDEX_CHECK(obj) 0.0;
   ELM_INDEX_DATA_GET(obj, sd);

   return sd->delay_change_time;
}

EAPI void
elm_index_omit_enabled_set(Evas_Object *obj,
                           Eina_Bool enabled)
{
   ELM_INDEX_CHECK(obj);
   ELM_INDEX_DATA_GET(obj, sd);

   if (sd->horizontal) return;

   enabled = !!enabled;
   if (sd->omit_enabled == enabled) return;
   sd->omit_enabled = enabled;

   _index_box_clear(obj, sd->bx[0], 0);
   _index_box_auto_fill(obj, sd->bx[0], 0);
   if (sd->level == 1)
     {
        _index_box_clear(obj, sd->bx[1], 1);
        _index_box_auto_fill(obj, sd->bx[1], 1);
     }
}

EAPI Eina_Bool
elm_index_omit_enabled_get(const Evas_Object *obj)
{
   ELM_INDEX_CHECK(obj) EINA_FALSE;
   ELM_INDEX_DATA_GET(obj, sd);

   return sd->omit_enabled;
}

EAPI void
elm_index_item_priority_set(Elm_Object_Item *it, int priority)
{
   ELM_INDEX_ITEM_CHECK(it);
   if (priority < -1)
     {
        WRN("priority value should be greater than or equal to -1.");
        return;
     }
   ((Elm_Index_Item *)it)->priority = priority;
}

EAPI void
elm_index_priority_set(Evas_Object *obj, int priority)
{
   ELM_INDEX_DATA_GET(obj, sd);
   if (priority < -1)
     {
        WRN("priority value should be greater than or equal to -1.");
        return;
     }
   if (priority != sd->show_group)
     {
        sd->next_group = priority;
        if (priority > sd->show_group)
          _priority_up_cb((void *)obj);
        else
          _priority_down_cb((void *)obj);
     }
}

EAPI int
elm_index_priority_get(const Evas_Object *obj)
{
   ELM_INDEX_DATA_GET(obj, sd);

   return sd->show_group;
}
