CC ?= gcc

TARGETS = utc_UIFW_elm_scroller_add_func \
	  utc_UIFW_elm_scroller_content_set_func \
	  utc_UIFW_elm_scroller_content_unset_func \
	  utc_UIFW_elm_scroller_custom_widget_base_theme_set_func \
	  utc_UIFW_elm_scroller_content_min_limit_func \
	  utc_UIFW_elm_scroller_policy_get_func \
	  utc_UIFW_elm_scroller_policy_set_func \
	  utc_UIFW_elm_scroller_region_get_func \
	  utc_UIFW_elm_scroller_region_show_func \
	  utc_UIFW_elm_scroller_child_size_get_func \
	  utc_UIFW_elm_scroller_bounce_set_func \
	  utc_UIFW_elm_scroller_page_relative_set_func \
	  utc_UIFW_elm_scroller_page_size_set_func \
	  utc_UIFW_elm_scroller_region_bring_in_func \
	  utc_UIFW_elm_scroller_propagate_events_set_func



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
	rm -f *~
