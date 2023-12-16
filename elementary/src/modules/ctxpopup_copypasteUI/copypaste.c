#include <Elementary.h>
#include "elm_module_priv.h"
#include "elm_priv.h"

#if HAVE_APPSVC
#include <appsvc/appsvc.h>
#endif

#include "cbhm_helper.h"

#define S_TRANSLATE dgettext("elementary", "IDS_MSG_HEADER_TRANSLATE")

#define CP_ICON_THEME_SET(ITEM) elm_widget_theme_object_set(ext_mod->popup, \
                                                           icon,"copypaste", ITEM, "default")

Elm_Entry_Extension_data *ext_mod;
static int _mod_hook_count = 0;
static Evas_Coord _previous_pressed_point_x = -1;
static Evas_Coord _previous_pressed_point_y = -1;
static int _copy_paste_move_threshold = 15;

typedef struct _Elm_Entry_Context_Menu_Item Elm_Entry_Context_Menu_Item;

struct _Elm_Entry_Context_Menu_Item
{
   Evas_Object *obj;
   const char *label;
   const char *icon_file;
   const char *icon_group;
   Elm_Icon_Type icon_type;
   Evas_Smart_Cb func;
   void *data;
};

static void _entry_del_cb(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _entry_hide_cb(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _entry_move_cb(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _entry_mouse_down_cb(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _entry_mouse_move_cb(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _entry_mouse_up_cb(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _ctxpopup_hide(Evas_Object *popup);
static void _ctxpopup_dismissed_cb(void *data, Evas_Object *obj, void *event_info);
void obj_longpress(Evas_Object *obj);

#if HAVE_APPSVC
static char *
_remove_tags(const char *str)
{
   char *ret;
   if (!str)
     return NULL;

   Eina_Strbuf *buf = eina_strbuf_new();
   if (!buf)
     return NULL;

   if (!eina_strbuf_append(buf, str))
     {
        eina_strbuf_free(buf);
        return NULL;
     }

   eina_strbuf_replace_all(buf, "<br>", " ");
   eina_strbuf_replace_all(buf, "<br/>", " ");
   eina_strbuf_replace_all(buf, "<ps>", " ");
   eina_strbuf_replace_all(buf, "<ps/>", " ");

   while (EINA_TRUE)
     {
        const char *temp = eina_strbuf_string_get(buf);

        char *startTag = NULL;
        char *endTag = NULL;

        startTag = strstr(temp, "<");
        if (startTag)
          endTag = strstr(startTag, ">");
        else
          break;
        if (!endTag || startTag > endTag)
          break;

        size_t sindex = startTag - temp;
        size_t eindex = endTag - temp + 1;
        if (!eina_strbuf_remove(buf, sindex, eindex))
          break;
     }
   ret = eina_strbuf_string_steal(buf);
   eina_strbuf_free(buf);
   return ret;
}
#endif

static void
_parent_mouse_down_cb(void *data __UNUSED__, Evas *e __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   if ((!ext_mod) || (!ext_mod->popup))
     return;
   ext_mod->popup_clicked = EINA_TRUE;
   evas_object_hide(ext_mod->popup);
   ext_mod->mouse_up = EINA_FALSE;
   ext_mod->entry_move = EINA_FALSE;
}

static void
_parent_mouse_up_cb(void *data __UNUSED__, Evas *e __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   if ((!ext_mod) || (!ext_mod->popup))
     return;
   if ((ext_mod->popup_clicked) && (ext_mod->popup_showing))
     {
        ext_mod->popup_showing = EINA_FALSE;
        evas_object_event_callback_del(ext_mod->caller, EVAS_CALLBACK_DEL, _entry_del_cb);
        evas_object_event_callback_del(ext_mod->caller, EVAS_CALLBACK_HIDE, _entry_hide_cb);
        evas_object_event_callback_del(ext_mod->caller, EVAS_CALLBACK_MOVE, _entry_move_cb);

        evas_object_event_callback_del(ext_mod->caller, EVAS_CALLBACK_MOUSE_DOWN,
                                       _entry_mouse_down_cb);
        evas_object_event_callback_del(ext_mod->caller, EVAS_CALLBACK_MOUSE_MOVE,
                                       _entry_mouse_move_cb);
        evas_object_event_callback_del(ext_mod->caller, EVAS_CALLBACK_MOUSE_UP,
                                       _entry_mouse_up_cb);
        evas_object_event_callback_del(ext_mod->ctx_par, EVAS_CALLBACK_MOUSE_DOWN,
                                       _parent_mouse_down_cb);
        evas_object_event_callback_del(ext_mod->ctx_par, EVAS_CALLBACK_MOUSE_UP,
                                       _parent_mouse_up_cb);
        evas_object_smart_callback_del(ext_mod->popup, "dismissed", _ctxpopup_dismissed_cb);
     }
}

static void
_entry_del_cb(void *data __UNUSED__, Evas *e __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   if (ext_mod)
     {
        evas_object_event_callback_del(ext_mod->ctx_par, EVAS_CALLBACK_MOUSE_DOWN,
                                       _parent_mouse_down_cb);
        evas_object_event_callback_del(ext_mod->ctx_par, EVAS_CALLBACK_MOUSE_UP,
                                       _parent_mouse_up_cb);
        if (ext_mod->popup)
          {
             _ctxpopup_hide(ext_mod->popup);
             evas_object_del(ext_mod->popup);
             ext_mod->popup = NULL;
          }
     }
}

static void
_entry_hide_cb(void *data, Evas *e __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   evas_object_hide(data);
}

static void
_entry_mouse_down_cb(void *data __UNUSED__, Evas *e __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   if ((!ext_mod) || (!ext_mod->popup))
     return;
   ext_mod->mouse_up = EINA_FALSE;
   ext_mod->mouse_move = EINA_FALSE;
   ext_mod->mouse_down = EINA_TRUE;

   Evas_Event_Mouse_Down *ev = event_info;

   _previous_pressed_point_x = ev->canvas.x;
   _previous_pressed_point_y = ev->canvas.y;
}

static Eina_Bool
_ctx_show(void *data)
{
   ext_mod->show_timer = NULL;
   if (!data) return ECORE_CALLBACK_CANCEL;
   ext_mod->entry_move = EINA_FALSE;
   if (ext_mod->popup_showing)
     {
        obj_longpress(data);
     }
   return ECORE_CALLBACK_CANCEL;
}

static void
_entry_mouse_move_cb(void *data __UNUSED__, Evas *e __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   if ((!ext_mod) || (!ext_mod->popup))
     return;

   if (!ext_mod->mouse_down)
     return;

   Evas_Event_Mouse_Move *ev = event_info;
   if ((abs(_previous_pressed_point_x - ev->cur.canvas.x) >= _copy_paste_move_threshold)||(abs(_previous_pressed_point_y - ev->cur.canvas.y) >= _copy_paste_move_threshold))
     {
        ext_mod->popup_clicked = EINA_FALSE;
        ext_mod->mouse_move = EINA_TRUE;
     }
   else
     {
        return;
     }

   evas_object_hide(ext_mod->popup);
   evas_object_smart_callback_del(ext_mod->popup, "dismissed", _ctxpopup_dismissed_cb);
   if (ext_mod->show_timer)
     {
        ecore_timer_del(ext_mod->show_timer);
        ext_mod->show_timer = NULL;
     }
}

static void
_entry_mouse_up_cb(void *data  __UNUSED__, Evas *e __UNUSED__, Evas_Object *obj, void *event_info __UNUSED__)
{
   _previous_pressed_point_x = -1;
   _previous_pressed_point_y = -1;

   if (!ext_mod)
     return;
   if ((ext_mod->mouse_down && ext_mod->entry_move) || (ext_mod->mouse_down && ext_mod->mouse_move))
     {
        evas_object_smart_callback_del(ext_mod->popup, "dismissed", _ctxpopup_dismissed_cb);
        if (ext_mod->show_timer)
          {
             ecore_timer_del(ext_mod->show_timer);
             ext_mod->show_timer = NULL;
          }

        if (ext_mod->popup && !evas_object_visible_get(ext_mod->popup))
          {
             ext_mod->show_timer = ecore_timer_add(0.1, _ctx_show, obj);
          }
     }
   ext_mod->mouse_up = EINA_TRUE;
   ext_mod->mouse_move = EINA_FALSE;
   ext_mod->mouse_down = EINA_FALSE;
}

static void
_entry_move_cb(void *data __UNUSED__, Evas *e __UNUSED__, Evas_Object *obj, void *event_info __UNUSED__)
{
   if (!ext_mod)
     return;
   ext_mod->entry_move = EINA_TRUE;
   ext_mod->popup_clicked = EINA_FALSE;
   evas_object_hide(ext_mod->popup);
   evas_object_smart_callback_del(ext_mod->popup, "dismissed", _ctxpopup_dismissed_cb);
   if (ext_mod->show_timer) ecore_timer_del(ext_mod->show_timer);
   ext_mod->show_timer = ecore_timer_add(0.1, _ctx_show, obj);
}

static void
_ctxpopup_hide(Evas_Object *popup)
{
   if (!ext_mod->popup_showing) return;
   ext_mod->popup_showing = EINA_FALSE;
   evas_object_hide(popup);
   evas_object_event_callback_del(ext_mod->caller, EVAS_CALLBACK_DEL, _entry_del_cb);
   evas_object_event_callback_del(ext_mod->caller, EVAS_CALLBACK_HIDE, _entry_hide_cb);
   evas_object_event_callback_del(ext_mod->caller, EVAS_CALLBACK_MOVE, _entry_move_cb);
   if (_elm_config->access_mode && ext_mod->caller)
     _elm_access_highlight_set(ext_mod->caller, EINA_TRUE);
}

static Eina_Bool
_in_viewport_check()
{
   if (!ext_mod) return EINA_FALSE;

   Evas_Coord sx, sy, sw, sh;
   Eina_Rectangle vp;

   /*update*/
   if (ext_mod->caller)
     elm_entry_extension_module_data_get(ext_mod->caller, ext_mod);

   vp.x = ext_mod->viewport_rect.x;
   vp.y = ext_mod->viewport_rect.y;
   vp.w = ext_mod->viewport_rect.w;
   vp.h = ext_mod->viewport_rect.h;

   if (edje_object_part_text_selection_geometry_get(ext_mod->ent, "elm.text", &sx, &sy, &sw, &sh))
     {
        if ((ext_mod->viewport_rect.x != -1) || (ext_mod->viewport_rect.y != -1) ||
            (ext_mod->viewport_rect.w != -1) || (ext_mod->viewport_rect.h != -1))
          {
             Eina_Rectangle sel;

             sel.x = sx;
             sel.y = sy;
             sel.w = sw;
             sel.h = sh;
             if (eina_rectangle_intersection(&sel, &vp))
               {
                  return EINA_TRUE;
               }
             return EINA_FALSE;
          }
        return EINA_TRUE;
     }
   else
     {
        if ((ext_mod->viewport_rect.x != -1) || (ext_mod->viewport_rect.y != -1) ||
            (ext_mod->viewport_rect.w != -1) || (ext_mod->viewport_rect.h != -1))
          {
             Evas_Coord ex, ey;
             Evas_Coord cx, cy, cw, ch;
             Eina_Rectangle cur;

             evas_object_geometry_get(ext_mod->ent, &ex, &ey, NULL, NULL);
             edje_object_part_text_cursor_geometry_get(ext_mod->ent, "elm.text",
                                                       &cx, &cy, &cw, &ch);
             cx = ex + cx;
             cy = ey + cy;
             cur.x = cx;
             cur.y = cy;
             cur.w = cw;
             cur.h = ch;
             if (eina_rectangle_intersection(&cur, &vp))
               {
                  return EINA_TRUE;
               }
             return EINA_FALSE;
          }
        return EINA_TRUE;
     }
   return EINA_FALSE;
}

static void
_ctxpopup_position(Evas_Object *obj __UNUSED__)
{
   if(!ext_mod) return;

   Evas_Coord ex, ey;
   Evas_Coord sx, sy, sw, sh;
   Evas_Coord x, y, w;
   int gap = 13; //in GUI
   Eina_Bool avai;

   evas_object_geometry_get(ext_mod->ent, &ex, &ey, NULL, NULL);
   elm_ctxpopup_direction_priority_set(ext_mod->popup, ELM_CTXPOPUP_DIRECTION_UP,
                                       ELM_CTXPOPUP_DIRECTION_DOWN, ELM_CTXPOPUP_DIRECTION_LEFT,
                                       ELM_CTXPOPUP_DIRECTION_RIGHT);
   if (!edje_object_part_text_selection_geometry_get(ext_mod->ent, "elm.text", &sx, &sy, &sw, &sh))
     { //cannot get selection shape
        Evas_Coord cx, cy, cw, ch;
        Evas_Coord chx, chy, chw, chh;
        Eina_Bool ch_visible = EINA_FALSE;
        Eina_Bool need_update = EINA_FALSE;
        Eina_Bool up_avai, down_avai;

        edje_object_part_text_cursor_geometry_get(ext_mod->ent, "elm.text",
                                                  &cx, &cy, &cw, &ch);
        x = ex + cx;
        y = ey + cy;
        evas_object_move(ext_mod->popup, x, y);
        avai = elm_ctxpopup_direction_available_get
           (ext_mod->popup, ELM_CTXPOPUP_DIRECTION_DOWN);
        if (avai == EINA_TRUE)
          {
             elm_ctxpopup_direction_priority_set(ext_mod->popup,
                                                 ELM_CTXPOPUP_DIRECTION_DOWN,
                                                 ELM_CTXPOPUP_DIRECTION_UP,
                                                 ELM_CTXPOPUP_DIRECTION_LEFT,
                                                 ELM_CTXPOPUP_DIRECTION_RIGHT);
             x = ex + cx;
             y = ey + cy + ch;
             evas_object_move(ext_mod->popup, x, y);
             avai = elm_ctxpopup_direction_available_get
                (ext_mod->popup, ELM_CTXPOPUP_DIRECTION_UP);
             if (avai == EINA_TRUE)
               need_update = EINA_TRUE;
          }

        //limit ctx in viewport
        if (ext_mod->viewport_rect.x != -1 || ext_mod->viewport_rect.y != -1
            || ext_mod->viewport_rect.w != -1 || ext_mod->viewport_rect.h != -1)
          {
             if (ext_mod->viewport_rect.x > x)
               x = ext_mod->viewport_rect.x;
             else if (x > ext_mod->viewport_rect.x + ext_mod->viewport_rect.w)
               x = ext_mod->viewport_rect.x + ext_mod->viewport_rect.w;

             if (ext_mod->viewport_rect.y > y)
               y = ext_mod->viewport_rect.y;
             else if (y > ext_mod->viewport_rect.y + ext_mod->viewport_rect.h)
               y = ext_mod->viewport_rect.y + ext_mod->viewport_rect.h;
          }

        evas_object_move(ext_mod->popup, x, y);

        ch_visible = edje_object_part_text_cursor_handler_geometry_get
                        (ext_mod->ent, "elm.text", &chx, &chy, &chw, &chh);
        up_avai = elm_ctxpopup_direction_available_get
           (ext_mod->popup, ELM_CTXPOPUP_DIRECTION_UP);
        down_avai = elm_ctxpopup_direction_available_get
           (ext_mod->popup, ELM_CTXPOPUP_DIRECTION_DOWN);
        //move to above or below cursor handler
        if (ch_visible)
          {
             if ((chy < cy) && (up_avai == EINA_TRUE))
               {
                  y = ey + chy;
                  evas_object_move(ext_mod->popup, x, y);
                  up_avai = elm_ctxpopup_direction_available_get
                     (ext_mod->popup, ELM_CTXPOPUP_DIRECTION_UP);
                  if (up_avai == EINA_FALSE)
                    need_update = EINA_TRUE;
               }
             else if ((chy > cy) && (down_avai == EINA_TRUE))
               {
                  y = ey + chy + chh;
                  evas_object_move(ext_mod->popup, x, y);
                  down_avai = elm_ctxpopup_direction_available_get
                     (ext_mod->popup, ELM_CTXPOPUP_DIRECTION_DOWN);
                  if (down_avai == EINA_FALSE)
                    need_update = EINA_TRUE;
               }
          }
        if (((up_avai == EINA_FALSE) && (down_avai == EINA_FALSE)) || need_update)
          {
             y = ey + cy + ch / 2;
             elm_ctxpopup_direction_priority_set(ext_mod->popup,
                                                 ELM_CTXPOPUP_DIRECTION_LEFT,
                                                 ELM_CTXPOPUP_DIRECTION_RIGHT,
                                                 ELM_CTXPOPUP_DIRECTION_UP,
                                                 ELM_CTXPOPUP_DIRECTION_DOWN);
             evas_object_move(ext_mod->popup, x, y);
          }
     }
   else //get selection shape
     {
        Evas_Coord shx, shy, shw, shh;
        Evas_Coord ehx, ehy, ehw, ehh;
        /* Currently, we cannot get size of ctxpopup, we must set hard value for threshold.
           After fixing ctxpopup, we can get correctly threshold */
        Evas_Coord threshold = 300;

        edje_object_part_text_selection_handler_geometry_get(ext_mod->ent, "elm.text",
                                                             EDJE_SELECTION_HANDLER_START,
                                                             &shx, &shy, &shw, &shh);
        edje_object_part_text_selection_handler_geometry_get(ext_mod->ent, "elm.text",
                                                             EDJE_SELECTION_HANDLER_END,
                                                             &ehx, &ehy, &ehw, &ehh);
        if (ext_mod->viewport_rect.x != -1 || ext_mod->viewport_rect.y != -1
            || ext_mod->viewport_rect.w != -1 || ext_mod->viewport_rect.h != -1)
          {
             Evas_Coord vx, vy, vw, vh, x2, y2;
             x2 = sx + sw;
             if ((ehh > 0) && (ey + ehy + ehh > sy + sh))
               {
                  //y2 = ey + ehy + ehh;
                  y2 = ey + ehy + gap;
               }
             else
               y2 = sy + sh;
             vx = ext_mod->viewport_rect.x;
             vy = ext_mod->viewport_rect.y;
             vw = ext_mod->viewport_rect.w;
             vh = ext_mod->viewport_rect.h;

             //limit ctx in viewport
             x = sx;
             if (sx < vx) x = vx;
             if (sy < vy)
               {
                  y = vy; //case: start of selection is behind the viewport
               }
             else
               {
                  if ((sx >= vx) && (ey + shy < sy)) //case: start handler is upside
                    {
                       y = ey + shy;
                       w = x2 - x;
                       evas_object_move(ext_mod->popup, x + w/2, y);
                       avai = elm_ctxpopup_direction_available_get
                          (ext_mod->popup, ELM_CTXPOPUP_DIRECTION_UP);
                       if (avai == EINA_FALSE)
                         {
                            y = sy - gap;
                         }
                    }
                  else if ((sx + sw <= vx + vw) && (ey + ehy < sy))
                    {
                       y = ey + ehy;
                    }
                  else
                    y = sy;
               }
             if (x2 > vx + vw) x2 = vx + vw;
             if (y2 > vy + vh) y2 = vy + vh;
             w = x2 - x;
             evas_object_move(ext_mod->popup, x + w/2, y);
             avai = elm_ctxpopup_direction_available_get
                (ext_mod->popup, ELM_CTXPOPUP_DIRECTION_UP);
             if (avai == EINA_TRUE)
               return;
          }
        else //get selection & cannot get viewport
          {
             Evas_Coord ww, wh, x2, y2;
             x2 = sx + sw;
             if (ey + ehy + ehh > sy + sh)
               y2 = ey + ehy + ehh; //end handler is downside
             else
               y2 = sy + sh;

             ecore_x_window_size_get(ecore_x_window_root_first_get(), &ww, &wh);

             x = sx;
             if (sx < 0) x = 0;
             if (sy < 0)
               {
                  y = 0; //start of selection is negative
               }
             else
               {
                  if (ey + shy < sy) //start handler is upside
                    {
                       y = ey + shy;
                       w = x2 - x;
                       evas_object_move(ext_mod->popup, x + w/2, y);
                       avai = elm_ctxpopup_direction_available_get
                          (ext_mod->popup, ELM_CTXPOPUP_DIRECTION_UP);
                       if (avai == EINA_FALSE)
                         {
                            y = sy - gap;
                         }
                    }
                  else
                    y = sy;
               }
             if (x2 > ww) x2 = ww;
             if (y2 > wh) y2 = wh;
             w = x2 - x;
          }
        x = x + (w / 2);
        evas_object_move(ext_mod->popup, x, y);

        avai = elm_ctxpopup_direction_available_get
           (ext_mod->popup, ELM_CTXPOPUP_DIRECTION_UP);
        if (avai == EINA_FALSE)
          {
             Eina_Rectangle vp;
             Eina_Rectangle sel; //selection area which is not covered by start,end sel handlers
             Evas_Coord cry;

             elm_ctxpopup_direction_priority_set(ext_mod->popup, ELM_CTXPOPUP_DIRECTION_DOWN,
                                                 ELM_CTXPOPUP_DIRECTION_UP,
                                                 ELM_CTXPOPUP_DIRECTION_LEFT,
                                                 ELM_CTXPOPUP_DIRECTION_RIGHT);
             vp.x = ext_mod->viewport_rect.x;
             vp.y = ext_mod->viewport_rect.y;
             vp.w = ext_mod->viewport_rect.w;
             vp.h = ext_mod->viewport_rect.h;
             sel.x = sx;
             if (ey + shy + shh < sy + sh)
               sel.y = sy > ey + shy + shh ? sy : ey + shy + shh;
             else
               sel.y = sy;
             sel.w = sw;
             cry = sy + sh < ey + ehy ? sy + sh : ey + ehy;
             sel.h = cry > sel.y ? cry - sel.y : sel.y - cry;
             if ((eina_rectangle_intersection(&sel, &vp)) && (sel.h > threshold))
               {
                  elm_ctxpopup_direction_priority_set(ext_mod->popup, ELM_CTXPOPUP_DIRECTION_UP,
                                                      ELM_CTXPOPUP_DIRECTION_DOWN,
                                                      ELM_CTXPOPUP_DIRECTION_LEFT,
                                                      ELM_CTXPOPUP_DIRECTION_RIGHT);
                  evas_object_move(ext_mod->popup, x, sel.y + sel.h/2);
                  avai = elm_ctxpopup_direction_available_get
                     (ext_mod->popup, ELM_CTXPOPUP_DIRECTION_UP);
                  if (avai == EINA_TRUE)
                    return;
               }

             y = sel.y + sel.h;
             if ((y < ey + ehy + ehh) && (ey + ehy < vp.y + vp.h)) //end handler is downside
               {
                 if (ex == 0)
                    y = ey + ehy + ehh;
                 else
                    y = (ey + ehy)/2 + ehh;
                  evas_object_move(ext_mod->popup, x, y);
                  avai = elm_ctxpopup_direction_available_get
                     (ext_mod->popup, ELM_CTXPOPUP_DIRECTION_DOWN);
                  if (avai == EINA_TRUE)
                    return;
                  y = sy + sh + gap;
                  evas_object_move(ext_mod->popup, x, y);
                  avai = elm_ctxpopup_direction_available_get
                     (ext_mod->popup, ELM_CTXPOPUP_DIRECTION_DOWN);
                  if (avai == EINA_TRUE)
                    return;
               }
             if (ex == 0)
                y = ey + ehy + ehh;
             else
                y = (ey + ehy)/2 + ehh;
             evas_object_move(ext_mod->popup, x, y);
             avai = elm_ctxpopup_direction_available_get
                (ext_mod->popup, ELM_CTXPOPUP_DIRECTION_DOWN);
             if (avai == EINA_TRUE)
               return;

             // not enough space and small viewport (like landscape mode)
             sel.x = sx;
             sel.y = sy;
             sel.w = sw;
             sel.h = sh;
             if (!eina_rectangle_intersection(&sel, &vp))
               return;

             if (ey + shy < sel.y) //start handler is up side
               {
                  y = ey + shy;
                  while (y <= sel.y + ehh/2)
                    {
                       evas_object_move(ext_mod->popup, x, y);
                       avai = elm_ctxpopup_direction_available_get
                          (ext_mod->popup, ELM_CTXPOPUP_DIRECTION_UP);
                       if (avai == EINA_TRUE)
                         return;
                       y += shh/4;
                    }
               }
             else
               {
                  if (ey + ehy + ehh > sel.y + sel.h) //end handler is down side
                    {
                       y = ey + ehy + ehh;
                       while (y > sel.y + sel.h - ehh/2)
                         {
                            evas_object_move(ext_mod->popup, x, y);
                            avai = elm_ctxpopup_direction_available_get
                               (ext_mod->popup, ELM_CTXPOPUP_DIRECTION_DOWN);
                            if (avai == EINA_TRUE)
                              return;
                            y -= ehh/4;
                         }
                    }
                  else
                    {
                       y = sel.y + sel.h;
                       while (y > sel.y + sel.h - ehh/2)
                         {
                            evas_object_move(ext_mod->popup, x, y);
                            avai = elm_ctxpopup_direction_available_get
                               (ext_mod->popup, ELM_CTXPOPUP_DIRECTION_DOWN);
                            if (avai == EINA_TRUE)
                              return;
                            y -= ehh/4;
                         }
                    }
               }

             elm_ctxpopup_direction_priority_set(ext_mod->popup,
                                                 ELM_CTXPOPUP_DIRECTION_LEFT,
                                                 ELM_CTXPOPUP_DIRECTION_RIGHT,
                                                 ELM_CTXPOPUP_DIRECTION_UP,
                                                 ELM_CTXPOPUP_DIRECTION_DOWN);
             x = sel.x;
             y = sel.y + sel.h/2;
             evas_object_move(ext_mod->popup, x, y);
             avai = elm_ctxpopup_direction_available_get
                (ext_mod->popup, ELM_CTXPOPUP_DIRECTION_LEFT);
             if (avai == EINA_TRUE)
               return;

             x = sel.x;
             y = sel.y + sel.h;
             evas_object_move(ext_mod->popup, x, y);
             avai = elm_ctxpopup_direction_available_get
                (ext_mod->popup, ELM_CTXPOPUP_DIRECTION_LEFT);
             if (avai == EINA_TRUE)
               return;

             elm_ctxpopup_direction_priority_set(ext_mod->popup,
                                                 ELM_CTXPOPUP_DIRECTION_RIGHT,
                                                 ELM_CTXPOPUP_DIRECTION_LEFT,
                                                 ELM_CTXPOPUP_DIRECTION_UP,
                                                 ELM_CTXPOPUP_DIRECTION_DOWN);
             x = sel.x + sel.w;
             y = sel.y + sel.h/2;
             evas_object_move(ext_mod->popup, x, y);
             avai = elm_ctxpopup_direction_available_get
                (ext_mod->popup, ELM_CTXPOPUP_DIRECTION_RIGHT);
             if (avai == EINA_TRUE)
               return;

             x = sel.x + sel.w;
             y = sel.y + sel.h;
             evas_object_move(ext_mod->popup, x, y);
             avai = elm_ctxpopup_direction_available_get
                (ext_mod->popup, ELM_CTXPOPUP_DIRECTION_RIGHT);
             if (avai == EINA_TRUE)
               return;

             x = sel.x;
             y = sel.y + sel.h/2;
             while (x < sel.x + sel.w/2)
               {
                  evas_object_move(ext_mod->popup, x, y);
                  avai = elm_ctxpopup_direction_available_get
                     (ext_mod->popup, ELM_CTXPOPUP_DIRECTION_LEFT);
                  if (avai == EINA_TRUE)
                    return;
                  x += sel.w/4;
               }

             x = sel.x + sel.w;
             y = sel.y + sel.h/2;
             while (x > sel.x + sel.w/2)
               {
                  evas_object_move(ext_mod->popup, x, y);
                  avai = elm_ctxpopup_direction_available_get
                     (ext_mod->popup, ELM_CTXPOPUP_DIRECTION_RIGHT);
                  if (avai == EINA_TRUE)
                    return;
                  x -= sel.w/4;
               }

             x = sel.x + sel.w;
             y = sel.y + sel.h;
             while (x > sel.x + sel.w/2)
               {
                  evas_object_move(ext_mod->popup, x, y);
                  avai = elm_ctxpopup_direction_available_get
                     (ext_mod->popup, ELM_CTXPOPUP_DIRECTION_RIGHT);
                  if (avai == EINA_TRUE)
                    return;
                  x -= sel.w/4;
               }
          }
     }
}

static void
_select_all(void *data, Evas_Object *obj, void *event_info)
{
   if((!ext_mod) || (!data)) return;

   _ctxpopup_hide(obj);
   ext_mod->selectall(data, obj, event_info);
}

static void
_select(void *data, Evas_Object *obj, void *event_info)
{
   if((!ext_mod) || (!data)) return;

   ext_mod->select(data, obj, event_info);
   _ctxpopup_hide(obj);
}

static void
_paste(void *data, Evas_Object *obj, void *event_info)
{
   if((!ext_mod) || (!data)) return;

   ext_mod->paste(data, obj, event_info);
   _ctxpopup_hide(obj);
}

static void
_cut(void *data, Evas_Object *obj, void *event_info)
{
   if((!ext_mod) || (!data)) return;

   ext_mod->cut(data, obj, event_info);
   _ctxpopup_hide(obj);
}

static void
_copy(void *data, Evas_Object *obj, void *event_info)
{
   if((!ext_mod) || (!data)) return;

   ext_mod->copy(data, obj, event_info);
   _ctxpopup_hide(obj);
   elm_entry_select_none(data);
}

static void
_cancel(void *data, Evas_Object *obj, void *event_info)
{
   if((!ext_mod) || (!data)) return;

   ext_mod->cancel(data, obj, event_info);
   _ctxpopup_hide(obj);
}

#if HAVE_APPSVC
static void
_recvd_bundle(const char *key, const int type __UNUSED__, const bundle_keyval_t *kv, void *date __UNUSED__)
{
   char *val;
   size_t size;

   if (!ext_mod) return;

   if (bundle_keyval_type_is_array((bundle_keyval_t *)kv) <= 0)
     {
        bundle_keyval_get_basic_val((bundle_keyval_t *)kv, (void **)&val, &size);
        if (!strcmp(key, "source_text"))
          {
             ext_mod->source_text = strdup(val);
          }
        if (!strcmp(key, "target_text"))
          {
             ext_mod->target_text = strdup(val);
          }
     }
}

static void
_translate_cb(bundle *b, int request_code __UNUSED__, appsvc_result_val rv, void *data __UNUSED__)
{
   if (!ext_mod) return;

   if (ext_mod->source_text)
     {
        free(ext_mod->source_text);
        ext_mod->source_text = NULL;
     }
   if (ext_mod->target_text)
     {
        free(ext_mod->target_text);
        ext_mod->target_text = NULL;
     }
   if ((rv == APPSVC_RES_CANCEL) || (rv == APPSVC_RES_NOT_OK)) //CANCEL, NOT_OK, OK
     {
        return;
     }
   else
     {
        bundle_foreach(b, _recvd_bundle, NULL);
     }
   if ((ext_mod->source_text) || (ext_mod->target_text))
     {
        if (ext_mod->editable)
          {
             ext_mod->paste_translation(ext_mod->source_text,
                                        ext_mod->caller, ext_mod->target_text);
          }
     }
}

static unsigned int
_get_window_id()
{
   Evas_Object *top;
   Ecore_X_Window xwin;

   top = elm_widget_top_get(ext_mod->caller);
   xwin = elm_win_xwindow_get(top);

   return xwin;
}

static void
_translate_menu(void *data __UNUSED__, Evas_Object *obj, void *event_info __UNUSED__)
{
   if (!ext_mod) return;

   bundle *b = bundle_create();
   if (!b)
     {
        return;
     }
   ext_mod->keep_selection(NULL, ext_mod->caller, NULL);

   appsvc_set_operation(b, APPSVC_OPERATION_PICK);
   appsvc_set_appid(b, "com.samsung.stranslator-shared");

   if (ext_mod->selmode)
     {
        const char *selection = elm_entry_selection_get(ext_mod->caller);
        if (selection)
          {
             unsigned int winid = _get_window_id();
             char wis[20];
             snprintf(wis, 20, "%u", winid);
             appsvc_add_data(b, "winid", wis);
             appsvc_add_data(b, "display", "yes");
             appsvc_add_data(b, "text_type", "plain_text");
             appsvc_add_data(b, "source_language", "recently_used"); //auto_detection, recently_used
             appsvc_add_data(b, "target_language", "recently_used"); //recently_used, specify(en_US)
             appsvc_add_data(b, "result_type", "selective"); //target_only, selective, both
             if (ext_mod->editable)
               appsvc_add_data(b, "mode", "edit_mode_translate"); //edit_mode, edit_mode_translate
             else
               appsvc_add_data(b, "mode", "view_mode_translate"); //view_mode, view_mode_translate

             char *str = _remove_tags(selection);
             if (str)
               appsvc_add_data(b, "source_text", str);
             else
               appsvc_add_data(b, "source_text", selection);
          }
     }
   appsvc_run_service(b, 0, _translate_cb, NULL);
   bundle_free(b);
   elm_entry_input_panel_hide(ext_mod->caller);
   _ctxpopup_hide(obj);
}
#endif

static void
_clipboard_menu(void *data, Evas_Object *obj, void *event_info __UNUSED__)
{
   if(!ext_mod) return;

   // start for cbhm
#ifdef HAVE_ELEMENTARY_X
   ecore_x_selection_secondary_set(elm_win_xwindow_get(obj), NULL, 0);
#endif
   if (ext_mod->cnp_mode != ELM_CNP_MODE_MARKUP)
     _cbhm_msg_send(data, "show0");
   else
     _cbhm_msg_send(data, "show1");
   _ctxpopup_hide(obj);
   // end for cbhm
}

static void
_item_clicked(void *data, Evas_Object *obj, void *event_info __UNUSED__)
{
   Elm_Entry_Context_Menu_Item *it = data;
   Evas_Object *obj2 = it->obj;

   if (it->func) it->func(it->data, obj2, NULL);
   _ctxpopup_hide(obj);
}

static void
_ctxpopup_dismissed_cb(void *data, Evas_Object *obj, void *event_info __UNUSED__)
{
   Elm_Ctxpopup_Direction dir;
   if (!ext_mod) return;

   dir = elm_ctxpopup_direction_get(obj);
   //clear selection if ctxpopup is dismissed by clicking (not parent resizing)
   if (dir != ELM_CTXPOPUP_DIRECTION_UNKNOWN)
     {
        if ((ext_mod->mouse_up) && (ext_mod->entry_move))
          {
             _ctxpopup_position(obj);
             evas_object_show(ext_mod->popup);
          }
        else
          {
             elm_entry_select_none(data);
             ext_mod->popup_showing = EINA_FALSE;
             if (_elm_config->access_mode)
               _elm_access_highlight_set(ext_mod->caller, EINA_FALSE);
          }
     }
   else if (ext_mod->popup_showing)
     {
        if (_in_viewport_check())
          {
             if (ext_mod->show_timer) ecore_timer_del(ext_mod->show_timer);
             ext_mod->show_timer = ecore_timer_add(0.1, _ctx_show, data);
          }
     }
   ext_mod->mouse_up = EINA_FALSE;
}

// module api funcs needed
EAPI int
elm_modapi_init(void *m __UNUSED__)
{
   return 1; // succeed always
}

EAPI int
elm_modapi_shutdown(void *m __UNUSED__)
{
   return 1; // succeed always
}

// module funcs for the specific module type
EAPI void
obj_hook(Evas_Object *obj)
{
   _mod_hook_count++;
   //if(_mod_hook_count > 1) return;

   if(!ext_mod)
     {
        ext_mod = ELM_NEW(Elm_Entry_Extension_data);
        if (!ext_mod) return;
        elm_entry_extension_module_data_get(obj, ext_mod);
        ext_mod->mouse_up = EINA_FALSE;
        ext_mod->mouse_down = EINA_FALSE;
        ext_mod->entry_move = EINA_FALSE;
     }
   // Clipboard item: can be removed by application
   elm_entry_context_menu_item_add(obj, E_("IDS_COM_BODY_CLIPBOARD"), NULL,
                                   ELM_ICON_STANDARD, NULL, NULL);

#if HAVE_APPSVC
   // Tranlate item: can be removed by application
   elm_entry_context_menu_item_add(obj, E_("IDS_MSG_HEADER_TRANSLATE"), NULL,
                                   ELM_ICON_STANDARD, NULL, NULL);
#endif
}

EAPI void
obj_unhook(Evas_Object *obj __UNUSED__)
{
   _mod_hook_count--;
   if(_mod_hook_count > 0) return;

   if(ext_mod)
     {
        evas_object_event_callback_del(ext_mod->ctx_par, EVAS_CALLBACK_MOUSE_DOWN,
                                       _parent_mouse_down_cb);
        evas_object_event_callback_del(ext_mod->ctx_par, EVAS_CALLBACK_MOUSE_UP,
                                       _parent_mouse_up_cb);
        if (ext_mod->show_timer)
          {
             ecore_timer_del(ext_mod->show_timer);
             ext_mod->show_timer = NULL;
          }
        if (ext_mod->popup)
          {
             evas_object_del(ext_mod->popup);
             ext_mod->popup = NULL;
          }
        if (ext_mod->source_text)
          {
             free(ext_mod->source_text);
             ext_mod->source_text = NULL;
          }
        if (ext_mod->target_text)
          {
             free(ext_mod->target_text);
             ext_mod->target_text = NULL;
          }
        free(ext_mod);
        ext_mod = NULL;
     }
}

EAPI void
obj_longpress(Evas_Object *obj)
{
   if(!ext_mod) return;

   Evas_Object *ctxparent;
   Evas_Object *parent, *child;
   const Eina_List *l;
   const Elm_Entry_Context_Menu_Item *it;
   const char *context_menu_orientation;
   Evas_Object* icon;
   Elm_Object_Item *added_item = NULL;
   Eina_Bool has_clipboard = EINA_FALSE;
#if HAVE_APPSVC
   Eina_Bool has_translate = EINA_FALSE;
#endif
   Eina_Bool has_focused = EINA_FALSE;
   Ecore_X_Atom first_cbhm_item_type = 0;

   /*update*/
   elm_entry_extension_module_data_get(obj, ext_mod);
   has_focused = elm_widget_focus_get(obj);
   if (ext_mod->context_menu && has_focused)
     {
        Eina_List *lao = NULL;
        Evas_Object *ao = NULL;
#ifdef HAVE_ELEMENTARY_X
        int cbhm_count = 0;
        if (elm_entry_cnp_mode_get(obj) != ELM_CNP_MODE_MARKUP)
          cbhm_count = _cbhm_item_count_get(obj, ATOM_INDEX_CBHM_COUNT_TEXT);
        else
          cbhm_count = _cbhm_item_count_get(obj, ATOM_INDEX_CBHM_COUNT_ALL);
#endif
        if (ext_mod->popup)
          {
             evas_object_event_callback_del(obj, EVAS_CALLBACK_DEL, _entry_del_cb);
             evas_object_event_callback_del(obj, EVAS_CALLBACK_HIDE, _entry_hide_cb);
             evas_object_event_callback_del(obj, EVAS_CALLBACK_MOVE, _entry_move_cb);
             evas_object_event_callback_del(obj, EVAS_CALLBACK_MOUSE_DOWN, _entry_mouse_down_cb);
             evas_object_event_callback_del(obj, EVAS_CALLBACK_MOUSE_MOVE, _entry_mouse_move_cb);
             evas_object_event_callback_del(obj, EVAS_CALLBACK_MOUSE_UP, _entry_mouse_up_cb);
             evas_object_event_callback_del(ext_mod->ctx_par, EVAS_CALLBACK_MOUSE_DOWN,
                                            _parent_mouse_down_cb);
             evas_object_event_callback_del(ext_mod->ctx_par, EVAS_CALLBACK_MOUSE_UP,
                                            _parent_mouse_up_cb);
             evas_object_smart_callback_del(ext_mod->popup, "dismissed", _ctxpopup_dismissed_cb);
             evas_object_del(ext_mod->popup);
             ext_mod->popup = NULL;
          }
        ctxparent = elm_widget_top_get(obj);
        parent = elm_widget_parent_get(obj);
        child = obj;
        if (parent)
          {
             while(parent)
               {
                  if (!strcmp(elm_widget_type_get(parent), "elm_conformant"))
                    {
                       ctxparent = child;
                       break;
                    }
                  child = parent;
                  parent = elm_widget_parent_get(parent);
               }
          }
        ext_mod->ctx_par = ctxparent;

        if(ctxparent)
          {
             ext_mod->popup = elm_ctxpopup_add(ctxparent);
             if (_elm_config->access_mode)
               {
                  elm_object_tree_focus_allow_set(ext_mod->popup, EINA_TRUE);
                  elm_object_focus_allow_set(ext_mod->popup, EINA_FALSE);
               }
             else
               {
                  elm_object_tree_focus_allow_set(ext_mod->popup, EINA_FALSE);
               }
             evas_object_smart_callback_add(ext_mod->popup, "dismissed", _ctxpopup_dismissed_cb, obj);
             evas_object_event_callback_add(obj, EVAS_CALLBACK_DEL, _entry_del_cb, ext_mod->popup);
             evas_object_event_callback_add(obj, EVAS_CALLBACK_HIDE, _entry_hide_cb, ext_mod->popup);
          }
        else
          {
             ext_mod->caller = NULL;
             return;
          }
        elm_object_style_set(ext_mod->popup, "copypaste");

        elm_ctxpopup_horizontal_set(ext_mod->popup, EINA_TRUE);
        context_menu_orientation = edje_object_data_get
           (ext_mod->ent, "context_menu_orientation");
        if ((context_menu_orientation) &&
            (!strcmp(context_menu_orientation, "vertical")))
          elm_ctxpopup_horizontal_set(ext_mod->popup, EINA_FALSE);

        EINA_LIST_FOREACH(ext_mod->items, l, it)
          {
             if (!strcmp(it->label, E_("IDS_COM_BODY_CLIPBOARD")))
               has_clipboard = EINA_TRUE;
#if HAVE_APPSVC
             else if (!strcmp(it->label, E_("IDS_MSG_HEADER_TRANSLATE")))
               has_translate = EINA_TRUE;
#endif
          }
        if (!ext_mod->selmode)
          {
             if (!elm_entry_is_empty(obj))
               {
                 if (!ext_mod->password)
                  {
#ifndef ELM_FEATURE_WEARABLE
                   icon = edje_object_add(evas_object_evas_get(ext_mod->popup));
                   CP_ICON_THEME_SET("select");
                   added_item = elm_ctxpopup_item_append(ext_mod->popup, E_("IDS_COM_SK_SELECT"), icon, _select, obj);
                   if (_elm_config->access_mode)
                     {
                        elm_object_focus_allow_set(VIEW(added_item), EINA_FALSE);
                        ao = ((Elm_Widget_Item *)added_item)->access_obj;
                        elm_object_focus_allow_set(ao, EINA_FALSE);
                        lao = eina_list_append(lao, ao);
                     }
#endif
                   }

                   icon = edje_object_add(evas_object_evas_get(ext_mod->popup));
                   CP_ICON_THEME_SET("select_all");
#ifdef ELM_FEATURE_WEARABLE
                   added_item = elm_ctxpopup_item_append(ext_mod->popup, NULL,
                                                          icon, _select_all, obj);
#else
                   added_item = elm_ctxpopup_item_append(ext_mod->popup, E_("IDS_COM_BODY_SELECT_ALL"),
                                                          icon, _select_all, obj);
#endif
                   if (_elm_config->access_mode)
                     {
                        elm_object_focus_allow_set(VIEW(added_item), EINA_FALSE);
                        ao = ((Elm_Widget_Item *)added_item)->access_obj;
                        elm_object_focus_allow_set(ao, EINA_FALSE);
                        lao = eina_list_append(lao, ao);
                     }
                }

#ifdef HAVE_ELEMENTARY_X
             if (cbhm_count)
#else
             if (1) // need way to detect if someone has a selection
#endif
               {
                  if (elm_entry_cnp_mode_get(obj) == ELM_CNP_MODE_PLAINTEXT)
                    {
                       _cbhm_item_get(obj, 0, &first_cbhm_item_type, NULL);
                       if (ext_mod->editable &&
                           !(first_cbhm_item_type == ecore_x_atom_get("text/uri")) &&
                           !(first_cbhm_item_type == ecore_x_atom_get("text/uri-list")))
                         {
                            icon = edje_object_add(evas_object_evas_get(ext_mod->popup));
                            CP_ICON_THEME_SET("paste");
#ifdef ELM_FEATURE_WEARABLE
                            added_item = elm_ctxpopup_item_append(ext_mod->popup, NULL,
                                                                  icon, _paste, obj);
#else
                            added_item = elm_ctxpopup_item_append(ext_mod->popup, E_("IDS_COM_BODY_PASTE"),
                                                                  icon, _paste, obj);
#endif
                            if (_elm_config->access_mode)
                              {
                                 elm_object_focus_allow_set(VIEW(added_item), EINA_FALSE);
                                 ao = ((Elm_Widget_Item *)added_item)->access_obj;
                                 elm_object_focus_allow_set(ao, EINA_FALSE);
                                 lao = eina_list_append(lao, ao);
                              }
                         }
                    }
                  else
                    {
                       if (ext_mod->editable)
                         {
                            icon = edje_object_add(evas_object_evas_get(ext_mod->popup));
                            CP_ICON_THEME_SET("paste");
#ifdef ELM_FEATURE_WEARABLE
                            added_item = elm_ctxpopup_item_append(ext_mod->popup, NULL,
                                                                  icon, _paste, obj);
#else
                            added_item = elm_ctxpopup_item_append(ext_mod->popup, E_("IDS_COM_BODY_PASTE"),
                                                                  icon, _paste, obj);
#endif
                            if (_elm_config->access_mode)
                              {
                                 elm_object_focus_allow_set(VIEW(added_item), EINA_FALSE);
                                 ao = ((Elm_Widget_Item *)added_item)->access_obj;
                                 elm_object_focus_allow_set(ao, EINA_FALSE);
                                 lao = eina_list_append(lao, ao);
                              }
                         }
                    }
               }
             //elm_ctxpopup_item_append(wd->ctxpopup, NULL, "Selectall",_select_all, obj );
             // start for cbhm
#ifdef HAVE_ELEMENTARY_X
             if ((ext_mod->editable) && (cbhm_count) && (has_clipboard))
#else
             if ((ext_mod->editable) && (has_clipboard))
#endif
               {
                  icon = edje_object_add(evas_object_evas_get(ext_mod->popup));
                  CP_ICON_THEME_SET("clipboard");
#ifdef ELM_FEATURE_WEARABLE
                  added_item = elm_ctxpopup_item_append(ext_mod->popup, NULL,
                                                        icon, _clipboard_menu, obj);  // Clipboard
#else
                  added_item = elm_ctxpopup_item_append(ext_mod->popup, E_("IDS_COM_BODY_CLIPBOARD"),
                                                        icon, _clipboard_menu, obj);	// Clipboard
#endif
                  if (_elm_config->access_mode)
                    {
                       elm_object_focus_allow_set(VIEW(added_item), EINA_FALSE);
                       ao = ((Elm_Widget_Item *)added_item)->access_obj;
                       elm_object_focus_allow_set(ao, EINA_FALSE);
                       lao = eina_list_append(lao, ao);
                    }
                  //elm_ctxpopup_item_append(ext_mod->popup, "More", NULL, _clipboard_menu, obj );
               }
             // end for cbhm
#if HAVE_APPSVC
             const char *entry_str;
             entry_str = elm_entry_selection_get(obj);
             if ((entry_str) && (has_translate))
               {
                  icon = edje_object_add(evas_object_evas_get(ext_mod->popup));
                  CP_ICON_THEME_SET("translate");
                  added_item = elm_ctxpopup_item_append(ext_mod->popup, S_TRANSLATE,
                                                        icon, _translate_menu, obj);
                  if (_elm_config->access_mode)
                    {
                       elm_object_focus_allow_set(VIEW(added_item), EINA_FALSE);
                       ao = ((Elm_Widget_Item *)added_item)->access_obj;
                       elm_object_focus_allow_set(ao, EINA_FALSE);
                       lao = eina_list_append(lao, ao);
                    }
               }
#endif
          }
        else
          {
              if (ext_mod->have_selection)
                {
                   Eina_Bool selected_all = EINA_TRUE;
                   ext_mod->is_selected_all(&selected_all, obj, NULL);
                   if (selected_all == EINA_FALSE)
                     {
                        icon = edje_object_add(evas_object_evas_get(ext_mod->popup));
                        CP_ICON_THEME_SET("select_all");
#ifdef ELM_FEATURE_WEARABLE
                        added_item = elm_ctxpopup_item_append(ext_mod->popup, NULL,
                                                              icon, _select_all, obj);
#else
                        added_item = elm_ctxpopup_item_append(ext_mod->popup, E_("IDS_COM_BODY_SELECT_ALL"),
                                                              icon, _select_all, obj);
#endif
                        if (_elm_config->access_mode)
                          {
                             elm_object_focus_allow_set(VIEW(added_item), EINA_FALSE);
                             ao = ((Elm_Widget_Item *)added_item)->access_obj;
                             elm_object_focus_allow_set(ao, EINA_FALSE);
                             lao = eina_list_append(lao, ao);
                          }
                     }

                   if (!ext_mod->password)
                     {
                        icon = edje_object_add(evas_object_evas_get(ext_mod->popup));
                        CP_ICON_THEME_SET("copy");
#ifdef ELM_FEATURE_WEARABLE
                        added_item = elm_ctxpopup_item_append(ext_mod->popup, NULL,
                                                         icon, _copy, obj);
#else
                        added_item = elm_ctxpopup_item_append(ext_mod->popup, E_("IDS_COM_BODY_COPY"),
                                                         icon, _copy, obj);
#endif
                        if (_elm_config->access_mode)
                          {
                             elm_object_focus_allow_set(VIEW(added_item), EINA_FALSE);
                             ao = ((Elm_Widget_Item *)added_item)->access_obj;
                             elm_object_focus_allow_set(ao, EINA_FALSE);
                             lao = eina_list_append(lao, ao);
                          }
                        if (ext_mod->editable)
                          {
                             icon = edje_object_add(evas_object_evas_get(ext_mod->popup));
                             CP_ICON_THEME_SET("cut");
#ifdef ELM_FEATURE_WEARABLE
                             added_item = elm_ctxpopup_item_append(ext_mod->popup, NULL,
                                                              icon, _cut, obj);
#else
                             added_item = elm_ctxpopup_item_append(ext_mod->popup, E_("IDS_COM_BODY_CUT"),
                                                              icon, _cut, obj);
#endif
                             if (_elm_config->access_mode)
                               {
                                  elm_object_focus_allow_set(VIEW(added_item), EINA_FALSE);
                                  ao = ((Elm_Widget_Item *)added_item)->access_obj;
                                  elm_object_focus_allow_set(ao, EINA_FALSE);
                                  lao = eina_list_append(lao, ao);
                               }
                          }
                     }

#ifdef HAVE_ELEMENTARY_X
                   if (ext_mod->editable && cbhm_count)
#else
                   if (ext_mod->editable)
#endif
                     {
                        if (elm_entry_cnp_mode_get(obj) == ELM_CNP_MODE_PLAINTEXT)
                          {
                             _cbhm_item_get(obj, 0, &first_cbhm_item_type, NULL);
                             if (!(first_cbhm_item_type == ecore_x_atom_get("text/uri")) &&
                                 !(first_cbhm_item_type == ecore_x_atom_get("text/uri-list")))
                               {
                                  icon = edje_object_add(evas_object_evas_get(ext_mod->popup));
                                  CP_ICON_THEME_SET("paste");
#ifdef ELM_FEATURE_WEARABLE
                                  added_item = elm_ctxpopup_item_append(ext_mod->popup, NULL,
                                                                        icon, _paste, obj);
#else
                                  added_item = elm_ctxpopup_item_append(ext_mod->popup, E_("IDS_COM_BODY_PASTE"),
                                                                        icon, _paste, obj);
#endif
                                  if (_elm_config->access_mode)
                                    {
                                       elm_object_focus_allow_set(VIEW(added_item), EINA_FALSE);
                                       ao = ((Elm_Widget_Item *)added_item)->access_obj;
                                       elm_object_focus_allow_set(ao, EINA_FALSE);
                                       lao = eina_list_append(lao, ao);
                                    }
                               }
                          }
                        else
                          {
                             icon = edje_object_add(evas_object_evas_get(ext_mod->popup));
                             CP_ICON_THEME_SET("paste");
#ifdef ELM_FEATURE_WEARABLE
                             added_item = elm_ctxpopup_item_append(ext_mod->popup, NULL,
                                                                   icon, _paste, obj);
#else
                             added_item = elm_ctxpopup_item_append(ext_mod->popup, E_("IDS_COM_BODY_PASTE"),
                                                                   icon, _paste, obj);
#endif
                             if (_elm_config->access_mode)
                               {
                                  elm_object_focus_allow_set(VIEW(added_item), EINA_FALSE);
                                  ao = ((Elm_Widget_Item *)added_item)->access_obj;
                                  elm_object_focus_allow_set(ao, EINA_FALSE);
                                  lao = eina_list_append(lao, ao);
                               }
                          }
                     }
                }
              else
                {
                   _cancel(obj,ext_mod->popup,NULL);
                   if (!elm_entry_is_empty(obj))
                     {
                       if (!ext_mod->password)
                         {
#ifndef ELM_FEATURE_WEARABLE
                            icon = edje_object_add(evas_object_evas_get(ext_mod->popup));
                            CP_ICON_THEME_SET("select");
                            added_item = elm_ctxpopup_item_append(ext_mod->popup, E_("IDS_COM_SK_SELECT"),
                                                              icon, _select, obj);
                            if (_elm_config->access_mode)
                              {
                                 elm_object_focus_allow_set(VIEW(added_item), EINA_FALSE);
                                 ao = ((Elm_Widget_Item *)added_item)->access_obj;
                                 elm_object_focus_allow_set(ao, EINA_FALSE);
                                 lao = eina_list_append(lao, ao);
                              }
#endif
                          }

                        icon = edje_object_add(evas_object_evas_get(ext_mod->popup));
                        CP_ICON_THEME_SET("select_all");
#ifdef ELM_FEATURE_WEARABLE
                        added_item = elm_ctxpopup_item_append(ext_mod->popup, NULL,
                                                              icon, _select_all, obj);
#else
                        added_item = elm_ctxpopup_item_append(ext_mod->popup, E_("IDS_COM_BODY_SELECT_ALL"),
                                                              icon, _select_all, obj);
#endif
                        if (_elm_config->access_mode)
                          {
                             elm_object_focus_allow_set(VIEW(added_item), EINA_FALSE);
                             ao = ((Elm_Widget_Item *)added_item)->access_obj;
                             elm_object_focus_allow_set(ao, EINA_FALSE);
                             lao = eina_list_append(lao, ao);
                          }
                     }
#ifdef HAVE_ELEMENTARY_X
                   if (cbhm_count)
#else
                   if (1) // need way to detect if someone has a selection
#endif
                     {
                        if (ext_mod->editable)
                          {
                             if (elm_entry_cnp_mode_get(obj) == ELM_CNP_MODE_PLAINTEXT)
                               {
                                  _cbhm_item_get(obj, 0, &first_cbhm_item_type, NULL);
                                  if (!(first_cbhm_item_type == ecore_x_atom_get("text/uri")) &&
                                      !(first_cbhm_item_type == ecore_x_atom_get("text/uri-list")))
                                    {
                                       icon = edje_object_add(evas_object_evas_get(ext_mod->popup));
                                       CP_ICON_THEME_SET("paste");
#ifdef ELM_FEATURE_WEARABLE
                                       added_item = elm_ctxpopup_item_append(ext_mod->popup, NULL,
                                                                             icon, _paste, obj);
#else
                                       added_item = elm_ctxpopup_item_append(ext_mod->popup, E_("IDS_COM_BODY_PASTE"),
                                                                             icon, _paste, obj);
#endif
                                       if (_elm_config->access_mode)
                                         {
                                            elm_object_focus_allow_set(VIEW(added_item), EINA_FALSE);
                                            ao = ((Elm_Widget_Item *)added_item)->access_obj;
                                            elm_object_focus_allow_set(ao, EINA_FALSE);
                                            lao = eina_list_append(lao, ao);
                                         }
                                    }
                                }
                               else
                                {
                                    icon = edje_object_add(evas_object_evas_get(ext_mod->popup));
                                    CP_ICON_THEME_SET("paste");
#ifdef ELM_FEATURE_WEARABLE
                                    added_item = elm_ctxpopup_item_append(ext_mod->popup, NULL,
                                                                          icon, _paste, obj);
#else
                                    added_item = elm_ctxpopup_item_append(ext_mod->popup, E_("IDS_COM_BODY_PASTE"),
                                                                          icon, _paste, obj);
#endif
                                    if (_elm_config->access_mode)
                                      {
                                         elm_object_focus_allow_set(VIEW(added_item), EINA_FALSE);
                                         ao = ((Elm_Widget_Item *)added_item)->access_obj;
                                         elm_object_focus_allow_set(ao, EINA_FALSE);
                                         lao = eina_list_append(lao, ao);
                                      }
                                }
                         }
                    }
                }
                  // start for cbhm
#ifdef HAVE_ELEMENTARY_X
                  if ((ext_mod->editable) && (cbhm_count) && (has_clipboard))
#else
                  if ((ext_mod->editable) && (has_clipboard))
#endif
                    {
                       icon = edje_object_add(evas_object_evas_get(ext_mod->popup));
                       CP_ICON_THEME_SET("clipboard");
#ifdef ELM_FEATURE_WEARABLE
                       added_item = elm_ctxpopup_item_append(ext_mod->popup, NULL,
                                                             icon, _clipboard_menu, obj);
#else
                       added_item = elm_ctxpopup_item_append(ext_mod->popup, E_("IDS_COM_BODY_CLIPBOARD"),
                                                             icon, _clipboard_menu, obj);
#endif
                       if (_elm_config->access_mode)
                         {
                            elm_object_focus_allow_set(VIEW(added_item), EINA_FALSE);
                            ao = ((Elm_Widget_Item *)added_item)->access_obj;
                            elm_object_focus_allow_set(ao, EINA_FALSE);
                            lao = eina_list_append(lao, ao);
                         }
                       //elm_ctxpopup_item_append(ext_mod->popup, "More", NULL, _clipboard_menu, obj );
                    }
                  // end for cbhm
#if HAVE_APPSVC
                  const char *entry_str;
                  entry_str = elm_entry_selection_get(obj);
                  if ((entry_str) && (has_translate))
                    {
                       icon = edje_object_add(evas_object_evas_get(ext_mod->popup));
                       CP_ICON_THEME_SET("translate");
                       added_item = elm_ctxpopup_item_append(ext_mod->popup, S_TRANSLATE,
                                                             icon, _translate_menu, obj);
                       if (_elm_config->access_mode)
                         {
                            elm_object_focus_allow_set(VIEW(added_item), EINA_FALSE);
                            ao = ((Elm_Widget_Item *)added_item)->access_obj;
                            elm_object_focus_allow_set(ao, EINA_FALSE);
                            lao = eina_list_append(lao, ao);
                         }
                    }
#endif
          }
        EINA_LIST_FOREACH(ext_mod->items, l, it)
          {
             if (strcmp(it->label, E_("IDS_COM_BODY_CLIPBOARD")) && strcmp(it->label, E_("IDS_MSG_HEADER_TRANSLATE")))
               {
                  Evas_Object *ic = NULL;
                  if (it->icon_file)
                    {
                       ic = elm_icon_add(obj);
                       elm_image_resizable_set(ic, EINA_FALSE, EINA_TRUE);
                       if (it->icon_type == ELM_ICON_FILE)
                         elm_image_file_set(ic, it->icon_file, it->icon_group);
                       else if (it->icon_type == ELM_ICON_STANDARD)
                         elm_icon_standard_set(ic, it->icon_file);
                    }
#ifdef ELM_FEATURE_WEARABLE
                  added_item = elm_ctxpopup_item_append(ext_mod->popup, NULL,
                                                        ic, _item_clicked, it );
#else
                  added_item = elm_ctxpopup_item_append(ext_mod->popup, it->label,
                                                        ic, _item_clicked, it );
#endif
                  if (_elm_config->access_mode)
                    {
                       elm_object_focus_allow_set(VIEW(added_item), EINA_FALSE);
                       ao = ((Elm_Widget_Item *)added_item)->access_obj;
                       elm_object_focus_allow_set(ao, EINA_FALSE);
                       lao = eina_list_append(lao, ao);
                    }
               }
          }
        if (ext_mod->popup && added_item)
          {
             Evas_Object *first_it_ao, *cur_it_ao, *prev_it_ao;
             Evas_Object *popup_ao;
             Elm_Widget_Smart_Data *wd;
             Evas_Object *po;
             int layer;
             int count, itn;
             Eina_List *lc;

             ext_mod->caller = obj;
             if (_in_viewport_check())
               {
                  ext_mod->popup_showing = EINA_TRUE;
                  _ctxpopup_position(obj);
                  evas_object_show(ext_mod->popup);
                  ext_mod->popup_clicked = EINA_FALSE;
                  ext_mod->mouse_up = EINA_FALSE;
               }

             evas_object_event_callback_add(obj, EVAS_CALLBACK_MOVE, _entry_move_cb, ext_mod->popup);
             evas_object_event_callback_add(obj, EVAS_CALLBACK_MOUSE_DOWN,
                                            _entry_mouse_down_cb, ext_mod->popup);
             evas_object_event_callback_add(obj, EVAS_CALLBACK_MOUSE_MOVE,
                                            _entry_mouse_move_cb, ext_mod->popup);
             evas_object_event_callback_add(obj, EVAS_CALLBACK_MOUSE_UP,
                                            _entry_mouse_up_cb, ext_mod->popup);
             evas_object_event_callback_add(ext_mod->ctx_par, EVAS_CALLBACK_MOUSE_DOWN,
                                            _parent_mouse_down_cb, ext_mod->popup);
             evas_object_event_callback_add(ext_mod->ctx_par, EVAS_CALLBACK_MOUSE_UP,
                                            _parent_mouse_up_cb, ext_mod->popup);
             layer = evas_object_layer_get(ext_mod->caller);
             layer = layer >= EVAS_LAYER_MAX ? EVAS_LAYER_MAX : layer + 1;
             evas_object_layer_set(ext_mod->popup, layer);

             if (_elm_config->access_mode)
               {
                  wd = evas_object_smart_data_get(ext_mod->popup);
                  po = (Evas_Object *)edje_object_part_object_get(wd->resize_obj,
                                                                  "access.outline");
                  popup_ao = elm_access_object_get(po);
                  count = eina_list_count(lao);
                  first_it_ao = eina_list_data_get(lao);
                  elm_access_highlight_next_set(popup_ao,
                                                ELM_HIGHLIGHT_DIR_NEXT,
                                                first_it_ao);
                  elm_access_highlight_next_set(first_it_ao,
                                                ELM_HIGHLIGHT_DIR_PREVIOUS,
                                                popup_ao);
                  prev_it_ao = eina_list_data_get(lao);
                  itn = 1;
                  EINA_LIST_FOREACH(lao, lc, cur_it_ao)
                    {
                       if (itn == count)
                         {
                            elm_access_highlight_next_set(prev_it_ao,
                                                          ELM_HIGHLIGHT_DIR_NEXT,
                                                          cur_it_ao);
                            elm_access_highlight_next_set(cur_it_ao,
                                                          ELM_HIGHLIGHT_DIR_PREVIOUS,
                                                          prev_it_ao);
                            elm_access_highlight_next_set(cur_it_ao,
                                                          ELM_HIGHLIGHT_DIR_NEXT,
                                                          popup_ao);
                            elm_access_highlight_next_set(popup_ao,
                                                          ELM_HIGHLIGHT_DIR_PREVIOUS,
                                                          cur_it_ao);
                         }
                       else if (itn > 1)
                         {
                            elm_access_highlight_next_set(prev_it_ao,
                                                          ELM_HIGHLIGHT_DIR_NEXT,
                                                          cur_it_ao);
                            elm_access_highlight_next_set(cur_it_ao,
                                                          ELM_HIGHLIGHT_DIR_PREVIOUS,
                                                          prev_it_ao);
                         }
                       itn++;
                       prev_it_ao = cur_it_ao;
                    }
                  elm_access_highlight_set(popup_ao);
               }
             eina_list_free(lao);
          }
        else
          ext_mod->caller = NULL;
     }
}

EAPI void
obj_mouseup(Evas_Object *obj)
{
   if (!obj || !ext_mod)
     return;
}


EAPI void
obj_hidemenu(Evas_Object *obj)
{
   if (!obj || !ext_mod || obj != ext_mod->caller)
     return;

   _ctxpopup_hide(ext_mod->popup);
   // if (ext_mod->popup) evas_object_del(ext_mod->popup);
}
