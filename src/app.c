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

static GtkWidget *app;

static void
app_activated_cb (GtkAppChooserWidget *w, GAppInfo *app, gpointer d)
{
  if (options.plug == -1)
    yad_exit (options.data.def_resp);
}

GtkWidget *
app_create_widget (GtkWidget *dlg)
{
  gchar *ctype;
  GtkWidget *w;

  if (options.extra_data && *options.extra_data)
    ctype = options.extra_data[0];
  else
    ctype = "text/plain";

  app = w = gtk_app_chooser_widget_new (ctype);
  gtk_widget_set_name (w, "yad-app-widget");

  gtk_app_chooser_widget_set_show_default (GTK_APP_CHOOSER_WIDGET (w), TRUE);
  gtk_app_chooser_widget_set_show_recommended (GTK_APP_CHOOSER_WIDGET (w), TRUE);
  gtk_app_chooser_widget_set_show_fallback (GTK_APP_CHOOSER_WIDGET (w), options.app_data.show_fallback);
  gtk_app_chooser_widget_set_show_other (GTK_APP_CHOOSER_WIDGET (w), options.app_data.show_other);
  gtk_app_chooser_widget_set_show_all (GTK_APP_CHOOSER_WIDGET (w), options.app_data.show_all);

  g_signal_connect (G_OBJECT (w), "application-activated", G_CALLBACK (app_activated_cb), NULL);

  return w;
}

void
app_print_result (void)
{
  GAppInfo *info = gtk_app_chooser_get_app_info (GTK_APP_CHOOSER (app));

  if (info)
    {
      if (options.app_data.extended)
        {
          if (options.common_data.quoted_output)
            {
              gchar *buf;

              buf = g_shell_quote (g_app_info_get_name (info));
              g_printf ("%s%s", buf, options.common_data.separator);
              g_free (buf);

              buf = g_shell_quote (g_app_info_get_display_name (info));
              g_printf ("%s%s", buf, options.common_data.separator);
              g_free (buf);

              buf = g_shell_quote (g_app_info_get_description (info));
              g_printf ("%s%s", buf, options.common_data.separator);
              g_free (buf);

              buf = g_shell_quote (g_icon_to_string (g_app_info_get_icon (info)));
              g_printf ("%s%s", buf, options.common_data.separator);
              g_free (buf);

              buf = g_shell_quote (g_app_info_get_executable (info));
              g_printf ("%s%s", buf, options.common_data.separator);
              g_free (buf);

              g_printf ("\n");
            }
          else
            {
              g_printf ("%s%s%s%s%s%s%s%s%s%s\n",
                        g_app_info_get_name (info), options.common_data.separator,
                        g_app_info_get_display_name (info), options.common_data.separator,
                        g_app_info_get_description (info), options.common_data.separator,
                        g_icon_to_string (g_app_info_get_icon (info)), options.common_data.separator,
                        g_app_info_get_executable (info), options.common_data.separator);
            }
        }
      else
        {
          if (options.common_data.quoted_output)
            {
              gchar *buf = g_shell_quote (g_app_info_get_executable (info));
              g_printf ("%s\n", buf);
              g_free (buf);
            }
          else
            g_printf ("%s\n", g_app_info_get_executable (info));
        }
    }
}
