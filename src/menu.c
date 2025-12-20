#include "menu.h"
#include "yad.h"
#include <string.h>
#include <ctype.h>

/* ===========================
   DATA STRUCTURES
   =========================== */

typedef struct _MenuItem {
    gchar *label;
    gchar *command;
    gchar *icon_name;

    gboolean is_submenu;
    gboolean is_check;
    gboolean checked;
    gboolean is_separator;

    GList *submenu_items;
    GtkWidget *indicator_image; 
} MenuItem;

typedef struct _RootMenu {
    gchar *label;
    GList *items;
} RootMenu;

static GList *root_menus = NULL;
static GList *menu_stack = NULL;

/* ===========================
   MEMORY CLEANUP
   =========================== */

static void free_menu_item(gpointer data) {
    MenuItem *item = (MenuItem *)data;
    if (!item) return;
    g_free(item->label);
    g_free(item->command);
    g_free(item->icon_name);
    if (item->submenu_items) {
        g_list_free_full(item->submenu_items, free_menu_item);
    }
    g_free(item);
}

static void free_root_menu(gpointer data) {
    RootMenu *root = (RootMenu *)data;
    if (!root) return;
    g_free(root->label);
    if (root->items) {
        g_list_free_full(root->items, free_menu_item);
    }
    g_free(root);
}

/* ===========================
   MNEMONIC SAFETY LOGIC
   =========================== */

static gchar* validate_mnemonic(const gchar *label, gboolean *used_list) {
    if (!label) return NULL;
    
    gchar *new_label = g_strdup(label);
    gchar *p = strchr(new_label, '_');
    
    while (p) {
        unsigned char c = (unsigned char)p[1];
        if (c != '\0' && c != '_') {
            int index = toupper(c);
            if (used_list[index]) {
                memmove(p, p + 1, strlen(p));
                p = strchr(p, '_'); 
            } else {
                used_list[index] = TRUE;
                break;
            }
        } else {
            p = strchr(p + 2, '_');
        }
    }
    return new_label;
}

/* ===========================
   CSS & STYLING
   =========================== */

static void load_menu_css(void) {
    GtkCssProvider *provider = gtk_css_provider_new();
    const gchar *css =
        ".yad-menu-box { margin-left: -24px; padding: 0; }\n"
        ".yad-menu-box label { padding: 0; margin: 0; }\n"
        "menuitem { padding-left: 0; }\n";
    
    gtk_css_provider_load_from_data(provider, css, -1, NULL);
    gtk_style_context_add_provider_for_screen(gdk_screen_get_default(),
        GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    g_object_unref(provider);
}

/* ===========================
   FORGIVING PARSER
   =========================== */

static void add_parsed_item(gchar **parts, gint n, RootMenu *current_root, GList **stack, gboolean *used_mnemonics) {
    MenuItem *item = g_new0(MenuItem, 1);
    
    // Label is mandatory. Empty or "-" = separator.
    gchar *raw_label = (n > 0) ? g_strstrip(g_strdup(parts[0])) : g_strdup("");
    
    if (!*raw_label || g_strcmp0(raw_label, "-") == 0) {
        item->is_separator = TRUE;
        g_free(raw_label);
    } else {
        item->label = validate_mnemonic(raw_label, used_mnemonics);
        g_free(raw_label);

        // Command handling: if n > 1 and not empty, it's an action.
        if (n > 1 && parts[1] && *parts[1]) {
            item->command = g_strdup(parts[1]);
            item->is_submenu = FALSE;
        } else {
            item->is_submenu = TRUE; 
        }

        // Icon/Type handling
        if (n > 2 && parts[2] && *parts[2]) {
            if (g_ascii_strcasecmp(parts[2], "check") == 0) {
                item->is_check = TRUE;
                item->is_submenu = FALSE;
            } else {
                item->icon_name = g_strdup(parts[2]);
            }
        }
    }

    GList **target = (*stack) ? &((MenuItem *)(*stack)->data)->submenu_items : &current_root->items;
    *target = g_list_append(*target, item);

    if (item->is_submenu && !item->command) {
        *stack = g_list_prepend(*stack, item);
    }
}

static void parse_menu_string(const gchar *def) {
    if (!def) return;
    gchar **tokens = g_strsplit(def, ",", 0);
    RootMenu *current_root = NULL;
    gboolean root_mnemonics[256] = { FALSE };
    gboolean level_mnemonics[256] = { FALSE };

    for (int i = 0; tokens[i]; i++) {
        gchar *tok = g_strstrip(g_strdup(tokens[i]));
        if (!*tok) { 
            if (menu_stack) {
                GList *next = menu_stack->next;
                g_list_free_1(menu_stack);
                menu_stack = next;
            }
            memset(level_mnemonics, 0, sizeof(level_mnemonics));
            g_free(tok);
            continue;
        }

        if (g_strrstr(tok, ";;")) { 
            gchar *p = g_strrstr(tok, ";;");
            gchar *remainder = p + 2;
            *p = 0;
            gchar *root_label = g_strstrip(tok);
            if (*root_label) {
                current_root = g_new0(RootMenu, 1);
                current_root->label = validate_mnemonic(root_label, root_mnemonics);
                root_menus = g_list_append(root_menus, current_root);
                menu_stack = NULL;
                memset(level_mnemonics, 0, sizeof(level_mnemonics));

                if (*remainder) {
                    gchar **parts = g_strsplit(remainder, "|", 3);
                    add_parsed_item(parts, g_strv_length(parts), current_root, &menu_stack, level_mnemonics);
                    g_strfreev(parts);
                }
            }
        } else if (current_root) { 
            gchar **parts = g_strsplit(tok, "|", 3);
            add_parsed_item(parts, g_strv_length(parts), current_root, &menu_stack, level_mnemonics);
            g_strfreev(parts);
        }
        g_free(tok);
    }
    g_strfreev(tokens);
}

/* ===========================
   UI GENERATION
   =========================== */

static void mb_activate(GtkWidget *w, gpointer data) {
    MenuItem *item = (MenuItem *)data;
    if (!item || !item->command) return;

    // 1. Handle Checkboxes
    if (item->is_check) {
        item->checked = !item->checked;
        if (item->indicator_image) {
            if (item->checked)
                gtk_image_set_from_icon_name(GTK_IMAGE(item->indicator_image), "check-symbolic", GTK_ICON_SIZE_MENU);
            else
                gtk_image_clear(GTK_IMAGE(item->indicator_image));
        }
    }

    // 2. Handle "Quit" or "Exit" internal commands
    if (g_ascii_strcasecmp(item->command, "quit") == 0 || 
        g_ascii_strcasecmp(item->command, "exit") == 0) {
        gtk_main_quit();
        return;
    }

    // 3. Export YAD_PID to the environment for shell scripts
    gchar *pid_str = g_strdup_printf("%d", getpid());
    g_setenv("YAD_PID", pid_str, TRUE);

    // 4. Run the command
    run_command_async(item->command);
    g_free(pid_str);
}

static void populate_yad_menu(GtkWidget *menu, GList *items) {
    for (GList *l = items; l; l = l->next) {
        MenuItem *item = (MenuItem *)l->data;
        GtkWidget *mi;

        if (item->is_separator) {
            mi = gtk_separator_menu_item_new();
        } else {
            mi = gtk_menu_item_new_with_mnemonic(item->label);
            GtkWidget *box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
            gtk_style_context_add_class(gtk_widget_get_style_context(box), "yad-menu-box");
            gtk_widget_set_halign(box, GTK_ALIGN_START);

            GtkWidget *ind = gtk_image_new();
            GtkWidget *ico = gtk_image_new();
            GtkWidget *lbl = gtk_bin_get_child(GTK_BIN(mi)); 
            
            if (lbl) {
                g_object_ref(lbl);
                gtk_container_remove(GTK_CONTAINER(mi), lbl);
                gtk_label_set_xalign(GTK_LABEL(lbl), 0.0);
            }

            gtk_widget_set_size_request(ind, 16, 16);
            gtk_widget_set_size_request(ico, 16, 16);

            item->indicator_image = ind;
            if (item->is_check && item->checked)
                gtk_image_set_from_icon_name(GTK_IMAGE(ind), "check-symbolic", GTK_ICON_SIZE_MENU);
            
            if (item->icon_name) {
                GdkPixbuf *pb = get_pixbuf(item->icon_name, YAD_SMALL_ICON, TRUE);
                if (pb) {
                    gtk_image_set_from_pixbuf(GTK_IMAGE(ico), pb);
                    g_object_unref(pb);
                }
            }

            gtk_box_pack_start(GTK_BOX(box), ind, FALSE, FALSE, 0);
            gtk_box_pack_start(GTK_BOX(box), ico, FALSE, FALSE, 0);
            gtk_box_pack_start(GTK_BOX(box), lbl, TRUE, TRUE, 0);
            
            gtk_container_add(GTK_CONTAINER(mi), box);
            g_object_unref(lbl);

            if (item->submenu_items) {
                GtkWidget *sub = gtk_menu_new();
                populate_yad_menu(sub, item->submenu_items);
                gtk_menu_item_set_submenu(GTK_MENU_ITEM(mi), sub);
            } else {
                g_signal_connect(mi, "activate", G_CALLBACK(mb_activate), item);
            }
        }
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), mi);
    }
}

GtkWidget* yad_build_global_menu(const gchar *def) {
    if (!def) return NULL;
    if (root_menus) {
        g_list_free_full(root_menus, free_root_menu);
        root_menus = NULL;
    }

    load_menu_css();
    parse_menu_string(def);

    GtkWidget *menubar = gtk_menu_bar_new();
    for (GList *l = root_menus; l; l = l->next) {
        RootMenu *root = (RootMenu *)l->data;
        GtkWidget *root_mi = gtk_menu_item_new_with_mnemonic(root->label);
        GtkWidget *sub = gtk_menu_new();
        populate_yad_menu(sub, root->items);
        gtk_menu_item_set_submenu(GTK_MENU_ITEM(root_mi), sub);
        gtk_menu_shell_append(GTK_MENU_SHELL(menubar), root_mi);
    }
    gtk_widget_show_all(menubar);
    return menubar;
}