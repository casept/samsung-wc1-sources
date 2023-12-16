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

static void utc_UIFW_elm_panes_content_left_get_func_01(void);
static void utc_UIFW_elm_panes_content_left_get_func_02(void);

enum {
	POSITIVE_TC_IDX = 0x01,
	NEGATIVE_TC_IDX,
};

struct tet_testlist tet_testlist[] = {
	{ utc_UIFW_elm_panes_content_left_get_func_01, POSITIVE_TC_IDX },
	{ utc_UIFW_elm_panes_content_left_get_func_02, NEGATIVE_TC_IDX },
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
 * @brief Positive test case of elm_panes_content_left_get()
 */
static void utc_UIFW_elm_panes_content_left_get_func_01(void)
{
   Evas_Object *panes = NULL;
   Evas_Object *btn = NULL;

   panes = elm_panes_add(main_win);
   btn = elm_button_add(panes);
   elm_button_label_set(btn, "left");
   evas_object_size_hint_weight_set(btn, 1.0, 1.0);
   evas_object_size_hint_align_set(btn, -1.0, -1.0);
   elm_panes_content_left_set(panes, btn);
   if(elm_panes_content_left_get(panes) == NULL)
      {
         tet_infoline("elm_panes_content_left_get() failed in positive test case");
         tet_result(TET_FAIL);
         return;
      }
   evas_object_show(panes);
   evas_object_del(panes);
   panes = NULL;
   tet_result(TET_PASS);
}

/**
 * @brief Negative test case of ug_init elm_panes_content_left_get()
 */
static void utc_UIFW_elm_panes_content_left_get_func_02(void)
{
   Evas_Object *panes = NULL;
   Evas_Object *btn = NULL;

   panes = elm_panes_add(main_win);
   btn = elm_button_add(panes);
   elm_button_label_set(btn, "left");
   evas_object_size_hint_weight_set(btn, 1.0, 1.0);
   evas_object_size_hint_align_set(btn, -1.0, -1.0);
   elm_panes_content_left_set(panes, btn);
   if(elm_panes_content_left_get(NULL) != NULL)
      {
         tet_infoline("elm_panes_content_left_get() failed in negative test case");
         tet_result(TET_FAIL);
         return;
      }
   tet_result(TET_PASS);
}
