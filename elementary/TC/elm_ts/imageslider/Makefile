CC ?= gcc

TARGETS = utc_UIFW_elm_imageslider_add_func \
	  utc_UIFW_elm_imageslider_item_append_func \
	  utc_UIFW_elm_imageslider_item_prepend_func \
	  utc_UIFW_elm_imageslider_item_del_func \
	  utc_UIFW_elm_imageslider_selected_item_get_func \
	  utc_UIFW_elm_imageslider_item_selected_get_func \
	  utc_UIFW_elm_imageslider_item_selected_set_func \
	  utc_UIFW_elm_imageslider_photo_file_get_func \
	  utc_UIFW_elm_imageslider_item_prev_func \
	  utc_UIFW_elm_imageslider_item_next_func \
	  utc_UIFW_elm_imageslider_prev_func \
	  utc_UIFW_elm_imageslider_next_func 
	  

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
