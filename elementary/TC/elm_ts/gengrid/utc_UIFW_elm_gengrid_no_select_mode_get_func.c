#include <tet_api.h>
#include <Elementary.h>

// Definitions
// For checking the result of the positive test case.
#define TET_CHECK_PASS(x1, y...) \
{ \
	if (y == (x1)) \
		{ \
			tet_printf("[TET_CHECK_PASS]:: %s[%d] : Test has failed..", __FILE__,__LINE__); \
			tet_result(TET_FAIL); \
			return; \
		} \
}

// For checking the result of the negative test case.
#define TET_CHECK_FAIL(x1, y...) \
{ \
	if (y != (x1)) \
		{ \
			tet_printf("[TET_CHECK_FAIL]:: %s[%d] : Test has failed..", __FILE__,__LINE__); \
			tet_result(TET_FAIL); \
			return; \
		} \
}

Evas_Object *main_win, *main_bg;
Evas_Object *test_win, *test_bg;
Evas_Object *test_eo = NULL;

void _elm_precondition(void);
static void _win_del(void *data, Evas_Object *obj, void *event_info);

static void _win_del(void *data, Evas_Object *obj, void *event_info)
{
	elm_exit();
}

void _elm_precondition(void)
{
	elm_init(0, NULL);

	main_win = elm_win_add(NULL, "main", ELM_WIN_BASIC);
	elm_win_title_set(main_win, "Elementary Unit Test Suite");
	evas_object_smart_callback_add(main_win, "delete,request", _win_del, NULL);
	main_bg = elm_bg_add(main_win);
	evas_object_size_hint_weight_set(main_bg, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);

	evas_object_resize(main_win, 320, 480);
	evas_object_show(main_win);
}

static void startup(void);
static void cleanup(void);

void (*tet_startup)(void) = startup;
void (*tet_cleanup)(void) = cleanup;

static void utc_UIFW_elm_gengrid_no_select_mode_get_func_01(void);
static void utc_UIFW_elm_gengrid_no_select_mode_get_func_02(void);

enum {
	POSITIVE_TC_IDX = 0x01,
	NEGATIVE_TC_IDX,
};

struct tet_testlist tet_testlist[] = {
	{ utc_UIFW_elm_gengrid_no_select_mode_get_func_01, POSITIVE_TC_IDX },
	{ utc_UIFW_elm_gengrid_no_select_mode_get_func_02, NEGATIVE_TC_IDX },
	{ NULL, 0 }
};

static void startup(void)
{
	tet_infoline("[[ TET_MSG ]]:: ============ Startup ============ ");

	_elm_precondition();

	test_win = elm_win_add(NULL, "Page Control", ELM_WIN_BASIC);
	elm_win_title_set(test_win, "Page Control");
	elm_win_autodel_set(test_win, 1);

	test_bg = elm_bg_add(test_win);
	elm_win_resize_object_add(test_win, test_bg);
	evas_object_size_hint_weight_set(test_bg, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_show(test_bg);

	evas_object_resize(test_win, 480, 800);
	evas_object_show(test_win);

	test_eo = elm_gengrid_add(test_win);

	elm_gengrid_no_select_mode_set(test_eo, EINA_TRUE);

	tet_infoline("[[ TET_MSG ]]:: Completing startup");
}

static void cleanup(void)
{
	if ( NULL != main_win ) {
		main_win = NULL;
	}

	if ( NULL != main_bg ) {
		main_bg = NULL;
	}

	if ( NULL != test_win ) {
		test_win = NULL;
	}

	if ( NULL != test_bg ) {
		test_bg = NULL;
	}

	if ( NULL != test_eo ) {
		test_eo = NULL;
	}

	elm_exit();

	tet_infoline("[[ TET_MSG ]]:: ============ Cleanup ============ ");
}

static void utc_UIFW_elm_gengrid_no_select_mode_get_func_01(void)
{
	Eina_Bool flag = EINA_FALSE;

	flag = elm_gengrid_no_select_mode_get(test_eo);

	TET_CHECK_PASS(EINA_FALSE, flag);

	tet_result(TET_PASS);
	tet_infoline("[[ TET_MSG ]]::[ID]:TC_01, [TYPE]: Positive, [RESULT]:PASS, elm_gengrid_no_select_mode_get");
}

static void utc_UIFW_elm_gengrid_no_select_mode_get_func_02(void)
{
	Eina_Bool flag = EINA_FALSE;

	flag = elm_gengrid_no_select_mode_get(NULL);

	TET_CHECK_FAIL(EINA_FALSE, flag);

	tet_result(TET_PASS);
	tet_infoline("[[ TET_MSG ]]::[ID]:TC_02, [TYPE]: Negative, [RESULT]:PASS, elm_gengrid_no_select_mode_get");
}
