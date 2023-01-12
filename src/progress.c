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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>

#include "yad.h"

static GSList *progress_bars = NULL;
static guint nbars = 0;

static GtkWidget *progress_log;
static GtkTextBuffer *log_buffer;

static gboolean single_mode = FALSE;

static gboolean
pulsate_progress_bar (GtkProgressBar *bar)
{
  gtk_progress_bar_pulse (bar);
  return TRUE;
}

static gboolean
handle_stdin (GIOChannel *channel, GIOCondition condition, gpointer data)
{
  float percentage = 0.0;

  if ((condition == G_IO_IN) || (condition == G_IO_IN + G_IO_HUP))
    {
      GString *string;
      gchar **value;
      GError *err = NULL;

      string = g_string_new (NULL);

      while (channel->is_readable != TRUE)
        usleep (100);

      do
        {
          gint status, num;
          GtkProgressBar *pb;
          YadProgressBar *b;

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
                  g_printerr ("yad_multi_progress_handle_stdin(): %s\n", err->message);
                  g_error_free (err);
                  err = NULL;
                }
              /* stop handling */
              g_io_channel_shutdown (channel, TRUE, NULL);
              return FALSE;
            }

          if (single_mode)
            {
              value = g_new0 (gchar *, 2);
              value[1] = g_strdup (string->str);
              num = 0;
            }
          else
            {
              value = g_strsplit (string->str, ":", 2);
              num = atoi (value[0]) - 1;
              if (num < 0 || num > nbars - 1)
                continue;
            }

          pb = GTK_PROGRESS_BAR (g_slist_nth_data (progress_bars, num));
          b = (YadProgressBar *) g_slist_nth_data (options.progress_data.bars, num);

          if (value[1] && value[1][0] == '#')
            {
              gchar *match;

              /* We have a comment, so let's try to change the label */
              match = g_strcompress (value[1] + 1);
              strip_new_line (match);
              if (options.progress_data.log)
                {
                  gchar *logline;
                  GtkTextIter end;

                  logline = g_strdup_printf ("%s\n", match);    /* add new line */
                  gtk_text_buffer_get_end_iter (log_buffer, &end);
                  if (options.data.no_markup)
                    gtk_text_buffer_insert (log_buffer, &end, logline, -1);
                  else
                    gtk_text_buffer_insert_markup (log_buffer, &end, logline, -1);
                  g_free (logline);

                  /* scroll to end */
                  while (gtk_events_pending ())
                    gtk_main_iteration ();
                  gtk_text_buffer_get_end_iter (log_buffer, &end);
                  gtk_text_view_scroll_to_iter (GTK_TEXT_VIEW (progress_log), &end, 0, FALSE, 0, 0);
                }
              else
                {
                  if (!options.common_data.hide_text)
                    gtk_progress_bar_set_text (pb, match);
                }
              g_free (match);
            }
          else
            {
              if (value[1] && b->type == YAD_PROGRESS_PULSE)
                gtk_progress_bar_pulse (pb);
              else if (value[1] && b->type == YAD_PROGRESS_PERM)
                {
                  guint id;

                  if (strncmp (value[1], "start", 5) == 0)
                    {
                      id = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (pb), "id"));
                      if (id == 0)
                        {
                          id = g_timeout_add (100, (GSourceFunc) pulsate_progress_bar, pb);
                          g_object_set_data (G_OBJECT (pb), "id", GINT_TO_POINTER (id));
                        }
                    }
                  else if (strncmp (value[1], "stop", 4) == 0)
                    {
                      id = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (pb), "id"));
                      if (id > 0)
                        {
                          g_source_remove (id);
                          g_object_set_data (G_OBJECT (pb), "id", GINT_TO_POINTER (0));
                        }
                    }
                }
              else
                {
                  if (!value[1] || !g_ascii_isdigit (*value[1]))
                    continue;

                  /* Now try to convert the thing to a number */
                  percentage = atoi (value[1]);
                  if (percentage >= 100)
                    gtk_progress_bar_set_fraction (pb, 1.0);
                  else
                    gtk_progress_bar_set_fraction (pb, percentage / 100.0);

                  /* Check if all of progress bars reaches 100% */
                  if (options.progress_data.autoclose && options.plug == -1)
                    {
                      guint i;
                      gboolean close = TRUE;
                      gboolean need_close = FALSE;

                      if (options.progress_data.watch_bar > 0 && options.progress_data.watch_bar <= nbars)
                        {
                          GtkProgressBar *cpb = GTK_PROGRESS_BAR (g_slist_nth_data (progress_bars,
                                                                                    options.progress_data.watch_bar - 1));

                          need_close = TRUE;
                          if (gtk_progress_bar_get_fraction (cpb) != 1.0)
                            close = FALSE;
                        }
                      else
                        {
                          for (i = 0; i < nbars; i++)
                            {
                              GtkProgressBar *cpb = GTK_PROGRESS_BAR (g_slist_nth_data (progress_bars, i));
                              YadProgressBar *cb = (YadProgressBar *) g_slist_nth_data (options.progress_data.bars, i);

                              if (cb->type != YAD_PROGRESS_PULSE)
                                {
                                  need_close = TRUE;
                                  if (gtk_progress_bar_get_fraction (cpb) != 1.0)
                                    {
                                      close = FALSE;
                                      break;
                                    }
                                }
                            }
                        }

                      if (need_close && close)
                        yad_exit (options.data.def_resp);
                    }
                }
            }
        }
      while (g_io_channel_get_buffer_condition (channel) == G_IO_IN);
      g_string_free (string, TRUE);
    }

  if ((condition != G_IO_IN) && (condition != G_IO_IN + G_IO_HUP))
    {
      g_io_channel_shutdown (channel, TRUE, NULL);

      if (options.progress_data.autoclose && options.plug == -1)
         yad_exit (options.data.def_resp);

      return FALSE;
    }
  return TRUE;
}

GtkWidget *
progress_create_widget (GtkWidget *dlg)
{
  GtkWidget *table, *w = NULL;
  GIOChannel *channel;
  GSList *b;
  gint i = 0;

  nbars = g_slist_length (options.progress_data.bars);
  if (nbars < 1)
    {
      YadProgressBar *bar = g_new0 (YadProgressBar, 1);

      if (options.debug)
        g_printerr (_("WARNING: You are using old-style progress dialog. This features is obsolete now, please use --bar style instead\n"));

      if (options.progress_data.pulsate)
        bar->type = YAD_PROGRESS_PULSE;
      else if (options.progress_data.rtl)
        bar->type = YAD_PROGRESS_RTL;
      else
        bar->type = YAD_PROGRESS_NORMAL;

      if (options.progress_data.progress_text)
        bar->name = options.progress_data.progress_text;

      options.progress_data.bars = g_slist_append (options.progress_data.bars, bar);
      nbars = 1;

      options.progress_data.watch_bar = 1;

      single_mode = TRUE;
    }

  table = gtk_grid_new ();
  gtk_grid_set_row_spacing (GTK_GRID (table), 2);
  gtk_grid_set_column_spacing (GTK_GRID (table), 2);

  if (options.common_data.scroll)
    {
      GtkWidget *sw = gtk_scrolled_window_new (NULL, NULL);
      gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (sw), GTK_SHADOW_NONE);
      gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (sw), options.data.hscroll_policy, options.data.vscroll_policy);
      gtk_container_add (GTK_CONTAINER (sw), table);
      w = sw;
    }
  else
    w = table;

  for (b = options.progress_data.bars; b; b = b->next)
    {
      GtkWidget *l, *w;
      YadProgressBar *p = (YadProgressBar *) b->data;

      /* add label */
      l = gtk_label_new (NULL);
      if (options.data.no_markup)
        gtk_label_set_text (GTK_LABEL (l), p->name);
      else
        gtk_label_set_markup (GTK_LABEL (l), p->name);
      gtk_label_set_xalign (GTK_LABEL (l), options.common_data.align);

      if (options.common_data.vertical)
        gtk_grid_attach (GTK_GRID (table), l, i, 1, 1, 1);
      else
        gtk_grid_attach (GTK_GRID (table), l, 0, i, 1, 1);

      /* add progress bar */
      w = gtk_progress_bar_new ();
      gtk_widget_set_name (w, "yad-progress-widget");
      gtk_progress_bar_set_show_text (GTK_PROGRESS_BAR (w), !options.common_data.hide_text);

      if (p->type != YAD_PROGRESS_PULSE)
        {
          if (options.extra_data && options.extra_data[i])
            {
              if (g_ascii_isdigit (*options.extra_data[i]))
                gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (w), atoi (options.extra_data[i]) / 100.0);
            }
        }
      else
        {
          if (options.extra_data && options.extra_data[i])
            {
              if (g_ascii_isdigit (*options.extra_data[i]))
                gtk_progress_bar_set_pulse_step (GTK_PROGRESS_BAR (w), atoi (options.extra_data[i]) / 100.0);
            }
        }

      gtk_progress_bar_set_inverted (GTK_PROGRESS_BAR (w), p->type == YAD_PROGRESS_RTL);

      if (options.common_data.vertical)
        {
          gtk_orientable_set_orientation (GTK_ORIENTABLE (w), GTK_ORIENTATION_VERTICAL);
          gtk_grid_attach (GTK_GRID (table), w, i, 0, 1, 1);
          gtk_widget_set_vexpand (w, TRUE);
        }
      else
        {
          gtk_grid_attach (GTK_GRID (table), w, 1, i, 1, 1);
          gtk_widget_set_hexpand (w, TRUE);
        }

      progress_bars = g_slist_append (progress_bars, w);

      i++;
    }

  if (options.progress_data.log)
    {
      GtkWidget *ex, *sw;

      ex = gtk_expander_new (options.progress_data.log);
      gtk_expander_set_spacing (GTK_EXPANDER (ex), 2);
      gtk_expander_set_expanded (GTK_EXPANDER (ex), options.progress_data.log_expanded);

      if (options.common_data.vertical)
        gtk_grid_attach (GTK_GRID (table), ex, 0, 2, i, 1);
      else
        gtk_grid_attach (GTK_GRID (table), ex, 0, i, 2, 1);

      sw = gtk_scrolled_window_new (NULL, NULL);
      gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (sw), GTK_SHADOW_ETCHED_IN);
      gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (sw), options.data.hscroll_policy, options.data.vscroll_policy);
      gtk_scrolled_window_set_propagate_natural_height (GTK_SCROLLED_WINDOW (sw), TRUE);
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

  channel = g_io_channel_unix_new (0);
  g_io_channel_set_encoding (channel, NULL, NULL);
  g_io_channel_set_flags (channel, G_IO_FLAG_NONBLOCK, NULL);
  g_io_add_watch (channel, G_IO_IN | G_IO_HUP, handle_stdin, dlg);

  return w;
}
