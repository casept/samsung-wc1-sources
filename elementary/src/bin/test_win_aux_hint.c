#ifdef HAVE_CONFIG_H
# include "elementary_config.h"
#endif
#include <Elementary.h>
#ifndef ELM_LIB_QUICKLAUNCH

typedef struct _App_Data App_Data;

struct _App_Data
{
   int          id;
   Evas_Object *lb;
};

static void
_win_del_cb(void *data __UNUSED__, Evas_Object *obj, void *event_info __UNUSED__)
{
   App_Data *ad = evas_object_data_get(obj, "ad");
   free(ad);
}

static void
_win_aux_hint_allowed_cb(void *data, Evas_Object *obj __UNUSED__, void *event_info)
{
   Evas_Object *win = (Evas_Object *)(data);
   App_Data *ad = evas_object_data_get(win, "ad");
   const char *str;

   if ((long)event_info == ad->id)
     {
        str = eina_stringshare_printf("aux_hint_allowed_cb. ID:%ld", (long)event_info);
        elm_object_text_set(ad->lb, str);
        eina_stringshare_del(str);

        /* do something */
        ;;
     }
}

void
test_win_aux_hint(void *data __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   Evas_Object *win, *bx, *lb;
   App_Data *ad;

   if (!(ad = calloc(1, sizeof(App_Data)))) return;

   win = elm_win_util_standard_add("auxhint", "AuxHint");
   elm_win_autodel_set(win, EINA_TRUE);
   evas_object_data_set(win, "ad", ad);

   bx = elm_box_add(win);
   evas_object_size_hint_weight_set(bx, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_win_resize_object_add(win, bx);

   lb = elm_label_add(win);
   elm_object_text_set(lb, "<b>Window auxiliary hint test</b>");
   evas_object_size_hint_weight_set(lb, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_box_pack_end(bx, lb);
   evas_object_show(lb);

   lb = elm_label_add(win);
   elm_object_text_set(lb, "Supported auxiliary hints");
   evas_object_size_hint_weight_set(lb, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_box_pack_end(bx, lb);
   evas_object_show(lb);

   if (elm_win_wm_rotation_supported_get(win))
     {
        int rots[] = { 0, 90, 280, 270 };
        elm_win_wm_rotation_available_rotations_set(win, rots, (sizeof(rots) / sizeof(int)));
     }

   const Eina_List *l, *ll;
   int id = -1, i = 0;
   char *hint;

   l = elm_win_aux_hints_supported_get(win);
   EINA_LIST_FOREACH(l, ll, hint)
     {
        Eina_Strbuf *buf = eina_strbuf_new();
        eina_strbuf_append_printf(buf, "%02d %s", i++, hint);

        if (!strncmp(hint, "wm.comp.win.effect.enable", strlen("wm.comp.win.effect.enable")))
          {
             id = elm_win_aux_hint_add(win, "wm.comp.win.effect.enable", "0");
             if (id != -1)
               {
                  eina_strbuf_append(buf, "<- Added");
                  ad->id = id;
               }
          }

        lb = elm_label_add(win);
        elm_object_text_set(lb, eina_strbuf_string_get(buf));
        evas_object_size_hint_weight_set(lb, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
        elm_box_pack_end(bx, lb);
        evas_object_show(lb);

        eina_strbuf_free(buf);
     }

   evas_object_smart_callback_add(win, "aux,hint,allowed", _win_aux_hint_allowed_cb, win);


   lb = elm_label_add(win);
   elm_object_text_set(lb, "N/A");
   evas_object_size_hint_weight_set(lb, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_box_pack_end(bx, lb);
   evas_object_show(lb);
   ad->lb = lb;

   evas_object_smart_callback_add(win, "delete,request", _win_del_cb, NULL);

   evas_object_resize(win, 480, 400);
   evas_object_show(bx);
   evas_object_show(win);
}
#endif
