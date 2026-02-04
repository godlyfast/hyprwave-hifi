#ifndef PIPEWIRE_VOLUME_H
#define PIPEWIRE_VOLUME_H

#include <glib.h>

/**
 * PipeWire Per-Application Volume Control
 *
 * Maps MPRIS players to their PipeWire sink-inputs by extracting
 * the PID from the D-Bus name and matching it against pactl output.
 *
 * This enables volume control for players that don't support MPRIS Volume
 * (like Roon, Chromium/Electron apps) by controlling their audio stream
 * directly in PipeWire.
 */

/**
 * Find the PipeWire sink-input index for an MPRIS player.
 *
 * Extracts the PID from the MPRIS bus name (e.g., "org.mpris.MediaPlayer2.chromium.instance280318")
 * and searches for a matching sink-input in PipeWire.
 *
 * @param mpris_bus_name The full D-Bus name of the MPRIS player
 * @return The sink-input index, or -1 if not found
 */
gint pw_find_sink_input_for_player(const gchar *mpris_bus_name);

/**
 * Find the PipeWire sink-input index by PID directly.
 *
 * @param pid The process ID to search for
 * @return The sink-input index, or -1 if not found
 */
gint pw_find_sink_input_by_pid(guint32 pid);

/**
 * Get the current volume of a PipeWire sink-input.
 *
 * @param sink_input_index The sink-input index from pw_find_sink_input_*
 * @return Volume as a fraction (0.0 to 1.0+), or -1.0 on error
 */
gdouble pw_get_volume(gint sink_input_index);

/**
 * Set the volume of a PipeWire sink-input.
 *
 * @param sink_input_index The sink-input index from pw_find_sink_input_*
 * @param volume Volume as a fraction (0.0 to 1.0, can exceed 1.0 for boost)
 * @return TRUE on success, FALSE on failure
 */
gboolean pw_set_volume(gint sink_input_index, gdouble volume);

/**
 * Extract PID from MPRIS D-Bus name.
 *
 * Handles formats like:
 * - org.mpris.MediaPlayer2.chromium.instance280318 -> 280318
 * - org.mpris.MediaPlayer2.spotify -> uses D-Bus to get owner PID
 *
 * @param mpris_bus_name The full D-Bus name of the MPRIS player
 * @return The PID, or 0 if not found
 */
guint32 pw_extract_pid_from_bus_name(const gchar *mpris_bus_name);

/**
 * Find the PipeWire sink-input index by application name substring.
 *
 * Matches against application.name property in pactl output.
 * Useful for ALSA-based players that don't set application.process.id.
 *
 * @param app_name Substring to match in application.name (e.g., "qobuz-player")
 * @return The sink-input index, or -1 if not found
 */
gint pw_find_sink_input_by_app_name(const gchar *app_name);

/**
 * Check if pactl is available on the system.
 *
 * @return TRUE if pactl command exists, FALSE otherwise
 */
gboolean pw_is_pactl_available(void);

#endif // PIPEWIRE_VOLUME_H
