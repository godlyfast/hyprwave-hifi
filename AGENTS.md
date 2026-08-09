# AGENTS.md — hyprwave/

Parent: `~/AGENTS.md`

## OVERVIEW

GTK4/PipeWire music control overlay for Wayland. HiFi fork with per-application volume control and PipeWire-native audio visualization. ~80–95 MB RAM, <0.3 % CPU idle.

## STRUCTURE

```
hyprwave/
├── main.c                # Core application (GTK4 + gtk4-layer-shell)
├── visualizer.c/h        # PipeWire native visualizer (AGC-normalized)
├── pipewire_volume.c/h   # Per-application volume control
├── volume.c/h            # Volume UI components
├── layout.c/h            # Layout engine (position, expand, vertical mode)
├── art.c/h               # Album art loading/display
├── notification.c/h      # Now-playing slide-in notifications
├── vertical_display.c/h  # Vertical layout mode
├── paths.c/h             # XDG path resolution
├── style.css             # GTK theming
├── themes/               # Community themes
├── icons/                # App icons
├── fonts/                # Bundled fonts
├── Makefile              # Build system
├── flake.nix             # Nix flake
└── hyprwave-toggle.sh    # Visibility toggle script
```

## WHERE TO LOOK

| Task | Location | Notes |
|------|----------|-------|
| Core app logic | `main.c` | MPRIS proxy, UI state, timers |
| Visualizer | `visualizer.c` | PipeWire stream capture, FFT, AGC |
| Per-app volume | `pipewire_volume.c` | Targets specific player, not system-wide; reads/writes fractional percentages so `VOLUME_STEP` stays intact |
| Volume slider / label | `volume.c`, `volume.h` | `VOLUME_STEP` (0.5%) drives arrow keys and scroll wheel; `-`/`+` buttons use `VOLUME_STEP_DB` (constant ratio, capped at `VOLUME_STEP`) with accelerating press-and-hold repeat |
| Volume apply rate | `volume.c` | Leading-edge throttle on `VOLUME_APPLY_INTERVAL_US`; `pw_set_volume` spawns `pactl` synchronously on the UI thread |
| Seek bar / track length | `main.c` | `current_length`/`current_track_id` in `AppState` are the single source of truth for the bar and for `SetPosition`; `refresh_track_length()` repairs them when a player omits `mpris:length` |
| Layout / theming | `layout.c`, `style.css` | Position, expand, vertical mode |
| Build | `Makefile` | `make` → `./hyprwave` |

## CONVENTIONS

- C11 with GTK4 and gtk4-layer-shell
- MPRIS D-Bus proxy for player detection/control
- PipeWire native API (not PulseAudio compatibility layer)
- GDBus for MPRIS, PipeWire for audio capture

## COMMANDS

```bash
# Build
make

# Run
./hyprwave

# Toggle visibility
./hyprwave-toggle.sh

# Install deps (Arch)
sudo pacman -S gtk4 gtk4-layer-shell pipewire
```

## NOTES

- Compiled binary `hyprwave` is gitignored; build it at the repo root with `make`
- Nix flake available for NixOS users
- Requires active MPRIS player (Spotify, Roon, VLC, etc.)
- A `GtkScale` fills its elapsed portion with a `trough > highlight` node. `progress`
  is `GtkProgressBar`'s node and silently matches nothing on a scale, so the fill
  falls back to the system accent colour — style `.track-progress highlight`, not
  `.track-progress progress`
- Do not read `Metadata` fields straight from `g_dbus_proxy_get_cached_property()`
  at the point of use. GDBus replaces that cache wholesale from each
  `PropertiesChanged`, so a player that omits a key in one signal drops it until
  the next complete dict arrives. Read it once in `update_metadata()` and keep it
  in `AppState`
