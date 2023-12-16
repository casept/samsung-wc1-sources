CC ?= gcc

TARGETS = utc_UIFW_elm_genlist_add_func \
	  utc_UIFW_elm_genlist_clear_func \
	  utc_UIFW_elm_genlist_multi_select_set_func \
	  utc_UIFW_elm_genlist_multi_select_get_func \
	  utc_UIFW_elm_genlist_horizontal_mode_set_func \
	  utc_UIFW_elm_genlist_horizontal_mode_get_func \
	  utc_UIFW_elm_genlist_always_select_mode_set_func \
	  utc_UIFW_elm_genlist_always_select_mode_get_func \
	  utc_UIFW_elm_genlist_no_select_mode_set_func \
	  utc_UIFW_elm_genlist_no_select_mode_get_func \
	  utc_UIFW_elm_genlist_compress_mode_set_func \
	  utc_UIFW_elm_genlist_compress_mode_get_func \
	  utc_UIFW_elm_genlist_bounce_set_func \
	  utc_UIFW_elm_genlist_bounce_get_func \
	  utc_UIFW_elm_genlist_homogeneous_set_func \
	  utc_UIFW_elm_genlist_homogeneous_get_func \
	  utc_UIFW_elm_genlist_block_count_set_func \
	  utc_UIFW_elm_genlist_block_count_get_func \
	  utc_UIFW_elm_genlist_longpress_timeout_set_func \
	  utc_UIFW_elm_genlist_longpress_timeout_get_func \
	  utc_UIFW_elm_genlist_effect_set_func \
	  utc_UIFW_elm_genlist_pinch_zoom_set_func \
	  utc_UIFW_elm_genlist_pinch_zoom_mode_set_func \
	  utc_UIFW_elm_genlist_pinch_zoom_mode_get_func  \
	  utc_UIFW_elm_genlist_item_append_func \
	  utc_UIFW_elm_genlist_item_prepend_func  \
	  utc_UIFW_elm_genlist_item_insert_before_func \
	  utc_UIFW_elm_genlist_item_insert_after_func \
	  utc_UIFW_elm_genlist_selected_item_get_func \
	  utc_UIFW_elm_genlist_selected_items_get_func \
	  utc_UIFW_elm_genlist_realized_items_get_func \
	  utc_UIFW_elm_genlist_at_xy_item_get_func \
	  utc_UIFW_elm_genlist_first_item_get_func  \
	  utc_UIFW_elm_genlist_last_item_get_func \
	  utc_UIFW_elm_genlist_item_next_get_func \
	  utc_UIFW_elm_genlist_item_prev_get_func \
	  utc_UIFW_elm_genlist_item_genlist_get_func \
	  utc_UIFW_elm_genlist_item_parent_get_func \
	  utc_UIFW_elm_genlist_item_subitems_clear_func \
	  utc_UIFW_elm_genlist_item_selected_set_func \
	  utc_UIFW_elm_genlist_item_selected_get_func \
	  utc_UIFW_elm_genlist_item_expanded_set_func \
	  utc_UIFW_elm_genlist_item_expanded_get_func \
	  utc_UIFW_elm_genlist_item_disabled_set_func \
	  utc_UIFW_elm_genlist_item_disabled_get_func \
	  utc_UIFW_elm_genlist_item_display_only_set_func \
	  utc_UIFW_elm_genlist_item_display_only_get_func \
	  utc_UIFW_elm_genlist_item_show_func \
	  utc_UIFW_elm_genlist_item_bring_in_func \
	  utc_UIFW_elm_genlist_item_top_show_func \
	  utc_UIFW_elm_genlist_item_top_bring_in_func \
	  utc_UIFW_elm_genlist_item_middle_show_func \
	  utc_UIFW_elm_genlist_item_middle_bring_in_func \
	  utc_UIFW_elm_genlist_item_del_func \
	  utc_UIFW_elm_genlist_item_data_set_func \
	  utc_UIFW_elm_genlist_item_data_get_func \
	  utc_UIFW_elm_genlist_item_update_func \
	  utc_UIFW_elm_genlist_scroller_policy_set_func \
	  utc_UIFW_elm_genlist_edit_mode_set_func \
	  utc_UIFW_elm_genlist_item_expanded_depth_get_func \
	  utc_UIFW_elm_genlist_item_rename_mode_set_func \
	  utc_UIFW_elm_genlist_item_move_after_func

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
