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
 * Copyright (C) 2019, Victor Ananjevsky <ananasik@gmail.com>
 */

#include <config.h>

#include <glib.h>
#include <glib/gi18n.h>
#include <pango/pango.h>
#include <locale.h>

typedef enum {
  PANGO_SPEC,
  XFT_SPEC
} FontType;

static FontType font_type = XFT_SPEC;

static gboolean
set_type (const gchar *name, const gchar *val, gpointer d, GError **err)
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

static gchar **fonts = NULL;
static gboolean ver = FALSE;
static GOptionEntry ents[] = {
  { "xft", 'x', G_OPTION_FLAG_NO_ARG, G_OPTION_ARG_CALLBACK, set_type, N_("Print font name in xft format"), NULL },
  { "pango", 'p', G_OPTION_FLAG_NO_ARG, G_OPTION_ARG_CALLBACK, set_type, N_("Print font name in pango format"), NULL },
  { "version", 'v', 0, G_OPTION_ARG_NONE, &ver, N_("Print version"), NULL },
  { G_OPTION_REMAINING, 0, 0, G_OPTION_ARG_STRING_ARRAY, &fonts, 0, N_("FONT") },
  { NULL }
};

int
main (int argc, char *argv[])
{
  GOptionContext *ctx;
  GError *err = NULL;
  PangoFontDescription *fnd = NULL;

  setlocale (LC_ALL, "");

#ifdef ENABLE_NLS
  bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);
  bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
  textdomain (GETTEXT_PACKAGE);
#endif

  ctx = g_option_context_new (_("- convert font description"));
  g_option_context_add_main_entries (ctx, ents, GETTEXT_PACKAGE);
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

  if (!fonts || !fonts[0])
    {
      g_printerr (_("no font specified\n"));
      return 1;
    }

  /* parse font */
  fnd = pango_font_description_from_string (parse_font (fonts[0]));
  if (!fnd)
    {
      g_printerr (_("cannot get font description for for string \"%s\"\n"), fonts[0]);
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
