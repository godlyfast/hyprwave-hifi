#!/bin/bash
# Regression tests for hyprwave player selection (tests/run-selection-tests.sh)
# Each scenario runs ./hyprwave inside an isolated D-Bus session with fake
# MPRIS services and asserts on stdout. Requires: python-dbus, a Wayland
# display (WAYLAND_DISPLAY), and a built ./hyprwave at the repo root.
set -u
cd "$(dirname "$0")/.."

PASS=0
FAIL=0

# run_scenario <name> <expected-regex> <setup-commands...>
run_scenario() {
    local name="$1" expect="$2" setup="$3"
    local log
    log=$(mktemp)
    XDG_CONFIG_HOME=$(mktemp -d) dbus-run-session -- bash -c "
        $setup
        sleep 0.5
        timeout 5 ./hyprwave
    " >"$log" 2>&1
    if grep -qE "$expect" "$log"; then
        echo "PASS: $name"
        PASS=$((PASS + 1))
    else
        echo "FAIL: $name (expected /$expect/)"
        grep -E 'Switched|Connected|Restored|No MPRIS' "$log" | sed 's/^/    /'
        FAIL=$((FAIL + 1))
    fi
    rm -f "$log"
}

# 1. No players at all -> clean empty state, no crash
run_scenario "no players" "No MPRIS players found" ":"

# 2. playerctld only -> still excluded (it is a proxy, not a player)
run_scenario "playerctld excluded" "No MPRIS players found" \
    "python3 tests/fake_mpris.py org.mpris.MediaPlayer2.playerctld playerctld Paused 0 &"

# 3. Browser only -> excluded, graceful empty state (browsers are never selected)
run_scenario "browser excluded" "No MPRIS players found" \
    "python3 tests/fake_mpris.py org.mpris.MediaPlayer2.firefox.instance_1_95 Firefox Paused 1 &"

# 4. Browser + preferred native player -> native player wins via preferred_player
#    (also proves the browser is NOT picked when a real player is present)
run_scenario "preferred native wins" "Restored last player: org.mpris.MediaPlayer2.vlc" \
    "mkdir -p \"\$XDG_CONFIG_HOME/hyprwave\"; echo -n org.mpris.MediaPlayer2.vlc > \"\$XDG_CONFIG_HOME/hyprwave/preferred_player\";
     python3 tests/fake_mpris.py org.mpris.MediaPlayer2.vlc 'VLC media player' Playing 1 &
     python3 tests/fake_mpris.py org.mpris.MediaPlayer2.firefox.instance_1_95 Firefox Paused 1 &"

echo "---"
echo "$PASS passed, $FAIL failed"
exit "$FAIL"
