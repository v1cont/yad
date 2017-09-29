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

#include "yad.h"

static GtkWidget *calendar;

static GHashTable *details;

static void
parse_details ()
{
  FILE *f;

  details = g_hash_table_new (g_str_hash, g_str_equal);

  /* open details file */
  f = fopen (options.calendar_data.details, "r");
  if (f == NULL)
    {
      g_printerr (_("Cannot open file '%s': %s\n"), options.common_data.uri, g_strerror (errno));
      return;
    }

  /* read details file */
  while (!feof (f))
    {
      gchar buf[4096], **dtl;

      /* read string */
      memset (buf, 0, 4096);
      fgets (buf, 4096, f);
      if (strlen (buf) > 0)
        {
          dtl = g_strsplit (buf, " ", 2);
          g_hash_table_insert (details, dtl[0], dtl[1]);
        }
    }

  fclose (f);
}

static gchar *
get_details (GtkCalendar * cal, guint year, guint month, guint day, gpointer data)
{
  GDate *d;
  gchar time_string[128];
  gchar *str = NULL;

  d = g_date_new_dmy (day, month + 1, year);
  if (g_date_valid (d))
    {
      g_date_strftime (time_string, 127, options.common_data.date_format, d);
      str = (gchar *) g_hash_table_lookup (details, time_string);
    }
  g_date_free (d);

  if (str)
    return g_strdup (str);
  return str;
}

static void
double_click_cb (GtkWidget * w, gpointer data)
{
  if (options.plug == -1)
    yad_exit (options.data.def_resp);
}

GtkWidget *
calendar_create_widget (GtkWidget * dlg)
{
  GtkWidget *w;
  gint cal_opts;

  w = calendar = gtk_calendar_new ();
  gtk_widget_set_name (w, "yad-calendar-widget");

  if (options.calendar_data.month > 0 || options.calendar_data.year > 0)
    gtk_calendar_select_month (GTK_CALENDAR (w), options.calendar_data.month - 1, options.calendar_data.year);
  if (options.calendar_data.day > 0)
    gtk_calendar_select_day (GTK_CALENDAR (w), options.calendar_data.day);

  if (options.calendar_data.details)
    {
      parse_details ();
      gtk_calendar_set_detail_func (GTK_CALENDAR (w), get_details, NULL, NULL);
    }

  cal_opts = GTK_CALENDAR_SHOW_HEADING | GTK_CALENDAR_SHOW_DAY_NAMES;
  if (options.calendar_data.weeks)
    cal_opts |= GTK_CALENDAR_SHOW_WEEK_NUMBERS;
  gtk_calendar_set_display_options (GTK_CALENDAR (w), cal_opts);

  g_signal_connect (w, "day-selected-double-click", G_CALLBACK (double_click_cb), dlg);

  return w;
}

void
calendar_print_result (void)
{
  guint day, month, year;
  gchar time_string[128];
  GDate *date = NULL;

  gtk_calendar_get_date (GTK_CALENDAR (calendar), &year, &month, &day);
  date = g_date_new_dmy (day, month + 1, year);
  g_date_strftime (time_string, 127, options.common_data.date_format, date);
  g_print ("%s\n", time_string);
}
