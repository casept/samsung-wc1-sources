#include <tet_api.h>
#include <Elementary.h>

// Definitions
// For checking the result of the positive test case.
#define TET_CHECK_PASS(x1, y...) \
{ \
	Evas_Object *err = y; \
	if (err == (x1)) \
		{ \
			tet_printf("[TET_CHECK_PASS]:: %s[%d] : Test has failed..", __FILE__,__LINE__); \
			tet_result(TET_FAIL); \
			return; \
		} \
}

// For checking the result of the negative test case.
#define TET_CHECK_FAIL(x1, y...) \
{ \
	Evas_Object *err = y; \
	if (err != (x1)) \
		{ \
			tet_printf("[TET_CHECK_FAIL]:: %s[%d] : Test has failed..", __FILE__,__LINE__); \
			tet_result(TET_FAIL); \
			return; \
		} \
}


Evas_Object *main_win;
static Elm_Genlist_Item_Class itci;

static void startup(void);
static void cleanup(void);

void (*tet_startup)(void) = startup;
void (*tet_cleanup)(void) = cleanup;

static void utc_UIFW_elm_index_item_append_relative_func_01(void);
static void utc_UIFW_elm_index_item_append_relative_func_02(void);

enum {
	POSITIVE_TC_IDX = 0x01,
	NEGATIVE_TC_IDX,
};

struct tet_testlist tet_testlist[] = {
	{ utc_UIFW_elm_index_item_append_relative_func_01, POSITIVE_TC_IDX },
	{ utc_UIFW_elm_index_item_append_relative_func_02, NEGATIVE_TC_IDX },
	{ NULL, 0 }
};

static void startup(void)
{
	tet_infoline("[[ TET_MSG ]]:: ============ Startup ============ ");
	elm_init(0, NULL);
	main_win = elm_win_add(NULL, "main", ELM_WIN_BASIC);
	evas_object_show(main_win);
}

static void cleanup(void)
{
	if ( NULL != main_win ) {
		evas_object_del(main_win);
	       	main_win = NULL;
	}
	elm_shutdown();
	tet_infoline("[[ TET_MSG ]]:: ============ Cleanup ============ ");
}

char *gli_label_get(void *data, Evas_Object *obj, const char *part)
{
   char buf[256];
   int j = (int)data;
   snprintf(buf, sizeof(buf), "%c%c",
            'A' + ((j >> 4) & 0xf),
            'a' + ((j     ) & 0xf)
            );
   return strdup(buf);
}
/**
 * @brief Positive test case of elm_index_item_append_relative()
 */
static void utc_UIFW_elm_index_item_append_relative_func_01(void)
{
	Evas_Object *idx = NULL;
	Elm_Object_Item *it = NULL, *it_gl=NULL;
	Evas_Object *gl = NULL;
	Elm_Object_Item *it_idx = NULL;
	int i = 0, j = 0;
	const char  *letter = NULL;
	gl = elm_genlist_add(main_win);
   	idx= elm_index_add(main_win);
	evas_object_show(gl);
	evas_object_show(idx);
	itci.item_style     = "default";
        itci.func.text_get = gli_label_get;
        itci.func.content_get  = NULL;
    	itci.func.state_get = NULL;
    	itci.func.del       = NULL;
    	for (i = 0; i <=40; i++) {
      		it = elm_genlist_item_append(gl, &itci,(void *)j, NULL, ELM_GENLIST_ITEM_NONE, NULL,NULL);
      		if ((j & 0xf) == 0) {
			char buf[32];
         		snprintf(buf, sizeof(buf), "%c", 'A' + ((j >> 3) & 0xf));
         		elm_index_item_append(idx, buf, it);
        	}
	  	if(i==0) it_gl=it;
      		j += 2;
    	}
        it = elm_genlist_item_append(gl, &itci,(void *)j, NULL, ELM_GENLIST_ITEM_NONE, NULL,NULL);
        char buf[32];
        snprintf(buf, sizeof(buf), "%c", 'A' + ((j >> 3) & 0xf));
        elm_index_item_append_relative(idx, buf, it, it_gl);
	elm_index_item_go(idx, 0);
        it_idx = elm_index_item_find(idx,(void*)it_gl);
        letter = elm_index_item_letter_get(it_idx);

	if((strcmp(letter,"A")&&(strcmp(buf,"K")))) {
		tet_infoline("elm_index_item_append_relative() failed in positive test case");
		tet_result(TET_FAIL);
	        return;
        }
	tet_result(TET_PASS);
}

/**
 * @brief Negative test case of ug_init elm_index_item_append_relative()
 */
static void utc_UIFW_elm_index_item_append_relative_func_02(void)
{
	Evas_Object *idx = NULL;
	Elm_Object_Item *it = NULL, *it_gl=NULL;
	Evas_Object *gl = NULL;
	Elm_Object_Item *it_idx = NULL;
	int i = 0, j = 0;
	gl = elm_genlist_add(main_win);
   	idx= elm_index_add(main_win);
	evas_object_show(gl);
	evas_object_show(idx);
    itci.item_style     = "default";
    itci.func.text_get = gli_label_get;
    itci.func.content_get  = NULL;
    itci.func.state_get = NULL;
    itci.func.del       = NULL;
    for (i = 0; i <=40; i++) {
      it = elm_genlist_item_append(gl, &itci,(void *)j, NULL, ELM_GENLIST_ITEM_NONE, NULL,NULL);
      if ((j & 0xf) == 0) {
		 char buf[32];
         snprintf(buf, sizeof(buf), "%c", 'A' + ((j >> 3) & 0xf));
         elm_index_item_append(idx, buf, it);
        }
	if(i==0)
         it_gl=it;
        j += 2;
    }
        it = elm_genlist_item_append(gl, &itci,(void *)j, NULL, ELM_GENLIST_ITEM_NONE, NULL,NULL);
          char buf[32];
        snprintf(buf, sizeof(buf), "%c", 'A' + ((j >> 3) & 0xf));
        elm_index_item_append_relative(NULL, buf, it, it_gl);
		elm_index_item_go(idx, 0);
        it_idx = elm_index_item_find(idx,(void*)it);
        if(it_idx) {
			tet_infoline("elm_index_item_append_relative() failed in negative test case");
			tet_result(TET_FAIL);
	        return;
         }
	  tet_result(TET_PASS);
}
