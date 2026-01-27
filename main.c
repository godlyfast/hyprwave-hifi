#include <gtk/gtk.h>
#include <gtk4-layer-shell.h>
#include <gio/gio.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include "layout.h"
#include "paths.h"
#include "notification.h"
#include "art.h"
#include "volume.h"
#include "visualizer.h"

typedef struct {
    GtkWidget *window;
    GtkWidget *window_revealer;
    GtkWidget *revealer;
    GtkWidget *play_icon;
    GtkWidget *expand_icon;
    GtkWidget *album_cover;
    GtkWidget *source_label;
    GtkWidget *format_label;           // Hi-Fi: Bitrate/format display
    GtkWidget *track_title;
    GtkWidget *artist_label;
    GtkWidget *time_remaining;
    GtkWidget *progress_bar;
    GtkWidget *player_label;           // Hi-Fi: Player selector display
    GtkWidget *expanded_with_volume;
    gboolean is_playing;
    gboolean is_expanded;
    gboolean is_visible;
    gboolean is_seeking;
    gboolean can_seek;                 // Hi-Fi: True if player supports seeking
    GDBusProxy *mpris_proxy;
    gchar *current_player;
    guint update_timer;
    LayoutConfig *layout;
    NotificationState *notification;
    VolumeState *volume;
    gchar *last_track_id;
    gchar *last_title;                 // Hi-Fi: Fallback for track change detection
    gchar *current_track_id;           // Hi-Fi: For seek operations
    gint64 current_length;             // Hi-Fi: Track length in microseconds
    guint notification_timer;
    gchar *pending_title;
    gchar *pending_artist;
    gchar *pending_art_url;
    GtkWidget *control_bar_container;  // Store reference to control bar
    GtkWidget *prev_btn;               // Store button references
    GtkWidget *play_btn;
    GtkWidget *next_btn;
    GtkWidget *expand_btn;
    // Hi-Fi: Multi-player support
    gchar **players;                   // Array of MPRIS player D-Bus names
    gint player_count;
    gint current_player_index;
    gchar *player_display_name;        // Human-readable name from Identity
    gboolean suppress_notification;    // Suppress during player switch

    VisualizerState *visualizer;       // Visualizer state (in expanded section)
    GtkWidget *visualizer_box;         // Container for visualizer bars
} AppState;

static void update_position(AppState *state);
static void update_metadata(AppState *state);
static void update_playback_status(AppState *state);
static void on_expand_clicked(GtkButton *button, gpointer user_data);
static void on_properties_changed(GDBusProxy *proxy, GVariant *changed_properties,
                                  GStrv invalidated_properties, gpointer user_data);
                                  
// Visualizer control (for expanded section)
static void start_visualizer_if_expanded(AppState *state);
static void stop_visualizer_if_collapsed(AppState *state);

// Hi-Fi: Multi-player functions
static void load_available_players(AppState *state);
static void switch_to_player(AppState *state, const gchar *bus_name);
static void cycle_player(AppState *state, gboolean forward);
static gchar* load_preferred_player(void);
static void save_preferred_player(const gchar *bus_name);

static AppState *global_state = NULL;

// ========================================
// Hi-Fi: PLAYER FILTERING
// ========================================

// Check if a D-Bus name should be excluded from player list
static gboolean is_excluded_player(const gchar *name) {
    // Exclude playerctld (it's a proxy, not a real player)
    if (g_str_has_suffix(name, ".playerctld")) return TRUE;

    // Exclude common browsers (poor MPRIS metadata)
    const gchar *excluded[] = {
        ".firefox", ".chromium", ".chrome", ".brave",
        ".vivaldi", ".opera", ".edge", NULL
    };

    for (const gchar **ex = excluded; *ex; ex++) {
        if (g_str_has_suffix(name, *ex)) return TRUE;
    }
    return FALSE;
}

// Check if chromium-based player is allowed (e.g., Cider, tidal-hifi)
// For chromium.instance* names, check the Identity property
static gboolean is_allowed_chromium_player(const gchar *name) {
    // Allow specific names directly in the D-Bus name
    const gchar *allowed[] = {
        "Cider", "tidal", "hifi", "qobuz", "spotify", "Plexamp", "roon", NULL
    };

    for (const gchar **a = allowed; *a; a++) {
        if (g_strstr_len(name, -1, *a)) return TRUE;
    }

    // If it's a chromium instance, check the Identity property
    if (g_strstr_len(name, -1, "chromium.instance") ||
        g_strstr_len(name, -1, "chrome.instance")) {
        // Fetch Identity property to check if it's an allowed app
        GDBusProxy *proxy = g_dbus_proxy_new_for_bus_sync(
            G_BUS_TYPE_SESSION, G_DBUS_PROXY_FLAGS_NONE, NULL,
            name, "/org/mpris/MediaPlayer2",
            "org.mpris.MediaPlayer2", NULL, NULL);

        if (proxy) {
            GVariant *identity = g_dbus_proxy_get_cached_property(proxy, "Identity");
            if (identity) {
                const gchar *id_str = g_variant_get_string(identity, NULL);
                gboolean allowed_app = FALSE;

                // Check if Identity contains allowed app names
                for (const gchar **a = allowed; *a; a++) {
                    if (g_strstr_len(id_str, -1, *a)) {
                        allowed_app = TRUE;
                        break;
                    }
                }

                g_variant_unref(identity);
                g_object_unref(proxy);
                return allowed_app;
            }
            g_object_unref(proxy);
        }
        return FALSE;  // Unknown chromium instance, filter it
    }

    // Block generic browser names
    if (g_strstr_len(name, -1, "chromium") ||
        g_strstr_len(name, -1, "chrome") ||
        g_strstr_len(name, -1, "firefox")) {
        return FALSE;
    }

    return TRUE;  // Allow non-chromium players
}

// ========================================
// Hi-Fi: PLAYER SWITCHING
// ========================================

// Load all available MPRIS players from D-Bus
static void load_available_players(AppState *state) {
    // Free previous list
    if (state->players) {
        g_strfreev(state->players);
        state->players = NULL;
    }
    state->player_count = 0;

    GError *error = NULL;
    GDBusProxy *dbus_proxy = g_dbus_proxy_new_for_bus_sync(
        G_BUS_TYPE_SESSION, G_DBUS_PROXY_FLAGS_NONE, NULL,
        "org.freedesktop.DBus", "/org/freedesktop/DBus",
        "org.freedesktop.DBus", NULL, &error);

    if (error) {
        g_error_free(error);
        return;
    }

    GVariant *result = g_dbus_proxy_call_sync(dbus_proxy, "ListNames",
        NULL, G_DBUS_CALL_FLAGS_NONE, -1, NULL, &error);

    if (error) {
        g_error_free(error);
        g_object_unref(dbus_proxy);
        return;
    }

    GVariantIter *iter;
    g_variant_get(result, "(as)", &iter);

    const gchar *name;
    GPtrArray *player_arr = g_ptr_array_new();

    while (g_variant_iter_loop(iter, "&s", &name)) {
        if (g_str_has_prefix(name, "org.mpris.MediaPlayer2.") &&
            !is_excluded_player(name) &&
            is_allowed_chromium_player(name)) {
            g_ptr_array_add(player_arr, g_strdup(name));
        }
    }

    g_variant_iter_free(iter);
    g_variant_unref(result);
    g_object_unref(dbus_proxy);

    g_ptr_array_add(player_arr, NULL);
    state->players = (gchar **)g_ptr_array_free(player_arr, FALSE);

    // Count players
    state->player_count = 0;
    for (gchar **p = state->players; *p; p++) state->player_count++;

    // Find current player index
    state->current_player_index = -1;
    if (state->current_player && state->players) {
        for (int i = 0; state->players[i]; i++) {
            if (g_strcmp0(state->players[i], state->current_player) == 0) {
                state->current_player_index = i;
                break;
            }
        }
    }

    // Update player label
    if (state->player_label) {
        if (state->player_display_name) {
            gtk_label_set_text(GTK_LABEL(state->player_label), state->player_display_name);
        } else if (state->player_count > 0) {
            gtk_label_set_text(GTK_LABEL(state->player_label), "Click to switch");
        } else {
            gtk_label_set_text(GTK_LABEL(state->player_label), "No players");
        }
    }
}

// Save preferred player to config file
static void save_preferred_player(const gchar *bus_name) {
    gchar *config_dir = g_build_filename(g_get_user_config_dir(), "hyprwave", NULL);
    gchar *pref_file = g_build_filename(config_dir, "preferred_player", NULL);
    g_mkdir_with_parents(config_dir, 0755);
    g_file_set_contents(pref_file, bus_name, -1, NULL);
    g_free(pref_file);
    g_free(config_dir);
}

// Load preferred player from config file
static gchar* load_preferred_player(void) {
    gchar *pref_file = g_build_filename(g_get_user_config_dir(), "hyprwave", "preferred_player", NULL);
    gchar *contents = NULL;
    if (g_file_get_contents(pref_file, &contents, NULL, NULL)) {
        g_strstrip(contents);
    }
    g_free(pref_file);
    return contents;
}

// Switch to a specific MPRIS player
static void switch_to_player(AppState *state, const gchar *bus_name) {
    if (!bus_name) return;

    // Disconnect from current player
    if (state->mpris_proxy) {
        g_object_unref(state->mpris_proxy);
        state->mpris_proxy = NULL;
    }

    g_free(state->current_player);
    state->current_player = g_strdup(bus_name);

    GError *error = NULL;
    state->mpris_proxy = g_dbus_proxy_new_for_bus_sync(
        G_BUS_TYPE_SESSION, G_DBUS_PROXY_FLAGS_NONE, NULL,
        bus_name, "/org/mpris/MediaPlayer2",
        "org.mpris.MediaPlayer2.Player", NULL, &error);

    if (error) {
        g_printerr("Failed to connect to player: %s\n", error->message);
        g_error_free(error);
        g_free(state->current_player);
        state->current_player = NULL;
        return;
    }

    g_signal_connect(state->mpris_proxy, "g-properties-changed",
                     G_CALLBACK(on_properties_changed), state);

    // Get display name from Identity
    GDBusProxy *player_proxy = g_dbus_proxy_new_for_bus_sync(
        G_BUS_TYPE_SESSION, G_DBUS_PROXY_FLAGS_NONE, NULL,
        bus_name, "/org/mpris/MediaPlayer2",
        "org.mpris.MediaPlayer2", NULL, NULL);

    g_free(state->player_display_name);
    const gchar *fallback_name = strrchr(bus_name, '.');
    state->player_display_name = g_strdup(fallback_name ? fallback_name + 1 : "Unknown");

    if (player_proxy) {
        GVariant *identity = g_dbus_proxy_get_cached_property(player_proxy, "Identity");
        if (identity) {
            g_free(state->player_display_name);
            state->player_display_name = g_strdup(g_variant_get_string(identity, NULL));
            g_variant_unref(identity);
        }
        g_object_unref(player_proxy);
    }

    // Check seeking support
    state->can_seek = FALSE;
    GVariant *seek_res = g_dbus_proxy_call_sync(state->mpris_proxy,
        "org.freedesktop.DBus.Properties.Get",
        g_variant_new("(ss)", "org.mpris.MediaPlayer2.Player", "CanSeek"),
        G_DBUS_CALL_FLAGS_NONE, -1, NULL, NULL);
    if (seek_res) {
        GVariant *val = NULL;
        g_variant_get(seek_res, "(v)", &val);
        if (val) {
            state->can_seek = g_variant_get_boolean(val);
            g_variant_unref(val);
        }
        g_variant_unref(seek_res);
    }

    // Update display and save preference
    if (state->player_label) {
        gtk_label_set_text(GTK_LABEL(state->player_label), state->player_display_name);
    }
    save_preferred_player(bus_name);

    g_print("Switched to player: %s (%s)\n", state->player_display_name, bus_name);

    // Suppress notification during player switch
    state->suppress_notification = TRUE;
    update_metadata(state);
    update_playback_status(state);
    state->suppress_notification = FALSE;

    // Update volume control with new player
    if (state->volume) {
        state->volume->mpris_proxy = state->mpris_proxy;
    }
}

static void cycle_player(AppState *state, gboolean forward) {
    load_available_players(state);

    if (!state->players || state->player_count == 0) {
        g_print("No MPRIS players available\n");
        return;
    }

    gint new_index;
    if (state->current_player_index < 0) {
        new_index = 0;
    } else if (forward) {
        new_index = (state->current_player_index + 1) % state->player_count;
    } else {
        new_index = (state->current_player_index - 1 + state->player_count) % state->player_count;
    }

    switch_to_player(state, state->players[new_index]);
}

static void on_player_clicked(GtkGestureClick *gesture, gint n_press, gdouble x, gdouble y, gpointer user_data) {
    AppState *state = (AppState *)user_data;
    cycle_player(state, TRUE);
}

// FIXED: Smooth contract animation callback
static void on_revealer_transition_done(GObject *revealer_obj, GParamSpec *pspec, gpointer user_data) {
    AppState *state = (AppState *)user_data;
    if (!gtk_revealer_get_child_revealed(GTK_REVEALER(revealer_obj))) {
        // SMOOTH CONTRACT ANIMATION: Set proper control bar size after transition
        if (state->layout->is_vertical) {
            gtk_window_set_default_size(GTK_WINDOW(state->window), -1, 60);
        } else {
            gtk_window_set_default_size(GTK_WINDOW(state->window), 300, -1);
        }
        gtk_widget_queue_resize(state->window);
    }
}

static void on_window_hide_complete(GObject *revealer, GParamSpec *pspec, gpointer user_data) {
    AppState *state = (AppState *)user_data;
    if (!gtk_revealer_get_child_revealed(GTK_REVEALER(state->window_revealer))) {
        gtk_widget_set_visible(state->window, FALSE);
    }
}

static void handle_sigusr1(int sig) {
    if (!global_state) return;
    global_state->is_visible = !global_state->is_visible;

    if (!global_state->is_visible) {
        // HIDE: Stop visualizer if expanded
        if (global_state->is_expanded && global_state->visualizer) {
            visualizer_stop(global_state->visualizer);
        }

        if (global_state->is_expanded) {
            global_state->is_expanded = FALSE;
            gtk_revealer_set_reveal_child(GTK_REVEALER(global_state->revealer), FALSE);
        }
        gtk_revealer_set_reveal_child(GTK_REVEALER(global_state->window_revealer), FALSE);
    } else {
        // SHOW
        gtk_widget_set_visible(global_state->window, TRUE);
        gtk_revealer_set_reveal_child(GTK_REVEALER(global_state->window_revealer), TRUE);
    }
}

static void handle_sigusr2(int sig) {
    if (!global_state) return;
    if (!global_state->is_visible) return;
    on_expand_clicked(NULL, global_state);
}




// ========================================
// VISUALIZER CONTROL (for expanded section)
// ========================================

static void start_visualizer_if_expanded(AppState *state) {
    if (!state->visualizer || !state->layout->visualizer_enabled) return;
    if (!state->visualizer->is_running) {
        visualizer_start(state->visualizer);
        g_print("✓ Visualizer started (expanded)\n");
    }
    visualizer_show(state->visualizer);
}

static void stop_visualizer_if_collapsed(AppState *state) {
    if (!state->visualizer) return;
    visualizer_hide(state->visualizer);
    // Keep audio capture running for quick resume
}

static gint64 get_variant_as_int64(GVariant *value) {
    if (value == NULL) return 0;
    if (g_variant_is_of_type(value, G_VARIANT_TYPE_INT64)) return g_variant_get_int64(value);
    else if (g_variant_is_of_type(value, G_VARIANT_TYPE_UINT64)) return (gint64)g_variant_get_uint64(value);
    else if (g_variant_is_of_type(value, G_VARIANT_TYPE_INT32)) return (gint64)g_variant_get_int32(value);
    else if (g_variant_is_of_type(value, G_VARIANT_TYPE_UINT32)) return (gint64)g_variant_get_uint32(value);
    else if (g_variant_is_of_type(value, G_VARIANT_TYPE_DOUBLE)) return (gint64)g_variant_get_double(value);
    return 0;
}

static gboolean update_position_tick(gpointer user_data) {
    AppState *state = (AppState *)user_data;
    update_position(state);
    return G_SOURCE_CONTINUE;
}

static gboolean clear_seeking_flag(gpointer user_data) {
    AppState *state = (AppState *)user_data;
    state->is_seeking = FALSE;
    return G_SOURCE_REMOVE;
}

static void perform_seek(AppState *state, gdouble fraction) {
    if (!state->mpris_proxy) return;
    
    GVariant *metadata = g_dbus_proxy_get_cached_property(state->mpris_proxy, "Metadata");
    if (!metadata) return;
    
    gint64 length = 0;
    const gchar *track_id = NULL;
    GVariantIter iter;
    gchar *key;
    GVariant *val;

    g_variant_iter_init(&iter, metadata);
    while (g_variant_iter_loop(&iter, "{sv}", &key, &val)) {
        if (g_strcmp0(key, "mpris:length") == 0) {
            length = get_variant_as_int64(val);
        } else if (g_strcmp0(key, "mpris:trackid") == 0) {
            track_id = g_variant_get_string(val, NULL);
        }
    }

    if (length > 0 && track_id) {
        gint64 target_position = (gint64)(fraction * length);
        g_dbus_proxy_call(state->mpris_proxy, "SetPosition",
            g_variant_new("(ox)", track_id, target_position),
            G_DBUS_CALL_FLAGS_NONE, -1, NULL, NULL, NULL);
        g_print("Seeking to %.1f%% (position: %ld µs)\n", fraction * 100, target_position);
    }
    g_variant_unref(metadata);
}

static void on_change_value(GtkRange *range, GtkScrollType scroll, gdouble value, gpointer user_data) {
    AppState *state = (AppState *)user_data;
    state->is_seeking = TRUE;
    
    if (state->mpris_proxy) {
        GVariant *metadata = g_dbus_proxy_get_cached_property(state->mpris_proxy, "Metadata");
        if (metadata) {
            gint64 length = 0;
            GVariantIter iter;
            gchar *key;
            GVariant *val;
            
            g_variant_iter_init(&iter, metadata);
            while (g_variant_iter_loop(&iter, "{sv}", &key, &val)) {
                if (g_strcmp0(key, "mpris:length") == 0) {
                    length = get_variant_as_int64(val);
                    break;
                }
            }
            g_variant_unref(metadata);

            if (length > 0) {
                gint64 target_pos = (gint64)(value * length);
                gint64 pos_seconds = target_pos / 1000000;
                gint64 len_seconds = length / 1000000;
                gint64 rem_seconds = len_seconds - pos_seconds;
                
                char time_str[32];
                if (rem_seconds >= 0) {
                    snprintf(time_str, sizeof(time_str), "-%ld:%02ld", 
                            rem_seconds / 60, rem_seconds % 60);
                } else {
                    snprintf(time_str, sizeof(time_str), "%ld:%02ld", 
                            pos_seconds / 60, pos_seconds % 60);
                }
                gtk_label_set_text(GTK_LABEL(state->time_remaining), time_str);
            }
        }
    }
}

static gboolean on_button_release_event(GtkWidget *widget, GdkEvent *event, gpointer user_data) {
    AppState *state = (AppState *)user_data;
    
    if (gdk_event_get_event_type(event) != GDK_BUTTON_RELEASE) {
        return FALSE;
    }
    
    gdouble value = gtk_range_get_value(GTK_RANGE(state->progress_bar));
    g_print("Button released - seeking to %.1f%%\n", value * 100);
    perform_seek(state, value);
    g_timeout_add(500, clear_seeking_flag, state);
    
    return FALSE;
}

static void on_position_received(GObject *source_object, GAsyncResult *res, gpointer user_data) {
    AppState *state = (AppState *)user_data;
    GError *error = NULL;
    
    GVariant *position_container = g_dbus_proxy_call_finish(G_DBUS_PROXY(source_object), res, &error);
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
    g_signal_handlers_block_by_func(state->progress_bar, on_change_value, state);
    gtk_range_set_value(GTK_RANGE(state->progress_bar), fraction);
    g_signal_handlers_unblock_by_func(state->progress_bar, on_change_value, state);
}

static void update_position(AppState *state) {
    if (state->is_seeking) return;
    if (!state->mpris_proxy) return;
    
    g_dbus_proxy_call(state->mpris_proxy,
        "org.freedesktop.DBus.Properties.Get",
        g_variant_new("(ss)", "org.mpris.MediaPlayer2.Player", "Position"),
        G_DBUS_CALL_FLAGS_NONE, -1, NULL, on_position_received, state);
}

static gint notification_retry_count = 0;
#define MAX_NOTIFICATION_RETRIES 5

static gboolean show_pending_notification(gpointer user_data) {
    AppState *state = (AppState *)user_data;
    gboolean has_title = state->pending_title && strlen(state->pending_title) > 0;
    gboolean has_artist = state->pending_artist && strlen(state->pending_artist) > 0;

    if (!has_title || !has_artist) {
        notification_retry_count++;
        if (notification_retry_count < MAX_NOTIFICATION_RETRIES) {
            state->notification_timer = g_timeout_add(200, show_pending_notification, state);
            return G_SOURCE_REMOVE;
        }
        g_print("Notification skipped - metadata incomplete\n");
    } else {
        notification_show(state->notification, state->pending_title,
                          state->pending_artist, state->pending_art_url, "Now Playing");
    }

    g_free(state->pending_title);
    g_free(state->pending_artist);
    g_free(state->pending_art_url);
    state->pending_title = NULL;
    state->pending_artist = NULL;
    state->pending_art_url = NULL;
    state->notification_timer = 0;
    notification_retry_count = 0;
    return G_SOURCE_REMOVE;
}

static void update_metadata(AppState *state) {
    if (!state->mpris_proxy) return;
    GVariant *metadata = g_dbus_proxy_get_cached_property(state->mpris_proxy, "Metadata");
    if (!metadata) return;

    GVariantIter iter;
    GVariant *value;
    gchar *key;
    gchar *title = NULL;
    gchar *artist = NULL;
    gchar *art_url = NULL;
    gchar *track_id = NULL;

    g_variant_iter_init(&iter, metadata);
    while (g_variant_iter_loop(&iter, "{sv}", &key, &value)) {
        if (g_strcmp0(key, "xesam:title") == 0) {
            g_free(title);
            title = g_strdup(g_variant_get_string(value, NULL));
        }
        else if (g_strcmp0(key, "xesam:artist") == 0) {
            if (g_variant_is_of_type(value, G_VARIANT_TYPE_STRING_ARRAY)) {
                gsize length;
                const gchar **artists = g_variant_get_strv(value, &length);
                if (length > 0) {
                    g_free(artist);
                    artist = g_strdup(artists[0]);
                }
                g_free(artists);
            }
        }
        else if (g_strcmp0(key, "mpris:artUrl") == 0) {
            g_free(art_url);
            art_url = g_strdup(g_variant_get_string(value, NULL));
        }
        else if (g_strcmp0(key, "mpris:trackid") == 0) {
            g_free(track_id);
            track_id = g_strdup(g_variant_get_string(value, NULL));
        }
    }
    
    gboolean track_changed = FALSE;
    if (track_id && state->last_track_id) {
        track_changed = (g_strcmp0(track_id, state->last_track_id) != 0);
    } else if (track_id && !state->last_track_id) {
        track_changed = TRUE;
    }
    
    if (track_id) {
        g_free(state->last_track_id);
        state->last_track_id = g_strdup(track_id);
    }
    
    if (state->layout->notifications_enabled && state->layout->now_playing_enabled && 
        state->notification && track_changed) {
        if (state->notification_timer > 0) {
            g_source_remove(state->notification_timer);
            state->notification_timer = 0;
        }
        notification_retry_count = 0;
        g_free(state->pending_title);
        g_free(state->pending_artist);
        g_free(state->pending_art_url);
        state->pending_title = g_strdup(title);
        state->pending_artist = g_strdup(artist);
        state->pending_art_url = g_strdup(art_url);
        if (state->notification->album_cover) {
            clear_album_art_container(state->notification->album_cover);
            load_album_art_to_container(art_url, state->notification->album_cover, 70);
        }
        state->notification_timer = g_timeout_add(300, show_pending_notification, state);
    }
    
    if (title && strlen(title) > 0) {
        gtk_label_set_text(GTK_LABEL(state->track_title), title);
    } else {
        gtk_label_set_text(GTK_LABEL(state->track_title), "No Track Playing");
    }
    
    if (artist && strlen(artist) > 0) {
        gtk_label_set_text(GTK_LABEL(state->artist_label), artist);
    } else {
        gtk_label_set_text(GTK_LABEL(state->artist_label), "Unknown Artist");
    }
    
    load_album_art_to_container(art_url, state->album_cover, 240);
    
    if (state->current_player) {
        GError *error = NULL;
        GDBusProxy *player_proxy = g_dbus_proxy_new_for_bus_sync(
            G_BUS_TYPE_SESSION, G_DBUS_PROXY_FLAGS_NONE, NULL,
            state->current_player, "/org/mpris/MediaPlayer2",
            "org.mpris.MediaPlayer2", NULL, &error);

        if (player_proxy && !error) {
            GVariant *identity = g_dbus_proxy_get_cached_property(player_proxy, "Identity");
            if (identity) {
                gtk_label_set_text(GTK_LABEL(state->source_label), 
                                   g_variant_get_string(identity, NULL));
                g_variant_unref(identity);
            }
            g_object_unref(player_proxy);
        } else if (error) {
            g_error_free(error);
        }
    }

    g_free(title);
    g_free(artist);
    g_free(art_url);
    g_free(track_id);
    g_variant_unref(metadata);
    update_position(state);
}

static void update_playback_status(AppState *state) {
    if (!state->mpris_proxy) return;
    GVariant *status_var = g_dbus_proxy_get_cached_property(state->mpris_proxy, "PlaybackStatus");
    if (status_var) {
        const gchar *status = g_variant_get_string(status_var, NULL);
        state->is_playing = g_strcmp0(status, "Playing") == 0;
        gchar *icon_path = get_icon_path(state->is_playing ? "pause.svg" : "play.svg");
        gtk_image_set_from_file(GTK_IMAGE(state->play_icon), icon_path);
        free_path(icon_path);
        g_variant_unref(status_var);
    }
}

static void on_properties_changed(GDBusProxy *proxy, GVariant *changed_properties,
                                  GStrv invalidated_properties, gpointer user_data) {
    AppState *state = (AppState *)user_data;
    update_metadata(state);
    update_playback_status(state);
}

static void connect_to_player(AppState *state, const gchar *bus_name) {
    if (state->mpris_proxy) g_object_unref(state->mpris_proxy);
    g_free(state->current_player);
    state->current_player = g_strdup(bus_name);

    GError *error = NULL;
    state->mpris_proxy = g_dbus_proxy_new_for_bus_sync(
        G_BUS_TYPE_SESSION, G_DBUS_PROXY_FLAGS_NONE, NULL, bus_name,
        "/org/mpris/MediaPlayer2", "org.mpris.MediaPlayer2.Player", NULL, &error);

    if (error) {
        g_printerr("Failed to connect: %s\n", error->message);
        g_error_free(error);
        return;
    }

    g_signal_connect(state->mpris_proxy, "g-properties-changed",
                     G_CALLBACK(on_properties_changed), state);
    update_metadata(state);
    
    if (state->volume) {
        state->volume->mpris_proxy = state->mpris_proxy;
    }
    
    g_print("Connected to player: %s\n", bus_name);
}

static void find_active_player(AppState *state) {
    // Hi-Fi: Use multi-player logic with preference persistence
    load_available_players(state);

    if (state->player_count == 0) {
        g_print("No MPRIS players found\n");
        return;
    }

    // Try to restore preferred player
    gchar *preferred = load_preferred_player();
    if (preferred) {
        for (int i = 0; state->players[i]; i++) {
            if (g_strcmp0(state->players[i], preferred) == 0) {
                switch_to_player(state, preferred);
                g_free(preferred);
                return;
            }
        }
        g_free(preferred);
    }

    // Otherwise connect to first available player
    if (state->players[0]) {
        switch_to_player(state, state->players[0]);
    }
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

static void on_expand_clicked(GtkButton *button, gpointer user_data) {
    AppState *state = (AppState *)user_data;

    state->is_expanded = !state->is_expanded;

    if (!state->is_expanded && state->volume && state->volume->is_showing) {
        volume_hide(state->volume);
    }

    const gchar *icon_name = layout_get_expand_icon(state->layout, state->is_expanded);
    gchar *icon_path = get_icon_path(icon_name);
    gtk_image_set_from_file(GTK_IMAGE(state->expand_icon), icon_path);
    free_path(icon_path);
    gtk_revealer_set_reveal_child(GTK_REVEALER(state->revealer), state->is_expanded);

    // Start/stop visualizer based on expanded state
    if (state->is_expanded) {
        start_visualizer_if_expanded(state);
    } else {
        stop_visualizer_if_collapsed(state);
    }
}

static void on_album_double_click(GtkGestureClick *gesture,
                                   int n_press,
                                   double x, double y,
                                   gpointer user_data) {
    AppState *state = (AppState *)user_data;
    
    if (n_press == 2 && state->volume) {
        if (state->volume->is_showing) {
            volume_hide(state->volume);
            g_print("Volume control hidden via double-click\n");
        } else {
            volume_show(state->volume);
            g_print("Volume control activated via double-click\n");
        }
    }
}

static gboolean enable_smooth_transitions(gpointer user_data) {
    GtkWidget *window_revealer = GTK_WIDGET(user_data);
    gtk_revealer_set_transition_duration(GTK_REVEALER(window_revealer), 300);
    g_print("Smooth transitions enabled\n");
    return G_SOURCE_REMOVE;
}

static void on_volume_visibility_changed(GObject *revealer, GParamSpec *pspec, gpointer user_data) {
    AppState *state = (AppState *)user_data;
    
    if (!state->layout->is_vertical && state->expanded_with_volume) {
        gtk_widget_queue_resize(state->expanded_with_volume);
        gtk_widget_queue_allocate(state->expanded_with_volume);
    }
}

static void load_css() {
    // 1. Load base style.css
    gchar *css_path = get_style_path();
    GtkCssProvider *provider = gtk_css_provider_new();
    GFile *base_file = g_file_new_for_path(css_path);
    GError *base_error = NULL;
    gchar *base_contents = NULL;
    gsize base_length = 0;

    if (g_file_load_contents(base_file, NULL, &base_contents, &base_length, NULL, &base_error)) {
        gtk_css_provider_load_from_string(provider, base_contents);
        gtk_style_context_add_provider_for_display(gdk_display_get_default(),
            GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
        g_print("Base CSS loaded successfully\n");
        g_free(base_contents);
    } else {
        g_warning("Failed to load CSS: %s", base_error ? base_error->message : "Unknown");
        if (base_error) g_error_free(base_error);
    }
    g_object_unref(base_file);
    g_object_unref(provider);
    free_path(css_path);

    // 2. Hi-Fi: Load theme CSS if not using default "light" theme
    gchar *theme = get_config_theme();
    g_print("Theme from config: %s\n", theme);

    gchar *theme_path = get_theme_path(theme);
    if (theme_path) {
        GtkCssProvider *theme_provider = gtk_css_provider_new();
        GFile *theme_file = g_file_new_for_path(theme_path);
        GError *theme_error = NULL;
        gchar *theme_contents = NULL;
        gsize theme_length = 0;

        if (g_file_load_contents(theme_file, NULL, &theme_contents, &theme_length, NULL, &theme_error)) {
            gtk_css_provider_load_from_string(theme_provider, theme_contents);
            gtk_style_context_add_provider_for_display(gdk_display_get_default(),
                GTK_STYLE_PROVIDER(theme_provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION + 1);
            g_print("Theme CSS loaded: %s\n", theme_path);
            g_free(theme_contents);
        } else {
            g_warning("Failed to load theme CSS: %s", theme_error ? theme_error->message : "Unknown");
            if (theme_error) g_error_free(theme_error);
        }
        g_object_unref(theme_file);
        g_object_unref(theme_provider);
        free_path(theme_path);
    }
    g_free(theme);

    // 3. Hi-Fi: Load optional user CSS overrides
    gchar *user_css = g_build_filename(g_get_user_config_dir(), "hyprwave", "user.css", NULL);
    if (g_file_test(user_css, G_FILE_TEST_EXISTS)) {
        GtkCssProvider *user_provider = gtk_css_provider_new();
        GFile *css_file = g_file_new_for_path(user_css);
        GError *css_error = NULL;
        gchar *css_contents = NULL;
        gsize css_length = 0;

        if (g_file_load_contents(css_file, NULL, &css_contents, &css_length, NULL, &css_error)) {
            gtk_css_provider_load_from_string(user_provider, css_contents);
            gtk_style_context_add_provider_for_display(gdk_display_get_default(),
                GTK_STYLE_PROVIDER(user_provider), GTK_STYLE_PROVIDER_PRIORITY_USER);
            g_print("User CSS loaded from: %s\n", user_css);
            g_free(css_contents);
        }
        if (css_error) g_error_free(css_error);
        g_object_unref(css_file);
        g_object_unref(user_provider);
    }
    g_free(user_css);
}

static gboolean delayed_window_show(gpointer user_data) {
    gtk_widget_set_visible(GTK_WIDGET(user_data), TRUE);
    return G_SOURCE_REMOVE;
}

static gboolean delayed_revealer_reveal(gpointer user_data) {
    gtk_revealer_set_reveal_child(GTK_REVEALER(user_data), TRUE);
    return G_SOURCE_REMOVE;
}


static void activate(GtkApplication *app, gpointer user_data) {
    AppState *state = g_new0(AppState, 1);
    state->is_playing = FALSE;
    state->is_expanded = FALSE;
    state->is_visible = TRUE;
    state->is_seeking = FALSE;
    state->mpris_proxy = NULL;
    state->current_player = NULL;
    state->last_track_id = NULL;
    state->layout = layout_load_config();
    state->notification = notification_init(app);
    state->volume = NULL;
    state->visualizer = NULL;
    state->visualizer_box = NULL;
    
    // Create window FIRST
    GtkWidget *window = gtk_application_window_new(app);
    state->window = window;
    gtk_window_set_title(GTK_WINDOW(window), "HyprWave");
    
    // Set window size IMMEDIATELY to match control_bar
    if (state->layout->is_vertical) {
        gtk_window_set_default_size(GTK_WINDOW(window), 50, -1);
        gtk_window_set_resizable(GTK_WINDOW(window), FALSE);
    } else {
        gtk_window_set_default_size(GTK_WINDOW(window), -1, 60);
        gtk_window_set_resizable(GTK_WINDOW(window), FALSE);
    }
    
    // LAYER SHELL SETUP
    gtk_layer_init_for_window(GTK_WINDOW(window));
    gtk_layer_set_layer(GTK_WINDOW(window), GTK_LAYER_SHELL_LAYER_OVERLAY);
    gtk_layer_set_namespace(GTK_WINDOW(window), "hyprwave");
    layout_setup_window_anchors(GTK_WINDOW(window), state->layout);
    gtk_layer_set_keyboard_mode(GTK_WINDOW(window), GTK_LAYER_SHELL_KEYBOARD_MODE_NONE);
    gtk_layer_set_exclusive_zone(GTK_WINDOW(window), 0);
    gtk_widget_set_name(window, "hyprwave-window");
    gtk_widget_add_css_class(window, "hyprwave-window");
    
    // Album cover setup
    GtkWidget *album_cover = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    state->album_cover = album_cover;
    gtk_widget_add_css_class(album_cover, "album-cover");
    gtk_widget_set_size_request(album_cover, 240, 240);
    gtk_widget_set_halign(album_cover, GTK_ALIGN_CENTER);
    gtk_widget_set_valign(album_cover, GTK_ALIGN_CENTER);
    gtk_widget_set_hexpand(album_cover, FALSE);
    gtk_widget_set_vexpand(album_cover, FALSE);
    gtk_widget_set_overflow(album_cover, GTK_OVERFLOW_HIDDEN);
    
    GtkGesture *double_click = gtk_gesture_click_new();
    gtk_gesture_single_set_button(GTK_GESTURE_SINGLE(double_click), GDK_BUTTON_PRIMARY);
    gtk_widget_add_controller(album_cover, GTK_EVENT_CONTROLLER(double_click));
    g_signal_connect(double_click, "pressed", G_CALLBACK(on_album_double_click), state);
    
    GtkWidget *source_label = gtk_label_new("No Source");
    state->source_label = source_label;
    gtk_widget_add_css_class(source_label, "source-label");

    // Hi-Fi: Format label for bitrate/quality display
    GtkWidget *format_label = gtk_label_new("");
    state->format_label = format_label;
    gtk_widget_add_css_class(format_label, "format-label");
    gtk_widget_set_visible(format_label, FALSE);

    // Hi-Fi: Player label with click-to-switch
    GtkWidget *player_label = gtk_label_new("Click to switch");
    state->player_label = player_label;
    gtk_widget_add_css_class(player_label, "player-label");
    GtkGesture *player_click = gtk_gesture_click_new();
    gtk_gesture_single_set_button(GTK_GESTURE_SINGLE(player_click), GDK_BUTTON_PRIMARY);
    gtk_widget_add_controller(player_label, GTK_EVENT_CONTROLLER(player_click));
    g_signal_connect(player_click, "pressed", G_CALLBACK(on_player_clicked), state);

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

    GtkWidget *progress_bar = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0.0, 1.0, 0.001);
    state->progress_bar = progress_bar;
    gtk_widget_add_css_class(progress_bar, "track-progress");
    gtk_scale_set_draw_value(GTK_SCALE(progress_bar), FALSE);
    gtk_widget_set_size_request(progress_bar, 140, 14);
    g_signal_connect(progress_bar, "change-value", G_CALLBACK(on_change_value), state);
    GtkEventController *controller = gtk_event_controller_legacy_new();
    g_signal_connect(controller, "event", G_CALLBACK(on_button_release_event), state);
    gtk_widget_add_controller(progress_bar, controller);

    GtkWidget *time_remaining = gtk_label_new("--:--");
    state->time_remaining = time_remaining;
    gtk_widget_add_css_class(time_remaining, "time-remaining");

    ExpandedWidgets expanded_widgets = {
        .album_cover = album_cover, .source_label = source_label,
        .format_label = format_label, .player_label = player_label,
        .track_title = track_title, .artist_label = artist_label,
        .progress_bar = progress_bar, .time_remaining = time_remaining,
        .visualizer_box = NULL  // Will be created by layout_create_expanded_section
    };
    GtkWidget *expanded_section = layout_create_expanded_section(state->layout, &expanded_widgets);
    state->visualizer_box = expanded_widgets.visualizer_box;  // Store reference

    // Initialize volume
    state->volume = volume_init(NULL, state->layout->is_vertical);

    GtkWidget *expanded_with_volume;
    if (state->layout->is_vertical) {
        expanded_with_volume = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
        gtk_box_append(GTK_BOX(expanded_with_volume), state->volume->revealer);
        gtk_box_append(GTK_BOX(expanded_with_volume), expanded_section);
        gtk_widget_set_size_request(expanded_with_volume, -1, 160);
        gtk_widget_set_vexpand(expanded_section, TRUE);
        gtk_widget_set_vexpand(state->volume->revealer, FALSE);
    } else {
        expanded_with_volume = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
        gtk_box_append(GTK_BOX(expanded_with_volume), expanded_section);
        gtk_box_append(GTK_BOX(expanded_with_volume), state->volume->revealer);
    }
    
    state->expanded_with_volume = expanded_with_volume;
    g_signal_connect(state->volume->revealer, "notify::child-revealed",
                     G_CALLBACK(on_volume_visibility_changed), state);

    GtkWidget *revealer = gtk_revealer_new();
    state->revealer = revealer;
    gtk_revealer_set_transition_type(GTK_REVEALER(revealer), layout_get_transition_type(state->layout));
    gtk_revealer_set_transition_duration(GTK_REVEALER(revealer), 300);
    gtk_revealer_set_child(GTK_REVEALER(revealer), expanded_with_volume);
    gtk_revealer_set_reveal_child(GTK_REVEALER(revealer), FALSE);
    g_signal_connect(revealer, "notify::child-revealed", G_CALLBACK(on_revealer_transition_done), state);

    // ========================================
    // CONTROL BUTTONS - Create all buttons
    // ========================================
    GtkWidget *prev_btn = gtk_button_new();
    gtk_widget_set_size_request(prev_btn, 36, 36);
    gchar *prev_icon_path = get_icon_path("previous.svg");
    GtkWidget *prev_icon = gtk_image_new_from_file(prev_icon_path);
    free_path(prev_icon_path);
    gtk_image_set_pixel_size(GTK_IMAGE(prev_icon), 20);
    gtk_button_set_child(GTK_BUTTON(prev_btn), prev_icon);
    gtk_widget_add_css_class(prev_btn, "control-button");
    gtk_widget_add_css_class(prev_btn, "prev-button");
    g_signal_connect(prev_btn, "clicked", G_CALLBACK(on_prev_clicked), state);

    GtkWidget *play_btn = gtk_button_new();
    gtk_widget_set_size_request(play_btn, 36, 36);
    gchar *play_icon_path = get_icon_path("play.svg");
    GtkWidget *play_icon = gtk_image_new_from_file(play_icon_path);
    free_path(play_icon_path);
    state->play_icon = play_icon;
    gtk_image_set_pixel_size(GTK_IMAGE(play_icon), 20);
    gtk_button_set_child(GTK_BUTTON(play_btn), play_icon);
    gtk_widget_add_css_class(play_btn, "control-button");
    gtk_widget_add_css_class(play_btn, "play-button");
    g_signal_connect(play_btn, "clicked", G_CALLBACK(on_play_clicked), state);

    GtkWidget *next_btn = gtk_button_new();
    gtk_widget_set_size_request(next_btn, 36, 36);
    gchar *next_icon_path = get_icon_path("next.svg");
    GtkWidget *next_icon = gtk_image_new_from_file(next_icon_path);
    free_path(next_icon_path);
    gtk_image_set_pixel_size(GTK_IMAGE(next_icon), 20);
    gtk_button_set_child(GTK_BUTTON(next_btn), next_icon);
    gtk_widget_add_css_class(next_btn, "control-button");
    gtk_widget_add_css_class(next_btn, "next-button");
    g_signal_connect(next_btn, "clicked", G_CALLBACK(on_next_clicked), state);

    GtkWidget *expand_btn = gtk_button_new();
    gtk_widget_set_size_request(expand_btn, 36, 36);
    const gchar *initial_icon_name = layout_get_expand_icon(state->layout, FALSE);
    gchar *expand_icon_path = get_icon_path(initial_icon_name);
    GtkWidget *expand_icon = gtk_image_new_from_file(expand_icon_path);
    free_path(expand_icon_path);
    state->expand_icon = expand_icon;
    gtk_image_set_pixel_size(GTK_IMAGE(expand_icon), 20);
    gtk_button_set_child(GTK_BUTTON(expand_btn), expand_icon);
    gtk_widget_add_css_class(expand_btn, "control-button");
    gtk_widget_add_css_class(expand_btn, "expand-button");
    g_signal_connect(expand_btn, "clicked", G_CALLBACK(on_expand_clicked), state);
    
    // Store button references in state
    state->prev_btn = prev_btn;
    state->play_btn = play_btn;
    state->next_btn = next_btn;
    state->expand_btn = expand_btn;

    // ========================================
    // CONTROL BAR SETUP
    // ========================================
    GtkWidget *control_bar = layout_create_control_bar(state->layout,
        &prev_btn, &play_btn, &next_btn, &expand_btn);
    state->control_bar_container = control_bar;

    // ========================================
    // VISUALIZER SETUP (in expanded section)
    // ========================================
    if (state->layout->visualizer_enabled && state->visualizer_box) {
        // Create visualizer (horizontal bars for vertical layout, vertical for horizontal)
        state->visualizer = visualizer_init(!state->layout->is_vertical);

        if (state->visualizer) {
            // Add visualizer container to the expanded section's visualizer_box
            gtk_box_append(GTK_BOX(state->visualizer_box), state->visualizer->container);
            gtk_widget_set_hexpand(state->visualizer->container, TRUE);
            gtk_widget_set_vexpand(state->visualizer->container, TRUE);

            // Start hidden (will show when expanded)
            gtk_widget_set_visible(state->visualizer->container, TRUE);
            gtk_widget_set_opacity(state->visualizer->container, 1.0);
            state->visualizer->fade_opacity = 1.0;
            state->visualizer->is_showing = FALSE;

            g_print("✓ Visualizer added to expanded section\n");
        }
    } else {
        state->visualizer = NULL;
    }
    
    // Create main container
    GtkWidget *main_container = layout_create_main_container(state->layout,
        control_bar, revealer);

    // ========================================
    // WINDOW REVEALER
    // ========================================
    GtkWidget *window_revealer = gtk_revealer_new();
    state->window_revealer = window_revealer;
    
    GtkRevealerTransitionType window_transition;
    if (state->layout->edge == EDGE_RIGHT) {
        window_transition = GTK_REVEALER_TRANSITION_TYPE_SLIDE_LEFT;
    } else if (state->layout->edge == EDGE_LEFT) {
        window_transition = GTK_REVEALER_TRANSITION_TYPE_SLIDE_RIGHT;
    } else if (state->layout->edge == EDGE_TOP) {
        window_transition = GTK_REVEALER_TRANSITION_TYPE_SLIDE_DOWN;
    } else {
        window_transition = GTK_REVEALER_TRANSITION_TYPE_SLIDE_UP;
    }
    gtk_revealer_set_transition_type(GTK_REVEALER(window_revealer), window_transition);
    gtk_revealer_set_transition_duration(GTK_REVEALER(window_revealer), 300);
    gtk_revealer_set_child(GTK_REVEALER(window_revealer), main_container);
    gtk_revealer_set_reveal_child(GTK_REVEALER(window_revealer), FALSE);
    g_signal_connect(window_revealer, "notify::child-revealed", 
                     G_CALLBACK(on_window_hide_complete), state);

    gtk_window_set_child(GTK_WINDOW(window), window_revealer);

    // ========================================
    // PRE-WARM REVEALERS (for smooth animations)
    // ========================================
    gtk_widget_realize(window);
    gtk_window_present(GTK_WINDOW(window));
    
    while (g_main_context_pending(NULL)) {
        g_main_context_iteration(NULL, FALSE);
    }
    
    guint window_duration = gtk_revealer_get_transition_duration(GTK_REVEALER(window_revealer));
    guint internal_duration = gtk_revealer_get_transition_duration(GTK_REVEALER(revealer));
    gtk_revealer_set_transition_duration(GTK_REVEALER(window_revealer), 0);
    gtk_revealer_set_transition_duration(GTK_REVEALER(revealer), 0);
    
    gtk_revealer_set_reveal_child(GTK_REVEALER(window_revealer), TRUE);
    gtk_widget_queue_allocate(window);
    while (g_main_context_pending(NULL)) {
        g_main_context_iteration(NULL, FALSE);
    }
    
    gtk_revealer_set_reveal_child(GTK_REVEALER(revealer), TRUE);
    gtk_widget_queue_allocate(window);
    while (g_main_context_pending(NULL)) {
        g_main_context_iteration(NULL, FALSE);
    }
    
    gtk_revealer_set_reveal_child(GTK_REVEALER(revealer), FALSE);
    gtk_widget_queue_allocate(window);
    while (g_main_context_pending(NULL)) {
        g_main_context_iteration(NULL, FALSE);
    }
    
    gtk_revealer_set_transition_duration(GTK_REVEALER(window_revealer), window_duration);
    gtk_revealer_set_transition_duration(GTK_REVEALER(revealer), internal_duration);
    
    // ========================================
    // FINALIZE
    // ========================================
    global_state = state;
    signal(SIGUSR1, handle_sigusr1);
    signal(SIGUSR2, handle_sigusr2);

    find_active_player(state);
    state->update_timer = g_timeout_add_seconds(1, update_position_tick, state);

    g_print("Layout: %s edge (%s)\n",
            state->layout->edge == EDGE_RIGHT ? "right" :
            state->layout->edge == EDGE_LEFT ? "left" :
            state->layout->edge == EDGE_TOP ? "top" : "bottom",
            state->layout->is_vertical ? "vertical" : "horizontal");
}


int main(int argc, char **argv) {
    GtkApplication *app = gtk_application_new("com.hyprwave.app", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
    g_signal_connect(app, "startup", G_CALLBACK(load_css), NULL);
    int status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);
    return status;
}
