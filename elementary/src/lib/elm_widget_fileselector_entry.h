#ifndef ELM_WIDGET_FILESELECTOR_ENTRY_H
#define ELM_WIDGET_FILESELECTOR_ENTRY_H

#include "elm_widget_layout.h"

#ifdef HAVE_EIO
# include <Eio.h>
#endif

/**
 * @addtogroup Widget
 * @{
 *
 * @section elm-fileselector-entry-class The Elementary Fileselector Entry Class
 *
 * Elementary, besides having the @ref Fileselector_Entry widget,
 * exposes its foundation -- the Elementary Fileselector Entry Class
 * -- in order to create other widgets which are a fileselector_entry
 * with some more logic on top.
 */

/**
 * @def ELM_FILESELECTOR_ENTRY_CLASS
 *
 * Use this macro to cast whichever subclass of
 * #Elm_Fileselector_Entry_Smart_Class into it, so to access its fields.
 *
 * @ingroup Widget
 */
#define ELM_FILESELECTOR_ENTRY_CLASS(x) ((Elm_Fileselector_Entry_Smart_Class *)x)

/**
 * @def ELM_FILESELECTOR_ENTRY_DATA
 *
 * Use this macro to cast whichever subdata of
 * #Elm_Fileselector_Entry_Smart_Data into it, so to access its fields.
 *
 * @ingroup Widget
 */
#define ELM_FILESELECTOR_ENTRY_DATA(x)  ((Elm_Fileselector_Entry_Smart_Data *)x)

/**
 * @def ELM_FILESELECTOR_ENTRY_SMART_CLASS_VERSION
 *
 * Current version for Elementary fileselector_entry @b base smart
 * class, a value which goes to
 * _Elm_Fileselector_Entry_Smart_Class::version.
 *
 * @ingroup Widget
 */
#define ELM_FILESELECTOR_ENTRY_SMART_CLASS_VERSION 1

/**
 * @def ELM_FILESELECTOR_ENTRY_SMART_CLASS_INIT
 *
 * Initializer for a whole #Elm_Fileselector_Entry_Smart_Class structure, with
 * @c NULL values on its specific fields.
 *
 * @param smart_class_init initializer to use for the "base" field
 * (#Evas_Smart_Class).
 *
 * @see EVAS_SMART_CLASS_INIT_NULL
 * @see EVAS_SMART_CLASS_INIT_NAME_VERSION
 * @see ELM_FILESELECTOR_ENTRY_SMART_CLASS_INIT_NULL
 * @see ELM_FILESELECTOR_ENTRY_SMART_CLASS_INIT_NAME_VERSION
 *
 * @ingroup Widget
 */
#define ELM_FILESELECTOR_ENTRY_SMART_CLASS_INIT(smart_class_init) \
  {smart_class_init, ELM_FILESELECTOR_ENTRY_SMART_CLASS_VERSION}

/**
 * @def ELM_FILESELECTOR_ENTRY_SMART_CLASS_INIT_NULL
 *
 * Initializer to zero out a whole
 * #Elm_Fileselector_Entry_Smart_Class structure.
 *
 * @see ELM_FILESELECTOR_ENTRY_SMART_CLASS_INIT_NAME_VERSION
 * @see ELM_FILESELECTOR_ENTRY_SMART_CLASS_INIT
 *
 * @ingroup Widget
 */
#define ELM_FILESELECTOR_ENTRY_SMART_CLASS_INIT_NULL \
  ELM_FILESELECTOR_ENTRY_SMART_CLASS_INIT(EVAS_SMART_CLASS_INIT_NULL)

/**
 * @def ELM_FILESELECTOR_ENTRY_SMART_CLASS_INIT_NAME_VERSION
 *
 * Initializer to zero out a whole
 * #Elm_Fileselector_Entry_Smart_Class structure and set its name and
 * version.
 *
 * This is similar to #ELM_FILESELECTOR_ENTRY_SMART_CLASS_INIT_NULL,
 * but it will also set the version field of
 * #Elm_Fileselector_Entry_Smart_Class (base field) to the latest
 * #ELM_FILESELECTOR_ENTRY_SMART_CLASS_VERSION and name it to the
 * specific value.
 *
 * It will keep a reference to the name field as a <c>"const char *"</c>,
 * i.e., the name must be available while the structure is
 * used (hint: static or global variable!) and must not be modified.
 *
 * @see ELM_FILESELECTOR_ENTRY_SMART_CLASS_INIT_NULL
 * @see ELM_FILESELECTOR_ENTRY_SMART_CLASS_INIT
 *
 * @ingroup Widget
 */
#define ELM_FILESELECTOR_ENTRY_SMART_CLASS_INIT_NAME_VERSION(name) \
  ELM_FILESELECTOR_ENTRY_SMART_CLASS_INIT                          \
    (ELM_LAYOUT_SMART_CLASS_INIT_NAME_VERSION(name))

/**
 * Elementary fileselector_entry base smart class. This inherits directly from
 * #Elm_Layout_Smart_Class and is meant to build widgets extending the
 * behavior of a fileselector_entry.
 *
 * All of the functions listed on @ref Fileselector_Entry namespace
 * will work for objects deriving from
 * #Elm_Fileselector_Entry_Smart_Class.
 */
typedef struct _Elm_Fileselector_Entry_Smart_Class
{
   Elm_Layout_Smart_Class base;

   int                    version;    /**< Version of this smart class definition */
} Elm_Fileselector_Entry_Smart_Class;

/**
 * Base entry smart data extended with fileselector_entry instance data.
 */
typedef struct _Elm_Fileselector_Entry_Smart_Data \
Elm_Fileselector_Entry_Smart_Data;
struct _Elm_Fileselector_Entry_Smart_Data
{
   Elm_Layout_Smart_Data base;

   Evas_Object *button;
   Evas_Object *entry;
   char *path;
};

/**
 * @}
 */

EAPI extern const char ELM_FILESELECTOR_ENTRY_SMART_NAME[];
EAPI const Elm_Fileselector_Entry_Smart_Class
*elm_fileselector_entry_smart_class_get(void);

#define ELM_FILESELECTOR_ENTRY_DATA_GET(o, sd) \
  Elm_Fileselector_Entry_Smart_Data * sd = evas_object_smart_data_get(o)

#define ELM_FILESELECTOR_ENTRY_DATA_GET_OR_RETURN(o, ptr) \
  ELM_FILESELECTOR_ENTRY_DATA_GET(o, ptr);                \
  if (!ptr)                                               \
    {                                                     \
       CRITICAL("No widget data for object %p (%s)",      \
                o, evas_object_type_get(o));              \
       return;                                            \
    }

#define ELM_FILESELECTOR_ENTRY_DATA_GET_OR_RETURN_VAL(o, ptr, val) \
  ELM_FILESELECTOR_ENTRY_DATA_GET(o, ptr);                         \
  if (!ptr)                                                        \
    {                                                              \
       CRITICAL("No widget data for object %p (%s)",               \
                o, evas_object_type_get(o));                       \
       return val;                                                 \
    }

#define ELM_FILESELECTOR_ENTRY_CHECK(obj)                     \
  if (!obj || !elm_widget_type_check                          \
        ((obj), ELM_FILESELECTOR_ENTRY_SMART_NAME, __func__)) \
    return

#endif
