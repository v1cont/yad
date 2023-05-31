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

#include <pango/pango.h>

#include "yad.h"

static GtkWidget *text_view;
static GObject *text_buffer;
static GtkTextTag *tag;
static GdkCursor *hand, *normal;
static YadSearchBar *search_bar = NULL;
static GtkTextIter match_start, match_end;
static gboolean text_changed = FALSE;
static gboolean search_changed = FALSE;

/* searching */
static void
do_find_next (GtkWidget *w, gpointer d)
{
  GtkTextIter search_pos;
  GtkTextSearchFlags sflags;

  if (search_bar->case_sensitive)
    sflags = GTK_TEXT_SEARCH_TEXT_ONLY;
  else
    sflags = GTK_TEXT_SEARCH_TEXT_ONLY | GTK_TEXT_SEARCH_CASE_INSENSITIVE;

  if (search_bar->new_search)
    {
      gtk_text_buffer_get_iter_at_mark (GTK_TEXT_BUFFER (text_buffer), &search_pos,
                                        gtk_text_buffer_get_insert (GTK_TEXT_BUFFER (text_buffer)));
    }
  else
    search_pos = match_end;

  if (gtk_text_iter_forward_search (&search_pos, search_bar->str, sflags, &match_start, &match_end, NULL))
    {
      gtk_text_view_scroll_to_iter (GTK_TEXT_VIEW (text_view), &match_start, 0.0, FALSE, 0.0, 0.0);
      gtk_text_buffer_select_range (GTK_TEXT_BUFFER (text_buffer), &match_start, &match_end);
      search_changed = TRUE;
    }

  search_bar->new_search = FALSE;
}

static void
do_find_prev (GtkWidget *w, gpointer d)
{
  GtkTextIter search_pos;
  GtkTextSearchFlags sflags;

  if (search_bar->case_sensitive)
    sflags = GTK_TEXT_SEARCH_TEXT_ONLY;
  else
    sflags = GTK_TEXT_SEARCH_TEXT_ONLY | GTK_TEXT_SEARCH_CASE_INSENSITIVE;

  search_pos = match_start;
  if (gtk_text_iter_backward_search (&search_pos, search_bar->str, sflags, &match_start, &match_end, NULL))
    {
      gtk_text_view_scroll_to_iter (GTK_TEXT_VIEW (text_view), &match_start, 0.0, FALSE, 0.0, 0.0);
      gtk_text_buffer_select_range (GTK_TEXT_BUFFER (text_buffer), &match_start, &match_end);
      search_changed = TRUE;
    }
}

static void
search_changed_cb (GtkWidget *w, gpointer d)
{
  search_bar->new_search = TRUE;
  search_bar->str = gtk_entry_get_text (GTK_ENTRY (search_bar->entry));
  do_find_next (NULL, NULL);
}

static void
stop_search_cb (GtkWidget *w, YadSearchBar *sb)
{
  ignore_esc = FALSE;
  gtk_search_bar_set_search_mode (GTK_SEARCH_BAR (search_bar->bar), FALSE);
  gtk_widget_grab_focus (text_view);
  if (search_changed)
    {
      gtk_text_iter_backward_char (&match_start);
      gtk_text_buffer_place_cursor (GTK_TEXT_BUFFER (text_buffer), &match_start);
    }
  search_changed = FALSE;
}

/* early prototype for use in open_file_cb() */
static void fill_buffer_from_file ();

/* file operations */
static void
open_file_cb (GtkWidget *w, gpointer d)
{
  GtkWidget *dlg;
  static gchar *dir = NULL;

  if (!dir && options.common_data.uri)
    dir = g_path_get_dirname (options.common_data.uri);

  dlg = gtk_file_chooser_dialog_new (_("YAD - Select File"),
                                     GTK_WINDOW (gtk_widget_get_toplevel (text_view)),
                                     GTK_FILE_CHOOSER_ACTION_OPEN,
                                     _("Cancel"), GTK_RESPONSE_CANCEL,
                                     _("OK"), GTK_RESPONSE_ACCEPT,
                                     NULL);
  if (dir)
    gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (dlg), dir);

  if (gtk_dialog_run (GTK_DIALOG (dlg)) == GTK_RESPONSE_ACCEPT)
    {
      /* set new filename */
      if (options.common_data.uri)
        g_free (options.common_data.uri);
      options.common_data.uri = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dlg));

      /* load file */
      gtk_text_buffer_set_text (GTK_TEXT_BUFFER (text_buffer), "", -1);
      fill_buffer_from_file ();

      /* keep current dir */
      g_free (dir);
      dir = gtk_file_chooser_get_current_folder (GTK_FILE_CHOOSER (dlg));

      text_changed = FALSE;
    }

  gtk_widget_destroy (dlg);
}

static void
save_file_cb (GtkWidget *w, gpointer d)
{
  GtkTextIter start, end;
  gchar *text;
  GStatBuf st;
  gint mode = -1;
  GError *err = NULL;

  gtk_text_buffer_get_bounds (GTK_TEXT_BUFFER (text_buffer), &start, &end);
  text = gtk_text_buffer_get_text (GTK_TEXT_BUFFER (text_buffer), &start, &end, 0);

  /* g_file_set_contents changes the file permissions. so it must be kept and restored after file saving */
  if (g_stat (options.common_data.uri, &st) == 0)
    mode = st.st_mode;

  if (!g_file_set_contents (options.common_data.uri, text, -1, &err))
    {
      g_printerr ("Cannot save file %s: %s\n", options.common_data.uri, err->message);
      g_error_free (err);
    }
  else
    {
      /* restore permissions */
      if (mode != -1)
        g_chmod (options.common_data.uri, mode);
      text_changed = FALSE;
    }

  g_free (text);
}

static void
save_as_file_cb (GtkWidget *w, gpointer d)
{
  GtkWidget *dlg;
  static gchar *dir = NULL;

  if (!dir && options.common_data.uri)
    dir = g_path_get_dirname (options.common_data.uri);

  dlg = gtk_file_chooser_dialog_new (_("YAD - Select File"),
                                     GTK_WINDOW (gtk_widget_get_toplevel (text_view)),
                                     GTK_FILE_CHOOSER_ACTION_SAVE,
                                     _("Cancel"), GTK_RESPONSE_CANCEL,
                                     _("OK"), GTK_RESPONSE_ACCEPT,
                                     NULL);

  gtk_file_chooser_set_create_folders (GTK_FILE_CHOOSER (dlg), TRUE);

  if (dir)
    gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (dlg), dir);

  if (options.common_data.uri)
    gtk_file_chooser_set_filename (GTK_FILE_CHOOSER (dlg), options.common_data.uri);
  else
    gtk_file_chooser_set_current_name (GTK_FILE_CHOOSER (dlg), "Untitled.txt");

  if (gtk_dialog_run (GTK_DIALOG (dlg)) == GTK_RESPONSE_ACCEPT)
    {
      /* set new filename and save it */
      if (options.common_data.uri)
        g_free (options.common_data.uri);
      options.common_data.uri = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dlg));
      save_file_cb (w, d);

      /* keep current dir */
      g_free (dir);
      dir = gtk_file_chooser_get_current_folder (GTK_FILE_CHOOSER (dlg));
    }

  gtk_widget_destroy (dlg);
}

static void
quit_cb (GtkWidget *w, gpointer d)
{
  yad_exit (options.data.def_resp);
}

static void
menu_popup_cb (GtkTextView *w, GtkWidget *popup, gpointer d)
{
  if (!GTK_IS_MENU (popup))
    return;

  if (options.common_data.file_op)
    {
      GtkWidget *mitem;

      /* on top */
      mitem = gtk_separator_menu_item_new ();
      gtk_menu_shell_prepend (GTK_MENU_SHELL (popup), mitem);
      gtk_widget_show (mitem);

      if (options.common_data.editable)
        {
          mitem = gtk_menu_item_new_with_mnemonic (_("Save _as..."));
          gtk_menu_shell_prepend (GTK_MENU_SHELL (popup), mitem);
          g_signal_connect (G_OBJECT (mitem), "activate", G_CALLBACK (save_as_file_cb), NULL);
          gtk_widget_show (mitem);

          mitem = gtk_menu_item_new_with_mnemonic (_("_Save"));
          gtk_menu_shell_prepend (GTK_MENU_SHELL (popup), mitem);
          g_signal_connect (G_OBJECT (mitem), "activate", G_CALLBACK (save_file_cb), NULL);
          gtk_widget_show (mitem);

          if (!options.common_data.uri)
            gtk_widget_set_sensitive (mitem, FALSE);
        }

      mitem = gtk_menu_item_new_with_mnemonic (_("_Open..."));
      gtk_menu_shell_prepend (GTK_MENU_SHELL (popup), mitem);
      g_signal_connect (G_OBJECT (mitem), "activate", G_CALLBACK (open_file_cb), NULL);
      gtk_widget_show (mitem);

      /* at bootom */
      mitem = gtk_separator_menu_item_new ();
      gtk_menu_shell_append (GTK_MENU_SHELL (popup), mitem);
      gtk_widget_show (mitem);

      mitem = gtk_menu_item_new_with_mnemonic (_("_Quit"));
      gtk_menu_shell_append (GTK_MENU_SHELL (popup), mitem);
      g_signal_connect (G_OBJECT (mitem), "activate", G_CALLBACK (quit_cb), NULL);
      gtk_widget_show (mitem);
    }
}

static gboolean
key_press_cb (GtkWidget *w, GdkEventKey *key, gpointer d)
{
  if ((key->state & GDK_CONTROL_MASK) && (key->keyval == GDK_KEY_F || key->keyval == GDK_KEY_f))
    {
      if (search_bar == NULL)
        return FALSE;

      ignore_esc = TRUE;
      gtk_search_bar_set_search_mode (GTK_SEARCH_BAR (search_bar->bar), TRUE);
      return gtk_search_bar_handle_event (GTK_SEARCH_BAR (search_bar->bar), (GdkEvent *) key);
    }
  else if (options.common_data.file_op)
    {
      if ((key->state & GDK_CONTROL_MASK) && (key->keyval == GDK_KEY_O || key->keyval == GDK_KEY_o))
        {
          open_file_cb (NULL, NULL);
          return TRUE;
        }
      else if ((key->state & GDK_CONTROL_MASK) && (key->keyval == GDK_KEY_S || key->keyval == GDK_KEY_s))
        {
          save_file_cb (NULL, NULL);
          return TRUE;
        }
      else if ((key->state & GDK_CONTROL_MASK) && (key->state & GDK_SHIFT_MASK) &&
               (key->keyval == GDK_KEY_S || key->keyval == GDK_KEY_s))
        {
          save_as_file_cb (NULL, NULL);
          return TRUE;
        }
      else if ((key->state & GDK_CONTROL_MASK) && (key->keyval == GDK_KEY_Q || key->keyval == GDK_KEY_q))
        {
          quit_cb (NULL, NULL);
          return TRUE;
        }
    }

  return FALSE;
}

/* mouse actions */
static gboolean
tag_event_cb (GtkTextTag * tag, GObject * obj, GdkEvent * ev, GtkTextIter * iter, gpointer d)
{
  GtkTextIter start = *iter;
  GtkTextIter end = *iter;
  gchar *url;

  if (ev->type == GDK_BUTTON_PRESS)
    {
      GdkEventButton *bev = (GdkEventButton *) ev;

      if (bev->button == 1)
        {
          gtk_text_iter_backward_to_tag_toggle (&start, tag);
          gtk_text_iter_forward_to_tag_toggle (&end, tag);

          url = gtk_text_iter_get_text (&start, &end);
          open_uri (url);
          g_free (url);

          return TRUE;
        }
    }

  return FALSE;
}

static gboolean hovering_over_link = FALSE;

static gboolean
motion_cb (GtkWidget * w, GdkEventMotion * ev, gpointer d)
{
  gint x, y;
  GSList *tags = NULL, *tagp = NULL;
  GtkTextIter iter;
  gboolean hovering = FALSE;

  gtk_text_view_window_to_buffer_coords (GTK_TEXT_VIEW (w), GTK_TEXT_WINDOW_WIDGET, ev->x, ev->y, &x, &y);

  gtk_text_view_get_iter_at_location (GTK_TEXT_VIEW (w), &iter, x, y);

  tags = gtk_text_iter_get_tags (&iter);
  for (tagp = tags; tagp != NULL; tagp = tagp->next)
    {
      GtkTextTag *t = tagp->data;
      gint link = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (t), "is_link"));

      if (link)
        {
          hovering = TRUE;
          break;
        }
    }

  if (hovering != hovering_over_link)
    {
      hovering_over_link = hovering;

      if (hovering_over_link)
        gdk_window_set_cursor (gtk_text_view_get_window (GTK_TEXT_VIEW (w), GTK_TEXT_WINDOW_TEXT), hand);
      else
        gdk_window_set_cursor (gtk_text_view_get_window (GTK_TEXT_VIEW (w), GTK_TEXT_WINDOW_TEXT), normal);
    }

  if (tags)
    g_slist_free (tags);

  return FALSE;
}

static void
linkify_cb (GtkTextBuffer * buf, GRegex * regex)
{
  gchar *text;
  GtkTextIter start, end;
  GMatchInfo *match;

  gtk_text_buffer_get_bounds (buf, &start, &end);
  text = gtk_text_buffer_get_text (buf, &start, &end, FALSE);

  gtk_text_buffer_remove_all_tags (buf, &start, &end);

  if (g_regex_match (regex, text, G_REGEX_MATCH_NOTEMPTY, &match))
    {
      do
        {
          gint sp, ep, spos, epos;

          g_match_info_fetch_pos (match, 0, &sp, &ep);

          /* positions are in bytes, not character, so here we must normalize it */
          spos = g_utf8_pointer_to_offset (text, text + sp);
          epos = g_utf8_pointer_to_offset (text, text + ep);

          gtk_text_buffer_get_iter_at_offset (buf, &start, spos);
          gtk_text_buffer_get_iter_at_offset (buf, &end, epos);

          gtk_text_buffer_apply_tag (buf, tag, &start, &end);
        }
      while (g_match_info_next (match, NULL));
    }
  g_match_info_free (match);

  g_free (text);
}

#if HAVE_SOURCEVIEW
static void
line_mark_activated (GtkSourceGutter *gutter, GtkTextIter *iter, GdkEventButton *ev, gpointer d)
{
  GSList *mark_list;
  const gchar *mark_type;

  if (ev->button == 1)
    mark_type = SV_MARK1;
  else if (ev->button == 3)
    mark_type = SV_MARK2;
  else
    return;

  /* get the marks already in the line */
  mark_list = gtk_source_buffer_get_source_marks_at_line (GTK_SOURCE_BUFFER (text_buffer),
                                                          gtk_text_iter_get_line (iter), mark_type);

  if (mark_list != NULL)
    gtk_text_buffer_delete_mark (GTK_TEXT_BUFFER (text_buffer), GTK_TEXT_MARK (mark_list->data));
  else
    gtk_source_buffer_create_source_mark (GTK_SOURCE_BUFFER (text_buffer), NULL, mark_type, iter);

  g_slist_free (mark_list);
}
#endif

/* data loading */
static gboolean
handle_stdin (GIOChannel * channel, GIOCondition condition, gpointer data)
{
  if ((condition & G_IO_IN) || (condition & (G_IO_IN | G_IO_HUP)))
    {
      GString *string;
      GError *err = NULL;
      gint status;
#ifdef HAVE_SOURCEVIEW
      GtkSourceLanguage *lang = NULL;
#endif

      string = g_string_new (NULL);
      while (channel->is_readable != TRUE)
        usleep (100);

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
              g_printerr ("yad_text_handle_stdin(): %s\n", err->message);
              g_error_free (err);
              err = NULL;
            }
          /* stop handling */
          g_io_channel_shutdown (channel, TRUE, NULL);
          return FALSE;
        }

      if (string->str[0] == '\014')
        {
          GtkTextIter start, end;

          /* clear text if ^L received */
          gtk_text_buffer_get_start_iter (GTK_TEXT_BUFFER (text_buffer), &start);
          gtk_text_buffer_get_end_iter (GTK_TEXT_BUFFER (text_buffer), &end);
          gtk_text_buffer_delete (GTK_TEXT_BUFFER (text_buffer), &start, &end);
        }
      else if (string->len > 0)
        {
          GtkTextIter end;

          gtk_text_buffer_get_end_iter (GTK_TEXT_BUFFER (text_buffer), &end);

          if (!g_utf8_validate (string->str, string->len, NULL))
            {
              gchar *utftext =
                g_convert_with_fallback (string->str, string->len, "UTF-8", "ISO-8859-1", NULL, NULL, NULL, NULL);
              if (options.text_data.formatted && !options.common_data.editable)
                gtk_text_buffer_insert_markup (GTK_TEXT_BUFFER (text_buffer), &end, utftext, -1);
              else
                gtk_text_buffer_insert (GTK_TEXT_BUFFER (text_buffer), &end, utftext, -1);
              g_free (utftext);
            }
          else
            {
              if (options.text_data.formatted && !options.common_data.editable)
                gtk_text_buffer_insert_markup (GTK_TEXT_BUFFER (text_buffer), &end, string->str, string->len);
              else
                gtk_text_buffer_insert (GTK_TEXT_BUFFER (text_buffer), &end, string->str, string->len);
            }

          if (options.common_data.tail)
            {
              while (gtk_events_pending ())
                gtk_main_iteration ();
              gtk_text_buffer_get_end_iter (GTK_TEXT_BUFFER (text_buffer), &end);
              gtk_text_view_scroll_to_iter (GTK_TEXT_VIEW (text_view), &end, 0, FALSE, 0, 0);
            }
        }

#ifdef HAVE_SOURCEVIEW
      if (options.source_data.lang)
        lang = gtk_source_language_manager_get_language (gtk_source_language_manager_get_default (), options.source_data.lang);
      else
        {
          gchar *ctype;
           if (options.common_data.mime && *options.common_data.mime)
             ctype = g_content_type_from_mime_type (options.common_data.mime);
           else
             ctype = g_content_type_guess (NULL, string->str, string->len, NULL);
           lang = gtk_source_language_manager_guess_language (gtk_source_language_manager_get_default (), options.common_data.uri, ctype);
          g_free (ctype);
        }
      gtk_source_buffer_set_language (GTK_SOURCE_BUFFER (text_buffer), lang);
#endif

      g_string_free (string, TRUE);
    }

  return TRUE;
}

static void
fill_buffer_from_file ()
{
  GtkTextIter iter;
#ifdef HAVE_SOURCEVIEW
  GtkSourceLanguage *lang;
#endif
  gchar *buf;
  gsize len;
  GError *err = NULL;

  if (options.common_data.uri == NULL)
    return;

  if (!g_file_get_contents (options.common_data.uri, &buf, &len, &err))
    {
      g_printerr (_("Cannot open file '%s': %s\n"), options.common_data.uri, err->message);
      return;
    }

  if (len <= 0)
    return;

  gtk_text_buffer_get_iter_at_offset (GTK_TEXT_BUFFER (text_buffer), &iter, 0);

  if (!g_utf8_validate (buf, -1, NULL))
    {
      gchar *utftext =
        g_convert_with_fallback (buf, -1, "UTF-8", "ISO-8859-1", NULL, NULL, NULL, NULL);
      if (options.text_data.formatted && !options.common_data.editable)
        gtk_text_buffer_insert_markup (GTK_TEXT_BUFFER (text_buffer), &iter, utftext, -1);
      else
        gtk_text_buffer_insert (GTK_TEXT_BUFFER (text_buffer), &iter, utftext, -1);
      g_free (utftext);
    }
  else
    {
      if (options.text_data.formatted && !options.common_data.editable)
        gtk_text_buffer_insert_markup (GTK_TEXT_BUFFER (text_buffer), &iter, buf, -1);
      else
        gtk_text_buffer_insert (GTK_TEXT_BUFFER (text_buffer), &iter, buf, -1);
    }
  g_free (buf);

#ifdef HAVE_SOURCEVIEW
  if (options.source_data.lang)
    lang = gtk_source_language_manager_get_language (gtk_source_language_manager_get_default (), options.source_data.lang);
  else
    {
      gchar *ctype = NULL;

      if (options.common_data.mime && *options.common_data.mime)
        ctype = g_content_type_from_mime_type (options.common_data.mime);
      else
        ctype = g_content_type_guess (options.common_data.uri, NULL, 0, NULL);
      lang = gtk_source_language_manager_guess_language (gtk_source_language_manager_get_default (), options.common_data.uri, ctype);
      g_free (ctype);
    }
  gtk_source_buffer_set_language (GTK_SOURCE_BUFFER (text_buffer), lang);
#endif
}

static void
fill_buffer_from_stdin ()
{
  GIOChannel *channel;

  channel = g_io_channel_unix_new (0);
  g_io_channel_set_encoding (channel, NULL, NULL);
  g_io_channel_set_flags (channel, G_IO_FLAG_NONBLOCK, NULL);
  g_io_add_watch (channel, G_IO_IN | G_IO_HUP, handle_stdin, NULL);
}

static void
text_changed_cb (GtkTextBuffer *b, gpointer d)
{
  text_changed = TRUE;
}

void
text_goto_line ()
{
  if (options.common_data.uri)
    {
      GtkTextIter iter;

      while (gtk_events_pending ())
        gtk_main_iteration ();

      gtk_text_buffer_get_iter_at_line (GTK_TEXT_BUFFER (text_buffer), &iter, options.text_data.line);
      gtk_text_buffer_place_cursor (GTK_TEXT_BUFFER (text_buffer), &iter);
      gtk_text_view_scroll_to_iter (GTK_TEXT_VIEW (text_view), &iter, 0, TRUE, 0, 0.5);
    }
}

GtkWidget *
text_create_widget (GtkWidget * dlg)
{
  GtkWidget *w, *sw, *tv;

  w = gtk_box_new (GTK_ORIENTATION_VERTICAL, 2);

  sw = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (sw), GTK_SHADOW_ETCHED_IN);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (sw), options.data.hscroll_policy, options.data.vscroll_policy);
  gtk_box_pack_start (GTK_BOX (w), sw, TRUE, TRUE, 0);

#ifdef HAVE_SOURCEVIEW
  text_buffer = (GObject *) gtk_source_buffer_new (NULL);
  tv = text_view = gtk_source_view_new_with_buffer (GTK_SOURCE_BUFFER (text_buffer));
#else
  text_buffer = (GObject *) gtk_text_buffer_new (NULL);
  tv = text_view = gtk_text_view_new_with_buffer (GTK_TEXT_BUFFER (text_buffer));
#endif
  gtk_widget_set_name (text_view, "yad-text-widget");
  gtk_text_view_set_justification (GTK_TEXT_VIEW (text_view), options.text_data.justify);
  gtk_text_view_set_left_margin (GTK_TEXT_VIEW (text_view), options.text_data.margins);
  gtk_text_view_set_right_margin (GTK_TEXT_VIEW (text_view), options.text_data.margins);
  gtk_text_view_set_editable (GTK_TEXT_VIEW (text_view), options.common_data.editable);
  if (!options.common_data.editable && options.text_data.hide_cursor)
    gtk_text_view_set_cursor_visible (GTK_TEXT_VIEW (text_view), FALSE);

  if (options.text_data.wrap)
    gtk_text_view_set_wrap_mode (GTK_TEXT_VIEW (text_view), GTK_WRAP_WORD_CHAR);

  gtk_text_view_set_monospace (GTK_TEXT_VIEW (text_view), TRUE);

  if (options.common_data.font || options.text_data.fore || options.text_data.back)
    {
      GtkCssProvider *provider;
      GtkStyleContext *context;
      GString *css;

      css = g_string_new (".view, .view text {\n");
      if (options.common_data.font)
        {
          gchar *font = pango_to_css (options.common_data.font);
          g_string_append_printf (css, "font: %s;\n", font);
          g_free (font);
        }
      if (options.text_data.fore)
        g_string_append_printf (css, "color: %s;\n", options.text_data.fore);
      if (options.text_data.back)
        g_string_append_printf (css, "background-color: %s;\n", options.text_data.back);
      g_string_append (css, "}\n");

      provider = gtk_css_provider_new ();
      gtk_css_provider_load_from_data (provider, css->str, -1, NULL);
      context = gtk_widget_get_style_context (text_view);
      gtk_style_context_add_provider (context, GTK_STYLE_PROVIDER (provider), GTK_STYLE_PROVIDER_PRIORITY_USER);

      g_string_free (css, TRUE);
    }

#ifdef HAVE_SOURCEVIEW
  if (options.source_data.theme && *options.source_data.theme)
    {
      GtkSourceStyleScheme *scheme = NULL;
      GtkSourceStyleSchemeManager *mgr;
      const gchar **ids;

      mgr = gtk_source_style_scheme_manager_get_default ();
      ids = (const gchar **) gtk_source_style_scheme_manager_get_scheme_ids (mgr);
      if (ids)
        {
          gint i;
          gboolean found = FALSE;

          for (i = 0; ids[i]; i++)
            {
              const gchar *name;

              scheme = gtk_source_style_scheme_manager_get_scheme (mgr, ids[i]);
              name = gtk_source_style_scheme_get_name (scheme);
              if (strcmp (name, options.source_data.theme) == 0)
                {
                  found = TRUE;
                  break;
                }
            }

          if (!found)
            scheme = NULL;
        }

      if (scheme)
        gtk_source_buffer_set_style_scheme (GTK_SOURCE_BUFFER (text_buffer), scheme);
      else
        g_printerr (_("Theme %s not found\n"), options.source_data.theme);
    }

  gtk_source_view_set_show_line_numbers (GTK_SOURCE_VIEW (text_view), options.source_data.line_num);
  gtk_source_view_set_highlight_current_line (GTK_SOURCE_VIEW (text_view), options.source_data.line_hl);
  if (options.source_data.line_marks)
    {
      GdkRGBA color;
      GtkSourceMarkAttributes *attr;

      gtk_source_view_set_show_line_marks (GTK_SOURCE_VIEW (text_view), TRUE);

      gdk_rgba_parse (&color, options.source_data.m1_color);
      attr = gtk_source_mark_attributes_new ();
      gtk_source_mark_attributes_set_background (attr, &color);
      gtk_source_mark_attributes_set_icon_name (attr, "checkbox-checked-symbolic");
      gtk_source_view_set_mark_attributes (GTK_SOURCE_VIEW (text_view), SV_MARK1, attr, 0);
      g_object_unref (attr);

      gdk_rgba_parse (&color, options.source_data.m2_color);
      attr = gtk_source_mark_attributes_new ();
      gtk_source_mark_attributes_set_background (attr, &color);
      gtk_source_mark_attributes_set_icon_name (attr, "checkbox-checked-symbolic");
      gtk_source_view_set_mark_attributes (GTK_SOURCE_VIEW (text_view), SV_MARK2, attr, 0);
      g_object_unref (attr);

      g_signal_connect (G_OBJECT (text_view), "line-mark-activated", G_CALLBACK (line_mark_activated), NULL);
    }
  if (options.source_data.right_margin > 0)
    {
      gtk_source_view_set_show_right_margin (GTK_SOURCE_VIEW (text_view), TRUE);
      gtk_source_view_set_right_margin_position (GTK_SOURCE_VIEW (text_view), options.source_data.right_margin);
    }
  if (options.source_data.indent)
    {
      gtk_source_view_set_auto_indent (GTK_SOURCE_VIEW (text_view), TRUE);
      gtk_source_view_set_indent_on_tab (GTK_SOURCE_VIEW (text_view), TRUE);
    }
  gtk_source_view_set_tab_width (GTK_SOURCE_VIEW (text_view), options.source_data.tab_width);
  gtk_source_view_set_indent_width (GTK_SOURCE_VIEW (text_view), options.source_data.indent_width);
  gtk_source_view_set_smart_home_end (GTK_SOURCE_VIEW (text_view), options.source_data.smart_he);
  gtk_source_view_set_smart_backspace (GTK_SOURCE_VIEW (text_view), options.source_data.smart_bs);
  gtk_source_view_set_insert_spaces_instead_of_tabs (GTK_SOURCE_VIEW (text_view), options.source_data.spaces);

  gtk_source_buffer_set_highlight_matching_brackets (GTK_SOURCE_BUFFER (text_buffer), options.source_data.brackets);
#endif

#ifdef HAVE_SPELL
  if (options.common_data.enable_spell)
    {
      GspellTextView *spell_view = gspell_text_view_get_from_gtk_text_view (GTK_TEXT_VIEW (text_view));
      gspell_text_view_basic_setup (spell_view);;
    }
#endif

  /* Add keyboard handler */
  g_signal_connect (text_view, "key-press-event", G_CALLBACK (key_press_cb), dlg);

  if (options.common_data.editable)
    g_signal_connect (G_OBJECT (text_buffer), "changed", G_CALLBACK (text_changed_cb), NULL);

  if (options.common_data.file_op)
    g_signal_connect_after (G_OBJECT (text_view), "populate-popup", G_CALLBACK (menu_popup_cb), dlg);

  /* Initialize linkifying */
  if (options.text_data.uri)
    {
      GRegex *regex;

      regex = g_regex_new (YAD_URL_REGEX,
                           G_REGEX_CASELESS | G_REGEX_OPTIMIZE | G_REGEX_EXTENDED, G_REGEX_MATCH_NOTEMPTY, NULL);

      /* Create text tag for URI */
      tag = gtk_text_buffer_create_tag (GTK_TEXT_BUFFER (text_buffer), NULL,
                                        "foreground", options.text_data.uri_color,
                                        "underline", PANGO_UNDERLINE_SINGLE,
                                        NULL);
      g_object_set_data (G_OBJECT (tag), "is_link", GINT_TO_POINTER (1));
      g_signal_connect (G_OBJECT (tag), "event", G_CALLBACK (tag_event_cb), NULL);

      /* Create cursors */
      hand = gdk_cursor_new_for_display (gdk_display_get_default (), GDK_HAND2);
      normal = gdk_cursor_new_for_display (gdk_display_get_default (), GDK_XTERM);
      g_signal_connect (G_OBJECT (text_view), "motion-notify-event", G_CALLBACK (motion_cb), NULL);

      g_signal_connect_after (G_OBJECT (text_buffer), "changed", G_CALLBACK (linkify_cb), regex);
    }

  gtk_container_add (GTK_CONTAINER (sw), tv);

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

  /* load data */
  if (options.common_data.uri)
    fill_buffer_from_file ();

  if (options.common_data.listen || options.common_data.uri == NULL)
    fill_buffer_from_stdin ();

  return w;
}

void
text_print_result (void)
{
  if (!options.common_data.editable)
    return;

  if (options.text_data.in_place && options.common_data.uri)
    {
      if (text_changed)
        {
          if (options.text_data.confirm_save)
            {
              if (yad_confirm_dlg (GTK_WINDOW (gtk_widget_get_toplevel (text_view)),
                                   options.text_data.confirm_text))
                save_file_cb (NULL, NULL);
            }
          else
            save_file_cb (NULL, NULL);
        }
    }
  else
    {
      GtkTextIter start, end;
      gchar *text;

      gtk_text_buffer_get_bounds (GTK_TEXT_BUFFER (text_buffer), &start, &end);
      text = gtk_text_buffer_get_text (GTK_TEXT_BUFFER (text_buffer), &start, &end, 0);
      g_print ("%s", text);
      g_free (text);
    }
}
