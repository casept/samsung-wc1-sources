#include <Elementary.h>
#include "elm_priv.h"
#include "elm_widget_multibuttonentry.h"

#define _VI_EFFECT

#ifdef _VI_EFFECT
#define TRANSIT_DURATION 0.167
#endif

EAPI const char ELM_MULTIBUTTONENTRY_SMART_NAME[] = "elm_multibuttonentry";

//widget signals
static const char SIG_ITEM_SELECTED[] = "item,selected";
static const char SIG_ITEM_ADDED[] = "item,added";
static const char SIG_ITEM_DELETED[] = "item,deleted";
static const char SIG_ITEM_CLICKED[] = "item,clicked";
static const char SIG_CLICKED[] = "clicked";
static const char SIG_FOCUSED[] = "focused";
static const char SIG_UNFOCUSED[] = "unfocused";
static const char SIG_EXPANDED[] = "expanded";
static const char SIG_CONTRACTED[] = "contracted";
static const char SIG_EXPAND_STATE_CHANGED[] = "expand,state,changed";
static const char SIG_LONGPRESSED[] = "longpressed";
static const char SIG_ACCESS_CHANGED[] = "access,changed";

static const Evas_Smart_Cb_Description _smart_callbacks[] = {
   {SIG_ITEM_SELECTED, ""},
   {SIG_ITEM_ADDED, ""},
   {SIG_ITEM_DELETED, ""},
   {SIG_ITEM_CLICKED, ""},
   {SIG_CLICKED, ""},
   {SIG_FOCUSED, ""},
   {SIG_UNFOCUSED, ""},
   {SIG_EXPANDED, ""},
   {SIG_CONTRACTED, ""},
   {SIG_EXPAND_STATE_CHANGED, ""},
   {SIG_LONGPRESSED, ""},
   {SIG_ACCESS_CHANGED, ""},
   {NULL, NULL}
};

EVAS_SMART_SUBCLASS_NEW
  (ELM_MULTIBUTTONENTRY_SMART_NAME, _elm_multibuttonentry,
  Elm_Multibuttonentry_Smart_Class, Elm_Layout_Smart_Class,
  elm_layout_smart_class_get, _smart_callbacks);

static void _on_item_clicked(void *data, Evas_Object *obj __UNUSED__, const char *emission __UNUSED__, const char *source __UNUSED__);
static void _on_item_focus2(void *data, Evas *e __UNUSED__, Evas_Object *obj, void *event_info __UNUSED__);

static Eina_Bool
_elm_multibuttonentry_smart_translate(Evas_Object *obj)
{
   ELM_MULTIBUTTONENTRY_DATA_GET(obj, sd);
   Elm_Multibuttonentry_Item *it;
   Eina_List *l;

   EINA_LIST_FOREACH(sd->items, l, it)
     elm_widget_item_translate(it);

   return EINA_TRUE;
}

static Eina_Bool
_elm_multibuttonentry_smart_event(Evas_Object *obj,
                                  Evas_Object *src __UNUSED__,
                                  Evas_Callback_Type type,
                                  void *event_info)
{
   Evas_Event_Key_Down *ev = event_info;

   ELM_MULTIBUTTONENTRY_DATA_GET(obj, sd);
   ELM_WIDGET_DATA_GET(obj, wsd);

   // TIZEN ONLY(20131221) : When access mode, focused ui is disabled.
   if (_elm_config->access_mode) return EINA_FALSE;

   if (elm_widget_disabled_get(obj)) return EINA_FALSE;
   if (type != EVAS_CALLBACK_KEY_DOWN) return EINA_FALSE;
   if (ev->event_flags & EVAS_EVENT_FLAG_ON_HOLD) return EINA_FALSE;
   if (!wsd->focus_highlighted) return EINA_TRUE;

   if (!strcmp(ev->keyname, "Return") ||
       !strcmp(ev->keyname, "KP_Enter"))
     {
        if (!sd->focused) return EINA_FALSE;
        Evas_Object *btn = elm_widget_focused_object_get(obj);

        if (VIEW(sd->focused) == btn)
          _on_item_clicked(sd->focused, NULL, NULL, NULL);
        else
          return EINA_FALSE;

        ev->event_flags |= EVAS_EVENT_FLAG_ON_HOLD;
        return EINA_TRUE;
     }

   return EINA_FALSE;
}

static Eina_Bool
_elm_multibuttonentry_smart_focus_direction_manager_is(const Evas_Object *obj __UNUSED__)
{
   return EINA_TRUE;
}

static Eina_Bool
_elm_multibuttonentry_smart_focus_direction(const Evas_Object *obj,
                                            const Evas_Object *base,
                                            double degree,
                                            Evas_Object **direction,
                                            double *weight)
{
   ELM_MULTIBUTTONENTRY_DATA_GET(obj, sd);

   if (sd->box)
     return elm_widget_focus_direction_get(sd->box, base, degree,
                                           direction,weight);
   return EINA_FALSE;
}

static Eina_Bool
_elm_multibuttonentry_smart_focus_next(const Evas_Object *obj,
                               Elm_Focus_Direction dir,
                               Evas_Object **next)
{
   Eina_List *items = NULL;
   Eina_List *elist = NULL;
   Elm_Multibuttonentry_Item *it;

   ELM_MULTIBUTTONENTRY_DATA_GET(obj, sd);

   if (!sd)
     return EINA_FALSE;

   if (_elm_config->access_mode)
     {
        if (!elm_widget_highlight_get(obj))
          {
             *next = (Evas_Object *)obj;
             return EINA_TRUE;
          }

        //TODO: enhance below lines
        if (!elm_widget_focus_next_get(sd->box, dir, next))
          {
             elm_widget_focused_object_clear(sd->box);
             elm_widget_focus_next_get(sd->box, dir, next);
          }

        if (sd->label)
          items = eina_list_append(items, elm_access_object_get(sd->label));

        EINA_LIST_FOREACH(sd->items, elist, it)
          items = eina_list_append(items, it->base.access_obj);

        return elm_widget_focus_list_next_get
                 (obj, items, eina_list_data_get, dir, next);
     }
   else
     {
        if (!elm_widget_focus_get(obj))
          {
             *next = (Evas_Object *)obj;
             return EINA_TRUE;
          }
        return elm_widget_focus_next_get(sd->box, dir, next);
     }
}

static char *
_access_label_info_cb(void *data, Evas_Object *obj __UNUSED__)
{
   Evas_Object *mbe = (Evas_Object *)data;
   const char *txt = NULL;

   ELM_MULTIBUTTONENTRY_DATA_GET(mbe, sd);

   if (!mbe) return NULL;

   txt = _elm_util_mkup_to_text(elm_object_part_text_get(sd->label, "mbe.label"));
   if (txt)
     return strdup(txt);
   else
     return NULL;
}

static char *
_access_shrink_info_cb(void *data, Evas_Object *obj __UNUSED__)
{
   Evas_Object *mbe = (Evas_Object *)data;
   int num = 0;
   char str[30];

   ELM_MULTIBUTTONENTRY_DATA_GET(mbe, sd);

   if (!mbe) return NULL;

   num = (int) evas_object_data_get(sd->end, "num");
   if (num)
     {
        snprintf(str, sizeof(str), E_("And %d more"), num);//[string:error] not included in po file.
        return strdup(str);
     }
   return NULL;
}

static char *
_access_mbe_info_cb(void *data, Evas_Object *obj __UNUSED__)
{
   Evas_Object *mbe = (Evas_Object *)data;
   char *txt = NULL;
   char *str = NULL;
   Eina_Strbuf *buf = NULL;
   Eina_List *l;
   Elm_Multibuttonentry_Item *it;

   ELM_MULTIBUTTONENTRY_DATA_GET(mbe, sd);

   if (!mbe) return NULL;

   txt = _access_label_info_cb(mbe, NULL);
   buf = eina_strbuf_new();
   if (txt)
     {
        eina_strbuf_append(buf, txt);
        eina_strbuf_append(buf, E_(", "));
        free(txt);
     }

   EINA_LIST_FOREACH(sd->items, l, it)
     {
        txt = (char *) elm_object_item_text_get((Elm_Object_Item *)it);
        if (txt && evas_object_visible_get(VIEW(it)))
          {
             eina_strbuf_append(buf, txt);
             eina_strbuf_append(buf, E_(", "));
          }
     }

   if (sd->end)
     {
        txt = _access_shrink_info_cb(mbe, NULL);
        if (txt)
          {
             eina_strbuf_append(buf, txt);
             free(txt);
          }
     }
   str = eina_strbuf_string_steal(buf);
   eina_strbuf_free(buf);
   return str;
}

static char *
_access_mbe_context_info_cb(void *data, Evas_Object *obj __UNUSED__)
{
   Evas_Object *mbe = (Evas_Object *)data;
   if (!mbe) return NULL;
   return strdup(E_("IDS_TPLATFORM_BODY_DOUBLE_TAP_TO_EDIT_THE_RECIPIENTS_T_TTS"));
}

static Eina_Bool
_guide_packed(Evas_Object *obj)
{
   Eina_List *children;

   ELM_MULTIBUTTONENTRY_DATA_GET(obj, sd);

   if (!sd->guide) return EINA_FALSE;

   children = elm_box_children_get(sd->box);
   if (sd->guide == eina_list_data_get(eina_list_last(children)))
     {
        eina_list_free(children);
        return EINA_TRUE;
     }
   else
     {
        eina_list_free(children);
        return EINA_FALSE;
     }
}

static Eina_Bool
_label_packed(Evas_Object *obj)
{
   Eina_List *children;

   ELM_MULTIBUTTONENTRY_DATA_GET(obj, sd);

   if (!sd->label) return EINA_FALSE;

   children = elm_box_children_get(sd->box);
   if (sd->label == eina_list_data_get(children))
     {
        eina_list_free(children);
        return EINA_TRUE;
     }
   else
     {
        eina_list_free(children);
        return EINA_FALSE;
     }
}

static Eina_Bool
_end_packed(Evas_Object *obj)
{
   Eina_List *children;

   ELM_MULTIBUTTONENTRY_DATA_GET(obj, sd);

   if (!sd->end) return EINA_FALSE;

   children = elm_box_children_get(sd->box);
   if (sd->end == eina_list_data_get(eina_list_last(children)))
     {
        eina_list_free(children);
        return EINA_TRUE;
     }
   else
     {
        eina_list_free(children);
        return EINA_FALSE;
     }
}

static void
_guide_set(Evas_Object *obj,
           const char *text)
{
   ELM_MULTIBUTTONENTRY_DATA_GET(obj, sd);

   if (!sd->guide && !strlen(text)) return;

   if (!sd->guide)
     {
        sd->guide = elm_layout_add(obj);
        elm_layout_theme_set
           (sd->guide, "multibuttonentry", "guidetext", elm_widget_style_get(obj));
        evas_object_size_hint_weight_set
           (sd->guide, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
        evas_object_size_hint_align_set
           (sd->guide, EVAS_HINT_FILL, EVAS_HINT_FILL);
        // TIZEN ONLY(20140624): below line should be checked with bounding box
        evas_object_smart_member_add(sd->guide, obj);
     }
   elm_object_text_set(sd->guide, text);

   if (!sd->items && !elm_object_focus_get(obj) && !_guide_packed(obj))
     {
        if (sd->editable)
          {
             elm_box_unpack(sd->box, sd->entry);
             evas_object_hide(sd->entry);
          }

        elm_box_pack_end(sd->box, sd->guide);
        // TIZEN ONLY(20140626): Until fixing bounding box issue, we send signal instead of "evas_object_show"
        elm_layout_signal_emit(sd->guide, "elm,state,guidetext,show", "elm");
     }
}

static void
_label_set(Evas_Object *obj,
           const char *text)
{
   ELM_MULTIBUTTONENTRY_DATA_GET(obj, sd);

   if (!sd->label && !strlen(text)) return;

   if (!sd->label)
     {
        sd->label = elm_layout_add(obj);
        elm_layout_theme_set
           (sd->label, "multibuttonentry", "label", elm_widget_style_get(obj));
     }
   elm_object_part_text_set(sd->label, "mbe.label", text);

   if (strlen(text) && !_label_packed(obj))
     {
        elm_box_pack_start(sd->box, sd->label);
        evas_object_show(sd->label);
     }
   else if (!strlen(text) && _label_packed(obj))
     {
        elm_box_unpack(sd->box, sd->label);
        evas_object_hide(sd->label);
     }

   // ACCESS
   if (_elm_config->access_mode)
     {
        Evas_Object *ao = elm_access_object_register(sd->label, obj);
        _elm_access_callback_set(_elm_access_object_get(ao), ELM_ACCESS_INFO, _access_label_info_cb, obj);
     }
}

static const char *
_end_set(Evas_Object *obj)
{
   const char *str = NULL;

   ELM_MULTIBUTTONENTRY_DATA_GET(obj, sd);

   str = elm_layout_data_get(obj, "closed_button_type");
   if (!sd->end)
     {
        sd->end = elm_layout_add(obj);
        if (str && !strcmp(str, "image"))
          elm_layout_theme_set(sd->end, "multibuttonentry",
                                "closedbutton", elm_widget_style_get(obj));
        else
          elm_layout_theme_set(sd->end, "multibuttonentry",
                                "number", elm_widget_style_get(obj));
     }
   else
     {
        elm_layout_signal_emit(sd->end, "elm,state,number,default", "");
        edje_object_message_signal_process(elm_layout_edje_get(sd->end));
     }
   evas_object_data_set(obj, "end", sd->end);
   return str;
}

#ifdef _VI_EFFECT
static void
_on_item_expanding_transit_del(void *data,
                               Elm_Transit *transit __UNUSED__)
{
   Elm_Multibuttonentry_Item *it = data;

   evas_object_data_set(VIEW(it), "transit", NULL);
   evas_object_smart_callback_call(WIDGET(it), SIG_ITEM_ADDED, it);
}

static void
_item_adding_effect_add(Evas_Object *obj,
                        Elm_Multibuttonentry_Item *it)
{
   Elm_Transit *trans;

   ELM_MULTIBUTTONENTRY_DATA_GET(obj, sd);

   Eina_List *cur;
   cur = eina_list_data_find_list(sd->items, it);
   if (cur == sd->items)
     {
        if (sd->label && _label_packed(obj))
          elm_box_pack_after(sd->box, VIEW(it), sd->label);
        else
          elm_box_pack_start(sd->box, VIEW(it));
     }
   else
     {
        Elm_Multibuttonentry_Item *prev_it;
        prev_it = eina_list_data_get(eina_list_prev(cur));
        elm_box_pack_after(sd->box, VIEW(it), VIEW(prev_it));
     }
   evas_object_show(VIEW(it));

   trans = elm_transit_add();
   elm_transit_object_add(trans, VIEW(it));
   elm_transit_effect_zoom_add(trans, 0.9, 1.0);
   elm_transit_effect_color_add(trans, 0, 0, 0, 0, 255, 255, 255, 255);
   elm_transit_del_cb_set(trans, _on_item_expanding_transit_del, it);
   elm_transit_duration_set(trans, TRANSIT_DURATION);
   evas_object_data_set(VIEW(it), "transit", trans);
   elm_transit_go(trans);
}

static void
_on_item_contracting_transit_del(void *data,
                                 Elm_Transit *transit __UNUSED__)
{
   Elm_Multibuttonentry_Item *it = data;

   ELM_MULTIBUTTONENTRY_DATA_GET(WIDGET(it), sd);

   evas_object_data_set(VIEW(it), "transit", NULL);

   // delete item and set focus to entry
   if (sd->editable && elm_object_focus_get(WIDGET(it)))
     elm_object_focus_set(sd->entry, EINA_TRUE);

   elm_object_item_del((Elm_Object_Item *)it);
}

static void
_item_deleting_effect_add(Elm_Multibuttonentry_Item *it)
{
   Elm_Transit *trans;

   trans = elm_transit_add();
   elm_transit_object_add(trans, VIEW(it));
   elm_transit_effect_zoom_add(trans, 1.0, 0.9);
   elm_transit_effect_color_add(trans, 255, 255, 255, 255, 0, 0, 0, 0);
   elm_transit_del_cb_set(trans, _on_item_contracting_transit_del, it);
   elm_transit_duration_set(trans, TRANSIT_DURATION);
   evas_object_data_set(VIEW(it), "transit", trans);
   elm_transit_go(trans);
}
#endif

static void
_adjust_first_item(Evas_Object *obj, Evas_Coord linew, int *adjustw)
{
   Evas_Coord mnw, mnh, w, labelw, itemw = 0, hpad = 0;
   Elm_Multibuttonentry_Item *item;

   ELM_MULTIBUTTONENTRY_DATA_GET(obj, sd);

   if (!sd->end) return;

   evas_object_geometry_get(sd->box, NULL, NULL, &w, NULL);
   elm_box_padding_get(sd->box, &hpad, NULL);

   evas_object_size_hint_min_get(sd->end, &mnw, NULL);

   item = eina_list_nth(sd->items, 0);
   itemw = (Evas_Coord)evas_object_data_get(VIEW(item), "maximum_width");
   if (itemw)
     evas_object_size_hint_min_get(VIEW(item), NULL, &mnh);
   else
     evas_object_size_hint_min_get(VIEW(item), &itemw, &mnh);

   linew -= itemw;
   if (sd->label && _label_packed(obj))
     {
        evas_object_size_hint_min_get(sd->label, &labelw, NULL);
        if (itemw > w - (labelw + hpad + mnw + hpad))
          {
             evas_object_data_set(VIEW(item), "shrinked_item", (void *)itemw);
             itemw = w - (labelw + hpad + mnw + hpad);
             elm_layout_signal_emit(VIEW(item), "elm,state,text,ellipsis", "");
          }
     }
   else if (itemw > w - (mnw + hpad))
     {
        evas_object_data_set(VIEW(item), "shrinked_item", (void *)itemw);
        itemw = w - (mnw + hpad);
        elm_layout_signal_emit(VIEW(item), "elm,state,text,ellipsis", "");
     }
   linew += itemw;
   edje_object_message_signal_process(elm_layout_edje_get(VIEW(item)));
   evas_object_size_hint_min_set(VIEW(item), itemw, mnh);
   evas_object_resize(VIEW(item), itemw, mnh);
   *adjustw = linew;
}

static void
_layout_expand(Evas_Object *obj)
{
   int count, items_count, i;
   Eina_List *children;
   Elm_Multibuttonentry_Item *it, *it1;
   Evas_Coord itemw = 0, mnh = 0, boxw = 0;

   ELM_MULTIBUTTONENTRY_DATA_GET(obj, sd);

   if (sd->expanded_state) return;
   if (!sd->items)
     {
        if (sd->end && _end_packed(obj))
          {
             evas_object_data_set(obj, "end", NULL);
             elm_box_unpack(sd->box, sd->end);
             evas_object_del(sd->end);
             sd->end = NULL;
          }
        goto done;
     }

   children = elm_box_children_get(sd->box);
   count = eina_list_count(children);
   if (sd->end)
     {
        evas_object_data_set(obj, "end", NULL);
        evas_object_data_set(sd->end, "num", NULL);
        evas_object_del(sd->end);
        sd->end = NULL;
        count--;
     }
   if (sd->label && _label_packed(obj)) count--;
   eina_list_free(children);

   // to check whether 1st item is shrinked (ex. multibutton@entry... +N)
   it1 = eina_list_nth(sd->items, 0);
   evas_object_geometry_get(sd->box, NULL, NULL, &boxw, NULL);

   itemw = (Evas_Coord)evas_object_data_get(VIEW(it1), "shrinked_item");
   if (itemw)
     {
        evas_object_size_hint_min_get(VIEW(it1), NULL, &mnh);
        elm_layout_signal_emit(VIEW(it1), "elm,state,text,ellipsis", "");
        edje_object_message_signal_process(elm_layout_edje_get(VIEW(it1)));
        if (sd->boxw && itemw > boxw)
          {
             evas_object_size_hint_min_set(VIEW(it1), boxw, mnh);
             evas_object_resize(VIEW(it1), boxw, mnh);
          }
        else
          {
             evas_object_size_hint_min_set(VIEW(it1), itemw, mnh);
             evas_object_resize(VIEW(it1), itemw, mnh);
          }
     }

   items_count = eina_list_count(sd->items);
   for (i = count; i < items_count; i++)
     {
        it = eina_list_nth(sd->items, i);
        elm_box_pack_end(sd->box, VIEW(it));
        evas_object_size_hint_min_get(VIEW(it), &itemw, &mnh);
        if (sd->boxw && itemw > boxw)
          {
             elm_layout_signal_emit(VIEW(it), "elm,state,text,ellipsis", "");
             edje_object_message_signal_process(elm_layout_edje_get(VIEW(it)));
             evas_object_size_hint_min_set(VIEW(it), boxw, mnh);
             evas_object_resize(VIEW(it), boxw, mnh);
          }
        evas_object_show(VIEW(it));
     }

done:
   if (sd->editable && elm_object_focus_get(obj))
     {
        elm_box_pack_end(sd->box, sd->entry);
        evas_object_show(sd->entry);
     }

   sd->expanded_state = EINA_TRUE;
   evas_object_smart_callback_call(obj, SIG_EXPAND_STATE_CHANGED, NULL);
}

static void
_layout_shrink(Evas_Object *obj,
               Eina_Bool force)
{
   Evas_Coord w, mnw, mnh, linew = 0, hpad = 0, labelw, endw, adjustw = 0;;
   int count = 0, items_count, i;
   Eina_List *l, *children;
   Evas_Object *child;
   Elm_Multibuttonentry_Item *it;
#ifdef _VI_EFFECT
   Elm_Transit *trans;
#endif

   ELM_MULTIBUTTONENTRY_DATA_GET(obj, sd);

   if (!sd->items) return;
   if (!sd->expanded_state && !force) return;
   if (!sd->boxw) return;

   evas_object_geometry_get(sd->box, NULL, NULL, &w, NULL);
   elm_box_padding_get(sd->box, &hpad, NULL);

   if (sd->label && _label_packed(obj))
     {
        evas_object_size_hint_min_get(sd->label, &mnw, NULL);
        if (mnw > w) return;
        linew += mnw;
        linew += hpad;
     }

   EINA_LIST_FOREACH(sd->items, l, it)
     {
        evas_object_size_hint_min_get(VIEW(it), &mnw, &mnh);
        linew += mnw;

        if (linew > w)
          {
             linew -= mnw;
             if (count)
               break;
             else if (sd->label && _label_packed(obj))
               {
                  // if 1st item is removed, next item should be recalculated
                  evas_object_size_hint_min_get(sd->label, &labelw, NULL);
                  if (sd->end)
                    {
                       evas_object_size_hint_min_get(sd->end, &endw, NULL);
                       if (mnw > w - (labelw + hpad + endw + hpad))
                         {
                            mnw = w - (labelw + hpad + endw + hpad);
                            elm_layout_signal_emit(VIEW(it), "elm,state,text,ellipsis", "");
                         }
                    }
                  else if (mnw > w - (labelw + hpad))
                    {
                       mnw = w - (labelw + hpad);
                       elm_layout_signal_emit(VIEW(it), "elm,state,text,ellipsis", "");
                    }

                  edje_object_message_signal_process(elm_layout_edje_get(VIEW(it)));
                  evas_object_size_hint_min_set(VIEW(it), mnw, mnh);
                  evas_object_resize(VIEW(it), mnw, mnh);
               }
          }

        count++;
        linew += hpad;
     }

   if (!count) return;

   const char *str = NULL;
   str = _end_set(obj);
   items_count = eina_list_count(sd->items);
   if (count < items_count)
     {
        char buf[16];
        if (!str || strcmp(str, "image"))
          {
             snprintf(buf, sizeof(buf), "+%d", items_count - count);
             elm_object_text_set(sd->end, buf);
             evas_object_data_set(sd->end, "num",
                                  (void *)(items_count - count));
          }
        evas_object_smart_calculate(sd->end);
        _adjust_first_item(obj, linew, &adjustw);
        linew = adjustw;

        evas_object_size_hint_min_get(sd->end, &mnw, NULL);
        linew += mnw;

        if (linew > w)
          {
             count--;
             if (!str || strcmp(str, "image"))
               {
                  snprintf(buf, sizeof(buf), "+%d", items_count - count);
                  elm_object_text_set(sd->end, buf);
                  evas_object_data_set(sd->end, "num",
                                       (void *)(items_count - count));
               }
          }

        if (!force)
          {
             for (i = count; i < items_count; i++)
               {
                  it = eina_list_nth(sd->items, i);
#ifdef _VI_EFFECT
                  // reset all effects
                  trans = (Elm_Transit *)evas_object_data_get(VIEW(it), "transit");
                  if (trans) elm_transit_del(trans);
#endif
                  elm_box_unpack(sd->box, VIEW(it));
                  evas_object_hide(VIEW(it));
               }

             if (sd->editable)
               {
                  if (elm_object_focus_get(sd->entry))
                    elm_object_focus_set(sd->entry, EINA_FALSE);
                  elm_box_unpack(sd->box, sd->entry);
                  evas_object_hide(sd->entry);
               }
          }
        else
          {
             // if it is called from item_append_xxx, item_del functions,
             // all items are unpacked and packed again
             for (i = count; i < items_count; i++)
               {
                  it = eina_list_nth(sd->items, i);
#ifdef _VI_EFFECT
                  // reset all effects
                  trans = (Elm_Transit *)evas_object_data_get(VIEW(it), "transit");
                  if (trans) elm_transit_del(trans);
#endif
                  elm_box_unpack(sd->box, VIEW(it));
                  evas_object_hide(VIEW(it));
               }

             children = elm_box_children_get(sd->box);
             EINA_LIST_FREE(children, child)
               {
                  if (child != sd->label)
                    {
                       elm_box_unpack(sd->box, child);
                       evas_object_hide(child);
                    }
               }

             for (i = 0; i < count; i++)
               {
                  it = eina_list_nth(sd->items, i);
                  elm_box_pack_end(sd->box, VIEW(it));
                  evas_object_show(VIEW(it));
               }
          }

        elm_box_pack_end(sd->box, sd->end);
        evas_object_show(sd->end);

        sd->expanded_state = EINA_FALSE;
     }
   else
     {
        _adjust_first_item(obj, linew, &adjustw);
        linew = adjustw;
        if (!force)
          {
             if (sd->editable)
               {
                  evas_object_size_hint_min_get(sd->entry, &mnw, NULL);
                  linew += mnw;
                  if (linew > (w * (2 / 3)))
                    {
                       if (elm_object_focus_get(sd->entry))
                         elm_object_focus_set(sd->entry, EINA_FALSE);
                       elm_box_unpack(sd->box, sd->entry);
                       evas_object_hide(sd->entry);
                       sd->expanded_state = EINA_FALSE;
                    }
               }
             if (!_end_packed(obj))
               {
                  evas_object_data_set(sd->end, "num", NULL);
                  elm_object_text_set(sd->end, NULL);
                  elm_box_pack_end(sd->box, sd->end);
                  evas_object_show(sd->end);
               }
          }
        else
          {
             // if it is called from item_append_xxx, item_del functions,
             // all items are unpacked and packed again
             children = elm_box_children_get(sd->box);
             EINA_LIST_FREE(children, child)
               {
                  if (child != sd->label)
                    {
                       elm_box_unpack(sd->box, child);
                       evas_object_hide(child);
                    }
               }

             for (i = 0; i < count; i++)
               {
                  it = eina_list_nth(sd->items, i);
                  elm_box_pack_end(sd->box, VIEW(it));
                  evas_object_show(VIEW(it));
               }

             sd->expanded_state = EINA_TRUE;

             if (!_end_packed(obj))
               {
                  evas_object_data_set(sd->end, "num", NULL);
                  elm_object_text_set(sd->end, NULL);
                  elm_box_pack_end(sd->box, sd->end);
                  evas_object_show(sd->end);
               }

             if (sd->editable)
               {
                  evas_object_size_hint_min_get(sd->entry, &mnw, NULL);
                  linew += mnw;
                  if (linew > (w * (2 / 3)))
                    sd->expanded_state = EINA_FALSE;
                  else
                    {
                       elm_box_pack_end(sd->box, sd->entry);
                       evas_object_show(sd->entry);
                    }
               }
          }
     }

   if (!sd->expanded_state && !force)
     evas_object_smart_callback_call(obj, SIG_EXPAND_STATE_CHANGED, NULL);
}

static Eina_Bool
_box_min_size_calculate(Evas_Object *box,
                        Evas_Object_Box_Data *priv,
                        int *line_height,
                        void *data)
{
   Evas_Coord mnw, mnh, w, minw, minh = 0, linew = 0, lineh = 0, labelw = 0, endw = 0, itemw;
   int line_num, cnt = 0;
   Eina_List *l, *l_next;
   Evas_Object_Box_Option *opt;

   ELM_MULTIBUTTONENTRY_DATA_GET(data, sd);

   evas_object_geometry_get(box, NULL, NULL, &w, NULL);
   evas_object_size_hint_min_get(box, &minw, NULL);
   if (!w) return EINA_FALSE;

   line_num = 1;
   EINA_LIST_FOREACH(priv->children, l, opt)
     {
        evas_object_size_hint_min_get(opt->obj, &mnw, &mnh);

        if (!sd->label) //if there isn't label in mbe
          {
             // all items follow the same rule
             if (sd->end && (opt->obj != sd->end))
               {
                  evas_object_size_hint_min_get(sd->end, &endw, NULL);
                  if (mnw > w - (endw + priv->pad.h))
                    mnw = w - (endw + priv->pad.h);
               }
             else if (!sd->end)
               {
                  if (mnw > w)
                    mnw = w;
               }
          }
        else if (sd->label && _label_packed(data)) //if there is label in mbe
          {
             if (cnt == 1) //in case of the 1st item
               {
                  evas_object_size_hint_min_get(sd->label, &labelw, NULL);
                  itemw = (Evas_Coord)evas_object_data_get(opt->obj, "shrinked_item");
                  if (sd->end && (opt->obj != sd->end))
                    {
                       evas_object_size_hint_min_get(sd->end, &endw, NULL);
                       if (itemw)
                         {
                            if (mnw > itemw - (endw + priv->pad.h))
                              mnw = itemw - (endw + priv->pad.h);
                         }
                       else
                         {
                            if (mnw > w - (labelw + priv->pad.h + endw + priv->pad.h))
                              mnw = w - (labelw + priv->pad.h + endw + priv->pad.h);
                         }
                    }
                  else if (!sd->end)
                    {
                       if (itemw)
                         mnw = itemw;
                       else
                         {
                            if (mnw > w - (labelw + priv->pad.h))
                              mnw = w - (labelw + priv->pad.h);
                         }
                    }
               }
             else if (cnt != 0) //other items without sd->label
               {
                  if (sd->end && (opt->obj != sd->end))
                    {
                       evas_object_size_hint_min_get(sd->end, &endw, NULL);
                       if (mnw > w - (endw + priv->pad.h))
                         mnw = w - (endw + priv->pad.h);
                    }
                  else if (!sd->end)
                    {
                       if (mnw > w)
                         mnw = w;
                    }
               }
          }

        linew += mnw;
        if (lineh < mnh) lineh = mnh;

        if (linew > w * 2 / 3)
          {
             if (linew > w)
               {
                  linew = mnw;
                  line_num++;
                  if (linew > w * 2 / 3)
                    {
                       l_next = eina_list_next(l);
                       opt = eina_list_data_get(l_next);
                       if (l_next && opt && opt->obj &&
                           !strcmp(elm_widget_type_get(opt->obj), "elm_entry"))
                         {
                            linew = 0;
                            line_num++;
                         }
                    }
               }
             else
               {
                  l_next = eina_list_next(l);
                  opt = eina_list_data_get(l_next);
                  if (l_next && opt && opt->obj &&
                      !strcmp(elm_widget_type_get(opt->obj), "elm_entry"))
                    {
                       linew = 0;
                       line_num++;
                    }
               }
          }
        if ((linew != 0) && (l != eina_list_last(priv->children)))
          linew += priv->pad.h;
        cnt++;
     }

   minh = lineh * line_num + (line_num - 1) * priv->pad.v;

   evas_object_size_hint_min_set(box, minw, minh);
   *line_height = lineh;

   return EINA_TRUE;
}

static void
_box_layout(Evas_Object *o,
            Evas_Object_Box_Data *priv,
            void *data)
{
   Evas_Coord x, y, w, h, xx, yy, xxx, yyy;
   Evas_Coord minw, minh, linew = 0, lineh = 0, lineww = 0, itemw;
   double ax, ay;
   Eina_Bool rtl;
   Eina_List *l, *l_next;
   Evas_Object *obj;
   Evas_Object_Box_Option *opt;
   int cnt = 0;

   ELM_MULTIBUTTONENTRY_DATA_GET(data, sd);

   if (!_box_min_size_calculate(o, priv, &lineh, data)) return;

   evas_object_geometry_get(o, &x, &y, &w, &h);
   evas_object_size_hint_min_get(o, &minw, &minh);
   evas_object_size_hint_align_get(o, &ax, &ay);

   rtl = elm_widget_mirrored_get(data);
   if (rtl) ax = 1.0 - ax;

   if (w < minw)
     {
        x = x + ((w - minw) * (1.0 - ax));
        w = minw;
     }
   if (h < minh)
     {
        y = y + ((h - minh) * (1.0 - ay));
        h = minh;
     }

   xx = x;
   yy = y;
   EINA_LIST_FOREACH(priv->children, l, opt)
     {
        Evas_Coord mnw, mnh, ww, hh, ow, oh, labelw, endw;
        double wx, wy;
        Eina_Bool fw, fh, xw, xh;

        obj = opt->obj;
        evas_object_size_hint_align_get(obj, &ax, &ay);
        evas_object_size_hint_weight_get(obj, &wx, &wy);
        evas_object_size_hint_min_get(obj, &mnw, &mnh);

        fw = fh = EINA_FALSE;
        xw = xh = EINA_FALSE;
        if (ax == -1.0) {fw = EINA_TRUE; ax = 0.5;}
        if (ay == -1.0) {fh = EINA_TRUE; ay = 0.5;}
        if (rtl) ax = 1.0 - ax;
        if (wx > 0.0) xw = EINA_TRUE;
        if (wy > 0.0) xh = EINA_TRUE;

        if (!sd->label) //if there isn't label in mbe
          {
             // all items follow the same rule
             if (sd->end && (obj != sd->end))
               {
                  evas_object_data_set(obj, "maximum_width", (void *)mnw);
                  evas_object_size_hint_min_get(sd->end, &endw, NULL);
                  if (mnw > w - (endw + priv->pad.h))
                    {
                       mnw = w - (endw + priv->pad.h);
                       elm_layout_signal_emit(obj, "elm,state,text,ellipsis", "");
                    }
               }
             else if (!sd->end)
               {
                  if (mnw > w)
                    {
                       mnw = w;
                       elm_layout_signal_emit(obj, "elm,state,text,ellipsis", "");
                    }
               }
             edje_object_message_signal_process(elm_layout_edje_get(obj));
          }
        else if (sd->label && _label_packed(data)) //if there is label in mbe
          {
             if (cnt == 1) //in case of the 1st item
               {
                  evas_object_size_hint_min_get(sd->label, &labelw, NULL);
                  evas_object_data_set(obj, "maximum_width", (void *)mnw);

                  itemw = (Evas_Coord)evas_object_data_get(obj, "shrinked_item");
                  if (sd->end && (obj != sd->end))
                    {
                       evas_object_size_hint_min_get(sd->end, &endw, NULL);
                       if (itemw)
                         {
                            if (mnw > itemw - (endw + priv->pad.h))
                              {
                                 mnw = itemw - (endw + priv->pad.h);
                                 elm_layout_signal_emit(obj, "elm,state,text,ellipsis", "");
                              }
                         }
                       else
                         {
                            if (mnw > w - (labelw + priv->pad.h + endw + priv->pad.h))
                              {
                                 mnw = w - (labelw + priv->pad.h + endw + priv->pad.h);
                                 elm_layout_signal_emit(obj, "elm,state,text,ellipsis", "");
                              }
                         }
                    }
                  else if (!sd->end)
                    {
                       if (itemw)
                         {
                            mnw = itemw;
                            elm_layout_signal_emit(obj, "elm,state,text,ellipsis", "");
                         }
                       else
                         {
                            if (mnw > w - (labelw + priv->pad.h))
                              {
                                 mnw = w - (labelw + priv->pad.h);
                                 elm_layout_signal_emit(obj, "elm,state,text,ellipsis", "");
                              }
                         }
                    }
               }
             else if (cnt != 0) //other items without sd->label
               {
                  if (sd->end && (obj != sd->end))
                    {
                       evas_object_data_set(obj, "maximum_width", (void *)mnw);
                       evas_object_size_hint_min_get(sd->end, &endw, NULL);
                       if (mnw > w - (endw + priv->pad.h))
                         {
                            mnw = w - (endw + priv->pad.h);
                            elm_layout_signal_emit(obj, "elm,state,text,ellipsis", "");
                         }
                    }
                  else if (!sd->end)
                    {
                       if (mnw > w)
                         {
                            mnw = w;
                            elm_layout_signal_emit(obj, "elm,state,text,ellipsis", "");
                         }
                    }
               }
             edje_object_message_signal_process(elm_layout_edje_get(obj));
          }
        evas_object_size_hint_min_set(obj, mnw, mnh);

        ww = mnw;
        if (xw)
          {
             if (ww <= w - linew) ww = w - linew;
             else ww = w;
          }
        hh = lineh;

        ow = mnw;
        if (fw) ow = ww;
        oh = mnh;
        if (fh) oh = hh;

        linew += ww;
        if (linew > w && l != priv->children)
          {
             if (obj == sd->end)
               {
                  elm_layout_signal_emit(obj, "elm,state,number,ellipsis", "");
                  edje_object_message_signal_process(elm_layout_edje_get(obj));
                  linew -= ww;
                  ww = w - (linew + priv->pad.h);
                  linew += ww;
                  evas_object_size_hint_min_set(obj, ww, mnh);
               }
             else
               {
                  xx = x;
                  yy += hh;
                  yy += priv->pad.v;
                  linew = ww;
               }
          }

        evas_object_move(obj,
                         ((!rtl) ? (xx) : (x + (w - (xx - x) - ww)))
                         + (Evas_Coord)(((double)(ww - ow)) * ax),
                         yy + (Evas_Coord)(((double)(hh - oh)) * ay));
        evas_object_resize(obj, ow, oh);

        xx += ww;
        xx += priv->pad.h;
        xxx = xx;
        yyy = yy;
        lineww = linew;

        // if linew is bigger than 2/3 of boxw, entry goes to next line.
        if (linew > w * 2 / 3)
          {
             l_next = eina_list_next(l);
             opt = eina_list_data_get(l_next);
             if (l_next && opt && opt->obj && !strcmp(elm_widget_type_get(opt->obj), "elm_entry"))
               {
                  xx = x;
                  yy += hh;
                  yy += priv->pad.v;
                  linew = 0;
               }
          }

        // whether there is "+%d" at the end of the box or entry is invisible,
        // box doesn't have to be larger.
        if (!sd->expanded_state)
          {
             xx = xxx;
             yy = yyy;
             linew = lineww;
          }
        if ((linew != 0) && (l != eina_list_last(priv->children)))
          linew += priv->pad.h;
        cnt++;
     }
}

static void
_on_box_resize(void *data,
               Evas *e __UNUSED__,
               Evas_Object *obj,
               void *event_info __UNUSED__)
{
   Evas_Coord w, h, mnw, mnh, labelw, hpad;
   int items_count = 0, i;
   Elm_Multibuttonentry_Item *it;

   ELM_MULTIBUTTONENTRY_DATA_GET(data, sd);

   evas_object_geometry_get(obj, NULL, NULL, &w, &h);
   elm_box_padding_get(obj, &hpad, NULL);

   if (sd->boxh < h)
     evas_object_smart_callback_call(data, SIG_EXPANDED, NULL);
   else if (sd->boxh > h)
     evas_object_smart_callback_call(data, SIG_CONTRACTED, NULL);

   // on rotation, items should be packed again in the shrinked layout
   if (sd->boxw && sd->boxw != w)
     {
        // recalculate width of all items
        if (sd->end)
          {
             elm_layout_signal_emit(sd->end, "elm,state,number,default", "");
             edje_object_message_signal_process(elm_layout_edje_get(sd->end));
             elm_layout_sizing_eval(sd->end);
             evas_object_smart_calculate(sd->end);
          }
        if (sd->items)
          {
             items_count = eina_list_count(sd->items);
             for(i = 0; i < items_count; i++)
               {
                  it = eina_list_nth(sd->items, i);

                  if (evas_object_data_get(VIEW(it), "shrinked_item"))
                    evas_object_data_set(VIEW(it), "shrinked_item", NULL);

                  elm_layout_signal_emit(VIEW(it), "elm,state,text,default", "");
                  edje_object_message_signal_process(elm_layout_edje_get(VIEW(it)));
                  elm_layout_sizing_eval(VIEW(it));
                  evas_object_smart_calculate(VIEW(it));

                  evas_object_size_hint_min_get(VIEW(it), &mnw, &mnh);

                  if (i == 0)
                    {
                       if (sd->label && _label_packed(data))
                         {
                            evas_object_size_hint_min_get(sd->label, &labelw, NULL);
                            if (mnw > w - (labelw + hpad))
                              {
                                 mnw = w - (labelw + hpad);
                                 elm_layout_signal_emit(VIEW(it), "elm,state,text,ellipsis", "");
                                 edje_object_message_signal_process(elm_layout_edje_get(VIEW(it)));
                                 evas_object_size_hint_min_set(VIEW(it), mnw, mnh);
                                 evas_object_resize(VIEW(it), mnw, mnh);
                              }
                         }
                       evas_object_data_set(VIEW(it), "maximum_width", (void *)mnw);
                    }
                  if (mnw > w)
                    {
                       mnw = w;
                       elm_layout_signal_emit(VIEW(it), "elm,state,text,ellipsis", "");
                       edje_object_message_signal_process(elm_layout_edje_get(VIEW(it)));
                       evas_object_size_hint_min_set(VIEW(it), mnw, mnh);
                       evas_object_resize(VIEW(it), mnw, mnh);
                    }
               }
          }
     }
   if (!elm_object_focus_get(data) && !sd->expanded && !evas_object_visible_get(sd->entry))
     _layout_shrink(data, EINA_TRUE);

   sd->boxh = h;
   sd->boxw = w;
}

static Elm_Multibuttonentry_Item_Filter *
_filter_new(Elm_Multibuttonentry_Item_Filter_Cb func,
            const void *data)
{
   Elm_Multibuttonentry_Item_Filter *ft;

   ft = ELM_NEW(Elm_Multibuttonentry_Item_Filter);
   if (!ft) return NULL;

   ft->func = func;
   ft->data = data;

   return ft;
}

static void
_on_item_clicked(void *data,
                 Evas_Object *obj __UNUSED__,
                 const char *emission __UNUSED__,
                 const char *source __UNUSED__)
{
   Elm_Multibuttonentry_Item *it = data;

   ELM_MULTIBUTTONENTRY_DATA_GET(WIDGET(it), sd);

   if (it->func) it->func((void *)it->base.data, WIDGET(it), it);

   elm_object_focus_set(VIEW(it), EINA_TRUE);

   // handles input panel because it can be hidden by user
   if (sd->editable)
     elm_entry_input_panel_show(sd->entry);

   evas_object_smart_callback_call(WIDGET(it), SIG_ITEM_CLICKED, it);
}

static void
_on_item_selected(void *data,
                  Evas_Object *obj __UNUSED__,
                  const char *emission __UNUSED__,
                  const char *source __UNUSED__)
{
   Elm_Multibuttonentry_Item *it = data;

   ELM_MULTIBUTTONENTRY_DATA_GET(WIDGET(it), sd);

   sd->selected_item = eina_list_append(sd->selected_item, it);
   it->selected = EINA_TRUE;

   evas_object_smart_callback_call(WIDGET(it), SIG_ITEM_SELECTED, it);
}

static void
_on_item_unselected(void *data,
                  Evas_Object *obj __UNUSED__,
                  const char *emission __UNUSED__,
                  const char *source __UNUSED__)
{
   Elm_Multibuttonentry_Item *it = data;

   ELM_MULTIBUTTONENTRY_DATA_GET(WIDGET(it), sd);

   sd->selected_item = eina_list_remove(sd->selected_item, it);
   it->selected = EINA_FALSE;
}

static Eina_Bool
_long_press_cb(void *data)
{
   Elm_Multibuttonentry_Item *it = data;

   ELM_MULTIBUTTONENTRY_DATA_GET(WIDGET(it), sd);

   sd->longpress_timer = NULL;

   evas_object_smart_callback_call(WIDGET(it), SIG_LONGPRESSED, it);

   return ECORE_CALLBACK_CANCEL;
}

static void
_mouse_down_cb(Elm_Multibuttonentry_Item *it,
               Evas *evas __UNUSED__,
               Evas_Object *obj __UNUSED__,
               Evas_Event_Mouse_Down *ev)
{
   ELM_MULTIBUTTONENTRY_DATA_GET(WIDGET(it), sd);

   if (ev->button != 1) return;

   if (sd->longpress_timer) ecore_timer_del(sd->longpress_timer);
   sd->longpress_timer = ecore_timer_add
       (_elm_config->longpress_timeout, _long_press_cb, it);
}

static void
_mouse_up_cb(Elm_Multibuttonentry_Item *it,
             Evas *evas __UNUSED__,
             Evas_Object *obj __UNUSED__,
             Evas_Event_Mouse_Down *ev __UNUSED__)
{
   ELM_MULTIBUTTONENTRY_DATA_GET(WIDGET(it), sd);

   if (sd->longpress_timer)
     {
        ecore_timer_del(sd->longpress_timer);
        sd->longpress_timer = NULL;
     }
}

static void
_on_item_focus(void *data, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   Elm_Multibuttonentry_Item *it = (Elm_Multibuttonentry_Item *)data;
   if (!it) return;

   ELM_MULTIBUTTONENTRY_DATA_GET(WIDGET(it), sd);

   if (elm_object_focus_get(sd->entry))
     elm_object_focus_set(sd->entry, EINA_FALSE);

   sd->focused = (Elm_Object_Item *)it;

   if (sd->editable)
     elm_entry_input_panel_show(sd->entry);
}

static void
_on_item_unfocus(void *data, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   Elm_Multibuttonentry_Item *it = (Elm_Multibuttonentry_Item *)data;
   if (!it) return;

   ELM_MULTIBUTTONENTRY_DATA_GET(WIDGET(it), sd);

   sd->focused = NULL;

   elm_layout_signal_emit(VIEW(it), "elm,action,btn,unselected", "");
}

static void
_item_text_set_hook(Elm_Object_Item *it,
                    const char *part,
                    const char *text)
{
   Evas_Coord minw, minh, boxw;
   ELM_MULTIBUTTONENTRY_DATA_GET(WIDGET(it), sd);

   if (part && strcmp(part, "elm.btn.text")) return;
   if (!text) return;

   elm_layout_signal_emit(VIEW(it), "elm,state,text,default", "");
   edje_object_message_signal_process(elm_layout_edje_get(VIEW(it)));
   elm_object_part_text_set(VIEW(it), "elm.btn.text", text);

   evas_object_smart_calculate(VIEW(it));
   evas_object_size_hint_min_get(VIEW(it), &minw, &minh);
   evas_object_geometry_get(sd->box, NULL, NULL, &boxw, NULL);

   if (sd->boxw && minw > boxw)
     {
         elm_layout_signal_emit(VIEW(it), "elm,state,text,ellipsis", "");
         edje_object_message_signal_process(elm_layout_edje_get(VIEW(it)));
         evas_object_size_hint_min_set(VIEW(it), boxw, minh);
         evas_object_resize(VIEW(it), boxw, minh);
     }
}

static const char *
_item_text_get_hook(const Elm_Object_Item *it,
                    const char *part)
{
   const char *src_part = NULL;

   if (!part || !strcmp(part, "elm.text"))
     src_part = "elm.btn.text";
   else
     src_part = part;

   return elm_layout_text_get(VIEW(it), src_part);
}

static Eina_Bool
_item_del_pre_hook(Elm_Object_Item *it)
{
   ELM_MULTIBUTTONENTRY_DATA_GET(WIDGET(it), sd);

   sd->items = eina_list_remove(sd->items, it);

   if (((Elm_Multibuttonentry_Item *)it)->selected)
     sd->selected_item = eina_list_remove(sd->selected_item, it);

   if (!elm_object_focus_get(WIDGET(it)) && !sd->expanded)
     _layout_shrink(WIDGET(it), EINA_TRUE);

#ifdef _VI_EFFECT
   Elm_Transit *trans;

   trans = (Elm_Transit *)evas_object_data_get(VIEW(it), "transit");
   if (trans)
     {
        elm_transit_del_cb_set(trans, NULL, NULL);
        elm_transit_del(trans);
     }
#endif

   if (!sd->items && !elm_object_focus_get(WIDGET(it)) &&
       sd->guide && !_guide_packed(WIDGET(it)))
     {
        if (sd->editable)
          {
             elm_box_unpack(sd->box, sd->entry);
             evas_object_hide(sd->entry);
          }

        elm_box_pack_end(sd->box, sd->guide);
        // TIZEN ONLY(20140626): Until fixing bounding box issue, we send signal instead of "evas_object_show"
        elm_layout_signal_emit(sd->guide, "elm,state,guidetext,show", "elm");
     }

   evas_object_smart_callback_call(WIDGET(it), SIG_ITEM_DELETED, it);

   return EINA_TRUE;
}

static void
_item_disable_hook(Elm_Object_Item *it)
{
   Elm_Multibuttonentry_Item *mbe_it = (Elm_Multibuttonentry_Item *)it;

   if (elm_widget_item_disabled_get(it))
     elm_layout_signal_emit(VIEW(mbe_it), "elm,state,disabled", "elm");
   else
     elm_layout_signal_emit(VIEW(mbe_it), "elm,state,enabled", "elm");
   edje_object_message_signal_process(VIEW(mbe_it));
}

static char *
_access_info_cb(void *data, Evas_Object *obj __UNUSED__)
{
   Elm_Multibuttonentry_Item *it = (Elm_Multibuttonentry_Item *)data;
   const char *txt = NULL;

   if (!it) return NULL;

   txt = elm_object_item_text_get((Elm_Object_Item *)it);
   if (txt) return strdup(txt);
   return NULL;
}

static char *
_access_state_cb(void *data, Evas_Object *obj __UNUSED__)
{
   Elm_Multibuttonentry_Item *it = (Elm_Multibuttonentry_Item *)data;
   if (!it) return NULL;

   if (it->base.disabled)
     return strdup(E_("IDS_ACCS_BODY_DISABLED_TTS"));

   return NULL;
}

static char *
_access_context_info_cb(void *data, Evas_Object *obj __UNUSED__)
{
   Elm_Multibuttonentry_Item *it = (Elm_Multibuttonentry_Item *)data;
   if (!it) return NULL;

   return strdup(E_("WDS_TTS_TBBODY_DOUBLE_TAP_TO_EDIT"));
}

static void
_access_item_activate_cb(void *data __UNUSED__,
                        Evas_Object *part_obj __UNUSED__,
                        Elm_Object_Item *item)
{
   _on_item_clicked(item, NULL, NULL, NULL);
}

static void
_access_widget_item_register(Elm_Multibuttonentry_Item *it, Eina_Bool is_access)
{
   Elm_Access_Info *ai;

   if (!is_access) _elm_access_widget_item_unregister((Elm_Widget_Item *)it);
   else
     {
        _elm_access_widget_item_register((Elm_Widget_Item *)it);

        ai = _elm_access_object_get(it->base.access_obj);

        _elm_access_callback_set(ai, ELM_ACCESS_INFO, _access_info_cb, it);
        _elm_access_callback_set(ai, ELM_ACCESS_STATE, _access_state_cb, it);
        _elm_access_callback_set(ai, ELM_ACCESS_CONTEXT_INFO, _access_context_info_cb, it);
        _elm_access_activate_callback_set(ai, _access_item_activate_cb, it);
     }
}

static Elm_Multibuttonentry_Item *
_item_new(Evas_Object *obj,
          const char *text,
          Evas_Smart_Cb func,
          const void *data)
{
   Elm_Multibuttonentry_Item *it;
   Elm_Multibuttonentry_Item_Filter *ft;
   Eina_List *l;
   Evas_Coord minw, minh, boxw = 0;

   ELM_MULTIBUTTONENTRY_DATA_GET(obj, sd);

   EINA_LIST_FOREACH(sd->filters, l, ft)
     {
        if (!ft->func(obj, text, data, ft->data))
          return NULL;
     }

   it = elm_widget_item_new(obj, Elm_Multibuttonentry_Item);
   if (!it) return NULL;

   VIEW(it) = elm_layout_add(obj);
   elm_layout_theme_set
      (VIEW(it), "multibuttonentry", "btn", elm_widget_style_get(obj));
   elm_layout_signal_callback_add
      (VIEW(it), "elm,action,clicked", "", _on_item_clicked, it);
   elm_layout_signal_callback_add
      (VIEW(it), "elm,action,selected", "", _on_item_selected, it);
   elm_layout_signal_callback_add
      (VIEW(it), "elm,action,unselected", "", _on_item_unselected, it);
   evas_object_event_callback_add
      (elm_layout_edje_get(VIEW(it)),
       EVAS_CALLBACK_MOUSE_DOWN, (Evas_Object_Event_Cb)_mouse_down_cb, it);
   evas_object_event_callback_add
      (elm_layout_edje_get(VIEW(it)),
       EVAS_CALLBACK_MOUSE_UP, (Evas_Object_Event_Cb)_mouse_up_cb, it);
   elm_object_part_text_set(VIEW(it), "elm.btn.text", text);
   evas_object_smart_callback_add
      (VIEW(it), "focused", _on_item_focus, it);
   evas_object_smart_callback_add
      (VIEW(it), "unfocused", _on_item_unfocus, it);
   if (_elm_config->access_mode)
     {
        evas_object_event_callback_add(elm_layout_edje_get(VIEW(it)),
                                       EVAS_CALLBACK_FOCUS_IN, _on_item_focus2, it);
        evas_object_event_callback_add(elm_layout_edje_get(VIEW(it)),
                                       EVAS_CALLBACK_FOCUS_OUT, _on_item_focus2, it);
     }

   it->func = func;
   it->base.data = data;

   // adjust item size if item is longer than maximum size
   evas_object_smart_calculate(VIEW(it));
   evas_object_size_hint_min_get(VIEW(it), &minw, &minh);
   evas_object_geometry_get(sd->box, NULL, NULL, &boxw, NULL);

   if (sd->boxw && minw > boxw)
     {
        elm_layout_signal_emit(VIEW(it), "elm,state,text,ellipsis", "");
        edje_object_message_signal_process(elm_layout_edje_get(VIEW(it)));
        evas_object_size_hint_min_set(VIEW(it), boxw, minh);
        evas_object_resize(VIEW(it), boxw, minh);
     }

   elm_widget_item_text_set_hook_set(it, _item_text_set_hook);
   elm_widget_item_text_get_hook_set(it, _item_text_get_hook);
   elm_widget_item_del_pre_hook_set(it, _item_del_pre_hook);
   elm_widget_item_disable_hook_set(it, _item_disable_hook);

   elm_object_focus_allow_set(VIEW(it), EINA_TRUE);

   sd->item_be_selected = EINA_TRUE;

   // ACCESS
   if (_elm_config->access_mode) _access_widget_item_register(it, EINA_TRUE);

   return it;
}

static void
_on_entry_unfocused(void *data,
                    Evas_Object *obj,
                    void *event_info __UNUSED__)
{
   const char *str;

   ELM_MULTIBUTTONENTRY_DATA_GET(data, sd);

   str = elm_object_text_get(obj);
   if (strlen(str))
     {
        Elm_Multibuttonentry_Item *it;

        it = _item_new(data, str, NULL, NULL);
        if (!it)
          {
             return;
          }

        sd->items = eina_list_append(sd->items, it);
#ifdef _VI_EFFECT
        _item_adding_effect_add(data, it);
#else
        elm_box_pack_before(sd->box, VIEW(it), obj);
        evas_object_show(VIEW(it));
        evas_object_smart_callback_call(data, SIG_ITEM_ADDED, it);
#endif

        elm_object_text_set(obj, "");
     }
}

static void
_on_entry_changed(void *data,
                 Evas_Object *obj,
                 void *event_info __UNUSED__)
{
   const char *str;

   ELM_MULTIBUTTONENTRY_DATA_GET(data, sd);

   str = elm_object_text_get(obj);
   if (strlen(str))
     sd->item_be_selected = EINA_FALSE;
   else
     sd->item_be_selected = EINA_TRUE;
}

// handles enter key
static void
_on_entry_key_down(void *data,
                 Evas *e __UNUSED__,
                 Evas_Object *obj,
                 void *event_info)
{
   const char *str;
   Evas_Event_Key_Up *ev = (Evas_Event_Key_Up *)event_info;

   ELM_MULTIBUTTONENTRY_DATA_GET(data, sd);

   // cancels item_be_selected when text inserting is started
   if (strcmp(ev->keyname, "KP_Enter") && strcmp(ev->keyname, "Return") &&
       strcmp(ev->keyname, "BackSpace") && strcmp(ev->keyname, "Delete") &&
       strcmp(ev->keyname, "semicolon") && strcmp(ev->keyname, "comma"))
     {
        sd->item_be_selected = EINA_FALSE;
        return;
     }

   if (!strcmp(ev->keyname, "KP_Enter") || !strcmp(ev->keyname, "Return"))
     {
        str = elm_object_text_get(obj);
        if (strlen(str))
          {
             Elm_Multibuttonentry_Item *it;

             it = _item_new(data, str, NULL, NULL);
             if (!it)
               {
                  return;
               }

             sd->items = eina_list_append(sd->items, it);
#ifdef _VI_EFFECT
             _item_adding_effect_add(data, it);
#else
             elm_box_pack_before(sd->box, VIEW(it), obj);
             evas_object_show(VIEW(it));
             evas_object_smart_callback_call(data, SIG_ITEM_ADDED, it);
#endif

             elm_object_text_set(obj, "");
          }
     }
}

static void
_on_item_focus2(void *data,
               Evas *e __UNUSED__,
               Evas_Object *obj,
               void *event_info __UNUSED__)
{
   Elm_Multibuttonentry_Item *it = data;

   ELM_MULTIBUTTONENTRY_DATA_GET(WIDGET(it), sd);

   if (evas_object_focus_get(obj))
     {
        if (elm_object_focus_get(sd->entry))
          elm_object_focus_set(sd->entry, EINA_FALSE);

        sd->focused = (Elm_Object_Item *)it;

        if (sd->editable)
          elm_entry_input_panel_show(sd->entry);
     }
   else
     {
        sd->focused = NULL;
        elm_layout_signal_emit(VIEW(it), "elm,action,btn,unselected", "");

        if (sd->editable)
          elm_entry_input_panel_hide(sd->entry);
     }
}
// handles delete key
// it can be pressed when button is selected, so it is handled on layout_key_down
static void
_on_layout_key_down(void *data __UNUSED__,
                  Evas *e __UNUSED__,
                  Evas_Object *obj,
                  void *event_info)
{
   const char *str;
   Evas_Event_Key_Up *ev = (Evas_Event_Key_Up *)event_info;

   ELM_MULTIBUTTONENTRY_DATA_GET(obj, sd);

   if (strcmp(ev->keyname, "BackSpace") && strcmp(ev->keyname, "Delete"))
     return;

   str = elm_object_text_get(sd->entry);
   if (strlen(str))
     {
        return;
     }

   if (!sd->items) return;

   if (!sd->focused)
     {
        if (sd->item_be_selected) // 2nd delete
          {
             Elm_Multibuttonentry_Item *it;

             it = eina_list_data_get(eina_list_last(sd->items));

             if (_elm_config->access_mode)
               evas_object_focus_set(elm_layout_edje_get(VIEW(it)), EINA_TRUE);
             // when backspace is pressed, the item has focus and called selected signal
             else
               elm_object_focus_set(VIEW(it), EINA_TRUE);
          }
        else // 1st delete
          sd->item_be_selected = EINA_TRUE;
     }
   else // 3rd delete
     {
#ifdef _VI_EFFECT
        Elm_Transit *trans;

        trans = (Elm_Transit *)evas_object_data_get(VIEW(sd->focused), "transit");

        if (!trans)
          _item_deleting_effect_add
             ((Elm_Multibuttonentry_Item *)sd->focused);
#else
        elm_object_item_del(sd->focused);

        if (sd->editable)
          elm_object_focus_set(sd->entry, EINA_TRUE);
#endif
     }
}

static void
_on_layout_clicked(void *data __UNUSED__,
                   Evas_Object *obj,
                   const char *emission __UNUSED__,
                   const char *source __UNUSED__)
{
   evas_object_smart_callback_call(obj, SIG_CLICKED, NULL);
}

static Eina_Bool
_elm_multibuttonentry_smart_on_focus(Evas_Object *obj, Elm_Focus_Info *info)
{
   ELM_MULTIBUTTONENTRY_DATA_GET(obj, sd);

   if (elm_object_focus_get(obj))
     {
        if (sd->guide && _guide_packed(obj))
          {
             if (sd->editable)
               {
                  elm_box_unpack(sd->box, sd->guide);
                  // TIZEN ONLY(20140624): below line should be checked with bounding box
                  evas_object_smart_member_add(sd->guide, obj);
                  // TIZEN ONLY(20140626): Until fixing bounding box issue, we send signal instead of "evas_object_hide"
                  elm_layout_signal_emit(sd->guide, "elm,state,guidetext,hide", "elm");

                  elm_box_pack_end(sd->box, sd->entry);
                  evas_object_show(sd->entry);
               }
          }
        // when object gets focused, it should be expanded layout
        else if (!sd->expanded)
          _layout_expand(obj);

        if (sd->editable)
          {
             elm_layout_signal_emit(obj, "elm,state,event,allow", "");
             if (!sd->focused)
               elm_object_focus_set(sd->entry, EINA_TRUE);
          }

        evas_object_smart_callback_call(obj, SIG_FOCUSED, info);
     }
   else
     {
        if (!sd->items && sd->guide)
          {
             if (sd->editable)
               {
                  elm_box_unpack(sd->box, sd->entry);
                  evas_object_hide(sd->entry);
               }
             const char *text;
             text = elm_object_text_get(sd->entry);
             if (text && strlen(text))
               {
                  _end_set(obj);
                  if (!_end_packed(obj))
                    elm_box_pack_end(sd->box, sd->end);
                  evas_object_show(sd->end);
               }
             else
               {
                  elm_box_pack_end(sd->box, sd->guide);
                  // TIZEN ONLY(20140626): Until fixing bounding box issue, we send signal instead of "evas_object_show"
                  elm_layout_signal_emit(sd->guide, "elm,state,guidetext,show", "elm");
               }
             if (!sd->expanded)
               sd->expanded_state = EINA_FALSE;
          }
        // if shrinked mode was set, it goes back to shrinked layout
        else if (!sd->expanded)
          _layout_shrink(obj, EINA_FALSE);

        if (sd->editable)
          {
             elm_entry_input_panel_hide(sd->entry);
             elm_layout_signal_emit(obj, "elm,state,event,block", "");
          }

        evas_object_smart_callback_call(obj, SIG_UNFOCUSED, NULL);
     }

   return EINA_TRUE;
}

static Eina_Bool
_elm_multibuttonentry_smart_text_set(Evas_Object *obj,
                                     const char *part,
                                     const char *text)
{
   if (!part || (part && !strcmp(part, "label")))
     {
        if (text) _label_set(obj, text);
        return EINA_TRUE;
     }
   else if (part && !strcmp(part, "guide"))
     {
        if (text) _guide_set(obj, text);
        return EINA_TRUE;
     }
   else return _elm_multibuttonentry_parent_sc->text_set(obj, part, text);
}

static const char *
_elm_multibuttonentry_smart_text_get(const Evas_Object *obj,
                                     const char *part)
{
   ELM_MULTIBUTTONENTRY_DATA_GET(obj, sd);

   if (!part || !strcmp(part, "label"))
     return elm_object_part_text_get(sd->label, "mbe.label");
   else if (!strcmp(part, "guide"))
     return elm_object_text_get(sd->guide);
   else return _elm_multibuttonentry_parent_sc->text_get(obj, part);
}

static void
_elm_multibuttonentry_smart_sizing_eval(Evas_Object *obj)
{
   Evas_Coord minw = -1, minh = -1, maxw = -1, maxh = -1;

   ELM_MULTIBUTTONENTRY_DATA_GET(obj, sd);

   elm_coords_finger_size_adjust(1, &minw, 1, &minh);
   edje_object_size_min_restricted_calc
      (ELM_WIDGET_DATA(sd)->resize_obj, &minw, &minh, minw, minh);
   elm_coords_finger_size_adjust(1, &minw, 1, &minh);
   evas_object_size_hint_min_set(obj, minw, minh);
   evas_object_size_hint_max_set(obj, maxw, maxh);
}

static Eina_Bool
_elm_multibuttonentry_smart_theme(Evas_Object *obj)
{
   const char *str;
   int hpad = 0, vpad = 0;
   Eina_List *l;
   Elm_Multibuttonentry_Item *it;

   ELM_MULTIBUTTONENTRY_DATA_GET(obj, sd);

   if (!ELM_WIDGET_CLASS(_elm_multibuttonentry_parent_sc)->theme(obj))
     return EINA_FALSE;

   str = elm_layout_data_get(obj, "horizontal_pad");
   if (str) hpad = atoi(str) / edje_object_base_scale_get(elm_layout_edje_get(obj));
   str = elm_layout_data_get(obj, "vertical_pad");
   if (str) vpad = atoi(str) / edje_object_base_scale_get(elm_layout_edje_get(obj));
   elm_box_padding_set
      (sd->box,
       hpad * elm_widget_scale_get(obj) * elm_config_scale_get(),
       vpad * elm_widget_scale_get(obj) * elm_config_scale_get());

   if (sd->label)
     elm_layout_theme_set
        (sd->label, "multibuttonentry", "label", elm_widget_style_get(obj));

   if (sd->guide)
     elm_layout_theme_set
        (sd->guide, "multibuttonentry", "guidetext", elm_widget_style_get(obj));

   if (sd->end)
     elm_layout_theme_set
        (sd->end, "multibuttonentry", "number", elm_widget_style_get(obj));

   if (sd->items)
     EINA_LIST_FOREACH(sd->items, l, it)
        elm_layout_theme_set
        (VIEW(it), "multibuttonentry", "btn", elm_widget_style_get(obj));

   elm_layout_sizing_eval(obj);

   return EINA_TRUE;
}

static void
_elm_multibuttonentry_smart_add(Evas_Object *obj)
{
   EVAS_SMART_DATA_ALLOC(obj, Elm_Multibuttonentry_Smart_Data);

   ELM_WIDGET_CLASS(_elm_multibuttonentry_parent_sc)->base.add(obj);
}

static void
_elm_multibuttonentry_smart_del(Evas_Object *obj)
{
   Elm_Multibuttonentry_Item *it;
   Elm_Multibuttonentry_Item_Filter *ft;

   ELM_MULTIBUTTONENTRY_DATA_GET(obj, sd);

   EINA_LIST_FREE(sd->items, it)
     {
        Evas_Coord maxw, shrinkw;
#ifdef _VI_EFFECT
        Elm_Transit *trans;

        trans = (Elm_Transit *)evas_object_data_get(VIEW(it), "transit");
        if (trans)
          {
             elm_transit_del_cb_set(trans, NULL, NULL);
             elm_transit_del(trans);
          }
#endif
        maxw = (Evas_Coord)evas_object_data_get(VIEW(it), "maximum_width");
        if (maxw)
          evas_object_data_set(VIEW(it), "maximum_width", NULL);

        shrinkw = (Evas_Coord)evas_object_data_get(VIEW(it), "shrinked_item");
        if (shrinkw)
          evas_object_data_set(VIEW(it), "shrinked_item", NULL);
        elm_widget_item_free(it);
     }

   EINA_LIST_FREE(sd->filters, ft)
      free(ft);

   EINA_LIST_FREE(sd->selected_item, it)
      elm_widget_item_free(it);

   if (sd->end)
     {
        evas_object_data_set(sd->end, "num", NULL);
        evas_object_del(sd->end);
     }
   if (sd->guide) evas_object_del(sd->guide);
   if (sd->label) evas_object_del(sd->label);
   if (sd->entry) evas_object_del(sd->entry);
   if (sd->box) evas_object_del(sd->box);
   if (sd->longpress_timer) ecore_timer_del(sd->longpress_timer);

   evas_object_data_set(obj, "end", NULL);

   ELM_WIDGET_CLASS(_elm_multibuttonentry_parent_sc)->base.del(obj);
}

static void
_elm_multibuttonentry_smart_access(Evas_Object *obj, Eina_Bool is_access)
{
   Eina_List *elist = NULL;
   Elm_Multibuttonentry_Item *it;

   ELM_MULTIBUTTONENTRY_CHECK(obj);
   ELM_MULTIBUTTONENTRY_DATA_GET(obj, sd);

   EINA_LIST_FOREACH(sd->items, elist, it)
     _access_widget_item_register(it, is_access);

   evas_object_smart_callback_call(obj, SIG_ACCESS_CHANGED, NULL);
}

static Eina_Bool
_elm_multibuttonentry_smart_activate(Evas_Object *obj, Elm_Activate act)
{
   ELM_MULTIBUTTONENTRY_DATA_GET(obj, sd);

   if (act != ELM_ACTIVATE_DEFAULT) return EINA_FALSE;

   if (!elm_widget_disabled_get(obj))
     {
        if (sd->guide && _guide_packed(obj))
          {
             elm_box_unpack(sd->box, sd->guide);
             // TIZEN ONLY(20140624): below line should be checked with bounding box
             evas_object_smart_member_add(sd->guide, obj);
             // TIZEN ONLY(20140626): Until fixing bounding box issue, we send signal instead of "evas_object_hide"
             elm_layout_signal_emit(sd->guide, "elm,state,guidetext,hide", "elm");

             if (sd->editable)
               {
                  elm_box_pack_end(sd->box, sd->entry);
                  evas_object_show(sd->entry);
               }
          }
        // when object gets focused, it should be expanded layout
        else if (!sd->expanded)
          _layout_expand(obj);

        if (sd->editable)
          {
             if (!elm_object_focus_get(obj) && !sd->guide && sd->items)
               {
                  elm_box_pack_end(sd->box, sd->entry);
                  evas_object_show(sd->entry);
               }
             elm_layout_signal_emit(obj, "elm,state,event,allow", "");
             elm_object_focus_set(sd->entry, EINA_TRUE);
          }

        evas_object_smart_callback_call(obj, SIG_FOCUSED, NULL);
    }

   return EINA_TRUE;
}

static Eina_Bool
_elm_multibuttonentry_smart_disable(Evas_Object *obj)
{
   Eina_List *l;
   Elm_Object_Item *it;

   ELM_MULTIBUTTONENTRY_DATA_GET(obj, sd);

   if (!ELM_WIDGET_CLASS(_elm_multibuttonentry_parent_sc)->disable(obj))
     return EINA_FALSE;

   EINA_LIST_FOREACH(sd->items, l, it)
     elm_object_item_disabled_set(it, elm_widget_disabled_get(obj));

   return EINA_TRUE;
}

static void
_elm_multibuttonentry_smart_set_user(Elm_Multibuttonentry_Smart_Class *sc)
{
   ELM_WIDGET_CLASS(sc)->base.add = _elm_multibuttonentry_smart_add;
   ELM_WIDGET_CLASS(sc)->base.del = _elm_multibuttonentry_smart_del;

   ELM_WIDGET_CLASS(sc)->theme = _elm_multibuttonentry_smart_theme;
   ELM_WIDGET_CLASS(sc)->translate = _elm_multibuttonentry_smart_translate;
   ELM_WIDGET_CLASS(sc)->on_focus = _elm_multibuttonentry_smart_on_focus;

   ELM_WIDGET_CLASS(sc)->event = _elm_multibuttonentry_smart_event;

   ELM_WIDGET_CLASS(sc)->focus_direction_manager_is = _elm_multibuttonentry_smart_focus_direction_manager_is;
   ELM_WIDGET_CLASS(sc)->focus_direction = _elm_multibuttonentry_smart_focus_direction;
   ELM_WIDGET_CLASS(sc)->focus_next = _elm_multibuttonentry_smart_focus_next;

   ELM_LAYOUT_CLASS(sc)->text_set = _elm_multibuttonentry_smart_text_set;
   ELM_LAYOUT_CLASS(sc)->text_get = _elm_multibuttonentry_smart_text_get;

   ELM_LAYOUT_CLASS(sc)->sizing_eval = _elm_multibuttonentry_smart_sizing_eval;

   ELM_WIDGET_CLASS(sc)->access = _elm_multibuttonentry_smart_access;
   ELM_WIDGET_CLASS(sc)->activate = _elm_multibuttonentry_smart_activate;
   ELM_WIDGET_CLASS(sc)->disable = _elm_multibuttonentry_smart_disable;
}

EAPI const Elm_Multibuttonentry_Smart_Class *
elm_multibuttonentry_smart_class_get(void)
{
   static Elm_Multibuttonentry_Smart_Class _sc =
      ELM_MULTIBUTTONENTRY_SMART_CLASS_INIT_NAME_VERSION
      (ELM_MULTIBUTTONENTRY_SMART_NAME);
   static const Elm_Multibuttonentry_Smart_Class *class = NULL;
   Evas_Smart_Class *esc = (Evas_Smart_Class *)&_sc;

   if (class) return class;

   _elm_multibuttonentry_smart_set(&_sc);
   esc->callbacks = _smart_callbacks;
   class = &_sc;

   return class;
}

EAPI Evas_Object *
elm_multibuttonentry_add(Evas_Object *parent)
{
   Evas_Object *obj;
   const char *str;
   int hpad = 0, vpad = 0;

   EINA_SAFETY_ON_NULL_RETURN_VAL(parent, NULL);

   obj = elm_widget_add(_elm_multibuttonentry_smart_class_new(), parent);
   if (!obj) return NULL;

   if (!elm_widget_sub_object_add(parent, obj))
     ERR("could not add %p as sub object of %p", obj, parent);

   ELM_MULTIBUTTONENTRY_DATA_GET(obj, sd);

   elm_layout_theme_set
      (obj, "multibuttonentry", "base", elm_widget_style_get(obj));
   elm_layout_signal_callback_add
      (obj, "elm,action,clicked", "", _on_layout_clicked, NULL);
   evas_object_event_callback_add
      (obj, EVAS_CALLBACK_KEY_DOWN, _on_layout_key_down, NULL);

   sd->box = elm_box_add(obj);
   str = elm_layout_data_get(obj, "horizontal_pad");
   if (str) hpad = atoi(str) / edje_object_base_scale_get(elm_layout_edje_get(obj));
   str = elm_layout_data_get(obj, "vertical_pad");
   if (str) vpad = atoi(str) / edje_object_base_scale_get(elm_layout_edje_get(obj));
   elm_box_padding_set
      (sd->box,
       hpad * elm_widget_scale_get(obj) * elm_config_scale_get(),
       vpad * elm_widget_scale_get(obj) * elm_config_scale_get());
   elm_box_layout_set(sd->box, _box_layout, obj, NULL);
   elm_layout_content_set(obj, "box.swallow", sd->box);
   evas_object_event_callback_add
      (sd->box, EVAS_CALLBACK_RESIZE, _on_box_resize, obj);

   sd->entry = elm_entry_add(obj);
   elm_entry_single_line_set(sd->entry, EINA_TRUE);
   elm_object_style_set(sd->entry, "multibuttonentry");
   elm_entry_scrollable_set(sd->entry, EINA_TRUE);
   elm_entry_cnp_mode_set(sd->entry, ELM_CNP_MODE_PLAINTEXT);
   evas_object_size_hint_weight_set
      (sd->entry, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set
      (sd->entry, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_event_callback_add
      (sd->entry, EVAS_CALLBACK_KEY_DOWN, _on_entry_key_down, obj);
   evas_object_smart_callback_add
      (sd->entry, "unfocused", _on_entry_unfocused, obj);
   evas_object_smart_callback_add
      (sd->entry, "changed", _on_entry_changed, obj);
   evas_object_smart_callback_add
      (sd->entry, "preedit,changed", _on_entry_changed, obj);
   elm_box_pack_end(sd->box, sd->entry);
   evas_object_show(sd->entry);

   elm_widget_can_focus_set(obj, EINA_TRUE);

   sd->editable = EINA_TRUE;
   sd->expanded = EINA_TRUE;
   sd->expanded_state = EINA_TRUE;

   // ACCESS
   _elm_access_object_register(obj, ELM_WIDGET_DATA(sd)->resize_obj);
   _elm_access_callback_set
      (_elm_access_object_get(obj), ELM_ACCESS_INFO, _access_mbe_info_cb, obj);
   _elm_access_callback_set
      (_elm_access_object_get(obj), ELM_ACCESS_CONTEXT_INFO, _access_mbe_context_info_cb, obj);

   //Tizen Only: This should be removed when eo is applied.
   ELM_WIDGET_DATA_GET(obj, wsd);
   wsd->on_create = EINA_FALSE;

   return obj;
}

EAPI Evas_Object *
elm_multibuttonentry_entry_get(const Evas_Object *obj)
{
   ELM_MULTIBUTTONENTRY_CHECK(obj) NULL;
   ELM_MULTIBUTTONENTRY_DATA_GET(obj, sd);

   return sd->entry;
}

EAPI void
elm_multibuttonentry_expanded_set(Evas_Object *obj,
                                  Eina_Bool expanded)
{
   ELM_MULTIBUTTONENTRY_CHECK(obj);
   ELM_MULTIBUTTONENTRY_DATA_GET(obj, sd);

   expanded = !!expanded;
   if (sd->expanded == expanded) return;
   sd->expanded = expanded;

   if (elm_object_focus_get(obj)) return;

   if (sd->expanded)
     _layout_expand(obj);
   else
     _layout_shrink(obj, EINA_FALSE);
}

EAPI Eina_Bool
elm_multibuttonentry_expanded_get(const Evas_Object *obj)
{
   ELM_MULTIBUTTONENTRY_CHECK(obj) EINA_FALSE;
   ELM_MULTIBUTTONENTRY_DATA_GET(obj, sd);

   return (elm_object_focus_get(obj) || sd->expanded);
}

EAPI void
elm_multibuttonentry_editable_set(Evas_Object *obj,
                                  Eina_Bool editable)
{
   ELM_MULTIBUTTONENTRY_CHECK(obj);
   ELM_MULTIBUTTONENTRY_DATA_GET(obj, sd);

   editable = !!editable;
   if (sd->editable == editable) return;
   sd->editable = editable;

   if (sd->editable)
     {
        if (!(sd->guide && _guide_packed(obj)) && sd->expanded)
          {
             elm_box_pack_end(sd->box, sd->entry);
             evas_object_show(sd->entry);
          }

        if (!elm_object_focus_get(obj))
          elm_layout_signal_emit(obj, "elm,state,event,block", "");

        if (_elm_config->access_mode)
          _elm_access_callback_set
            (_elm_access_object_get(obj), ELM_ACCESS_CONTEXT_INFO, _access_mbe_context_info_cb, obj);
     }
   else
     {
        if (!(sd->guide && _guide_packed(obj)))
          {
             elm_box_unpack(sd->box, sd->entry);
             evas_object_hide(sd->entry);
          }

        if (!elm_object_focus_get(obj))
          elm_layout_signal_emit(obj, "elm,state,event,allow", "");

        if (_elm_config->access_mode)
          _elm_access_callback_set
            (_elm_access_object_get(obj), ELM_ACCESS_CONTEXT_INFO, NULL, NULL);
     }
}

EAPI Eina_Bool
elm_multibuttonentry_editable_get(const Evas_Object *obj)
{
   ELM_MULTIBUTTONENTRY_CHECK(obj) EINA_FALSE;
   ELM_MULTIBUTTONENTRY_DATA_GET(obj, sd);

   return sd->editable;
}

EAPI Elm_Object_Item *
elm_multibuttonentry_item_prepend(Evas_Object *obj,
                                  const char *label,
                                  Evas_Smart_Cb func,
                                  const void *data)
{
   Elm_Multibuttonentry_Item *it;

   ELM_MULTIBUTTONENTRY_CHECK(obj) NULL;
   ELM_MULTIBUTTONENTRY_DATA_GET(obj, sd);

   if (!label) return NULL;

   // if guide text was shown, hide it
   if (sd->guide && _guide_packed(obj))
     {
        elm_box_unpack(sd->box, sd->guide);
        // TIZEN ONLY(20140624): below line should be checked with bounding box
        evas_object_smart_member_add(sd->guide, obj);
        // TIZEN ONLY(20140626): Until fixing bounding box issue, we send signal instead of "evas_object_hide"
        elm_layout_signal_emit(sd->guide, "elm,state,guidetext,hide", "elm");

        if (sd->editable)
          {
             elm_box_pack_end(sd->box, sd->entry);
             evas_object_show(sd->entry);
          }
     }

   it = _item_new(obj, label, func, data);
   if (!it) return NULL;

   sd->items = eina_list_prepend(sd->items, it);

   if (!elm_object_focus_get(obj) && !sd->expanded && sd->boxw)
     {
#ifdef _VI_EFFECT
        _item_adding_effect_add(obj, it);
#endif
        _layout_shrink(obj, EINA_TRUE);
     }
   else
     {
#ifdef _VI_EFFECT
        if (sd->boxh && sd->boxw)
          _item_adding_effect_add(obj, it);
        else
          {
             if (sd->label && _label_packed(obj))
               elm_box_pack_after(sd->box, VIEW(it), sd->label);
             else
               elm_box_pack_start(sd->box, VIEW(it));
             evas_object_show(VIEW(it));
          }
#else
        if (sd->label && _label_packed(obj))
          elm_box_pack_after(sd->box, VIEW(it), sd->label);
        else
          elm_box_pack_start(sd->box, VIEW(it));
        evas_object_show(VIEW(it));
#endif
     }

#ifdef _VI_EFFECT
   if (!sd->boxh || !sd->boxw)
     evas_object_smart_callback_call(obj, SIG_ITEM_ADDED, it);
#else
   evas_object_smart_callback_call(obj, SIG_ITEM_ADDED, it);
#endif

   return (Elm_Object_Item *)it;
}

EAPI Elm_Object_Item *
elm_multibuttonentry_item_append(Evas_Object *obj,
                                 const char *label,
                                 Evas_Smart_Cb func,
                                 const void *data)
{
   Elm_Multibuttonentry_Item *it;

   ELM_MULTIBUTTONENTRY_CHECK(obj) NULL;
   ELM_MULTIBUTTONENTRY_DATA_GET(obj, sd);

   if (!label) return NULL;

   // if guide text was shown, hide it
   if (sd->guide && _guide_packed(obj))
     {
        elm_box_unpack(sd->box, sd->guide);
        // TIZEN ONLY(20140624): below line should be checked with bounding box
        evas_object_smart_member_add(sd->guide, obj);
        // TIZEN ONLY(20140626): Until fixing bounding box issue, we send signal instead of "evas_object_hide"
        elm_layout_signal_emit(sd->guide, "elm,state,guidetext,hide", "elm");

        if (sd->editable)
          {
             elm_box_pack_end(sd->box, sd->entry);
             evas_object_show(sd->entry);
          }
     }

   it = _item_new(obj, label, func, data);
   if (!it) return NULL;

   sd->items = eina_list_append(sd->items, it);

   if (!elm_object_focus_get(obj) && !sd->expanded && sd->boxw)
     {
#ifdef _VI_EFFECT
        _item_adding_effect_add(obj, it);
#endif
        _layout_shrink(obj, EINA_TRUE);
     }
   else
     {
#ifdef _VI_EFFECT
        if (sd->boxh && sd->boxw)
          _item_adding_effect_add(obj, it);
        else
          {
             if (sd->editable)
               elm_box_pack_before(sd->box, VIEW(it), sd->entry);
             else
               elm_box_pack_end(sd->box, VIEW(it));
             evas_object_show(VIEW(it));
          }
#else
        if (sd->editable)
          elm_box_pack_before(sd->box, VIEW(it), sd->entry);
        else
          elm_box_pack_end(sd->box, VIEW(it));
        evas_object_show(VIEW(it));
#endif
     }

#ifdef _VI_EFFECT
   if (!sd->boxh || !sd->boxw)
     evas_object_smart_callback_call(obj, SIG_ITEM_ADDED, it);
#else
   evas_object_smart_callback_call(obj, SIG_ITEM_ADDED, it);
#endif

   return (Elm_Object_Item *)it;
}

EAPI Elm_Object_Item *
elm_multibuttonentry_item_insert_before(Evas_Object *obj,
                                        Elm_Object_Item *before,
                                        const char *label,
                                        Evas_Smart_Cb func,
                                        const void *data)
{
   Elm_Multibuttonentry_Item *it;

   ELM_MULTIBUTTONENTRY_CHECK(obj) NULL;
   ELM_MULTIBUTTONENTRY_DATA_GET(obj, sd);

   if (!label) return NULL;
   if (!before) return NULL;

   it = _item_new(obj, label, func, data);
   if (!it) return NULL;

   sd->items = eina_list_prepend_relative(sd->items, it, before);

   if (!elm_object_focus_get(obj) && !sd->expanded && sd->boxw)
     {
#ifdef _VI_EFFECT
        _item_adding_effect_add(obj, it);
#endif
        _layout_shrink(obj, EINA_TRUE);
     }
   else
     {
#ifdef _VI_EFFECT
        if (sd->boxh && sd->boxw)
          _item_adding_effect_add(obj, it);
        else
          {
             elm_box_pack_before(sd->box, VIEW(it), VIEW(before));
             evas_object_show(VIEW(it));
          }
#else
        elm_box_pack_before(sd->box, VIEW(it), VIEW(before));
        evas_object_show(VIEW(it));
#endif
     }

#ifdef _VI_EFFECT
   if (!sd->boxh || !sd->boxw)
     evas_object_smart_callback_call(obj, SIG_ITEM_ADDED, it);
#else
   evas_object_smart_callback_call(obj, SIG_ITEM_ADDED, it);
#endif

   return (Elm_Object_Item *)it;
}

EAPI Elm_Object_Item *
elm_multibuttonentry_item_insert_after(Evas_Object *obj,
                                       Elm_Object_Item *after,
                                       const char *label,
                                       Evas_Smart_Cb func,
                                       const void *data)
{
   Elm_Multibuttonentry_Item *it;

   ELM_MULTIBUTTONENTRY_CHECK(obj) NULL;
   ELM_MULTIBUTTONENTRY_DATA_GET(obj, sd);

   if (!label) return NULL;
   if (!after) return NULL;

   it = _item_new(obj, label, func, data);
   if (!it) return NULL;

   sd->items = eina_list_append_relative(sd->items, it, after);

   if (!elm_object_focus_get(obj) && !sd->expanded && sd->boxw)
     {
#ifdef _VI_EFFECT
        _item_adding_effect_add(obj, it);
#endif
        _layout_shrink(obj, EINA_TRUE);
     }
   else
     {
#ifdef _VI_EFFECT
        if (sd->boxw && sd->boxh)
          _item_adding_effect_add(obj, it);
        else
          {
             elm_box_pack_after(sd->box, VIEW(it), VIEW(after));
             evas_object_show(VIEW(it));
          }
#else
        elm_box_pack_after(sd->box, VIEW(it), VIEW(after));
        evas_object_show(VIEW(it));
#endif
     }

#ifdef _VI_EFFECT
   if (!sd->boxh || !sd->boxw)
     evas_object_smart_callback_call(obj, SIG_ITEM_ADDED, it);
#else
   evas_object_smart_callback_call(obj, SIG_ITEM_ADDED, it);
#endif

   return (Elm_Object_Item *)it;
}

EAPI const Eina_List *
elm_multibuttonentry_items_get(const Evas_Object *obj)
{
   ELM_MULTIBUTTONENTRY_CHECK(obj) NULL;
   ELM_MULTIBUTTONENTRY_DATA_GET(obj, sd);

   return sd->items;
}

EAPI Evas_Object *
elm_multibuttonentry_item_object_get(const Elm_Object_Item *it)
{
   ELM_MULTIBUTTONENTRY_ITEM_CHECK_OR_RETURN(it, NULL);

   return VIEW(it);
}

EAPI Elm_Object_Item *
elm_multibuttonentry_first_item_get(const Evas_Object *obj)
{
   ELM_MULTIBUTTONENTRY_CHECK(obj) NULL;
   ELM_MULTIBUTTONENTRY_DATA_GET(obj, sd);

   return eina_list_data_get(sd->items);
}

EAPI Elm_Object_Item *
elm_multibuttonentry_last_item_get(const Evas_Object *obj)
{
   ELM_MULTIBUTTONENTRY_CHECK(obj) NULL;
   ELM_MULTIBUTTONENTRY_DATA_GET(obj, sd);

   return eina_list_data_get(eina_list_last(sd->items));
}

EAPI Elm_Object_Item *
elm_multibuttonentry_selected_item_get(const Evas_Object *obj)
{
   ELM_MULTIBUTTONENTRY_CHECK(obj) NULL;
   ELM_MULTIBUTTONENTRY_DATA_GET(obj, sd);

   if (sd->selected_item)
     return eina_list_data_get(sd->selected_item);
   return NULL;
}

EAPI void
elm_multibuttonentry_item_selected_set(Elm_Object_Item *it,
                                       Eina_Bool selected)
{
   ELM_MULTIBUTTONENTRY_ITEM_CHECK_OR_RETURN(it);

   Elm_Multibuttonentry_Item *item = (Elm_Multibuttonentry_Item *)it;

   if (selected && !(item->selected))
     {
        elm_layout_signal_emit(VIEW(it), "elm,action,btn,selected", "");
     }
   else if (!selected && item->selected)
     {
        elm_layout_signal_emit(VIEW(it), "elm,action,btn,unselected", "");
     }
}

EAPI Eina_Bool
elm_multibuttonentry_item_selected_get(const Elm_Object_Item *it)
{
   ELM_MULTIBUTTONENTRY_ITEM_CHECK_OR_RETURN(it, EINA_FALSE);

   Elm_Multibuttonentry_Item *item = (Elm_Multibuttonentry_Item *)it;

   if (item->selected)
     return EINA_TRUE;

   return EINA_FALSE;
}

EAPI void
elm_multibuttonentry_clear(Evas_Object *obj)
{
   Elm_Multibuttonentry_Item *it;

   ELM_MULTIBUTTONENTRY_CHECK(obj);
   ELM_MULTIBUTTONENTRY_DATA_GET(obj, sd);

   EINA_LIST_FREE(sd->items, it)
     {
        Evas_Coord maxw, shrinkw;
#ifdef _VI_EFFECT
        Elm_Transit *trans;
        trans = (Elm_Transit *)evas_object_data_get(VIEW(it), "transit");
        if (trans)
          {
             elm_transit_del_cb_set(trans, NULL, NULL);
             elm_transit_del(trans);
          }
#endif
        maxw = (Evas_Coord)evas_object_data_get(VIEW(it), "maximum_width");
        if (maxw)
          evas_object_data_set(VIEW(it), "maximum_width", NULL);

        shrinkw = (Evas_Coord)evas_object_data_get(VIEW(it), "shrinked_item");
        if (shrinkw)
          evas_object_data_set(VIEW(it), "shrinked_item", NULL);
        elm_widget_item_free(it);
     }

   sd->items = NULL;
   sd->item_be_selected = EINA_FALSE;
   sd->expanded_state = EINA_TRUE;

   if (sd->end)
     {
        evas_object_data_set(obj, "end", NULL);
        evas_object_data_set(sd->end, "num", NULL);
        evas_object_del(sd->end);
        sd->end = NULL;
     }

   if (!sd->items && !elm_object_focus_get(obj) &&
       sd->guide && !_guide_packed(obj))
     {
        if (sd->editable)
          {
             elm_object_text_set(sd->entry, "");
             elm_box_unpack(sd->box, sd->entry);
             evas_object_hide(sd->entry);
          }

        elm_box_pack_end(sd->box, sd->guide);
        // TIZEN ONLY(20140626): Until fixing bounding box issue, we send signal instead of "evas_object_show"
        elm_layout_signal_emit(sd->guide, "elm,state,guidetext,show", "elm");
     }
}

EAPI Elm_Object_Item *
elm_multibuttonentry_item_prev_get(const Elm_Object_Item *it)
{
   Eina_List *l;
   Elm_Multibuttonentry_Item *item;

   ELM_MULTIBUTTONENTRY_ITEM_CHECK_OR_RETURN(it, NULL);
   ELM_MULTIBUTTONENTRY_DATA_GET(WIDGET(it), sd);

   EINA_LIST_FOREACH(sd->items, l, item)
     {
        if (item == (Elm_Multibuttonentry_Item *)it)
          {
             l = eina_list_prev(l);
             if (!l) return NULL;
             return eina_list_data_get(l);
          }
     }
   return NULL;
}

EAPI Elm_Object_Item *
elm_multibuttonentry_item_next_get(const Elm_Object_Item *it)
{
   Eina_List *l;
   Elm_Multibuttonentry_Item *item;

   ELM_MULTIBUTTONENTRY_ITEM_CHECK_OR_RETURN(it, NULL);
   ELM_MULTIBUTTONENTRY_DATA_GET(WIDGET(it), sd);

   EINA_LIST_FOREACH(sd->items, l, item)
     {
        if (item == (Elm_Multibuttonentry_Item *)it)
          {
             l = eina_list_next(l);
             if (!l) return NULL;
             return eina_list_data_get(l);
          }
     }
   return NULL;
}

EAPI void
elm_multibuttonentry_item_filter_append(Evas_Object *obj,
                                        Elm_Multibuttonentry_Item_Filter_Cb func,
                                        const void *data)
{
   Elm_Multibuttonentry_Item_Filter *ft = NULL;

   ELM_MULTIBUTTONENTRY_CHECK(obj);
   EINA_SAFETY_ON_NULL_RETURN(func);
   ELM_MULTIBUTTONENTRY_DATA_GET(obj, sd);

   ft = _filter_new(func, data);
   if (!ft) return;

   sd->filters = eina_list_append(sd->filters, ft);
}

EAPI void
elm_multibuttonentry_item_filter_prepend(Evas_Object *obj,
                                         Elm_Multibuttonentry_Item_Filter_Cb func,
                                         const void *data)
{
   Elm_Multibuttonentry_Item_Filter *ft = NULL;

   ELM_MULTIBUTTONENTRY_CHECK(obj);
   EINA_SAFETY_ON_NULL_RETURN(func);
   ELM_MULTIBUTTONENTRY_DATA_GET(obj, sd);

   ft = _filter_new(func, data);
   if (!ft) return;

   sd->filters = eina_list_prepend(sd->filters, ft);
}

EAPI void
elm_multibuttonentry_item_filter_remove(Evas_Object *obj,
                                        Elm_Multibuttonentry_Item_Filter_Cb func,
                                        const void *data)
{
   Elm_Multibuttonentry_Item_Filter *ft;
   Eina_List *l;

   ELM_MULTIBUTTONENTRY_CHECK(obj);
   EINA_SAFETY_ON_NULL_RETURN(func);
   ELM_MULTIBUTTONENTRY_DATA_GET(obj, sd);

   EINA_LIST_FOREACH(sd->filters, l, ft)
     {
        if ((ft->func == func) && (!data || ft->data == data))
          {
             sd->filters = eina_list_remove_list(sd->filters, l);
             free(ft);
             return;
          }
     }
}
