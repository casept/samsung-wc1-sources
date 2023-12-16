MAINTAINERCLEANFILES = Makefile.in
CLEANFILES           = *.bak

INCLUDES = -I$(top_builddir) \
           -I$(top_builddir)/ism/src \
           -I$(top_srcdir) \
           -I$(top_srcdir)/ism/src \
           -I$(top_srcdir)/ism/intl \
           -I$(top_srcdir)/ism/data \
           -I$(top_srcdir)/ism/utils \
           -I$(top_srcdir)/ism/demos/include \
           -I$(includedir) \
           -DSCIM_DATADIR=\"@SCIM_DATADIR@\" \
           -DSCIM_LOCALEDIR=\"@SCIM_LOCALEDIR@\" \
           -DSCIM_SYSCONFDIR=\"@SCIM_SYSCONFDIR@\" \
           -DSCIM_LIBEXECDIR=\"@SCIM_LIBEXECDIR@\" \
           -DSCIM_ICONDIR=\"@SCIM_ICONDIR@\" \
           -DSCIM_MODULE_PATH=\"@SCIM_MODULE_PATH@\" \
           -DSCIM_TEMPDIR=\"@SCIM_TEMPDIR@\"


noinst_HEADERS =

if ISF_BUILD_SETTING_EFL
ISF_DEMO_EFL = isf-demo-efl
endif

optexecdir            = /opt/usr/devel/usr/bin/
optexec_PROGRAMS      = $(ISF_DEMO_EFL)

isf_demo_efl_SOURCES  = isf_demo_efl.cpp \
                        isf_imcontrol_efl.cpp \
                        isf_layout_efl.cpp \
                        isf_event_efl.cpp \
                        isf_entry_event_efl.cpp \
                        isf_autocapital_efl.cpp \
                        isf_prediction_efl.cpp \
                        isf_return_key_type_efl.cpp \
                        isf_return_key_disable_efl.cpp \
                        isf_imdata_set_efl.cpp \
                        isf_focus_movement_efl.cpp \
                        isf_language_efl.cpp \
                        isf_ondemand_efl.cpp \
                        isf_input_hint_efl.cpp

isf_demo_efl_CXXFLAGS = @EFL_CFLAGS@ \
                        @APPCORE_EFL_CFLAGS@ \
                        @UIGADGET_CFLAGS@ \
                        @VCONF_CFLAGS@ \
                        @PRIVILEGE_CONTROL_CFLAGS@ \
                        @EFL_EXTENSION_CFLAGS@ \
                        @DLOG_CFLAGS@ \
                        -fPIE

isf_demo_efl_LDFLAGS  = @EFL_LIBS@ @LTLIBINTL@ -rpath $(libdir) \
                        @VCONF_LIBS@ \
                        @APPCORE_EFL_LIBS@ \
                        @UIGADGET_LIBS@ \
                        @PRIVILEGE_CONTROL_LIBS@ \
                        @EFL_EXTENSION_LIBS@ \
                        @DLOG_LIBS@ \
                        -pie

isf_demo_efl_LDADD    = $(top_builddir)/ism/src/libscim@SCIM_EPOCH@.la
