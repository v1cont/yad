SUBDIRS = icons

desktop_in_files = yad-icon-browser.desktop.in
if BUILD_IB
desktopdir = $(datadir)/applications
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@
endif

gsettings_in_files = yad.gschema.xml.in
gsettings_SCHEMAS = $(gsettings_in_files:.xml.in=.xml)
@INTLTOOL_XML_NOMERGE_RULE@
@GSETTINGS_RULES@

dist_man_MANS = yad.1
if BUILD_TOOLS
dist_man_MANS += yad-tools.1
endif

m4dir = $(datadir)/aclocal
m4_DATA = yad.m4

EXTRA_DIST = $(desktop_in_files) $(gsettings_in_files) zenity.sh notify-send pfd

DISTCLEANFILES = $(desktop_DATA) $(gsettings_SCHEMAS) $(m4_DATA) yad.spec
