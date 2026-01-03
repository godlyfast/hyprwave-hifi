#include <gtk/gtk.h>
#include <gtk4-layer-shell.h>
#include <gio/gio.h>
#include <stdio.h>
#include <string.h>
#include "layout.h"

typedef struct {
    GtkWidget *window;
    GtkWidget *revealer;
    GtkWidget *play_icon;
    GtkWidget *expand_icon;
    GtkWidget *album_cover;
    GtkWidget *source_label;
    GtkWidget *track_title;
    GtkWidget *artist_label;
    GtkWidget *time_remaining;
    GtkWidget *progress_bar;
    gboolean is_playing;
    gboolean is_expanded;
    GDBusProxy *mpris_proxy;
    gchar *current_player;
    guint update_timer;
    LayoutConfig *layout;
} AppState;

static void update_position(AppState *state);
static void update_metadata(AppState *state);

// ========================================
// HELPER FUNCTIONS
// ========================================

static gint64 get_variant_as_int64(GVariant *value) {
    if (value == NULL) return 0;
    if (g_variant_is_of_type(value, G_VARIANT_TYPE_INT64)) {
        return g_variant_get_int64(value);
    } 
    else if (g_variant_is_of_type(value, G_VARIANT_TYPE_UINT64)) {
        return (gint64)g_variant_get_uint64(value);
    }
    else if (g_variant_is_of_type(value, G_VARIANT_TYPE_INT32)) {
        return (gint64)g_variant_get_int32(value);
    }
    else if (g_variant_is_of_type(value, G_VARIANT_TYPE_UINT32)) {
        return (gint64)g_variant_get_uint32(value);
    }
    else if (g_variant_is_of_type(value, G_VARIANT_TYPE_DOUBLE)) {
        return (gint64)g_variant_get_double(value);
    }
    return 0;
}

static gboolean update_position_tick(gpointer user_data) {
    AppState *state = (AppState *)user_data;
    update_position(state);
    return G_SOURCE_CONTINUE;
}

static void update_position(AppState *state) {
    if (!state->mpris_proxy) return;
    
    GError *error = NULL;
    GVariant *position_container = g_dbus_proxy_call_sync(
        state->mpris_proxy,
        "org.freedesktop.DBus.Properties.Get",
        g_variant_new("(ss)", "org.mpris.MediaPlayer2.Player", "Position"),
        G_DBUS_CALL_FLAGS_NONE,
        -1, NULL, &error
    );
    
    if (error) {
        g_error_free(error);
        return;
    }
    
    GVariant *position_val_wrapped;
    g_variant_get(position_container, "(v)", &position_val_wrapped);
    gint64 position = get_variant_as_int64(position_val_wrapped);
    g_variant_unref(position_val_wrapped);
    g_variant_unref(position_container);
    
    gint64 length = 0;
    GVariant *metadata_var = g_dbus_proxy_get_cached_property(state->mpris_proxy, "Metadata");
    
    if (metadata_var) {
        GVariantIter iter;
        gchar *key;
        GVariant *val;
        
        g_variant_iter_init(&iter, metadata_var);
        while (g_variant_iter_loop(&iter, "{sv}", &key, &val)) {
            if (g_strcmp0(key, "mpris:length") == 0) {
                length = get_variant_as_int64(val);
                break;
            }
        }
        g_variant_unref(metadata_var);
    }

    char time_str[32];
    double fraction = 0.0;
    gint64 pos_seconds = position / 1000000;

    if (length > 0) {
        gint64 len_seconds = length / 1000000;
        gint64 rem_seconds = len_seconds - pos_seconds;
        if (rem_seconds < 0) rem_seconds = 0;
        int mins = rem_seconds / 60;
        int secs = rem_seconds % 60;
        snprintf(time_str, sizeof(time_str), "-%d:%02d", mins, secs);
        fraction = (double)position / (double)length;
    } else {
        int mins = pos_seconds / 60;
        int secs = pos_seconds % 60;
        snprintf(time_str, sizeof(time_str), "%d:%02d", mins, secs);
        fraction = 0.0;
    }

    if (fraction > 1.0) fraction = 1.0;
    if (fraction < 0.0) fraction = 0.0;

    gtk_label_set_text(GTK_LABEL(state->time_remaining), time_str);
    gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(state->progress_bar), fraction);
}

static void update_metadata(AppState *state) {
    if (!state->mpris_proxy) return;
    
    GVariant *metadata = g_dbus_proxy_get_cached_property(state->mpris_proxy, "Metadata");
    if (!metadata) return;
    
    GVariantIter iter;
    GVariant *value;
    gchar *key;
    
    const gchar *title = NULL;
    const gchar *artist = NULL;
    const gchar *art_url = NULL;
    
    g_variant_iter_init(&iter, metadata);
    while (g_variant_iter_loop(&iter, "{sv}", &key, &value)) {
        if (g_strcmp0(key, "xesam:title") == 0) {
            title = g_variant_get_string(value, NULL);
        }
        else if (g_strcmp0(key, "xesam:artist") == 0) {
            if (g_variant_is_of_type(value, G_VARIANT_TYPE_STRING_ARRAY)) {
                gsize length;
                const gchar **artists = g_variant_get_strv(value, &length);
                if (length > 0) artist = artists[0];
            }
        }
        else if (g_strcmp0(key, "mpris:artUrl") == 0) {
            art_url = g_variant_get_string(value, NULL);
        }
    }
    
    if (title && strlen(title) > 0) {
        gtk_label_set_text(GTK_LABEL(state->track_title), title);
    } else {
        gtk_label_set_text(GTK_LABEL(state->track_title), "No Track Playing");
    }
    
    if (artist && strlen(artist) > 0) {
        gtk_label_set_text(GTK_LABEL(state->artist_label), artist);
    } else {
        const gchar *current = gtk_label_get_text(GTK_LABEL(state->artist_label));
        if (g_strcmp0(current, "Unknown Artist") == 0 || strlen(current) == 0) {
            gtk_label_set_text(GTK_LABEL(state->artist_label), "Unknown Artist");
        }
    }
    
    // Handle album art
    if (art_url && strlen(art_url) > 0) {
        GdkPixbuf *pixbuf = NULL;
        
        if (g_str_has_prefix(art_url, "file://")) {
            gchar *file_path = g_filename_from_uri(art_url, NULL, NULL);
            if (file_path && g_file_test(file_path, G_FILE_TEST_EXISTS)) {
                GError *error = NULL;
                pixbuf = gdk_pixbuf_new_from_file_at_scale(file_path, 120, 120, FALSE, &error);
                if (error) g_error_free(error);
            }
            g_free(file_path);
        } else if (g_str_has_prefix(art_url, "http://") || g_str_has_prefix(art_url, "https://")) {
            GFile *file = g_file_new_for_uri(art_url);
            GError *error = NULL;
            GInputStream *stream = G_INPUT_STREAM(g_file_read(file, NULL, &error));
            if (stream && !error) {
                pixbuf = gdk_pixbuf_new_from_stream_at_scale(stream, 120, 120, FALSE, NULL, &error);
                if (error) g_error_free(error);
                g_object_unref(stream);
            } else if (error) {
                g_error_free(error);
            }
            g_object_unref(file);
        }
        
        if (pixbuf) {
            GdkTexture *texture = gdk_texture_new_for_pixbuf(pixbuf);
            GtkWidget *image = gtk_picture_new_for_paintable(GDK_PAINTABLE(texture));
            gtk_widget_set_size_request(image, 120, 120);
            
            GtkWidget *child = gtk_widget_get_first_child(state->album_cover);
            while (child) {
                GtkWidget *next = gtk_widget_get_next_sibling(child);
                gtk_widget_unparent(child);
                child = next;
            }
            
            gtk_box_append(GTK_BOX(state->album_cover), image);
            g_object_unref(texture);
            g_object_unref(pixbuf);
        }
    }
    
    // Set source (player name)
    if (state->current_player) {
        GError *error = NULL;
        GDBusProxy *player_proxy = g_dbus_proxy_new_for_bus_sync(
            G_BUS_TYPE_SESSION,
            G_DBUS_PROXY_FLAGS_NONE,
            NULL,
            state->current_player,
            "/org/mpris/MediaPlayer2",
            "org.mpris.MediaPlayer2",
            NULL,
            &error
        );
        
        if (player_proxy && !error) {
            GVariant *identity = g_dbus_proxy_get_cached_property(player_proxy, "Identity");
            if (identity) {
                const gchar *player_name = g_variant_get_string(identity, NULL);
                gtk_label_set_text(GTK_LABEL(state->source_label), player_name);
                g_variant_unref(identity);
            }
            g_object_unref(player_proxy);
        } else if (error) {
            g_error_free(error);
        }
    }
    
    g_variant_unref(metadata);
    update_position(state);
}

static void on_properties_changed(GDBusProxy *proxy, GVariant *changed_properties,
                                  GStrv invalidated_properties, gpointer user_data) {
    AppState *state = (AppState *)user_data;
    update_metadata(state);
    
    GVariant *status_var = g_dbus_proxy_get_cached_property(proxy, "PlaybackStatus");
    if (status_var) {
        const gchar *status = g_variant_get_string(status_var, NULL);
        state->is_playing = g_strcmp0(status, "Playing") == 0;
        
        if (state->is_playing) {
            gtk_image_set_from_file(GTK_IMAGE(state->play_icon), "icons/pause.svg");
        } else {
            gtk_image_set_from_file(GTK_IMAGE(state->play_icon), "icons/play.svg");
        }
        g_variant_unref(status_var);
    }
}

static void connect_to_player(AppState *state, const gchar *bus_name) {
    if (state->mpris_proxy) {
        g_object_unref(state->mpris_proxy);
    }
    
    g_free(state->current_player);
    state->current_player = g_strdup(bus_name);
    
    GError *error = NULL;
    state->mpris_proxy = g_dbus_proxy_new_for_bus_sync(
        G_BUS_TYPE_SESSION,
        G_DBUS_PROXY_FLAGS_NONE,
        NULL,
        bus_name,
        "/org/mpris/MediaPlayer2",
        "org.mpris.MediaPlayer2.Player",
        NULL,
        &error
    );
    
    if (error) {
        g_printerr("Failed to connect to player: %s\n", error->message);
        g_error_free(error);
        return;
    }
    
    g_signal_connect(state->mpris_proxy, "g-properties-changed",
                     G_CALLBACK(on_properties_changed), state);
    
    update_metadata(state);
    g_print("Connected to player: %s\n", bus_name);
}

static void find_active_player(AppState *state) {
    GError *error = NULL;
    GDBusProxy *dbus_proxy = g_dbus_proxy_new_for_bus_sync(
        G_BUS_TYPE_SESSION,
        G_DBUS_PROXY_FLAGS_NONE,
        NULL,
        "org.freedesktop.DBus",
        "/org/freedesktop/DBus",
        "org.freedesktop.DBus",
        NULL,
        &error
    );
    
    if (error) {
        g_error_free(error);
        return;
    }
    
    GVariant *result = g_dbus_proxy_call_sync(
        dbus_proxy,
        "ListNames",
        NULL,
        G_DBUS_CALL_FLAGS_NONE,
        -1, NULL, &error
    );
    
    if (error) {
        g_error_free(error);
        g_object_unref(dbus_proxy);
        return;
    }
    
    GVariantIter *iter;
    g_variant_get(result, "(as)", &iter);
    
    const gchar *name;
    while (g_variant_iter_loop(iter, "&s", &name)) {
        if (g_str_has_prefix(name, "org.mpris.MediaPlayer2.")) {
            connect_to_player(state, name);
            break;
        }
    }
    
    g_variant_iter_free(iter);
    g_variant_unref(result);
    g_object_unref(dbus_proxy);
}

static void on_play_clicked(GtkButton *button, gpointer user_data) {
    AppState *state = (AppState *)user_data;
    
    if (!state->mpris_proxy) {
        find_active_player(state);
        return;
    }
    
    g_dbus_proxy_call(state->mpris_proxy, "PlayPause", NULL,
                      G_DBUS_CALL_FLAGS_NONE, -1, NULL, NULL, NULL);
}

static void on_prev_clicked(GtkButton *button, gpointer user_data) {
    AppState *state = (AppState *)user_data;
    if (!state->mpris_proxy) return;
    
    g_dbus_proxy_call(state->mpris_proxy, "Previous", NULL,
                      G_DBUS_CALL_FLAGS_NONE, -1, NULL, NULL, NULL);
}

static void on_next_clicked(GtkButton *button, gpointer user_data) {
    AppState *state = (AppState *)user_data;
    if (!state->mpris_proxy) return;
    
    g_dbus_proxy_call(state->mpris_proxy, "Next", NULL,
                      G_DBUS_CALL_FLAGS_NONE, -1, NULL, NULL, NULL);
}

static void on_revealer_transition_done(GObject *revealer, GParamSpec *pspec, gpointer user_data) {
    AppState *state = (AppState *)user_data;
    
    if (!state->is_expanded) {
        gtk_window_set_default_size(GTK_WINDOW(state->window), 1, 1);
        gtk_widget_queue_resize(state->window);
    }
}

static void on_expand_clicked(GtkButton *button, gpointer user_data) {
    AppState *state = (AppState *)user_data;
    state->is_expanded = !state->is_expanded;
    
    // Update icon using layout helper
    const gchar *icon_path = layout_get_expand_icon(state->layout, state->is_expanded);
    gtk_image_set_from_file(GTK_IMAGE(state->expand_icon), icon_path);
    
    gtk_revealer_set_reveal_child(GTK_REVEALER(state->revealer), state->is_expanded);
}

static void load_css() {
    GtkCssProvider *provider = gtk_css_provider_new();
    gtk_css_provider_load_from_path(provider, "style.css");
    gtk_style_context_add_provider_for_display(
        gdk_display_get_default(),
        GTK_STYLE_PROVIDER(provider),
        GTK_STYLE_PROVIDER_PRIORITY_APPLICATION
    );
    g_object_unref(provider);
}

static void activate(GtkApplication *app, gpointer user_data) {
    AppState *state = g_new0(AppState, 1);
    state->is_playing = FALSE;
    state->is_expanded = FALSE;
    state->mpris_proxy = NULL;
    state->current_player = NULL;
    state->layout = layout_load_config();
    
    GtkWidget *window = gtk_application_window_new(app);
    state->window = window;
    gtk_window_set_title(GTK_WINDOW(window), "HyprWave");
    
    // Layer Shell Setup
    gtk_layer_init_for_window(GTK_WINDOW(window));
    gtk_layer_set_exclusive_zone(GTK_WINDOW(window), 0);
    gtk_layer_set_layer(GTK_WINDOW(window), GTK_LAYER_SHELL_LAYER_OVERLAY);
    gtk_layer_set_namespace(GTK_WINDOW(window), "hyprwave");
    
    // Setup anchors using layout module
    layout_setup_window_anchors(GTK_WINDOW(window), state->layout);
    
    gtk_layer_set_keyboard_mode(GTK_WINDOW(window), GTK_LAYER_SHELL_KEYBOARD_MODE_NONE);
    gtk_widget_add_css_class(window, "hyprwave-window");
    
    // Create widget elements
    GtkWidget *album_cover = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    state->album_cover = album_cover;
    gtk_widget_add_css_class(album_cover, "album-cover");
    gtk_widget_set_size_request(album_cover, 120, 120);
    
    GtkWidget *source_label = gtk_label_new("No Source");
    state->source_label = source_label;
    gtk_widget_add_css_class(source_label, "source-label");
    
    GtkWidget *track_title = gtk_label_new("No Track Playing");
    state->track_title = track_title;
    gtk_widget_add_css_class(track_title, "track-title");
    gtk_label_set_ellipsize(GTK_LABEL(track_title), PANGO_ELLIPSIZE_END);
    gtk_label_set_max_width_chars(GTK_LABEL(track_title), 20);
    
    GtkWidget *artist_label = gtk_label_new("Unknown Artist");
    state->artist_label = artist_label;
    gtk_widget_add_css_class(artist_label, "artist-label");
    gtk_label_set_ellipsize(GTK_LABEL(artist_label), PANGO_ELLIPSIZE_END);
    gtk_label_set_max_width_chars(GTK_LABEL(artist_label), 20);
    
    GtkWidget *progress_bar = gtk_progress_bar_new();
    state->progress_bar = progress_bar;
    gtk_widget_add_css_class(progress_bar, "track-progress");
    gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progress_bar), 0.0);
    gtk_widget_set_size_request(progress_bar, 140, 4);
    
    GtkWidget *time_remaining = gtk_label_new("--:--");
    state->time_remaining = time_remaining;
    gtk_widget_add_css_class(time_remaining, "time-remaining");
    
    // Create expanded section using layout module
    ExpandedWidgets expanded_widgets = {
        .album_cover = album_cover,
        .source_label = source_label,
        .track_title = track_title,
        .artist_label = artist_label,
        .progress_bar = progress_bar,
        .time_remaining = time_remaining
    };
    
    GtkWidget *expanded_section = layout_create_expanded_section(state->layout, &expanded_widgets);
    
    // Create revealer
    GtkWidget *revealer = gtk_revealer_new();
    state->revealer = revealer;
    gtk_revealer_set_transition_type(GTK_REVEALER(revealer), layout_get_transition_type(state->layout));
    gtk_revealer_set_transition_duration(GTK_REVEALER(revealer), 300);
    gtk_revealer_set_child(GTK_REVEALER(revealer), expanded_section);
    gtk_revealer_set_reveal_child(GTK_REVEALER(revealer), FALSE);
    g_signal_connect(revealer, "notify::child-revealed", G_CALLBACK(on_revealer_transition_done), state);
    
    // Create buttons
    GtkWidget *prev_btn = gtk_button_new();
    gtk_widget_set_size_request(prev_btn, 44, 44);
    GtkWidget *prev_icon = gtk_image_new_from_file("icons/previous.svg");
    gtk_image_set_pixel_size(GTK_IMAGE(prev_icon), 20);
    gtk_button_set_child(GTK_BUTTON(prev_btn), prev_icon);
    gtk_widget_add_css_class(prev_btn, "control-button");
    gtk_widget_add_css_class(prev_btn, "prev-button");
    g_signal_connect(prev_btn, "clicked", G_CALLBACK(on_prev_clicked), state);
    
    GtkWidget *play_btn = gtk_button_new();
    gtk_widget_set_size_request(play_btn, 44, 44);
    GtkWidget *play_icon = gtk_image_new_from_file("icons/play.svg");
    state->play_icon = play_icon;
    gtk_image_set_pixel_size(GTK_IMAGE(play_icon), 20);
    gtk_button_set_child(GTK_BUTTON(play_btn), play_icon);
    gtk_widget_add_css_class(play_btn, "control-button");
    gtk_widget_add_css_class(play_btn, "play-button");
    g_signal_connect(play_btn, "clicked", G_CALLBACK(on_play_clicked), state);
    
    GtkWidget *next_btn = gtk_button_new();
    gtk_widget_set_size_request(next_btn, 44, 44);
    GtkWidget *next_icon = gtk_image_new_from_file("icons/next.svg");
    gtk_image_set_pixel_size(GTK_IMAGE(next_icon), 20);
    gtk_button_set_child(GTK_BUTTON(next_btn), next_icon);
    gtk_widget_add_css_class(next_btn, "control-button");
    gtk_widget_add_css_class(next_btn, "next-button");
    g_signal_connect(next_btn, "clicked", G_CALLBACK(on_next_clicked), state);
    
    GtkWidget *expand_btn = gtk_button_new();
    gtk_widget_set_size_request(expand_btn, 44, 44);
    const gchar *initial_icon = layout_get_expand_icon(state->layout, FALSE);
    GtkWidget *expand_icon = gtk_image_new_from_file(initial_icon);
    state->expand_icon = expand_icon;
    gtk_image_set_pixel_size(GTK_IMAGE(expand_icon), 20);
    gtk_button_set_child(GTK_BUTTON(expand_btn), expand_icon);
    gtk_widget_add_css_class(expand_btn, "control-button");
    gtk_widget_add_css_class(expand_btn, "expand-button");
    g_signal_connect(expand_btn, "clicked", G_CALLBACK(on_expand_clicked), state);
    
    // Create control bar using layout module
    GtkWidget *control_bar = layout_create_control_bar(state->layout, &prev_btn, &play_btn, &next_btn, &expand_btn);
    
    // Create main container using layout module
    GtkWidget *main_container = layout_create_main_container(state->layout, control_bar, revealer);
    
    gtk_window_set_child(GTK_WINDOW(window), main_container);
    gtk_window_present(GTK_WINDOW(window));
    
    // Find and connect to active media player
    find_active_player(state);
    
    // Update position every second
    state->update_timer = g_timeout_add_seconds(1, update_position_tick, state);
    
    g_print("HyprWave started!\n");
}

int main(int argc, char **argv) {
    GtkApplication *app = gtk_application_new("com.hyprwave.app", G_APPLICATION_DEFAULT_FLAGS);
    
    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
    g_signal_connect(app, "startup", G_CALLBACK(load_css), NULL);
    
    int status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);
    
    return status;
}
