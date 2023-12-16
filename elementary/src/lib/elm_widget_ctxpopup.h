#ifndef ELM_WIDGET_CTXPOPUP_H
#define ELM_WIDGET_CTXPOPUP_H

#include "elm_widget_layout.h"

#define MAX_ITEMS_PER_VIEWPORT 15
/**
 * @addtogroup Widget
 * @{
 *
 * @section elm-ctxpopup-class The Elementary Ctxpopup Class
 *
 * Elementary, besides having the @ref Ctxpopup widget, exposes its
 * foundation -- the Elementary Ctxpopup Class -- in order to create other
 * widgets which are a ctxpopup with some more logic on top.
 */

/**
 * @def ELM_CTXPOPUP_CLASS
 *
 * Use this macro to cast whichever subclass of
 * #Elm_Ctxpopup_Smart_Class into it, so to access its fields.
 *
 * @ingroup Widget
 */
#define ELM_CTXPOPUP_CLASS(x) ((Elm_Ctxpopup_Smart_Class *)x)

/**
 * @def ELM_CTXPOPUP_DATA
 *
 * Use this macro to cast whichever subdata of
 * #Elm_Ctxpopup_Smart_Data into it, so to access its fields.
 *
 * @ingroup Widget
 */
#define ELM_CTXPOPUP_DATA(x)  ((Elm_Ctxpopup_Smart_Data *)x)

/**
 * @def ELM_CTXPOPUP_SMART_CLASS_VERSION
 *
 * Current version for Elementary ctxpopup @b base smart class, a value
 * which goes to _Elm_Ctxpopup_Smart_Class::version.
 *
 * @ingroup Widget
 */
#define ELM_CTXPOPUP_SMART_CLASS_VERSION 1

/**
 * @def ELM_CTXPOPUP_SMART_CLASS_INIT
 *
 * Initializer for a whole #Elm_Ctxpopup_Smart_Class structure, with
 * @c NULL values on its specific fields.
 *
 * @param smart_class_init initializer to use for the "base" field
 * (#Evas_Smart_Class).
 *
 * @see EVAS_SMART_CLASS_INIT_NULL
 * @see EVAS_SMART_CLASS_INIT_NAME_VERSION
 * @see ELM_CTXPOPUP_SMART_CLASS_INIT_NULL
 * @see ELM_CTXPOPUP_SMART_CLASS_INIT_NAME_VERSION
 *
 * @ingroup Widget
 */
#define ELM_CTXPOPUP_SMART_CLASS_INIT(smart_class_init) \
  {smart_class_init, ELM_CTXPOPUP_SMART_CLASS_VERSION}

/**
 * @def ELM_CTXPOPUP_SMART_CLASS_INIT_NULL
 *
 * Initializer to zero out a whole #Elm_Ctxpopup_Smart_Class structure.
 *
 * @see ELM_CTXPOPUP_SMART_CLASS_INIT_NAME_VERSION
 * @see ELM_CTXPOPUP_SMART_CLASS_INIT
 *
 * @ingroup Widget
 */
#define ELM_CTXPOPUP_SMART_CLASS_INIT_NULL \
  ELM_CTXPOPUP_SMART_CLASS_INIT(EVAS_SMART_CLASS_INIT_NULL)

/**
 * @def ELM_CTXPOPUP_SMART_CLASS_INIT_NAME_VERSION
 *
 * Initializer to zero out a whole #Elm_Ctxpopup_Smart_Class structure and
 * set its name and version.
 *
 * This is similar to #ELM_CTXPOPUP_SMART_CLASS_INIT_NULL, but it will
 * also set the version field of #Elm_Ctxpopup_Smart_Class (base field)
 * to the latest #ELM_CTXPOPUP_SMART_CLASS_VERSION and name it to the
 * specific value.
 *
 * It will keep a reference to the name field as a <c>"const char *"</c>,
 * i.e., the name must be available while the structure is
 * used (hint: static or global variable!) and must not be modified.
 *
 * @see ELM_CTXPOPUP_SMART_CLASS_INIT_NULL
 * @see ELM_CTXPOPUP_SMART_CLASS_INIT
 *
 * @ingroup Widget
 */
#define ELM_CTXPOPUP_SMART_CLASS_INIT_NAME_VERSION(name) \
  ELM_CTXPOPUP_SMART_CLASS_INIT(ELM_LAYOUT_SMART_CLASS_INIT_NAME_VERSION(name))

/**
 * Elementary ctxpopup base smart class. This inherits directly from
 * #Elm_Layout_Smart_Class and is meant to build widgets extending the
 * behavior of a ctxpopup.
 *
 * All of the functions listed on @ref Ctxpopup namespace will work for
 * objects deriving from #Elm_Ctxpopup_Smart_Class.
 */
typedef struct _Elm_Ctxpopup_Smart_Class
{
   Elm_Layout_Smart_Class base;

   int                    version; /**< Version of this smart class definition */
} Elm_Ctxpopup_Smart_Class;

typedef struct _Elm_Ctxpopup_Item       Elm_Ctxpopup_Item;

/**
 * Base widget smart data extended with ctxpopup instance data.
 */
typedef struct _Elm_Ctxpopup_Smart_Data Elm_Ctxpopup_Smart_Data;

struct _Elm_Ctxpopup_Item
{
   ELM_WIDGET_ITEM;

   const char   *label;
   Evas_Object  *icon;
   Evas_Smart_Cb func;
   Evas_Object  *btn;
};

struct _Elm_Ctxpopup_Smart_Data
{
   Elm_Layout_Smart_Data  base;

   Evas_Object           *parent;
   Evas_Object           *box;

   Evas_Object           *layout;
   Evas_Object           *arrow;
   Evas_Object           *scr;
   Evas_Object           *bg;
   Evas_Object           *content;
   Eina_List             *items;

   Elm_Ctxpopup_Direction dir;
   Elm_Ctxpopup_Direction dir_priority[4];

   int                    multi_down;

   Eina_Bool              horizontal : 1;
   Eina_Bool              visible : 1;
   Eina_Bool              auto_hide : 1;
   Eina_Bool              mouse_down : 1;
//******************** TIZEN Only
   Eina_Bool              pressed : 1;
   Eina_Bool              dir_requested : 1;
//****************************
#ifndef ELM_FEATURE_WEARABLE
  /*for banded ux*/
   int                    item_color[MAX_ITEMS_PER_VIEWPORT][3];
   Elm_Ctxpopup_Item      *top_drawn_item;
   Eina_Bool              color_calculated : 1;
#endif
};

/**
 * @}
 */

EAPI extern const char ELM_CTXPOPUP_SMART_NAME[];
EAPI const Elm_Ctxpopup_Smart_Class
*elm_ctxpopup_smart_class_get(void);

#define ELM_CTXPOPUP_DATA_GET(o, sd) \
  Elm_Ctxpopup_Smart_Data * sd = evas_object_smart_data_get(o)

#define ELM_CTXPOPUP_DATA_GET_OR_RETURN(o, ptr)      \
  ELM_CTXPOPUP_DATA_GET(o, ptr);                     \
  if (!ptr)                                          \
    {                                                \
       CRITICAL("No widget data for object %p (%s)", \
                o, evas_object_type_get(o));         \
       return;                                       \
    }

#define ELM_CTXPOPUP_DATA_GET_OR_RETURN_VAL(o, ptr, val) \
  ELM_CTXPOPUP_DATA_GET(o, ptr);                         \
  if (!ptr)                                              \
    {                                                    \
       CRITICAL("No widget data for object %p (%s)",     \
                o, evas_object_type_get(o));             \
       return val;                                       \
    }

#define ELM_CTXPOPUP_CHECK(obj)                                          \
  if (!obj || !elm_widget_type_check((obj),                              \
                                     ELM_CTXPOPUP_SMART_NAME, __func__)) \
    return

#endif
