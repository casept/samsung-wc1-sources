CC ?= gcc

TARGETS = utc_UIFW_elm_ctxpopup_add_func \
	  utc_UIFW_elm_ctxpopup_horizontal_set_func \
	  utc_UIFW_elm_ctxpopup_horizontal_get_func \
	  utc_UIFW_elm_ctxpopup_item_append_func \
	  utc_UIFW_elm_ctxpopup_item_icon_set_func \
	  utc_UIFW_elm_ctxpopup_item_label_set_func \
	  utc_UIFW_elm_ctxpopup_item_del_func \
	  utc_UIFW_elm_ctxpopup_item_disabled_set_func \
	  utc_UIFW_elm_ctxpopup_item_disabled_get_func \
	  utc_UIFW_elm_ctxpopup_clear_func \
	  utc_UIFW_elm_ctxpopup_item_label_get_func \
	  utc_UIFW_elm_ctxpopup_item_icon_get_func \
	  utc_UIFW_elm_ctxpopup_direction_priority_set_func \
	  utc_UIFW_elm_ctxpopup_direction_priority_get_func \
	  utc_UIFW_elm_ctxpopup_content_set_func \
	  utc_UIFW_elm_ctxpopup_content_unset_func \
	  utc_UIFW_elm_ctxpopup_hover_parent_set_func \
	  utc_UIFW_elm_ctxpopup_hover_parent_get_func

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
