AC_INIT([YAD], [14.2], [https://github.com/v1cont/yad/issues/], yad, [https://github.com/v1cont/yad/])
AC_CONFIG_AUX_DIR([build])

AM_INIT_AUTOMAKE([1.11 foreign dist-xz no-dist-gzip])
AM_SILENT_RULES([yes])

AC_PREREQ([2.59])

AC_CONFIG_HEADERS([config.h])
AC_CONFIG_SRCDIR([src])

AM_MAINTAINER_MODE

AC_PROG_CC

AC_PATH_PROG([ENV], [env])

PKG_CHECK_MODULES([GTK], [gtk+-3.0 >= 3.22.0 gtk+-unix-print-3.0])
AC_SUBST([GTK_CFLAGS])
AC_SUBST([GTK_LIBS])

dnl rgb.txt
AC_ARG_WITH([rgb],
	[AS_HELP_STRING([--with-rgb=PATH],
			[set path to rgb.txt file (default - /etc/X11/rgb.txt)])],
	[with_rgb=$withval], [with_rgb=/etc/X11/rgb.txt])
AC_DEFINE_UNQUOTED([RGB_FILE], ["$with_rgb"], [Path to rgb.txt file])

dnl html widget
AC_ARG_ENABLE([html],
	[AS_HELP_STRING([--enable-html],
			[Build YAD with HTML widget])],
	[build_html=$enableval], [build_html=yes])
if test x$build_html = xyes; then
    PKG_CHECK_MODULES([HTML], [webkit2gtk-4.1], [have_html=yes], [have_html=no])
    if test x$have_html=no; then
        PKG_CHECK_MODULES([HTML], [webkit2gtk-4.0], [have_html=yes], [have_html=no])
    fi
else
    have_html=no
fi
AC_SUBST([HTML_CFLAGS])
AC_SUBST([HTML_LIBS])
AM_CONDITIONAL([HTML], [test x$have_html = xyes])

if test x$have_html = xyes; then
    AC_DEFINE([HAVE_HTML], [1], [Define this if you have webkit2gt library])
fi

dnl paned and notebook
AC_ARG_WITH([x11],
    [AS_HELP_STRING([--with-x11],
        [Build YAD modules that require x11 (notebook, paned)])],
    [with_x11=$enableval], [with_x11=yes]
)
if test x$with_html = xyes; then
   PKG_CHECK_MODULES([GTK_X11], [gtk+-x11-3.0], [with_x11=yes], [with_x11=no])
else
    with_x11=no
fi
AM_CONDITIONAL([X11], [test x$with_x11 = xyes])

if test x$with_x11 = xyes; then
   AC_DEFINE([HAVE_X11], [1], [Define this if you are using GTK+3 with X11 backend])
fi

dnl status icon widget
AC_ARG_ENABLE([tray],
	[AS_HELP_STRING([--enable-tray],
			[Build YAD with status icon])],
	[build_tray=$enableval], [build_tray=yes])
AM_CONDITIONAL([TRAY], [test x$build_tray = xyes])

if test x$build_tray = xyes; then
    AC_DEFINE([HAVE_TRAY], [1], [Define this if you need status icon])
fi

dnl spell check
AC_ARG_ENABLE([spell],
	[AS_HELP_STRING([--enable-spell],
			[Build YAD with Spell check support (default - yes)])],
	[with_spell=$enableval], [with_spell=yes])
if test x$with_spell = xyes; then
    PKG_CHECK_MODULES([SPELL], [gspell-1], [have_spell=yes], [have_spell=no])
else
    have_spell=no
fi
AC_SUBST([SPELL_CFLAGS])
AC_SUBST([SPELL_LIBS])

if test x$have_spell = xyes; then
    AC_DEFINE([HAVE_SPELL], [1], [Define this if you need spell check support])
fi

dnl sourceview
AC_ARG_ENABLE([sourceview],
	[AS_HELP_STRING([--enable-sourceview],
			[Build YAD with GtkSourceView support (default - yes)])],
	[with_sourceview=$enableval], [with_sourceview=yes])
if test x$with_sourceview = xyes; then
    PKG_CHECK_MODULES([SOURCEVIEW], [gtksourceview-3.0 >= 3.18.0], [have_sourceview=yes], [have_sourceview=no])
else
    have_sourceview=no
fi
AC_SUBST([SOURCEVIEW_CFLAGS])
AC_SUBST([SOURCEVIEW_LIBS])

if test x$have_sourceview = xyes; then
    AC_DEFINE([HAVE_SOURCEVIEW], [1], [Define this if you need GtkSourceView support])
fi

dnl standalone build
AC_ARG_ENABLE([standalone],
	[AS_HELP_STRING([--enable-standalone],
			[Build standalone YAD binary (without gsettings)])],
	[build_sa=$enableval], [build_sa=no])
AM_CONDITIONAL([STANDALONE], [test x$build_sa = xyes])

if test x$build_sa = xyes; then
    AC_DEFINE([STANDALONE], [1], [Define this if you need standalone YAD binary])
fi

dnl deprecated options
AC_ARG_ENABLE([deprecated],
	[AS_HELP_STRING([--enable-deprecated],
			[Build YAD with deprecated options])],
	[build_depr=$enableval], [build_depr=yes])

if test x$build_depr = xyes; then
    AC_DEFINE([DEPRECATED], [1], [Define this if you need to build YAD with deprecated options])
fi

dnl tools
AC_ARG_ENABLE([tools],
	[AS_HELP_STRING([--enable-tools],
			[Build yad-tools utility])],
	[build_tools=$enableval], [build_tools=yes])
AM_CONDITIONAL([BUILD_TOOLS], [test x$build_tools = xyes])

dnl icon browser
AC_ARG_ENABLE([icon-browser],
	[AS_HELP_STRING([--enable-icon-browser],
			[Build YAD icon browser])],
	[build_ib=$enableval], [build_ib=yes])
AM_CONDITIONAL([BUILD_IB], [test x$build_ib = xyes])

# GSettings
GLIB_GSETTINGS

AC_SEARCH_LIBS(round, m)

# *******************************
# Internationalization
# ******************************* 

GETTEXT_PACKAGE=yad
AC_SUBST([GETTEXT_PACKAGE])
AC_DEFINE_UNQUOTED([GETTEXT_PACKAGE], ["$GETTEXT_PACKAGE"], [Gettext domain])

AM_GNU_GETTEXT_VERSION([0.19.7])
AM_GNU_GETTEXT([external])

# *******************************

AC_SUBST([AM_CFLAGS])
AC_SUBST([AM_CPPFLAGS])
AC_SUBST([AM_LDFLAGS])

# *******************************

AC_CONFIG_LINKS([
src/yad-settings.sh:src/yad-settings.in
])

AC_CONFIG_FILES([
Makefile 
po/Makefile.in
src/Makefile
src/yad-settings
data/Makefile
data/icons/Makefile
data/icons/16x16/Makefile
data/icons/24x24/Makefile
data/icons/32x32/Makefile
data/icons/48x48/Makefile
data/icons/96x96/Makefile
data/icons/128x128/Makefile
data/yad.m4
data/yad.spec
])

AC_OUTPUT

echo
echo "Build configuratioh:"
echo "  Status icon          - $build_tray"
echo "  HTML widget          - $have_html"
echo "  GtkSourceView        - $have_sourceview"
echo "  Spell checking       - $have_spell"
echo "  X11-only utilities   - $with_x11"
echo "  Path to rgb.txt      - $with_rgb"
echo "  Standalone build     - $build_sa"
echo "  Deprecated options   - $build_depr"
echo "  Tools                - $build_tools"
echo "  Icon browser         - $build_ib"
echo
