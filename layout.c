#include "layout.h"
#include <stdio.h>
#include <string.h>

// ========================================
// CONFIG LOADING
// ========================================

LayoutConfig* layout_load_config(void) {
    LayoutConfig *config = g_new0(LayoutConfig, 1);

    gchar *config_dir = g_build_filename(g_get_user_config_dir(), "hyprwave", NULL);
    gchar *config_file = g_build_filename(config_dir, "config.conf", NULL);

    // Create config directory if it doesn't exist
    g_mkdir_with_parents(config_dir, 0755);

    // Check if config file exists, create default if not
    if (!g_file_test(config_file, G_FILE_TEST_EXISTS)) {
        gchar *default_config =
            "# HyprWave Configuration File\n"
            "\n"
            "[General]\n"
            "# Edge to anchor HyprWave to\n"
            "# Options: right, left, top, bottom\n"
            "edge = right\n"
            "\n"
            "# Margin from the screen edge (in pixels)\n"
            "margin = 10\n"
            "\n"
            "# Theme: light or dark\n"
            "theme = light\n"
            "\n"
            "# Size: tiny, small, default, large\n"
            "size = default\n"
            "\n"
            "[MusicPlayer]\n"
            "# Comma-separated list of preferred music players (first = highest priority)\n"
            "# HyprWave will search for these in order and latch onto the first one found\n"
            "# Leave empty to connect to any available player\n"
            "# Common names: spotify, vlc, firefox, chromium, mpd, rhythmbox, strawberry\n"
            "preference = spotify,vlc\n"
            "\n"
            "[Keybinds]\n"
            "# Toggle HyprWave visibility (hide/show entire window)\n"
            "toggle_visibility = Super+Shift+M\n"
            "\n"
            "# Toggle expanded section (show/hide album details)\n"
            "toggle_expand = Super+M\n"
            "\n"
            "[Notifications]\n"
            "# Enable/disable notifications\n"
            "enabled = true\n"
            "\n"
            "# Show notification when song changes\n"
            "now_playing = true\n"
            "\n"
            "[Visualizer]\n"
            "# Enable/disable visualizer (horizontal layout only)\n"
            "enabled = true\n"
            "\n"
            "# Idle timeout in seconds before visualizer appears\n"
            "# Set to 0 to disable auto-activation (visualizer only shows on demand)\n"
            "idle_timeout = 30\n"
            "\n"
            "[VerticalDisplay]\n"
            "# Enable/disable vertical display (vertical layout only)\n"
            "enabled = true\n"
            "\n"
            "# Idle timeout in seconds before vertical display appears\n"
            "# Set to 0 to disable auto-activation (display only shows on demand)\n"
            "idle_timeout = 5\n";

        g_file_set_contents(config_file, default_config, -1, NULL);
        g_print("Created default config at: %s\n", config_file);
    }

    // Load config
    GKeyFile *keyfile = g_key_file_new();

    // Set defaults
    config->edge = EDGE_RIGHT;
    config->margin = 10;
    config->toggle_visibility_bind = g_strdup("Super+Shift+M");
    config->toggle_expand_bind = g_strdup("Super+M");
    config->notifications_enabled = TRUE;
    config->now_playing_enabled = TRUE;
    config->theme = g_strdup("light");
    config->button_size = 70;
    config->size_name = g_strdup("default");
    config->visualizer_enabled = TRUE;
    config->visualizer_idle_timeout = 30;
    config->vertical_display_enabled = TRUE;
    config->vertical_display_scroll_interval = 5;
    config->player_preference = NULL;
    config->player_preference_count = 0;


    if (g_key_file_load_from_file(keyfile, config_file, G_KEY_FILE_NONE, NULL)) {
        // Load General section
        gchar *edge_str = g_key_file_get_string(keyfile, "General", "edge", NULL);

        if (edge_str) {
            if (g_strcmp0(edge_str, "left") == 0) {
                config->edge = EDGE_LEFT;
            } else if (g_strcmp0(edge_str, "top") == 0) {
                config->edge = EDGE_TOP;
            } else if (g_strcmp0(edge_str, "bottom") == 0) {
                config->edge = EDGE_BOTTOM;
            } else {
                config->edge = EDGE_RIGHT;
            }
            g_free(edge_str);
        }

        config->margin = g_key_file_get_integer(keyfile, "General", "margin", NULL);
        if (config->margin == 0) config->margin = 10;

        // Load theme
        gchar *theme_str = g_key_file_get_string(keyfile, "General", "theme", NULL);
        if (theme_str) {
            g_free(config->theme);
            config->theme = theme_str;
        }

        // Load size: tiny | small | default | large
        // Drives button/icon/spacing/padding scaling across the control bar
        gchar *size_str = g_key_file_get_string(keyfile, "General", "size", NULL);
        if (size_str) {
            if (g_strcmp0(size_str, "tiny") == 0) {
                config->button_size = 35;
            } else if (g_strcmp0(size_str, "small") == 0) {
                config->button_size = 52;
            } else if (g_strcmp0(size_str, "default") == 0) {
                config->button_size = 70;
            } else if (g_strcmp0(size_str, "large") == 0) {
                config->button_size = 93;
            } else {
                g_warning("Invalid size '%s' in config (expected tiny|small|default|large) — using default", size_str);
                g_free(size_str);
                size_str = g_strdup("default");
                config->button_size = 70;
            }
            g_free(config->size_name);
            config->size_name = size_str;
        }

        // Load Keybinds section (optional)
        gchar *vis_bind = g_key_file_get_string(keyfile, "Keybinds", "toggle_visibility", NULL);
        if (vis_bind) {
            g_free(config->toggle_visibility_bind);
            config->toggle_visibility_bind = vis_bind;
        }

        gchar *exp_bind = g_key_file_get_string(keyfile, "Keybinds", "toggle_expand", NULL);
        if (exp_bind) {
            g_free(config->toggle_expand_bind);
            config->toggle_expand_bind = exp_bind;
        }

        // Load Notifications section (optional)
        GError *error = NULL;
        gboolean notif_enabled = g_key_file_get_boolean(keyfile, "Notifications", "enabled", &error);
        if (!error) {
            config->notifications_enabled = notif_enabled;
        } else {
            g_error_free(error);
            error = NULL;
        }

        gboolean now_playing = g_key_file_get_boolean(keyfile, "Notifications", "now_playing", &error);
        if (!error) {
            config->now_playing_enabled = now_playing;
        } else {
            g_error_free(error);
            error = NULL;
        }

        // Load Visualizer section (optional)
        gboolean viz_enabled = g_key_file_get_boolean(keyfile, "Visualizer", "enabled", &error);
        if (!error) {
            config->visualizer_enabled = viz_enabled;
        } else {
            g_error_free(error);
            error = NULL;
        }

        gint viz_timeout = g_key_file_get_integer(keyfile, "Visualizer", "idle_timeout", &error);
        if (!error) {
            config->visualizer_idle_timeout = viz_timeout;
            if (config->visualizer_idle_timeout < 0) config->visualizer_idle_timeout = 0;
        } else {
            g_error_free(error);
        }
    
    
        gboolean vert_enabled = g_key_file_get_boolean(keyfile, "VerticalDisplay", "enabled", &error);
        if (!error) {
            config->vertical_display_enabled = vert_enabled;
        } else {
            g_error_free(error);
            error = NULL;
        }
        
        gint vert_timeout = g_key_file_get_integer(keyfile, "VerticalDisplay", "idle_timeout", &error);
        if (!error) {
            config->vertical_display_scroll_interval = vert_timeout;
            if (config->vertical_display_scroll_interval < 0) config->vertical_display_scroll_interval = 0;
        } else {
            g_error_free(error);
        }
        
        // MusicPlayer section: preference config removed in favor of file-based persistence
        // Last used player is saved to ~/.config/hyprwave/preferred_player
    }
    config->is_vertical = (config->edge == EDGE_RIGHT || config->edge == EDGE_LEFT);

    g_key_file_free(keyfile);
    g_free(config_file);
    g_free(config_dir);

    g_print("Layout: %s edge (%s), theme: %s\n",
            config->edge == EDGE_RIGHT ? "right" :
            config->edge == EDGE_LEFT ? "left" :
            config->edge == EDGE_TOP ? "top" : "bottom",
            config->is_vertical ? "vertical" : "horizontal",
            config->theme);

    return config;
}

void layout_free_config(LayoutConfig *config) {
    if (config) {
        g_free(config->toggle_visibility_bind);
        g_free(config->toggle_expand_bind);
        g_free(config->theme);
        g_free(config->size_name);
        if (config->player_preference) {
            g_strfreev(config->player_preference);
        }
        g_free(config);
    }
}

// ========================================
// WINDOW SETUP
// ========================================

void layout_setup_window_anchors(GtkWindow *window, LayoutConfig *config) {
    gboolean right = config->edge == EDGE_RIGHT;
    gboolean left = config->edge == EDGE_LEFT;
    gboolean top = config->edge == EDGE_TOP;
    gboolean bottom = config->edge == EDGE_BOTTOM;

    if (config->is_vertical) {
        top = TRUE;
        bottom = TRUE;
    } else {
        left = TRUE;
        right = TRUE;
    }

    // Keep the layer surface stable on the perpendicular axis while revealers resize.
    gtk_layer_set_anchor(window, GTK_LAYER_SHELL_EDGE_RIGHT, right);
    gtk_layer_set_anchor(window, GTK_LAYER_SHELL_EDGE_LEFT, left);
    gtk_layer_set_anchor(window, GTK_LAYER_SHELL_EDGE_TOP, top);
    gtk_layer_set_anchor(window, GTK_LAYER_SHELL_EDGE_BOTTOM, bottom);

    // Set margin
    if (config->edge == EDGE_RIGHT) {
        gtk_layer_set_margin(window, GTK_LAYER_SHELL_EDGE_RIGHT, config->margin);
    } else if (config->edge == EDGE_LEFT) {
        gtk_layer_set_margin(window, GTK_LAYER_SHELL_EDGE_LEFT, config->margin);
    } else if (config->edge == EDGE_TOP) {
        gtk_layer_set_margin(window, GTK_LAYER_SHELL_EDGE_TOP, config->margin);
    } else {
        gtk_layer_set_margin(window, GTK_LAYER_SHELL_EDGE_BOTTOM, config->margin);
    }
}

// ========================================
// CONTROL BAR
// ========================================

gint layout_get_control_padding(LayoutConfig *config) {
    if (!config || !config->size_name) return 12;

    if (g_strcmp0(config->size_name, "tiny") == 0) return 6;
    if (g_strcmp0(config->size_name, "small") == 0) return 9;
    if (g_strcmp0(config->size_name, "large") == 0) return 16;
    return 12;
}

gint layout_get_control_spacing(LayoutConfig *config) {
    if (!config) return 8;
    gint spacing = (gint)(config->button_size * 0.11);
    return spacing > 0 ? spacing : 1;
}

gint layout_get_control_button_size(LayoutConfig *config) {
    if (!config) return 46;
    gint button_size = config->button_size - (2 * layout_get_control_padding(config));
    return button_size > 16 ? button_size : 16;
}

gint layout_get_control_bar_length(LayoutConfig *config) {
    gint button_size = layout_get_control_button_size(config);
    gint spacing = layout_get_control_spacing(config);
    gint padding = layout_get_control_padding(config);
    return (4 * button_size) + (3 * spacing) + (2 * padding);
}

GtkWidget* layout_create_control_bar(LayoutConfig *config,
                                      GtkWidget **prev_btn,
                                      GtkWidget **play_btn,
                                      GtkWidget **next_btn,
                                      GtkWidget **expand_btn) {
    GtkOrientation orientation = config->is_vertical ? GTK_ORIENTATION_VERTICAL : GTK_ORIENTATION_HORIZONTAL;
    int spacing = layout_get_control_spacing(config);
    int bar_length = layout_get_control_bar_length(config);
    GtkWidget *control_bar = gtk_box_new(orientation, spacing);

    gtk_widget_add_css_class(control_bar, config->is_vertical ? "control-container" : "control-container-horizontal");

    // Tag the bar with its size class so style.css can pick the right padding
    gchar *size_class = g_strdup_printf("size-%s", config->size_name ? config->size_name : "default");
    gtk_widget_add_css_class(control_bar, size_class);
    g_free(size_class);

    GtkAlign halign = GTK_ALIGN_CENTER;
    GtkAlign valign = GTK_ALIGN_CENTER;
    if (config->is_vertical) {
        halign = config->edge == EDGE_RIGHT ? GTK_ALIGN_END : GTK_ALIGN_START;
    } else {
        valign = config->edge == EDGE_BOTTOM ? GTK_ALIGN_END : GTK_ALIGN_START;
    }
    gtk_widget_set_halign(control_bar, halign);
    gtk_widget_set_valign(control_bar, valign);
    gtk_widget_set_hexpand(control_bar, FALSE);
    gtk_widget_set_vexpand(control_bar, FALSE);

    if (config->is_vertical) {
        gtk_widget_set_size_request(control_bar, config->button_size, bar_length);
    } else {
        gtk_widget_set_size_request(control_bar, bar_length, config->button_size);
    }

    // Create buttons (widgets created externally, we just arrange them)
    gtk_box_append(GTK_BOX(control_bar), *prev_btn);
    gtk_box_append(GTK_BOX(control_bar), *play_btn);
    gtk_box_append(GTK_BOX(control_bar), *next_btn);
    gtk_box_append(GTK_BOX(control_bar), *expand_btn);

    return control_bar;
}

// ========================================
// EXPANDED SECTION
// ========================================

GtkWidget* layout_create_expanded_section(LayoutConfig *config, ExpandedWidgets *widgets) {
    GtkWidget *expanded_section;

    // Create visualizer container (empty box - bars added by main.c)
    widgets->visualizer_box = gtk_box_new(
        config->is_vertical ? GTK_ORIENTATION_VERTICAL : GTK_ORIENTATION_HORIZONTAL, 1);
    gtk_widget_add_css_class(widgets->visualizer_box, "visualizer-container");
    gtk_widget_set_halign(widgets->visualizer_box, GTK_ALIGN_CENTER);

    if (config->is_vertical) {
        // Vertical layout: stack everything vertically
        expanded_section = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
        gtk_widget_add_css_class(expanded_section, "expanded-section");
        gtk_widget_set_halign(expanded_section, GTK_ALIGN_CENTER);
        gtk_widget_set_valign(expanded_section, GTK_ALIGN_CENTER);

        // Album cover first
        gtk_box_append(GTK_BOX(expanded_section), widgets->album_cover);

        // Visualizer below album art with fixed size
        gtk_widget_set_size_request(widgets->visualizer_box, 300, 40);
        gtk_widget_set_vexpand(widgets->visualizer_box, FALSE);
        gtk_box_append(GTK_BOX(expanded_section), widgets->visualizer_box);

        gtk_box_append(GTK_BOX(expanded_section), widgets->player_label);
        gtk_box_append(GTK_BOX(expanded_section), widgets->track_title);
        gtk_box_append(GTK_BOX(expanded_section), widgets->artist_label);
        gtk_box_append(GTK_BOX(expanded_section), widgets->progress_bar);
        gtk_box_append(GTK_BOX(expanded_section), widgets->time_remaining);

    } else {
        // Horizontal layout: album+visualizer on left, info on right
        expanded_section = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
        gtk_widget_add_css_class(expanded_section, "expanded-section-horizontal");
        gtk_widget_set_halign(expanded_section, GTK_ALIGN_CENTER);
        gtk_widget_set_valign(expanded_section, GTK_ALIGN_CENTER);

        // Left column: album cover + visualizer below it
        GtkWidget *left_column = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
        gtk_widget_set_valign(left_column, GTK_ALIGN_CENTER);
        gtk_widget_set_halign(left_column, GTK_ALIGN_CENTER);

        // Info panel (right side)
        GtkWidget *info_panel = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
        gtk_widget_set_valign(info_panel, GTK_ALIGN_CENTER);

        gtk_label_set_xalign(GTK_LABEL(widgets->player_label), 0.0);
        gtk_label_set_xalign(GTK_LABEL(widgets->track_title), 0.0);
        gtk_label_set_xalign(GTK_LABEL(widgets->artist_label), 0.0);
        gtk_label_set_xalign(GTK_LABEL(widgets->time_remaining), 0.0);

        // Increase max width for horizontal layout
        gtk_label_set_max_width_chars(GTK_LABEL(widgets->track_title), 25);
        gtk_label_set_max_width_chars(GTK_LABEL(widgets->artist_label), 25);
        gtk_widget_set_size_request(widgets->progress_bar, 180, 16);  // Taller for easier clicking

        gtk_box_append(GTK_BOX(info_panel), widgets->player_label);
        gtk_box_append(GTK_BOX(info_panel), widgets->track_title);
        gtk_box_append(GTK_BOX(info_panel), widgets->artist_label);
        gtk_box_append(GTK_BOX(info_panel), widgets->progress_bar);
        gtk_box_append(GTK_BOX(info_panel), widgets->time_remaining);

        // Visualizer below album art with fixed height
        gtk_widget_set_size_request(widgets->visualizer_box, 300, 40);
        gtk_widget_set_vexpand(widgets->visualizer_box, FALSE);

        // Build left column: album cover on top, visualizer below
        gtk_box_append(GTK_BOX(left_column), widgets->album_cover);
        gtk_box_append(GTK_BOX(left_column), widgets->visualizer_box);

        gtk_box_append(GTK_BOX(expanded_section), left_column);
        gtk_box_append(GTK_BOX(expanded_section), info_panel);
    }

    return expanded_section;
}

// ========================================
// MAIN CONTAINER
// ========================================

GtkWidget* layout_create_main_container(LayoutConfig *config,
                                         GtkWidget *control_bar,
                                         GtkWidget *revealer) {
    GtkOrientation orientation = config->is_vertical ? GTK_ORIENTATION_HORIZONTAL : GTK_ORIENTATION_VERTICAL;
    GtkWidget *main_container = gtk_box_new(orientation, 0);

    gtk_widget_add_css_class(main_container, "main-container");
    gtk_widget_set_hexpand(main_container, FALSE);
    gtk_widget_set_vexpand(main_container, FALSE);
    if (config->is_vertical) {
        gtk_widget_set_halign(main_container, config->edge == EDGE_RIGHT ? GTK_ALIGN_END : GTK_ALIGN_START);
        gtk_widget_set_valign(main_container, GTK_ALIGN_CENTER);
    } else {
        gtk_widget_set_halign(main_container, GTK_ALIGN_CENTER);
        gtk_widget_set_valign(main_container, config->edge == EDGE_BOTTOM ? GTK_ALIGN_END : GTK_ALIGN_START);
    }

    if (config->is_vertical) {
        // Vertical: control bar on edge, revealer slides outward
        gtk_box_append(GTK_BOX(main_container), control_bar);
        gtk_box_append(GTK_BOX(main_container), revealer);
    } else {
        // Horizontal: order depends on top or bottom
        if (config->edge == EDGE_TOP) {
            gtk_box_append(GTK_BOX(main_container), control_bar);
            gtk_box_append(GTK_BOX(main_container), revealer);
        } else {
            gtk_box_append(GTK_BOX(main_container), revealer);
            gtk_box_append(GTK_BOX(main_container), control_bar);
        }
    }

    return main_container;
}

// ========================================
// HELPER FUNCTIONS
// ========================================

const gchar* layout_get_expand_icon(LayoutConfig *config, gboolean is_expanded) {
    if (config->is_vertical) {
        return is_expanded ? "arrow-right.svg" : "arrow-left.svg";
    } else {
        if (config->edge == EDGE_TOP) {
            return is_expanded ? "arrow-up.svg" : "arrow-down.svg";
        } else {
            return is_expanded ? "arrow-down.svg" : "arrow-up.svg";
        }
    }
}

GtkRevealerTransitionType layout_get_transition_type(LayoutConfig *config) {
    if (config->is_vertical) {
        return GTK_REVEALER_TRANSITION_TYPE_SLIDE_RIGHT;
    } else {
        if (config->edge == EDGE_TOP) {
            return GTK_REVEALER_TRANSITION_TYPE_SLIDE_DOWN;
        } else {
            return GTK_REVEALER_TRANSITION_TYPE_SLIDE_UP;
        }
    }
}
