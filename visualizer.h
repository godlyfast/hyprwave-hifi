#ifndef VISUALIZER_H
#define VISUALIZER_H

#include <gtk/gtk.h>
#include <pipewire/pipewire.h>
#include <spa/param/audio/format-utils.h>
#include <spa/utils/hook.h>

#define VISUALIZER_BARS 55
#define VISUALIZER_UPDATE_FPS 60

typedef struct {
    GtkWidget *container;  // Main container with bars
    GtkWidget *bars[VISUALIZER_BARS];

    // PipeWire context
    struct pw_thread_loop *pw_loop;
    struct pw_context *pw_context;
    struct pw_core *pw_core;
    struct pw_registry *pw_registry;
    struct spa_hook core_listener;
    struct spa_hook registry_listener;

    // PipeWire stream for audio capture
    struct pw_stream *pw_stream;
    struct spa_hook stream_listener;

    // Target player tracking
    guint32 target_pid;           // PID of current MPRIS player
    gchar *target_bus_name;       // MPRIS bus name for app-name fallback matching
    gint target_serial;           // PipeWire object.serial (same as pactl sink-input index)
    guint32 target_node_id;       // PipeWire node ID to capture from
    gchar *target_node_name;      // Node name for logging
    gboolean target_found;        // Whether we found the target node

    // Node cache for searching when target changes
    GHashTable *audio_nodes;      // node_id -> AudioNodeInfo*

    // Audio data
    gdouble bar_heights[VISUALIZER_BARS];
    gdouble bar_smoothed[VISUALIZER_BARS];

    // Automatic Gain Control (AGC) - makes visualization volume-independent
    gdouble agc_peak;             // Current tracked peak level
    gdouble agc_attack;           // How fast peak rises (0-1)
    gdouble agc_decay;            // How fast peak falls (0-1)

    // State
    gboolean is_showing;
    gboolean is_running;
    gboolean is_vertical;         // Layout orientation
    guint render_timer;
    guint fade_timer;
    gdouble fade_opacity;

    // Thread safety
    GMutex data_mutex;
} VisualizerState;

// Initialize visualizer (supports both horizontal and vertical layouts)
VisualizerState* visualizer_init(gboolean is_vertical);

// Show/hide visualizer (fades in/out)
void visualizer_show(VisualizerState *state);
void visualizer_hide(VisualizerState *state);

// Start/stop audio capture
void visualizer_start(VisualizerState *state);
void visualizer_stop(VisualizerState *state);

// Set target player by PID (call when MPRIS player changes)
// bus_name is used for app-name fallback when PID matching fails (ALSA players)
void visualizer_set_target_pid(VisualizerState *state, guint32 pid, const gchar *bus_name);

// Retry finding sink-input for current target (call when playback starts)
void visualizer_retry_target(VisualizerState *state);

// Cleanup
void visualizer_cleanup(VisualizerState *state);

#endif // VISUALIZER_H
