#include "paths.h"
#include <stdio.h>

gchar* get_icon_path(const gchar *icon_name) {
    gchar *path = NULL;
    
    // Try 1: Local directory (for development - running ./hyprwave)
    path = g_strdup_printf("icons/%s", icon_name);
    if (g_file_test(path, G_FILE_TEST_EXISTS)) {
        g_print("Icon found (local): %s\n", path);
        return path;
    }
    g_free(path);
    
    // Try 2: User local install (~/.local/share/hyprwave/icons/)
    path = g_build_filename(g_get_user_data_dir(), "hyprwave", "icons", icon_name, NULL);
    if (g_file_test(path, G_FILE_TEST_EXISTS)) {
        g_print("Icon found (user): %s\n", path);
        return path;
    }
    g_free(path);
    
    // Try 3: System install (/usr/share/hyprwave/icons/)
    path = g_strdup_printf("/usr/share/hyprwave/icons/%s", icon_name);
    if (g_file_test(path, G_FILE_TEST_EXISTS)) {
        g_print("Icon found (system): %s\n", path);
        return path;
    }
    g_free(path);
    
    // Fallback: return local path anyway (will show broken icon)
    g_printerr("Warning: Icon not found: %s\n", icon_name);
    return g_strdup_printf("icons/%s", icon_name);
}

gchar* get_style_path(void) {
    gchar *path = NULL;
    
    // Try 1: Local directory
    path = g_strdup("style.css");
    if (g_file_test(path, G_FILE_TEST_EXISTS)) {
        g_print("CSS found (local): %s\n", path);
        return path;
    }
    g_free(path);
    
    // Try 2: User local install
    path = g_build_filename(g_get_user_data_dir(), "hyprwave", "style.css", NULL);
    if (g_file_test(path, G_FILE_TEST_EXISTS)) {
        g_print("CSS found (user): %s\n", path);
        return path;
    }
    g_print("Tried user path: %s (not found)\n", path);
    g_free(path);
    
    // Try 3: System install
    path = g_strdup("/usr/share/hyprwave/style.css");
    if (g_file_test(path, G_FILE_TEST_EXISTS)) {
        g_print("CSS found (system): %s\n", path);
        return path;
    }
    g_free(path);
    
    // Fallback
    g_printerr("Warning: style.css not found in any location\n");
    g_printerr("User data dir: %s\n", g_get_user_data_dir());
    return g_strdup("style.css");
}

gchar* get_theme_path(const gchar *theme) {
    if (!theme || g_strcmp0(theme, "light") == 0) {
        return NULL;  // Light theme uses base styles only
    }

    gchar *filename = g_strdup_printf("%s.css", theme);
    gchar *path = NULL;

    // Try 1: Local directory (for development)
    path = g_strdup_printf("themes/%s", filename);
    if (g_file_test(path, G_FILE_TEST_EXISTS)) {
        g_print("Theme found (local): %s\n", path);
        g_free(filename);
        return path;
    }
    g_free(path);

    // Try 2: User local install (~/.local/share/hyprwave/themes/)
    path = g_build_filename(g_get_user_data_dir(), "hyprwave", "themes", filename, NULL);
    if (g_file_test(path, G_FILE_TEST_EXISTS)) {
        g_print("Theme found (user): %s\n", path);
        g_free(filename);
        return path;
    }
    g_free(path);

    // Try 3: System install (/usr/share/hyprwave/themes/)
    path = g_strdup_printf("/usr/share/hyprwave/themes/%s", filename);
    if (g_file_test(path, G_FILE_TEST_EXISTS)) {
        g_print("Theme found (system): %s\n", path);
        g_free(filename);
        return path;
    }
    g_free(path);

    g_printerr("Warning: Theme '%s' not found\n", theme);
    g_free(filename);
    return NULL;
}

gchar* get_config_theme(void) {
    gchar *config_file = g_build_filename(g_get_user_config_dir(), "hyprwave", "config.conf", NULL);
    gchar *theme = NULL;

    GKeyFile *keyfile = g_key_file_new();
    if (g_key_file_load_from_file(keyfile, config_file, G_KEY_FILE_NONE, NULL)) {
        theme = g_key_file_get_string(keyfile, "General", "theme", NULL);
    }
    g_key_file_free(keyfile);
    g_free(config_file);

    return theme ? theme : g_strdup("light");
}

void free_path(gchar *path) {
    g_free(path);
}
