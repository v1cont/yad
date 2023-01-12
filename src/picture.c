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

enum {
  COPY_IMAGE,
  COPY_FILE
};

typedef struct {
  gchar *filename;
  GdkPixbufAnimation *anim_pb;
  GdkPixbuf *orig_pb;
  gboolean loaded;
  gboolean animated;
} ImageItem;

static GList *img_list = NULL;
static GList *lp = NULL;
static ImageItem *img = NULL;

static GtkWidget *picture;
static GtkWidget *viewport;
static GtkWidget *popup_menu;

static void create_popup_menu ();

static void
img_free (ImageItem *ii)
{
  if (ii)
    {
      g_free (ii->filename);
      if (ii->orig_pb)
        g_object_unref (ii->orig_pb);
      if (ii->anim_pb)
        g_object_unref (ii->anim_pb);
      g_free (ii);
    }
}

static void
load_picture ()
{
  if (!img)
    return;

  if (!img->loaded)
    {
      if (img->filename && g_file_test (img->filename, G_FILE_TEST_EXISTS))
        {
          img->anim_pb = gdk_pixbuf_animation_new_from_file (img->filename, NULL);
          img->orig_pb = gdk_pixbuf_animation_get_static_image (img->anim_pb);

          img->animated = !gdk_pixbuf_animation_is_static_image (img->anim_pb);
        }
      img->loaded = TRUE;
    }

  if (img->orig_pb)
    {
      if (img->animated)
        gtk_image_set_from_animation (GTK_IMAGE (picture), g_object_ref (img->anim_pb));
      else
        gtk_image_set_from_pixbuf (GTK_IMAGE (picture), g_object_ref (img->orig_pb));
      if (options.picture_data.size == YAD_PICTURE_FIT)
        picture_fit_to_window ();
    }
  else
    gtk_image_set_from_icon_name (GTK_IMAGE (picture), "image-missing", GTK_ICON_SIZE_DIALOG);
}

static void
img_changed_hook ()
{
  gchar *qfn, *cmd = NULL;

  if (options.picture_data.change_cmd == NULL)
    return;
  if (!img)
    return;
  
  qfn = g_shell_quote (img->filename);
  if (g_strstr_len (options.picture_data.change_cmd, -1, "%s") != NULL)
    cmd = g_strdup_printf (options.picture_data.change_cmd, qfn);
  else
    cmd = g_strdup_printf ("%s '%s'", options.picture_data.change_cmd, qfn);
  g_free (qfn);

  run_command_async (cmd);
  g_free (cmd);
}

static void
next_img_cb (GtkWidget *w, gpointer d)
{
  lp = g_list_next (lp);
  if (!lp)
    lp = g_list_first (img_list);
  if (lp)
    img = (ImageItem *) lp->data;

  load_picture ();
  img_changed_hook ();
}

static void
prev_img_cb (GtkWidget *w, gpointer d)
{
  lp = g_list_previous (lp);
  if (!lp)
    lp = g_list_last (img_list);
  if (lp)
    img = (ImageItem *) lp->data;

  load_picture ();
  img_changed_hook ();
}

static void
first_img_cb (GtkWidget *w, gpointer d)
{
  lp = g_list_first (img_list);
  if (lp)
    img = (ImageItem *) lp->data;

  load_picture ();
  img_changed_hook ();
}

static void
last_img_cb (GtkWidget *w, gpointer d)
{
  lp = g_list_last (img_list);
  if (lp)
    img = (ImageItem *) lp->data;

  load_picture ();
  img_changed_hook ();
}

void
picture_fit_to_window ()
{
  gdouble width, height, ww, wh;
  gdouble factor;

  if (!gtk_widget_get_realized (viewport) || !img)
    return;

  width = gdk_pixbuf_get_width (img->orig_pb);
  height = gdk_pixbuf_get_height (img->orig_pb);

  ww = gdk_window_get_width (gtk_viewport_get_view_window (GTK_VIEWPORT (viewport)));
  wh = gdk_window_get_height (gtk_viewport_get_view_window (GTK_VIEWPORT (viewport)));

  factor = MIN (ww / width, wh / height);
  if (factor < 1.0)
    {
      GdkPixbuf *pb = gdk_pixbuf_scale_simple (img->orig_pb, width * factor, height * factor, GDK_INTERP_HYPER);
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
      options.picture_data.size = YAD_PICTURE_FIT;
      break;
    case SIZE_ORIG:
      gtk_image_set_from_pixbuf (GTK_IMAGE (picture), g_object_ref (img->orig_pb));
      options.picture_data.size = YAD_PICTURE_ORIG;
      break;
    case SIZE_INC:
      new_pb = gdk_pixbuf_scale_simple (img->orig_pb, width + options.picture_data.inc,
                                        height + options.picture_data.inc, GDK_INTERP_HYPER);
      if (new_pb)
        {
          gtk_image_set_from_pixbuf (GTK_IMAGE (picture), new_pb);
          g_object_unref (pb);
        }
      break;
    case SIZE_DEC:
      new_pb = gdk_pixbuf_scale_simple (img->orig_pb, width - options.picture_data.inc,
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
open_file_cb (GtkWidget *w, gpointer d)
{
  GtkWidget *dlg;
  GtkFileFilter *imgf, *allf;
  static gchar *dir = NULL;

  imgf = gtk_file_filter_new ();
  gtk_file_filter_set_name (imgf, _("Images"));
  gtk_file_filter_add_pixbuf_formats (imgf);

  allf = gtk_file_filter_new ();
  gtk_file_filter_set_name (allf, _("All files"));
  gtk_file_filter_add_pattern (allf, "*");

  dlg = gtk_file_chooser_dialog_new (_("YAD - Select Image(s)"),
                                     GTK_WINDOW (gtk_widget_get_toplevel (w)),
                                     GTK_FILE_CHOOSER_ACTION_OPEN,
                                     _("Cancel"), GTK_RESPONSE_CANCEL,
                                     _("OK"), GTK_RESPONSE_ACCEPT,
                                     NULL);
  gtk_file_chooser_set_select_multiple (GTK_FILE_CHOOSER (dlg), TRUE);
  gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (dlg), imgf);
  gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (dlg), allf);
  if (dir)
    gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (dlg), dir);

  if (gtk_dialog_run (GTK_DIALOG (dlg)) == GTK_RESPONSE_ACCEPT)
    {
      GSList *fp, *fn = gtk_file_chooser_get_filenames (GTK_FILE_CHOOSER (dlg));

      if (img_list)
        {
          g_list_free_full (img_list, (GDestroyNotify) img_free);
          img_list = NULL;
        }

      for (fp = fn; fp; fp = fp->next)
        {
          ImageItem *ii = g_new0 (ImageItem, 1);

          ii->filename = g_strdup (fp->data);
          img_list = g_list_append (img_list, ii);
        }
      g_slist_free_full (fn, g_free);

      lp = g_list_first (img_list);
      img = (ImageItem *) lp->data;
      load_picture ();
      img_changed_hook ();

      /* recreate menu */
      gtk_widget_destroy (popup_menu);
      create_popup_menu ();

      /* keep current dir */
      g_free (dir);
      dir = gtk_file_chooser_get_current_folder (GTK_FILE_CHOOSER (dlg));
    }

  gtk_widget_destroy (dlg);
}

static void
copy_cb (GtkWidget *w, gint type)
{
  GtkClipboard *clip;

  clip = gtk_clipboard_get_default (gdk_display_get_default ());

  switch (type)
    {
    case COPY_IMAGE:
      gtk_clipboard_set_image (clip, img->orig_pb);
      break;
    case COPY_FILE:
      gtk_clipboard_set_text (clip, img->filename, -1);
      break;
    }
}

static void
create_popup_menu ()
{
  GtkWidget *mi, *al;

  popup_menu = gtk_menu_new ();
  gtk_menu_set_reserve_toggle_size (GTK_MENU (popup_menu), FALSE);

  if (options.common_data.file_op)
    {
      mi = gtk_menu_item_new_with_label (_("Open file(s)..."));
      al = gtk_bin_get_child (GTK_BIN (mi));
      gtk_accel_label_set_accel (GTK_ACCEL_LABEL (al), GDK_KEY_o, GDK_CONTROL_MASK);
      gtk_widget_show (mi);
      gtk_menu_shell_append (GTK_MENU_SHELL (popup_menu), mi);
      g_signal_connect (G_OBJECT (mi), "activate", G_CALLBACK (open_file_cb), NULL);

      mi = gtk_separator_menu_item_new ();
      gtk_widget_show (mi);
      gtk_menu_shell_append (GTK_MENU_SHELL (popup_menu), mi);
    }

  if (img_list)
    {
      mi = gtk_menu_item_new_with_label (_("Next image"));
      al = gtk_bin_get_child (GTK_BIN (mi));
      gtk_accel_label_set_accel (GTK_ACCEL_LABEL (al), GDK_KEY_Right, 0);
      gtk_widget_show (mi);
      gtk_menu_shell_append (GTK_MENU_SHELL (popup_menu), mi);
      g_signal_connect (G_OBJECT (mi), "activate", G_CALLBACK (next_img_cb), NULL);

      mi = gtk_menu_item_new_with_label (_("Previous image"));
      al = gtk_bin_get_child (GTK_BIN (mi));
      gtk_accel_label_set_accel (GTK_ACCEL_LABEL (al), GDK_KEY_Left, 0);
      gtk_widget_show (mi);
      gtk_menu_shell_append (GTK_MENU_SHELL (popup_menu), mi);
      g_signal_connect (G_OBJECT (mi), "activate", G_CALLBACK (prev_img_cb), NULL);

      mi = gtk_menu_item_new_with_label (_("First image"));
      al = gtk_bin_get_child (GTK_BIN (mi));
      gtk_accel_label_set_accel (GTK_ACCEL_LABEL (al), GDK_KEY_Home, 0);
      gtk_widget_show (mi);
      gtk_menu_shell_append (GTK_MENU_SHELL (popup_menu), mi);
      g_signal_connect (G_OBJECT (mi), "activate", G_CALLBACK (first_img_cb), NULL);

      mi = gtk_menu_item_new_with_label (_("Last image"));
      al = gtk_bin_get_child (GTK_BIN (mi));
      gtk_accel_label_set_accel (GTK_ACCEL_LABEL (al), GDK_KEY_End, 0);
      gtk_widget_show (mi);
      gtk_menu_shell_append (GTK_MENU_SHELL (popup_menu), mi);
      g_signal_connect (G_OBJECT (mi), "activate", G_CALLBACK (last_img_cb), NULL);

      mi = gtk_separator_menu_item_new ();
      gtk_widget_show (mi);
      gtk_menu_shell_append (GTK_MENU_SHELL (popup_menu), mi);
    }

  mi = gtk_menu_item_new_with_label (_("Fit to window"));
  al = gtk_bin_get_child (GTK_BIN (mi));
  gtk_accel_label_set_accel (GTK_ACCEL_LABEL (al), GDK_KEY_w, 0);
  gtk_widget_show (mi);
  gtk_menu_shell_append (GTK_MENU_SHELL (popup_menu), mi);
  g_signal_connect (G_OBJECT (mi), "activate", G_CALLBACK (change_size_cb), GINT_TO_POINTER (SIZE_FIT));

  mi = gtk_menu_item_new_with_label (_("Original size"));
  al = gtk_bin_get_child (GTK_BIN (mi));
  gtk_accel_label_set_accel (GTK_ACCEL_LABEL (al), GDK_KEY_o, 0);
  gtk_widget_show (mi);
  gtk_menu_shell_append (GTK_MENU_SHELL (popup_menu), mi);
  g_signal_connect (G_OBJECT (mi), "activate", G_CALLBACK (change_size_cb), GINT_TO_POINTER (SIZE_ORIG));

  mi = gtk_menu_item_new_with_label (_("Increase size"));
  al = gtk_bin_get_child (GTK_BIN (mi));
  gtk_accel_label_set_accel (GTK_ACCEL_LABEL (al), GDK_KEY_plus, 0);
  gtk_widget_show (mi);
  gtk_menu_shell_append (GTK_MENU_SHELL (popup_menu), mi);
  g_signal_connect (G_OBJECT (mi), "activate", G_CALLBACK (change_size_cb), GINT_TO_POINTER (SIZE_INC));

  mi = gtk_menu_item_new_with_label (_("Decrease size"));
  al = gtk_bin_get_child (GTK_BIN (mi));
  gtk_accel_label_set_accel (GTK_ACCEL_LABEL (al), GDK_KEY_minus, 0);
  gtk_widget_show (mi);
  gtk_menu_shell_append (GTK_MENU_SHELL (popup_menu), mi);
  g_signal_connect (G_OBJECT (mi), "activate", G_CALLBACK (change_size_cb), GINT_TO_POINTER (SIZE_DEC));

  mi = gtk_separator_menu_item_new ();
  gtk_widget_show (mi);
  gtk_menu_shell_append (GTK_MENU_SHELL (popup_menu), mi);

  mi = gtk_menu_item_new_with_label (_("Rotate left"));
  al = gtk_bin_get_child (GTK_BIN (mi));
  gtk_accel_label_set_accel (GTK_ACCEL_LABEL (al), GDK_KEY_l, 0);
  gtk_widget_show (mi);
  gtk_menu_shell_append (GTK_MENU_SHELL (popup_menu), mi);
  g_signal_connect (G_OBJECT (mi), "activate", G_CALLBACK (rotate_cb), GINT_TO_POINTER (ROTATE_LEFT));

  mi = gtk_menu_item_new_with_label (_("Rotate right"));
  al = gtk_bin_get_child (GTK_BIN (mi));
  gtk_accel_label_set_accel (GTK_ACCEL_LABEL (al), GDK_KEY_r, 0);
  gtk_widget_show (mi);
  gtk_menu_shell_append (GTK_MENU_SHELL (popup_menu), mi);
  g_signal_connect (G_OBJECT (mi), "activate", G_CALLBACK (rotate_cb), GINT_TO_POINTER (ROTATE_RIGHT));

  mi = gtk_menu_item_new_with_label (_("Flip vertical"));
  al = gtk_bin_get_child (GTK_BIN (mi));
  gtk_accel_label_set_accel (GTK_ACCEL_LABEL (al), GDK_KEY_v, 0);
  gtk_widget_show (mi);
  gtk_menu_shell_append (GTK_MENU_SHELL (popup_menu), mi);
  g_signal_connect (G_OBJECT (mi), "activate", G_CALLBACK (rotate_cb), GINT_TO_POINTER (ROTATE_FLIP_VERT));

  mi = gtk_menu_item_new_with_label (_("Flip horizontal"));
  al = gtk_bin_get_child (GTK_BIN (mi));
  gtk_accel_label_set_accel (GTK_ACCEL_LABEL (al), GDK_KEY_h, 0);
  gtk_widget_show (mi);
  gtk_menu_shell_append (GTK_MENU_SHELL (popup_menu), mi);
  g_signal_connect (G_OBJECT (mi), "activate", G_CALLBACK (rotate_cb), GINT_TO_POINTER (ROTATE_FLIP_HOR));

  mi = gtk_separator_menu_item_new ();
  gtk_widget_show (mi);
  gtk_menu_shell_append (GTK_MENU_SHELL (popup_menu), mi);

  mi = gtk_menu_item_new_with_label (_("Copy image to clipboard"));
  gtk_widget_show (mi);
  gtk_menu_shell_append (GTK_MENU_SHELL (popup_menu), mi);
  g_signal_connect (G_OBJECT (mi), "activate", G_CALLBACK (copy_cb), GINT_TO_POINTER (COPY_IMAGE));

  mi = gtk_menu_item_new_with_label (_("Copy filename to clipboard"));
  gtk_widget_show (mi);
  gtk_menu_shell_append (GTK_MENU_SHELL (popup_menu), mi);
  g_signal_connect (G_OBJECT (mi), "activate", G_CALLBACK (copy_cb), GINT_TO_POINTER (COPY_FILE));
}

static gboolean
button_handler (GtkWidget *w, GdkEventButton *ev, gpointer data)
{
  if (ev->button == 3 && (!img || !img->animated))
    {
      gtk_menu_popup_at_pointer (GTK_MENU (popup_menu), NULL);
      return TRUE;
    }

  return FALSE;
}

static gboolean
key_handler (GtkWidget *w, GdkEventKey *ev, gpointer data)
{
  switch (ev->keyval)
    {
    case GDK_KEY_Right:
    case GDK_KEY_KP_Right:
      next_img_cb (w, NULL);
      return TRUE;
    case GDK_KEY_Left:
    case GDK_KEY_KP_Left:
      prev_img_cb (w, NULL);
      return TRUE;
    case GDK_KEY_Home:
    case GDK_KEY_KP_Home:
      first_img_cb (w, NULL);
      return TRUE;
    case GDK_KEY_End:
    case GDK_KEY_KP_End:
      last_img_cb (w, NULL);
      return TRUE;
    case GDK_KEY_W:
    case GDK_KEY_w:
      change_size_cb (w, SIZE_FIT);
      return TRUE;
    case GDK_KEY_O:
    case GDK_KEY_o:
      if (ev->state & GDK_CONTROL_MASK)
        open_file_cb (w, NULL);
      else
        change_size_cb (w, SIZE_ORIG);
      return TRUE;
    case GDK_KEY_plus:
    case GDK_KEY_KP_Add:
      change_size_cb (w, SIZE_INC);
      return TRUE;
    case GDK_KEY_minus:
    case GDK_KEY_KP_Subtract:
      change_size_cb (w, SIZE_DEC);
      return TRUE;
    case GDK_KEY_L:
    case GDK_KEY_l:
      rotate_cb (w, ROTATE_LEFT);
      return TRUE;
    case GDK_KEY_R:
    case GDK_KEY_r:
      rotate_cb (w, ROTATE_RIGHT);
      return TRUE;
    case GDK_KEY_V:
    case GDK_KEY_v:
      rotate_cb (w, ROTATE_FLIP_VERT);
      return TRUE;
    case GDK_KEY_H:
    case GDK_KEY_h:
      rotate_cb (w, ROTATE_FLIP_HOR);
      return TRUE;
    }
  return FALSE;
}

static void
size_allocate_cb ()
{
  if (options.picture_data.size == YAD_PICTURE_FIT)
    picture_fit_to_window ();
}

static void init_img_data ()
{
  if (options.common_data.uri)
    {
      img = g_new0 (ImageItem, 1);
      img->filename = options.common_data.uri;
    }
  else if (options.extra_data && *options.extra_data)
    {
      gchar **args = options.extra_data;
      gint i = 0;

      while (args[i] != NULL)
        {
          ImageItem *ii;

          ii = g_new0 (ImageItem, 1);
          ii->filename = g_strdup (args[i]);
          img_list = g_list_append (img_list, ii);
          i++;
        }

      lp = g_list_first (img_list);
      img = (ImageItem *) lp->data;
    }
}

GtkWidget *
picture_create_widget (GtkWidget * dlg)
{
  GtkWidget *sw, *ev;

  sw = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (sw), GTK_SHADOW_NONE);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (sw), options.data.hscroll_policy, options.data.vscroll_policy);

  viewport = gtk_viewport_new (gtk_scrolled_window_get_hadjustment (GTK_SCROLLED_WINDOW (sw)),
                               gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW (sw)));
  gtk_container_add (GTK_CONTAINER (sw), viewport);

  ev = gtk_event_box_new ();
  gtk_container_add (GTK_CONTAINER (viewport), ev);

  picture = gtk_image_new ();
  gtk_widget_set_can_focus (picture, TRUE); /* need to get key press events */
  gtk_container_add (GTK_CONTAINER (ev), picture);

  g_signal_connect (G_OBJECT (ev), "button-press-event", G_CALLBACK (button_handler), NULL);
  g_signal_connect (G_OBJECT (ev), "key-press-event", G_CALLBACK (key_handler), NULL);
  g_signal_connect (G_OBJECT (ev), "size-allocate", G_CALLBACK (size_allocate_cb), NULL);

  init_img_data ();

  /* must be after calling init_img_data() */
  create_popup_menu ();

  /* load picture */
  if (img)
    load_picture ();
  else
    gtk_image_set_from_icon_name (GTK_IMAGE (picture), "image-missing", GTK_ICON_SIZE_DIALOG);

  return sw;
}
