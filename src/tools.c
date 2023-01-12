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
 * Copyright (C) 2019-2023, Victor Ananjevsky <victor@sanana.kiev.ua>
 */

#include <config.h>
#include <locale.h>
#include <gtk/gtk.h>
#include <glib/gi18n.h>

#ifdef HAVE_SPELL
#include <gspell/gspell.h>
#endif

#ifdef HAVE_SOURCEVIEW
#include <gtksourceview/gtksource.h>
#endif

#include "cpicker.h"

typedef enum {
  PANGO_SPEC,
  XFT_SPEC
} FontType;

static gboolean set_font_type (const gchar *name, const gchar *val, gpointer d, GError **err);
static gboolean set_size_type (const gchar *name, const gchar *val, gpointer d, GError **err);

static FontType font_type = XFT_SPEC;

static gboolean pfd_mode = FALSE;
static gboolean icon_mode = FALSE;
static gboolean color_mode = FALSE;
static gboolean mime = FALSE;
static gboolean ver = FALSE;
#ifdef HAVE_SPELL
static gboolean langs_mode = FALSE;
#endif
#ifdef HAVE_SOURCEVIEW
static gboolean themes_mode = FALSE;
#endif
gboolean pick_color = FALSE;

static guint icon_size = 24;
static gchar *icon_theme_name = NULL;

static gchar **args = NULL;

static GOptionEntry ents[] = {
  { "version", 'v', 0, G_OPTION_ARG_NONE, &ver, N_("Print version"), NULL },
#ifdef HAVE_SPELL
  { "show-langs", 0, 0, G_OPTION_ARG_NONE, &langs_mode, N_("Show list of spell languages"), NULL },
#endif
#ifdef HAVE_SOURCEVIEW
  { "show-themes", 0, 0, G_OPTION_ARG_NONE, &themes_mode, N_("Show list of GtkSourceView themes"), NULL },
#endif
  { G_OPTION_REMAINING, 0, 0, G_OPTION_ARG_STRING_ARRAY, &args, NULL, N_("STRING ...") },
  { NULL }
};

static GOptionEntry pfd_ents[] = {
  { "pfd", 'f', G_OPTION_FLAG_IN_MAIN, G_OPTION_ARG_NONE, &pfd_mode, N_("Pango fonts description tools"), NULL },
  { "xft", 'x', G_OPTION_FLAG_NO_ARG, G_OPTION_ARG_CALLBACK, set_font_type, N_("Print font name in xft format"), NULL },
  { "pango", 'p', G_OPTION_FLAG_NO_ARG, G_OPTION_ARG_CALLBACK, set_font_type, N_("Print font name in pango format"), NULL },
  { NULL }
};

static GOptionEntry icon_ents[] = {
  { "icon", 'i', G_OPTION_FLAG_IN_MAIN, G_OPTION_ARG_NONE, &icon_mode, N_("Icon tools"), NULL },
  { "mime", 'm', 0, G_OPTION_ARG_NONE, &mime, N_("Get icon name for mime type"), NULL },
  { "size", 's', 0, G_OPTION_ARG_INT, &icon_size, N_("Use specified icon size"), N_("SIZE") },
  { "type", 't', 0, G_OPTION_ARG_CALLBACK, set_size_type, N_("Get icon size from GtkIconSize type"), N_("TYPE") },
  { "theme", 0, 0, G_OPTION_ARG_STRING, &icon_theme_name, N_("Use icon theme"), N_("THEME") },
  { NULL }
};

static GOptionEntry color_ents[] = {
  { "color", 'c', G_OPTION_FLAG_IN_MAIN, G_OPTION_ARG_NONE, &color_mode, N_("Color manipulation tools"), NULL },
  { "pick", 0, 0, G_OPTION_ARG_NONE, &pick_color, N_("Pick up screen color"), NULL },
  { NULL }
};

static gboolean
set_font_type (const gchar *name, const gchar *val, gpointer d, GError **err)
{
  gint i = 1;

  if (name[i] == '-')
    i = 2;

  if (name[i] == 'p')
    font_type = PANGO_SPEC;
  else if (name[i] == 'x')
    font_type = XFT_SPEC;
  else
    return FALSE;

  return TRUE;
}

static gboolean
set_size_type (const gchar *name, const gchar *val, gpointer d, GError **err)
{
  gint w = 0, h = 0;

  if (strcasecmp (val, "MENU") == 0)
    gtk_icon_size_lookup (GTK_ICON_SIZE_MENU, &w, &h);
  else if (strcasecmp (val, "BUTTON") == 0)
    gtk_icon_size_lookup (GTK_ICON_SIZE_MENU, &w, &h);
  else if (strcasecmp (val, "SMALL_TOOLBAR") == 0)
    gtk_icon_size_lookup (GTK_ICON_SIZE_SMALL_TOOLBAR, &w, &h);
  else if (strcasecmp (val, "LARGE_TOOLBAR") == 0)
    gtk_icon_size_lookup (GTK_ICON_SIZE_LARGE_TOOLBAR, &w, &h);
  else if (strcasecmp (val, "DND") == 0)
    gtk_icon_size_lookup (GTK_ICON_SIZE_DND, &w, &h);
  else if (strcasecmp (val, "DIALOG") == 0)
    gtk_icon_size_lookup (GTK_ICON_SIZE_DIALOG, &w, &h);

  if (w && h)
    icon_size = MIN (w, h);
}

static gchar *
parse_font (gchar *fn)
{
  gchar **fnt;
  gint i = 1;
  GString *pfd = g_string_new (NULL);

  fnt = g_strsplit (fn, ":",  -1);

  if (g_ascii_strcasecmp (fnt[0], "xft") != 0)
    return fnt[0];

  while (fnt[i])
    {
      if (g_ascii_strncasecmp (fnt[i], "style=", 6) == 0)
        g_string_append_printf (pfd, "%s ", fnt[i] + 6);
      else if (g_ascii_strncasecmp (fnt[i], "size=", 5) == 0)
        g_string_append_printf (pfd, "%s ", fnt[i] + 5);
      else if (g_ascii_strncasecmp (fnt[i], "pixelsize=", 10) == 0)
        g_string_append_printf (pfd, "%spx ", fnt[i] + 10);
      else
        g_string_append_printf (pfd, "%s ", fnt[i]);
      i++;
    }

  return pfd->str;
}

static gint
run_pfd_mode ()
{
  PangoFontDescription *fnd = NULL;

  if (!args || !args[0])
    {
      g_printerr (_("no font specified\n"));
      return 1;
    }

  /* parse font */
  fnd = pango_font_description_from_string (parse_font (args[0]));
  if (!fnd)
    {
      g_printerr (_("cannot get font description for for string \"%s\"\n"), args[0]);
      return 1;
    }

  /* print font */
  switch (font_type)
    {
    case PANGO_SPEC:
      g_print ("%s\n", pango_font_description_to_string (fnd));
      break;
    case XFT_SPEC:
      {
        PangoFontMask mask = pango_font_description_get_set_fields (fnd);

        if (mask & PANGO_FONT_MASK_FAMILY)
          g_print ("xft:%s", pango_font_description_get_family (fnd));
        else
          g_print ("xft:Sans"); /* simple default */

        if (mask & PANGO_FONT_MASK_WEIGHT)
          {
            switch (pango_font_description_get_weight (fnd))
              {
              case PANGO_WEIGHT_THIN:
                g_print (":Thin");
                break;
              case PANGO_WEIGHT_ULTRALIGHT:
                g_print (":Ultralight");
                break;
              case PANGO_WEIGHT_LIGHT:
                g_print (":Light");
                break;
              case PANGO_WEIGHT_SEMILIGHT:
                g_print (":Semilight");
                break;
              case PANGO_WEIGHT_BOOK:
                g_print (":Book");
                break;
              case PANGO_WEIGHT_NORMAL:
                break;
              case PANGO_WEIGHT_MEDIUM:
                g_print (":Medium");
                break;
              case PANGO_WEIGHT_SEMIBOLD:
                g_print (":Semibold");
                break;
              case PANGO_WEIGHT_BOLD:
                g_print (":Bold");
                break;
              case PANGO_WEIGHT_ULTRABOLD:
                g_print (":Ultrabold");
                break;
              case PANGO_WEIGHT_HEAVY:
                g_print (":Heavy");
                break;
              case PANGO_WEIGHT_ULTRAHEAVY:
                g_print (":Ultraheavy");
                break;
              }
          }

        if (mask & PANGO_FONT_MASK_STYLE)
          {
            switch (pango_font_description_get_style (fnd))
              {
              case PANGO_STYLE_NORMAL:
                break;
              case PANGO_STYLE_OBLIQUE:
                g_print (":Oblique");
                break;
              case PANGO_STYLE_ITALIC:
                g_print (":Italic");
                break;
              }
          }

        if (mask & PANGO_FONT_MASK_SIZE)
          {
            if (pango_font_description_get_size_is_absolute (fnd))
              g_print (":pixelsize=%d", pango_font_description_get_size (fnd) / PANGO_SCALE);
            else
              g_print (":size=%d", pango_font_description_get_size (fnd) / PANGO_SCALE);
          }

        g_print ("\n");

        break;
      }
    }

  return 0;
}

static gint
run_icon_mode ()
{
  GtkIconTheme *theme;
  GtkIconInfo *ii;
  gchar *output = NULL;

  if (!args || !args[0])
    {
      g_printerr (_("no icon specified\n"));
      return 1;
    }

  if (icon_theme_name)
    {
      theme = gtk_icon_theme_new ();
      gtk_icon_theme_set_custom_theme (theme, icon_theme_name);
    }
  else
    theme = gtk_icon_theme_get_default ();

  if (mime)
    {
      gchar *ctype = g_content_type_from_mime_type (args[0]);
      output = g_content_type_get_generic_icon_name (ctype);
    }
  else
    {
      ii = gtk_icon_theme_lookup_icon (theme, args[0], icon_size, 0);
      if (ii == NULL)
        return 1;
      output = (gchar *) gtk_icon_info_get_filename (ii);
    }

  if (output)
    g_print ("%s\n", output);
  else
    return 1;

  return 0;
}

static gint
run_color_mode ()
{
  if (pick_color)
    {
      gtk_init (NULL, NULL);
      yad_get_screen_color (NULL);
      gtk_main ();
    }

  return 0;
}

#ifdef HAVE_SPELL
static gint
show_langs ()
{
  const GList *lng;

  for (lng = gspell_language_get_available (); lng; lng = lng->next)
    {
      const GspellLanguage *l = lng->data;
      g_print ("%s\n", gspell_language_get_code (l));
    }

  return 0;
}
#endif

#ifdef HAVE_SOURCEVIEW
static gint
show_themes ()
{
  GtkSourceStyleSchemeManager *sm;
  const gchar **si;
  guint i = 0;

  sm = gtk_source_style_scheme_manager_get_default ();
  if ((si = (const gchar **) gtk_source_style_scheme_manager_get_scheme_ids (sm)) == NULL)
    return 1;

  while (si[i])
    {
      GtkSourceStyleScheme *s = gtk_source_style_scheme_manager_get_scheme (sm, si[i]);
      g_print ("%s\n", gtk_source_style_scheme_get_name (s));
      i++;
    }

  return 0;
}
#endif

int
main (int argc, char *argv[])
{
  GOptionContext *ctx;
  GOptionGroup *grp;
  GError *err = NULL;
  gint ret = 0;

  setlocale (LC_ALL, "");

#ifdef ENABLE_NLS
  bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);
  bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
  textdomain (GETTEXT_PACKAGE);
#endif

  gtk_init (&argc, &argv);

  ctx = g_option_context_new (_("- yad miscellaneous tools"));
  g_option_context_add_main_entries (ctx, ents, GETTEXT_PACKAGE);

  grp = g_option_group_new ("pfd", _("PFD mode"), _("Show pfd mode options"), NULL, NULL);
  g_option_group_add_entries (grp, pfd_ents);
  g_option_group_set_translation_domain (grp, GETTEXT_PACKAGE);
  g_option_context_add_group (ctx, grp);

  grp = g_option_group_new ("icon", _("Icon mode"), _("Show icon mode options"), NULL, NULL);
  g_option_group_add_entries (grp, icon_ents);
  g_option_group_set_translation_domain (grp, GETTEXT_PACKAGE);
  g_option_context_add_group (ctx, grp);

  grp = g_option_group_new ("color", _("Color mode"), _("Show color mode options"), NULL, NULL);
  g_option_group_add_entries (grp, color_ents);
  g_option_group_set_translation_domain (grp, GETTEXT_PACKAGE);
  g_option_context_add_group (ctx, grp);

  if (!g_option_context_parse (ctx, &argc, &argv, &err))
    {
      g_printerr (_("option parsing failed: %s\n"), err->message);
      return 1;
    }

  if (ver)
    {
      g_print ("%s\n", VERSION);
      return 0;
    }

  if (pfd_mode)
    ret = run_pfd_mode ();
  else if (icon_mode)
    ret = run_icon_mode ();
  else if (color_mode)
    ret = run_color_mode ();
#ifdef HAVE_SPELL
  else if (langs_mode)
    ret = show_langs ();
#endif
#ifdef HAVE_SOURCEVIEW
  else if (themes_mode)
    ret = show_themes ();
#endif
  else
    {
      g_printerr (_("no mode specified\n"));
      return 1;
    }

  return ret;
}
