CC ?= gcc

TARGETS = utc_UIFW_elm_calendar_add_func \
	  utc_UIFW_elm_calendar_weekdays_names_get_func \
	  utc_UIFW_elm_calendar_interval_get_func \
	  utc_UIFW_elm_calendar_day_selection_enabled_get_func \
	  utc_UIFW_elm_calendar_mark_add_func \
	  utc_UIFW_elm_calendar_marks_get_func \
	  utc_UIFW_elm_calendar_min_max_year_get_func \
	  utc_UIFW_elm_calendar_selected_time_get_func \
	  utc_UIFW_elm_calendar_weekdays_names_set_func \
	  utc_UIFW_elm_calendar_interval_set_func \
	  utc_UIFW_elm_calendar_min_max_year_set_func \
	  utc_UIFW_elm_calendar_day_selection_enabled_set_func \
	  utc_UIFW_elm_calendar_selected_time_set_func \
	  utc_UIFW_elm_calendar_format_function_set_func \
	  utc_UIFW_elm_calendar_mark_del_func \
	  utc_UIFW_elm_calendar_marks_clear_func \
	  utc_UIFW_elm_calendar_marks_draw_func

PKGS = elementary evas

LDFLAGS = `pkg-config --libs $(PKGS)`
LDFLAGS += $(TET_ROOT)/lib/tet3/tcm_s.o
LDFLAGS += -L$(TET_ROOT)/lib/tet3 -ltcm_s
LDFLAGS += -L$(TET_ROOT)/lib/tet3 -lapi_s

CFLAGS = -I. `pkg-config --cflags $(PKGS)`
CFLAGS += -I$(TET_ROOT)/inc/tet3
CFLAGS += -Wall

all: $(TARGETS)

$(TARGETS): %: %.c
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

clean:
	rm -f $(TARGETS)
	rm -f tet_captured
	rm -f tet_lock
	rm -f *~
