#include <Elementary.h>
#include "elm_priv.h"
#include "elm_interface_scrollable.h"

static const char PAN_SMART_NAME[] = "elm_pan";
static const char SIG_SCROLL_ANIM_CANCEL[] = "scroll,anim,cancel";


#define ELM_PAN_DATA_GET_OR_RETURN(o, ptr)                      \
  Elm_Pan_Smart_Data *ptr = evas_object_smart_data_get(o);      \
  if (!ptr)                                                     \
    {                                                           \
       CRITICAL("No smart data for object %p (%s)",             \
                o, evas_object_type_get(o));                    \
       return;                                                  \
    }

#define ELM_PAN_DATA_GET_OR_RETURN_VAL(o, ptr, val)		\
  Elm_Pan_Smart_Data *ptr = evas_object_smart_data_get(o);	\
  if (!ptr)							\
    {								\
       CRITICAL("No smart data for object %p (%s)",		\
                o, evas_object_type_get(o));			\
       return val;						\
    }

static const char SIG_CHANGED[] = "changed";
static const Evas_Smart_Cb_Description _smart_callbacks[] = {
   {SIG_CHANGED, ""},
   {NULL, NULL}
};

ELM_INTERNAL_SMART_SUBCLASS_NEW
  (PAN_SMART_NAME, _elm_pan, Elm_Pan_Smart_Class, Evas_Smart_Class,
  evas_object_smart_clipped_class_get, _smart_callbacks);

static void _elm_pan_content_set(Evas_Object *, Evas_Object *);
static void
_elm_scroll_scroll_to_x(Elm_Scrollable_Smart_Interface_Data *sid,
                        double t_in,
                        Evas_Coord pos_x);
static void
_elm_scroll_scroll_to_y(Elm_Scrollable_Smart_Interface_Data *sid,
                        double t_in,
                        Evas_Coord pos_y);
static void
_elm_scroll_current_page_get(const Evas_Object *obj,
                             int *pagenumber_h,
                             int *pagenumber_v);
static void
_elm_scroll_wanted_coordinates_update(Elm_Scrollable_Smart_Interface_Data *sid,
                                      Evas_Coord x,
                                      Evas_Coord y);
EAPI const Elm_Pan_Smart_Class *
elm_pan_smart_class_get(void)
{
   static Elm_Pan_Smart_Class _sc =
     ELM_PAN_SMART_CLASS_INIT_NAME_VERSION(PAN_SMART_NAME);
   static const Elm_Pan_Smart_Class *class = NULL;
   Evas_Smart_Class *esc = (Evas_Smart_Class *)&_sc;

   if (class)
     return class;

   _elm_pan_smart_set(&_sc);
   esc->callbacks = _smart_callbacks;
   class = &_sc;

   return class;
}

static double
_round(double value, int pos)
{
   double temp;

   temp = value * pow( 10, pos );
   temp = floor( temp + 0.5 );
   temp *= pow( 10, -pos );

   return temp;
}

static void
_elm_pan_update(Elm_Pan_Smart_Data *psd)
{
   if (!psd->gravity_x && !psd->gravity_y)
     {
        evas_object_move(psd->content, psd->x - psd->px, psd->y - psd->py);
        return;
     }

   if ((!psd->px) && (!psd->py))
     {
        psd->px = psd->delta_posx * psd->gravity_x;
        psd->py = psd->delta_posy * psd->gravity_y;
     }
   psd->delta_posx += psd->content_w - psd->prev_cw;
   psd->prev_cw = psd->content_w;
   psd->delta_posy += psd->content_h - psd->prev_ch;
   psd->prev_ch = psd->content_h;

   evas_object_move(psd->content, psd->x - psd->px, psd->y - psd->py);
   psd->px = psd->delta_posx * psd->gravity_x;
   psd->py = psd->delta_posy * psd->gravity_y;
}

static void
_elm_pan_smart_add(Evas_Object *obj)
{
   const Evas_Smart_Class *sc;
   const Evas_Smart *smart;

   EVAS_SMART_DATA_ALLOC(obj, Elm_Pan_Smart_Data);

   _elm_pan_parent_sc->add(obj);

   priv->self = obj;

   priv->x = 0;
   priv->y = 0;
   priv->w = 0;
   priv->h = 0;
   priv->gravity_x = 0.0;
   priv->gravity_y = 0.0;

   smart = evas_object_smart_smart_get(obj);
   sc = evas_smart_class_get(smart);
   priv->api = (const Elm_Pan_Smart_Class *)sc;
}

static void
_elm_pan_smart_del(Evas_Object *obj)
{
   _elm_pan_content_set(obj, NULL);

   _elm_pan_parent_sc->del(obj);
}

static void
_elm_pan_smart_move(Evas_Object *obj,
                    Evas_Coord x,
                    Evas_Coord y)
{
   ELM_PAN_DATA_GET_OR_RETURN(obj, psd);

   /* we don't want the clipped smart object version here */

   psd->x = x;
   psd->y = y;

   _elm_pan_update(psd);
}

static void
_elm_pan_smart_resize(Evas_Object *obj,
                      Evas_Coord w,
                      Evas_Coord h)
{
   ELM_PAN_DATA_GET_OR_RETURN(obj, psd);

   psd->w = w;
   psd->h = h;

   _elm_pan_update(psd);
   evas_object_smart_callback_call(psd->self, SIG_CHANGED, NULL);
}

static void
_elm_pan_smart_show(Evas_Object *obj)
{
   ELM_PAN_DATA_GET_OR_RETURN(obj, psd);

   _elm_pan_parent_sc->show(obj);

   if (psd->content)
     evas_object_show(psd->content);
}

static void
_elm_pan_smart_hide(Evas_Object *obj)
{
   ELM_PAN_DATA_GET_OR_RETURN(obj, psd);

   _elm_pan_parent_sc->hide(obj);

   if (psd->content)
     evas_object_hide(psd->content);
}

static void
_elm_pan_pos_set(Evas_Object *obj,
                 Evas_Coord x,
                 Evas_Coord y)
{
   ELM_PAN_DATA_GET_OR_RETURN(obj, psd);

   if ((x == psd->px) && (y == psd->py)) return;
   psd->px = x;
   psd->py = y;

   _elm_pan_update(psd);
   evas_object_smart_callback_call(psd->self, SIG_CHANGED, NULL);
}

static void
_elm_pan_pos_get(const Evas_Object *obj,
                 Evas_Coord *x,
                 Evas_Coord *y)
{
   ELM_PAN_DATA_GET_OR_RETURN(obj, psd);

   if (x) *x = psd->px;
   if (y) *y = psd->py;
}

static void
_elm_pan_pos_max_get(const Evas_Object *obj,
                     Evas_Coord *x,
                     Evas_Coord *y)
{
   ELM_PAN_DATA_GET_OR_RETURN(obj, psd);

   if (x)
     {
        if (psd->w < psd->content_w) *x = psd->content_w - psd->w;
        else *x = 0;
     }
   if (y)
     {
        if (psd->h < psd->content_h) *y = psd->content_h - psd->h;
        else *y = 0;
     }
}

static void
_elm_pan_pos_min_get(const Evas_Object *obj __UNUSED__,
                     Evas_Coord *x,
                     Evas_Coord *y)
{
   if (x)
     *x = 0;
   if (y)
     *y = 0;
}

static void
_elm_pan_content_size_get(const Evas_Object *obj,
                          Evas_Coord *w,
                          Evas_Coord *h)
{
   ELM_PAN_DATA_GET_OR_RETURN(obj, psd);

   if (w) *w = psd->content_w;
   if (h) *h = psd->content_h;
}

static void
_elm_pan_gravity_set(Evas_Object *obj,
                     double x,
                     double y)
{
   ELM_PAN_DATA_GET_OR_RETURN(obj, psd);

   psd->gravity_x = x;
   psd->gravity_y = y;
   psd->prev_cw = psd->content_w;
   psd->prev_ch = psd->content_h;
   psd->delta_posx = 0;
   psd->delta_posy = 0;
}

static void
_elm_pan_gravity_get(const Evas_Object *obj,
                     double *x,
                     double *y)
{
   ELM_PAN_DATA_GET_OR_RETURN(obj, psd);

   if (x) *x = psd->gravity_x;
   if (y) *y = psd->gravity_y;
}

static void
_elm_pan_smart_set_user(Elm_Pan_Smart_Class *sc)
{
   sc->base.add = _elm_pan_smart_add;
   sc->base.del = _elm_pan_smart_del;
   sc->base.move = _elm_pan_smart_move;
   sc->base.resize = _elm_pan_smart_resize;
   sc->base.show = _elm_pan_smart_show;
   sc->base.hide = _elm_pan_smart_hide;
   sc->pos_set = _elm_pan_pos_set;
   sc->pos_get = _elm_pan_pos_get;
   sc->pos_max_get = _elm_pan_pos_max_get;
   sc->pos_min_get = _elm_pan_pos_min_get;
   sc->content_size_get = _elm_pan_content_size_get;
   sc->gravity_set = _elm_pan_gravity_set;
   sc->gravity_get = _elm_pan_gravity_get;
   sc->pos_adjust = NULL;
}

static Evas_Object *
_elm_pan_add(Evas *evas)
{
   return evas_object_smart_add(evas, _elm_pan_smart_class_new());
}

static void
_elm_pan_content_del_cb(void *data,
                        Evas *e __UNUSED__,
                        Evas_Object *obj __UNUSED__,
                        void *event_info __UNUSED__)
{
   Elm_Pan_Smart_Data *psd;

   psd = data;
   psd->content = NULL;
   psd->content_w = psd->content_h = psd->px = psd->py =
           psd->prev_cw = psd->prev_ch = psd->delta_posx = psd->delta_posy = 0;
   evas_object_smart_callback_call(psd->self, SIG_CHANGED, NULL);
}

static void
_elm_pan_content_resize_cb(void *data,
                           Evas *e __UNUSED__,
                           Evas_Object *obj __UNUSED__,
                           void *event_info __UNUSED__)
{
   Elm_Pan_Smart_Data *psd;
   Evas_Coord w, h;

   psd = data;
   evas_object_geometry_get(psd->content, NULL, NULL, &w, &h);
   if ((w != psd->content_w) || (h != psd->content_h))
     {
        psd->content_w = w;
        psd->content_h = h;
        _elm_pan_update(psd);
     }
   evas_object_smart_callback_call(psd->self, SIG_CHANGED, NULL);
}

static void
_elm_pan_content_set(Evas_Object *obj,
                     Evas_Object *content)
{
   Evas_Coord w, h;

   ELM_PAN_DATA_GET_OR_RETURN(obj, psd);

   if (content == psd->content) return;
   if (psd->content)
     {
        evas_object_smart_member_del(psd->content);
        evas_object_event_callback_del_full
          (psd->content, EVAS_CALLBACK_DEL, _elm_pan_content_del_cb, psd);
        evas_object_event_callback_del_full
          (psd->content, EVAS_CALLBACK_RESIZE, _elm_pan_content_resize_cb,
          psd);
        psd->content = NULL;
     }
   if (!content) goto end;

   psd->content = content;
   evas_object_smart_member_add(psd->content, psd->self);
   evas_object_geometry_get(psd->content, NULL, NULL, &w, &h);
   psd->content_w = w;
   psd->content_h = h;
   evas_object_event_callback_add
     (content, EVAS_CALLBACK_DEL, _elm_pan_content_del_cb, psd);
   evas_object_event_callback_add
     (content, EVAS_CALLBACK_RESIZE, _elm_pan_content_resize_cb, psd);

   if (evas_object_visible_get(psd->self))
     evas_object_show(psd->content);
   else
     evas_object_hide(psd->content);

   _elm_pan_update(psd);

end:
   evas_object_smart_callback_call(psd->self, SIG_CHANGED, NULL);
}

/* pan smart object on top, scroller interface on bottom */
/* ============================================================ */

static const char SCROLL_SMART_NAME[] = "elm_scroll";

#define ELM_SCROLL_IFACE_DATA_GET_OR_RETURN(o, ptr)                     \
  Elm_Scrollable_Smart_Interface_Data *ptr =                            \
    evas_object_smart_interface_data_get(o, &(ELM_SCROLLABLE_IFACE.base)); \
  if (!ptr)                                                             \
    {                                                                   \
       CRITICAL("No interface data for object %p (%s)",                 \
                o, evas_object_type_get(o));                            \
       return;                                                          \
    }

#define ELM_SCROLL_IFACE_DATA_GET_OR_RETURN_VAL(o, ptr, val)            \
  Elm_Scrollable_Smart_Interface_Data *ptr =                            \
    evas_object_smart_interface_data_get(o, &(ELM_SCROLLABLE_IFACE.base)); \
  if (!ptr)                                                             \
    {                                                                   \
       CRITICAL("No interface data for object %p (%s)",                 \
                o, evas_object_type_get(o));                            \
       return val;                                                      \
    }

static void _elm_scroll_scroll_bar_size_adjust(
  Elm_Scrollable_Smart_Interface_Data *);
static void _elm_scroll_wanted_region_set(Evas_Object *);
static Eina_Bool _paging_is_enabled(Elm_Scrollable_Smart_Interface_Data *sid);
static Evas_Coord _elm_scroll_page_x_get(
   Elm_Scrollable_Smart_Interface_Data *sid, int offset, Eina_Bool limit);
static Evas_Coord _elm_scroll_page_y_get(
   Elm_Scrollable_Smart_Interface_Data *sid, int offset, Eina_Bool limit);
static void _elm_scroll_content_pos_get(const Evas_Object *,
                                        Evas_Coord *,
                                        Evas_Coord *);
static void _elm_scroll_content_pos_set(Evas_Object *,
                                        Evas_Coord,
                                        Evas_Coord,
                                        Eina_Bool);

#define LEFT               0
#define RIGHT              1
#define UP                 2
#define DOWN               3
#define EVTIME             1
//#define SCROLLDBG 1
/* smoothness debug calls - for debugging how much smooth your app is */
#define SMOOTHDBG          1

#ifdef SMOOTHDBG
#define SMOOTH_DEBUG_COUNT 100

#define FPS                1 / 60
typedef struct _smooth_debug_info smooth_debug_info;
struct _smooth_debug_info
{
   double     t;
   double     dt;
   Evas_Coord pos;
   Evas_Coord dpos;
   double     vpos;
};

static smooth_debug_info smooth_x_history[SMOOTH_DEBUG_COUNT];
static smooth_debug_info smooth_y_history[SMOOTH_DEBUG_COUNT];
static int smooth_info_x_count = 0;
static int smooth_info_y_count = 0;
static double start_time = 0;
static int _elm_scroll_smooth_debug = 0;


void
_elm_scroll_smooth_debug_init(void)
{
   start_time = ecore_time_get();
   smooth_info_x_count = 0;
   smooth_info_y_count = 0;

   memset(&(smooth_x_history[0]), 0,
          sizeof(smooth_x_history[0]) * SMOOTH_DEBUG_COUNT);
   memset(&(smooth_y_history[0]), 0,
          sizeof(smooth_y_history[0]) * SMOOTH_DEBUG_COUNT);

   return;
}

void
_elm_scroll_smooth_debug_shutdown(void)
{
   int i = 0;
   int info_x_count = 0;
   int info_y_count = 0;
   double x_ave = 0, y_ave = 0;
   double x_sum = 0, y_sum = 0;
   double x_dev = 0, y_dev = 0;
   double x_dev_sum = 0, y_dev_sum = 0;

   if (smooth_info_x_count >= SMOOTH_DEBUG_COUNT)
     info_x_count = SMOOTH_DEBUG_COUNT;
   else
     info_x_count = smooth_info_x_count;

   if (smooth_info_y_count >= SMOOTH_DEBUG_COUNT)
     info_y_count = SMOOTH_DEBUG_COUNT;
   else
     info_y_count = smooth_info_y_count;

   DBG("\n\n<<< X-axis Smoothness >>>\n");
   DBG("| Num  | t(time)  | dt       | x    | dx   |vx(dx/1fps) |\n");

   for (i = info_x_count - 1; i >= 0; i--)
     {
        DBG("| %4d | %1.6f | %1.6f | %4d | %4d | %9.3f |\n", info_x_count - i,
            smooth_x_history[i].t,
            smooth_x_history[i].dt,
            smooth_x_history[i].pos,
            smooth_x_history[i].dpos,
            smooth_x_history[i].vpos);
        if (i == info_x_count - 1) continue;
        x_sum += smooth_x_history[i].vpos;
     }

   x_ave = x_sum / (info_x_count - 1);
   for (i = 0; i < info_x_count - 1; i++)
     {
        x_dev_sum += (smooth_x_history[i].vpos - x_ave) *
          (smooth_x_history[i].vpos - x_ave);
     }
   x_dev = x_dev_sum / (info_x_count - 1);
   DBG(" Standard deviation of X-axis velocity: %9.3f\n", sqrt(x_dev));

   DBG("\n\n<<< Y-axis Smoothness >>>\n");
   DBG("| Num  | t(time)  | dt       | y    |  dy  |vy(dy/1fps) |\n");
   for (i = info_y_count - 1; i >= 0; i--)
     {
        DBG("| %4d | %1.6f | %1.6f | %4d | %4d | %9.3f |\n", info_y_count - i,
            smooth_y_history[i].t,
            smooth_y_history[i].dt,
            smooth_y_history[i].pos,
            smooth_y_history[i].dpos,
            smooth_y_history[i].vpos);
        if (i == info_y_count - 1) continue;
        y_sum += smooth_y_history[i].vpos;
     }
   y_ave = y_sum / (info_y_count - 1);
   for (i = 0; i < info_y_count - 1; i++)
     {
        y_dev_sum += (smooth_y_history[i].vpos - y_ave) *
          (smooth_y_history[i].vpos - y_ave);
     }
   y_dev = y_dev_sum / (info_y_count - 1);

   DBG(" Standard deviation of Y-axis velocity: %9.3f\n", sqrt(y_dev));
}

static void
_elm_direction_arrows_eval(Elm_Scrollable_Smart_Interface_Data *sid)
{
   Eina_Bool go_left = EINA_TRUE, go_right = EINA_TRUE;
   Eina_Bool go_up = EINA_TRUE, go_down = EINA_TRUE;
   Evas_Coord x = 0, y = 0, mx = 0, my = 0, minx = 0, miny = 0;
   
   if (!sid->edje_obj || !sid->pan_obj) return;
   ELM_PAN_DATA_GET_OR_RETURN(sid->pan_obj, psd);
   
   psd->api->pos_max_get(sid->pan_obj, &mx, &my);
   psd->api->pos_min_get(sid->pan_obj, &minx, &miny);
   psd->api->pos_get(sid->pan_obj, &x, &y);

   if (x == minx) go_left = EINA_FALSE;
   if (x == (mx + minx)) go_right = EINA_FALSE;
   if (y == miny) go_up = EINA_FALSE;
   if (y == (my + miny)) go_down = EINA_FALSE;
   if (go_left != sid->go_left)
     {
        if (go_left)
          edje_object_signal_emit(sid->edje_obj, "elm,action,show,left", "elm");
        else
          edje_object_signal_emit(sid->edje_obj, "elm,action,hide,left", "elm");
        sid->go_left = go_left;
     }
   if (go_right != sid->go_right)
     {
        if (go_right)
          edje_object_signal_emit(sid->edje_obj, "elm,action,show,right", "elm");
        else
          edje_object_signal_emit(sid->edje_obj, "elm,action,hide,right", "elm");
        sid->go_right= go_right;
     }
   if (go_up != sid->go_up)
     {
        if (go_up)
          edje_object_signal_emit(sid->edje_obj, "elm,action,show,up", "elm");
        else
          edje_object_signal_emit(sid->edje_obj, "elm,action,hide,up", "elm");
        sid->go_up = go_up;
     }
   if (go_down != sid->go_down)
     {
        if (go_down)
          edje_object_signal_emit(sid->edje_obj, "elm,action,show,down", "elm");
        else
          edje_object_signal_emit(sid->edje_obj, "elm,action,hide,down", "elm");
        sid->go_down= go_down;
     }
}

void
_elm_scroll_smooth_debug_movetime_add(int x,
                                      int y)
{
   double tim = 0;
   static int bx = 0;
   static int by = 0;

   tim = ecore_time_get();

   if (bx != x)
     {
        smooth_info_x_count++;
        memmove(&(smooth_x_history[1]), &(smooth_x_history[0]),
                sizeof(smooth_x_history[0]) * (SMOOTH_DEBUG_COUNT - 1));
        smooth_x_history[0].t = tim - start_time;
        smooth_x_history[0].dt = smooth_x_history[0].t - smooth_x_history[1].t;
        smooth_x_history[0].pos = x;
        smooth_x_history[0].dpos =
          smooth_x_history[0].pos - smooth_x_history[1].pos;

        if (smooth_x_history[0].dpos >= 0)
          smooth_x_history[0].vpos = (double)(smooth_x_history[0].dpos) /
            smooth_x_history[0].dt * FPS;
        else
          smooth_x_history[0].vpos = -((double)(smooth_x_history[0].dpos) /
                                       smooth_x_history[0].dt * FPS);
     }

   if (by != y)
     {
        smooth_info_y_count++;
        memmove(&(smooth_y_history[1]), &(smooth_y_history[0]),
                sizeof(smooth_y_history[0]) * (SMOOTH_DEBUG_COUNT - 1));
        smooth_y_history[0].t = tim - start_time;
        smooth_y_history[0].dt = smooth_y_history[0].t - smooth_y_history[1].t;
        smooth_y_history[0].pos = y;
        smooth_y_history[0].dpos = smooth_y_history[0].pos -
          smooth_y_history[1].pos;

        if (smooth_y_history[0].dpos >= 0)
          smooth_y_history[0].vpos = (double)(smooth_y_history[0].dpos) /
            smooth_y_history[0].dt * FPS;
        else
          smooth_y_history[0].vpos = -((double)(smooth_y_history[0].dpos) /
                                       smooth_y_history[0].dt * FPS);
     }

   bx = x;
   by = y;
}

#endif

static int
_elm_scroll_scroll_bar_h_visibility_adjust(
  Elm_Scrollable_Smart_Interface_Data *sid)
{
   int scroll_h_vis_change = 0;
   Evas_Coord w, vw = 0, vh = 0;

   if (!sid->edje_obj) return 0;

   w = sid->content_info.w;
   if (sid->pan_obj)
     evas_object_geometry_get(sid->pan_obj, NULL, NULL, &vw, &vh);
   if (sid->hbar_visible)
     {
       if (sid->hbar_flags == ELM_SCROLLER_POLICY_AUTO)
         {
            if ((sid->content) || (sid->extern_pan))
              {
                 if (w <= vw)
                   {
                      scroll_h_vis_change = 1;
                      sid->hbar_visible = EINA_FALSE;
                    }
              }
            else
              {
                 scroll_h_vis_change = 1;
                 sid->hbar_visible = EINA_FALSE;
              }
         }
         else if (sid->hbar_flags == ELM_SCROLLER_POLICY_OFF)
           {
               scroll_h_vis_change = 1;
               sid->hbar_visible = EINA_FALSE;
            }
       }
   else
     {
          if (sid->hbar_flags == ELM_SCROLLER_POLICY_AUTO)
            {
               if ((sid->content) || (sid->extern_pan))
                 {
                    if (w > vw)
                      {
                         scroll_h_vis_change = 1;
                         sid->hbar_visible = EINA_TRUE;
                      }
                 }
            }
          else if (sid->hbar_flags == ELM_SCROLLER_POLICY_ON)
            {
               scroll_h_vis_change = 1;
               sid->hbar_visible = EINA_TRUE;
            }
      }
   if (scroll_h_vis_change)
     {
        if (sid->hbar_flags != ELM_SCROLLER_POLICY_OFF)
          {
             if (sid->hbar_visible)
               edje_object_signal_emit
                 (sid->edje_obj, "elm,action,show,hbar", "elm");
             else
               edje_object_signal_emit
                 (sid->edje_obj, "elm,action,hide,hbar", "elm");
          }
        else
          edje_object_signal_emit
            (sid->edje_obj, "elm,action,hide,hbar", "elm");
        edje_object_message_signal_process(sid->edje_obj);
        _elm_scroll_scroll_bar_size_adjust(sid);
        if (sid->cb_func.content_min_limit)
          sid->cb_func.content_min_limit(sid->obj, sid->min_w, sid->min_h);
     }
   
   _elm_direction_arrows_eval(sid);
   return scroll_h_vis_change;
}

static int
_elm_scroll_scroll_bar_v_visibility_adjust(
  Elm_Scrollable_Smart_Interface_Data *sid)
{
   int scroll_v_vis_change = 0;
   Evas_Coord h, vw = 0, vh = 0;

   if (!sid->edje_obj) return 0;

   h = sid->content_info.h;
   if (sid->pan_obj)
     evas_object_geometry_get(sid->pan_obj, NULL, NULL, &vw, &vh);
   if (sid->vbar_visible)
     {
         if (sid->vbar_flags == ELM_SCROLLER_POLICY_AUTO)
           {
              if ((sid->content) || (sid->extern_pan))
                {
                   if (h <= vh)
                     {
                        scroll_v_vis_change = 1;
                        sid->vbar_visible = EINA_FALSE;
                      }
                 }
              else
                {
                    scroll_v_vis_change = 1;
                    sid->vbar_visible = EINA_FALSE;
                 }
            }
        else if (sid->vbar_flags == ELM_SCROLLER_POLICY_OFF)
          {
             scroll_v_vis_change = 1;
             sid->vbar_visible = EINA_FALSE;
          }
     }
   else
     {
         if (sid->vbar_flags == ELM_SCROLLER_POLICY_AUTO)
           {
              if ((sid->content) || (sid->extern_pan))
                {
                   if (h > vh)
                     {
                        scroll_v_vis_change = 1;
                        sid->vbar_visible = EINA_TRUE;
                     }
                }
           }
         else if (sid->vbar_flags == ELM_SCROLLER_POLICY_ON)
           {
              scroll_v_vis_change = 1;
              sid->vbar_visible = EINA_TRUE;
           }
     }
   if (scroll_v_vis_change)
     {
        if (sid->vbar_flags != ELM_SCROLLER_POLICY_OFF)
          {
             if (sid->vbar_visible)
               edje_object_signal_emit
                 (sid->edje_obj, "elm,action,show,vbar", "elm");
             else
               edje_object_signal_emit
                 (sid->edje_obj, "elm,action,hide,vbar", "elm");
          }
        else
          edje_object_signal_emit
            (sid->edje_obj, "elm,action,hide,vbar", "elm");
        edje_object_message_signal_process(sid->edje_obj);
        _elm_scroll_scroll_bar_size_adjust(sid);
        if (sid->cb_func.content_min_limit)
          sid->cb_func.content_min_limit(sid->obj, sid->min_w, sid->min_h);
     }

   _elm_direction_arrows_eval(sid);
   return scroll_v_vis_change;
}

static void
_elm_scroll_scroll_bar_visibility_adjust(
  Elm_Scrollable_Smart_Interface_Data *sid)
{
   int changed = 0;

   changed |= _elm_scroll_scroll_bar_h_visibility_adjust(sid);
   changed |= _elm_scroll_scroll_bar_v_visibility_adjust(sid);

   if (changed)
     {
        _elm_scroll_scroll_bar_h_visibility_adjust(sid);
        _elm_scroll_scroll_bar_v_visibility_adjust(sid);
     }
}

static void
_elm_scroll_scroll_bar_size_adjust(Elm_Scrollable_Smart_Interface_Data *sid)
{
   if (!sid->pan_obj || !sid->edje_obj) return;

   ELM_PAN_DATA_GET_OR_RETURN(sid->pan_obj, psd);

   if ((sid->content) || (sid->extern_pan))
     {
        Evas_Coord x, y, w, h, mx = 0, my = 0, vw = 0, vh = 0, px, py,
                   minx = 0, miny = 0;
        double vx, vy, size;

        edje_object_calc_force(sid->edje_obj);
        edje_object_part_geometry_get
          (sid->edje_obj, "elm.swallow.content", NULL, NULL, &vw, &vh);
        w = sid->content_info.w;
        if (w < 1) w = 1;
        size = (double)vw / (double)w;

        if (size > 1.0)
          {
             size = 1.0;
             edje_object_part_drag_value_set
               (sid->edje_obj, "elm.dragable.hbar", 0.0, 0.0);
          }
        edje_object_part_drag_size_set
          (sid->edje_obj, "elm.dragable.hbar", size, 1.0);

        h = sid->content_info.h;
        if (h < 1) h = 1;
        size = (double)vh / (double)h;
        if (size > 1.0)
          {
             size = 1.0;
             edje_object_part_drag_value_set
               (sid->edje_obj, "elm.dragable.vbar", 0.0, 0.0);
          }
        edje_object_part_drag_size_set
          (sid->edje_obj, "elm.dragable.vbar", 1.0, size);

        edje_object_part_drag_value_get
          (sid->edje_obj, "elm.dragable.hbar", &vx, NULL);
        edje_object_part_drag_value_get
          (sid->edje_obj, "elm.dragable.vbar", NULL, &vy);

        psd->api->pos_max_get(sid->pan_obj, &mx, &my);
        psd->api->pos_min_get(sid->pan_obj, &minx, &miny);
        x = vx * mx + minx;
        y = vy * my + miny;

        edje_object_part_drag_step_set
          (sid->edje_obj, "elm.dragable.hbar", (double)sid->step.x /
          (double)w, 0.0);
        edje_object_part_drag_step_set
          (sid->edje_obj, "elm.dragable.vbar", 0.0, (double)sid->step.y /
          (double)h);
        if (sid->page.x > 0)
          edje_object_part_drag_page_set
            (sid->edje_obj, "elm.dragable.hbar", (double)sid->page.x /
            (double)w, 0.0);
        else
          edje_object_part_drag_page_set
            (sid->edje_obj, "elm.dragable.hbar",
            -((double)sid->page.x * ((double)vw / (double)w)) / 100.0, 0.0);
        if (sid->page.y > 0)
          edje_object_part_drag_page_set
            (sid->edje_obj, "elm.dragable.vbar", 0.0,
            (double)sid->page.y / (double)h);
        else
          edje_object_part_drag_page_set
            (sid->edje_obj, "elm.dragable.vbar", 0.0,
            -((double)sid->page.y * ((double)vh / (double)h)) / 100.0);

        psd->api->pos_get(sid->pan_obj, &px, &py);
        if (vx != mx) x = px;
        if (vy != my) y = py;
        psd->api->pos_set(sid->pan_obj, x, y);
     }
   else
     {
        Evas_Coord px = 0, py = 0, minx = 0, miny = 0;

        edje_object_part_drag_size_set
          (sid->edje_obj, "elm.dragable.vbar", 1.0, 1.0);
        edje_object_part_drag_size_set
          (sid->edje_obj, "elm.dragable.hbar", 1.0, 1.0);
        psd->api->pos_min_get(sid->pan_obj, &minx, &miny);
        psd->api->pos_get(sid->pan_obj, &px, &py);
        psd->api->pos_set(sid->pan_obj, minx, miny);
        if ((px != minx) || (py != miny))
          edje_object_signal_emit(sid->edje_obj, "elm,action,scroll", "elm");
     }
   _elm_scroll_scroll_bar_visibility_adjust(sid);
}

static void
_elm_scroll_scroll_bar_read_and_update(
  Elm_Scrollable_Smart_Interface_Data *sid)
{
   Evas_Coord x, y, mx = 0, my = 0, px, py, minx = 0, miny = 0;
   double vx, vy;

   if (!sid->edje_obj || !sid->pan_obj) return;

   ELM_PAN_DATA_GET_OR_RETURN(sid->pan_obj, psd);

   if ((sid->down.dragged) || (sid->down.bounce_x_animator)
       || (sid->down.bounce_y_animator) || (sid->down.momentum_animator)
       || (sid->scrollto.x.animator) || (sid->scrollto.y.animator))
     return;
   edje_object_part_drag_value_get
     (sid->edje_obj, "elm.dragable.vbar", NULL, &vy);
   edje_object_part_drag_value_get
     (sid->edje_obj, "elm.dragable.hbar", &vx, NULL);
   psd->api->pos_max_get(sid->pan_obj, &mx, &my);
   psd->api->pos_min_get(sid->pan_obj, &minx, &miny);
   x = _round(vx * (double)mx + minx, 1);
   y = _round(vy * (double)my + miny, 1);
   psd->api->pos_get(sid->pan_obj, &px, &py);

  if (!sid->is_rotary_event_enable)
  {
     if (!sid->freeze && _paging_is_enabled(sid))
     {
        x = _elm_scroll_page_x_get(sid, 0, EINA_FALSE);
        y = _elm_scroll_page_y_get(sid, 0, EINA_FALSE);
     }
  }
   psd->api->pos_set(sid->pan_obj, x, y);
   if ((px != x) || (py != y))
     {
        edje_object_signal_emit(sid->edje_obj, "elm,action,scroll", "elm");
     }
}

static void
_elm_scroll_drag_start(Elm_Scrollable_Smart_Interface_Data *sid)
{
   if (sid->cb_func.drag_start)
     sid->cb_func.drag_start(sid->obj, NULL);
}

static void
_elm_scroll_drag_stop(Elm_Scrollable_Smart_Interface_Data *sid)
{
   sid->current_page.x = _elm_scroll_page_x_get(sid, 0, EINA_FALSE);
   sid->current_page.y = _elm_scroll_page_y_get(sid, 0, EINA_FALSE);

   if (sid->cb_func.drag_stop)
     sid->cb_func.drag_stop(sid->obj, NULL);
}

static void
_elm_scroll_anim_start(Elm_Scrollable_Smart_Interface_Data *sid)
{
   if (sid->cb_func.animate_start)
     sid->cb_func.animate_start(sid->obj, NULL);
}

static void
_elm_scroll_anim_cancel(Elm_Scrollable_Smart_Interface_Data *sid)
{
   sid->current_page.x = _elm_scroll_page_x_get(sid, 0, EINA_FALSE);
   sid->current_page.y = _elm_scroll_page_y_get(sid, 0, EINA_FALSE);
   sid->is_unset_cb_called = EINA_FALSE;

   evas_object_smart_callback_call(sid->obj, SIG_SCROLL_ANIM_CANCEL, NULL);
}

static void
_elm_scroll_anim_stop(Elm_Scrollable_Smart_Interface_Data *sid, Eina_Bool force)
{
   sid->current_page.x = _elm_scroll_page_x_get(sid, 0, EINA_FALSE);
   sid->current_page.y = _elm_scroll_page_y_get(sid, 0, EINA_FALSE);

   if (sid->cb_func.animate_stop)
     sid->cb_func.animate_stop(sid->obj, NULL);

   if (!force && _elm_config->scroll_item_align_enable)
     {
        if (sid->cb_func.item_scroll_align_end_set)
           sid->cb_func.item_scroll_align_end_set(sid->obj, NULL);
        sid->is_unset_cb_called = EINA_FALSE;
     }
}

static void
_elm_scroll_policy_signal_emit(Elm_Scrollable_Smart_Interface_Data *sid)
{
   if (sid->hbar_flags == ELM_SCROLLER_POLICY_ON)
     edje_object_signal_emit
       (sid->edje_obj, "elm,action,show_always,hbar", "elm");
   else if (sid->hbar_flags == ELM_SCROLLER_POLICY_OFF)
     edje_object_signal_emit
       (sid->edje_obj, "elm,action,hide,hbar", "elm");
   else
     edje_object_signal_emit
       (sid->edje_obj, "elm,action,show_notalways,hbar", "elm");
   if (sid->vbar_flags == ELM_SCROLLER_POLICY_ON)
     edje_object_signal_emit
       (sid->edje_obj, "elm,action,show_always,vbar", "elm");
   else if (sid->vbar_flags == ELM_SCROLLER_POLICY_OFF)
     edje_object_signal_emit
       (sid->edje_obj, "elm,action,hide,vbar", "elm");
   else
     edje_object_signal_emit
       (sid->edje_obj, "elm,action,show_notalways,vbar", "elm");
   edje_object_message_signal_process(sid->edje_obj);
   _elm_scroll_scroll_bar_size_adjust(sid);
}

static void
_elm_scroll_reload_cb(void *data,
                      Evas_Object *obj __UNUSED__,
                      const char *emission __UNUSED__,
                      const char *source __UNUSED__)
{
   Elm_Scrollable_Smart_Interface_Data *sid = data;

   // FIXME: refactoring for determining bar's size and visiblity
   sid->hbar_visible = EINA_TRUE;
   sid->vbar_visible = EINA_TRUE;

   _elm_scroll_policy_signal_emit(sid);
}

static void
_elm_scroll_vbar_drag_cb(void *data,
                           Evas_Object *obj __UNUSED__,
                           const char *emission __UNUSED__,
                           const char *source __UNUSED__)
{
   Elm_Scrollable_Smart_Interface_Data *sid = data;

   if (sid->cb_func.vbar_drag)
     sid->cb_func.vbar_drag(sid->obj, NULL);

   _elm_scroll_scroll_bar_read_and_update(sid);
}

static void
_elm_scroll_vbar_press_cb(void *data,
                           Evas_Object *obj __UNUSED__,
                           const char *emission __UNUSED__,
                           const char *source __UNUSED__)
{
   Elm_Scrollable_Smart_Interface_Data *sid = data;

   if (sid->cb_func.vbar_press)
     sid->cb_func.vbar_press(sid->obj, NULL);
}

static void
_elm_scroll_vbar_unpress_cb(void *data,
                           Evas_Object *obj __UNUSED__,
                           const char *emission __UNUSED__,
                           const char *source __UNUSED__)
{
   Elm_Scrollable_Smart_Interface_Data *sid = data;

   if (sid->cb_func.vbar_unpress)
     sid->cb_func.vbar_unpress(sid->obj, NULL);
}

static void
_elm_scroll_edje_drag_v_start_cb(void *data,
                                 Evas_Object *obj __UNUSED__,
                                 const char *emission __UNUSED__,
                                 const char *source __UNUSED__)
{
   Elm_Scrollable_Smart_Interface_Data *sid = data;

   _elm_scroll_scroll_bar_read_and_update(sid);
   _elm_scroll_drag_start(sid);
   sid->freeze = EINA_TRUE;
}

static void
_elm_scroll_edje_drag_v_stop_cb(void *data,
                                Evas_Object *obj __UNUSED__,
                                const char *emission __UNUSED__,
                                const char *source __UNUSED__)
{
   Elm_Scrollable_Smart_Interface_Data *sid = data;

   _elm_scroll_scroll_bar_read_and_update(sid);
   _elm_scroll_drag_stop(sid);
   sid->freeze = EINA_FALSE;
}

static void
_elm_scroll_edje_drag_v_cb(void *data,
                           Evas_Object *obj __UNUSED__,
                           const char *emission __UNUSED__,
                           const char *source __UNUSED__)
{
   Elm_Scrollable_Smart_Interface_Data *sid = data;

   _elm_scroll_scroll_bar_read_and_update(sid);
}

static void
_elm_scroll_hbar_drag_cb(void *data,
                           Evas_Object *obj __UNUSED__,
                           const char *emission __UNUSED__,
                           const char *source __UNUSED__)
{
   Elm_Scrollable_Smart_Interface_Data *sid = data;

   if (sid->cb_func.hbar_drag)
     sid->cb_func.hbar_drag(sid->obj, NULL);

   _elm_scroll_scroll_bar_read_and_update(sid);
}

static void
_elm_scroll_hbar_press_cb(void *data,
                           Evas_Object *obj __UNUSED__,
                           const char *emission __UNUSED__,
                           const char *source __UNUSED__)
{
   Elm_Scrollable_Smart_Interface_Data *sid = data;

   if (sid->cb_func.hbar_press)
     sid->cb_func.hbar_press(sid->obj, NULL);
}

static void
_elm_scroll_hbar_unpress_cb(void *data,
                           Evas_Object *obj __UNUSED__,
                           const char *emission __UNUSED__,
                           const char *source __UNUSED__)
{
   Elm_Scrollable_Smart_Interface_Data *sid = data;

   if (sid->cb_func.hbar_unpress)
     sid->cb_func.hbar_unpress(sid->obj, NULL);
}

static void
_elm_scroll_edje_drag_h_start_cb(void *data,
                                 Evas_Object *obj __UNUSED__,
                                 const char *emission __UNUSED__,
                                 const char *source __UNUSED__)
{
   Elm_Scrollable_Smart_Interface_Data *sid = data;

   _elm_scroll_scroll_bar_read_and_update(sid);
   _elm_scroll_drag_start(sid);
   sid->freeze = EINA_TRUE;
}

static void
_elm_scroll_edje_drag_h_stop_cb(void *data,
                                Evas_Object *obj __UNUSED__,
                                const char *emission __UNUSED__,
                                const char *source __UNUSED__)
{
   Elm_Scrollable_Smart_Interface_Data *sid = data;

   _elm_scroll_scroll_bar_read_and_update(sid);
   _elm_scroll_drag_stop(sid);
   sid->freeze = EINA_FALSE;
}

static void
_elm_scroll_edje_drag_h_cb(void *data,
                           Evas_Object *obj __UNUSED__,
                           const char *emission __UNUSED__,
                           const char *source __UNUSED__)
{
   Elm_Scrollable_Smart_Interface_Data *sid = data;

   _elm_scroll_scroll_bar_read_and_update(sid);
}

static void
_elm_scroll_content_size_get(const Evas_Object *obj,
                             Evas_Coord *w,
                             Evas_Coord *h)
{
   ELM_SCROLL_IFACE_DATA_GET_OR_RETURN(obj, sid);

   if (sid->content)
     evas_object_geometry_get(sid->content, NULL, NULL, w, h);
   else if (sid->pan_obj)
     {
        ELM_PAN_DATA_GET_OR_RETURN(sid->pan_obj, psd);
        psd->api->content_size_get(sid->pan_obj, w, h);
     }
}

static void
_elm_scroll_content_viewport_size_get(const Evas_Object *obj,
                                      Evas_Coord *w,
                                      Evas_Coord *h)
{
   ELM_SCROLL_IFACE_DATA_GET_OR_RETURN(obj, sid);

   if (!sid->pan_obj || !sid->edje_obj) return;

   edje_object_calc_force(sid->edje_obj);
   evas_object_geometry_get(sid->pan_obj, NULL, NULL, w, h);
}

static void
_elm_scroll_content_viewport_pos_get(const Evas_Object *obj,
                                     Evas_Coord *x,
                                     Evas_Coord *y)
{
   ELM_SCROLL_IFACE_DATA_GET_OR_RETURN(obj, sid);

   if (!sid->pan_obj || !sid->edje_obj) return;

   edje_object_calc_force(sid->edje_obj);
   evas_object_geometry_get(sid->pan_obj, x, y, NULL, NULL);
}

static void
_elm_scroll_content_min_limit(Evas_Object *obj,
                              Eina_Bool w,
                              Eina_Bool h)
{
   ELM_SCROLL_IFACE_DATA_GET_OR_RETURN(obj, sid);

   if (!sid->edje_obj) return;

   if (!sid->cb_func.content_min_limit)
     {
        ERR("Content minimim size limiting is unimplemented -- you "
            "must provide it yourself\n");
        return;
     }

   sid->min_w = !!w;
   sid->min_h = !!h;
   sid->cb_func.content_min_limit(sid->obj, w, h);
}

static void
_elm_scroll_origin_reverse_set(Evas_Object *obj,
                          Eina_Bool origin_x, Eina_Bool origin_y)
{
   Evas_Coord x, y;

   ELM_SCROLL_IFACE_DATA_GET_OR_RETURN(obj, sid);

   ERR("[DDO] obj(%x), origin_x(%d), origin_y(%d)", sid->obj, origin_x, origin_y);

   sid->origin_x = origin_x;
   sid->origin_y = origin_y;

   _elm_scroll_content_pos_get(sid->obj, &x, &y);
   _elm_scroll_wanted_coordinates_update(sid, x, y);
}

static void
_elm_scroll_origin_reverse_get(const Evas_Object *obj,
                          Eina_Bool *origin_x, Eina_Bool *origin_y)
{
   ELM_SCROLL_IFACE_DATA_GET_OR_RETURN(obj, sid);

   if (origin_x) *origin_x = sid->origin_x;
   if (origin_y) *origin_y = sid->origin_y;
}

static Evas_Coord
_elm_scroll_x_mirrored_get(const Evas_Object *obj,
                           Evas_Coord x)
{
   Evas_Coord cw, ch, w, ret;

   ELM_SCROLL_IFACE_DATA_GET_OR_RETURN_VAL(obj, sid, x);

   if (!sid->pan_obj) return 0;

   ELM_PAN_DATA_GET_OR_RETURN_VAL(sid->pan_obj, psd, 0);

   _elm_scroll_content_viewport_size_get(obj, &w, NULL);
   psd->api->content_size_get(sid->pan_obj, &cw, &ch);
   ret = (cw - (x + w));

   return (ret >= 0) ? ret : 0;
}

/* Update the wanted coordinates according to the x, y passed
 * widget directionality, content size and etc. */
static void
_elm_scroll_wanted_coordinates_update(Elm_Scrollable_Smart_Interface_Data *sid,
                                      Evas_Coord x,
                                      Evas_Coord y)
{
   Evas_Coord cw, ch;

   if (!sid->pan_obj) return;

   ELM_PAN_DATA_GET_OR_RETURN(sid->pan_obj, psd);

   psd->api->content_size_get(sid->pan_obj, &cw, &ch);

   /* Update wx/y/w/h - and if the requested positions aren't legal
    * adjust a bit. */
   _elm_scroll_content_viewport_size_get(sid->obj, &sid->ww, &sid->wh);

   if (x < 0)
     {
        if (!sid->loop_h) sid->wx = 0;
        else sid->wx = cw - sid->ww;
     }
   else if (!sid->loop_h && (x + sid->ww) > cw)
     sid->wx = cw - sid->ww;
   else if (sid->loop_h && x >= cw)
     sid->wx = 0;
   else if (sid->is_mirrored)
     sid->wx = _elm_scroll_x_mirrored_get(sid->obj, x);
   else
     sid->wx = x;

   if (sid->origin_x)
     {
        sid->wx = cw - sid->wx;
        if (sid->wx < 0) sid->wx = 0;
     }

   if (y < 0)
     {
        if (!sid->loop_v) sid->wy = 0;
        else  sid->wy = ch - sid->wh;
     }
   else if (!sid->loop_v && (y + sid->wh) > ch)
     sid->wy = ch - sid->wh;
   else if (sid->loop_v && y >= ch)
     sid->wy = 0;
   else sid->wy = y;

   if (sid->origin_y)
     {
        sid->wy = ch - sid->wy;
        if (sid->wy < 0) sid->wy = 0;
     }
}

static void
_elm_scroll_momentum_end(Elm_Scrollable_Smart_Interface_Data *sid)
{
   if ((sid->down.bounce_x_animator) || (sid->down.bounce_y_animator)) return;
   if (sid->down.momentum_animator)
     {
        Evas_Coord px, py;
        _elm_scroll_content_pos_get(sid->obj, &px, &py);
        _elm_scroll_wanted_coordinates_update(sid, px, py);

        ecore_animator_del(sid->down.momentum_animator);
        sid->down.momentum_animator = NULL;
        sid->down.bounce_x_hold = EINA_FALSE;
        sid->down.bounce_y_hold = EINA_FALSE;
        sid->down.ax = 0;
        sid->down.ay = 0;
        sid->down.dx = 0;
        sid->down.dy = 0;
        sid->down.pdx = 0;
        sid->down.pdy = 0;
        if (sid->content_info.resized)
          _elm_scroll_wanted_region_set(sid->obj);
     }
}

static Eina_Bool
_elm_scroll_bounce_x_animator(void *data)
{
   Elm_Scrollable_Smart_Interface_Data *sid;
   Evas_Coord x, y, dx, w, odx, ed, md;
   double t, p, dt, pd, r;

   sid = data;
   t = ecore_loop_time_get();
   dt = t - sid->down.anim_start2;
   if (dt >= 0.0)
     {
        dt = dt / _elm_config->thumbscroll_bounce_friction;
        odx = sid->down.b2x - sid->down.bx;
        _elm_scroll_content_viewport_size_get(sid->obj, &w, NULL);
        if (!sid->down.momentum_animator && (w > abs(odx)))
          {
             pd = (double)odx / (double)w;
             pd = (pd > 0) ? pd : -pd;
             pd = 1.0 - ((1.0 - pd) * (1.0 - pd));
             dt = dt / pd;
          }
        if (dt > 1.0) dt = 1.0;
        p = 1.0 - ((1.0 - dt) * (1.0 - dt));
        _elm_scroll_content_pos_get(sid->obj, &x, &y);
        dx = (odx * p);
        r = 1.0;
        if (sid->down.momentum_animator)
          {
             ed = abs(sid->down.dx * (_elm_config->thumbscroll_friction +
                                      sid->down.extra_time) - sid->down.b0x);
             md = abs(_elm_config->thumbscroll_friction * 5 * w);
             if (ed > md) r = (double)(md) / (double)ed;
          }
        x = sid->down.b2x + (int)((double)(dx - odx) * r);
        if (!sid->down.cancelled)
          _elm_scroll_content_pos_set(sid->obj, x, y, EINA_TRUE);
        if (dt >= 1.0)
          {
             if (sid->down.momentum_animator)
               sid->down.bounce_x_hold = EINA_TRUE;
             if ((!sid->down.bounce_y_animator) &&
                 (!sid->scrollto.y.animator))
               _elm_scroll_anim_stop(sid, EINA_FALSE);
             sid->down.bounce_x_animator = NULL;
             sid->down.pdx = 0;
             sid->bouncemex = EINA_FALSE;
             _elm_scroll_momentum_end(sid);
             if (sid->content_info.resized)
               _elm_scroll_wanted_region_set(sid->obj);
             return ECORE_CALLBACK_CANCEL;
          }
     }
   return ECORE_CALLBACK_RENEW;
}

static Eina_Bool
_elm_scroll_bounce_y_animator(void *data)
{
   Elm_Scrollable_Smart_Interface_Data *sid;
   Evas_Coord x, y, dy, h, ody, ed, md;
   double t, p, dt, pd, r;

   sid = data;
   t = ecore_loop_time_get();
   dt = t - sid->down.anim_start3;
   if (dt >= 0.0)
     {
        dt = dt / _elm_config->thumbscroll_bounce_friction;
        ody = sid->down.b2y - sid->down.by;
        _elm_scroll_content_viewport_size_get(sid->obj, NULL, &h);
        if (!sid->down.momentum_animator && (h > abs(ody)))
          {
             pd = (double)ody / (double)h;
             pd = (pd > 0) ? pd : -pd;
             pd = 1.0 - ((1.0 - pd) * (1.0 - pd));
             dt = dt / pd;
          }
        if (dt > 1.0) dt = 1.0;
        p = 1.0 - ((1.0 - dt) * (1.0 - dt));
        _elm_scroll_content_pos_get(sid->obj, &x, &y);
        dy = (ody * p);
        r = 1.0;
        if (sid->down.momentum_animator)
          {
             ed = abs(sid->down.dy * (_elm_config->thumbscroll_friction +
                                      sid->down.extra_time) - sid->down.b0y);
             md = abs(_elm_config->thumbscroll_friction * 5 * h);
             if (ed > md) r = (double)(md) / (double)ed;
          }
        y = sid->down.b2y + (int)((double)(dy - ody) * r);
        if (!sid->down.cancelled)
          _elm_scroll_content_pos_set(sid->obj, x, y, EINA_TRUE);
        if (dt >= 1.0)
          {
             if (sid->down.momentum_animator)
               sid->down.bounce_y_hold = EINA_TRUE;
             if ((!sid->down.bounce_x_animator) &&
                 (!sid->scrollto.y.animator))
               _elm_scroll_anim_stop(sid, EINA_FALSE);
             sid->down.bounce_y_animator = NULL;
             sid->down.pdy = 0;
             sid->bouncemey = EINA_FALSE;
             _elm_scroll_momentum_end(sid);
             if (sid->content_info.resized)
               _elm_scroll_wanted_region_set(sid->obj);
             return ECORE_CALLBACK_CANCEL;
          }
     }

   return ECORE_CALLBACK_RENEW;
}

static void
_elm_scroll_bounce_eval(Elm_Scrollable_Smart_Interface_Data *sid)
{
   Evas_Coord mx, my, px, py, bx, by, b2x, b2y, minx = 0, miny = 0;

   if (!sid->pan_obj) return;

   ELM_PAN_DATA_GET_OR_RETURN(sid->pan_obj, psd);

   if (sid->freeze) return;
   if ((!sid->bouncemex) && (!sid->bouncemey)) return;
   if (sid->down.now) return;  // down bounce while still held down
   if (sid->down.onhold_animator)
     {
        ecore_animator_del(sid->down.onhold_animator);
        sid->down.onhold_animator = NULL;
        if (sid->content_info.resized)
          _elm_scroll_wanted_region_set(sid->obj);
     }
   if (sid->down.hold_animator)
     {
        ecore_animator_del(sid->down.hold_animator);
        sid->down.hold_animator = NULL;
        if (sid->content_info.resized)
          _elm_scroll_wanted_region_set(sid->obj);
     }
   psd->api->pos_max_get(sid->pan_obj, &mx, &my);
   psd->api->pos_min_get(sid->pan_obj, &minx, &miny);
   psd->api->pos_get(sid->pan_obj, &px, &py);
   bx = px;
   by = py;
   if (px < minx) px = minx;
   if ((px - minx) > mx) px = mx + minx;
   if (py < miny) py = miny;
   if ((py - miny) > my) py = my + miny;
   b2x = px;
   b2y = py;
   if ((!sid->obj) ||
       (!elm_widget_drag_child_locked_x_get(sid->obj)))
     {
        if ((!sid->down.bounce_x_animator) && (!sid->bounce_animator_disabled))
          {
             if (sid->bouncemex)
               {
                  if (sid->scrollto.x.animator)
                    {
                       ecore_animator_del(sid->scrollto.x.animator);
                       sid->scrollto.x.animator = NULL;
                    }
                  sid->down.bounce_x_animator =
                    ecore_animator_add(_elm_scroll_bounce_x_animator, sid);
                  sid->down.anim_start2 = ecore_loop_time_get();
                  sid->down.bx = bx;
                  sid->down.bx0 = bx;
                  sid->down.b2x = b2x;
                  if (sid->down.momentum_animator)
                    sid->down.b0x = sid->down.ax;
                  else sid->down.b0x = 0;
               }
          }
     }
   if ((!sid->obj) ||
       (!elm_widget_drag_child_locked_y_get(sid->obj)))
     {
        if ((!sid->down.bounce_y_animator) && (!sid->bounce_animator_disabled))
          {
             if (sid->bouncemey)
               {
                  if (sid->scrollto.y.animator)
                    {
                       ecore_animator_del(sid->scrollto.y.animator);
                       sid->scrollto.y.animator = NULL;
                    }
                  sid->down.bounce_y_animator =
                    ecore_animator_add(_elm_scroll_bounce_y_animator, sid);
                  sid->down.anim_start3 = ecore_loop_time_get();
                  sid->down.by = by;
                  sid->down.by0 = by;
                  sid->down.b2y = b2y;
                  if (sid->down.momentum_animator)
                    sid->down.b0y = sid->down.ay;
                  else sid->down.b0y = 0;
               }
          }
     }
}

static void
_elm_scroll_content_pos_get(const Evas_Object *obj,
                            Evas_Coord *x,
                            Evas_Coord *y)
{
   ELM_SCROLL_IFACE_DATA_GET_OR_RETURN(obj, sid);

   if (!sid->pan_obj) return;

   ELM_PAN_DATA_GET_OR_RETURN(sid->pan_obj, psd);

   psd->api->pos_get(sid->pan_obj, x, y);
}

static void
_elm_scroll_content_pos_set(Evas_Object *obj,
                            Evas_Coord x,
                            Evas_Coord y,
                            Eina_Bool sig)
{
   Evas_Coord mx = 0, my = 0, px = 0, py = 0, spx = 0, spy = 0, minx = 0, miny = 0, pw = 0, ph = 0;
   Evas_Coord cw =0, ch = 0, ww = 0, wh = 0; //// TIZEN ONLY
   double vx, vy;

   ELM_SCROLL_IFACE_DATA_GET_OR_RETURN(obj, sid);

   if (!sid->edje_obj || !sid->pan_obj) return;

   ELM_PAN_DATA_GET_OR_RETURN(sid->pan_obj, psd);

   psd->api->pos_get(sid->pan_obj, &px, &py);

   if ((px == x) && (py == y)) return;

   // FIXME: allow for bounce outside of range
   psd->api->pos_max_get(sid->pan_obj, &mx, &my);
   psd->api->pos_min_get(sid->pan_obj, &minx, &miny);
   psd->api->content_size_get(sid->pan_obj, &cw, &ch); //// TIZEN ONLY
   _elm_scroll_content_viewport_size_get(obj, &ww, &wh); //// TIZEN ONLY
   evas_object_geometry_get(sid->pan_obj, NULL, NULL, &pw, &ph);

   if (_elm_config->access_mode)
     {
        if (((y > 0) && (y % _elm_config->finger_size) < 2 &&
             !((py % _elm_config->finger_size) < 2 && abs(py - y) < 2)) ||
            ((x > 0) && (x % _elm_config->finger_size) < 2 &&
             !((px % _elm_config->finger_size) < 2 && abs(px - x) < 2)))
          _elm_access_sound_play(ELM_ACCESS_SOUND_SCROLL);
     }

   if (_paging_is_enabled(sid))
     {
        //Work when Page Snap is enabled.
        /*
        //we passed one page to the right
        if (x > sid->current_page.x + sid->pagesize_h)
          x = sid->current_page.x + sid->pagesize_h;
        //we passed one page to the left
        if (x < sid->current_page.x - sid->pagesize_h)
          x = sid->current_page.x - sid->pagesize_h;

        //we passed one page to the bottom
        if (y > sid->current_page.y + sid->pagesize_v)
          y = sid->current_page.y + sid->pagesize_v;
        //we passed one page to the top
        if (y < sid->current_page.y - sid->pagesize_v)
          y = sid->current_page.y - sid->pagesize_v;
        */
     }

   if (sid->loop_h && cw > 0)
     {
        if (_paging_is_enabled(sid))
          {
             if (x < 0) x = cw + (x % cw);
             else if (x >= (pw * sid->pagecount_h)) x = (x % cw);
          }
        else
          {
             if (x < 0) x = cw + (x % cw);
             else if (x >= cw) x = (x % cw);
          }
     }
   if (sid->loop_v && ch > 0)
     {
       if (_paging_is_enabled(sid))
         {
            if (y < 0) y = ch + (y % ch);
            else if (y >= (ph * sid->pagecount_v)) y = (y % ch);
         }
       else
         {
            if (y < 0) y = ch + (y % ch);
            else if (y >= ch) y = (y % ch);
         }
     }

//// TIZEN ONLY
   if (cw > ww)
     {
        if (x < minx)
          edje_object_signal_emit(sid->edje_obj, "elm,edge,left", "elm");
        if (!sid->loop_h && (x - minx) > mx)
          edje_object_signal_emit(sid->edje_obj, "elm,edge,right", "elm");
     }
   if (ch > wh)
     {
        if (y < miny)
          edje_object_signal_emit(sid->edje_obj, "elm,edge,top", "elm");
        if (!sid->loop_v && (y - miny) > my)
          edje_object_signal_emit(sid->edje_obj, "elm,edge,bottom", "elm");
     }
//
   if (!_elm_config->thumbscroll_bounce_enable)
     {
        if (x < minx) x = minx;
        if (!sid->loop_h && (x - minx) > mx) x = mx + minx;
        if (y < miny) y = miny;
        if (!sid->loop_v && (y - miny) > my) y = my + miny;
     }

   if (!sid->bounce_horiz)
     {
        if (x < minx) x = minx;
        if (!sid->loop_h && ((x - minx) > mx)) x = mx + minx;
     }
   if (!sid->bounce_vert)
     {
        if (y < miny) y = miny;
        if (!sid->loop_v && ((y - miny) > my)) y = my + miny;
     }

   psd->api->pos_set(sid->pan_obj, x, y);
   psd->api->pos_get(sid->pan_obj, &spx, &spy);

   if (mx > 0) vx = (double)(spx - minx) / (double)mx;
   else vx = 0.0;

   if (vx < 0.0) vx = 0.0;
   else if (vx > 1.0)
     vx = 1.0;

   if (my > 0) vy = (double)(spy - miny) / (double)my;
   else vy = 0.0;

   if (vy < 0.0) vy = 0.0;
   else if (vy > 1.0)
     vy = 1.0;

   edje_object_part_drag_value_set
     (sid->edje_obj, "elm.dragable.vbar", 0.0, vy);
   edje_object_part_drag_value_set
     (sid->edje_obj, "elm.dragable.hbar", vx, 0.0);

   if (!sid->loop_h && !sid->down.bounce_x_animator)
     {
        if (((x < minx) && (0 <= sid->down.dx)) ||
            ((x > (mx + minx)) && (0 >= sid->down.dx)))
          {
             sid->bouncemex = EINA_TRUE;
             _elm_scroll_bounce_eval(sid);
          }
        else
          sid->bouncemex = EINA_FALSE;
     }
   if (!sid->loop_v && !sid->down.bounce_y_animator)
     {
        if (((y < miny) && (0 <= sid->down.dy)) ||
            ((y > (my + miny)) && (0 >= sid->down.dy)))
          {
             sid->bouncemey = EINA_TRUE;
             _elm_scroll_bounce_eval(sid);
          }
        else
          sid->bouncemey = EINA_FALSE;
     }

   if (sig)
     {
        if ((x != px) || (y != py))
          {
             if (_elm_config->scroll_item_align_enable && !sid->is_unset_cb_called)
               {
                  if (sid->cb_func.item_scroll_align_end_unset)
                    {
                       sid->cb_func.item_scroll_align_end_unset(sid->obj, NULL);
                       sid->is_unset_cb_called = EINA_TRUE;
                    }
               }

             if (sid->cb_func.scroll)
               sid->cb_func.scroll(obj, NULL);
             edje_object_signal_emit(sid->edje_obj, "elm,action,scroll", "elm");
             if (x < px)
               {
                  if (sid->cb_func.scroll_left)
                    sid->cb_func.scroll_left(obj, NULL);
                  edje_object_signal_emit(sid->edje_obj, "elm,action,scroll,left", "elm");
               }
             if (x > px)
               {
                  if (sid->cb_func.scroll_right)
                    sid->cb_func.scroll_right(obj, NULL);
                  edje_object_signal_emit(sid->edje_obj, "elm,action,scroll,right", "elm");
               }
             if (y < py)
               {
                  if (sid->cb_func.scroll_up)
                    sid->cb_func.scroll_up(obj, NULL);
                  edje_object_signal_emit(sid->edje_obj, "elm,action,scroll,up", "elm");
               }
             if (y > py)
               {
                  if (sid->cb_func.scroll_down)
                    sid->cb_func.scroll_down(obj, NULL);
                  edje_object_signal_emit(sid->edje_obj, "elm,action,scroll,down", "elm");
               }
             edje_object_message_signal_process(sid->edje_obj);
          }
        if (x != px)
          {
             if (x == minx)
               {
                  if (sid->cb_func.edge_left)
                    sid->cb_func.edge_left(obj, NULL);
               }
             if (x == (mx + minx))
               {
                  if (sid->cb_func.edge_right)
                    sid->cb_func.edge_right(obj, NULL);
               }
          }
        if (y != py)
          {
             if (y == miny)
               {
                  if (sid->cb_func.edge_top)
                    sid->cb_func.edge_top(obj, NULL);
               }
             if (y == my + miny)
               {
                  if (sid->cb_func.edge_bottom)
                    sid->cb_func.edge_bottom(obj, NULL);
               }
          }
     }

   _elm_direction_arrows_eval(sid);
}

static void
_elm_scroll_mirrored_set(Evas_Object *obj,
                         Eina_Bool mirrored)
{
   Evas_Coord wx;

   ELM_SCROLL_IFACE_DATA_GET_OR_RETURN(obj, sid);

   if (!sid->edje_obj) return;

   mirrored = !!mirrored;

   if (sid->is_mirrored == mirrored)
     return;

   sid->is_mirrored = mirrored;
   edje_object_mirrored_set(sid->edje_obj, mirrored);

   if (sid->is_mirrored)
     wx = _elm_scroll_x_mirrored_get(sid->obj, sid->wx);
   else
     wx = sid->wx;

   _elm_scroll_content_pos_set(sid->obj, wx, sid->wy, EINA_FALSE);
}

/* returns TRUE when we need to move the scroller, FALSE otherwise.
 * Updates w and h either way, so save them if you need them. */
static Eina_Bool
_elm_scroll_content_region_show_internal(Evas_Object *obj,
                                         Evas_Coord *_x,
                                         Evas_Coord *_y,
                                         Evas_Coord w,
                                         Evas_Coord h)
{
   Evas_Coord mx = 0, my = 0, cw = 0, ch = 0, px = 0, py = 0, nx = 0, ny = 0,
              minx = 0, miny = 0, pw = 0, ph = 0, x = *_x, y = *_y;

   ELM_SCROLL_IFACE_DATA_GET_OR_RETURN_VAL(obj, sid, EINA_FALSE);

   if (!sid->pan_obj) return EINA_FALSE;

   ELM_PAN_DATA_GET_OR_RETURN_VAL(sid->pan_obj, psd, EINA_FALSE);

   psd->api->pos_max_get(sid->pan_obj, &mx, &my);
   psd->api->pos_min_get(sid->pan_obj, &minx, &miny);
   psd->api->content_size_get(sid->pan_obj, &cw, &ch);
   psd->api->pos_get(sid->pan_obj, &px, &py);
   evas_object_geometry_get(sid->pan_obj, NULL, NULL, &pw, &ph);

   nx = px;
   if ((x < px) && ((x + w) < (px + (cw - mx)))) nx = x;
   else if ((x > px) && ((x + w) > (px + (cw - mx))))
     nx = x + w - (cw - mx);
   ny = py;
   if ((y < py) && ((y + h) < (py + (ch - my)))) ny = y;
   else if ((y > py) && ((y + h) > (py + (ch - my))))
     ny = y + h - (ch - my);

   if ((sid->down.bounce_x_animator) || (sid->down.bounce_y_animator) ||
       (sid->scrollto.x.animator) || (sid->scrollto.y.animator))
     {
        if (sid->scrollto.x.animator)
          {
             ecore_animator_del(sid->scrollto.x.animator);
             sid->scrollto.x.animator = NULL;
          }
        if (sid->scrollto.y.animator)
          {
             ecore_animator_del(sid->scrollto.y.animator);
             sid->scrollto.y.animator = NULL;
          }
        if (sid->down.bounce_x_animator)
          {
             ecore_animator_del(sid->down.bounce_x_animator);
             sid->down.bounce_x_animator = NULL;
             sid->bouncemex = EINA_FALSE;
             if (sid->content_info.resized)
               _elm_scroll_wanted_region_set(sid->obj);
          }
        if (sid->down.bounce_y_animator)
          {
             ecore_animator_del(sid->down.bounce_y_animator);
             sid->down.bounce_y_animator = NULL;
             sid->bouncemey = EINA_FALSE;
             if (sid->content_info.resized)
               _elm_scroll_wanted_region_set(sid->obj);
          }
        _elm_scroll_anim_cancel(sid);
     }
   if (sid->down.hold_animator)
     {
        ecore_animator_del(sid->down.hold_animator);
        sid->down.hold_animator = NULL;
        _elm_scroll_drag_stop(sid);
        if (sid->content_info.resized)
          _elm_scroll_wanted_region_set(sid->obj);
     }
   if (sid->down.momentum_animator)
     {
        ecore_animator_del(sid->down.momentum_animator);
        sid->down.momentum_animator = NULL;
        sid->down.bounce_x_hold = EINA_FALSE;
        sid->down.bounce_y_hold = EINA_FALSE;
        sid->down.ax = 0;
        sid->down.ay = 0;
        sid->down.pdx = 0;
        sid->down.pdy = 0;
        _elm_scroll_anim_stop(sid, EINA_FALSE);
        if (sid->content_info.resized)
          _elm_scroll_wanted_region_set(sid->obj);
     }

   if (_paging_is_enabled(sid))
     {
        x = _elm_scroll_page_x_get(sid, nx - px, EINA_FALSE);
        y = _elm_scroll_page_y_get(sid, ny - py, EINA_FALSE);
        if ((pw != sid->pagesize_h) && (sid->pagesize_h != 0))
          pw = sid->pagesize_h;
        if ((ph != sid->pagesize_v) && (sid->pagesize_v != 0))
          ph = sid->pagesize_v;
     }
   else
     {
        x = nx;
        y = ny;
     }
   if (!sid->loop_h)
     {
        if ((x + pw) > cw) x = cw - pw;
        if (x < minx) x = minx;
     }
   if (!sid->loop_v)
     {
       if ((y + ph) > ch) y = ch - ph;
       if (y < miny) y = miny;
     }

   if ((x == px) && (y == py)) return EINA_FALSE;
   *_x = x;
   *_y = y;

   return EINA_TRUE;
}

/* Set should be used for calculated positions, for example, when we move
 * because of an animation or because this is the correct position after
 * constraints. */
static void
_elm_scroll_content_region_set(Evas_Object *obj,
                               Evas_Coord x,
                               Evas_Coord y,
                               Evas_Coord w,
                               Evas_Coord h)
{
   ELM_SCROLL_IFACE_DATA_GET_OR_RETURN(obj, sid);

   if (_elm_scroll_content_region_show_internal(obj, &x, &y, w, h))
     {
        _elm_scroll_content_pos_set(obj, x, y, EINA_FALSE);
        sid->down.sx = x;
        sid->down.sy = y;
        sid->down.x = sid->down.history[0].x;
        sid->down.y = sid->down.history[0].y;
     }
}

/* Set should be used for setting the wanted position, for example a
 * user scroll or moving the cursor in an entry. */
static void
_elm_scroll_content_region_show(Evas_Object *obj,
                                Evas_Coord x,
                                Evas_Coord y,
                                Evas_Coord w,
                                Evas_Coord h)
{
   Evas_Coord cw = 0, ch = 0;
   ELM_SCROLL_IFACE_DATA_GET_OR_RETURN(obj, sid);

   if (sid->pan_obj)
     {
        ELM_PAN_DATA_GET_OR_RETURN(sid->pan_obj, psd);
        psd->api->content_size_get(sid->pan_obj, &cw, &ch);
     }

   if (sid->origin_x && cw > 0)
     {
        sid->wx = cw - y;
        if (sid->wx < 0) sid->wx = 0;
     }
   else
     sid->wx = x;

   if (sid->origin_y && ch > 0)
     {
        sid->wy = ch - y;
        if (sid->wy < 0) sid->wy = 0;
     }
   else
     sid->wy = y;

   sid->ww = w;
   sid->wh = h;
   if (_paging_is_enabled(sid))
     {
        if (sid->pagesize_h != 0)
          {
             x = (int)(x / sid->pagesize_h) * sid->pagesize_h;
             if (sid->origin_x && cw > 0)
               {
                  sid->wx = cw - x;
                  if (sid->wx < 0) sid->wx = 0;
               }
             else
               sid->wx = x;
             sid->ww = w = sid->pagesize_h;
          }
        if (sid->pagesize_v != 0)
          {
             y = (int)(y / sid->pagesize_v) * sid->pagesize_v;
             if (sid->origin_y && ch > 0)
               {
                  sid->wy = ch - y;
                  if (sid->wy < 0) sid->wy = 0;
               }
             else
               sid->wy = y;
             sid->wh = h = sid->pagesize_v;
          }
     }

   if (_elm_scroll_content_region_show_internal(obj, &x, &y, w, h))
     {
        _elm_scroll_content_pos_set(obj, x, y, EINA_TRUE);
        sid->down.sx = x;
        sid->down.sy = y;
        sid->down.x = sid->down.history[0].x;
        sid->down.y = sid->down.history[0].y;
        if (_paging_is_enabled(sid))
          {
             int pagenumber_h = 0, pagenumber_v = 0;
             _elm_scroll_current_page_get(sid->obj, &pagenumber_h, &pagenumber_v);
             if (sid->pagesize_h)
                sid->rotary_animation_info.current_page = pagenumber_h;
             else if (sid->pagesize_v)
                sid->rotary_animation_info.current_page = pagenumber_v;
          }
     }
}

static void
_elm_scroll_wanted_region_set(Evas_Object *obj)
{
   Evas_Coord ww, wh, wx, wy, cw= 0, ch = 0;

   ELM_SCROLL_IFACE_DATA_GET_OR_RETURN(obj, sid);

   wx = sid->wx;
   wy = sid->wy;

   if (sid->down.now || sid->down.momentum_animator ||
       sid->down.bounce_x_animator || sid->down.bounce_y_animator ||
       sid->down.hold_animator || sid->down.onhold_animator ||
       sid->scrollto.x.animator || sid->scrollto.y.animator)
     return;

   if (sid->pan_obj)
     {
        ELM_PAN_DATA_GET_OR_RETURN(sid->pan_obj, psd);
        psd->api->content_size_get(sid->pan_obj, &cw, &ch);
     }

   sid->content_info.resized = EINA_FALSE;

   /* Flip to RTL cords only if init in RTL mode */
   if (sid->is_mirrored)
     wx = _elm_scroll_x_mirrored_get(obj, sid->wx);

   if (sid->origin_x && cw > 0)
     {
        wx = cw - sid->wx;
        if (wx < 0) wx = 0;
     }

   if (sid->origin_y && ch > 0)
     {
        wy = ch - sid->wy;
        if (wy < 0) wy = 0;
     }

   if (sid->ww == -1)
     {
        _elm_scroll_content_viewport_size_get(obj, &ww, &wh);
     }
   else
     {
        ww = sid->ww;
        wh = sid->wh;
     }

   _elm_scroll_content_region_set(obj, wx, wy, ww, wh);

   if (_paging_is_enabled(sid))
     {
        int pagenumber_h = 0, pagenumber_v = 0;
        _elm_scroll_current_page_get(sid->obj, &pagenumber_h, &pagenumber_v);
        if (sid->pagesize_h)
             sid->rotary_animation_info.current_page = pagenumber_h;
        else if (sid->pagesize_v)
             sid->rotary_animation_info.current_page = pagenumber_v;
   }
}

static void
_elm_scroll_wheel_event_cb(void *data,
                           Evas *e __UNUSED__,
                           Evas_Object *obj __UNUSED__,
                           void *event_info)
{
   Elm_Scrollable_Smart_Interface_Data *sid;
   Evas_Event_Mouse_Wheel *ev;
   Evas_Coord x = 0, y = 0, vw = 0, vh = 0, cw = 0, ch = 0;
   int direction = 0;
   int pagenumber_h, pagenumber_v;

   sid = data;
   ELM_PAN_DATA_GET_OR_RETURN(sid->pan_obj, psd);
   ev = event_info;
   direction = ev->direction;

   if (sid->block & ELM_SCROLLER_MOVEMENT_BLOCK_VERTICAL)
     return;

   if (ev->event_flags & EVAS_EVENT_FLAG_ON_HOLD) return;
   if ((evas_key_modifier_is_set(ev->modifiers, "Control")) ||
       (evas_key_modifier_is_set(ev->modifiers, "Alt")) ||
       (evas_key_modifier_is_set(ev->modifiers, "Meta")) ||
       (evas_key_modifier_is_set(ev->modifiers, "Hyper")) ||
       (evas_key_modifier_is_set(ev->modifiers, "Super")))
     return;
   else if (evas_key_modifier_is_set(ev->modifiers, "Shift"))
     direction = !direction;
   _elm_scroll_content_pos_get(sid->obj, &x, &y);
   if ((sid->down.bounce_x_animator) || (sid->down.bounce_y_animator) ||
       (sid->scrollto.x.animator) || (sid->scrollto.y.animator))
     {
        _elm_scroll_anim_stop(sid, EINA_FALSE);
     }
   if (sid->scrollto.x.animator)
     {
        ecore_animator_del(sid->scrollto.x.animator);
        sid->scrollto.x.animator = NULL;
     }
   if (sid->scrollto.y.animator)
     {
        ecore_animator_del(sid->scrollto.y.animator);
        sid->scrollto.y.animator = NULL;
     }
   if (sid->down.bounce_x_animator)
     {
        ecore_animator_del(sid->down.bounce_x_animator);
        sid->down.bounce_x_animator = NULL;
        sid->bouncemex = EINA_FALSE;
        if (sid->content_info.resized)
          _elm_scroll_wanted_region_set(sid->obj);
     }
   if (sid->down.bounce_y_animator)
     {
        ecore_animator_del(sid->down.bounce_y_animator);
        sid->down.bounce_y_animator = NULL;
        sid->bouncemey = EINA_FALSE;
        if (sid->content_info.resized)
          _elm_scroll_wanted_region_set(sid->obj);
     }
   _elm_scroll_content_viewport_size_get(sid->obj, &vw, &vh);
   psd->api->content_size_get(sid->pan_obj, &cw, &ch);
   if (!_paging_is_enabled(sid))
     {
        if (!direction)
          {
             if (ch > vh || cw <= vw)
               y += ev->z * sid->step.y;
             else
               x += ev->z * sid->step.x;
          }
        else if (direction == 1)
          {
             if (cw > vw || ch <= vh)
               x += ev->z * sid->step.x;
             else
               y += ev->z * sid->step.y;
          }

        if ((!sid->hold) && (!sid->freeze))
          {
             _elm_scroll_wanted_coordinates_update(sid, x, y);
             _elm_scroll_content_pos_set(sid->obj, x, y, EINA_TRUE);
          }
     }
   else
     {
        _elm_scroll_current_page_get(sid->obj, &pagenumber_h, &pagenumber_v);
        if (!direction)
          {
             if (ch > vh || cw <= vw)
               y = (pagenumber_v + (1 * ev->z)) * sid->pagesize_v;
             else
               x = (pagenumber_h + (1 * ev->z)) * sid->pagesize_h;
          }
        else if (direction == 1)
          {
             if (cw > vw || ch <= vh)
               x = (pagenumber_h + (1 * ev->z)) * sid->pagesize_h;
             else
               y = (pagenumber_v + (1 * ev->z)) * sid->pagesize_v;
          }

        if ((!sid->hold) && (!sid->freeze))
          {
             //_elm_scroll_scroll_to_x(sid, _elm_config->bring_in_scroll_friction, x);
             _elm_scroll_scroll_to_y(sid, _elm_config->bring_in_scroll_friction, y);
          }
     }
}

static Eina_Bool
_elm_scroll_post_event_up(void *data,
                          Evas *e __UNUSED__)
{
   Elm_Scrollable_Smart_Interface_Data *sid = data;

   if (sid->obj)
     {
        if (sid->down.dragged)
          {
             elm_widget_drag_lock_x_set(sid->obj, EINA_FALSE);
             elm_widget_drag_lock_y_set(sid->obj, EINA_FALSE);
             ERR("[DDO] lock set false. : obj(%x), type(%s)", sid->obj, elm_widget_type_get(sid->obj));
          }
     }
   return EINA_TRUE;
}

static Eina_Bool
_paging_is_enabled(Elm_Scrollable_Smart_Interface_Data *sid)
{
   if ((sid->pagerel_h == 0.0) && (!sid->pagesize_h) &&
       (sid->pagerel_v == 0.0) && (!sid->pagesize_v))
     return EINA_FALSE;
   return EINA_TRUE;
}

static Eina_Bool
_elm_scroll_momentum_animator(void *data)
{
   double t, at, dt, p, r;
   Elm_Scrollable_Smart_Interface_Data *sid = data;
   Evas_Coord x, y, dx, dy, px, py, maxx, maxy, minx, miny, ox = 0, oy = 0;
   Eina_Bool no_bounce_x_end = EINA_FALSE, no_bounce_y_end = EINA_FALSE;
   double v[4] = {_elm_config->thumbscroll_cubic_bezier_p1_x,
      _elm_config->thumbscroll_cubic_bezier_p1_y,
      _elm_config->thumbscroll_cubic_bezier_p2_x,
      _elm_config->thumbscroll_cubic_bezier_p2_y};
   double duration = _elm_config->thumbscroll_duration;

   if (!sid->pan_obj) return ECORE_CALLBACK_CANCEL;

   ELM_PAN_DATA_GET_OR_RETURN_VAL(sid->pan_obj, psd, ECORE_CALLBACK_CANCEL);

   if (_elm_config->scroll_item_align_enable)
     {
        int vw = 0, vh = 0;
        _elm_scroll_content_viewport_size_get(sid->obj, &vw, &vh);

        duration = duration * ecore_animator_pos_map_n((double)((double)abs(sid->down.dy)) / ((double)(4 * vh)), ECORE_POS_MAP_CUBIC_BEZIER, 4, v);

        if (duration < 0.2)
           duration = 0.2;
        if (duration > _elm_config->thumbscroll_duration)
           duration = _elm_config->thumbscroll_duration;
     }

   t = ecore_loop_time_get();
   dt = t - sid->down.anim_start;
   if (dt > 0.0)
     {
        p = ecore_animator_pos_map_n(dt / duration, ECORE_POS_MAP_CUBIC_BEZIER, 4, v);

        if (psd->api->pos_adjust || sid->cb_func.scroll_position_get)
          {
             dx = sid->down.dx * p;
             dy = sid->down.dy * p;
          }
        else
          {
             if (sid->is_page_flick && _paging_is_enabled(sid))
               {
                 dx = sid->down.dx * p;
                 dy = sid->down.dy * p;
               }
             else
               {
                  dx = (sid->down.dx * (_elm_config->thumbscroll_friction +
                           sid->down.extra_time) * p);
                  dy = (sid->down.dy * (_elm_config->thumbscroll_friction +
                           sid->down.extra_time) * p);
               }
          }

        sid->down.ax = dx;
        sid->down.ay = dy;
        x = sid->down.sx - dx;
        y = sid->down.sy - dy;
        _elm_scroll_content_pos_get(sid->obj, &px, &py);
        if ((sid->down.bounce_x_animator) ||
            (sid->down.bounce_x_hold))
          {
             sid->down.bx = sid->down.bx0 - dx + sid->down.b0x;
             x = px;
          }
        if ((sid->down.bounce_y_animator) ||
            (sid->down.bounce_y_hold))
          {
             sid->down.by = sid->down.by0 - dy + sid->down.b0y;
             y = py;
          }
        _elm_scroll_content_pos_set(sid->obj, x, y, EINA_TRUE);
        _elm_scroll_wanted_coordinates_update(sid, x, y);
        psd->api->pos_max_get(sid->pan_obj, &maxx, &maxy);
        psd->api->pos_min_get(sid->pan_obj, &minx, &miny);
        if (!_elm_config->thumbscroll_bounce_enable || !sid->bounce_horiz)
          {
             if (x <= minx) no_bounce_x_end = EINA_TRUE;
             if (!sid->loop_h && (x - minx) >= maxx) no_bounce_x_end = EINA_TRUE;
          }
        if (!_elm_config->thumbscroll_bounce_enable || !sid->bounce_vert)
          {
             if (y <= miny) no_bounce_y_end = EINA_TRUE;
             if (!sid->loop_v && (y - miny) >= maxy) no_bounce_y_end = EINA_TRUE;
          }
        if ((dt >= duration) ||
            ((sid->down.bounce_x_hold) && (sid->down.bounce_y_hold)) ||
            (no_bounce_x_end && no_bounce_y_end))
          {
             _elm_scroll_anim_stop(sid, EINA_FALSE);

             sid->down.momentum_animator = NULL;
             sid->down.bounce_x_hold = EINA_FALSE;
             sid->down.bounce_y_hold = EINA_FALSE;
             sid->down.ax = 0;
             sid->down.ay = 0;
             sid->down.pdx = 0;
             sid->down.pdy = 0;
             if (sid->content_info.resized)
               _elm_scroll_wanted_region_set(sid->obj);

             return ECORE_CALLBACK_CANCEL;
          }
     }

   return ECORE_CALLBACK_RENEW;
}

static Evas_Coord
_elm_scroll_page_x_get(Elm_Scrollable_Smart_Interface_Data *sid,
                       int offset, Eina_Bool limit)
{
   Evas_Coord x, y, w, h, dx, cw, ch, minx = 0;

   if (!sid->pan_obj) return 0;

   ELM_PAN_DATA_GET_OR_RETURN_VAL(sid->pan_obj, psd, 0);

   _elm_scroll_content_pos_get(sid->obj, &x, &y);
   _elm_scroll_content_viewport_size_get(sid->obj, &w, &h);
   psd->api->content_size_get(sid->pan_obj, &cw, &ch);
   psd->api->pos_min_get(sid->pan_obj, &minx, NULL);

   if (sid->pagerel_h > 0.0)
     {
        if (sid->pagecount_h != 0)
          w = cw / (sid->pagecount_h * sid->pagerel_h);
        x *= (w * sid->pagerel_h) / sid->pagesize_h;
        sid->pagesize_h = w * sid->pagerel_h;
     }

   if (!limit)
     x += offset;
   else
     {
        dx = (sid->pagesize_h * ((double)sid->page_limit_h - 0.5));

        if (offset > 0)
          x += (abs(offset) < dx ? offset : dx);
        else
          x += (abs(offset) < dx ? offset : -(dx + 1));
     }

   if (sid->pagesize_h > 0)
     {
        if (x >= minx) x = x + (sid->pagesize_h * 0.5);
        else x = x - (sid->pagesize_h * 0.5);
        x = x / (sid->pagesize_h);
        x = x * (sid->pagesize_h);
     }
   if (!sid->loop_h)
     {
        if ((x + w) > cw) x = cw - w;
        if (x < minx) x = minx;
     }

   return x;
}

static Evas_Coord
_elm_scroll_page_y_get(Elm_Scrollable_Smart_Interface_Data *sid,
                       int offset, Eina_Bool limit)
{
   Evas_Coord x, y, w, h, dy, cw, ch, miny = 0;

   if (!sid->pan_obj) return 0;

   ELM_PAN_DATA_GET_OR_RETURN_VAL(sid->pan_obj, psd, 0);

   _elm_scroll_content_pos_get(sid->obj, &x, &y);
   _elm_scroll_content_viewport_size_get(sid->obj, &w, &h);
   psd->api->content_size_get(sid->pan_obj, &cw, &ch);
   psd->api->pos_min_get(sid->pan_obj, NULL, &miny);

   if (sid->pagerel_v > 0.0)
     {
        if (sid->pagecount_v != 0)
          h = ch / (sid->pagecount_v * sid->pagerel_v);
        y *= (h * sid->pagerel_v) / sid->pagesize_v;
        sid->pagesize_v = h * sid->pagerel_v;
     }
   if (!limit)
     y += offset;
   else
     {
        dy = (sid->pagesize_v * ((double)sid->page_limit_v - 0.5));

        if (offset > 0)
          y += (abs(offset) < dy ? offset : dy);
        else
          y += (abs(offset) < dy ? offset : -(dy + 1));
     }

   if (sid->pagesize_v > 0)
     {
        if (y >= miny) y = y + (sid->pagesize_v * 0.5);
        else y = y - (sid->pagesize_v * 0.5);
        y = y / (sid->pagesize_v);
        y = y * (sid->pagesize_v);
     }
   if (!sid->loop_v)
     {
        if ((y + h) > ch) y = ch - h;
        if (y < miny) y = miny;
     }

   return y;
}

static Eina_Bool
_elm_scroll_scroll_to_x_animator(void *data)
{
   Elm_Scrollable_Smart_Interface_Data *sid = data;
   Evas_Coord px, py;
   double t, tt;

   if (!sid->pan_obj) return ECORE_CALLBACK_CANCEL;

   ELM_PAN_DATA_GET_OR_RETURN_VAL(sid->pan_obj, psd, ECORE_CALLBACK_CANCEL);

   t = ecore_loop_time_get();
   tt = (t - sid->scrollto.x.t_start) /
     (sid->scrollto.x.t_end - sid->scrollto.x.t_start);

   if (tt <= 0.0) tt += 0.08;

   tt = 1.0 - tt;
   tt = 1.0 - (tt * tt);

   psd->api->pos_get(sid->pan_obj, &px, &py);
   px = (sid->scrollto.x.start * (1.0 - tt)) +
     (sid->scrollto.x.end * tt);
   if (t >= sid->scrollto.x.t_end)
     {
        px = sid->scrollto.x.end;
        _elm_scroll_content_pos_set(sid->obj, px, py, EINA_TRUE);
        sid->down.sx = px;
        sid->down.x = sid->down.history[0].x;
        _elm_scroll_wanted_coordinates_update(sid, px, py);
        sid->scrollto.x.animator = NULL;
        if ((!sid->scrollto.y.animator) && (!sid->down.bounce_y_animator))
          _elm_scroll_anim_stop(sid, EINA_FALSE);
        return ECORE_CALLBACK_CANCEL;
     }
   _elm_scroll_content_pos_set(sid->obj, px, py, EINA_TRUE);
   _elm_scroll_wanted_coordinates_update(sid, px, py);
   return ECORE_CALLBACK_RENEW;
}

static Eina_Bool
_elm_scroll_scroll_to_y_animator(void *data)
{
   Elm_Scrollable_Smart_Interface_Data *sid = data;
   Evas_Coord px, py;
   double t, tt;

   if (!sid->pan_obj) return EINA_FALSE;

   ELM_PAN_DATA_GET_OR_RETURN_VAL(sid->pan_obj, psd, EINA_FALSE);

   t = ecore_loop_time_get();
   tt = (t - sid->scrollto.y.t_start) /
     (sid->scrollto.y.t_end - sid->scrollto.y.t_start);

   if (tt <= 0.0) tt += 0.08;

   tt = 1.0 - tt;
   tt = 1.0 - (tt * tt);

   psd->api->pos_get(sid->pan_obj, &px, &py);
   py = (sid->scrollto.y.start * (1.0 - tt)) +
     (sid->scrollto.y.end * tt);
   if (t >= sid->scrollto.y.t_end)
     {
        py = sid->scrollto.y.end;
        _elm_scroll_content_pos_set(sid->obj, px, py, EINA_TRUE);
        sid->down.sy = py;
        sid->down.y = sid->down.history[0].y;
        _elm_scroll_wanted_coordinates_update(sid, px, py);
        sid->scrollto.y.animator = NULL;
        if ((!sid->scrollto.x.animator) && (!sid->down.bounce_x_animator))
          _elm_scroll_anim_stop(sid, EINA_FALSE);
        return ECORE_CALLBACK_CANCEL;
     }
   _elm_scroll_content_pos_set(sid->obj, px, py, EINA_TRUE);
   _elm_scroll_wanted_coordinates_update(sid, px, py);

   return ECORE_CALLBACK_RENEW;
}

static void
_elm_scroll_scroll_to_y(Elm_Scrollable_Smart_Interface_Data *sid,
                        double t_in,
                        Evas_Coord pos_y)
{
   Evas_Coord px, py, x, y, w, h;
   double t;

   if (!sid->pan_obj) return;

   ELM_PAN_DATA_GET_OR_RETURN(sid->pan_obj, psd);

   if (sid->freeze) return;
   if (t_in <= 0.0)
     {
        _elm_scroll_content_pos_get(sid->obj, &x, &y);
        _elm_scroll_content_viewport_size_get(sid->obj, &w, &h);
        y = pos_y;
        _elm_scroll_content_region_set(sid->obj, x, y, w, h);
        return;
     }
   t = ecore_loop_time_get();
   psd->api->pos_get(sid->pan_obj, &px, &py);
   sid->scrollto.y.start = py;
   sid->scrollto.y.end = pos_y;
   sid->scrollto.y.t_start = t;
   sid->scrollto.y.t_end = t + t_in;
   if (!sid->scrollto.y.animator)
     {
        sid->scrollto.y.animator =
          ecore_animator_add(_elm_scroll_scroll_to_y_animator, sid);
        if (!sid->scrollto.x.animator)
          _elm_scroll_anim_start(sid);
     }
   if (sid->down.bounce_y_animator)
     {
        ecore_animator_del(sid->down.bounce_y_animator);
        sid->down.bounce_y_animator = NULL;
        _elm_scroll_momentum_end(sid);
        if (sid->content_info.resized)
          _elm_scroll_wanted_region_set(sid->obj);
     }
   sid->bouncemey = EINA_FALSE;
}

static void
_elm_scroll_scroll_to_x(Elm_Scrollable_Smart_Interface_Data *sid,
                        double t_in,
                        Evas_Coord pos_x)
{
   Evas_Coord px, py, x, y, w, h;
   double t;

   if (!sid->pan_obj) return;

   ELM_PAN_DATA_GET_OR_RETURN(sid->pan_obj, psd);

   if (sid->freeze) return;
   if (t_in <= 0.0)
     {
        _elm_scroll_content_pos_get(sid->obj, &x, &y);
        _elm_scroll_content_viewport_size_get(sid->obj, &w, &h);
        x = pos_x;
        _elm_scroll_content_region_set(sid->obj, x, y, w, h);
        return;
     }
   t = ecore_loop_time_get();
   psd->api->pos_get(sid->pan_obj, &px, &py);
   sid->scrollto.x.start = px;
   sid->scrollto.x.end = pos_x;
   sid->scrollto.x.t_start = t;
   sid->scrollto.x.t_end = t + t_in;
   if (!sid->scrollto.x.animator)
     {
        sid->scrollto.x.animator =
          ecore_animator_add(_elm_scroll_scroll_to_x_animator, sid);
        if (!sid->scrollto.y.animator)
          _elm_scroll_anim_start(sid);
     }
   if (sid->down.bounce_x_animator)
     {
        ecore_animator_del(sid->down.bounce_x_animator);
        sid->down.bounce_x_animator = NULL;
        _elm_scroll_momentum_end(sid);
        if (sid->content_info.resized)
          _elm_scroll_wanted_region_set(sid->obj);
     }
   sid->bouncemex = EINA_FALSE;
}

static void
_elm_scroll_can_position_adjust(Elm_Scrollable_Smart_Interface_Data *sid,
                                    Evas_Coord *adjust_x, Evas_Coord *adjust_y,
                                    Eina_Bool *scroll_x, Eina_Bool *scroll_y)
{
   ELM_PAN_DATA_GET_OR_RETURN_VAL(sid->pan_obj, psd, EINA_FALSE);

   Evas_Coord minx = 0, miny = 0;
   Evas_Coord mx = 0, my = 0;
   Evas_Coord px = 0, py = 0;

   psd->api->pos_min_get(sid->pan_obj, &minx, &miny);
   psd->api->pos_max_get(sid->pan_obj, &mx, &my);
   psd->api->pos_get(sid->pan_obj, &px, &py);

   if (*adjust_x < 0)
     {
        if ((px - *adjust_x) > mx)
          {
             *adjust_x = -px;
          }
     }
   else if(*adjust_x > 0)
     {
        if ((px - *adjust_x) < minx)
          {
             *adjust_x = px;
          }
     }
   else
     {
        *scroll_x = EINA_FALSE;
     }
   if (*adjust_y < 0)
     {
        if ((py - *adjust_y) > my)
          {
             *adjust_y = -py;
          }
     }
   else if (*adjust_y > 0)
     {
        if ((py - *adjust_y) < miny)
          {
             *adjust_y = py;
          }
     }
    else
      {
        *scroll_y = EINA_FALSE;
      }
}

static void
_elm_scroll_mouse_up_event_cb(void *data,
                              Evas *e,
                              Evas_Object *obj __UNUSED__,
                              void *event_info)
{
   Elm_Scrollable_Smart_Interface_Data *sid = data;
   Evas_Coord x = 0, y = 0, ox = 0, oy = 0;
   Evas_Event_Mouse_Down *ev;

   if (!sid->pan_obj) return;

   ELM_PAN_DATA_GET_OR_RETURN(sid->pan_obj, psd);

   if ((sid->block & ELM_SCROLLER_MOVEMENT_BLOCK_VERTICAL) &&
       (sid->block & ELM_SCROLLER_MOVEMENT_BLOCK_HORIZONTAL))
     return;

#ifdef SMOOTHDBG
   if (_elm_scroll_smooth_debug) _elm_scroll_smooth_debug_shutdown();
#endif

   ev = event_info;

   sid->down.hold_parent = EINA_FALSE;
   sid->down.dx = 0;
   sid->down.dy = 0;
   evas_post_event_callback_push(e, _elm_scroll_post_event_up, sid);

   //REDWOOD ONLY
   edje_object_signal_emit(sid->edje_obj, "elm,state,mouse,up", "elm");

   // FIXME: respect elm_widget_scroll_hold_get of parent container
   if (!_elm_config->thumbscroll_enable) return;

   if (ev->button == 1)
     {
        if (sid->down.onhold_animator)
          {
             ecore_animator_del(sid->down.onhold_animator);
             sid->down.onhold_animator = NULL;
             if (sid->content_info.resized)
               _elm_scroll_wanted_region_set(sid->obj);
          }
        x = ev->canvas.x - sid->down.x;
        y = ev->canvas.y - sid->down.y;
        if (sid->down.dragged)
          {
             _elm_scroll_drag_stop(sid);
             if ((!sid->hold) && (!sid->freeze))
               {
                  int i;
                  double t, at, dt;
                  Evas_Coord ax, ay, dx, dy, vel;

#ifdef EVTIME
                  t = ev->timestamp / 1000.0;
#else
                  t = ecore_loop_time_get();
#endif

                  ev->event_flags |= EVAS_EVENT_FLAG_ON_HOLD;

                  ax = ev->canvas.x;
                  ay = ev->canvas.y;
                  at = 0.0;
#ifdef SCROLLDBG
                  DBG("------ %i %i\n", ev->canvas.x, ev->canvas.y);
#endif
                  for (i = 0; i < 60; i++)
                    {
                       dt = t - sid->down.history[i].timestamp;
                       if (dt > 0.2) break;
#ifdef SCROLLDBG
                       DBG("H: %i %i @ %1.3f\n",
                           sid->down.history[i].x,
                           sid->down.history[i].y, dt);
#endif
                       at += dt;
                       ax += sid->down.history[i].x;
                       ay += sid->down.history[i].y;
                    }
                  ax /= (i + 1);
                  ay /= (i + 1);
                  at /= (i + 1);
                  at /= _elm_config->thumbscroll_sensitivity_friction;
                  dx = ev->canvas.x - ax;
                  dy = ev->canvas.y - ay;
                  if (at > 0)
                    {
                       vel = sqrt((dx * dx) + (dy * dy)) / at;
                       if ((_elm_config->thumbscroll_friction > 0.0) &&
                           (vel > _elm_config->thumbscroll_momentum_threshold))
                         {
                            Evas_Coord vw, vh, aw, ah, max_d;
                            int minx, miny, mx, my, px, py;
                            double tt = 0.0, dtt = 0.0;

                            psd->api->pos_min_get
                              (sid->pan_obj, &minx, &miny);
                            psd->api->pos_max_get(sid->pan_obj, &mx, &my);
                            psd->api->pos_get(sid->pan_obj, &px, &py);
                            max_d = _elm_config->thumbscroll_flick_distance_tolerance;
                            /* TIZEN ONLY */
                            /* Original scroller used sin graph for calculating flick distance.
                               But calculated flick distance is so long.
                               So I divided into two sections. First section use sin grapth divided into 2.
                               And second section use original logic.
                               As a result, if flicking velocity is slow, flicking distance will be short.
                               */
                            if (dx > 0)
                              {
                                 if (dx > max_d) dx = max_d;
                                 if (0.4 >= ((sin((M_PI * (double)dx / max_d) - (M_PI / 2)) + 1)))
                                   sid->down.dx = ((sin((M_PI * (double)dx / max_d)
                                                          - (M_PI / 2)) + 1)) * max_d / at;
                                 else
                                    sid->down.dx = ((sin((M_PI * (double)dx / max_d)
                                                          - (M_PI / 2)) + 1) * 2) * max_d / at;
                              }
                            else
                              {
                                 if (dx < -max_d) dx = -max_d;
                                 if (-0.4 <= ((sin((M_PI * (double)dx / max_d) + (M_PI / 2)) - 1)))
                                   sid->down.dx = ((sin((M_PI * (double)dx / max_d)
                                                       + (M_PI / 2)) - 1) ) * max_d / at;
                                 else
                                   sid->down.dx = ((sin((M_PI * (double)dx / max_d)
                                                       + (M_PI / 2)) - 1) * 2) * max_d / at;
                              }
                            if (dy > 0)
                              {
                                 if (dy > max_d) dy = max_d;
                                 if (0.4 >= ((sin((M_PI * (double)dy / max_d) - (M_PI / 2)) + 1)))
                                   sid->down.dy = ((sin((M_PI * (double)dy / max_d)
                                                       - (M_PI / 2)) + 1)) * max_d / at;
                                 else
                                   sid->down.dy = ((sin((M_PI * (double)dy / max_d)
                                                       - (M_PI / 2)) + 1) * 2) * max_d / at;
                              }
                            else
                              {
                                 if (dy < -max_d) dy = -max_d;
                                 if (-0.4 <= ((sin((M_PI * (double)dy / max_d) + (M_PI / 2)) - 1)))
                                   sid->down.dy = ((sin((M_PI * (double)dy / max_d)
                                                       + (M_PI / 2)) - 1)) * max_d / at;
                                 else
                                   sid->down.dy = ((sin((M_PI * (double)dy / max_d)
                                                       + (M_PI / 2)) - 1) * 2) * max_d / at;
                              }
                            if (((sid->down.dx > 0) && (sid->down.pdx > 0)) ||
                                ((sid->down.dx < 0) && (sid->down.pdx < 0)) ||
                                ((sid->down.dy > 0) && (sid->down.pdy > 0)) ||
                                ((sid->down.dy < 0) && (sid->down.pdy < 0)))
                              {
                                 tt = ecore_loop_time_get();
                                 dtt = tt - sid->down.anim_start;

                                 if (dtt < 0.0) dtt = 0.0;
                                 else if (dtt >
                                          _elm_config->thumbscroll_friction)
                                   dtt = _elm_config->thumbscroll_friction;
                                 sid->down.extra_time =
                                   _elm_config->thumbscroll_friction - dtt;
                              }
                            else
                               sid->down.extra_time = 0.0;

                            if (_elm_config->item_scroll_align)
                              sid->down.extra_time = 0.0;

                            if ((abs(sid->down.dx) > _elm_config->thumbscroll_acceleration_threshold) &&
                                (dtt < _elm_config->thumbscroll_acceleration_time_limit) &&
                                (((sid->down.dx > 0) && (sid->down.pdx > 0)) ||
                                 ((sid->down.dx < 0) && (sid->down.pdx < 0))))
                              if (px > minx && px < mx)
                                sid->down.dx += (double)sid->down.pdx * _elm_config->thumbscroll_acceleration_weight;
                            if ((abs(sid->down.dy) > _elm_config->thumbscroll_acceleration_threshold) &&
                                (dtt < _elm_config->thumbscroll_acceleration_time_limit) &&
                                (((sid->down.dy > 0) && (sid->down.pdy > 0)) ||
                                 ((sid->down.dy < 0) && (sid->down.pdy < 0))))
                              if (py > miny && py < my)
                                sid->down.dy += (double)sid->down.pdy * _elm_config->thumbscroll_acceleration_weight;
                            aw = abs(sid->down.dx);
                            _elm_scroll_content_viewport_size_get(sid->obj, &vw, &vh);
                            if (aw  > vw * 4)
                              {
                                 if (sid->down.dx > 0) sid->down.dx = vw * 4;
                                 else sid->down.dx = -(vw * 4);
                              }
                            ah = abs(sid->down.dy);
                            if (ah  > vh * 4)
                              {
                                 if (sid->down.dy > 0) sid->down.dy = vh * 4;
                                 else sid->down.dy = -(vh * 4);
                              }

                            if (psd->api->pos_adjust || sid->cb_func.scroll_position_get)
                              {
                                 Evas_Coord pos_x, pos_y;

                                 pos_x = sid->down.dx;
                                 pos_y = sid->down.dy;

                                 pos_x = _round(pos_x * (_elm_config->thumbscroll_friction +
                                                         sid->down.extra_time), 0);
                                 pos_y = _round(pos_y * (_elm_config->thumbscroll_friction +
                                                         sid->down.extra_time), 0);
                                 if (pos_x < 0)
                                   {
                                      if ((px - pos_x) > mx)
                                        pos_x = px - mx;
                                   }
                                 else
                                   {
                                      if ((px - pos_x) < minx)
                                        pos_x = px - minx;
                                   }
                                 if (pos_y < 0)
                                   {
                                      if ((py - pos_y) > my)
                                        pos_y = py - my;
                                   }
                                 else
                                   {
                                      if ((py - pos_y) < miny)
                                        pos_y = py - miny;
                                   }

                                 if (sid->cb_func.scroll_position_get)
                                    sid->cb_func.scroll_position_get(sid->obj, &pos_x, &pos_y);
                                 else
                                    psd->api->pos_adjust(sid, &pos_x, &pos_y);

                                 sid->down.dx = pos_x;
                                 sid->down.dy = pos_y;
                              }

                            sid->down.pdx = sid->down.dx;
                            sid->down.pdy = sid->down.dy;
                            ox = -sid->down.dx;
                            oy = -sid->down.dy;

                            if (sid->is_page_flick && _paging_is_enabled(sid))
                              {
                                 int x = 0, y = 0, pos_x = 0, pos_y = 0;
                                 _elm_scroll_content_pos_get(sid->obj, &pos_x, &pos_y);

                                 x = _elm_scroll_page_x_get(sid, ox * (_elm_config->thumbscroll_friction + sid->down.extra_time) , EINA_FALSE);
                                 y = _elm_scroll_page_y_get(sid, oy * (_elm_config->thumbscroll_friction + sid->down.extra_time) , EINA_FALSE);

                                 sid->down.dx = pos_x - x;
                                 sid->down.dy = pos_y - y;
                              }

                            if (sid->is_page_flick || !_paging_is_enabled(sid))
                              {
                                 if ((!sid->down.momentum_animator) &&
                                     (!sid->momentum_animator_disabled) &&
                                     (sid->obj) &&
                                     (!elm_widget_drag_child_locked_y_get
                                        (sid->obj)))
                                   {
                                      sid->down.momentum_animator =
                                        ecore_animator_add
                                          (_elm_scroll_momentum_animator, sid);
                                      ev->event_flags |=
                                        EVAS_EVENT_FLAG_ON_SCROLL;
                                      _elm_scroll_anim_start(sid);
                                   }
                                 sid->down.anim_start = ecore_loop_time_get();
                                 _elm_scroll_content_pos_get(sid->obj, &x, &y);
                                 sid->down.sx = x;
                                 sid->down.sy = y;
                                 sid->down.b0x = 0;
                                 sid->down.b0y = 0;
                              }
                         }
                  }
               }
             else
               {
                  sid->down.pdx = 0;
                  sid->down.pdy = 0;
               }
             evas_event_feed_hold(e, 0, ev->timestamp, ev->data);
            if (!sid->down.momentum_animator && _paging_is_enabled(sid))
               {
                  Evas_Coord pgx, pgy;

                  _elm_scroll_content_pos_get(sid->obj, &x, &y);
                  if ((!sid->obj) ||
                      (!elm_widget_drag_child_locked_x_get
                         (sid->obj)))
                    {
                       //TIZEN ONLY
                       if (sid->down.dir_x && sid->pagesize_h > 0)
                         {
                            if (ox > 0)
                              {
                                if (sid->down.pageflick_h < 0)
                                  ox = sid->pagesize_h + sid->down.pageflick_h;
                                else
                                  ox = sid->pagesize_h * 0.5;
                              }
                            else if (ox < 0)
                              {
                                if (sid->down.pageflick_h > 0)
                                  ox = -(sid->pagesize_h - sid->down.pageflick_h + 1);
                                else
                                  ox = -(sid->pagesize_h * 0.5 + 1);
                              }
                         }
                       pgx = _elm_scroll_page_x_get(sid, ox, EINA_TRUE);
                       if (pgx != x &&
                           !(sid->block &
                             ELM_SCROLLER_MOVEMENT_BLOCK_HORIZONTAL))
                         {
                            ev->event_flags |= EVAS_EVENT_FLAG_ON_SCROLL;
                            _elm_scroll_scroll_to_x
                               (sid, _elm_config->page_scroll_friction, pgx);
                         }
                    }
                  if ((!sid->obj) ||
                      (!elm_widget_drag_child_locked_y_get
                         (sid->obj)))
                    {
                       //TIZEN ONLY
                       if (sid->down.dir_y && sid->pagesize_v > 0)
                         {
                            if (oy > 0)
                              {
                                if (sid->down.pageflick_v < 0)
                                  oy = sid->pagesize_v + sid->down.pageflick_v;
                                else
                                  oy = sid->pagesize_v * 0.5;
                              }
                            else if (oy < 0)
                              {
                                if (sid->down.pageflick_v > 0)
                                  oy = -(sid->pagesize_v - sid->down.pageflick_v + 1);
                                else
                                  oy = -(sid->pagesize_v * 0.5 + 1);
                              }
                         }
                       pgy = _elm_scroll_page_y_get(sid, oy, EINA_TRUE);
                       if (pgy != y &&
                           !(sid->block &
                             ELM_SCROLLER_MOVEMENT_BLOCK_VERTICAL))
                         {
                            ev->event_flags |= EVAS_EVENT_FLAG_ON_SCROLL;
                            _elm_scroll_scroll_to_y
                               (sid, _elm_config->page_scroll_friction, pgy);
                         }
                    }
               }
          }
        else
          {
             sid->down.pdx = 0;
             sid->down.pdy = 0;
             if (_paging_is_enabled(sid))
               {
                  Evas_Coord pgx, pgy;

                  _elm_scroll_content_pos_get(sid->obj, &x, &y);
                  if ((!sid->obj) ||
                      (!elm_widget_drag_child_locked_x_get
                         (sid->obj)))
                    {
                       pgx = _elm_scroll_page_x_get(sid, ox, EINA_TRUE);
                       if (pgx != x)
                         _elm_scroll_scroll_to_x
                           (sid, _elm_config->page_scroll_friction, pgx);
                    }
                  if ((!sid->obj) ||
                      (!elm_widget_drag_child_locked_y_get
                         (sid->obj)))
                    {
                       pgy = _elm_scroll_page_y_get(sid, oy, EINA_TRUE);
                       if (pgy != y)
                         _elm_scroll_scroll_to_y
                           (sid, _elm_config->page_scroll_friction, pgy);
                    }
               }
          }
        if (!sid->down.momentum_animator && (psd->api->pos_adjust || sid->cb_func.scroll_position_get))
          {
             Evas_Coord pos_x = 0, pos_y = 0;
             Evas_Coord adjust_x, adjust_y;
             Eina_Bool scroll_x = EINA_TRUE, scroll_y = EINA_TRUE;

             if (sid->cb_func.scroll_position_get)
                sid->cb_func.scroll_position_get(sid->obj, &pos_x, &pos_y);
             else
                psd->api->pos_adjust(sid, &pos_x, &pos_y);

             _elm_scroll_content_pos_get(sid->obj, &adjust_x, &adjust_y);
             _elm_scroll_can_position_adjust(sid, &pos_x, &pos_y, &scroll_x, &scroll_y);

             pos_y = -pos_y;
             adjust_y += pos_y;

             pos_x = -pos_x;
             adjust_x += pos_x;

             if (scroll_y)
                _elm_scroll_scroll_to_y(sid, _elm_config->bring_in_scroll_friction, adjust_y);
             if (scroll_x)
                _elm_scroll_scroll_to_x(sid, _elm_config->bring_in_scroll_friction, adjust_x);
          }
        if (sid->down.hold_animator)
          {
             ecore_animator_del(sid->down.hold_animator);
             sid->down.hold_animator = NULL;
             if (sid->content_info.resized)
               _elm_scroll_wanted_region_set(sid->obj);
          }
        if (sid->down.scroll)
          {
             ev->event_flags |= EVAS_EVENT_FLAG_ON_SCROLL;
             sid->down.scroll = EINA_FALSE;
          }
        if (sid->down.hold)
          {
             ev->event_flags |= EVAS_EVENT_FLAG_ON_HOLD;
             sid->down.hold = EINA_FALSE;
          }
        sid->down.dragged_began = EINA_FALSE;
        sid->down.dir_x = EINA_FALSE;
        sid->down.dir_y = EINA_FALSE;
        sid->down.want_dragged = EINA_FALSE;
        sid->down.dragged = EINA_FALSE;
        sid->down.now = EINA_FALSE;
        _elm_scroll_content_pos_get(sid->obj, &x, &y);
        _elm_scroll_content_pos_set(sid->obj, x, y, EINA_TRUE);
        _elm_scroll_wanted_coordinates_update(sid, x, y);

        if (sid->content_info.resized)
          _elm_scroll_wanted_region_set(sid->obj);

        if (!_paging_is_enabled(sid))
          _elm_scroll_bounce_eval(sid);
     }
}

static void
_elm_scroll_mouse_down_event_cb(void *data,
                                Evas *e __UNUSED__,
                                Evas_Object *obj __UNUSED__,
                                void *event_info)
{
   Elm_Scrollable_Smart_Interface_Data *sid;
   Evas_Event_Mouse_Down *ev;
   Evas_Coord x = 0, y = 0;

   sid = data;
   ev = event_info;

#ifdef SMOOTHDBG
   if (getenv("ELS_SCROLLER_SMOOTH_DEBUG")) _elm_scroll_smooth_debug = 1;
   if (_elm_scroll_smooth_debug) _elm_scroll_smooth_debug_init();
#endif

   if ((sid->block & ELM_SCROLLER_MOVEMENT_BLOCK_VERTICAL) &&
       (sid->block & ELM_SCROLLER_MOVEMENT_BLOCK_HORIZONTAL))
     return;

   //REDWOOD ONLY
   edje_object_signal_emit(sid->edje_obj, "elm,state,mouse,down", "elm");

   if (_elm_config->thumbscroll_enable)
     {
        sid->down.hold = EINA_FALSE;
        if ((sid->down.bounce_x_animator) || (sid->down.bounce_y_animator) ||
            (sid->down.momentum_animator) || (sid->scrollto.x.animator) ||
            (sid->scrollto.y.animator))
          {
             ev->event_flags |= EVAS_EVENT_FLAG_ON_SCROLL |
               EVAS_EVENT_FLAG_ON_HOLD;
             sid->down.scroll = EINA_TRUE;
             sid->down.hold = EINA_TRUE;
             _elm_scroll_anim_stop(sid, EINA_TRUE);
          }
        if (sid->scrollto.x.animator)
          {
             ecore_animator_del(sid->scrollto.x.animator);
             sid->scrollto.x.animator = NULL;
          }
        if (sid->scrollto.y.animator)
          {
             ecore_animator_del(sid->scrollto.y.animator);
             sid->scrollto.y.animator = NULL;
          }
        if (sid->down.bounce_x_animator)
          {
             ecore_animator_del(sid->down.bounce_x_animator);
             sid->down.bounce_x_animator = NULL;
             sid->bouncemex = EINA_FALSE;
             if (sid->content_info.resized)
               _elm_scroll_wanted_region_set(sid->obj);
          }
        if (sid->down.bounce_y_animator)
          {
             ecore_animator_del(sid->down.bounce_y_animator);
             sid->down.bounce_y_animator = NULL;
             sid->bouncemey = EINA_FALSE;
             if (sid->content_info.resized)
               _elm_scroll_wanted_region_set(sid->obj);
          }
        if (sid->down.hold_animator)
          {
             ecore_animator_del(sid->down.hold_animator);
             sid->down.hold_animator = NULL;
             _elm_scroll_drag_stop(sid);
             if (sid->content_info.resized)
               _elm_scroll_wanted_region_set(sid->obj);
          }
        if (sid->down.momentum_animator)
          {
             ecore_animator_del(sid->down.momentum_animator);
             sid->down.momentum_animator = NULL;
             sid->down.bounce_x_hold = EINA_FALSE;
             sid->down.bounce_y_hold = EINA_FALSE;
             sid->down.ax = 0;
             sid->down.ay = 0;
             if (sid->content_info.resized)
               _elm_scroll_wanted_region_set(sid->obj);
          }
        if (ev->button == 1)
          {
             if (_paging_is_enabled(sid))
               {
                  sid->down.pageflick_h = 0;
                  sid->down.pageflick_v = 0;
               }
             sid->down.hist.est_timestamp_diff =
               ecore_loop_time_get() - ((double)ev->timestamp / 1000.0);
             sid->down.hist.tadd = 0.0;
             sid->down.hist.dxsum = 0.0;
             sid->down.hist.dysum = 0.0;
             sid->down.now = EINA_TRUE;
             sid->down.want_dragged = EINA_FALSE;
             sid->down.dragged = EINA_FALSE;
             sid->down.dir_x = EINA_FALSE;
             sid->down.dir_y = EINA_FALSE;
             sid->down.x = ev->canvas.x;
             sid->down.y = ev->canvas.y;
             _elm_scroll_content_pos_get(sid->obj, &x, &y);
             sid->down.sx = x;
             sid->down.sy = y;
             sid->down.locked = EINA_FALSE;
             memset(&(sid->down.history[0]), 0,
                    sizeof(sid->down.history[0]) * ELM_SCROLL_HISTORY_SIZE);
#ifdef EVTIME
             sid->down.history[0].timestamp = ev->timestamp / 1000.0;
             sid->down.history[0].localtimestamp = ecore_loop_time_get();
#else
             sid->down.history[0].timestamp = ecore_loop_time_get();
#endif
             sid->down.dragged_began_timestamp = sid->down.history[0].timestamp;
             sid->down.history[0].x = ev->canvas.x;
             sid->down.history[0].y = ev->canvas.y;
          }
        sid->down.dragged_began = EINA_FALSE;
        sid->down.hold_parent = EINA_FALSE;
        sid->down.cancelled = EINA_FALSE;
        if (sid->hold || sid->freeze)
          sid->down.want_reset = EINA_TRUE;
        else
          sid->down.want_reset = EINA_FALSE;
     }
}

static Eina_Bool
_elm_scroll_can_scroll(Elm_Scrollable_Smart_Interface_Data *sid,
                       int dir)
{
   Evas_Coord mx = 0, my = 0, px = 0, py = 0, minx = 0, miny = 0;

   if (!sid->pan_obj) return EINA_FALSE;

   ELM_PAN_DATA_GET_OR_RETURN_VAL(sid->pan_obj, psd, EINA_FALSE);

   psd->api->pos_max_get(sid->pan_obj, &mx, &my);
   psd->api->pos_min_get(sid->pan_obj, &minx, &miny);
   psd->api->pos_get(sid->pan_obj, &px, &py);
   switch (dir)
     {
      case LEFT:
        if (px > minx) return EINA_TRUE;
        break;

      case RIGHT:
        if ((px - minx) < mx) return EINA_TRUE;
        break;

      case UP:
        if (py > miny) return EINA_TRUE;
        break;

      case DOWN:
        if ((py - miny) < my) return EINA_TRUE;
        break;

      default:
        break;
     }
   return EINA_FALSE;
}

static Eina_Bool
_elm_scroll_post_event_move(void *data,
                            Evas *e __UNUSED__)
{
   Elm_Scrollable_Smart_Interface_Data *sid = data;
   Eina_Bool horiz, vert;

   if (sid->down.want_dragged)
     {
        int start = 0;

        elm_widget_parents_bounce_get(sid->obj, &horiz, &vert);

        ERR("[DDO] obj(%x), type(%s)", sid->obj, elm_widget_type_get(sid->obj));
        ERR("[DDO] hold_parent(%d)", sid->down.hold_parent);
        if (sid->down.hold_parent)
          {
             if ((sid->down.dir_x) && (horiz || !sid->bounce_horiz) &&
                 !_elm_scroll_can_scroll(sid, sid->down.hdir))
               {
                  sid->down.dir_x = EINA_FALSE;
                  ERR("[DDO] down.dir_x(%d)", sid->down.dir_x);
               }
             if ((sid->down.dir_y) && (vert || !sid->bounce_vert) &&
                 !_elm_scroll_can_scroll(sid, sid->down.vdir))
               {
                  sid->down.dir_y = EINA_FALSE;
                  ERR("[DDO] down.dir_y(%d)", sid->down.dir_y);
               }
             sid->down.dragged_began = EINA_TRUE;
          }
        if (sid->down.dir_x)
          {
             if ((!sid->obj) ||
                 (!elm_widget_drag_child_locked_x_get(sid->obj)))
               {
                  if (sid->down.dragged_began)
                    {
                       sid->down.want_dragged = EINA_FALSE;
                       sid->down.dragged = EINA_TRUE;
                       if (sid->obj)
                         {
                            ERR("[DDO] elm_widget_drag_lock_x_set : obj(%x), type(%s)", sid->obj, elm_widget_type_get(sid->obj));
                            elm_widget_drag_lock_x_set(sid->obj, 1);
                         }
                       start = 1;
                    }
               }
             else
               {
                  sid->down.dragged_began = EINA_TRUE;
                  sid->down.dir_x = EINA_FALSE;
               }
          }
        if (sid->down.dir_y)
          {
             if ((!sid->obj) ||
                 (!elm_widget_drag_child_locked_y_get(sid->obj)))
               {
                  if (sid->down.dragged_began)
                    {
                       sid->down.want_dragged = EINA_FALSE;
                       sid->down.dragged = EINA_TRUE;
                       if (sid->obj)
                         {
                            ERR("[DDO] elm_widget_drag_lock_y_set : obj(%x), type(%s)", sid->obj, elm_widget_type_get(sid->obj));
                            elm_widget_drag_lock_y_set
                               (sid->obj, EINA_TRUE);
                         }
                       start = 1;
                    }
               }
             else
               {
                  sid->down.dragged_began = EINA_TRUE;
                  sid->down.dir_y = EINA_FALSE;
               }
          }
        if ((!sid->down.dir_x) && (!sid->down.dir_y))
          {
             sid->down.cancelled = EINA_TRUE;
          }
        if (start) _elm_scroll_drag_start(sid);
     }

   return EINA_TRUE;
}

static void
_elm_scroll_down_coord_eval(Elm_Scrollable_Smart_Interface_Data *sid,
                            Evas_Coord *x,
                            Evas_Coord *y)
{
   Evas_Coord minx, miny;

   if (!sid->pan_obj) return;

   ELM_PAN_DATA_GET_OR_RETURN(sid->pan_obj, psd);

   if (_paging_is_enabled(sid))
     {
        sid->down.pageflick_h = *x - sid->down.x;
        sid->down.pageflick_v = *y - sid->down.y;
     }

   if (sid->down.dir_x) *x = sid->down.sx - (*x - sid->down.x);
   else *x = sid->down.sx;
   if (sid->down.dir_y) *y = sid->down.sy - (*y - sid->down.y);
   else *y = sid->down.sy;

   if ((sid->down.dir_x) || (sid->down.dir_y))
     {
        if (!((sid->down.dir_x) && (sid->down.dir_y)))
          {
             if (sid->down.dir_x) *y = sid->down.locked_y;
             else *x = sid->down.locked_x;
          }
     }

   psd->api->pos_min_get(sid->pan_obj, &minx, &miny);

   if (!sid->loop_h && *x < minx)
     *x += (minx - *x) * _elm_config->thumbscroll_border_friction;
   else if (!sid->loop_h && sid->content_info.w <= sid->w)
     *x += (sid->down.sx - *x) * _elm_config->thumbscroll_border_friction;
   else if (!sid->loop_h && (sid->content_info.w - sid->w + minx) < *x)
     *x += (sid->content_info.w - sid->w + minx - *x) *
       _elm_config->thumbscroll_border_friction;

   if (!sid->loop_v && *y < miny)
     *y += (miny - *y) * _elm_config->thumbscroll_border_friction;
   else if (!sid->loop_v && sid->content_info.h <= sid->h)
     *y += (sid->down.sy - *y) * _elm_config->thumbscroll_border_friction;
   else if (!sid->loop_v && (sid->content_info.h - sid->h + miny) < *y)
     *y += (sid->content_info.h - sid->h + miny - *y) *
       _elm_config->thumbscroll_border_friction;
}

//TIZEN ONLY : for scroll smooth algorithm

#define iround(x) ((x)>=0?(int)((x)+0.5):(int)((x)-0.5))

//> Frequency of move events
#define ELM_MOVE_PER_SECOND (90.75f)
//>Time between two subsequent events
static const float ELM_MOVE_TIMEOUT = (1.0f/ELM_MOVE_PER_SECOND);

//>Quantity of move events in time interval
#define ELM_MOVE_COUNT(t) iround((t)*ELM_MOVE_PER_SECOND)

//>Getting coordinate (x is 0, y is 1) from struct Elm_Scroll_Pos or Elm_Scroll_History_Item.
#define ELM_GET_COORD(p, coord) (*((int *)p + coord))

//>Difference of coordinates of points with indexes ind and ind+inc.
#define ELM_DIFF(p, coord, ind, inc)  (ELM_GET_COORD((p + ind), coord) - ELM_GET_COORD((p + ind + inc), coord))

//>Index in array for calculation of smoothed velocity.
#define ELM_SMOOTH_SPEED_INDEX 9

//>in seconds, includes driver to X time gap
#define EXTRA_PREDIOCTION_TIME (-0.005)

//>Calculating current velocity. In normal situation it is (pos[0].x - pos[ELM_SMOOTH_SPEED_INDEX].x) / (pos[0].t - pos[ELM_SMOOTH_SPEED_INDEX].t).
//>But in situations when: 1. Sign of velocity was changes or 2. The acceleration is of constant sign or 3. There was stop of movement
//>The index ELM_SMOOTH_SPEED_INDEX is replaced by 1.
double _elm_scroll_get_v(Elm_Scroll_Pos *pos, int num, int coord,
                         double *dt, int *pdiff, double *padt, Elm_Scroll_Predict *predict)
{
   double v = 0;
   int nmeasure_idx = (num > ELM_SMOOTH_SPEED_INDEX) ? ELM_SMOOTH_SPEED_INDEX : (num -1);
   // a recent input takes higher priority
#if USE_HISTORY_WEIGHT
   if (nmeasure_idx >= 2)
     *pdiff = (ELM_DIFF(pos, coord, 0, 1) * 2 + ELM_DIFF(pos, coord, 1, 1))/3;
   else
#else
     *pdiff = ELM_DIFF(pos, coord, 0, 1);
#endif

#if ADJUST_EVENT_TIME
   *dt = -ELM_MOVE_TIMEOUT;
#else
   if (nmeasure_idx >= 2)
       *dt = -(pos[2].t - pos[0].t)/2.;
   else
       *dt = -(pos[1].t - pos[0].t);
#endif
   if (*dt == 0)
     *dt = -ELM_MOVE_TIMEOUT;
   v = *pdiff / *dt;
   predict->k[coord] = 0;
   *padt = EXTRA_PREDIOCTION_TIME;

   return v;
}

static Eina_Bool
_elm_scroll_get_pos(Elm_Scrollable_Smart_Interface_Data *sid,
                                     Elm_Scroll_Pos *pos, int num, int *fx, int *fy)
{
   double vx = 0, vy = 0, dt = 0, dtx = 0, dty = 0;

   if (num < 1)
     {
        return EINA_FALSE;
     }
   else if (num == 1)
     {
        *fx = iround((double)pos[0].x);
        *fy = iround((double)pos[0].y);
     }
   else if (num >= 2)
     {
        int diffx, diffy;
        if (sid->down.dir_y)
          vy = _elm_scroll_get_v(pos, num, 1, &dt, &diffy, &dty, &sid->down.predict);
        if (sid->down.dir_x)
          vx = _elm_scroll_get_v(pos, num, 0, &dt, &diffx, &dtx, &sid->down.predict);
     }

   if (sid->down.dir_x)
     {
        *fx = iround((double)pos[0].x - (pos[0].t + dtx) * vx);
        // don't go back even though over-run is detected.
        if (sid->down.anim_vx_prev && sid->down.anim_vx_prev * vx >= 0)
          if (vx == 0 || (vx > 0 && *fx  > sid->down.anim_x_prev) ||
              (vx < 0 && *fx  < sid->down.anim_x_prev))
            *fx = sid->down.anim_x_prev;
     }

   if (sid->down.dir_y)
     {
        *fy = iround((double)pos[0].y - (pos[0].t + dty) * vy);
        // don't go back even though over-run is detected.
        if (sid->down.anim_vy_prev && sid->down.anim_vy_prev * vy >= 0)
          if (vy == 0 || (vy > 0 && *fy  > sid->down.anim_y_prev) ||
              (vy < 0 && *fy  < sid->down.anim_y_prev))
            *fy = sid->down.anim_y_prev;
     }

   sid->down.anim_x_prev = *fx;
   sid->down.anim_y_prev = *fy;
   sid->down.anim_vx_prev = vx;
   sid->down.anim_vy_prev = vy;

   return EINA_TRUE;
}

#define COMPENSATE_FOR_INITIAL_RENDER_DELAY 0
#define ADJUST_ANIMATOR_TIMING              1
#define SMART_SMOOTH_START                  1
#define HOLD_ANIMATOR_DEBUG_LEVEL1          0
#define HOLD_ANIMATOR_DEBUG_LEVEL2          0
#define HOLD_ANIMATOR_DEBUG_X_AXIS          1
#define USE_LOOP_TIME                       0
#define ADJUST_EVENT_TIME                   0
#define USE_HISTORY_WEIGHT                  0
#define PREDICT_WHEN_PAUSED                 0

static Eina_Bool
_elm_scroll_hold_animator(void *data)
{
   Elm_Scrollable_Smart_Interface_Data *sid = data;
   Evas_Coord ox = 0, oy = 0, fx = 0, fy = 0, x = 0, y = 0, num = 0;
   Evas_Coord fx_coord = 0, fy_coord = 0;
   fx = sid->down.hold_x;
   fy = sid->down.hold_y;
   fx_coord = sid->down.anim_x_coord_prev;
   fy_coord = sid->down.anim_y_coord_prev;
#define QUEUE_SIZE 3 /* for event queue size */
   Elm_Scroll_Pos pos[QUEUE_SIZE];

   double now, now_diff, prev;
   double animators_frametime=0, d = 0;

   sid->down.anim_count++;

   ERR("[DDO] obj(%x), locked_x(%d)", sid->obj, elm_widget_drag_child_locked_x_get(sid->obj));

#if COMPENSATE_FOR_INITIAL_RENDER_DELAY
   Ecore_Animator_Source source = ecore_animator_source_get();
#endif

   // FIXME: assume server and client have the same "timezone"
   // (0 timepoint) for now. this needs to be figured out in advance
   // though.
#if USE_LOOP_TIME
   now = ecore_loop_time_get();
#else
   now = ecore_time_get();
#endif

   // init variables for the first animator run
   if (sid->down.anim_count == 1)
     {
        sid->down.anim_t_prev = now;
        sid->down.anim_x_coord_prev = fx;
        sid->down.anim_y_coord_prev = fy;
        fx_coord = fx;
        fy_coord = fy;
     }
   prev = sid->down.anim_t_prev;
   now_diff = now - prev;
   animators_frametime = ecore_animator_frametime_get();
#if COMPENSATE_FOR_INITIAL_RENDER_DELAY
   if (sid->down.anim_count == 1)
     {
        if (source != ECORE_ANIMATOR_SOURCE_CUSTOM)
          sid->down.anim_t_delay = fmod(now, animators_frametime);
     }
   else
#endif
     {
        sid->down.anim_t_delay += now_diff - animators_frametime;
     }

   // skip this turn if specified.
   if (sid->down.anim_skip > 0)
     {
        sid->down.anim_skip--;
#if HOLD_ANIMATOR_DEBUG_LEVEL1
        DBG("[%03d/%s] %.4f %.3f/%.3f dt:%.3f = %.3f%+.3f ev: %.4f skip(%d)\n",
            sid->down.anim_count, (source == ECORE_ANIMATOR_SOURCE_CUSTOM) ? "V" : "T",
            now, sid->down.anim_t_delay * 1000, sid->down.anim_t_adjusted * 1000,
            now_diff*1000,
            (now_diff - d)*1000,
            d*1000,
            sid->down.history[0].timestamp,
            sid->down.anim_skip);
#endif
            if ((now_diff * 1000) > 18)
              {
#if ADJUST_ANIMATOR_TIMING
                 sid->down.anim_t_dont_adjust = 1;
#endif
                 goto update_time_and_quit;
              }
     }

   // We don't need to process old events again.
   if (sid->down.anim_count != 1 &&
           sid->down.anim_pos_t_prev == sid->down.history[0].timestamp)
     {
#if HOLD_ANIMATOR_DEBUG_LEVEL1
        DBG("[%03d/%s] %.4f %.3f/%.3f dt:%.3f = %.3f%+.3f ev:%.4f skip(%d) no events.\n",
            sid->down.anim_count, (source == ECORE_ANIMATOR_SOURCE_CUSTOM) ? "V" : "T",
            now, sid->down.anim_t_delay * 1000, sid->down.anim_t_adjusted * 1000,
            now_diff*1000,
            (now_diff - d)*1000,
            d*1000,
            sid->down.history[0].timestamp,
            sid->down.anim_skip);
#endif

        goto update_time_and_quit;
   }

#if ADJUST_ANIMATOR_TIMING
   if (sid->down.anim_count != 1 && !sid->down.anim_t_dont_adjust)
     {
        if (now_diff < animators_frametime*3/4)
          {
             d += animators_frametime/4;
          }
        else if (now_diff < animators_frametime)
          {
             d = animators_frametime - now_diff;
          }
        else if (now_diff < animators_frametime*5/4)
          {
             d = -(now_diff - animators_frametime);
          }
        else
          {
             d = -animators_frametime/4;
          }

        now += d;
        now_diff +=d;
        sid->down.anim_t_adjusted += d;
     }

   sid->down.anim_t_dont_adjust = 0;

#endif

#if COMPENSATE_FOR_INITIAL_RENDER_DELAY
   // in case the first animator is called manually to make up for gpu's wake-up time,
   // then the second animator should be skipped.
   if ((sid->down.anim_count == 1) && (sid->down.anim_skip <= 0))
     sid->down.anim_skip++;
#endif

   if ((!sid->hold) && (!sid->freeze) &&
       _elm_config->scroll_smooth_time_interval > 0.0)
     {
        int i = 0;
        int ncur_diff_x=0, ncur_diff_y=0;
        int nprev_diff_x=0, nprev_diff_y=0;

        for (i = 0; i < QUEUE_SIZE; i++)
          {
             if (sid->down.history[i].timestamp >=
                 sid->down.dragged_began_timestamp)
               {
                  x = sid->down.history[i].x;
                  y = sid->down.history[i].y;

                  //if there is no history value, we don't deal with it if
                  //there is better wat to know existance of history value
                  //, I will modify this code to it
                  if ((x == 0) && (y == 0))
                    {
                       break;
                    }

                  pos[i].x = x;
                  pos[i].y = y;
                  pos[i].t = now - sid->down.history[i].timestamp;
                  num++;

                  if (num <= 1)
                      continue;

                  // get rid of histories with different move direction.
                  if (sid->down.dir_x)
                    ncur_diff_x = pos[i].x - pos[i-1].x;
                  if (sid->down.dir_y)
                    ncur_diff_y = pos[i].y - pos[i-1].y;

                  if (ncur_diff_x * nprev_diff_x < 0 || ncur_diff_y * nprev_diff_y < 0)
                    {
                       num--;
#if HOLD_ANIMATOR_DEBUG_LEVEL2
                       DBG("[%03d] i=%d a dir change detected. stopped.\n",
                           sid->down.anim_count, i);
#endif
                       break;
                    }

                  nprev_diff_x = ncur_diff_x;
                  nprev_diff_y = ncur_diff_y;

#if ADJUST_EVENT_TIME
                  //> a event is delayed??
                  if (ELM_MOVE_COUNT(pos[i].t - pos[i-1].t) <= 0)
                    {
                       DBG("[%03d] i=%d a event is delayed (%f) ??????????????????\n",
                           sid->down.anim_count,
                           i, pos[i].t - pos[i-1].t);
                    }

                  //> a pause in movement detected
                  if (ELM_MOVE_COUNT(pos[i].t - pos[i-1].t) >= 2)
                    {
#if HOLD_ANIMATOR_DEBUG_LEVEL2
                       DBG("[%03d] i=%d a pause(%f) in movement detected. stopped.\n",
                           sid->down.anim_count, i, pos[i].t - pos[i-1].t);
#endif
#if PREDICT_WHEN_PAUSED
                       pos[i].t = pos[i-1].t + ELM_MOVE_TIMEOUT;
#else
                       num--;
#endif
                       break;
                    }
#endif
               }
          }

#if ADJUST_EVENT_TIME
        // X server fills in time in milisecond order and it rounds off.
        // let's makt it more precise and this is proven by machine moving at constant speed.
        //double pos_diff = 0;
        if (num >= 2)
          {
             double tmp = sid->down.history[0].timestamp;
             for (i = num - 1; i >= 1; i--)
               {
                  pos[i-1].t = pos[i].t - ELM_MOVE_TIMEOUT;
                  sid->down.history[i-1].timestamp = now - pos[i-1].t;
               }
             //pos_diff = sid->down.history[0].timestamp - tmp;
          }

#endif
        sid->down.anim_pos_t_prev = sid->down.history[0].timestamp;

        //TIZEN ONLY : for scroll smooth algorithm
        _elm_scroll_get_pos(sid, pos, num, &fx, &fy);

        fx_coord = fx;
        fy_coord = fy;
        _elm_scroll_down_coord_eval(sid, &fx_coord, &fy_coord);

     }

   _elm_scroll_content_pos_get(sid->obj, &ox, &oy);
   if (sid->down.dir_x)
     {
        if ((!sid->obj) ||
            (!elm_widget_drag_child_locked_x_get(sid->obj)))
            {
            ERR("[DDO] obj(%x)", sid->obj);
          ox = fx_coord;
     }
     }
   if (sid->down.dir_y)
     {
        if ((!sid->obj) ||
            (!elm_widget_drag_child_locked_y_get(sid->obj)))
            {
            ERR("[DDO] obj(%x)", sid->obj);            
          oy = fy_coord;
     }
     }

#ifdef SMOOTHDBG
   if (_elm_scroll_smooth_debug)
     _elm_scroll_smooth_debug_movetime_add(ox, oy);
#endif

   _elm_scroll_content_pos_set(sid->obj, ox, oy, EINA_TRUE);

#if HOLD_ANIMATOR_DEBUG_LEVEL1
#if HOLD_ANIMATOR_DEBUG_X_AXIS
   DBG("[%03d/%s] %.4f %.3f/%.3f dt:%.3f = %.3f%+.3f ev:%02d %3.4f/%+.3f p:%d(%d) = %d%+d %d %d\n",
       sid->down.anim_count, (source == ECORE_ANIMATOR_SOURCE_CUSTOM) ? "V" : "T",
       now, sid->down.anim_t_delay * 1000, sid->down.anim_t_adjusted * 1000,
       now_diff*1000,
       (now_diff - d)*1000,
       d*1000,
       num, sid->down.history[0].timestamp, pos_diff*1000,
       ox, fx,
       sid->down.hold_x, ox - sid->down.hold_x,
       ox - sid->down.anim_x_coord_prev,
       (int)sid->down.anim_vx_prev);
#else
   DBG("[%03d/%s] %.4f %.3f/%.3f dt:%.3f = %.3f%+.3f ev:%02d %3.4f/%+.3f p:%d(%d) = %d%+d %d %d\n",
       sid->down.anim_count, (source == ECORE_ANIMATOR_SOURCE_CUSTOM) ? "V" : "T",
       now, sid->down.anim_t_delay * 1000, sid->down.anim_t_adjusted * 1000,
       now_diff*1000,
       (now_diff - d)*1000,
       d*1000,
       num, sid->down.history[0].timestamp, pos_diff*1000,
       oy, fy,
       sid->down.hold_y, oy - sid->down.hold_y,
       oy - sid->down.anim_y_coord_prev,
       (int)sid->down.anim_vy_prev);
#endif
#endif

   sid->down.anim_x_coord_prev = ox;
   sid->down.anim_y_coord_prev = oy;

update_time_and_quit:
   sid->down.anim_t_prev = now;

   return ECORE_CALLBACK_RENEW;
}

static Eina_Bool
_elm_scroll_on_hold_animator(void *data)
{
   double t, td;
   double vx, vy;
   Evas_Coord x, y, ox, oy;
   Elm_Scrollable_Smart_Interface_Data *sid;

   sid = data;
   t = ecore_loop_time_get();
   if (sid->down.onhold_tlast > 0.0)
     {
        td = t - sid->down.onhold_tlast;
        vx = sid->down.onhold_vx * td *
          (double)_elm_config->thumbscroll_hold_threshold * 2.0;
        vy = sid->down.onhold_vy * td *
          (double)_elm_config->thumbscroll_hold_threshold * 2.0;
        _elm_scroll_content_pos_get(sid->obj, &ox, &oy);
        x = ox;
        y = oy;

        if (sid->down.dir_x)
          {
             if ((!sid->obj) ||
                 (!elm_widget_drag_child_locked_x_get(sid->obj)))
               {
                  sid->down.onhold_vxe += vx;
                  x = ox + (int)sid->down.onhold_vxe;
                  sid->down.onhold_vxe -= (int)sid->down.onhold_vxe;
               }
          }

        if (sid->down.dir_y)
          {
             if ((!sid->obj) ||
                 (!elm_widget_drag_child_locked_y_get(sid->obj)))
               {
                  sid->down.onhold_vye += vy;
                  y = oy + (int)sid->down.onhold_vye;
                  sid->down.onhold_vye -= (int)sid->down.onhold_vye;
               }
          }

        _elm_scroll_content_pos_set(sid->obj, x, y, EINA_TRUE);
     }
   sid->down.onhold_tlast = t;

   return ECORE_CALLBACK_RENEW;
}

static void
_elm_scroll_mouse_move_event_cb(void *data,
                                Evas *e,
                                Evas_Object *obj __UNUSED__,
                                void *event_info)
{
   Elm_Scrollable_Smart_Interface_Data *sid = data;
   Evas_Event_Mouse_Move *ev;
   Evas_Coord x = 0, y = 0;

   if (!sid->pan_obj) return;

   ELM_PAN_DATA_GET_OR_RETURN(sid->pan_obj, psd);

   ERR("[DDO] obj(%x), block(%d)", sid->obj, sid->block);

   if ((sid->block & ELM_SCROLLER_MOVEMENT_BLOCK_VERTICAL) &&
       (sid->block & ELM_SCROLLER_MOVEMENT_BLOCK_HORIZONTAL))
     return;

   ev = event_info;
   if (ev->event_flags & EVAS_EVENT_FLAG_ON_HOLD)
   	{
   	ERR("[DDO] obj(%x), EVAS_EVENT_FLAG_ON_HOLD", sid->obj);
     sid->down.hold_parent = EINA_TRUE;
   	}
   evas_post_event_callback_push(e, _elm_scroll_post_event_move, sid);

   ERR("[DDO] obj(%x), ev->cur.canvas.x(%d) ev->cur.canvas.y(%d)", sid->obj, ev->cur.canvas.x, ev->cur.canvas.y);
   ERR("[DDO] obj(%x), hold(%d) freeze(%d)", sid->obj, sid->hold, sid->freeze);

   // FIXME: respect elm_widget_scroll_hold_get of parent container
   if (_elm_config->thumbscroll_enable)
     {
        if (sid->down.now)
          {
             //REDWOOD ONLY
             edje_object_signal_emit(sid->edje_obj, "elm,state,mouse,move", "elm");

             if ((sid->scrollto.x.animator) && (!sid->hold) && (!sid->freeze) &&
                 !(sid->block & ELM_SCROLLER_MOVEMENT_BLOCK_HORIZONTAL))
               {
                  Evas_Coord px;
                  ecore_animator_del(sid->scrollto.x.animator);
                  sid->scrollto.x.animator = NULL;
                  psd->api->pos_get(sid->pan_obj, &px, NULL);
                  sid->down.sx = px;
                  sid->down.x = sid->down.history[0].x;
               }

             if ((sid->scrollto.y.animator) && (!sid->hold) && (!sid->freeze) &&
                 !(sid->block & ELM_SCROLLER_MOVEMENT_BLOCK_VERTICAL))
               {
                  Evas_Coord py;
                  ecore_animator_del(sid->scrollto.y.animator);
                  sid->scrollto.y.animator = NULL;
                  psd->api->pos_get(sid->pan_obj, NULL, &py);
                  sid->down.sy = py;
                  sid->down.y = sid->down.history[0].y;
               }

#ifdef SCROLLDBG
             DBG("::: %i %i\n", ev->cur.canvas.x, ev->cur.canvas.y);
#endif
             memmove(&(sid->down.history[1]), &(sid->down.history[0]),
                     sizeof(sid->down.history[0]) * (ELM_SCROLL_HISTORY_SIZE - 1));
#ifdef EVTIME
             sid->down.history[0].timestamp = ev->timestamp / 1000.0;
             sid->down.history[0].localtimestamp = ecore_loop_time_get();
#else
             sid->down.history[0].timestamp = ecore_loop_time_get();
#endif
             sid->down.history[0].x = ev->cur.canvas.x;
             sid->down.history[0].y = ev->cur.canvas.y;

             if (!sid->down.dragged_began)
               {
                  x = ev->cur.canvas.x - sid->down.x;
                  y = ev->cur.canvas.y - sid->down.y;

                  sid->down.hdir = -1;
                  sid->down.vdir = -1;

                  if (x > 0) sid->down.hdir = LEFT;
                  else if (x < 0)
                    sid->down.hdir = RIGHT;
                  if (y > 0) sid->down.vdir = UP;
                  else if (y < 0)
                    sid->down.vdir = DOWN;

                  if (x < 0) x = -x;
                  if (y < 0) y = -y;

                  if (sid->one_direction_at_a_time)
                    {
                       if (((x * x) + (y * y)) >
                            (_elm_config->thumbscroll_threshold *
                             _elm_config->thumbscroll_threshold))
                         {
                            if (sid->one_direction_at_a_time ==
                                ELM_SCROLLER_SINGLE_DIRECTION_SOFT)
                              {
                                 int dodir = 0;
                                 if (x > (y * 2))
                                   {
                                      if (!(sid->block &
                                            ELM_SCROLLER_MOVEMENT_BLOCK_HORIZONTAL))
                                        {
                                           sid->down.dir_x = EINA_TRUE;
                                        }
                                      sid->down.dir_y = EINA_FALSE;
                                      dodir++;
                                   }
                                 if (y > (x * 2))
                                   {
                                      sid->down.dir_x = EINA_FALSE;
                                      if (!(sid->block &
                                            ELM_SCROLLER_MOVEMENT_BLOCK_VERTICAL))
                                        {
                                           sid->down.dir_y = EINA_TRUE;
                                        }
                                      dodir++;
                                   }
                                 if (!dodir)
                                   {
                                      if (!(sid->block &
                                            ELM_SCROLLER_MOVEMENT_BLOCK_HORIZONTAL))
                                        {
                                           sid->down.dir_x = EINA_TRUE;
                                        }
                                      if (!(sid->block &
                                            ELM_SCROLLER_MOVEMENT_BLOCK_VERTICAL))
                                        {
                                           sid->down.dir_y = EINA_TRUE;
                                        }
                                   }
                              }
                            else if (sid->one_direction_at_a_time ==
                                     ELM_SCROLLER_SINGLE_DIRECTION_HARD)
                              {
                                 if (x > y)
                                   {
                                      if (!(sid->block &
                                            ELM_SCROLLER_MOVEMENT_BLOCK_HORIZONTAL))
                                        {
                                           sid->down.dir_x = EINA_TRUE;
                                        }
                                      sid->down.dir_y = EINA_FALSE;
                                   }
                                 if (y > x)
                                   {
                                      sid->down.dir_x = EINA_FALSE;
                                      if (!(sid->block &
                                            ELM_SCROLLER_MOVEMENT_BLOCK_VERTICAL))
                                        {
                                           sid->down.dir_y = EINA_TRUE;
                                        }
                                   }
                              }
                         }
                    }
                  else
                    {
                       if (!(sid->block &
                             ELM_SCROLLER_MOVEMENT_BLOCK_HORIZONTAL))
                         {
                            sid->down.dir_x = EINA_TRUE;
                         }
                       if (!(sid->block &
                             ELM_SCROLLER_MOVEMENT_BLOCK_VERTICAL))
                         {
                            sid->down.dir_y = EINA_TRUE;
                         }
                    }
               }
             if ((!sid->hold) && (!sid->freeze))
               {
                  if ((sid->down.dragged) ||
                      (((x * x) + (y * y)) >
                       (_elm_config->thumbscroll_threshold *
                        _elm_config->thumbscroll_threshold)))
                    {
                       if (!sid->down.dragged_began &&
                           _elm_config->scroll_smooth_start_enable)
                         {
#if SMART_SMOOTH_START
                            int i = 0;
                            for (i = 0 ; i < 5 ; i++)
                              if (!sid->down.history[i].timestamp)
                                break;
                            if (i > 0)
                              {
                                 i--;
                                 DBG("smooth-start(-%d): %d->%d->%d->%d->%d->%d\n",
                                     i,
                                     sid->down.history[0].x,
                                     sid->down.history[1].x,
                                     sid->down.history[2].x,
                                     sid->down.history[3].x,
                                     sid->down.history[4].x,
                                     sid->down.x);
                                 sid->down.x = sid->down.history[i].x;
                                 sid->down.y = sid->down.history[i].y;
                                 sid->down.dragged_began_timestamp = sid->down.history[i].timestamp;

                              }
#else
                            sid->down.x = ev->cur.canvas.x;
                            sid->down.y = ev->cur.canvas.y;
#ifdef EVTIME
                            sid->down.dragged_began_timestamp =
                               ev->timestamp / 1000.0;
#else
                            sid->down.dragged_began_timestamp =
                               ecore_loop_time_get();
#endif
#endif
                         }

                       if (!sid->down.dragged)
                         {
                            sid->down.want_dragged = EINA_TRUE;
                         }
                       //***************** TIZEN Only
                       if ((((_elm_scroll_can_scroll(sid, LEFT) || _elm_scroll_can_scroll(sid, RIGHT)) && sid->down.dir_x) ||
                            ((_elm_scroll_can_scroll(sid, UP) || _elm_scroll_can_scroll(sid, DOWN)) && sid->down.dir_y)) &&
                           !sid->down.dragged_began)
                         {
                            ev->event_flags |= EVAS_EVENT_FLAG_ON_HOLD;
                            sid->down.dragged_began = EINA_TRUE;
                         }
                       else if (sid->down.dragged_began)
                         {
                            if (sid->cb_func.item_scroll_aligned_get)
                              {
                                 if (sid->cb_func.item_scroll_aligned_get(sid->obj, NULL))
                                   sid->is_unset_cb_called = EINA_FALSE;
                              }
                            ev->event_flags |= EVAS_EVENT_FLAG_ON_HOLD;
                         }
                       //****************************
                       if (sid->down.dir_x)
                         x = sid->down.sx - (ev->cur.canvas.x - sid->down.x);
                       else
                         x = sid->down.sx;
                       if (sid->down.dir_y)
                         y = sid->down.sy - (ev->cur.canvas.y - sid->down.y);
                       else
                         y = sid->down.sy;
                       if (sid->down.want_reset)
                         {
                            sid->down.x = ev->cur.canvas.x;
                            sid->down.y = ev->cur.canvas.y;
                            sid->down.want_reset = EINA_FALSE;
                         }
                       if ((sid->down.dir_x) || (sid->down.dir_y))
                         {
                            if (!sid->down.locked)
                              {
                                 sid->down.locked_x = x;
                                 sid->down.locked_y = y;
                                 sid->down.locked = EINA_TRUE;
                              }
                            if (!((sid->down.dir_x) && (sid->down.dir_y)))
                              {
                                 if (sid->down.dir_x) y = sid->down.locked_y;
                                 else x = sid->down.locked_x;
                              }
                         }
                       {
                          Evas_Coord minx, miny, mx, my;

                          psd->api->pos_min_get(sid->pan_obj, &minx, &miny);
                          psd->api->pos_max_get(sid->pan_obj, &mx, &my);
                          if (!sid->loop_v && y < miny)
                            y += (miny - y) *
                              _elm_config->thumbscroll_border_friction;
                          else if (!sid->loop_v && my <= 0)
                            y += (sid->down.sy - y) *
                              _elm_config->thumbscroll_border_friction;
                          else if (!sid->loop_v && (my + miny) < y)
                            y += (my + miny - y) *
                              _elm_config->thumbscroll_border_friction;
                          if (!sid->loop_h && x < minx)
                            x += (minx - x) *
                              _elm_config->thumbscroll_border_friction;
                          else if (!sid->loop_h && mx <= 0)
                            x += (sid->down.sx - x) *
                              _elm_config->thumbscroll_border_friction;
                          else if (!sid->loop_h && (mx + minx) < x)
                            x += (mx + minx - x) *
                              _elm_config->thumbscroll_border_friction;
                       }

                       sid->down.hold_x = x;
                       sid->down.hold_y = y;
                       if (!sid->down.hold_animator)
                         {
                             ERR("[DDO] animator");
                            sid->down.hold_animator =
                               ecore_animator_add(_elm_scroll_hold_animator, sid);
                            sid->down.anim_x_prev = 0;
                            sid->down.anim_y_prev = 0;
                            sid->down.anim_vx_prev = 0;
                            sid->down.anim_vy_prev = 0;
                            sid->down.anim_t_prev = 0;
                            sid->down.anim_x_coord_prev = 0;
                            sid->down.anim_y_coord_prev = 0;
                            sid->down.anim_count = 0;
                            sid->down.anim_skip = 0;
                            sid->down.anim_t_dont_adjust = 0;
                            sid->down.anim_t_delay = 0;
                            sid->down.anim_t_adjusted = 0;
                            sid->down.anim_pos_t_prev = 0;
                            memset(&sid->down.predict, 0 , sizeof(sid->down.predict));
                         }
                    }
                  else
                    {
                       if (sid->down.dragged_began)
                         {
                            //***************** TIZEN Only
                            if ((_elm_scroll_can_scroll(sid, sid->down.hdir) && sid->down.dir_x) ||
                                (_elm_scroll_can_scroll(sid, sid->down.vdir) && sid->down.dir_y))
                              {
                                 ev->event_flags |= EVAS_EVENT_FLAG_ON_HOLD;
                              }
                            //****************************
                            if (!sid->down.hold)
                              {
                                 sid->down.hold = EINA_TRUE;
                                 evas_event_feed_hold
                                   (e, 1, ev->timestamp, ev->data);
                              }
                         }
                    }
               }
             else if (!sid->freeze)
               {
                  double vx = 0.0, vy = 0.0;

                  x = ev->cur.canvas.x - sid->x;
                  y = ev->cur.canvas.y - sid->y;
                  if (x < _elm_config->thumbscroll_hold_threshold)
                    {
                       if (_elm_config->thumbscroll_hold_threshold > 0.0)
                         vx = -(double)(_elm_config->thumbscroll_hold_threshold - x)
                           / _elm_config->thumbscroll_hold_threshold;
                       else
                         vx = -1.0;
                    }
                  else if (x > (sid->w - _elm_config->thumbscroll_hold_threshold))
                    {
                       if (_elm_config->thumbscroll_hold_threshold > 0.0)
                         vx = (double)(_elm_config->thumbscroll_hold_threshold -
                                       (sid->w - x)) /
                           _elm_config->thumbscroll_hold_threshold;
                       else
                         vx = 1.0;
                    }
                  if (y < _elm_config->thumbscroll_hold_threshold)
                    {
                       if (_elm_config->thumbscroll_hold_threshold > 0.0)
                         vy = -(double)(_elm_config->thumbscroll_hold_threshold - y)
                           / _elm_config->thumbscroll_hold_threshold;
                       else
                         vy = -1.0;
                    }
                  else if (y > (sid->h - _elm_config->thumbscroll_hold_threshold))
                    {
                       if (_elm_config->thumbscroll_hold_threshold > 0.0)
                         vy = (double)(_elm_config->thumbscroll_hold_threshold -
                                       (sid->h - y)) /
                           _elm_config->thumbscroll_hold_threshold;
                       else
                         vy = 1.0;
                    }
                  if ((vx != 0.0) || (vy != 0.0))
                    {
                       sid->down.onhold_vx = vx;
                       sid->down.onhold_vy = vy;
                       if (!sid->down.onhold_animator)
                         {
                            sid->down.onhold_vxe = 0.0;
                            sid->down.onhold_vye = 0.0;
                            sid->down.onhold_tlast = 0.0;
                            sid->down.onhold_animator = ecore_animator_add
                               (_elm_scroll_on_hold_animator, sid);
                         }
                    }
                  else
                    {
                       if (sid->down.onhold_animator)
                         {
                            ecore_animator_del(sid->down.onhold_animator);
                            sid->down.onhold_animator = NULL;
                            if (sid->content_info.resized)
                              _elm_scroll_wanted_region_set(sid->obj);
                         }
                    }
               }
          }
     }
}

static void
_elm_scroll_reconfigure(Elm_Scrollable_Smart_Interface_Data *sid)
{
   _elm_scroll_scroll_bar_size_adjust(sid);
}

static void
_on_edje_move(void *data,
              Evas *e __UNUSED__,
              Evas_Object *edje_obj,
              void *event_info __UNUSED__)
{
   Elm_Scrollable_Smart_Interface_Data *sid = data;
   int x, y;

   evas_object_geometry_get(edje_obj, &x, &y, NULL, NULL);

   sid->x = x;
   sid->y = y;

   _elm_scroll_reconfigure(sid);
}

static void
_on_edje_resize(void *data,
                Evas *e __UNUSED__,
                Evas_Object *edje_obj,
                void *event_info __UNUSED__)
{
   Elm_Scrollable_Smart_Interface_Data *sid = data;
   int w, h;

   evas_object_geometry_get(edje_obj, NULL, NULL, &w, &h);

   sid->w = w;
   sid->h = h;

   _elm_scroll_reconfigure(sid);
   _elm_scroll_wanted_region_set(sid->obj);
}

static void
_scroll_edje_object_attach(Evas_Object *obj)
{
   ELM_SCROLL_IFACE_DATA_GET_OR_RETURN(obj, sid);

   evas_object_event_callback_add
     (sid->edje_obj, EVAS_CALLBACK_RESIZE, _on_edje_resize, sid);
   evas_object_event_callback_add
     (sid->edje_obj, EVAS_CALLBACK_MOVE, _on_edje_move, sid);

   edje_object_signal_callback_add
     (sid->edje_obj, "reload", "elm", _elm_scroll_reload_cb, sid);
   edje_object_signal_callback_add
     (sid->edje_obj, "drag", "elm.dragable.vbar", _elm_scroll_vbar_drag_cb,
     sid);
   edje_object_signal_callback_add
     (sid->edje_obj, "drag,set", "elm.dragable.vbar",
     _elm_scroll_edje_drag_v_cb, sid);
   edje_object_signal_callback_add
     (sid->edje_obj, "drag,start", "elm.dragable.vbar",
     _elm_scroll_edje_drag_v_start_cb, sid);
   edje_object_signal_callback_add
     (sid->edje_obj, "drag,stop", "elm.dragable.vbar",
     _elm_scroll_edje_drag_v_stop_cb, sid);
   edje_object_signal_callback_add
     (sid->edje_obj, "drag,step", "elm.dragable.vbar",
     _elm_scroll_edje_drag_v_cb, sid);
   edje_object_signal_callback_add
     (sid->edje_obj, "drag,page", "elm.dragable.vbar",
     _elm_scroll_edje_drag_v_cb, sid);
   edje_object_signal_callback_add
     (sid->edje_obj, "elm,vbar,press", "elm",
     _elm_scroll_vbar_press_cb, sid);
   edje_object_signal_callback_add
     (sid->edje_obj, "elm,vbar,unpress", "elm",
     _elm_scroll_vbar_unpress_cb, sid);
   edje_object_signal_callback_add
     (sid->edje_obj, "drag", "elm.dragable.hbar", _elm_scroll_hbar_drag_cb,
     sid);
   edje_object_signal_callback_add
     (sid->edje_obj, "drag,set", "elm.dragable.hbar",
     _elm_scroll_edje_drag_h_cb, sid);
   edje_object_signal_callback_add
     (sid->edje_obj, "drag,start", "elm.dragable.hbar",
     _elm_scroll_edje_drag_h_start_cb, sid);
   edje_object_signal_callback_add
     (sid->edje_obj, "drag,stop", "elm.dragable.hbar",
     _elm_scroll_edje_drag_h_stop_cb, sid);
   edje_object_signal_callback_add
     (sid->edje_obj, "drag,step", "elm.dragable.hbar",
     _elm_scroll_edje_drag_h_cb, sid);
   edje_object_signal_callback_add
     (sid->edje_obj, "drag,page", "elm.dragable.hbar",
     _elm_scroll_edje_drag_h_cb, sid);
   edje_object_signal_callback_add
     (sid->edje_obj, "elm,hbar,press", "elm",
     _elm_scroll_hbar_press_cb, sid);
   edje_object_signal_callback_add
     (sid->edje_obj, "elm,hbar,unpress", "elm",
     _elm_scroll_hbar_unpress_cb, sid);
}

static void
_scroll_event_object_attach(Evas_Object *obj)
{
   ELM_SCROLL_IFACE_DATA_GET_OR_RETURN(obj, sid);

   evas_object_event_callback_add
     (sid->event_rect, EVAS_CALLBACK_MOUSE_WHEEL, _elm_scroll_wheel_event_cb,
     sid);
   evas_object_event_callback_add
     (sid->event_rect, EVAS_CALLBACK_MOUSE_DOWN,
     _elm_scroll_mouse_down_event_cb, sid);
   evas_object_event_callback_add
     (sid->event_rect, EVAS_CALLBACK_MOUSE_UP,
     _elm_scroll_mouse_up_event_cb, sid);
   evas_object_event_callback_add
     (sid->event_rect, EVAS_CALLBACK_MOUSE_MOVE,
     _elm_scroll_mouse_move_event_cb, sid);
}

static void
_scroll_edje_object_detach(Evas_Object *obj)
{
   ELM_SCROLL_IFACE_DATA_GET_OR_RETURN(obj, sid);

   evas_object_event_callback_del_full
     (sid->edje_obj, EVAS_CALLBACK_RESIZE, _on_edje_resize, sid);
   evas_object_event_callback_del_full
     (sid->edje_obj, EVAS_CALLBACK_MOVE, _on_edje_move, sid);

   edje_object_signal_callback_del_full
     (sid->edje_obj, "drag", "elm.dragable.vbar", _elm_scroll_vbar_drag_cb,
     sid);
   edje_object_signal_callback_del_full
     (sid->edje_obj, "drag,set", "elm.dragable.vbar",
     _elm_scroll_edje_drag_v_cb, sid);
   edje_object_signal_callback_del_full
     (sid->edje_obj, "drag,start", "elm.dragable.vbar",
     _elm_scroll_edje_drag_v_start_cb, sid);
   edje_object_signal_callback_del_full
     (sid->edje_obj, "drag,stop", "elm.dragable.vbar",
     _elm_scroll_edje_drag_v_stop_cb, sid);
   edje_object_signal_callback_del_full
     (sid->edje_obj, "drag,step", "elm.dragable.vbar",
     _elm_scroll_edje_drag_v_cb, sid);
   edje_object_signal_callback_del_full
     (sid->edje_obj, "drag,page", "elm.dragable.vbar",
     _elm_scroll_edje_drag_v_cb, sid);
   edje_object_signal_callback_del_full
     (sid->edje_obj, "elm,vbar,press", "elm",
     _elm_scroll_vbar_press_cb, sid);
   edje_object_signal_callback_del_full
     (sid->edje_obj, "elm,vbar,unpress", "elm",
     _elm_scroll_vbar_unpress_cb, sid);
   edje_object_signal_callback_del_full
     (sid->edje_obj, "drag", "elm.dragable.hbar", _elm_scroll_hbar_drag_cb,
     sid);
   edje_object_signal_callback_del_full
     (sid->edje_obj, "drag,set", "elm.dragable.hbar",
     _elm_scroll_edje_drag_h_cb, sid);
   edje_object_signal_callback_del_full
     (sid->edje_obj, "drag,start", "elm.dragable.hbar",
     _elm_scroll_edje_drag_h_start_cb, sid);
   edje_object_signal_callback_del_full
     (sid->edje_obj, "drag,stop", "elm.dragable.hbar",
     _elm_scroll_edje_drag_h_stop_cb, sid);
   edje_object_signal_callback_del_full
     (sid->edje_obj, "drag,step", "elm.dragable.hbar",
     _elm_scroll_edje_drag_h_cb, sid);
   edje_object_signal_callback_del_full
     (sid->edje_obj, "drag,page", "elm.dragable.hbar",
     _elm_scroll_edje_drag_h_cb, sid);
   edje_object_signal_callback_del_full
     (sid->edje_obj, "elm,hbar,press", "elm",
     _elm_scroll_hbar_press_cb, sid);
   edje_object_signal_callback_del_full
     (sid->edje_obj, "elm,hbar,unpress", "elm",
     _elm_scroll_hbar_unpress_cb, sid);
}

static void
_scroll_event_object_detach(Evas_Object *obj)
{
   ELM_SCROLL_IFACE_DATA_GET_OR_RETURN(obj, sid);

   evas_object_event_callback_del_full
     (sid->event_rect, EVAS_CALLBACK_MOUSE_WHEEL, _elm_scroll_wheel_event_cb,
     sid);
   evas_object_event_callback_del_full
     (sid->event_rect, EVAS_CALLBACK_MOUSE_DOWN,
     _elm_scroll_mouse_down_event_cb, sid);
   evas_object_event_callback_del_full
     (sid->event_rect, EVAS_CALLBACK_MOUSE_UP,
     _elm_scroll_mouse_up_event_cb, sid);
   evas_object_event_callback_del_full
     (sid->event_rect, EVAS_CALLBACK_MOUSE_MOVE,
     _elm_scroll_mouse_move_event_cb, sid);
}

static void
_elm_scroll_objects_set(Evas_Object *obj,
                        Evas_Object *edje_object,
                        Evas_Object *hit_rectangle)
{
   Evas_Coord mw, mh;

   ELM_SCROLL_IFACE_DATA_GET_OR_RETURN(obj, sid);

   if (!edje_object || !hit_rectangle) return;

   if (sid->edje_obj)
       _scroll_edje_object_detach(obj);

   sid->edje_obj = edje_object;

   if (sid->event_rect)
       _scroll_event_object_detach(obj);

   sid->event_rect = hit_rectangle;
   evas_object_repeat_events_set(hit_rectangle, EINA_TRUE);

   _scroll_edje_object_attach(obj);
   _scroll_event_object_attach(obj);

   mw = mh = -1;
   elm_coords_finger_size_adjust(1, &mw, 1, &mh);
   if (edje_object_part_exists(sid->edje_obj, "elm.scrollbar.base"))
     {
        Evas_Object *base;

        base = edje_object_part_swallow_get
            (sid->edje_obj, "elm.scrollbar.base");
        if (!base)
          {
             base = evas_object_rectangle_add
                 (evas_object_evas_get(sid->edje_obj));
             evas_object_color_set(base, 0, 0, 0, 0);
             edje_object_part_swallow
               (sid->edje_obj, "elm.scrollbar.base", base);
          }
        if (!_elm_config->thumbscroll_enable)
          evas_object_size_hint_min_set(base, mw, mh);
     }

   _elm_scroll_scroll_bar_visibility_adjust(sid);
}

static void
_elm_scroll_scroll_bar_reset(Elm_Scrollable_Smart_Interface_Data *sid)
{
   Evas_Coord px = 0, py = 0, minx = 0, miny = 0;

   if (!sid->edje_obj) return;

   edje_object_part_drag_value_set
     (sid->edje_obj, "elm.dragable.vbar", 0.0, 0.0);
   edje_object_part_drag_value_set
     (sid->edje_obj, "elm.dragable.hbar", 0.0, 0.0);
   if ((!sid->content) && (!sid->extern_pan))
     {
        edje_object_part_drag_size_set
          (sid->edje_obj, "elm.dragable.vbar", 1.0, 1.0);
        edje_object_part_drag_size_set
          (sid->edje_obj, "elm.dragable.hbar", 1.0, 1.0);
     }
   if (sid->pan_obj)
     {
        ELM_PAN_DATA_GET_OR_RETURN(sid->pan_obj, psd);

        psd->api->pos_min_get(sid->pan_obj, &minx, &miny);
        psd->api->pos_get(sid->pan_obj, &px, &py);
        psd->api->pos_set(sid->pan_obj, minx, miny);
     }
   if ((px != minx) || (py != miny))
     edje_object_signal_emit(sid->edje_obj, "elm,action,scroll", "elm");
   _elm_direction_arrows_eval(sid);
}

/* even external pan objects get this */
static void
_elm_scroll_pan_changed_cb(void *data,
                           Evas_Object *obj __UNUSED__,
                           void *event_info __UNUSED__)
{
   Evas_Coord w, h, vw, vh;
   Elm_Scrollable_Smart_Interface_Data *sid = data;

   if (!sid->pan_obj) return;

   ELM_PAN_DATA_GET_OR_RETURN(sid->pan_obj, psd);

   psd->api->content_size_get(sid->pan_obj, &w, &h);
   _elm_scroll_content_viewport_size_get(sid->obj, &vw, &vh);
   if ((w != sid->content_info.w) || (h != sid->content_info.h))
     {
        if (_paging_is_enabled(sid))
          {
             if (w != sid->content_info.w)
               {
                  if (sid->pagerel_h > 0)
                    sid->pagecount_h = w / (vw * sid->pagerel_h);
               }
             if (h != sid->content_info.h)
               {
                  if (sid->pagerel_v > 0)
                    sid->pagecount_v = h / (vh * sid->pagerel_v);
               }
          }
        sid->content_info.w = w;
        sid->content_info.h = h;
        _elm_scroll_scroll_bar_size_adjust(sid);

        evas_object_size_hint_min_set
          (sid->edje_obj, sid->content_info.w, sid->content_info.h);
        sid->content_info.resized = EINA_TRUE;
        _elm_scroll_reconfigure(sid);
        _elm_scroll_wanted_region_set(sid->obj);
     }
}

static void
_elm_scroll_pan_resized_cb(void *data,
                          Evas *e __UNUSED__,
                          Evas_Object *obj __UNUSED__,
                          void *event_info __UNUSED__)
{
   Evas_Coord w, h;
   Elm_Scrollable_Smart_Interface_Data *sid = data;

   if (sid->cb_func.content_viewport_resize)
     {
        _elm_scroll_content_viewport_size_get(sid->obj, &w, &h);
        sid->cb_func.content_viewport_resize(sid->obj, w, h);
     }
}

static void
_elm_scroll_content_del_cb(void *data,
                           Evas *e __UNUSED__,
                           Evas_Object *obj __UNUSED__,
                           void *event_info __UNUSED__)
{
   Elm_Scrollable_Smart_Interface_Data *sid = data;

   sid->content = NULL;
   _elm_scroll_scroll_bar_size_adjust(sid);
   _elm_scroll_scroll_bar_reset(sid);
}

static void
_elm_scroll_content_set(Evas_Object *obj,
                        Evas_Object *content)
{
   Evas_Coord w, h;
   Evas_Object *o;

   ELM_SCROLL_IFACE_DATA_GET_OR_RETURN(obj, sid);

   if (!sid->edje_obj) return;

   if (sid->content)
     {
        /* if we had content, for sure we had a pan object */
        _elm_pan_content_set(sid->pan_obj, NULL);
        evas_object_event_callback_del_full
          (sid->content, EVAS_CALLBACK_DEL, _elm_scroll_content_del_cb, sid);
     }

   sid->content = content;
   sid->wx = sid->wy = 0;
   /* (-1) means want viewports size */
   sid->ww = sid->wh = -1;
   if (!content) return;

   if (!sid->pan_obj)
     {
        o = _elm_pan_add(evas_object_evas_get(obj));
        sid->pan_obj = o;
        evas_object_smart_callback_add
          (o, SIG_CHANGED, _elm_scroll_pan_changed_cb, sid);
        evas_object_event_callback_add(o, EVAS_CALLBACK_RESIZE,
                                       _elm_scroll_pan_resized_cb, sid);
        edje_object_part_swallow(sid->edje_obj, "elm.swallow.content", o);
     }

   ELM_PAN_DATA_GET_OR_RETURN(sid->pan_obj, psd);

   evas_object_event_callback_add
     (content, EVAS_CALLBACK_DEL, _elm_scroll_content_del_cb, sid);

   _elm_pan_content_set(sid->pan_obj, content);
   psd->api->content_size_get(sid->pan_obj, &w, &h);
   sid->content_info.w = w;
   sid->content_info.h = h;

   if (sid->cb_func.content_set)
      sid->cb_func.content_set(obj, content, NULL);

   _elm_scroll_scroll_bar_size_adjust(sid);
   _elm_scroll_scroll_bar_reset(sid);
}

static void
_elm_scroll_extern_pan_set(Evas_Object *obj,
                           Evas_Object *pan)
{
   ELM_SCROLL_IFACE_DATA_GET_OR_RETURN(obj, sid);

   if (!sid->edje_obj) return;

   _elm_scroll_content_set(obj, NULL);

   if (sid->pan_obj)
     {
        evas_object_smart_callback_del
          (sid->pan_obj, SIG_CHANGED, _elm_scroll_pan_changed_cb);
        evas_object_event_callback_del(sid->pan_obj, EVAS_CALLBACK_RESIZE,
                                       _elm_scroll_pan_resized_cb);
     }

   if (sid->extern_pan)
     {
        if (sid->pan_obj)
          {
             /* not owned by scroller, just leave (was external already) */
             edje_object_part_unswallow(sid->edje_obj, sid->pan_obj);
             sid->pan_obj = NULL;
          }
     }
   else
     {
        if (sid->pan_obj)
          {
             evas_object_del(sid->pan_obj);
             sid->pan_obj = NULL;
          }
     }
   if (!pan)
     {
        sid->extern_pan = EINA_FALSE;
        return;
     }

   sid->pan_obj = pan;

   sid->extern_pan = EINA_TRUE;
   evas_object_smart_callback_add
     (sid->pan_obj, SIG_CHANGED, _elm_scroll_pan_changed_cb, sid);
   evas_object_event_callback_add(sid->pan_obj, EVAS_CALLBACK_RESIZE,
                                  _elm_scroll_pan_resized_cb, sid);
   edje_object_part_swallow
     (sid->edje_obj, "elm.swallow.content", sid->pan_obj);
}

static void
_elm_scroll_drag_start_cb_set(Evas_Object *obj,
                              void (*drag_start_cb)(Evas_Object *obj,
                                                    void *data))
{
   ELM_SCROLL_IFACE_DATA_GET_OR_RETURN(obj, sid);

   sid->cb_func.drag_start = drag_start_cb;
}

static void
_elm_scroll_drag_stop_cb_set(Evas_Object *obj,
                             void (*drag_stop_cb)(Evas_Object *obj,
                                                  void *data))
{
   ELM_SCROLL_IFACE_DATA_GET_OR_RETURN(obj, sid);

   sid->cb_func.drag_stop = drag_stop_cb;
}

static void
_elm_scroll_animate_start_cb_set(Evas_Object *obj,
                                 void (*animate_start_cb)(Evas_Object *obj,
                                                          void *data))
{
   ELM_SCROLL_IFACE_DATA_GET_OR_RETURN(obj, sid);

   sid->cb_func.animate_start = animate_start_cb;
}

static void
_elm_scroll_animate_stop_cb_set(Evas_Object *obj,
                                void (*animate_stop_cb)(Evas_Object *obj,
                                                        void *data))
{
   ELM_SCROLL_IFACE_DATA_GET_OR_RETURN(obj, sid);

   sid->cb_func.animate_stop = animate_stop_cb;
}

static void
_elm_scroll_scroll_cb_set(Evas_Object *obj,
                          void (*scroll_cb)(Evas_Object *obj,
                                            void *data))
{
   ELM_SCROLL_IFACE_DATA_GET_OR_RETURN(obj, sid);

   sid->cb_func.scroll = scroll_cb;
}

static void
_elm_scroll_scroll_left_cb_set(Evas_Object *obj,
                          void (*scroll_left_cb)(Evas_Object *obj,
                                            void *data))
{
   ELM_SCROLL_IFACE_DATA_GET_OR_RETURN(obj, sid);

   sid->cb_func.scroll_left = scroll_left_cb;
}

static void
_elm_scroll_scroll_right_cb_set(Evas_Object *obj,
                          void (*scroll_right_cb)(Evas_Object *obj,
                                            void *data))
{
   ELM_SCROLL_IFACE_DATA_GET_OR_RETURN(obj, sid);

   sid->cb_func.scroll_right = scroll_right_cb;
}

static void
_elm_scroll_scroll_up_cb_set(Evas_Object *obj,
                          void (*scroll_up_cb)(Evas_Object *obj,
                                            void *data))
{
   ELM_SCROLL_IFACE_DATA_GET_OR_RETURN(obj, sid);

   sid->cb_func.scroll_up = scroll_up_cb;
}

static void
_elm_scroll_scroll_down_cb_set(Evas_Object *obj,
                          void (*scroll_down_cb)(Evas_Object *obj,
                                            void *data))
{
   ELM_SCROLL_IFACE_DATA_GET_OR_RETURN(obj, sid);

   sid->cb_func.scroll_down = scroll_down_cb;
}

static void
_elm_scroll_edge_left_cb_set(Evas_Object *obj,
                             void (*edge_left_cb)(Evas_Object *obj,
                                                  void *data))
{
   ELM_SCROLL_IFACE_DATA_GET_OR_RETURN(obj, sid);

   sid->cb_func.edge_left = edge_left_cb;
}

static void
_elm_scroll_edge_right_cb_set(Evas_Object *obj,
                              void (*edge_right_cb)(Evas_Object *obj,
                                                    void *data))
{
   ELM_SCROLL_IFACE_DATA_GET_OR_RETURN(obj, sid);

   sid->cb_func.edge_right = edge_right_cb;
}

static void
_elm_scroll_edge_top_cb_set(Evas_Object *obj,
                            void (*edge_top_cb)(Evas_Object *obj,
                                                void *data))
{
   ELM_SCROLL_IFACE_DATA_GET_OR_RETURN(obj, sid);

   sid->cb_func.edge_top = edge_top_cb;
}

static void
_elm_scroll_edge_bottom_cb_set(Evas_Object *obj,
                               void (*edge_bottom_cb)(Evas_Object *obj,
                                                      void *data))
{
   ELM_SCROLL_IFACE_DATA_GET_OR_RETURN(obj, sid);

   sid->cb_func.edge_bottom = edge_bottom_cb;
}

static void
_elm_scroll_vbar_drag_cb_set(Evas_Object *obj,
                               void (*vbar_drag_cb)(Evas_Object *obj,
                                                      void *data))
{
   ELM_SCROLL_IFACE_DATA_GET_OR_RETURN(obj, sid);

   sid->cb_func.vbar_drag = vbar_drag_cb;
}

static void
_elm_scroll_vbar_press_cb_set(Evas_Object *obj,
                               void (*vbar_press_cb)(Evas_Object *obj,
                                                      void *data))
{
   ELM_SCROLL_IFACE_DATA_GET_OR_RETURN(obj, sid);

   sid->cb_func.vbar_press = vbar_press_cb;
}

static void
_elm_scroll_vbar_unpress_cb_set(Evas_Object *obj,
                               void (*vbar_unpress_cb)(Evas_Object *obj,
                                                      void *data))
{
   ELM_SCROLL_IFACE_DATA_GET_OR_RETURN(obj, sid);

   sid->cb_func.vbar_unpress = vbar_unpress_cb;
}

static void
_elm_scroll_hbar_drag_cb_set(Evas_Object *obj,
                               void (*hbar_drag_cb)(Evas_Object *obj,
                                                      void *data))
{
   ELM_SCROLL_IFACE_DATA_GET_OR_RETURN(obj, sid);

   sid->cb_func.hbar_drag = hbar_drag_cb;
}

static void
_elm_scroll_hbar_press_cb_set(Evas_Object *obj,
                               void (*hbar_press_cb)(Evas_Object *obj,
                                                      void *data))
{
   ELM_SCROLL_IFACE_DATA_GET_OR_RETURN(obj, sid);

   sid->cb_func.hbar_press = hbar_press_cb;
}

static void
_elm_scroll_hbar_unpress_cb_set(Evas_Object *obj,
                               void (*hbar_unpress_cb)(Evas_Object *obj,
                                                      void *data))
{
   ELM_SCROLL_IFACE_DATA_GET_OR_RETURN(obj, sid);

   sid->cb_func.hbar_unpress = hbar_unpress_cb;
}

static void
_elm_scroll_content_min_limit_cb_set(Evas_Object *obj,
                                     void (*c_min_limit_cb)(Evas_Object *obj,
                                                            Eina_Bool w,
                                                            Eina_Bool h))
{
   ELM_SCROLL_IFACE_DATA_GET_OR_RETURN(obj, sid);

   sid->cb_func.content_min_limit = c_min_limit_cb;
}

static void
_elm_scroll_content_viewport_resize_cb_set(Evas_Object *obj,
                                 void (*c_viewport_resize_cb)(Evas_Object *obj,
                                                              Evas_Coord w,
                                                              Evas_Coord h))
{
   ELM_SCROLL_IFACE_DATA_GET_OR_RETURN(obj, sid);

   sid->cb_func.content_viewport_resize = c_viewport_resize_cb;
}

static void
_elm_scroll_item_scroll_align_end_set_cb_set(Evas_Object *obj,
                        void (*item_scroll_align_end_set_cb)(Evas_Object *obj,
                                                             void *data))
{
   ELM_SCROLL_IFACE_DATA_GET_OR_RETURN(obj, sid);

   sid->cb_func.item_scroll_align_end_set = item_scroll_align_end_set_cb;
}

static void
_elm_scroll_item_scroll_align_end_unset_cb_set(Evas_Object *obj,
                        void (*item_scroll_align_end_unset_cb)(Evas_Object *obj,
                                                               void *data))
{
   ELM_SCROLL_IFACE_DATA_GET_OR_RETURN(obj, sid);

   sid->cb_func.item_scroll_align_end_unset = item_scroll_align_end_unset_cb;
}

static void
_elm_scroll_content_set_cb_set(Evas_Object *obj, void (*content_set_cb)(Evas_Object *obj, Evas_Object *content, void *data))
{
   ELM_SCROLL_IFACE_DATA_GET_OR_RETURN(obj, sid);

   sid->cb_func.content_set = content_set_cb;
}

static void
_elm_scroll_item_scroll_aligned_get_cb_set(Evas_Object *obj,
                        Eina_Bool (*item_scroll_aligned_get_cb)(Evas_Object *obj,
                                                               void *data))
{
   ELM_SCROLL_IFACE_DATA_GET_OR_RETURN(obj, sid);

   sid->cb_func.item_scroll_aligned_get = item_scroll_aligned_get_cb;
}

static void
_elm_scroll_scroll_position_get_cb_set(Evas_Object *obj, void (*scroll_position_get)(Evas_Object *obj, Evas_Coord *x, Evas_Coord *y))
{
   ELM_SCROLL_IFACE_DATA_GET_OR_RETURN(obj, sid);

   sid->cb_func.scroll_position_get = scroll_position_get;
}

static Eina_Bool
_elm_scroll_momentum_animator_disabled_get(const Evas_Object *obj)
{
   ELM_SCROLL_IFACE_DATA_GET_OR_RETURN_VAL(obj, sid, EINA_FALSE);

   return sid->momentum_animator_disabled;
}

static void
_elm_scroll_momentum_animator_disabled_set(Evas_Object *obj,
                                           Eina_Bool disabled)
{
   ELM_SCROLL_IFACE_DATA_GET_OR_RETURN(obj, sid);

   sid->momentum_animator_disabled = disabled;
   if (sid->momentum_animator_disabled)
     {
        if (sid->down.momentum_animator)
          {
             ecore_animator_del(sid->down.momentum_animator);
             sid->down.momentum_animator = NULL;
             if (sid->content_info.resized)
               _elm_scroll_wanted_region_set(sid->obj);
          }
     }
}

static Eina_Bool
_elm_scroll_bounce_animator_disabled_get(const Evas_Object *obj)
{
   ELM_SCROLL_IFACE_DATA_GET_OR_RETURN_VAL(obj, sid, EINA_FALSE);

   return sid->bounce_animator_disabled;
}

static void
_elm_scroll_bounce_animator_disabled_set(Evas_Object *obj,
                                         Eina_Bool disabled)
{
   ELM_SCROLL_IFACE_DATA_GET_OR_RETURN(obj, sid);

   sid->bounce_animator_disabled = disabled;
   if (sid->bounce_animator_disabled)
     {
        if (sid->scrollto.x.animator)
          {
             ecore_animator_del(sid->scrollto.x.animator);
             sid->scrollto.x.animator = NULL;
          }

        if (sid->scrollto.y.animator)
          {
             ecore_animator_del(sid->scrollto.y.animator);
             sid->scrollto.y.animator = NULL;
          }
     }
}

static Eina_Bool
_elm_scroll_wheel_disabled_get(const Evas_Object *obj)
{
   ELM_SCROLL_IFACE_DATA_GET_OR_RETURN_VAL(obj, sid, EINA_FALSE);

   return sid->wheel_disabled;
}

static void
_elm_scroll_wheel_disabled_set(Evas_Object *obj,
                               Eina_Bool disabled)
{
   ELM_SCROLL_IFACE_DATA_GET_OR_RETURN(obj, sid);

   if (!sid->event_rect) return;

   if ((!sid->wheel_disabled) && (disabled))
     evas_object_event_callback_del_full
       (sid->event_rect, EVAS_CALLBACK_MOUSE_WHEEL,
       _elm_scroll_wheel_event_cb, sid);
   else if ((sid->wheel_disabled) && (!disabled))
     evas_object_event_callback_add
       (sid->event_rect, EVAS_CALLBACK_MOUSE_WHEEL,
       _elm_scroll_wheel_event_cb, sid);
   sid->wheel_disabled = disabled;
}

static void
_elm_scroll_movement_block_set(Evas_Object *obj,
                               Elm_Scroller_Movement_Block block)
{
   ELM_SCROLL_IFACE_DATA_GET_OR_RETURN(obj, sid);

   sid->block = block;
}

static Elm_Scroller_Movement_Block
_elm_scroll_movement_block_get(const Evas_Object *obj)
{
   ELM_SCROLL_IFACE_DATA_GET_OR_RETURN_VAL(obj, sid, ELM_SCROLLER_MOVEMENT_NO_BLOCK);

   return sid->block;
}

static void
_elm_scroll_step_size_set(Evas_Object *obj,
                          Evas_Coord x,
                          Evas_Coord y)
{
   ELM_SCROLL_IFACE_DATA_GET_OR_RETURN(obj, sid);

   if (x < 1) x = 1;
   if (y < 1) y = 1;
   sid->step.x = x;
   sid->step.y = y;

   _elm_scroll_scroll_bar_size_adjust(sid);
}

static void
_elm_scroll_step_size_get(const Evas_Object *obj,
                          Evas_Coord *x,
                          Evas_Coord *y)
{
   ELM_SCROLL_IFACE_DATA_GET_OR_RETURN(obj, sid);

   if (x) *x = sid->step.x;
   if (y) *y = sid->step.y;
}

static void
_elm_scroll_page_size_set(Evas_Object *obj,
                          Evas_Coord x,
                          Evas_Coord y)
{
   ELM_SCROLL_IFACE_DATA_GET_OR_RETURN(obj, sid);

   sid->page.x = x;
   sid->page.y = y;

   _elm_scroll_scroll_bar_size_adjust(sid);
}

static void
_elm_scroll_page_size_get(const Evas_Object *obj,
                          Evas_Coord *x,
                          Evas_Coord *y)
{
   ELM_SCROLL_IFACE_DATA_GET_OR_RETURN(obj, sid);

   if (x) *x = sid->page.x;
   if (y) *y = sid->page.y;
}

static void
_elm_scroll_policy_set(Evas_Object *obj,
                       Elm_Scroller_Policy hbar,
                       Elm_Scroller_Policy vbar)
{
   ELM_SCROLL_IFACE_DATA_GET_OR_RETURN(obj, sid);

   if (!sid->edje_obj) return;

   if ((sid->hbar_flags == hbar) && (sid->vbar_flags == vbar)) return;

   sid->hbar_flags = hbar;
   sid->vbar_flags = vbar;
   _elm_scroll_policy_signal_emit(sid);
   if (sid->cb_func.content_min_limit)
     sid->cb_func.content_min_limit(sid->obj, sid->min_w, sid->min_h);
   _elm_direction_arrows_eval(sid);
}

static void
_elm_scroll_policy_get(const Evas_Object *obj,
                       Elm_Scroller_Policy *hbar,
                       Elm_Scroller_Policy *vbar)
{
   ELM_SCROLL_IFACE_DATA_GET_OR_RETURN(obj, sid);

   if (hbar) *hbar = sid->hbar_flags;
   if (vbar) *vbar = sid->vbar_flags;
}

static void
_elm_scroll_single_direction_set(Evas_Object *obj,
                                 Elm_Scroller_Single_Direction single_dir)
{
   ELM_SCROLL_IFACE_DATA_GET_OR_RETURN(obj, sid);

   sid->one_direction_at_a_time = single_dir;
}

static Elm_Scroller_Single_Direction
_elm_scroll_single_direction_get(const Evas_Object *obj)
{
   ELM_SCROLL_IFACE_DATA_GET_OR_RETURN_VAL(obj, sid, ELM_SCROLLER_SINGLE_DIRECTION_SOFT);

   return sid->one_direction_at_a_time;
}

static void
_elm_scroll_repeat_events_set(Evas_Object *obj,
                                 Eina_Bool repeat_events)
{
   ELM_SCROLL_IFACE_DATA_GET_OR_RETURN(obj, sid);

   evas_object_repeat_events_set(sid->event_rect, repeat_events);
}

static Eina_Bool
_elm_scroll_repeat_events_get(const Evas_Object *obj)
{
   ELM_SCROLL_IFACE_DATA_GET_OR_RETURN_VAL(obj, sid, EINA_FALSE);

   return evas_object_repeat_events_get(sid->event_rect);
}

static void
_elm_scroll_hold_set(Evas_Object *obj,
                     Eina_Bool hold)
{
   ELM_SCROLL_IFACE_DATA_GET_OR_RETURN(obj, sid);

   ERR("[DDO] obj(%x), hold(%d)", sid->obj, hold);

   sid->hold = hold;
}

static void
_elm_scroll_freeze_set(Evas_Object *obj,
                       Eina_Bool freeze)
{
   ELM_SCROLL_IFACE_DATA_GET_OR_RETURN(obj, sid);

   ERR("[DDO] obj(%x), freeze(%d)", sid->obj, freeze);

   sid->freeze = freeze;
   if (sid->freeze)
     {
        if (sid->down.onhold_animator)
          {
             ecore_animator_del(sid->down.onhold_animator);
             sid->down.onhold_animator = NULL;
             if (sid->content_info.resized)
               _elm_scroll_wanted_region_set(sid->obj);
          }
     }
   else
     _elm_scroll_bounce_eval(sid);
}

static void
_elm_scroll_bounce_allow_set(Evas_Object *obj,
                             Eina_Bool horiz,
                             Eina_Bool vert)
{
   ELM_SCROLL_IFACE_DATA_GET_OR_RETURN(obj, sid);

   sid->bounce_horiz = !!horiz;
   sid->bounce_vert = !!vert;
}

static void
_elm_scroll_bounce_allow_get(const Evas_Object *obj,
                             Eina_Bool *horiz,
                             Eina_Bool *vert)
{
   ELM_SCROLL_IFACE_DATA_GET_OR_RETURN(obj, sid);

   if (horiz) *horiz = sid->bounce_horiz;
   if (vert) *vert = sid->bounce_vert;
}

static void
_elm_scroll_paging_set(Evas_Object *obj,
                       double pagerel_h,
                       double pagerel_v,
                       Evas_Coord pagesize_h,
                       Evas_Coord pagesize_v)
{
   ELM_SCROLL_IFACE_DATA_GET_OR_RETURN(obj, sid);

   sid->pagerel_h = pagerel_h;
   sid->pagerel_v = pagerel_v;
   sid->pagesize_h = pagesize_h;
   sid->pagesize_v = pagesize_v;
}

static void
_elm_scroll_paging_get(const Evas_Object *obj,
                       double *pagerel_h,
                       double *pagerel_v,
                       Evas_Coord *pagesize_h,
                       Evas_Coord *pagesize_v)
{
   ELM_SCROLL_IFACE_DATA_GET_OR_RETURN(obj, sid);

   if (pagerel_h) *pagerel_h = sid->pagerel_h;
   if (pagerel_v) *pagerel_v = sid->pagerel_v;
   if (pagesize_h) *pagesize_h = sid->pagesize_h;
   if (pagesize_v) *pagesize_v = sid->pagesize_v;
}

static void
_elm_scroll_page_scroll_limit_set(const Evas_Object *obj,
                                  int page_limit_h,
                                  int page_limit_v)
{
   ELM_SCROLL_IFACE_DATA_GET_OR_RETURN(obj, sid);

   sid->page_limit_h = page_limit_h;
   sid->page_limit_v = page_limit_v;
}

static void
_elm_scroll_page_scroll_limit_get(const Evas_Object *obj,
                                  int *page_limit_h,
                                  int *page_limit_v)
{
   ELM_SCROLL_IFACE_DATA_GET_OR_RETURN(obj, sid);

   if (page_limit_h) *page_limit_h = sid->page_limit_h;
   if (page_limit_v) *page_limit_v = sid->page_limit_v;
}

static void
_elm_scroll_current_page_get(const Evas_Object *obj,
                             int *pagenumber_h,
                             int *pagenumber_v)
{
   Evas_Coord x, y;

   ELM_SCROLL_IFACE_DATA_GET_OR_RETURN(obj, sid);

   _elm_scroll_content_pos_get(sid->obj, &x, &y);
   if (pagenumber_h)
     {
        if (sid->pagesize_h > 0)
          {
             double result = (double)x / (double)sid->pagesize_h;
             double rest = result - (x / sid->pagesize_h);
             if (rest >= 0.5)
               *pagenumber_h = result + 1;
             else
               *pagenumber_h = result;
          }
        else
          *pagenumber_h = 0;
     }
   if (pagenumber_v)
     {
        if (sid->pagesize_v > 0)
          {
             double result = (double)y / (double)sid->pagesize_v;
             double rest = result - (y / sid->pagesize_v);
             if (rest >= 0.5)
               *pagenumber_v = result + 1;
             else
               *pagenumber_v = result;
          }
        else
          *pagenumber_v = 0;
     }
}

static void
_elm_scroll_last_page_get(const Evas_Object *obj,
                          int *pagenumber_h,
                          int *pagenumber_v)
{
   Evas_Coord cw, ch;

   ELM_SCROLL_IFACE_DATA_GET_OR_RETURN(obj, sid);

   if (!sid->pan_obj) return;

   ELM_PAN_DATA_GET_OR_RETURN(sid->pan_obj, psd);

   psd->api->content_size_get(sid->pan_obj, &cw, &ch);
   if (pagenumber_h)
     {
        if (sid->pagesize_h > 0)
          *pagenumber_h = cw / sid->pagesize_h - 1;
        else
          *pagenumber_h = 0;
     }
   if (pagenumber_v)
     {
        if (sid->pagesize_v > 0)
          *pagenumber_v = ch / sid->pagesize_v - 1;
        else
          *pagenumber_v = 0;
     }
}

static void
_elm_scroll_page_show(Evas_Object *obj,
                      int pagenumber_h,
                      int pagenumber_v)
{
   Evas_Coord w, h, cw = 0, ch = 0;
   Evas_Coord x = 0;
   Evas_Coord y = 0;

   ELM_SCROLL_IFACE_DATA_GET_OR_RETURN(obj, sid);
   if (sid->pan_obj)
     {
        ELM_PAN_DATA_GET_OR_RETURN(sid->pan_obj, psd);
        psd->api->content_size_get(sid->pan_obj, &cw, &ch);
     }

   sid->current_page.x = _elm_scroll_page_x_get(sid, 0, EINA_FALSE);
   sid->current_page.y = _elm_scroll_page_y_get(sid, 0, EINA_FALSE);

   _elm_scroll_content_viewport_size_get(sid->obj, &w, &h);
   if (pagenumber_h >= 0) x = sid->pagesize_h * pagenumber_h;
   if (pagenumber_v >= 0) y = sid->pagesize_v * pagenumber_v;

   if (sid->origin_x && cw > 0)
     {
        sid->wx = cw - x;
        if (sid->wx < 0) sid->wx = 0;
     }
   else
     sid->wx = x;

   if (sid->origin_y && ch > 0)
     {
        sid->wy = ch - y;
        if (sid->wy < 0) sid->wy = 0;
     }
   else
     sid->wy = y;
   sid->ww = w;
   sid->wh = h;

   if (_elm_scroll_content_region_show_internal(obj, &x, &y, w, h))
     _elm_scroll_content_pos_set(obj, x, y, EINA_TRUE);

   if (sid->pagesize_h)
      sid->rotary_animation_info.current_page = pagenumber_h;
   else if (sid->pagesize_v)
      sid->rotary_animation_info.current_page = pagenumber_v;
}

static void
_elm_scroll_page_bring_in(Evas_Object *obj,
                          int pagenumber_h,
                          int pagenumber_v)
{
   Evas_Coord w, h;
   Evas_Coord x = 0;
   Evas_Coord y = 0;

   ELM_SCROLL_IFACE_DATA_GET_OR_RETURN(obj, sid);

   sid->current_page.x = _elm_scroll_page_x_get(sid, 0, EINA_FALSE);
   sid->current_page.y = _elm_scroll_page_y_get(sid, 0, EINA_FALSE);

   _elm_scroll_content_viewport_size_get(sid->obj, &w, &h);
   if (pagenumber_h >= 0) x = sid->pagesize_h * pagenumber_h;
   if (pagenumber_v >= 0) y = sid->pagesize_v * pagenumber_v;
   if (sid->loop_v || sid->loop_h)
     {
        if (pagenumber_h < 0) x = sid->pagesize_h * pagenumber_h;
        if (pagenumber_v < 0) y = sid->pagesize_v * pagenumber_v;
     }

   if (_elm_scroll_content_region_show_internal(obj, &x, &y, w, h))
     {
        _elm_scroll_scroll_to_x(sid, _elm_config->bring_in_scroll_friction, x);
        _elm_scroll_scroll_to_y(sid, _elm_config->bring_in_scroll_friction, y);
     }
}

static void
_elm_scroll_region_bring_in(Evas_Object *obj,
                            Evas_Coord x,
                            Evas_Coord y,
                            Evas_Coord w,
                            Evas_Coord h)
{
   ELM_SCROLL_IFACE_DATA_GET_OR_RETURN(obj, sid);

   if (_elm_scroll_content_region_show_internal(obj, &x, &y, w, h))
     {
        _elm_scroll_scroll_to_x(sid, _elm_config->bring_in_scroll_friction, x);
        _elm_scroll_scroll_to_y(sid, _elm_config->bring_in_scroll_friction, y);
     }
}

static void
_elm_scroll_gravity_set(Evas_Object *obj,
                        double x,
                        double y)
{
   ELM_SCROLL_IFACE_DATA_GET_OR_RETURN(obj, sid);

   if (!sid->pan_obj) return;

   ELM_PAN_DATA_GET_OR_RETURN(sid->pan_obj, psd);

   psd->api->gravity_set(sid->pan_obj, x, y);
}

static void
_elm_scroll_gravity_get(const Evas_Object *obj,
                        double *x,
                        double *y)
{
   ELM_SCROLL_IFACE_DATA_GET_OR_RETURN(obj, sid);

   if (!sid->pan_obj) return;

   ELM_PAN_DATA_GET_OR_RETURN(sid->pan_obj, psd);

   psd->api->gravity_get(sid->pan_obj, x, y);
}


static void
_elm_scroll_loop_set(Evas_Object *obj,
                              Eina_Bool loop_h,
                              Eina_Bool loop_v)
{
   ELM_SCROLL_IFACE_DATA_GET_OR_RETURN(obj, sid);

   if (sid->loop_h == loop_h &&
       sid->loop_v == loop_v)
     return;

   sid->loop_h = loop_h;
   sid->loop_v = loop_v;
}

static void
_elm_scroll_loop_get(const Evas_Object *obj,
                              Eina_Bool *loop_h,
                              Eina_Bool *loop_v)
{
   ELM_SCROLL_IFACE_DATA_GET_OR_RETURN(obj, sid);

   *loop_h = sid->loop_h;
   *loop_v = sid->loop_v;
}

static void
_elm_scroll_auto_scroll_set(Evas_Object *obj, Eina_Bool enable)
{
        ELM_SCROLL_IFACE_DATA_GET_OR_RETURN(obj, sid);

	sid->is_auto_scroll = enable;
}

static Eina_Bool
_elm_scroll_auto_scroll_get(const Evas_Object *obj)
{
        ELM_SCROLL_IFACE_DATA_GET_OR_RETURN_VAL(obj, sid, EINA_FALSE);

	return sid->is_auto_scroll;
}

static void
_elm_scroll_page_flick_set(Evas_Object *obj, Eina_Bool enable)
{
        ELM_SCROLL_IFACE_DATA_GET_OR_RETURN(obj, sid);

	sid->is_page_flick = enable;
}

static Eina_Bool
_elm_scroll_page_flick_get(const Evas_Object *obj)
{
        ELM_SCROLL_IFACE_DATA_GET_OR_RETURN_VAL(obj, sid, EINA_FALSE);

	return sid->is_page_flick;
}

static void
_elm_scroll_rotary_event_set(Evas_Object *obj, Eina_Bool enable)
{
	ELM_SCROLL_IFACE_DATA_GET_OR_RETURN(obj, sid);

	sid->is_rotary_event_enable = enable;
}

static Eina_Bool
_elm_scroll_rotary_event_get(const Evas_Object *obj)
{
        ELM_SCROLL_IFACE_DATA_GET_OR_RETURN_VAL(obj, sid, EINA_FALSE);

	return sid->is_rotary_event_enable;
}

static Eina_Bool
_elm_scroll_interface_add(Evas_Object *obj)
{
   ELM_SCROLL_IFACE_DATA_GET_OR_RETURN_VAL(obj, sid, EINA_FALSE);

   memset(sid, 0, sizeof(*sid));

   sid->obj = obj;

   sid->x = 0;
   sid->y = 0;
   sid->w = 0;
   sid->h = 0;
   sid->step.x = 32;
   sid->step.y = 32;
   sid->page.x = -50;
   sid->page.y = -50;
   sid->page_limit_h = 9999;
   sid->page_limit_v = 9999;
   sid->hbar_flags = ELM_SCROLLER_POLICY_AUTO;
   sid->vbar_flags = ELM_SCROLLER_POLICY_AUTO;
   sid->hbar_visible = EINA_TRUE;
   sid->vbar_visible = EINA_TRUE;
   sid->loop_h = EINA_FALSE;
   sid->loop_v = EINA_FALSE;
   sid->origin_x = EINA_FALSE;
   sid->origin_y = EINA_FALSE;

   sid->bounce_horiz = EINA_TRUE;
   sid->bounce_vert = EINA_TRUE;

   sid->one_direction_at_a_time = ELM_SCROLLER_SINGLE_DIRECTION_SOFT;
   sid->momentum_animator_disabled = EINA_FALSE;
   sid->bounce_animator_disabled = EINA_FALSE;
   sid->block = ELM_SCROLLER_MOVEMENT_NO_BLOCK;

   sid->is_auto_scroll = EINA_TRUE;
   sid->is_page_flick = EINA_FALSE;
   sid->is_rotary_event_enable = EINA_FALSE;
   sid->is_unset_cb_called = EINA_FALSE;

   sid->rotary_animation_info.rotary_animator = NULL;
   sid->rotary_animation_info.distance_v = 0;
   sid->rotary_animation_info.distance_h = 0;
   sid->rotary_animation_info.direction_v = 0;
   sid->rotary_animation_info.direction_h = 0;
   sid->rotary_animation_info.detent_count = 0;
   sid->rotary_animation_info.current_page = 0;

   _elm_scroll_scroll_bar_reset(sid);

   return EINA_TRUE;
}

static void
_elm_scroll_interface_del(Evas_Object *obj)
{
   ELM_SCROLL_IFACE_DATA_GET_OR_RETURN(obj, sid);

   _elm_scroll_content_set(obj, NULL);
   if (!sid->extern_pan) evas_object_del(sid->pan_obj);

   if (sid->down.hold_animator)
     ecore_animator_del(sid->down.hold_animator);
   if (sid->down.onhold_animator)
     ecore_animator_del(sid->down.onhold_animator);
   if (sid->down.momentum_animator)
     ecore_animator_del(sid->down.momentum_animator);
   if (sid->down.bounce_x_animator)
     ecore_animator_del(sid->down.bounce_x_animator);
   if (sid->down.bounce_y_animator)
     ecore_animator_del(sid->down.bounce_y_animator);
   if (sid->scrollto.x.animator) ecore_animator_del(sid->scrollto.x.animator);
   if (sid->scrollto.y.animator) ecore_animator_del(sid->scrollto.y.animator);
}

EAPI const char ELM_SCROLLABLE_IFACE_NAME[] = "elm_interface_scrollable";

EAPI const Elm_Scrollable_Smart_Interface ELM_SCROLLABLE_IFACE =
{
   {
      ELM_SCROLLABLE_IFACE_NAME,
      sizeof(Elm_Scrollable_Smart_Interface_Data),
      _elm_scroll_interface_add,
      _elm_scroll_interface_del
   },

   _elm_scroll_objects_set,
   _elm_scroll_content_set,
   _elm_scroll_extern_pan_set,
   _elm_scroll_drag_start_cb_set,
   _elm_scroll_drag_stop_cb_set,
   _elm_scroll_animate_start_cb_set,
   _elm_scroll_animate_stop_cb_set,
   _elm_scroll_scroll_cb_set,
   _elm_scroll_scroll_left_cb_set,
   _elm_scroll_scroll_right_cb_set,
   _elm_scroll_scroll_up_cb_set,
   _elm_scroll_scroll_down_cb_set,
   _elm_scroll_edge_left_cb_set,
   _elm_scroll_edge_right_cb_set,
   _elm_scroll_edge_top_cb_set,
   _elm_scroll_edge_bottom_cb_set,
   _elm_scroll_vbar_drag_cb_set,
   _elm_scroll_vbar_press_cb_set,
   _elm_scroll_vbar_unpress_cb_set,
   _elm_scroll_hbar_drag_cb_set,
   _elm_scroll_hbar_press_cb_set,
   _elm_scroll_hbar_unpress_cb_set,
   _elm_scroll_content_min_limit_cb_set,
   _elm_scroll_content_viewport_resize_cb_set,
   _elm_scroll_item_scroll_align_end_set_cb_set,
   _elm_scroll_item_scroll_align_end_unset_cb_set,
   _elm_scroll_content_set_cb_set,
   _elm_scroll_item_scroll_aligned_get_cb_set,
   _elm_scroll_scroll_position_get_cb_set,
   _elm_scroll_content_pos_set,
   _elm_scroll_content_pos_get,
   _elm_scroll_content_region_show,
   _elm_scroll_content_region_set,
   _elm_scroll_content_size_get,
   _elm_scroll_content_viewport_size_get,
   _elm_scroll_content_viewport_pos_get,
   _elm_scroll_content_min_limit,
   _elm_scroll_step_size_set,
   _elm_scroll_step_size_get,
   _elm_scroll_page_size_set,
   _elm_scroll_page_size_get,
   _elm_scroll_policy_set,
   _elm_scroll_policy_get,
   _elm_scroll_single_direction_set,
   _elm_scroll_single_direction_get,
   _elm_scroll_repeat_events_set,
   _elm_scroll_repeat_events_get,
   _elm_scroll_mirrored_set,
   _elm_scroll_hold_set,
   _elm_scroll_freeze_set,
   _elm_scroll_bounce_allow_set,
   _elm_scroll_bounce_allow_get,
   _elm_scroll_origin_reverse_set,
   _elm_scroll_origin_reverse_get,
   _elm_scroll_paging_set,
   _elm_scroll_paging_get,
   _elm_scroll_page_scroll_limit_set,
   _elm_scroll_page_scroll_limit_get,
   _elm_scroll_current_page_get,
   _elm_scroll_last_page_get,
   _elm_scroll_page_show,
   _elm_scroll_page_bring_in,
   _elm_scroll_region_bring_in,
   _elm_scroll_gravity_set,
   _elm_scroll_gravity_get,
   _elm_scroll_loop_set,
   _elm_scroll_loop_get,
   _elm_scroll_momentum_animator_disabled_get,
   _elm_scroll_momentum_animator_disabled_set,
   _elm_scroll_bounce_animator_disabled_set,
   _elm_scroll_bounce_animator_disabled_get,
   _elm_scroll_wheel_disabled_get,
   _elm_scroll_wheel_disabled_set,
   _elm_scroll_movement_block_set,
   _elm_scroll_movement_block_get,
   _elm_scroll_auto_scroll_set,
   _elm_scroll_auto_scroll_get,
   _elm_scroll_page_flick_set,
   _elm_scroll_page_flick_get,
   _elm_scroll_rotary_event_set,
   _elm_scroll_rotary_event_get
};
