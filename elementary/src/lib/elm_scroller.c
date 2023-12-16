#include <Elementary.h>
#include "elm_priv.h"
#include "elm_widget_scroller.h"
EAPI const char ELM_SCROLLER_SMART_NAME[] = "elm_scroller";

static const char SIG_SCROLL[] = "scroll";
static const char SIG_SCROLL_LEFT[] = "scroll,left";
static const char SIG_SCROLL_RIGHT[] = "scroll,right";
static const char SIG_SCROLL_UP[] = "scroll,up";
static const char SIG_SCROLL_DOWN[] = "scroll,down";
static const char SIG_SCROLL_ANIM_START[] = "scroll,anim,start";
static const char SIG_SCROLL_ANIM_STOP[] = "scroll,anim,stop";
static const char SIG_SCROLL_DRAG_START[] = "scroll,drag,start";
static const char SIG_SCROLL_DRAG_STOP[] = "scroll,drag,stop";
static const char SIG_EDGE_LEFT[] = "edge,left";
static const char SIG_EDGE_RIGHT[] = "edge,right";
static const char SIG_EDGE_TOP[] = "edge,top";
static const char SIG_EDGE_BOTTOM[] = "edge,bottom";
static const char SIG_VBAR_DRAG[] = "vbar,drag";
static const char SIG_VBAR_PRESS[] = "vbar,press";
static const char SIG_VBAR_UNPRESS[] = "vbar,unpress";
static const char SIG_HBAR_DRAG[] = "hbar,drag";
static const char SIG_HBAR_PRESS[] = "hbar,press";
static const char SIG_HBAR_UNPRESS[] = "hbar,unpress";
static const char SIG_CONTENT_RESIZE[] = "content,resize";
static const Evas_Smart_Cb_Description _smart_callbacks[] =
{
   {SIG_SCROLL, ""},
   {SIG_SCROLL_LEFT, ""},
   {SIG_SCROLL_RIGHT, ""},
   {SIG_SCROLL_UP, ""},
   {SIG_SCROLL_DOWN, ""},
   {SIG_SCROLL_ANIM_START, ""},
   {SIG_SCROLL_ANIM_STOP, ""},
   {SIG_SCROLL_DRAG_START, ""},
   {SIG_SCROLL_DRAG_STOP, ""},
   {SIG_EDGE_LEFT, ""},
   {SIG_EDGE_RIGHT, ""},
   {SIG_EDGE_TOP, ""},
   {SIG_EDGE_BOTTOM, ""},
   {SIG_VBAR_DRAG, ""},
   {SIG_VBAR_PRESS, ""},
   {SIG_VBAR_UNPRESS, ""},
   {SIG_HBAR_DRAG, ""},
   {SIG_HBAR_PRESS, ""},
   {SIG_HBAR_UNPRESS, ""},
   {NULL, NULL}
};

static const Evas_Smart_Interface *_smart_interfaces[] =
{
   (Evas_Smart_Interface *)&ELM_SCROLLABLE_IFACE, NULL
};

EVAS_SMART_SUBCLASS_IFACE_NEW
  (ELM_SCROLLER_SMART_NAME, _elm_scroller, Elm_Scroller_Smart_Class,
  Elm_Layout_Smart_Class, elm_layout_smart_class_get, _smart_callbacks,
  _smart_interfaces);

static void
_elm_scroller_proxy_set(Evas_Object *obj, Elm_Scroller_Smart_Data *sd, Evas_Object *proxy)
{
   Evas_Coord h_pagesize, v_pagesize;
   Evas_Coord cw, ch;
   Evas_Object *content = sd->content;

   if (!content) return;

   sd->s_iface->paging_get(obj, NULL, NULL, &h_pagesize, &v_pagesize);
   sd->s_iface->content_size_get(obj, &cw, &ch);
   /* Since Proxy has the texture size limitation problem, we set a key value
      for evas works in some hackish way to avoid this problem. This hackish
      code should be removed once evas supports a mechanism like a virtual
      texture. */
   evas_object_data_set(proxy, "tizenonly", (void *) 1);
   evas_object_image_fill_set(proxy, 0, 0, cw, ch);
   evas_object_size_hint_min_set(proxy, h_pagesize, v_pagesize);
   evas_object_image_source_clip_set(proxy, EINA_FALSE);
   evas_object_image_source_set(proxy, content);
   evas_object_show(proxy);

   /* Tizen Only: In apptray usage, apptray needs to change color of the
   proxy dynamically, here is the code is product specific code to change the
   page color without any performance drop. */
   evas_object_data_set(obj, "proxy_obj", proxy);
}

static Eina_Bool
_elm_scroller_smart_event(Evas_Object *obj,
                          Evas_Object *src __UNUSED__,
                          Evas_Callback_Type type,
                          void *event_info)
{
   Evas_Coord x = 0;
   Evas_Coord y = 0;
   Evas_Coord c_x = 0;
   Evas_Coord c_y = 0;
   Evas_Coord v_w = 0;
   Evas_Coord v_h = 0;
   Evas_Coord max_x = 0;
   Evas_Coord max_y = 0;
   Evas_Coord page_x = 0;
   Evas_Coord page_y = 0;
   Evas_Coord step_x = 0;
   Evas_Coord step_y = 0;
   Evas_Event_Key_Down *ev = event_info;
   Elm_Scroller_Movement_Block block = ELM_SCROLLER_MOVEMENT_NO_BLOCK;
#ifdef ELM_FOCUSED_UI
   // TIZEN_ONLY (20130713) : For supporting Focused UI of TIZEN.
   const Evas_Object  *win = elm_widget_top_get(obj);
   const char *top_type = elm_widget_type_get(win);

   if (top_type && !strcmp(top_type, "elm_win"))
     {
        if (!elm_win_focus_highlight_enabled_get(win))
          return EINA_FALSE;
     }
   ///////////////////
#endif

   ELM_SCROLLER_DATA_GET(obj, sd);

   // TIZEN ONLY(20131221) : When access mode, focused ui is disabled.
   if (_elm_config->access_mode) return EINA_FALSE;

   if (elm_widget_disabled_get(obj)) return EINA_FALSE;
   if (type != EVAS_CALLBACK_KEY_DOWN) return EINA_FALSE;
   if (ev->event_flags & EVAS_EVENT_FLAG_ON_HOLD) return EINA_FALSE;

   sd->s_iface->content_pos_get(obj, &x, &y);
   sd->s_iface->step_size_get(obj, &step_x, &step_y);
   sd->s_iface->page_size_get(obj, &page_x, &page_y);
   sd->s_iface->content_viewport_size_get(obj, &v_w, &v_h);
   evas_object_geometry_get(sd->content, &c_x, &c_y, &max_x, &max_y);
   block = sd->s_iface->movement_block_get(obj);
#ifdef ELM_FOCUSED_UI
   if (((!strcmp(ev->keyname, "Left")) ||
        (!strcmp(ev->keyname, "KP_Left")) ||
        (!strcmp(ev->keyname, "Right")) ||
        (!strcmp(ev->keyname, "KP_Right")) ||
        (!strcmp(ev->keyname, "Up")) ||
        (!strcmp(ev->keyname, "KP_Up")) ||
        (!strcmp(ev->keyname, "Down")) ||
        (!strcmp(ev->keyname, "KP_Down"))) && (!ev->string))
     {
        Evas_Object *current_focus = NULL;
        Evas_Object *new_focus = NULL;
        Eina_List *can_focus_list = NULL;
        Elm_Focus_Direction dir = ELM_FOCUS_NONE;
        Evas_Coord f_x = 0;
        Evas_Coord f_y = 0;
        Evas_Coord f_w = 0;
        Evas_Coord f_h = 0;

        if ((!strcmp(ev->keyname, "Left")) || (!strcmp(ev->keyname, "KP_Left")))
          dir = ELM_FOCUS_LEFT;
        else if ((!strcmp(ev->keyname, "Right")) || (!strcmp(ev->keyname, "KP_Right")))
          dir = ELM_FOCUS_RIGHT;
        else if ((!strcmp(ev->keyname, "Up")) || (!strcmp(ev->keyname, "KP_Up")))
          dir = ELM_FOCUS_UP;
        else if ((!strcmp(ev->keyname, "Down")) || (!strcmp(ev->keyname, "KP_Down")))
          dir = ELM_FOCUS_DOWN;

        current_focus = elm_widget_focused_object_get(obj);
        evas_object_geometry_get(current_focus, &f_x, &f_y, &f_w, &f_h);
        can_focus_list = elm_widget_can_focus_child_list_get(obj);
        if ((current_focus == obj) ||
            (!ELM_RECTS_INTERSECT
               (x, y, v_w, v_h, (f_x - c_x), (f_y - c_y), f_w, f_h)))
          {
             Eina_List *l;
             Evas_Object *cur;
             double weight = 0.0;

             EINA_LIST_FOREACH(can_focus_list, l, cur)
               {
                  double cur_weight = 0.0;

                  evas_object_geometry_get(cur, &f_x, &f_y, &f_w, &f_h);
                  if (ELM_RECTS_INTERSECT
                        (x, y, v_w, v_h, (f_x - c_x), (f_y - c_y), f_w, f_h))
                    {
                       if ((f_x - c_x) > x)
                         cur_weight += ((f_x - c_x) - x) * ((f_x - c_x) - x);
                       if ((f_y - c_y) > y)
                         cur_weight += ((f_y - c_y) - y) * ((f_y - c_y) - y);
                       if (cur_weight == 0.0)
                         {
                            elm_widget_focus_steal_with_direction(cur, dir);
                            ev->event_flags |= EVAS_EVENT_FLAG_ON_HOLD;
                            return EINA_TRUE;
                         }
                       cur_weight = 1.0 / cur_weight;
                       if (cur_weight > weight)
                         {
                            new_focus = cur;
                            weight = cur_weight;
                         }
                    }
               }
             if (new_focus)
               {
                  elm_widget_focus_steal_with_direction(new_focus, dir);
                  ev->event_flags |= EVAS_EVENT_FLAG_ON_HOLD;
                  return EINA_TRUE;
               }
          }
        else
          {
             Eina_Bool r = EINA_FALSE;

             if ((!strcmp(ev->key, "Left")) ||
                 (!strcmp(ev->key, "KP_Left")))
               r = elm_widget_focus_next_get(obj, ELM_FOCUS_LEFT, &new_focus);
             else if ((!strcmp(ev->key, "Right")) ||
                      (!strcmp(ev->key, "KP_Right")))
               r = elm_widget_focus_next_get(obj, ELM_FOCUS_RIGHT, &new_focus);
             else if ((!strcmp(ev->key, "Up")) ||
                      (!strcmp(ev->key, "KP_Up")))
               r = elm_widget_focus_next_get(obj, ELM_FOCUS_UP, &new_focus);
             else if ((!strcmp(ev->key, "Down")) ||
                      (!strcmp(ev->key, "KP_Down")))
               r = elm_widget_focus_next_get(obj, ELM_FOCUS_DOWN, &new_focus);

             if (r && new_focus)
               {
                  Evas_Coord l_x = 0;
                  Evas_Coord l_y = 0;
                  Evas_Coord l_w = 0;
                  Evas_Coord l_h = 0;

                  evas_object_geometry_get(new_focus, &f_x, &f_y, &f_w, &f_h);
                  l_x = f_x - c_x - step_x;
                  l_y = f_y - c_y - step_y;
                  l_w = f_w + (step_x * 2);
                  l_h = f_h + (step_y * 2);

                  if (ELM_RECTS_INTERSECT(x, y, v_w, v_h, l_x, l_y, l_w, l_h))
                    {
                       elm_widget_focus_steal_with_direction(new_focus, dir);
                       ev->event_flags |= EVAS_EVENT_FLAG_ON_HOLD;
                       return EINA_TRUE;
                    }
               }
          }
     }
   if (!(block & ELM_SCROLLER_MOVEMENT_BLOCK_HORIZONTAL))
     {
        if ((!strcmp(ev->keyname, "Left")) ||
            ((!strcmp(ev->keyname, "KP_Left")) && (!ev->string)))
          {
             if (x <= 0) return EINA_FALSE;
             x -= step_x;
          }
        else if ((!strcmp(ev->keyname, "Right")) ||
                 ((!strcmp(ev->keyname, "KP_Right")) && (!ev->string)))
          {
             if (x >= (max_x - v_w)) return EINA_FALSE;
             x += step_x;
          }
        else return EINA_FALSE;
     }
   else if (!(block & ELM_SCROLLER_MOVEMENT_BLOCK_VERTICAL))
     {
        if ((!strcmp(ev->keyname, "Up")) ||
            ((!strcmp(ev->keyname, "KP_Up")) && (!ev->string)))
          {
             if (y == 0) return EINA_FALSE;
             y -= step_y;
          }
        else if ((!strcmp(ev->keyname, "Down")) ||
                 ((!strcmp(ev->keyname, "KP_Down")) && (!ev->string)))
          {
             if (y >= (max_y - v_h)) return EINA_FALSE;
             y += step_y;
          }
        else if ((!strcmp(ev->keyname, "Home")) ||
                 ((!strcmp(ev->keyname, "KP_Home")) && (!ev->string)))
          {
             y = 0;
          }
        else if ((!strcmp(ev->keyname, "End")) ||
                 ((!strcmp(ev->keyname, "KP_End")) && (!ev->string)))
          {
             y = max_y - v_h;
          }
        else if ((!strcmp(ev->keyname, "Prior")) ||
                 ((!strcmp(ev->keyname, "KP_Prior")) && (!ev->string)))
          {
             if (page_y < 0)
               y -= -(page_y * v_h) / 100;
             else
               y -= page_y;
          }
        else if ((!strcmp(ev->keyname, "Next")) ||
                 ((!strcmp(ev->keyname, "KP_Next")) && (!ev->string)))
          {
             if (page_y < 0)
               y += -(page_y * v_h) / 100;
             else
               y += page_y;
          }
        else return EINA_FALSE;
     }
   else return EINA_FALSE;

   ev->event_flags |= EVAS_EVENT_FLAG_ON_HOLD;
   sd->s_iface->content_pos_set(obj, x, y, EINA_TRUE);

   return EINA_TRUE;
#else
   return EINA_FALSE;
#endif
}

static Eina_Bool
_elm_scroller_smart_focus_direction_manager_is(const Evas_Object *obj __UNUSED__)
{
   if (!elm_widget_can_focus_get(obj))
     return EINA_TRUE;
   else
     return EINA_FALSE;
}

static Eina_Bool
_elm_scroller_smart_focus_direction(const Evas_Object *obj,
                                    const Evas_Object *base,
                                    double degree,
                                    Evas_Object **direction,
                                    double *weight)
{
   ELM_SCROLLER_DATA_GET(obj, sd);
   if (!sd->content) return EINA_FALSE;
   return elm_widget_focus_direction_get(sd->content, base, degree, direction, weight);
}

static Eina_Bool
_elm_scroller_smart_activate(Evas_Object *obj, Elm_Activate act)
{
   Evas_Coord x = 0;
   Evas_Coord y = 0;
   Evas_Coord v_w = 0;
   Evas_Coord v_h = 0;
   Evas_Coord page_x = 0;
   Evas_Coord page_y = 0;
   Elm_Scroller_Movement_Block block = ELM_SCROLLER_MOVEMENT_NO_BLOCK;

   ELM_SCROLLER_DATA_GET(obj, sd);

   if (_elm_config->access_mode) return EINA_FALSE;
   if ((elm_widget_disabled_get(obj)) ||
       (act == ELM_ACTIVATE_DEFAULT) ||
       (act == ELM_ACTIVATE_BACK)) return EINA_FALSE;

   sd->s_iface->content_pos_get(obj, &x, &y);
   sd->s_iface->page_size_get(obj, &page_x, &page_y);
   sd->s_iface->content_viewport_size_get(obj, &v_w, &v_h);
   block = sd->s_iface->movement_block_get(obj);

   if (!(block & ELM_SCROLLER_MOVEMENT_BLOCK_VERTICAL))
     {
        if (act == ELM_ACTIVATE_UP)
          {
             if (page_y < 0)
               y -= -(page_y * v_h) / 100;
             else
               y -= page_y;
          }
        else if (act == ELM_ACTIVATE_DOWN)
          {
             if (page_y < 0)
               y += -(page_y * v_h) / 100;
             else
               y += page_y;
          }
        else return EINA_FALSE;
     }
   else if (!(block & ELM_SCROLLER_MOVEMENT_BLOCK_HORIZONTAL))
     {
        if (act == ELM_ACTIVATE_LEFT)
          {
             if (page_x < 0)
               x -= -(page_x * v_w) / 100;
             else
               x -= page_x;
          }
        else if (act == ELM_ACTIVATE_RIGHT)
          {
             if (page_x < 0)
               x += -(page_x * v_w) / 100;
             else
               x += page_x;
          }
        else return EINA_FALSE;
     }
   else return EINA_FALSE;

   sd->s_iface->content_pos_set(obj, x, y, EINA_TRUE);
   return EINA_TRUE;
}

static void
_elm_scroller_smart_sizing_eval(Evas_Object *obj)
{
   Evas_Coord vw = 0, vh = 0, minw = 0, minh = 0, maxw = 0, maxh = 0, w, h,
              vmw, vmh;
   Evas_Coord h_pagesize, v_pagesize;
   double xw = 0.0, yw = 0.0;
   int i;

   ELM_SCROLLER_DATA_GET(obj, sd);

   /* parent class' early call */
   if (!sd->s_iface) return;

   if (sd->content)
     {
        evas_object_size_hint_min_get(sd->content, &minw, &minh);
        evas_object_size_hint_max_get(sd->content, &maxw, &maxh);
        evas_object_size_hint_weight_get(sd->content, &xw, &yw);
     }

   sd->s_iface->content_viewport_size_get(obj, &vw, &vh);
   if (xw > 0.0)
     {
        if ((minw > 0) && (vw < minw))
          vw = minw;
        else if ((maxw > 0) && (vw > maxw))
          vw = maxw;
     }
   else if (minw > 0)
     vw = minw;

   if (yw > 0.0)
     {
        if ((minh > 0) && (vh < minh))
          vh = minh;
        else if ((maxh > 0) && (vh > maxh))
          vh = maxh;
     }
   else if (minh > 0)
     vh = minh;

   if (sd->content) evas_object_resize(sd->content, vw, vh);
   if (sd->contents) evas_object_resize(sd->contents, vw, vh);

   for (i = 0 ; i < 3 ; i++)
     {
        if (!sd->proxy_content[i]) continue;
        sd->s_iface->paging_get(obj, NULL, NULL, &h_pagesize, &v_pagesize);
        evas_object_image_fill_set(sd->proxy_content[i], 0, 0, vw,
                                   vh);
        evas_object_size_hint_min_set(sd->proxy_content[i], h_pagesize,
                                      v_pagesize);
     }

   w = -1;
   h = -1;
   edje_object_size_min_calc(ELM_WIDGET_DATA(sd)->resize_obj, &vmw, &vmh);

   if (sd->min_w) w = vmw + minw;
   if (sd->min_h) h = vmh + minh;

   evas_object_size_hint_max_get(obj, &maxw, &maxh);
   if ((maxw > 0) && (w > maxw)) w = maxw;
   if ((maxh > 0) && (h > maxh)) h = maxh;

   evas_object_size_hint_min_set(obj, w, h);
}

static void
_mirrored_set(Evas_Object *obj,
              Eina_Bool mirrored)
{
   ELM_SCROLLER_DATA_GET(obj, sd);

   sd->s_iface->mirrored_set(obj, mirrored);
}

static Eina_Bool
_elm_scroller_smart_theme(Evas_Object *obj)
{
   if (!ELM_WIDGET_CLASS(_elm_scroller_parent_sc)->theme(obj))
     return EINA_FALSE;

   _mirrored_set(obj, elm_widget_mirrored_get(obj));

   elm_layout_sizing_eval(obj);

   return EINA_TRUE;
}

static Eina_Bool
_elm_scroller_smart_focus_next(const Evas_Object *obj,
                               Elm_Focus_Direction dir,
                               Evas_Object **next)
{
   Evas_Object *cur;

   ELM_SCROLLER_DATA_GET(obj, sd);

   if (!sd->content) return EINA_FALSE;

   cur = sd->content;

   /* access */
   if (_elm_config->access_mode)
     return elm_widget_focus_next_get(cur, dir, next);

   /* Try focus cycle in subitem */
   if (elm_widget_focus_get(obj) || !elm_widget_can_focus_get(obj))
     {
        if ((elm_widget_can_focus_get(cur)) ||
            (elm_widget_child_can_focus_get(cur)))
          return elm_widget_focus_next_get(cur, dir, next);
     }

   /* Return */
   if (elm_widget_can_focus_get(obj))
     {
        *next = (Evas_Object *)obj;
        return !elm_widget_focus_get(obj);
     }
   else
     {
        *next = NULL;
        return EINA_FALSE;
     }
}

static void
_show_region_hook(void *data,
                  Evas_Object *content_obj)
{
   Evas_Coord x, y, w, h;

   ELM_SCROLLER_DATA_GET(data, sd);

   elm_widget_show_region_get(content_obj, &x, &y, &w, &h);
   if (sd->s_iface->is_auto_scroll_enabled(data))
      sd->s_iface->content_region_show(data, x, y, w, h);
}

static void
_changed_size_hints_cb(void *data,
                       Evas *e __UNUSED__,
                       Evas_Object *obj __UNUSED__,
                       void *event_info __UNUSED__)
{
   elm_layout_sizing_eval(data);
}

static void
_content_resize_cb(void *data,
                   Evas *e __UNUSED__,
                   Evas_Object *obj __UNUSED__,
                   void *event_info __UNUSED__)
{
   evas_object_smart_callback_call(data, SIG_CONTENT_RESIZE, NULL);
}

static Eina_Bool
_elm_scroller_smart_sub_object_del(Evas_Object *obj,
                                   Evas_Object *sobj)
{
   ELM_SCROLLER_DATA_GET(obj, sd);

   if (!ELM_WIDGET_CLASS(_elm_scroller_parent_sc)->sub_object_del(obj, sobj))
     return EINA_FALSE;

   if (sobj == sd->content)
     {
        elm_widget_on_show_region_hook_set(sd->content, NULL, NULL);

        sd->content = NULL;
     }

   return EINA_TRUE;
}

static void
_resize_cb(void *data,
           Evas *e __UNUSED__,
           Evas_Object *obj __UNUSED__,
           void *event_info __UNUSED__)
{
   elm_layout_sizing_eval(data);
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

static void
_vbar_drag_cb(Evas_Object *obj,
                void *data __UNUSED__)
{
   evas_object_smart_callback_call(obj, SIG_VBAR_DRAG, NULL);
}

static void
_vbar_press_cb(Evas_Object *obj,
                void *data __UNUSED__)
{
   evas_object_smart_callback_call(obj, SIG_VBAR_PRESS, NULL);
}

static void
_vbar_unpress_cb(Evas_Object *obj,
                void *data __UNUSED__)
{
   evas_object_smart_callback_call(obj, SIG_VBAR_UNPRESS, NULL);
}

static void
_hbar_drag_cb(Evas_Object *obj,
                void *data __UNUSED__)
{
   evas_object_smart_callback_call(obj, SIG_HBAR_DRAG, NULL);
}

static void
_hbar_press_cb(Evas_Object *obj,
                void *data __UNUSED__)
{
   evas_object_smart_callback_call(obj, SIG_HBAR_PRESS, NULL);
}

static void
_hbar_unpress_cb(Evas_Object *obj,
                void *data __UNUSED__)
{
   evas_object_smart_callback_call(obj, SIG_HBAR_UNPRESS, NULL);
}

static void
_scroll_cb(Evas_Object *obj,
           void *data __UNUSED__)
{
   evas_object_smart_callback_call(obj, SIG_SCROLL, NULL);
}

static void
_scroll_left_cb(Evas_Object *obj,
           void *data __UNUSED__)
{
   evas_object_smart_callback_call(obj, SIG_SCROLL_LEFT, NULL);
}

static void
_scroll_right_cb(Evas_Object *obj,
           void *data __UNUSED__)
{
   evas_object_smart_callback_call(obj, SIG_SCROLL_RIGHT, NULL);
}

static void
_scroll_up_cb(Evas_Object *obj,
           void *data __UNUSED__)
{
   evas_object_smart_callback_call(obj, SIG_SCROLL_UP, NULL);
}

static void
_scroll_down_cb(Evas_Object *obj,
           void *data __UNUSED__)
{
   evas_object_smart_callback_call(obj, SIG_SCROLL_DOWN, NULL);
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
   evas_object_smart_callback_call(obj, SIG_SCROLL_DRAG_START, NULL);
}

static void
_scroll_drag_stop_cb(Evas_Object *obj,
                     void *data __UNUSED__)
{
   evas_object_smart_callback_call(obj, SIG_SCROLL_DRAG_STOP, NULL);
}

static void
_loop_content_set(Evas_Object *obj, Evas_Object *content)
{
   ELM_SCROLLABLE_CHECK(obj);
   ELM_SCROLLER_DATA_GET(obj, sd);

   if (!sd->contents)
     {
        sd->contents = elm_layout_add(obj);
        evas_object_smart_member_add(sd->contents, obj);
        elm_layout_theme_set(sd->contents, "scroller", "contents", elm_widget_style_get(obj));
        evas_object_size_hint_weight_set(sd->contents, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
        evas_object_size_hint_align_set(sd->contents, EVAS_HINT_FILL, EVAS_HINT_FILL);

        elm_widget_sub_object_add(obj, sd->contents);
        elm_widget_on_show_region_hook_set(sd->contents, _show_region_hook, obj);
     }
   elm_object_part_content_set(sd->contents, "elm.swallow.content", content);
   sd->content = content;

   if (sd->loop_h)
     {
       if (!sd->proxy_content[0])
          sd->proxy_content[0] =
             evas_object_image_add(evas_object_evas_get(sd->contents));
        _elm_scroller_proxy_set(obj, sd, sd->proxy_content[0]);
        elm_object_part_content_set(sd->contents, "elm.swallow.content_r",
                                    sd->proxy_content[0]);
     }

   if (sd->loop_v)
     {
        if (!sd->proxy_content[1])
          sd->proxy_content[1] =
             evas_object_image_add(evas_object_evas_get(sd->contents));
        _elm_scroller_proxy_set(obj, sd, sd->proxy_content[1]);
        elm_object_part_content_set(sd->contents, "elm.swallow.content_b",
                                    sd->proxy_content[1]);
     }

   if (sd->loop_h && sd->loop_v)
     {
        if (!sd->proxy_content[2])
          sd->proxy_content[2] =
             evas_object_image_add(evas_object_evas_get(sd->contents));
        _elm_scroller_proxy_set(obj, sd, sd->proxy_content[2]);
        elm_object_part_content_set(sd->contents, "elm.swallow.content_rb",
                                    sd->proxy_content[2]);
     }
}

static Eina_Bool
_elm_scroller_smart_content_set(Evas_Object *obj,
                                const char *part,
                                Evas_Object *content)
{
   ELM_SCROLLER_DATA_GET(obj, sd);

   if (part && strcmp(part, "default"))
     return ELM_CONTAINER_CLASS
              (_elm_scroller_parent_sc)->content_set(obj, part, content);

   if (sd->content == content) return EINA_TRUE;

   if (sd->content) evas_object_del(sd->content);
   sd->content = content;

   if (content)
     {
        elm_widget_on_show_region_hook_set(content, _show_region_hook, obj);
        elm_widget_sub_object_add(obj, content);

        if (sd->loop_h || sd->loop_v)
          {
             _loop_content_set(obj, content);
             if(sd->contents)
               content = sd->contents;
          }
        sd->s_iface->content_set(obj, content);

        evas_object_event_callback_add
           (sd->content, EVAS_CALLBACK_CHANGED_SIZE_HINTS, _changed_size_hints_cb, obj);
        evas_object_event_callback_add
           (sd->content, EVAS_CALLBACK_RESIZE, _content_resize_cb, obj);
     }
   else
     {
        int i;
        for (i = 0; i < 3; i ++)
          {
             if (!sd->proxy_content[i]) continue;
             evas_object_del(sd->proxy_content[i]);
             sd->proxy_content[i] = NULL;
          }
     }

   elm_layout_sizing_eval(obj);

   return EINA_TRUE;
}

static Evas_Object *
_elm_scroller_smart_content_get(const Evas_Object *obj,
                                const char *part)
{
   ELM_SCROLLER_DATA_GET(obj, sd);

   if (part && strcmp(part, "default"))
     return ELM_CONTAINER_CLASS
              (_elm_scroller_parent_sc)->content_get(obj, part);

   return sd->content;
}

static Evas_Object *
_elm_scroller_smart_content_unset(Evas_Object *obj,
                                  const char *part)
{
   Evas_Object *content;

   ELM_SCROLLER_DATA_GET(obj, sd);

   if (part && strcmp(part, "default"))
     return ELM_CONTAINER_CLASS
              (_elm_scroller_parent_sc)->content_unset(obj, part);

   if (!sd->content) return NULL;
   evas_object_event_callback_del_full
      (sd->content, EVAS_CALLBACK_CHANGED_SIZE_HINTS, _changed_size_hints_cb, obj);
   evas_object_event_callback_del_full
      (sd->content, EVAS_CALLBACK_RESIZE, _content_resize_cb, obj);

   content = sd->content;
   if (sd->loop_h || sd->loop_v)
     elm_widget_sub_object_del(obj, sd->contents);
   else
     elm_widget_sub_object_del(obj, sd->content);
   sd->s_iface->content_set(obj, NULL);
   sd->content = NULL;

   return content;
}

static void
_elm_scroller_content_min_limit_cb(Evas_Object *obj,
                                   Eina_Bool w,
                                   Eina_Bool h)
{
   ELM_SCROLLER_DATA_GET(obj, sd);

   sd->min_w = !!w;
   sd->min_h = !!h;

   elm_layout_sizing_eval(obj);
}

static void
_elm_scroller_content_viewport_resize_cb(Evas_Object *obj,
                                         Evas_Coord w __UNUSED__,
                                         Evas_Coord h __UNUSED__)
{
   elm_layout_sizing_eval(obj);
}

static void
_elm_scroller_smart_add(Evas_Object *obj)
{
   EVAS_SMART_DATA_ALLOC(obj, Elm_Scroller_Smart_Data);

   ELM_WIDGET_CLASS(_elm_scroller_parent_sc)->base.add(obj);
}

static void
_elm_scroller_smart_move(Evas_Object *obj,
                         Evas_Coord x,
                         Evas_Coord y)
{
   ELM_SCROLLER_DATA_GET(obj, sd);

   ELM_WIDGET_CLASS(_elm_scroller_parent_sc)->base.move(obj, x, y);

   evas_object_move(sd->hit_rect, x, y);
}

static void
_elm_scroller_smart_resize(Evas_Object *obj,
                           Evas_Coord w,
                           Evas_Coord h)
{
   ELM_SCROLLER_DATA_GET(obj, sd);

   ELM_WIDGET_CLASS(_elm_scroller_parent_sc)->base.resize(obj, w, h);

   evas_object_resize(sd->hit_rect, w, h);
}

static void
_elm_scroller_smart_member_add(Evas_Object *obj,
                               Evas_Object *member)
{
   ELM_SCROLLER_DATA_GET(obj, sd);

   ELM_WIDGET_CLASS(_elm_scroller_parent_sc)->base.member_add(obj, member);

   if (sd->hit_rect)
     evas_object_raise(sd->hit_rect);
}

static void
_elm_scroller_smart_set_user(Elm_Scroller_Smart_Class *sc)
{
   ELM_WIDGET_CLASS(sc)->base.add = _elm_scroller_smart_add;
   ELM_WIDGET_CLASS(sc)->base.move = _elm_scroller_smart_move;
   ELM_WIDGET_CLASS(sc)->base.resize = _elm_scroller_smart_resize;
   ELM_WIDGET_CLASS(sc)->base.member_add = _elm_scroller_smart_member_add;

   ELM_WIDGET_CLASS(sc)->sub_object_del = _elm_scroller_smart_sub_object_del;
   ELM_WIDGET_CLASS(sc)->theme = _elm_scroller_smart_theme;
   ELM_WIDGET_CLASS(sc)->focus_next = _elm_scroller_smart_focus_next;
   ELM_WIDGET_CLASS(sc)->event = _elm_scroller_smart_event;
   ELM_WIDGET_CLASS(sc)->focus_direction_manager_is =
      _elm_scroller_smart_focus_direction_manager_is;
   ELM_WIDGET_CLASS(sc)->focus_direction = _elm_scroller_smart_focus_direction;
   ELM_WIDGET_CLASS(sc)->activate = _elm_scroller_smart_activate;

   ELM_CONTAINER_CLASS(sc)->content_set = _elm_scroller_smart_content_set;
   ELM_CONTAINER_CLASS(sc)->content_get = _elm_scroller_smart_content_get;
   ELM_CONTAINER_CLASS(sc)->content_unset = _elm_scroller_smart_content_unset;

   ELM_LAYOUT_CLASS(sc)->sizing_eval = _elm_scroller_smart_sizing_eval;
}

EAPI const Elm_Scroller_Smart_Class *
elm_scroller_smart_class_get(void)
{
   static Elm_Scroller_Smart_Class _sc =
     ELM_SCROLLER_SMART_CLASS_INIT_NAME_VERSION(ELM_SCROLLER_SMART_NAME);
   static const Elm_Scroller_Smart_Class *class = NULL;
   Evas_Smart_Class *esc = (Evas_Smart_Class *)&_sc;

   if (class)
     return class;

   _elm_scroller_smart_set(&_sc);
   esc->callbacks = _smart_callbacks;
   class = &_sc;

   return class;
}

EAPI Evas_Object *
elm_scroller_add(Evas_Object *parent)
{
   Evas *e;
   Evas_Object *obj;
   Evas_Coord minw, minh;

   EINA_SAFETY_ON_NULL_RETURN_VAL(parent, NULL);

   e = evas_object_evas_get(parent);
   if (!e) return NULL;

   obj = evas_object_smart_add(e, _elm_scroller_smart_class_new());

   if (!elm_widget_sub_object_add(parent, obj))
     ERR("could not add %p as sub object of %p", obj, parent);

   ELM_SCROLLER_DATA_GET(obj, sd);

   elm_widget_can_focus_set(obj, EINA_TRUE);

   elm_layout_theme_set(obj, "scroller", "base", elm_widget_style_get(obj));

   sd->hit_rect = evas_object_rectangle_add(evas_object_evas_get(obj));
   evas_object_smart_member_add(sd->hit_rect, obj);
   elm_widget_sub_object_add(obj, sd->hit_rect);

   evas_object_color_set(sd->hit_rect, 0, 0, 0, 0);
   evas_object_show(sd->hit_rect);
   evas_object_repeat_events_set(sd->hit_rect, EINA_TRUE);

   sd->s_iface = evas_object_smart_interface_get
       (obj, ELM_SCROLLABLE_IFACE_NAME);

   sd->s_iface->objects_set
     (obj, ELM_WIDGET_DATA(sd)->resize_obj, sd->hit_rect);

   evas_object_event_callback_add
     (obj, EVAS_CALLBACK_CHANGED_SIZE_HINTS, _changed_size_hints_cb, obj);

   edje_object_size_min_calc(ELM_WIDGET_DATA(sd)->resize_obj, &minw, &minh);
   evas_object_size_hint_min_set(obj, minw, minh);
   evas_object_event_callback_add(obj, EVAS_CALLBACK_RESIZE, _resize_cb, obj);

   sd->s_iface->edge_left_cb_set(obj, _edge_left_cb);
   sd->s_iface->edge_right_cb_set(obj, _edge_right_cb);
   sd->s_iface->edge_top_cb_set(obj, _edge_top_cb);
   sd->s_iface->edge_bottom_cb_set(obj, _edge_bottom_cb);
   sd->s_iface->vbar_drag_cb_set(obj, _vbar_drag_cb);
   sd->s_iface->vbar_press_cb_set(obj, _vbar_press_cb);
   sd->s_iface->vbar_unpress_cb_set(obj, _vbar_unpress_cb);
   sd->s_iface->hbar_drag_cb_set(obj, _hbar_drag_cb);
   sd->s_iface->hbar_press_cb_set(obj, _hbar_press_cb);
   sd->s_iface->hbar_unpress_cb_set(obj, _hbar_unpress_cb);
   sd->s_iface->scroll_cb_set(obj, _scroll_cb);
   sd->s_iface->scroll_left_cb_set(obj, _scroll_left_cb);
   sd->s_iface->scroll_right_cb_set(obj, _scroll_right_cb);
   sd->s_iface->scroll_up_cb_set(obj, _scroll_up_cb);
   sd->s_iface->scroll_down_cb_set(obj, _scroll_down_cb);
   sd->s_iface->animate_start_cb_set(obj, _scroll_anim_start_cb);
   sd->s_iface->animate_stop_cb_set(obj, _scroll_anim_stop_cb);
   sd->s_iface->drag_start_cb_set(obj, _scroll_drag_start_cb);
   sd->s_iface->drag_stop_cb_set(obj, _scroll_drag_stop_cb);

   sd->s_iface->content_min_limit_cb_set
     (obj, _elm_scroller_content_min_limit_cb);
   sd->s_iface->content_viewport_resize_cb_set
     (obj, _elm_scroller_content_viewport_resize_cb);

   //Tizen Only: This should be removed when eo is applied.
   ELM_WIDGET_DATA_GET(obj, wsd);
   wsd->on_create = EINA_FALSE;
   wsd->end_dir = ELM_HIGHLIGHT_DIR_FIRST;

   return obj;
}

/* deprecated */
EAPI void
elm_scroller_custom_widget_base_theme_set(Evas_Object *obj,
                                          const char *klass,
                                          const char *group)
{
   ELM_SCROLLER_CHECK(obj);
   ELM_SCROLLER_DATA_GET(obj, sd);

   EINA_SAFETY_ON_NULL_RETURN(klass);
   EINA_SAFETY_ON_NULL_RETURN(group);

   if (eina_stringshare_replace(&(ELM_LAYOUT_DATA(sd)->klass), klass) ||
       eina_stringshare_replace(&(ELM_LAYOUT_DATA(sd)->group), group))
     _elm_scroller_smart_theme(obj);
}

EAPI void
elm_scroller_content_min_limit(Evas_Object *obj,
                               Eina_Bool w,
                               Eina_Bool h)
{
   ELM_SCROLLABLE_CHECK(obj);

   s_iface->content_min_limit(obj, w, h);
}

EAPI void
elm_scroller_region_show(Evas_Object *obj,
                         Evas_Coord x,
                         Evas_Coord y,
                         Evas_Coord w,
                         Evas_Coord h)
{
   ELM_SCROLLABLE_CHECK(obj);

   s_iface->content_region_show(obj, x, y, w, h);
}

EAPI void
elm_scroller_policy_set(Evas_Object *obj,
                        Elm_Scroller_Policy policy_h,
                        Elm_Scroller_Policy policy_v)
{
   ELM_SCROLLABLE_CHECK(obj);

   if ((policy_h >= ELM_SCROLLER_POLICY_LAST) ||
       (policy_v >= ELM_SCROLLER_POLICY_LAST))
     return;

   s_iface->policy_set(obj, policy_h, policy_v);
}

EAPI void
elm_scroller_policy_get(const Evas_Object *obj,
                        Elm_Scroller_Policy *policy_h,
                        Elm_Scroller_Policy *policy_v)
{
   ELM_SCROLLABLE_CHECK(obj);

   s_iface->policy_get(obj, policy_h, policy_v);
}

EAPI void
elm_scroller_single_direction_set(Evas_Object *obj,
                                  Elm_Scroller_Single_Direction single_dir)
{
   ELM_SCROLLABLE_CHECK(obj);

   if (single_dir >= ELM_SCROLLER_SINGLE_DIRECTION_LAST)
     return;

   s_iface->single_direction_set(obj, single_dir);
}

EAPI Elm_Scroller_Single_Direction
elm_scroller_single_direction_get(const Evas_Object *obj)
{
   ELM_SCROLLABLE_CHECK(obj, ELM_SCROLLER_SINGLE_DIRECTION_SOFT);

   return s_iface->single_direction_get(obj);
}

EAPI void
elm_scroller_region_get(const Evas_Object *obj,
                        Evas_Coord *x,
                        Evas_Coord *y,
                        Evas_Coord *w,
                        Evas_Coord *h)
{
   ELM_SCROLLABLE_CHECK(obj);

   if ((x) || (y)) s_iface->content_pos_get(obj, x, y);
   if ((w) || (h)) s_iface->content_viewport_size_get(obj, w, h);
}

EAPI void
elm_scroller_child_size_get(const Evas_Object *obj,
                            Evas_Coord *w,
                            Evas_Coord *h)
{
   ELM_SCROLLABLE_CHECK(obj);

   s_iface->content_size_get(obj, w, h);
}

EAPI void
elm_scroller_bounce_set(Evas_Object *obj,
                        Eina_Bool h_bounce,
                        Eina_Bool v_bounce)
{
   ELM_SCROLLABLE_CHECK(obj);

   s_iface->bounce_allow_set(obj, h_bounce, v_bounce);
}

EAPI void
elm_scroller_bounce_get(const Evas_Object *obj,
                        Eina_Bool *h_bounce,
                        Eina_Bool *v_bounce)
{
   ELM_SCROLLABLE_CHECK(obj);

   s_iface->bounce_allow_get(obj, h_bounce, v_bounce);
}

EAPI void
elm_scroller_origin_reverse_set(Evas_Object *obj,
                               Eina_Bool origin_x, Eina_Bool origin_y)
{
   ELM_SCROLLABLE_CHECK(obj);

   s_iface->origin_reverse_set(obj, origin_x, origin_y);
}

EAPI void
elm_scroller_origin_reverse_get(Evas_Object *obj,
                               Eina_Bool *origin_x, Eina_Bool *origin_y)
{
   ELM_SCROLLABLE_CHECK(obj);

   s_iface->origin_reverse_get(obj, origin_x, origin_y);
}

EAPI void
elm_scroller_page_relative_set(Evas_Object *obj,
                               double h_pagerel,
                               double v_pagerel)
{
   Evas_Coord pagesize_h, pagesize_v;

   ELM_SCROLLABLE_CHECK(obj);

   s_iface->paging_get(obj, NULL, NULL, &pagesize_h, &pagesize_v);

   s_iface->paging_set
     (obj, h_pagerel, v_pagerel, pagesize_h, pagesize_v);
}

EAPI void
elm_scroller_page_relative_get(const Evas_Object *obj,
                               double *h_pagerel,
                               double *v_pagerel)
{
   ELM_SCROLLABLE_CHECK(obj);

   s_iface->paging_get(obj, h_pagerel, v_pagerel, NULL, NULL);
}

EAPI void
elm_scroller_page_size_set(Evas_Object *obj,
                           Evas_Coord h_pagesize,
                           Evas_Coord v_pagesize)
{
   double pagerel_h, pagerel_v;

   ELM_SCROLLABLE_CHECK(obj);

   s_iface->paging_get(obj, &pagerel_h, &pagerel_v, NULL, NULL);

   s_iface->paging_set
     (obj, pagerel_h, pagerel_v, h_pagesize, v_pagesize);
}

EAPI void
elm_scroller_page_size_get(const Evas_Object *obj,
                           Evas_Coord *h_pagesize,
                           Evas_Coord *v_pagesize)
{
   ELM_SCROLLABLE_CHECK(obj);

   s_iface->paging_get(obj, NULL, NULL, h_pagesize, v_pagesize);
}

EAPI void
elm_scroller_page_scroll_limit_set(Evas_Object *obj,
                                   int page_limit_h,
                                   int page_limit_v)
{
   ELM_SCROLLABLE_CHECK(obj);

   if (page_limit_h < 1)
     page_limit_h = 9999;
   if (page_limit_v < 1)
     page_limit_v = 9999;

   s_iface->page_scroll_limit_set(obj, page_limit_h, page_limit_v);
}

EAPI void
elm_scroller_page_scroll_limit_get(Evas_Object *obj,
                                   int *page_limit_h,
                                   int *page_limit_v)
{
   ELM_SCROLLABLE_CHECK(obj);

   s_iface->page_scroll_limit_get(obj, page_limit_h, page_limit_v);
}

EAPI void
elm_scroller_current_page_get(const Evas_Object *obj,
                              int *h_pagenumber,
                              int *v_pagenumber)
{
   ELM_SCROLLABLE_CHECK(obj);

   s_iface->current_page_get(obj, h_pagenumber, v_pagenumber);
}

EAPI void
elm_scroller_last_page_get(const Evas_Object *obj,
                           int *h_pagenumber,
                           int *v_pagenumber)
{
   ELM_SCROLLABLE_CHECK(obj);

   s_iface->last_page_get(obj, h_pagenumber, v_pagenumber);
}

EAPI void
elm_scroller_page_show(Evas_Object *obj,
                       int h_pagenumber,
                       int v_pagenumber)
{
   ELM_SCROLLABLE_CHECK(obj);

   s_iface->page_show(obj, h_pagenumber, v_pagenumber);
}

EAPI void
elm_scroller_page_bring_in(Evas_Object *obj,
                           int h_pagenumber,
                           int v_pagenumber)
{
   ELM_SCROLLABLE_CHECK(obj);

   s_iface->page_bring_in(obj, h_pagenumber, v_pagenumber);
}

EAPI void
elm_scroller_region_bring_in(Evas_Object *obj,
                             Evas_Coord x,
                             Evas_Coord y,
                             Evas_Coord w,
                             Evas_Coord h)
{
   ELM_SCROLLABLE_CHECK(obj);

   s_iface->region_bring_in(obj, x, y, w, h);
}

EAPI void
elm_scroller_gravity_set(Evas_Object *obj,
                         double x,
                         double y)
{
   ELM_SCROLLABLE_CHECK(obj);

   s_iface->gravity_set(obj, x, y);
}

EAPI void
elm_scroller_gravity_get(const Evas_Object *obj,
                         double *x,
                         double *y)
{
   ELM_SCROLLABLE_CHECK(obj);

   s_iface->gravity_get(obj, x, y);
}

EAPI void
elm_scroller_wheel_disabled_set(Evas_Object *obj,
                                Eina_Bool disabled)
{
   ELM_SCROLLABLE_CHECK(obj);

   s_iface->wheel_disabled_set(obj, disabled);
}

EAPI Eina_Bool
elm_scroller_wheel_disabled_get(const Evas_Object *obj)
{
   ELM_SCROLLABLE_CHECK(obj, EINA_FALSE);

   return s_iface->wheel_disabled_get(obj);
}

EAPI void
elm_scroller_movement_block_set(Evas_Object *obj,
                                Elm_Scroller_Movement_Block block)
{
   ELM_SCROLLABLE_CHECK(obj);

   s_iface->movement_block_set(obj, block);
}

EAPI Elm_Scroller_Movement_Block
elm_scroller_movement_block_get(const Evas_Object *obj)
{
   ELM_SCROLLABLE_CHECK(obj, ELM_SCROLLER_MOVEMENT_NO_BLOCK);

   return s_iface->movement_block_get(obj);
}

EAPI void
elm_scroller_loop_set(Evas_Object *obj,
                      Eina_Bool loop_h,
                      Eina_Bool loop_v)
{
   ELM_SCROLLABLE_CHECK(obj);
   ELM_SCROLLER_DATA_GET(obj, sd);

   int i;

   if (sd->loop_h == loop_h && sd->loop_v == loop_v) return;

   sd->loop_h = loop_h;
   sd->loop_v = loop_v;

   s_iface->loop_set(obj, loop_h, loop_v);

   if (sd->content)
     {
     if (sd->loop_h || sd->loop_v)
       {
          sd->s_iface->content_set(obj, NULL);
          _loop_content_set(obj, sd->content);

          if (sd->contents)
            {
               sd->s_iface->content_set(obj, sd->contents);
               elm_widget_sub_object_add(obj, sd->contents);
               elm_widget_on_show_region_hook_set(sd->contents, _show_region_hook, obj);
            }
       }
     else
       {
          for (i = 0 ; i < 3 ; i++)
            {
               if (sd->proxy_content[i])
                 {
                    evas_object_del(sd->proxy_content[i]);
                    sd->proxy_content[i]= NULL;
                 }
            }
       }
     }
   elm_layout_sizing_eval(obj);
}

EAPI void
elm_scroller_loop_get(const Evas_Object *obj,
                      Eina_Bool *loop_h,
                      Eina_Bool *loop_v)
{
   ELM_SCROLLABLE_CHECK(obj);

   s_iface->loop_get(obj, loop_h, loop_v);
}

EAPI void
elm_scroller_propagate_events_set(Evas_Object *obj,
                                  Eina_Bool propagation)
{
   Elm_Widget_Smart_Data *sd;

   ELM_SCROLLABLE_CHECK(obj);

   sd = evas_object_smart_data_get(obj);
   if (!sd) return;  /* just being paranoid */

   evas_object_propagate_events_set(sd->resize_obj, propagation);
}

EAPI Eina_Bool
elm_scroller_propagate_events_get(const Evas_Object *obj)
{
   Elm_Widget_Smart_Data *sd;

   ELM_SCROLLABLE_CHECK(obj, EINA_FALSE);

   sd = evas_object_smart_data_get(obj);
   if (!sd) return EINA_FALSE;  /* just being paranoid */

   return evas_object_propagate_events_get(sd->resize_obj);
}
