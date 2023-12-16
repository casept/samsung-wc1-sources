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
static char *Items[] = { "Main Item1", "Main Item 2", "Main Item 3", "Main Item 4", "Main Item 5", "Main Item 6", "Main Item 7", "Main Item 8"  };
Elm_Genlist_Item_Class itc;

static void startup(void);
static void cleanup(void);

void (*tet_startup)(void) = startup;
void (*tet_cleanup)(void) = cleanup;

static void utc_UIFW_elm_genlist_longpress_timeout_set_func_01(void);
static void utc_UIFW_elm_genlist_longpress_timeout_set_func_02(void);

enum {
	POSITIVE_TC_IDX = 0x01,
	NEGATIVE_TC_IDX,
};

struct tet_testlist tet_testlist[] = {
	{ utc_UIFW_elm_genlist_longpress_timeout_set_func_01, POSITIVE_TC_IDX },
	{ utc_UIFW_elm_genlist_longpress_timeout_set_func_02, NEGATIVE_TC_IDX },
	{ NULL, 0 }
};

static char *_gl_text_get( const void *data, Evas_Object *obj, const char *part )
{
	int index = (int) data;

	if (!strcmp(part, "elm.text")) {
		return strdup(Items[index]);
	}
	return NULL;
}

static void startup(void)
{
	Elm_Object_Item *item = NULL;
	int index = 0;
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
	itc.func.text_get = _gl_text_get;
	itc.func.content_get = NULL;
	itc.func.state_get = NULL;
	itc.func.del = NULL;
	for (index = 0; index < 8; index++) {
		item = elm_genlist_item_append(genlist, &itc, (void *) index, NULL,
				ELM_GENLIST_ITEM_NONE, NULL, NULL);
	}
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
 * @brief Positive test case of elm_genlist_longpress_timeout_set()
 */
static void utc_UIFW_elm_genlist_longpress_timeout_set_func_01(void)
{
	elm_genlist_longpress_timeout_set(genlist, 1);
	tet_result(TET_PASS);
}

/**
 * @brief Negative test case of ug_init elm_genlist_longpress_timeout_set()
 */
static void utc_UIFW_elm_genlist_longpress_timeout_set_func_02(void)
{
	elm_genlist_longpress_timeout_set(NULL, 1);
	tet_result(TET_PASS);
}
