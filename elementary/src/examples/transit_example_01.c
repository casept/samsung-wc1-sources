//Compile with:
//gcc -o transit_example_01 transit_example_01.c `pkg-config --cflags --libs elementary`

#include <Elementary.h>

static void
on_done(void *data, Evas_Object *obj, void *event_info)
{
   /* quit the mainloop (elm_run) */
   elm_exit();
}

EAPI_MAIN int
elm_main(int argc, char **argv)
{
   Evas_Object *win, *bg, *bt;
   Elm_Transit *trans;

   win = elm_win_add(NULL, "transit-basic", ELM_WIN_BASIC);
   elm_win_title_set(win, "Transit - Basic");
   evas_object_smart_callback_add(win, "delete,request", on_done, NULL);
   elm_win_autodel_set(win, EINA_TRUE);
   elm_policy_set(ELM_POLICY_QUIT, ELM_POLICY_QUIT_LAST_WINDOW_CLOSED);
   evas_object_resize(win, 400, 400);

   bg = elm_bg_add(win);
   evas_object_size_hint_weight_set(bg, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_win_resize_object_add(win, bg);
   evas_object_show(bg);

   bt = elm_button_add(win);
   elm_object_text_set(bt, "Resizing Effect");
   evas_object_show(bt);
   evas_object_move(bt, 50, 100);
   evas_object_resize(bt, 100, 50);

   evas_object_show(win);

   trans = elm_transit_add();
   elm_transit_object_add(trans, bt);

   elm_transit_effect_resizing_add(trans, 100, 50, 300, 150);

   elm_transit_duration_set(trans, 5.0);
   elm_transit_go(trans);

   elm_run();
   elm_shutdown();

   return 0;
}
ELM_MAIN()
