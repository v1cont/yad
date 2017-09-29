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
 * Copyright (C) 2008-2017, Victor Ananjevsky <ananasik@gmail.com>
 */

#ifndef _YAD_H_
#define _YAD_H_

#include <config.h>

#include <sys/types.h>
#include <sys/ipc.h>

#include <gdk/gdkx.h>

#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include <gdk/gdkkeysyms.h>

#if GTK_CHECK_VERSION(3,0,0)
#include <gtk/gtkx.h>
#endif

#ifdef HAVE_SPELL
#include <gtkspell/gtkspell.h>
#endif

#ifdef HAVE_SOURCEVIEW
#include <gtksourceview/gtksourceview.h>
#include <gtksourceview/gtksourcebuffer.h>
#include <gtksourceview/gtksourcelanguage.h>
#include <gtksourceview/gtksourcelanguagemanager.h>
#include <gtksourceview/gtksourcestylescheme.h>
#include <gtksourceview/gtksourcestyleschememanager.h>
#endif

G_BEGIN_DECLS

#define YAD_SETTINGS_FILE "yad.conf"

#define YAD_RESPONSE_OK         0
#define YAD_RESPONSE_CANCEL     1
#define YAD_RESPONSE_TIMEOUT   	70
#define YAD_RESPONSE_ESC        -4      /* 252 */

#define YAD_URL_REGEX "(http|https|ftp)://[a-zA-Z0-9./_%#&-]+"

typedef enum {
  YAD_MODE_MESSAGE,
  YAD_MODE_CALENDAR,
  YAD_MODE_COLOR,
  YAD_MODE_DND,
  YAD_MODE_ENTRY,
  YAD_MODE_FILE,
  YAD_MODE_FONT,
  YAD_MODE_FORM,
#ifdef HAVE_HTML
  YAD_MODE_HTML,
#endif
  YAD_MODE_ICONS,
  YAD_MODE_LIST,
  YAD_MODE_MULTI_PROGRESS,
  YAD_MODE_NOTEBOOK,
  YAD_MODE_NOTIFICATION,
  YAD_MODE_PANED,
  YAD_MODE_PICTURE,
  YAD_MODE_PRINT,
  YAD_MODE_PROGRESS,
  YAD_MODE_SCALE,
  YAD_MODE_TEXTINFO,
  YAD_MODE_ABOUT,
  YAD_MODE_VERSION,
  YAD_MODE_LANGS,
  YAD_MODE_THEMES
} YadDialogMode;

typedef enum {
  YAD_COLOR_HEX,
  YAD_COLOR_RGB
} YadColorMode;

typedef enum {
  YAD_FIELD_SIMPLE = 0,
  YAD_FIELD_HIDDEN,
  YAD_FIELD_READ_ONLY,
  YAD_FIELD_COMPLETE,
  YAD_FIELD_NUM,
  YAD_FIELD_CHECK,
  YAD_FIELD_COMBO,
  YAD_FIELD_COMBO_ENTRY,
  YAD_FIELD_FILE,
  YAD_FIELD_FILE_SAVE,
  YAD_FIELD_MFILE,
  YAD_FIELD_DIR,
  YAD_FIELD_DIR_CREATE,
  YAD_FIELD_MDIR,
  YAD_FIELD_FONT,
  YAD_FIELD_COLOR,
  YAD_FIELD_DATE,
  YAD_FIELD_SCALE,
  YAD_FIELD_BUTTON,
  YAD_FIELD_FULL_BUTTON,
  YAD_FIELD_LABEL,
  YAD_FIELD_TEXT
} YadFieldType;

typedef enum {
  YAD_COLUMN_TEXT = 0,
  YAD_COLUMN_NUM,
  YAD_COLUMN_SIZE,
  YAD_COLUMN_FLOAT,
  YAD_COLUMN_CHECK,
  YAD_COLUMN_RADIO,
  YAD_COLUMN_BAR,
  YAD_COLUMN_IMAGE,
  YAD_COLUMN_HIDDEN,
  YAD_COLUMN_ATTR_FORE,
  YAD_COLUMN_ATTR_BACK,
  YAD_COLUMN_ATTR_FONT
} YadColumnType;

typedef enum {
  YAD_PICTURE_FIT,
  YAD_PICTURE_ORIG
} YadPictureType;

typedef enum {
  YAD_PRINT_TEXT = 0,
  YAD_PRINT_IMAGE,
  YAD_PRINT_RAW
} YadPrintType;

typedef enum {
  YAD_PROGRESS_NORMAL = 0,
  YAD_PROGRESS_RTL,
  YAD_PROGRESS_PULSE,
  YAD_PROGRESS_PERM
} YadProgressType;

typedef enum {
  YAD_BIG_ICON = 0,
  YAD_SMALL_ICON
} YadIconSize;

typedef enum {
  YAD_COMPLETE_SIMPLE = 0,
  YAD_COMPLETE_ANY,
  YAD_COMPLETE_ALL,
  YAD_COMPLETE_REGEX
} YadCompletionType;

typedef struct {
  gchar *name;
  gchar *cmd;
  gint response;
} YadButton;

typedef struct {
  gchar *name;
  YadFieldType type;
} YadField;

typedef struct {
  gchar *name;
  YadColumnType type;
  gboolean wrap;
  gboolean ellipsize;
  gboolean editable;
} YadColumn;

typedef struct {
  gchar *name;
  YadProgressType type;
} YadProgressBar;

typedef struct {
  gchar *name;
  gint value;
} YadScaleMark;

typedef struct {
  gchar *dialog_title;
  gchar *window_icon;
  gint width;
  gint height;
  gboolean use_posx;
  gint posx;
  gboolean use_posy;
  gint posy;
  gchar *geometry;
  guint timeout;
  gchar *to_indicator;
  gchar *dialog_text;
  GtkJustification text_align;
  gchar *dialog_image;
  gboolean image_on_top;
  gchar *icon_theme;
  gchar *expander;
  gint borders;
  GSList *buttons;
  gboolean no_buttons;
  gboolean no_markup;
  gboolean no_escape;
  gboolean escape_ok;
  gboolean always_print;
  gboolean selectable_labels;
  GtkButtonBoxStyle buttons_layout;
  gint def_resp;
  /* window settings */
  gboolean sticky;
  gboolean fixed;
  gboolean ontop;
  gboolean center;
  gboolean mouse;
  gboolean undecorated;
  gboolean skip_taskbar;
  gboolean maximized;
  gboolean fullscreen;
  gboolean splash;
  gboolean focus;
  gboolean close_on_unfocus;
} YadData;

typedef struct {
  gint day;
  gint month;
  gint year;
  gchar *details;
  gboolean weeks;
} YadCalendarData;

typedef struct {
  gchar *init_color;
  gboolean gtk_palette;
  gboolean use_palette;
  gboolean expand_palette;
  gchar *palette;
  gboolean extra;
  gboolean alpha;
  YadColorMode mode;
} YadColorData;

typedef struct {
  gboolean tooltip;
  guint exit_on_drop;
} YadDNDData;

typedef struct {
  gchar *entry_text;
  gchar *entry_label;
  gboolean hide_text;
  gboolean completion;
  gboolean numeric;
  gchar *licon;
  gchar *licon_action;
  gchar *ricon;
  gchar *ricon_action;
} YadEntryData;

typedef struct {
  gboolean directory;
  gboolean save;
  gboolean confirm_overwrite;
  gchar *confirm_text;
  gchar **file_filt;
  gchar **mime_filt;
  gchar *image_filt;
} YadFileData;

typedef struct {
  gchar *preview;
  gboolean separate_output;
} YadFontData;

typedef struct {
  GSList *fields;
  guint columns;
  gboolean scroll;
  gboolean output_by_row;
  guint focus_field;
  gboolean cycle_read;
} YadFormData;

#ifdef HAVE_HTML
typedef struct {
  gchar *uri;
  gboolean browser;
  gboolean print_uri;
  gchar *mime;
  gchar *encoding;
  gchar *uri_cmd;
  gchar *user_agent;
  gchar *user_style;
} YadHtmlData;
#endif

typedef struct {
  gchar *directory;
  gboolean compact;
  gboolean generic;
  gboolean descend;
  gboolean sort_by_name;
  gboolean single_click;
  guint width;
  gchar *term;
#ifdef HAVE_GIO
  gboolean monitor;
#endif
} YadIconsData;

typedef struct {
  GSList *columns;
  gboolean no_headers;
  gboolean checkbox;
  gboolean radiobox;
  gboolean print_all;
  gboolean rules_hint;
  GtkTreeViewGridLines grid_lines;
  gint print_column;
  gint hide_column;
  gint expand_column;
  gint search_column;
  gint tooltip_column;
  gint sep_column;
  gchar *sep_value;
  guint limit;
  gchar *editable_cols;
  gint wrap_width;
  gchar *wrap_cols;
  PangoEllipsizeMode ellipsize;
  gchar *ellipsize_cols;
  gchar *dclick_action;
  gchar *select_action;
  gchar *add_action;
  gboolean regex_search;
  gboolean clickable;
  gboolean no_selection;
  gboolean add_on_top;
} YadListData;

typedef struct {
  GSList *bars;
  gint watch_bar;
} YadMultiProgressData;

typedef struct {
  GSList *tabs;
  guint borders;
  GtkPositionType pos;
  guint active;
} YadNotebookData;

typedef struct {
  gboolean middle;
  gboolean hidden;
  gchar *menu;
} YadNotificationData;

typedef struct {
  GtkOrientation orient;
  gint splitter;
} YadPanedData;

typedef struct {
  YadPictureType size;
  gint inc;
} YadPictureData;

typedef struct {
  YadPrintType type;
  gboolean headers;
} YadPrintData;

typedef struct {
  gchar *progress_text;
  gboolean pulsate;
  gboolean autoclose;
#ifndef G_OS_WIN32
  gboolean autokill;
#endif
  guint percentage;
  gboolean rtl;
  gchar *log;
  gboolean log_expanded;
  gboolean log_on_top;
  gint log_height;
} YadProgressData;

typedef struct {
  gint value;
  gint min_value;
  gint max_value;
  gint step;
  gint page;
  gboolean print_partial;
  gboolean hide_value;
  gboolean have_value;
  gboolean invert;
  gboolean buttons;
  GSList *marks;
} YadScaleData;

typedef struct {
  gchar *fore;
  gchar *back;
  gboolean wrap;
  GtkJustification justify;
  gint margins;
  gboolean uri;
  gboolean hide_cursor;
  gchar *uri_color;
} YadTextData;

#ifdef HAVE_SOURCEVIEW
typedef struct {
  gchar *lang;
  gchar *theme;
} YadSourceData;
#endif

typedef struct {
  gchar *uri;
  gchar *font;
  gchar *separator;
  gchar *item_separator;
  gboolean editable;
  gboolean multi;
  gboolean vertical;
  gboolean tail;
  gchar *command;
  gchar *date_format;
  guint float_precision;
  gdouble align;
  gboolean listen;
  gboolean preview;
  gboolean show_hidden;
  gboolean quoted_output;
  gboolean num_output;
#if GLIB_CHECK_VERSION(2,30,0)
  GFormatSizeFlags size_fmt;
#endif
  YadCompletionType complete;
  GList *filters;
  key_t key;
#ifdef HAVE_SPELL
  gboolean enable_spell;
  gchar *spell_lang;
#endif
} YadCommonData;

typedef struct {
  YadDialogMode mode;

  YadData data;
  YadCommonData common_data;

  YadCalendarData calendar_data;
  YadColorData color_data;
  YadDNDData dnd_data;
  YadEntryData entry_data;
  YadFileData file_data;
  YadFontData font_data;
  YadFormData form_data;
#ifdef HAVE_HTML
  YadHtmlData html_data;
#endif
  YadIconsData icons_data;
  YadListData list_data;
  YadMultiProgressData multi_progress_data;
  YadNotebookData notebook_data;
  YadNotificationData notification_data;
  YadPanedData paned_data;
  YadPictureData picture_data;
  YadPrintData print_data;
  YadProgressData progress_data;
  YadScaleData scale_data;
  YadTextData text_data;
#ifdef HAVE_SOURCEVIEW
  YadSourceData source_data;
#endif

  gchar *gtkrc_file;

  GtkPolicyType hscroll_policy;
  GtkPolicyType vscroll_policy;

  gchar *rest_file;
  gchar **extra_data;

  key_t plug;
  guint tabnum;

#ifndef G_OS_WIN32
  guint kill_parent;
  gboolean print_xid;
  gchar *xid_file;
#endif
} YadOptions;

extern YadOptions options;

typedef struct {
  guint width;
  guint height;
  guint timeout;
  gchar *to_indicator;
  gboolean show_remain;
  gboolean combo_always_editable;
  gboolean ignore_unknown;
  GtkIconTheme *icon_theme;
  GdkPixbuf *big_fallback_image;
  GdkPixbuf *small_fallback_image;
  gchar *term;
  gchar *open_cmd;
  gchar *date_format;
  guint max_tab;

  GtkPrintSettings *print_settings;
  GtkPageSetup *page_setup;
} YadSettings;

extern YadSettings settings;

typedef struct {
  pid_t pid;
  Window xid;
} YadNTabs;

/* pointer to shared memory for tabbed dialog */
/* 0 item used for special info: */
/*   pid - memory id */
/*   xid - count of registered tabs (for sync) */
extern YadNTabs *tabs;
extern gint t_sem;

void yad_options_init (void);
GOptionContext *yad_create_context (void);
void yad_set_mode (void);
void yad_print_result (void);
void yad_exit (gint id);

GtkWidget *calendar_create_widget (GtkWidget *dlg);
GtkWidget *color_create_widget (GtkWidget *dlg);
GtkWidget *entry_create_widget (GtkWidget *dlg);
GtkWidget *file_create_widget (GtkWidget *dlg);
GtkWidget *font_create_widget (GtkWidget *dlg);
GtkWidget *form_create_widget (GtkWidget *dlg);
#ifdef HAVE_HTML
GtkWidget *html_create_widget (GtkWidget *dlg);
#endif
GtkWidget *icons_create_widget (GtkWidget *dlg);
GtkWidget *list_create_widget (GtkWidget *dlg);
GtkWidget *multi_progress_create_widget (GtkWidget *dlg);
GtkWidget *notebook_create_widget (GtkWidget *dlg);
GtkWidget *paned_create_widget (GtkWidget *dlg);
GtkWidget *picture_create_widget (GtkWidget *dlg);
GtkWidget *progress_create_widget (GtkWidget *dlg);
GtkWidget *scale_create_widget (GtkWidget *dlg);
GtkWidget *text_create_widget (GtkWidget *dlg);

gboolean file_confirm_overwrite (GtkWidget *dlg);
void notebook_swallow_childs (void);
void paned_swallow_childs (void);
void picture_fit_to_window (void);

void calendar_print_result (void);
void color_print_result (void);
void entry_print_result (void);
void file_print_result (void);
void font_print_result (void);
void form_print_result (void);
void list_print_result (void);
void notebook_print_result (void);
void paned_print_result (void);
void scale_print_result (void);
void text_print_result (void);

void dnd_init (GtkWidget *w);

gint yad_notification_run (void);
gint yad_print_run (void);
gint yad_about (void);

gboolean yad_send_notify (gboolean);

void notebook_close_childs (void);
void paned_close_childs (void);

void read_settings (void);
void write_settings (void);

void update_preview (GtkFileChooser *chooser, GtkWidget *p);
void filechooser_mapped (GtkWidget *w, gpointer data);

GdkPixbuf *get_pixbuf (gchar *name, YadIconSize size);
gchar *get_color (GdkColor *c, guint64 alpha);

gchar **split_arg (const gchar *str);

YadNTabs *get_tabs (key_t key, gboolean create);

GtkWidget *get_label (gchar *str, guint border);

gchar *escape_str (gchar *str);
gchar *escape_char (gchar *str, gchar ch);

gboolean check_complete (GtkEntryCompletion *c, const gchar *key, GtkTreeIter *iter, gpointer data);

void show_langs ();
void show_themes ();

static inline void
strip_new_line (gchar * str)
{
  gint nl = strlen (str) - 1;

  if (str[nl] == '\n')
    str[nl] = '\0';
}

G_END_DECLS

#endif /* _YAD_H_ */
