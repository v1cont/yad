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

static void
drop_data_cb (GtkWidget * w, GdkDragContext * dc, gint x, gint y,
              GtkSelectionData * sel, guint info, guint t, gpointer data)
{
  GdkAtom stgt;
  static guint drop_cnt = 0;

  stgt = gtk_selection_data_get_target (sel);

  if (gtk_targets_include_uri (&stgt, 1))
    {
      gchar **uris;
      gint i = 0;

      uris = gtk_selection_data_get_uris (sel);
      if (!uris)
        return;

      while (uris[i])
        {
          gchar *dstr = g_uri_unescape_string (uris[i], NULL);
          if (options.common_data.command)
            {
              gchar *action = g_strdup_printf ("%s '%s'", options.common_data.command, dstr);
              g_spawn_command_line_async (action, NULL);
              g_free (action);
            }
          else
            {
              g_printf ("%s\n", dstr);
              fflush (stdout);
            }
          g_free (dstr);
          i++;
        }
      g_strfreev (uris);
    }
  else if (gtk_targets_include_text (&stgt, 1))
    {
      guchar *str = gtk_selection_data_get_text (sel);
      if (str)
        {
          gchar *dstr = g_uri_unescape_string ((const gchar *) str, NULL);
          if (options.common_data.command)
            {
              gchar *action, *arg;

              arg = g_shell_quote (dstr);
              action = g_strdup_printf ("%s %s", options.common_data.command, arg);
              g_free (arg);
              g_spawn_command_line_async (action, NULL);
              g_free (action);
            }
          else
            {
              g_printf ("%s\n", dstr);
              fflush (stdout);
            }
          g_free (dstr);
          g_free (str);
        }
    }

  if (options.dnd_data.exit_on_drop)
    {
      drop_cnt++;
      if (drop_cnt == options.dnd_data.exit_on_drop)
        yad_exit (options.data.def_resp);
    }
}

void
dnd_init (GtkWidget * w)
{
  GtkTargetList *tlist;
  GtkTargetEntry *tgts;
  gint ntgts;

  tlist = gtk_target_list_new (NULL, 0);
  gtk_target_list_add_uri_targets (tlist, 0);
  gtk_target_list_add_text_targets (tlist, 0);

  tgts = gtk_target_table_new_from_list (tlist, &ntgts);

  gtk_drag_dest_set (w, GTK_DEST_DEFAULT_ALL, tgts, ntgts, GDK_ACTION_COPY | GDK_ACTION_MOVE);
  g_signal_connect (G_OBJECT (w), "drag_data_received", G_CALLBACK (drop_data_cb), NULL);

  gtk_target_table_free (tgts, ntgts);
  gtk_target_list_unref (tlist);

  /* set tooltip */
  if (options.dnd_data.tooltip)
    {
      if (!options.data.no_markup)
        gtk_widget_set_tooltip_markup (w, options.data.dialog_text);
      else
        gtk_widget_set_tooltip_text (w, options.data.dialog_text);
    }
}
