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

INST_ICONS		= keyboard.png pop_play.png

SCTC_FILTER_ICONS	= sctc.png \
			  sctc-sc-to-tc.png \
			  sctc-tc-to-sc.png

if SCIM_BUILD_FILTER_SCTC
INST_SCTC_ICONS		= $(SCTC_FILTER_ICONS)
endif

EXTRA_DIST		= $(INST_ICONS) \
			  $(SCTC_FILTER_ICONS)

icondir			= @SCIM_ICONDIR@
icon_DATA		= $(INST_ICONS) $(INST_SCTC_ICONS)

