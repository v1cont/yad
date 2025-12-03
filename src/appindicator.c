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
 * Copyright (C) 2008-2025, Victor Ananjevsky <victor@sanana.kiev.ua>
 */

/*
 * AppIndicator/StatusNotifier support for modern desktop environments.
 * This is a separate dialog type from the legacy GtkStatusIcon notification.
 *
 * This module provides system tray functionality using the StatusNotifier
 * specification (D-Bus based) which is supported by KDE Plasma, GNOME
 * (with extensions), and other modern desktop environments.
 */

#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <string.h>
#include <unistd.h>

#include "yad.h"

/* Try Ayatana AppIndicator first (recommended), fall back to legacy */
#if __has_include(<libayatana-appindicator/app-indicator.h>)
#include <libayatana-appindicator/app-indicator.h>
#elif __has_include(<libappindicator/app-indicator.h>)
#include <libappindicator/app-indicator.h>
#else
#error "AppIndicator support enabled but headers not found"
#endif

typedef struct {
  gchar *name;
  gchar *action;
  gchar *icon;
} MenuData;

static AppIndicator *app_indicator = NULL;
static GtkWidget *indicator_menu = NULL;

static gchar *icon = NULL;
static gchar *action = NULL;

static GSList *menu_data = NULL;

static gint exit_code;

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
parse_menu_str (gchar *str)
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
  const gchar *icon_name = icon ? icon : "yad";

  if (app_indicator)
    {
      app_indicator_set_icon_full (app_indicator, icon_name, "YAD");
    }
}

static void
activate_cb (GtkMenuItem *item, gpointer data)
{
  if ((action == NULL && !options.common_data.listen) || (action && g_ascii_strcasecmp (action, "quit") == 0))
    {
      exit_code = YAD_RESPONSE_OK;
      gtk_main_quit ();
    }
  else if (action)
    {
      run_command_async (action);
    }
}

static void
popup_menu_item_activate_cb (GtkWidget *w, gpointer data)
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
build_indicator_menu (void)
{
  GtkWidget *item;
  GSList *m;

  if (indicator_menu)
    gtk_widget_destroy (indicator_menu);

  indicator_menu = gtk_menu_new ();

  /* Add activate action as first item if action is set */
  if (action && g_ascii_strcasecmp (action, "quit") != 0 && g_ascii_strcasecmp (action, "menu") != 0)
    {
      item = gtk_menu_item_new_with_label (_("Activate"));
      g_signal_connect (item, "activate", G_CALLBACK (activate_cb), NULL);
      gtk_menu_shell_append (GTK_MENU_SHELL (indicator_menu), item);

      item = gtk_separator_menu_item_new ();
      gtk_menu_shell_append (GTK_MENU_SHELL (indicator_menu), item);
    }

  /* Add custom menu items */
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

      gtk_menu_shell_append (GTK_MENU_SHELL (indicator_menu), item);
    }

  /* Add quit item */
  item = gtk_separator_menu_item_new ();
  gtk_menu_shell_append (GTK_MENU_SHELL (indicator_menu), item);

  item = gtk_menu_item_new_with_label (_("Quit"));
  g_signal_connect (item, "activate", G_CALLBACK (popup_menu_item_activate_cb), "quit");
  gtk_menu_shell_append (GTK_MENU_SHELL (indicator_menu), item);

  gtk_widget_show_all (indicator_menu);
  app_indicator_set_menu (app_indicator, GTK_MENU (indicator_menu));
}

static gboolean
handle_stdin (GIOChannel *channel, GIOCondition condition, gpointer data)
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
                  g_printerr ("yad_appindicator_handle_stdin(): %s\n", err->message);
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
              set_icon ();
            }
          else if (!g_ascii_strcasecmp (command, "tooltip"))
            {
              if (g_utf8_validate (value, -1, NULL))
                {
                  gchar *message = g_strcompress (value);
                  app_indicator_set_title (app_indicator, message);
                  g_free (message);
                }
              else
                g_printerr (_("Invalid UTF-8 in tooltip!\n"));
            }
          else if (!g_ascii_strcasecmp (command, "visible"))
            {
              if (!g_ascii_strcasecmp (value, "false"))
                app_indicator_set_status (app_indicator, APP_INDICATOR_STATUS_PASSIVE);
              else
                app_indicator_set_status (app_indicator, APP_INDICATOR_STATUS_ACTIVE);
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
                {
                  parse_menu_str (value);
                  build_indicator_menu ();
                }
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
yad_appindicator_run (void)
{
  GIOChannel *channel = NULL;

  if (options.data.dialog_image)
    icon = g_strdup (options.data.dialog_image);
  if (options.common_data.command)
    action = g_strdup (options.common_data.command);

  if (options.notification_data.menu)
    parse_menu_str (options.notification_data.menu);

  /* Create AppIndicator */
  app_indicator = app_indicator_new ("yad-indicator",
                                     icon ? icon : "yad",
                                     APP_INDICATOR_CATEGORY_APPLICATION_STATUS);

  if (!app_indicator)
    {
      g_printerr (_("Failed to create AppIndicator\n"));
      return YAD_RESPONSE_CANCEL;
    }

  app_indicator_set_status (app_indicator,
                           options.notification_data.hidden ?
                           APP_INDICATOR_STATUS_PASSIVE :
                           APP_INDICATOR_STATUS_ACTIVE);

  if (options.data.dialog_text)
    app_indicator_set_title (app_indicator, options.data.dialog_text);
  else
    app_indicator_set_title (app_indicator, _("Yad indicator"));

  set_icon ();

  /* Build menu - AppIndicator requires a menu */
  build_indicator_menu ();

  if (options.debug)
    g_printerr (_("Using AppIndicator (StatusNotifier) backend.\n"));

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

  if (options.data.timeout > 0)
    g_timeout_add_seconds (options.data.timeout, (GSourceFunc) timeout_cb, NULL);

  gtk_main ();

  return exit_code;
}

