#include <Elementary.h>
#include "elm_priv.h"
#include <sys/time.h>

#define MAX_LENGTH  15000

typedef struct _Dump_Tree_Item Dump_Tree_Item;

struct _Dump_Tree_Item
{
   const char *type;
   void *ptr;
   const char *text;
   int x, y, w, h;
   Eina_Bool visible;
   Eina_List *children;
   struct _Dump_Tree_Item *parent;
};

static Eina_Strbuf *cur_buf;

static int
_object_sort_cb(const void *data1, const void *data2)
{
   Evas_Coord y, y2;

   evas_object_geometry_get(data1, NULL, &y, NULL, NULL);
   evas_object_geometry_get(data2, NULL, &y2, NULL, NULL);

   return y-y2;
}

static void
_add_tree_items(Evas_Object *obj, Dump_Tree_Item *parent)
{
   Dump_Tree_Item *item;
   Eina_List *children, *sorted_children;
   Evas_Object *child;

   item = calloc(1, sizeof(Dump_Tree_Item));
   if(!item) return;

   item->type = eina_stringshare_add(evas_object_type_get(obj));
   item->ptr = obj;
   evas_object_geometry_get(obj, &item->x, &item->y, &item->w, &item->h);
   item->visible = evas_object_visible_get(obj);
   if (!strcmp(item->type, "text"))
      item->text = eina_stringshare_add(evas_object_text_text_get(obj));
   else if (!strcmp(item->type, "textblock"))
     {
        const char *str;
        char *utf_str;
        str = evas_object_textblock_text_markup_get(obj);
        utf_str = evas_textblock_text_markup_to_utf8(obj, str);
        item->text = eina_stringshare_add(utf_str);
        free(utf_str);
     }

   item->parent = parent;
   parent->children = eina_list_append(parent->children, item);

   children = evas_object_smart_members_get(obj);
   sorted_children = eina_list_sort(children, eina_list_count(children), _object_sort_cb);

   EINA_LIST_FREE(sorted_children, child)
      _add_tree_items(child, item);
}

static void
_add_item_string(Eina_List *children, Eina_List *bufs, int depth)
{
   Eina_List *l;
   Dump_Tree_Item *item;
   int i;

   if (eina_strbuf_length_get(cur_buf) > MAX_LENGTH)
     {
        Eina_Strbuf *strbuf = NULL;

        strbuf = eina_strbuf_new();
        if (!strbuf) return;
        bufs = eina_list_append(bufs, strbuf);
        cur_buf = strbuf;
     }

   EINA_LIST_FOREACH(children, l, item)
     {
        for (i = 0; i < depth; i++)
           eina_strbuf_append(cur_buf, " ");

        eina_strbuf_append_printf(cur_buf, "[%s,", item->type);
        eina_strbuf_append_printf(cur_buf, "%p,", item->ptr);
        if (item->text) eina_strbuf_append_printf(cur_buf, "\"%s\",", item->text);
        eina_strbuf_append_printf(cur_buf, "%d,%d,%d,%d,%d {", item->x, item->y, item->w, item->h, item->visible);

        if (item->children)
          {
             eina_strbuf_append(cur_buf, "\n");
             _add_item_string(item->children, bufs, depth + 1);

             for (i = 0; i < depth; i++)
                eina_strbuf_append(cur_buf, " ");
          }
        eina_strbuf_append(cur_buf, "}]\n");
     }
}

static Eina_Bool
_item_can_be_added(Dump_Tree_Item *item)
{
   Dump_Tree_Item *parent, *child;
   Eina_List *l;

   if (!strncmp(item->type, "elm", 3))
      return EINA_TRUE;
   else if (!strcmp(item->type, "edje"))
     {
        // widget layout
        parent = item->parent;
        if (!strncmp(parent->type, "elm", 3) &&
            item->x == parent->x && item->y == parent->y &&
            item->w == parent->w && item->h == parent->h)
           return EINA_FALSE;

        // if the edje has valid child objects
        EINA_LIST_FOREACH(item->children, l, child)
          {
             if (!strncmp(child->type, "elm", 3))
                return EINA_TRUE;
             else if (!strcmp(child->type, "text") || !strcmp(child->type, "textblock"))
               {
                  if (child->text && strlen(child->text))
                     return EINA_TRUE;
               }
          }
     }

   return EINA_FALSE;
}


static char *
_get_text(Dump_Tree_Item *item)
{
   Dump_Tree_Item *child;
   Eina_List *children = NULL, *l;
   Eina_Strbuf *strbuf = NULL;

   // if it is an elm_widget,
   if (!strncmp(item->type, "elm", 3) && item->children)
     {
        child = eina_list_data_get(item->children);
        if (!strcmp(child->type, "edje") && !_item_can_be_added(child))
           children = child->children;
     }
   // if it is an edje object,
   else if (!strcmp(item->type, "edje"))
      children = item->children;

   EINA_LIST_FOREACH(children, l, child)
     {
        if (!strcmp(child->type, "text") || !strcmp(child->type, "textblock"))
          {
             if (child->text && strlen(child->text))
               {
                  if (!strbuf) strbuf = eina_strbuf_new();
                  else eina_strbuf_append(strbuf, ",");
                  eina_strbuf_append_printf(strbuf, "\"%s\"", child->text);
               }
          }
     }

   if (strbuf)
     {
        char *text = NULL;
        text = eina_strbuf_string_steal(strbuf);
        eina_strbuf_free(strbuf);
        return text;
     }
   return NULL;
}

static void
_add_item_string_for_sdb(Eina_List *children, Eina_List *bufs)
{
   Eina_List *l;
   Dump_Tree_Item *item;
   Eina_Bool item_add = EINA_FALSE;
   char *str;

   if (eina_strbuf_length_get(cur_buf) > MAX_LENGTH)
     {
        Eina_Strbuf *strbuf = NULL;

        strbuf = eina_strbuf_new();
        if (!strbuf) return;
        bufs = eina_list_append(bufs, strbuf);
        cur_buf = strbuf;
     }

   EINA_LIST_FOREACH(children, l, item)
     {
        item_add = _item_can_be_added(item);

        /* add item info */
        if (item_add)
          {
             if (!strncmp(item->type, "elm", 3)) eina_strbuf_append_printf(cur_buf, "[%s,", item->type + 4);
             else eina_strbuf_append_printf(cur_buf, "[%s,", item->type);
             eina_strbuf_append_printf(cur_buf, "%p,", item->ptr);
             str = _get_text(item);
             if (str)
               {
                  eina_strbuf_append_printf(cur_buf, "%s,", str);
                  free(str);
               }
             else eina_strbuf_append(cur_buf, "\"\",");
             eina_strbuf_append_printf(cur_buf, "%d,%d,%d,%d,%d { ", item->x, item->y, item->w, item->h, item->visible);
          }

        /* call child node */
        if (item->children)
           _add_item_string_for_sdb(item->children, bufs);

        /* close the item info */
        if (item_add)
           eina_strbuf_append(cur_buf, " } ]");

        str = NULL;
        item_add = EINA_FALSE;
     }
}

static Dump_Tree_Item *
_get_item_info(Eina_List *tree, char *id)
{
   Eina_List *l;
   Dump_Tree_Item *item, *child;
   const char *str;

   EINA_LIST_FOREACH(tree, l, item)
     {
        str = eina_stringshare_printf("%p", item->ptr);
        if (str && !strcmp(str, id))
          {
             eina_stringshare_del(str);
             return item;
          }

        if (item->children)
          {
             child = _get_item_info(item->children, id);
             if (child) return child;
          }
     }
     return NULL;
}

/**
 * Create a object dump tree
 * It creates the tree from the evas of the root object
 *
 * @param root The root object
 * @return The list of Dump_Tree_Item
 */
EAPI Eina_List *
tree_create(Evas_Object *root)
{
   Evas *e;
   Eina_List *tree = NULL;
   Dump_Tree_Item *item;
   Eina_List *objs, *l;
   Evas_Object *obj;

   /* root node */
   item = calloc(1, sizeof(Dump_Tree_Item));
   if (!item) return NULL;

   item->type = eina_stringshare_add(evas_object_type_get(root));
   item->ptr = root;
   evas_object_geometry_get(root, &item->x, &item->y, &item->w, &item->h);
   item->visible = evas_object_visible_get(root);

   tree = eina_list_append(tree, item);

   /* children node*/
   e = evas_object_evas_get(root);
   objs = evas_objects_in_rectangle_get(e, SHRT_MIN, SHRT_MIN, USHRT_MAX, USHRT_MAX, EINA_TRUE, EINA_TRUE);

   // remove root object from the list
   EINA_LIST_FOREACH(objs, l, obj)
     {
        if (obj == root)
          {
             objs = eina_list_remove_list(objs, l);
             break;
          }
     }

   //FIXME: other object except for elm_win
   EINA_LIST_FREE(objs, obj)
      _add_tree_items(obj, item);

   return tree;
}

/**
 * Free the object dump tree
 *
 * @param tree The object dump tree
 */
EAPI void
tree_free(Eina_List *tree)
{
   Dump_Tree_Item *item;
   if (!tree) return;

   EINA_LIST_FREE(tree, item)
     {
        if (item->type) eina_stringshare_del(item->type);
        if (item->text) eina_stringshare_del(item->text);
        if (item->children)
           tree_free(item->children);
        free(item);
     }
}

/**
 * Get a string of the tree items
 *
 * @param tree The object dump tree
 * @return the list of Eina_Strbuf
 */
EAPI Eina_List *
tree_string_get(Eina_List *tree)
{
   Eina_List *bufs = NULL;
   Eina_Strbuf *strbuf = NULL;
   if (!tree) return NULL;

   strbuf = eina_strbuf_new();
   if (!strbuf) return NULL;

   bufs = eina_list_append(bufs, strbuf);
   cur_buf = strbuf;

   eina_strbuf_append(cur_buf, "[Object,Address,(Text),X,Y,W,H,Visible]\n");
   _add_item_string(tree, bufs, 0);
   eina_strbuf_append_printf(cur_buf, "\nOK\n");

   cur_buf = NULL;

   return bufs;
}

/**
 * Get a string of the tree items for sdb
 *
 * @param tree The object dump tree
 * @return the list of Eina_Strbuf
 */
EAPI Eina_List *
tree_string_get_for_sdb(Eina_List *tree)
{
   Eina_List *bufs = NULL;
   Eina_Strbuf *strbuf = NULL;
   if (!tree) return NULL;

   strbuf = eina_strbuf_new();
   if (!strbuf) return NULL;

   bufs = eina_list_append(bufs, strbuf);
   cur_buf = strbuf;

   eina_strbuf_append(cur_buf, "+DUMPWND:");
   _add_item_string_for_sdb(tree, bufs);
   eina_strbuf_append_printf(cur_buf, "\r\nOK\r\n");

   cur_buf = NULL;

   return bufs;
}

/**
 * Get a string for the command from sdb server
 *
 * @param tree The object dump tree
 * @param data Command from the sdb server
 * @return The tree string (it should be freed)
 */
EAPI char *
command_for_sdb(Eina_List *tree, char *data)
{
   char *str = NULL;
   char *command, *id, *text, *result;
   Dump_Tree_Item *item = NULL;
   Eina_Strbuf *buf = NULL;

   if (!tree) return NULL;

   // get command, arguments
   str = strdup(data);
   if (!str) return NULL;

   command = strtok(str + 2, "=");
   if (!command)
     {
        free(str);
        return NULL;
     }
   id = strtok(NULL, ",");
   text = strtok(NULL, ",");

   item = _get_item_info(tree, id);
   if (!item)
     {
        free(str);
        return strdup("Invalid Item!!\r\nOK\r\n");
     }

   // response for the command
   buf = eina_strbuf_new();
   if (!buf)
     {
        free(str);
        return NULL;
     }

   eina_strbuf_append_printf(buf, "%s:", command);

   if (!strcmp(command, "+GETOTEXT"))
     {
        result = _get_text(item);
        if (result)
          {
             eina_strbuf_append_printf(buf, "%s,", result);
             free(result);
          }
     }
   else if (!strcmp(command, "+SETOTEXT"))
     {
        if (!strcmp(item->type, "elm_entry"))
             elm_object_text_set(item->ptr, text);
     }
   else if (!strcmp(command, "+CLROTEXT"))
     {
        if (!strcmp(item->type, "elm_entry"))
             elm_object_text_set(item->ptr, "");
     }
   else if (!strcmp(command, "+GETPARAM"))
     {
        // common data
        if (!strncmp(item->type, "elm", 3)) eina_strbuf_append_printf(buf, "%s,", item->type + 4);
        else eina_strbuf_append_printf(buf, "%s,", item->type);
        eina_strbuf_append_printf(buf, "%p,", item->ptr);
        eina_strbuf_append_printf(buf, "%d,%d,%d,%d,%d,", item->x, item->y, item->w, item->h, item->visible);

        // widget specific data
        if (!strcmp(item->type, "elm_button"))
          {
             result = (char *)(elm_object_text_get(item->ptr));
             if (result) eina_strbuf_append_printf(buf, "\"%s\"", result);
             else eina_strbuf_append(buf, "\"\"");
          }
        else if (!strcmp(item->type, "elm_check"))
          {
             result = (char *)(elm_object_text_get(item->ptr));
             if (result) eina_strbuf_append_printf(buf, "\"%s\",%d", result, elm_check_state_get(item->ptr));
             else eina_strbuf_append_printf(buf, "\"\",%d", elm_check_state_get(item->ptr));
          }
        else if (!strcmp(item->type, "elm_radio"))
          {
             result = (char *)(elm_object_text_get(item->ptr));
             if (result) eina_strbuf_append_printf(buf, "\"%s\",%d", result, elm_radio_state_value_get(item->ptr));
             else eina_strbuf_append_printf(buf, "\"\",%d", elm_radio_state_value_get(item->ptr));
          }
        else if (!strcmp(item->type, "elm_naviframe"))
          {
             result = (char *)(elm_object_item_text_get(elm_naviframe_top_item_get(item->ptr)));
             if (result) eina_strbuf_append_printf(buf, "\"%s\"", result);
             else eina_strbuf_append_printf(buf, "\"\"");
          }
        else if (!strcmp(item->type, "elm_popup"))
          {
             result = (char *)(elm_object_part_text_get(item->ptr, "title,text"));
             if (result) eina_strbuf_append_printf(buf, "\"%s\"", result);
             else eina_strbuf_append(buf, "\"\"");
          }
        else if (!strcmp(item->type, "elm_calendar"))
          {
             struct tm stime1;
             elm_calendar_selected_time_get(item->ptr, &stime1);
             eina_strbuf_append_printf(buf, "%d-%d-%d", stime1.tm_year + 1900, stime1.tm_mon + 1, stime1.tm_mday);
          }
        else if (!strcmp(item->type, "elm_datetime"))
          {
             struct tm stime2;
             elm_datetime_value_get(item->ptr, &stime2);
             eina_strbuf_append_printf(buf, "%d-%d-%d %d:%d", stime2.tm_year + 1900, stime2.tm_mon + 1, stime2.tm_mday, stime2.tm_hour, stime2.tm_min);
          }
        else if (!strcmp(item->type, "elm_label"))
          {
             const char *markup;
             markup = elm_object_text_get(item->ptr);
             result = evas_textblock_text_markup_to_utf8(NULL, markup);
             if (result) eina_strbuf_append_printf(buf, "\"%s\"", result);
             else eina_strbuf_append(buf, "\"\"");
          }
        else if (!strcmp(item->type, "elm_entry"))
          {
             Ecore_IMF_Context *context = elm_entry_imf_context_get(item->ptr);
             const char *markup = elm_object_text_get(item->ptr);
             result = evas_textblock_text_markup_to_utf8(NULL, markup);
             if (result) eina_strbuf_append_printf(buf, "\"%s\",%d", result, ecore_imf_context_input_panel_state_get(context));
             else eina_strbuf_append_printf(buf, "\"\",%d", ecore_imf_context_input_panel_state_get(context));
          }
     }

   eina_strbuf_append_printf(buf, "\r\nOK\r\n");

   result = NULL;
   result = eina_strbuf_string_steal(buf);
   eina_strbuf_free(buf);
   free(str);

   return result;
}

// module api funcs needed
EAPI int
elm_modapi_init(void *m __UNUSED__)
{
   return 1; // succeed always
}

EAPI int
elm_modapi_shutdown(void *m __UNUSED__)
{
   return 1; // succeed always
}
