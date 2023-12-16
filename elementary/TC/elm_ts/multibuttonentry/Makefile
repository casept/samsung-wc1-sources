CC ?= gcc

TARGETS = utc_UIFW_elm_multibuttonentry_add_func \
	  utc_UIFW_elm_multibuttonentry_entry_get_func \
	  utc_UIFW_elm_multibuttonentry_item_prepend_func \
	  utc_UIFW_elm_multibuttonentry_item_append_func \
	  utc_UIFW_elm_multibuttonentry_item_insert_before_func \
	  utc_UIFW_elm_multibuttonentry_item_insert_after_func \
	  utc_UIFW_elm_multibuttonentry_items_get_func \
	  utc_UIFW_elm_multibuttonentry_first_item_get_func \
	  utc_UIFW_elm_multibuttonentry_last_item_get_func \
	  utc_UIFW_elm_multibuttonentry_selected_item_get_func \
	  utc_UIFW_elm_multibuttonentry_items_del_func \
	  utc_UIFW_elm_multibuttonentry_item_prev_get_func \
	  utc_UIFW_elm_multibuttonentry_item_next_get_func \
	  utc_UIFW_elm_multibuttonentry_item_data_get_func

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
