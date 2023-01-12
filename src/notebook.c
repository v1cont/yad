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
static guint n_tabs = 0;

static void
stack_switch_cb (GObject *obj, GParamSpec *p, gpointer d)
{
  GtkWidget *s = gtk_stack_get_visible_child (GTK_STACK (notebook));
  gtk_widget_child_focus (s, GTK_DIR_TAB_FORWARD);
}

static void
notebook_switch_cb (GtkNotebook *nb, GtkWidget *s, guint pn, gpointer d)
{
  gtk_widget_child_focus (s, GTK_DIR_TAB_FORWARD);
}

GtkWidget *
notebook_create_widget (GtkWidget * dlg)
{
  GtkWidget *w;
  guint i;

  /* get shared memory */
  tabs = get_tabs (options.common_data.key, TRUE);
  if (!tabs)
    exit (-1);

  /* create widget */
  if (options.notebook_data.stack)
    {
      GtkWidget *ss;

      w = gtk_box_new (GTK_ORIENTATION_VERTICAL, 2);

      notebook = gtk_stack_new ();
      gtk_widget_set_name (notebook, "yad-stack-widget");
      gtk_stack_set_homogeneous (GTK_STACK (notebook), TRUE);

      ss = gtk_stack_switcher_new ();
      gtk_widget_set_name (ss, "yad-stack-switcher-widget");
      gtk_box_set_homogeneous (GTK_BOX (ss), options.notebook_data.expand);
      gtk_stack_switcher_set_stack (GTK_STACK_SWITCHER (ss), GTK_STACK (notebook));

      if (options.notebook_data.pos == GTK_POS_BOTTOM)
        {
          gtk_box_pack_start (GTK_BOX (w), notebook, TRUE, TRUE, 2);
          gtk_box_pack_start (GTK_BOX (w), ss, FALSE, FALSE, 2);
        }
      else
        {
          gtk_box_pack_start (GTK_BOX (w), ss, FALSE, FALSE, 2);
          gtk_box_pack_start (GTK_BOX (w), notebook, TRUE, TRUE, 2);
        }

      gtk_widget_set_halign (ss, GTK_ALIGN_CENTER);
    }
  else
    {
      w = notebook = gtk_notebook_new ();
      gtk_widget_set_name (w, "yad-notebook-widget");
      gtk_notebook_set_tab_pos (GTK_NOTEBOOK (w), options.notebook_data.pos);
    }

  /* add tabs */
  for (i = 0; options.notebook_data.tabs[i] != NULL; i++)
    {
      GtkWidget *s;

      s = gtk_socket_new ();
      gtk_widget_set_margin_start (s, options.notebook_data.borders);
      gtk_widget_set_margin_end (s, options.notebook_data.borders);
      gtk_widget_set_margin_top (s, options.notebook_data.borders);
      gtk_widget_set_margin_bottom (s, options.notebook_data.borders);
      gtk_widget_set_can_focus (s, TRUE);

      if (options.notebook_data.stack)
        gtk_stack_add_titled (GTK_STACK (notebook), s, options.notebook_data.tabs[i], options.notebook_data.tabs[i]);
      else
        {
          gtk_notebook_append_page (GTK_NOTEBOOK (w), s, get_label (options.notebook_data.tabs[i], 0, NULL));
          gtk_container_child_set (GTK_CONTAINER (w), s, "tab-expand", options.notebook_data.expand, NULL);
        }
    }
  n_tabs = i;

  return w;
}

void
notebook_swallow_childs (void)
{
  guint i;
  gboolean all_registered;
  GtkWidget *s;

  /* wait until all children are registered */
  do
    {
      all_registered = TRUE;
      for (i = 1; i <= n_tabs; i++)
        if (!tabs[i].xid)
          {
            all_registered = FALSE;
            break;
          }
      if (!all_registered)
        usleep (1000);
    }
  while (!all_registered);

  for (i = 1; i <= n_tabs; i++)
    {
      if (options.notebook_data.stack)
        s = gtk_stack_get_child_by_name (GTK_STACK (notebook), options.notebook_data.tabs[i - 1]);
      else
        s = gtk_notebook_get_nth_page (GTK_NOTEBOOK (notebook), i - 1);

      if (tabs[i].pid != -1 && s)
        gtk_socket_add_id (GTK_SOCKET (s), tabs[i].xid);
    }

  /* add signal handler for passing focus to a child */
  if (options.notebook_data.stack)
    g_signal_connect (G_OBJECT (notebook), "notify::visible-child", G_CALLBACK (stack_switch_cb), NULL);
  else
    g_signal_connect (G_OBJECT (notebook), "switch-page", G_CALLBACK (notebook_switch_cb), NULL);

  /* set active tab */
  if (options.notebook_data.active > 0)
    {
      if (options.notebook_data.stack)
        {
          s = gtk_stack_get_child_by_name (GTK_STACK (notebook), options.notebook_data.tabs[options.notebook_data.active - 1]);
          gtk_stack_set_visible_child (GTK_STACK (notebook), s);
        }
      else
        gtk_notebook_set_current_page (GTK_NOTEBOOK (notebook), options.notebook_data.active - 1);
    }
}

void
notebook_print_result (void)
{
  guint i;

  for (i = 1; i <= n_tabs; i++)
    {
      if (tabs[i].pid != -1)
        kill (tabs[i].pid, SIGUSR1);
    }
}

void
notebook_close_childs (void)
{
  guint i;
  struct shmid_ds buf;
  gboolean is_running;

  for (i = 1; i <= n_tabs; i++)
    {
      if (tabs[i].pid != -1)
        kill (tabs[i].pid, SIGUSR2);
      else
        break;
    }

  /* wait for stop subprocesses */
  do
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
      if (is_running)
        usleep (1000);
    }
  while (is_running);

  /* cleanup shared memory */
  shmctl (tabs[0].pid, IPC_RMID, &buf);
  shmdt (tabs);
}
