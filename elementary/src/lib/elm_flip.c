#include <Elementary.h>
#include "elm_priv.h"
#include "elm_widget_flip.h"

EAPI const char ELM_FLIP_SMART_NAME[] = "elm_flip";

static const char SIG_ANIMATE_BEGIN[] = "animate,begin";
static const char SIG_ANIMATE_DONE[] = "animate,done";
static const Evas_Smart_Cb_Description _smart_callbacks[] = {
   {SIG_ANIMATE_BEGIN, ""},
   {SIG_ANIMATE_DONE, ""},
   {NULL, NULL}
};

static Eina_Bool _flip(Evas_Object *obj);

EVAS_SMART_SUBCLASS_NEW
  (ELM_FLIP_SMART_NAME, _elm_flip, Elm_Flip_Smart_Class,
  Elm_Container_Smart_Class, elm_container_smart_class_get, _smart_callbacks);

static void
_slice_free(Slice *sl)
{
   evas_object_del(sl->obj);
   free(sl);
}

static void
_state_slices_clear(Elm_Flip_Smart_Data *sd)
{
   int i, j, num;

   if (sd->slices)
     {
        num = 0;
        for (j = 0; j < sd->slices_h; j++)
          {
             for (i = 0; i < sd->slices_w; i++)
               {
                  if (sd->slices[num]) _slice_free(sd->slices[num]);
                  if (sd->slices2[num]) _slice_free(sd->slices2[num]);
                  num++;
               }
          }

        free(sd->slices);
        free(sd->slices2);
        sd->slices = NULL;
        sd->slices2 = NULL;
     }

   sd->slices_w = 0;
   sd->slices_h = 0;
}

static void
_sizing_eval(Evas_Object *obj)
{
   Evas_Coord minw = -1, minh = -1, minw2 = -1, minh2 = -1;
   Evas_Coord maxw = -1, maxh = -1, maxw2 = -1, maxh2 = -1;
   int fingx = 0, fingy = 0;

   ELM_FLIP_DATA_GET(obj, sd);

   if (sd->front.content)
     evas_object_size_hint_min_get(sd->front.content, &minw, &minh);
   if (sd->back.content)
     evas_object_size_hint_min_get(sd->back.content, &minw2, &minh2);
   if (sd->front.content)
     evas_object_size_hint_max_get(sd->front.content, &maxw, &maxh);
   if (sd->back.content)
     evas_object_size_hint_max_get(sd->back.content, &maxw2, &maxh2);

   if (minw2 > minw) minw = minw2;
   if (minh2 > minh) minh = minh2;
   if ((maxw2 >= 0) && (maxw2 < maxw)) maxw = maxw2;
   if ((maxh2 >= 0) && (maxh2 < maxh)) maxh = maxh2;

   if (sd->dir_enabled[0]) fingy++;
   if (sd->dir_enabled[1]) fingy++;
   if (sd->dir_enabled[2]) fingx++;
   if (sd->dir_enabled[3]) fingx++;

   elm_coords_finger_size_adjust(fingx, &minw, fingy, &minh);

   evas_object_size_hint_min_set(obj, minw, minh);
   evas_object_size_hint_max_set(obj, maxw, maxh);
}

static Eina_Bool
_elm_flip_smart_theme(Evas_Object *obj)
{
   if (!ELM_WIDGET_CLASS(_elm_flip_parent_sc)->theme(obj)) return EINA_FALSE;

   _sizing_eval(obj);

   return EINA_TRUE;
}

static Eina_Bool
_elm_flip_smart_focus_next(const Evas_Object *obj,
                           Elm_Focus_Direction dir,
                           Evas_Object **next)
{
   ELM_FLIP_DATA_GET(obj, sd);

   /* attempt to cycle focus on in sub-items */
   if (sd->state)
     return elm_widget_focus_next_get(sd->front.content, dir, next);
   else
     return elm_widget_focus_next_get(sd->back.content, dir, next);
}

static void
_changed_size_hints_cb(void *data,
                       Evas *e __UNUSED__,
                       Evas_Object *obj __UNUSED__,
                       void *event_info __UNUSED__)
{
   _sizing_eval(data);
}

static Eina_Bool
_elm_flip_smart_sub_object_add(Evas_Object *obj,
                               Evas_Object *sobj)
{
   if (evas_object_data_get(sobj, "elm-parent") == obj)
     return EINA_TRUE;

   if (!ELM_WIDGET_CLASS(_elm_flip_parent_sc)->sub_object_add(obj, sobj))
     return EINA_FALSE;

   evas_object_data_set(sobj, "_elm_leaveme", sobj);
   evas_object_smart_member_add(sobj, obj);
   evas_object_event_callback_add
     (sobj, EVAS_CALLBACK_CHANGED_SIZE_HINTS, _changed_size_hints_cb, obj);

   return EINA_TRUE;
}

static Eina_Bool
_elm_flip_smart_sub_object_del(Evas_Object *obj,
                               Evas_Object *sobj)
{
   ELM_FLIP_DATA_GET(obj, sd);

   if (!ELM_WIDGET_CLASS(_elm_flip_parent_sc)->sub_object_del(obj, sobj))
     return EINA_FALSE;

   if (sobj == sd->front.content)
     {
        evas_object_data_del(sobj, "_elm_leaveme");
        sd->front.content = NULL;
        evas_object_hide(sd->front.clip);
     }
   else if (sobj == sd->back.content)
     {
        evas_object_data_del(sobj, "_elm_leaveme");
        sd->back.content = NULL;
        evas_object_hide(sd->back.clip);
     }

   evas_object_smart_member_del(sobj);
   evas_object_clip_unset(sobj);

   evas_object_event_callback_del_full
     (sobj, EVAS_CALLBACK_CHANGED_SIZE_HINTS, _changed_size_hints_cb, obj);
   _sizing_eval(obj);

   return EINA_TRUE;
}

static Slice *
_slice_new(Elm_Flip_Smart_Data *sd,
           Evas_Object *obj)
{
   Slice *sl;

   sl = calloc(1, sizeof(Slice));
   if (!sl) return NULL;

   sl->obj = evas_object_image_add(evas_object_evas_get(obj));

   evas_object_smart_member_add(sl->obj, ELM_WIDGET_DATA(sd)->obj);

   evas_object_image_smooth_scale_set(sl->obj, EINA_FALSE);
   evas_object_pass_events_set(sl->obj, EINA_TRUE);
   evas_object_image_source_set(sl->obj, obj);

   return sl;
}

static void
_slice_apply(Elm_Flip_Smart_Data *sd,
             Slice *sl,
             Evas_Coord x __UNUSED__,
             Evas_Coord y __UNUSED__,
             Evas_Coord w,
             Evas_Coord h __UNUSED__,
             Evas_Coord ox,
             Evas_Coord oy,
             Evas_Coord ow,
             Evas_Coord oh)
{
   static Evas_Map *m = NULL;
   int i;

   if (!m) m = evas_map_new(4);
   if (!m) return;

   evas_map_smooth_set(m, EINA_FALSE);
   for (i = 0; i < 4; i++)
     {
        evas_map_point_color_set(m, i, 255, 255, 255, 255);
        if (sd->dir == 0)
          {
             int p[4] = { 0, 1, 2, 3 };
             evas_map_point_coord_set(m, i, ox + sl->x[p[i]], oy + sl->y[p[i]],
                                      sl->z[p[i]]);
             evas_map_point_image_uv_set(m, i, sl->u[p[i]], sl->v[p[i]]);
          }
        else if (sd->dir == 1)
          {
             int p[4] = { 1, 0, 3, 2 };
             evas_map_point_coord_set(m, i, ox + (w - sl->x[p[i]]),
                                      oy + sl->y[p[i]], sl->z[p[i]]);
             evas_map_point_image_uv_set(m, i, ow - sl->u[p[i]], sl->v[p[i]]);
          }
        else if (sd->dir == 2)
          {
             int p[4] = { 1, 0, 3, 2 };
             evas_map_point_coord_set(m, i, ox + sl->y[p[i]], oy + sl->x[p[i]],
                                      sl->z[p[i]]);
             evas_map_point_image_uv_set(m, i, sl->v[p[i]], sl->u[p[i]]);
          }
        else /* if (sd->dir == 3) will be this anyway */
          {
             int p[4] = { 0, 1, 2, 3 };
             evas_map_point_coord_set(m, i, ox + sl->y[p[i]],
                                      oy + (w - sl->x[p[i]]), sl->z[p[i]]);
             evas_map_point_image_uv_set(m, i, sl->v[p[i]], oh - sl->u[p[i]]);
          }
     }

   evas_object_map_enable_set(sl->obj, EINA_TRUE);
   evas_object_image_fill_set(sl->obj, 0, 0, ow, oh);
   evas_object_map_set(sl->obj, m);
}

static void
_slice_3d(Elm_Flip_Smart_Data *sd __UNUSED__,
          Slice *sl,
          Evas_Coord x,
          Evas_Coord y,
          Evas_Coord w,
          Evas_Coord h)
{
   Evas_Map *m = (Evas_Map *)evas_object_map_get(sl->obj);
   int i;

   if (!m) return;

   // vanishing point is center of page, and focal dist is 1024
   evas_map_util_3d_perspective(m, x + (w / 2), y + (h / 2), 0, 1024);
   for (i = 0; i < 4; i++)
     {
        Evas_Coord xx, yy;
        evas_map_point_coord_get(m, i, &xx, &yy, NULL);
        evas_map_point_coord_set(m, i, xx, yy, 0);
     }

   if (evas_map_util_clockwise_get(m)) evas_object_show(sl->obj);
   else evas_object_hide(sl->obj);

   evas_object_map_set(sl->obj, m);
}

static void
_slice_light(Elm_Flip_Smart_Data *sd __UNUSED__,
             Slice *sl,
             Evas_Coord x,
             Evas_Coord y,
             Evas_Coord w,
             Evas_Coord h)
{
   Evas_Map *m = (Evas_Map *)evas_object_map_get(sl->obj);
   int i;

   if (!m) return;

   evas_map_util_3d_lighting(m,
     /* light position
      * (centered over page 10 * h toward camera) */
                             x + (w / 2), y + (h / 2), -10000,
                             255, 255, 255, // light color
                             0, 0, 0);   // ambient minimum

   // multiply brightness by 1.2 to make lightish bits all white so we dont
   // add shading where we could otherwise be pure white
   for (i = 0; i < 4; i++)
     {
        int r, g, b, a;

        evas_map_point_color_get(m, i, &r, &g, &b, &a);
        r = (double)r * 1.2; if (r > 255) r = 255;
        g = (double)g * 1.2; if (g > 255) g = 255;
        b = (double)b * 1.2; if (b > 255) b = 255;
        evas_map_point_color_set(m, i, r, g, b, a);
     }

   evas_object_map_set(sl->obj, m);
}

static void
_slice_xyz(Elm_Flip_Smart_Data *sd __UNUSED__,
           Slice *sl,
           double xx1,
           double yy1,
           double zz1,
           double xx2,
           double yy2,
           double zz2,
           double xx3,
           double yy3,
           double zz3,
           double xx4,
           double yy4,
           double zz4)
{
   sl->x[0] = xx1; sl->y[0] = yy1; sl->z[0] = zz1;
   sl->x[1] = xx2; sl->y[1] = yy2; sl->z[1] = zz2;
   sl->x[2] = xx3; sl->y[2] = yy3; sl->z[2] = zz3;
   sl->x[3] = xx4; sl->y[3] = yy4; sl->z[3] = zz4;
}

static void
_slice_uv(Elm_Flip_Smart_Data *sd __UNUSED__,
          Slice *sl,
          double u1,
          double v1,
          double u2,
          double v2,
          double u3,
          double v3,
          double u4,
          double v4)
{
   sl->u[0] = u1; sl->v[0] = v1;
   sl->u[1] = u2; sl->v[1] = v2;
   sl->u[2] = u3; sl->v[2] = v3;
   sl->u[3] = u4; sl->v[3] = v4;
}

static void
_deform_point(Vertex2 *vi,
              Vertex3 *vo,
              double rho,
              double theta,
              double A)
{
   // ^Y
   // |
   // |    X
   // +---->
   // theta == cone angle (0 -> PI/2)
   // A     == distance of cone apex from origin
   // rho   == angle of cone from vertical axis (...-PI/2 to PI/2...)
   Vertex3 v1;
   double d, r, b;

   d = sqrt((vi->x * vi->x) + pow(vi->y - A, 2));
   r = d * sin(theta);
   b = asin(vi->x / d) / sin(theta);

   v1.x = r * sin(b);
   v1.y = d + A - (r * (1 - cos(b)) * sin(theta));
   v1.z = r * (1 - cos(b)) * cos(theta);

   vo->x = (v1.x * cos(rho)) - (v1.z * sin(rho));
   vo->y = v1.y;
   vo->z = (v1.x * sin(rho)) + (v1.z * cos(rho));
}

static void
_interp_point(Vertex3 *vi1,
              Vertex3 *vi2,
              Vertex3 *vo,
              double v)
{
   vo->x = (v * vi2->x) + ((1.0 - v) * vi1->x);
   vo->y = (v * vi2->y) + ((1.0 - v) * vi1->y);
   vo->z = (v * vi2->z) + ((1.0 - v) * vi1->z);
}

static int
_slice_obj_color_sum(Slice *s,
                     int p,
                     int *r,
                     int *g,
                     int *b,
                     int *a)
{
   Evas_Map *m;
   int rr = 0, gg = 0, bb = 0, aa = 0;

   if (!s) return 0;

   m = (Evas_Map *)evas_object_map_get(s->obj);
   if (!m) return 0;

   evas_map_point_color_get(m, p, &rr, &gg, &bb, &aa);
   *r += rr; *g += gg; *b += bb; *a += aa;

   return 1;
}

static void
_slice_obj_color_set(Slice *s,
                     int p,
                     int r,
                     int g,
                     int b,
                     int a)
{
   Evas_Map *m;

   if (!s) return;

   m = (Evas_Map *)evas_object_map_get(s->obj);
   if (!m) return;

   evas_map_point_color_set(m, p, r, g, b, a);
   evas_object_map_set(s->obj, m);
}

static void
_slice_obj_vert_color_merge(Slice *s1,
                            int p1,
                            Slice *s2,
                            int p2,
                            Slice *s3,
                            int p3,
                            Slice *s4,
                            int p4)
{
   int r = 0, g = 0, b = 0, a = 0, n = 0;

   n += _slice_obj_color_sum(s1, p1, &r, &g, &b, &a);
   n += _slice_obj_color_sum(s2, p2, &r, &g, &b, &a);
   n += _slice_obj_color_sum(s3, p3, &r, &g, &b, &a);
   n += _slice_obj_color_sum(s4, p4, &r, &g, &b, &a);

   if (n < 1) return;
   r /= n; g /= n; b /= n; a /= n;

   _slice_obj_color_set(s1, p1, r, g, b, a);
   _slice_obj_color_set(s2, p2, r, g, b, a);
   _slice_obj_color_set(s3, p3, r, g, b, a);
   _slice_obj_color_set(s4, p4, r, g, b, a);
}

static int
_state_update(Elm_Flip_Smart_Data *sd)
{
   Slice *sl;
   Vertex3 *tvo, *tvol;
   Evas_Object *front, *back;
   int i, j, num, nn, jump, num2;
   double b, minv = 0.0, minva, mgrad;
   Evas_Coord xx1, yy1, xx2, yy2, mx, my;
   Evas_Coord x, y, w, h, ox, oy, ow, oh;
   int gx, gy, gszw, gszh, gw, gh, col, row, nw, nh;
   double rho, A, theta, perc, percm, n, rhol, Al, thetal;

   sd->backflip = EINA_TRUE;
   if (sd->state)
     {
        front = sd->front.content;
        back = sd->front.content;
     }
   else
     {
        front = sd->back.content;
        back = sd->back.content;
     }

   evas_object_geometry_get(ELM_WIDGET_DATA(sd)->obj, &x, &y, &w, &h);
   ox = x; oy = y; ow = w; oh = h;
   xx1 = sd->down_x;
   yy1 = sd->down_y;
   xx2 = sd->x;
   yy2 = sd->y;

   if (sd->dir == 0)
     {
        // no nothing. left drag is standard
     }
   else if (sd->dir == 1)
     {
        xx1 = (w - 1) - xx1;
        xx2 = (w - 1) - xx2;
     }
   else if (sd->dir == 2)
     {
        Evas_Coord tmp;

        tmp = xx1; xx1 = yy1; yy1 = tmp;
        tmp = xx2; xx2 = yy2; yy2 = tmp;
        tmp = w; w = h; h = tmp;
     }
   else /* if (sd->dir == 3) will be this anyway */
     {
        Evas_Coord tmp;

        tmp = xx1; xx1 = yy1; yy1 = tmp;
        tmp = xx2; xx2 = yy2; yy2 = tmp;
        tmp = w; w = h; h = tmp;
        xx1 = (w - 1) - xx1;
        xx2 = (w - 1) - xx2;
     }

   if (xx2 >= xx1) xx2 = xx1 - 1;
   mx = (xx1 + xx2) / 2;
   my = (yy1 + yy2) / 2;

   if (mx < 0) mx = 0;
   else if (mx >= w)
     mx = w - 1;
   if (my < 0) my = 0;
   else if (my >= h)
     my = h - 1;

   mgrad = (double)(yy1 - yy2) / (double)(xx1 - xx2);

   if (mx < 1) mx = 1;  // quick hack to keep curl line visible

   if (mgrad == 0.0) // special horizontal case
     mgrad = 0.001;  // quick dirty hack for now
   // else
   {
      minv = 1.0 / mgrad;
      // y = (m * x) + b
      b = my + (minv * mx);
   }
   if ((b >= -5) && (b <= (h + 5)))
     {
        if (minv > 0.0) // clamp to h
          {
             minv = (double)(h + 5 - my) / (double)(mx);
             b = my + (minv * mx);
          }
        else // clamp to 0
          {
             minv = (double)(-5 - my) / (double)(mx);
             b = my + (minv * mx);
          }
     }

   perc = (double)xx2 / (double)xx1;
   percm = (double)mx / (double)xx1;
   if (perc < 0.0) perc = 0.0;
   else if (perc > 1.0)
     perc = 1.0;
   if (percm < 0.0) percm = 0.0;
   else if (percm > 1.0)
     percm = 1.0;

   minva = atan(minv) / (M_PI / 2);
   if (minva < 0.0) minva = -minva;

   // A = apex of cone
   if (b <= 0) A = b;
   else A = h - b;
   if (A < -(h * 20)) A = -h * 20;
   //--//
   Al = -5;

   // rho = is how much the page is turned
   n = 1.0 - perc;
   n = 1.0 - cos(n * M_PI / 2.0);
   n = n * n;
   rho = -(n * M_PI);
   //--//
   rhol = -(n * M_PI);

   // theta == curliness (how much page culrs in on itself
   n = sin((1.0 - perc) * M_PI);
   n = n * 1.2;
   theta = 7.86 + n;
   //--//
   n = sin((1.0 - perc) * M_PI);
   n = 1.0 - n;
   n = n * n;
   n = 1.0 - n;
   thetal = 7.86 + n;

   nw = 16;
   nh = 16;
   gszw = w / nw;
   gszh = h / nh;
   if (gszw < 4) gszw = 4;
   if (gszh < 4) gszh = 4;

   nw = (w + gszw - 1) / gszw;
   nh = (h + gszh - 1) / gszh;
   if ((sd->slices_w != nw) || (sd->slices_h != nh)) _state_slices_clear(sd);
   sd->slices_w = nw;
   sd->slices_h = nh;
   if (!sd->slices)
     {
        sd->slices = calloc(sd->slices_w * sd->slices_h, sizeof(Slice *));
        if (!sd->slices) return 0;
        sd->slices2 = calloc(sd->slices_w * sd->slices_h, sizeof(Slice *));
        if (!sd->slices2)
          {
             free(sd->slices);
             sd->slices = NULL;
             return 0;
          }
     }

   num = (sd->slices_w + 1) * (sd->slices_h + 1);

   tvo = alloca(sizeof(Vertex3) * num);
   tvol = alloca(sizeof(Vertex3) * (sd->slices_w + 1));

   for (col = 0, gx = 0; gx <= (w + gszw - 1); gx += gszw, col++)
     {
        Vertex2 vil;

        vil.x = gx;
        vil.y = h - ((gx * h) / (w + gszw - 1));
        _deform_point(&vil, &(tvol[col]), rhol, thetal, Al);
     }

   n = minva * sin(perc * M_PI);
   n = n * n;

   num = 0;
   for (col = 0, gx = 0; gx <= (w + gszw - 1); gx += gszw, col++)
     {
        for (gy = 0; gy <= (h + gszh - 1); gy += gszh)
          {
             Vertex2 vi;
             Vertex3 vo, tvo1;

             if (gx > w) vi.x = w;
             else vi.x = gx;
             if (gy > h) vi.y = h;
             else vi.y = gy;
             _deform_point(&vi, &vo, rho, theta, A);
             tvo1 = tvol[col];
             if (gy > h) tvo1.y = h;
             else tvo1.y = gy;
             _interp_point(&vo, &tvo1, &(tvo[num]), n);
             num++;
          }
     }

   jump = sd->slices_h + 1;
   for (col = 0, gx = 0; gx < w; gx += gszw, col++)
     {
        num = sd->slices_h * col;
        num2 = jump * col;

        gw = gszw;
        if ((gx + gw) > w) gw = w - gx;

        for (row = 0, gy = 0; gy < h; gy += gszh, row++)
          {
             Vertex3 vo[4];

             memset(vo, 0, sizeof (vo));

             if (b > 0) nn = num + sd->slices_h - row - 1;
             else nn = num + row;

             gh = gszh;
             if ((gy + gh) > h) gh = h - gy;

             vo[0] = tvo[num2 + row];
             vo[1] = tvo[num2 + row + jump];
             vo[2] = tvo[num2 + row + jump + 1];
             vo[3] = tvo[num2 + row + 1];

#define SWP(a, b)   \
  do {              \
       typeof(a)vt; \
       vt = (a);    \
       (a) = (b);   \
       (b) = vt;    \
    } while (0)

             if (b > 0)
               {
                  SWP(vo[0], vo[3]);
                  SWP(vo[1], vo[2]);
                  vo[0].y = h - vo[0].y;
                  vo[1].y = h - vo[1].y;
                  vo[2].y = h - vo[2].y;
                  vo[3].y = h - vo[3].y;
               }

             // FRONT
             sl = sd->slices[nn];
             if (!sl)
               {
                  sl = _slice_new(sd, front);
                  sd->slices[nn] = sl;
               }
             _slice_xyz(sd, sl,
                        vo[0].x, vo[0].y, vo[0].z,
                        vo[1].x, vo[1].y, vo[1].z,
                        vo[2].x, vo[2].y, vo[2].z,
                        vo[3].x, vo[3].y, vo[3].z);
             if (b <= 0)
               _slice_uv(sd, sl,
                         gx, gy, gx + gw, gy, gx + gw, gy + gh, gx, gy + gh);
             else
               _slice_uv(sd, sl,
                         gx, h - (gy + gh), gx + gw, h - (gy + gh), gx + gw,
                         h - gy, gx, h - gy);

             // BACK
             sl = sd->slices2[nn];
             if (!sl)
               {
                  sl = _slice_new(sd, back);
                  sd->slices2[nn] = sl;
               }

             _slice_xyz(sd, sl,
                        vo[1].x, vo[1].y, vo[1].z,
                        vo[0].x, vo[0].y, vo[0].z,
                        vo[3].x, vo[3].y, vo[3].z,
                        vo[2].x, vo[2].y, vo[2].z);
             if (sd->backflip)
               {
                  if (b <= 0)
                    _slice_uv(sd, sl, gx + gw, gy, gx, gy, gx, gy + gh, gx + gw,
                              gy + gh);
                  else
                    _slice_uv(sd, sl, gx + gw, h - (gy + gh), gx, h - (gy + gh),
                              gx, h - gy, gx + gw, h - gy);
               }
             else
               {
                  if (b <= 0)
                    _slice_uv(sd, sl, w - (gx + gw), gy, w - (gx), gy, w - (gx),
                              gy + gh, w - (gx + gw), gy + gh);
                  else
                    _slice_uv(sd, sl, w - (gx + gw), h - (gy + gh), w - (gx),
                              h - (gy + gh), w - (gx), h - gy, w - (gx + gw),
                              h - gy);
               }
          }
     }

   for (num = 0; num < sd->slices_h * sd->slices_w; num++)
     {
        _slice_apply(sd, sd->slices[num], x, y, w, h, ox, oy, ow, oh);
        _slice_apply(sd, sd->slices2[num], x, y, w, h, ox, oy, ow, oh);
        _slice_light(sd, sd->slices[num], ox, oy, ow, oh);
        _slice_light(sd, sd->slices2[num], ox, oy, ow, oh);
     }

   for (i = 0; i <= sd->slices_w; i++)
     {
        num = i * sd->slices_h;
        for (j = 0; j <= sd->slices_h; j++)
          {
             Slice *s[4] = { NULL }, *s2[4] = { NULL };

             if ((i > 0) && (j > 0))
               s[0] = sd->slices[num - 1 - sd->slices_h],
               s2[0] = sd->slices2[num - 1 - sd->slices_h];
             if ((i < sd->slices_w) && (j > 0))
               s[1] = sd->slices[num - 1],
               s2[1] = sd->slices2[num - 1];
             if ((i > 0) && (j < sd->slices_h))
               s[2] = sd->slices[num - sd->slices_h],
               s2[2] = sd->slices2[num - sd->slices_h];
             if ((i < sd->slices_w) && (j < sd->slices_h))
               s[3] = sd->slices[num],
               s2[3] = sd->slices2[num];
             switch (sd->dir)
               {
                case 0:
                  _slice_obj_vert_color_merge
                    (s[0], 2, s[1], 3, s[2], 1, s[3], 0);
                  _slice_obj_vert_color_merge
                    (s2[0], 3, s2[1], 2, s2[2], 0, s2[3], 1);
                  break;

                case 1:
                  _slice_obj_vert_color_merge
                    (s[0], 3, s[1], 2, s[2], 0, s[3], 1);
                  _slice_obj_vert_color_merge
                    (s2[0], 2, s2[1], 3, s2[2], 1, s2[3], 0);
                  break;

                case 2:
                  _slice_obj_vert_color_merge
                    (s[0], 3, s[1], 2, s[2], 0, s[3], 1);
                  _slice_obj_vert_color_merge
                    (s2[0], 2, s2[1], 3, s2[2], 1, s2[3], 0);
                  break;

                default:
                  _slice_obj_vert_color_merge
                    (s[0], 2, s[1], 3, s[2], 1, s[3], 0);
                  _slice_obj_vert_color_merge
                    (s2[0], 3, s2[1], 2, s2[2], 0, s2[3], 1);
               }
             num++;
          }
     }

   for (num = 0; num < sd->slices_h * sd->slices_w; num++)
     {
        _slice_3d(sd, sd->slices[num], ox, oy, ow, oh);
        _slice_3d(sd, sd->slices2[num], ox, oy, ow, oh);
     }

   return 1;
}

static void
_state_end(Elm_Flip_Smart_Data *sd)
{
   _state_slices_clear(sd);
}

static void
_flip_show_hide(Evas_Object *obj)
{
   ELM_FLIP_DATA_GET(obj, sd);
   if (elm_flip_front_visible_get(obj))
     {
        if (sd->pageflip)
          {
             if (sd->front.content)
               {
                  evas_object_move(sd->front.content, 4999, 4999);
                  evas_object_show(sd->front.clip);
               }
             else
               evas_object_hide(sd->front.clip);
             if (sd->back.content)
               evas_object_show(sd->back.clip);
             else
               evas_object_hide(sd->back.clip);
          }
        else
          {
             if (sd->front.content)
               evas_object_show(sd->front.clip);
             else
               evas_object_hide(sd->front.clip);
             evas_object_hide(sd->back.clip);
          }
     }
   else
     {
        if (sd->pageflip)
          {
             if (sd->front.content)
               evas_object_show(sd->front.clip);
             else
               evas_object_hide(sd->front.clip);
             if (sd->back.content)
               {
                  evas_object_move(sd->back.content, 4999, 4999);
                  evas_object_show(sd->back.clip);
               }
             else
               evas_object_hide(sd->back.clip);
          }
        else
          {
             evas_object_hide(sd->front.clip);
             if (sd->back.content)
               evas_object_show(sd->back.clip);
             else
               evas_object_hide(sd->back.clip);
          }
     }
}

static void
_flip_do(Evas_Object *obj,
         double t,
         Elm_Flip_Mode mode,
         int lin,
         int rev)
{
   double p, deg, pp;
   Evas_Map *mf, *mb;
   Evas_Coord x, y, w, h;
   Evas_Coord cx, cy, px, py, foc;
   int lx, ly, lz, lr, lg, lb, lar, lag, lab;

   ELM_FLIP_DATA_GET(obj, sd);

   mf = evas_map_new(4);
   evas_map_smooth_set(mf, EINA_FALSE);
   mb = evas_map_new(4);
   evas_map_smooth_set(mb, EINA_FALSE);

   if (sd->front.content)
     {
        const char *type = evas_object_type_get(sd->front.content);

        // FIXME: only handles filled obj
        if ((type) && (!strcmp(type, "image")))
          {
             int iw, ih;
             evas_object_image_size_get(sd->front.content, &iw, &ih);
             evas_object_geometry_get(sd->front.content, &x, &y, &w, &h);
             evas_map_util_points_populate_from_geometry(mf, x, y, w, h, 0);
             evas_map_point_image_uv_set(mf, 0, 0, 0);
             evas_map_point_image_uv_set(mf, 1, iw, 0);
             evas_map_point_image_uv_set(mf, 2, iw, ih);
             evas_map_point_image_uv_set(mf, 3, 0, ih);
          }
        else
          {
             evas_object_geometry_get(sd->front.content, &x, &y, &w, &h);
             evas_map_util_points_populate_from_geometry(mf, x, y, w, h, 0);
          }
     }
   if (sd->back.content)
     {
        const char *type = evas_object_type_get(sd->back.content);

        if ((type) && (!strcmp(type, "image")))
          {
             int iw, ih;
             evas_object_image_size_get(sd->back.content, &iw, &ih);
             evas_object_geometry_get(sd->back.content, &x, &y, &w, &h);
             evas_map_util_points_populate_from_geometry(mb, x, y, w, h, 0);
             evas_map_point_image_uv_set(mb, 0, 0, 0);
             evas_map_point_image_uv_set(mb, 1, iw, 0);
             evas_map_point_image_uv_set(mb, 2, iw, ih);
             evas_map_point_image_uv_set(mb, 3, 0, ih);
          }
        else
          {
             evas_object_geometry_get(sd->back.content, &x, &y, &w, &h);
             evas_map_util_points_populate_from_geometry(mb, x, y, w, h, 0);
          }
     }

   evas_object_geometry_get(obj, &x, &y, &w, &h);

   cx = x + (w / 2);
   cy = y + (h / 2);

   px = x + (w / 2);
   py = y + (h / 2);
   foc = 2048;

   lx = cx;
   ly = cy;
   lz = -10000;
   lr = 255;
   lg = 255;
   lb = 255;
   lar = 0;
   lag = 0;
   lab = 0;

   switch (mode)
     {
      case ELM_FLIP_ROTATE_Y_CENTER_AXIS:
        p = 1.0 - t;
        pp = p;
        if (!lin) pp = (p * p);
        p = 1.0 - pp;
        if (sd->state) deg = 180.0 * p;
        else deg = 180 + (180.0 * p);
        if (rev) deg = -deg;
        evas_map_util_3d_rotate(mf, 0.0, deg, 0.0, cx, cy, 0);
        evas_map_util_3d_rotate(mb, 0.0, 180 + deg, 0.0, cx, cy, 0);
        break;

      case ELM_FLIP_ROTATE_X_CENTER_AXIS:
        p = 1.0 - t;
        pp = p;
        if (!lin) pp = (p * p);
        p = 1.0 - pp;
        if (sd->state) deg = 180.0 * p;
        else deg = 180 + (180.0 * p);
        if (rev) deg = -deg;
        evas_map_util_3d_rotate(mf, deg, 0.0, 0.0, cx, cy, 0);
        evas_map_util_3d_rotate(mb, 180.0 + deg, 0.0, 0.0, cx, cy, 0);
        break;

      case ELM_FLIP_ROTATE_XZ_CENTER_AXIS:
        p = 1.0 - t;
        pp = p;
        if (!lin) pp = (p * p);
        p = 1.0 - pp;
        if (sd->state) deg = 180.0 * p;
        else deg = 180 + (180.0 * p);
        if (rev) deg = -deg;
        evas_map_util_3d_rotate(mf, deg, 0.0, deg, cx, cy, 0);
        evas_map_util_3d_rotate(mb, 180 + deg, 0.0, 180 + deg, cx, cy, 0);
        break;

      case ELM_FLIP_ROTATE_YZ_CENTER_AXIS:
        p = 1.0 - t;
        pp = p;
        if (!lin) pp = (p * p);
        p = 1.0 - pp;
        if (sd->state) deg = 180.0 * p;
        else deg = 180 + (180.0 * p);
        if (rev) deg = -deg;
        evas_map_util_3d_rotate(mf, 0.0, deg, deg, cx, cy, 0);
        evas_map_util_3d_rotate(mb, 0.0, 180.0 + deg, 180.0 + deg, cx, cy, 0);
        break;

      case ELM_FLIP_CUBE_LEFT:
        p = 1.0 - t;
        pp = p;
        if (!lin) pp = (p * p);
        p = 1.0 - pp;
        deg = -90.0 * p;
        if (sd->state)
          {
             evas_map_util_3d_rotate(mf, 0.0, deg, 0.0, cx, cy, w / 2);
             evas_map_util_3d_rotate(mb, 0.0, deg + 90, 0.0, cx, cy, w / 2);
          }
        else
          {
             evas_map_util_3d_rotate(mf, 0.0, deg + 90, 0.0, cx, cy, w / 2);
             evas_map_util_3d_rotate(mb, 0.0, deg, 0.0, cx, cy, w / 2);
          }
        break;

      case ELM_FLIP_CUBE_RIGHT:
        p = 1.0 - t;
        pp = p;
        if (!lin) pp = (p * p);
        p = 1.0 - pp;
        deg = 90.0 * p;
        if (sd->state)
          {
             evas_map_util_3d_rotate(mf, 0.0, deg, 0.0, cx, cy, w / 2);
             evas_map_util_3d_rotate(mb, 0.0, deg - 90, 0.0, cx, cy, w / 2);
          }
        else
          {
             evas_map_util_3d_rotate(mf, 0.0, deg - 90, 0.0, cx, cy, w / 2);
             evas_map_util_3d_rotate(mb, 0.0, deg, 0.0, cx, cy, w / 2);
          }
        break;

      case ELM_FLIP_CUBE_UP:
        p = 1.0 - t;
        pp = p;
        if (!lin) pp = (p * p);
        p = 1.0 - pp;
        deg = -90.0 * p;
        if (sd->state)
          {
             evas_map_util_3d_rotate(mf, deg, 0.0, 0.0, cx, cy, h / 2);
             evas_map_util_3d_rotate(mb, deg + 90, 0.0, 0.0, cx, cy, h / 2);
          }
        else
          {
             evas_map_util_3d_rotate(mf, deg + 90, 0.0, 0.0, cx, cy, h / 2);
             evas_map_util_3d_rotate(mb, deg, 0.0, 0.0, cx, cy, h / 2);
          }
        break;

      case ELM_FLIP_CUBE_DOWN:
        p = 1.0 - t;
        pp = p;
        if (!lin) pp = (p * p);
        p = 1.0 - pp;
        deg = 90.0 * p;
        if (sd->state)
          {
             evas_map_util_3d_rotate(mf, deg, 0.0, 0.0, cx, cy, h / 2);
             evas_map_util_3d_rotate(mb, deg - 90, 0.0, 0.0, cx, cy, h / 2);
          }
        else
          {
             evas_map_util_3d_rotate(mf, deg - 90, 0.0, 0.0, cx, cy, h / 2);
             evas_map_util_3d_rotate(mb, deg, 0.0, 0.0, cx, cy, h / 2);
          }
        break;

      case ELM_FLIP_PAGE_LEFT:
        break;

      case ELM_FLIP_PAGE_RIGHT:
        break;

      case ELM_FLIP_PAGE_UP:
        break;

      case ELM_FLIP_PAGE_DOWN:
        break;

      default:
        break;
     }

   if (sd->front.content)
     {
        evas_map_util_3d_lighting(mf, lx, ly, lz, lr, lg, lb, lar, lag, lab);
        evas_map_util_3d_perspective(mf, px, py, 0, foc);
        evas_object_map_set(sd->front.content, mf);
        evas_object_map_enable_set(sd->front.content, 1);
        if (evas_map_util_clockwise_get(mf)) evas_object_show(sd->front.clip);
        else evas_object_hide(sd->front.clip);
     }

   if (sd->back.content)
     {
        evas_map_util_3d_lighting(mb, lx, ly, lz, lr, lg, lb, lar, lag, lab);
        evas_map_util_3d_perspective(mb, px, py, 0, foc);
        evas_object_map_set(sd->back.content, mb);
        evas_object_map_enable_set(sd->back.content, 1);
        if (evas_map_util_clockwise_get(mb)) evas_object_show(sd->back.clip);
        else evas_object_hide(sd->back.clip);
     }

   evas_map_free(mf);
   evas_map_free(mb);
}

static void
_show_hide(Evas_Object *obj)
{
   ELM_FLIP_DATA_GET(obj, sd);
   Evas_Coord x, y, w, h;
   if (!sd) return;

   evas_object_geometry_get(obj, &x, &y, &w, &h);
   if (sd->front.content)
     {
        if ((sd->pageflip) && (sd->state))
          {
             evas_object_move(sd->front.content, 4999, 4999);
          }
        else
          {
             if (!sd->animator)
               evas_object_move(sd->front.content, x, y);
          }
        evas_object_resize(sd->front.content, w, h);
     }
   if (sd->back.content)
     {
        if ((sd->pageflip) && (!sd->state))
          {
             evas_object_move(sd->back.content, 4999, 4999);
          }
        else
          {
             if (!sd->animator)
               evas_object_move(sd->back.content, x, y);
          }
        evas_object_resize(sd->back.content, w, h);
     }
}

static void
_configure(Evas_Object *obj)
{
   Evas_Coord x, y, w, h;
   Evas_Coord fsize;

   ELM_FLIP_DATA_GET(obj, sd);

   _show_hide(obj);
   evas_object_geometry_get(obj, &x, &y, &w, &h);
   // FIXME: manual flip wont get fixed
   if (sd->animator) _flip(obj);

   if (sd->event[0])
     {
        fsize = (double)w * sd->dir_hitsize[0];
        elm_coords_finger_size_adjust(0, NULL, 1, &fsize);
        evas_object_move(sd->event[0], x, y);
        evas_object_resize(sd->event[0], w, fsize);
     }
   if (sd->event[1])
     {
        fsize = (double)w * sd->dir_hitsize[1];
        elm_coords_finger_size_adjust(0, NULL, 1, &fsize);
        evas_object_move(sd->event[1], x, y + h - fsize);
        evas_object_resize(sd->event[1], w, fsize);
     }
   if (sd->event[2])
     {
        fsize = (double)h * sd->dir_hitsize[2];
        elm_coords_finger_size_adjust(1, &fsize, 0, NULL);
        evas_object_move(sd->event[2], x, y);
        evas_object_resize(sd->event[2], fsize, h);
     }
   if (sd->event[3])
     {
        fsize = (double)h * sd->dir_hitsize[3];
        elm_coords_finger_size_adjust(1, &fsize, 0, NULL);
        evas_object_move(sd->event[3], x + w - fsize, y);
        evas_object_resize(sd->event[3], fsize, h);
     }
}

static Eina_Bool
_flip(Evas_Object *obj)
{
   double t;
   Evas_Coord w, h;

   ELM_FLIP_DATA_GET(obj, sd);

   t = ecore_loop_time_get() - sd->start;

   if (!sd->animator) return ECORE_CALLBACK_CANCEL;

   t = t / sd->len;
   if (t > 1.0) t = 1.0;

   evas_object_geometry_get(obj, NULL, NULL, &w, &h);
   if (!sd->manual)
     {
        if (sd->mode == ELM_FLIP_PAGE_LEFT)
          {
             sd->dir = 0;
             sd->started = EINA_TRUE;
             sd->pageflip = EINA_TRUE;
             sd->down_x = w - 1;
             sd->down_y = h / 2;
             sd->x = (1.0 - t) * sd->down_x;
             sd->y = sd->down_y;
             _flip_show_hide(obj);
             _state_update(sd);
          }
        else if (sd->mode == ELM_FLIP_PAGE_RIGHT)
          {
             sd->dir = 1;
             sd->started = EINA_TRUE;
             sd->pageflip = EINA_TRUE;
             sd->down_x = 0;
             sd->down_y = h / 2;
             sd->x = (t) * w;
             sd->y = sd->down_y;
             _flip_show_hide(obj);
             _state_update(sd);
          }
        else if (sd->mode == ELM_FLIP_PAGE_UP)
          {
             sd->dir = 2;
             sd->started = EINA_TRUE;
             sd->pageflip = EINA_TRUE;
             sd->down_x = w / 2;
             sd->down_y = h - 1;
             sd->x = sd->down_x;
             sd->y = (1.0 - t) * sd->down_y;
             _flip_show_hide(obj);
             _state_update(sd);
          }
        else if (sd->mode == ELM_FLIP_PAGE_DOWN)
          {
             sd->dir = 3;
             sd->started = EINA_TRUE;
             sd->pageflip = EINA_TRUE;
             sd->down_x = w / 2;
             sd->down_y = 0;
             sd->x = sd->down_x;
             sd->y = (t) * h;
             _flip_show_hide(obj);
             _state_update(sd);
          }
        else
          _flip_do(obj, t, sd->mode, 0, 0);
     }

   if (t >= 1.0)
     {
#if 0 // this breaks manual flipping. :/
        if (sd->state == sd->next_state)
          {
             /* it was flipped while flipping, do it again */
             sd->start = ecore_loop_time_get();
             sd->state = !sd->next_state;
             return ECORE_CALLBACK_RENEW;
          }
#endif
        sd->pageflip = EINA_FALSE;
        _state_end(sd);
        evas_object_map_enable_set(sd->front.content, 0);
        evas_object_map_enable_set(sd->back.content, 0);
        // FIXME: hack around evas rendering bug (only fix makes evas bitch-slow
        evas_object_resize(sd->front.content, 0, 0);
        evas_object_resize(sd->back.content, 0, 0);
        evas_smart_objects_calculate(evas_object_evas_get(obj));
        // FIXME: if object is deleted, callback will not be called. Need to redesign
        ELM_WIDGET_CHECK_OR_RETURN(obj, ECORE_CALLBACK_CANCEL);

        // FIXME: end hack
        sd->animator = NULL;
        if (((sd->manual) && (sd->finish)) || (!sd->manual))
          sd->state = sd->next_state;
        _configure(obj);
        _flip_show_hide(obj);
        evas_object_smart_callback_call(obj, SIG_ANIMATE_DONE, NULL);

        return ECORE_CALLBACK_CANCEL;
     }

   return ECORE_CALLBACK_RENEW;
}

/* we have to have move/resize info up-to-date on those events. it
 * happens that smarts callbacks on them happen before we have the new
 * values, so using event callbacks instead */
static void
_on_move(void *data __UNUSED__,
         Evas *e __UNUSED__,
         Evas_Object *obj,
         void *event_info __UNUSED__)
{
   _configure(obj);
}

static void
_on_resize(void *data __UNUSED__,
           Evas *e __UNUSED__,
           Evas_Object *obj,
           void *event_info __UNUSED__)
{
   _configure(obj);
}

static Eina_Bool
_animate(void *data)
{
   return _flip(data);
}

static double
_pos_get(Elm_Flip_Smart_Data *sd,
         int *rev,
         Elm_Flip_Mode *m)
{
   Evas_Coord x, y, w, h;
   double t = 1.0;

   evas_object_geometry_get(ELM_WIDGET_DATA(sd)->obj, &x, &y, &w, &h);
   switch (sd->intmode)
     {
      case ELM_FLIP_INTERACTION_ROTATE:
      case ELM_FLIP_INTERACTION_CUBE:
      {
         if (sd->dir == 0)
           {
              if (sd->down_x > 0)
                t = 1.0 - ((double)sd->x / (double)sd->down_x);
              *rev = 1;
           }
         else if (sd->dir == 1)
           {
              if (sd->down_x < w)
                t = 1.0 - ((double)(w - sd->x) / (double)(w - sd->down_x));
           }
         else if (sd->dir == 2)
           {
              if (sd->down_y > 0)
                t = 1.0 - ((double)sd->y / (double)sd->down_y);
           }
         else if (sd->dir == 3)
           {
              if (sd->down_y < h)
                t = 1.0 - ((double)(h - sd->y) / (double)(h - sd->down_y));
              *rev = 1;
           }

         if (t < 0.0) t = 0.0;
         else if (t > 1.0)
           t = 1.0;

         if ((sd->dir == 0) || (sd->dir == 1))
           {
              if (sd->intmode == ELM_FLIP_INTERACTION_ROTATE)
                *m = ELM_FLIP_ROTATE_Y_CENTER_AXIS;
              else if (sd->intmode == ELM_FLIP_INTERACTION_CUBE)
                {
                   if (*rev)
                     *m = ELM_FLIP_CUBE_LEFT;
                   else
                     *m = ELM_FLIP_CUBE_RIGHT;
                }
           }
         else
           {
              if (sd->intmode == ELM_FLIP_INTERACTION_ROTATE)
                *m = ELM_FLIP_ROTATE_X_CENTER_AXIS;
              else if (sd->intmode == ELM_FLIP_INTERACTION_CUBE)
                {
                   if (*rev)
                     *m = ELM_FLIP_CUBE_UP;
                   else
                     *m = ELM_FLIP_CUBE_DOWN;
                }
           }
      }

      default:
        break;
     }
   return t;
}

static Eina_Bool
_event_anim(void *data,
            double pos)
{
   Elm_Flip_Smart_Data *sd = data;
   double p;

   p = ecore_animator_pos_map(pos, ECORE_POS_MAP_ACCELERATE, 0.0, 0.0);
   if (sd->finish)
     {
        if (sd->dir == 0)
          sd->x = sd->ox * (1.0 - p);
        else if (sd->dir == 1)
          sd->x = sd->ox + ((sd->w - sd->ox) * p);
        else if (sd->dir == 2)
          sd->y = sd->oy * (1.0 - p);
        else if (sd->dir == 3)
          sd->y = sd->oy + ((sd->h - sd->oy) * p);
     }
   else
     {
        if (sd->dir == 0)
          sd->x = sd->ox + ((sd->w - sd->ox) * p);
        else if (sd->dir == 1)
          sd->x = sd->ox * (1.0 - p);
        else if (sd->dir == 2)
          sd->y = sd->oy + ((sd->h - sd->oy) * p);
        else if (sd->dir == 3)
          sd->y = sd->oy * (1.0 - p);
     }
   switch (sd->intmode)
     {
      case ELM_FLIP_INTERACTION_NONE:
        break;

      case ELM_FLIP_INTERACTION_ROTATE:
      case ELM_FLIP_INTERACTION_CUBE:
      {
         Elm_Flip_Mode m = ELM_FLIP_ROTATE_X_CENTER_AXIS;
         int rev = 0;
         p = _pos_get(sd, &rev, &m);
         _flip_do(ELM_WIDGET_DATA(sd)->obj, p, m, 1, rev);
      }
      break;

      case ELM_FLIP_INTERACTION_PAGE:
        sd->pageflip = EINA_TRUE;
        _configure(ELM_WIDGET_DATA(sd)->obj);
        _state_update(sd);
        break;

      default:
        break;
     }
   if (pos < 1.0) return ECORE_CALLBACK_RENEW;

   sd->pageflip = EINA_FALSE;
   _state_end(sd);
   evas_object_map_enable_set(sd->front.content, 0);
   evas_object_map_enable_set(sd->back.content, 0);
   // FIXME: hack around evas rendering bug (only fix makes evas bitch-slow
   evas_object_resize(sd->front.content, 0, 0);
   evas_object_resize(sd->back.content, 0, 0);
   evas_smart_objects_calculate
     (evas_object_evas_get(ELM_WIDGET_DATA(sd)->obj));
   // FIXME: if object is deleted, callback will not be called. Need to redesign
   ELM_WIDGET_CHECK_OR_RETURN(ELM_WIDGET_DATA(sd)->obj, ECORE_CALLBACK_CANCEL);

   // FIXME: end hack
   sd->animator = NULL;
   if (sd->finish) sd->state = sd->next_state;
   _flip_show_hide(ELM_WIDGET_DATA(sd)->obj);
   _configure(ELM_WIDGET_DATA(sd)->obj);
   sd->animator = NULL;
   evas_object_smart_callback_call
     (ELM_WIDGET_DATA(sd)->obj, SIG_ANIMATE_DONE, NULL);

   return ECORE_CALLBACK_CANCEL;
}

static void
_update_job(void *data)
{
   Elm_Flip_Mode m = ELM_FLIP_ROTATE_X_CENTER_AXIS;
   Elm_Flip_Smart_Data *sd = data;
   int rev = 0;
   double p;

   sd->job = NULL;
   switch (sd->intmode)
     {
      case ELM_FLIP_INTERACTION_ROTATE:
      case ELM_FLIP_INTERACTION_CUBE:
        p = _pos_get(sd, &rev, &m);
        _flip_do(ELM_WIDGET_DATA(sd)->obj, p, m, 1, rev);
        break;

      case ELM_FLIP_INTERACTION_PAGE:
        sd->pageflip = EINA_TRUE;
        _configure(ELM_WIDGET_DATA(sd)->obj);
        _state_update(sd);
        break;

      default:
        break;
     }
}

static void
_down_cb(void *data,
         Evas *e __UNUSED__,
         Evas_Object *obj __UNUSED__,
         void *event_info)
{
   Evas_Event_Mouse_Down *ev = event_info;
   Evas_Object *fl = data;
   Evas_Coord x, y, w, h;

   ELM_FLIP_DATA_GET(fl, sd);

   if (ev->button != 1) return;
   if (ev->event_flags & EVAS_EVENT_FLAG_ON_HOLD) return;
   if (sd->animator)
     {
        ecore_animator_del(sd->animator);
        sd->animator = NULL;
     }
   sd->down = EINA_TRUE;
   sd->started = EINA_FALSE;
   evas_object_geometry_get(data, &x, &y, &w, &h);
   sd->x = ev->canvas.x - x;
   sd->y = ev->canvas.y - y;
   sd->w = w;
   sd->h = h;
   sd->down_x = sd->x;
   sd->down_y = sd->y;
}

static void
_up_cb(void *data,
       Evas *e __UNUSED__,
       Evas_Object *obj __UNUSED__,
       void *event_info)
{
   Evas_Event_Mouse_Up *ev = event_info;
   Evas_Object *fl = data;
   Evas_Coord x, y, w, h;
   double tm = 0.5;

   ELM_FLIP_DATA_GET(fl, sd);

   if (ev->button != 1) return;
   if (ev->event_flags & EVAS_EVENT_FLAG_ON_HOLD) return;
   sd->down = 0;
   if (!sd->started) return;
   evas_object_geometry_get(data, &x, &y, &w, &h);
   sd->x = ev->canvas.x - x;
   sd->y = ev->canvas.y - y;
   sd->w = w;
   sd->h = h;
   sd->ox = sd->x;
   sd->oy = sd->y;
   if (sd->job)
     {
        ecore_job_del(sd->job);
        sd->job = NULL;
     }
   sd->finish = EINA_FALSE;
   if (sd->dir == 0)
     {
        tm = (double)sd->x / (double)sd->w;
        if (sd->x < (sd->w / 2)) sd->finish = EINA_TRUE;
     }
   else if (sd->dir == 1)
     {
        if (sd->x > (sd->w / 2)) sd->finish = EINA_TRUE;
        tm = 1.0 - ((double)sd->x / (double)sd->w);
     }
   else if (sd->dir == 2)
     {
        if (sd->y < (sd->h / 2)) sd->finish = EINA_TRUE;
        tm = (double)sd->y / (double)sd->h;
     }
   else if (sd->dir == 3)
     {
        if (sd->y > (sd->h / 2)) sd->finish = EINA_TRUE;
        tm = 1.0 - ((double)sd->y / (double)sd->h);
     }
   if (tm < 0.01) tm = 0.01;
   else if (tm > 0.99) tm = 0.99;
   if (!sd->finish) tm = 1.0 - tm;
   else sd->next_state = !sd->state;
   tm *= 1.0; // FIXME: config for anim time
   if (sd->animator) ecore_animator_del(sd->animator);
   sd->animator = ecore_animator_timeline_add(tm, _event_anim, sd);
   sd->len = tm;
   sd->start = ecore_loop_time_get();
   sd->manual = EINA_TRUE;
   _event_anim(sd, 0.0);
}

static void
_move_cb(void *data,
         Evas *e __UNUSED__,
         Evas_Object *obj __UNUSED__,
         void *event_info)
{
   Evas_Event_Mouse_Move *ev = event_info;
   Evas_Object *fl = data;
   Evas_Coord x, y, w, h;

   ELM_FLIP_DATA_GET(fl, sd);

   if (!sd->down) return;
   if (ev->event_flags & EVAS_EVENT_FLAG_ON_HOLD) return;
   evas_object_geometry_get(data, &x, &y, &w, &h);
   sd->x = ev->cur.canvas.x - x;
   sd->y = ev->cur.canvas.y - y;
   sd->w = w;
   sd->h = h;
   if (!sd->started)
     {
        Evas_Coord dx, dy;

        dx = sd->x - sd->down_x;
        dy = sd->y - sd->down_y;
        if (((dx * dx) + (dy * dy)) >
            (_elm_config->finger_size * _elm_config->finger_size / 4))
          {
             sd->dir = 0;
             if ((sd->x > (w / 2)) &&
                 (dx < 0) && (abs(dx) > abs(dy)))
               {
                  sd->dir = 0;  // left
                  if (!sd->dir_enabled[ELM_FLIP_DIRECTION_LEFT]) return;
               }
             else if ((sd->x < (w / 2)) && (dx >= 0) &&
                      (abs(dx) > abs(dy)))
               {
                  sd->dir = 1;  // right
                  if (!sd->dir_enabled[ELM_FLIP_DIRECTION_RIGHT]) return;
               }
             else if ((sd->y > (h / 2)) && (dy < 0) && (abs(dy) >= abs(dx)))
               {
                  sd->dir = 2;  // up
                  if (!sd->dir_enabled[ELM_FLIP_DIRECTION_UP]) return;
               }
             else if ((sd->y < (h / 2)) && (dy >= 0) && (abs(dy) >= abs(dx)))
               {
                  sd->dir = 3;  // down
                  if (!sd->dir_enabled[ELM_FLIP_DIRECTION_DOWN]) return;
               }
             sd->started = EINA_TRUE;
             if (sd->intmode == ELM_FLIP_INTERACTION_PAGE)
               sd->pageflip = EINA_TRUE;
             _flip_show_hide(data);
             evas_smart_objects_calculate(evas_object_evas_get(data));
             // FIXME: if object is deleted, callback will not be called. Need to redesign
             ELM_WIDGET_CHECK_OR_RETURN(fl);
             _flip(data);
             // FIXME: hack around evas rendering bug (only fix makes
             // evas bitch-slow)
             evas_object_map_enable_set(sd->front.content, EINA_FALSE);
             evas_object_map_enable_set(sd->back.content, EINA_FALSE);
// FIXME: XXX why does this bork interactive flip??
//             evas_object_resize(sd->front.content, 0, 0);
//             evas_object_resize(sd->back.content, 0, 0);
             evas_smart_objects_calculate(evas_object_evas_get(data));
             // FIXME: if object is deleted, callback will not be called. Need to redesign
             ELM_WIDGET_CHECK_OR_RETURN(fl);

             _configure(fl);
             // FIXME: end hack
             evas_object_smart_callback_call(fl, SIG_ANIMATE_BEGIN, NULL);
          }
        else return;
     }

   ev->event_flags |= EVAS_EVENT_FLAG_ON_HOLD;
   if (sd->job) ecore_job_del(sd->job);
   sd->job = ecore_job_add(_update_job, sd);
}

static Eina_Bool
_flip_content_set(Evas_Object *obj,
                  Evas_Object *content,
                  Eina_Bool front)
{
   int i;
   Evas_Object **cont;

   ELM_FLIP_DATA_GET(obj, sd);

   cont = front ? &(sd->front.content) : &(sd->back.content);

   if (*cont == content) return EINA_TRUE;

   if (*cont) evas_object_del(*cont);
   *cont = content;

   if (content)
     {
        elm_widget_sub_object_add(obj, content);
        //evas_object_smart_member_add(content, obj);
        evas_object_clip_set
          (content, front ? sd->front.clip : sd->back.clip);
     }

   // force calc to contents are the right size before transition
   evas_smart_objects_calculate(evas_object_evas_get(obj));
   //evas_object_smart_calculate(obj);
   // FIXME: if object is deleted, callback will not be called. Need to redesign
   ELM_WIDGET_CHECK_OR_RETURN(obj, EINA_FALSE);

   _flip_show_hide(obj);
   _configure(obj);
   _sizing_eval(obj);

   if (sd->intmode != ELM_FLIP_INTERACTION_NONE)
     {
        for (i = 0; i < 4; i++)
          evas_object_raise(sd->event[i]);
     }

   return EINA_TRUE;
}

static Evas_Object *
_flip_content_unset(Evas_Object *obj,
                    Eina_Bool front)
{
   Evas_Object *content;
   Evas_Object **cont;

   ELM_FLIP_DATA_GET(obj, sd);

   cont = front ? &(sd->front.content) : &(sd->back.content);

   if (!*cont) return NULL;

   content = *cont;
   elm_widget_sub_object_del(obj, content);

   return content;
}

static Eina_Bool
_elm_flip_smart_content_set(Evas_Object *obj,
                            const char *part,
                            Evas_Object *content)
{
   if (!part || !strcmp(part, "front"))
     return _flip_content_set(obj, content, EINA_TRUE);
   else if (!strcmp(part, "back"))
     return _flip_content_set(obj, content, EINA_FALSE);

   return EINA_FALSE;
}

static Evas_Object *
_elm_flip_smart_content_get(const Evas_Object *obj,
                            const char *part)
{
   ELM_FLIP_DATA_GET(obj, sd);

   if (!part || !strcmp(part, "front"))
     return sd->front.content;
   else if (!strcmp(part, "back"))
     return sd->back.content;

   return NULL;
}

static Evas_Object *
_elm_flip_smart_content_unset(Evas_Object *obj,
                              const char *part)
{
   if (!part || !strcmp(part, "front"))
     return _flip_content_unset(obj, EINA_TRUE);
   else if (!strcmp(part, "back"))
     return _flip_content_unset(obj, EINA_FALSE);

   return NULL;
}

static void
_elm_flip_smart_add(Evas_Object *obj)
{
   EVAS_SMART_DATA_ALLOC(obj, Elm_Flip_Smart_Data);

   ELM_WIDGET_CLASS(_elm_flip_parent_sc)->base.add(obj);

}

static void
_elm_flip_smart_del(Evas_Object *obj)
{
   ELM_FLIP_DATA_GET(obj, sd);

   if (sd->animator) ecore_animator_del(sd->animator);
   _state_slices_clear(sd);

   ELM_WIDGET_CLASS(_elm_flip_parent_sc)->base.del(obj);
}

static void
_elm_flip_smart_set_user(Elm_Flip_Smart_Class *sc)
{
   ELM_WIDGET_CLASS(sc)->base.add = _elm_flip_smart_add;
   ELM_WIDGET_CLASS(sc)->base.del = _elm_flip_smart_del;

   ELM_WIDGET_CLASS(sc)->theme = _elm_flip_smart_theme;
   ELM_WIDGET_CLASS(sc)->focus_next = _elm_flip_smart_focus_next;
   ELM_WIDGET_CLASS(sc)->sub_object_add = _elm_flip_smart_sub_object_add;
   ELM_WIDGET_CLASS(sc)->sub_object_del = _elm_flip_smart_sub_object_del;

   ELM_CONTAINER_CLASS(sc)->content_set = _elm_flip_smart_content_set;
   ELM_CONTAINER_CLASS(sc)->content_get = _elm_flip_smart_content_get;
   ELM_CONTAINER_CLASS(sc)->content_unset = _elm_flip_smart_content_unset;
}

EAPI const Elm_Flip_Smart_Class *
elm_flip_smart_class_get(void)
{
   static Elm_Flip_Smart_Class _sc =
     ELM_FLIP_SMART_CLASS_INIT_NAME_VERSION(ELM_FLIP_SMART_NAME);
   static const Elm_Flip_Smart_Class *class = NULL;
   Evas_Smart_Class *esc = (Evas_Smart_Class *)&_sc;

   if (class) return class;

   _elm_flip_smart_set(&_sc);
   esc->callbacks = _smart_callbacks;
   class = &_sc;

   return class;
}

EAPI Evas_Object *
elm_flip_add(Evas_Object *parent)
{
   Evas_Object *obj;

   EINA_SAFETY_ON_NULL_RETURN_VAL(parent, NULL);

   obj = elm_widget_add(_elm_flip_smart_class_new(), parent);
   if (!obj) return NULL;

   if (!elm_widget_sub_object_add(parent, obj))
     ERR("could not add %p as sub object of %p", obj, parent);

   ELM_FLIP_DATA_GET(obj, sd);

   sd->clip = evas_object_rectangle_add(evas_object_evas_get(obj));
   evas_object_static_clip_set(sd->clip, EINA_TRUE);
   evas_object_move(sd->clip, -49999, -49999);
   evas_object_resize(sd->clip, 99999, 99999);
   evas_object_smart_member_add(sd->clip, obj);

   sd->front.clip = evas_object_rectangle_add(evas_object_evas_get(obj));
   evas_object_static_clip_set(sd->front.clip, EINA_TRUE);
   evas_object_data_set(sd->front.clip, "_elm_leaveme", obj);
   evas_object_move(sd->front.clip, -49999, -49999);
   evas_object_resize(sd->front.clip, 99999, 99999);
   evas_object_smart_member_add(sd->front.clip, obj);
   evas_object_clip_set(sd->front.clip, sd->clip);

   sd->back.clip = evas_object_rectangle_add(evas_object_evas_get(obj));
   evas_object_static_clip_set(sd->back.clip, EINA_TRUE);
   evas_object_data_set(sd->back.clip, "_elm_leaveme", obj);
   evas_object_move(sd->back.clip, -49999, -49999);
   evas_object_resize(sd->back.clip, 99999, 99999);
   evas_object_smart_member_add(sd->back.clip, obj);
   evas_object_clip_set(sd->back.clip, sd->clip);

   evas_object_event_callback_add
     (obj, EVAS_CALLBACK_CHANGED_SIZE_HINTS, _changed_size_hints_cb, obj);

   evas_object_event_callback_add(obj, EVAS_CALLBACK_RESIZE, _on_resize, NULL);
   evas_object_event_callback_add(obj, EVAS_CALLBACK_MOVE, _on_move, NULL);

   sd->state = EINA_TRUE;
   sd->next_state = EINA_TRUE;
   sd->intmode = ELM_FLIP_INTERACTION_NONE;

   elm_widget_can_focus_set(obj, EINA_FALSE);

   _sizing_eval(obj);

   //Tizen Only: This should be removed when eo is applied.
   ELM_WIDGET_DATA_GET(obj, wsd);
   wsd->on_create = EINA_FALSE;

   return obj;
}

EAPI Eina_Bool
elm_flip_front_visible_get(const Evas_Object *obj)
{
   ELM_FLIP_CHECK(obj) EINA_FALSE;
   ELM_FLIP_DATA_GET(obj, sd);

   return sd->state;
}

EAPI void
elm_flip_perspective_set(Evas_Object *obj,
                         Evas_Coord foc __UNUSED__,
                         Evas_Coord x __UNUSED__,
                         Evas_Coord y __UNUSED__)
{
   ELM_FLIP_CHECK(obj);
}

// FIXME: add ambient and lighting control

static void
_elm_flip_go_to(Elm_Flip_Smart_Data *sd,
                Eina_Bool front,
                Elm_Flip_Mode mode)
{
   Evas_Object *obj = ELM_WIDGET_DATA(sd)->obj;

   if (!sd->animator) sd->animator = ecore_animator_add(_animate, obj);
   _flip_show_hide(obj);

   sd->mode = mode;
   sd->start = ecore_loop_time_get();
   sd->next_state = front;
   sd->len = 0.5; // FIXME: make config val
   sd->manual = EINA_FALSE;
   if ((sd->mode == ELM_FLIP_PAGE_LEFT) ||
       (sd->mode == ELM_FLIP_PAGE_RIGHT) ||
       (sd->mode == ELM_FLIP_PAGE_UP) ||
       (sd->mode == ELM_FLIP_PAGE_DOWN))
     sd->pageflip = EINA_TRUE;
   // force calc to contents are the right size before transition
   evas_smart_objects_calculate(evas_object_evas_get(obj));
   // FIXME: if object is deleted, callback will not be called. Need to redesign
   ELM_WIDGET_CHECK_OR_RETURN(obj);

   _flip(obj);
   // FIXME: hack around evas rendering bug (only fix makes evas bitch-slow)
   evas_object_map_enable_set(sd->front.content, 0);
   evas_object_map_enable_set(sd->back.content, 0);
   evas_object_resize(sd->front.content, 0, 0);
   evas_object_resize(sd->back.content, 0, 0);
   evas_smart_objects_calculate(evas_object_evas_get(obj));
   // FIXME: if object is deleted, callback will not be called. Need to redesign
   ELM_WIDGET_CHECK_OR_RETURN(obj);

   _configure(obj);
   // FIXME: end hack
   evas_object_smart_callback_call(obj, SIG_ANIMATE_BEGIN, NULL);
}

EAPI void
elm_flip_go_to(Evas_Object *obj,
               Eina_Bool front,
               Elm_Flip_Mode mode)
{
   ELM_FLIP_CHECK(obj);
   ELM_FLIP_DATA_GET(obj, sd);

   if (sd->next_state == front) return;

   _elm_flip_go_to(sd, front, mode);
}

EAPI void
elm_flip_go(Evas_Object *obj,
            Elm_Flip_Mode mode)
{
   ELM_FLIP_CHECK(obj);
   ELM_FLIP_DATA_GET(obj, sd);

   _elm_flip_go_to(sd, !sd->state, mode);
}

EAPI void
elm_flip_interaction_set(Evas_Object *obj,
                         Elm_Flip_Interaction mode)
{
   int i;

   ELM_FLIP_CHECK(obj);
   ELM_FLIP_DATA_GET(obj, sd);

   if (sd->intmode == mode) return;
   sd->intmode = mode;
   for (i = 0; i < 4; i++)
     {
        if (sd->intmode == ELM_FLIP_INTERACTION_NONE)
          {
             if (sd->event[i])
               {
                  evas_object_del(sd->event[i]);
                  sd->event[i] = NULL;
               }
          }
        else
          {
             if ((sd->dir_enabled[i]) && (!sd->event[i]))
               {
                  Evas *e = evas_object_evas_get(obj);
                  sd->event[i] = evas_object_rectangle_add(e);

                  evas_object_data_set(sd->event[i], "_elm_leaveme", obj);
                  evas_object_clip_set(sd->event[i], evas_object_clip_get(obj));
                  evas_object_color_set(sd->event[i], 0, 0, 0, 0);
                  evas_object_show(sd->event[i]);
                  evas_object_smart_member_add(sd->event[i], obj);
                  evas_object_event_callback_add
                    (sd->event[i], EVAS_CALLBACK_MOUSE_DOWN, _down_cb, obj);
                  evas_object_event_callback_add
                    (sd->event[i], EVAS_CALLBACK_MOUSE_UP, _up_cb, obj);
                  evas_object_event_callback_add
                    (sd->event[i], EVAS_CALLBACK_MOUSE_MOVE, _move_cb, obj);
               }
          }
     }

   _sizing_eval(obj);
   _configure(obj);
}

EAPI Elm_Flip_Interaction
elm_flip_interaction_get(const Evas_Object *obj)
{
   ELM_FLIP_CHECK(obj) ELM_FLIP_INTERACTION_NONE;
   ELM_FLIP_DATA_GET(obj, sd);

   return sd->intmode;
}

EAPI void
elm_flip_interaction_direction_enabled_set(Evas_Object *obj,
                                           Elm_Flip_Direction dir,
                                           Eina_Bool enabled)
{
   int i = -1;

   ELM_FLIP_CHECK(obj);
   ELM_FLIP_DATA_GET(obj, sd);

   enabled = !!enabled;
   if (dir == ELM_FLIP_DIRECTION_UP) i = 0;
   else if (dir == ELM_FLIP_DIRECTION_DOWN)
     i = 1;
   else if (dir == ELM_FLIP_DIRECTION_LEFT)
     i = 2;
   else if (dir == ELM_FLIP_DIRECTION_RIGHT)
     i = 3;
   if (i < 0) return;
   if (sd->dir_enabled[i] == enabled) return;
   sd->dir_enabled[i] = enabled;
   if (sd->intmode == ELM_FLIP_INTERACTION_NONE) return;
   if ((sd->dir_enabled[i]) && (!sd->event[i]))
     {
        sd->event[i] = evas_object_rectangle_add(evas_object_evas_get(obj));

        evas_object_data_set(sd->event[i], "_elm_leaveme", obj);
        evas_object_clip_set(sd->event[i], evas_object_clip_get(obj));
        evas_object_color_set(sd->event[i], 0, 0, 0, 0);
        evas_object_show(sd->event[i]);
        evas_object_smart_member_add(sd->event[i], obj);
        evas_object_event_callback_add(sd->event[i], EVAS_CALLBACK_MOUSE_DOWN,
                                       _down_cb, obj);
        evas_object_event_callback_add(sd->event[i], EVAS_CALLBACK_MOUSE_UP,
                                       _up_cb, obj);
        evas_object_event_callback_add(sd->event[i], EVAS_CALLBACK_MOUSE_MOVE,
                                       _move_cb, obj);
     }
   else if (!(sd->dir_enabled[i]) && (sd->event[i]))
     {
        evas_object_del(sd->event[i]);
        sd->event[i] = NULL;
     }
   _sizing_eval(obj);
   _configure(obj);
}

EAPI Eina_Bool
elm_flip_interaction_direction_enabled_get(Evas_Object *obj,
                                           Elm_Flip_Direction dir)
{
   int i = -1;

   ELM_FLIP_CHECK(obj) EINA_FALSE;
   ELM_FLIP_DATA_GET(obj, sd);

   if (dir == ELM_FLIP_DIRECTION_UP) i = 0;
   else if (dir == ELM_FLIP_DIRECTION_DOWN)
     i = 1;
   else if (dir == ELM_FLIP_DIRECTION_LEFT)
     i = 2;
   else if (dir == ELM_FLIP_DIRECTION_RIGHT)
     i = 3;
   if (i < 0) return EINA_FALSE;
   return sd->dir_enabled[i];
}

EAPI void
elm_flip_interaction_direction_hitsize_set(Evas_Object *obj,
                                           Elm_Flip_Direction dir,
                                           double hitsize)
{
   int i = -1;

   ELM_FLIP_CHECK(obj);
   ELM_FLIP_DATA_GET(obj, sd);

   if (dir == ELM_FLIP_DIRECTION_UP) i = 0;
   else if (dir == ELM_FLIP_DIRECTION_DOWN)
     i = 1;
   else if (dir == ELM_FLIP_DIRECTION_LEFT)
     i = 2;
   else if (dir == ELM_FLIP_DIRECTION_RIGHT)
     i = 3;
   if (i < 0) return;
   if (hitsize < 0.0) hitsize = 0.0;
   else if (hitsize > 1.0)
     hitsize = 1.0;
   if (sd->dir_hitsize[i] == hitsize) return;
   sd->dir_hitsize[i] = hitsize;
   _sizing_eval(obj);
   _configure(obj);
}

EAPI double
elm_flip_interaction_direction_hitsize_get(Evas_Object *obj,
                                           Elm_Flip_Direction dir)
{
   int i = -1;
   ELM_FLIP_CHECK(obj) EINA_FALSE;
   ELM_FLIP_DATA_GET(obj, sd);

   if (dir == ELM_FLIP_DIRECTION_UP) i = 0;
   else if (dir == ELM_FLIP_DIRECTION_DOWN)
     i = 1;
   else if (dir == ELM_FLIP_DIRECTION_LEFT)
     i = 2;
   else if (dir == ELM_FLIP_DIRECTION_RIGHT)
     i = 3;
   if (i < 0) return 0.0;
   return sd->dir_hitsize[i];
}
