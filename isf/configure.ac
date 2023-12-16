# Process this file with autoconf to produce a configure script
#AC_INIT(scim, 1.4.7, suzhe@tsinghua.org.cn)
AC_INIT(isf, 2.4.8513, isf@samsung.com)
AC_CONFIG_SRCDIR([ism/src/scim.h])

###########################################################
##          Some definitions of Version macros.          ##
###########################################################

# ISF version
ISF_MAJOR_VERSION=2
ISF_MINOR_VERSION=4
ISF_MICRO_VERSION=8513

ISF_VERSION=$ISF_MAJOR_VERSION.$ISF_MINOR_VERSION.$ISF_MICRO_VERSION

AC_SUBST(ISF_MAJOR_VERSION)
AC_SUBST(ISF_MINOR_VERSION)
AC_SUBST(ISF_MICRO_VERSION)
AC_SUBST(ISF_VERSION)

# SCIM version
SCIM_MAJOR_VERSION=1
SCIM_MINOR_VERSION=4
SCIM_MICRO_VERSION=7

SCIM_VERSION=$SCIM_MAJOR_VERSION.$SCIM_MINOR_VERSION.$SCIM_MICRO_VERSION

AC_SUBST(SCIM_MAJOR_VERSION)
AC_SUBST(SCIM_MINOR_VERSION)
AC_SUBST(SCIM_MICRO_VERSION)
AC_SUBST(SCIM_VERSION)

# libtool versioning for SCIM

# increment if the interface has additions, changes, removals.
SCIM_CURRENT=10

# increment any time the source changes; set to 0 if you increment CURRENT
SCIM_REVISION=3

# increment if any interfaces have been added; set to 0
# if any interfaces have been removed. removal has 
# precedence over adding, so set to 0 if both happened.
SCIM_AGE=2

AC_SUBST(SCIM_CURRENT)
AC_SUBST(SCIM_REVISION)
AC_SUBST(SCIM_AGE)

# Define a string for the earliest version that this release cannot
# coexist with. This is used for directory hierarchies.
#
SCIM_EPOCH=-1.0
AC_SUBST(SCIM_EPOCH)

# Define a string for the earliest version that this release has
# binary compatibility with. This is used for module locations.
#
SCIM_BINARY_VERSION=1.4.0
AC_SUBST(SCIM_BINARY_VERSION)

AC_DEFINE_UNQUOTED(SCIM_BINARY_VERSION, "$SCIM_BINARY_VERSION", [The binary version of SCIM library.])
AC_DEFINE_UNQUOTED(SCIM_VERSION, "$SCIM_VERSION", [The release version of SCIM library.])
AC_DEFINE_UNQUOTED(SCIM_MAJOR_VERSION, $SCIM_MAJOR_VERSION, [The Major version of SCIM library.])
AC_DEFINE_UNQUOTED(SCIM_MINOR_VERSION, $SCIM_MINOR_VERSION, [The Minor version of SCIM library.])
AC_DEFINE_UNQUOTED(SCIM_MICRO_VERSION, $SCIM_MICRO_VERSION, [The Micro version of SCIM library.])

GETTEXT_PACKAGE=scim
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE,"$GETTEXT_PACKAGE", [The gettext domain])

###########################################################
##                  Start Configuration.                 ##
###########################################################
AC_CANONICAL_HOST

# Init automake stuff
AM_INIT_AUTOMAKE
AM_CONFIG_HEADER([config.h])

# Init platform
PLATFORM_INIT

# Init gettext
ALL_LINGUAS="hy az eu bg ca zh_CN zh_HK zh_TW hr cs da nl_NL en en_US et fi fr_FR gl ka de_DE el_GR hu is ga it_IT ja_JP kk ko_KR lv lt mk nb pl pt_PT pt_BR ro ru_RU sr sk sl es_ES es_US es_MX sv tr_TR uk uz ar zh_SG hi en_PH fr_CA fa th ur as bn gu id km kn lo ml mn_MN mr ms my ne or pa si ta te tl vi"
AM_GNU_GETTEXT

# Init libtool
AC_LIBTOOL_DLOPEN
AC_PROG_LIBTOOL
AC_LIB_LTDL
AC_SUBST(LIBTOOL_DEPS)

# libtool option to control which symbols are exported
# right now, symbols starting with _ are not exported
# !!! DO NOT USE IT !!!
LIBTOOL_EXPORT_OPTIONS=''
AC_SUBST(LIBTOOL_EXPORT_OPTIONS)

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC
AC_LANG(C++)

# Checks if doxygen is avaiable

DOXYGEN=no
if test "x$with_doxygen" != "xno"; then
  AC_PATH_PROG(DOXYGEN, doxygen, no)
fi
 
AM_CONDITIONAL(HAVE_DOXYGEN, test x$DOXYGEN != xno)
AC_SUBST(DOXYGEN)

# Checks if graphviz is available
AC_CHECK_PROG(HAVE_GRAPHVIZ, dot, YES, NO)
AC_PATH_PROG(DOT, dot)
DOT=`dirname $DOT`
AC_SUBST(DOT)

AC_PATH_PROG(PERL, perl)
AC_SUBST(PERL)

# Check intltool
#IT_PROG_INTLTOOL([0.33], [no-xml])

# Checks for libraries.
AC_HEADER_STDC
AC_HEADER_TIME
AC_CHECK_HEADERS([langinfo.h libintl.h string.h dirent.h hash_map ext/hash_map])
AC_CHECK_HEADERS([stdio.h ctype.h errno.h assert.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_C_INLINE
AC_TYPE_SIZE_T

if test x$ac_cv_type_size_t != xyes; then
  AC_MSG_ERROR([No type size_t, but SCIM needs it!])
fi

AC_CHECK_SIZEOF(char)
AC_CHECK_SIZEOF(unsigned short int)
AC_CHECK_SIZEOF(unsigned int)
AC_CHECK_SIZEOF(unsigned long int)
AC_CHECK_SIZEOF(unsigned long long int)
AC_CHECK_HEADERS([stdint.h],[has_stdint=yes],[has_stdint=no])

if test x$ac_cv_sizeof_unsigned_short_int = x2 && test x$ac_cv_sizeof_char = x1 ; then
  :
else
  AC_MSG_ERROR([
*** SCIM requires
***      sizeof (unsigned short int)  == 2
***      sizeof (char)                == 1
*** You might want to consider using the GNU C compiler.
])
fi

if test x$ac_cv_sizeof_wchar_t = x0 ; then
  AC_MSG_ERROR([
*** SCIM requires a compiler that supports wchar_t,
*** You might want to consider using the GNU C compiler.
])
fi

if test x$has_stdint = xyes; then
  INCLUDE_STDINT="#include <stdint.h>"
  UINT16="uint16_t"
  UINT32="uint32_t"
  UINT64="uint64_t"
else
  INCLUDE_STDINT="//#include <stdint.h>"
  UINT16="unsigned short int"
  if test x$ac_cv_sizeof_unsigned_int = x4; then
    UINT32="unsigned int"
  else
    if test x$ac_cv_sizeof_unsigned_long_int = x4; then
      UINT32="unsigned long int"
    else
      AC_MSG_ERROR([*** No suitable integer type for uint32 found.])
    fi
  fi
  if test x$ac_cv_sizeof_unsigned_long_long_int = x8; then
    UINT64="unsigned long long int"
  else
    AC_MSG_ERROR([*** No suitable integer type for uint64 found.])
  fi
fi
AC_SUBST(INCLUDE_STDINT)
AC_SUBST(UINT16)
AC_SUBST(UINT32)
AC_SUBST(UINT64)

# Checks for library functions.
AC_FUNC_MALLOC
AC_CHECK_FUNCS([gettimeofday memmove memset nl_langinfo setlocale daemon])
AC_CHECK_FUNCS([memcpy strcmp strchr strrchr])
AC_CHECK_FUNCS([opendir closedir readdir])
AC_CHECK_FUNCS([usleep nanosleep])
AC_CHECK_FUNCS([gethostbyname gethostbyname_r socket bind accept connect listen],
               [socket_ok=yes],
               [socket_ok=no])

AM_ICONV

# Check gconf library
PKG_CHECK_MODULES(GCONF, [gconf-2.0 phonestatus noti-service],
                  [ISF_HAS_GCONF=yes],
                  [ISF_HAS_GCONF=no])

if test "$ISF_HAS_GCONF" = "yes"; then
  AC_DEFINE(HAVE_GCONF,1,[Have GConf functions.])
fi

# Check ECORE
PKG_CHECK_MODULES(ECORE, [ecore])

# Check EFL library
PKG_CHECK_MODULES(EFL, [elementary ecore-x evas ecore ecore-evas ecore-file edje ecore-input ecore-ipc ecore-imf],
                  [ISF_HAS_EFL=yes],
                  [ISF_HAS_EFL=no])

if test "$ISF_HAS_EFL" = "yes"; then
  EFL_LIBDIR=`$PKG_CONFIG --variable=libdir ecore-imf`
  if test -z "$EFL_LIBDIR)"; then
    EFL_LIBDIR="$libdir"
  fi
  EFL_IM_MODULEDIR=$EFL_LIBDIR/ecore/immodules
  AC_SUBST(EFL_LIBDIR)
  AC_SUBST(EFL_IM_MODULEDIR)
fi

# Check EFL assist library
PKG_CHECK_MODULES(EFL_EXTENSION, [efl-extension])

# Check x11 library
PKG_CHECK_MODULES(X11, [x11])

# Check ui-gadget library
PKG_CHECK_MODULES(UIGADGET, [ui-gadget-1],
                  [ISF_HAS_UIGADGET=yes],
                  [ISF_HAS_UIGADGET=no])

# Check utilX library
PKG_CHECK_MODULES(UTILX, [utilX])

# Check appcore-efl library
PKG_CHECK_MODULES(APPCORE_EFL, [appcore-efl])

# Check dlog library
PKG_CHECK_MODULES(DLOG, [dlog >= 0])

# Check privilege control library
PKG_CHECK_MODULES(PRIVILEGE_CONTROL, [libprivilege-control >= 0])

# Check minicontrol library
PKG_CHECK_MODULES(MINICONTROL_PROVIDER, [minicontrol-provider],
                  [ISF_HAS_MINICONTROL=yes],
                  [ISF_HAS_MINICONTROL=no])

# Check notification library
PKG_CHECK_MODULES(NOTIFICATION, [notification],
                  [ISF_HAS_NOTIFICATION=yes],
                  [ISF_HAS_NOTIFICATION=no])

# Check tts library
PKG_CHECK_MODULES(TTS, [tts],
                  [ISF_HAS_TTS=yes],
                  [ISF_HAS_TTS=no])

if test "$ISF_HAS_TTS" = "yes"; then
  AC_DEFINE(HAVE_TTS,1,[Have TTS functions.])
fi

# Check security server
PKG_CHECK_MODULES(SECURITY_SERVER, [security-server])

# Check vconf library
PKG_CHECK_MODULES(VCONF, [vconf],
                  [ISF_HAS_VCONF=yes],
                  [ISF_HAS_VCONF=no])

# Check edbus library
PKG_CHECK_MODULES(EDBUS, [edbus])

# Check bluetooth library
PKG_CHECK_MODULES(BLUETOOTH, [capi-network-bluetooth],
                  [ISF_HAS_BLUETOOTH=yes],
                  [ISF_HAS_BLUETOOTH=no])

# Check feedback library
PKG_CHECK_MODULES(FEEDBACK, [feedback],
                  [ISF_HAS_FEEDBACK=yes],
                  [ISF_HAS_FEEDBACK=no])

# Check package manager
PKG_CHECK_MODULES(PACKAGE_MANAGER, [capi-appfw-package-manager])

# Check core package manager
PKG_CHECK_MODULES(PKGMGR_INFO, [pkgmgr-info],
                  [ISF_HAS_PKGMGR_INFO=yes],
                  [ISF_HAS_PKGMGR_INFO=no])

if test "$ISF_HAS_VCONF" = "yes"; then
  AC_DEFINE(HAVE_VCONF,1,[Have VConf functions.])
fi

if test "$ISF_HAS_BLUETOOTH" = "yes"; then
  AC_DEFINE(HAVE_BLUETOOTH,1,[Have Bluetooth functions.])
fi

if test "$ISF_HAS_FEEDBACK" = "yes"; then
  AC_DEFINE(HAVE_FEEDBACK,1,[Have Feedback functions.])
fi

if test "$ISF_HAS_MINICONTROL" = "yes"; then
  AC_DEFINE(HAVE_MINICONTROL,1,[Have Minicontrol functions.])
fi

if test "$ISF_HAS_NOTIFICATION" = "yes"; then
  AC_DEFINE(HAVE_NOTIFICATION,1,[Have Notification functions.])
fi

if test "$ISF_HAS_UIGADGET" = "yes"; then
  AC_DEFINE(HAVE_UIGADGET,1,[Have UI gadget functions.])
fi

if test "$ISF_HAS_PKGMGR_INFO" = "yes"; then
  AC_DEFINE(HAVE_PKGMGR_INFO,1,[Have Package manager functions.])
fi

# Check GTK2 library
PKG_CHECK_MODULES(GTK2, [gtk+-2.0 >= 2.4.0 pango >= 1.1.0 gdk-pixbuf-2.0 >= 2.4.0],
                  [SCIM_HAS_GTK2=yes],
                  [SCIM_HAS_GTK2=no])

SCIM_HAS_GTK2_2=no
if test "$SCIM_HAS_GTK2" = "yes"; then
  if $PKG_CONFIG --exists "gtk+-2.0 >= 2.2" ; then
    SCIM_HAS_GTK2_2=yes
    GTK_VERSION=2.2.0
    AC_DEFINE(GDK_MULTIHEAD_SAFE,1,[Force use of GDK's multihead-safe APIs.])
  fi
  if $PKG_CONFIG --exists "gtk+-2.0 >= 2.3.5" ; then
    SCIM_HAS_GTK2_4=yes
    GTK_VERSION=2.3.5
    AC_DEFINE(HAVE_GTK_DRAW_INSERTION_CURSOR,1,[Have gtk_draw_insertion_cursor ().])
  fi
  GTK_BINARY_VERSION=`$PKG_CONFIG --variable=gtk_binary_version gtk+-2.0`
  GTK_LIBDIR=`$PKG_CONFIG --variable=libdir gtk+-2.0`
  if test -z "$GTK_LIBDIR)"; then
    GTK_LIBDIR="$libdir"
  fi
  GTK_IM_MODULEDIR=$GTK_LIBDIR/gtk-2.0/$GTK_BINARY_VERSION/immodules
  AC_SUBST(GTK_LIBDIR)
  AC_SUBST(GTK_IM_MODULEDIR)
  AC_SUBST(GTK_VERSION)
  AC_SUBST(GTK_BINARY_VERSION)
fi

# Check if we have gthread
PKG_CHECK_MODULES(GTHREAD2,[gthread-2.0 >= 2.0.0],
                  [SCIM_HAS_GTHREAD2=yes],
                  [SCIM_HAS_GTHREAD2=no])

# Checks for X11 header files.
AC_PATH_XTRA

# Add -lX11 to X_LIBS
if test "$have_x" = "yes"; then
  X_LIBS="$X_LIBS -lX11"
fi

case $host_cpu in
  *arm* ) TARGET=ARM;;
      * ) TARGET=X86;;
esac
AM_CONDITIONAL(ARM, test x$TARGET = xARM)

###########################################################
##             Definiation of compile args.              ##
###########################################################
AC_ARG_WITH(gtk-im-module-dir,
            AS_HELP_STRING([--with-gtk-im-module-dir=dir],
                           [Select gtk immodule dir]),
            GTK_IM_MODULEDIR=$with_gtk_im_module_dir)

AC_ARG_WITH(efl-im-module-dir,
            AS_HELP_STRING([--with-efl-im-module-dir=dir],
                           [Select efl immodule dir]),
            EFL_IM_MODULEDIR=$with_efl_im_module_dir)

AC_ARG_ENABLE(ld-version-script,
              [  --enable-ld-version-script Use ld version script to limit exported symbols],
              enable_ld_version_script=yes,
              enable_ld_version_script=no)

AC_ARG_ENABLE(debug,
              [  --enable-debug            Turn on debugging],
              enable_debug=yes,
              enable_debug=no)

AC_ARG_ENABLE(x86,
              [  --enable-x86              Build for x86 host],
              enable_x86=yes,
              enable_x86=no)

AC_ARG_ENABLE(hash-map,
              [  --enable-hash-map         Use hast_map instead of std::map internally],
              enable_hash_map=yes,
              enable_hash_map=no)

AC_ARG_ENABLE(config-simple,
              [  --disable-config-simple   Do not build Simple Config module],
              enable_config_simple=no,
              enable_config_simple=yes)

AC_ARG_ENABLE(config-socket,
              [  --disable-config-socket   Do not build Socket Config module],
              enable_config_socket=no,
              enable_config_socket=yes)

AC_ARG_ENABLE(frontend-x11,
              [  --disable-frontend-x11    Do not build X11 FrontEnd module],
              enable_frontend_x11=no,
              enable_frontend_x11=yes)

AC_ARG_ENABLE(frontend-socket,
              [  --disable-frontend-socket Do not build Socket FrontEnd module],
              enable_frontend_socket=no,
              enable_frontend_socket=yes)

AC_ARG_ENABLE(im-socket,
              [  --disable-im-socket       Do not build Socket IMEngine module],
              enable_im_socket=no,
              enable_im_socket=yes)

AC_ARG_ENABLE(filter-sctc,
              [  --disable-filter-sctc     Do not build Simplified/Traditional Chinese conversion Filter module],
              enable_filter_sctc=no,
              enable_filter_sctc=yes)

AC_ARG_ENABLE(gtk2-immodule,
              [  --disable-gtk2-immodule   Do not build GTK2 IMModule],
              enable_gtk2_immodule=no,
              enable_gtk2_immodule=yes)

AC_ARG_ENABLE(efl-immodule,
              [  --disable-efl-immodule    Do not build EFL IMModule],
              enable_efl_immodule=no,
              enable_efl_immodule=yes)

AC_ARG_ENABLE(panel-efl,
              [  --disable-panel-efl       Do not build EFL Panel],
              enable_panel_efl=no,
              enable_panel_efl=check)

AC_ARG_ENABLE(setting-efl,
              [  --disable-setting-efl     Do not build EFL Setting],
              enable_setting_efl=no,
              enable_setting_efl=check)

AC_ARG_ENABLE(tray-icon,
              [  --disable-tray-icon       Do not build Tray Icon],
              enable_tray_icon=no,
              enable_tray_icon=yes)

AC_ARG_ENABLE(multiwindow-support,
              [  --disable-multiwindow-support       Do not build Multiwindow Support],
              enable_multiwindow_support=no,
              enable_multiwindow_support=yes)

if test "$have_x" = "yes"; then
  SCIM_BUILD_X11_UTILS=1
  enable_x11_utils=yes
else
  SCIM_BUILD_X11_UTILS=0
  enable_x11_utils=no
fi

if test "$enable_hash_map" = "yes"; then
  AC_DEFINE(ENABLE_HASH_MAP,1,[Use hash map instead of map in libscim])
fi

if test "$enable_debug" = "yes"; then
  AC_DEFINE(ENABLE_DEBUG,1,[Define this to enable the debug facility in libscim])
  CFLAGS="$CFLAGS -g"
  CXXFLAGS="$CXXFLAGS -g"
fi

if test "$enable_x86" = "yes"; then
  AC_DEFINE(ENABLE_X86_HOST,1,[Build for x86 host])
fi

if test "$enable_tests" = "yes"; then
  SCIM_BUILD_TESTS=1
else
  SCIM_BUILD_TESTS=0
  enable_tests=no
fi

if test "$enable_config_simple" = "yes"; then
  SCIM_BUILD_CONFIG_SIMPLE=1
else
  SCIM_BUILD_CONFIG_SIMPLE=0
  enable_config_simple=no
fi

if test "$enable_config_socket" = "yes" -a "$socket_ok" = "yes"; then
  SCIM_BUILD_CONFIG_SOCKET=1
else
  SCIM_BUILD_CONFIG_SOCKET=0
  enable_config_socket=no
fi

if test "$enable_frontend_x11" = "yes" -a "$have_x" = "yes"; then
  SCIM_BUILD_FRONTEND_X11=1
else
  SCIM_BUILD_FRONTEND_X11=0
  enable_frontend_x11=no
fi

if test "$enable_frontend_socket" = "yes" -a "$socket_ok" = "yes"; then
  SCIM_BUILD_FRONTEND_SOCKET=1
else
  SCIM_BUILD_FRONTEND_SOCKET=0
  enable_frontend_socket=no
fi

if test "$enable_im_rawcode" = "yes"; then
  SCIM_BUILD_IMENGINE_RAWCODE=1
else
  SCIM_BUILD_IMENGINE_RAWCODE=0
  enable_im_rawcode=no
fi

if test "$enable_im_socket" = "yes" -a "$socket_ok" = "yes"; then
  SCIM_BUILD_IMENGINE_SOCKET=1
else
  SCIM_BUILD_IMENGINE_SOCKET=0
  enable_im_socket=no
fi

if test "$enable_filter_sctc" = "yes"; then
  SCIM_BUILD_FILTER_SCTC=1
else
  SCIM_BUILD_FILTER_SCTC=0
  enable_filter_sctc=no
fi

if test "$enable_gtk2_immodule" = "yes" -a "$SCIM_HAS_GTK2" = "yes"; then
  SCIM_BUILD_GTK2_IMMODULE=1
else
  SCIM_BUILD_GTK2_IMMODULE=0
  enable_gtk2_immodule=no
fi

if test "$SCIM_HAS_GTK2" = "yes"; then
  enable_gtk_utils=yes
  SCIM_BUILD_GTK_UTILS=1
else
  enable_gtk_utils=no
  SCIM_BUILD_GTK_UTILS=0
fi

if test "$enable_tray_icon" = "yes"; then
  if test "$SCIM_HAS_GTK2_2" = "yes" -a "$no_x" != "yes"; then
    enable_tray_icon=yes
    AC_DEFINE(ENABLE_TRAY_ICON,1,[Enable the TrayIcon code.])
  fi
else
  enable_tray_icon=no
fi

if test "$enable_multiwindow_support" = "yes"; then
  enable_multiwindow_support=yes
  AC_DEFINE(ENABLE_MULTIWINDOW_SUPPORT,1,[Enable the Multi Window code.])
else
  enable_multiwindow_support=no
fi

if test "$enable_efl_immodule" = "yes" -a "$ISF_HAS_EFL" = "yes"; then
  ISF_BUILD_EFL_IMMODULE=1
else
  ISF_BUILD_EFL_IMMODULE=0
  enable_efl_immodule=no
fi

ISF_BUILD_PANEL_EFL=0
if test "$enable_panel_efl" != "no"; then
  enable_panel_efl=no
  if test "$ISF_HAS_EFL" = "yes"; then
    ISF_BUILD_PANEL_EFL=1
    enable_panel_efl=yes
  fi
fi

ISF_BUILD_SETTING_EFL=0
if test "$enable_setting_efl" != "no"; then
  enable_setting_efl=no
  if test "$ISF_HAS_EFL" = "yes" -a "$ISF_HAS_VCONF" = "yes"; then
    ISF_BUILD_SETTING_EFL=1
    enable_setting_efl=yes
  fi
fi

#if nothing needs libscim-gtkutils, just disable it
if test "$enable_panel_gtk" != "yes"; then
  enable_gtk_utils=no
  SCIM_BUILD_GTK_UTILS=0
fi

AM_CONDITIONAL(SCIM_LD_VERSION_SCRIPT,
                [test "$enable_ld_version_script" = "yes"])

AM_CONDITIONAL(SCIM_BUILD_TESTS,
                [test "$enable_tests" = "yes"])

AM_CONDITIONAL(SCIM_BUILD_CONFIG_SIMPLE,
                [test "$enable_config_simple" = "yes"])

AM_CONDITIONAL(SCIM_BUILD_CONFIG_SOCKET,
                [test "$enable_config_socket" = "yes"])

AM_CONDITIONAL(SCIM_BUILD_FRONTEND_X11,
                [test "$enable_frontend_x11" = "yes"])

AM_CONDITIONAL(SCIM_BUILD_FRONTEND_SOCKET,
                [test "$enable_frontend_socket" = "yes"])

AM_CONDITIONAL(SCIM_BUILD_IMENGINE_SOCKET,
                [test "$enable_im_socket" = "yes"])

AM_CONDITIONAL(SCIM_BUILD_IMENGINE_RAWCODE,
                [test "$enable_im_rawcode" = "yes"])

AM_CONDITIONAL(SCIM_BUILD_GTK2_IMMODULE,
                [test "$enable_gtk2_immodule" = "yes"])

AM_CONDITIONAL(SCIM_BUILD_FILTER_SCTC,
                [test "$enable_filter_sctc" = "yes"])

AM_CONDITIONAL(SCIM_BUILD_GTK_UTILS,
                [test "$enable_gtk_utils" = "yes"])

AM_CONDITIONAL(SCIM_BUILD_X11_UTILS,
                [test "$enable_x11_utils" = "yes"])

AM_CONDITIONAL(SCIM_BUILD_TRAY_ICON,
                [test "$enable_tray_icon" = "yes"])

AM_CONDITIONAL(ISF_BUILD_EFL_IMMODULE,
                [test "$enable_efl_immodule" = "yes"])

AM_CONDITIONAL(ISF_BUILD_PANEL_EFL,
                [test "$enable_panel_efl" = "yes"])

AM_CONDITIONAL(ISF_BUILD_SETTING_EFL,
                [test "$enable_setting_efl" = "yes"])

AM_CONDITIONAL(ISF_BUILD_WITH_GCONF,
                [test "$ISF_HAS_GCONF" = "yes"])

AM_CONDITIONAL(ISF_BUILD_MINICONTROL,
                [test "$ISF_HAS_MINICONTROL" = "yes"])

AC_SUBST(SCIM_BUILD_TESTS)
AC_SUBST(SCIM_BUILD_CONFIG_SIMPLE)
AC_SUBST(SCIM_BUILD_CONFIG_SOCKET)
AC_SUBST(SCIM_BUILD_FRONTEND_X11)
AC_SUBST(SCIM_BUILD_FRONTEND_SOCKET)
AC_SUBST(SCIM_BUILD_IMENGINE_RAWCODE)
AC_SUBST(SCIM_BUILD_IMENGINE_TABLE)
AC_SUBST(SCIM_BUILD_IMENGINE_SOCKET)
AC_SUBST(SCIM_BUILD_FILTER_SCTC)
AC_SUBST(SCIM_BUILD_GTK2_IMMODULE)
AC_SUBST(SCIM_BUILD_SCIM_SETUP)
AC_SUBST(SCIM_BUILD_GTK_UTILS)
AC_SUBST(SCIM_BUILD_X11_UTILS)
AC_SUBST(ISF_BUILD_EFL_IMMODULE)
AC_SUBST(ISF_BUILD_PANEL_EFL)
AC_SUBST(ISF_BUILD_SETTING_EFL)
AC_SUBST(ISF_BUILD_WITH_GCONF)

ISF_BUILDING_DLL="-DISF_BUILDING_DLL"
AC_SUBST(ISF_BUILDING_DLL)

# Output All necessary Paths.
SCIM_BINDIR="$prefix/bin"
SCIM_DATADIR="$datadir/scim"
SCIM_SYSCONFDIR="$sysconfdir"
SCIM_ICONDIR="$datadir/scim/icons"
SCIM_MODULE_PATH="$libdir/scim$SCIM_EPOCH"
SCIM_LIBEXECDIR="$libdir/scim$SCIM_EPOCH"
SCIM_LOCALEDIR="/usr/share/locale"
SCIM_TEMPDIR="/tmp"

AC_SUBST(SCIM_BINDIR)
AC_SUBST(SCIM_DATADIR)
AC_SUBST(SCIM_SYSCONFDIR)
AC_SUBST(SCIM_ICONDIR)
AC_SUBST(SCIM_MODULE_PATH)
AC_SUBST(SCIM_LIBEXECDIR)
AC_SUBST(SCIM_LOCALEDIR)
AC_SUBST(SCIM_TEMPDIR)

###########################################################
##                      Output files.                    ##
###########################################################
AC_SUBST(ac_aux_dir)
AC_CONFIG_FILES([Makefile
                 po/scim/Makefile.in
                 ism/src/Makefile
                 ism/src/scim_types.h
                 ism/utils/Makefile
                 ism/data/Makefile
                 ism/data/icons/Makefile
                 ism/data/pixmaps/Makefile
                 ism/modules/Makefile
                 ism/modules/frontend/imdkit/Makefile
                 ism/modules/frontend/Makefile
                 ism/modules/imengine/Makefile
                 ism/modules/filter/Makefile
                 ism/modules/config/Makefile
                 ism/configs/Makefile
                 ism/extras/Makefile
                 ism/extras/gtk2_immodule/Makefile
                 ism/extras/efl_immodule/Makefile
                 ism/extras/efl_panel/Makefile
                 ism/demos/Makefile
                 intltool-extract
                 intltool-merge
                 intltool-update
                 isf.pc
                 scim.pc
                 scim-gtkutils.pc
                 scim.spec])
AC_OUTPUT

AC_MSG_RESULT([
Build options:
  Version                  $ISF_VERSION
  Install prefix           $prefix
  Build shared libs        $enable_shared
  Build static libs        $enable_static
  Enable debug             $enable_debug
  Build for x86 host       $enable_x86
  Enable ld version script $enable_ld_version_script

Module options:
  Simple config module     $enable_config_simple
  Socket config module     $enable_config_socket

  X11 FrontEnd module      $enable_frontend_x11
  Socket FrontEnd module   $enable_frontend_socket

  Socket IMEngine module   $enable_im_socket

  SCTC Filter module       $enable_filter_sctc

  GTK2 Utility Library     $enable_gtk_utils
  X11 Utility Library      $enable_x11_utils

  GTK2 IMModule            $enable_gtk2_immodule
  GTK2 IMModule dir        $GTK_IM_MODULEDIR

  EFL  IMModule            $enable_efl_immodule
  EFL  IMModule dir        $EFL_IM_MODULEDIR
  EFL  Panel GUI           $enable_panel_efl
  EFL  Setting             $enable_setting_efl

  Enable TrayIcon          $enable_tray_icon
  Enable MultiWindow       $enable_multiwindow_support
])

