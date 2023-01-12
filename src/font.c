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

#include "yad.h"

static GtkWidget *font;

static void
font_activated_cb (GtkFontChooser *w, gchar *fn, gpointer d)
{
  if (options.plug == -1)
    yad_exit (options.data.def_resp);
}

static void
realize_cb (GtkWidget * w, gpointer d)
{
  gtk_font_chooser_set_font (GTK_FONT_CHOOSER (w), options.common_data.font);
}

GtkWidget *
font_create_widget (GtkWidget * dlg)
{
  GtkWidget *w;

  w = font = gtk_font_chooser_widget_new ();
  gtk_widget_set_name (w, "yad-font-widget");

  if (options.font_data.preview)
    gtk_font_chooser_set_preview_text (GTK_FONT_CHOOSER (w), options.font_data.preview);

  g_signal_connect (G_OBJECT (w), "font-activated", G_CALLBACK (font_activated_cb), NULL);

  /* font must be set after widget inserted in toplevel */
  if (options.common_data.font)
    g_signal_connect_after (G_OBJECT (w), "realize", G_CALLBACK (realize_cb), NULL);

  return w;
}

void
font_print_result (void)
{
  if (options.font_data.separate_output)
    {
      PangoFontFace *face;
      PangoFontFamily *family;
      gint size;

      face = gtk_font_chooser_get_font_face (GTK_FONT_CHOOSER (font));
      family = gtk_font_chooser_get_font_family (GTK_FONT_CHOOSER (font));
      size = gtk_font_chooser_get_font_size (GTK_FONT_CHOOSER (font));

      if (options.common_data.quoted_output)
        {
          gchar *q1 = g_shell_quote (pango_font_family_get_name (family));
          gchar *q2 = g_shell_quote (pango_font_face_get_face_name (face));

          g_printf ("%s%s%s%s%d\n", q1, options.common_data.separator, q2,
                    options.common_data.separator, size / PANGO_SCALE);

          g_free (q1);
          g_free (q2);
        }
      else
        {
          g_printf ("%s%s%s%s%d\n", pango_font_family_get_name (family), options.common_data.separator,
                    pango_font_face_get_face_name (face), options.common_data.separator, size / PANGO_SCALE);
        }
    }
  else
    {
      gchar *fn = gtk_font_chooser_get_font (GTK_FONT_CHOOSER (font));

      if (options.common_data.quoted_output)
        {
          gchar *buf = g_shell_quote (fn);
          g_printf ("%s\n", buf);
          g_free (buf);
        }
      else
        g_printf ("%s\n", fn);

      g_free (fn);
    }
}
