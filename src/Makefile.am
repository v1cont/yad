AM_CPPFLAGS =                           \
	-DLOCALEDIR=\""$(datadir)/locale"\"	\
	@DISABLE_DEPRECATED@

bin_PROGRAMS = yad

if GTK2
CPICKER_SOURCES = cpicker_gtk2.c cpicker.h
else
CPICKER_SOURCES = cpicker.c cpicker.h
endif

yad_SOURCES = 			\
	about.c			\
	calendar.c		\
	color.c			\
	dnd.c           	\
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
