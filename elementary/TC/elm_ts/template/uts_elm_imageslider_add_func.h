

#ifndef _UTS_ELM_IMAGESLIDER_ADD_TEST_
#define _UTS_ELM_IMAGESLIDER_ADD_TEST_

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <glib.h>
#include <tet_api.h>

// Test cases in unit test suite
void uts_elm_imageslider_add_test_001();
void uts_elm_imageslider_add_test_002();


static void startup();
static void cleanup();

// Initialize TCM data structures
void (*tet_startup)() = startup;
void (*tet_cleanup)() = cleanup;

struct tet_testlist tet_testlist[] = {
		{uts_elm_imageslider_add_test_001, 1},
		{uts_elm_imageslider_add_test_002, 2},
		{NULL, 0}
};

#endif // _UTS_ELM_IMAGESLIDER_ADD_TEST_
