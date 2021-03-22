project('yad',
    'c',
    version: '10.90',
    meson_version: '>=0.53.0',
)

cc = meson.get_compiler('c')
i18n = import('i18n')
gnome = import('gnome')
pkg = import('pkgconfig')

###############################################################################
# Project configuration

project_url = 'https://github.com/v1cont/yad'

config = configuration_data()

gettext_package = 'yad'
config.set_quoted('GETTEXT_PACKAGE', gettext_package)

config.set_quoted('PACKAGE_NAME',       meson.project_name())
config.set_quoted('PACKAGE_VERSION',    meson.project_version())
config.set_quoted('VERSION',            meson.project_version())
config.set_quoted('PACKAGE_URL',        project_url)

config.set('ENABLE_NLS', true)
config.set_quoted('LOCALEDIR', get_option('prefix') / get_option('localedir'))

add_project_arguments([
    '-Wno-deprecated-declarations',
    '-Wno-unused-parameter',
], language: 'c')


###############################################################################
# Dependencies

math = cc.find_library('m')

gtk3            = dependency('gtk+-3.0', version: '>=3.22.0')
gtk_unix_print  = dependency('gtk+-unix-print-3.0')

html = dependency('webkit2gtk-4.0', required: get_option('html'))
config.set('HAVE_HTML', html.found())

spell = dependency('gspell-1', required: get_option('spell'))
config.set('HAVE_TRAY', spell.found())

gtksourceview = dependency('gtksourceview-3.0', version: '>=3.18.0', required: get_option('sourceview'))
config.set('HAVE_SOURCEVIEW', gtksourceview.found() ? 1 : false)

###############################################################################

rgb_file = get_option('rgb-file')
config.set_quoted('RGB_FILE', rgb_file)

config.set('HAVE_TRAY', get_option('tray'))

config.set('STANDALONE', get_option('standalone'))


subdir('po')
subdir('src')
subdir('data')

summary({
# echo "Build configuratioh:"
    'Status icon':      get_option('tray'),
    'HTML widget':      html.found(),
    'GtkSourceView':    gtksourceview.found(),
    'Spell checking':   spell.found(),
    'Path to rgb.txt':  rgb_file,
    'Standalone build': get_option('standalone'),
    'Tools':            get_option('tools'),
    'Icon browser':     get_option('icons'),
}, section: 'Build configuration:')
