#ifdef HAVE_CONFIG_H
# include "elementary_config.h"
#endif
#include <Elementary.h>

static const char *img[9] =
{
   "panel_01.jpg",
   "plant_01.jpg",
   "rock_01.jpg",
   "rock_02.jpg",
   "sky_01.jpg",
   "sky_02.jpg",
   "sky_03.jpg",
   "sky_04.jpg",
   "wood_01.jpg",
};

struct _anim_icon_st
{
   int start_x;
   int start_y;
   Evas_Object *o;
};
typedef struct _anim_icon_st anim_icon_st;

struct _drag_anim_st
{
   Evas_Object *icwin;
   Evas *e;
   Evas_Coord mdx;     /* Mouse-down x */
   Evas_Coord mdy;     /* Mouse-down y */
   Eina_List *icons;   /* List of icons to animate (anim_icon_st) */
   Ecore_Timer *tm;
   Ecore_Animator *ea;
   Evas_Object *gl;
};
typedef struct _drag_anim_st drag_anim_st;

#define DRAG_TIMEOUT 0.3
#define ANIM_TIME 0.5

static Eina_Bool _5s_cancel = EINA_FALSE;
static Ecore_Timer *_5s_timeout = NULL;

static int
_item_ptr_cmp(const void *d1, const void *d2)
{
   return (d1 - d2);
}

static Elm_Genlist_Item_Class *itc1;
static Elm_Gengrid_Item_Class *gic;

static char *
gl_text_get(void *data, Evas_Object *obj EINA_UNUSED, const char *part EINA_UNUSED)
{
   return strdup(data);
}

static Evas_Object *
gl_content_get(void *data, Evas_Object *obj, const char *part)
{
   if (!strcmp(part, "elm.swallow.icon"))
     {
        Evas_Object *icon = elm_icon_add(obj);
        elm_image_file_set(icon, data, NULL);
        evas_object_size_hint_aspect_set(icon, EVAS_ASPECT_CONTROL_VERTICAL, 1, 1);
        evas_object_show(icon);
        return icon;
     }
   return NULL;
}

static void
_win_del(void *data, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   printf("<%s> <%d> will del <%p>\n", __func__, __LINE__, data);
   elm_drop_item_container_del(data);
   elm_drag_item_container_del(data);

   if (gic) elm_gengrid_item_class_free(gic);
   gic = NULL;
   if (itc1) elm_genlist_item_class_free(itc1);
   itc1 = NULL;
}

static Elm_Object_Item *
_gl_item_getcb(Evas_Object *obj, Evas_Coord x, Evas_Coord y, int *xposret EINA_UNUSED, int *yposret)
{  /* This function returns pointer to item under (x,y) coords */
   printf("<%s> <%d> obj=<%p>\n", __func__, __LINE__, obj);
   Elm_Object_Item *gli;
   gli = elm_genlist_at_xy_item_get(obj, x, y, yposret);
   if (gli)
     printf("over <%s>, gli=<%p> yposret %i\n",
           (char *)elm_object_item_data_get(gli), gli, *yposret);
   else
     printf("over none, yposret %i\n", *yposret);
   return gli;
}

static Elm_Object_Item *
_grid_item_getcb(Evas_Object *obj, Evas_Coord x, Evas_Coord y, int *xposret, int *yposret)
{  /* This function returns pointer to item under (x,y) coords */
   printf("<%s> <%d> obj=<%p>\n", __func__, __LINE__, obj);
   Elm_Object_Item *item;
   item = elm_gengrid_at_xy_item_get(obj, x, y, xposret, yposret);
   if (item)
     printf("over <%s>, item=<%p> xposret %i yposret %i\n",
           (char *)elm_object_item_data_get(item), item, *xposret, *yposret);
   else
     printf("over none, xposret %i yposret %i\n", *xposret, *yposret);
   return item;
}

static Eina_Bool
_gl_dropcb(void *data EINA_UNUSED, Evas_Object *obj, Elm_Object_Item *it, Elm_Selection_Data *ev, int xposret EINA_UNUSED, int yposret)
{  /* This function is called when data is dropped on the genlist */
   printf("<%s> <%d> str=<%s>\n", __func__, __LINE__, (char *) ev->data);
   if (!ev->data)
     return EINA_FALSE;

   char *p = ev->data;
   p = strchr(p, '#');
   while(p)
     {
        p++;
        char *p2 = strchr(p, '#');
        if (p2)
          {
             *p2 = '\0';
             printf("Item %s\n", p);
             switch(yposret)
               {
                case -1:  /* Dropped on top-part of the it item */
                     {
                        elm_genlist_item_insert_before(obj,
                              itc1, strdup(p), NULL, it,
                              ELM_GENLIST_ITEM_NONE,
                              NULL, NULL);
                        break;
                     }
                case  0:  /* Dropped on center of the it item      */
                case  1:  /* Dropped on botton-part of the it item */
                     {
                        if (!it) it = elm_genlist_last_item_get(obj);
                        if (it) it = elm_genlist_item_insert_after(obj,
                              itc1, strdup(p), NULL, it,
                              ELM_GENLIST_ITEM_NONE,
                              NULL, NULL);
                        else
                           it = elm_genlist_item_append(obj,
                                 itc1, strdup(p), NULL,
                                 ELM_GENLIST_ITEM_NONE,
                                 NULL, NULL);
                        break;
                     }
                default:
                   return EINA_FALSE;
               }
             p = p2;
          }
        else p = NULL;
     }

   return EINA_TRUE;
}

static Eina_Bool
_grid_dropcb(void *data EINA_UNUSED, Evas_Object *obj, Elm_Object_Item *it, Elm_Selection_Data *ev, int xposret EINA_UNUSED, int yposret EINA_UNUSED)
{  /* This function is called when data is dropped on the genlist */
   printf("<%s> <%d> str=<%s>\n", __func__, __LINE__, (char *) ev->data);
   if (!ev->data)
     return EINA_FALSE;

   char *p = ev->data;
   p = strchr(p, '#');
   while(p)
     {
        p++;
        char *p2 = strchr(p, '#');
        if (p2)
          {
             *p2 = '\0';
             printf("Item %s\n", p);
             if (!it) it = elm_gengrid_last_item_get(obj);
             if (it) it = elm_gengrid_item_insert_after(obj, gic, strdup(p), it, NULL, NULL);
             else it = elm_gengrid_item_append(obj, gic, strdup(p), NULL, NULL);
             p = p2;
          }
        else p = NULL;
     }

   return EINA_TRUE;
}

static void _gl_obj_mouse_move( void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _gl_obj_mouse_up( void *data, Evas *e, Evas_Object *obj, void *event_info);

static void
anim_st_free(drag_anim_st *anim_st)
{  /* Stops and free mem of ongoing animation */
   printf("<%s> <%d>\n", __func__, __LINE__);
   if (anim_st)
     {
        evas_object_event_callback_del_full
           (anim_st->gl, EVAS_CALLBACK_MOUSE_MOVE, _gl_obj_mouse_move, anim_st);
        evas_object_event_callback_del_full
           (anim_st->gl, EVAS_CALLBACK_MOUSE_UP, _gl_obj_mouse_up, anim_st);
        if (anim_st->tm)
          {
             ecore_timer_del(anim_st->tm);
             anim_st->tm = NULL;
          }

        if (anim_st->ea)
          {
             ecore_animator_del(anim_st->ea);
             anim_st->ea = NULL;
          }

        anim_icon_st *st;

        EINA_LIST_FREE(anim_st->icons, st)
          {
             evas_object_hide(st->o);
             evas_object_del(st->o);
             free(st);
          }

        free(anim_st);
     }
}

static Eina_Bool
_drag_anim_play(void *data, double pos)
{  /* Impl of the animation of icons, called on frame time */
   drag_anim_st *anim_st = data;
   printf("<%s> <%d>\n", __func__, __LINE__);
   Eina_List *l;
   anim_icon_st *st;

   if (anim_st)
     {
        if (pos > 0.99)
          {
             anim_st->ea = NULL;  /* Avoid deleting on mouse up */

             EINA_LIST_FOREACH(anim_st->icons, l, st)
                evas_object_hide(st->o);   /* Hide animated icons */
             anim_st_free(anim_st);
             return ECORE_CALLBACK_CANCEL;
          }

        EINA_LIST_FOREACH(anim_st->icons, l, st)
          {
             int x, y, w, h;
             Evas_Coord xm, ym;
             evas_object_geometry_get(st->o, NULL, NULL, &w, &h);
             evas_pointer_canvas_xy_get(anim_st->e, &xm, &ym);
             x = st->start_x + (pos * (xm - (st->start_x + (w/2))));
             y = st->start_y + (pos * (ym - (st->start_y + (h/2))));
             evas_object_move(st->o, x, y);
          }

        return ECORE_CALLBACK_RENEW;
     }

   return ECORE_CALLBACK_CANCEL;
}

static Eina_Bool
_gl_anim_start(void *data)
{  /* Start icons animation before actually drag-starts */
   drag_anim_st *anim_st = data;
   printf("<%s> <%d>\n", __func__, __LINE__);
   int yposret = 0;

   Eina_List *l;
   Eina_List *items = eina_list_clone(elm_genlist_selected_items_get(anim_st->gl));
   Elm_Object_Item *gli = elm_genlist_at_xy_item_get(anim_st->gl,
         anim_st->mdx, anim_st->mdy, &yposret);
   if (gli)
     {  /* Add the item mouse is over to the list if NOT seleced */
        void *p = eina_list_search_unsorted(items, _item_ptr_cmp, gli);
        if (!p)
          items = eina_list_append(items, gli);
     }

   EINA_LIST_FOREACH(items, l, gli)
     {  /* Now add icons to animation window */
        Evas_Object *o = elm_object_item_part_content_get(gli,
              "elm.swallow.icon");

        if (o)
          {
             int w, h;
             const char *f;
             const char *g;
             anim_icon_st *st = calloc(1, sizeof(*st));
             elm_image_file_get(o, &f, &g);
             Evas_Object *ic = elm_icon_add(anim_st->gl);
             elm_image_file_set(ic, f, g);
             evas_object_geometry_get(o, &st->start_x, &st->start_y, &w, &h);
             evas_object_size_hint_align_set(ic,
                   EVAS_HINT_FILL, EVAS_HINT_FILL);
             evas_object_size_hint_weight_set(ic,
                   EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);

             evas_object_move(ic, st->start_x, st->start_y);
             evas_object_resize(ic, w, h);
             evas_object_show(ic);

             st->o = ic;
             anim_st->icons = eina_list_append(anim_st->icons, st);
          }
     }

   eina_list_free(items);

   anim_st->tm = NULL;
   anim_st->ea = ecore_animator_timeline_add(DRAG_TIMEOUT,
         _drag_anim_play, anim_st);

   return ECORE_CALLBACK_CANCEL;
}

static void
_gl_obj_mouse_up(
   void *data,
   Evas *e EINA_UNUSED,
   Evas_Object *obj EINA_UNUSED,
   void *event_info EINA_UNUSED)
{  /* Cancel any drag waiting to start on timeout */
   drag_anim_st *anim_st = data;
   anim_st_free(anim_st);
}

static void
_gl_obj_mouse_move(
   void *data,
   Evas *e EINA_UNUSED,
   Evas_Object *obj EINA_UNUSED,
   void *event_info)
{  /* Cancel any drag waiting to start on timeout */

   if (((Evas_Event_Mouse_Move *)event_info)->event_flags & EVAS_EVENT_FLAG_ON_HOLD)
     {
        drag_anim_st *anim_st = data;
        anim_st_free(anim_st);
     }
}

static void
_gl_obj_mouse_down(
   void *data,
   Evas *e EINA_UNUSED,
   Evas_Object *obj EINA_UNUSED,
   void *event_info)
{  /* Launch a timer to start drag animation */
   Evas_Event_Mouse_Down *ev = event_info;
   drag_anim_st *anim_st = calloc(1, sizeof(*anim_st));
   anim_st->e = e;
   anim_st->mdx = ev->canvas.x;
   anim_st->mdy = ev->canvas.y;
   anim_st->gl = data;
   anim_st->tm = ecore_timer_add(DRAG_TIMEOUT, _gl_anim_start, anim_st);
   evas_object_event_callback_add(data, EVAS_CALLBACK_MOUSE_UP,
         _gl_obj_mouse_up, anim_st);
   evas_object_event_callback_add(data, EVAS_CALLBACK_MOUSE_MOVE,
         _gl_obj_mouse_move, anim_st);
}
/* END   - Handling drag start animation */

static void
_gl_dragdone(void *data, Evas_Object *obj EINA_UNUSED, Eina_Bool doaccept)
{
   printf("<%s> <%d> data=<%p> doaccept=<%d>\n",
         __func__, __LINE__, data, doaccept);

   Elm_Object_Item *it;
   Eina_List *l;

   if (_5s_cancel)
     {
        ecore_timer_del(_5s_timeout);
        _5s_timeout = NULL;
     }

   if (doaccept)
     {  /* Remove items dragged out (accepted by target) */
        EINA_LIST_FOREACH(data, l, it)
           elm_object_item_del(it);
     }

   eina_list_free(data);
   return;
}

static Eina_Bool
_5s_timeout_gone(void *data)
{
   printf("Cancel DnD\n");
   elm_drag_cancel(data);
   _5s_timeout = NULL;
   return ECORE_CALLBACK_CANCEL;
}

static Evas_Object *
_gl_createicon(void *data, Evas_Object *win, Evas_Coord *xoff, Evas_Coord *yoff)
{
   printf("<%s> <%d>\n", __func__, __LINE__);
   Evas_Object *icon = NULL;
   Evas_Object *o = elm_object_item_part_content_get(data, "elm.swallow.icon");

   if (o)
     {
        int xm, ym, w = 30, h = 30;
        const char *f;
        const char *g;
        elm_image_file_get(o, &f, &g);
        evas_pointer_canvas_xy_get(evas_object_evas_get(o), &xm, &ym);
        if (xoff) *xoff = xm - (w/2);
        if (yoff) *yoff = ym - (h/2);
        icon = elm_icon_add(win);
        elm_image_file_set(icon, f, g);
        evas_object_size_hint_align_set(icon,
              EVAS_HINT_FILL, EVAS_HINT_FILL);
        evas_object_size_hint_weight_set(icon,
              EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
        if (xoff && yoff) evas_object_move(icon, *xoff, *yoff);
        evas_object_resize(icon, w, h);
     }

   return icon;
}

static Eina_List *
_gl_icons_get(void *data)
{  /* Start icons animation before actually drag-starts */
   printf("<%s> <%d>\n", __func__, __LINE__);
   int yposret = 0;

   Eina_List *l;

   Eina_List *icons = NULL;

   Evas_Coord xm, ym;
   evas_pointer_canvas_xy_get(evas_object_evas_get(data), &xm, &ym);
   Eina_List *items = eina_list_clone(elm_genlist_selected_items_get(data));
   Elm_Object_Item *gli = elm_genlist_at_xy_item_get(data,
         xm, ym, &yposret);
   if (gli)
     {  /* Add the item mouse is over to the list if NOT seleced */
        void *p = eina_list_search_unsorted(items, _item_ptr_cmp, gli);
        if (!p)
          items = eina_list_append(items, gli);
     }

   EINA_LIST_FOREACH(items, l, gli)
     {  /* Now add icons to animation window */
        Evas_Object *o = elm_object_item_part_content_get(gli,
              "elm.swallow.icon");

        if (o)
          {
             int x, y, w, h;
             const char *f, *g;
             elm_image_file_get(o, &f, &g);
             Evas_Object *ic = elm_icon_add(data);
             elm_image_file_set(ic, f, g);
             evas_object_geometry_get(o, &x, &y, &w, &h);
             evas_object_size_hint_align_set(ic,
                   EVAS_HINT_FILL, EVAS_HINT_FILL);
             evas_object_size_hint_weight_set(ic,
                   EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);

             evas_object_move(ic, x, y);
             evas_object_resize(ic, w, h);
             evas_object_show(ic);

             icons =  eina_list_append(icons, ic);
          }
     }

   eina_list_free(items);
   return icons;
}

static const char *
_gl_get_drag_data(Evas_Object *obj, Elm_Object_Item *it, Eina_List **items)
{  /* Construct a string of dragged info, user frees returned string */
   const char *drag_data = NULL;
   printf("<%s> <%d>\n", __func__, __LINE__);

   *items = eina_list_clone(elm_genlist_selected_items_get(obj));
   if (it)
     {  /* Add the item mouse is over to the list if NOT seleced */
        void *p = eina_list_search_unsorted(*items, _item_ptr_cmp, it);
        if (!p)
          *items = eina_list_append(*items, it);
     }

   if (*items)
     {  /* Now we can actually compose string to send and start dragging */
        Eina_List *l;
        const char *t;
        unsigned int len = 0;

        EINA_LIST_FOREACH(*items, l, it)
          {
             t = (char *)elm_object_item_data_get(it);
             if (t)
               len += strlen(t);
          }

        drag_data = malloc(len + eina_list_count(*items) * 2 + 8);
        strcpy((char *) drag_data, "file://");

        EINA_LIST_FOREACH(*items, l, it)
          {
             t = (char *)elm_object_item_data_get(it);
             if (t)
               {
                  strcat((char *) drag_data, "#");
                  strcat((char *) drag_data, t);
               }
          }
        strcat((char *) drag_data, "#");

        printf("<%s> <%d> Sending <%s>\n", __func__, __LINE__, drag_data);
     }

   return drag_data;
}

static const char *
_grid_get_drag_data(Evas_Object *obj, Elm_Object_Item *it, Eina_List **items)
{  /* Construct a string of dragged info, user frees returned string */
   const char *drag_data = NULL;
   printf("<%s> <%d>\n", __func__, __LINE__);

   *items = eina_list_clone(elm_gengrid_selected_items_get(obj));
   if (it)
     {  /* Add the item mouse is over to the list if NOT seleced */
        void *p = eina_list_search_unsorted(*items, _item_ptr_cmp, it);
        if (!p)
          *items = eina_list_append(*items, it);
     }

   if (*items)
     {  /* Now we can actually compose string to send and start dragging */
        Eina_List *l;
        const char *t;
        unsigned int len = 0;

        EINA_LIST_FOREACH(*items, l, it)
          {
             t = (char *)elm_object_item_data_get(it);
             if (t)
               len += strlen(t);
          }

        drag_data = malloc(len + eina_list_count(*items) * 2 + 8);
        strcpy((char *) drag_data, "file://");

        EINA_LIST_FOREACH(*items, l, it)
          {
             t = (char *)elm_object_item_data_get(it);
             if (t)
               {
                  strcat((char *) drag_data, "#");
                  strcat((char *) drag_data, t);
               }
          }
        strcat((char *) drag_data, "#");

        printf("<%s> <%d> Sending <%s>\n", __func__, __LINE__, drag_data);
     }

   return drag_data;
}

static Eina_Bool
_gl_dnd_default_anim_data_getcb(Evas_Object *obj,  /* The genlist object */
      Elm_Object_Item *it,
      Elm_Drag_User_Info *info)
{  /* This called before starting to drag, mouse-down was on it */
   info->format = ELM_SEL_FORMAT_TARGETS;
   info->createicon = _gl_createicon;
   info->createdata = it;
   info->icons = _gl_icons_get(obj);
   info->dragdone = _gl_dragdone;

   /* Now, collect data to send for drop from ALL selected items */
   /* Save list pointer to remove items after drop and free list on done */
   info->data = _gl_get_drag_data(obj, it, (Eina_List **) &info->donecbdata);
   printf("%s - data = %s\n", __FUNCTION__, info->data);
   info->acceptdata = info->donecbdata;

   if (info->data)
     return EINA_TRUE;
   else
     return EINA_FALSE;
}

static Eina_Bool
_gl_data_getcb(Evas_Object *obj,  /* The genlist object */
      Elm_Object_Item *it,
      Elm_Drag_User_Info *info)
{  /* This called before starting to drag, mouse-down was on it */
   info->format = ELM_SEL_FORMAT_TARGETS;
   info->createicon = _gl_createicon;
   info->createdata = it;
   info->dragdone = _gl_dragdone;

   /* Now, collect data to send for drop from ALL selected items */
   /* Save list pointer to remove items after drop and free list on done */
   info->data = _gl_get_drag_data(obj, it, (Eina_List **) &info->donecbdata);
   info->acceptdata = info->donecbdata;

   if (info->data)
     return EINA_TRUE;
   else
     return EINA_FALSE;
}

static Eina_List *
_grid_icons_get(void *data)
{  /* Start icons animation before actually drag-starts */
   printf("<%s> <%d>\n", __func__, __LINE__);

   Eina_List *l;

   Eina_List *icons = NULL;

   Evas_Coord xm, ym;
   evas_pointer_canvas_xy_get(evas_object_evas_get(data), &xm, &ym);
   Eina_List *items = eina_list_clone(elm_gengrid_selected_items_get(data));
   Elm_Object_Item *gli = elm_gengrid_at_xy_item_get(data,
         xm, ym, NULL, NULL);
   if (gli)
     {  /* Add the item mouse is over to the list if NOT seleced */
        void *p = eina_list_search_unsorted(items, _item_ptr_cmp, gli);
        if (!p)
          items = eina_list_append(items, gli);
     }

   EINA_LIST_FOREACH(items, l, gli)
     {  /* Now add icons to animation window */
        Evas_Object *o = elm_object_item_part_content_get(gli,
              "elm.swallow.icon");

        if (o)
          {
             int x, y, w, h;
             const char *f, *g;
             elm_image_file_get(o, &f, &g);
             Evas_Object *ic = elm_icon_add(data);
             elm_image_file_set(ic, f, g);
             evas_object_geometry_get(o, &x, &y, &w, &h);
             evas_object_size_hint_align_set(ic,
                   EVAS_HINT_FILL, EVAS_HINT_FILL);
             evas_object_size_hint_weight_set(ic,
                   EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);

             evas_object_move(ic, x, y);
             evas_object_resize(ic, w, h);
             evas_object_show(ic);

             icons =  eina_list_append(icons, ic);
          }
     }

   eina_list_free(items);
   return icons;
}

static void
_gl_dragstart(void *data EINA_UNUSED, Evas_Object *obj)
{
   printf("<%s> <%d>\n", __func__, __LINE__);
   if (_5s_cancel)
     _5s_timeout = ecore_timer_add(5.0, _5s_timeout_gone, obj);
}

static Eina_Bool
_grid_data_getcb(Evas_Object *obj,  /* The genlist object */
      Elm_Object_Item *it,
      Elm_Drag_User_Info *info)
{  /* This called before starting to drag, mouse-down was on it */
   info->format = ELM_SEL_FORMAT_TARGETS;
   info->createicon = _gl_createicon;
   info->createdata = it;
   info->dragstart = _gl_dragstart;
   info->icons = _grid_icons_get(obj);
   info->dragdone = _gl_dragdone;

   /* Now, collect data to send for drop from ALL selected items */
   /* Save list pointer to remove items after drop and free list on done */
   info->data = _grid_get_drag_data(obj, it, (Eina_List **) &info->donecbdata);
   printf("%s - data = %s\n", __FUNCTION__, info->data);
   info->acceptdata = info->donecbdata;

   if (info->data)
     return EINA_TRUE;
   else
     return EINA_FALSE;
}

void
test_dnd_genlist_default_anim(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   char buf[PATH_MAX];
   Evas_Object *win, *gl, *bxx;
   int i, j;

   win = elm_win_util_standard_add("dnd-genlist-default-anim", "DnD-Genlist-Default-Anim");
   elm_win_autodel_set(win, EINA_TRUE);

   bxx = elm_box_add(win);
   elm_box_horizontal_set(bxx, EINA_TRUE);
   evas_object_size_hint_weight_set(bxx, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_win_resize_object_add(win, bxx);
   evas_object_show(bxx);

   itc1 = elm_genlist_item_class_new();
   itc1->item_style     = "default";
   itc1->func.text_get = gl_text_get;
   itc1->func.content_get  = gl_content_get;
   itc1->func.del       = NULL;

   for (j = 0; j < 2; j++)
     {
        gl = elm_genlist_add(win);

        /* START Drag and Drop handling */
        evas_object_smart_callback_add(win, "delete,request", _win_del, gl);
        elm_genlist_multi_select_set(gl, EINA_TRUE); /* We allow multi drag */
        elm_drop_item_container_add(gl,
              ELM_SEL_FORMAT_TARGETS,
              _gl_item_getcb,
              NULL, NULL,
              NULL, NULL,
              NULL, NULL,
              _gl_dropcb, NULL);

        elm_drag_item_container_add(gl, ANIM_TIME, DRAG_TIMEOUT,
              _gl_item_getcb, _gl_dnd_default_anim_data_getcb);

        // FIXME: This causes genlist to resize the horiz axis very slowly :(
        // Reenable this and resize the window horizontally, then try to resize it back
        //elm_genlist_mode_set(gl, ELM_LIST_LIMIT);
        evas_object_size_hint_weight_set(gl, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
        evas_object_size_hint_align_set(gl, EVAS_HINT_FILL, EVAS_HINT_FILL);
        elm_box_pack_end(bxx, gl);
        evas_object_show(gl);

        for (i = 0; i < 20; i++)
          {
             snprintf(buf, sizeof(buf), "%s/images/%s", elm_app_data_dir_get(), img[(i % 9)]);
             const char *path = eina_stringshare_add(buf);
             elm_genlist_item_append(gl, itc1, path, NULL, ELM_GENLIST_ITEM_NONE, NULL, NULL);
          }
     }

   evas_object_resize(win, 680, 800);
   evas_object_show(win);
}

void
test_dnd_genlist_user_anim(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   char buf[PATH_MAX];
   Evas_Object *win, *gl, *bxx;
   int i, j;

   win = elm_win_util_standard_add("dnd-genlist-user-anim", "DnD-Genlist-User-Anim");
   elm_win_autodel_set(win, EINA_TRUE);

   bxx = elm_box_add(win);
   elm_box_horizontal_set(bxx, EINA_TRUE);
   evas_object_size_hint_weight_set(bxx, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_win_resize_object_add(win, bxx);
   evas_object_show(bxx);

   itc1 = elm_genlist_item_class_new();
   itc1->item_style     = "default";
   itc1->func.text_get = gl_text_get;
   itc1->func.content_get  = gl_content_get;
   itc1->func.del       = NULL;

   for (j = 0; j < 2; j++)
     {
        gl = elm_genlist_add(win);

        /* START Drag and Drop handling */
        evas_object_smart_callback_add(win, "delete,request", _win_del, gl);
        elm_genlist_multi_select_set(gl, EINA_TRUE); /* We allow multi drag */
        elm_drop_item_container_add(gl,
              ELM_SEL_FORMAT_TARGETS,
              _gl_item_getcb,
              NULL, NULL,
              NULL, NULL,
              NULL, NULL,
              _gl_dropcb, NULL);

        elm_drag_item_container_add(gl, ANIM_TIME, DRAG_TIMEOUT,
              _gl_item_getcb, _gl_data_getcb);

        /* We add mouse-down, up callbacks to start/stop drag animation */
        evas_object_event_callback_add(gl, EVAS_CALLBACK_MOUSE_DOWN,
              _gl_obj_mouse_down, gl);
        /* END Drag and Drop handling */

        // FIXME: This causes genlist to resize the horiz axis very slowly :(
        // Reenable this and resize the window horizontally, then try to resize it back
        //elm_genlist_mode_set(gl, ELM_LIST_LIMIT);
        evas_object_size_hint_weight_set(gl, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
        evas_object_size_hint_align_set(gl, EVAS_HINT_FILL, EVAS_HINT_FILL);
        elm_box_pack_end(bxx, gl);
        evas_object_show(gl);

        for (i = 0; i < 20; i++)
          {
             snprintf(buf, sizeof(buf), "%s/images/%s", elm_app_data_dir_get(), img[(i % 9)]);
             const char *path = eina_stringshare_add(buf);
             elm_genlist_item_append(gl, itc1, path, NULL, ELM_GENLIST_ITEM_NONE, NULL, NULL);
          }
     }

   evas_object_resize(win, 680, 800);
   evas_object_show(win);
}

void
test_dnd_genlist_gengrid(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   char buf[PATH_MAX];
   Evas_Object *win, *bxx;
   int i;

   win = elm_win_util_standard_add("dnd-genlist-gengrid", "DnD-Genlist-Gengrid");
   elm_win_autodel_set(win, EINA_TRUE);

   bxx = elm_box_add(win);
   elm_box_horizontal_set(bxx, EINA_TRUE);
   evas_object_size_hint_weight_set(bxx, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_win_resize_object_add(win, bxx);
   evas_object_show(bxx);

     {
        itc1 = elm_genlist_item_class_new();
        itc1->item_style     = "default";
        itc1->func.text_get = gl_text_get;
        itc1->func.content_get  = gl_content_get;
        itc1->func.del       = NULL;

        Evas_Object *gl = elm_genlist_add(win);
        evas_object_smart_callback_add(win, "delete,request", _win_del, gl);

        /* START Drag and Drop handling */
        elm_genlist_multi_select_set(gl, EINA_TRUE); /* We allow multi drag */
        elm_drop_item_container_add(gl, ELM_SEL_FORMAT_TARGETS, _gl_item_getcb, NULL, NULL,
              NULL, NULL, NULL, NULL, _gl_dropcb, NULL);

        elm_drag_item_container_add(gl, ANIM_TIME, DRAG_TIMEOUT,
              _gl_item_getcb, _gl_dnd_default_anim_data_getcb);
        /* END Drag and Drop handling */

        // FIXME: This causes genlist to resize the horiz axis very slowly :(
        // Reenable this and resize the window horizontally, then try to resize it back
        //elm_genlist_mode_set(gl, ELM_LIST_LIMIT);
        evas_object_size_hint_weight_set(gl, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
        evas_object_size_hint_align_set(gl, EVAS_HINT_FILL, EVAS_HINT_FILL);
        elm_box_pack_end(bxx, gl);
        evas_object_show(gl);

        for (i = 0; i < 20; i++)
          {
             snprintf(buf, sizeof(buf), "%s/images/%s", elm_app_data_dir_get(), img[(i % 9)]);
             const char *path = eina_stringshare_add(buf);
             elm_genlist_item_append(gl, itc1, path, NULL, ELM_GENLIST_ITEM_NONE, NULL, NULL);
          }
     }

     {
        Evas_Object *grid = elm_gengrid_add(win);
        evas_object_smart_callback_add(win, "delete,request", _win_del, grid);
        elm_gengrid_item_size_set(grid,
              elm_config_scale_get() * 150,
              elm_config_scale_get() * 150);
        elm_gengrid_horizontal_set(grid, EINA_FALSE);
        elm_gengrid_reorder_mode_set(grid, EINA_FALSE);
        elm_gengrid_multi_select_set(grid, EINA_TRUE); /* We allow multi drag */
        evas_object_size_hint_weight_set(grid, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
        evas_object_size_hint_align_set(grid, EVAS_HINT_FILL, EVAS_HINT_FILL);

        gic = elm_gengrid_item_class_new();
        gic->item_style = "default";
        gic->func.text_get = gl_text_get;
        gic->func.content_get = gl_content_get;

        elm_drop_item_container_add(grid, ELM_SEL_FORMAT_TARGETS, _grid_item_getcb, NULL, NULL,
              NULL, NULL, NULL, NULL, _grid_dropcb, NULL);

        elm_drag_item_container_add(grid, ANIM_TIME, DRAG_TIMEOUT,
              _grid_item_getcb, _grid_data_getcb);
        for (i = 0; i < 20; i++)
          {
             snprintf(buf, sizeof(buf), "%s/images/%s", elm_app_data_dir_get(), img[(i % 9)]);
             const char *path = eina_stringshare_add(buf);
             elm_gengrid_item_append(grid, gic, path, NULL, NULL);
          }
        elm_box_pack_end(bxx, grid);
        evas_object_show(grid);
     }

   evas_object_resize(win, 680, 800);
   evas_object_show(win);
}

static Eina_Bool _drop_box_button_new_cb(void *data, Evas_Object *obj, Elm_Selection_Data *ev)
{
   Evas_Object *win = data;
   char *p = strchr(ev->data, '#');
   while(p)
     {
        p++;
        char *p2 = strchr(p, '#');
        if (p2)
          {
             *p2 = '\0';
             Evas_Object *ic = elm_icon_add(win);
             elm_image_file_set(ic, p, NULL);
             evas_object_size_hint_aspect_set(ic, EVAS_ASPECT_CONTROL_VERTICAL, 1, 1);
             Evas_Object *bt = elm_button_add(win);
             elm_object_text_set(bt, "Dropped button");
             elm_object_part_content_set(bt, "icon", ic);
             elm_box_pack_end(obj, bt);
             evas_object_show(bt);
             evas_object_show(ic);
             p = p2;
          }
        else p = NULL;
     }
   return EINA_TRUE;
}

void _enter_but_cb(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED)
{
   printf("Entered %s - drop it here and I will never print this line anymore.\n", __FUNCTION__);
}

static Eina_Bool _drop_but_icon_change_cb(void *data, Evas_Object *obj, Elm_Selection_Data *ev)
{
   Evas_Object *win = data;
   Evas_Object *ic = elm_icon_add(win);
   char *p = strchr(ev->data, '#');
   if (!p) return EINA_FALSE;
   p++;
   char *p2 = strchr(p, '#');
   if (!p2) return EINA_FALSE;
   *p2 = '\0';
   elm_image_file_set(ic, p, NULL);
   evas_object_size_hint_aspect_set(ic, EVAS_ASPECT_CONTROL_VERTICAL, 1, 1);
   evas_object_del(elm_object_part_content_get(obj, "icon"));
   elm_object_part_content_set(obj, "icon", ic);
   evas_object_show(ic);
   return EINA_TRUE;
}

/* Callback used to test multi-callbacks feature */
static Eina_Bool _drop_but_cb_remove_cb(void *data EINA_UNUSED, Evas_Object *obj, Elm_Selection_Data *ev EINA_UNUSED)
{
   printf("Second callback called - removing it\n");
   elm_drop_target_del(obj, ELM_SEL_FORMAT_TARGETS, _enter_but_cb, NULL, NULL, NULL, NULL, NULL, _drop_but_cb_remove_cb, NULL);
   return EINA_TRUE;
}

static Eina_Bool _drop_bg_change_cb(void *data EINA_UNUSED, Evas_Object *obj, Elm_Selection_Data *ev)
{
   char *p = strchr(ev->data, '#');
   if (!p) return EINA_FALSE;
   p++;
   char *p2 = strchr(p, '#');
   if (!p2) return EINA_FALSE;
   *p2 = '\0';
   elm_bg_file_set(obj, p, NULL);
   return EINA_TRUE;
}

static void
_5s_cancel_ck_changed(void *data EINA_UNUSED, Evas_Object *obj, void *event_info EINA_UNUSED)
{
   _5s_cancel = elm_check_state_get(obj);
}

void
test_dnd_multi_features(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   char buf[PATH_MAX];
   Evas_Object *win, *bxx, *bg;
   int i;

   win = elm_win_util_standard_add("dnd-multi-features", "DnD-Multi Features");
   elm_win_autodel_set(win, EINA_TRUE);

   bg = elm_bg_add(win);
   evas_object_size_hint_weight_set(bg, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_drop_target_add(bg, ELM_SEL_FORMAT_TARGETS, NULL, NULL, NULL, NULL, NULL, NULL, _drop_bg_change_cb, NULL);
   elm_win_resize_object_add(win, bg);

   /* And show the background. */
   evas_object_show(bg);
   bxx = elm_box_add(win);
   elm_box_horizontal_set(bxx, EINA_TRUE);
   evas_object_size_hint_weight_set(bxx, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_win_resize_object_add(win, bxx);
   evas_object_show(bxx);

     {
        Evas_Object *grid = elm_gengrid_add(bxx);
        evas_object_smart_callback_add(win, "delete,request", _win_del, grid);
        elm_gengrid_item_size_set(grid,
              elm_config_scale_get() * 100,
              elm_config_scale_get() * 100);
        elm_gengrid_horizontal_set(grid, EINA_FALSE);
        elm_gengrid_reorder_mode_set(grid, EINA_FALSE);
        elm_gengrid_multi_select_set(grid, EINA_TRUE); /* We allow multi drag */
        evas_object_size_hint_weight_set(grid, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
        evas_object_size_hint_align_set(grid, EVAS_HINT_FILL, EVAS_HINT_FILL);

        gic = elm_gengrid_item_class_new();
        gic->item_style = "default";
        gic->func.text_get = gl_text_get;
        gic->func.content_get = gl_content_get;

        elm_drag_item_container_add(grid, ANIM_TIME, DRAG_TIMEOUT,
              _grid_item_getcb, _grid_data_getcb);
        for (i = 0; i < 10; i++)
          {
             snprintf(buf, sizeof(buf), "%s/images/%s", elm_app_data_dir_get(), img[(i % 9)]);
             const char *path = eina_stringshare_add(buf);
             Elm_Object_Item *item = elm_gengrid_item_append(grid, gic, path, NULL, NULL);
             elm_object_item_part_text_set(item, "elm.text", path);
          }
        elm_box_pack_end(bxx, grid);
        evas_object_show(grid);
     }

     {
        Evas_Object *ic, *bt;
        Evas_Object *vert_box = elm_box_add(bxx);
        evas_object_size_hint_weight_set(vert_box, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
        elm_box_pack_end(bxx, vert_box);
        evas_object_show(vert_box);
        elm_drop_target_add(vert_box, ELM_SEL_FORMAT_TARGETS, NULL, NULL, NULL, NULL, NULL, NULL, _drop_box_button_new_cb, win);

        _5s_cancel = EINA_FALSE;
        Evas_Object *ck = elm_check_add(vert_box);
        elm_object_style_set(ck, "toggle");
        elm_object_text_set(ck, "Cancel after 5s:");
        elm_check_state_set(ck, _5s_cancel);
        evas_object_smart_callback_add(ck, "changed", _5s_cancel_ck_changed, NULL);
        elm_box_pack_end(vert_box, ck);
        evas_object_show(ck);

        ic = elm_icon_add(win);
        snprintf(buf, sizeof(buf), "%s/images/logo_small.png", elm_app_data_dir_get());
        elm_image_file_set(ic, buf, NULL);
        evas_object_size_hint_aspect_set(ic, EVAS_ASPECT_CONTROL_VERTICAL, 1, 1);
        bt = elm_button_add(win);
        elm_object_text_set(bt, "Multi-callbacks check");
        elm_drop_target_add(bt, ELM_SEL_FORMAT_TARGETS, NULL, NULL, NULL, NULL, NULL, NULL, _drop_but_icon_change_cb, win);
        elm_drop_target_add(bt, ELM_SEL_FORMAT_TARGETS, _enter_but_cb, NULL, NULL, NULL, NULL, NULL, _drop_but_cb_remove_cb, NULL);
        elm_object_part_content_set(bt, "icon", ic);
        elm_box_pack_end(vert_box, bt);
        evas_object_show(bt);
        evas_object_show(ic);

        ic = elm_icon_add(win);
        snprintf(buf, sizeof(buf), "%s/images/logo_small.png", elm_app_data_dir_get());
        elm_image_file_set(ic, buf, NULL);
        evas_object_size_hint_aspect_set(ic, EVAS_ASPECT_CONTROL_VERTICAL, 1, 1);
        bt = elm_button_add(win);
        elm_object_text_set(bt, "Drop into me to change my icon");
        elm_drop_target_add(bt, ELM_SEL_FORMAT_TARGETS, NULL, NULL, NULL, NULL, NULL, NULL, _drop_but_icon_change_cb, win);
        elm_object_part_content_set(bt, "icon", ic);
        elm_box_pack_end(vert_box, bt);
        evas_object_show(bt);
        evas_object_show(ic);

        ic = elm_icon_add(win);
        snprintf(buf, sizeof(buf), "%s/images/logo_small.png", elm_app_data_dir_get());
        elm_image_file_set(ic, buf, NULL);
        evas_object_size_hint_aspect_set(ic, EVAS_ASPECT_CONTROL_VERTICAL, 1, 1);
        bt = elm_button_add(win);
        elm_object_text_set(bt, "No action on drop");
        elm_object_part_content_set(bt, "icon", ic);
        elm_box_pack_end(vert_box, bt);
        evas_object_show(bt);
        evas_object_show(ic);
     }
   evas_object_resize(win, 680, 800);
   evas_object_show(win);
}

