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

#include <string.h>
#include <stdlib.h>

#include "yad.h"

static GtkWidget *list_view;

static GHashTable *row_hash = NULL;

static gint fore_col, back_col, font_col;
static guint n_cols = 0;

static gulong select_hndl = 0;

static gchar *column_align = NULL;
static gchar *header_align = NULL;

static inline void
yad_list_add_row (GtkTreeStore *m, GtkTreeIter *it, gchar *row_id, gchar *par_id)
{
  GtkTreePath *row_path;
  GtkTreeIter pit, *parent = NULL;

  if (par_id && par_id[0])
    {
      GtkTreePath *par_path = g_hash_table_lookup (row_hash, par_id);
      if (par_path)
        {
          if (gtk_tree_model_get_iter (GTK_TREE_MODEL (m), &pit, par_path))
            parent = &pit;
        }
    }

  if (options.list_data.add_on_top)
    gtk_tree_store_prepend (m, it, parent);
  else
    gtk_tree_store_append (m, it, parent);

  row_path = gtk_tree_model_get_path (GTK_TREE_MODEL (m), it);
  if (row_id && row_id[0])
    g_hash_table_insert (row_hash, row_id, row_path);
  if (options.common_data.tail)
    gtk_tree_view_scroll_to_cell (GTK_TREE_VIEW (list_view), row_path, NULL, FALSE, 1.0, 1.0);
}

static gboolean
list_activate_cb (GtkWidget *widget, GdkEventKey *event, gpointer data)
{
  if (event->keyval == GDK_KEY_Return || event->keyval == GDK_KEY_KP_Enter)
    {
      if (options.list_data.dclick_action)
        {
          /* FIXME: check this under gtk-3.0 */
          if (event->state & GDK_CONTROL_MASK)
            {
              if (options.plug == -1)
                yad_exit (options.data.def_resp);
            }
          else
            return FALSE;
        }
      else
        {
          if (options.plug == -1)
            yad_exit (options.data.def_resp);
        }

      return TRUE;
    }

  return FALSE;
}

/* custom tooltip signal handler for no-markup mode */
static gboolean
tooltip_cb (GtkWidget *w, gint x, gint y, gboolean mode, GtkTooltip *tip, gpointer data)
{
  gchar *str;
  gint bx, by;
  GtkTreePath *path;
  GtkTreeIter iter;
  GtkTreeModel *model = gtk_tree_view_get_model (GTK_TREE_VIEW (w));

  gtk_tree_view_convert_widget_to_bin_window_coords (GTK_TREE_VIEW (w), x, y, &bx, &by);
  if (!gtk_tree_view_get_path_at_pos (GTK_TREE_VIEW (w), bx, by, &path, NULL, NULL, NULL))
    return FALSE;

  gtk_tree_model_get_iter (model, &iter, path);
  gtk_tree_path_free (path);

  gtk_tree_model_get (model, &iter, options.list_data.tooltip_column - 1, &str, -1);
  if (str)
    {
      gtk_tooltip_set_text (tip, str);
      return TRUE;
    }

  return FALSE;
}

static void
toggled_cb (GtkCellRendererToggle *cell, gchar *path_str, gpointer data)
{
  gint column;
  gboolean fixed;
  GtkTreeIter iter;
  GtkTreePath *path = gtk_tree_path_new_from_string (path_str);
  GtkTreeModel *model = gtk_tree_view_get_model (GTK_TREE_VIEW (list_view));

  column = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (cell), "column"));
  gtk_tree_model_get_iter (model, &iter, path);
  gtk_tree_model_get (model, &iter, column, &fixed, -1);

  fixed ^= 1;

  gtk_tree_store_set (GTK_TREE_STORE (model), &iter, column, fixed, -1);

  gtk_tree_path_free (path);
}

static gboolean
runtoggle (GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter, gpointer data)
{
  gint col = GPOINTER_TO_INT (data);
  gtk_tree_store_set (GTK_TREE_STORE (model), iter, col, FALSE, -1);
  return FALSE;
}

static void
rtoggled_cb (GtkCellRendererToggle *cell, gchar *path_str, gpointer data)
{
  gint column;
  GtkTreeIter iter;
  GtkTreePath *path = gtk_tree_path_new_from_string (path_str);
  GtkTreeModel *model = gtk_tree_view_get_model (GTK_TREE_VIEW (list_view));

  column = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (cell), "column"));

  gtk_tree_model_foreach (model, runtoggle, GINT_TO_POINTER (column));

  gtk_tree_model_get_iter (model, &iter, path);
  gtk_tree_store_set (GTK_TREE_STORE (model), &iter, column, TRUE, -1);

  gtk_tree_path_free (path);
}

static void
cell_edited_cb (GtkCellRendererText *cell, const gchar *path_string, const gchar *new_text, gpointer data)
{
  gint column;
  GtkTreeIter iter;
  GtkTreePath *path = gtk_tree_path_new_from_string (path_string);
  GtkTreeModel *model = gtk_tree_view_get_model (GTK_TREE_VIEW (list_view));
  YadColumn *col;

  column = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (cell), "column"));
  gtk_tree_model_get_iter (model, &iter, path);
  col = (YadColumn *) g_slist_nth_data (options.list_data.columns, column);

  if (col->type == YAD_COLUMN_NUM)
    gtk_tree_store_set (GTK_TREE_STORE (model), &iter, column, g_ascii_strtoll (new_text, NULL, 10), -1);
  else if (col->type == YAD_COLUMN_FLOAT)
    gtk_tree_store_set (GTK_TREE_STORE (model), &iter, column, g_ascii_strtod (new_text, NULL), -1);
  else
    gtk_tree_store_set (GTK_TREE_STORE (model), &iter, column, new_text, -1);

  gtk_tree_path_free (path);
}

static gboolean
regex_search (GtkTreeModel *model, gint col, const gchar *key, GtkTreeIter *iter, gpointer data)
{
  static GRegex *pattern = NULL;
  static guint pos = 0;
  gchar *str;

  if (key[pos])
    {
      if (pattern)
        g_regex_unref (pattern);
      pattern = g_regex_new (key, G_REGEX_CASELESS | G_REGEX_EXTENDED | G_REGEX_OPTIMIZE, G_REGEX_MATCH_NOTEMPTY, NULL);
      pos = strlen (key);
    }

  if (pattern)
    {
      gboolean ret;

      gtk_tree_model_get (model, iter, col, &str, -1);

      ret = g_regex_match (pattern, str, G_REGEX_MATCH_NOTEMPTY, NULL);
      /* if get it, clear key end position */
      if (!ret)
        pos = 0;

      return !ret;
    }
  else
    return TRUE;
}

static GtkTreeModel *
create_model ()
{
  GtkTreeStore *store;
  GType *ctypes;
  gint i;

  ctypes = g_new0 (GType, n_cols);

  if (options.list_data.checkbox)
    {
      YadColumn *col = (YadColumn *) g_slist_nth_data (options.list_data.columns, 0);
      col->type = YAD_COLUMN_CHECK;
    }
  else if (options.list_data.radiobox)
    {
      YadColumn *col = (YadColumn *) g_slist_nth_data (options.list_data.columns, 0);
      col->type = YAD_COLUMN_RADIO;
    }

  for (i = 0; i < n_cols; i++)
    {
      YadColumn *col = (YadColumn *) g_slist_nth_data (options.list_data.columns, i);

      switch (col->type)
        {
        case YAD_COLUMN_CHECK:
        case YAD_COLUMN_RADIO:
          ctypes[i] = G_TYPE_BOOLEAN;
          break;
        case YAD_COLUMN_NUM:
        case YAD_COLUMN_SIZE:
        case YAD_COLUMN_BAR:
          ctypes[i] = G_TYPE_INT64;
          break;
        case YAD_COLUMN_FLOAT:
          ctypes[i] = G_TYPE_DOUBLE;
          break;
        case YAD_COLUMN_IMAGE:
          ctypes[i] = GDK_TYPE_PIXBUF;
          break;
        case YAD_COLUMN_ATTR_FORE:
          ctypes[i] = G_TYPE_STRING;
          fore_col = i;
          break;
        case YAD_COLUMN_ATTR_BACK:
          ctypes[i] = G_TYPE_STRING;
          back_col = i;
          break;
        case YAD_COLUMN_ATTR_FONT:
          ctypes[i] = G_TYPE_STRING;
          font_col = i;
          break;
        default:
          ctypes[i] = G_TYPE_STRING;
          break;
        }
    }

  store = gtk_tree_store_newv (n_cols, ctypes);

  return GTK_TREE_MODEL (store);
}

static void
float_col_format (GtkTreeViewColumn *col, GtkCellRenderer *cell, GtkTreeModel *model,
                  GtkTreeIter *iter, gpointer data)
{
  gdouble val;
  gchar buf[20];

  gtk_tree_model_get (model, iter, GPOINTER_TO_INT (data), &val, -1);
  g_snprintf (buf, sizeof (buf), "%.*f", options.common_data.float_precision, val);
  g_object_set (cell, "text", buf, NULL);
}

static void
size_col_format (GtkTreeViewColumn *col, GtkCellRenderer *cell, GtkTreeModel *model,
                  GtkTreeIter *iter, gpointer data)
{
  guint64 val;
  gchar buf[20], *sz;

  gtk_tree_model_get (model, iter, GPOINTER_TO_INT (data), &val, -1);
#if GLIB_CHECK_VERSION(2,30,0)
  sz = g_format_size_full (val, options.common_data.size_fmt);
#elif  GLIB_CHECK_VERSION(2,16,0)
  sz = g_format_size_for_display (val);
#else
  sz = g_strdup_printf ("%d", val);
#endif
  g_snprintf (buf, sizeof (buf), "%s", sz);
  g_free (sz);
  g_object_set (cell, "text", buf, NULL);
}

static void
set_column_title (GtkTreeViewColumn *col, gchar *title)
{
  GtkWidget *lbl;
  gchar **str;

  str = g_strsplit (title, options.common_data.item_separator, 2);
  lbl = gtk_label_new (NULL);

  if (options.data.no_markup)
    {
      gtk_label_set_text (GTK_LABEL (lbl), str[0]);
      if (str[1] || options.list_data.header_tips)
        gtk_widget_set_tooltip_text (lbl, str[1] ? str[1] : str[0]);
    }
  else
    {
      gtk_label_set_markup (GTK_LABEL (lbl), str[0]);
      if (str[1] || options.list_data.header_tips)
        gtk_widget_set_tooltip_markup (lbl, str[1] ? str[1] : str[0]);
    }

  gtk_widget_show (lbl);
  gtk_tree_view_column_set_widget (col, lbl);
}

static void
add_columns ()
{
  gint i;
  GtkCellRenderer *renderer;
  GtkTreeViewColumn *column;

  for (i = 0; i < n_cols; i++)
    {
      YadColumn *col = (YadColumn *) g_slist_nth_data (options.list_data.columns, i);

      if (i == options.list_data.hide_column - 1 || col->type == YAD_COLUMN_HIDDEN ||
          i == fore_col || i == back_col || i == font_col)
        continue;

      switch (col->type)
        {
        case YAD_COLUMN_CHECK:
        case YAD_COLUMN_RADIO:
          renderer = gtk_cell_renderer_toggle_new ();
          column = gtk_tree_view_column_new_with_attributes (NULL, renderer, "active", i, NULL);
          set_column_title (column, col->name);
          if (back_col != -1)
            gtk_tree_view_column_add_attribute (column, renderer, "cell-background", back_col);
          if (col->type == YAD_COLUMN_RADIO)
            {
              gtk_cell_renderer_toggle_set_radio (GTK_CELL_RENDERER_TOGGLE (renderer), TRUE);
              g_signal_connect (renderer, "toggled", G_CALLBACK (rtoggled_cb), NULL);
            }
          else
            g_signal_connect (renderer, "toggled", G_CALLBACK (toggled_cb), NULL);
          break;
        case YAD_COLUMN_IMAGE:
          renderer = gtk_cell_renderer_pixbuf_new ();
          column = gtk_tree_view_column_new_with_attributes (NULL, renderer, "pixbuf", i, NULL);
          set_column_title (column, col->name);
          if (back_col != -1)
            gtk_tree_view_column_add_attribute (column, renderer, "cell-background", back_col);
          break;
        case YAD_COLUMN_NUM:
        case YAD_COLUMN_SIZE:
        case YAD_COLUMN_FLOAT:
          renderer = gtk_cell_renderer_text_new ();
          if (col->editable)
            {
              g_object_set (G_OBJECT (renderer), "editable", TRUE, NULL);
              g_signal_connect (renderer, "edited", G_CALLBACK (cell_edited_cb), NULL);
            }
          column = gtk_tree_view_column_new_with_attributes (NULL, renderer, "text", i, NULL);
          set_column_title (column, col->name);
          if (fore_col != -1)
            gtk_tree_view_column_add_attribute (column, renderer, "foreground", fore_col);
          if (back_col != -1)
            gtk_tree_view_column_add_attribute (column, renderer, "cell-background", back_col);
          if (font_col != -1)
            gtk_tree_view_column_add_attribute (column, renderer, "font", font_col);
          gtk_tree_view_column_set_sort_column_id (column, i);
          gtk_tree_view_column_set_resizable (column, TRUE);
          if (col->type == YAD_COLUMN_FLOAT)
            gtk_tree_view_column_set_cell_data_func (column, renderer, float_col_format, GINT_TO_POINTER (i), NULL);
          else if (col->type == YAD_COLUMN_SIZE)
            gtk_tree_view_column_set_cell_data_func (column, renderer, size_col_format, GINT_TO_POINTER (i), NULL);
          break;
        case YAD_COLUMN_BAR:
          renderer = gtk_cell_renderer_progress_new ();
          column = gtk_tree_view_column_new_with_attributes (NULL, renderer, "value", i, NULL);
          set_column_title (column, col->name);
          if (back_col != -1)
            gtk_tree_view_column_add_attribute (column, renderer, "cell-background", back_col);
          gtk_tree_view_column_set_sort_column_id (column, i);
          gtk_tree_view_column_set_resizable (column, TRUE);
          break;
        default:
          renderer = gtk_cell_renderer_text_new ();
          if (col->editable)
            {
              g_object_set (G_OBJECT (renderer), "editable", TRUE, NULL);
              g_signal_connect (renderer, "edited", G_CALLBACK (cell_edited_cb), NULL);
            }
          if (options.data.no_markup)
            column = gtk_tree_view_column_new_with_attributes (NULL, renderer, "text", i, NULL);
          else
            column = gtk_tree_view_column_new_with_attributes (NULL, renderer, "markup", i, NULL);
          set_column_title (column, col->name);
          if (col->ellipsize)
            g_object_set (G_OBJECT (renderer), "ellipsize", options.list_data.ellipsize, NULL);
          if (col->wrap)
            {
              g_object_set (G_OBJECT (renderer), "wrap-width", options.list_data.wrap_width, NULL);
              g_object_set (G_OBJECT (renderer), "wrap-mode", PANGO_WRAP_WORD_CHAR, NULL);
            }
          if (fore_col != -1)
            gtk_tree_view_column_add_attribute (column, renderer, "foreground", fore_col);
          if (back_col != -1)
            gtk_tree_view_column_add_attribute (column, renderer, "cell-background", back_col);
          if (font_col != -1)
            gtk_tree_view_column_add_attribute (column, renderer, "font", font_col);
          gtk_tree_view_column_set_sort_column_id (column, i);
          gtk_tree_view_column_set_resizable (column, TRUE);

          if (col->type == YAD_COLUMN_TIP)
            options.list_data.tooltip_column = i + 1;
          break;
        }

      gtk_cell_renderer_set_alignment (renderer, col->c_align, 0.5);
      g_object_set_data (G_OBJECT (renderer), "column", GINT_TO_POINTER (i));
      gtk_tree_view_append_column (GTK_TREE_VIEW (list_view), column);

      gtk_tree_view_column_set_clickable (column, options.list_data.clickable);
      gtk_tree_view_column_set_alignment (column, col->h_align);

      if (col->type != YAD_COLUMN_CHECK && col->type != YAD_COLUMN_IMAGE)
        {
          if (i == options.list_data.expand_column - 1 || options.list_data.expand_column == 0)
            gtk_tree_view_column_set_expand (column, TRUE);
        }
    }

  if (options.list_data.checkbox && !options.list_data.search_column)
    options.list_data.search_column += 1;
  if (options.list_data.search_column <= n_cols)
    {
      options.list_data.search_column -= 1;
      gtk_tree_view_set_search_column (GTK_TREE_VIEW (list_view), options.list_data.search_column);
    }
}

static void
cell_set_data (GtkTreeIter *it, guint num, gchar *data)
{
  GtkTreeModel *model = gtk_tree_view_get_model (GTK_TREE_VIEW (list_view));
  YadColumn *col = (YadColumn *) g_slist_nth_data (options.list_data.columns, num);

  switch (col->type)
    {
    case YAD_COLUMN_CHECK:
    case YAD_COLUMN_RADIO:
        gtk_tree_store_set (GTK_TREE_STORE (model), it, num, get_bool_val (data), -1);
      break;
    case YAD_COLUMN_NUM:
    case YAD_COLUMN_SIZE:
      gtk_tree_store_set (GTK_TREE_STORE (model), it, num, g_ascii_strtoll (data, NULL, 10), -1);
      break;
    case YAD_COLUMN_FLOAT:
      gtk_tree_store_set (GTK_TREE_STORE (model), it, num, g_ascii_strtod (data, NULL), -1);
      break;
    case YAD_COLUMN_BAR:
      {
        gint64 val = g_ascii_strtoll (data, NULL, 10);
        if (val < 0)
          val = 0;
        if (val > 100)
          val = 100;
        gtk_tree_store_set (GTK_TREE_STORE (model), it, num, val, -1);
        break;
      }
    case YAD_COLUMN_IMAGE:
      {
        GdkPixbuf *pb;

        if (g_file_test (data, G_FILE_TEST_EXISTS))
          pb = get_pixbuf (data, YAD_SMALL_ICON, FALSE);
        else
          pb = get_pixbuf (data, YAD_SMALL_ICON, TRUE);
        if (pb)
          {
            gtk_tree_store_set (GTK_TREE_STORE (model), it, num, pb, -1);
            g_object_unref (pb);
          }
        break;
      }
    default:
      if (data && *data)
        gtk_tree_store_set (GTK_TREE_STORE (model), it, num, data, -1);
      break;
    }
}

static gchar *
cell_get_data (GtkTreeIter *it, guint num)
{
  gchar *data = NULL;
  GtkTreeModel *model = gtk_tree_view_get_model (GTK_TREE_VIEW (list_view));
  YadColumn *col = (YadColumn *) g_slist_nth_data (options.list_data.columns, num);

  switch (col->type)
    {
    case YAD_COLUMN_CHECK:
    case YAD_COLUMN_RADIO:
      {
        gboolean bval;
        gtk_tree_model_get (model, it, num, &bval, -1);
        data = g_strdup (print_bool_val (bval));
        break;
      }
    case YAD_COLUMN_NUM:
    case YAD_COLUMN_SIZE:
    case YAD_COLUMN_BAR:
      {
        gint64 nval;
        gtk_tree_model_get (model, it, num, &nval, -1);
        data = g_strdup_printf ("%ld", (long) nval);
        break;
      }
    case YAD_COLUMN_FLOAT:
      {
        gdouble nval;
        gtk_tree_model_get (model, it, num, &nval, -1);
        data = g_strdup_printf ("%lf", nval);
        break;
      }
    case YAD_COLUMN_IMAGE:
      {
        data = g_strdup ("''");
        break;
      }
    default:
      {
        gchar *cval;
        gtk_tree_model_get (model, it, num, &cval, -1);
        if (cval)
          data = g_shell_quote (cval);
        break;
      }
    }

  return data;
}

static gboolean
handle_stdin (GIOChannel *channel, GIOCondition condition, gpointer data)
{
  static GtkTreeIter iter;
  static gulong column_count = 0;
  static gulong row_count = 0;
  static gboolean node_added = FALSE;
  GtkTreeModel *model = gtk_tree_view_get_model (GTK_TREE_VIEW (list_view));

  if ((condition == G_IO_IN) || (condition == G_IO_IN + G_IO_HUP))
    {
      GError *err = NULL;
      GString *string = g_string_new (NULL);

      while (channel->is_readable != TRUE)
        usleep (100);

      do
        {
          gint status;

          do
            {
              status = g_io_channel_read_line_string (channel, string, NULL, &err);
            }
          while (status == G_IO_STATUS_AGAIN);

          if (status != G_IO_STATUS_NORMAL)
            {
              if (err)
                {
                  g_printerr ("yad_list_handle_stdin(): %s\n", err->message);
                  g_error_free (err);
                  err = NULL;
                }
              /* stop handling */
              g_io_channel_shutdown (channel, TRUE, NULL);
              return FALSE;
            }

          strip_new_line (string->str);

          /* clear list if ^L received */
          if (string->str[0] == '\014')
            {
              GtkTreeSelection *sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (list_view));
              if (select_hndl)
                g_signal_handler_block (G_OBJECT (sel), select_hndl);
              gtk_tree_store_clear (GTK_TREE_STORE (model));
              row_count = column_count = 0;
              if (row_hash)
                g_hash_table_remove_all (row_hash);
              if (select_hndl)
                g_signal_handler_unblock (G_OBJECT (sel), select_hndl);
              continue;
            }

          if (row_count == 0 && column_count == 0)
            {
              if (options.list_data.tree_mode)
                {
                  if (!node_added)
                    {
                      gchar **ids = g_strsplit (string->str, ":", 2);
                      yad_list_add_row (GTK_TREE_STORE (model), &iter, ids[0], ids[1]);
                      node_added = TRUE;
                      continue;
                    }
                  else
                    node_added = FALSE;
                }
              else
                yad_list_add_row (GTK_TREE_STORE (model), &iter, NULL, NULL);
            }
          else if (column_count == n_cols)
            {
              /* We're starting a new row */
              if (options.list_data.tree_mode)
                {
                  if (!node_added)
                    {
                      gchar **ids = g_strsplit (string->str, ":", 2);
                      yad_list_add_row (GTK_TREE_STORE (model), &iter, ids[0], ids[1]);
                      node_added = TRUE;
                      continue;
                    }
                  else
                    node_added = FALSE;
                }
              else
                yad_list_add_row (GTK_TREE_STORE (model), &iter, NULL, NULL);
              column_count = 0;
              row_count++;
              if (options.list_data.limit && row_count >= options.list_data.limit)
                {
                  gtk_tree_model_get_iter_first (model, &iter);
                  gtk_tree_store_remove (GTK_TREE_STORE (model), &iter);
                }
            }

          cell_set_data (&iter, column_count, string->str);
          column_count++;
        }
      while (g_io_channel_get_buffer_condition (channel) == G_IO_IN);
      g_string_free (string, TRUE);
    }

  if (options.list_data.tree_expanded)
    gtk_tree_view_expand_all (GTK_TREE_VIEW (list_view));

  if ((condition != G_IO_IN) && (condition != G_IO_IN + G_IO_HUP))
    {
      g_io_channel_shutdown (channel, TRUE, NULL);
      return FALSE;
    }

  return TRUE;
}

static void
fill_data ()
{
  GtkTreeIter iter;
  GtkTreeStore *model = GTK_TREE_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (list_view)));
  GIOChannel *channel;

  if (options.extra_data && *options.extra_data)
    {
      gchar **args = options.extra_data;
      gint i = 0;

      gtk_widget_freeze_child_notify (list_view);

      while (args[i] != NULL)
        {
          gint j;

          if (options.list_data.tree_mode)
            {
              gchar **ids = g_strsplit (args[i], ":", 2);
              yad_list_add_row (model, &iter, ids[0], ids[1]);
              i++;
            }
          else
            yad_list_add_row (model, &iter, NULL, NULL);
          for (j = 0; j < n_cols; j++, i++)
            {
              if (args[i] == NULL)
                break;

              cell_set_data (&iter, j, args[i]);
            }
        }

      gtk_widget_thaw_child_notify (list_view);
    }

  if (options.common_data.listen || !(options.extra_data && *options.extra_data))
    {
      channel = g_io_channel_unix_new (0);
      g_io_channel_set_encoding (channel, NULL, NULL);
      g_io_channel_set_flags (channel, G_IO_FLAG_NONBLOCK, NULL);
      g_io_add_watch (channel, G_IO_IN | G_IO_HUP, handle_stdin, NULL);
    }
}

static gchar *
get_data_as_string (GtkTreeIter *iter)
{
  GString *str;
  gchar *res;
  guint i;

  str = g_string_new (NULL);

  for (i = 0; i < n_cols; i++)
    {
      gchar *val = cell_get_data (iter, i);
      if (val)
        {
          g_string_append_printf (str, "%s ", val);
          g_free (val);
        }
    }

  str->str[str->len-1] = '\0';
  res = str->str;
  g_string_free (str, FALSE);

  return res;
}

static void edit_row_cb (GtkMenuItem *item, gpointer data);

static void
double_click_cb (GtkTreeView *view, GtkTreePath *path, GtkTreeViewColumn *column, gpointer d)
{
  GtkTreeModel *model;
  GtkTreeIter iter;

  model = gtk_tree_view_get_model (view);

  if (options.list_data.dclick_action)
    {
      gchar *cmd, *args = NULL;

      if (gtk_tree_model_get_iter (model, &iter, path))
        args = get_data_as_string (&iter);
      else
        args = g_strdup ("");

      if (g_strstr_len (options.list_data.dclick_action, -1, "%s"))
        {
          static GRegex *regex = NULL;

          if (!regex)
            regex = g_regex_new ("\%s", G_REGEX_OPTIMIZE, 0, NULL);
          cmd = g_regex_replace_literal (regex, options.list_data.dclick_action, -1, 0, args, 0, NULL);
        }
      else
        cmd = g_strdup_printf ("%s %s", options.list_data.dclick_action, args);
      g_free (args);

      if (cmd[0] == '@')
        {
          gchar *data = NULL;
          gint exit;

          exit = run_command_sync (cmd + 1, &data);
          if (exit == 0)
            {
              gint i;
              gchar **lines = g_strsplit (data, "\n", 0);

              for (i = 0; i < n_cols; i++)
                {
                  if (lines[i] == NULL)
                    break;

                  cell_set_data (&iter, i, lines[i]);
                }
              g_strfreev (lines);
            }
          g_free (data);
        }
      else
        run_command_async (cmd);

      g_free (cmd);
    }
  else if (options.common_data.editable && options.list_data.row_action)
    edit_row_cb (NULL, NULL);
  else
    {
      if (options.list_data.checkbox)
        {
          if (gtk_tree_model_get_iter (model, &iter, path))
            {
              gboolean chk;

              gtk_tree_model_get (model, &iter, 0, &chk, -1);
              chk = !chk;
              gtk_tree_store_set (GTK_TREE_STORE (model), &iter, 0, chk, -1);
            }
        }
      else if (options.list_data.radiobox)
        {
          if (gtk_tree_model_get_iter (model, &iter, path))
            {
              gtk_tree_model_foreach (model, runtoggle, GINT_TO_POINTER (0));
              gtk_tree_store_set (GTK_TREE_STORE (model), &iter, 0, TRUE, -1);
            }
        }
      else if (options.plug == -1)
        yad_exit (options.data.def_resp);
    }
}

static void
select_cb (GtkTreeSelection *sel, gpointer data)
{
  GtkTreeModel *model;
  GtkTreeIter iter;
  gchar *cmd, *args;

  if (!gtk_tree_selection_get_selected (sel, &model, &iter))
    return;

  args = get_data_as_string (&iter);
  if (!args)
    args = g_strdup ("");

  if (g_strstr_len (options.list_data.select_action, -1, "%s"))
    {
      static GRegex *regex = NULL;

      if (!regex)
        regex = g_regex_new ("\%s", G_REGEX_OPTIMIZE, 0, NULL);
      cmd = g_regex_replace_literal (regex, options.list_data.select_action, -1, 0, args, 0, NULL);
    }
  else
    cmd = g_strdup_printf ("%s %s", options.list_data.select_action, args);
  g_free (args);

  run_command_async (cmd);

  g_free (cmd);
}

static void
add_row_cb (GtkMenuItem *item, gpointer data)
{
  GtkTreeModel *model;
  GtkTreeIter iter;
  gchar *cmd;

  model = gtk_tree_view_get_model (GTK_TREE_VIEW (list_view));
  if (g_object_get_data (G_OBJECT (item), "child") != NULL)
    {
      GtkTreeIter parent;
      GtkTreeSelection *sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (list_view));

      if (gtk_tree_selection_get_selected (sel, NULL, &parent))
        gtk_tree_store_append (GTK_TREE_STORE (model), &iter, &parent);
      else
        gtk_tree_store_append (GTK_TREE_STORE (model), &iter, NULL);
    }
  else
    gtk_tree_store_append (GTK_TREE_STORE (model), &iter, NULL);

  if (options.list_data.row_action)
    {
      gchar *out = NULL;
      gint exit;

      /* hide menu first */
      if (data)
        {
          gtk_menu_popdown (GTK_MENU (data));
          while (gtk_events_pending ())
            gtk_main_iteration ();
        }

      /* run command */
      cmd = g_strdup_printf ("%s add", options.list_data.row_action);
      exit = run_command_sync (cmd, &out);
      g_free (cmd);
      if (exit == 0)
        {
          guint i;
          gchar **lines = g_strsplit (out, "\n", 0);

          for (i = 0; i < n_cols; i++)
            {
              if (lines[i] == NULL)
                break;

              cell_set_data (&iter, i, lines[i]);
            }
          g_strfreev (lines);
        }
      g_free (out);
    }
}

static void
edit_row_cb (GtkMenuItem *item, gpointer data)
{
  GtkTreeIter iter;
  GtkTreeSelection *sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (list_view));

  if (!gtk_tree_selection_get_selected (sel, NULL, &iter))
    return;

  if (options.list_data.row_action)
    {
      gchar *cmd, *args, *out = NULL;
      gint exit;

      /* hide menu first */
      if (data)
        {
          gtk_menu_popdown (GTK_MENU (data));
          while (gtk_events_pending ())
            gtk_main_iteration ();
        }

      /* run command */
      args = get_data_as_string (&iter);
      cmd = g_strdup_printf ("%s edit %s", options.list_data.row_action, args);
      g_free (args);
      exit = run_command_sync (cmd, &out);
      g_free (cmd);
      if (exit == 0)
        {
          guint i;
          gchar **lines = g_strsplit (out, "\n", 0);

          for (i = 0; i < n_cols; i++)
            {
              if (lines[i] == NULL)
                break;

              cell_set_data (&iter, i, lines[i]);
            }
          g_strfreev (lines);
        }
      g_free (out);
    }
}

static void
del_row_cb (GtkMenuItem *item, gpointer data)
{
  GtkTreeIter iter;
  GtkTreeModel *model = gtk_tree_view_get_model (GTK_TREE_VIEW (list_view));
  GtkTreeSelection *sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (list_view));

  if (gtk_tree_selection_get_selected (sel, NULL, &iter))
    {
      if (options.list_data.row_action)
        {
          gchar *cmd, *args;
          gint exit;

          /* hide menu first */
          gtk_menu_popdown (GTK_MENU (data));
          while (gtk_events_pending ())
            gtk_main_iteration ();

          /* run command */
          args = get_data_as_string (&iter);
          cmd = g_strdup_printf ("%s del %s", options.list_data.row_action, args);
          g_free (args);
          exit = run_command_sync (cmd, NULL);
          g_free (cmd);
          if (exit == 0)
            gtk_tree_store_remove (GTK_TREE_STORE (model), &iter);
        }
      else
        gtk_tree_store_remove (GTK_TREE_STORE (model), &iter);
    }
}

static void
copy_row_cb (GtkMenuItem *item, gpointer data)
{
  GtkTreeIter iter;
  GtkTreeModel *model = gtk_tree_view_get_model (GTK_TREE_VIEW (list_view));
  GtkTreeSelection *sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (list_view));

  if (gtk_tree_selection_get_selected (sel, NULL, &iter))
    {
      GtkTreeIter new_iter, parent;
      gint i;

      if (gtk_tree_model_iter_parent (model, &parent, &iter))
        gtk_tree_store_insert_after (GTK_TREE_STORE (model), &new_iter, &parent, &iter);
      else
        gtk_tree_store_insert_after (GTK_TREE_STORE (model), &new_iter, NULL, &iter);

      for (i = 0; i < n_cols; i++)
        {
          GdkPixbuf *pb;
          gchar *tv;
          gint64 iv;
          gfloat fv;
          gboolean bv;
          YadColumn *col = (YadColumn *) g_slist_nth_data (options.list_data.columns, i);

          switch (col->type)
            {
            case YAD_COLUMN_CHECK:
            case YAD_COLUMN_RADIO:
              gtk_tree_model_get (model, &iter, i, &bv, -1);
              gtk_tree_store_set (GTK_TREE_STORE (model), &new_iter, i, bv, -1);
              break;
            case YAD_COLUMN_NUM:
            case YAD_COLUMN_SIZE:
            case YAD_COLUMN_BAR:
              gtk_tree_model_get (model, &iter, i, &iv, -1);
              gtk_tree_store_set (GTK_TREE_STORE (model), &new_iter, i, iv, -1);
              break;
            case YAD_COLUMN_FLOAT:
              gtk_tree_model_get (model, &iter, i, &fv, -1);
              gtk_tree_store_set (GTK_TREE_STORE (model), &new_iter, i, fv, -1);
              break;
            case YAD_COLUMN_IMAGE:
              gtk_tree_model_get (model, &iter, i, &pb, -1);
              gtk_tree_store_set (GTK_TREE_STORE (model), &new_iter, i, g_object_ref (pb), -1);
              break;
            default:
              gtk_tree_model_get (model, &iter, i, &tv, -1);
              gtk_tree_store_set (GTK_TREE_STORE (model), &new_iter, i, g_strdup (tv), -1);
              break;
            }
        }
    }
}

static void
move_row_up_cb (GtkMenuItem *item, gpointer data)
{
  GtkTreeIter iter;
  GtkTreeModel *model = gtk_tree_view_get_model (GTK_TREE_VIEW (list_view));
  GtkTreeSelection *sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (list_view));

  if (gtk_tree_selection_get_selected (sel, NULL, &iter))
    {
      GtkTreeIter *prev = gtk_tree_iter_copy (&iter);
      if (gtk_tree_model_iter_previous (model, prev))
        gtk_tree_store_move_before (GTK_TREE_STORE (model), &iter, prev);
      gtk_tree_iter_free (prev);
    }
}

static void
move_row_down_cb (GtkMenuItem *item, gpointer data)
{
  GtkTreeIter iter;
  GtkTreeModel *model = gtk_tree_view_get_model (GTK_TREE_VIEW (list_view));
  GtkTreeSelection *sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (list_view));

  if (gtk_tree_selection_get_selected (sel, NULL, &iter))
    {
      GtkTreeIter *next = gtk_tree_iter_copy (&iter);
      if (gtk_tree_model_iter_next (model, next))
        gtk_tree_store_move_after (GTK_TREE_STORE (model), &iter, next);
      gtk_tree_iter_free (next);
    }
}

static gboolean
popup_menu_cb (GtkWidget *w, GdkEventButton *ev, gpointer data)
{
  static GtkWidget *menu = NULL;
  if (ev->button == 3)
    {
      GtkWidget *item;

      if (menu == NULL)
        {
          menu = gtk_menu_new ();
          gtk_menu_set_reserve_toggle_size (GTK_MENU (menu), FALSE);

          item = gtk_menu_item_new_with_label (_("Add row"));
          gtk_widget_show (item);
          gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
          g_signal_connect (G_OBJECT (item), "activate", G_CALLBACK (add_row_cb), menu);

          if (options.list_data.tree_mode)
            {
              item = gtk_menu_item_new_with_label (_("Add child row"));
              gtk_widget_show (item);
              gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
              g_object_set_data (G_OBJECT (item), "child", "1");
              g_signal_connect (G_OBJECT (item), "activate", G_CALLBACK (add_row_cb), menu);
            }

          item = gtk_menu_item_new_with_label (_("Delete row"));
          gtk_widget_show (item);
          gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
          g_signal_connect (G_OBJECT (item), "activate", G_CALLBACK (del_row_cb), menu);

          if (options.list_data.row_action)
            {
              item = gtk_menu_item_new_with_label (_("Edit row"));
              gtk_widget_show (item);
              gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
              g_signal_connect (G_OBJECT (item), "activate", G_CALLBACK (edit_row_cb), menu);
            }

          item = gtk_menu_item_new_with_label (_("Duplicate row"));
          gtk_widget_show (item);
          gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
          g_signal_connect (G_OBJECT (item), "activate", G_CALLBACK (copy_row_cb), menu);

          item = gtk_separator_menu_item_new ();
          gtk_widget_show (item);
          gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);

          item = gtk_menu_item_new_with_label (_("Move row up"));
          gtk_widget_show (item);
          gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
          g_signal_connect (G_OBJECT (item), "activate", G_CALLBACK (move_row_up_cb), menu);

          item = gtk_menu_item_new_with_label (_("Move row down"));
          gtk_widget_show (item);
          gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
          g_signal_connect (G_OBJECT (item), "activate", G_CALLBACK (move_row_down_cb), menu);

          gtk_widget_show (menu);
        }
      gtk_menu_popup_at_pointer (GTK_MENU (menu), NULL);
    }
  return FALSE;
}

static gboolean
row_sep_func (GtkTreeModel *m, GtkTreeIter *it, gpointer data)
{
  gchar *name;

  if (!options.list_data.sep_value)
    return FALSE;

  gtk_tree_model_get (m, it, options.list_data.sep_column - 1, &name, -1);
  return (name && strcmp (name, options.list_data.sep_value) == 0);
}

static inline void
parse_cols_props ()
{
  GSList *c;
  guint i;

  /* set editable property for columns */
  if (options.common_data.editable)
    {
      if (options.list_data.editable_cols)
        {
          gchar **cnum;

          i = 0;
          cnum = g_strsplit (options.list_data.editable_cols, ",", -1);

          while (cnum[i])
            {
              gint num = atoi (cnum[i]);
              if (num)
                {
                  YadColumn *col = (YadColumn *) g_slist_nth_data (options.list_data.columns, num - 1);
                  if (col)
                    col->editable = TRUE;
                }
              i++;
            }
          g_strfreev (cnum);
        }
      else
        {
          for (c = options.list_data.columns; c; c = c->next)
            {
              YadColumn *col = (YadColumn *) c->data;
              col->editable = TRUE;
            }
        }
    }

  /* set wrap property for columns */
  if (options.list_data.wrap_width > 0)
    {
      if (options.list_data.wrap_cols)
        {
          gchar **cnum;

          i = 0;
          cnum = g_strsplit (options.list_data.wrap_cols, ",", -1);

          while (cnum[i])
            {
              gint num = atoi (cnum[i]);
              if (num)
                {
                  YadColumn *col = (YadColumn *) g_slist_nth_data (options.list_data.columns, num - 1);
                  if (col)
                    col->wrap = TRUE;
                }
              i++;
            }
          g_strfreev (cnum);
        }
      else
        {
          for (c = options.list_data.columns; c; c = c->next)
            {
              YadColumn *col = (YadColumn *) c->data;
              col->wrap = TRUE;
            }
        }
    }

  /* set ellipsize property for columns */
  if (options.list_data.ellipsize)
    {
      if (options.list_data.ellipsize_cols)
        {
          gchar **cnum;

          i = 0;
          cnum = g_strsplit (options.list_data.ellipsize_cols, ",", -1);

          while (cnum[i])
            {
              gint num = atoi (cnum[i]);
              if (num)
                {
                  YadColumn *col = (YadColumn *) g_slist_nth_data (options.list_data.columns, num - 1);
                  if (col)
                    col->ellipsize = TRUE;
                }
              i++;
            }
          g_strfreev (cnum);
        }
      else
        {
          for (c = options.list_data.columns; c; c = c->next)
            {
              YadColumn *col = (YadColumn *) c->data;
              col->ellipsize = TRUE;
            }
        }
    }

  /* set alignment */
  i = 0;
  for (c = options.list_data.columns; c; c = c->next)
    {
      YadColumn *col = (YadColumn *) c->data;

      if (column_align)
        {
          switch (column_align[i])
            {
            case 'l':
              col->c_align = 0.0;
              break;
            case 'r':
              col->c_align = 1.0;
              break;
            case 'c':
              col->c_align = 0.5;
              break;
            }
        }
      if (header_align)
        {
          switch (header_align[i])
            {
            case 'l':
              col->h_align = 0.0;
              break;
            case 'r':
              col->h_align = 1.0;
              break;
            case 'c':
              col->h_align = 0.5;
              break;
            }
        }

      i++;
    }
}

GtkWidget *
list_create_widget (GtkWidget *dlg)
{
  GtkWidget *w;
  GtkTreeModel *model;

  fore_col = back_col = font_col = -1;

  if (options.debug)
    {
      if (options.list_data.checkbox || options.list_data.radiobox)
        g_printerr (_("WARNING: You are use --checklist or --radiolist option. Those options obsoleted and will be removed in the future\n"));
    }

  n_cols = g_slist_length (options.list_data.columns);
  if (n_cols == 0)
    {
      g_printerr (_("No column titles specified for List dialog.\n"));
      return NULL;
    }

  if (options.list_data.tree_mode)
    row_hash = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, (GDestroyNotify) gtk_tree_path_free);

  /* set normalized alignment array for list keaders and columns content */
  if (options.list_data.col_align)
    {
      column_align = g_new0 (gchar, n_cols);
      strncpy (column_align, options.list_data.col_align, n_cols);
    }
  if (options.list_data.hdr_align)
    {
      header_align = g_new0 (gchar, n_cols);
      strncpy (header_align, options.list_data.hdr_align, n_cols);
    }

  parse_cols_props ();

  /* create widget */
  w = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (w), GTK_SHADOW_ETCHED_IN);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (w), options.data.hscroll_policy, options.data.vscroll_policy);

  model = create_model ();

  list_view = gtk_tree_view_new_with_model (model);
  gtk_widget_set_name (list_view, "yad-list-widget");
  gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (list_view), !options.list_data.no_headers);
  gtk_tree_view_set_grid_lines (GTK_TREE_VIEW (list_view), options.list_data.grid_lines);
  gtk_tree_view_set_reorderable (GTK_TREE_VIEW (list_view), options.common_data.editable);
  g_object_unref (model);

  gtk_container_add (GTK_CONTAINER (w), list_view);

  add_columns ();

  /* add popup menu */
  if (options.common_data.editable)
    g_signal_connect_swapped (G_OBJECT (list_view), "button_press_event", G_CALLBACK (popup_menu_cb), NULL);

  /* add tooltip column */
  if (options.list_data.tooltip_column > 0)
    {
      if (options.list_data.simple_tips || options.data.no_markup)
        {
          gtk_widget_set_has_tooltip (list_view, TRUE);
          g_signal_connect (G_OBJECT (list_view), "query-tooltip", G_CALLBACK (tooltip_cb), NULL);
        }
      else
        gtk_tree_view_set_tooltip_column (GTK_TREE_VIEW (list_view), options.list_data.tooltip_column - 1);
    }

  /* set search function for regex search */
  if (options.list_data.search_column != -1 && options.list_data.regex_search)
    {
      YadColumn *col = (YadColumn *) g_slist_nth_data (options.list_data.columns,
                                                       options.list_data.search_column);

      if (col->type == YAD_COLUMN_TEXT)
        gtk_tree_view_set_search_equal_func (GTK_TREE_VIEW (list_view), regex_search, NULL, NULL);
    }

  /* add row separator function */
  if (options.list_data.sep_column > 0)
    gtk_tree_view_set_row_separator_func (GTK_TREE_VIEW (list_view), row_sep_func, NULL, NULL);

  if (options.list_data.no_selection)
    {
      GtkTreeSelection *sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (list_view));
      gtk_tree_selection_set_mode (sel, GTK_SELECTION_NONE);
      gtk_widget_set_can_focus (list_view, FALSE);
    }
  else
    {
      GtkTreeSelection *sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (list_view));

      if (options.common_data.multi && !options.list_data.checkbox && !options.list_data.radiobox)
        gtk_tree_selection_set_mode (sel, GTK_SELECTION_MULTIPLE);

      if (!options.common_data.multi && options.list_data.select_action)
        select_hndl = g_signal_connect (G_OBJECT (sel), "changed", G_CALLBACK (select_cb), NULL);

      g_signal_connect (G_OBJECT (list_view), "row-activated", G_CALLBACK (double_click_cb), dlg);
      g_signal_connect (G_OBJECT (list_view), "key-press-event", G_CALLBACK (list_activate_cb), dlg);
    }

  /* load data */
  fill_data ();

  if (options.list_data.tree_expanded)
    gtk_tree_view_expand_all (GTK_TREE_VIEW (list_view));

  return w;
}

static void
print_col (GtkTreeModel *model, GtkTreeIter *iter, gint num)
{
  YadColumn *col = (YadColumn *) g_slist_nth_data (options.list_data.columns, num);

  /* don't print attributes */
  if (col->type == YAD_COLUMN_ATTR_FORE || col->type == YAD_COLUMN_ATTR_BACK || col->type == YAD_COLUMN_ATTR_FONT)
    return;

  switch (col->type)
    {
    case YAD_COLUMN_CHECK:
    case YAD_COLUMN_RADIO:
      {
        gboolean bval;
        gtk_tree_model_get (model, iter, num, &bval, -1);
        if (options.common_data.quoted_output)
          g_printf ("'%s'", print_bool_val (bval));
        else
          g_printf ("%s", print_bool_val (bval));
        break;
      }
    case YAD_COLUMN_NUM:
    case YAD_COLUMN_SIZE:
    case YAD_COLUMN_BAR:
      {
        gint64 nval;
        gtk_tree_model_get (model, iter, num, &nval, -1);
        if (options.common_data.quoted_output)
          g_printf ("'%ld'", (long) nval);
        else
          g_printf ("%ld", (long) nval);
        break;
      }
    case YAD_COLUMN_FLOAT:
      {
        gdouble nval;
        gtk_tree_model_get (model, iter, num, &nval, -1);
        if (options.common_data.quoted_output)
          g_printf ("'%.*f'", options.common_data.float_precision, nval);
        else
          g_printf ("%.*f", options.common_data.float_precision, nval);
        break;
      }
    case YAD_COLUMN_IMAGE:
      if (options.common_data.quoted_output)
        g_printf ("''");
      break;
    default:
      {
        gchar *val;
        gtk_tree_model_get (model, iter, num, &val, -1);
        if (options.common_data.quoted_output)
          {
            gchar *buf = g_shell_quote (val ? val : "");
            g_printf ("%s", buf);
            g_free (buf);
          }
        else
          g_printf ("%s", val ? val : "");
        break;
      }
    }
  g_printf ("%s", options.common_data.separator);
}

static void
print_selected (GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter, gpointer data)
{
  gint i,col;

  col = options.list_data.print_column;

  if (col && col <= n_cols)
    print_col (model, iter, col - 1);
  else
    {
      for (i = 0; i < n_cols; i++)
        print_col (model, iter, i);
    }
  g_printf ("\n");
}

static void
print_all (GtkTreeModel *model, GtkTreeIter *parent)
{
  GtkTreeIter iter;
  gint i;

  if (gtk_tree_model_iter_children (model, &iter, parent))
    {
      do
        {
          for (i = 0; i < n_cols; i++)
            print_col (model, &iter, i);
          g_printf ("\n");
          /* print children */
          print_all (model, &iter);
        }
      while (gtk_tree_model_iter_next (model, &iter));
    }
}

void
list_print_result (void)
{
  GtkTreeModel *model;
  gint col = options.list_data.print_column;

  model = gtk_tree_view_get_model (GTK_TREE_VIEW (list_view));

  if (options.list_data.print_all)
    {
      print_all (model, NULL);
      return;
    }

  if (options.list_data.checkbox || options.list_data.radiobox)
    {
      // don't check in cycle
      if (col > 0)
        {
          GtkTreeIter iter;

          if (gtk_tree_model_get_iter_first (model, &iter))
            {
              do
                {
                  gboolean chk;
                  gtk_tree_model_get (model, &iter, 0, &chk, -1);
                  if (chk)
                    {
                      print_col (model, &iter, col - 1);
                      g_printf ("\n");
                    }
                }
              while (gtk_tree_model_iter_next (model, &iter));
            }
        }
      else
        {
          GtkTreeIter iter;

          if (gtk_tree_model_get_iter_first (model, &iter))
            {
              do
                {
                  gboolean chk;
                  gtk_tree_model_get (model, &iter, 0, &chk, -1);
                  if (chk)
                    {
                      gint i;
                      for (i = 0; i < n_cols; i++)
                        print_col (model, &iter, i);
                      g_printf ("\n");
                    }
                }
              while (gtk_tree_model_iter_next (model, &iter));
            }
        }
    }
  else
    {
      GtkTreeSelection *sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (list_view));
      gtk_tree_selection_selected_foreach (sel, print_selected, NULL);
    }
}
