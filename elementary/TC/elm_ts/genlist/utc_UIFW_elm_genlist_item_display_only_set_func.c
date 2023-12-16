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


Evas_Object *main_win, *genlist;
Elm_Genlist_Item_Class itc;

static void startup(void);
static void cleanup(void);

void (*tet_startup)(void) = startup;
void (*tet_cleanup)(void) = cleanup;

static void utc_UIFW_elm_genlist_item_display_only_set_func_01(void);
static void utc_UIFW_elm_genlist_item_display_only_set_func_02(void);

enum {
	POSITIVE_TC_IDX = 0x01,
	NEGATIVE_TC_IDX,
};

struct tet_testlist tet_testlist[] = {
	{ utc_UIFW_elm_genlist_item_display_only_set_func_01, POSITIVE_TC_IDX },
	{ utc_UIFW_elm_genlist_item_display_only_set_func_02, NEGATIVE_TC_IDX },
	{ NULL, 0 }
};

static void startup(void)
{
	tet_infoline("[[ TET_MSG ]]:: ============ Startup ============ ");
	elm_init(0, NULL);
	main_win = elm_win_add(NULL, "main", ELM_WIN_BASIC);
	evas_object_show(main_win);
	genlist = elm_genlist_add(main_win);
	evas_object_show(genlist);
	elm_win_resize_object_add(main_win, genlist);
	evas_object_size_hint_align_set(genlist, EVAS_HINT_FILL, EVAS_HINT_FILL);
	evas_object_size_hint_weight_set(genlist, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	itc.item_style = "1line_textonly";
	itc.func.text_get = NULL;
	itc.func.content_get = NULL;
	itc.func.state_get = NULL;
	itc.func.del = NULL;
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

/**
 * @brief Positive test case of elm_genlist_item_display_only_set()
 */
static void utc_UIFW_elm_genlist_item_display_only_set_func_01(void)
{
	Elm_Object_Item *item = NULL;
	Eina_Bool ret = EINA_FALSE;

	item = elm_genlist_item_append(genlist, &itc, (void *) 0, NULL, ELM_GENLIST_ITEM_NONE, NULL, NULL);
	elm_genlist_item_display_only_set(item, EINA_TRUE);

	ret = elm_genlist_item_display_only_get(item);

	if (!ret) {
		tet_infoline("elm_genlist_item_display_only_set() failed in positive test case");
		tet_result(TET_FAIL);
		return;
	}
	tet_result(TET_PASS);
}

/**
 * @brief Negative test case of ug_init elm_genlist_item_display_only_set()
 */
static void utc_UIFW_elm_genlist_item_display_only_set_func_02(void)
{
	Elm_Object_Item *item = NULL;

	item = elm_genlist_item_append(genlist, &itc, (void *) 0, NULL, ELM_GENLIST_ITEM_NONE, NULL, NULL);
	elm_genlist_item_display_only_set(NULL, EINA_FALSE);

	tet_result(TET_PASS);
}
