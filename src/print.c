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

#include <sys/types.h>
#include <unistd.h>

#include <gtk/gtkunixprint.h>

#include "yad.h"

#define HEADER_HEIGHT (10*72/25.4)
#define HEADER_GAP (3*72/25.4)
#define HEADER_FONT "Sans 11"

#define FONTNAME "Monospace"
#define FONTSIZE 11.0

static gchar **text;
static gint nlines, npages;

static PangoFontDescription *fdesc = NULL;

static void
draw_header (GtkPrintContext * cnt, gint pn, gint pc)
{
  cairo_t *cr;
  PangoFontDescription *desc;
  PangoLayout *layout;
  gint pw, tw, th;
  gchar *page;

  cr = gtk_print_context_get_cairo_context (cnt);
  pw = gtk_print_context_get_width (cnt);

  layout = gtk_print_context_create_pango_layout (cnt);

  desc = pango_font_description_from_string (HEADER_FONT);
  pango_layout_set_font_description (layout, desc);
  pango_font_description_free (desc);

  pango_layout_set_text (layout, options.common_data.uri, -1);
  pango_layout_get_pixel_size (layout, &tw, &th);
  if (tw > pw)
    {
      pango_layout_set_width (layout, pw);
      pango_layout_set_ellipsize (layout, PANGO_ELLIPSIZE_START);
      pango_layout_get_pixel_size (layout, &tw, &th);
    }

  cairo_move_to (cr, (pw - tw) / 2, (HEADER_HEIGHT - th) / 2);
  pango_cairo_show_layout (cr, layout);

  page = g_strdup_printf ("%d/%d", pn, pc);
  pango_layout_set_text (layout, page, -1);
  g_free (page);

  pango_layout_set_width (layout, -1);
  pango_layout_get_pixel_size (layout, &tw, &th);
  cairo_move_to (cr, pw - tw - 4, (HEADER_HEIGHT - th) / 2);
  pango_cairo_show_layout (cr, layout);

  g_object_unref (layout);

  cairo_move_to (cr, 0.0, HEADER_HEIGHT);
  cairo_line_to (cr, pw, HEADER_HEIGHT);

  cairo_set_source_rgb (cr, 0, 0, 0);
  cairo_set_line_width (cr, 1);
  cairo_stroke (cr);
}

static void
begin_print_text (GtkPrintOperation * op, GtkPrintContext * cnt, gpointer data)
{
  gchar *buf;
  gint i = 0;
  gdouble ph;

  /* load file */
  g_file_get_contents (options.common_data.uri, &buf, NULL, NULL);
  text = g_strsplit (buf, "\n", 0);
  g_free (buf);

  while (text[i] != NULL)
    i++;

  ph = gtk_print_context_get_height (cnt);
  if (options.print_data.headers)
    ph -= HEADER_HEIGHT + HEADER_GAP;

  nlines = ph / FONTSIZE;
  npages = i / nlines + 1;
  gtk_print_operation_set_n_pages (op, npages);

  /* set font */
  if (options.common_data.font)
    fdesc = pango_font_description_from_string (options.common_data.font);
  else
    {
      fdesc = pango_font_description_from_string (FONTNAME);
      pango_font_description_set_size (fdesc, FONTSIZE * PANGO_SCALE);
    }
}

static void
draw_page_text (GtkPrintOperation * op, GtkPrintContext * cnt, gint page, gpointer data)
{
  cairo_t *cr;
  PangoLayout *layout;
  gint i, line;

  cr = gtk_print_context_get_cairo_context (cnt);

  /* create header */
  if (options.print_data.headers)
    draw_header (cnt, page + 1, npages);

  /* add text */
  layout = gtk_print_context_create_pango_layout (cnt);
  pango_layout_set_font_description (layout, fdesc);

  cairo_move_to (cr, 0, HEADER_HEIGHT + HEADER_GAP);

  line = page * nlines;
  for (i = 0; i < nlines; i++)
    {
      if (text[line + i] == NULL)
        break;
      pango_layout_set_text (layout, text[line + i], -1);
      pango_cairo_show_layout (cr, layout);
      cairo_rel_move_to (cr, 0, FONTSIZE);
    }

  g_object_unref (layout);
}

static void
draw_page_image (GtkPrintOperation * op, GtkPrintContext * cnt, gint page, gpointer data)
{
  cairo_t *cr;
  GdkPixbuf *pb, *spb;
  guint iw, ih;
  gdouble pw, ph;
  gdouble factor;

  cr = gtk_print_context_get_cairo_context (cnt);

  pw = gtk_print_context_get_width (cnt);
  ph = gtk_print_context_get_height (cnt);
  if (options.print_data.headers)
    ph -= HEADER_HEIGHT + HEADER_GAP;

  /* create header */
  if (options.print_data.headers)
    draw_header (cnt, 1, 1);

  /* scale image to page size */
  pb = gdk_pixbuf_new_from_file (options.common_data.uri, NULL);
  iw = gdk_pixbuf_get_width (pb);
  ih = gdk_pixbuf_get_height (pb);

  if (pw < iw || ph < ih)
    {
      factor = MIN (pw / iw, ph / ih);
      factor = (factor > 1.0) ? 1.0 : factor;
      spb = gdk_pixbuf_scale_simple (pb, iw * factor, ih * factor, GDK_INTERP_HYPER);
    }
  else
    spb = g_object_ref (pb);
  g_object_unref (pb);

  /* add image to surface */
  gdk_cairo_set_source_pixbuf (cr, spb, 0.0, HEADER_HEIGHT + HEADER_GAP);
  cairo_paint (cr);
  g_object_unref (spb);
}

static void
raw_print_done (GtkPrintJob * job, gint * ret, GError * err)
{
  if (err)
    {
      g_printerr (_("Printing failed: %s\n"), err->message);
      *ret = 1;
    }
  yad_exit (options.data.def_resp);
}

static void
size_allocate_cb (GtkWidget * w, GtkAllocation * al, gpointer data)
{
  gtk_widget_set_size_request (w, al->width, -1);
}

gint
yad_print_run (void)
{
  GtkWidget *dlg;
  GtkWidget *box, *img, *lbl;
  gchar *uri, *job_name = NULL;
  GtkPrintCapabilities pcap;
  GtkPrintOperationAction act = GTK_PRINT_OPERATION_ACTION_PRINT;
  gint resp, ret = 0;
  GError *err = NULL;

  /* check if file is exists */
  if (options.common_data.uri && options.common_data.uri[0])
    {
      if (!g_file_test (options.common_data.uri, G_FILE_TEST_EXISTS))
        {
          g_printerr (_("File %s not found.\n"), options.common_data.uri);
          return 1;
        }
    }
  else
    {
      g_printerr (_("Filename is not specified.\n"));
      return 1;
    }

  /* create print dialog */
  dlg = gtk_print_unix_dialog_new (options.data.dialog_title, NULL);
  gtk_window_set_type_hint (GTK_WINDOW (dlg), GDK_WINDOW_TYPE_HINT_NORMAL);
  gtk_print_unix_dialog_set_embed_page_setup (GTK_PRINT_UNIX_DIALOG (dlg), TRUE);
  pcap = GTK_PRINT_CAPABILITY_PAGE_SET | GTK_PRINT_CAPABILITY_COPIES |
    GTK_PRINT_CAPABILITY_COLLATE | GTK_PRINT_CAPABILITY_REVERSE |
    GTK_PRINT_CAPABILITY_NUMBER_UP | GTK_PRINT_CAPABILITY_NUMBER_UP_LAYOUT;
  if (options.common_data.preview && options.print_data.type != YAD_PRINT_RAW)
    pcap |= GTK_PRINT_CAPABILITY_PREVIEW;
  gtk_print_unix_dialog_set_manual_capabilities (GTK_PRINT_UNIX_DIALOG (dlg), pcap);

  if (!settings.print_settings)
    settings.print_settings = gtk_print_unix_dialog_get_settings (GTK_PRINT_UNIX_DIALOG (dlg));

  uri = g_build_filename (g_get_current_dir (), "yad.pdf", NULL);
  gtk_print_settings_set (settings.print_settings, "output-uri", g_filename_to_uri (uri, NULL, NULL));
  g_free (uri);

  gtk_print_unix_dialog_set_settings (GTK_PRINT_UNIX_DIALOG (dlg), settings.print_settings);

  if (settings.page_setup)
    gtk_print_unix_dialog_set_page_setup (GTK_PRINT_UNIX_DIALOG (dlg), settings.page_setup);

  /* set window behavior */
  gtk_widget_set_name (dlg, "yad-dialog-window");
  if (options.data.sticky)
    gtk_window_stick (GTK_WINDOW (dlg));
  gtk_window_set_resizable (GTK_WINDOW (dlg), !options.data.fixed);
  gtk_window_set_keep_above (GTK_WINDOW (dlg), options.data.ontop);
  gtk_window_set_decorated (GTK_WINDOW (dlg), !options.data.undecorated);
  gtk_window_set_skip_taskbar_hint (GTK_WINDOW (dlg), options.data.skip_taskbar);
  gtk_window_set_skip_pager_hint (GTK_WINDOW (dlg), options.data.skip_taskbar);

  /* set window size and position */
  if (!options.data.geometry)
    {
      gtk_window_set_default_size (GTK_WINDOW (dlg), options.data.width, options.data.height);
      if (options.data.center)
        gtk_window_set_position (GTK_WINDOW (dlg), GTK_WIN_POS_CENTER);
      else if (options.data.mouse)
        gtk_window_set_position (GTK_WINDOW (dlg), GTK_WIN_POS_MOUSE);
    }
  else
    {
      /* parse geometry, if given. must be after showing widget */
      gtk_widget_realize (dlg);
      gtk_window_parse_geometry (GTK_WINDOW (dlg), options.data.geometry);
    }

  /* create yad's top box */
  if (options.data.dialog_text || options.data.dialog_image)
    {
#if !GTK_CHECK_VERSION(3,0,0)
      box = gtk_hbox_new (FALSE, 0);
#else
      box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
#endif

      if (options.data.dialog_image)
        {
          GdkPixbuf *pb = NULL;

          pb = get_pixbuf (options.data.dialog_image, YAD_BIG_ICON);
          img = gtk_image_new_from_pixbuf (pb);
          if (pb)
            g_object_unref (pb);

          gtk_widget_set_name (img, "yad-dialog-image");
          gtk_box_pack_start (GTK_BOX (box), img, FALSE, FALSE, 2);
        }
      if (options.data.dialog_text)
        {
          gchar *buf = g_strcompress (options.data.dialog_text);

          lbl = gtk_label_new (NULL);
          if (!options.data.no_markup)
            gtk_label_set_markup (GTK_LABEL (lbl), buf);
          else
            gtk_label_set_text (GTK_LABEL (lbl), buf);
          gtk_widget_set_name (lbl, "yad-dialog-label");
          gtk_label_set_selectable (GTK_LABEL (lbl), options.data.selectable_labels);
          gtk_misc_set_alignment (GTK_MISC (lbl), options.data.text_align, 0.5);
          if (options.data.geometry || options.data.width != -1)
            gtk_label_set_line_wrap (GTK_LABEL (lbl), TRUE);
          gtk_box_pack_start (GTK_BOX (box), lbl, TRUE, TRUE, 2);
          g_signal_connect (G_OBJECT (lbl), "size-allocate", G_CALLBACK (size_allocate_cb), NULL);
          g_free (buf);
        }

      /* add tob box to dialog */
      gtk_widget_show_all (box);
      gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dlg))), box, TRUE, TRUE, 5);
      gtk_box_reorder_child (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dlg))), box, 0);
    }

  do
    {
      resp = gtk_dialog_run (GTK_DIALOG (dlg));
      switch (resp)
        {
        case GTK_RESPONSE_APPLY:   /* ask for preview */
          act = GTK_PRINT_OPERATION_ACTION_PREVIEW;
        case GTK_RESPONSE_OK:      /* run print */
          settings.print_settings = gtk_print_unix_dialog_get_settings (GTK_PRINT_UNIX_DIALOG (dlg));
          settings.page_setup = gtk_print_unix_dialog_get_page_setup (GTK_PRINT_UNIX_DIALOG (dlg));
          job_name = g_strdup_printf ("yad-%s-%d", g_path_get_basename (options.common_data.uri), getpid ());
          if (options.print_data.type != YAD_PRINT_RAW)
            {
              /* print text or image */
              GtkPrintOperation *op = gtk_print_operation_new ();
              gtk_print_operation_set_unit (op, GTK_UNIT_POINTS);
              gtk_print_operation_set_print_settings (op, settings.print_settings);
              gtk_print_operation_set_default_page_setup (op, settings.page_setup);
              gtk_print_operation_set_job_name (op, job_name);

              switch (options.print_data.type)
                {
                case YAD_PRINT_TEXT:
                  g_signal_connect (G_OBJECT (op), "begin-print", G_CALLBACK (begin_print_text), NULL);
                  g_signal_connect (G_OBJECT (op), "draw-page", G_CALLBACK (draw_page_text), NULL);
                  break;
                case YAD_PRINT_IMAGE:
                  gtk_print_operation_set_n_pages (op, 1);
                  g_signal_connect (G_OBJECT (op), "draw-page", G_CALLBACK (draw_page_image), NULL);
                  break;
                default:;
                }

              if (gtk_print_operation_run (op, act, NULL, &err) == GTK_PRINT_OPERATION_RESULT_ERROR)
                {
                  g_printerr (_("Printing failed: %s\n"), err->message);
                  ret = 1;
                }
            }
          else
            {
              /* print raw ps or pdf data */
              GtkPrinter *prnt;
              GtkPrintJob *job;

              prnt = gtk_print_unix_dialog_get_selected_printer (GTK_PRINT_UNIX_DIALOG (dlg));

              if (g_str_has_suffix (options.common_data.uri, ".ps"))
                {
                  if (!gtk_printer_accepts_ps (prnt))
                    {
                      g_printerr (_("Printer doesn't support ps format.\n"));
                      ret = 1;
                    }
                }
              else if (g_str_has_suffix (options.common_data.uri, ".pdf"))
                {
                  if (!gtk_printer_accepts_pdf (prnt))
                    {
                      g_printerr (_("Printer doesn't support pdf format.\n"));
                      ret = 1;
                    }
                }
              else
                {
                  g_printerr (_("This file type is not supported for raw printing.\n"));
                  ret = 1;
                }
              if (ret == 1)
                break;

              job = gtk_print_job_new (job_name, prnt, settings.print_settings, settings.page_setup);
              if (gtk_print_job_set_source_file (job, options.common_data.uri, &err))
                {
                  gtk_print_job_send (job, (GtkPrintJobCompleteFunc) raw_print_done, &ret, NULL);
                  gtk_main ();
                }
              else
                {
                  g_printerr (_("Load source file failed: %s\n"), err->message);
                  ret = 1;
                }
            }
          break;
        default:
          ret = 1;
          break;
        }
    }
  while (resp == GTK_RESPONSE_APPLY);

  gtk_widget_destroy (dlg);
  write_settings ();
  return ret;
}
