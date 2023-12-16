CC ?= gcc

TARGETS = utc_UIFW_elm_segment_control_add_func \
      utc_UIFW_elm_segment_control_item_add_func \
      utc_UIFW_elm_segment_control_item_count_get_func \
      utc_UIFW_elm_segment_control_item_del_at_func \
      utc_UIFW_elm_segment_control_item_del_func \
      utc_UIFW_elm_segment_control_item_get_func \
      utc_UIFW_elm_segment_control_item_icon_get_func \
      utc_UIFW_elm_segment_control_item_icon_set_func \
      utc_UIFW_elm_segment_control_item_index_get_func \
      utc_UIFW_elm_segment_control_item_insert_at_func \
      utc_UIFW_elm_segment_control_item_label_get_func \
      utc_UIFW_elm_segment_control_item_label_set_func \
      utc_UIFW_elm_segment_control_item_object_get_func \
      utc_UIFW_elm_segment_control_item_selected_get_func \
      utc_UIFW_elm_segment_control_item_selected_set_func 

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
