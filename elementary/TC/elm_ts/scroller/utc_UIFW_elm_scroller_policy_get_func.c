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

static void utc_UIFW_elm_scroller_policy_get_func_01(void);
static void utc_UIFW_elm_scroller_policy_get_func_02(void);

enum {
	POSITIVE_TC_IDX = 0x01,
	NEGATIVE_TC_IDX,
};

struct tet_testlist tet_testlist[] = {
	{ utc_UIFW_elm_scroller_policy_get_func_01, POSITIVE_TC_IDX },
	{ utc_UIFW_elm_scroller_policy_get_func_02, NEGATIVE_TC_IDX },
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
 * @brief Positive test case of elm_scroller_policy_get()
 */
static void utc_UIFW_elm_scroller_policy_get_func_01(void)
{
	Elm_Scroller_Policy policy_h;
	Elm_Scroller_Policy policy_v;

	Evas_Object *test_scroller = NULL;

	test_scroller = elm_scroller_add(main_win);

	// Current return type of this API is "Void"
	elm_scroller_policy_get(test_scroller, &policy_h, &policy_v);

	switch(policy_h)
	{
		case ELM_SCROLLER_POLICY_AUTO:
		case ELM_SCROLLER_POLICY_ON:
		case ELM_SCROLLER_POLICY_OFF:
		case ELM_SCROLLER_POLICY_LAST:
			{
				switch(policy_v)
				{
					case ELM_SCROLLER_POLICY_AUTO:
					case ELM_SCROLLER_POLICY_ON:
					case ELM_SCROLLER_POLICY_OFF:
					case ELM_SCROLLER_POLICY_LAST:
						tet_result(TET_PASS);
						tet_infoline("[[ TET_MSG ]]::[ID]: TC_01, [TYPE]: Positive, [RESULT]: PASS, Get the scroller scrollbar policy.");
						break;
					default:
						tet_result(TET_FAIL);
						tet_infoline("[[ TET_MSG ]]::[ID]: TC_01, [TYPE]: Positive, [RESULT]: FAIL, Getting the scroller scrollbar policy has failed.");

				}
			}
			break;
		default:
			tet_result(TET_FAIL);
			tet_infoline("[[ TET_MSG ]]::[ID]: TC_01, [TYPE]: Positive, [RESULT]: FAIL, Getting the scroller scrollbar policy has failed.");

	}

}

/**
 * @brief Negative test case of ug_init elm_scroller_policy_get()
 */
static void utc_UIFW_elm_scroller_policy_get_func_02(void)
{
	Elm_Scroller_Policy *policy_h;
	Elm_Scroller_Policy *policy_v;

	// Current return type of this API is "Void"
	elm_scroller_policy_get(NULL, policy_h, policy_v);

	tet_result(TET_PASS);
	tet_infoline("[[ TET_MSG ]]::[ID]: TC_02, [TYPE]: Negative, [RESULT]: PASS, Getting the scroller scrollbar policy has failed.");

}
