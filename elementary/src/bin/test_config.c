#ifdef HAVE_CONFIG_H
# include "elementary_config.h"
#endif
#include <Elementary.h>
#ifndef ELM_LIB_QUICKLAUNCH
//
//typedef struct _Prof_Data Prof_Data;
//
//struct _Prof_Data
//{
//   Evas_Object *rdg;
//   Eina_List   *profiles;
//};
//
//static void
//_bt_clicked_cb(void *data, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
//{
//   Evas_Object *win = (Evas_Object *)(data);
//   Prof_Data *pd = evas_object_data_get(win, "pd");
//   Evas_Object *rd = elm_radio_selected_object_get(pd->rdg);
//   const char *str = elm_object_text_get(rd);
//
//   elm_win_profiles_set(win, &str, 1);
//}
//
//static void
//_win_profile_changed_cb(void *data, Evas_Object *obj __UNUSED__, void *event __UNUSED__)
//{
//   Evas_Object *win = (Evas_Object *)(data);
//   Prof_Data *pd = evas_object_data_get(win, "pd");
//   const char *profile = elm_win_profile_get(win);
//
//   const char *str;
//   int num, i = 0;
//   Eina_List *l;
//
//   EINA_LIST_FOREACH(pd->profiles, l, str)
//     {
//        if ((str) && (profile) &&
//            (strcmp(str, profile) == 0))
//          {
//             break;
//          }
//        i++;
//     }
//
//   num = eina_list_count(pd->profiles);
//   if (i >= num) i = num - 1;
//   if (i < 0) i = 0;
//
//   elm_radio_value_set(pd->rdg, i);
//}
//
//void
//test_config(void *data __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
//{
//   Prof_Data *pd;
//   Evas_Object *win, *bx, *lb, *bt, *rd, *rdg = NULL;
//   const char *str;
//   const char **plist;
//   Eina_List *profs, *l;
//   int num, i = 0;
//
//   if (!(pd = calloc(1, sizeof(Prof_Data)))) return;
//
//   win = elm_win_util_standard_add("config", "Configuration");
//   elm_win_autodel_set(win, EINA_TRUE);
//   evas_object_data_set(win, "pd", pd);
//
//   bx = elm_box_add(win);
//   evas_object_size_hint_weight_set(bx, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
//   elm_win_resize_object_add(win, bx);
//
//   lb = elm_label_add(win);
//   elm_object_text_set(lb, "<b>List of Profiles</b>");
//   evas_object_size_hint_weight_set(lb, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
//   elm_box_pack_end(bx, lb);
//   evas_object_show(lb);
//
//   profs = elm_config_profile_list_get();
//   num = eina_list_count(profs);
//   plist = calloc(num, sizeof(char *));
//
//   EINA_LIST_FOREACH(profs, l, str)
//     {
//        rd = elm_radio_add(win);
//        if (!rdg)
//          {
//             rdg = rd;
//             pd->rdg = rdg;
//          }
//        plist[i] = strdup(str);
//        pd->profiles = eina_list_append(pd->profiles,
//                                        eina_stringshare_add(str));
//        elm_radio_state_value_set(rd, i);
//        elm_radio_group_add(rd, rdg);
//        elm_object_text_set(rd, str);
//        evas_object_size_hint_weight_set(rd, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
//        elm_box_pack_end(bx, rd);
//        evas_object_show(rd);
//        i++;
//     }
//   elm_config_profile_list_free(profs);
//
//   bt = elm_button_add(win);
//   elm_object_text_set(bt, "Change Window Profile");
//   evas_object_smart_callback_add(bt, "clicked", _bt_clicked_cb, win);
//   elm_box_pack_end(bx, bt);
//   evas_object_show(bt);
//
//   elm_win_profiles_set(win, plist, num);
//   evas_object_smart_callback_add(win, "profile,changed", _win_profile_changed_cb, win);
//
//   evas_object_resize(win, 480, 400);
//   evas_object_show(bx);
//   evas_object_show(win);
//
//   free(plist);
//}
#endif
