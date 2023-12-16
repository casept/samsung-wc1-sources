#include <X11/Xlib.h>
#include <Elementary.h>
#include "elm_priv.h"
#include "elm_interface_scrollable.h"

static const char ACCESS_SMART_NAME[] = "elm_access";

static const char SIG_HIGHLIGHTED[] = "access,highlighted";
static const char SIG_UNHIGHLIGHTED[] = "access,unhighlighted";
static const char SIG_ACTIVATED[] = "access,activated";
static const char SIG_READ_START[] = "access,read,start";
static const char SIG_READ_STOP[] = "access,read,stop";
static const char SIG_READ_CANCEL[] = "access,read,cancel";
static const char SIG_HIGHLIGHT_ENABLED[] = "access,highlight,enabled";
static const char SIG_HIGHLIGHT_DISABLED[] = "access,highlight,disabled";

static const Evas_Smart_Cb_Description _smart_callbacks[] = {
   {SIG_HIGHLIGHTED, ""},
   {SIG_UNHIGHLIGHTED, ""},
   {SIG_ACTIVATED, ""},
   {SIG_READ_START, ""},
   {SIG_READ_STOP, ""},
   {SIG_READ_CANCEL, ""},
   {SIG_HIGHLIGHT_ENABLED, ""},
   {SIG_HIGHLIGHT_DISABLED, ""},
   {NULL, NULL}
};

ELM_INTERNAL_SMART_SUBCLASS_NEW
  (ACCESS_SMART_NAME, _elm_access, Elm_Widget_Smart_Class,
  Elm_Widget_Smart_Class, elm_widget_smart_class_get, _smart_callbacks);

struct _Func_Data
{
   void                *user_data; /* Holds user data to CB */
   Elm_Access_Action_Cb cb;
};

typedef struct _Func_Data Func_Data;

struct _Action_Info
{
   Evas_Object      *obj;
   Func_Data         fn[ELM_ACCESS_ACTION_LAST + 1]; /* Callback for specific action */
};

typedef struct _Action_Info Action_Info;

static Eina_Bool highlight_read_enable = EINA_FALSE;
static Eina_List *highlight_read_block_objects_list = NULL;

static Eina_Bool access_disable = EINA_TRUE;
static Eina_Bool mouse_event_enable = EINA_TRUE;
static Eina_Bool auto_highlight = EINA_FALSE;
static Eina_Bool reading_cancel = EINA_FALSE;
static Eina_Bool focus_chain_end = EINA_FALSE;
static Eina_Bool force_saying = EINA_FALSE;
static Evas_Coord_Point offset;
static Evas_Object *scroller = NULL;
static Evas_Object *clipper = NULL;
static Evas_Object *h_base_obj; /* highlight base object */
static Elm_Access_Action_Type action_by = ELM_ACCESS_ACTION_FIRST;
static Ecore_Timer *highlight_read_timer = NULL;
static Ecore_Job *clip_job = NULL;
static Eina_List *s_parents = NULL;
static char *waiting_text = NULL;

static Evas_Object * _elm_access_add(Evas_Object *parent);
static void _access_scroll_highlight_next(Evas_Object *obj);
static Eina_Bool _access_highlight_next_get(Evas_Object *obj, Elm_Access_Action_Type type, Evas_Object **target);
static Eina_Bool _access_highlight_next(Evas_Object *obj, Elm_Access_Action_Type dir, Eina_Bool delay);
static void _access_read_obj_del_cb(void *data, Evas *e, Evas_Object *obj, void *event_info);

static void
_elm_access_smart_add(Evas_Object *obj)
{
   EVAS_SMART_DATA_ALLOC(obj, Elm_Widget_Smart_Data);
   ELM_WIDGET_CLASS(_elm_access_parent_sc)->base.add(obj);

   elm_widget_can_focus_set(obj, EINA_FALSE);
}

static Eina_Bool
_elm_access_smart_on_focus(Evas_Object *obj, Elm_Focus_Info *info __UNUSED__)
{
   evas_object_focus_set(obj, elm_widget_focus_get(obj));

   return EINA_TRUE;
}

static Eina_Bool
_access_highlight_end_is(Evas_Object *obj, Elm_Access_Action_Type type)
{
   Evas_Object *parent, *target = NULL;
   Elm_Focus_Direction fdir = ELM_FOCUS_NONE;
   Elm_Highlight_Direction hdir;
   Eina_Bool ret;
   Elm_Access_Info *ac;

   fdir = ELM_FOCUS_NONE;
   hdir = ELM_ACCESS_SOUND_FIRST;
   ret = EINA_FALSE;

   if (type == ELM_ACCESS_ACTION_HIGHLIGHT_NEXT)
     {
        hdir = ELM_HIGHLIGHT_DIR_NEXT;
        fdir = ELM_FOCUS_NEXT;
     }
   else if (type == ELM_ACCESS_ACTION_HIGHLIGHT_PREV)
     {
        hdir = ELM_HIGHLIGHT_DIR_PREVIOUS;
        fdir = ELM_FOCUS_PREVIOUS;
     }

   ac = _elm_access_object_get(obj);
   if (ac && ac->end_dir == hdir)
     {
        ret = EINA_TRUE;
        return ret;
     }

   parent = obj;

   /* find highlight root */
   while (parent)
     {
        ELM_WIDGET_DATA_GET(parent, sd);
        if (sd->highlight_root)
          {
             /* change highlight root */
             obj = parent;
             break;
          }
        obj = parent;
        parent = elm_widget_parent_get(parent);
     }

   /* you don't have to set auto highlight here, it is set by caller */
   ret = elm_widget_focus_next_get(obj, fdir, &target);

   return !ret;
}

static Eina_Bool
_access_action_callback_call(Evas_Object *obj,
                             Elm_Access_Action_Type type,
                             Elm_Access_Action_Info *action_info)
{
   Elm_Access_Action_Info *ai = NULL;
   Action_Info *a;
   Eina_Bool ret;

   ret = EINA_FALSE;
   a = evas_object_data_get(obj, "_elm_access_action_info");

   if (type == ELM_ACCESS_ACTION_UNHIGHLIGHT) focus_chain_end = EINA_FALSE;

   if (!action_info)
     {
        ai = calloc(1, sizeof(Elm_Access_Action_Info));
        action_info = ai;
     }

   if (action_info)
     {
        action_info->action_type = type;

        if ((type == ELM_ACCESS_ACTION_HIGHLIGHT_NEXT) ||
            (type == ELM_ACCESS_ACTION_HIGHLIGHT_PREV))
          {
             if (a && (a->fn[type].cb))
               action_info->highlight_end = _access_highlight_end_is(obj, type);
          }
     }

   if (a && (a->fn[type].cb))
     ret = a->fn[type].cb(a->fn[type].user_data, obj, action_info);

   if (ai) free(ai);

   return ret;
}

static Eina_Bool
_elm_access_smart_activate(Evas_Object *obj, Elm_Activate act)
{
   Elm_Access_Info *ac;

   if (act != ELM_ACTIVATE_DEFAULT) return EINA_FALSE;

   ac = evas_object_data_get(obj, "_elm_access");
   if (!ac) return EINA_FALSE;

   if (ac->activate)
     ac->activate(ac->activate_data, ac->part_object,
                  (Elm_Object_Item *)ac->widget_item);

   return EINA_TRUE;
}

static void
_elm_access_smart_set_user(Elm_Widget_Smart_Class *sc)
{
   sc->base.add = _elm_access_smart_add;

   /* not a 'focus chain manager' */
   sc->focus_next = NULL;
   sc->focus_direction_manager_is = NULL;
   sc->focus_direction = NULL;
   sc->on_focus = _elm_access_smart_on_focus;
   sc->activate = _elm_access_smart_activate;

   return;
}

typedef struct _Mod_Api Mod_Api;

struct _Mod_Api
{
   void (*out_read) (const char *txt);
   void (*out_read_done) (void);
   void (*out_cancel) (void);
   void (*out_done_callback_set) (void (*func) (void *data), const void *data);
   void (*sound_play) (const Elm_Access_Sound_Type type);
};

static int initted = 0;
static Mod_Api *mapi = NULL;

static void
_access_init(void)
{
   Elm_Module *m;
   initted++;
   if (initted > 1) return;
   if (!(m = _elm_module_find_as("access/api"))) return;
   m->api = malloc(sizeof(Mod_Api));
   if (!m->api) return;
   m->init_func(m);
   ((Mod_Api *)(m->api)      )->out_read = // called to read out some text
      _elm_module_symbol_get(m, "out_read");
   ((Mod_Api *)(m->api)      )->out_read_done = // called to set a done marker so when it is reached the done callback is called
      _elm_module_symbol_get(m, "out_read_done");
   ((Mod_Api *)(m->api)      )->out_cancel = // called to read out some text
      _elm_module_symbol_get(m, "out_cancel");
   ((Mod_Api *)(m->api)      )->out_done_callback_set = // called when last read done
      _elm_module_symbol_get(m, "out_done_callback_set");
   ((Mod_Api *)(m->api)      )->sound_play = // play sound
      _elm_module_symbol_get(m, "sound_play");
   mapi = m->api;
}

static void
_access_shutdown(void)
{
   Elm_Module *m;
   if (initted == 0) return;
   if (!(m = _elm_module_find_as("access/api"))) return;

   m->shutdown_func(m);

   initted = 0;

   /* FIXME: _elm_module_unload(); could access m->api and try to free(); */
   free(m->api);
   m->api = NULL;
   mapi = NULL;
}

static Elm_Access_Item *
_access_add_set(Elm_Access_Info *ac, int type)
{
   Elm_Access_Item *ai;
   Eina_List *l;

   if (!ac) return NULL;
   EINA_LIST_FOREACH(ac->items, l, ai)
     {
        if (ai->type == type)
          {
             if (!ai->func)
               {
                  if (ai->data) eina_stringshare_del(ai->data);
               }
             ai->func = NULL;
             ai->data = NULL;
             return ai;
          }
     }
   ai = calloc(1, sizeof(Elm_Access_Item));
   ai->type = type;
   ac->items = eina_list_prepend(ac->items, ai);
   return ai;
}

static Evas_Object *
_access_highlight_object_get(Evas_Object *obj)
{
   Evas_Object *o, *ho;

   o = evas_object_name_find(evas_object_evas_get(obj), "_elm_access_disp");
   if (!o) return NULL;

   ho = evas_object_data_get(o, "_elm_access_target");

   return ho;
}

static void
_access_intersection_region_get(Evas_Coord *x, Evas_Coord *y,
                         Evas_Coord *w, Evas_Coord *h,
                         Evas_Coord sx, Evas_Coord sy,
                         Evas_Coord sw, Evas_Coord sh)
{
   Evas_Coord px = *x;
   Evas_Coord py = *y;
   *x = *x > sx ? *x : sx;
   *y = *y > sy ? *y : sy;
   *w = px + *w < sx + sw ? px + *w - *x : sx + sw - *x;
   *h = py + *h < sy + sh ? py + *h - *y : sy + sh - *y;
}

// for temporary blocking read and highlith on the specific object, use below apis.
// If you want to block drawing highlight ring and reading tts on the specific object, add the object that you want to block.
// If you want to release blocking, remove the object.
EAPI void
_access_highlight_read_block_object_add(Evas_Object* obj)
{
	highlight_read_block_objects_list = eina_list_append(highlight_read_block_objects_list, obj);
}

EAPI void
_access_highlight_read_block_object_remove(Evas_Object* obj)
{
	highlight_read_block_objects_list = eina_list_remove(highlight_read_block_objects_list, obj);
}

EAPI Eina_Bool
_access_highlight_is_read_block_object(Evas_Object* obj)
{
   const Evas_Object *subobj;
   const Eina_List *l;

   EINA_LIST_FOREACH(highlight_read_block_objects_list, l, subobj)
     {
        if (subobj == obj)
          return EINA_TRUE;
     }
   return EINA_FALSE;
}


static void
_access_highlight_read(Elm_Access_Info *ac, Evas_Object *obj)
{
   int type;
   char *txt = NULL;
   Eina_Strbuf *strbuf;

   if (!highlight_read_enable) return;
   if (_access_highlight_is_read_block_object(obj)) return;

   strbuf = eina_strbuf_new();
   if (_elm_config->access_mode != ELM_ACCESS_MODE_OFF)
     {
        if (ac->on_highlight) ac->on_highlight(ac->on_highlight_data);
        _elm_access_object_hilight(obj);

        // This function is not needed, if there's any side effect, remove this
        // elm_widget_focus_region_show(obj);

        for (type = ELM_ACCESS_INFO_FIRST + 1; type < ELM_ACCESS_INFO_LAST; type++)
          {
             txt = _elm_access_text_get(ac, type, obj);
             if (txt && (strlen(txt) > 0))
               {
                  int length = eina_strbuf_length_get(strbuf);
                  const char *pre_txt = eina_strbuf_string_get(strbuf);
                  char end = pre_txt[length-1];
                  if (length > 0 && end != '?' && end != '!' && end != '.')
                    eina_strbuf_append_printf(strbuf, ", %s", txt);
                  else
                    eina_strbuf_append_printf(strbuf, " %s", txt);
               }
             if (txt) free(txt);
          }
     }

   txt = eina_strbuf_string_steal(strbuf);
   eina_strbuf_free(strbuf);

   /* play highlight sound */
   _access_init();
   if (mapi && mapi->sound_play) mapi->sound_play(ELM_ACCESS_SOUND_HIGHLIGHT);

   _elm_access_say(obj, txt, EINA_FALSE);
   free(txt);
}

static Eina_Bool
_highlight_read_timeout_cb(void *data)
{
   Elm_Access_Info *ac;

   ac = evas_object_data_get(data, "_elm_access");
   if (!ac) goto end;

   _access_highlight_read(ac, data);

end:
   highlight_read_timer = NULL;
   return ECORE_CALLBACK_CANCEL;
}

static void
_access_hover_mouse_in_cb(void *data, Evas *e __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info  __UNUSED__)
{
   Elm_Access_Info *ac;

   if (!mouse_event_enable) return;
   _elm_access_mouse_event_enabled_set(EINA_FALSE);

   ac = evas_object_data_get(data, "_elm_access");
   if (!ac) return;

   if (_elm_config->access_mode)
     {
        if (highlight_read_timer)
          {
             ecore_timer_del(highlight_read_timer);
             highlight_read_timer = NULL;
          }

        /* could not call ecore_timer_add(); here, because the callback for READ
           action. The callback for READ action should be called even though an
           object has a highlight. elm_win calls _access_action_callback_call()
           for READ action. */
        action_by = ELM_ACCESS_ACTION_HIGHLIGHT;
        _access_highlight_read(ac, data);
     }
}

static void
_access_parent_callback_call(Evas_Object *obj, const char *sig, void *data)
{
   Elm_Access_Info *ac;
   ac = evas_object_data_get(obj, "_elm_access");
   if (ac && ac->parent)
     evas_object_smart_callback_call(ac->parent, sig, data);
}

static void
_access_read_done(void *data)
{
   Evas_Object *obj = data;
   if (!obj) goto end;

   if (!reading_cancel)
     {
        evas_object_smart_callback_call(obj, SIG_READ_STOP, NULL);
        _access_parent_callback_call(obj, SIG_READ_STOP, NULL);
     }
   else
     {
        evas_object_smart_callback_call(obj, SIG_READ_CANCEL, NULL);
        _access_parent_callback_call(obj, SIG_READ_CANCEL, NULL);
     }
   evas_object_event_callback_del_full(obj, EVAS_CALLBACK_DEL,
                                       _access_read_obj_del_cb, NULL);
 end:
   if (force_saying)
     {
        force_saying = EINA_FALSE;
        if (waiting_text)
          {
             _elm_access_say(NULL, waiting_text, EINA_FALSE);
             waiting_text = NULL;
          }
     }
}

static void
_access_2nd_click_del_cb(void *data __UNUSED__, Evas *e __UNUSED__, Evas_Object *obj, void *event_info __UNUSED__)
{
   Ecore_Timer *t;

   evas_object_event_callback_del_full(obj, EVAS_CALLBACK_DEL,
                                       _access_2nd_click_del_cb, NULL);
   t = evas_object_data_get(obj, "_elm_2nd_timeout");
   if (t)
     {
        ecore_timer_del(t);
        evas_object_data_del(obj, "_elm_2nd_timeout");
     }
}

static Eina_Bool
_access_2nd_click_timeout_cb(void *data)
{
   evas_object_event_callback_del_full(data, EVAS_CALLBACK_DEL,
                                       _access_2nd_click_del_cb, NULL);
   evas_object_data_del(data, "_elm_2nd_timeout");
   return EINA_FALSE;
}

static void
_access_obj_hilight_del_cb(void *data __UNUSED__, Evas *e, Evas_Object *obj, void *event_info __UNUSED__)
{
   _elm_access_object_hilight_disable(e);
   elm_widget_focus_restore(elm_widget_top_get(obj));
}

static void
_access_obj_hilight_hide_cb(void *data __UNUSED__, Evas *e, Evas_Object *obj, void *event_info __UNUSED__)
{
   _elm_access_object_hilight_disable(e);
   elm_widget_focus_restore(elm_widget_top_get(obj));
}

static Eina_List *
_access_scrollable_parents_get(Evas_Object *obj,
                               Evas_Coord *x,
                               Evas_Coord *y,
                               Evas_Coord *w,
                               Evas_Coord *h)
{
   Evas_Object *sp;
   Eina_List *l = NULL;
   Evas_Coord sx, sy, sw, sh;

   if (!obj) return NULL;

   if (x) *x = -49999;
   if (y) *y = -49999;
   if (w) *w = 99999;
   if (h) *h = 99999;

   sp = elm_widget_parent_get(obj);
   while (sp)
     {
        if(!!evas_object_smart_interface_get(sp, ELM_SCROLLABLE_IFACE_NAME))
          {
             l = eina_list_append(l, sp);
             evas_object_geometry_get(sp, &sx, &sy, &sw, &sh);
             if(x && y && w && h)
               _access_intersection_region_get(x, y, w, h, sx, sy, sw, sh);
          }
        sp = elm_widget_parent_get(sp);
     }
   return l;
}

static Evas_Object *
_access_scrollable_object_at_xy_get(Evas_Object *obj, Evas_Coord x, Evas_Coord y)
{
   Evas *evas;
   Evas_Object *so, *scr = NULL;
   Evas_Object *top_obj;
   Eina_List *l, *top_objects_list = NULL;

   evas = evas_object_evas_get(obj);
   if (!evas) return NULL;

   top_objects_list = evas_tree_objects_at_xy_get(evas, NULL, x, y);

   EINA_LIST_REVERSE_FOREACH(top_objects_list, l, top_obj)
     {
        so = top_obj;

        while (so)
          {
             if(elm_object_widget_check(so) &&
                !!evas_object_smart_interface_get(so, ELM_SCROLLABLE_IFACE_NAME))
               {
                  scr = so;
                  break;
               }
             so = evas_object_smart_parent_get(so);
          }
        if (scr) break;
     }

   return scr;
}

static void
_clipper_del_cb(void *data __UNUSED__, Evas *e __UNUSED__,
                 Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   clipper = NULL;
}

static void
_access_scrollable_parent_clip(void *data)
{
   Evas_Object *obj = data;
   Evas_Object *o;
   Eina_List *l;
   Evas_Coord x = 0, y = 0, w = 0, h = 0;

   clip_job = NULL;
   o = evas_object_name_find(evas_object_evas_get(obj), "_elm_access_disp");
   if (!o) return;

   l = _access_scrollable_parents_get(obj, &x, &y, &w, &h);
   if (l && w > 0 && h > 0)
     {
        if (evas_object_evas_get(obj) != evas_object_evas_get(clipper))
          {
             evas_object_del(clipper);
             clipper = NULL;
          }
        if (!clipper)
          {
             clipper = evas_object_rectangle_add(evas_object_evas_get(obj));
             evas_object_event_callback_add(clipper, EVAS_CALLBACK_DEL, _clipper_del_cb, NULL);
          }
        evas_object_move(clipper, x, y);
        evas_object_resize(clipper, w, h);
        evas_object_show(clipper);
        evas_object_clip_set(o, clipper);
     }
   else
     {
        evas_object_clip_set(o, NULL);
        if (clipper)
          evas_object_hide(clipper);
     }
   eina_list_free(l);
}

static void
_access_obj_scroll_virtual_value_get(Evas_Object *obj, Evas_Coord *vx, Evas_Coord *vy)
{
   Eina_Bool lh, lv;
   Evas_Coord x, y, sx, sy, vw, vh, cx, cy, cw, ch;
   Evas_Object *sp = NULL;
   Eina_List *l;

   *vx = 0;
   *vy = 0;

   l = _access_scrollable_parents_get(obj, NULL, NULL, NULL, NULL);
   if (l) sp = eina_list_data_get(l);
   eina_list_free(l);

   if (!sp) return;

   ELM_SCROLLABLE_IFACE_GET(sp, s_iface);
   s_iface->loop_get(sp, &lh, &lv);

   if (lh || lv)
     {
        s_iface->content_pos_get(sp, &cx, &cy);
        s_iface->content_size_get(sp, &cw, &ch);
        evas_object_geometry_get(obj, &x, &y, NULL, NULL);
        evas_object_geometry_get(sp, &sx, &sy, NULL, NULL);
        s_iface->content_viewport_size_get(sp, &vw, &vh);

        if ((cx > (cw - vw)) && (sx - x) > (cw - vw))
          *vx = cw;
        else
          *vx = 0;

        if ((cy > (ch - vh)) && (sy - y) > (ch - vh))
          *vy = ch;
        else
          *vy = 0;
     }
}

static void
_access_obj_hilight_move_cb(void *data __UNUSED__, Evas *e __UNUSED__, Evas_Object *obj, void *event_info __UNUSED__)
{
   Evas_Coord x, y, vx, vy;
   Evas_Object *o;

   o = evas_object_name_find(evas_object_evas_get(obj), "_elm_access_disp");
   if (!o) return;
   evas_object_geometry_get(obj, &x, &y, NULL, NULL);
   _access_obj_scroll_virtual_value_get(obj, &vx, &vy);
   x += vx;
   y += vy;
   evas_object_move(o, x, y);

   _access_scrollable_parent_clip(obj);
}

static void
_access_obj_hilight_resize_cb(void *data __UNUSED__, Evas *e __UNUSED__, Evas_Object *obj, void *event_info __UNUSED__)
{
   Evas_Coord w, h;
   Evas_Object *o;

   o = evas_object_name_find(evas_object_evas_get(obj), "_elm_access_disp");
   if (!o) return;
   evas_object_geometry_get(obj, NULL, NULL, &w, &h);
   evas_object_resize(o, w, h);

   if (clip_job) ecore_job_del(clip_job);
   clip_job = ecore_job_add(_access_scrollable_parent_clip, obj);
}

void
_elm_access_mouse_event_enabled_set(Eina_Bool enabled)
{
   enabled = !!enabled;
   if (mouse_event_enable == enabled) return;
   mouse_event_enable = enabled;
}

void
_elm_access_auto_highlight_set(Eina_Bool enabled)
{
   enabled = !!enabled;
   if (auto_highlight == enabled) return;
   auto_highlight = enabled;
}

Eina_Bool
_elm_access_auto_highlight_get(void)
{
   return auto_highlight;
}

void
_elm_access_shutdown()
{
   _access_shutdown();
}

static void
_access_order_del_cb(void *data,
                     Evas *e __UNUSED__,
                     Evas_Object *obj,
                     void *event_info __UNUSED__)
{
   Elm_Widget_Item *item = data;

   item->access_order = eina_list_remove(item->access_order, obj);
}

void
_elm_access_widget_item_access_order_set(Elm_Widget_Item *item,
                                              Eina_List *objs)
{
   Eina_List *l;
   Evas_Object *o;

   if (!item) return;

   _elm_access_widget_item_access_order_unset(item);

   EINA_LIST_FOREACH(objs, l, o)
     {
        evas_object_event_callback_add(o, EVAS_CALLBACK_DEL,
                                       _access_order_del_cb, item);
     }

   item->access_order = objs;
}

const Eina_List *
_elm_access_widget_item_access_order_get(const Elm_Widget_Item *item)
{
   if (!item) return NULL;
   return item->access_order;
}

void
_elm_access_widget_item_access_order_unset(Elm_Widget_Item *item)
{
   Eina_List *l, *l_next;
   Evas_Object *o;

   if (!item) return;

   EINA_LIST_FOREACH_SAFE(item->access_order, l, l_next, o)
     {
        evas_object_event_callback_del_full
          (o, EVAS_CALLBACK_DEL, _access_order_del_cb, item);
        item->access_order = eina_list_remove_list(item->access_order, l);
     }
}

static void
_access_s_parent_del_cb(void *data __UNUSED__, Evas *e __UNUSED__,
                 Evas_Object *obj, void *event_info __UNUSED__)
{
   Evas *evas;

   evas = evas_object_evas_get(obj);
   if (!evas) return;

   evas_event_feed_mouse_move(evas, INT_MIN, INT_MIN, ecore_loop_time_get() * 1000, NULL);
   evas_event_feed_mouse_up(evas, 1, EVAS_BUTTON_NONE, ecore_loop_time_get() * 1000, NULL);

   s_parents = eina_list_remove(s_parents, obj);
}

static void
_scroll_cb(void *data __UNUSED__ , Evas_Object *obj, void *event_info __UNUSED__)
{
   _access_scroll_highlight_next(obj);
}

static void
_scroll_repeat_events_set(Evas_Object *obj, Eina_Bool repeat_events)
{
   ELM_SCROLLABLE_IFACE_GET(obj, s_iface);
   s_iface->repeat_events_set(obj, repeat_events);
}

static Eina_Bool
_scroll_repeat_events_get(Evas_Object *obj)
{
   ELM_SCROLLABLE_IFACE_GET(obj, s_iface);
   return s_iface->repeat_events_get(obj);
}

static void
_scroll_anim_stop_cb(void *data __UNUSED__ , Evas_Object *obj, void *event_info __UNUSED__)
{
   Eina_List *l = NULL;
   Evas_Object *sp;

   if (_scroll_repeat_events_get(obj))
     {
        EINA_LIST_FOREACH(s_parents, l, sp)
          {
             evas_object_smart_callback_del(sp, "scroll", _scroll_cb);
             evas_object_smart_callback_del(sp, "scroll,anim,stop", _scroll_anim_stop_cb);
             _scroll_repeat_events_set(sp, EINA_TRUE);
          }
     }
}

static void
_access_scroll_highlight_next(Evas_Object *obj)
{
   Evas_Object *ho, *cur_ho;
   static Evas_Object *pre_ho = NULL;
   Evas_Coord hx, hy, hw, hh, sx, sy, sw, sh, vx, vy;
   Eina_List *new_s_parents, *l = NULL;
   Evas_Object *sp, *nsp;
   Eina_Bool done = EINA_FALSE;
   int direction = -1;
   Elm_Focus_Direction dir;
   int scroll_count_down = 0;
   int scroll_count_up = 0;

   if (!obj) return;
   if (!s_parents) return;

   cur_ho = ho = _access_highlight_object_get(obj);
   while (!done)
     {
        if (ho)
          {
             evas_object_geometry_get(ho, &hx, &hy, &hw, &hh);
             _access_obj_scroll_virtual_value_get(ho, &vx, &vy);
             hx += vx;
             hy += vy;

             l = _access_scrollable_parents_get(ho, &sx, &sy, &sw, &sh);
             if (!l) return;
             eina_list_free(l);

             if (hx + (hw / 2) < sx || hy + (hh / 2) < sy)
               {
                  if (direction < 0 || direction == ELM_ACCESS_ACTION_HIGHLIGHT_NEXT)
                    {
                       direction = ELM_ACCESS_ACTION_HIGHLIGHT_NEXT;
                       done = !_access_highlight_next_get(obj, ELM_ACCESS_ACTION_HIGHLIGHT_NEXT, &ho);
                       if (!done && !ho) ho = _access_highlight_object_get(obj);
                    }
                  else
                    {
                       done = !_access_highlight_next_get(obj, ELM_ACCESS_ACTION_HIGHLIGHT_PREV, &ho);
                       if (!done && !ho) ho = _access_highlight_object_get(obj);
                    }
                  scroll_count_down++;
               }
             else if (hx + (hw / 2) > sx + sw || hy + (hh / 2) > sy + sh)
               {
                  if (direction < 0 || direction == ELM_ACCESS_ACTION_HIGHLIGHT_PREV)
                    {
                       direction = ELM_ACCESS_ACTION_HIGHLIGHT_PREV;
                       done = !_access_highlight_next_get(obj, ELM_ACCESS_ACTION_HIGHLIGHT_PREV, &ho);
                       if (!done && !ho) ho = _access_highlight_object_get(obj);
                    }
                  else
                    {
                       done = !_access_highlight_next_get(obj, ELM_ACCESS_ACTION_HIGHLIGHT_NEXT, &ho);
                       if (!done && !ho) ho = _access_highlight_object_get(obj);
                    }
                  scroll_count_up++;
               }
             else
               done = EINA_TRUE;

             //prevent watchdog issue
             if (scroll_count_down > 50 || scroll_count_up > 50)
               {
                  return;
               }

             _elm_access_object_hilight(ho);
             /* scrollable parent could be changed when highlight object is
                changed. FIXME: scrollable parent could be null.. then? */
             new_s_parents = _access_scrollable_parents_get(ho, NULL, NULL, NULL, NULL);

             sp = eina_list_data_get(s_parents);
             nsp = eina_list_data_get(new_s_parents);
             if ((nsp) && (sp != nsp))
               {
                  EINA_LIST_FOREACH(s_parents, l, sp)
                    {
                       evas_object_event_callback_del(sp, EVAS_CALLBACK_DEL,
                                                      _access_s_parent_del_cb);
                       evas_object_smart_callback_del(sp, "scroll", _scroll_cb);
                       evas_object_smart_callback_del(sp, "scroll,anim,stop", _scroll_anim_stop_cb);
                    }
                  EINA_LIST_FOREACH(new_s_parents, l, nsp)
                    {
                       evas_object_event_callback_add(nsp, EVAS_CALLBACK_DEL,
                                                      _access_s_parent_del_cb, NULL);
                       evas_object_smart_callback_add(nsp, "scroll", _scroll_cb, NULL);
                       evas_object_smart_callback_add(nsp, "scroll,anim,stop", _scroll_anim_stop_cb, NULL);
                    }
                  eina_list_free(s_parents);
                  s_parents = new_s_parents;
               }

             if (cur_ho == ho) done = EINA_TRUE;
          }
        else
          done = EINA_TRUE;
     }

      scroll_count_down = 0;
      scroll_count_up = 0;

   dir = (direction == ELM_ACCESS_ACTION_HIGHLIGHT_NEXT) ? ELM_FOCUS_NEXT : ELM_FOCUS_PREVIOUS;

   if (!_elm_access_widget_highlight(ho, dir))
     {
        if (cur_ho != ho && pre_ho != ho)
          {
             pre_ho = ho;
             _elm_access_highlight_set(ho, EINA_TRUE);
          }
     }
}

void
_elm_access_highlight_object_scroll(Evas_Object *obj, int type, int x, int y)
{
   Evas *evas;
   Eina_List *l = NULL;
   Evas_Object *ho, *sp;

   if (!obj) return;

   evas = evas_object_evas_get(obj);
   if (!evas) return;

   switch (type)
     {
      case 0:
         if (s_parents)
           {
              EINA_LIST_FOREACH(s_parents, l, sp)
                {
                   evas_object_event_callback_del(sp, EVAS_CALLBACK_DEL,
                                                  _access_s_parent_del_cb);
                   evas_object_smart_callback_del(sp, "scroll", _scroll_cb);
                   evas_object_smart_callback_del(sp, "scroll,anim,stop", _scroll_anim_stop_cb);
                }
              eina_list_free(s_parents);
              s_parents = NULL;
           }

         ho = _access_highlight_object_get(obj);
         if (ho)
           {
              /* find scrollable parents */
              s_parents = _access_scrollable_parents_get(ho, NULL, NULL, NULL, NULL);

              if (s_parents)
                {
                   EINA_LIST_FOREACH(s_parents, l, sp)
                     {
                        evas_object_event_callback_add(sp, EVAS_CALLBACK_DEL,
                                                       _access_s_parent_del_cb, NULL);
                        evas_object_smart_callback_add(sp, "scroll", _scroll_cb, NULL);
                        evas_object_smart_callback_add(sp, "scroll,anim,stop", _scroll_anim_stop_cb, NULL);
                     }
                }
           }

         scroller = _access_scrollable_object_at_xy_get(obj, x, y);
         if (scroller)
           {
              _scroll_repeat_events_set(scroller, EINA_FALSE);

              evas_event_feed_mouse_in(evas, ecore_loop_time_get() * 1000, NULL);
              evas_event_feed_mouse_move(evas, x, y, ecore_loop_time_get() * 1000, NULL);
              evas_event_feed_mouse_down(evas, 1, EVAS_BUTTON_NONE, ecore_loop_time_get() * 1000, NULL);
           }
         break;

      case 1:
         if (scroller)
           evas_event_feed_mouse_move(evas, x, y, ecore_loop_time_get() * 1000, NULL);
         break;

      case 2:
         if (scroller)
           {
              evas_event_feed_mouse_up(evas, 1, EVAS_BUTTON_NONE, ecore_loop_time_get() * 1000, NULL);
              _scroll_repeat_events_set(scroller, EINA_TRUE);
              scroller = NULL;
           }
         break;

      default:
         break;
     }
}

static unsigned int
_elm_access_win_angle_get(Ecore_X_Window win)
{
   Ecore_X_Window root;

   if (!win) return 0;

   int ret;
   int count;
   int angle = 0;
   unsigned char *prop_data = NULL;

   root = ecore_x_window_root_get(win);
   ret = ecore_x_window_prop_property_get(root,
                                          ECORE_X_ATOM_E_ILLUME_ROTATE_ROOT_ANGLE,
                                          ECORE_X_ATOM_CARDINAL,
                                          32, &prop_data, &count);

   if (ret && prop_data)
     memcpy(&angle, prop_data, sizeof(int));

   if (prop_data) free(prop_data);

   return angle;
}

static void
_elm_access_coordinate_calibrate(Ecore_X_Window win, int *x, int *y)
{
   int tx, ty, w, h;
   unsigned int angle;

   if (!x) return;
   if (!y) return;

   angle = _elm_access_win_angle_get(win);
   ecore_x_window_geometry_get(win, NULL, NULL, &w, &h);

   tx = *x;
   ty = *y;

   switch (angle)
     {
      case 90:
         *x = ty;
         *y = h - tx;
         break;

      case 180:
         *x = w - tx;
         *y = h - ty;
         break;

      case 270:
         *x = w - ty;
         *y = tx;
         break;

      default:
         break;
     }
}

static Eina_Bool
_elm_access_mouse_move_send(Ecore_X_Window win,
                        int x,
                        int y)
{
   XEvent xev;
   XWindowAttributes att;
   Window tw;
   int rx, ry;
   Ecore_X_Display *_ecore_x_disp = ecore_x_display_get();

   _elm_access_coordinate_calibrate(win, &x, &y);
   XGetWindowAttributes(_ecore_x_disp, win, &att);
   XTranslateCoordinates(_ecore_x_disp, win, att.root, x, y, &rx, &ry, &tw);
   xev.xmotion.type = MotionNotify;
   xev.xmotion.window = win;
   xev.xmotion.root = att.root;
   xev.xmotion.subwindow = win;
   xev.xmotion.time = ecore_loop_time_get() * 1000;
   xev.xmotion.x = x;
   xev.xmotion.y = y;
   xev.xmotion.x_root = rx;
   xev.xmotion.y_root = ry;
   xev.xmotion.state = 0;
   xev.xmotion.is_hint = 0;
   xev.xmotion.same_screen = 1;
   return XSendEvent(_ecore_x_disp, win, True, PointerMotionMask, &xev) ? EINA_TRUE : EINA_FALSE;
}

static Eina_Bool
_elm_access_mouse_down_send(Ecore_X_Window win,
                        int x,
                        int y,
                        int b)
{
   XEvent xev;
   XWindowAttributes att;
   Window tw;
   int rx, ry;
   Ecore_X_Display *_ecore_x_disp = ecore_x_display_get();

   _elm_access_coordinate_calibrate(win, &x, &y);
   XGetWindowAttributes(_ecore_x_disp, win, &att);
   XTranslateCoordinates(_ecore_x_disp, win, att.root, x, y, &rx, &ry, &tw);
   xev.xbutton.type = ButtonPress;
   xev.xbutton.window = win;
   xev.xbutton.root = att.root;
   xev.xbutton.subwindow = win;
   xev.xbutton.time = ecore_loop_time_get() * 1000;
   xev.xbutton.x = x;
   xev.xbutton.y = y;
   xev.xbutton.x_root = rx;
   xev.xbutton.y_root = ry;
   xev.xbutton.state = 1 << b;
   xev.xbutton.button = b;
   xev.xbutton.same_screen = 1;
   return XSendEvent(_ecore_x_disp, win, True, ButtonPressMask, &xev) ? EINA_TRUE : EINA_FALSE;
}

static Eina_Bool
_elm_access_mouse_up_send(Ecore_X_Window win,
                      int x,
                      int y,
                      int b)
{
   XEvent xev;
   XWindowAttributes att;
   Window tw;
   int rx, ry;
   Ecore_X_Display *_ecore_x_disp = ecore_x_display_get();

   _elm_access_coordinate_calibrate(win, &x, &y);
   XGetWindowAttributes(_ecore_x_disp, win, &att);
   XTranslateCoordinates(_ecore_x_disp, win, att.root, x, y, &rx, &ry, &tw);
   xev.xbutton.type = ButtonRelease;
   xev.xbutton.window = win;
   xev.xbutton.root = att.root;
   xev.xbutton.subwindow = win;
   xev.xbutton.time = ecore_loop_time_get() * 1000;
   xev.xbutton.x = x;
   xev.xbutton.y = y;
   xev.xbutton.x_root = rx;
   xev.xbutton.y_root = ry;
   xev.xbutton.state = 0;
   xev.xbutton.button = b;
   xev.xbutton.same_screen = 1;
   return XSendEvent(_ecore_x_disp, win, True, ButtonReleaseMask, &xev) ? EINA_TRUE : EINA_FALSE;
}

static Eina_Bool
_elm_access_mouse_in_send(Ecore_X_Window win,
                      int x,
                      int y)
{
   XEvent xev;
   XWindowAttributes att;
   Window tw;
   int rx, ry;
   Ecore_X_Display *_ecore_x_disp = ecore_x_display_get();

   _elm_access_coordinate_calibrate(win, &x, &y);
   XGetWindowAttributes(_ecore_x_disp, win, &att);
   XTranslateCoordinates(_ecore_x_disp, win, att.root, x, y, &rx, &ry, &tw);
   xev.xcrossing.type = EnterNotify;
   xev.xcrossing.window = win;
   xev.xcrossing.root = att.root;
   xev.xcrossing.subwindow = win;
   xev.xcrossing.time = ecore_loop_time_get() * 1000;
   xev.xcrossing.x = x;
   xev.xcrossing.y = y;
   xev.xcrossing.x_root = rx;
   xev.xcrossing.y_root = ry;
   xev.xcrossing.mode = NotifyNormal;
   xev.xcrossing.detail = NotifyNonlinear;
   xev.xcrossing.same_screen = 1;
   xev.xcrossing.focus = 0;
   xev.xcrossing.state = 0;
   return XSendEvent(_ecore_x_disp, win, True, EnterWindowMask, &xev) ? EINA_TRUE : EINA_FALSE;
}

void
_elm_access_highlight_object_mouse(Evas_Object *obj, int type, int x, int y)
{
   Evas *evas;
   Evas_Object *ho, *win;
   Evas_Coord_Rectangle ho_area;

#ifdef HAVE_ELEMENTARY_X
   Ecore_X_Window xwin = 0;
#endif

   if (!obj) return;

   evas = evas_object_evas_get(obj);
   if (!evas) return;

   switch (type)
     {
      case 0:
        ho = _access_highlight_object_get(obj);
        if (!ho)
          {
             h_base_obj = NULL;
             return;
          }
        else
          {
             h_base_obj = ho;
             evas_object_geometry_get
               (ho, &ho_area.x, &ho_area.y, &ho_area.w, &ho_area.h);

             offset.x = x - (ho_area.x + (ho_area.w / 2));
             offset.y = y - (ho_area.y + (ho_area.h / 2));
          }

#ifdef HAVE_ELEMENTARY_X
        win = elm_widget_top_get(ho);
        xwin = elm_win_xwindow_get(win);
        if (!xwin) return;

        x = x - offset.x;
        y = y - offset.y;
        _elm_access_mouse_in_send(xwin, x, y);
        _elm_access_mouse_move_send(xwin, x, y);
        _elm_access_mouse_down_send(xwin, x, y, 1);
#endif
        break;

      case 1:
        if (!h_base_obj) return;

#ifdef HAVE_ELEMENTARY_X
        win = elm_widget_top_get(h_base_obj);
        xwin = elm_win_xwindow_get(win);
        if (!xwin) return;

        _elm_access_mouse_move_send(xwin, x - offset.x, y - offset.y);
#endif
        break;

      case 2:
        if (!h_base_obj) return;

#ifdef HAVE_ELEMENTARY_X
        win = elm_widget_top_get(h_base_obj);
        xwin = elm_win_xwindow_get(win);
        if (!xwin) return;

        _elm_access_mouse_up_send(xwin, x - offset.x, y - offset.y, 1);
        h_base_obj = NULL;
#endif
        break;

      default:
        break;
     }
}
static void
_highlight_next_get_internal(const Evas_Object *obj,
                            Elm_Highlight_Direction dir,
                            Evas_Object **next)
{
   Elm_Access_Info *info;
   Evas_Object *comming = NULL;
   *next = NULL;

   info = _elm_access_object_get(obj);
   if (!info) return;

   if (dir == ELM_HIGHLIGHT_DIR_NEXT)
     comming = info->next;
   else
     comming = info->prev;

   if (!comming) return;
   else
     {
        if (!evas_object_visible_get(comming)
            || (elm_widget_tree_unfocusable_get(comming)))
          _highlight_next_get_internal(comming, dir, next);
        else
          *next = comming;
     }
}

static Eina_Bool
_access_widget_highlight(Evas_Object *obj, Elm_Focus_Direction dir)
{
   Evas_Object *parent, *o;

   if (!obj) return EINA_FALSE;

   parent = obj;
   while (parent)
     {
        o = elm_widget_parent_get(parent);
        if (!o) break;
        if (elm_widget_tree_unfocusable_get(o)) return EINA_FALSE;
        parent = o;
     }

   parent = obj;
   while (parent)
     {
        /* if an widget works for highlighting by itself, then give a chance to
           the widget to highlight. the widget should return EINA_TRUE, when it
           has its own work for highlighting. */
        if (_elm_access_widget_highlight(parent, dir)) return EINA_TRUE;

        ELM_WIDGET_DATA_GET(parent, sd);
        if (sd->highlight_root) break;

        parent = elm_widget_parent_get(parent);
     }

   return EINA_FALSE;
}

static Eina_Bool
_access_highlight_next_get(Evas_Object *obj, Elm_Access_Action_Type type, Evas_Object **target)
{
   Evas_Object *ho, *parent;
   Elm_Focus_Direction dir;
   Elm_Access_Info *ac;
   Eina_Bool ret;
   ret = EINA_FALSE;

   if (!target)
     return EINA_FALSE;

   *target = NULL;
   dir = ELM_FOCUS_NEXT;

   ho = _access_highlight_object_get(obj);

   parent = ho;

   /* find highlight root */
   while (parent)
     {
        ELM_WIDGET_DATA_GET(parent, sd);
        if (sd->highlight_root)
          {
             /* change highlight root */
             obj = parent;
             break;
          }
        parent = elm_widget_parent_get(parent);
     }

   _elm_access_auto_highlight_set(EINA_TRUE);

   action_by = type;

   if (!_access_action_callback_call(ho, type, NULL))
     {
        if (ho)
          {
             if (type == ELM_ACCESS_ACTION_HIGHLIGHT_NEXT)
               _highlight_next_get_internal(ho,
                                            ELM_HIGHLIGHT_DIR_NEXT,
                                            target);
             else
               _highlight_next_get_internal(ho,
                                            ELM_HIGHLIGHT_DIR_PREVIOUS,
                                            target);
          }

        if (type == ELM_ACCESS_ACTION_HIGHLIGHT_NEXT)
          dir = ELM_FOCUS_NEXT;
        else if (type == ELM_ACCESS_ACTION_HIGHLIGHT_PREV)
          dir = ELM_FOCUS_PREVIOUS;

        if (*target)
          {
             elm_widget_focus_next_get(*target, dir, target);
             ret = EINA_TRUE;
          }
        else
          {
             if (_access_widget_highlight(ho, dir))
               {
                  /* return EINA_TRUE with *target = NULL */
                  ret = EINA_TRUE;
               }
             else
               ret = elm_widget_focus_next_get(obj, dir, target);
          }
     }
   else
     {
       /* The object does something for highlight next, prev */
       ac = _elm_access_object_get(ho);
       if (ac) ac->action_relay = EINA_TRUE;
     }

   _elm_access_auto_highlight_set(EINA_FALSE);

   return ret;
}

static Eina_Bool
_access_highlight_next(Evas_Object *obj, Elm_Access_Action_Type type, Eina_Bool delay)
{
   Evas_Object *target;
   Eina_Bool ret;

   ret = _access_highlight_next_get(obj, type, &target);

   if (ret && target)
     {
        _elm_access_highlight_set(target, delay);
        elm_widget_focus_region_show(target);
     }
   return ret;
}

void
_elm_access_highlight_read_enable_set(Evas_Object *obj, Eina_Bool enabled, Eina_Bool access_enable)
{
   Evas *evas;
   Eina_List *l = NULL;
   Evas_Object *o, *sp;
   if (!obj) return;

   enabled = !!enabled;
   access_enable = !!access_enable;

   if (access_enable)
     {
        if(!enabled) access_disable = EINA_TRUE;
        else access_disable = EINA_FALSE;
     }
   else if (access_disable) return;

   if (highlight_read_enable == enabled) return;
   highlight_read_enable = enabled;

   if (highlight_read_enable)
     {
        elm_widget_focus_restore(obj);
        evas_object_smart_callback_call(obj, SIG_HIGHLIGHT_ENABLED, NULL);
     }
   else
     {
        if (s_parents)
          {
             sp = eina_list_data_get(s_parents);
             _scroll_repeat_events_set(sp, EINA_TRUE);

             EINA_LIST_FOREACH(s_parents, l, sp)
               {
                  evas_object_event_callback_del(sp, EVAS_CALLBACK_DEL,
                                                 _access_s_parent_del_cb);
                  evas_object_smart_callback_del(sp, "scroll", _scroll_cb);
                  evas_object_smart_callback_del(sp, "scroll,anim,stop", _scroll_anim_stop_cb);
               }
             eina_list_free(s_parents);
             s_parents = NULL;
          }
        evas = evas_object_evas_get(obj);
        o = evas_object_name_find(evas, "_elm_access_disp");
        if (o)
          {
             if (clip_job)
               ecore_job_del(clip_job);
             clip_job = NULL;
             if (clipper)
               evas_object_hide(clipper);
             evas_object_hide(o);
          }
        evas_event_feed_mouse_move(evas, INT_MIN, INT_MIN, ecore_loop_time_get() * 1000, NULL);
        evas_event_feed_mouse_up(evas, 1, EVAS_BUTTON_NONE, ecore_loop_time_get() * 1000, NULL);
        evas_object_smart_callback_call(obj, SIG_HIGHLIGHT_DISABLED, NULL);
     }
}

void
_elm_access_all_read_stop(void)
{
   _access_init();
   if (mapi)
     {
        if (mapi->out_done_callback_set)
           mapi->out_done_callback_set(NULL, NULL);
     }
}

static void
_access_all_read_done(void *data)
{
   Eina_Bool ret;
   ret = _access_highlight_next(data, ELM_FOCUS_NEXT, EINA_FALSE);

   if (!ret) _elm_access_all_read_stop();
}

void
_elm_access_all_read_start(Evas_Object *obj)
{
   int type;
   Evas_Object *ho, *parent, *target;
   Eina_Bool ret;

   target = NULL;
   ret = EINA_FALSE;

   ho = _access_highlight_object_get(obj);
   parent = ho;

   /* find highlight root */
   while (parent)
     {
        ELM_WIDGET_DATA_GET(parent, sd);
        if (sd->highlight_root)
          {
             /* change highlight root */
             obj = parent;
             break;
          }
        parent = elm_widget_parent_get(parent);
     }

   if (ho) _elm_access_object_unhilight(ho);

   _access_init();
   if (mapi)
     {
        if (mapi->out_done_callback_set)
           mapi->out_done_callback_set(_access_all_read_done, obj);
     }

   _elm_access_auto_highlight_set(EINA_TRUE);

   ret = elm_widget_focus_next_get(obj, ELM_FOCUS_NEXT, &target);
   if (ret && target)
     {
        type = ELM_ACCESS_ACTION_HIGHLIGHT_NEXT;

        if (!_access_action_callback_call(ho, type, NULL))
          {
             /* this value is used in _elm_access_object_highlight();
                to inform the target object of how to get highlight */
             action_by = type;

             _elm_access_highlight_set(target, EINA_FALSE);
          }
     }

   _elm_access_auto_highlight_set(EINA_FALSE);

   if (!ret) _elm_access_all_read_stop();
}

Eina_Bool
_elm_access_widget_highlight(Evas_Object *obj, Elm_Focus_Direction dir)
{
   Elm_Access_Info *ac;
   Elm_Highlight_Direction hdir;

   ac = evas_object_data_get(obj, "_elm_access");
   if (!ac) return EINA_FALSE;
   if (!ac->widget_highlight) return EINA_FALSE;

   if (dir == ELM_FOCUS_PREVIOUS)
     hdir = ELM_HIGHLIGHT_DIR_PREVIOUS;
   else if (dir == ELM_FOCUS_NEXT)
     hdir = ELM_HIGHLIGHT_DIR_NEXT;
   else
     return EINA_FALSE;

   return ac->widget_highlight(obj, hdir);
}
//-------------------------------------------------------------------------//
EAPI void
_elm_access_highlight_set(Evas_Object* obj, Eina_Bool delay)
{
   Evas_Object *parent;
   Elm_Access_Info *ac;

   /* evas_event_feed_mouse_move(); could cause unexpected result. so check
      whether the access mode is enabled or not. */
   if (!_elm_config->access_mode) return;
   if (!obj) return;

   /* check tree focus allow (tree unfocusable) */
   parent = obj;
   while (parent)
     {
        ELM_WIDGET_DATA_GET(parent, sd);
        if (sd && sd->tree_unfocusable) return;
        parent = elm_widget_parent_get(parent);
     }

   ac = evas_object_data_get(obj, "_elm_access");
   if (!ac) return;

   elm_widget_highlight_steal(obj);
   if (highlight_read_timer)
     {
        ecore_timer_del(highlight_read_timer);
        highlight_read_timer = NULL;
     }
   /* use ecore_timer_add(); here, an object could have a highlight even though
      its text is not yet translated in case of the naviframe title */
   if (delay)
     highlight_read_timer = ecore_timer_add(0.1, _highlight_read_timeout_cb, obj);
   else
     _access_highlight_read(ac, obj);
}

EAPI void
_elm_access_clear(Elm_Access_Info *ac)
{
   Elm_Access_Item *ai;

   if (!ac) return;

   EINA_LIST_FREE(ac->items, ai)
     {
        if (!ai->func)
          {
             if (ai->data) eina_stringshare_del(ai->data);
          }
        free(ai);
     }
}

EAPI void
_elm_access_text_set(Elm_Access_Info *ac, int type, const char *text)
{
   Elm_Access_Item *ai = _access_add_set(ac, type);
   if (!ai) return;
   ai->func = NULL;
   ai->data = eina_stringshare_add(text);
}

EAPI void
_elm_access_callback_set(Elm_Access_Info *ac, int type, Elm_Access_Info_Cb func, const void *data)
{
   Elm_Access_Item *ai = _access_add_set(ac, type);
   if (!ai) return;
   ai->func = func;
   ai->data = data;
}

EAPI void
_elm_access_on_highlight_hook_set(Elm_Access_Info           *ac,
                                  Elm_Access_On_Highlight_Cb func,
                                  void                      *data)
{
    if (!ac) return;
    ac->on_highlight = func;
    ac->on_highlight_data = data;
}

EAPI void
_elm_access_activate_callback_set(Elm_Access_Info           *ac,
                                  Elm_Access_Activate_Cb     func,
                                  void                      *data)
{
   if (!ac) return;
   ac->activate = func;
   ac->activate_data = data;
}

EAPI void
_elm_access_highlight_object_activate(Evas_Object *obj, Elm_Activate act)
{
   Evas_Object *ho;
   Evas_Object* parent_obj;

   ho = _access_highlight_object_get(obj);
   if (!ho) return;

   switch (act)
     {
      case ELM_ACTIVATE_DEFAULT:
        if (!elm_object_focus_get(ho))
        {
          if (!elm_widget_can_focus_get(ho))
            {
               parent_obj = elm_widget_parent_get(ho);
               elm_widget_focus_steal(parent_obj);
            }
          else
            {
               elm_object_focus_set(ho, EINA_TRUE);
            }
        }

        // When the access is activated, it reads the infomation again.
        _elm_access_highlight_set(ho, EINA_TRUE);

        elm_widget_activate(ho, act);
        evas_object_smart_callback_call(ho, SIG_ACTIVATED, NULL);
        _access_parent_callback_call(obj, SIG_ACTIVATED, NULL);
        break;

      case ELM_ACTIVATE_UP:
      case ELM_ACTIVATE_DOWN:
      case ELM_ACTIVATE_BACK:
        elm_widget_activate(ho, act);
        break;

      default:
        break;
     }

   return;
}

EAPI void
_elm_access_highlight_cycle(Evas_Object *obj, Elm_Access_Action_Type type)
{
   Evas_Object *ho, *parent, *target = NULL;
   Elm_Highlight_Direction dir;
   Eina_Bool ret;
   Elm_Access_Info *ac;

   ho = _access_highlight_object_get(obj);

   parent = ho;

   ac = _elm_access_object_get(ho);

   dir = (type == ELM_ACCESS_ACTION_HIGHLIGHT_NEXT) ?
      ELM_HIGHLIGHT_DIR_NEXT : ELM_HIGHLIGHT_DIR_PREVIOUS;

   /* find highlight root */
   while (parent)
     {
        ELM_WIDGET_DATA_GET(parent, sd);
        if (sd->highlight_root)
          {
             /* change highlight root */
             obj = parent;
             break;
          }
        if ((!focus_chain_end ||
             (sd->highlight_previous && dir == ELM_HIGHLIGHT_DIR_PREVIOUS) ||
             (sd->highlight_next && dir == ELM_HIGHLIGHT_DIR_NEXT)) &&
            sd->end_dir == dir)
          {
             obj = parent;
             break;
          }
        parent = elm_widget_parent_get(parent);
     }

   if (ac && ac->end_dir == dir && !focus_chain_end && !ac->action_relay)
     {
        target = ho;
        ret = EINA_FALSE;
     }
   else
     {
        ret = _access_highlight_next_get(obj, type, &target);
     }

   if (target)
     {
        if (ac && ac->action_relay) ret = EINA_TRUE;

        if (ret)
          focus_chain_end = !ret;
        else
          focus_chain_end = !focus_chain_end;

        if (!focus_chain_end)
          {
             _elm_access_highlight_set(target, EINA_TRUE);
             elm_widget_focus_region_show(target);
          }
        else
          {
             /* play highlight sound */
             _access_init();
             INF("[ScreenReader] end sound");
             printf("[ScreenReader] end sound\n");
             if (mapi && mapi->sound_play) mapi->sound_play(ELM_ACCESS_SOUND_END);
          }
     }
}

EAPI char *
_elm_access_text_get(const Elm_Access_Info *ac, int type, const Evas_Object *obj)
{
   Elm_Access_Item *ai;
   Eina_List *l;

   if (!ac) return NULL;
   EINA_LIST_FOREACH(ac->items, l, ai)
     {
        if (ai->type == type)
          {
             if (ai->func) return ai->func((void *)(ai->data), (Evas_Object *)obj);
             else if (ai->data) return strdup(ai->data);
             return NULL;
          }
     }
   return NULL;
}

EAPI void
_elm_access_read(Elm_Access_Info *ac, int type, const Evas_Object *obj)
{
   char *txt = _elm_access_text_get(ac, type, obj);

   if (!txt) return;
   if (strlen(txt) == 0)/* Tizen only: TTS engine does not work properly */
     {
         free(txt);
         return;
     }

   _access_init();
   if (mapi)
     {
        if (mapi->out_done_callback_set)
           mapi->out_done_callback_set(_access_read_done, NULL);
        if (type == ELM_ACCESS_DONE)
          {
             if (mapi->out_read_done) mapi->out_read_done();
          }
        else if (type == ELM_ACCESS_CANCEL)
          {
             reading_cancel = EINA_TRUE;
             if (mapi->out_cancel) mapi->out_cancel();
             reading_cancel = EINA_FALSE;
          }
        else
          {
             if (txt && mapi->out_read) mapi->out_read(txt);
          }
     }
   if (txt) free(txt);
}

static void
_access_read_obj_del_cb(void *data __UNUSED__, Evas *e __UNUSED__, Evas_Object *obj, void *event_info __UNUSED__)
{
   if (mapi && mapi->out_done_callback_set)
     mapi->out_done_callback_set(NULL, obj);
}

EAPI void
_elm_access_say(Evas_Object *obj, const char *txt, Eina_Bool force)
{
   char *out_text = NULL;
   if (!_elm_config->access_mode) return;
   if (!highlight_read_enable && !force) return;

   if (!txt) return; /* Tizen only: TTS engine does not work properly */
   if (strlen(txt) == 0) return;

   if (!force)
     {
        if (highlight_read_timer && !force)
          {
             ecore_timer_del(highlight_read_timer);
             highlight_read_timer = NULL;
          }

        if (force_saying)
          {
             waiting_text = strdup(txt);
             return;
          }
     }

   _access_init();
   if (mapi)
     {
        reading_cancel = EINA_TRUE;
        if (mapi->out_cancel) mapi->out_cancel();
        reading_cancel = EINA_FALSE;

        evas_object_smart_callback_call(obj, SIG_READ_START, NULL);
        _access_parent_callback_call(obj, SIG_READ_START, NULL);
        if (mapi->out_read)
          {
             if (txt)
               {
                  out_text = evas_textblock_text_markup_to_utf8(NULL, txt);
                  ERR("[ScreenReader] %s", out_text);
                  printf("[ScreenReader] %s\n", out_text);
               }
             mapi->out_read(out_text);
          }
        //if (mapi->out_read) mapi->out_read(".\n"); /* TIZEN only: Tizen TTS engine performance is not good */
        evas_object_event_callback_add(obj, EVAS_CALLBACK_DEL,
                                       _access_read_obj_del_cb, NULL);
        if (mapi->out_done_callback_set)
          mapi->out_done_callback_set(_access_read_done, obj);

        if (mapi->out_read_done) mapi->out_read_done();
     }
}

EAPI Elm_Access_Info *
_elm_access_object_get(const Evas_Object *obj)
{
   return evas_object_data_get(obj, "_elm_access");
}

static Evas_Object *
_elm_access_widget_target_get(Evas_Object *obj)
{
   Evas_Object *o = obj;

   do
     {
        if (elm_widget_is(o))
          break;
        else
          {
             o = elm_widget_parent_widget_get(o);
             if (!o)
               o = evas_object_smart_parent_get(o);
          }
     }
   while (o);

   return o;
}

EAPI void
_elm_access_object_hilight(Evas_Object *obj)
{
   Evas_Object *o, *widget, *ptarget = NULL;
   Evas_Coord x, y, w, h, vx, vy;
   Elm_Access_Action_Info *a;
   Eina_Bool in_theme = EINA_FALSE;
   Elm_Access_Info *ac, *nac;

   o = evas_object_name_find(evas_object_evas_get(obj), "_elm_access_disp");
   if (!o)
     {
        /* edje_object_add(); calls evas_event_feed_mouse_move();
           and it calls _access_hover_mouse_in_cb(); again. */
        _elm_access_mouse_event_enabled_set(EINA_FALSE);
        o = edje_object_add(evas_object_evas_get(obj));
        evas_object_name_set(o, "_elm_access_disp");
        //evas_object_layer_set(o, ELM_OBJECT_LAYER_TOOLTIP);
     }

   ptarget = evas_object_data_get(o, "_elm_access_target");

   ac = evas_object_data_get(ptarget, "_elm_access");
   nac = evas_object_data_get(obj, "_elm_access");

   if (ptarget != obj)
     {
        if (ptarget)
          {
             evas_object_smart_member_del(o);
             evas_object_data_del(o, "_elm_access_target");

             evas_object_event_callback_del_full(ptarget, EVAS_CALLBACK_DEL,
                                                 _access_obj_hilight_del_cb, NULL);
             evas_object_event_callback_del_full(ptarget, EVAS_CALLBACK_HIDE,
                                                 _access_obj_hilight_hide_cb, NULL);
             evas_object_event_callback_del_full(ptarget, EVAS_CALLBACK_MOVE,
                                                 _access_obj_hilight_move_cb, NULL);
             evas_object_event_callback_del_full(ptarget, EVAS_CALLBACK_RESIZE,
                                                 _access_obj_hilight_resize_cb, NULL);
             _access_action_callback_call(ptarget, ELM_ACCESS_ACTION_UNHIGHLIGHT, NULL);

             widget = _elm_access_widget_target_get(ptarget);
             if (widget)
               {
                  if (elm_widget_access_highlight_in_theme_get(widget))
                    {
                       elm_widget_signal_emit(widget, "elm,action,access_highlight,hide", "elm");
                    }
               }

             if (nac && nac->hoverobj)
               {
                  DBG("nac hoverobj type:%s", evas_object_type_get(nac->hoverobj));
                  if(!strcmp(evas_object_type_get(nac->hoverobj), "image")      ||
                      !strcmp(evas_object_type_get(nac->hoverobj), "rectangle") ||
                      !strcmp(evas_object_type_get(nac->hoverobj), "textblock") ||
                      !strcmp(evas_object_type_get(nac->hoverobj), "text"))
                    {
                       DBG("[ScreenReader] %s object can't be added to member of smart object. so skipped", evas_object_type_get(nac->hoverobj));
                    }
                  else
                    {
                        evas_object_smart_member_add(o, nac->hoverobj);
                        evas_object_smart_changed(nac->hoverobj);

                        if (ac && ac->hoverobj)
                          {
                             DBG("ac hoverobj type:%s", evas_object_type_get(ac->hoverobj));
                             if(!strcmp(evas_object_type_get(ac->hoverobj), "image")      ||
                                 !strcmp(evas_object_type_get(ac->hoverobj), "rectangle") ||
                                 !strcmp(evas_object_type_get(ac->hoverobj), "textblock") ||
                                 !strcmp(evas_object_type_get(ac->hoverobj), "text"))
                                  DBG("[ScreenReader] %s object can't be called smart changed. so skipped", evas_object_type_get(ac->hoverobj));
                             else
                               {
                                  evas_object_smart_changed(ac->hoverobj);
                               }
                          }
                    }
               }
          }
        evas_object_data_set(o, "_elm_access_target", obj);
        elm_widget_highlight_steal(obj);

        elm_widget_theme_object_set(obj, o, "access", "base", "default");

        evas_object_event_callback_add(obj, EVAS_CALLBACK_DEL,
                                       _access_obj_hilight_del_cb, NULL);
        evas_object_event_callback_add(obj, EVAS_CALLBACK_HIDE,
                                       _access_obj_hilight_hide_cb, NULL);
        evas_object_event_callback_add(obj, EVAS_CALLBACK_MOVE,
                                       _access_obj_hilight_move_cb, NULL);
        evas_object_event_callback_add(obj, EVAS_CALLBACK_RESIZE,
                                       _access_obj_hilight_resize_cb, NULL);
     }
   evas_object_raise(o);
   evas_object_geometry_get(obj, &x, &y, &w, &h);
   _access_obj_scroll_virtual_value_get(obj, &vx, &vy);
   x += vx;
   y += vy;
   evas_object_move(o, x, y);
   evas_object_resize(o, w, h);

   _access_scrollable_parent_clip(obj);

   /* use callback, should an access object do below every time when
    * a window gets a client message ECORE_X_ATOM_E_ILLMUE_ACTION_READ? */
   a = calloc(1, sizeof(Elm_Access_Action_Info));
   a->action_by = action_by;
   widget = _elm_access_widget_target_get(obj);
   if (widget)
     {
        if (elm_widget_access_highlight_in_theme_get(widget))
          {
             in_theme = EINA_TRUE;
             elm_widget_signal_emit(widget, "elm,action,access_highlight,show", "elm");
          }
     }
   if (!in_theme &&
       !_access_action_callback_call(obj, ELM_ACCESS_ACTION_HIGHLIGHT, a))
     evas_object_show(o);
   else
     {
        if (clip_job)
          ecore_job_del(clip_job);
        clip_job = NULL;
        if (clipper)
          evas_object_hide(clipper);
        evas_object_hide(o);
     }
   free(a);

   if (ptarget != obj)
     {
        evas_object_smart_callback_call(ptarget, SIG_UNHIGHLIGHTED, NULL);
        evas_object_smart_callback_call(obj, SIG_HIGHLIGHTED, NULL);
     }

   if (ac && nac && ac->parent != nac->parent)
     {
        _access_parent_callback_call(ptarget, SIG_UNHIGHLIGHTED, NULL);
        _access_parent_callback_call(obj, SIG_HIGHLIGHTED, NULL);
     }
}

EAPI void
_elm_access_object_unhilight(Evas_Object *obj)
{
   Evas_Object *o, *ptarget;

   o = evas_object_name_find(evas_object_evas_get(obj), "_elm_access_disp");
   if (!o) return;
   ptarget = evas_object_data_get(o, "_elm_access_target");
   if (ptarget == obj)
     {
        evas_object_event_callback_del_full(ptarget, EVAS_CALLBACK_DEL,
                                            _access_obj_hilight_del_cb, NULL);
        evas_object_event_callback_del_full(ptarget, EVAS_CALLBACK_HIDE,
                                            _access_obj_hilight_hide_cb, NULL);
        evas_object_event_callback_del_full(ptarget, EVAS_CALLBACK_MOVE,
                                            _access_obj_hilight_move_cb, NULL);
        evas_object_event_callback_del_full(ptarget, EVAS_CALLBACK_RESIZE,
                                            _access_obj_hilight_resize_cb, NULL);
        if (clip_job)
          ecore_job_del(clip_job);
        clip_job = NULL;
        if (clipper)
          evas_object_hide(clipper);
        evas_object_del(o);
        elm_widget_parent_highlight_set(ptarget, EINA_FALSE);
        _access_action_callback_call(ptarget, ELM_ACCESS_ACTION_UNHIGHLIGHT, NULL);
     }
}

static void
_content_resize(void *data, Evas *e __UNUSED__, Evas_Object *obj,
                void *event_info __UNUSED__)
{
   Evas_Object *accessobj;
   Evas_Coord w, h;

   accessobj = data;
   if (!accessobj) return;

   evas_object_geometry_get(obj, NULL, NULL, &w, &h);
   evas_object_resize(accessobj, w, h);
}

static void
_content_move(void *data, Evas *e __UNUSED__, Evas_Object *obj,
              void *event_info __UNUSED__)
{
   Evas_Object *accessobj;
   Evas_Coord x, y;

   accessobj = data;
   if (!accessobj) return;

   evas_object_geometry_get(obj, &x, &y, NULL, NULL);
   evas_object_move(accessobj, x, y);
}

static Evas_Object *
_access_object_register(Evas_Object *obj, Evas_Object *parent)
{
   Evas_Object *ao;
   Elm_Access_Info *ac;
   Evas_Coord x, y, w, h;

   if (!obj) return NULL;

   /* create access object */
   ao = _elm_access_add(parent);
   if (!ao) return NULL;

   evas_object_event_callback_add(obj, EVAS_CALLBACK_RESIZE,
                                  _content_resize, ao);
   evas_object_event_callback_add(obj, EVAS_CALLBACK_MOVE,
                                  _content_move, ao);

   evas_object_geometry_get(obj, &x, &y, &w, &h);
   evas_object_move(ao, x, y);
   evas_object_resize(ao, w, h);
   evas_object_show(ao);

   /* register access object */
   _elm_access_object_register(ao, obj);

   /* set access object */
   evas_object_data_set(obj, "_part_access_obj", ao);

   /* set owner part object */
   ac = evas_object_data_get(ao, "_elm_access");
   ac->part_object = obj;
   ac->parent = parent;

   return ao;
}

static void
_access_object_unregister(Evas_Object *obj)
{
   Elm_Access_Info *ac;
   Evas_Object *ao;

   if (!obj) return;

   ao = evas_object_data_get(obj, "_part_access_obj");

   if (ao)
     {
        /* delete callbacks and unregister access object in _access_obj_del_cb*/
        evas_object_del(ao);
     }
   else
     {
        /* button, check, label etc. */
        ac = evas_object_data_get(obj, "_elm_access");
        if (ac && ac->hoverobj)
          _elm_access_object_unregister(obj, ac->hoverobj);
     }
}

EAPI Evas_Object *
_elm_access_edje_object_part_object_register(Evas_Object* obj,
                                             const Evas_Object *eobj,
                                             const char* part)
{
   Evas_Object *ao, *po;

   po = (Evas_Object *)edje_object_part_object_get(eobj, part);
   if (!obj || !po) return NULL;

   /* check previous access object */
   ao = evas_object_data_get(po, "_part_access_obj");
   if (ao)
     _elm_access_edje_object_part_object_unregister(obj, eobj, part);

   ao = _access_object_register(po, obj);

   return ao;
}

//FIXME: unused obj should be removed from here and each widget.
EAPI void
_elm_access_edje_object_part_object_unregister(Evas_Object* obj __UNUSED__,
                                               const Evas_Object *eobj,
                                               const char* part)
{
   Evas_Object *po;

   po = (Evas_Object *)edje_object_part_object_get(eobj, part);
   if (!po) return;

   _access_object_unregister(po);
}

EAPI void
_elm_access_object_hilight_disable(Evas *e)
{
   Evas_Object *o, *ptarget;

   o = evas_object_name_find(e, "_elm_access_disp");
   if (!o) return;
   ptarget = evas_object_data_get(o, "_elm_access_target");
   if (ptarget)
     {
        evas_object_event_callback_del_full(ptarget, EVAS_CALLBACK_DEL,
                                            _access_obj_hilight_del_cb, NULL);
        evas_object_event_callback_del_full(ptarget, EVAS_CALLBACK_HIDE,
                                            _access_obj_hilight_hide_cb, NULL);
        evas_object_event_callback_del_full(ptarget, EVAS_CALLBACK_MOVE,
                                            _access_obj_hilight_move_cb, NULL);
        evas_object_event_callback_del_full(ptarget, EVAS_CALLBACK_RESIZE,
                                            _access_obj_hilight_resize_cb, NULL);
        elm_widget_parent_highlight_set(ptarget, EINA_FALSE);
        if (mapi && mapi->out_done_callback_set)
           mapi->out_done_callback_set(NULL, ptarget);
        _access_action_callback_call(ptarget, ELM_ACCESS_ACTION_UNHIGHLIGHT, NULL);
     }
   if (clip_job)
     ecore_job_del(clip_job);
   clip_job = NULL;
   if (clipper)
     evas_object_hide(clipper);
   evas_object_del(o);
}

static void
_access_obj_del_cb(void *data __UNUSED__, Evas *e __UNUSED__, Evas_Object *obj, void *event_info __UNUSED__)
{
   Elm_Access_Info *ac;
   Ecore_Job *ao_del_job = NULL;

   evas_object_event_callback_del(obj, EVAS_CALLBACK_DEL, _access_obj_del_cb);

   ac = evas_object_data_get(obj, "_elm_access");

   if (ac && ac->hoverobj) /* hover object */
     {
        evas_object_event_callback_del_full(ac->hoverobj, EVAS_CALLBACK_RESIZE,
                                            _content_resize, obj);
        evas_object_event_callback_del_full(ac->hoverobj, EVAS_CALLBACK_MOVE,
                                            _content_move, obj);

        _elm_access_object_unregister(obj, ac->hoverobj);
     }

   if (clip_job)
     ecore_job_del(clip_job);
   clip_job = NULL;

   ao_del_job = evas_object_data_get(obj, "_access_obj_del_job");

   if (ao_del_job)
     {
        ecore_job_del(ao_del_job);
        evas_object_data_del(obj, "_access_obj_del_job");
     }
}

static void
_access_obj_del_job(void *data)
{
   if (!data) return;

   evas_object_data_del(data, "_access_obj_del_job");

   evas_object_event_callback_del(data, EVAS_CALLBACK_DEL, _access_obj_del_cb);

   if (mapi && mapi->out_done_callback_set)
     mapi->out_done_callback_set(NULL, data);

   evas_object_del(data);
}

static void
_access_hover_del_cb(void *data, Evas *e __UNUSED__, Evas_Object *obj, void *event_info __UNUSED__)
{
   Ecore_Job *ao_del_job = NULL;

   /* data - access object - could be NULL */
   if (!data) return;

   evas_object_event_callback_del_full(obj, EVAS_CALLBACK_RESIZE,
                                       _content_resize, data);
   evas_object_event_callback_del_full(obj, EVAS_CALLBACK_MOVE,
                                       _content_move, data);

   _elm_access_object_unregister(data, obj);

   /* delete access object in job */
   ao_del_job = evas_object_data_get(data, "_access_obj_del_job");
   if (ao_del_job)
     {
        ecore_job_del(ao_del_job);
        evas_object_data_del(data, "_access_obj_del_job");
     }

   ao_del_job = ecore_job_add(_access_obj_del_job, data);
   evas_object_data_set(data, "_access_obj_del_job", ao_del_job);
}

static void
_access_hoverobj_hide_cb(void *data __UNUSED__, Evas *e __UNUSED__, Evas_Object *obj, void *event_info __UNUSED__)
{
   Evas_Object *ao;

   ao = evas_object_data_get(obj, "_part_access_obj");
   if (ao) evas_object_hide(ao);
}

static void
_access_hoverobj_show_cb(void *data __UNUSED__, Evas *e __UNUSED__, Evas_Object *obj, void *event_info __UNUSED__)
{
   Evas_Object *ao;

   ao = evas_object_data_get(obj, "_part_access_obj");
   if (ao) evas_object_show(ao);
}

void
_elm_access_object_hover_unregister(Evas_Object *ao)
{
   Elm_Access_Info * ac;
   Evas_Object *hoverobj;

   ac = evas_object_data_get(ao, "_elm_access");
   if (!ac) return;

   hoverobj = ac->hoverobj;

   evas_object_data_del(hoverobj, "_part_access_obj");

   evas_object_event_callback_del_full(hoverobj, EVAS_CALLBACK_RESIZE,
                                       _content_resize, ao);
   evas_object_event_callback_del_full(hoverobj, EVAS_CALLBACK_MOVE,
                                       _content_move, ao);

   evas_object_event_callback_del_full(hoverobj, EVAS_CALLBACK_MOUSE_IN,
                                       _access_hover_mouse_in_cb, ao);
   evas_object_event_callback_del_full(hoverobj, EVAS_CALLBACK_DEL,
                                       _access_hover_del_cb, ao);
   evas_object_event_callback_del_full(hoverobj, EVAS_CALLBACK_HIDE,
                                       _access_hoverobj_hide_cb, ao);
   evas_object_event_callback_del_full(hoverobj, EVAS_CALLBACK_SHOW,
                                       _access_hoverobj_show_cb, ao);

   ac->hoverobj = NULL;
}

void
_elm_access_object_hover_register(Evas_Object *ao, Evas_Object *hoverobj)
{
   Elm_Access_Info * ac;

   evas_object_data_set(hoverobj, "_part_access_obj", ao);

   evas_object_event_callback_add(hoverobj, EVAS_CALLBACK_RESIZE,
                                  _content_resize, ao);
   evas_object_event_callback_add(hoverobj, EVAS_CALLBACK_MOVE,
                                  _content_move, ao);

   evas_object_event_callback_add(hoverobj, EVAS_CALLBACK_MOUSE_IN,
                                  _access_hover_mouse_in_cb, ao);
   evas_object_event_callback_add(hoverobj, EVAS_CALLBACK_DEL,
                                  _access_hover_del_cb, ao);
   evas_object_event_callback_add(hoverobj, EVAS_CALLBACK_HIDE,
                                  _access_hoverobj_hide_cb, ao);
   evas_object_event_callback_add(hoverobj, EVAS_CALLBACK_SHOW,
                                  _access_hoverobj_show_cb, ao);

   ac = evas_object_data_get(ao, "_elm_access");
   if (!ac) return;

   ac->hoverobj = hoverobj;
}

void
_elm_access_sound_play(const Elm_Access_Sound_Type type)
{
   switch (type)
     {
      case ELM_ACCESS_SOUND_HIGHLIGHT:
      case ELM_ACCESS_SOUND_SCROLL:
      case ELM_ACCESS_SOUND_END:
        _access_init();
	if (type == ELM_ACCESS_SOUND_HIGHLIGHT)
	{
		INF("[ScreenReader] highlight sound");
		printf("[ScreenReader] highlight sound\n");
	}
	else if (type == ELM_ACCESS_SOUND_SCROLL)
	{
		INF("[ScreenReader] scroll sound");
		printf("[ScreenReader] scroll sound\n");
	}
	else if (type == ELM_ACCESS_SOUND_END)
	{
		INF("[ScreenReader] end sound");
		printf("[ScreenReader] end sound\n");
	}

        if (mapi && mapi->sound_play) mapi->sound_play(type);
        break;

      default:
        break;
     }
}

EAPI void
_elm_access_object_register(Evas_Object *obj, Evas_Object *hoverobj)
{
   Elm_Access_Info *ac;

   evas_object_event_callback_add(hoverobj, EVAS_CALLBACK_MOUSE_IN,
                                  _access_hover_mouse_in_cb, obj);
   evas_object_event_callback_add(hoverobj, EVAS_CALLBACK_DEL,
                                  _access_hover_del_cb, obj);
   evas_object_event_callback_add(hoverobj, EVAS_CALLBACK_HIDE,
                                  _access_hoverobj_hide_cb, obj);
   evas_object_event_callback_add(hoverobj, EVAS_CALLBACK_SHOW,
                                  _access_hoverobj_show_cb, obj);

   evas_object_event_callback_add(obj, EVAS_CALLBACK_DEL,
                                  _access_obj_del_cb, NULL);


   ac = calloc(1, sizeof(Elm_Access_Info));
   evas_object_data_set(obj, "_elm_access", ac);
   ac->end_dir = ELM_HIGHLIGHT_DIR_FIRST;

   ac->hoverobj = hoverobj;
}

EAPI void
_elm_access_object_unregister(Evas_Object *obj, Evas_Object *hoverobj)
{
   Elm_Access_Info *ac;
   Evas_Object *ao;

   evas_object_event_callback_del_full(hoverobj, EVAS_CALLBACK_MOUSE_IN,
                                       _access_hover_mouse_in_cb, obj);
   evas_object_event_callback_del_full(hoverobj, EVAS_CALLBACK_DEL,
                                       _access_hover_del_cb, obj);
   evas_object_event_callback_del_full(hoverobj, EVAS_CALLBACK_HIDE,
                                       _access_hoverobj_hide_cb, obj);
   evas_object_event_callback_del_full(hoverobj, EVAS_CALLBACK_SHOW,
                                       _access_hoverobj_show_cb, obj);

   /* _access_obj_del_cb and _access_hover_del_cb calls this function,
      both do not need _part_access_obj data, so delete the data here. */
   ao = evas_object_data_get(hoverobj, "_part_access_obj");
   if (ao) evas_object_data_del(hoverobj, "_part_access_obj");

   ac = evas_object_data_get(obj, "_elm_access");
   evas_object_data_del(obj, "_elm_access");
   if (ac)
     {
        /* widget could delete VIEW(it) only and register item again,
           in this case _elm_access_widget_ntem_register could try to delet
           access object again in _elm_access_widget_item_unregister */
        if (ac->widget_item) ac->widget_item->access_obj = NULL;

        _elm_access_clear(ac);
        free(ac);
     }

   Action_Info *a;
   a = evas_object_data_get(obj, "_elm_access_action_info");
   evas_object_data_del(obj,  "_elm_access_action_info");
   if (a) free(a);
}

EAPI void
_elm_access_widget_item_register(Elm_Widget_Item *item)
{
   Evas_Object *ao, *ho;
   Evas_Coord x, y, w, h;
   Elm_Access_Info *ac;

   ELM_WIDGET_ITEM_CHECK_OR_RETURN(item);

   /* check previous access object */
   if (item->access_obj)
     _elm_access_widget_item_unregister(item);

   // create access object
   ho = item->view;
   ao = _elm_access_add(item->widget);
   if (!ao) return;

   evas_object_event_callback_add(ho, EVAS_CALLBACK_RESIZE,
                                  _content_resize, ao);
   evas_object_event_callback_add(ho, EVAS_CALLBACK_MOVE,
                                  _content_move, ao);

   evas_object_geometry_get(ho, &x, &y, &w, &h);
   evas_object_move(ao, x, y);
   evas_object_resize(ao, w, h);
   evas_object_show(ao);

   // register access object
   _elm_access_object_register(ao, ho);

   item->access_obj = ao;

   /* set owner widget item */
   ac = evas_object_data_get(ao, "_elm_access");
   ac->widget_item = item;
   ac->parent = item->widget;
}

EAPI void
_elm_access_widget_item_unregister(Elm_Widget_Item *item)
{
   Evas_Object *ao;

   ELM_WIDGET_ITEM_CHECK_OR_RETURN(item);

   if (!item->access_obj) return;

   /* delete callbacks and unregister access object in _access_obj_del_cb*/
   ao = item->access_obj;
   item->access_obj = NULL;

   evas_object_del(ao);
}

EAPI Eina_Bool
_elm_access_2nd_click_timeout(Evas_Object *obj)
{
   Ecore_Timer *t;

   t = evas_object_data_get(obj, "_elm_2nd_timeout");
   if (t)
     {
        ecore_timer_del(t);
        evas_object_data_del(obj, "_elm_2nd_timeout");
        evas_object_event_callback_del_full(obj, EVAS_CALLBACK_DEL,
                                            _access_2nd_click_del_cb, NULL);
        return EINA_TRUE;
     }
   t = ecore_timer_add(0.3, _access_2nd_click_timeout_cb, obj);
   evas_object_data_set(obj, "_elm_2nd_timeout", t);
   evas_object_event_callback_add(obj, EVAS_CALLBACK_DEL,
                                  _access_2nd_click_del_cb, NULL);
   return EINA_FALSE;
}

static Evas_Object *
_elm_access_add(Evas_Object *parent)
{
   Evas_Object *obj;

   EINA_SAFETY_ON_NULL_RETURN_VAL(parent, NULL);

   obj = elm_widget_add(_elm_access_smart_class_new(), parent);
   if (!obj) return NULL;

   if (!elm_widget_sub_object_add(parent, obj))
     ERR("could not add %p as sub object of %p", obj, parent);

   //Tizen Only: This should be removed when eo is applied.
   ELM_WIDGET_DATA_GET(obj, sd);
   sd->on_create = EINA_FALSE;

   return obj;
}

EAPI Evas_Object *
elm_access_object_register(Evas_Object *obj, Evas_Object *parent)
{
   return _access_object_register(obj, parent);
}

EAPI void
elm_access_object_unregister(Evas_Object *obj)
{
   _access_object_unregister(obj);
}

EAPI Evas_Object *
elm_access_object_get(const Evas_Object *obj)
{
   return evas_object_data_get(obj, "_part_access_obj");
}

EAPI void
elm_access_info_set(Evas_Object *obj, int type, const char *text)
{
   _elm_access_text_set(_elm_access_object_get(obj), type, text);
}

EAPI char *
elm_access_info_get(const Evas_Object *obj, int type)
{
   return _elm_access_text_get(_elm_access_object_get(obj), type, obj);
}

EAPI void
elm_access_info_cb_set(Evas_Object *obj, int type,
                          Elm_Access_Info_Cb func, const void *data)
{
   _elm_access_callback_set(_elm_access_object_get(obj), type, func, data);
}

EAPI void
elm_access_activate_cb_set(Evas_Object *obj,
                           Elm_Access_Activate_Cb  func, void *data)
{
   Elm_Access_Info *ac;

   ac = _elm_access_object_get(obj);
   if (!ac) return;

   ac->activate = func;
   ac->activate_data = data;
}

EAPI void
elm_access_say(const char *text)
{
   if (!text) return;

   _elm_access_say(NULL, text, EINA_FALSE);
}

EAPI void
elm_access_force_say(const char *text)
{
   if (!text) return;
   if (force_saying) return;

   force_saying = EINA_TRUE;
   _elm_access_say(NULL, text, EINA_TRUE);
}

EAPI void
elm_access_highlight_set(Evas_Object* obj)
{
   _elm_access_highlight_set(obj, EINA_FALSE);
}

EAPI void
elm_access_item_highlight_set(Elm_Object_Item *item)
{
   ELM_WIDGET_ITEM_CHECK_OR_RETURN((Elm_Widget_Item *)item);
   Evas_Object *ao = ((Elm_Widget_Item *)item)->access_obj;
   if (ao) _elm_access_highlight_set(ao, EINA_FALSE);
}

EAPI Eina_Bool
elm_access_action(Evas_Object *obj, const Elm_Access_Action_Type type, void *action_info)
{
   Evas *evas;
   Evas_Object *ho;
   Elm_Access_Action_Info *a;
   Eina_Bool ret = EINA_FALSE;
   Evas_Object *win;

   win = elm_widget_top_get(obj);
   if (win)
     {
        Elm_Win_Type win_type;
        win_type = elm_win_type_get(win);
        if (win_type == ELM_WIN_SOCKET_IMAGE || win_type == ELM_WIN_TIZEN_WIDGET)
          highlight_read_enable = EINA_TRUE;
     }

   if (!highlight_read_timer)
     action_by = ELM_ACCESS_ACTION_FIRST;

   a = (Elm_Access_Action_Info *) action_info;

   switch (type)
     {
      case ELM_ACCESS_ACTION_READ:
      case ELM_ACCESS_ACTION_HIGHLIGHT:
        evas = evas_object_evas_get(obj);
        if (!evas) return EINA_FALSE;

        evas_event_feed_mouse_in(evas, ecore_loop_time_get() * 1000, NULL);

        _elm_access_mouse_event_enabled_set(EINA_TRUE);
        evas_event_feed_mouse_move(evas, INT_MIN, INT_MIN, ecore_loop_time_get() * 1000, NULL);
        evas_event_feed_mouse_move(evas, a->x, a->y, ecore_loop_time_get() * 1000, NULL);
        _elm_access_mouse_event_enabled_set(EINA_FALSE);

        ho = _access_highlight_object_get(obj);
        if (ho)
          _access_action_callback_call(ho, ELM_ACCESS_ACTION_READ, a);
        break;

      case ELM_ACCESS_ACTION_OVER:
        evas = evas_object_evas_get(obj);
        if (!evas) return EINA_FALSE;

        evas_event_feed_mouse_in(evas, ecore_loop_time_get() * 1000, NULL);

        _elm_access_mouse_event_enabled_set(EINA_TRUE);
        evas_event_feed_mouse_move(evas, a->x, a->y, ecore_loop_time_get() * 1000, NULL);
        _elm_access_mouse_event_enabled_set(EINA_FALSE);

        ho = _access_highlight_object_get(obj);
        if (ho)
          _access_action_callback_call(ho, ELM_ACCESS_ACTION_OVER, a);
        break;


      case ELM_ACCESS_ACTION_UNHIGHLIGHT:
        evas = evas_object_evas_get(obj);
        if (!evas) return EINA_FALSE;
        _elm_access_object_hilight_disable(evas);
        break;

      case ELM_ACCESS_ACTION_HIGHLIGHT_NEXT:
      case ELM_ACCESS_ACTION_HIGHLIGHT_PREV:
        if (a && a->highlight_cycle)
          _elm_access_highlight_cycle(obj, type);
        else
          return _access_highlight_next(obj, type, EINA_TRUE);
        break;

      case ELM_ACCESS_ACTION_ACTIVATE:
        ho = _access_highlight_object_get(obj);
        if (ho)
          ret = _access_action_callback_call(ho, ELM_ACCESS_ACTION_ACTIVATE, a);

        if (!ret)
          _elm_access_highlight_object_activate(obj, ELM_ACTIVATE_DEFAULT);
        break;

      case ELM_ACCESS_ACTION_UP:
        ho = _access_highlight_object_get(obj);
        if (ho)
          ret = _access_action_callback_call(ho, ELM_ACCESS_ACTION_UP, a);

        if (!ret)
          _elm_access_highlight_object_activate(obj, ELM_ACTIVATE_UP);
        break;

      case ELM_ACCESS_ACTION_DOWN:
        ho = _access_highlight_object_get(obj);
        if (ho)
          ret = _access_action_callback_call(ho, ELM_ACCESS_ACTION_DOWN, a);

        if (!ret)
          _elm_access_highlight_object_activate(obj, ELM_ACTIVATE_DOWN);
        break;

      case ELM_ACCESS_ACTION_SCROLL:
        ho = _access_highlight_object_get(obj);
        if (ho)
          ret = _access_action_callback_call(ho, ELM_ACCESS_ACTION_SCROLL, a);

        if (!ret)
          _elm_access_highlight_object_scroll(obj, a->mouse_type, a->x, a->y);
        break;

      case ELM_ACCESS_ACTION_ZOOM:
        ho = _access_highlight_object_get(obj);
        if (ho)
          _access_action_callback_call(ho, ELM_ACCESS_ACTION_ZOOM, a);
        break;

      case ELM_ACCESS_ACTION_MOUSE:
        ho = _access_highlight_object_get(obj);
        if (ho)
          ret = _access_action_callback_call(ho, ELM_ACCESS_ACTION_MOUSE, a);

        if (!ret)
          _elm_access_highlight_object_mouse(obj, a->mouse_type, a->x, a->y);
        break;

      case ELM_ACCESS_ACTION_BACK:
        break;

      case ELM_ACCESS_ACTION_ENABLE:
        _elm_access_highlight_read_enable_set(obj, EINA_TRUE, EINA_TRUE);
        break;

      case ELM_ACCESS_ACTION_DISABLE:
        _elm_access_highlight_read_enable_set(obj, EINA_FALSE, EINA_TRUE);
        break;


      default:
        break;
     }

   return EINA_TRUE;
}

EAPI void
elm_access_action_cb_set(Evas_Object *obj, const Elm_Access_Action_Type type, const Elm_Access_Action_Cb cb, const void *data)
{
   Action_Info *a;
   a =  evas_object_data_get(obj, "_elm_access_action_info");

   if (!a)
     {
        a = calloc(1, sizeof(Action_Info));
        evas_object_data_set(obj, "_elm_access_action_info", a);
     }

   a->obj = obj;
   a->fn[type].cb = cb;
   a->fn[type].user_data = (void *)data;
}

EAPI void
elm_access_external_info_set(Evas_Object *obj, const char *text)
{
   _elm_access_text_set
     (_elm_access_object_get(obj), ELM_ACCESS_CONTEXT_INFO, text);
}

EAPI char *
elm_access_external_info_get(const Evas_Object *obj)
{
   Elm_Access_Info *ac;

   ac = _elm_access_object_get(obj);
   return _elm_access_text_get(ac, ELM_ACCESS_CONTEXT_INFO, obj);
}

EAPI void
elm_access_highlight_next_set(Evas_Object *obj, Elm_Highlight_Direction dir, Evas_Object *next)
{
   EINA_SAFETY_ON_NULL_RETURN(obj);
   EINA_SAFETY_ON_NULL_RETURN(next);

   Elm_Access_Info *info = _elm_access_object_get(obj);

   if (dir == ELM_HIGHLIGHT_DIR_NEXT)
     {
        if (info && info->part_object)
          info->next = next;
        else
          {
             ELM_WIDGET_DATA_GET(obj, sd);
             sd->highlight_next = next;
          }
     }
   else if (dir == ELM_HIGHLIGHT_DIR_PREVIOUS)
     {
        if (info && info->part_object)
          info->prev = next;
        else
          {
             ELM_WIDGET_DATA_GET(obj, sd);
             sd->highlight_previous = next;
          }
     }
   else
      ERR("Not supported focus direction for access highlight [%d]", dir);
}

EAPI void
elm_access_chain_end_set(Evas_Object *obj, Elm_Highlight_Direction dir)
{
   EINA_SAFETY_ON_NULL_RETURN(obj);

   ELM_WIDGET_DATA_GET(obj, sd);
   Elm_Access_Info *info = _elm_access_object_get(obj);

   if (info)
     info->end_dir = dir;
   else if (sd)
     sd->end_dir = dir;
}

EAPI Eina_Bool
_elm_access_object_lower_than_target(Evas_Object *obj, const char *target)
{
   Evas *evas = NULL;
   Evas_Object *child = NULL;
   Evas_Object *top_obj = NULL;
   Eina_List *l = NULL;
   Eina_List *obj_list = NULL;
   int x, y, w, h;

   evas_object_geometry_get(obj, &x, &y, &w, &h);

   evas = evas_object_evas_get(obj);
   if (!evas) return EINA_FALSE;

   obj_list = evas_tree_objects_at_xy_get(evas, NULL, x+w/2, y+h/2);

   EINA_LIST_REVERSE_FOREACH(obj_list, l, top_obj)
     {
       child = top_obj;

        while (child)
          {
             child = evas_object_smart_parent_get(child);
             if (child && !strcmp(evas_object_type_get(child), target))
               {
                   DBG("Find target type = %s, 0x%x", evas_object_type_get(child), child);
                   return EINA_TRUE;
               }
          }
     }

   return EINA_FALSE;
}