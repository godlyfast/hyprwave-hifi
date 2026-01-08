#ifndef NOTIFICATION_H
#define NOTIFICATION_H

#include <gtk/gtk.h>
#include <gtk4-layer-shell.h>

typedef struct {
    GtkWidget *window;
    GtkWidget *album_cover;
    GtkWidget *song_label;
    GtkWidget *artist_label;
    guint hide_timer;
    guint animation_timer;
    gint current_offset;
    gboolean is_showing;
} NotificationState;

// Initialize notification system
NotificationState* notification_init(GtkApplication *app);

// Show notification with track info
void notification_show(NotificationState *state, 
                       const gchar *title,
                       const gchar *artist, 
                       const gchar *art_url,
                       const gchar *notification_type);

// Hide notification
void notification_hide(NotificationState *state);

// Cleanup
void notification_cleanup(NotificationState *state);

#endif // NOTIFICATION_H
