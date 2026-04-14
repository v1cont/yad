SUBDIRS = icons

desktop_in_files = yad-settings.desktop.in
if BUILD_IB
desktop_in_files += yad-icon-browser.desktop.in
endif

desktopdir = $(datadir)/applications
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)

%.desktop: %.desktop.in
	$(AM_V_GEN)$(MSGFMT) --desktop --template $< -d $(top_srcdir)/po -o $@

dist_man_MANS = yad.1
if BUILD_TOOLS
dist_man_MANS += yad-tools.1
endif

m4dir = $(datadir)/aclocal
m4_DATA = yad.m4

EXTRA_DIST = $(desktop_in_files) misc

DISTCLEANFILES = $(desktop_DATA) $(m4_DATA) yad.spec
