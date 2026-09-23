# hyprwave tests

## `run-selection-tests.sh` — player selection (automated)

Four scenarios against fake MPRIS services (`fake_mpris.py`) on an isolated
D-Bus session (`dbus-run-session`), asserting on stdout:

1. no players → `No MPRIS players found`
2. `playerctld` only → excluded (it is a proxy, not a player)
3. browser only → excluded (firefox/chromium are never admitted)
4. browser + preferred native player → native player wins via `preferred_player`

Requires: `python-dbus`, a Wayland display, and a built `./hyprwave`.
Run after touching player selection: `tests/run-selection-tests.sh`.

**What it cannot cover:** pointer/input behavior. There is no Wayland input
automation (ydotool/wtype/libei) and no input-region assertion, so the
regression below is manual.

## Input region — manual validation (~2 min)

Run this after touching `update_input_region_now`, `queue_input_region_update`,
revealer/expand transitions, or anything that resizes the layer surface.
Pause playback first so the time label is not repainting and masking staleness.

1. **Collapsed:** play button works. Clicks on the desktop just beside the bar
   must pass through.
2. **Expand,** then immediately click the player name, the seek bar, and the
   album art — all must respond. The empty strip above the pane must still
   pass clicks through.
3. **Race:** click the player name ~10 times rapidly, collapse, expand; within
   a second click the seek bar and player name. Repeat three rounds.
4. **Idle:** wait for the 32px idle strip — the strip must capture the pointer
   (motion exits idle); the area beside it must pass through. Super+M while
   idle, then click the pane.

**Fix is falsified if:** after the expand animation has fully finished, pane
clicks pass through while bar clicks work, and the state persists until an
unrelated repaint. That is the stale-region bug; do not accept "it healed
later" as a pass.

**Also failing:** the empty strip above the bar starts eating clicks (region
grew beyond widget bounds), or volume controls stay dead after the volume
slide-in finishes.
