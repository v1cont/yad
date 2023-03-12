/*
 * This file is part of YAD.
 *
 * YAD is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * YAD is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with YAD. If not, see <http://www.gnu.org/licenses/>.
 *
 * Copyright (C) 2008-2023, Victor Ananjevsky <victor@sanana.kiev.ua>
 */

#include <stdlib.h>

#include "yad.h"

static gboolean add_button (const gchar *, const gchar *, gpointer, GError **);
static gboolean add_column (const gchar *, const gchar *, gpointer, GError **);
static gboolean add_field (const gchar *, const gchar *, gpointer, GError **);
static gboolean add_bar (const gchar *, const gchar *, gpointer, GError **);
static gboolean add_scale_mark (const gchar *, const gchar *, gpointer, GError **);
static gboolean add_palette (const gchar *, const gchar *, gpointer, GError **);
static gboolean add_confirm_overwrite (const gchar *, const gchar *, gpointer, GError **);
static gboolean add_confirm_save (const gchar *, const gchar *, gpointer, GError **);
static gboolean add_file_filter (const gchar *, const gchar *, gpointer, GError **);
static gboolean set_color_mode (const gchar *, const gchar *, gpointer, GError **);
static gboolean set_buttons_layout (const gchar *, const gchar *, gpointer, GError **);
static gboolean set_align (const gchar *, const gchar *, gpointer, GError **);
static gboolean set_justify (const gchar *, const gchar *, gpointer, GError **);
static gboolean set_tab_pos (const gchar *, const gchar *, gpointer, GError **);
static gboolean set_scale_value (const gchar *, const gchar *, gpointer, GError **);
static gboolean set_ellipsize (const gchar *, const gchar *, gpointer, GError **);
static gboolean set_expander (const gchar *, const gchar *, gpointer, GError **);
static gboolean set_orient (const gchar *, const gchar *, gpointer, GError **);
static gboolean set_print_type (const gchar *, const gchar *, gpointer, GError **);
static gboolean set_progress_log (const gchar *, const gchar *, gpointer, GError **);
static gboolean set_size (const gchar *, const gchar *, gpointer, GError **);
static gboolean set_posx (const gchar *, const gchar *, gpointer, GError **);
static gboolean set_posy (const gchar *, const gchar *, gpointer, GError **);
#ifndef G_OS_WIN32
static gboolean set_xid_file (const gchar *, const gchar *, gpointer, GError **);
static gboolean parse_signal (const gchar *, const gchar *, gpointer, GError **);
#endif
static gboolean add_image_path (const gchar *, const gchar *, gpointer, GError **);
static gboolean set_complete_type (const gchar *, const gchar *, gpointer, GError **);
static gboolean set_bool_fmt_type (const gchar *, const gchar *, gpointer, GError **);
static gboolean set_grid_lines (const gchar *, const gchar *, gpointer, GError **);
static gboolean set_scroll_policy (const gchar *, const gchar *, gpointer, GError **);
#if GLIB_CHECK_VERSION(2,30,0)
static gboolean set_size_format (const gchar *, const gchar *, gpointer, GError **);
#endif
static gboolean set_interp (const gchar *, const gchar *, gpointer, GError **);
#ifdef HAVE_SOURCEVIEW
static gboolean set_right_margin (const gchar *, const gchar *, gpointer, GError **);
static gboolean set_smart_homend (const gchar *, const gchar *, gpointer, GError **);
#endif

static gboolean about_mode = FALSE;
static gboolean version_mode = FALSE;
static gboolean app_mode = FALSE;
static gboolean calendar_mode = FALSE;
static gboolean color_mode = FALSE;
static gboolean dnd_mode = FALSE;
static gboolean entry_mode = FALSE;
static gboolean file_mode = FALSE;
static gboolean font_mode = FALSE;
static gboolean form_mode = FALSE;
#ifdef HAVE_HTML
static gboolean html_mode = FALSE;
#endif
static gboolean icons_mode = FALSE;
static gboolean list_mode = FALSE;
static gboolean notebook_mode = FALSE;
#ifdef HAVE_TRAY
static gboolean notification_mode = FALSE;
#endif
static gboolean paned_mode = FALSE;
static gboolean picture_mode = FALSE;
static gboolean print_mode = FALSE;
static gboolean progress_mode = FALSE;
static gboolean scale_mode = FALSE;
static gboolean text_mode = FALSE;

static GOptionEntry general_options[] = {
  { "title", 0, 0, G_OPTION_ARG_STRING, &options.data.dialog_title,
    N_("Set the dialog title"), N_("TITLE") },
  { "window-icon", 0, 0, G_OPTION_ARG_FILENAME, &options.data.window_icon,
    N_("Set the window icon"), N_("ICONPATH") },
  { "width", 0, 0, G_OPTION_ARG_INT, &options.data.width,
    N_("Set the window width"), N_("WIDTH") },
  { "height", 0, 0, G_OPTION_ARG_INT, &options.data.height,
    N_("Set the window height"), N_("HEIGHT") },
  { "posx", 0, 0, G_OPTION_ARG_CALLBACK, set_posx,
    N_("Set the X position of a window"), N_("NUMBER") },
  { "posy", 0, 0, G_OPTION_ARG_CALLBACK, set_posy,
    N_("Set the Y position of a window"), N_("NUMBER") },
  { "geometry", 0, 0, G_OPTION_ARG_STRING, &options.data.geometry,
    N_("Set the window geometry"), N_("WxH+X+Y") },
  { "timeout", 0, 0, G_OPTION_ARG_INT, &options.data.timeout,
    N_("Set dialog timeout in seconds"), N_("TIMEOUT") },
  { "timeout-indicator", 0, 0, G_OPTION_ARG_STRING, &options.data.to_indicator,
    N_("Show remaining time indicator (top, bottom, left, right)"), N_("POS") },
  { "text", 0, G_OPTION_FLAG_NOALIAS, G_OPTION_ARG_STRING, &options.data.dialog_text,
    N_("Set the dialog text"), N_("TEXT") },
  { "text-width", 0, G_OPTION_FLAG_NOALIAS, G_OPTION_ARG_INT, &options.data.text_width,
    N_("Set the dialog text width in characters"), N_("NUMBER") },
  { "text-align", 0, G_OPTION_FLAG_NOALIAS, G_OPTION_ARG_CALLBACK, set_justify,
    N_("Set the dialog text alignment (left, center, right, fill)"), N_("TYPE") },
  { "image", 0, G_OPTION_FLAG_NOALIAS, G_OPTION_ARG_FILENAME, &options.data.dialog_image,
    N_("Set the dialog image"), N_("IMAGE") },
  { "icon-theme", 0, G_OPTION_FLAG_NOALIAS, G_OPTION_ARG_STRING, &options.data.icon_theme,
    N_("Use specified icon theme instead of default"), N_("THEME") },
  { "expander", 0, G_OPTION_FLAG_OPTIONAL_ARG, G_OPTION_ARG_CALLBACK, set_expander,
    N_("Hide main widget with expander"), N_("[TEXT]") },
  { "button", 0, 0, G_OPTION_ARG_CALLBACK, add_button,
    N_("Add dialog button (may be used multiple times)"), N_("NAME:ID") },
  { "no-buttons", 0, 0, G_OPTION_ARG_NONE, &options.data.no_buttons,
    N_("Don't show buttons"), NULL },
  { "buttons-layout", 0, 0, G_OPTION_ARG_CALLBACK, set_buttons_layout,
    N_("Set buttons layout type (spread, edge, start, end or center)"), N_("TYPE") },
  { "no-markup", 0, 0, G_OPTION_ARG_NONE, &options.data.no_markup,
    N_("Don't use pango markup language in dialog's text"), NULL },
  { "no-escape", 0, 0, G_OPTION_ARG_NONE, &options.data.no_escape,
    N_("Don't close dialog if Escape was pressed"), NULL },
  { "escape-ok", 0, 0, G_OPTION_ARG_NONE, &options.data.escape_ok,
    N_("Escape acts like OK button"), NULL },
  { "borders", 0, 0, G_OPTION_ARG_INT, &options.data.borders,
    N_("Set window borders"), N_("NUMBER") },
  { "always-print-result", 0, 0, G_OPTION_ARG_NONE, &options.data.always_print,
    N_("Always print result"), NULL },
  { "response", 0, 0, G_OPTION_ARG_INT, &options.data.def_resp,
    N_("Set default return code"), N_("NUMBER") },
  { "selectable-labels", 0, 0, G_OPTION_ARG_NONE, &options.data.selectable_labels,
    N_("Dialog text can be selected"), NULL },
  { "keep-icon-size", 0, 0, G_OPTION_ARG_NONE, &options.data.keep_icon_size,
    N_("Don't scale icons"), NULL },
  { "use-interp", 0, G_OPTION_FLAG_OPTIONAL_ARG, G_OPTION_ARG_CALLBACK, set_interp,
    N_("Run commands under specified interpreter (default: bash -c '%s')"), N_("CMD") },
  { "uri-handler", 0, 0, G_OPTION_ARG_STRING, &options.data.uri_handler,
    N_("Set URI handler"), N_("CMD") },
  { "f1-action", 0, 0, G_OPTION_ARG_STRING, &options.data.f1_action,
    N_("Set command running when F1 was pressed"), N_("CMD") },
  /* window settings */
  { "sticky", 0, 0, G_OPTION_ARG_NONE, &options.data.sticky,
    N_("Set window sticky"), NULL },
  { "fixed", 0, 0, G_OPTION_ARG_NONE, &options.data.fixed,
    N_("Set window unresizable"), NULL },
  { "on-top", 0, 0, G_OPTION_ARG_NONE, &options.data.ontop,
    N_("Place window on top"), NULL },
  { "center", 0, 0, G_OPTION_ARG_NONE, &options.data.center,
    N_("Place window on center of screen"), NULL },
  { "mouse", 0, 0, G_OPTION_ARG_NONE, &options.data.mouse,
    N_("Place window at the mouse position"), NULL },
  { "undecorated", 0, 0, G_OPTION_ARG_NONE, &options.data.undecorated,
    N_("Set window undecorated"), NULL },
  { "skip-taskbar", 0, 0, G_OPTION_ARG_NONE, &options.data.skip_taskbar,
    N_("Don't show window in taskbar"), NULL },
  { "maximized", 0, 0, G_OPTION_ARG_NONE, &options.data.maximized,
    N_("Set window maximized"), NULL },
  { "fullscreen", 0, 0, G_OPTION_ARG_NONE, &options.data.fullscreen,
    N_("Set window fullscreen"), NULL },
  { "no-focus", 0, G_OPTION_FLAG_REVERSE, G_OPTION_ARG_NONE, &options.data.focus,
    N_("Don't focus dialog window"), NULL },
  { "close-on-unfocus", 0, 0, G_OPTION_ARG_NONE, &options.data.close_on_unfocus,
    N_("Close window when it sets unfocused"), NULL },
  { "splash", 0, 0, G_OPTION_ARG_NONE, &options.data.splash,
    N_("Open window as a splashscreen"), NULL },
  { "plug", 0, 0, G_OPTION_ARG_INT, &options.plug,
    N_("Special type of dialog for XEMBED"), N_("KEY") },
  { "tabnum", 0, 0, G_OPTION_ARG_INT, &options.tabnum,
    N_("Tab number of this dialog"), N_("NUMBER") },
#ifndef G_OS_WIN32
  { "kill-parent", 0, G_OPTION_FLAG_OPTIONAL_ARG, G_OPTION_ARG_CALLBACK, parse_signal,
    N_("Send SIGNAL to parent"), N_("[SIGNAL]") },
  { "print-xid", 0, G_OPTION_FLAG_OPTIONAL_ARG, G_OPTION_ARG_CALLBACK, set_xid_file,
    N_("Print X Window Id to the file/stderr"), N_("[FILENAME]") },
#endif
  { NULL }
};

static GOptionEntry common_options[] = {
  { "date-format", 0, 0, G_OPTION_ARG_STRING, &options.common_data.date_format,
    N_("Set the format for the returned date"), N_("PATTERN") },
  { "float-precision", 0, 0, G_OPTION_ARG_INT, &options.common_data.float_precision,
    N_("Set presicion of floating numbers (default - 3)"), N_("NUMBER") },
  { "command", 0, 0, G_OPTION_ARG_STRING, &options.common_data.command,
    N_("Set command handler"), N_("CMD") },
  { "listen", 0, 0, G_OPTION_ARG_NONE, &options.common_data.listen,
    N_("Listen for data on stdin"), NULL },
  { "separator", 0, 0, G_OPTION_ARG_STRING, &options.common_data.separator,
    N_("Set common separator character"), N_("SEPARATOR") },
  { "item-separator", 0, 0, G_OPTION_ARG_STRING, &options.common_data.item_separator,
    N_("Set item separator character"), N_("SEPARATOR") },
  { "editable", 0, 0, G_OPTION_ARG_NONE, &options.common_data.editable,
    N_("Allow changes to text in some cases"), NULL },
  { "tail", 0, 0, G_OPTION_ARG_NONE, &options.common_data.tail,
    N_("Autoscroll to end of text"), NULL },
  { "auto-scroll", 0, 0, G_OPTION_ARG_NONE, &options.common_data.tail,
    N_("Autoscroll to end of text (alias to tail)"), NULL },
  { "quoted-output", 0, 0, G_OPTION_ARG_NONE, &options.common_data.quoted_output,
    N_("Quote dialogs output"), NULL },
  { "num-output", 0, 0, G_OPTION_ARG_NONE, &options.common_data.num_output,
    N_("Output number instead of text for combo-box"), NULL },
  { "fontname", 0, 0, G_OPTION_ARG_STRING, &options.common_data.font,
    N_("Specify font name to use"), N_("FONTNAME") },
  { "multiple", 0, 0, G_OPTION_ARG_NONE, &options.common_data.multi,
    N_("Allow multiple selection"), NULL },
  { "add-preview", 0, 0, G_OPTION_ARG_NONE, &options.common_data.preview,
    N_("Enable preview"), NULL },
  { "large-preview", 0, 0, G_OPTION_ARG_NONE, &options.common_data.large_preview,
    N_("Use large preview"), NULL },
  { "show-hidden", 0, 0, G_OPTION_ARG_NONE, &options.common_data.show_hidden,
    N_("Show hidden files in file selection dialogs"), NULL },
  { "filename", 0, 0, G_OPTION_ARG_FILENAME, &options.common_data.uri,
    N_("Set source filename"), N_("FILENAME") },
  { "mime", 0, 0, G_OPTION_ARG_STRING, &options.common_data.mime,
    N_("Set mime type of input data"), N_("TYPE") },
  { "vertical", 0, 0, G_OPTION_ARG_NONE, &options.common_data.vertical,
    N_("Set vertical orientation"), NULL },
  { "key", 0, 0, G_OPTION_ARG_INT, &options.common_data.key,
    N_("Identifier of embedded dialogs"), N_("KEY") },
  { "complete", 0, 0, G_OPTION_ARG_CALLBACK, set_complete_type,
    N_("Set extended completion for entries (any, all, or regex)"), N_("TYPE") },
  { "bool-fmt", 0, 0, G_OPTION_ARG_CALLBACK, set_bool_fmt_type,
    N_("Set type of output for boolean values (T, t, Y, y, O, o, 1)"), N_("TYPE") },
  { "scroll", 0, 0, G_OPTION_ARG_NONE, &options.common_data.scroll,
    N_("Make main widget scrollable"), NULL },
  { "disable-search", 0, G_OPTION_FLAG_REVERSE, G_OPTION_ARG_NONE, &options.common_data.enable_search,
    N_("Disable search in text and html dialogs"), NULL },
  { "file-op", 0, 0, G_OPTION_ARG_NONE, &options.common_data.file_op,
    N_("Enable file operations"), NULL },
#if GLIB_CHECK_VERSION(2,30,0)
  { "iec-format", 0, G_OPTION_FLAG_OPTIONAL_ARG, G_OPTION_ARG_CALLBACK, set_size_format,
    N_("Use IEC (base 1024) units with for size values"), NULL },
#endif
#ifdef HAVE_SPELL
  { "enable-spell", 0, 0, G_OPTION_ARG_NONE, &options.common_data.enable_spell,
    N_("Enable spell check for text"), NULL },
  { "spell-lang", 0, 0, G_OPTION_ARG_STRING, &options.common_data.spell_lang,
    N_("Set spell checking language"), N_("LANGUAGE") },
#endif
  { NULL }
};

static GOptionEntry about_options[] = {
  { "about", 0, G_OPTION_FLAG_IN_MAIN, G_OPTION_ARG_NONE, &about_mode,
    N_("Display about dialog"), NULL },
  { "pname", 0, 0, G_OPTION_ARG_STRING, &options.about_data.name,
    N_("Set application name"), N_("STRING") },
  { "pversion", 0, 0, G_OPTION_ARG_STRING, &options.about_data.version,
    N_("Set application version"), N_("STRING") },
  { "copyright", 0, 0, G_OPTION_ARG_STRING, &options.about_data.copyright,
    N_("Set application copyright string"), N_("STRING") },
  { "comments", 0, 0, G_OPTION_ARG_STRING, &options.about_data.comments,
    N_("Set application comments string"), N_("TEXT") },
  { "license", 0, 0, G_OPTION_ARG_STRING, &options.about_data.license,
    N_("Set application license"), N_("TEXT") },
  { "authors", 0, 0, G_OPTION_ARG_STRING, &options.about_data.authors,
    N_("Set application authors"), N_("LIST") },
  { "website", 0, 0, G_OPTION_ARG_STRING, &options.about_data.website,
    N_("Set application website"), N_("URI") },
  { "website-label", 0, 0, G_OPTION_ARG_STRING, &options.about_data.website_lbl,
    N_("Set application website label"), N_("STRING") },
  { NULL }
};

static GOptionEntry app_options[] = {
  { "app", 0, G_OPTION_FLAG_IN_MAIN, G_OPTION_ARG_NONE, &app_mode,
    N_("Display application selection dialog"), NULL },
  { "enable-fallback", 0, 0, G_OPTION_ARG_NONE, &options.app_data.show_fallback,
    N_("Show fallback applications"), NULL },
  { "enable-other", 0, 0, G_OPTION_ARG_NONE, &options.app_data.show_other,
    N_("Show other applications"), NULL },
  { "enable-all", 0, 0, G_OPTION_ARG_NONE, &options.app_data.show_all,
    N_("Show all applications"), NULL },
  { "extended", 0, 0, G_OPTION_ARG_NONE, &options.app_data.extended,
    N_("Output all application data"), NULL },
};

static GOptionEntry calendar_options[] = {
  { "calendar", 0, G_OPTION_FLAG_IN_MAIN, G_OPTION_ARG_NONE, &calendar_mode,
    N_("Display calendar dialog"), NULL },
  { "day", 0, 0, G_OPTION_ARG_INT, &options.calendar_data.day,
    N_("Set the calendar day"), N_("DAY") },
  { "month", 0, 0, G_OPTION_ARG_INT, &options.calendar_data.month,
    N_("Set the calendar month"), N_("MONTH") },
  { "year", 0, 0, G_OPTION_ARG_INT, &options.calendar_data.year,
    N_("Set the calendar year"), N_("YEAR") },
  { "details", 0, 0, G_OPTION_ARG_FILENAME, &options.calendar_data.details,
    N_("Set the filename with dates details"), N_("FILENAME") },
  { "show-weeks", 0, 0, G_OPTION_ARG_NONE, &options.calendar_data.weeks,
    N_("Show week numbers at the left side of calendar"), NULL },
  { NULL }
};

static GOptionEntry color_options[] = {
  { "color", 0, G_OPTION_FLAG_IN_MAIN, G_OPTION_ARG_NONE, &color_mode,
    N_("Display color selection dialog"), NULL },
  { "init-color", 0, 0, G_OPTION_ARG_STRING, &options.color_data.init_color,
    N_("Set initial color value"), N_("COLOR") },
  { "gtk-palette", 0, 0, G_OPTION_ARG_NONE, &options.color_data.gtk_palette,
    N_("Show system palette in color dialog"), NULL },
  { "palette", 0, G_OPTION_FLAG_OPTIONAL_ARG, G_OPTION_ARG_CALLBACK, add_palette,
    N_("Set path to palette file. Default - " RGB_FILE), N_("[FILENAME]") },
  { "expand-palette", 0, 0, G_OPTION_ARG_NONE, &options.color_data.expand_palette,
    N_("Expand user palette"), NULL },
  { "picker", 0, 0, G_OPTION_ARG_NONE, &options.color_data.color_picker,
    N_("Show color picker"), NULL },
  { "mode", 0, 0, G_OPTION_ARG_CALLBACK, set_color_mode,
    N_("Set output mode to MODE. Values are hex (default) or rgb"), N_("MODE") },
  { "alpha", 0, 0, G_OPTION_ARG_NONE, &options.color_data.alpha,
    N_("Add opacity to output color value"), NULL },
  { NULL }
};

static GOptionEntry dnd_options[] = {
  { "dnd", 0, G_OPTION_FLAG_IN_MAIN, G_OPTION_ARG_NONE, &dnd_mode,
    N_("Display drag-n-drop box"), NULL },
  { "tooltip", 0, 0, G_OPTION_ARG_NONE, &options.dnd_data.tooltip,
    N_("Use dialog text as tooltip"), NULL },
  { "exit-on-drop", 0, 0, G_OPTION_ARG_INT, &options.dnd_data.exit_on_drop,
    N_("Exit after NUMBER of drops"), N_("NUMBER") },
  { NULL }
};

static GOptionEntry entry_options[] = {
  { "entry", 0, G_OPTION_FLAG_IN_MAIN, G_OPTION_ARG_NONE, &entry_mode,
    N_("Display text entry or combo-box dialog"), NULL },
  { "entry-label", 0, 0, G_OPTION_ARG_STRING, &options.entry_data.entry_label,
    N_("Set the entry label"), N_("TEXT") },
  { "entry-text", 0, 0, G_OPTION_ARG_STRING, &options.entry_data.entry_text,
    N_("Set the entry text"), N_("TEXT") },
  { "hide-text", 0, G_OPTION_FLAG_NOALIAS, G_OPTION_ARG_NONE, &options.common_data.hide_text,
    N_("Hide the entry text"), NULL },
  { "completion", 0, 0, G_OPTION_ARG_NONE, &options.entry_data.completion,
    N_("Use completion instead of combo-box"), NULL },
  { "numeric", 0, 0, G_OPTION_ARG_NONE, &options.entry_data.numeric,
    N_("Use spin button for text entry"), NULL },
  { "licon", 0, 0, G_OPTION_ARG_FILENAME, &options.entry_data.licon,
    N_("Set the left entry icon"), N_("IMAGE") },
  { "licon-action", 0, 0, G_OPTION_ARG_STRING, &options.entry_data.licon_action,
    N_("Set the left entry icon action"), N_("CMD") },
  { "ricon", 0, 0, G_OPTION_ARG_FILENAME, &options.entry_data.ricon,
    N_("Set the right entry icon"), N_("IMAGE") },
  { "ricon-action", 0, 0, G_OPTION_ARG_STRING, &options.entry_data.ricon_action,
    N_("Set the right entry icon action"), N_("CMD") },
  { NULL }
};

static GOptionEntry file_options[] = {
  { "file", 0, G_OPTION_FLAG_IN_MAIN, G_OPTION_ARG_NONE, &file_mode,
    N_("Display file selection dialog"), NULL },
  { "directory", 0, 0, G_OPTION_ARG_NONE, &options.file_data.directory,
    N_("Activate directory-only selection"), NULL },
  { "save", 0, 0, G_OPTION_ARG_NONE, &options.file_data.save,
    N_("Activate save mode"), NULL },
  { "confirm-overwrite", 0, G_OPTION_FLAG_OPTIONAL_ARG, G_OPTION_ARG_CALLBACK, add_confirm_overwrite,
    N_("Confirm file selection if filename already exists"), N_("[TEXT]") },
  { NULL }
};

static GOptionEntry font_options[] = {
  { "font", 0, G_OPTION_FLAG_IN_MAIN, G_OPTION_ARG_NONE, &font_mode,
    N_("Display font selection dialog"), NULL },
  { "preview", 0, 0, G_OPTION_ARG_STRING, &options.font_data.preview,
    N_("Set text string for preview"), N_("TEXT") },
  { "separate-output", 0, 0, G_OPTION_ARG_NONE, &options.font_data.separate_output,
    N_("Separate output of font description"), NULL },
  { NULL }
};

static GOptionEntry form_options[] = {
  { "form", 0, G_OPTION_FLAG_IN_MAIN, G_OPTION_ARG_NONE, &form_mode,
    N_("Display form dialog"), NULL },
  { "field", 0, 0, G_OPTION_ARG_CALLBACK, add_field,
    N_ ("Add field to form (see man page for list of possible types)"), N_("LABEL[:TYPE]") },
  { "align", 0, G_OPTION_FLAG_NOALIAS, G_OPTION_ARG_CALLBACK, set_align,
    N_("Set alignment of filed labels (left, center or right)"), N_("TYPE") },
  { "columns", 0, 0, G_OPTION_ARG_INT, &options.form_data.columns,
    N_("Set number of columns in form"), N_("NUMBER") },
  { "homogeneous", 0, 0, G_OPTION_ARG_NONE, &options.form_data.homogeneous,
    N_("Make form fields height and columns width the same size"), NULL },
  { "output-by-row", 0, 0, G_OPTION_ARG_NONE, &options.form_data.output_by_row,
    N_("Order output fields by rows"), NULL },
  { "focus-field", 0, 0, G_OPTION_ARG_INT, &options.form_data.focus_field,
    N_("Set focused field"), N_("NUMBER") },
  { "cycle-read", 0, 0, G_OPTION_ARG_NONE, &options.form_data.cycle_read,
    N_("Cycled reading of stdin data"), NULL },
  { "align-buttons", 0, 0, G_OPTION_ARG_NONE, &options.form_data.align_buttons,
    N_("Align labels on button fields"), NULL },
  { "changed-action", 0, 0, G_OPTION_ARG_STRING, &options.form_data.changed_action,
    N_("Set changed action"), N_("CMD") },
  { NULL }
};

#ifdef HAVE_HTML
static GOptionEntry html_options[] = {
  { "html", 0, G_OPTION_FLAG_IN_MAIN, G_OPTION_ARG_NONE, &html_mode,
    N_("Display HTML dialog"), NULL },
  { "uri", 0, 0, G_OPTION_ARG_STRING, &options.html_data.uri,
    N_("Open specified location"), "URI" },
  { "browser", 0, 0, G_OPTION_ARG_NONE, &options.html_data.browser,
    N_("Turn on browser mode"), NULL },
  { "print-uri", 0, 0, G_OPTION_ARG_NONE, &options.html_data.print_uri,
    N_("Print clicked uri to stdout"), NULL },
  { "encoding", 0, 0, G_OPTION_ARG_STRING, &options.html_data.encoding,
    N_("Set encoding of input stream data"), N_("ENCODING") },
  { "user-agent", 0, 0, G_OPTION_ARG_STRING, &options.html_data.user_agent,
    N_("Set user agent string"), N_("STRING") },
  { "user-style", 0, 0, G_OPTION_ARG_STRING, &options.html_data.user_style,
    N_("Set custom user style"), N_("STRING") },
  { "wk-prop", 0, 0, G_OPTION_ARG_STRING_ARRAY, &options.html_data.wk_props,
    N_("Set WebKit property"), N_("PROP") },
  { NULL }
};
#endif

static GOptionEntry icons_options[] = {
  { "icons", 0, G_OPTION_FLAG_IN_MAIN, G_OPTION_ARG_NONE, &icons_mode,
    N_("Display icons box dialog"), NULL },
  { "read-dir", 0, 0, G_OPTION_ARG_STRING, &options.icons_data.directory,
    N_("Read data from .desktop files in specified directory"), N_("DIR") },
  { "compact", 0, 0, G_OPTION_ARG_NONE, &options.icons_data.compact,
    N_("Use compact (list) view"), NULL },
  { "generic", 0, 0, G_OPTION_ARG_NONE, &options.icons_data.generic,
    N_("Use GenericName field instead of Name for icon label"), NULL },
  { "item-width", 0, 0, G_OPTION_ARG_INT, &options.icons_data.width,
    N_("Set the width of dialog items"), NULL },
  { "icon-size", 0,  G_OPTION_FLAG_NOALIAS, G_OPTION_ARG_INT, &options.common_data.icon_size,
    N_("Force using specified icon size"), N_("SIZE") },
  { "term", 0, 0, G_OPTION_ARG_STRING, &options.icons_data.term,
    /* xgettext: no-c-format */
    N_("Use specified pattern for launch command in terminal (default: xterm -e %s)"), N_("PATTERN") },
  { "sort-by-name", 0, 0, G_OPTION_ARG_NONE, &options.icons_data.sort_by_name,
    N_("Sort items by name instead of filename"), NULL },
  { "descend", 0, 0, G_OPTION_ARG_NONE, &options.icons_data.descend,
    N_("Sort items in descending order"), NULL },
  { "single-click", 0, 0, G_OPTION_ARG_NONE, &options.icons_data.single_click,
    N_("Activate items by single click"), NULL },
  { "monitor", 0, 0, G_OPTION_ARG_NONE, &options.icons_data.monitor,
    N_("Watch fot changes in directory"), NULL },
  { NULL }
};

static GOptionEntry list_options[] = {
  { "list", 0, G_OPTION_FLAG_IN_MAIN, G_OPTION_ARG_NONE, &list_mode,
    N_("Display list dialog"), NULL },
  { "column", 0, 0, G_OPTION_ARG_CALLBACK, add_column,
    N_("Set the column header (see man page for list of possible types)"), N_("COLUMN[:TYPE]") },
  { "tree", 0, 0, G_OPTION_ARG_NONE, &options.list_data.tree_mode,
    N_("Enbale tree mode"), NULL },
  { "checklist", 0, 0, G_OPTION_ARG_NONE, &options.list_data.checkbox,
    N_("Use checkboxes for first column"), NULL },
  { "radiolist", 0, 0, G_OPTION_ARG_NONE, &options.list_data.radiobox,
    N_("Use radioboxes for first column"), NULL },
  { "no-headers", 0, 0, G_OPTION_ARG_NONE, &options.list_data.no_headers,
    N_("Don't show column headers"), NULL },
  { "no-click", 0, G_OPTION_FLAG_REVERSE, G_OPTION_ARG_NONE, &options.list_data.clickable,
    N_("Disable clickable column headers"), NULL },
  { "grid-lines", 0, 0, G_OPTION_ARG_CALLBACK, set_grid_lines,
    N_("Set grid lines (hor[izontal], vert[ical] or both)"), N_("TYPE") },
  { "print-all", 0, 0, G_OPTION_ARG_NONE, &options.list_data.print_all,
    N_("Print all data from list"), NULL },
  { "editable-cols", 0, 0, G_OPTION_ARG_STRING, &options.list_data.editable_cols,
    N_("Set the list of editable columns"), N_("LIST") },
  { "wrap-width", 0, 0, G_OPTION_ARG_INT, &options.list_data.wrap_width,
    N_("Set the width of a column for start wrapping text"), N_("NUMBER") },
  { "wrap-cols", 0, 0, G_OPTION_ARG_STRING, &options.list_data.wrap_cols,
    N_("Set the list of wrapped columns"), N_("LIST") },
  { "ellipsize", 0, 0, G_OPTION_ARG_CALLBACK, set_ellipsize,
    N_("Set ellipsize mode for text columns (none, start, middle or end)"), N_("TYPE") },
  { "ellipsize-cols", 0, 0, G_OPTION_ARG_STRING, &options.list_data.ellipsize_cols,
    N_("Set the list of ellipsized columns"), N_("LIST") },
  { "print-column", 0, 0, G_OPTION_ARG_INT, &options.list_data.print_column,
    N_("Print a specific column. By default or if 0 is specified will be printed all columns"), N_("NUMBER") },
  { "hide-column", 0, 0, G_OPTION_ARG_INT, &options.list_data.hide_column,
    N_("Hide a specific column"), N_("NUMBER") },
  { "expand-column", 0, 0, G_OPTION_ARG_INT, &options.list_data.expand_column,
    N_("Set the column expandable by default. 0 sets all columns expandable"), N_("NUMBER") },
  { "search-column", 0, 0, G_OPTION_ARG_INT, &options.list_data.search_column,
    N_("Set the quick search column. Default is first column. Set it to 0 for disable searching"), N_("NUMBER") },
  { "tooltip-column", 0, 0, G_OPTION_ARG_INT, &options.list_data.tooltip_column,
    N_("Set the tooltip column"), N_("NUMBER") },
  { "sep-column", 0, 0, G_OPTION_ARG_INT, &options.list_data.sep_column,
    N_("Set the row separator column"), N_("NUMBER") },
  { "sep-value", 0, 0, G_OPTION_ARG_STRING, &options.list_data.sep_value,
    N_("Set the row separator value"), N_("TEXT") },
  { "limit", 0, 0, G_OPTION_ARG_INT, &options.list_data.limit,
    N_("Set the limit of rows in list"), N_("NUMBER") },
  { "dclick-action", 0, 0, G_OPTION_ARG_STRING, &options.list_data.dclick_action,
    N_("Set double-click action"), N_("CMD") },
  { "select-action", 0, 0, G_OPTION_ARG_STRING, &options.list_data.select_action,
    N_("Set select action"), N_("CMD") },
  { "row-action", 0, 0, G_OPTION_ARG_STRING, &options.list_data.row_action,
    N_("Set row action"), N_("CMD") },
  { "tree-expanded", 0, 0, G_OPTION_ARG_NONE, &options.list_data.tree_expanded,
    N_("Expand all tree nodes"), NULL },
  { "regex-search", 0, 0, G_OPTION_ARG_NONE, &options.list_data.regex_search,
    N_("Use regex in search"), NULL },
  { "no-selection", 0, 0, G_OPTION_ARG_NONE, &options.list_data.no_selection,
    N_("Disable selection"), NULL },
  { "add-on-top", 0, 0, G_OPTION_ARG_NONE, &options.list_data.add_on_top,
    N_("Add new records on the top of a list"), NULL },
  { "simple-tips", 0, 0, G_OPTION_ARG_NONE, &options.list_data.simple_tips,
    N_("Don't use markup in tooltips"), NULL },
  { "header-tips", 0, 0, G_OPTION_ARG_NONE, &options.list_data.header_tips,
    N_("Use column name as a header tooltip"), NULL },
  { "column-align", 0, 0, G_OPTION_ARG_STRING, &options.list_data.col_align,
    N_("Set columns alignment"), N_("STRING") },
  { "header-align", 0, 0, G_OPTION_ARG_STRING, &options.list_data.hdr_align,
    N_("Set headers alignment"), N_("STRING") },
  { NULL }
};

static GOptionEntry notebook_options[] = {
  { "notebook", 0, G_OPTION_FLAG_IN_MAIN, G_OPTION_ARG_NONE, &notebook_mode,
    N_("Display notebook dialog"), NULL },
  { "tab", 0, 0, G_OPTION_ARG_STRING_ARRAY, &options.notebook_data.tabs,
    N_("Add a tab to notebook"), N_("LABEL") },
  { "tab-pos", 0, G_OPTION_FLAG_NOALIAS, G_OPTION_ARG_CALLBACK, set_tab_pos,
    N_("Set position of a notebook tabs (top, bottom, left or right)"), N_("TYPE") },
  { "tab-borders", 0, 0, G_OPTION_ARG_INT, &options.notebook_data.borders,
    N_("Set tab borders"), N_("NUMBER") },
  { "active-tab", 0, 0, G_OPTION_ARG_INT, &options.notebook_data.active,
    N_("Set active tab"), N_("NUMBER") },
  { "expand", 0, 0, G_OPTION_ARG_NONE, &options.notebook_data.expand,
    N_("Expand tabs"), NULL },
  { "stack", 0, 0, G_OPTION_ARG_NONE, &options.notebook_data.stack,
    N_("Use stack mode"), NULL },
  { NULL }
};

#ifdef HAVE_TRAY
static GOptionEntry notification_options[] = {
  { "notification", 0, G_OPTION_FLAG_IN_MAIN, G_OPTION_ARG_NONE, &notification_mode,
    N_("Display notification"), NULL },
  { "menu", 0, 0, G_OPTION_ARG_STRING, &options.notification_data.menu,
    N_("Set initial popup menu"), N_("STRING") },
  { "no-middle", 0, G_OPTION_FLAG_REVERSE, G_OPTION_ARG_NONE, &options.notification_data.middle,
    N_("Disable exit on middle click"), NULL },
  { "hidden", 0, 0, G_OPTION_ARG_NONE, &options.notification_data.hidden,
    N_("Doesn't show icon at startup"), NULL },
  { "icon-size", 0,  G_OPTION_FLAG_NOALIAS, G_OPTION_ARG_INT, &options.common_data.icon_size,
    N_("Set icon size for fully specified icons (default - 16)"), N_("SIZE") },
  { NULL }
};
#endif

static GOptionEntry paned_options[] = {
  { "paned", 0, G_OPTION_FLAG_IN_MAIN, G_OPTION_ARG_NONE, &paned_mode,
    N_("Display paned dialog"), NULL },
  { "orient", 0, 0, G_OPTION_ARG_CALLBACK, set_orient,
    N_("Set orientation (hor[izontal] or vert[ical])"), N_("TYPE") },
  { "splitter", 0, 0, G_OPTION_ARG_INT, &options.paned_data.splitter,
    N_("Set initial splitter position"), N_("POS") },
  { "focused", 0, 0, G_OPTION_ARG_INT, &options.paned_data.focused,
    N_("Set focused pane (1 or 2)"), N_("PANE") },
  { NULL }
};

static GOptionEntry picture_options[] = {
  { "picture", 0, G_OPTION_FLAG_IN_MAIN, G_OPTION_ARG_NONE, &picture_mode,
    N_("Display picture dialog"), NULL },
  { "size", 0, 0, G_OPTION_ARG_CALLBACK, set_size,
    N_("Set initial size (fit or orig)"), N_("TYPE") },
  { "inc", 0, 0, G_OPTION_ARG_INT, &options.picture_data.inc,
    N_("Set increment for picture scaling (default - 5)"), N_("NUMBER") },
  { "image-changed", 0, 0, G_OPTION_ARG_STRING, &options.picture_data.change_cmd,
    N_("Set action on image changing"), N_("CMD") },
  { NULL }
};

static GOptionEntry print_options[] = {
  { "print", 0, G_OPTION_FLAG_IN_MAIN, G_OPTION_ARG_NONE, &print_mode,
    N_("Display printing dialog"), NULL },
  { "type", 0, 0, G_OPTION_ARG_CALLBACK, set_print_type,
    N_("Set source type (text, image or raw)"), N_("TYPE") },
  { "headers", 0, 0, G_OPTION_ARG_NONE, &options.print_data.headers,
    N_("Add headers to page"), NULL },
  { NULL }
};

static GOptionEntry progress_options[] = {
  { "progress", 0, G_OPTION_FLAG_IN_MAIN, G_OPTION_ARG_NONE, &progress_mode,
    N_("Display progress indication dialog"), NULL },
  { "bar", 0, 0, G_OPTION_ARG_CALLBACK, add_bar,
    N_("Add the progress bar (norm, rtl, pulse or perm)"), N_("LABEL[:TYPE]") },
  { "watch-bar", 0, 0, G_OPTION_ARG_INT, &options.progress_data.watch_bar,
    N_("Watch for specific bar for auto close"), N_("NUMBER") },
  { "align", 0, G_OPTION_FLAG_NOALIAS, G_OPTION_ARG_CALLBACK, set_align,
    N_("Set alignment of bar labels (left, center or right)"), N_("TYPE") },
  { "progress-text", 0, 0, G_OPTION_ARG_STRING, &options.progress_data.progress_text,
    N_("Set progress text"), N_("TEXT") },
  { "hide-text", 0, G_OPTION_FLAG_NOALIAS, G_OPTION_ARG_NONE, &options.common_data.hide_text,
    N_("Hide text on progress bar"), NULL },
  { "pulsate", 0, 0, G_OPTION_ARG_NONE, &options.progress_data.pulsate,
    N_("Pulsate progress bar"), NULL },
  { "auto-close", 0, G_OPTION_FLAG_NOALIAS, G_OPTION_ARG_NONE, &options.progress_data.autoclose,
    /* xgettext: no-c-format */
    N_("Dismiss the dialog when 100% has been reached"), NULL },
#ifndef G_OS_WIN32
  { "auto-kill", 0, G_OPTION_FLAG_NOALIAS, G_OPTION_ARG_NONE, &options.progress_data.autokill,
    N_("Kill parent process if cancel button is pressed"), NULL },
#endif
  { "rtl", 0, 0, G_OPTION_ARG_NONE, &options.progress_data.rtl,
    N_("Right-To-Left progress bar direction"), NULL },
  { "enable-log", 0, G_OPTION_FLAG_OPTIONAL_ARG, G_OPTION_ARG_CALLBACK, set_progress_log,
    N_("Show log window"), N_("[TEXT]") },
  { "log-expanded", 0, 0, G_OPTION_ARG_NONE, &options.progress_data.log_expanded,
    N_("Expand log window"), NULL },
  { "log-on-top", 0, 0, G_OPTION_ARG_NONE, &options.progress_data.log_on_top,
    N_("Place log window above progress bar"), NULL },
  { "log-height", 0, 0, G_OPTION_ARG_INT, &options.progress_data.log_height,
    N_("Height of log window"), NULL },
  { NULL }
};

static GOptionEntry scale_options[] = {
  { "scale", 0, G_OPTION_FLAG_IN_MAIN, G_OPTION_ARG_NONE, &scale_mode,
    N_("Display scale dialog"), NULL },
  { "value", 0, 0, G_OPTION_ARG_CALLBACK, set_scale_value,
    N_("Set initial value"), N_("VALUE") },
  { "min-value", 0, 0, G_OPTION_ARG_INT, &options.scale_data.min_value,
    N_("Set minimum value"), N_("VALUE") },
  { "max-value", 0, 0, G_OPTION_ARG_INT, &options.scale_data.max_value,
    N_("Set maximum value"), N_("VALUE") },
  { "step", 0, 0, G_OPTION_ARG_INT, &options.scale_data.step,
    N_("Set step size"), N_("VALUE") },
  { "enforce-step", 0, 0, G_OPTION_ARG_NONE,  &options.scale_data.enforce_step,
    N_("Only allow values in step increments"), NULL },
  { "page", 0, 0, G_OPTION_ARG_INT, &options.scale_data.page,
    N_("Set paging size"), N_("VALUE") },
  { "print-partial", 0, 0, G_OPTION_ARG_NONE, &options.scale_data.print_partial,
    N_("Print partial values"), NULL },
  { "hide-value", 0, 0, G_OPTION_ARG_NONE, &options.scale_data.hide_value,
    N_("Hide value"), NULL },
  { "invert", 0, 0, G_OPTION_ARG_NONE, &options.scale_data.invert,
    N_("Invert direction"), NULL },
  { "inc-buttons", 0, 0, G_OPTION_ARG_NONE, &options.scale_data.buttons,
    N_("Show +/- buttons in scale"), NULL },
  { "mark", 0, 0, G_OPTION_ARG_CALLBACK, add_scale_mark,
    N_("Add mark to scale (may be used multiple times)"), N_("NAME:VALUE") },
  { NULL }
};

static GOptionEntry text_options[] = {
  { "text-info", 0, G_OPTION_FLAG_IN_MAIN, G_OPTION_ARG_NONE, &text_mode,
    N_("Display text information dialog"), NULL },
  { "wrap", 0, 0, G_OPTION_ARG_NONE, &options.text_data.wrap,
    N_("Enable text wrapping"), NULL },
  { "justify", 0, 0, G_OPTION_ARG_CALLBACK, set_justify,
    N_("Set justification (left, right, center or fill)"), N_("TYPE") },
  { "margins", 0, 0, G_OPTION_ARG_INT, &options.text_data.margins,
    N_("Set text margins"), N_("SIZE") },
  { "line", 0, 0, G_OPTION_ARG_INT64, &options.text_data.line,
    N_("Jump to specific line"), N_("NUMBER") },
  { "fore", 0, 0, G_OPTION_ARG_STRING, &options.text_data.fore,
    N_("Use specified color for text"), N_("COLOR") },
  { "back", 0, 0, G_OPTION_ARG_STRING, &options.text_data.back,
    N_("Use specified color for background"), N_("COLOR") },
  { "show-cursor", 0, G_OPTION_FLAG_REVERSE, G_OPTION_ARG_NONE, &options.text_data.hide_cursor,
    N_("Show cursor in read-only mode"), NULL },
  { "show-uri", 0, 0, G_OPTION_ARG_NONE, &options.text_data.uri,
    N_("Make URI clickable"), NULL },
  { "uri-color", 0, 0, G_OPTION_ARG_STRING, &options.text_data.uri_color,
    N_("Use specified color for links"), N_("COLOR") },
  { "formatted", 0, 0, G_OPTION_ARG_NONE, &options.text_data.formatted,
    N_("Use pango markup"), NULL },
  { "in-place", 0, 0, G_OPTION_ARG_NONE, &options.text_data.in_place,
    N_("Save file instead of print on exit"), NULL },
  { "confirm-save", 0, G_OPTION_FLAG_OPTIONAL_ARG, G_OPTION_ARG_CALLBACK, add_confirm_save,
    N_("Confirm save the file if text was changed"), N_("[TEXT]") },
  { NULL }
};

#ifdef HAVE_SOURCEVIEW
static GOptionEntry source_options[] = {
  { "lang", 0, 0, G_OPTION_ARG_STRING, &options.source_data.lang,
    N_("Use specified langauge for syntax highlighting"), N_("LANG") },
  { "theme", 0, 0, G_OPTION_ARG_STRING, &options.source_data.theme,
    N_("Use specified theme"), N_("THEME") },
  { "line-num", 0, 0, G_OPTION_ARG_NONE, &options.source_data.line_num,
    N_("Show line numbers"), NULL },
  { "line-hl", 0, 0, G_OPTION_ARG_NONE, &options.source_data.line_hl,
    N_("Highlight current line"), NULL },
  { "line-marks", 0, 0, G_OPTION_ARG_NONE, &options.source_data.line_marks,
    N_("Enable line marks"), NULL },
  { "mark1-color", 0, 0, G_OPTION_ARG_STRING, &options.source_data.m1_color,
    N_("Set color for first type marks"), N_("COLOR") },
  { "mark2-color", 0, 0, G_OPTION_ARG_STRING, &options.source_data.m1_color,
    N_("Set color for second type marks"), N_("COLOR") },
  { "right-margin", 0, G_OPTION_FLAG_OPTIONAL_ARG, G_OPTION_ARG_CALLBACK, set_right_margin,
    N_("Set position of right margin"), N_("[POS]") },
  { "brackets", 0, 0, G_OPTION_ARG_NONE, &options.source_data.brackets,
    N_("Highlight matching brackets"), NULL },
  { "indent", 0, 0, G_OPTION_ARG_NONE, &options.source_data.indent,
    N_("Use autoindent"), NULL },
  { "tab-width", 0, 0, G_OPTION_ARG_INT, &options.source_data.tab_width,
    N_("Set tabulation width"), N_("VALUE") },
  { "indent-width", 0, 0, G_OPTION_ARG_INT, &options.source_data.indent_width,
    N_("Set indentation width"), N_("VALUE") },
  { "smart-he", 0, 0, G_OPTION_ARG_CALLBACK, set_smart_homend,
    N_("Set smart Home/End behavior"), N_("TYPE") },
  { "smart-bs", 0, 0, G_OPTION_ARG_NONE, &options.source_data.smart_bs,
    N_("Enable smart backspace"), NULL },
  { "spaces", 0, 0, G_OPTION_ARG_NONE, &options.source_data.spaces,
    N_("Insert spaces instead of tabs"), NULL },
  { NULL }
};
#endif

static GOptionEntry filter_options[] = {
  { "file-filter", 0, 0, G_OPTION_ARG_CALLBACK, add_file_filter,
    N_("Sets a filename filter"), N_("NAME | PATTERN1 PATTERN2 ...") },
  { "mime-filter", 0, 0, G_OPTION_ARG_CALLBACK, add_file_filter,
    N_("Sets a mime-type filter"), N_("NAME | MIME1 MIME2 ...") },
  { "image-filter", 0, G_OPTION_FLAG_OPTIONAL_ARG, G_OPTION_ARG_CALLBACK, add_file_filter,
    N_("Add filter for images"), N_("[NAME]") },
  { NULL }
};

static GOptionEntry misc_options[] = {
  { "version", 0, 0, G_OPTION_ARG_NONE, &version_mode,
    N_("Print version"), NULL },
  { "css", 0, 0, G_OPTION_ARG_STRING, &options.css,
    N_("Load additional CSS settings from file or string"), N_("STRING") },
  { "gtkrc", 0, 0, G_OPTION_ARG_FILENAME, &options.gtkrc_file,
    N_("Load additional CSS settings from file"), N_("FILENAME") },
  { "hscroll-policy", 0, 0, G_OPTION_ARG_CALLBACK, set_scroll_policy,
    N_("Set policy for horizontal scrollbars (auto, always, never)"), N_("TYPE") },
  { "vscroll-policy", 0, 0, G_OPTION_ARG_CALLBACK, set_scroll_policy,
    N_("Set policy for vertical scrollbars (auto, always, never)"), N_("TYPE") },
  { "image-path", 0, 0, G_OPTION_ARG_CALLBACK, add_image_path,
    N_("Add path for search icons by name"), N_("PATH") },
  { NULL }
};

static GOptionEntry rest_options[] = {
  { "rest", 0, 0, G_OPTION_ARG_FILENAME, &options.rest_file,
    N_("Load extra arguments from file"), N_("FILENAME") },
  {G_OPTION_REMAINING, 0, 0, G_OPTION_ARG_STRING_ARRAY, &options.extra_data,
   NULL, NULL },
  { NULL }
};

static gboolean
add_button (const gchar * option_name, const gchar * value, gpointer data, GError ** err)
{
  YadButton *btn;
  gchar **bstr = split_arg (value);

  btn = g_new0 (YadButton, 1);
  btn->name = g_strdup (bstr[0]);
  if (bstr[1])
    {
      if (bstr[1][0] >= '0' && bstr[1][0] <= '9')
        btn->response = g_ascii_strtoll (bstr[1], NULL, 10);
      else
        btn->cmd = g_strdup (bstr[1]);
    }
  else
    btn->response = g_slist_length (options.data.buttons);
  options.data.buttons = g_slist_append (options.data.buttons, btn);

  g_strfreev (bstr);
  return TRUE;
}

static gboolean
add_column (const gchar * option_name, const gchar * value, gpointer data, GError ** err)
{
  YadColumn *col;
  gchar **cstr = split_arg (value);

  col = g_new0 (YadColumn, 1);
  col->name = g_strdup (cstr[0]);
  if (strcasecmp (cstr[0], "@fore@") == 0)
    col->type = YAD_COLUMN_ATTR_FORE;
  else if (strcasecmp (cstr[0], "@back@") == 0)
    col->type = YAD_COLUMN_ATTR_BACK;
  else if (strcasecmp (cstr[0], "@font@") == 0)
    col->type = YAD_COLUMN_ATTR_FONT;
  else
    {
      if (cstr[1])
        {
          if (strcasecmp (cstr[1], "NUM") == 0)
            col->type = YAD_COLUMN_NUM;
          else if (strcasecmp (cstr[1], "CHK") == 0)
            col->type = YAD_COLUMN_CHECK;
          else if (strcasecmp (cstr[1], "RD") == 0)
            col->type = YAD_COLUMN_RADIO;
          else if (strcasecmp (cstr[1], "FLT") == 0)
            col->type = YAD_COLUMN_FLOAT;
          else if (strcasecmp (cstr[1], "IMG") == 0)
            col->type = YAD_COLUMN_IMAGE;
          else if (strcasecmp (cstr[1], "HD") == 0)
            col->type = YAD_COLUMN_HIDDEN;
          else if (strcasecmp (cstr[1], "BAR") == 0)
            col->type = YAD_COLUMN_BAR;
          else if (strcasecmp (cstr[1], "SZ") == 0)
            col->type = YAD_COLUMN_SIZE;
          else if (strcasecmp (cstr[1], "TIP") == 0)
            col->type = YAD_COLUMN_TIP;
          else
            col->type = YAD_COLUMN_TEXT;
        }
      else
        col->type = YAD_COLUMN_TEXT;
    }
  options.list_data.columns = g_slist_append (options.list_data.columns, col);

  g_strfreev (cstr);
  return TRUE;
}

static gboolean
add_field (const gchar * option_name, const gchar * value, gpointer data, GError ** err)
{
  YadField *fld;
  gchar **fstr = split_arg (value);

  fld = g_new0 (YadField, 1);

  if (*fstr[0])
    {
      gchar **names = g_strsplit (fstr[0], options.common_data.item_separator, 3);
      if (names[1] && names[2])
        {
          fld->name = g_strdup_printf ("%s%s%s", names[0], options.common_data.item_separator, names[1]);
          fld->tip = g_strdup (names[2]);
        }
      else
        {
          fld->name = g_strdup (names[0]);
          if (names[1])
            fld->tip = g_strdup (names[1]);
        }
      g_strfreev (names);
    }
  else
    fld->name = g_strdup ("");

  if (fstr[1])
    {
      if (strcasecmp (fstr[1], "H") == 0)
        fld->type = YAD_FIELD_HIDDEN;
      else if (strcasecmp (fstr[1], "RO") == 0)
        fld->type = YAD_FIELD_READ_ONLY;
      else if (strcasecmp (fstr[1], "NUM") == 0)
        fld->type = YAD_FIELD_NUM;
      else if (strcasecmp (fstr[1], "CHK") == 0)
        fld->type = YAD_FIELD_CHECK;
      else if (strcasecmp (fstr[1], "CB") == 0)
        fld->type = YAD_FIELD_COMBO;
      else if (strcasecmp (fstr[1], "CBE") == 0)
        fld->type = YAD_FIELD_COMBO_ENTRY;
      else if (strcasecmp (fstr[1], "CE") == 0)
        fld->type = YAD_FIELD_COMPLETE;
      else if (strcasecmp (fstr[1], "FL") == 0)
        fld->type = YAD_FIELD_FILE;
      else if (strcasecmp (fstr[1], "SFL") == 0)
        fld->type = YAD_FIELD_FILE_SAVE;
      else if (strcasecmp (fstr[1], "DIR") == 0)
        fld->type = YAD_FIELD_DIR;
      else if (strcasecmp (fstr[1], "CDIR") == 0)
        fld->type = YAD_FIELD_DIR_CREATE;
      else if (strcasecmp (fstr[1], "FN") == 0)
        fld->type = YAD_FIELD_FONT;
      else if (strcasecmp (fstr[1], "APP") == 0)
        fld->type = YAD_FIELD_APP;
      else if (strcasecmp (fstr[1], "ICON") == 0)
        fld->type = YAD_FIELD_ICON;
      else if (strcasecmp (fstr[1], "CLR") == 0)
        fld->type = YAD_FIELD_COLOR;
      else if (strcasecmp (fstr[1], "MFL") == 0)
        fld->type = YAD_FIELD_MFILE;
      else if (strcasecmp (fstr[1], "MDIR") == 0)
        fld->type = YAD_FIELD_MDIR;
      else if (strcasecmp (fstr[1], "DT") == 0)
        fld->type = YAD_FIELD_DATE;
      else if (strcasecmp (fstr[1], "SCL") == 0)
        fld->type = YAD_FIELD_SCALE;
      else if (strcasecmp (fstr[1], "SW") == 0)
        fld->type = YAD_FIELD_SWITCH;
      else if (strcasecmp (fstr[1], "BTN") == 0)
        fld->type = YAD_FIELD_BUTTON;
      else if (strcasecmp (fstr[1], "FBTN") == 0)
        fld->type = YAD_FIELD_FULL_BUTTON;
      else if (strcasecmp (fstr[1], "LINK") == 0)
        fld->type = YAD_FIELD_LINK;
      else if (strcasecmp (fstr[1], "LBL") == 0)
        fld->type = YAD_FIELD_LABEL;
      else if (strcasecmp (fstr[1], "TXT") == 0)
        fld->type = YAD_FIELD_TEXT;
      else
        fld->type = YAD_FIELD_SIMPLE;
    }
  else
    fld->type = YAD_FIELD_SIMPLE;
  options.form_data.fields = g_slist_append (options.form_data.fields, fld);

  g_strfreev (fstr);
  return TRUE;
}

static gboolean
add_bar (const gchar * option_name, const gchar * value, gpointer data, GError ** err)
{
  YadProgressBar *bar;
  gchar **bstr = split_arg (value);

  bar = g_new0 (YadProgressBar, 1);
  bar->name = g_strdup (bstr[0]);
  if (bstr[1])
    {
      if (strcasecmp (bstr[1], "RTL") == 0)
        bar->type = YAD_PROGRESS_RTL;
      else if (strcasecmp (bstr[1], "PULSE") == 0)
        bar->type = YAD_PROGRESS_PULSE;
      else if (strcasecmp (bstr[1], "PERM") == 0)
        bar->type = YAD_PROGRESS_PERM;
      else
        bar->type = YAD_PROGRESS_NORMAL;
    }
  else
    bar->type = YAD_PROGRESS_NORMAL;
  options.progress_data.bars = g_slist_append (options.progress_data.bars, bar);

  g_strfreev (bstr);
  return TRUE;
}

static gboolean
add_scale_mark (const gchar * option_name, const gchar * value, gpointer data, GError ** err)
{
  YadScaleMark *mark;
  gchar **mstr = split_arg (value);

  mark = g_new0 (YadScaleMark, 1);
  mark->name = g_strdup (mstr[0]);
  if (mstr[1])
    {
      mark->value = g_ascii_strtoll (mstr[1], NULL, 10);
      options.scale_data.marks = g_slist_append (options.scale_data.marks, mark);
    }
  else
    {
      g_printerr (_("Mark %s doesn't have a value\n"), mark->name);
      g_free (mark->name);
      g_free (mark);
    }

  g_strfreev (mstr);
  return TRUE;
}

static gboolean
add_palette (const gchar * option_name, const gchar * value, gpointer data, GError ** err)
{
  options.color_data.use_palette = TRUE;
  if (value)
    options.color_data.palette = g_strdup (value);

  return TRUE;
}

static gboolean
add_confirm_overwrite (const gchar * option_name, const gchar * value, gpointer data, GError ** err)
{
  options.file_data.confirm_overwrite = TRUE;
  if (value)
    options.file_data.confirm_text = g_strdup (value);

  return TRUE;
}

static gboolean
add_confirm_save (const gchar * option_name, const gchar * value, gpointer data, GError ** err)
{
  options.text_data.confirm_save = TRUE;
  if (value)
    options.text_data.confirm_text = g_strdup (value);

  return TRUE;
}

static gboolean
add_file_filter (const gchar * option_name, const gchar * value, gpointer data, GError ** err)
{
  GtkFileFilter *filter = gtk_file_filter_new ();

  /* add image filter */
  if (strcmp (option_name, "--image-filter") == 0)
    {
      gtk_file_filter_set_name (filter, value ? value : _("Images"));
      gtk_file_filter_add_pixbuf_formats (filter);
      options.common_data.filters = g_list_append (options.common_data.filters, filter);
    }
  else
    {
      gint i;
      gchar **pattern, **patterns;
      gchar *name = NULL;
      gboolean is_mime = (strcmp (option_name, "--mime-filter") == 0);

      /* Set name */
      for (i = 0; value[i] != '\0'; i++)
        {
          if (value[i] == '|')
            break;
        }

      if (value[i] == '|')
        name = g_strstrip (g_strndup (value, i));

      if (name)
        {
          gtk_file_filter_set_name (filter, name);

          /* Point i to the right position for split */
          for (++i; value[i] == ' '; i++);
        }
      else
        {
          gtk_file_filter_set_name (filter, value);
          i = 0;
        }

      /* Get patterns */
      patterns = g_strsplit_set (value + i, " ", -1);

      if (is_mime)
        {
          for (pattern = patterns; *pattern; pattern++)
            gtk_file_filter_add_mime_type (filter, *pattern);
        }
      else
        {
          for (pattern = patterns; *pattern; pattern++)
            gtk_file_filter_add_pattern (filter, *pattern);
        }

      g_free (name);
      g_strfreev (patterns);

      options.common_data.filters = g_list_append (options.common_data.filters, filter);
    }

  return TRUE;
}

static gboolean
set_color_mode (const gchar * option_name, const gchar * value, gpointer data, GError ** err)
{
  if (strcasecmp (value, "hex") == 0)
    options.color_data.mode = YAD_COLOR_HEX;
  else if (strcasecmp (value, "rgb") == 0)
    options.color_data.mode = YAD_COLOR_RGB;
  else
    g_printerr (_("Unknown color mode: %s\n"), value);

  return TRUE;
}

static gboolean
set_buttons_layout (const gchar * option_name, const gchar * value, gpointer data, GError ** err)
{
  if (strcasecmp (value, "spread") == 0)
    options.data.buttons_layout = GTK_BUTTONBOX_SPREAD;
  else if (strcasecmp (value, "edge") == 0)
    options.data.buttons_layout = GTK_BUTTONBOX_EDGE;
  else if (strcasecmp (value, "start") == 0)
    options.data.buttons_layout = GTK_BUTTONBOX_START;
  else if (strcasecmp (value, "end") == 0)
    options.data.buttons_layout = GTK_BUTTONBOX_END;
  else if (strcasecmp (value, "center") == 0)
    options.data.buttons_layout = GTK_BUTTONBOX_CENTER;
  else
    g_printerr (_("Unknown buttons layout type: %s\n"), value);

  return TRUE;
}

static gboolean
set_align (const gchar * option_name, const gchar * value, gpointer data, GError ** err)
{
  if (strcasecmp (value, "left") == 0)
    options.common_data.align = 0.0;
  else if (strcasecmp (value, "right") == 0)
    options.common_data.align = 1.0;
  else if (strcasecmp (value, "center") == 0)
    options.common_data.align = 0.5;
  else
    g_printerr (_("Unknown align type: %s\n"), value);

  return TRUE;
}

static gboolean
set_justify (const gchar * option_name, const gchar * value, gpointer data, GError ** err)
{
  GtkJustification *just;

  if (strcasecmp (option_name, "justify") == 0)
    just = &options.text_data.justify;
  else
    just = &options.data.text_align;

  if (strcasecmp (value, "left") == 0)
    *just = GTK_JUSTIFY_LEFT;
  else if (strcasecmp (value, "right") == 0)
    *just = GTK_JUSTIFY_RIGHT;
  else if (strcasecmp (value, "center") == 0)
    *just = GTK_JUSTIFY_CENTER;
  else if (strcasecmp (value, "fill") == 0)
    *just = GTK_JUSTIFY_FILL;
  else
    g_printerr (_("Unknown justification type: %s\n"), value);

  return TRUE;
}

static gboolean
set_tab_pos (const gchar * option_name, const gchar * value, gpointer data, GError ** err)
{
  if (strcasecmp (value, "top") == 0)
    options.notebook_data.pos = GTK_POS_TOP;
  else if (strcasecmp (value, "bottom") == 0)
    options.notebook_data.pos = GTK_POS_BOTTOM;
  else if (strcasecmp (value, "left") == 0)
    options.notebook_data.pos = GTK_POS_LEFT;
  else if (strcasecmp (value, "right") == 0)
    options.notebook_data.pos = GTK_POS_RIGHT;
  else
    g_printerr (_("Unknown tab position type: %s\n"), value);

  return TRUE;
}

static gboolean
set_expander (const gchar * option_name, const gchar * value, gpointer data, GError ** err)
{
  if (value)
    options.data.expander = g_strdup (value);
  else
    options.data.expander = "";

  return TRUE;
}

static gboolean
set_scale_value (const gchar * option_name, const gchar * value, gpointer data, GError ** err)
{
  options.scale_data.value = atoi (value);
  options.scale_data.have_value = TRUE;

  return TRUE;
}

static gboolean
set_ellipsize (const gchar * option_name, const gchar * value, gpointer data, GError ** err)
{
  if (strcasecmp (value, "none") == 0)
    options.list_data.ellipsize = PANGO_ELLIPSIZE_NONE;
  else if (strcasecmp (value, "start") == 0)
    options.list_data.ellipsize = PANGO_ELLIPSIZE_START;
  else if (strcasecmp (value, "middle") == 0)
    options.list_data.ellipsize = PANGO_ELLIPSIZE_MIDDLE;
  else if (strcasecmp (value, "end") == 0)
    options.list_data.ellipsize = PANGO_ELLIPSIZE_END;
  else
    g_printerr (_("Unknown ellipsize type: %s\n"), value);

  return TRUE;
}

static gboolean
set_orient (const gchar * option_name, const gchar * value, gpointer data, GError ** err)
{
  if (strncasecmp (value, "hor", 3) == 0)
    options.paned_data.orient = GTK_ORIENTATION_HORIZONTAL;
  else if (strncasecmp (value, "vert", 4) == 0)
    options.paned_data.orient = GTK_ORIENTATION_VERTICAL;
  else
    g_printerr (_("Unknown orientation: %s\n"), value);

  return TRUE;
}

static gboolean
set_print_type (const gchar * option_name, const gchar * value, gpointer data, GError ** err)
{
  if (strcasecmp (value, "text") == 0)
    options.print_data.type = YAD_PRINT_TEXT;
  else if (strcasecmp (value, "image") == 0)
    options.print_data.type = YAD_PRINT_IMAGE;
  else if (strcasecmp (value, "raw") == 0)
    options.print_data.type = YAD_PRINT_RAW;
  else
    g_printerr (_("Unknown source type: %s\n"), value);

  return TRUE;
}

static gboolean
set_progress_log (const gchar * option_name, const gchar * value, gpointer data, GError ** err)
{
  if (value)
    options.progress_data.log = g_strdup (value);
  else
    options.progress_data.log = _("Progress log");

  return TRUE;
}

static gboolean
set_size (const gchar * option_name, const gchar * value, gpointer data, GError ** err)
{
  if (strcasecmp (value, "fit") == 0)
    options.picture_data.size = YAD_PICTURE_FIT;
  else if (strcasecmp (value, "orig") == 0)
    options.picture_data.size = YAD_PICTURE_ORIG;
  else
    g_printerr (_("Unknown size type: %s\n"), value);

  return TRUE;
}

static gboolean
set_posx (const gchar * option_name, const gchar * value, gpointer data, GError ** err)
{
  options.data.use_posx = TRUE;
  options.data.posx = atol (value);
  options.data.negx = (*value == '-');

  return TRUE;
}

static gboolean
set_posy (const gchar * option_name, const gchar * value, gpointer data, GError ** err)
{
  options.data.use_posy = TRUE;
  options.data.posy = atol (value);
  options.data.negy = (*value == '-');

  return TRUE;
}

static gboolean
add_image_path (const gchar * option_name, const gchar * value, gpointer data, GError ** err)
{
  if (value)
    gtk_icon_theme_append_search_path (yad_icon_theme, value);

  return TRUE;
}

static gboolean
set_complete_type (const gchar * option_name, const gchar * value, gpointer data, GError ** err)
{
  if (strcasecmp (value, "any") == 0)
    options.common_data.complete = YAD_COMPLETE_ANY;
  else if (strcasecmp (value, "all") == 0)
    options.common_data.complete = YAD_COMPLETE_ALL;
  else if (strcasecmp (value, "regex") == 0)
    options.common_data.complete = YAD_COMPLETE_REGEX;
  else
    g_printerr (_("Unknown completion type: %s\n"), value);

  return TRUE;
}

static gboolean
set_bool_fmt_type (const gchar * option_name, const gchar * value, gpointer data, GError ** err)
{
  switch (value[0])
    {
    case 'T':
      options.common_data.bool_fmt = YAD_BOOL_FMT_UT;
      break;
    case 't':
      options.common_data.bool_fmt = YAD_BOOL_FMT_LT;
      break;
    case 'Y':
      options.common_data.bool_fmt = YAD_BOOL_FMT_UY;
      break;
    case 'y':
      options.common_data.bool_fmt = YAD_BOOL_FMT_LY;
      break;
    case 'O':
      options.common_data.bool_fmt = YAD_BOOL_FMT_UO;
      break;
    case 'o':
      options.common_data.bool_fmt = YAD_BOOL_FMT_LO;
      break;
    case '1':
      options.common_data.bool_fmt = YAD_BOOL_FMT_1;
      break;
    default:
      g_printerr (_("Unknown boolean format type: %s\n"), value);
    }

  return TRUE;
}

static gboolean
set_grid_lines (const gchar * option_name, const gchar * value, gpointer data, GError ** err)
{
  if (strncasecmp (value, "hor", 3) == 0)
    options.list_data.grid_lines = GTK_TREE_VIEW_GRID_LINES_HORIZONTAL;
  else if (strncasecmp (value, "vert", 4) == 0)
    options.list_data.grid_lines = GTK_TREE_VIEW_GRID_LINES_VERTICAL;
  else if (strncasecmp (value, "both", 4) == 0)
    options.list_data.grid_lines = GTK_TREE_VIEW_GRID_LINES_BOTH;
  else
    g_printerr (_("Unknown grid lines type: %s\n"), value);

  return TRUE;
}

static gboolean
set_scroll_policy (const gchar * option_name, const gchar * value, gpointer data, GError ** err)
{
  GtkPolicyType pt = GTK_POLICY_AUTOMATIC;

  if (strcmp (value, "auto") == 0)
    pt = GTK_POLICY_AUTOMATIC;
  else if (strcmp (value, "always") == 0)
    pt = GTK_POLICY_ALWAYS;
  else if (strcmp (value, "never") == 0)
    pt = GTK_POLICY_NEVER;
  else
    g_printerr (_("Unknown scrollbar policy type: %s\n"), value);

  if (strcmp (option_name, "--hscroll-policy") == 0)
    options.data.hscroll_policy = pt;
  else
    options.data.vscroll_policy = pt;

  return TRUE;
}

#if GLIB_CHECK_VERSION(2,30,0)
static gboolean
set_size_format (const gchar * option_name, const gchar * value, gpointer data, GError ** err)
{
  options.common_data.size_fmt = G_FORMAT_SIZE_IEC_UNITS;

  return TRUE;
}
#endif

static gboolean
set_interp (const gchar * option_name, const gchar * value, gpointer data, GError ** err)
{
  options.data.use_interp = TRUE;

  if (value)
    options.data.interp = g_strdup (value);

  return TRUE;
}

#if HAVE_SOURCEVIEW
static gboolean
set_right_margin (const gchar * option_name, const gchar * value, gpointer data, GError ** err)
{
  if (value)
    options.source_data.right_margin = atoi (value);
  else
    options.source_data.right_margin = RIGHT_MARGIN;
}

static gboolean
set_smart_homend (const gchar * option_name, const gchar * value, gpointer data, GError ** err)
{
  if (strcasecmp (value, "never") == 0)
    options.source_data.smart_he = GTK_SOURCE_SMART_HOME_END_DISABLED;
  else if (strcasecmp (value, "before") == 0)
    options.source_data.smart_he = GTK_SOURCE_SMART_HOME_END_BEFORE;
  else if (strcasecmp (value, "after") == 0)
    options.source_data.smart_he = GTK_SOURCE_SMART_HOME_END_AFTER;
  else if (strcasecmp (value, "always") == 0)
    options.source_data.smart_he = GTK_SOURCE_SMART_HOME_END_ALWAYS;
  else
    g_printerr (_("Unknown smart Home/End mode: %s\n"), value);

  return TRUE;
}
#endif

#ifndef G_OS_WIN32
static gboolean
set_xid_file (const gchar * option_name, const gchar * value, gpointer data, GError ** err)
{
  options.print_xid = TRUE;
  if (value && value[0])
    options.xid_file = g_strdup (value);

  return TRUE;
}

static gboolean
parse_signal (const gchar * option_name, const gchar * value, gpointer data, GError ** err)
{
  guint sn = 0;

  if (!value || !value[0])
    {
      options.kill_parent = SIGTERM;
      return TRUE;
    }

  sn = (guint) strtol (value, NULL, 0);
  if (!sn)
    {
      guint ofst = 0;

      if (strncmp (value, "SIG", 3) == 0)
        ofst = 3;

      /* signals names from bits/sn.h */
      if (strcmp (value + ofst, "HUP") == 0)
        sn = SIGHUP;
      else if (strcmp (value + ofst, "INT") == 0)
        sn = SIGINT;
      else if (strcmp (value + ofst, "QUIT") == 0)
        sn = SIGQUIT;
      else if (strcmp (value + ofst, "ILL") == 0)
        sn = SIGILL;
      else if (strcmp (value + ofst, "TRAP") == 0)
        sn = SIGTRAP;
      else if (strcmp (value + ofst, "ABRT") == 0)
        sn = SIGABRT;
      else if (strcmp (value + ofst, "IOT") == 0)
        sn = SIGIOT;
      else if (strcmp (value + ofst, "BUS") == 0)
        sn = SIGBUS;
      else if (strcmp (value + ofst, "FPE") == 0)
        sn = SIGFPE;
      else if (strcmp (value + ofst, "KILL") == 0)
        sn = SIGKILL;
      else if (strcmp (value + ofst, "USR1") == 0)
        sn = SIGUSR1;
      else if (strcmp (value + ofst, "SEGV") == 0)
        sn = SIGSEGV;
      else if (strcmp (value + ofst, "USR2") == 0)
        sn = SIGUSR2;
      else if (strcmp (value + ofst, "PIPE") == 0)
        sn = SIGPIPE;
      else if (strcmp (value + ofst, "ALRM") == 0)
        sn = SIGPIPE;
      else if (strcmp (value + ofst, "TERM") == 0)
        sn = SIGTERM;
#ifdef SIGSTKFLT
      else if (strcmp (value + ofst, "STKFLT") == 0)
        sn = SIGSTKFLT;
#endif
      else if (strcmp (value + ofst, "CHLD") == 0 || strcmp (value + ofst, "CLD") == 0)
        sn = SIGCHLD;
      else if (strcmp (value + ofst, "CONT") == 0)
        sn = SIGCONT;
      else if (strcmp (value + ofst, "STOP") == 0)
        sn = SIGSTOP;
      else if (strcmp (value + ofst, "TSTP") == 0)
        sn = SIGTSTP;
      else if (strcmp (value + ofst, "TTIN") == 0)
        sn = SIGTTIN;
      else if (strcmp (value + ofst, "TTOU") == 0)
        sn = SIGTTOU;
      else if (strcmp (value + ofst, "URG") == 0)
        sn = SIGURG;
      else if (strcmp (value + ofst, "XCPU") == 0)
        sn = SIGXCPU;
      else if (strcmp (value + ofst, "XFSZ") == 0)
        sn = SIGXFSZ;
      else if (strcmp (value + ofst, "VTALRM") == 0)
        sn = SIGVTALRM;
      else if (strcmp (value + ofst, "PROF") == 0)
        sn = SIGPROF;
      else if (strcmp (value + ofst, "WINCH") == 0)
        sn = SIGWINCH;
      else if (strcmp (value + ofst, "IO") == 0 || strcmp (value + ofst, "POLL") == 0)
        sn = SIGIO;
#ifdef SIGPWR
      else if (strcmp (value + ofst, "PWR") == 0)
        sn = SIGPWR;
#endif
      else if (strcmp (value + ofst, "SYS") == 0)
        sn = SIGSYS;
    }

  if (sn > 0 && sn < NSIG)
    options.kill_parent = sn;
  else
    g_printerr (_("Unknown signal: %s\n"), value);

  return TRUE;
}
#endif

void
yad_set_mode (void)
{
  if (app_mode)
    options.mode = YAD_MODE_APP;
  else if (calendar_mode)
    options.mode = YAD_MODE_CALENDAR;
  else if (color_mode)
    options.mode = YAD_MODE_COLOR;
  else if (dnd_mode)
    options.mode = YAD_MODE_DND;
  else if (entry_mode)
    options.mode = YAD_MODE_ENTRY;
  else if (file_mode)
    options.mode = YAD_MODE_FILE;
  else if (font_mode)
    options.mode = YAD_MODE_FONT;
  else if (form_mode)
    options.mode = YAD_MODE_FORM;
#ifdef HAVE_HTML
  else if (html_mode)
    options.mode = YAD_MODE_HTML;
#endif
  else if (icons_mode)
    options.mode = YAD_MODE_ICONS;
  else if (list_mode)
    options.mode = YAD_MODE_LIST;
  else if (notebook_mode)
    options.mode = YAD_MODE_NOTEBOOK;
#ifdef HAVE_TRAY
  else if (notification_mode)
    options.mode = YAD_MODE_NOTIFICATION;
#endif
  else if (paned_mode)
    options.mode = YAD_MODE_PANED;
  else if (picture_mode)
    options.mode = YAD_MODE_PICTURE;
  else if (print_mode)
    options.mode = YAD_MODE_PRINT;
  else if (progress_mode)
    options.mode = YAD_MODE_PROGRESS;
  else if (scale_mode)
    options.mode = YAD_MODE_SCALE;
  else if (text_mode)
    options.mode = YAD_MODE_TEXTINFO;
  else if (about_mode)
    options.mode = YAD_MODE_ABOUT;
  else if (version_mode)
    options.mode = YAD_MODE_VERSION;
}

void
yad_options_init (void)
{
  /* Set default mode */
  options.mode = YAD_MODE_MESSAGE;
  options.rest_file = NULL;
  options.extra_data = NULL;
  options.css = NULL;
  options.gtkrc_file = NULL;
#ifndef G_OS_WIN32
  options.kill_parent = 0;
  options.print_xid = FALSE;
  options.xid_file = NULL;
#endif

#ifndef STANDALONE
  options.debug = g_settings_get_boolean (settings, "debug");
#else
  options.debug = FALSE;
#endif

  /* plug settings */
  options.plug = -1;
  options.tabnum = 0;

  /* Initialize general data */
  options.data.dialog_title = NULL;
  options.data.window_icon = NULL;
#ifndef STANDALONE
  options.data.width = g_settings_get_int (settings, "width");
  options.data.height = g_settings_get_int (settings, "height");
#else
  options.data.width = options.data.height = -1;
#endif
  options.data.use_posx = FALSE;
  options.data.posx = 0;
  options.data.negx = FALSE;
  options.data.use_posy = FALSE;
  options.data.posy = 0;
  options.data.negy = FALSE;
  options.data.geometry = NULL;
  options.data.dialog_text = NULL;
  options.data.text_width = 0;
  options.data.text_align = GTK_JUSTIFY_LEFT;
  options.data.dialog_image = NULL;
  options.data.icon_theme = NULL;
  options.data.expander = NULL;
  options.data.timeout = 0;
  options.data.to_indicator = NULL;
  options.data.hscroll_policy = GTK_POLICY_AUTOMATIC;
  options.data.vscroll_policy = GTK_POLICY_AUTOMATIC;
  options.data.buttons = NULL;
  options.data.no_buttons = FALSE;
  options.data.buttons_layout = GTK_BUTTONBOX_END;
#ifndef STANDALONE
  options.data.borders = g_settings_get_int (settings, "border");
#else
  options.data.borders = BORDERS;
#endif
  options.data.no_markup = FALSE;
  options.data.no_escape = FALSE;
  options.data.escape_ok = FALSE;
  options.data.always_print = FALSE;
  options.data.selectable_labels = FALSE;
  options.data.keep_icon_size = FALSE;
  options.data.def_resp = YAD_RESPONSE_OK;
  options.data.use_interp = FALSE;
  options.data.interp = "bash -c \"%s\"";
#ifndef STANDALONE
  options.data.uri_handler = g_settings_get_string (settings, "open-command");
#else
  options.data.uri_handler = OPEN_CMD;
#endif
  options.data.f1_action = NULL;

  /* Initialize window options */
  options.data.sticky = FALSE;
  options.data.fixed = FALSE;
  options.data.ontop = FALSE;
  options.data.center = FALSE;
  options.data.mouse = FALSE;
  options.data.undecorated = FALSE;
  options.data.skip_taskbar = FALSE;
  options.data.maximized = FALSE;
  options.data.fullscreen = FALSE;
  options.data.splash = FALSE;
  options.data.focus = TRUE;
  options.data.close_on_unfocus = FALSE;

  /* Initialize common data */
  options.common_data.uri = NULL;
  options.common_data.mime = NULL;
  options.common_data.font = NULL;
  options.common_data.separator = "|";
  options.common_data.item_separator = "!";
  options.common_data.multi = FALSE;
  options.common_data.editable = FALSE;
  options.common_data.tail = FALSE;
  options.common_data.command = NULL;
#ifndef STANDALONE
  options.common_data.date_format = g_settings_get_string (settings, "date-format");
#else
  options.common_data.date_format = DATE_FMT;
#endif
  options.common_data.float_precision = 3;
  options.common_data.vertical = FALSE;
  options.common_data.align = 0.0;
  options.common_data.listen = FALSE;
  options.common_data.preview = FALSE;
#ifndef STANDALONE
  options.common_data.large_preview = g_settings_get_boolean (settings, "large-preview");
#else
  options.common_data.large_preview = FALSE;
#endif
  options.common_data.show_hidden = FALSE;
  options.common_data.quoted_output = FALSE;
  options.common_data.num_output = FALSE;
  options.common_data.hide_text = FALSE;
  options.common_data.filters = NULL;
  options.common_data.key = -1;
  options.common_data.bool_fmt = YAD_BOOL_FMT_UT;
  options.common_data.complete = YAD_COMPLETE_SIMPLE;
  options.common_data.icon_size = 0;
  options.common_data.scroll = FALSE;
  options.common_data.enable_search = TRUE;
  options.common_data.file_op = FALSE;
#if GLIB_CHECK_VERSION(2,30,0)
  options.common_data.size_fmt = G_FORMAT_SIZE_DEFAULT;
#endif
#ifdef HAVE_SPELL
  options.common_data.enable_spell = FALSE;
  options.common_data.spell_lang = NULL;
#endif

  /* initialize about data */
  options.about_data.name = NULL;
  options.about_data.copyright = NULL;
  options.about_data.comments = NULL;
  options.about_data.license = NULL;
  options.about_data.authors = NULL;
  options.about_data.website = NULL;
  options.about_data.website_lbl = NULL;

  /* Initialize application data */
  options.app_data.show_fallback = FALSE;
  options.app_data.show_other = FALSE;
  options.app_data.show_all = FALSE;
  options.app_data.extended = FALSE;

  /* Initialize calendar data */
  options.calendar_data.day = -1;
  options.calendar_data.month = -1;
  options.calendar_data.year = -1;
  options.calendar_data.details = NULL;
  options.calendar_data.weeks = FALSE;

  /* Initialize color data */
  options.color_data.init_color = NULL;
  options.color_data.gtk_palette = FALSE;
  options.color_data.use_palette = FALSE;
  options.color_data.color_picker = FALSE;
  options.color_data.expand_palette = FALSE;
  options.color_data.palette = NULL;
  options.color_data.alpha = FALSE;
  options.color_data.mode = YAD_COLOR_HEX;

  /* Initialize DND data */
  options.dnd_data.tooltip = FALSE;
  options.dnd_data.exit_on_drop = 0;

  /* Initialize entry data */
  options.entry_data.entry_text = NULL;
  options.entry_data.entry_label = NULL;
  options.entry_data.completion = FALSE;
  options.entry_data.numeric = FALSE;
  options.entry_data.licon = NULL;
  options.entry_data.licon_action = NULL;
  options.entry_data.ricon = NULL;
  options.entry_data.ricon_action = NULL;

  /* Initialize file data */
  options.file_data.directory = FALSE;
  options.file_data.save = FALSE;
  options.file_data.confirm_overwrite = FALSE;
  options.file_data.confirm_text = _("File exist. Overwrite?");
  options.file_data.file_filt = NULL;
  options.file_data.mime_filt = NULL;
  options.file_data.image_filt = NULL;

  /* Initialize font data */
  options.font_data.preview = NULL;
  options.font_data.separate_output = FALSE;

  /* Initialize form data */
  options.form_data.fields = NULL;
  options.form_data.columns = 1;
  options.form_data.output_by_row = FALSE;
  options.form_data.focus_field = 1;
  options.form_data.cycle_read = FALSE;
  options.form_data.align_buttons = FALSE;
  options.form_data.changed_action = NULL;
  options.form_data.homogeneous = FALSE;

#ifdef HAVE_HTML
  /* Initialize html data */
  options.html_data.uri = NULL;
  options.html_data.browser = FALSE;
  options.html_data.print_uri = FALSE;
  options.html_data.encoding = NULL;
  options.html_data.user_agent = "YAD-Webkit (" VERSION ")";
  options.html_data.user_style = NULL;
  options.html_data.wk_props = NULL;
#endif

  /* Initialize icons data */
  options.icons_data.directory = NULL;
  options.icons_data.compact = FALSE;
  options.icons_data.generic = FALSE;
  options.icons_data.width = -1;
#ifndef STANDALONE
  options.icons_data.term = g_settings_get_string (settings, "terminal");
#else
  options.icons_data.term = TERM_CMD;
#endif
  options.icons_data.sort_by_name = FALSE;
  options.icons_data.descend = FALSE;
  options.icons_data.single_click = FALSE;
  options.icons_data.monitor = FALSE;

  /* Initialize list data */
  options.list_data.columns = NULL;
  options.list_data.tree_mode = FALSE;
  options.list_data.checkbox = FALSE;
  options.list_data.radiobox = FALSE;
  options.list_data.no_headers = FALSE;
  options.list_data.print_all = FALSE;
  options.list_data.grid_lines = GTK_TREE_VIEW_GRID_LINES_NONE;
  options.list_data.print_column = 0;
  options.list_data.hide_column = 0;
  options.list_data.expand_column = -1; // must be -1 for disable expand by default (keep the original behavior)
  options.list_data.search_column = 0;
  options.list_data.tooltip_column = 0;
  options.list_data.sep_column = 0;
  options.list_data.sep_value = NULL;
  options.list_data.limit = 0;
  options.list_data.editable_cols = NULL;
  options.list_data.wrap_width = 0;
  options.list_data.wrap_cols = NULL;
  options.list_data.ellipsize = PANGO_ELLIPSIZE_NONE;
  options.list_data.ellipsize_cols = NULL;
  options.list_data.dclick_action = NULL;
  options.list_data.select_action = NULL;
  options.list_data.row_action = NULL;
  options.list_data.tree_expanded = FALSE;
  options.list_data.regex_search = FALSE;
  options.list_data.clickable = TRUE;
  options.list_data.no_selection = FALSE;
  options.list_data.add_on_top = FALSE;
  options.list_data.simple_tips = FALSE;
  options.list_data.header_tips = FALSE;
  options.list_data.col_align = NULL;
  options.list_data.hdr_align = NULL;

  /* Initialize notebook data */
  options.notebook_data.tabs = NULL;
  options.notebook_data.borders = 5;
  options.notebook_data.pos = GTK_POS_TOP;
  options.notebook_data.active = 1;
  options.notebook_data.expand = FALSE;
  options.notebook_data.stack = FALSE;

#ifdef HAVE_TRAY
  /* Initialize notification data */
  options.notification_data.middle = TRUE;
  options.notification_data.hidden = FALSE;
  options.notification_data.menu = NULL;
#endif

  /* Initialize paned data */
  options.paned_data.orient = GTK_ORIENTATION_VERTICAL;
  options.paned_data.splitter = -1;
  options.paned_data.focused = 1;

  /* Initialize picture data */
  options.picture_data.size = YAD_PICTURE_ORIG;
  options.picture_data.inc = 5;
  options.picture_data.change_cmd = NULL;

  /* Initialize print data */
  options.print_data.type = YAD_PRINT_TEXT;
  options.print_data.headers = FALSE;

  /* Initialize progress data */
  options.progress_data.bars = NULL;
  options.progress_data.watch_bar = 0;
  options.progress_data.progress_text = NULL;
  options.progress_data.pulsate = FALSE;
  options.progress_data.autoclose = FALSE;
#ifndef G_OS_WIN32
  options.progress_data.autokill = FALSE;
#endif
  options.progress_data.rtl = FALSE;
  options.progress_data.log = NULL;
  options.progress_data.log_expanded = FALSE;
  options.progress_data.log_on_top = FALSE;
  options.progress_data.log_height = -1;

  /* Initialize scale data */
  options.scale_data.value = 0;
  options.scale_data.min_value = 0;
  options.scale_data.max_value = 100;
  options.scale_data.step = 1;
  options.scale_data.page = -1;
  options.scale_data.print_partial = FALSE;
  options.scale_data.hide_value = FALSE;
  options.scale_data.have_value = FALSE;
  options.scale_data.invert = FALSE;
  options.scale_data.buttons = FALSE;
  options.scale_data.marks = NULL;
  options.scale_data.enforce_step = FALSE;

  /* Initialize text data */
  options.text_data.wrap = FALSE;
  options.text_data.justify = GTK_JUSTIFY_LEFT;
  options.text_data.margins = 0;
  options.text_data.line = 0;
  options.text_data.hide_cursor = TRUE;
  options.text_data.fore = NULL;
  options.text_data.back = NULL;
  options.text_data.confirm_save = FALSE;
  options.text_data.confirm_text = _("File was changed. Save it?");
#ifndef STANDALONE
  options.text_data.uri_color = g_settings_get_string (settings, "uri-color");
#else
  options.text_data.uri_color = URI_COLOR;
#endif
  options.text_data.formatted = FALSE;
  options.text_data.in_place = FALSE;

#ifdef HAVE_SOURCEVIEW
  /* Initialize sourceview data */
  options.source_data.lang = NULL;
#ifndef STANDALONE
  options.source_data.theme = g_settings_get_string (sv_settings, "theme");;
  options.source_data.line_num = g_settings_get_boolean (sv_settings, "line-num");
  options.source_data.line_hl = g_settings_get_boolean (sv_settings, "line-hl");
  options.source_data.line_marks = g_settings_get_boolean (sv_settings, "line-marks");
  options.source_data.m1_color = g_settings_get_string (sv_settings, "mark1-color");
  options.source_data.m2_color = g_settings_get_string (sv_settings, "mark2-color");
  options.source_data.right_margin = g_settings_get_int (sv_settings, "right-margin");
  options.source_data.brackets = g_settings_get_boolean (sv_settings, "brackets");
  options.source_data.indent = g_settings_get_boolean (sv_settings, "indent");
  options.source_data.tab_width = g_settings_get_int (sv_settings, "tab-width");
  options.source_data.indent_width = g_settings_get_int (sv_settings, "indent-width");
  options.source_data.smart_he = g_settings_get_enum (sv_settings, "homend");
  options.source_data.smart_bs = g_settings_get_boolean (sv_settings, "smart-bs");
  options.source_data.spaces = g_settings_get_boolean (sv_settings, "spaces");
#else
  options.source_data.theme = NULL;
  options.source_data.line_num = FALSE;
  options.source_data.line_hl = FALSE;
  options.source_data.line_marks = FALSE;
  options.source_data.m1_color = MARK1_COLOR;
  options.source_data.m2_color = MARK2_COLOR;
  options.source_data.right_margin = 0;
  options.source_data.brackets = FALSE;
  options.source_data.indent = FALSE;
  options.source_data.tab_width = 8;
  options.source_data.indent_width = 4;
  options.source_data.smart_he = GTK_SOURCE_SMART_HOME_END_DISABLED;
  options.source_data.smart_bs = FALSE;
  options.source_data.spaces = FALSE;
#endif
#endif
}

GOptionContext *
yad_create_context (void)
{
  GOptionContext *tmp_ctx;
  GOptionGroup *a_group;

  tmp_ctx = g_option_context_new (_("- Yet another dialoging program"));
  g_option_context_add_main_entries (tmp_ctx, rest_options, GETTEXT_PACKAGE);

  /* Adds general option entries */
  a_group = g_option_group_new ("general", _("General options"), _("Show general options"), NULL, NULL);
  g_option_group_add_entries (a_group, general_options);
  g_option_group_set_translation_domain (a_group, GETTEXT_PACKAGE);
  g_option_context_add_group (tmp_ctx, a_group);

  /* Adds common option entries */
  a_group = g_option_group_new ("common", _("Common options"), _("Show common options"), NULL, NULL);
  g_option_group_add_entries (a_group, common_options);
  g_option_group_set_translation_domain (a_group, GETTEXT_PACKAGE);
  g_option_context_add_group (tmp_ctx, a_group);

  /* Adds about option entries */
  a_group = g_option_group_new ("about", _("About dialog options"), _("Show custom about dialog options"), NULL, NULL);
  g_option_group_add_entries (a_group, about_options);
  g_option_group_set_translation_domain (a_group, GETTEXT_PACKAGE);
  g_option_context_add_group (tmp_ctx, a_group);

  /* Adds app option entries */
  a_group = g_option_group_new ("app", _("Application selection options"), _("Show application selection options"), NULL, NULL);
  g_option_group_add_entries (a_group, app_options);
  g_option_group_set_translation_domain (a_group, GETTEXT_PACKAGE);
  g_option_context_add_group (tmp_ctx, a_group);

  /* Adds calendar option entries */
  a_group = g_option_group_new ("calendar", _("Calendar options"), _("Show calendar options"), NULL, NULL);
  g_option_group_add_entries (a_group, calendar_options);
  g_option_group_set_translation_domain (a_group, GETTEXT_PACKAGE);
  g_option_context_add_group (tmp_ctx, a_group);

  /* Adds color option entries */
  a_group = g_option_group_new ("color", _("Color selection options"), _("Show color selection options"), NULL, NULL);
  g_option_group_add_entries (a_group, color_options);
  g_option_group_set_translation_domain (a_group, GETTEXT_PACKAGE);
  g_option_context_add_group (tmp_ctx, a_group);

  /* Adds dnd option entries */
  a_group = g_option_group_new ("dnd", _("DND options"), _("Show drag-n-drop options"), NULL, NULL);
  g_option_group_add_entries (a_group, dnd_options);
  g_option_group_set_translation_domain (a_group, GETTEXT_PACKAGE);
  g_option_context_add_group (tmp_ctx, a_group);

  /* Adds entry option entries */
  a_group = g_option_group_new ("entry", _("Text entry options"), _("Show text entry options"), NULL, NULL);
  g_option_group_add_entries (a_group, entry_options);
  g_option_group_set_translation_domain (a_group, GETTEXT_PACKAGE);
  g_option_context_add_group (tmp_ctx, a_group);

  /* Adds file selection option entries */
  a_group = g_option_group_new ("file", _("File selection options"), _("Show file selection options"), NULL, NULL);
  g_option_group_add_entries (a_group, file_options);
  g_option_group_set_translation_domain (a_group, GETTEXT_PACKAGE);
  g_option_context_add_group (tmp_ctx, a_group);

  /* Add font selection option entries */
  a_group = g_option_group_new ("font", _("Font selection options"), _("Show font selection options"), NULL, NULL);
  g_option_group_add_entries (a_group, font_options);
  g_option_group_set_translation_domain (a_group, GETTEXT_PACKAGE);
  g_option_context_add_group (tmp_ctx, a_group);

  /* Add form option entries */
  a_group = g_option_group_new ("form", _("Form options"), _("Show form options"), NULL, NULL);
  g_option_group_add_entries (a_group, form_options);
  g_option_group_set_translation_domain (a_group, GETTEXT_PACKAGE);
  g_option_context_add_group (tmp_ctx, a_group);

#ifdef HAVE_HTML
  /* Add html options entries */
  a_group = g_option_group_new ("html", _("HTML options"), _("Show HTML options"), NULL, NULL);
  g_option_group_add_entries (a_group, html_options);
  g_option_group_set_translation_domain (a_group, GETTEXT_PACKAGE);
  g_option_context_add_group (tmp_ctx, a_group);
#endif

  /* Add icons option entries */
  a_group = g_option_group_new ("icons", _("Icons box options"), _("Show icons box options"), NULL, NULL);
  g_option_group_add_entries (a_group, icons_options);
  g_option_group_set_translation_domain (a_group, GETTEXT_PACKAGE);
  g_option_context_add_group (tmp_ctx, a_group);

  /* Adds list option entries */
  a_group = g_option_group_new ("list", _("List options"), _("Show list options"), NULL, NULL);
  g_option_group_add_entries (a_group, list_options);
  g_option_group_set_translation_domain (a_group, GETTEXT_PACKAGE);
  g_option_context_add_group (tmp_ctx, a_group);

  /* Adds notebook option entries */
  a_group = g_option_group_new ("notebook", _("Notebook options"), _("Show notebook dialog options"), NULL, NULL);
  g_option_group_add_entries (a_group, notebook_options);
  g_option_group_set_translation_domain (a_group, GETTEXT_PACKAGE);
  g_option_context_add_group (tmp_ctx, a_group);

#ifdef HAVE_TRAY
  /* Adds notification option entries */
  a_group = g_option_group_new ("notification", _("Notification icon options"),
                                _("Show notification icon options"), NULL, NULL);
  g_option_group_add_entries (a_group, notification_options);
  g_option_group_set_translation_domain (a_group, GETTEXT_PACKAGE);
  g_option_context_add_group (tmp_ctx, a_group);
#endif

  /* Adds paned option entries */
  a_group = g_option_group_new ("paned", _("Paned dialog options"), _("Show paned dialog options"), NULL, NULL);
  g_option_group_add_entries (a_group, paned_options);
  g_option_group_set_translation_domain (a_group, GETTEXT_PACKAGE);
  g_option_context_add_group (tmp_ctx, a_group);

  /* Adds picture option entries */
  a_group = g_option_group_new ("picture", _("Picture dialog options"), _("Show picture dialog options"), NULL, NULL);
  g_option_group_add_entries (a_group, picture_options);
  g_option_group_set_translation_domain (a_group, GETTEXT_PACKAGE);
  g_option_context_add_group (tmp_ctx, a_group);

  /* Adds print option entries */
  a_group = g_option_group_new ("print", _("Print dialog options"), _("Show print dialog options"), NULL, NULL);
  g_option_group_add_entries (a_group, print_options);
  g_option_group_set_translation_domain (a_group, GETTEXT_PACKAGE);
  g_option_context_add_group (tmp_ctx, a_group);

  /* Adds progress option entries */
  a_group = g_option_group_new ("progress", _("Progress options"), _("Show progress options"), NULL, NULL);
  g_option_group_add_entries (a_group, progress_options);
  g_option_group_set_translation_domain (a_group, GETTEXT_PACKAGE);
  g_option_context_add_group (tmp_ctx, a_group);

  /* Adds scale option entries */
  a_group = g_option_group_new ("scale", _("Scale options"), _("Show scale options"), NULL, NULL);
  g_option_group_add_entries (a_group, scale_options);
  g_option_group_set_translation_domain (a_group, GETTEXT_PACKAGE);
  g_option_context_add_group (tmp_ctx, a_group);

  /* Adds text option entries */
  a_group = g_option_group_new ("text", _("Text information options"), _("Show text information options"), NULL, NULL);
  g_option_group_add_entries (a_group, text_options);
  g_option_group_set_translation_domain (a_group, GETTEXT_PACKAGE);
  g_option_context_add_group (tmp_ctx, a_group);

#ifdef HAVE_SOURCEVIEW
  /* Adds sourceview option entries */
  a_group = g_option_group_new ("source", _("SourceView options"), _("Show SourceView options"), NULL, NULL);
  g_option_group_add_entries (a_group, source_options);
  g_option_group_set_translation_domain (a_group, GETTEXT_PACKAGE);
  g_option_context_add_group (tmp_ctx, a_group);
#endif

  /* Adds file filters option entries */
  a_group = g_option_group_new ("filter", _("File filter options"), _("Show file filter options"), NULL, NULL);
  g_option_group_add_entries (a_group, filter_options);
  g_option_group_set_translation_domain (a_group, GETTEXT_PACKAGE);
  g_option_context_add_group (tmp_ctx, a_group);

  /* Adds miscellaneous option entries */
  a_group = g_option_group_new ("misc", _("Miscellaneous options"), _("Show miscellaneous options"), NULL, NULL);
  g_option_group_add_entries (a_group, misc_options);
  g_option_group_set_translation_domain (a_group, GETTEXT_PACKAGE);
  g_option_context_add_group (tmp_ctx, a_group);

  /* Adds gtk option entries */
  a_group = gtk_get_option_group (TRUE);
  g_option_context_add_group (tmp_ctx, a_group);

  g_option_context_set_help_enabled (tmp_ctx, TRUE);
#ifndef STANDALONE
  if (!options.debug)
    g_option_context_set_ignore_unknown_options (tmp_ctx, g_settings_get_boolean (settings, "ignore-unknown-options"));
  else
    g_option_context_set_ignore_unknown_options (tmp_ctx, FALSE);
#else
  g_option_context_set_ignore_unknown_options (tmp_ctx, TRUE);
#endif

  return tmp_ctx;
}
