#include "visualizer.h"
#include "pipewire_volume.h"
#include <math.h>
#include <string.h>
#include <spa/param/props.h>
#include <spa/pod/builder.h>
#include <spa/pod/parser.h>

#define SMOOTHING_FACTOR 0.7
#define AGC_ATTACK 0.9      // Fast attack - quickly respond to louder audio
#define AGC_DECAY 0.9995    // Very slow decay - maintain level during quiet parts
#define AGC_MIN_THRESHOLD 0.0001  // Minimum level to avoid amplifying silence

// Cached audio node info for searching when player changes
typedef struct {
    guint32 id;
    guint32 pid;
    gchar *name;
    gchar *app_name;
} AudioNodeInfo;

static void audio_node_info_free(gpointer data) {
    AudioNodeInfo *info = (AudioNodeInfo *)data;
    if (info) {
        g_free(info->name);
        g_free(info->app_name);
        g_free(info);
    }
}

// Check if child_pid is a descendant of parent_pid
static gboolean is_descendant_of(guint32 child_pid, guint32 parent_pid) {
    if (child_pid == 0 || parent_pid == 0) return FALSE;
    if (child_pid == parent_pid) return TRUE;

    // Read /proc/<child_pid>/stat to get parent PID
    gchar *stat_path = g_strdup_printf("/proc/%u/stat", child_pid);
    gchar *contents = NULL;
    gsize length;

    if (!g_file_get_contents(stat_path, &contents, &length, NULL)) {
        g_free(stat_path);
        return FALSE;
    }
    g_free(stat_path);

    // Parse stat file - format: pid (comm) state ppid ...
    // Find the closing parenthesis of comm, then skip to ppid
    gchar *close_paren = g_strrstr(contents, ")");
    if (!close_paren) {
        g_free(contents);
        return FALSE;
    }

    guint32 ppid = 0;
    if (sscanf(close_paren + 2, "%*c %u", &ppid) != 1) {
        g_free(contents);
        return FALSE;
    }
    g_free(contents);

    if (ppid == parent_pid) return TRUE;
    if (ppid <= 1) return FALSE;  // Reached init/systemd

    // Recurse up the tree
    return is_descendant_of(ppid, parent_pid);
}

// Search cached nodes for matching PID and connect if found
static void search_cached_nodes_for_target(VisualizerState *state);

/**
 * PipeWire Per-Player Audio Visualizer
 *
 * This implementation captures audio from a specific player (identified by PID)
 * using PipeWire's native API. It includes AGC to make the visualization
 * independent of volume level.
 *
 * Architecture:
 * 1. pw_registry monitors for nodes matching the target PID
 * 2. When found, pw_stream connects to capture that node's audio
 * 3. Audio is processed with AGC normalization
 * 4. GTK widgets are updated from the main thread via render timer
 */

// Forward declarations
static void on_stream_process(void *userdata);
static void on_stream_state_changed(void *userdata, enum pw_stream_state old,
                                    enum pw_stream_state state, const char *error);
static void on_registry_global(void *data, uint32_t id, uint32_t permissions,
                               const char *type, uint32_t version,
                               const struct spa_dict *props);
static void on_registry_global_remove(void *data, uint32_t id);
static void connect_to_target(VisualizerState *state);
static void disconnect_stream(VisualizerState *state);

// PipeWire stream events
static const struct pw_stream_events stream_events = {
    PW_VERSION_STREAM_EVENTS,
    .state_changed = on_stream_state_changed,
    .process = on_stream_process,
};

// PipeWire registry events
static const struct pw_registry_events registry_events = {
    PW_VERSION_REGISTRY_EVENTS,
    .global = on_registry_global,
    .global_remove = on_registry_global_remove,
};

// Easing function for smooth transitions
static gdouble ease_out_sine(gdouble t) {
    return sin(t * M_PI / 6.0);
}

// Process audio samples with AGC normalization
// Handles stereo input by averaging channels
static void process_audio_samples(VisualizerState *state, const float *samples, size_t n_samples) {
    if (n_samples == 0) return;

    // n_samples is total floats, for stereo that's n_samples/2 frames
    // We'll process as interleaved stereo and average the channels
    size_t n_frames = n_samples / 2;  // Stereo frames
    if (n_frames == 0) n_frames = n_samples;  // Fallback for mono

    // Find peak in this buffer (average of L+R)
    float buffer_peak = 0.0f;
    for (size_t i = 0; i < n_samples; i += 2) {
        float left = fabsf(samples[i]);
        float right = (i + 1 < n_samples) ? fabsf(samples[i + 1]) : left;
        float avg = (left + right) * 0.5f;
        if (avg > buffer_peak) {
            buffer_peak = avg;
        }
    }

    // Update AGC peak with attack/decay
    if (buffer_peak > state->agc_peak) {
        // Attack: quickly rise to new peak
        state->agc_peak = AGC_ATTACK * state->agc_peak + (1.0 - AGC_ATTACK) * buffer_peak;
    } else {
        // Decay: slowly fall when audio is quieter
        state->agc_peak = AGC_DECAY * state->agc_peak;
    }

    // Ensure minimum threshold to avoid division by zero or amplifying silence
    gdouble effective_peak = state->agc_peak;
    if (effective_peak < AGC_MIN_THRESHOLD) {
        effective_peak = AGC_MIN_THRESHOLD;
    }

    // Calculate gain factor (normalize to ~1.0 peak)
    gdouble gain = 1.0 / effective_peak;

    // Process into frequency bars (using stereo frames)
    size_t frames_per_bin = n_frames / VISUALIZER_BARS;
    if (frames_per_bin == 0) frames_per_bin = 1;

    for (int i = 0; i < VISUALIZER_BARS; i++) {
        gdouble sum = 0.0;
        size_t start_frame = i * frames_per_bin;
        size_t end_frame = start_frame + frames_per_bin;

        for (size_t f = start_frame; f < end_frame && f < n_frames; f++) {
            size_t idx = f * 2;  // Stereo interleaved
            float left = (idx < n_samples) ? samples[idx] : 0.0f;
            float right = (idx + 1 < n_samples) ? samples[idx + 1] : left;
            // Average L+R and apply gain
            gdouble mono = ((left + right) * 0.5) * gain;
            sum += mono * mono;
        }

        gdouble rms = sqrt(sum / frames_per_bin);

        // Scale for visualization (already normalized by AGC, so smaller multiplier)
        gdouble normalized = rms * 2.5;
        if (normalized > 1.0) normalized = 1.0;

        // Smooth the values
        state->bar_smoothed[i] = (SMOOTHING_FACTOR * state->bar_smoothed[i]) +
                                 ((1.0 - SMOOTHING_FACTOR) * normalized);

        state->bar_heights[i] = state->bar_smoothed[i];
    }
}

// PipeWire stream process callback - called when audio data is available
static void on_stream_process(void *userdata) {
    VisualizerState *state = (VisualizerState *)userdata;
    struct pw_buffer *buf;
    struct spa_buffer *spa_buf;
    float *samples;
    uint32_t n_samples;

    if ((buf = pw_stream_dequeue_buffer(state->pw_stream)) == NULL) {
        return;
    }

    spa_buf = buf->buffer;
    if (spa_buf->datas[0].data == NULL) {
        pw_stream_queue_buffer(state->pw_stream, buf);
        return;
    }

    samples = spa_buf->datas[0].data;
    n_samples = spa_buf->datas[0].chunk->size / sizeof(float);

    g_mutex_lock(&state->data_mutex);
    process_audio_samples(state, samples, n_samples);
    g_mutex_unlock(&state->data_mutex);

    pw_stream_queue_buffer(state->pw_stream, buf);
}

// Stream state change callback
static void on_stream_state_changed(void *userdata, enum pw_stream_state old,
                                    enum pw_stream_state new_state, const char *error) {
    VisualizerState *state = (VisualizerState *)userdata;

    switch (new_state) {
        case PW_STREAM_STATE_ERROR:
            g_printerr("Visualizer stream error: %s\n", error ? error : "unknown");
            break;
        case PW_STREAM_STATE_STREAMING:
            g_print("✓ Visualizer capturing from node %u (%s)\n",
                    state->target_node_id,
                    state->target_node_name ? state->target_node_name : "unknown");
            break;
        case PW_STREAM_STATE_PAUSED:
            g_print("Visualizer stream paused\n");
            break;
        default:
            break;
    }
}

// Registry global callback - called for each PipeWire object
static void on_registry_global(void *data, uint32_t id, uint32_t permissions,
                               const char *type, uint32_t version,
                               const struct spa_dict *props) {
    VisualizerState *state = (VisualizerState *)data;

    // Only interested in audio stream nodes
    if (strcmp(type, PW_TYPE_INTERFACE_Node) != 0) {
        return;
    }

    if (!props) return;

    // Check if this is an audio output (playback) stream
    const char *media_class = spa_dict_lookup(props, PW_KEY_MEDIA_CLASS);
    if (!media_class) {
        return;
    }

    // We want to capture from "Stream/Output/Audio" nodes (playback streams)
    if (strstr(media_class, "Stream/Output/Audio") == NULL) {
        return;  // Silently skip non-audio nodes
    }

    // Get object.serial - this corresponds to pactl sink-input index
    const char *serial_str = spa_dict_lookup(props, PW_KEY_OBJECT_SERIAL);
    if (!serial_str) {
        return;  // No serial, can't match
    }

    gint node_serial = atoi(serial_str);
    const char *node_name = spa_dict_lookup(props, PW_KEY_NODE_NAME);
    const char *app_name = spa_dict_lookup(props, PW_KEY_APP_NAME);

    // Log audio nodes only when we have a target
    if (state->target_serial > 0) {
        g_print("Audio node: id=%u serial=%d app='%s'\n",
                id, node_serial, app_name ? app_name : "?");
    }

    // Cache this audio node for later searching
    if (state->audio_nodes) {
        AudioNodeInfo *info = g_new0(AudioNodeInfo, 1);
        info->id = id;
        info->pid = (guint32)node_serial;  // Store serial in pid field for cache
        info->name = g_strdup(node_name);
        info->app_name = g_strdup(app_name);
        g_hash_table_insert(state->audio_nodes, GUINT_TO_POINTER(id), info);
    }

    // Check if this matches our target serial (sink-input index)
    if (state->target_serial <= 0) {
        return;
    }
    if (node_serial != state->target_serial) {
        return;
    }

    g_print("✓ Found target audio node: id=%u serial=%d app='%s'\n",
            id, node_serial, app_name ? app_name : "?");

    // Store target info
    state->target_node_id = id;
    g_free(state->target_node_name);
    state->target_node_name = g_strdup(app_name ? app_name : node_name);
    state->target_found = TRUE;

    // Connect stream to this node
    connect_to_target(state);
}

// Search cached nodes for matching serial and connect if found
static void search_cached_nodes_for_target(VisualizerState *state) {
    if (!state->audio_nodes || state->target_serial <= 0) return;

    GHashTableIter iter;
    gpointer key, value;
    g_hash_table_iter_init(&iter, state->audio_nodes);

    while (g_hash_table_iter_next(&iter, &key, &value)) {
        AudioNodeInfo *info = (AudioNodeInfo *)value;
        // info->pid is actually the serial number (we stored it there)
        if ((gint)info->pid == state->target_serial) {
            g_print("Found cached audio node for serial %d: id=%u app='%s'\n",
                    state->target_serial, info->id,
                    info->app_name ? info->app_name : "?");

            state->target_node_id = info->id;
            g_free(state->target_node_name);
            state->target_node_name = g_strdup(info->app_name ? info->app_name : info->name);
            state->target_found = TRUE;

            connect_to_target(state);
            return;
        }
    }

    g_print("No cached audio node found for serial %d (will connect when node appears)\n",
            state->target_serial);
}

// Registry global remove callback
static void on_registry_global_remove(void *data, uint32_t id) {
    VisualizerState *state = (VisualizerState *)data;

    // Remove from cache
    if (state->audio_nodes) {
        g_hash_table_remove(state->audio_nodes, GUINT_TO_POINTER(id));
    }

    if (id == state->target_node_id) {
        g_print("Target node %u removed, disconnecting visualizer\n", id);
        disconnect_stream(state);
        state->target_node_id = 0;
        state->target_found = FALSE;
    }
}

// Connect pw_stream to capture audio from a specific player's node
// Only connects if a target node has been found; no fallback to system audio
static void connect_to_target(VisualizerState *state) {
    if (!state->pw_stream) return;

    // Only connect to a specific player's audio node, never system-wide
    if (!state->target_found || state->target_node_id == 0) {
        g_print("Visualizer: No target node found, skipping connection\n");
        return;
    }

    // Disconnect existing connection first
    pw_stream_disconnect(state->pw_stream);

    // Build stream parameters for audio capture
    uint8_t buffer[1024];
    struct spa_pod_builder b = SPA_POD_BUILDER_INIT(buffer, sizeof(buffer));

    // Request float audio, stereo, 48kHz
    const struct spa_pod *params[1];
    params[0] = spa_format_audio_raw_build(&b, SPA_PARAM_EnumFormat,
            &SPA_AUDIO_INFO_RAW_INIT(
                .format = SPA_AUDIO_FORMAT_F32,
                .channels = 2,
                .rate = 48000));

    g_print("Visualizer: Connecting to player node %u '%s' (AGC-normalized)\n",
            state->target_node_id, state->target_node_name ? state->target_node_name : "?");

    // Update stream properties to request capture from a sink (not source)
    pw_stream_update_properties(state->pw_stream,
        &SPA_DICT_INIT_ARRAY(((struct spa_dict_item[]) {
            { PW_KEY_STREAM_CAPTURE_SINK, "true" },
            { PW_KEY_NODE_NAME, "hyprwave-visualizer" },
        })));

    pw_stream_connect(state->pw_stream,
                      PW_DIRECTION_INPUT,
                      state->target_node_id,
                      PW_STREAM_FLAG_AUTOCONNECT |
                      PW_STREAM_FLAG_RT_PROCESS |
                      PW_STREAM_FLAG_MAP_BUFFERS,
                      params, 1);
}

// Disconnect stream
static void disconnect_stream(VisualizerState *state) {
    if (state->pw_stream) {
        pw_stream_disconnect(state->pw_stream);
    }

    // Clear visualization
    g_mutex_lock(&state->data_mutex);
    for (int i = 0; i < VISUALIZER_BARS; i++) {
        state->bar_heights[i] = 0.0;
        state->bar_smoothed[i] = 0.0;
    }
    state->agc_peak = AGC_MIN_THRESHOLD;
    g_mutex_unlock(&state->data_mutex);
}

// Update visualizer bars (~60fps) - called from GTK main thread
static gboolean update_visualizer(gpointer user_data) {
    VisualizerState *state = (VisualizerState *)user_data;

    if (!state->is_showing) {
        return G_SOURCE_CONTINUE;
    }

    g_mutex_lock(&state->data_mutex);

    for (int i = 0; i < VISUALIZER_BARS; i++) {
        gint min_size = 1;
        gint max_size = state->is_vertical ? 50 : 24;

        // Decay to minimum if no audio
        if (state->bar_heights[i] < 0.01) {
            state->bar_heights[i] = 0.0;
        }

        // Calculate bar size
        gint bar_size = min_size + (gint)(state->bar_heights[i] * (max_size - min_size));

        // Update size based on orientation
        if (state->is_vertical) {
            gtk_widget_set_size_request(state->bars[i], bar_size, 3);
        } else {
            gtk_widget_set_size_request(state->bars[i], 3, bar_size);
        }

        gtk_widget_set_visible(state->bars[i], TRUE);
        gtk_widget_set_opacity(state->bars[i], bar_size <= min_size ? 0.0 : 1.0);
    }

    g_mutex_unlock(&state->data_mutex);

    return G_SOURCE_CONTINUE;
}

// Fade animation (for smooth show/hide)
static gboolean fade_visualizer(gpointer user_data) {
    VisualizerState *state = (VisualizerState *)user_data;

    if (state->is_showing) {
        state->fade_opacity += 0.025;  // Slower = smoother
        if (state->fade_opacity >= 1.0) {
            state->fade_opacity = 1.0;
            gtk_widget_set_opacity(state->container, 1.0);
            state->fade_timer = 0;
            return G_SOURCE_REMOVE;
        }
        // Apply easing for smooth fade-in
        gdouble eased = ease_out_sine(state->fade_opacity);
        gtk_widget_set_opacity(state->container, eased);
    } else {
        state->fade_opacity -= 0.05;
        if (state->fade_opacity <= 0.0) {
            state->fade_opacity = 0.0;
            state->fade_timer = 0;
            return G_SOURCE_REMOVE;
        }
        gtk_widget_set_opacity(state->container, state->fade_opacity);
    }


    return G_SOURCE_CONTINUE;
}

// Initialize visualizer
VisualizerState* visualizer_init(gboolean is_vertical) {
    // Initialize PipeWire library
    pw_init(NULL, NULL);

    VisualizerState *state = g_new0(VisualizerState, 1);
    state->is_showing = FALSE;
    state->is_running = FALSE;
    state->is_vertical = is_vertical;
    state->fade_opacity = 0.0;
    state->target_pid = 0;
    state->target_serial = -1;
    state->target_node_id = 0;
    state->target_node_name = NULL;
    state->target_found = FALSE;
    state->agc_peak = AGC_MIN_THRESHOLD;

    g_mutex_init(&state->data_mutex);

    // Create node cache for searching when player changes
    state->audio_nodes = g_hash_table_new_full(g_direct_hash, g_direct_equal,
                                                NULL, audio_node_info_free);

    // Zero out audio data
    for (int i = 0; i < VISUALIZER_BARS; i++) {
        state->bar_heights[i] = 0.0;
        state->bar_smoothed[i] = 0.0;
    }

    // Create container based on orientation
    GtkOrientation container_orient = is_vertical ? GTK_ORIENTATION_VERTICAL : GTK_ORIENTATION_HORIZONTAL;
    GtkWidget *container = gtk_box_new(container_orient, 0);
    state->container = container;

    gtk_widget_set_overflow(container, GTK_OVERFLOW_HIDDEN);

    if (is_vertical) {
        gtk_widget_set_halign(container, GTK_ALIGN_START);
        gtk_widget_set_valign(container, GTK_ALIGN_CENTER);
        gtk_widget_set_size_request(container, -1, 200);
    } else {
        gtk_widget_set_halign(container, GTK_ALIGN_CENTER);
        gtk_widget_set_valign(container, GTK_ALIGN_END);
        gtk_widget_set_size_request(container, 275, 32);
    }

    gtk_widget_set_hexpand(container, FALSE);
    gtk_widget_set_vexpand(container, FALSE);
    gtk_widget_add_css_class(container, "visualizer-container");

    g_print("✓ Visualizer container: %s layout (PipeWire per-player capture)\n",
            is_vertical ? "vertical" : "horizontal");

    // Create bars
    for (int i = 0; i < VISUALIZER_BARS; i++) {
        GtkOrientation bar_orient = is_vertical ? GTK_ORIENTATION_HORIZONTAL : GTK_ORIENTATION_VERTICAL;
        GtkWidget *bar = gtk_box_new(bar_orient, 0);
        state->bars[i] = bar;

        gtk_widget_add_css_class(bar, "visualizer-bar");
        gtk_widget_set_visible(bar, TRUE);

        if (is_vertical) {
            gtk_widget_set_size_request(bar, 3, -1);
            gtk_widget_set_halign(bar, GTK_ALIGN_START);
            gtk_widget_set_vexpand(bar, TRUE);
            gtk_widget_set_valign(bar, GTK_ALIGN_FILL);
        } else {
            gtk_widget_set_size_request(bar, -1, 3);
            gtk_widget_set_valign(bar, GTK_ALIGN_END);
            gtk_widget_set_hexpand(bar, TRUE);
            gtk_widget_set_halign(bar, GTK_ALIGN_FILL);
        }

        gtk_box_append(GTK_BOX(container), bar);
    }

    g_print("✓ %d bars created for %s layout\n", VISUALIZER_BARS, is_vertical ? "vertical" : "horizontal");

    // Initialize PipeWire
    state->pw_loop = pw_thread_loop_new("hyprwave-visualizer", NULL);
    if (!state->pw_loop) {
        g_printerr("Failed to create PipeWire thread loop\n");
        return state;
    }

    state->pw_context = pw_context_new(pw_thread_loop_get_loop(state->pw_loop), NULL, 0);
    if (!state->pw_context) {
        g_printerr("Failed to create PipeWire context\n");
        return state;
    }

    // Start render loop
    state->render_timer = g_timeout_add(1000 / VISUALIZER_UPDATE_FPS, update_visualizer, state);

    return state;
}

void visualizer_show(VisualizerState *state) {
    if (!state || state->is_showing) return;

    state->is_showing = TRUE;

    if (state->fade_timer > 0) {
        g_source_remove(state->fade_timer);
    }

    // Make visible, then fade in
    gtk_widget_set_visible(state->container, TRUE);
    state->fade_opacity = 0.0;
    state->fade_timer = g_timeout_add(16, fade_visualizer, state);
    g_print("Visualizer fading in\n");
}

void visualizer_hide(VisualizerState *state) {
    if (!state || !state->is_showing) return;

    state->is_showing = FALSE;

    if (state->fade_timer > 0) {
        g_source_remove(state->fade_timer);
    }

    state->fade_timer = g_timeout_add(16, fade_visualizer, state);
    g_print("Visualizer fading out\n");
}

void visualizer_start(VisualizerState *state) {
    if (!state || state->is_running || !state->pw_loop) return;

    pw_thread_loop_lock(state->pw_loop);

    // Connect to PipeWire
    state->pw_core = pw_context_connect(state->pw_context, NULL, 0);
    if (!state->pw_core) {
        g_printerr("Failed to connect to PipeWire\n");
        pw_thread_loop_unlock(state->pw_loop);
        return;
    }

    // Get registry to find nodes
    state->pw_registry = pw_core_get_registry(state->pw_core, PW_VERSION_REGISTRY, 0);
    if (!state->pw_registry) {
        g_printerr("Failed to get PipeWire registry\n");
        pw_thread_loop_unlock(state->pw_loop);
        return;
    }

    spa_zero(state->registry_listener);
    pw_registry_add_listener(state->pw_registry, &state->registry_listener,
                             &registry_events, state);

    // Create capture stream
    state->pw_stream = pw_stream_new(state->pw_core, "HyprWave Visualizer",
        pw_properties_new(
            PW_KEY_MEDIA_TYPE, "Audio",
            PW_KEY_MEDIA_CATEGORY, "Capture",
            PW_KEY_MEDIA_ROLE, "DSP",
            NULL));

    if (!state->pw_stream) {
        g_printerr("Failed to create PipeWire stream\n");
        pw_thread_loop_unlock(state->pw_loop);
        return;
    }

    spa_zero(state->stream_listener);
    pw_stream_add_listener(state->pw_stream, &state->stream_listener,
                           &stream_events, state);

    // If target serial is known but node not yet matched, search the cache
    // (node may have been enumerated by registry before target was set)
    if (state->target_serial > 0 && !state->target_found) {
        search_cached_nodes_for_target(state);
    }
    // If target was already found, connect now
    if (state->target_found) {
        connect_to_target(state);
    }

    pw_thread_loop_unlock(state->pw_loop);

    // Start the thread loop
    if (pw_thread_loop_start(state->pw_loop) < 0) {
        g_printerr("Failed to start PipeWire thread loop\n");
        return;
    }

    state->is_running = TRUE;
    g_print("✓ Visualizer started (AGC-normalized audio capture)\n");
}

void visualizer_stop(VisualizerState *state) {
    if (!state || !state->is_running) return;

    if (state->pw_loop) {
        pw_thread_loop_stop(state->pw_loop);
    }

    if (state->pw_stream) {
        pw_stream_destroy(state->pw_stream);
        state->pw_stream = NULL;
    }

    if (state->pw_registry) {
        pw_proxy_destroy((struct pw_proxy *)state->pw_registry);
        state->pw_registry = NULL;
    }

    if (state->pw_core) {
        pw_core_disconnect(state->pw_core);
        state->pw_core = NULL;
    }

    state->is_running = FALSE;
    g_print("Visualizer stopped\n");
}

void visualizer_set_target_pid(VisualizerState *state, guint32 pid, const gchar *bus_name) {
    if (!state) return;

    if (state->target_pid == pid && pid != 0) {
        return;  // Same target, no change needed
    }

    g_print("Visualizer: Setting target PID to %u\n", pid);

    state->target_pid = pid;
    g_free(state->target_bus_name);
    state->target_bus_name = g_strdup(bus_name);
    state->target_found = FALSE;
    state->target_node_id = 0;
    state->target_serial = -1;

    // Find the sink-input index for this PID using pactl
    // This handles Chromium/Electron child process lookup
    gint sink_input = -1;
    if (pid > 0) {
        // pw_find_sink_input_by_pid walks the process tree
        sink_input = pw_find_sink_input_by_pid(pid);
        if (sink_input < 0) {
            // Try walking the tree manually
            gchar *cmd = g_strdup_printf("pgrep -P %u", pid);
            gchar *output = NULL;
            if (g_spawn_command_line_sync(cmd, &output, NULL, NULL, NULL) && output) {
                gchar **child_pids = g_strsplit(output, "\n", -1);
                for (gchar **p = child_pids; *p && **p; p++) {
                    guint32 child_pid = (guint32)g_ascii_strtoull(*p, NULL, 10);
                    if (child_pid > 0) {
                        sink_input = pw_find_sink_input_by_pid(child_pid);
                        if (sink_input >= 0) break;
                    }
                }
                g_strfreev(child_pids);
                g_free(output);
            }
            g_free(cmd);
        }
    }

    // Fallback: match by application name (for ALSA players without process.id)
    if (sink_input < 0 && bus_name) {
        // Extract app name from "org.mpris.MediaPlayer2.qobuz-player" -> "qobuz-player"
        const gchar *prefix = "org.mpris.MediaPlayer2.";
        const gchar *app_name = bus_name;
        if (g_str_has_prefix(bus_name, prefix)) {
            app_name = bus_name + strlen(prefix);
        }
        sink_input = pw_find_sink_input_by_app_name(app_name);
    }

    if (sink_input >= 0) {
        state->target_serial = sink_input;
        g_print("Visualizer: Found sink-input %d for PID %u\n", sink_input, pid);
    } else {
        g_print("Visualizer: No sink-input found for PID %u\n", pid);
    }

    // If running, disconnect current stream and search for new target
    if (state->is_running && state->pw_loop) {
        pw_thread_loop_lock(state->pw_loop);
        disconnect_stream(state);

        // Search cached nodes for the new target
        if (state->target_serial > 0) {
            search_cached_nodes_for_target(state);
        }
        pw_thread_loop_unlock(state->pw_loop);
    }
}

void visualizer_retry_target(VisualizerState *state) {
    if (!state || (state->target_pid == 0 && !state->target_bus_name)) return;

    // Already have a valid target, no need to retry
    if (state->target_serial > 0 && state->target_found) return;

    g_print("Visualizer: Retrying sink-input lookup for PID %u\n", state->target_pid);

    // Re-attempt to find sink-input by PID
    gint sink_input = -1;
    if (state->target_pid > 0) {
        sink_input = pw_find_sink_input_by_pid(state->target_pid);
        if (sink_input < 0) {
            // Try child processes
            gchar *cmd = g_strdup_printf("pgrep -P %u", state->target_pid);
            gchar *output = NULL;
            if (g_spawn_command_line_sync(cmd, &output, NULL, NULL, NULL) && output) {
                gchar **child_pids = g_strsplit(output, "\n", -1);
                for (gchar **p = child_pids; *p && **p; p++) {
                    guint32 child_pid = (guint32)g_ascii_strtoull(*p, NULL, 10);
                    if (child_pid > 0) {
                        sink_input = pw_find_sink_input_by_pid(child_pid);
                        if (sink_input >= 0) break;
                    }
                }
                g_strfreev(child_pids);
                g_free(output);
            }
            g_free(cmd);
        }
    }

    // Fallback: match by application name
    if (sink_input < 0 && state->target_bus_name) {
        const gchar *prefix = "org.mpris.MediaPlayer2.";
        const gchar *app_name = state->target_bus_name;
        if (g_str_has_prefix(state->target_bus_name, prefix)) {
            app_name = state->target_bus_name + strlen(prefix);
        }
        sink_input = pw_find_sink_input_by_app_name(app_name);
    }

    if (sink_input >= 0 && sink_input != state->target_serial) {
        state->target_serial = sink_input;
        state->target_found = FALSE;
        g_print("Visualizer: Found sink-input %d for PID %u (retry)\n", sink_input, state->target_pid);

        // Search cached nodes
        if (state->is_running && state->pw_loop) {
            pw_thread_loop_lock(state->pw_loop);
            search_cached_nodes_for_target(state);
            pw_thread_loop_unlock(state->pw_loop);
        }
    }
}

void visualizer_cleanup(VisualizerState *state) {
    if (!state) return;

    if (state->render_timer > 0) {
        g_source_remove(state->render_timer);
    }

    if (state->fade_timer > 0) {
        g_source_remove(state->fade_timer);
    }

    visualizer_stop(state);

    if (state->pw_context) {
        pw_context_destroy(state->pw_context);
    }

    if (state->pw_loop) {
        pw_thread_loop_destroy(state->pw_loop);
    }

    g_free(state->target_node_name);
    g_free(state->target_bus_name);
    if (state->audio_nodes) {
        g_hash_table_destroy(state->audio_nodes);
    }
    g_mutex_clear(&state->data_mutex);
    g_free(state);

    pw_deinit();
}
