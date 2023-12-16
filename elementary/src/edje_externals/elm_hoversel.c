#include "private.h"

typedef struct _Elm_Params_Hoversel
{
   Elm_Params base;
   const char *label;
   Evas_Object *icon;
   Eina_Bool horizontal:1;
   Eina_Bool horizontal_exists:1;
} Elm_Params_Hoversel;

static void
external_hoversel_state_set(void *data __UNUSED__, Evas_Object *obj, const void *from_params, const void *to_params, float pos __UNUSED__)
{
   const Elm_Params_Hoversel *p;

   if (to_params) p = to_params;
   else if (from_params) p = from_params;
   else return;

   if (p->label)
     elm_object_text_set(obj, p->label);
   if (p->icon)
     elm_object_part_content_set(obj, "icon", p->icon);
   if (p->horizontal_exists)
      elm_hoversel_horizontal_set(obj, p->horizontal);
}

static Eina_Bool
external_hoversel_param_set(void *data __UNUSED__, Evas_Object *obj, const Edje_External_Param *param)
{
   if (!strcmp(param->name, "label"))
     {
	if (param->type == EDJE_EXTERNAL_PARAM_TYPE_STRING)
	  {
	     elm_object_text_set(obj, param->s);
	     return EINA_TRUE;
	  }
     }
   else if (!strcmp(param->name, "icon"))
     {
	if (param->type == EDJE_EXTERNAL_PARAM_TYPE_STRING)
	  {
	     Evas_Object *icon = external_common_param_icon_get(obj, param);
	     if ((strcmp(param->s, "")) && (!icon)) return EINA_FALSE;
	     elm_object_part_content_set(obj, "icon", icon);
	     return EINA_TRUE;
	  }
     }
   else if (!strcmp(param->name, "horizontal"))
     {
	if (param->type == EDJE_EXTERNAL_PARAM_TYPE_BOOL)
	  {
	     elm_hoversel_horizontal_set(obj, param->i);
	     return EINA_TRUE;
	  }
     }

   ERR("unknown parameter '%s' of type '%s'",
       param->name, edje_external_param_type_str(param->type));

   return EINA_FALSE;
}

static Eina_Bool
external_hoversel_param_get(void *data __UNUSED__, const Evas_Object *obj, Edje_External_Param *param)
{
   if (!strcmp(param->name, "label"))
     {
	if (param->type == EDJE_EXTERNAL_PARAM_TYPE_STRING)
	  {
	     param->s = elm_object_text_get(obj);
	     return EINA_TRUE;
	  }
     }
   else if (!strcmp(param->name, "icon"))
     {
	/* not easy to get icon name back from live object */
	return EINA_FALSE;
     }
   else if (!strcmp(param->name, "horizontal"))
     {
	if (param->type == EDJE_EXTERNAL_PARAM_TYPE_BOOL)
	  {
	     param->i = elm_hoversel_horizontal_get(obj);
	     return EINA_TRUE;
	  }
     }

   ERR("unknown parameter '%s' of type '%s'",
       param->name, edje_external_param_type_str(param->type));

   return EINA_FALSE;
}

static void *
external_hoversel_params_parse(void *data __UNUSED__, Evas_Object *obj, const Eina_List *params)
{
   Elm_Params_Hoversel *mem;
   Edje_External_Param *param;
   const Eina_List *l;

   mem = calloc(1, sizeof(Elm_Params_Hoversel));
   if (!mem)
     return NULL;

   external_common_icon_param_parse(&mem->icon, obj, params);

   EINA_LIST_FOREACH(params, l, param)
     {
	if (!strcmp(param->name, "horizontal"))
	  {
	     mem->horizontal = !!param->i;
	     mem->horizontal_exists = EINA_TRUE;
	  }
	else if (!strcmp(param->name, "label"))
	  mem->label = eina_stringshare_add(param->s);
     }

   return mem;
}

static Evas_Object *external_hoversel_content_get(void *data __UNUSED__,
		const Evas_Object *obj __UNUSED__, const char *content __UNUSED__)
{
	ERR("No content.");
	return NULL;
}

 static void
external_hoversel_params_free(void *params)
{
   Elm_Params_Hoversel *mem = params;
   if (mem->label)
      eina_stringshare_del(mem->label);
   free(params);
}

static Edje_External_Param_Info external_hoversel_params[] = {
   DEFINE_EXTERNAL_COMMON_PARAMS,
   EDJE_EXTERNAL_PARAM_INFO_STRING("label"),
   EDJE_EXTERNAL_PARAM_INFO_STRING("icon"),
   EDJE_EXTERNAL_PARAM_INFO_BOOL("horizontal"),
   EDJE_EXTERNAL_PARAM_INFO_SENTINEL
};

DEFINE_EXTERNAL_ICON_ADD(hoversel, "hoversel");
DEFINE_EXTERNAL_TYPE_SIMPLE(hoversel, "Hoversel");
