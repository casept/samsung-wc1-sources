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
Evas_Object *colorselector;

static void startup(void);
static void cleanup(void);

void (*tet_startup)(void) = startup;
void (*tet_cleanup)(void) = cleanup;

static void utc_UIFW_elm_colorselector_color_set_func_01(void);
static void utc_UIFW_elm_colorselector_color_set_func_02(void);

enum {
	POSITIVE_TC_IDX = 0x01,
	NEGATIVE_TC_IDX,
};

struct tet_testlist tet_testlist[] = {
	{ utc_UIFW_elm_colorselector_color_set_func_01, POSITIVE_TC_IDX },
	{ utc_UIFW_elm_colorselector_color_set_func_02, NEGATIVE_TC_IDX },
	{ NULL, 0 }
};

static void startup(void)
{
	tet_infoline("[[ TET_MSG ]]:: ============ Startup ============ ");
	elm_init(0, NULL);
	main_win = elm_win_add(NULL, "main", ELM_WIN_BASIC);
	evas_object_show(main_win);
	colorselector = elm_colorselector_add(main_win);
	evas_object_show(colorselector);
}

static void cleanup(void)
{
	if ( NULL != main_win ) {
		evas_object_del(main_win);
	       	main_win = NULL;
	}
	if ( NULL != colorselector ) {
		evas_object_del(colorselector);
	       	colorselector = NULL;
	}
	elm_shutdown();
	tet_infoline("[[ TET_MSG ]]:: ============ Cleanup ============ ");
}

/**
 * @brief Positive test case of elm_colorselector_color_set()
 */
static void utc_UIFW_elm_colorselector_color_set_func_01(void)
{
   int r, g, b, a;
   r = g = b = a = -1;
   elm_colorselector_color_set(colorselector, 255, 255, 255, 255);
   elm_colorselector_color_get(colorselector, &r, &g, &b, &a);
   if( (r == 255) && (g == 255) && (b == 255) && (a == 255) )
     tet_result(TET_PASS);
   else
     tet_result(TET_FAIL);
}

/**
 * @brief Negative test case of ug_init elm_colorselector_color_set()
 */
static void utc_UIFW_elm_colorselector_color_set_func_02(void)
{
   elm_colorselector_color_set(NULL, -1, -1, -1, -1);
   tet_result(TET_PASS);
}
