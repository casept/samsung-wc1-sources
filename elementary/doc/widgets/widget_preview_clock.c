#include "widget_preview_tmpl_head.c"

Evas_Object *o = elm_clock_add(win);
evas_object_size_hint_weight_set(o, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
elm_win_resize_object_add(win, o);
evas_object_show(o);

#include "widget_preview_tmpl_foot.c"
