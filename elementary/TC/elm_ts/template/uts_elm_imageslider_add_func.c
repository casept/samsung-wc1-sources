
#include <Elementary.h>
//#include "winset_test.h"
//#include "winset_until.h"
#include "uts_elm_imageslider_add_test.h"

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
	test_win = elm_win_add(NULL, "Image Silder", ELM_WIN_BASIC);
	elm_win_title_set(test_win, "Image Slider");
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
	// Clean up the used resources.
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

// Positive test case.
void uts_elm_imageslider_add_test_001()
{
	test_eo = elm_imageslider_add(test_win);
	TET_CHECK_PASS(NULL, test_win);

	tet_result(TET_PASS);
	tet_infoline("[[ TET_MSG ]]::[ID]:TC_01, [TYPE]: Positive, [RESULT]:PASS, An Image Slider is added successfully.");

}


// Negative test case.
void uts_elm_imageslider_add_test_002()
{
	test_eo = elm_imageslider_add(NULL);
	TET_CHECK_FAIL(NULL, test_eo);

	tet_result(TET_PASS);
	tet_infoline("[[ TET_MSG ]]::[ID]:TC_02, [TYPE]: Negative, [RESULT]:PASS, Adding an Image Slider has failed.");
}


