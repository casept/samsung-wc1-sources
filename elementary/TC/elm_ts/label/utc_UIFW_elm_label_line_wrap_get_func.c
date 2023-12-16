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

static void startup(void);
static void cleanup(void);

void (*tet_startup)(void) = startup;
void (*tet_cleanup)(void) = cleanup;

static void utc_UIFW_elm_label_line_wrap_get_func_01(void);
static void utc_UIFW_elm_label_line_wrap_get_func_02(void);

enum {
	POSITIVE_TC_IDX = 0x01,
	NEGATIVE_TC_IDX,
};

struct tet_testlist tet_testlist[] = {
	{ utc_UIFW_elm_label_line_wrap_get_func_01, POSITIVE_TC_IDX },
	{ utc_UIFW_elm_label_line_wrap_get_func_02, NEGATIVE_TC_IDX },
	{ NULL, 0}
};

// Declare the global variables
Evas_Object *main_win, *main_bg;
Evas_Object *test_win, *test_bg;
Evas_Object *test_eo = NULL;
// Declare internal functions
void _elm_precondition(void);
static void _win_del(void *data, Evas_Object *obj, void *event_info);


// Delete main window
static void _win_del(void *data, Evas_Object *obj, void *event_info)
{
	elm_exit();
}

// Do precondition.
void _elm_precondition(void)
{
	elm_init(0, NULL);

	main_win = elm_win_add(NULL, "main", ELM_WIN_BASIC);
	elm_win_title_set(main_win, "Elementary Unit Test Suite");
	evas_object_smart_callback_add(main_win, "delete,request", _win_del, NULL);
	main_bg = elm_bg_add(main_win);
	evas_object_size_hint_weight_set(main_bg, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	elm_win_resize_object_add(main_win, main_bg);
	evas_object_show(main_bg);

	// set an initial window size
	evas_object_resize(main_win, 320, 480);
	// show the window
	evas_object_show(main_win);

	//elm_run();
}


// Start up function for each test purpose
static void
startup()
{
	tet_infoline("[[ TET_MSG ]]:: ============ Startup ============ ");

	// Elm precondition
	_elm_precondition();

	// Test precondition
	test_win = elm_win_add(NULL, "Label", ELM_WIN_BASIC);
	elm_win_title_set(test_win, "Label");
	elm_win_autodel_set(test_win, 1);

	test_bg = elm_bg_add(test_win);
	elm_win_resize_object_add(test_win, test_bg);
	evas_object_size_hint_weight_set(test_bg, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_show(test_bg);

	evas_object_resize(test_win, 480, 800);
	evas_object_show(test_win);

	tet_infoline("[[ TET_MSG ]]:: Completing startup");
}

// Clean up function for each test purpose
static void
cleanup()
{
	tet_infoline("[[ TET_MSG ]]:: ============ Cleanup ============ ");

	// Clean up the used resources.
	if ( NULL != main_win ) {
		evas_object_del(main_win);
		main_win = NULL;
	}

	if ( NULL != main_bg ) {
		evas_object_del(main_bg);
		main_bg = NULL;
	}

	if ( NULL != test_win ) {
		evas_object_del(test_win);
		test_win = NULL;
	}

	if ( NULL != test_bg ) {
		evas_object_del(test_bg);
		test_bg = NULL;
	}

	if ( NULL != test_eo ) {
		evas_object_del(test_eo);
		test_eo = NULL;
	}

	// clean up and shut down
	elm_shutdown(); ;

	tet_infoline("[[ TET_MSG ]]:: ========= TC COMPLETE  ========== ");
}

/**
 * @brief Positive test case of elm_label_line_wrap_get()
 */
static void utc_UIFW_elm_label_line_wrap_get_func_01(void)
{
   test_eo = elm_label_add(test_win);
   elm_label_line_wrap_set(test_eo, 1);
   int ret = 0;
   ret = elm_label_line_wrap_get(test_eo);

   if (!ret) {
        tet_infoline("elm_label_line_wrap_get() failed in positive test case");
        tet_result(TET_FAIL);
        return;
   }
   tet_result(TET_PASS);

}

/**
 * @brief Negative test case of ug_init elm_label_line_wrap_get()
 */
static void utc_UIFW_elm_label_line_wrap_get_func_02(void)
{
   test_eo = elm_label_add(test_win);
   elm_label_line_wrap_set(test_eo, 0);
   int ret = 0;
   ret = elm_label_line_wrap_get(test_eo);

   if(ret) {
        tet_result(TET_FAIL);
        tet_infoline("elm_label_line_wrap_get() failed in negative test case");
        return;
   }
   tet_result(TET_PASS);
}
