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

#include <limits.h>
#include <stdlib.h>

#include "yad.h"

#include <webkit2/webkit2.h>

static WebKitWebView *view;
static WebKitFindController *find_ctl = NULL;;
static YadSearchBar *search_bar = NULL;

static GString *inbuf;

static gboolean is_loaded = FALSE;
static gboolean uri_cmd = FALSE;

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

/* searching */
static void
do_find_next (GtkWidget *w, gpointer d)
{
  webkit_find_controller_search_next (find_ctl);
}

static void
do_find_prev (GtkWidget *w, gpointer d)
{
  webkit_find_controller_search_previous (find_ctl);
}

static void
search_changed_cb (GtkWidget *w, gpointer d)
{
  WebKitFindOptions fopts = WEBKIT_FIND_OPTIONS_WRAP_AROUND;

  if (!find_ctl)
    find_ctl = webkit_web_view_get_find_controller (view);
  search_bar->new_search = TRUE;
  search_bar->str = gtk_entry_get_text (GTK_ENTRY (search_bar->entry));

  if (!search_bar->case_sensitive)
    fopts |= WEBKIT_FIND_OPTIONS_CASE_INSENSITIVE;

  webkit_find_controller_search (find_ctl, search_bar->str, fopts, G_MAXUINT);
}

static void
stop_search_cb (GtkWidget *w, YadSearchBar *sb)
{
  if (find_ctl)
    {
      webkit_find_controller_search_finish (find_ctl);
      find_ctl = NULL;
    }

  gtk_search_bar_set_search_mode (GTK_SEARCH_BAR (search_bar->bar), FALSE);
  gtk_widget_grab_focus (GTK_WIDGET (view));
  ignore_esc = FALSE;
}

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
          gchar *afn = realpath (uri, NULL);
          addr = g_filename_to_uri (afn, NULL, NULL);
          g_free (afn);
        }
    }
  else
    {
      if (g_uri_parse_scheme (uri) == NULL)
        addr = g_strdup_printf ("https://%s", uri);
      else
        addr = g_strdup (uri);
    }

  is_loaded = FALSE;
  webkit_web_view_load_uri (view, addr);
  g_free (addr);
}

static void
loaded_cb (WebKitWebView *v, WebKitLoadEvent ev, gpointer d)
{
  if (ev == WEBKIT_LOAD_FINISHED)
    is_loaded = TRUE;
}

static gboolean
policy_cb (WebKitWebView *v, WebKitPolicyDecision *pd, WebKitPolicyDecisionType pt, gpointer d)
{
  const gchar *uri;

  if (!is_loaded)
    return FALSE;

  if (!options.html_data.browser)
    {
      WebKitNavigationAction *act = webkit_navigation_policy_decision_get_navigation_action (WEBKIT_NAVIGATION_POLICY_DECISION (pd));
      if (webkit_navigation_action_get_navigation_type (act) == WEBKIT_NAVIGATION_TYPE_LINK_CLICKED)
        {
          WebKitURIRequest *r = webkit_navigation_action_get_request (act);

          uri = webkit_uri_request_get_uri (r);
          if (strncmp (uri, "about:blank#", 11) == 0)
            webkit_policy_decision_use (pd);
          else
            {
              webkit_policy_decision_ignore (pd);
              if (options.html_data.print_uri)
                g_printf ("%s\n", uri);
              else
                g_app_info_launch_default_for_uri (uri, NULL, NULL);
            }
        }
      else
        webkit_policy_decision_ignore (pd);
    }
  else if (uri_cmd && options.data.uri_handler && options.data.uri_handler[0])
    {
      gchar *v1, *v2, *cmd;
      gint status;

      if (pt == WEBKIT_POLICY_DECISION_TYPE_RESPONSE)
        {
          WebKitURIRequest *r = webkit_response_policy_decision_get_request (WEBKIT_RESPONSE_POLICY_DECISION (pd));
          uri =  webkit_uri_request_get_uri (r);
          v1 = g_strdup ("");
          v2 = g_strdup ("");
        }
      else
        {
          WebKitNavigationAction *act = webkit_navigation_policy_decision_get_navigation_action (WEBKIT_NAVIGATION_POLICY_DECISION (pd));
          WebKitURIRequest *r = webkit_navigation_action_get_request (act);
          uri =  webkit_uri_request_get_uri (r);
          v1 = g_strdup_printf ("%d", webkit_navigation_action_get_mouse_button (act));
          v2 = g_strdup_printf ("%d", webkit_navigation_action_get_modifiers (act));
        }

      g_setenv ("YAD_HTML_BUTTON", v1, TRUE);
      g_setenv ("YAD_HTML_STATE", v2, TRUE);

      if (g_strstr_len (options.data.uri_handler, -1, "%s") != NULL)
        cmd = g_strdup_printf (options.data.uri_handler, uri);
      else
        cmd = g_strdup_printf ("%s '%s'", options.data.uri_handler, uri);
      status = run_command_sync (cmd, NULL);
      g_free (cmd);

      g_unsetenv ("YAD_HTML_BUTTON");
      g_unsetenv ("YAD_HTML_STATE");

      g_free (v1);
      g_free (v2);

      /* actial exit code in a highest byte */
      switch (status >> 8)
        {
        case 0:
          webkit_policy_decision_use (pd);
          break;
        case 1:
          webkit_policy_decision_ignore (pd);
          break;
        case 2:
          if (pt == WEBKIT_POLICY_DECISION_TYPE_RESPONSE)
            webkit_policy_decision_download (pd);
          else
            webkit_policy_decision_use (pd);
          break;
        default:
          g_printerr ("yad: wrong return code (%d) from uri handler\n", status >> 8);
          return FALSE;
        }
    }
  else
    return FALSE;

  return TRUE;
}

static void
select_file_cb (GtkEntry *entry, GtkEntryIconPosition pos, GdkEventButton *ev, gpointer d)
{
  GtkWidget *dlg;
  static gchar *dir = NULL;

  if (ev->button != 1 || pos != GTK_ENTRY_ICON_SECONDARY)
    return;

  dlg = gtk_file_chooser_dialog_new (_("YAD - Select File"),
                                     GTK_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (entry))),
                                     GTK_FILE_CHOOSER_ACTION_OPEN,
                                     _("Cancel"), GTK_RESPONSE_CANCEL,
                                     _("OK"), GTK_RESPONSE_ACCEPT,
                                     NULL);
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
do_open_cb (GtkWidget *w, GtkDialog *dlg)
{
  gtk_dialog_response (dlg, GTK_RESPONSE_ACCEPT);
}

static void
open_cb (GSimpleAction *act, GVariant *param, gpointer d)
{
  GtkWidget *dlg, *cnt, *lbl, *entry;

  dlg = gtk_dialog_new_with_buttons (_("Open URI"), GTK_WINDOW (d),
                                     GTK_DIALOG_DESTROY_WITH_PARENT,
                                     _("Cancel"), GTK_RESPONSE_REJECT,
                                     _("Open"), GTK_RESPONSE_ACCEPT,
                                     NULL);
  gtk_window_set_default_size (GTK_WINDOW (dlg), 350, -1);

  cnt = gtk_dialog_get_content_area (GTK_DIALOG (dlg));

  lbl = gtk_label_new (_("Enter URI or file name:"));
  gtk_label_set_xalign (GTK_LABEL (lbl), 0.0);
  gtk_widget_show (lbl);
  gtk_box_pack_start (GTK_BOX (cnt), lbl, TRUE, FALSE, 2);

  entry = gtk_entry_new ();
  gtk_entry_set_icon_from_icon_name (GTK_ENTRY (entry), GTK_ENTRY_ICON_SECONDARY, "document-open");
  gtk_widget_show (entry);
  gtk_box_pack_start (GTK_BOX (cnt), entry, TRUE, FALSE, 2);

  g_signal_connect (G_OBJECT (entry), "icon-press", G_CALLBACK (select_file_cb), NULL);
  g_signal_connect (G_OBJECT (entry), "activate", G_CALLBACK (do_open_cb), dlg);

  if (gtk_dialog_run (GTK_DIALOG (dlg)) == GTK_RESPONSE_ACCEPT)
    load_uri (gtk_entry_get_text (GTK_ENTRY (entry)));

  gtk_widget_destroy (dlg);
}

static void
quit_cb (GSimpleAction *act, GVariant *param, gpointer d)
{
  yad_exit (options.data.def_resp);
}

static gboolean
menu_cb (WebKitWebView *view, WebKitContextMenu *menu, GdkEvent *ev, WebKitHitTestResult *hit, gpointer d)
{
  WebKitContextMenuItem *mi;
  GSimpleAction *act;

  if (options.common_data.file_op)
    {
      mi = webkit_context_menu_item_new_separator ();
      webkit_context_menu_prepend (menu, mi);

      act = g_simple_action_new ("open", NULL);
      g_signal_connect (G_OBJECT (act), "activate", G_CALLBACK (open_cb), d);

      mi = webkit_context_menu_item_new_from_gaction (G_ACTION (act), _("Open URI"), NULL);
      webkit_context_menu_prepend (menu, mi);
    }

  mi = webkit_context_menu_item_new_separator ();
  webkit_context_menu_append (menu, mi);

  act = g_simple_action_new ("quit", NULL);
  g_signal_connect (G_OBJECT (act), "activate", G_CALLBACK (quit_cb), d);

  mi = webkit_context_menu_item_new_from_gaction (G_ACTION (act), _("Quit"), NULL);
  webkit_context_menu_append (menu, mi);

  return FALSE;
}

static gboolean
key_press_cb (GtkWidget *w, GdkEventKey *key, gpointer d)
{
  if ((key->state & GDK_CONTROL_MASK) && (key->keyval == GDK_KEY_O || key->keyval == GDK_KEY_o))
    {
      open_cb (NULL, NULL, d);
      return TRUE;
    }
  else if ((key->state & GDK_CONTROL_MASK) && (key->keyval == GDK_KEY_Q || key->keyval == GDK_KEY_q))
    {
      yad_exit (options.data.def_resp);
      return TRUE;
    }
  else if ((key->state & GDK_CONTROL_MASK) && (key->keyval == GDK_KEY_F || key->keyval == GDK_KEY_f))
    {
      if (search_bar == NULL)
        return FALSE;

      ignore_esc = TRUE;
      gtk_search_bar_set_search_mode (GTK_SEARCH_BAR (search_bar->bar), TRUE);
      return gtk_search_bar_handle_event (GTK_SEARCH_BAR (search_bar->bar), (GdkEvent *) key);
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
  GdkPixbuf *pb = gdk_pixbuf_get_from_surface (webkit_web_view_get_favicon (view), 0, 0, -1, -1);

  if (pb)
    {
      gtk_window_set_icon (dlg, pb);
      g_object_unref (pb);
    }
}

static gboolean
handle_stdin (GIOChannel *ch, GIOCondition cond, gpointer d)
{
  if ((cond == G_IO_IN) || (cond == G_IO_IN + G_IO_HUP))
    {
       GError *err = NULL;
      GString *string = g_string_new (NULL);

      while (ch->is_readable != TRUE)
        usleep (100);

      do
        {
          gint status;

          do
            status = g_io_channel_read_line_string (ch, string, NULL, &err);
          while (status == G_IO_STATUS_AGAIN);

          if (status != G_IO_STATUS_NORMAL)
            {
              if (err)
                {
                  g_printerr ("yad_html_handle_stdin(): %s\n", err->message);
                  g_error_free (err);
                  err = NULL;
                }
              /* stop handling */
              g_io_channel_shutdown (ch, TRUE, NULL);
              return FALSE;
            }

          if (string->str[0] == '\014' )
            g_string_truncate (inbuf, 0);
          else
            g_string_append (inbuf, string->str);
        }
      while (g_io_channel_get_buffer_condition (ch) == G_IO_IN);
      g_string_free (string, TRUE);

      if (inbuf->len)
        {
          GBytes *data = g_bytes_new (inbuf->str, inbuf->len);
          is_loaded = FALSE;
          webkit_web_view_load_bytes (view, data, options.common_data.mime, options.html_data.encoding, NULL);
          g_bytes_unref (data);
        }
    }

  if ((cond != G_IO_IN) && (cond != G_IO_IN + G_IO_HUP))
    {
      g_io_channel_shutdown (ch, TRUE, NULL);
      return FALSE;
    }

  return TRUE;
}

static void
set_user_props (WebKitSettings *wk_settings)
{
  gint i;

  if (!options.html_data.wk_props)
    return;

  for (i = 0; options.html_data.wk_props[i] != NULL; i++)
    {
      gchar **prop = g_strsplit (options.html_data.wk_props[i], " ", 2);

      if (prop[0] && prop[1])
        {
          switch (prop[1][0])
            {
            case 'b':
              g_object_set (G_OBJECT (wk_settings), prop[0], (strcmp (prop[1] + 2, "true") == 0), NULL);
              break;
            case 'i':
              g_object_set (G_OBJECT (wk_settings), prop[0], atol (prop[1] + 2), NULL);
              break;
            case 's':
              g_object_set (G_OBJECT (wk_settings), prop[0], prop[1] + 2, NULL);
              break;
            default:
              if (options.debug)
                g_warning ("Wrong value '%s' for setting '%s'\n", prop[1], prop[0]);
            }
        }

      g_strfreev (prop);
    }
}

GtkWidget *
html_create_widget (GtkWidget * dlg)
{
  GtkWidget *w, *sw;
  WebKitSettings *wk_settings;
  WebKitUserContentManager *wk_cman;
  gchar *str;

  w = gtk_box_new (GTK_ORIENTATION_VERTICAL, 2);

  sw = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (sw), options.data.hscroll_policy, options.data.vscroll_policy);
  gtk_box_pack_start (GTK_BOX (w), sw, TRUE, TRUE, 0);

  wk_cman = webkit_user_content_manager_new ();
  view = WEBKIT_WEB_VIEW (webkit_web_view_new_with_user_content_manager (wk_cman));
  gtk_container_add (GTK_CONTAINER (sw), GTK_WIDGET (view));

  g_signal_connect (view, "decide-policy", G_CALLBACK (policy_cb), NULL);
  g_signal_connect (view, "load-changed", G_CALLBACK (loaded_cb), NULL);

  wk_settings = webkit_settings_new ();

  g_object_set (G_OBJECT (wk_settings),
                "user-agent", options.html_data.user_agent,
                "default-charset", g_get_codeset (),
                NULL);

  if (options.html_data.browser)
    {
      g_signal_connect (view, "context-menu", G_CALLBACK (menu_cb), dlg);
      g_signal_connect (view, "key-press-event", G_CALLBACK (key_press_cb), dlg);
      if (!options.data.dialog_title)
        g_signal_connect (view, "notify::title", G_CALLBACK (title_cb), dlg);
      if (!options.data.window_icon)
        g_signal_connect (view, "notify::favicon", G_CALLBACK (icon_cb), dlg);
    }
  else
    {
      g_object_set (G_OBJECT (wk_settings),
                    "enable-write-console-messages-to-stdout", TRUE,
                    "enable-caret-browsing", FALSE,
                    "enable-developer-extras", FALSE,
                    "enable-html5-database", FALSE,
                    "enable-html5-local-storage", FALSE,
                    "enable-offline-web-application-cache", FALSE,
                    "enable-page-cache", FALSE,
                    "enable-plugins", FALSE,
                    NULL);
    }

  set_user_props (wk_settings);
  webkit_web_view_set_settings (view, wk_settings);

  /* add user defined css */
  if (options.html_data.user_style)
    {
      gchar *css = NULL;
      GError *err = NULL;

      if (g_file_test (options.html_data.user_style, G_FILE_TEST_EXISTS))
        {
          if (!g_file_get_contents (options.html_data.user_style, &css, NULL, &err))
            {
              g_printerr ("yad_html: unable to load user css file: %s\n", err->message);
              g_error_free (err);
            }
        }
      else
        css = g_strdup (options.html_data.user_style);

      if (css)
        {
          WebKitUserStyleSheet *wk_css;
          wk_css = webkit_user_style_sheet_new (css, WEBKIT_USER_CONTENT_INJECT_ALL_FRAMES,
                                                WEBKIT_USER_STYLE_LEVEL_USER, NULL, NULL);
          webkit_user_content_manager_add_style_sheet (wk_cman, wk_css);
          g_free (css);
        }
    }

  gtk_widget_show_all (sw);
  gtk_widget_grab_focus (GTK_WIDGET (view));

  /* create search bar */
  if (options.common_data.enable_search)
    {
      if ((search_bar = create_search_bar ()) != NULL)
        {
          gtk_box_pack_start (GTK_BOX (w), search_bar->bar, FALSE, FALSE, 0);
          g_signal_connect (G_OBJECT (search_bar->entry), "search-changed", G_CALLBACK (search_changed_cb), NULL);
          g_signal_connect (G_OBJECT (search_bar->entry), "stop-search", G_CALLBACK (stop_search_cb), NULL);
          g_signal_connect (G_OBJECT (search_bar->entry), "next-match", G_CALLBACK (do_find_next), NULL);
          g_signal_connect (G_OBJECT (search_bar->entry), "previous-match", G_CALLBACK (do_find_prev), NULL);
        }
    }

  /* check for user specified uri handler */
#ifndef STANDALONE
  str = g_settings_get_string (settings, "open-command");
#else
  str = g_strdup (OPEN_CMD);
#endif
  if (strcmp (options.data.uri_handler, str) != 0)
    uri_cmd = TRUE;
  g_free (str);

  /* load data */
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
  else if (options.extra_data)
    load_uri (options.extra_data[0]);

  return w;
}
