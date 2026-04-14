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
 * Copyright (C) 2008-2026, Victor Ananjevsky <victor@sanana.kiev.ua>
 */

#include <stdlib.h>

#include "yad.h"

YadSettings *settings = NULL;
gboolean write_settings = FALSE;

static gchar *
get_settings_file ()
{
  gchar *filename = NULL;

  if (g_getenv ("YAD_SETTINGS_FILE") != NULL)
    filename = g_strdup (g_getenv ("YAD_SETTINGS_FILE"));
  else
    filename = g_build_filename (g_get_user_config_dir (), "yad", "settings.conf", NULL);

  return filename;
}

void
yad_load_settings ()
{
  gchar *filename;
  gchar *buf = NULL;

  settings = g_new0 (YadSettings, 1);

  settings->width = -1;
  settings->height = -1;
  settings->border = 5;
  settings->terminal = g_strdup ("xterm -e \"%s\"");
  settings->open_command = g_strdup ("xdg-open \"%s\"");
  settings->date_format = g_strdup ("%x");
  settings->uri_color = "blue";
  settings->max_tab = 100;
  settings->search_width = -1;
  settings->ignore_unknown_options = TRUE;

#ifdef HAVE_SOURCEVIEW
  settings->sv = g_new0 (YadSVSettings, 1);

  settings->sv->mark1_color = "lightgreen";
  settings->sv->mark2_color = "pink";
  settings->sv->homend = GTK_SOURCE_SMART_HOME_END_DISABLED;
  settings->sv->tab_width = 8;
  settings->sv->indent_width = 4;
  settings->sv->spaces = TRUE;
#endif

  filename = get_settings_file ();
  if (g_file_test (filename, G_FILE_TEST_EXISTS))
    {
      GKeyFile *kf;

      kf = g_key_file_new ();
      if (g_key_file_load_from_file (kf, filename, G_KEY_FILE_NONE, NULL))
        {
          if (g_key_file_has_group (kf, "Common"))
            {
              if (g_key_file_has_key (kf, "Common", "Width", NULL))
                settings->width = g_key_file_get_integer (kf, "Common", "Width", NULL);
              if (g_key_file_has_key (kf, "Common", "Height", NULL))
                settings->height = g_key_file_get_integer (kf, "Common", "Height", NULL);
              if (g_key_file_has_key (kf, "Common", "Border", NULL))
                settings->border = g_key_file_get_integer (kf, "Common", "Border", NULL);
              if (g_key_file_has_key (kf, "Common", "ShowRemain", NULL))
                settings->show_remain = g_key_file_get_boolean (kf, "Common", "ShowRemain", NULL);
              if (g_key_file_has_key (kf, "Common", "ComboAlwaysEditable", NULL))
                settings->combo_always_editable = g_key_file_get_boolean (kf, "Common", "ComboAlwaysEditable", NULL);
              if (g_key_file_has_key (kf, "Common", "Terminal", NULL))
                {
                  buf = g_key_file_get_string (kf, "Common", "Terminal", NULL);
                  if (buf)
                    {
                      g_free (settings->terminal);
                      settings->terminal = buf;
                    }
                }
              if (g_key_file_has_key (kf, "Common", "OpenCommand", NULL))
                {
                  buf = g_key_file_get_string (kf, "Common", "OpenCommand", NULL);
                  if (buf)
                    {
                      g_free (settings->open_command);
                      settings->open_command = buf;
                    }
                }
              if (g_key_file_has_key (kf, "Common", "DateFormat", NULL))
                {
                  buf = g_key_file_get_string (kf, "Common", "DateFormat", NULL);
                  if (buf)
                    {
                      g_free (settings->date_format);
                      settings->date_format = buf;
                    }
                }
              if (g_key_file_has_key (kf, "Common", "URIColor", NULL))
                settings->uri_color = g_key_file_get_string (kf, "Common", "URIColor", NULL);
              if (g_key_file_has_key (kf, "Common", "MaxTab", NULL))
                settings->max_tab = g_key_file_get_integer (kf, "Common", "MaxTab", NULL);
              if (g_key_file_has_key (kf, "Common", "LargePreview", NULL))
                settings->large_preview = g_key_file_get_boolean (kf, "Common", "LargePreview", NULL);
              if (g_key_file_has_key (kf, "Common", "SearchWidth", NULL))
                settings->max_tab = g_key_file_get_integer (kf, "Common", "SearchWidth", NULL);
              if (g_key_file_has_key (kf, "Common", "IgnoreUnknownOptions", NULL))
                settings->ignore_unknown_options = g_key_file_get_boolean (kf, "Common", "IgnoreUnknownOptions", NULL);
              if (g_key_file_has_key (kf, "Common", "Debug", NULL))
                settings->debug = g_key_file_get_boolean (kf, "Common", "Debug", NULL);
            }

#ifdef HAVE_SOURCEVIEW
          if (g_key_file_has_group (kf, "SourceView"))
            {
              /* theme is NULL here */
              if (g_key_file_has_key (kf, "SourceView", "Theme", NULL))
                settings->sv->theme = g_key_file_get_string (kf, "SourceView", "Theme", NULL);
              if (g_key_file_has_key (kf, "SourceView", "LineNum", NULL))
                settings->sv->line_num = g_key_file_get_boolean (kf, "SourceView", "LineNum", NULL);
              if (g_key_file_has_key (kf, "SourceView", "LineHilite", NULL))
                settings->sv->line_hl = g_key_file_get_boolean (kf, "SourceView", "LineHilite", NULL);
              if (g_key_file_has_key (kf, "SourceView", "LineMarks", NULL))
                settings->sv->line_marks = g_key_file_get_boolean (kf, "SourceView", "LineMarks", NULL);
              if (g_key_file_has_key (kf, "SourceView", "Mark1Color", NULL))
                settings->sv->mark1_color = g_key_file_get_string (kf, "SourceView", "Mark1Color", NULL);
              if (g_key_file_has_key (kf, "SourceView", "Mark2Color", NULL))
                settings->sv->mark2_color = g_key_file_get_string (kf, "SourceView", "Mark2Color", NULL);
              if (g_key_file_has_key (kf, "SourceView", "RightMargin", NULL))
                settings->sv->right_margin = g_key_file_get_integer (kf, "SourceView", "RightMargin", NULL);
              if (g_key_file_has_key (kf, "SourceView", "Brackets", NULL))
                settings->sv->brackets = g_key_file_get_boolean (kf, "SourceView", "Brackets", NULL);
              if (g_key_file_has_key (kf, "SourceView", "Indent", NULL))
                settings->sv->indent = g_key_file_get_boolean (kf, "SourceView", "Indent", NULL);
              if (g_key_file_has_key (kf, "SourceView", "HomeEnd", NULL))
                settings->sv->homend = g_key_file_get_integer (kf, "SourceView", "HomeEnd", NULL);
              if (g_key_file_has_key (kf, "SourceView", "SmartBS", NULL))
                settings->sv->smart_bs = g_key_file_get_boolean (kf, "SourceView", "SmartBS", NULL);
              if (g_key_file_has_key (kf, "SourceView", "TabWidth", NULL))
                settings->sv->tab_width = g_key_file_get_integer (kf, "SourceView", "TabWidth", NULL);
              if (g_key_file_has_key (kf, "SourceView", "IndentWidth", NULL))
                settings->sv->indent_width = g_key_file_get_integer (kf, "SourceView", "IndentWidth", NULL);
              if (g_key_file_has_key (kf, "SourceView", "Spaces", NULL))
                settings->sv->spaces = g_key_file_get_boolean (kf, "SourceView", "Spaces", NULL);
            }

            if (settings->sv->homend < 0 || settings->sv->homend > 3)
              {
                if (settings->debug)
                  g_printerr (_("Value (%d) of HomeEnd settings is out of range. Reset to default\n"),
                              settings->sv->homend);
                settings->sv->homend = GTK_SOURCE_SMART_HOME_END_DISABLED;
              }
#endif
        }
      g_key_file_free (kf);
    }
  g_free (filename);
}

void
yad_write_settings ()
{
  gchar *filename;
  gchar *path;
  GKeyFile *kf;
  GError *err = NULL;

  filename = get_settings_file ();
  if (g_file_test (filename, G_FILE_TEST_EXISTS))
    {
      if (settings->debug)
        g_printerr (_("Settings file exists. Do nothing\n"));
      return;
    }

  kf = g_key_file_new ();

  g_key_file_set_integer (kf, "Common", "Width", settings->width);
  g_key_file_set_comment (kf, "Common", "Width", " Default width of dialog window", NULL);
  g_key_file_set_integer (kf, "Common", "Height", settings->height);
  g_key_file_set_comment (kf, "Common", "Height", " Default height of dialog window", NULL);
  g_key_file_set_integer (kf, "Common", "Border", settings->border);
  g_key_file_set_comment (kf, "Common", "Border", " Borders around dialog", NULL);
  g_key_file_set_boolean (kf, "Common", "ShowRemain", settings->show_remain);
  g_key_file_set_comment (kf, "Common", "ShowRemain", " Show remaining time and percentage in timeout progress bar", NULL);
  g_key_file_set_boolean (kf, "Common", "ComboAlwaysEditable", settings->combo_always_editable);
  g_key_file_set_comment (kf, "Common", "ComboAlwaysEditable", " Combo-box in entry dialog is always editable", NULL);
  g_key_file_set_string (kf, "Common", "Terminal", settings->terminal);
  g_key_file_set_comment (kf, "Common", "Terminal", " Default terminal command (use %s for arguments placeholder)", NULL);
  g_key_file_set_string (kf, "Common", "OpenCommand", settings->open_command);
  g_key_file_set_comment (kf, "Common", "OpenCommand", " Default open command (use %s for arguments placeholder)", NULL);
  g_key_file_set_string (kf, "Common", "DateFormat", settings->date_format);
  g_key_file_set_comment (kf, "Common", "DateFormat", " Default date format (see strftime(3) for details)", NULL);
  g_key_file_set_string (kf, "Common", "URIColor", settings->uri_color);
  g_key_file_set_comment (kf, "Common", "URIColor", " Default color for URI highlight in text-info dialog", NULL);
  g_key_file_set_integer (kf, "Common", "MaxTab", settings->max_tab);
  g_key_file_set_comment (kf, "Common", "MaxTab", " Maximum number of tabs in notebook dialog", NULL);
  g_key_file_set_boolean (kf, "Common", "LargePreview", settings->large_preview);
  g_key_file_set_comment (kf, "Common", "LargePreview", " Use large previews in file selection dialogs)", NULL);
  g_key_file_set_integer (kf, "Common", "SearchWidth", settings->max_tab);
  g_key_file_set_comment (kf, "Common", "SearchWidth", " Set width of search entry in search bar", NULL);
  g_key_file_set_boolean (kf, "Common", "IgnoreUnknownOptions", settings->ignore_unknown_options);
  g_key_file_set_comment (kf, "Common", "IgnoreUnknownOptions", " Ignore unknown command-line options", NULL);
  g_key_file_set_boolean (kf, "Common", "Debug", settings->debug);
  g_key_file_set_comment (kf, "Common", "Debug", " Enable debug mode with information about deprecated features", NULL);

#ifdef HAVE_SOURCEVIEW
  g_key_file_set_string (kf, "SourceView", "Theme", (settings->sv->theme ? settings->sv->theme : ""));
  g_key_file_set_comment (kf, "SourceView", "Theme", " Default color theme for text-info dialog", NULL);
  g_key_file_set_boolean (kf, "SourceView", "LineNum", settings->sv->line_num);
  g_key_file_set_comment (kf, "SourceView", "LineNum", " Show line numbers", NULL);
  g_key_file_set_boolean (kf, "SourceView", "LineHilite", settings->sv->line_hl);
  g_key_file_set_comment (kf, "SourceView", "LineHilite", " Highlight current line", NULL);
  g_key_file_set_boolean (kf, "SourceView", "LineMarks", settings->sv->line_marks);
  g_key_file_set_comment (kf, "SourceView", "LineMarks", " Enable line marks mode", NULL);
  g_key_file_set_string (kf, "SourceView", "Mark1Color", settings->sv->mark1_color);
  g_key_file_set_comment (kf, "SourceView", "Mark1Color", " Default color for first type of text marks", NULL);
  g_key_file_set_string (kf, "SourceView", "Mark2Color", settings->sv->mark2_color);
  g_key_file_set_comment (kf, "SourceView", "Mark2Color", " Default color for second type of text marks", NULL);
  g_key_file_set_integer (kf, "SourceView", "RightMargin", settings->sv->right_margin);
  g_key_file_set_comment (kf, "SourceView", "RightMargin", " Show right margin at position (0 to disable)", NULL);
  g_key_file_set_boolean (kf, "SourceView", "Brackets", settings->sv->brackets);
  g_key_file_set_comment (kf, "SourceView", "Brackets", " Highlight matching brackets", NULL);
  g_key_file_set_boolean (kf, "SourceView", "Indent", settings->sv->indent);
  g_key_file_set_comment (kf, "SourceView", "Indent", " Use autoindent", NULL);
  g_key_file_set_integer (kf, "SourceView", "HomeEnd", settings->sv->homend);
  g_key_file_set_comment (kf, "SourceView", "HomeEnd",
                          " Smart Home/End behavior (0 - never, 1 - before, 2 - after, 3 - always)", NULL);
  g_key_file_set_boolean (kf, "SourceView", "SmartBS", settings->sv->smart_bs);
  g_key_file_set_comment (kf, "SourceView", "SmartBS", " Use smart backspace", NULL);
  g_key_file_set_integer (kf, "SourceView", "TabWidth", settings->sv->tab_width);
  g_key_file_set_comment (kf, "SourceView", "TabWidth", " Default tabulation width", NULL);
  g_key_file_set_integer (kf, "SourceView", "IndentWidth", settings->sv->indent_width);
  g_key_file_set_comment (kf, "SourceView", "IndentWidth", " Default indentation width", NULL);
  g_key_file_set_boolean (kf, "SourceView", "Spaces", settings->sv->spaces);
  g_key_file_set_comment (kf, "SourceView", "Spaces", " Insert spaces instead of tabs", NULL);
#endif

  /* make sure that directory exists */
  path = g_path_get_dirname (filename);
  g_mkdir_with_parents (path, 0700);
  g_free (path);

  if (!g_key_file_save_to_file (kf, filename, &err))
    g_warning (_("Unable to write settings: %s\n"), err->message);
  g_key_file_free (kf);

  g_free (filename);
}
