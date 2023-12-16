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


Evas_Object *main_win, *popup;


static void startup(void);
static void cleanup(void);

void (*tet_startup)(void) = startup;
void (*tet_cleanup)(void) = cleanup;

static void utc_UIFW_elm_popup_run_func_01(void);
static void utc_UIFW_elm_popup_run_func_02(void);

enum {
	POSITIVE_TC_IDX = 0x01,
	NEGATIVE_TC_IDX,
};

struct tet_testlist tet_testlist[] = {
	{ utc_UIFW_elm_popup_run_func_01, POSITIVE_TC_IDX },
	{ utc_UIFW_elm_popup_run_func_02, NEGATIVE_TC_IDX },
	{ NULL, 0 }
};

static void startup(void)
{
	tet_infoline("[[ TET_MSG ]]:: ============ Startup ============ ");
	elm_init(0, NULL);
	main_win = elm_win_add(NULL, "main", ELM_WIN_BASIC);
	evas_object_show(main_win);
	popup = elm_popup_add(main_win);
	evas_object_show(popup);
}

static void cleanup(void)
{
	if ( NULL != main_win ) {
		evas_object_del(main_win);
	       	main_win = NULL;
	}
	if ( NULL != popup ) {
		evas_object_del(popup);
		popup = NULL;
	}
	elm_shutdown();
	tet_infoline("[[ TET_MSG ]]:: ============ Cleanup ============ ");
}

static Eina_Bool
_exit_timer_popup(void *data)
{
	printf("\n\nexiting timer\n");
   elm_popup_response((Evas_Object *)data, ELM_POPUP_RESPONSE_NONE);
   return EINA_FALSE;
}


/**
 * @brief Positive test case of elm_popup_run()
 */
static void utc_UIFW_elm_popup_run_func_01(void)
{
	int r = 0;

	ecore_timer_add(1.0, _exit_timer_popup, popup);
   	r = elm_popup_run(popup);

	if (!r) {
		tet_infoline("elm_popup_run() failed in positive test case");
		tet_result(TET_FAIL);
		return;
	}
	else
	{
		evas_object_del(popup);
		elm_exit();
	}
	tet_result(TET_PASS);
}

/**
 * @brief Negative test case of ug_init elm_popup_run()
 */
static void utc_UIFW_elm_popup_run_func_02(void)
{
	int r = 0;
   	r = elm_popup_run(NULL);

	if (r) {
		tet_infoline("elm_popup_run() failed in negative test case");
		tet_result(TET_FAIL);
		return;
	}
	tet_result(TET_PASS);
}