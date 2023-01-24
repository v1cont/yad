SUBDIRS = icons

desktop_in_files = yad-icon-browser.desktop.in
if BUILD_IB
desktopdir = $(datadir)/applications
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@
endif

dist_man_MANS = yad.1
if BUILD_TOOLS
dist_man_MANS += yad-tools.1
endif

m4dir = $(datadir)/aclocal
m4_DATA = yad.m4

EXTRA_DIST = $(desktop_in_files) misc

DISTCLEANFILES = $(desktop_DATA) $(m4_DATA) yad.spec
