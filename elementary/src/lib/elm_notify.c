#include <Elementary.h>
#include "elm_priv.h"
#include "elm_widget_notify.h"

EAPI const char ELM_NOTIFY_SMART_NAME[] = "elm_notify";

static const char SIG_DISMISSED[] = "dismissed";
static const char SIG_BLOCK_CLICKED[] = "block,clicked";
static const char SIG_SHOW_FINISHED[] = "show,finished";
static const char SIG_TIMEOUT[] = "timeout";
static const Evas_Smart_Cb_Description _smart_callbacks[] = {
   {SIG_DISMISSED, ""},
   {SIG_SHOW_FINISHED, ""},
   {SIG_BLOCK_CLICKED, ""},
   {SIG_TIMEOUT, ""},
   {NULL, NULL}
};

EVAS_SMART_SUBCLASS_NEW
  (ELM_NOTIFY_SMART_NAME, _elm_notify, Elm_Notify_Smart_Class,
  Elm_Container_Smart_Class, elm_container_smart_class_get, NULL);

static void
_notify_theme_apply(Evas_Object *obj)
{
   const char *style = elm_widget_style_get(obj);
   double ax, ay;

   ELM_NOTIFY_DATA_GET(obj, sd);

   ax = sd->horizontal_align;
   ay = sd->vertical_align;
   if ((elm_widget_mirrored_get(obj)) && (ax != ELM_NOTIFY_ALIGN_FILL)) ax = 1.0 - ax;

   if (ay == 0.0)
     elm_widget_theme_object_set(obj, sd->notify, "notify", "top", style);
   else if (ay == 1.0)
     elm_widget_theme_object_set(obj, sd->notify, "notify", "bottom", style);
   else if (ax == 0.0)
        elm_widget_theme_object_set(obj, sd->notify, "notify", "left", style);
   else if (ax == 1.0)
        elm_widget_theme_object_set(obj, sd->notify, "notify", "right", style);
   else
     elm_widget_theme_object_set(obj, sd->notify, "notify", "center", style);
}

/**
 * Moves notification to orientation.
 *
 * This function moves notification to orientation
 * according to object RTL orientation.
 *
 * @param obj notification object.
 *
 * @param orient notification orientation.
 *
 * @internal
 **/
static void
_notify_move_to_orientation(Evas_Object *obj)
{
   Evas_Coord minw = -1, minh = -1;
   Evas_Coord x, y, w, h;
   double ax, ay;

   ELM_NOTIFY_DATA_GET(obj, sd);

   evas_object_geometry_get(obj, &x, &y, &w, &h);
   edje_object_size_min_get(sd->notify, &minw, &minh);
   edje_object_size_min_restricted_calc(sd->notify, &minw, &minh, minw, minh);

   ax = sd->horizontal_align;
   ay = sd->vertical_align;
   if ((elm_widget_mirrored_get(obj)) && (ax != ELM_NOTIFY_ALIGN_FILL)) ax = 1.0 - ax;

   if (ax == ELM_NOTIFY_ALIGN_FILL) minw = w;
   if (ay == ELM_NOTIFY_ALIGN_FILL) minh = h;

   x = x + ((w - minw) * ax);
   y = y + ((h - minh) * ay);

   evas_object_move(sd->notify, x, y);
}

static void
_block_events_theme_apply(Evas_Object *obj)
{
   ELM_NOTIFY_DATA_GET(obj, sd);

   elm_layout_theme_set(sd->block_events, "notify", "block_events", elm_widget_style_get(obj));
}

static void
_mirrored_set(Evas_Object *obj, Eina_Bool rtl)
{
   ELM_NOTIFY_DATA_GET(obj, sd);
   edje_object_mirrored_set(sd->notify, rtl);
   _notify_move_to_orientation(obj);
}

static void
_sizing_eval(Evas_Object *obj)
{
   Evas_Coord x, y, w, h;

   ELM_NOTIFY_DATA_GET(obj, sd);

   if (!sd->parent) return;
   evas_object_geometry_get(sd->parent, &x, &y, &w, &h);
   evas_object_move(obj, x, y);
   evas_object_resize(obj, w, h);
}

static Eina_Bool
_elm_notify_smart_theme(Evas_Object *obj)
{
   ELM_NOTIFY_DATA_GET(obj, sd);

   if (!ELM_WIDGET_CLASS(_elm_notify_parent_sc)->theme(obj)) return EINA_FALSE;

   _mirrored_set(obj, elm_widget_mirrored_get(obj));

   _notify_theme_apply(obj);
   if (sd->block_events) _block_events_theme_apply(obj);

   edje_object_scale_set
     (sd->notify, elm_widget_scale_get(obj) * elm_config_scale_get());

   _sizing_eval(obj);

   return EINA_TRUE;
}

static void
_calc(Evas_Object *obj)
{
   Evas_Coord minw = -1, minh = -1;
   Evas_Coord x, y, w, h;

   ELM_NOTIFY_DATA_GET(obj, sd);

   _sizing_eval(obj);

   evas_object_geometry_get(obj, &x, &y, &w, &h);

   edje_object_size_min_get(sd->notify, &minw, &minh);
   edje_object_size_min_restricted_calc(sd->notify, &minw, &minh, minw, minh);

   if (sd->horizontal_align == ELM_NOTIFY_ALIGN_FILL) minw = w;
   if (sd->vertical_align == ELM_NOTIFY_ALIGN_FILL) minh = h;

   if (sd->content)
     {
        _notify_move_to_orientation(obj);
        evas_object_resize(sd->notify, minw, minh);
     }
}

static void
_changed_size_hints_cb(void *data,
                       Evas *e __UNUSED__,
                       Evas_Object *obj __UNUSED__,
                       void *event_info __UNUSED__)
{
   _calc(data);
}

static Eina_Bool
_elm_notify_smart_sub_object_del(Evas_Object *obj,
                                 Evas_Object *sobj)
{
   ELM_NOTIFY_DATA_GET(obj, sd);

   if (!ELM_WIDGET_CLASS(_elm_notify_parent_sc)->sub_object_del(obj, sobj))
     return EINA_FALSE;

   if (sobj == sd->content)
     {
        evas_object_event_callback_del_full
          (sobj, EVAS_CALLBACK_CHANGED_SIZE_HINTS,
          _changed_size_hints_cb, obj);
        sd->content = NULL;
     }

   return EINA_TRUE;
}

static void
_block_area_clicked_cb(void *data,
                       Evas_Object *obj __UNUSED__,
                       const char *emission __UNUSED__,
                       const char *source __UNUSED__)
{
   evas_object_smart_callback_call(data, SIG_BLOCK_CLICKED, NULL);
}

static void
_elm_notify_smart_resize(Evas_Object *obj,
                         Evas_Coord w,
                         Evas_Coord h)
{
   ELM_WIDGET_CLASS(_elm_notify_parent_sc)->base.resize(obj, w, h);

   _calc(obj);
}

static void
_elm_notify_smart_move(Evas_Object *obj,
                       Evas_Coord x,
                       Evas_Coord y)
{
   ELM_WIDGET_CLASS(_elm_notify_parent_sc)->base.move(obj, x, y);

   _calc(obj);
}

static Eina_Bool
_timer_cb(void *data)
{
   const char *hide_signal;
   Evas_Object *obj = data;

   ELM_NOTIFY_DATA_GET(obj, sd);

   sd->timer = NULL;
   if (!evas_object_visible_get(obj)) goto end;

   hide_signal = edje_object_data_get(sd->notify, "hide_finished_signal");
   if ((hide_signal) && (!strcmp(hide_signal, "on")))
     edje_object_signal_emit(sd->notify, "elm,state,hide", "elm");
   else  //for backport supporting: edc without emitting hide finished signal
     evas_object_hide(obj);

   evas_object_smart_callback_call(obj, SIG_TIMEOUT, NULL);

end:
   return ECORE_CALLBACK_CANCEL;
}

static void
_timer_init(Evas_Object *obj,
            Elm_Notify_Smart_Data *sd)
{
   if (sd->timer)
     {
        ecore_timer_del(sd->timer);
        sd->timer = NULL;
     }
   if (sd->timeout > 0.0)
     sd->timer = ecore_timer_add(sd->timeout, _timer_cb, obj);
}

static void
_elm_notify_smart_show(Evas_Object *obj)
{
   char buf[1028];
   int rotation = -1;

   ELM_NOTIFY_DATA_GET(obj, sd);

   ELM_WIDGET_CLASS(_elm_notify_parent_sc)->base.show(obj);

   evas_object_show(sd->notify);
   if (!sd->allow_events) evas_object_show(sd->block_events);
   evas_object_raise(sd->notify);
   _timer_init(obj, sd);
   elm_object_focus_set(obj, EINA_TRUE);

   rotation = elm_win_rotation_get(elm_widget_top_get(obj));
   snprintf(buf, sizeof(buf), "elm,state,orient,%d", rotation);
   edje_object_signal_emit(sd->notify, buf, "elm");
}

static void
_elm_notify_smart_hide(Evas_Object *obj)
{
   ELM_NOTIFY_DATA_GET(obj, sd);

   ELM_WIDGET_CLASS(_elm_notify_parent_sc)->base.hide(obj);

   evas_object_hide(sd->notify);
   if (!sd->allow_events) evas_object_hide(sd->block_events);
   if (sd->timer)
     {
        ecore_timer_del(sd->timer);
        sd->timer = NULL;
     }
}

static void
_parent_del_cb(void *data,
               Evas *e __UNUSED__,
               Evas_Object *obj __UNUSED__,
               void *event_info __UNUSED__)
{
   elm_notify_parent_set(data, NULL);
   evas_object_hide(data);
}

static void
_parent_hide_cb(void *data,
                Evas *e __UNUSED__,
                Evas_Object *obj __UNUSED__,
                void *event_info __UNUSED__)
{
   evas_object_hide(data);
}

static Eina_Bool
_elm_notify_smart_focus_next(const Evas_Object *obj,
                             Elm_Focus_Direction dir,
                             Evas_Object **next)
{
   Evas_Object *cur;

   ELM_NOTIFY_DATA_GET(obj, sd);

   if (!sd->content)
     return EINA_FALSE;

   cur = sd->content;

   /* Try to cycle focus on content */
   return elm_widget_focus_next_get(cur, dir, next);
}
static void
_elm_notify_smart_orientation_set(Evas_Object *obj, int rotation)
{
   char buf[128];

   ELM_NOTIFY_DATA_GET(obj, sd);

   snprintf(buf, sizeof(buf), "elm,state,orient,%d", rotation);
   edje_object_signal_emit(sd->notify, buf, "elm");
}

static Eina_Bool
_elm_notify_smart_focus_direction_manager_is(const Evas_Object *obj __UNUSED__)
{
   return EINA_TRUE;
}

static Eina_Bool
_elm_notify_smart_focus_direction(const Evas_Object *obj,
                                  const Evas_Object *base,
                                  double degree,
                                  Evas_Object **direction,
                                  double *weight)
{
   Evas_Object *cur;

   ELM_NOTIFY_DATA_GET(obj, sd);

   if (!sd->content)
     return EINA_FALSE;

   cur = sd->content;

   return elm_widget_focus_direction_get(cur, base, degree, direction, weight);
}

static Eina_Bool
_elm_notify_smart_content_set(Evas_Object *obj,
                              const char *part,
                              Evas_Object *content)
{
   ELM_NOTIFY_DATA_GET(obj, sd);

   if (part && strcmp(part, "default")) return EINA_FALSE;
   if (sd->content == content) return EINA_TRUE;
   if (sd->content) evas_object_del(sd->content);
   sd->content = content;

   if (content)
     {
        elm_widget_sub_object_add(obj, content);
        evas_object_event_callback_add
          (content, EVAS_CALLBACK_CHANGED_SIZE_HINTS,
          _changed_size_hints_cb, obj);
        edje_object_part_swallow(sd->notify, "elm.swallow.content", content);
     }

   _sizing_eval(obj);
   _calc(obj);

   return EINA_TRUE;
}

static Evas_Object *
_elm_notify_smart_content_get(const Evas_Object *obj,
                              const char *part)
{
   ELM_NOTIFY_DATA_GET(obj, sd);

   if (part && strcmp(part, "default")) return NULL;

   return sd->content;
}

static Evas_Object *
_elm_notify_smart_content_unset(Evas_Object *obj,
                                const char *part)
{
   Evas_Object *content;

   ELM_NOTIFY_DATA_GET(obj, sd);

   if (part && strcmp(part, "default")) return NULL;
   if (!sd->content) return NULL;

   content = sd->content;
   elm_widget_sub_object_del(obj, sd->content);
   edje_object_part_unswallow(sd->notify, content);

   return content;
}

static void
_elm_notify_smart_add(Evas_Object *obj)
{
   EVAS_SMART_DATA_ALLOC(obj, Elm_Notify_Smart_Data);

   ELM_WIDGET_CLASS(_elm_notify_parent_sc)->base.add(obj);
}

static void
_elm_notify_smart_del(Evas_Object *obj)
{
   ELM_NOTIFY_DATA_GET(obj, sd);

   elm_notify_parent_set(obj, NULL);
   elm_notify_allow_events_set(obj, EINA_FALSE);
   if (sd->timer)
     {
        ecore_timer_del(sd->timer);
        sd->timer = NULL;
     }

   ELM_SAFE_FREE(sd->notify, evas_object_del);
   ELM_SAFE_FREE(sd->block_events, evas_object_del);
   ELM_WIDGET_CLASS(_elm_notify_parent_sc)->base.del(obj);
}

static void
_elm_notify_smart_parent_set(Evas_Object *obj,
                             Evas_Object *parent)
{
   elm_notify_parent_set(obj, parent);

   _sizing_eval(obj);
}

static void
_elm_notify_smart_set_user(Elm_Notify_Smart_Class *sc)
{
   ELM_WIDGET_CLASS(sc)->base.add = _elm_notify_smart_add;
   ELM_WIDGET_CLASS(sc)->base.del = _elm_notify_smart_del;

   ELM_WIDGET_CLASS(sc)->base.resize = _elm_notify_smart_resize;
   ELM_WIDGET_CLASS(sc)->base.move = _elm_notify_smart_move;
   ELM_WIDGET_CLASS(sc)->base.show = _elm_notify_smart_show;
   ELM_WIDGET_CLASS(sc)->base.hide = _elm_notify_smart_hide;

   ELM_WIDGET_CLASS(sc)->parent_set = _elm_notify_smart_parent_set;
   ELM_WIDGET_CLASS(sc)->theme = _elm_notify_smart_theme;
   ELM_WIDGET_CLASS(sc)->focus_next = _elm_notify_smart_focus_next;
   ELM_WIDGET_CLASS(sc)->focus_direction_manager_is =
      _elm_notify_smart_focus_direction_manager_is;
   ELM_WIDGET_CLASS(sc)->focus_direction = _elm_notify_smart_focus_direction;
   ELM_WIDGET_CLASS(sc)->orientation_set = _elm_notify_smart_orientation_set;

   ELM_WIDGET_CLASS(sc)->sub_object_del = _elm_notify_smart_sub_object_del;

   ELM_CONTAINER_CLASS(sc)->content_set = _elm_notify_smart_content_set;
   ELM_CONTAINER_CLASS(sc)->content_get = _elm_notify_smart_content_get;
   ELM_CONTAINER_CLASS(sc)->content_unset = _elm_notify_smart_content_unset;
}

EAPI const Elm_Notify_Smart_Class *
elm_notify_smart_class_get(void)
{
   static Elm_Notify_Smart_Class _sc =
     ELM_NOTIFY_SMART_CLASS_INIT_NAME_VERSION(ELM_NOTIFY_SMART_NAME);
   static const Elm_Notify_Smart_Class *class = NULL;
   Evas_Smart_Class *esc = (Evas_Smart_Class *)&_sc;

   if (class) return class;

   _elm_notify_smart_set(&_sc);
   esc->callbacks = _smart_callbacks;
   class = &_sc;

   return class;
}
static void
_notify_show_finished_cb(void *data,
                  Evas_Object *obj __UNUSED__,
                  const char *emission __UNUSED__,
                  const char *source __UNUSED__)
{
   evas_object_smart_callback_call(data, SIG_SHOW_FINISHED, NULL);
}

static void
_hide_effect_finished_cb(void *data,
                  Evas_Object *obj __UNUSED__,
                  const char *emission __UNUSED__,
                  const char *source __UNUSED__)
{
   evas_object_smart_callback_call(data, SIG_DISMISSED, NULL);
}

EAPI Evas_Object *
elm_notify_add(Evas_Object *parent)
{
   Evas_Object *obj;

   EINA_SAFETY_ON_NULL_RETURN_VAL(parent, NULL);

   obj = elm_widget_add(_elm_notify_smart_class_new(), parent);
   if (!obj) return NULL;

   if (!elm_widget_sub_object_add(parent, obj))
     ERR("could not add %p as sub object of %p", obj, parent);

   ELM_NOTIFY_DATA_GET(obj, sd);

   sd->allow_events = EINA_TRUE;

   sd->notify = edje_object_add(evas_object_evas_get(obj));
   evas_object_smart_member_add(sd->notify, obj);

   sd->horizontal_align = 0.5;
   sd->vertical_align = 0.0;

   edje_object_signal_callback_add
     (sd->notify, "elm,action,show,finished", "", _notify_show_finished_cb, obj);
   edje_object_signal_callback_add
     (sd->notify, "elm,state,hide,finished", "elm", _hide_effect_finished_cb, obj);

   elm_widget_can_focus_set(obj, EINA_FALSE);
   elm_notify_align_set(obj, 0.5, 0.0);

   //Tizen Only: This should be removed when eo is applied.
   ELM_WIDGET_DATA_GET(obj, wsd);
   wsd->on_create = EINA_FALSE;

   return obj;
}

EAPI void
elm_notify_parent_set(Evas_Object *obj,
                      Evas_Object *parent)
{
   ELM_NOTIFY_CHECK(obj);
   ELM_NOTIFY_DATA_GET(obj, sd);

   if (sd->parent)
     {
        evas_object_event_callback_del_full
          (sd->parent, EVAS_CALLBACK_CHANGED_SIZE_HINTS,
          _changed_size_hints_cb, obj);
        evas_object_event_callback_del_full
          (sd->parent, EVAS_CALLBACK_RESIZE, _changed_size_hints_cb, obj);
        evas_object_event_callback_del_full
          (sd->parent, EVAS_CALLBACK_MOVE, _changed_size_hints_cb, obj);
        evas_object_event_callback_del_full
          (sd->parent, EVAS_CALLBACK_DEL, _parent_del_cb, obj);
        evas_object_event_callback_del_full
          (sd->parent, EVAS_CALLBACK_HIDE, _parent_hide_cb, obj);
        sd->parent = NULL;
     }

   if (parent)
     {
        sd->parent = parent;
        evas_object_event_callback_add
          (parent, EVAS_CALLBACK_CHANGED_SIZE_HINTS,
          _changed_size_hints_cb, obj);
        evas_object_event_callback_add
          (parent, EVAS_CALLBACK_RESIZE, _changed_size_hints_cb, obj);
        evas_object_event_callback_add
          (parent, EVAS_CALLBACK_MOVE, _changed_size_hints_cb, obj);
        evas_object_event_callback_add
          (parent, EVAS_CALLBACK_DEL, _parent_del_cb, obj);
        evas_object_event_callback_add
          (parent, EVAS_CALLBACK_HIDE, _parent_hide_cb, obj);
        _sizing_eval(obj);
     }

   _calc(obj);
}

EAPI Evas_Object *
elm_notify_parent_get(const Evas_Object *obj)
{
   ELM_NOTIFY_CHECK(obj) NULL;
   ELM_NOTIFY_DATA_GET(obj, sd);

   return sd->parent;
}

EINA_DEPRECATED EAPI void
elm_notify_orient_set(Evas_Object *obj,
                      Elm_Notify_Orient orient)
{
   double horizontal = 0, vertical = 0;

   switch (orient)
     {
      case ELM_NOTIFY_ORIENT_TOP:
         horizontal = 0.5; vertical = 0.0;
        break;

      case ELM_NOTIFY_ORIENT_CENTER:
         horizontal = 0.5; vertical = 0.5;
        break;

      case ELM_NOTIFY_ORIENT_BOTTOM:
         horizontal = 0.5; vertical = 1.0;
        break;

      case ELM_NOTIFY_ORIENT_LEFT:
         horizontal = 0.0; vertical = 0.5;
        break;

      case ELM_NOTIFY_ORIENT_RIGHT:
         horizontal = 1.0; vertical = 0.5;
        break;

      case ELM_NOTIFY_ORIENT_TOP_LEFT:
         horizontal = 0.0; vertical = 0.0;
        break;

      case ELM_NOTIFY_ORIENT_TOP_RIGHT:
         horizontal = 1.0; vertical = 0.0;
        break;

      case ELM_NOTIFY_ORIENT_BOTTOM_LEFT:
         horizontal = 0.0; vertical = 1.0;
        break;

      case ELM_NOTIFY_ORIENT_BOTTOM_RIGHT:
         horizontal = 1.0; vertical = 1.0;
        break;

      case ELM_NOTIFY_ORIENT_LAST:
        break;
     }
   elm_notify_align_set(obj, horizontal, vertical);
}

EINA_DEPRECATED EAPI Elm_Notify_Orient
elm_notify_orient_get(const Evas_Object *obj)
{
   Elm_Notify_Orient orient;
   double horizontal, vertical;

   elm_notify_align_get(obj, &horizontal, &vertical);

   if ((horizontal == 0.5) && (vertical == 0.0))
     orient = ELM_NOTIFY_ORIENT_TOP;
   else if ((horizontal == 0.5) && (vertical == 0.5))
     orient = ELM_NOTIFY_ORIENT_CENTER;
   else if ((horizontal == 0.5) && (vertical == 1.0))
     orient = ELM_NOTIFY_ORIENT_BOTTOM;
   else if ((horizontal == 0.0) && (vertical == 0.5))
     orient = ELM_NOTIFY_ORIENT_LEFT;
   else if ((horizontal == 1.0) && (vertical == 0.5))
     orient = ELM_NOTIFY_ORIENT_RIGHT;
   else if ((horizontal == 0.0) && (vertical == 0.0))
     orient = ELM_NOTIFY_ORIENT_TOP_LEFT;
   else if ((horizontal == 1.0) && (vertical == 0.0))
     orient = ELM_NOTIFY_ORIENT_TOP_RIGHT;
   else if ((horizontal == 0.0) && (vertical == 1.0))
     orient = ELM_NOTIFY_ORIENT_BOTTOM_LEFT;
   else if ((horizontal == 1.0) && (vertical == 1.0))
     orient = ELM_NOTIFY_ORIENT_BOTTOM_RIGHT;
   else
     orient = ELM_NOTIFY_ORIENT_TOP;
   return orient;
}

EAPI void
elm_notify_timeout_set(Evas_Object *obj,
                       double timeout)
{
   ELM_NOTIFY_CHECK(obj);
   ELM_NOTIFY_DATA_GET(obj, sd);

   sd->timeout = timeout;
   _timer_init(obj, sd);
}

EAPI double
elm_notify_timeout_get(const Evas_Object *obj)
{
   ELM_NOTIFY_CHECK(obj) 0.0;
   ELM_NOTIFY_DATA_GET(obj, sd);

   return sd->timeout;
}

EAPI void
elm_notify_allow_events_set(Evas_Object *obj,
                            Eina_Bool allow)
{
   ELM_NOTIFY_CHECK(obj);
   ELM_NOTIFY_DATA_GET(obj, sd);
   Evas_Object *win;

   if (allow == sd->allow_events) return;
   sd->allow_events = allow;
   if (!allow)
     {
        win = elm_widget_top_get(obj);
        if (win && !strcmp(evas_object_type_get(win), "elm_win"))
          {
             sd->block_events = elm_layout_add(win);
             evas_object_size_hint_weight_set(sd->block_events,
                                              EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
             elm_win_resize_object_add(win, sd->block_events);
             evas_object_smart_member_add(sd->block_events, obj);
          }
        else
          {
             sd->block_events = elm_layout_add(obj);
             elm_widget_resize_object_set(obj, sd->block_events, EINA_TRUE);
          }
        evas_object_stack_above(sd->notify, sd->block_events);
        _block_events_theme_apply(obj);
        elm_layout_signal_callback_add
          (sd->block_events, "elm,action,click", "elm",
          _block_area_clicked_cb, obj);
        elm_layout_signal_callback_add
          (sd->block_events, "elm,state,hide,finished", "elm",
          _hide_effect_finished_cb, obj);
     }
   else
     {
        evas_object_del(sd->block_events);
        sd->block_events = NULL;
     }
}

EAPI Eina_Bool
elm_notify_allow_events_get(const Evas_Object *obj)
{
   ELM_NOTIFY_CHECK(obj) EINA_FALSE;
   ELM_NOTIFY_DATA_GET(obj, sd);

   return sd->allow_events;
}

EAPI void
elm_notify_align_set(Evas_Object *obj, double horizontal, double vertical)
{
   ELM_NOTIFY_CHECK(obj);
   ELM_NOTIFY_DATA_GET(obj, sd);

   sd->horizontal_align = horizontal;
   sd->vertical_align = vertical;

   _notify_theme_apply(obj);
   _calc(obj);
}

EAPI void
elm_notify_align_get(const Evas_Object *obj, double *horizontal, double *vertical)
{
   ELM_NOTIFY_CHECK(obj);
   ELM_NOTIFY_DATA_GET(obj, sd);

   *horizontal = sd->horizontal_align;
   *vertical = sd->vertical_align;
}

/* This is temporary API. Do not use this, it will be removed.*/
EAPI void
elm_notify_dismiss(Evas_Object *obj)
{
   ELM_NOTIFY_CHECK(obj);
   ELM_NOTIFY_DATA_GET(obj, sd);

   elm_layout_signal_emit(sd->block_events, "elm,state,hide", "elm");
   edje_object_signal_emit(sd->notify, "elm,state,hide", "elm");
}
