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

Evas_Object *main_win, *stackedicon;

static char *icon_path[] = {
        "/usr/share/beat_winset_test/icon/Albums_Item/Albums_Item1.jpg",
        "/usr/share/beat_winset_test/icon/Albums_Item/Albums_Item2.jpg",
        "/usr/share/beat_winset_test/icon/Albums_Item/Albums_Item3.jpg",
        "/usr/share/beat_winset_test/icon/Albums_Item/Albums_Item4.jpg",
        "/usr/share/beat_winset_test/icon/Albums_Item/Albums_Item5.jpg",
        "/usr/share/beat_winset_test/icon/Albums_Item/Albums_Item6.jpg",
        "/usr/share/beat_winset_test/icon/Albums_Item/Albums_Item7.jpg",
        "/usr/share/beat_winset_test/icon/Albums_Item/Albums_Item8.jpg",
        "/usr/share/beat_winset_test/icon/Albums_Item/Albums_Item9.jpg",
        "/usr/share/beat_winset_test/icon/Albums_Item/Albums_Item10.jpg",
        "/usr/share/beat_winset_test/icon/Albums_Item/Albums_Item11.jpg",
        "/usr/share/beat_winset_test/icon/Albums_Item/Albums_Item12.jpg",
        "/usr/share/beat_winset_test/icon/Albums_Item/Albums_Item13.jpg",
        "/usr/share/beat_winset_test/icon/Albums_Item/Albums_Item14.jpg",
        "/usr/share/beat_winset_test/icon/Albums_Item/Albums_Item15.jpg",
        "/usr/share/beat_winset_test/icon/Albums_Item/Albums_Item16.jpg",
        "/usr/share/beat_winset_test/icon/Albums_Item/Albums_Item17.jpg",
        "/usr/share/beat_winset_test/icon/Albums_Item/Albums_Item18.jpg",
        "/usr/share/beat_winset_test/icon/Albums_Item/Albums_Item19.jpg",
        "/usr/share/beat_winset_test/icon/Albums_Item/Albums_Item20.jpg",
        "/usr/share/beat_winset_test/icon/Albums_Item/Albums_Item21.jpg",
        "/usr/share/beat_winset_test/icon/Albums_Item/Albums_Item22.jpg",
};

static void startup(void);
static void cleanup(void);

void (*tet_startup)(void) = startup;
void (*tet_cleanup)(void) = cleanup;

static void utc_UIFW_elm_stackedicon_item_prepend_func_01(void);
static void utc_UIFW_elm_stackedicon_item_prepend_func_02(void);

enum {
	POSITIVE_TC_IDX = 0x01,
	NEGATIVE_TC_IDX,
};

struct tet_testlist tet_testlist[] = {
	{ utc_UIFW_elm_stackedicon_item_prepend_func_01, POSITIVE_TC_IDX },
	{ utc_UIFW_elm_stackedicon_item_prepend_func_02, NEGATIVE_TC_IDX },
	{ NULL, 0 }
};

static void startup(void)
{
	tet_infoline("[[ TET_MSG ]]:: ============ Startup ============ ");
	elm_init(0, NULL);
	main_win = elm_win_add(NULL, "main", ELM_WIN_BASIC);
	evas_object_show(main_win);

	stackedicon = elm_stackedicon_add(main_win);
	evas_object_show(stackedicon);
	elm_win_resize_object_add(main_win, stackedicon);
	evas_object_size_hint_align_set(stackedicon, EVAS_HINT_FILL, EVAS_HINT_FILL);
	evas_object_size_hint_weight_set(stackedicon, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
}

static void cleanup(void)
{
	if ( NULL != stackedicon ) {
		evas_object_del(stackedicon);
		stackedicon = NULL;
	}

	if ( NULL != main_win ) {
		evas_object_del(main_win);
	    main_win = NULL;
	}
	elm_shutdown();
	tet_infoline("[[ TET_MSG ]]:: ============ Cleanup ============ ");
}

/**
 * @brief Positive test case of elm_stackedicon_item_prepend()
 */
static void utc_UIFW_elm_stackedicon_item_prepend_func_01(void)
{
	Elm_Stackedicon_Item *it = NULL;

	tet_infoline("[[ DEBUG :: Positive ]]");

	it = elm_stackedicon_item_prepend(stackedicon, icon_path[0]);
	if (!it) {
		tet_infoline("elm_stackedicon_item_prepend() failed in positive test case");
		tet_result(TET_FAIL);
		return;
	}
	tet_result(TET_PASS);
	tet_infoline("[[ TET_MSG ]]::[ID]:TC_01, [TYPE]: Positive, [RESULT]:PASS, elm_stackedicon_item_prepend().");
}

/**
 * @brief Negative test case of ug_init elm_stackedicon_item_prepend()
 */
static void utc_UIFW_elm_stackedicon_item_prepend_func_02(void)
{
	Elm_Imageslider_Item *it = NULL;

	tet_infoline("[[ DEBUG:: Negative ]]");

	it = elm_stackedicon_item_prepend(NULL, icon_path[1]);
	if (it) {
		tet_infoline("elm_stackedicon_item_prepend() failed in negative test case");
		tet_result(TET_FAIL);
		return;
	}
	tet_result(TET_PASS);
	tet_infoline("[[ TET_MSG ]]::[ID]:TC_02, [TYPE]: Negative, [RESULT]:PASS, elm_stackedicon_item_prepend().");
}
