#ifndef __DEF_Utility_Mod_H_
#define __DEF_Utility_Mod_H_

#include <Elementary.h>
#ifdef HAVE_CONFIG_H
# include "elementary_config.h"
#endif
#include <math.h>
#include <Ecore_Ipc.h>
#include <Ecore_X.h>
#include <string.h>
#include <Eina.h>
#include <elm_widget.h>
#define EDJE_EDIT_IS_UNSTABLE_AND_I_KNOW_ABOUT_IT
#include "Edje_Edit.h"

#include "efl_utiliy_interface.h"

typedef struct _Ea_Util_Mgr
{
   Evas *evas;
   FILE *fp;

   Eina_List *edje_part_name_list;
   int x,y,w,h;
   int cnt;
   EFL_Util_Ipc_Data  *req_data;
   Evas_Object *req_obj;

} Ea_Util_Mgr;

typedef struct _Edje_Info
{
   const Evas_Object *obj;

   char part_name[256];
   const char *image_name;
   const char *color_class;

} Edje_Info;

extern void       evas_dump(Evas* e);
extern void        evas_dump_path_set(const char* path);
typedef void      (*func_cb)();

static Ea_Util_Mgr util_mgr;

static Ecore_Ipc_Server *ipc_server;
static int _utility_count, _utility_ipc_count;
static Ecore_Event_Handler *ecore_ipc_event_handlers[10];

#define RECTS_INTERSECT(x, y, w, h, xx, yy, ww, hh) \
   (((x) <= ((xx) + (ww))) && ((y) <= ((yy) + (hh))) && (((x) + (w)) >= (xx)) && (((y) + (h)) >= (yy)))

#endif
