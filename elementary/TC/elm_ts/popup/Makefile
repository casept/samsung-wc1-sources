CC ?= gcc

TARGETS = utc_UIFW_elm_popup_add_func \
	  utc_UIFW_elm_popup_with_buttons_add_func \
	  utc_UIFW_elm_popup_desc_set_func \
	  utc_UIFW_elm_popup_desc_get_func \
	  utc_UIFW_elm_popup_title_label_set_func \
	  utc_UIFW_elm_popup_title_label_get_func \
	  utc_UIFW_elm_popup_title_icon_set_func \
	  utc_UIFW_elm_popup_title_icon_get_func \
	  utc_UIFW_elm_popup_content_set_func \
	  utc_UIFW_elm_popup_content_get_func \
	  utc_UIFW_elm_popup_buttons_add_func \
	  utc_UIFW_elm_popup_timeout_set_func \
	  utc_UIFW_elm_popup_mode_set_func \
	  utc_UIFW_elm_popup_response_func \
	  utc_UIFW_elm_popup_orient_set_func \
	  utc_UIFW_elm_popup_run_func

PKGS = elementary evas ecore

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
