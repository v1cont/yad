#ifndef MENU_H
#define MENU_H

#include <gtk/gtk.h>

/* 
 * Build a GtkMenu from a hierarchical string.
 * Example format:
 * "Settings;;Dark Mode|/bin/toggle_dark|check,Auto-Refresh|/bin/toggle_ref|check"
 */
GtkWidget* yad_build_global_menu(const gchar *def);

#endif /* MENU_H */
