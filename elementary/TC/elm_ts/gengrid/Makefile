CC ?= gcc

TARGETS = utc_UIFW_elm_gengrid_add_func \
	  utc_UIFW_elm_gengrid_item_size_get_func \
	  utc_UIFW_elm_gengrid_align_get_func \
	  utc_UIFW_elm_gengrid_item_append_func \
	  utc_UIFW_elm_gengrid_item_prepend_func \
	  utc_UIFW_elm_gengrid_item_object_get_func \
	  utc_UIFW_elm_gengrid_item_pos_get_func \
	  utc_UIFW_elm_gengrid_multi_select_get_func \
	  utc_UIFW_elm_gengrid_selected_item_get_func \
	  utc_UIFW_elm_gengrid_selected_items_get_func \
	  utc_UIFW_elm_gengrid_item_selected_get_func \
	  utc_UIFW_elm_gengrid_always_select_mode_get_func \
	  utc_UIFW_elm_gengrid_no_select_mode_get_func \
	  utc_UIFW_elm_gengrid_bounce_get_func

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
