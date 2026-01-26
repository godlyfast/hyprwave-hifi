#ifndef VISUALIZER_H
#define VISUALIZER_H

#include <gtk/gtk.h>
#include <pulse/pulseaudio.h>

#define VISUALIZER_BARS 55
#define VISUALIZER_UPDATE_FPS 60

typedef struct {
    GtkWidget *container;  // Main container with bars
    GtkWidget *bars[VISUALIZER_BARS];
    
    // PulseAudio context
    pa_threaded_mainloop *pa_mainloop;
    pa_mainloop_api *pa_mainloop_api;
    pa_context *pa_context;
    pa_stream *pa_stream;
    
    // Audio data
    gdouble bar_heights[VISUALIZER_BARS];
    gdouble bar_smoothed[VISUALIZER_BARS];
    
    // State
    gboolean is_showing;
    gboolean is_running;
    guint render_timer;
    guint fade_timer;
    gdouble fade_opacity;
} VisualizerState;

// Initialize visualizer (horizontal layout only)
VisualizerState* visualizer_init();

// Show/hide visualizer (fades in/out)
void visualizer_show(VisualizerState *state);
void visualizer_hide(VisualizerState *state);

// Start/stop audio capture
void visualizer_start(VisualizerState *state);
void visualizer_stop(VisualizerState *state);

// Cleanup
void visualizer_cleanup(VisualizerState *state);

#endif // VISUALIZER_H
