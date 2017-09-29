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

#include <glib/gprintf.h>

#include "yad.h"

static GtkWidget *filechooser;

static void
file_activated_cb (GtkFileChooser * chooser, gpointer data)
{
  if (options.plug == -1)
    yad_exit (options.data.def_resp);
}

gboolean
file_confirm_overwrite (GtkWidget * dlg)
{
  if (options.file_data.save && options.file_data.confirm_overwrite && !options.common_data.multi)
    {
      gchar *filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (filechooser));

      if (g_file_test (filename, G_FILE_TEST_EXISTS))
        {
          GtkWidget *d;
          gint r;
          gchar *buf;

          buf = g_strcompress (options.file_data.confirm_text);
          d = gtk_message_dialog_new (GTK_WINDOW (dlg), GTK_DIALOG_DESTROY_WITH_PARENT,
                                      GTK_MESSAGE_QUESTION, GTK_BUTTONS_YES_NO, "%s", buf);
          g_free (buf);
          r = gtk_dialog_run (GTK_DIALOG (d));
          gtk_widget_destroy (d);
          if (r != GTK_RESPONSE_YES)
            return FALSE;
        }
    }

  return TRUE;
}

GtkWidget *
file_create_widget (GtkWidget * dlg)
{
  GtkWidget *w;
  GList *filt;
  GtkFileChooserAction action = GTK_FILE_CHOOSER_ACTION_OPEN;

  if (options.file_data.directory)
    {
      if (options.file_data.save)
        action = GTK_FILE_CHOOSER_ACTION_CREATE_FOLDER;
      else
        action = GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER;
    }
  else
    {
      if (options.file_data.save)
        action = GTK_FILE_CHOOSER_ACTION_SAVE;
    }

  w = filechooser = gtk_file_chooser_widget_new (action);
  gtk_widget_set_name (w, "yad-file-widget");

  if (options.common_data.uri)
    {
      if (!options.file_data.directory && g_file_test (options.common_data.uri, G_FILE_TEST_IS_DIR))
        gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (w), options.common_data.uri);
      else
        {
          gchar *dir = g_path_get_dirname (options.common_data.uri);

          if (g_path_is_absolute (options.common_data.uri) == TRUE)
            gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (w), dir);

          if (options.common_data.uri[strlen (options.common_data.uri) - 1] != '/')
            {
              gchar *basename = g_path_get_basename (options.common_data.uri);
              if (options.file_data.save)
                gtk_file_chooser_set_current_name (GTK_FILE_CHOOSER (w), basename);
              else
                gtk_file_chooser_set_filename (GTK_FILE_CHOOSER (w), options.common_data.uri);
              g_free (basename);
            }
          g_free (dir);
        }
    }
  else
    gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (w), g_get_current_dir ());

  if (options.common_data.multi)
    gtk_file_chooser_set_select_multiple (GTK_FILE_CHOOSER (w), TRUE);

  /* add preview */
  if (options.common_data.preview)
    {
      GtkWidget *p = gtk_image_new ();
      gtk_file_chooser_set_preview_widget (GTK_FILE_CHOOSER (w), p);
      g_signal_connect (w, "update-preview", G_CALLBACK (update_preview), p);
    }

  /* add filters */
  for (filt = options.common_data.filters; filt; filt = filt->next)
    gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (w), GTK_FILE_FILTER (filt->data));

  g_signal_connect (w, "map", G_CALLBACK (filechooser_mapped), NULL);
  g_signal_connect (w, "file-activated", G_CALLBACK (file_activated_cb), dlg);

  return w;
}

void
file_print_result (void)
{
  GSList *selections, *iter;

  selections = gtk_file_chooser_get_filenames (GTK_FILE_CHOOSER (filechooser));
  for (iter = selections; iter != NULL; iter = iter->next)
    {
      if (options.common_data.quoted_output)
        {
          gchar *buf = g_shell_quote (g_filename_to_utf8 ((gchar *) iter->data, -1, NULL, NULL, NULL));
          g_printf ("%s", buf);
          g_free (buf);
        }
      else
        g_printf ("%s", g_filename_to_utf8 ((gchar *) iter->data, -1, NULL, NULL, NULL));
      g_free (iter->data);
      if (iter->next != NULL)
        g_printf ("%s", options.common_data.separator);
    }
  g_printf ("\n");
  g_slist_free (selections);
}
