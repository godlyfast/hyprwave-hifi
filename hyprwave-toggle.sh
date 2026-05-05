#!/bin/bash
# HyprWave Toggle Script

ACTION="$1"

if [ -z "$ACTION" ]; then
    echo "Usage: hyprwave-toggle {visibility|expand}"
    exit 1
fi

PID=$(pgrep -x hyprwave)
if [ -z "$PID" ]; then
    # No instance running. Auto-spawn on visibility toggle so the keybind
    # acts as "open hyprwave" on first press; subsequent presses toggle.
    if [ "$ACTION" = "visibility" ]; then
        # Resolve binary: rely on PATH (Hyprland inherits /usr/local/bin),
        # then fall back to ~/.local/bin if PATH is stripped.
        if command -v hyprwave >/dev/null 2>&1; then
            HYPRWAVE_BIN=hyprwave
        elif [ -x "$HOME/.local/bin/hyprwave" ]; then
            HYPRWAVE_BIN="$HOME/.local/bin/hyprwave"
        else
            echo "hyprwave binary not found in PATH or ~/.local/bin" >&2
            exit 1
        fi
        # Detach fully so hyprwave outlives this script invocation.
        setsid "$HYPRWAVE_BIN" >/dev/null 2>&1 </dev/null &
        exit 0
    fi
    echo "HyprWave is not running" >&2
    exit 1
fi

case "$ACTION" in
    visibility)
        kill -USR1 "$PID"
        ;;
    expand)
        kill -USR2 "$PID"
        ;;
    *)
        echo "Invalid action: $ACTION"
        echo "Usage: hyprwave-toggle {visibility|expand}"
        exit 1
        ;;
esac
