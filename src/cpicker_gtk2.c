
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
 * Copyright (C) 2023, step
 */

/* Some code adapted from GtkColorSelection widget (gtk/gtkcolorsel.c GTK+-2.24.33) */

#define GDK_DISABLE_DEPRECATION_WARNINGS

#include "cpicker.h"
#include <gdk/gdkkeysyms-compat.h>

#define GDK_BUTTON_PRIMARY      (1)

typedef struct {
  GtkWidget *grab_widget;
  gboolean has_grab;
  guint32 time;
} GrabData;

static gboolean key_press (GtkWidget * invisible, GdkEventKey * event, gpointer data);
static gboolean mouse_press (GtkWidget * invisible, GdkEventButton * event, gpointer data);

static GdkCursor *
make_picker_cursor (GdkScreen * screen)
{
  GdkDisplay *display;
  GdkCursor *cursor;

  display = gdk_screen_get_display (screen);
  cursor = gdk_cursor_new_from_name (display, "color-picker");
  if (!cursor)
    cursor = gdk_cursor_new_from_name (display, "crosshair");
  if (!cursor)
    g_printerr ("cpicker: cannot create cursor\n");
  return cursor;
}

static void
grab_color_at_pointer (GdkScreen * screen, gint x_root, gint y_root, gpointer data)
{
  GdkPixbuf *pixbuf;
  guchar *pixels;
  GdkColor color;
  GdkWindow *root_window = gdk_screen_get_root_window (screen);
  GrabData *gd = (GrabData *) data;

  pixbuf = gdk_pixbuf_get_from_drawable (NULL, root_window, NULL, x_root, y_root, 0, 0, 1, 1);
  if (!pixbuf)
    {
      gint x, y;
      GdkDisplay *display = gdk_screen_get_display (screen);
      GdkWindow *window = gdk_display_get_window_at_pointer (display, &x, &y);
      if (!window)
        return;
      pixbuf = gdk_pixbuf_get_from_drawable (NULL, window, NULL, x, y, 0, 0, 1, 1);
      if (!pixbuf)
        return;
    }
  pixels = gdk_pixbuf_get_pixels (pixbuf);
  color.red = pixels[0] & 0xFF;
  color.green = pixels[1] & 0xFF;
  color.blue = pixels[2] & 0xFF;
  g_object_unref (pixbuf);

  g_print ("#%02X%02X%02X\n", (gint) color.red, (gint) color.green, (gint) color.blue);
}

static void
shutdown (gpointer data)
{
  GrabData *gd = (GrabData *) data;

  if (gd->has_grab)
    {
      GdkDisplay *display = gtk_widget_get_display (gd->grab_widget);
      gdk_display_keyboard_ungrab (display, gd->time);
      gdk_display_pointer_ungrab (display, gd->time);
      gtk_grab_remove (gd->grab_widget);
      gd->has_grab = FALSE;
    }

  gtk_main_quit ();
}

static void
mouse_motion (GtkWidget * invisible, GdkEventMotion * event, gpointer data)
{
  grab_color_at_pointer (gdk_event_get_screen ((GdkEvent *) event),
                         event->x_root, event->y_root, data);
}

static gboolean
mouse_release (GtkWidget * invisible, GdkEventButton * event, gpointer data)
{
  /* GrabData *gd = data; */

  if (event->button != GDK_BUTTON_PRIMARY)
    return FALSE;

  grab_color_at_pointer (gdk_event_get_screen ((GdkEvent *) event),
                         event->x_root, event->y_root, data);

  shutdown (data);

  g_signal_handlers_disconnect_by_func (invisible, mouse_motion, data);
  g_signal_handlers_disconnect_by_func (invisible, mouse_release, data);

  return TRUE;
}

/* Helper Functions */

static gboolean
key_press (GtkWidget * invisible, GdkEventKey * event, gpointer data)
{
  GdkDisplay *display = gtk_widget_get_display (invisible);
  GdkScreen *screen = gdk_event_get_screen ((GdkEvent *) event);
  guint state = event->state & gtk_accelerator_get_default_mod_mask ();
  gint x, y, dx, dy;

  gdk_display_get_pointer (display, NULL, &x, &y, NULL);

  dx = dy = 0;

  switch (event->keyval)
    {
    case GDK_space:
    case GDK_Return:
    case GDK_ISO_Enter:
    case GDK_KP_Enter:
    case GDK_KP_Space:
      grab_color_at_pointer (screen, x, y, data);
      /* fall through */

    case GDK_Escape:
      shutdown (data);

      g_signal_handlers_disconnect_by_func (invisible, mouse_press, data);
      g_signal_handlers_disconnect_by_func (invisible, key_press, data);

      return TRUE;

    default:
      return FALSE;
    }

  gdk_display_warp_pointer (display, screen, x + dx, y + dy);

  return TRUE;
}

static gboolean
mouse_press (GtkWidget * invisible, GdkEventButton * event, gpointer data)
{
  if (event->type == GDK_BUTTON_PRESS && event->button == GDK_BUTTON_PRIMARY)
    {
      g_signal_connect (invisible, "motion-notify-event", G_CALLBACK (mouse_motion), data);
      g_signal_connect (invisible, "button-release-event", G_CALLBACK (mouse_release), data);
      g_signal_handlers_disconnect_by_func (invisible, mouse_press, data);
      g_signal_handlers_disconnect_by_func (invisible, key_press, data);
      return TRUE;
    }

  return FALSE;
}

void
yad_get_screen_color (GtkWidget *widget)
{
  GrabData *gd;
  GdkScreen *screen;
  GdkCursor *picker_cursor;
  GdkGrabStatus grab_status;
  GdkWindow *window;

  gd = g_new0 (GrabData, 1);
  gd->time = GDK_CURRENT_TIME;

  screen = gdk_screen_get_default ();
  gd->grab_widget = gtk_window_new (GTK_WINDOW_POPUP);
  gtk_window_set_screen (GTK_WINDOW (gd->grab_widget), screen);
  gtk_window_resize (GTK_WINDOW (gd->grab_widget), 1, 1);
  gtk_window_move (GTK_WINDOW (gd->grab_widget), -100, -100);
  gtk_widget_show (gd->grab_widget);

  gtk_widget_add_events (gd->grab_widget, GDK_BUTTON_RELEASE_MASK | GDK_BUTTON_PRESS_MASK | GDK_POINTER_MOTION_MASK);

  window = gtk_widget_get_window (gd->grab_widget);
  if (gdk_keyboard_grab (window,
                      GDK_KEY_PRESS_MASK | GDK_KEY_RELEASE_MASK, gd->time) != GDK_GRAB_SUCCESS)
    return;

  picker_cursor = make_picker_cursor (screen);
  grab_status = gdk_pointer_grab (window, FALSE,
                                 GDK_BUTTON_RELEASE_MASK | GDK_BUTTON_PRESS_MASK | GDK_POINTER_MOTION_MASK,
                                 NULL, picker_cursor, gd->time);
  gdk_cursor_unref (picker_cursor);

  if (grab_status != GDK_GRAB_SUCCESS)
    {
      gdk_keyboard_ungrab (gd->time);
      return;
    }

  gd->has_grab = TRUE;

  g_signal_connect (gd->grab_widget, "button-press-event", G_CALLBACK (mouse_press), gd);
  g_signal_connect (gd->grab_widget, "key-press-event", G_CALLBACK (key_press), gd);
}
