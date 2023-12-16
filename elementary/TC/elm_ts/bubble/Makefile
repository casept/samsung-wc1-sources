CC ?= gcc

TARGETS = utc_UIFW_elm_bubble_add_func \
	  utc_UIFW_elm_bubble_label_set_func \
	  utc_UIFW_elm_bubble_label_get_func \
	  utc_UIFW_elm_bubble_info_set_func \
	  utc_UIFW_elm_bubble_info_get_func \
	  utc_UIFW_elm_bubble_content_set_func \
	  utc_UIFW_elm_bubble_content_unset_func \
	  utc_UIFW_elm_bubble_icon_set_func \
	  utc_UIFW_elm_bubble_icon_get_func \
	  utc_UIFW_elm_bubble_corner_set_func

PKGS = elementary

LDFLAGS = `pkg-config --libs $(PKGS)`
LDFLAGS += $(TET_ROOT)/lib/tet3/tcm_s.o
LDFLAGS += -L$(TET_ROOT)/lib/tet3 -ltcm_s
LDFLAGS += -L$(TET_ROOT)/lib/tet3 -lapi_s

CFLAGS = -I. `pkg-config --cflags $(PKGS)`
CFLAGS += -I$(TET_ROOT)/inc/tet3
CFLAGS += -Wall

all: $(TARGETS)

$(TARGET): %: %.c
	$(CC) -o $@ $< $(CFLAGS) $(LDFLAGS)

clean:
	rm -f $(TARGETS)
	rm -f tet_captured
	rm -f *~
