#include "Elementary.h"
#include <Eina.h>
#ifdef HAVE_CONFIG_H
# include "elementary_config.h"
#endif

static Evas_Object *view;

EAPI const char *
map_module_name_get(void)
{
   return "Test Map Engine";
}

EAPI Evas_Object *
map_module_add(Evas_Object *parent __UNUSED__)
{
   if (!view)
     {
        view = evas_object_rectangle_add(evas_object_evas_get(parent));
        evas_object_color_set(view, 0, 50, 0, 50);
        evas_object_repeat_events_set(view, EINA_TRUE);
     }
   evas_object_show(view);
   return view;
}

EAPI void
map_module_del(Evas_Object *parent __UNUSED__)
{
   if (view) evas_object_hide(view);
}

EAPI void
map_module_resize(Evas_Object *parent __UNUSED__, int w __UNUSED__, int h __UNUSED__)
{
   evas_object_resize(view, w, h);
}

EAPI int
map_module_zoom_min_get()
{
   return 0;
}

EAPI int
map_module_zoom_max_get()
{
   return 18;
}

EAPI void
map_module_show(Evas_Object *obj __UNUSED__, double lon __UNUSED__, double lat __UNUSED__, int animation __UNUSED__)
{
   if (view) evas_object_color_set(view, 0, 0, 50, 50);
}

EAPI void
map_module_zoom(Evas_Object *obj __UNUSED__, double zoom __UNUSED__, int animation __UNUSED__)
{
   if (view) evas_object_color_set(view, 50, 0, 0, 50);
}

EAPI void
map_module_rotate(Evas_Object *obj __UNUSED__, double angle __UNUSED__, int x __UNUSED__, int y __UNUSED__, int animation __UNUSED__)
{
   if (view) evas_object_color_set(view, 0, 0, 50, 50);
}

EAPI void
map_module_region_get(Evas_Object *obj __UNUSED__, double *lon __UNUSED__, double *lat __UNUSED__)
{
   return;
}

EAPI void
map_module_rotate_get(const Evas_Object *obj __UNUSED__, double *angle __UNUSED__, int *x __UNUSED__, int *y __UNUSED__)
{
   return;
}

EAPI void
map_module_perpective_set(Evas_Object *obj __UNUSED__, double perspective __UNUSED__, int animation __UNUSED__)
{
   if (view) evas_object_color_set(view, 50, 50, 50, 50);
}

EAPI void
map_module_canvas_to_region(const Evas_Object *obj __UNUSED__, int x __UNUSED__, int y __UNUSED__, double *lon __UNUSED__, double *lat __UNUSED__)
{
   return;
}

EAPI void
map_module_region_to_canvas(const Evas_Object *obj __UNUSED__, double lon __UNUSED__, double lat __UNUSED__, int *x __UNUSED__, int *y __UNUSED__)
{
   return;
}

EAPI void
map_module_pan(Evas_Object *obj __UNUSED__, int x_1 __UNUSED__, int y_1 __UNUSED__, int x_2 __UNUSED__, int y_2 __UNUSED__)
{
   if (view) evas_object_color_set(view, 128, 128,128, 128);
}

static Eina_Bool
_module_init(void)
{
   return EINA_TRUE;
}

static void
_module_shutdown()
{
}

EINA_MODULE_INIT(_module_init);
EINA_MODULE_SHUTDOWN(_module_shutdown);

