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

static void
yad_set_about_license (GtkWidget *dlg)
{
  if (options.about_data.license == NULL)
    {
      gtk_about_dialog_set_license_type (GTK_ABOUT_DIALOG (dlg), GTK_LICENSE_UNKNOWN);
      return;
    }

  gtk_about_dialog_set_wrap_license (GTK_ABOUT_DIALOG (dlg), TRUE);

  /* check for predefined */
  if (strncmp (options.about_data.license, "GPL2", 4) == 0)
    {
      gtk_about_dialog_set_license_type (GTK_ABOUT_DIALOG (dlg), GTK_LICENSE_GPL_2_0);
      return;
    }
  if (strncmp (options.about_data.license, "GPL3", 4) == 0)
    {
      gtk_about_dialog_set_license_type (GTK_ABOUT_DIALOG (dlg), GTK_LICENSE_GPL_3_0);
      return;
    }
  if (strncmp (options.about_data.license, "LGPL2", 5) == 0)
    {
      gtk_about_dialog_set_license_type (GTK_ABOUT_DIALOG (dlg), GTK_LICENSE_LGPL_2_1);
      return;
    }
  if (strncmp (options.about_data.license, "LGPL3", 5) == 0)
    {
      gtk_about_dialog_set_license_type (GTK_ABOUT_DIALOG (dlg), GTK_LICENSE_LGPL_3_0);
      return;
    }
  if (strncmp (options.about_data.license, "BSD", 3) == 0)
    {
      gtk_about_dialog_set_license_type (GTK_ABOUT_DIALOG (dlg), GTK_LICENSE_BSD);
      return;
    }
  if (strncmp (options.about_data.license, "MIT", 3) == 0)
    {
      gtk_about_dialog_set_license_type (GTK_ABOUT_DIALOG (dlg), GTK_LICENSE_MIT_X11);
      return;
    }
  if (strncmp (options.about_data.license, "ARTISTIC", 8) == 0)
    {
      gtk_about_dialog_set_license_type (GTK_ABOUT_DIALOG (dlg), GTK_LICENSE_ARTISTIC);
      return;
    }

  /* user specified */
  gtk_about_dialog_set_license_type (GTK_ABOUT_DIALOG (dlg), GTK_LICENSE_CUSTOM);
  if (g_file_test (options.about_data.license, G_FILE_TEST_EXISTS))
    {
      gchar *buf;

      if (g_file_get_contents (options.about_data.license, &buf, NULL, NULL))
        {
          gtk_about_dialog_set_license (GTK_ABOUT_DIALOG (dlg), buf);
          return;
        }
    }

  /* set as is */
  gtk_about_dialog_set_license (GTK_ABOUT_DIALOG (dlg), options.about_data.license);

  return;
}

gint
yad_about (void)
{
  GtkWidget *dialog;

  const gchar *const authors[] = {
    "Victor Ananjevsky <victor@sanana.kiev.ua>",
    NULL
  };
  const gchar *translators = N_("translator-credits");

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
                                       "Built with GSpell\n"
#endif
                                       "Using GTK+ %d.%d.%d\n"),
                                     gtk_major_version, gtk_minor_version, gtk_micro_version);

  dialog = gtk_about_dialog_new ();
  if (options.data.window_icon)
    gtk_window_set_icon_name (GTK_WINDOW (dialog), options.data.window_icon);
  else
    gtk_window_set_icon_name (GTK_WINDOW (dialog), "yad");

  if (options.about_data.name != NULL)
    {
      /* custom about dialog */
      gtk_about_dialog_set_program_name (GTK_ABOUT_DIALOG (dialog), options.about_data.name);
      if (options.data.dialog_image)
        gtk_about_dialog_set_logo (GTK_ABOUT_DIALOG (dialog), get_pixbuf (options.data.dialog_image, YAD_BIG_ICON, TRUE));
      if (options.about_data.version)
        gtk_about_dialog_set_version (GTK_ABOUT_DIALOG (dialog), options.about_data.version);
      if (options.about_data.copyright)
        gtk_about_dialog_set_copyright (GTK_ABOUT_DIALOG (dialog), options.about_data.copyright);
      if (options.about_data.comments)
        gtk_about_dialog_set_comments (GTK_ABOUT_DIALOG (dialog), options.about_data.comments);
      if (options.about_data.authors)
        gtk_about_dialog_set_authors (GTK_ABOUT_DIALOG (dialog), (const gchar **) g_strsplit (options.about_data.authors, ",", -1));
      if (options.about_data.website)
        gtk_about_dialog_set_website (GTK_ABOUT_DIALOG (dialog), options.about_data.website);
      if (options.about_data.website_lbl)
        gtk_about_dialog_set_website_label (GTK_ABOUT_DIALOG (dialog), options.about_data.website_lbl);
      yad_set_about_license (dialog);
    }
  else
    {
      g_object_set (G_OBJECT (dialog),
                    "name", PACKAGE_NAME,
                    "version", PACKAGE_VERSION,
                    "copyright", "Copyright \xc2\xa9 2008-2023, Victor Ananjevsky <victor@sanana.kiev.ua>",
                    "comments", comments,
                    "authors", authors,
                    "website", PACKAGE_URL,
                    "translator-credits", translators,
                    "wrap-license", TRUE,
                    "license-type", GTK_LICENSE_GPL_3_0,
                    "logo-icon-name", "yad",
                    NULL);
    }

  return gtk_dialog_run (GTK_DIALOG (dialog));
}
