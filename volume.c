#include "volume.h"
#include "paths.h"
#include <math.h>

static gboolean auto_hide_volume(gpointer user_data) {
    VolumeState *state = (VolumeState *)user_data;
    volume_hide(state);
    state->hide_timer = 0;
    return G_SOURCE_REMOVE;
}

static void reset_hide_timer(VolumeState *state) {
    if (state->hide_timer > 0) {
        g_source_remove(state->hide_timer);
    }
    state->hide_timer = g_timeout_add_seconds(3, auto_hide_volume, state);
}

void volume_update_icon(VolumeState *state, gint percentage) {
    const gchar *icon_name;
    
    if (percentage == 0) {
        icon_name = "volume-mute.svg";
    } else if (percentage <= 25) {
        icon_name = "volume-low.svg";
    } else if (percentage <= 50) {
        icon_name = "volume-medium.svg";
    } else {
        icon_name = "volume-high.svg";
    }
    
    gchar *icon_path = get_icon_path(icon_name);
    gtk_image_set_from_file(GTK_IMAGE(state->icon), icon_path);
    free_path(icon_path);
}

// Throttled volume setter to prevent lag
static gboolean delayed_volume_set(gpointer user_data) {
    VolumeState *state = (VolumeState *)user_data;
    
    if (!state->mpris_proxy) {
        state->pending_set_timer = 0;
        return G_SOURCE_REMOVE;
    }
    
    // Set via MPRIS D-Bus property
    g_dbus_proxy_call(
        state->mpris_proxy,
        "org.freedesktop.DBus.Properties.Set",
        g_variant_new("(ssv)", 
            "org.mpris.MediaPlayer2.Player",
            "Volume",
            g_variant_new_double(state->pending_volume)),
        G_DBUS_CALL_FLAGS_NONE,
        -1,
        NULL,
        NULL,
        NULL
    );
    
    state->pending_set_timer = 0;
    return G_SOURCE_REMOVE;
}

static void on_volume_changed(GtkRange *range, gpointer user_data) {
    VolumeState *state = (VolumeState *)user_data;
    gdouble value = gtk_range_get_value(range);
    
    state->pending_volume = value;
    
    // Cancel any pending volume set
    if (state->pending_set_timer > 0) {
        g_source_remove(state->pending_set_timer);
    }
    
    // Schedule throttled volume set (100ms delay)
    state->pending_set_timer = g_timeout_add(100, delayed_volume_set, state);
    
    // Update UI immediately for responsive feel
    gint percentage = (gint)round(value * 100);
    volume_update_icon(state, percentage);
    
    gchar *text = g_strdup_printf("%d%%", percentage);
    gtk_label_set_text(GTK_LABEL(state->percentage), text);
    g_free(text);
    
    // Reset auto-hide timer
    reset_hide_timer(state);
}

VolumeState* volume_init(GDBusProxy *mpris_proxy, gboolean is_vertical) {
    VolumeState *state = g_new0(VolumeState, 1);
    state->mpris_proxy = mpris_proxy;
    state->is_showing = FALSE;
    state->hide_timer = 0;
    state->pending_set_timer = 0;
    state->pending_volume = 0.5;
    
    // Get current volume from MPRIS
    state->current_volume = volume_get_current(state);
    
    // Main container
    GtkOrientation orientation = is_vertical ? GTK_ORIENTATION_HORIZONTAL : GTK_ORIENTATION_VERTICAL;
    GtkWidget *container = gtk_box_new(orientation, 8);
    state->container = container;
    gtk_widget_add_css_class(container, "volume-container");
    gtk_widget_set_halign(container, GTK_ALIGN_CENTER);
    gtk_widget_set_valign(container, GTK_ALIGN_CENTER);
    
    // Speaker icon
    gint initial_percentage = (gint)round(state->current_volume * 100);
    const gchar *initial_icon = initial_percentage == 0 ? "volume-mute.svg" :
                                initial_percentage <= 25 ? "volume-low.svg" :
                                initial_percentage <= 50 ? "volume-medium.svg" :
                                "volume-high.svg";
    
    gchar *icon_path = get_icon_path(initial_icon);
    GtkWidget *icon = gtk_image_new_from_file(icon_path);
    free_path(icon_path);
    state->icon = icon;
    gtk_image_set_pixel_size(GTK_IMAGE(icon), 20);
    gtk_widget_add_css_class(icon, "volume-icon");
    
    // Volume slider
    GtkWidget *slider = gtk_scale_new_with_range(
        is_vertical ? GTK_ORIENTATION_HORIZONTAL : GTK_ORIENTATION_VERTICAL,
        0.0, 1.0, 0.01  // 1% increments for smoother control
    );
    state->slider = slider;
    gtk_widget_add_css_class(slider, "volume-slider");
    gtk_scale_set_draw_value(GTK_SCALE(slider), FALSE);
    gtk_range_set_value(GTK_RANGE(slider), state->current_volume);
    
    if (is_vertical) {
        gtk_widget_set_size_request(slider, 120, 24);
    } else {
        gtk_widget_set_size_request(slider, 24, 100);
        gtk_range_set_inverted(GTK_RANGE(slider), TRUE);  // Top = high volume
    }
    
    g_signal_connect(slider, "value-changed", G_CALLBACK(on_volume_changed), state);
    
    // Percentage label
    gchar *percentage_text = g_strdup_printf("%d%%", initial_percentage);
    GtkWidget *percentage = gtk_label_new(percentage_text);
    g_free(percentage_text);
    state->percentage = percentage;
    gtk_widget_add_css_class(percentage, "volume-percentage");
    
    // Pack widgets
    gtk_box_append(GTK_BOX(container), icon);
    gtk_box_append(GTK_BOX(container), slider);
    gtk_box_append(GTK_BOX(container), percentage);
    
    // Wrap in revealer for animation
    GtkWidget *revealer = gtk_revealer_new();
    state->revealer = revealer;
    gtk_revealer_set_transition_type(GTK_REVEALER(revealer),
        is_vertical ? GTK_REVEALER_TRANSITION_TYPE_SLIDE_UP :
                      GTK_REVEALER_TRANSITION_TYPE_SLIDE_LEFT);
    gtk_revealer_set_transition_duration(GTK_REVEALER(revealer), 250);
    gtk_revealer_set_child(GTK_REVEALER(revealer), container);
    gtk_revealer_set_reveal_child(GTK_REVEALER(revealer), FALSE);
    
    return state;
}

void volume_show(VolumeState *state) {
    if (!state || state->is_showing) return;
    
    // Update current volume from MPRIS
    state->current_volume = volume_get_current(state);
    
    // Block signal to prevent triggering on_volume_changed
    g_signal_handlers_block_by_func(state->slider, on_volume_changed, state);
    gtk_range_set_value(GTK_RANGE(state->slider), state->current_volume);
    g_signal_handlers_unblock_by_func(state->slider, on_volume_changed, state);
    
    gint percentage = (gint)round(state->current_volume * 100);
    volume_update_icon(state, percentage);
    
    gchar *text = g_strdup_printf("%d%%", percentage);
    gtk_label_set_text(GTK_LABEL(state->percentage), text);
    g_free(text);
    
    // Show with animation
    state->is_showing = TRUE;
    gtk_revealer_set_reveal_child(GTK_REVEALER(state->revealer), TRUE);
    
    // Start auto-hide timer
    reset_hide_timer(state);
}

void volume_hide(VolumeState *state) {
    if (!state || !state->is_showing) return;
    
    state->is_showing = FALSE;
    
    // Cancel hide timer
    if (state->hide_timer > 0) {
        g_source_remove(state->hide_timer);
        state->hide_timer = 0;
    }
    
    // Cancel any pending volume set
    if (state->pending_set_timer > 0) {
        g_source_remove(state->pending_set_timer);
        state->pending_set_timer = 0;
    }
    
    // Hide with animation
    gtk_revealer_set_reveal_child(GTK_REVEALER(state->revealer), FALSE);
}

void volume_set(VolumeState *state, gdouble volume) {
    if (!state->mpris_proxy) return;
    
    // Clamp to 0.0 - 1.0
    if (volume < 0.0) volume = 0.0;
    if (volume > 1.0) volume = 1.0;
    
    state->current_volume = volume;
    state->pending_volume = volume;
    
    // Set via MPRIS D-Bus property
    g_dbus_proxy_call(
        state->mpris_proxy,
        "org.freedesktop.DBus.Properties.Set",
        g_variant_new("(ssv)", 
            "org.mpris.MediaPlayer2.Player",
            "Volume",
            g_variant_new_double(volume)),
        G_DBUS_CALL_FLAGS_NONE,
        -1,
        NULL,
        NULL,
        NULL
    );
}

gdouble volume_get_current(VolumeState *state) {
    if (!state->mpris_proxy) return 0.5;  // Default to 50%

    GVariant *volume_var = g_dbus_proxy_get_cached_property(
        state->mpris_proxy, "Volume");

    if (volume_var) {
        gdouble volume = g_variant_get_double(volume_var);
        g_variant_unref(volume_var);
        return volume;
    }

    return 0.5;  // Default fallback
}

gboolean volume_is_supported(VolumeState *state) {
    if (!state || !state->mpris_proxy) return FALSE;

    // Check player name - some players don't support MPRIS volume control
    const gchar *name = g_dbus_proxy_get_name(state->mpris_proxy);
    if (name) {
        // Chromium-based players expose Volume but don't respond to changes
        if (g_strstr_len(name, -1, "chromium") != NULL) return FALSE;
        // Roon MPRIS bridge doesn't support volume control
        if (g_strstr_len(name, -1, "roon") != NULL) return FALSE;
    }

    // Check if Volume property exists and has a valid value
    GVariant *volume_var = g_dbus_proxy_get_cached_property(
        state->mpris_proxy, "Volume");

    if (volume_var) {
        gdouble volume = g_variant_get_double(volume_var);
        g_variant_unref(volume_var);
        // Volume=0 often indicates no volume control
        return volume > 0.0;
    }

    return FALSE;
}

void volume_cleanup(VolumeState *state) {
    if (!state) return;
    
    if (state->hide_timer > 0) {
        g_source_remove(state->hide_timer);
    }
    
    if (state->pending_set_timer > 0) {
        g_source_remove(state->pending_set_timer);
    }
    
    g_free(state);
}
