#ifndef VOLUME_H
#define VOLUME_H

#include <gtk/gtk.h>
#include <gio/gio.h>

// Slider granularity as a fraction of full scale (0.005 = 0.5%)
#define VOLUME_STEP 0.005

// The +/- buttons step by a constant ratio rather than a constant amount, so
// the change sounds the same size wherever the level sits. Players that map a
// stepped control onto the MPRIS 0.0-1.0 double can end up using only the very
// bottom of the range, where a flat 0.5% is a several-dB jump.
#define VOLUME_STEP_DB 0.5      // Perceived size of one button press
#define VOLUME_STEP_MIN 0.0005  // Floor, so 0.0 is not a trap

typedef struct {
    GDBusProxy *mpris_proxy;
    GtkWidget *revealer;
    GtkWidget *container;
    GtkWidget *icon;
    GtkWidget *slider;
    GtkWidget *step_down;
    GtkWidget *step_up;
    GtkWidget *percentage;
    gdouble current_volume;
    gdouble pending_volume;  // For throttled updates
    gboolean is_showing;
    guint hide_timer;
    guint pending_set_timer;  // For throttled volume setting

    // PipeWire per-application volume control
    gchar *mpris_bus_name;       // D-Bus name for PID extraction
    gint pw_sink_input_index;    // PipeWire sink-input index, -1 if not found
    gboolean use_pipewire_volume; // TRUE if using PipeWire, FALSE for MPRIS
} VolumeState;

// Initialize volume control
// mpris_bus_name is used to find the PipeWire sink-input for PID-based mapping
VolumeState* volume_init(GDBusProxy *mpris_proxy, const gchar *mpris_bus_name, gboolean is_vertical);

// Update the MPRIS proxy and reinitialize PipeWire state (call when player changes)
void volume_update_player(VolumeState *state, GDBusProxy *mpris_proxy, const gchar *mpris_bus_name);

// Show volume control with animation
void volume_show(VolumeState *state);

// Hide volume control with animation
void volume_hide(VolumeState *state);

// Set volume via MPRIS (0.0 to 1.0)
void volume_set(VolumeState *state, gdouble volume);

// Get current volume from MPRIS
gdouble volume_get_current(VolumeState *state);

// Check if current player supports volume control
gboolean volume_is_supported(VolumeState *state);

// Update volume icon based on percentage
void volume_update_icon(VolumeState *state, gint percentage);

// Cleanup
void volume_cleanup(VolumeState *state);

#endif // VOLUME_H
