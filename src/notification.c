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

#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <string.h>
#include <unistd.h>

#include "yad.h"

typedef struct {
  gchar *name;
  gchar *action;
  gchar *icon;
} MenuData;

static GtkStatusIcon *status_icon;

static gchar *icon = NULL;
static gchar *action = NULL;

static GSList *menu_data = NULL;

static gint exit_code;

static void popup_menu_cb (GtkStatusIcon *, guint, guint, gpointer);

static void
free_menu_data (gpointer data)
{
  MenuData *m = (MenuData *) data;

  if (m) {
    g_free (m->name);
    g_free (m->action);
    g_free (m->icon);
    g_free (m);
  }
}

static void
parse_menu_str (gchar * str)
{
  gchar **menu_vals;
  gint i = 0;

  if (menu_data)
    {
      g_slist_free_full (menu_data, free_menu_data);
      menu_data = NULL;
    }

  menu_vals = g_strsplit (str, options.common_data.separator, -1);

  while (menu_vals[i] != NULL)
    {
      MenuData *mdata = g_new0 (MenuData, 1);
      gchar **s = g_strsplit (menu_vals[i], options.common_data.item_separator, 3);

      if (s[0])
        {
          YadStock sit;
          if (stock_lookup (s[0], &sit))
            {
              mdata->name = g_strdup (sit.label);
              mdata->icon = g_strdup (sit.icon);
            }
          else
              mdata->name = g_strdup (s[0]);
          if (s[1])
            {
              mdata->action = g_strdup (s[1]);
              if (s[2])
                mdata->icon = g_strdup (s[2]);
            }
        }
      menu_data = g_slist_append (menu_data, mdata);
      g_strfreev (s);
      i++;
    }

  g_strfreev (menu_vals);
}

static void
timeout_cb (gpointer data)
{
  exit_code = YAD_RESPONSE_TIMEOUT;
  gtk_main_quit ();
}

static void
set_icon (void)
{
  GdkPixbuf *pixbuf;
  GError *err = NULL;

  if (icon == NULL)
    {
      G_GNUC_BEGIN_IGNORE_DEPRECATIONS;
      gtk_status_icon_set_from_icon_name (status_icon, "yad");
      G_GNUC_END_IGNORE_DEPRECATIONS;
      return;
    }

  if (g_file_test (icon, G_FILE_TEST_EXISTS))
    {
      gint isize = (options.common_data.icon_size > 0) ? options.common_data.icon_size : 16;

      pixbuf = gdk_pixbuf_new_from_file_at_scale (icon, isize, isize, TRUE, &err);
      if (err)
        {
          g_printerr (_("Could not load notification icon '%s': %s\n"), icon, err->message);
          g_clear_error (&err);
        }
      if (pixbuf)
        {
          G_GNUC_BEGIN_IGNORE_DEPRECATIONS;
          gtk_status_icon_set_from_pixbuf (status_icon, pixbuf);
          G_GNUC_END_IGNORE_DEPRECATIONS;
          g_object_unref (pixbuf);
        }
      else
        {
          G_GNUC_BEGIN_IGNORE_DEPRECATIONS;
          gtk_status_icon_set_from_icon_name (status_icon, "yad");
          G_GNUC_END_IGNORE_DEPRECATIONS;
        }
    }
  else
    {
      G_GNUC_BEGIN_IGNORE_DEPRECATIONS;
      gtk_status_icon_set_from_icon_name (status_icon, icon);
      G_GNUC_END_IGNORE_DEPRECATIONS;
    }
}

static gboolean
activate_cb (GtkWidget * widget, YadData * data)
{
  if ((action == NULL && !options.common_data.listen) || (action && g_ascii_strcasecmp (action, "quit") == 0))
    {
      exit_code = YAD_RESPONSE_OK;
      gtk_main_quit ();
    }
  else if (action)
    {
      if (g_ascii_strcasecmp (action, "menu") == 0)
        popup_menu_cb (GTK_STATUS_ICON (widget), 1, GDK_CURRENT_TIME, data);
      else
        run_command_async (action);
    }

  return TRUE;
}

static gboolean
middle_quit_cb (GtkStatusIcon * icon, GdkEventButton * ev, gpointer data)
{
  if (ev->button == 2)
    {
      if (options.data.escape_ok)
        exit_code = YAD_RESPONSE_OK;
      else
        exit_code = YAD_RESPONSE_ESC;
      gtk_main_quit ();
    }

  return FALSE;
}

static void
popup_menu_item_activate_cb (GtkWidget * w, gpointer data)
{
  gchar *cmd = (gchar *) data;

  if (cmd)
    {
      if (g_ascii_strcasecmp (g_strstrip (cmd), "quit") == 0)
        {
          exit_code = YAD_RESPONSE_OK;
          gtk_main_quit ();
        }
      else
        run_command_async (cmd);
    }
}

static void
popup_menu_cb (GtkStatusIcon *icon, guint button, guint activate_time, gpointer data)
{
  GtkWidget *menu;
  GtkWidget *item;
  GSList *m;

  if (!menu_data)
    return;

  menu = gtk_menu_new ();
  gtk_menu_set_reserve_toggle_size (GTK_MENU (menu), FALSE);

  for (m = menu_data; m; m = m->next)
    {
      MenuData *d = (MenuData *) m->data;

      if (d->name)
        {
          GtkWidget *b, *i, *l;

          item = gtk_menu_item_new ();
          b = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 5);

          if (d->icon)
            {
              GdkPixbuf *pb = get_pixbuf (d->icon, YAD_SMALL_ICON, TRUE);
              if (pb)
                {
                  i = gtk_image_new_from_pixbuf (pb);
                  gtk_container_add (GTK_CONTAINER (b), i);
                  g_object_unref (pb);
                }
            }
          l = gtk_label_new_with_mnemonic (d->name);
          gtk_label_set_xalign (GTK_LABEL (l), 0.0);
          gtk_label_set_mnemonic_widget (GTK_LABEL (l), item);
          gtk_box_pack_end (GTK_BOX (b), l, TRUE, TRUE, 0);

          gtk_container_add (GTK_CONTAINER (item), b);

          g_signal_connect (GTK_MENU_ITEM (item), "activate",
                            G_CALLBACK (popup_menu_item_activate_cb), (gpointer) d->action);
        }
      else
        item = gtk_separator_menu_item_new ();

      gtk_widget_show_all (item);
      gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
    }

  gtk_menu_popup_at_pointer (GTK_MENU (menu), NULL);
}

static gboolean
handle_stdin (GIOChannel * channel, GIOCondition condition, gpointer data)
{
  if ((condition & G_IO_IN) != 0)
    {
      GString *string;
      GError *err = NULL;

      string = g_string_new (NULL);
      while (channel->is_readable == FALSE)
        usleep (100);

      do
        {
          gint status;
          gchar *command = NULL, *value = NULL, **args;

          do
            {
              status = g_io_channel_read_line_string (channel, string, NULL, &err);

              while (gdk_events_pending ())
                gtk_main_iteration ();
            }
          while (status == G_IO_STATUS_AGAIN);

          if (status != G_IO_STATUS_NORMAL)
            {
              if (err)
                {
                  g_printerr ("yad_notification_handle_stdin(): %s\n", err->message);
                  g_error_free (err);
                  err = NULL;
                }
              /* stop handling but not exit */
              g_io_channel_shutdown (channel, TRUE, NULL);
              return FALSE;
            }

          strip_new_line (string->str);
          if (!string->str[0])
            continue;

          args = g_strsplit (string->str, ":", 2);
          command = g_strdup (args[0]);
          if (args[1])
            value = g_strdup (args[1]);
          g_strfreev (args);
          if (value)
            g_strstrip (value);

          if (!g_ascii_strcasecmp (command, "icon") && value)
            {
              g_free (icon);
              icon = g_strdup (value);

              G_GNUC_BEGIN_IGNORE_DEPRECATIONS;
              if (gtk_status_icon_get_visible (status_icon) && gtk_status_icon_is_embedded (status_icon))
                set_icon ();
              G_GNUC_END_IGNORE_DEPRECATIONS;
            }
          else if (!g_ascii_strcasecmp (command, "tooltip"))
            {
              if (g_utf8_validate (value, -1, NULL))
                {
                  gchar *message = g_strcompress (value);
                  G_GNUC_BEGIN_IGNORE_DEPRECATIONS;
                  if (!options.data.no_markup)
                    gtk_status_icon_set_tooltip_markup (status_icon, message);
                  else
                    gtk_status_icon_set_tooltip_text (status_icon, message);
                  G_GNUC_END_IGNORE_DEPRECATIONS;
                  g_free (message);
                }
              else
                g_printerr (_("Invalid UTF-8 in tooltip!\n"));
            }
          else if (!g_ascii_strcasecmp (command, "visible"))
            {
              G_GNUC_BEGIN_IGNORE_DEPRECATIONS;
              if (!g_ascii_strcasecmp (value, "false"))
                gtk_status_icon_set_visible (status_icon, FALSE);
              else
                gtk_status_icon_set_visible (status_icon, TRUE);
              G_GNUC_END_IGNORE_DEPRECATIONS;
            }
          else if (!g_ascii_strcasecmp (command, "action"))
            {
              g_free (action);
              action = NULL;
              if (value && *value)
                action = g_strdup (value);
            }
          else if (!g_ascii_strcasecmp (command, "quit"))
            {
              exit_code = YAD_RESPONSE_OK;
              gtk_main_quit ();
            }
          else if (!g_ascii_strcasecmp (command, "menu"))
            {
              if (value && *value)
                parse_menu_str (value);
            }
          else
            g_printerr (_("Unknown command '%s'\n"), command);

          g_free (command);
          g_free (value);
        }
      while (g_io_channel_get_buffer_condition (channel) == G_IO_IN);
      g_string_free (string, TRUE);
    }

  if ((condition & G_IO_HUP) != 0)
    {
      g_io_channel_shutdown (channel, TRUE, NULL);
      gtk_main_quit ();
      return FALSE;
    }

  return TRUE;
}

gint
yad_notification_run ()
{
  GIOChannel *channel = NULL;

  G_GNUC_BEGIN_IGNORE_DEPRECATIONS;
  status_icon = gtk_status_icon_new ();
  G_GNUC_END_IGNORE_DEPRECATIONS;

  G_GNUC_BEGIN_IGNORE_DEPRECATIONS;
  if (options.data.dialog_text)
    {
      if (!options.data.no_markup)
        gtk_status_icon_set_tooltip_markup (status_icon, options.data.dialog_text);
      else
        gtk_status_icon_set_tooltip_text (status_icon, options.data.dialog_text);
    }
  else
    gtk_status_icon_set_tooltip_text (status_icon, _("Yad notification"));
  G_GNUC_END_IGNORE_DEPRECATIONS;

  if (options.data.dialog_image)
    icon = g_strdup (options.data.dialog_image);
  if (options.common_data.command)
    action = g_strdup (options.common_data.command);

  set_icon ();

  g_signal_connect (status_icon, "activate", G_CALLBACK (activate_cb), NULL);
  g_signal_connect (status_icon, "popup_menu", G_CALLBACK (popup_menu_cb), NULL);

  if (options.notification_data.menu)
    parse_menu_str (options.notification_data.menu);

  /* quit on middle click (like press Esc) */
  if (options.notification_data.middle)
    g_signal_connect (status_icon, "button-press-event", G_CALLBACK (middle_quit_cb), NULL);

  if (options.common_data.listen)
    {
      channel = g_io_channel_unix_new (0);
      if (channel)
        {
          g_io_channel_set_encoding (channel, NULL, NULL);
          g_io_channel_set_flags (channel, G_IO_FLAG_NONBLOCK, NULL);
          g_io_add_watch (channel, G_IO_IN | G_IO_HUP, handle_stdin, NULL);
        }
    }

  /* Show icon and wait */
  G_GNUC_BEGIN_IGNORE_DEPRECATIONS;
  gtk_status_icon_set_visible (status_icon, !options.notification_data.hidden);
  G_GNUC_END_IGNORE_DEPRECATIONS;

  if (options.data.timeout > 0)
    g_timeout_add_seconds (options.data.timeout, (GSourceFunc) timeout_cb, NULL);

  gtk_main ();

  return exit_code;
}
