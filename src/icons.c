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

#include "yad.h"

static GtkWidget *icon_view;
static GtkListStore *store;

enum {
  COL_FILENAME = 0,
  COL_NAME,
  COL_TOOLTIP,
  COL_PIXBUF,
  COL_COMMAND,
  COL_TERM,
  NUM_COLS
};

enum {
  TYPE_APP,
  TYPE_LINK
};

typedef struct {
  gchar *name;
  gchar *comment;
  GdkPixbuf *pixbuf;
  gchar *command;
  gboolean in_term;
} DEntry;

static void
select_cb (GObject * obj, gpointer data)
{
  static gboolean first_time = TRUE;

  if (!options.icons_data.compact)
    {
      GList *sel = gtk_icon_view_get_selected_items (GTK_ICON_VIEW (icon_view));
      if (sel)
        gtk_icon_view_item_activated (GTK_ICON_VIEW (icon_view), (GtkTreePath *) sel->data);
      g_list_foreach (sel, (GFunc) gtk_tree_path_free, NULL);
      g_list_free (sel);
    }
  else
    {
      GtkTreeIter iter;
      GtkTreeSelection *sel = (GtkTreeSelection *) obj;

      if (first_time)
        {
          /* don't activate item when dialog is appear and clear the selection */
          first_time = FALSE;
          gtk_tree_selection_unselect_all (sel);
          return;
        }

      if (gtk_tree_selection_get_selected (sel, NULL, &iter))
        {
          GtkTreeModel *model;
          GtkTreePath *path;

          model = gtk_tree_view_get_model (GTK_TREE_VIEW (icon_view));
          path = gtk_tree_model_get_path (model, &iter);

          gtk_tree_view_row_activated (GTK_TREE_VIEW (icon_view), path, (GtkTreeViewColumn *) data);
        }
    }
}

static void
activate_cb (GtkWidget * view, GtkTreePath * path, gpointer data)
{
  GtkTreeIter iter;
  GtkTreeModel *model;
  gchar *cmd;
  gboolean in_term;

  if (!path)
    return;

  if (!options.icons_data.compact)
    model = gtk_icon_view_get_model (GTK_ICON_VIEW (view));
  else
    model = gtk_tree_view_get_model (GTK_TREE_VIEW (view));

  gtk_tree_model_get_iter (model, &iter, path);
  gtk_tree_model_get (model, &iter, COL_COMMAND, &cmd, COL_TERM, &in_term, -1);

  if (cmd && cmd[0])
    {
      if (in_term)
        {
          gchar *tcmd;

          tcmd = g_strdup_printf (options.icons_data.term, cmd);
          g_spawn_command_line_async (tcmd, NULL);
          g_free (tcmd);
        }
      else
        g_spawn_command_line_async (cmd, NULL);
    }
}

static gboolean
handle_stdin (GIOChannel * channel, GIOCondition condition, gpointer data)
{
  static GtkTreeIter iter;
  static gint column_count = 1;
  static gint row_count = 0;
  static gboolean first_time = TRUE;
  GtkTreeModel *model;

  if (!options.icons_data.compact)
    model = gtk_icon_view_get_model (GTK_ICON_VIEW (icon_view));
  else
    model = gtk_tree_view_get_model (GTK_TREE_VIEW (icon_view));

  if (first_time)
    {
      first_time = FALSE;
      gtk_list_store_append (GTK_LIST_STORE (model), &iter);
      gtk_list_store_set (GTK_LIST_STORE (model), &iter, COL_FILENAME, "", -1);
    }

  if ((condition == G_IO_IN) || (condition == G_IO_IN + G_IO_HUP))
    {
      GError *err = NULL;
      GString *string = g_string_new (NULL);

      while (channel->is_readable != TRUE);

      do
        {
          GdkPixbuf *pb;
          gint status;

          do
            {
              status = g_io_channel_read_line_string (channel, string, NULL, &err);

              while (gtk_events_pending ())
                gtk_main_iteration ();
            }
          while (status == G_IO_STATUS_AGAIN);
          strip_new_line (string->str);

          if (status != G_IO_STATUS_NORMAL)
            {
              if (err)
                {
                  g_printerr ("yad_icons_handle_stdin(): %s\n", err->message);
                  g_error_free (err);
                  err = NULL;
                }
              /* stop handling */
              g_io_channel_shutdown (channel, TRUE, NULL);
              return FALSE;
            }

          /* clear list if ^L received */
          if (string->str[0] == '\014')
            {
              gtk_list_store_clear (GTK_LIST_STORE (model));
              row_count = 0;
              column_count = 1;
              continue;
            }

          if (column_count == NUM_COLS)
            {
              /* We're starting a new row */
              column_count = 1;
              row_count++;
              gtk_list_store_append (GTK_LIST_STORE (model), &iter);
              gtk_list_store_set (GTK_LIST_STORE (model), &iter, COL_FILENAME, "", -1);
            }

          switch (column_count)
            {
            case COL_NAME:
            case COL_COMMAND:
              gtk_list_store_set (GTK_LIST_STORE (model), &iter, column_count, string->str, -1);
              break;
            case COL_TOOLTIP:
              {
                gchar *buf = g_markup_escape_text (string->str, -1);
                gtk_list_store_set (GTK_LIST_STORE (model), &iter, column_count, buf, -1);
                g_free (buf);
                break;
              }
            case COL_PIXBUF:
              if (options.icons_data.compact)
                if (*string->str)
                  pb = get_pixbuf (string->str, YAD_SMALL_ICON);
                else
                  pb = NULL;
              else
                pb = get_pixbuf (string->str, YAD_BIG_ICON);
              gtk_list_store_set (GTK_LIST_STORE (model), &iter, column_count, pb, -1);
              if (pb)
                g_object_unref (pb);
              break;
            case COL_TERM:
              if (strcasecmp (string->str, "true") == 0)
                gtk_list_store_set (GTK_LIST_STORE (model), &iter, column_count, TRUE, -1);
              else
                gtk_list_store_set (GTK_LIST_STORE (model), &iter, column_count, FALSE, -1);
              break;
            }

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

static DEntry *
parse_desktop_file (gchar * filename)
{
  DEntry *ent;
  GKeyFile *kf;
  GError *err = NULL;

  ent = g_new0 (DEntry, 1);
  kf = g_key_file_new ();

  if (g_key_file_load_from_file (kf, filename, 0, &err))
    {
      gchar *icon;

      if (g_key_file_has_group (kf, "Desktop Entry"))
        {
          gint i, type;
          gchar *val;

          /* get type */
          val = g_key_file_get_string (kf, "Desktop Entry", "Type", NULL);
          if (g_ascii_strcasecmp (val, "Link") == 0)
            type = TYPE_LINK;
          else
            type = TYPE_APP;
          g_free (val);

          /* get name */
          if (options.icons_data.generic)
            ent->name = g_key_file_get_locale_string (kf, "Desktop Entry", "GenericName", NULL, NULL);
          if (!ent->name)
            ent->name = g_key_file_get_locale_string (kf, "Desktop Entry", "Name", NULL, NULL);

          /* use filename as a fallback */
          if (!ent->name)
            {
              gchar *ext_pos, *nm = g_path_get_basename (filename);

              ext_pos = g_strrstr (nm, ".desktop");
              if (ext_pos)
                *ext_pos = '\000';
              ent->name = g_strdup (nm);
              g_free (nm);
            }

          /* get tooltip */
          val = g_key_file_get_locale_string (kf, "Desktop Entry", "Comment", NULL, NULL);
          if (val)
            {
              ent->comment = g_markup_escape_text (val, -1);
              g_free (val);
            }
          else
            ent->comment = g_strdup (ent->name);

          /* parse command or url */
          if (type == TYPE_APP)
            {
              ent->command = g_key_file_get_string (kf, "Desktop Entry", "Exec", NULL);
              /* remove possible arguments patterns */
              for (i = strlen (ent->command); i > 0; i--)
                {
                  if (ent->command[i] == '%')
                    {
                      ent->command[i] = '\0';
                      break;
                    }
                }
              ent->in_term = g_key_file_get_boolean (kf, "Desktop Entry", "Terminal", NULL);
            }
          else
            {
              gchar *url = g_key_file_get_string (kf, "Desktop Entry", "URL", NULL);
              if (url)
                {
                  ent->command = g_strdup_printf (settings.open_cmd, url);
                  g_free (url);
                }
            }

          /* add icon */
          icon = g_key_file_get_string (kf, "Desktop Entry", "Icon", NULL);
          if (icon)
            {
              if (options.icons_data.compact)
                ent->pixbuf = get_pixbuf (icon, YAD_SMALL_ICON);
              else
                ent->pixbuf = get_pixbuf (icon, YAD_BIG_ICON);
              g_free (icon);
            }
        }
    }
  else
    g_printerr (_("Unable to parse file %s: %s\n"), filename, err->message);

  g_key_file_free (kf);

  return ent;
}

static void
read_dir ()
{
  GDir *dir;
  const gchar *filename;
  GError *err = NULL;

  dir = g_dir_open (options.icons_data.directory, 0, &err);
  if (!dir)
    {
      g_printerr (_("Unable to open directory %s: %s\n"), options.icons_data.directory, err->message);
      return;
    }

  gtk_list_store_clear (store);

  while ((filename = g_dir_read_name (dir)) != NULL)
    {
      DEntry *ent;
      GtkTreeIter iter;
      gchar *fullname;

      if (!g_str_has_suffix (filename, ".desktop"))
        continue;

      fullname = g_build_filename (options.icons_data.directory, filename, NULL);
      ent = parse_desktop_file (fullname);
      g_free (fullname);

      if (ent->name)
        {
          gtk_list_store_append (store, &iter);
          gtk_list_store_set (store, &iter,
                              COL_FILENAME, filename,
                              COL_NAME, ent->name,
                              COL_TOOLTIP, ent->comment ? ent->comment : "",
                              COL_PIXBUF, ent->pixbuf,
                              COL_COMMAND, ent->command ? ent->command : "", COL_TERM, ent->in_term, -1);
        }

      /* free desktop entry */
      g_free (ent->name);
      g_free (ent->comment);
      g_free (ent->command);
      if (ent->pixbuf)
        g_object_unref (ent->pixbuf);
      g_free (ent);
    }

  g_dir_close (dir);
}

#ifdef HAVE_GIO
static void
dir_changed_cb (GFileMonitor *mon, GFile *file, GFile *ofile, GFileMonitorEvent ev, gpointer data)
{
  if (ev == G_FILE_MONITOR_EVENT_DELETED || ev == G_FILE_MONITOR_EVENT_CREATED)
    read_dir ();
}
#endif

GtkWidget *
icons_create_widget (GtkWidget * dlg)
{
  GtkWidget *w;

  w = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (w), GTK_SHADOW_ETCHED_IN);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (w), options.hscroll_policy, options.vscroll_policy);

  store = gtk_list_store_new (NUM_COLS, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING,
                              GDK_TYPE_PIXBUF, G_TYPE_STRING, G_TYPE_BOOLEAN);
  gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (store),
                                        options.icons_data.sort_by_name ? COL_NAME : COL_FILENAME,
                                        options.icons_data.descend ? GTK_SORT_DESCENDING : GTK_SORT_ASCENDING);

  if (!options.icons_data.compact)
    {
      icon_view = gtk_icon_view_new_with_model (GTK_TREE_MODEL (store));
      gtk_widget_set_name (icon_view, "yad-icons-full");
      gtk_icon_view_set_text_column (GTK_ICON_VIEW (icon_view), COL_NAME);
      gtk_icon_view_set_pixbuf_column (GTK_ICON_VIEW (icon_view), COL_PIXBUF);
      gtk_icon_view_set_tooltip_column (GTK_ICON_VIEW (icon_view), COL_TOOLTIP);
      gtk_icon_view_set_item_width (GTK_ICON_VIEW (icon_view), options.icons_data.width);

      if (options.icons_data.single_click)
        g_signal_connect (G_OBJECT (icon_view), "selection-changed", G_CALLBACK (select_cb), NULL);
    }
  else
    {
      GtkCellRenderer *r;
      GtkTreeViewColumn *col;

      icon_view = gtk_tree_view_new_with_model (GTK_TREE_MODEL (store));
      gtk_widget_set_name (icon_view, "yad-icons-compact");
      gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (icon_view), FALSE);

      col = gtk_tree_view_column_new ();
      r = gtk_cell_renderer_pixbuf_new ();
      gtk_tree_view_column_pack_start (col, r, FALSE);
      gtk_tree_view_column_set_attributes (col, r, "pixbuf", COL_PIXBUF, NULL);
      r = gtk_cell_renderer_text_new ();
      gtk_tree_view_column_pack_start (col, r, FALSE);
      gtk_tree_view_column_set_attributes (col, r, "text", COL_NAME, NULL);
      gtk_tree_view_column_set_resizable (col, TRUE);
      gtk_tree_view_column_set_expand (col, TRUE);
      gtk_tree_view_append_column (GTK_TREE_VIEW (icon_view), col);

      gtk_tree_view_set_tooltip_column (GTK_TREE_VIEW (icon_view), COL_TOOLTIP);

      if (options.icons_data.single_click)
        {
          GtkTreeSelection *sel;
          sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (icon_view));
          g_signal_connect (G_OBJECT (sel), "changed", G_CALLBACK (select_cb), col);
        }
    }

  /* handle directory */
  if (options.icons_data.directory)
    read_dir ();
  else if (options.common_data.listen)
    {
      /* read from stdin */
      GIOChannel *channel;

      channel = g_io_channel_unix_new (0);
      if (channel)
        {
          g_io_channel_set_encoding (channel, NULL, NULL);
          g_io_channel_set_flags (channel, G_IO_FLAG_NONBLOCK, NULL);
          g_io_add_watch (channel, G_IO_IN | G_IO_HUP, handle_stdin, NULL);
        }
    }

  if (!options.icons_data.compact)
    g_signal_connect (G_OBJECT (icon_view), "item-activated", G_CALLBACK (activate_cb), NULL);
  else
    g_signal_connect (G_OBJECT (icon_view), "row-activated", G_CALLBACK (activate_cb), NULL);

#ifdef HAVE_GIO
  /* start file monitor */
  if (options.icons_data.monitor && options.icons_data.directory)
    {
      GFile *file = g_file_new_for_path (options.icons_data.directory);
      if (file)
        {
          GFileMonitor *mon = g_file_monitor_directory (file, 0, NULL, NULL);
          g_signal_connect (G_OBJECT (mon), "changed", G_CALLBACK (dir_changed_cb), NULL);
          g_object_unref (file);
        }
    }
#endif

  gtk_container_add (GTK_CONTAINER (w), icon_view);

  return w;
}
