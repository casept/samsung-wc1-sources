CC ?= gcc

TARGETS = utc_UIFW_elm_slider_add_func \
		utc_UIFW_elm_slider_end_get_func \
		utc_UIFW_elm_slider_end_set_func \
		utc_UIFW_elm_slider_horizontal_get_func \
		utc_UIFW_elm_slider_horizontal_set_func \
		utc_UIFW_elm_slider_icon_get_func \
		utc_UIFW_elm_slider_icon_set_func \
		utc_UIFW_elm_slider_indicator_format_function_set_func \
		utc_UIFW_elm_slider_indicator_format_get_func \
		utc_UIFW_elm_slider_indicator_format_set_func \
		utc_UIFW_elm_slider_indicator_show_set_func \
        utc_UIFW_elm_slider_inverted_get_func \
        utc_UIFW_elm_slider_inverted_set_func \
		utc_UIFW_elm_slider_label_get_func \
		utc_UIFW_elm_slider_label_set_func \
        utc_UIFW_elm_slider_min_max_get_func \
        utc_UIFW_elm_slider_min_max_set_func \
		utc_UIFW_elm_slider_span_size_get_func \
		utc_UIFW_elm_slider_span_size_set_func \
        utc_UIFW_elm_slider_unit_format_get_func \
        utc_UIFW_elm_slider_unit_format_set_func \
		utc_UIFW_elm_slider_value_get_func \
        utc_UIFW_elm_slider_value_set_func 
                

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
