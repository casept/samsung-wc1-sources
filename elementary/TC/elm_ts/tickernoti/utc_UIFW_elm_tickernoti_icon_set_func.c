#include <tet_api.h>
#include <Elementary.h>
#define PKG_DATA_DIR "$PREFIX/share/elementary"

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
Evas_Object *tickernoti;

static void startup(void);
static void cleanup(void);

void (*tet_startup)(void) = startup;
void (*tet_cleanup)(void) = cleanup;

static void utc_UIFW_elm_tickernoti_icon_set_func_01(void);
static void utc_UIFW_elm_tickernoti_icon_set_func_02(void);

enum {
   POSITIVE_TC_IDX = 0x01,
   NEGATIVE_TC_IDX,
};

struct tet_testlist tet_testlist[] = {
   { utc_UIFW_elm_tickernoti_icon_set_func_01, POSITIVE_TC_IDX },
   { utc_UIFW_elm_tickernoti_icon_set_func_02, NEGATIVE_TC_IDX },
   { NULL, 0 }
};

static void startup(void)
{
   tet_infoline("[[ TET_MSG ]]:: ============ Startup ============ ");
   elm_init(0, NULL);
   main_win = elm_win_add(NULL, "main", ELM_WIN_BASIC);
   evas_object_show(main_win);

   tickernoti = elm_tickernoti_add(NULL);
}

static void cleanup(void)
{
   if ( NULL != main_win ) {
      evas_object_del(main_win);
      main_win = NULL;
   }
   elm_shutdown();
   tet_infoline("[[ TET_MSG ]]:: ============ Cleanup ============ ");

   evas_object_del(tickernoti);
}

/**
 * @brief Positive test case of elm_tickernoti_icon_set()
 */
static void utc_UIFW_elm_tickernoti_icon_set_func_01(void)
{
   char buf[PATH_MAX] = {0,};
   Evas_Object *test_icon = NULL;

   /* icon for tickernoti */
   Evas_Object *icon1;
   icon1 = elm_icon_add (tickernoti);
   snprintf (buf, sizeof(buf), "%s/00_noti_msg.png", PKG_DATA_DIR);
   elm_icon_file_set (icon1, buf, NULL);
   elm_icon_scale_set (icon1, 1, 1);

   elm_tickernoti_icon_set (tickernoti, icon1);
   test_icon = elm_tickernoti_icon_get(tickernoti);

   if (test_icon != icon1) {
      tet_infoline("elm_tickernoti_icon_set() failed in positive test case");
      tet_result(TET_FAIL);
      return;
   }
   tet_result(TET_PASS);
}

/**
 * @brief Negative test case of ug_init elm_tickernoti_icon_set()
 */
static void utc_UIFW_elm_tickernoti_icon_set_func_02(void)
{
   char buf[PATH_MAX] = {0,};
   Evas_Object *test_icon = NULL;

   /* icon for tickernoti */
   Evas_Object *icon1;
   icon1 = elm_icon_add (tickernoti);
   snprintf (buf, sizeof(buf), "%s/00_noti_msg.png", PKG_DATA_DIR);
   elm_icon_file_set (icon1, buf, NULL);
   elm_icon_scale_set (icon1, 1, 1);

   elm_tickernoti_icon_set (NULL, icon1);
   test_icon = elm_tickernoti_icon_get(tickernoti);

   tet_result(TET_PASS);
}
