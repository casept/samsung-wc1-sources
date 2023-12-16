#ifndef ELM_GEN_H_
#define ELM_GEN_H_

#include <Elementary.h>
#include <Elementary_Cursor.h>
#include "elm_priv.h"

#define ELM_GEN_ITEM_FROM_INLIST(it) \
  ((it) ? EINA_INLIST_CONTAINER_GET(it, Elm_Gen_Item) : NULL)

#define SWIPE_MOVES 12

/* common item handles for genlist/gengrid */

typedef struct Elm_Gen_Item_Type    Elm_Gen_Item_Type;
typedef struct Elm_Gen_Item_Tooltip Elm_Gen_Item_Tooltip;

struct Elm_Gen_Item_Tooltip
{
   const void                 *data;
   Elm_Tooltip_Item_Content_Cb content_cb;
   Evas_Smart_Cb               del_cb;
   const char                 *style;
   Eina_Bool                   free_size : 1;
};

struct Elm_Gen_Item
{
   ELM_WIDGET_ITEM;
   EINA_INLIST;

   Elm_Gen_Item_Type        *item;
   const Elm_Gen_Item_Class *itc;
   Evas_Coord                x, y, dx, dy;
   Evas_Object              *spacer, *deco_all_view;
   Elm_Gen_Item             *parent;
   Eina_List                *texts, *contents, *states, *content_objs;
   Ecore_Timer              *long_timer;
   int                       walking;
   int                       generation; /**< a generation of an item. when the item is created, this value is set to the value of genlist generation. this value will be decreased when the item is going to be deleted */
   const char               *mouse_cursor;

   struct
   {
      Evas_Smart_Cb func;
      const void   *data;
   } func;

   Elm_Gen_Item_Tooltip      tooltip;
   Ecore_Cb                  del_cb, unrealize_cb;
   Ecore_Cb                  sel_cb, unsel_cb;
   Ecore_Cb                  highlight_cb, unhighlight_cb;

   int                       position;
   Elm_Object_Select_Mode    select_mode;

   Eina_Bool                 position_update : 1;
   Eina_Bool                 want_unrealize : 1;
   Eina_Bool                 realized : 1;
   Eina_Bool                 selected : 1;
   Eina_Bool                 highlighted : 1;
   Eina_Bool                 dragging : 1; /**< this is set true when an item is being dragged. this is set false on multidown/mouseup/mousedown. when this is true, the item should not be unrealized. or evas mouse down/up event will be corrupted. */
   Eina_Bool                 down : 1;
   Eina_Bool                 group : 1;
   Eina_Bool                 reorder : 1;
   Eina_Bool                 decorate_it_set : 1; /**< item uses style mode for highlight/select */
   Eina_Bool                 flipped : 1; /**< a flag that shows the flip status of the item. */
   Eina_Bool                 hide : 1;
};

typedef enum
{
   ELM_GEN_ITEM_FX_TYPE_SAME,
   ELM_GEN_ITEM_FX_TYPE_ADD,
   ELM_GEN_ITEM_FX_TYPE_DEL,
} Elm_Gen_Item_Fx_Type;

typedef struct _Proxy_Item Proxy_Item;
struct _Proxy_Item
{
   int                      num;
   Elm_Gen_Item             *it;
   Evas_Object              *proxy;
   Evas_Coord                x, y, w, h;
};

typedef struct _Elm_Gen_FX_Item Elm_Gen_FX_Item;
struct _Elm_Gen_FX_Item
{
   int                       num;
   Elm_Gen_Item             *it;
   Evas_Object              *proxy;
   Elm_Gen_Item_Fx_Type      type;
   Elm_Transit              *trans;

   struct
   {
      Evas_Coord x, y, w, h;
   } from;

   struct
   {
      Evas_Coord x, y, w, h;
   } to;

   Eina_Bool update : 1;
   Eina_Bool changed : 1;
};

typedef enum
{
   ELM_GEN_PINCH_ZOOM_NONE = 0,
   ELM_GEN_PINCH_ZOOM_CONTRACT = 1,
   ELM_GEN_PINCH_ZOOM_EXPAND = 2
} Elm_Gen_Pinch_Zoom_Mode;

#endif
