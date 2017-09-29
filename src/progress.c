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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>

#include "yad.h"

static GtkWidget *progress_bar;
static GtkWidget *progress_log;
static GtkTextBuffer *log_buffer;

static gboolean
pulsate_progress_bar (gpointer user_data)
{
  gtk_progress_bar_pulse (GTK_PROGRESS_BAR (progress_bar));
  return TRUE;
}

static gboolean
handle_stdin (GIOChannel * channel, GIOCondition condition, gpointer data)
{
  static guint pulsate_timeout = 0;
  float percentage = 0.0;

  if ((condition == G_IO_IN) || (condition == G_IO_IN + G_IO_HUP))
    {
      GString *string;
      GError *err = NULL;

      string = g_string_new (NULL);

      if (options.progress_data.pulsate)
        {
          if (pulsate_timeout == 0)
            pulsate_timeout = g_timeout_add (100, pulsate_progress_bar, NULL);
        }

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
                  g_printerr ("yad_progress_handle_stdin(): %s\n", err->message);
                  g_error_free (err);
                  err = NULL;
                }
              /* stop handling */
              g_io_channel_shutdown (channel, TRUE, NULL);
              return FALSE;
            }

          if (string->str[0] == '#')
            {
              gchar *match;

              /* We have a comment, so let's try to change the label or write it to the log */
              match = g_strcompress (g_strstrip (string->str + 1));
              if (options.progress_data.log)
                {
                  gchar *logline;
                  GtkTextIter end;

                  logline = g_strdup_printf ("%s\n", match);    /* add new line */
                  gtk_text_buffer_get_end_iter (log_buffer, &end);
                  gtk_text_buffer_insert (log_buffer, &end, logline, -1);
                  g_free (logline);

                  /* scroll to end */
                  while (gtk_events_pending ())
                    gtk_main_iteration ();
                  gtk_text_buffer_get_end_iter (log_buffer, &end);
                  gtk_text_view_scroll_to_iter (GTK_TEXT_VIEW (progress_log), &end, 0, FALSE, 0, 0);
                }
              else
                gtk_progress_bar_set_text (GTK_PROGRESS_BAR (progress_bar), match);
              g_free (match);
            }
          else
            {
              if (g_ascii_isdigit (*(string->str)))
                {
                  /* Now try to convert the thing to a number */
                  percentage = atoi (string->str);
                  if (percentage >= 100)
                    {
                      gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (progress_bar), 1.0);
                      if (options.progress_data.autoclose && options.plug == -1)
                        yad_exit (options.data.def_resp);
                    }
                  else
                    gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (progress_bar), percentage / 100.0);
                }
            }

        }
      while (g_io_channel_get_buffer_condition (channel) == G_IO_IN);
      g_string_free (string, TRUE);
    }

  if ((condition != G_IO_IN) && (condition != G_IO_IN + G_IO_HUP))
    {
      gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (progress_bar), 1.0);

      if (options.progress_data.pulsate)
        {
          g_source_remove (pulsate_timeout);
          pulsate_timeout = 0;
        }

      if (options.progress_data.autoclose && options.plug == -1)
        yad_exit (options.data.def_resp);

      g_io_channel_shutdown (channel, TRUE, NULL);
      return FALSE;
    }
  return TRUE;
}

GtkWidget *
progress_create_widget (GtkWidget * dlg)
{
  GtkWidget *w;
  GIOChannel *channel;

  // fix it when vertical specified
#if GTK_CHECK_VERSION(3,0,0)
  w = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
#else
  w = gtk_vbox_new (FALSE, 0);
#endif

  progress_bar = gtk_progress_bar_new ();
  gtk_widget_set_name (progress_bar, "yad-progress-widget");
  if (options.progress_data.log_on_top)
    gtk_box_pack_end (GTK_BOX (w), progress_bar, FALSE, FALSE, 0);
  else
    gtk_box_pack_start (GTK_BOX (w), progress_bar, FALSE, FALSE, 0);

  if (options.progress_data.percentage > 100)
    options.progress_data.percentage = 100;
  gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (progress_bar), options.progress_data.percentage / 100.0);
  if (options.progress_data.progress_text)
    gtk_progress_bar_set_text (GTK_PROGRESS_BAR (progress_bar), options.progress_data.progress_text);
#if GTK_CHECK_VERSION(3,0,0)
  gtk_progress_bar_set_inverted (GTK_PROGRESS_BAR (progress_bar), options.progress_data.rtl);
#else
  if (options.progress_data.rtl)
    gtk_progress_bar_set_orientation (GTK_PROGRESS_BAR (progress_bar), GTK_PROGRESS_RIGHT_TO_LEFT);
#endif

  if (options.progress_data.log)
    {
      GtkWidget *ex, *sw;

      ex = gtk_expander_new (options.progress_data.log);
      gtk_expander_set_spacing (GTK_EXPANDER (ex), 2);
      gtk_expander_set_expanded (GTK_EXPANDER (ex), options.progress_data.log_expanded);
      gtk_box_pack_start (GTK_BOX (w), ex, TRUE, TRUE, 2);

      sw = gtk_scrolled_window_new (NULL, NULL);
      gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (sw), GTK_SHADOW_ETCHED_IN);
      gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (sw), options.hscroll_policy, options.vscroll_policy);
      gtk_container_add (GTK_CONTAINER (ex), sw);

      progress_log = gtk_text_view_new ();
      gtk_widget_set_name (progress_log, "yad-text-widget");
      gtk_widget_set_size_request (progress_log, -1, options.progress_data.log_height);
      gtk_container_add (GTK_CONTAINER (sw), progress_log);

      log_buffer = gtk_text_buffer_new (NULL);
      gtk_text_view_set_buffer (GTK_TEXT_VIEW (progress_log), log_buffer);
      gtk_text_view_set_left_margin (GTK_TEXT_VIEW (progress_log), 5);
      gtk_text_view_set_right_margin (GTK_TEXT_VIEW (progress_log), 5);
      gtk_text_view_set_editable (GTK_TEXT_VIEW (progress_log), FALSE);
      gtk_text_view_set_cursor_visible (GTK_TEXT_VIEW (progress_log), FALSE);
    }
#if GTK_CHECK_VERSION(3,0,0)
  else
    gtk_progress_bar_set_show_text (GTK_PROGRESS_BAR (progress_bar), TRUE);
#endif

  channel = g_io_channel_unix_new (0);
  g_io_channel_set_encoding (channel, NULL, NULL);
  g_io_channel_set_flags (channel, G_IO_FLAG_NONBLOCK, NULL);
  g_io_add_watch (channel, G_IO_IN | G_IO_HUP, handle_stdin, dlg);

  return w;
}
