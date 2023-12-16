MAINTAINERCLEANFILES = Makefile.in
CLEANFILES           = *.bak *.edj

INCLUDES = -I$(top_builddir) \
           -I$(top_builddir)/ism/src \
           -I$(top_srcdir) \
           -I$(top_srcdir)/ism/src \
           -I$(top_srcdir)/ism/intl \
           -I$(top_srcdir)/ism/data \
           -I$(top_srcdir)/ism/utils \
           -I$(includedir) \
           -DSCIM_DATADIR=\"@SCIM_DATADIR@\" \
           -DSCIM_LOCALEDIR=\"@SCIM_LOCALEDIR@\" \
           -DSCIM_SYSCONFDIR=\"@SCIM_SYSCONFDIR@\" \
           -DSCIM_LIBEXECDIR=\"@SCIM_LIBEXECDIR@\" \
           -DSCIM_ICONDIR=\"@SCIM_ICONDIR@\" \
           -DSCIM_MODULE_PATH=\"@SCIM_MODULE_PATH@\" \
           -DSCIM_TEMPDIR=\"@SCIM_TEMPDIR@\"

noinst_HEADERS = isf_panel_utility.h

if ISF_BUILD_PANEL_EFL
CONFIG_SCIM_PANEL_EFL  = isf-panel-efl
endif

bin_PROGRAMS           = $(CONFIG_SCIM_PANEL_EFL)

isf_panel_efl_SOURCES  = isf_panel_efl.cpp \
                         isf_panel_utility.cpp \
                         iseselector.cpp \
                         center_popup.cpp

if ISF_BUILD_MINICONTROL
isf_panel_efl_SOURCES  += minicontrol.cpp
endif

isf_panel_efl_CXXFLAGS = @EFL_CFLAGS@ \
                         @EFL_EXTENSION_CFLAGS@ \
                         @VCONF_CFLAGS@ \
                         @X11_CFLAGS@ \
                         @PRIVILEGE_CONTROL_CFLAGS@ \
                         @DLOG_CFLAGS@ \
                         @MINICONTROL_PROVIDER_CFLAGS@ \
                         @NOTIFICATION_CFLAGS@ \
                         @TTS_CFLAGS@ \
                         @EDBUS_CFLAGS@ \
                         @BLUETOOTH_CFLAGS@ \
                         @FEEDBACK_CFLAGS@ \
                         @UIGADGET_CFLAGS@ \
                         @PACKAGE_MANAGER_CFLAGS@ \
                         @PKGMGR_INFO_CFLAGS@ \
                         -fPIE

isf_panel_efl_LDFLAGS  = @EFL_LIBS@ @LTLIBINTL@ -rpath $(libdir) \
                         @EFL_EXTENSION_LIBS@ \
                         @VCONF_LIBS@ \
                         @X11_LIBS@ \
                         @PRIVILEGE_CONTROL_LIBS@ \
                         @DLOG_LIBS@ \
                         @MINICONTROL_PROVIDER_LIBS@ \
                         @NOTIFICATION_LIBS@ \
                         @TTS_LIBS@ \
                         @EDBUS_LIBS@ \
                         @BLUETOOTH_LIBS@ \
                         @FEEDBACK_LIBS@ \
                         @UIGADGET_LIBS@ \
                         @PACKAGE_MANAGER_LIBS@ \
                         @PKGMGR_INFO_LIBS@ \
                         -pie

isf_panel_efl_LDADD    = $(top_builddir)/ism/src/libscim@SCIM_EPOCH@.la

