#include "pipewire_volume.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gio/gio.h>

/**
 * PipeWire Per-Application Volume Control Implementation
 *
 * Uses pactl (PipeWire-Pulse compatibility layer) to control per-application
 * volume via sink-inputs. This approach was chosen over libpipewire because:
 * 1. HyprWave already uses libpulse for the visualizer
 * 2. pactl works on all PipeWire systems with pipewire-pulse
 * 3. No new library dependencies required
 * 4. Simpler than async libpipewire event loop
 */

gboolean pw_is_pactl_available(void) {
    gchar *stdout_str = NULL;
    gchar *stderr_str = NULL;
    gint exit_status;
    GError *error = NULL;

    gboolean result = g_spawn_command_line_sync(
        "which pactl",
        &stdout_str, &stderr_str, &exit_status, &error);

    g_free(stdout_str);
    g_free(stderr_str);
    if (error) g_error_free(error);

    return result && exit_status == 0;
}

guint32 pw_extract_pid_from_bus_name(const gchar *mpris_bus_name) {
    if (!mpris_bus_name) return 0;

    // Pattern 1: "org.mpris.MediaPlayer2.chromium.instance280318"
    // Look for ".instance" followed by digits
    const gchar *instance = g_strstr_len(mpris_bus_name, -1, ".instance");
    if (instance) {
        instance += 9;  // Skip ".instance"
        guint32 pid = (guint32)g_ascii_strtoull(instance, NULL, 10);
        if (pid > 0) {
            g_print("PipeWire: Extracted PID %u from bus name instance suffix\n", pid);
            return pid;
        }
    }

    // Pattern 2: Use D-Bus to get the owner PID for players like spotify
    // Call org.freedesktop.DBus.GetConnectionUnixProcessId
    GError *error = NULL;
    GDBusProxy *dbus_proxy = g_dbus_proxy_new_for_bus_sync(
        G_BUS_TYPE_SESSION, G_DBUS_PROXY_FLAGS_NONE, NULL,
        "org.freedesktop.DBus", "/org/freedesktop/DBus",
        "org.freedesktop.DBus", NULL, &error);

    if (error) {
        g_error_free(error);
        return 0;
    }

    GVariant *result = g_dbus_proxy_call_sync(
        dbus_proxy,
        "GetConnectionUnixProcessID",  // Note: capital ID at end
        g_variant_new("(s)", mpris_bus_name),
        G_DBUS_CALL_FLAGS_NONE,
        -1, NULL, &error);

    g_object_unref(dbus_proxy);

    if (error) {
        g_print("PipeWire: Could not get PID for %s: %s\n", mpris_bus_name, error->message);
        g_error_free(error);
        return 0;
    }

    guint32 pid = 0;
    g_variant_get(result, "(u)", &pid);
    g_variant_unref(result);

    if (pid > 0) {
        g_print("PipeWire: Got PID %u from D-Bus for %s\n", pid, mpris_bus_name);
    }

    return pid;
}

gint pw_find_sink_input_by_pid(guint32 pid) {
    if (pid == 0) return -1;

    gchar *stdout_str = NULL;
    gchar *stderr_str = NULL;
    gint exit_status;
    GError *error = NULL;

    // Run pactl list sink-inputs
    gboolean result = g_spawn_command_line_sync(
        "pactl list sink-inputs",
        &stdout_str, &stderr_str, &exit_status, &error);

    g_free(stderr_str);
    if (error) {
        g_error_free(error);
        return -1;
    }

    if (!result || exit_status != 0 || !stdout_str) {
        g_free(stdout_str);
        return -1;
    }

    // Parse output to find sink-input with matching PID
    // Format:
    // Sink Input #6882
    //     ...
    //     Properties:
    //         application.process.id = "280318"
    //         ...
    gint found_index = -1;
    gint current_index = -1;
    gchar pid_pattern[64];
    g_snprintf(pid_pattern, sizeof(pid_pattern), "application.process.id = \"%u\"", pid);

    gchar **lines = g_strsplit(stdout_str, "\n", -1);
    g_free(stdout_str);

    for (gchar **line = lines; *line; line++) {
        // Check for "Sink Input #<number>"
        if (g_str_has_prefix(*line, "Sink Input #")) {
            gchar *index_str = *line + 12;  // Skip "Sink Input #"
            current_index = (gint)g_ascii_strtoll(index_str, NULL, 10);
        }

        // Check for matching PID
        if (current_index >= 0 && g_strstr_len(*line, -1, pid_pattern)) {
            found_index = current_index;
            g_print("PipeWire: Found sink-input #%d for PID %u\n", found_index, pid);
            break;
        }
    }

    g_strfreev(lines);
    return found_index;
}

gint pw_find_sink_input_by_app_name(const gchar *app_name) {
    if (!app_name || !*app_name) return -1;

    gchar *stdout_str = NULL;
    gchar *stderr_str = NULL;
    gint exit_status;
    GError *error = NULL;

    gboolean result = g_spawn_command_line_sync(
        "pactl list sink-inputs",
        &stdout_str, &stderr_str, &exit_status, &error);

    g_free(stderr_str);
    if (error) { g_error_free(error); return -1; }
    if (!result || exit_status != 0 || !stdout_str) { g_free(stdout_str); return -1; }

    gint found_index = -1;
    gint current_index = -1;

    gchar **lines = g_strsplit(stdout_str, "\n", -1);
    g_free(stdout_str);

    for (gchar **line = lines; *line; line++) {
        if (g_str_has_prefix(*line, "Sink Input #")) {
            current_index = (gint)g_ascii_strtoll(*line + 12, NULL, 10);
        }
        // Match application.name containing the app name (case-insensitive)
        if (current_index >= 0 && g_strstr_len(*line, -1, "application.name")) {
            gchar *lower_line = g_ascii_strdown(*line, -1);
            gchar *lower_name = g_ascii_strdown(app_name, -1);
            gboolean match = g_strstr_len(lower_line, -1, lower_name) != NULL;
            g_free(lower_line);
            g_free(lower_name);
            if (match) {
                found_index = current_index;
                g_print("PipeWire: Found sink-input #%d by app name '%s'\n", found_index, app_name);
                break;
            }
        }
    }

    g_strfreev(lines);
    return found_index;
}

/**
 * Find sink-input by checking the main PID and all its child processes.
 * This handles Chromium-based apps where audio comes from a child subprocess.
 */
static gint find_sink_input_in_process_tree(guint32 root_pid) {
    // First try the root PID directly
    gint result = pw_find_sink_input_by_pid(root_pid);
    if (result >= 0) return result;

    // Try to find child processes using pgrep
    gchar *command = g_strdup_printf("pgrep -P %u", root_pid);
    gchar *stdout_str = NULL;
    gchar *stderr_str = NULL;
    gint exit_status;
    GError *error = NULL;

    gboolean success = g_spawn_command_line_sync(
        command, &stdout_str, &stderr_str, &exit_status, &error);

    g_free(command);
    g_free(stderr_str);
    if (error) g_error_free(error);

    if (!success || exit_status != 0 || !stdout_str) {
        g_free(stdout_str);
        return -1;
    }

    // Parse child PIDs and check each one
    gchar **child_pids = g_strsplit(stdout_str, "\n", -1);
    g_free(stdout_str);

    for (gchar **pid_str = child_pids; *pid_str && **pid_str; pid_str++) {
        guint32 child_pid = (guint32)g_ascii_strtoull(*pid_str, NULL, 10);
        if (child_pid > 0) {
            result = pw_find_sink_input_by_pid(child_pid);
            if (result >= 0) {
                g_print("PipeWire: Found sink-input via child PID %u\n", child_pid);
                g_strfreev(child_pids);
                return result;
            }
            // Recursively check grandchildren
            result = find_sink_input_in_process_tree(child_pid);
            if (result >= 0) {
                g_strfreev(child_pids);
                return result;
            }
        }
    }

    g_strfreev(child_pids);
    return -1;
}

gint pw_find_sink_input_for_player(const gchar *mpris_bus_name) {
    guint32 pid = pw_extract_pid_from_bus_name(mpris_bus_name);
    if (pid == 0) {
        g_print("PipeWire: Could not extract PID from %s\n", mpris_bus_name);
        return -1;
    }

    // Search the entire process tree for a sink-input
    return find_sink_input_in_process_tree(pid);
}

gdouble pw_get_volume(gint sink_input_index) {
    if (sink_input_index < 0) return -1.0;

    gchar *stdout_str = NULL;
    gchar *stderr_str = NULL;
    gint exit_status;
    GError *error = NULL;

    // Run pactl list sink-inputs
    gboolean result = g_spawn_command_line_sync(
        "pactl list sink-inputs",
        &stdout_str, &stderr_str, &exit_status, &error);

    g_free(stderr_str);
    if (error) {
        g_error_free(error);
        return -1.0;
    }

    if (!result || exit_status != 0 || !stdout_str) {
        g_free(stdout_str);
        return -1.0;
    }

    // Parse output to find volume for the specific sink-input
    // Format:
    // Sink Input #6882
    //     ...
    //     Volume: front-left: 65536 / 100% / 0.00 dB,   front-right: 65536 / 100% / 0.00 dB
    //     ...
    gdouble volume = -1.0;
    gint current_index = -1;
    gboolean in_target_sink = FALSE;

    gchar **lines = g_strsplit(stdout_str, "\n", -1);
    g_free(stdout_str);

    for (gchar **line = lines; *line; line++) {
        // Check for "Sink Input #<number>"
        if (g_str_has_prefix(*line, "Sink Input #")) {
            gchar *index_str = *line + 12;
            current_index = (gint)g_ascii_strtoll(index_str, NULL, 10);
            in_target_sink = (current_index == sink_input_index);
        }

        // Parse volume line for target sink
        if (in_target_sink && g_strstr_len(*line, -1, "Volume:")) {
            // Look for percentage like "100%" or "75%"
            const gchar *percent = g_strstr_len(*line, -1, "%");
            if (percent) {
                // Walk backwards to find the start of the number
                const gchar *num_start = percent - 1;
                while (num_start > *line && g_ascii_isdigit(*(num_start - 1))) {
                    num_start--;
                }
                gint percent_val = (gint)g_ascii_strtoll(num_start, NULL, 10);
                volume = percent_val / 100.0;
                g_print("PipeWire: Sink-input #%d volume is %d%% (%.2f)\n",
                        sink_input_index, percent_val, volume);
                break;
            }
        }
    }

    g_strfreev(lines);
    return volume;
}

gboolean pw_set_volume(gint sink_input_index, gdouble volume) {
    if (sink_input_index < 0) return FALSE;

    // Clamp volume to reasonable range (0-150% to allow some boost)
    if (volume < 0.0) volume = 0.0;
    if (volume > 1.5) volume = 1.5;

    // Convert to percentage for pactl
    gint percent = (gint)(volume * 100.0 + 0.5);

    gchar *command = g_strdup_printf(
        "pactl set-sink-input-volume %d %d%%",
        sink_input_index, percent);

    g_print("PipeWire: Setting sink-input #%d volume to %d%%\n",
            sink_input_index, percent);

    gchar *stdout_str = NULL;
    gchar *stderr_str = NULL;
    gint exit_status;
    GError *error = NULL;

    gboolean result = g_spawn_command_line_sync(
        command, &stdout_str, &stderr_str, &exit_status, &error);

    g_free(command);
    g_free(stdout_str);
    g_free(stderr_str);

    if (error) {
        g_printerr("PipeWire: Failed to set volume: %s\n", error->message);
        g_error_free(error);
        return FALSE;
    }

    return result && exit_status == 0;
}
