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
 * Copyright (C) 2010-2017, Victor Ananjevsky <ananasik@gmail.com>
 */

#include <config.h>

#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include <gdk/gdkkeysyms.h>

typedef struct {
  GtkWidget *win;
  GtkWidget *image;
  GtkWidget *lname;
  GtkWidget *lsize;
  GtkWidget *lfile;
  GtkWidget *cat_list;
  GtkWidget *icon_list;

  GtkIconTheme *theme;

  GHashTable *icons;
} IconBrowserData;

static gboolean
key_press_cb (GtkWidget * w, GdkEventKey * ev, gpointer data)
{
#if GTK_CHECK_VERSION(2,24,0)
  if (ev->keyval == GDK_KEY_Escape)
#else
  if (ev->keyval == GDK_Escape)
#endif
    {
      gtk_main_quit ();
      return TRUE;
    }
  return FALSE;
}

static GtkListStore *
load_icon_cat (IconBrowserData * data, gchar * cat)
{
  GtkListStore *store;
  GList *i, *icons;
  gint size, w, h;

  gtk_icon_size_lookup (GTK_ICON_SIZE_MENU, &w, &h);
  size = MIN (w, h);

  store = gtk_list_store_new (2, GDK_TYPE_PIXBUF, G_TYPE_STRING);

  icons = gtk_icon_theme_list_icons (data->theme, cat);
  for (i = icons; i; i = i->next)
    {
      GtkTreeIter iter;
      GdkPixbuf *pb, *spb;

      spb = pb = gtk_icon_theme_load_icon (data->theme, i->data, size, GTK_ICON_LOOKUP_GENERIC_FALLBACK, NULL);

      if (pb)
        {
          /* scale pixbuf if needed */
          w = gdk_pixbuf_get_width (pb);
          h = gdk_pixbuf_get_height (pb);
          if (w > size || h > size)
            {
              pb = gdk_pixbuf_scale_simple (spb, size, size, GDK_INTERP_BILINEAR);
              g_object_unref (spb);
            }
        }

      gtk_list_store_append (store, &iter);
      gtk_list_store_set (store, &iter, 0, pb, 1, i->data, -1);

      if (pb)
        g_object_unref (pb);
      g_free (i->data);
    }
  g_list_free (icons);

  gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (store), 1, GTK_SORT_ASCENDING);

  return store;
}

static void
select_icon (GtkTreeSelection * sel, IconBrowserData * data)
{
  GtkTreeModel *model;
  GtkTreeIter iter;
  GtkIconInfo *info;
  gint *sz, i;
  gchar *icon, *file;
  GString *sizes;

  if (!gtk_tree_selection_get_selected (sel, &model, &iter))
    return;

  gtk_tree_model_get (model, &iter, 1, &icon, -1);

  gtk_image_set_from_icon_name (GTK_IMAGE (data->image), icon, GTK_ICON_SIZE_DIALOG);

  sz = gtk_icon_theme_get_icon_sizes (data->theme, icon);
  info = gtk_icon_theme_lookup_icon (data->theme, icon, sz[0], 0);

  if (info)
    file = (gchar *) gtk_icon_info_get_filename (info);
  else
    file = NULL;

  /* create sizes string */
  i = 0;
  sizes = g_string_new ("");
  while (sz[i])
    {
      if (sz[i] == -1)
        g_string_append (sizes, _("scalable "));
      else
        g_string_append_printf (sizes, "%dx%d ", sz[i], sz[i]);
      i++;
    }
  /* free memory */
  g_free (sz);

  gtk_label_set_text (GTK_LABEL (data->lname), icon);
  gtk_label_set_text (GTK_LABEL (data->lsize), sizes->str);
  gtk_label_set_text (GTK_LABEL (data->lfile), file ? file : _("built-in"));

  g_string_free (sizes, TRUE);

  if (info)
    gtk_icon_info_free (info);
}

static void
select_cat (GtkTreeSelection * sel, IconBrowserData * data)
{
  GtkTreeModel *model;
  GtkListStore *store;
  GtkTreeIter iter;
  gchar *cat;

  if (!gtk_tree_selection_get_selected (sel, &model, &iter))
    return;

  gtk_tree_model_get (model, &iter, 0, &cat, -1);

  store = g_hash_table_lookup (data->icons, cat);
  if (!store)
    {
      store = load_icon_cat (data, cat);
      g_hash_table_insert (data->icons, cat, store);
    }
  gtk_tree_view_set_model (GTK_TREE_VIEW (data->icon_list), GTK_TREE_MODEL (store));
}

gint
main (gint argc, gchar * argv[])
{
  IconBrowserData *data;
  gchar **themes = NULL;
  GList *ic, *icat;
  GtkListStore *store;
  GtkTreeSelection *sel;
  GtkTreeViewColumn *col;
  GtkCellRenderer *r;
  GtkWidget *w, *p, *box, *t;

  GOptionEntry entrs[] = {
    {G_OPTION_REMAINING, 0, 0, G_OPTION_ARG_STRING_ARRAY, &themes, NULL, NULL},
    {NULL}
  };

  data = g_new0 (IconBrowserData, 1);

#ifdef ENABLE_NLS
  bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);
  bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
  textdomain (GETTEXT_PACKAGE);
#endif

  data = g_new0 (IconBrowserData, 1);

  /* initialize GTK+ and parse the command line arguments */
  gtk_init_with_args (&argc, &argv, _("- Icon browser"), entrs, GETTEXT_PACKAGE, NULL);

  /* load icon theme */
  if (themes && themes[0])
    {
      data->theme = gtk_icon_theme_new ();
      gtk_icon_theme_set_custom_theme (data->theme, themes[0]);
    }
  else
    data->theme = gtk_icon_theme_get_default ();

  /* create interface */
  data->win = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title (GTK_WINDOW (data->win), _("Icon browser"));
  gtk_window_set_icon_name (GTK_WINDOW (data->win), "gtk-info");
  gtk_window_set_default_size (GTK_WINDOW (data->win), 500, 400);
  g_signal_connect (G_OBJECT (data->win), "delete-event", G_CALLBACK (gtk_main_quit), NULL);
  g_signal_connect (G_OBJECT (data->win), "key-press-event", G_CALLBACK (key_press_cb), NULL);

#if !GTK_CHECK_VERSION(3,0,0)
  box = gtk_vbox_new (FALSE, 5);
#else
  box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 5);
#endif
  gtk_container_add (GTK_CONTAINER (data->win), box);
  gtk_container_set_border_width (GTK_CONTAINER (data->win), 5);

  /* create icon info box */
#if !GTK_CHECK_VERSION(3,0,0)
  t = gtk_table_new (3, 3, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (t), 5);
  gtk_table_set_row_spacings (GTK_TABLE (t), 5);
#else
  t = gtk_grid_new ();
  gtk_grid_set_row_spacing (GTK_GRID (t), 5);
  gtk_grid_set_column_spacing (GTK_GRID (t), 5);
#endif
  gtk_box_pack_start (GTK_BOX (box), t, FALSE, FALSE, 2);

  data->image = gtk_image_new_from_stock ("gtk-missing-image", GTK_ICON_SIZE_DIALOG);
#if !GTK_CHECK_VERSION(3,0,0)
  gtk_table_attach (GTK_TABLE (t), data->image, 0, 1, 0, 3, GTK_FILL, 0, 0, 0);
#else
  gtk_grid_attach (GTK_GRID (t), data->image, 0, 0, 1, 3);
#endif

  w = gtk_label_new (NULL);
  gtk_label_set_markup (GTK_LABEL (w), _("<b>Name:</b>"));
  gtk_misc_set_alignment (GTK_MISC (w), 0, 0.5);
#if !GTK_CHECK_VERSION(3,0,0)
  gtk_table_attach (GTK_TABLE (t), w, 1, 2, 0, 1, GTK_FILL, 0, 0, 0);
#else
  gtk_grid_attach (GTK_GRID (t), w, 1, 0, 1, 1);
#endif
  data->lname = gtk_label_new (NULL);
  gtk_label_set_selectable (GTK_LABEL (data->lname), TRUE);
  gtk_misc_set_alignment (GTK_MISC (data->lname), 0, 0.5);
#if !GTK_CHECK_VERSION(3,0,0)
  gtk_table_attach (GTK_TABLE (t), data->lname, 2, 3, 0, 1, GTK_FILL | GTK_EXPAND, 0, 0, 0);
#else
  gtk_grid_attach (GTK_GRID (t), data->lname, 2, 0, 1, 1);
  gtk_widget_set_hexpand (data->lname, TRUE);
#endif

  w = gtk_label_new (NULL);
  gtk_label_set_markup (GTK_LABEL (w), _("<b>Sizes:</b>"));
  gtk_misc_set_alignment (GTK_MISC (w), 0, 0.5);
#if !GTK_CHECK_VERSION(3,0,0)
  gtk_table_attach (GTK_TABLE (t), w, 1, 2, 1, 2, GTK_FILL, 0, 0, 0);
#else
  gtk_grid_attach (GTK_GRID (t), w, 1, 1, 1, 1);
#endif
  data->lsize = gtk_label_new (NULL);
  gtk_label_set_selectable (GTK_LABEL (data->lsize), TRUE);
  gtk_misc_set_alignment (GTK_MISC (data->lsize), 0, 0.5);
#if !GTK_CHECK_VERSION(3,0,0)
  gtk_table_attach (GTK_TABLE (t), data->lsize, 2, 3, 1, 2, GTK_FILL | GTK_EXPAND, 0, 0, 0);
#else
  gtk_grid_attach (GTK_GRID (t), data->lsize, 2, 1, 1, 1);
  gtk_widget_set_hexpand (data->lsize, TRUE);
#endif

  w = gtk_label_new (NULL);
  gtk_label_set_markup (GTK_LABEL (w), _("<b>Filename:</b>"));
  gtk_misc_set_alignment (GTK_MISC (w), 0, 0.5);
#if !GTK_CHECK_VERSION(3,0,0)
  gtk_table_attach (GTK_TABLE (t), w, 1, 2, 2, 3, GTK_FILL, 0, 0, 0);
#else
  gtk_grid_attach (GTK_GRID (t), w, 1, 2, 1, 1);
#endif
  data->lfile = gtk_label_new (NULL);
  gtk_label_set_selectable (GTK_LABEL (data->lfile), TRUE);
  gtk_misc_set_alignment (GTK_MISC (data->lfile), 0, 0.5);
#if !GTK_CHECK_VERSION(3,0,0)
  gtk_table_attach (GTK_TABLE (t), data->lfile, 2, 3, 2, 3, GTK_FILL | GTK_EXPAND, 0, 0, 0);
#else
  gtk_grid_attach (GTK_GRID (t), data->lfile, 2, 2, 1, 1);
  gtk_widget_set_hexpand (data->lfile, TRUE);
#endif

  /* create icon browser */
#if !GTK_CHECK_VERSION(3,0,0)
  p = gtk_hpaned_new ();
#else
  p = gtk_paned_new (GTK_ORIENTATION_HORIZONTAL);
#endif
  gtk_paned_set_position (GTK_PANED (p), 150);
  gtk_box_pack_start (GTK_BOX (box), p, TRUE, TRUE, 2);

  /* create category list */
  w = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (w), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (w), GTK_SHADOW_ETCHED_IN);
  gtk_paned_add1 (GTK_PANED (p), w);

  store = gtk_list_store_new (1, G_TYPE_STRING);

  data->cat_list = gtk_tree_view_new_with_model (GTK_TREE_MODEL (store));
  gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (data->cat_list), TRUE);
  gtk_container_add (GTK_CONTAINER (w), data->cat_list);

  sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (data->cat_list));
  g_signal_connect (G_OBJECT (sel), "changed", G_CALLBACK (select_cat), data);

  r = gtk_cell_renderer_text_new ();
  col = gtk_tree_view_column_new_with_attributes (_("Category"), r, "text", 0, NULL);
  gtk_tree_view_column_set_expand (col, TRUE);
  gtk_tree_view_append_column (GTK_TREE_VIEW (data->cat_list), col);

  /* load icons category */
  data->icons = g_hash_table_new (g_direct_hash, g_direct_equal);
  icat = gtk_icon_theme_list_contexts (data->theme);
  for (ic = icat; ic; ic = ic->next)
    {
      GtkTreeIter iter;

      gtk_list_store_append (store, &iter);
      gtk_list_store_set (store, &iter, 0, ic->data, -1);
      g_free (ic->data);
    }
  g_list_free (icat);

  /* create icons list */
  w = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (w), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (w), GTK_SHADOW_ETCHED_IN);
  gtk_paned_add2 (GTK_PANED (p), w);

  data->icon_list = gtk_tree_view_new ();
  gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (data->icon_list), TRUE);
  gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (data->icon_list), TRUE);
  gtk_container_add (GTK_CONTAINER (w), data->icon_list);

  sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (data->icon_list));
  g_signal_connect (G_OBJECT (sel), "changed", G_CALLBACK (select_icon), data);

  col = gtk_tree_view_column_new ();
  gtk_tree_view_column_set_title (col, _("Icons"));
  gtk_tree_view_column_set_sort_column_id (col, 1);
  r = gtk_cell_renderer_pixbuf_new ();
  gtk_tree_view_column_pack_start (col, r, FALSE);
  gtk_tree_view_column_set_attributes (col, r, "pixbuf", 0, NULL);
  r = gtk_cell_renderer_text_new ();
  gtk_tree_view_column_pack_start (col, r, FALSE);
  gtk_tree_view_column_set_attributes (col, r, "text", 1, NULL);
  gtk_tree_view_column_set_expand (col, TRUE);
  gtk_tree_view_append_column (GTK_TREE_VIEW (data->icon_list), col);

  gtk_widget_show_all (data->win);

  /* run it */
  gtk_main ();

  return 0;
}
