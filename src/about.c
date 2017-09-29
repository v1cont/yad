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

gint
yad_about (void)
{
  GtkWidget *dialog;
  const gchar *const authors[] = {
    "Victor Ananjevsky <ananasik@gmail.com>",
    NULL
  };
  const gchar *translators = N_("translator-credits");
  const gchar *license =
    N_("YAD is free software; you can redistribute it and/or modify "
       "it under the terms of the GNU General Public License as published by "
       "the Free Software Foundation; either version 3 of the License, or "
       "(at your option) any later version.\n\n"
       "YAD is distributed in the hope that it will be useful, "
       "but WITHOUT ANY WARRANTY; without even the implied warranty of "
       "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the "
       "GNU General Public License for more details.\n\n"
       "You should have received a copy of the GNU General Public License "
       "along with YAD. If not, see <http://www.gnu.org/licenses/>.");

  gchar *comments = g_strdup_printf (_("Yet Another Dialog\n"
                                       "(show dialog boxes from shell scripts)\n"
                                       "\nBased on Zenity code\n\n"
#ifdef HAVE_HTML
                                       "Built with Webkit\n"
#endif
#ifdef HAVE_SOURCEVIEW
                                       "Built with GtkSourceView\n"
#endif
#ifdef HAVE_SPELL
                                       "Built with GtkSpell\n"
#endif
                                       "Using GTK+ %d.%d.%d\n"),
                                     gtk_major_version, gtk_minor_version, gtk_micro_version);

  dialog = gtk_about_dialog_new ();
  gtk_window_set_icon_name (GTK_WINDOW (dialog), "yad");
  g_object_set (G_OBJECT (dialog),
                "name", PACKAGE_NAME,
                "version", PACKAGE_VERSION,
                "copyright", "Copyright \xc2\xa9 2008-2017, Victor Ananjevsky <ananasik@gmail.com>",
                "comments", comments,
                "authors", authors,
                "website", PACKAGE_URL,
                "translator-credits", translators,
                "wrap-license", TRUE, 
                "license", license, 
                "logo-icon-name", "yad",
                NULL);

  return gtk_dialog_run (GTK_DIALOG (dialog));
}
