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

MAINTAINERCLEANFILES	= Makefile.in
CLEANFILES		= *.bak
EXTRA_DIST		= imengine.version-script

INCLUDES		= -I$(top_builddir) \
			  -I$(top_builddir)/ism/src \
	    		  -I$(top_srcdir) \
			  -I$(top_srcdir)/ism/src \
			  -I$(top_srcdir)/ism/intl \
			  -I$(top_srcdir)/ism/utils \
			  -DSCIM_DATADIR=\"@SCIM_DATADIR@\" \
			  -DSCIM_LOCALEDIR=\"@SCIM_LOCALEDIR@\" \
			  -DSCIM_SYSCONFDIR=\"@SCIM_SYSCONFDIR@\" \
			  -DSCIM_LIBEXECDIR=\"@SCIM_LIBEXECDIR@\" \
			  -DSCIM_ICONDIR=\"@SCIM_ICONDIR@\" \
			  -DSCIM_MODULE_PATH=\"@SCIM_MODULE_PATH@\" \
			  -DSCIM_TEMPDIR=\"@SCIM_TEMPDIR@\" \
			  @ISF_BUILDING_DLL@

if SCIM_BUILD_IMENGINE_SOCKET
CONFIG_IMENGINE_SOCKET_MODULE	= socket.la
endif

if SCIM_LD_VERSION_SCRIPT
LD_VERSION_SCRIPT_OPTION="-Wl,--version-script=$(srcdir)/imengine.version-script"
endif

noinst_HEADERS		= scim_socket_imengine.h

moduledir		= @SCIM_MODULE_PATH@/$(SCIM_BINARY_VERSION)/IMEngine
module_LTLIBRARIES	= $(CONFIG_IMENGINE_SOCKET_MODULE)

socket_la_SOURCES 	= scim_socket_imengine.cpp

socket_la_LDFLAGS	= -avoid-version \
		     	  -rpath $(moduledir) \
			  -module \
			  $(LD_VERSION_SCRIPT_OPTION) \
			  @LIBTOOL_EXPORT_OPTIONS@ \
			  @LTLIBINTL@
			  
socket_la_LIBADD	= $(top_builddir)/ism/src/libscim@SCIM_EPOCH@.la

