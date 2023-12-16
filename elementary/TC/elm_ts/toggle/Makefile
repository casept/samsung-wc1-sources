CC ?= gcc

TARGETS = utc_UIFW_elm_toggle_add_func \
	  utc_UIFW_elm_toggle_label_set_func \
	  utc_UIFW_elm_toggle_label_get_func \
	  utc_UIFW_elm_toggle_icon_set_func \
	  utc_UIFW_elm_toggle_icon_get_func \
	  utc_UIFW_elm_toggle_states_labels_set_func \
	  utc_UIFW_elm_toggle_states_labels_get_func \
	  utc_UIFW_elm_toggle_state_set_func \
	  utc_UIFW_elm_toggle_state_get_func \
	  utc_UIFW_elm_toggle_state_pointer_set_func
	  

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
