#include <Elementary.h>
#include "elm_priv.h"
#ifdef HAVE_CONFIG_H
#include "elementary_config.h"
#endif
#define EDJE_EDIT_IS_UNSTABLE_AND_I_KNOW_ABOUT_IT
#include "Edje_Edit.h"
#include "ua_interface.h"

#define NAME_BUF_SIZE  255
#define GUI_MODE_TEXT_LENGTH 20
#define RECTS_INTERSECT(x, y, w, h, xx, yy, ww, hh) \
   (((x) <= ((xx) + (ww))) && ((y) <= ((yy) + (hh))) && (((x) + (w)) >= (xx)) && (((y) + (h)) >= (yy)))
#define PRINT(...) fprintf(ua_data.fp,__VA_ARGS__)

typedef struct
{
   Evas                   *evas;
   FILE                   *fp;

   Eina_List              *edje_part_name_list;
   int                    x,y,w,h;
   int                    cnt;
   UA_Ipc_Data            *req_data;
   Evas_Object            *req_obj;
   Evas_Object            *proxy_img, *black_bg;
   Eina_Bool              gui_mode;

} ua_provider_data_s;

typedef struct
{
   const Evas_Object      *obj;
   char                    part_name[NAME_BUF_SIZE];
   const char             *image_name;
   const char             *color_class;

} edje_info_s;

typedef void      (*func_cb)();
typedef void (*_Evas_Obj_Del_Dump_Cb)(char *file_path);

static ua_provider_data_s ua_data;

static Ecore_Ipc_Server *ipc_server;
static int _ua_provider_count, _ua_provider_ipc_count;
static Ecore_Event_Handler *ecore_ipc_event_handlers[10];

static Eina_Bool _crash_mode = EINA_FALSE;
static Eina_Bool _log_mode = EINA_FALSE;

static int
file_write(char *file, func_cb func)
{
   if (ua_data.fp) fclose(ua_data.fp);
   ua_data.fp = fopen(file, "wr");
   if (!ua_data.fp) return 0;

   func();

   if (ua_data.fp) fclose(ua_data.fp);
   ua_data.fp = NULL;
   return 1;
}

/////////////////////////////////////
// Object Dump
/////////////////////////////////////
static Eina_Bool
_ua_eo_is_swallow_rect(const Evas_Object *obj)
{
   int r, g, b, a;

   evas_object_color_get(obj, &r, &g, &b, &a);
   if (!r && !g && !b && !a
       && evas_object_pointer_mode_get(obj) == EVAS_OBJECT_POINTER_MODE_NOGRAB
       && evas_object_pass_events_get(obj))
     return EINA_TRUE;

   return EINA_FALSE;
}

static Eina_Bool
_ua_eo_visible_get(Evas_Object *obj)
{
   int r, g, b, a;
   Evas_Coord x, y, w, h;
   Evas_Object *clip;
   int vis = 0;

   if ((_ua_eo_is_swallow_rect(obj)) && ((ua_data.req_data)->swallow_show)) return EINA_TRUE;

   evas_object_color_get(obj, &r, &g, &b, &a);
   evas_object_geometry_get(obj, &x, &y, &w, &h);

   if (evas_object_visible_get(obj)) vis = 1;
   clip = evas_object_clip_get(obj);
   if (clip) evas_object_color_get(obj, &r, &g, &b, &a);
   if (clip && !evas_object_visible_get(clip)) vis = 0;

   if (clip && a == 0) vis = 0;

   return vis;
}

static Eina_Bool
_ua_eo_type_match(const Evas_Object *obj, const char *type)
{
   int ret;
   ret = strcmp(evas_object_type_get(obj), type);

   if (!ret) return EINA_TRUE;
   return EINA_FALSE;
}

static void
_extract_edje_file_name(const char *full_path)
{
   char name[NAME_BUF_SIZE];
   int i, cnt = 0;

   if (!full_path) return;

   for (i = 0; i < (int)strlen(full_path); i++)
     {
        if (full_path[i] == '/')
          {
             cnt = 0;
             continue;
          }
        name[cnt++] = full_path[i];
     }
   name[cnt] = '\0';
   PRINT("[%s]", name);
}

static void
_edje_file_info_save(const Evas_Object *obj)
{
   Ecore_Evas *ee;
   Evas *evas;
   Evas_Object *ed;
   const Evas_Object *pobj;
   Eina_List *parts, *l;
   edje_info_s *edje_info;
   const char *file, *group, *pname, *ret;
   double val;

   if(!_ua_eo_type_match(obj, "edje")) return;

   edje_object_file_get(obj, &file, &group);

   ee = ecore_evas_buffer_new(ua_data.w, ua_data.h);
   evas = ecore_evas_get(ee);

   ed = edje_edit_object_add(evas);
   if (edje_object_file_set(ed, file, group))
     {
        parts = edje_edit_parts_list_get(ed);
        EINA_LIST_FOREACH(parts, l, pname)
          {
             if ((pobj = edje_object_part_object_get(obj, pname)))
               {
                  edje_info = malloc(sizeof(edje_info_s));
                  edje_info->obj = pobj;
                  strcpy(edje_info->part_name, pname);
                  ret = edje_object_part_state_get(obj, edje_info->part_name, &val);

                  if (ret)
                    {
                       edje_info->color_class = edje_edit_state_color_class_get(ed, edje_info->part_name , ret, val);
                       edje_info->image_name = edje_edit_state_image_get(ed, edje_info->part_name , ret, val);
                    }
                  ua_data.edje_part_name_list = eina_list_append(ua_data.edje_part_name_list, edje_info);
               }
          }
        edje_edit_string_list_free(parts);
     }
   evas_object_del(ed);
   ecore_evas_free(ee);
}

static void
_obj_tree_items(Evas_Object *obj, int lvl)
{
   Eina_List *children, *l;
   Evas_Object *child, *smart_parent_obj;
   Evas_Coord x, y, w, h;
   Eina_Bool is_clip = EINA_FALSE;
   Eina_Bool visible_only = EINA_FALSE, gui_mode = EINA_FALSE;
   int i, r, g, b, a;
   const char *ret;
   double val;
   const char *file, *key, *group, *text = NULL, *tb_style = NULL;
   char **ts = NULL;
   edje_info_s *edje_info;

   visible_only = (ua_data.req_data)->visible_only;
   gui_mode = (ua_data.req_data)->gui_mode;

   if ((_crash_mode == EINA_TRUE) || (_log_mode == EINA_TRUE))
     {
        visible_only = EINA_FALSE;
        gui_mode = EINA_FALSE;
     }
   else
     {
        visible_only = (ua_data.req_data)->visible_only;
        gui_mode = (ua_data.req_data)->gui_mode;
     }

   // visible check
   if (visible_only && !_ua_eo_visible_get(obj)) return;

   // viewport check
   evas_object_geometry_get(obj, &x, &y, &w, &h);

   if (visible_only && !RECTS_INTERSECT(x,y,w,h, ua_data.x, ua_data.y, ua_data.w, ua_data.h)) return;

   // clipper check
   if (visible_only && evas_object_clipees_get(obj)) is_clip = EINA_TRUE;
   if (is_clip) goto next;

   if (!gui_mode)
     {
        if(elm_widget_is(obj))
          {
             if ((_crash_mode == EINA_FALSE) && (_log_mode == EINA_FALSE))
               PRINT("\n\033[36m");
             else
               PRINT("\n");
          }

        if ((_crash_mode == EINA_FALSE) && (_log_mode == EINA_FALSE))
          {
             if (_ua_eo_type_match(obj, "edje")) PRINT("\033[33;1m");
          }
     }

   if (gui_mode) PRINT("%4d| ", lvl);

   if (!visible_only && evas_object_clipees_get(obj))
      PRINT("%3d: %-10p [ min  min  max  max]", ++ua_data.cnt, obj);
   else
   PRINT("%3d: %-10p [%4d %4d %4d %4d]", ++ua_data.cnt, obj, x, y, w, h);

   for (i = 0; i < lvl * 2; i++)
     PRINT( " ");

   if (_ua_eo_visible_get(obj)) PRINT("+ ");
   else PRINT("- ");

   // evas object type check
   if (_ua_eo_is_swallow_rect(obj)) PRINT("[%d] swallow ", lvl);
   else if (!visible_only && evas_object_clipees_get(obj))
      PRINT("[%d] clipper %s", lvl, evas_object_type_get(obj));
   else if (_ua_eo_type_match(obj, "rectangle"))
     {
        evas_object_color_get(obj, &r, &g, &b, &a);
        PRINT("[%d] rect [%d %d %d %d] ", lvl, r, g, b, a);
     }
   else PRINT("[%d] %s ", lvl, evas_object_type_get(obj));

   smart_parent_obj = evas_object_smart_parent_get(obj);

   // image info save
   if (!strcmp(evas_object_type_get(obj), "elm_icon") ||
       !strcmp(evas_object_type_get(obj), "elm_image"))
     {
        elm_image_file_get(obj, &file, &key);
        evas_object_data_set(obj, "image_name", file);
     }

   // image name check
   if (smart_parent_obj && _ua_eo_type_match(obj, "image")
       && (_ua_eo_type_match(smart_parent_obj, "elm_icon")
           || _ua_eo_type_match(smart_parent_obj, "elm_image")))
     {
        if ((ret = evas_object_data_get(smart_parent_obj, "image_name")))
          {
             _extract_edje_file_name(ret);
             evas_object_data_del(smart_parent_obj, "edje_image_name");
          }
     }

   // edje info save
   if(_ua_eo_type_match(obj, "edje"))
     {
        edje_object_file_get(obj, &file, &group);
        if (group) PRINT("[%s] ", group);

        _extract_edje_file_name(file);
        _edje_file_info_save(obj);
     }

   // edje info check
   if( !is_clip && smart_parent_obj
       && !elm_widget_is(obj) && _ua_eo_type_match(smart_parent_obj, "edje"))
     {
        EINA_LIST_FOREACH(ua_data.edje_part_name_list, l, edje_info)
          {
             if(edje_info->obj == obj)
               {
                  if (edje_info->color_class)
                    PRINT("[%s] ", edje_info->color_class);

                  ret = edje_object_part_state_get(evas_object_smart_parent_get(obj), edje_info->part_name, &val);
                  PRINT("[%s : \"%s\" %2.1f]  ", edje_info->part_name , ret, val);

                  if (edje_info->image_name)
                    PRINT("[%s] ", edje_info->image_name);
                  break;
               }
          }
     }

   if (!strcmp(evas_object_type_get(obj), "text"))
     text = eina_stringshare_add(evas_object_text_text_get(obj));
   else if (!strcmp(evas_object_type_get(obj), "textblock"))
     {
        text = evas_object_textblock_text_markup_get(obj);
        tb_style = evas_textblock_style_get(evas_object_textblock_style_get(obj));
        if (tb_style)
          {
             ts = eina_str_split(tb_style, "'", 0);
             ts = eina_str_split(ts[1], " ", 0);
          }
     }

   if (text && strlen(text) > 0)
     {
        if (gui_mode)
          PRINT("[%s]", text);
        else
          PRINT("\n\t\t\t\t\t     ");
        if ((_crash_mode == EINA_FALSE) && (_log_mode == EINA_FALSE))
          {
             if (!gui_mode) PRINT("\033[32;1m");
          }

        if (!gui_mode)
          {
             for (i = 0; i < lvl * 2; i++)
               PRINT(" ");

             PRINT("==> [%s] ", text);
             if (tb_style)
               {
                  if ((_crash_mode == EINA_FALSE) && (_log_mode == EINA_FALSE))
                    {
                       if (!gui_mode) PRINT("\033[36m'");
                    }
                  i = 0;
                  while(1)
                    {
                       if (ts[i] && !strstr(ts[i], "font_source") )  PRINT("%s ", ts[i]);
                       else if (!ts[i]) break;
                       i++;
                    }
                  PRINT("'\n");
               }
          }
     }

   if (!gui_mode)
     {
        if ((_crash_mode == EINA_FALSE) && (_log_mode == EINA_FALSE))
          PRINT("\n\033[37;1m");
        else
          PRINT("\n");
     }
   else PRINT("\n");
next:
   children = evas_object_smart_members_get(obj);
   EINA_LIST_FREE(children, child)
      _obj_tree_items(child, lvl + 1 );
}


static void
_object_tree( )
{
   Eina_List *objs;
   Evas_Object *obj;
   int x, y, w, h;

   objs = evas_objects_in_rectangle_get(ua_data.evas, SHRT_MIN, SHRT_MIN, USHRT_MAX, USHRT_MAX, EINA_TRUE, EINA_TRUE);

   ecore_evas_geometry_get(ecore_evas_ecore_evas_get(ua_data.evas), &x, &y, &w, &h);

   EINA_LIST_FREE(objs, obj)
      _obj_tree_items(obj, 0 );

}

/////////////////////////////////////
// Widget Dump
/////////////////////////////////////
static void
_sub_widget_tree(const Evas_Object *obj, int lvl)
{
   Evas_Coord x, y, w, h;
   int i;

   evas_object_geometry_get(obj, &x, &y, &w, &h);

   if (elm_widget_is(obj))
     PRINT("%-10p [%4d %4d %4d %4d] ",
           obj, x, y, w, h);

   if (elm_widget_is(obj))
     {
        for (i = 0; i < lvl * 3; i++)
          PRINT(" ");
     }

   if (elm_widget_is(obj))
     {
        const Eina_List *l, *list;

        if (elm_object_focus_get(obj))
          {
             if ((_crash_mode == EINA_FALSE) && (_log_mode == EINA_FALSE))
               PRINT("\033[33;1m [f] ");
             else
               PRINT(" [f] ");
          }
        else
          PRINT("    ");

        if (evas_object_visible_get(obj))
          PRINT("+ %s \n", evas_object_type_get(obj));
        else
          PRINT("- %s \n", evas_object_type_get(obj));

        list = elm_widget_sub_object_list_get(obj);

        if ((_crash_mode == EINA_FALSE) && (_log_mode == EINA_FALSE))
          {
             PRINT("\033[37;1m");
          }

        EINA_LIST_FOREACH(list, l, obj)
          {
             _sub_widget_tree(obj, lvl + 1);
          }
     }
}

static void
_widget_tree()
{
   Evas_Object *top = evas_object_top_get(ua_data.evas);
   _sub_widget_tree(top, 0);
}

static void
_ua_tree_process()
{
   UA_IPC_Msg_Type msg = UA_MSG_NONE;
   Eina_List *ecore_evas_list;
   Ecore_Evas *ee;
   Evas *evas;

   msg = (ua_data.req_data)->msg;

   ecore_evas_list = ecore_evas_ecore_evas_list_get();

   ua_data.cnt = 0;
   ua_data.req_obj = NULL;

   EINA_LIST_FREE(ecore_evas_list, ee)
     {
        evas = ecore_evas_get(ee);
        ua_data.evas = evas;

        ecore_x_window_geometry_get(ecore_x_window_root_first_get(),
                                    &ua_data.x, &ua_data.y, &ua_data.w, &ua_data.h);

        if ((msg == UA_MSG_OBJECT_VIEWER_REQ) ||
            (msg == UA_MSG_OBJECT_TREE_DUMP_REQ))
          {
             file_write(FILE_DUMP_PATH, _object_tree);
          }
        else if (msg == UA_MSG_WIDGET_TREE_DUMP_REQ)
          {
             file_write(FILE_DUMP_PATH, _widget_tree);
          }

     }
   if (ua_data.edje_part_name_list)
     eina_list_free(ua_data.edje_part_name_list);
   ua_data.edje_part_name_list = NULL;
}

/////////////////////////////////////
// Object Info
/////////////////////////////////////
static int
_find_obj_tree_item(Evas_Object *obj, char *obj_addr)
{
   Eina_List *children;
   Evas_Object *child;
   Eina_Bool visible_only;
   Evas_Coord x, y, w, h;
   char buf[20];

   if (ua_data.req_obj) return 1;
   visible_only = (ua_data.req_data)->visible_only;
   if (visible_only && !_ua_eo_visible_get(obj)) return 0;

   // viewport check
   evas_object_geometry_get(obj, &x, &y, &w, &h);

   if (visible_only && !RECTS_INTERSECT(x,y,w,h, ua_data.x, ua_data.y, ua_data.w, ua_data.h)) return 0;

   // clipper check
   if (visible_only && evas_object_clipees_get(obj)) return 0;

   sprintf(buf, "%p", obj);
   if (!strcmp(buf, obj_addr))
     {
        ua_data.req_obj = obj;
        return 1;
     }
   children = evas_object_smart_members_get(obj);
   EINA_LIST_FREE(children, child)
      _find_obj_tree_item(child, obj_addr);

   return (ua_data.req_obj ? 1 : 0);
}

static Evas_Object *
_find_obj(char *obj_addr)
{
   Eina_List *objs, *l;
   Evas_Object *obj;
   int x, y, w, h;

   objs = evas_objects_in_rectangle_get(ua_data.evas, SHRT_MIN, SHRT_MIN, USHRT_MAX, USHRT_MAX, EINA_TRUE, EINA_TRUE);

   ecore_evas_geometry_get(ecore_evas_ecore_evas_get(ua_data.evas), &x, &y, &w, &h);
   EINA_LIST_FOREACH(objs, l, obj)
     {
        if (_find_obj_tree_item(obj, obj_addr)) break;
     }

   eina_list_free(objs);
   return ua_data.req_obj;
}

static void _obj_del_cb(void *data __UNUSED__, Evas *evas __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   ua_data.req_obj = NULL;
}

static void
_object_info()
{
   Evas_Object *obj;
   Evas_Coord x, y, w, h;
   int r, g, b, a;
   double dx, dy;
   Evas_Aspect_Control aspect;
   const char *text = NULL, *tb_style = NULL;

   obj = ua_data.req_obj;

   if (!obj)
     {
        PRINT("Evas Object is removed. \n");
        return;
     }

   evas_object_event_callback_add(obj, EVAS_CALLBACK_DEL, _obj_del_cb, NULL);

   if(_ua_eo_type_match(obj, "edje"))
     {
        const char *file, *group;
        PRINT("## EDC ## \n");

        edje_object_file_get(obj, &file, &group);
        if (file && group)
          {
             PRINT("%s %s\n", file, group);
          }
     }
   PRINT("## end\n\n");

   PRINT("## Evas ## \n");
     {
        Evas *evas;
        Ecore_Evas *ecore_evas;

        evas = evas_object_evas_get(obj);
        ecore_evas = ecore_evas_ecore_evas_get(evas);

        PRINT("Evas address \t\t\t\t %p \n", evas);
        PRINT("evas_focus_state_get \t\t\t %d \n", evas_focus_state_get(evas));

        ecore_evas_geometry_get(ecore_evas, &x, &y, &w, &h);
        PRINT("ecore_evas_geometry_get [x y w h] \t\t [%d %d %d %d] \n", x, y, w,h);
        evas_output_size_get(evas, &w, &h);
        PRINT("evas_output_size_get [w h] \t\t\t [%d %d] \n", w, h);
        evas_output_viewport_get(evas, &x, &y, &w, &h);
        PRINT("evas_output_viewport_get [x y w h] \t\t [%d %d %d %d] \n", x, y, w, h);
     }

   PRINT("\n## Evas Object ## \n");
     {
        PRINT("evas_object address \t\t\t %p \n", obj);
        PRINT("evas_object_type_get \t\t\t %s \n", evas_object_type_get(obj));

        PRINT("evas_object_visible_get \t\t\t %d \n", evas_object_visible_get(obj));

        evas_object_geometry_get(obj, &x, &y, &w, &h);
        PRINT( "evas_object_geometry_get [x y w h]\t\t [%4d %4d %4d %4d]\n", x, y, w, h);

        evas_object_color_get(obj, &r, &g, &b, &a);
        PRINT( "evas_object_color_get [r g b a]\t\t [%4d %4d %4d %4d] \n", r, g, b, a);

        evas_object_size_hint_min_get(obj, &w, &h);
        PRINT( "evas_object_size_hint_min_get [w h]\t\t [%4d %4d] \n", w, h);

        evas_object_size_hint_max_get(obj, &w, &h);
        PRINT( "evas_object_size_hint_max_get [w h]\t\t [%4d %4d] \n", w, h);

        evas_object_size_hint_align_get(obj, &dx, &dy);
        PRINT( "evas_object_size_hint_align_get [x y]\t [%3.1f %3.1f] \n", dx, dy);

        evas_object_size_hint_weight_get(obj, &dx, &dy);
        PRINT( "evas_object_size_hint_weight_get [x y]\t [%3.1f %3.1f] \n", dx, dy);

        evas_object_size_hint_aspect_get(obj, &aspect, &w, &h);
        PRINT("evas_object_size_hint_aspect_get [w h] \t [%d %d] aspect : ", w, h);
        if (aspect == EVAS_ASPECT_CONTROL_NEITHER) PRINT("EVAS_ASPECT_CONTROL_NEITHER \n");
        else if (aspect == EVAS_ASPECT_CONTROL_HORIZONTAL) PRINT("EVAS_ASPECT_CONTROL_HORIZONTAL \n");
        else if (aspect == EVAS_ASPECT_CONTROL_VERTICAL) PRINT("EVAS_ASPECT_CONTROL_HORIZONTAL \n");
        else PRINT("EVAS_ASPECT_CONTROL_BOTH \n");

        evas_object_size_hint_padding_get(obj, &x, &y, &w, &h);
        PRINT("evas_object_size_hint_padding_get [l r t b]  [%d %d %d %d] \n", x, y, w, h);

        PRINT("evas_object_focus_get \t\t\t %d \n", evas_object_focus_get(obj));
        PRINT("evas_object_layer_get \t\t\t %d \n", evas_object_layer_get(obj));

        Evas_Render_Op op = evas_object_render_op_get(obj);
        PRINT("evas_object_render_op_get \t\t\t ");
        if (op == EVAS_RENDER_BLEND) PRINT("EVAS_RENDER_BLEND \n");
        else if (op == EVAS_RENDER_BLEND_REL) PRINT("EVAS_RENDER_BLEND_REL \n");
        else if (op == EVAS_RENDER_COPY) PRINT("EVAS_RENDER_COPY \n");
        else if (op == EVAS_RENDER_COPY_REL) PRINT("EVAS_RENDER_COPY_REL \n");
        else if (op == EVAS_RENDER_ADD) PRINT("EVAS_RENDER_ADD \n");
        else if (op == EVAS_RENDER_ADD_REL) PRINT("EVAS_RENDER_ADD_REL \n");
        else if (op == EVAS_RENDER_TINT) PRINT("EVAS_RENDER_TINT \n");
        else if (op == EVAS_RENDER_SUB_REL) PRINT("EVAS_RENDER_SUB_REL \n");
        else if (op == EVAS_RENDER_TINT_REL) PRINT("EVAS_RENDER_TINT_REL \n");
        else if (op == EVAS_RENDER_MASK) PRINT("EVAS_RENDER_MASK \n");
        else if (op == EVAS_RENDER_MUL) PRINT("EVAS_RENDER_MUL \n");

        PRINT("evas_object_*_event_get \t\t\t pass [%d] repeat [%d] propagate [%d] freeze [%d] \n",
              evas_object_pass_events_get(obj), evas_object_repeat_events_get(obj), evas_object_propagate_events_get(obj), evas_object_freeze_events_get(obj));

        PRINT("evas_object_map_enable_get \t\t\t %d \n", evas_object_map_enable_get(obj));
        PRINT("evas_object_scale_get \t\t\t %3.1f \n", evas_object_scale_get(obj));

        // text info
        if (!strcmp(evas_object_type_get(obj), "text"))
          text = eina_stringshare_add(evas_object_text_text_get(obj));
        else if (!strcmp(evas_object_type_get(obj), "textblock"))
          {
             text = evas_object_textblock_text_markup_get(obj);
             tb_style = evas_textblock_style_get(evas_object_textblock_style_get(obj));
          }
        if (text) PRINT("evas_object_text_text_get \t\t\t %s \n",text);
        if (tb_style) PRINT("evas_textblock_style_get \n[%s]\n", tb_style);

        // image info
        if ((!strcmp(evas_object_type_get(obj), "image")))
          {
             const char *file = NULL, *key = NULL;
             evas_object_image_file_get(obj, &file, &key);
             PRINT("\nevas_object_image_file_get \t\t\t [%s]  [key : %s] \n", file, key);

             evas_object_image_border_get(obj, &x, &y, &w, &h);
             PRINT("evas_object_image_border_get [l r t b] \t [%d %d %d %d] \n", x, y, w, h);

             evas_object_image_border_get(obj, &x, &y, &w, &h);
             PRINT("evas_object_image_border_get [l r t b] \t [%d %d %d %d] \n", x, y, w, h);
             PRINT("evas_object_image_filled_get \t\t %d \n", evas_object_image_filled_get(obj));
             PRINT("evas_object_image_border_scale_get \t\t %2.1f \n", evas_object_image_border_scale_get(obj));

             evas_object_image_fill_get(obj, &x, &y, &w, &h);
             PRINT("evas_object_image_fill_get [x y w h] \t [%d %d %d %d] \n", x, y, w, h);

             evas_object_image_size_get(obj, &w, &h);
             PRINT("evas_object_image_size_get [w h] \t\t [%d %d] \n", w, h);
             PRINT("evas_object_image_alpha_get \t\t %d \n", evas_object_image_alpha_get(obj));
             PRINT("evas_object_image_smooth_scale_get \t\t %d \n", evas_object_image_smooth_scale_get(obj));

             evas_object_image_load_size_get(obj, &w, &h);
             PRINT("evas_object_image_load_size_get [w h] \t [%d %d] \n", w, h);

             evas_object_image_load_region_get(obj, &x, &y, &w, &h);
             PRINT("evas_object_image_load_region_get [x y w h]  [%d %d %d %d] \n", x, y, w, h);

             PRINT("evas_object_image_colorspace_get \t\t %d \n", evas_object_image_colorspace_get(obj));

             PRINT("evas_object_image_native_surface_get \t %p \n", evas_object_image_native_surface_get(obj));
          }
     }
   PRINT("## end\n\n");

   PRINT("## Elementary ## \n");

   char *buf = NULL;
   buf = elm_widget_description_get(obj);
   if (buf)
     {
        PRINT("%s\n", buf);
        free(buf);
     }
   PRINT("## end\n\n");
}

/////////////////////////////////////
// XY Object Get
/////////////////////////////////////
static Evas_Object *xy_obj;

static void
_ua_xy_object_tree_find(Evas_Object *obj, int tx, int ty, char *str)
{
   Eina_List *children;
   Evas_Object *child;
   Eina_Bool visible_only;
   Evas_Coord x, y, w, h;
   int r,g,b,a;

   visible_only = 1;

   if (visible_only && !_ua_eo_visible_get(obj)) return;

   if (!strcmp(evas_object_type_get(obj), "elm_win")) return;
   // viewport check
   evas_object_geometry_get(obj, &x, &y, &w, &h);

   if (visible_only && !RECTS_INTERSECT(x,y,w,h, ua_data.x, ua_data.y, ua_data.w, ua_data.h)) return;

   evas_object_color_get(obj, &r, &g, &b, &a);

   if (!r && !g && !b && !a) return;

   // clipper check
   if (visible_only && evas_object_clipees_get(obj)) return ;

   //   if (RECTS_INTERSECT(x,y,w,h, tx, ty, 1, 1) && (elm_widget_is(obj) || (!strcmp(evas_object_type_get(obj), "edje"))))
   if (RECTS_INTERSECT(x,y,w,h, tx, ty, 1, 1) )
     {
        xy_obj = obj;
     }

   children = evas_object_smart_members_get(obj);
   EINA_LIST_FREE(children, child)
      _ua_xy_object_tree_find(child, tx, ty, str);

   return;
}

static Evas_Object *
_ua_xy_object_get(int tx, int ty, char *str)
{
   Eina_List *objs, *l;
   Evas_Object *obj;
   int x, y, w, h;

   xy_obj = NULL;

   objs = evas_objects_in_rectangle_get(ua_data.evas, SHRT_MIN, SHRT_MIN, USHRT_MAX, USHRT_MAX, EINA_TRUE, EINA_TRUE);

   ecore_evas_geometry_get(ecore_evas_ecore_evas_get(ua_data.evas), &x, &y, &w, &h);
   EINA_LIST_FOREACH(objs, l, obj)
     {
        _ua_xy_object_tree_find(obj, tx, ty, str);
     }

   eina_list_free(objs);

   return xy_obj;
}

static void
_object_del_info()
{
   char file_path[50], buf[1024];
   FILE *fp = NULL;

   PRINT("## end\n\n");
   PRINT("## Evas ## \n");

   PRINT("DEL \n");

   sprintf(file_path, "/tmp/.evas_object_del_history_%i", getpid());
   if ((fp = fopen(file_path, "r")) != NULL)
     {
        while(fgets(buf, 1024, fp) != NULL)
          {
             PRINT("%s", buf);
          }
     }

   if (fp)
     fclose(fp);

   PRINT("## end\n\n");
}

static void
_ua_object_info_process()
{
   Evas *evas;
   Ecore_Evas *ee;
   Eina_List *ecore_evas_list, *l;
   char buf[20];

   if (ua_data.req_obj)
     sprintf(buf, "%p", ua_data.req_obj);

   if ((!ua_data.req_obj)
       || (ua_data.req_obj && (strcmp(buf, (ua_data.req_data)->obj_addr))))
     {
        ecore_evas_list = ecore_evas_ecore_evas_list_get();

        ua_data.cnt = 0;
        ua_data.req_obj = NULL;

        EINA_LIST_FOREACH(ecore_evas_list, l, ee)
          {
             evas = ecore_evas_get(ee);
             ua_data.evas = evas;

             ua_data.req_obj = _find_obj((ua_data.req_data)->obj_addr);
          }
        eina_list_free(ecore_evas_list);

     }

   if (!ua_data.req_obj)
     {

        file_write(FILE_DUMP_PATH, _object_del_info);

        return;
     }

   if ((ua_data.req_data)->msg == UA_MSG_OBJECT_IMAGE_DUMP_REQ)
     {
        Evas_Coord x, y, w, h;
        evas = evas_object_evas_get(ua_data.req_obj);
        ua_data.black_bg = evas_object_rectangle_add(evas);
        evas_object_color_set(ua_data.black_bg, 30, 30, 30, 255);
        evas_object_move(ua_data.black_bg, ua_data.x, ua_data.y);
        evas_object_resize(ua_data.black_bg, ua_data.w, ua_data.h);
        evas_object_show(ua_data.black_bg);

        ua_data.proxy_img= evas_object_image_filled_add(evas);
        evas_object_image_source_set(ua_data.proxy_img, ua_data.req_obj );
        evas_object_geometry_get(ua_data.req_obj, &x, &y, &w, &h);
        evas_object_size_hint_min_set(ua_data.proxy_img, w, h);

        evas_object_move(ua_data.proxy_img, x, y);
        evas_object_resize(ua_data.proxy_img, w, h);
        evas_object_show(ua_data.proxy_img);
        evas_render(evas);
     }
   else
     file_write(FILE_DUMP_PATH, _object_info);
}

static void
_ua_memory_info_process()
{
   PRINT("%s \n\n", (char*)_eina_chained_mempool_dump());
}

/////////////////////////////////////
// IPC
/////////////////////////////////////
static Eina_Bool
_ipc_client_add_cb(void *data, int type __UNUSED__, void *event)
{
   Ecore_Ipc_Server  *server;
   Ecore_Ipc_Event_Client_Data *e;

   server = (Ecore_Ipc_Server *) data;
   e = (Ecore_Ipc_Event_Client_Data *) event;

   if (server != ecore_ipc_client_server_get(e->client))
     return EINA_TRUE;

   printf("UI Analyzer is connected @ %p \n", server);
   return EINA_TRUE;
}

static Eina_Bool
_ipc_client_data_recv_cb(void *data __UNUSED__, int type __UNUSED__, void *event)
{
   Ecore_Ipc_Event_Client_Data *e = event;
   UA_Ipc_Data rep_data;
   UA_IPC_Msg_Type msg = UA_MSG_NONE;
   Evas_Object *obj = NULL;
   Evas *evas = NULL;
   Elm_Widget_Input_History_Dump_Cb input_history_dump = NULL;
   char buf[PATH_MAX];
   float speed = 1.0;
   static int image_dump = 0;

   if (ipc_server != ecore_ipc_client_server_get(e->client))
     return EINA_TRUE;

   ua_data.req_data = (UA_Ipc_Data*)e->data;
   msg = (ua_data.req_data)->msg;

   if (ua_data.proxy_img)
     {
        evas_object_del(ua_data.proxy_img);
        ua_data.proxy_img = NULL;
     }

   if (ua_data.black_bg)
     {
        evas_object_del(ua_data.black_bg);
        ua_data.black_bg = NULL;
     }

   switch (msg)
     {
      case UA_MSG_OBJECT_VIEWER_REQ:
         _ua_tree_process();
         rep_data.msg = UA_MSG_OBJECT_VIEWER_REP;
         break;

      case UA_MSG_WIDGET_TREE_DUMP_REQ:
         _ua_tree_process();
         rep_data.msg = UA_MSG_WIDGET_TREE_DUMP_REP;
         break;

      case UA_MSG_OBJECT_TREE_DUMP_REQ:
         _ua_tree_process();
         rep_data.msg = UA_MSG_OBJECT_TREE_DUMP_REP;
         break;

      case UA_MSG_OBJECT_IMAGE_DUMP_REQ:
         image_dump = !image_dump;
         if (image_dump)
           {
              _ua_object_info_process();
           }
         rep_data.msg = UA_MSG_OBJECT_IMAGE_DUMP_REP;
         break;

      case UA_MSG_OBJECT_INFO_REQ:
         _ua_object_info_process();
         rep_data.msg = UA_MSG_OBJECT_INFO_REP;
         break;

      case UA_MSG_XY_OBJECT_REQ:
         obj = _ua_xy_object_get((ua_data.req_data)->obj_x, (ua_data.req_data)->obj_y,
                                 (ua_data.req_data)->obj_addr);
         if (obj) sprintf(rep_data.obj_addr, "%p", obj);
         rep_data.msg = UA_MSG_XY_OBJECT_REP;
         break;

      case UA_MSG_EVAS_DUMP_REQ:
         rep_data.msg = UA_MSG_EVAS_DUMP_REP;
         break;

      case UA_MSG_EVENT_INFO_REQ:

         input_history_dump = elm_widget_input_history_dump_callback_get();
         if (input_history_dump)
           {
              input_history_dump(NULL);
           }

         rep_data.msg = UA_MSG_EVENT_INFO_REP;
         break;

      case UA_MSG_MEMORY_INFO_REQ:
         file_write(FILE_DUMP_PATH, _ua_memory_info_process);
         rep_data.msg = UA_MSG_MEMORY_INFO_REP;
         break;

      case UA_MSG_TIMER_SPEED_CHANGE_REQ:
         speed = (ua_data.req_data)->timer_speed;
         sprintf(buf,"%3.1lf", speed);
         setenv("ECORE_TIME_SCALE", buf, 1);
         rep_data.msg = UA_MSG_TIMER_SPEED_CHANGE_REP;
         break;

      case UA_MSG_ECORE_RESOURCE_DUMP_REQ:
         rep_data.msg = UA_MSG_ECORE_RESOURCE_DUMP_REP;
         break;

      default:
         break;
     }

   if (msg) ecore_ipc_client_send(e->client, 0, 0, 0, 0, 0, &rep_data, sizeof(rep_data));

   return 1;
}

static int
_ua_provider_ipc_del()
{
   int i;
   if (_ua_provider_ipc_count)
     {
        ecore_ipc_shutdown();

        for(i = 0; i < 10; i++)
          {
             if (ecore_ipc_event_handlers[i])
               ecore_event_handler_del(ecore_ipc_event_handlers[i]);
          }
        if (ipc_server)
          {
             ecore_ipc_server_del(ipc_server);
             ipc_server = NULL;
          }
     }
   _ua_provider_ipc_count = 0;
   return 1;
}

static Eina_Bool
_ipc_client_del_cb(void *data __UNUSED__, int type __UNUSED__, void *event)
{
   Ecore_Ipc_Event_Client_Data *e = event;
   if (ipc_server != ecore_ipc_client_server_get(e->client))
     return EINA_TRUE;

   _ua_provider_ipc_del();
   return ECORE_CALLBACK_PASS_ON;
}

static int
_ua_provider_ipc_add()
{
   if (_ua_provider_ipc_count) return 0;

   if (ecore_ipc_init() < 1)
     return 0;

   _ua_provider_ipc_count++;

   ipc_server = ecore_ipc_server_add(ECORE_IPC_LOCAL_SYSTEM, "ua_provider", 516, NULL);
   if (ipc_server) printf("Server is listening on \n");
   else
     {
        printf("Connection Fail ! \n");
        return 0;
     }

   ecore_ipc_event_handlers[0] = ecore_event_handler_add(ECORE_IPC_EVENT_CLIENT_ADD, _ipc_client_add_cb, ipc_server);
   ecore_ipc_event_handlers[1] = ecore_event_handler_add(ECORE_IPC_EVENT_CLIENT_DATA, _ipc_client_data_recv_cb, ipc_server);
   ecore_ipc_event_handlers[2] = ecore_event_handler_add(ECORE_IPC_EVENT_CLIENT_DEL, _ipc_client_del_cb, ipc_server);

   return 1;
}

static int
_ua_provider_init()
{
   int ret;
   if (_ua_provider_count) return 0;

   if (!(ret = _ua_provider_ipc_add()))
     {
        printf("_ua_provider_ipc_add ret = %d \n", ret);
        _ua_provider_ipc_del();
     }

   _ua_provider_count++;
   return _ua_provider_count;
}

static int
_ua_provider_shutdown()
{
   if (!_ua_provider_count) return _ua_provider_count;

   _ua_provider_ipc_del();
   _ua_provider_count = 0;

   return _ua_provider_count;
}

EAPI int
elm_modapi_init(void *m __UNUSED__)
{
   _ua_provider_init();

   return 1;
}

EAPI int
elm_modapi_shutdown(void *m __UNUSED__)
{
   _ua_provider_shutdown();

   return 1;
}
