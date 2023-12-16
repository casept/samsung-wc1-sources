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

MAINTAINERCLEANFILES = Makefile.in
CLEANFILES           = *.bak

INCLUDES             = -I$(top_builddir) \
                       -I$(top_builddir)/ism/src \
                       -I$(top_srcdir) \
                       -I$(top_srcdir)/ism/src \
                       -I$(top_srcdir)/ism/intl \
                       -I$(top_srcdir)/ism/data \
                       -I$(top_srcdir)/ism/utils \
                       -I$(top_srcdir)/ism/extras/panel \
                       -I$(top_srcdir)/idm/src \
                       -DSCIM_DATADIR=\"@SCIM_DATADIR@\" \
                       -DSCIM_LOCALEDIR=\"@SCIM_LOCALEDIR@\" \
                       -DSCIM_SYSCONFDIR=\"@SCIM_SYSCONFDIR@\" \
                       -DSCIM_LIBEXECDIR=\"@SCIM_LIBEXECDIR@\" \
                       -DSCIM_ICONDIR=\"@SCIM_ICONDIR@\" \
                       -DSCIM_MODULE_PATH=\"@SCIM_MODULE_PATH@\" \
                       @ISF_BUILDING_DLL@

if SCIM_BUILD_FRONTEND_X11
CONFIG_FRONTEND_X11_MODULE    = x11.la
SUBDIRS                       = imdkit
endif

if SCIM_BUILD_FRONTEND_SOCKET
CONFIG_FRONTEND_SOCKET_MODULE = socket.la
endif

noinst_HEADERS     = scim_x11_ic.h \
                     scim_x11_frontend.h \
                     scim_socket_frontend.h

moduledir          = @SCIM_MODULE_PATH@/$(SCIM_BINARY_VERSION)/FrontEnd

module_LTLIBRARIES = $(CONFIG_FRONTEND_X11_MODULE) $(CONFIG_FRONTEND_SOCKET_MODULE)

x11_la_SOURCES     = scim_x11_frontend.cpp \
                     scim_x11_ic.cpp

x11_la_CFLAGS      = @X_CFLAGS@

x11_la_CXXFLAGS    = @X_CFLAGS@

x11_la_LDFLAGS     = -avoid-version \
                     -rpath $(moduledir) \
                     -module \
                     @LIBTOOL_EXPORT_OPTIONS@ \
                     @X_LIBS@ \
                     @LTLIBINTL@

x11_la_LIBADD      = imdkit/libXimd.la \
                     $(top_builddir)/ism/src/libscim@SCIM_EPOCH@.la \
                     $(top_builddir)/ism/utils/libscim-x11utils@SCIM_EPOCH@.la

socket_la_SOURCES  = scim_socket_frontend.cpp

socket_la_CXXFLAGS = @DLOG_CFLAGS@ @ECORE_CFLAGS@ @PKGMGR_INFO_CFLAGS@

socket_la_LDFLAGS  = -avoid-version \
                     -rpath $(moduledir) \
                     -module \
                     @DLOG_LIBS@ \
                     @ECORE_LIBS@ \
                     @PKGMGR_INFO_LIBS@ \
                     @LIBTOOL_EXPORT_OPTIONS@

socket_la_LIBADD   = $(top_builddir)/ism/src/libscim@SCIM_EPOCH@.la

