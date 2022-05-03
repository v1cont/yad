AM_CPPFLAGS = -DLOCALEDIR=\""$(datadir)/locale"\"

bin_PROGRAMS = yad

if !STANDALONE
bin_SCRIPTS = yad-settings
endif

CPICKER_SOURCES = cpicker.c cpicker.h

yad_SOURCES = 		\
	about.c			\
	app.c			\
	calendar.c		\
	color.c			\
	dnd.c           \
	entry.c			\
	file.c			\
	font.c			\
	form.c			\
	icons.c			\
	list.c			\
	notebook.c		\
	option.c		\
	paned.c			\
	picture.c       \
	print.c			\
	progress.c		\
	scale.c			\
	text.c			\
	util.c			\
	main.c			\
	yad.h			\
	calendar.xpm	\
	$(CPICKER_SOURCES)

if TRAY
yad_SOURCES += notification.c
endif

if HTML
yad_SOURCES += html.c
endif

if STANDALONE
yad_SOURCES += defaults.h
endif

yad_CFLAGS = $(GTK_CFLAGS) $(HTML_CFLAGS) $(SPELL_CFLAGS) $(SOURCEVIEW_CFLAGS)
yad_LDADD = $(GTK_LIBS) $(HTML_LIBS) $(SPELL_LIBS) $(SOURCEVIEW_LIBS)

if BUILD_TOOLS
bin_PROGRAMS += yad-tools

yad_tools_SOURCES = tools.c	$(CPICKER_SOURCES)
yad_tools_CFLAGS = $(GTK_CFLAGS) $(SPELL_CFLAGS) $(SOURCEVIEW_CFLAGS)
yad_tools_LDADD = $(GTK_LIBS) $(SPELL_LIBS) $(SOURCEVIEW_LIBS)
endif

if BUILD_IB
bin_PROGRAMS += yad-icon-browser

yad_icon_browser_SOURCES = browser.c
yad_icon_browser_CFLAGS = $(GTK_CFLAGS)
yad_icon_browser_LDADD = $(GTK_LIBS)
endif
