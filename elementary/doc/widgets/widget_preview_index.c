#include "widget_preview_tmpl_head.c"

static const char *dict[] = \
{
   "awkward",
   "businessman",
   "issuing",
   "just",
   "knows",
   "putting",
   "systematically",
   "tyranny",
   "xenophobia",
   "yacht",
   "zebra"
};

Evas_Object *o = elm_list_add(win);
evas_object_size_hint_weight_set(o, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
elm_win_resize_object_add(win, o);
evas_object_show(o);

Evas_Object *id = elm_index_add(win);
evas_object_size_hint_weight_set(id, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
elm_win_resize_object_add(win, id);
evas_object_show(id);

unsigned int i;
for (i = 0; i < (sizeof(dict) / sizeof(dict[0])); i++)
  {
     char buf[32];
     Elm_Object_Item *list_it;

     list_it = elm_list_item_append(o, dict[i], NULL, NULL, NULL, NULL);
     snprintf(buf, sizeof(buf), "%c", dict[i][0]);
     elm_index_item_append(id, buf, NULL, list_it);
  }

elm_index_autohide_disabled_set(id, EINA_FALSE);

#include "widget_preview_tmpl_foot.c"
