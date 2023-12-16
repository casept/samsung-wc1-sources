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

static void utc_UIFW_elm_slider_min_max_set_func_01(void);
static void utc_UIFW_elm_slider_min_max_set_func_02(void);

enum {
	POSITIVE_TC_IDX = 0x01,
	NEGATIVE_TC_IDX,
};

struct tet_testlist tet_testlist[] = {
	{ utc_UIFW_elm_slider_min_max_set_func_01, POSITIVE_TC_IDX },
	{ utc_UIFW_elm_slider_min_max_set_func_02, NEGATIVE_TC_IDX },
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
 * @brief Positive test case of elm_slider_min_max_set()
 */
static void utc_UIFW_elm_slider_min_max_set_func_01(void)
{
        Evas_Object *slider;
        double min = 0.0, max = 0.0;

        slider = elm_slider_add(main_win);

        elm_slider_min_max_set(slider, 1, 9);
        elm_slider_label_set(slider, "Text");
        elm_slider_value_set(slider, 3);
        evas_object_size_hint_weight_set(slider, EVAS_HINT_EXPAND, 0.0);
        evas_object_size_hint_align_set(slider, EVAS_HINT_FILL, 0.5);
        elm_slider_min_max_get(slider, &min, &max);

         if (!(min == 1) || !(max == 9)) {
		tet_infoline("elm_slider_min_max_set() failed in positive test case");
		tet_result(TET_FAIL);
		return;
	}
	tet_result(TET_PASS);
}

/**
 * @brief Negative test case of ug_init elm_slider_min_max_set()
 */
static void utc_UIFW_elm_slider_min_max_set_func_02(void)
{
        Evas_Object *slider;
        double min = 0.0, max = 0.0;

        slider = elm_slider_add(main_win);

        elm_slider_min_max_set(NULL, 1, 9);
        elm_slider_label_set(slider, "Text");
        elm_slider_value_set(slider, 3);
        evas_object_size_hint_weight_set(slider, EVAS_HINT_EXPAND, 0.0);
        evas_object_size_hint_align_set(slider, EVAS_HINT_FILL, 0.5);
        elm_slider_min_max_get(slider, &min, &max);

        if ((min == 1) || (max == 9)) {
		tet_infoline("elm_slider_min_max_set() failed in negative test case");
		tet_result(TET_FAIL);
		return;
	}
	tet_result(TET_PASS);
}
