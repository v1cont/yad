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

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <locale.h>
#include <stdlib.h>
#include <signal.h>
#include <stdio.h>
#include <unistd.h>

#ifndef G_OS_WIN32
# include <sys/shm.h>
# include <gdk/gdkx.h>
#endif

#include "yad.h"

YadOptions options;
GtkIconTheme *yad_icon_theme;

#ifndef STANDALONE
GSettings *settings;
GSettings *sv_settings;
#endif

GdkPixbuf *big_fallback_image = NULL;
GdkPixbuf *small_fallback_image = NULL;

/* global flag for temporary disable ESC handling (needed for searchbar) */
gboolean ignore_esc = FALSE;

static GtkWidget *dialog = NULL;
static GtkWidget *text = NULL;

static gint ret = YAD_RESPONSE_ESC;

static gboolean is_x11 = FALSE;

YadNTabs *tabs;

#ifndef G_OS_WIN32
static void
sa_usr1 (gint sig)
{
  if (options.plug != -1)
    yad_print_result ();
  else
    yad_exit (options.data.def_resp);
}

static void
sa_usr2 (gint sig)
{
  if (options.plug != -1)
    gtk_main_quit ();
  else
    yad_exit (YAD_RESPONSE_CANCEL);
}
#endif

static gboolean
keys_cb (GtkWidget *w, GdkEventKey *ev, gpointer d)
{
  if (options.plug != -1)
    return FALSE;

  switch (ev->keyval)
    {
    case GDK_KEY_F1:
      if (options.data.f1_action)
        {
          run_command_async (options.data.f1_action);
          return TRUE;
        }
      break;
    case GDK_KEY_Escape:
      if (!ignore_esc)
        {
          if (options.data.escape_ok)
            yad_exit (options.data.def_resp);
          else if (!options.data.no_escape)
            yad_exit (YAD_RESPONSE_ESC);
          return TRUE;
        }
      break;
    case GDK_KEY_Return:
    case GDK_KEY_KP_Enter:
      if (ev->state & GDK_CONTROL_MASK)
        {
          yad_exit (options.data.def_resp);
          return TRUE;
        }
      break;
     }

   return FALSE;
}

static void
btn_cb (GtkWidget *b, gchar *cmd)
{
  if (cmd)
    run_command_async (cmd);
  else
    {
      gint resp = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (b), "resp"));
      yad_exit (resp);
    }
}

static gboolean
timeout_cb (gpointer data)
{
  static guint count = 1;
  GtkWidget *w = (GtkWidget *) data;

  if (options.data.timeout < count)
    {
      yad_exit (YAD_RESPONSE_TIMEOUT);
      return FALSE;
    }

  if (w)
    {
      gdouble percent = ((gdouble) options.data.timeout - count) / (gdouble) options.data.timeout;
      gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (w), percent);
#ifndef STANDALONE
      if (g_settings_get_boolean (settings, "show-remain"))
#else
      if (SHOW_REMAIN)
#endif
        {
          gchar *lbl = g_strdup_printf (_("%d sec"), options.data.timeout - count);
          gtk_progress_bar_set_text (GTK_PROGRESS_BAR (w), lbl);
          g_free (lbl);
        }
    }

  count++;

  return TRUE;
}

static gboolean
unfocus_cb (GtkWidget *w, GdkEventFocus *ev, gpointer d)
{
  if (options.data.close_on_unfocus)
    gtk_main_quit ();
  return FALSE;
}

static gboolean
link_cb (GtkLabel *l, gchar *uri, gpointer d)
{
  open_uri (uri);
  return TRUE;
}

void
yad_exit (gint id)
{
  if ((options.mode == YAD_MODE_FILE) && !(id & 1))
    {
      /* show custom confirmation dialog */
      if (!file_confirm_overwrite (dialog))
        return;
    }

  ret = id;
  gtk_main_quit ();
}

static GtkWidget *
create_layout (GtkWidget *dlg)
{
  GtkWidget *image, *mw, *imw, *layout, *exp, *box;

  layout = image = mw = exp = NULL;

  /* create image */
  if (options.data.dialog_image)
    {
      GdkPixbuf *pb = NULL;

      pb = get_pixbuf (options.data.dialog_image, YAD_BIG_ICON, FALSE);
      image = gtk_image_new_from_pixbuf (pb);
      if (pb)
        g_object_unref (pb);

      gtk_widget_set_name (image, "yad-dialog-image");
      gtk_widget_set_halign (image, GTK_ALIGN_CENTER);
      gtk_widget_set_valign (image, GTK_ALIGN_START);
    }

  /* create text label */
  if (options.data.dialog_text)
    {
      /* for dnd's tooltip we don't need text label */
      if (options.mode != YAD_MODE_DND || !options.dnd_data.tooltip)
        {
          gchar *buf = g_strcompress (options.data.dialog_text);

          text = gtk_label_new (NULL);
          gtk_widget_set_name (text, "yad-dialog-label");
          gtk_label_set_line_wrap (GTK_LABEL (text), TRUE);
          gtk_label_set_selectable (GTK_LABEL (text), options.data.selectable_labels);
          gtk_widget_set_state_flags (text, GTK_STATE_FLAG_NORMAL, FALSE);
          gtk_widget_set_can_focus (text, FALSE);

          if (options.data.text_width > 0)
            gtk_label_set_width_chars (GTK_LABEL (text), options.data.text_width);

          /* set label align and justification */
          switch (options.data.text_align)
            {
            case GTK_JUSTIFY_LEFT:
            case GTK_JUSTIFY_FILL:
              gtk_label_set_xalign (GTK_LABEL (text), 0.0);
              break;
            case GTK_JUSTIFY_RIGHT:
              gtk_label_set_xalign (GTK_LABEL (text), 1.0);
              break;
            case GTK_JUSTIFY_CENTER:
              gtk_label_set_xalign (GTK_LABEL (text), 0.5);
              break;
            }
          gtk_label_set_justify (GTK_LABEL (text), options.data.text_align);

          if (!options.data.no_markup)
            {
              gtk_label_set_markup (GTK_LABEL (text), buf);
              g_signal_connect (G_OBJECT (text), "activate-link", G_CALLBACK (link_cb), NULL);
            }
          else
            gtk_label_set_text (GTK_LABEL (text), buf);
          g_free (buf);
        }
    }

  /* create main widget */
  switch (options.mode)
    {
    case YAD_MODE_APP:
      mw = app_create_widget (dlg);
      break;
    case YAD_MODE_CALENDAR:
      mw = calendar_create_widget (dlg);
      break;
    case YAD_MODE_COLOR:
      mw = color_create_widget (dlg);
      break;
    case YAD_MODE_ENTRY:
      mw = entry_create_widget (dlg);
      break;
    case YAD_MODE_FILE:
      mw = file_create_widget (dlg);
      break;
    case YAD_MODE_FONT:
      mw = font_create_widget (dlg);
      break;
    case YAD_MODE_FORM:
      mw = form_create_widget (dlg);
      break;
#ifdef HAVE_HTML
    case YAD_MODE_HTML:
      mw = html_create_widget (dlg);
      break;
#endif
    case YAD_MODE_ICONS:
      mw = icons_create_widget (dlg);
      break;
    case YAD_MODE_LIST:
      mw = list_create_widget (dlg);
      break;
    case YAD_MODE_NOTEBOOK:
      if (options.plug == -1)
        mw = notebook_create_widget (dlg);
      break;
    case YAD_MODE_PANED:
      if (options.plug == -1)
        mw = paned_create_widget (dlg);
      break;
    case YAD_MODE_PICTURE:
      mw = picture_create_widget (dlg);
      break;
    case YAD_MODE_PROGRESS:
      mw = progress_create_widget (dlg);
      break;
    case YAD_MODE_SCALE:
      mw = scale_create_widget (dlg);
      break;
    case YAD_MODE_TEXTINFO:
      mw = text_create_widget (dlg);
      break;
    default: ;
    }

  /* add expander */
  if (mw && options.data.expander)
    {
      imw = exp = gtk_expander_new_with_mnemonic (options.data.expander);
      gtk_expander_set_expanded (GTK_EXPANDER (exp), FALSE);
      gtk_container_add (GTK_CONTAINER (exp), mw);
    }
  else
    imw = mw;

  /* create layout */
  layout = gtk_box_new (GTK_ORIENTATION_VERTICAL, 5);
  box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 5);

  if (image)
    gtk_box_pack_start (GTK_BOX (box), image, FALSE, FALSE, 2);
  if (text)
    gtk_box_pack_start (GTK_BOX (box), text, TRUE, TRUE, 2);

  gtk_box_pack_start (GTK_BOX (layout), box, FALSE, FALSE, 0);
  if (imw)
    {
      if (options.mode == YAD_MODE_ENTRY)
        gtk_box_pack_start (GTK_BOX (layout), imw, TRUE, FALSE, 0);
      else
        gtk_box_pack_start (GTK_BOX (layout), imw, TRUE, TRUE, 0);
    }

  if (options.mode == YAD_MODE_DND)
    dnd_init (layout);

  return layout;
}

static void
realize_cb (GtkWidget *dlg, gpointer d)
{
  GdkGravity gravity = GDK_GRAVITY_NORTH_WEST;

  if (options.data.fixed)
    {
      gtk_widget_set_size_request (dlg, options.data.width, options.data.height);
      gtk_window_set_resizable (GTK_WINDOW (dlg), !options.data.fixed);
    }
  else
    {
      gint cw, ch;
      /* get current window size for gtk_window_resize */
      gtk_window_get_size (GTK_WINDOW (dlg), &cw, &ch);
      if (options.data.width == -1)
        options.data.width = cw;
      if (options.data.height == -1)
        options.data.height = ch;
      gtk_window_resize (GTK_WINDOW (dlg), options.data.width, options.data.height);
    }

  if (options.data.use_posx || options.data.use_posy)
    {
      gint ww, wh, sw, sh;
      gtk_window_get_size (GTK_WINDOW (dlg), &ww, &wh);
      gdk_window_get_geometry (gdk_get_default_root_window (), NULL, NULL, &sw, &sh);
      /* place window to specified coordinates */
      if (!options.data.use_posx)
        gtk_window_get_position (GTK_WINDOW (dlg), &options.data.posx, NULL);
      if (!options.data.use_posy)
        gtk_window_get_position (GTK_WINDOW (dlg), NULL, &options.data.posy);
      if (options.data.negx)
        {
          options.data.posx = sw - ww + options.data.posx;
          gravity = GDK_GRAVITY_NORTH_EAST;
        }
      if (options.data.negy)
        {
          options.data.posy = sh - wh + options.data.posy;
          gravity = GDK_GRAVITY_SOUTH_WEST;
        }
      if (options.data.negx && options.data.negy)
        gravity = GDK_GRAVITY_SOUTH_EAST;
      gtk_window_set_gravity (GTK_WINDOW (dlg), gravity);
      gtk_window_move (GTK_WINDOW (dlg), options.data.posx, options.data.posy);
    }
}

static GtkWidget *
create_dialog (void)
{
  GtkWidget *dlg, *vbox, *layout;

  /* create dialog window */
  dlg = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  if (options.data.splash)
    gtk_window_set_type_hint (GTK_WINDOW (dlg), GDK_WINDOW_TYPE_HINT_SPLASHSCREEN);
  gtk_window_set_title (GTK_WINDOW (dlg), options.data.dialog_title);
  gtk_widget_set_name (dlg, "yad-dialog-window");

  g_signal_connect (G_OBJECT (dlg), "delete-event", G_CALLBACK (gtk_main_quit), NULL);
  g_signal_connect (G_OBJECT (dlg), "key-press-event", G_CALLBACK (keys_cb), NULL);
  g_signal_connect (G_OBJECT (dlg), "focus-out-event", G_CALLBACK (unfocus_cb), NULL);

  /* set window icon */
  if (options.data.window_icon)
    {
      if (g_file_test (options.data.window_icon, G_FILE_TEST_EXISTS))
        gtk_window_set_icon_from_file (GTK_WINDOW (dlg), options.data.window_icon, NULL);
      else
        gtk_window_set_icon_name (GTK_WINDOW (dlg), options.data.window_icon);
    }
  else
    gtk_window_set_icon_name (GTK_WINDOW (dlg), "yad");

  /* set window borders */
  if (options.data.borders < 0)
    options.data.borders = 2;
  gtk_container_set_border_width (GTK_CONTAINER (dlg), (guint) options.data.borders);

  /* set window size and position */
  if (!options.data.maximized && !options.data.fullscreen)
    {
      if (options.data.center)
        gtk_window_set_position (GTK_WINDOW (dlg), GTK_WIN_POS_CENTER_ALWAYS);
      else if (options.data.mouse)
        gtk_window_set_position (GTK_WINDOW (dlg), GTK_WIN_POS_MOUSE);
    }

  /* set window behavior */
  if (options.data.sticky)
    gtk_window_stick (GTK_WINDOW (dlg));
  gtk_window_set_keep_above (GTK_WINDOW (dlg), options.data.ontop);
  gtk_window_set_decorated (GTK_WINDOW (dlg), !options.data.undecorated);
  gtk_window_set_skip_taskbar_hint (GTK_WINDOW (dlg), options.data.skip_taskbar);
  gtk_window_set_skip_pager_hint (GTK_WINDOW (dlg), options.data.skip_taskbar);
  gtk_window_set_accept_focus (GTK_WINDOW (dlg), options.data.focus);

  /* create box */
  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 2);
  gtk_container_add (GTK_CONTAINER (dlg), vbox);

  layout = create_layout (dlg);

  /* create timeout indicator widget */
  if (options.data.timeout)
    {
      GtkWidget *cbox = NULL, *topb = NULL;

      if (G_LIKELY (options.data.to_indicator) && strcasecmp (options.data.to_indicator, "none") != 0)
        {
          topb = gtk_progress_bar_new ();
          gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (topb), 1.0);
          gtk_widget_set_name (topb, "yad-timeout-indicator");
        }

      /* add timeout indicator */
      if (topb)
        {
          if (strcasecmp (options.data.to_indicator, "top") == 0)
            {
              gtk_orientable_set_orientation (GTK_ORIENTABLE (topb), GTK_ORIENTATION_HORIZONTAL);
              cbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
              gtk_box_pack_start (GTK_BOX (cbox), topb, FALSE, FALSE, 2);
              gtk_box_pack_end (GTK_BOX (cbox), layout, TRUE, TRUE, 0);
            }
          else if (strcasecmp (options.data.to_indicator, "bottom") == 0)
            {
              gtk_orientable_set_orientation (GTK_ORIENTABLE (topb), GTK_ORIENTATION_HORIZONTAL);
              cbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
              gtk_box_pack_start (GTK_BOX (cbox), layout, TRUE, TRUE, 0);
              gtk_box_pack_end (GTK_BOX (cbox), topb, FALSE, FALSE, 2);
            }
          else if (strcasecmp (options.data.to_indicator, "left") == 0)
            {
              gtk_orientable_set_orientation (GTK_ORIENTABLE (topb), GTK_ORIENTATION_VERTICAL);
              gtk_progress_bar_set_inverted (GTK_PROGRESS_BAR (topb), TRUE);
              cbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
              gtk_box_pack_start (GTK_BOX (cbox), topb, FALSE, FALSE, 2);
              gtk_box_pack_end (GTK_BOX (cbox), layout, TRUE, TRUE, 0);
            }
          else if (strcasecmp (options.data.to_indicator, "right") == 0)
            {
              gtk_orientable_set_orientation (GTK_ORIENTABLE (topb), GTK_ORIENTATION_VERTICAL);
              gtk_progress_bar_set_inverted (GTK_PROGRESS_BAR (topb), TRUE);
              cbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
              gtk_box_pack_start (GTK_BOX (cbox), layout, TRUE, TRUE, 0);
              gtk_box_pack_end (GTK_BOX (cbox), topb, FALSE, FALSE, 2);
            }

#ifndef STANDALONE
          if (g_settings_get_boolean (settings, "show-remain"))
#else
          if (SHOW_REMAIN)
#endif
            {
              gchar *lbl = g_strdup_printf (_("%d sec"), options.data.timeout);
              gtk_progress_bar_set_show_text (GTK_PROGRESS_BAR (topb), TRUE);
              gtk_progress_bar_set_text (GTK_PROGRESS_BAR (topb), lbl);
              g_free (lbl);
            }
        }
      else
        {
          cbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
          gtk_box_pack_start (GTK_BOX (cbox), layout, TRUE, TRUE, 0);
        }

      if (cbox)
        gtk_box_pack_start (GTK_BOX (vbox), cbox, TRUE, TRUE, 0);

      /* set timeout handler */
      g_timeout_add_seconds (1, timeout_cb, topb);
    }
  else
    gtk_box_pack_start (GTK_BOX (vbox), layout, TRUE, TRUE, 0);

#ifdef HAVE_HTML
  /* enable no-buttons mode if --browser is specified and sets no custom buttons */
  if (options.mode == YAD_MODE_HTML && options.html_data.browser && !options.data.buttons)
    options.data.no_buttons = TRUE;
#endif

  if (!options.data.no_buttons)
    {
      GtkWidget *btn;
      /* create buttons container */
      GtkWidget *bbox = gtk_button_box_new (GTK_ORIENTATION_HORIZONTAL);
      gtk_box_set_homogeneous (GTK_BOX (bbox), TRUE);
      gtk_container_set_border_width (GTK_CONTAINER (bbox), 2);
      gtk_box_set_spacing (GTK_BOX (bbox), 5);
      gtk_button_box_set_layout (GTK_BUTTON_BOX (bbox), options.data.buttons_layout);

      /* add buttons */
      if (options.data.buttons)
        {
          GSList *tmp = options.data.buttons;
          do
            {
              YadButton *b = (YadButton *) tmp->data;

              btn = gtk_button_new ();
              gtk_container_add (GTK_CONTAINER (btn), get_label (b->name, 2, btn));
              g_object_set_data (G_OBJECT (btn), "resp", GINT_TO_POINTER (b->response));
              g_signal_connect (G_OBJECT (btn), "clicked", G_CALLBACK (btn_cb), b->cmd);
              gtk_box_pack_start (GTK_BOX (bbox), btn, TRUE, TRUE, 0);

              tmp = tmp->next;
            }
          while (tmp != NULL);
        }
      else
        {
          if (options.mode == YAD_MODE_PROGRESS || options.mode == YAD_MODE_DND || options.mode == YAD_MODE_PICTURE)
            {
              /* add close button */
              btn = gtk_button_new ();
              gtk_container_add (GTK_CONTAINER (btn), get_label ("yad-close", 2, btn));
              g_object_set_data (G_OBJECT (btn), "resp", GINT_TO_POINTER (YAD_RESPONSE_OK));
              g_signal_connect (G_OBJECT (btn), "clicked", G_CALLBACK (btn_cb), NULL);
              gtk_box_pack_start (GTK_BOX (bbox), btn, TRUE, TRUE, 0);

            }
          else
            {
              /* add cancel button */
              btn = gtk_button_new ();
              gtk_container_add (GTK_CONTAINER (btn), get_label ("yad-cancel", 2, btn));
              g_object_set_data (G_OBJECT (btn), "resp", GINT_TO_POINTER (YAD_RESPONSE_CANCEL));
              g_signal_connect (G_OBJECT (btn), "clicked", G_CALLBACK (btn_cb), NULL);
              gtk_box_pack_start (GTK_BOX (bbox), btn, TRUE, TRUE, 0);

              /*add ok button */
              btn = gtk_button_new ();
              gtk_container_add (GTK_CONTAINER (btn), get_label ("yad-ok", 2, btn));
              g_object_set_data (G_OBJECT (btn), "resp", GINT_TO_POINTER (YAD_RESPONSE_OK));
              g_signal_connect (G_OBJECT (btn), "clicked", G_CALLBACK (btn_cb), NULL);
              gtk_box_pack_start (GTK_BOX (bbox), btn, TRUE, TRUE, 0);
            }
        }
      /* add buttons box to main window */
      gtk_box_pack_start (GTK_BOX (vbox), bbox, FALSE, FALSE, 0);
    }

  /* show widgets */
  gtk_widget_show_all (vbox);

  /* parse geometry or move window, if given. must be after showing widget */
  if (!options.data.maximized && !options.data.fullscreen)
    {
      parse_geometry ();
      g_signal_connect (G_OBJECT (dlg), "realize", G_CALLBACK (realize_cb), NULL);
      gtk_widget_show_all (dlg);
    }
  else
    {
      gtk_widget_show (dlg);
      /* set maximized or fixed size after showing widget */
      if (options.data.maximized)
        gtk_window_maximize (GTK_WINDOW (dlg));
      else if (options.data.fullscreen)
        gtk_window_fullscreen (GTK_WINDOW (dlg));
    }

  /* print xid */
  if (is_x11 && options.print_xid)
    {
      FILE *xf;

      if (options.xid_file)
        xf = fopen (options.xid_file, "w");
      else
        xf = stderr;

      if (xf)
        {
          fprintf (xf, "0x%lX\n", GDK_WINDOW_XID (gtk_widget_get_window (dlg)));

          if (options.xid_file)
            fclose (xf);
          else
            fflush (xf);
        }
    }

  return dlg;
}

static void
create_plug (void)
{
  GtkWidget *win, *box;

  tabs = get_tabs (options.plug, FALSE);
  while (!tabs)
    {
      usleep (1000);
      tabs = get_tabs (options.plug, FALSE);
    }

  while (!tabs[0].xid)
    usleep (1000);

  win = gtk_plug_new (0);
  /* set window borders */
  if (options.data.borders == -1)
    options.data.borders = (gint) gtk_container_get_border_width (GTK_CONTAINER (win));
  gtk_container_set_border_width (GTK_CONTAINER (win), (guint) options.data.borders);

  box = create_layout (win);
  if (box)
    gtk_container_add (GTK_CONTAINER (win), box);

  gtk_widget_show_all (win);

  /* add plug data */
  /* notebook/paned will count non-zero xids */
  tabs[options.tabnum].pid = getpid ();
  tabs[options.tabnum].xid = gtk_plug_get_id (GTK_PLUG (win));
  shmdt (tabs);
}

void
yad_print_result (void)
{
  switch (options.mode)
    {
    case YAD_MODE_APP:
      app_print_result ();
      break;
    case YAD_MODE_CALENDAR:
      calendar_print_result ();
      break;
    case YAD_MODE_COLOR:
      color_print_result ();
      break;
    case YAD_MODE_ENTRY:
      entry_print_result ();
      break;
    case YAD_MODE_FILE:
      file_print_result ();
      break;
    case YAD_MODE_FONT:
      font_print_result ();
      break;
    case YAD_MODE_FORM:
      form_print_result ();
      break;
    case YAD_MODE_LIST:
      list_print_result ();
      break;
    case YAD_MODE_NOTEBOOK:
      notebook_print_result ();
      break;
    case YAD_MODE_PANED:
      paned_print_result ();
      break;
    case YAD_MODE_SCALE:
      scale_print_result ();
      break;
    case YAD_MODE_TEXTINFO:
      text_print_result ();
      break;
    default:;
    }
}

gint
main (gint argc, gchar ** argv)
{
  GOptionContext *ctx;
  GError *err = NULL;
  gint w, h;
  gchar *str;

  setlocale (LC_ALL, "");

#ifdef ENABLE_NLS
  bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);
  bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
  textdomain (GETTEXT_PACKAGE);
#endif

  gtk_init (&argc, &argv);
  g_set_application_name ("YAD");

#ifndef STANDALONE
  settings = g_settings_new ("yad.settings");
  sv_settings = g_settings_new ("yad.sourceview");
#endif

  yad_icon_theme = gtk_icon_theme_get_default ();

  yad_options_init ();

  ctx = yad_create_context ();
  /* parse YAD_OPTIONS */
  if (g_getenv ("YAD_OPTIONS"))
    {
      gchar *cmd, **args = NULL;
      gint cnt;

      cmd = g_strdup_printf ("yad %s", g_getenv ("YAD_OPTIONS"));

      if (g_shell_parse_argv (cmd, &cnt, &args, &err))
        {
          g_option_context_parse (ctx, &cnt, &args, &err);
          if (err)
            {
              g_printerr (_("Unable to parse YAD_OPTIONS: %s\n"), err->message);
              g_error_free (err);
              err = NULL;
            }
        }
      else
        {
          g_printerr (_("Unable to parse YAD_OPTIONS: %s\n"), err->message);
          g_error_free (err);
          err = NULL;
        }

      g_free (cmd);
    }
  /* parse command line */
  g_option_context_parse (ctx, &argc, &argv, &err);
  if (err)
    {
      g_printerr (_("Unable to parse command line: %s\n"), err->message);
      return -1;
    }
  yad_set_mode ();

  /* check for current GDK backend */
#ifdef GDK_WINDOWING_X11
  if (GDK_IS_X11_DISPLAY (gdk_display_get_default ()))
    is_x11 = TRUE;
#endif

  /* parse custom css */
  if (options.css)
    {
      GtkCssProvider *css = gtk_css_provider_new ();

      if (g_file_test (options.css, G_FILE_TEST_EXISTS))
        gtk_css_provider_load_from_path (css, options.css, NULL);
      else
        gtk_css_provider_load_from_data (css, options.css, -1, NULL);
      gtk_style_context_add_provider_for_screen (gdk_screen_get_default (), GTK_STYLE_PROVIDER (css),
                                                 GTK_STYLE_PROVIDER_PRIORITY_USER);
      g_object_unref (css);
    }
  else if (options.gtkrc_file)
    {
      GtkCssProvider *css = gtk_css_provider_new ();

      if (options.debug)
        g_printerr (_("WARNING: Using gtkrc option is deprecated, please use --css instead\n"));

      gtk_css_provider_load_from_path (css, options.gtkrc_file, NULL);
      gtk_style_context_add_provider_for_screen (gdk_screen_get_default (), GTK_STYLE_PROVIDER (css),
                                                 GTK_STYLE_PROVIDER_PRIORITY_USER);
      g_object_unref (css);
    }

  /* set default icons and icon theme */
  if (options.data.icon_theme)
    gtk_icon_theme_set_custom_theme (yad_icon_theme, options.data.icon_theme);
  gtk_icon_size_lookup (GTK_ICON_SIZE_DIALOG, &w, &h);
  big_fallback_image = gtk_icon_theme_load_icon (yad_icon_theme, "yad", MIN (w, h), GTK_ICON_LOOKUP_GENERIC_FALLBACK, NULL);
  gtk_icon_size_lookup (GTK_ICON_SIZE_MENU, &w, &h);
  small_fallback_image = gtk_icon_theme_load_icon (yad_icon_theme, "yad", MIN (w, h), GTK_ICON_LOOKUP_GENERIC_FALLBACK, NULL);

  /* correct separators */
  str = g_strcompress (options.common_data.separator);
  options.common_data.separator = str;
  str = g_strcompress (options.common_data.item_separator);
  options.common_data.item_separator = str;

  /* loads an extra arguments, if specified */
  if (options.rest_file)
    {
      GIOChannel *ioc;
      gchar *buf;
      guint len, line = 0;

      g_strfreev (options.extra_data);
      options.extra_data = NULL;

      ioc = g_io_channel_new_file (options.rest_file, "r", NULL);
      while (TRUE)
        {
          gint status = g_io_channel_read_line (ioc, &buf, NULL, NULL, NULL);

          if (status != G_IO_STATUS_NORMAL)
            break;

          /* remove \n at the end of string */
          len = strlen (buf);
          if (buf[len - 1] == '\n')
            buf[len - 1] = '\0';

          /* add line to arguments array */
          options.extra_data = g_realloc (options.extra_data, (line + 2) * sizeof (gchar *));
          options.extra_data[line] = g_strcompress (buf);
          options.extra_data[line + 1] = NULL;

          g_free (buf);
          line++;
        }
      g_io_channel_shutdown (ioc, FALSE, NULL);
    }

#ifndef G_OS_WIN32
  /* add YAD_PID variable */
  str = g_strdup_printf ("%d", getpid ());
  g_setenv ("YAD_PID", str, TRUE);
  /* set signal handlers */
  signal (SIGUSR1, sa_usr1);
  signal (SIGUSR2, sa_usr2);
#endif

  if (!is_x11 && options.plug != -1)
    {
      options.plug = -1;
      if (options.debug)
        g_printerr (_("WARNING: --plug mode not supported outside X11\n"));
    }

  /* plug mode */
  if (options.plug != -1)
    {
      create_plug ();
      gtk_main ();
      shmdt (tabs);
      return ret;
    }

  if (!is_x11)
    {
      if (options.mode == YAD_MODE_NOTEBOOK || options.mode == YAD_MODE_PANED
#ifdef HAVE_TRAY
          || options.mode == YAD_MODE_NOTIFICATION
#endif
         )
        {
          g_printerr (_("WARNING: This mode not supported outside X11\n"));
          return 1;
        }
    }

  switch (options.mode)
    {
    case YAD_MODE_ABOUT:
      ret = yad_about ();
      break;

    case YAD_MODE_VERSION:
      g_print ("%s (GTK+ %d.%d.%d)\n", VERSION, gtk_major_version, gtk_minor_version, gtk_micro_version);
      break;

#ifdef HAVE_TRAY
    case YAD_MODE_NOTIFICATION:
      ret = yad_notification_run ();
      break;
#endif

    case YAD_MODE_PRINT:
      ret = yad_print_run ();
      break;

    default:
      dialog = create_dialog ();

      if (is_x11)
        {
          /* add YAD_XID variable */
          str = g_strdup_printf ("0x%lX", GDK_WINDOW_XID (gtk_widget_get_window (dialog)));
          g_setenv ("YAD_XID", str, TRUE);
        }

      /* make some specific init actions */
      if (options.mode == YAD_MODE_NOTEBOOK)
        notebook_swallow_childs ();
      else if (options.mode == YAD_MODE_PANED)
        paned_swallow_childs ();
      else if (options.mode == YAD_MODE_TEXTINFO)
        text_goto_line ();
      else if (options.mode == YAD_MODE_PICTURE)
        {
          if (options.picture_data.size == YAD_PICTURE_FIT)
            picture_fit_to_window ();
        }

      if (text && options.data.selectable_labels)
        gtk_label_select_region (GTK_LABEL (text), 0, 0);

      /* run main loop */
      gtk_main ();

      /* print results */
      if (ret != YAD_RESPONSE_TIMEOUT && ret != YAD_RESPONSE_ESC)
        {
          if (options.data.always_print)
            yad_print_result ();
          else
            {
              /* standard OK button pressed */
              if (ret == YAD_RESPONSE_OK && options.data.buttons == NULL)
                yad_print_result ();
              /* custom even button pressed */
              else if (!(ret & 1))
                yad_print_result ();
            }
        }
#ifndef G_OS_WIN32
      if (options.mode == YAD_MODE_NOTEBOOK)
        notebook_close_childs ();
      else if (options.mode == YAD_MODE_PANED)
        paned_close_childs ();
      /* autokill option for progress dialog */
      if (!options.kill_parent)
        {
          if (options.mode == YAD_MODE_PROGRESS && options.progress_data.autokill && ret != YAD_RESPONSE_OK)
            kill (getppid (), SIGHUP);
        }
#endif
    }

#ifndef G_OS_WIN32
  /* NSIG defined in signal.h */
  if (options.kill_parent > 0 && options.kill_parent < NSIG)
    kill (getppid (), options.kill_parent);
#endif

  return ret;
}
