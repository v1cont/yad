install_subdir('icons',
    install_dir: get_option('datadir') / 'icons' / 'hicolor',
    strip_directory: true,
)

i18n.merge_file(
    input : 'yad-icon-browser.desktop.in',
    output: 'yad-icon-browser.desktop',
    type: 'desktop',
    po_dir: po_dir,
    install: true,
    install_dir: get_option('datadir') / 'applications',
)

i18n.merge_file(
    input : 'yad.gschema.xml.in',
    output: 'yad.gschema.xml',
    type: 'xml',
    po_dir: po_dir,
    install: true,
    install_dir: get_option('datadir') / 'glib-2.0' / 'schemas',
)

install_man('yad.1')
if get_option('tools')
    install_man('yad-tools.1')
endif

configure_file(
    input : 'yad.m4.in',
    output: 'yad.m4',
    configuration: config,
    install: true,
    install_dir: get_option('datadir') / 'aclocal',
)

pkg.generate(
    name: 'yad',
    description: 'Yet Another Dialog',
    variables: [
        'bindir=' + '${prefix}' / get_option('bindir'),
        'yad_bin=' + '${bindir}' / 'yad',
    ],
)
