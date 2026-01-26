#include "visualizer.h"
#include <math.h>
#include <string.h>

#define SMOOTHING_FACTOR 0.7

// Process audio samples into bar heights
static void process_audio_samples(VisualizerState *state, const float *samples, size_t n_samples) {
    size_t samples_per_bin = n_samples / VISUALIZER_BARS;
    
    for (int i = 0; i < VISUALIZER_BARS; i++) {
        gdouble sum = 0.0;
        size_t start = i * samples_per_bin;
        size_t end = start + samples_per_bin;
        
        for (size_t j = start; j < end && j < n_samples; j++) {
            sum += samples[j] * samples[j];
        }
        
        gdouble rms = sqrt(sum / samples_per_bin);
        gdouble normalized = rms * 10.0;
        if (normalized > 1.0) normalized = 1.0;
        
        // Smooth the values
        state->bar_smoothed[i] = (SMOOTHING_FACTOR * state->bar_smoothed[i]) + 
                                 ((1.0 - SMOOTHING_FACTOR) * normalized);
        
        state->bar_heights[i] = state->bar_smoothed[i];
    }
}

// PulseAudio stream read callback
static void pa_stream_read_callback(pa_stream *stream, size_t nbytes, void *userdata) {
    VisualizerState *state = (VisualizerState *)userdata;
    const void *data;
    size_t length;
    
    if (pa_stream_peek(stream, &data, &length) < 0 || !data) {
        if (data) pa_stream_drop(stream);
        return;
    }
    
    const float *samples = (const float *)data;
    size_t n_samples = length / sizeof(float);
    
    process_audio_samples(state, samples, n_samples);
    pa_stream_drop(stream);
}

// Update visualizer bars (~30fps)
static gboolean update_visualizer(gpointer user_data) {
    VisualizerState *state = (VisualizerState *)user_data;
    
    if (!state->is_showing) {
        return G_SOURCE_CONTINUE;
    }
    
    for (int i = 0; i < VISUALIZER_BARS; i++) {
        gint min_height = 1;   // Minimum bar height
        gint max_height = 24;  // Maximum bar height (fits in 32px control bar)
        
        // Decay to minimum if no audio
        if (state->bar_heights[i] < 0.01) {
            state->bar_heights[i] = 0.0;
        }
        
        // Calculate bar height
        gint bar_height = min_height + (gint)(state->bar_heights[i] * (max_height - min_height));
        
        // Update size (opacity handled by container)
        gtk_widget_set_size_request(state->bars[i], 3, bar_height);
        
        // Make sure bars are visible
        gtk_widget_set_visible(state->bars[i], TRUE);
        
        if (bar_height <= min_height) {
          gtk_widget_set_opacity(state->bars[i], 0.0);
        } else {
          gtk_widget_set_opacity(state->bars[i], 1.0);
}

gtk_widget_set_size_request(state->bars[i], 3, bar_height);
        
    }

    
    return G_SOURCE_CONTINUE;
}

// Fade animation (for smooth show/hide)
static gboolean fade_visualizer(gpointer user_data) {
    VisualizerState *state = (VisualizerState *)user_data;
    
    if (state->is_showing) {
        // Fade in
        state->fade_opacity += 0.05;
        if (state->fade_opacity >= 1.0) {
            state->fade_opacity = 1.0;
            state->fade_timer = 0;
            return G_SOURCE_REMOVE;
        }
    } else {
        // Fade out
        state->fade_opacity -= 0.05;
        if (state->fade_opacity <= 0.0) {
            state->fade_opacity = 0.0;
            state->fade_timer = 0;
            return G_SOURCE_REMOVE;
        }
    }
    
    // Apply opacity to container too (not just bars)
    gtk_widget_set_opacity(state->container, state->fade_opacity);
    
    return G_SOURCE_CONTINUE;
}

// PulseAudio context state callback
static void pa_context_state_callback(pa_context *context, void *userdata) {
    VisualizerState *state = (VisualizerState *)userdata;
    
    switch (pa_context_get_state(context)) {
        case PA_CONTEXT_READY: {
            // Create audio stream for recording
            pa_sample_spec sample_spec = {
                .format = PA_SAMPLE_FLOAT32LE,
                .rate = 44100,
                .channels = 1
            };
            
            state->pa_stream = pa_stream_new(context, "HyprWave Visualizer", &sample_spec, NULL);
            if (!state->pa_stream) {
                g_printerr("Failed to create PulseAudio stream\n");
                return;
            }
            
            pa_stream_set_read_callback(state->pa_stream, pa_stream_read_callback, state);
            
            pa_buffer_attr buffer_attr = {
                .maxlength = (uint32_t) -1,
                .fragsize = 4096
            };
            
            // CRITICAL: Monitor the default sink (playback audio), not microphone!
            // This captures what's being played, not what's being recorded
            const char *monitor_source = "@DEFAULT_MONITOR@";
            
            if (pa_stream_connect_record(state->pa_stream, monitor_source, &buffer_attr, 
                                         PA_STREAM_ADJUST_LATENCY) < 0) {
                g_printerr("Failed to connect PulseAudio stream\n");
            } else {
                g_print("✓ Visualizer capturing playback audio (monitor)\n");
            }
            break;
        }
        case PA_CONTEXT_FAILED:
        case PA_CONTEXT_TERMINATED:
            g_printerr("PulseAudio context failed/terminated\n");
            break;
        default:
            break;
    }
}

// Initialize visualizer
VisualizerState* visualizer_init() {
    VisualizerState *state = g_new0(VisualizerState, 1);
    state->is_showing = FALSE;
    state->is_running = FALSE;
    state->fade_opacity = 0.0;
    
    // Zero out audio data
    for (int i = 0; i < VISUALIZER_BARS; i++) {
        state->bar_heights[i] = 0.0;
        state->bar_smoothed[i] = 0.0;
    }
    
    // Create container - Fixed width that fits perfectly
    GtkWidget *container = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    state->container = container;
    
    gtk_widget_set_overflow(container, GTK_OVERFLOW_HIDDEN);
    
    // Fixed width, centered, bottom aligned
    gtk_widget_set_halign(container, GTK_ALIGN_CENTER);
    gtk_widget_set_valign(container, GTK_ALIGN_END);   // Bottom align
    gtk_widget_set_hexpand(container, FALSE);          // Don't expand!
    gtk_widget_set_vexpand(container, FALSE);
    
    // Fixed width that leaves no gaps
    gtk_widget_set_size_request(container, 275, -1);
    
    
    // Add CSS class for padding control
    gtk_widget_add_css_class(container, "visualizer-container");
    
    g_print("✓ Visualizer container: 270px fixed width with padding\n");
    
    // Create 15 bars - distribute across 220px
    for (int i = 0; i < VISUALIZER_BARS; i++) {
        GtkWidget *bar = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
        state->bars[i] = bar;
        
        gtk_widget_add_css_class(bar, "visualizer-bar");
        // 220px / 15 bars = ~14-15px per bar
        gtk_widget_set_size_request(bar, -1, 3);
        gtk_widget_set_visible(bar, TRUE);
        gtk_widget_set_valign(bar, GTK_ALIGN_END);
        gtk_widget_set_hexpand(bar, TRUE);
        gtk_widget_set_halign(bar, GTK_ALIGN_FILL);
        
        gtk_box_append(GTK_BOX(container), bar);
    }
    
    g_print("✓ 15 bars created (~14-15px each)\n");
    
    // Initialize PulseAudio
    state->pa_mainloop = pa_threaded_mainloop_new();
    if (state->pa_mainloop) {
        state->pa_mainloop_api = pa_threaded_mainloop_get_api(state->pa_mainloop);
        state->pa_context = pa_context_new(state->pa_mainloop_api, "HyprWave");
        if (state->pa_context) {
            pa_context_set_state_callback(state->pa_context, pa_context_state_callback, state);
        }
    }
    
    // Start render loop
    state->render_timer = g_timeout_add(1000 / VISUALIZER_UPDATE_FPS, update_visualizer, state);
    
    return state;
}

void visualizer_show(VisualizerState *state) {
    if (!state || state->is_showing) return;
    
    state->is_showing = TRUE;
    
    // Cancel existing fade timer
    if (state->fade_timer > 0) {
        g_source_remove(state->fade_timer);
    }
    
    // Start fade-in animation
    state->fade_timer = g_timeout_add(16, fade_visualizer, state);  // ~60fps
    g_print("Visualizer fading in\n");
}

void visualizer_hide(VisualizerState *state) {
    if (!state || !state->is_showing) return;
    
    state->is_showing = FALSE;
    
    // Cancel existing fade timer
    if (state->fade_timer > 0) {
        g_source_remove(state->fade_timer);
    }
    
    // Start fade-out animation
    state->fade_timer = g_timeout_add(16, fade_visualizer, state);  // ~60fps
    g_print("Visualizer fading out\n");
}

void visualizer_start(VisualizerState *state) {
    if (!state || state->is_running || !state->pa_context) return;
    
    if (pa_context_connect(state->pa_context, NULL, PA_CONTEXT_NOFLAGS, NULL) < 0) {
        g_printerr("Failed to connect to PulseAudio\n");
        return;
    }
    
    if (pa_threaded_mainloop_start(state->pa_mainloop) < 0) {
        g_printerr("Failed to start PulseAudio mainloop\n");
        return;
    }
    
    state->is_running = TRUE;
    g_print("✓ Visualizer started\n");
}

void visualizer_stop(VisualizerState *state) {
    if (!state || !state->is_running) return;
    
    if (state->pa_stream) {
        pa_stream_disconnect(state->pa_stream);
        pa_stream_unref(state->pa_stream);
        state->pa_stream = NULL;
    }
    
    if (state->pa_mainloop) {
        pa_threaded_mainloop_stop(state->pa_mainloop);
    }
    
    state->is_running = FALSE;
    g_print("Visualizer stopped\n");
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
    
    if (state->pa_context) {
        pa_context_disconnect(state->pa_context);
        pa_context_unref(state->pa_context);
    }
    
    if (state->pa_mainloop) {
        pa_threaded_mainloop_free(state->pa_mainloop);
    }
    
    g_free(state);
}
