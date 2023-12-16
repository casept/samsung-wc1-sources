MAINTAINERCLEANFILES = Makefile.in
EXTRA_DIST     =

INCLUDES       = -I$(top_builddir) \
                 -I$(top_builddir)/ism/src \
                 -I$(top_srcdir) \
                 -I$(top_srcdir)/ism/src \
                 -I$(top_srcdir)/ism/intl \
                 -I$(top_srcdir)/ism/utils \
                 -I$(includedir) \
                 -DSCIM_DATADIR=\"@SCIM_DATADIR@\" \
                 -DSCIM_LOCALEDIR=\"@SCIM_LOCALEDIR@\" \
                 -DSCIM_SYSCONFDIR=\"@SCIM_SYSCONFDIR@\" \
                 -DSCIM_LIBEXECDIR=\"@SCIM_LIBEXECDIR@\" \
                 -DSCIM_ICONDIR=\"@SCIM_ICONDIR@\" \
                 -DSCIM_MODULE_PATH=\"@SCIM_MODULE_PATH@\" \
                 -DSCIM_TEMPDIR=\"@SCIM_TEMPDIR@\" \
                 -DSCIM_KEYBOARD_ICON_FILE=\"@SCIM_ICONDIR@/keyboard.png\" \
                 @ISF_BUILDING_DLL@

if ISF_BUILD_EFL_IMMODULE
CONFIG_EFL_IMMODULE = libisf-imf-module.la
endif

noinst_HEADERS             = isf_imf_context.h  \
                             isf_imf_control_ui.h \
                             isf_imf_control.h

moduledir                  = @EFL_IM_MODULEDIR@
module_LTLIBRARIES         = $(CONFIG_EFL_IMMODULE)

libisf_imf_module_la_SOURCES  = isf_imf_module.cpp \
                                isf_imf_context.cpp \
                                isf_imf_control_ui.cpp \
                                isf_imf_control.cpp

libisf_imf_module_la_CXXFLAGS = @EFL_CFLAGS@ \
                                @UTILX_CFLAGS@ \
                                @VCONF_CFLAGS@ \
                                @DLOG_CFLAGS@

libisf_imf_module_la_LDFLAGS  = -rpath $(moduledir) \
                                -avoid-version \
                                -module

libisf_imf_module_la_LIBADD  =  -lstdc++ \
                                $(LD_VERSION_SCRIPT_OPTION) \
                                @EFL_LIBS@ \
                                @UTILX_LIBS@ \
                                @VCONF_LIBS@ \
                                @DLOG_LIBS@ \
                                $(top_builddir)/ism/src/libscim@SCIM_EPOCH@.la
