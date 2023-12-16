#include <Elementary.h>
#include "elm_priv.h"
#include "elm_widget_label.h"

EAPI const char ELM_LABEL_SMART_NAME[] = "elm_label";

static const char SIG_SLIDE_END[] = "slide,end";
static const char SIG_LANGUAGE_CHANGED[] = "language,changed";
// TIZEN_ONLY(131028): Support item, anchor formats
//////////////////////////////////////////////////////////////////////
//  Do Not Use this two smart callbacks "clicked" and "longpressed".
//  These will be deprecated soon.
static const char SIG_CLICKED[] = "clicked";
static const char SIG_LONGPRESSED[] = "longpressed";
//////////////////////////////////////////////////////////////////////
static const char SIG_ANCHOR_CLICKED[] = "anchor,clicked";
static const char SIG_ANCHOR_MOUSE_DOWN[] = "anchor,mouse,down";
static const char SIG_ANCHOR_MOUSE_UP[] = "anchor,mouse,up";
//////////////////////////////////////////////////////////////////////
static const char SIG_NO_SLIDING[] = "no,sliding";

static const int BATCH_PROCESS_SIZE = 100;

static const Evas_Smart_Cb_Description _smart_callbacks[] = {
   {SIG_LANGUAGE_CHANGED, ""},
   {SIG_SLIDE_END, ""},
   {SIG_CLICKED, ""},
   {SIG_LONGPRESSED, ""},
   {SIG_ANCHOR_CLICKED, ""},
   {SIG_ANCHOR_MOUSE_DOWN, ""},
   {SIG_ANCHOR_MOUSE_UP, ""},
   {SIG_NO_SLIDING, ""},
   {NULL, NULL}
};

static const Elm_Layout_Part_Alias_Description _text_aliases[] =
{
   {"default", "elm.text"},
   {NULL, NULL}
};

EVAS_SMART_SUBCLASS_NEW
  (ELM_LABEL_SMART_NAME, _elm_label, Elm_Label_Smart_Class,
  Elm_Layout_Smart_Class, elm_layout_smart_class_get, NULL);

// TIZEN_ONLY(20140628): Add elm_label_anchor_access_provider_* APIs and Support anchor access features.
static void
_elm_label_anchors_highlight_clear(Evas_Object *obj)
{
   char *data;

   ELM_LABEL_DATA_GET(obj, sd);

   if (sd->anchor_highlight_rect)
     {
        evas_object_data_del(sd->anchor_highlight_rect, "anchor_name");
        data = (char *)evas_object_data_del(sd->anchor_highlight_rect, "anchor_text");
        if (data) free(data);
        evas_object_del(sd->anchor_highlight_rect);
        sd->anchor_highlight_rect = NULL;
     }
   sd->anchor_highlight_pos = -1;
}
//

// TIZEN_ONLY(131028): Support item, anchor formats
static void
_elm_label_anchors_clear(Evas_Object *obj)
{
   Elm_Label_Anchor_Data *an;

   ELM_LABEL_DATA_GET(obj, sd);

   // TIZEN_ONLY(140520): Improve performance when *_smart_theme is called.
   if (sd->on_clear)
     return;
   sd->on_clear = EINA_TRUE;
   //
   // TIZEN_ONLY(20140628): Add elm_label_anchor_access_provider_* APIs and Support anchor access features.
   _elm_label_anchors_highlight_clear(obj);
   //

   while (sd->anchors)
     {
        an = sd->anchors->data;

        evas_textblock_cursor_free(an->start);
        evas_textblock_cursor_free(an->end);
        while (an->sel)
          {
             Sel *sel = an->sel->data;
             if (sel->obj) evas_object_del(sel->obj);
             sel->obj = NULL;
             free(sel);
             sel = NULL;
             an->sel = eina_list_remove_list(an->sel, an->sel);
          }
        free(an->name);
        free(an);
        sd->anchors = eina_list_remove_list(sd->anchors, sd->anchors);
     }

   sd->current_anchor_idx = 0;
   // TIZEN_ONLY(140520): Improve performance when *_smart_theme is called.
   sd->on_clear = EINA_FALSE;
   //
}

static void
_elm_label_anchors_get(Evas_Object *obj)
{
   const Eina_List *anchors_a, *anchors_item;
   Evas_Object *tb;
   Elm_Label_Anchor_Data *an = NULL;

   ELM_LABEL_DATA_GET(obj, sd);

   _elm_label_anchors_clear(obj);

   tb = (Evas_Object *)edje_object_part_object_get(ELM_WIDGET_DATA(sd)->resize_obj, "elm.text");

   anchors_a = evas_textblock_node_format_list_get(tb, "a");
   anchors_item = evas_textblock_node_format_list_get(tb, "item");

   if (anchors_a)
     {
        const Evas_Object_Textblock_Node_Format *node;
        const Eina_List *itr;
        EINA_LIST_FOREACH(anchors_a, itr, node)
          {
             const char *s = evas_textblock_node_format_text_get(node);
             char *p;
             an = calloc(1, sizeof(Elm_Label_Anchor_Data));
             if (!an)
               break;

             an->obj = obj;
             p = strstr(s, "href=");
             if (p)
               {
                  an->name = strdup(p + 5);
               }
             sd->anchors = eina_list_append(sd->anchors, an);
             an->start = evas_object_textblock_cursor_new(tb);
             an->end = evas_object_textblock_cursor_new(tb);
             evas_textblock_cursor_at_format_set(an->start, node);
             evas_textblock_cursor_copy(an->start, an->end);

             /* Close the anchor, if the anchor was without text,
              * free it as well */
             node = evas_textblock_node_format_next_get(node);
             for (; node; node = evas_textblock_node_format_next_get(node))
               {
                  s = evas_textblock_node_format_text_get(node);
                  if ((!strcmp(s, "- a")) || (!strcmp(s, "-a")))
                    break;
               }

             if (node)
               {
                  evas_textblock_cursor_at_format_set(an->end, node);
               }
             else if (!evas_textblock_cursor_compare(an->start, an->end))
               {
                  if (an->name) free(an->name);
                  evas_textblock_cursor_free(an->start);
                  evas_textblock_cursor_free(an->end);
                  sd->anchors = eina_list_remove(sd->anchors, an);
                  free(an);
               }
             an = NULL;
          }
     }

   if (anchors_item)
     {
        const Evas_Object_Textblock_Node_Format *node;
        const Eina_List *itr;
        EINA_LIST_FOREACH(anchors_item, itr, node)
          {
             const char *s = evas_textblock_node_format_text_get(node);
             char *p;
             an = calloc(1, sizeof(Elm_Label_Anchor_Data));
             if (!an)
               break;

             an->obj = obj;
             an->item = 1;
             p = strstr(s, "href=");
             if (p)
               {
                  an->name = strdup(p + 5);
               }
             sd->anchors = eina_list_append(sd->anchors, an);
             an->start = evas_object_textblock_cursor_new(tb);
             an->end = evas_object_textblock_cursor_new(tb);
             evas_textblock_cursor_at_format_set(an->start, node);
             evas_textblock_cursor_copy(an->start, an->end);
             /* Although needed in textblock, don't bother with finding the end
              * here cause it doesn't really matter. */
          }
     }
}

static Evas_Object *
_elm_label_item_get(Evas_Object *obj, char *name)
{
   Eina_List *l;
   Evas_Object *item_obj, *tb;
   Elm_Label_Item_Provider *ip;
   Evas *e;

   ELM_LABEL_DATA_GET(obj, sd);

   tb = (Evas_Object *)edje_object_part_object_get(ELM_WIDGET_DATA(sd)->resize_obj, "elm.text");
   e = evas_object_evas_get(tb);

   EINA_LIST_FOREACH(sd->item_providers, l, ip)
     {
        item_obj = ip->func(ip->data, obj, name);
        if (item_obj) return item_obj;
     }
   if (!strncmp(name, "file://", 7))
     {
        const char *fname = name + 7;

        item_obj = evas_object_image_filled_add(e);
        evas_object_image_file_set(item_obj, fname, NULL);
        if (evas_object_image_load_error_get(item_obj) != EVAS_LOAD_ERROR_NONE)
          {
             evas_object_del(item_obj);
             item_obj = edje_object_add(e);
             elm_widget_theme_object_set
                (obj, item_obj, "entry/emoticon", "wtf",
                 elm_widget_style_get(obj));
          }
        return item_obj;
     }

   item_obj = edje_object_add(e);
   if (!elm_widget_theme_object_set
       (obj, item_obj, "entry", name, elm_widget_style_get(obj)))
     elm_widget_theme_object_set
        (obj, item_obj, "entry/emoticon", "wtf", elm_widget_style_get(obj));
   return item_obj;
}

static Eina_Bool
_long_press_cb(void *data)
{
   ELM_LABEL_DATA_GET(data, sd);

   sd->longpress_timer = NULL;
   if (sd->mouse_down_flag)
     evas_object_smart_callback_call(data, SIG_LONGPRESSED, NULL);

   return ECORE_CALLBACK_CANCEL;
}

static void
_label_mouse_down_cb(void *data, Evas *e __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info)
{
   Evas_Event_Mouse_Down *ev = event_info;

   ELM_LABEL_DATA_GET(data, sd);

   sd->mouse_down_flag = EINA_TRUE;
   sd->down_x = ev->canvas.x;
   sd->down_y = ev->canvas.y;
   if (ev->button == 1)
     {
        if (sd->longpress_timer) ecore_timer_del(sd->longpress_timer);
        sd->longpress_timer = ecore_timer_add
           (_elm_config->longpress_timeout, _long_press_cb, data);
     }
}

static void
_label_mouse_up_cb(void *data, Evas *e __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info)
{
   Evas_Event_Mouse_Down *ev = event_info;

   ELM_LABEL_DATA_GET(data, sd);

   if (ev->button == 1 && sd->longpress_timer)
     {
        ecore_timer_del(sd->longpress_timer);
        sd->longpress_timer = NULL;
     }

   if (sd->mouse_down_flag)
     evas_object_smart_callback_call(data, SIG_CLICKED, NULL);
   sd->mouse_down_flag = EINA_FALSE;
}

static void
_label_mouse_move_cb(void *data, Evas *e __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info)
{
   Evas_Event_Mouse_Move *ev = event_info;
   Evas_Coord mx, my;
   Evas_Coord x, y, w, h;

   ELM_LABEL_DATA_GET(data, sd);

   if (!sd->mouse_down_flag)
     return;

   evas_object_geometry_get(data, &x, &y, &w, &h);

   if (ev->cur.canvas.x > sd->down_x)
     mx = ev->cur.canvas.x - sd->down_x;
   else
     mx = sd->down_x - ev->cur.canvas.x;

   if (ev->cur.canvas.y > sd->down_y)
     my = ev->cur.canvas.y - sd->down_y;
   else
     my = sd->down_y - ev->cur.canvas.y;

   if ((mx > _elm_config->finger_size) ||
       (my > _elm_config->finger_size))
     {
        sd->mouse_down_flag = EINA_FALSE;
        return;
     }

   if ((ev->cur.canvas.x < x) || (ev->cur.canvas.x > x + w) ||
       (ev->cur.canvas.y < y) || (ev->cur.canvas.y > y + h))
     sd->mouse_down_flag = EINA_FALSE;
}

static void
_label_anchor_mouse_down_cb(void *data, Evas *e __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info)
{
   Elm_Label_Anchor_Data *an = (Elm_Label_Anchor_Data *)data;
   Evas_Event_Mouse_Down *ev = event_info;
   Elm_Label_Anchor_Info ai;

   an->mouse_down_flag = EINA_TRUE;
   ai.name = an->name;
   an->down_x = ev->canvas.x;
   an->down_y = ev->canvas.y;

   evas_object_smart_callback_call(an->obj, SIG_ANCHOR_MOUSE_DOWN, &ai);
}

static void
_label_anchor_mouse_up_cb(void *data, Evas *e __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   Elm_Label_Anchor_Data *an = (Elm_Label_Anchor_Data *)data;
   Elm_Label_Anchor_Info ai;

   ai.name = an->name;
   evas_object_smart_callback_call(an->obj, SIG_ANCHOR_MOUSE_UP, &ai);
   if (an->mouse_down_flag)
     evas_object_smart_callback_call(an->obj, SIG_ANCHOR_CLICKED, &ai);
   an->mouse_down_flag = EINA_FALSE;
}

static void
_label_anchor_mouse_move_cb(void *data, Evas *e __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info)
{
   Evas_Event_Mouse_Move *ev = event_info;
   Elm_Label_Anchor_Data *an = (Elm_Label_Anchor_Data *)data;
   Evas_Coord mx, my;

   if (!an->mouse_down_flag)
     return;

   if (ev->cur.canvas.x > an->down_x)
     mx = ev->cur.canvas.x - an->down_x;
   else
     mx = an->down_x - ev->cur.canvas.x;

   if (ev->cur.canvas.y > an->down_y)
     my = ev->cur.canvas.y - an->down_y;
   else
     my = an->down_y - ev->cur.canvas.y;

   if ((mx > _elm_config->finger_size) ||
       (my > _elm_config->finger_size))
     {
        an->mouse_down_flag = EINA_FALSE;
        return;
     }
}

static void
_label_anchors_show(void *data,
                 Evas *e __UNUSED__,
                 Evas_Object *obj __UNUSED__,
                 void *event_info __UNUSED__)
{
   Eina_List *l, *ll;
   Elm_Label_Anchor_Data *an;
   Evas_Object *label_obj;
   Sel *sel;

   label_obj = (Evas_Object *)data;
   ELM_LABEL_DATA_GET(label_obj, sd);

   EINA_LIST_FOREACH(sd->anchors, l, an)
     {
        EINA_LIST_FOREACH(an->sel, ll, sel)
          {
             if (an->item)
               {
                  evas_object_show(sel->obj);
               }
             else
               {
                  if (sel->obj)
                    evas_object_show(sel->obj);
               }
          }
     }
}

static void
_label_anchors_hide(void *data,
                 Evas *e __UNUSED__,
                 Evas_Object *obj __UNUSED__,
                 void *event_info __UNUSED__)
{
   Eina_List *l, *ll;
   Elm_Label_Anchor_Data *an;
   Evas_Object *label_obj;
   Sel *sel;

   label_obj = (Evas_Object *)data;
   ELM_LABEL_DATA_GET(label_obj, sd);

   // TIZEN_ONLY(20140628): Add elm_label_anchor_access_provider_* APIs and Support anchor access features.
   _elm_label_anchors_highlight_clear(label_obj);
   //
   EINA_LIST_FOREACH(sd->anchors, l, an)
     {
        EINA_LIST_FOREACH(an->sel, ll, sel)
          {
             if (an->item)
               {
                  evas_object_hide(sel->obj);
               }
             else
               {
                  if (sel->obj)
                    evas_object_hide(sel->obj);
               }
          }
     }
}

static void
_label_anchors_move(void *data,
                 Evas *e __UNUSED__,
                 Evas_Object *obj __UNUSED__,
                 void *event_info __UNUSED__)
{
   Evas_Coord x, y, w, h;
   Eina_List *l, *ll;
   Evas_Object *tb;
   Elm_Label_Anchor_Data *an;
   Sel *sel;

   ELM_LABEL_DATA_GET(data, sd);

   tb = (Evas_Object *)edje_object_part_object_get(ELM_WIDGET_DATA(sd)->resize_obj, "elm.text");

   evas_object_geometry_get(tb, &x, &y, &w, &h);

   EINA_LIST_FOREACH(sd->anchors, l, an)
     {
        EINA_LIST_FOREACH(an->sel, ll, sel)
          {
             if (an->item)
               {
                  Evas_Coord cx, cy, cw, ch;

                  if (!evas_textblock_cursor_format_item_geometry_get
                      (an->start, &cx, &cy, &cw, &ch))
                    continue;
                  evas_object_move(sel->obj, x + cx, y + cy);
                  evas_object_resize(sel->obj, cw, ch);
               }
             else
               {
                  if (sel->obj)
                    {
                       evas_object_move(sel->obj, x + sel->rect.x, y + sel->rect.y);
                       evas_object_resize(sel->obj, sel->rect.w, sel->rect.h);
                    }
               }
          }
     }
}

static void
_elm_label_anchor_item_update(Evas_Object *obj, Elm_Label_Anchor_Data *an, Evas_Object *tb)
{
   Evas_Coord x, y, w, h;
   Evas_Object *smart, *clip;
   Eina_List *ll, *range = NULL;
   Sel *sel;
   Evas *e;

   e = evas_object_evas_get(tb);
   smart = evas_object_smart_parent_get(tb);
   clip = evas_object_clip_get(tb);

   evas_object_geometry_get(tb, &x, &y, &w, &h);

   if (an->item)
     {
        if (!an->sel)
          {
             Evas_Object *item_obj;

             sel = calloc(1, sizeof(Sel));
             an->sel = eina_list_append(an->sel, sel);

             item_obj = _elm_label_item_get(obj, an->name);
             evas_object_smart_member_add(item_obj, smart);
             evas_object_stack_above(item_obj, tb);
             evas_object_clip_set(item_obj, clip);
             evas_object_repeat_events_set(item_obj, EINA_TRUE);
             evas_object_show(item_obj);
             sel->obj = item_obj;
          }
     }
   else
     {
        range =
          evas_textblock_cursor_range_geometry_get(an->start, an->end);
        if (eina_list_count(range) != eina_list_count(an->sel))
          {
             while (an->sel)
               {
                  sel = an->sel->data;
                  if (sel->obj) evas_object_del(sel->obj);
                  free(sel);
                  an->sel = eina_list_remove_list(an->sel, an->sel);
               }
             for (ll = range; ll; ll = eina_list_next(ll))
               {
                  Evas_Object *a_obj;

                  sel = calloc(1, sizeof(Sel));
                  an->sel = eina_list_append(an->sel, sel);

                  a_obj = evas_object_rectangle_add(e);
                  evas_object_color_set(a_obj, 0, 0, 0, 0);
                  evas_object_smart_member_add(a_obj, smart);
                  evas_object_stack_above(a_obj, tb);
                  evas_object_clip_set(a_obj, clip);
                  evas_object_repeat_events_set(a_obj, EINA_TRUE);
                  evas_object_event_callback_add(a_obj, EVAS_CALLBACK_MOUSE_DOWN, _label_anchor_mouse_down_cb, an);
                  evas_object_event_callback_add(a_obj, EVAS_CALLBACK_MOUSE_UP, _label_anchor_mouse_up_cb, an);
                  evas_object_event_callback_add(a_obj, EVAS_CALLBACK_MOUSE_MOVE, _label_anchor_mouse_move_cb, an);
                  evas_object_show(a_obj);
                  sel->obj = a_obj;
               }
          }
     }

   EINA_LIST_FOREACH(an->sel, ll, sel)
     {
        if (an->item)
          {
             Evas_Coord cx, cy, cw, ch;

             if (!evas_textblock_cursor_format_item_geometry_get
                 (an->start, &cx, &cy, &cw, &ch))
               {
                  evas_object_hide(sel->obj);
                  continue;
               }

             evas_object_move(sel->obj, x + cx, y + cy);
             evas_object_resize(sel->obj, cw, ch);
             evas_object_show(sel->obj);
          }
        else
          {
             Evas_Textblock_Rectangle *r;

             r = range->data;
             *(&(sel->rect)) = *r;
             if (sel->obj)
               {
                  evas_object_move(sel->obj, x + r->x, y + r->y);
                  evas_object_resize(sel->obj, r->w, r->h);
               }
             range = eina_list_remove_list(range, range);
             free(r);
          }
     }
}

static void
_elm_label_batch_anchors_update_internal(Evas_Object *obj)
{
   Evas_Object *tb;
   Elm_Label_Anchor_Data *an = NULL;
   int anchor_index;
   int indexLimiter;
   int anchorCount;

   ELM_LABEL_DATA_GET(obj, sd);

   // TIZEN_ONLY(140520): Improve performance when *_smart_theme is called.
   if (sd->on_clear)
     return;

   tb = (Evas_Object *)edje_object_part_object_get(ELM_WIDGET_DATA(sd)->resize_obj, "elm.text");

   sd->forward_rendering = !sd->forward_rendering;
   indexLimiter = sd->current_anchor_idx + BATCH_PROCESS_SIZE;
   anchorCount = eina_list_count(sd->anchors);
   for (anchor_index = sd->current_anchor_idx; (anchor_index < indexLimiter && anchor_index < anchorCount); ++anchor_index)
     {
        if(sd->forward_rendering)
          {
             //Forward Draw/Rendering
             int _idx = anchor_index - sd->forward_batch_processed;
             if(_idx >= 0)
               an = eina_list_nth(sd->anchors, _idx);
          }
        else
          {
             //Reverse Draw/Rendering
             int _idx = anchor_index - sd->forward_batch_processed;
             _idx = anchorCount - _idx - 1;
             if(_idx >= 0)
               an = eina_list_nth(sd->anchors, _idx);
          }

        if(!an)
          continue;

        _elm_label_anchor_item_update(obj, an, tb);
     }
   sd->current_anchor_idx = anchor_index;
   if (sd->forward_rendering)
      sd->forward_batch_processed += BATCH_PROCESS_SIZE;
}

//static void
//_elm_label_anchors_update(Evas_Object *obj)
//{
//   Eina_List *l;
//   Evas_Object *tb;
//   Elm_Label_Anchor_Data *an;
//
//   ELM_LABEL_DATA_GET(obj, sd);
//
//   // TIZEN_ONLY(140520): Improve performance when *_smart_theme is called.
//   if (sd->on_clear)
//     return;
//   //
//
//   tb = (Evas_Object *)edje_object_part_object_get(ELM_WIDGET_DATA(sd)->resize_obj, "elm.text");
//
//   EINA_LIST_FOREACH(sd->anchors, l, an)
//     {
//        _elm_label_anchor_item_update(obj, an, tb);
//     }
//}
/////////////////////////////////////////////////

static void
_recalc(void *data)
{
   ELM_LABEL_DATA_GET(data, sd);

   Evas_Coord minw = -1, minh = -1;
   Evas_Coord resw;

   evas_event_freeze(evas_object_evas_get(data));
   evas_object_geometry_get
     (ELM_WIDGET_DATA(sd)->resize_obj, NULL, NULL, &resw, NULL);
   if (sd->wrap_w > resw)
     resw = sd->wrap_w;

   edje_object_size_min_restricted_calc
     (ELM_WIDGET_DATA(sd)->resize_obj, &minw, &minh, resw, 0);

   /* This is a hack to workaround the way min size hints are treated.
    * If the minimum width is smaller than the restricted width, it means
    * the mininmum doesn't matter. */
   if ((minw <= resw) && (minw != sd->wrap_w))
     {
        Evas_Coord ominw = -1;

        evas_object_size_hint_min_get(data, &ominw, NULL);
        minw = ominw;
     }

   evas_object_size_hint_min_set(data, minw, minh);
   evas_event_thaw(evas_object_evas_get(data));
   evas_event_thaw_eval(evas_object_evas_get(data));
}

static void
_label_format_set(Evas_Object *obj,
                  const char *format)
{
   if (format)
     edje_object_part_text_style_user_push(obj, "elm.text", format);
   else
     edje_object_part_text_style_user_pop(obj, "elm.text");
}

static void
_label_slide_change(Evas_Object *obj)
{
   Evas_Object *tb;
   char *plaintxt;
   int plainlen = 0;

   ELM_LABEL_DATA_GET(obj, sd);

   edje_object_signal_emit(ELM_WIDGET_DATA(sd)->resize_obj,
                           "elm,state,slide,stop", "elm");

   //doesn't support multiline slide effect
   if (sd->linewrap)
     {
        WRN("Doesn't support slide effect for multiline! : label=%p", obj);
        return;
     }

   //stop if the text is none.
   plaintxt = _elm_util_mkup_to_text
       (edje_object_part_text_get
         (ELM_WIDGET_DATA(sd)->resize_obj, "elm.text"));
   if (plaintxt)
     {
        plainlen = strlen(plaintxt);
        free(plaintxt);
     }
   if (plainlen < 1) return;

   //has slide effect.
   if (sd->slide_mode != ELM_LABEL_SLIDE_MODE_NONE)
     {
        if (sd->ellipsis)
          {
             sd->slide_ellipsis = EINA_TRUE;
             elm_label_ellipsis_set(obj, EINA_FALSE);
          }

        Evas_Coord w, tb_w = 0;

        tb = (Evas_Object *) edje_object_part_object_get(ELM_WIDGET_DATA(sd)->resize_obj, "elm.text");

        if (tb)
          {
             evas_object_textblock_size_native_get(tb, &tb_w, NULL);
             evas_object_geometry_get(ELM_WIDGET_DATA(sd)->resize_obj,
                                                 NULL, NULL, &w, NULL);
          }

        //slide only if the slide area is smaller than text width size.
        if (sd->slide_mode == ELM_LABEL_SLIDE_MODE_AUTO)
          {
             if ((tb_w > 0) && (tb_w < w))
               {
                  if (sd->slide_ellipsis)
                    {
                       sd->slide_ellipsis = EINA_FALSE;
                       elm_label_ellipsis_set(obj, EINA_TRUE);
                    }
				  evas_object_smart_callback_call(obj, SIG_NO_SLIDING, NULL);
                  return;
               }
          }
        Edje_Message_Float_Set *msg =
          alloca(sizeof(Edje_Message_Float_Set) + (sizeof(double)));

        msg->count = 1;

        if ((tb_w > 0) && (tb_w >= w))
          msg->val[0] = sd->slide_duration ? sd->slide_duration : (double) (tb_w - w);
        else
          msg->val[0] = sd->slide_duration;

        if (sd->slide_duration)
          edje_object_message_send
             (ELM_WIDGET_DATA(sd)->resize_obj, EDJE_MESSAGE_FLOAT_SET, 0, msg);
        else
          edje_object_message_send
             (ELM_WIDGET_DATA(sd)->resize_obj, EDJE_MESSAGE_FLOAT_SET, 1, msg);

        edje_object_signal_emit(ELM_WIDGET_DATA(sd)->resize_obj, "elm,state,slide,start", "elm");
     }
   //no slide effect.
   else
     {
        if (sd->slide_ellipsis)
          {
             sd->slide_ellipsis = EINA_FALSE;
             elm_label_ellipsis_set(obj, EINA_TRUE);
          }
     }
}

static Eina_Bool _elm_label_batch_anchors_update(void *data)
{
   Evas_Object *obj = (Evas_Object*)data;
   ELM_LABEL_CHECK(obj) ECORE_CALLBACK_CANCEL;
   ELM_LABEL_DATA_GET_OR_RETURN_VAL(obj, sd, ECORE_CALLBACK_CANCEL);

   if (sd->current_anchor_idx >= eina_list_count(sd->anchors))
     {
        sd->batch_anchor_update_handle = NULL;
        return ECORE_CALLBACK_CANCEL;
     }
   else
     {
        _elm_label_batch_anchors_update_internal(obj);
        return ECORE_CALLBACK_RENEW;
     }
}

static Eina_Bool
_elm_label_smart_theme(Evas_Object *obj)
{
   Evas_Object *tb;
   Eina_Bool ret;

   ELM_LABEL_DATA_GET(obj, sd);

   evas_event_freeze(evas_object_evas_get(obj));

   ret = ELM_WIDGET_CLASS(_elm_label_parent_sc)->theme(obj);
   if (!ret) goto end;

   _label_format_set(ELM_WIDGET_DATA(sd)->resize_obj, sd->format);
   _label_slide_change(obj);

   // TIZEN_ONLY(131028): Support item, anchor formats
   tb = (Evas_Object *)edje_object_part_object_get(ELM_WIDGET_DATA(sd)->resize_obj, "elm.text");
   //
   // TIZEN_ONLY(140520): Improve performance when *_smart_theme is called.
   if (sd->tb != tb)
     {
        _elm_label_anchors_get(obj);
        // TIZEN_ONLY(140626): Add callbacks to textblock for updating anchor, label
        evas_object_event_callback_add(tb, EVAS_CALLBACK_SHOW, _label_anchors_show, obj);
        evas_object_event_callback_add(tb, EVAS_CALLBACK_HIDE, _label_anchors_hide, obj);
        evas_object_event_callback_add(tb, EVAS_CALLBACK_MOVE, _label_anchors_move, obj);
        //
        // TIZEN_ONLY(140701): Prevent to append duplicated callbacks
        evas_object_event_callback_add(tb, EVAS_CALLBACK_MOUSE_DOWN, _label_mouse_down_cb, obj);
        evas_object_event_callback_add(tb, EVAS_CALLBACK_MOUSE_UP, _label_mouse_up_cb, obj);
        evas_object_event_callback_add(tb, EVAS_CALLBACK_MOUSE_MOVE, _label_mouse_move_cb, obj);
        //
     }
   //_elm_label_anchors_update(obj);
   if (eina_list_count(sd->anchors) > 0)
     {
        sd->forward_rendering = EINA_FALSE;
        sd->forward_batch_processed = 0;
        sd->current_anchor_idx = 0;
        _elm_label_batch_anchors_update_internal(obj);

        /* Idler not running already*/
        if (!sd->batch_anchor_update_handle)
          sd->batch_anchor_update_handle = ecore_idler_add(_elm_label_batch_anchors_update, obj);
     }
   sd->tb = tb;
   sd->lastw = 0;
   //
   elm_layout_sizing_eval(obj);
end:
   evas_event_thaw(evas_object_evas_get(obj));
   evas_event_thaw_eval(evas_object_evas_get(obj));

   return ret;
}

static void
_elm_label_smart_sizing_eval(Evas_Object *obj)
{
   Evas_Coord minw = -1, minh = -1;
   Evas_Coord resw, resh;

   ELM_LABEL_DATA_GET(obj, sd);

   if (sd->on_clear)
     return;

   if (sd->linewrap)
     {
        evas_object_geometry_get
          (ELM_WIDGET_DATA(sd)->resize_obj, NULL, NULL, &resw, &resh);
        // TIZEN_ONLY(20140606): Update anchor when textblock geometry is changed.
        //if (resw == sd->lastw) return;
        if ((resw == sd->lastw) || ((resw == 0) && (sd->wrap_w <= 0))) goto end;
        //
        sd->lastw = resw;
        _recalc(obj);
     }
   else
     {
        evas_event_freeze(evas_object_evas_get(obj));
        evas_object_geometry_get
          (ELM_WIDGET_DATA(sd)->resize_obj, NULL, NULL, &resw, &resh);
        edje_object_size_min_calc
          (ELM_WIDGET_DATA(sd)->resize_obj, &minw, &minh);
        if (sd->wrap_w > 0 && minw > sd->wrap_w) minw = sd->wrap_w;
        evas_object_size_hint_min_set(obj, minw, minh);
        evas_event_thaw(evas_object_evas_get(obj));
        evas_event_thaw_eval(evas_object_evas_get(obj));
     }

   // TIZEN_ONLY(20140606): Update anchor when textblock geometry is changed.
end:
   if (sd->tb)
     {
        Evas_Coord tx, ty, tw, th;
        evas_object_geometry_get(sd->tb, &tx, &ty, &tw, &th);

        // TIZEN_ONLY(20140623): Optimize performance for anchor, item.
        if ((tx != sd->tx) || (ty != sd->ty) ||
            (tw != sd->tw) || (th != sd->th) || sd->anchors_force_update)
          {
             //_elm_label_anchors_update(obj);
             if (eina_list_count(sd->anchors) > 0)
               {
                  sd->forward_rendering = EINA_FALSE;
                  sd->forward_batch_processed = 0;
                  sd->current_anchor_idx = 0;
                  _elm_label_batch_anchors_update_internal(obj);

                  /* Idler not running already*/
                  if (!sd->batch_anchor_update_handle)
                    sd->batch_anchor_update_handle = ecore_idler_add(_elm_label_batch_anchors_update, obj);
               }
          }
        //
        sd->anchors_force_update = EINA_FALSE;
        sd->tx = tx;
        sd->ty = ty;
        sd->tw = tw;
        sd->th = th;
     }
   //
}

static void
_on_label_resize(void *data,
                 Evas *e __UNUSED__,
                 Evas_Object *obj __UNUSED__,
                 void *event_info __UNUSED__)
{
   ELM_LABEL_DATA_GET(data, sd);

   if (sd->linewrap)
     {
        elm_layout_sizing_eval(data);
     }
   else
     {
        //_elm_label_anchors_update(data);
      if (eina_list_count(sd->anchors) > 0)
        {
           sd->forward_rendering = EINA_FALSE;
           sd->forward_batch_processed = 0;
           sd->current_anchor_idx = 0;
           _elm_label_batch_anchors_update_internal(data);

           /* Idler not running already*/
           if (!sd->batch_anchor_update_handle)
             sd->batch_anchor_update_handle = ecore_idler_add(_elm_label_batch_anchors_update, data);
        }
     }
}

static int
_get_value_in_key_string(const char *oldstring,
                         const char *key,
                         char **value)
{
   char *curlocater, *endtag;
   int firstindex = 0, foundflag = -1;

   curlocater = strstr(oldstring, key);
   if (curlocater)
     {
        int key_len = strlen(key);
        endtag = curlocater + key_len;
        if ((!endtag) || (*endtag != '='))
          {
             foundflag = 0;
             return -1;
          }
        firstindex = abs(oldstring - curlocater);
        firstindex += key_len + 1; // strlen("key") + strlen("=")
        *value = (char *)oldstring + firstindex;

        foundflag = 1;
     }
   else
     {
        foundflag = 0;
     }

   if (foundflag == 1) return 0;

   return -1;
}

static int
_strbuf_key_value_replace(Eina_Strbuf *srcbuf,
                          const char *key,
                          const char *value,
                          int deleteflag)
{
   char *kvalue;
   const char *srcstring = NULL;

   srcstring = eina_strbuf_string_get(srcbuf);

   if (_get_value_in_key_string(srcstring, key, &kvalue) == 0)
     {
        const char *val_end;
        int val_end_idx = 0;
        int key_start_idx = 0;
        val_end = strchr(kvalue, ' ');

        if (val_end)
          val_end_idx = val_end - srcstring;
        else
          val_end_idx = kvalue - srcstring + strlen(kvalue) - 1;

        /* -1 is because of the '=' */
        key_start_idx = kvalue - srcstring - 1 - strlen(key);
        eina_strbuf_remove(srcbuf, key_start_idx, val_end_idx);
        if (!deleteflag)
          {
             eina_strbuf_insert_printf(srcbuf, "%s=%s", key_start_idx, key,
                                       value);
          }
     }
   else if (!deleteflag)
     {
        if (*srcstring)
          {
             /* -1 because we want it before the ' */
             eina_strbuf_insert_printf
               (srcbuf, " %s=%s", eina_strbuf_length_get(srcbuf) - 1, key,
               value);
          }
        else
          {
             eina_strbuf_append_printf(srcbuf, "DEFAULT='%s=%s'", key, value);
          }
     }

   return 0;
}

static int
_stringshare_key_value_replace(const char **srcstring,
                               const char *key,
                               const char *value,
                               int deleteflag)
{
   Eina_Strbuf *sharebuf = NULL;

   sharebuf = eina_strbuf_new();
   eina_strbuf_append(sharebuf, *srcstring);
   _strbuf_key_value_replace(sharebuf, key, value, deleteflag);
   eina_stringshare_del(*srcstring);
   *srcstring = eina_stringshare_add(eina_strbuf_string_get(sharebuf));
   eina_strbuf_free(sharebuf);

   return 0;
}

static Eina_Bool
_elm_label_smart_text_set(Evas_Object *obj,
                          const char *item,
                          const char *label)
{
   ELM_LABEL_DATA_GET(obj, sd);

   // TIZEN_ONLY(20150526): sizing_eval called twice from layout and label.
   elm_layout_freeze(obj);
   //
   if (!label) label = "";
   _label_format_set(ELM_WIDGET_DATA(sd)->resize_obj, sd->format);

   if (_elm_label_parent_sc->text_set(obj, item, label))
     {
        sd->lastw = -1;
        // TIZEN_ONLY(20140623): Optimize performance for anchor, item.
        if (!item || !strcmp(item, "elm.text"))
          {
             _elm_label_anchors_get(obj);
             sd->anchors_force_update = EINA_TRUE;
          }
        //
        // TIZEN_ONLY(20150526): sizing_eval called twice from layout and label.
        //_elm_label_smart_sizing_eval(obj);
        elm_layout_thaw(obj);
        //
        return EINA_TRUE;
     }

   // TIZEN_ONLY(20150526): sizing_eval called twice from layout and label.
   elm_layout_thaw(obj);
   //
   return EINA_FALSE;
}

static Eina_Bool
_elm_label_smart_translate(Evas_Object *obj)
{
   evas_object_smart_callback_call(obj, SIG_LANGUAGE_CHANGED, NULL);

   return EINA_TRUE;
}

static char *
_access_info_cb(void *data __UNUSED__, Evas_Object *obj)
{
   const char *txt = elm_widget_access_info_get(obj);

   if (!txt) txt = _elm_util_mkup_to_text(elm_layout_text_get(obj, NULL));
   if (txt) return strdup(txt);

   return NULL;
}

static void
_on_slide_end(void *data, Evas_Object *obj __UNUSED__,
              const char *emission __UNUSED__, const char *source __UNUSED__)
{
   evas_object_smart_callback_call(data, SIG_SLIDE_END, NULL);
}

static void
_on_no_sliding(void *data, Evas_Object *obj __UNUSED__,
              const char *emission __UNUSED__, const char *source __UNUSED__)
{
   evas_object_smart_callback_call(data, SIG_NO_SLIDING, NULL);
}

static void
_elm_label_smart_add(Evas_Object *obj)
{
   EVAS_SMART_DATA_ALLOC(obj, Elm_Label_Smart_Data);

   ELM_WIDGET_CLASS(_elm_label_parent_sc)->base.add(obj);
}

static void
_elm_label_smart_del(Evas_Object *obj)
{
   ELM_LABEL_DATA_GET(obj, sd);

   evas_event_freeze(evas_object_evas_get(obj));

   // TIZEN_ONLY(131203): Clear anchors and long press timer
   if (sd->batch_anchor_update_handle)
     ecore_idler_del(sd->batch_anchor_update_handle);
   sd->batch_anchor_update_handle = NULL;

   _elm_label_anchors_clear(obj);

   if (sd->longpress_timer) ecore_timer_del(sd->longpress_timer);
   //

   // TIZEN_ONLY(140425): Remove item providers when the label is deleted.
   if (sd->item_providers)
     {
        Elm_Label_Item_Provider *ip;
        EINA_LIST_FREE(sd->item_providers, ip)
          {
             free(ip);
          }
     }
   //
   // TIZEN_ONLY(20140628): Add elm_label_anchor_access_provider_* APIs and Support anchor access features.
   if (sd->anchor_access_providers)
     {
        Elm_Label_Anchor_Access_Provider *ap;
        EINA_LIST_FREE (sd->anchor_access_providers, ap)
          {
             free(ap);
          }
     }
   //

   evas_event_thaw(evas_object_evas_get(obj));
   evas_event_thaw_eval(evas_object_evas_get(obj));

   ELM_WIDGET_CLASS(_elm_label_parent_sc)->base.del(obj);
}

static Eina_Bool
_elm_label_smart_activate(Evas_Object *obj, Elm_Activate act)
{
   Elm_Access_Info *ac;

   if (!_elm_config->access_mode) return EINA_FALSE;

   if (act != ELM_ACTIVATE_DEFAULT) return EINA_FALSE;

   ac = _elm_access_object_get(obj);
   if (!ac) return EINA_FALSE;
   if (!ac->activate) return EINA_FALSE;

   ac->activate(ac->activate_data, obj, NULL);

   return EINA_TRUE;
}

static void
_elm_label_smart_set_user(Elm_Label_Smart_Class *sc)
{
   ELM_WIDGET_CLASS(sc)->base.add = _elm_label_smart_add;
   ELM_WIDGET_CLASS(sc)->base.del = _elm_label_smart_del;

   /* not a 'focus chain manager' */
   ELM_WIDGET_CLASS(sc)->focus_next = NULL;
   ELM_WIDGET_CLASS(sc)->focus_direction_manager_is = NULL;
   ELM_WIDGET_CLASS(sc)->focus_direction = NULL;
   ELM_WIDGET_CLASS(sc)->activate = _elm_label_smart_activate;

   ELM_WIDGET_CLASS(sc)->theme = _elm_label_smart_theme;
   ELM_WIDGET_CLASS(sc)->translate = _elm_label_smart_translate;

   ELM_LAYOUT_CLASS(sc)->sizing_eval = _elm_label_smart_sizing_eval;
   ELM_LAYOUT_CLASS(sc)->text_set = _elm_label_smart_text_set;

   ELM_LAYOUT_CLASS(sc)->text_aliases = _text_aliases;
}

EAPI const Elm_Label_Smart_Class *
elm_label_smart_class_get(void)
{
   static Elm_Label_Smart_Class _sc =
     ELM_LABEL_SMART_CLASS_INIT_NAME_VERSION(ELM_LABEL_SMART_NAME);
   static const Elm_Label_Smart_Class *class = NULL;

   if (class)
     return class;

   _elm_label_smart_set(&_sc);
   class = &_sc;

   return class;
}

/////////////////////////////////////////////////////////////////////
// TIZEN_ONLY(20140628): Add elm_label_anchor_access_provider_* APIs and Support anchor access features.
/////////////////////////////////////////////////////////////////////
static const char *
_elm_label_anchor_access_string_get(Evas_Object *obj,
                                    const char *name,
                                    const char *text)
{
   Eina_List *l;
   char *ret;
   Elm_Label_Anchor_Access_Provider *ap;

   ELM_LABEL_DATA_GET(obj, sd);

   EINA_LIST_FOREACH(sd->anchor_access_providers, l, ap)
     {
        ret = ap->func(ap->data, obj, name, text);
        if (ret) return ret;
     }
   return text;
}

static Eina_Bool
_elm_label_anchors_highlight_set(Evas_Object *obj, int pos)
{
   Evas *e = evas_object_evas_get(obj);
   Eina_List *l, *ll;
   Evas_Coord x, y, w, h;
   Elm_Label_Anchor_Data *an;
   Sel *sel;
   int count = 0;

   ELM_LABEL_DATA_GET(obj, sd);

   if (pos < 0)
     {
        sd->anchor_highlight_pos = -1;
        return EINA_FALSE;
     }
   sd->anchor_highlight_pos = pos;
   evas_object_geometry_get(sd->tb, &x, &y, &w, &h);

   EINA_LIST_FOREACH(sd->anchors, l, an)
     {
        if (!an->item)
          {
             Evas_Coord sx, sy, ex, ey;
             char *text, *last_text;
             const char *access_string;

             if (count != pos)
               {
                  count++;
                  continue;
               }

             sx = w;
             sy = h;
             ex = 0;
             ey = 0;
             text = (char *)evas_textblock_cursor_range_text_get(
                an->start, an->end, EVAS_TEXTBLOCK_TEXT_PLAIN);

             if (!(sd->anchor_highlight_rect))
               {
                  Evas_Object *rect = edje_object_add(e);
                  _elm_theme_object_set(obj, rect, "access", "base", "default");
                  evas_object_smart_member_add(rect, obj);
                  evas_object_repeat_events_set(rect, EINA_TRUE);
                  sd->anchor_highlight_rect = rect;
               }
             else
               {
                  evas_object_data_del(sd->anchor_highlight_rect, "anchor_name");
                  last_text = evas_object_data_del(sd->anchor_highlight_rect, "anchor_text");
                  if (last_text) free(last_text);
               }

             EINA_LIST_FOREACH(an->sel, ll, sel)
               {
                  if (sx > sel->rect.x) sx = sel->rect.x;
                  if (sy > sel->rect.y) sy = sel->rect.y;
                  if (ex < sel->rect.x + sel->rect.w) ex = sel->rect.x + sel->rect.w;
                  if (ey < sel->rect.y + sel->rect.h) ey = sel->rect.y + sel->rect.h;
               }
             elm_widget_show_region_set(obj, sx, sy, ex - sx, ey - sy, EINA_FALSE);

             evas_object_geometry_get(sd->tb, &x, &y, &w, &h);
             evas_object_resize(sd->anchor_highlight_rect, ex - sx, ey - sy);
             evas_object_move(sd->anchor_highlight_rect, sx + x, sy + y);
             evas_object_show(sd->anchor_highlight_rect);
             evas_object_data_set(sd->anchor_highlight_rect, "anchor_name", an->name);
             evas_object_data_set(sd->anchor_highlight_rect, "anchor_text", text);

             access_string = _elm_label_anchor_access_string_get(obj, an->name, text);
             _elm_access_say(obj, access_string, EINA_FALSE);
             return EINA_TRUE;
          }
     }
   sd->anchor_highlight_pos = -1;
   return EINA_FALSE;
}

static void
_elm_label_anchor_highlight_activate(Evas_Object *obj, int pos)
{
   Elm_Label_Anchor_Info ei;

   ELM_LABEL_DATA_GET(obj, sd);

   if (pos != sd->anchor_highlight_pos)
     _elm_label_anchors_highlight_set(obj, pos);

   ei.name = evas_object_data_get(sd->anchor_highlight_rect, "anchor_name");

   if (!elm_widget_disabled_get(obj))
     evas_object_smart_callback_call(obj, SIG_ANCHOR_CLICKED, &ei);
}

static Eina_Bool
_elm_label_access_action_cb(void *data, Evas_Object *obj, Elm_Access_Action_Info *action_info)
{
   if (!action_info) return EINA_FALSE;
   ELM_LABEL_DATA_GET(data, sd);

   switch (action_info->action_type)
     {
      case ELM_ACCESS_ACTION_ACTIVATE:
        if (!elm_widget_disabled_get(data) &&
            !evas_object_freeze_events_get(data))
          {
             if (sd->anchor_highlight_pos > -1)
               _elm_label_anchor_highlight_activate(data, sd->anchor_highlight_pos);

             evas_object_smart_callback_call(data, SIG_CLICKED, NULL);
          }
        break;

      case ELM_ACCESS_ACTION_HIGHLIGHT_NEXT:
        if (!elm_widget_disabled_get(data) &&
            !evas_object_freeze_events_get(data))
          {
             if (_elm_label_anchors_highlight_set(data, sd->anchor_highlight_pos + 1))
               {
                  _elm_access_sound_play(ELM_ACCESS_SOUND_HIGHLIGHT);
                  return EINA_TRUE;
               }
             _elm_label_anchors_highlight_clear(data);
             return EINA_FALSE;
          }
        break;

      case ELM_ACCESS_ACTION_HIGHLIGHT_PREV:
        if (!elm_widget_disabled_get(data) &&
            !evas_object_freeze_events_get(data))
          {
             if (sd->anchor_highlight_pos == -1)
               return EINA_FALSE;
             if (_elm_label_anchors_highlight_set(data, sd->anchor_highlight_pos - 1))
               {
                  _elm_access_sound_play(ELM_ACCESS_SOUND_HIGHLIGHT);
                  return EINA_TRUE;
               }
             _elm_label_anchors_highlight_clear(data);
             elm_access_highlight_set(obj);
             return EINA_TRUE;
          }
        break;

      case ELM_ACCESS_ACTION_UNHIGHLIGHT:
        _elm_label_anchors_highlight_clear(data);
        return EINA_FALSE;

      default:
        return EINA_FALSE;
        break;
     }
   return EINA_TRUE;
}
/////////////////////////////////////////////

EAPI Evas_Object *
elm_label_add(Evas_Object *parent)
{
   Evas_Object *obj;

   EINA_SAFETY_ON_NULL_RETURN_VAL(parent, NULL);

   obj = elm_widget_add(_elm_label_smart_class_new(), parent);
   if (!obj) return NULL;

   if (!elm_widget_sub_object_add(parent, obj))
     ERR("could not add %p as sub object of %p", obj, parent);

   ELM_LABEL_DATA_GET(obj, sd);

   sd->linewrap = ELM_WRAP_NONE;
   sd->wrap_w = -1;
   sd->slide_duration = 0;
   // TIZEN_ONLY(20140606): Update anchor when textblock geometry is changed.
   sd->tx = 0;
   sd->ty = 0;
   sd->tw = 0;
   sd->th = 0;
   //
   // TIZEN_ONLY(20140623): Optimize performance for anchor, item.
   sd->anchors_force_update = EINA_FALSE;
   //
   // TIZEN_ONLY(20140628): Add elm_label_anchor_access_provider_* APIs and Support anchor access features.
   sd->anchor_highlight_pos = -1;
   sd->anchor_highlight_rect = NULL;
   sd->anchor_access_providers = NULL;
   sd->current_anchor_idx = 0;
   sd->forward_rendering = EINA_FALSE;
   sd->forward_batch_processed = 0;
   sd->batch_anchor_update_handle = NULL;
   //

   sd->format = eina_stringshare_add("");
   _label_format_set(ELM_WIDGET_DATA(sd)->resize_obj, sd->format);

   evas_object_event_callback_add
     (ELM_WIDGET_DATA(sd)->resize_obj, EVAS_CALLBACK_RESIZE,
     _on_label_resize, obj);


   edje_object_signal_callback_add(ELM_WIDGET_DATA(sd)->resize_obj,
                                   "elm,state,slide,end", "", _on_slide_end,
                                   obj);

   edje_object_signal_callback_add(ELM_WIDGET_DATA(sd)->resize_obj,
                                   "elm,state,no,sliding", "", _on_no_sliding,
                                   obj);
   /* access */
   _elm_access_object_register(obj, ELM_WIDGET_DATA(sd)->resize_obj);
   _elm_access_text_set
     (_elm_access_object_get(obj), ELM_ACCESS_TYPE, E_("Label"));
   _elm_access_callback_set
     (_elm_access_object_get(obj), ELM_ACCESS_INFO, _access_info_cb, NULL);
   // TIZEN_ONLY(20140628): Add elm_label_anchor_access_provider_* APIs and Support anchor access features.
   elm_access_action_cb_set(obj, ELM_ACCESS_ACTION_ACTIVATE,
                            _elm_label_access_action_cb, obj);
   elm_access_action_cb_set(obj, ELM_ACCESS_ACTION_HIGHLIGHT_NEXT,
                            _elm_label_access_action_cb, obj);
   elm_access_action_cb_set(obj, ELM_ACCESS_ACTION_HIGHLIGHT_PREV,
                            _elm_label_access_action_cb, obj);
   elm_access_action_cb_set(obj, ELM_ACCESS_ACTION_UNHIGHLIGHT,
                            _elm_label_access_action_cb, obj);
   //

   elm_layout_theme_set(obj, "label", "base", elm_widget_style_get(obj));
   elm_layout_text_set(obj, NULL, "<br>");
   elm_layout_sizing_eval(obj);

   // TIZEN_ONLY(140520): Improve performance when *_smart_theme is called.
   sd->tb = (Evas_Object *)edje_object_part_object_get(ELM_WIDGET_DATA(sd)->resize_obj, "elm.text");
   //
   // TIZEN_ONLY(140626): Add callbacks to textblock for updating anchor, label
   evas_object_event_callback_add(sd->tb, EVAS_CALLBACK_SHOW, _label_anchors_show, obj);
   evas_object_event_callback_add(sd->tb, EVAS_CALLBACK_HIDE, _label_anchors_hide, obj);
   evas_object_event_callback_add(sd->tb, EVAS_CALLBACK_MOVE, _label_anchors_move, obj);
   //
   // TIZEN_ONLY(140701): Prevent to append duplicated callbacks
   evas_object_event_callback_add(sd->tb, EVAS_CALLBACK_MOUSE_DOWN, _label_mouse_down_cb, obj);
   evas_object_event_callback_add(sd->tb, EVAS_CALLBACK_MOUSE_UP, _label_mouse_up_cb, obj);
   evas_object_event_callback_add(sd->tb, EVAS_CALLBACK_MOUSE_MOVE, _label_mouse_move_cb, obj);
   //
   //Tizen Only: This should be removed when eo is applied.
   ELM_WIDGET_DATA_GET(obj, wsd);
   wsd->on_create = EINA_FALSE;

   return obj;
}

EAPI void
elm_label_line_wrap_set(Evas_Object *obj,
                        Elm_Wrap_Type wrap)
{
   const char *wrap_str, *text;
   int len;

   ELM_LABEL_CHECK(obj);
   ELM_LABEL_DATA_GET(obj, sd);

   if (sd->linewrap == wrap) return;

   sd->linewrap = wrap;
   text = elm_layout_text_get(obj, NULL);
   if (!text) return;

   len = strlen(text);
   if (len <= 0) return;

   switch (wrap)
     {
      case ELM_WRAP_CHAR:
        wrap_str = "char";
        break;

      case ELM_WRAP_WORD:
        wrap_str = "word";
        break;

      case ELM_WRAP_MIXED:
        wrap_str = "mixed";
        break;

      default:
        wrap_str = "none";
        break;
     }

   if (_stringshare_key_value_replace(&sd->format, "wrap", wrap_str, 0) == 0)
     {
        sd->lastw = -1;
        _label_format_set(ELM_WIDGET_DATA(sd)->resize_obj, sd->format);
        elm_layout_sizing_eval(obj);
     }
}

EAPI Elm_Wrap_Type
elm_label_line_wrap_get(const Evas_Object *obj)
{
   ELM_LABEL_CHECK(obj) EINA_FALSE;
   ELM_LABEL_DATA_GET(obj, sd);

   return sd->linewrap;
}

EAPI void
elm_label_wrap_width_set(Evas_Object *obj,
                         Evas_Coord w)
{
   ELM_LABEL_CHECK(obj);
   ELM_LABEL_DATA_GET(obj, sd);

   if (w < 0) w = 0;

   if (sd->wrap_w == w) return;

   if (sd->ellipsis)
     _label_format_set(ELM_WIDGET_DATA(sd)->resize_obj, sd->format);
   sd->wrap_w = w;
   sd->lastw = -1;

   elm_layout_sizing_eval(obj);
}

EAPI Evas_Coord
elm_label_wrap_width_get(const Evas_Object *obj)
{
   ELM_LABEL_CHECK(obj) 0;
   ELM_LABEL_DATA_GET(obj, sd);

   return sd->wrap_w;
}

EAPI void
elm_label_ellipsis_set(Evas_Object *obj,
                       Eina_Bool ellipsis)
{
   Eina_Strbuf *fontbuf = NULL;
   int len, removeflag = 0;
   const char *text;

   ELM_LABEL_CHECK(obj);
   ELM_LABEL_DATA_GET(obj, sd);

   if (sd->ellipsis == ellipsis) return;
   sd->ellipsis = ellipsis;

   text = elm_layout_text_get(obj, NULL);
   if (!text) return;

   len = strlen(text);
   if (len <= 0) return;

   if (ellipsis == EINA_FALSE) removeflag = 1;  // remove fontsize tag

   fontbuf = eina_strbuf_new();
   eina_strbuf_append_printf(fontbuf, "%f", 1.0);

   if (_stringshare_key_value_replace
         (&sd->format, "ellipsis", eina_strbuf_string_get
           (fontbuf), removeflag) == 0)
     {
        _label_format_set(ELM_WIDGET_DATA(sd)->resize_obj, sd->format);
        elm_layout_sizing_eval(obj);
     }
   eina_strbuf_free(fontbuf);
}

EAPI Eina_Bool
elm_label_ellipsis_get(const Evas_Object *obj)
{
   ELM_LABEL_CHECK(obj) EINA_FALSE;
   ELM_LABEL_DATA_GET(obj, sd);

   return sd->ellipsis;
}

EAPI void
elm_label_slide_mode_set(Evas_Object *obj, Elm_Label_Slide_Mode mode)
{
   ELM_LABEL_CHECK(obj);
   ELM_LABEL_DATA_GET(obj, sd);
   sd->slide_mode = mode;
}

EAPI Elm_Label_Slide_Mode
elm_label_slide_mode_get(const Evas_Object *obj)
{
   ELM_LABEL_CHECK(obj) ELM_LABEL_SLIDE_MODE_NONE;
   ELM_LABEL_DATA_GET(obj, sd);
   return sd->slide_mode;
}

EINA_DEPRECATED EAPI void
elm_label_slide_set(Evas_Object *obj,
                    Eina_Bool slide)
{
   if (slide)
     elm_label_slide_mode_set(obj, ELM_LABEL_SLIDE_MODE_ALWAYS);
   else
     elm_label_slide_mode_set(obj, ELM_LABEL_SLIDE_MODE_NONE);
}

EINA_DEPRECATED EAPI Eina_Bool
elm_label_slide_get(const Evas_Object *obj)
{
   Eina_Bool ret = EINA_FALSE;
   if (elm_label_slide_mode_get(obj) == ELM_LABEL_SLIDE_MODE_ALWAYS)
     ret = EINA_TRUE;
   return ret;
}

EAPI void
elm_label_slide_go(Evas_Object *obj)
{
   ELM_LABEL_CHECK(obj);
   _label_slide_change(obj);
   elm_layout_sizing_eval(obj);
}

EAPI void
elm_label_slide_duration_set(Evas_Object *obj, double duration)
{
   ELM_LABEL_CHECK(obj);
   ELM_LABEL_DATA_GET(obj, sd);

   sd->slide_duration = duration;
}

EAPI double
elm_label_slide_duration_get(const Evas_Object *obj)
{
   ELM_LABEL_CHECK(obj) 0.0;
   ELM_LABEL_DATA_GET(obj, sd);

   return sd->slide_duration;
}

// TIZEN_ONLY(131028): Support item, anchor formats
EAPI void
elm_label_item_provider_append(Evas_Object *obj,
                               Elm_Label_Item_Provider_Cb func,
                               void *data)
{
   Elm_Label_Item_Provider *ip;

   ELM_LABEL_CHECK(obj);
   ELM_LABEL_DATA_GET(obj, sd);
   EINA_SAFETY_ON_NULL_RETURN(func);

   ip = calloc(1, sizeof(Elm_Label_Item_Provider));
   if (!ip) return;

   ip->func = func;
   ip->data = data;
   sd->item_providers = eina_list_append(sd->item_providers, ip);
}

EAPI void
elm_label_item_provider_prepend(Evas_Object *obj,
                                Elm_Label_Item_Provider_Cb func,
                                void *data)
{
   Elm_Label_Item_Provider *ip;

   ELM_LABEL_CHECK(obj);
   ELM_LABEL_DATA_GET(obj, sd);
   EINA_SAFETY_ON_NULL_RETURN(func);

   ip = calloc(1, sizeof(Elm_Label_Item_Provider));
   if (!ip) return;

   ip->func = func;
   ip->data = data;
   sd->item_providers = eina_list_prepend(sd->item_providers, ip);
}

EAPI void
elm_label_item_provider_remove(Evas_Object *obj,
                               Elm_Label_Item_Provider_Cb func,
                               void *data)
{
   Eina_List *l;
   Elm_Label_Item_Provider *ip;

   ELM_LABEL_CHECK(obj);
   ELM_LABEL_DATA_GET(obj, sd);
   EINA_SAFETY_ON_NULL_RETURN(func);

   EINA_LIST_FOREACH(sd->item_providers, l, ip)
     {
        if ((ip->func == func) && ((!data) || (ip->data == data)))
          {
             sd->item_providers = eina_list_remove_list(sd->item_providers, l);
             free(ip);
             return;
          }
     }
}
/////////////////////////////////////////

/////////////////////////////////////////////////////////////////////
// TIZEN_ONLY(20140628): Add elm_label_anchor_access_provider_* APIs and Support anchor access features.
/////////////////////////////////////////////////////////////////////
EAPI void
elm_label_anchor_access_provider_append(Evas_Object *obj,
                                        Elm_Label_Anchor_Access_Provider_Cb func,
                                        void *data)
{
   Elm_Label_Anchor_Access_Provider *ap;

   ELM_LABEL_CHECK(obj);
   ELM_LABEL_DATA_GET(obj, sd);
   EINA_SAFETY_ON_NULL_RETURN(func);

   ap = calloc(1, sizeof(Elm_Label_Anchor_Access_Provider));
   if (!ap) return;

   ap->func = func;
   ap->data = data;
   sd->anchor_access_providers = eina_list_append(sd->anchor_access_providers, ap);
}

EAPI void
elm_label_anchor_access_provider_prepend(Evas_Object *obj,
                                         Elm_Label_Anchor_Access_Provider_Cb func,
                                         void *data)
{
   Elm_Label_Anchor_Access_Provider *ap;

   ELM_LABEL_CHECK(obj);
   ELM_LABEL_DATA_GET(obj, sd);
   EINA_SAFETY_ON_NULL_RETURN(func);

   ap = calloc(1, sizeof(Elm_Label_Anchor_Access_Provider));
   if (!ap) return;

   ap->func = func;
   ap->data = data;
   sd->anchor_access_providers = eina_list_prepend(sd->anchor_access_providers, ap);
}

EAPI void
elm_label_anchor_access_provider_remove(Evas_Object *obj,
                                        Elm_Label_Anchor_Access_Provider_Cb func,
                                        void *data)
{
   Eina_List *l;
   Elm_Label_Anchor_Access_Provider *ap;

   ELM_LABEL_CHECK(obj);
   ELM_LABEL_DATA_GET(obj, sd);
   EINA_SAFETY_ON_NULL_RETURN(func);

   EINA_LIST_FOREACH(sd->anchor_access_providers, l, ap)
     {
        if ((ap->func == func) && ((!data) || (ap->data == data)))
          {
             sd->anchor_access_providers = eina_list_remove_list(sd->anchor_access_providers, l);
             free(ap);
             return;
          }
     }
}
/////////////////////////////////////////////////////////////////////
