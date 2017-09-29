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

static GtkWidget *picture;
static GtkWidget *viewport;
static GtkWidget *popup_menu;

static GdkPixbufAnimation *anim_pb;
static GdkPixbuf *orig_pb;

static gboolean loaded = FALSE;
static gboolean animated = FALSE;

enum {
  SIZE_FIT,
  SIZE_ORIG,
  SIZE_INC,
  SIZE_DEC
};

enum {
  ROTATE_LEFT,
  ROTATE_RIGHT,
  ROTATE_FLIP_VERT,
  ROTATE_FLIP_HOR
};

static void
load_picture (gchar *filename)
{
  anim_pb = gdk_pixbuf_animation_new_from_file (filename, NULL);
  orig_pb = gdk_pixbuf_animation_get_static_image (anim_pb);

  if (orig_pb)
    {
      if (gdk_pixbuf_animation_is_static_image (anim_pb))
        gtk_image_set_from_pixbuf (GTK_IMAGE (picture), orig_pb);
      else
        {
          gtk_image_set_from_animation (GTK_IMAGE (picture), anim_pb);
          animated = TRUE;
        }
      loaded = TRUE;
    }
  else
    gtk_image_set_from_stock (GTK_IMAGE (picture), "gtk-missing-image", GTK_ICON_SIZE_DIALOG);
}

void
picture_fit_to_window ()
{
  gdouble width, height, ww, wh;
  gdouble factor;

  if (animated)
    return;

  width = gdk_pixbuf_get_width (orig_pb);
  height = gdk_pixbuf_get_height (orig_pb);

  ww = gdk_window_get_width (gtk_viewport_get_view_window (GTK_VIEWPORT (viewport)));
  wh = gdk_window_get_height (gtk_viewport_get_view_window (GTK_VIEWPORT (viewport)));

  factor = MIN (ww / width, wh / height);
  if (factor < 1.0)
    {
      GdkPixbuf *pb = gdk_pixbuf_scale_simple (g_object_ref (orig_pb), width * factor, height * factor, GDK_INTERP_HYPER);
      if (pb)
        gtk_image_set_from_pixbuf (GTK_IMAGE (picture), pb);
    }
}

static void
change_size_cb (GtkWidget *w, gint type)
{
  gdouble width, height;
  GdkPixbuf *new_pb, *pb = gtk_image_get_pixbuf (GTK_IMAGE (picture));

  if (!pb)
    {
      g_printerr ("picture: can't get pixbuf\n");
      return;
    }

  width = gdk_pixbuf_get_width (pb);
  height = gdk_pixbuf_get_height (pb);

  switch (type)
    {
    case SIZE_FIT:
      picture_fit_to_window ();
      break;
    case SIZE_ORIG:
      gtk_image_set_from_pixbuf (GTK_IMAGE (picture), orig_pb);
      break;
    case SIZE_INC:
      new_pb = gdk_pixbuf_scale_simple (pb, width + options.picture_data.inc,
                                        height + options.picture_data.inc, GDK_INTERP_HYPER);
      if (new_pb)
        {
          gtk_image_set_from_pixbuf (GTK_IMAGE (picture), new_pb);
          g_object_unref (pb);
        }
      break;
    case SIZE_DEC:
      new_pb = gdk_pixbuf_scale_simple (pb, width - options.picture_data.inc,
                                        height - options.picture_data.inc, GDK_INTERP_HYPER);
      if (new_pb)
        {
          gtk_image_set_from_pixbuf (GTK_IMAGE (picture), new_pb);
          g_object_unref (pb);
        }
      break;
    }
}

static void
rotate_cb (GtkWidget *w, gint type)
{
  GdkPixbuf *new_pb = NULL;
  GdkPixbuf *pb = gtk_image_get_pixbuf (GTK_IMAGE (picture));

  if (!pb)
    {
      g_printerr ("picture: can't get pixbuf\n");
      return;
    }

  switch (type)
    {
    case ROTATE_LEFT:
      new_pb = gdk_pixbuf_rotate_simple (pb, GDK_PIXBUF_ROTATE_COUNTERCLOCKWISE);
      break;
    case ROTATE_RIGHT:
      new_pb = gdk_pixbuf_rotate_simple (pb, GDK_PIXBUF_ROTATE_CLOCKWISE);
      break;
    case ROTATE_FLIP_VERT:
      new_pb = gdk_pixbuf_flip (pb, FALSE);
      break;
    case ROTATE_FLIP_HOR:
      new_pb = gdk_pixbuf_flip (pb, TRUE);
      break;
    }

  if (new_pb)
    {
      gtk_image_set_from_pixbuf (GTK_IMAGE (picture), new_pb);
      g_object_unref (pb);
    }
}

static void
create_popup_menu ()
{
  GtkWidget *mi;

  popup_menu = gtk_menu_new ();

  mi = gtk_image_menu_item_new_with_label (_("Fit to window"));
  gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (mi),
                                 gtk_image_new_from_icon_name ("gtk-zoom-fit", GTK_ICON_SIZE_MENU));
  gtk_widget_show (mi);
  gtk_menu_shell_append (GTK_MENU_SHELL (popup_menu), mi);
  g_signal_connect (G_OBJECT (mi), "activate", G_CALLBACK (change_size_cb), GINT_TO_POINTER (SIZE_FIT));

  mi = gtk_image_menu_item_new_with_label (_("Original size"));
  gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (mi),
                                 gtk_image_new_from_icon_name ("gtk-zoom-100", GTK_ICON_SIZE_MENU));
  gtk_widget_show (mi);
  gtk_menu_shell_append (GTK_MENU_SHELL (popup_menu), mi);
  g_signal_connect (G_OBJECT (mi), "activate", G_CALLBACK (change_size_cb), GINT_TO_POINTER (SIZE_ORIG));

  mi = gtk_image_menu_item_new_with_label (_("Increase size"));
  gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (mi),
                                 gtk_image_new_from_icon_name ("gtk-zoom-in", GTK_ICON_SIZE_MENU));
  gtk_widget_show (mi);
  gtk_menu_shell_append (GTK_MENU_SHELL (popup_menu), mi);
  g_signal_connect (G_OBJECT (mi), "activate", G_CALLBACK (change_size_cb), GINT_TO_POINTER (SIZE_INC));

  mi = gtk_image_menu_item_new_with_label (_("Decrease size"));
  gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (mi),
                                 gtk_image_new_from_icon_name ("gtk-zoom-out", GTK_ICON_SIZE_MENU));
  gtk_widget_show (mi);
  gtk_menu_shell_append (GTK_MENU_SHELL (popup_menu), mi);
  g_signal_connect (G_OBJECT (mi), "activate", G_CALLBACK (change_size_cb), GINT_TO_POINTER (SIZE_DEC));

  mi = gtk_separator_menu_item_new ();
  gtk_widget_show (mi);
  gtk_menu_shell_append (GTK_MENU_SHELL (popup_menu), mi);

  mi = gtk_image_menu_item_new_with_label (_("Rotate left"));
  gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (mi),
                                 gtk_image_new_from_icon_name ("object-rotate-left", GTK_ICON_SIZE_MENU));
  gtk_widget_show (mi);
  gtk_menu_shell_append (GTK_MENU_SHELL (popup_menu), mi);
  g_signal_connect (G_OBJECT (mi), "activate", G_CALLBACK (rotate_cb), GINT_TO_POINTER (ROTATE_LEFT));

  mi = gtk_image_menu_item_new_with_label (_("Rotate right"));
  gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (mi),
                                 gtk_image_new_from_icon_name ("object-rotate-right", GTK_ICON_SIZE_MENU));
  gtk_widget_show (mi);
  gtk_menu_shell_append (GTK_MENU_SHELL (popup_menu), mi);
  g_signal_connect (G_OBJECT (mi), "activate", G_CALLBACK (rotate_cb), GINT_TO_POINTER (ROTATE_RIGHT));

  mi = gtk_image_menu_item_new_with_label (_("Flip vertical"));
  gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (mi),
                                 gtk_image_new_from_icon_name ("object-flip-vertical", GTK_ICON_SIZE_MENU));
  gtk_widget_show (mi);
  gtk_menu_shell_append (GTK_MENU_SHELL (popup_menu), mi);
  g_signal_connect (G_OBJECT (mi), "activate", G_CALLBACK (rotate_cb), GINT_TO_POINTER (ROTATE_FLIP_VERT));

  mi = gtk_image_menu_item_new_with_label (_("Flip horizontal"));
  gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (mi),
                                 gtk_image_new_from_icon_name ("object-flip-horizontal", GTK_ICON_SIZE_MENU));
  gtk_widget_show (mi);
  gtk_menu_shell_append (GTK_MENU_SHELL (popup_menu), mi);
  g_signal_connect (G_OBJECT (mi), "activate", G_CALLBACK (rotate_cb), GINT_TO_POINTER (ROTATE_FLIP_HOR));
}

static gboolean
button_handler (GtkWidget *w, GdkEventButton *ev, gpointer data)
{
  if (ev->button == 3)
    {
      gtk_menu_popup (GTK_MENU (popup_menu), NULL, NULL, NULL, NULL, ev->button, ev->time);
      return TRUE;
    }

  return FALSE;
}

static gboolean
key_handler (GtkWidget *w, GdkEventKey *ev, gpointer data)
{
  return FALSE;
}

GtkWidget *
picture_create_widget (GtkWidget * dlg)
{
  GtkWidget *sw;

  sw = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (sw), GTK_SHADOW_NONE);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (sw), options.hscroll_policy, options.vscroll_policy);

  viewport = gtk_viewport_new (gtk_scrolled_window_get_hadjustment (GTK_SCROLLED_WINDOW (sw)),
                               gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW (sw)));
  gtk_container_add (GTK_CONTAINER (sw), viewport);

  picture = gtk_image_new ();
  gtk_container_add (GTK_CONTAINER (viewport), picture);

  /* load picture */
  if (options.common_data.uri &&
      g_file_test (options.common_data.uri, G_FILE_TEST_EXISTS))
    load_picture (options.common_data.uri);
  else
    gtk_image_set_from_stock (GTK_IMAGE (picture), "gtk-missing-image", GTK_ICON_SIZE_DIALOG);

  if (loaded && !animated)
    {
      create_popup_menu ();
      g_signal_connect (G_OBJECT (viewport), "button-press-event", G_CALLBACK (button_handler), NULL);
      g_signal_connect (G_OBJECT (viewport), "key-press-event", G_CALLBACK (key_handler), NULL);
    }

  return sw;
}
