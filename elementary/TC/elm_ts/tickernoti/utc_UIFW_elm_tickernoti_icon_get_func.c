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

static void utc_UIFW_elm_tickernoti_icon_get_func_01(void);
static void utc_UIFW_elm_tickernoti_icon_get_func_02(void);

enum {
   POSITIVE_TC_IDX = 0x01,
   NEGATIVE_TC_IDX,
};

struct tet_testlist tet_testlist[] = {
   { utc_UIFW_elm_tickernoti_icon_get_func_01, POSITIVE_TC_IDX },
   { utc_UIFW_elm_tickernoti_icon_get_func_02, NEGATIVE_TC_IDX },
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
 * @brief Positive test case of elm_tickernoti_icon_get()
 */
static void utc_UIFW_elm_tickernoti_icon_get_func_01(void)
{
   Evas_Object *icon = NULL;
   Evas_Object* ic = NULL;
   char buf[PATH_MAX] = {0,};

   ic = elm_icon_add(tickernoti);
   snprintf(buf, sizeof(buf), "%s/images/logo_small.png", PKG_DATA_DIR);
   elm_icon_file_set(ic, buf, NULL);
   elm_icon_scale_set (ic, 1, 1);

   elm_tickernoti_icon_set(tickernoti, ic);
   icon = elm_tickernoti_icon_get(tickernoti);

   if (ic != icon) {
      tet_infoline("elm_tickernoti_icon_get() failed in positive test case");
      tet_result(TET_FAIL);
      return;
   }
   tet_result(TET_PASS);
}

/**
 * @brief Negative test case of ug_init elm_tickernoti_icon_get()
 */
static void utc_UIFW_elm_tickernoti_icon_get_func_02(void)
{
   Evas_Object *icon = NULL;
   Evas_Object* ic = NULL;
   char buf[PATH_MAX] = {0,};

   ic = elm_icon_add(tickernoti);
   snprintf(buf, sizeof(buf), "%s/images/logo_small.png", PKG_DATA_DIR);
   elm_icon_file_set(ic, buf, NULL);
   elm_icon_scale_set (ic, 1, 1);

   elm_tickernoti_icon_set(tickernoti, ic);
   icon = elm_tickernoti_icon_get(NULL);

   if (icon) {
      tet_infoline("elm_tickernoti_icon_get() failed in negative test case");
      tet_result(TET_FAIL);
      return;
   }
   tet_result(TET_PASS);
}
