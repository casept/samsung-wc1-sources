#include <Elementary.h>
#include "elm_priv.h"
#include "elm_widget_mapbuf.h"

EAPI const char ELM_MAPBUF_SMART_NAME[] = "elm_mapbuf";

EVAS_SMART_SUBCLASS_NEW
  (ELM_MAPBUF_SMART_NAME, _elm_mapbuf, Elm_Mapbuf_Smart_Class,
  Elm_Container_Smart_Class, elm_container_smart_class_get, NULL);

static void
_sizing_eval(Evas_Object *obj)
{
   Evas_Coord minw = -1, minh = -1;
   Evas_Coord maxw = -1, maxh = -1;

   ELM_MAPBUF_DATA_GET(obj, sd);
   if (sd->content)
     {
        evas_object_size_hint_min_get(sd->content, &minw, &minh);
        evas_object_size_hint_max_get(sd->content, &maxw, &maxh);
     }
   evas_object_size_hint_min_set(obj, minw, minh);
   evas_object_size_hint_max_set(obj, maxw, maxh);
}

static Eina_Bool
_elm_mapbuf_smart_theme(Evas_Object *obj)
{
   if (!ELM_WIDGET_CLASS(_elm_mapbuf_parent_sc)->theme(obj)) return EINA_FALSE;

   _sizing_eval(obj);

   return EINA_TRUE;
}

static void
_changed_size_hints_cb(void *data,
                       Evas *e __UNUSED__,
                       Evas_Object *obj __UNUSED__,
                       void *event_info __UNUSED__)
{
   _sizing_eval(data);
}

static void
_elm_mapbuf_content_unset(Elm_Mapbuf_Smart_Data *sd, Evas_Object *obj,
                          Evas_Object *content)
{
   evas_object_smart_member_del(content);
   evas_object_data_del(content, "_elm_leaveme");
   evas_object_clip_unset(content);
   evas_object_event_callback_del_full(content,
                                       EVAS_CALLBACK_CHANGED_SIZE_HINTS,
                                       _changed_size_hints_cb, obj);
   evas_object_color_set(ELM_WIDGET_DATA(sd)->resize_obj, 0, 0, 0, 0);

   sd->content = NULL;
   _sizing_eval(obj);
}

static Eina_Bool
_elm_mapbuf_smart_sub_object_del(Evas_Object *obj,
                                 Evas_Object *sobj)
{
   ELM_MAPBUF_DATA_GET(obj, sd);

   if (!ELM_WIDGET_CLASS(_elm_mapbuf_parent_sc)->sub_object_del(obj, sobj))
     return EINA_FALSE;

   if (sobj == sd->content) _elm_mapbuf_content_unset(sd, obj, sobj);

   return EINA_TRUE;
}

static void
_configure(Evas_Object *obj)
{
   ELM_MAPBUF_DATA_GET(obj, sd);

   Evas_Coord x = 0, y = 0, w = 0, h = 0;
   int i;

   if (!sd->content) return;

   evas_object_geometry_get(ELM_WIDGET_DATA(sd)->resize_obj, &x, &y, &w, &h);
   if (sd->enabled)
     {
        if (!sd->map) sd->map = evas_map_new(4);
        evas_map_util_points_populate_from_geometry(sd->map, x, y, w, h, 0);
        for (i = 0; i < (int)(sizeof(sd->colors)/sizeof(sd->colors[0])); i++)
          {
             evas_map_point_color_set(sd->map, i, sd->colors[i].r,
                                      sd->colors[i].g, sd->colors[i].b,
                                      sd->colors[i].a);
          }
        evas_map_smooth_set(sd->map, sd->smooth);
        evas_map_alpha_set(sd->map, sd->alpha);
        evas_object_map_set(sd->content, sd->map);
        evas_object_map_enable_set(sd->content, EINA_TRUE);
     }
   else
     {
        evas_object_move(sd->content, x, y);
     }
}

static void
_elm_mapbuf_smart_move(Evas_Object *obj,
                       Evas_Coord x,
                       Evas_Coord y)
{
   ELM_WIDGET_CLASS(_elm_mapbuf_parent_sc)->base.move(obj, x, y);

   _configure(obj);
}

static void
_elm_mapbuf_smart_resize(Evas_Object *obj,
                         Evas_Coord w,
                         Evas_Coord h)
{
   ELM_WIDGET_CLASS(_elm_mapbuf_parent_sc)->base.resize(obj, w, h);

   ELM_MAPBUF_DATA_GET(obj, sd);

   if (sd->content) evas_object_resize(sd->content, w, h);
   _configure(obj);
}

static Eina_Bool
_elm_mapbuf_smart_content_set(Evas_Object *obj,
                              const char *part,
                              Evas_Object *content)
{
   ELM_MAPBUF_DATA_GET(obj, sd);

   if (part && strcmp(part, "default")) return EINA_FALSE;
   if (sd->content == content) return EINA_TRUE;

   if (sd->content) evas_object_del(sd->content);
   sd->content = content;

   if (content)
     {
        evas_object_data_set(content, "_elm_leaveme", (void *)1);
        elm_widget_sub_object_add(obj, content);
        evas_object_smart_member_add(content, obj);
        evas_object_clip_set(content, ELM_WIDGET_DATA(sd)->resize_obj);
        evas_object_color_set
          (ELM_WIDGET_DATA(sd)->resize_obj, 255, 255, 255, 255);
        evas_object_event_callback_add
          (content, EVAS_CALLBACK_CHANGED_SIZE_HINTS,
          _changed_size_hints_cb, obj);
     }
   else
     evas_object_color_set(ELM_WIDGET_DATA(sd)->resize_obj, 0, 0, 0, 0);

   _sizing_eval(obj);
   _configure(obj);

   return EINA_TRUE;
}

static Evas_Object *
_elm_mapbuf_smart_content_get(const Evas_Object *obj,
                              const char *part)
{
   ELM_MAPBUF_DATA_GET(obj, sd);

   if (part && strcmp(part, "default")) return NULL;
   return sd->content;
}

static Evas_Object *
_elm_mapbuf_smart_content_unset(Evas_Object *obj,
                                const char *part)
{
   Evas_Object *content;

   ELM_MAPBUF_DATA_GET(obj, sd);

   if (part && strcmp(part, "default")) return NULL;
   if (!sd->content) return NULL;

   content = sd->content;
   elm_widget_sub_object_del(obj, content);
   _elm_mapbuf_content_unset(sd, obj, content);
   return content;
}

static void
_elm_mapbuf_smart_del(Evas_Object *obj)
{
   ELM_MAPBUF_DATA_GET(obj, sd);

   if (sd->map) evas_map_free(sd->map);

   ELM_WIDGET_CLASS(_elm_mapbuf_parent_sc)->base.del(obj);
}

static void
_elm_mapbuf_smart_add(Evas_Object *obj)
{
   int i;

   EVAS_SMART_DATA_ALLOC(obj, Elm_Mapbuf_Smart_Data);

   Evas_Object *rect = evas_object_rectangle_add(evas_object_evas_get(obj));
   elm_widget_resize_object_set(obj, rect, EINA_TRUE);

   ELM_WIDGET_CLASS(_elm_mapbuf_parent_sc)->base.add(obj);

   evas_object_static_clip_set(rect, EINA_TRUE);
   evas_object_pass_events_set(rect, EINA_TRUE);
   evas_object_color_set(rect, 0, 0, 0, 0);

   priv->enabled = 0;
   priv->alpha = 1;
   priv->smooth = 1;

   for (i = 0; i < (int)(sizeof(priv->colors)/sizeof(priv->colors[0])); i++)
     {
        priv->colors[i].r = 255;
        priv->colors[i].g = 255;
        priv->colors[i].b = 255;
        priv->colors[i].a = 255;
     }

   elm_widget_can_focus_set(obj, EINA_FALSE);

   _sizing_eval(obj);
}

static Eina_Bool
_elm_mapbuf_smart_focus_next(const Evas_Object *obj,
                             Elm_Focus_Direction dir,
                             Evas_Object **next)
{
   ELM_MAPBUF_CHECK(obj) EINA_FALSE;
   ELM_MAPBUF_DATA_GET(obj, sd);

   if (sd->content)
     return elm_widget_focus_next_get(sd->content, dir, next);

   return EINA_FALSE;
}

static Eina_Bool
_elm_mapbuf_smart_focus_direction_manager_is(const Evas_Object *obj __UNUSED__)
{
   return EINA_TRUE;
}

static Eina_Bool
_elm_mapbuf_smart_focus_direction(const Evas_Object *obj,
                                  const Evas_Object *base,
                                  double degree,
                                  Evas_Object **direction,
                                  double *weight)
{
   ELM_MAPBUF_CHECK(obj) EINA_FALSE;
   ELM_MAPBUF_DATA_GET(obj, sd);

   if (sd->content)
     return elm_widget_focus_direction_get(sd->content, base, degree,
                                           direction, weight);

   return EINA_FALSE;
}

static void
_elm_mapbuf_smart_set_user(Elm_Mapbuf_Smart_Class *sc)
{
   ELM_WIDGET_CLASS(sc)->base.add = _elm_mapbuf_smart_add;
   ELM_WIDGET_CLASS(sc)->base.del = _elm_mapbuf_smart_del;
   ELM_WIDGET_CLASS(sc)->base.resize = _elm_mapbuf_smart_resize;
   ELM_WIDGET_CLASS(sc)->base.move = _elm_mapbuf_smart_move;

   ELM_WIDGET_CLASS(sc)->theme = _elm_mapbuf_smart_theme;
   ELM_WIDGET_CLASS(sc)->sub_object_del = _elm_mapbuf_smart_sub_object_del;

   ELM_WIDGET_CLASS(sc)->focus_next = _elm_mapbuf_smart_focus_next;
   ELM_WIDGET_CLASS(sc)->focus_direction_manager_is =
      _elm_mapbuf_smart_focus_direction_manager_is;
   ELM_WIDGET_CLASS(sc)->focus_direction = _elm_mapbuf_smart_focus_direction;

   ELM_CONTAINER_CLASS(sc)->content_set = _elm_mapbuf_smart_content_set;
   ELM_CONTAINER_CLASS(sc)->content_get = _elm_mapbuf_smart_content_get;
   ELM_CONTAINER_CLASS(sc)->content_unset = _elm_mapbuf_smart_content_unset;
}

EAPI const Elm_Mapbuf_Smart_Class *
elm_mapbuf_smart_class_get(void)
{
   static Elm_Mapbuf_Smart_Class _sc =
     ELM_MAPBUF_SMART_CLASS_INIT_NAME_VERSION(ELM_MAPBUF_SMART_NAME);
   static const Elm_Mapbuf_Smart_Class *class = NULL;

   if (class) return class;

   _elm_mapbuf_smart_set(&_sc);
   class = &_sc;

   return class;
}

EAPI Evas_Object *
elm_mapbuf_add(Evas_Object *parent)
{
   Evas_Object *obj;

   EINA_SAFETY_ON_NULL_RETURN_VAL(parent, NULL);

   obj = elm_widget_add(_elm_mapbuf_smart_class_new(), parent);
   if (!obj) return NULL;

   if (!elm_widget_sub_object_add(parent, obj))
     ERR("could not add %p as sub object of %p", obj, parent);

   //Tizen Only: This should be removed when eo is applied.
   ELM_WIDGET_DATA_GET(obj, sd);
   sd->on_create = EINA_FALSE;

   return obj;
}

EAPI void
elm_mapbuf_enabled_set(Evas_Object *obj,
                       Eina_Bool enabled)
{
   ELM_MAPBUF_CHECK(obj);
   ELM_MAPBUF_DATA_GET(obj, sd);

   if (sd->enabled == enabled) return;
   sd->enabled = enabled;

   if (!enabled && sd->content)
     {
        evas_object_map_set(sd->content, NULL);
        evas_object_map_enable_set(sd->content, EINA_FALSE);
     }

   if (sd->content) evas_object_static_clip_set(sd->content, sd->enabled);

   _configure(obj);
}

EAPI Eina_Bool
elm_mapbuf_enabled_get(const Evas_Object *obj)
{
   ELM_MAPBUF_CHECK(obj) EINA_FALSE;
   ELM_MAPBUF_DATA_GET(obj, sd);

   return sd->enabled;
}

EAPI void
elm_mapbuf_smooth_set(Evas_Object *obj,
                      Eina_Bool smooth)
{
   ELM_MAPBUF_CHECK(obj);
   ELM_MAPBUF_DATA_GET(obj, sd);

   if (sd->smooth == smooth) return;
   sd->smooth = smooth;
   _configure(obj);
}

EAPI Eina_Bool
elm_mapbuf_smooth_get(const Evas_Object *obj)
{
   ELM_MAPBUF_CHECK(obj) EINA_FALSE;
   ELM_MAPBUF_DATA_GET(obj, sd);

   return sd->smooth;
}

EAPI void
elm_mapbuf_alpha_set(Evas_Object *obj,
                     Eina_Bool alpha)
{
   ELM_MAPBUF_CHECK(obj);
   ELM_MAPBUF_DATA_GET(obj, sd);

   if (sd->alpha == alpha) return;
   sd->alpha = alpha;
   _configure(obj);
}

EAPI Eina_Bool
elm_mapbuf_alpha_get(const Evas_Object *obj)
{
   ELM_MAPBUF_CHECK(obj) EINA_FALSE;
   ELM_MAPBUF_DATA_GET(obj, sd);

   return sd->alpha;
}

EAPI void
elm_mapbuf_point_color_set(Evas_Object *obj, int idx,
                           int r, int g, int b, int a)
{
   ELM_MAPBUF_CHECK(obj);
   ELM_MAPBUF_DATA_GET(obj, sd);

   if ((idx < 0) || (idx >= (int)(sizeof(sd->colors)/sizeof(sd->colors[0]))))
     {
        ERR("idx value should be 0 ~ %d, mapbuf(%p)",
            ((sizeof(sd->colors)/sizeof(sd->colors[0])) - 1),
             obj);
        return;
     }

   sd->colors[idx].r = r;
   sd->colors[idx].g = g;
   sd->colors[idx].b = b;
   sd->colors[idx].a = a;

   _configure(obj);
}

EAPI void
elm_mapbuf_point_color_get(Evas_Object *obj, int idx,
                           int *r, int *g, int *b, int *a)
{
   ELM_MAPBUF_CHECK(obj);
   ELM_MAPBUF_DATA_GET(obj, sd);

   if ((idx < 0) || (idx >= (int)(sizeof(sd->colors)/sizeof(sd->colors[0]))))
     {
        ERR("idx value should be 0 ~ %d, mapbuf(%p)",
            ((sizeof(sd->colors)/sizeof(sd->colors[0])) - 1),
             obj);
        return;
     }

   *r = sd->colors[idx].r;
   *g = sd->colors[idx].g;
   *b = sd->colors[idx].b;
   *a = sd->colors[idx].a;
}
