/**
 * Simple Elementary's <b>toolbar widget</b> example, illustrating its
 * usage and API.
 *
 * See stdout/stderr for output. Compile with:
 *
 * @verbatim
 * gcc -g toolbar_example_01.c -o toolbar_example_01 `pkg-config --cflags --libs elementary`
 * @endverbatim
 */

#include <Elementary.h>

static void
_on_done(void *data,
         Evas_Object *obj,
         void *event_info)
{
   elm_exit();
}

static void
_item_1_pressed(void *data, Evas_Object *obj, void *event_info)
{
   Evas_Object *bt = data;
   elm_object_text_set(bt, "Print Document");
}

static void
_item_2_pressed(void *data, Evas_Object *obj, void *event_info)
{
   Evas_Object *bt = data;
   elm_object_text_set(bt, "Create new folder");
}

static void
_item_3_pressed(void *data, Evas_Object *obj, void *event_info)
{
   /* This function won't be called because we disabled item that call it */
   Evas_Object *bt = data;
   elm_object_text_set(bt, "Create and send email");
}

EAPI_MAIN int
elm_main(int argc, char **argv)
{
   Evas_Object *win, *bg, *bx, *tb, *bt;
   Elm_Object_Item *tb_it;

   win = elm_win_add(NULL, "toolbar", ELM_WIN_BASIC);
   elm_win_title_set(win, "Toolbar Example");
   evas_object_smart_callback_add(win, "delete,request", _on_done, NULL);

   bg = elm_bg_add(win);
   elm_win_resize_object_add(win, bg);
   evas_object_size_hint_weight_set(bg, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_show(bg);

   bx = elm_box_add(win);
   elm_win_resize_object_add(win, bx);
   evas_object_size_hint_weight_set(bx, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(bx, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_show(bx);

   bt = elm_button_add(win);
   evas_object_size_hint_weight_set(bt, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(bt, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_object_text_set(bt, "Button");
   evas_object_show(bt);

   tb = elm_toolbar_add(win);
   elm_toolbar_shrink_mode_set(tb, ELM_TOOLBAR_SHRINK_SCROLL);
   evas_object_size_hint_weight_set(tb, 0.0, 0.0);
   evas_object_size_hint_align_set(tb, EVAS_HINT_FILL, 0.0);
   evas_object_show(tb);

   elm_toolbar_item_append(tb, "document-print", "Print", _item_1_pressed, bt);
   elm_toolbar_item_append(tb, "folder-new", "New Folder", _item_2_pressed, bt);

   tb_it = elm_toolbar_item_append(tb, "mail-send", "Create and send email",
                                  _item_3_pressed, bt);
   elm_object_item_disabled_set(tb_it, EINA_TRUE);

   elm_toolbar_homogeneous_set(tb, EINA_FALSE);

   elm_box_pack_end(bx, tb);
   elm_box_pack_end(bx, bt);

   evas_object_resize(win, 200, 200);
   evas_object_show(win);

   elm_run();
   elm_shutdown();

   return 0;
}
ELM_MAIN()
