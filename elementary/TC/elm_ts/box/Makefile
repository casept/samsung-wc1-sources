CC ?= gcc

TARGETS = utc_UIFW_elm_box_add_func \
	  utc_UIFW_elm_box_horizontal_set_func \
	  utc_UIFW_elm_box_homogenous_set_func \
	  utc_UIFW_elm_box_pack_start_func \
	  utc_UIFW_elm_box_pack_end_func \
	  utc_UIFW_elm_box_pack_before_func \
	  utc_UIFW_elm_box_pack_after_func \
	  utc_UIFW_elm_box_clear_func \
	  utc_UIFW_elm_box_unpack_func \
	  utc_UIFW_elm_box_unpack_all_func

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
