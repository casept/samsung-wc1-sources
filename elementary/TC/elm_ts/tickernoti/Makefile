CC ?= gcc

TARGETS = utc_UIFW_elm_tickernoti_add_func \
   utc_UIFW_elm_tickernoti_icon_set_func \
   utc_UIFW_elm_tickernoti_icon_get_func \
   utc_UIFW_elm_tickernoti_label_set_func \
   utc_UIFW_elm_tickernoti_label_get_func \
   utc_UIFW_elm_tickernoti_detailview_label_set_func \
   utc_UIFW_elm_tickernoti_detailview_label_get_func \
   utc_UIFW_elm_tickernoti_detailview_button_set_func \
   utc_UIFW_elm_tickernoti_detailview_button_get_func \
   utc_UIFW_elm_tickernoti_detailview_icon_set_func \
   utc_UIFW_elm_tickernoti_detailview_icon_get_func \
   utc_UIFW_elm_tickernoti_detailview_get_func \
   utc_UIFW_elm_tickernoti_rotation_set_func \
   utc_UIFW_elm_tickernoti_rotation_get_func \
   utc_UIFW_elm_tickernoti_mode_set_func \
   utc_UIFW_elm_tickernoti_mode_get_func 

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
