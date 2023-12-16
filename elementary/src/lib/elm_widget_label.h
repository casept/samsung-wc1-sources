#ifndef ELM_WIDGET_LABEL_H
#define ELM_WIDGET_LABEL_H

#include "elm_widget_layout.h"

#ifdef HAVE_EIO
# include <Eio.h>
#endif

/**
 * @addtogroup Widget
 * @{
 *
 * @section elm-label-class The Elementary Label Class
 *
 * Elementary, besides having the @ref Label widget, exposes its
 * foundation -- the Elementary Label Class -- in order to create other
 * widgets which are a label with some more logic on top.
 */

/**
 * @def ELM_LABEL_CLASS
 *
 * Use this macro to cast whichever subclass of
 * #Elm_Label_Smart_Class into it, so to access its fields.
 *
 * @ingroup Widget
 */
#define ELM_LABEL_CLASS(x) ((Elm_Label_Smart_Class *)x)

/**
 * @def ELM_LABEL_DATA
 *
 * Use this macro to cast whichever subdata of
 * #Elm_Label_Smart_Data into it, so to access its fields.
 *
 * @ingroup Widget
 */
#define ELM_LABEL_DATA(x)  ((Elm_Label_Smart_Data *)x)

/**
 * @def ELM_LABEL_SMART_CLASS_VERSION
 *
 * Current version for Elementary label @b base smart class, a value
 * which goes to _Elm_Label_Smart_Class::version.
 *
 * @ingroup Widget
 */
#define ELM_LABEL_SMART_CLASS_VERSION 1

/**
 * @def ELM_LABEL_SMART_CLASS_INIT
 *
 * Initializer for a whole #Elm_Label_Smart_Class structure, with
 * @c NULL values on its specific fields.
 *
 * @param smart_class_init initializer to use for the "base" field
 * (#Evas_Smart_Class).
 *
 * @see EVAS_SMART_CLASS_INIT_NULL
 * @see EVAS_SMART_CLASS_INIT_NAME_VERSION
 * @see ELM_LABEL_SMART_CLASS_INIT_NULL
 * @see ELM_LABEL_SMART_CLASS_INIT_NAME_VERSION
 *
 * @ingroup Widget
 */
#define ELM_LABEL_SMART_CLASS_INIT(smart_class_init) \
  {smart_class_init, ELM_LABEL_SMART_CLASS_VERSION}

/**
 * @def ELM_LABEL_SMART_CLASS_INIT_NULL
 *
 * Initializer to zero out a whole #Elm_Label_Smart_Class structure.
 *
 * @see ELM_LABEL_SMART_CLASS_INIT_NAME_VERSION
 * @see ELM_LABEL_SMART_CLASS_INIT
 *
 * @ingroup Widget
 */
#define ELM_LABEL_SMART_CLASS_INIT_NULL \
  ELM_LABEL_SMART_CLASS_INIT(EVAS_SMART_CLASS_INIT_NULL)

/**
 * @def ELM_LABEL_SMART_CLASS_INIT_NAME_VERSION
 *
 * Initializer to zero out a whole #Elm_Label_Smart_Class structure and
 * set its name and version.
 *
 * This is similar to #ELM_LABEL_SMART_CLASS_INIT_NULL, but it will
 * also set the version field of #Elm_Label_Smart_Class (base field)
 * to the latest #ELM_LABEL_SMART_CLASS_VERSION and name it to the
 * specific value.
 *
 * It will keep a reference to the name field as a <c>"const char *"</c>,
 * i.e., the name must be available while the structure is
 * used (hint: static or global variable!) and must not be modified.
 *
 * @see ELM_LABEL_SMART_CLASS_INIT_NULL
 * @see ELM_LABEL_SMART_CLASS_INIT
 *
 * @ingroup Widget
 */
#define ELM_LABEL_SMART_CLASS_INIT_NAME_VERSION(name) \
  ELM_LABEL_SMART_CLASS_INIT                          \
    (ELM_LAYOUT_SMART_CLASS_INIT_NAME_VERSION(name))

/**
 * Elementary label base smart class. This inherits directly from
 * #Elm_Layout_Smart_Class and is meant to build widgets extending the
 * behavior of a label.
 *
 * All of the functions listed on @ref Label namespace will work for
 * objects deriving from #Elm_Label_Smart_Class.
 */
typedef struct _Elm_Label_Smart_Class
{
   Elm_Layout_Smart_Class base;

   int                    version;    /**< Version of this smart class definition */
} Elm_Label_Smart_Class;

/**
 * Base layout smart data extended with label instance data.
 */
typedef struct _Elm_Label_Smart_Data Elm_Label_Smart_Data;
struct _Elm_Label_Smart_Data
{
   Elm_Layout_Smart_Data base;

   const char           *format;
   double                slide_duration;
   Evas_Coord            lastw;
   Evas_Coord            wrap_w;
   Elm_Wrap_Type         linewrap;
   Elm_Label_Slide_Mode  slide_mode;

   // TIZEN_ONLY(20140628): Add elm_label_anchor_access_provider_* APIs and Support anchor access features.
   int                   anchor_highlight_pos;
   Evas_Object          *anchor_highlight_rect;
   Eina_List            *anchor_access_providers;
   //
   // TIZEN_ONLY(140520): Improve performance when *_smart_theme is called.
   Evas_Object          *tb;
   Eina_Bool             on_clear : 1;
   //
   // TIZEN_ONLY(20140606): Update anchor when textblock geometry is changed.
   Evas_Coord            tx, ty, tw, th;
   //
   // TIZEN_ONLY(131028): Support item, anchor formats
   Ecore_Timer          *longpress_timer;
   Eina_List            *anchors;
   Eina_List            *item_providers;
   Evas_Coord            down_x, down_y;
   Eina_Bool             mouse_down_flag : 1;
   //
   // TIZEN_ONLY(20140623): Optimize performance for anchor, item.
   Eina_Bool             anchors_force_update : 1;
   //
   Eina_Bool             ellipsis : 1;
   Eina_Bool             slide_ellipsis : 1;

   unsigned int          current_anchor_idx;
   Ecore_Idler           *batch_anchor_update_handle;
   Eina_Bool             forward_rendering : 1;
   int                   forward_batch_processed;
};

// TIZEN_ONLY(131028): Support item, anchor formats
typedef struct _Sel Sel;
struct _Sel
{
   Evas_Textblock_Rectangle rect;
   Evas_Object *obj, *sobj;
};

typedef struct _Elm_Label_Anchor_Data Elm_Label_Anchor_Data;
struct _Elm_Label_Anchor_Data
{
   Evas_Object          *obj;
   Eina_List            *sel;
   char                 *name;
   Evas_Textblock_Cursor *start, *end;

   Evas_Coord            down_x, down_y;
   Eina_Bool             mouse_down_flag : 1;
   Eina_Bool             item : 1;
};

typedef struct _Elm_Label_Item_Provider     Elm_Label_Item_Provider;
struct _Elm_Label_Item_Provider
{
   Evas_Object *(*func)(void *data, Evas_Object * entry, const char *item);
   void        *data;
};

// TIZEN_ONLY(20140628): Add elm_label_anchor_access_provider_* APIs and Support anchor access features.
typedef struct _Elm_Label_Anchor_Access_Provider Elm_Label_Anchor_Access_Provider;
struct _Elm_Label_Anchor_Access_Provider
{
   char        *(*func)(void *data, Evas_Object * label, const char *name, const char *text);
   void        *data;
};
//

///////////////////////////
/**
 * @}
 */

EAPI extern const char ELM_LABEL_SMART_NAME[];
EAPI const Elm_Label_Smart_Class *elm_label_smart_class_get(void);

#define ELM_LABEL_DATA_GET(o, sd) \
  Elm_Label_Smart_Data * sd = evas_object_smart_data_get(o)

#define ELM_LABEL_DATA_GET_OR_RETURN(o, ptr)         \
  ELM_LABEL_DATA_GET(o, ptr);                        \
  if (!ptr)                                          \
    {                                                \
       CRITICAL("No widget data for object %p (%s)", \
                o, evas_object_type_get(o));         \
       return;                                       \
    }

#define ELM_LABEL_DATA_GET_OR_RETURN_VAL(o, ptr, val) \
  ELM_LABEL_DATA_GET(o, ptr);                         \
  if (!ptr)                                           \
    {                                                 \
       CRITICAL("No widget data for object %p (%s)",  \
                o, evas_object_type_get(o));          \
       return val;                                    \
    }

#define ELM_LABEL_CHECK(obj)                     \
  if (!obj || !elm_widget_type_check             \
        ((obj), ELM_LABEL_SMART_NAME, __func__)) \
    return

#endif
