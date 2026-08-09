#include "volume.h"
#include "paths.h"
#include "pipewire_volume.h"
#include <math.h>

/**
 * Volume Control with PipeWire Fallback
 *
 * Supports two volume control methods:
 * 1. PipeWire sink-input: Direct control of application audio stream
 * 2. MPRIS Volume property: Standard D-Bus interface
 *
 * The fallback chain (when volume_method=auto):
 * 1. Try PipeWire sink-input (works for Chromium/Electron apps)
 * 2. Fall back to MPRIS Volume (works for native players like Spotify)
 * 3. If neither works, volume slider is hidden
 */

// Forward declarations
static void init_pipewire_state(VolumeState *state);

/**
 * Format a 0.0-1.0 volume as a percentage label.
 *
 * Steps get finer towards the bottom of the range, so the label needs more
 * decimals there to show that a press did anything. Trailing zeros are
 * trimmed, so a level that lands on a whole percent still reads as "50%".
 */
static gchar* format_volume_percentage(gdouble volume) {
    gdouble percentage = volume * 100.0;
    gint decimals = percentage < 10.0 ? 2 : 1;

    // g_ascii_formatd rather than g_strdup_printf: the trimming below looks
    // for '.', which a comma-decimal locale would never produce.
    gchar buf[G_ASCII_DTOSTR_BUF_SIZE];
    gchar format[8];
    g_snprintf(format, sizeof(format), "%%.%df", decimals);
    g_ascii_formatd(buf, sizeof(buf), format, percentage);

    if (strchr(buf, '.')) {
        gchar *end = buf + strlen(buf) - 1;
        while (*end == '0') *end-- = '\0';
        if (*end == '.') *end = '\0';
    }

    return g_strdup_printf("%s%%", buf);
}

/**
 * Signed step for one press of the +/- buttons.
 *
 * A constant ratio (VOLUME_STEP_DB) rather than a constant amount, so the
 * change sounds the same size at 1% as it does at 80%. Capped at VOLUME_STEP
 * so a press never moves more than the slider's own step, and floored at
 * VOLUME_STEP_MIN so the step does not vanish as the level approaches zero.
 */
static gdouble volume_fine_step(gdouble volume, gboolean up) {
    gdouble factor = pow(10.0, (up ? VOLUME_STEP_DB : -VOLUME_STEP_DB) / 20.0);
    gdouble delta = fabs(volume * factor - volume);

    delta = CLAMP(delta, VOLUME_STEP_MIN, VOLUME_STEP);
    return up ? delta : -delta;
}

/**
 * Move one step. Returns FALSE when the level is already at the end of the
 * range, which is what stops a held button from repeating into the stop.
 */
static gboolean volume_step(VolumeState *state, gboolean up) {
    gdouble value = gtk_range_get_value(GTK_RANGE(state->slider));
    gdouble stepped = CLAMP(value + volume_fine_step(value, up), 0.0, 1.0);

    if (stepped == value) return FALSE;

    // Setting the range drives on_volume_changed, which applies the level,
    // relabels, and restarts the auto-hide timer.
    gtk_range_set_value(GTK_RANGE(state->slider), stepped);
    return TRUE;
}

/**
 * Press-and-hold state for one step button.
 *
 * A held button starts slow and accelerates, so a tap is one step and a hold
 * crosses the range without a separate coarse control. The throttle in
 * on_volume_changed coalesces the fast end of that into one backend call per
 * VOLUME_APPLY_INTERVAL_US, so the repeat rate is not limited by how long
 * pactl takes.
 */
typedef struct {
    VolumeState *state;
    gboolean up;
    guint timer;        // Delay before repeating, then the repeat itself
    guint interval_ms;  // Shrinks while held
} StepRepeat;

static void step_repeat_stop(StepRepeat *repeat) {
    if (repeat->timer > 0) {
        g_source_remove(repeat->timer);
        repeat->timer = 0;
    }
}

static gboolean step_repeat_tick(gpointer user_data) {
    StepRepeat *repeat = (StepRepeat *)user_data;

    if (!volume_step(repeat->state, repeat->up)) {
        repeat->timer = 0;  // Hit the end of the range
        return G_SOURCE_REMOVE;
    }

    guint next_ms = MAX(repeat->interval_ms * VOLUME_REPEAT_DECAY_NUM / VOLUME_REPEAT_DECAY_DEN,
                        VOLUME_REPEAT_MIN_MS);
    if (next_ms == repeat->interval_ms) {
        return G_SOURCE_CONTINUE;  // Fully accelerated, keep this source
    }

    // Timeout intervals are fixed at creation, so speeding up means a new one
    repeat->interval_ms = next_ms;
    repeat->timer = g_timeout_add(next_ms, step_repeat_tick, repeat);
    return G_SOURCE_REMOVE;
}

static gboolean step_repeat_begin(gpointer user_data) {
    StepRepeat *repeat = (StepRepeat *)user_data;

    repeat->interval_ms = VOLUME_REPEAT_START_MS;
    repeat->timer = g_timeout_add(repeat->interval_ms, step_repeat_tick, repeat);
    return G_SOURCE_REMOVE;
}

static void step_repeat_press(StepRepeat *repeat) {
    step_repeat_stop(repeat);
    volume_step(repeat->state, repeat->up);
    repeat->timer = g_timeout_add(VOLUME_REPEAT_DELAY_MS, step_repeat_begin, repeat);
}

static void on_step_pressed(GtkGestureClick *gesture, gint n_press,
                            gdouble x, gdouble y, gpointer user_data) {
    StepRepeat *repeat = (StepRepeat *)user_data;

    step_repeat_press(repeat);
    gtk_widget_add_css_class(gtk_event_controller_get_widget(
        GTK_EVENT_CONTROLLER(gesture)), "held");

    // Claiming the sequence keeps GtkButton's own click gesture from also
    // firing on release, which would step twice per press.
    gtk_gesture_set_state(GTK_GESTURE(gesture), GTK_EVENT_SEQUENCE_CLAIMED);
}

static void on_step_release(GtkGesture *gesture, GdkEventSequence *sequence, gpointer user_data) {
    step_repeat_stop((StepRepeat *)user_data);
    gtk_widget_remove_css_class(gtk_event_controller_get_widget(
        GTK_EVENT_CONTROLLER(gesture)), "held");
}

static void step_repeat_free(gpointer data) {
    StepRepeat *repeat = (StepRepeat *)data;

    step_repeat_stop(repeat);
    g_free(repeat);
}

static GtkWidget* create_step_button(const gchar *label, gboolean up, VolumeState *state) {
    GtkWidget *button = gtk_button_new_with_label(label);

    gtk_widget_add_css_class(button, "volume-step");
    gtk_widget_set_valign(button, GTK_ALIGN_CENTER);
    gtk_widget_set_halign(button, GTK_ALIGN_CENTER);
    gtk_widget_set_focus_on_click(button, FALSE);

    StepRepeat *repeat = g_new0(StepRepeat, 1);
    repeat->state = state;
    repeat->up = up;
    g_object_set_data_full(G_OBJECT(button), "step-repeat", repeat, step_repeat_free);

    // Capture phase, so the press is seen before GtkButton's own gesture and
    // can claim the sequence. The "held" class stands in for :active, which
    // that claim suppresses.
    GtkGesture *gesture = gtk_gesture_click_new();
    gtk_event_controller_set_propagation_phase(GTK_EVENT_CONTROLLER(gesture), GTK_PHASE_CAPTURE);
    g_signal_connect(gesture, "pressed", G_CALLBACK(on_step_pressed), repeat);
    g_signal_connect(gesture, "end", G_CALLBACK(on_step_release), repeat);
    g_signal_connect(gesture, "cancel", G_CALLBACK(on_step_release), repeat);
    gtk_widget_add_controller(button, GTK_EVENT_CONTROLLER(gesture));

    return button;
}

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

/**
 * Push the pending level to whichever backend this player uses.
 *
 * pw_set_volume spawns pactl synchronously on the main thread, so callers go
 * through the throttle in on_volume_changed rather than calling this directly.
 */
static void apply_pending_volume(VolumeState *state) {
    state->last_apply_us = g_get_monotonic_time();

    // Try PipeWire first if available
    if (state->use_pipewire_volume && state->pw_sink_input_index >= 0) {
        if (pw_set_volume(state->pw_sink_input_index, state->pending_volume)) {
            return;
        }
        // PipeWire failed, sink-input may have changed - try to refresh
        g_print("Volume: PipeWire set failed, refreshing sink-input\n");
        if (state->mpris_bus_name) {
            state->pw_sink_input_index = pw_find_sink_input_for_player(state->mpris_bus_name);
            if (state->pw_sink_input_index >= 0) {
                pw_set_volume(state->pw_sink_input_index, state->pending_volume);
            }
        }
        return;
    }

    // Fall back to MPRIS
    if (!state->mpris_proxy) return;

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
}

static gboolean delayed_volume_set(gpointer user_data) {
    VolumeState *state = (VolumeState *)user_data;

    state->pending_set_timer = 0;
    apply_pending_volume(state);
    return G_SOURCE_REMOVE;
}

static void on_volume_changed(GtkRange *range, gpointer user_data) {
    VolumeState *state = (VolumeState *)user_data;
    gdouble value = gtk_range_get_value(range);

    state->pending_volume = value;

    // Throttle on the leading edge: apply at once when the backend has been
    // idle, otherwise let the already-pending timer carry the latest value.
    // Cancelling and rescheduling instead would starve the backend entirely
    // whenever changes arrive faster than the interval, which is exactly what
    // holding a step button or dragging the slider does.
    gint64 since_apply = g_get_monotonic_time() - state->last_apply_us;

    if (since_apply >= VOLUME_APPLY_INTERVAL_US) {
        if (state->pending_set_timer > 0) {
            g_source_remove(state->pending_set_timer);
            state->pending_set_timer = 0;
        }
        apply_pending_volume(state);
    } else if (state->pending_set_timer == 0) {
        guint remaining_ms = (guint)((VOLUME_APPLY_INTERVAL_US - since_apply) / 1000);
        state->pending_set_timer = g_timeout_add(remaining_ms, delayed_volume_set, state);
    }

    // Update UI immediately for responsive feel
    volume_update_icon(state, (gint)round(value * 100));

    gchar *text = format_volume_percentage(value);
    gtk_label_set_text(GTK_LABEL(state->percentage), text);
    g_free(text);

    // Reset auto-hide timer
    reset_hide_timer(state);
}

/**
 * Initialize PipeWire state based on config and available sink-inputs.
 */
static void init_pipewire_state(VolumeState *state) {
    VolumeMethod method = get_config_volume_method();

    // Reset PipeWire state
    state->pw_sink_input_index = -1;
    state->use_pipewire_volume = FALSE;

    // If MPRIS-only mode, skip PipeWire entirely
    if (method == VOLUME_METHOD_MPRIS) {
        g_print("Volume: Using MPRIS-only mode (config)\n");
        return;
    }

    // Check if pactl is available
    if (!pw_is_pactl_available()) {
        g_print("Volume: pactl not available, using MPRIS\n");
        return;
    }

    // Try to find sink-input for this player
    if (state->mpris_bus_name) {
        state->pw_sink_input_index = pw_find_sink_input_for_player(state->mpris_bus_name);

        if (state->pw_sink_input_index >= 0) {
            state->use_pipewire_volume = TRUE;
            g_print("Volume: Using PipeWire sink-input #%d for %s\n",
                    state->pw_sink_input_index, state->mpris_bus_name);
        } else if (method == VOLUME_METHOD_PIPEWIRE) {
            // PipeWire-only mode but no sink-input found
            g_print("Volume: PipeWire mode requested but no sink-input found for %s\n",
                    state->mpris_bus_name);
        } else {
            // Auto mode - will fall back to MPRIS
            g_print("Volume: No PipeWire sink-input found, falling back to MPRIS\n");
        }
    }
}

VolumeState* volume_init(GDBusProxy *mpris_proxy, const gchar *mpris_bus_name, gboolean is_vertical) {
    VolumeState *state = g_new0(VolumeState, 1);
    state->mpris_proxy = mpris_proxy;
    state->mpris_bus_name = g_strdup(mpris_bus_name);
    state->is_showing = FALSE;
    state->hide_timer = 0;
    state->pending_set_timer = 0;
    state->pending_volume = 0.5;
    state->pw_sink_input_index = -1;
    state->use_pipewire_volume = FALSE;

    // Initialize PipeWire state
    init_pipewire_state(state);

    // Get current volume (uses PipeWire or MPRIS depending on state)
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
        0.0, 1.0, VOLUME_STEP  // 0.5% increments (arrow keys and scroll wheel)
    );
    state->slider = slider;
    // gtk_scale_new_with_range derives the page increment from the step, which
    // would shrink Page Up/Down to 5%. Keep the coarse jump at 10%.
    gtk_adjustment_set_page_increment(gtk_range_get_adjustment(GTK_RANGE(slider)), 0.1);
    gtk_widget_add_css_class(slider, "volume-slider");
    gtk_scale_set_draw_value(GTK_SCALE(slider), FALSE);
    gtk_range_set_value(GTK_RANGE(slider), state->current_volume);

    if (is_vertical) {
        gtk_widget_set_size_request(slider, 96, 24);
    } else {
        gtk_widget_set_size_request(slider, 24, 84);
        gtk_range_set_inverted(GTK_RANGE(slider), TRUE);  // Top = high volume
    }

    g_signal_connect(slider, "value-changed", G_CALLBACK(on_volume_changed), state);

    // Fine adjustment buttons. Dragging resolves to about a percent per pixel,
    // which is useless for players that only use the bottom of the range.
    state->step_down = create_step_button("−", FALSE, state);
    state->step_up = create_step_button("+", TRUE, state);

    // Percentage label
    gchar *percentage_text = format_volume_percentage(state->current_volume);
    GtkWidget *percentage = gtk_label_new(percentage_text);
    g_free(percentage_text);
    state->percentage = percentage;
    gtk_widget_add_css_class(percentage, "volume-percentage");

    // Pack widgets. The step buttons flank the slider on the side each one
    // moves towards: the vertical slider is inverted, so up is above it.
    gtk_box_append(GTK_BOX(container), icon);
    gtk_box_append(GTK_BOX(container), is_vertical ? state->step_down : state->step_up);
    gtk_box_append(GTK_BOX(container), slider);
    gtk_box_append(GTK_BOX(container), is_vertical ? state->step_up : state->step_down);
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

void volume_update_player(VolumeState *state, GDBusProxy *mpris_proxy, const gchar *mpris_bus_name) {
    if (!state) return;

    // Update MPRIS proxy
    state->mpris_proxy = mpris_proxy;

    // Update bus name
    g_free(state->mpris_bus_name);
    state->mpris_bus_name = g_strdup(mpris_bus_name);

    // Reinitialize PipeWire state for new player
    init_pipewire_state(state);

    g_print("Volume: Updated player to %s (PipeWire: %s, sink-input: %d)\n",
            mpris_bus_name ? mpris_bus_name : "none",
            state->use_pipewire_volume ? "yes" : "no",
            state->pw_sink_input_index);
}

void volume_show(VolumeState *state) {
    if (!state || state->is_showing) return;

    // Update current volume (from PipeWire or MPRIS)
    state->current_volume = volume_get_current(state);

    // Block signal to prevent triggering on_volume_changed
    g_signal_handlers_block_by_func(state->slider, on_volume_changed, state);
    gtk_range_set_value(GTK_RANGE(state->slider), state->current_volume);
    g_signal_handlers_unblock_by_func(state->slider, on_volume_changed, state);

    volume_update_icon(state, (gint)round(state->current_volume * 100));

    gchar *text = format_volume_percentage(state->current_volume);
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
    // Clamp to 0.0 - 1.0
    if (volume < 0.0) volume = 0.0;
    if (volume > 1.0) volume = 1.0;

    state->current_volume = volume;
    state->pending_volume = volume;

    // Try PipeWire first
    if (state->use_pipewire_volume && state->pw_sink_input_index >= 0) {
        if (pw_set_volume(state->pw_sink_input_index, volume)) {
            return;
        }
    }

    // Fall back to MPRIS
    if (!state->mpris_proxy) return;

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
    // Try PipeWire first
    if (state->use_pipewire_volume && state->pw_sink_input_index >= 0) {
        gdouble vol = pw_get_volume(state->pw_sink_input_index);
        if (vol >= 0.0) {
            return vol > 1.0 ? 1.0 : vol;  // Cap at 100% for display
        }
        // PipeWire failed, sink-input may have changed
        g_print("Volume: PipeWire get failed, refreshing sink-input\n");
        if (state->mpris_bus_name) {
            state->pw_sink_input_index = pw_find_sink_input_for_player(state->mpris_bus_name);
            if (state->pw_sink_input_index >= 0) {
                vol = pw_get_volume(state->pw_sink_input_index);
                if (vol >= 0.0) {
                    return vol > 1.0 ? 1.0 : vol;
                }
            }
        }
    }

    // Fall back to MPRIS
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
    if (!state) return FALSE;

    // PipeWire volume is always "supported" if we have a valid sink-input
    if (state->use_pipewire_volume && state->pw_sink_input_index >= 0) {
        return TRUE;
    }

    // Check MPRIS support
    if (!state->mpris_proxy) return FALSE;

    // For auto mode, check if MPRIS volume works
    VolumeMethod method = get_config_volume_method();
    if (method == VOLUME_METHOD_PIPEWIRE) {
        // PipeWire-only mode but no sink-input
        return FALSE;
    }

    // Check player name - some players don't support MPRIS volume control
    const gchar *name = g_dbus_proxy_get_name(state->mpris_proxy);
    if (name) {
        // Chromium-based players expose Volume but don't respond to changes
        // However, if we're in auto mode and PipeWire is available, we already tried it
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

    g_free(state->mpris_bus_name);
    g_free(state);
}
