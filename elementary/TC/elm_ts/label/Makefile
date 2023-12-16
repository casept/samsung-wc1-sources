CC ?= gcc

TARGETS = utc_UIFW_elm_label_add_func \
	utc_UIFW_elm_label_background_color_set_func \
	utc_UIFW_elm_label_ellipsis_set_func \
	utc_UIFW_elm_label_fontsize_set_func \
	utc_UIFW_elm_label_line_wrap_get_func \
	utc_UIFW_elm_label_slide_duration_get_func \
	utc_UIFW_elm_label_slide_get_func \
	utc_UIFW_elm_label_text_align_set_func \
	utc_UIFW_elm_label_text_color_set_func \
	utc_UIFW_elm_label_wrap_height_get_func \
	utc_UIFW_elm_label_wrap_width_get_func \

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
	rm -f *~ *.o tet_captured tet_lock $(TARGETS)
