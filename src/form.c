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
 * Copyright (C) 2008-2020, Victor Ananjevsky <victor@sanana.kiev.ua>
 */

#include <ctype.h>
#include <stdlib.h>

#include "yad.h"

#include "calendar.xpm"

static GSList *fields = NULL;
static guint n_fields;

static gboolean disable_changed = TRUE;

/* expand %N in command to fields values */
static GString *
expand_action (gchar * cmd)
{
  GString *xcmd;
  guint i = 0;

  xcmd = g_string_new ("");
  while (cmd[i])
    {
      if (cmd[i] == '%')
        {
          i++;
          if (g_ascii_isdigit (cmd[i]))
            {
              YadField *fld;
              gchar *buf, *arg;
              guint num, j = i;

              /* get field num */
              while (g_ascii_isdigit (cmd[j]))
                j++;
              buf = g_strndup (cmd + i, j - i);
              num = g_ascii_strtoll (buf, NULL, 10);
              g_free (buf);
              if (num > 0 && num <= n_fields)
                num--;
              else
                continue;

              /* get field value */
              arg = NULL;
              fld = g_slist_nth_data (options.form_data.fields, num);
              switch (fld->type)
                {
                case YAD_FIELD_SIMPLE:
                case YAD_FIELD_HIDDEN:
                case YAD_FIELD_READ_ONLY:
                case YAD_FIELD_COMPLETE:
                case YAD_FIELD_FILE_SAVE:
                case YAD_FIELD_DIR_CREATE:
                case YAD_FIELD_MFILE:
                case YAD_FIELD_MDIR:
                case YAD_FIELD_DATE:
                case YAD_FIELD_ICON:
                  buf = escape_char ((gchar *) gtk_entry_get_text (GTK_ENTRY (g_slist_nth_data (fields, num))), '"');
                  arg = g_shell_quote (buf ? buf : "");
                  g_free (buf);
                  break;
                case YAD_FIELD_NUM:
                  {
                    guint prec = gtk_spin_button_get_digits (GTK_SPIN_BUTTON (g_slist_nth_data (fields, num)));
                    arg = g_strdup_printf ("%.*f", prec, gtk_spin_button_get_value (GTK_SPIN_BUTTON (g_slist_nth_data (fields, num))));
                    break;
                  }
                case YAD_FIELD_CHECK:
                  arg = g_strdup (print_bool_val (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (g_slist_nth_data (fields, num)))));
                  break;
                case YAD_FIELD_COMBO:
                case YAD_FIELD_COMBO_ENTRY:
                  buf = gtk_combo_box_text_get_active_text (GTK_COMBO_BOX_TEXT (g_slist_nth_data (fields, num)));
                  arg = g_shell_quote (buf ? buf : "");
                  g_free (buf);
                  break;
                case YAD_FIELD_SWITCH:
                  arg = g_strdup (print_bool_val (gtk_switch_get_state (GTK_SWITCH (g_slist_nth_data (fields, num)))));
                  break;
                case YAD_FIELD_SCALE:
                  arg = g_strdup_printf ("%d", (gint) gtk_range_get_value (GTK_RANGE (g_slist_nth_data (fields, num))));
                  break;
                case YAD_FIELD_FILE:
                case YAD_FIELD_DIR:
                  arg = g_shell_quote (gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (g_slist_nth_data (fields, num))));
                  break;
                case YAD_FIELD_FONT:
                  arg = g_shell_quote (gtk_font_chooser_get_font (GTK_FONT_CHOOSER (g_slist_nth_data (fields, num))));
                  break;
                case YAD_FIELD_LINK:
                  arg = g_shell_quote (gtk_link_button_get_uri (GTK_LINK_BUTTON (g_slist_nth_data (fields, num))));
                  break;
                case YAD_FIELD_APP:
                  {
                    GList *wl = gtk_container_get_children (GTK_CONTAINER (g_slist_nth_data (fields, num)));
                    GAppInfo *info = gtk_app_chooser_get_app_info (GTK_APP_CHOOSER (wl->data));
                    arg =  g_shell_quote (g_app_info_get_executable (info));
                    g_object_unref (info);
                    break;
                  }
                case YAD_FIELD_COLOR:
                  {
                    GdkRGBA c;
                    GtkColorChooser *cb = GTK_COLOR_CHOOSER (g_slist_nth_data (fields, num));
                    gtk_color_chooser_get_rgba (cb, &c);
                    buf = get_color (&c);
                    arg = g_shell_quote (buf ? buf : "");
                    g_free (buf);
                    break;
                  }
                case YAD_FIELD_TEXT:
                  {
                    GtkTextBuffer *tb;
                    GtkTextIter b, e;
                    gchar *txt;

                    tb = gtk_text_view_get_buffer (GTK_TEXT_VIEW (g_slist_nth_data (fields, num)));
                    gtk_text_buffer_get_bounds (tb, &b, &e);
                    txt = gtk_text_buffer_get_text (tb, &b, &e, FALSE);

                    /* escape special chars */
                    buf = escape_str (txt);
                    g_free (txt);

                    /* escape quotes */
                    txt = escape_char (buf, '"');
                    g_free (buf);

                    arg = g_shell_quote (txt ? txt : "");
                    g_free (txt);
                  }
                default: ;
                }
              if (arg)
                {
                  g_string_append (xcmd, arg);
                  g_free (arg);
                }
              i = j;
            }
          else
            {
              g_string_append_c (xcmd, cmd[i]);
              i++;
            }
        }
      else
        {
          g_string_append_c (xcmd, cmd[i]);
          i++;
        }
    }
  g_string_append_c (xcmd, '\0');

  return xcmd;
}

static void
set_field_value (guint num, gchar *value)
{
  GtkWidget *w;
  gchar **s;
  YadField *fld = g_slist_nth_data (options.form_data.fields, num);

  w = GTK_WIDGET (g_slist_nth_data (fields, num));
  if (g_ascii_strcasecmp (value, "@disabled@") == 0)
    {
      gtk_widget_set_sensitive (w, FALSE);
      return;
    }
  else
    gtk_widget_set_sensitive (w, TRUE);

  switch (fld->type)
    {
    case YAD_FIELD_READ_ONLY:
      gtk_widget_set_sensitive (w, FALSE);
    case YAD_FIELD_SIMPLE:
    case YAD_FIELD_HIDDEN:
    case YAD_FIELD_MFILE:
    case YAD_FIELD_MDIR:
    case YAD_FIELD_FILE_SAVE:
    case YAD_FIELD_DIR_CREATE:
    case YAD_FIELD_DATE:
    case YAD_FIELD_ICON:
      gtk_entry_set_text (GTK_ENTRY (w), value);
      break;

    case YAD_FIELD_NUM:
      s = g_strsplit (value, options.common_data.item_separator, -1);
      if (s[0])
        {
          gdouble val = g_ascii_strtod (s[0], NULL);
          w = g_slist_nth_data (fields, num);
          if (s[1])
            {
              gchar **s1 = g_strsplit (s[1], "..", 2);
              if (s1[0] && s1[1])
                {
                  gdouble min, max;
                  min = g_ascii_strtod (s1[0], NULL);
                  max = g_ascii_strtod (s1[1], NULL);
                  if (min < max)
                    gtk_spin_button_set_range (GTK_SPIN_BUTTON (w), min, max);
                }
              g_strfreev (s1);
              if (s[2])
                {
                  gdouble step = g_ascii_strtod (s[2], NULL);
                  gtk_spin_button_set_increments (GTK_SPIN_BUTTON (w), step, step * 10);
                  if (s[3])
                    {
                      guint prec = (guint) g_ascii_strtoull (s[3], NULL, 0);
                      if (prec > 20)
                        prec = 20;
                      gtk_spin_button_set_digits (GTK_SPIN_BUTTON (w), prec);
                    }
                }
            }
          /* set initial value must be after setting range and step */
          gtk_spin_button_set_value (GTK_SPIN_BUTTON (w), val);
        }
      g_strfreev (s);
      break;

    case YAD_FIELD_CHECK:
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (w), get_bool_val (value));
      break;

    case YAD_FIELD_SWITCH:
      gtk_switch_set_state (GTK_SWITCH (w), get_bool_val (value));
      break;

    case YAD_FIELD_COMPLETE:
      {
        GtkEntryCompletion *c;
        GtkTreeModel *m;
        GtkTreeIter it;
        gint i = 0, def = -1;

        c = gtk_entry_get_completion (GTK_ENTRY (w));
        m = gtk_entry_completion_get_model (GTK_ENTRY_COMPLETION (c));
        gtk_list_store_clear (GTK_LIST_STORE (m));

        s = g_strsplit (value, options.common_data.item_separator, -1);
        while (s[i])
          {
            gtk_list_store_append (GTK_LIST_STORE (m), &it);
            if (s[i][0] == '^')
              {
                gtk_list_store_set (GTK_LIST_STORE (m), &it, 0, g_strcompress (s[i] + 1), -1);
                def = i;
              }
            else
              gtk_list_store_set (GTK_LIST_STORE (m), &it, 0, g_strcompress (s[i]), -1);

            i++;
          }
        if (def >= 0)
          gtk_entry_set_text (GTK_ENTRY (w), s[def] + 1);
        else
          gtk_entry_set_text (GTK_ENTRY (w), "");
        g_strfreev (s);
        break;
      }

    case YAD_FIELD_COMBO:
    case YAD_FIELD_COMBO_ENTRY:
      {
        GtkTreeModel *m;
        gint i = 0, def = 0;

        /* cleanup previous values */
        m = gtk_combo_box_get_model (GTK_COMBO_BOX (w));
        gtk_list_store_clear (GTK_LIST_STORE (m));

        s = g_strsplit (value, options.common_data.item_separator, -1);
        while (s[i])
          {
            gchar *buf;

            if (s[i][0] == '^')
              {
                buf = g_strcompress (s[i] + 1);
                def = i;
              }
            else
              buf = g_strcompress (s[i]);
            gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (w), buf);
            g_free (buf);
            i++;
          }
        gtk_combo_box_set_active (GTK_COMBO_BOX (w), def);
        g_strfreev (s);
        break;
      }

    case YAD_FIELD_DIR:
      gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (w), value);
    case YAD_FIELD_FILE:
      gtk_file_chooser_set_filename (GTK_FILE_CHOOSER (w), value);
      break;

    case YAD_FIELD_FONT:
      gtk_font_chooser_set_font (GTK_FONT_CHOOSER (w), value);
      break;

    case YAD_FIELD_SCALE:
      gtk_range_set_value (GTK_RANGE (w), atoi (value));
      break;

    case YAD_FIELD_APP:
      {
        GtkWidget *b;
        GList *wl;

        for (wl = gtk_container_get_children (GTK_CONTAINER (w)); wl; wl = wl->next)
          {
            GtkWidget *cw = GTK_WIDGET (wl->data);
            gtk_container_remove (GTK_CONTAINER (w), cw);
            gtk_widget_destroy (cw);
          }

        if (value && value[0])
          b = gtk_app_chooser_button_new (value);
        else
          b = gtk_app_chooser_button_new ("text/plain");
        gtk_widget_set_name (b, "yad-form-app");
        gtk_box_pack_start (GTK_BOX (w), b, TRUE, TRUE, 0);
        gtk_widget_show_all (w);
        break;
      }

    case YAD_FIELD_COLOR:
      {
        GdkRGBA c;
        gdk_rgba_parse (&c, value);
        gtk_color_chooser_set_rgba (GTK_COLOR_CHOOSER (w), &c);
        break;
      }

    case YAD_FIELD_BUTTON:
    case YAD_FIELD_FULL_BUTTON:
      g_object_set_data_full (G_OBJECT (w), "cmd", g_strdup (value), g_free);
      break;

    case YAD_FIELD_LINK:
      gtk_link_button_set_uri (GTK_LINK_BUTTON (w), value);
      break;

    case YAD_FIELD_TEXT:
      {
        GtkTextIter iter;
        GtkTextBuffer *tb = gtk_text_view_get_buffer (GTK_TEXT_VIEW (w));
        gchar *txt = g_strcompress (value);
        gtk_text_buffer_set_text (tb, "", -1);
        gtk_text_buffer_get_start_iter (tb, &iter);
        if (options.data.no_markup)
          gtk_text_buffer_insert (tb, &iter, txt, -1);
        else
          gtk_text_buffer_insert_markup (tb, &iter, txt, -1);
        g_free (txt);
        break;
      }

    default:;
    }
}

static void
parse_cmd_output (gchar *data)
{
  guint i = 0;
  gchar **lines;

  if (!data)
    return;

  lines = g_strsplit (data, "\n", 0);

  disable_changed = TRUE;
  while (lines[i] && lines[i][0])
    {
      gint fn;
      gchar *ptr = lines[i];

      while (isblank (*ptr)) ptr++;

      if (isdigit (*ptr))
        {
          gchar **ln = g_strsplit (ptr, ":", 2);
          fn = g_ascii_strtoll (ln[0], NULL, 10);
          if (fn && ln[1])
            set_field_value (fn - 1, ln[1]);
          g_strfreev (ln);
        }
      i++;
    }
  disable_changed = FALSE;
}

static void
button_clicked_cb (GtkButton * b, gpointer d)
{
  gchar *action = (gchar *) g_object_get_data (G_OBJECT (b), "cmd");

  if (action && action[0])
    {
      GString *cmd;
      if (action[0] == '@')
        {
          gchar *data = NULL;
          gint exit = 1;
          cmd = expand_action (action + 1);
          exit = run_command_sync (cmd->str, &data);
          if (exit == 0)
            parse_cmd_output (data);
          g_free (data);
        }
      else
        {
          cmd = expand_action (action);
          run_command_async (cmd->str);
        }
      g_string_free (cmd, TRUE);
    }

  /* set focus to specified field */
  if (options.form_data.focus_field > 0 && options.form_data.focus_field <= n_fields)
    gtk_widget_grab_focus (GTK_WIDGET (g_slist_nth_data (fields, options.form_data.focus_field - 1)));
}

static void
field_changed_cb (GtkWidget *w, guint fn)
{
  gchar *data = NULL;
  gint exit = 1;

  if (disable_changed)
    return;

  if (options.form_data.changed_action)
    {
      gchar *str;
      GString *cmd;

      str = g_strdup_printf ("%s %d %%%d", options.form_data.changed_action, fn + 1, fn + 1);
      cmd = expand_action (str);
      g_free (str);

      exit = run_command_sync (cmd->str, &data);
      if (exit == 0)
        parse_cmd_output (data);
      g_free (data);

      g_string_free (cmd, TRUE);
    }
}

static void
switch_changed_cb (GObject *obj, GParamSpec *spec, gpointer data)
{
  field_changed_cb (GTK_WIDGET (obj), GPOINTER_TO_INT (data));
}

static void
form_activate_cb (GtkEntry * entry, gpointer data)
{
  if (options.plug == -1)
    yad_exit (options.data.def_resp);
}

static void
select_files_cb (GtkEntry * entry, GtkEntryIconPosition pos, GdkEventButton * event, gpointer data)
{
  GtkWidget *dlg;
  GList *filt;
  static gchar *path = NULL;

  if (event->button == 1)
    {
      YadFieldType type = GPOINTER_TO_INT (data);

      if (!path)
        {
          const gchar *val = gtk_entry_get_text (entry);

          if (g_file_test (val, G_FILE_TEST_IS_DIR))
            path = g_strdup (val);
          else
            path = val ? g_path_get_dirname (val) : g_get_current_dir ();
        }

      if (type == YAD_FIELD_MFILE)
        {
          dlg = gtk_file_chooser_dialog_new (_("Select files"),
                                             GTK_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (entry))),
                                             GTK_FILE_CHOOSER_ACTION_OPEN,
                                             _("Cancel"), GTK_RESPONSE_CANCEL,
                                             _("OK"), GTK_RESPONSE_ACCEPT, NULL);
        }
      else
        {
          dlg = gtk_file_chooser_dialog_new (_("Select folders"),
                                             GTK_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (entry))),
                                             GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER,
                                             _("Cancel"), GTK_RESPONSE_CANCEL,
                                             _("OK"), GTK_RESPONSE_ACCEPT, NULL);
        }
      gtk_file_chooser_set_select_multiple (GTK_FILE_CHOOSER (dlg), TRUE);
      gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (dlg), path);

      g_signal_connect (dlg, "map", G_CALLBACK (gtk_file_chooser_set_show_hidden), GINT_TO_POINTER (options.common_data.show_hidden));

      /* add preview */
      if (options.common_data.preview)
        {
          GtkWidget *p = gtk_image_new ();
          gtk_file_chooser_set_preview_widget (GTK_FILE_CHOOSER (dlg), p);
          g_signal_connect (dlg, "update-preview", G_CALLBACK (update_preview), p);
        }

      /* add filters */
      for (filt = options.common_data.filters; filt; filt = filt->next)
        gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (dlg), GTK_FILE_FILTER (filt->data));

      if (gtk_dialog_run (GTK_DIALOG (dlg)) == GTK_RESPONSE_ACCEPT)
        {
          GSList *files, *ptr;
          GString *str;

          str = g_string_new ("");
          files = ptr = gtk_file_chooser_get_uris (GTK_FILE_CHOOSER (dlg));

          while (ptr)
            {
              if (ptr->data)
                {
                  gchar *fn = g_filename_from_uri ((gchar *) ptr->data, NULL, NULL);
                  g_string_append (str, fn);
                  g_string_append (str, options.common_data.item_separator);
                  g_free (fn);
                }
              ptr = ptr->next;
            }

          str->str[str->len - 1] = '\0';        // remove last item separator
          gtk_entry_set_text (entry, str->str);

          g_slist_free (files);
          g_string_free (str, TRUE);
        }

      g_free (path);
      path = gtk_file_chooser_get_current_folder (GTK_FILE_CHOOSER (dlg));
      gtk_widget_destroy (dlg);
    }
}

static void
create_files_cb (GtkEntry * entry, GtkEntryIconPosition pos, GdkEventButton * event, gpointer data)
{
  GtkWidget *dlg;
  GList *filt;
  static gchar *path = NULL;

  if (event->button == 1)
    {
      YadFieldType type = GPOINTER_TO_INT (data);

      if (!path)
        {
          const gchar *val = gtk_entry_get_text (entry);

          if (g_file_test (val, G_FILE_TEST_IS_DIR))
            path = g_strdup (val);
          else
            path = val ? g_path_get_dirname (val) : g_get_current_dir ();
        }

      if (type == YAD_FIELD_FILE_SAVE)
        {
          dlg = gtk_file_chooser_dialog_new (_("Select or create file"),
                                             GTK_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (entry))),
                                             GTK_FILE_CHOOSER_ACTION_SAVE,
                                             _("Cancel"), GTK_RESPONSE_CANCEL,
                                             _("OK"), GTK_RESPONSE_ACCEPT, NULL);
        }
      else
        {
          dlg = gtk_file_chooser_dialog_new (_("Select or create folder"),
                                             GTK_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (entry))),
                                             GTK_FILE_CHOOSER_ACTION_CREATE_FOLDER,
                                             _("Cancel"), GTK_RESPONSE_CANCEL,
                                             _("OK"), GTK_RESPONSE_ACCEPT, NULL);
        }
      gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (dlg), path);

      g_signal_connect (dlg, "map", G_CALLBACK (gtk_file_chooser_set_show_hidden), GINT_TO_POINTER (options.common_data.show_hidden));

      /* add preview */
      if (options.common_data.preview)
        {
          GtkWidget *p = gtk_image_new ();
          gtk_file_chooser_set_preview_widget (GTK_FILE_CHOOSER (dlg), p);
          g_signal_connect (dlg, "update-preview", G_CALLBACK (update_preview), p);
        }

      /* add filters */
      for (filt = options.common_data.filters; filt; filt = filt->next)
        gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (dlg), GTK_FILE_FILTER (filt->data));

      if (gtk_dialog_run (GTK_DIALOG (dlg)) == GTK_RESPONSE_ACCEPT)
        {
          gchar *file = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dlg));

          gtk_entry_set_text (entry, file);
          g_free (file);
        }

      g_free (path);
      path = gtk_file_chooser_get_current_folder (GTK_FILE_CHOOSER (dlg));
      gtk_widget_destroy (dlg);
    }
}

static void
date_selected_cb (GtkCalendar * c, gpointer data)
{
  gtk_dialog_response (GTK_DIALOG (data), GTK_RESPONSE_ACCEPT);
}

static void
select_date_cb (GtkEntry * entry, GtkEntryIconPosition pos, GdkEventButton * event, gpointer data)
{
  GtkWidget *dlg, *cal;

  if (event->button == 1)
    {
      GDate *d;

      dlg = gtk_dialog_new_with_buttons (_("Select date"),
                                         GTK_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (entry))),
                                         GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                         _("Cancel"), GTK_RESPONSE_CANCEL,
                                         _("OK"), GTK_RESPONSE_ACCEPT, NULL);
      cal = gtk_calendar_new ();
      gtk_widget_show (cal);
      g_signal_connect (G_OBJECT (cal), "day-selected-double-click", G_CALLBACK (date_selected_cb), dlg);
      gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dlg))), cal, TRUE, TRUE, 5);

      d = g_date_new ();
      g_date_set_parse (d, gtk_entry_get_text (entry));
      if (g_date_valid (d))
        {
          gtk_calendar_select_day (GTK_CALENDAR (cal), g_date_get_day (d));
          gtk_calendar_select_month (GTK_CALENDAR (cal), g_date_get_month (d) - 1, g_date_get_year (d));
        }
      g_date_free (d);

      if (gtk_dialog_run (GTK_DIALOG (dlg)) == GTK_RESPONSE_ACCEPT)
        {
          guint day, month, year;
          gchar *format = options.common_data.date_format;
          gchar time_string[128];

          gtk_calendar_get_date (GTK_CALENDAR (cal), &day, &month, &year);
          d = g_date_new_dmy (year, month + 1, day);
          g_date_strftime (time_string, 127, format, d);
          gtk_entry_set_text (entry, time_string);
          g_date_free (d);
        }
      gtk_widget_destroy (dlg);
    }

  gtk_widget_grab_focus (GTK_WIDGET (entry));
}

static gboolean
link_clicked_cb (GtkLinkButton *btn, gpointer data)
{
  const gchar *uri = gtk_link_button_get_uri (btn);
  open_uri (uri);
  return TRUE;
}

static void
select_icon_cb (GtkEntry *e, GtkEntryIconPosition pos, GdkEventButton *ev, gpointer d)
{
  gchar *ib;

  if (ev->button !=1 || pos != GTK_ENTRY_ICON_SECONDARY)
    return;

  ib = g_find_program_in_path ("yad-icon-browser");
  if (ib)
    {
      gint exit;
      gchar *cmd;
      gchar *out = NULL;

      cmd = g_strdup_printf ("%s -i", ib);
      g_free (ib);

      exit = run_command_sync (cmd, &out);
      g_free (cmd);
      if (exit == 0)
        {
          strip_new_line (out);
          gtk_entry_set_text (e, out);
          g_free (out);
        }
    }
}

static void
set_icon_cb (GtkEntry *e, gpointer d)
{
  const gchar *icon = gtk_entry_get_text (e);

  if (icon && *icon)
    gtk_entry_set_icon_from_icon_name (e, GTK_ENTRY_ICON_PRIMARY, icon);
}

static gboolean
handle_stdin (GIOChannel * ch, GIOCondition cond, gpointer data)
{
  static guint cnt = 0;

  if ((cond == G_IO_IN) || (cond == G_IO_IN + G_IO_HUP))
    {
      GError *err = NULL;
      GString *string = g_string_new (NULL);

      while (ch->is_readable != TRUE);

      do
        {
          gint status;

          if (cnt == n_fields)
            {
              if (options.form_data.cycle_read)
                cnt = 0;
              else
                goto shutdown;
            }

          do
            {
              status = g_io_channel_read_line_string (ch, string, NULL, &err);

              while (gtk_events_pending ())
                gtk_main_iteration ();
            }
          while (status == G_IO_STATUS_AGAIN);

          if (status != G_IO_STATUS_NORMAL)
            {
              if (err)
                {
                  g_printerr ("yad_form_handle_stdin(): %s\n", err->message);
                  g_error_free (err);
                  err = NULL;
                }
              /* stop handling */
              goto shutdown;
            }

          strip_new_line (string->str);
          if (string->str[0])
            {
              if (string->str[0] == '\014')
                {
                  gint i;
                  /* clear the form and reset fields counter */
                  for (i = 0; i < n_fields; i++)
                    set_field_value (i, "");
                  cnt = -1; /* must be -1 due to next increment */
                }
              else
                set_field_value (cnt, string->str);
            }
          cnt++;
        }
      while (g_io_channel_get_buffer_condition (ch) == G_IO_IN);
      g_string_free (string, TRUE);
    }

  if ((cond != G_IO_IN) && (cond != G_IO_IN + G_IO_HUP))
    goto shutdown;

  return TRUE;

 shutdown:
  g_io_channel_shutdown (ch, TRUE, NULL);
  disable_changed = FALSE;
  return FALSE;
}

GtkWidget *
form_create_widget (GtkWidget * dlg)
{
  GtkWidget *tbl, *w = NULL;
  GList *filt;

  if (options.form_data.fields)
    {
      GtkWidget *l, *e;
      GdkPixbuf *pb;
      guint i, col, row, rows;

      n_fields = g_slist_length (options.form_data.fields);

      row = col = 0;
      rows = n_fields / options.form_data.columns;
      if (n_fields % options.form_data.columns > 0)
        rows++;

      tbl = gtk_grid_new ();
      gtk_grid_set_row_spacing (GTK_GRID (tbl), 5);
      gtk_grid_set_column_spacing (GTK_GRID (tbl), 5);

      gtk_grid_set_row_homogeneous (GTK_GRID (tbl), options.form_data.homogeneous);
      gtk_grid_set_column_homogeneous (GTK_GRID (tbl), options.form_data.homogeneous);

      if (options.common_data.scroll)
        {
          GtkWidget *sw = gtk_scrolled_window_new (NULL, NULL);
          gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (sw), GTK_SHADOW_NONE);
          gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (sw), options.data.hscroll_policy, options.data.vscroll_policy);
          gtk_container_add (GTK_CONTAINER (sw), tbl);
          w = sw;
        }
      else
        w = tbl;

      /* create form */
      for (i = 0; i < n_fields; i++)
        {
          YadField *fld = g_slist_nth_data (options.form_data.fields, i);

          /* add field label */
          l = NULL;
          if (fld->type != YAD_FIELD_CHECK && fld->type != YAD_FIELD_BUTTON &&
              fld->type != YAD_FIELD_FULL_BUTTON && fld->type != YAD_FIELD_LINK &&
              fld->type != YAD_FIELD_LABEL && fld->type != YAD_FIELD_TEXT)
            {
              gchar *buf;

              if (fld->name)
                buf = g_strcompress (fld->name);
              else
                buf = g_strdup ("");

              l = gtk_label_new (NULL);
              if (!options.data.no_markup)
                {
                  gtk_label_set_markup_with_mnemonic (GTK_LABEL (l), buf);
                  if (fld->tip)
                    gtk_widget_set_tooltip_markup (l, fld->tip);
                }
              else
                {
                  gtk_label_set_text_with_mnemonic (GTK_LABEL (l), buf);
                  if (fld->tip)
                    gtk_widget_set_tooltip_text (l, fld->tip);
                }
              gtk_widget_set_name (l, "yad-form-flabel");
              gtk_label_set_xalign (GTK_LABEL (l), options.common_data.align);
              gtk_grid_attach (GTK_GRID (tbl), l, col * 2, row, 1, 1);
              g_free (buf);
            }

          /* add field entry */
          switch (fld->type)
            {
            case YAD_FIELD_SIMPLE:
            case YAD_FIELD_HIDDEN:
            case YAD_FIELD_READ_ONLY:
            case YAD_FIELD_COMPLETE:
            case YAD_FIELD_ICON:
              e = gtk_entry_new ();
              gtk_widget_set_name (e, "yad-form-entry");
              if (fld->tip)
                {
                  if (!options.data.no_markup)
                    gtk_widget_set_tooltip_markup (e, fld->tip);
                  else
                    gtk_widget_set_tooltip_text (e, fld->tip);
                }
              g_signal_connect (G_OBJECT (e), "activate", G_CALLBACK (form_activate_cb), dlg);
              if (fld->type == YAD_FIELD_HIDDEN)
                gtk_entry_set_visibility (GTK_ENTRY (e), FALSE);
              else if (fld->type == YAD_FIELD_READ_ONLY)
                gtk_widget_set_sensitive (e, FALSE);
              gtk_grid_attach (GTK_GRID (tbl), e, 1 + col * 2, row, 1, 1);
              gtk_widget_set_hexpand (e, TRUE);

              if (fld->type == YAD_FIELD_COMPLETE)
                {
                  GtkEntryCompletion *c = gtk_entry_completion_new ();
                  GtkListStore *m = gtk_list_store_new (1, G_TYPE_STRING);

                  gtk_entry_set_completion (GTK_ENTRY (e), c);
                  gtk_entry_completion_set_model (c, GTK_TREE_MODEL (m));
                  gtk_entry_completion_set_text_column (c, 0);

                  if (options.common_data.complete != YAD_COMPLETE_SIMPLE)
                    gtk_entry_completion_set_match_func (c, check_complete, NULL, NULL);

                  g_object_unref (m);
                  g_object_unref (c);
                }
              else if (fld->type == YAD_FIELD_ICON)
                {
                  gtk_entry_set_icon_from_icon_name (GTK_ENTRY (e), GTK_ENTRY_ICON_PRIMARY, "image-missing");
                  gtk_entry_set_icon_from_icon_name (GTK_ENTRY (e), GTK_ENTRY_ICON_SECONDARY, "insert-image");
                  g_signal_connect (G_OBJECT (e), "changed", G_CALLBACK (set_icon_cb), NULL);
                  g_signal_connect (G_OBJECT (e), "icon-press", G_CALLBACK (select_icon_cb), NULL);
                }

              gtk_label_set_mnemonic_widget (GTK_LABEL (l), e);
              fields = g_slist_append (fields, e);
              break;

            case YAD_FIELD_NUM:
              e = gtk_spin_button_new_with_range (0.0, 65525.0, 1.0);
              gtk_widget_set_name (e, "yad-form-spin");
              if (fld->tip)
                {
                  if (!options.data.no_markup)
                    gtk_widget_set_tooltip_markup (e, fld->tip);
                  else
                    gtk_widget_set_tooltip_text (e, fld->tip);
                }
              gtk_entry_set_alignment (GTK_ENTRY (e), 1.0);
              gtk_grid_attach (GTK_GRID (tbl), e, 1 + col * 2, row, 1, 1);
              gtk_widget_set_hexpand (e, TRUE);
              gtk_label_set_mnemonic_widget (GTK_LABEL (l), e);
              fields = g_slist_append (fields, e);
              break;

            case YAD_FIELD_CHECK:
              {
                gchar *buf;
                if (fld->name)
                  buf = g_strcompress (fld->name);
                else
                  buf = g_strdup ("");
                e = gtk_check_button_new_with_label (buf);
                gtk_widget_set_name (e, "yad-form-check");
                if (fld->tip)
                  {
                    if (!options.data.no_markup)
                      gtk_widget_set_tooltip_markup (e, fld->tip);
                    else
                      gtk_widget_set_tooltip_text (e, fld->tip);
                  }
                gtk_grid_attach (GTK_GRID (tbl), e, col * 2, row, 2, 1);
                gtk_widget_set_hexpand (e, TRUE);
                fields = g_slist_append (fields, e);
                g_free (buf);
                g_signal_connect_after (G_OBJECT (e), "toggled", G_CALLBACK (field_changed_cb), GINT_TO_POINTER (i));
              }
              break;

           case YAD_FIELD_SWITCH:
              {
                e = gtk_switch_new ();
                gtk_widget_set_name (e, "yad-form-switch");
                if (fld->tip)
                  {
                    if (!options.data.no_markup)
                      gtk_widget_set_tooltip_markup (e, fld->tip);
                    else
                      gtk_widget_set_tooltip_text (e, fld->tip);
                  }
                gtk_grid_attach (GTK_GRID (tbl), e, 1 + col * 2, row, 1, 1);
                gtk_widget_set_hexpand (e, TRUE);
                gtk_widget_set_halign (e, GTK_ALIGN_START); /* prevent expanding widget (make it always compact) */
                gtk_label_set_mnemonic_widget (GTK_LABEL (l), e);
                fields = g_slist_append (fields, e);
                g_signal_connect_after (G_OBJECT (e), "notify::active", G_CALLBACK (switch_changed_cb), GINT_TO_POINTER (i));
              }
              break;

            case YAD_FIELD_COMBO:
              e = gtk_combo_box_text_new ();
              gtk_widget_set_name (e, "yad-form-combo");
              if (fld->tip)
                {
                  if (!options.data.no_markup)
                    gtk_widget_set_tooltip_markup (e, fld->tip);
                  else
                    gtk_widget_set_tooltip_text (e, fld->tip);
                }
              gtk_grid_attach (GTK_GRID (tbl), e, 1 + col * 2, row, 1, 1);
              gtk_widget_set_hexpand (e, TRUE);
              gtk_label_set_mnemonic_widget (GTK_LABEL (l), e);
              fields = g_slist_append (fields, e);
              g_signal_connect_after (G_OBJECT (e), "changed", G_CALLBACK (field_changed_cb), GINT_TO_POINTER (i));
              break;

            case YAD_FIELD_COMBO_ENTRY:
              e = gtk_combo_box_text_new_with_entry ();
              gtk_widget_set_name (e, "yad-form-edit-combo");
              if (fld->tip)
                {
                  if (!options.data.no_markup)
                    gtk_widget_set_tooltip_markup (e, fld->tip);
                  else
                    gtk_widget_set_tooltip_text (e, fld->tip);
                }
              gtk_grid_attach (GTK_GRID (tbl), e, 1 + col * 2, row, 1, 1);
              gtk_widget_set_hexpand (e, TRUE);
              gtk_label_set_mnemonic_widget (GTK_LABEL (l), e);
              fields = g_slist_append (fields, e);
              break;

            case YAD_FIELD_FILE:
              e = gtk_file_chooser_button_new (_("Select file"), GTK_FILE_CHOOSER_ACTION_OPEN);
              gtk_widget_set_name (e, "yad-form-file");
              if (fld->tip)
                {
                  if (!options.data.no_markup)
                    gtk_widget_set_tooltip_markup (e, fld->tip);
                  else
                    gtk_widget_set_tooltip_text (e, fld->tip);
                }
              gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (e), g_get_current_dir ());
              gtk_grid_attach (GTK_GRID (tbl), e, 1 + col * 2, row, 1, 1);
              gtk_widget_set_hexpand (e, TRUE);
              gtk_label_set_mnemonic_widget (GTK_LABEL (l), e);
              fields = g_slist_append (fields, e);

              /* add preview */
              if (options.common_data.preview)
                {
                  GtkWidget *p = gtk_image_new ();
                  gtk_file_chooser_set_preview_widget (GTK_FILE_CHOOSER (e), p);
                  g_signal_connect (e, "update-preview", G_CALLBACK (update_preview), p);
                }

              /* add filters */
              for (filt = options.common_data.filters; filt; filt = filt->next)
                gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (e), GTK_FILE_FILTER (filt->data));

              break;

            case YAD_FIELD_DIR:
              e = gtk_file_chooser_button_new (_("Select folder"), GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER);
              gtk_widget_set_name (e, "yad-form-file");
              if (fld->tip)
                {
                  if (!options.data.no_markup)
                    gtk_widget_set_tooltip_markup (e, fld->tip);
                  else
                    gtk_widget_set_tooltip_text (e, fld->tip);
                }
              gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (e), g_get_current_dir ());
              gtk_grid_attach (GTK_GRID (tbl), e, 1 + col * 2, row, 1, 1);
              gtk_widget_set_hexpand (e, TRUE);
              gtk_label_set_mnemonic_widget (GTK_LABEL (l), e);
              fields = g_slist_append (fields, e);
              break;

            case YAD_FIELD_FONT:
              e = gtk_font_button_new ();
              gtk_widget_set_name (e, "yad-form-font");
              if (fld->tip)
                {
                  if (!options.data.no_markup)
                    gtk_widget_set_tooltip_markup (e, fld->tip);
                  else
                    gtk_widget_set_tooltip_text (e, fld->tip);
                }
              gtk_grid_attach (GTK_GRID (tbl), e, 1 + col * 2, row, 1, 1);
              gtk_widget_set_hexpand (e, TRUE);
              gtk_label_set_mnemonic_widget (GTK_LABEL (l), e);
              fields = g_slist_append (fields, e);
              break;

            case YAD_FIELD_APP:
              e = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
              if (fld->tip)
                {
                  if (!options.data.no_markup)
                    gtk_widget_set_tooltip_markup (e, fld->tip);
                  else
                    gtk_widget_set_tooltip_text (e, fld->tip);
                }
              gtk_grid_attach (GTK_GRID (tbl), e, 1 + col * 2, row, 1, 1);
              gtk_widget_set_hexpand (e, TRUE);
              gtk_label_set_mnemonic_widget (GTK_LABEL (l), e);
              fields = g_slist_append (fields, e);
              break;

            case YAD_FIELD_COLOR:
              e = gtk_color_button_new ();
              gtk_widget_set_name (e, "yad-form-color");
              if (fld->tip)
                {
                  if (!options.data.no_markup)
                    gtk_widget_set_tooltip_markup (e, fld->tip);
                  else
                    gtk_widget_set_tooltip_text (e, fld->tip);
                }
              gtk_grid_attach (GTK_GRID (tbl), e, 1 + col * 2, row, 1, 1);
              gtk_widget_set_hexpand (e, TRUE);
              gtk_label_set_mnemonic_widget (GTK_LABEL (l), e);
              fields = g_slist_append (fields, e);
              break;

            case YAD_FIELD_MFILE:
            case YAD_FIELD_MDIR:
              e = gtk_entry_new ();
              gtk_widget_set_name (e, "yad-form-entry");
              if (fld->tip)
                {
                  if (!options.data.no_markup)
                    gtk_widget_set_tooltip_markup (e, fld->tip);
                  else
                    gtk_widget_set_tooltip_text (e, fld->tip);
                }
              gtk_entry_set_icon_from_icon_name (GTK_ENTRY (e), GTK_ENTRY_ICON_SECONDARY, "document-open");
              g_signal_connect (G_OBJECT (e), "icon-press", G_CALLBACK (select_files_cb), GINT_TO_POINTER (fld->type));
              g_signal_connect (G_OBJECT (e), "activate", G_CALLBACK (form_activate_cb), dlg);
              gtk_grid_attach (GTK_GRID (tbl), e, 1 + col * 2, row, 1, 1);
              gtk_widget_set_hexpand (e, TRUE);
              gtk_label_set_mnemonic_widget (GTK_LABEL (l), e);
              fields = g_slist_append (fields, e);
              break;

            case YAD_FIELD_FILE_SAVE:
            case YAD_FIELD_DIR_CREATE:
              e = gtk_entry_new ();
              gtk_widget_set_name (e, "yad-form-entry");
              if (fld->tip)
                {
                  if (!options.data.no_markup)
                    gtk_widget_set_tooltip_markup (e, fld->tip);
                  else
                    gtk_widget_set_tooltip_text (e, fld->tip);
                }
              gtk_entry_set_icon_from_icon_name (GTK_ENTRY (e), GTK_ENTRY_ICON_SECONDARY, "document-open");
              g_signal_connect (G_OBJECT (e), "icon-press", G_CALLBACK (create_files_cb), GINT_TO_POINTER (fld->type));
              g_signal_connect (G_OBJECT (e), "activate", G_CALLBACK (form_activate_cb), dlg);
              gtk_grid_attach (GTK_GRID (tbl), e, 1 + col * 2, row, 1, 1);
              gtk_widget_set_hexpand (e, TRUE);
              gtk_label_set_mnemonic_widget (GTK_LABEL (l), e);
              fields = g_slist_append (fields, e);
              break;

            case YAD_FIELD_DATE:
              e = gtk_entry_new ();
              gtk_widget_set_name (e, "yad-form-entry");
              if (fld->tip)
                {
                  if (!options.data.no_markup)
                    gtk_widget_set_tooltip_markup (e, fld->tip);
                  else
                    gtk_widget_set_tooltip_text (e, fld->tip);
                }
              pb = gdk_pixbuf_new_from_xpm_data (calendar_xpm);
              gtk_entry_set_icon_from_pixbuf (GTK_ENTRY (e), GTK_ENTRY_ICON_SECONDARY, pb);
              g_object_unref (pb);
              g_signal_connect (G_OBJECT (e), "icon-press", G_CALLBACK (select_date_cb), e);
              g_signal_connect (G_OBJECT (e), "activate", G_CALLBACK (form_activate_cb), dlg);
              gtk_grid_attach (GTK_GRID (tbl), e, 1 + col * 2, row, 1, 1);
              gtk_widget_set_hexpand (e, TRUE);
              gtk_label_set_mnemonic_widget (GTK_LABEL (l), e);
              fields = g_slist_append (fields, e);
              break;

            case YAD_FIELD_SCALE:
              e = gtk_scale_new_with_range (GTK_ORIENTATION_HORIZONTAL, 0.0, 100.0, 1.0);
              gtk_widget_set_name (e, "yad-form-scale");
              if (fld->tip)
                {
                  if (!options.data.no_markup)
                    gtk_widget_set_tooltip_markup (e, fld->tip);
                  else
                    gtk_widget_set_tooltip_text (e, fld->tip);
                }
              gtk_scale_set_value_pos (GTK_SCALE (e), GTK_POS_LEFT);
              gtk_grid_attach (GTK_GRID (tbl), e, 1 + col * 2, row, 1, 1);
              gtk_widget_set_hexpand (e, TRUE);
              gtk_label_set_mnemonic_widget (GTK_LABEL (l), e);
              fields = g_slist_append (fields, e);
              break;

            case YAD_FIELD_BUTTON:
            case YAD_FIELD_FULL_BUTTON:
              e = gtk_button_new ();
              gtk_widget_set_name (e, "yad-form-button");
              if (fld->tip)
                {
                  if (!options.data.no_markup)
                    gtk_widget_set_tooltip_markup (e, fld->tip);
                  else
                    gtk_widget_set_tooltip_text (e, fld->tip);
                }
              g_signal_connect (G_OBJECT (e), "clicked", G_CALLBACK (button_clicked_cb), NULL);
              l = get_label (fld->name, 2, e);
              gtk_container_add (GTK_CONTAINER (e), l);
              if (options.form_data.align_buttons)
                gtk_widget_set_halign (l, options.common_data.align);
              if (fld->type == YAD_FIELD_BUTTON)
                gtk_button_set_relief (GTK_BUTTON (e), GTK_RELIEF_NONE);
              gtk_grid_attach (GTK_GRID (tbl), e, col * 2, row, 2, 1);
              gtk_widget_set_hexpand (e, TRUE);
              fields = g_slist_append (fields, e);
              break;

            case YAD_FIELD_LINK:
              {
                gchar *buf;
                if (fld->name)
                  buf = g_strdup (fld->name[0] ? fld->name : _("Link"));
                else
                  buf = g_strdup (_("Link"));

                e = gtk_link_button_new_with_label ("", buf);
                gtk_widget_set_name (e, "yad-form-link");
                if (fld->tip)
                  {
                    if (!options.data.no_markup)
                      gtk_widget_set_tooltip_markup (e, fld->tip);
                    else
                      gtk_widget_set_tooltip_text (e, fld->tip);
                  }
                g_signal_connect (G_OBJECT (e), "activate-link", G_CALLBACK (link_clicked_cb), NULL);
                gtk_grid_attach (GTK_GRID (tbl), e, col * 2, row, 2, 1);
                gtk_widget_set_hexpand (e, TRUE);
                fields = g_slist_append (fields, e);
                g_free (buf);
                break;
              }

            case YAD_FIELD_LABEL:
              if (fld->name && fld->name[0])
                {
                  gchar *buf = g_strcompress (fld->name);
                  e = gtk_label_new (NULL);
                  gtk_widget_set_name (e, "yad-form-label");
                  if (fld->tip)
                    {
                      if (!options.data.no_markup)
                        gtk_widget_set_tooltip_markup (e, fld->tip);
                      else
                        gtk_widget_set_tooltip_text (e, fld->tip);
                    }
                  if (options.data.no_markup)
                    gtk_label_set_text (GTK_LABEL (e), buf);
                  else
                    gtk_label_set_markup (GTK_LABEL (e), buf);
                  gtk_label_set_line_wrap (GTK_LABEL (e), TRUE);
                  gtk_label_set_selectable (GTK_LABEL (e), options.data.selectable_labels);
                  gtk_label_set_xalign (GTK_LABEL (e), options.common_data.align);
                  g_free (buf);
                }
              else
                {
                  e = gtk_separator_new (GTK_ORIENTATION_HORIZONTAL);
                  gtk_widget_set_name (e, "yad-form-separator");
                }
              gtk_grid_attach (GTK_GRID (tbl), e, col * 2, row, 2, 1);
              gtk_widget_set_hexpand (e, TRUE);
              fields = g_slist_append (fields, e);
              break;

            case YAD_FIELD_TEXT:
              {
                GtkWidget *sw, *b;
                gchar *ltxt;

                if (fld->name)
                  ltxt = g_strcompress (fld->name);
                else
                  ltxt = g_strdup ("");

                b = gtk_box_new (GTK_ORIENTATION_VERTICAL, 2);
                l = gtk_label_new ("");
                gtk_label_set_xalign (GTK_LABEL (l), 0.0);
                if (options.data.no_markup)
                  gtk_label_set_text (GTK_LABEL (l), ltxt);
                else
                  gtk_label_set_markup (GTK_LABEL (l), ltxt);
                g_free (ltxt);
                gtk_box_pack_start (GTK_BOX (b), l, FALSE, FALSE, 0);

                sw = gtk_scrolled_window_new (NULL, NULL);
                gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (sw), GTK_SHADOW_ETCHED_IN);
                gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (sw), options.data.hscroll_policy, options.data.vscroll_policy);
                gtk_box_pack_start (GTK_BOX (b), sw, TRUE, TRUE, 0);

                e = gtk_text_view_new ();
                gtk_widget_set_name (e, "yad-form-text");
                if (fld->tip)
                  {
                    if (!options.data.no_markup)
                      gtk_widget_set_tooltip_markup (e, fld->tip);
                    else
                      gtk_widget_set_tooltip_text (e, fld->tip);
                  }
                gtk_text_view_set_editable (GTK_TEXT_VIEW (e), TRUE);
                gtk_text_view_set_wrap_mode (GTK_TEXT_VIEW (e), GTK_WRAP_WORD_CHAR);
                gtk_container_add (GTK_CONTAINER (sw), e);

#ifdef HAVE_SPELL
                if (options.common_data.enable_spell)
                  {
                    GspellTextView *spell_view = gspell_text_view_get_from_gtk_text_view (GTK_TEXT_VIEW (e));
                    gspell_text_view_basic_setup (spell_view);
                  }
#endif

                gtk_grid_attach (GTK_GRID (tbl), b, col * 2, row, 2, 1);
                gtk_widget_set_hexpand (b, TRUE);
                gtk_widget_set_vexpand (b, TRUE);
                gtk_label_set_mnemonic_widget (GTK_LABEL (l), e);
                fields = g_slist_append (fields, e);

                break;
              }
            }

          /* increase row and column */
          row++;
          if (row >= rows)
            {
              row = 0;
              col++;
            }
        }

      /* fill entries with data */
      if (options.extra_data)
        {
          i = 0;
          while (options.extra_data[i] && i < n_fields)
            {
              set_field_value (i, options.extra_data[i]);
              i++;
            }
        }
      else
        {
          GIOChannel *channel = g_io_channel_unix_new (0);
          g_io_channel_set_encoding (channel, NULL, NULL);
          g_io_channel_set_flags (channel, G_IO_FLAG_NONBLOCK, NULL);
          g_io_add_watch (channel, G_IO_IN | G_IO_HUP, handle_stdin, NULL);
        }
    }

  if (options.form_data.focus_field > 0 && options.form_data.focus_field <= n_fields)
    gtk_widget_grab_focus (GTK_WIDGET (g_slist_nth_data (fields, options.form_data.focus_field - 1)));

  disable_changed = FALSE;

  return w;
}

static void
form_print_field (guint fn)
{
  gchar *buf;
  YadField *fld = g_slist_nth_data (options.form_data.fields, fn);

  switch (fld->type)
    {
    case YAD_FIELD_SIMPLE:
    case YAD_FIELD_HIDDEN:
    case YAD_FIELD_READ_ONLY:
    case YAD_FIELD_COMPLETE:
    case YAD_FIELD_MFILE:
    case YAD_FIELD_MDIR:
    case YAD_FIELD_FILE_SAVE:
    case YAD_FIELD_DIR_CREATE:
    case YAD_FIELD_DATE:
    case YAD_FIELD_ICON:
      if (options.common_data.quoted_output)
        {
          buf = g_shell_quote (gtk_entry_get_text (GTK_ENTRY (g_slist_nth_data (fields, fn))));
          g_printf ("%s%s", buf, options.common_data.separator);
          g_free (buf);
        }
      else
        g_printf ("%s%s", gtk_entry_get_text (GTK_ENTRY (g_slist_nth_data (fields, fn))),
                  options.common_data.separator);
      break;
    case YAD_FIELD_NUM:
      {
        guint prec = gtk_spin_button_get_digits (GTK_SPIN_BUTTON (g_slist_nth_data (fields, fn)));
        if (options.common_data.quoted_output)
          g_printf ("'%.*f'%s", prec, gtk_spin_button_get_value (GTK_SPIN_BUTTON (g_slist_nth_data (fields, fn))),
                    options.common_data.separator);
        else
          g_printf ("%.*f%s", prec, gtk_spin_button_get_value (GTK_SPIN_BUTTON (g_slist_nth_data (fields, fn))),
                    options.common_data.separator);
        break;
      }
    case YAD_FIELD_CHECK:
      if (options.common_data.quoted_output)
        g_printf ("'%s'%s", print_bool_val (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (g_slist_nth_data (fields, fn)))),
                  options.common_data.separator);
      else
        g_printf ("%s%s", print_bool_val (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (g_slist_nth_data (fields, fn)))),
                  options.common_data.separator);
      break;
    case YAD_FIELD_SWITCH:
      if (options.common_data.quoted_output)
        g_printf ("'%s'%s", print_bool_val (gtk_switch_get_state (GTK_SWITCH (g_slist_nth_data (fields, fn)))),
                  options.common_data.separator);
      else
        g_printf ("%s%s", print_bool_val (gtk_switch_get_state (GTK_SWITCH (g_slist_nth_data (fields, fn)))),
                  options.common_data.separator);
      break;
    case YAD_FIELD_COMBO:
    case YAD_FIELD_COMBO_ENTRY:
      if (options.common_data.num_output && fld->type == YAD_FIELD_COMBO)
        g_printf ("%d%s", gtk_combo_box_get_active (GTK_COMBO_BOX (g_slist_nth_data (fields, fn))) + 1,
                  options.common_data.separator);
      else if (options.common_data.quoted_output)
        {
          buf = g_shell_quote (gtk_combo_box_text_get_active_text (GTK_COMBO_BOX_TEXT (g_slist_nth_data (fields, fn))));
          g_printf ("%s%s", buf, options.common_data.separator);
          g_free (buf);
        }
      else
        g_printf ("%s%s", gtk_combo_box_text_get_active_text (GTK_COMBO_BOX_TEXT (g_slist_nth_data (fields, fn))),
                  options.common_data.separator);
      break;
    case YAD_FIELD_FILE:
    case YAD_FIELD_DIR:
      if (options.common_data.quoted_output)
        {
          gchar *fname = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (g_slist_nth_data (fields, fn)));
          buf = g_shell_quote (fname ? fname : "");
          g_free (fname);
          g_printf ("%s%s", buf ? buf : "", options.common_data.separator);
          g_free (buf);
        }
      else
        {
          buf = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (g_slist_nth_data (fields, fn)));
          g_printf ("%s%s", buf ? buf : "", options.common_data.separator);
          g_free (buf);
        }
      break;
    case YAD_FIELD_FONT:
      {
        gchar *fname = gtk_font_chooser_get_font (GTK_FONT_CHOOSER (g_slist_nth_data (fields, fn)));
        if (options.common_data.quoted_output)
          g_printf ("'%s'%s", fname ? fname : "", options.common_data.separator);
        else
          g_printf ("%s%s", fname ? fname : "", options.common_data.separator);
        g_free (fname);
        break;
      }
    case YAD_FIELD_APP:
      {
        gchar *exec;
        GAppInfo *info = NULL;
        GList *wl = gtk_container_get_children (GTK_CONTAINER (g_slist_nth_data (fields, fn)));

        if (wl)
          {
            info = gtk_app_chooser_get_app_info (GTK_APP_CHOOSER (wl->data));
            if (info)
              exec = (gchar *) g_app_info_get_executable (info);
            else
              exec = "";
          }
        else
          exec = "";

        if (options.common_data.quoted_output)
          {
            buf = g_shell_quote (exec);
            g_printf ("%s%s", buf, options.common_data.separator);
            g_free (buf);
          }
        else
          g_printf ("%s%s", exec, options.common_data.separator);
        if (info)
          g_object_unref (info);
        break;
      }
    case YAD_FIELD_COLOR:
      {
        gchar *cs;
        GdkRGBA c;
        GtkColorChooser *cb = GTK_COLOR_CHOOSER (g_slist_nth_data (fields, fn));
        gtk_color_chooser_get_rgba (cb, &c);
        cs = get_color (&c);

        if (options.common_data.quoted_output)
          {
            buf = g_shell_quote (cs ? cs : "");
            g_printf ("%s%s", buf, options.common_data.separator);
            g_free (buf);
          }
        else
          g_printf ("%s%s", cs, options.common_data.separator);
        g_free (cs);
        break;
      }
    case YAD_FIELD_SCALE:
      if (options.common_data.quoted_output)
        g_printf ("'%d'%s", (gint) gtk_range_get_value (GTK_RANGE (g_slist_nth_data (fields, fn))),
                  options.common_data.separator);
      else
        g_printf ("%d%s", (gint) gtk_range_get_value (GTK_RANGE (g_slist_nth_data (fields, fn))),
                  options.common_data.separator);
      break;
    case YAD_FIELD_LINK:
      if (options.common_data.quoted_output)
        {
          buf = g_shell_quote (gtk_link_button_get_uri (GTK_LINK_BUTTON (g_slist_nth_data (fields, fn))));
          g_printf ("%s%s", buf, options.common_data.separator);
          g_free (buf);
        }
      else
        g_printf ("%s%s", gtk_link_button_get_uri (GTK_LINK_BUTTON (g_slist_nth_data (fields, fn))),
                  options.common_data.separator);
      break;
    case YAD_FIELD_BUTTON:
    case YAD_FIELD_FULL_BUTTON:
    case YAD_FIELD_LABEL:
      if (options.common_data.quoted_output)
        g_printf ("''%s", options.common_data.separator);
      else
        g_printf ("%s", options.common_data.separator);
      break;
    case YAD_FIELD_TEXT:
      {
        gchar *txt;
        GtkTextBuffer *tb;
        GtkTextIter b, e;

        tb = gtk_text_view_get_buffer (GTK_TEXT_VIEW (g_slist_nth_data (fields, fn)));
        gtk_text_buffer_get_bounds (tb, &b, &e);
        txt = escape_str (gtk_text_buffer_get_text (tb, &b, &e, FALSE));
        if (options.common_data.quoted_output)
          {
            buf = g_shell_quote (txt);
            g_printf ("%s%s", buf, options.common_data.separator);
            g_free (buf);
          }
        else
          g_printf ("%s%s", txt, options.common_data.separator);
        g_free (txt);
      }
    }
}

void
form_print_result (void)
{
  guint i;

  if (options.form_data.output_by_row)
    {
      guint j, rows;

      rows = n_fields / options.form_data.columns;
      rows += ((n_fields % options.form_data.columns) ? 1 : 0);
      for (i = 0; i < rows; i++)
        {
          for (j = 0; j < options.form_data.columns; j++)
            {
              guint fld = i + rows * j;
              if (fld < n_fields)
                form_print_field (fld);
            }
        }
    }
  else
    {
      for (i = 0; i < n_fields; i++)
        form_print_field (i);
    }
  g_printf ("\n");
}
