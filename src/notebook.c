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

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/ipc.h>
#include <sys/shm.h>

#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>
#include <gdk/gdk.h>

#include "yad.h"

static GtkWidget *notebook;

GtkWidget *
notebook_create_widget (GtkWidget * dlg)
{
  GtkWidget *w;
  GSList *tab;

  /* get shared memory */
  tabs = get_tabs (options.common_data.key, TRUE);
  if (!tabs)
    exit (-1);

  /* create widget */
  w = notebook = gtk_notebook_new ();
  gtk_widget_set_name (w, "yad-notebook-widget");
  gtk_notebook_set_tab_pos (GTK_NOTEBOOK (w), options.notebook_data.pos);
  gtk_container_set_border_width (GTK_CONTAINER (w), 5);

  /* add tabs */
  for (tab = options.notebook_data.tabs; tab; tab = tab->next)
    {
      GtkWidget *a, *s;

      a = gtk_alignment_new (0.5, 0.5, 1, 1);
      gtk_alignment_set_padding (GTK_ALIGNMENT (a),
                                 options.notebook_data.borders, options.notebook_data.borders,
                                 options.notebook_data.borders, options.notebook_data.borders);

      s = gtk_socket_new ();
      gtk_container_add (GTK_CONTAINER (a), s);
      g_object_set_data (G_OBJECT (a), "socket", s);

      gtk_notebook_append_page (GTK_NOTEBOOK (w), a, get_label ((gchar *) tab->data, 0));
    }

  /* set active tab */
  if (options.notebook_data.active <= 0)
    options.notebook_data.active = 1;
  gtk_notebook_set_current_page (GTK_NOTEBOOK (w), options.notebook_data.active - 1);

  return w;
}

void
notebook_swallow_childs (void)
{
  guint i, n_tabs;

  n_tabs = g_slist_length (options.notebook_data.tabs);

  /* wait until all children are register */
  while (tabs[0].xid != n_tabs)
    usleep (1000);

  for (i = 1; i <= n_tabs; i++)
    {
      GtkWidget *s =
        GTK_WIDGET (g_object_get_data
                    (G_OBJECT (gtk_notebook_get_nth_page (GTK_NOTEBOOK (notebook), i - 1)), "socket"));

      if (tabs[i].pid != -1)
        gtk_socket_add_id (GTK_SOCKET (s), tabs[i].xid);
    }
}

void
notebook_print_result (void)
{
  guint i, n_tabs;

  n_tabs = g_slist_length (options.notebook_data.tabs);
  for (i = 1; i <= n_tabs; i++)
    {
      if (tabs[i].pid != -1)
        kill (tabs[i].pid, SIGUSR1);
    }
}

void
notebook_close_childs (void)
{
  guint i, n_tabs;
  struct shmid_ds buf;
  gboolean is_running = TRUE;

  n_tabs = g_slist_length (options.notebook_data.tabs);
  for (i = 1; i <= n_tabs; i++)
    {
      if (tabs[i].pid != -1)
        kill (tabs[i].pid, SIGUSR2);
    }

  /* wait for stop subprocesses */
  while (is_running)
    {
      is_running = FALSE;
      for (i = 1; i <= n_tabs; i++)
        {
          if (tabs[i].pid != -1 && kill (tabs[i].pid, 0) == 0)
            {
              is_running = TRUE;
              break;
            }
        }
      usleep (1000);
    }

  /* cleanup shared memory */
  shmctl (tabs[0].pid, IPC_RMID, &buf);
  shmdt (tabs);
}
