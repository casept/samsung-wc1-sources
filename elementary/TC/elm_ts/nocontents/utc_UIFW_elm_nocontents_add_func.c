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

static void utc_UIFW_elm_nocontents_add_func_01(void);
static void utc_UIFW_elm_nocontents_add_func_02(void);

enum {
	POSITIVE_TC_no_contents = 0x01,
	NEGATIVE_TC_no_contents,
};

struct tet_testlist tet_testlist[] = {
	{ utc_UIFW_elm_nocontents_add_func_01, POSITIVE_TC_no_contents },
	{ utc_UIFW_elm_nocontents_add_func_02, NEGATIVE_TC_no_contents },
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
 * @brief Positive test case of elm_nocontents_add()
 */
static void utc_UIFW_elm_nocontents_add_func_01(void)
{
	Evas_Object *no_contents = NULL;
   	no_contents = elm_nocontents_add(main_win);
	evas_object_show(no_contents);
	if (!no_contents) {
		tet_infoline("elm_nocontents_add() failed in positive test case");
		tet_result(TET_FAIL);
		return;
	}
	tet_result(TET_PASS);
}

/**
 * @brief Negative test case of elm_nocontents_add()
 */
static void utc_UIFW_elm_nocontents_add_func_02(void)
{
	Evas_Object *no_contents = NULL;
   	no_contents = elm_nocontents_add(NULL);
	evas_object_show(no_contents);
	if (no_contents) {
		tet_infoline("elm_nocontents_add() failed in negative test case");
		tet_result(TET_FAIL);
		return;
	}
	tet_result(TET_PASS);
}
