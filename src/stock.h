
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
 * Copyright (C) 2008-2019, Victor Ananjevsky <ananasik@gmail.com>
 */

#ifndef _STOCK_H_
#define _STOCK_H_

#define YAD_STOCK_COUNT 19

typedef struct {
  gchar *key;
  gchar *label;
  gchar *icon;
} YadStock;

YadStock yad_stock_items[] = {
  { "yad-about", N_("About"), "help-about" },
  { "yad-add",  N_("Add"), "list-add" },
  { "yad-apply",  N_("Apply"), "gtk-apply" },
  { "yad-cancel",  N_("Cancel"), "gtk-cancel" },
  { "yad-clear",  N_("Clear"), "document-clear" },
  { "yad-close",  N_("Close"), "window-close" },
  { "yad-edit",  N_("Edit"), "gtk-edit" },
  { "yad-execute",  N_("Execute"), "system-run" },
  { "yad-no",  N_("No"), "gtk-no" },
  { "yad-ok",  N_("OK"), "gtk-ok" },
  { "yad-open",  N_("Open"), "document-open" },
  { "yad-print",  N_("Print"), "document-print" },
  { "yad-quit",  N_("Quit"), "application-exit" },
  { "yad-refresh",  N_("Refresh"), "view-refresh" },
  { "yad-remove",  N_("Remove"), "list-remove" },
  { "yad-save",  N_("Save"), "document-save" },
  { "yad-search", N_("Search"), "system-search" },
  { "yad-settings",  N_("Settings"), "gtk-preferences" },
  { "yad-yes",  N_("Yes"), "gtk-yes" }
};

#endif /* _STOCK_H_ */
