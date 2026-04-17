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

#include "yad.h"

#define MIN_BORDERS          15

static gboolean
activate_link_cb (GtkLabel *l, gchar *uri, gpointer d)
{
  open_uri (uri);
  return TRUE;
}

static gboolean
on_draw_event (GtkWidget *widget, cairo_t *cr, gpointer d)
{
  gdouble val;

  if (options.popup_data.transparent >= 100)
    val = 0.0;
  else
    val = options.popup_data.transparent / 100.0;

  cairo_set_source_rgba (cr, 0.2, 0.2, 0.2, val);
  cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);
  cairo_paint (cr);

  return FALSE;
}

gint
yad_popup_run ()
{
  GtkWidget *win;
  GtkWidget *box, *tbox, *img, *lbl;

  win = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_widget_set_name (win, "yad-dialog-popup");
  gtk_widget_set_size_request (win, options.data.width, options.data.height);
  gtk_window_set_decorated (GTK_WINDOW (win), FALSE);
  gtk_window_set_resizable (GTK_WINDOW (win), FALSE);
  gtk_window_set_accept_focus (GTK_WINDOW (win), FALSE);
  gtk_window_set_keep_above (GTK_WINDOW (win), TRUE);
  gtk_window_set_skip_taskbar_hint (GTK_WINDOW (win), TRUE);
  gtk_window_set_skip_pager_hint (GTK_WINDOW (win), TRUE);
  gtk_window_stick (GTK_WINDOW (win));

  if (options.data.borders < MIN_BORDERS)
    options.data.borders = MIN_BORDERS;
  gtk_container_set_border_width (GTK_CONTAINER (win), (guint) options.data.borders);

  if (options.popup_data.transparent > 0)
    {
      GdkScreen *screen;
      GdkVisual *visual;

      screen = gdk_screen_get_default ();
      visual = gdk_screen_get_rgba_visual (screen);

      if (visual != NULL && gdk_screen_is_composited (screen))
        {
          gtk_widget_set_app_paintable (win, TRUE);
          gtk_widget_set_visual (win, visual);

          g_signal_connect (G_OBJECT (win), "draw", G_CALLBACK (on_draw_event), NULL);
        }
    }

  box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 5);
  gtk_container_add (GTK_CONTAINER (win), box);

  if (options.data.dialog_image)
    {
      GdkPixbuf *pb = NULL;

      pb = get_pixbuf (options.data.dialog_image, YAD_BIG_ICON, FALSE);
      img = gtk_image_new_from_pixbuf (pb);
      if (pb)
        g_object_unref (pb);

      gtk_widget_set_name (img, "yad-dialog-image");
      gtk_widget_set_halign (img, GTK_ALIGN_CENTER);
      gtk_widget_set_valign (img, GTK_ALIGN_START);

      gtk_box_pack_start (GTK_BOX (box), img, FALSE, FALSE, 0);
    }

  tbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 2);
  gtk_box_pack_start (GTK_BOX (box), tbox, TRUE, TRUE, 0);

  if (options.data.dialog_text || options.data.dialog_title)
    {
      if (options.data.dialog_title)
        {
          PangoAttrList *attrs;

          lbl = gtk_label_new (options.data.dialog_title);
          gtk_label_set_xalign (GTK_LABEL (lbl), options.common_data.align);

          attrs = pango_attr_list_new ();
          pango_attr_list_insert (attrs, pango_attr_scale_new (1.5));
          pango_attr_list_insert (attrs, pango_attr_weight_new (PANGO_WEIGHT_BOLD));
          gtk_label_set_attributes (GTK_LABEL (lbl), attrs);
          pango_attr_list_unref (attrs);

          gtk_box_pack_start (GTK_BOX (tbox), lbl, FALSE, FALSE, 2);
        }

      if (options.data.dialog_text)
        {
          lbl = gtk_label_new (NULL);
          gtk_label_set_markup (GTK_LABEL (lbl), options.data.dialog_text);
          gtk_label_set_xalign (GTK_LABEL (lbl), options.common_data.align);
          gtk_label_set_justify (GTK_LABEL (lbl), options.data.text_align);
          gtk_label_set_line_wrap (GTK_LABEL (lbl), TRUE);
          gtk_label_set_line_wrap_mode (GTK_LABEL (lbl), PANGO_WRAP_WORD_CHAR);
          gtk_box_pack_start (GTK_BOX (tbox), lbl, TRUE, TRUE, 2);

          g_signal_connect (G_OBJECT (lbl), "activate-link", G_CALLBACK (activate_link_cb), NULL);
        }
    }

  gtk_widget_show_all (win);

  gtk_main ();

  return 0;
}
