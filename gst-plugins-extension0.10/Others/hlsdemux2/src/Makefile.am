
plugin_LTLIBRARIES = libgsthlsdemux2.la

libgsthlsdemux2_la_SOURCES =			\
	m3u8.c					\
	gsthlsdemux2.c

libgsthlsdemux2_la_CFLAGS = $(GST_CFLAGS) $(GST_BASE_CFLAGS) $(SOUP_CFLAGS) $(GST_APP_CFLAGS) $(CRYPTO_CFLAGS)
libgsthlsdemux2_la_LIBADD = -lgsttag-@GST_MAJORMINOR@ \
                $(GST_LIBS) $(GST_BASE_LIBS) $(SOUP_LIBS) $(GST_APP_LIBS) -lgstapp-0.10 $(CRYPTO_LIBS)
libgsthlsdemux2_la_LDFLAGS = $(GST_PLUGIN_LDFLAGS) -no-undefined
libgsthlsdemux2_la_LIBTOOLFLAGS = --tag=disable-static

# headers we need but don't want installed
noinst_HEADERS = 			\
	gsthlsdemux2.h		\
	m3u8.h

Android.mk: Makefile.am $(BUILT_SOURCES)
	androgenizer \
	-:PROJECT libgsthls -:SHARED libgsthls \
	 -:TAGS eng debug \
         -:REL_TOP $(top_srcdir) -:ABS_TOP $(abs_top_srcdir) \
	 -:SOURCES $(libgsthls_la_SOURCES) \
	 -:CFLAGS $(DEFS) $(DEFAULT_INCLUDES) $(libgsthls_la_CFLAGS) \
	 -:LDFLAGS $(libgsthls_la_LDFLAGS) \
	           $(libgsthls_la_LIBADD) \
	           -ldl \
	 -:PASSTHROUGH LOCAL_ARM_MODE:=arm \
		       LOCAL_MODULE_PATH:='$$(TARGET_OUT)/lib/gstreamer-0.10' \
	> $@
