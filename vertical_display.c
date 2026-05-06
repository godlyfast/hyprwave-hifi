#include "vertical_display.h"
#include <string.h>
#include <ctype.h>

#define SCROLL_INTERVAL_MS 200
#define VISIBLE_LINES 6

// Forward declarations
static gboolean scroll_animation(gpointer user_data);

// Sanitize text
static gchar* sanitize_text(const gchar *text) {
    if (!text || strlen(text) == 0) return g_strdup("UNKNOWN");
    
    GString *result = g_string_new("");
    
    for (gsize i = 0; i < strlen(text); i++) {
        char c = text[i];
        if (isalnum(c) || c == ' ' || c == '-' || c == '\'' || c == '&') {
            g_string_append_c(result, c);
        }
    }
    
    gchar *sanitized = g_string_free(result, FALSE);
    if (strlen(sanitized) < 2) {
        g_free(sanitized);
        return g_strdup("UNKNOWN");
    }
    
    return sanitized;
}

// Format text vertically
static gchar* format_vertical_text(const gchar *text) {
    if (!text || strlen(text) == 0) return g_strdup("");
    
    gchar *clean_text = sanitize_text(text);
    GString *result = g_string_new("");
    
    for (gsize i = 0; i < strlen(clean_text); i++) {
        char c = clean_text[i];
        
        if (c >= 'a' && c <= 'z') {
            c = c - 32;
        }
        
        if (c == ' ') {
            g_string_append(result, "\n\n");
        } else {
            g_string_append_c(result, c);
            g_string_append_c(result, '\n');
        }
    }
    
    g_free(clean_text);
    return g_string_free(result, FALSE);
}

// Format time vertically
static gchar* format_vertical_time(gint64 position_us, gint64 length_us) {
    gint64 pos_seconds = position_us / 1000000;
    gint minutes = pos_seconds / 60;
    gint seconds = pos_seconds % 60;
    
    return g_strdup_printf("%d\n%d\n:\n%d\n%d",
                          minutes / 10, minutes % 10,
                          seconds / 10, seconds % 10);
}

static gchar* format_vertical_system_clock(void) {
    GDateTime *now = g_date_time_new_now_local();
    gint hours = g_date_time_get_hour(now);
    gint minutes = g_date_time_get_minute(now);
    gchar *clock_text = g_strdup_printf("%d\n%d\n:\n%d\n%d",
                                        hours / 10, hours % 10,
                                        minutes / 10, minutes % 10);
    g_date_time_unref(now);
    return clock_text;
}

static void update_system_clock_display(VerticalDisplayState *state) {
    gchar *clock_text = format_vertical_system_clock();
    gtk_label_set_text(GTK_LABEL(state->label), clock_text);
    g_free(clock_text);
}

// Show PLAYING briefly then return to timer
static gboolean show_playing_status(gpointer user_data) {
    VerticalDisplayState *state = (VerticalDisplayState *)user_data;
    
    if (state->animation_frame >= 4) {  // Show for 1 second (4 frames * 250ms)
        // Switch back to timer mode
        state->current_mode = DISPLAY_MODE_TIME;
        state->status_animation_timer = 0;
        
        // Show current time
        gchar *time_text = format_vertical_time(state->current_position, state->track_length);
        gtk_label_set_text(GTK_LABEL(state->label), time_text);
        g_free(time_text);
        
        return G_SOURCE_REMOVE;
    }
    
    // Alternate between PLAYING and blank
if (state->animation_frame % 2 == 0) {
    gtk_label_set_text(GTK_LABEL(state->label), "P\nL\nA\nY\n▶");  // Removed extra \n
} else {
    gtk_label_set_text(GTK_LABEL(state->label), "P\nL\nA\nY\n◆");
}
    state->animation_frame++;
    return G_SOURCE_CONTINUE;
}

// Show SKIPPING briefly then scroll track
static gboolean show_skip_status(gpointer user_data) {
    VerticalDisplayState *state = (VerticalDisplayState *)user_data;
    
    if (state->animation_frame >= 4) {  // Show for 800ms
        state->status_animation_timer = 0;
        
        // Immediately start track scroll after SKIP finishes
        state->current_mode = DISPLAY_MODE_SCROLL_TRACK;
        state->scroll_index = 0;
        state->scroll_timer = g_timeout_add(SCROLL_INTERVAL_MS, scroll_animation, state);
        
        return G_SOURCE_REMOVE;
    }
    
   const gchar *arrows[] = {"►", "►►", "►►►", "►►"};
gchar *text = g_strdup_printf("S\nK\nI\nP\n%s", arrows[state->animation_frame % 4]);
    gtk_label_set_text(GTK_LABEL(state->label), text);
    g_free(text);
    
    state->animation_frame++;
    return G_SOURCE_CONTINUE;
}

// Scroll animation callback - TOP TO BOTTOM
static gboolean scroll_animation(gpointer user_data) {
    VerticalDisplayState *state = (VerticalDisplayState *)user_data;
    
    if (state->current_mode != DISPLAY_MODE_SCROLL_TRACK) {
        state->scroll_timer = 0;
        return G_SOURCE_REMOVE;
    }
    
    // Build full text: SONG + BY + ARTIST (no padding)
    gchar *song_text = format_vertical_text(state->current_title);
    gchar *artist_text = format_vertical_text(state->current_artist);
    
    GString *full = g_string_new("");
    g_string_append_printf(full, "%s\nB\nY\n\n%s", song_text, artist_text);
    
    gchar *full_text = g_string_free(full, FALSE);
    gchar **lines = g_strsplit(full_text, "\n", -1);
    gint total_lines = g_strv_length(lines);
    
    // Maximum scroll position: when last line reaches bottom of visible window
    gint max_scroll = total_lines - VISIBLE_LINES;
    if (max_scroll < 0) max_scroll = 0;
    
    // Check if scrolling is complete
    if (state->scroll_index > max_scroll) {
        state->current_mode = DISPLAY_MODE_TIME;
        state->scroll_timer = 0;
        
        gchar *time_text = format_vertical_time(state->current_position, state->track_length);
        gtk_label_set_text(GTK_LABEL(state->label), time_text);
        g_free(time_text);
        
        g_strfreev(lines);
        g_free(full_text);
        g_free(song_text);
        g_free(artist_text);
        
        return G_SOURCE_REMOVE;
    }
    
    // Build visible window - simple sliding window approach
    GString *visible = g_string_new("");
    
    gint start_line = state->scroll_index;
    gint end_line = start_line + VISIBLE_LINES;
    
    if (end_line > total_lines) end_line = total_lines;
    
    // Show current window
    for (gint i = start_line; i < end_line; i++) {
        if (i > start_line) {
            g_string_append_c(visible, '\n');
        }
        g_string_append(visible, lines[i]);
    }
    
    // If we have fewer lines than VISIBLE_LINES, pad at the bottom
    gint lines_shown = end_line - start_line;
    if (lines_shown < VISIBLE_LINES) {
        for (gint i = 0; i < (VISIBLE_LINES - lines_shown); i++) {
            g_string_append(visible, "\n");
        }
    }
    
    gtk_label_set_text(GTK_LABEL(state->label), visible->str);
    g_string_free(visible, TRUE);
    
    state->scroll_index++;
    
    g_strfreev(lines);
    g_free(full_text);
    g_free(song_text);
    g_free(artist_text);
    
    return G_SOURCE_CONTINUE;
}



// Update timer display (only updates when in TIME mode)
static gboolean update_timer_display(gpointer user_data) {
    VerticalDisplayState *state = (VerticalDisplayState *)user_data;
    
    if (state->current_mode == DISPLAY_MODE_STATUS_PAUSED) {
        update_system_clock_display(state);
        return G_SOURCE_CONTINUE;
    }

    // CRITICAL: Only update if in TIME mode
    if (state->current_mode != DISPLAY_MODE_TIME) {
        return G_SOURCE_CONTINUE;
    }
    
    gchar *time_text = format_vertical_time(state->current_position, state->track_length);
    gtk_label_set_text(GTK_LABEL(state->label), time_text);
    g_free(time_text);
    
    state->current_position += 1000000;
    
    return G_SOURCE_CONTINUE;
}

VerticalDisplayState* vertical_display_init() {
    VerticalDisplayState *state = g_new0(VerticalDisplayState, 1);
    
    state->container = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_add_css_class(state->container, "vertical-display-container");
    gtk_widget_set_halign(state->container, GTK_ALIGN_CENTER);
    gtk_widget_set_valign(state->container, GTK_ALIGN_CENTER);
    gtk_widget_set_hexpand(state->container, TRUE);
    gtk_widget_set_vexpand(state->container, TRUE);
    gtk_widget_set_overflow(state->container, GTK_OVERFLOW_HIDDEN);
    gtk_widget_set_size_request(state->container, 32, 280);
    
    state->label = gtk_label_new("");
    gtk_widget_add_css_class(state->label, "vertical-display-label");
    gtk_label_set_justify(GTK_LABEL(state->label), GTK_JUSTIFY_CENTER);
    gtk_widget_set_valign(state->label, GTK_ALIGN_CENTER);
    gtk_widget_set_halign(state->label, GTK_ALIGN_CENTER);
    gtk_widget_set_vexpand(state->label, TRUE);
    
    gtk_box_append(GTK_BOX(state->container), state->label);
    
    state->is_showing = FALSE;
    state->scroll_index = 0;
    state->fade_opacity = 0.0;
    state->current_mode = DISPLAY_MODE_TIME;
    state->is_paused = FALSE;
    state->animation_frame = 0;
    
    state->current_title = g_strdup("NO TRACK");
    state->current_artist = g_strdup("NO ARTIST");
    
    // Start timer update
    state->update_timer = g_timeout_add_seconds(1, update_timer_display, state);
    
    return state;
}

void vertical_display_show(VerticalDisplayState *state) {
    if (!state || state->is_showing) return;
    
    state->is_showing = TRUE;
    gtk_widget_set_visible(state->container, TRUE);
    gtk_widget_set_opacity(state->container, 1.0);
}

void vertical_display_hide(VerticalDisplayState *state) {
    if (!state || !state->is_showing) return;
    
    state->is_showing = FALSE;
    gtk_widget_set_opacity(state->container, 0.0);
    gtk_widget_set_visible(state->container, FALSE);
}

void vertical_display_update_track(VerticalDisplayState *state,
                                   const gchar *title,
                                   const gchar *artist) {
    if (!state) return;
    
    if (!title || strlen(title) == 0) title = "UNKNOWN TRACK";
    if (!artist || strlen(artist) == 0) artist = "UNKNOWN ARTIST";
    
    // Update stored track info
    g_free(state->current_title);
    g_free(state->current_artist);
    state->current_title = g_strdup(title);
    state->current_artist = g_strdup(artist);
    
    // If currently showing SKIP animation, let it finish naturally
    // It will trigger the track scroll when done
    if (state->current_mode == DISPLAY_MODE_STATUS_SKIPPING) {
        return;
    }
    
    // Cancel other timers
    if (state->scroll_timer > 0) {
        g_source_remove(state->scroll_timer);
        state->scroll_timer = 0;
    }
    if (state->status_animation_timer > 0) {
        g_source_remove(state->status_animation_timer);
        state->status_animation_timer = 0;
    }
    if (state->is_paused) {
        state->current_mode = DISPLAY_MODE_STATUS_PAUSED;
        update_system_clock_display(state);
        return;
    }
    
    // Start track scroll
    state->current_mode = DISPLAY_MODE_SCROLL_TRACK;
    state->scroll_index = 0;
    state->scroll_timer = g_timeout_add(SCROLL_INTERVAL_MS, scroll_animation, state);
}

void vertical_display_update_position(VerticalDisplayState *state,
                                      gint64 position,
                                      gint64 length) {
    if (!state) return;
    
    state->current_position = position;
    state->track_length = length;
    
    // Timer display will update automatically if in TIME mode
}

void vertical_display_set_paused(VerticalDisplayState *state, gboolean paused) {
    if (!state) return;
    
    state->is_paused = paused;
    
    // Cancel status animations
    if (state->status_animation_timer > 0) {
        g_source_remove(state->status_animation_timer);
        state->status_animation_timer = 0;
    }
    
    if (paused) {
        // Enter PAUSED mode - show local 24-hour clock
        state->current_mode = DISPLAY_MODE_STATUS_PAUSED;
        state->animation_frame = 0;
        update_system_clock_display(state);
    } else {
        // Show PLAYING briefly, then return to timer
        state->current_mode = DISPLAY_MODE_STATUS_PLAYING;
        state->animation_frame = 0;
        state->status_animation_timer = g_timeout_add(250, show_playing_status, state);
    }
}

void vertical_display_notify_skip(VerticalDisplayState *state) {
    if (!state) return;
    
    // Cancel existing animations
    if (state->status_animation_timer > 0) {
        g_source_remove(state->status_animation_timer);
    }
    if (state->scroll_timer > 0) {
        g_source_remove(state->scroll_timer);
    }
    
    // Show SKIPPING
    state->current_mode = DISPLAY_MODE_STATUS_SKIPPING;
    state->animation_frame = 0;
    state->status_animation_timer = g_timeout_add(200, show_skip_status, state);
}

void vertical_display_cleanup(VerticalDisplayState *state) {
    if (!state) return;
    
    if (state->scroll_timer > 0) g_source_remove(state->scroll_timer);
    if (state->status_animation_timer > 0) g_source_remove(state->status_animation_timer);
    if (state->update_timer > 0) g_source_remove(state->update_timer);
    
    g_free(state->current_title);
    g_free(state->current_artist);
    g_free(state);
}
