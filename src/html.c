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

#include <limits.h>
#include <stdlib.h>

#include "yad.h"

#include <glib/gprintf.h>

#include <webkit/webkit.h>

static WebKitWebView *view;

static GString *inbuf;

static gboolean is_link = FALSE;

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

static void
load_uri (const gchar * uri)
{
  gchar *addr = NULL;

  if (!uri || !uri[0])
    return;

  if (g_file_test (uri, G_FILE_TEST_EXISTS))
    {
      if (g_path_is_absolute (uri))
        addr = g_filename_to_uri (uri, NULL, NULL);
      else
        {
          gchar *afn = g_new0 (gchar, PATH_MAX);
          realpath (uri, afn);
          addr = g_filename_to_uri (afn, NULL, NULL);
          g_free (afn);
        }
    }
  else
    {
      if (g_uri_parse_scheme (uri) == NULL)
        addr = g_strdup_printf ("http://%s", uri);
      else
        addr = g_strdup (uri);
    }

  if (addr)
    {
      webkit_web_view_load_uri (view, addr);
      g_free (addr);
    }
  else
    g_printerr ("yad_html_load_uri: cannot load uri '%s'\n", uri);
}

static gboolean
link_cb (WebKitWebView * v, WebKitWebFrame * f, WebKitNetworkRequest * r,
         WebKitWebNavigationAction * act, WebKitWebPolicyDecision * pd, gpointer d)
{
  gchar *uri;

  if (webkit_web_navigation_action_get_reason (act) != WEBKIT_WEB_NAVIGATION_REASON_LINK_CLICKED)
    {
      /* skip handling non clicked reasons */
      webkit_web_policy_decision_use (pd);
      return TRUE;
    }

  uri = (gchar *) webkit_network_request_get_uri (r);

  if (!options.html_data.browser)
    {
      if (options.html_data.print_uri)
        g_printf ("%s\n", uri);
      else
        {
          gchar *cmd = g_strdup_printf (settings.open_cmd, uri);
          g_spawn_command_line_async (cmd, NULL);
          g_free (cmd);
        }
      webkit_web_policy_decision_ignore (pd);
    }
  else
    {
      if (options.html_data.uri_cmd)
        {
          gint ret = -1;
          gchar *cmd = g_strdup_printf (options.html_data.uri_cmd, uri);
          static gchar *vb = NULL, *vm = NULL;

          /* set environment */
          g_free (vb);
          vb = g_strdup_printf ("%d", webkit_web_navigation_action_get_button (act));
          g_setenv ("YAD_HTML_BUTTON", vb, TRUE);
          g_free (vm);
          vm = g_strdup_printf ("%d", webkit_web_navigation_action_get_modifier_state (act));
          g_setenv ("YAD_HTML_KEYS", vm, TRUE);

          /* run handler */
          g_spawn_command_line_sync (cmd, NULL, NULL, &ret, NULL);
          switch (ret)
            {
            case 0:
              webkit_web_policy_decision_use (pd);
              break;
            case 1:
              webkit_web_policy_decision_ignore (pd);
              break;
            case 2:
              webkit_web_policy_decision_download (pd);
              break;
            default:
              g_printerr ("html: undefined result of external uri handler\n");
              webkit_web_policy_decision_ignore (pd);
              break;
            }
          g_free (cmd);
        }
      else
        webkit_web_policy_decision_use (pd);
    }

  return TRUE;
}

static void
link_hover_cb (WebKitWebView * v, const gchar * t, const gchar * link, gpointer * d)
{
  is_link = (link != NULL);
}

static void
select_file_cb (GtkEntry * entry, GtkEntryIconPosition pos, GdkEventButton * ev, gpointer d)
{
  GtkWidget *dlg;
  static gchar *dir = NULL;

  if (ev->button != 1 || pos != GTK_ENTRY_ICON_SECONDARY)
    return;

  dlg = gtk_file_chooser_dialog_new (_("YAD - Select File"),
                                     GTK_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (entry))),
                                     GTK_FILE_CHOOSER_ACTION_OPEN,
                                     GTK_STOCK_OK, GTK_RESPONSE_ACCEPT, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, NULL);
  if (dir)
    gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (dlg), dir);

  if (gtk_dialog_run (GTK_DIALOG (dlg)) == GTK_RESPONSE_ACCEPT)
    {
      gchar *uri = gtk_file_chooser_get_uri (GTK_FILE_CHOOSER (dlg));
      gtk_entry_set_text (entry, uri);
      g_free (uri);

      /* keep current dir */
      g_free (dir);
      dir = gtk_file_chooser_get_current_folder (GTK_FILE_CHOOSER (dlg));
    }

  gtk_widget_destroy (dlg);
}

static void
do_open_cb (GtkWidget * w, GtkDialog * dlg)
{
  gtk_dialog_response (dlg, GTK_RESPONSE_ACCEPT);
}

static void
open_cb (GtkWidget * w, gpointer d)
{
  GtkWidget *dlg, *cnt, *lbl, *entry;

  dlg = gtk_dialog_new_with_buttons (_("Open URI"),
                                     GTK_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (view))),
                                     GTK_DIALOG_DESTROY_WITH_PARENT,
                                     GTK_STOCK_CANCEL, GTK_RESPONSE_REJECT, GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT, NULL);
  gtk_window_set_default_size (GTK_WINDOW (dlg), 350, -1);

  cnt = gtk_dialog_get_content_area (GTK_DIALOG (dlg));

  lbl = gtk_label_new (_("Enter URI or file name:"));
  gtk_misc_set_alignment (GTK_MISC (lbl), 0, 0);
  gtk_widget_show (lbl);
  gtk_box_pack_start (GTK_BOX (cnt), lbl, TRUE, FALSE, 2);

  entry = gtk_entry_new ();
  gtk_entry_set_icon_from_stock (GTK_ENTRY (entry), GTK_ENTRY_ICON_SECONDARY, "gtk-directory");
  gtk_widget_show (entry);
  gtk_box_pack_start (GTK_BOX (cnt), entry, TRUE, FALSE, 2);

  g_signal_connect (G_OBJECT (entry), "icon-press", G_CALLBACK (select_file_cb), NULL);
  g_signal_connect (G_OBJECT (entry), "activate", G_CALLBACK (do_open_cb), dlg);

  if (gtk_dialog_run (GTK_DIALOG (dlg)) == GTK_RESPONSE_ACCEPT)
    load_uri (gtk_entry_get_text (GTK_ENTRY (entry)));

  gtk_widget_destroy (dlg);
}

static gboolean
menu_cb (WebKitWebView * view, GtkWidget * menu, WebKitHitTestResult * hit, gboolean kb, gpointer d)
{
  GtkWidget *mi;

  if (!is_link)
    {
      /* add open item */
      mi = gtk_separator_menu_item_new ();
      gtk_widget_show (mi);
      gtk_menu_shell_prepend (GTK_MENU_SHELL (menu), mi);

      mi = gtk_image_menu_item_new_from_stock ("gtk-open", NULL);
      gtk_widget_show (mi);
      gtk_menu_shell_prepend (GTK_MENU_SHELL (menu), mi);
      g_signal_connect (G_OBJECT (mi), "activate", G_CALLBACK (open_cb), NULL);

      /* add quit item */
      mi = gtk_separator_menu_item_new ();
      gtk_widget_show (mi);
      gtk_menu_shell_append (GTK_MENU_SHELL (menu), mi);

      mi = gtk_image_menu_item_new_from_stock ("gtk-quit", NULL);
      gtk_widget_show (mi);
      gtk_menu_shell_append (GTK_MENU_SHELL (menu), mi);
      g_signal_connect (G_OBJECT (mi), "activate", G_CALLBACK (gtk_main_quit), NULL);
    }

  return FALSE;
}

static void
title_cb (GObject *obj, GParamSpec *spec, GtkWindow *dlg)
{
  const gchar *title = webkit_web_view_get_title (view);
  if (title)
    gtk_window_set_title (dlg, title);
}

static void
icon_cb (GObject *obj, GParamSpec *spec, GtkWindow *dlg)
{
  GdkPixbuf *pb = webkit_web_view_try_get_favicon_pixbuf (view, 16, 16);
  if (pb)
    {
      gtk_window_set_icon (dlg, pb);
      g_object_unref (pb);
    }
}

static gboolean
handle_stdin (GIOChannel * ch, GIOCondition cond, gpointer d)
{
  gchar *buf;
  GError *err = NULL;

  switch (g_io_channel_read_line (ch, &buf, NULL, NULL, &err))
    {
    case G_IO_STATUS_NORMAL:
      g_string_append (inbuf, buf);
      return TRUE;

    case G_IO_STATUS_ERROR:
      g_printerr ("yad_html_handle_stdin(): %s\n", err->message);
      g_error_free (err);
      return FALSE;

    case G_IO_STATUS_EOF:
      webkit_web_view_load_string (view, inbuf->str, options.html_data.mime, options.html_data.encoding, NULL);
      return FALSE;

    case G_IO_STATUS_AGAIN:
      return TRUE;
    }

  return FALSE;
}

GtkWidget *
html_create_widget (GtkWidget * dlg)
{
  GtkWidget *sw;
  WebKitWebSettings *settings;
  SoupSession *sess;

  sw = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (sw), options.hscroll_policy, options.vscroll_policy);

  view = WEBKIT_WEB_VIEW (webkit_web_view_new ());
  gtk_container_add (GTK_CONTAINER (sw), GTK_WIDGET (view));

  settings = webkit_web_settings_new ();
  g_object_set (G_OBJECT (settings), "default-encoding", g_get_codeset (), NULL);
  g_object_set (G_OBJECT (settings), "user-agent", options.html_data.user_agent, NULL);
  if (options.html_data.user_style)
    {
      gchar *uri = g_filename_to_uri (options.html_data.user_style, NULL, NULL);
      g_object_set (G_OBJECT (settings), "user-stylesheet-uri", uri, NULL);
    }
  webkit_web_view_set_settings (view, settings);

  g_signal_connect (view, "hovering-over-link", G_CALLBACK (link_hover_cb), NULL);
  g_signal_connect (view, "navigation-policy-decision-requested", G_CALLBACK (link_cb), NULL);

  if (options.html_data.browser)
    {
      g_signal_connect (view, "context-menu", G_CALLBACK (menu_cb), NULL);
      if (!options.data.dialog_title)
        g_signal_connect (view, "notify::title", G_CALLBACK (title_cb), dlg);
      if (strcmp (options.data.window_icon, "yad") == 0)
        g_signal_connect (view, "icon-loaded", G_CALLBACK (icon_cb), dlg);
    }

  sess = webkit_get_default_session ();
  soup_session_add_feature_by_type (sess, SOUP_TYPE_PROXY_RESOLVER_DEFAULT);
  g_object_set (G_OBJECT (sess), SOUP_SESSION_ACCEPT_LANGUAGE_AUTO, TRUE, NULL);

  gtk_widget_show_all (sw);
  gtk_widget_grab_focus (GTK_WIDGET (view));

  if (options.html_data.uri)
    load_uri (options.html_data.uri);
  else if (!options.html_data.browser)
    {
      GIOChannel *ch;

      inbuf = g_string_new (NULL);
      ch = g_io_channel_unix_new (0);
      g_io_channel_set_encoding (ch, NULL, NULL);
      g_io_channel_set_flags (ch, G_IO_FLAG_NONBLOCK, NULL);
      g_io_add_watch (ch, G_IO_IN | G_IO_HUP, handle_stdin, NULL);
    }

  return sw;
}
