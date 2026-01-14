#ifndef VOLUME_H
#define VOLUME_H

#include <gtk/gtk.h>
#include <gio/gio.h>

typedef struct {
    GDBusProxy *mpris_proxy;
    GtkWidget *revealer;
    GtkWidget *container;
    GtkWidget *icon;
    GtkWidget *slider;
    GtkWidget *percentage;
    gdouble current_volume;
    gdouble pending_volume;  // For throttled updates
    gboolean is_showing;
    guint hide_timer;
    guint pending_set_timer;  // For throttled volume setting
} VolumeState;

// Initialize volume control
VolumeState* volume_init(GDBusProxy *mpris_proxy, gboolean is_vertical);

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
