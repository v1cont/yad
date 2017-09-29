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

enum {
  PLUS_BTN = 0,
  MINUS_BTN,
};

static GtkWidget *scale;
static GtkWidget *plus_btn = NULL;
static GtkWidget *minus_btn = NULL;

static void
value_changed_cb (GtkWidget * w, gpointer data)
{
  if (options.scale_data.print_partial)
    g_print ("%.0f\n", gtk_range_get_value (GTK_RANGE (scale)));

  if (options.scale_data.buttons)
    {
      gdouble v = gtk_range_get_value (GTK_RANGE (scale));

      if (v >= options.scale_data.max_value)
        gtk_widget_set_sensitive (plus_btn, FALSE);
      else
        gtk_widget_set_sensitive (plus_btn, TRUE);

      if (v <= options.scale_data.min_value)
        gtk_widget_set_sensitive (minus_btn, FALSE);
      else
        gtk_widget_set_sensitive (minus_btn, TRUE);
    }
}

static void
vb_pressed (GtkWidget *b, gpointer data)
{
  gdouble v, cv = gtk_range_get_value (GTK_RANGE (scale));

  switch (GPOINTER_TO_INT (data))
    {
    case PLUS_BTN:
      v = cv + options.scale_data.step;
      gtk_range_set_value (GTK_RANGE (scale), MIN (v, options.scale_data.max_value));
      break;
    case MINUS_BTN:
      v = cv - options.scale_data.step;
      gtk_range_set_value (GTK_RANGE (scale), MAX (v, options.scale_data.min_value));
      break;
    }
}

GtkWidget *
scale_create_widget (GtkWidget * dlg)
{
  GtkWidget *w;
  GtkAdjustment *adj;
  gint page;

  if (options.scale_data.min_value >= options.scale_data.max_value)
    {
      g_printerr (_("Maximum value must be greater than minimum value.\n"));
      return NULL;
    }

  /* check for initial value */
  if (options.scale_data.have_value)
    {
      if (options.scale_data.value < options.scale_data.min_value)
        {
          g_printerr (_("Initial value less than minimal.\n"));
          options.scale_data.value = options.scale_data.min_value;
        }
      else if (options.scale_data.value > options.scale_data.max_value)
        {
          g_printerr (_("Initial value greater than maximum.\n"));
          options.scale_data.value = options.scale_data.max_value;
        }
    }
  else
    options.scale_data.value = options.scale_data.min_value;

  page = options.scale_data.page == -1 ? options.scale_data.step * 10 : options.scale_data.page;
  /* this type conversion needs only for gtk-2.0 */
  adj = (GtkAdjustment *) gtk_adjustment_new ((double) options.scale_data.value,
                                              (double) options.scale_data.min_value,
                                              (double) options.scale_data.max_value,
                                              (double) options.scale_data.step,
                                              (double) page,
                                              0.0);
  if (options.common_data.vertical)
    {
#if GTK_CHECK_VERSION(3,0,0)
      scale = gtk_scale_new (GTK_ORIENTATION_VERTICAL, GTK_ADJUSTMENT (adj));
#else
      scale = gtk_vscale_new (GTK_ADJUSTMENT (adj));
#endif
      gtk_range_set_inverted (GTK_RANGE (scale), !options.scale_data.invert);
    }
  else
    {
#if GTK_CHECK_VERSION(3,0,0)
      scale = gtk_scale_new (GTK_ORIENTATION_HORIZONTAL, GTK_ADJUSTMENT (adj));
#else
      scale = gtk_hscale_new (GTK_ADJUSTMENT (adj));
#endif
      gtk_range_set_inverted (GTK_RANGE (scale), options.scale_data.invert);
    }
  gtk_widget_set_name (scale, "yad-scale-widget");
  gtk_scale_set_digits (GTK_SCALE (scale), 0);

  if (options.scale_data.hide_value)
    gtk_scale_set_draw_value (GTK_SCALE (scale), FALSE);

  /* add marks */
  if (options.scale_data.marks)
    {
      GtkPositionType pos;
      GSList *m = options.scale_data.marks;

      pos = options.common_data.vertical ? GTK_POS_LEFT : GTK_POS_BOTTOM;
      for (; m; m = m->next)
        {
          YadScaleMark *mark = (YadScaleMark *) m->data;
          gtk_scale_add_mark (GTK_SCALE (scale), mark->value, pos, mark->name);
        }
    }

  /* create container */
  if (options.common_data.vertical)
    {
#if GTK_CHECK_VERSION(3,0,0)
      w = gtk_box_new (GTK_ORIENTATION_VERTICAL, 1);
#else
      w = gtk_vbox_new (FALSE, 1);
#endif
    }
  else
    {
#if GTK_CHECK_VERSION(3,0,0)
      w = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 1);
#else
      w = gtk_hbox_new (FALSE, 1);
#endif
    }

  /* create buttons */
  if (options.scale_data.buttons)
    {
      minus_btn = gtk_button_new_with_label ("-");
      gtk_button_set_relief (GTK_BUTTON (minus_btn), GTK_RELIEF_NONE);
      g_signal_connect (G_OBJECT (minus_btn), "clicked", G_CALLBACK (vb_pressed), GINT_TO_POINTER (MINUS_BTN));
      gtk_widget_set_sensitive (minus_btn, (options.scale_data.value > options.scale_data.min_value));

      plus_btn = gtk_button_new_with_label ("+");
      gtk_button_set_relief (GTK_BUTTON (plus_btn), GTK_RELIEF_NONE);
      g_signal_connect (G_OBJECT (plus_btn), "clicked", G_CALLBACK (vb_pressed), GINT_TO_POINTER (PLUS_BTN));
      gtk_widget_set_sensitive (plus_btn, (options.scale_data.value < options.scale_data.max_value));
    }

  /* create complex widget */
  if (options.scale_data.buttons)
    gtk_box_pack_start (GTK_BOX (w), options.common_data.vertical ? plus_btn : minus_btn, FALSE, FALSE, 0);

  gtk_box_pack_start (GTK_BOX (w), scale, TRUE, TRUE, 2);

  if (options.scale_data.buttons)
    gtk_box_pack_start (GTK_BOX (w), options.common_data.vertical ? minus_btn : plus_btn, FALSE, FALSE, 0);

  g_signal_connect (G_OBJECT (scale), "value-changed", G_CALLBACK (value_changed_cb), NULL);
  gtk_widget_grab_focus (scale);

  return w;
}

void
scale_print_result (void)
{
  g_print ("%.0f\n", gtk_range_get_value (GTK_RANGE (scale)));
}
