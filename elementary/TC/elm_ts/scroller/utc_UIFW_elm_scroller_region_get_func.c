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

static void utc_UIFW_elm_scroller_region_get_func_01(void);
static void utc_UIFW_elm_scroller_region_get_func_02(void);

enum {
	POSITIVE_TC_IDX = 0x01,
	NEGATIVE_TC_IDX,
};

struct tet_testlist tet_testlist[] = {
	{ utc_UIFW_elm_scroller_region_get_func_01, POSITIVE_TC_IDX },
	{ utc_UIFW_elm_scroller_region_get_func_02, NEGATIVE_TC_IDX },
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
 * @brief Positive test case of elm_scroller_region_get()
 */
static void utc_UIFW_elm_scroller_region_get_func_01(void)
{
	int x, y, w, h;

	Evas_Object *test_scroller = NULL;

	test_scroller = elm_scroller_add(main_win);

	// Current return type of this API is "Void".
	elm_scroller_region_get(test_scroller, &x, &y, &w, &h);

	if ( x < 0 || y < 0 || w < 0 || h < 0 )	{
		tet_result(TET_FAIL);
		tet_infoline("[[[ TET_MSG ]]]::[ID]: TC_01, [TYPE]: Positive, [RESULT]: FAIL, Getting the currently visible content region has failed.");
	} else {
		tet_result(TET_PASS);
		tet_infoline("[[[ TET_MSG ]]]::[ID]: TC_01, [TYPE]: Positive, [RESULT]: PASS, Getting the currently visible content region had succeed.");
	}
}

/**
 * @brief Negative test case of ug_init elm_scroller_region_get()
 */
static void utc_UIFW_elm_scroller_region_get_func_02(void)
{
	int x, y, w, h;

	// Current return type of this API is "Void"
	elm_scroller_region_get(NULL, &x, &y, &w, &h);

	tet_result(TET_PASS);
	tet_infoline("[[ TET_MSG ]]::[ID]: TC_02, [TYPE]: Negative, [RESULT]: PASS, Getting the currently visible content region has failed.");

}
