#include <tet_api.h>
#include <Elementary.h>
#define PKG_DATA_DIR "/usr/share/beat_winset_test/icon/"

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

static void utc_UIFW_elm_nocontents_custom_get_func_01(void);
static void utc_UIFW_elm_nocontents_custom_get_func_02(void);

enum {
	POSITIVE_TC_no_contents = 0x01,
	NEGATIVE_TC_no_contents,
};

struct tet_testlist tet_testlist[] = {
	{ utc_UIFW_elm_nocontents_custom_get_func_01, POSITIVE_TC_no_contents },
	{ utc_UIFW_elm_nocontents_custom_get_func_02, NEGATIVE_TC_no_contents },
        { NULL, 0}
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
 * @brief Positive test case of elm_nocontents_custom_get()
 */
static void utc_UIFW_elm_nocontents_custom_get_func_01(void)
{
        Evas_Object *no_contents = NULL;
        Evas_Object  *custom_area,*btn , *icon;
        Evas_Object *custom;
	char buf[255] = {0,};
   	no_contents = elm_nocontents_add(main_win);
	evas_object_show(no_contents);
 	custom_area = elm_layout_add (main_win);
	elm_layout_file_set (custom_area, NULL, "winset-test/nocontents/search_google");
        elm_nocontents_custom_set(no_contents,custom_area);
	btn = elm_button_add (main_win);
	icon = elm_icon_add (main_win);
	snprintf (buf, sizeof(buf), "%s/30_SmartSearch_google_icon.png", PKG_DATA_DIR);
	elm_icon_file_set (icon, buf, NULL);
	elm_icon_scale_set (icon, 1, 1);
	elm_button_icon_set (btn, icon);
	elm_layout_content_set (custom_area, "buttons", btn);
	custom = elm_nocontents_custom_get(no_contents);
	if (!custom) {
		tet_infoline("elm_nocontents_custom_get() failed in positive test case");
		tet_result(TET_FAIL);
		return;
	}
	tet_result(TET_PASS);
}

/**
 * @brief Negative test case of elm_nocontents_custom_get()
 */
static void utc_UIFW_elm_nocontents_custom_get_func_02(void)
{
        Evas_Object *no_contents = NULL;
        Evas_Object  *custom_area,*btn , *icon;
        Evas_Object *custom;
	char buf[255] = {0,};
   	no_contents = elm_nocontents_add(main_win);
	evas_object_show(no_contents);
 	custom_area = elm_layout_add (main_win);
	elm_layout_file_set (custom_area, NULL, "winset-test/nocontents/search_google");
        elm_nocontents_custom_set(no_contents,custom_area);
	btn = elm_button_add (main_win);
	icon = elm_icon_add (main_win);
	snprintf (buf, sizeof(buf), "%s/30_SmartSearch_google_icon.png", PKG_DATA_DIR);
	elm_icon_file_set (icon, buf, NULL);
	elm_icon_scale_set (icon, 1, 1);
	elm_button_icon_set (btn, icon);
	elm_layout_content_set (custom_area, "buttons", btn);
	custom = elm_nocontents_custom_get(NULL);
	if (custom) {
		tet_infoline("elm_nocontents_custom_get() failed in negative test case");
		tet_result(TET_FAIL);
		return;
	}
	tet_result(TET_PASS);
}
