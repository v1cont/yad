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

#include <string.h>
#include <stdlib.h>

#include <glib/gprintf.h>

#include "yad.h"

static GtkWidget *list_view;

static gint fore_col, back_col, font_col;

static gulong select_hndl = 0;

static inline void
yad_list_add_row (GtkListStore *m, GtkTreeIter *it)
{
  if (options.list_data.add_on_top)
    gtk_list_store_prepend (m, it);
  else
    gtk_list_store_append (m, it);

  if (options.common_data.tail)
    gtk_tree_view_scroll_to_cell (GTK_TREE_VIEW (list_view), gtk_tree_model_get_path (GTK_TREE_MODEL (m), it),
                                  NULL, FALSE, 1.0, 1.0);
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

  gtk_list_store_set (GTK_LIST_STORE (model), &iter, column, fixed, -1);

  gtk_tree_path_free (path);
}

static gboolean
runtoggle (GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter, gpointer data)
{
  gint col = GPOINTER_TO_INT (data);
  gtk_list_store_set (GTK_LIST_STORE (model), iter, col, FALSE, -1);
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
  gtk_list_store_set (GTK_LIST_STORE (model), &iter, column, TRUE, -1);

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
    gtk_list_store_set (GTK_LIST_STORE (model), &iter, column, g_ascii_strtoll (new_text, NULL, 10), -1);
  else if (col->type == YAD_COLUMN_FLOAT)
    gtk_list_store_set (GTK_LIST_STORE (model), &iter, column, g_ascii_strtod (new_text, NULL), -1);
  else
    gtk_list_store_set (GTK_LIST_STORE (model), &iter, column, new_text, -1);

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
create_model (gint n_columns)
{
  GtkListStore *store;
  GType *ctypes;
  gint i;

  ctypes = g_new0 (GType, n_columns);

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

  for (i = 0; i < n_columns; i++)
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

  store = gtk_list_store_newv (n_columns, ctypes);

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
add_columns (gint n_columns)
{
  gint i;
  GtkCellRenderer *renderer;
  GtkTreeViewColumn *column;

  for (i = 0; i < n_columns; i++)
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
          column = gtk_tree_view_column_new_with_attributes (col->name, renderer, "active", i, NULL);
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
          column = gtk_tree_view_column_new_with_attributes (col->name, renderer, "pixbuf", i, NULL);
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
          column = gtk_tree_view_column_new_with_attributes (col->name, renderer, "text", i, NULL);
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
          column = gtk_tree_view_column_new_with_attributes (col->name, renderer, "value", i, NULL);
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
            column = gtk_tree_view_column_new_with_attributes (col->name, renderer, "text", i, NULL);
          else
            column = gtk_tree_view_column_new_with_attributes (col->name, renderer, "markup", i, NULL);
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
          break;
        }
      g_object_set_data (G_OBJECT (renderer), "column", GINT_TO_POINTER (i));
      gtk_tree_view_append_column (GTK_TREE_VIEW (list_view), column);

      gtk_tree_view_column_set_clickable (column, options.list_data.clickable);

      if (col->type != YAD_COLUMN_CHECK && col->type != YAD_COLUMN_IMAGE)
        {
          if (i == options.list_data.expand_column - 1 || options.list_data.expand_column == 0)
            gtk_tree_view_column_set_expand (column, TRUE);
        }
    }

  if (options.list_data.checkbox && !options.list_data.search_column)
    options.list_data.search_column += 1;
  if (options.list_data.search_column <= n_columns)
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
        gtk_list_store_set (GTK_LIST_STORE (model), it, num, get_bool_val (data), -1);
      break;
    case YAD_COLUMN_NUM:
    case YAD_COLUMN_SIZE:
      gtk_list_store_set (GTK_LIST_STORE (model), it, num, g_ascii_strtoll (data, NULL, 10), -1);
      break;
    case YAD_COLUMN_FLOAT:
      gtk_list_store_set (GTK_LIST_STORE (model), it, num, g_ascii_strtod (data, NULL), -1);
      break;
    case YAD_COLUMN_BAR:
      {
        gint64 val = g_ascii_strtoll (data, NULL, 10);
        if (val < 0)
          val = 0;
        if (val > 100)
          val = 100;
        gtk_list_store_set (GTK_LIST_STORE (model), it, num, val, -1);
        break;
      }
    case YAD_COLUMN_IMAGE:
      {
        GdkPixbuf *pb = get_pixbuf (data, YAD_SMALL_ICON, FALSE);
        if (pb)
          {
            gtk_list_store_set (GTK_LIST_STORE (model), it, num, pb, -1);
            g_object_unref (pb);
          }
        break;
      }
    default:
      if (data && *data)
        gtk_list_store_set (GTK_LIST_STORE (model), it, num, data, -1);
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
handle_stdin (GIOChannel * channel, GIOCondition condition, gpointer data)
{
  static GtkTreeIter iter;
  static gint column_count = 0;
  static gint row_count = 0;
  gint n_columns = GPOINTER_TO_INT (data);
  GtkTreeModel *model = gtk_tree_view_get_model (GTK_TREE_VIEW (list_view));

  if ((condition == G_IO_IN) || (condition == G_IO_IN + G_IO_HUP))
    {
      GError *err = NULL;
      GString *string = g_string_new (NULL);

      while (channel->is_readable != TRUE);

      do
        {
          gint status;

          do
            {
              status = g_io_channel_read_line_string (channel, string, NULL, &err);

              while (gtk_events_pending ())
                gtk_main_iteration ();
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
              gtk_list_store_clear (GTK_LIST_STORE (model));
              row_count = column_count = 0;
              if (select_hndl)
                g_signal_handler_unblock (G_OBJECT (sel), select_hndl);
              continue;
            }

          if (row_count == 0 && column_count == 0)
            yad_list_add_row (GTK_LIST_STORE (model), &iter);
          else if (column_count == n_columns)
            {
              /* We're starting a new row */
              column_count = 0;
              row_count++;
              if (options.list_data.limit && row_count >= options.list_data.limit)
                {
                  gtk_tree_model_get_iter_first (model, &iter);
                  gtk_list_store_remove (GTK_LIST_STORE (model), &iter);
                }
              yad_list_add_row (GTK_LIST_STORE (model), &iter);
            }

          cell_set_data (&iter, column_count, string->str);
          column_count++;
        }
      while (g_io_channel_get_buffer_condition (channel) == G_IO_IN);
      g_string_free (string, TRUE);
    }

  if ((condition != G_IO_IN) && (condition != G_IO_IN + G_IO_HUP))
    {
      g_io_channel_shutdown (channel, TRUE, NULL);
      return FALSE;
    }

  return TRUE;
}

static void
fill_data (gint n_columns)
{
  GtkTreeIter iter;
  GtkListStore *model = GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (list_view)));
  GIOChannel *channel;

  if (options.extra_data && *options.extra_data)
    {
      gchar **args = options.extra_data;
      gint i = 0;

      gtk_widget_freeze_child_notify (list_view);

      while (args[i] != NULL)
        {
          gint j;

          yad_list_add_row (model, &iter);
          for (j = 0; j < n_columns; j++, i++)
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
      g_io_add_watch (channel, G_IO_IN | G_IO_HUP, handle_stdin, GINT_TO_POINTER (n_columns));
    }
}

static void
double_click_cb (GtkTreeView * view, GtkTreePath * path, GtkTreeViewColumn * column, gpointer data)
{
  GtkTreeModel *model;
  GtkTreeIter iter;

  model = gtk_tree_view_get_model (view);

  if (options.list_data.dclick_action)
    {
      gchar *cmd;
      GString *args;
      guint n_cols;

      args = g_string_new ("");

      n_cols = gtk_tree_model_get_n_columns (model);

      if (gtk_tree_model_get_iter (model, &iter, path))
        {
          gint i;

          for (i = 0; i < n_cols; i++)
            {
              gchar *val = cell_get_data (&iter, i);
              if (val)
                {
                  g_string_append_printf (args, " %s", val);
                  g_free (val);
                }
            }
        }

      if (g_strstr_len (options.list_data.dclick_action, -1, "%s"))
        {
          static GRegex *regex = NULL;

          if (!regex)
            regex = g_regex_new ("\%s", G_REGEX_OPTIMIZE, 0, NULL);
          cmd = g_regex_replace_literal (regex, options.list_data.dclick_action, -1, 0, args->str, 0, NULL);
        }
      else
        cmd = g_strdup_printf ("%s %s", options.list_data.dclick_action, args->str);
      g_string_free (args, TRUE);

      if (cmd[0] == '@')
        {
          gchar *data = NULL;
          gint exit;

          g_spawn_command_line_sync (cmd + 1, &data, NULL, &exit, NULL);
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
        g_spawn_command_line_async (cmd, NULL);

      g_free (cmd);
    }
  else
    {
      if (options.list_data.checkbox)
        {
          if (gtk_tree_model_get_iter (model, &iter, path))
            {
              gboolean chk;

              gtk_tree_model_get (model, &iter, 0, &chk, -1);
              chk = !chk;
              gtk_list_store_set (GTK_LIST_STORE (model), &iter, 0, chk, -1);
            }
        }
      else if (options.list_data.radiobox)
        {
          if (gtk_tree_model_get_iter (model, &iter, path))
            {
              gtk_tree_model_foreach (model, runtoggle, GINT_TO_POINTER (0));
              gtk_list_store_set (GTK_LIST_STORE (model), &iter, 0, TRUE, -1);
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
  gchar *cmd;
  GString *args;
  guint i, n_cols;

  if (!gtk_tree_selection_get_selected (sel, &model, &iter))
    return;

  args = g_string_new ("");
  n_cols = gtk_tree_model_get_n_columns (model);

  for (i = 0; i < n_cols; i++)
    {
      gchar *val = cell_get_data (&iter, i);
      if (val)
        {
          g_string_append_printf (args, " %s", val);
          g_free (val);
        }
    }

  if (g_strstr_len (options.list_data.select_action, -1, "%s"))
    {
      static GRegex *regex = NULL;

      if (!regex)
        regex = g_regex_new ("\%s", G_REGEX_OPTIMIZE, 0, NULL);
      cmd = g_regex_replace_literal (regex, options.list_data.select_action, -1, 0, args->str, 0, NULL);
    }
  else
    cmd = g_strdup_printf ("%s %s", options.list_data.select_action, args->str);
  g_string_free (args, TRUE);

  g_spawn_command_line_async (cmd, NULL);

  g_free (cmd);
}

static void
add_row_cb (GtkMenuItem * item, gpointer data)
{
  GtkTreeModel *model;
  GtkTreeIter iter;

  model = gtk_tree_view_get_model (GTK_TREE_VIEW (list_view));
  yad_list_add_row (GTK_LIST_STORE (model), &iter);

  if (options.list_data.add_action)
    {
      gchar *out = NULL;
      gint exit;

      /* hide menu first */
      gtk_menu_popdown (GTK_MENU (data));
      while (gtk_events_pending ())
        gtk_main_iteration ();

      /* run command */
      g_spawn_command_line_sync (options.list_data.add_action, &out, NULL, &exit, NULL);
      if (exit == 0)
        {
          guint i, n_cols = gtk_tree_model_get_n_columns (model);
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
del_row_cb (GtkMenuItem * item, gpointer data)
{
  GtkTreeIter iter;
  GtkTreeModel *model = gtk_tree_view_get_model (GTK_TREE_VIEW (list_view));
  GtkTreeSelection *sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (list_view));

  if (gtk_tree_selection_get_selected (sel, NULL, &iter))
    gtk_list_store_remove (GTK_LIST_STORE (model), &iter);
}

static void
copy_row_cb (GtkMenuItem * item, gpointer data)
{
  GtkTreeIter iter, new_iter;
  GtkTreeModel *model = gtk_tree_view_get_model (GTK_TREE_VIEW (list_view));
  GtkTreeSelection *sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (list_view));

  if (gtk_tree_selection_get_selected (sel, NULL, &iter))
    {
      gint i, n_columns;

      /* decrease by 1 due to last ellipsize column */
      n_columns = gtk_tree_model_get_n_columns (model);

      gtk_list_store_insert_after (GTK_LIST_STORE (model), &new_iter, &iter);

      for (i = 0; i < n_columns; i++)
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
              gtk_list_store_set (GTK_LIST_STORE (model), &new_iter, i, bv, -1);
              break;
            case YAD_COLUMN_NUM:
            case YAD_COLUMN_SIZE:
            case YAD_COLUMN_BAR:
              gtk_tree_model_get (model, &iter, i, &iv, -1);
              gtk_list_store_set (GTK_LIST_STORE (model), &new_iter, i, iv, -1);
              break;
            case YAD_COLUMN_FLOAT:
              gtk_tree_model_get (model, &iter, i, &fv, -1);
              gtk_list_store_set (GTK_LIST_STORE (model), &new_iter, i, fv, -1);
              break;
            case YAD_COLUMN_IMAGE:
              gtk_tree_model_get (model, &iter, i, &pb, -1);
              gtk_list_store_set (GTK_LIST_STORE (model), &new_iter, i, pb, -1);
              break;
            default:
              gtk_tree_model_get (model, &iter, i, &tv, -1);
              gtk_list_store_set (GTK_LIST_STORE (model), &new_iter, i, tv, -1);
              break;
            }
        }
    }
}

static gboolean
popup_menu_cb (GtkWidget * w, GdkEventButton * ev, gpointer data)
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

          item = gtk_menu_item_new_with_label (_("Delete row"));
          gtk_widget_show (item);
          gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
          g_signal_connect (G_OBJECT (item), "activate", G_CALLBACK (del_row_cb), menu);

          item = gtk_menu_item_new_with_label (_("Duplicate row"));
          gtk_widget_show (item);
          gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
          g_signal_connect (G_OBJECT (item), "activate", G_CALLBACK (copy_row_cb), menu);

          gtk_widget_show (menu);
        }
      gtk_menu_popup_at_pointer (GTK_MENU (menu), NULL);
    }
  return FALSE;
}

static gboolean
row_sep_func (GtkTreeModel * m, GtkTreeIter * it, gpointer data)
{
  gchar *name;

  if (!options.list_data.sep_value)
    return FALSE;

  gtk_tree_model_get (m, it, options.list_data.sep_column - 1, &name, -1);
  return (strcmp (name, options.list_data.sep_value) == 0);
}

static inline void
parse_cols_props ()
{
  /* set editable property for columns */
  if (options.common_data.editable)
    {
      if (options.list_data.editable_cols)
        {
          gchar **cnum;
          guint i = 0;

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
          GSList *c;
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
          guint i = 0;

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
          GSList *c;
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
          guint i = 0;

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
          GSList *c;
          for (c = options.list_data.columns; c; c = c->next)
            {
              YadColumn *col = (YadColumn *) c->data;
              col->ellipsize = TRUE;
            }
        }
    }
}

GtkWidget *
list_create_widget (GtkWidget * dlg)
{
  GtkWidget *w;
  GtkTreeModel *model;
  gint n_columns;

  fore_col = back_col = font_col = -1;

  n_columns = g_slist_length (options.list_data.columns);

  if (n_columns == 0)
    {
      g_printerr (_("No column titles specified for List dialog.\n"));
      return NULL;
    }

  parse_cols_props ();

  /* create widget */
  w = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (w), GTK_SHADOW_ETCHED_IN);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (w), options.hscroll_policy, options.vscroll_policy);

  model = create_model (n_columns);

  list_view = gtk_tree_view_new_with_model (model);
  gtk_widget_set_name (list_view, "yad-list-widget");
  gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (list_view), !options.list_data.no_headers);
  gtk_tree_view_set_grid_lines (GTK_TREE_VIEW (list_view), options.list_data.grid_lines);
  gtk_tree_view_set_reorderable (GTK_TREE_VIEW (list_view), options.common_data.editable);
  g_object_unref (model);

  gtk_container_add (GTK_CONTAINER (w), list_view);

  add_columns (n_columns);

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
  fill_data (n_columns);

  return w;
}

static void
print_col (GtkTreeModel * model, GtkTreeIter * iter, gint num)
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
            gchar *buf = g_shell_quote (val);
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
print_selected (GtkTreeModel * model, GtkTreePath * path, GtkTreeIter * iter, gpointer data)
{
  gint i, n_cols, col;

  col = options.list_data.print_column;
  n_cols = gtk_tree_model_get_n_columns (model);

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
print_all (GtkTreeModel * model)
{
  GtkTreeIter iter;
  gint i, n_cols = gtk_tree_model_get_n_columns (model);

  if (gtk_tree_model_get_iter_first (model, &iter))
    {
      do
        {
          for (i = 0; i < n_cols; i++)
            print_col (model, &iter, i);
          g_printf ("\n");
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
      print_all (model);
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
                      for (i = 0; i < gtk_tree_model_get_n_columns (model); i++)
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
