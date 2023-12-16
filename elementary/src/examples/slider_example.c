/**
 * Simple Elementary's <b>slider widget</b> example, illustrating its
 * usage and API.
 *
 * See stdout/stderr for output. Compile with:
 *
 * @verbatim
 * gcc -g slider_example.c -o slider_example `pkg-config --cflags --libs elementary`
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
_changed_cb(void *data, Evas_Object *obj, void *event_info)
{
    double val = elm_slider_value_get(obj);
    printf("Changed to %1.2f\n", val);
}

static void
_delay_changed_cb(void *data, Evas_Object *obj, void *event_info)
{
    double val = elm_slider_value_get(obj);
    printf("Delay changed to %1.2f\n", val);
}

static char*
_indicator_format(double val)
{
   char *indicator = malloc(sizeof(char) * 32);
   snprintf(indicator, 32, "%1.2f u", val);
   return indicator;
}

static void
_indicator_free(char *str)
{
   free(str);
}

EAPI_MAIN int
elm_main(int argc, char **argv)
{
   Evas_Object *win, *bg, *bx, *sl, *ic;

   win = elm_win_add(NULL, "slider", ELM_WIN_BASIC);
   elm_win_title_set(win, "Slider Example");
   evas_object_smart_callback_add(win, "delete,request", _on_done, NULL);

   bg = elm_bg_add(win);
   elm_win_resize_object_add(win, bg);
   evas_object_size_hint_weight_set(bg, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_show(bg);

   bx = elm_box_add(win);
   evas_object_size_hint_weight_set(bx, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_win_resize_object_add(win, bx);
   evas_object_show(bx);

   /* default */
   sl = elm_slider_add(win);
   evas_object_size_hint_align_set(sl, EVAS_HINT_FILL, 0.5);
   evas_object_size_hint_weight_set(sl, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_box_pack_end(bx, sl);
   evas_object_show(sl);

   /* with icon, end and label */
   sl = elm_slider_add(win);
   elm_object_text_set(sl, "Counter");

   ic = elm_icon_add(win);
   elm_icon_standard_set(ic, "home");
   elm_image_resizable_set(ic, EINA_FALSE, EINA_FALSE);
   elm_object_part_content_set(sl, "icon", ic);

   ic = elm_icon_add(win);
   elm_icon_standard_set(ic, "folder");
   elm_image_resizable_set(ic, EINA_FALSE, EINA_FALSE);
   elm_object_part_content_set(sl, "end", ic);

   evas_object_size_hint_align_set(sl, EVAS_HINT_FILL, 0.5);
   evas_object_size_hint_weight_set(sl, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_box_pack_end(bx, sl);
   evas_object_show(sl);

   /* value set and span size */
   sl = elm_slider_add(win);
   elm_slider_value_set(sl, 1);
   elm_slider_span_size_set(sl, 200);
   evas_object_size_hint_align_set(sl, EVAS_HINT_FILL, 0.5);
   evas_object_size_hint_weight_set(sl, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_box_pack_end(bx, sl);
   evas_object_show(sl);

   /* with unit label and min - max */
   sl = elm_slider_add(win);
   elm_slider_unit_format_set(sl, "%1.0f units");
   elm_slider_min_max_set(sl, 0, 100);
   evas_object_size_hint_align_set(sl, EVAS_HINT_FILL, 0.5);
   evas_object_size_hint_weight_set(sl, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_box_pack_end(bx, sl);
   evas_object_show(sl);

   /* with indicator label and inverted */
   sl = elm_slider_add(win);
   elm_slider_indicator_format_set(sl, "%1.2f");
   elm_slider_inverted_set(sl, EINA_TRUE);
   evas_object_size_hint_align_set(sl, EVAS_HINT_FILL, 0.5);
   evas_object_size_hint_weight_set(sl, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_box_pack_end(bx, sl);
   evas_object_show(sl);

   /* vertical with indicator format func */
   sl = elm_slider_add(win);
   elm_slider_horizontal_set(sl, EINA_FALSE);
   elm_slider_indicator_format_function_set(sl, _indicator_format,
                                            _indicator_free);
   evas_object_size_hint_align_set(sl, 0.5, EVAS_HINT_FILL);
   evas_object_size_hint_weight_set(sl, 0, EVAS_HINT_EXPAND);
   elm_box_pack_end(bx, sl);
   evas_object_show(sl);

   /* callbacks */
   sl = elm_slider_add(win);
   elm_slider_unit_format_set(sl, "%1.3f units");
   evas_object_size_hint_align_set(sl, EVAS_HINT_FILL, 0.5);
   evas_object_size_hint_weight_set(sl, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_box_pack_end(bx, sl);
   evas_object_show(sl);
   evas_object_smart_callback_add(sl, "changed", _changed_cb, NULL);
   evas_object_smart_callback_add(sl, "delay,changed", _delay_changed_cb, NULL);

   evas_object_show(win);

   elm_run();
   elm_shutdown();

   return 0;
}
ELM_MAIN()
