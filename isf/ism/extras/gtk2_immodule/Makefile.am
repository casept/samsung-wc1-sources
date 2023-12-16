MAINTAINERCLEANFILES 	= Makefile.in
EXTRA_DIST		= im-scim.version-script

INCLUDES		= -I$(top_builddir) \
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
			  -DSCIM_TEMPDIR=\"@SCIM_TEMPDIR@\" \
			  -DSCIM_KEYBOARD_ICON_FILE=\"@SCIM_ICONDIR@/keyboard.png\" \
			  @ISF_BUILDING_DLL@

if SCIM_BUILD_GTK2_IMMODULE
CONFIG_GTK2_IMMODULE = im-scim.la
endif

if SCIM_LD_VERSION_SCRIPT
LD_VERSION_SCRIPT_OPTION="-Wl,--version-script=$(srcdir)/im-scim.version-script"
endif

noinst_HEADERS = gtkimcontextscim.h

moduledir = @GTK_IM_MODULEDIR@

module_LTLIBRARIES = $(CONFIG_GTK2_IMMODULE)

im_scim_la_SOURCES = gtkimcontextscim.cpp imscim.cpp

im_scim_la_CXXFLAGS=@GTK2_CFLAGS@
im_scim_la_CFLAGS  =@GTK2_CFLAGS@

im_scim_la_LDFLAGS = -rpath $(moduledir) \
		     -avoid-version \
		     -module \
		     -lstdc++ \
		     $(LD_VERSION_SCRIPT_OPTION) \
		     -L@GTK_LIBDIR@ -lgtk-x11-2.0 -lgdk-x11-2.0 -lglib-2.0 -lgobject-2.0 -lpango-1.0

im_scim_la_LIBADD  = $(top_builddir)/ism/src/libscim@SCIM_EPOCH@.la
