
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
 * Copyright (C) 2020-2023, Victor Ananjevsky <victor@sanana.kiev.ua>
 */

/* This code getted from deprecated GtkColorSelection widget (gtk+-3.24.33) */

#define GDK_DISABLE_DEPRECATION_WARNINGS

#include "cpicker.h"

#define BIG_STEP 20

typedef struct {
  GtkWidget *color_widget;
  GtkWidget *grab_widget;
  GdkDevice *keyb_device;
  GdkDevice *pointer_device;
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
    cursor = gdk_cursor_new_from_name (display, "cell");
  if (!cursor)
    g_printerr ("cpicker: cannot create cursor\n");
  return cursor;
}

static void
grab_color_at_pointer (GdkScreen * screen, GdkDevice * device, gint x_root, gint y_root, gpointer data)
{
  GdkPixbuf *pixbuf;
  guchar *pixels;
  GdkRGBA color;
  GdkWindow *root_window = gdk_screen_get_root_window (screen);
  GrabData *gd = (GrabData *) data;

  pixbuf = gdk_pixbuf_get_from_window (root_window, x_root, y_root, 1, 1);
  if (!pixbuf)
    {
      gint x, y;
      GdkWindow *window = gdk_device_get_window_at_position (device, &x, &y);
      if (!window)
        return;
      pixbuf = gdk_pixbuf_get_from_window (window, x, y, 1, 1);
      if (!pixbuf)
        return;
    }
  pixels = gdk_pixbuf_get_pixels (pixbuf);
  color.red = pixels[0] / 255.;
  color.green = pixels[1] / 255.;
  color.blue = pixels[2] / 255.;
  color.alpha = 1.0;
  g_object_unref (pixbuf);

  if (gd->color_widget)
    gtk_color_chooser_set_rgba (GTK_COLOR_CHOOSER (gd->color_widget), &color);
  else
    g_print ("#%02X%02X%02X\n", (gint) (color.red * 255), (gint) (color.green * 255), (gint) (color.blue * 255));
}

static void
shutdown_eyedropper (gpointer data)
{
  GrabData *gd = (GrabData *) data;

  if (gd->has_grab)
    {
      gdk_device_ungrab (gd->keyb_device, gd->time);
      gdk_device_ungrab (gd->pointer_device, gd->time);
      gtk_device_grab_remove (gd->grab_widget, gd->pointer_device);
    }

  if (gd->color_widget == NULL)
    gtk_main_quit ();
}

static void
mouse_motion (GtkWidget * invisible, GdkEventMotion * event, gpointer data)
{
  grab_color_at_pointer (gdk_event_get_screen ((GdkEvent *) event),
                         gdk_event_get_device ((GdkEvent *) event), event->x_root, event->y_root, data);
}

static gboolean
mouse_release (GtkWidget * invisible, GdkEventButton * event, gpointer data)
{
  /* GtkColorSelection *colorsel = data; */

  if (event->button != GDK_BUTTON_PRIMARY)
    return FALSE;

  grab_color_at_pointer (gdk_event_get_screen ((GdkEvent *) event),
                         gdk_event_get_device ((GdkEvent *) event), event->x_root, event->y_root, data);

  shutdown_eyedropper (data);

  g_signal_handlers_disconnect_by_func (invisible, mouse_motion, data);
  g_signal_handlers_disconnect_by_func (invisible, mouse_release, data);

  return TRUE;
}

/* Helper Functions */

static gboolean
key_press (GtkWidget * invisible, GdkEventKey * event, gpointer data)
{
  GdkScreen *screen = gdk_event_get_screen ((GdkEvent *) event);
  GdkDevice *device, *pointer_device;
  guint state = event->state & gtk_accelerator_get_default_mod_mask ();
  gint x, y, dx, dy;

  device = gdk_event_get_device ((GdkEvent *) event);
  pointer_device = gdk_device_get_associated_device (device);
  gdk_device_get_position (pointer_device, NULL, &x, &y);

  dx = dy = 0;

  switch (event->keyval)
    {
    case GDK_KEY_space:
    case GDK_KEY_Return:
    case GDK_KEY_ISO_Enter:
    case GDK_KEY_KP_Enter:
    case GDK_KEY_KP_Space:
      grab_color_at_pointer (screen, pointer_device, x, y, data);
      /* fall through */

    case GDK_KEY_Escape:
      shutdown_eyedropper (data);

      g_signal_handlers_disconnect_by_func (invisible, mouse_press, data);
      g_signal_handlers_disconnect_by_func (invisible, key_press, data);

      return TRUE;

    case GDK_KEY_Up:
    case GDK_KEY_KP_Up:
      dy = state == GDK_MOD1_MASK ? -BIG_STEP : -1;
      break;

    case GDK_KEY_Down:
    case GDK_KEY_KP_Down:
      dy = state == GDK_MOD1_MASK ? BIG_STEP : 1;
      break;

    case GDK_KEY_Left:
    case GDK_KEY_KP_Left:
      dx = state == GDK_MOD1_MASK ? -BIG_STEP : -1;
      break;

    case GDK_KEY_Right:
    case GDK_KEY_KP_Right:
      dx = state == GDK_MOD1_MASK ? BIG_STEP : 1;
      break;

    default:
      return FALSE;
    }

  gdk_device_warp (pointer_device, screen, x + dx, y + dy);

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
  GdkDevice *device;
  GdkCursor *picker_cursor;
  GdkGrabStatus grab_status;
  GdkWindow *window;

  gd = g_new0 (GrabData, 1);
  gd->color_widget = widget;
  gd->time = gtk_get_current_event_time ();

  screen = gdk_screen_get_default ();
  device = gtk_get_current_event_device ();

  if (device)
    {
  if (gdk_device_get_source (device) == GDK_SOURCE_KEYBOARD)
    {
      gd->keyb_device = device;
      gd->pointer_device = gdk_device_get_associated_device (device);
    }
  else
    {
      gd->pointer_device = device;
      gd->keyb_device = gdk_device_get_associated_device (device);
    }
  }
  else
    {
      GdkSeat *seat = gdk_display_get_default_seat (gdk_screen_get_display (screen));
      gd->keyb_device = gdk_seat_get_keyboard (seat);
      gd->pointer_device = gdk_seat_get_pointer (seat);
    }

  gd->grab_widget = gtk_window_new (GTK_WINDOW_POPUP);
  gtk_window_set_screen (GTK_WINDOW (gd->grab_widget), screen);
  gtk_window_resize (GTK_WINDOW (gd->grab_widget), 1, 1);
  gtk_window_move (GTK_WINDOW (gd->grab_widget), -100, -100);
  gtk_widget_show (gd->grab_widget);

  gtk_widget_add_events (gd->grab_widget, GDK_BUTTON_RELEASE_MASK | GDK_BUTTON_PRESS_MASK | GDK_POINTER_MOTION_MASK);

  window = gtk_widget_get_window (gd->grab_widget);
  if (gdk_device_grab (gd->keyb_device, window, GDK_OWNERSHIP_APPLICATION, FALSE,
                       GDK_KEY_PRESS_MASK | GDK_KEY_RELEASE_MASK, NULL, gd->time) != GDK_GRAB_SUCCESS)
    return;

  picker_cursor = make_picker_cursor (screen);
  grab_status = gdk_device_grab (gd->pointer_device, window, GDK_OWNERSHIP_APPLICATION, FALSE,
                                 GDK_BUTTON_RELEASE_MASK | GDK_BUTTON_PRESS_MASK | GDK_POINTER_MOTION_MASK,
                                 picker_cursor, gd->time);
  if (picker_cursor)
    g_object_unref (picker_cursor);

  if (grab_status != GDK_GRAB_SUCCESS)
    {
      gdk_device_ungrab (gd->keyb_device, gd->time);
      return;
    }

  gtk_device_grab_add (gd->grab_widget, gd->pointer_device, TRUE);

  gd->has_grab = TRUE;

  g_signal_connect (gd->grab_widget, "button-press-event", G_CALLBACK (mouse_press), gd);
  g_signal_connect (gd->grab_widget, "key-press-event", G_CALLBACK (key_press), gd);
}
