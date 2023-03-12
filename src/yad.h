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

#ifndef _YAD_H_
#define _YAD_H_

#include <config.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ipc.h>
#include <fcntl.h>

#include <gdk/gdkx.h>

#include <gtk/gtk.h>
#include <gtk/gtkx.h>
#include <gdk/gdkkeysyms.h>

#include <glib/gi18n.h>
#include <glib/gprintf.h>
#include <glib/gstdio.h>

#ifdef HAVE_HTML
#include <webkit2/webkit2.h>
#endif

#ifdef HAVE_SPELL
#include <gspell/gspell.h>
#endif

#ifdef HAVE_SOURCEVIEW
#include <gtksourceview/gtksource.h>
#endif

#ifdef STANDALONE
#include "defaults.h"
#endif

G_BEGIN_DECLS

#define YAD_RESPONSE_OK         0
#define YAD_RESPONSE_CANCEL     1
#define YAD_RESPONSE_TIMEOUT   	70
#define YAD_RESPONSE_ESC        -4      /* 252 */

#define YAD_URL_REGEX "(http|https|ftp|file)://[a-zA-Z0-9./_%#&-]+"

#define RIGHT_MARGIN     80  /* default right margin position for GtkSourceView */

#define SV_MARK1 "one"
#define SV_MARK2 "two"

typedef enum {
  YAD_MODE_MESSAGE,
  YAD_MODE_APP,
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
  YAD_MODE_NOTEBOOK,
#ifdef HAVE_TRAY
  YAD_MODE_NOTIFICATION,
#endif
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
  YAD_FIELD_SWITCH,
  YAD_FIELD_COMBO,
  YAD_FIELD_COMBO_ENTRY,
  YAD_FIELD_FILE,
  YAD_FIELD_FILE_SAVE,
  YAD_FIELD_MFILE,
  YAD_FIELD_DIR,
  YAD_FIELD_DIR_CREATE,
  YAD_FIELD_MDIR,
  YAD_FIELD_FONT,
  YAD_FIELD_APP,
  YAD_FIELD_ICON,
  YAD_FIELD_COLOR,
  YAD_FIELD_DATE,
  YAD_FIELD_SCALE,
  YAD_FIELD_BUTTON,
  YAD_FIELD_FULL_BUTTON,
  YAD_FIELD_LINK,
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
  YAD_COLUMN_TIP,
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

typedef enum {
  YAD_BOOL_FMT_UT,
  YAD_BOOL_FMT_UY,
  YAD_BOOL_FMT_UO,
  YAD_BOOL_FMT_LT,
  YAD_BOOL_FMT_LY,
  YAD_BOOL_FMT_LO,
  YAD_BOOL_FMT_1
} YadBoolFormat;

typedef struct {
  gchar *name;
  gchar *cmd;
  gint response;
} YadButton;

typedef struct {
  gchar *name;
  gchar *tip;
  YadFieldType type;
} YadField;

typedef struct {
  gchar *name;
  YadColumnType type;
  gboolean wrap;
  gboolean ellipsize;
  gboolean editable;
  gdouble c_align;
  gdouble h_align;
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
  gboolean negx;
  gboolean use_posy;
  gint posy;
  gboolean negy;
  gchar *geometry;
  guint timeout;
  gchar *to_indicator;
  gchar *dialog_text;
  guint text_width;
  GtkJustification text_align;
  gchar *dialog_image;
  gchar *icon_theme;
  gchar *expander;
  gint borders;
  GtkPolicyType hscroll_policy;
  GtkPolicyType vscroll_policy;
  GSList *buttons;
  gboolean no_buttons;
  gboolean no_markup;
  gboolean no_escape;
  gboolean escape_ok;
  gboolean always_print;
  gboolean selectable_labels;
  gboolean keep_icon_size;
  GtkButtonBoxStyle buttons_layout;
  gint def_resp;
  gboolean use_interp;
  gchar *interp;
  gchar *uri_handler;
  gchar *f1_action;
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
  gboolean show_fallback;
  gboolean show_other;
  gboolean show_all;
  gboolean extended;
} YadAppData;

typedef struct {
  gchar *name;
  gchar *version;
  gchar *copyright;
  gchar *comments;
  gchar *license;
  gchar *authors;
  gchar *website;
  gchar *website_lbl;
} YadAboutData;

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
  gboolean color_picker;
  gboolean expand_palette;
  gchar *palette;
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
  gboolean output_by_row;
  guint focus_field;
  gboolean cycle_read;
  gboolean align_buttons;
  gchar *changed_action;
  gboolean homogeneous;
} YadFormData;

#ifdef HAVE_HTML
typedef struct {
  gchar *uri;
  gboolean browser;
  gboolean print_uri;
  gchar *encoding;
  gchar *user_agent;
  gchar *user_style;
  gchar **wk_props;
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
  gboolean monitor;
} YadIconsData;

typedef struct {
  GSList *columns;
  gboolean tree_mode;
  gboolean checkbox;
  gboolean radiobox;
  gboolean no_headers;
  gboolean print_all;
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
  gchar *row_action;
  gboolean tree_expanded;
  gboolean regex_search;
  gboolean clickable;
  gboolean no_selection;
  gboolean add_on_top;
  gboolean simple_tips;
  gboolean header_tips;
  gchar *col_align;
  gchar *hdr_align;
} YadListData;

typedef struct {
  gchar **tabs;
  guint borders;
  GtkPositionType pos;
  guint active;
  gboolean expand;
  gboolean stack;
} YadNotebookData;

#ifdef HAVE_TRAY
typedef struct {
  gboolean middle;
  gboolean hidden;
  gchar *menu;
} YadNotificationData;
#endif

typedef struct {
  GtkOrientation orient;
  gint splitter;
  gint focused;
} YadPanedData;

typedef struct {
  YadPictureType size;
  gchar *change_cmd;
  gint inc;
} YadPictureData;

typedef struct {
  YadPrintType type;
  gboolean headers;
} YadPrintData;

typedef struct {
  GSList *bars;
  gint watch_bar;
  gchar *progress_text;
  gboolean pulsate;
  gboolean autoclose;
#ifndef G_OS_WIN32
  gboolean autokill;
#endif
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
  gboolean enforce_step;
} YadScaleData;

typedef struct {
  gboolean wrap;
  GtkJustification justify;
  gint margins;
  gulong line;
  gboolean uri;
  gboolean hide_cursor;
  gchar *uri_color;
  gboolean formatted;
  gchar *fore;
  gchar *back;
  gboolean in_place;
  gboolean confirm_save;
  gchar *confirm_text;
} YadTextData;

#ifdef HAVE_SOURCEVIEW
typedef struct {
  gchar *lang;
  gchar *theme;
  gboolean line_num;
  gboolean line_hl;
  gboolean line_marks;
  gchar *m1_color;
  gchar *m2_color;
  guint right_margin;
  gboolean brackets;
  gboolean indent;
  gint tab_width;
  gint indent_width;
  GtkSourceSmartHomeEndType smart_he;
  gboolean smart_bs;
  gboolean spaces;
} YadSourceData;
#endif

typedef struct {
  gchar *uri;
  gchar *mime;
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
  gboolean large_preview;
  gboolean show_hidden;
  gboolean quoted_output;
  gboolean num_output;
  gboolean hide_text;
  gint icon_size;
  gboolean enable_search;
  gboolean file_op;
  gboolean scroll;
#if GLIB_CHECK_VERSION(2,30,0)
  GFormatSizeFlags size_fmt;
#endif
  YadBoolFormat bool_fmt;
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

  YadAboutData about_data;
  YadAppData app_data;
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
  YadNotebookData notebook_data;
#ifdef HAVE_TRAY
  YadNotificationData notification_data;
#endif
  YadPanedData paned_data;
  YadPictureData picture_data;
  YadPrintData print_data;
  YadProgressData progress_data;
  YadScaleData scale_data;
  YadTextData text_data;
#ifdef HAVE_SOURCEVIEW
  YadSourceData source_data;
#endif

  gchar *css;
  gchar *gtkrc_file;

  gchar *rest_file;
  gchar **extra_data;

  key_t plug;
  guint tabnum;

  gboolean debug;

#ifndef G_OS_WIN32
  guint kill_parent;
  gboolean print_xid;
  gchar *xid_file;
#endif
} YadOptions;

/* Searvh bar */
typedef struct {
  GtkWidget *bar;
  GtkWidget *entry;
  GtkWidget *next;
  GtkWidget *prev;
  GtkWidget *case_toggle;
  gboolean case_sensitive;
  gboolean new_search;
  const gchar *str;
} YadSearchBar;

extern YadOptions options;
extern GtkIconTheme *yad_icon_theme;

#ifndef STANDALONE
extern GSettings *settings;
extern GSettings *sv_settings;
#endif

extern GdkPixbuf *big_fallback_image;
extern GdkPixbuf *small_fallback_image;

extern gboolean ignore_esc;

/* TABS */
typedef struct {
  pid_t pid;
  Window xid;
} YadNTabs;

/* pointer to shared memory for tabbed dialog */
/* 0 item used for special info: */
/*   pid - memory id */
/*   xid - allow plugs to write shmem (for sync) */
extern YadNTabs *tabs;

/* STOCK ITEMS */
#define YAD_STOCK_COUNT 19

typedef struct {
  gchar *key;
  gchar *label;
  gchar *icon;
} YadStock;

extern const YadStock yad_stock_items[];

/* FUNCTION PROTOTYPES */
void yad_options_init (void);
GOptionContext *yad_create_context (void);
void yad_set_mode (void);
void yad_print_result (void);
void yad_exit (gint id);

GtkWidget *app_create_widget (GtkWidget *dlg);
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
void text_goto_line (void);

void app_print_result (void);
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

#ifdef HAVE_TRAY
gint yad_notification_run (void);
#endif
gint yad_print_run (void);
gint yad_about (void);

gboolean yad_send_notify (gboolean);

void notebook_close_childs (void);
void paned_close_childs (void);

void read_settings (void);
void write_settings (void);

void update_preview (GtkFileChooser *chooser, GtkWidget *p);

GdkPixbuf *get_pixbuf (gchar *name, YadIconSize size, gboolean force);
gchar *get_color (GdkRGBA *c);

gchar **split_arg (const gchar *str);

YadNTabs *get_tabs (key_t key, gboolean create);

gboolean stock_lookup (gchar *key, YadStock *it);

GtkWidget *get_label (gchar *str, guint border, GtkWidget *w);

gchar *escape_str (gchar *str);
gchar *escape_char (gchar *str, gchar ch);

gboolean check_complete (GtkEntryCompletion *c, const gchar *key, GtkTreeIter *iter, gpointer data);

void parse_geometry ();

gboolean get_bool_val (gchar *str);
gchar *print_bool_val (gboolean val);

gint run_command_sync (gchar *cmd, gchar **out);
void run_command_async (gchar *cmd);

gchar *pango_to_css (gchar *font);

void open_uri (const gchar *uri);

YadSearchBar *create_search_bar ();

gboolean yad_confirm_dlg (GtkWindow *parent, gchar *txt);

static inline void
strip_new_line (gchar * str)
{
  gint nl = strlen (str) - 1;

  if (str[nl] == '\n')
    str[nl] = '\0';
}

G_END_DECLS

#endif /* _YAD_H_ */
