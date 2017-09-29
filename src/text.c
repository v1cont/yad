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

#include <errno.h>

#include <pango/pango.h>

#include "yad.h"

static GtkWidget *text_view;
static GObject *text_buffer;
static GtkTextTag *tag;
static GdkCursor *hand, *normal;
static gchar *pattern = NULL;
static gboolean new_search = TRUE;

/* searching */
static void
do_search (GtkWidget * e, GtkWidget * w)
{
  static gchar *text = NULL;
  static guint offset;
  static GRegex *regex = NULL;
  GMatchInfo *match = NULL;
  GtkTextIter begin, end;

  g_free (pattern);
  pattern = g_strdup (gtk_entry_get_text (GTK_ENTRY (e)));
  gtk_widget_destroy (w);
  gtk_widget_queue_draw (text_view);

  if (new_search || gtk_text_buffer_get_modified (GTK_TEXT_BUFFER (text_buffer)))
    {
      /* get the text */
      g_free (text);
      gtk_text_buffer_get_bounds (GTK_TEXT_BUFFER (text_buffer), &begin, &end);
      text = gtk_text_buffer_get_text (GTK_TEXT_BUFFER (text_buffer), &begin, &end, FALSE);
      offset = 0;
      /* compile new regex */
      if (regex)
        g_regex_unref (regex);
      regex = g_regex_new (pattern, G_REGEX_EXTENDED | G_REGEX_OPTIMIZE, G_REGEX_MATCH_NOTEMPTY, NULL);
      new_search = FALSE;
    }

  /* search and select if found */
  if (g_regex_match (regex, text + offset, G_REGEX_MATCH_NOTEMPTY, &match))
    {
      gint sp, ep, spos, epos;

      g_match_info_fetch_pos (match, 0, &sp, &ep);

      /* positions are in bytes, not character, so here we must normalize it */
      spos = g_utf8_pointer_to_offset (text, text + sp + offset);
      epos = g_utf8_pointer_to_offset (text, text + ep + offset);

      gtk_text_buffer_get_iter_at_offset (GTK_TEXT_BUFFER (text_buffer), &begin, spos);
      gtk_text_buffer_get_iter_at_offset (GTK_TEXT_BUFFER (text_buffer), &end, epos);

      gtk_text_buffer_select_range (GTK_TEXT_BUFFER (text_buffer), &begin, &end);
      gtk_text_view_scroll_to_iter (GTK_TEXT_VIEW (text_view), &begin, 0, FALSE, 0, 0);

      offset += epos;

      g_match_info_free (match);
      match = NULL;
    }
  else
    new_search = TRUE;
}

static gboolean
search_key_cb (GtkWidget * w, GdkEventKey * key, GtkWidget * win)
{
#if GTK_CHECK_VERSION(2,24,0)
  if (key->keyval == GDK_KEY_Escape)
#else
  if (key->keyval == GDK_Escape)
#endif
    {
      gtk_widget_destroy (win);
      return TRUE;
    }
  return FALSE;
}

static void
search_changed (GtkWidget * w, gpointer d)
{
  new_search = TRUE;
}

static void
show_search ()
{
  GtkWidget *w, *f, *a, *e;
  GdkEvent *fev;

  w = gtk_window_new (GTK_WINDOW_POPUP);
  gtk_window_set_transient_for (GTK_WINDOW (w), GTK_WINDOW (gtk_widget_get_toplevel (text_view)));
  gtk_window_set_position (GTK_WINDOW (w), GTK_WIN_POS_CENTER_ON_PARENT);
  /* next two lines needs for get focus to search window */
  gtk_window_set_type_hint (GTK_WINDOW (w), GDK_WINDOW_TYPE_HINT_UTILITY);
  gtk_window_set_modal (GTK_WINDOW (w), TRUE);

  g_signal_connect (G_OBJECT (w), "key-press-event", G_CALLBACK (search_key_cb), w);

  f = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (f), GTK_SHADOW_ETCHED_IN);
  gtk_container_add (GTK_CONTAINER (w), f);

  a = gtk_alignment_new (0.5, 0.5, 1.0, 1.0);
  gtk_alignment_set_padding (GTK_ALIGNMENT (a), 2, 2, 2, 2);
  gtk_container_add (GTK_CONTAINER (f), a);

  e = gtk_entry_new ();
  if (pattern)
    gtk_entry_set_text (GTK_ENTRY (e), pattern);
  gtk_container_add (GTK_CONTAINER (a), e);

  g_signal_connect (G_OBJECT (e), "activate", G_CALLBACK (do_search), w);
  g_signal_connect (G_OBJECT (e), "changed", G_CALLBACK (search_changed), NULL);
  g_signal_connect (G_OBJECT (e), "key-press-event", G_CALLBACK (search_key_cb), w);

  gtk_widget_show_all (w);

  /* send focus event to search entry (so complex due to popup window) */
  fev = gdk_event_new (GDK_FOCUS_CHANGE);
  fev->focus_change.type = GDK_FOCUS_CHANGE;
  fev->focus_change.in = TRUE;
  fev->focus_change.window = gtk_widget_get_window (e);
  if (fev->focus_change.window != NULL)
    g_object_ref (fev->focus_change.window);
#if GTK_CHECK_VERSION(2,22,0)
  gtk_widget_send_focus_change (e, fev);
#else
  g_object_ref (e);
  GTK_OBJECT_FLAGS (e) |= GTK_HAS_FOCUS;
  gtk_widget_event (e, fev);
  g_object_notify (G_OBJECT (e), "has-focus");
  g_object_unref (e);
#endif
  gdk_event_free (fev);
}

static gboolean
key_press_cb (GtkWidget * w, GdkEventKey * key, gpointer data)
{
#if GTK_CHECK_VERSION(2,24,0)
  if ((key->state & GDK_CONTROL_MASK) && (key->keyval == GDK_KEY_S || key->keyval == GDK_KEY_s))
#else
  if ((key->state & GDK_CONTROL_MASK) && (key->keyval == GDK_S || key->keyval == GDK_s))
#endif
    {
      show_search ();
      return TRUE;
    }

  return FALSE;
}

static gboolean
tag_event_cb (GtkTextTag * tag, GObject * obj, GdkEvent * ev, GtkTextIter * iter, gpointer d)
{
  GtkTextIter start = *iter;
  GtkTextIter end = *iter;
  gchar *url, *cmdline;

  if (ev->type == GDK_BUTTON_PRESS)
    {
      GdkEventButton *bev = (GdkEventButton *) ev;

      if (bev->button == 1)
        {
          gtk_text_iter_backward_to_tag_toggle (&start, tag);
          gtk_text_iter_forward_to_tag_toggle (&end, tag);

          url = gtk_text_iter_get_text (&start, &end);
          cmdline = g_strdup_printf (settings.open_cmd, url);
          g_free (url);

          g_spawn_command_line_async (cmdline, NULL);

          g_free (cmdline);
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
      GtkTextTag *tag = tagp->data;
      gint link = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (tag), "is_link"));

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

  gdk_window_get_pointer (gtk_widget_get_window (w), NULL, NULL, NULL);

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

static gboolean
handle_stdin (GIOChannel * channel, GIOCondition condition, gpointer data)
{
  if ((condition & G_IO_IN) || (condition & (G_IO_IN | G_IO_HUP)))
    {
      GString *string;
      GError *err = NULL;
      gint status;

      string = g_string_new (NULL);
      while (channel->is_readable != TRUE);

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
              gtk_text_buffer_insert (GTK_TEXT_BUFFER (text_buffer), &end, utftext, -1);
              g_free (utftext);
            }
          else
            gtk_text_buffer_insert (GTK_TEXT_BUFFER (text_buffer), &end, string->str, string->len);

          if (options.common_data.tail)
            {
              while (gtk_events_pending ())
                gtk_main_iteration ();
              gtk_text_buffer_get_end_iter (GTK_TEXT_BUFFER (text_buffer), &end);
              gtk_text_view_scroll_to_iter (GTK_TEXT_VIEW (text_view), &end, 0, FALSE, 0, 0);
            }
        }

      g_string_free (string, TRUE);
    }

#ifdef HAVE_SOURCEVIEW
  if (options.source_data.lang)
    {
      GtkSourceLanguage *lang = gtk_source_language_manager_get_language (gtk_source_language_manager_get_default (),
                                                                          options.source_data.lang);
      gtk_source_buffer_set_language (GTK_SOURCE_BUFFER (text_buffer), lang);
    }
#endif

  return TRUE;
}

static void
fill_buffer_from_file ()
{
  GtkTextIter iter, end;
#ifdef HAVE_SOURCEVIEW
  GtkSourceLanguage *lang;
#endif
  FILE *f;
  gchar buf[2048];
  gint remaining = 0;

  if (options.common_data.uri == NULL)
    return;

  f = fopen (options.common_data.uri, "r");

  if (f == NULL)
    {
      g_printerr (_("Cannot open file '%s': %s\n"), options.common_data.uri, g_strerror (errno));
      return;
    }

  gtk_text_buffer_get_iter_at_offset (GTK_TEXT_BUFFER (text_buffer), &iter, 0);

  while (!feof (f))
    {
      gint count;
      const char *leftover;
      int to_read = 2047 - remaining;

      count = fread (buf + remaining, 1, to_read, f);
      buf[count + remaining] = '\0';

      g_utf8_validate (buf, count + remaining, &leftover);

      g_assert (g_utf8_validate (buf, leftover - buf, NULL));
      gtk_text_buffer_insert (GTK_TEXT_BUFFER (text_buffer), &iter, buf, leftover - buf);

      remaining = (buf + remaining + count) - leftover;
      memmove (buf, leftover, remaining);

      if (remaining > 6 || count < to_read)
        break;
    }

  if (remaining)
    {
      g_printerr (_("Invalid UTF-8 data encountered reading file %s\n"), options.common_data.uri);
      return;
    }

  /* We had a newline in the buffer to begin with. (The buffer always contains
   * a newline, so we delete to the end of the buffer to clean up.
   */

  gtk_text_buffer_get_end_iter (GTK_TEXT_BUFFER (text_buffer), &end);
  gtk_text_buffer_delete (GTK_TEXT_BUFFER (text_buffer), &iter, &end);
  gtk_text_buffer_set_modified (GTK_TEXT_BUFFER (text_buffer), FALSE);

#ifdef HAVE_SOURCEVIEW
  if (options.source_data.lang)
    lang = gtk_source_language_manager_get_language (gtk_source_language_manager_get_default (), options.source_data.lang);
  else
    lang = gtk_source_language_manager_guess_language (gtk_source_language_manager_get_default (), options.common_data.uri, NULL);
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

GtkWidget *
text_create_widget (GtkWidget * dlg)
{
  GtkWidget *w;
  PangoFontDescription *fd;

  w = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (w), GTK_SHADOW_ETCHED_IN);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (w), options.hscroll_policy, options.vscroll_policy);

#ifdef HAVE_SOURCEVIEW
  text_buffer = (GObject *) gtk_source_buffer_new (NULL);
  text_view = gtk_source_view_new_with_buffer (GTK_SOURCE_BUFFER (text_buffer));
#else
  text_buffer = (GObject *) gtk_text_buffer_new (NULL);
  text_view = gtk_text_view_new_with_buffer (GTK_TEXT_BUFFER (text_buffer));
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

  if (options.text_data.fore)
    {
#if GTK_CHECK_VERSION(3,0,0)
      GdkRGBA clr;
      if (gdk_rgba_parse (&clr, options.text_data.fore))
        gtk_widget_override_color (text_view, GTK_STATE_FLAG_NORMAL, &clr);
#else
      GdkColor clr;
      if (gdk_color_parse (options.text_data.fore, &clr))
        gtk_widget_modify_text (text_view, GTK_STATE_NORMAL, &clr);
#endif
    }

  if (options.text_data.back)
    {
#if GTK_CHECK_VERSION(3,0,0)
      GdkRGBA clr;
      if (gdk_rgba_parse (&clr, options.text_data.back))
        gtk_widget_override_background_color (text_view, GTK_STATE_FLAG_NORMAL, &clr);
#else
      GdkColor clr;
      if (gdk_color_parse (options.text_data.back, &clr))
        gtk_widget_modify_base (text_view, GTK_STATE_NORMAL, &clr);
#endif
    }

#ifdef HAVE_SOURCEVIEW
  if (options.source_data.theme)
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
#endif

  /* set font */
  if (options.common_data.font)
    fd = pango_font_description_from_string (options.common_data.font);
  else
    fd = pango_font_description_from_string ("Monospace");

#if GTK_CHECK_VERSION(3,0,0)
  gtk_widget_override_font (text_view, fd);
#else
  gtk_widget_modify_font (text_view, fd);
#endif
  pango_font_description_free (fd);

#ifdef HAVE_SPELL
  if (options.common_data.enable_spell)
    {
      GtkSpellChecker *spell = gtk_spell_checker_new ();
      gtk_spell_checker_set_language (spell, options.common_data.spell_lang, NULL);
      gtk_spell_checker_attach (spell, GTK_TEXT_VIEW (text_view));
    }
#endif

  /* Add submit on ctrl+enter */
  g_signal_connect (text_view, "key-press-event", G_CALLBACK (key_press_cb), dlg);

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
      hand = gdk_cursor_new (GDK_HAND2);
      normal = gdk_cursor_new (GDK_XTERM);
      g_signal_connect (G_OBJECT (text_view), "motion-notify-event", G_CALLBACK (motion_cb), NULL);

      g_signal_connect_after (G_OBJECT (text_buffer), "changed", G_CALLBACK (linkify_cb), regex);
    }

  gtk_container_add (GTK_CONTAINER (w), text_view);

  if (options.common_data.uri)
    fill_buffer_from_file ();

  if (options.common_data.listen || options.common_data.uri == NULL)
    fill_buffer_from_stdin ();
  else
    {
      /* place cursor at start of file */
      GtkTextIter iter;

      gtk_text_buffer_get_iter_at_line (GTK_TEXT_BUFFER (text_buffer), &iter, 0);
      gtk_text_buffer_place_cursor (GTK_TEXT_BUFFER (text_buffer), &iter);
    }

  return w;
}

void
text_print_result (void)
{
  GtkTextIter start, end;
  gchar *text;

  if (!options.common_data.editable)
    return;

  gtk_text_buffer_get_bounds (GTK_TEXT_BUFFER (text_buffer), &start, &end);
  text = gtk_text_buffer_get_text (GTK_TEXT_BUFFER (text_buffer), &start, &end, 0);
  g_print ("%s", text);
  g_free (text);
}
