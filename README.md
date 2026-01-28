# HyprWave HiFi

A sleek music control overlay for Wayland with **per-application volume control** and **PipeWire-native audio visualization**.

This is a HiFi-focused fork of [hyprwave](https://github.com/shantanubaddar/hyprwave) with enhanced audio integration for audiophile setups.

<p align="center">
  <img src="screenshots/dark-expanded.png" alt="Expanded view with visualizer" width="280">
</p>

## What Makes This Fork Different

| Feature | Upstream | HyprWave HiFi |
|---------|----------|---------------|
| **Volume Control** | System-wide (pactl) | Per-application (targets specific player) |
| **Visualizer Backend** | PulseAudio | PipeWire native API |
| **Visualizer Layout** | Horizontal only | Works in vertical layout too |
| **Volume Independence** | Tied to volume level | AGC normalizes dynamics |
| **Album Art** | Standard size | Larger, prominent display |
| **Visualizer Position** | Replaces controls | Below album art in expanded view |

### Per-Application Volume Control

When you adjust volume in HyprWave HiFi, it controls **only the music player's volume**, not your entire system. This means:
- Your notification sounds stay at their level
- Game audio remains unaffected
- Video call volume is independent
- Perfect for multi-application audio setups

### PipeWire Native Visualizer

The visualizer uses PipeWire's native API directly (not the PulseAudio compatibility layer), providing:
- Lower latency audio capture
- Automatic Gain Control (AGC) - visualization responds to audio dynamics, not volume level
- Per-player audio capture (visualizes only your music player, not system sounds)

## Screenshots

### Collapsed State
| Dark Theme | Light Theme |
|:----------:|:-----------:|
| <img src="screenshots/dark-collapsed.png" width="120"> | <img src="screenshots/light-collapsed.png" width="120"> |

### Expanded State
| Dark Theme | Light Theme |
|:----------:|:-----------:|
| <img src="screenshots/dark-expanded.png" width="200"> | <img src="screenshots/light-expanded.png" width="200"> |

## Features

- **Per-Application Volume** - Control only your music player's volume
- **PipeWire Native Visualizer** - Real-time frequency visualization with AGC
- **MPRIS Integration** - Works with Spotify, Roon, VLC, and any MPRIS player
- **Large Album Art** - Prominent artwork display for your music
- **Progress Seeking** - Click or drag the progress bar to seek
- **Now Playing Notifications** - Elegant slide-in notifications on track change
- **Configurable Layout** - Position on any screen edge
- **Light & Dark Themes** - Built-in theme support
- **Keybind Support** - Toggle visibility and expand with keyboard shortcuts
- **Minimal Resources** - ~80-95MB RAM, <0.3% CPU idle

## Installation

### Dependencies

```bash
# Arch Linux
sudo pacman -S gtk4 gtk4-layer-shell pipewire

# Ubuntu / Debian
sudo apt install libgtk-4-dev gtk4-layer-shell libpipewire-0.3-dev

# Fedora
sudo dnf install gtk4-devel gtk4-layer-shell-devel pipewire-devel
```

### Building from Source

```bash
git clone https://github.com/godlyfast/hyprwave-hifi.git
cd hyprwave-hifi
make
make install
```

This installs:
- Binary to `~/.local/bin/hyprwave`
- Resources to `~/.local/share/hyprwave/`
- Toggle script `hyprwave-toggle` for keybinds
- Default config at `~/.config/hyprwave/config.conf`

## Usage

```bash
hyprwave
```

Then start any MPRIS-compatible music player (Spotify, Roon, VLC, etc.).

### Keybinds

Add to your Hyprland config (`~/.config/hypr/hyprland.conf`):

```conf
bind = SUPER_ALT, M, exec, hyprwave-toggle visibility
bind = SUPER_CTRL, M, exec, hyprwave-toggle expand
```

| Keybind | Action |
|---------|--------|
| `Super+Alt+M` | Toggle visibility (hide/show) |
| `Super+Ctrl+M` | Toggle expanded view |

### Auto-start

```conf
# Hyprland
exec-once = hyprwave

# Niri
spawn-at-startup "hyprwave"
```

## Configuration

Edit `~/.config/hyprwave/config.conf`:

```conf
[General]
# Position: right, left, top, bottom
edge = right

# Margin from screen edge (pixels)
margin = 10

# Theme: light or dark
theme = dark

[Notifications]
enabled = true
now_playing = true

[Visualizer]
# Visualizer appears in expanded section below album art
enabled = true

# Seconds before visualizer activates (0 to disable auto-activation)
idle_timeout = 30
```

### Layout Options

| Edge | Layout | Visualizer |
|------|--------|------------|
| `right` / `left` | Vertical | In expanded section (below album art) |
| `top` / `bottom` | Horizontal | In expanded section |

## Troubleshooting

### Black box around HyprWave

Add to `hyprland.conf`:
```conf
layerrule = noblur, hyprwave
layerrule = noblur, hyprwave-notification
```

### Visualizer not showing

1. Ensure PipeWire is running: `systemctl --user status pipewire`
2. Expand the panel (visualizer appears below album art in expanded view)
3. Check `enabled = true` under `[Visualizer]` in config
4. Play music - visualizer activates when audio is detected

### Volume control not working

Per-application volume requires `pactl` (part of pipewire-pulse):
```bash
# Check if available
which pactl

# Install if missing (Arch)
sudo pacman -S pipewire-pulse
```

## Technical Details

- **Language:** C
- **GUI:** GTK4 with gtk4-layer-shell
- **Audio Visualizer:** PipeWire native API with AGC
- **Volume Control:** PipeWire via pactl (per-application sink-inputs)
- **Player Control:** D-Bus MPRIS2 protocol
- **Memory:** ~80-95MB (base), ~100-110MB with visualizer
- **CPU:** <0.3% idle, <2% with visualizer

### AGC Parameters

The Automatic Gain Control normalizes audio levels so visualization responds to dynamics, not volume:

| Parameter | Value | Purpose |
|-----------|-------|---------|
| Attack | 0.9 | Fast response to louder audio |
| Decay | 0.9995 | Slow decay during quiet parts |
| Min Threshold | 0.0001 | Avoid amplifying silence |

## Credits

- **Original project:** [hyprwave](https://github.com/shantanubaddar/hyprwave) by shantanubaddar
- **GTK4:** [gtk.org](https://gtk.org/)
- **Layer shell:** [gtk4-layer-shell](https://github.com/wmww/gtk-layer-shell)
- **Audio:** [PipeWire](https://pipewire.org/)

## License

GPL-3.0 - See [LICENSE](LICENSE)
