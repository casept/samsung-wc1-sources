AUTOMAKE_OPTIONS = 1.4 foreign

MAINTAINERCLEANFILES = Makefile.in

AM_CPPFLAGS = \
-I$(top_srcdir) \
-I$(top_srcdir)/src/lib \
-I$(top_builddir)/src/lib \
-I$(top_srcdir)/src/bin \
-DPACKAGE_BIN_DIR=\"$(bindir)\" \
-DPACKAGE_DATA_DIR=\"$(datadir)/$(PACKAGE)\" \
@ELEMENTARY_CFLAGS@ \
@ELEMENTARY_X_CFLAGS@ \
@ELEMENTARY_FB_CFLAGS@ \
@ELEMENTARY_SDL_CFLAGS@ \
@ELEMENTARY_WIN32_CFLAGS@ \
@ELEMENTARY_WINCE_CFLAGS@ \
@ELEMENTARY_EDBUS_CFLAGS@ \
@ELEMENTARY_EFREET_CFLAGS@ \
@ELEMENTARY_EWEATHER_CFLAGS@ \
@ELEMENTARY_ETHUMB_CFLAGS@ \
@ELEMENTARY_WEB_CFLAGS@ \
@ELEMENTARY_EMAP_CFLAGS@ \
@ELEMENTARY_ECORE_CON_CFLAGS@ \
@ELEMENTARY_ECORE_IMF_CFLAGS@ \
@ELEMENTARY_WAYLAND_CFLAGS@ \
@EVIL_CFLAGS@ \
@EIO_CFLAGS@ \
@EMOTION_CFLAGS@

bin_PROGRAMS = @ELEMENTARY_TEST_PRG@ @ELEMENTARY_CONFIG_PRG@  @ELEMENTARY_CODEGEN_PRG@
if BUILD_QUICKLAUNCH
bin_PROGRAMS += elementary_quicklaunch elementary_run elementary_testql
endif

EXTRA_PROGRAMS = elementary_test elementary_config elementary_codegen

elementary_test_SOURCES = \
test.c \
test_3d.c \
test_actionslider.c \
test_anim.c \
test_bg.c \
test_box.c \
test_bubble.c \
test_button.c \
test_calendar.c \
test_check.c \
test_clock.c \
test_cnp.c \
test_colorselector.c \
test_conform.c \
test_ctxpopup.c \
test_cursor.c \
test_datetime.c \
test_dayselector.c \
test_diskselector.c \
test_dnd.c \
test_entry.c \
test_entry_anchor.c \
test_entry_anchor2.c \
test_external.c \
test_fileselector_button.c \
test_fileselector.c \
test_fileselector_entry.c \
test_flip.c \
test_flip_page.c \
test_flipselector.c \
test_floating.c \
test_focus.c \
test_focus2.c \
test_focus3.c \
test_gengrid.c \
test_genlist.c \
test_gesture_layer.c \
test_gesture_layer2.c \
test_gesture_layer3.c \
test_glview_simple.c \
test_glview.c \
test_grid.c \
test_hover.c \
test_hoversel.c \
test_icon.c \
test_icon_desktops.c \
test_icon_animated.c \
test_image.c \
test_index.c \
test_inwin.c \
test_label.c \
test_launcher.c \
test_layout.c \
test_list.c \
test_map.c \
test_menu.c \
test_multi.c \
test_multibuttonentry.c \
test_naviframe.c \
test_naviframe_complex.c \
test_notify.c \
test_panel.c \
test_panes.c \
test_photo.c \
test_photocam.c \
test_popup.c \
test_progressbar.c \
test_radio.c \
test_scaling.c \
test_scroller.c \
test_segment_control.c \
test_separator.c \
test_slider.c \
test_slideshow.c \
test_spinner.c \
test_store.c \
test_table.c \
test_thumb.c \
test_toolbar.c \
test_tooltip.c \
test_transit.c \
test_video.c \
test_weather.c \
test_web.c \
test_win_inline.c \
test_win_socket.c \
test_win_plug.c \
test_win_state.c \
test_win_wm_rotation.c \
test_win_aux_hint.c \
test.h

if HAVE_EIO
elementary_test_SOURCES += test_eio.c
endif

elementary_test_LDADD = $(top_builddir)/src/lib/libelementary.la \
	@ELEMENTARY_EWEATHER_LIBS@ \
	@ELEMENTARY_EDBUS_LIBS@ \
	@ELEMENTARY_EFREET_LIBS@ \
	@ELEMENTARY_EMAP_LIBS@ \
	@ELEMENTARY_LIBS@ \
	@EIO_LIBS@ \
	@ELEMENTARY_WEB_LIBS@ \
	@my_libs@
elementary_test_LDFLAGS =

elementary_config_SOURCES = \
config.c

elementary_config_LDADD = $(top_builddir)/src/lib/libelementary.la \
	@ELEMENTARY_EWEATHER_LIBS@ \
	@ELEMENTARY_EDBUS_LIBS@ \
	@ELEMENTARY_EFREET_LIBS@ \
	@ELEMENTARY_EMAP_LIBS@ \
	@ELEMENTARY_LIBS@ \
	@EIO_LIBS@ \
	@ELEMENTARY_WEB_LIBS@ \
	@my_libs@
elementary_config_LDFLAGS =

elementary_codegen_SOURCES = \
elementary_codegen.c

elementary_codegen_LDADD = \
	$(top_builddir)/src/lib/libelementary.la \
	@ELEMENTARY_EWEATHER_LIBS@ \
	@ELEMENTARY_EDBUS_LIBS@ \
	@ELEMENTARY_EFREET_LIBS@ \
	@ELEMENTARY_EMAP_LIBS@ \
	@ELEMENTARY_LIBS@ \
	@EIO_LIBS@ \
	@ELEMENTARY_WEB_LIBS@ \
	@my_libs@
elementary_codegen_LDFLAGS =

if BUILD_QUICKLAUNCH
elementary_quicklaunch_SOURCES = quicklaunch.c
elementary_quicklaunch_LDADD = $(top_builddir)/src/lib/libelementary.la \
	@ELEMENTARY_EWEATHER_LIBS@ \
	@ELEMENTARY_EDBUS_LIBS@ \
	@ELEMENTARY_EFREET_LIBS@ \
	@ELEMENTARY_EMAP_LIBS@ \
	@ELEMENTARY_LIBS@ \
	@EIO_LIBS@ \
	@ELEMENTARY_WEB_LIBS@ \
	@my_libs@
elementary_quicklaunch_LDFLAGS =

if BUILD_RUN
elementary_run_SOURCES = run.c
elementary_run_LDADD =
elementary_run_LDFLAGS =
endif

## This is how to build a quicklanch capable app
# build the shared lib version - libtool produces a .a and .la file as well
# as a .so - these get put in libdir ($PREFIX/lib) as elementary_testql.so
# etc. - this is where elementary will expect to find the .so's for
# quicklaunch apps.
elementary_testqldir = $(libdir)
elementary_testql_LTLIBRARIES = elementary_testql.la
elementary_testql_la_SOURCES = $(elementary_test_SOURCES)
elementary_testql_la_LIBADD = $(top_builddir)/src/lib/libelementary.la \
	@ELEMENTARY_EWEATHER_LIBS@ \
	@ELEMENTARY_EDBUS_LIBS@ \
	@ELEMENTARY_EFREET_LIBS@ \
	@ELEMENTARY_EMAP_LIBS@ \
	@ELEMENTARY_LIBS@ \
	@EIO_LIBS@ \
	@ELEMENTARY_WEB_LIBS@ \
	@my_libs@

elementary_testql_la_CFLAGS =
elementary_testql_la_LDFLAGS = -module -avoid-version -no-undefined
elementary_testql_SOURCES = $(elementary_test_SOURCES)
elementary_testql_LDADD = $(top_builddir)/src/lib/libelementary.la \
	@ELEMENTARY_EWEATHER_LIBS@ \
	@ELEMENTARY_EDBUS_LIBS@ \
	@ELEMENTARY_EFREET_LIBS@ \
	@ELEMENTARY_EMAP_LIBS@ \
	@ELEMENTARY_LIBS@ \
	@EIO_LIBS@ \
	@ELEMENTARY_WEB_LIBS@ \
	@my_libs@

elementary_testql_CFLAGS = -DELM_LIB_QUICKLAUNCH=1
elementary_testql_LDFLAGS =
endif

EXTRA_DIST = \
test_factory.c
