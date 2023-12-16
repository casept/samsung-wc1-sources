# Copyright (c) 2006 Software Laboratory, SAMSUNG Electronics, Inc.
# Orignal by Jeechul jeon <heechul.jeon@samsung.com>


ACLOCAL_AMFLAGS = -I common/m4

aclocaldir = $(datadir)/aclocal

SUBDIRS = \
	common \
	Others

DIST_SUBDIRS = \
	common \
	Others

EXTRA_DIST = \
	gstreamer.spec gstreamer.spec.in \
	configure.ac autogen.sh depcomp \
	MAINTAINERS ABOUT-NLS RELEASE gstreamer.doap \
	$(win32)

check: check-exports

