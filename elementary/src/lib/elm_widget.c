#include <Elementary.h>
#include "elm_priv.h"
#include "elm_widget_container.h"
#include "elm_interface_scrollable.h"

static const char ELM_WIDGET_SMART_NAME[] = "elm_widget";

#define API_ENTRY                                               \
  Elm_Widget_Smart_Data * sd;                                   \
  if ((!obj) || (!_elm_widget_is(obj)) ||                       \
      (!(sd = evas_object_smart_data_get(obj))))
#define INTERNAL_ENTRY                                          \
  Elm_Widget_Smart_Data * sd;                                   \
  if ((!obj) || (!(sd = evas_object_smart_data_get(obj))))      \
    return

#define ELM_WIDGET_FOCUS_GET(obj)                                         \
  ((_elm_access_auto_highlight_get()) ? (elm_widget_highlight_get(obj)) : \
                                        (elm_widget_focus_get(obj)))
typedef struct _Elm_Event_Cb_Data         Elm_Event_Cb_Data;
typedef struct _Elm_Translate_String_Data Elm_Translate_String_Data;

struct _Elm_Event_Cb_Data
{
   Elm_Event_Cb func;
   const void  *data;
};

struct _Elm_Translate_String_Data
{
   EINA_INLIST;
   Eina_Stringshare *id;
   Eina_Stringshare *domain;
   Eina_Stringshare *string;
   Eina_Bool   preset : 1;
};

/* local subsystem globals */
static unsigned int focus_order = 0;
static unsigned int highlight_order = 0;

// TIZEN ONLY(20131210) : Canceling key event when focus is changed.
static Evas_Object *old_focus = NULL;
static Eina_Bool enter_key_down = EINA_TRUE;
//////

static inline Eina_Bool
_elm_widget_is(const Evas_Object *obj)
{
   return evas_object_smart_type_check_ptr(obj, ELM_WIDGET_SMART_NAME);
}

static inline Eina_Bool
_is_focusable(Evas_Object *obj)
{
   API_ENTRY return EINA_FALSE;
   return sd->can_focus || (sd->child_can_focus);
}

static inline Eina_Bool
_elm_scrollable_is(const Evas_Object *obj)
{
   INTERNAL_ENTRY EINA_FALSE;
   return !!evas_object_smart_interface_get(obj, ELM_SCROLLABLE_IFACE_NAME);
}

/* what follows are basic (unimplemented) smart class functions */
#define UNIMPLEMENTED_MAKE(_prefix)                                         \
  static Eina_Bool                                                          \
  _elm_widget_##_prefix##_func_unimplemented(Evas_Object * obj)             \
  {                                                                         \
     WRN("The %s widget does not implement the \"" #_prefix "\" function.", \
         elm_widget_type_get(obj));                                         \
     return EINA_FALSE;                                                     \
  }

UNIMPLEMENTED_MAKE(disable);
UNIMPLEMENTED_MAKE(translate);

#undef UNIMPLEMENTED_MAKE

#define FOCUS_NEXT_DEL_MAKE(_prefix)                                        \
  static void                                                               \
  _focus_##_prefix##_object_del_cb(void *data, Evas *e __UNUSED__,          \
                                   Evas_Object * obj,                       \
                                   void *event_info __UNUSED__)             \
  {                                                                         \
     Elm_Widget_Smart_Data *sd = data;                                      \
     if (!sd) return;                                                       \
     if (sd->focus_##_prefix == obj)                                        \
        sd->focus_##_prefix = NULL;                                         \
  }

FOCUS_NEXT_DEL_MAKE(next);
FOCUS_NEXT_DEL_MAKE(previous);
FOCUS_NEXT_DEL_MAKE(up);
FOCUS_NEXT_DEL_MAKE(down);
FOCUS_NEXT_DEL_MAKE(right);
FOCUS_NEXT_DEL_MAKE(left);

/**
 * @internal
 * Resets the mirrored mode from the system mirror mode for widgets that are in
 * automatic mirroring mode. This function does not call elm_widget_theme.
 *
 * @param obj The widget.
 * @param mirrored EINA_TRUE to set mirrored mode. EINA_FALSE to unset.
 */
static void
_elm_widget_mirrored_reload(Evas_Object *obj)
{
   API_ENTRY return;
   Eina_Bool mirrored = elm_config_mirrored_get();

   if (elm_widget_mirrored_automatic_get(obj) && (sd->is_mirrored != mirrored))
     {
        sd->is_mirrored = mirrored;
     }
}

static Eina_Bool
_elm_widget_theme_func(Evas_Object *obj)
{
   _elm_widget_mirrored_reload(obj);

   elm_widget_disabled_set(obj, elm_widget_disabled_get(obj));

   return EINA_TRUE;
}

static Eina_Bool
_elm_widget_on_focus_region_func_unimplemented(const Evas_Object *obj __UNUSED__,
                                               Evas_Coord *x __UNUSED__,
                                               Evas_Coord *y __UNUSED__,
                                               Evas_Coord *w __UNUSED__,
                                               Evas_Coord *h __UNUSED__)
{
   WRN("The %s widget does not implement the \"on_focus_region\" function.",
       elm_widget_type_get(obj));
   return EINA_FALSE;
}

static Eina_Bool
_elm_widget_on_focus_func(Evas_Object *obj, Elm_Focus_Info *info)
{
   ELM_WIDGET_DATA_GET(obj, sd);

   if (elm_widget_can_focus_get(obj))
     {
        if (elm_widget_focus_get(obj))
          {
             if (!sd->resize_obj) evas_object_focus_set(obj, EINA_TRUE);
             evas_object_smart_callback_call(obj, "focused", info);
          }
        else
          {
             if (!sd->resize_obj) evas_object_focus_set(obj, EINA_FALSE);
             evas_object_smart_callback_call(obj, "unfocused", NULL);
          }
     }
   else
      return EINA_FALSE;

   return EINA_TRUE;
}

static Eina_Bool
_elm_widget_event_func_unimplemented(Evas_Object *obj,
                                     Evas_Object *source __UNUSED__,
                                     Evas_Callback_Type type __UNUSED__,
                                     void *event_info __UNUSED__)
{
   WRN("The %s widget does not implement the \"event\" function.",
       elm_widget_type_get(obj));
   return EINA_FALSE;
}

static Eina_Bool
_elm_widget_focus_next_func_unimplemented(const Evas_Object *obj,
                                          Elm_Focus_Direction dir __UNUSED__,
                                          Evas_Object **next __UNUSED__)
{
   WRN("The %s widget does not implement the \"focus_next\" function.",
       elm_widget_type_get(obj));
   return EINA_FALSE;
}

static Eina_Bool
_elm_widget_focus_direction_func_unimplemented(const Evas_Object *obj,
                                               const Evas_Object *b __UNUSED__,
                                               double degree __UNUSED__,
                                               Evas_Object **target __UNUSED__,
                                               double *weight __UNUSED__)
{
   WRN("The %s widget does not implement the \"focus_direction\" function.",
       elm_widget_type_get(obj));
   return EINA_FALSE;
}

static Eina_Bool
_elm_widget_focus_direction_manager_is_func_unimplemented(const Evas_Object *obj)
{
   WRN("The %s widget does not implement the \"focus_direction_manager_is\" function.",
       elm_widget_type_get(obj));
   return EINA_FALSE;
}

static void
_parents_focus(Evas_Object *obj)
{
   for (; obj; obj = elm_widget_parent_get(obj))
     {
        INTERNAL_ENTRY;
        if (sd->focused) return;
        sd->focused = 1;
     }
}

static void
_parents_unfocus(Evas_Object *obj)
{
   for (; obj; obj = elm_widget_parent_get(obj))
     {
        INTERNAL_ENTRY;
        if (!sd->focused) return;
        sd->focused = 0;
     }
}

static void
_on_sub_obj_hide(void *data __UNUSED__,
              Evas *e __UNUSED__,
              Evas_Object *obj,
              void *event_info __UNUSED__)
{
   elm_widget_focus_hide_handle(obj);
}

static void
_on_sub_obj_del(void *data,
             Evas *e __UNUSED__,
             Evas_Object *obj,
             void *event_info __UNUSED__)
{
   Elm_Widget_Smart_Data *sd = data;

   if (_elm_widget_is(obj))
     {
        if (elm_widget_focus_get(obj)) _parents_unfocus(sd->obj);
     }
   if (obj == sd->resize_obj)
     {
        /* already dels sub object */
        elm_widget_resize_object_set(sd->obj, NULL, EINA_TRUE);
        return;
     }
   else if (obj == sd->hover_obj)
     {
        sd->hover_obj = NULL;
        return;
     }
   else
     {
        if (!elm_widget_sub_object_del(sd->obj, obj))
          ERR("failed to remove sub object %p from %p\n", obj, sd->obj);
     }
}

static Eina_Bool
_elm_widget_sub_object_add_func(Evas_Object *obj,
                                Evas_Object *sobj)
{
   double scale, pscale = elm_widget_scale_get(sobj);
   Elm_Theme *th, *pth = elm_widget_theme_get(sobj);
   Eina_Bool mirrored, pmirrored = elm_widget_mirrored_get(obj);

   ELM_WIDGET_DATA_GET(obj, sd);
   EINA_SAFETY_ON_TRUE_RETURN_VAL(obj == sobj, EINA_FALSE);

   if (sobj == sd->parent_obj)
     {
        /* in this case, sobj must be an elm widget, or something
         * very wrong is happening */
        if (!_elm_widget_is(sobj)) return EINA_FALSE;

        if (!elm_widget_sub_object_del(sobj, obj)) return EINA_FALSE;
        WRN("You passed a parent object of obj = %p as the sub object = %p!",
            obj, sobj);
     }

   if (_elm_widget_is(sobj))
     {
        ELM_WIDGET_DATA_GET(sobj, sdc);

        if (sdc->parent_obj == obj) return EINA_TRUE;
        if (sdc->parent_obj)
          {
             if (!elm_widget_sub_object_del(sdc->parent_obj, sobj))
               return EINA_FALSE;
          }
        sdc->parent_obj = obj;
        if (sdc->orient_mode != sd->orient_mode)
          {
             if (!sdc->on_create) elm_widget_orientation_set(sobj, sd->orient_mode);
             else sdc->orient_mode = sd->orient_mode;
          }

        _elm_widget_top_win_focused_set(sobj, sd->top_win_focused);
        // TIZEN ONLY (20131211) : For broadcasting the state of focus highlight.
        _elm_widget_focus_highlighted_set(sobj, sd->focus_highlighted);

        /* update child focusable-ness on self and parents, now that a
         * focusable child got in */
        if (!sd->child_can_focus && (_is_focusable(sobj)))
          {
             Elm_Widget_Smart_Data *sdp = sd;

             sdp->child_can_focus = EINA_TRUE;
             while (sdp->parent_obj)
               {
                  sdp = evas_object_smart_data_get(sdp->parent_obj);

                  if (sdp->child_can_focus) break;

                  sdp->child_can_focus = EINA_TRUE;
               }
          }
     }
   else
     {
        void *data = evas_object_data_get(sobj, "elm-parent");

        if (data)
          {
             if (data == obj) return EINA_TRUE;
             if (!elm_widget_sub_object_del(data, sobj)) return EINA_FALSE;
          }
     }

   sd->subobjs = eina_list_append(sd->subobjs, sobj);
   evas_object_data_set(sobj, "elm-parent", obj);
   evas_object_event_callback_add
     (sobj, EVAS_CALLBACK_DEL, _on_sub_obj_del, sd);
   if (_elm_widget_is(sobj))
     {
        ELM_WIDGET_DATA_GET(sobj, sdc);

        evas_object_event_callback_add
          (sobj, EVAS_CALLBACK_HIDE, _on_sub_obj_hide, sd);

        scale = elm_widget_scale_get(sobj);
        th = elm_widget_theme_get(sobj);
        mirrored = elm_widget_mirrored_get(sobj);

        if ((scale != pscale) || (!sdc->on_create && th != pth) ||
            (!sdc->on_create && (pmirrored != mirrored)))
          elm_widget_theme(sobj);

        if (elm_widget_focus_get(sobj)) _parents_focus(obj);
     }

   elm_widget_display_mode_set(sobj,
                               evas_object_size_hint_display_mode_get(obj));
   return EINA_TRUE;
}

static Eina_Bool
_elm_widget_sub_object_del_func(Evas_Object *obj,
                                Evas_Object *sobj)
{
   Evas_Object *sobj_parent;

   if (!sobj) return EINA_FALSE;

   ELM_WIDGET_DATA_GET(obj, sd);
   EINA_SAFETY_ON_TRUE_RETURN_VAL(obj == sobj, EINA_FALSE);

   sobj_parent = evas_object_data_del(sobj, "elm-parent");
   if (sobj_parent != obj)
     {
        static int abort_on_warn = -1;

        ERR("removing sub object %p (%s) from parent %p (%s), "
            "but elm-parent is different %p (%s)!",
            sobj, elm_widget_type_get(sobj), obj, elm_widget_type_get(obj),
            sobj_parent, elm_widget_type_get(sobj_parent));

        if (EINA_UNLIKELY(abort_on_warn == -1))
          {
             if (getenv("ELM_ERROR_ABORT")) abort_on_warn = 1;
             else abort_on_warn = 0;
          }
        if (abort_on_warn == 1)
          {
             ERR("### EFL abort on errors ###\n");
             abort();
          }

        return EINA_FALSE;
     }

   if (_elm_widget_is(sobj))
     {
        if (elm_widget_focus_get(sobj))
          {
             if (!elm_widget_tree_unfocusable_get(sobj))
               {
                  elm_widget_tree_unfocusable_set(sobj, EINA_TRUE);
                  elm_widget_tree_unfocusable_set(sobj, EINA_FALSE);
               }
          }
        if ((sd->child_can_focus) && (_is_focusable(sobj)))
          {
             Evas_Object *parent = obj;

             /* update child focusable-ness on self and parents, now that a
              * focusable child is gone */
             while (parent)
               {
                  const Eina_List *l;
                  Evas_Object *subobj;

                  ELM_WIDGET_DATA_GET(parent, sdp);

                  sdp->child_can_focus = EINA_FALSE;
                  EINA_LIST_FOREACH(sdp->subobjs, l, subobj)
                    {
                       if ((subobj != sobj) && (_is_focusable(subobj)))
                         {
                            sdp->child_can_focus = EINA_TRUE;
                            break;
                         }
                    }

                  /* break again, child_can_focus went back to
                   * original value */
                  if (sdp->child_can_focus) break;
                  parent = sdp->parent_obj;
               }
          }

        ELM_WIDGET_DATA_GET(sobj, sdc);
        sdc->parent_obj = NULL;
     }

   if (sd->resize_obj == sobj) sd->resize_obj = NULL;

   sd->subobjs = eina_list_remove(sd->subobjs, sobj);

   evas_object_event_callback_del_full
     (sobj, EVAS_CALLBACK_DEL, _on_sub_obj_del, sd);
   if (_elm_widget_is(sobj))
     evas_object_event_callback_del_full
       (sobj, EVAS_CALLBACK_HIDE, _on_sub_obj_hide, sd);

   return EINA_TRUE;
}

static void
_elm_widget_sound_effect_enabled_set_func(Evas_Object *obj, Eina_Bool enable)
{
   if (!obj) return;
   ELM_WIDGET_DATA_GET(obj, sd);

   sd->is_sound_effect = enable;
}

static Eina_Bool
_elm_widget_sound_effect_enabled_is_func(const Evas_Object *obj)
{
   if (!obj) return EINA_FALSE;
   ELM_WIDGET_DATA_GET(obj, sd);

   return sd->is_sound_effect;
}

static const Evas_Smart_Cb_Description _smart_callbacks[] =
{
   /* FIXME: complete later */
   {NULL, NULL}
};

static void
_obj_mouse_down(void *data __UNUSED__,
                Evas *e __UNUSED__,
                Evas_Object *obj __UNUSED__,
                void *event_info)
{
   ELM_WIDGET_DATA_GET(obj, sd);
   Evas_Event_Mouse_Down *ev = event_info;
   if (!(ev->event_flags & EVAS_EVENT_FLAG_ON_HOLD))
     sd->still_in = EINA_TRUE;
}

static void
_obj_mouse_move(void *data __UNUSED__,
                Evas *e __UNUSED__,
                Evas_Object *obj,
                void *event_info)
{
   ELM_WIDGET_DATA_GET(obj, sd);
   Evas_Event_Mouse_Move *ev = event_info;
   if (sd->still_in)
     {
        if (ev->event_flags & EVAS_EVENT_FLAG_ON_HOLD)
          {
             sd->still_in = EINA_FALSE;
             // TIZEN ONLY
             // To give infomation when the touch mouse event goes out of the object from inside.
             evas_object_smart_callback_call(obj, "touch,mouse,out", ev);
          }
        else
          {
             Evas_Coord x, y, w, h;
             evas_object_geometry_get(obj, &x, &y, &w, &h);
             if (ELM_RECTS_POINT_OUT(x, y, w, h, ev->cur.canvas.x, ev->cur.canvas.y))
               {
                  sd->still_in = EINA_FALSE;
                  // TIZEN ONLY
                  // To give infomation when the touch mouse event goes out of the object from inside.
                  evas_object_smart_callback_call(obj, "touch,mouse,out", ev);
               }
          }
     }
}

static void
_obj_mouse_up(void *data __UNUSED__,
              Evas *e __UNUSED__,
              Evas_Object *obj,
              void *event_info __UNUSED__)
{
   ELM_WIDGET_DATA_GET(obj, sd);
   if (sd->still_in)
     elm_widget_focus_mouse_up_handle(obj);
   sd->still_in = EINA_FALSE;
}

static void
_smart_add(Evas_Object *obj)
{
   const Evas_Smart_Class *sc;
   const Evas_Smart *smart;

   EVAS_SMART_DATA_ALLOC(obj, Elm_Widget_Smart_Data);

   smart = evas_object_smart_smart_get(obj);
   sc = evas_smart_class_get(smart);
   priv->api = (const Elm_Widget_Smart_Class *)sc;
   priv->obj = obj;
   priv->x = priv->y = priv->w = priv->h = 0;
   priv->mirrored_auto_mode = EINA_TRUE; /* will follow system locale
                                          * settings */
   priv->on_create = EINA_TRUE;
   elm_widget_can_focus_set(obj, EINA_TRUE);
   priv->is_mirrored = elm_config_mirrored_get();

   evas_object_event_callback_add(obj, EVAS_CALLBACK_MOUSE_DOWN,
                                  _obj_mouse_down, obj);
   evas_object_event_callback_add(obj, EVAS_CALLBACK_MOUSE_MOVE,
                                  _obj_mouse_move, obj);
   evas_object_event_callback_add(obj, EVAS_CALLBACK_MOUSE_UP,
                                  _obj_mouse_up, obj);
   /* just a helper for inheriting classes */
   if (priv->resize_obj)
     {
        Evas_Object *r_obj = priv->resize_obj;
        priv->resize_obj = NULL;

        elm_widget_resize_object_set(obj, r_obj, EINA_TRUE);
     }
}


static void
_if_focused_revert(Evas_Object *obj,
                   Eina_Bool can_focus_only)
{
   Evas_Object *top;
   Evas_Object *newest = NULL, *access_newest = NULL;

   INTERNAL_ENTRY;

   if (!sd->parent_obj) return;

   top = elm_widget_top_get(sd->parent_obj);
   if (top)
     {
        if (_elm_config->access_mode && sd->highlighted)
          {
             unsigned int newest_focus_order = 0;
             newest_focus_order = 0;
             _elm_access_auto_highlight_set(EINA_TRUE);
             access_newest = elm_widget_newest_focus_order_get
                (top, &newest_focus_order, EINA_FALSE);
             _elm_access_auto_highlight_set(EINA_FALSE);
          }

        if (sd->focused)
          {
             unsigned int newest_focus_order = 0;
             newest = elm_widget_newest_focus_order_get
                (top, &newest_focus_order, can_focus_only);

             if (newest)
               {
                  if (elm_widget_type_get(newest) &&
                      !strcmp(elm_widget_type_get(newest), "elm_win"))
                    {
                       Elm_Widget_Smart_Data *sd2 = evas_object_smart_data_get(newest);
                       if (!sd2) return;

                       if (!elm_widget_focus_get(newest))
                         elm_widget_focus_steal(newest);
                       else
                         {
                            if (sd2->resize_obj && elm_widget_focus_get(sd2->resize_obj))
                              elm_widget_focused_object_clear(sd2->resize_obj);
                            else
                              {
                                 const Eina_List *l;
                                 Evas_Object *child;
                                 EINA_LIST_FOREACH(sd2->subobjs, l, child)
                                   {
                                      if (elm_widget_focus_get(child))
                                        {
                                           elm_widget_focused_object_clear(child);
                                           break;
                                        }
                                   }
                              }
                         }
                       evas_object_focus_set(newest, EINA_TRUE);
                    }
                  else
                    {
                       elm_object_focus_set(newest, EINA_FALSE);
                       elm_object_focus_set(newest, EINA_TRUE);
                    }
               }
          }

        if (access_newest)
          _elm_access_highlight_set(access_newest, EINA_TRUE);
     }
}

static void
_smart_del(Evas_Object *obj)
{
   Evas_Object *sobj;
   Elm_Translate_String_Data *ts;
   Elm_Event_Cb_Data *ecb;

   ELM_WIDGET_DATA_GET(obj, sd);

   if (obj == old_focus)
     old_focus = NULL;

   if (sd->hover_obj)
     {
        /* detach it from us */
        evas_object_event_callback_del_full
          (sd->hover_obj, EVAS_CALLBACK_DEL, _on_sub_obj_del, sd);
        sd->hover_obj = NULL;
     }

   while (sd->subobjs)
     {
        sobj = eina_list_data_get(sd->subobjs);

        /* let the objects clean-up themselves and get rid of this list */
        if (!elm_widget_sub_object_del(obj, sobj))
          {
             ERR("failed to remove sub object %p from %p\n", sobj, obj);
             sd->subobjs = eina_list_remove_list
                 (sd->subobjs, sd->subobjs);
          }
        evas_object_del(sobj);
     }
   sd->tooltips = eina_list_free(sd->tooltips); /* should be empty anyway */
   sd->cursors = eina_list_free(sd->cursors); /* should be empty anyway */
   while (sd->translate_strings)
     {
        ts = EINA_INLIST_CONTAINER_GET(sd->translate_strings,
                                       Elm_Translate_String_Data);
        eina_stringshare_del(ts->id);
        eina_stringshare_del(ts->domain);
        eina_stringshare_del(ts->string);
        sd->translate_strings = eina_inlist_remove(sd->translate_strings,
                                                   sd->translate_strings);
        free(ts);
     }

   EINA_LIST_FREE (sd->event_cb, ecb)
     free(ecb);

   if (sd->style) eina_stringshare_del(sd->style);
   if (sd->theme) elm_theme_free(sd->theme);
   _if_focused_revert(obj, EINA_TRUE);
   if (sd->access_info) eina_stringshare_del(sd->access_info);

   if (sd->focus_previous)
     evas_object_event_callback_del_full(sd->focus_previous, EVAS_CALLBACK_DEL,
                                         _focus_previous_object_del_cb, sd);
   if (sd->focus_next)
     evas_object_event_callback_del_full(sd->focus_next, EVAS_CALLBACK_DEL,
                                         _focus_next_object_del_cb, sd);
   if (sd->focus_up)
     evas_object_event_callback_del_full(sd->focus_up, EVAS_CALLBACK_DEL,
                                         _focus_up_object_del_cb, sd);
   if (sd->focus_down)
     evas_object_event_callback_del_full(sd->focus_down, EVAS_CALLBACK_DEL,
                                         _focus_down_object_del_cb, sd);
   if (sd->focus_right)
     evas_object_event_callback_del_full(sd->focus_right, EVAS_CALLBACK_DEL,
                                         _focus_right_object_del_cb, sd);
   if (sd->focus_left)
     evas_object_event_callback_del_full(sd->focus_left, EVAS_CALLBACK_DEL,
                                         _focus_left_object_del_cb, sd);
   free(sd);
   evas_object_smart_data_set(obj, NULL);
}

static void
_smart_reconfigure(Elm_Widget_Smart_Data *sd)
{
   if (sd->resize_obj)
     {
        evas_object_move(sd->resize_obj, sd->x, sd->y);
        evas_object_resize(sd->resize_obj, sd->w, sd->h);
     }
   if (sd->hover_obj)
     {
        evas_object_move(sd->hover_obj, sd->x, sd->y);
        evas_object_resize(sd->hover_obj, sd->w, sd->h);
     }
}

static void
_smart_move(Evas_Object *obj,
            Evas_Coord x,
            Evas_Coord y)
{
   ELM_WIDGET_DATA_GET(obj, sd);

   sd->x = x;
   sd->y = y;

   _smart_reconfigure(sd);
}

static void
_smart_resize(Evas_Object *obj,
              Evas_Coord w,
              Evas_Coord h)
{
   ELM_WIDGET_DATA_GET(obj, sd);

   sd->w = w;
   sd->h = h;

   _smart_reconfigure(sd);
}

static void
_smart_show(Evas_Object *obj)
{
   Eina_List *list;
   Evas_Object *o;

   if ((list = evas_object_smart_members_get(obj)))
     {
        EINA_LIST_FREE (list, o)
          {
             if (evas_object_data_get(o, "_elm_leaveme")) continue;
             evas_object_show(o);
          }
     }
}

static void
_smart_hide(Evas_Object *obj)
{
   Eina_List *list;
   Evas_Object *o;

   list = evas_object_smart_members_get(obj);
   EINA_LIST_FREE (list, o)
     {
        if (evas_object_data_get(o, "_elm_leaveme")) continue;
        evas_object_hide(o);
     }
}

static void
_smart_color_set(Evas_Object *obj,
                 int r,
                 int g,
                 int b,
                 int a)
{
   Eina_List *list;
   Evas_Object *o;

   if ((list = evas_object_smart_members_get(obj)))
     {
        EINA_LIST_FREE (list, o)
          {
             if (evas_object_data_get(o, "_elm_leaveme")) continue;
             evas_object_color_set(o, r, g, b, a);
          }
     }
}

static void
_smart_clip_set(Evas_Object *obj,
                Evas_Object *clip)
{
   Eina_List *list;
   Evas_Object *o;

   if ((list = evas_object_smart_members_get(obj)))
     {
        EINA_LIST_FREE (list, o)
          {
             if (evas_object_data_get(o, "_elm_leaveme")) continue;
             evas_object_clip_set(o, clip);
          }
     }
}

static void
_smart_clip_unset(Evas_Object *obj)
{
   Eina_List *list;
   Evas_Object *o;

   if ((list = evas_object_smart_members_get(obj)))
     {
        EINA_LIST_FREE (list, o)
          {
             if (evas_object_data_get(o, "_elm_leaveme")) continue;
             evas_object_clip_unset(o);
          }
     }
}

static void
_smart_calculate(Evas_Object *obj __UNUSED__)
{
   /* a NO-OP, on the base */
}

static void
_smart_member_add(Evas_Object *obj,
                  Evas_Object *child)
{
   int r, g, b, a;

   if (evas_object_data_get(child, "_elm_leaveme")) return;

   evas_object_color_get(obj, &r, &g, &b, &a);
   evas_object_color_set(child, r, g, b, a);

   evas_object_clip_set(child, evas_object_clip_get(obj));

   if (evas_object_visible_get(obj))
     evas_object_show(child);
   else
     evas_object_hide(child);
}

static void
_smart_member_del(Evas_Object *obj __UNUSED__,
                  Evas_Object *child)
{
   if (evas_object_data_get(child, "_elm_leaveme")) return;
   evas_object_clip_unset(child);
}

static void
_elm_widget_smart_set(Elm_Widget_Smart_Class *api)
{
   Evas_Smart_Class *sc;

   if (!(sc = (Evas_Smart_Class *)api))
     return;

   sc->add = _smart_add;
   sc->del = _smart_del;
   sc->move = _smart_move;
   sc->resize = _smart_resize;
   sc->show = _smart_show;
   sc->hide = _smart_hide;
   sc->color_set = _smart_color_set;
   sc->clip_set = _smart_clip_set;
   sc->clip_unset = _smart_clip_unset;
   sc->calculate = _smart_calculate;
   sc->member_add = _smart_member_add;
   sc->member_del = _smart_member_del;

#define API_DEFAULT_SET_UNIMPLEMENTED(_prefix) \
  api->_prefix = _elm_widget_##_prefix##_func_unimplemented;

   /* NB: always remember to call these parent versions on children,
    * except for the unimplemented ones and calculate, which is moot */

#define API_DEFAULT_SET(_prefix) \
  api->_prefix = _elm_widget_##_prefix##_func;

   /* base api */
   API_DEFAULT_SET(on_focus);
   API_DEFAULT_SET_UNIMPLEMENTED(disable);

   API_DEFAULT_SET(theme);
   API_DEFAULT_SET_UNIMPLEMENTED(on_focus_region);

   API_DEFAULT_SET_UNIMPLEMENTED(translate);
   API_DEFAULT_SET_UNIMPLEMENTED(event);
   API_DEFAULT_SET_UNIMPLEMENTED(focus_next);
   API_DEFAULT_SET_UNIMPLEMENTED(focus_direction);
   API_DEFAULT_SET_UNIMPLEMENTED(focus_direction_manager_is);

   /* NB: because those two weren't hooks before, translate the
    * individual calls to them on the widgets as we bring them to the
    * new class hierarchy. also, sub_object_{add,del} must be
    * different than member_{add,del} here, because widget parenting
    * on elm does not always imply parent and child will live on the
    * same Evas layer */
   API_DEFAULT_SET(sub_object_add);
   API_DEFAULT_SET(sub_object_del);
   API_DEFAULT_SET(sound_effect_enabled_set);
   API_DEFAULT_SET(sound_effect_enabled_is);

#undef API_DEFAULT_SET
#undef API_DEFAULT_SET_UNIMPLEMENTED

   sc->callbacks = _smart_callbacks;
}

// internal funcs
static inline Eina_Bool
_elm_widget_focus_chain_manager_is(const Evas_Object *obj)
{
   API_ENTRY return EINA_FALSE;

   if (!sd->api) return EINA_FALSE;
   return sd->api->focus_next &&
          (sd->api->focus_next != _elm_widget_focus_next_func_unimplemented);
}

static inline Eina_Bool
_elm_widget_focus_direction_manager_is(const Evas_Object *obj)
{
   API_ENTRY return EINA_FALSE;

   if (!sd->api) return EINA_FALSE;
   if (!sd->api->focus_direction_manager_is) return EINA_FALSE;
   return sd->api->focus_direction_manager_is(obj);
}

static void
_propagate_x_drag_lock(Evas_Object *obj,
                       int dir)
{
   INTERNAL_ENTRY;
   if (sd->parent_obj)
     {
        Elm_Widget_Smart_Data *sd2 = evas_object_smart_data_get(sd->parent_obj);
        if (sd2)
          {
             sd2->child_drag_x_locked += dir;
             _propagate_x_drag_lock(sd->parent_obj, dir);
          }
     }
}

static void
_propagate_y_drag_lock(Evas_Object *obj,
                       int dir)
{
   INTERNAL_ENTRY;
   if (sd->parent_obj)
     {
        Elm_Widget_Smart_Data *sd2 = evas_object_smart_data_get(sd->parent_obj);
        if (sd2)
          {
             sd2->child_drag_y_locked += dir;
             _propagate_y_drag_lock(sd->parent_obj, dir);
          }
     }
}

static void
_propagate_event(void *data,
                 Evas *e __UNUSED__,
                 Evas_Object *obj,
                 void *event_info)
{
   INTERNAL_ENTRY;
   Evas_Object *current_focus;
   Evas_Callback_Type type = (Evas_Callback_Type)(long)data;
   Evas_Event_Flags *event_flags = NULL;

   switch (type)
     {
      case EVAS_CALLBACK_KEY_DOWN:
      {
         Evas_Event_Key_Down *ev = event_info;
         event_flags = &(ev->event_flags);
      }
      break;

      case EVAS_CALLBACK_KEY_UP:
      {
         Evas_Event_Key_Up *ev = event_info;
         event_flags = &(ev->event_flags);
      }
      break;

      case EVAS_CALLBACK_MOUSE_WHEEL:
      {
         Evas_Event_Mouse_Wheel *ev = event_info;
         event_flags = &(ev->event_flags);
      }
      break;

      default:
        break;
     }

   current_focus = evas_focus_get(evas_object_evas_get(obj));
   if (type == EVAS_CALLBACK_KEY_DOWN)
     {
        Evas_Event_Key_Down *ev = event_info;
        if (!strcmp(ev->keyname, "Return") || !strcmp(ev->keyname, "KP_Enter"))
          {
             if (old_focus && (old_focus != current_focus))
               {
                  if (enter_key_down) return;
               }
             old_focus = current_focus;
             enter_key_down = EINA_TRUE;
          }
     }
   else if (type == EVAS_CALLBACK_KEY_UP)
     {
        Evas_Event_Key_Up *ev = event_info;
        if (!strcmp(ev->keyname, "Return") || !strcmp(ev->keyname, "KP_Enter"))
          {
             enter_key_down = EINA_FALSE;
             if (old_focus != current_focus)
               {
                  ev->event_flags |= EVAS_EVENT_FLAG_ON_HOLD;
                  old_focus = current_focus;
                  return;
               }
          }
     }
   elm_widget_event_propagate(obj, type, event_info, event_flags);
}

/**
 * If elm_widget_focus_region_get() returns EINA_FALSE, this function will
 * ignore region show action.
 */
static void
_elm_widget_focus_region_show(const Evas_Object *obj)
{
   Evas_Coord x, y, ox, oy, w, h;
   Evas_Object *o;

   API_ENTRY return;

   o = elm_widget_parent_get(obj);
   if (!o) return;

   if (!elm_widget_focus_region_get(obj, &x, &y, &w, &h))
     return;

   evas_object_geometry_get(obj, &ox, &oy, NULL, NULL);
   if (w == 0 && h == 0) return;

   while (o)
     {
        if (_elm_scrollable_is(o) && !elm_widget_disabled_get(o))
          {
             Evas_Coord px, py, pw, ph;
             Evas_Coord rx, ry;

             evas_object_geometry_get(o, &px, &py, &pw, &ph);

             if ((px <= ox) && (px + pw) >= (ox + w) &&
                 (py <= oy) && (py + ph) >= (oy + h))
               {
                  // if object is already shown
                  o = elm_widget_parent_get(o);
                  continue;
               }

             ELM_SCROLLABLE_IFACE_GET(o, s_iface);
             s_iface->content_pos_get(o, &rx, &ry);

             x += rx + ox - px;
             y += ry + oy - py;
             if (ox < px) ox = px;
             else if ((ox + w) > (px + pw)) ox = px + pw - w;
             if (oy < py) oy = py;
             else if ((oy + h) > (py + ph)) oy = py + ph - h;

             if (s_iface->is_auto_scroll_enabled(o))
                s_iface->content_region_show(o, x, y, w, h);
             x = 0;
             y = 0;
          }
        o = elm_widget_parent_get(o);
     }
}

EAPI void
elm_widget_focus_region_show(const Evas_Object *obj)
{
   _elm_widget_focus_region_show(obj);
}

static void
_parent_focus(Evas_Object *obj, Elm_Focus_Direction dir, Evas_Object *prev_focused)
{
   API_ENTRY return;

   if (sd->focused) return;

   Evas_Object *o = elm_widget_parent_get(obj);
   sd->focus_order_on_calc = EINA_TRUE;

   if (o) _parent_focus(o, dir, prev_focused);

   if (!sd->focus_order_on_calc)
     return;  /* we don't want to override it if by means of any of the
                 callbacks below one gets to calculate our order
                 first. */

   focus_order++;
   sd->focus_order = focus_order;
   if (!sd->api) return;
   if (sd->top_win_focused)
     {
        Elm_Focus_Info fi;

        fi.prev_focused = prev_focused;
        fi.dir = dir;
        sd->focused = EINA_TRUE;
        sd->api->on_focus(obj, &fi);
     }
   sd->focus_order_on_calc = EINA_FALSE;
}

static void
_elm_object_focus_chain_del_cb(void *data,
                               Evas *e __UNUSED__,
                               Evas_Object *obj,
                               void *event_info __UNUSED__)
{
   Elm_Widget_Smart_Data *sd = data;

   sd->focus_chain = eina_list_remove(sd->focus_chain, obj);
}

// TIZEN ONLY: for color class setting //
void
_elm_widget_color_overlay_set(Evas_Object *obj, Evas_Object *sobj)
{
   Evas_Object *edje;

   edje = evas_object_data_get(sobj, "color_overlay");
   if (edje) return;

   edje = evas_object_data_get(obj, "color_overlay");
   if (edje)
     evas_object_data_set(sobj, "color_overlay", edje);
}

void
_elm_widget_font_overlay_set(Evas_Object *obj, Evas_Object *sobj)
{
   Evas_Object *edje;

   edje = evas_object_data_get(sobj, "font_overlay");
   if (edje) return;

   edje = evas_object_data_get(obj, "font_overlay");
   if (edje)
     evas_object_data_set(sobj, "font_overlay", edje);
}
///////////////////////////////////////////

EAPI const Elm_Widget_Smart_Class *
elm_widget_smart_class_get(void)
{
   static Elm_Widget_Smart_Class _sc =
     ELM_WIDGET_SMART_CLASS_INIT_NAME_VERSION(ELM_WIDGET_SMART_NAME);
   static const Elm_Widget_Smart_Class *class = NULL;

   if (class)
     return class;

   _elm_widget_smart_set(&_sc);
   class = &_sc;

   return class;
}

EAPI Evas_Object *
elm_widget_add(Evas_Smart *smart,
               Evas_Object *parent)
{
   Evas *e;
   Evas_Object *o;

   e = evas_object_evas_get(parent);
   if (!e) return NULL;

   o = evas_object_smart_add(e, smart);
   elm_widget_parent_set(o, parent);

   ELM_WIDGET_DATA_GET(o, sd);
   sd->end_dir = ELM_HIGHLIGHT_DIR_FIRST;
   sd->is_sound_effect = EINA_TRUE;

   return o;
}

EAPI void
elm_widget_parent_set(Evas_Object *obj,
                      Evas_Object *parent)
{
   ELM_WIDGET_DATA_GET(obj, sd);

   if (!sd->api->parent_set) return;

   sd->api->parent_set(obj, parent);
}

EAPI Eina_Bool
elm_widget_api_check(int ver)
{
   if (ver != ELM_INTERNAL_API_VERSION)
     {
        CRITICAL("Elementary widget api versions do not match");
        return EINA_FALSE;
     }
   return EINA_TRUE;
}

EAPI Eina_Bool
elm_widget_access(Evas_Object *obj,
                  Eina_Bool is_access)
{
   const Eina_List *l;
   Evas_Object *child;
   Eina_Bool ret = EINA_TRUE;

   API_ENTRY return EINA_FALSE;
   EINA_LIST_FOREACH(sd->subobjs, l, child)
     ret &= elm_widget_access(child, is_access);

   if (sd->api && sd->api->access)
     sd->api->access(obj, is_access);
   else
     return EINA_FALSE;

   return ret;
}

EAPI Eina_Bool
elm_widget_theme(Evas_Object *obj)
{
   const Eina_List *l;
   Evas_Object *child;
   Elm_Tooltip *tt;
   Elm_Cursor *cur;
   Eina_Bool ret = EINA_TRUE;

   API_ENTRY return EINA_FALSE;

   EINA_LIST_FOREACH(sd->subobjs, l, child)
      if (_elm_widget_is(child)) ret &= elm_widget_theme(child);
   if (sd->hover_obj) ret &= elm_widget_theme(sd->hover_obj);

   EINA_LIST_FOREACH(sd->tooltips, l, tt)
     elm_tooltip_theme(tt);
   EINA_LIST_FOREACH(sd->cursors, l, cur)
     elm_cursor_theme(cur);

   if (!sd->api) return EINA_FALSE;

   ret &= sd->api->theme(obj);

   return ret;
}

EAPI void
elm_widget_theme_specific(Evas_Object *obj,
                          Elm_Theme *th,
                          Eina_Bool force)
{
   const Eina_List *l;
   Evas_Object *child;
   Elm_Tooltip *tt;
   Elm_Cursor *cur;
   Elm_Theme *th2, *thdef;

   API_ENTRY return;

   thdef = elm_theme_default_get();
   if (!th) th = thdef;
   if (!force)
     {
        th2 = sd->theme;
        if (!th2) th2 = thdef;
        while (th2)
          {
             if (th2 == th)
               {
                  force = EINA_TRUE;
                  break;
               }
             if (th2 == thdef) break;
             th2 = th2->ref_theme;
             if (!th2) th2 = thdef;
          }
     }
   if (!force) return;
   EINA_LIST_FOREACH(sd->subobjs, l, child)
     elm_widget_theme_specific(child, th, force);
   if (sd->hover_obj) elm_widget_theme(sd->hover_obj);
   EINA_LIST_FOREACH(sd->tooltips, l, tt)
     elm_tooltip_theme(tt);
   EINA_LIST_FOREACH(sd->cursors, l, cur)
     elm_cursor_theme(cur);
   if (!sd->api) return;
   sd->api->theme(obj);
}

/**
 * Returns the widget's mirrored mode.
 *
 * @param obj The widget.
 * @return mirrored mode of the object.
 *
 **/
EAPI Eina_Bool
elm_widget_mirrored_get(const Evas_Object *obj)
{
   API_ENTRY return EINA_FALSE;
   return sd->is_mirrored;
}

/**
 * Sets the widget's mirrored mode.
 *
 * @param obj The widget.
 * @param mirrored EINA_TRUE to set mirrored mode. EINA_FALSE to unset.
 */
EAPI void
elm_widget_mirrored_set(Evas_Object *obj,
                        Eina_Bool mirrored)
{
   API_ENTRY return;

   mirrored = !!mirrored;

   if (sd->is_mirrored == mirrored) return;

   sd->is_mirrored = mirrored;
   elm_widget_theme(obj);
}

/**
 * Returns the widget's mirrored mode setting.
 *
 * @param obj The widget.
 * @return mirrored mode setting of the object.
 *
 **/
EAPI Eina_Bool
elm_widget_mirrored_automatic_get(const Evas_Object *obj)
{
   API_ENTRY return EINA_FALSE;
   return sd->mirrored_auto_mode;
}

/**
 * Sets the widget's mirrored mode setting.
 * When widget in automatic mode, it follows the system mirrored mode set by
 * elm_mirrored_set().
 * @param obj The widget.
 * @param automatic EINA_TRUE for auto mirrored mode. EINA_FALSE for manual.
 */
EAPI void
elm_widget_mirrored_automatic_set(Evas_Object *obj,
                                  Eina_Bool automatic)
{
   API_ENTRY return;
   if (sd->mirrored_auto_mode != automatic)
     {
        sd->mirrored_auto_mode = automatic;

        if (automatic)
          {
             elm_widget_mirrored_set(obj, elm_config_mirrored_get());
          }
     }
}

EAPI void
elm_widget_on_show_region_hook_set(Evas_Object *obj,
                                   void (*func)(void *data,
                                                Evas_Object *obj),
                                   void *data)
{
   API_ENTRY return;

   sd->on_show_region = func;
   sd->on_show_region_data = data;
}

EAPI Eina_Bool
elm_widget_sub_object_add(Evas_Object *obj,
                          Evas_Object *sobj)
{
   API_ENTRY return EINA_FALSE;
   EINA_SAFETY_ON_TRUE_RETURN_VAL(obj == sobj, EINA_FALSE);

   // TIZEN ONLY: for color class setting //
   _elm_widget_color_overlay_set(obj, sobj);
   _elm_widget_font_overlay_set(obj, sobj);

   if (!sd->api) return EINA_FALSE;
   return sd->api->sub_object_add(obj, sobj);
}

EAPI Eina_Bool
elm_widget_sub_object_del(Evas_Object *obj,
                          Evas_Object *sobj)
{
   API_ENTRY return EINA_FALSE;
   EINA_SAFETY_ON_TRUE_RETURN_VAL(obj == sobj, EINA_FALSE);

   if (!sobj) return EINA_FALSE;

   if (!sd->api) return EINA_FALSE;
   return sd->api->sub_object_del(obj, sobj);
}

//SHKWAK : Temp Function for Indicator Issue
// THIS MUST BE REMOVED

EAPI const Eina_List *
elm_widget_sub_object_list_get(const Evas_Object *obj)
{
   API_ENTRY return NULL;
   return (const Eina_List *)sd->subobjs;
}

//SHKWAK : END


/* a resize object is added to and deleted from the smart member and the sub object
 * of the parent if the third argument, Eina_Bool sub_obj, is set as EINA_TRUE.
 */
EAPI void
elm_widget_resize_object_set(Evas_Object *obj,
                             Evas_Object *sobj,
                             Eina_Bool sub_obj)
{
   Evas_Object *parent;

   API_ENTRY return;

   if (sd->resize_obj == sobj) return;

   // orphan previous resize obj
   if (sd->resize_obj)
     {
        if (sub_obj)
          {
             evas_object_clip_unset(sd->resize_obj);
             evas_object_smart_member_del(sd->resize_obj);

             if (_elm_widget_is(sd->resize_obj))
               {
                  if (elm_widget_focus_get(sd->resize_obj)) _parents_unfocus(obj);
               }
             elm_widget_sub_object_del(obj, sd->resize_obj);
          }
     }

   sd->resize_obj = sobj;
   if (!sobj) return;

   // orphan new resize obj
   parent = evas_object_data_get(sobj, "elm-parent");
   if (parent && parent != obj)
     {
        ELM_WIDGET_DATA_GET(parent, sdp);

        /* should be there, just being paranoid */
        if (sdp)
          {
             if (sdp->resize_obj == sobj)
               elm_widget_resize_object_set(parent, NULL, sub_obj);
             else if (sub_obj)
               elm_widget_sub_object_del(parent, sobj);
          }
     }

   if (sub_obj)
     {
        elm_widget_sub_object_add(obj, sobj);
        evas_object_smart_member_add(sobj, obj);
     }

   _smart_reconfigure(sd);
}

/* WARNING: the programmer is responsible, in the scenario of
 * exchanging a hover object, of cleaning the old hover "target"
 * before
 */
EAPI void
elm_widget_hover_object_set(Evas_Object *obj,
                            Evas_Object *sobj)
{
   API_ENTRY return;
   if (sd->hover_obj)
     {
        evas_object_event_callback_del_full(sd->hover_obj, EVAS_CALLBACK_DEL,
                                            _on_sub_obj_del, sd);
     }
   sd->hover_obj = sobj;
   if (sd->hover_obj)
     {
        evas_object_event_callback_add(sobj, EVAS_CALLBACK_DEL,
                                       _on_sub_obj_del, sd);
        _smart_reconfigure(sd);
     }
}

EAPI void
elm_widget_can_focus_set(Evas_Object *obj,
                         Eina_Bool can_focus)
{
   API_ENTRY return;

   can_focus = !!can_focus;

   if (sd->can_focus == can_focus) return;
   sd->can_focus = can_focus;
   if (sd->can_focus)
     {
        /* update child_can_focus of parents */
        Evas_Object *o = obj;

        for (;;)
          {
             o = elm_widget_parent_get(o);
             if (!o) break;
             sd = evas_object_smart_data_get(o);
             if (!sd || sd->child_can_focus) break;
             sd->child_can_focus = EINA_TRUE;
          }

        evas_object_event_callback_add(obj, EVAS_CALLBACK_KEY_DOWN,
                                       _propagate_event,
                                       (void *)(long)EVAS_CALLBACK_KEY_DOWN);
        evas_object_event_callback_add(obj, EVAS_CALLBACK_KEY_UP,
                                       _propagate_event,
                                       (void *)(long)EVAS_CALLBACK_KEY_UP);
        evas_object_event_callback_add(obj, EVAS_CALLBACK_MOUSE_WHEEL,
                                       _propagate_event,
                                       (void *)(long)EVAS_CALLBACK_MOUSE_WHEEL);
     }
   else
     {
        evas_object_event_callback_del(obj, EVAS_CALLBACK_KEY_DOWN,
                                       _propagate_event);
        evas_object_event_callback_del(obj, EVAS_CALLBACK_KEY_UP,
                                       _propagate_event);
        evas_object_event_callback_del(obj, EVAS_CALLBACK_MOUSE_WHEEL,
                                       _propagate_event);
     }
}

EAPI Eina_Bool
elm_widget_can_focus_get(const Evas_Object *obj)
{
   API_ENTRY return EINA_FALSE;
   return sd->can_focus;
}

EAPI Eina_Bool
elm_widget_child_can_focus_get(const Evas_Object *obj)
{
   API_ENTRY return EINA_FALSE;
   return sd->child_can_focus;
}

/**
 * @internal
 *
 * This API makes the widget object and its children to be unfocusable.
 *
 * This API can be helpful for an object to be deleted.
 * When an object will be deleted soon, it and its children may not
 * want to get focus (by focus reverting or by other focus controls).
 * Then, just use this API before deleting.
 *
 * @param obj The widget root of sub-tree
 * @param tree_unfocusable If true, set the object sub-tree as unfocusable
 *
 * @ingroup Widget
 */
EAPI void
elm_widget_tree_unfocusable_set(Evas_Object *obj,
                                Eina_Bool tree_unfocusable)
{
   API_ENTRY return;

   tree_unfocusable = !!tree_unfocusable;
   if (sd->tree_unfocusable == tree_unfocusable) return;
   sd->tree_unfocusable = tree_unfocusable;
   if (sd->tree_unfocusable)
     elm_widget_focus_tree_unfocusable_handle(obj);
}

/**
 * @internal
 *
 * This returns true, if the object sub-tree is unfocusable.
 *
 * @param obj The widget root of sub-tree
 * @return EINA_TRUE if the object sub-tree is unfocusable
 *
 * @ingroup Widget
 */
EAPI Eina_Bool
elm_widget_tree_unfocusable_get(const Evas_Object *obj)
{
   API_ENTRY return EINA_FALSE;
   return sd->tree_unfocusable;
}

/**
 * @internal
 *
 * Get the list of focusable child objects.
 *
 * This function retruns list of child objects which can get focus.
 *
 * @param obj The parent widget
 * @retrun list of focusable child objects.
 *
 * @ingroup Widget
 */
EAPI Eina_List *
elm_widget_can_focus_child_list_get(const Evas_Object *obj)
{
   API_ENTRY return NULL;

   const Eina_List *l;
   Eina_List *child_list = NULL;
   Evas_Object *child;

   if (sd->subobjs)
     {
        EINA_LIST_FOREACH(sd->subobjs, l, child)
          {
             if ((elm_widget_can_focus_get(child)) &&
                 (evas_object_visible_get(child)) &&
                 (!elm_widget_disabled_get(child)))
               child_list = eina_list_append(child_list, child);
             else if (elm_widget_is(child))
               {
                  Eina_List *can_focus_list;
                  can_focus_list = elm_widget_can_focus_child_list_get(child);
                  if (can_focus_list)
                    child_list = eina_list_merge(child_list, can_focus_list);
               }
          }
     }
   return child_list;
}

EAPI void
elm_widget_highlight_ignore_set(Evas_Object *obj,
                                Eina_Bool ignore)
{
   API_ENTRY return;
   sd->highlight_ignore = !!ignore;
}

EAPI Eina_Bool
elm_widget_highlight_ignore_get(const Evas_Object *obj)
{
   API_ENTRY return EINA_FALSE;
   return sd->highlight_ignore;
}

EAPI void
elm_widget_highlight_in_theme_set(Evas_Object *obj,
                                  Eina_Bool highlight)
{
   API_ENTRY return;
   sd->highlight_in_theme = !!highlight;
   /* FIXME: if focused, it should switch from one mode to the other */
}

EAPI Eina_Bool
elm_widget_highlight_in_theme_get(const Evas_Object *obj)
{
   API_ENTRY return EINA_FALSE;
   return sd->highlight_in_theme;
}

EAPI void
elm_widget_access_highlight_in_theme_set(Evas_Object *obj,
                                         Eina_Bool highlight)
{
   API_ENTRY return;
   sd->access_highlight_in_theme = !!highlight;
}

EAPI Eina_Bool
elm_widget_access_highlight_in_theme_get(const Evas_Object *obj)
{
   API_ENTRY return EINA_FALSE;
   return sd->access_highlight_in_theme;
}

EAPI Eina_Bool
elm_widget_focus_get(const Evas_Object *obj)
{
   API_ENTRY return EINA_FALSE;
   return sd->focused;
}

EAPI Evas_Object *
elm_widget_focused_object_get(const Evas_Object *obj)
{
   const Evas_Object *subobj;
   const Eina_List *l;
   API_ENTRY return NULL;

   if (!sd->focused) return NULL;
   EINA_LIST_FOREACH(sd->subobjs, l, subobj)
     {
        Evas_Object *fobj = elm_widget_focused_object_get(subobj);
        if (fobj) return fobj;
     }
   return (Evas_Object *)obj;
}

EAPI Evas_Object *
elm_widget_top_get(const Evas_Object *obj)
{
   API_ENTRY return NULL;
   if (sd->parent_obj) return elm_widget_top_get(sd->parent_obj);
   return (Evas_Object *)obj;
}

EAPI Eina_Bool
elm_widget_is(const Evas_Object *obj)
{
   return _elm_widget_is(obj);
}

EAPI Evas_Object *
elm_widget_parent_widget_get(const Evas_Object *obj)
{
   Evas_Object *parent;

   if (_elm_widget_is(obj))
     {
        Elm_Widget_Smart_Data *sd = evas_object_smart_data_get(obj);
        if (!sd) return NULL;
        parent = sd->parent_obj;
     }
   else
     {
        parent = evas_object_data_get(obj, "elm-parent");
        if (!parent) parent = evas_object_smart_parent_get(obj);
     }

   while (parent)
     {
        Evas_Object *elm_parent;
        if (_elm_widget_is(parent)) break;
        elm_parent = evas_object_data_get(parent, "elm-parent");
        if (elm_parent) parent = elm_parent;
        else parent = evas_object_smart_parent_get(parent);
     }
   return parent;
}

EAPI Evas_Object *
elm_widget_parent2_get(const Evas_Object *obj)
{
   if (_elm_widget_is(obj))
     {
        Elm_Widget_Smart_Data *sd = evas_object_smart_data_get(obj);
        if (sd) return sd->parent2;
     }
   return NULL;
}

EAPI void
elm_widget_parent2_set(Evas_Object *obj, Evas_Object *parent)
{
   API_ENTRY return;
   sd->parent2 = parent;
}

EAPI void
elm_widget_event_callback_add(Evas_Object *obj,
                              Elm_Event_Cb func,
                              const void *data)
{
   API_ENTRY return;
   EINA_SAFETY_ON_NULL_RETURN(func);
   Elm_Event_Cb_Data *ecb = ELM_NEW(Elm_Event_Cb_Data);
   ecb->func = func;
   ecb->data = data;
   sd->event_cb = eina_list_append(sd->event_cb, ecb);
}

EAPI void *
elm_widget_event_callback_del(Evas_Object *obj,
                              Elm_Event_Cb func,
                              const void *data)
{
   API_ENTRY return NULL;
   EINA_SAFETY_ON_NULL_RETURN_VAL(func, NULL);
   Eina_List *l;
   Elm_Event_Cb_Data *ecd;
   EINA_LIST_FOREACH(sd->event_cb, l, ecd)
     if ((ecd->func == func) && (ecd->data == data))
       {
          free(ecd);
          sd->event_cb = eina_list_remove_list(sd->event_cb, l);
          return (void *)data;
       }
   return NULL;
}

EAPI Eina_Bool
elm_widget_event_propagate(Evas_Object *obj,
                           Evas_Callback_Type type,
                           void *event_info,
                           Evas_Event_Flags *event_flags)
{
   API_ENTRY return EINA_FALSE; //TODO reduce.

   if (!_elm_widget_is(obj)) return EINA_FALSE;
   Evas_Object *parent = obj;
   Elm_Event_Cb_Data *ecd;
   Eina_List *l, *l_prev;

   while (parent &&
          (!(event_flags && ((*event_flags) & EVAS_EVENT_FLAG_ON_HOLD))))
     {
        sd = evas_object_smart_data_get(parent);
        if ((!sd) || (!_elm_widget_is(obj)))
          return EINA_FALSE;  //Not Elm Widget
        if (!sd->api) return EINA_FALSE;

        if (sd->api->event(parent, obj, type, event_info))
          return EINA_TRUE;

        EINA_LIST_FOREACH_SAFE(sd->event_cb, l, l_prev, ecd)
          {
             if (ecd->func((void *)ecd->data, parent, obj, type, event_info) ||
                 (event_flags && ((*event_flags) & EVAS_EVENT_FLAG_ON_HOLD)))
               return EINA_TRUE;
          }
        parent = sd->parent_obj;
     }

   return EINA_FALSE;
}

/**
 * @internal
 *
 * Set custom focus chain.
 *
 * This function i set one new and overwrite any previous custom focus chain
 * with the list of objects. The previous list will be deleted and this list
 * will be managed. After setted, don't modity it.
 *
 * @note On focus cycle, only will be evaluated children of this container.
 *
 * @param obj The container widget
 * @param objs Chain of objects to pass focus
 * @ingroup Widget
 */
EAPI void
elm_widget_focus_custom_chain_set(Evas_Object *obj,
                                  Eina_List *objs)
{
   API_ENTRY return;

   if (!_elm_widget_focus_chain_manager_is(obj)) return;

   elm_widget_focus_custom_chain_unset(obj);

   Eina_List *l;
   Evas_Object *o;

   EINA_LIST_FOREACH(objs, l, o)
     {
        evas_object_event_callback_add(o, EVAS_CALLBACK_DEL,
                                       _elm_object_focus_chain_del_cb, sd);
     }

   sd->focus_chain = objs;
}

/**
 * @internal
 *
 * Get custom focus chain
 *
 * @param obj The container widget
 * @ingroup Widget
 */
EAPI const Eina_List *
elm_widget_focus_custom_chain_get(const Evas_Object *obj)
{
   API_ENTRY return NULL;
   return (const Eina_List *)sd->focus_chain;
}

/**
 * @internal
 *
 * Unset custom focus chain
 *
 * @param obj The container widget
 * @ingroup Widget
 */
EAPI void
elm_widget_focus_custom_chain_unset(Evas_Object *obj)
{
   API_ENTRY return;
   Eina_List *l, *l_next;
   Evas_Object *o;

   EINA_LIST_FOREACH_SAFE(sd->focus_chain, l, l_next, o)
     {
        evas_object_event_callback_del_full(o, EVAS_CALLBACK_DEL,
                                            _elm_object_focus_chain_del_cb, sd);
        sd->focus_chain = eina_list_remove_list(sd->focus_chain, l);
     }
}

/**
 * @internal
 *
 * Append object to custom focus chain.
 *
 * @note If relative_child equal to NULL or not in custom chain, the object
 * will be added in end.
 *
 * @note On focus cycle, only will be evaluated children of this container.
 *
 * @param obj The container widget
 * @param child The child to be added in custom chain
 * @param relative_child The relative object to position the child
 * @ingroup Widget
 */
EAPI void
elm_widget_focus_custom_chain_append(Evas_Object *obj,
                                     Evas_Object *child,
                                     Evas_Object *relative_child)
{
   API_ENTRY return;
   EINA_SAFETY_ON_NULL_RETURN(child);

   if (!_elm_widget_focus_chain_manager_is(obj)) return;

   evas_object_event_callback_add(child, EVAS_CALLBACK_DEL,
                                       _elm_object_focus_chain_del_cb, sd);
   if (!relative_child)
     sd->focus_chain = eina_list_append(sd->focus_chain, child);
   else
     sd->focus_chain = eina_list_append_relative(sd->focus_chain,
                                                 child, relative_child);
}

/**
 * @internal
 *
 * Prepend object to custom focus chain.
 *
 * @note If relative_child equal to NULL or not in custom chain, the object
 * will be added in begin.
 *
 * @note On focus cycle, only will be evaluated children of this container.
 *
 * @param obj The container widget
 * @param child The child to be added in custom chain
 * @param relative_child The relative object to position the child
 * @ingroup Widget
 */
EAPI void
elm_widget_focus_custom_chain_prepend(Evas_Object *obj,
                                      Evas_Object *child,
                                      Evas_Object *relative_child)
{
   API_ENTRY return;
   EINA_SAFETY_ON_NULL_RETURN(child);

   if (!_elm_widget_focus_chain_manager_is(obj)) return;

   evas_object_event_callback_add(child, EVAS_CALLBACK_DEL,
                                       _elm_object_focus_chain_del_cb, sd);
   if (!relative_child)
     sd->focus_chain = eina_list_prepend(sd->focus_chain, child);
   else
     sd->focus_chain = eina_list_prepend_relative(sd->focus_chain,
                                                  child, relative_child);
}

/**
 * @internal
 *
 * Give focus to next object in object tree.
 *
 * Give focus to next object in focus chain of one object sub-tree.
 * If the last object of chain already have focus, the focus will go to the
 * first object of chain.
 *
 * @param obj The widget root of sub-tree
 * @param dir Direction to cycle the focus
 *
 * @ingroup Widget
 */
EAPI void
elm_widget_focus_cycle(Evas_Object *obj,
                       Elm_Focus_Direction dir)
{
   Evas_Object *target = NULL;
   if (!_elm_widget_is(obj))
     return;
   elm_widget_focus_next_get(obj, dir, &target);
   if (target)
     {
        /* access */
        if (_elm_config->access_mode)
          {
             /* if an widget works for highlighting by itself, then give a
                chance to the widget to highlight. the widget should return
                EINA_TRUE, when it has its own work for highlighting. */
             if (_elm_access_widget_highlight(target, dir)) return;

             /* highlight cycle does not steal a focus, only after window gets
                the ECORE_X_ATOM_E_ILLUME_ACCESS_ACTION_ACTIVATE message,
                target will steal focus, or focus its own job. */
             if (!_elm_access_auto_highlight_get())
               elm_widget_focus_steal_with_direction(target, dir);

             _elm_access_highlight_set(target, EINA_TRUE);
             _elm_widget_focus_region_show(target);
          }
        else
          elm_widget_focus_steal_with_direction(target, dir);
     }
}

/**
 * @internal
 *
 * Give focus to near object(in object tree) in one direction.
 *
 * Give focus to near object(in object tree) in direction of current
 * focused object.  If none focusable object in given direction or
 * none focused object in object tree, the focus will not change.
 *
 * @param obj The reference widget
 * @param degree Degree changes clockwise. i.e. 0-degree: Up,
 *               90-degree: Right, 180-degree: Down, and 270-degree: Left
 * @return EINA_TRUE if focus is moved.
 *
 * @ingroup Widget
 */
EAPI Eina_Bool
elm_widget_focus_direction_go(Evas_Object *obj,
                              double degree)
{
   Evas_Object *target = NULL;
   Evas_Object *current_focused = NULL;
   double weight = 0.0;

   if (!_elm_widget_is(obj)) return EINA_FALSE;
   if (!elm_widget_focus_get(obj)) return EINA_FALSE;

   current_focused = elm_widget_focused_object_get(obj);

   if (elm_widget_focus_direction_get
         (obj, current_focused, degree, &target, &weight))
     {
        elm_widget_focus_steal(target);
        return EINA_TRUE;
     }
   return EINA_FALSE;
}

static double
_direction_weight_get(const Evas_Object *obj1,
                      const Evas_Object *obj2,
                      double degree)
{
   Evas_Coord obj_x1, obj_y1, w1, h1, obj_x2, obj_y2, w2, h2;
   double x1, yy1, x2, yy2, xx1, yyy1, xx2, yyy2;
   double ax, ay, cx, cy;
   double weight = -1.0, g = 0.0;

   if (obj1 == obj2) return 0.0;

   degree -= 90.0;
   while (degree >= 360.0)
     degree -= 360.0;
   while (degree < 0.0)
     degree += 360.0;

   evas_object_geometry_get(obj1, &obj_x1, &obj_y1, &w1, &h1);
   cx = obj_x1 + (w1 / 2.0);
   cy = obj_y1 + (h1 / 2.0);
   evas_object_geometry_get(obj2, &obj_x2, &obj_y2, &w2, &h2);

   /* For overlapping cases. */
   if (ELM_RECTS_INTERSECT(obj_x1, obj_y1, w1, h1, obj_x2, obj_y2, w2, h2))
     return 0.0;

   /* Change all points to relative one. */
   x1 = obj_x1 - cx;
   xx1 = x1 + w1;
   yy1 = obj_y1 - cy;
   yyy1 = yy1 + h1;
   x2 = obj_x2 - cx;
   xx2 = x2 + w2;
   yy2 = obj_y2 - cy;
   yyy2 = yy2 + h2;

   /* Get crossing points (ax, ay) between obj1 and a line extending
    * to the direction of current degree. */
   if (degree == 0.0)
     {
        ax = xx1;
        ay = 0.0;
     }
   else if (degree == 90.0)
     {
        ax = 0.0;
        ay = yyy1;
     }
   else if (degree == 180.0)
     {
        ax = x1;
        ay = 0.0;
     }
   else if (degree == 270.0)
     {
        ax = 0.0;
        ay = yy1;
     }
   else
     {
        g = tan(degree * (M_PI / 180.0));
        if ((degree > 0.0) && (degree < 90.0))
          {
             ay = g * xx1;
             if (ay <= yyy1) ax = xx1;
             else
               {
                  ax = yyy1 / g;
                  ay = yyy1;
               }
          }
        else if ((degree > 90.0) && (degree < 180.0))
          {
             ay = g * x1;
             if (ay <= yyy1) ax = x1;
             else
               {
                  ax = yyy1 / g;
                  ay = yyy1;
               }
          }
        else if ((degree > 180.0) && (degree < 270.0))
          {
             ay = g * x1;
             if (ay >= yy1) ax = x1;
             else
               {
                  ax = yy1 / g;
                  ay = yy1;
               }
          }
        else
          {
             ay = g * xx1;
             if (ay >= yy1) ax = xx1;
             else
               {
                  ax = yy1 / g;
                  ay = yy1;
               }
          }
     }

   /* Filter obj2, if it is not in the specific derection. */
   int i = 0;
   double rx[4] = {0.0, 0.0, 0.0, 0.0}, ry[4] = {0.0, 0.0, 0.0, 0.0};
   double t1, t2, u1, v1, u2, v2;

   if ((degree == 45.0) || (degree == 225.0) || (degree == 135.0) ||
       (degree == 315.0))
     {
        u1 = 1.0;
        v1 = 0.0;
        u2 = 0.0;
        v2 = 1.0;
     }
   else
     {
        double g2 = tan((degree + 45.0) * (M_PI / 180.0));
        u1 = (-1.0 * g2);
        u2 = (1.0 / g2);
        v1 = v2 = 1.0;
     }
   t1 = (u1 * ax) + (v1 * ay);
   t2 = (u2 * ax) + (v2 * ay);

#define _R(x) (int)((x + 0.05) * 10.0)

   if ((_R(t1 * ((u1 * x2) + (v1 * yy2))) > 0) && (_R(t2 * ((u2 * x2) +
                                                            (v2 * yy2))) > 0))
     {
        rx[i] = x2;
        ry[i++] = yy2;
     }
   if ((_R(t1 * ((u1 * x2) + (v1 * yyy2))) > 0) && (_R(t2 * ((u2 * x2) +
                                                             (v2 * yyy2))) > 0))
     {
        rx[i] = x2;
        ry[i++] = yyy2;
     }
   if ((_R(t1 * ((u1 * xx2) + (v1 * yy2))) > 0) && (_R(t2 * ((u2 * xx2) +
                                                             (v2 * yy2))) > 0))
     {
        rx[i] = xx2;
        ry[i++] = yy2;
     }
   if ((_R(t1 * ((u1 * xx2) + (v1 * yyy2))) > 0) &&
       (_R(t2 * ((u2 * xx2) + (v2 * yyy2))) > 0))
     {
        rx[i] = xx2;
        ry[i++] = yyy2;
     }
   if (i == 0)
     {
        if (degree == 0.0)
          {
             if ((_R(xx2) < 0) || (_R(yy2) > _R(yyy1)) || (_R(yyy2) < _R(yy1))) return 0.0;
          }
        else if (degree == 90.0)
          {
             if ((_R(yyy2) < 0) || (_R(x2) > _R(xx1)) || (_R(xx2) < _R(x1))) return 0.0;
          }
        else if (degree == 180.0)
          {
             if ((_R(x2) > 0) || (_R(yy2) > _R(yyy1)) || (_R(yyy2) < _R(yy1))) return 0.0;
          }
        else if (degree == 270.0)
          {
             if ((_R(yy2) > 0) || (_R(x2) > _R(xx1)) || (_R(xx2) < _R(x1))) return 0.0;
          }
        else
          {
             if ((_R(g * x2) >= _R(yy2)) && (_R((g * x2)) <= _R(yyy2)))
               {
                  if (!((_R(ax * x2) > 0) && (_R(ay * (g * x2)) > 0)))
                    return 0.0;
               }
             else if ((_R(g * xx2) >= _R(yy2)) && (_R((g * xx2)) <= _R(yyy2)))
               {
                  if (!((_R(ax * xx2) > 0) && (_R(ay * (g * xx2)) > 0)))
                    return 0.0;
               }
             else if ((_R((1.0 / g) * yy2) >= _R(xx2)) && (_R((1.0 / g) * yy2)
                                                           <= _R(xx2)))
               {
                  if (!((_R(ax * ((1.0 / g) * yy2)) > 0)
                        && (_R(ay * yy2) > 0)))
                    return 0.0;
               }
             else if ((_R((1.0 / g) * yyy2) >= _R(xx2)) &&
                      (_R((1.0 / g) * yyy2) <= _R(xx2)))
               {
                  if (!((_R(ax * ((1.0 / g) * yyy2)) > 0)
                        && (_R(ay * yyy2) > 0))) return 0.0;
               }
             else return 0.0;
          }
     }

   /* Calculate the weight for obj2. */
   if (degree == 0.0)
     {
        if (_R(xx1) > _R(x2)) weight = -1.0;
        else if ((_R(yy2) >= _R(yy1)) && (_R(yyy2) <= _R(yyy1)))
          weight = (x2 - xx1) * (x2 - xx1);
        else if (_R(yy2) > 0)
          weight = ((x2 - xx1) * (x2 - xx1)) + (yy2 * yy2);
        else if (_R(yyy2) < 0)
          weight = ((x2 - xx1) * (x2 - xx1)) + (yyy2 * yyy2);
        else weight = (x2 - xx1) * (x2 - xx1);
     }
   else if (degree == 90.0)
     {
        if (_R(yyy1) > _R(yy2)) weight = -1.0;
        else if ((_R(x2) >= _R(x1)) && (_R(xx2) <= _R(xx1)))
          weight = (yy2 - yyy1) * (yy2 - yyy1);
        else if (_R(x2) > 0)
          weight = (x2 * x2) + ((yy2 - yyy1) * (yy2 - yyy1));
        else if (_R(xx2) < 0)
          weight = (xx2 * xx2) + ((yy2 - yyy1) * (yy2 - yyy1));
        else weight = (yy2 - yyy1) * (yy2 - yyy1);
     }
   else if (degree == 180.0)
     {
        if (_R(x1) < _R(xx2)) weight = -1.0;
        else if ((_R(yy2) >= _R(yy1)) && (_R(yyy2) <= _R(yyy1)))
          weight = (x1 - xx2) * (x1 - xx2);
        else if (_R(yy2) > 0)
          weight = ((x1 - xx2) * (x1 - xx2)) + (yy2 * yy2);
        else if (_R(yyy2) < 0)
          weight = ((x1 - xx2) * (x1 - xx2)) + (yyy2 * yyy2);
        else weight = (x1 - xx2) * (x1 - xx2);
     }
   else if (degree == 270.0)
     {
        if (_R(yy1) < _R(yyy2)) weight = -1.0;
        else if ((_R(x2) >= _R(x1)) && (_R(xx2) <= _R(xx1)))
          weight = (yy1 - yyy2) * (yy1 - yyy2);
        else if (_R(x2) > 0)
          weight = (x2 * x2) + ((yy1 - yyy2) * (yy1 - yyy2));
        else if (_R(xx2) < 0)
          weight = (xx2 * xx2) + ((yy1 - yyy2) * (yy1 - yyy2));
        else weight = (yy1 - yyy2) * (yy1 - yyy2);
     }
   else
     {
        int j = 0, k = 0;
        double sx[4] = {0.0, 0.0, 0.0, 0.0}, sy[4] = {0.0, 0.0, 0.0, 0.0};
        double t_weight[4] = {-1.0, -1.0, -1.0, -1.0};
        if ((_R(g * x2) >= _R(yy2)) && (_R(g * x2) <= _R(yyy2)))
          {
             sx[j] = x2;
             sy[j] = g * x2;
             t_weight[j++] = ((ax - x2) * (ax - x2)) +
               ((ay - (g * x2)) * (ay - (g * x2)));
          }
        if ((_R(g * xx2) >= _R(yy2)) && (_R(g * xx2) <= _R(yyy2)))
          {
             sx[j] = xx2;
             sy[j] = g * xx2;
             t_weight[j++] = ((ax - xx2) * (ax - xx2)) +
               ((ay - (g * xx2)) * (ay - (g * xx2)));
          }
        if ((_R((1.0 / g) * yy2) >= _R(x2)) && (_R((1.0 / g) * yy2) <= _R(xx2)))
          {
             sx[j] = (1.0 / g) * yy2;
             sy[j] = yy2;
             t_weight[j++] =
               ((ax - ((1.0 / g) * yy2)) * (ax - ((1.0 / g) * yy2))) +
               ((ay - yy2) * (ay - yy2));
          }
        if ((_R((1.0 / g) * yyy2) >= _R(x2)) && (_R((1.0 / g) * yyy2)
                                                 <= _R(xx2)))
          {
             sx[j] = (1.0 / g) * yyy2;
             sy[j] = yyy2;
             t_weight[j++] =
               ((ax - ((1.0 / g) * yyy2)) * (ax - ((1.0 / g) * yyy2))) +
               ((ay - yyy2) * (ay - yyy2));
          }

        if ((j > 2) || ((j == 2) && ((_R(sx[0]) != _R(sx[1])) ||
                                     (_R(sy[0]) != _R(sy[1])))))
          {
             for (; k < j; k++)
               {
                  if (_R(t_weight[k]) == 0) return -1.0;
                  if ((1 / weight) < (1 / t_weight[k])) weight = t_weight[k];
               }
          }
        else
          {
             for (; k < i; k++)
               {
                  double ccx, ccy, t1_weight, x_diff, y_diff;
                  ccx = ((1.0 / g) * rx[k] + ry[k]) / (g + (1.0 / g));
                  ccy = g * ccx;
                  x_diff = rx[k] - ccx;
                  if (x_diff < 0) x_diff *= -1.0;
                  y_diff = ry[k] - ccy;
                  if (y_diff < 0) y_diff *= -1.0;
                  t1_weight =
                    (((ax - ccx) * (ax - ccx)) + ((ay - ccy) * (ay - ccy))) +
                    ((x_diff * x_diff * x_diff) + (y_diff * y_diff * y_diff));
                  if ((_R(t1_weight) != 0) && ((1 / weight) < (1 / t1_weight)))
                    weight = t1_weight;
               }
          }
     }
   /* Return the current object's weight. */
   if (weight == -1.0) return 0.0;
   if (_R(weight) == 0) return -1.0;

#undef _R

   return 1.0 / weight;
}

/**
 * @internal
 *
 * Get near object in one direction of base object.
 *
 * Get near object(in the object sub-tree) in one direction of
 * base object. Return the near object by reference.
 * By initializing weight, you can filter objects locating far
 * from base object. If object is in the specific direction,
 * weight is (1/(distance^2)). If object is not exactly in one
 * direction, some penalty will be added.
 *
 * @param obj The widget root of sub-tree
 * @param base The base object of the direction
 * @param degree Degree changes clockwise. i.e. 0-degree: Up,
 *               90-degree: Right, 180-degree: Down, and 270-degree: Left
 * @param direction The near object in one direction
 * @param weight The weight is bigger when the object is located near
 * @return EINA_TRUE if near object is updated.
 *
 * @ingroup Widget
 */
EAPI Eina_Bool
elm_widget_focus_direction_get(const Evas_Object *obj,
                               const Evas_Object *base,
                               double degree,
                               Evas_Object **direction,
                               double *weight)
{
   double c_weight;

   API_ENTRY return EINA_FALSE;

   /* -1 means the best was already decided. Don't need any more searching. */
   if (!direction || !weight || !base || (obj == base))
     return EINA_FALSE;

   /* Ignore if disabled */
   if ((!evas_object_visible_get(obj))
       || (elm_widget_disabled_get(obj))
       || (elm_widget_tree_unfocusable_get(obj)))
     return EINA_FALSE;

   if (!sd->api) return EINA_FALSE;

   /* Try use hook */
   if (_elm_widget_focus_direction_manager_is(obj))
     return sd->api->focus_direction(obj, base, degree, direction, weight);

   if (!elm_widget_can_focus_get(obj) || elm_widget_focus_get(obj))
     return EINA_FALSE;

   c_weight = _direction_weight_get(base, obj, degree);
   if ((c_weight == -1.0) ||
       ((c_weight != 0.0) && (*weight != -1.0) &&
        ((int)(*weight * 1000000) <= (int)(c_weight * 1000000))))
     {
        if ((int)(*weight * 1000000) == (int)(c_weight * 1000000))
          {
             Elm_Widget_Smart_Data *sd1 =
               evas_object_smart_data_get(*direction);
             if (sd1)
               {
                  if (sd->focus_order <= sd1->focus_order)
                    return EINA_FALSE;
               }
          }
        *direction = (Evas_Object *)obj;
        *weight = c_weight;
        return EINA_TRUE;
     }
   return EINA_FALSE;
}

/**
 * @internal
 *
 * Get near object in one direction of base object in list.
 *
 * Get near object in one direction of base object in the specific
 * object list. Return the near object by reference.
 * By initializing weight, you can filter objects locating far
 * from base object. If object is in the specific direction,
 * weight is (1/(distance^2)). If object is not exactly in one
 * direction, some penalty will be added.
 *
 * @param obj The widget root of sub-tree
 * @param base The base object of the direction
 * @param items list with ordered objects
 * @param list_data_get function to get the object from one item of list
 * @param degree Degree changes clockwise. i.e. 0-degree: Up,
 *               90-degree: Right, 180-degree: Down, and 270-degree: Left
 * @param direction The near object in one direction
 * @param weight The weight is bigger when the object is located near
 * @return EINA_TRUE if near object is updated.
 *
 * @ingroup Widget
 */
EAPI Eina_Bool
elm_widget_focus_list_direction_get(const Evas_Object *obj,
                                    const Evas_Object *base,
                                    const Eina_List *items,
                                    void *(*list_data_get)(const Eina_List *l),
                                    double degree,
                                    Evas_Object **direction,
                                    double *weight)
{
   API_ENTRY return EINA_FALSE;
   if (!direction || !weight || !base || !items)
     return EINA_FALSE;

   const Eina_List *l = items;
   Evas_Object *current_best = *direction;
   //TIZEN ONLY(20131128) : For focus debugging.
   Evas_Coord x, y, w, h, xx, yy, ww, hh;
   evas_object_geometry_get(base, &x, &y, &w, &h);
   DBG("--[FOCUS] obj(%p::%s) base(%p::%s) %d %d %d %d", obj, elm_widget_type_get(obj),
            base, elm_widget_type_get(base), x, y, w, h);
   ///////
   for (; l; l = eina_list_next(l))
     {
        Evas_Object *cur = list_data_get(l);
        elm_widget_focus_direction_get(cur, base, degree, direction, weight);
        //TIZEN ONLY(20131128) : For focus debugging.
        evas_object_geometry_get(cur, &xx, &yy, &ww, &hh);
        DBG("---[FOCUS] cur(%p::%s) weight(%lf) %d %d %d %d",
            cur, elm_widget_type_get(cur), *weight, xx, yy, ww, hh);
        ////////////////
     }
   if (current_best != *direction)
     return EINA_TRUE;
   else
     return EINA_FALSE;
}

/**
 * @internal
 *
 * Get next object in focus chain of object tree.
 *
 * Get next object in focus chain of one object sub-tree.
 * Return the next object by reference. If don't have any candidate to receive
 * focus before chain end, the first candidate will be returned.
 *
 * @param obj The widget root of sub-tree
 * @param dir Direction os focus chain
 * @param next The next object in focus chain
 * @return EINA_TRUE if don't need focus chain restart/loop back
 *         to use 'next' obj.
 *
 * @ingroup Widget
 */
EAPI Eina_Bool
elm_widget_focus_next_get(const Evas_Object *obj,
                          Elm_Focus_Direction dir,
                          Evas_Object **next)
{
   Elm_Access_Info *ac;

   if (!next)
     return EINA_FALSE;
   *next = NULL;

   API_ENTRY return EINA_FALSE;
   DBG("[FOCUS] obj(%p::%s) dir(%d)", obj, elm_widget_type_get(obj), dir);
   /* Ignore if disabled */
   if (_elm_config->access_mode && _elm_access_auto_highlight_get())
     {
        if (!evas_object_visible_get(obj)
            || (elm_widget_tree_unfocusable_get(obj)))
          return EINA_FALSE;
     }
   else
     {
        if ((!evas_object_visible_get(obj))
            || (elm_widget_disabled_get(obj))
            || (elm_widget_tree_unfocusable_get(obj)))
          {
             DBG("[FOCUS] = obj(%p::%s) %d:%d:%d return(0)", obj,
                 elm_widget_type_get(obj),
                 evas_object_visible_get(obj),
                 elm_widget_disabled_get(obj),
                 elm_widget_tree_unfocusable_get(obj));
             return EINA_FALSE;
          }
     }

   if (!sd->api) return EINA_FALSE;

   /* Try use hook */
   if (_elm_widget_focus_chain_manager_is(obj))
     {
        Eina_Bool ret;
        ret = sd->api->focus_next(obj, dir, next);
        if (!ret && ELM_WIDGET_FOCUS_GET(obj))
          {
             Evas_Object *o = NULL;
             if (_elm_access_auto_highlight_get())
               {
                  if (dir == ELM_FOCUS_PREVIOUS)
                    o = sd->highlight_previous;
                  else if (dir == ELM_FOCUS_NEXT)
                    o = sd->highlight_next;
               }
             else
               {
                  if (dir == ELM_FOCUS_PREVIOUS)
                    o = sd->focus_previous;
                  else if (dir == ELM_FOCUS_NEXT)
                    o = sd->focus_next;
                  else if (dir == ELM_FOCUS_UP)
                    o = sd->focus_up;
                  else if (dir == ELM_FOCUS_DOWN)
                    o = sd->focus_down;
                  else if (dir == ELM_FOCUS_RIGHT)
                    o = sd->focus_right;
                  else if (dir == ELM_FOCUS_LEFT)
                    o = sd->focus_left;
               }

             if (o)
               {
                  *next = o;
                  if (_elm_access_auto_highlight_get())
                    {
                       elm_widget_focus_next_get(o, dir, next);
                    }
                  DBG("[FOCUS] = obj(%p::%s) next(%p:%s) return(1)",
                      obj, elm_widget_type_get(obj),
                      *next, elm_widget_type_get(*next));

                  if (dir == ELM_FOCUS_PREVIOUS &&
                      sd->end_dir == ELM_HIGHLIGHT_DIR_PREVIOUS)
                    {
                       return EINA_FALSE;
                    }
                  if (dir == ELM_FOCUS_NEXT &&
                      sd->end_dir == ELM_HIGHLIGHT_DIR_NEXT)
                    {
                       return EINA_FALSE;
                    }

                  return EINA_TRUE;
               }
          }
        DBG("[FOCUS] = obj(%p::%s) next(%p:%s) return(%d)",
            obj, elm_widget_type_get(obj),
            *next, elm_widget_type_get(*next), ret);
        return ret;
     }

   /* access object does not check sd->can_focus, because an object could
      have highlight even though the object is not focusable. */
   if (_elm_config->access_mode && _elm_access_auto_highlight_get())
     {
        ac = _elm_access_object_get(obj);
        if (!ac) return EINA_FALSE;

        /* check whether the hover object is visible or not */
        if (!evas_object_visible_get(ac->hoverobj))
          return EINA_FALSE;
     }
   else if (!elm_widget_can_focus_get(obj))
     {
        DBG("[FOCUS] = obj(%p::%s) return(0)", obj, elm_widget_type_get(obj));
        return EINA_FALSE;
     }

   if (ELM_WIDGET_FOCUS_GET(obj))
     {
        if (_elm_access_auto_highlight_get())
          {
             if (dir == ELM_FOCUS_PREVIOUS)
               *next = sd->highlight_previous;
             else if (dir == ELM_FOCUS_NEXT)
               *next = sd->highlight_next;
          }
        else
          {
             if (dir == ELM_FOCUS_PREVIOUS)
               *next = sd->focus_previous;
             else if (dir == ELM_FOCUS_NEXT)
               *next = sd->focus_next;
             else if (dir == ELM_FOCUS_UP)
               *next = sd->focus_up;
             else if (dir == ELM_FOCUS_DOWN)
               *next = sd->focus_down;
             else if (dir == ELM_FOCUS_RIGHT)
               *next = sd->focus_right;
             else if (dir == ELM_FOCUS_LEFT)
               *next = sd->focus_left;
          }

        if (*next)
          {
             if (_elm_access_auto_highlight_get())
               {
                  Evas_Object *o = *next;
                  elm_widget_focus_next_get(o, dir, next);
               }
             DBG("[FOCUS] = by direction obj(%p::%s) return(1)", obj, elm_widget_type_get(obj));

             if (dir == ELM_FOCUS_PREVIOUS &&
                 sd->end_dir == ELM_HIGHLIGHT_DIR_PREVIOUS)
               {
                  return EINA_FALSE;
               }
             if (dir == ELM_FOCUS_NEXT &&
                 sd->end_dir == ELM_HIGHLIGHT_DIR_NEXT)
               {
                  return EINA_FALSE;
               }

             return EINA_TRUE;
          }
     }
   /* Return */
   *next = (Evas_Object *)obj;
   return !ELM_WIDGET_FOCUS_GET(obj);
}

/**
 * @internal
 *
 * Get next object in focus chain of object tree in list.
 *
 * Get next object in focus chain of one object sub-tree ordered by one list.
 * Return the next object by reference. If don't have any candidate to receive
 * focus before list end, the first candidate will be returned.
 *
 * @param obj The widget root of sub-tree
 * @param dir Direction os focus chain
 * @param items list with ordered objects
 * @param list_data_get function to get the object from one item of list
 * @param next The next object in focus chain
 * @return EINA_TRUE if don't need focus chain restart/loop back
 *         to use 'next' obj.
 *
 * @ingroup Widget
 */
EAPI Eina_Bool
elm_widget_focus_list_next_get(const Evas_Object *obj,
                               const Eina_List *items,
                               void *(*list_data_get)(const Eina_List *list),
                               Elm_Focus_Direction dir,
                               Evas_Object **next)
{
   Eina_List *(*list_next)(const Eina_List *list) = NULL;
   Evas_Object *focused_object = NULL;

   if (!next)
     return EINA_FALSE;
   *next = NULL;

   if (!_elm_widget_is(obj))
     return EINA_FALSE;

   if (!items)
     return EINA_FALSE;

   /* When Up, Down, Right, or Left, try direction_get first. */
   focused_object = elm_widget_focused_object_get(obj);
   //TIZEN ONLY(20131128) : For focus debugging.
   DBG("[FOCUS] obj(%p::%s) cur_focused(%p::%s) dir(%d)",
       obj, elm_widget_type_get(obj),
       focused_object, elm_widget_type_get(focused_object), dir);

   if (focused_object)
     {
        if((dir == ELM_FOCUS_UP)
           || (dir == ELM_FOCUS_DOWN)
           || (dir == ELM_FOCUS_RIGHT)
           || (dir == ELM_FOCUS_LEFT))
          {
             *next = elm_widget_focus_next_object_get(focused_object, dir);
             if (*next)
               {
                  DBG("-[FOCUS] next(%p::%s)", *next, elm_widget_type_get(*next));
                  return EINA_TRUE;
               }
             else
               {
                  Evas_Object *n = NULL;
                  double degree;
                  double weight;

                  if (dir == ELM_FOCUS_UP) degree = 0.0;
                  else if (dir == ELM_FOCUS_DOWN) degree = 180.0;
                  else if (dir == ELM_FOCUS_RIGHT) degree = 90.0;
                  else if (dir == ELM_FOCUS_LEFT) degree = 270.0;

                  if (elm_widget_focus_list_direction_get(obj, focused_object,
                                                          items, list_data_get,
                                                          degree, &n, &weight))
                    {
                       *next = n;
                       //TIZEN ONLY(20131128) : For focus debugging.
                       DBG("-[FOCUS] next(%p::%s)", *next, elm_widget_type_get(*next));
                       return EINA_TRUE;
                    }
               }
          }
     }

   /* Direction */
   if (dir == ELM_FOCUS_PREVIOUS)
     {
        items = eina_list_last(items);
        list_next = eina_list_prev;
     }
   else if ((dir == ELM_FOCUS_NEXT)
            || (dir == ELM_FOCUS_UP)
            || (dir == ELM_FOCUS_DOWN)
            || (dir == ELM_FOCUS_RIGHT)
            || (dir == ELM_FOCUS_LEFT))
     list_next = eina_list_next;
   else
     return EINA_FALSE;

   const Eina_List *l = items;

   /* Recovery last focused sub item */
   if (ELM_WIDGET_FOCUS_GET(obj))
     {
        for (; l; l = list_next(l))
          {
             Evas_Object *cur = list_data_get(l);
             if (ELM_WIDGET_FOCUS_GET(cur)) break;
          }

         /* Focused object, but no focused sub item */
         if (!l) l = items;
     }

   const Eina_List *start = l;
   Evas_Object *to_focus = NULL;

   /* Interate sub items */
   /* Go to end of list */
   for (; l; l = list_next(l))
     {
        Evas_Object *tmp = NULL;
        Evas_Object *cur = list_data_get(l);

        if (elm_widget_parent_get(cur) != obj)
          continue;

        /* Try Focus cycle in subitem */
        if (elm_widget_focus_next_get(cur, dir, &tmp))
          {
             *next = tmp;
             //TIZEN ONLY(20131128) : For focus debugging.
             DBG("-[FOCUS] next(%p::%s) ret(%d)", *next, elm_widget_type_get(*next), 0);
             return EINA_TRUE;
          }
        else if ((dir == ELM_FOCUS_UP)
                 || (dir == ELM_FOCUS_DOWN)
                 || (dir == ELM_FOCUS_RIGHT)
                 || (dir == ELM_FOCUS_LEFT))
          {
             if (tmp && elm_widget_focus_get(cur))
               {
                  *next = tmp;
                  //TIZEN ONLY(20131128) : For focus debugging.
                  DBG("-[FOCUS] next(%p::%s) ret(%d)", *next, elm_widget_type_get(*next), 0);
                  return EINA_FALSE;
               }
          }
        else if ((tmp) && (!to_focus))
          to_focus = tmp;
     }

   l = items;

   /* Get First possible */
   for (; l != start; l = list_next(l))
     {
        Evas_Object *tmp = NULL;
        Evas_Object *cur = list_data_get(l);

        if (elm_widget_parent_get(cur) != obj)
          continue;

        /* Try Focus cycle in subitem */
        elm_widget_focus_next_get(cur, dir, &tmp);
        if (tmp)
          {
             *next = tmp;
             //TIZEN ONLY(20131128) : For focus debugging.
             DBG("-[FOCUS] next(%p::%s) ret(%d)", *next, elm_widget_type_get(*next), 0);
             return EINA_FALSE;
          }
     }

   *next = to_focus;
   //TIZEN ONLY(20131128) : For focus debugging.
   DBG("-[FOCUS] next(%p::%s) ret(%d)", *next, elm_widget_type_get(*next), 0);
   return EINA_FALSE;
}


/**
 * @internal
 *
 * Get next object which was set with specific focus direction.
 *
 * Get next object which was set by elm_widget_focus_next_object_set
 * with specific focus directioin.
 *
 * @param obj The widget
 * @param dir Direction of focus
 * @return Widget which was registered with sepecific focus direction.
 *
 * @ingroup Widget
 */
EAPI Evas_Object *
elm_widget_focus_next_object_get(const Evas_Object *obj, Elm_Focus_Direction dir)
{
   API_ENTRY return NULL;

   if (dir == ELM_FOCUS_PREVIOUS)
     return sd->focus_previous;
   else if (dir == ELM_FOCUS_NEXT)
     return sd->focus_next;
   else if (dir == ELM_FOCUS_UP)
     return sd->focus_up;
   else if (dir == ELM_FOCUS_DOWN)
     return sd->focus_down;
   else if (dir == ELM_FOCUS_RIGHT)
     return sd->focus_right;
   else if (dir == ELM_FOCUS_LEFT)
     return sd->focus_left;

   return NULL;
}

/**
 * @internal
 *
 * Set next object with specific focus direction.
 *
 * When a widget is set with specific focus direction, this widget will be
 * the first candidate when finding the next focus object.
 * Focus next object can be registered with six directions that are previous,
 * next, up, down, right, and left.
 *
 * @param obj The widget
 * @param next Next focus object
 * @param dir Direction of focus
 *
 * @ingroup Widget
 */
EAPI void
elm_widget_focus_next_object_set(Evas_Object *obj, Evas_Object *next, Elm_Focus_Direction dir)
{
   API_ENTRY return;

   if (dir == ELM_FOCUS_PREVIOUS)
     {
        if (sd->focus_previous)
          evas_object_event_callback_del_full(sd->focus_previous, EVAS_CALLBACK_DEL,
                                              _focus_previous_object_del_cb, sd);
        sd->focus_previous = next;
        if (next)
          evas_object_event_callback_add(next, EVAS_CALLBACK_DEL,
                                         _focus_previous_object_del_cb, sd);
     }
   else if (dir == ELM_FOCUS_NEXT)
     {
        if (sd->focus_next)
          evas_object_event_callback_del_full(sd->focus_next, EVAS_CALLBACK_DEL,
                                              _focus_next_object_del_cb, sd);
        sd->focus_next = next;
        if (next)
          evas_object_event_callback_add(next, EVAS_CALLBACK_DEL,
                                         _focus_next_object_del_cb, sd);
     }
   else if (dir == ELM_FOCUS_UP)
     {
        if (sd->focus_up)
          evas_object_event_callback_del_full(sd->focus_up, EVAS_CALLBACK_DEL,
                                              _focus_up_object_del_cb, sd);
        sd->focus_up = next;
        if (next)
          evas_object_event_callback_add(next, EVAS_CALLBACK_DEL,
                                         _focus_up_object_del_cb, sd);
     }
   else if (dir == ELM_FOCUS_DOWN)
     {
        if (sd->focus_down)
          evas_object_event_callback_del_full(sd->focus_down, EVAS_CALLBACK_DEL,
                                              _focus_down_object_del_cb, sd);
        sd->focus_down = next;
        if (next)
          evas_object_event_callback_add(next, EVAS_CALLBACK_DEL,
                                         _focus_down_object_del_cb, sd);
     }
   else if (dir == ELM_FOCUS_RIGHT)
     {
        if (sd->focus_right)
          evas_object_event_callback_del_full(sd->focus_right, EVAS_CALLBACK_DEL,
                                              _focus_right_object_del_cb, sd);
        sd->focus_right = next;
        if (next)
          evas_object_event_callback_add(next, EVAS_CALLBACK_DEL,
                                         _focus_right_object_del_cb, sd);
     }
   else if (dir == ELM_FOCUS_LEFT)
     {
        if (sd->focus_left)
          evas_object_event_callback_del_full(sd->focus_left, EVAS_CALLBACK_DEL,
                                              _focus_left_object_del_cb, sd);
        sd->focus_left = next;
        if (next)
          evas_object_event_callback_add(next, EVAS_CALLBACK_DEL,
                                         _focus_left_object_del_cb, sd);
     }
   else
     return;
}


EAPI Eina_Bool
elm_widget_highlight_get(const Evas_Object *obj)
{
   API_ENTRY return EINA_FALSE;
   return sd->highlighted;
}

EAPI void
elm_widget_highlighted_object_clear(Evas_Object *obj)
{
   API_ENTRY return;

   if (!sd->highlighted) return;

   if (sd->resize_obj && elm_widget_highlight_get(sd->resize_obj))
     elm_widget_highlighted_object_clear(sd->resize_obj);
   else
     {
        const Eina_List *l;
        Evas_Object *child;
        EINA_LIST_FOREACH(sd->subobjs, l, child)
          {
             if (elm_widget_highlight_get(child))
               {
                  elm_widget_highlighted_object_clear(child);
                  break;
               }
          }
     }
   sd->highlighted = EINA_FALSE;
}

EAPI void
elm_widget_parent_highlight_set(Evas_Object *obj,
                                Eina_Bool highlighted)
{
   API_ENTRY return;

   highlighted = !!highlighted;

   Evas_Object *o = elm_widget_parent_get(obj);

   if (o)
     {
        /* The _elm_access_object_hilight_disalbe(); which is called by
           the _access_obj_hilight_hide_cb(); calls this func with EINA_FALSE,
           after the _elm_widget_highlight_steal(); which is called by the
           _elm_access_highlight_set(); handles sd->highlighted with EINA_FALSE
           already. This makes improper highlight tree information */
        if (highlighted || (sd->highlighted != highlighted))
          elm_widget_parent_highlight_set(o, highlighted);
     }

   if (highlighted)
     {
        highlight_order++;
        sd->highlight_order = highlight_order;
     }
   sd->highlighted = highlighted;
}

EAPI void
elm_widget_highlight_steal(Evas_Object *obj)
{
   Evas_Object *parent, *parent2, *o;
   API_ENTRY return;

   if (!evas_object_visible_get(obj)) return;
   if (sd->highlighted) return;
   if (sd->tree_unfocusable) return;
   parent = obj;
   for (;;)
     {
        o = elm_widget_parent_get(parent);
        if (!o) break;
        sd = evas_object_smart_data_get(o);
        if (sd->tree_unfocusable) return;
        if (sd->highlighted) break;
        parent = o;
     }
   if ((!elm_widget_parent_get(parent)) &&
       (!elm_widget_parent2_get(parent)))
     elm_widget_highlighted_object_clear(parent);
   else
     {
        parent2 = elm_widget_parent_get(parent);
        if (!parent2) parent2 = elm_widget_parent2_get(parent);
        parent = parent2;
        sd = evas_object_smart_data_get(parent);
        if (sd)
          {
             if ((sd->resize_obj) && (elm_widget_highlight_get(sd->resize_obj)))
               elm_widget_highlighted_object_clear(sd->resize_obj);
             else
               {
                  const Eina_List *l;
                  Evas_Object *child;
                  EINA_LIST_FOREACH(sd->subobjs, l, child)
                    {
                       if (elm_widget_highlight_get(child))
                         {
                            elm_widget_highlighted_object_clear(child);
                            break;
                         }
                    }
               }
          }
     }
   elm_widget_parent_highlight_set(obj, EINA_TRUE);

   return;
}

EAPI void
elm_widget_signal_emit(Evas_Object *obj,
                       const char *emission,
                       const char *source)
{
   API_ENTRY return;

   if (evas_object_smart_type_check(obj, "elm_layout"))
     elm_layout_signal_emit(obj, emission, source);
   else if (evas_object_smart_type_check(obj, "elm_icon"))
     {
        WRN("Deprecated function. This functionality on icon objects"
            " will be dropped on a next release.");
        _elm_icon_signal_emit(obj, emission, source);
     }
}

EAPI void
elm_widget_signal_callback_add(Evas_Object *obj,
                               const char *emission,
                               const char *source,
                               Edje_Signal_Cb func,
                               void *data)
{
   API_ENTRY return;

   EINA_SAFETY_ON_NULL_RETURN(func);

   if (evas_object_smart_type_check(obj, "elm_layout"))
     elm_layout_signal_callback_add(obj, emission, source, func, data);
   else if (evas_object_smart_type_check(obj, "elm_icon"))
     {
        WRN("Deprecated function. This functionality on icon objects"
            " will be dropped on a next release.");

        _elm_icon_signal_callback_add(obj, emission, source, func, data);
     }
}

EAPI void *
elm_widget_signal_callback_del(Evas_Object *obj,
                               const char *emission,
                               const char *source,
                               Edje_Signal_Cb func)
{
   void *data = NULL;

   API_ENTRY return NULL;

   if (evas_object_smart_type_check(obj, "elm_layout"))
     data = elm_layout_signal_callback_del(obj, emission, source, func);
   else if (evas_object_smart_type_check(obj, "elm_icon"))
     {
        WRN("Deprecated function. This functionality on icon objects"
            " will be dropped on a next release.");

        data = _elm_icon_signal_callback_del(obj, emission, source, func);
     }

   return data;
}

EAPI void
elm_widget_focus_set(Evas_Object *obj,
                     int first)
{
   API_ENTRY return;

   if (!sd->api) return;

   if (!sd->focused)
     {
        Elm_Focus_Info fi;

        fi.prev_focused = elm_widget_focused_object_get(elm_widget_top_get(obj));
        fi.dir = ELM_FOCUS_NONE;
        focus_order++;
        sd->focus_order = focus_order;
        sd->focused = EINA_TRUE;
        sd->api->on_focus(obj, &fi);
     }

   if (first)
     {
        if ((_is_focusable(sd->resize_obj)) &&
            (!elm_widget_disabled_get(sd->resize_obj)))
          {
             elm_widget_focus_set(sd->resize_obj, first);
          }
        else
          {
             const Eina_List *l;
             Evas_Object *child;

             EINA_LIST_FOREACH(sd->subobjs, l, child)
               {
                  if ((_is_focusable(child)) &&
                      (!elm_widget_disabled_get(child)))
                    {
                       elm_widget_focus_set(child, first);
                       break;
                    }
               }
          }
     }
   else
     {
        const Eina_List *l;
        Evas_Object *child;

        EINA_LIST_REVERSE_FOREACH(sd->subobjs, l, child)
          {
             if ((_is_focusable(child)) &&
                 (!elm_widget_disabled_get(child)))
               {
                  elm_widget_focus_set(child, first);
                  break;
               }
          }
     }
}

EAPI Evas_Object *
elm_widget_parent_get(const Evas_Object *obj)
{
   API_ENTRY return NULL;
   return sd->parent_obj;
}

EAPI void
elm_widget_focused_object_clear(Evas_Object *obj)
{
   API_ENTRY return;

   if (!sd->api) return;

   if (!sd->focused) return;

   // FIXME: evas_object_ref/unref is temporary code to fix logical issue.
   // After Eo is applied to elementary, remove these.
   evas_object_ref(obj);
   if (sd->resize_obj && elm_widget_focus_get(sd->resize_obj))
     elm_widget_focused_object_clear(sd->resize_obj);
   else
     {
        const Eina_List *l;
        Evas_Object *child;
        EINA_LIST_FOREACH(sd->subobjs, l, child)
          {
             if (elm_widget_focus_get(child))
               {
                  elm_widget_focused_object_clear(child);
                  break;
               }
          }
     }
   sd->focused = EINA_FALSE;
   sd->api->on_focus(obj, NULL);
   evas_object_unref(obj);
}

EAPI void
elm_widget_focus_steal_with_direction(Evas_Object *obj, Elm_Focus_Direction dir)
{
   Evas_Object *parent, *parent2, *o, *prev_focused;
   API_ENTRY return;

   if (!evas_object_visible_get(obj)) return;
   if (sd->focused) return;
   if (sd->disabled) return;
   if (!sd->can_focus) return;
   if (sd->tree_unfocusable) return;
   parent = obj;
   for (;; )
     {
        o = elm_widget_parent_get(parent);
        if (!o) break;
        sd = evas_object_smart_data_get(o);
        if (sd->disabled || sd->tree_unfocusable) return;
        if (sd->focused) break;
        parent = o;
     }
   prev_focused = elm_widget_focused_object_get(o);
   if ((!elm_widget_parent_get(parent)) &&
       (!elm_widget_parent2_get(parent)))
     elm_widget_focused_object_clear(parent);
   else
     {
        parent2 = elm_widget_parent_get(parent);
        if (!parent2) parent2 = elm_widget_parent2_get(parent);
        parent = parent2;
        sd = evas_object_smart_data_get(parent);
        if (sd)
          {
             if ((sd->resize_obj) && (elm_widget_focus_get(sd->resize_obj)))
               elm_widget_focused_object_clear(sd->resize_obj);
             else
               {
                  const Eina_List *l;
                  Evas_Object *child;
                  EINA_LIST_FOREACH(sd->subobjs, l, child)
                    {
                       if (elm_widget_focus_get(child))
                         {
                            elm_widget_focused_object_clear(child);
                            break;
                         }
                    }
               }
          }
     }
   _parent_focus(obj, dir, prev_focused);
   _elm_widget_focus_region_show(obj);

   return;
}

EAPI void
elm_widget_focus_steal(Evas_Object *obj)
{
   elm_widget_focus_steal_with_direction(obj, ELM_FOCUS_NONE);
}

EAPI void
elm_widget_focus_restore(Evas_Object *obj)
{
   Evas_Object *newest = NULL, *access_newest = NULL;
   API_ENTRY return;

   if (_elm_config->access_mode)
     {
        unsigned int newest_focus_order = 0;
        _elm_access_auto_highlight_set(EINA_TRUE);
        access_newest = elm_widget_newest_focus_order_get
           (obj, &newest_focus_order, EINA_FALSE);
        _elm_access_auto_highlight_set(EINA_FALSE);
     }

   if (!sd->focused)
     {
        unsigned int newest_focus_order = 0;
        newest = elm_widget_newest_focus_order_get
           (obj, &newest_focus_order, EINA_TRUE);

        if (newest)
          {
             elm_object_focus_set(newest, EINA_FALSE);
             elm_object_focus_set(newest, EINA_TRUE);
          }
     }

   if (access_newest)
     _elm_access_highlight_set(access_newest, EINA_TRUE);
}

void
_elm_widget_top_win_focused_set(Evas_Object *obj,
                                Eina_Bool top_win_focused)
{
   const Eina_List *l;
   Evas_Object *child;
   API_ENTRY return;

   if (sd->top_win_focused == top_win_focused) return;
   EINA_LIST_FOREACH(sd->subobjs, l, child)
     {
        const char *type = elm_widget_type_get(child);
        if (type && !strcmp(type, "elm_win"))
          continue;
        _elm_widget_top_win_focused_set(child, top_win_focused);
     }
   sd->top_win_focused = top_win_focused;
}

Eina_Bool
_elm_widget_top_win_focused_get(const Evas_Object *obj)
{
   API_ENTRY return EINA_FALSE;
   return sd->top_win_focused;
}

// TIZEN ONLY (20131211) : For broadcasting the state of focus highlight.
void
_elm_widget_focus_highlighted_set(Evas_Object *obj,
                                  Eina_Bool focus_highlighted)
{
   const Eina_List *l;
   Evas_Object *child;
   API_ENTRY return;

   if (sd->focus_highlighted == focus_highlighted) return;
   EINA_LIST_FOREACH(sd->subobjs, l, child)
     {
        const char *type = elm_widget_type_get(child);
        if (type && !strcmp(type, "elm_win"))
          continue;
        _elm_widget_focus_highlighted_set(child, focus_highlighted);
     }
   sd->focus_highlighted = focus_highlighted;
}

Eina_Bool
_elm_widget_focus_highlighted_get(const Evas_Object *obj)
{
   API_ENTRY return EINA_FALSE;
   return sd->focus_highlighted;
}

EAPI void
elm_widget_disabled_set(Evas_Object *obj,
                        Eina_Bool disabled)
{
   API_ENTRY return;

   Evas_Object *child;
   Eina_List *l;

   if (sd->disabled == disabled) return;
   sd->disabled = !!disabled;
   elm_widget_focus_disabled_handle(obj);
   if (!sd->api) return;
   sd->api->disable(obj);
   EINA_LIST_FOREACH(ELM_WIDGET_DATA(sd)->subobjs, l, child)
     elm_object_disabled_set(child, disabled);
}

EAPI Eina_Bool
elm_widget_disabled_get(const Evas_Object *obj)
{
   API_ENTRY return 0;
   return sd->disabled;
}

EAPI void
elm_widget_show_region_set(Evas_Object *obj,
                           Evas_Coord x,
                           Evas_Coord y,
                           Evas_Coord w,
                           Evas_Coord h,
                           Eina_Bool forceshow)
{
   Evas_Object *parent_obj, *child_obj;
   Evas_Coord px = 0, py = 0, cx = 0, cy = 0, nx = 0, ny = 0;

   API_ENTRY return;

   evas_smart_objects_calculate(evas_object_evas_get(obj));
   ELM_WIDGET_CHECK_OR_RETURN(obj);

   if (!forceshow && (x == sd->rx) && (y == sd->ry) &&
       (w == sd->rw) && (h == sd->rh)) return;

   sd->rx = x;
   sd->ry = y;
   sd->rw = w;
   sd->rh = h;
   if (sd->on_show_region)
   {
     sd->on_show_region
       (sd->on_show_region_data, obj);

     if (_elm_scrollable_is(obj))
       {
          ELM_SCROLLABLE_IFACE_GET(obj, s_iface);
          s_iface->content_pos_get(obj, &nx, &ny);
          x -= nx;
          y -= ny;
       }
   }

   do
     {
        // TIZEN_ONLY(20131208)
        // If show_region_repeat_disabled is set, do not call on_show_region of parent object.
        if (sd->show_region_repeat_disabled)
          return;
        //
        parent_obj = sd->parent_obj;
        child_obj = sd->obj;
        if ((!parent_obj) || (!_elm_widget_is(parent_obj))) break;
        sd = evas_object_smart_data_get(parent_obj);
        if (!sd) break;

        evas_object_geometry_get(parent_obj, &px, &py, NULL, NULL);
        evas_object_geometry_get(child_obj, &cx, &cy, NULL, NULL);

        x += (cx - px);
        y += (cy - py);
        sd->rx = x;
        sd->ry = y;
        sd->rw = w;
        sd->rh = h;

        if (sd->on_show_region)
           sd->on_show_region(sd->on_show_region_data, parent_obj);
     }
   while (parent_obj);
}

EAPI void
elm_widget_show_region_get(const Evas_Object *obj,
                           Evas_Coord *x,
                           Evas_Coord *y,
                           Evas_Coord *w,
                           Evas_Coord *h)
{
   API_ENTRY return;
   if (x) *x = sd->rx;
   if (y) *y = sd->ry;
   if (w) *w = sd->rw;
   if (h) *h = sd->rh;
}

EAPI void
elm_widget_show_region_repeat_disabled_set(Evas_Object *obj, Eina_Bool disabled)
{
   API_ENTRY return;
   sd->show_region_repeat_disabled = disabled;
}

EAPI Eina_Bool
elm_widget_show_region_repeat_disabled_get(Evas_Object *obj)
{
   API_ENTRY return EINA_FALSE;
   return sd->show_region_repeat_disabled;
}

/**
 * @internal
 *
 * Get the focus region of the given widget.
 *
 * @return show region or not
 * (@c EINA_TRUE = show region/@c EINA_FALSE = do not show region). Default is @c EINA_FALSE.
 *
 * The focus region is the area of a widget that should brought into the
 * visible area when the widget is focused. Mostly used to show the part of
 * an entry where the cursor is, for example. The area returned is relative
 * to the object @p obj.
 *
 * @param obj The widget object
 * @param x Where to store the x coordinate of the area
 * @param y Where to store the y coordinate of the area
 * @param w Where to store the width of the area
 * @param h Where to store the height of the area
 *
 * @ingroup Widget
 */
EAPI Eina_Bool
elm_widget_focus_region_get(const Evas_Object *obj,
                            Evas_Coord *x,
                            Evas_Coord *y,
                            Evas_Coord *w,
                            Evas_Coord *h)
{
   API_ENTRY return EINA_FALSE;
   if (!sd->api->on_focus_region(obj, x, y, w, h))
     {
        evas_object_geometry_get(obj, NULL, NULL, w, h);
        if (x) *x = 0;
        if (y) *y = 0;
     }
   return EINA_TRUE;
}

EAPI void
elm_widget_parents_bounce_get(Evas_Object *obj,
                              Eina_Bool *horiz, Eina_Bool *vert)
{
   ELM_WIDGET_CHECK_OR_RETURN(obj);

   Evas_Object *parent_obj = obj;
   Eina_Bool h, v;

   *horiz = EINA_FALSE;
   *vert = EINA_FALSE;

   do
     {
        parent_obj = elm_widget_parent_get(parent_obj);
        if ((!parent_obj) || (!_elm_widget_is(parent_obj))) break;

        if (_elm_scrollable_is(parent_obj))
          {
             ELM_SCROLLABLE_IFACE_GET(parent_obj, s_iface);
             s_iface->bounce_allow_get(parent_obj, &h, &v);
             if (h) *horiz = EINA_TRUE;
             if (v) *vert = EINA_TRUE;
          }
     }
   while (parent_obj);
}

EAPI Eina_List *
elm_widget_scrollable_children_get(Evas_Object *obj)
{
   Eina_List *l, *ret = NULL;
   Evas_Object *child;

   API_ENTRY return NULL;

   EINA_LIST_FOREACH(sd->subobjs, l, child)
     {
        if (_elm_scrollable_is(child))
          ret = eina_list_append(ret, child);
     }

   return ret;
}

EAPI void
elm_widget_scroll_hold_push(Evas_Object *obj)
{
   API_ENTRY return;
   sd->scroll_hold++;
   if (sd->scroll_hold == 1)
     {
        if (_elm_scrollable_is(obj))
          {
             ELM_SCROLLABLE_IFACE_GET(obj, s_iface);
             s_iface->hold_set(obj, EINA_TRUE);
          }
        else
          {
             Eina_List *scr_children, *l;
             Evas_Object *child;

             scr_children = elm_widget_scrollable_children_get(obj);
             EINA_LIST_FOREACH(scr_children, l, child)
               {
                  ELM_SCROLLABLE_IFACE_GET(child, s_iface);
                  s_iface->hold_set(child, EINA_TRUE);
               }
             eina_list_free(scr_children);
          }
     }
   if (sd->parent_obj) elm_widget_scroll_hold_push(sd->parent_obj);
   // FIXME: on delete/reparent hold pop
}

EAPI void
elm_widget_scroll_hold_pop(Evas_Object *obj)
{
   API_ENTRY return;
   sd->scroll_hold--;
   if (!sd->scroll_hold)
     {
        if (_elm_scrollable_is(obj))
          {
             ELM_SCROLLABLE_IFACE_GET(obj, s_iface);
             s_iface->hold_set(obj, EINA_FALSE);
          }
        else
          {
             Eina_List *scr_children, *l;
             Evas_Object *child;

             scr_children = elm_widget_scrollable_children_get(obj);
             EINA_LIST_FOREACH(scr_children, l, child)
               {
                  ELM_SCROLLABLE_IFACE_GET(child, s_iface);
                  s_iface->hold_set(child, EINA_FALSE);
               }
             eina_list_free(scr_children);
          }
     }
   if (sd->parent_obj) elm_widget_scroll_hold_pop(sd->parent_obj);
   if (sd->scroll_hold < 0) sd->scroll_hold = 0;
}

EAPI int
elm_widget_scroll_hold_get(const Evas_Object *obj)
{
   API_ENTRY return 0;
   return sd->scroll_hold;
}

EAPI void
elm_widget_scroll_freeze_push(Evas_Object *obj)
{
   API_ENTRY return;
   sd->scroll_freeze++;
   if (sd->scroll_freeze == 1)
     {
        if (_elm_scrollable_is(obj))
          {
             ELM_SCROLLABLE_IFACE_GET(obj, s_iface);
             s_iface->freeze_set(obj, EINA_TRUE);
          }
        else
          {
             Eina_List *scr_children, *l;
             Evas_Object *child;

             scr_children = elm_widget_scrollable_children_get(obj);
             EINA_LIST_FOREACH(scr_children, l, child)
               {
                  ELM_SCROLLABLE_IFACE_GET(child, s_iface);
                  s_iface->freeze_set(child, EINA_TRUE);
               }
             eina_list_free(scr_children);
          }
     }
   if (sd->parent_obj) elm_widget_scroll_freeze_push(sd->parent_obj);
   // FIXME: on delete/reparent freeze pop
}

EAPI void
elm_widget_scroll_freeze_pop(Evas_Object *obj)
{
   API_ENTRY return;
   sd->scroll_freeze--;
   if (!sd->scroll_freeze)
     {
        if (_elm_scrollable_is(obj))
          {
             ELM_SCROLLABLE_IFACE_GET(obj, s_iface);
             s_iface->freeze_set(obj, EINA_FALSE);
          }
        else
          {
             Eina_List *scr_children, *l;
             Evas_Object *child;

             scr_children = elm_widget_scrollable_children_get(obj);
             EINA_LIST_FOREACH(scr_children, l, child)
               {
                  ELM_SCROLLABLE_IFACE_GET(child, s_iface);
                  s_iface->freeze_set(child, EINA_FALSE);
               }
             eina_list_free(scr_children);
          }
     }
   if (sd->parent_obj) elm_widget_scroll_freeze_pop(sd->parent_obj);
   if (sd->scroll_freeze < 0) sd->scroll_freeze = 0;
}

EAPI int
elm_widget_scroll_freeze_get(const Evas_Object *obj)
{
   API_ENTRY return 0;
   return sd->scroll_freeze;
}

// TIZEN_ONLY(20150323)
// In now implementation of this feature is only genlist.
EAPI void
elm_widget_scroll_item_align_enabled_set(Evas_Object *obj,
                                        Eina_Bool scroll_item_align_enable)
{
   API_ENTRY return;
   if (sd->scroll_item_align_enable == scroll_item_align_enable) return;
   sd->scroll_item_align_enable = scroll_item_align_enable;
}

EAPI Eina_Bool
elm_widget_scroll_item_align_enabled_get(const Evas_Object *obj)
{
   API_ENTRY return EINA_FALSE;
   return sd->scroll_item_align_enable;
}

EAPI void
elm_widget_scroll_item_valign_set(Evas_Object *obj,
                                  char *scroll_item_valign)
{
   API_ENTRY return;
   if (sd->scroll_item_valign) eina_stringshare_del(sd->scroll_item_valign);
   if (!scroll_item_valign) sd->scroll_item_valign = NULL;
   else sd->scroll_item_valign = eina_stringshare_add(scroll_item_valign);
}

EAPI const char*
elm_widget_scroll_item_valign_get(const Evas_Object *obj)
{
   API_ENTRY return NULL;
   return sd->scroll_item_valign;
}
////////////////////

EAPI void
elm_widget_scale_set(Evas_Object *obj,
                     double scale)
{
   API_ENTRY return;
   if (scale <= 0.0) scale = 0.0;
   if (sd->scale != scale)
     {
        sd->scale = scale;
        elm_widget_theme(obj);
     }
}

EAPI double
elm_widget_scale_get(const Evas_Object *obj)
{
   API_ENTRY return 1.0;
   // FIXME: save walking up the tree by storing/caching parent scale
   if (sd->scale == 0.0)
     {
        if (sd->parent_obj)
          return elm_widget_scale_get(sd->parent_obj);
        else
          return 1.0;
     }
   return sd->scale;
}

EAPI void
elm_widget_theme_set(Evas_Object *obj,
                     Elm_Theme *th)
{
   API_ENTRY return;
   Eina_Bool apply = EINA_FALSE;

   if (sd->theme != th)
     {
        if (elm_widget_theme_get(obj) != th) apply = EINA_TRUE;
        if (sd->theme) elm_theme_free(sd->theme);
        sd->theme = th;
        if (th) th->ref++;
        if (apply) elm_widget_theme(obj);
     }
}

EAPI void
elm_widget_text_part_set(Evas_Object *obj,
                         const char *part,
                         const char *label)
{
   API_ENTRY return;

   if (evas_object_smart_type_check(obj, "elm_layout"))
     elm_layout_text_set(obj, part, label);
}

EAPI const char *
elm_widget_text_part_get(const Evas_Object *obj,
                         const char *part)
{
   API_ENTRY return NULL;

   if (evas_object_smart_type_check(obj, "elm_layout"))
     return elm_layout_text_get(obj, part);

   return NULL;
}

static Elm_Translate_String_Data *
_translate_string_data_get(Eina_Inlist *translate_strings, const char *part)
{
   Elm_Translate_String_Data *ts;
   Eina_Stringshare *str;

   if (!translate_strings) return NULL;

   str = eina_stringshare_add(part);
   EINA_INLIST_FOREACH(translate_strings, ts)
     {
        if (ts->id == str) break;
     }
   eina_stringshare_del(str);

   return ts;
}

static Elm_Translate_String_Data *
_part_text_translatable_set(Eina_Inlist **translate_strings, const char *part, Eina_Bool translatable, Eina_Bool preset)
{
   Eina_Inlist *t;
   Elm_Translate_String_Data *ts;
   t = *translate_strings;
   ts = _translate_string_data_get(t, part);

   if (translatable)
     {
        if (!ts)
          {
             ts = ELM_NEW(Elm_Translate_String_Data);
             if (!ts) return NULL;

             ts->id = eina_stringshare_add(part);
             t = eina_inlist_append(t, (Eina_Inlist*) ts);
          }
        if (preset) ts->preset = EINA_TRUE;
     }
   //Delete this exist one if this part has been not preset.
   //see elm_widget_part_text_translatable_set()
   else if (ts)
     {
        if ((preset) || (!ts->preset))
          {
             t = eina_inlist_remove(t, EINA_INLIST_GET(ts));
             eina_stringshare_del(ts->id);
             eina_stringshare_del(ts->domain);
             eina_stringshare_del(ts->string);
             free(ts);
             ts = NULL;
          }
     }

   *translate_strings = t;

   return ts;
}

EAPI void
elm_widget_domain_translatable_part_text_set(Evas_Object *obj,
                                             const char *part,
                                             const char *domain,
                                             const char *label)
{
   API_ENTRY return;
   Elm_Translate_String_Data *ts;
   if (!label)
     {
        _part_text_translatable_set(&sd->translate_strings, part, EINA_FALSE,
                                    EINA_FALSE);
     }
   else
     {
        ts = _part_text_translatable_set(&sd->translate_strings, part,
                                         EINA_TRUE, EINA_FALSE);
        if (!ts) return;
        if (!ts->string) ts->string = eina_stringshare_add(label);
        else eina_stringshare_replace(&ts->string, label);
        if (!ts->domain) ts->domain = eina_stringshare_add(domain);
        else eina_stringshare_replace(&ts->domain, domain);
#ifdef HAVE_GETTEXT
        if (label[0]) label = dgettext(domain, label);
#endif
     }

   sd->on_translate = EINA_TRUE;
   elm_widget_text_part_set(obj, part, label);
   sd->on_translate = EINA_FALSE;
}

EAPI const char *
elm_widget_translatable_part_text_get(const Evas_Object *obj,
                                      const char *part)
{
   API_ENTRY return NULL;
   Elm_Translate_String_Data *ts;
   ts = _translate_string_data_get(sd->translate_strings, part);
   if (ts) return ts->string;
   return NULL;
}

EAPI void
elm_widget_domain_part_text_translatable_set(Evas_Object *obj,
                                             const char *part,
                                             const char *domain,
                                             Eina_Bool translatable)
{
   API_ENTRY return;
   Elm_Translate_String_Data *ts;
   const char *text;

   ts = _part_text_translatable_set(&sd->translate_strings, part,
                                    translatable, EINA_TRUE);
   if (!ts) return;
   if (!ts->domain) ts->domain = eina_stringshare_add(domain);
   else eina_stringshare_replace(&ts->domain, domain);

   text = elm_widget_text_part_get(obj, part);
   if (!text || !text[0]) return;

   if (!ts->string) ts->string = eina_stringshare_add(text);

//Try to translate text since we don't know the text is already translated.
#ifdef HAVE_GETTEXT
   text = dgettext(domain, text);
#endif
   sd->on_translate = EINA_TRUE;
   elm_widget_text_part_set(obj, part, text);
   sd->on_translate = EINA_FALSE;
}

static const char*
_part_text_translate(Eina_Inlist *translate_strings,
                     const char *part,
                     const char *text)
{
   Elm_Translate_String_Data *ts;
   ts = _translate_string_data_get(translate_strings, part);
   if (!ts) return text;

   if (!ts->string) ts->string = eina_stringshare_add(text);
   else eina_stringshare_replace(&ts->string, text);
#ifdef HAVE_GETTEXT
   if (text && text[0])
     text = dgettext(ts->domain, text);
#endif
   return text;
}

EAPI const char *
elm_widget_part_text_translate(Evas_Object *obj, const char *part, const char *text)
{
   API_ENTRY return text;

   if (!sd->translate_strings || sd->on_translate) return text;
   return _part_text_translate(sd->translate_strings, part, text);
}

EAPI void
elm_widget_translate(Evas_Object *obj)
{
   API_ENTRY return;

   const Eina_List *l;
   Evas_Object *child;

   EINA_LIST_FOREACH(sd->subobjs, l, child)
     elm_widget_translate(child);
   if (sd->hover_obj) elm_widget_translate(sd->hover_obj);
   if (!sd->api) return;
   sd->api->translate(obj);

#ifdef HAVE_GETTEXT
   Elm_Translate_String_Data *ts;
   EINA_INLIST_FOREACH(sd->translate_strings, ts)
     {
        if (!ts->string) continue;
        const char *s = dgettext(ts->domain, ts->string);
        sd->on_translate = EINA_TRUE;
        elm_widget_text_part_set(obj, ts->id, s);
        sd->on_translate = EINA_FALSE;
     }
#endif
}

EAPI void
elm_widget_content_part_set(Evas_Object *obj,
                            const char *part,
                            Evas_Object *content)
{
   API_ENTRY return;

   if (!sd->api) return;
   if (evas_object_smart_type_check(obj, "elm_container"))
     ELM_CONTAINER_CLASS(sd->api)->content_set(obj, part, content);
}

EAPI Evas_Object *
elm_widget_content_part_get(const Evas_Object *obj,
                            const char *part)
{
   API_ENTRY return NULL;

   if (!sd->api) return NULL;

   if (evas_object_smart_type_check(obj, "elm_container"))
     return ELM_CONTAINER_CLASS(sd->api)->content_get(obj, part);

   return NULL;
}

EAPI Evas_Object *
elm_widget_content_part_unset(Evas_Object *obj,
                              const char *part)
{
   API_ENTRY return NULL;

   if (!sd->api) return NULL;
   if (evas_object_smart_type_check(obj, "elm_container"))
     return ELM_CONTAINER_CLASS(sd->api)->content_unset(obj, part);

   return NULL;
}

EAPI Evas_Object *
elm_widget_part_access_object_get(const Evas_Object *obj,
                                  const char *part)
{
   API_ENTRY return NULL;

   if (!sd->api) return NULL;

   if (sd->api->access_object_get)
     return sd->api->access_object_get((Evas_Object *)obj, (char *)part);

   return NULL;
}

EAPI void
elm_widget_access_info_set(Evas_Object *obj,
                           const char *txt)
{
   API_ENTRY return;
   if (sd->access_info) eina_stringshare_del(sd->access_info);
   if (!txt) sd->access_info = NULL;
   else sd->access_info = eina_stringshare_add(txt);
}

EAPI const char *
elm_widget_access_info_get(const Evas_Object *obj)
{
   API_ENTRY return NULL;
   return sd->access_info;
}

EAPI Elm_Theme *
elm_widget_theme_get(const Evas_Object *obj)
{
   API_ENTRY return NULL;
   if (!sd->theme)
     {
        if (sd->parent_obj)
          return elm_widget_theme_get(sd->parent_obj);
        else
          return NULL;
     }
   return sd->theme;
}

EAPI Eina_Bool
elm_widget_style_set(Evas_Object *obj,
                     const char *style)
{
   API_ENTRY return EINA_FALSE;

   if (eina_stringshare_replace(&sd->style, style))
     return elm_widget_theme(obj);

   return EINA_TRUE;
}

EAPI const char *
elm_widget_style_get(const Evas_Object *obj)
{
   API_ENTRY return NULL;
   if (sd->style) return sd->style;
   return "default";
}

EAPI void
elm_widget_tooltip_add(Evas_Object *obj,
                       Elm_Tooltip *tt)
{
   API_ENTRY return;
   sd->tooltips = eina_list_append(sd->tooltips, tt);
}

EAPI void
elm_widget_tooltip_del(Evas_Object *obj,
                       Elm_Tooltip *tt)
{
   API_ENTRY return;
   sd->tooltips = eina_list_remove(sd->tooltips, tt);
}

EAPI void
elm_widget_cursor_add(Evas_Object *obj,
                      Elm_Cursor *cur)
{
   API_ENTRY return;
   sd->cursors = eina_list_append(sd->cursors, cur);
}

EAPI void
elm_widget_cursor_del(Evas_Object *obj,
                      Elm_Cursor *cur)
{
   API_ENTRY return;
   sd->cursors = eina_list_remove(sd->cursors, cur);
}

EAPI void
elm_widget_drag_lock_x_set(Evas_Object *obj,
                           Eina_Bool lock)
{
   API_ENTRY return;
   if (sd->drag_x_locked == lock) return;
   sd->drag_x_locked = lock;
   if (sd->drag_x_locked) _propagate_x_drag_lock(obj, 1);
   else _propagate_x_drag_lock(obj, -1);
}

EAPI void
elm_widget_drag_lock_y_set(Evas_Object *obj,
                           Eina_Bool lock)
{
   API_ENTRY return;
   if (sd->drag_y_locked == lock) return;
   sd->drag_y_locked = lock;
   if (sd->drag_y_locked) _propagate_y_drag_lock(obj, 1);
   else _propagate_y_drag_lock(obj, -1);
}

EAPI Eina_Bool
elm_widget_drag_lock_x_get(const Evas_Object *obj)
{
   API_ENTRY return EINA_FALSE;
   return sd->drag_x_locked;
}

EAPI Eina_Bool
elm_widget_drag_lock_y_get(const Evas_Object *obj)
{
   API_ENTRY return EINA_FALSE;
   return sd->drag_y_locked;
}

EAPI int
elm_widget_drag_child_locked_x_get(const Evas_Object *obj)
{
   API_ENTRY return 0;
   return sd->child_drag_x_locked;
}

EAPI int
elm_widget_drag_child_locked_y_get(const Evas_Object *obj)
{
   API_ENTRY return 0;
   return sd->child_drag_y_locked;
}

EAPI Eina_Bool
elm_widget_theme_object_set(Evas_Object *obj,
                            Evas_Object *edj,
                            const char *wname,
                            const char *welement,
                            const char *wstyle)
{
   API_ENTRY return EINA_FALSE;
   char buf[128];

   if (!_elm_theme_object_set(obj, edj, wname, welement, wstyle))
     return EINA_FALSE;

   if (sd->orient_mode != -1)
     {
        // TIZEN ONLY: for color class setting //
        _elm_widget_color_overlay_set(obj, edj);
        _elm_widget_font_overlay_set(obj, edj);

        snprintf(buf, sizeof(buf), "elm,state,orient,%d", sd->orient_mode);
        elm_widget_signal_emit(obj, buf, "elm");

     }
   return EINA_TRUE;
}

EAPI Eina_Bool
elm_widget_is_check(const Evas_Object *obj)
{
   static int abort_on_warn = -1;
   if (elm_widget_is(obj))
     return EINA_TRUE;

   ERR("Passing Object: %p.", obj);
   if (abort_on_warn == -1)
     {
        if (getenv("ELM_ERROR_ABORT")) abort_on_warn = 1;
        else abort_on_warn = 0;
     }
   if (abort_on_warn == 1)
     {
        ERR("### EFL abort on errors ###\n");
        abort();
     }
   return EINA_FALSE;
}

EAPI const char *
elm_widget_type_get(const Evas_Object *obj)
{
   API_ENTRY return NULL;

   return evas_object_type_get(obj);
}

EAPI Eina_Bool
elm_widget_type_check(const Evas_Object *obj,
                      const char *type,
                      const char *func)
{
   const char *provided, *expected = "(unknown)";
   static int abort_on_warn = -1;

   provided = elm_widget_type_get(obj);
   /* TODO: eventually migrate to check_ptr version */
   if (evas_object_smart_type_check(obj, type)) return EINA_TRUE;
   if (type) expected = type;
   if ((!provided) || (!provided[0]))
     {
        provided = evas_object_type_get(obj);
        if ((!provided) || (!provided[0]))
          provided = "(unknown)";
     }
   ERR("Passing Object: %p in function: %s, of type: '%s' when expecting"
       " type: '%s'", obj, func, provided, expected);
   if (abort_on_warn == -1)
     {
        if (getenv("ELM_ERROR_ABORT")) abort_on_warn = 1;
        else abort_on_warn = 0;
     }
   if (abort_on_warn == 1)
     {
        ERR("### EFL abort on errors ###\n");
        abort();
     }
   return EINA_FALSE;
}

static Evas_Object *
_widget_name_find(const Evas_Object *obj,
                  const char *name,
                  int recurse)
{
   Eina_List *l;
   Evas_Object *child;
   const char *s;
   INTERNAL_ENTRY NULL;

   if (!_elm_widget_is(obj)) return NULL;
   EINA_LIST_FOREACH(sd->subobjs, l, child)
     {
        s = evas_object_name_get(child);
        if ((s) && (!strcmp(s, name))) return child;
        if ((recurse != 0) &&
            ((child = _widget_name_find(child, name, recurse - 1))))
          return child;
     }
   if (sd->hover_obj)
     {
        s = evas_object_name_get(sd->hover_obj);
        if ((s) && (!strcmp(s, name))) return sd->hover_obj;
        if ((recurse != 0) &&
            ((child = _widget_name_find(sd->hover_obj, name, recurse - 1))))
          return child;
     }
   return NULL;
}

EAPI Evas_Object *
elm_widget_name_find(const Evas_Object *obj,
                     const char *name,
                     int recurse)
{
   API_ENTRY return NULL;
   if (!name) return NULL;
   return _widget_name_find(obj, name, recurse);
}

EAPI void
elm_widget_orientation_mode_disabled_set(Evas_Object *obj, Eina_Bool disabled)
{
   int orient_mode = -1;

   API_ENTRY return;

   if (disabled && (sd->orient_mode == -1)) return;
   if (!disabled && (sd->orient_mode != -1)) return;

   if (!disabled)
     {
        //Get current orient mode from it's parent otherwise, 0.
        sd->orient_mode = 0;
        ELM_WIDGET_DATA_GET(sd->parent_obj, sd_parent);
        if (!sd_parent) orient_mode = 0;
        else orient_mode = sd_parent->orient_mode;
     }
   elm_widget_orientation_set(obj, orient_mode);
}

EAPI Eina_Bool
elm_widget_orientation_mode_disabled_get(const Evas_Object *obj)
{
   Eina_Bool ret;

   API_ENTRY return EINA_FALSE;

   if (sd->orient_mode == -1) ret = EINA_TRUE;
   else ret = EINA_FALSE;
   return ret;
}

EAPI void
elm_widget_orientation_set(Evas_Object *obj, int rotation)
{
   Evas_Object *child;
   Eina_List *l;

   API_ENTRY return;

   if ((sd->orient_mode == rotation) || (sd->orient_mode == -1)) return;

   sd->orient_mode = rotation;

   EINA_LIST_FOREACH (sd->subobjs, l, child)
     elm_widget_orientation_set(child, rotation);

   if (sd->api->orientation_set)
     {
        sd->api->orientation_set(obj, rotation);
        return;
     }
   if (rotation != -1)
     {
        char buf[128];
        snprintf(buf, sizeof(buf), "elm,state,orient,%d", sd->orient_mode);
        elm_widget_signal_emit(obj, buf, "elm");
     }
}

static void
_track_obj_del(void *data, Evas *e, Evas_Object *obj, void *event_info);

static void
_track_obj_update(Evas_Object *track, Evas_Object *obj)
{
   //Geometry
   Evas_Coord x, y, w, h;
   evas_object_geometry_get(obj, &x, &y, &w, &h);
   evas_object_move(track, x, y);
   evas_object_resize(track, w, h);

   //Visibility
   if (evas_object_visible_get(obj)) evas_object_show(track);
   else evas_object_hide(track);
}

static void
_track_obj_view_update(void *data, Evas *e EINA_UNUSED, Evas_Object *obj,
                       void *event_info EINA_UNUSED)
{
   Evas_Object *track = data;
   _track_obj_update(track, obj);
}

static void
_track_obj_view_del(void *data, Evas *e EINA_UNUSED,
                    Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Elm_Widget_Item *item = data;

   while (evas_object_ref_get(item->track_obj) > 0)
     evas_object_unref(item->track_obj);

   evas_object_event_callback_del(item->track_obj, EVAS_CALLBACK_DEL,
                                  _track_obj_del);
   evas_object_del(item->track_obj);
   item->track_obj = NULL;
}

static void
_track_obj_del(void *data, Evas *e EINA_UNUSED,
                    Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Elm_Widget_Item *item = data;
   item->track_obj = NULL;

   if (!item->view) return;

   evas_object_event_callback_del(item->view, EVAS_CALLBACK_RESIZE,
                                  _track_obj_view_update);
   evas_object_event_callback_del(item->view, EVAS_CALLBACK_MOVE,
                                  _track_obj_view_update);
   evas_object_event_callback_del(item->view, EVAS_CALLBACK_SHOW,
                                  _track_obj_view_update);
   evas_object_event_callback_del(item->view, EVAS_CALLBACK_HIDE,
                                  _track_obj_view_update);
   evas_object_event_callback_del(item->view, EVAS_CALLBACK_DEL,
                                  _track_obj_view_del);
}

/**
 * @internal
 *
 * Split string in words
 *
 * @param str Source string
 * @return List of const words
 *
 * @see elm_widget_stringlist_free()
 * @ingroup Widget
 */
EAPI Eina_List *
elm_widget_stringlist_get(const char *str)
{
   Eina_List *list = NULL;
   const char *s, *b;
   if (!str) return NULL;
   for (b = s = str; 1; s++)
     {
        if ((*s == ' ') || (!*s))
          {
             char *t = malloc(s - b + 1);
             if (t)
               {
                  strncpy(t, b, s - b);
                  t[s - b] = 0;
                  list = eina_list_append(list, eina_stringshare_add(t));
                  free(t);
               }
             b = s + 1;
          }
        if (!*s) break;
     }
   return list;
}

EAPI void
elm_widget_stringlist_free(Eina_List *list)
{
   const char *s;
   EINA_LIST_FREE (list, s)
     eina_stringshare_del(s);
}

EAPI void
elm_widget_focus_hide_handle(Evas_Object *obj)
{
   if (!_elm_widget_is(obj))
     return;
   _if_focused_revert(obj, EINA_TRUE);
}

EAPI void
elm_widget_focus_mouse_up_handle(Evas_Object *obj)
{
// TIZEN ONLY (20140613) : Added checking if widget can get focus.
   API_ENTRY return;
//
   Evas_Object *o = obj;
// TIZEN ONLY (20140613) : Added checking if widget can get focus.
   Evas_Coord x, y;
   Eina_Bool ret;
//
   do
     {
        if (_elm_widget_is(o)) break;
        o = evas_object_smart_parent_get(o);
     }
   while (o);
   if (!o) return;
   if (!_is_focusable(o)) return;

// TIZEN ONLY (20140613) : Added checking if widget can get focus.
   evas_pointer_canvas_xy_get(evas_object_evas_get(o), &x, &y);
   if (sd->api->focusable_is)
     {
        ret = sd->api->focusable_is(o, x, y);
        if (!ret) return;
     }
//
   elm_widget_focus_steal(o);
}

EAPI void
elm_widget_focus_tree_unfocusable_handle(Evas_Object *obj)
{
   API_ENTRY return;

   //FIXME: Need to check whether the object is unfocusable or not.

   if (!elm_widget_parent_get(obj))
     elm_widget_focused_object_clear(obj);
   else
     _if_focused_revert(obj, EINA_TRUE);
}

EAPI void
elm_widget_focus_disabled_handle(Evas_Object *obj)
{
   API_ENTRY return;

   elm_widget_focus_tree_unfocusable_handle(obj);
}

EAPI unsigned int
elm_widget_focus_order_get(const Evas_Object *obj)
{
   API_ENTRY return 0;
   return sd->focus_order;
}

EAPI Evas_Object *
elm_widget_newest_focus_order_get(const Evas_Object *obj,
                                  unsigned int *newest_focus_order,
                                  Eina_Bool can_focus_only)
{
   const Eina_List *l;
   Evas_Object *child, *ret, *best;

   API_ENTRY return NULL;

   if (!evas_object_visible_get(obj)
       || (elm_widget_disabled_get(obj))
       || (elm_widget_tree_unfocusable_get(obj)))
     return NULL;

   best = NULL;
   if (!_elm_access_auto_highlight_get())
     {
        if (*newest_focus_order < sd->focus_order)
          {
             if (!can_focus_only || elm_widget_can_focus_get(obj))
               {
                  *newest_focus_order = sd->focus_order;
                  best = (Evas_Object *)obj;
               }
          }
     }
   else
     {
        if (*newest_focus_order < sd->highlight_order)
          {
             if(_elm_access_object_get(obj))
               {
                  *newest_focus_order = sd->highlight_order;
                  best = (Evas_Object *)obj;
               }
          }
     }

   EINA_LIST_FOREACH(sd->subobjs, l, child)
     {
        ret = elm_widget_newest_focus_order_get
           (child, newest_focus_order, can_focus_only);
        if (!ret) continue;
        best = ret;
     }
   return best;
}

EAPI void
elm_widget_activate(Evas_Object *obj, Elm_Activate act)
{
   Evas_Object *parent;
   Eina_Bool ret;

   API_ENTRY return;

   ret = EINA_FALSE;
   if (sd->api->activate)
     ret = sd->api->activate(obj, act);

   if (ret) return;

   parent = elm_widget_parent_get(obj);
   if (parent)
     elm_widget_activate(parent, act);

   return;
}

/**
 * @internal
 *
 * Sets the widget and child widget's Evas_Display_Mode.
 *
 * @param obj The widget.
 * @param dispmode Evas_Display_Mode to set widget's mode.
 *
 * Widgets are resized by several reasons.
 * Evas_Display_Mode can help for widgets to get one more reason of resize.
 * For example, elm conform widget resizes it's contents when keypad state changed.
 * After keypad showing, conform widget can change child's Evas_Display_Mode.
 * @ingroup Widget
 */
EAPI void
elm_widget_display_mode_set(Evas_Object *obj, Evas_Display_Mode dispmode)
{
   Evas_Object *child;
   Eina_List *l;
   Evas_Display_Mode prev_dispmode;

   API_ENTRY return;

   prev_dispmode = evas_object_size_hint_display_mode_get(obj);

   if ((prev_dispmode == dispmode) ||
       (prev_dispmode == EVAS_DISPLAY_MODE_DONT_CHANGE)) return;
   evas_object_size_hint_display_mode_set(obj, dispmode);

   EINA_LIST_FOREACH (sd->subobjs, l, child)
      elm_widget_display_mode_set(child, dispmode);
}

// TIZEN ONLY: temporary code. should be removed after eo is applied.
EAPI void
_elm_widget_orient_signal_emit(Evas_Object *obj)
{
   ELM_WIDGET_DATA_GET(obj, sd);
   char buf[128];
   if (sd->orient_mode > 0)
     {
        snprintf(buf, sizeof(buf), "elm,state,orient,%d", sd->orient_mode);
        elm_widget_signal_emit(obj, buf, "elm");
     }
}

/**
 * @internal
 *
 * Allocate a new Elm_Widget_Item-derived structure.
 *
 * The goal of this structure is to provide common ground for actions
 * that a widget item have, such as the owner widget, callback to
 * notify deletion, data pointer and maybe more.
 *
 * @param widget the owner widget that holds this item, must be an elm_widget!
 * @param alloc_size any number greater than sizeof(Elm_Widget_Item) that will
 *        be used to allocate memory.
 *
 * @return allocated memory that is already zeroed out, or NULL on errors.
 *
 * @see elm_widget_item_new() convenience macro.
 * @see elm_widget_item_del() to release memory.
 * @ingroup Widget
 */
EAPI Elm_Widget_Item *
_elm_widget_item_new(Evas_Object *widget,
                     size_t alloc_size)
{
   if (!_elm_widget_is(widget))
     return NULL;

   Elm_Widget_Item *item;

   EINA_SAFETY_ON_TRUE_RETURN_VAL(alloc_size < sizeof(Elm_Widget_Item), NULL);
   EINA_SAFETY_ON_TRUE_RETURN_VAL(!_elm_widget_is(widget), NULL);

   item = calloc(1, alloc_size);
   EINA_SAFETY_ON_NULL_RETURN_VAL(item, NULL);

   EINA_MAGIC_SET(item, ELM_WIDGET_ITEM_MAGIC);
   item->widget = widget;
   return item;
}

EAPI void
_elm_widget_item_free(Elm_Widget_Item *item)
{
   Elm_Translate_String_Data *ts;
   Elm_Widget_Item_Signal_Data *wisd;

   ELM_WIDGET_ITEM_CHECK_OR_RETURN(item);

   if (item->del_func)
     item->del_func((void *)item->data, item->widget, item);

   if (item->access_info)
     eina_stringshare_del(item->access_info);

   if (item->access_order)
     _elm_access_widget_item_access_order_unset(item);

   EINA_LIST_FREE(item->signals, wisd)
     {
        _elm_widget_item_signal_callback_del
           (item, wisd->emission, wisd->source, wisd->func);
     }

   if (item->view)
     evas_object_del(item->view);

   while (item->translate_strings)
     {
        ts = EINA_INLIST_CONTAINER_GET(item->translate_strings,
                                       Elm_Translate_String_Data);
        eina_stringshare_del(ts->id);
        eina_stringshare_del(ts->domain);
        eina_stringshare_del(ts->string);
        item->translate_strings = eina_inlist_remove(item->translate_strings,
                                                     item->translate_strings);
        free(ts);
     }

   EINA_MAGIC_SET(item, EINA_MAGIC_NONE);
   free(item);
}

/**
 * @internal
 *
 * Releases widget item memory, calling back del_cb() if it exists.
 *
 * If there is a Elm_Widget_Item::del_cb, then it will be called prior
 * to memory release. Note that elm_widget_item_pre_notify_del() calls
 * this function and then unset it, thus being useful for 2 step
 * cleanup whenever the del_cb may use any of the data that must be
 * deleted from item.
 *
 * The Elm_Widget_Item::view will be deleted (evas_object_del()) if it
 * is presented!
 *
 * @param item a valid #Elm_Widget_Item to be deleted.
 * @see elm_widget_item_del() convenience macro.
 * @ingroup Widget
 */
EAPI void
_elm_widget_item_del(Elm_Widget_Item *item)
{
   ELM_WIDGET_ITEM_CHECK_OR_RETURN(item);
   ELM_WIDGET_ITEM_RETURN_IF_ONDEL(item);
   item->on_deletion = EINA_TRUE;

   //Widget item delete callback
   if (item->del_pre_func)
     {
        if (item->del_pre_func((Elm_Object_Item *)item))
          _elm_widget_item_free(item);
     }
   else
     _elm_widget_item_free(item);
 }

/**
 * @internal
 *
 * Set the function to notify to widgets when item is being deleted by user.
 *
 * @param item a valid #Elm_Widget_Item to be notified
 * @see elm_widget_item_del_pre_hook_set() convenience macro.
 * @ingroup Widget
 */
EAPI void
_elm_widget_item_del_pre_hook_set(Elm_Widget_Item *item,
                                  Elm_Widget_Del_Pre_Cb func)
{
   ELM_WIDGET_ITEM_CHECK_OR_RETURN(item);
   ELM_WIDGET_ITEM_RETURN_IF_ONDEL(item);
   item->del_pre_func = func;
}

/**
 * @internal
 *
 * Set the function to set the style of item
 *
 * @param item a valid #Elm_Widget_Item to be notified
 * @see elm_widget_item_style_set_hook_set() convenience macro.
 * @ingroup Widget
 */
EAPI void
_elm_widget_item_style_set_hook_set(Elm_Widget_Item *item,
                                  Elm_Widget_Style_Set_Cb func)
{
   ELM_WIDGET_ITEM_CHECK_OR_RETURN(item);
   ELM_WIDGET_ITEM_RETURN_IF_ONDEL(item);
   item->style_set_func = func;
}

/**
 * @internal
 *
 * Set the function to get the style of item
 *
 * @param item a valid #Elm_Widget_Item to be notified
 * @see elm_widget_item_style_get_hook_set() convenience macro.
 * @ingroup Widget
 */
EAPI void
_elm_widget_item_style_get_hook_set(Elm_Widget_Item *item,
                                  Elm_Widget_Style_Get_Cb func)
{
   ELM_WIDGET_ITEM_CHECK_OR_RETURN(item);
   ELM_WIDGET_ITEM_RETURN_IF_ONDEL(item);
   item->style_get_func = func;
}

/**
 * @internal
 *
 * Notify object will be deleted without actually deleting it.
 *
 * This function will callback Elm_Widget_Item::del_cb if it is set
 * and then unset it so it is not called twice (ie: from
 * elm_widget_item_del()).
 *
 * @param item a valid #Elm_Widget_Item to be notified
 * @see elm_widget_item_pre_notify_del() convenience macro.
 * @ingroup Widget
 */
EAPI void
_elm_widget_item_pre_notify_del(Elm_Widget_Item *item)
{
   ELM_WIDGET_ITEM_CHECK_OR_RETURN(item);
   if (!item->del_func) return;
   item->del_func((void *)item->data, item->widget, item);
   item->del_func = NULL;
}

/**
 * @internal
 *
 * Set the function to notify when item is being deleted.
 *
 * This function will complain if there was a callback set already,
 * however it will set the new one.
 *
 * The callback will be called from elm_widget_item_pre_notify_del()
 * or elm_widget_item_del() will be called with:
 *   - data: the Elm_Widget_Item::data value.
 *   - obj: the Elm_Widget_Item::widget evas object.
 *   - event_info: the item being deleted.
 *
 * @param item a valid #Elm_Widget_Item to be notified
 * @see elm_widget_item_del_cb_set() convenience macro.
 * @ingroup Widget
 */
EAPI void
_elm_widget_item_del_cb_set(Elm_Widget_Item *item,
                            Evas_Smart_Cb func)
{
   ELM_WIDGET_ITEM_CHECK_OR_RETURN(item);
   ELM_WIDGET_ITEM_RETURN_IF_ONDEL(item);

   if ((item->del_func) && (item->del_func != func))
     WRN("You're replacing a previously set del_cb %p of item %p with %p",
         item->del_func, item, func);

   item->del_func = func;
}

/**
 * @internal
 *
 * Retrieves owner widget of this item.
 *
 * @param item a valid #Elm_Widget_Item to get data from.
 * @return owner widget of this item.
 * @ingroup Widget
 */
EAPI Evas_Object *
_elm_widget_item_widget_get(const Elm_Widget_Item *item)
{
   ELM_WIDGET_ITEM_CHECK_OR_RETURN(item, NULL);
   return item->widget;
}

/**
 * @internal
 *
 * Set user-data in this item.
 *
 * User data may be used to identify this item or just store any
 * application data. It is automatically given as the first parameter
 * of the deletion notify callback.
 *
 * @param item a valid #Elm_Widget_Item to store data in.
 * @param data user data to store.
 * @see elm_widget_item_del_cb_set() convenience macro.
 * @ingroup Widget
 */
EAPI void
_elm_widget_item_data_set(Elm_Widget_Item *item,
                          const void *data)
{
   ELM_WIDGET_ITEM_CHECK_OR_RETURN(item);
   ELM_WIDGET_ITEM_RETURN_IF_ONDEL(item);

   if ((item->data) && (item->data != data))
     DBG("Replacing item %p data %p with %p", item, item->data, data);
   item->data = data;
}

/**
 * @internal
 *
 * Retrieves user-data of this item.
 *
 * @param item a valid #Elm_Widget_Item to get data from.
 * @see elm_widget_item_data_set()
 * @ingroup Widget
 */
EAPI void *
_elm_widget_item_data_get(const Elm_Widget_Item *item)
{
   ELM_WIDGET_ITEM_CHECK_OR_RETURN(item, NULL);
   return (void *)item->data;
}

EAPI void
_elm_widget_item_disabled_set(Elm_Widget_Item *item,
                              Eina_Bool disabled)
{
   ELM_WIDGET_ITEM_CHECK_OR_RETURN(item);
   ELM_WIDGET_ITEM_RETURN_IF_ONDEL(item);

   if (item->disabled == disabled) return;
   item->disabled = !!disabled;
   if (item->disable_func) item->disable_func(item);
}

EAPI Eina_Bool
_elm_widget_item_disabled_get(const Elm_Widget_Item *item)
{
   ELM_WIDGET_ITEM_CHECK_OR_RETURN(item, EINA_FALSE);
   return item->disabled;
}

EAPI void
_elm_widget_item_style_set(Elm_Widget_Item *item, const char *style)
{
   ELM_WIDGET_ITEM_CHECK_OR_RETURN(item);
   ELM_WIDGET_ITEM_RETURN_IF_ONDEL(item);
   if (item->style_set_func) item->style_set_func(item, style);
}

EAPI const char *
_elm_widget_item_style_get(Elm_Widget_Item *item)
{
   ELM_WIDGET_ITEM_CHECK_OR_RETURN(item, NULL);
   return item->style_get_func ? item->style_get_func(item) : NULL;
}

EAPI void
_elm_widget_item_disable_hook_set(Elm_Widget_Item *item,
                                  Elm_Widget_Disable_Cb func)
{
   ELM_WIDGET_ITEM_CHECK_OR_RETURN(item);
   ELM_WIDGET_ITEM_RETURN_IF_ONDEL(item);

   item->disable_func = func;
}

EAPI void
_elm_widget_item_domain_translatable_part_text_set(Elm_Widget_Item *item,
                                                   const char *part,
                                                   const char *domain,
                                                   const char *label)
{
   ELM_WIDGET_ITEM_CHECK_OR_RETURN(item);
   Elm_Translate_String_Data *ts;

   if (!label)
     {
        _part_text_translatable_set(&item->translate_strings, part, EINA_FALSE,
                                    EINA_FALSE);
     }
   else
     {
        ts = _part_text_translatable_set(&item->translate_strings, part,
                                         EINA_TRUE, EINA_FALSE);
        if (!ts) return;
        if (!ts->string) ts->string = eina_stringshare_add(label);
        else eina_stringshare_replace(&ts->string, label);
        if (!ts->domain) ts->domain = eina_stringshare_add(domain);
        else eina_stringshare_replace(&ts->domain, domain);
#ifdef HAVE_GETTEXT
        if (label[0]) label = dgettext(domain, label);
#endif
     }
   item->on_translate = EINA_TRUE;
   _elm_widget_item_part_text_set(item, part, label);
   item->on_translate = EINA_FALSE;
}

EAPI const char *
_elm_widget_item_translatable_part_text_get(const Elm_Widget_Item *item,
                                            const char *part)
{
   ELM_WIDGET_ITEM_CHECK_OR_RETURN(item, NULL);
   Elm_Translate_String_Data *ts;
   ts = _translate_string_data_get(item->translate_strings, part);
   if (ts) return ts->string;
   return NULL;
}

EAPI void
_elm_widget_item_domain_part_text_translatable_set(Elm_Widget_Item *item,
                                                   const char *part,
                                                   const char *domain,
                                                   Eina_Bool translatable)
{
   ELM_WIDGET_ITEM_CHECK_OR_RETURN(item);
   Elm_Translate_String_Data *ts;
   const char *text;

   ts = _part_text_translatable_set(&item->translate_strings, part,
                                    translatable, EINA_TRUE);
   if (!ts) return;
   if (!ts->domain) ts->domain = eina_stringshare_add(domain);
   else eina_stringshare_replace(&ts->domain, domain);

   text = _elm_widget_item_part_text_get(item, part);
   if (!text || !text[0]) return;

   if (!ts->string) ts->string = eina_stringshare_add(text);

//Try to translate text since we don't know the text is already translated.
#ifdef HAVE_GETTEXT
   text = dgettext(domain, text);
#endif
   item->on_translate = EINA_TRUE;
   _elm_widget_item_part_text_set(item, part, text);
   item->on_translate = EINA_FALSE;
}

EAPI void
_elm_widget_item_track_cancel(Elm_Widget_Item *item)
{
   ELM_WIDGET_ITEM_CHECK_OR_RETURN(item);

   if (!item->track_obj) return;

   while (evas_object_ref_get(item->track_obj) > 0)
     evas_object_unref(item->track_obj);

   evas_object_del(item->track_obj);
}

EAPI Evas_Object *
elm_widget_item_track(Elm_Widget_Item *item)
{
   ELM_WIDGET_ITEM_CHECK_OR_RETURN(item, NULL);

   if (item->track_obj)
     {
        evas_object_ref(item->track_obj);
        return item->track_obj;
     }

   if (!item->view)
     {
        WRN("view obj of the item(%p) is invalid. Please make sure the view obj is created!", item);
        return NULL;
     }

   Evas_Object *track =
      evas_object_rectangle_add(evas_object_evas_get(item->widget));
   evas_object_color_set(track, 0, 0, 0, 0);
   evas_object_pass_events_set(track, EINA_TRUE);
   _track_obj_update(track, item->view);
   evas_object_event_callback_add(track, EVAS_CALLBACK_DEL, _track_obj_del,
                                  item);

   evas_object_event_callback_add(item->view, EVAS_CALLBACK_RESIZE,
                                  _track_obj_view_update, track);
   evas_object_event_callback_add(item->view, EVAS_CALLBACK_MOVE,
                                  _track_obj_view_update, track);
   evas_object_event_callback_add(item->view, EVAS_CALLBACK_SHOW,
                                  _track_obj_view_update, track);
   evas_object_event_callback_add(item->view, EVAS_CALLBACK_HIDE,
                                  _track_obj_view_update, track);
   evas_object_event_callback_add(item->view, EVAS_CALLBACK_DEL,
                                  _track_obj_view_del, item);

   evas_object_ref(track);

   item->track_obj = track;

   return track;
}

void
elm_widget_item_untrack(Elm_Widget_Item *item)
{
   ELM_WIDGET_ITEM_CHECK_OR_RETURN(item);

   if (!item->track_obj) return;
   evas_object_unref(item->track_obj);

   if (evas_object_ref_get(item->track_obj) == 0)
     evas_object_del(item->track_obj);
}

int
elm_widget_item_track_get(const Elm_Widget_Item *item)
{
   ELM_WIDGET_ITEM_CHECK_OR_RETURN(item, 0);

   if (!item->track_obj) return 0;
   return evas_object_ref_get(item->track_obj);
}

typedef struct _Elm_Widget_Item_Tooltip Elm_Widget_Item_Tooltip;

struct _Elm_Widget_Item_Tooltip
{
   Elm_Widget_Item            *item;
   Elm_Tooltip_Item_Content_Cb func;
   Evas_Smart_Cb               del_cb;
   const void                 *data;
};

static Evas_Object *
_elm_widget_item_tooltip_label_create(void *data,
                                      Evas_Object *obj __UNUSED__,
                                      Evas_Object *tooltip,
                                      void *item __UNUSED__)
{
   Evas_Object *label = elm_label_add(tooltip);
   if (!label)
     return NULL;
   elm_object_style_set(label, "tooltip");
   elm_object_text_set(label, data);
   return label;
}

static Evas_Object *
_elm_widget_item_tooltip_trans_label_create(void *data,
                                            Evas_Object *obj __UNUSED__,
                                            Evas_Object *tooltip,
                                            void *item __UNUSED__)
{
   Evas_Object *label = elm_label_add(tooltip);
   if (!label)
     return NULL;
   elm_object_style_set(label, "tooltip");
   elm_object_translatable_text_set(label, data);
   return label;
}

static void
_elm_widget_item_tooltip_label_del_cb(void *data,
                                      Evas_Object *obj __UNUSED__,
                                      void *event_info __UNUSED__)
{
   eina_stringshare_del(data);
}

/**
 * @internal
 *
 * Set the text to be shown in the widget item.
 *
 * @param item Target item
 * @param text The text to set in the content
 *
 * Setup the text as tooltip to object. The item can have only one tooltip,
 * so any previous tooltip data is removed.
 *
 * @ingroup Widget
 */
EAPI void
_elm_widget_item_tooltip_text_set(Elm_Widget_Item *item,
                                  const char *text)
{
   ELM_WIDGET_ITEM_CHECK_OR_RETURN(item);
   EINA_SAFETY_ON_NULL_RETURN(text);
   ELM_WIDGET_ITEM_RETURN_IF_ONDEL(item);

   text = eina_stringshare_add(text);
   _elm_widget_item_tooltip_content_cb_set
     (item, _elm_widget_item_tooltip_label_create, text,
     _elm_widget_item_tooltip_label_del_cb);
}

EAPI void
_elm_widget_item_tooltip_translatable_text_set(Elm_Widget_Item *item,
                                               const char *text)
{
   ELM_WIDGET_ITEM_CHECK_OR_RETURN(item);
   EINA_SAFETY_ON_NULL_RETURN(text);
   ELM_WIDGET_ITEM_RETURN_IF_ONDEL(item);

   text = eina_stringshare_add(text);
   _elm_widget_item_tooltip_content_cb_set
     (item, _elm_widget_item_tooltip_trans_label_create, text,
     _elm_widget_item_tooltip_label_del_cb);
}

static Evas_Object *
_elm_widget_item_tooltip_create(void *data,
                                Evas_Object *obj,
                                Evas_Object *tooltip)
{
   Elm_Widget_Item_Tooltip *wit = data;
   return wit->func((void *)wit->data, obj, tooltip, wit->item);
}

static void
_elm_widget_item_tooltip_del_cb(void *data,
                                Evas_Object *obj,
                                void *event_info __UNUSED__)
{
   Elm_Widget_Item_Tooltip *wit = data;
   if (wit->del_cb) wit->del_cb((void *)wit->data, obj, wit->item);
   free(wit);
}

/**
 * @internal
 *
 * Set the content to be shown in the tooltip item
 *
 * Setup the tooltip to item. The item can have only one tooltip,
 * so any previous tooltip data is removed. @p func(with @p data) will
 * be called every time that need show the tooltip and it should
 * return a valid Evas_Object. This object is then managed fully by
 * tooltip system and is deleted when the tooltip is gone.
 *
 * @param item the widget item being attached a tooltip.
 * @param func the function used to create the tooltip contents.
 * @param data what to provide to @a func as callback data/context.
 * @param del_cb called when data is not needed anymore, either when
 *        another callback replaces @func, the tooltip is unset with
 *        elm_widget_item_tooltip_unset() or the owner @a item
 *        dies. This callback receives as the first parameter the
 *        given @a data, and @c event_info is the item.
 *
 * @ingroup Widget
 */
EAPI void
_elm_widget_item_tooltip_content_cb_set(Elm_Widget_Item *item,
                                        Elm_Tooltip_Item_Content_Cb func,
                                        const void *data,
                                        Evas_Smart_Cb del_cb)
{
   Elm_Widget_Item_Tooltip *wit;

   ELM_WIDGET_ITEM_CHECK_OR_GOTO(item, error_noitem);
   ELM_WIDGET_ITEM_RETURN_IF_ONDEL(item);
   if (!func)
     {
        _elm_widget_item_tooltip_unset(item);
        return;
     }

   wit = ELM_NEW(Elm_Widget_Item_Tooltip);
   if (!wit) goto error;
   wit->item = item;
   wit->func = func;
   wit->data = data;
   wit->del_cb = del_cb;

   elm_object_sub_tooltip_content_cb_set
     (item->view, item->widget, _elm_widget_item_tooltip_create, wit,
     _elm_widget_item_tooltip_del_cb);

   return;

error_noitem:
   if (del_cb) del_cb((void *)data, NULL, item);
   return;
error:
   if (del_cb) del_cb((void *)data, item->widget, item);
}

/**
 * @internal
 *
 * Unset tooltip from item
 *
 * @param item widget item to remove previously set tooltip.
 *
 * Remove tooltip from item. The callback provided as del_cb to
 * elm_widget_item_tooltip_content_cb_set() will be called to notify
 * it is not used anymore.
 *
 * @see elm_widget_item_tooltip_content_cb_set()
 *
 * @ingroup Widget
 */
EAPI void
_elm_widget_item_tooltip_unset(Elm_Widget_Item *item)
{
   ELM_WIDGET_ITEM_CHECK_OR_RETURN(item);
   ELM_WIDGET_ITEM_RETURN_IF_ONDEL(item);

   elm_object_tooltip_unset(item->view);
}

/**
 * @internal
 *
 * Sets a different style for this item tooltip.
 *
 * @note before you set a style you should define a tooltip with
 *       elm_widget_item_tooltip_content_cb_set() or
 *       elm_widget_item_tooltip_text_set()
 *
 * @param item widget item with tooltip already set.
 * @param style the theme style to use (default, transparent, ...)
 *
 * @ingroup Widget
 */
EAPI void
_elm_widget_item_tooltip_style_set(Elm_Widget_Item *item,
                                   const char *style)
{
   ELM_WIDGET_ITEM_CHECK_OR_RETURN(item);
   ELM_WIDGET_ITEM_RETURN_IF_ONDEL(item);

   elm_object_tooltip_style_set(item->view, style);
}

EAPI Eina_Bool
_elm_widget_item_tooltip_window_mode_set(Elm_Widget_Item *item,
                                         Eina_Bool disable)
{
   ELM_WIDGET_ITEM_CHECK_OR_RETURN(item, EINA_FALSE);
   ELM_WIDGET_ITEM_RETURN_IF_ONDEL(item, EINA_FALSE);

   return elm_object_tooltip_window_mode_set(item->view, disable);
}

EAPI Eina_Bool
_elm_widget_item_tooltip_window_mode_get(const Elm_Widget_Item *item)
{
   ELM_WIDGET_ITEM_CHECK_OR_RETURN(item, EINA_FALSE);
   return elm_object_tooltip_window_mode_get(item->view);
}

/**
 * @internal
 *
 * Get the style for this item tooltip.
 *
 * @param item widget item with tooltip already set.
 * @return style the theme style in use, defaults to "default". If the
 *         object does not have a tooltip set, then NULL is returned.
 *
 * @ingroup Widget
 */
EAPI const char *
_elm_widget_item_tooltip_style_get(const Elm_Widget_Item *item)
{
   ELM_WIDGET_ITEM_CHECK_OR_RETURN(item, NULL);
   ELM_WIDGET_ITEM_RETURN_IF_ONDEL(item, NULL);

   return elm_object_tooltip_style_get(item->view);
}

EAPI void
_elm_widget_item_cursor_set(Elm_Widget_Item *item,
                            const char *cursor)
{
   ELM_WIDGET_ITEM_CHECK_OR_RETURN(item);
   ELM_WIDGET_ITEM_RETURN_IF_ONDEL(item);

   elm_object_sub_cursor_set(item->view, item->widget, cursor);
}

EAPI const char *
_elm_widget_item_cursor_get(const Elm_Widget_Item *item)
{
   ELM_WIDGET_ITEM_CHECK_OR_RETURN(item, NULL);
   return elm_object_cursor_get(item->view);
}

EAPI void
_elm_widget_item_cursor_unset(Elm_Widget_Item *item)
{
   ELM_WIDGET_ITEM_CHECK_OR_RETURN(item);
   ELM_WIDGET_ITEM_RETURN_IF_ONDEL(item);

   elm_object_cursor_unset(item->view);
}

/**
 * @internal
 *
 * Sets a different style for this item cursor.
 *
 * @note before you set a style you should define a cursor with
 *       elm_widget_item_cursor_set()
 *
 * @param item widget item with cursor already set.
 * @param style the theme style to use (default, transparent, ...)
 *
 * @ingroup Widget
 */
EAPI void
_elm_widget_item_cursor_style_set(Elm_Widget_Item *item,
                                  const char *style)
{
   ELM_WIDGET_ITEM_CHECK_OR_RETURN(item);
   ELM_WIDGET_ITEM_RETURN_IF_ONDEL(item);

   elm_object_cursor_style_set(item->view, style);
}

/**
 * @internal
 *
 * Get the style for this item cursor.
 *
 * @param item widget item with cursor already set.
 * @return style the theme style in use, defaults to "default". If the
 *         object does not have a cursor set, then NULL is returned.
 *
 * @ingroup Widget
 */
EAPI const char *
_elm_widget_item_cursor_style_get(const Elm_Widget_Item *item)
{
   ELM_WIDGET_ITEM_CHECK_OR_RETURN(item, NULL);
   return elm_object_cursor_style_get(item->view);
}

/**
 * @internal
 *
 * Set if the cursor set should be searched on the theme or should use
 * the provided by the engine, only.
 *
 * @note before you set if should look on theme you should define a cursor
 * with elm_object_cursor_set(). By default it will only look for cursors
 * provided by the engine.
 *
 * @param item widget item with cursor already set.
 * @param engine_only boolean to define it cursors should be looked only
 * between the provided by the engine or searched on widget's theme as well.
 *
 * @ingroup Widget
 */
EAPI void
_elm_widget_item_cursor_engine_only_set(Elm_Widget_Item *item,
                                        Eina_Bool engine_only)
{
   ELM_WIDGET_ITEM_CHECK_OR_RETURN(item);
   ELM_WIDGET_ITEM_RETURN_IF_ONDEL(item);

   elm_object_cursor_theme_search_enabled_set(item->view, engine_only);
}

/**
 * @internal
 *
 * Get the cursor engine only usage for this item cursor.
 *
 * @param item widget item with cursor already set.
 * @return engine_only boolean to define it cursors should be looked only
 * between the provided by the engine or searched on widget's theme as well. If
 *         the object does not have a cursor set, then EINA_FALSE is returned.
 *
 * @ingroup Widget
 */
EAPI Eina_Bool
_elm_widget_item_cursor_engine_only_get(const Elm_Widget_Item *item)
{
   ELM_WIDGET_ITEM_CHECK_OR_RETURN(item, EINA_FALSE);
   return elm_object_cursor_theme_search_enabled_get(item->view);
}

EAPI void
_elm_widget_item_part_content_set(Elm_Widget_Item *item,
                                  const char *part,
                                  Evas_Object *content)
{
   ELM_WIDGET_ITEM_CHECK_OR_RETURN(item);
   ELM_WIDGET_ITEM_RETURN_IF_ONDEL(item);
   if (!item->content_set_func)
     {
        ERR("%s does not support elm_object_item_part_content_set() API.",
            elm_widget_type_get(item->widget));
        return;
     }
   item->content_set_func((Elm_Object_Item *)item, part, content);
}

EAPI Evas_Object *
_elm_widget_item_part_content_get(const Elm_Widget_Item *item,
                                  const char *part)
{
   ELM_WIDGET_ITEM_CHECK_OR_RETURN(item, NULL);
   ELM_WIDGET_ITEM_RETURN_IF_ONDEL(item, NULL);

   return item->content_get_func((Elm_Object_Item *)item, part);
}

EAPI Evas_Object *
_elm_widget_item_part_content_unset(Elm_Widget_Item *item,
                                    const char *part)
{
   ELM_WIDGET_ITEM_CHECK_OR_RETURN(item, NULL);
   ELM_WIDGET_ITEM_RETURN_IF_ONDEL(item, NULL);
   if (!item->content_unset_func)
     {
        ERR("%s does not support elm_object_item_part_content_unset() API.",
            elm_widget_type_get(item->widget));
        return NULL;
     }

   return item->content_unset_func((Elm_Object_Item *)item, part);
}

static const char *
_elm_widget_item_part_text_translate(Elm_Widget_Item *item,
                                     const char *part,
                                     const char *label)
{
   if (!item->translate_strings || item->on_translate) return label;
   return _part_text_translate(item->translate_strings, part, label);
}

EAPI void
_elm_widget_item_part_text_set(Elm_Widget_Item *item,
                               const char *part,
                               const char *label)
{
   ELM_WIDGET_ITEM_CHECK_OR_RETURN(item);
   ELM_WIDGET_ITEM_RETURN_IF_ONDEL(item);
   if (!item->text_set_func)
     {
        ERR("%s does not support elm_object_item_part_text_set() API.",
            elm_widget_type_get(item->widget));
        return;
     }

   label = _elm_widget_item_part_text_translate(item, part, label);
   item->text_set_func((Elm_Object_Item *)item, part, label);
}

EAPI const char *
_elm_widget_item_part_text_get(const Elm_Widget_Item *item,
                               const char *part)
{
   ELM_WIDGET_ITEM_CHECK_OR_RETURN(item, NULL);
   ELM_WIDGET_ITEM_RETURN_IF_ONDEL(item, NULL);
   if (!item->text_get_func)
     {
        ERR("%s does not support elm_object_item_part_text_get() API.",
            elm_widget_type_get(item->widget));
        return NULL;
     }

   return item->text_get_func((Elm_Object_Item *)item, part);
}

EAPI void
_elm_widget_item_content_set_hook_set(Elm_Widget_Item *item,
                                      Elm_Widget_Content_Set_Cb func)
{
   ELM_WIDGET_ITEM_CHECK_OR_RETURN(item);
   ELM_WIDGET_ITEM_RETURN_IF_ONDEL(item);

   item->content_set_func = func;
}

EAPI void
_elm_widget_item_content_get_hook_set(Elm_Widget_Item *item,
                                      Elm_Widget_Content_Get_Cb func)
{
   ELM_WIDGET_ITEM_CHECK_OR_RETURN(item);
   ELM_WIDGET_ITEM_RETURN_IF_ONDEL(item);

   item->content_get_func = func;
}

EAPI void
_elm_widget_item_content_unset_hook_set(Elm_Widget_Item *item,
                                        Elm_Widget_Content_Unset_Cb func)
{
   ELM_WIDGET_ITEM_CHECK_OR_RETURN(item);
   ELM_WIDGET_ITEM_RETURN_IF_ONDEL(item);

   item->content_unset_func = func;
}

EAPI void
_elm_widget_item_text_set_hook_set(Elm_Widget_Item *item,
                                   Elm_Widget_Text_Set_Cb func)
{
   ELM_WIDGET_ITEM_CHECK_OR_RETURN(item);
   ELM_WIDGET_ITEM_RETURN_IF_ONDEL(item);

   item->text_set_func = func;
}

EAPI void
_elm_widget_item_text_get_hook_set(Elm_Widget_Item *item,
                                   Elm_Widget_Text_Get_Cb func)
{
   ELM_WIDGET_ITEM_CHECK_OR_RETURN(item);
   ELM_WIDGET_ITEM_RETURN_IF_ONDEL(item);

   item->text_get_func = func;
}

////////////////// TIZEN only for changeable GUI ////////////////
EAPI Evas_Object *
_elm_widget_item_edje_get(Elm_Widget_Item *item)
{
   ELM_WIDGET_ITEM_CHECK_OR_RETURN(item, NULL);
   ELM_WIDGET_ITEM_RETURN_IF_ONDEL(item, NULL);

   if (_elm_widget_is(item->view))
     return elm_layout_edje_get(item->view);
   else if (item->view &&
            !strcmp(evas_object_type_get(item->view), "edje"))
     return item->view;

   return NULL;
}
////////////////////////////////////////////////////////////////

EAPI void
_elm_widget_item_signal_emit(Elm_Widget_Item *item,
                             const char *emission,
                             const char *source)
{
   ELM_WIDGET_ITEM_CHECK_OR_RETURN(item);
   ELM_WIDGET_ITEM_RETURN_IF_ONDEL(item);

   if (item->signal_emit_func)
     item->signal_emit_func((Elm_Object_Item *)item, emission, source);
}

EAPI void
_elm_widget_item_signal_emit_hook_set(Elm_Widget_Item *item,
                                      Elm_Widget_Signal_Emit_Cb func)
{
   ELM_WIDGET_ITEM_CHECK_OR_RETURN(item);
   ELM_WIDGET_ITEM_RETURN_IF_ONDEL(item);

   item->signal_emit_func = func;
}


static void
_elm_widget_item_signal_cb(void *data, Evas_Object *obj __UNUSED__, const char *emission,
                           const char *source)
{
   Elm_Widget_Item_Signal_Data *wisd = data;
   wisd->func(wisd->data, wisd->item, emission, source);
}

EAPI void
_elm_widget_item_signal_callback_add(Elm_Widget_Item *item,
                                     const char *emission,
                                     const char *source,
                                     Elm_Widget_Item_Signal_Cb func,
                                     void *data)
{
   ELM_WIDGET_ITEM_CHECK_OR_RETURN(item);
   EINA_SAFETY_ON_NULL_RETURN(func);

   Elm_Widget_Item_Signal_Data *wisd;

   wisd = malloc(sizeof(Elm_Widget_Item_Signal_Data));
   if (!wisd) return;

   wisd->item = item;
   wisd->func = func;
   wisd->data = data;
   wisd->emission = eina_stringshare_add(emission);
   wisd->source = eina_stringshare_add(source);

   if (_elm_widget_is(item->view))
     elm_object_signal_callback_add(item->view, emission, source, _elm_widget_item_signal_cb, wisd);
   else if (!strcmp(evas_object_type_get(item->view), "edje"))
     edje_object_signal_callback_add(item->view, emission, source, _elm_widget_item_signal_cb, wisd);
   else
     {
        WRN("The %s widget item doesn't support signal callback add!",
            evas_object_type_get(item->widget));
        eina_stringshare_del(emission);
        eina_stringshare_del(source);
        free(wisd);
        return;
     }

   item->signals = eina_list_append(item->signals, wisd);
}

EAPI void *
_elm_widget_item_signal_callback_del(Elm_Widget_Item *item,
                                    const char *emission,
                                    const char *source,
                                    Elm_Widget_Item_Signal_Cb func)
{
   ELM_WIDGET_ITEM_CHECK_OR_RETURN(item, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(func, NULL);

   Elm_Widget_Item_Signal_Data *wisd;
   Eina_List *l, *ll;
   void *data = NULL;

   EINA_LIST_FOREACH_SAFE(item->signals, l, ll, wisd)
     {
        if ((wisd->func == func) && !strcmp(wisd->emission, emission) &&
            !strcmp(wisd->source, source))
          {
             item->signals = eina_list_remove(item->signals, wisd);
             data = wisd->data;
             // FIXME: item->view can be removed already (e.g genlist, gengrid)
             // So, before string compare, we need to check it.
             if (item->view)
               {
                  if (_elm_widget_is(item->view))
                    elm_object_signal_callback_del(item->view, emission, source,
                                                   _elm_widget_item_signal_cb);
                  else if (!strcmp(evas_object_type_get(item->view), "edje"))
                    edje_object_signal_callback_del_full(item->view, emission,
                                                         source,
                                                         _elm_widget_item_signal_cb,
                                                         wisd);
               }
             eina_stringshare_del(wisd->emission);
             eina_stringshare_del(wisd->source);
             free(wisd);
          }
     }

   return data;
}

EAPI void
_elm_widget_item_access_info_set(Elm_Widget_Item *item,
                                 const char *txt)
{
   ELM_WIDGET_ITEM_CHECK_OR_RETURN(item);
   ELM_WIDGET_ITEM_RETURN_IF_ONDEL(item);

   if (item->access_info) eina_stringshare_del(item->access_info);
   if (!txt) item->access_info = NULL;
   else item->access_info = eina_stringshare_add(txt);
}

EAPI void
_elm_widget_item_translate(Elm_Widget_Item *item)
{
   ELM_WIDGET_ITEM_CHECK_OR_RETURN(item);

#ifdef HAVE_GETTEXT
   Elm_Translate_String_Data *ts;
   EINA_INLIST_FOREACH(item->translate_strings, ts)
     {
        if (!ts->string) continue;
        const char *s = dgettext(ts->domain, ts->string);
        item->on_translate = EINA_TRUE;
        _elm_widget_item_part_text_set(item, ts->id, s);
        item->on_translate = EINA_FALSE;
     }
#endif
}

/* happy debug functions */
#ifdef ELM_DEBUG
static void
_sub_obj_tree_dump(const Evas_Object *obj,
                   int lvl)
{
   int i;

   for (i = 0; i < lvl * 3; i++)
     putchar(' ');

   if (_elm_widget_is(obj))
     {
        Eina_List *l;
        INTERNAL_ENTRY;
        printf("+ %s(%p)\n",
               elm_widget_type_get(obj),
               obj);
        EINA_LIST_FOREACH(sd->subobjs, l, obj)
           _sub_obj_tree_dump(obj, lvl + 1);
     }
   else
     printf("+ %s(%p)\n", evas_object_type_get(obj), obj);
}

static void
_sub_obj_tree_dot_dump(const Evas_Object *obj,
                       FILE *output)
{
   if (!_elm_widget_is(obj))
     return;
   INTERNAL_ENTRY;

   Eina_Bool visible = evas_object_visible_get(obj);
   Eina_Bool disabled = elm_widget_disabled_get(obj);
   Eina_Bool focused = elm_widget_focus_get(obj);
   Eina_Bool can_focus = elm_widget_can_focus_get(obj);

   if (sd->parent_obj)
     {
        fprintf(output, "\"%p\" -- \"%p\" [ color=black", sd->parent_obj, obj);

        if (focused)
          fprintf(output, ", style=bold");

        if (!visible)
          fprintf(output, ", color=gray28");

        fprintf(output, " ];\n");
     }

   fprintf(output, "\"%p\" [ label = \"{%p|%s|%s|visible: %d|"
                   "disabled: %d|focused: %d/%d|focus order:%d}\"",
           obj, obj, elm_widget_type_get(obj),
           evas_object_name_get(obj), visible, disabled, focused, can_focus,
           sd->focus_order);

   if (focused)
     fprintf(output, ", style=bold");

   if (!visible)
     fprintf(output, ", fontcolor=gray28");

   if ((disabled) || (!visible))
     fprintf(output, ", color=gray");

   fprintf(output, " ];\n");

   Eina_List *l;
   Evas_Object *o;
   EINA_LIST_FOREACH(sd->subobjs, l, o)
     _sub_obj_tree_dot_dump(o, output);
}

#endif

EAPI void
elm_widget_tree_dump(const Evas_Object *top)
{
#ifdef ELM_DEBUG
   if (!_elm_widget_is(top))
     return;
   _sub_obj_tree_dump(top, 0);
#else
   (void)top;
   return;
#endif
}

EAPI void
elm_widget_tree_dot_dump(const Evas_Object *top,
                         FILE *output)
{
#ifdef ELM_DEBUG
   if (!_elm_widget_is(top))
     return;
   fprintf(output, "graph " " { node [shape=record];\n");
   _sub_obj_tree_dot_dump(top, output);
   fprintf(output, "}\n");
#else
   (void)top;
   (void)output;
   return;
#endif
}

// TIZEN ONLY: This is for UI Analyzer information.
EAPI char *
elm_widget_description_get(const Evas_Object *obj)
{
   API_ENTRY return EINA_FALSE;

   char *ret = NULL;

   if ((!sd->api) || (!sd->api->description_get)) return NULL;

   ret = sd->api->description_get(obj);

   return ret;
}

static Elm_Widget_Input_History_Dump_Cb input_history_dump_cb = NULL;

EAPI void
elm_widget_input_history_dump_callback_set(Elm_Widget_Input_History_Dump_Cb func)
{
   if (!input_history_dump_cb)
     {
        input_history_dump_cb = func;
     }
}

EAPI Elm_Widget_Input_History_Dump_Cb
elm_widget_input_history_dump_callback_get(void)
{
   return input_history_dump_cb;
}

