## Makefile.am -- Process this file with automake to produce Makefile.in
## Copyright (C) 2002 James Su
##
## This program is free software; you can redistribute it and/or modify
## it under the terms of the GNU General Public License as published by
## the Free Software Foundation; either version 2, or (at your option)
## any later version.
##
## This program is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## GNU General Public License for more details.
##
## You should have received a copy of the GNU General Public License
## along with this program; if not, write to the Free Software
## Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

MAINTAINERCLEANFILES	= Makefile.in scim_types.h
CLEANFILES		= libltdlc.la libscim.la *.bak
DISTCLEANFILES		= scim_types.h
EXTRA_DIST		= libscim.version-script

INCLUDES		= -I$(top_builddir) \
			  -I$(top_builddir)/ism/src \
	    		  -I$(top_srcdir) \
			  -I$(top_srcdir)/ism/src \
			  -I$(top_srcdir)/ism/intl \
			  -I$(includedir) \
			  -DISF_VERSION=\"@ISF_VERSION@\" \
			  -DSCIM_BINDIR=\"@SCIM_BINDIR@\" \
			  -DSCIM_DATADIR=\"@SCIM_DATADIR@\" \
			  -DSCIM_LOCALEDIR=\"@SCIM_LOCALEDIR@\" \
			  -DSCIM_SYSCONFDIR=\"@SCIM_SYSCONFDIR@\" \
			  -DSCIM_LIBEXECDIR=\"@SCIM_LIBEXECDIR@\" \
			  -DSCIM_ICONDIR=\"@SCIM_ICONDIR@\" \
			  -DSCIM_MODULE_PATH=\"@SCIM_MODULE_PATH@\" \
			  @ISF_BUILDING_DLL@


noinst_HEADERS		= ltdl.h \
			  scim_private.h \
			  scim_compose_key_data.h \
			  scim_keyboard_layout_data.h \
			  scim_keyevent_data.h \
			  scim_stl_map.h \
			  isf_query_utility.h

libscimincludedir       = $(includedir)/scim@SCIM_EPOCH@

libsciminclude_HEADERS 	= scim.h \
			  scim_attribute.h \
			  scim_backend.h \
			  scim_bind.h \
			  scim_compose_key.h \
			  scim_config_base.h \
			  scim_config_module.h \
			  scim_config_path.h \
			  scim_connection.h \
			  scim_debug.h \
			  scim_event.h \
			  scim_exception.h \
			  scim_filter.h \
			  scim_filter_module.h \
			  scim_filter_manager.h \
			  scim_frontend.h \
			  scim_frontend_module.h \
			  scim_global_config.h \
			  scim_helper.h \
			  scim_helper_manager.h \
			  scim_helper_module.h \
			  scim_hotkey.h \
			  scim_iconv.h \
			  scim_imengine.h \
			  scim_imengine_module.h \
			  scim_lookup_table.h \
			  scim_module.h \
			  scim_object.h \
			  scim_panel_agent.h \
			  scim_panel_client.h \
			  scim_panel_common.h \
			  scim_pointer.h \
			  scim_property.h \
			  scim_signals.h \
			  scim_slot.h \
			  scim_socket.h \
			  scim_trans_commands.h \
			  scim_transaction.h \
			  scim_types.h \
			  scim_utility.h \
			  scim_setup_module_efl.h \
			  isf_control.h \
			  isf_imcontrol_client.h \
			  ise_context.h \
			  scim_visibility.h

noinst_LTLIBRARIES	= libltdlc.la

libltdlc_la_SOURCES	= ltdl.cpp
libltdlc_la_LIBADD	= $(LIBADD_DL)

if SCIM_LD_VERSION_SCRIPT
LD_VERSION_SCRIPT_OPTION="-Wl,--version-script=$(srcdir)/libscim.version-script"
endif

lib_LTLIBRARIES 	= libscim@SCIM_EPOCH@.la

libscim@SCIM_EPOCH@_la_SOURCES = \
			  scim_debug.cpp \
			  scim_module.cpp \
			  scim_backend.cpp \
			  scim_chartraits.cpp \
			  scim_compose_key.cpp \
			  scim_config_base.cpp \
			  scim_config_module.cpp \
			  scim_connection.cpp \
			  scim_event.cpp \
			  scim_filter.cpp \
			  scim_filter_module.cpp \
			  scim_filter_manager.cpp \
			  scim_frontend.cpp \
			  scim_frontend_module.cpp \
			  scim_global_config.cpp \
			  scim_helper.cpp \
			  scim_helper_manager.cpp \
			  scim_helper_module.cpp \
			  scim_hotkey.cpp \
			  scim_iconv.cpp \
			  scim_imengine.cpp \
			  scim_imengine_module.cpp \
			  scim_lookup_table.cpp \
			  scim_object.cpp \
			  scim_panel_agent.cpp \
			  scim_panel_client.cpp \
			  scim_private.cpp \
			  scim_signals.cpp \
			  scim_slot.cpp \
			  scim_socket.cpp \
			  scim_transaction.cpp \
			  scim_utility.cpp \
			  scim_setup_module_efl.cpp \
			  isf_control.cpp \
			  isf_imcontrol_client.cpp \
			  isf_query_utility.cpp

libscim@SCIM_EPOCH@_la_CXXFLAGS = @EFL_CFLAGS@ @DLOG_CFLAGS@ @SECURITY_SERVER_CFLAGS@ @VCONF_CFLAGS@

libscim@SCIM_EPOCH@_la_LDFLAGS  = -version-info $(SCIM_CURRENT):$(SCIM_REVISION):$(SCIM_AGE) \
			  -export-dynamic \
			  -rpath $(libdir) \
			  $(LD_VERSION_SCRIPT_OPTION) \
			  @DLOG_LIBS@ \
			  @LIBTOOL_EXPORT_OPTIONS@ \
			  @LIBICONV@ \
			  @LTLIBINTL@ \
			  @SECURITY_SERVER_LIBS@ \
			  @EFL_LIBS@ \
			  @VCONF_LIBS@ \
			  -lstdc++

libscim@SCIM_EPOCH@_la_LIBADD	= libltdlc.la


bin_PROGRAMS		= scim isf-log isf-query-engines

scim_SOURCES	 	= scim.cpp
scim_CXXFLAGS		= @EFL_CFLAGS@ @PRIVILEGE_CONTROL_CFLAGS@ @VCONF_CFLAGS@ -fPIE
scim_LDADD		= libscim@SCIM_EPOCH@.la
scim_LDFLAGS		= @LTLIBINTL@ @EFL_LIBS@ @PRIVILEGE_CONTROL_LIBS@ @VCONF_LIBS@ -pie

isf_log_SOURCES	 	= isf_log.cpp
isf_log_LDADD		= libscim@SCIM_EPOCH@.la
if SCIM_BUILD_GTK_UTILS
isf_log_CXXFLAGS	= @GTK2_CFLAGS@ -fPIE
isf_log_LDFLAGS		= @LTLIBINTL@ -rpath $(libdir) -L@GTK_LIBDIR@ -lgtk-x11-2.0 -lgdk-x11-2.0 -lglib-2.0 -lgobject-2.0 -pie
else
isf_log_CXXFLAGS	= -fPIE
isf_log_LDFLAGS		= @LTLIBINTL@ -rpath $(libdir) -lglib-2.0 -lgobject-2.0 -pie
endif

isf_query_engines_SOURCES  = isf_query_engines.cpp
isf_query_engines_CXXFLAGS = @VCONF_CFLAGS@ @PRIVILEGE_CONTROL_CFLAGS@ -fPIE
isf_query_engines_LDADD    = libscim@SCIM_EPOCH@.la
isf_query_engines_LDFLAGS  = @LTLIBINTL@ @VCONF_LIBS@ @PRIVILEGE_CONTROL_LIBS@ -pie

libexecdir		= $(libdir)/scim@SCIM_EPOCH@
libexec_PROGRAMS        = scim-launcher scim-helper-launcher

scim_launcher_SOURCES 	= scim_launcher.cpp
scim_launcher_LDADD	= libscim@SCIM_EPOCH@.la
scim_launcher_CXXFLAGS  = @PRIVILEGE_CONTROL_CFLAGS@ @VCONF_CFLAGS@ -fPIE
scim_launcher_LDFLAGS	= @LTLIBINTL@ @PRIVILEGE_CONTROL_LIBS@ @VCONF_LIBS@ -pie

scim_helper_launcher_SOURCES 	= scim_helper_launcher.cpp ise_preexec.cpp
scim_helper_launcher_CXXFLAGS	= @PRIVILEGE_CONTROL_CFLAGS@ @EFL_CFLAGS@ @PKGMGR_INFO_CFLAGS@ @DLOG_CFLAGS@ -fPIE
scim_helper_launcher_LDADD	= libscim@SCIM_EPOCH@.la
scim_helper_launcher_LDFLAGS	= @LTLIBINTL@ @PRIVILEGE_CONTROL_LIBS@ @EFL_LIBS@ @PKGMGR_INFO_LIBS@ @DLOG_LIBS@ -pie



