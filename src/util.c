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

#ifndef _GNU_SOURCE
#define _GNU_SOURCE 1
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <sys/ipc.h>
#include <sys/shm.h>

#include "yad.h"

const YadStock yad_stock_items[] = {
  { "yad-about", N_("About"), "help-about" },
  { "yad-add",  N_("Add"), "list-add" },
  { "yad-apply",  N_("Apply"), "gtk-apply" },
  { "yad-cancel",  N_("Cancel"), "gtk-cancel" },
  { "yad-clear",  N_("Clear"), "document-clear" },
  { "yad-close",  N_("Close"), "window-close" },
  { "yad-edit",  N_("Edit"), "gtk-edit" },
  { "yad-execute",  N_("Execute"), "system-run" },
  { "yad-no",  N_("No"), "gtk-no" },
  { "yad-ok",  N_("OK"), "gtk-ok" },
  { "yad-open",  N_("Open"), "document-open" },
  { "yad-print",  N_("Print"), "document-print" },
  { "yad-quit",  N_("Quit"), "application-exit" },
  { "yad-refresh",  N_("Refresh"), "view-refresh" },
  { "yad-remove",  N_("Remove"), "list-remove" },
  { "yad-save",  N_("Save"), "document-save" },
  { "yad-search", N_("Search"), "system-search" },
  { "yad-settings",  N_("Settings"), "gtk-preferences" },
  { "yad-yes",  N_("Yes"), "gtk-yes" }
};

gboolean
stock_lookup (gchar *key, YadStock *it)
{
  gint i;
  gboolean found = FALSE;

  if (key == NULL || strncmp (key, "yad-", 4) != 0)
    return FALSE;

  for (i = 0; i < YAD_STOCK_COUNT; i++)
    {
      if (strcmp (key, yad_stock_items[i].key) == 0)
        {
          it->key = yad_stock_items[i].key;
          it->label = _(yad_stock_items[i].label);
          it->icon = yad_stock_items[i].icon;
          found = TRUE;
          break;
        }
    }

  return found;
}

GdkPixbuf *
get_pixbuf (gchar *name, YadIconSize size, gboolean force)
{
  gint w, h;
  GdkPixbuf *pb = NULL;
  GError *err = NULL;

  if (size == YAD_BIG_ICON)
    gtk_icon_size_lookup (GTK_ICON_SIZE_DIALOG, &w, &h);
  else
    gtk_icon_size_lookup (GTK_ICON_SIZE_MENU, &w, &h);

  if (g_file_test (name, G_FILE_TEST_IS_REGULAR))
    {
      pb = gdk_pixbuf_new_from_file (name, &err);
      if (!pb)
        {
          g_printerr ("yad: get_pixbuf(): %s\n", err->message);
          g_error_free (err);
        }
    }
  else
    pb = gtk_icon_theme_load_icon (yad_icon_theme, name, MIN (w, h), GTK_ICON_LOOKUP_GENERIC_FALLBACK, NULL);

  if (!pb)
    {
      if (size == YAD_BIG_ICON)
        pb = g_object_ref (big_fallback_image);
      else
        pb = g_object_ref (small_fallback_image);
    }

  /* force scaling image to specific size */
  if (!options.data.keep_icon_size && force && pb)
    {
      gint iw = gdk_pixbuf_get_width (pb);
      gint ih = gdk_pixbuf_get_height (pb);

      if (w != iw || h != ih)
        {
          GdkPixbuf *spb;
          spb = gdk_pixbuf_scale_simple (pb, w, h, GDK_INTERP_BILINEAR);
          g_object_unref (pb);
          pb = spb;
        }
    }

  return pb;
}

gchar *
get_color (GdkRGBA *c)
{
  gchar *res = NULL;

  switch (options.color_data.mode)
    {
    case YAD_COLOR_HEX:
      if (options.color_data.alpha)
        res = g_strdup_printf ("#%02X%02X%02X%02X", (int) (c->red * 255), (int) (c->green * 255), (int) (c->blue * 255), (int) (c->alpha * 255));
      else
        res = g_strdup_printf ("#%02X%02X%02X", (int) (c->red * 255), (int) (c->green * 255), (int) (c->blue * 255));
      break;
    case YAD_COLOR_RGB:
      res = gdk_rgba_to_string (c);
      break;
    }
  return res;
}

void
update_preview (GtkFileChooser * chooser, GtkWidget *p)
{
  gchar *uri;
  static gchar *normal_path = NULL;
  static gchar *large_path = NULL;

  /* init thumbnails path */
  if (!normal_path)
    normal_path = g_build_filename (g_get_user_cache_dir (), "thumbnails", "normal", NULL);
  if (!large_path)
    large_path = g_build_filename (g_get_user_cache_dir (), "thumbnails", "large", NULL);

  /* load preview */
  uri = gtk_file_chooser_get_preview_uri (chooser);
  if (uri)
    {
      gchar *file;
      GChecksum *chs;
      GdkPixbuf *pb = NULL;

      chs = g_checksum_new (G_CHECKSUM_MD5);
      g_checksum_update (chs, (const guchar *) uri, -1);
      /* first try to get preview from large thumbnail */
      file = g_strdup_printf ("%s/%s.png", large_path, g_checksum_get_string (chs));
      if (options.common_data.large_preview && g_file_test (file, G_FILE_TEST_IS_REGULAR))
        pb = gdk_pixbuf_new_from_file (file, NULL);
      else
        {
          /* try to get preview from normal thumbnail */
          g_free (file);
          file = g_strdup_printf ("%s/%s.png", normal_path, g_checksum_get_string (chs));
          if (!options.common_data.large_preview && g_file_test (file, G_FILE_TEST_IS_REGULAR))
            pb = gdk_pixbuf_new_from_file (file, NULL);
          else
            {
              /* try to create it */
              g_free (file);
              file = g_filename_from_uri (uri, NULL, NULL);
              if (g_file_test (file, G_FILE_TEST_IS_REGULAR))
                {
                  if (options.common_data.large_preview)
                    pb = gdk_pixbuf_new_from_file_at_scale (file, 256, 256, TRUE, NULL);
                  else
                    pb = gdk_pixbuf_new_from_file_at_scale (file, 128, 128, TRUE, NULL);
                }
              if (pb)
                {
                  struct stat st;
                  gchar *smtime;

                  stat (file, &st);
                  smtime = g_strdup_printf ("%lu", st.st_mtime);
                  g_free (file);

                  /* save thumbnail */
                  if (options.common_data.large_preview)
                    {
                      g_mkdir_with_parents (large_path, 0700);
                      file = g_strdup_printf ("%s/%s.png", large_path, g_checksum_get_string (chs));
                    }
                  else
                    {
                      g_mkdir_with_parents (normal_path, 0700);
                      file = g_strdup_printf ("%s/%s.png", normal_path, g_checksum_get_string (chs));
                    }
                  gdk_pixbuf_save (pb, file, "png", NULL,
                                   "tEXt::Thumb::URI", uri,
                                   "tEXt::Thumb::MTime", smtime,
                                   NULL);
                  g_chmod (file, 0600);
                  g_free (smtime);
                }
            }
        }
      g_checksum_free (chs);
      g_free (file);

      if (pb)
        {
          gtk_image_set_from_pixbuf (GTK_IMAGE (p), pb);
          g_object_unref (pb);
          gtk_file_chooser_set_preview_widget_active (chooser, TRUE);
        }
      else
        gtk_file_chooser_set_preview_widget_active (chooser, FALSE);

      g_free (uri);
    }
  else
    gtk_file_chooser_set_preview_widget_active (chooser, FALSE);
}

gchar **
split_arg (const gchar *str)
{
  gchar **res;
  gchar *p_col;

  res = g_new0 (gchar *, 3);

  p_col = g_strrstr (str, ":");
  if (p_col && p_col[1])
    {
      res[0] = g_strndup (str, p_col - str);
      res[1] = g_strdup (p_col + 1);
    }
  else
    res[0] = g_strdup (str);

  return res;
}

YadNTabs *
get_tabs (key_t key, gboolean create)
{
  YadNTabs *t = NULL;
  int shmid, i, max_tab;

  /* get shared memory */
#ifndef STANDALONE
  max_tab = g_settings_get_int (settings, "max-tab") + 1;
#else
  max_tab = MAX_TABS + 1;
#endif
  if (create)
    {
      if ((shmid = shmget (key, max_tab * sizeof (YadNTabs), IPC_CREAT | IPC_EXCL | 0644)) == -1)
        {
          g_printerr ("yad: cannot create shared memory for key %d: %s\n", key, strerror (errno));
          return NULL;
        }
    }
  else
    {
      if ((shmid = shmget (key, max_tab * sizeof (YadNTabs), 0)) == -1)
        {
          if (errno != ENOENT)
            g_printerr ("yad: cannot get shared memory for key %d: %s\n", key, strerror (errno));
          return NULL;
        }
    }

  /* attach shared memory */
  if ((t = shmat (shmid, NULL, 0)) == (YadNTabs *) -1)
    {
      g_printerr ("yad: cannot attach shared memory for key %d: %s\n", key, strerror (errno));
      return NULL;
    }

  /* initialize memory */
  if (create)
    {
      for (i = 1; i < max_tab; i++)
        {
          t[i].pid = -1;
          t[i].xid = 0;
        }
      t[0].pid = shmid;
      /* lastly, allow plugs to write shmem */
      t[0].xid = 1;
    }

  return t;
}

GtkWidget *
get_label (gchar *str, guint border, GtkWidget *w)
{
  GtkWidget *t, *i, *l;
  YadStock it;
  gchar **vals;

  if (!str || !*str)
    return gtk_label_new (NULL);

  l = i = NULL;

  t = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 2);
  gtk_container_set_border_width (GTK_CONTAINER (t), border);

  gtk_widget_set_halign (t, GTK_ALIGN_CENTER);
  gtk_widget_set_valign (t, GTK_ALIGN_CENTER);

  vals = g_strsplit_set (str, options.common_data.item_separator, 3);
  if (stock_lookup (vals[0], &it))
    {
      l = gtk_label_new_with_mnemonic (it.label);
      i = gtk_image_new_from_pixbuf (get_pixbuf (it.icon, YAD_SMALL_ICON, TRUE));
    }
  else
    {
      if (vals[0] && *vals[0])
        {
          l = gtk_label_new (NULL);
          if (!options.data.no_markup)
            gtk_label_set_markup_with_mnemonic (GTK_LABEL (l), vals[0]);
          else
            gtk_label_set_text_with_mnemonic (GTK_LABEL (l), vals[0]);
        }

      if (vals[1] && *vals[1])
        i = gtk_image_new_from_pixbuf (get_pixbuf (vals[1], YAD_SMALL_ICON, TRUE));
    }

  if (i)
    gtk_container_add (GTK_CONTAINER (t), i);

  if (l)
    {
      if (w)
        gtk_label_set_mnemonic_widget (GTK_LABEL (l), w);
      gtk_label_set_xalign (GTK_LABEL (l), 0.0);
      gtk_box_pack_start (GTK_BOX (t), l, FALSE, FALSE, 1);
    }

  /* !!! must check both 1 and 2 values for !NULL */
  if (vals[1] && vals[2] && *vals[2])
    {
      if (!options.data.no_markup)
        gtk_widget_set_tooltip_markup (t, vals[2]);
      else
        gtk_widget_set_tooltip_text (t, vals[2]);
    }

  g_strfreev (vals);

  gtk_widget_show_all (t);

  return t;
}

gchar *
escape_str (gchar *str)
{
  gchar *res, *buf = str;
  guint i = 0, len;

  if (!str)
    return NULL;

  len = strlen (str);
  res = (gchar *) calloc (len * 2 + 1, sizeof (gchar));

  while (*buf)
    {
      switch (*buf)
        {
        case '\n':
          strcpy (res + i, "\\n");
          i += 2;
          break;
        case '\t':
          strcpy (res + i, "\\t");
          i += 2;
          break;
        case '\\':
          strcpy (res + i, "\\\\");
          i += 2;
          break;
        default:
          *(res + i) = *buf;
          i++;
          break;
        }
      buf++;
    }
  res[i] = '\0';

  return res;
}

gchar *
escape_char (gchar *str, gchar ch)
{
  gchar *res, *buf = str;
  guint i = 0, len;

  if (!str)
    return NULL;

  len = strlen (str);
  res = (gchar *) calloc (len * 2 + 1, sizeof (gchar));

  while (*buf)
    {
      if (*buf == ch)
        {
          strcpy (res + i, "\\\"");
          i += 2;
        }
      else
        {
          *(res + i) = *buf;
          i++;
        }
      buf++;
    }
  res[i] = '\0';

  return res;
}

gboolean
check_complete (GtkEntryCompletion *c, const gchar *key, GtkTreeIter *iter, gpointer data)
{
  gchar *value = NULL;
  GtkTreeModel *model = gtk_entry_completion_get_model (c);
  gboolean found = FALSE;

  if (!model || !key || !key[0])
    return FALSE;

  gtk_tree_model_get (model, iter, 0, &value, -1);

  if (value)
    {
      gchar **words = NULL;
      guint i = 0;

      switch (options.common_data.complete)
        {
        case YAD_COMPLETE_ANY:
          words = g_strsplit_set (key, " \t", -1);
          while (words[i])
            {
              if (strcasestr (value, words[i]) != NULL)
                {
                  /* found one of the words */
                  found = TRUE;
                  break;
                }
              i++;
            }
          break;
        case YAD_COMPLETE_ALL:
          words = g_strsplit_set (key, " \t", -1);
          found = TRUE;
          while (words[i])
            {
              if (strcasestr (value, words[i]) == NULL)
                {
                  /* not found one of the words */
                  found = FALSE;
                  break;
                }
              i++;
            }
          break;
        case YAD_COMPLETE_REGEX:
          found = g_regex_match_simple (key, value, G_REGEX_CASELESS | G_REGEX_OPTIMIZE, G_REGEX_MATCH_NOTEMPTY);
          break;
        default: ;
        }

      if (words)
        g_strfreev (words);
    }

  return found;
}

void
parse_geometry ()
{
  gchar *geom, *ptr;
  gint w = -1, h = -1, x = 0, y = 0;
  gboolean usexy = FALSE;
  guint i = 0;

  if (!options.data.geometry)
    return;

  geom = options.data.geometry;

  if (geom[i] != '+' && geom[i] != '-')
    {
      ptr = geom + i;
      w = atoi (ptr);

      while (geom[i] && geom[i] != 'x') i++;

      if (!geom[i])
        return;

      ptr = geom + i + 1;
      h = atoi (ptr);

      while (geom[i] && geom[i] != '-' && geom[i] != '+') i++;
    }

  if (geom[i])
    {
      usexy = TRUE;

      ptr = geom + i;
      x = atoi (ptr);
      options.data.negx = (*ptr == '-');

      i++;
      while (geom[i] && geom[i] != '-' && geom[i] != '+') i++;

      if (!geom[i])
        return;

      ptr = geom + i;
      y = atoi (ptr);
      options.data.negy = (*ptr == '-');
    }

  if (w != -1)
    options.data.width = w;
  if (h != -1)
    options.data.height = h;
  options.data.posx = x;
  options.data.posy = y;
  options.data.use_posx = options.data.use_posy = usexy;
}

gboolean
get_bool_val (gchar *str)
{
  if (!str || !str[0])
    return FALSE;

  switch (str[0])
    {
    case '1':
    case 't':
    case 'T':
    case 'y':
    case 'Y':
      if (strcasecmp (str, "t") == 0 || strcasecmp (str, "true") == 0 ||
          strcasecmp (str, "y") == 0 || strcasecmp (str, "yes") == 0 ||
          strcmp (str, "1") == 0)
        return TRUE;
      break;
    case '0':
    case 'f':
    case 'F':
    case 'n':
    case 'N':
      if (strcasecmp (str, "f") == 0 || strcasecmp (str, "false") == 0 ||
          strcasecmp (str, "n") == 0 || strcasecmp (str, "no") == 0 ||
          strcmp (str, "0") == 0)
        return FALSE;
      break;
    case 'o':
    case 'O':
      if (strcasecmp (str, "on") == 0)
        return TRUE;
      else if (strcasecmp (str, "off") == 0)
        return FALSE;
      break;
    default: ;
    }

 g_printerr ("yad: wrong boolean value '%s'\n", str);
 return FALSE;
}

gchar *
print_bool_val (gboolean val)
{
  gchar *rv = "";

  switch (options.common_data.bool_fmt)
    {
    case YAD_BOOL_FMT_UT:
      rv = val ? "TRUE" : "FALSE";
      break;
    case YAD_BOOL_FMT_UY:
      rv = val ? "YES" : "NO";
      break;
    case YAD_BOOL_FMT_UO:
      rv = val ? "ON" : "OFF";
      break;
    case YAD_BOOL_FMT_LT:
      rv = val ? "true" : "false";
      break;
    case YAD_BOOL_FMT_LY:
      rv = val ? "yes" : "no";
      break;
    case YAD_BOOL_FMT_LO:
      rv = val ? "on" : "off";
      break;
    case YAD_BOOL_FMT_1:
      rv = val ? "1" : "0";
      break;
    }

  return rv;
}

typedef struct {
  gchar *cmd;
  gchar **out;
  gint ret;
  gboolean lock;
} RunData;

static void
run_thread (RunData *d)
{
  GError *err = NULL;

  if (!g_spawn_command_line_sync (d->cmd, d->out, NULL, NULL, &err))
    {
      if (options.debug)
        g_printerr (_("WARNING: Run command failed: %s\n"), err->message);
      g_error_free (err);
      d->ret = -1;
    }
  d->lock = FALSE;
}

gint
run_command_sync (gchar *cmd, gchar **out)
{
  RunData *d;
  gint ret;

  d = g_new0 (RunData, 1);

  if (options.data.use_interp)
    {
      if (g_strstr_len (options.data.interp, -1, "%s") != NULL)
        d->cmd = g_strdup_printf (options.data.interp, cmd);
      else
        d->cmd = g_strdup_printf ("%s %s", options.data.interp, cmd);
    }
  else
    d->cmd = g_strdup (cmd);
  d->out = out;

  d->lock = TRUE;
  g_thread_new ("run_sync", (GThreadFunc) run_thread, d);

  while (d->lock != FALSE)
    {
      gtk_main_iteration_do (FALSE);
      usleep (10000);
    }

  ret = d->ret;
  g_free (d->cmd);
  g_free (d);

  return ret;
}

void
run_command_async (gchar *cmd)
{
  gchar *full_cmd = NULL;
  GError *err = NULL;

  if (options.data.use_interp)
    {
      if (g_strstr_len (options.data.interp, -1, "%s") != NULL)
        full_cmd = g_strdup_printf (options.data.interp, cmd);
      else
        full_cmd = g_strdup_printf ("%s %s", options.data.interp, cmd);
    }
  else
    full_cmd = g_strdup (cmd);

  if (!g_spawn_command_line_async (full_cmd, &err))
    {
      if (options.debug)
        g_printerr (_("WARNING: Run command failed: %s\n"), err->message);
      g_error_free (err);
    }

  g_free (full_cmd);
}

gchar *
pango_to_css (gchar *font)
{
  PangoFontDescription *desc;
  PangoFontMask mask;
  GString *str;
  gchar *res;

  str = g_string_new (NULL);

  desc = pango_font_description_from_string (font);
  mask = pango_font_description_get_set_fields (desc);

  if (mask & PANGO_FONT_MASK_STYLE)
    {
      switch (pango_font_description_get_style (desc))
        {
        case PANGO_STYLE_OBLIQUE:
          g_string_append (str, "oblique ");
          break;
        case PANGO_STYLE_ITALIC:
          g_string_append (str, "italic ");
          break;
        default: ;
        }
    }
  if (mask & PANGO_FONT_MASK_VARIANT)
    {
      if (pango_font_description_get_variant (desc) == PANGO_VARIANT_SMALL_CAPS)
        g_string_append (str, "small-caps ");
    }

  if (mask & PANGO_FONT_MASK_WEIGHT)
    {
      switch (pango_font_description_get_weight (desc))
        {
        case PANGO_WEIGHT_THIN:
          g_string_append (str, "Thin ");
          break;
        case PANGO_WEIGHT_ULTRALIGHT:
          g_string_append (str, "Ultralight ");
          break;
        case PANGO_WEIGHT_LIGHT:
          g_string_append (str, "Light ");
          break;
        case PANGO_WEIGHT_SEMILIGHT:
          g_string_append (str, "Semilight ");
          break;
        case PANGO_WEIGHT_BOOK:
          g_string_append (str, "Book ");
          break;
        case PANGO_WEIGHT_MEDIUM:
          g_string_append (str, "Medium ");
          break;
        case PANGO_WEIGHT_SEMIBOLD:
          g_string_append (str, "Semibold ");
          break;
        case PANGO_WEIGHT_BOLD:
          g_string_append (str, "Bold ");
          break;
        case PANGO_WEIGHT_ULTRABOLD:
          g_string_append (str, "Ultrabold ");
          break;
        case PANGO_WEIGHT_HEAVY:
          g_string_append (str, "Heavy ");
          break;
        case PANGO_WEIGHT_ULTRAHEAVY:
          g_string_append (str, "Ultraheavy ");
          break;
        default: ;
        }
    }

  if (mask & PANGO_FONT_MASK_SIZE)
    {
      if (pango_font_description_get_size_is_absolute (desc))
        g_string_append_printf (str, "%dpx ", pango_font_description_get_size (desc) / PANGO_SCALE);
      else
        g_string_append_printf (str, "%dpt ", pango_font_description_get_size (desc) / PANGO_SCALE);
    }

  if (mask & PANGO_FONT_MASK_FAMILY)
    g_string_append (str, pango_font_description_get_family (desc));

  if (str->str)
    res = str->str;
  else
    res = g_strdup (font);

  return res;
}

void
open_uri (const gchar *uri)
{
  gchar *cmdline;

  if (!uri || !uri[0])
    return;

  if (g_strstr_len (options.data.uri_handler, -1, "%s") != NULL)
    cmdline = g_strdup_printf (options.data.uri_handler, uri);
  else
    cmdline = g_strdup_printf ("%s '%s'", options.data.uri_handler, uri);
  run_command_async (cmdline);
  g_free (cmdline);
}

/* Search bar */
static void
next_clicked_cb (GtkWidget *w, YadSearchBar *sb)
{
  g_signal_emit_by_name (sb->entry, "next-match");
}

static void
prev_clicked_cb (GtkWidget *w, YadSearchBar *sb)
{
  g_signal_emit_by_name (sb->entry, "previous-match");
}

static void
case_toggle_cb (GtkToggleButton *b, YadSearchBar *sb)
{
  sb->case_sensitive = !sb->case_sensitive;
  gtk_toggle_button_set_active (b, sb->case_sensitive);
}

YadSearchBar *
create_search_bar ()
{
  YadSearchBar *sb;
  GtkWidget *b;
  gint e_width = -1;

  sb = g_new0 (YadSearchBar, 1);
  sb->new_search = TRUE;

  sb->bar = gtk_search_bar_new ();
  gtk_search_bar_set_show_close_button (GTK_SEARCH_BAR (sb->bar), TRUE);

  b = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 2);
  gtk_container_add (GTK_CONTAINER (sb->bar), b);

  sb->entry = gtk_search_entry_new ();
  gtk_box_pack_start (GTK_BOX (b), sb->entry, TRUE, TRUE, 0);
  gtk_search_bar_connect_entry (GTK_SEARCH_BAR (sb->bar), GTK_ENTRY (sb->entry));

#ifndef STANDALONE
  e_width = g_settings_get_int (settings, "search-width");
#endif
  if (e_width > 0)
    gtk_widget_set_size_request (sb->entry, e_width, -1);

  sb->next = gtk_button_new_from_icon_name ("go-down", GTK_ICON_SIZE_BUTTON);
  gtk_widget_set_focus_on_click (sb->next, FALSE);
  gtk_box_pack_start (GTK_BOX (b), sb->next, FALSE, FALSE, 0);

  g_signal_connect (G_OBJECT (sb->next), "clicked", G_CALLBACK (next_clicked_cb), sb);

  sb->prev = gtk_button_new_from_icon_name ("go-up", GTK_ICON_SIZE_BUTTON);
  gtk_widget_set_focus_on_click (sb->prev, FALSE);
  gtk_box_pack_start (GTK_BOX (b), sb->prev, FALSE, FALSE, 0);

  g_signal_connect (G_OBJECT (sb->prev), "clicked", G_CALLBACK (prev_clicked_cb), sb);

  sb->case_toggle = gtk_check_button_new_with_mnemonic (_("Case _sensitive"));
  gtk_widget_set_focus_on_click (sb->case_toggle, FALSE);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (sb->case_toggle), sb->case_sensitive);
  gtk_box_pack_start (GTK_BOX (b), sb->case_toggle, FALSE, FALSE, 0);

  g_signal_connect (G_OBJECT (sb->case_toggle), "toggled", G_CALLBACK (case_toggle_cb), sb);

  return sb;
}

/* Confirmation dialog */
gboolean
yad_confirm_dlg (GtkWindow *parent, gchar *txt)
{
  GtkWidget *d;
  gchar *buf;
  gint ret;

  buf = g_strcompress (options.text_data.confirm_text);
  d = gtk_message_dialog_new (parent, GTK_DIALOG_DESTROY_WITH_PARENT,
                              GTK_MESSAGE_QUESTION, GTK_BUTTONS_YES_NO, "%s", buf);
  gtk_window_set_position (GTK_WINDOW (d), GTK_WIN_POS_CENTER_ON_PARENT);
  g_free (buf);

  ret = gtk_dialog_run (GTK_DIALOG (d));
  gtk_widget_destroy (d);

  return (ret == GTK_RESPONSE_YES);
}
