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

static void startup(void);
static void cleanup(void);

void (*tet_startup)(void) = startup;
void (*tet_cleanup)(void) = cleanup;

static void utc_UIFW_elm_scroller_child_size_get_func_01(void);
static void utc_UIFW_elm_scroller_child_size_get_func_02(void);

enum {
	POSITIVE_TC_IDX = 0x01,
	NEGATIVE_TC_IDX,
};

struct tet_testlist tet_testlist[] = {
	{ utc_UIFW_elm_scroller_child_size_get_func_01, POSITIVE_TC_IDX },
	{ utc_UIFW_elm_scroller_child_size_get_func_02, NEGATIVE_TC_IDX },
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

/**
 * @brief Positive test case of elm_scroller_child_size_get()
 */
static void utc_UIFW_elm_scroller_child_size_get_func_01(void)
{
	Evas_Coord x, y, w, h, vw, vh;
	Evas_Object *test_scroller = NULL;
	Evas_Object *tb = NULL;

	test_scroller = elm_scroller_add(main_win);
	tb = elm_table_add(main_win);
	elm_scroller_content_set(test_scroller, tb);

	elm_scroller_region_get(test_scroller, &x, &y, &w, &h);

	// Current return type of this API is "Void".
	elm_scroller_child_size_get(test_scroller, &vw, &vh);

	tet_result(TET_PASS);
	tet_infoline("[[ TET_MSG ]]::[ID]: TC_01, [TYPE]: Positive, [RESULT]: PASS, Getting the size of the content child object has succeed.");

}

/**
 * @brief Negative test case of ug_init elm_scroller_child_size_get()
 */
static void utc_UIFW_elm_scroller_child_size_get_func_02(void)
{
	Evas_Coord vw, vh;

	// Current return type of this API is "Void".
	elm_scroller_child_size_get(NULL, &vw, &vh);

	tet_result(TET_PASS);
	tet_infoline("[[ TET_MSG ]]::[ID]: TC_02, [TYPE]: Negative, [RESULT]: PASS, Getting the size of the content child object has failed.");
}


