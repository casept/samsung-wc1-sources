CC ?= gcc

TARGETS = utc_UIFW_elm_index_add_func \
		utc_UIFW_elm_index_active_set_func \
		utc_UIFW_elm_index_item_append_func \
		utc_UIFW_elm_index_item_append_relative_func \
		utc_UIFW_elm_index_item_find_func \
		utc_UIFW_elm_index_item_go_func \
		utc_UIFW_elm_index_item_letter_get_func \
		utc_UIFW_elm_index_item_level_set_func \
		utc_UIFW_elm_index_item_prepend_func \
		utc_UIFW_elm_index_item_prepend_relative_func \
		utc_UIFW_elm_index_item_selected_get_func \
		utc_UIFW_elm_index_item_sorted_insert_func \
		utc_UIFW_elm_index_item_clear_func	\
		utc_UIFW_elm_index_item_level_get_func

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
