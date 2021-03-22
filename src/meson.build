yad_sources = [
    'about.c',
    'app.c',
    'calendar.c',
    'color.c',
    'dnd.c',
    'entry.c',
    'file.c',
    'font.c',
    'form.c',
    'icons.c',
    'list.c',
    'notebook.c',
    'option.c',
    'paned.c',
    'picture.c',
    'print.c',
    'progress.c',
    'scale.c',
    'text.c',
    'util.c',
    'main.c',
    'yad.h',
]

configure_file(
    output: 'config.h',
    configuration: config,
)

if get_option('tray')
    yad_sources += 'notification.c'
endif
if html.found()
    yad_sources += 'html.c'
endif
if get_option('standalone')
    yad_sources += 'defaults.h'
endif

yad = executable('yad',
    yad_sources,
    dependencies: [
        math,
        gtk3,
        gtk_unix_print,
        gtksourceview,
        html,
    ],
    install: true,
)

if get_option('tools')
    yad_tools = executable('yad-tools',
        'tools.c',
        dependencies: [
            gtk3,
            gtksourceview,
        ],
        install: true,
    )
endif
if get_option('icons')
    yad_icon_browser = executable('yad-icon-browser',
        'browser.c',
        dependencies: [
            gtk3,
        ],
        install: true,
    )
endif



# AM_CPPFLAGS = -DLOCALEDIR=\""$(datadir)/locale"\"
