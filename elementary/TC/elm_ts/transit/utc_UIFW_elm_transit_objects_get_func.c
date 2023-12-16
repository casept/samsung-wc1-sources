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
Elm_Transit *transit;

static void startup(void);
static void cleanup(void);

void (*tet_startup)(void) = startup;
void (*tet_cleanup)(void) = cleanup;

static void utc_UIFW_elm_transit_objects_get_func_01(void);
static void utc_UIFW_elm_transit_objects_get_func_02(void);

enum {
	POSITIVE_TC_IDX = 0x01,
	NEGATIVE_TC_IDX,
};

struct tet_testlist tet_testlist[] = {
	{ utc_UIFW_elm_transit_objects_get_func_01, POSITIVE_TC_IDX },
	{ utc_UIFW_elm_transit_objects_get_func_02, NEGATIVE_TC_IDX },
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
	if ( NULL != transit ) {
		elm_transit_del(transit);
		transit = NULL;
	}
	elm_shutdown();
	tet_infoline("[[ TET_MSG ]]:: ============ Cleanup ============ ");
}


/**
 * @brief Positive test case of elm_transit_objects_get()
 */
static void utc_UIFW_elm_transit_objects_get_func_01(void)
{
   	transit = elm_transit_add();
	Evas_Object *btn = elm_button_add(main_win);
	elm_transit_object_add(transit, btn);
	if(!elm_transit_objects_get(transit)) {
		tet_infoline("elm_transit_objects_get failed in positive test case");
		tet_result(TET_FAIL);
	}
	tet_result(TET_PASS);
}

/**
 * @brief Negative test case of elm_transit_objects_get()
 */
static void utc_UIFW_elm_transit_objects_get_func_02(void)
{
   	transit = elm_transit_add();
	if(elm_transit_objects_get(transit)) {
		tet_infoline("elm_transit_objects_get failed in negative test case");
		tet_result(TET_FAIL);
	}
	tet_result(TET_PASS);
}

