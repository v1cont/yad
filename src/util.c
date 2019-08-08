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
 * Copyright (C) 2008-2019, Victor Ananjevsky <ananasik@gmail.com>
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

  if (g_file_test (name, G_FILE_TEST_EXISTS))
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
  int shmid, i, max_tab;

  /* get shared memory */
  max_tab = g_settings_get_int (settings, "max-tab") + 1;
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
  if ((t = shmat (shmid, NULL, 0)) == (YadNTabs *) - 1)
    {
      g_printerr ("yad: cannot attach shared memory for key %d: %s\n", key, strerror (errno));
      return NULL;
    }

  /* initialize memory */
  if (create)
    {
      for (i = 0; i < max_tab; i++)
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

  t = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
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

      i++;
      while (geom[i] && geom[i] != '-' && geom[i] != '+') i++;

      if (!geom[i])
        return;

      ptr = geom + i;
      y = atoi (ptr);
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
  if (!str && !str[0])
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
  gchar *ret = "";

  switch (options.common_data.bool_fmt)
    {
    case YAD_BOOL_FMT_UT:
      ret = val ? "TRUE" : "FALSE";
      break;
    case YAD_BOOL_FMT_UY:
      ret = val ? "YES" : "NO";
      break;
    case YAD_BOOL_FMT_UO:
      ret = val ? "ON" : "OFF";
      break;
    case YAD_BOOL_FMT_LT:
      ret = val ? "true" : "false";
      break;
    case YAD_BOOL_FMT_LY:
      ret = val ? "yes" : "no";
      break;
    case YAD_BOOL_FMT_LO:
      ret = val ? "on" : "off";
      break;
    case YAD_BOOL_FMT_1:
      ret = val ? "1" : "0";
      break;
    }

  return ret;
}

#ifdef HAVE_SPELL
void
show_langs ()
{
  const GList *lng;

  for (lng = gspell_language_get_available (); lng; lng = lng->next)
    {
      const GspellLanguage *l = lng->data;
      g_print ("%s\n", gspell_language_get_code (l));
    }
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
