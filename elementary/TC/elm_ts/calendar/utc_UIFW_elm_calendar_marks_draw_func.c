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

Evas_Object *main_win = NULL;

static void startup(void);
static void cleanup(void);

void (*tet_startup)(void) = startup;
void (*tet_cleanup)(void) = cleanup;

static void utc_UIFW_elm_calendar_marks_draw_func_01(void);
static void utc_UIFW_elm_calendar_marks_draw_func_02(void);

enum {
	POSITIVE_TC_IDX = 0x01,
	NEGATIVE_TC_IDX,
};

struct tet_testlist tet_testlist[] = {
	{ utc_UIFW_elm_calendar_marks_draw_func_01, POSITIVE_TC_IDX },
	{ utc_UIFW_elm_calendar_marks_draw_func_02, NEGATIVE_TC_IDX },
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
   if (NULL != main_win)
     {
        evas_object_del(main_win);
        main_win = NULL;
     }

   elm_shutdown();
   tet_infoline("[[ TET_MSG ]]:: ============ Cleanup ============ ");
}

/**
 * @brief Positive test case of elm_calendar_marks_draw()
 */
static void utc_UIFW_elm_calendar_marks_draw_func_01(void)
{
   Evas_Object *test_eo = NULL;
   Elm_Calendar_Mark *mark = NULL;
   struct tm selected_time;
   time_t current_time;
   test_eo = elm_calendar_add(main_win);
   current_time = time(NULL) + 1 * 84600;
   localtime_r(&current_time, &selected_time);
   mark = elm_calendar_mark_add(test_eo, "checked", &selected_time, ELM_CALENDAR_UNIQUE);
   elm_calendar_marks_draw(test_eo);
   TET_CHECK_PASS(NULL, test_eo);

   tet_result(TET_PASS);
   tet_infoline("elm_calendar_marks_draw() passed in positive test case");
   evas_object_del(test_eo);
   test_eo = NULL;
}

/**
 * @brief Negative test case of ug_init elm_calendar_marks_draw()
 */
static void utc_UIFW_elm_calendar_marks_draw_func_02(void)
{
   Evas_Object *test_eo = NULL;
   Elm_Calendar_Mark *mark = NULL;
   struct tm selected_time;
   time_t current_time;
   test_eo = elm_calendar_add(main_win);
   current_time = time(NULL) + 1 * 84600;
   localtime_r(&current_time, &selected_time);
   mark = elm_calendar_mark_add(test_eo, "checked", &selected_time, ELM_CALENDAR_UNIQUE);
   elm_calendar_marks_draw(NULL);
   tet_infoline("elm_calendar_marks_draw() passed in negative test case");
   evas_object_del(test_eo);
   test_eo = NULL;
   tet_result(TET_PASS);
}
