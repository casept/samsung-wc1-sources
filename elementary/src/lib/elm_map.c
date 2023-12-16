#include <Elementary.h>
#include "elm_priv.h"
#include "elm_widget_map.h"

EAPI const char ELM_MAP_SMART_NAME[] = "elm_map";
EAPI const char ELM_MAP_PAN_SMART_NAME[] = "elm_map_pan";

#define MAP_ENGINE_OVERLAY_SUPPORT(wsd) (wsd->engine->icon_add && \
                                        wsd->engine->icon_del && \
                                        wsd->engine->object_from_coord && \
                                        wsd->engine->object_visibility && \
                                        ENABLE_MAP_ENGINE_OVERLAYS)
#define MAP_ENGINE_GROUP_OVERLAY_SUPPORT(wsd) (MAP_ENGINE_OVERLAY_SUPPORT(wsd) && \
                                              wsd->engine->group_create && \
                                              wsd->engine->group_del &&\
                                              wsd->engine->group_object_add && \
                                              wsd->engine->group_object_del && \
                                              ENABLE_MAP_ENGINE_OVERLAYS)

#define IS_EXTERNAL_ENGINE(wsd) strcmp(wsd->engine->name, INTERNAL_ENGINE_NAME)

#define INTERNAL_ENGINE_NAME    "Elm_Map_Tile"
#define OVERLAY_CLASS_ZOOM_MAX  255
#define MAX_CONCURRENT_DOWNLOAD 10

#define ROUND(z) (((z) < 0) ? (int)ceil((z) - 0.005) : (int)floor((z) + 0.005))
#define EVAS_MAP_POINT         4
#define DEFAULT_TILE_SIZE      256
#define MARER_MAX_NUMBER       30
#define OVERLAY_GROUPING_SCALE 2
#define ROTATE_THRESH 15.0

#define CACHE_ROOT             "/elm_map"
#define CACHE_TILE_ROOT        CACHE_ROOT "/%d/%d/%d"
#define CACHE_TILE_PATH        "%s/%d.png"
#define CACHE_ROUTE_ROOT       CACHE_ROOT "/route"
#define CACHE_NAME_ROOT        CACHE_ROOT "/name"

#define ROUTE_YOURS_URL        "http://www.yournavigation.org/api/dev/route.php"
#define ROUTE_TYPE_MOTORCAR    "motocar"
#define ROUTE_TYPE_BICYCLE     "bicycle"
#define ROUTE_TYPE_FOOT        "foot"
#define YOURS_DISTANCE         "distance"
#define YOURS_DESCRIPTION      "description"
#define YOURS_COORDINATES      "coordinates"

#define NAME_NOMINATIM_URL     "http://nominatim.openstreetmap.org"
#define NOMINATIM_RESULT       "result"
#define NOMINATIM_PLACE        "place"
#define NOMINATIM_ATTR_LON     "lon"
#define NOMINATIM_ATTR_LAT     "lat"
#define NOMINATIM_ATTR_ADDRESS "display_name"

static Eina_Bool ENABLE_MAP_ENGINE_OVERLAYS = EINA_FALSE;
static unsigned int scroll_threshold = 5;

#ifdef HAVE_ELEMENTARY_ECORE_CON

static void _overlay_place(Elm_Map_Smart_Data *sd);

static char *
_mapnik_url_cb(const Evas_Object *obj __UNUSED__,
               int x,
               int y,
               int zoom)
{
   char buf[PATH_MAX];

   // ((x+y+zoom)%3)+'a' is requesting map images from distributed
   // tile servers (eg., a, b, c)
   snprintf(buf, sizeof(buf), "http://%c.tile.openstreetmap.org/%d/%d/%d.png",
            ((x + y + zoom) % 3) + 'a', zoom, x, y);
   return strdup(buf);
}

static char *
_osmarender_url_cb(const Evas_Object *obj __UNUSED__,
                   int x,
                   int y,
                   int zoom)
{
   char buf[PATH_MAX];

   snprintf(buf, sizeof(buf),
            "http://%c.tah.openstreetmap.org/Tiles/tile/%d/%d/%d.png",
            ((x + y + zoom) % 3) + 'a', zoom, x, y);

   return strdup(buf);
}

static char *
_cyclemap_url_cb(const Evas_Object *obj __UNUSED__,
                 int x,
                 int y,
                 int zoom)
{
   char buf[PATH_MAX];

   snprintf(buf, sizeof(buf),
            "http://%c.tile.opencyclemap.org/cycle/%d/%d/%d.png",
            ((x + y + zoom) % 3) + 'a', zoom, x, y);

   return strdup(buf);
}

static char *
_mapquest_url_cb(const Evas_Object *obj __UNUSED__,
                 int x,
                 int y,
                 int zoom)
{
   char buf[PATH_MAX];

   snprintf(buf, sizeof(buf),
            "http://otile%d.mqcdn.com/tiles/1.0.0/osm/%d/%d/%d.png",
            ((x + y + zoom) % 4) + 1, zoom, x, y);

   return strdup(buf);
}

static char *
_mapquest_aerial_url_cb(const Evas_Object *obj __UNUSED__,
                        int x,
                        int y,
                        int zoom)
{
   char buf[PATH_MAX];

   snprintf(buf, sizeof(buf), "http://oatile%d.mqcdn.com/naip/%d/%d/%d.png",
            ((x + y + zoom) % 4) + 1, zoom, x, y);

   return strdup(buf);
}

static char *
_yours_url_cb(const Evas_Object *obj __UNUSED__,
              const char *type_name,
              int method,
              double flon,
              double flat,
              double tlon,
              double tlat)
{
   char buf[PATH_MAX];

   snprintf
     (buf, sizeof(buf),
     "%s?flat=%lf&flon=%lf&tlat=%lf&tlon=%lf&v=%s&fast=%d&instructions=1",
     ROUTE_YOURS_URL, flat, flon, tlat, tlon, type_name, method);

   return strdup(buf);
}

// TODO: fix monav api
/*
   static char *
   _monav_url_cb(const Evas_Object *obj __UNUSED__,
              char *type_name,
              int method,
              double flon,
              double flat,
              double tlon,
              double tlat)
   {
   char buf[PATH_MAX];

   snprintf(buf, sizeof(buf),
            "%s?flat=%f&flon=%f&tlat=%f&tlon=%f&v=%s&fast=%d&instructions=1",
            ROUTE_MONAV_URL, flat, flon, tlat, tlon, type_name, method);

   return strdup(buf);
   }

   //TODO: fix ors api

   static char *
   _ors_url_cb(const Evas_Object *obj __UNUSED__,
            char *type_name,
            int method,
            double flon,
            double flat,
            double tlon,
            double tlat)
   {
   char buf[PATH_MAX];

   snprintf(buf, sizeof(buf),
            "%s?flat=%f&flon=%f&tlat=%f&tlon=%f&v=%s&fast=%d&instructions=1",
            ROUTE_ORS_URL, flat, flon, tlat, tlon, type_name, method);

   return strdup(buf);
   }
 */

static char *
_nominatim_url_cb(const Evas_Object *obj,
                  int method,
                  const char *name,
                  double lon,
                  double lat)
{
   char **str;
   char buf[PATH_MAX];
   unsigned int ele, idx;
   char search_url[PATH_MAX];

   ELM_MAP_DATA_GET(obj, sd);

   if (method == ELM_MAP_NAME_METHOD_SEARCH)
     {
        search_url[0] = '\0';
        str = eina_str_split_full(name, " ", 0, &ele);
        for (idx = 0; idx < ele; idx++)
          {
             eina_strlcat(search_url, str[idx], sizeof(search_url));
             if (!(idx == (ele - 1)))
               eina_strlcat(search_url, "+", sizeof(search_url));
          }
        snprintf(buf, sizeof(buf),
                 "%s/search?q=%s&format=xml&polygon=0&addressdetails=0",
                 NAME_NOMINATIM_URL, search_url);

        if (str)
          {
             if (str[0]) free(str[0]);
             free(str);
          }
     }
   else if (method == ELM_MAP_NAME_METHOD_REVERSE)
     snprintf(buf, sizeof(buf),
              "%s/reverse?format=xml&lat=%lf&lon=%lf&zoom=%d&addressdetails=0",
              NAME_NOMINATIM_URL, lat, lon, (int)sd->zoom);
   else strcpy(buf, "");

   return strdup(buf);
}

// Refer : http://wiki.openstreetmap.org/wiki/FAQ
// meters per pixel when latitude is 0 (equator)
// meters per pixel  = _osm_scale_meter[zoom] * cos (latitude)
const double _osm_scale_meter[] =
{
   78206, 39135.758482, 19567.879241, 9783.939621, 4891.969810,
   2445.984905, 1222.992453, 611.496226, 305.748113, 152.874057, 76.437028,
   38.218514, 19.109257, 9.554629, 4.777314, 2.388657, 1.194329, 0.597164,
   0.29858
};

static double
_scale_cb(const Evas_Object *obj __UNUSED__,
          double lon __UNUSED__,
          double lat,
          int zoom)
{
   if (zoom < 0 || zoom >= (int)(sizeof(_osm_scale_meter)/sizeof(_osm_scale_meter[0]))) return 0;
   return _osm_scale_meter[zoom] / cos(lat * ELM_PI / 180.0);
}

const Source_Tile src_tiles[] =
{
   {"Mapnik", 0, 18, _mapnik_url_cb, NULL, NULL, _scale_cb},
   {"Osmarender", 0, 17, _osmarender_url_cb, NULL, NULL, _scale_cb},
   {"CycleMap", 0, 16, _cyclemap_url_cb, NULL, NULL, _scale_cb},
   {"MapQuest", 0, 18, _mapquest_url_cb, NULL, NULL, _scale_cb},
   {"MapQuest Open Aerial", 0, 11, _mapquest_aerial_url_cb, NULL, NULL,
    _scale_cb}
};

// FIXME: Fix more open sources
const Source_Route src_routes[] =
{
   {"Yours", _yours_url_cb}    // http://www.yournavigation.org/
   //{"Monav", _monav_url_cb},
   //{"ORS", _ors_url_cb},     // http://www.openrouteservice.org
};

// Scale in meters
const double _scale_tb[] =
{
   10000000, 5000000, 2000000, 1000000, 500000, 200000, 100000, 50000,
   20000, 10000, 5000, 2000, 1000, 500, 500, 200, 100, 50, 20, 10, 5, 2, 1
};

// FIXME: Add more open sources
const Source_Name src_names[] =
{
   {"Nominatim", _nominatim_url_cb}
};

static int id_num = 1;

static const char SIG_CLICKED[] = "clicked";
static const char SIG_CLICKED_DOUBLE[] = "clicked,double";
static const char SIG_PRESS[] = "press";
static const char SIG_LONGPRESSED[] = "longpressed";
static const char SIG_SCROLL[] = "scroll";
static const char SIG_SCROLL_DRAG_START[] = "scroll,drag,start";
static const char SIG_SCROLL_DRAG_STOP[] = "scroll,drag,stop";
static const char SIG_SCROLL_ANIM_START[] = "scroll,anim,start";
static const char SIG_SCROLL_ANIM_STOP[] = "scroll,anim,stop";
static const char SIG_ZOOM_START[] = "zoom,start";
static const char SIG_ZOOM_STOP[] = "zoom,stop";
static const char SIG_ZOOM_CHANGE[] = "zoom,change";
static const char SIG_LOADED[] = "loaded";
static const char SIG_TILE_LOAD[] = "tile,load";
static const char SIG_TILE_LOADED[] = "tile,loaded";
static const char SIG_TILE_LOADED_FAIL[] = "tile,loaded,fail";
static const char SIG_ROUTE_LOAD[] = "route,load";
static const char SIG_ROUTE_LOADED[] = "route,loaded";
static const char SIG_ROUTE_LOADED_FAIL[] = "route,loaded,fail";
static const char SIG_NAME_LOAD[] = "name,load";
static const char SIG_NAME_LOADED[] = "name,loaded";
static const char SIG_NAME_LOADED_FAIL[] = "name,loaded,fail";
static const char SIG_OVERLAY_CLICKED[] = "overlay,clicked";
static const char SIG_OVERLAY_DEL[] = "overlay,del";
static const char SIG_LANG_CHANGED[] = "language,changed";

static const Evas_Smart_Cb_Description _smart_callbacks[] = {
   {SIG_CLICKED, ""},
   {SIG_CLICKED_DOUBLE, ""},
   {SIG_PRESS, ""},
   {SIG_LONGPRESSED, ""},
   {SIG_SCROLL, ""},
   {SIG_SCROLL_DRAG_START, ""},
   {SIG_SCROLL_DRAG_STOP, ""},
   {SIG_SCROLL_ANIM_START, ""},
   {SIG_SCROLL_ANIM_STOP, ""},
   {SIG_ZOOM_START, ""},
   {SIG_ZOOM_STOP, ""},
   {SIG_ZOOM_CHANGE, ""},
   {SIG_LOADED, ""},
   {SIG_TILE_LOAD, ""},
   {SIG_TILE_LOADED, ""},
   {SIG_TILE_LOADED_FAIL, ""},
   {SIG_ROUTE_LOAD, ""},
   {SIG_ROUTE_LOADED, ""},
   {SIG_ROUTE_LOADED_FAIL, ""},
   {SIG_NAME_LOAD, ""},
   {SIG_NAME_LOADED, ""},
   {SIG_NAME_LOADED_FAIL, ""},
   {SIG_OVERLAY_CLICKED, ""},
   {SIG_OVERLAY_DEL, ""},
   {SIG_LANG_CHANGED, ""},
   {NULL, NULL}
};

static const Evas_Smart_Interface *_smart_interfaces[] =
{
   (Evas_Smart_Interface *)&ELM_SCROLLABLE_IFACE, NULL
};

EVAS_SMART_SUBCLASS_IFACE_NEW
  (ELM_MAP_SMART_NAME, _elm_map, Elm_Map_Smart_Class,
  Elm_Widget_Smart_Class, elm_widget_smart_class_get, _smart_callbacks,
  _smart_interfaces);

ELM_INTERNAL_SMART_SUBCLASS_NEW
  (ELM_MAP_PAN_SMART_NAME, _elm_map_pan, Elm_Map_Pan_Smart_Class,
  Elm_Pan_Smart_Class, elm_pan_smart_class_get, NULL);

static Eina_Bool
_elm_map_smart_translate(Evas_Object *obj)
{
   evas_object_smart_callback_call(obj, SIG_LANG_CHANGED, NULL);
   return EINA_TRUE;
}

static void
_edj_overlay_size_get(Elm_Map_Smart_Data *sd,
                      Evas_Coord *w,
                      Evas_Coord *h)
{
   Evas_Object *edj = NULL;
   const char *s;

   EINA_SAFETY_ON_NULL_RETURN(w);
   EINA_SAFETY_ON_NULL_RETURN(h);

   edj = edje_object_add(evas_object_evas_get(ELM_WIDGET_DATA(sd)->obj));
   if (!IS_EXTERNAL_ENGINE(sd))
     {
        elm_widget_theme_object_set
        (ELM_WIDGET_DATA(sd)->obj, edj, "map/marker", "radio",
        elm_widget_style_get(ELM_WIDGET_DATA(sd)->obj));
     }

   s = edje_object_data_get(edj, "size_w");
   if (s) *w = atoi(s);
   else *w = 0;

   s = edje_object_data_get(edj, "size_h");
   if (s) *h = atoi(s);
   else *h = 0;

   evas_object_del(edj);
}

static void
_rotate_do(Evas_Coord x,
           Evas_Coord y,
           Evas_Coord cx,
           Evas_Coord cy,
           double degree,
           Evas_Coord *xx,
           Evas_Coord *yy)
{
   double r = (degree * M_PI) / 180.0;

   if (xx) *xx = ((x - cx) * cos(r)) + ((y - cy) * cos(r + M_PI_2)) + cx;
   if (yy) *yy = ((x - cx) * sin(r)) + ((y - cy) * sin(r + M_PI_2)) + cy;
}

static void
_obj_rotate(Elm_Map_Smart_Data *sd,
            Evas_Object *obj)
{
   Evas_Coord w, h, ow, oh;

   evas_map_util_points_populate_from_object(sd->map, obj);

   evas_object_geometry_get(obj, NULL, NULL, &ow, &oh);
   evas_object_image_size_get(obj, &w, &h);
   if ((w > ow) || (h > oh))
     {
        evas_map_point_image_uv_set(sd->map, 0, 0, 0);
        evas_map_point_image_uv_set(sd->map, 1, w, 0);
        evas_map_point_image_uv_set(sd->map, 2, w, h);
        evas_map_point_image_uv_set(sd->map, 3, 0, h);
     }
   evas_map_util_rotate(sd->map, sd->pan_rotate.d, sd->pan_rotate.cx,
                        sd->pan_rotate.cy);

   evas_object_map_set(obj, sd->map);
   evas_object_map_enable_set(obj, EINA_TRUE);
}

static void
_obj_place(Evas_Object *obj,
           Evas_Coord x,
           Evas_Coord y,
           Evas_Coord w,
           Evas_Coord h)
{
   EINA_SAFETY_ON_NULL_RETURN(obj);

   DBG("_obj_place obj = %p, %d, %d, %d, %d", obj, x, y, w, h);
   evas_object_move(obj, x, y);
   evas_object_resize(obj, w, h);
   evas_object_show(obj);
}

static void
_coord_to_region_convert(Elm_Map_Smart_Data *sd,
                         Evas_Coord x,
                         Evas_Coord y,
                         Evas_Coord size,
                         double *lon,
                         double *lat)
{
   int zoom;

   EINA_SAFETY_ON_NULL_RETURN(sd);

   zoom = floor(log(size / sd->size.tile) / log(2));
   if ((sd->src_tile) && (sd->src_tile->coord_to_geo))
     {
        if (sd->src_tile->coord_to_geo
              (ELM_WIDGET_DATA(sd)->obj, zoom, x, y, size, lon, lat))
          return;
     }

   if (IS_EXTERNAL_ENGINE(sd))
     {
        sd->engine->canvas_to_region
               (ELM_WIDGET_DATA(sd)->obj, x, y, lon, lat);
        return;
     }

   if (lon) *lon = (x / (double)size * 360.0) - 180;
   if (lat)
     {
        double n = ELM_PI - (2.0 * ELM_PI * y / size);
        *lat = 180.0 / ELM_PI *atan(0.5 * (exp(n) - exp(-n)));
     }
}

static void
_region_to_coord_convert(Elm_Map_Smart_Data *sd,
                         double lon,
                         double lat,
                         Evas_Coord size,
                         Evas_Coord *x,
                         Evas_Coord *y)
{
   int zoom;

   EINA_SAFETY_ON_NULL_RETURN(sd);

   zoom = floor(log(size / 256) / log(2));

   if (IS_EXTERNAL_ENGINE(sd))
     {
        sd->engine->region_to_canvas
              (ELM_WIDGET_DATA(sd)->obj, lon, lat, x, y);
        return;
     }

   if ((sd->src_tile) && (sd->src_tile->geo_to_coord))
     {
        if (sd->src_tile->geo_to_coord
              (ELM_WIDGET_DATA(sd)->obj, zoom, lon, lat, size, x, y)) return;
     }

   if (x) *x = floor((lon + 180.0) / 360.0 * size);
   if (y)
     *y = floor((1.0 - log(tan(lat * ELM_PI / 180.0) +
                           (1.0 / cos(lat * ELM_PI / 180.0)))
                 / ELM_PI) / 2.0 * size);
}

static void
_viewport_coord_get(Elm_Map_Smart_Data *sd,
                    Evas_Coord *vx,
                    Evas_Coord *vy,
                    Evas_Coord *vw,
                    Evas_Coord *vh)
{
   Evas_Coord x, y, w, h;

   EINA_SAFETY_ON_NULL_RETURN(sd);

   sd->s_iface->content_pos_get(ELM_WIDGET_DATA(sd)->obj, &x, &y);
   sd->s_iface->content_viewport_size_get(ELM_WIDGET_DATA(sd)->obj, &w, &h);

   if (w > sd->size.w) x -= ((w - sd->size.w) / 2);
   if (h > sd->size.h) y -= ((h - sd->size.h) / 2);
   if (vx) *vx = x;
   if (vy) *vy = y;
   if (vw) *vw = w;
   if (vh) *vh = h;
}

// Map coordinates to canvas geometry without rotate
static void
_coord_to_canvas_no_rotation(Elm_Map_Smart_Data *sd,
                             Evas_Coord x,
                             Evas_Coord y,
                             Evas_Coord *xx,
                             Evas_Coord *yy)
{
   Evas_Coord vx, vy, sx, sy;

   if (IS_EXTERNAL_ENGINE(sd))
     {
        if (xx) *xx = x;
        if (yy) *yy = y;
     }
   else
     {
        _viewport_coord_get(sd, &vx, &vy, NULL, NULL);
        evas_object_geometry_get(sd->pan_obj, &sx, &sy, NULL, NULL);
        if (xx) *xx = x - vx + sx;
        if (yy) *yy = y - vy + sy;
     }
}

// Map coordinates to canvas geometry
static void
_coord_to_canvas(Elm_Map_Smart_Data *sd,
                 Evas_Coord x,
                 Evas_Coord y,
                 Evas_Coord *xx,
                 Evas_Coord *yy)
{
   _coord_to_canvas_no_rotation(sd, x, y, &x, &y);
   if (!IS_EXTERNAL_ENGINE(sd))
     {
        _rotate_do(x, y, sd->pan_rotate.cx, sd->pan_rotate.cy, sd->pan_rotate.d, &x, &y);
     }
   if (xx) *xx = x;
   if (yy) *yy = y;
}

// Canvas geometry to map coordinates
static void
_canvas_to_coord(Elm_Map_Smart_Data *sd,
                 Evas_Coord x,
                 Evas_Coord y,
                 Evas_Coord *xx,
                 Evas_Coord *yy)
{
   Evas_Coord vx, vy, sx, sy;

   _viewport_coord_get(sd, &vx, &vy, NULL, NULL);
   evas_object_geometry_get(sd->pan_obj, &sx, &sy, NULL, NULL);
   _rotate_do(x - sx + vx, y - sy + vy, sd->pan_rotate.cx - sx + vx,
              sd->pan_rotate.cy - sy + vy, -sd->pan_rotate.d, &x, &y);
   if (xx) *xx = x;
   if (yy) *yy = y;
}

static void
_grid_item_coord_get(Grid_Item *gi,
                     int *x,
                     int *y,
                     int *w,
                     int *h)
{
   EINA_SAFETY_ON_NULL_RETURN(gi);

   if (x) *x = gi->x * gi->wsd->size.tile;
   if (y) *y = gi->y * gi->wsd->size.tile;
   if (w) *w = gi->wsd->size.tile;
   if (h) *h = gi->wsd->size.tile;
}

static Eina_Bool
_grid_item_in_viewport(Grid_Item *gi)
{
   Evas_Coord vx, vy, vw, vh;
   Evas_Coord x, y, w, h;

   EINA_SAFETY_ON_NULL_RETURN_VAL(gi, EINA_FALSE);

   _viewport_coord_get(gi->wsd, &vx, &vy, &vw, &vh);
   _grid_item_coord_get(gi, &x, &y, &w, &h);

   return ELM_RECTS_INTERSECT(x, y, w, h, vx, vy, vw, vh);
}

static Eina_Bool
_loaded_timeout_cb(void *data)
{
   Elm_Map_Smart_Data *sd = data;

   EINA_SAFETY_ON_NULL_RETURN_VAL(data, EINA_FALSE);

   sd->loaded_timer = NULL;
   if (!(sd->download_num) && !(sd->download_idler))
     evas_object_smart_callback_call
       (ELM_WIDGET_DATA(sd)->obj, SIG_LOADED, NULL);
   return ECORE_CALLBACK_CANCEL;
}

static void
_grid_item_update(Grid_Item *gi)
{
   Evas_Load_Error err;

   EINA_SAFETY_ON_NULL_RETURN(gi);

   evas_object_image_file_set(gi->img, gi->file, NULL);
   if (!gi->wsd->zoom_timer && !gi->wsd->scr_timer)
     evas_object_image_smooth_scale_set(gi->img, EINA_TRUE);
   else evas_object_image_smooth_scale_set(gi->img, EINA_FALSE);

   err = evas_object_image_load_error_get(gi->img);
   if (err != EVAS_LOAD_ERROR_NONE)
     {
        ERR("Image loading error (%s): %s", gi->file, evas_load_error_str(err));
        ecore_file_remove(gi->file);
        gi->file_have = EINA_FALSE;
     }
   else
     {
        Evas_Coord x, y, w, h;

        _grid_item_coord_get(gi, &x, &y, &w, &h);
        _coord_to_canvas_no_rotation(gi->wsd, x, y, &x, &y);
        _obj_place(gi->img, x, y, w, h);
        _obj_rotate(gi->wsd, gi->img);
        gi->file_have = EINA_TRUE;
     }

   if (gi->wsd->loaded_timer) ecore_timer_del(gi->wsd->loaded_timer);
   gi->wsd->loaded_timer = ecore_timer_add(0.25, _loaded_timeout_cb, gi->wsd);
}

static void
_grid_item_load(Grid_Item *gi)
{
   EINA_SAFETY_ON_NULL_RETURN(gi);

   if (gi->file_have) _grid_item_update(gi);
   else if (!gi->job)
     {
        gi->wsd->download_list = eina_list_remove(gi->wsd->download_list, gi);
        gi->wsd->download_list = eina_list_append(gi->wsd->download_list, gi);
     }
}

static void
_grid_item_unload(Grid_Item *gi)
{
   EINA_SAFETY_ON_NULL_RETURN(gi);

   if (gi->file_have)
     {
        evas_object_hide(gi->img);
        evas_object_image_file_set(gi->img, NULL, NULL);
     }
   else if (gi->job)
     {
        ecore_file_download_abort(gi->job);
        ecore_file_remove(gi->file);
        gi->job = NULL;
        gi->wsd->try_num--;
     }
   else gi->wsd->download_list = eina_list_remove(gi->wsd->download_list, gi);
}

static Grid_Item *
_grid_item_create(Grid *g,
                  Evas_Coord x,
                  Evas_Coord y)
{
   char buf[PATH_MAX];
   char buf2[PATH_MAX];
   Grid_Item *gi;
   char *url;

   EINA_SAFETY_ON_NULL_RETURN_VAL(g, NULL);

   gi = ELM_NEW(Grid_Item);
   gi->wsd = g->wsd;
   gi->g = g;
   gi->x = x;
   gi->y = y;

   gi->file_have = EINA_FALSE;
   gi->job = NULL;

   gi->img = evas_object_image_add
       (evas_object_evas_get(ELM_WIDGET_DATA(g->wsd)->obj));
   evas_object_image_smooth_scale_set(gi->img, EINA_FALSE);
   evas_object_image_scale_hint_set(gi->img, EVAS_IMAGE_SCALE_HINT_DYNAMIC);
   evas_object_image_filled_set(gi->img, EINA_TRUE);
   evas_object_smart_member_add(gi->img, g->wsd->pan_obj);
   evas_object_pass_events_set(gi->img, EINA_TRUE);
   evas_object_stack_below(gi->img, g->wsd->sep_maps_overlays);

   {
      const char *cachedir;

#ifdef ELM_EFREET
      snprintf(buf, sizeof(buf), "%s" CACHE_TILE_ROOT, efreet_cache_home_get(),
               g->wsd->id, g->zoom, x);
      (void)cachedir;
#else
      cachedir = getenv("XDG_CACHE_HOME");
      snprintf(buf, sizeof(buf), "%s/%s" CACHE_TILE_ROOT, getenv("HOME"),
               cachedir ? : "/.config", g->wsd->id, g->zoom, x);
#endif
   }

   snprintf(buf2, sizeof(buf2), CACHE_TILE_PATH, buf, y);
   if (!ecore_file_exists(buf)) ecore_file_mkpath(buf);

   eina_stringshare_replace(&gi->file, buf2);
   url = g->wsd->src_tile->url_cb(ELM_WIDGET_DATA(g->wsd)->obj, x, y, g->zoom);
   if ((!url) || (!strlen(url)))
     {
        eina_stringshare_replace(&gi->url, NULL);
        ERR("Getting source url failed: %s", gi->file);
     }
   else eina_stringshare_replace(&gi->url, url);

   if (url) free(url);
   eina_matrixsparse_data_idx_set(g->grid, y, x, gi);

   return gi;
}

static void
_grid_item_free(Grid_Item *gi)
{
   EINA_SAFETY_ON_NULL_RETURN(gi);

   _grid_item_unload(gi);
   if (gi->g && gi->g->grid)
     eina_matrixsparse_data_idx_set(gi->g->grid, gi->y, gi->x, NULL);
   if (gi->url) eina_stringshare_del(gi->url);
   if (gi->file_have) ecore_file_remove(gi->file);
   if (gi->file) eina_stringshare_del(gi->file);
   if (gi->img) evas_object_del(gi->img);

   free(gi);
}

static void
_downloaded_cb(void *data,
               const char *file __UNUSED__,
               int status)
{
   Grid_Item *gi = data;

   if (status == 200)
     {
        DBG("Download success from %s to %s", gi->url, gi->file);

        _grid_item_update(gi);
        gi->wsd->finish_num++;
        evas_object_smart_callback_call
          (ELM_WIDGET_DATA(gi->wsd)->obj, SIG_TILE_LOADED, NULL);
     }
   else
     {
        WRN("Download failed from %s to %s (%d) ", gi->url, gi->file, status);

        ecore_file_remove(gi->file);
        gi->file_have = EINA_FALSE;
        evas_object_smart_callback_call
          (ELM_WIDGET_DATA(gi->wsd)->obj, SIG_TILE_LOADED_FAIL, NULL);
     }

   gi->job = NULL;
   gi->wsd->download_num--;
   if (!gi->wsd->download_num)
     edje_object_signal_emit(ELM_WIDGET_DATA(gi->wsd)->resize_obj,
                             "elm,state,busy,stop", "elm");
}

static Eina_Bool
_download_job(void *data)
{
   Elm_Map_Smart_Data *sd = data;
   Eina_List *l, *ll;
   Grid_Item *gi;

   if (!eina_list_count(sd->download_list))
     {
        sd->download_idler = NULL;
        return ECORE_CALLBACK_CANCEL;
     }

   EINA_LIST_REVERSE_FOREACH_SAFE(sd->download_list, l, ll, gi)
   {
      Eina_Bool ret;

      if ((gi->g->zoom != sd->zoom) || !(_grid_item_in_viewport(gi)))
        {
           sd->download_list = eina_list_remove(sd->download_list, gi);
           continue;
        }
      if (sd->download_num >= MAX_CONCURRENT_DOWNLOAD)
        return ECORE_CALLBACK_RENEW;

      ret = ecore_file_download_full
          (gi->url, gi->file, _downloaded_cb, NULL, gi, &(gi->job), sd->ua);

      if ((!ret) || (!gi->job))
        ERR("Can't start to download from %s to %s", gi->url, gi->file);
      else
        {
           sd->download_list = eina_list_remove(sd->download_list, gi);
           sd->try_num++;
           sd->download_num++;
           evas_object_smart_callback_call
             (ELM_WIDGET_DATA(sd)->obj, SIG_TILE_LOAD, NULL);
           if (sd->download_num == 1)
             edje_object_signal_emit(ELM_WIDGET_DATA(sd)->resize_obj,
                                     "elm,state,busy,start", "elm");
        }
   }

   return ECORE_CALLBACK_RENEW;
}

static void
_grid_viewport_get(Grid *g,
                   int *x,
                   int *y,
                   int *w,
                   int *h)
{
   int xx, yy, ww, hh;
   Evas_Coord vx, vy, vw, vh;

   EINA_SAFETY_ON_NULL_RETURN(g);

   _viewport_coord_get(g->wsd, &vx, &vy, &vw, &vh);
   if (vx < 0) vx = 0;
   if (vy < 0) vy = 0;

   xx = (vx / g->wsd->size.tile) - 1;
   if (xx < 0) xx = 0;

   yy = (vy / g->wsd->size.tile) - 1;
   if (yy < 0) yy = 0;

   ww = (vw / g->wsd->size.tile) + 3;
   if (xx + ww >= g->tw) ww = g->tw - xx;

   hh = (vh / g->wsd->size.tile) + 3;
   if (yy + hh >= g->th) hh = g->th - yy;

   if (x) *x = xx;
   if (y) *y = yy;
   if (w) *w = ww;
   if (h) *h = hh;
}

static void
_grid_unload(Grid *g)
{
   Eina_Matrixsparse_Cell *cell;
   Eina_Iterator *it;
   Grid_Item *gi;

   EINA_SAFETY_ON_NULL_RETURN(g);

   it = eina_matrixsparse_iterator_new(g->grid);
   EINA_ITERATOR_FOREACH(it, cell)
     {
        gi = eina_matrixsparse_cell_data_get(cell);
        _grid_item_unload(gi);
     }
   eina_iterator_free(it);
}

static void
_grid_load(Grid *g)
{
   Eina_Matrixsparse_Cell *cell;
   int x, y, xx, yy, ww, hh;
   Eina_Iterator *it;
   Grid_Item *gi;

   EINA_SAFETY_ON_NULL_RETURN(g);

   it = eina_matrixsparse_iterator_new(g->grid);
   EINA_ITERATOR_FOREACH(it, cell)
     {
        gi = eina_matrixsparse_cell_data_get(cell);
        if (!_grid_item_in_viewport(gi)) _grid_item_unload(gi);
     }
   eina_iterator_free(it);

   _grid_viewport_get(g, &xx, &yy, &ww, &hh);
   for (y = yy; y < yy + hh; y++)
     {
        for (x = xx; x < xx + ww; x++)
          {
             gi = eina_matrixsparse_data_idx_get(g->grid, y, x);
             if (!gi) gi = _grid_item_create(g, x, y);
             _grid_item_load(gi);
          }
     }
}

static void
_grid_place(Elm_Map_Smart_Data *sd)
{
   Eina_List *l;
   Grid *g;

   EINA_SAFETY_ON_NULL_RETURN(sd);

   EINA_LIST_FOREACH(sd->grids, l, g)
     {
        if (sd->zoom == g->zoom) _grid_load(g);
        else _grid_unload(g);
     }
   if (!sd->download_idler)
     sd->download_idler = ecore_idler_add(_download_job, sd);
}

static void
_grid_all_create(Elm_Map_Smart_Data *sd)
{
   int zoom;

   EINA_SAFETY_ON_NULL_RETURN(sd->src_tile);

   for (zoom = sd->src_tile->zoom_min; zoom <= sd->src_tile->zoom_max; zoom++)
     {
        Grid *g;
        int tnum;

        g = ELM_NEW(Grid);
        g->wsd = sd;
        g->zoom = zoom;
        tnum = pow(2.0, g->zoom);
        g->tw = tnum;
        g->th = tnum;
        g->grid = eina_matrixsparse_new(g->th, g->tw, NULL, NULL);
        sd->grids = eina_list_append(sd->grids, g);
     }
}

static void
_grid_all_clear(Elm_Map_Smart_Data *sd)
{
   Grid *g;

   EINA_SAFETY_ON_NULL_RETURN(sd);

   EINA_LIST_FREE (sd->grids, g)
     {
        Eina_Matrixsparse_Cell *cell;
        Eina_Iterator *it = eina_matrixsparse_iterator_new(g->grid);

        EINA_ITERATOR_FOREACH(it, cell)
          {
             Grid_Item *gi;

             gi = eina_matrixsparse_cell_data_get(cell);
             if (gi) _grid_item_free(gi);
          }
        eina_iterator_free(it);

        eina_matrixsparse_free(g->grid);
        free(g);
     }
}

static void
_track_place(Elm_Map_Smart_Data *sd)
{
#ifdef ELM_EMAP
   Eina_List *l;
   Evas_Coord size;
   Evas_Object *route;
   Evas_Coord px, py, ow, oh;
   int xmin, xmax, ymin, ymax;

   px = sd->pan_x;
   py = sd->pan_y;
   _viewport_coord_get(sd, NULL, NULL, &ow, &oh);

   size = sd->size.w;

   EINA_LIST_FOREACH(sd->track, l, route)
     {
        double lon_min, lon_max;
        double lat_min, lat_max;

        elm_route_longitude_min_max_get(route, &lon_min, &lon_max);
        elm_route_latitude_min_max_get(route, &lat_min, &lat_max);
        _region_to_coord_convert(sd, lon_min, lat_max, size, &xmin, &ymin);
        _region_to_coord_convert(sd, lon_max, lat_min, size, &xmax, &ymax);

        if ( !(xmin < px && xmax < px) && !(xmin > px + ow && xmax > px + ow))
          {
             if ((ymin < py && ymax < py) ||
                 (ymin > py + oh && ymax > py + oh))
               {
                  //display the route
                  evas_object_move(route, xmin - px, ymin - py);
                  evas_object_resize(route, xmax - xmin, ymax - ymin);

                  evas_object_raise(route);
                  _obj_rotate(sd, route);
                  evas_object_show(route);

                  continue;
               }
          }
        //the route is not display
        evas_object_hide(route);
     }
#else
   (void)sd;
#endif
}

static void
_smooth_update(Elm_Map_Smart_Data *sd)
{
   Eina_List *l;
   Grid *g;

   EINA_LIST_FOREACH(sd->grids, l, g)
     {
        Eina_Iterator *it = eina_matrixsparse_iterator_new(g->grid);
        Eina_Matrixsparse_Cell *cell;

        EINA_ITERATOR_FOREACH(it, cell)
          {
             Grid_Item *gi = eina_matrixsparse_cell_data_get(cell);
             if (_grid_item_in_viewport(gi))
               evas_object_image_smooth_scale_set(gi->img, EINA_TRUE);
          }
        eina_iterator_free(it);
     }
}

static Eina_Bool
_zoom_timeout_cb(void *data)
{
   Elm_Map_Smart_Data *sd = data;

   _smooth_update(sd);
   sd->zoom_timer = NULL;
   evas_object_smart_callback_call
     (ELM_WIDGET_DATA(sd)->obj, SIG_ZOOM_STOP, NULL);

   return ECORE_CALLBACK_CANCEL;
}

static void
_zoom(Elm_Map_Smart_Data *sd, double zoom, int animation)
{
   if (zoom > sd->zoom_max) zoom = sd->zoom_max;
   else if (zoom < sd->zoom_min) zoom = sd->zoom_min;

   sd->engine->zoom(ELM_WIDGET_DATA(sd)->obj, zoom, animation);
   sd->zoom_detail = zoom;
   sd->zoom = ROUND(sd->zoom_detail);

   if (sd->zoom_timer) ecore_timer_del(sd->zoom_timer);
   else
     evas_object_smart_callback_call
       (ELM_WIDGET_DATA(sd)->obj, SIG_ZOOM_START, NULL);

   sd->zoom_timer = ecore_timer_add(0.25, _zoom_timeout_cb, sd);
   evas_object_smart_callback_call
     (ELM_WIDGET_DATA(sd)->obj, SIG_ZOOM_CHANGE, NULL);
}

static void
_sizing_eval(Elm_Map_Smart_Data *sd)
{
   Evas_Coord maxw = -1, maxh = -1;

   evas_object_size_hint_max_get(ELM_WIDGET_DATA(sd)->obj, &maxw, &maxh);
   evas_object_size_hint_max_set(ELM_WIDGET_DATA(sd)->obj, maxw, maxh);
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
_scr_timeout_cb(void *data)
{
   Elm_Map_Smart_Data *sd = data;

   _smooth_update(sd);
   sd->scr_timer = NULL;
   evas_object_smart_callback_call
     (ELM_WIDGET_DATA(sd)->obj, SIG_SCROLL_DRAG_STOP, NULL);

   return ECORE_CALLBACK_CANCEL;
}

static void
_scroll_cb(Evas_Object *obj,
           void *data __UNUSED__)
{
   ELM_MAP_DATA_GET(obj, sd);

   if (sd->scr_timer) ecore_timer_del(sd->scr_timer);
   else
     evas_object_smart_callback_call
       (ELM_WIDGET_DATA(sd)->obj, SIG_SCROLL_DRAG_START, NULL);
   sd->scr_timer = ecore_timer_add(0.25, _scr_timeout_cb, sd);
   evas_object_smart_callback_call(ELM_WIDGET_DATA(sd)->obj, SIG_SCROLL, NULL);
}

static void
_scroll_animate_start_cb(Evas_Object *obj,
                         void *data __UNUSED__)
{
   ELM_MAP_DATA_GET(obj, sd);

   evas_object_smart_callback_call
     (ELM_WIDGET_DATA(sd)->obj, SIG_SCROLL_ANIM_START, NULL);
}

static void
_scroll_animate_stop_cb(Evas_Object *obj,
                        void *data __UNUSED__)
{
   ELM_MAP_DATA_GET(obj, sd);

   evas_object_smart_callback_call
     (ELM_WIDGET_DATA(sd)->obj, SIG_SCROLL_ANIM_STOP, NULL);
}

static Eina_Bool
_long_press_cb(void *data)
{
   Elm_Map_Smart_Data *sd = data;

   sd->long_timer = NULL;

   if (sd->on_hold)
     {
        evas_object_smart_callback_call
          (ELM_WIDGET_DATA(sd)->obj, SIG_LONGPRESSED, &sd->ev);
     }

   return ECORE_CALLBACK_CANCEL;
}

static void
_mouse_down_cb(void *data,
               Evas *evas __UNUSED__,
               Evas_Object *obj __UNUSED__,
               void *event_info)
{
   Elm_Map_Smart_Data *sd = data;
   Evas_Event_Mouse_Down *ev = event_info;

   if (ev->button != 1) return;
   //if (ev->event_flags & EVAS_EVENT_FLAG_ON_HOLD) sd->on_hold = EINA_TRUE;
   //else sd->on_hold = EINA_FALSE;
   sd->on_hold = EINA_TRUE;

   if (ev->flags & EVAS_BUTTON_DOUBLE_CLICK)
     evas_object_smart_callback_call
       (ELM_WIDGET_DATA(sd)->obj, SIG_CLICKED_DOUBLE, ev);
   else
     evas_object_smart_callback_call
       (ELM_WIDGET_DATA(sd)->obj, SIG_PRESS, ev);

   if (sd->long_timer) ecore_timer_del(sd->long_timer);
   sd->ev = *ev;
   sd->long_timer =
     ecore_timer_add(_elm_config->longpress_timeout, _long_press_cb, sd);
}

static void
_mouse_up_cb(void *data,
             Evas *evas __UNUSED__,
             Evas_Object *obj __UNUSED__,
             void *event_info)
{
   Elm_Map_Smart_Data *sd = data;
   Evas_Event_Mouse_Up *ev = event_info;

   EINA_SAFETY_ON_NULL_RETURN(ev);

   if (ev->button != 1) return;
   /*
   if (ev->event_flags & EVAS_EVENT_FLAG_ON_HOLD) sd->on_hold = EINA_TRUE;
   else sd->on_hold = EINA_FALSE;
   */

   if (sd->long_timer)
     {
        ecore_timer_del(sd->long_timer);
        sd->long_timer = NULL;
     }

   if (sd->on_hold)
     evas_object_smart_callback_call(ELM_WIDGET_DATA(sd)->obj, SIG_CLICKED, ev);

   if (sd->on_hold && MAP_ENGINE_OVERLAY_SUPPORT(sd))
     {
         Map_Engine_Object *engine_obj = NULL;
         Elm_Map_Overlay *overlay;
         Eina_List *l;

         ELM_MAP_ENG_OBJECT_FIND(sd->engine->object_from_coord, engine_obj, ELM_WIDGET_DATA(sd)->obj, ev->output.x, ev->output.y);

         if (!engine_obj)return;

        EINA_LIST_FOREACH(sd->overlays, l, overlay)
        {
           if ( overlay->engine_obj == engine_obj )
           {
               evas_object_smart_callback_call
                      (ELM_WIDGET_DATA(overlay->wsd)->obj, SIG_OVERLAY_CLICKED, overlay);
               if (overlay->cb)
                  overlay->cb(overlay->cb_data, ELM_WIDGET_DATA(overlay->wsd)->obj, overlay);
               break;
           }
        }
   }

   //sd->on_hold = EINA_FALSE;
}

static void
_mouse_wheel_cb(void *data,
                Evas *e __UNUSED__,
                Evas_Object *obj __UNUSED__,
                void *event_info)
{
   Elm_Map_Smart_Data *sd = data;

   if (!sd->paused)
     {
        Evas_Event_Mouse_Wheel *ev = event_info;

        _zoom(sd, sd->zoom_detail - ((double)ev->z /10), 0);
     }
}

static void
_region_max_min_get(Eina_List *overlays,
                    double *max_longitude,
                    double *min_longitude,
                    double *max_latitude,
                    double *min_latitude)
{
   double max_lon = -180, min_lon = 180;
   double max_lat = -90, min_lat = 90;
   Elm_Map_Overlay *overlay;

   EINA_LIST_FREE(overlays, overlay)
     {
        double lon = -180, lat = -90;

        if (overlay->type == ELM_MAP_OVERLAY_TYPE_DEFAULT)
          {
             Overlay_Default *ovl = overlay->ovl;
             if (ovl->x == -1 || ovl->y == -1)
               continue;

             lon = ovl->lon;
             lat = ovl->lat;
          }
        else if (overlay->type == ELM_MAP_OVERLAY_TYPE_CLASS)
          {
             // FIXME: class center coord is alright??
             Overlay_Class *ovl = overlay->ovl;
             double max_lo, min_lo, max_la, min_la;

             _region_max_min_get
               (ovl->members, &max_lo, &min_lo, &max_la, &min_la);
             lon = (max_lo + min_lo) / 2;
             lat = (max_la + min_la) / 2;
          }
        else if (overlay->type == ELM_MAP_OVERLAY_TYPE_BUBBLE)
          {
             Overlay_Bubble *ovl = overlay->ovl;

             lon = ovl->lon;
             lat = ovl->lat;
          }
        else if (overlay->type == ELM_MAP_OVERLAY_TYPE_POLYLINE)
          {
             Overlay_Polyline*ovl = overlay->ovl;
             Elm_Map_Region *region;
             Eina_List *l;
             EINA_LIST_FOREACH(ovl->regions, l, region)
               {
                  lon = region->longitude;
                  lat = region->latitude;
                  if (lon > max_lon) max_lon = lon;
                  if (lon < min_lon) min_lon = lon;
                  if (lat > max_lat) max_lat = lat;
                  if (lat < min_lat) min_lat = lat;
               }
          }
        else
          {
             WRN("Not supported overlay type: %d", overlay->type);
             continue;
          }
        if (lon > max_lon) max_lon = lon;
        if (lon < min_lon) min_lon = lon;
        if (lat > max_lat) max_lat = lat;
        if (lat < min_lat) min_lat = lat;
     }

   if (max_longitude) *max_longitude = max_lon;
   if (min_longitude) *min_longitude = min_lon;
   if (max_latitude) *max_latitude = max_lat;
   if (min_latitude) *min_latitude = min_lat;
}

static Evas_Object *
_icon_dup(Evas_Object *icon,
          Evas_Object *parent)
{
   Evas_Object *dupp;
   Evas_Coord w, h;

   if (!icon || !parent) return NULL;
   dupp = evas_object_image_filled_add(evas_object_evas_get(parent));
   evas_object_image_source_set(dupp, icon);
   // Set size as origin' sizse for proxy
   evas_object_geometry_get(icon, NULL, NULL, &w, &h);
   if (w <= 0 || h <= 0)
     {
        evas_object_size_hint_min_get(icon, &w, &h);
        evas_object_size_hint_min_set(dupp, w, h);
     }
   else evas_object_resize(dupp, w, h);
   // Original should have size for proxy
   evas_object_resize(icon, w, h);

   return dupp;
}

static void
_overlay_clicked_cb(void *data,
                    Evas *e __UNUSED__,
                    Evas_Object *obj __UNUSED__,
                    void *ev __UNUSED__)
{
   Elm_Map_Overlay *overlay = data;

   EINA_SAFETY_ON_NULL_RETURN(data);

   if (overlay->wsd->on_hold)
     {
        evas_object_smart_callback_call
           (ELM_WIDGET_DATA(overlay->wsd)->obj, SIG_OVERLAY_CLICKED, overlay);
        overlay->wsd->on_hold = EINA_FALSE;
     }

   if (overlay->cb)
     overlay->cb(overlay->cb_data, ELM_WIDGET_DATA(overlay->wsd)->obj, overlay);
}

static void
_overlay_default_hide(Overlay_Default *ovl)
{
   EINA_SAFETY_ON_NULL_RETURN(ovl);

   if (ovl->content) evas_object_hide(ovl->content);
   if (ovl->icon) evas_object_hide(ovl->icon);
   if (ovl->clas_content) evas_object_hide(ovl->clas_content);
   if (ovl->clas_icon) evas_object_hide(ovl->clas_icon);
   if (ovl->layout) evas_object_hide(ovl->layout);
   if (ovl->base->engine_obj) ELM_MAP_ENG_OBJECT_SET(ovl->wsd->engine->object_visibility, ELM_WIDGET_DATA(ovl->wsd)->obj, ovl->base->engine_obj, 0);
}

static void
_overlay_default_show(Overlay_Default *ovl)
{
   Evas_Object *disp = NULL;
   Evas_Coord x, y, w, h;
   Eina_Bool  show_engine_obj = EINA_FALSE;

   EINA_SAFETY_ON_NULL_RETURN(ovl);

   show_engine_obj = (ovl->base->engine_obj && NULL == ovl->base->grp->klass) ? EINA_TRUE : EINA_FALSE;

   if (ovl->x == -1 || ovl->y == -1)
    {
        _overlay_default_hide(ovl);
        return;
    }

   evas_object_hide(ovl->layout);
   if (ovl->content)
    {
       if (EINA_FALSE == show_engine_obj)
       {
           disp = ovl->content;
           evas_object_geometry_get(disp, NULL, NULL, &w, &h);
           if (w <= 0 || h <= 0) evas_object_size_hint_min_get(disp, &w, &h);
           ovl->w = w;
           ovl->h = h;
       }
    }
    else if (!(ovl->icon) && ovl->clas_content)
       {
          disp = ovl->clas_content;
          evas_object_geometry_get(disp, NULL, NULL, &w, &h);
          if (w <= 0 || h <= 0) evas_object_size_hint_min_get(disp, &w, &h);
       }
    else if (EINA_FALSE == show_engine_obj)
       {
          if (ovl->icon)
            evas_object_show(ovl->icon);
          else if (ovl->clas_icon)
            evas_object_show(ovl->clas_icon);
          disp = ovl->layout;
          w = ovl->w;
          h = ovl->h;
       }
   if (show_engine_obj)//to display engine object the overlay must NOT belong to class
     {
        ELM_MAP_ENG_OBJECT_SET(ovl->wsd->engine->object_visibility, ELM_WIDGET_DATA(ovl->wsd)->obj, ovl->base->engine_obj, 1);
        if (ovl->content) evas_object_hide(ovl->content);
    }

    if (disp && !show_engine_obj)
     {
       _coord_to_canvas(ovl->wsd, ovl->x, ovl->y, &x, &y);
       _obj_place(disp, x - (w / 2), y - (h / 2), w, h);
     }
}

static void
_overlay_default_coord_get(Overlay_Default *ovl,
                           Evas_Coord *x,
                           Evas_Coord *y,
                           Evas_Coord *w,
                           Evas_Coord *h)
{
   EINA_SAFETY_ON_NULL_RETURN(ovl);

   if (x) *x = ovl->x;
   if (y) *y = ovl->y;
   if (w) *w = ovl->w;
   if (h) *h = ovl->h;
}

static void
_overlay_default_coord_set(Overlay_Default *ovl,
                           Evas_Coord x,
                           Evas_Coord y)
{
   EINA_SAFETY_ON_NULL_RETURN(ovl);

   ovl->x = x;
   ovl->y = y;
}

static void
_overlay_default_coord_update(Overlay_Default *ovl)
{
   EINA_SAFETY_ON_NULL_RETURN(ovl);

   _region_to_coord_convert
     (ovl->wsd, ovl->lon, ovl->lat, ovl->wsd->size.w, &ovl->x, &ovl->y);
}

static void
_overlay_default_layout_text_update(Overlay_Default *ovl,
                                    const char *text)
{
   if (!ovl->content && !ovl->icon && !ovl->clas_content && !ovl->clas_icon)
     elm_layout_text_set(ovl->layout, "elm.text", text);
}

static void
_overlay_default_content_update(Overlay_Default *ovl,
                                Evas_Object *content,
                                Elm_Map_Overlay *overlay)
{
   EINA_SAFETY_ON_NULL_RETURN(ovl);

   if (ovl->content == content) return;
   if (ovl->content) evas_object_del(ovl->content);
   ovl->content = content;

   if (IS_EXTERNAL_ENGINE(ovl->wsd))
     evas_object_smart_member_add(ovl->content, ELM_WIDGET_DATA(ovl->wsd)->obj);
   else
     evas_object_smart_member_add(ovl->content, ovl->wsd->pan_obj);

   evas_object_stack_above(ovl->content, ovl->wsd->sep_maps_overlays);

   if (ovl->content)
     evas_object_event_callback_add(ovl->content, EVAS_CALLBACK_MOUSE_UP,
                                    _overlay_clicked_cb, overlay);
}

static void
_overlay_default_layout_update(Overlay_Default *ovl)
{
   EINA_SAFETY_ON_NULL_RETURN(ovl);

   if (ovl->icon)
     {
        evas_object_color_set(ovl->layout, 255, 255, 255, 255);
        if (!IS_EXTERNAL_ENGINE(ovl->wsd))
          {
             elm_layout_theme_set
             (ovl->layout, "map/marker", "empty",
             elm_widget_style_get(ELM_WIDGET_DATA(ovl->wsd)->obj));
             elm_object_part_content_set(ovl->layout, "elm.icon", ovl->icon);
          }
     }
   else if (!ovl->icon && ovl->clas_icon)
     {
        evas_object_color_set(ovl->layout, 255, 255, 255, 255);
        if (!IS_EXTERNAL_ENGINE(ovl->wsd))
          {
             elm_layout_theme_set
             (ovl->layout, "map/marker", "empty",
             elm_widget_style_get(ELM_WIDGET_DATA(ovl->wsd)->obj));
             elm_object_part_content_set(ovl->layout, "elm.icon", ovl->clas_icon);
          }
     }
   else
     {
        if (IS_EXTERNAL_ENGINE(ovl->wsd))
          {
             if (ovl->content && !ovl->base->engine_obj)
               evas_object_color_set(ovl->content, ovl->c.r, ovl->c.g, ovl->c.b, ovl->c.a);
          }
        else
          {
             evas_object_color_set(ovl->layout, ovl->c.r, ovl->c.g, ovl->c.b, ovl->c.a);
             elm_layout_theme_set(ovl->layout, "map/marker", "radio", elm_widget_style_get(ELM_WIDGET_DATA(ovl->wsd)->obj));
          }
     }
}

static void
_overlay_default_class_content_update(Overlay_Default *ovl,
                                      Evas_Object *content)
{
   EINA_SAFETY_ON_NULL_RETURN(ovl);

   if (ovl->clas_content) evas_object_del(ovl->clas_content);
   ovl->clas_content = _icon_dup(content, ovl->layout);

   if (IS_EXTERNAL_ENGINE(ovl->wsd))
     evas_object_smart_member_add(ovl->clas_content, ELM_WIDGET_DATA(ovl->wsd)->obj);
   else
     evas_object_smart_member_add(ovl->clas_content, ovl->wsd->pan_obj);

   evas_object_stack_above(ovl->clas_content, ovl->wsd->sep_maps_overlays);
   _overlay_default_layout_update(ovl);
}

static Evas_Object*
_img_from_icon(Evas *e __UNUSED__, Evas_Object *icon)
{
   char *icon_file = NULL;
   Evas_Object *img = NULL;
   img = elm_image_object_get(icon);
   if ( EVAS_LOAD_ERROR_NONE != evas_object_image_load_error_get(img) )
    {
        ERR("Failed to load image file : %s", icon_file);
        evas_object_del(img);
        return NULL;
    }
    return img;
}

static void
_overlay_engine_icon_create(Elm_Map_Overlay *overlay,
                            Evas_Object *icon)
{
   char *imgbuffer = NULL;
   int w = 0;
   int h = 0;
   Overlay_Default *ovl = (Overlay_Default*)(overlay->ovl);

   Evas_Object *img = _img_from_icon(evas_object_evas_get(ELM_WIDGET_DATA(overlay->wsd)->obj), icon);

   EINA_SAFETY_ON_NULL_RETURN(img);

   imgbuffer = (char*)evas_object_image_data_get(img, EINA_FALSE);

   EINA_SAFETY_ON_NULL_RETURN(imgbuffer);

   evas_object_image_size_get(img, &w, &h);

   if (overlay->engine_obj)
      ELM_MAP_ENG_OBJECT_DELETE(overlay->wsd->engine->icon_del,  ELM_WIDGET_DATA(overlay->wsd)->obj, overlay->engine_obj);

   ELM_MAP_ENG_OBJECT_CREATE(overlay->wsd->engine->icon_add, overlay->engine_obj,  ELM_WIDGET_DATA(overlay->wsd)->obj, imgbuffer, w, h, ovl->lon, ovl->lat);

   if (ovl->layout)
      evas_object_hide(ovl->layout);//we do not show the evas object
}

static void
_overlay_default_engine_icon_create_or_update(Elm_Map_Overlay *overlay,
                             Evas_Object *icon)
{
   Overlay_Default *ovl = (Overlay_Default*)(overlay->ovl);

   if (ovl->icon == icon )
      return;

   _overlay_engine_icon_create(overlay, icon);

   if ( ovl->icon )
      evas_object_del(icon);
   ovl->icon = icon;
}

static void
_overlay_default_engine_content_create_or_update(Elm_Map_Overlay *overlay,
                             Evas_Object *content)
{
   Overlay_Default *ovl = (Overlay_Default*)(overlay->ovl);
   Eina_Bool img_or_icon = evas_object_smart_type_check(content, "elm_icon") |
   evas_object_smart_type_check(content, "elm_image");

   if (ovl->content == content )
      return;

   if ( ovl->content )
      evas_object_del(ovl->content);
   ovl->content = NULL;
   if (img_or_icon)
    {
       _overlay_engine_icon_create(overlay, content);
       if (!overlay->engine_obj)
         _overlay_default_content_update(overlay->ovl, content, overlay);
       else
         ovl->content = content;
    }
   else
    {
       _overlay_default_content_update(overlay->ovl, content, overlay);
    }
}


static void
_overlay_default_icon_update(Overlay_Default *ovl,
                             Evas_Object *icon)
{
   EINA_SAFETY_ON_NULL_RETURN(ovl);
   Elm_Map_Overlay *overlay = ovl->base;

   if (ovl->icon == icon) return;

   if (MAP_ENGINE_OVERLAY_SUPPORT(overlay->wsd))
    {
       _overlay_default_engine_icon_create_or_update(overlay, icon);
       if (!overlay->engine_obj)
         _overlay_default_layout_update(ovl);
    }
   else
    {
       if (ovl->icon) evas_object_del(ovl->icon);
           ovl->icon = icon;
       _overlay_default_layout_update(ovl);
    }
}

static void
_overlay_default_class_icon_update(Overlay_Default *ovl,
                                   Evas_Object *icon)
{
   EINA_SAFETY_ON_NULL_RETURN(ovl);

   if (ovl->clas_icon) evas_object_del(ovl->clas_icon);
   ovl->clas_icon = _icon_dup(icon, ovl->layout);
   _overlay_default_layout_update(ovl);
}

static void
_overlay_default_color_update(Overlay_Default *ovl,
                              Color c)
{
   EINA_SAFETY_ON_NULL_RETURN(ovl);

   ovl->c = c;
   _overlay_default_layout_update(ovl);
}

static void
_overlay_default_free(Overlay_Default *ovl)
{
   EINA_SAFETY_ON_NULL_RETURN(ovl);

   if (ovl->content) evas_object_del(ovl->content);
   if (ovl->icon) evas_object_del(ovl->icon);
   if (ovl->clas_content) evas_object_del(ovl->clas_content);
   if (ovl->clas_icon) evas_object_del(ovl->clas_icon);
   if (ovl->layout) evas_object_del(ovl->layout);

   free(ovl);
}

static Overlay_Default *
_overlay_default_new(Elm_Map_Overlay *overlay,
                     double lon,
                     double lat,
                     Color c,
                     double scale)
{
   Overlay_Default *ovl;

   EINA_SAFETY_ON_NULL_RETURN_VAL(overlay, NULL);

   ovl = ELM_NEW(Overlay_Default);
   ovl->wsd = overlay->wsd;
   _edj_overlay_size_get(ovl->wsd, &(ovl->w), &(ovl->h));
   ovl->w *= scale;
   ovl->h *= scale;
   ovl->layout = elm_layout_add(ELM_WIDGET_DATA(ovl->wsd)->obj);

   if (IS_EXTERNAL_ENGINE(ovl->wsd))
      evas_object_smart_member_add(ovl->layout, ELM_WIDGET_DATA(ovl->wsd)->obj);
   else
      evas_object_smart_member_add(ovl->layout, ovl->wsd->pan_obj);

   evas_object_stack_above(ovl->layout, ovl->wsd->sep_maps_overlays);
   if (!IS_EXTERNAL_ENGINE(ovl->wsd))
     {
        elm_layout_theme_set(ovl->layout, "map/marker", "radio",
                          elm_widget_style_get(ELM_WIDGET_DATA(ovl->wsd)->obj));

        evas_object_event_callback_add(ovl->layout, EVAS_CALLBACK_MOUSE_DOWN,
                                  _overlay_clicked_cb, overlay);
     }

   ovl->lon = lon;
   ovl->lat = lat;
   ovl->base = overlay;
   _overlay_default_color_update(ovl, c);

   return ovl;
}

static void
_overlay_group_hide(Overlay_Group *grp)
{
   EINA_SAFETY_ON_NULL_RETURN(grp);

   if (grp->ovl) _overlay_default_hide(grp->ovl);
}

static void
_overlay_group_show(Overlay_Group *grp)
{
   EINA_SAFETY_ON_NULL_RETURN(grp);

   if (grp->ovl) _overlay_default_show(grp->ovl);
}

static void
_overlay_group_coord_member_update(Overlay_Group *grp,
                                   Evas_Coord x,
                                   Evas_Coord y,
                                   Eina_List *members)
{
   char text[32];

   EINA_SAFETY_ON_NULL_RETURN(grp);

   if (!grp->ovl) return;

   _overlay_default_coord_set(grp->ovl, x, y);
   _coord_to_region_convert
     (grp->wsd, x, y, grp->wsd->size.w, &grp->lon, &grp->lat);

   if (grp->members) eina_list_free(grp->members);
   grp->members = members;
   snprintf(text, sizeof(text), "%d", eina_list_count(members));

   _overlay_default_layout_text_update(grp->ovl, text);
}

static void
_overlay_group_icon_update(Overlay_Group *grp,
                           Evas_Object *icon)
{
   EINA_SAFETY_ON_NULL_RETURN(grp);

   if (grp->ovl)
     _overlay_default_icon_update
       (grp->ovl, _icon_dup(icon, ELM_WIDGET_DATA(grp->wsd)->obj));
}

static void
_overlay_group_content_update(Overlay_Group *grp,
                              Evas_Object *content,
                              Elm_Map_Overlay *overlay)
{
   EINA_SAFETY_ON_NULL_RETURN(grp);

   _overlay_default_content_update
      (grp->ovl, _icon_dup(content, ELM_WIDGET_DATA(grp->wsd)->obj), overlay);

   return;
}

static void
_overlay_group_color_update(Overlay_Group *grp,
                            Color c)
{
   EINA_SAFETY_ON_NULL_RETURN(grp);

   _overlay_default_color_update(grp->ovl, c);
}

static void
_overlay_group_cb_set(Overlay_Group *grp,
                      Elm_Map_Overlay_Get_Cb cb,
                      void *data)
{
   EINA_SAFETY_ON_NULL_RETURN(grp);

   grp->overlay->cb = cb;
   grp->overlay->data = data;
}

static void
_overlay_group_free(Overlay_Group *grp)
{
   EINA_SAFETY_ON_NULL_RETURN(grp);

   if (grp->overlay) free(grp->overlay);
   if (grp->ovl) _overlay_default_free(grp->ovl);
   if (grp->members) eina_list_free(grp->members);

   free(grp);
}

static Overlay_Group *
_overlay_group_new(Elm_Map_Smart_Data *sd)
{
   Overlay_Group *grp;
   Color c = {0x90, 0xee, 0x90, 0xff};

   grp = ELM_NEW(Overlay_Group);
   grp->wsd = sd;
   grp->overlay = ELM_NEW(Elm_Map_Overlay);  // this is a virtual overlay
   grp->overlay->wsd = sd;
   grp->overlay->type = ELM_MAP_OVERLAY_TYPE_GROUP;
   grp->overlay->ovl = grp;
   grp->overlay->engine_obj = NULL;
   grp->ovl = _overlay_default_new(grp->overlay, -1, -1, c, 1);

   return grp;
}

static void
_overlay_class_cb_set(Overlay_Class *ovl,
                      Elm_Map_Overlay_Get_Cb cb,
                      void *data)
{
   Eina_List *l;
   Elm_Map_Overlay *overlay;

   EINA_SAFETY_ON_NULL_RETURN(ovl);

   // Update class members' callbacks
   EINA_LIST_FOREACH(ovl->members, l, overlay)
     _overlay_group_cb_set(overlay->grp, cb, data);
}

static void
_overlay_class_icon_update(Overlay_Class *ovl,
                           Evas_Object *icon)
{
   Eina_List *l;
   Elm_Map_Overlay *overlay;

   EINA_SAFETY_ON_NULL_RETURN(ovl);

   if (ovl->icon == icon) return;
   if (ovl->icon) evas_object_del(ovl->icon);
   ovl->icon = icon;
   // For using proxy, it should have size and be shown but moved away to hide.
   evas_object_resize(icon, 32, 32);
   evas_object_move(icon, -9999, -9999);
   evas_object_show(icon);

   // Update class members' class icons
   EINA_LIST_FOREACH(ovl->members, l, overlay)
     {
        _overlay_group_icon_update(overlay->grp, icon);

        if (overlay->type == ELM_MAP_OVERLAY_TYPE_DEFAULT)
          _overlay_default_class_icon_update(overlay->ovl, icon);
     }
}

static void
_overlay_class_content_update(Overlay_Class *ovl,
                              Evas_Object *content)
{
   Eina_List *l;
   Elm_Map_Overlay *overlay;

   EINA_SAFETY_ON_NULL_RETURN(ovl);

   if (ovl->content == content) return;
   if (ovl->content) evas_object_del(ovl->content);
   ovl->content = content;
   // For using proxy, it should have size and be shown but moved away to hide.
   // content should have it's own size
   evas_object_move(content, -9999, -9999);

   // Update class members' class contents
   EINA_LIST_FOREACH(ovl->members, l, overlay)
     {
        _overlay_group_content_update(overlay->grp, content, overlay);

        if (overlay->type == ELM_MAP_OVERLAY_TYPE_DEFAULT)
          _overlay_default_class_content_update(overlay->ovl, content);
     }
}

static void
_overlay_class_color_update(Overlay_Class *ovl,
                            Color c)
{
   Eina_List *l;
   Elm_Map_Overlay *overlay;

   EINA_SAFETY_ON_NULL_RETURN(ovl);

   // Update class members' class contents
   EINA_LIST_FOREACH(ovl->members, l, overlay)
     _overlay_group_color_update(overlay->grp, c);
}

static void
_overlay_class_free(Overlay_Class *clas)
{
   Eina_List *l;
   Elm_Map_Overlay *overlay;

   EINA_SAFETY_ON_NULL_RETURN(clas);

   // Update class members' class contents
   EINA_LIST_FOREACH(clas->members, l, overlay)
     {
        overlay->grp->klass = NULL;
        _overlay_group_content_update(overlay->grp, NULL, NULL);
        _overlay_group_icon_update(overlay->grp, NULL);

        if (overlay->type == ELM_MAP_OVERLAY_TYPE_DEFAULT)
          {
             _overlay_default_class_content_update(overlay->ovl, NULL);
             _overlay_default_class_icon_update(overlay->ovl, NULL);
          }
     }
   if (clas->icon) evas_object_del(clas->icon);
   if (clas->members) eina_list_free(clas->members);

   free(clas);
}

static Overlay_Class *
_overlay_class_new(Elm_Map_Smart_Data *sd)
{
   Overlay_Class *ovl;

   ovl = ELM_NEW(Overlay_Class);
   ovl->wsd = sd;
   ovl->icon = NULL;
   ovl->zoom_max = OVERLAY_CLASS_ZOOM_MAX;

   return ovl;
}

static void
_overlay_bubble_coord_update(Overlay_Bubble *bubble)
{
   EINA_SAFETY_ON_NULL_RETURN(bubble);

   if (bubble->pobj)
     {
        Evas_Coord x, y, w, h;

        evas_object_geometry_get(bubble->pobj, &x, &y, &w, &h);
        bubble->x = x + (w / 2);
        bubble->y = y - (bubble->h / 2);
        _canvas_to_coord
          (bubble->wsd, bubble->x, bubble->y, &(bubble->x), &(bubble->y));
        _coord_to_region_convert
          (bubble->wsd, bubble->x, bubble->y, bubble->wsd->size.w,
          &(bubble->lon), &(bubble->lat));
     }
   else
     {
        _region_to_coord_convert(bubble->wsd, bubble->lon, bubble->lat,
                                 bubble->wsd->size.w, &bubble->x, &bubble->y);
     }
}

static void
_overlay_bubble_coord_get(Overlay_Bubble *bubble,
                          Evas_Coord *x,
                          Evas_Coord *y,
                          Evas_Coord *w,
                          Evas_Coord *h)
{
   EINA_SAFETY_ON_NULL_RETURN(bubble);

   if (!(bubble->pobj))
     {
        if (x) *x = bubble->x;
        if (y) *y = bubble->y;
        if (w) *w = bubble->w;
        if (h) *h = bubble->h;
     }
   else
     {
        if (x) *x = 0;
        if (y) *y = 0;
        if (w) *w = 0;
        if (h) *h = 0;
     }
}

static Eina_Bool
_overlay_bubble_show_hide(Overlay_Bubble *bubble,
                          Eina_Bool visible)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(bubble, EINA_FALSE);
   EINA_SAFETY_ON_NULL_RETURN_VAL(bubble->obj, EINA_FALSE);

   if (!visible) evas_object_hide(bubble->obj);
   else if (bubble->pobj && !evas_object_visible_get(bubble->pobj))
     {
        evas_object_hide(bubble->obj);
        visible = EINA_FALSE;
     }
   else
     {
        _coord_to_canvas
          (bubble->wsd, bubble->x, bubble->y, &(bubble->x), &(bubble->y));
        _obj_place(bubble->obj, bubble->x - (bubble->w / 2),
                   bubble->y - (bubble->h / 2), bubble->w, bubble->h);
        evas_object_raise(bubble->obj);
     }
   return visible;
}

static void
_overlay_bubble_free(Overlay_Bubble *bubble)
{
   EINA_SAFETY_ON_NULL_RETURN(bubble);

   evas_object_del(bubble->bx);
   evas_object_del(bubble->sc);
   evas_object_del(bubble->obj);

   free(bubble);
}

static Overlay_Bubble *
_overlay_bubble_new(Elm_Map_Overlay *overlay)
{
   Evas_Coord h;
   const char *s;
   Overlay_Bubble *bubble;

   EINA_SAFETY_ON_NULL_RETURN_VAL(overlay, NULL);

   bubble = ELM_NEW(Overlay_Bubble);
   bubble->wsd = overlay->wsd;

   bubble->obj =
     edje_object_add(evas_object_evas_get(ELM_WIDGET_DATA(overlay->wsd)->obj));
   elm_widget_theme_object_set
     (ELM_WIDGET_DATA(overlay->wsd)->obj, bubble->obj, "map", "marker_bubble",
     elm_widget_style_get(ELM_WIDGET_DATA(overlay->wsd)->obj));

   evas_object_event_callback_add(bubble->obj, EVAS_CALLBACK_MOUSE_DOWN,
                                  _overlay_clicked_cb, overlay);

   // FIXME: different from upstream.
   // elm_widget_sub_object_add will fail if edje is used as parent object.
   bubble->sc = elm_scroller_add(ELM_WIDGET_DATA(overlay->wsd)->obj);
   elm_widget_style_set(bubble->sc, "map_bubble");
   elm_scroller_content_min_limit(bubble->sc, EINA_FALSE, EINA_TRUE);
   elm_scroller_policy_set
     (bubble->sc, ELM_SCROLLER_POLICY_AUTO, ELM_SCROLLER_POLICY_OFF);
   elm_scroller_bounce_set
     (bubble->sc, _elm_config->thumbscroll_bounce_enable, EINA_FALSE);
   edje_object_part_swallow(bubble->obj, "elm.swallow.content", bubble->sc);

   bubble->bx = elm_box_add(bubble->sc);
   evas_object_size_hint_align_set(bubble->bx, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_size_hint_weight_set
     (bubble->bx, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_box_horizontal_set(bubble->bx, EINA_TRUE);
   elm_object_content_set(bubble->sc, bubble->bx);

   s = edje_object_data_get(bubble->obj, "size_w");
   if (s) bubble->w = atoi(s);
   else bubble->w = 0;

   edje_object_size_min_calc(bubble->obj, NULL, &(bubble->h));
   s = edje_object_data_get(bubble->obj, "size_h");
   if (s) h = atoi(s);
   else h = 0;

   if (bubble->h < h) bubble->h = h;

   bubble->lon = -1;
   bubble->lat = -1;
   bubble->x = -1;
   bubble->y = -1;

   return bubble;
}

static void
_overlay_route_color_update(Overlay_Route *ovl,
                            Color c)
{
   EINA_SAFETY_ON_NULL_RETURN(ovl);

   evas_object_color_set(ovl->obj, c.r, c.g, c.b, c.a);
}

static void
_overlay_route_hide(Overlay_Route *ovl)
{
   EINA_SAFETY_ON_NULL_RETURN(ovl);

   evas_object_hide(ovl->obj);
}

static void
_overlay_route_show(Overlay_Route *r)
{
   Path *p;
   Eina_List *l;
   Path_Node *n;

   EINA_SAFETY_ON_NULL_RETURN(r);
   EINA_SAFETY_ON_NULL_RETURN(r->wsd);

   evas_object_polygon_points_clear(r->obj);
   EINA_LIST_FOREACH(r->nodes, l, n)
     {
        p = eina_list_nth(r->paths, n->idx);
        if (!p) continue;

        _region_to_coord_convert
          (r->wsd, n->pos.lon, n->pos.lat, r->wsd->size.w, &p->x, &p->y);
        _coord_to_canvas(r->wsd, p->x, p->y, &p->x, &p->y);
     }
   EINA_LIST_FOREACH(r->paths, l, p)
     evas_object_polygon_point_add(r->obj, p->x - 3, p->y - 3);

   EINA_LIST_REVERSE_FOREACH(r->paths, l, p)
     evas_object_polygon_point_add(r->obj, p->x + 3, p->y + 3);

   evas_object_show(r->obj);
}

static void
_overlay_route_free(Overlay_Route *route)
{
   Path *p;
   Path_Node *n;

   EINA_SAFETY_ON_NULL_RETURN(route);

   evas_object_del(route->obj);

   EINA_LIST_FREE (route->paths, p)
     free(p);

   EINA_LIST_FREE (route->nodes, n)
     {
        if (n->pos.address) eina_stringshare_del(n->pos.address);
        free(n);
     }

   free(route);
}

static Overlay_Route *
_overlay_route_new(Elm_Map_Smart_Data *sd,
                   const Elm_Map_Route *route,
                   Color c)
{
   Eina_List *l;
   Path_Node *n;
   Overlay_Route *ovl;

   EINA_SAFETY_ON_NULL_RETURN_VAL(route, NULL);

   ovl = ELM_NEW(Overlay_Route);
   ovl->wsd = sd;

   ovl->obj = evas_object_polygon_add(evas_object_evas_get(ELM_WIDGET_DATA(sd)->obj));

   if (IS_EXTERNAL_ENGINE(ovl->wsd))
     evas_object_smart_member_add(ovl->obj, ELM_WIDGET_DATA(ovl->wsd)->obj);
   else
     evas_object_smart_member_add(ovl->obj, sd->pan_obj);

   _overlay_route_color_update(ovl, c);

   EINA_LIST_FOREACH(route->nodes, l, n)
   {
      Path *path;
      Path_Node *node;

      node = ELM_NEW(Path_Node);
      node->idx = n->idx;
      node->pos.lon = n->pos.lon;
      node->pos.lat = n->pos.lat;
      if (n->pos.address) node->pos.address = strdup(n->pos.address);
         ovl->nodes = eina_list_append(ovl->nodes, node);

      path = ELM_NEW(Path);
      ovl->paths = eina_list_append(ovl->paths, path);
   }

   return ovl;
}

static void
_overlay_line_color_update(Overlay_Line *ovl,
                           Color c)
{
   EINA_SAFETY_ON_NULL_RETURN(ovl);

   evas_object_color_set(ovl->obj, c.r, c.g, c.b, c.a);
}

static void
_overlay_line_hide(Overlay_Line *ovl)
{
   EINA_SAFETY_ON_NULL_RETURN(ovl);

   if (ovl->obj) evas_object_hide(ovl->obj);
}

static void
_overlay_line_show(Overlay_Line *ovl)
{
   Evas_Coord fx, fy, tx, ty;
   Elm_Map_Smart_Data *sd = ovl->wsd;

   _region_to_coord_convert(sd, ovl->flon, ovl->flat, sd->size.w, &fx, &fy);
   _region_to_coord_convert(sd, ovl->tlon, ovl->tlat, sd->size.w, &tx, &ty);
   _coord_to_canvas(sd, fx, fy, &fx, &fy);
   _coord_to_canvas(sd, tx, ty, &tx, &ty);
   evas_object_line_xy_set(ovl->obj, fx, fy, tx, ty);
   evas_object_show(ovl->obj);
}

static void
_overlay_line_free(Overlay_Line *ovl)
{
   EINA_SAFETY_ON_NULL_RETURN(ovl);

   evas_object_del(ovl->obj);
   free(ovl);
}

static Overlay_Line *
_overlay_line_new(Elm_Map_Smart_Data *sd,
                  double flon,
                  double flat,
                  double tlon,
                  double tlat,
                  Color c)
{
   Overlay_Line *ovl;

   ovl = ELM_NEW(Overlay_Line);
   ovl->wsd = sd;
   ovl->flon = flon;
   ovl->flat = flat;
   ovl->tlon = tlon;
   ovl->tlat = tlat;
   ovl->obj =
     evas_object_line_add(evas_object_evas_get(ELM_WIDGET_DATA(sd)->obj));

   if (IS_EXTERNAL_ENGINE(ovl->wsd))
     evas_object_smart_member_add(ovl->obj, ELM_WIDGET_DATA(ovl->wsd)->obj);
   else
     evas_object_smart_member_add(ovl->obj, sd->pan_obj);

   _overlay_line_color_update(ovl, c);

   return ovl;
}

static void
_overlay_polyline_color_update(Overlay_Polyline *ovl, Color c)
{
   EINA_SAFETY_ON_NULL_RETURN(ovl);
   Eina_List *l;
   Evas_Object *segment = NULL;

   EINA_LIST_FOREACH(ovl->segments, l, segment)
   evas_object_color_set(segment, c.r, c.g, c.b, c.a);
}

static void
_overlay_polyline_hide(Overlay_Polyline *ovl)
{
   EINA_SAFETY_ON_NULL_RETURN(ovl);
   Eina_List *l;
   Evas_Object *segment = NULL;

   EINA_LIST_FOREACH(ovl->segments, l, segment)
   if (segment) evas_object_hide(segment);
}

static void
_overlay_polyline_show(Overlay_Polyline *ovl)
{
   EINA_SAFETY_ON_NULL_RETURN(ovl);
   Eina_List *region_list = NULL;
   Eina_List *segment_list = NULL;
   Eina_List *temp_list = NULL;
   Elm_Map_Region *start = NULL;
   Elm_Map_Region *end = NULL;
   Elm_Map_Smart_Data *sd;
   Evas_Coord fx, fy, tx, ty;
   Evas_Object *segment = NULL;

   sd = ovl->wsd;

   region_list = ovl->regions;
   segment_list = ovl->segments;

   while (region_list)
     {
        start = eina_list_data_get(region_list);
        temp_list = eina_list_next(region_list);
        if (temp_list)
          end = eina_list_data_get(temp_list);
        else
	   break;

        _region_to_coord_convert(sd, start->longitude, start->latitude, sd->size.w, &fx, &fy);
        _region_to_coord_convert(sd, end->longitude, end->latitude, sd->size.w, &tx, &ty);
        _coord_to_canvas(sd, fx, fy, &fx, &fy);
        _coord_to_canvas(sd, tx, ty, &tx, &ty);

        segment =  eina_list_data_get(segment_list);
        if (segment)
          {
             evas_object_line_xy_set(segment, fx, fy, tx, ty);
             evas_object_show(segment);
             segment_list = eina_list_next(segment_list);
          }
        region_list = eina_list_next(region_list);
      }
}

static void
_overlay_polyline_free(Overlay_Polyline *ovl)
{
   Evas_Object *segment = NULL;
   Elm_Map_Region *region = NULL;

   EINA_SAFETY_ON_NULL_RETURN(ovl);

   EINA_LIST_FREE (ovl->segments, segment)
   evas_object_del(segment);

   EINA_LIST_FREE (ovl->regions, region)
   free(region);

   free(ovl);
}

static Overlay_Polyline *
_overlay_polyline_new(Elm_Map_Smart_Data *sd, Color c __UNUSED__)
{
   Overlay_Polyline *ovl;

   ovl = ELM_NEW(Overlay_Polyline);
   ovl->wsd = sd;
   ovl->regions = NULL;
   ovl->base = NULL;
   ovl->width = 1;

   return ovl;
}

static void
_overlay_polyline_region_add(Overlay_Polyline *ovl)
{
   EINA_SAFETY_ON_NULL_RETURN(ovl);
   Eina_List *region_list = NULL;
   Elm_Map_Smart_Data *sd;
   Evas_Object *segment = NULL;

   EINA_SAFETY_ON_NULL_RETURN(ovl);
   sd = ovl->wsd;

   region_list = ovl->regions;
   unsigned int total_count = eina_list_count(region_list);
   if (total_count < 2)
     {
        return;
     }
   else
     {
        segment = evas_object_line_add(evas_object_evas_get(ELM_WIDGET_DATA(ovl->wsd)->obj));
	 evas_object_smart_member_add(segment, sd->pan_obj);
        evas_object_color_set(segment, ovl->base->c.r, ovl->base->c.g, ovl->base->c.b, ovl->base->c.a);
	 ovl->segments =  eina_list_append(ovl->segments, segment);
     }
}

static void
_overlay_polygon_color_update(Overlay_Polygon *ovl,
                              Color c)
{
   EINA_SAFETY_ON_NULL_RETURN(ovl);

   evas_object_color_set(ovl->obj, c.r, c.g, c.b, c.a);
}

static void
_overlay_polygon_hide(Overlay_Polygon *ovl)
{
   EINA_SAFETY_ON_NULL_RETURN(ovl);

   if (ovl->obj) evas_object_hide(ovl->obj);
}

static void
_overlay_polygon_show(Overlay_Polygon *ovl)
{
   Eina_List *l;
   Elm_Map_Region *region;
   Elm_Map_Smart_Data *sd;

   EINA_SAFETY_ON_NULL_RETURN(ovl);

   sd = ovl->wsd;

   evas_object_polygon_points_clear(ovl->obj);
   EINA_LIST_FOREACH(ovl->regions, l, region)
     {
        Evas_Coord x, y;

        _region_to_coord_convert(sd, region->longitude, region->latitude, sd->size.w, &x, &y);
        _coord_to_canvas(sd, x, y, &x, &y);
        evas_object_polygon_point_add(ovl->obj, x, y);
     }
   evas_object_show(ovl->obj);
}

static void
_overlay_polygon_free(Overlay_Polygon *ovl)
{
   Elm_Map_Region *region;

   EINA_SAFETY_ON_NULL_RETURN(ovl);

   evas_object_del(ovl->obj);

   EINA_LIST_FREE (ovl->regions, region)
   free(region);

   free(ovl);
}

static Overlay_Polygon *
_overlay_polygon_new(Elm_Map_Smart_Data *sd, Color c)
{
   Overlay_Polygon *ovl;

   ovl = ELM_NEW(Overlay_Polygon);
   ovl->wsd = sd;
   ovl->obj =
     evas_object_polygon_add(evas_object_evas_get(ELM_WIDGET_DATA(sd)->obj));

   if (IS_EXTERNAL_ENGINE(ovl->wsd))
     evas_object_smart_member_add(ovl->obj, ELM_WIDGET_DATA(ovl->wsd)->obj);
   else
     evas_object_smart_member_add(ovl->obj, sd->pan_obj);

   _overlay_polygon_color_update(ovl, c);

   return ovl;
}

static void
_overlay_circle_color_update(Overlay_Circle *ovl,
                             Color c)
{
   Evas_Object *obj;

   EINA_SAFETY_ON_NULL_RETURN(ovl);

   obj = elm_layout_edje_get(ovl->obj);
   evas_object_color_set(obj, c.r, c.g, c.b, c.a);
}

static void
_overlay_circle_hide(Overlay_Circle *ovl)
{
   EINA_SAFETY_ON_NULL_RETURN(ovl);

   if (ovl->obj) evas_object_hide(ovl->obj);
}

static void
_overlay_circle_show(Overlay_Circle *ovl)
{
   double r;
   Evas_Coord x, y;
   Elm_Map_Smart_Data *sd;

   EINA_SAFETY_ON_NULL_RETURN(ovl);

   sd = ovl->wsd;

   r = (ovl->ratio) * sd->size.w;

   if (IS_EXTERNAL_ENGINE(ovl->wsd))
     {
        double meters_per_screen_pixel = -1;

        if (sd->engine->scale_get)//value returned by this function will be in meters/pixel
            meters_per_screen_pixel = sd->engine->scale_get(ELM_WIDGET_DATA(sd)->obj, ovl->lon, ovl->lat);

        if ((int)meters_per_screen_pixel != -1)
            r = ovl->radius / meters_per_screen_pixel;
     }
   _region_to_coord_convert(sd, ovl->lon, ovl->lat, sd->size.w, &x, &y);
   _coord_to_canvas(sd, x, y, &x, &y);
   _obj_place(ovl->obj, x - r, y - r, r * 2, r * 2);
}

static void
_overlay_circle_free(Overlay_Circle *ovl)
{
   EINA_SAFETY_ON_NULL_RETURN(ovl);

   evas_object_del(ovl->obj);
   free(ovl);
}

static Overlay_Circle *
_overlay_circle_new(Elm_Map_Smart_Data *sd,
                    double lon,
                    double lat,
                    double radius,
                    Color c)
{
   Overlay_Circle *ovl;

   ovl = ELM_NEW(Overlay_Circle);
   ovl->wsd = sd;
   ovl->lon = lon;
   ovl->lat = lat;
   ovl->radius = radius;
   ovl->ratio = radius / sd->size.w;

   ovl->obj = elm_layout_add(ELM_WIDGET_DATA(sd)->obj);

   if (IS_EXTERNAL_ENGINE(ovl->wsd))
     evas_object_smart_member_add(ovl->obj, ELM_WIDGET_DATA(ovl->wsd)->obj);
   else
     evas_object_smart_member_add(ovl->obj, sd->pan_obj);

   evas_object_stack_above(ovl->obj, sd->sep_maps_overlays);
   elm_layout_theme_set(ovl->obj, "map/circle", "base",
                        elm_widget_style_get(ELM_WIDGET_DATA(sd)->obj));

   _overlay_circle_color_update(ovl, c);

   return ovl;
}

static void
_overlay_scale_color_update(Overlay_Scale *ovl,
                            Color c)
{
   EINA_SAFETY_ON_NULL_RETURN(ovl);

   evas_object_color_set(ovl->obj, c.r, c.g, c.b, c.a);
}

static void
_overlay_scale_hide(Overlay_Scale *ovl)
{
   EINA_SAFETY_ON_NULL_RETURN(ovl);

   if (ovl->obj) evas_object_hide(ovl->obj);
}

static void
_overlay_scale_show(Overlay_Scale *ovl)
{
   double text;
   char buf[32];
   double meter;
   double lon, lat;
   Elm_Map_Smart_Data *sd;
   Evas_Coord text_width = 0, scale_width = 0,total_width = 0;
   Evas_Coord minw = -1, minh = -1;

   EINA_SAFETY_ON_NULL_RETURN(ovl);

   sd = ovl->wsd;

   if ((int)(sizeof(_scale_tb)/sizeof(double)) <= sd->zoom)
     {
        ERR("Zoom level is too high");
        return;
     }

   elm_map_region_get(ELM_WIDGET_DATA(sd)->obj, &lon, &lat);

   if (IS_EXTERNAL_ENGINE(ovl->wsd))
     meter = sd->engine->scale_get(ELM_WIDGET_DATA(sd)->obj, lon, lat);
   else
     meter = sd->src_tile->scale_cb(ELM_WIDGET_DATA(sd)->obj, lon, lat, sd->zoom);

   if (meter <= 0)
     {
        ERR("Scale callback returns a value below zero");
        return;
     }

   if (IS_EXTERNAL_ENGINE(ovl->wsd))
     scale_width = (_scale_tb[sd->zoom] / meter);
   else
     scale_width = (_scale_tb[sd->zoom] / meter) * (sd->zoom_detail - sd->zoom + 1);

   text = _scale_tb[sd->zoom] / 1000;
   if (text < 1) snprintf(buf, sizeof(buf), "%d m", (int)(text * 1000));
   else snprintf(buf, sizeof(buf), "%d km", (int)text);

   elm_layout_text_set(ovl->obj, "elm.text", buf);
   edje_object_size_min_calc(elm_layout_edje_get(ovl->obj), &minw, &minh);
   edje_object_part_geometry_get(elm_layout_edje_get(ovl->obj), "elm.text", NULL, NULL, &text_width, NULL);
   total_width = scale_width + text_width + ovl->padding_w;

   _obj_place(ovl->obj, ovl->x, ovl->y, total_width, ovl->h);
}

static void
_overlay_scale_free(Overlay_Scale *ovl)
{
   EINA_SAFETY_ON_NULL_RETURN(ovl);

   evas_object_del(ovl->obj);
   free(ovl);
}

static Overlay_Scale *
_overlay_scale_new(Elm_Map_Smart_Data *sd,
                   Evas_Coord x,
                   Evas_Coord y,
                   Color c)
{
   const char *s;
   Overlay_Scale *ovl;

   ovl = ELM_NEW(Overlay_Scale);
   ovl->wsd = sd;
   ovl->x = x;
   ovl->y = y;

   ovl->obj = elm_layout_add(ELM_WIDGET_DATA(sd)->obj);

   if (IS_EXTERNAL_ENGINE(ovl->wsd))
     evas_object_smart_member_add(ovl->obj, ELM_WIDGET_DATA(ovl->wsd)->obj);
   else
     evas_object_smart_member_add(ovl->obj, sd->pan_obj);

   evas_object_stack_above(ovl->obj, sd->sep_maps_overlays);
   elm_layout_theme_set(ovl->obj, "map/scale", "base",
                        elm_widget_style_get(ELM_WIDGET_DATA(sd)->obj));
   s = edje_object_data_get(elm_layout_edje_get(ovl->obj), "size_w");

   if (s) ovl->w = atoi(s);
   else ovl->w = 100;

   s = edje_object_data_get(elm_layout_edje_get(ovl->obj), "size_h");
   if (s) ovl->h = atoi(s);
   else ovl->h = 60;

   s = edje_object_data_get(elm_layout_edje_get(ovl->obj), "padding_w");
   if (s) ovl->padding_w = atoi(s);
   else ovl->padding_w = 10;

   _overlay_scale_color_update(ovl, c);

   return ovl;
}

static void
_overlay_grouping(Eina_List *clas_membs,
                  Elm_Map_Overlay *boss)
{
   Eina_List *l;
   Elm_Map_Overlay *memb;
   Eina_List *grp_membs = NULL;
   int sum_x = 0, sum_y = 0, cnt = 0;
   Evas_Coord bx = 0, by = 0, bw = 0, bh = 0;

   EINA_SAFETY_ON_NULL_RETURN(clas_membs);
   EINA_SAFETY_ON_NULL_RETURN(boss);

   if (boss->type == ELM_MAP_OVERLAY_TYPE_DEFAULT)
     _overlay_default_coord_get(boss->ovl, &bx, &by, &bw, &bh);
   else if (boss->type == ELM_MAP_OVERLAY_TYPE_BUBBLE)
     _overlay_bubble_coord_get(boss->ovl, &bx, &by, &bw, &bh);

   EINA_LIST_FOREACH(clas_membs, l, memb)
     {
        Evas_Coord x = 0, y = 0, w = 0, h = 0;

        if (boss == memb || memb->grp->in) continue;
        if ((memb->hide) || (memb->zoom_min > memb->wsd->zoom)) continue;

        if (memb->type == ELM_MAP_OVERLAY_TYPE_DEFAULT)
          _overlay_default_coord_get(memb->ovl, &x, &y, &w, &h);
        else if (memb->type == ELM_MAP_OVERLAY_TYPE_BUBBLE)
          _overlay_bubble_coord_get(memb->ovl, &x, &y, &w, &h);

        if (bw <= 0 || bh <= 0 || w <= 0 || h <= 0) continue;
        if (ELM_RECTS_INTERSECT(x, y, w, h, bx, by,
                                bw * OVERLAY_GROUPING_SCALE,
                                bh * OVERLAY_GROUPING_SCALE))
          {
             // Join group.
             memb->grp->boss = EINA_FALSE;
             memb->grp->in = EINA_TRUE;
             sum_x += x;
             sum_y += y;
             cnt++;
             grp_membs = eina_list_append(grp_membs, memb);
          }
     }

   if (cnt >= 1)
     {
        // Mark as boss
        boss->grp->boss = EINA_TRUE;
        boss->grp->in = EINA_TRUE;
        sum_x = (sum_x + bx) / (cnt + 1);
        sum_y = (sum_y + by) / (cnt + 1);
        grp_membs = eina_list_append(grp_membs, boss);
        _overlay_group_coord_member_update(boss->grp, sum_x, sum_y, grp_membs);

        // Append group to all overlay list
        boss->wsd->group_overlays =
          eina_list_append(boss->wsd->group_overlays, boss->grp->overlay);
     }
}

static void
_overlay_show(Elm_Map_Overlay *overlay)
{
   Elm_Map_Smart_Data *sd = overlay->wsd;

   if (overlay->paused) return;
   if (overlay->ovl == NULL) return;
   if ((overlay->grp) && (overlay->grp->klass) && (overlay->grp->klass->paused))
     return;

   overlay->visible = EINA_TRUE;
   if (overlay->type == ELM_MAP_OVERLAY_TYPE_CLASS)
     {
        overlay->visible = EINA_FALSE;
        return;
     }
   if (overlay->grp)
     {
        if ((overlay->grp->in) ||
            (overlay->hide) || (overlay->zoom_min > sd->zoom))
          overlay->visible = EINA_FALSE;

        if ((overlay->grp->klass) &&
            ((overlay->grp->klass->hide) ||
             (overlay->grp->klass->zoom_min > sd->zoom)))
          overlay->visible = EINA_FALSE;
     }

   switch (overlay->type)
     {
      case ELM_MAP_OVERLAY_TYPE_DEFAULT:
        if (overlay->visible) _overlay_default_show(overlay->ovl);
        else _overlay_default_hide(overlay->ovl);
        break;

      case ELM_MAP_OVERLAY_TYPE_GROUP:
        if (overlay->visible) _overlay_group_show(overlay->ovl);
        else _overlay_group_hide(overlay->ovl);
        break;

      case ELM_MAP_OVERLAY_TYPE_BUBBLE:
        overlay->visible =
          _overlay_bubble_show_hide(overlay->ovl, overlay->visible);
        break;

      case ELM_MAP_OVERLAY_TYPE_ROUTE:
        if (overlay->visible) _overlay_route_show(overlay->ovl);
        else _overlay_route_hide(overlay->ovl);
        break;

      case ELM_MAP_OVERLAY_TYPE_LINE:
        if (overlay->visible) _overlay_line_show(overlay->ovl);
        else _overlay_line_hide(overlay->ovl);
        break;

      case ELM_MAP_OVERLAY_TYPE_POLYLINE:
        if (overlay->visible)
	   {
	      if (IS_EXTERNAL_ENGINE(sd))
               {
                  if (sd->engine->polyline_show)
                    sd->engine->polyline_show(ELM_WIDGET_DATA(overlay->wsd)->obj, overlay->engine_obj);
               }
             else
               _overlay_polyline_show(overlay->ovl);
          }
        else _overlay_polyline_hide(overlay->ovl);
        break;

      case ELM_MAP_OVERLAY_TYPE_POLYGON:
        if (overlay->visible) _overlay_polygon_show(overlay->ovl);
        else _overlay_polygon_hide(overlay->ovl);
        break;

      case ELM_MAP_OVERLAY_TYPE_CIRCLE:
        if (overlay->visible) _overlay_circle_show(overlay->ovl);
        else _overlay_circle_hide(overlay->ovl);
        break;

      case ELM_MAP_OVERLAY_TYPE_SCALE:
        if (overlay->visible) _overlay_scale_show(overlay->ovl);
        else _overlay_scale_hide(overlay->ovl);
        break;

      default:
        ERR("Invalid overlay type to show: %d", overlay->type);
     }
}

static void
_overlay_place(Elm_Map_Smart_Data *sd)
{
   Eina_List *l, *ll;
   Elm_Map_Overlay *overlay;

   eina_list_free(sd->group_overlays);
   sd->group_overlays = NULL;

   DBG("_overlay_place entered!!");

   EINA_LIST_FOREACH(sd->overlays, l, overlay)
     {
        if (overlay->ovl == NULL || overlay->grp == NULL)
	    continue;
        // Reset groups
        if (overlay->type == ELM_MAP_OVERLAY_TYPE_CLASS) continue;
        overlay->grp->in = EINA_FALSE;
        overlay->grp->boss = EINA_FALSE;
        _overlay_group_hide(overlay->grp);

        // Update overlays' coord
        if (overlay->type == ELM_MAP_OVERLAY_TYPE_DEFAULT)
          _overlay_default_coord_update(overlay->ovl);
        else if (overlay->type == ELM_MAP_OVERLAY_TYPE_BUBBLE)
          _overlay_bubble_coord_update(overlay->ovl);
     }

   // Classify into group boss or follwer
   EINA_LIST_FOREACH(sd->overlays, l, overlay)
     {
        Elm_Map_Overlay *boss;
        Overlay_Class *clas;

        if (overlay->ovl == NULL || overlay->grp == NULL)
	    continue;

        if (overlay->type != ELM_MAP_OVERLAY_TYPE_CLASS) continue;
        if (overlay->hide || (overlay->zoom_min > sd->zoom)) continue;

        clas = overlay->ovl;
        if (clas->zoom_max < sd->zoom) continue;
        EINA_LIST_FOREACH(clas->members, ll, boss)
          {
             if (boss->type == ELM_MAP_OVERLAY_TYPE_CLASS) continue;
             if (boss->hide || (boss->zoom_min > sd->zoom)) continue;
             if (boss->grp->in) continue;
             _overlay_grouping(clas->members, boss);
          }
     }

   // Place group overlays and overlays
   EINA_LIST_FOREACH(sd->group_overlays, l, overlay)
     _overlay_show(overlay);
   EINA_LIST_FOREACH(sd->overlays, l, overlay)
     _overlay_show(overlay);
}

static Evas_Object *
_overlay_obj_get(const Elm_Map_Overlay *overlay)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(overlay, NULL);

   if (overlay->type == ELM_MAP_OVERLAY_TYPE_GROUP)
     {
        Overlay_Group *ovl = overlay->ovl;
        Overlay_Default *df = ovl->ovl;

        return df->layout;
     }
   else if (overlay->type == ELM_MAP_OVERLAY_TYPE_DEFAULT)
     {
        Overlay_Default *ovl = overlay->ovl;

        return ovl->layout;
     }
   else
     {
        ERR("Not supported overlay type: %d", overlay->type);
        return NULL;
     }
}

static Eina_Bool
_xml_name_attrs_dump_cb(void *data,
                        const char *key,
                        const char *value)
{
   Name_Dump *dump = (Name_Dump *)data;

   EINA_SAFETY_ON_NULL_RETURN_VAL(dump, EINA_FALSE);
   EINA_SAFETY_ON_NULL_RETURN_VAL(key, EINA_FALSE);
   EINA_SAFETY_ON_NULL_RETURN_VAL(value, EINA_FALSE);

   if (!strncmp(key, NOMINATIM_ATTR_LON, sizeof(NOMINATIM_ATTR_LON)))
     dump->lon = _elm_atof(value);
   else if (!strncmp(key, NOMINATIM_ATTR_LAT, sizeof(NOMINATIM_ATTR_LAT)))
     dump->lat = _elm_atof(value);
   else if (!strncmp(key, NOMINATIM_ATTR_ADDRESS, sizeof(NOMINATIM_ATTR_ADDRESS)))
     {
        if (!dump->address)
          dump->address = strdup(value);
     }

   return EINA_TRUE;
}

static Eina_Bool
_xml_route_dump_cb(void *data,
                   Eina_Simple_XML_Type type,
                   const char *value,
                   unsigned offset __UNUSED__,
                   unsigned length)
{
   Route_Dump *dump = data;

   EINA_SAFETY_ON_NULL_RETURN_VAL(dump, EINA_FALSE);
   EINA_SAFETY_ON_NULL_RETURN_VAL(value, EINA_FALSE);

   switch (type)
     {
      case EINA_SIMPLE_XML_OPEN:
      case EINA_SIMPLE_XML_OPEN_EMPTY:
      {
         const char *attrs;

         attrs = eina_simple_xml_tag_attributes_find(value, length);
         if (!attrs)
           {
              if (!strncmp(value, YOURS_DISTANCE, length))
                dump->id = ROUTE_XML_DISTANCE;
              else if (!strncmp(value, YOURS_DESCRIPTION, length))
                dump->id = ROUTE_XML_DESCRIPTION;
              else if (!strncmp(value, YOURS_COORDINATES, length))
                dump->id = ROUTE_XML_COORDINATES;
              else dump->id = ROUTE_XML_NONE;
           }
      }
      break;

      case EINA_SIMPLE_XML_DATA:
      {
         char *buf = malloc(length);

         if (!buf) return EINA_FALSE;

         snprintf(buf, length, "%s", value);
         if (dump->id == ROUTE_XML_DISTANCE) dump->distance = _elm_atof(buf);
         else if (!(dump->description) && (dump->id == ROUTE_XML_DESCRIPTION))
           dump->description = strdup(buf);
         else if (dump->id == ROUTE_XML_COORDINATES)
           dump->coordinates = strdup(buf);

         free(buf);
      }
      break;

      default:
        break;
     }

   return EINA_TRUE;
}

static Eina_Bool
_xml_name_dump_cb(void *data,
                  Eina_Simple_XML_Type type,
                  const char *value,
                  unsigned offset __UNUSED__,
                  unsigned length)
{
   Name_Dump *dump = data;

   EINA_SAFETY_ON_NULL_RETURN_VAL(dump, EINA_FALSE);
   EINA_SAFETY_ON_NULL_RETURN_VAL(value, EINA_FALSE);

   switch (type)
     {
      case EINA_SIMPLE_XML_OPEN:
      case EINA_SIMPLE_XML_OPEN_EMPTY:
      {
         const char *attrs;
         attrs = eina_simple_xml_tag_attributes_find(value, length);
         if (attrs)
           {
              if (!strncmp(value, NOMINATIM_RESULT,
                           sizeof(NOMINATIM_RESULT) - 1))
                dump->id = NAME_XML_NAME;
              else dump->id = NAME_XML_NONE;

              eina_simple_xml_attributes_parse
                (attrs, length - (attrs - value), _xml_name_attrs_dump_cb,
                dump);
           }
      }
      break;

      case EINA_SIMPLE_XML_DATA:
      {
         char *buf = malloc(length + 1);

         if (!buf) return EINA_FALSE;
         snprintf(buf, length + 1, "%s", value);
         if (dump->id == NAME_XML_NAME) dump->address = strdup(buf);
         free(buf);
      }
      break;

      default:
        break;
     }

   return EINA_TRUE;
}

static Eina_Bool
_xml_name_dump_list_cb(void *data,
                       Eina_Simple_XML_Type type,
                       const char *value,
                       unsigned offset,
                       unsigned length)
{
   Elm_Map_Name_List *name_list = data;
   Elm_Map_Name *name;
   Name_Dump dump = {0, NULL, 0.0, 0.0};
   _xml_name_dump_cb(&dump, type, value, offset, length);
   name = calloc(1, sizeof(Elm_Map_Name));
   if (!name) return EINA_FALSE;
   if (dump.address)
     {
        name->address = strdup(dump.address);
        name->lon = dump.lon;
        name->lat = dump.lat;
        name->wsd = name_list->wsd;
        name_list->names = eina_list_append(name_list->names, name);
        name->wsd->names = eina_list_append(name->wsd->names, name);
     }
  else
    free(name);

   return EINA_TRUE;
}


static void
_kml_parse(Elm_Map_Route *r)
{
   FILE *f;
   char **str;
   double lon, lat;
   unsigned int ele, idx;

   EINA_SAFETY_ON_NULL_RETURN(r);
   EINA_SAFETY_ON_NULL_RETURN(r->fname);

   Route_Dump dump = {0, r->fname, 0.0, NULL, NULL};

   f = fopen(r->fname, "rb");
   if (f)
     {
        long sz;

        fseek(f, 0, SEEK_END);
        sz = ftell(f);
        if (sz > 0)
          {
             char *buf = malloc(sz + 1);
             if (buf)
               {
                  memset(buf, 0, sz + 1);
                  rewind(f);
                  if (fread(buf, 1, sz, f))
                    {
                       eina_simple_xml_parse
                         (buf, sz, EINA_TRUE, _xml_route_dump_cb, &dump);
                    }
                  else
                    free(buf);
               }
          }
        fclose(f);

        if (dump.distance) r->info.distance = dump.distance;
        if (dump.description)
          {
             eina_stringshare_replace(&r->info.waypoints, dump.description);
             str = eina_str_split_full(dump.description, "\n", 0, &ele);
             r->info.waypoint_count = ele;
             for (idx = 0; idx < ele; idx++)
               {
                  Path_Waypoint *wp = ELM_NEW(Path_Waypoint);

                  if (wp)
                    {
                       wp->wsd = r->wsd;
                       wp->point = eina_stringshare_add(str[idx]);
                       DBG("%s", str[idx]);
                       r->waypoint = eina_list_append(r->waypoint, wp);
                    }
               }
             if (str && str[0])
               {
                  free(str[0]);
                  free(str);
               }
          }
        else WRN("description is not found !");

        if (dump.coordinates)
          {
             eina_stringshare_replace(&r->info.nodes, dump.coordinates);
             str = eina_str_split_full(dump.coordinates, "\n", 0, &ele);
             r->info.node_count = ele;
             for (idx = 0; idx < ele; idx++)
               {
                  Path_Node *n = ELM_NEW(Path_Node);

                  sscanf(str[idx], "%lf,%lf", &lon, &lat);
                  if (n)
                    {
                       n->wsd = r->wsd;
                       n->pos.lon = lon;
                       n->pos.lat = lat;
                       n->idx = idx;
                       DBG("%lf:%lf", lon, lat);
                       n->pos.address = NULL;
                       r->nodes = eina_list_append(r->nodes, n);
                    }
               }
             if (str && str[0])
               {
                  free(str[0]);
                  free(str);
               }
          }
     }
}

static void
_name_parse(Elm_Map_Name *n)
{
   FILE *f;

   EINA_SAFETY_ON_NULL_RETURN(n);
   EINA_SAFETY_ON_NULL_RETURN(n->fname);

   Name_Dump dump = {0, NULL, 0.0, 0.0};

   f = fopen(n->fname, "rb");
   if (f)
     {
        long sz;

        fseek(f, 0, SEEK_END);
        sz = ftell(f);
        if (sz > 0)
          {
             char *buf = malloc(sz + 1);
             if (buf)
               {
                  memset(buf, 0, sz + 1);
                  rewind(f);
                  if (fread(buf, 1, sz, f))
                    {
                       eina_simple_xml_parse
                         (buf, sz, EINA_TRUE, _xml_name_dump_cb, &dump);
                    }
                  free(buf);
               }
          }
        fclose(f);

        if (dump.address)
          {
             INF("[%lf : %lf] ADDRESS : %s", n->lon, n->lat, dump.address);
             n->address = strdup(dump.address);
          }
        n->lon = dump.lon;
        n->lat = dump.lat;
     }
}

static void
_name_list_parse(Elm_Map_Name_List *nl)
{
   FILE *f;
   EINA_SAFETY_ON_NULL_RETURN(nl);
   EINA_SAFETY_ON_NULL_RETURN(nl->fname);

   f = fopen(nl->fname, "rb");
   if (f)
     {
        long sz;

        fseek(f, 0, SEEK_END);
        sz = ftell(f);
        if (sz > 0)
          {
             char *buf = malloc(sz + 1);
             if (buf)
               {
                  memset(buf, 0, sz + 1);
                  rewind(f);
                  if (fread(buf, 1, sz, f))
                    {
                       eina_simple_xml_parse
                         (buf, sz, EINA_TRUE, _xml_name_dump_list_cb, nl);
                    }
                  else
                    free(buf);
               }
          }
        fclose(f);
     }
}

static void
_route_cb(void *data,
          const char *file,
          int status)
{
   Elm_Map_Route *route;
   Elm_Map_Smart_Data *sd;

   EINA_SAFETY_ON_NULL_RETURN(data);
   EINA_SAFETY_ON_NULL_RETURN(file);

   route = data;
   sd = route->wsd;

   route->job = NULL;
   if (status == 200)
     {
        _kml_parse(route);
        INF("Route request success from (%lf, %lf) to (%lf, %lf)",
            route->flon, route->flat, route->tlon, route->tlat);
        if (route->cb) route->cb(route->data, ELM_WIDGET_DATA(sd)->obj, route);
        evas_object_smart_callback_call
          (ELM_WIDGET_DATA(sd)->obj, SIG_ROUTE_LOADED, NULL);
     }
   else
     {
        ERR("Route request failed: %d", status);
        if (route->cb) route->cb(route->data, ELM_WIDGET_DATA(sd)->obj, NULL);
        evas_object_smart_callback_call
          (ELM_WIDGET_DATA(sd)->obj, SIG_ROUTE_LOADED_FAIL, NULL);
     }

   edje_object_signal_emit(ELM_WIDGET_DATA(sd)->resize_obj,
                           "elm,state,busy,stop", "elm");
}

static void
_name_cb(void *data,
         const char *file,
         int status)
{
   Elm_Map_Name *name;
   Elm_Map_Smart_Data *sd;

   EINA_SAFETY_ON_NULL_RETURN(data);
   EINA_SAFETY_ON_NULL_RETURN(file);

   name = data;
   sd = name->wsd;

   name->job = NULL;
   if (status == 200)
     {
        _name_parse(name);
        INF("Name request success address:%s, lon:%lf, lat:%lf",
            name->address, name->lon, name->lat);
        if (name->cb) name->cb(name->data, ELM_WIDGET_DATA(sd)->obj, name);
        evas_object_smart_callback_call
          (ELM_WIDGET_DATA(sd)->obj, SIG_NAME_LOADED, NULL);
     }
   else
     {
        ERR("Name request failed: %d", status);
        if (name->cb) name->cb(name->data, ELM_WIDGET_DATA(sd)->obj, NULL);
        evas_object_smart_callback_call
          (ELM_WIDGET_DATA(sd)->obj, SIG_NAME_LOADED_FAIL, NULL);
     }
   edje_object_signal_emit(ELM_WIDGET_DATA(sd)->resize_obj,
                           "elm,state,busy,stop", "elm");
}

static void
_name_list_cb(void *data,
              const char *file,
              int status)
{
   Elm_Map_Name_List *name_list;
   Elm_Map_Smart_Data *sd;

   EINA_SAFETY_ON_NULL_RETURN(data);
   EINA_SAFETY_ON_NULL_RETURN(file);

   name_list = data;
   sd = name_list->wsd;

   name_list->job = NULL;
   if (status == 200)
     {
        _name_list_parse(name_list);
        INF("Name List request success address");
        if (name_list->cb)
          name_list->cb(name_list->data, ELM_WIDGET_DATA(sd)->obj,
                        name_list->names);
        evas_object_smart_callback_call
          (ELM_WIDGET_DATA(sd)->obj, SIG_NAME_LOADED, NULL);
     }
   else
     {
        ERR("Name List request failed: %d", status);
        if (name_list->cb)
          name_list->cb(name_list->data, ELM_WIDGET_DATA(sd)->obj, NULL);
        evas_object_smart_callback_call
          (ELM_WIDGET_DATA(sd)->obj, SIG_NAME_LOADED_FAIL, NULL);
     }
   edje_object_signal_emit(ELM_WIDGET_DATA(sd)->resize_obj,
                           "elm,state,busy,stop", "elm");
   free(name_list->fname);
   free(name_list);
}


static char *
_prepare_download()
{
   char fname[PATH_MAX];
     {
        const char *cachedir;

#ifdef ELM_EFREET
        snprintf(fname, sizeof(fname), "%s" CACHE_NAME_ROOT,
                 efreet_cache_home_get());
        (void)cachedir;
#else
        cachedir = getenv("XDG_CACHE_HOME");
        snprintf(fname, sizeof(fname), "%s/%s" CACHE_NAME_ROOT, getenv("HOME"),
                 cachedir ? : "/.config");
#endif
        if (!ecore_file_exists(fname)) ecore_file_mkpath(fname);
     }
   return strdup(fname);
}


static Elm_Map_Name *
_name_request(const Evas_Object *obj,
              int method,
              const char *address,
              double lon,
              double lat,
              Elm_Map_Name_Cb name_cb,
              void *data)
{
   char *url;
   Elm_Map_Name *name;
   char *fname, fname2[PATH_MAX];

   ELM_MAP_DATA_GET(obj, sd);
   EINA_SAFETY_ON_NULL_RETURN_VAL(sd->src_name, NULL);

   fname = _prepare_download();
   url = sd->src_name->url_cb
   (ELM_WIDGET_DATA(sd)->obj, method, address, lon, lat);
   if (!url)
     {
        ERR("Name URL is NULL");
        free(fname);
        return NULL;
     }

   name = ELM_NEW(Elm_Map_Name);
   name->wsd = sd;
   snprintf(fname2, sizeof(fname2), "%s/%d", fname, rand());
   name->fname = strdup(fname2);
   name->method = method;
   if (method == ELM_MAP_NAME_METHOD_SEARCH) name->address = strdup(address);
   else if (method == ELM_MAP_NAME_METHOD_REVERSE)
     {
        name->lon = lon;
        name->lat = lat;
     }
   name->cb = name_cb;
   name->data = data;

   if (!ecore_file_download_full(url, name->fname, _name_cb, NULL, name,
                                 &(name->job), sd->ua) || !(name->job))
     {
        ERR("Can't request Name from %s to %s", url, name->fname);
        if (name->address) free(name->address);
        free(name->fname);
        free(name);
        free(fname);
        return NULL;
     }
   INF("Name requested from %s to %s", url, name->fname);
   free(url);
   free(fname);

   sd->names = eina_list_append(sd->names, name);
   evas_object_smart_callback_call
     (ELM_WIDGET_DATA(sd)->obj, SIG_NAME_LOAD, name);
   edje_object_signal_emit(ELM_WIDGET_DATA(sd)->resize_obj,
                           "elm,state,busy,start", "elm");
   return name;
}


static void
_name_list_request(const Evas_Object *obj,
              int method,
              const char *address,
              double lon,
              double lat,
              Elm_Map_Name_List_Cb name_cb,
              void *data)
{
   char *url;
   Elm_Map_Name_List *name_list;
   char *fname, fname2[PATH_MAX];

   ELM_MAP_DATA_GET(obj, sd);
   EINA_SAFETY_ON_NULL_RETURN(sd->src_name);

   fname = _prepare_download();
   url = sd->src_name->url_cb
   (ELM_WIDGET_DATA(sd)->obj, method, address, lon, lat);
   if (!url)
     {
        ERR("Name URL is NULL");
        free(fname);
        return;
     }
   name_list = ELM_NEW(Elm_Map_Name_List);
   name_list->wsd = sd;
   snprintf(fname2, sizeof(fname2), "%s/%d", fname, rand());
   name_list->fname = strdup(fname2);
   name_list->cb = name_cb;
   name_list->data = data;
   if (!ecore_file_download_full(url, name_list->fname, _name_list_cb,
                                 NULL, name_list,
                                 &(name_list->job), sd->ua) || !(name_list->job))
     {
        ERR("Can't request Name from %s to %s", url, name_list->fname);
        free(name_list->fname);
        free(name_list);
        free(fname);
        free(url);
        return;
     }
   INF("Name requested from %s to %s", url, name_list->fname);
   free(url);
   free(fname);

   evas_object_smart_callback_call
     (ELM_WIDGET_DATA(sd)->obj, SIG_NAME_LOAD, name_list);
   edje_object_signal_emit(ELM_WIDGET_DATA(sd)->resize_obj,
                           "elm,state,busy,start", "elm");
}


static Evas_Event_Flags
_pinch_zoom_start_cb(void *data,
                     void *event_info __UNUSED__)
{
   Elm_Map_Smart_Data *sd = data;
   EINA_SAFETY_ON_NULL_RETURN_VAL(data, EVAS_EVENT_FLAG_NONE);
   if (!sd->pinch_zoom_enabled) return  EVAS_EVENT_FLAG_NONE;
   if (sd->paused) return EVAS_EVENT_FLAG_NONE;

   sd->on_hold = EINA_FALSE;
   sd->pinch_zoom = sd->zoom_detail;

   if (sd->long_timer)
     {
        ecore_timer_del(sd->long_timer);
        sd->long_timer = NULL;
     }
   return EVAS_EVENT_FLAG_NONE;
}

static Evas_Event_Flags
_pinch_zoom_cb(void *data,
               void *event_info)
{
   Elm_Map_Smart_Data *sd = data;
   double zoom_level = 0;
   EINA_SAFETY_ON_NULL_RETURN_VAL(sd, EVAS_EVENT_FLAG_NONE);
   if (!sd->pinch_zoom_enabled) return  EVAS_EVENT_FLAG_NONE;
   if (sd->paused) return EVAS_EVENT_FLAG_NONE;

   Elm_Gesture_Zoom_Info *ei = event_info;

   if (ei->zoom >= 1) zoom_level = sd->pinch_zoom + ei->zoom - 1;
   else zoom_level = sd->pinch_zoom - (1 / ei->zoom) + 1;

   _zoom(sd, zoom_level, 0);

   if (sd->long_timer)
     {
        ecore_timer_del(sd->long_timer);
        sd->long_timer = NULL;
     }
   return EVAS_EVENT_FLAG_NONE;
}

#if 0 // TIZEN: Remove pinch rotate feature
static Evas_Event_Flags
_pinch_rotate_cb(void *data,
                 void *event_info)
{
   Elm_Map_Smart_Data *sd = data;
   EINA_SAFETY_ON_NULL_RETURN_VAL(sd, EVAS_EVENT_FLAG_NONE);
   if (sd->paused) return EVAS_EVENT_FLAG_NONE;

   int x, y, w, h;
   Elm_Gesture_Rotate_Info *ei = event_info;

   if (!sd->pinch_rotate.a)
     {
        double diff = ei->angle - ei->base_angle;
        if (diff < 0) diff = -diff;
        if (diff >= ROTATE_THRESH)
          {
             sd->pinch_rotate.a = ei->angle - ei->base_angle;
          }
     }
   if (sd->pinch_rotate.a)
     {
        evas_object_geometry_get(ELM_WIDGET_DATA(sd)->obj, &x, &y, &w, &h);
        x = x + ((double)w * 0.5);
        y = y + ((double)h * -1.5);
        sd->pinch_rotate.d = (ei->angle - ei->base_angle) - sd->pinch_rotate.a;
        sd->engine->rotate(ELM_WIDGET_DATA(sd)->obj, sd->pinch_rotate.d, x, y, 0);
     }

   _overlay_place(sd);
   if (sd->long_timer)
     {
        ecore_timer_del(sd->long_timer);
        sd->long_timer = NULL;
     }
   return EVAS_EVENT_FLAG_NONE;
}

static Evas_Event_Flags
_pinch_rotate_start_cb(void *data,
                     void *event_info __UNUSED__)
{
   Elm_Map_Smart_Data *sd = data;
   EINA_SAFETY_ON_NULL_RETURN_VAL(sd, EVAS_EVENT_FLAG_NONE);

   sd->pinch_rotate.a = 0;

   return EVAS_EVENT_FLAG_NONE;
}
#endif

static Evas_Event_Flags
_pinch_momentum_start_cb(void *data,
                         void *ei)
{
   Elm_Map_Smart_Data *sd = data;
   EINA_SAFETY_ON_NULL_RETURN_VAL(sd, EVAS_EVENT_FLAG_NONE);
   Elm_Gesture_Momentum_Info *mi = ei;

   if (IS_EXTERNAL_ENGINE(sd) &&
       ((mi->n == 1) || (mi->n == 2 && sd->perspective_enabled == EINA_TRUE)))
     {
        sd->scroll_pan.start_x = mi->x1;
        sd->scroll_pan.start_y = mi->y1;
        sd->scroll_pan.x = mi->x1;
        sd->scroll_pan.y = mi->y1;
        sd->scroll_pan.is_scroll_start = EINA_FALSE;
        _overlay_place(sd);
     }
   return EVAS_EVENT_FLAG_NONE;
}

static Evas_Event_Flags
_pinch_momentum_move_cb(void *data,
                        void *ei)
{
   Elm_Map_Smart_Data *sd = data;
   EINA_SAFETY_ON_NULL_RETURN_VAL(sd, EVAS_EVENT_FLAG_NONE);
   Elm_Gesture_Momentum_Info *mi = ei;
   Evas_Coord xdiff = 0, ydiff = 0, fw = 0, fh = 0;
   static Evas_Coord pan_x_diff = 0, pan_y_diff = 0;

   if (IS_EXTERNAL_ENGINE(sd))
     {
        if (mi->n == 1)
          {
             if (!sd->scroll_pan.is_scroll_start)
               {
                  pan_x_diff = mi->x2 - sd->scroll_pan.start_x;
                  pan_y_diff = mi->y2 - sd->scroll_pan.start_y;
	              if (abs(pan_x_diff) >= scroll_threshold || abs(pan_y_diff) >= scroll_threshold)
                    {
                       evas_object_smart_callback_call(ELM_WIDGET_DATA(sd)->obj, SIG_SCROLL_DRAG_START, NULL);
                       sd->scroll_pan.is_scroll_start = EINA_TRUE;
                    }
               }
             if (sd->scroll_pan.is_scroll_start)
               {
                  sd->on_hold = EINA_FALSE;
                  sd->engine->pan(ELM_WIDGET_DATA(sd)->obj, sd->scroll_pan.x, sd->scroll_pan.y, mi->x2 - pan_x_diff, mi->y2 - pan_y_diff);
                  evas_object_smart_callback_call(ELM_WIDGET_DATA(sd)->obj, SIG_SCROLL, NULL);
                  sd->scroll_pan.x = mi->x2 - pan_x_diff;
                  sd->scroll_pan.y = mi->y2 - pan_y_diff;
               }
          }
        else if (mi->n == 2 && sd->perspective_enabled == EINA_TRUE)
          {
             sd->on_hold = EINA_FALSE;
             xdiff = mi->x1 - mi->x2;
             ydiff = mi->y1 - mi->y2;
             if (xdiff < 0) xdiff = -xdiff;
             if (ydiff < 0) ydiff = -ydiff;
             elm_coords_finger_size_adjust(1, &fw, 1, &fh);
             if ((xdiff <= (fw/2)) && (ydiff >= fh))
               {
                  if (fh == 0) fh = 60.0;
                  sd->pinch_pan.perspect +=
                     (double)(sd->pinch_pan.y - mi->y2)/3;
                  if (sd->pinch_pan.perspect > 90)
                     sd->pinch_pan.perspect = 90;
                  else if (sd->pinch_pan.perspect < 0)
                     sd->pinch_pan.perspect = 0;

                  sd->engine->perspective
                     (ELM_WIDGET_DATA(sd)->obj, sd->pinch_pan.perspect, 0);
               }
             sd->pinch_pan.x = mi->x2;
             sd->pinch_pan.y = mi->y2;
          }
        _overlay_place(sd);
     }

   if (sd->long_timer)
     {
        xdiff = mi->x1 - mi->x2;
        ydiff = mi->y1 - mi->y2;
        if (xdiff < 0) xdiff = -xdiff;
        if (ydiff < 0) ydiff = -ydiff;
        elm_coords_finger_size_adjust(1, &fw, 1, &fh);
        if ((xdiff >= fw) || (ydiff >= fh))
          {
             ecore_timer_del(sd->long_timer);
             sd->long_timer = NULL;
          }
     }
   return EVAS_EVENT_FLAG_NONE;
}

static Evas_Event_Flags
_pinch_momentum_end_cb(void *data,
                        void *ei)
{
   Elm_Map_Smart_Data *sd = data;
   EINA_SAFETY_ON_NULL_RETURN_VAL(sd, EVAS_EVENT_FLAG_NONE);
   Elm_Gesture_Momentum_Info *mi = ei;

   if (IS_EXTERNAL_ENGINE(sd) && (mi->n == 1))
     {
	 if (sd->scroll_pan.is_scroll_start)
         {
            evas_object_smart_callback_call(ELM_WIDGET_DATA(sd)->obj, SIG_SCROLL_DRAG_STOP, NULL);
            sd->scroll_pan.is_scroll_start = EINA_FALSE;
            sd->engine->pan(ELM_WIDGET_DATA(sd)->obj, sd->scroll_pan.x,
                        sd->scroll_pan.y,
                        sd->scroll_pan.x + (mi->mx)/25,
                        sd->scroll_pan.y + (mi->my)/25);
           _overlay_place(sd);
         }
     }

   return EVAS_EVENT_FLAG_NONE;
}

static Evas_Event_Flags
_pinch_momentum_abort_cb(void *data,
                        void *ei)
{
   Elm_Map_Smart_Data *sd = data;
   EINA_SAFETY_ON_NULL_RETURN_VAL(sd, EVAS_EVENT_FLAG_NONE);
   Elm_Gesture_Momentum_Info *mi = ei;

   if (IS_EXTERNAL_ENGINE(sd) && (mi->n == 1))
     {
	 if (sd->scroll_pan.is_scroll_start)
         {
            evas_object_smart_callback_call(ELM_WIDGET_DATA(sd)->obj, SIG_SCROLL_DRAG_STOP, NULL);
            sd->scroll_pan.is_scroll_start = EINA_FALSE;
         }
     }

   return EVAS_EVENT_FLAG_NONE;
}

static void
_elm_map_pan_smart_pos_set(Evas_Object *obj,
                           Evas_Coord x,
                           Evas_Coord y)
{
   ELM_MAP_PAN_DATA_GET(obj, psd);

   if ((x == psd->wsd->pan_x) && (y == psd->wsd->pan_y)) return;

   psd->wsd->pan_x = x;
   psd->wsd->pan_y = y;

   evas_object_smart_changed(obj);
}

static void
_elm_map_pan_smart_pos_get(const Evas_Object *obj,
                           Evas_Coord *x,
                           Evas_Coord *y)
{
   ELM_MAP_PAN_DATA_GET(obj, psd);

   if (x) *x = psd->wsd->pan_x;
   if (y) *y = psd->wsd->pan_y;
}

static void
_elm_map_pan_smart_pos_max_get(const Evas_Object *obj,
                               Evas_Coord *x,
                               Evas_Coord *y)
{
   Evas_Coord ow, oh;

   ELM_MAP_PAN_DATA_GET(obj, psd);

   evas_object_geometry_get(obj, NULL, NULL, &ow, &oh);
   ow = psd->wsd->size.w - ow;
   oh = psd->wsd->size.h - oh;

   if (ow < 0) ow = 0;
   if (oh < 0) oh = 0;
   if (x) *x = ow;
   if (y) *y = oh;
}

static void
_elm_map_pan_smart_pos_min_get(const Evas_Object *obj __UNUSED__,
                               Evas_Coord *x,
                               Evas_Coord *y)
{
   if (x) *x = 0;
   if (y) *y = 0;
}

static void
_elm_map_pan_smart_content_size_get(const Evas_Object *obj,
                                    Evas_Coord *w,
                                    Evas_Coord *h)
{
   ELM_MAP_PAN_DATA_GET(obj, psd);

   if (w) *w = psd->wsd->size.w;
   if (h) *h = psd->wsd->size.h;
}

static void
_elm_map_pan_smart_add(Evas_Object *obj)
{
   /* here just to allocate our extended data */
   EVAS_SMART_DATA_ALLOC(obj, Elm_Map_Pan_Smart_Data);

   ELM_PAN_CLASS(_elm_map_pan_parent_sc)->base.add(obj);
}

static void
_elm_map_pan_smart_resize(Evas_Object *obj,
                          Evas_Coord w __UNUSED__,
                          Evas_Coord h __UNUSED__)
{
   ELM_MAP_PAN_DATA_GET(obj, psd);

   _sizing_eval(psd->wsd);
   if (!psd->wsd->calc_job.zoom_changed)
     {
        psd->wsd->calc_job.zoom_changed = EINA_TRUE;
        psd->wsd->calc_job.zoom_anim = EINA_FALSE;
        psd->wsd->calc_job.zoom = psd->wsd->zoom;
     }
   evas_object_smart_changed(obj);
}

static Evas_Object *
_map_pan_add(Evas_Object *obj)
{
   Evas_Coord minw, minh;
   ELM_MAP_DATA_GET(obj, sd);

   if (!sd->s_iface)
     {
        sd->layout = edje_object_add(evas_object_evas_get(obj));
        elm_widget_theme_object_set
           (obj, sd->layout , "map", "base", elm_widget_style_get(obj));

        /* common scroller hit rectangle setup */
        sd->hit_rect = evas_object_rectangle_add(evas_object_evas_get(obj));
        evas_object_smart_member_add(sd->hit_rect, obj);
        elm_widget_sub_object_add(obj, sd->hit_rect);
        evas_object_color_set(sd->hit_rect, 0, 0, 0, 0);
        evas_object_show(sd->hit_rect);
        evas_object_repeat_events_set(sd->hit_rect, EINA_TRUE);

        /* interface's add() routine issued AFTER the object's smart_add() */
        sd->s_iface = evas_object_smart_interface_get
           (obj, ELM_SCROLLABLE_IFACE_NAME);

        sd->s_iface->objects_set(obj, sd->layout, sd->hit_rect);
        edje_object_size_min_calc(sd->layout, &minw, &minh);
        evas_object_size_hint_min_set(obj, minw, minh);
        sd->s_iface->wheel_disabled_set(obj, EINA_TRUE);
        sd->s_iface->bounce_allow_set
           (obj, _elm_config->thumbscroll_bounce_enable,
            _elm_config->thumbscroll_bounce_enable);
        sd->s_iface->extern_pan_set(obj, sd->pan_obj);

     }

   evas_object_show(sd->layout);
   sd->s_iface->animate_start_cb_set(obj, _scroll_animate_start_cb);
   sd->s_iface->animate_stop_cb_set(obj, _scroll_animate_stop_cb);
   sd->s_iface->scroll_cb_set(obj, _scroll_cb);

   sd->sep_maps_overlays =
      evas_object_rectangle_add(evas_object_evas_get(obj));
   elm_widget_sub_object_add(obj, sd->sep_maps_overlays);
   evas_object_smart_member_add(sd->sep_maps_overlays, sd->pan_obj);

   sd->map = evas_map_new(EVAS_MAP_POINT);

   srand(time(NULL));
   sd->id = ((int)getpid() << 16) | id_num;
   id_num++;
   _grid_all_create(sd);

   // FIXME: Tile Provider is better to provide default tile size!
   sd->tsize = DEFAULT_TILE_SIZE;
   sd->size.w = sd->tsize;
   sd->size.h = sd->tsize;
   sd->size.tile = sd->tsize;

   if (!ecore_file_download_protocol_available("http://"))
     ERR("Ecore must be built with curl support for the map widget!");
   return sd->layout;
}

static void
_map_pan_del(Evas_Object *obj)
{
   Elm_Map_Route *r;
   Elm_Map_Name *na;
   Eina_List *l, *ll;
   Evas_Object *track;
   Elm_Map_Overlay *overlay;
   ELM_MAP_DATA_GET(obj, sd);

   EINA_LIST_FOREACH_SAFE(sd->routes, l, ll, r)
     elm_map_route_del(r);
   eina_list_free(sd->routes);
   sd->routes = NULL;

   EINA_LIST_FOREACH_SAFE(sd->names, l, ll, na)
     elm_map_name_del(na);
   eina_list_free(sd->names);
   sd->names = NULL;

   EINA_LIST_FOREACH_SAFE(sd->overlays, l, ll, overlay)
     elm_map_overlay_del(overlay);
   eina_list_free(sd->overlays);
   sd->overlays = NULL;

   eina_list_free(sd->group_overlays);
   sd->group_overlays = NULL;
   eina_list_free(sd->all_overlays);
   sd->all_overlays = NULL;

   EINA_LIST_FREE (sd->track, track)
     evas_object_del(track);
   sd->track = NULL;

   if (sd->scr_timer)
     {
        ecore_timer_del(sd->scr_timer);
        sd->scr_timer = NULL;
     }
   if (sd->zoom_animator)
     {
        ecore_animator_del(sd->zoom_animator);
        sd->zoom_animator = NULL;
     }
   _grid_all_clear(sd);
   if (sd->download_idler)
    {   ecore_idler_del(sd->download_idler);
        sd->download_idler = NULL;
    }
   if (sd->download_list) eina_list_free(sd->download_list);

   {
      char buf[4096];
      const char *cachedir;

#ifdef ELM_EFREET
      snprintf(buf, sizeof(buf), "%s" CACHE_ROOT, efreet_cache_home_get());
      (void)cachedir;
#else
      cachedir = getenv("XDG_CACHE_HOME");
      snprintf(buf, sizeof(buf), "%s/%s" CACHE_ROOT, getenv("HOME"),
               cachedir ? : "/.config");
#endif
      if (!ecore_file_recursive_rm(buf))
        WRN("Deletion of %s failed", buf);
   }
   if (sd->map)
     {
        evas_map_free(sd->map);
        sd->map = NULL;
     }

   evas_object_smart_member_del(sd->sep_maps_overlays);
   elm_widget_sub_object_del(obj, sd->sep_maps_overlays);
   evas_object_del(sd->sep_maps_overlays);
   sd->sep_maps_overlays = NULL;

   sd->s_iface->scroll_cb_set(obj, NULL);
   sd->s_iface->animate_stop_cb_set(obj, NULL);
   sd->s_iface->animate_start_cb_set(obj, NULL);
}

static void
_map_pan_show(Evas_Object *obj, double lon, double lat, int animation)
{
   int x, y, w, h;
   ELM_MAP_DATA_GET(obj, sd);

   _region_to_coord_convert
     (sd, lon, lat, sd->size.w, &x, &y);
   _viewport_coord_get(sd, NULL, NULL, &w, &h);
   x = x - (w / 2);
   y = y - (h / 2);

   if (animation)
      sd->s_iface->region_bring_in(ELM_WIDGET_DATA(sd)->obj, x, y, w, h);
   else
      sd->s_iface->content_region_show(ELM_WIDGET_DATA(sd)->obj, x, y, w, h);
}

static void
_map_pan_show_area(Evas_Object *obj __UNUSED__, double max_lon __UNUSED__,
double min_lon __UNUSED__, double max_lat __UNUSED__, double min_lat __UNUSED__, int animation __UNUSED__, void (*result_callback)(Evas_Object *obj) __UNUSED__)
{
	WRN("Not implemented function");
}

static void
_map_pan_zoom_do(Elm_Map_Smart_Data *sd,
         double zoom)
{
   Evas_Coord vx, vy, vw, vh;
   Evas_Coord ow, oh;

   if (zoom > sd->src_tile->zoom_max) zoom = sd->src_tile->zoom_max;
   else if (zoom < sd->src_tile->zoom_min)
     zoom = sd->src_tile->zoom_min;

   ow = sd->size.w;
   oh = sd->size.h;
   sd->size.tile = pow(2.0, (zoom - sd->zoom)) * sd->tsize;
   sd->size.w = pow(2.0, sd->zoom) * sd->size.tile;
   sd->size.h = sd->size.w;

   // Fix to zooming with (viewport center vx, vy) as the center to prevent
   // from zooming with (0,0) as the cetner. (scroller default behavior)
   _viewport_coord_get(sd, &vx, &vy, &vw, &vh);
   if ((vw > 0) && (vh > 0) && (ow > 0) && (oh > 0))
     {
        Evas_Coord x, y;
        double sx, sy;

        if (vw > ow) sx = 0.5;
        else sx = (double)(vx + (double)(vw / 2)) / ow;
        if (vh > oh) sy = 0.5;
        else sy = (double)(vy + (double)(vh / 2)) / oh;

        if (sx > 1.0) sx = 1.0;
        if (sy > 1.0) sy = 1.0;

        x = ceil((sx * sd->size.w) - (vw / 2));
        y = ceil((sy * sd->size.h) - (vh / 2));
        if (x < 0) x = 0;
        else if (x > (sd->size.w - vw))
          x = sd->size.w - vw;
        if (y < 0) y = 0;
        else if (y > (sd->size.h - vh))
          y = sd->size.h - vh;
        sd->s_iface->content_region_show
          (ELM_WIDGET_DATA(sd)->obj, x, y, vw, vh);
     }

   evas_object_smart_callback_call(sd->pan_obj, "changed", NULL);
   evas_object_smart_changed(sd->pan_obj);
}

static Eina_Bool
_zoom_anim_cb(void *data)
{
   Elm_Map_Smart_Data *sd = data;

   if (sd->ani.cnt <= 0)
     {
        sd->zoom_animator = NULL;
        evas_object_smart_changed(sd->pan_obj);

        return ECORE_CALLBACK_CANCEL;
     }
   else
     {
        sd->ani.zoom += sd->ani.diff;
        sd->ani.cnt--;
        sd->zoom_detail = sd->ani.zoom;
        sd->zoom = ROUND(sd->zoom_detail);
        _map_pan_zoom_do(sd, sd->ani.zoom);

        return ECORE_CALLBACK_RENEW;
     }
}

static void
_map_pan_zoom(Evas_Object *obj, double zoom, int animation)
{
   ELM_MAP_DATA_GET(obj, sd);

   if (sd->mode == ELM_MAP_ZOOM_MODE_MANUAL)
     {
        if (animation)
          {
             sd->ani.cnt = 10;
             sd->ani.zoom = sd->zoom;
             sd->ani.diff = (double)(zoom - sd->zoom) / 10;
             if (sd->zoom_animator) ecore_animator_del(sd->zoom_animator);
             sd->zoom_animator = ecore_animator_add(_zoom_anim_cb, sd);
          }
        else
          {
             sd->zoom_detail = zoom;
             sd->zoom = ROUND(sd->zoom_detail);
             _map_pan_zoom_do(sd, zoom);
          }
     }
   else
     {
        double diff;
        Evas_Coord w, h;
        Evas_Coord vw, vh;

        w = sd->size.w;
        h = sd->size.h;
        _viewport_coord_get(sd, NULL, NULL, &vw, &vh);

        if (sd->mode == ELM_MAP_ZOOM_MODE_AUTO_FIT)
          {
             if ((w < vw) && (h < vh))
               {
                  diff = 0.001;
                  while ((w < vw) && (h < vh))
                    {
                       zoom += diff;
                       w = pow(2.0, zoom) * sd->tsize;
                       h = pow(2.0, zoom) * sd->tsize;
                    }
               }
             else
               {
                  diff = -0.001;
                  while ((w > vw) || (h > vh))
                    {
                       zoom += diff;
                       w = pow(2.0, zoom) * sd->tsize;
                       h = pow(2.0, zoom) * sd->tsize;
                    }
               }
          }
        else if (sd->mode == ELM_MAP_ZOOM_MODE_AUTO_FILL)
          {
             if ((w < vw) || (h < vh))
               {
                  diff = 0.01;
                  while ((w < vw) || (h < vh))
                    {
                       zoom += diff;
                       w = pow(2.0, zoom) * sd->tsize;
                       h = pow(2.0, zoom) * sd->tsize;
                    }
               }
             else
               {
                  diff = -0.01;
                  while ((w > vw) && (h > vh))
                    {
                       zoom += diff;
                       w = pow(2.0, zoom) * sd->tsize;
                       h = pow(2.0, zoom) * sd->tsize;
                    }
               }
          }
        sd->zoom_detail = zoom;
        sd->zoom = ROUND(sd->zoom_detail);
        _map_pan_zoom_do(sd, zoom);
     }
}

static double
_map_pan_zoom_level_get(Evas_Object *obj)
{
   ELM_MAP_DATA_GET(obj, sd);
   return sd->zoom_detail;
}

static void
_map_pan_rotate(Evas_Object *obj, double angle, int x, int y, int animation __UNUSED__)
{
   ELM_MAP_DATA_GET(obj, sd);
   sd->pan_rotate.d = angle;
   sd->pan_rotate.cx = x;
   sd->pan_rotate.cy = y;
   evas_object_smart_changed(sd->pan_obj);
}

static void
_map_pan_rotate_get(const Evas_Object *obj, double *angle, int *x, int *y)
{
   ELM_MAP_DATA_GET(obj, sd);
   if (angle) *angle = sd->pan_rotate.d;
   if (x) *x = sd->pan_rotate.cx;
   if (y) *y = sd->pan_rotate.cy;
}

static void
_map_pan_perspective_set(Evas_Object *obj __UNUSED__, double perspective __UNUSED__, int animate __UNUSED__)
{
   WRN("Not implemented function");
}

static void
_map_pan_region_get(const Evas_Object *obj, double *lon, double *lat)
{
   double tlon, tlat;
   Evas_Coord vx, vy, vw, vh;
   ELM_MAP_DATA_GET(obj, sd);

   _viewport_coord_get(sd, &vx, &vy, &vw, &vh);
   _coord_to_region_convert
     (sd, vx + vw / 2, vy + vh / 2, sd->size.w, &tlon, &tlat);
   if (lon) *lon = tlon;
   if (lat) *lat = tlat;
}

static void
_map_pan_canvas_to_region(const Evas_Object *obj, int x, int y, double *lon, double *lat)
{
   ELM_MAP_DATA_GET(obj, sd);
   _canvas_to_coord(sd, x, y, &x, &y);
   _coord_to_region_convert(sd, x, y, sd->size.w, lon, lat);
}

static void
_map_pan_region_to_canvas(const Evas_Object *obj, double lon, double lat, int *x, int *y)
{
   ELM_MAP_DATA_GET(obj, sd);
   _region_to_coord_convert(sd, lon, lat, sd->size.w, x, y);
   _coord_to_canvas(sd, *x, *y, x, y);
}

static void
_move_unimplemented(Evas_Object *obj __UNUSED__, int x __UNUSED__, int y __UNUSED__)
{
   WRN("move is not implemented");
}

static void
_resize_unimplemented(Evas_Object *obj __UNUSED__, int w __UNUSED__, int y __UNUSED__)
{
   WRN("resize is not implemented");
}

static void
_key_set_unimplemented(Evas_Object *obj __UNUSED__, const char *key __UNUSED__)
{
   WRN("key set is not implemented");
}

static void
_pan_unimplemented(Evas_Object *obj __UNUSED__, int x_1 __UNUSED__, int y_1 __UNUSED__, int x_2 __UNUSED__, int y_2 __UNUSED__)
{
   WRN("pan is not implemented");
}

static void
_canvas_to_region_unimplemented(const Evas_Object *obj __UNUSED__, int x __UNUSED__, int y __UNUSED__, double *lon __UNUSED__, double *lat __UNUSED__)
{
   WRN("canvas to region is not implemented");
}

static void
_region_to_canvas_unimplemented(const Evas_Object *obj __UNUSED__, double lon __UNUSED__, double lat __UNUSED__, int *x __UNUSED__, int *y __UNUSED__)
{
   WRN("region to canvas is not implemented");
}

static void
_map_pan_overlays_show(Elm_Map_Smart_Data *sd, Eina_List *overlays)
{
   double max_lon, min_lon, max_lat, min_lat, lon, lat;
   int zoom, zoom_max;
   Evas_Coord vw, vh;

   EINA_SAFETY_ON_NULL_RETURN(sd);
   EINA_SAFETY_ON_NULL_RETURN(overlays);

   _region_max_min_get(overlays, &max_lon, &min_lon, &max_lat, &min_lat);
   lon = (max_lon + min_lon) / 2;
   lat = (max_lat + min_lat) / 2;

   zoom = sd->src_tile->zoom_min;
   _viewport_coord_get(sd, NULL, NULL, &vw, &vh);
   if (sd->src_tile->zoom_max < sd->zoom_max)
     zoom_max = sd->src_tile->zoom_max;
   else zoom_max = sd->zoom_max;
   while (zoom <= zoom_max)
     {
        Evas_Coord size, max_x, max_y, min_x, min_y;

        size = pow(2.0, zoom) * sd->tsize;
        _region_to_coord_convert(sd, min_lon, max_lat, size, &min_x, &min_y);//max lat is min_y.
        _region_to_coord_convert(sd, max_lon, min_lat, size, &max_x, &max_y);//min lat is max_y.
        if ((max_x - min_x) > vw || (max_y - min_y) > vh)
          break;
        zoom++;
     }
   zoom--;

   sd->engine->zoom(ELM_WIDGET_DATA(sd)->obj, zoom, 0);
   sd->engine->show(ELM_WIDGET_DATA(sd)->obj, lon, lat, 0);
}

static void
_calc_job(Elm_Map_Smart_Data *sd)
{
   if (sd->calc_job.zoom_changed)
     {
        _map_pan_zoom(ELM_WIDGET_DATA(sd)->obj, sd->calc_job.zoom, sd->calc_job.zoom_anim);
        sd->calc_job.zoom_changed = EINA_FALSE;
     }
   if (!sd->zoom_animator && sd->calc_job.show_changed)
     {
        _map_pan_show(ELM_WIDGET_DATA(sd)->obj, sd->calc_job.lon, sd->calc_job.lat,
                      sd->calc_job.show_anim);
        sd->calc_job.show_changed = EINA_FALSE;
     }
   if (!sd->zoom_animator && sd->calc_job.overlays_changed)
     {
        _map_pan_overlays_show(sd, sd->calc_job.overlays);
        sd->calc_job.overlays_changed = EINA_FALSE;
     }
}

static void
_elm_map_pan_smart_calculate(Evas_Object *obj)
{
   Evas_Coord w, h;

   ELM_MAP_PAN_DATA_GET(obj, psd);

   evas_object_geometry_get(obj, NULL, NULL, &w, &h);
   if (w <= 0 || h <= 0) return;

   _grid_place(psd->wsd);
   _overlay_place(psd->wsd);
   _track_place(psd->wsd);
   _calc_job(psd->wsd);
}

static void
_elm_map_pan_smart_move(Evas_Object *obj,
                        Evas_Coord x __UNUSED__,
                        Evas_Coord y __UNUSED__)
{
   EINA_SAFETY_ON_NULL_RETURN(obj);

   evas_object_smart_changed(obj);
}

static void
_elm_map_pan_smart_set_user(Elm_Map_Pan_Smart_Class *sc)
{
   ELM_PAN_CLASS(sc)->base.add = _elm_map_pan_smart_add;
   ELM_PAN_CLASS(sc)->base.move = _elm_map_pan_smart_move;
   ELM_PAN_CLASS(sc)->base.resize = _elm_map_pan_smart_resize;
   ELM_PAN_CLASS(sc)->base.calculate = _elm_map_pan_smart_calculate;

   ELM_PAN_CLASS(sc)->pos_set = _elm_map_pan_smart_pos_set;
   ELM_PAN_CLASS(sc)->pos_get = _elm_map_pan_smart_pos_get;
   ELM_PAN_CLASS(sc)->pos_max_get = _elm_map_pan_smart_pos_max_get;
   ELM_PAN_CLASS(sc)->pos_min_get = _elm_map_pan_smart_pos_min_get;
   ELM_PAN_CLASS(sc)->content_size_get = _elm_map_pan_smart_content_size_get;
}

static Eina_Bool
_elm_map_smart_on_focus(Evas_Object *obj, Elm_Focus_Info *info)
{
   ELM_MAP_DATA_GET(obj, sd);

   if (!ELM_WIDGET_CLASS(_elm_map_parent_sc)->on_focus(obj, info))
     return EINA_FALSE;

   if (elm_widget_focus_get(obj))
     {
        edje_object_signal_emit
          (ELM_WIDGET_DATA(sd)->resize_obj, "elm,action,focus", "elm");
        evas_object_focus_set(ELM_WIDGET_DATA(sd)->resize_obj, EINA_TRUE);
     }
   else
     {
        edje_object_signal_emit
          (ELM_WIDGET_DATA(sd)->resize_obj, "elm,action,unfocus", "elm");
        evas_object_focus_set(ELM_WIDGET_DATA(sd)->resize_obj, EINA_FALSE);
     }

   return EINA_TRUE;
}

static Eina_Bool
_elm_map_smart_theme(Evas_Object *obj)
{
   ELM_MAP_DATA_GET(obj, sd);

   if (!ELM_WIDGET_CLASS(_elm_map_parent_sc)->theme(obj))
     return EINA_FALSE;

   _sizing_eval(sd);

   return EINA_TRUE;
}

static Eina_Bool
_elm_map_smart_event(Evas_Object *obj,
                     Evas_Object *src __UNUSED__,
                     Evas_Callback_Type type,
                     void *event_info)
{
   Evas_Coord vh;
   Evas_Coord x, y;
   Evas_Event_Key_Down *ev = event_info;
   Evas_Coord step_x, step_y, page_x, page_y;

   ELM_MAP_DATA_GET(obj, sd);

   // TIZEN ONLY(20131221) : When access mode, focused ui is disabled.
   if (_elm_config->access_mode) return EINA_FALSE;

   if (elm_widget_disabled_get(obj)) return EINA_FALSE;
   if (type != EVAS_CALLBACK_KEY_DOWN) return EINA_FALSE;
   if (ev->event_flags & EVAS_EVENT_FLAG_ON_HOLD) return EINA_FALSE;

   sd->s_iface->content_pos_get(obj, &x, &y);
   sd->s_iface->step_size_get(obj, &step_x, &step_y);
   sd->s_iface->page_size_get(obj, &page_x, &page_y);
   sd->s_iface->content_viewport_size_get(obj, NULL, &vh);

   if ((!strcmp(ev->keyname, "Left")) ||
       ((!strcmp(ev->keyname, "KP_Left")) && (!ev->string)))
     {
        x -= step_x;
     }
   else if ((!strcmp(ev->keyname, "Right")) ||
            ((!strcmp(ev->keyname, "KP_Right")) && (!ev->string)))
     {
        x += step_x;
     }
   else if ((!strcmp(ev->keyname, "Up")) ||
            ((!strcmp(ev->keyname, "KP_Up")) && (!ev->string)))
     {
        y -= step_y;
     }
   else if ((!strcmp(ev->keyname, "Down")) ||
            ((!strcmp(ev->keyname, "KP_Down")) && (!ev->string)))
     {
        y += step_y;
     }
   else if ((!strcmp(ev->keyname, "Prior")) ||
            ((!strcmp(ev->keyname, "KP_Prior")) && (!ev->string)))
     {
        if (page_y < 0)
          y -= -(page_y * vh) / 100;
        else
          y -= page_y;
     }
   else if ((!strcmp(ev->keyname, "Next")) ||
            ((!strcmp(ev->keyname, "KP_Next")) && (!ev->string)))
     {
        if (page_y < 0)
          y += -(page_y * vh) / 100;
        else
          y += page_y;
     }
   else if (!strcmp(ev->keyname, "KP_Add"))
     {
        _zoom(sd, sd->zoom + 1, 1);
        return EINA_TRUE;
     }
   else if (!strcmp(ev->keyname, "KP_Subtract"))
     {
        _zoom(sd, sd->zoom - 1, 1);
        return EINA_TRUE;
     }
   else return EINA_FALSE;

   ev->event_flags |= EVAS_EVENT_FLAG_ON_HOLD;
   sd->s_iface->content_pos_set(obj, x, y, EINA_TRUE);

   return EINA_TRUE;
}

static void
_source_tile_set(Elm_Map_Smart_Data *sd,
                 const char *source_name)
{
   Source_Tile *s;
   Eina_List *l;

   EINA_SAFETY_ON_NULL_RETURN(source_name);

   if (sd->src_tile && !strcmp(sd->src_tile->name, source_name))
     return;

   EINA_LIST_FOREACH(sd->src_tiles, l, s)
     {
        if (!strcmp(s->name, source_name))
          {
             sd->src_tile = s;
             break;
          }
     }
   if (!sd->src_tile)
     {
        ERR("source name (%s) is not found", source_name);
        return;
     }

   if (sd->src_tile->zoom_max < sd->zoom)
     sd->zoom = sd->src_tile->zoom_max;
   else if (sd->src_tile->zoom_min > sd->zoom)
     sd->zoom = sd->src_tile->zoom_min;

   if (sd->src_tile->zoom_max < sd->zoom_max)
     sd->zoom_max = sd->src_tile->zoom_max;
   if (sd->src_tile->zoom_min > sd->zoom_min)
     sd->zoom_min = sd->src_tile->zoom_min;

   _grid_all_clear(sd);
   _grid_all_create(sd);
   sd->zoom_detail = sd->zoom;
   _map_pan_zoom_do(sd, sd->zoom);
}

static void
_source_route_set(Elm_Map_Smart_Data *sd,
                  const char *source_name)
{
   Source_Route *s;
   Eina_List *l;

   EINA_SAFETY_ON_NULL_RETURN(source_name);

   if (sd->src_route && !strcmp(sd->src_route->name, source_name))
     return;

   EINA_LIST_FOREACH(sd->src_routes, l, s)
     {
        if (!strcmp(s->name, source_name))
          {
             sd->src_route = s;
             break;
          }
     }
   if (!sd->src_route)
     {
        ERR("source name (%s) is not found", source_name);
        return;
     }
}

static void
_source_name_set(Elm_Map_Smart_Data *sd,
                 const char *source_name)
{
   Source_Name *s;
   Eina_List *l;

   EINA_SAFETY_ON_NULL_RETURN(source_name);

   if (sd->src_name && !strcmp(sd->src_name->name, source_name))
     return;

   EINA_LIST_FOREACH(sd->src_names, l, s)
     {
        if (!strcmp(s->name, source_name))
          {
             sd->src_name = s;
             break;
          }
     }
   if (!sd->src_name)
     {
        ERR("source name (%s) is not found", source_name);
        return;
     }
}

static Eina_Bool
_source_mod_cb(Eina_Module *m,
                    void *data)
{
   const char *file;
   Elm_Map_Smart_Data *sd = data;

   Elm_Map_Module_Source_Name_Func name_cb;
   Elm_Map_Module_Add_Func add_cb;
   Elm_Map_Module_Key_Set_Func key_set_cb;
   Elm_Map_Module_Del_Func del_cb;
   Elm_Map_Module_Move_Func move_cb;
   Elm_Map_Module_Resize_Func resize_cb;
   Elm_Map_Module_Region_Get_Func region_get_cb;
   Elm_Map_Module_Pan_Func pan_cb;
   Elm_Map_Module_Show_Func show_cb;
   Elm_Map_Module_Show_Area_Func show_area_cb;
   Elm_Map_Module_Zoom_Func zoom_cb;
   Elm_Map_Module_Zoom_Level_Get_Func zoom_level_get_cb;
   Elm_Map_Module_Rotate_Func rotate_cb;
   Elm_Map_Module_Rotate_Get_Func rotate_get_cb;
   Elm_Map_Module_Canvas_to_Region_Func canvas_to_region_cb;
   Elm_Map_Module_Region_to_Canvas_Func region_to_canvas_cb;
   Elm_Map_Module_Scale_Get_Func scale_get_cb;
   Elm_Map_Module_Perspective_Set_Func perspective_cb;
   Elm_Map_Module_Tile_Url_Func tile_url_cb;
   Elm_Map_Module_Tile_Scale_Func scale_cb;
   Elm_Map_Module_Tile_Zoom_Min_Func zoom_min;
   Elm_Map_Module_Tile_Zoom_Max_Func zoom_max;
   Elm_Map_Module_Tile_Geo_to_Coord_Func geo_to_coord;
   Elm_Map_Module_Tile_Coord_to_Geo_Func coord_to_geo;
   Elm_Map_Module_Route_Url_Func route_url_cb;
   Elm_Map_Module_Name_Url_Func name_url_cb;

   EINA_SAFETY_ON_NULL_RETURN_VAL(data, EINA_FALSE);

   file = eina_module_file_get(m);
   if (!eina_module_load(m))
     {
        ERR("Could not load module \"%s\": %s", file,
            eina_error_msg_get(eina_error_get()));
        return EINA_FALSE;
     }
   name_cb = eina_module_symbol_get(m, "map_module_source_name_get");
   if (!name_cb)
     {
        name_cb = eina_module_symbol_get(m, "map_module_name_get");
        if (!name_cb)
          {
             WRN("Could not find map module name from module \"%s\": %s",
                 file, eina_error_msg_get(eina_error_get()));
             eina_module_unload(m);
             return EINA_FALSE;
          }
     }

   // Find Map Engine
   zoom_min = eina_module_symbol_get(m, "map_module_zoom_min_get");
   zoom_max = eina_module_symbol_get(m, "map_module_zoom_max_get");
   add_cb = eina_module_symbol_get(m, "map_module_add");
   key_set_cb = eina_module_symbol_get(m, "map_module_key_set");
   del_cb = eina_module_symbol_get(m, "map_module_del");
   move_cb = eina_module_symbol_get(m, "map_module_move");
   resize_cb = eina_module_symbol_get(m, "map_module_resize");
   region_get_cb = eina_module_symbol_get(m, "map_module_region_get");
   pan_cb = eina_module_symbol_get(m, "map_module_pan");
   show_cb = eina_module_symbol_get(m, "map_module_show");
   show_area_cb = eina_module_symbol_get(m, "map_module_show_area");
   zoom_cb = eina_module_symbol_get(m, "map_module_zoom");
   zoom_level_get_cb = eina_module_symbol_get(m, "map_module_zoom_level_get");
   rotate_cb = eina_module_symbol_get(m, "map_module_rotate");
   rotate_get_cb = eina_module_symbol_get(m, "map_module_rotate_get");
   perspective_cb = eina_module_symbol_get(m, "map_module_perspective_set");
   canvas_to_region_cb = eina_module_symbol_get(m, "map_module_canvas_to_region");
   region_to_canvas_cb = eina_module_symbol_get(m, "map_module_region_to_canvas");
   scale_get_cb = eina_module_symbol_get(m, "map_module_scale_get");
   if (zoom_min && zoom_max && add_cb && del_cb && show_cb && zoom_cb && rotate_cb && rotate_get_cb && perspective_cb)
     {
        INF("Map ENGINE module is loaded \"%s\"",  file);
        Source_Engine *s;
        s = ELM_NEW(Source_Engine);
        s->name = name_cb();
        s->zoom_min = zoom_min();
        s->zoom_max = zoom_max();
        s->add = add_cb;
        s->key_set = key_set_cb;
        s->del = del_cb;
        if (move_cb) s->move = move_cb;
        else s->move = _move_unimplemented;
        if (resize_cb) s->resize = resize_cb;
        else s->resize = _resize_unimplemented;
        s->region_get = region_get_cb;
        if (pan_cb) s->pan = pan_cb;
        else s->pan = _pan_unimplemented;
        s->show = show_cb;
        s->show_area = show_area_cb;
        s->zoom = zoom_cb;
        s->rotate = rotate_cb;
        s->rotate_get = rotate_get_cb;
        s->perspective = perspective_cb;
        if (canvas_to_region_cb) s->canvas_to_region = canvas_to_region_cb;
        else s->canvas_to_region = _canvas_to_region_unimplemented;
        if (region_to_canvas_cb) s->region_to_canvas = region_to_canvas_cb;
        else s->region_to_canvas = _region_to_canvas_unimplemented;
        if (scale_get_cb) s->scale_get = scale_get_cb;
        if (zoom_level_get_cb) s->zoom_level_get = zoom_level_get_cb;

//NLP specific changes
//Turn off/on map engine overlays with this variable during compilation
//put all the engine overlay symbol loading inside this condition
        {
           s->icon_add =  eina_module_symbol_get(m, "map_module_icon_add");
           s->icon_del = eina_module_symbol_get(m, "map_module_icon_del");
           s->polyline_add = eina_module_symbol_get(m, "map_module_polyline_add");
           s->polyline_region_add = eina_module_symbol_get(m, "map_module_polyline_region_add");
           s->polyline_show = eina_module_symbol_get(m, "map_module_polyline_show");
           s->polyline_color_set = eina_module_symbol_get(m, "map_module_polyline_color_set");
           s->polyline_width_set = eina_module_symbol_get(m, "map_module_polyline_width_set");
           s->polyline_width_get = eina_module_symbol_get(m, "map_module_polyline_width_get");
           s->polyline_del = eina_module_symbol_get(m, "map_module_polyline_del");
           //s->polygon_add = eina_module_symbol_get(m, "map_module_polygon_add");
           //s->polygon_del = eina_module_symbol_get(m, "map_module_polygon_del");
           s->route_add =  eina_module_symbol_get(m, "map_module_route_add");
           s->route_del =  eina_module_symbol_get(m, "map_module_route_del");
           s->route_draw = eina_module_symbol_get(m, "map_module_route_draw");
           s->route_color_set = eina_module_symbol_get(m, "map_module_route_color_set");
           s->object_from_coord =  eina_module_symbol_get(m, "map_module_get_object_from_coord");
           s->object_visibility =   eina_module_symbol_get(m, "map_module_object_visibility_set");
           s->object_visibility_range = eina_module_symbol_get(m, "map_module_object_zoom_range_set");
           s->group_create = eina_module_symbol_get(m, "map_module_group_create");
           s->group_del = eina_module_symbol_get(m, "map_module_group_del");
           s->group_object_add = eina_module_symbol_get(m, "map_module_group_object_add");
           s->group_object_del = eina_module_symbol_get(m, "map_module_group_object_del");
           s->map_capture =  eina_module_symbol_get(m, "map_module_offscreen_capture");
           s->display_layer_groups_set = eina_module_symbol_get(m, "map_module_display_layer_groups_set");
           s->display_layer_groups_get = eina_module_symbol_get(m, "map_module_display_layer_groups_get");
           s->traffic_show = eina_module_symbol_get(m, "map_module_traffic_layer_show");
           s->traffic_hide = eina_module_symbol_get(m, "map_module_traffic_layer_hide");
        }

        sd->engines = eina_list_append(sd->engines, s);
     }
   else
   {
      ERR("The specific symbol is not implemented so, Engine loading is failed.");
   }

   // Find TILE module
   tile_url_cb = eina_module_symbol_get(m, "map_module_tile_url_get");
   zoom_min = eina_module_symbol_get(m, "map_module_tile_zoom_min_get");
   zoom_max = eina_module_symbol_get(m, "map_module_tile_zoom_max_get");
   geo_to_coord = eina_module_symbol_get(m, "map_module_tile_geo_to_coord");
   coord_to_geo = eina_module_symbol_get(m, "map_module_tile_coord_to_geo");
   scale_cb = eina_module_symbol_get(m, "map_module_tile_scale_get");
   if (tile_url_cb && zoom_min && zoom_max && geo_to_coord && coord_to_geo && scale_cb)
     {
        INF("Map TILE module is loaded \"%s\"",  file);
        Source_Tile *s;
        s = ELM_NEW(Source_Tile);
        s->name = name_cb();
        s->zoom_min = zoom_min();
        s->zoom_max = zoom_max();
        s->url_cb = tile_url_cb;
        s->geo_to_coord = geo_to_coord;
        s->coord_to_geo = coord_to_geo;
        s->scale_cb = scale_cb;
        sd->src_tiles = eina_list_append(sd->src_tiles, s);
     }

   // Find ROUTE module
   route_url_cb = eina_module_symbol_get(m, "map_module_route_url_get");
   if (route_url_cb)
     {
        INF("Map ROUTE module is loaded \"%s\"",  file);
        Source_Route *s;
        s = ELM_NEW(Source_Route);
        s->name = name_cb();
        s->url_cb = route_url_cb;
        sd->src_routes = eina_list_append(sd->src_routes, s);
     }

   // Find NAME module
   name_url_cb = eina_module_symbol_get(m, "map_module_name_url_get");
   if (name_url_cb)
     {
        INF("Map NAME module is loaded \"%s\"",  file);
        Source_Name *s;
        s = ELM_NEW(Source_Name);
        s->name = name_cb();
        s->url_cb = name_url_cb;
        sd->src_names = eina_list_append(sd->src_names, s);
     }
   return EINA_TRUE;
}

static void
_source_all_unload(Elm_Map_Smart_Data *sd)
{
   int idx = 0;
   Source_Tile *s;

   for (idx = 0; sd->engine_names[idx]; idx++)
     eina_stringshare_del(sd->engine_names[idx]);
   for (idx = 0; sd->src_tile_names[idx]; idx++)
     eina_stringshare_del(sd->src_tile_names[idx]);
   for (idx = 0; sd->src_route_names[idx]; idx++)
     eina_stringshare_del(sd->src_route_names[idx]);
   for (idx = 0; sd->src_name_names[idx]; idx++)
     eina_stringshare_del(sd->src_name_names[idx]);

   free(sd->engine_names);
   free(sd->src_tile_names);
   free(sd->src_route_names);
   free(sd->src_name_names);

   EINA_LIST_FREE(sd->src_tiles, s) free(s);
   EINA_LIST_FREE(sd->src_routes, s) free(s);
   EINA_LIST_FREE(sd->src_names, s) free(s);

   eina_module_list_free(sd->src_mods);
}


static void
_source_all_load(Elm_Map_Smart_Data *sd)
{
   Source_Engine *engine;
   Source_Tile *src_tile;
   Source_Route *src_route;
   Source_Name *src_name;
   unsigned int idx;
   Eina_List *l;

   // Load hard coded TILE source
   for (idx = 0; idx < (sizeof(src_tiles) / sizeof(Source_Tile)); idx++)
     {
        src_tile = ELM_NEW(Source_Tile);
        src_tile->name = src_tiles[idx].name;
        src_tile->zoom_min = src_tiles[idx].zoom_min;
        src_tile->zoom_max = src_tiles[idx].zoom_max;
        src_tile->url_cb = src_tiles[idx].url_cb;
        src_tile->geo_to_coord = src_tiles[idx].geo_to_coord;
        src_tile->coord_to_geo = src_tiles[idx].coord_to_geo;
        src_tile->scale_cb = src_tiles[idx].scale_cb;
        sd->src_tiles = eina_list_append(sd->src_tiles, src_tile);
     }
   // Load hard coded ROUTE source
   for (idx = 0; idx < (sizeof(src_routes) / sizeof(Source_Route)); idx++)
     {
        src_route = ELM_NEW(Source_Route);
        src_route->name = src_routes[idx].name;
        src_route->url_cb = src_routes[idx].url_cb;
        sd->src_routes = eina_list_append(sd->src_routes, src_route);
     }
   // Load from hard coded NAME source
   for (idx = 0; idx < (sizeof(src_names) / sizeof(Source_Name)); idx++)
     {
        src_name = ELM_NEW(Source_Name);
        src_name->name = src_names[idx].name;
        src_name->url_cb = src_names[idx].url_cb;
        sd->src_names = eina_list_append(sd->src_names, src_name);
     }
   // Load Internal Default Map Engine
   engine = ELM_NEW(Source_Engine);
   engine->name = INTERNAL_ENGINE_NAME;
   engine->zoom_min = src_tiles[0].zoom_min;
   engine->zoom_max = src_tiles[0].zoom_max;
   engine->add = _map_pan_add;
   engine->key_set = _key_set_unimplemented;
   engine->del = _map_pan_del;
   engine->pan = _pan_unimplemented;
   engine->show = _map_pan_show;
   engine->show_area = _map_pan_show_area;
   engine->zoom = _map_pan_zoom;
   engine->zoom_level_get = _map_pan_zoom_level_get;
   engine->rotate = _map_pan_rotate;
   engine->rotate_get = _map_pan_rotate_get;
   engine->perspective = _map_pan_perspective_set;
   engine->region_get = _map_pan_region_get;
   engine->canvas_to_region = _map_pan_canvas_to_region;
   engine->region_to_canvas = _map_pan_region_to_canvas;
   sd->engines = eina_list_append(sd->engines, engine);

   // Load from modules
   sd->src_mods = eina_module_list_get(sd->src_mods, MODULES_PATH, 1,
                                       &_source_mod_cb, sd);

   // Set default source
   sd->engine = eina_list_nth(sd->engines, 0);
   sd->src_tile = eina_list_nth(sd->src_tiles, 0);
   sd->src_route = eina_list_nth(sd->src_routes, 0);
   sd->src_name = eina_list_nth(sd->src_names, 0);

   // Make name string of sources
   idx = 0;
   sd->engine_names = calloc((eina_list_count(sd->engines) + 1),
                                 sizeof(const char *));
   EINA_LIST_FOREACH(sd->engines, l, engine)
     {
        eina_stringshare_replace(&sd->engine_names[idx], engine->name);
        INF("engine : %s", sd->engine_names[idx]);
        idx++;
     }
   idx = 0;
   sd->src_tile_names = calloc((eina_list_count(sd->src_tiles) + 1),
                               sizeof(const char *));
   EINA_LIST_FOREACH(sd->src_tiles, l, src_tile)
     {
        eina_stringshare_replace(&sd->src_tile_names[idx], src_tile->name);
        INF("source tile: %s", sd->src_tile_names[idx]);
        idx++;
     }
   idx = 0;
   sd->src_route_names = calloc((eina_list_count(sd->src_routes) + 1),
                                sizeof(const char *));
   EINA_LIST_FOREACH(sd->src_routes, l, src_route)
     {
        eina_stringshare_replace(&sd->src_route_names[idx], src_route->name);
        INF("source route: %s", sd->src_route_names[idx]);
        idx++;
     }
   idx = 0;
   sd->src_name_names = calloc((eina_list_count(sd->src_names) + 1),
                               sizeof(const char *));
   EINA_LIST_FOREACH(sd->src_names, l, src_name)
     {
        eina_stringshare_replace(&sd->src_name_names[idx], src_name->name);
        INF("source name: %s", sd->src_name_names[idx]);
        idx++;
     }
}

static void
_elm_map_smart_add(Evas_Object *obj)
{
   EVAS_SMART_DATA_ALLOC(obj, Elm_Map_Smart_Data);
   Evas_Object *clip_rect;

   ELM_WIDGET_CLASS(_elm_map_parent_sc)->base.add(obj);

   clip_rect = evas_object_rectangle_add(evas_object_evas_get(obj));
   evas_object_clip_set(obj, clip_rect);
   evas_object_pass_events_set(clip_rect, EINA_TRUE);
   evas_object_color_set(clip_rect, 0, 0, 0, 0);
   evas_object_show(clip_rect);
}

static void
_elm_map_smart_del(Evas_Object *obj)
{
   Source_Engine *e;
   ELM_MAP_DATA_GET(obj, sd);

   if (sd->zoom_timer) ecore_timer_del(sd->zoom_timer);
   if (sd->long_timer) ecore_timer_del(sd->long_timer);
   if (sd->loaded_timer) ecore_timer_del(sd->loaded_timer);

   if (sd->user_agent) eina_stringshare_del(sd->user_agent);
   if (sd->ua) eina_hash_free(sd->ua);

   evas_object_clip_unset(obj);

   if (IS_EXTERNAL_ENGINE(sd))
   {
      Elm_Map_Route *r;
      Elm_Map_Name *na;
      Eina_List *l, *ll;
      Elm_Map_Overlay *overlay;

      EINA_LIST_FOREACH_SAFE(sd->routes, l, ll, r)
      elm_map_route_del(r);
      eina_list_free(sd->routes);
      sd->routes = NULL;

      EINA_LIST_FOREACH_SAFE(sd->names, l, ll, na)
      elm_map_name_del(na);
      eina_list_free(sd->names);
      sd->names = NULL;

      EINA_LIST_FOREACH_SAFE(sd->overlays, l, ll, overlay)
      elm_map_overlay_del(overlay);
      eina_list_free(sd->overlays);
      sd->overlays = NULL;

      eina_list_free(sd->group_overlays);
      sd->group_overlays = NULL;
      eina_list_free(sd->all_overlays);
      sd->all_overlays = NULL;
   }

   sd->engine->del(obj);
   EINA_LIST_FREE(sd->engines, e)
     {
        if (e->key) free(e->key);
        free(e);
     }
   _source_all_unload(sd);

   evas_object_del(sd->pan_obj);
   ELM_WIDGET_CLASS(_elm_map_parent_sc)->base.del(obj);
}

static void
_elm_map_smart_calculate(Evas_Object *obj)
{
   ELM_MAP_DATA_GET(obj, sd);

   if (IS_EXTERNAL_ENGINE(sd))
     _overlay_place(sd);
}

static void
_elm_map_smart_move(Evas_Object *obj,
                    Evas_Coord x,
                    Evas_Coord y)
{
   ELM_MAP_DATA_GET(obj, sd);
   Evas_Object *clip_rect = evas_object_clip_get(obj);

   ELM_WIDGET_CLASS(_elm_map_parent_sc)->base.move(obj, x, y);

   if (IS_EXTERNAL_ENGINE(sd))
     {
        sd->engine->move(obj, x, y);
        _overlay_place(sd);
     }
   else
      evas_object_move(sd->hit_rect, x, y);

   if (clip_rect)
     evas_object_move(clip_rect, x, y);
}

static void
_elm_map_smart_resize(Evas_Object *obj,
                      Evas_Coord w,
                      Evas_Coord h)
{
   Evas_Coord map_x = -1, map_y = -1, map_w = -1, map_h = -1;
   Evas_Coord clip_x = -1, clip_y = -1, clip_w = -1, clip_h = -1;
   ELM_MAP_DATA_GET(obj, sd);
   Evas_Object *clip_rect = evas_object_clip_get(obj);

   ELM_WIDGET_CLASS(_elm_map_parent_sc)->base.resize(obj, w, h);

   evas_object_geometry_get(obj, &map_x, &map_y, &map_w, &map_h);

   if (IS_EXTERNAL_ENGINE(sd))
     {
        sd->engine->resize(obj, w, h);
        _overlay_place(sd);
     }
   else
      evas_object_resize(sd->hit_rect, w, h);

   if (clip_rect)
     {
        evas_object_geometry_get(clip_rect, &clip_x, &clip_y, &clip_w, &clip_h);

        if ((map_x != clip_x) || (map_y != clip_y))
        {
           INF("forcely move the clip rect to(x:%d,y:%d)", map_x, map_y);
	       evas_object_move(clip_rect, map_x, map_y);
        }
        evas_object_resize(clip_rect, w, h);
     }
}

static void
_elm_map_smart_member_add(Evas_Object *obj,
                          Evas_Object *member)
{
   ELM_MAP_DATA_GET(obj, sd);

   ELM_WIDGET_CLASS(_elm_map_parent_sc)->base.member_add(obj, member);

   if (sd->hit_rect)
     evas_object_raise(sd->hit_rect);
}

static void
_elm_map_smart_set_user(Elm_Map_Smart_Class *sc)
{
   ELM_WIDGET_CLASS(sc)->base.add = _elm_map_smart_add;
   ELM_WIDGET_CLASS(sc)->base.del = _elm_map_smart_del;
   ELM_WIDGET_CLASS(sc)->base.calculate = _elm_map_smart_calculate;

   ELM_WIDGET_CLASS(sc)->base.move = _elm_map_smart_move;
   ELM_WIDGET_CLASS(sc)->base.resize = _elm_map_smart_resize;
   ELM_WIDGET_CLASS(sc)->base.member_add = _elm_map_smart_member_add;

   ELM_WIDGET_CLASS(sc)->on_focus = _elm_map_smart_on_focus;
   ELM_WIDGET_CLASS(sc)->theme = _elm_map_smart_theme;
   ELM_WIDGET_CLASS(sc)->event = _elm_map_smart_event;
   ELM_WIDGET_CLASS(sc)->translate = _elm_map_smart_translate;
}

static Elm_Map_Route*
_map_custom_route_add(Evas_Object *obj,
                  Elm_Map_Route_Type type,
                  Elm_Map_Route_Method method,
                  double flon,
                  double flat,
                  double tlon,
                  double tlat,
                  Elm_Map_Route_Cb route_cb,
                  void *data)
{
   Elm_Map_Route *route;

   ELM_MAP_CHECK(obj) NULL;
   ELM_MAP_DATA_GET(obj, sd);
   if (!data) return NULL;

   route = ELM_NEW(Elm_Map_Route);
   route->wsd = sd;
   route->fname = NULL;
   route->type = type;
   route->method = method;
   route->flon = flon;
   route->flat = flat;
   route->tlon = tlon;
   route->tlat = tlat;
   route->cb = route_cb;
   route->data = data;

   sd->routes = eina_list_append(sd->routes, route);
   if (route->cb) route->cb(route->data, ELM_WIDGET_DATA(sd)->obj, route);
   evas_object_smart_callback_call
      (ELM_WIDGET_DATA(sd)->obj, SIG_ROUTE_LOADED, NULL);

   return route;
}

static void
_map_custom_route_del(Elm_Map_Route *route)
{
   route->wsd->routes = eina_list_remove(route->wsd->routes, route);
   free(route);
}

#endif

EAPI const Elm_Map_Smart_Class *
elm_map_smart_class_get(void)
{
#ifdef HAVE_ELEMENTARY_ECORE_CON
   static Elm_Map_Smart_Class _sc =
     ELM_MAP_SMART_CLASS_INIT_NAME_VERSION(ELM_MAP_SMART_NAME);
   static const Elm_Map_Smart_Class *class = NULL;
   Evas_Smart_Class *esc = (Evas_Smart_Class *)&_sc;

   if (class) return class;

   _elm_map_smart_set(&_sc);
   esc->callbacks = _smart_callbacks;
   class = &_sc;

   return class;
#else
   return NULL;
#endif
}

EAPI Evas_Object *
elm_map_add(Evas_Object *parent)
{
#ifdef HAVE_ELEMENTARY_ECORE_CON
   Evas_Object *obj;
   Evas_Object *layout;
   Elm_Map_Pan_Smart_Data *pan_data;

   EINA_SAFETY_ON_NULL_RETURN_VAL(parent, NULL);

   obj = elm_widget_add(_elm_map_smart_class_new(), parent);
   if (!obj) return NULL;

   if (!elm_widget_sub_object_add(parent, obj))
     ERR("could not add %p as sub object of %p", obj, parent);

   elm_widget_can_focus_set(obj, EINA_TRUE);

   ELM_MAP_DATA_GET(obj, sd);

   sd->pan_obj = evas_object_smart_add
      (evas_object_evas_get(obj), _elm_map_pan_smart_class_new());
   pan_data = evas_object_smart_data_get(sd->pan_obj);
   pan_data->wsd = sd;

   _source_all_load(sd);
   layout = sd->engine->add(obj);
   elm_widget_resize_object_set(obj, layout, EINA_TRUE);

   evas_object_event_callback_add
     (obj, EVAS_CALLBACK_MOUSE_DOWN, _mouse_down_cb, sd);
   evas_object_event_callback_add
     (obj, EVAS_CALLBACK_MOUSE_UP, _mouse_up_cb, sd);
   evas_object_event_callback_add
     (obj, EVAS_CALLBACK_MOUSE_WHEEL, _mouse_wheel_cb, sd);
   evas_object_event_callback_add
      (obj, EVAS_CALLBACK_CHANGED_SIZE_HINTS, _changed_size_hints_cb, sd);

   sd->g_layer = elm_gesture_layer_add(obj);
   if (!sd->g_layer) ERR("elm_gesture_layer_add() failed");
   elm_gesture_layer_attach(sd->g_layer, obj);
   elm_gesture_layer_cb_set
     (sd->g_layer, ELM_GESTURE_ZOOM, ELM_GESTURE_STATE_START,
     _pinch_zoom_start_cb, sd);
   elm_gesture_layer_cb_set
     (sd->g_layer, ELM_GESTURE_ZOOM, ELM_GESTURE_STATE_MOVE,
     _pinch_zoom_cb, sd);
#if 0 // TIZEN: Remove pinch rotate feature
   elm_gesture_layer_cb_set
     (sd->g_layer, ELM_GESTURE_ROTATE, ELM_GESTURE_STATE_START,
     _pinch_rotate_start_cb, sd);
   elm_gesture_layer_cb_set
     (sd->g_layer, ELM_GESTURE_ROTATE, ELM_GESTURE_STATE_MOVE,
     _pinch_rotate_cb, sd);
#endif
   elm_gesture_layer_cb_set
     (sd->g_layer, ELM_GESTURE_MOMENTUM, ELM_GESTURE_STATE_START,
     _pinch_momentum_start_cb, sd);
   elm_gesture_layer_cb_set
     (sd->g_layer, ELM_GESTURE_MOMENTUM, ELM_GESTURE_STATE_MOVE,
     _pinch_momentum_move_cb, sd);
   elm_gesture_layer_cb_set
     (sd->g_layer, ELM_GESTURE_MOMENTUM, ELM_GESTURE_STATE_END,
     _pinch_momentum_end_cb, sd);

   elm_gesture_layer_cb_set
     (sd->g_layer, ELM_GESTURE_MOMENTUM, ELM_GESTURE_STATE_ABORT,
     _pinch_momentum_abort_cb, sd);

   scroll_threshold = elm_config_scroll_thumbscroll_threshold_get();
   INF("map scrolling threshold(%d)", scroll_threshold);

   sd->mode = ELM_MAP_ZOOM_MODE_MANUAL;
   sd->zoom_min = sd->engine->zoom_min;
   sd->zoom_max = sd->engine->zoom_max;
   sd->zoom = 0;
   sd->zoom_detail = 0;
   sd->pinch_zoom_enabled = EINA_TRUE;
   sd->perspective_enabled = EINA_TRUE;

   sd->engine->zoom(obj, 0, 0);

   //Tizen Only: This should be removed when eo is applied.
   ELM_WIDGET_DATA_GET(obj, wsd);
   wsd->on_create = EINA_FALSE;

   return obj;
#else
   (void)parent;
   return NULL;
#endif
}

EAPI void
elm_map_engine_key_set(Evas_Object *obj,
                       const char *engine_name,
                       const char *key)
{
   ELM_MAP_CHECK(obj);
   ELM_MAP_DATA_GET(obj, sd);
   EINA_SAFETY_ON_NULL_RETURN(engine_name);
   EINA_SAFETY_ON_NULL_RETURN(key);

   Eina_List *l;
   Source_Engine *e;

   EINA_LIST_FOREACH(sd->engines, l, e)
     {
        if (!strcmp(e->name, engine_name))
          {
             if (e->key) free(e->key);
             e->key = strdup(key);
             e->key_set(obj, key);
             break;
          }
     }
}

EAPI void
elm_map_zoom_set(Evas_Object *obj,
                 int zoom)
{
#ifdef HAVE_ELEMENTARY_ECORE_CON

   ELM_MAP_CHECK(obj);
   ELM_MAP_DATA_GET(obj, sd);
   EINA_SAFETY_ON_NULL_RETURN(sd->src_tile);

   Eina_Bool animation;

   if (sd->mode != ELM_MAP_ZOOM_MODE_MANUAL) return;
   if (zoom < 0) zoom = 0;
   if (sd->zoom == zoom) return;

   if (sd->paused) animation = EINA_FALSE;
   else animation = EINA_TRUE;

   if (IS_EXTERNAL_ENGINE(sd))
     _zoom(sd, zoom, animation);
   else
     {
        sd->calc_job.zoom_changed = EINA_TRUE;
        sd->calc_job.zoom_anim = animation;
        sd->calc_job.zoom = zoom;
        evas_object_smart_changed(sd->pan_obj);
     }

#else
   (void)obj;
   (void)zoom;
#endif
}

EAPI int
elm_map_zoom_get(const Evas_Object *obj)
{
#ifdef HAVE_ELEMENTARY_ECORE_CON
   ELM_MAP_CHECK(obj) 0;
   ELM_MAP_DATA_GET(obj, sd);

   return sd->zoom;
#else
   (void)obj;
   return 0;
#endif
}

EAPI void
elm_map_zoom_mode_set(Evas_Object *obj,
                      Elm_Map_Zoom_Mode mode)
{
#ifdef HAVE_ELEMENTARY_ECORE_CON
   ELM_MAP_CHECK(obj);
   ELM_MAP_DATA_GET(obj, sd);

   if ((mode == ELM_MAP_ZOOM_MODE_MANUAL) && (sd->mode == !!mode)) return;

   sd->mode = mode;

   if (IS_EXTERNAL_ENGINE(sd))
     ERR("Engine(%s) do not support this function", sd->engine->name);
   else
     {
        sd->calc_job.zoom_changed = EINA_TRUE;
        sd->calc_job.zoom_anim = EINA_FALSE;
        sd->calc_job.zoom = sd->zoom;
        evas_object_smart_changed(sd->pan_obj);
     }

#else
   (void)obj;
   (void)mode;
#endif
}

EAPI Elm_Map_Zoom_Mode
elm_map_zoom_mode_get(const Evas_Object *obj)
{
#ifdef HAVE_ELEMENTARY_ECORE_CON
   ELM_MAP_CHECK(obj) ELM_MAP_ZOOM_MODE_MANUAL;
   ELM_MAP_DATA_GET(obj, sd);

   return sd->mode;
#else
   (void)obj;
   return ELM_MAP_ZOOM_MODE_MANUAL;
#endif
}

EAPI void
elm_map_zoom_max_set(Evas_Object *obj,
                     int zoom)
{
#ifdef HAVE_ELEMENTARY_ECORE_CON
   ELM_MAP_CHECK(obj);
   ELM_MAP_DATA_GET(obj, sd);
   EINA_SAFETY_ON_NULL_RETURN(sd->src_tile);

   sd->zoom_max = zoom;
#else
   (void)obj;
   (void)zoom;
#endif
}

EAPI int
elm_map_zoom_max_get(const Evas_Object *obj)
{
#ifdef HAVE_ELEMENTARY_ECORE_CON
   ELM_MAP_CHECK(obj) - 1;
   ELM_MAP_DATA_GET(obj, sd);
   EINA_SAFETY_ON_NULL_RETURN_VAL(sd->src_tile, -1);

   return sd->zoom_max;
#else
   (void)obj;
   return -1;
#endif
}

EAPI void
elm_map_zoom_min_set(Evas_Object *obj,
                     int zoom)
{
#ifdef HAVE_ELEMENTARY_ECORE_CON
   ELM_MAP_CHECK(obj);
   ELM_MAP_DATA_GET(obj, sd);
   EINA_SAFETY_ON_NULL_RETURN(sd->src_tile);

   sd->zoom_min = zoom;
#else
   (void)obj;
   (void)zoom;
#endif
}

EAPI int
elm_map_zoom_min_get(const Evas_Object *obj)
{
#ifdef HAVE_ELEMENTARY_ECORE_CON
   ELM_MAP_CHECK(obj) - 1;
   ELM_MAP_DATA_GET(obj, sd);
   EINA_SAFETY_ON_NULL_RETURN_VAL(sd->src_tile, -1);

   return sd->zoom_min;
#else
   (void)obj;
   return -1;
#endif
}

EAPI void
elm_map_region_bring_in(Evas_Object *obj,
                        double lon,
                        double lat)
{
#ifdef HAVE_ELEMENTARY_ECORE_CON
   ELM_MAP_CHECK(obj);
   ELM_MAP_DATA_GET(obj, sd);

   if (IS_EXTERNAL_ENGINE(sd))
     {
        sd->engine->show(obj, lon, lat, 1);
     }
   else
     {
        sd->calc_job.show_changed = EINA_TRUE;
        sd->calc_job.show_anim = EINA_TRUE;
        sd->calc_job.lon = lon;
        sd->calc_job.lat = lat;
        evas_object_smart_changed(sd->pan_obj);
     }

#else
   (void)obj;
   (void)lon;
   (void)lat;
#endif
}

EAPI void
elm_map_region_show(Evas_Object *obj,
                    double lon,
                    double lat)
{
#ifdef HAVE_ELEMENTARY_ECORE_CON
   ELM_MAP_CHECK(obj);
   ELM_MAP_DATA_GET(obj, sd);

   if (IS_EXTERNAL_ENGINE(sd))
     {
        sd->engine->show(obj, lon, lat, 0);
     }
   else
     {
        sd->calc_job.show_changed = EINA_TRUE;
        sd->calc_job.show_anim = EINA_FALSE;
        sd->calc_job.lon = lon;
        sd->calc_job.lat = lat;
        evas_object_smart_changed(sd->pan_obj);
     }

#else
   (void)obj;
   (void)lon;
   (void)lat;
#endif
}

EAPI void
elm_map_region_get(const Evas_Object *obj,
                   double *lon,
                   double *lat)
{
#ifdef HAVE_ELEMENTARY_ECORE_CON

   ELM_MAP_CHECK(obj);
   ELM_MAP_DATA_GET(obj, sd);

   double tlon, tlat;
   sd->engine->region_get(obj, &tlon, &tlat);
   if (lon) *lon = tlon;
   if (lat) *lat = tlat;
#else
   (void)obj;
   (void)lon;
   (void)lat;
#endif
}

EAPI void
elm_map_paused_set(Evas_Object *obj,
                   Eina_Bool paused)
{
#ifdef HAVE_ELEMENTARY_ECORE_CON
   ELM_MAP_CHECK(obj);
   ELM_MAP_DATA_GET(obj, sd);

   if (sd->paused == !!paused) return;
   sd->paused = !!paused;

   if (IS_EXTERNAL_ENGINE(sd))
     return;
   else
     {
        if (paused)
          {
             if (sd->zoom_animator)
               {
                  if (sd->zoom_animator) ecore_animator_del(sd->zoom_animator);
                  sd->zoom_animator = NULL;
                  sd->zoom_detail = sd->zoom;
                  _map_pan_zoom_do(sd, sd->zoom);
               }
             edje_object_signal_emit(ELM_WIDGET_DATA(sd)->resize_obj,
                                     "elm,state,busy,stop", "elm");
          }
        else
          {
             if (sd->download_num >= 1)
                edje_object_signal_emit(ELM_WIDGET_DATA(sd)->resize_obj,
                                        "elm,state,busy,start", "elm");
          }
     }

#else
   (void)obj;
   (void)paused;
#endif
}

EAPI Eina_Bool
elm_map_paused_get(const Evas_Object *obj)
{
#ifdef HAVE_ELEMENTARY_ECORE_CON
   ELM_MAP_CHECK(obj) EINA_FALSE;
   ELM_MAP_DATA_GET(obj, sd);

   return sd->paused;
#else
   (void)obj;
   return EINA_FALSE;
#endif
}

EAPI void
elm_map_rotate_set(Evas_Object *obj,
                   double degree,
                   Evas_Coord cx,
                   Evas_Coord cy)
{
#ifdef HAVE_ELEMENTARY_ECORE_CON
   ELM_MAP_CHECK(obj);
   ELM_MAP_DATA_GET(obj, sd);

   sd->engine->rotate(obj, degree, cx, cy, degree);
#else
   (void)obj;
   (void)degree;
   (void)cx;
   (void)cy;
#endif
}

EAPI void
elm_map_rotate_get(const Evas_Object *obj,
                   double *degree,
                   Evas_Coord *cx,
                   Evas_Coord *cy)
{
#ifdef HAVE_ELEMENTARY_ECORE_CON
   ELM_MAP_CHECK(obj);
   ELM_MAP_DATA_GET(obj, sd);

   sd->engine->rotate_get(obj, degree, cx, cy);
#else
   (void)obj;
   (void)degree;
   (void)cx;
   (void)cy;
#endif
}

EAPI void
elm_map_perspective_enabled_set(Evas_Object *obj,
                           Eina_Bool enabled)
{
#ifdef HAVE_ELEMENTARY_ECORE_CON
   ELM_MAP_CHECK(obj);
   ELM_MAP_DATA_GET(obj, sd);

   sd->perspective_enabled = !!enabled;
#else
   (void)obj;
   (void)disabled;
#endif
}

EAPI Eina_Bool
elm_map_perspective_enabled_get(const Evas_Object *obj)
{
#ifdef HAVE_ELEMENTARY_ECORE_CON
   ELM_MAP_CHECK(obj) EINA_FALSE;
   ELM_MAP_DATA_GET(obj, sd);

   return sd->perspective_enabled;
#else
   (void)obj;
   return EINA_FALSE;
#endif
}

EAPI void
elm_map_wheel_disabled_set(Evas_Object *obj,
                           Eina_Bool disabled)
{
#ifdef HAVE_ELEMENTARY_ECORE_CON
   ELM_MAP_CHECK(obj);
   ELM_MAP_DATA_GET(obj, sd);

   if (IS_EXTERNAL_ENGINE(sd))
     {
        ERR("Engine(%s) do not support this function", sd->engine->name);
        return;
     }

   if ((!sd->wheel_disabled) && (disabled))
     evas_object_event_callback_del_full
       (obj, EVAS_CALLBACK_MOUSE_WHEEL, _mouse_wheel_cb, sd);
   else if ((sd->wheel_disabled) && (!disabled))
     evas_object_event_callback_add
       (obj, EVAS_CALLBACK_MOUSE_WHEEL, _mouse_wheel_cb, sd);
   sd->wheel_disabled = !!disabled;
#else
   (void)obj;
   (void)disabled;
#endif
}

EAPI Eina_Bool
elm_map_wheel_disabled_get(const Evas_Object *obj)
{
#ifdef HAVE_ELEMENTARY_ECORE_CON
   ELM_MAP_CHECK(obj) EINA_FALSE;
   ELM_MAP_DATA_GET(obj, sd);

   return sd->wheel_disabled;
#else
   (void)obj;
   return EINA_FALSE;
#endif
}

EAPI void
elm_map_tile_load_status_get(const Evas_Object *obj,
                             int *try_num,
                             int *finish_num)
{
#ifdef HAVE_ELEMENTARY_ECORE_CON
   ELM_MAP_CHECK(obj);
   ELM_MAP_DATA_GET(obj, sd);

   if (IS_EXTERNAL_ENGINE(sd))
     {
        ERR("Engine(%s) do not support this function", sd->engine->name);
        return;
     }

   if (try_num) *try_num = sd->try_num;
   if (finish_num) *finish_num = sd->finish_num;
#else
   (void)obj;
   (void)try_num;
   (void)finish_num;
#endif
}

EAPI void
elm_map_canvas_to_region_convert(const Evas_Object *obj,
                                 Evas_Coord x,
                                 Evas_Coord y,
                                 double *lon,
                                 double *lat)
{
#ifdef HAVE_ELEMENTARY_ECORE_CON
   ELM_MAP_CHECK(obj);
   ELM_MAP_DATA_GET(obj, sd);
   EINA_SAFETY_ON_NULL_RETURN(lon);
   EINA_SAFETY_ON_NULL_RETURN(lat);

   sd->engine->canvas_to_region(obj, x, y, lon, lat);
#else
   (void)obj;
   (void)x;
   (void)y;
   (void)lon;
   (void)lat;
#endif
}

EAPI void
elm_map_region_to_canvas_convert(const Evas_Object *obj,
                                 double lon,
                                 double lat,
                                 Evas_Coord *x,
                                 Evas_Coord *y)
{
#ifdef HAVE_ELEMENTARY_ECORE_CON
   ELM_MAP_CHECK(obj);
   ELM_MAP_DATA_GET(obj, sd);
   EINA_SAFETY_ON_NULL_RETURN(x);
   EINA_SAFETY_ON_NULL_RETURN(y);

   sd->engine->region_to_canvas(obj, lon, lat, x, y);
#else
   (void)obj;
   (void)lon;
   (void)lat;
   (void)x;
   (void)y;
#endif
}

EAPI void
elm_map_user_agent_set(Evas_Object *obj,
                       const char *user_agent)
{
#ifdef HAVE_ELEMENTARY_ECORE_CON
   ELM_MAP_CHECK(obj);
   ELM_MAP_DATA_GET(obj, sd);
   EINA_SAFETY_ON_NULL_RETURN(user_agent);

   if (IS_EXTERNAL_ENGINE(sd))
     {
        ERR("Engine(%s) do not support this function", sd->engine->name);
        return;
     }

   eina_stringshare_replace(&sd->user_agent, user_agent);

   if (!sd->ua) sd->ua = eina_hash_string_small_new(NULL);
   eina_hash_set(sd->ua, "User-Agent", sd->user_agent);
#else
   (void)obj;
   (void)user_agent;
#endif
}

EAPI const char *
elm_map_user_agent_get(const Evas_Object *obj)
{
#ifdef HAVE_ELEMENTARY_ECORE_CON
   ELM_MAP_CHECK(obj) NULL;
   ELM_MAP_DATA_GET(obj, sd);

   return sd->user_agent;
#else
   (void)obj;
   return NULL;
#endif
}

EAPI Eina_Bool
elm_map_engine_set(Evas_Object *obj,
                   const char *engine_name)
{
   ELM_MAP_CHECK(obj) EINA_FALSE;
   ELM_MAP_DATA_GET(obj, sd);
   EINA_SAFETY_ON_NULL_RETURN_VAL(engine_name, EINA_FALSE);

   Source_Engine *engine = NULL;
   Source_Engine *e;
   Eina_List *l;
   Evas_Object *layout;

   if (sd->engine && !strcmp(sd->engine->name, engine_name))
      return EINA_FALSE;

   EINA_LIST_FOREACH(sd->engines, l, e)
     {
        if (!strcmp(e->name, engine_name))
          {
             engine = e;
             break;
          }
     }
   if (!engine)
     {
        WRN("Engine name (%s) is not found", engine_name);
        return EINA_FALSE;
     }
   layout = engine->add(obj);
   if (!layout)
     {
        engine->del(obj);
        WRN("%s engine can not be added", engine->name);
        return EINA_FALSE;
     }
   sd->engine->del(obj);
   sd->engine = engine;

   if (sd->engine->key)
      sd->engine->key_set(obj, sd->engine->key);

   if (sd->engine->zoom_max < sd->zoom)
     sd->zoom = sd->engine->zoom_max;
   else if (sd->engine->zoom_min > sd->zoom)
     sd->zoom = sd->engine->zoom_min;

   if (sd->engine->zoom_max < sd->zoom_max)
     sd->zoom_max = sd->engine->zoom_max;
   if (sd->engine->zoom_min > sd->zoom_min)
     sd->zoom_min = sd->engine->zoom_min;

   evas_object_hide(ELM_WIDGET_DATA(sd)->resize_obj);
   elm_widget_resize_object_set(obj, layout, EINA_TRUE);
   sd->engine->zoom(obj, sd->zoom, 0);
   return EINA_TRUE;
}

EAPI const char *
elm_map_engine_get(const Evas_Object *obj)
{
   ELM_MAP_CHECK(obj) NULL;
   ELM_MAP_DATA_GET(obj, sd);

   EINA_SAFETY_ON_NULL_RETURN_VAL(sd->src_tile, NULL);

   return sd->engine->name;
}

EAPI const char **
elm_map_engines_get(const Evas_Object *obj)
{
   ELM_MAP_CHECK(obj) NULL;
   ELM_MAP_DATA_GET(obj, sd);

   return sd->engine_names;
}

EAPI void
elm_map_source_set(Evas_Object *obj,
                   Elm_Map_Source_Type type,
                   const char *source_name)
{
#ifdef HAVE_ELEMENTARY_ECORE_CON
   ELM_MAP_CHECK(obj);
   ELM_MAP_DATA_GET(obj, sd);
   EINA_SAFETY_ON_NULL_RETURN(source_name);

   if (IS_EXTERNAL_ENGINE(sd))
     {
        ERR("Current engine is not %s", INTERNAL_ENGINE_NAME);
        return;
     }

   if (type == ELM_MAP_SOURCE_TYPE_TILE)
      _source_tile_set(sd, source_name);
   else if (type == ELM_MAP_SOURCE_TYPE_ROUTE)
     _source_route_set(sd, source_name);
   else if (type == ELM_MAP_SOURCE_TYPE_NAME)
     _source_name_set(sd, source_name);
   else ERR("Not supported map source type: %d", type);

#else
   (void)obj;
   (void)type;
   (void)source_name;
#endif
}

EAPI const char *
elm_map_source_get(const Evas_Object *obj,
                   Elm_Map_Source_Type type)
{
#ifdef HAVE_ELEMENTARY_ECORE_CON
   ELM_MAP_CHECK(obj) NULL;
   ELM_MAP_DATA_GET(obj, sd);

   if (IS_EXTERNAL_ENGINE(sd))
     {
        ERR("Current engine is not %s", INTERNAL_ENGINE_NAME);
        return NULL;
     }
   EINA_SAFETY_ON_NULL_RETURN_VAL(sd->src_tile, NULL);

   if (type == ELM_MAP_SOURCE_TYPE_TILE) return sd->src_tile->name;
   else if (type == ELM_MAP_SOURCE_TYPE_ROUTE)
     return sd->src_route->name;
   else if (type == ELM_MAP_SOURCE_TYPE_NAME)
     return sd->src_name->name;
   else ERR("Not supported map source type: %d", type);

   return NULL;
#else
   (void)obj;
   (void)type;
   return NULL;
#endif
}

EAPI const char **
elm_map_sources_get(const Evas_Object *obj,
                    Elm_Map_Source_Type type)
{
#ifdef HAVE_ELEMENTARY_ECORE_CON
   ELM_MAP_CHECK(obj) NULL;
   ELM_MAP_DATA_GET(obj, sd);

   if (type == ELM_MAP_SOURCE_TYPE_TILE) return sd->src_tile_names;
   else if (type == ELM_MAP_SOURCE_TYPE_ROUTE)
     return sd->src_route_names;
   else if (type == ELM_MAP_SOURCE_TYPE_NAME)
     return sd->src_name_names;
   else ERR("Not supported map source type: %d", type);

   return NULL;
#else
   (void)obj;
   (void)type;
   return NULL;
#endif
}

EAPI Elm_Map_Route *
elm_map_route_add(Evas_Object *obj,
                  Elm_Map_Route_Type type,
                  Elm_Map_Route_Method method,
                  double flon,
                  double flat,
                  double tlon,
                  double tlat,
                  Elm_Map_Route_Cb route_cb,
                  void *data)
{
#ifdef HAVE_ELEMENTARY_ECORE_CON
   char *url;
   char *type_name;
   Elm_Map_Route *route;
   char fname[PATH_MAX], fname2[PATH_MAX];

   ELM_MAP_CHECK(obj) NULL;
   ELM_MAP_DATA_GET(obj, sd);

   EINA_SAFETY_ON_NULL_RETURN_VAL(sd->src_route, NULL);

   if (IS_EXTERNAL_ENGINE(sd))
       return _map_custom_route_add(obj, type, method, flon, flat,
                                     tlon, tlat, route_cb, data);

   {
      const char *cachedir;

#ifdef ELM_EFREET
      snprintf(fname, sizeof(fname), "%s" CACHE_ROUTE_ROOT,
               efreet_cache_home_get());
      (void)cachedir;
#else
      cachedir = getenv("XDG_CACHE_HOME");
      snprintf(fname, sizeof(fname), "%s/%s" CACHE_ROUTE_ROOT, getenv("HOME"),
               cachedir ? : "/.config");
#endif
      if (!ecore_file_exists(fname)) ecore_file_mkpath(fname);
   }

   if (type == ELM_MAP_ROUTE_TYPE_MOTOCAR)
     type_name = strdup(ROUTE_TYPE_MOTORCAR);
   else if (type == ELM_MAP_ROUTE_TYPE_BICYCLE)
     type_name = strdup(ROUTE_TYPE_BICYCLE);
   else if (type == ELM_MAP_ROUTE_TYPE_FOOT)
     type_name = strdup(ROUTE_TYPE_FOOT);
   else type_name = NULL;

   url = sd->src_route->url_cb(obj, type_name, method, flon, flat, tlon, tlat);
   if (!url)
     {
        ERR("Route URL is NULL");
        if (type_name) free(type_name);
        return NULL;
     }
   if (type_name) free(type_name);

   route = ELM_NEW(Elm_Map_Route);
   route->wsd = sd;
   snprintf(fname2, sizeof(fname2), "%s/%d", fname, rand());
   route->fname = strdup(fname2);
   route->type = type;
   route->method = method;
   route->flon = flon;
   route->flat = flat;
   route->tlon = tlon;
   route->tlat = tlat;
   route->cb = route_cb;
   route->data = data;

   if (!ecore_file_download_full(url, route->fname, _route_cb, NULL, route,
                                 &(route->job), sd->ua) || !(route->job))
     {
        ERR("Can't request Route from %s to %s", url, route->fname);
        free(route->fname);
        free(route);
        return NULL;
     }
   INF("Route requested from %s to %s", url, route->fname);
   free(url);

   sd->routes = eina_list_append(sd->routes, route);
   evas_object_smart_callback_call
     (ELM_WIDGET_DATA(sd)->obj, SIG_ROUTE_LOAD, route);
   edje_object_signal_emit(ELM_WIDGET_DATA(sd)->resize_obj,
                           "elm,state,busy,start", "elm");
   return route;
#else
   (void)obj;
   (void)type;
   (void)method;
   (void)flon;
   (void)flat;
   (void)tlon;
   (void)tlat;
   (void)route_cb;
   (void)data;
   return NULL;
#endif
}

EAPI void
elm_map_route_del(Elm_Map_Route *route)
{
#ifdef HAVE_ELEMENTARY_ECORE_CON
   Path_Waypoint *w;
   Path_Node *n;

   EINA_SAFETY_ON_NULL_RETURN(route);
   EINA_SAFETY_ON_NULL_RETURN(route->wsd);
   ELM_MAP_CHECK(ELM_WIDGET_DATA(route->wsd)->obj);

   ELM_MAP_DATA_GET(ELM_WIDGET_DATA(route->wsd)->obj, sd);

   if (IS_EXTERNAL_ENGINE(sd))
     {
        _map_custom_route_del(route);
        return;
     }

   if (route->job) ecore_file_download_abort(route->job);

   EINA_LIST_FREE (route->waypoint, w)
     {
        if (w->point) eina_stringshare_del(w->point);
        free(w);
     }

   EINA_LIST_FREE (route->nodes, n)
     {
        if (n->pos.address) eina_stringshare_del(n->pos.address);
        free(n);
     }

   if (route->fname)
     {
        ecore_file_remove(route->fname);
        free(route->fname);
     }

   route->wsd->routes = eina_list_remove(route->wsd->routes, route);
   free(route);
#else
   (void)route;
#endif
}

EAPI double
elm_map_route_distance_get(const Elm_Map_Route *route)
{
#ifdef HAVE_ELEMENTARY_ECORE_CON
   EINA_SAFETY_ON_NULL_RETURN_VAL(route, 0.0);

   if (IS_EXTERNAL_ENGINE(route->wsd))
     {
        ERR("Engine(%s) do not support this function", route->wsd->engine->name);
        return 0.0;
     }

   return route->info.distance;
#else
   (void)route;
   return 0.0;
#endif
}

EAPI const char *
elm_map_route_node_get(const Elm_Map_Route *route)
{
#ifdef HAVE_ELEMENTARY_ECORE_CON
   EINA_SAFETY_ON_NULL_RETURN_VAL(route, NULL);

   if (IS_EXTERNAL_ENGINE(route->wsd))
     {
        ERR("Engine(%s) do not support this function", route->wsd->engine->name);
        return NULL;
     }

   return route->info.nodes;
#else
   (void)route;
   return NULL;
#endif
}

EAPI const char *
elm_map_route_waypoint_get(const Elm_Map_Route *route)
{
#ifdef HAVE_ELEMENTARY_ECORE_CON
   EINA_SAFETY_ON_NULL_RETURN_VAL(route, NULL);

   if (IS_EXTERNAL_ENGINE(route->wsd))
     {
        ERR("Engine(%s) do not support this function", route->wsd->engine->name);
        return NULL;
     }

   return route->info.waypoints;
#else
   (void)route;
   return NULL;
#endif
}

EAPI Elm_Map_Name *
elm_map_name_add(const Evas_Object *obj,
                 const char *address,
                 double lon,
                 double lat,
                 Elm_Map_Name_Cb name_cb,
                 void *data)
{
#ifdef HAVE_ELEMENTARY_ECORE_CON
   ELM_MAP_CHECK(obj) NULL;

   if (address)
     return _name_request(obj, ELM_MAP_NAME_METHOD_SEARCH, address, 0, 0,
                          name_cb, data);
   else
     return _name_request(obj, ELM_MAP_NAME_METHOD_REVERSE, NULL, lon, lat,
                          name_cb, data);
#else
   (void)obj;
   (void)address;
   (void)lon;
   (void)lat;
   (void)name_cb;
   (void)data;
   return NULL;
#endif
}

EAPI void
elm_map_name_search(const Evas_Object *obj,
                  const char *address,
                  Elm_Map_Name_List_Cb name_cb,
                  void *data)
{
#ifdef HAVE_ELEMENTARY_ECORE_CON
   ELM_MAP_CHECK(obj);
   if (address)
     _name_list_request(obj, ELM_MAP_NAME_METHOD_SEARCH, address, 0, 0,
                        name_cb, data);
#else
   (void) obj;
   (void) address;
   (void) name_cb;
   (void) data;
#endif
}

EAPI void
elm_map_name_del(Elm_Map_Name *name)
{
#ifdef HAVE_ELEMENTARY_ECORE_CON
   EINA_SAFETY_ON_NULL_RETURN(name);
   EINA_SAFETY_ON_NULL_RETURN(name->wsd);
   ELM_MAP_CHECK(ELM_WIDGET_DATA(name->wsd)->obj);

   if (name->job) ecore_file_download_abort(name->job);
   if (name->address) free(name->address);
   if (name->fname)
     {
        ecore_file_remove(name->fname);
        free(name->fname);
     }

   name->wsd->names = eina_list_remove(name->wsd->names, name);
   free(name);
#else
   (void)name;
#endif
}

EAPI const char *
elm_map_name_address_get(const Elm_Map_Name *name)
{
#ifdef HAVE_ELEMENTARY_ECORE_CON
   EINA_SAFETY_ON_NULL_RETURN_VAL(name, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(name->wsd, NULL);
   ELM_MAP_CHECK(ELM_WIDGET_DATA(name->wsd)->obj) NULL;

   return name->address;
#else
   (void)name;
   return NULL;
#endif
}

EAPI void
elm_map_name_region_get(const Elm_Map_Name *name,
                        double *lon,
                        double *lat)
{
#ifdef HAVE_ELEMENTARY_ECORE_CON
   EINA_SAFETY_ON_NULL_RETURN(name);
   EINA_SAFETY_ON_NULL_RETURN(name->wsd);
   ELM_MAP_CHECK(ELM_WIDGET_DATA(name->wsd)->obj);

   if (lon) *lon = name->lon;
   if (lat) *lat = name->lat;
#else
   (void)name;
   (void)lon;
   (void)lat;
#endif
}

/*Used only for map image capture using Nokia engine. Add icons to nokia engine
before rendering
*/
static void
_map_engine_icon_add(Elm_Map_Smart_Data *sd, Overlay_Default *ovl)
{
  int w, h;
  char *buffer;
  Evas_Object *img = NULL;
  void *iconhandle = NULL;
  if (ovl->icon)
     img = elm_image_object_get(ovl->icon);
  else if (ovl->clas_icon)
     img = elm_image_object_get(ovl->clas_icon);
  else if (ovl->content)
         {
           if (evas_object_smart_type_check(ovl->content, "elm_icon") |
                    evas_object_smart_type_check(ovl->content, "elm_image"))
                 img = elm_image_object_get(ovl->content);
         }

  if (!img) return;
     buffer = (char*)evas_object_image_data_get(img, EINA_FALSE);

  evas_object_image_size_get(img, &w, &h);
  ELM_MAP_ENG_OBJECT_CREATE(sd->engine->icon_add, iconhandle,  ELM_WIDGET_DATA(sd)->obj, buffer, w, h, ovl->lon, ovl->lat);

  if (iconhandle)
    sd->icons_map_capture = eina_list_append(sd->icons_map_capture, iconhandle);
}

/*FIXME: Currently overlay icons do not get displayed in map capture,
  hence make the Nokia engine itself render the icons just for
  map capture case. Only default icons are rendered
*/
static void
_map_capture_clean(Evas_Object *obj)
{
  Eina_List *l, *ll;
  void *iconhandle;

  ELM_MAP_DATA_GET(obj, sd);

  EINA_LIST_FOREACH_SAFE(sd->icons_map_capture, l, ll, iconhandle)
  ELM_MAP_ENG_OBJECT_DELETE(sd->engine->icon_del,  ELM_WIDGET_DATA(sd)->obj, iconhandle);

  eina_list_free(sd->icons_map_capture);
  sd->icons_map_capture = NULL;

  evas_object_data_del(obj, ".mapcapturecallback");
  evas_object_data_del(obj, ".mapcaptureuserdata");
}

static void
_map_capture_cb(Evas_Object *img, void *data)
{
   Evas_Object *obj = (Evas_Object*)data;
   if (!obj) return;

   Elm_Map_Capture_Result_Cb result_cb = (Elm_Map_Capture_Result_Cb) evas_object_data_get(obj, ".mapcapturecallback");
   void *userdata = evas_object_data_get(obj, ".mapcaptureuserdata");

   if (result_cb)
       result_cb(img, userdata);
   _map_capture_clean(obj);
}

EAPI Eina_Bool
elm_map_image_get(Evas_Object *obj, int w, int h,
                  Elm_Map_Capture_Result_Cb result_cb,
                  void *data)
{
  Eina_List *l;
  Elm_Map_Overlay *ovl;
  Eina_Bool ret = EINA_FALSE;

  ELM_MAP_DATA_GET(obj, sd);

  if (w <=0 || h <=0) return ret;
  if (!result_cb) return ret;
  if (!sd->engine->map_capture) return ret;
  if (!IS_EXTERNAL_ENGINE(sd)) return ret;

 /*add all the icons using the nokia engine api
*/
  if (sd->engine->icon_add && sd->engine->icon_del)
    {
      EINA_LIST_FOREACH(sd->overlays, l, ovl)
         if (ovl->type == ELM_MAP_OVERLAY_TYPE_DEFAULT)
             _map_engine_icon_add(sd, ovl->ovl);
    }

  evas_object_data_set(obj, ".mapcapturecallback", (void*)result_cb);
  evas_object_data_set(obj, ".mapcaptureuserdata", data);

  /*call async nokia map capture image now*/
  ret = sd->engine->map_capture(ELM_WIDGET_DATA(sd)->obj, 0, 0, w, h, _map_capture_cb);
  if (!ret)
      _map_capture_clean(obj);

  return ret;
}

EAPI Elm_Map_Overlay *
elm_map_overlay_add(Evas_Object *obj,
                    double lon,
                    double lat)
{
#ifdef HAVE_ELEMENTARY_ECORE_CON
   Elm_Map_Overlay *overlay;

   ELM_MAP_CHECK(obj) NULL;
   ELM_MAP_DATA_GET(obj, sd);

   overlay = ELM_NEW(Elm_Map_Overlay);
   overlay->wsd = sd;
   overlay->type = ELM_MAP_OVERLAY_TYPE_DEFAULT;
   overlay->c.r = 0x87;
   overlay->c.g = 0xce;
   overlay->c.b = 0xeb;
   overlay->c.a = 255;
   overlay->engine_obj = NULL;
   overlay->ovl = _overlay_default_new(overlay, lon, lat, overlay->c, 1);
   overlay->grp = _overlay_group_new(sd);
   sd->overlays = eina_list_append(sd->overlays, overlay);

   if (IS_EXTERNAL_ENGINE(sd))
     evas_object_smart_changed(ELM_WIDGET_DATA(sd)->obj);
   else
     evas_object_smart_changed(sd->pan_obj);

   return overlay;
#else
   (void)obj;
   (void)lon;
   (void)lat;
   return NULL;
#endif
}

EAPI Eina_List *
elm_map_overlays_get(Evas_Object *obj)
{
#ifdef HAVE_ELEMENTARY_ECORE_CON
   Eina_List *l;
   Elm_Map_Overlay *ovl;

   ELM_MAP_CHECK(obj) NULL;
   ELM_MAP_DATA_GET(obj, sd);

   eina_list_free(sd->all_overlays);
   sd->all_overlays = NULL;

   EINA_LIST_FOREACH(sd->overlays, l, ovl)
     sd->all_overlays = eina_list_append(sd->all_overlays, ovl);
   EINA_LIST_FOREACH(sd->group_overlays, l, ovl)
     sd->all_overlays = eina_list_append(sd->all_overlays, ovl);

   return sd->all_overlays;
#else
   (void)obj;
   return NULL;
#endif
}

EAPI void
elm_map_overlay_del(Elm_Map_Overlay *overlay)
{
#ifdef HAVE_ELEMENTARY_ECORE_CON
   EINA_SAFETY_ON_NULL_RETURN(overlay);
   EINA_SAFETY_ON_NULL_RETURN(overlay->wsd);
   ELM_MAP_CHECK(ELM_WIDGET_DATA(overlay->wsd)->obj);

   evas_object_smart_callback_call
     (ELM_WIDGET_DATA(overlay->wsd)->obj, SIG_OVERLAY_DEL, overlay);
   if (overlay->del_cb)
     overlay->del_cb
       (overlay->del_cb_data, ELM_WIDGET_DATA(overlay->wsd)->obj, overlay);

   if (overlay->grp)
     {
        if (overlay->grp->klass)
          elm_map_overlay_class_remove(overlay->grp->klass, overlay);
        _overlay_group_free(overlay->grp);
     }

   if (overlay->type == ELM_MAP_OVERLAY_TYPE_DEFAULT)
     {
        _overlay_default_free(overlay->ovl);
        if (overlay->engine_obj)
          ELM_MAP_ENG_OBJECT_DELETE(overlay->wsd->engine->icon_del,  ELM_WIDGET_DATA(overlay->wsd)->obj, overlay->engine_obj);
     }
   else if (overlay->type == ELM_MAP_OVERLAY_TYPE_BUBBLE)
     _overlay_bubble_free(overlay->ovl);
   else if (overlay->type == ELM_MAP_OVERLAY_TYPE_CLASS)
     {
        _overlay_class_free(overlay->ovl);
        if (overlay->engine_obj)
          ELM_MAP_ENG_OBJECT_DELETE(overlay->wsd->engine->group_del,  ELM_WIDGET_DATA(overlay->wsd)->obj, overlay->engine_obj);
     }
   else if (overlay->type == ELM_MAP_OVERLAY_TYPE_ROUTE)
     {
	 if (!IS_EXTERNAL_ENGINE(overlay->wsd))
	   {
          _overlay_route_free(overlay->ovl);
	   }
	 else
	   {
          if (overlay->wsd->engine->route_del)
          overlay->wsd->engine->route_del(ELM_WIDGET_DATA(overlay->wsd)->obj, overlay->engine_obj);
	      overlay->wsd->overlays = eina_list_remove(overlay->wsd->overlays, overlay);
	      free(overlay);
	      return;
	   }
     }
   else if (overlay->type == ELM_MAP_OVERLAY_TYPE_LINE)
     _overlay_line_free(overlay->ovl);
   else if (overlay->type == ELM_MAP_OVERLAY_TYPE_POLYLINE)
     {
        _overlay_polyline_free(overlay->ovl);
	 if (IS_EXTERNAL_ENGINE(overlay->wsd))
	   {
             if (overlay->wsd->engine->polyline_del)
               overlay->wsd->engine->polyline_del(ELM_WIDGET_DATA(overlay->wsd)->obj, overlay->engine_obj);
	      overlay->wsd->overlays = eina_list_remove(overlay->wsd->overlays, overlay);
	      free(overlay);
	      return;
          }
     }
   else if (overlay->type == ELM_MAP_OVERLAY_TYPE_POLYGON)
     _overlay_polygon_free(overlay->ovl);
   else if (overlay->type == ELM_MAP_OVERLAY_TYPE_CIRCLE)
     _overlay_circle_free(overlay->ovl);
   else if (overlay->type == ELM_MAP_OVERLAY_TYPE_SCALE)
     _overlay_scale_free(overlay->ovl);
   else ERR("Invalid overlay type: %d", overlay->type);

   overlay->wsd->overlays = eina_list_remove(overlay->wsd->overlays, overlay);

   if (IS_EXTERNAL_ENGINE(overlay->wsd))
     evas_object_smart_changed(ELM_WIDGET_DATA(overlay->wsd)->obj);
   else
     evas_object_smart_changed(overlay->wsd->pan_obj);

   free(overlay);
#else
   (void)overlay;
#endif
}

EAPI Elm_Map_Overlay_Type
elm_map_overlay_type_get(const Elm_Map_Overlay *overlay)
{
#ifdef HAVE_ELEMENTARY_ECORE_CON
   EINA_SAFETY_ON_NULL_RETURN_VAL(overlay, ELM_MAP_OVERLAY_TYPE_NONE);
   EINA_SAFETY_ON_NULL_RETURN_VAL(overlay->wsd, ELM_MAP_OVERLAY_TYPE_NONE);
   ELM_MAP_CHECK(ELM_WIDGET_DATA(overlay->wsd)->obj) ELM_MAP_OVERLAY_TYPE_NONE;

   return overlay->type;
#else
   (void)overlay;
   return ELM_MAP_OVERLAY_TYPE_NONE;
#endif
}

EAPI void
elm_map_overlay_data_set(Elm_Map_Overlay *overlay,
                         void *data)
{
#ifdef HAVE_ELEMENTARY_ECORE_CON
   EINA_SAFETY_ON_NULL_RETURN(overlay);
   EINA_SAFETY_ON_NULL_RETURN(overlay->wsd);
   ELM_MAP_CHECK(ELM_WIDGET_DATA(overlay->wsd)->obj);

   overlay->data = data;
#else
   (void)overlay;
   (void)data;
#endif
}

EAPI void *
elm_map_overlay_data_get(const Elm_Map_Overlay *overlay)
{
#ifdef HAVE_ELEMENTARY_ECORE_CON
   EINA_SAFETY_ON_NULL_RETURN_VAL(overlay, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(overlay->wsd, NULL);
   ELM_MAP_CHECK(ELM_WIDGET_DATA(overlay->wsd)->obj) NULL;

   return overlay->data;
#else
   (void)overlay;
   return NULL;
#endif
}

EAPI void
elm_map_overlay_hide_set(Elm_Map_Overlay *overlay,
                         Eina_Bool hide)
{
#ifdef HAVE_ELEMENTARY_ECORE_CON
   EINA_SAFETY_ON_NULL_RETURN(overlay);
   EINA_SAFETY_ON_NULL_RETURN(overlay->wsd);
   ELM_MAP_CHECK(ELM_WIDGET_DATA(overlay->wsd)->obj);

   if (overlay->hide == !!hide) return;
   overlay->hide = hide;

   if (IS_EXTERNAL_ENGINE(overlay->wsd))
     evas_object_smart_changed(ELM_WIDGET_DATA(overlay->wsd)->obj);
   else
     evas_object_smart_changed(overlay->wsd->pan_obj);

#else
   (void)overlay;
   (void)hide;
#endif
}

EAPI Eina_Bool
elm_map_overlay_hide_get(const Elm_Map_Overlay *overlay)
{
#ifdef HAVE_ELEMENTARY_ECORE_CON
   EINA_SAFETY_ON_NULL_RETURN_VAL(overlay, EINA_FALSE);
   EINA_SAFETY_ON_NULL_RETURN_VAL(overlay->wsd, EINA_FALSE);
   ELM_MAP_CHECK(ELM_WIDGET_DATA(overlay->wsd)->obj) EINA_FALSE;

   return overlay->hide;
#else
   (void)overlay;
   return EINA_FALSE;
#endif
}

EAPI void
elm_map_overlay_displayed_zoom_min_set(Elm_Map_Overlay *overlay,
                                       int zoom)
{
#ifdef HAVE_ELEMENTARY_ECORE_CON
   EINA_SAFETY_ON_NULL_RETURN(overlay);
   EINA_SAFETY_ON_NULL_RETURN(overlay->wsd);
   ELM_MAP_CHECK(ELM_WIDGET_DATA(overlay->wsd)->obj);

   overlay->zoom_min = zoom;

   if (IS_EXTERNAL_ENGINE(overlay->wsd))
     {
        if (overlay->engine_obj)
          {
             overlay->wsd->engine->object_visibility_range(ELM_WIDGET_DATA(overlay->wsd)->obj, overlay->engine_obj, overlay->zoom_min, -1);
          }
     }

#else
   (void)overlay;
   (void)zoom;
#endif
}

EAPI int
elm_map_overlay_displayed_zoom_min_get(const Elm_Map_Overlay *overlay)
{
#ifdef HAVE_ELEMENTARY_ECORE_CON
   EINA_SAFETY_ON_NULL_RETURN_VAL(overlay, 0);
   EINA_SAFETY_ON_NULL_RETURN_VAL(overlay->wsd, 0);
   ELM_MAP_CHECK(ELM_WIDGET_DATA(overlay->wsd)->obj) 0;

   return overlay->zoom_min;
#else
   (void)overlay;
   return 0;
#endif
}

EAPI void
elm_map_overlay_paused_set(Elm_Map_Overlay *overlay,
                           Eina_Bool paused)
{
#ifdef HAVE_ELEMENTARY_ECORE_CON
   EINA_SAFETY_ON_NULL_RETURN(overlay);
   EINA_SAFETY_ON_NULL_RETURN(overlay->wsd);
   ELM_MAP_CHECK(ELM_WIDGET_DATA(overlay->wsd)->obj);

   if (IS_EXTERNAL_ENGINE(overlay->wsd))
     {
        ERR("Engine(%s) do not support this function", overlay->wsd->engine->name);
        return;
     }

   if (overlay->paused == !!paused) return;
   overlay->paused = paused;

#else
   (void)overlay;
   (void)paused;
#endif
}

EAPI Eina_Bool
elm_map_overlay_paused_get(const Elm_Map_Overlay *overlay)
{
#ifdef HAVE_ELEMENTARY_ECORE_CON
   EINA_SAFETY_ON_NULL_RETURN_VAL(overlay, EINA_FALSE);
   EINA_SAFETY_ON_NULL_RETURN_VAL(overlay->wsd, EINA_FALSE);
   ELM_MAP_CHECK(ELM_WIDGET_DATA(overlay->wsd)->obj) EINA_FALSE;

   return overlay->paused;
#else
   (void)overlay;
   return EINA_FALSE;
#endif
}

EAPI Eina_Bool
elm_map_overlay_visible_get(const Elm_Map_Overlay *overlay)
{
#ifdef HAVE_ELEMENTARY_ECORE_CON
   EINA_SAFETY_ON_NULL_RETURN_VAL(overlay, EINA_FALSE);
   EINA_SAFETY_ON_NULL_RETURN_VAL(overlay->wsd, EINA_FALSE);
   ELM_MAP_CHECK(ELM_WIDGET_DATA(overlay->wsd)->obj) EINA_FALSE;

   return overlay->visible;
#else
   (void)overlay;
   return EINA_FALSE;
#endif
}

static void
_zoom_level_determined_cb(Evas_Object *obj)
{
   ELM_MAP_CHECK(obj);
   ELM_MAP_DATA_GET(obj, sd);
   sd->zoom_detail = sd->engine->zoom_level_get(obj);
   sd->zoom = ROUND(sd->zoom_detail);
   INF("zoom level is determined by engine:%f", sd->zoom_detail);
}

EAPI void
elm_map_overlay_show(Elm_Map_Overlay *overlay)
{
#ifdef HAVE_ELEMENTARY_ECORE_CON
   EINA_SAFETY_ON_NULL_RETURN(overlay);
   EINA_SAFETY_ON_NULL_RETURN(overlay->wsd);
   ELM_MAP_CHECK(ELM_WIDGET_DATA(overlay->wsd)->obj);

   Elm_Map_Smart_Data *sd;
   sd = overlay->wsd;
   double lon, lat, max_lon, min_lon, max_lat, min_lat;

   if (overlay->type == ELM_MAP_OVERLAY_TYPE_DEFAULT)
     {
        Overlay_Default *ovl = overlay->ovl;

        elm_map_region_show
          (ELM_WIDGET_DATA(sd)->obj, ovl->lon, ovl->lat);
     }
   else if (overlay->type == ELM_MAP_OVERLAY_TYPE_POLYLINE)
     {
        eina_list_free(sd->calc_job.overlays);
        sd->calc_job.overlays = eina_list_append(sd->calc_job.overlays, overlay);
        if (!IS_EXTERNAL_ENGINE(sd))
          {
             sd->calc_job.overlays_changed = EINA_TRUE;
             evas_object_smart_changed(sd->pan_obj);
          }
        else
         {
            _region_max_min_get(sd->calc_job.overlays, &max_lon, &min_lon, &max_lat, &min_lat);
            sd->engine->show_area(ELM_WIDGET_DATA(sd)->obj, max_lon, min_lon, max_lat, min_lat, 0, _zoom_level_determined_cb);
         }
     }
   else if (overlay->type == ELM_MAP_OVERLAY_TYPE_BUBBLE)
     {
        Overlay_Bubble *ovl = overlay->ovl;

        elm_map_region_show
          (ELM_WIDGET_DATA(sd)->obj, ovl->lon, ovl->lat);
     }
   else if (overlay->type == ELM_MAP_OVERLAY_TYPE_CLASS)
     {
        Overlay_Class *ovl = overlay->ovl;

        _region_max_min_get(ovl->members, &max_lon, &min_lon, &max_lat, &min_lat);
        lon = (max_lon + min_lon) / 2;
        lat = (max_lat + min_lat) / 2;
        elm_map_region_show(ELM_WIDGET_DATA(sd)->obj, lon, lat);
     }
   else
     {
        ERR("Not supported overlay type: %d", overlay->type);
	    return;
     }

#else
   (void)overlay;
#endif
}

EAPI void
elm_map_overlays_show(Eina_List *overlays)
{
#ifdef HAVE_ELEMENTARY_ECORE_CON
   Elm_Map_Smart_Data *sd;
   Elm_Map_Overlay *overlay;
   Eina_List* overlays_clone = NULL;

   double max_lon, min_lon, max_lat, min_lat;

   EINA_SAFETY_ON_NULL_RETURN(overlays);
   EINA_SAFETY_ON_FALSE_RETURN(eina_list_count(overlays));

   overlay = eina_list_data_get(overlays);
   sd = overlay->wsd;

   overlays_clone = eina_list_clone(overlays);

   if (IS_EXTERNAL_ENGINE(sd))
     {
        _region_max_min_get(overlays_clone, &max_lon, &min_lon, &max_lat, &min_lat);
        sd->engine->show_area(ELM_WIDGET_DATA(sd)->obj, max_lon, min_lon, max_lat, min_lat, 0, _zoom_level_determined_cb);
     }
   else
     {
        sd->calc_job.overlays_changed = EINA_TRUE;
        sd->calc_job.overlays = overlays_clone;
        evas_object_smart_changed(sd->pan_obj);
     }

#else
   (void)overlays;
#endif
}

EAPI void
elm_map_overlay_region_set(Elm_Map_Overlay *overlay,
                           double lon,
                           double lat)
{
#ifdef HAVE_ELEMENTARY_ECORE_CON
   EINA_SAFETY_ON_NULL_RETURN(overlay);
   EINA_SAFETY_ON_NULL_RETURN(overlay->wsd);
   ELM_MAP_CHECK(ELM_WIDGET_DATA(overlay->wsd)->obj);

   char *imgbuffer = NULL;
   int w = 0;
   int h = 0;
   Evas_Object *img = NULL;

   if (overlay->type == ELM_MAP_OVERLAY_TYPE_DEFAULT)
     {
        Overlay_Default *ovl = overlay->ovl;

        ovl->lon = lon;
        ovl->lat = lat;

       if (overlay->engine_obj)
         {
            img = _img_from_icon(evas_object_evas_get(ELM_WIDGET_DATA(overlay->wsd)->obj), ovl->content);

            if (!img)
              {
                 ERR("Can not extract image object from content");
                 return;
              }

            imgbuffer = (char*)evas_object_image_data_get(img, EINA_TRUE);
            if (!imgbuffer)
              {
                 ERR("Cannot extract image buffer from image object");
                 return;
              }

            evas_object_image_size_get(img, &w, &h);

            ELM_MAP_ENG_OBJECT_DELETE(overlay->wsd->engine->icon_del,  ELM_WIDGET_DATA(overlay->wsd)->obj, overlay->engine_obj);

            overlay->engine_obj = NULL;
            ELM_MAP_ENG_OBJECT_CREATE(overlay->wsd->engine->icon_add, overlay->engine_obj,  ELM_WIDGET_DATA(overlay->wsd)->obj, imgbuffer, w, h, ovl->lon, ovl->lat);
         }
     }
   else if (overlay->type == ELM_MAP_OVERLAY_TYPE_BUBBLE)
     {
        Overlay_Bubble *ovl = overlay->ovl;

        ovl->pobj = NULL;
        ovl->lon = lon;
        ovl->lat = lat;
     }
   else
     {
        ERR("Not supported overlay type: %d", overlay->type);
        return;
     }

   if (IS_EXTERNAL_ENGINE(overlay->wsd))
     evas_object_smart_changed(ELM_WIDGET_DATA(overlay->wsd)->obj);
   else
     evas_object_smart_changed(overlay->wsd->pan_obj);

#else
   (void)overlay;
   (void)lon;
   (void)lat;
#endif
}

EAPI void
elm_map_overlay_region_get(const Elm_Map_Overlay *overlay,
                           double *lon,
                           double *lat)
{
#ifdef HAVE_ELEMENTARY_ECORE_CON
   EINA_SAFETY_ON_NULL_RETURN(overlay);
   EINA_SAFETY_ON_NULL_RETURN(overlay->wsd);
   ELM_MAP_CHECK(ELM_WIDGET_DATA(overlay->wsd)->obj);

   if (overlay->type == ELM_MAP_OVERLAY_TYPE_GROUP)
     {
        Overlay_Group *ovl = overlay->ovl;

        if (lon) *lon = ovl->lon;
        if (lat) *lat = ovl->lat;
     }
   else if (overlay->type == ELM_MAP_OVERLAY_TYPE_DEFAULT)
     {
        const Overlay_Default *ovl = overlay->ovl;

        if (lon) *lon = ovl->lon;
        if (lat) *lat = ovl->lat;
     }
   else if (overlay->type == ELM_MAP_OVERLAY_TYPE_BUBBLE)
     {
        const Overlay_Bubble *ovl = overlay->ovl;

        if (lon) *lon = ovl->lon;
        if (lat) *lat = ovl->lat;
     }
   else
     {
        ERR("Not supported overlay type: %d", overlay->type);
     }

#else
   (void)overlay;
   (void)lon;
   (void)lat;
#endif
}

EAPI void
elm_map_overlay_icon_set(Elm_Map_Overlay *overlay,
                         Evas_Object *icon)
{
#ifdef HAVE_ELEMENTARY_ECORE_CON
   EINA_SAFETY_ON_NULL_RETURN(overlay);
   EINA_SAFETY_ON_NULL_RETURN(icon);
   EINA_SAFETY_ON_NULL_RETURN(overlay->wsd);
   ELM_MAP_CHECK(ELM_WIDGET_DATA(overlay->wsd)->obj);

   if (overlay->type == ELM_MAP_OVERLAY_TYPE_DEFAULT)
     {
       _overlay_default_icon_update(overlay->ovl, icon);
     }
   else if (overlay->type == ELM_MAP_OVERLAY_TYPE_CLASS)
     {
       _overlay_class_icon_update(overlay->ovl, icon);
     }
   else
     {
        ERR("Not supported overlay type: %d", overlay->type);
	 return;
     }

   if (IS_EXTERNAL_ENGINE(overlay->wsd))
     evas_object_smart_changed(ELM_WIDGET_DATA(overlay->wsd)->obj);
   else
     evas_object_smart_changed(overlay->wsd->pan_obj);

#else
   (void)overlay;
   (void)icon;
#endif
}

EAPI const Evas_Object *
elm_map_overlay_icon_get(const Elm_Map_Overlay *overlay)
{
#ifdef HAVE_ELEMENTARY_ECORE_CON
   EINA_SAFETY_ON_NULL_RETURN_VAL(overlay, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(overlay->wsd, NULL);
   ELM_MAP_CHECK(ELM_WIDGET_DATA(overlay->wsd)->obj) NULL;

   if (overlay->type == ELM_MAP_OVERLAY_TYPE_DEFAULT)
     {
        const Overlay_Default *ovl = overlay->ovl;

        return ovl->icon;
     }
   else if (overlay->type == ELM_MAP_OVERLAY_TYPE_CLASS)
     {
        const Overlay_Class *ovl = overlay->ovl;

        return ovl->icon;
     }
   else
     {
        ERR("Not supported overlay type: %d", overlay->type);
        return NULL;
     }
#else
   (void)overlay;
   return NULL;
#endif
}

EAPI void
elm_map_overlay_content_set(Elm_Map_Overlay *overlay,
                            Evas_Object *content)
{
#ifdef HAVE_ELEMENTARY_ECORE_CON
   EINA_SAFETY_ON_NULL_RETURN(overlay);
   EINA_SAFETY_ON_NULL_RETURN(content);
   EINA_SAFETY_ON_NULL_RETURN(overlay->wsd);
   ELM_MAP_CHECK(ELM_WIDGET_DATA(overlay->wsd)->obj);

   if (overlay->type == ELM_MAP_OVERLAY_TYPE_DEFAULT)
   {
     if (MAP_ENGINE_OVERLAY_SUPPORT(overlay->wsd))
       {
          _overlay_default_engine_content_create_or_update(overlay, content);
       }
     else
       {
           _overlay_default_content_update(overlay->ovl, content, overlay);
       }
   }
   else if (overlay->type == ELM_MAP_OVERLAY_TYPE_CLASS)
     {
        _overlay_class_content_update(overlay->ovl, content);
     }
   else
     {
        ERR("Not supported overlay type: %d", overlay->type);
        return;
     }

   if (IS_EXTERNAL_ENGINE(overlay->wsd))
     evas_object_smart_changed(ELM_WIDGET_DATA(overlay->wsd)->obj);
   else
     evas_object_smart_changed(overlay->wsd->pan_obj);

#else
   (void)overlay;
   (void)content;
#endif
}

EAPI const Evas_Object *
elm_map_overlay_content_get(const Elm_Map_Overlay *overlay)
{
#ifdef HAVE_ELEMENTARY_ECORE_CON
   EINA_SAFETY_ON_NULL_RETURN_VAL(overlay, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(overlay->wsd, NULL);

   ELM_MAP_CHECK(ELM_WIDGET_DATA(overlay->wsd)->obj) NULL;

   if (overlay->type == ELM_MAP_OVERLAY_TYPE_DEFAULT)
     {
        const Overlay_Default *ovl = overlay->ovl;
        return ovl->content;
     }
   else if (overlay->type == ELM_MAP_OVERLAY_TYPE_CLASS)
     {
        const Overlay_Class *ovl = overlay->ovl;
        return ovl->content;
     }
   else
     {
        ERR("Not supported overlay type: %d", overlay->type);
        return NULL;
     }
#else
   (void)overlay;
   return NULL;
#endif
}

EAPI void
elm_map_overlay_color_set(Elm_Map_Overlay *overlay,
                          int r,
                          int g,
                          int b,
                          int a)
{
#ifdef HAVE_ELEMENTARY_ECORE_CON
   EINA_SAFETY_ON_NULL_RETURN(overlay);
   EINA_SAFETY_ON_NULL_RETURN(overlay->wsd);
   ELM_MAP_CHECK(ELM_WIDGET_DATA(overlay->wsd)->obj);

   overlay->c.r = r;
   overlay->c.g = g;
   overlay->c.b = b;
   overlay->c.a = a;

   if (overlay->type == ELM_MAP_OVERLAY_TYPE_ROUTE)
     {
        if (IS_EXTERNAL_ENGINE(overlay->wsd))
          {
             if (overlay->wsd->engine->route_add && overlay->wsd->engine->route_color_set)
               {
                  overlay->wsd->engine->route_color_set(ELM_WIDGET_DATA(overlay->wsd)->obj, overlay->engine_obj, (unsigned char)r, (unsigned char)g, (unsigned char)b, (unsigned char)a);
                  return;
               }
          }
     }

   if (overlay->type == ELM_MAP_OVERLAY_TYPE_CLASS)
     _overlay_class_color_update(overlay->ovl, overlay->c);
   else if (overlay->type == ELM_MAP_OVERLAY_TYPE_DEFAULT)
     _overlay_default_color_update(overlay->ovl, overlay->c);
   else if (overlay->type == ELM_MAP_OVERLAY_TYPE_ROUTE)
     _overlay_route_color_update(overlay->ovl, overlay->c);
   else if (overlay->type == ELM_MAP_OVERLAY_TYPE_CIRCLE)
     _overlay_circle_color_update(overlay->ovl, overlay->c);
   else if (overlay->type == ELM_MAP_OVERLAY_TYPE_POLYLINE)
     {
        if (IS_EXTERNAL_ENGINE(overlay->wsd))
          {
             if (overlay->wsd->engine->polyline_add && overlay->wsd->engine->polyline_color_set)
               {
                  overlay->wsd->engine->polyline_color_set(ELM_WIDGET_DATA(overlay->wsd)->obj, overlay->engine_obj, (unsigned char)r, (unsigned char)g, (unsigned char)b, (unsigned char)a);
                  return;
               }
          }
	 else
	   {
	      _overlay_polyline_color_update(overlay->ovl, overlay->c);
	   }
     }
   else
     {
        ERR("Not supported overlay type: %d", overlay->type);
     }

#else
   (void)overlay;
   (void)r;
   (void)g;
   (void)b;
   (void)a;
#endif
}

EAPI void
elm_map_overlay_color_get(const Elm_Map_Overlay *overlay,
                          int *r,
                          int *g,
                          int *b,
                          int *a)
{
#ifdef HAVE_ELEMENTARY_ECORE_CON
   EINA_SAFETY_ON_NULL_RETURN(overlay);
   EINA_SAFETY_ON_NULL_RETURN(overlay->wsd);
   ELM_MAP_CHECK(ELM_WIDGET_DATA(overlay->wsd)->obj);

   if (overlay->type == ELM_MAP_OVERLAY_TYPE_ROUTE)// || overlay->type == ELM_MAP_OVERLAY_TYPE_POLYLINE)
     {
        if (r) *r = overlay->c.r;
        if (g) *g = overlay->c.g;
        if (b) *b = overlay->c.b;
        if (a) *a = overlay->c.a;
     }
   else
     {
        ERR("Not supported overlay type: %d", overlay->type);
     }

#else
   (void)overlay;
   (void)r;
   (void)g;
   (void)b;
   (void)a;
#endif
}

EAPI void
elm_map_overlay_get_cb_set(Elm_Map_Overlay *overlay,
                           Elm_Map_Overlay_Get_Cb get_cb,
                           void *data)
{
#ifdef HAVE_ELEMENTARY_ECORE_CON
   EINA_SAFETY_ON_NULL_RETURN(overlay);
   EINA_SAFETY_ON_NULL_RETURN(overlay->wsd);
   ELM_MAP_CHECK(ELM_WIDGET_DATA(overlay->wsd)->obj);

   overlay->cb = get_cb;
   overlay->cb_data = data;

   if (overlay->type == ELM_MAP_OVERLAY_TYPE_CLASS)
     _overlay_class_cb_set(overlay->ovl, get_cb, data);
#else
   (void)overlay;
   (void)get_cb;
   (void)data;
#endif
}

EAPI void
elm_map_overlay_del_cb_set(Elm_Map_Overlay *overlay,
                           Elm_Map_Overlay_Del_Cb del_cb,
                           void *data)
{
#ifdef HAVE_ELEMENTARY_ECORE_CON
   EINA_SAFETY_ON_NULL_RETURN(overlay);
   EINA_SAFETY_ON_NULL_RETURN(overlay->wsd);
   ELM_MAP_CHECK(ELM_WIDGET_DATA(overlay->wsd)->obj);

   overlay->del_cb = del_cb;
   overlay->del_cb_data = data;
#else
   (void)overlay;
   (void)del_cb;
   (void)data;
#endif
}

EAPI Elm_Map_Overlay *
elm_map_overlay_class_add(Evas_Object *obj)
{
#ifdef HAVE_ELEMENTARY_ECORE_CON
   Elm_Map_Overlay *overlay;
   Overlay_Class *ovl;

   ELM_MAP_CHECK(obj) NULL;
   ELM_MAP_DATA_GET(obj, sd);

   overlay = ELM_NEW(Elm_Map_Overlay);
   overlay->wsd = sd;
   overlay->type = ELM_MAP_OVERLAY_TYPE_CLASS;
   overlay->ovl = _overlay_class_new(sd);
   overlay->c.r = 0x90;
   overlay->c.g = 0xee;
   overlay->c.b = 0x90;
   overlay->c.a = 0xff;
   overlay->engine_obj = NULL;
   sd->overlays = eina_list_append(sd->overlays, overlay);
   ovl = (Overlay_Class*)(overlay->ovl);
   ovl->base = overlay;

   if (MAP_ENGINE_GROUP_OVERLAY_SUPPORT(overlay->wsd))
     {
        //ELM_MAP_ENG_OBJECT_CREATE(sd->engine->group_create, overlay->engine_obj, ELM_WIDGET_DATA(overlay->wsd)->obj, -1, -1);
        evas_object_smart_changed(ELM_WIDGET_DATA(sd)->obj);
     }
   else
     evas_object_smart_changed(sd->pan_obj);

   return overlay;
#else
   (void)obj;
   return NULL;
#endif
}

EAPI void
elm_map_overlay_class_append(Elm_Map_Overlay *klass,
                             Elm_Map_Overlay *overlay)
{
#ifdef HAVE_ELEMENTARY_ECORE_CON
   Overlay_Class *class_ovl;

   EINA_SAFETY_ON_NULL_RETURN(klass);
   EINA_SAFETY_ON_NULL_RETURN(overlay);
   EINA_SAFETY_ON_NULL_RETURN(klass->wsd);
   ELM_MAP_CHECK(ELM_WIDGET_DATA(klass->wsd)->obj);
   EINA_SAFETY_ON_FALSE_RETURN(klass->type == ELM_MAP_OVERLAY_TYPE_CLASS);

   class_ovl = klass->ovl;
   if (eina_list_data_find(class_ovl->members, overlay))
     {
        ERR("Already added overlay into clas");
        return;
     }
   class_ovl->members = eina_list_append(class_ovl->members, overlay);

   // Update group by class
   overlay->grp->klass = klass;
   _overlay_group_icon_update(overlay->grp, class_ovl->icon);
   _overlay_group_content_update(overlay->grp, class_ovl->content, overlay);
   _overlay_group_color_update(overlay->grp, klass->c);
   _overlay_group_cb_set(overlay->grp, klass->cb, klass->data);

   if (overlay->type == ELM_MAP_OVERLAY_TYPE_DEFAULT)
     {
         _overlay_default_class_icon_update(overlay->ovl, class_ovl->icon);
         _overlay_default_class_content_update(overlay->ovl, class_ovl->content);
     }

   if (IS_EXTERNAL_ENGINE(overlay->wsd))
     {
        //if (klass->engine_obj && overlay->engine_obj && (overlay->type == ELM_MAP_OVERLAY_TYPE_DEFAULT))
        //  ELM_MAP_ENG_OBJECT_SET(overlay->wsd->engine->group_object_add, ELM_WIDGET_DATA(overlay->wsd)->obj, klass->engine_obj, overlay->engine_obj);

        evas_object_smart_changed(ELM_WIDGET_DATA(klass->wsd)->obj);
     }
   else
     evas_object_smart_changed(klass->wsd->pan_obj);

#else
   (void)klass;
   (void)overlay;
#endif
}

EAPI void
elm_map_overlay_class_remove(Elm_Map_Overlay *klass,
                             Elm_Map_Overlay *overlay)
{
#ifdef HAVE_ELEMENTARY_ECORE_CON
   Overlay_Class *ovl;

   EINA_SAFETY_ON_NULL_RETURN(klass);
   EINA_SAFETY_ON_NULL_RETURN(overlay);
   EINA_SAFETY_ON_NULL_RETURN(klass->wsd);
   ELM_MAP_CHECK(ELM_WIDGET_DATA(klass->wsd)->obj);
   EINA_SAFETY_ON_FALSE_RETURN(klass->type == ELM_MAP_OVERLAY_TYPE_CLASS);

   ovl = klass->ovl;
   ovl->members = eina_list_remove(ovl->members, overlay);

   overlay->grp->klass = NULL;
   _overlay_group_icon_update(overlay->grp, NULL);
   _overlay_group_content_update(overlay->grp, NULL, NULL);
   if (overlay->type == ELM_MAP_OVERLAY_TYPE_DEFAULT)
     {
        _overlay_default_class_icon_update(overlay->ovl, NULL);
        _overlay_default_class_content_update(overlay->ovl, NULL);
     }

   if (IS_EXTERNAL_ENGINE(overlay->wsd))
     {
        //if (klass->engine_obj && overlay->engine_obj && (overlay->type == ELM_MAP_OVERLAY_TYPE_DEFAULT))
        //  ELM_MAP_ENG_OBJECT_SET(overlay->wsd->engine->group_object_del, ELM_WIDGET_DATA(overlay->wsd)->obj, klass->engine_obj, overlay->engine_obj);
        evas_object_smart_changed(ELM_WIDGET_DATA(klass->wsd)->obj);
     }
   else
     evas_object_smart_changed(klass->wsd->pan_obj);

#else
   (void)klass;
   (void)overlay;
#endif
}

EAPI void
elm_map_overlay_class_zoom_max_set(Elm_Map_Overlay *klass,
                                   int zoom)
{
#ifdef HAVE_ELEMENTARY_ECORE_CON
   Overlay_Class *ovl;

   EINA_SAFETY_ON_NULL_RETURN(klass);
   EINA_SAFETY_ON_NULL_RETURN(klass->wsd);
   ELM_MAP_CHECK(ELM_WIDGET_DATA(klass->wsd)->obj);
   EINA_SAFETY_ON_FALSE_RETURN(klass->type == ELM_MAP_OVERLAY_TYPE_CLASS);

   ovl = klass->ovl;
   if (ovl->zoom_max == !!zoom) return;
   ovl->zoom_max = zoom;

#else
   (void)klass;
   (void)zoom;
#endif
}

EAPI int
elm_map_overlay_class_zoom_max_get(const Elm_Map_Overlay *klass)
{
#ifdef HAVE_ELEMENTARY_ECORE_CON
   const Overlay_Class *ovl;

   EINA_SAFETY_ON_NULL_RETURN_VAL(klass, OVERLAY_CLASS_ZOOM_MAX);
   EINA_SAFETY_ON_NULL_RETURN_VAL(klass->wsd, OVERLAY_CLASS_ZOOM_MAX);
   ELM_MAP_CHECK(ELM_WIDGET_DATA(klass->wsd)->obj) OVERLAY_CLASS_ZOOM_MAX;

   EINA_SAFETY_ON_FALSE_RETURN_VAL
     (klass->type == ELM_MAP_OVERLAY_TYPE_CLASS, OVERLAY_CLASS_ZOOM_MAX);

   ovl = klass->ovl;
   return ovl->zoom_max;
#else
   (void)klass;
   return OVERLAY_CLASS_ZOOM_MAX;
#endif
}

EAPI Eina_List *
elm_map_overlay_group_members_get(const Elm_Map_Overlay *grp)
{
#ifdef HAVE_ELEMENTARY_ECORE_CON
   Overlay_Group *ovl;

   EINA_SAFETY_ON_NULL_RETURN_VAL(grp, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(grp->wsd, NULL);
   ELM_MAP_CHECK(ELM_WIDGET_DATA(grp->wsd)->obj) NULL;

   EINA_SAFETY_ON_FALSE_RETURN_VAL
     (grp->type == ELM_MAP_OVERLAY_TYPE_GROUP, NULL);

   ovl = grp->ovl;
   return ovl->members;
#else
   (void)grp;
   return NULL;
#endif
}

EAPI Elm_Map_Overlay *
elm_map_overlay_bubble_add(Evas_Object *obj)
{
#ifdef HAVE_ELEMENTARY_ECORE_CON
   Elm_Map_Overlay *overlay;
   Overlay_Bubble *ovl;

   ELM_MAP_CHECK(obj) NULL;
   ELM_MAP_DATA_GET(obj, sd);

   if (IS_EXTERNAL_ENGINE(sd))
     {
        ERR("Engine(%s) do not support this function", sd->engine->name);
        return NULL;
     }

   overlay = ELM_NEW(Elm_Map_Overlay);
   overlay->wsd = sd;
   overlay->type = ELM_MAP_OVERLAY_TYPE_BUBBLE;
   overlay->c.r = 255;
   overlay->c.g = 255;
   overlay->c.b = 255;
   overlay->c.a = 255;
   overlay->engine_obj = NULL;
   overlay->ovl = _overlay_bubble_new(overlay);
   overlay->grp = _overlay_group_new(sd);
   sd->overlays = eina_list_append(sd->overlays, overlay);
   ovl = (Overlay_Bubble*)(overlay->ovl);
   ovl->base = overlay;

   if (IS_EXTERNAL_ENGINE(sd))
     evas_object_smart_changed(ELM_WIDGET_DATA(sd)->obj);
   else
     evas_object_smart_changed(sd->pan_obj);

   return overlay;
#else
   (void)obj;
   return NULL;
#endif
}

EAPI void
elm_map_overlay_bubble_follow(Elm_Map_Overlay *bubble,
                              const Elm_Map_Overlay *parent)
{
#ifdef HAVE_ELEMENTARY_ECORE_CON
   Overlay_Bubble *ovl;
   Evas_Object *pobj;

   EINA_SAFETY_ON_NULL_RETURN(bubble);
   EINA_SAFETY_ON_NULL_RETURN(parent);
   ELM_MAP_CHECK(ELM_WIDGET_DATA(bubble->wsd)->obj);
   EINA_SAFETY_ON_FALSE_RETURN(bubble->type == ELM_MAP_OVERLAY_TYPE_BUBBLE);

   ovl = bubble->ovl;
   pobj = _overlay_obj_get(parent);
   if (!pobj) return;

   ovl->pobj = pobj;

   if (IS_EXTERNAL_ENGINE(bubble->wsd))
     evas_object_smart_changed(ELM_WIDGET_DATA(bubble->wsd)->obj);
   else
     evas_object_smart_changed(bubble->wsd->pan_obj);

#else
   (void)bubble;
   (void)parent;
#endif
}

EAPI void
elm_map_overlay_bubble_content_append(Elm_Map_Overlay *bubble,
                                      Evas_Object *content)
{
#ifdef HAVE_ELEMENTARY_ECORE_CON
   Overlay_Bubble *bb;

   EINA_SAFETY_ON_NULL_RETURN(bubble);
   EINA_SAFETY_ON_NULL_RETURN(content);
   ELM_MAP_CHECK(ELM_WIDGET_DATA(bubble->wsd)->obj);
   EINA_SAFETY_ON_FALSE_RETURN(bubble->type == ELM_MAP_OVERLAY_TYPE_BUBBLE);

   bb = bubble->ovl;
   elm_box_pack_end(bb->bx, content);

   if (IS_EXTERNAL_ENGINE(bubble->wsd))
     evas_object_smart_changed(ELM_WIDGET_DATA(bubble->wsd)->obj);
   else
     evas_object_smart_changed(bubble->wsd->pan_obj);

#else
   (void)bubble;
   (void)content;
#endif
}

EAPI void
elm_map_overlay_bubble_content_clear(Elm_Map_Overlay *bubble)
{
#ifdef HAVE_ELEMENTARY_ECORE_CON
   Overlay_Bubble *bb;

   EINA_SAFETY_ON_NULL_RETURN(bubble);
   ELM_MAP_CHECK(ELM_WIDGET_DATA(bubble->wsd)->obj);
   EINA_SAFETY_ON_FALSE_RETURN(bubble->type == ELM_MAP_OVERLAY_TYPE_BUBBLE);

   bb = bubble->ovl;
   elm_box_clear(bb->bx);

   if (IS_EXTERNAL_ENGINE(bubble->wsd))
     evas_object_smart_changed(ELM_WIDGET_DATA(bubble->wsd)->obj);
   else
     evas_object_smart_changed(bubble->wsd->pan_obj);

#else
   (void)bubble;
#endif
}

EAPI Elm_Map_Overlay *
elm_map_overlay_route_add(Evas_Object *obj,
                          const Elm_Map_Route *route)
{
#ifdef HAVE_ELEMENTARY_ECORE_CON
   Elm_Map_Overlay *overlay;
   Overlay_Route *ovl = NULL;

   ELM_MAP_CHECK(obj) NULL;
   ELM_MAP_DATA_GET(obj, sd);

   EINA_SAFETY_ON_NULL_RETURN_VAL(route, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(route->wsd, NULL);
   EINA_SAFETY_ON_FALSE_RETURN_VAL
     (obj == ELM_WIDGET_DATA(route->wsd)->obj, NULL);

   overlay = ELM_NEW(Elm_Map_Overlay);
   overlay->wsd = sd;
   overlay->type = ELM_MAP_OVERLAY_TYPE_ROUTE;
   overlay->c.r = 255;
   overlay->c.g = 0;
   overlay->c.b = 0;
   overlay->c.a = 255;
   overlay->engine_obj = NULL;

   if (sd->engine->route_add)
     {
        overlay->engine_obj = sd->engine->route_add(ELM_WIDGET_DATA(sd)->obj, route->data );
        overlay->ovl = NULL;
        overlay->grp = NULL;
        sd->overlays = eina_list_append(sd->overlays, overlay);
     }
   else
     {
        overlay->ovl = _overlay_route_new(sd, route, overlay->c);
        overlay->grp = _overlay_group_new(sd);
        sd->overlays = eina_list_append(sd->overlays, overlay);
        ovl = (Overlay_Route*)(overlay->ovl);
        ovl->base = overlay;
        evas_object_smart_changed(sd->pan_obj);
     }

   return overlay;
#else
   (void)obj;
   (void)route;
   return NULL;
#endif
}

EAPI Elm_Map_Overlay *
elm_map_overlay_line_add(Evas_Object *obj,
                         double flon,
                         double flat,
                         double tlon,
                         double tlat)
{
#ifdef HAVE_ELEMENTARY_ECORE_CON
   Elm_Map_Overlay *overlay;
   Overlay_Line *ovl = NULL;

   ELM_MAP_CHECK(obj) NULL;
   ELM_MAP_DATA_GET(obj, sd);

   overlay = ELM_NEW(Elm_Map_Overlay);
   overlay->wsd = sd;
   overlay->type = ELM_MAP_OVERLAY_TYPE_LINE;
   overlay->c.r = 0xff;
   overlay->c.g = 0x00;
   overlay->c.b = 0x00;
   overlay->c.a = 0xff;
   overlay->engine_obj = NULL;
   overlay->ovl = _overlay_line_new(sd, flon, flat, tlon, tlat, overlay->c);
   overlay->grp = _overlay_group_new(sd);
   sd->overlays = eina_list_append(sd->overlays, overlay);
   ovl = (Overlay_Line*)(overlay->ovl);
   ovl->base = overlay;

   evas_object_smart_changed(sd->pan_obj);
   return overlay;
#else
   (void)obj;
   (void)flon;
   (void)flat;
   (void)tlon;
   (void)tlat;
   return NULL;
#endif
}

EAPI Elm_Map_Overlay *
elm_map_overlay_polyline_add(Evas_Object *obj)
{
#ifdef HAVE_ELEMENTARY_ECORE_CON
   ELM_MAP_CHECK(obj) NULL;
   Elm_Map_Overlay *overlay;
   Overlay_Polyline *ovl = NULL;

   ELM_MAP_DATA_GET(obj, sd);

   overlay = ELM_NEW(Elm_Map_Overlay);
   overlay->wsd = sd;
   overlay->type = ELM_MAP_OVERLAY_TYPE_POLYLINE;
   overlay->c.r = 0xdc;
   overlay->c.g = 0x14;
   overlay->c.b = 0x3c;
   overlay->c.a = 220;
   overlay->engine_obj = NULL;

   overlay->ovl = _overlay_polyline_new(sd, overlay->c);
   ovl = (Overlay_Polyline*)(overlay->ovl);
   ovl->base = overlay;

   sd->overlays = eina_list_append(sd->overlays, overlay);

   if (IS_EXTERNAL_ENGINE(sd))
     {
        if (sd->engine->polyline_add)
	   overlay->engine_obj = sd->engine->polyline_add(obj);

        overlay->grp = NULL;
     }
   else
     {
        overlay->grp = _overlay_group_new(sd);
     }
   return overlay;
#else
   (void)obj;
   return NULL;
#endif
}

EAPI void
elm_map_overlay_polyline_region_add(Elm_Map_Overlay *overlay, double lon, double lat)
{
#ifdef HAVE_ELEMENTARY_ECORE_CON
   EINA_SAFETY_ON_NULL_RETURN(overlay);
   ELM_MAP_CHECK(ELM_WIDGET_DATA(overlay->wsd)->obj);
   EINA_SAFETY_ON_FALSE_RETURN(overlay->type == ELM_MAP_OVERLAY_TYPE_POLYLINE);

   Overlay_Polyline *ovl;
   Elm_Map_Region *region;

   ovl = overlay->ovl;
   region = ELM_NEW(Elm_Map_Region);
   region->longitude= lon;
   region->latitude= lat;
   region->altitude = 0;

   ovl->regions = eina_list_append(ovl->regions, region);

   if (IS_EXTERNAL_ENGINE(overlay->wsd))
     {
        if (overlay->wsd->engine->polyline_region_add)
	   overlay->wsd->engine->polyline_region_add(ELM_WIDGET_DATA(overlay->wsd)->obj, overlay->engine_obj, region->longitude,  region->latitude);
     }
   else
     {
        _overlay_polyline_region_add(ovl);
     }

#else
   (void)overlay;
   (void)lon;
   (void)lat;
#endif
}

EAPI void
elm_map_overlay_polyline_width_set(Elm_Map_Overlay *overlay, int width)
{
#ifdef HAVE_ELEMENTARY_ECORE_CON
   EINA_SAFETY_ON_NULL_RETURN(overlay);
   ELM_MAP_CHECK(ELM_WIDGET_DATA(overlay->wsd)->obj);
   EINA_SAFETY_ON_FALSE_RETURN(overlay->type == ELM_MAP_OVERLAY_TYPE_POLYLINE);

   if (width < 0)
     {
        ERR("The width should be bigger than zero.");
        return;
     }

   Overlay_Polyline *ovl = NULL;
   ovl = (Overlay_Polyline*)overlay->ovl;

   if (IS_EXTERNAL_ENGINE(overlay->wsd))
     {
        ovl->width = width;
        if (overlay->wsd->engine->polyline_add && overlay->wsd->engine->polyline_width_set)
          overlay->wsd->engine->polyline_width_set(ELM_WIDGET_DATA(overlay->wsd)->obj, overlay->engine_obj, width);
     }
   else
     {
        ERR("Engine(%s) do not support this function", overlay->wsd->engine->name);
        ERR("Forcely, the width of polyline is set to '0'.");
        ovl->width = 0;
     }

#else
   (void)overlay;
   (void)width;
#endif
}

EAPI int
elm_map_overlay_polyline_width_get(Elm_Map_Overlay *overlay)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(overlay, -1);
   ELM_MAP_CHECK(ELM_WIDGET_DATA(overlay->wsd)->obj) -1;
   EINA_SAFETY_ON_FALSE_RETURN_VAL(overlay->type == ELM_MAP_OVERLAY_TYPE_POLYLINE, -1);

   Overlay_Polyline *ovl = NULL;
   ovl = (Overlay_Polyline*)overlay->ovl;

   if (IS_EXTERNAL_ENGINE(overlay->wsd))
     {
        if (overlay->wsd->engine->polyline_add && overlay->wsd->engine->polyline_width_get)
          ovl->width = overlay->wsd->engine->polyline_width_get(ELM_WIDGET_DATA(overlay->wsd)->obj, overlay->engine_obj);
     }

   return ovl->width;
}

EAPI Elm_Map_Overlay *
elm_map_overlay_polygon_add(Evas_Object *obj)
{
#ifdef HAVE_ELEMENTARY_ECORE_CON
   Elm_Map_Overlay *overlay;
   Overlay_Polygon *ovl = NULL;

   ELM_MAP_CHECK(obj) NULL;
   ELM_MAP_DATA_GET(obj, sd);

   overlay = ELM_NEW(Elm_Map_Overlay);
   overlay->wsd = sd;
   overlay->type = ELM_MAP_OVERLAY_TYPE_POLYGON;
   overlay->c.r = 0xdc;
   overlay->c.g = 0x14;
   overlay->c.b = 0x3c;
   overlay->c.a = 220;
   overlay->engine_obj = NULL;
   overlay->ovl = _overlay_polygon_new(sd, overlay->c);
   overlay->grp = _overlay_group_new(sd);
   ovl = (Overlay_Polygon*)(overlay->ovl);
   ovl->base = overlay;

   sd->overlays = eina_list_append(sd->overlays, overlay);

   if (IS_EXTERNAL_ENGINE(overlay->wsd))
     evas_object_smart_changed(ELM_WIDGET_DATA(sd)->obj);
   else
     evas_object_smart_changed(sd->pan_obj);

   return overlay;
#else
   (void)obj;
   return NULL;
#endif
}

EAPI void
elm_map_overlay_polygon_region_add(Elm_Map_Overlay *overlay,
                                   double lon,
                                   double lat)
{
#ifdef HAVE_ELEMENTARY_ECORE_CON
   Overlay_Polygon *ovl;
   Elm_Map_Region *region;

   EINA_SAFETY_ON_NULL_RETURN(overlay);
   ELM_MAP_CHECK(ELM_WIDGET_DATA(overlay->wsd)->obj);
   EINA_SAFETY_ON_FALSE_RETURN(overlay->type == ELM_MAP_OVERLAY_TYPE_POLYGON);

   ovl = overlay->ovl;
   region = ELM_NEW(Elm_Map_Region);
   region->longitude= lon;
   region->latitude= lat;
   region->altitude = 0;

   ovl->regions = eina_list_append(ovl->regions, region);

   if (IS_EXTERNAL_ENGINE(overlay->wsd))
     evas_object_smart_changed(ELM_WIDGET_DATA(ovl->wsd)->obj);
   else
     evas_object_smart_changed(ovl->wsd->pan_obj);

#else
   (void)overlay;
   (void)lon;
   (void)lat;
#endif
}

EAPI Elm_Map_Overlay *
elm_map_overlay_circle_add(Evas_Object *obj,
                           double lon,
                           double lat,
                           double radius)
{
#ifdef HAVE_ELEMENTARY_ECORE_CON
   Elm_Map_Overlay *overlay;

   ELM_MAP_CHECK(obj) NULL;
   ELM_MAP_DATA_GET(obj, sd);

   overlay = ELM_NEW(Elm_Map_Overlay);
   overlay->wsd = sd;
   overlay->type = ELM_MAP_OVERLAY_TYPE_CIRCLE;
   overlay->c.r = 0xdc;
   overlay->c.g = 0x14;
   overlay->c.b = 0x3c;
   overlay->c.a = 220;
   overlay->engine_obj = NULL;
   overlay->ovl = _overlay_circle_new(sd, lon, lat, radius, overlay->c);
   overlay->grp = _overlay_group_new(sd);
   sd->overlays = eina_list_append(sd->overlays, overlay);

   if (IS_EXTERNAL_ENGINE(overlay->wsd))
     evas_object_smart_changed(ELM_WIDGET_DATA(sd)->obj);
   else
     evas_object_smart_changed(sd->pan_obj);
   return overlay;
#else
   (void)obj;
   (void)lon;
   (void)lat;
   (void)radius;
   return NULL;
#endif
}

EAPI Elm_Map_Overlay *
elm_map_overlay_scale_add(Evas_Object *obj,
                          Evas_Coord x,
                          Evas_Coord y)
{
#ifdef HAVE_ELEMENTARY_ECORE_CON
   Elm_Map_Overlay *overlay;
   Overlay_Scale *ovl = NULL;

   ELM_MAP_CHECK(obj) NULL;
   ELM_MAP_DATA_GET(obj, sd);

   overlay = ELM_NEW(Elm_Map_Overlay);
   overlay->wsd = sd;
   overlay->type = ELM_MAP_OVERLAY_TYPE_SCALE;
   overlay->c.r = 0;
   overlay->c.g = 0;
   overlay->c.b = 0;
   overlay->c.a = 255;
   overlay->ovl = _overlay_scale_new(sd, x, y, overlay->c);
   overlay->grp = _overlay_group_new(sd);
   sd->overlays = eina_list_append(sd->overlays, overlay);
   ovl = (Overlay_Scale*)(overlay->ovl);
   ovl->base = overlay;

   if (IS_EXTERNAL_ENGINE(overlay->wsd))
     evas_object_smart_changed(ELM_WIDGET_DATA(sd)->obj);
   else
     evas_object_smart_changed(sd->pan_obj);

   return overlay;
#else
   (void)obj;
   (void)x;
   (void)y;
   return NULL;
#endif
}

#ifdef ELM_EMAP
EAPI Evas_Object *
elm_map_track_add(Evas_Object *obj,
                  void *emap)
{
#ifdef HAVE_ELEMENTARY_ECORE_CON
   EMap_Route *emapr = emap;
   Evas_Object *route;

   ELM_MAP_CHECK(obj) NULL;
   ELM_MAP_DATA_GET(obj, sd);

   route = elm_route_add(obj);
   elm_route_emap_set(route, emapr);
   sd->track = eina_list_append(sd->track, route);

   return route;
#else
   (void)obj;
   (void)emap;
   return NULL;
#endif
}

EAPI void
elm_map_track_remove(Evas_Object *obj,
                     Evas_Object *route)
{
#ifdef HAVE_ELEMENTARY_ECORE_CON
   ELM_MAP_CHECK(obj);
   ELM_MAP_DATA_GET(obj, sd);

   sd->track = eina_list_remove(sd->track, route);
   evas_object_del(route);
#else
   (void)obj;
   (void)route;
#endif
}

#else
EAPI Evas_Object *
elm_map_track_add(Evas_Object *obj __UNUSED__,
                  void *emap __UNUSED__)
{
   return NULL;
}

EAPI void
elm_map_track_remove(Evas_Object *obj __UNUSED__,
                     Evas_Object *route __UNUSED__)
{
}
#endif
