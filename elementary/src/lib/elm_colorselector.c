#include <Elementary.h>
#include "elm_priv.h"
#include "elm_widget_colorselector.h"

EAPI const char ELM_COLORSELECTOR_SMART_NAME[] = "elm_colorselector";

#define BASE_STEP       360.0
#define HUE_STEP        360.0
#define SAT_STEP        128.0
#define LIG_STEP        256.0
#define ALP_STEP        256.0
#define COLORPLANE_STEP 0.01
#define DEFAULT_HOR_PAD 10
#define DEFAULT_VER_PAD 10
#define PI 3.14
#define PI_DEGREE 180.0

static const char SIG_CHANGED[] = "changed";
static const char SIG_COLOR_ITEM_SELECTED[] = "color,item,selected";
static const char SIG_COLOR_ITEM_LONGPRESSED[] = "color,item,longpressed";
static const Evas_Smart_Cb_Description _smart_callbacks[] =
{
   {SIG_COLOR_ITEM_SELECTED, ""},
   {SIG_COLOR_ITEM_LONGPRESSED, ""},
   {SIG_CHANGED, ""},
   {NULL, NULL}
};

enum Palette_Box_Direction
{
   PALETTE_BOX_UP,
   PALETTE_BOX_DOWN
};

static void _colorplane_color_get(Evas_Object *obj, int *r, int *g, int *b, int *a);
static void _colorplane_color_set(Evas_Object *obj, int r, int g, int b, int a);
static void _colorcircle_color_get(Evas_Object *obj, int *r, int *g, int *b, int *a);
static void _colorcircle_color_set(Evas_Object *obj, int r, int g, int b, int a);
static void _colors_save(Evas_Object *obj);
static void _colors_remove(Evas_Object *obj);

EVAS_SMART_SUBCLASS_NEW
  (ELM_COLORSELECTOR_SMART_NAME, _elm_colorselector,
  Elm_Colorselector_Smart_Class, Elm_Layout_Smart_Class,
  elm_layout_smart_class_get, _smart_callbacks);

static void
_items_del(Elm_Colorselector_Smart_Data *sd)
{
   Elm_Color_Item *item;

   if (!sd->items) return;

   EINA_LIST_FREE (sd->items, item)
     {
        eina_stringshare_del(item->color->color_name);
        free(item->color);
        elm_widget_item_free(item);
     }

   sd->items = NULL;
   sd->selected = NULL;
   sd->highlighted = NULL;
}

static void
_color_with_saturation(Elm_Colorselector_Smart_Data *sd)
{
   if (sd->er > 127)
     sd->sr = 127 + (int)((double)(sd->er - 127) * sd->s);
   else
     sd->sr = 127 - (int)((double)(127 - sd->er) * sd->s);

   if (sd->eg > 127)
     sd->sg = 127 + (int)((double)(sd->eg - 127) * sd->s);
   else
     sd->sg = 127 - (int)((double)(127 - sd->eg) * sd->s);

   if (sd->eb > 127)
     sd->sb = 127 + (int)((double)(sd->eb - 127) * sd->s);
   else
     sd->sb = 127 - (int)((double)(127 - sd->eb) * sd->s);
}

static void
_color_with_lightness(Elm_Colorselector_Smart_Data *sd)
{
   if (sd->l > 0.5)
     {
        sd->lr = sd->er + (int)((double)(255 - sd->er) * (sd->l - 0.5) * 2.0);
        sd->lg = sd->eg + (int)((double)(255 - sd->eg) * (sd->l - 0.5) * 2.0);
        sd->lb = sd->eb + (int)((double)(255 - sd->eb) * (sd->l - 0.5) * 2.0);
     }
   else if (sd->l < 0.5)
     {
        sd->lr = (double)sd->er * sd->l * 2.0;
        sd->lg = (double)sd->eg * sd->l * 2.0;
        sd->lb = (double)sd->eb * sd->l * 2.0;
     }
   else
     {
        sd->lr = sd->er;
        sd->lg = sd->eg;
        sd->lb = sd->eb;
     }
}

static void
_color_picker_init(Elm_Colorselector_Smart_Data *sd)
{
   char buf[12];
   unsigned int *pixels;
   unsigned int *copy;
   int color;
   int x, y, w, h;

   snprintf(buf, 12, "%i", sd->r);
   elm_object_text_set(sd->entries[0], buf);

   snprintf(buf, 12, "%i", sd->g);
   elm_object_text_set(sd->entries[1], buf);

   snprintf(buf, 12, "%i", sd->b);
   elm_object_text_set(sd->entries[2], buf);

   snprintf(buf, 12, "%i", sd->a);
   elm_object_text_set(sd->entries[3], buf);

   color = (sd->a << 24) + (sd->r << 16) + (sd->g << 8) + sd->b;

   if (sd->b == 255)
     evas_object_image_alpha_set(sd->picker_display, EINA_FALSE);
   else
     evas_object_image_alpha_set(sd->picker_display, EINA_TRUE);

   pixels = evas_object_image_data_get(sd->picker_display, EINA_TRUE);
   copy = pixels;
   for (y = 0; y < 17; y++)
     for (x = 0; x < 17; x++)
       {
          *(pixels++) = color;
       }
   evas_object_image_data_set(sd->picker_display, copy);
   evas_object_geometry_get(sd->picker_display, NULL, NULL, &w, &h);
   evas_object_image_data_update_add(sd->picker_display, 0, 0, w, h);
}


static void
_rgb_to_hsl(Elm_Colorselector_Smart_Data *sd)
{
   double r2, g2, b2;
   double v, m, vm;
   double r, g, b;

   r = sd->r;
   g = sd->g;
   b = sd->b;

   r /= 255.0;
   g /= 255.0;
   b /= 255.0;

   v = (r > g) ? r : g;
   v = (v > b) ? v : b;

   m = (r < g) ? r : g;
   m = (m < b) ? m : b;

   sd->h = 0.0;
   sd->s = 0.0;
   sd->l = 0.0;

   sd->l = (m + v) / 2.0;

   if (sd->l <= 0.0) return;

   vm = v - m;
   sd->s = vm;

   if (sd->s > 0.0) sd->s /= (sd->l <= 0.5) ? (v + m) : (2.0 - v - m);
   else return;

   r2 = (v - r) / vm;
   g2 = (v - g) / vm;
   b2 = (v - b) / vm;

   if (r == v) sd->h = (g == m ? 5.0 + b2 : 1.0 - g2);
   else if (g == v)
     sd->h = (b == m ? 1.0 + r2 : 3.0 - b2);
   else sd->h = (r == m ? 3.0 + g2 : 5.0 - r2);

   sd->h *= 60.0;
}

static void
_hsl_to_rgb(Elm_Colorselector_Smart_Data *sd)
{
   double sv, vsf, f, p, q, t, v;
   double r = 0, g = 0, b = 0;
   double _h, _s, _l;
   int i = 0;

   _h = sd->h;
   _s = sd->s;
   _l = sd->l;

   if (_s == 0.0) r = g = b = _l;
   else
     {
        if (_h == 360.0) _h = 0.0;
        _h /= 60.0;

        v = (_l <= 0.5) ? (_l * (1.0 + _s)) : (_l + _s - (_l * _s));
        p = _l + _l - v;

        if (v) sv = (v - p) / v;
        else sv = 0;

        i = (int)_h;
        f = _h - i;

        vsf = v * sv * f;

        t = p + vsf;
        q = v - vsf;

        switch (i)
          {
           case 0:
             r = v;
             g = t;
             b = p;
             break;

           case 1:
             r = q;
             g = v;
             b = p;
             break;

           case 2:
             r = p;
             g = v;
             b = t;
             break;

           case 3:
             r = p;
             g = q;
             b = v;
             break;

           case 4:
             r = t;
             g = p;
             b = v;
             break;

           case 5:
             r = v;
             g = p;
             b = q;
             break;
          }
     }
   i = (int)(r * 255.0);
   f = (r * 255.0) - i;
   sd->r = (f <= 0.5) ? i : (i + 1);

   i = (int)(g * 255.0);
   f = (g * 255.0) - i;
   sd->g = (f <= 0.5) ? i : (i + 1);

   i = (int)(b * 255.0);
   f = (b * 255.0) - i;
   sd->b = (f <= 0.5) ? i : (i + 1);
}

static void
_rectangles_redraw(Color_Bar_Data *cb_data, double x)
{
   double one_six = 1.0 / 6.0;

   ELM_COLORSELECTOR_DATA_GET(cb_data->parent, sd);

   switch (cb_data->color_type)
     {
      case HUE:
        sd->h = 360.0 * x;

        if (x < one_six)
          {
             sd->er = 255;
             sd->eg = (255.0 * x * 6.0);
             sd->eb = 0;
          }
        else if (x < 2 * one_six)
          {
             sd->er = 255 - (int)(255.0 * (x - one_six) * 6.0);
             sd->eg = 255;
             sd->eb = 0;
          }
        else if (x < 3 * one_six)
          {
             sd->er = 0;
             sd->eg = 255;
             sd->eb = (int)(255.0 * (x - (2.0 * one_six)) * 6.0);
          }
        else if (x < 4 * one_six)
          {
             sd->er = 0;
             sd->eg = 255 - (int)(255.0 * (x - (3.0 * one_six)) * 6.0);
             sd->eb = 255;
          }
        else if (x < 5 * one_six)
          {
             sd->er = 255.0 * (x - (4.0 * one_six)) * 6.0;
             sd->eg = 0;
             sd->eb = 255;
          }
        else
          {
             sd->er = 255;
             sd->eg = 0;
             sd->eb = 255 - (int)(255.0 * (x - (5.0 * one_six)) * 6.0);
          }

        evas_object_color_set
          (sd->cb_data[0]->arrow, sd->er, sd->eg, sd->eb, 255);
        evas_object_color_set
          (sd->cb_data[1]->bg_rect, sd->er, sd->eg, sd->eb, 255);
        evas_object_color_set
          (sd->cb_data[2]->bg_rect, sd->er, sd->eg, sd->eb, 255);
        evas_object_color_set
          (sd->cb_data[3]->bar, sd->er, sd->eg, sd->eb, 255);

        _color_with_saturation(sd);
        evas_object_color_set
          (sd->cb_data[1]->arrow, sd->sr, sd->sg, sd->sb, 255);

        _color_with_lightness(sd);
        evas_object_color_set
          (sd->cb_data[2]->arrow, sd->lr, sd->lg, sd->lb, 255);

        evas_object_color_set(sd->cb_data[3]->arrow,
                              (sd->er * sd->a) / 255,
                              (sd->eg * sd->a) / 255,
                              (sd->eb * sd->a) / 255,
                              sd->a);
        break;

      case SATURATION:
        sd->s = 1.0 - x;
        _color_with_saturation(sd);
        evas_object_color_set
          (sd->cb_data[1]->arrow, sd->sr, sd->sg, sd->sb, 255);
        break;

      case LIGHTNESS:
        sd->l = x;
        _color_with_lightness(sd);
        evas_object_color_set
          (sd->cb_data[2]->arrow, sd->lr, sd->lg, sd->lb, 255);
        break;

      case ALPHA:
        sd->a = 255.0 * x;
        evas_object_color_set(sd->cb_data[3]->arrow,
                              (sd->er * sd->a) / 255,
                              (sd->eg * sd->a) / 255,
                              (sd->eb * sd->a) / 255,
                              sd->a);
        break;

      default:
        break;
     }

   _hsl_to_rgb(sd);
   if ((sd->mode == ELM_COLORSELECTOR_PICKER) || (sd->mode == ELM_COLORSELECTOR_ALL))
     _color_picker_init(sd);
}

static void
_colors_set(Evas_Object *obj,
            int r,
            int g,
            int b,
            int a)
{
   double x, y;

   ELM_COLORSELECTOR_DATA_GET(obj, sd);

   sd->r = r;
   sd->g = g;
   sd->b = b;
   sd->a = a;

   _rgb_to_hsl(sd);

   edje_object_part_drag_value_get
     (sd->cb_data[0]->colorbar, "elm.arrow", &x, &y);
   x = sd->h / 360.0;
   edje_object_part_drag_value_set
     (sd->cb_data[0]->colorbar, "elm.arrow", x, y);
   _rectangles_redraw(sd->cb_data[0], x);

   edje_object_part_drag_value_get
     (sd->cb_data[1]->colorbar, "elm.arrow", &x, &y);
   x = 1.0 - sd->s;
   edje_object_part_drag_value_set
     (sd->cb_data[1]->colorbar, "elm.arrow", x, y);
   _rectangles_redraw(sd->cb_data[1], x);

   edje_object_part_drag_value_get
     (sd->cb_data[2]->colorbar, "elm.arrow", &x, &y);
   x = sd->l;
   edje_object_part_drag_value_set(sd->cb_data[2]->colorbar, "elm.arrow", x, y);
   _rectangles_redraw(sd->cb_data[2], x);

   edje_object_part_drag_value_get
     (sd->cb_data[3]->colorbar, "elm.arrow", &x, &y);
   x = sd->a / 255.0;
   edje_object_part_drag_value_set
     (sd->cb_data[3]->colorbar, "elm.arrow", x, y);

   _rectangles_redraw(sd->cb_data[3], x);
}

static void
_entry_changed_cb(void *data,
                  Evas_Object *obj,
                  void *event_info __UNUSED__)
{
   Elm_Colorselector_Smart_Data *sd = data;
   Evas_Object *parent;
   const char *text;
   int i;
   int v;

   for (i = 0; i < 4 && sd->entries[i] != obj; i++)
     ;

   parent = evas_object_data_get(obj, "parent");
   text = elm_object_text_get(obj);
   v = atoi(text);
   if (v > 255) v = 255;
   else if (v < 0) v = 0;

   switch (i)
     {
      case 0:
         if (v != sd->r)
           _colors_set(parent, v, sd->g, sd->b, sd->a);
         break;
      case 1:
         if (v != sd->g)
           _colors_set(parent, sd->r, v, sd->b, sd->a);
         break;
      case 2:
         if (v != sd->b)
           _colors_set(parent, sd->r, sd->g, v, sd->a);
         break;
      case 3:
         if (v != sd->a)
           _colors_set(parent, sd->r, sd->g, sd->b, v);
         break;
     }
}

#ifdef HAVE_ELEMENTARY_X
static Eina_Bool _mouse_grab_pixels(void *data, int type __UNUSED__, void *event __UNUSED__);

static Ecore_X_Window
_x11_elm_widget_xwin_get(const Evas_Object *obj)
{
   Evas_Object *top;
   Ecore_X_Window xwin = 0;

   top = elm_widget_top_get(obj);
   if (!top) top = elm_widget_top_get(elm_widget_parent_widget_get(obj));
   if (top) xwin = elm_win_xwindow_get(top);
   if (!xwin)
     {
        Ecore_Evas *ee;
        Evas *evas = evas_object_evas_get(obj);
        if (!evas) return 0;
        ee = ecore_evas_ecore_evas_get(evas);
        if (!ee) return 0;
        xwin = _elm_ee_xwin_get(ee);
     }
   return xwin;
}

static void
_start_grab_pick_cb(void *data, Evas_Object *obj, void *event_info __UNUSED__)
{
   Evas_Object *o = data;

   ELM_COLORSELECTOR_DATA_GET(o, sd);

   elm_object_disabled_set(obj, EINA_TRUE);

   ecore_event_handler_del(sd->grab.mouse_motion);
   sd->grab.mouse_motion = ecore_event_handler_add(ECORE_EVENT_MOUSE_MOVE, _mouse_grab_pixels, o);

   ecore_x_keyboard_grab(sd->grab.xroot);
   ecore_x_pointer_grab(sd->grab.xroot);
}

static Eina_Bool
_key_up_cb(void *data, int type __UNUSED__, void *event __UNUSED__)
{
   Evas_Object *o = data;

   /* key => cancel */
   ELM_COLORSELECTOR_DATA_GET(o, sd);

   elm_object_disabled_set(sd->button, EINA_FALSE);
   ecore_x_keyboard_ungrab();
   ecore_x_pointer_ungrab();
   ecore_event_handler_del(sd->grab.mouse_motion);
   sd->grab.mouse_motion = ecore_event_handler_add(ECORE_X_RAW_MOTION, _mouse_grab_pixels, o);

   return EINA_TRUE;
}

static Eina_Bool
_mouse_up_cb(void *data, int type __UNUSED__, void *event __UNUSED__)
{
   const unsigned int *pixels;
   Evas_Object *o = data;

   /* mouse up => check it */
   ELM_COLORSELECTOR_DATA_GET(o, sd);

   elm_object_disabled_set(sd->button, EINA_FALSE);
   ecore_x_keyboard_ungrab();
   ecore_x_pointer_ungrab();
   ecore_event_handler_del(sd->grab.mouse_motion);
   sd->grab.mouse_motion = ecore_event_handler_add(ECORE_X_RAW_MOTION, _mouse_grab_pixels, o);

   pixels = evas_object_image_data_get(sd->picker_display, EINA_FALSE);
   sd->a = 0xff;
   sd->r = (pixels[17 * 9 + 9] >> 16) & 0xFF;
   sd->g = (pixels[17 * 9 + 9] >> 8) & 0xFF;
   sd->b = pixels[17 * 9 + 9] & 0xFF;

   _color_picker_init(sd);

   return EINA_TRUE;
}

static Eina_Bool
_mouse_grab_pixels(void *data, int type __UNUSED__, void *event __UNUSED__)
{
   Evas_Object *obj = data;
   Ecore_X_Visual visual;
   Ecore_X_Display *display;
   Ecore_X_Screen *scr;
   Ecore_X_Image *img;
   Ecore_X_Window xwin;
   int *src;
   int bpl = 0, rows = 0, bpp = 0;
   int x, y, w, h, depth;

   ELM_COLORSELECTOR_DATA_GET(obj, sd);

   if (sd->grab.in) return EINA_TRUE;

   xwin = _x11_elm_widget_xwin_get(obj);
   sd->grab.xroot = ecore_x_window_root_get(xwin);
   ecore_x_pointer_xy_get(sd->grab.xroot, &x, &y);

   if (x < 8) x += 8;
   if (y < 8) y += 8;

   if (sd->grab.x == x && sd->grab.y == y) return EINA_TRUE;
   sd->grab.x = x;
   sd->grab.y = y;

   evas_object_image_alpha_set(sd->picker_display, EINA_FALSE);

   display = ecore_x_display_get();
   scr = ecore_x_default_screen_get();
   visual = ecore_x_default_visual_get(display, scr);
   depth = ecore_x_window_depth_get(sd->grab.xroot);
   if (depth == 0) return EINA_TRUE;
   img = ecore_x_image_new(17, 17, visual, depth);
   if (!img) return EINA_TRUE;
   ecore_x_image_get(img, sd->grab.xroot, x - 8, y - 8, 0, 0, 17, 17);
   src = ecore_x_image_data_get(img, &bpl, &rows, &bpp);
   if (!ecore_x_image_is_argb32_get(img))
     {
        Ecore_X_Colormap colormap;
        unsigned int *pixels;

        colormap = ecore_x_default_colormap_get(display, scr);
        pixels = evas_object_image_data_get(sd->picker_display, EINA_TRUE);
        ecore_x_image_to_argb_convert(src, bpp, bpl, colormap, visual,
                                      0, 0, 17, 17,
                                      pixels, (17 * sizeof(int)), 0, 0);
     }
   else
     {
        evas_object_image_data_copy_set(sd->picker_display, src);
     }

   ecore_x_image_free(img);

   evas_object_geometry_get(sd->picker_display, NULL, NULL, &w, &h);
   evas_object_image_data_update_add(sd->picker_display, 0, 0, w, h);

   return EINA_TRUE;
}
#endif

static void
_mouse_in_canvas(void *data, Evas *e __UNUSED__, void *event_info __UNUSED__)
{
   Evas_Object *obj = data;
   ELM_COLORSELECTOR_DATA_GET(obj, sd);

   sd->grab.in = EINA_TRUE;
}

static void
_mouse_out_canvas(void *data, Evas *e __UNUSED__, void *event_info __UNUSED__)
{
   Evas_Object *obj = data;
   ELM_COLORSELECTOR_DATA_GET(obj, sd);

   sd->grab.in = EINA_FALSE;
}

static void
_color_picker_add(Evas_Object *obj, Elm_Colorselector_Smart_Data *sd)
{
   Evas_Object *ed;
   Evas_Object *im;
   Evas_Object *label;
   Evas_Object *entry;
   Evas_Object *table;
   Evas_Object *bx;
   static const char *labels[4] = { "R:", "G:", "B:", "A:" };
   int i;
   if (sd->picker) return;
#ifdef HAVE_ELEMENTARY_X
   Ecore_X_Window xwin;

   xwin = _x11_elm_widget_xwin_get(obj);
   if (xwin)
     {
        sd->grab.xroot = ecore_x_window_root_get(xwin);
        ecore_x_input_raw_select(sd->grab.xroot);

        sd->grab.mouse_motion = ecore_event_handler_add(ECORE_X_RAW_MOTION, _mouse_grab_pixels, obj);
        sd->grab.key_up = ecore_event_handler_add(ECORE_EVENT_KEY_UP, _key_up_cb, obj);
        sd->grab.mouse_up = ecore_event_handler_add(ECORE_EVENT_MOUSE_BUTTON_UP, _mouse_up_cb, obj);
     }
#endif
   /* setup the color picker */
   sd->picker = elm_box_add(obj);
   elm_box_horizontal_set(sd->picker, EINA_TRUE);
   evas_object_size_hint_weight_set(sd->picker, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(sd->picker, EVAS_HINT_FILL, EVAS_HINT_FILL);

   elm_box_padding_set(sd->picker,
                       (sd->h_pad * elm_widget_scale_get(obj) * elm_config_scale_get()),
                       (sd->v_pad * elm_widget_scale_get(obj) * elm_config_scale_get()));
   elm_box_align_set(sd->picker, 0.5, 0.5);

   bx = elm_box_add(sd->picker);
   evas_object_size_hint_weight_set(bx, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(bx, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_box_pack_end(sd->picker, bx);
   evas_object_show(bx);

   ed = edje_object_add(evas_object_evas_get(sd->picker));
   elm_widget_theme_object_set(obj, ed, "colorselector", "picker", elm_widget_style_get(obj));
   evas_object_size_hint_weight_set(ed, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(ed, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_box_pack_end(bx, ed);
   evas_object_show(ed);

#ifdef HAVE_ELEMENTARY_X
   if (xwin)
     {
        sd->button = elm_button_add(sd->picker);
        elm_object_text_set(sd->button, "Pick a color");
        evas_object_smart_callback_add(sd->button, "clicked", _start_grab_pick_cb, obj);
        elm_box_pack_end(bx, sd->button);
        evas_object_show(sd->button);
     }
#endif

   im = evas_object_image_add(evas_object_evas_get(sd->picker));
   evas_object_size_hint_aspect_set(im, EVAS_ASPECT_CONTROL_BOTH, 1, 1);
   evas_object_image_smooth_scale_set(im, EINA_FALSE);
   evas_object_image_colorspace_set(im, EVAS_COLORSPACE_ARGB8888);
   evas_object_image_size_set(im, 17, 17);
   evas_object_image_alpha_set(im, 1);
   evas_object_image_filled_set(im, EINA_TRUE);
   edje_object_part_swallow(ed, "elm.picker", im);
   elm_widget_sub_object_add(obj, im);

   sd->picker_display = im;

   table = elm_table_add(sd->picker);
   evas_object_size_hint_weight_set(table, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(table, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_box_pack_end(sd->picker, table);
   evas_object_show(table);

   for (i = 0; i < 4; i++)
     {
        static Elm_Entry_Filter_Accept_Set accept_set = {
          .accepted = "0123456789",
          .rejected = NULL
        };

        label = elm_label_add(table);
        elm_object_text_set(label, labels[i]);
        evas_object_size_hint_weight_set(label, 0.0, EVAS_HINT_EXPAND);
        evas_object_size_hint_align_set(label, 0.0, EVAS_HINT_FILL);
        elm_table_pack(table, label, 0, i, 1, 1);
        evas_object_show(label);

        entry = elm_entry_add(table);
        elm_entry_markup_filter_append(entry, elm_entry_filter_accept_set, &accept_set);
        elm_entry_single_line_set(entry, EINA_TRUE);
        elm_entry_scrollable_set(entry, EINA_TRUE);
        evas_object_data_set(entry, "parent", obj);
        evas_object_smart_callback_add(entry, "changed", _entry_changed_cb, sd);
        evas_object_size_hint_weight_set(entry, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
        evas_object_size_hint_align_set(entry, EVAS_HINT_FILL, EVAS_HINT_FILL);
        elm_table_pack(table, entry, 1, i, 1, 1);
        evas_object_show(entry);

        sd->entries[i] = entry;
     }

   evas_event_callback_add(evas_object_evas_get(obj), EVAS_CALLBACK_CANVAS_FOCUS_IN, _mouse_in_canvas, obj);
   evas_event_callback_add(evas_object_evas_get(obj), EVAS_CALLBACK_CANVAS_FOCUS_OUT, _mouse_out_canvas, obj);

   _color_picker_init(sd);
}

static void
_arrow_cb(void *data,
          Evas_Object *obj,
          const char *emission __UNUSED__,
          const char *source __UNUSED__)
{
   Color_Bar_Data *cb_data = data;
   double x, y;

   edje_object_part_drag_value_get(obj, "elm.arrow", &x, &y);

   _rectangles_redraw(data, x);
   evas_object_smart_callback_call(cb_data->parent, SIG_CHANGED, NULL);
}

static void
_colorbar_cb(void *data,
             Evas *e,
             Evas_Object *obj __UNUSED__,
             void *event_info)
{
   Evas_Event_Mouse_Down *ev = event_info;
   Color_Bar_Data *cb_data = data;
   double arrow_x = 0, arrow_y;
   Evas_Coord x, y, w, h;
   ELM_COLORSELECTOR_DATA_GET(cb_data->parent, sd);

   evas_object_geometry_get(cb_data->bar, &x, &y, &w, &h);
   edje_object_part_drag_value_get
     (cb_data->colorbar, "elm.arrow", &arrow_x, &arrow_y);

   if (w > 0) arrow_x = (double)(ev->canvas.x - x) / (double)w;
   if (arrow_x > 1) arrow_x = 1;
   if (arrow_x < 0) arrow_x = 0;
   edje_object_part_drag_value_set
     (cb_data->colorbar, "elm.arrow", arrow_x, arrow_y);

   _rectangles_redraw(data, arrow_x);
   evas_object_smart_callback_call(cb_data->parent, SIG_CHANGED, NULL);
   evas_event_feed_mouse_cancel(e, 0, NULL);
   evas_event_feed_mouse_down(e, 1, EVAS_BUTTON_NONE, 0, NULL);
   sd->sel_color_type = cb_data->color_type;
   sd->focused = ELM_COLORSELECTOR_COMPONENTS;
}

static void
_button_clicked_cb(void *data,
                   Evas_Object *obj,
                   void *event_info __UNUSED__)
{
   Eina_Bool is_right = EINA_FALSE;
   Color_Bar_Data *cb_data = data;
   double x, y, step;
   ELM_COLORSELECTOR_DATA_GET(cb_data->parent, sd);

   if (obj == cb_data->rbt)
     {
        is_right = EINA_TRUE;
        step = 1.0;
     }
   else step = -1.0;

   edje_object_part_drag_value_get(cb_data->colorbar, "elm.arrow", &x, &y);

   switch (cb_data->color_type)
     {
      case HUE:
        x += step / HUE_STEP;
        break;

      case SATURATION:
        x += step / SAT_STEP;
        break;

      case LIGHTNESS:
        x += step / LIG_STEP;
        break;

      case ALPHA:
        x += step / ALP_STEP;
        break;

      default:
        break;
     }

   if (is_right)
     {
        if (x > 1.0) x = 1.0;
     }
   else
     {
        if (x < 0.0) x = 0.0;
     }

   edje_object_part_drag_value_set(cb_data->colorbar, "elm.arrow", x, y);
   _rectangles_redraw(data, x);
   evas_object_smart_callback_call(cb_data->parent, SIG_CHANGED, NULL);
   sd->sel_color_type = cb_data->color_type;
   sd->focused = ELM_COLORSELECTOR_COMPONENTS;
}

static void
_button_repeat_cb(void *data,
                  Evas_Object *obj __UNUSED__,
                  void *event_info __UNUSED__)
{
   Eina_Bool is_right = EINA_FALSE;
   Color_Bar_Data *cb_data = data;
   double x, y, step;

   if (obj == cb_data->rbt)
     {
        is_right = EINA_TRUE;
        step = 1.0;
     }
   else step = -1.0;

   edje_object_part_drag_value_get(cb_data->colorbar, "elm.arrow", &x, &y);
   x += step / BASE_STEP;

   if (is_right)
     {
        if (x > 1.0) x = 1.0;
     }
   else
     {
        if (x < 0.0) x = 0.0;
     }

   edje_object_part_drag_value_set(cb_data->colorbar, "elm.arrow", x, y);
   _rectangles_redraw(data, x);
   evas_object_smart_callback_call(cb_data->parent, SIG_CHANGED, NULL);
}

static void
_on_colorbar_highlight_cb(void *data)
{
   Color_Bar_Data *cb_data = (Color_Bar_Data *)data;
   ELM_COLORSELECTOR_DATA_GET(cb_data->parent, sd);
   sd->sel_color_type = cb_data->color_type;
}

static char *
_access_state_cb(void *data __UNUSED__, Evas_Object *obj)
{
   char *ret;
   Eina_Strbuf *buf = NULL;
   double x, y;
   double per;
   Color_Bar_Data *cb_data = (Color_Bar_Data *)data;

   buf = eina_strbuf_new();

   edje_object_part_drag_value_get(cb_data->colorbar, "elm.arrow", &x, &y);

   per = (x / 1.0) * 100;
   per = abs(per);
   eina_strbuf_append_printf(buf, E_("IDS_TPLATFORM_BODY_PD_PERCENT_T_TTS"), (int)floor(per));

   if (!elm_widget_disabled_get(obj))
     eina_strbuf_append(buf, E_("IDS_TPLATFORM_BODY_FLICK_UP_AND_DOWN_TO_ADJUST_THE_POSITION_T_TTS"));

   if (eina_strbuf_length_get(buf))
     {
        ret = eina_strbuf_string_steal(buf);
        eina_strbuf_free(buf);
        return ret;
     }

   eina_strbuf_free(buf);
   return NULL;
}

static char *
_plane_access_state_cb(void *data __UNUSED__, Evas_Object *obj)
{
   char *ret;
   Eina_Strbuf *buf = NULL;
   buf = eina_strbuf_new();
   if (!elm_widget_disabled_get(obj))
     eina_strbuf_append(buf, E_("IDS_SMEMO_BODY_TAP_TO_APPLY_T_TTS"));
   if (eina_strbuf_length_get(buf))
     {
        ret = eina_strbuf_string_steal(buf);
        eina_strbuf_free(buf);
        return ret;
     }

   eina_strbuf_free(buf);
   return NULL;
}

static void
_access_colorbar_register(Evas_Object *obj,
                          Color_Bar_Data *cd,
                          const char* part)
{
   Evas_Object *ao;
   Elm_Access_Info *ai;
   const char* colorbar_type = NULL;

   ao = _elm_access_edje_object_part_object_register(obj, cd->colorbar, part);
   ai = _elm_access_object_get(ao);

   switch (cd->color_type)
     {
      case HUE:
        colorbar_type = E_("IDS_TPLATFORM_BODY_COLOUR_CONTROL_SLIDER_T_TALKBACK");
        break;

      case SATURATION:
        colorbar_type = E_("IDS_TPLATFORM_BODY_SATURATION_CONTROL_SLIDER_T_TALKBACK");
        break;

      case LIGHTNESS:
        colorbar_type = E_("IDS_TPLATFORM_BODY_BRIGHTNESS_CONTROL_SLIDER_T_TALKBACK");
        break;

      case ALPHA:
        colorbar_type = E_("Alpha control slider");
        break;

      default:
        break;
     }

   _elm_access_text_set(ai, ELM_ACCESS_TYPE, colorbar_type);
   _elm_access_callback_set(ai, ELM_ACCESS_STATE, _access_state_cb, cd);
   _elm_access_on_highlight_hook_set(ai,_on_colorbar_highlight_cb, cd);

   // this will be used in focus_next();
   cd->access_obj = ao;
}

static void
_color_bars_add(Evas_Object *obj)
{
   char colorbar_name[128];
   char colorbar_s[128];
   char buf[1024];
   int i = 0;
   Evas *e;

   ELM_COLORSELECTOR_DATA_GET(obj, sd);

   e = evas_object_evas_get(obj);

   for (i = 0; i < 4; i++)
     {
        sd->cb_data[i] = ELM_NEW(Color_Bar_Data);
        sd->cb_data[i]->parent = obj;

        switch (i)
          {
           case 0:
             sd->cb_data[i]->color_type = HUE;
             break;

           case 1:
             sd->cb_data[i]->color_type = SATURATION;
             break;

           case 2:
             sd->cb_data[i]->color_type = LIGHTNESS;
             break;

           case 3:
             sd->cb_data[i]->color_type = ALPHA;
             break;
          }

        /* load colorbar area */
        sd->cb_data[i]->colorbar = edje_object_add(e);
        elm_widget_theme_object_set
          (obj, sd->cb_data[i]->colorbar, "colorselector", "base",
          elm_widget_style_get(obj));
        snprintf(colorbar_name, sizeof(colorbar_name), "colorbar_%d", i);
        snprintf(colorbar_s, sizeof(colorbar_s), "elm.colorbar_%d", i);
        edje_object_signal_callback_add
          (sd->cb_data[i]->colorbar, "drag", "*", _arrow_cb, sd->cb_data[i]);
        edje_object_part_swallow
          (sd->col_bars_area, colorbar_s, sd->cb_data[i]->colorbar);
        elm_widget_sub_object_add(obj, sd->cb_data[i]->colorbar);

        /* load colorbar image */
        sd->cb_data[i]->bar = edje_object_add(e);
        snprintf(buf, sizeof(buf), "%s/%s", colorbar_name,
                 elm_widget_style_get(obj));
        elm_widget_theme_object_set
          (obj, sd->cb_data[i]->bar, "colorselector", "image", buf);
        edje_object_part_swallow
          (sd->cb_data[i]->colorbar, "elm.bar", sd->cb_data[i]->bar);
        elm_widget_sub_object_add(obj, sd->cb_data[i]->bar);

        /* provide expanded touch area */
        sd->cb_data[i]->touch_area = evas_object_rectangle_add(e);
        evas_object_color_set(sd->cb_data[i]->touch_area, 0, 0, 0, 0);
        edje_object_part_swallow
          (sd->cb_data[i]->colorbar, "elm.arrow_bg",
          sd->cb_data[i]->touch_area);
        evas_object_event_callback_add
          (sd->cb_data[i]->touch_area, EVAS_CALLBACK_MOUSE_DOWN, _colorbar_cb,
          sd->cb_data[i]);
        elm_widget_sub_object_add(obj, sd->cb_data[i]->touch_area);

        // ACCESS
        if (_elm_config->access_mode == ELM_ACCESS_MODE_ON)
           _access_colorbar_register(obj, sd->cb_data[i], "elm.arrow_bg_access");

        /* load background rectangle of the colorbar. used for
           changing color of the opacity bar */
        if ((i == 1) || (i == 2))
          {
             sd->cb_data[i]->bg_rect = evas_object_rectangle_add(e);
             evas_object_color_set
               (sd->cb_data[i]->bg_rect, sd->er, sd->eg, sd->eb, 255);
             edje_object_part_swallow
               (sd->cb_data[i]->colorbar, "elm.bar_bg",
               sd->cb_data[i]->bg_rect);

             elm_widget_sub_object_add(obj, sd->cb_data[i]->bg_rect);
          }
        if (i == 3)
          {
             sd->cb_data[i]->bg_rect = edje_object_add(e);
             snprintf(buf, sizeof(buf), "%s/%s", colorbar_name,
                      elm_widget_style_get(obj));
             elm_widget_theme_object_set
               (obj, sd->cb_data[i]->bg_rect, "colorselector", "bg_image",
               buf);
             edje_object_part_swallow
               (sd->cb_data[i]->colorbar, "elm.bar_bg",
               sd->cb_data[i]->bg_rect);
             elm_widget_sub_object_add(obj, sd->cb_data[i]->bg_rect);
             evas_object_color_set
               (sd->cb_data[i]->bar, sd->er, sd->eg, sd->eb, 255);
          }

        /* load arrow image, pointing the colorbar */
        sd->cb_data[i]->arrow = edje_object_add(e);
        elm_widget_theme_object_set
          (obj, sd->cb_data[i]->arrow, "colorselector", "arrow",
          elm_widget_style_get(obj));
        edje_object_part_swallow
          (sd->cb_data[i]->colorbar, "elm.arrow_icon",
          sd->cb_data[i]->arrow);
        elm_widget_sub_object_add(obj, sd->cb_data[i]->arrow);

        if (i == 2)
          evas_object_color_set(sd->cb_data[i]->arrow, 0, 0, 0, 255);
        else
          evas_object_color_set
            (sd->cb_data[i]->arrow, sd->er, sd->eg, sd->eb, 255);

        /* load left button */
        sd->cb_data[i]->lbt = elm_button_add(obj);
        snprintf(buf, sizeof(buf), "colorselector/left/%s",
                 elm_widget_style_get(obj));
        elm_object_style_set(sd->cb_data[i]->lbt, buf);
        elm_widget_sub_object_add(obj, sd->cb_data[i]->lbt);
        edje_object_part_swallow
          (sd->cb_data[i]->colorbar, "elm.l_button", sd->cb_data[i]->lbt);
        evas_object_smart_callback_add
          (sd->cb_data[i]->lbt, "clicked", _button_clicked_cb,
          sd->cb_data[i]);
        elm_button_autorepeat_set(sd->cb_data[i]->lbt, EINA_TRUE);
        elm_button_autorepeat_initial_timeout_set
          (sd->cb_data[i]->lbt, _elm_config->longpress_timeout);
        elm_button_autorepeat_gap_timeout_set
          (sd->cb_data[i]->lbt, (1.0 / _elm_config->fps));
        evas_object_smart_callback_add
          (sd->cb_data[i]->lbt, "repeated", _button_repeat_cb,
          sd->cb_data[i]);

        /* load right button */
        sd->cb_data[i]->rbt = elm_button_add(obj);
        snprintf(buf, sizeof(buf), "colorselector/right/%s",
                 elm_widget_style_get(obj));
        elm_object_style_set(sd->cb_data[i]->rbt, buf);
        elm_widget_sub_object_add(obj, sd->cb_data[i]->rbt);
        edje_object_part_swallow
          (sd->cb_data[i]->colorbar, "elm.r_button", sd->cb_data[i]->rbt);
        evas_object_smart_callback_add
          (sd->cb_data[i]->rbt, "clicked", _button_clicked_cb,
          sd->cb_data[i]);
        elm_button_autorepeat_set(sd->cb_data[i]->rbt, EINA_TRUE);
        elm_button_autorepeat_initial_timeout_set
          (sd->cb_data[i]->rbt, _elm_config->longpress_timeout);
        elm_button_autorepeat_gap_timeout_set
          (sd->cb_data[i]->rbt, (1.0 / _elm_config->fps));
        evas_object_smart_callback_add
          (sd->cb_data[i]->rbt, "repeated", _button_repeat_cb,
          sd->cb_data[i]);
     }
}

static Eina_Bool
_elm_colorselector_smart_theme(Evas_Object *obj)
{
   int i;
   Eina_List *elist;
   Elm_Color_Item *item;
   const char *hpadstr, *vpadstr;
   unsigned int h_pad = DEFAULT_HOR_PAD;
   unsigned int v_pad = DEFAULT_VER_PAD;

   ELM_COLORSELECTOR_DATA_GET(obj, sd);

   if (!ELM_WIDGET_CLASS(_elm_colorselector_parent_sc)->theme(obj))
     return EINA_FALSE;

   if ((sd->mode == ELM_COLORSELECTOR_PALETTE) || (sd->mode == ELM_COLORSELECTOR_ALL)
       || (sd->mode == ELM_COLORSELECTOR_PALETTE_PLANE) || (sd->mode == ELM_COLORSELECTOR_BOTH))
     {
        hpadstr = edje_object_data_get
         (ELM_WIDGET_DATA(sd)->resize_obj, "horizontal_pad");
        if (hpadstr) h_pad = atoi(hpadstr) / edje_object_base_scale_get(ELM_WIDGET_DATA(sd)->resize_obj);
        vpadstr = edje_object_data_get
            (ELM_WIDGET_DATA(sd)->resize_obj, "vertical_pad");
        if (vpadstr) v_pad = atoi(vpadstr) / edje_object_base_scale_get(ELM_WIDGET_DATA(sd)->resize_obj);

        elm_box_padding_set
          (sd->palette_box,
          (h_pad * elm_widget_scale_get(obj) * elm_config_scale_get()),
          (v_pad * elm_widget_scale_get(obj) * elm_config_scale_get()));

        EINA_LIST_FOREACH(sd->items, elist, item)
          {
             elm_layout_theme_set
               (VIEW(item), "colorselector", "item", elm_widget_style_get(obj));
             elm_widget_theme_object_set
               (obj, item->color_obj, "colorselector", "item/color",
               elm_widget_style_get(obj));
          }
     }

   if ((sd->mode == ELM_COLORSELECTOR_COMPONENTS) || (sd->mode == ELM_COLORSELECTOR_ALL)
       || (sd->mode == ELM_COLORSELECTOR_BOTH))
     {
        if (!sd->col_bars_area) return EINA_FALSE;
        elm_widget_theme_object_set
          (obj, sd->col_bars_area, "colorselector", "bg",
        elm_widget_style_get(obj));

        for (i = 0; i < 4; i++)
          {
             if (sd->cb_data[i])
               {
                  evas_object_del(sd->cb_data[i]->colorbar);
                  sd->cb_data[i]->colorbar = NULL;
                  evas_object_del(sd->cb_data[i]->bar);
                  sd->cb_data[i]->bar = NULL;
                  evas_object_del(sd->cb_data[i]->lbt);
                  sd->cb_data[i]->lbt = NULL;
                  evas_object_del(sd->cb_data[i]->rbt);
                  sd->cb_data[i]->rbt = NULL;
                  if (i != 0)
                    {
                       evas_object_del(sd->cb_data[i]->bg_rect);
                       sd->cb_data[i]->bg_rect = NULL;
                    }
                  evas_object_del(sd->cb_data[i]->arrow);
                  sd->cb_data[i]->arrow = NULL;
                  evas_object_del(sd->cb_data[i]->touch_area);
                  sd->cb_data[i]->touch_area = NULL;
               }
          }
        _color_bars_add(obj);
        elm_colorselector_color_set(obj, sd->r, sd->g, sd->b, sd->a);
     }
   elm_layout_sizing_eval(obj);

   return EINA_TRUE;
}

static void
_sub_obj_size_hints_set(Evas_Object *sobj,
                        int timesw,
                        int timesh)
{
   Evas_Coord minw = -1, minh = -1;

   elm_coords_finger_size_adjust(timesw, &minw, timesh, &minh);
   edje_object_size_min_restricted_calc(sobj, &minw, &minh, minw, minh);
   evas_object_size_hint_min_set(sobj, minw, minh);
   evas_object_size_hint_max_set(sobj, -1, -1);
}

static void
_item_sizing_eval(Elm_Color_Item *item)
{
   Evas_Coord minw = -1, minh = -1;

   if (!item) return;

   elm_coords_finger_size_adjust(1, &minw, 1, &minh);
   edje_object_size_min_restricted_calc(VIEW(item), &minw, &minh, minw, minh);
   evas_object_size_hint_min_set(VIEW(item), minw, minh);
}

/* fix size hints of color palette items, so that the box gets it */
static void
_palette_sizing_eval(Evas_Object *obj)
{
   Eina_List *elist;
   Elm_Color_Item *item;

   ELM_COLORSELECTOR_DATA_GET(obj, sd);

   EINA_LIST_FOREACH(sd->items, elist, item)
     _item_sizing_eval(item);
}

static void
_component_sizing_eval(Evas_Object *obj)
{
   Evas_Coord minw = -1, minh = -1;
   int i;

   ELM_COLORSELECTOR_DATA_GET(obj, sd);

   for (i = 0; i < 4; i++)
     {
        if (sd->cb_data[i])
          {
             if (sd->cb_data[i]->bg_rect)
               _sub_obj_size_hints_set(sd->cb_data[i]->bg_rect, 1, 1);

             _sub_obj_size_hints_set(sd->cb_data[i]->bar, 1, 1);
             _sub_obj_size_hints_set(sd->cb_data[i]->rbt, 1, 1);
             _sub_obj_size_hints_set(sd->cb_data[i]->lbt, 1, 1);
             _sub_obj_size_hints_set(sd->cb_data[i]->colorbar, 4, 1);
          }
     }

   edje_object_size_min_restricted_calc
     (sd->col_bars_area, &minw, &minh, minw, minh);
   evas_object_size_hint_min_set(sd->col_bars_area, minw, minh);
}

static void
_full_sizing_eval(Evas_Object *obj)
{
   _palette_sizing_eval(obj);
   _component_sizing_eval(obj);
}

static void
_elm_colorselector_smart_sizing_eval(Evas_Object *obj)
{
   Evas_Coord minw = -1, minh = -1;

   ELM_COLORSELECTOR_DATA_GET(obj, sd);

   elm_coords_finger_size_adjust(1, &minw, 1, &minh);

   switch (sd->mode)
     {
      case ELM_COLORSELECTOR_PALETTE:
        _palette_sizing_eval(obj);
        break;

      case ELM_COLORSELECTOR_COMPONENTS:
        _component_sizing_eval(obj);
        break;

      case ELM_COLORSELECTOR_BOTH:
        _full_sizing_eval(obj);
        break;

      default:
        return;
     }

   edje_object_size_min_calc(ELM_WIDGET_DATA(sd)->resize_obj, &minw, &minh);
   evas_object_size_hint_min_set(obj, minw, minh);
   evas_object_size_hint_max_set(obj, -1, -1);
}

static Eina_Bool
_on_color_long_press(void *data)
{
   Elm_Color_Item *item = (Elm_Color_Item *)data;
   ELM_COLORSELECTOR_DATA_GET(WIDGET(item), sd);

   sd->longpress_timer = NULL;
   sd->longpressed = EINA_TRUE;
   evas_object_smart_callback_call
     (WIDGET(item), SIG_COLOR_ITEM_LONGPRESSED, item);

   return ECORE_CALLBACK_CANCEL;
}

static void
_on_color_pressed(void *data,
                  Evas_Object *obj __UNUSED__,
                  const char *emission __UNUSED__,
                  const char *source __UNUSED__)
{
   Elm_Color_Item *item = (Elm_Color_Item *)data;
   if (!item) return;

   ELM_COLORSELECTOR_DATA_GET(WIDGET(item), sd);

   sd->longpressed = EINA_FALSE;

   if (sd->longpress_timer) ecore_timer_del(sd->longpress_timer);
   sd->longpress_timer = ecore_timer_add
       (_elm_config->longpress_timeout, _on_color_long_press, data);
   sd->focused = ELM_COLORSELECTOR_PALETTE;
}

static void
_on_color_released(void *data,
                   Evas_Object *obj __UNUSED__,
                   const char *emission __UNUSED__,
                   const char *source __UNUSED__)
{
   Elm_Color_Item *item = (Elm_Color_Item *)data;
   Eina_List *l;
   Elm_Color_Item *temp_item;
   if (!item) return;

   ELM_COLORSELECTOR_DATA_GET(WIDGET(item), sd);

   if (sd->longpress_timer)
     {
        ecore_timer_del(sd->longpress_timer);
        sd->longpress_timer = NULL;
     }

   temp_item = eina_list_data_get(sd->selected);
   evas_object_smart_callback_call
     (WIDGET(item), SIG_COLOR_ITEM_SELECTED, item);
   elm_colorselector_color_set
     (WIDGET(item), item->color->r, item->color->g, item->color->b,
     item->color->a);
   if (temp_item && (temp_item != item))
     elm_object_signal_emit(VIEW(temp_item), "elm,state,unhighlight", "elm");
   EINA_LIST_FOREACH(sd->items, l, temp_item)
     {
        if (item == temp_item)
          {
             sd->selected = l;
             sd->highlighted = sd->selected;
          }
     }
}

static char *
_access_info_cb(void *data, Evas_Object *obj __UNUSED__)
{
   char *ret;
   Eina_Strbuf *buf;

   Elm_Color_Item *it = data;
   ELM_COLORSELECTOR_ITEM_CHECK_OR_RETURN(it, NULL);

   buf = eina_strbuf_new();
   eina_strbuf_append_printf(buf, "%s", E_(it->color->color_name));
   ret = eina_strbuf_string_steal(buf);
   eina_strbuf_free(buf);
   return ret;
}

static void
_access_activate_cb(void *data __UNUSED__,
                    Evas_Object *part_obj __UNUSED__,
                    Elm_Object_Item *item)
{
   elm_object_item_signal_emit(item, "elm,state,selected", "elm");
   _on_color_released((Elm_Color_Item *)item, NULL, NULL, NULL);
}

static void
_access_widget_item_register(Elm_Color_Item *it)
{
   Elm_Access_Info *ai;

   _elm_access_widget_item_register((Elm_Widget_Item *)it);

   ai = _elm_access_object_get(it->base.access_obj);

   _elm_access_callback_set(ai, ELM_ACCESS_INFO, _access_info_cb, it);
   _elm_access_activate_callback_set(ai, _access_activate_cb, it);
}

static void
_item_resize(void *data __UNUSED__, Evas *e __UNUSED__, Evas_Object *obj,
             void *event_info __UNUSED__)
{
   elm_layout_sizing_eval(obj);
}

static void
_item_signal_emit_hook(Elm_Object_Item *item,
                       const char *emission,
                       const char *source)
{
   elm_object_signal_emit(VIEW(item), emission, source);
}

static Eina_Bool
_item_del_pre_hook(Elm_Object_Item *it)
{
   Elm_Color_Item *item = (Elm_Color_Item *)it;
   ELM_COLORSELECTOR_ITEM_CHECK_OR_RETURN(it, EINA_FALSE);
   ELM_COLORSELECTOR_DATA_GET(WIDGET(it), sd);

   if (item->color)
     {
        eina_stringshare_del(item->color->color_name);
        free(item->color);
     }
   sd->items = eina_list_remove(sd->items, item);
   _colors_save(WIDGET(it));

   return EINA_TRUE;
}

static Elm_Color_Item *
_item_new(Evas_Object *obj)
{
   Elm_Color_Item *item;

   item = elm_widget_item_new(obj, Elm_Color_Item);
   if (!item) return NULL;

   VIEW(item) = elm_layout_add(obj);
   elm_layout_theme_set
     (VIEW(item), "colorselector", "item", elm_widget_style_get(obj));
   evas_object_size_hint_weight_set
     (VIEW(item), EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(VIEW(item), EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_event_callback_add
     (VIEW(item), EVAS_CALLBACK_RESIZE, _item_resize, NULL);

   item->color_obj = edje_object_add(evas_object_evas_get(obj));
   elm_widget_theme_object_set
     (obj, item->color_obj, "colorselector", "item/color",
     elm_widget_style_get(obj));
   evas_object_size_hint_weight_set
     (item->color_obj, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set
     (item->color_obj, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_object_signal_callback_add
     (VIEW(item), "elm,state,down", "", _on_color_pressed, item);
   elm_object_signal_callback_add
     (VIEW(item), "elm,state,up", "", _on_color_released, item);
   elm_object_part_content_set(VIEW(item), "color_obj", item->color_obj);

   elm_widget_item_signal_emit_hook_set(item, _item_signal_emit_hook);
   elm_widget_item_del_pre_hook_set(item, _item_del_pre_hook);

   _item_sizing_eval(item);
   evas_object_show(VIEW(item));

   // ACCESS
   if (_elm_config->access_mode == ELM_ACCESS_MODE_ON)
     _access_widget_item_register(item);

   return item;
}

static void
_colors_remove(Evas_Object *obj)
{
   ELM_COLORSELECTOR_DATA_GET(obj, sd);

   _items_del(sd);
   _elm_config_colors_free(sd->palette_name);
}

static void
_colors_save(Evas_Object *obj)
{
   Eina_List *elist;
   Elm_Color_Item *item;

   ELM_COLORSELECTOR_DATA_GET(obj, sd);

   _elm_config_colors_free(sd->palette_name);
   EINA_LIST_FOREACH(sd->items, elist, item)
     _elm_config_color_set(sd->palette_name, item->color->r, item->color->g,
                           item->color->b, item->color->a);
}

static void
_palette_colors_load(Evas_Object *obj)
{
   Eina_List *elist;
   Elm_Color_Item *item;
   Eina_List *color_list;
   Elm_Color_RGBA *color;

   ELM_COLORSELECTOR_DATA_GET(obj, sd);

   color_list = _elm_config_color_list_get(sd->palette_name);
   if (!color_list) return;

   EINA_LIST_REVERSE_FOREACH(color_list, elist, color)
     {
        item = _item_new(obj);
        if (!item) return;

        item->color = ELM_NEW(Elm_Color_RGBA);
        if (!item->color) return;
        item->color->r = color->r;
        item->color->g = color->g;
        item->color->b = color->b;
        item->color->a = color->a;
        eina_stringshare_replace(&item->color->color_name, color->color_name);
        elm_box_pack_end(sd->palette_box, VIEW(item));
        evas_object_color_set
          (item->color_obj, item->color->r, item->color->g, item->color->b,
          item->color->a);

        sd->items = eina_list_append(sd->items, item);
     }

   sd->config_load = EINA_TRUE;
}

static void _colorplane_cb(void *data, Evas_Object *obj, const char *emission __UNUSED__, const char *source __UNUSED__)
{
   Evas_Object* colorselector = (Evas_Object*)data;

   ELM_COLORSELECTOR_CHECK(colorselector);
   ELM_COLORSELECTOR_DATA_GET(colorselector, sd);

   _colorplane_color_get(obj, &(sd->r), &(sd->g), &(sd->b), &(sd->a));
   evas_object_smart_callback_call(colorselector, SIG_CHANGED, NULL);
}

static void
_elm_colorselector_smart_add(Evas_Object *obj)
{
   EVAS_SMART_DATA_ALLOC(obj, Elm_Colorselector_Smart_Data);

   ELM_WIDGET_CLASS(_elm_colorselector_parent_sc)->base.add(obj);
}

static void
_elm_colorselector_smart_del(Evas_Object *obj)
{
   int i = 0;
   void *tmp[4];

   ELM_COLORSELECTOR_DATA_GET(obj, sd);

   evas_event_callback_del_full(evas_object_evas_get(obj), EVAS_CALLBACK_CANVAS_FOCUS_IN, _mouse_in_canvas, obj);
   evas_event_callback_del_full(evas_object_evas_get(obj), EVAS_CALLBACK_CANVAS_FOCUS_OUT, _mouse_out_canvas, obj);

   if (sd->longpress_timer) ecore_timer_del(sd->longpress_timer);
   if (sd->palette_name) eina_stringshare_del(sd->palette_name);
   if (sd->grab.mouse_motion) ecore_event_handler_del(sd->grab.mouse_motion);
   if (sd->grab.mouse_up) ecore_event_handler_del(sd->grab.mouse_up);
   if (sd->grab.key_up) ecore_event_handler_del(sd->grab.key_up);

   _items_del(sd);
   /* This cb_data are used during the destruction process of base.del */
   for (i = 0; i < 4; i++)
     tmp[i] = sd->cb_data[i];
   ELM_WIDGET_CLASS(_elm_colorselector_parent_sc)->base.del(obj);
   for (i = 0; i < 4; i++)
     free(tmp[i]);
}

static Eina_List*
_palette_box_vertical_item_get(Eina_List* ref_item, enum Palette_Box_Direction dir)
{
   Evas_Coord basex, basey, x, y, dx, min_dx;
   Elm_Color_Item *item;
   Eina_List* l;
   Eina_List* res = NULL;
   Eina_List* (*dir_func)(const Eina_List*);

   if (!ref_item) return NULL;

   switch(dir)
     {
        case PALETTE_BOX_UP:
          dir_func = eina_list_prev;
          break;
        case PALETTE_BOX_DOWN:
          dir_func = eina_list_next;
          break;
        default:
          return NULL;
     }

   item = eina_list_data_get(ref_item);
   evas_object_geometry_get(VIEW(item), &basex, &basey, NULL, NULL);

   for (l = ref_item; l; l = dir_func(l))
     {
        item = eina_list_data_get(l);
        evas_object_geometry_get(VIEW(item), &x, &y, NULL, NULL);
        if (basey != y) break;
     }

   basey = y;
   min_dx = -1;

   for (; l; l = dir_func(l))
     {
        item = eina_list_data_get(l);
        evas_object_geometry_get(VIEW(item), &x, &y, NULL, NULL);
        if (basey != y) break;

        dx = abs(x - basex);
        if (dx < min_dx || min_dx < 0)
          {
             min_dx = dx;
             res = l;
          }
        else
          {
             break;
          }
      }

   return res;
}

static void _colorplane_color_get(Evas_Object *obj, int *r, int *g, int *b, int *a)
{
   if (!obj) return;
   double x, y;
   Evas_Coord w, h;
   Evas_Coord x_off = 0, y_off = 0;
   Evas_Coord offset;
   unsigned char *origin_data, *p;
   const Evas_Object *img = edje_object_part_object_get(elm_layout_edje_get(obj), "colorplane_bg");

   if (!img) return;
   evas_object_image_size_get(img, &w, &h);
   origin_data =  evas_object_image_data_get(img, EINA_FALSE);
   edje_object_part_drag_value_get(elm_layout_edje_get(obj), "elm.touch", &x, &y);
   x_off = x * (w - 1);
   y_off = y * (h - 1);
   offset = (y_off * w + x_off)* COLORSELECTOR_COLOR_COMPONENT_NUM;
   p = origin_data + offset;
   if (a) *a = *(p + COLORSELECTOR_COLOR_COMPONENT_A);
   if (r) *r = *(p + COLORSELECTOR_COLOR_COMPONENT_R);
   if (g) *g = *(p + COLORSELECTOR_COLOR_COMPONENT_G);
   if (b) *b = *(p + COLORSELECTOR_COLOR_COMPONENT_B);
}

static void _colorplane_color_set(Evas_Object *obj, int r, int g, int b, int a __UNUSED__)
{
   if (!obj) return;
   int i, j;
   Evas_Coord w, h;
   Evas_Coord x_off = 0, y_off = 0;
   Eina_Bool found = EINA_FALSE;
   unsigned char *origin_data, *p;
   double x, y;
   const Evas_Object *img = edje_object_part_object_get(elm_layout_edje_get(obj), "colorplane_bg");

   if(!img) return;
   evas_object_image_size_get(img, &w, &h);
   origin_data =  evas_object_image_data_get(img, EINA_FALSE);

   for (j = 0; j < h; j++)
     {
        for (i = 0; i < w; i++)
          {
             p = origin_data + COLORSELECTOR_COLOR_COMPONENT_NUM * (i + j * w);

             if (*(p + COLORSELECTOR_COLOR_COMPONENT_R) == r &&
                *(p + COLORSELECTOR_COLOR_COMPONENT_G) == g &&
                *(p + COLORSELECTOR_COLOR_COMPONENT_B) == b)
               {
                   x_off = i;
                   y_off = j;
                   found = EINA_TRUE;
                   break;
               }
           }
      }
   if (found)
     {
        x = (double) (x_off) / (w - 1);
        y = (double) (y_off) / (h - 1);
        edje_object_part_drag_value_set(elm_layout_edje_get(obj), "elm.touch", x, y);
        elm_object_signal_emit(obj, "show", "elm");
     }
   else
     {
        edje_object_part_drag_value_set(elm_layout_edje_get(obj), "elm.touch", 0, 0);
        elm_object_signal_emit(obj, "hide", "elm");
     }
}

static Eina_Bool
_elm_colorselector_smart_event(Evas_Object *obj,
                               Evas_Object *src __UNUSED__,
                               Evas_Callback_Type type,
                               void *event_info)
{
   Eina_List *cl = NULL;
   Elm_Color_Item *item = NULL;
   char colorbar_s[128];
   if (type != EVAS_CALLBACK_KEY_DOWN) return EINA_FALSE;
   Evas_Event_Key_Down *ev = event_info;
   ELM_COLORSELECTOR_DATA_GET(obj, sd);
   Elm_Color_Item *temp_item = NULL;
   double x = 0.0, y = 0.0;

   // TIZEN ONLY(20131221) : When access mode, focused ui is disabled.
   if (_elm_config->access_mode) return EINA_FALSE;

   if (!sd) return EINA_FALSE;
   if (ev->event_flags & EVAS_EVENT_FLAG_ON_HOLD) return EINA_FALSE;
   if (elm_widget_disabled_get(obj)) return EINA_FALSE;
   if (!sd->highlighted) sd->highlighted = sd->items;
   temp_item = eina_list_data_get(sd->highlighted);

   if ((!strcmp(ev->keyname, "Return")) ||
            ((!strcmp(ev->keyname, "KP_Enter")) && !ev->string))
     {
        if (sd->focused == ELM_COLORSELECTOR_PALETTE && sd->highlighted)
          {
             item = eina_list_data_get(sd->highlighted);
             temp_item = eina_list_data_get(sd->selected);
             elm_object_signal_emit(VIEW(item), "elm,state,selected", "elm");
             if (temp_item && (temp_item != item))
               elm_object_signal_emit(VIEW(temp_item), "elm,state,unhighlight", "elm");
             evas_object_smart_callback_call(WIDGET(item),
                                             SIG_COLOR_ITEM_SELECTED,
                                             item);
             elm_colorselector_color_set(WIDGET(item), item->color->r, item->color->g,
                                         item->color->b,
                                         item->color->a);
             sd->selected = sd->highlighted;
          }
        return EINA_TRUE;
     }
   else if ((!strcmp(ev->keyname, "Left")) ||
       ((!strcmp(ev->keyname, "KP_Left")) && (!ev->string)))
     {
        if (sd->focused == ELM_COLORSELECTOR_PALETTE && sd->highlighted)
          cl = eina_list_prev(sd->highlighted);
        else if (sd->focused == ELM_COLORSELECTOR_COMPONENTS)
          {
             if (sd->cb_data[sd->sel_color_type]->focused)
                _button_clicked_cb(sd->cb_data[sd->sel_color_type], sd->cb_data[sd->sel_color_type]->lbt, NULL);
             else
               {
                  edje_object_signal_emit(sd->cb_data[sd->sel_color_type]->colorbar, "elm,state,focused", "elm");
                  sd->cb_data[sd->sel_color_type]->focused = EINA_TRUE;
               }
          }
        else if (sd->focused == ELM_COLORSELECTOR_PLANE)
          {
             elm_object_signal_emit(sd->col_plane, "show", "elm");
             edje_object_part_drag_value_get(elm_layout_edje_get(sd->col_plane), "elm.touch", &x, &y);
             x = x - COLORPLANE_STEP;
             if (x < 0.0) x = 0.0;
             edje_object_part_drag_value_set(elm_layout_edje_get(sd->col_plane), "elm.touch", x, y);
             _colorplane_color_get(sd->col_plane, &(sd->r), &(sd->g), &(sd->b), &(sd->a));
             evas_object_smart_callback_call(obj, SIG_CHANGED, NULL);
          }
        else return EINA_FALSE;
     }
   else if ((!strcmp(ev->keyname, "Right")) ||
            ((!strcmp(ev->keyname, "KP_Right")) && (!ev->string)))
     {
        if (sd->focused == ELM_COLORSELECTOR_PALETTE && sd->highlighted)
          cl = eina_list_next(sd->highlighted);
        else if (sd->focused == ELM_COLORSELECTOR_COMPONENTS)
          {
             if (sd->cb_data[sd->sel_color_type]->focused)
               _button_clicked_cb(sd->cb_data[sd->sel_color_type], sd->cb_data[sd->sel_color_type]->rbt, NULL);
             else
               {
                  edje_object_signal_emit(sd->cb_data[sd->sel_color_type]->colorbar, "elm,state,focused", "elm");
                  sd->cb_data[sd->sel_color_type]->focused = EINA_TRUE;
               }
          }
        else if (sd->focused == ELM_COLORSELECTOR_PLANE)
          {
             elm_object_signal_emit(sd->col_plane, "show", "elm");
             edje_object_part_drag_value_get(elm_layout_edje_get(sd->col_plane), "elm.touch", &x, &y);
             x = x + COLORPLANE_STEP;
             if (x > 1.0) x = 1.0;
             edje_object_part_drag_value_set(elm_layout_edje_get(sd->col_plane), "elm.touch", x, y);
             _colorplane_color_get(sd->col_plane, &(sd->r), &(sd->g), &(sd->b), &(sd->a));
             evas_object_smart_callback_call(obj, SIG_CHANGED, NULL);
          }
        else return EINA_FALSE;
     }
   else if ((!strcmp(ev->keyname, "Up")) ||
            ((!strcmp(ev->keyname, "KP_Up")) && (!ev->string)))
     {
        if (sd->focused == ELM_COLORSELECTOR_COMPONENTS)
          {
             if (sd->sel_color_type > HUE)
               {
                  edje_object_signal_emit(sd->cb_data[sd->sel_color_type]->colorbar, "elm,state,unfocused", "elm");
                  sd->cb_data[sd->sel_color_type]->focused = EINA_FALSE;
               }
             sd->sel_color_type = sd->sel_color_type - 1;
             if (sd->sel_color_type < HUE)
               {
                  if (sd->mode == ELM_COLORSELECTOR_BOTH)
                    {
                       sd->focused = ELM_COLORSELECTOR_PALETTE;
                       /*when focus is shifted to palette start from
                        * first item*/
                       sd->highlighted = sd->items;
                       cl = sd->highlighted;
                    }
                  else
                    {
                       sd->sel_color_type = HUE;
                       return EINA_FALSE;
                    }
               }
             edje_object_signal_emit(sd->cb_data[sd->sel_color_type]->colorbar, "elm,state,focused", "elm");
             sd->cb_data[sd->sel_color_type]->focused = EINA_TRUE;
          }
        else if (sd->focused == ELM_COLORSELECTOR_PALETTE)
          {
             cl = _palette_box_vertical_item_get(sd->highlighted, PALETTE_BOX_UP);
             if (!cl) cl = sd->highlighted;
          }
        else if (sd->focused == ELM_COLORSELECTOR_PLANE)
          {
             edje_object_part_drag_value_get(elm_layout_edje_get(sd->col_plane), "elm.touch", &x, &y);
             y = y - COLORPLANE_STEP;
             if (y < 0.0) y = 0.0;
             edje_object_part_drag_value_set(elm_layout_edje_get(sd->col_plane), "elm.touch", x, y);
             _colorplane_color_get(sd->col_plane, &(sd->r), &(sd->g), &(sd->b), &(sd->a));
             evas_object_smart_callback_call(obj, SIG_CHANGED, NULL);
          }
     }
   else if ((!strcmp(ev->keyname, "Down")) ||
            ((!strcmp(ev->keyname, "KP_Down")) && (!ev->string)))
     {
        if (sd->focused == ELM_COLORSELECTOR_PALETTE)
          {
             cl = _palette_box_vertical_item_get(sd->highlighted, PALETTE_BOX_DOWN);
             if (sd->mode == ELM_COLORSELECTOR_BOTH && !cl)
               {
                  sd->focused = ELM_COLORSELECTOR_COMPONENTS;
                  /*when focus is shifted to component start from
                   * first color type*/
                  sd->sel_color_type = HUE;
               }
             else if (sd->mode == ELM_COLORSELECTOR_PALETTE_PLANE && !cl)
               {
                  sd->focused = ELM_COLORSELECTOR_PLANE;
                  elm_object_signal_emit(sd->col_plane, "show", "elm");
               }
          }
        else if (sd->focused == ELM_COLORSELECTOR_COMPONENTS)
          {
             snprintf(colorbar_s, sizeof(colorbar_s), "elm.colorbar_%d",
                      (sd->sel_color_type + 1));
             /*Append color type only if next color bar is available*/
             if (edje_object_part_swallow_get(sd->col_bars_area, colorbar_s))
               {
                  sd->cb_data[sd->sel_color_type]->focused = EINA_FALSE;
                  edje_object_signal_emit(sd->cb_data[sd->sel_color_type]->colorbar, "elm,state,unfocused", "elm");
                  sd->sel_color_type = sd->sel_color_type + 1;
                  edje_object_signal_emit(sd->cb_data[sd->sel_color_type]->colorbar, "elm,state,focused", "elm");
                  sd->cb_data[sd->sel_color_type]->focused = EINA_TRUE;
               }
             else return EINA_FALSE;
          }
        else if (sd->focused == ELM_COLORSELECTOR_PLANE)
          {
             edje_object_part_drag_value_get(elm_layout_edje_get(sd->col_plane), "elm.touch", &x, &y);
             y = y + COLORPLANE_STEP;
             if (y > 1.0) y = 1.0;
             edje_object_part_drag_value_set(elm_layout_edje_get(sd->col_plane), "elm.touch", x, y);
             _colorplane_color_get(sd->col_plane, &(sd->r), &(sd->g), &(sd->b), &(sd->a));
             evas_object_smart_callback_call(obj, SIG_CHANGED, NULL);
          }
     }
   else return EINA_FALSE;
   if (cl)
     {
        item = eina_list_data_get(cl);
        elm_object_signal_emit(VIEW(item), "elm,highlight,on", "elm");
        if (temp_item && (temp_item != item))
          elm_object_signal_emit(VIEW(temp_item), "elm,highlight,off", "elm");
        sd->highlighted = cl;
     }
   else if (!cl && sd->focused == ELM_COLORSELECTOR_PALETTE)
     return EINA_FALSE;
   ev->event_flags |= EVAS_EVENT_FLAG_ON_HOLD;
   return EINA_TRUE;
}

static Eina_Bool
_elm_colorselector_smart_focus_next(const Evas_Object *obj,
                                    Elm_Focus_Direction dir,
                                    Evas_Object **next)
{
   Eina_List *items = NULL;
   Eina_List *l;
   Elm_Widget_Item *item;
   char colorbar_s[128];
   int i = 0;

   ELM_COLORSELECTOR_DATA_GET(obj, sd);
   if (!sd) return EINA_FALSE;

   if ((sd->mode == ELM_COLORSELECTOR_PALETTE) || (sd->mode == ELM_COLORSELECTOR_ALL)
       || (sd->mode == ELM_COLORSELECTOR_PALETTE_PLANE) || (sd->mode == ELM_COLORSELECTOR_BOTH))
     {
        if (!sd->items) return EINA_FALSE;
        EINA_LIST_FOREACH(sd->items, l, item)
          items = eina_list_append(items, item->access_obj);
     }
   if ((sd->mode == ELM_COLORSELECTOR_PLANE) || (sd->mode == ELM_COLORSELECTOR_ALL)
       || (sd->mode == ELM_COLORSELECTOR_PALETTE_PLANE))
     items = eina_list_append(items, sd->col_plane_access_obj);
   if ((sd->mode == ELM_COLORSELECTOR_COMPONENTS) || (sd->mode == ELM_COLORSELECTOR_ALL)
       || (sd->mode == ELM_COLORSELECTOR_BOTH))
     {
        for (i = 0; i < 4; i++)
          {
             snprintf(colorbar_s, sizeof(colorbar_s), "elm.colorbar_%d", i);
             /*Append color type only if next color bar is available*/
             if (edje_object_part_swallow_get(sd->col_bars_area, colorbar_s))
               items = eina_list_append(items, sd->cb_data[i]->access_obj);
          }
     }

   return elm_widget_focus_list_next_get(obj, items, eina_list_data_get,
                                         dir, next);
}

static void
_access_obj_process(Evas_Object *obj, Eina_Bool is_access)
{
   Eina_List *l;
   Elm_Color_Item *it;
   int i = 0;

   ELM_COLORSELECTOR_DATA_GET(obj, sd);

   if ((sd->mode == ELM_COLORSELECTOR_PALETTE) || (sd->mode == ELM_COLORSELECTOR_ALL)
       || (sd->mode == ELM_COLORSELECTOR_PALETTE_PLANE) || (sd->mode == ELM_COLORSELECTOR_BOTH))
     {
        if (is_access)
          {
             EINA_LIST_FOREACH(sd->items, l, it)
               _access_widget_item_register(it);
          }
        else
          {
             EINA_LIST_FOREACH(sd->items, l, it)
               _elm_access_widget_item_unregister((Elm_Widget_Item *)it);
          }
     }
   if ((sd->mode == ELM_COLORSELECTOR_PLANE) || (sd->mode == ELM_COLORSELECTOR_ALL)
       || (sd->mode == ELM_COLORSELECTOR_PALETTE_PLANE))
     {
        if (is_access)
          {
              sd->col_plane_access_obj = elm_access_object_register(sd->col_plane,
                                                                    obj);
             _elm_access_text_set(_elm_access_object_get(sd->col_plane_access_obj),
                                  ELM_ACCESS_TYPE, E_("IDS_ST_HEADER_COLOUR_PICKER_ABB"));
             _elm_access_callback_set(_elm_access_object_get(sd->col_plane_access_obj),
                                      ELM_ACCESS_STATE, _plane_access_state_cb, obj);
           }
         else
           elm_access_object_unregister(sd->col_plane);
     }
   if ((sd->mode == ELM_COLORSELECTOR_COMPONENTS) || (sd->mode == ELM_COLORSELECTOR_ALL)
       || (sd->mode == ELM_COLORSELECTOR_BOTH))
     {
        for (i = 0; i < 4; i++)
          {
             if (is_access)
               _access_colorbar_register(obj, sd->cb_data[i],
                                         "elm.arrow_bg_access");
             else
               _elm_access_edje_object_part_object_unregister(obj, sd->cb_data[i]->colorbar,
                                                              "elm.arrow_bg_access");
          }
     }
}

static void
_access_hook(Evas_Object *obj, Eina_Bool is_access)
{
   ELM_COLORSELECTOR_CHECK(obj);
   ELM_COLORSELECTOR_DATA_GET(obj, sd);

   if (is_access)
     ELM_WIDGET_CLASS(ELM_WIDGET_DATA(sd)->api)->focus_next =
       _elm_colorselector_smart_focus_next;
   else
     ELM_WIDGET_CLASS(ELM_WIDGET_DATA(sd)->api)->focus_next = NULL;

   _access_obj_process(obj, is_access);
}

static Eina_Bool
_elm_colorselector_smart_activate(Evas_Object *obj, Elm_Activate act)
{
   char *text = NULL;
   Eina_Strbuf *buf;
   double per, x, y;
   ELM_COLORSELECTOR_DATA_GET(obj, sd);

   if ((elm_widget_disabled_get(obj)) ||
       (act == ELM_ACTIVATE_DEFAULT)) return EINA_FALSE;
   if (sd->focused != ELM_COLORSELECTOR_COMPONENTS) return EINA_FALSE;

   if ((act == ELM_ACTIVATE_UP) || (act == ELM_ACTIVATE_RIGHT))
     _button_clicked_cb(sd->cb_data[sd->sel_color_type], sd->cb_data[sd->sel_color_type]->rbt, NULL);
   else if ((act == ELM_ACTIVATE_DOWN) || (act == ELM_ACTIVATE_LEFT))
     _button_clicked_cb(sd->cb_data[sd->sel_color_type], sd->cb_data[sd->sel_color_type]->lbt, NULL);

   buf = eina_strbuf_new();
   edje_object_part_drag_value_get(sd->cb_data[sd->sel_color_type]->colorbar, "elm.arrow", &x, &y);

   per = (x / 1.0) * 100;
   per = abs(per);
   eina_strbuf_append_printf(buf, E_("IDS_TPLATFORM_BODY_PD_PERCENT_T_TTS"), (int)floor(per));

   text = eina_strbuf_string_steal(buf);
   eina_strbuf_free(buf);
   _elm_access_say(obj, text, EINA_FALSE);
   return EINA_TRUE;
}

static void
_elm_colorselector_smart_signal(Evas_Object *obj,
                        const char *emission,
                        const char *source)
{
   ELM_COLORSELECTOR_CHECK(obj);
   ELM_COLORSELECTOR_DATA_GET(obj, sd);
   int i = 0;

   edje_object_signal_emit(ELM_WIDGET_DATA(sd)->resize_obj, emission, source);
   if ((sd->mode == ELM_COLORSELECTOR_COMPONENTS) || (sd->mode == ELM_COLORSELECTOR_ALL)
       || (sd->mode == ELM_COLORSELECTOR_BOTH))
     {
        edje_object_signal_emit(sd->col_bars_area, emission, source);
        for (i = 0; i < 4; i++)
          {
             if (sd->cb_data[i])
               {
                  edje_object_signal_emit(sd->cb_data[i]->colorbar, emission, source);
                  edje_object_signal_emit(sd->cb_data[i]->bar, emission, source);
               }
          }
     }
   if ((sd->mode == ELM_COLORSELECTOR_PLANE) || (sd->mode == ELM_COLORSELECTOR_ALL)
       || (sd->mode == ELM_COLORSELECTOR_PALETTE_PLANE))
     elm_object_signal_emit(sd->col_plane, emission, source);
}

static void
_elm_colorselector_smart_set_user(Elm_Colorselector_Smart_Class *sc)
{
   ELM_WIDGET_CLASS(sc)->base.add = _elm_colorselector_smart_add;
   ELM_WIDGET_CLASS(sc)->base.del = _elm_colorselector_smart_del;

   /* not a 'focus chain manager' */
   ELM_WIDGET_CLASS(sc)->focus_next = NULL;
   ELM_WIDGET_CLASS(sc)->focus_direction_manager_is = NULL;
   ELM_WIDGET_CLASS(sc)->focus_direction = NULL;

   ELM_WIDGET_CLASS(sc)->theme = _elm_colorselector_smart_theme;
   ELM_WIDGET_CLASS(sc)->event = _elm_colorselector_smart_event;

   ELM_LAYOUT_CLASS(sc)->sizing_eval = _elm_colorselector_smart_sizing_eval;
   ELM_LAYOUT_CLASS(sc)->signal = _elm_colorselector_smart_signal;

   // ACCESS
   if (_elm_config->access_mode == ELM_ACCESS_MODE_ON)
      {
         ELM_WIDGET_CLASS(sc)->focus_next = _elm_colorselector_smart_focus_next;
         ELM_WIDGET_CLASS(sc)->activate = _elm_colorselector_smart_activate;
      }


   ELM_WIDGET_CLASS(sc)->access = _access_hook;
}

EAPI const Elm_Colorselector_Smart_Class *
elm_colorselector_smart_class_get(void)
{
   static Elm_Colorselector_Smart_Class _sc =
     ELM_COLORSELECTOR_SMART_CLASS_INIT_NAME_VERSION
       (ELM_COLORSELECTOR_SMART_NAME);
   static const Elm_Colorselector_Smart_Class *class = NULL;
   Evas_Smart_Class *esc = (Evas_Smart_Class *)&_sc;

   if (class)
     return class;

   _elm_colorselector_smart_set(&_sc);
   esc->callbacks = _smart_callbacks;
   class = &_sc;

   return class;
}

static void
_create_colorpalette(Evas_Object *obj)
{
   ELM_COLORSELECTOR_DATA_GET(obj, sd);
   if (sd->palette_box) return;
   sd->palette_box = elm_box_add(obj);
   elm_box_layout_set
     (sd->palette_box, evas_object_box_layout_flow_horizontal, NULL, NULL);
   elm_box_horizontal_set(sd->palette_box, EINA_TRUE);
   evas_object_size_hint_weight_set
     (sd->palette_box, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set
     (sd->palette_box, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_box_homogeneous_set(sd->palette_box, EINA_TRUE);

   elm_box_padding_set
     (sd->palette_box,
     (sd->h_pad * elm_widget_scale_get(obj) * elm_config_scale_get()),
     (sd->v_pad * elm_widget_scale_get(obj) * elm_config_scale_get()));

   elm_box_align_set(sd->palette_box, 0.5, 0.5);
   elm_layout_content_set(obj, "palette", sd->palette_box);
   sd->palette_name = eina_stringshare_add("default");
   _palette_colors_load(obj);
}

static void
_create_colorbars(Evas_Object *obj)
{
   ELM_COLORSELECTOR_DATA_GET(obj, sd);
   if (sd->col_bars_area) return;
   sd->col_bars_area = edje_object_add(evas_object_evas_get(obj));
   elm_widget_theme_object_set
     (obj, sd->col_bars_area, "colorselector", "bg",
     elm_widget_style_get(obj));
   elm_layout_content_set(obj, "selector", sd->col_bars_area);
   _hsl_to_rgb(sd);
   _color_bars_add(obj);
}

static Eina_Bool
_plane_access_read_cb(void *data, Evas_Object *obj __UNUSED__, Elm_Access_Action_Info *action_info)
{
   Evas_Coord ox, oy, ow, oh;
   Evas_Coord x, y;
   double drag_x = 0.0, drag_y = 0.0;

   ELM_COLORSELECTOR_DATA_GET(data, sd);

   evas_object_geometry_get(sd->col_plane, &ox, &oy, &ow, &oh);
   if ((action_info->x <= ox) || (action_info->x >= ox + ow) ||
       (action_info->y <= oy) || (action_info->y >= oy + oh))
     return EINA_FALSE;
   x = action_info->x - ox;
   y = action_info->y - oy;
   elm_object_signal_emit(sd->col_plane, "show", "elm");
   drag_x = (double)x / ow;
   drag_y = (double)y / oh;
   edje_object_part_drag_value_set(elm_layout_edje_get(sd->col_plane), "elm.touch", drag_x, drag_y);
   _colorplane_color_get(sd->col_plane, &(sd->r), &(sd->g), &(sd->b), &(sd->a));
   evas_object_smart_callback_call(data, SIG_CHANGED, NULL);
   return EINA_TRUE;
}

static void
_create_colorplane(Evas_Object *obj)
{
   ELM_COLORSELECTOR_DATA_GET(obj, sd);
   if (sd->col_plane) return;
   sd->col_plane = elm_layout_add(obj);
   elm_object_focus_allow_set(sd->col_plane, EINA_TRUE);
   elm_layout_edje_object_can_access_set(sd->col_plane, EINA_TRUE);
   elm_layout_theme_set(sd->col_plane, "colorselector", "colorplane",
                        elm_widget_style_get(obj));
   elm_object_signal_callback_add(sd->col_plane, "changed", "", _colorplane_cb, obj);

   if (_elm_config->access_mode == ELM_ACCESS_MODE_ON)
     {
        sd->col_plane_access_obj = elm_access_object_register(sd->col_plane, obj);
        _elm_access_text_set(_elm_access_object_get(sd->col_plane_access_obj),
                             ELM_ACCESS_TYPE, E_("IDS_ST_HEADER_COLOUR_PICKER_ABB"));
        _elm_access_callback_set(_elm_access_object_get(sd->col_plane_access_obj),
                                 ELM_ACCESS_STATE, _plane_access_state_cb, obj);
        elm_access_action_cb_set(sd->col_plane_access_obj, ELM_ACCESS_ACTION_READ,
                                 _plane_access_read_cb, obj);
     }
}

static void
_rotation_angle_set(Evas_Object *obj, double angle)
{
   Evas_Object *edje_obj;

   Edje_Message_Float_Set *msg = malloc(sizeof(Edje_Message_Float_Set));
   msg->count = 1;
   msg->val[0] = angle;
   edje_obj = elm_layout_edje_get(obj);
   edje_object_message_send(edje_obj, EDJE_MESSAGE_FLOAT_SET, 0, msg);

   free(msg);
}

static void
_colorcircle_mouse_move_cb(void *data,
               Evas *evas __UNUSED__,
               Evas_Object *obj __UNUSED__,
               void *event_info)
{
   Evas_Coord w, h, x, y;
   const Evas_Object *img;
   double r1, r2, degree = 0.0;
   Evas_Event_Mouse_Move *ev = event_info;
   ELM_COLORSELECTOR_DATA_GET(data, sd);

   img = edje_object_part_object_get(elm_layout_edje_get(sd->col_circle), "colorcircle_bg");
   if (!img) return;

   evas_object_image_size_get(img, &w, &h);
   evas_object_geometry_get(img, &x, &y, NULL, NULL);

   r1 = ev->cur.canvas.y - (y + h / 2) ;
   r2 = ev->cur.canvas.x - (x + w / 2);

   degree = (PI_DEGREE / PI) * atan2(r1, r2);
   degree = degree + PI_DEGREE;
   sd->degree = degree;
   _rotation_angle_set(sd->col_circle, degree);
   _colorcircle_color_get(data, &(sd->r), &(sd->g), &(sd->b), &(sd->a));
   evas_object_smart_callback_call(data, SIG_CHANGED, NULL);
}

static void
_colorcircle_color_get(Evas_Object *obj, int *r, int *g, int *b, int *a)
{
   if (!obj) return;
   const Evas_Object *img;
   Evas_Coord w, h, offset, x, y;
   double  x_off = 0.0, y_off = 0.0;
   unsigned char *origin_data, *p;

   ELM_COLORSELECTOR_DATA_GET(obj, sd);
   img = edje_object_part_object_get(elm_layout_edje_get(sd->col_circle), "colorcircle_bg");
   if (!img) return;

   evas_object_image_size_get(img, &w, &h);
   origin_data = evas_object_image_data_get(img, EINA_FALSE);

   x_off = sd->radius * cos(sd->degree * (PI / PI_DEGREE));
   y_off = sd->radius * sin(sd->degree * (PI / PI_DEGREE));
   x = w / 2 - x_off;
   y = h / 2 - y_off;

   offset = (y * w + x) * COLORSELECTOR_COLOR_COMPONENT_NUM;
   p = origin_data + offset;
   if (a) *a = *(p + COLORSELECTOR_COLOR_COMPONENT_A);
   if (r) *r = *(p + COLORSELECTOR_COLOR_COMPONENT_R);
   if (g) *g = *(p + COLORSELECTOR_COLOR_COMPONENT_G);
   if (b) *b = *(p + COLORSELECTOR_COLOR_COMPONENT_B);
}

static void
_colorcircle_color_set(Evas_Object *obj, int r, int g, int b, int a __UNUSED__)
{
   if (!obj) return;
   const Evas_Object *img;
   Evas_Coord w, h, x, y;
   int j;
   unsigned char *origin_data, *p;
   double x_off, y_off;
   ELM_COLORSELECTOR_DATA_GET(obj, sd);

   img = edje_object_part_object_get(elm_layout_edje_get(sd->col_circle), "colorcircle_bg");
   if(!img) return;

   evas_object_image_size_get(img, &w, &h);
   origin_data =  evas_object_image_data_get(img, EINA_FALSE);

   for (j = 0; j <= 360; j++)
     {
        x_off = sd->radius * cos(j * (PI / PI_DEGREE));
        y_off = sd->radius * sin(j * (PI / PI_DEGREE));
        x = w / 2 - x_off;
        y = h / 2 - y_off;
        p = origin_data + COLORSELECTOR_COLOR_COMPONENT_NUM * (x + y * w);
        if (*(p + COLORSELECTOR_COLOR_COMPONENT_R) == r &&
             *(p + COLORSELECTOR_COLOR_COMPONENT_G) == g &&
              *(p + COLORSELECTOR_COLOR_COMPONENT_B) == b)
          {
             sd->degree = j;
             _rotation_angle_set(sd->col_circle, j);
             return;
          }
     }
}

static void
_create_colorcircle(Evas_Object *obj)
{
   const char *radiusstr;
   ELM_COLORSELECTOR_DATA_GET(obj, sd);
   if (sd->col_circle) return;
   sd->col_circle = elm_layout_add(obj);
   elm_layout_theme_set(sd->col_circle, "colorselector", "colorcircle",
                        elm_widget_style_get(obj));
   radiusstr = edje_object_data_get(elm_layout_edje_get(sd->col_circle), "radius");
   if (radiusstr) sd->radius = atof(radiusstr);
   evas_object_event_callback_add(sd->col_circle, EVAS_CALLBACK_MOUSE_MOVE, _colorcircle_mouse_move_cb, obj);
}

EAPI Evas_Object *
elm_colorselector_add(Evas_Object *parent)
{
   Evas_Object *obj;
   const char *hpadstr, *vpadstr;
   unsigned int h_pad = DEFAULT_HOR_PAD;
   unsigned int v_pad = DEFAULT_VER_PAD;

   EINA_SAFETY_ON_NULL_RETURN_VAL(parent, NULL);

   obj = elm_widget_add(_elm_colorselector_smart_class_new(), parent);
   if (!obj) return NULL;

   if (!elm_widget_sub_object_add(parent, obj))
     ERR("could not add %p as sub object of %p", obj, parent);

   ELM_COLORSELECTOR_DATA_GET(obj, sd);

   elm_layout_theme_set
     (obj, "colorselector", "palette", elm_object_style_get(obj));

   hpadstr =
     edje_object_data_get(ELM_WIDGET_DATA(sd)->resize_obj, "horizontal_pad");
   if (hpadstr) h_pad = atoi(hpadstr) / edje_object_base_scale_get(ELM_WIDGET_DATA(sd)->resize_obj);
   vpadstr = edje_object_data_get
       (ELM_WIDGET_DATA(sd)->resize_obj, "vertical_pad");
   if (vpadstr) v_pad = atoi(vpadstr) / edje_object_base_scale_get(ELM_WIDGET_DATA(sd)->resize_obj);
   sd->h_pad = h_pad;
   sd->v_pad = v_pad;

   _create_colorpalette(obj);

   sd->mode = ELM_COLORSELECTOR_PALETTE;
   sd->focused = ELM_COLORSELECTOR_PALETTE;
   sd->sel_color_type = HUE;
   sd->selected = sd->items;
   sd->highlighted = sd->selected;
   sd->er = 255;
   sd->eg = 0;
   sd->eb = 0;
   sd->h = 0.0;
   sd->s = 1.0;
   sd->l = 0.0;
   sd->a = 255;
   sd->grab.x = -1;
   sd->grab.y = -1;
   sd->grab.xroot = -1;
   sd->grab.mouse_motion = NULL;
   sd->grab.mouse_up = NULL;
   sd->grab.key_up = NULL;
   sd->grab.in = EINA_TRUE;

   elm_layout_sizing_eval(obj);
   elm_widget_can_focus_set(obj, EINA_TRUE);

   //Tizen Only: This should be removed when eo is applied.
   ELM_WIDGET_DATA_GET(obj, wsd);
   wsd->on_create = EINA_FALSE;

   return obj;
}

EAPI void
elm_colorselector_color_set(Evas_Object *obj,
                            int r,
                            int g,
                            int b,
                            int a)
{
   ELM_COLORSELECTOR_CHECK(obj);
   ELM_COLORSELECTOR_DATA_GET(obj, sd);

  /* TODO: Should move this code inside_colors_set but with newly added picker code,
   * _colors_set is being called multiple times with different colors which creates
   * problem in setting the touch pointer.
   */
   if ((sd->mode == ELM_COLORSELECTOR_PLANE) || (sd->mode == ELM_COLORSELECTOR_ALL)
      || (sd->mode == ELM_COLORSELECTOR_PALETTE_PLANE))
     _colorplane_color_set(sd->col_plane, r, g, b, a);
   if ((sd->mode == ELM_COLORSELECTOR_ALL) || (sd->mode == ELM_COLORSELECTOR_COMPONENTS)
      || (sd->mode == ELM_COLORSELECTOR_BOTH))
     _colors_set(obj, r, g, b, a);
   if (sd->mode == ELM_COLORSELECTOR_CIRCLE)
     _colorcircle_color_set(obj, r, g, b, a);
}

EAPI void
elm_colorselector_color_get(const Evas_Object *obj,
                            int *r,
                            int *g,
                            int *b,
                            int *a)
{
   ELM_COLORSELECTOR_CHECK(obj);
   ELM_COLORSELECTOR_DATA_GET(obj, sd);

   if (r) *r = sd->r;
   if (g) *g = sd->g;
   if (b) *b = sd->b;
   if (a) *a = sd->a;
}

EAPI void
elm_colorselector_mode_set(Evas_Object *obj,
                           Elm_Colorselector_Mode mode)
{
   ELM_COLORSELECTOR_CHECK(obj);
   ELM_COLORSELECTOR_DATA_GET(obj, sd);

   if (sd->mode == mode) return;
   sd->mode = mode;

   evas_object_hide(elm_layout_content_unset(obj, "selector"));
   evas_object_hide(elm_layout_content_unset(obj, "palette"));
   evas_object_hide(elm_layout_content_unset(obj, "picker"));
   evas_object_hide(elm_layout_content_unset(obj, "plane"));

   switch (sd->mode)
     {
      case ELM_COLORSELECTOR_PALETTE:
        elm_layout_content_set(obj, "palette", sd->palette_box);
        elm_layout_signal_emit(obj, "elm,state,palette", "elm");
        sd->focused = ELM_COLORSELECTOR_PALETTE;
        break;

      case ELM_COLORSELECTOR_COMPONENTS:
        _create_colorbars(obj);
        elm_layout_content_set(obj, "selector", sd->col_bars_area);
        elm_layout_signal_emit(obj, "elm,state,components", "elm");
        sd->focused = ELM_COLORSELECTOR_COMPONENTS;
        sd->sel_color_type = HUE;
        break;

      case ELM_COLORSELECTOR_BOTH:
        _create_colorbars(obj);
        elm_layout_content_set(obj, "palette", sd->palette_box);
        elm_layout_content_set(obj, "selector", sd->col_bars_area);
        elm_layout_signal_emit(obj, "elm,state,both", "elm");
        sd->focused = ELM_COLORSELECTOR_PALETTE;
        break;

      case ELM_COLORSELECTOR_PICKER:
        _color_picker_add(obj, sd);
        elm_layout_content_set(obj, "picker", sd->picker);
        elm_layout_signal_emit(obj, "elm,state,picker", "elm");
        sd->focused = ELM_COLORSELECTOR_PICKER;
        break;

      case ELM_COLORSELECTOR_PLANE:
         _create_colorplane(obj);
         elm_layout_content_set(obj, "plane", sd->col_plane);
         elm_layout_signal_emit(obj, "elm,state,plane", "elm");
         sd->focused = ELM_COLORSELECTOR_PLANE;
         break;

      case ELM_COLORSELECTOR_CIRCLE:
         _create_colorcircle(obj);
         elm_layout_content_set(obj, "circle", sd->col_circle);
         elm_layout_signal_emit(obj, "elm,state,circle", "elm");
         sd->focused = ELM_COLORSELECTOR_CIRCLE;
         break;

      case ELM_COLORSELECTOR_PALETTE_PLANE:
         _create_colorplane(obj);
         elm_layout_content_set(obj, "palette", sd->palette_box);
         elm_layout_content_set(obj, "plane", sd->col_plane);
         elm_layout_signal_emit(obj, "elm,state,palette,plane", "elm");
         sd->focused = ELM_COLORSELECTOR_PALETTE;
         break;

      case ELM_COLORSELECTOR_ALL:
        _create_colorbars(obj);
        _create_colorplane(obj);
        _color_picker_add(obj, sd);
        elm_layout_content_set(obj, "picker", sd->picker);
        elm_layout_content_set(obj, "palette", sd->palette_box);
        elm_layout_content_set(obj, "selector", sd->col_bars_area);
        elm_layout_content_set(obj, "plane", sd->col_plane);
        elm_layout_signal_emit(obj, "elm,state,all", "elm");
        sd->focused = ELM_COLORSELECTOR_PALETTE;
        break;

      default:
        return;
     }

   edje_object_message_signal_process(ELM_WIDGET_DATA(sd)->resize_obj);

   elm_layout_sizing_eval(obj);
}

EAPI Elm_Colorselector_Mode
elm_colorselector_mode_get(const Evas_Object *obj)
{
   ELM_COLORSELECTOR_CHECK(obj) ELM_COLORSELECTOR_PALETTE;
   ELM_COLORSELECTOR_DATA_GET(obj, sd);

   return sd->mode;
}

EAPI void
elm_colorselector_palette_item_color_get(const Elm_Object_Item *it,
                                         int *r,
                                         int *g,
                                         int *b,
                                         int *a)
{
   Elm_Color_Item *item;

   ELM_COLORSELECTOR_ITEM_CHECK_OR_RETURN(it);

   item = (Elm_Color_Item *)it;
   if (item)
     {
        if (r) *r = item->color->r;
        if (g) *g = item->color->g;
        if (b) *b = item->color->b;
        if (a) *a = item->color->a;
     }
}

EAPI void
elm_colorselector_palette_item_color_set(Elm_Object_Item *it,
                                         int r,
                                         int g,
                                         int b,
                                         int a)
{
   Elm_Color_Item *item;

   ELM_COLORSELECTOR_ITEM_CHECK_OR_RETURN(it);

   item = (Elm_Color_Item *)it;
   item->color->r = r;
   item->color->g = g;
   item->color->b = b;
   item->color->a = a;
   evas_object_color_set
     (item->color_obj, item->color->r, item->color->g, item->color->b,
     item->color->a);

   _colors_save(WIDGET(it));
}

EAPI Elm_Object_Item *
elm_colorselector_palette_color_add(Evas_Object *obj,
                                    int r,
                                    int g,
                                    int b,
                                    int a)
{
   Elm_Color_Item *item;

   ELM_COLORSELECTOR_CHECK(obj) NULL;
   ELM_COLORSELECTOR_DATA_GET(obj, sd);

   if (sd->config_load)
     {
        _colors_remove(obj);
        sd->config_load = EINA_FALSE;
     }
   item = _item_new(obj);
   if (!item) return NULL;

   item->color = ELM_NEW(Elm_Color_RGBA);
   if (!item->color) return NULL;

   item->color->r = r;
   item->color->g = g;
   item->color->b = b;
   item->color->a = a;
   _elm_config_color_set
     (sd->palette_name, item->color->r, item->color->g, item->color->b,
     item->color->a);

   elm_box_pack_end(sd->palette_box, VIEW(item));
   evas_object_color_set
     (item->color_obj, item->color->r, item->color->g, item->color->b,
     item->color->a);

   sd->items = eina_list_append(sd->items, item);

   elm_layout_sizing_eval(obj);

   return (Elm_Object_Item *)item;
}

EAPI void
elm_colorselector_palette_clear(Evas_Object *obj)
{
   ELM_COLORSELECTOR_CHECK(obj);
   ELM_COLORSELECTOR_DATA_GET(obj, sd);

   _colors_remove(obj);
   if (sd->mode == ELM_COLORSELECTOR_BOTH)
     sd->focused = ELM_COLORSELECTOR_COMPONENTS;
   if (sd->mode == ELM_COLORSELECTOR_PALETTE_PLANE)
     sd->focused = ELM_COLORSELECTOR_PLANE;
}

EAPI Eina_List *
elm_colorselector_palette_items_get(const Evas_Object *obj)
{
   ELM_COLORSELECTOR_CHECK(obj) NULL;
   ELM_COLORSELECTOR_DATA_GET(obj, sd);

   return sd->items;
}

EAPI void
elm_colorselector_palette_name_set(Evas_Object *obj,
                                   const char *palette_name)
{
   ELM_COLORSELECTOR_CHECK(obj);
   ELM_COLORSELECTOR_DATA_GET(obj, sd);
   EINA_SAFETY_ON_NULL_RETURN(palette_name);

   if (!strcmp(sd->palette_name, palette_name)) return;

   eina_stringshare_replace(&sd->palette_name, palette_name);
   _items_del(sd);
   _palette_colors_load(obj);
}

EAPI const char *
elm_colorselector_palette_name_get(const Evas_Object *obj)
{
   ELM_COLORSELECTOR_CHECK(obj) NULL;
   ELM_COLORSELECTOR_DATA_GET(obj, sd);

   return sd->palette_name;
}
