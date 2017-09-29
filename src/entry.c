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

#include "yad.h"

static GtkWidget *entry;
static gboolean is_combo = FALSE;

static void
entry_activate_cb (GtkEntry * entry, gpointer data)
{
  if (options.plug == -1)
    yad_exit (options.data.def_resp);
}

static gboolean
combo_activate_cb (GtkWidget * w, GdkEventKey * ev, gpointer data)
{
#if GTK_CHECK_VERSION(2,24,0)
  if (ev->keyval == GDK_KEY_Return || ev->keyval == GDK_KEY_KP_Enter)
#else
  if (ev->keyval == GDK_Return || ev->keyval == GDK_KP_Enter)
#endif
    {
      if (options.plug == -1)
        yad_exit (options.data.def_resp);
      return TRUE;
    }
  return FALSE;
}

static void
icon_cb (GtkEntry * entry, GtkEntryIconPosition pos, GdkEventButton * event, gpointer data)
{
  if (event->button == 1)
    {
      gchar *cmd = NULL;

      switch (pos)
        {
        case GTK_ENTRY_ICON_PRIMARY:
          cmd = options.entry_data.licon_action;
          break;
        case GTK_ENTRY_ICON_SECONDARY:
          cmd = options.entry_data.ricon_action;
          break;
        }

      if (cmd)
        {
          FILE *pf;
          gchar buf[1024];
          GString *str;

          str = g_string_new ("");
          pf = popen (cmd, "r");
          while (fgets (buf, sizeof (buf), pf))
            g_string_append (str, buf);
          if (pclose (pf) == 0)
            {
              if (str->str[str->len - 1] == '\n')
                str->str[str->len - 1] = '\0';
              gtk_entry_set_text (GTK_ENTRY (entry), str->str);
            }
          g_string_free (str, TRUE);
        }
      else
        gtk_entry_set_text (GTK_ENTRY (entry), "");

      /* move cursor to the end of text */
      gtk_editable_set_position (GTK_EDITABLE (entry), -1);
    }
}

static GtkTreeModel *
create_completion_model (void)
{
  GtkListStore *store;
  GtkTreeIter iter;
  gint i = 0;

  store = gtk_list_store_new (1, G_TYPE_STRING);

  if (options.extra_data)
    {
      while (options.extra_data[i] != NULL)
        {
          gtk_list_store_append (store, &iter);
          gtk_list_store_set (store, &iter, 0, options.extra_data[i], -1);
          i++;
        }
    }

  return GTK_TREE_MODEL (store);
}

GtkWidget *
entry_create_widget (GtkWidget * dlg)
{
  GtkWidget *c, *l = NULL, *w = NULL;

#if !GTK_CHECK_VERSION(3,0,0)
  w = gtk_hbox_new (FALSE, 5);
#else
  w = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 5);
#endif

  if (options.entry_data.entry_label)
    {
      l = gtk_label_new (NULL);
      if (options.data.no_markup)
        gtk_label_set_text_with_mnemonic (GTK_LABEL (l), options.entry_data.entry_label);
      else
        gtk_label_set_markup_with_mnemonic (GTK_LABEL (l), options.entry_data.entry_label);
      gtk_widget_set_name (l, "yad-entry-label");
      gtk_box_pack_start (GTK_BOX (w), l, FALSE, FALSE, 1);
    }

  if (options.entry_data.numeric)
    {
      gdouble min, max, step, val;
      guint prec;

      min = 0.0;
      max = 65535.0;
      step = 1.0;
      prec = 0;
      val = 0.0;

      if (options.extra_data && options.extra_data[0])
        {
          min = g_ascii_strtod (options.extra_data[0], NULL);
          if (options.extra_data[1])
            {
              max = g_ascii_strtod (options.extra_data[1], NULL);
              if (options.extra_data[2])
                {
                  step = g_ascii_strtod (options.extra_data[2], NULL);
                  if (options.extra_data[3])
                    {
                      prec = (guint) g_ascii_strtoull (options.extra_data[3], NULL, 0);
                      if (prec > 20)
                        prec = 20;
                    }
                }
            }
        }

      c = entry = gtk_spin_button_new_with_range (min, max, step);
      gtk_entry_set_alignment (GTK_ENTRY (c), 1.0);
      gtk_spin_button_set_digits (GTK_SPIN_BUTTON (c), prec);
      gtk_widget_set_name (entry, "yad-entry-spin");

      if (options.entry_data.entry_text)
        {
          val = g_ascii_strtod (options.entry_data.entry_text, NULL);

          if (min >= max)
            {
              g_printerr (_("Maximum value must be greater than minimum value.\n"));
              min = 0.0;
              max = 65535.0;
            }

          if (val < min)
            {
              g_printerr (_("Initial value less than minimal.\n"));
              val = min;
            }
          else if (val > max)
            {
              g_printerr (_("Initial value greater than maximum.\n"));
              val = max;
            }
        }

      gtk_spin_button_set_value (GTK_SPIN_BUTTON (c), val);
    }
  else if (!options.entry_data.completion && options.extra_data && *options.extra_data)
    {
      gint active, i;

      if (options.common_data.editable || settings.combo_always_editable)
        {
#if GTK_CHECK_VERSION(2,24,0)
          c = gtk_combo_box_text_new_with_entry ();
#else
          c = gtk_combo_box_entry_new_text ();
#endif
          gtk_widget_set_name (c, "yad-entry-edit-combo");
          entry = gtk_bin_get_child (GTK_BIN (c));
          if (options.entry_data.licon)
            {
              GdkPixbuf *pb = get_pixbuf (options.entry_data.licon, YAD_SMALL_ICON);

              if (pb)
                gtk_entry_set_icon_from_pixbuf (GTK_ENTRY (entry), GTK_ENTRY_ICON_PRIMARY, pb);
            }
          if (options.entry_data.ricon)
            {
              GdkPixbuf *pb = get_pixbuf (options.entry_data.ricon, YAD_SMALL_ICON);

              if (pb)
                gtk_entry_set_icon_from_pixbuf (GTK_ENTRY (entry), GTK_ENTRY_ICON_SECONDARY, pb);
            }
        }
      else
        {
#if GTK_CHECK_VERSION(2,24,0)
          c = entry = gtk_combo_box_text_new ();
#else
          c = entry = gtk_combo_box_new_text ();
#endif
          gtk_widget_set_name (c, "yad-entry-combo");
          is_combo = TRUE;
        }

      i = 0;
      active = -1;
      while (options.extra_data[i] != NULL)
        {
          if (options.entry_data.entry_text &&
              g_ascii_strcasecmp (options.extra_data[i], options.entry_data.entry_text) == 0)
            active = i;
#if GTK_CHECK_VERSION(2,24,0)
          gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (c), options.extra_data[i]);
#else
          gtk_combo_box_append_text (GTK_COMBO_BOX (c), options.extra_data[i]);
#endif
          i++;
        }

      if (options.entry_data.entry_text && active == -1)
        {
#if GTK_CHECK_VERSION(2,24,0)
          gtk_combo_box_text_prepend_text (GTK_COMBO_BOX_TEXT (c), options.entry_data.entry_text);
#else
          gtk_combo_box_prepend_text (GTK_COMBO_BOX (c), options.entry_data.entry_text);
#endif
        }

      /* set first iter active */
      if (!options.common_data.editable)
        gtk_combo_box_set_active (GTK_COMBO_BOX (c), (active != -1 ? active : 0));
    }
  else
    {
      c = entry = gtk_entry_new ();
      gtk_widget_set_name (c, "yad-entry-widget");

      gtk_entry_set_activates_default (GTK_ENTRY (entry), TRUE);

      if (options.entry_data.entry_text)
        gtk_entry_set_text (GTK_ENTRY (entry), options.entry_data.entry_text);

      if (options.entry_data.hide_text)
        g_object_set (G_OBJECT (entry), "visibility", FALSE, NULL);

      if (options.entry_data.completion)
        {
          GtkEntryCompletion *completion;
          GtkTreeModel *completion_model;

          completion = gtk_entry_completion_new ();
          gtk_entry_set_completion (GTK_ENTRY (entry), completion);

          completion_model = create_completion_model ();
          gtk_entry_completion_set_model (completion, completion_model);
          g_object_unref (completion_model);

          gtk_entry_completion_set_text_column (completion, 0);

          if (options.common_data.complete != YAD_COMPLETE_SIMPLE)
            gtk_entry_completion_set_match_func (completion, check_complete, NULL, NULL);

          g_object_unref (completion);
        }

      if (options.entry_data.licon)
        {
          GdkPixbuf *pb = get_pixbuf (options.entry_data.licon, YAD_SMALL_ICON);

          if (pb)
            gtk_entry_set_icon_from_pixbuf (GTK_ENTRY (entry), GTK_ENTRY_ICON_PRIMARY, pb);
        }
      if (options.entry_data.ricon)
        {
          GdkPixbuf *pb = get_pixbuf (options.entry_data.ricon, YAD_SMALL_ICON);

          if (pb)
            gtk_entry_set_icon_from_pixbuf (GTK_ENTRY (entry), GTK_ENTRY_ICON_SECONDARY, pb);
        }
    }

  if (l)
    gtk_label_set_mnemonic_widget (GTK_LABEL (l), entry);

  if (!is_combo)
    g_signal_connect (G_OBJECT (entry), "activate", G_CALLBACK (entry_activate_cb), dlg);
  else
    g_signal_connect (G_OBJECT (entry), "key-press-event", G_CALLBACK (combo_activate_cb), dlg);

  if (options.entry_data.licon || options.entry_data.ricon)
    g_signal_connect (G_OBJECT (entry), "icon-press", G_CALLBACK (icon_cb), NULL);

  gtk_box_pack_start (GTK_BOX (w), c, TRUE, TRUE, 1);

  return w;
}

void
entry_print_result (void)
{
  if (options.entry_data.numeric)
    {
      guint prec = gtk_spin_button_get_digits (GTK_SPIN_BUTTON (entry));
      g_print ("%.*f\n", prec, gtk_spin_button_get_value (GTK_SPIN_BUTTON (entry)));
    }
  else if (is_combo)
    {
      if (options.common_data.num_output)
        g_print ("%d\n", gtk_combo_box_get_active (GTK_COMBO_BOX (entry)) + 1);
      else
        {
#if GTK_CHECK_VERSION(2,24,0)
          g_print ("%s\n", gtk_combo_box_text_get_active_text (GTK_COMBO_BOX_TEXT (entry)));
#else
          g_print ("%s\n", gtk_combo_box_get_active_text (GTK_COMBO_BOX (entry)));
#endif
        }
    }
  else
    g_print ("%s\n", gtk_entry_get_text (GTK_ENTRY (entry)));
}
