#include <Elementary.h>
#include "elm_priv.h"
#include "elm_widget_plug.h"

EAPI const char ELM_PLUG_SMART_NAME[] = "elm_plug";

static const char PLUG_KEY[] = "__Plug_Ecore_Evas";

static const char SIG_CLICKED[] = "clicked";
static const char SIG_IMAGE_DELETED[] = "image.deleted";
static const char SIG_MESSAGE_RECEIVED[] = "message.received";
static const Evas_Smart_Cb_Description _smart_callbacks[] = {
   {SIG_CLICKED, ""},
   {SIG_IMAGE_DELETED, ""},
   {SIG_MESSAGE_RECEIVED, ""},
   {NULL, NULL}
};

EVAS_SMART_SUBCLASS_NEW
  (ELM_PLUG_SMART_NAME, _elm_plug, Elm_Plug_Smart_Class,
  Elm_Widget_Smart_Class, elm_widget_smart_class_get, _smart_callbacks);

static void
_sizing_eval(Evas_Object *obj __UNUSED__)
{
   //Evas_Coord minw = -1, minh = -1, maxw = -1, maxh = -1;

   //TODO: get socket object size
   //this reset plug's min/max size
   //evas_object_size_hint_min_set(obj, minw, minh);
   //evas_object_size_hint_max_set(obj, maxw, maxh);
}

static void
_elm_plug_disconnected(Ecore_Evas *ee)
{
   Evas_Object *plug = NULL;

   if (!ee) return;
   plug = ecore_evas_data_get(ee, PLUG_KEY);
   if (!plug) return;
   evas_object_smart_callback_call(plug, SIG_IMAGE_DELETED, NULL);
}

static Eina_Bool
_elm_plug_smart_theme(Evas_Object *obj)
{
   if (!_elm_plug_parent_sc->theme(obj)) return EINA_FALSE;

   _sizing_eval(obj);

   return EINA_TRUE;
}

static Eina_Bool
_access_action_release_cb(void *data __UNUSED__,
                          Evas_Object *obj __UNUSED__,
                          Elm_Access_Action_Info *action_info __UNUSED__)
{
   return EINA_FALSE;
}

static void
_elm_plug_ecore_evas_msg_handle(Ecore_Evas *ee, int msg_domain, int msg_id, void *data, int size)
{
   Evas_Object *plug, *parent;
   Elm_Plug_Message pm;

   if (!data) return;
   DBG("Elm plug receive msg from socket ee=%p msg_domain=%x msg_id=%x size=%d", ee, msg_domain, msg_id, size);
   //get plug object form ee
   plug = (Evas_Object *)ecore_evas_data_get(ee, PLUG_KEY);
   ELM_PLUG_CHECK(plug);
   pm.msg_domain = msg_domain;
   pm.msg_id = msg_id;
   pm.data = data;
   pm.size = size;

   evas_object_smart_callback_call(plug, SIG_MESSAGE_RECEIVED, &pm);

   /* get message from parent */
   if (msg_domain == MSG_DOMAIN_CONTROL_ACCESS)
     {
        if (msg_id == ELM_ACCESS_ACTION_HIGHLIGHT_NEXT ||
            msg_id == ELM_ACCESS_ACTION_HIGHLIGHT_PREV)
          {
             elm_access_action_cb_set(plug, msg_id,
               _access_action_release_cb, NULL);

             parent = plug;
             do
               {
                  ELM_WIDGET_DATA_GET(parent, sd);
                  if (sd->highlight_root)
                    {
                       /* change highlight root */
                       plug = parent;
                       break;
                    }
                 plug = parent;
                 parent = elm_widget_parent_get(parent);
               }
             while (parent);

             if (msg_id == ELM_ACCESS_ACTION_HIGHLIGHT_NEXT)
                _elm_access_highlight_cycle(plug, ELM_ACCESS_ACTION_HIGHLIGHT_NEXT);
             else
                _elm_access_highlight_cycle(plug, ELM_ACCESS_ACTION_HIGHLIGHT_PREV);
          }
     }
}

static void
_on_mouse_up(void *data,
             Evas *e __UNUSED__,
             Evas_Object *obj __UNUSED__,
             void *event_info __UNUSED__)
{
   evas_object_smart_callback_call(data, SIG_CLICKED, NULL);
}

static void
_elm_plug_smart_add(Evas_Object *obj)
{
   EVAS_SMART_DATA_ALLOC(obj, Elm_Plug_Smart_Data);

   _elm_plug_parent_sc->base.add(obj);
}

static Ecore_Evas*
_elm_plug_ecore_evas_get(Evas_Object *obj)
{
   Ecore_Evas *ee = NULL;
   Evas_Object *plug_img = NULL;

   ELM_PLUG_CHECK(obj) NULL;

   plug_img = elm_plug_image_object_get(obj);
   if (!plug_img) return NULL;

   ee = ecore_evas_object_ecore_evas_get(plug_img);
   return ee;
}

static Eina_Bool
_elm_plug_smart_activate(Evas_Object *obj, Elm_Activate act)
{
   Ecore_Evas *ee = NULL;
   Elm_Access_Action_Info *action_info = NULL;
   int msg_id = ELM_ACCESS_ACTION_FIRST;

   ee = _elm_plug_ecore_evas_get(obj);
   if (!ee) return EINA_FALSE;

   switch (act)
     {
       case ELM_ACTIVATE_DEFAULT:
          msg_id = ELM_ACCESS_ACTION_ACTIVATE;
          break;

       case ELM_ACTIVATE_UP:
          msg_id = ELM_ACCESS_ACTION_UP;
          break;

       case ELM_ACTIVATE_DOWN:
          msg_id = ELM_ACCESS_ACTION_DOWN;
          break;

       default:
          return EINA_FALSE;
     }

   action_info = calloc(1, sizeof(Elm_Access_Action_Info));
   action_info->action_type = msg_id;

   ecore_evas_msg_parent_send(ee, MSG_DOMAIN_CONTROL_ACCESS,
                              msg_id, action_info, sizeof(Elm_Access_Action_Info));

   free(action_info);
   return EINA_TRUE;
}

static void
_elm_plug_smart_set_user(Elm_Plug_Smart_Class *sc)
{
   ELM_WIDGET_CLASS(sc)->base.add = _elm_plug_smart_add;

   ELM_WIDGET_CLASS(sc)->theme = _elm_plug_smart_theme;
   ELM_WIDGET_CLASS(sc)->activate = _elm_plug_smart_activate;
}

EAPI const Elm_Plug_Smart_Class *
elm_plug_smart_class_get(void)
{
   static Elm_Plug_Smart_Class _sc =
     ELM_PLUG_SMART_CLASS_INIT_NAME_VERSION(ELM_PLUG_SMART_NAME);
   static const Elm_Plug_Smart_Class *class = NULL;

   if (class) return class;

   _elm_plug_smart_set(&_sc);
   class = &_sc;

   return class;
}

static Eina_Bool
_access_action_highlight_next_cb(void *data __UNUSED__,
                                 Evas_Object *obj,
                                 Elm_Access_Action_Info *action_info)
{
   Ecore_Evas *ee = NULL;

   ee = _elm_plug_ecore_evas_get(obj);
   if (!ee) return EINA_TRUE;

   ecore_evas_msg_parent_send(ee, MSG_DOMAIN_CONTROL_ACCESS,
                              action_info->action_type,
                              action_info, sizeof(Elm_Access_Action_Info));
   return EINA_TRUE;
}

static Eina_Bool
_access_action_cb(void *data __UNUSED__,
                  Evas_Object *obj,
                  Elm_Access_Action_Info *action_info)
{
   Evas_Coord_Point pos = {0, 0};
   Ecore_Evas *ee = NULL;

   ee = _elm_plug_ecore_evas_get(obj);
   if (!ee) return EINA_FALSE;

   if (action_info->action_type == ELM_ACCESS_ACTION_HIGHLIGHT)
     {
        elm_access_action_cb_set(obj, ELM_ACCESS_ACTION_HIGHLIGHT_NEXT,
                                 _access_action_highlight_next_cb, NULL);
        elm_access_action_cb_set(obj, ELM_ACCESS_ACTION_HIGHLIGHT_PREV,
                                 _access_action_highlight_next_cb, NULL);
     }
   else if (action_info->action_type == ELM_ACCESS_ACTION_READ ||
            action_info->action_type == ELM_ACCESS_ACTION_OVER ||
            action_info->action_type == ELM_ACCESS_ACTION_SCROLL ||
            action_info->action_type == ELM_ACCESS_ACTION_MOUSE)
     {
        evas_object_geometry_get(obj, &pos.x, &pos.y, NULL, NULL);
        action_info->x = action_info->x - pos.x;
        action_info->y = action_info->y - pos.y;
     }

   ecore_evas_msg_parent_send(ee, MSG_DOMAIN_CONTROL_ACCESS,
                              action_info->action_type,
                              action_info, sizeof(Elm_Access_Action_Info));

   if (action_info->action_type == ELM_ACCESS_ACTION_SCROLL)
      return EINA_FALSE;

   return EINA_TRUE;
}

EAPI Evas_Object *
elm_plug_add(Evas_Object *parent)
{
   Evas_Object *obj;
   Evas_Object *p_obj;
   Ecore_Evas *ee;

   EINA_SAFETY_ON_NULL_RETURN_VAL(parent, NULL);

   obj = elm_widget_add(_elm_plug_smart_class_new(), parent);
   if (!obj) return NULL;

   if (!elm_widget_sub_object_add(parent, obj))
     ERR("could not add %p as sub object of %p", obj, parent);

   ELM_PLUG_DATA_GET(obj, sd);
   ELM_WIDGET_DATA_GET(obj, wsd);

   ee = ecore_evas_ecore_evas_get(evas_object_evas_get(obj));
   if (ee)
     {
        p_obj = ecore_evas_extn_plug_new(ee);
        elm_widget_resize_object_set(obj, p_obj, EINA_TRUE);
     }

   evas_object_event_callback_add
     (wsd->resize_obj, EVAS_CALLBACK_MOUSE_UP, _on_mouse_up, obj);

   elm_widget_can_focus_set(obj, EINA_FALSE);
   _sizing_eval(obj);

   _elm_access_object_register(obj, ELM_WIDGET_DATA(sd)->resize_obj);
   elm_access_action_cb_set(obj, ELM_ACCESS_ACTION_HIGHLIGHT,
                            _access_action_cb, NULL);
   elm_access_action_cb_set(obj, ELM_ACCESS_ACTION_UNHIGHLIGHT,
                            _access_action_cb, NULL);
   elm_access_action_cb_set(obj, ELM_ACCESS_ACTION_READ,
                            _access_action_cb, NULL);
   elm_access_action_cb_set(obj, ELM_ACCESS_ACTION_OVER,
                            _access_action_cb, NULL);
   elm_access_action_cb_set(obj, ELM_ACCESS_ACTION_SCROLL,
                            _access_action_cb, NULL);
   elm_access_action_cb_set(obj, ELM_ACCESS_ACTION_MOUSE,
                            _access_action_cb, NULL);

   //Tizen Only: This should be removed when eo is applied.
   wsd->on_create = EINA_FALSE;

   return obj;
}

EAPI Evas_Object *
elm_plug_image_object_get(const Evas_Object *obj)
{
   ELM_PLUG_CHECK(obj) NULL;
   ELM_PLUG_DATA_GET(obj, sd);

   return ELM_WIDGET_DATA(sd)->resize_obj;
}

EAPI Eina_Bool
elm_plug_connect(Evas_Object *obj,
                 const char *svcname,
                 int svcnum,
                 Eina_Bool svcsys)
{
   Evas_Object *plug_img = NULL;

   ELM_PLUG_CHECK(obj) EINA_FALSE;

   plug_img = elm_plug_image_object_get(obj);
   if (!plug_img) return EINA_FALSE;

   if (ecore_evas_extn_plug_connect(plug_img, svcname, svcnum, svcsys))
     {
        Ecore_Evas *ee = NULL;
        ee = ecore_evas_object_ecore_evas_get(plug_img);
        if (!ee) return EINA_FALSE;

        ecore_evas_data_set(ee, PLUG_KEY, obj);
        ecore_evas_callback_delete_request_set(ee, _elm_plug_disconnected);
        ecore_evas_callback_msg_handle_set(ee, _elm_plug_ecore_evas_msg_handle);
        return EINA_TRUE;
     }
   else
     return EINA_FALSE;
}

EAPI Eina_Bool
elm_plug_msg_send(Evas_Object *obj,
                  int msg_domain,
                  int msg_id,
                  void *data, int size)
{
   Ecore_Evas *ee = NULL;
   Evas_Object *plug_img = NULL;

   ELM_PLUG_CHECK(obj) EINA_FALSE;

   plug_img = elm_plug_image_object_get(obj);
   if (!plug_img) return EINA_FALSE;

   ee = ecore_evas_object_ecore_evas_get(plug_img);
   if (!ee) return EINA_FALSE;
   ecore_evas_msg_parent_send(ee, msg_domain, msg_id, data, size);
   return EINA_TRUE;
}
