CC ?= gcc

TARGETS = utc_UIFW_elm_progressbar_add_func \
      utc_UIFW_elm_progressbar_pulse_get_func \
      utc_UIFW_elm_progressbar_pulse_func \
      utc_UIFW_elm_progressbar_value_get_func \
      utc_UIFW_elm_progressbar_label_get_func \
      utc_UIFW_elm_progressbar_icon_get_func \
      utc_UIFW_elm_progressbar_span_size_get_func \
      utc_UIFW_elm_progressbar_unit_format_get_func \
      utc_UIFW_elm_progressbar_horizontal_get_func \
      utc_UIFW_elm_progressbar_inverted_get_func 

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
