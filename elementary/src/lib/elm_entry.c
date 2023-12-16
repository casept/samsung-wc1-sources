#include <Elementary.h>
#include <Elementary_Cursor.h>
#include "elm_priv.h"
#include "elm_widget_entry.h"
#include "elm_module_priv.h"   // TIZEN ONLY
// TIZEN ONLY
#ifdef HAVE_ELEMENTARY_X
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#endif
//

EAPI const char ELM_ENTRY_SMART_NAME[] = "elm_entry";

/* Maximum chunk size to be inserted to the entry at once
 * FIXME: This size is arbitrary, should probably choose a better size.
 * Possibly also find a way to set it to a low value for weak computers,
 * and to a big value for better computers. */
#define _CHUNK_SIZE 10000

static const char SIG_ABORTED[] = "aborted";
static const char SIG_ACTIVATED[] = "activated";
static const char SIG_ACCESS_CHANGED[] = "access,changed";
static const char SIG_ANCHOR_CLICKED[] = "anchor,clicked";
static const char SIG_ANCHOR_DOWN[] = "anchor,down";
static const char SIG_ANCHOR_HOVER_OPENED[] = "anchor,hover,opened";
static const char SIG_ANCHOR_IN[] = "anchor,in";
static const char SIG_ANCHOR_OUT[] = "anchor,out";
static const char SIG_ANCHOR_UP[] = "anchor,up";
static const char SIG_CHANGED[] = "changed";
static const char SIG_CHANGED_USER[] = "changed,user";
static const char SIG_CLICKED[] = "clicked";
static const char SIG_CLICKED_DOUBLE[] = "clicked,double";
static const char SIG_CLICKED_TRIPLE[] = "clicked,triple";
static const char SIG_CURSOR_CHANGED[] = "cursor,changed";
static const char SIG_CURSOR_CHANGED_MANUAL[] = "cursor,changed,manual";
static const char SIG_FOCUSED[] = "focused";
static const char SIG_LANG_CHANGED[] = "language,changed";
static const char SIG_LONGPRESSED[] = "longpressed";
static const char SIG_MAX_LENGTH[] = "maxlength,reached";
static const char SIG_PREEDIT_CHANGED[] = "preedit,changed";
static const char SIG_PRESS[] = "press";
static const char SIG_REDO_REQUEST[] = "redo,request";
static const char SIG_SELECTION_CHANGED[] = "selection,changed";
static const char SIG_SELECTION_CLEARED[] = "selection,cleared";
static const char SIG_SELECTION_COPY[] = "selection,copy";
static const char SIG_SELECTION_CUT[] = "selection,cut";
static const char SIG_SELECTION_PASTE[] = "selection,paste";
static const char SIG_SELECTION_START[] = "selection,start";
static const char SIG_TEXT_SET_DONE[] = "text,set,done";
static const char SIG_THEME_CHANGED[] = "theme,changed";
static const char SIG_UNDO_REQUEST[] = "undo,request";
static const char SIG_UNFOCUSED[] = "unfocused";
static const char SIG_REJECTED[] = "rejected";
static const char SIG_DRAG[] = "drag,start";
static const Evas_Smart_Cb_Description _smart_callbacks[] = {
   {SIG_ABORTED, ""},
   {SIG_ACTIVATED, ""},
   {SIG_ACCESS_CHANGED, ""},
   {SIG_ANCHOR_CLICKED, ""},
   {SIG_ANCHOR_DOWN, ""},
   {SIG_ANCHOR_HOVER_OPENED, ""},
   {SIG_ANCHOR_IN, ""},
   {SIG_ANCHOR_OUT, ""},
   {SIG_ANCHOR_UP, ""},
   {SIG_CHANGED, ""},
   {SIG_CHANGED_USER, ""},
   {SIG_CLICKED, ""},
   {SIG_CLICKED_DOUBLE, ""},
   {SIG_CLICKED_TRIPLE, ""},
   {SIG_CURSOR_CHANGED, ""},
   {SIG_CURSOR_CHANGED_MANUAL, ""},
   {SIG_FOCUSED, ""},
   {SIG_LANG_CHANGED, ""},
   {SIG_LONGPRESSED, ""},
   {SIG_MAX_LENGTH, ""},
   {SIG_PREEDIT_CHANGED, ""},
   {SIG_PRESS, ""},
   {SIG_REDO_REQUEST, ""},
   {SIG_SELECTION_CHANGED, ""},
   {SIG_SELECTION_CLEARED, ""},
   {SIG_SELECTION_COPY, ""},
   {SIG_SELECTION_CUT, ""},
   {SIG_SELECTION_PASTE, ""},
   {SIG_SELECTION_START, ""},
   {SIG_TEXT_SET_DONE, ""},
   {SIG_THEME_CHANGED, ""},
   {SIG_UNDO_REQUEST, ""},
   {SIG_UNFOCUSED, ""},
   {SIG_DRAG,""},
   {NULL, NULL}
};

static const Elm_Layout_Part_Alias_Description _content_aliases[] =
{
   {"icon", "elm.swallow.icon"},
   {"end", "elm.swallow.end"},
   {NULL, NULL}
};

static const Evas_Smart_Interface *_smart_interfaces[] =
{
   (Evas_Smart_Interface *)&ELM_SCROLLABLE_IFACE, NULL
};

EVAS_SMART_SUBCLASS_IFACE_NEW
  (ELM_ENTRY_SMART_NAME, _elm_entry, Elm_Entry_Smart_Class,
  Elm_Layout_Smart_Class, elm_layout_smart_class_get, _smart_callbacks,
  _smart_interfaces);

static Eina_List *entries = NULL;

struct _Mod_Api
{
   void (*obj_hook)(Evas_Object *obj);
   void (*obj_unhook)(Evas_Object *obj);
   void (*obj_longpress)(Evas_Object *obj);
   // TIZEN
   void (*obj_hidemenu) (Evas_Object *obj);
   void (*obj_mouseup) (Evas_Object *obj);
   //
};

////////////////////////// TIZEN ONLY - START
static void _hover_cancel_cb(void *data, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__);
static void _hover_selected_cb(void *data, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__);
static void _copy_cb(void *data, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__);
static void _cut_cb(void *data, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__);
static void _paste_cb(void *data, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__);
#ifndef ELM_FEATURE_WEARABLE
static void _menu_call(Evas_Object *obj);
#endif
static void _dnd_enter_cb(void *data, Evas_Object *obj);
static void _dnd_leave_cb(void *data, Evas_Object *obj);
static void _dnd_pos_cb(void *data, Evas_Object *obj, Evas_Coord x, Evas_Coord y, Elm_Xdnd_Action action);

static void
_select_all(void *data, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   ELM_ENTRY_DATA_GET(data, sd);

   if (!sd->sel_allow) return;
   sd->sel_mode = EINA_TRUE;

   if (edje_object_part_text_selection_get(sd->entry_edje, "elm.text") == NULL)
     {
        edje_object_part_text_select_none(sd->entry_edje, "elm.text");
        edje_object_signal_emit(sd->entry_edje, "elm,state,select,on", "elm");
        edje_object_part_text_select_all(sd->entry_edje, "elm.text");
     }
   else
     {
        edje_object_part_text_cursor_begin_set(sd->entry_edje, "elm.text",
                                               EDJE_CURSOR_SELECTION_BEGIN);
        edje_object_part_text_cursor_begin_set(sd->entry_edje, "elm.text",
                                               EDJE_CURSOR_SELECTION_END);
        edje_object_part_text_cursor_end_set(sd->entry_edje, "elm.text", EDJE_CURSOR_MAIN);
        edje_object_part_text_select_extend(sd->entry_edje, "elm.text");
#ifndef ELM_FEATURE_WEARABLE
        _menu_call(data);
#endif
     }
}

static void
_select_word(void *data, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   ELM_ENTRY_DATA_GET(data, sd);

   if (!sd->sel_allow) return;
   sd->sel_mode = EINA_TRUE;

   if (!_elm_config->desktop_entry)
     {
          edje_object_part_text_select_allow_set
            (sd->entry_edje, "elm.text", EINA_TRUE);
     }
   edje_object_signal_emit(sd->entry_edje, "elm,state,select,on", "elm");
   edje_object_part_text_select_word(sd->entry_edje, "elm.text");
}

#ifdef HAVE_ELEMENTARY_X
static Ecore_X_Window
_cbhm_window_get()
{
   Ecore_X_Atom x_atom_cbhm = ecore_x_atom_get("CBHM_XWIN");
   Ecore_X_Window x_cbhm_win = 0;
   unsigned char *buf = NULL;
   int num = 0;
   int ret = ecore_x_window_prop_property_get(0, x_atom_cbhm, XA_WINDOW, 0, &buf, &num);
   if (ret && num)
     memcpy(&x_cbhm_win, buf, sizeof(Ecore_X_Window));
   if (buf)
     free(buf);
   return x_cbhm_win;
}
#endif

static Eina_Bool
_cbhm_msg_send(Evas_Object *obj, char *msg)
{
#ifdef HAVE_ELEMENTARY_X
   Ecore_X_Window x_cbhm_win = _cbhm_window_get();
   Ecore_X_Atom x_atom_cbhm_msg = ecore_x_atom_get("CBHM_MSG");
   Ecore_X_Window xwin = elm_win_xwindow_get(obj);

   if (!x_cbhm_win || !x_atom_cbhm_msg)
     return EINA_FALSE;

   XClientMessageEvent m;
   memset(&m, 0, sizeof(m));
   m.type = ClientMessage;
   m.display = ecore_x_display_get();
   m.window = xwin;
   m.message_type = x_atom_cbhm_msg;
   m.format = 8;
   snprintf(m.data.b, 20, "%s", msg);

   XSendEvent(ecore_x_display_get(), x_cbhm_win, False, NoEventMask, (XEvent*)&m);

   ecore_x_sync();
   return EINA_TRUE;
#else
   return EINA_FALSE;
#endif
}

static Eina_Bool
_xclient_msg_cb(void *data, int type __UNUSED__, void *event)
{
#ifdef HAVE_ELEMENTARY_X
   Evas_Object *obj = (Evas_Object *)data;
   ELM_ENTRY_DATA_GET(data, sd);
   Ecore_X_Event_Client_Message *ev = event;

   if (ev->message_type != ecore_x_atom_get("CBHM_MSG"))
     return ECORE_CALLBACK_PASS_ON;

   if (!strcmp("SET_OWNER", ev->data.b))
     {
        if (elm_object_focus_get(obj) == EINA_TRUE)
          {
             ecore_x_selection_secondary_set(elm_win_xwindow_get(data), NULL, 0);

             if (sd->cnp_mode != ELM_CNP_MODE_MARKUP)
               _cbhm_msg_send(data, "show0");
             else
               _cbhm_msg_send(data, "show1");
          }
     }
#endif
   return ECORE_CALLBACK_PASS_ON;
}

static void
_magnifier_hide(void *data)
{
   ELM_ENTRY_DATA_GET(data, sd);

   if (!sd->magnifier_showing) return;
   evas_object_hide(sd->mgf_bg);
   elm_object_scroll_freeze_pop(data);
   sd->magnifier_showing = EINA_FALSE;
}

static void
_magnifier_show(void *data)
{
   ELM_ENTRY_DATA_GET(data, sd);

   if (sd->magnifier_showing) return;
   evas_object_show(sd->mgf_bg);
   elm_object_scroll_freeze_push(data);
   sd->magnifier_showing = EINA_TRUE;
}

static Evas_Object *
_entry_createicon(void *data,
      Evas_Object *parent, Evas_Coord *xoff, Evas_Coord *yoff)
{
   int xm, ym;
   Evas_Object *icon;
   const char *text;
   Evas_Coord sel_x = 0, sel_y = 0, sel_h = 0, sel_w = 0, margin_w = 0, margin_h = 0;
   ELM_ENTRY_DATA_GET(data, sd);

   icon = evas_object_textblock_add(evas_object_evas_get(parent));
   text = edje_object_part_text_selection_get(sd->entry_edje, "elm.text");

   int r, g, b, a;
   evas_object_color_get(data, &r, &g, &b, &a);
   evas_object_color_set(icon, r, g, b, a);

   // Copy style
   Evas_Textblock_Style *tb_style =
      (Evas_Textblock_Style *)evas_object_textblock_style_get(elm_entry_textblock_get(data));
   evas_object_textblock_style_set(icon, tb_style);
   evas_object_textblock_size_formatted_get(icon, &margin_w, &margin_h);

   evas_object_textblock_text_markup_set(icon, text);

   edje_object_part_text_selection_geometry_get
      (sd->entry_edje, "elm.text", &sel_x, &sel_y, &sel_w, &sel_h);

   evas_object_size_hint_weight_set(icon, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(icon, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_move(icon, sel_x, sel_y);
   evas_object_resize(icon, sel_w + margin_w, sel_h + margin_h);
   evas_object_show(icon);

   evas_pointer_canvas_xy_get(evas_object_evas_get(data), &xm, &ym);
   if (xoff) *xoff = xm - (sel_w / 2);
   if (yoff) *yoff = ym - (sel_h / 2);

   return icon;
}

static void
_magnifier_proxy_update(Evas_Object *obj, double scale)
{
   ELM_ENTRY_DATA_GET(obj, sd);

   Evas_Coord x, y, w, h;
   Evas_Coord bx, bw;
   Evas_Coord cx, cy, cw, ch;
   Evas_Coord mh, ox, oy;
   Evas_Coord ctnx = 0, ctny = 0;
   Evas_Coord swx, sww;
   double px, py, pw, ph;

   evas_object_geometry_get(sd->mgf_bg, &bx, NULL, &bw, NULL);
   edje_object_part_text_cursor_geometry_get(sd->entry_edje, "elm.text",
                                             &cx, &cy, &cw, &ch);
   cy += (ch / 2);
   evas_object_geometry_get(obj, &x, &y, &w, &h);

   if (sd->scroll)
     {
        sd->s_iface->content_pos_get(obj, &ox, &oy);
        cx -= ox;
        cy -= oy;
        edje_object_part_geometry_get(sd->scr_edje, "elm.swallow.content",
                                      &ctnx, &ctny, NULL, NULL);
     }

   evas_object_geometry_get(sd->mgf_proxy, NULL, NULL, NULL, &mh);
   edje_object_part_geometry_get(sd->mgf_bg, "swallow", &swx, NULL, &sww, NULL);
   /* px, py are the scaled offsets that indicates the start
      position of the magnifier viewport area. */
   px = -(cx * scale) + (cx + x + ctnx - bx) - swx - ctnx * scale;
   if (cx + x + ctnx <= bx + swx)
     {
        //show cursor at first pos
        px = -(cx * scale) - ctnx * scale;
     }
   else if (x + ctnx + cx + cw * scale >= bx + swx + sww)
     {
        //show cursor at last pos
        double right_off;
        const char *type = NULL;
        Evas_Object *top = elm_widget_top_get(obj);
        Evas_Coord tx = 0, tw = 0;
        if (top)
          {
             type = evas_object_type_get(top);
             if (type && !strcmp(type, "elm_win"))
               evas_object_geometry_get(top, &tx, NULL, &tw, NULL);
          }
        right_off = tx + tw - (bx + swx + sww);
        px = px - (cw * scale + right_off);
     }
   py = -(cy * scale) + mh * 0.5 - ctny * scale;
   pw = w * scale;
   ph = h * scale;
   evas_object_image_fill_set(sd->mgf_proxy, px, py, pw, ph);
}

static void
_magnifier_move(void *data)
{
   ELM_ENTRY_DATA_GET(data, sd);

   Evas_Coord x, y, w, h;
   Evas_Coord cx, cy, ch, ox = 0, oy = 0;
   Evas_Coord mw, mh;
   Evas_Coord sww, swh, adjh;
   Evas_Coord bdh;
   Evas_Coord ctnx = 0, ctny = 0;
   double adj_scale;
   Evas_Coord mx, my;
   Evas_Object *top;
   const char *type = NULL;
   const char *key_data = NULL;

   edje_object_part_text_cursor_geometry_get(sd->entry_edje, "elm.text",
                                             &cx, &cy, NULL, &ch);
   if (sd->scroll)
     {
        evas_object_geometry_get(sd->scr_edje, &x, &y, &w, &h);
        sd->s_iface->content_pos_get(data, &ox, &oy);
        cx -= ox;
        cy -= oy;
     }
   else
     evas_object_geometry_get(data, &x, &y, &w, &h);

   edje_object_part_geometry_get(sd->mgf_bg, "bg", NULL, NULL, &mw, &mh);
   edje_object_part_geometry_get(sd->mgf_bg, "swallow", NULL, NULL, &sww, &swh);
   bdh = mh - swh;
   if (sd->scroll)
     {
        edje_object_part_geometry_get(sd->scr_edje, "elm.swallow.content",
                                      &ctnx, &ctny, NULL, NULL);
        ox = oy = 0;
     }

   adjh = ch * sd->mgf_scale + mh - swh;
   if (adjh < sd->mgf_height)
     adjh = sd->mgf_height;

   key_data = edje_object_data_get(sd->mgf_bg, "max_height");
   if (key_data)
     {
        if (adjh > atoi(key_data))
          {
             adjh = atoi(key_data);
          }
     }

   /* move to fully show magnifier */
   if (cy + y - sd->mgf_arrow_height - adjh < 0)
     {
        oy = adjh - (cy + y - sd->mgf_arrow_height);
     }


   //adjusting scale and size
   adj_scale = sd->mgf_scale;

   evas_object_resize(sd->mgf_bg, mw, adjh);

   mx = cx + x - (mw / 2) + ctnx;
   my = cy + y - sd->mgf_arrow_height - adjh + oy + ctny;
   evas_object_move(sd->mgf_bg, mx, my);

   // keep magnifier inside window
   top = elm_widget_top_get(data);
   if (top)
     {
        type = evas_object_type_get(top);
        if (type && !strcmp(type, "elm_win"))
          {
             Evas_Coord wx, wy, wh, ww;
             evas_object_geometry_get(top, &wx, &wy, &ww, &wh);
             if (mx < wx)
               evas_object_move(sd->mgf_bg, wx, my);
             else if (mx + mw > wx + ww)
               evas_object_move(sd->mgf_bg, wx + ww - mw, my);
          }
     }

   _magnifier_proxy_update(data, adj_scale);
}

static void
_magnifier_create(void *data)
{
   ELM_ENTRY_DATA_GET(data, sd);

   Evas_Coord x, y, w, h, mw, mh;
   Evas *e;
   const char* key_data;
   double elm_scale;

   if (sd->mgf_proxy) evas_object_del(sd->mgf_proxy);
   if (sd->mgf_bg) evas_object_del(sd->mgf_bg);

   if (sd->scroll)
     evas_object_geometry_get(sd->scr_edje, &x, &y, &w, &h);
   else
     evas_object_geometry_get(data, &x, &y, &w, &h);

   if ((w <= 0) || (h <= 0)) return;

   sd->mgf_bg = edje_object_add(evas_object_evas_get(data));

   _elm_theme_object_set(data, sd->mgf_bg, "entry", "magnifier",
                         "fixed-size");

   e = evas_object_evas_get(data);

   key_data = edje_object_data_get(sd->mgf_bg, "height");
   if (key_data) sd->mgf_height = atoi(key_data);
   key_data = edje_object_data_get(sd->mgf_bg, "scale");
   if (key_data)
     {
        struct lconv *lc;
        lc = localeconv();
        if (lc && lc->decimal_point && strcmp(lc->decimal_point, "."))
          {
             char local[128];
             strncpy(local, key_data, sizeof(local));
             local[127] = '\0';
             /* change '.' to local decimal point (ex: ',') */
             int j = 0;
             while(local[j] != 0)
               {
                  if (local[j] == '.')
                    {
                       local[j] = lc->decimal_point[0];
                       break;
                    }
                  j++;
               }
             sd->mgf_scale = atof(local);
          }
        else
          sd->mgf_scale = atof(key_data);
     }
   else
     sd->mgf_scale = 1.0;

   key_data = edje_object_data_get(sd->mgf_bg, "arrow");
   if (key_data)
     sd->mgf_arrow_height = atoi(key_data);
   else
     sd->mgf_arrow_height = 0;

   elm_scale = elm_config_scale_get();
   sd->mgf_height = (int)((float)sd->mgf_height /
                          edje_object_base_scale_get(sd->mgf_bg) * elm_scale);

   sd->mgf_proxy = evas_object_image_add(e);
   edje_object_part_swallow(sd->mgf_bg, "swallow", sd->mgf_proxy);
   evas_object_image_source_set(sd->mgf_proxy, data);

   /* Tizen Only: Since Proxy has the texture size limitation problem,
      we set a key value for evas works in some hackish way to avoid this
      problem. This hackish code should be removed once evas supports
      a mechanism like a virtual texture. */
   evas_object_data_set(sd->mgf_proxy, "tizenonly", (void *) 1);

   // REDWOOD ONLY (20140617): support setting mgf color from app
   if (sd->mgf_bg_color_set)
   {
      Edje_Message_Int_Set *msg;
      msg = malloc(sizeof(Edje_Message_Int_Set) + 3 * sizeof(int));
      if (!msg) return;
      msg->count = 4;
      msg->val[0] = sd->mgf_r;
      msg->val[1] = sd->mgf_g;
      msg->val[2] = sd->mgf_b;
      msg->val[3] = sd->mgf_a;
      edje_object_message_send(sd->mgf_bg, EDJE_MESSAGE_INT_SET, 1, msg);
      free(msg);
   }
   //

   mw = (Evas_Coord)((float)w * sd->mgf_scale);
   mh = (Evas_Coord)((float)h * sd->mgf_scale);
   if ((mw <= 0) || (mh <= 0))
     return;

   evas_object_layer_set(sd->mgf_bg, EVAS_LAYER_MAX);
   evas_object_layer_set(sd->mgf_proxy, EVAS_LAYER_MAX);
}

static void
_signal_long_pressed(void *data, Evas_Object *obj __UNUSED__, const char *emission __UNUSED__, const char *source __UNUSED__)
{
   ELM_ENTRY_DATA_GET(data, sd);
   if (sd->disabled) return;

   sd->long_pressed = EINA_TRUE;
   sd->hit_selection = (sd->have_selection) ? /* For text DND */
      edje_object_part_text_xy_in_selection_get(sd->entry_edje,
                                                "elm.text", sd->downx, sd->downy) : EINA_FALSE;
   if (!sd->hit_selection)
     sd->drag_started = EINA_FALSE;
   else if (sd->drag_enabled && !sd->password)
     {
        const char *sel =
           edje_object_part_text_selection_get(sd->entry_edje, "elm.text");

        if ((sel) && (sel[0]))
          {
             sd->drag_started = EINA_TRUE;
             evas_object_smart_callback_call(data, SIG_DRAG, NULL);
             elm_drag_start(data, ELM_SEL_FORMAT_TEXT,
                            sel,
                            ELM_XDND_ACTION_COPY,
                            _entry_createicon, data,
                            NULL /*Elm_Drag_Pos dragpos*/,
                            NULL /*void *dragdata*/,
                            NULL /*Elm_Drag_Accept acceptcb*/,
                            NULL /*void *acceptdata*/,
                            NULL /*Elm_Drag_Done dragdone*/,
                            NULL /*void *donecbdata*/);
             edje_object_part_text_cursor_handler_disabled_set
                (sd->entry_edje, "elm.text", EINA_TRUE);
             _hover_cancel_cb(data, NULL, NULL);
          }
     }
   if (!sd->drag_started)
     {
        _hover_cancel_cb(data, NULL, NULL);
        if (sd->magnifier_enabled)
          {
             if ((sd->api) && (sd->api->obj_hidemenu))
               sd->api->obj_hidemenu(data);
             if (sd->has_text)
               {
                  _magnifier_create(data);
                  _magnifier_move(data);
                  _magnifier_show(data);
               }
          }
     }
}

static void
_signal_handler_move_start_cb(void *data, Evas_Object *obj __UNUSED__, const char *emission __UNUSED__, const char *source __UNUSED__)
{
   ELM_ENTRY_DATA_GET(data, sd);

   elm_object_scroll_freeze_push(data);

   if ((sd->api) && (sd->api->obj_hidemenu))
     sd->api->obj_hidemenu(data);

   if (sd->magnifier_enabled)
     {
        if (sd->has_text)
          {
             _magnifier_create(data);
             _magnifier_move(data);
             _magnifier_show(data);
          }
     }
}

static void
_signal_handler_move_end_cb(void *data, Evas_Object *obj __UNUSED__, const char *emission __UNUSED__, const char *source __UNUSED__)
{
   elm_object_scroll_freeze_pop(data);

   _magnifier_hide(data);
#ifndef ELM_FEATURE_WEARABLE
   _menu_call(data);
#endif
}

static void
_signal_handler_moving_cb(void *data, Evas_Object *obj __UNUSED__, const char *emission __UNUSED__, const char *source __UNUSED__)
{
   ELM_ENTRY_DATA_GET(data, sd);

   if ((sd->magnifier_enabled) && (sd->has_text))
     {
        _magnifier_move(data);
        _magnifier_show(data);
     }
}

static void
_signal_handler_click_cb(void *data, Evas_Object *obj __UNUSED__, const char *emission __UNUSED__, const char *source __UNUSED__)
{
   _magnifier_hide(data);
#ifndef ELM_FEATURE_WEARABLE
   _menu_call(data);
#endif
   elm_widget_scroll_freeze_pop(data);
}

static Evas_Coord_Rectangle
_intersection_region_get(Evas_Coord_Rectangle rect1, Evas_Coord_Rectangle rect2)
{
   Evas_Coord_Rectangle ret_rect;
   Evas_Coord_Point l1, l2, r1, r2, p1, p2;

   l1.x = rect1.x;
   l1.y = rect1.y;
   r1.x = rect1.x + rect1.w;
   r1.y = rect1.y + rect1.h;

   l2.x = rect2.x;
   l2.y = rect2.y;
   r2.x = rect2.x + rect2.w;
   r2.y = rect2.y + rect2.h;

   p1.x = (l1.x > l2.x) ? l1.x : l2.x;
   p1.y = (l1.y > l2.y) ? l1.y : l2.y;
   p2.x = (r1.x < r2.x) ? r1.x : r2.x;
   p2.y = (r1.y < r2.y) ? r1.y : r2.y;

   ret_rect.x = p1.x;
   ret_rect.y = p1.y;
   ret_rect.w = (p2.x > p1.x) ? p2.x - p1.x : -1;
   ret_rect.h = (p2.y > p1.y) ? p2.y - p1.y : -1;

   return ret_rect;
}

static Evas_Coord_Rectangle
_viewport_region_get(Evas_Object *data)
{
   Evas_Object *parent_obj;
   Evas_Coord_Rectangle geometry, ret_rect;
   geometry.x = geometry.y = geometry.w = geometry.h = -1;
   ret_rect = geometry;

   ELM_ENTRY_DATA_GET(data, sd);
   if (!data || !strlen(elm_widget_type_get(data))) return geometry;

   if (sd->scroll)
     {
        sd->s_iface->content_viewport_size_get(data, &geometry.w, &geometry.h);
        sd->s_iface->content_viewport_pos_get(data, &geometry.x, &geometry.y);
     }
   else
     {
        evas_object_geometry_get(data,
                                 &geometry.x,
                                 &geometry.y,
                                 &geometry.w,
                                 &geometry.h);
     }
   ret_rect = geometry;

   parent_obj = data;

   while ((parent_obj = elm_widget_parent_get(parent_obj)))
     {
        if (!strcmp(elm_widget_type_get(parent_obj), "elm_scroller") ||
            !strcmp(elm_widget_type_get(parent_obj), "elm_genlist"))
          {
             evas_object_geometry_get(parent_obj, &geometry.x, &geometry.y, &geometry.w, &geometry.h);
             if ((ret_rect.w == -1) && (ret_rect.h == -1)) ret_rect = geometry;
             ret_rect = _intersection_region_get(geometry, ret_rect);
          }
     }

   return ret_rect;
}

static Evas_Coord_Rectangle
_layout_region_get(Evas_Object *data)
{
   Evas_Coord_Rectangle geometry;
   geometry.x = geometry.y = geometry.w = geometry.h = -1;

   if (!data || !strlen(elm_widget_type_get(data))) return geometry;

   Evas_Object *child_obj = data;
   Evas_Object *parent_obj;

   while ((parent_obj = elm_widget_parent_get(child_obj)))
     {
        if (!strcmp(elm_widget_type_get(parent_obj), "elm_conformant"))
          {
             evas_object_geometry_get(child_obj, &geometry.x, &geometry.y, &geometry.w, &geometry.h);
             return geometry;
          }
        child_obj = parent_obj;
     }

   return geometry;
}

static void
_region_recalc_job(void *data)
{
   ELM_ENTRY_DATA_GET(data, sd);
   Evas_Coord_Rectangle ret_rect;

   sd->region_recalc_job = NULL;

   if (!_elm_config->desktop_entry)
     {
        ret_rect = _viewport_region_get(data);
        edje_object_part_text_viewport_region_set(sd->entry_edje, "elm.text",
                                                  ret_rect.x, ret_rect.y,
                                                  ret_rect.w, ret_rect.h);
        ret_rect = _layout_region_get(data);
        edje_object_part_text_layout_region_set(sd->entry_edje, "elm.text",
                                                ret_rect.x, ret_rect.y,
                                                ret_rect.w, ret_rect.h);
     }
}

static void
_region_get_job(void *data)
{
   ELM_ENTRY_DATA_GET(data, sd);

   sd->region_get_job = NULL;

   if (!_elm_config->desktop_entry)
     {
        if (sd->region_recalc_job) ecore_job_del(sd->region_recalc_job);
        sd->region_recalc_job = ecore_job_add(_region_recalc_job, data);

        //evas_smart_objects_calculate(evas_object_evas_get(data)); //remove: too many calculation
     }
}

static void
_signal_selection_end(void *data, Evas_Object *obj __UNUSED__, const char *emission __UNUSED__, const char *source __UNUSED__)
{
   ELM_ENTRY_DATA_GET(data, sd);
   if (sd->disabled) return;

   if (sd->magnifier_enabled)
     _magnifier_hide(data);
#ifndef ELM_FEATURE_WEARABLE
   _menu_call(data);
#endif
}

static void
_signal_magnifier_changed(void *data, Evas_Object *obj __UNUSED__, const char *emission __UNUSED__, const char *source __UNUSED__)
{
   ELM_ENTRY_DATA_GET(data, sd);
   Evas_Coord cx, cy, cw, ch;

   edje_object_part_text_cursor_geometry_get(sd->entry_edje, "elm.text", &cx, &cy, &cw, &ch);
   if (!sd->deferred_recalc_job)
     elm_widget_show_region_set(data, cx, cy, cw, ch, EINA_FALSE);
   else
     {
        sd->deferred_cur = EINA_TRUE;
        sd->cx = cx;
        sd->cy = cy;
        sd->cw = cw;
        sd->ch = ch;
     }
}

static void
_elm_entry_key_down_cb(void *data, Evas *e __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info)
{
   Evas_Event_Key_Down *ev = event_info;
   if (!ev->keyname) return;

   ELM_ENTRY_DATA_GET(data, sd);
   if ((sd->api) && (sd->api->obj_hidemenu) && !sd->sel_mode)
     sd->api->obj_hidemenu(data);
}

static void
_keep_selection(void *data __UNUSED__, Evas_Object *obj, void *event_info __UNUSED__)
{
   ELM_ENTRY_DATA_GET(obj, sd);

   sd->start_sel_pos = edje_object_part_text_cursor_pos_get(sd->entry_edje, "elm.text",
                                                            EDJE_CURSOR_SELECTION_BEGIN);
   sd->end_sel_pos = edje_object_part_text_cursor_pos_get(sd->entry_edje, "elm.text",
                                                          EDJE_CURSOR_SELECTION_END);
}

static void
_paste_translation(void *data, Evas_Object *obj, void *event_info)
{
   ELM_ENTRY_DATA_GET(obj, sd);

   elm_entry_cursor_pos_set(obj, sd->start_sel_pos);
   elm_entry_cursor_selection_begin(obj);
   elm_entry_cursor_pos_set(obj, sd->end_sel_pos);
   elm_entry_cursor_selection_end(obj);
   if (data)
     _elm_entry_entry_paste(obj, data);
   if ((data) && (event_info))
     _elm_entry_entry_paste(obj, " ");
   if (event_info)
     _elm_entry_entry_paste(obj, event_info);
}

static void
_is_selected_all(void *data, Evas_Object *obj, void *event_info __UNUSED__)
{
   if ((!data) || (!obj)) return;
   ELM_ENTRY_DATA_GET(obj, sd);
   int pos;
   Eina_Bool *ret = (Eina_Bool *)data;

   pos = edje_object_part_text_cursor_pos_get(sd->entry_edje, "elm.text",
                                              EDJE_CURSOR_SELECTION_BEGIN);
   edje_object_part_text_cursor_pos_set(sd->entry_edje, "elm.text",
                                        EDJE_CURSOR_USER, pos);
   if (edje_object_part_text_cursor_prev(sd->entry_edje, "elm.text", EDJE_CURSOR_USER))
     {
        *ret = EINA_FALSE;
        return;
     }
   else
     {
        pos = edje_object_part_text_cursor_pos_get(sd->entry_edje, "elm.text",
                                                   EDJE_CURSOR_SELECTION_END);
        edje_object_part_text_cursor_pos_set(sd->entry_edje, "elm.text",
                                             EDJE_CURSOR_USER, pos);
        if (edje_object_part_text_cursor_next(sd->entry_edje, "elm.text", EDJE_CURSOR_USER))
          {
             *ret = EINA_FALSE;
             return;
          }
     }
   *ret = EINA_TRUE;
}

void elm_entry_extension_module_data_get(Evas_Object *obj, Elm_Entry_Extension_data *ext_mod)
{
   ELM_ENTRY_DATA_GET(obj, sd);

   ext_mod->cancel = _hover_cancel_cb;
   ext_mod->copy = _copy_cb;
   ext_mod->cut = _cut_cb;
   ext_mod->paste = _paste_cb;
   ext_mod->select = _select_word;
   ext_mod->selectall = _select_all;
   ext_mod->ent = sd->entry_edje;
   ext_mod->items = sd->items;
   ext_mod->editable = sd->editable;
   ext_mod->have_selection = sd->have_selection;
   ext_mod->password = sd->password;
   ext_mod->selmode = sd->sel_mode;
   ext_mod->context_menu = sd->context_menu;
   ext_mod->cnp_mode = sd->cnp_mode;
   ext_mod->viewport_rect = _viewport_region_get(obj);
   ext_mod->keep_selection = _keep_selection;
   ext_mod->paste_translation = _paste_translation;
   ext_mod->is_selected_all = _is_selected_all;
}

#ifdef HAVE_ELEMENTARY_X
/* Example of env vars:
 * ILLUME_KBD="0, 0, 800, 301"
 * ILLUME_IND="0, 0, 800, 32"
 * ILLUME_STK="0, 568, 800, 32"
 */
static Eina_Bool
_sip_part_geometry_get_from_env(const char *part,
                                int *sx,
                                int *sy,
                                int *sw,
                                int *sh)
{
   const char delimiters[] = " ,;";
   char *env_val, *token;
   char buf[PATH_MAX];
   int tsx, tsy, tsw;

   if (!(env_val = getenv(part))) return EINA_FALSE;

   /* strtok would modify env var if not copied to a buffer */
   strncpy(buf, env_val, sizeof(buf));
   buf[PATH_MAX - 1] = '\0';

   token = strtok(buf, delimiters);
   if (!token) return EINA_FALSE;
   tsx = atoi(token);

   token = strtok(NULL, delimiters);
   if (!token) return EINA_FALSE;
   tsy = atoi(token);

   token = strtok(NULL, delimiters);
   if (!token) return EINA_FALSE;
   tsw = atoi(token);

   token = strtok(NULL, delimiters);
   if (!token) return EINA_FALSE;
   *sh = atoi(token);

   *sx = tsx;
   *sy = tsy;
   *sw = tsw;

   return EINA_TRUE;
}

static void
_sip_geometry_get(Evas_Object *obj)
{
   Ecore_X_Window zone = 0;
   Evas_Object *top;
   Ecore_X_Window xwin;
   Eina_Rectangle unirect, srect, vrect, crect;
   Evas_Coord tx, ty, tw, th;
   Evas_Coord ww = 0, wh = 0;

   ELM_ENTRY_DATA_GET(obj, sd);

   unirect.x = unirect.y = unirect.w = unirect.h = -1;
   vrect.x = vrect.y = vrect.w = vrect.h = -1;
   srect.x = srect.y = srect.w = srect.h = -1;
   crect.x = crect.y = crect.w = crect.h = -1;
   top = elm_widget_top_get(obj);
   xwin = elm_win_xwindow_get(top);

   if (xwin)
     zone = ecore_x_e_illume_zone_get(xwin);

   //Virtual Keypad
   if ((!_sip_part_geometry_get_from_env
        ("ILLUME_KBD", &vrect.x, &vrect.y, &vrect.w, &vrect.h)) && (xwin))
     {
        if (!ecore_x_e_illume_keyboard_geometry_get
            (xwin, &vrect.x, &vrect.y, &vrect.w, &vrect.h))
          {
             if (!ecore_x_e_illume_keyboard_geometry_get
                 (zone, &vrect.x, &vrect.y, &vrect.w, &vrect.h))
               vrect.x = vrect.y = vrect.w = vrect.h = 0;
          }
     }

   //Softkey
   if ((!_sip_part_geometry_get_from_env
        ("ILLUME_STK", &srect.x, &srect.y, &srect.w, &srect.h)) && (xwin))
     {
        if (!ecore_x_e_illume_softkey_geometry_get
            (zone, &srect.x, &srect.y, &srect.w, &srect.h))
          srect.x = srect.y = srect.w = srect.h = 0;
     }

   //Clipboard
   if ((!_sip_part_geometry_get_from_env
        ("ILLUME_CB", &crect.x, &crect.y, &crect.w, &crect.h)) && (xwin))
     {
        if (!ecore_x_e_illume_clipboard_geometry_get
            (zone, &crect.x, &crect.y, &crect.w, &crect.h))
          crect.x = crect.y = crect.w = crect.h = 0;
     }

   if (!eina_rectangle_is_empty(&vrect))
     {
        unirect.x = vrect.x;
        unirect.y = vrect.y;
        unirect.w = vrect.w;
        unirect.h = vrect.h;
     }
   if (!eina_rectangle_is_empty(&srect))
     {
        if (eina_rectangle_is_empty(&unirect))
          {
             unirect.x = srect.x;
             unirect.y = srect.y;
             unirect.w = srect.w;
             unirect.h = srect.h;
          }
        else
          {
             eina_rectangle_union(&unirect, &srect);
          }
     }
   if (!eina_rectangle_is_empty(&crect))
     {
        if (eina_rectangle_is_empty(&unirect))
          {
             unirect.x = crect.x;
             unirect.y = crect.y;
             unirect.w = crect.w;
             unirect.h = crect.h;
          }
        else
          {
             eina_rectangle_union(&unirect, &crect);
          }
     }

   tx = ty = tw = th = -1;
   evas_object_geometry_get(top, &tx, &ty, &tw, &th);
   if (xwin)
     ecore_x_window_geometry_get(xwin, NULL, NULL, &ww, &wh);
   if (eina_rectangle_is_empty(&unirect))
     {
        edje_object_part_text_layout_region_set(sd->entry_edje, "elm.text", tx, ty, tw, th);
     }
   else
     {
        Evas_Coord lx, ly, lw, lh;
        if (unirect.y > 0)
          {
             lx = tx;
             ly = ty;
             lw = tw;
             lh = unirect.y - ly;
          }
        else
          {
             lx = tx;
             ly = ty > unirect.y + unirect.h ? ty : unirect.y + unirect.h;
             lw = tw;
             lh = ty - ly;
          }
        if ((lw > ww) && (ww > 0)) lw = ww;
        if ((lh > wh) && (wh > 0)) lh = wh;
        edje_object_part_text_layout_region_set(sd->entry_edje, "elm.text", lx, ly, lw, lh);
     }
}

static Eina_Bool
_on_prop_change(void *data,
                int type __UNUSED__,
                void *event __UNUSED__)
{
   Ecore_X_Event_Window_Property *ev = event;

   if ((ev->atom == ECORE_X_ATOM_E_ILLUME_ZONE) ||
       (ev->atom == ECORE_X_ATOM_E_ILLUME_CLIPBOARD_STATE) ||
       (ev->atom == ECORE_X_ATOM_E_VIRTUAL_KEYBOARD_STATE) ||
       (ev->atom == ECORE_X_ATOM_E_ILLUME_SOFTKEY_GEOMETRY) ||
       (ev->atom == ECORE_X_ATOM_E_ILLUME_KEYBOARD_GEOMETRY) ||
       (ev->atom == ECORE_X_ATOM_E_ILLUME_CLIPBOARD_GEOMETRY))
     {
        if (elm_widget_focus_get(data))
          _sip_geometry_get(data);
     }

   return ECORE_CALLBACK_PASS_ON;
}
#endif
////////////////////////// TIZEN ONLY - END

static Mod_Api *
_module_find(Evas_Object *obj __UNUSED__)
{
   static Elm_Module *m = NULL;

   if (m) goto ok;  // already found - just use
   if (!(m = _elm_module_find_as("entry/api"))) return NULL;
   // get module api
   m->api = malloc(sizeof(Mod_Api));
   if (!m->api) return NULL;

   ((Mod_Api *)(m->api))->obj_hook = // called on creation
     _elm_module_symbol_get(m, "obj_hook");
   ((Mod_Api *)(m->api))->obj_unhook = // called on deletion
     _elm_module_symbol_get(m, "obj_unhook");
   ((Mod_Api *)(m->api))->obj_longpress = // called on long press menu
     _elm_module_symbol_get(m, "obj_longpress");
   //TIZEN ONLY
   ((Mod_Api *)(m->api))->obj_hidemenu = // called on hide menu
     _elm_module_symbol_get(m, "obj_hidemenu");
   ((Mod_Api *)(m->api))->obj_mouseup = // called on mouseup
     _elm_module_symbol_get(m, "obj_mouseup");
   //
ok: // ok - return api
   return m->api;
}

static char *
_file_load(const char *file)
{
   Eina_File *f;
   char *text = NULL;
   void *tmp = NULL;

   f = eina_file_open(file, EINA_FALSE);
   if (!f) return NULL;

   tmp = eina_file_map_all(f, EINA_FILE_SEQUENTIAL);
   if (!tmp) goto on_error;

   text = malloc(eina_file_size_get(f) + 1);
   if (!text) goto on_error;

   memcpy(text, tmp, eina_file_size_get(f));
   text[eina_file_size_get(f)] = 0;

   if (eina_file_map_faulted(f, tmp))
     {
        free(text);
        text = NULL;
     }

 on_error:
   if (tmp) eina_file_map_free(f, tmp);
   eina_file_close(f);

   return text;
}

static char *
_plain_load(const char *file)
{
   char *text;

   text = _file_load(file);
   if (text)
     {
        char *text2;

        text2 = elm_entry_utf8_to_markup(text);
        free(text);
        return text2;
     }

   return NULL;
}

static Eina_Bool
_load_do(Evas_Object *obj)
{
   char *text;

   ELM_ENTRY_DATA_GET(obj, sd);

   if (!sd->file)
     {
        elm_object_text_set(obj, "");
        return EINA_TRUE;
     }

   switch (sd->format)
     {
      case ELM_TEXT_FORMAT_PLAIN_UTF8:
        text = _plain_load(sd->file);
        break;

      case ELM_TEXT_FORMAT_MARKUP_UTF8:
        text = _file_load(sd->file);
        break;

      default:
        text = NULL;
        break;
     }

   if (text)
     {
        elm_object_text_set(obj, text);
        free(text);

        return EINA_TRUE;
     }
   else
     {
        elm_object_text_set(obj, "");

        return EINA_FALSE;
     }
}

static void
_utf8_markup_save(const char *file,
                  const char *text)
{
   FILE *f;

   if ((!text) || (!text[0]))
     {
        ecore_file_unlink(file);

        return;
     }

   f = fopen(file, "wb");
   if (!f)
     {
        // FIXME: report a write error
        return;
     }

   fputs(text, f); // FIXME: catch error
   fclose(f);
}

static void
_utf8_plain_save(const char *file,
                 const char *text)
{
   char *text2;

   text2 = elm_entry_markup_to_utf8(text);
   if (!text2)
     return;

   _utf8_markup_save(file, text2);
   free(text2);
}

static void
_save_do(Evas_Object *obj)
{
   ELM_ENTRY_DATA_GET(obj, sd);

   if (!sd->file) return;
   switch (sd->format)
     {
      case ELM_TEXT_FORMAT_PLAIN_UTF8:
        _utf8_plain_save(sd->file, elm_object_text_get(obj));
        break;

      case ELM_TEXT_FORMAT_MARKUP_UTF8:
        _utf8_markup_save(sd->file, elm_object_text_get(obj));
        break;

      default:
        break;
     }
}

static Eina_Bool
_delay_write(void *data)
{
   ELM_ENTRY_DATA_GET(data, sd);

   _save_do(data);
   sd->delay_write = NULL;

   return ECORE_CALLBACK_CANCEL;
}

static void
_elm_entry_guide_update(Evas_Object *obj,
                        Eina_Bool has_text)
{
   ELM_ENTRY_DATA_GET(obj, sd);

   if ((has_text) && (!sd->has_text))
     edje_object_signal_emit(sd->entry_edje, "elm,guide,disabled", "elm");
   else if ((!has_text) && (sd->has_text))
     edje_object_signal_emit(sd->entry_edje, "elm,guide,enabled", "elm");

   sd->has_text = has_text;
}

static Elm_Entry_Markup_Filter *
_filter_new(Elm_Entry_Filter_Cb func,
            void *data)
{
   Elm_Entry_Markup_Filter *tf = ELM_NEW(Elm_Entry_Markup_Filter);

   if (!tf) return NULL;

   tf->func = func;
   tf->orig_data = data;
   if (func == elm_entry_filter_limit_size)
     {
        Elm_Entry_Filter_Limit_Size *lim = data, *lim2;

        if (!data)
          {
             free(tf);

             return NULL;
          }

        lim2 = malloc(sizeof(Elm_Entry_Filter_Limit_Size));
        if (!lim2)
          {
             free(tf);

             return NULL;
          }
        memcpy(lim2, lim, sizeof(Elm_Entry_Filter_Limit_Size));
        tf->data = lim2;
     }
   else if (func == elm_entry_filter_accept_set)
     {
        Elm_Entry_Filter_Accept_Set *as = data, *as2;

        if (!data)
          {
             free(tf);

             return NULL;
          }
        as2 = malloc(sizeof(Elm_Entry_Filter_Accept_Set));
        if (!as2)
          {
             free(tf);

             return NULL;
          }
        if (as->accepted)
          as2->accepted = eina_stringshare_add(as->accepted);
        else
          as2->accepted = NULL;
        if (as->rejected)
          as2->rejected = eina_stringshare_add(as->rejected);
        else
          as2->rejected = NULL;
        tf->data = as2;
     }
   else
     tf->data = data;
   return tf;
}

static void
_filter_free(Elm_Entry_Markup_Filter *tf)
{
   if (tf->func == elm_entry_filter_limit_size)
     {
        Elm_Entry_Filter_Limit_Size *lim = tf->data;

        if (lim) free(lim);
     }
   else if (tf->func == elm_entry_filter_accept_set)
     {
        Elm_Entry_Filter_Accept_Set *as = tf->data;

        if (as)
          {
             if (as->accepted) eina_stringshare_del(as->accepted);
             if (as->rejected) eina_stringshare_del(as->rejected);

             free(as);
          }
     }
   free(tf);
}

static void
_mirrored_set(Evas_Object *obj,
              Eina_Bool rtl)
{
   ELM_ENTRY_DATA_GET(obj, sd);

   edje_object_mirrored_set(sd->entry_edje, rtl);

   if (sd->anchor_hover.hover)
     elm_widget_mirrored_set(sd->anchor_hover.hover, rtl);
}

static const char *
_elm_entry_theme_group_get(Evas_Object *obj)
{
   ELM_ENTRY_DATA_GET(obj, sd);

   if (sd->editable)
     {
        if (sd->password) return "base-password";
        else
          {
             if (sd->single_line) return "base-single";
             else
               {
                  switch (sd->line_wrap)
                    {
                     case ELM_WRAP_CHAR:
                       return "base-charwrap";

                     case ELM_WRAP_WORD:
                       return "base";

                     case ELM_WRAP_MIXED:
                       return "base-mixedwrap";

                     case ELM_WRAP_NONE:
                     default:
                       return "base-nowrap";
                    }
               }
          }
     }
   else
     {
        if (sd->password) return "base-password";
        else
          {
             if (sd->single_line) return "base-single-noedit";
             else
               {
                  switch (sd->line_wrap)
                    {
                     case ELM_WRAP_CHAR:
                       return "base-noedit-charwrap";

                     case ELM_WRAP_WORD:
                       return "base-noedit";

                     case ELM_WRAP_MIXED:
                       return "base-noedit-mixedwrap";

                     case ELM_WRAP_NONE:
                     default:
                       return "base-nowrap-noedit";
                    }
               }
          }
     }
}

#ifdef HAVE_ELEMENTARY_X
static Eina_Bool
_drag_drop_cb(void *data __UNUSED__,
              Evas_Object *obj,
              Elm_Selection_Data *drop)
{
   Eina_Bool rv;

   ELM_ENTRY_DATA_GET(obj, sd);

   rv = edje_object_part_text_cursor_coord_set
       (sd->entry_edje, "elm.text", EDJE_CURSOR_MAIN, drop->x, drop->y);

   if (!rv) WRN("Warning: Failed to position cursor: paste anyway");

   elm_entry_entry_insert(obj, drop->data);

   return EINA_TRUE;
}

static Elm_Sel_Format
_get_drop_format(Evas_Object *obj)
{
   ELM_ENTRY_DATA_GET(obj, sd);

   if ((sd->editable) && (!sd->single_line) && (!sd->password) && (!sd->disabled))
     return ELM_SEL_FORMAT_MARKUP | ELM_SEL_FORMAT_IMAGE;
   return ELM_SEL_FORMAT_MARKUP;
}
#endif

/* we can't reuse layout's here, because it's on entry_edje only */
static Eina_Bool
_elm_entry_smart_disable(Evas_Object *obj)
{
   ELM_ENTRY_DATA_GET(obj, sd);

#ifdef HAVE_ELEMENTARY_X
   elm_drop_target_del(obj, sd->drop_format,
                       _dnd_enter_cb, obj,
                       _dnd_leave_cb, obj,
                       _dnd_pos_cb, obj,
                       _drag_drop_cb, NULL);
   sd->drop_added = EINA_FALSE;
#endif
   if (elm_object_disabled_get(obj))
     {
        if (sd->sel_allow)
          edje_object_part_text_select_disabled_set(sd->entry_edje, "elm.text", EINA_TRUE);
        edje_object_signal_emit(sd->entry_edje, "elm,state,disabled", "elm");
        if (sd->scroll)
          {
             edje_object_signal_emit(sd->scr_edje, "elm,state,disabled", "elm");
             sd->s_iface->freeze_set(obj, EINA_TRUE);
          }
        sd->disabled = EINA_TRUE;
     }
   else
     {
        if (sd->sel_allow)
          edje_object_part_text_select_disabled_set(sd->entry_edje, "elm.text", EINA_FALSE);
        edje_object_signal_emit(sd->entry_edje, "elm,state,enabled", "elm");
        if (sd->scroll)
          {
             edje_object_signal_emit(sd->scr_edje, "elm,state,enabled", "elm");
             sd->s_iface->freeze_set(obj, EINA_FALSE);
          }
        sd->disabled = EINA_FALSE;
#ifdef HAVE_ELEMENTARY_X
        if (sd->drop_enabled)
          {
             sd->drop_format = _get_drop_format(obj);
             elm_drop_target_add(obj, sd->drop_format,
                                 _dnd_enter_cb, obj,
                                 _dnd_leave_cb, obj,
                                 _dnd_pos_cb, obj,
                                 _drag_drop_cb, NULL);
             sd->drop_added = EINA_TRUE;
          }
#endif
     }

   return EINA_TRUE;
}

// TIZEN ONLY(130911)
// When cursor geometry is changed by theme or tag, show region geometry needs to be updated.
static void
_cursor_geometry_update(Evas_Object *obj)
{
   Evas_Coord cx, cy, cw, ch, rx, ry, rw, rh;

   ELM_ENTRY_DATA_GET(obj, sd);
   Elm_Widget_Smart_Data *wsd = evas_object_smart_data_get(obj);

   edje_object_part_text_cursor_geometry_get
      (sd->entry_edje, "elm.text", &cx, &cy, &cw, &ch);
   elm_widget_show_region_get(obj, &rx, &ry, &rw, &rh);

   if (cw != rw && cw != 0)
     wsd->rw = cw;
   if (ch != rh && ch != 0)
     wsd->rh = ch;
}
//

// TIZEN ONLY(131106): Add rectangle for processing mouse up event on paddings inside of entry.
static void
_event_rect_cursor_changed_signal_cb(void *data,
                                     Evas_Object *obj __UNUSED__,
                                     const char *emission __UNUSED__,
                                     const char *source __UNUSED__)
{
   ELM_ENTRY_DATA_GET(data, sd);

   edje_object_signal_callback_del
     (sd->entry_edje, "cursor,changed", "elm.text",
     _event_rect_cursor_changed_signal_cb);

   evas_object_smart_callback_call(data, SIG_CURSOR_CHANGED_MANUAL, NULL);
}

static void
_event_rect_mouse_up_cb(void *data,
                        Evas *e __UNUSED__,
                        Evas_Object *obj __UNUSED__,
                        void *event_info)
{
   Evas_Event_Mouse_Up *ev;
   Evas_Object *tb;
   Evas_Coord ox, oy, ow, oh;
   Evas_Coord tx, ty, tw, th;
   Evas_Coord cx, cy;
   Evas_Coord ex, ey, ew, eh;
   Evas_Textblock_Cursor *tc;
   int pos;

   ELM_ENTRY_DATA_GET(data, sd);

   ev = (Evas_Event_Mouse_Up *)event_info;
   if (ev->event_flags & EVAS_EVENT_FLAG_ON_HOLD)
     return;

   tb = elm_entry_textblock_get(data);
   evas_object_geometry_get(data, &ox, &oy, &ow, &oh);
   if ((ev->canvas.x <= ox) || (ev->canvas.x >= ox + ow) ||
       (ev->canvas.y <= oy) || (ev->canvas.y >= oy + oh))
     return;
   edje_object_part_geometry_get(sd->entry_edje, "elm.text", &ex, &ey, &ew, &eh);
   evas_object_geometry_get(tb, &tx, &ty, &tw, &th);

   cx = ev->canvas.x - tx;
   cy = ev->canvas.y - ty;
   if (cy >= eh) cy = eh - 1;
   if (cy < ey) cy = 0;
   if (cx >= ew) cx = ew - 1;
   if (cx < 0) cx = 0;

   tc = evas_object_textblock_cursor_new(tb);
   evas_textblock_cursor_cluster_coord_set(tc, cx, cy);
   pos = evas_textblock_cursor_pos_get(tc);

   if (pos != elm_entry_cursor_pos_get(data))
     {
        edje_object_signal_callback_add
           (sd->entry_edje, "cursor,changed", "elm.text",
            _event_rect_cursor_changed_signal_cb, data);
     }
   elm_entry_cursor_pos_set(data, pos);
   evas_textblock_cursor_free(tc);

   elm_entry_select_none(data);
   evas_object_smart_callback_call(data, SIG_CLICKED, NULL);

   if (!elm_widget_disabled_get(data) &&
       !evas_object_freeze_events_get(data))
     {
        if (sd->editable && sd->input_panel_enable)
          edje_object_part_text_input_panel_show(sd->entry_edje, "elm.text");
     }
}
////////////////////////////////

////////////////////////////////////////////////
// TIZEN_ONLY(131221): Customize accessiblity for editfield
////////////////////////////////////////////////

static char *
_access_info_cb(void *data, Evas_Object *obj __UNUSED__)
{
   const char *txt;

   ELM_ENTRY_DATA_GET(data, sd);

   if ((sd->password && _elm_config->access_password_read_enable) ||
       (!sd->password))
     {
        txt = elm_widget_access_info_get(data);

        if (!txt) txt = _elm_util_mkup_to_text(elm_entry_entry_get(data));
        if (txt && (strlen(txt) > 0)) return strdup(txt);
     }
   else if (sd->password && !(_elm_config->access_password_read_enable))
     {
        Eina_Strbuf *buf = eina_strbuf_new();
        char *text = elm_entry_markup_to_utf8(elm_entry_entry_get(data));
        char *ret;

        eina_strbuf_append_printf(buf, E_("IDS_ACCS_BODY_PD_CHARACTERS_TTS"), evas_string_char_len_get(text));
        ret = eina_strbuf_string_steal(buf);

        free(text);
        eina_strbuf_free(buf);
        return ret;
     }

   /* to take care guide text */
   txt = edje_object_part_text_get(sd->entry_edje, "elm.guide");
   txt = _elm_util_mkup_to_text(txt);
   if (txt && (strlen(txt) > 0)) return strdup(txt);

   return NULL;
}

static char *
_access_state_cb(void *data, Evas_Object *obj __UNUSED__)
{
   Evas_Object *top;
   Eina_Strbuf *buf;
   char *ret;

   ELM_ENTRY_DATA_GET(data, sd);

   top = elm_widget_top_get(data);
   ret = NULL;
   buf = eina_strbuf_new();

   if (elm_widget_disabled_get(data))
     eina_strbuf_append(buf, E_("IDS_ACCS_BODY_DISABLED_TTS"));

   if (!sd->editable)
     {
        if (!eina_strbuf_length_get(buf))
          eina_strbuf_append(buf, E_("State: Not Editable"));
        else eina_strbuf_append(buf, E_(", Not Editable"));
     }
   else if (!elm_widget_disabled_get(data))
     {
        if ((elm_object_focus_get(data) && (elm_win_keyboard_mode_get(top) == ELM_WIN_KEYBOARD_OFF) && !sd->on_activate) ||
            (!elm_object_focus_get(data)))
          eina_strbuf_append(buf, E_("WDS_TTS_TBBODY_DOUBLE_TAP_TO_EDIT"));
        else
          eina_strbuf_append(buf, E_("IDS_TPLATFORM_BODY_EDITING_T_TTS"));
     }

   if (!eina_strbuf_length_get(buf)) goto buf_free;

   ret = eina_strbuf_string_steal(buf);

buf_free:
   eina_strbuf_free(buf);
   return ret;
}

static char *
_access_context_info_cb(void *data, Evas_Object *obj __UNUSED__)
{
   Evas_Object *top;
   Eina_Strbuf *buf;
   char *ret;

   ELM_ENTRY_DATA_GET(data, sd);

   top = elm_widget_top_get(data);
   ret = NULL;
   buf = eina_strbuf_new();

   if (sd->editable && !elm_widget_disabled_get(data))
     {
        if (!((elm_object_focus_get(data) && (elm_win_keyboard_mode_get(top) == ELM_WIN_KEYBOARD_OFF) && !sd->on_activate) ||
            (!elm_object_focus_get(data))))
          {
             if (sd->on_activate)
               sd->on_activate = EINA_FALSE;
             if (!elm_entry_is_empty(data))
               eina_strbuf_append(buf, E_("IDS_TPLATFORM_BODY_FLICK_UP_AND_DOWN_TO_ADJUST_THE_POSITION_T_TTS"));
          }
     }

   if (!eina_strbuf_length_get(buf)) goto buf_free;

   ret = eina_strbuf_string_steal(buf);

buf_free:
   eina_strbuf_free(buf);
   return ret;
}

// TIZEN_ONLY(20140625): Add elm_entry_anchor_access_provider_* APIs.
static const char *
_elm_entry_anchor_access_string_get(Evas_Object *obj,
                                    const char *name,
                                    const char *text)
{
   Eina_List *l;
   char *ret;
   Elm_Entry_Anchor_Access_Provider *ap;

   ELM_ENTRY_DATA_GET(obj, sd);

   EINA_LIST_FOREACH(sd->anchor_access_providers, l, ap)
     {
        ret = ap->func(ap->data, obj, name, text);
        if (ret) return ret;
     }
   return text;
}
//

// TIZEN_ONLY(20140625): Support accessibility for entry anchors.
static void
_elm_entry_anchors_highlight_clear(Evas_Object *obj)
{
   char *data;

   ELM_ENTRY_DATA_GET(obj, sd);

   if (sd->anchor_highlight_rect)
     {
        data = (char *)evas_object_data_del(sd->anchor_highlight_rect, "anchor_name");
        if (data) free(data);
        data = (char *)evas_object_data_del(sd->anchor_highlight_rect, "anchor_text");
        if (data) free(data);
        evas_object_del(sd->anchor_highlight_rect);
        sd->anchor_highlight_rect = NULL;
     }
   sd->anchor_highlight_pos = -1;
}
//

// TIZEN_ONLY(20140625): Support accessibility for entry anchors.
static void
_elm_entry_hide_cb(void *data __UNUSED__,
                   Evas *e __UNUSED__,
                   Evas_Object *obj,
                   void *event_info __UNUSED__)
{
   _elm_entry_anchors_highlight_clear(obj);
}
//

// TIZEN_ONLY(20140625): Support accessibility for entry anchors.
static Eina_Bool
_elm_entry_anchors_highlight_set(Evas_Object *obj, int pos)
{
   Evas_Textblock_Rectangle *r;
   Evas_Object *tb = elm_entry_textblock_get(obj);
   Evas *e = evas_object_evas_get(obj);
   const Eina_List *anchors_a, *l;
   Eina_List *ll, *ll_next, *range = NULL;
   Evas_Coord x, y, w, h;
   int count = 0;

   ELM_ENTRY_DATA_GET(obj, sd);

   if (pos < 0)
     {
        sd->anchor_highlight_pos = -1;
        return EINA_FALSE;
     }
   sd->anchor_highlight_pos = pos;
   evas_object_geometry_get(sd->entry_edje, &x, &y, &w, &h);
   anchors_a = evas_textblock_node_format_list_get(tb, "a");

   if (anchors_a)
     {
        const Evas_Object_Textblock_Node_Format *node;
        Evas_Textblock_Cursor *start, *end;

        EINA_LIST_FOREACH(anchors_a, l, node)
          {
             if (count != pos)
               {
                  count++;
                  continue;
               }

             const char *s = evas_textblock_node_format_text_get(node);
             char *p, *name = NULL;

             p = strstr(s, "href=");
             if (p)
               {
                  name = strdup(p + 5);
               }

             start = evas_object_textblock_cursor_new(tb);
             end = evas_object_textblock_cursor_new(tb);
             evas_textblock_cursor_at_format_set(start, node);
             evas_textblock_cursor_copy(start, end);

             node = evas_textblock_node_format_next_get(node);
             for (; node; node = evas_textblock_node_format_next_get(node))
               {
                  s = evas_textblock_node_format_text_get(node);
                  if ((!strcmp(s, "- a")) || (!strcmp(s, "-a")))
                    break;
               }

             if (node)
               {
                  Evas_Coord sx, sy, ex, ey;
                  Evas_Object *rect = NULL;
                  char *text, *last_name, *last_text;
                  // TIZEN_ONLY(20140625): Add elm_entry_anchor_access_provider_* APIs.
                  const char *access_string;
                  //

                  sx = w;
                  sy = h;
                  ex = 0;
                  ey = 0;

                  evas_textblock_cursor_at_format_set(end, node);
                  text = (char *)evas_textblock_cursor_range_text_get(
                     start, end, EVAS_TEXTBLOCK_TEXT_PLAIN);

                  if (!(sd->anchor_highlight_rect))
                    {
                       rect = edje_object_add(e);
                       _elm_theme_object_set(obj, rect, "access", "base", "default");
                       evas_object_smart_member_add(rect, obj);
                       evas_object_repeat_events_set(rect, EINA_TRUE);
                       sd->anchor_highlight_rect = rect;
                    }
                  else
                    {
                       last_name = evas_object_data_del(sd->anchor_highlight_rect, "anchor_name");
                       last_text = evas_object_data_del(sd->anchor_highlight_rect, "anchor_text");
                       if (last_name) free(last_name);
                       if (last_text) free(last_text);
                    }

                  range = evas_textblock_cursor_range_geometry_get(start, end);
                  EINA_LIST_FOREACH_SAFE(range, ll, ll_next, r)
                    {
                       if (sx > r->x) sx = r->x;
                       if (sy > r->y) sy = r->y;
                       if (ex < r->x + r->w) ex = r->x + r->w;
                       if (ey < r->y + r->h) ey = r->y + r->h;

                       free(r);
                       range = eina_list_remove_list(range, ll);
                    }
                  elm_widget_show_region_set(obj, sx, sy, ex - sx, ey - sy, EINA_FALSE);

                  evas_object_geometry_get(sd->entry_edje, &x, &y, &w, &h);
                  evas_object_resize(sd->anchor_highlight_rect, ex - sx, ey - sy);
                  evas_object_move(sd->anchor_highlight_rect, sx + x, sy + y);
                  evas_object_show(sd->anchor_highlight_rect);
                  evas_object_data_set(sd->anchor_highlight_rect, "anchor_name", name);
                  evas_object_data_set(sd->anchor_highlight_rect, "anchor_text", text);

                  // TIZEN_ONLY(20140625): Add elm_entry_anchor_access_provider_* APIs.
                  access_string = _elm_entry_anchor_access_string_get(obj, name, text);
                  _elm_access_say(obj, access_string, EINA_FALSE);
                  //
                  return EINA_TRUE;
               }
             else
               {
                  if (name)
                    free(name);
                  evas_textblock_cursor_free(start);
                  evas_textblock_cursor_free(end);
                  return EINA_TRUE;
               }
          }
     }
   sd->anchor_highlight_pos = -1;
   return EINA_FALSE;
}
//

// TIZEN_ONLY(20140625): Support accessibility for entry anchors.
static void
_elm_entry_anchor_highlight_activate(Evas_Object *obj, int pos)
{
   Elm_Entry_Anchor_Info ei;

   ELM_ENTRY_DATA_GET(obj, sd);

   if (pos != sd->anchor_highlight_pos)
     _elm_entry_anchors_highlight_set(obj, pos);

   ei.button = 1;
   ei.name = evas_object_data_get(sd->anchor_highlight_rect, "anchor_name");
   ei.x = ei.y = ei.w = ei.h = 0;

   if (!sd->disabled)
     evas_object_smart_callback_call(obj, SIG_ANCHOR_CLICKED, &ei);
}
//

static Eina_Bool
_access_action_cb(void *data, Evas_Object *obj, Elm_Access_Action_Info *action_info)
{
   if (!action_info) return EINA_FALSE;
   char *cp = NULL;
   int pos = -1;
   ELM_ENTRY_DATA_GET(data, sd);

   switch (action_info->action_type)
     {
      case ELM_ACCESS_ACTION_UP:
        if (sd->sel_allow && sd->sel_mode)
          elm_entry_select_none(data);
        elm_entry_cursor_prev(data);
        cp = elm_entry_cursor_content_get(data);
        _elm_access_say(data, cp, EINA_FALSE);
        break;

      case ELM_ACCESS_ACTION_DOWN:
        if (sd->sel_allow && sd->sel_mode)
          elm_entry_select_none(data);
        elm_entry_cursor_next(data);
        cp = elm_entry_cursor_content_get(data);
        _elm_access_say(data, cp, EINA_FALSE);
        break;

      case ELM_ACCESS_ACTION_ACTIVATE:
        if (!elm_widget_disabled_get(data) &&
            !evas_object_freeze_events_get(data))
          {
             if (!elm_widget_focus_get(data))
               elm_object_focus_set(data, EINA_TRUE);
             else if (sd->editable)
               {
                  pos = elm_entry_cursor_pos_get(data);
                  if (pos > 0)
                    {
                       elm_entry_cursor_begin_set(data);
                       _elm_access_say(data, E_("IDS_SCR_OPT_CURSOR_AT_BEGINNING_OF_TEXT_TTS"), EINA_FALSE);
                    }
                  else
                    {
                       elm_entry_cursor_end_set(data);
                       _elm_access_say(data, E_("IDS_SCR_OPT_CURSOR_AT_END_OF_TEXT_TTS"), EINA_FALSE);
                    }
               }

             // TIZEN_ONLY(20140625): Support accessibility for entry anchors.
             if (!(sd->editable) && (sd->anchor_highlight_pos > -1))
               _elm_entry_anchor_highlight_activate(data, sd->anchor_highlight_pos);
             //

             evas_object_smart_callback_call(data, SIG_CLICKED, NULL);
             if (sd->editable && sd->input_panel_enable)
               {
                  edje_object_part_text_input_panel_show(sd->entry_edje, "elm.text");
                  sd->on_activate = EINA_TRUE;
               }
          }
        break;

      // TIZEN_ONLY(20140625): Support accessibility for entry anchors.
      case ELM_ACCESS_ACTION_HIGHLIGHT_NEXT:
        if (!elm_widget_disabled_get(data) &&
            !evas_object_freeze_events_get(data))
          {
             if (sd->editable)
               return EINA_FALSE;
             if (_elm_entry_anchors_highlight_set(data, sd->anchor_highlight_pos + 1))
               {
                  _elm_access_sound_play(ELM_ACCESS_SOUND_HIGHLIGHT);
                  return EINA_TRUE;
               }
             _elm_entry_anchors_highlight_clear(data);
             return EINA_FALSE;
          }
        break;

      case ELM_ACCESS_ACTION_HIGHLIGHT_PREV:
        if (!elm_widget_disabled_get(data) &&
            !evas_object_freeze_events_get(data))
          {
             if (elm_entry_editable_get(data) || (sd->anchor_highlight_pos == -1))
               return EINA_FALSE;
             if (_elm_entry_anchors_highlight_set(data, sd->anchor_highlight_pos - 1))
               {
                  _elm_access_sound_play(ELM_ACCESS_SOUND_HIGHLIGHT);
                  return EINA_TRUE;
               }
             _elm_entry_anchors_highlight_clear(data);
             elm_access_highlight_set(obj);
             return EINA_TRUE;
          }
        break;

      case ELM_ACCESS_ACTION_UNHIGHLIGHT:
        _elm_entry_anchors_highlight_clear(data);
        return EINA_FALSE;
      //

      default:
        return EINA_FALSE;
        break;
     }
   return EINA_TRUE;
}

static void
_access_obj_process(Evas_Object *obj, Eina_Bool is_access)
{
   Evas_Object *ao;

   ELM_ENTRY_DATA_GET(obj, sd);

   if (is_access)
     {
        ao = elm_access_object_get(elm_entry_textblock_get(obj));

        if (!ao)
          {
             ao = _elm_access_edje_object_part_object_register
                (obj, sd->entry_edje, "elm.text");
             _elm_access_text_set(_elm_access_object_get(ao),
                                  ELM_ACCESS_TYPE, E_("IDS_TPLATFORM_BODY_EDIT_FIELD_M_NOUN_T_TTS"));
             _elm_access_callback_set(_elm_access_object_get(ao),
                                      ELM_ACCESS_INFO, _access_info_cb, obj);
             _elm_access_callback_set(_elm_access_object_get(ao),
                                      ELM_ACCESS_STATE, _access_state_cb, obj);
             _elm_access_callback_set(_elm_access_object_get(ao),
                                      ELM_ACCESS_CONTEXT_INFO, _access_context_info_cb, obj);
             elm_access_action_cb_set(ao, ELM_ACCESS_ACTION_ACTIVATE,
                                      _access_action_cb, obj);
             elm_access_action_cb_set(ao, ELM_ACCESS_ACTION_UP,
                                      _access_action_cb, obj);
             elm_access_action_cb_set(ao, ELM_ACCESS_ACTION_DOWN,
                                      _access_action_cb, obj);
             elm_access_action_cb_set(ao, ELM_ACCESS_ACTION_HIGHLIGHT_NEXT,
                                      _access_action_cb, obj);
             elm_access_action_cb_set(ao, ELM_ACCESS_ACTION_HIGHLIGHT_PREV,
                                      _access_action_cb, obj);
             elm_access_action_cb_set(ao, ELM_ACCESS_ACTION_UNHIGHLIGHT,
                                      _access_action_cb, obj);
          }
     }
   else
     {
        ao = elm_access_object_get(elm_entry_textblock_get(obj));
        if (!ao) return;

        _elm_access_edje_object_part_object_unregister
           (obj, sd->entry_edje, "elm.text");
     }
}

static Eina_Bool
_elm_entry_smart_focus_next(const Evas_Object *obj,
                            Elm_Focus_Direction dir,
                            Evas_Object **next)
{
   Eina_List *items = NULL;
   Evas_Object *clear;

   if (!_elm_access_auto_highlight_get())
     {
        *next = (Evas_Object *)obj;
        return !elm_widget_focus_get(obj);
     }
   items = eina_list_append(items,
            elm_access_object_get(elm_entry_textblock_get((Evas_Object *)obj)));

   clear = elm_object_part_content_get(obj, "elm.swallow.clear");
   if (clear && evas_object_visible_get(clear))
     items = eina_list_append(items, clear);

   return elm_widget_focus_list_next_get(obj, items, eina_list_data_get, dir, next);
}

static void
_elm_entry_smart_access(Evas_Object *obj, Eina_Bool is_access)
{
   ELM_ENTRY_CHECK(obj);

   _access_obj_process(obj, is_access);

   evas_object_smart_callback_call(obj, SIG_ACCESS_CHANGED, NULL);
}
/////////////////////////////////////

/* we can't issue the layout's theming code here, cause it assumes an
 * unique edje object, always */
static Eina_Bool
_elm_entry_smart_theme(Evas_Object *obj)
{
   Evas_Object *ao;
   const char *str;
   const char *t;
   Elm_Widget_Smart_Class *parent_parent =
     (Elm_Widget_Smart_Class *)((Evas_Smart_Class *)
                                _elm_entry_parent_sc)->parent;

   ELM_ENTRY_DATA_GET(obj, sd);

   if (!parent_parent->theme(obj))
     return EINA_FALSE;

   evas_event_freeze(evas_object_evas_get(obj));

   // TIZEN_ONLY(131221) : Customize accessiblity for editfield
   // do not delete access object, application could refer
   ao = elm_access_object_get(elm_entry_textblock_get(obj));
   _elm_access_object_hover_unregister(ao);

   edje_object_mirrored_set
     (ELM_WIDGET_DATA(sd)->resize_obj, elm_widget_mirrored_get(obj));

   edje_object_scale_set
     (ELM_WIDGET_DATA(sd)->resize_obj,
     elm_widget_scale_get(obj) * elm_config_scale_get());

   _mirrored_set(obj, elm_widget_mirrored_get(obj));

   // TIZEN ONLY(130225) : when password mode, elm_object_text_get returns utf8 string.
   elm_entry_imf_context_reset(obj);
   if (sd->password)
     {
        char *tmp = elm_entry_utf8_to_markup(elm_object_text_get(obj));
        t = eina_stringshare_add(tmp);
        if (tmp) free(tmp);
     }
   else
     t = eina_stringshare_add(elm_object_text_get(obj));

   // TIZEN ONLY(131106): Add rectangle for processing mouse up event on paddings inside of entry.
   if (sd->event_rect)
     {
        if (sd->scroll)
          elm_object_part_content_unset(obj, "scroller_event_rect");
        else
          elm_object_part_content_unset(obj, "event_rect");
        evas_object_hide(sd->event_rect);
     }
   //

   elm_widget_theme_object_set
     (obj, sd->entry_edje, "entry", _elm_entry_theme_group_get(obj),
     elm_widget_style_get(obj));

   // TIZEN ONLY(130129) : Currently, for freezing cursor movement only.
   edje_object_part_text_freeze(sd->entry_edje, "elm.text");
   //

   if (_elm_config->desktop_entry)
     edje_object_part_text_select_allow_set
       (sd->entry_edje, "elm.text", EINA_TRUE);

   // TIZEN_ONLY(20130830): For cursor movement when mouse up.
   edje_object_part_text_select_disabled_set(sd->entry_edje, "elm.text",
                                             !elm_entry_select_allow_get(obj));
   ////

   elm_object_text_set(obj, t);
   eina_stringshare_del(t);

   if (elm_widget_disabled_get(obj))
     edje_object_signal_emit(sd->entry_edje, "elm,state,disabled", "elm");

   edje_object_part_text_input_panel_layout_set
     (sd->entry_edje, "elm.text", sd->input_panel_layout);
   edje_object_part_text_input_panel_layout_variation_set
     (sd->entry_edje, "elm.text", sd->input_panel_layout_variation);
   edje_object_part_text_autocapital_type_set
     (sd->entry_edje, "elm.text", sd->autocapital_type);
   edje_object_part_text_prediction_allow_set
     (sd->entry_edje, "elm.text", sd->prediction_allow);
   edje_object_part_text_input_hint_set
     (sd->entry_edje, "elm.text", sd->input_hints);
   edje_object_part_text_input_panel_enabled_set
     (sd->entry_edje, "elm.text", sd->input_panel_enable);
   edje_object_part_text_input_panel_imdata_set
     (sd->entry_edje, "elm.text", sd->input_panel_imdata,
     sd->input_panel_imdata_len);
   edje_object_part_text_input_panel_return_key_type_set
     (sd->entry_edje, "elm.text", sd->input_panel_return_key_type);
   edje_object_part_text_input_panel_return_key_disabled_set
     (sd->entry_edje, "elm.text", sd->input_panel_return_key_disabled);
   edje_object_part_text_input_panel_show_on_demand_set
     (sd->entry_edje, "elm.text", sd->input_panel_show_on_demand);
   edje_object_part_text_cursor_handler_disabled_set
     (sd->entry_edje, "elm.text", sd->cursor_handler_disabled);

   // elm_entry_cursor_pos_set -> cursor,changed -> widget_show_region_set
   // -> smart_objects_calculate will call all smart calculate functions,
   // and one of them can delete elm_entry.
   evas_object_ref(obj);

   if (sd->cursor_pos != 0)
     elm_entry_cursor_pos_set(obj, sd->cursor_pos);

   if (elm_widget_focus_get(obj))
     edje_object_signal_emit(sd->entry_edje, "elm,action,focus", "elm");

   edje_object_message_signal_process(sd->entry_edje);

   Evas_Object* clip = evas_object_clip_get(sd->entry_edje);
   evas_object_clip_set(sd->hit_rect, clip);

   if (sd->scroll)
     {
        sd->s_iface->mirrored_set(obj, elm_widget_mirrored_get(obj));

        elm_widget_theme_object_set
          (obj, sd->scr_edje, "scroller", "entry", elm_widget_style_get(obj));

        str = edje_object_data_get(sd->scr_edje, "focus_highlight");
     }
   else
     {
        str = edje_object_data_get(sd->entry_edje, "focus_highlight");
     }

   // TIZEN ONLY(131106): Add rectangle for processing mouse up event on paddings inside of entry.
   if ((sd->scroll && edje_object_part_exists(sd->scr_edje, "scroller_event_rect")) ||
       (!sd->scroll && edje_object_part_exists(sd->entry_edje, "event_rect")))
     {
        if (!sd->event_rect)
          {
             sd->event_rect = evas_object_rectangle_add(evas_object_evas_get(obj));
             evas_object_color_set(sd->event_rect, 0, 0, 0, 0);
             evas_object_size_hint_align_set(sd->event_rect, EVAS_HINT_FILL, EVAS_HINT_FILL);
             evas_object_event_callback_add(sd->event_rect, EVAS_CALLBACK_MOUSE_UP,
                                            _event_rect_mouse_up_cb, obj);
          }

        if (sd->scroll)
          elm_object_part_content_set(obj, "scroller_event_rect", sd->event_rect);
        else
          elm_object_part_content_set(obj, "event_rect", sd->event_rect);
     }
   //

   if ((str) && (!strcmp(str, "on")))
     elm_widget_highlight_in_theme_set(obj, EINA_TRUE);
   else
     elm_widget_highlight_in_theme_set(obj, EINA_FALSE);

   elm_layout_sizing_eval(obj);

   sd->has_text = !sd->has_text;
   _elm_entry_guide_update(obj, !sd->has_text);
   // TIZEN_ONLY(131221) : Customize accessiblity for editfield
   if (_elm_config->access_mode)
     _elm_access_object_hover_register(ao, elm_entry_textblock_get(obj));
   //
   evas_event_thaw(evas_object_evas_get(obj));
   evas_event_thaw_eval(evas_object_evas_get(obj));

   // TIZEN ONLY
   edje_object_part_text_thaw(sd->entry_edje, "elm.text"); //(130129) : Currently, for freezing cursor movement only.
   if (!_elm_config->desktop_entry)
     {
        if (sd->region_get_job) ecore_job_del(sd->region_get_job);
        sd->region_get_job = ecore_job_add(_region_get_job, obj);
     }
   //

   // TIZEN ONLY(130911)
   _cursor_geometry_update(obj);
   //

   evas_object_smart_callback_call(obj, SIG_THEME_CHANGED, NULL);

   evas_object_unref(obj);

   return EINA_TRUE;
}

static void
_cursor_geometry_recalc(Evas_Object *obj)
{
   ELM_ENTRY_DATA_GET(obj, sd);

   if (sd->cur_changed)
     evas_object_smart_callback_call(obj, SIG_CURSOR_CHANGED, NULL);

   if (!sd->deferred_recalc_job)
     {
        Evas_Coord cx, cy, cw, ch;

        edje_object_part_text_cursor_geometry_get
          (sd->entry_edje, "elm.text", &cx, &cy, &cw, &ch);
// TIZEN ONLY(150617) : P150602-02981 & cursor_pos_set region error
#ifdef ELM_FEATURE_WEARABLE_C1
        sd->cur_changed = EINA_FALSE;
        elm_widget_show_region_set(obj, cx, cy, cw, ch, EINA_FALSE);
#else
        if (sd->cur_changed)
          {
             sd->cur_changed = EINA_FALSE;
             elm_widget_show_region_set(obj, cx, cy, cw, ch, EINA_FALSE);
          }
#endif
// TIZEN ONLY(150617) : P150602-02981 & cursor_pos_set region error
     }
   else
     sd->deferred_cur = EINA_TRUE;
}

static void
_deferred_recalc_job(void *data)
{
   Evas_Coord minh = -1, resw = -1, minw = -1, fw = 0, fh = 0;

   ELM_ENTRY_DATA_GET(data, sd);

   if (!sd) return;

   sd->deferred_recalc_job = NULL;

   evas_object_geometry_get(sd->entry_edje, NULL, NULL, &resw, NULL);
   edje_object_size_min_restricted_calc(sd->entry_edje, &minw, &minh, resw, 0);
   elm_coords_finger_size_adjust(1, &minw, 1, &minh);

   /* This is a hack to workaround the way min size hints are treated.
    * If the minimum width is smaller than the restricted width, it
    * means the minimum doesn't matter. */
   if (minw <= resw)
     {
        Evas_Coord ominw = -1;

        evas_object_size_hint_min_get(data, &ominw, NULL);
        minw = ominw;
     }

   sd->ent_mw = minw;
   sd->ent_mh = minh;

   elm_coords_finger_size_adjust(1, &fw, 1, &fh);
   if (sd->scroll)
     {
        Evas_Coord vmw = 0, vmh = 0;

        edje_object_size_min_calc(sd->scr_edje, &vmw, &vmh);
        if (sd->single_line)
          {
             evas_object_size_hint_min_set(data, vmw, minh + vmh);
             evas_object_size_hint_max_set(data, -1, minh + vmh);
          }
        else
          {
             evas_object_size_hint_min_set(data, vmw, vmh);
             evas_object_size_hint_max_set(data, -1, -1);
          }
     }
   else
     {
        if (sd->single_line)
          {
             evas_object_size_hint_min_set(data, minw, minh);
             evas_object_size_hint_max_set(data, -1, minh);
          }
        else
          {
             evas_object_size_hint_min_set(data, fw, minh);
             evas_object_size_hint_max_set(data, -1, -1);
          }
     }

   if (sd->deferred_cur)
     {
        Evas_Coord cx, cy, cw, ch;

        edje_object_part_text_cursor_geometry_get
          (sd->entry_edje, "elm.text", &cx, &cy, &cw, &ch);
        if (sd->cur_changed)
          {
             sd->cur_changed = EINA_FALSE;
             elm_widget_show_region_set(data, cx, cy, cw, ch, EINA_FALSE);
          }
     }
}

static void
_elm_entry_smart_sizing_eval(Evas_Object *obj)
{
   Evas_Coord minw = -1, minh = -1;
   Evas_Coord resw, resh;

   ELM_ENTRY_DATA_GET(obj, sd);

   evas_object_geometry_get(obj, NULL, NULL, &resw, &resh);

   if (sd->line_wrap)
     {
        if ((resw == sd->last_w) && (!sd->changed))
          {
             if (sd->scroll)
               {
                  Evas_Coord vw = 0, vh = 0, w = 0, h = 0;

                  sd->s_iface->content_viewport_size_get(obj, &vw, &vh);

                  w = sd->ent_mw;
                  h = sd->ent_mh;
                  if (vw > sd->ent_mw) w = vw;
                  if (vh > sd->ent_mh) h = vh;
                  evas_object_resize(sd->entry_edje, w, h);

                  return;
               }

             return;
          }

        evas_event_freeze(evas_object_evas_get(obj));
        sd->changed = EINA_FALSE;
        sd->last_w = resw;
        if (sd->scroll)
          {
             Evas_Coord vw = 0, vh = 0, vmw = 0, vmh = 0, w = -1, h = -1;

             evas_object_resize(sd->scr_edje, resw, resh);
             edje_object_size_min_calc(sd->scr_edje, &vmw, &vmh);
             sd->s_iface->content_viewport_size_get(obj, &vw, &vh);
             edje_object_size_min_restricted_calc
               (sd->entry_edje, &minw, &minh, vw, 0);
             elm_coords_finger_size_adjust(1, &minw, 1, &minh);

             /* This is a hack to workaround the way min size hints
              * are treated.  If the minimum width is smaller than the
              * restricted width, it means the minimum doesn't
              * matter. */
             if (minw <= vw)
               {
                  Evas_Coord ominw = -1;

                  evas_object_size_hint_min_get(sd->entry_edje, &ominw, NULL);
                  minw = ominw;
               }
             sd->ent_mw = minw;
             sd->ent_mh = minh;

             if ((minw > 0) && (vw < minw)) vw = minw;
             if (minh > vh) vh = minh;

             if (sd->single_line) h = vmh + minh;
             else h = vmh;

             evas_object_resize(sd->entry_edje, vw, vh);
             evas_object_size_hint_min_set(obj, w, h);

             if (sd->single_line)
               evas_object_size_hint_max_set(obj, -1, h);
             else
               evas_object_size_hint_max_set(obj, -1, -1);
          }
        else
          {
             if (sd->deferred_recalc_job)
               ecore_job_del(sd->deferred_recalc_job);
             sd->deferred_recalc_job =
               ecore_job_add(_deferred_recalc_job, obj);
          }

        evas_event_thaw(evas_object_evas_get(obj));
        evas_event_thaw_eval(evas_object_evas_get(obj));
     }
   else
     {
        if (!sd->changed) return;
        evas_event_freeze(evas_object_evas_get(obj));
        sd->changed = EINA_FALSE;
        sd->last_w = resw;
        if (sd->scroll)
          {
             Evas_Coord vw = 0, vh = 0, vmw = 0, vmh = 0, w = -1, h = -1;

             edje_object_size_min_calc(sd->entry_edje, &minw, &minh);
             sd->ent_mw = minw;
             if(sd->ent_mh < minh || !(strcmp(elm_entry_entry_get(obj),"")))
                sd->ent_mh = minh;
             elm_coords_finger_size_adjust(1, &minw, 1, &minh);

             sd->s_iface->content_viewport_size_get(obj, &vw, &vh);

             if (minw > vw) vw = minw;
             if (minh > vh) vh = minh;

             evas_object_resize(sd->entry_edje, vw, vh);
             edje_object_size_min_calc(sd->scr_edje, &vmw, &vmh);
             if (sd->single_line) h = vmh + sd->ent_mh;
             else h = vmh;

             evas_object_size_hint_min_set(obj, w, h);
             if (sd->single_line)
               evas_object_size_hint_max_set(obj, -1, h);
             else
               evas_object_size_hint_max_set(obj, -1, -1);
          }
        else
          {
             edje_object_size_min_calc(sd->entry_edje, &minw, &minh);
             sd->ent_mw = minw;
             sd->ent_mh = minh;
             elm_coords_finger_size_adjust(1, &minw, 1, &minh);
             evas_object_size_hint_min_set(obj, minw, minh);

             if (sd->single_line)
               evas_object_size_hint_max_set(obj, -1, minh);
             else
               evas_object_size_hint_max_set(obj, -1, -1);
          }
        evas_event_thaw(evas_object_evas_get(obj));
        evas_event_thaw_eval(evas_object_evas_get(obj));
     }

   _cursor_geometry_recalc(obj);
}

static void
_return_key_enabled_check(Evas_Object *obj)
{
   Eina_Bool return_key_disabled = EINA_FALSE;

   ELM_ENTRY_DATA_GET(obj, sd);

   if (!sd->auto_return_key) return;

   if (elm_entry_is_empty(obj) == EINA_TRUE)
     return_key_disabled = EINA_TRUE;

   elm_entry_input_panel_return_key_disabled_set(obj, return_key_disabled);
}

static Eina_Bool
_elm_entry_smart_on_focus(Evas_Object *obj, Elm_Focus_Info *info)
{
   Evas_Object *top;
   Eina_Bool top_is_win = EINA_FALSE;
   Evas_Object *ao;

   ELM_ENTRY_DATA_GET(obj, sd);

   top = elm_widget_top_get(obj);
   if (!strcmp(evas_object_type_get(top), "elm_win"))
     top_is_win = EINA_TRUE;

   // if (!sd->editable) return EINA_FALSE;   // TIZEN ONLY
   if (elm_widget_focus_get(obj))
     {
        printf("[Elm_entry::Focused] obj : %p\n", obj); // TIZEN ONLY
        evas_object_focus_set(sd->entry_edje, EINA_TRUE);
        edje_object_signal_emit(sd->entry_edje, "elm,action,focus", "elm");
        sd->needs_focus_region = EINA_TRUE; // TIZEN_ONLY(20130830): For cursor movement when mouse up.
        if (sd->editable) // TIZEN ONLY
          {
             if (top && top_is_win && sd->input_panel_enable &&
               !edje_object_part_text_imf_context_get(sd->entry_edje, "elm.text"))
               elm_win_keyboard_mode_set(top, ELM_WIN_KEYBOARD_ON);
             evas_object_smart_callback_call(obj, SIG_FOCUSED, info);
             _return_key_enabled_check(obj);
          }
        // TIZEN_ONLY(20140418): Call _elm_access_highlight_set manually in smart on focus of entry.
        if (_elm_config->access_mode)
          {
             Evas_Object *tb = elm_entry_textblock_get(obj);
             ao = elm_access_object_get(tb);
             _elm_access_highlight_set(ao, EINA_TRUE);
          }
        //
     }
   else
     {
        printf("[Elm_entry::Unfocused] obj : %p\n", obj); // TIZEN ONLY
        _cbhm_msg_send(obj, "cbhm_hide"); // TIZEN ONLY : Hide clipboard
        edje_object_signal_emit(sd->entry_edje, "elm,action,unfocus", "elm");
        evas_object_focus_set(sd->entry_edje, EINA_FALSE);
        // TIZEN ONLY
        if ((sd->api) && (sd->api->obj_hidemenu))
          sd->api->obj_hidemenu(obj);
        //
        if (sd->editable) // TIZEN ONLY
          {
             if (top && top_is_win && sd->input_panel_enable &&
               !edje_object_part_text_imf_context_get(sd->entry_edje, "elm.text"))
               elm_win_keyboard_mode_set(top, ELM_WIN_KEYBOARD_OFF);
             evas_object_smart_callback_call(obj, SIG_UNFOCUSED, NULL);
          }

        if (_elm_config->selection_clear_enable)
          {
             if ((sd->have_selection) && (!sd->hoversel))
               {
                  sd->sel_mode = EINA_FALSE;
                  elm_widget_scroll_hold_pop(obj);
                  edje_object_part_text_select_allow_set(sd->entry_edje, "elm.text", EINA_FALSE);
                  edje_object_signal_emit(sd->entry_edje, "elm,state,select,off", "elm");
                  edje_object_part_text_select_none(sd->entry_edje, "elm.text");
               }
          }
     }

   return EINA_TRUE;
}

static Eina_Bool
_elm_entry_smart_translate(Evas_Object *obj)
{
   evas_object_smart_callback_call(obj, SIG_LANG_CHANGED, NULL);

   return EINA_TRUE;
}

// TIZEN_ONLY(20130830): For cursor movement when mouse up.
//static void
//_entry_cursor_changed_for_focus_region_cb(void *data,
//                                          Evas_Object *obj __UNUSED__,
//                                          const char *emission __UNUSED__,
//                                          const char *source __UNUSED__)
//{
//   ELM_ENTRY_DATA_GET(data, sd);
//   if (elm_widget_focus_get(data))
//     elm_widget_focus_region_show(data);
//   edje_object_signal_callback_del
//      (sd->entry_edje, "cursor,updated", "elm.text",
//       _entry_cursor_changed_for_focus_region_cb);
//}
////

static Eina_Bool
_elm_entry_smart_on_focus_region(const Evas_Object *obj,
                                 Evas_Coord *x,
                                 Evas_Coord *y,
                                 Evas_Coord *w,
                                 Evas_Coord *h)
{
   ELM_ENTRY_DATA_GET(obj, sd);

   // TIZEN_ONLY(20130830): For cursor movement when mouse up.
   //if (sd->needs_focus_region)
   //  {
   //     edje_object_signal_callback_add
   //        (sd->entry_edje, "cursor,updated", "elm.text",
   //         _entry_cursor_changed_for_focus_region_cb, (void *)obj);
   //     sd->needs_focus_region = EINA_FALSE;
   //     return EINA_FALSE;
   //  }
   //else
     {
        edje_object_part_text_cursor_geometry_get
           (sd->entry_edje, "elm.text", x, y, w, h);
        return EINA_TRUE;
     }
   //edje_object_part_text_cursor_geometry_get
   //   (sd->entry_edje, "elm.text", x, y, w, h);
   //return EINA_TRUE;
   //////////////////////////////////////////////////////
}

static void
_show_region_hook(void *data,
                  Evas_Object *obj)
{
   Evas_Coord x, y, w, h;

   ELM_ENTRY_DATA_GET(data, sd);

   elm_widget_show_region_get(obj, &x, &y, &w, &h);

   sd->s_iface->content_region_show(obj, x, y, w, h);
}

static Eina_Bool
_elm_entry_smart_sub_object_del(Evas_Object *obj,
                                Evas_Object *sobj)
{
   /* unfortunately entry doesn't follow the signal pattern
    * elm,state,icon,{visible,hidden}, so we have to replicate this
    * smart function */
   if (sobj == elm_layout_content_get(obj, "elm.swallow.icon"))
     {
        elm_layout_signal_emit(obj, "elm,action,hide,icon", "elm");
     }
   else if (sobj == elm_layout_content_get(obj, "elm.swallow.end"))
     {
        elm_layout_signal_emit(obj, "elm,action,hide,end", "elm");
     }

   if (!ELM_WIDGET_CLASS(_elm_entry_parent_sc)->sub_object_del(obj, sobj))
     return EINA_FALSE;

   return EINA_TRUE;
}

static void
_hoversel_position(Evas_Object *obj)
{
   Evas_Coord cx, cy, cw, ch, x, y, mw, mh;

   ELM_ENTRY_DATA_GET(obj, sd);

   cx = cy = 0;
   cw = ch = 1;
   evas_object_geometry_get(sd->entry_edje, &x, &y, NULL, NULL);
   if (sd->use_down)
     {
        cx = sd->downx - x;
        cy = sd->downy - y;
        cw = 1;
        ch = 1;
     }
   else
     edje_object_part_text_cursor_geometry_get
       (sd->entry_edje, "elm.text", &cx, &cy, &cw, &ch);

   evas_object_size_hint_min_get(sd->hoversel, &mw, &mh);
   if (cw < mw)
     {
        cx += (cw - mw) / 2;
        cw = mw;
     }
   if (ch < mh)
     {
        cy += (ch - mh) / 2;
        ch = mh;
     }
   evas_object_move(sd->hoversel, x + cx, y + cy);
   evas_object_resize(sd->hoversel, cw, ch);
}

static void
_hover_del_job(void *data)
{
   ELM_ENTRY_DATA_GET(data, sd);

   if (sd->hoversel)
     {
        evas_object_del(sd->hoversel);
        sd->hoversel = NULL;
     }
   sd->hov_deljob = NULL;
}

static void
_hover_dismissed_cb(void *data,
                    Evas_Object *obj __UNUSED__,
                    void *event_info __UNUSED__)
{
   ELM_ENTRY_DATA_GET(data, sd);

   sd->use_down = 0;
   if (sd->hoversel) evas_object_hide(sd->hoversel);
   if (sd->sel_mode)
     {
        if (!_elm_config->desktop_entry)
          {
             if (!sd->password)
               edje_object_part_text_select_allow_set
                 (sd->entry_edje, "elm.text", EINA_TRUE);
          }
     }
   elm_widget_scroll_freeze_pop(data);
   if (sd->hov_deljob) ecore_job_del(sd->hov_deljob);
   sd->hov_deljob = ecore_job_add(_hover_del_job, data);
}

static void
_hover_selected_cb(void *data,
                   Evas_Object *obj __UNUSED__,
                   void *event_info __UNUSED__)
{
   ELM_ENTRY_DATA_GET(data, sd);

   sd->sel_mode = EINA_TRUE;
   edje_object_part_text_select_none(sd->entry_edje, "elm.text");

   if (!_elm_config->desktop_entry)
     {
        if (!sd->password)
          edje_object_part_text_select_allow_set
            (sd->entry_edje, "elm.text", EINA_TRUE);
     }
   edje_object_signal_emit(sd->entry_edje, "elm,state,select,on", "elm");

   if (!_elm_config->desktop_entry)
     {
        elm_widget_scroll_hold_push(data);
        sd->scroll_holding = EINA_TRUE;
     }
}

static char *
_item_tags_remove(const char *str)
{
   char *ret;
   Eina_Strbuf *buf;

   if (!str)
     return NULL;

   buf = eina_strbuf_new();
   if (!buf)
     return NULL;

   if (!eina_strbuf_append(buf, str))
     {
        eina_strbuf_free(buf);
        return NULL;
     }

   while (EINA_TRUE)
     {
        const char *temp = eina_strbuf_string_get(buf);
        char *start_tag = NULL;
        char *end_tag = NULL;
        size_t sindex;
        size_t eindex;

        start_tag = strstr(temp, "<item");
        if (!start_tag)
          start_tag = strstr(temp, "</item");
        if (start_tag)
          end_tag = strstr(start_tag, ">");
        else
          break;
        if (!end_tag || start_tag > end_tag)
          break;

        sindex = start_tag - temp;
        eindex = end_tag - temp + 1;
        if (!eina_strbuf_remove(buf, sindex, eindex))
          break;
     }

   ret = eina_strbuf_string_steal(buf);
   eina_strbuf_free(buf);

   return ret;
}

void
_elm_entry_entry_paste(Evas_Object *obj,
                       const char *entry)
{
   char *str = NULL;

   ELM_ENTRY_CHECK(obj);
   ELM_ENTRY_DATA_GET(obj, sd);

   if (sd->cnp_mode == ELM_CNP_MODE_NO_IMAGE)
     {
        str = _item_tags_remove(entry);
        if (!str) str = strdup(entry);
     }
   else
     str = strdup(entry);
   if (!str) str = (char *)entry;

   edje_object_part_text_user_insert(sd->entry_edje, "elm.text", str);

   if (str != entry) free(str);

   // TIZEN ONLY
#ifdef HAVE_ELEMENTARY_X
   if (elm_widget_focus_get(obj))
     ecore_x_selection_secondary_set(elm_win_xwindow_get(obj), NULL,0);
#endif
   //
}

static void
_paste_cb(void *data,
          Evas_Object *obj __UNUSED__,
          void *event_info __UNUSED__)
{
   ELM_ENTRY_DATA_GET(data, sd);

   evas_object_smart_callback_call(data, SIG_SELECTION_PASTE, NULL);
   if (sd->sel_notify_handler)
     {
#ifdef HAVE_ELEMENTARY_X
        Elm_Sel_Format formats = ELM_SEL_FORMAT_MARKUP;

        sd->selection_asked = EINA_TRUE;

        if (sd->cnp_mode == ELM_CNP_MODE_PLAINTEXT)
          formats = ELM_SEL_FORMAT_TEXT;
        else if (sd->cnp_mode != ELM_CNP_MODE_NO_IMAGE)
          formats |= ELM_SEL_FORMAT_IMAGE;

        elm_cnp_selection_get
          (data, ELM_SEL_TYPE_CLIPBOARD, formats, NULL, NULL);
#endif
     }
   else
     {
#ifdef HAVE_ELEMENTARY_WAYLAND
        Elm_Sel_Format formats = ELM_SEL_FORMAT_MARKUP;
        sd->selection_asked = EINA_TRUE;
        if (sd->cnp_mode == ELM_CNP_MODE_PLAINTEXT)
           formats = ELM_SEL_FORMAT_TEXT;
        else if (sd->cnp_mode != ELM_CNP_MODE_NO_IMAGE)
           formats |= ELM_SEL_FORMAT_IMAGE;
        elm_cnp_selection_get(data, ELM_SEL_TYPE_CLIPBOARD, formats, NULL, NULL);
#endif
     }
}

static void
_selection_store(Elm_Sel_Type seltype,
                 Evas_Object *obj)
{
   const char *sel;

   ELM_ENTRY_DATA_GET(obj, sd);

   sel = edje_object_part_text_selection_get(sd->entry_edje, "elm.text");
   if ((!sel) || (!sel[0])) return;  /* avoid deleting our own selection */

   elm_cnp_selection_set
     (obj, seltype, ELM_SEL_FORMAT_MARKUP, sel, strlen(sel));
   if (seltype == ELM_SEL_TYPE_CLIPBOARD)
     eina_stringshare_replace(&sd->cut_sel, sel);
}

static void
_cut_cb(void *data,
        Evas_Object *obj __UNUSED__,
        void *event_info __UNUSED__)
{
   ELM_ENTRY_DATA_GET(data, sd);

   evas_object_smart_callback_call(data, SIG_SELECTION_CUT, NULL);
   /* Store it */
   sd->sel_mode = EINA_FALSE;
   if (!_elm_config->desktop_entry)
     edje_object_part_text_select_allow_set
       (sd->entry_edje, "elm.text", EINA_FALSE);
   edje_object_signal_emit(sd->entry_edje, "elm,state,select,off", "elm");

   if ((!_elm_config->desktop_entry) && (sd->scroll_holding))
     {
        elm_widget_scroll_hold_pop(data);
        sd->scroll_holding = EINA_FALSE;
     }

   _selection_store(ELM_SEL_TYPE_CLIPBOARD, data);
   edje_object_part_text_user_insert(sd->entry_edje, "elm.text", "");
   elm_layout_sizing_eval(data);
}

static void
_copy_cb(void *data,
         Evas_Object *obj __UNUSED__,
         void *event_info __UNUSED__)
{
   ELM_ENTRY_DATA_GET(data, sd);

   evas_object_smart_callback_call(data, SIG_SELECTION_COPY, NULL);

   sd->sel_mode = EINA_FALSE;
   if (!_elm_config->desktop_entry)
     {
        edje_object_part_text_select_allow_set
          (sd->entry_edje, "elm.text", EINA_FALSE);
        edje_object_signal_emit(sd->entry_edje, "elm,state,select,off", "elm");
        if (sd->scroll_holding)
          {
             elm_widget_scroll_hold_pop(data);
             sd->scroll_holding = EINA_FALSE;
          }
     }
   _selection_store(ELM_SEL_TYPE_CLIPBOARD, data);
}

static void
_hover_cancel_cb(void *data,
                 Evas_Object *obj __UNUSED__,
                 void *event_info __UNUSED__)
{
   ELM_ENTRY_DATA_GET(data, sd);

   sd->sel_mode = EINA_FALSE;
   if (!_elm_config->desktop_entry)
     edje_object_part_text_select_allow_set
       (sd->entry_edje, "elm.text", EINA_FALSE);
   edje_object_signal_emit(sd->entry_edje, "elm,state,select,off", "elm");
   if ((!_elm_config->desktop_entry) && (sd->scroll_holding))
     {
        elm_widget_scroll_hold_pop(data);
        sd->scroll_holding = EINA_FALSE;
     }
   edje_object_part_text_select_none(sd->entry_edje, "elm.text");
}

static void
_hover_item_clicked_cb(void *data,
                       Evas_Object *obj __UNUSED__,
                       void *event_info __UNUSED__)
{
   Elm_Entry_Context_Menu_Item *it = data;
   Evas_Object *obj2 = it->obj;

   if (it->func) it->func(it->data, obj2, NULL);
}

#ifndef ELM_FEATURE_WEARABLE
static void
_menu_call(Evas_Object *obj)
{
   Evas_Object *top;
   const Eina_List *l;
   const Elm_Entry_Context_Menu_Item *it;

   ELM_ENTRY_DATA_GET(obj, sd);

   if ((sd->api) && (sd->api->obj_longpress))
     {
        sd->api->obj_longpress(obj);
     }
   else if (sd->context_menu)
     {
        const char *context_menu_orientation;

        if (sd->hoversel) evas_object_del(sd->hoversel);
        else elm_widget_scroll_freeze_push(obj);

        sd->hoversel = elm_hoversel_add(obj);
        context_menu_orientation = edje_object_data_get
            (sd->entry_edje, "context_menu_orientation");

        if ((context_menu_orientation) &&
            (!strcmp(context_menu_orientation, "horizontal")))
          elm_hoversel_horizontal_set(sd->hoversel, EINA_TRUE);

        elm_object_style_set(sd->hoversel, "entry");
        elm_widget_sub_object_add(obj, sd->hoversel);
        elm_object_text_set(sd->hoversel, "Text");
        top = elm_widget_top_get(obj);

        if (top) elm_hoversel_hover_parent_set(sd->hoversel, top);

        evas_object_smart_callback_add
          (sd->hoversel, "dismissed", _hover_dismissed_cb, obj);
        if (sd->have_selection)
          {
             if (!sd->password)
               {
                  elm_hoversel_item_add
                     (sd->hoversel, E_("IDS_COM_BODY_COPY"), NULL, ELM_ICON_NONE,
                      _copy_cb, obj);
                  if (sd->editable)
                    elm_hoversel_item_add
                       (sd->hoversel, E_("IDS_COM_BODY_CUT"), NULL, ELM_ICON_NONE,
                        _cut_cb, obj);
                  elm_hoversel_item_add
                    (sd->hoversel, E_("IDS_COM_SK_CANCEL"), NULL, ELM_ICON_NONE,
                    _hover_cancel_cb, obj);
               }
          }
        else
          {
             if (!sd->sel_mode)
               {
                  if (!_elm_config->desktop_entry)
                    {
                       if (!sd->password)
                         elm_hoversel_item_add
                           (sd->hoversel, E_("IDS_COM_SK_SELECT"), NULL, ELM_ICON_NONE,
                           _hover_selected_cb, obj);
                    }
                  if (elm_selection_selection_has_owner(obj))
                    {
                       if (sd->editable)
                         elm_hoversel_item_add
                           (sd->hoversel, E_("IDS_COM_BODY_PASTE"), NULL, ELM_ICON_NONE,
                           _paste_cb, obj);
                    }
               }
             else
               elm_hoversel_item_add
                  (sd->hoversel, E_("IDS_COM_SK_CANCEL"), NULL, ELM_ICON_NONE,
                   _hover_cancel_cb, obj);
          }

        EINA_LIST_FOREACH(sd->items, l, it)
          {
             elm_hoversel_item_add(sd->hoversel, it->label, it->icon_file,
                                   it->icon_type, _hover_item_clicked_cb, it);
          }

        if (sd->hoversel)
          {
             _hoversel_position(obj);
             evas_object_show(sd->hoversel);
             elm_hoversel_hover_begin(sd->hoversel);
          }

        if (!_elm_config->desktop_entry)
          {
             edje_object_part_text_select_allow_set
               (sd->entry_edje, "elm.text", EINA_FALSE);
             edje_object_part_text_select_abort(sd->entry_edje, "elm.text");
          }
     }
}
#endif

static Eina_Bool
_long_press_cb(void *data)
{
   ELM_ENTRY_DATA_GET(data, sd);

   sd->longpress_timer = NULL;
   evas_object_smart_callback_call(data, SIG_LONGPRESSED, NULL);

   return ECORE_CALLBACK_CANCEL;
}

static void
_mouse_down_cb(void *data,
               Evas *evas __UNUSED__,
               Evas_Object *obj __UNUSED__,
               void *event_info)
{
   Evas_Event_Mouse_Down *ev = event_info;

   ELM_ENTRY_DATA_GET(data, sd);

   if (sd->disabled) return;
   if (ev->event_flags & EVAS_EVENT_FLAG_ON_HOLD) return;
   sd->downx = ev->canvas.x;
   sd->downy = ev->canvas.y;
   sd->long_pressed = EINA_FALSE;
   if (ev->button == 1)
     {
        if (sd->longpress_timer) ecore_timer_del(sd->longpress_timer);
        sd->longpress_timer = ecore_timer_add
            (_elm_config->longpress_timeout, _long_press_cb, data);
     }
#ifndef ELM_FEATURE_WEARABLE
   else if (ev->button == 3)
     {
        if (_elm_config->desktop_entry)
          _menu_call(data);
     }
#endif
   if (!sd->editable)
     {
         edje_object_part_text_cursor_handler_disabled_set(sd->entry_edje, "elm.text", EINA_TRUE);
     }
   else if (!sd->cursor_handler_disabled)
     {
         edje_object_part_text_cursor_handler_disabled_set(sd->entry_edje, "elm.text", EINA_FALSE);
     }
}

static void
_mouse_up_cb(void *data,
             Evas *evas __UNUSED__,
             Evas_Object *obj __UNUSED__,
             void *event_info)
{
   Evas_Event_Mouse_Up *ev = event_info;

   ELM_ENTRY_DATA_GET(data, sd);

   if (sd->disabled) return;
   if (ev->button == 1)
     {
        if ((sd->editable) && (sd->drag_started))
          {
             edje_object_part_text_cursor_handler_disabled_set
                (sd->entry_edje, "elm.text", sd->cursor_handler_disabled);
          }
        // TIZEN ONLY
        if (!sd->double_clicked)
          {
             if ((sd->api) && (sd->api->obj_mouseup))
               sd->api->obj_mouseup(data);
          }
        if (sd->magnifier_enabled)
          {
             _magnifier_hide(data);
          }
#ifndef ELM_FEATURE_WEARABLE
        if ((sd->long_pressed) && (!sd->drag_started))
           _menu_call(data);
#endif
        sd->mouse_upped = EINA_TRUE;
        /////
        if (sd->longpress_timer)
          {
             ecore_timer_del(sd->longpress_timer);
             sd->longpress_timer = NULL;
          }
     }
   else if ((ev->button == 3) && (!_elm_config->desktop_entry))
     {
        sd->use_down = 1;
#ifndef ELM_FEATURE_WEARABLE
        _menu_call(data);
#endif
     }
}

static void
_mouse_move_cb(void *data,
               Evas *evas __UNUSED__,
               Evas_Object *obj __UNUSED__,
               void *event_info)
{
   Evas_Event_Mouse_Move *ev = event_info;

   ELM_ENTRY_DATA_GET(data, sd);

   if (sd->disabled) return;

   // TIZEN ONLY
   if (ev->buttons == 1)
     {
        if (sd->long_pressed)
          {
             if ((!sd->drag_started) && (sd->magnifier_enabled))
               {
                  if (sd->has_text)
                    {
                       _magnifier_show(data);
                       _magnifier_move(data);
                    }
               }
          }
     }
   //////////

   if (!sd->sel_mode)
     {
        if (ev->event_flags & EVAS_EVENT_FLAG_ON_HOLD)
          {
             if (sd->longpress_timer)
               {
                  ecore_timer_del(sd->longpress_timer);
                  sd->longpress_timer = NULL;
               }
          }
        else if (sd->longpress_timer)
          {
             Evas_Coord dx, dy;

             dx = sd->downx - ev->cur.canvas.x;
             dx *= dx;
             dy = sd->downy - ev->cur.canvas.y;
             dy *= dy;
             if ((dx + dy) >
                 ((_elm_config->finger_size / 2) *
                  (_elm_config->finger_size / 2)))
               {
                  ecore_timer_del(sd->longpress_timer);
                  sd->longpress_timer = NULL;
               }
          }
     }
   else if (sd->longpress_timer)
     {
        Evas_Coord dx, dy;

        dx = sd->downx - ev->cur.canvas.x;
        dx *= dx;
        dy = sd->downy - ev->cur.canvas.y;
        dy *= dy;
        if ((dx + dy) >
            ((_elm_config->finger_size / 2) *
             (_elm_config->finger_size / 2)))
          {
             ecore_timer_del(sd->longpress_timer);
             sd->longpress_timer = NULL;
          }
     }
}

static void
_entry_changed_handle(void *data,
                      const char *event)
{
   Evas_Coord minh;
   const char *text;

   ELM_ENTRY_DATA_GET(data, sd);

   evas_event_freeze(evas_object_evas_get(data));
   sd->changed = EINA_TRUE;
   /* Reset the size hints which are no more relevant. Keep the
    * height, this is a hack, but doesn't really matter cause we'll
    * re-eval in a moment. */
   evas_object_size_hint_min_get(data, NULL, &minh);
   evas_object_size_hint_min_set(data, -1, minh);

   elm_layout_sizing_eval(data);
   if (sd->text) eina_stringshare_del(sd->text);
   sd->text = NULL;

   // TIZEN ONLY
   if (sd->password_text)
     eina_stringshare_del(sd->password_text);
   sd->password_text = NULL;

   if ((sd->api) && (sd->api->obj_hidemenu))
     sd->api->obj_hidemenu(data);
   /////

   if (sd->delay_write)
     {
        ecore_timer_del(sd->delay_write);
        sd->delay_write = NULL;
     }
   evas_event_thaw(evas_object_evas_get(data));
   evas_event_thaw_eval(evas_object_evas_get(data));
   if ((sd->auto_save) && (sd->file))
     sd->delay_write = ecore_timer_add(2.0, _delay_write, data);

   _return_key_enabled_check(data);
   text = edje_object_part_text_get(sd->entry_edje, "elm.text");
   if (text)
     {
        if (text[0])
          _elm_entry_guide_update(data, EINA_TRUE);
        else
          _elm_entry_guide_update(data, EINA_FALSE);
     }
   /* callback - this could call callbacks that delete the
    * entry... thus... any access to sd after this could be
    * invalid */

   // TIZEN ONLY(130911)
   _cursor_geometry_update(data);
   //

   evas_object_smart_callback_call(data, event, NULL);
}

static void
_entry_changed_signal_cb(void *data,
                         Evas_Object *obj __UNUSED__,
                         const char *emission __UNUSED__,
                         const char *source __UNUSED__)
{
   _entry_changed_handle(data, SIG_CHANGED);
}

static void
_entry_changed_user_signal_cb(void *data,
                              Evas_Object *obj __UNUSED__,
                              const char *emission __UNUSED__,
                              const char *source __UNUSED__)
{
   Elm_Entry_Change_Info info;
   Edje_Entry_Change_Info *edje_info = (Edje_Entry_Change_Info *)
     edje_object_signal_callback_extra_data_get();

   if (edje_info)
     {
        memcpy(&info, edje_info, sizeof(info));
        evas_object_smart_callback_call(data, SIG_CHANGED_USER, &info);
     }
   else
     {
        evas_object_smart_callback_call(data, SIG_CHANGED_USER, NULL);
     }
}

static void
_entry_preedit_changed_signal_cb(void *data,
                                 Evas_Object *obj __UNUSED__,
                                 const char *emission __UNUSED__,
                                 const char *source __UNUSED__)
{
   _entry_changed_handle(data, SIG_PREEDIT_CHANGED);
}

static void
_entry_undo_request_signal_cb(void *data,
                              Evas_Object *obj __UNUSED__,
                              const char *emission __UNUSED__,
                              const char *source __UNUSED__)
{
   evas_object_smart_callback_call(data, SIG_UNDO_REQUEST, NULL);
}

static void
_entry_redo_request_signal_cb(void *data,
                              Evas_Object *obj __UNUSED__,
                              const char *emission __UNUSED__,
                              const char *source __UNUSED__)
{
   evas_object_smart_callback_call(data, SIG_REDO_REQUEST, NULL);
}

static void
_entry_selection_start_signal_cb(void *data,
                                 Evas_Object *obj __UNUSED__,
                                 const char *emission __UNUSED__,
                                 const char *source __UNUSED__)
{
   const Eina_List *l;
   Evas_Object *entry;

   ELM_ENTRY_DATA_GET(data, sd);

   EINA_LIST_FOREACH(entries, l, entry)
     {
        if (entry != data) elm_entry_select_none(entry);
     }

   // TIZEN ONLY
   sd->sel_mode = EINA_TRUE;
   ///

   evas_object_smart_callback_call(data, SIG_SELECTION_START, NULL);
#ifdef HAVE_ELEMENTARY_X
   if (sd->sel_notify_handler)
     {
        const char *txt = elm_entry_selection_get(data);
        Evas_Object *top;

        top = elm_widget_top_get(data);
        if (txt && top && (elm_win_xwindow_get(top)))
          elm_cnp_selection_set(data, ELM_SEL_TYPE_PRIMARY,
                                ELM_SEL_FORMAT_MARKUP, txt, strlen(txt));
     }
#endif
}

static void
_entry_selection_all_signal_cb(void *data,
                               Evas_Object *obj __UNUSED__,
                               const char *emission __UNUSED__,
                               const char *source __UNUSED__)
{
   elm_entry_select_all(data);
}

static void
_entry_selection_none_signal_cb(void *data,
                                Evas_Object *obj __UNUSED__,
                                const char *emission __UNUSED__,
                                const char *source __UNUSED__)
{
   ELM_ENTRY_DATA_GET(data, sd); // TIZEN ONLY

   elm_entry_select_none(data);

   // TIZEN ONLY
   if ((sd->api) && (sd->api->obj_hidemenu))
     sd->api->obj_hidemenu(data);
   /////

}

static void
_entry_selection_changed_signal_cb(void *data,
                                   Evas_Object *obj __UNUSED__,
                                   const char *emission __UNUSED__,
                                   const char *source __UNUSED__)
{
   ELM_ENTRY_DATA_GET(data, sd);
   Evas_Coord cx, cy, cw, ch; // TIZEN ONLY

   sd->have_selection = EINA_TRUE;
   sd->sel_mode = EINA_TRUE; // TIZEN ONLY
   evas_object_smart_callback_call(data, SIG_SELECTION_CHANGED, NULL);
   _selection_store(ELM_SEL_TYPE_PRIMARY, data);

   // TIZEN ONLY
   edje_object_part_text_cursor_geometry_get(sd->entry_edje, "elm.text", &cx, &cy, &cw, &ch);
   if (!sd->deferred_recalc_job)
     elm_widget_show_region_set(data, cx, cy, cw, ch, EINA_FALSE);
   else
     {
        sd->deferred_cur = EINA_TRUE;
        sd->cx = cx;
        sd->cy = cy;
        sd->cw = cw;
        sd->ch = ch;
     }
   ///////
}

static void
_entry_selection_cleared_signal_cb(void *data,
                                   Evas_Object *obj __UNUSED__,
                                   const char *emission __UNUSED__,
                                   const char *source __UNUSED__)
{
   ELM_ENTRY_DATA_GET(data, sd);

   if (!sd->have_selection) return;

   sd->have_selection = EINA_FALSE;

   // TIZEN ONLY
   sd->sel_mode = EINA_FALSE;
   ///

   // TIZEN_ONLY(20130830): For cursor movement when mouse up.
   edje_object_part_text_select_allow_set(sd->entry_edje, "elm.text", EINA_FALSE);
   //
   evas_object_smart_callback_call(data, SIG_SELECTION_CLEARED, NULL);
   if (sd->sel_notify_handler)
     {
        if (sd->cut_sel)
          {
#ifdef HAVE_ELEMENTARY_X
             Evas_Object *top;

             top = elm_widget_top_get(data);
             if ((top) && (elm_win_xwindow_get(top)))
               elm_cnp_selection_set
                 (data, ELM_SEL_TYPE_PRIMARY, ELM_SEL_FORMAT_MARKUP,
                 sd->cut_sel, strlen(sd->cut_sel));
#endif
             eina_stringshare_del(sd->cut_sel);
             sd->cut_sel = NULL;
          }
        else if (!sd->drag_started)
          {
#ifdef HAVE_ELEMENTARY_X
             Evas_Object *top;

             top = elm_widget_top_get(data);
             if ((top) && (elm_win_xwindow_get(top)))
               elm_object_cnp_selection_clear(data, ELM_SEL_TYPE_PRIMARY);
#endif
          }
     }

   // TIZEN ONLY
   if ((sd->api) && (sd->api->obj_hidemenu))
     {
        sd->api->obj_hidemenu(data);
     }
   /////
}

static void
_entry_paste_request_signal_cb(void *data,
                               Evas_Object *obj __UNUSED__,
                               const char *emission,
                               const char *source __UNUSED__)
{
   ELM_ENTRY_DATA_GET(data, sd);

#ifdef HAVE_ELEMENTARY_X
   Elm_Sel_Type type = (emission[sizeof("ntry,paste,request,")] == '1') ?
     ELM_SEL_TYPE_PRIMARY : ELM_SEL_TYPE_CLIPBOARD;
#endif

   if (!sd->editable) return;
   evas_object_smart_callback_call(data, SIG_SELECTION_PASTE, NULL);
   if (sd->sel_notify_handler)
     {
#ifdef HAVE_ELEMENTARY_X
        Evas_Object *top;

        top = elm_widget_top_get(data);
        if ((top) && (elm_win_xwindow_get(top)))
          {
             Elm_Sel_Format formats = ELM_SEL_FORMAT_MARKUP;

             sd->selection_asked = EINA_TRUE;

             if (sd->cnp_mode == ELM_CNP_MODE_PLAINTEXT)
               formats = ELM_SEL_FORMAT_TEXT;
             else if (sd->cnp_mode != ELM_CNP_MODE_NO_IMAGE)
               formats |= ELM_SEL_FORMAT_IMAGE;

             elm_cnp_selection_get(data, type, formats, NULL, NULL);
          }
#endif
     }
}

static void
_entry_copy_notify_signal_cb(void *data,
                             Evas_Object *obj __UNUSED__,
                             const char *emission __UNUSED__,
                             const char *source __UNUSED__)
{
   _copy_cb(data, NULL, NULL);
}

static void
_entry_cut_notify_signal_cb(void *data,
                            Evas_Object *obj __UNUSED__,
                            const char *emission __UNUSED__,
                            const char *source __UNUSED__)
{
   _cut_cb(data, NULL, NULL);
}

static void
_entry_cursor_changed_signal_cb(void *data,
                                Evas_Object *obj __UNUSED__,
                                const char *emission __UNUSED__,
                                const char *source __UNUSED__)
{
   ELM_ENTRY_DATA_GET(data, sd);
   int new_pos = edje_object_part_text_cursor_pos_get
      (sd->entry_edje, "elm.text", EDJE_CURSOR_MAIN);
   if (sd->cursor_pos != new_pos)
     {
        sd->cursor_pos = new_pos;
        sd->cur_changed = EINA_TRUE;
     }
   _cursor_geometry_recalc(data);
   // TIZEN ONY - START
   if (elm_widget_focus_get(data))
     edje_object_signal_emit(sd->entry_edje, "elm,action,show,cursor", "elm");
   // TIZEN ONLY - END
}

static void
_entry_cursor_changed_manual_signal_cb(void *data,
                                       Evas_Object *obj __UNUSED__,
                                       const char *emission __UNUSED__,
                                       const char *source __UNUSED__)
{
   ELM_ENTRY_DATA_GET(data, sd);
   evas_object_smart_callback_call(data, SIG_CURSOR_CHANGED_MANUAL, NULL);
   // TIZEN ONY - START
   if (elm_widget_focus_get(data))
     edje_object_signal_emit(sd->entry_edje, "elm,action,show,cursor", "elm");
   // TIZEN ONLY - END
}

static void
_signal_anchor_geoms_do_things_with_lol(Elm_Entry_Smart_Data *sd,
                                        Elm_Entry_Anchor_Info *ei)
{
   Evas_Textblock_Rectangle *r;
   const Eina_List *geoms, *l;
   Evas_Coord px, py, x, y;

   geoms = edje_object_part_text_anchor_geometry_get
       (sd->entry_edje, "elm.text", ei->name);

   if (!geoms) return;

   evas_object_geometry_get(sd->entry_edje, &x, &y, NULL, NULL);
   evas_pointer_canvas_xy_get
     (evas_object_evas_get(sd->entry_edje), &px, &py);

   EINA_LIST_FOREACH(geoms, l, r)
     {
        if (((r->x + x) <= px) && ((r->y + y) <= py) &&
            ((r->x + x + r->w) > px) && ((r->y + y + r->h) > py))
          {
             ei->x = r->x + x;
             ei->y = r->y + y;
             ei->w = r->w;
             ei->h = r->h;
             break;
          }
     }
}

static void
_entry_anchor_down_signal_cb(void *data,
                             Evas_Object *obj __UNUSED__,
                             const char *emission __UNUSED__,
                             const char *source __UNUSED__)
{
   Elm_Entry_Anchor_Info ei;
   const char *p;
   char *p2;

   ELM_ENTRY_DATA_GET(data, sd);

   p = emission + sizeof("nchor,mouse,down,");
   ei.button = strtol(p, &p2, 10);
   ei.name = p2 + 1;
   ei.x = ei.y = ei.w = ei.h = 0;

   _signal_anchor_geoms_do_things_with_lol(sd, &ei);

   if (!sd->disabled)
     evas_object_smart_callback_call(data, SIG_ANCHOR_DOWN, &ei);
}

static void
_entry_anchor_up_signal_cb(void *data,
                           Evas_Object *obj __UNUSED__,
                           const char *emission __UNUSED__,
                           const char *source __UNUSED__)
{
   Elm_Entry_Anchor_Info ei;
   const char *p;
   char *p2;

   ELM_ENTRY_DATA_GET(data, sd);

   p = emission + sizeof("nchor,mouse,up,");
   ei.button = strtol(p, &p2, 10);
   ei.name = p2 + 1;
   ei.x = ei.y = ei.w = ei.h = 0;

   _signal_anchor_geoms_do_things_with_lol(sd, &ei);

   if (!sd->disabled)
     evas_object_smart_callback_call(data, SIG_ANCHOR_UP, &ei);
}

static void
_anchor_hover_del_cb(void *data,
                     Evas *e __UNUSED__,
                     Evas_Object *obj __UNUSED__,
                     void *event_info __UNUSED__)
{
   ELM_ENTRY_DATA_GET(data, sd);

   if (sd->anchor_hover.pop) evas_object_del(sd->anchor_hover.pop);
   sd->anchor_hover.pop = NULL;
   evas_object_event_callback_del_full
     (sd->anchor_hover.hover, EVAS_CALLBACK_DEL, _anchor_hover_del_cb, obj);
}

static void
_anchor_hover_clicked_cb(void *data,
                         Evas_Object *obj __UNUSED__,
                         void *event_info __UNUSED__)
{
   elm_entry_anchor_hover_end(data);
}

static void
_entry_hover_anchor_clicked_do(Evas_Object *obj,
                               Elm_Entry_Anchor_Info *info)
{
   Evas_Object *hover_parent;
   Evas_Coord x, w, y, h, px, py;
   Elm_Entry_Anchor_Hover_Info ei;

   ELM_ENTRY_DATA_GET(obj, sd);

   ei.anchor_info = info;

   sd->anchor_hover.pop = elm_icon_add(obj);
   evas_object_move(sd->anchor_hover.pop, info->x, info->y);
   evas_object_resize(sd->anchor_hover.pop, info->w, info->h);

   sd->anchor_hover.hover = elm_hover_add(obj);
   evas_object_event_callback_add
     (sd->anchor_hover.hover, EVAS_CALLBACK_DEL, _anchor_hover_del_cb, obj);
   elm_widget_mirrored_set
     (sd->anchor_hover.hover, elm_widget_mirrored_get(obj));
   if (sd->anchor_hover.hover_style)
     elm_object_style_set
       (sd->anchor_hover.hover, sd->anchor_hover.hover_style);

   hover_parent = sd->anchor_hover.hover_parent;
   if (!hover_parent) hover_parent = obj;
   elm_hover_parent_set(sd->anchor_hover.hover, hover_parent);
   elm_hover_target_set(sd->anchor_hover.hover, sd->anchor_hover.pop);
   ei.hover = sd->anchor_hover.hover;

   evas_object_geometry_get(hover_parent, &x, &y, &w, &h);
   ei.hover_parent.x = x;
   ei.hover_parent.y = y;
   ei.hover_parent.w = w;
   ei.hover_parent.h = h;
   px = info->x + (info->w / 2);
   py = info->y + (info->h / 2);
   ei.hover_left = 1;
   if (px < (x + (w / 3))) ei.hover_left = 0;
   ei.hover_right = 1;
   if (px > (x + ((w * 2) / 3))) ei.hover_right = 0;
   ei.hover_top = 1;
   if (py < (y + (h / 3))) ei.hover_top = 0;
   ei.hover_bottom = 1;
   if (py > (y + ((h * 2) / 3))) ei.hover_bottom = 0;

   /* Swap right and left because they switch sides in RTL */
   if (elm_widget_mirrored_get(sd->anchor_hover.hover))
     {
        Eina_Bool tmp = ei.hover_left;

        ei.hover_left = ei.hover_right;
        ei.hover_right = tmp;
     }

   evas_object_smart_callback_call(obj, SIG_ANCHOR_HOVER_OPENED, &ei);
   evas_object_smart_callback_add
     (sd->anchor_hover.hover, "clicked", _anchor_hover_clicked_cb, obj);

   /* FIXME: Should just check if there's any callback registered to
    * the smart events instead.  This is used to determine if anyone
    * cares about the hover or not. */
   if (!elm_layout_content_get(sd->anchor_hover.hover, "middle") &&
       !elm_layout_content_get(sd->anchor_hover.hover, "left") &&
       !elm_layout_content_get(sd->anchor_hover.hover, "right") &&
       !elm_layout_content_get(sd->anchor_hover.hover, "top") &&
       !elm_layout_content_get(sd->anchor_hover.hover, "bottom"))
     {
        evas_object_del(sd->anchor_hover.hover);
        sd->anchor_hover.hover = NULL;
     }
   else
     evas_object_show(sd->anchor_hover.hover);
}

static void
_entry_anchor_clicked_signal_cb(void *data,
                                Evas_Object *obj __UNUSED__,
                                const char *emission,
                                const char *source __UNUSED__)
{
   Elm_Entry_Anchor_Info ei;
   const char *p;
   char *p2;

   ELM_ENTRY_DATA_GET(data, sd);

   p = emission + sizeof("nchor,mouse,clicked,");
   ei.button = strtol(p, &p2, 10);
   ei.name = p2 + 1;
   ei.x = ei.y = ei.w = ei.h = 0;

   _signal_anchor_geoms_do_things_with_lol(sd, &ei);

   if (!sd->disabled)
     {
        evas_object_smart_callback_call(data, SIG_ANCHOR_CLICKED, &ei);

        _entry_hover_anchor_clicked_do(data, &ei);
     }
}

static void
_entry_anchor_move_signal_cb(void *data __UNUSED__,
                             Evas_Object *obj __UNUSED__,
                             const char *emission __UNUSED__,
                             const char *source __UNUSED__)
{
}

static void
_entry_anchor_in_signal_cb(void *data,
                           Evas_Object *obj __UNUSED__,
                           const char *emission __UNUSED__,
                           const char *source __UNUSED__)
{
   Elm_Entry_Anchor_Info ei;

   ELM_ENTRY_DATA_GET(data, sd);

   ei.name = emission + sizeof("nchor,mouse,in,");
   ei.button = 0;
   ei.x = ei.y = ei.w = ei.h = 0;

   _signal_anchor_geoms_do_things_with_lol(sd, &ei);

   if (!sd->disabled)
     evas_object_smart_callback_call(data, SIG_ANCHOR_IN, &ei);
}

static void
_entry_anchor_out_signal_cb(void *data,
                            Evas_Object *obj __UNUSED__,
                            const char *emission __UNUSED__,
                            const char *source __UNUSED__)
{
   Elm_Entry_Anchor_Info ei;

   ELM_ENTRY_DATA_GET(data, sd);

   ei.name = emission + sizeof("nchor,mouse,out,");
   ei.button = 0;
   ei.x = ei.y = ei.w = ei.h = 0;

   _signal_anchor_geoms_do_things_with_lol(sd, &ei);

   if (!sd->disabled)
     evas_object_smart_callback_call(data, SIG_ANCHOR_OUT, &ei);
}

static void
_entry_key_enter_signal_cb(void *data,
                           Evas_Object *obj __UNUSED__,
                           const char *emission __UNUSED__,
                           const char *source __UNUSED__)
{
   evas_object_smart_callback_call(data, SIG_ACTIVATED, NULL);
}

static void
_entry_key_escape_signal_cb(void *data,
                            Evas_Object *obj __UNUSED__,
                            const char *emission __UNUSED__,
                            const char *source __UNUSED__)
{
   evas_object_smart_callback_call(data, SIG_ABORTED, NULL);
}

static void
_entry_mouse_down_signal_cb(void *data,
                            Evas_Object *obj __UNUSED__,
                            const char *emission __UNUSED__,
                            const char *source __UNUSED__)
{
   // TIZEN ONLY
   ELM_ENTRY_DATA_GET(data, sd);
   sd->double_clicked = EINA_FALSE;
   /////////
   evas_object_smart_callback_call(data, SIG_PRESS, NULL);
}

static void
_entry_mouse_clicked_signal_cb(void *data,
                               Evas_Object *obj __UNUSED__,
                               const char *emission __UNUSED__,
                               const char *source __UNUSED__)
{
   evas_object_smart_callback_call(data, SIG_CLICKED, NULL);
}

static void
_entry_mouse_double_signal_cb(void *data,
                              Evas_Object *obj __UNUSED__,
                              const char *emission __UNUSED__,
                              const char *source __UNUSED__)
{
   // TIZEN ONLY
   ELM_ENTRY_DATA_GET(data, sd);
   if (sd->disabled) return;
   sd->double_clicked = EINA_TRUE;
   if (!sd->sel_allow) return;

   if (!_elm_config->desktop_entry)
     edje_object_part_text_select_allow_set
        (sd->entry_edje, "elm.text", EINA_TRUE);
   _magnifier_hide(data);
#ifndef ELM_FEATURE_WEARABLE
   _menu_call(data);
#endif
   /////
   evas_object_smart_callback_call(data, SIG_CLICKED_DOUBLE, NULL);
}

static void
_entry_mouse_triple_signal_cb(void *data,
                              Evas_Object *obj __UNUSED__,
                              const char *emission __UNUSED__,
                              const char *source __UNUSED__)
{
   // TIZEN ONLY
   ELM_ENTRY_DATA_GET(data, sd);
   if (sd->disabled) return;
   sd->double_clicked = EINA_TRUE;
   if (!sd->sel_allow) return;

   if (!_elm_config->desktop_entry)
     edje_object_part_text_select_allow_set
        (sd->entry_edje, "elm.text", EINA_TRUE);
   _magnifier_hide(data);
#ifndef ELM_FEATURE_WEARABLE
   _menu_call(data);
#endif
   /////
   evas_object_smart_callback_call(data, SIG_CLICKED_TRIPLE, NULL);
}

#ifdef HAVE_ELEMENTARY_X
static Eina_Bool
_event_selection_notify(void *data,
                        int type __UNUSED__,
                        void *event)
{
   Ecore_X_Event_Selection_Notify *ev = event;

   ELM_ENTRY_DATA_GET(data, sd);

   if ((!sd->selection_asked) && (!sd->drag_selection_asked))
     return ECORE_CALLBACK_PASS_ON;

   if ((ev->selection == ECORE_X_SELECTION_CLIPBOARD) ||
       (ev->selection == ECORE_X_SELECTION_PRIMARY))
     {
        Ecore_X_Selection_Data_Text *text_data;

        text_data = ev->data;
        if (text_data->data.content == ECORE_X_SELECTION_CONTENT_TEXT)
          {
             if (text_data->text)
               {
                  char *txt = _elm_util_text_to_mkup(text_data->text);

                  if (txt)
                    {
                       elm_entry_entry_insert(data, txt);
                       free(txt);
                    }
               }
          }
        sd->selection_asked = EINA_FALSE;
     }
   else if (ev->selection == ECORE_X_SELECTION_XDND)
     {
        Ecore_X_Selection_Data_Text *text_data;

        text_data = ev->data;
        if (text_data->data.content == ECORE_X_SELECTION_CONTENT_TEXT)
          {
             if (text_data->text)
               {
                  char *txt = _elm_util_text_to_mkup(text_data->text);

                  if (txt)
                    {
                       /* Massive FIXME: this should be at the drag point */
                       elm_entry_entry_insert(data, txt);
                       free(txt);
                    }
               }
          }
        sd->drag_selection_asked = EINA_FALSE;

        ecore_x_dnd_send_finished();
     }

   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_event_selection_clear(void *data __UNUSED__,
                       int type __UNUSED__,
                       void *event __UNUSED__)
{
#if 0
   Ecore_X_Event_Selection_Clear *ev = event;

   ELM_ENTRY_DATA_GET(data, sd);

   if (!sd->have_selection) return ECORE_CALLBACK_PASS_ON;
   if ((ev->selection == ECORE_X_SELECTION_CLIPBOARD) ||
       (ev->selection == ECORE_X_SELECTION_PRIMARY))
     {
        elm_entry_select_none(data);
     }
#else
   /// TIZEN ONLY
   Evas_Object *top = elm_widget_top_get(data);
   Ecore_X_Event_Selection_Clear *ev = event;

   if (!top)
      return ECORE_CALLBACK_PASS_ON;

   if (ev->selection != ECORE_X_SELECTION_SECONDARY)
     {
        return ECORE_CALLBACK_PASS_ON;
     }

   if (elm_widget_focus_get(data))
     {
        ELM_ENTRY_DATA_GET(data, sd);
        Elm_Sel_Format formats = ELM_SEL_FORMAT_MARKUP;
        evas_object_smart_callback_call(data, SIG_SELECTION_PASTE, NULL);
        if (sd->cnp_mode == ELM_CNP_MODE_PLAINTEXT)
          formats = ELM_SEL_FORMAT_TEXT;
        else if (sd->cnp_mode != ELM_CNP_MODE_NO_IMAGE)
          formats |= ELM_SEL_FORMAT_IMAGE;
        elm_cnp_selection_get(data, ELM_SEL_TYPE_SECONDARY, formats, NULL, NULL);

        return ECORE_CALLBACK_DONE;
     }
   ///////////
#endif
   return ECORE_CALLBACK_PASS_ON;
}

static void
_dnd_enter_cb(void *data,
              Evas_Object *obj __UNUSED__)
{
   elm_object_focus_set(data, EINA_TRUE);
}

static void
_dnd_leave_cb(void *data,
              Evas_Object *obji __UNUSED__)
{
   elm_object_focus_set(data, EINA_FALSE);
}

static void
_dnd_pos_cb(void *data, Evas_Object *obj __UNUSED__, Evas_Coord x, Evas_Coord y,
            Elm_Xdnd_Action action EINA_UNUSED)
{
   elm_object_focus_set(data, EINA_TRUE);

   ELM_ENTRY_DATA_GET(data, sd);

   edje_object_part_text_cursor_coord_set
      (sd->entry_edje, "elm.text", EDJE_CURSOR_USER, x, y);
   int pos = edje_object_part_text_cursor_pos_get
      (sd->entry_edje, "elm.text", EDJE_CURSOR_USER);
   elm_entry_cursor_pos_set(data, pos);
}

#endif

static Evas_Object *
_item_get(void *data,
          Evas_Object *edje __UNUSED__,
          const char *part __UNUSED__,
          const char *item)
{
   Eina_List *l;
   Evas_Object *o;
   Elm_Entry_Item_Provider *ip;

   ELM_ENTRY_DATA_GET(data, sd);

   EINA_LIST_FOREACH(sd->item_providers, l, ip)
     {
        o = ip->func(ip->data, data, item);
        if (o) return o;
     }
   if (!strncmp(item, "file://", 7))
     {
        const char *fname = item + 7;

        o = evas_object_image_filled_add(evas_object_evas_get(data));
        evas_object_image_file_set(o, fname, NULL);
        if (evas_object_image_load_error_get(o) == EVAS_LOAD_ERROR_NONE)
          {
             evas_object_show(o);
          }
        else
          {
             evas_object_del(o);
             o = evas_object_rectangle_add(evas_object_evas_get(data));
             evas_object_color_set(o, 0, 0, 0, 0);
          }
        return o;
     }

   o = edje_object_add(evas_object_evas_get(data));
   if (!elm_widget_theme_object_set
         (data, o, "entry", item, elm_widget_style_get(data)))
     {
        evas_object_del(o);
        o = evas_object_rectangle_add(evas_object_evas_get(data));
        evas_object_color_set(o, 0, 0, 0, 0);
     }
   return o;
}

static void
_text_filter_cb(void *data,
                Evas_Object *edje __UNUSED__,
                const char *part __UNUSED__,
                Edje_Text_Filter_Type type,
                char **text)
{
   Eina_List *l;
   Elm_Entry_Markup_Filter *tf;

   ELM_ENTRY_DATA_GET(data, sd);

   if (type == EDJE_TEXT_FILTER_FORMAT)
     return;

   EINA_LIST_FOREACH(sd->text_filters, l, tf)
     {
        tf->func(tf->data, data, text);
        if (!*text)
          break;
     }
}

static void
_markup_filter_cb(void *data,
                  Evas_Object *edje __UNUSED__,
                  const char *part __UNUSED__,
                  char **text)
{
   Eina_List *l;
   Elm_Entry_Markup_Filter *tf;

   ELM_ENTRY_DATA_GET(data, sd);

   EINA_LIST_FOREACH(sd->markup_filters, l, tf)
     {
        tf->func(tf->data, data, text);
        if (!*text)
          break;
     }
}

/* This function is used to insert text by chunks in jobs */
static Eina_Bool
_text_append_idler(void *data)
{
   int start;
   char backup;
   Evas_Object *obj = (Evas_Object *)data;

   ELM_ENTRY_DATA_GET(obj, sd);

   evas_event_freeze(evas_object_evas_get(obj));
   if (sd->text) eina_stringshare_del(sd->text);
   sd->text = NULL;
   // TIZEN ONLY
   if (sd->password_text) eina_stringshare_del(sd->password_text);
   sd->password_text = NULL;
   //////
   sd->changed = EINA_TRUE;

   start = sd->append_text_position;
   if ((start + _CHUNK_SIZE) < sd->append_text_len)
     {
        int pos = start;
        int tag_start, esc_start;

        tag_start = esc_start = -1;
        /* Find proper markup cut place */
        while (pos - start < _CHUNK_SIZE)
          {
             int prev_pos = pos;
             Eina_Unicode tmp =
               eina_unicode_utf8_get_next(sd->append_text_left, &pos);

             if (esc_start == -1)
               {
                  if (tmp == '<')
                    tag_start = prev_pos;
                  else if (tmp == '>')
                    tag_start = -1;
               }
             if (tag_start == -1)
               {
                  if (tmp == '&')
                    esc_start = prev_pos;
                  else if (tmp == ';')
                    esc_start = -1;
               }
          }

        if (tag_start >= 0)
          {
             sd->append_text_position = tag_start;
          }
        else if (esc_start >= 0)
          {
             sd->append_text_position = esc_start;
          }
        else
          {
             sd->append_text_position = pos;
          }
     }
   else
     {
        sd->append_text_position = sd->append_text_len;
     }

   backup = sd->append_text_left[sd->append_text_position];
   sd->append_text_left[sd->append_text_position] = '\0';

   edje_object_part_text_append
     (sd->entry_edje, "elm.text", sd->append_text_left + start);

   sd->append_text_left[sd->append_text_position] = backup;

   evas_event_thaw(evas_object_evas_get(obj));
   evas_event_thaw_eval(evas_object_evas_get(obj));

   _elm_entry_guide_update(obj, EINA_TRUE);

   /* If there's still more to go, renew the idler, else, cleanup */
   if (sd->append_text_position < sd->append_text_len)
     {
        return ECORE_CALLBACK_RENEW;
     }
   else
     {
        free(sd->append_text_left);
        sd->append_text_left = NULL;
        sd->append_text_idler = NULL;
        evas_object_smart_callback_call(obj, SIG_TEXT_SET_DONE, NULL);
        return ECORE_CALLBACK_CANCEL;
     }
}

static void
_chars_add_till_limit(Evas_Object *obj,
                      char **text,
                      int can_add,
                      Length_Unit unit)
{
   Eina_List *tag_list = NULL;
   int i = 0, current_len = 0;
   char *new_text;

   if (!*text) return;
   if (unit >= LENGTH_UNIT_LAST) return;
   // TIZEN_ONLY (20131111) : Preediting text is not allowed, either.
   // if (strstr(*text, "<preedit")) return;

   new_text = *text;
   current_len = strlen(*text);
   while (*new_text)
     {
        int idx = 0, unit_size = 0;
        char *markup, *utfstr;

        if (*new_text == '<')
          {
             while (*(new_text + idx) != '>')
               {
                  idx++;
                  if (!*(new_text + idx)) break;
               }
             if ((*(new_text + idx - 1) == '/') && (idx == 2))
               {
                  // </> case
                  if (tag_list && eina_list_data_get(tag_list))
                    {
                       char *tag = eina_list_data_get(tag_list);
                       tag_list = eina_list_remove_list(tag_list, tag_list);
                       free(tag);
                    }
               }
             else if (*(new_text + 1) == '/')
               {
                  // closer case
                  Eina_List *l, *l_next;
                  char *tag;
                  EINA_LIST_FOREACH_SAFE(tag_list, l, l_next, tag)
                    {
                       if (!strncmp(new_text + 1, tag, idx))
                         {
                            tag_list = eina_list_remove_list(tag_list, l);
                            free(tag);
                            break;
                         }
                    }
               }
             else if (*(new_text + idx - 1) != '/')
               {
                  // opener case
                  int len = 0;
                  while (*(new_text + 1 + len))
                    {
                       if ((*(new_text + 1 + len) == ' ') ||
                           (*(new_text + 1 + len) == '=') ||
                           (*(new_text + 1 + len) == '>'))
                         break;
                       len++;
                    }
                  char *opener = malloc(len + 1);
                  strncpy(opener, new_text + 1, len);
                  opener[len] = 0;
                  if (!strcmp(opener, "br") || !strcmp(opener, "ps") ||
                      !strcmp(opener, "tab"))
                    free(opener);
                  else
                    tag_list = eina_list_prepend(tag_list, opener);
               }
          }
        else if (*new_text == '&')
          {
             while (*(new_text + idx) != ';')
               {
                  idx++;
                  if (!*(new_text + idx)) break;
               }
          }
        idx = evas_string_char_next_get(new_text, idx, NULL);
        markup = malloc(idx + 1);
        if (markup)
          {
             strncpy(markup, new_text, idx);
             markup[idx] = 0;
             utfstr = elm_entry_markup_to_utf8(markup);
             if (utfstr)
               {
                  if (unit == LENGTH_UNIT_BYTE)
                    unit_size = strlen(utfstr);
                  else if (unit == LENGTH_UNIT_CHAR)
                    unit_size = evas_string_char_len_get(utfstr);
                  free(utfstr);
                  utfstr = NULL;
               }
             free(markup);
             markup = NULL;
          }
        if (can_add < unit_size)
          {
             if (!i)
               {
                  if (elm_object_focus_get(obj))
                    evas_object_smart_callback_call(obj, SIG_MAX_LENGTH, NULL);
                  free(*text);
                  *text = NULL;
                  return;
               }
             can_add = 0;
             current_len = new_text - *text;
             (*text)[current_len] = 0;
             if (tag_list)
               {
                  Eina_List *l, *l_next;
                  char *tag;

                  Eina_Strbuf *str = eina_strbuf_new();
                  eina_strbuf_append(str, *text);

                  EINA_LIST_FOREACH_SAFE(tag_list, l, l_next, tag)
                    {
                       eina_strbuf_append(str, "</");
                       eina_strbuf_append(str, tag);
                       eina_strbuf_append(str, ">");
                       tag_list = eina_list_remove_list(tag_list, l);
                       free(tag);
                    }
                  free(*text);
                  *text = eina_strbuf_string_steal(str);
                  eina_strbuf_free(str);
               }
             break;
          }
        else
          {
             new_text += idx;
             can_add -= unit_size;
          }
        i++;
     }

   if (elm_object_focus_get(obj))
     evas_object_smart_callback_call(obj, SIG_MAX_LENGTH, NULL);
}

static void
_elm_entry_smart_signal(Evas_Object *obj,
                        const char *emission,
                        const char *source)
{
   ELM_ENTRY_DATA_GET(obj, sd);
   evas_object_ref(obj);
   /* always pass to both edje objs */
   edje_object_signal_emit(sd->entry_edje, emission, source);
   edje_object_message_signal_process(sd->entry_edje);

   if (sd->scr_edje)
     {
        edje_object_signal_emit(sd->scr_edje, emission, source);
        edje_object_message_signal_process(sd->scr_edje);
     }

   // REDWOOD ONLY (140617): Support setting mgf bg from application
   if (!strcmp(emission, "elm,mgf,bg,color"))
     {
        char **arr = eina_str_split(source, ",", 0);
        int i;
        for (i = 0; arr[i]; i++)
          {
             switch (i)
               {
                case 0:
                   sd->mgf_r = atoi(arr[i]);
                   break;
                case 1:
                   sd->mgf_g = atoi(arr[i]);
                   break;
                case 2:
                   sd->mgf_b = atoi(arr[i]);
                   break;
                case 3:
                   sd->mgf_a = atoi(arr[i]);
                   break;
               }
          }
        if (i == 4)
          {
             sd->mgf_bg_color_set = EINA_TRUE;
          }
        else
          {
             sd->mgf_bg_color_set = EINA_FALSE;
          }
     }
   //
   evas_object_unref(obj);
}

static void
_elm_entry_smart_callback_add(Evas_Object *obj,
                              const char *emission,
                              const char *source,
                              Edje_Signal_Cb func_cb,
                              void *data)
{
   Evas_Object *ro;

   ELM_ENTRY_DATA_GET(obj, sd);

   ro = ELM_WIDGET_DATA(sd)->resize_obj;

   ELM_WIDGET_DATA(sd)->resize_obj = sd->entry_edje;

   ELM_LAYOUT_CLASS(_elm_entry_parent_sc)->callback_add
     (obj, emission, source, func_cb, data);

   if (sd->scr_edje)
     {
        ELM_WIDGET_DATA(sd)->resize_obj = sd->scr_edje;

        ELM_LAYOUT_CLASS(_elm_entry_parent_sc)->callback_add
          (obj, emission, source, func_cb, data);
     }

   ELM_WIDGET_DATA(sd)->resize_obj = ro;
}

static void *
_elm_entry_smart_callback_del(Evas_Object *obj,
                              const char *emission,
                              const char *source,
                              Edje_Signal_Cb func_cb)
{
   Evas_Object *ro;
   void *data;

   ELM_ENTRY_DATA_GET(obj, sd);

   ro = ELM_WIDGET_DATA(sd)->resize_obj;

   ELM_WIDGET_DATA(sd)->resize_obj = sd->entry_edje;

   data = ELM_LAYOUT_CLASS(_elm_entry_parent_sc)->callback_del
       (obj, emission, source, func_cb);

   if (sd->scr_edje)
     {
        ELM_WIDGET_DATA(sd)->resize_obj = sd->scr_edje;

        ELM_LAYOUT_CLASS(_elm_entry_parent_sc)->callback_del
          (obj, emission, source, func_cb);
     }

   ELM_WIDGET_DATA(sd)->resize_obj = ro;

   return data;
}

static Eina_Bool
_elm_entry_smart_content_set(Evas_Object *obj,
                             const char *part,
                             Evas_Object *content)
{
   if (!ELM_CONTAINER_CLASS(_elm_entry_parent_sc)->content_set
         (obj, part, content))
     return EINA_FALSE;

   /* too bad entry does not follow the pattern
    * "elm,state,{icon,end},visible", we have to repeat ourselves */
   if (!part || !strcmp(part, "icon") || !strcmp(part, "elm.swallow.icon"))
     elm_entry_icon_visible_set(obj, EINA_TRUE);

   if (!part || !strcmp(part, "end") || !strcmp(part, "elm.swallow.end"))
     elm_entry_end_visible_set(obj, EINA_TRUE);

   return EINA_TRUE;
}

static Evas_Object *
_elm_entry_smart_content_unset(Evas_Object *obj,
                               const char *part)
{
   Evas_Object *ret;

   ret = ELM_CONTAINER_CLASS(_elm_entry_parent_sc)->content_unset(obj, part);
   if (!ret) return NULL;

   /* too bad entry does not follow the pattern
    * "elm,state,{icon,end},hidden", we have to repeat ourselves */
   if (!part || !strcmp(part, "icon") || !strcmp(part, "elm.swallow.icon"))
     elm_entry_icon_visible_set(obj, EINA_FALSE);

   if (!part || !strcmp(part, "end") || !strcmp(part, "elm.swallow.end"))
     elm_entry_end_visible_set(obj, EINA_FALSE);

   return ret;
}

static Eina_Bool
_elm_entry_smart_text_set(Evas_Object *obj,
                          const char *item,
                          const char *entry)
{
   int len = 0;

   ELM_ENTRY_DATA_GET(obj, sd);

   if (!entry) entry = "";
   if (item)
     {
        if (!strcmp(item, "guide"))
          edje_object_part_text_set(sd->entry_edje, "elm.guide", entry);
        else
          edje_object_part_text_set(sd->entry_edje, item, entry);

        return EINA_TRUE;
     }

   evas_event_freeze(evas_object_evas_get(obj));
   if (sd->text) eina_stringshare_del(sd->text);
   sd->text = NULL;
   // TIZEN ONLY
   if (sd->password_text) eina_stringshare_del(sd->password_text);
   sd->password_text = NULL;
   /////
   sd->changed = EINA_TRUE;

   /* Clear currently pending job if there is one */
   if (sd->append_text_idler)
     {
        ecore_idler_del(sd->append_text_idler);
        sd->append_text_idler = NULL;
     }

   if (sd->append_text_left)
     {
        free(sd->append_text_left);
        sd->append_text_left = NULL;
     }

   len = strlen(entry);
   /* Split to ~_CHUNK_SIZE chunks */
   if (len > _CHUNK_SIZE)
     {
        sd->append_text_left = (char *)malloc(len + 1);
     }

   /* If we decided to use the idler */
   if (sd->append_text_left)
     {
        /* Need to clear the entry first */
        edje_object_part_text_set(sd->entry_edje, "elm.text", "");
        memcpy(sd->append_text_left, entry, len);
        sd->append_text_left[len] = '\0';
        sd->append_text_position = 0;
        sd->append_text_len = len;
        sd->append_text_idler = ecore_idler_add(_text_append_idler, obj);
     }
   else
     {
        edje_object_part_text_set(sd->entry_edje, "elm.text", entry);
        evas_object_smart_callback_call(obj, SIG_TEXT_SET_DONE, NULL);
     }

   if (len > 0)
     _elm_entry_guide_update(obj, EINA_TRUE);
   else
     _elm_entry_guide_update(obj, EINA_FALSE);

   // TIZEN_ONLY(20140625): Support accessibility for entry anchors.
   _elm_entry_anchors_highlight_clear(obj);
   //
   evas_event_thaw(evas_object_evas_get(obj));
   evas_event_thaw_eval(evas_object_evas_get(obj));

   return EINA_TRUE;
}

static const char *
_elm_entry_smart_text_get(const Evas_Object *obj,
                          const char *item)
{
   const char *text;

   ELM_ENTRY_DATA_GET(obj, sd);

   if (item)
     {
        if (!strcmp(item, "default")) goto proceed;
        else if (!strcmp(item, "guide"))
          return edje_object_part_text_get(sd->entry_edje, "elm.guide");
        else
          return edje_object_part_text_get(sd->entry_edje, item);
     }

proceed:

   text = edje_object_part_text_get(sd->entry_edje, "elm.text");
   if (!text)
     {
        ERR("text=NULL for edje %p, part 'elm.text'", sd->entry_edje);

        return NULL;
     }

   if (sd->append_text_len > 0)
     {
        char *tmpbuf;
        size_t len, tlen;

        tlen = strlen(text);
        len = tlen +  sd->append_text_len - sd->append_text_position;
        tmpbuf = calloc(1, len + 1);
        if (!tmpbuf)
          {
             ERR("Failed to allocate memory for entry's text %p", obj);
             return NULL;
          }
        memcpy(tmpbuf, text, tlen);

        if (sd->append_text_left)
          memcpy(tmpbuf + tlen, sd->append_text_left
                 + sd->append_text_position, sd->append_text_len
                 - sd->append_text_position);
        tmpbuf[len] = '\0';
        eina_stringshare_replace(&sd->text, tmpbuf);
        free(tmpbuf);
     }
   else
     {
        eina_stringshare_replace(&sd->text, text);
     }

   // TIZEN ONLY
   if (sd->password)
     {
        char *pw_text;
        pw_text = elm_entry_markup_to_utf8(sd->text);
        if (pw_text)
          {
             eina_stringshare_replace(&sd->password_text, pw_text);
             free(pw_text);
             return sd->password_text;
          }
     }
   /////

   return sd->text;
}

// TIZEN_ONLY(131221) : Customize accessiblity for editfield
/* Original Code */
/*
static char *
_access_info_cb(void *data __UNUSED__, Evas_Object *obj)
{
   const char *txt;

   ELM_ENTRY_DATA_GET(obj, sd);

   if ((sd->password && _elm_config->access_password_read_enable) ||
       (!sd->password))
     {
        txt = elm_widget_access_info_get(obj);

        if (!txt) txt = _elm_util_mkup_to_text(elm_entry_entry_get(obj));
        if (txt && (strlen(txt) > 0)) return strdup(txt);
     }

   // to take care guide text
   txt = edje_object_part_text_get(sd->entry_edje, "elm.guide");
   txt = _elm_util_mkup_to_text(txt);
   if (txt && (strlen(txt) > 0)) return strdup(txt);

   return NULL;
}

static char *
_access_state_cb(void *data __UNUSED__, Evas_Object *obj)
{
   Eina_Strbuf *buf;
   char *ret;

   ELM_ENTRY_DATA_GET(obj, sd);

   ret = NULL;
   buf = eina_strbuf_new();

   if (elm_widget_disabled_get(obj))
     eina_strbuf_append(buf, E_("State: Disabled"));

   if (!sd->editable)
     {
        if (!eina_strbuf_length_get(buf))
          eina_strbuf_append(buf, E_("State: Not Editable"));
        else eina_strbuf_append(buf, E_(", Not Editable"));
     }

   if (!eina_strbuf_length_get(buf)) goto buf_free;

   ret = eina_strbuf_string_steal(buf);

buf_free:
   eina_strbuf_free(buf);
   return ret;
}
*/
/////////////////////////////////

static void
_entry_selection_callbacks_unregister(Evas_Object *obj)
{
   ELM_ENTRY_DATA_GET(obj, sd);

   edje_object_signal_callback_del_full
     (sd->entry_edje, "selection,start", "elm.text",
     _entry_selection_start_signal_cb, obj);
   edje_object_signal_callback_del_full
     (sd->entry_edje, "selection,changed", "elm.text",
     _entry_selection_changed_signal_cb, obj);
   edje_object_signal_callback_del_full
     (sd->entry_edje, "entry,selection,all,request",
     "elm.text", _entry_selection_all_signal_cb, obj);
   edje_object_signal_callback_del_full
     (sd->entry_edje, "entry,selection,none,request",
     "elm.text", _entry_selection_none_signal_cb, obj);
   edje_object_signal_callback_del_full
     (sd->entry_edje, "selection,cleared", "elm.text",
     _entry_selection_cleared_signal_cb, obj);
   edje_object_signal_callback_del_full
     (sd->entry_edje, "entry,paste,request,*", "elm.text",
     _entry_paste_request_signal_cb, obj);
   edje_object_signal_callback_del_full
     (sd->entry_edje, "entry,copy,notify", "elm.text",
     _entry_copy_notify_signal_cb, obj);
   edje_object_signal_callback_del_full
     (sd->entry_edje, "entry,cut,notify", "elm.text",
     _entry_cut_notify_signal_cb, obj);
}

static void
_entry_selection_callbacks_register(Evas_Object *obj)
{
   ELM_ENTRY_DATA_GET(obj, sd);

   edje_object_signal_callback_add
     (sd->entry_edje, "selection,start", "elm.text",
     _entry_selection_start_signal_cb, obj);
   edje_object_signal_callback_add
     (sd->entry_edje, "selection,changed", "elm.text",
     _entry_selection_changed_signal_cb, obj);
   edje_object_signal_callback_add
     (sd->entry_edje, "entry,selection,all,request",
     "elm.text", _entry_selection_all_signal_cb, obj);
   edje_object_signal_callback_add
     (sd->entry_edje, "entry,selection,none,request",
     "elm.text", _entry_selection_none_signal_cb, obj);
   edje_object_signal_callback_add
     (sd->entry_edje, "selection,cleared", "elm.text",
     _entry_selection_cleared_signal_cb, obj);
   edje_object_signal_callback_add
     (sd->entry_edje, "entry,paste,request,*", "elm.text",
     _entry_paste_request_signal_cb, obj);
   edje_object_signal_callback_add
     (sd->entry_edje, "entry,copy,notify", "elm.text",
     _entry_copy_notify_signal_cb, obj);
   edje_object_signal_callback_add
     (sd->entry_edje, "entry,cut,notify", "elm.text",
     _entry_cut_notify_signal_cb, obj);
}

static void
_resize_cb(void *data,
           Evas *e __UNUSED__,
           Evas_Object *obj __UNUSED__,
           void *event_info __UNUSED__)
{
   ELM_ENTRY_DATA_GET(data, sd);

   if (sd->line_wrap)
     {
        elm_layout_sizing_eval(data);
     }
   else if (sd->scroll)
     {
        Evas_Coord vw = 0, vh = 0;

        sd->s_iface->content_viewport_size_get(data, &vw, &vh);
        if (vw < sd->ent_mw) vw = sd->ent_mw;
        if (vh < sd->ent_mh) vh = sd->ent_mh;
        evas_object_resize(sd->entry_edje, vw, vh);
     }

   if (sd->hoversel) _hoversel_position(data);

   // TIZEN ONLY
   if (!_elm_config->desktop_entry)
     {
        if (sd->region_get_job) ecore_job_del(sd->region_get_job);
        sd->region_get_job = ecore_job_add(_region_get_job, data);

        if (sd->magnifier_showing)
          _magnifier_move(data);
     }
   //
}

static void
_elm_entry_smart_add(Evas_Object *obj)
{
   EVAS_SMART_DATA_ALLOC(obj, Elm_Entry_Smart_Data);

   ELM_WIDGET_CLASS(_elm_entry_parent_sc)->base.add(obj);
}

static void
_elm_entry_smart_del(Evas_Object *obj)
{
   Elm_Entry_Context_Menu_Item *it;
   Elm_Entry_Item_Provider *ip;
   Elm_Entry_Markup_Filter *tf;
   // TIZEN_ONLY(20140625): Add elm_entry_anchor_access_provider_* APIs.
   Elm_Entry_Anchor_Access_Provider *ap;
   //

   ELM_ENTRY_DATA_GET(obj, sd);

   if (sd->delay_write)
     {
        ecore_timer_del(sd->delay_write);
        sd->delay_write = NULL;
        if (sd->auto_save) _save_do(obj);
     }

   // TIZEN ONLY(131106): Add rectangle for processing mouse up event on paddings inside of entry.
   if (sd->event_rect)
     {
        evas_object_del(sd->event_rect);
        sd->event_rect = NULL;
     }
   //
   if (sd->scroll)
     sd->s_iface->content_viewport_resize_cb_set(obj, NULL);

   // TIZEN_ONLY(20140625): Support accessibility for entry anchors.
   _elm_entry_anchors_highlight_clear(obj);
   //

   elm_entry_anchor_hover_end(obj);
   elm_entry_anchor_hover_parent_set(obj, NULL);

   evas_event_freeze(evas_object_evas_get(obj));

   if (sd->file) eina_stringshare_del(sd->file);

   if (sd->hov_deljob) ecore_job_del(sd->hov_deljob);
   if ((sd->api) && (sd->api->obj_unhook))
     sd->api->obj_unhook(obj);  // module - unhook

   entries = eina_list_remove(entries, obj);
#ifdef HAVE_ELEMENTARY_X
   if (sd->sel_notify_handler)
     ecore_event_handler_del(sd->sel_notify_handler);
   if (sd->sel_clear_handler)
     ecore_event_handler_del(sd->sel_clear_handler);
   if (sd->client_msg_handler)                        // TIZEN ONLY
     ecore_event_handler_del(sd->client_msg_handler); //
   if (sd->prop_hdl) ecore_event_handler_del(sd->prop_hdl); // TIZEN ONLY
#endif
   if (sd->cut_sel) eina_stringshare_del(sd->cut_sel);
   if (sd->text) eina_stringshare_del(sd->text);
   if (sd->deferred_recalc_job)
     ecore_job_del(sd->deferred_recalc_job);
   if (sd->append_text_idler)
     {
        ecore_idler_del(sd->append_text_idler);
        free(sd->append_text_left);
        sd->append_text_left = NULL;
        sd->append_text_idler = NULL;
     }
   if (sd->longpress_timer) ecore_timer_del(sd->longpress_timer);
   EINA_LIST_FREE (sd->items, it)
     {
        eina_stringshare_del(it->label);
        eina_stringshare_del(it->icon_file);
        eina_stringshare_del(it->icon_group);
        free(it);
     }
   EINA_LIST_FREE (sd->item_providers, ip)
     {
        free(ip);
     }
   EINA_LIST_FREE (sd->text_filters, tf)
     {
        _filter_free(tf);
     }
   EINA_LIST_FREE (sd->markup_filters, tf)
     {
        _filter_free(tf);
     }
   // TIZEN_ONLY(20140625): Add elm_entry_anchor_access_provider_* APIs.
   EINA_LIST_FREE (sd->anchor_access_providers, ap)
     {
        free(ap);
     }
   //
   if (sd->delay_write) ecore_timer_del(sd->delay_write);
   if (sd->input_panel_imdata) free(sd->input_panel_imdata);

   if (sd->anchor_hover.hover_style)
     eina_stringshare_del(sd->anchor_hover.hover_style);

   //TIZEN ONLY
   if (sd->password_text) eina_stringshare_del(sd->password_text);
   if (sd->region_get_job) ecore_job_del(sd->region_get_job);
   if (sd->region_recalc_job) ecore_job_del(sd->region_recalc_job);
   if (sd->mgf_proxy) evas_object_del(sd->mgf_proxy);
   if (sd->mgf_bg) evas_object_del(sd->mgf_bg);
   //
   evas_event_thaw(evas_object_evas_get(obj));
   evas_event_thaw_eval(evas_object_evas_get(obj));

   ELM_WIDGET_CLASS(_elm_entry_parent_sc)->base.del(obj);
}

static void
_elm_entry_smart_move(Evas_Object *obj,
                      Evas_Coord x,
                      Evas_Coord y)
{
   ELM_ENTRY_DATA_GET(obj, sd);

   ELM_WIDGET_CLASS(_elm_entry_parent_sc)->base.move(obj, x, y);

   evas_object_move(sd->hit_rect, x, y);

   if (sd->hoversel) _hoversel_position(obj);

   // TIZEN ONLY
   if (!_elm_config->desktop_entry)
     {
        if (sd->region_get_job) ecore_job_del(sd->region_get_job);
        sd->region_get_job = ecore_job_add(_region_get_job, obj);
     }
   //
}

static void
_elm_entry_smart_resize(Evas_Object *obj,
                        Evas_Coord w,
                        Evas_Coord h)
{
   ELM_ENTRY_DATA_GET(obj, sd);

   ELM_WIDGET_CLASS(_elm_entry_parent_sc)->base.resize(obj, w, h);

   evas_object_resize(sd->hit_rect, w, h);
}

static void
_elm_entry_smart_member_add(Evas_Object *obj,
                            Evas_Object *member)
{
   ELM_ENTRY_DATA_GET(obj, sd);

   ELM_WIDGET_CLASS(_elm_entry_parent_sc)->base.member_add(obj, member);

   if (sd->hit_rect)
     evas_object_raise(sd->hit_rect);
}

// TIZEN_ONLY(131221) : Customize accessiblity for editfield
/* Original Code */
/*
static Eina_Bool
_elm_entry_smart_activate(Evas_Object *obj, Elm_Activate act)
{
   if (act != ELM_ACTIVATE_DEFAULT) return EINA_FALSE;

   ELM_ENTRY_DATA_GET(obj, sd);

   if (!elm_widget_disabled_get(obj) &&
       !evas_object_freeze_events_get(obj))
     {
        evas_object_smart_callback_call(obj, SIG_CLICKED, NULL);
        if (sd->editable && sd->input_panel_enable)
          edje_object_part_text_input_panel_show(sd->entry_edje, "elm.text");
     }
   return EINA_TRUE;
}
*/
////////////////////

static void
_elm_entry_smart_set_user(Elm_Entry_Smart_Class *sc)
{
   ELM_WIDGET_CLASS(sc)->base.add = _elm_entry_smart_add;
   ELM_WIDGET_CLASS(sc)->base.del = _elm_entry_smart_del;
   ELM_WIDGET_CLASS(sc)->base.move = _elm_entry_smart_move;
   ELM_WIDGET_CLASS(sc)->base.resize = _elm_entry_smart_resize;
   ELM_WIDGET_CLASS(sc)->base.member_add = _elm_entry_smart_member_add;

   ELM_WIDGET_CLASS(sc)->on_focus_region = _elm_entry_smart_on_focus_region;
   ELM_WIDGET_CLASS(sc)->sub_object_del = _elm_entry_smart_sub_object_del;
   ELM_WIDGET_CLASS(sc)->on_focus = _elm_entry_smart_on_focus;
   ELM_WIDGET_CLASS(sc)->theme = _elm_entry_smart_theme;
   ELM_WIDGET_CLASS(sc)->disable = _elm_entry_smart_disable;
   ELM_WIDGET_CLASS(sc)->translate = _elm_entry_smart_translate;
   // TIZEN_ONLY(131221) : Customize accessiblity for editfield
   //ELM_WIDGET_CLASS(sc)->activate = _elm_entry_smart_activate;
   ELM_WIDGET_CLASS(sc)->activate = NULL;
   ELM_WIDGET_CLASS(sc)->access = _elm_entry_smart_access;
   //

   /* not a 'focus chain manager' */
   // TIZEN_ONLY(131221) : Customize accessiblity for editfield
   //ELM_WIDGET_CLASS(sc)->focus_next = NULL;
   ELM_WIDGET_CLASS(sc)->focus_next = _elm_entry_smart_focus_next;
   //
   ELM_WIDGET_CLASS(sc)->focus_direction_manager_is = NULL;
   ELM_WIDGET_CLASS(sc)->focus_direction = NULL;

   ELM_CONTAINER_CLASS(sc)->content_set = _elm_entry_smart_content_set;
   ELM_CONTAINER_CLASS(sc)->content_unset = _elm_entry_smart_content_unset;

   ELM_LAYOUT_CLASS(sc)->signal = _elm_entry_smart_signal;
   ELM_LAYOUT_CLASS(sc)->callback_add = _elm_entry_smart_callback_add;
   ELM_LAYOUT_CLASS(sc)->callback_del = _elm_entry_smart_callback_del;
   ELM_LAYOUT_CLASS(sc)->text_set = _elm_entry_smart_text_set;
   ELM_LAYOUT_CLASS(sc)->text_get = _elm_entry_smart_text_get;
   ELM_LAYOUT_CLASS(sc)->sizing_eval = _elm_entry_smart_sizing_eval;
   ELM_LAYOUT_CLASS(sc)->content_aliases = _content_aliases;
}

EAPI const Elm_Entry_Smart_Class *
elm_entry_smart_class_get(void)
{
   static Elm_Entry_Smart_Class _sc =
     ELM_ENTRY_SMART_CLASS_INIT_NAME_VERSION(ELM_ENTRY_SMART_NAME);
   static const Elm_Entry_Smart_Class *class = NULL;
   Evas_Smart_Class *esc = (Evas_Smart_Class *)&_sc;

   if (class)
     return class;

   _elm_entry_smart_set(&_sc);
   esc->callbacks = _smart_callbacks;
   class = &_sc;

   return class;
}

EAPI Evas_Object *
elm_entry_add(Evas_Object *parent)
{
   Evas_Object *obj;

#ifdef HAVE_ELEMENTARY_X
   Evas_Object *top;
#endif

   EINA_SAFETY_ON_NULL_RETURN_VAL(parent, NULL);

   obj = elm_widget_add(_elm_entry_smart_class_new(), parent);
   if (!obj) return NULL;

   if (!elm_widget_sub_object_add(parent, obj))
     ERR("could not add %p as sub object of %p", obj, parent);

   ELM_ENTRY_DATA_GET(obj, sd);

   sd->entry_edje = ELM_WIDGET_DATA(sd)->resize_obj;

   sd->cnp_mode = ELM_CNP_MODE_MARKUP;
   sd->line_wrap = ELM_WRAP_WORD;
   sd->context_menu = EINA_TRUE;
   sd->disabled = EINA_FALSE;
   sd->auto_save = EINA_TRUE;
   sd->editable = EINA_TRUE;
   sd->scroll = EINA_FALSE;

   sd->input_panel_imdata = NULL;
   //TIZEN ONLY
   sd->magnifier_enabled = EINA_TRUE;
#ifdef ELM_FEATURE_WEARABLE
   sd->drag_enabled = EINA_FALSE;
   sd->drop_enabled = EINA_FALSE;
#else
   sd->drag_enabled = EINA_TRUE;
   sd->drop_enabled = EINA_TRUE;
#endif
   sd->mouse_upped = EINA_FALSE;
   sd->append_text_idler = NULL;
   sd->append_text_left = NULL;
#ifdef ELM_FEATURE_WEARABLE
   sd->sel_allow = EINA_FALSE;
#else
   sd->sel_allow = EINA_TRUE;
#endif
   sd->cursor_handler_disabled = EINA_FALSE;
   sd->scroll_holding = EINA_FALSE;
   //
   // TIZEN_ONLY(20140625): Support accessibility for entry anchors.
   sd->anchor_highlight_rect = NULL;
   sd->anchor_highlight_pos = -1;
   //
   // TIZEN_ONLY(20140625): Add elm_entry_anchor_access_provider_* APIs.
   sd->anchor_access_providers = NULL;
   //

#ifdef HAVE_ELEMENTARY_X
   sd->drop_format = ELM_SEL_FORMAT_MARKUP | ELM_SEL_FORMAT_IMAGE;
   elm_drop_target_add(obj, sd->drop_format,
                       _dnd_enter_cb, obj,
                       _dnd_leave_cb, obj,
                       _dnd_pos_cb, obj,
                       _drag_drop_cb, NULL);
   sd->drop_added = EINA_TRUE;
#endif
   elm_layout_theme_set(obj, "entry", "base", elm_widget_style_get(obj));

   sd->hit_rect = evas_object_rectangle_add(evas_object_evas_get(obj));
   evas_object_data_set(sd->hit_rect, "_elm_leaveme", obj);

   Evas_Object* clip = evas_object_clip_get(sd->entry_edje);
   evas_object_clip_set(sd->hit_rect, clip);

   evas_object_smart_member_add(sd->hit_rect, obj);
   elm_widget_sub_object_add(obj, sd->hit_rect);

   /* common scroller hit rectangle setup */
   evas_object_color_set(sd->hit_rect, 0, 0, 0, 0);
   evas_object_show(sd->hit_rect);
   evas_object_repeat_events_set(sd->hit_rect, EINA_TRUE);

   sd->s_iface = evas_object_smart_interface_get
       (obj, ELM_SCROLLABLE_IFACE_NAME);

   sd->s_iface->objects_set(obj, sd->entry_edje, sd->hit_rect);

   edje_object_item_provider_set(sd->entry_edje, _item_get, obj);

   edje_object_text_insert_filter_callback_add
     (sd->entry_edje, "elm.text", _text_filter_cb, obj);
   edje_object_text_markup_filter_callback_add
     (sd->entry_edje, "elm.text", _markup_filter_cb, obj);

   evas_object_event_callback_add
     (sd->entry_edje, EVAS_CALLBACK_MOUSE_DOWN, _mouse_down_cb, obj);
   evas_object_event_callback_add
     (sd->entry_edje, EVAS_CALLBACK_MOUSE_UP, _mouse_up_cb, obj);
   evas_object_event_callback_add
     (sd->entry_edje, EVAS_CALLBACK_MOUSE_MOVE, _mouse_move_cb, obj);

   /* this code can't go in smart_resize. sizing gets wrong */
   evas_object_event_callback_add(obj, EVAS_CALLBACK_RESIZE, _resize_cb, obj);

   // TIZEN_ONLY(20140625): Support accessibility for entry anchors.
   evas_object_event_callback_add(obj, EVAS_CALLBACK_HIDE, _elm_entry_hide_cb, obj);
   //

   edje_object_signal_callback_add
     (sd->entry_edje, "entry,changed", "elm.text",
     _entry_changed_signal_cb, obj);
   edje_object_signal_callback_add
     (sd->entry_edje, "entry,changed,user", "elm.text",
     _entry_changed_user_signal_cb, obj);
   edje_object_signal_callback_add
     (sd->entry_edje, "preedit,changed", "elm.text",
     _entry_preedit_changed_signal_cb, obj);

   _entry_selection_callbacks_register(obj);

   edje_object_signal_callback_add
     (sd->entry_edje, "cursor,changed", "elm.text",
     _entry_cursor_changed_signal_cb, obj);
   edje_object_signal_callback_add
     (sd->entry_edje, "cursor,changed,manual", "elm.text",
     _entry_cursor_changed_manual_signal_cb, obj);
   edje_object_signal_callback_add
     (sd->entry_edje, "anchor,mouse,down,*", "elm.text",
     _entry_anchor_down_signal_cb, obj);
   edje_object_signal_callback_add
     (sd->entry_edje, "anchor,mouse,up,*", "elm.text",
     _entry_anchor_up_signal_cb, obj);
   edje_object_signal_callback_add
     (sd->entry_edje, "anchor,mouse,clicked,*", "elm.text",
     _entry_anchor_clicked_signal_cb, obj);
   edje_object_signal_callback_add
     (sd->entry_edje, "anchor,mouse,move,*", "elm.text",
     _entry_anchor_move_signal_cb, obj);
   edje_object_signal_callback_add
     (sd->entry_edje, "anchor,mouse,in,*", "elm.text",
     _entry_anchor_in_signal_cb, obj);
   edje_object_signal_callback_add
     (sd->entry_edje, "anchor,mouse,out,*", "elm.text",
     _entry_anchor_out_signal_cb, obj);
   edje_object_signal_callback_add
     (sd->entry_edje, "entry,key,enter", "elm.text",
     _entry_key_enter_signal_cb, obj);
   edje_object_signal_callback_add
     (sd->entry_edje, "entry,key,escape", "elm.text",
     _entry_key_escape_signal_cb, obj);
   edje_object_signal_callback_add
     (sd->entry_edje, "mouse,down,1", "elm.text",
     _entry_mouse_down_signal_cb, obj);
   edje_object_signal_callback_add
     (sd->entry_edje, "mouse,clicked,1", "elm.text",
     _entry_mouse_clicked_signal_cb, obj);
   edje_object_signal_callback_add
     (sd->entry_edje, "mouse,down,1,double", "elm.text",
     _entry_mouse_double_signal_cb, obj);
   edje_object_signal_callback_add
     (sd->entry_edje, "mouse,down,1,triple", "elm.text",
     _entry_mouse_triple_signal_cb, obj);
   edje_object_signal_callback_add
     (sd->entry_edje, "entry,undo,request", "elm.text",
     _entry_undo_request_signal_cb, obj);
   edje_object_signal_callback_add
     (sd->entry_edje, "entry,redo,request", "elm.text",
     _entry_redo_request_signal_cb, obj);

   // TIZEN ONLY
   edje_object_signal_callback_add
      (sd->entry_edje, "handler,move,start", "elm.text",
       _signal_handler_move_start_cb, obj);
   edje_object_signal_callback_add
      (sd->entry_edje, "handler,move,end", "elm.text",
       _signal_handler_move_end_cb, obj);
   edje_object_signal_callback_add
      (sd->entry_edje, "handler,moving", "elm.text",
       _signal_handler_moving_cb, obj);
   edje_object_signal_callback_add
      (sd->entry_edje, "selection,end", "elm.text",
       _signal_selection_end, obj);
   edje_object_signal_callback_add
      (sd->entry_edje, "long,pressed", "elm.text",
       _signal_long_pressed, obj);
   edje_object_signal_callback_add
      (sd->entry_edje, "magnifier,changed", "elm.text",
       _signal_magnifier_changed, obj);
   edje_object_signal_callback_add
      (sd->entry_edje, "cursor,handler,move,start", "elm.text",
       _signal_handler_move_start_cb, obj);
   edje_object_signal_callback_add
      (sd->entry_edje, "cursor,handler,move,end", "elm.text",
       _signal_handler_move_end_cb, obj);
   edje_object_signal_callback_add
      (sd->entry_edje, "cursor,handler,moving", "elm.text",
       _signal_handler_moving_cb, obj);
   edje_object_signal_callback_add
      (sd->entry_edje, "cursor,handler,clicked", "elm.text",
       _signal_handler_click_cb, obj);
   evas_object_event_callback_add(sd->entry_edje, EVAS_CALLBACK_KEY_DOWN,
       _elm_entry_key_down_cb, obj);
   /////////

   elm_layout_text_set(obj, "elm.text", "");

   elm_object_sub_cursor_set
     (ELM_WIDGET_DATA(sd)->resize_obj, obj, ELM_CURSOR_XTERM);
   elm_widget_can_focus_set(obj, EINA_TRUE);
   if (_elm_config->desktop_entry)
     edje_object_part_text_select_allow_set
       (sd->entry_edje, "elm.text", EINA_TRUE);
   else
     {
        // TIZEN ONLY
        edje_object_part_text_select_allow_set(sd->entry_edje, "elm.text", EINA_FALSE);
        edje_object_part_text_viewport_region_set(sd->entry_edje, "elm.text", -1, -1, -1, -1);
        edje_object_part_text_layout_region_set(sd->entry_edje, "elm.text", -1, -1, -1, -1);
        ////////
     }

   elm_layout_sizing_eval(obj);

   elm_entry_input_panel_layout_set(obj, ELM_INPUT_PANEL_LAYOUT_NORMAL);
   elm_entry_input_panel_enabled_set(obj, EINA_TRUE);
   elm_entry_prediction_allow_set(obj, EINA_TRUE);
   elm_entry_input_hint_set(obj, ELM_INPUT_HINT_AUTO_COMPLETE);

   sd->autocapital_type = edje_object_part_text_autocapital_type_get
       (sd->entry_edje, "elm.text");

#ifdef HAVE_ELEMENTARY_X
   top = elm_widget_top_get(obj);
   if ((top) && (elm_win_xwindow_get(top)))
     {
        sd->sel_notify_handler =
          ecore_event_handler_add
            (ECORE_X_EVENT_SELECTION_NOTIFY, _event_selection_notify, obj);
        sd->sel_clear_handler =
          ecore_event_handler_add
            (ECORE_X_EVENT_SELECTION_CLEAR, _event_selection_clear, obj);
     }

   sd->client_msg_handler = ecore_event_handler_add(ECORE_X_EVENT_CLIENT_MESSAGE, _xclient_msg_cb, obj);  // TIZEN ONLY

   sd->prop_hdl = ecore_event_handler_add
      (ECORE_X_EVENT_WINDOW_PROPERTY, _on_prop_change, obj); // TIZEN ONLY
#endif

   entries = eina_list_prepend(entries, obj);

#ifndef ELM_FEATURE_WEARABLE
   // module - find module for entry
   sd->api = _module_find(obj);
   // if found - hook in
   if ((sd->api) && (sd->api->obj_hook)) sd->api->obj_hook(obj);
#endif

   _mirrored_set(obj, elm_widget_mirrored_get(obj));

   // TIZEN_ONLY(131221): Customize accessiblity for editfield
   /* Original Code */
   /*
   // access
   _elm_access_object_register(obj, sd->entry_edje);
   _elm_access_text_set
     (_elm_access_object_get(obj), ELM_ACCESS_TYPE, E_("IDS_TPLATFORM_BODY_EDIT_FIELD_M_NOUN_T_TTS")); // TIZEN ONLY
   _elm_access_callback_set
     (_elm_access_object_get(obj), ELM_ACCESS_INFO, _access_info_cb, NULL);
   _elm_access_callback_set
     (_elm_access_object_get(obj), ELM_ACCESS_STATE, _access_state_cb, NULL);
   */
   if (_elm_config->access_mode)
     _access_obj_process(obj, EINA_TRUE);
   //////////////////////////

   //Tizen Only: This should be removed when eo is applied.
   ELM_WIDGET_DATA_GET(obj, wsd);
   wsd->on_create = EINA_FALSE;

   return obj;
}

EAPI void
elm_entry_text_style_user_push(Evas_Object *obj,
                               const char *style)
{
   ELM_ENTRY_CHECK(obj);
   ELM_ENTRY_DATA_GET(obj, sd);

   edje_object_part_text_style_user_push(sd->entry_edje, "elm.text", style);
   _elm_entry_smart_theme(obj);
}

EAPI void
elm_entry_text_style_user_pop(Evas_Object *obj)
{
   ELM_ENTRY_CHECK(obj);
   ELM_ENTRY_DATA_GET(obj, sd);

   edje_object_part_text_style_user_pop(sd->entry_edje, "elm.text");

   _elm_entry_smart_theme(obj);
}

EAPI const char *
elm_entry_text_style_user_peek(const Evas_Object *obj)
{
   ELM_ENTRY_CHECK(obj) NULL;
   ELM_ENTRY_DATA_GET(obj, sd);

   return edje_object_part_text_style_user_peek(sd->entry_edje, "elm.text");
}

EAPI void
elm_entry_single_line_set(Evas_Object *obj,
                          Eina_Bool single_line)
{
   ELM_ENTRY_CHECK(obj);
   ELM_ENTRY_DATA_GET(obj, sd);

   if (sd->single_line == single_line) return;

   sd->single_line = single_line;
   sd->line_wrap = ELM_WRAP_NONE;
   if (elm_entry_cnp_mode_get(obj) == ELM_CNP_MODE_MARKUP)
     elm_entry_cnp_mode_set(obj, ELM_CNP_MODE_NO_IMAGE);
   _elm_entry_smart_theme(obj);

   if (sd->scroll)
     {
        if (sd->single_line)
          {
             sd->s_iface->policy_set
                (obj, ELM_SCROLLER_POLICY_OFF, ELM_SCROLLER_POLICY_OFF);
             //***************** TIZEN Only
             sd->s_iface->movement_block_set(obj, ELM_SCROLLER_MOVEMENT_BLOCK_VERTICAL);
             //****************************
             // TIZEN ONLY
             sd->s_iface->bounce_allow_set(obj, EINA_FALSE, EINA_FALSE);
             ///
          }
        else
          {
             sd->s_iface->policy_set(obj, sd->policy_h, sd->policy_v);
             // TIZEN ONLY
             sd->s_iface->bounce_allow_set(obj, EINA_FALSE, EINA_FALSE);
             ///
          }
        elm_layout_sizing_eval(obj);
     }
}

EAPI Eina_Bool
elm_entry_single_line_get(const Evas_Object *obj)
{
   ELM_ENTRY_CHECK(obj) EINA_FALSE;
   ELM_ENTRY_DATA_GET(obj, sd);

   return sd->single_line;
}

EAPI void
elm_entry_password_set(Evas_Object *obj,
                       Eina_Bool password)
{
   ELM_ENTRY_CHECK(obj);
   ELM_ENTRY_DATA_GET(obj, sd);

   password = !!password;

   if (sd->password == password) return;
   sd->password = password;

#ifdef HAVE_ELEMENTARY_X
   elm_drop_target_del(obj, sd->drop_format,
                       _dnd_enter_cb, obj,
                       _dnd_leave_cb, obj,
                       _dnd_pos_cb, obj,
                       _drag_drop_cb, NULL);
   sd->drop_added = EINA_FALSE;
#endif
   if (password)
     {
        sd->single_line = EINA_TRUE;
        sd->line_wrap = ELM_WRAP_NONE;
        elm_entry_input_hint_set(obj, ((sd->input_hints & ~ELM_INPUT_HINT_AUTO_COMPLETE) | ELM_INPUT_HINT_SENSITIVE_DATA));
//        _entry_selection_callbacks_unregister(obj);
     }
   else
     {
#ifdef HAVE_ELEMENTARY_X
        if (sd->drop_enabled)
          {
             sd->drop_format = _get_drop_format(obj);
             elm_drop_target_add(obj, sd->drop_format,
                                 _dnd_enter_cb, obj,
                                 _dnd_leave_cb, obj,
                                 _dnd_pos_cb, obj,
                                 _drag_drop_cb, NULL);
             sd->drop_added = EINA_TRUE;
          }
#endif
        elm_entry_input_hint_set(obj, ((sd->input_hints | ELM_INPUT_HINT_AUTO_COMPLETE) & ~ELM_INPUT_HINT_SENSITIVE_DATA));
        _entry_selection_callbacks_register(obj);
     }

   _elm_entry_smart_theme(obj);
}

EAPI Eina_Bool
elm_entry_password_get(const Evas_Object *obj)
{
   ELM_ENTRY_CHECK(obj) EINA_FALSE;
   ELM_ENTRY_DATA_GET(obj, sd);

   return sd->password;
}

EAPI void
elm_entry_entry_set(Evas_Object *obj,
                    const char *entry)
{
   ELM_ENTRY_CHECK(obj);
   _elm_entry_smart_text_set(obj, NULL, entry);
}

EAPI const char *
elm_entry_entry_get(const Evas_Object *obj)
{
   ELM_ENTRY_CHECK(obj) NULL;
   return _elm_entry_smart_text_get(obj, NULL);
}

EAPI void
elm_entry_entry_append(Evas_Object *obj,
                       const char *entry)
{
   int len = 0;

   ELM_ENTRY_CHECK(obj);
   ELM_ENTRY_DATA_GET(obj, sd);

   if (!entry) entry = "";

   sd->changed = EINA_TRUE;

   len = strlen(entry);
   if (sd->append_text_left)
     {
        char *tmpbuf;

        tmpbuf = realloc(sd->append_text_left, sd->append_text_len + len + 1);
        if (!tmpbuf)
          {
             /* Do something */
             return;
          }
        sd->append_text_left = tmpbuf;
        memcpy(sd->append_text_left + sd->append_text_len, entry, len + 1);
        sd->append_text_len += len;
     }
   else
     {
        /* FIXME: Add chunked appending here (like in entry_set) */
        edje_object_part_text_append(sd->entry_edje, "elm.text", entry);
     }
}

EAPI Eina_Bool
elm_entry_is_empty(const Evas_Object *obj)
{
   /* FIXME: until there's support for that in textblock, we just
    * check to see if the there is text or not. */
   const Evas_Object *tb;
   Evas_Textblock_Cursor *cur;
   Eina_Bool ret;

   ELM_ENTRY_CHECK(obj) EINA_TRUE;
   ELM_ENTRY_DATA_GET(obj, sd);

   /* It's a hack until we get the support suggested above.  We just
    * create a cursor, point it to the begining, and then try to
    * advance it, if it can advance, the tb is not empty, otherwise it
    * is. */
   tb = edje_object_part_object_get(sd->entry_edje, "elm.text");

   /* This is actually, ok for the time being, these hackish stuff
      will be removed once evas 1.0 is out */
   cur = evas_object_textblock_cursor_new((Evas_Object *)tb);
   evas_textblock_cursor_pos_set(cur, 0);
   ret = evas_textblock_cursor_char_next(cur);
   evas_textblock_cursor_free(cur);

   return !ret;
#if 0  // TIZEN ONLY CODES : IF ABOVE CODES HAVE NO PROBLEM, THEN DELETE THESE CODES.
   char *str = elm_entry_markup_to_utf8(elm_entry_entry_get(obj));
   if (!str) return EINA_TRUE;

   ret = (strlen(str) == 0);

   free(str);
   return ret;
#endif
}

EAPI Evas_Object *
elm_entry_textblock_get(Evas_Object *obj)
{
   ELM_ENTRY_CHECK(obj) NULL;
   ELM_ENTRY_DATA_GET(obj, sd);

   return (Evas_Object *)edje_object_part_object_get
            (sd->entry_edje, "elm.text");
}

EAPI void
elm_entry_calc_force(Evas_Object *obj)
{
   ELM_ENTRY_CHECK(obj);
   ELM_ENTRY_DATA_GET(obj, sd);

   edje_object_calc_force(sd->entry_edje);
   sd->changed = EINA_TRUE;
   elm_layout_sizing_eval(obj);
}

EAPI const char *
elm_entry_selection_get(const Evas_Object *obj)
{
   ELM_ENTRY_CHECK(obj) NULL;
   ELM_ENTRY_DATA_GET(obj, sd);

   return edje_object_part_text_selection_get(sd->entry_edje, "elm.text");
}

EAPI void
elm_entry_entry_insert(Evas_Object *obj,
                       const char *entry)
{
   ELM_ENTRY_CHECK(obj);
   ELM_ENTRY_DATA_GET(obj, sd);

   edje_object_part_text_insert(sd->entry_edje, "elm.text", entry);
   // TIZEN ONLY
#ifdef HAVE_ELEMENTARY_X
   if (elm_widget_focus_get(obj))
      ecore_x_selection_secondary_set(elm_win_xwindow_get(obj), NULL,0);
#endif
   ///////////
   sd->changed = EINA_TRUE;
   elm_layout_sizing_eval(obj);
}

EAPI void
elm_entry_line_wrap_set(Evas_Object *obj,
                        Elm_Wrap_Type wrap)
{
   ELM_ENTRY_CHECK(obj);
   ELM_ENTRY_DATA_GET(obj, sd);

   if (sd->line_wrap == wrap) return;
   sd->last_w = -1;
   sd->line_wrap = wrap;
   _elm_entry_smart_theme(obj);
}

EAPI Elm_Wrap_Type
elm_entry_line_wrap_get(const Evas_Object *obj)
{
   ELM_ENTRY_CHECK(obj) EINA_FALSE;
   ELM_ENTRY_DATA_GET(obj, sd);

   return sd->line_wrap;
}

EAPI void
elm_entry_editable_set(Evas_Object *obj,
                       Eina_Bool editable)
{
   ELM_ENTRY_CHECK(obj);
   ELM_ENTRY_DATA_GET(obj, sd);

   if (sd->editable == editable) return;
   sd->editable = editable;
   _elm_entry_smart_theme(obj);

#ifdef HAVE_ELEMENTARY_X
   elm_drop_target_del(obj, sd->drop_format,
                       _dnd_enter_cb, obj,
                       _dnd_leave_cb, obj,
                       _dnd_pos_cb, obj,
                       _drag_drop_cb, NULL);
   sd->drop_added = EINA_FALSE;
   if (editable)
     {
        if (sd->drop_enabled)
          {
             sd->drop_format = _get_drop_format(obj);
             elm_drop_target_add(obj, sd->drop_format,
                                 _dnd_enter_cb, obj,
                                 _dnd_leave_cb, obj,
                                 _dnd_pos_cb, obj,
                                 _drag_drop_cb, NULL);
             sd->drop_added = EINA_TRUE;
          }
     }
#endif
}

EAPI Eina_Bool
elm_entry_editable_get(const Evas_Object *obj)
{
   ELM_ENTRY_CHECK(obj) EINA_FALSE;
   ELM_ENTRY_DATA_GET(obj, sd);

   return sd->editable;
}

EAPI void
elm_entry_select_none(Evas_Object *obj)
{
   ELM_ENTRY_CHECK(obj);
   ELM_ENTRY_DATA_GET(obj, sd);

   if (sd->sel_mode)
     {
        sd->sel_mode = EINA_FALSE;
        if (!_elm_config->desktop_entry)
          edje_object_part_text_select_allow_set
            (sd->entry_edje, "elm.text", EINA_FALSE);
        edje_object_signal_emit(sd->entry_edje, "elm,state,select,off", "elm");
     }
   edje_object_part_text_select_none(sd->entry_edje, "elm.text");
}

EAPI void
elm_entry_select_all(Evas_Object *obj)
{
   ELM_ENTRY_CHECK(obj);
   ELM_ENTRY_DATA_GET(obj, sd);

   if (sd->sel_mode)
     {
        sd->sel_mode = EINA_FALSE;
        if (!_elm_config->desktop_entry)
          edje_object_part_text_select_allow_set
            (sd->entry_edje, "elm.text", EINA_FALSE);
        edje_object_signal_emit(sd->entry_edje, "elm,state,select,off", "elm");
     }
   edje_object_part_text_select_all(sd->entry_edje, "elm.text");
}

EAPI Eina_Bool
elm_entry_cursor_geometry_get(const Evas_Object *obj,
                              Evas_Coord *x,
                              Evas_Coord *y,
                              Evas_Coord *w,
                              Evas_Coord *h)
{
   ELM_ENTRY_CHECK(obj) EINA_FALSE;
   ELM_ENTRY_DATA_GET(obj, sd);

   edje_object_part_text_cursor_geometry_get
     (sd->entry_edje, "elm.text", x, y, w, h);
   return EINA_TRUE;
}

EAPI Eina_Bool
elm_entry_cursor_next(Evas_Object *obj)
{
   ELM_ENTRY_CHECK(obj) EINA_FALSE;
   ELM_ENTRY_DATA_GET(obj, sd);

   return edje_object_part_text_cursor_next
            (sd->entry_edje, "elm.text", EDJE_CURSOR_MAIN);
}

EAPI Eina_Bool
elm_entry_cursor_prev(Evas_Object *obj)
{
   ELM_ENTRY_CHECK(obj) EINA_FALSE;
   ELM_ENTRY_DATA_GET(obj, sd);

   return edje_object_part_text_cursor_prev
            (sd->entry_edje, "elm.text", EDJE_CURSOR_MAIN);
}

EAPI Eina_Bool
elm_entry_cursor_up(Evas_Object *obj)
{
   ELM_ENTRY_CHECK(obj) EINA_FALSE;
   ELM_ENTRY_DATA_GET(obj, sd);

   return edje_object_part_text_cursor_up
            (sd->entry_edje, "elm.text", EDJE_CURSOR_MAIN);
}

EAPI Eina_Bool
elm_entry_cursor_down(Evas_Object *obj)
{
   ELM_ENTRY_CHECK(obj) EINA_FALSE;
   ELM_ENTRY_DATA_GET(obj, sd);

   return edje_object_part_text_cursor_down
            (sd->entry_edje, "elm.text", EDJE_CURSOR_MAIN);
}

EAPI void
elm_entry_cursor_begin_set(Evas_Object *obj)
{
   ELM_ENTRY_CHECK(obj);
   ELM_ENTRY_DATA_GET(obj, sd);

   edje_object_part_text_cursor_begin_set
     (sd->entry_edje, "elm.text", EDJE_CURSOR_MAIN);
}

EAPI void
elm_entry_cursor_end_set(Evas_Object *obj)
{
   ELM_ENTRY_CHECK(obj);
   ELM_ENTRY_DATA_GET(obj, sd);

   edje_object_part_text_cursor_end_set
     (sd->entry_edje, "elm.text", EDJE_CURSOR_MAIN);
}

EAPI void
elm_entry_cursor_line_begin_set(Evas_Object *obj)
{
   ELM_ENTRY_CHECK(obj);
   ELM_ENTRY_DATA_GET(obj, sd);

   edje_object_part_text_cursor_line_begin_set
     (sd->entry_edje, "elm.text", EDJE_CURSOR_MAIN);
}

EAPI void
elm_entry_cursor_line_end_set(Evas_Object *obj)
{
   ELM_ENTRY_CHECK(obj);
   ELM_ENTRY_DATA_GET(obj, sd);

   edje_object_part_text_cursor_line_end_set
     (sd->entry_edje, "elm.text", EDJE_CURSOR_MAIN);
}

EAPI void
elm_entry_cursor_selection_begin(Evas_Object *obj)
{
   ELM_ENTRY_CHECK(obj);
   ELM_ENTRY_DATA_GET(obj, sd);

   edje_object_part_text_select_begin(sd->entry_edje, "elm.text");
}

EAPI void
elm_entry_cursor_selection_end(Evas_Object *obj)
{
   ELM_ENTRY_CHECK(obj);
   ELM_ENTRY_DATA_GET(obj, sd);

   edje_object_part_text_select_extend(sd->entry_edje, "elm.text");
}

EAPI Eina_Bool
elm_entry_cursor_is_format_get(const Evas_Object *obj)
{
   ELM_ENTRY_CHECK(obj) EINA_FALSE;
   ELM_ENTRY_DATA_GET(obj, sd);

   return edje_object_part_text_cursor_is_format_get
            (sd->entry_edje, "elm.text", EDJE_CURSOR_MAIN);
}

EAPI Eina_Bool
elm_entry_cursor_is_visible_format_get(const Evas_Object *obj)
{
   ELM_ENTRY_CHECK(obj) EINA_FALSE;
   ELM_ENTRY_DATA_GET(obj, sd);

   return edje_object_part_text_cursor_is_visible_format_get
            (sd->entry_edje, "elm.text", EDJE_CURSOR_MAIN);
}

EAPI char *
elm_entry_cursor_content_get(const Evas_Object *obj)
{
   ELM_ENTRY_CHECK(obj) NULL;
   ELM_ENTRY_DATA_GET(obj, sd);

   return edje_object_part_text_cursor_content_get
            (sd->entry_edje, "elm.text", EDJE_CURSOR_MAIN);
}

EAPI void
elm_entry_cursor_pos_set(Evas_Object *obj,
                         int pos)
{
   ELM_ENTRY_CHECK(obj);
   ELM_ENTRY_DATA_GET(obj, sd);

   edje_object_part_text_cursor_pos_set
     (sd->entry_edje, "elm.text", EDJE_CURSOR_MAIN, pos);
   edje_object_message_signal_process(sd->entry_edje);
}

EAPI int
elm_entry_cursor_pos_get(const Evas_Object *obj)
{
   ELM_ENTRY_CHECK(obj) 0;
   ELM_ENTRY_DATA_GET(obj, sd);

   return edje_object_part_text_cursor_pos_get
            (sd->entry_edje, "elm.text", EDJE_CURSOR_MAIN);
}

EAPI void
elm_entry_selection_cut(Evas_Object *obj)
{
   ELM_ENTRY_CHECK(obj);
   ELM_ENTRY_DATA_GET(obj, sd);

   if ((sd->password)) return;
   _cut_cb(obj, NULL, NULL);
}

EAPI void
elm_entry_selection_copy(Evas_Object *obj)
{
   ELM_ENTRY_CHECK(obj);
   ELM_ENTRY_DATA_GET(obj, sd);

   if ((sd->password)) return;
   _copy_cb(obj, NULL, NULL);
}

EAPI void
elm_entry_selection_paste(Evas_Object *obj)
{
   ELM_ENTRY_CHECK(obj);
   ELM_ENTRY_DATA_GET(obj, sd);

   _paste_cb(obj, NULL, NULL);
}

EAPI void
elm_entry_context_menu_clear(Evas_Object *obj)
{
   Elm_Entry_Context_Menu_Item *it;

   ELM_ENTRY_CHECK(obj);
   ELM_ENTRY_DATA_GET(obj, sd);

   EINA_LIST_FREE (sd->items, it)
     {
        eina_stringshare_del(it->label);
        eina_stringshare_del(it->icon_file);
        eina_stringshare_del(it->icon_group);
        free(it);
     }
}

EAPI void
elm_entry_context_menu_item_add(Evas_Object *obj,
                                const char *label,
                                const char *icon_file,
                                Elm_Icon_Type icon_type,
                                Evas_Smart_Cb func,
                                const void *data)
{
   Elm_Entry_Context_Menu_Item *it;

   ELM_ENTRY_CHECK(obj);
   ELM_ENTRY_DATA_GET(obj, sd);

   it = calloc(1, sizeof(Elm_Entry_Context_Menu_Item));
   if (!it) return;

   sd->items = eina_list_append(sd->items, it);
   it->obj = obj;
   it->label = eina_stringshare_add(label);
   it->icon_file = eina_stringshare_add(icon_file);
   it->icon_type = icon_type;
   it->func = func;
   it->data = (void *)data;
}

EAPI void
elm_entry_context_menu_disabled_set(Evas_Object *obj,
                                    Eina_Bool disabled)
{
   ELM_ENTRY_CHECK(obj);
   ELM_ENTRY_DATA_GET(obj, sd);

   if (sd->context_menu == !disabled) return;
   sd->context_menu = !disabled;
}

EAPI Eina_Bool
elm_entry_context_menu_disabled_get(const Evas_Object *obj)
{
   ELM_ENTRY_CHECK(obj) EINA_FALSE;
   ELM_ENTRY_DATA_GET(obj, sd);

   return !sd->context_menu;
}

// TIZEN ONLY - START
EAPI void
elm_entry_select_allow_set(Evas_Object *obj,
                           Eina_Bool allow)
{
   ELM_ENTRY_CHECK(obj);
   ELM_ENTRY_DATA_GET(obj, sd);

   if (sd->sel_allow == allow) return;
   sd->sel_allow = allow;

   // TIZEN_ONLY(20130830): For cursor movement when mouse up.
   edje_object_part_text_select_disabled_set(sd->entry_edje, "elm.text", !allow);
   //
}

EAPI Eina_Bool
elm_entry_select_allow_get(const Evas_Object *obj)
{
   ELM_ENTRY_CHECK(obj) EINA_FALSE;
   ELM_ENTRY_DATA_GET(obj, sd);

   return sd->sel_allow;
}

EAPI void
elm_entry_cursor_handler_disabled_set(Evas_Object *obj,
                                      Eina_Bool disabled)
{
   ELM_ENTRY_CHECK(obj);
   ELM_ENTRY_DATA_GET(obj, sd);

   if (sd->cursor_handler_disabled == disabled) return;
   sd->cursor_handler_disabled = disabled;

   if (!_elm_config->desktop_entry)
     edje_object_part_text_cursor_handler_disabled_set(sd->entry_edje, "elm.text", disabled);
}

EAPI Eina_Bool
elm_entry_cursor_handler_disabled_get(const Evas_Object *obj)
{
   ELM_ENTRY_CHECK(obj) EINA_FALSE;
   ELM_ENTRY_DATA_GET(obj, sd);
   return sd->cursor_handler_disabled;
}
// TIZEN ONLY - END

EAPI void
elm_entry_item_provider_append(Evas_Object *obj,
                               Elm_Entry_Item_Provider_Cb func,
                               void *data)
{
   Elm_Entry_Item_Provider *ip;

   ELM_ENTRY_CHECK(obj);
   ELM_ENTRY_DATA_GET(obj, sd);
   EINA_SAFETY_ON_NULL_RETURN(func);

   ip = calloc(1, sizeof(Elm_Entry_Item_Provider));
   if (!ip) return;

   ip->func = func;
   ip->data = data;
   sd->item_providers = eina_list_append(sd->item_providers, ip);
}

EAPI void
elm_entry_item_provider_prepend(Evas_Object *obj,
                                Elm_Entry_Item_Provider_Cb func,
                                void *data)
{
   Elm_Entry_Item_Provider *ip;

   ELM_ENTRY_CHECK(obj);
   ELM_ENTRY_DATA_GET(obj, sd);
   EINA_SAFETY_ON_NULL_RETURN(func);

   ip = calloc(1, sizeof(Elm_Entry_Item_Provider));
   if (!ip) return;

   ip->func = func;
   ip->data = data;
   sd->item_providers = eina_list_prepend(sd->item_providers, ip);
}

EAPI void
elm_entry_item_provider_remove(Evas_Object *obj,
                               Elm_Entry_Item_Provider_Cb func,
                               void *data)
{
   Eina_List *l;
   Elm_Entry_Item_Provider *ip;

   ELM_ENTRY_CHECK(obj);
   ELM_ENTRY_DATA_GET(obj, sd);
   EINA_SAFETY_ON_NULL_RETURN(func);

   EINA_LIST_FOREACH(sd->item_providers, l, ip)
     {
        if ((ip->func == func) && ((!data) || (ip->data == data)))
          {
             sd->item_providers = eina_list_remove_list(sd->item_providers, l);
             free(ip);
             return;
          }
     }
}

EAPI void
elm_entry_markup_filter_append(Evas_Object *obj,
                               Elm_Entry_Filter_Cb func,
                               void *data)
{
   Elm_Entry_Markup_Filter *tf;

   ELM_ENTRY_CHECK(obj);
   ELM_ENTRY_DATA_GET(obj, sd);
   EINA_SAFETY_ON_NULL_RETURN(func);

   tf = _filter_new(func, data);
   if (!tf) return;

   sd->markup_filters = eina_list_append(sd->markup_filters, tf);
}

EAPI void
elm_entry_markup_filter_prepend(Evas_Object *obj,
                                Elm_Entry_Filter_Cb func,
                                void *data)
{
   Elm_Entry_Markup_Filter *tf;

   ELM_ENTRY_CHECK(obj);
   ELM_ENTRY_DATA_GET(obj, sd);
   EINA_SAFETY_ON_NULL_RETURN(func);

   tf = _filter_new(func, data);
   if (!tf) return;

   sd->markup_filters = eina_list_prepend(sd->markup_filters, tf);
}

EAPI void
elm_entry_markup_filter_remove(Evas_Object *obj,
                               Elm_Entry_Filter_Cb func,
                               void *data)
{
   Eina_List *l;
   Elm_Entry_Markup_Filter *tf;

   ELM_ENTRY_CHECK(obj);
   ELM_ENTRY_DATA_GET(obj, sd);
   EINA_SAFETY_ON_NULL_RETURN(func);

   EINA_LIST_FOREACH(sd->markup_filters, l, tf)
     {
        if ((tf->func == func) && ((!data) || (tf->orig_data == data)))
          {
             sd->markup_filters = eina_list_remove_list(sd->markup_filters, l);
             _filter_free(tf);
             return;
          }
     }
}

EAPI char *
elm_entry_markup_to_utf8(const char *s)
{
   char *ss = _elm_util_mkup_to_text(s);
   if (!ss) ss = strdup("");
   return ss;
}

EAPI char *
elm_entry_utf8_to_markup(const char *s)
{
   char *ss = _elm_util_text_to_mkup(s);
   if (!ss) ss = strdup("");
   return ss;
}

static const char *
_text_get(const Evas_Object *obj)
{
   return elm_object_text_get(obj);
}

EAPI void
elm_entry_filter_limit_size(void *data,
                            Evas_Object *entry,
                            char **text)
{
   const char *(*text_get)(const Evas_Object *);
   Elm_Entry_Filter_Limit_Size *lim = data;
   char *current, *utfstr;
   int len, newlen;

   EINA_SAFETY_ON_NULL_RETURN(data);
   EINA_SAFETY_ON_NULL_RETURN(entry);
   EINA_SAFETY_ON_NULL_RETURN(text);

   /* hack. I don't want to copy the entire function to work with
    * scrolled_entry */
   text_get = _text_get;

   current = elm_entry_markup_to_utf8(text_get(entry));
   utfstr = elm_entry_markup_to_utf8(*text);

   if (lim->max_char_count > 0)
     {
        len = evas_string_char_len_get(current);
        newlen = evas_string_char_len_get(utfstr);
        if ((len >= lim->max_char_count) && (newlen > 0))
          {
             if (elm_object_focus_get(entry))
               evas_object_smart_callback_call(entry, SIG_MAX_LENGTH, NULL);
             free(*text);
             *text = NULL;
             free(current);
             free(utfstr);
             return;
          }
        if ((len + newlen) > lim->max_char_count)
          _chars_add_till_limit
            (entry, text, (lim->max_char_count - len), LENGTH_UNIT_CHAR);
     }
   else if (lim->max_byte_count > 0)
     {
        len = strlen(current);
        newlen = strlen(utfstr);
        if ((len >= lim->max_byte_count) && (newlen > 0))
          {
             if (elm_object_focus_get(entry))
               evas_object_smart_callback_call(entry, SIG_MAX_LENGTH, NULL);
             free(*text);
             *text = NULL;
             free(current);
             free(utfstr);
             return;
          }
        if ((len + newlen) > lim->max_byte_count)
          _chars_add_till_limit
            (entry, text, (lim->max_byte_count - len), LENGTH_UNIT_BYTE);
     }

   free(current);
   free(utfstr);
}

EAPI void
elm_entry_filter_accept_set(void *data,
                            Evas_Object *entry,
                            char **text)
{
   int read_idx, last_read_idx = 0, read_char;
   Elm_Entry_Filter_Accept_Set *as = data;
   Eina_Bool goes_in;
   Eina_Bool rejected;
   const char *set;
   char *insert;

   EINA_SAFETY_ON_NULL_RETURN(data);
   EINA_SAFETY_ON_NULL_RETURN(text);

   if ((!as->accepted) && (!as->rejected))
     return;

   if (as->accepted)
     {
        set = as->accepted;
        goes_in = EINA_TRUE;
     }
   else
     {
        set = as->rejected;
        goes_in = EINA_FALSE;
     }

   insert = *text;
   read_idx = evas_string_char_next_get(*text, 0, &read_char);
   rejected = EINA_FALSE;
   while (read_char)
     {
        int cmp_idx, cmp_char;
        Eina_Bool in_set = EINA_FALSE;

        if (read_char == '<')
          {
             while (read_char && (read_char != '>'))
               read_idx = evas_string_char_next_get(*text, read_idx, &read_char);

             if (goes_in) in_set = EINA_TRUE;
             else in_set = EINA_FALSE;
          }
        else
          {
             if (read_char == '&')
               {
                  while (read_char && (read_char != ';'))
                    read_idx = evas_string_char_next_get(*text, read_idx, &read_char);

                  if (!read_char)
                    {
                       if (goes_in) in_set = EINA_TRUE;
                       else in_set = EINA_FALSE;
                       goto inserting;
                    }
                  if (read_char == ';')
                    {
                       char *tag;
                       int utf8 = 0;
                       tag = malloc(read_idx - last_read_idx + 1);
                       if (tag)
                         {
                            char *markup;
                            strncpy(tag, (*text) + last_read_idx, read_idx - last_read_idx);
                            tag[read_idx - last_read_idx] = 0;
                            markup = elm_entry_markup_to_utf8(tag);
                            free(tag);
                            if (markup)
                              {
                                 utf8 = *markup;
                                 free(markup);
                              }
                            if (!utf8)
                              {
                                 in_set = EINA_FALSE;
                                 goto inserting;
                              }
                            read_char = utf8;
                         }
                    }
               }

             cmp_idx = evas_string_char_next_get(set, 0, &cmp_char);
             while (cmp_char)
               {
                  if (read_char == cmp_char)
                    {
                       in_set = EINA_TRUE;
                       break;
                    }
                  cmp_idx = evas_string_char_next_get(set, cmp_idx, &cmp_char);
               }
          }

inserting:

        if (in_set == goes_in)
          {
             int size = read_idx - last_read_idx;
             const char *src = (*text) + last_read_idx;
             if (src != insert)
               memcpy(insert, *text + last_read_idx, size);
             insert += size;
          }
        else
          {
             rejected = EINA_TRUE;
          }

        if (read_char)
          {
             last_read_idx = read_idx;
             read_idx = evas_string_char_next_get(*text, read_idx, &read_char);
          }
     }
   *insert = 0;
   if (rejected)
     evas_object_smart_callback_call(entry, SIG_REJECTED, NULL);
}

EAPI Eina_Bool
elm_entry_file_set(Evas_Object *obj,
                   const char *file,
                   Elm_Text_Format format)
{
   ELM_ENTRY_CHECK(obj) EINA_FALSE;
   ELM_ENTRY_DATA_GET(obj, sd);

   if (sd->delay_write)
     {
        ecore_timer_del(sd->delay_write);
        sd->delay_write = NULL;
     }

   if (sd->auto_save) _save_do(obj);
   eina_stringshare_replace(&sd->file, file);
   sd->format = format;
   return _load_do(obj);
}

EAPI void
elm_entry_file_get(const Evas_Object *obj,
                   const char **file,
                   Elm_Text_Format *format)
{
   ELM_ENTRY_CHECK(obj);
   ELM_ENTRY_DATA_GET(obj, sd);

   if (file) *file = sd->file;
   if (format) *format = sd->format;
}

EAPI void
elm_entry_file_save(Evas_Object *obj)
{
   ELM_ENTRY_CHECK(obj);
   ELM_ENTRY_DATA_GET(obj, sd);

   if (sd->delay_write)
     {
        ecore_timer_del(sd->delay_write);
        sd->delay_write = NULL;
     }
   _save_do(obj);
   sd->delay_write = ecore_timer_add(2.0, _delay_write, obj);
}

EAPI void
elm_entry_autosave_set(Evas_Object *obj,
                       Eina_Bool auto_save)
{
   ELM_ENTRY_CHECK(obj);
   ELM_ENTRY_DATA_GET(obj, sd);

   sd->auto_save = !!auto_save;
}

EAPI Eina_Bool
elm_entry_autosave_get(const Evas_Object *obj)
{
   ELM_ENTRY_CHECK(obj) EINA_FALSE;
   ELM_ENTRY_DATA_GET(obj, sd);

   return sd->auto_save;
}

EINA_DEPRECATED EAPI void
elm_entry_cnp_textonly_set(Evas_Object *obj,
                           Eina_Bool textonly)
{
   Elm_Cnp_Mode cnp_mode = ELM_CNP_MODE_MARKUP;

   ELM_ENTRY_CHECK(obj);

   if (textonly)
     cnp_mode = ELM_CNP_MODE_NO_IMAGE;
   elm_entry_cnp_mode_set(obj, cnp_mode);
}

EINA_DEPRECATED EAPI Eina_Bool
elm_entry_cnp_textonly_get(const Evas_Object *obj)
{
   ELM_ENTRY_CHECK(obj) EINA_FALSE;

   return elm_entry_cnp_mode_get(obj) != ELM_CNP_MODE_MARKUP;
}

EAPI void
elm_entry_cnp_mode_set(Evas_Object *obj,
                       Elm_Cnp_Mode cnp_mode)
{
   Elm_Sel_Format format = ELM_SEL_FORMAT_MARKUP;

   ELM_ENTRY_CHECK(obj);
   ELM_ENTRY_DATA_GET(obj, sd);

   if (sd->cnp_mode == cnp_mode) return;
   sd->cnp_mode = cnp_mode;
   if (sd->cnp_mode == ELM_CNP_MODE_PLAINTEXT)
     format = ELM_SEL_FORMAT_TEXT;
   else if (cnp_mode == ELM_CNP_MODE_MARKUP)
     format |= ELM_SEL_FORMAT_IMAGE;
#ifdef HAVE_ELEMENTARY_X
   if (sd->drop_enabled)
     {
        elm_drop_target_del(obj, sd->drop_format,
                            _dnd_enter_cb, obj,
                            _dnd_leave_cb, obj,
                            _dnd_pos_cb, obj,
                            _drag_drop_cb, NULL);
        sd->drop_format = format;
        elm_drop_target_add(obj, format,
                            _dnd_enter_cb, obj,
                            _dnd_leave_cb, obj,
                            _dnd_pos_cb, obj,
                            _drag_drop_cb, NULL);
        sd->drop_added = EINA_TRUE;
     }
#endif
}

EAPI Elm_Cnp_Mode
elm_entry_cnp_mode_get(const Evas_Object *obj)
{
   ELM_ENTRY_CHECK(obj) ELM_CNP_MODE_MARKUP;
   ELM_ENTRY_DATA_GET(obj, sd);

   return sd->cnp_mode;
}

static void
_elm_entry_content_viewport_resize_cb(Evas_Object *obj,
                                      Evas_Coord w __UNUSED__, Evas_Coord h __UNUSED__)
{
   ELM_ENTRY_DATA_GET(obj, sd);

   if (sd->line_wrap)
     {
        elm_layout_sizing_eval(obj);
     }
   else if (sd->scroll)
     {
        Evas_Coord vw = 0, vh = 0;

        sd->s_iface->content_viewport_size_get(obj, &vw, &vh);
        if (vw < sd->ent_mw) vw = sd->ent_mw;
        if (vh < sd->ent_mh) vh = sd->ent_mh;
        evas_object_resize(sd->entry_edje, vw, vh);
     }

   if (sd->hoversel) _hoversel_position(obj);

   // TIZEN ONLY
   if (!_elm_config->desktop_entry)
     {
        if (sd->region_get_job) ecore_job_del(sd->region_get_job);
        sd->region_get_job = ecore_job_add(_region_get_job, obj);

        if (sd->magnifier_showing)
          _magnifier_move(obj);
     }
   //
}

EAPI void
elm_entry_scrollable_set(Evas_Object *obj,
                         Eina_Bool scroll)
{
   ELM_ENTRY_CHECK(obj);
   ELM_ENTRY_DATA_GET(obj, sd);

   scroll = !!scroll;
   if (sd->scroll == scroll) return;
   sd->scroll = scroll;

   if (sd->scroll)
     {
        /* we now must re-theme ourselves to a scroller decoration
         * and move the entry looking object to be the content of the
         * scrollable view */
        elm_widget_resize_object_set(obj, NULL, EINA_TRUE);
        elm_widget_sub_object_add(obj, sd->entry_edje);

        if (!sd->scr_edje)
          {
             sd->scr_edje = edje_object_add(evas_object_evas_get(obj));

             elm_widget_theme_object_set
               (obj, sd->scr_edje, "scroller", "entry",
               elm_widget_style_get(obj));

             evas_object_size_hint_weight_set
               (sd->scr_edje, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
             evas_object_size_hint_align_set
               (sd->scr_edje, EVAS_HINT_FILL, EVAS_HINT_FILL);

             evas_object_propagate_events_set(sd->scr_edje, EINA_TRUE);
          }

        elm_widget_resize_object_set(obj, sd->scr_edje, EINA_TRUE);

        sd->s_iface->objects_set(obj, sd->scr_edje, sd->hit_rect);

        sd->s_iface->bounce_allow_set(obj, sd->h_bounce, sd->v_bounce);
        if (sd->single_line)
          {
             sd->s_iface->policy_set
                (obj, ELM_SCROLLER_POLICY_OFF, ELM_SCROLLER_POLICY_OFF);
             //***************** TIZEN Only
             sd->s_iface->movement_block_set(obj, ELM_SCROLLER_MOVEMENT_BLOCK_VERTICAL);
             //****************************
          }
        else
          sd->s_iface->policy_set(obj, sd->policy_h, sd->policy_v);
        sd->s_iface->content_set(obj, sd->entry_edje);
        sd->s_iface->content_viewport_resize_cb_set(obj, _elm_entry_content_viewport_resize_cb);
        elm_widget_on_show_region_hook_set(obj, _show_region_hook, obj);
     }
   else
     {
        if (sd->scr_edje)
          {
             sd->s_iface->content_set(obj, NULL);
             evas_object_hide(sd->scr_edje);
          }
        elm_widget_resize_object_set(obj, sd->entry_edje, EINA_TRUE);

        if (sd->scr_edje)
          elm_widget_sub_object_add(obj, sd->scr_edje);

        sd->s_iface->objects_set(obj, sd->entry_edje, sd->hit_rect);

        elm_widget_on_show_region_hook_set(obj, NULL, NULL);
     }
   sd->last_w = -1;
   _elm_entry_smart_theme(obj);
}

EAPI Eina_Bool
elm_entry_scrollable_get(const Evas_Object *obj)
{
   ELM_ENTRY_CHECK(obj) EINA_FALSE;
   ELM_ENTRY_DATA_GET(obj, sd);

   return sd->scroll;
}

EAPI void
elm_entry_icon_visible_set(Evas_Object *obj,
                           Eina_Bool setting)
{
   ELM_ENTRY_CHECK(obj);

   if (!elm_layout_content_get(obj, "elm.swallow.icon")) return;

   if (setting)
     elm_layout_signal_emit(obj, "elm,action,show,icon", "elm");
   else
     elm_layout_signal_emit(obj, "elm,action,hide,icon", "elm");

   elm_layout_sizing_eval(obj);
}

EAPI void
elm_entry_end_visible_set(Evas_Object *obj,
                          Eina_Bool setting)
{
   ELM_ENTRY_CHECK(obj);

   if (!elm_layout_content_get(obj, "elm.swallow.end")) return;

   if (setting)
     elm_layout_signal_emit(obj, "elm,action,show,end", "elm");
   else
     elm_layout_signal_emit(obj, "elm,action,hide,end", "elm");

   elm_layout_sizing_eval(obj);
}

EAPI void
elm_entry_scrollbar_policy_set(Evas_Object *obj,
                               Elm_Scroller_Policy h,
                               Elm_Scroller_Policy v)
{
   ELM_ENTRY_CHECK(obj);
   ELM_ENTRY_DATA_GET(obj, sd);

   sd->policy_h = h;
   sd->policy_v = v;
   sd->s_iface->policy_set(obj, sd->policy_h, sd->policy_v);
}

EAPI void
elm_entry_bounce_set(Evas_Object *obj,
                     Eina_Bool h_bounce,
                     Eina_Bool v_bounce)
{
   ELM_ENTRY_CHECK(obj);
   ELM_ENTRY_DATA_GET(obj, sd);

   sd->h_bounce = h_bounce;
   sd->v_bounce = v_bounce;
   sd->s_iface->bounce_allow_set(obj, h_bounce, v_bounce);
}

EAPI void
elm_entry_bounce_get(const Evas_Object *obj,
                     Eina_Bool *h_bounce,
                     Eina_Bool *v_bounce)
{
   ELM_ENTRY_CHECK(obj);
   ELM_ENTRY_DATA_GET(obj, sd);

   sd->s_iface->bounce_allow_get(obj, h_bounce, v_bounce);
}

EAPI void
elm_entry_input_panel_layout_set(Evas_Object *obj,
                                 Elm_Input_Panel_Layout layout)
{
   ELM_ENTRY_CHECK(obj);
   ELM_ENTRY_DATA_GET(obj, sd);

   sd->input_panel_layout = layout;

   edje_object_part_text_input_panel_layout_set
     (sd->entry_edje, "elm.text", layout);

   if (layout == ELM_INPUT_PANEL_LAYOUT_PASSWORD)
     elm_entry_input_hint_set(obj, ((sd->input_hints & ~ELM_INPUT_HINT_AUTO_COMPLETE) | ELM_INPUT_HINT_SENSITIVE_DATA));
   else if (layout == ELM_INPUT_PANEL_LAYOUT_TERMINAL)
     elm_entry_input_hint_set(obj, (sd->input_hints & ~ELM_INPUT_HINT_AUTO_COMPLETE));
}

EAPI Elm_Input_Panel_Layout
elm_entry_input_panel_layout_get(const Evas_Object *obj)
{
   ELM_ENTRY_CHECK(obj) ELM_INPUT_PANEL_LAYOUT_INVALID;
   ELM_ENTRY_DATA_GET(obj, sd);

   return sd->input_panel_layout;
}

EAPI void
elm_entry_input_panel_layout_variation_set(Evas_Object *obj,
                                           int variation)
{
   ELM_ENTRY_CHECK(obj);
   ELM_ENTRY_DATA_GET(obj, sd);

   sd->input_panel_layout_variation = variation;

   edje_object_part_text_input_panel_layout_variation_set
      (sd->entry_edje, "elm.text", variation);
}

EAPI int
elm_entry_input_panel_layout_variation_get(const Evas_Object *obj)
{
   ELM_ENTRY_CHECK(obj) 0;
   ELM_ENTRY_DATA_GET(obj, sd);

   return sd->input_panel_layout_variation;
}

EAPI void
elm_entry_autocapital_type_set(Evas_Object *obj,
                               Elm_Autocapital_Type autocapital_type)
{
   ELM_ENTRY_CHECK(obj);
   ELM_ENTRY_DATA_GET(obj, sd);

   sd->autocapital_type = autocapital_type;
   edje_object_part_text_autocapital_type_set
     (sd->entry_edje, "elm.text", autocapital_type);
}

EAPI Elm_Autocapital_Type
elm_entry_autocapital_type_get(const Evas_Object *obj)
{
   ELM_ENTRY_CHECK(obj) ELM_AUTOCAPITAL_TYPE_NONE;
   ELM_ENTRY_DATA_GET(obj, sd);

   return sd->autocapital_type;
}

EAPI void
elm_entry_prediction_allow_set(Evas_Object *obj,
                               Eina_Bool prediction)
{
   ELM_ENTRY_CHECK(obj);
   ELM_ENTRY_DATA_GET(obj, sd);

   sd->prediction_allow = prediction;
   edje_object_part_text_prediction_allow_set
     (sd->entry_edje, "elm.text", prediction);
}

EAPI Eina_Bool
elm_entry_prediction_allow_get(const Evas_Object *obj)
{
   ELM_ENTRY_CHECK(obj) EINA_TRUE;
   ELM_ENTRY_DATA_GET(obj, sd);

   return sd->prediction_allow;
}

EAPI void
elm_entry_input_hint_set(Evas_Object *obj,
                         Elm_Input_Hints hints)
{
   ELM_ENTRY_CHECK(obj);
   ELM_ENTRY_DATA_GET(obj, sd);

   sd->input_hints = hints;
   edje_object_part_text_input_hint_set
     (sd->entry_edje, "elm.text", hints);
}

EAPI Elm_Input_Hints
elm_entry_input_hint_get(const Evas_Object *obj)
{
   ELM_ENTRY_CHECK(obj) EINA_TRUE;
   ELM_ENTRY_DATA_GET(obj, sd);

   return sd->input_hints;
}

EAPI void
elm_entry_imf_context_reset(Evas_Object *obj)
{
   ELM_ENTRY_CHECK(obj);
   ELM_ENTRY_DATA_GET(obj, sd);

   edje_object_part_text_imf_context_reset(sd->entry_edje, "elm.text");
}

EAPI void
elm_entry_input_panel_enabled_set(Evas_Object *obj,
                                  Eina_Bool enabled)
{
   ELM_ENTRY_CHECK(obj);
   ELM_ENTRY_DATA_GET(obj, sd);

   sd->input_panel_enable = enabled;
   edje_object_part_text_input_panel_enabled_set
     (sd->entry_edje, "elm.text", enabled);
}

EAPI Eina_Bool
elm_entry_input_panel_enabled_get(const Evas_Object *obj)
{
   ELM_ENTRY_CHECK(obj) EINA_TRUE;
   ELM_ENTRY_DATA_GET(obj, sd);

   return sd->input_panel_enable;
}

EAPI void
elm_entry_input_panel_show(Evas_Object *obj)
{
   ELM_ENTRY_CHECK(obj);
   ELM_ENTRY_DATA_GET(obj, sd);

   edje_object_part_text_input_panel_show(sd->entry_edje, "elm.text");
}

EAPI void
elm_entry_input_panel_hide(Evas_Object *obj)
{
   ELM_ENTRY_CHECK(obj);
   ELM_ENTRY_DATA_GET(obj, sd);

   edje_object_part_text_input_panel_hide(sd->entry_edje, "elm.text");
}

EAPI void
elm_entry_input_panel_language_set(Evas_Object *obj,
                                   Elm_Input_Panel_Lang lang)
{
   ELM_ENTRY_CHECK(obj);
   ELM_ENTRY_DATA_GET(obj, sd);

   sd->input_panel_lang = lang;
   edje_object_part_text_input_panel_language_set
     (sd->entry_edje, "elm.text", lang);
}

EAPI Elm_Input_Panel_Lang
elm_entry_input_panel_language_get(const Evas_Object *obj)
{
   ELM_ENTRY_CHECK(obj) ELM_INPUT_PANEL_LANG_AUTOMATIC;
   ELM_ENTRY_DATA_GET(obj, sd);

   return sd->input_panel_lang;
}

EAPI void
elm_entry_input_panel_imdata_set(Evas_Object *obj,
                                 const void *data,
                                 int len)
{
   ELM_ENTRY_CHECK(obj);
   ELM_ENTRY_DATA_GET(obj, sd);

   if (sd->input_panel_imdata)
     free(sd->input_panel_imdata);

   sd->input_panel_imdata = calloc(1, len);
   sd->input_panel_imdata_len = len;
   memcpy(sd->input_panel_imdata, data, len);

   edje_object_part_text_input_panel_imdata_set
     (sd->entry_edje, "elm.text", sd->input_panel_imdata,
     sd->input_panel_imdata_len);
}

EAPI void
elm_entry_input_panel_imdata_get(const Evas_Object *obj,
                                 void *data,
                                 int *len)
{
   ELM_ENTRY_CHECK(obj);
   ELM_ENTRY_DATA_GET(obj, sd);

   edje_object_part_text_input_panel_imdata_get
     (sd->entry_edje, "elm.text", data, len);
}

EAPI void
elm_entry_input_panel_return_key_type_set(Evas_Object *obj,
                                          Elm_Input_Panel_Return_Key_Type
                                          return_key_type)
{
   ELM_ENTRY_CHECK(obj);
   ELM_ENTRY_DATA_GET(obj, sd);

   sd->input_panel_return_key_type = return_key_type;

   edje_object_part_text_input_panel_return_key_type_set
     (sd->entry_edje, "elm.text", return_key_type);
}

EAPI Elm_Input_Panel_Return_Key_Type
elm_entry_input_panel_return_key_type_get(const Evas_Object *obj)
{
   ELM_ENTRY_CHECK(obj) ELM_INPUT_PANEL_RETURN_KEY_TYPE_DEFAULT;
   ELM_ENTRY_DATA_GET(obj, sd);

   return sd->input_panel_return_key_type;
}

EAPI void
elm_entry_input_panel_return_key_disabled_set(Evas_Object *obj,
                                              Eina_Bool disabled)
{
   ELM_ENTRY_CHECK(obj);
   ELM_ENTRY_DATA_GET(obj, sd);

   sd->input_panel_return_key_disabled = disabled;

   edje_object_part_text_input_panel_return_key_disabled_set
     (sd->entry_edje, "elm.text", disabled);
}

EAPI Eina_Bool
elm_entry_input_panel_return_key_disabled_get(const Evas_Object *obj)
{
   ELM_ENTRY_CHECK(obj) EINA_FALSE;
   ELM_ENTRY_DATA_GET(obj, sd);

   return sd->input_panel_return_key_disabled;
}

EAPI void
elm_entry_input_panel_return_key_autoenabled_set(Evas_Object *obj,
                                                 Eina_Bool enabled)
{
   ELM_ENTRY_CHECK(obj);
   ELM_ENTRY_DATA_GET(obj, sd);

   sd->auto_return_key = enabled;
   _return_key_enabled_check(obj);
}

EAPI void
elm_entry_input_panel_show_on_demand_set(Evas_Object *obj,
                                         Eina_Bool ondemand)
{
   ELM_ENTRY_CHECK(obj);
   ELM_ENTRY_DATA_GET(obj, sd);

   sd->input_panel_show_on_demand = ondemand;

   edje_object_part_text_input_panel_show_on_demand_set
     (sd->entry_edje, "elm.text", ondemand);
}

EAPI Eina_Bool
elm_entry_input_panel_show_on_demand_get(const Evas_Object *obj)
{
   ELM_ENTRY_CHECK(obj) EINA_FALSE;
   ELM_ENTRY_DATA_GET(obj, sd);

   return sd->input_panel_show_on_demand;
}

EAPI void *
elm_entry_imf_context_get(Evas_Object *obj)
{
   ELM_ENTRY_CHECK(obj) NULL;
   ELM_ENTRY_DATA_GET(obj, sd);
   if (!sd) return NULL;

   return edje_object_part_text_imf_context_get(sd->entry_edje, "elm.text");
}

/* START - ANCHOR HOVER */
static void
_anchor_parent_del_cb(void *data,
                      Evas *e __UNUSED__,
                      Evas_Object *obj __UNUSED__,
                      void *event_info __UNUSED__)
{
   ELM_ENTRY_DATA_GET(data, sd);

   sd->anchor_hover.hover_parent = NULL;
}

EAPI void
elm_entry_anchor_hover_parent_set(Evas_Object *obj,
                                  Evas_Object *parent)
{
   ELM_ENTRY_CHECK(obj);
   ELM_ENTRY_DATA_GET(obj, sd);

   if (sd->anchor_hover.hover_parent)
     evas_object_event_callback_del_full
       (sd->anchor_hover.hover_parent, EVAS_CALLBACK_DEL,
       _anchor_parent_del_cb, obj);
   sd->anchor_hover.hover_parent = parent;
   if (sd->anchor_hover.hover_parent)
     evas_object_event_callback_add
       (sd->anchor_hover.hover_parent, EVAS_CALLBACK_DEL,
       _anchor_parent_del_cb, obj);
}

EAPI Evas_Object *
elm_entry_anchor_hover_parent_get(const Evas_Object *obj)
{
   ELM_ENTRY_CHECK(obj) NULL;
   ELM_ENTRY_DATA_GET(obj, sd);

   return sd->anchor_hover.hover_parent;
}

EAPI void
elm_entry_anchor_hover_style_set(Evas_Object *obj,
                                 const char *style)
{
   ELM_ENTRY_CHECK(obj);
   ELM_ENTRY_DATA_GET(obj, sd);

   eina_stringshare_replace(&sd->anchor_hover.hover_style, style);
}

EAPI const char *
elm_entry_anchor_hover_style_get(const Evas_Object *obj)
{
   ELM_ENTRY_CHECK(obj) NULL;
   ELM_ENTRY_DATA_GET(obj, sd);

   return sd->anchor_hover.hover_style;
}

EAPI void
elm_entry_anchor_hover_end(Evas_Object *obj)
{
   ELM_ENTRY_CHECK(obj);
   ELM_ENTRY_DATA_GET(obj, sd);

   if (sd->anchor_hover.hover) evas_object_del(sd->anchor_hover.hover);
   if (sd->anchor_hover.pop) evas_object_del(sd->anchor_hover.pop);
   sd->anchor_hover.hover = NULL;
   sd->anchor_hover.pop = NULL;
}

/* END - ANCHOR HOVER */

////////////// TIZEN ONLY APIs
EAPI void
elm_entry_magnifier_disabled_set(Evas_Object *obj, Eina_Bool disabled)
{
   ELM_ENTRY_CHECK(obj);
   ELM_ENTRY_DATA_GET(obj, sd);

   if (sd->magnifier_enabled == !disabled) return;
   sd->magnifier_enabled = !disabled;
}

EAPI Eina_Bool
elm_entry_magnifier_disabled_get(const Evas_Object *obj)
{
   ELM_ENTRY_CHECK(obj) EINA_FALSE;
   ELM_ENTRY_DATA_GET(obj, sd);

   return !sd->magnifier_enabled;
}
////////////////////////////////

////////////// REDWOOD ONLY APIs
EAPI void
elm_entry_drag_disabled_set(Evas_Object *obj, Eina_Bool disabled)
{
   ELM_ENTRY_CHECK(obj);
   ELM_ENTRY_DATA_GET(obj, sd);

   if (sd->drag_enabled == !disabled) return;
   sd->drag_enabled = !disabled;
}

EAPI Eina_Bool
elm_entry_drag_disabled_get(const Evas_Object *obj)
{
   ELM_ENTRY_CHECK(obj) EINA_FALSE;
   ELM_ENTRY_DATA_GET(obj, sd);

   return !sd->drag_enabled;
}

EAPI void
elm_entry_drop_disabled_set(Evas_Object *obj, Eina_Bool disabled)
{
   ELM_ENTRY_CHECK(obj);
   ELM_ENTRY_DATA_GET(obj, sd);

   if (sd->drop_enabled == !disabled) return;
   sd->drop_enabled = !disabled;
   elm_drop_target_del(obj, sd->drop_format,
                       _dnd_enter_cb, obj,
                       _dnd_leave_cb, obj,
                       _dnd_pos_cb, obj,
                       _drag_drop_cb, NULL);
   if (!disabled && sd->drop_added)
     {
        sd->drop_format = _get_drop_format(obj);
        elm_drop_target_add(obj, sd->drop_format,
                            _dnd_enter_cb, obj,
                            _dnd_leave_cb, obj,
                            _dnd_pos_cb, obj,
                            _drag_drop_cb, NULL);
     }
}

EAPI Eina_Bool
elm_entry_drop_disabled_get(Evas_Object *obj)
{
   ELM_ENTRY_CHECK(obj) EINA_FALSE;
   ELM_ENTRY_DATA_GET(obj, sd);

   return !sd->drop_enabled;
}
////////////////////////////////

/////////////////////////////////////////////////////////////////////
// TIZEN_ONLY(20140625): Add elm_entry_anchor_access_provider_* APIs.
/////////////////////////////////////////////////////////////////////
EAPI void
elm_entry_anchor_access_provider_append(Evas_Object *obj,
                                        Elm_Entry_Anchor_Access_Provider_Cb func,
                                        void *data)
{
   Elm_Entry_Anchor_Access_Provider *ap;

   ELM_ENTRY_CHECK(obj);
   ELM_ENTRY_DATA_GET(obj, sd);
   EINA_SAFETY_ON_NULL_RETURN(func);

   ap = calloc(1, sizeof(Elm_Entry_Anchor_Access_Provider));
   if (!ap) return;

   ap->func = func;
   ap->data = data;
   sd->anchor_access_providers = eina_list_append(sd->anchor_access_providers, ap);
}

EAPI void
elm_entry_anchor_access_provider_prepend(Evas_Object *obj,
                                         Elm_Entry_Anchor_Access_Provider_Cb func,
                                         void *data)
{
   Elm_Entry_Anchor_Access_Provider *ap;

   ELM_ENTRY_CHECK(obj);
   ELM_ENTRY_DATA_GET(obj, sd);
   EINA_SAFETY_ON_NULL_RETURN(func);

   ap = calloc(1, sizeof(Elm_Entry_Anchor_Access_Provider));
   if (!ap) return;

   ap->func = func;
   ap->data = data;
   sd->anchor_access_providers = eina_list_prepend(sd->anchor_access_providers, ap);
}

EAPI void
elm_entry_anchor_access_provider_remove(Evas_Object *obj,
                                        Elm_Entry_Anchor_Access_Provider_Cb func,
                                        void *data)
{
   Eina_List *l;
   Elm_Entry_Anchor_Access_Provider *ap;

   ELM_ENTRY_CHECK(obj);
   ELM_ENTRY_DATA_GET(obj, sd);
   EINA_SAFETY_ON_NULL_RETURN(func);

   EINA_LIST_FOREACH(sd->anchor_access_providers, l, ap)
     {
        if ((ap->func == func) && ((!data) || (ap->data == data)))
          {
             sd->anchor_access_providers = eina_list_remove_list(sd->anchor_access_providers, l);
             free(ap);
             return;
          }
     }
}

EAPI void
elm_entry_select_region_set(Evas_Object *obj, int start, int end)
{
   ELM_ENTRY_CHECK(obj);
   ELM_ENTRY_DATA_GET(obj, sd);

   if (sd->sel_mode)
     {
        sd->sel_mode = EINA_FALSE;
        if (!_elm_config->desktop_entry)
          edje_object_part_text_select_allow_set
            (sd->entry_edje, "elm.text", EINA_FALSE);
        edje_object_signal_emit(sd->entry_edje, "elm,state,select,off", "elm");
     }

   edje_object_part_text_cursor_pos_set(sd->entry_edje, "elm.text", EDJE_CURSOR_MAIN, start);
   edje_object_part_text_select_begin(sd->entry_edje, "elm.text");
   edje_object_part_text_cursor_pos_set(sd->entry_edje, "elm.text", EDJE_CURSOR_MAIN, end);
   edje_object_part_text_select_extend(sd->entry_edje, "elm.text");
}

/////////////////////////////////////////////////////////////////////
