#include "utility.h"

#define PRINT(...) fprintf(util_mgr.fp,__VA_ARGS__)

#define RESET "\033[0m"
//extern char *_ecore_timer_dump(void);
//extern char *_ecore_animator_dump(void);

static Eina_Bool
evas_object_is_visible_get(Evas_Object *obj)
{
   int r, g, b, a;
   Evas_Coord x, y, w, h;
   Evas_Object *clip;
   int vis = 0;

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
evas_object_type_match(const Evas_Object *obj, const char *type)
{
   int ret;
   ret = strcmp(evas_object_type_get(obj), type);

   if (!ret) return EINA_TRUE;
   return EINA_FALSE;
}

static Eina_Bool
evas_object_is_swallow_rect(const Evas_Object *obj)
{
   int r, g, b, a;

   evas_object_color_get(obj, &r, &g, &b, &a);
   if (!r && !g && !b && !a
       && evas_object_pointer_mode_get(obj) == EVAS_OBJECT_POINTER_MODE_NOGRAB
       && evas_object_pass_events_get(obj))
     return EINA_TRUE;

   return EINA_FALSE;
}

static int
file_write(char *file, func_cb func)
{
   if (util_mgr.fp) fclose(util_mgr.fp);
   util_mgr.fp = fopen(file, "wr");
   if (!util_mgr.fp) return 0;

   func();

   if (util_mgr.fp) fclose(util_mgr.fp);
   util_mgr.fp = NULL;
   return 1;
}

static void
_extract_edje_file_name(const char *full_path)
{
   char name[255];
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
   Edje_Info *edje_info;
   const char *file, *group, *pname, *ret;
   double val;

   if(!evas_object_type_match(obj, "edje")) return;

   edje_object_file_get(obj, &file, &group);

   ee = ecore_evas_buffer_new(util_mgr.w, util_mgr.h);
   evas = ecore_evas_get(ee);

   ed = edje_edit_object_add(evas);
   if (edje_object_file_set(ed, file, group))
     {
        parts = edje_edit_parts_list_get(ed);
        EINA_LIST_FOREACH(parts, l, pname)
          {
             if ((pobj = edje_object_part_object_get(obj, pname)))
               {
                  edje_info = malloc(sizeof(Edje_Info));
                  edje_info->obj = pobj;
                  strcpy(edje_info->part_name, pname);
                  ret = edje_object_part_state_get(obj, edje_info->part_name, &val);

                  if (ret)
                    {
                       edje_info->color_class = edje_edit_state_color_class_get(ed, edje_info->part_name , ret, val);
                       edje_info->image_name = edje_edit_state_image_get(ed, edje_info->part_name , ret, val);
                    }
                  util_mgr.edje_part_name_list = eina_list_append(util_mgr.edje_part_name_list, edje_info);
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
   Eina_Bool visible_only, gui_mode;
   int i, r, g, b, a;
   const char *ret;
   double val;
   const char *file, *key, *group, *text = NULL, *tb_style = NULL;
   char **ts = NULL;
   Edje_Info *edje_info;

   visible_only = (util_mgr.req_data)->visible_only;
   gui_mode = (util_mgr.req_data)->gui_mode;

   // visible check
   if (visible_only && !evas_object_is_visible_get(obj)) return;

   // viewport check
   evas_object_geometry_get(obj, &x, &y, &w, &h);

   if (visible_only && !RECTS_INTERSECT(x,y,w,h, util_mgr.x, util_mgr.y, util_mgr.w, util_mgr.h)) return;

   // clipper check
   if (evas_object_clipees_get(obj)) is_clip = EINA_TRUE;
   if (is_clip) goto next;

   if (!gui_mode)
     {
        if(elm_widget_is(obj)) PRINT("\n\033[36m");
        if (evas_object_type_match(obj, "edje")) PRINT("\033[33;1m");
     }

   if (gui_mode) PRINT("| ");

   PRINT("%3d: %-10p [%4d %4d %4d %4d]", ++util_mgr.cnt, obj, x, y, w, h);
   for (i = 0; i < lvl * 2; i++)
     PRINT( " ");

   if (evas_object_is_visible_get(obj)) PRINT("+ ");
   else PRINT("- ");

   // evas object type check
   if (evas_object_is_swallow_rect(obj)) PRINT("[%d] swallow ", lvl);
   else if (evas_object_type_match(obj, "rectangle"))
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
   if (smart_parent_obj && evas_object_type_match(obj, "image")
       && (evas_object_type_match(smart_parent_obj, "elm_icon")
           || evas_object_type_match(smart_parent_obj, "elm_image")))
     {
        if ((ret = evas_object_data_get(smart_parent_obj, "image_name")))
          {
             _extract_edje_file_name(ret);
             evas_object_data_del(smart_parent_obj, "edje_image_name");
          }
     }

   // edje info save
   if(evas_object_type_match(obj, "edje"))
     {
        edje_object_file_get(obj, &file, &group);
        if (group) PRINT("[%s] ", group);

        _extract_edje_file_name(file);
        _edje_file_info_save(obj);
     }

   // edje info check
   if( !is_clip && smart_parent_obj
       && !elm_widget_is(obj) && evas_object_type_match(smart_parent_obj, "edje"))
     {
        EINA_LIST_FOREACH(util_mgr.edje_part_name_list, l, edje_info)
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
          PRINT("[%s]\n", text);
        else
          PRINT("\n\t\t\t\t\t     ");
        if (!gui_mode) PRINT("\033[32;1m");

        for (i = 0; i < lvl * 2; i++)
          PRINT(" ");

        PRINT("==> [%s] ", text);
        if (tb_style)
          {
             if (!gui_mode) PRINT("\033[36m'");
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

   if (!gui_mode) PRINT("\n\033[37;1m");
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

   objs = evas_objects_in_rectangle_get(util_mgr.evas, SHRT_MIN, SHRT_MIN, USHRT_MAX, USHRT_MAX, EINA_TRUE, EINA_TRUE);

   ecore_evas_geometry_get(ecore_evas_ecore_evas_get(util_mgr.evas), &x, &y, &w, &h);

   EINA_LIST_FREE(objs, obj)
      _obj_tree_items(obj, 0 );

}

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
          PRINT("\033[33;1m [f] ");
        else
          PRINT("    ");

        if (evas_object_visible_get(obj))
          PRINT("+ %s \n", evas_object_type_get(obj));
        else
          PRINT("- %s \n", evas_object_type_get(obj));

        list = elm_widget_sub_object_list_get(obj);

        PRINT("\033[37;1m");

        EINA_LIST_FOREACH(list, l, obj)
          {
             _sub_widget_tree(obj, lvl + 1);
          }
     }
}

static void
_widget_tree()
{
   Evas_Object *top = evas_object_top_get(util_mgr.evas);
   _sub_widget_tree(top, 0);
}

static int
_find_obj_tree_item(Evas_Object *obj, char *obj_addr)
{
   Eina_List *children;
   Evas_Object *child;
   Eina_Bool visible_only;
   Evas_Coord x, y, w, h;
   char buf[20];

   if (util_mgr.req_obj) return 1;
   visible_only = (util_mgr.req_data)->visible_only;
   if (visible_only && !evas_object_is_visible_get(obj)) return 0;

   // viewport check
   evas_object_geometry_get(obj, &x, &y, &w, &h);

   if (visible_only && !RECTS_INTERSECT(x,y,w,h, util_mgr.x, util_mgr.y, util_mgr.w, util_mgr.h)) return 0;

   // clipper check
   if (evas_object_clipees_get(obj)) return 0;

   sprintf(buf, "%p", obj);
   if (!strcmp(buf, obj_addr))
     {
        util_mgr.req_obj = obj;
        return 1;
     }
   children = evas_object_smart_members_get(obj);
   EINA_LIST_FREE(children, child)
      _find_obj_tree_item(child, obj_addr);

   return (util_mgr.req_obj ? 1 : 0);
}

static Evas_Object *
_find_obj(char *obj_addr)
{
   Eina_List *objs, *l;
   Evas_Object *obj;
   int x, y, w, h;

   objs = evas_objects_in_rectangle_get(util_mgr.evas, SHRT_MIN, SHRT_MIN, USHRT_MAX, USHRT_MAX, EINA_TRUE, EINA_TRUE);

   ecore_evas_geometry_get(ecore_evas_ecore_evas_get(util_mgr.evas), &x, &y, &w, &h);
   EINA_LIST_FOREACH(objs, l, obj)
     {
        if (_find_obj_tree_item(obj, obj_addr)) break;
     }

   eina_list_free(objs);
   return util_mgr.req_obj;
}

static void
_object_info()
{
   Evas_Object *obj;
   Evas_Coord x, y, w, h;
   Evas_Aspect_Control aspect;
   const char *text = NULL, *tb_style = NULL;
   int r, g, b, a;
   double dx, dy;

   obj = util_mgr.req_obj;

   // evas_object info
   PRINT("\n------------------------- EVAS OBJECT ------------------------------------\n");
   PRINT("EVAS_OBJECT : %p  type = %s \n", obj, evas_object_type_get(obj));

   PRINT("evas_object_visible_get : %d \n", evas_object_visible_get(obj));

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
   PRINT("evas_object_size_hint_padding_get [l r t b] \t [%d %d %d %d] \n", x, y, w, h);

   PRINT("evas_object_anti_alias_get \t\t\t %d \n", evas_object_anti_alias_get(obj));
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
        PRINT("evas_object_image_file_get \t\t\t [%s] key : %s \n", file, key);

        evas_object_image_border_get(obj, &x, &y, &w, &h);
        PRINT("evas_object_image_border_get [l r t b] \t [%d %d %d %d] \n", x, y, w, h);

        evas_object_image_border_get(obj, &x, &y, &w, &h);
        PRINT("evas_object_image_border_get [l r t b] \t [%d %d %d %d] \n", x, y, w, h);
        PRINT("evas_object_image_filled_get \t\t\t %d \n", evas_object_image_filled_get(obj));
        PRINT("evas_object_image_border_scale_get \t\t %2.1f \n", evas_object_image_border_scale_get(obj));

        evas_object_image_fill_get(obj, &x, &y, &w, &h);
        PRINT("evas_object_image_fill_get [x y w h] \t [%d %d %d %d] \n", x, y, w, h);

        evas_object_image_size_get(obj, &w, &h);
        PRINT("evas_object_image_size_get [w h] \t\t [%d %d] \n", w, h);
        PRINT("evas_object_image_alpha_get \t\t\t %d \n", evas_object_image_alpha_get(obj));
        PRINT("evas_object_image_smooth_scale_get \t\t %d \n", evas_object_image_smooth_scale_get(obj));

        evas_object_image_load_size_get(obj, &w, &h);
        PRINT("evas_object_image_load_size_get [w h] \t [%d %d] \n", w, h);

        evas_object_image_load_region_get(obj, &x, &y, &w, &h);
        PRINT("evas_object_image_load_region_get [x y w h] \t [%d %d %d %d] \n", x, y, w, h);

        PRINT("evas_object_image_colorspace_get \t\t %d \n", evas_object_image_colorspace_get(obj));

        PRINT("evas_object_image_native_surface_get \t %p \n", evas_object_image_native_surface_get(obj));
     }

   PRINT("\n--------------------------- EVAS --------------------------------------\n");
   Evas *evas;
   Ecore_Evas *ecore_evas;

   evas = evas_object_evas_get(obj);
   ecore_evas = ecore_evas_ecore_evas_get(evas);

   PRINT("EVAS = %p  \n", evas);
   PRINT("evas_focus_state_get \t\t\t %d \n", evas_focus_state_get(evas));

   ecore_evas_geometry_get(ecore_evas, &x, &y, &w, &h);
   PRINT("ecore_evas_geometry_get [x y w h] \t\t [%d %d %d %d] \n", x, y, w,h);
   evas_output_size_get(evas, &w, &h);
   PRINT("evas_output_size_get [w h] \t\t\t [%d %d] \n", w, h);
   evas_output_viewport_get(evas, &x, &y, &w, &h);
   PRINT("evas_output_viewport_get [x y w h] \t\t [%d %d %d %d] \n", x, y, w, h);

   PRINT("\n--------------------------- SCALE  --------------------------------------\n");
   PRINT("elm_config_scale_get       [%2.1f] \t\t elm_widget_scale_get   [%2.1f] \n", elm_config_scale_get(), elm_widget_scale_get(obj));
   PRINT("elm_app_base_scale_get     [%2.1f] \t\t evas_object_scale_get  [%2.1f] \n", elm_app_base_scale_get(), evas_object_scale_get(obj));
   if(evas_object_type_match(obj, "edje"))
     PRINT("edje_object_base_scale_get [%2.1f]\n", edje_object_base_scale_get(obj));

   // elementary info
   if (elm_widget_is(obj))
     {
        PRINT("\n------------------------- ELEMENTARY ------------------------------------\n");
        const char *type;

        type = evas_object_type_get(obj);
        if (!strcmp(type, "elm_conformant"))
          {
             Evas_Object *win = elm_object_top_widget_get(obj);
             PRINT("elm_win_indicator_mode_get \t\t\t ");
             if (elm_win_indicator_mode_get(win) == ELM_WIN_INDICATOR_SHOW)
               PRINT("ELM_WIN_INDICATOR_SHOW \n");
             else if (elm_win_indicator_mode_get(win) == ELM_WIN_INDICATOR_HIDE)
               PRINT("ELM_WIN_INDICATOR_HIDE \n");
             else PRINT("ELM_WIN_INDICATOR_UNKNOWN \n");

             PRINT("elm_win_indicator_opacity_get \t\t ");
             if (elm_win_indicator_opacity_get(win) == ELM_WIN_INDICATOR_OPAQUE)
               PRINT("ELM_WIN_INDICATOR_OPAQUE \n");
             else if (elm_win_indicator_opacity_get(win) == ELM_WIN_INDICATOR_TRANSLUCENT)
               PRINT("ELM_WIN_INDICATOR_TRANSLUCENT \n");
             else if (elm_win_indicator_opacity_get(win) == ELM_WIN_INDICATOR_TRANSPARENT)
               PRINT("ELM_WIN_INDICATOR_TRANSPARENT \n");
             else if (elm_win_indicator_opacity_get(win) == ELM_WIN_INDICATOR_BG_TRANSPARENT)
               PRINT("ELM_WIN_INDICATOR_BG_TRANSPARENT \n");
             else PRINT("ELM_WIN_INDICATOR_OPACITY_UNKNOWN \n");

             PRINT("elm_win_indicator_type_get \t\t\t ");
             if (elm_win_indicator_type_get(win) == ELM_WIN_INDICATOR_TYPE_1)
               PRINT("ELM_WIN_INDICATOR_TYPE_1 \n");
             else if (elm_win_indicator_type_get(win) == ELM_WIN_INDICATOR_TYPE_2)
               PRINT("ELM_WIN_INDICATOR_TYPE_2 \n");
             else PRINT("ELM_WIN_INDICATOR_TYPE_UNKNOWN \n");

             Evas_Object *swallow;
             swallow = elm_layout_content_get(obj, "elm.swallow.indicator");
             evas_object_geometry_get(swallow, &x, &y, &w, &h);
             if (swallow) PRINT("\nelm.swallow.indicator [x y w h] \t\t [%4d %4d %4d %4d] \n", x, y, w, h);

             swallow = elm_layout_content_get(obj, "elm.swallow.content");
             evas_object_geometry_get(swallow, &x, &y, &w, &h);
             if (swallow) PRINT("elm.swallow.content [x y w h] \t\t [%4d %4d %4d %4d] \n", x, y, w, h);

             swallow = elm_layout_content_get(obj, "elm.swallow.ug");
             evas_object_geometry_get(swallow, &x, &y, &w, &h);
             if (swallow) PRINT("elm.swallow.ug [x y w h] \t\t [%4d %4d %4d %4d] \n", x, y, w, h);

             swallow = elm_layout_content_get(obj, "elm.swallow.virtualkeypad");
             evas_object_geometry_get(swallow, &x, &y, &w, &h);
             if (swallow) PRINT("elm.swallow.virtualkeypad [x y w h] \t\t [%4d %4d %4d %4d] \n", x, y, w, h);

             swallow = elm_layout_content_get(obj, "elm.swallow.clipboard");
             evas_object_geometry_get(swallow, &x, &y, &w, &h);
             if (swallow) PRINT("elm.swallow.clipboard [x y w h] \t\t [%4d %4d %4d %4d] \n", x, y, w, h);
          }
     }
   // TODO evas, elementary, edje info ADD
}

static void
utility_object_info_process()
{
   Evas *evas;
   Ecore_Evas *ee;
   Eina_List *ecore_evas_list, *l;
   char buf[20];

   if (util_mgr.req_obj)
     sprintf(buf, "%p", util_mgr.req_obj);

   if ((!util_mgr.req_obj)
       || (util_mgr.req_obj && (strcmp(buf, (util_mgr.req_data)->obj_addr))))
     {
        ecore_evas_list = ecore_evas_ecore_evas_list_get();

        util_mgr.cnt = 0;
        util_mgr.req_obj = NULL;

        EINA_LIST_FOREACH(ecore_evas_list, l, ee)
          {
             evas = ecore_evas_get(ee);
             util_mgr.evas = evas;

             util_mgr.req_obj = _find_obj((util_mgr.req_data)->obj_addr);
          }
        eina_list_free(ecore_evas_list);

     }
   file_write(FILE_LOG_PATH, _object_info);
}

static void
utility_tree_process()
{
   EFL_Util_IPC_Msg_Type msg = EFL_UTIL_MSG_NONE;
   Eina_Bool gui_mode = EINA_FALSE;
   Eina_List *ecore_evas_list;
   Ecore_Evas *ee;
   Evas *evas;

   msg = (util_mgr.req_data)->msg;
   gui_mode = (util_mgr.req_data)->gui_mode;

   ecore_evas_list = ecore_evas_ecore_evas_list_get();

   util_mgr.cnt = 0;
   util_mgr.req_obj = NULL;

   EINA_LIST_FREE(ecore_evas_list, ee)
     {
        evas = ecore_evas_get(ee);
        util_mgr.evas = evas;

        ecore_x_window_geometry_get(ecore_x_window_root_first_get(),
                                    &util_mgr.x, &util_mgr.y, &util_mgr.w, &util_mgr.h);

        if (msg == EFL_UTIL_MSG_WIDGET_TREE_DUMP_REQ)
          {
             file_write(FILE_LOG_PATH, _widget_tree);
          }
        else if (msg == EFL_UTIL_MSG_OBJECT_TREE_DUMP_REQ && gui_mode)
          {
             (util_mgr.req_data)->visible_only = EINA_TRUE;
             file_write(FILE_LOG_VISIBLE_FOR_GUI_PATH, _object_tree);

             (util_mgr.req_data)->visible_only = EINA_FALSE;
             util_mgr.cnt = 0;
             file_write(FILE_LOG_FULL_FOR_GUI_PATH, _object_tree);
          }
        else if (msg == EFL_UTIL_MSG_OBJECT_TREE_DUMP_REQ)
          {
             file_write(FILE_LOG_PATH, _object_tree);
          }
     }
   if (util_mgr.edje_part_name_list)
     eina_list_free(util_mgr.edje_part_name_list);
   util_mgr.edje_part_name_list = NULL;
}

static void
utility_evas_dump_process()
{
#if 0
   Eina_List *ecore_evas_list = NULL;
   Ecore_Evas *ee = NULL;
   Evas *evas = NULL;

   ecore_evas_list = ecore_evas_ecore_evas_list_get();
   char dump_path[1024];
   int i = 0;

   EINA_LIST_FREE(ecore_evas_list, ee)
     {
        evas = ecore_evas_get(ee);
        sprintf(dump_path,"/tmp/evas_dump/layout.%d",i);
        evas_dump_path_set(dump_path);
        evas_dump(evas);
        i++;
     }
#endif
}

static void
_ecore_resource_dump(void)
{
   PRINT(RESET "\n                                   < ECORE RESOURCE INFO > \n\n");
//   PRINT(RESET "%s \n\n", _ecore_timer_dump());
//   PRINT(RESET "%s \n", _ecore_animator_dump());
}

static void
utility_ecore_resource_process(void)
{
   file_write(FILE_LOG_PATH, _ecore_resource_dump);
}

static Eina_Bool
_ipc_client_add_cb(void *data, int type __UNUSED__, void *event)
{
   Ecore_Ipc_Server  *server;
   Ecore_Ipc_Event_Client_Data *e;

   server = (Ecore_Ipc_Server *) data;
   e = (Ecore_Ipc_Event_Client_Data *) event;

   if (server != ecore_ipc_client_server_get(e->client))
     return EINA_TRUE;

   printf("EFL DBG Tool is connected @ %p \n", server);
   return EINA_TRUE;
}

static Eina_Bool
_ipc_client_data_recv_cb(void *data __UNUSED__, int type __UNUSED__, void *event)
{
   Ecore_Ipc_Event_Client_Data *e = event;
   EFL_Util_Ipc_Data rep_data;
   EFL_Util_IPC_Msg_Type msg = EFL_UTIL_MSG_NONE;

   if (ipc_server != ecore_ipc_client_server_get(e->client))
      return EINA_TRUE;

   util_mgr.req_data = (EFL_Util_Ipc_Data*)e->data;
   msg = (util_mgr.req_data)->msg;

   switch (msg)
     {
      case EFL_UTIL_MSG_WIDGET_TREE_DUMP_REQ:
         utility_tree_process();
         rep_data.msg = EFL_UTIL_MSG_WIDGET_TREE_DUMP_REP;
         break;

      case EFL_UTIL_MSG_OBJECT_TREE_DUMP_REQ:
         utility_tree_process();
         rep_data.msg = EFL_UTIL_MSG_OBJECT_TREE_DUMP_REP;
         break;

      case EFL_UTIL_MSG_OBJECT_INFO_REQ:
         utility_object_info_process();
         rep_data.msg = EFL_UTIL_MSG_OBJECT_INFO_REP;
         break;

      case EFL_UTIL_MSG_EVAS_DUMP_REQ:
         utility_evas_dump_process();
         rep_data.msg = EFL_UTIL_MSG_EVAS_DUMP_REP;
         break;

      case EFL_UTIL_MSG_EVENT_INFO_REQ:
         rep_data.msg = EFL_UTIL_MSG_EVENT_INFO_REP;
         break;

      case EFL_UTIL_MSG_MEMORY_INFO_REQ:
         rep_data.msg = EFL_UTIL_MSG_MEMORY_INFO_REP;
         break;

      case EFL_UTIL_MSG_ECORE_RESOURCE_DUMP_REQ:
         utility_ecore_resource_process();
         rep_data.msg = EFL_UTIL_MSG_ECORE_RESOURCE_DUMP_REP;
         break;

      default:
         break;
     }

   if (msg) ecore_ipc_client_send(e->client, 0, 0, 0, 0, 0, &rep_data, sizeof(rep_data));

   return 1;
}

EAPI int
_utilily_ipc_del()
{
   int i;
   if (_utility_ipc_count)
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
   _utility_ipc_count = 0;
   return 1;
}

static Eina_Bool
_ipc_client_del_cb(void *data __UNUSED__, int type __UNUSED__, void *event __UNUSED__)
{
   Ecore_Ipc_Event_Client_Data *e = event;
   if (ipc_server != ecore_ipc_client_server_get(e->client))
      return EINA_TRUE;

   _utilily_ipc_del();
   return ECORE_CALLBACK_PASS_ON;
}

EAPI int
_utilily_ipc_add()
{
   if (_utility_ipc_count) return 0;

   if (ecore_ipc_init() < 1)
     return 0;

   _utility_ipc_count++;

   ipc_server = ecore_ipc_server_add(ECORE_IPC_LOCAL_SYSTEM, "efl_util", 516, NULL);
   if (ipc_server) printf("Server is listening on \n");
   else
     {
        printf("CONNECT FAIL ! \n");
        return 0;
     }

   ecore_ipc_event_handlers[0] = ecore_event_handler_add(ECORE_IPC_EVENT_CLIENT_ADD, _ipc_client_add_cb, ipc_server);
   ecore_ipc_event_handlers[1] = ecore_event_handler_add(ECORE_IPC_EVENT_CLIENT_DATA, _ipc_client_data_recv_cb, ipc_server);
   ecore_ipc_event_handlers[2] = ecore_event_handler_add(ECORE_IPC_EVENT_CLIENT_DEL, _ipc_client_del_cb, ipc_server);

   return 1;
}

static int
_utility_init()
{
   int ret;
   if (_utility_count) return 0;

   if (!(ret = _utilily_ipc_add()))
     {
        printf("_utilily_ipc_add ret = %d \n", ret);
        _utilily_ipc_del();
     }

   _utility_count++;
   return _utility_count;
}

static int
_utility_shutdown()
{
   if (!_utility_count) return _utility_count;

   _utilily_ipc_del();
   _utility_count = 0;

   return _utility_count;
}

EAPI int
elm_modapi_init(void *m __UNUSED__)
{
   _utility_init();

   return 1;
}

EAPI int
elm_modapi_shutdown(void *m __UNUSED__)
{
   _utility_shutdown();

   return 1;
}

void
evas_dump(Evas* e)
{

}

void
evas_dump_path_set(const char* path)
{

}
