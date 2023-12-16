#include <tet_api.h>

static void startup(void);
static void cleanup(void);

void (*tet_startup)(void) = startup;
void (*tet_cleanup)(void) = cleanup;

static void utc_UIFW_elm_imageslider_append_func_01(void);
static void utc_UIFW_elm_imageslider_append_func_02(void);

enum {
	POSITIVE_TC_IDX = 0x01,
	NEGATIVE_TC_IDX,
};

struct tet_testlist tet_testlist[] = {
	{ utc_UIFW_elm_imageslider_append_func_01, POSITIVE_TC_IDX },
	{ utc_UIFW_elm_imageslider_append_func_02, NEGATIVE_TC_IDX },
};

static void startup(void)
{
}

static void cleanup(void)
{
}

/**
 * @brief Positive test case of elm_imageslider_append()
 */
static void utc_UIFW_elm_imageslider_append_func_01(void)
{
	int r = 0;

/*
   	r = elm_imageslider_append(...);
*/
	if (r) {
		tet_infoline("elm_imageslider_append() failed in positive test case");
		tet_result(TET_FAIL);
		return;
	}
	tet_result(TET_PASS);
}

/**
 * @brief Negative test case of ug_init elm_imageslider_append()
 */
static void utc_UIFW_elm_imageslider_append_func_02(void)
{
	int r = 0;

/*
   	r = elm_imageslider_append(...);
*/
	if (r) {
		tet_infoline("elm_imageslider_append() failed in negative test case");
		tet_result(TET_FAIL);
		return;
	}
	tet_result(TET_PASS);
}
