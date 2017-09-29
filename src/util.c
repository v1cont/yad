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

#ifndef _GNU_SOURCE
#define _GNU_SOURCE 1
#endif

#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <sys/ipc.h>
#include <sys/shm.h>

#include "yad.h"

YadSettings settings;

void
read_settings (void)
{
  GKeyFile *kf;
  gchar *filename;

  /* set defaults */
  settings.width = settings.height = -1;
  settings.timeout = 0;
  settings.to_indicator = "none";
  settings.show_remain = FALSE;
  settings.combo_always_editable = FALSE;
  settings.term = "xterm -e '%s'";
  settings.open_cmd = "xdg-open '%s'";
  settings.date_format = "%x";
  settings.ignore_unknown = TRUE;
  settings.max_tab = 100;

  settings.print_settings = NULL;
  settings.page_setup = NULL;

  filename = g_build_filename (g_get_user_config_dir (), YAD_SETTINGS_FILE, NULL);

  if (g_file_test (filename, G_FILE_TEST_EXISTS))
    {
      kf = g_key_file_new ();

      if (g_key_file_load_from_file (kf, filename, G_KEY_FILE_NONE, NULL))
        {
          if (g_key_file_has_key (kf, "General", "width", NULL))
            settings.width = g_key_file_get_integer (kf, "General", "width", NULL);
          if (g_key_file_has_key (kf, "General", "height", NULL))
            settings.height = g_key_file_get_integer (kf, "General", "height", NULL);
          if (g_key_file_has_key (kf, "General", "timeout", NULL))
            settings.timeout = g_key_file_get_integer (kf, "General", "timeout", NULL);
          if (g_key_file_has_key (kf, "General", "timeout_indicator", NULL))
            settings.to_indicator = g_key_file_get_string (kf, "General", "timeout_indicator", NULL);
          if (g_key_file_has_key (kf, "General", "show_remain", NULL))
            settings.show_remain = g_key_file_get_boolean (kf, "General", "show_remain", NULL);
          if (g_key_file_has_key (kf, "General", "combo_always_editable", NULL))
            settings.combo_always_editable = g_key_file_get_boolean (kf, "General", "combo_always_editable", NULL);
          if (g_key_file_has_key (kf, "General", "terminal", NULL))
            settings.term = g_key_file_get_string (kf, "General", "terminal", NULL);
          if (g_key_file_has_key (kf, "General", "open_command", NULL))
            settings.open_cmd = g_key_file_get_string (kf, "General", "open_command", NULL);
          if (g_key_file_has_key (kf, "General", "date_format", NULL))
            settings.date_format = g_key_file_get_string (kf, "General", "date_format", NULL);
          if (g_key_file_has_key (kf, "General", "ignore_unknown_options", NULL))
            settings.ignore_unknown = g_key_file_get_boolean (kf, "General", "ignore_unknown_options", NULL);
          if (g_key_file_has_key (kf, "General", "max_tab", NULL))
            settings.max_tab = g_key_file_get_integer (kf, "General", "max_tab", NULL);

          settings.print_settings = gtk_print_settings_new_from_key_file (kf, NULL, NULL);
          settings.page_setup = gtk_page_setup_new_from_key_file (kf, NULL, NULL);
        }

      g_key_file_free (kf);
    }
  else
    write_settings ();

  g_free (filename);
}

void
write_settings (void)
{
  GKeyFile *kf;
  gchar *context;

  kf = g_key_file_new ();

  g_key_file_set_integer (kf, "General", "width", settings.width);
  g_key_file_set_comment (kf, "General", "width", " Default dialog width", NULL);
  g_key_file_set_integer (kf, "General", "height", settings.height);
  g_key_file_set_comment (kf, "General", "height", " Default dialog height", NULL);
  g_key_file_set_integer (kf, "General", "timeout", settings.timeout);
  g_key_file_set_comment (kf, "General", "timeout", " Default timeout (0 for no timeout)", NULL);
  g_key_file_set_string (kf, "General", "timeout_indicator", settings.to_indicator);
  g_key_file_set_comment (kf, "General", "timeout_indicator",
                          " Position of timeout indicator (top, bottom, left, right, none)", NULL);
  g_key_file_set_boolean (kf, "General", "show_remain", settings.show_remain);
  g_key_file_set_comment (kf, "General", "show_remain", " Show remain seconds in timeout indicator", NULL);
  g_key_file_set_boolean (kf, "General", "combo_always_editable", settings.combo_always_editable);
  g_key_file_set_comment (kf, "General", "combo_always_editable", " Combo-box in entry dialog is always editable", NULL);
  g_key_file_set_string (kf, "General", "terminal", settings.term);
  g_key_file_set_comment (kf, "General", "terminal", " Default terminal command (use %s for arguments placeholder)", NULL);
  g_key_file_set_string (kf, "General", "open_command", settings.open_cmd);
  g_key_file_set_comment (kf, "General", "open_command", " Default open command (use %s for arguments placeholder)", NULL);
  g_key_file_set_string (kf, "General", "date_format", settings.date_format);
  g_key_file_set_comment (kf, "General", "date_format", " Default date format (see strftime(3) for details)", NULL);
  g_key_file_set_boolean (kf, "General", "ignore_unknown_options", settings.ignore_unknown);
  g_key_file_set_comment (kf, "General", "ignore_unknown_options", " Ignore unknown command-line options", NULL);
  g_key_file_set_integer (kf, "General", "max_tab", settings.max_tab);
  g_key_file_set_comment (kf, "General", "max_tab", " Maximum number of tabs in notebook", NULL);

  if (settings.print_settings)
    gtk_print_settings_to_key_file (settings.print_settings, kf, NULL);
  if (settings.page_setup)
    gtk_page_setup_to_key_file (settings.page_setup, kf, NULL);

  context = g_key_file_to_data (kf, NULL, NULL);

  g_key_file_free (kf);

  if (g_mkdir_with_parents (g_get_user_config_dir (), 0755) != -1)
    {
      gchar *filename = g_build_filename (g_get_user_config_dir (), YAD_SETTINGS_FILE, NULL);
      g_file_set_contents (filename, context, -1, NULL);
      g_free (filename);
    }
  else
    g_printerr ("yad: cannot write settings file: %s\n", strerror (errno));

  g_free (context);
}

GdkPixbuf *
get_pixbuf (gchar * name, YadIconSize size)
{
  gint w, h;
  GdkPixbuf *pb = NULL;
  GError *err = NULL;

  if (g_file_test (name, G_FILE_TEST_EXISTS))
    {
      pb = gdk_pixbuf_new_from_file (name, &err);
      if (!pb)
        {
          g_printerr ("yad_get_pixbuf(): %s\n", err->message);
          g_error_free (err);
        }
    }
  else
    {
      if (size == YAD_BIG_ICON)
        gtk_icon_size_lookup (GTK_ICON_SIZE_DIALOG, &w, &h);
      else
        gtk_icon_size_lookup (GTK_ICON_SIZE_MENU, &w, &h);

      pb = gtk_icon_theme_load_icon (settings.icon_theme, name, MIN (w, h), GTK_ICON_LOOKUP_GENERIC_FALLBACK, NULL);
    }

  if (!pb)
    {
      if (size == YAD_BIG_ICON)
        pb = settings.big_fallback_image;
      else
        pb = settings.small_fallback_image;
    }

  return pb;
}

gchar *
get_color (GdkColor *c, guint64 alpha)
{
  gchar *cs;
  gchar *res = NULL;

  switch (options.color_data.mode)
    {
    case YAD_COLOR_HEX:
      cs = gdk_color_to_string (c);
      if (options.color_data.alpha)
        {
          if (options.color_data.extra)
            res = g_strdup_printf ("#%s%hx", cs + 1, alpha);
          else
            res = g_strdup_printf ("#%c%c%c%c%c%c%hx", cs[1], cs[2], cs[5], cs[6], cs[9], cs[10], alpha / 256);
        }
      else
        {
          if (options.color_data.extra)
            res = g_strdup_printf ("%s", cs);
          else
            res = g_strdup_printf ("#%c%c%c%c%c%c", cs[1], cs[2], cs[5], cs[6], cs[9], cs[10]);
        }
      g_free (cs);
      break;
    case YAD_COLOR_RGB:
      if (options.color_data.alpha)
        res = g_strdup_printf ("rgba(%.1f, %.1f, %.1f, %.1f)", (double) c->red / 255.0, (double) c->green / 255.0,
                               (double) c->blue / 255.0, (double) alpha / 255 / 255);
      else
        res = g_strdup_printf ("rgb(%.1f, %.1f, %.1f)", (double) c->red / 255.0, (double) c->green / 255.0,
                               (double) c->blue / 255.0);
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
      GdkPixbuf *pb;

      chs = g_checksum_new (G_CHECKSUM_MD5);
      g_checksum_update (chs, (const guchar *) uri, -1);
      /* first try to get preview from large thumbnail */
      file = g_strdup_printf ("%s/%s.png", large_path, g_checksum_get_string (chs));
      if (g_file_test (file, G_FILE_TEST_EXISTS))
        pb = gdk_pixbuf_new_from_file (file, NULL);
      else
        {
          /* try to get preview from normal thumbnail */
          g_free (file);
          file = g_strdup_printf ("%s/%s.png", normal_path, g_checksum_get_string (chs));
          if (g_file_test (file, G_FILE_TEST_EXISTS))
            pb = gdk_pixbuf_new_from_file (file, NULL);
          else
            {
              /* try to create it */
              g_free (file);
              file = g_filename_from_uri (uri, NULL, NULL);
              pb = gdk_pixbuf_new_from_file_at_size (file, 256, 256, NULL);
              g_free (file);
              if (pb)
                {
                  /* save thumbnail */
                  g_mkdir_with_parents (large_path, 0755);
                  file = g_strdup_printf ("%s/%s.png", large_path, g_checksum_get_string (chs));
                  gdk_pixbuf_save (pb, file, "png", NULL, NULL);
                }
            }
        }
      g_checksum_free (chs);

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

void
filechooser_mapped (GtkWidget *w, gpointer data)
{
  gtk_file_chooser_set_show_hidden (GTK_FILE_CHOOSER (w), options.common_data.show_hidden);
}

gchar **
split_arg (const gchar * str)
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
  int shmid, i;

  /* get shared memory */
  if (create)
    {
      if ((shmid = shmget (key, (settings.max_tab + 1) * sizeof (YadNTabs), IPC_CREAT | IPC_EXCL | 0644)) == -1)
        {
          g_printerr ("yad: cannot create shared memory for key %d: %s\n", key, strerror (errno));
          return NULL;
        }
    }
  else
    {
      if ((shmid = shmget (key, (settings.max_tab + 1) * sizeof (YadNTabs), 0)) == -1)
        {
          if (errno != ENOENT)
            g_printerr ("yad: cannot get shared memory for key %d: %s\n", key, strerror (errno));
          return NULL;
        }
    }

  /* attach shared memory */
  if ((t = shmat (shmid, NULL, 0)) == (YadNTabs *) - 1)
    {
      g_printerr ("yad: cannot attach shared memory for key %d: %s\n", key, strerror (errno));
      return NULL;
    }

  /* initialize memory */
  if (create)
    {
      for (i = 0; i < settings.max_tab + 1; i++)
        {
          t[i].pid = -1;
          t[i].xid = 0;
        }
      t[0].pid = shmid;
    }

  return t;
}

GtkWidget *
get_label (gchar * str, guint border)
{
  GtkWidget *a, *t, *i, *l;
  GtkStockItem it;
  gchar **vals;

  if (!str || !*str)
    return gtk_label_new (NULL);

  l = i = NULL;

  a = gtk_alignment_new (0.0, 0.5, 0, 0);
  gtk_container_set_border_width (GTK_CONTAINER (a), border);

#if !GTK_CHECK_VERSION(3,0,0)
  t = gtk_hbox_new (FALSE, 0);
#else
  t = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
#endif
  gtk_container_add (GTK_CONTAINER (a), t);

  vals = g_strsplit_set (str, options.common_data.item_separator, 3);
  if (gtk_stock_lookup (vals[0], &it))
    {
      l = gtk_label_new_with_mnemonic (it.label);
      gtk_misc_set_alignment (GTK_MISC (l), 0.0, 0.5);

      i = gtk_image_new_from_pixbuf (get_pixbuf (it.stock_id, YAD_SMALL_ICON));
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
          gtk_misc_set_alignment (GTK_MISC (l), 0.0, 0.5);
        }

      if (vals[1] && *vals[1])
        i = gtk_image_new_from_pixbuf (get_pixbuf (vals[1], YAD_SMALL_ICON));
    }

  if (i)
    gtk_box_pack_start (GTK_BOX (t), i, FALSE, FALSE, 1);
  if (l)
    gtk_box_pack_start (GTK_BOX (t), l, FALSE, FALSE, 1);

  /* !!! must check both 1 and 2 values for !NULL */
  if (vals[1] && vals[2] && *vals[2])
    {
      if (!options.data.no_markup)
        gtk_widget_set_tooltip_markup (t, vals[2]);
      else
        gtk_widget_set_tooltip_text (t, vals[2]);
    }

  g_strfreev (vals);

  gtk_widget_show_all (a);

  return a;
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

#ifdef HAVE_SPELL
void
show_langs ()
{
  GList *lng;

  for (lng = gtk_spell_checker_get_language_list (); lng; lng = lng->next)
    g_print ("%s\n", lng->data);
}
#endif

#ifdef HAVE_SOURCEVIEW
void
show_themes ()
{
  GtkSourceStyleSchemeManager *sm;
  const gchar **si;
  guint i = 0;

  sm = gtk_source_style_scheme_manager_get_default ();
  if ((si = (const gchar **) gtk_source_style_scheme_manager_get_scheme_ids (sm)) == NULL)
    return;

  while (si[i])
    {
      GtkSourceStyleScheme *s = gtk_source_style_scheme_manager_get_scheme (sm, si[i]);
      g_print ("%s\n", gtk_source_style_scheme_get_name (s));
      i++;
    }
}
#endif
