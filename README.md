# hyprwave - v0.9 (latest release)

A sleek, modern music control bar for Wayland compositors (Hyprland, Niri, Sway, etc.) with MPRIS integration.

Updates till now: Multi-Anchor support, Notifications, Music Controls, CSS Styling (control bar, expanded section and notifications), launching it as an application, huge UI bug fixing, ability to seek to different song parts via click or drag and click, volume controls, audio visualizer with idle mode animation (for horizontal mode), 8-bit dot matrix info bar with idle mode animation (for vertical layouts). All visual bugs fixed.

Built and primarily tested on Niri, for all Wayland compositors that support GTK4 and GTK4-layer-shell.

Check out the subreddit for posting your rices, themes, favorite albums -> https://www.reddit.com/r/hyprwave/

# Your Control Bar, Your Imagination

<img width="518" height="240" alt="image" src="https://github.com/user-attachments/assets/d79afb29-844d-4bae-b899-28aea6646139" /> <br>



<img width="289" height="301" alt="image" src="https://github.com/user-attachments/assets/53a82a39-b073-4b05-8aaf-bdc3698a0fe9" />


Style hyprwave to your taste via the style.css!

## OCTANE THEME 

<img width="1366" height="768" alt="2026-01-30 00-05-13" src="https://github.com/user-attachments/assets/777942b8-5be2-46bf-be2b-209f4c8eb7e7" />
<img width="1366" height="768" alt="2026-01-30 00-04-10" src="https://github.com/user-attachments/assets/4c230ca6-4b08-42fd-8a86-eb42f16ef1aa" />
<img width="1366" height="768" alt="2026-01-30 00-04-29" src="https://github.com/user-attachments/assets/8c0d18a9-ee99-4455-9cd3-0c13f5082303" />
<img width="1366" height="768" alt="2026-01-30 00-04-46" src="https://github.com/user-attachments/assets/e5cce51b-8bdd-4e69-b5f6-5a71456dc459" />

<img width="629" height="351" alt="2026-01-29 18-10-19" src="https://github.com/user-attachments/assets/88fab7b1-160e-496d-9836-27cc5db8dbd5" />

Check out THEMES.md to get the theme, and more!


## Screenshots & GIFs

### Vertical Layout

<img width="105" height="305" alt="image" src="https://github.com/user-attachments/assets/7675da1e-d6ae-417d-8201-17e1eb670ed0" />

<img width="313" height="377" alt="image" src="https://github.com/user-attachments/assets/432a85ad-a013-400c-9652-2481ff9a93ad" />

### Horizontal Layout

<img width="388" height="84" alt="image" src="https://github.com/user-attachments/assets/67a477a9-3cdc-4029-941c-d99cff080b16" />

<img width="449" height="275" alt="image" src="https://github.com/user-attachments/assets/0b1bede5-102b-484b-ab23-d7a6f87fe8ac" />

### Audio Visualizer (Idle Mode - Horizontal)

#### Top Bar
<img width="361" height="296" alt="2026-01-25 17-19-32" src="https://github.com/user-attachments/assets/02424fab-6da7-40a3-bf45-a69595a299fb" />

#### Bottom Bar
<img width="633" height="746" alt="2026-01-25 17-09-42" src="https://github.com/user-attachments/assets/3e4228f3-6cda-4fd8-bbd6-426ab24df9cf" />

#### Transition from control bar to visualizer bar
![ezgif-10ec52723eb401f8](https://github.com/user-attachments/assets/f974ff9b-6271-4dcd-833c-230bb1dbf951)

### Dot Matrix Display (Idle Mode - Vertical)


https://github.com/user-attachments/assets/c4e84d3c-c93f-4bfe-b5e9-f7888b3b8b53

<img width="1366" height="768" alt="2026-02-01 22-40-45" src="https://github.com/user-attachments/assets/b7bd1bc0-f56f-437c-b938-adc9210e3c83" />

### Layout Examples

#### Right

![right](https://github.com/user-attachments/assets/0ecf38e1-535c-4858-91d8-190e9c9fd55a)

#### Top

![top](https://github.com/user-attachments/assets/eb1e279f-e47d-4f37-82ef-833810014fc9)

#### Bottom

![bottom](https://github.com/user-attachments/assets/9dab5619-efc3-4523-a135-038c3c6b2551)

#### Left

![left](https://github.com/user-attachments/assets/e02741cc-738d-4e3e-a097-89b98934842d)

## Features

- **Elegant Design** - Glassmorphic UI with smooth animations
- **MPRIS Integration** - Works with Spotify, VLC, and any MPRIS-compatible player
- **Album Art Display** - Fetches and displays album artwork
- **Live Progress Tracking** - Real-time progress bar with countdown timer
- **Full Playback Controls** - Play/Pause, Next, Previous buttons
- **Expandable Panel** - Toggle to reveal detailed track information
- **Volume Control** - Double-click album cover to show/hide volume slider
- **Audio Visualizer** - Real-time audio visualization with idle mode animation
- **Idle Mode** - Automatically morphs into visualizer after inactivity
- **Now Playing Notifications** - Elegant slide-in notifications for track changes
- **Configurable Layout** - Position on any screen edge (left, right, top, bottom)
- **Keybind Support** - Hide/show and expand with keyboard shortcuts
- **Minimal Resource Usage** - ~80-95MB RAM, <0.3% CPU

### Audio Visualizer

The visualizer captures your system audio playback and displays real-time frequency bars. After a configurable period of inactivity (default: 5 seconds), HyprWave automatically transitions from the control bar to an animated visualizer. Mouse movement restores the control buttons.

Features:
- Real-time audio frequency visualization
- Smooth fade animations between modes
- Configurable idle timeout
- Option to disable visualizer completely
- Works only in horizontal layouts (top/bottom edges)

### Dot Matrix Display 
Ever felt nostalgic about old mp3 players? Revive it back with vertical mode, with a minimalistic idle mode, scrolling name of song and artist name, PAUSE animations and PLAY animations. Mouse hover restores it back to the control bar. Also, if you don't want the dot matrix font, you are free to replace it with any other font, just place it in the same place as the VT323 font where it is placed after install, and fix the size of font in style.css if needed. 

### Collapsed State

The control bar sits on your chosen screen edge with essential controls.

### Expanded State

Shows album cover, track title, artist, progress bar, and time remaining.

### Now Playing Notifications

Smooth slide-in notifications appear in the top-right corner when tracks change, showing album art, song title, and artist.

### Volume Control

Double-click the album cover to reveal the volume bar. The volume bar auto-hides after 3 seconds of inactivity or when you collapse the expanded state.

## Installation

### Dependencies

```bash
# Arch Linux / Manjaro
sudo pacman -S gtk4 gtk4-layer-shell pulseaudio

# Ubuntu / Debian
sudo apt install libgtk-4-dev gtk4-layer-shell libpulse-dev

# Fedora
sudo dnf install gtk4-devel gtk4-layer-shell-devel pulseaudio-libs-devel
```
### Arch(-based)
Also, Massive update - hyprwave is now on AUR.
Simply install it with:

```bash
yay -S hyprwave
```

It will not give you the bleeding new updates, but the latest releases.

### NixOS
Installing the package:
1. Download the `default.nix` File.
2. Add the package to your `configuration.nix` or `flake.nix`:
```nix
let
  hyprwave = pkgs.callPackage ./path/to/default.nix { };
in
...
environment.systemPackages = with pkgs; [
  hyprwave
];
```
3. Rebuild.

Testing the package without installing:
1. Run `nix run github:shantanubaddar/hyprwave`.

### Building from Source

```bash
# Clone the repository
git clone https://github.com/shantanubaddar/hyprwave.git
cd hyprwave

# Build and install
make
make install
```

The installer will:
- Copy binary to `~/.local/bin/hyprwave`
- Install resources to `~/.local/share/hyprwave/`
- Install `hyprwave-toggle` script for keybinds
- Create config at `~/.config/hyprwave/config.conf`

## Usage

### Quick Start

1. **Install and run HyprWave**: `hyprwave`
2. **Start a music player** (Spotify, VLC, etc.)
3. **Control your music** with the on-screen controls
4. **Use keybinds** for quick access (see configuration below)

### Supported Music Players

Fully Supported:
- Spotify (Desktop app)
- VLC Media Player
- Any MPRIS2-compatible player (Rhythmbox, Audacious, MPD with mpDris2, etc.)

Limited Support:
- Web browsers - Basic controls only, limited metadata

## Configuration

### Config File

Edit `~/.config/hyprwave/config.conf`:

```conf
# HyprWave Configuration File

[General]
# Edge to anchor HyprWave to
# Options: right, left, top, bottom
edge = right

# Margin from the screen edge (in pixels)
margin = 10

[Notifications]
enabled = true
now_playing = true

[Visualizer]
# Enable/disable visualizer (horizontal layout only)
enabled = true

# Idle timeout in seconds before visualizer appears
# Set to 0 to disable auto-activation
idle_timeout = 5

[VerticalDisplay]
enabled=true
idle_timeout=5

[MusicPlayer]
preference = spotify,vlc
```

**Layout Options:**
- **`edge = right`** - Vertical layout on right edge (default)
- **`edge = left`** - Vertical layout on left edge
- **`edge = top`** - Horizontal layout on top edge
- **`edge = bottom`** - Horizontal layout on bottom edge

**Notification Options:**
- **`enabled = true`** - Master switch for all notifications
- **`now_playing = true`** - Show "Now Playing" notifications when tracks change

**Visualizer Options:**
- **`enabled = true`** - Enable audio visualizer (horizontal layouts only)
- **`idle_timeout = 5`** - Seconds of inactivity before visualizer appears (0 to disable)

**Dot Matrix Display Options:**
- **`enabled = true`** - Enable audio visualizer (horizontal layouts only)
- **`idle_timeout = 5`** - Seconds of inactivity before visualizer appears (0 to disable)

**Music Player:**
Still being developed, however, if you are a spotify user, don't worry about it. Your experience will be smooth all around.

### How Notifications Will Appear

https://github.com/user-attachments/assets/7328c91b-c9fa-43ac-a8fd-8c63c9b676d3

### Keybinds

HyprWave supports keybinds for toggling visibility and expanding details. Add these to your compositor config:

#### Hyprland

Add to `~/.config/hypr/hyprland.conf`:

```conf
# HyprWave keybinds
bind = SUPER_SHIFT, M, exec, hyprwave-toggle visibility
bind = SUPER, M, exec, hyprwave-toggle expand
```

Then reload: `hyprctl reload`

#### Niri

Add to `~/.config/niri/config.kdl`:

```kdl
binds {
    Mod+Shift+M { spawn "hyprwave-toggle" "visibility"; }
    Mod+M { spawn "hyprwave-toggle" "expand"; }
}
```

Then reload: `niri msg action reload-config`

#### Sway

Add to `~/.config/sway/config`:

```conf
# HyprWave keybinds
bindsym $mod+Shift+M exec hyprwave-toggle visibility
bindsym $mod+M exec hyprwave-toggle expand
```

Then reload: `swaymsg reload`

#### What the Keybinds Do:

- **Toggle Visibility** (`Super+Shift+M`) - Smoothly hides/shows entire HyprWave with slide animation
- **Toggle Expand** (`Super+M`) - Shows/hides album details
  - Works even in visualizer mode - expanded section appears without exiting idle mode
  - If HyprWave is hidden, this will show it AND expand in one smooth motion

### Keybind Demo

https://github.com/user-attachments/assets/5bd27ec4-6b51-46fb-bf6e-fcb3cb3252b1

### Auto-start

#### Hyprland

Add to `~/.config/hypr/hyprland.conf`:
```conf
exec-once = hyprwave
```

#### Niri

Add to `~/.config/niri/config.kdl`:
```kdl
spawn-at-startup "hyprwave"
```

## Troubleshooting

### Black box around HyprWave (Hyprland)

If you see a black box around HyprWave, disable blur for the overlay:

Add to `hyprland.conf`:
```conf
layerrule = noblur, hyprwave
layerrule = noblur, hyprwave-notification
```

If that doesn't work, it's most probably a broken gtk4 or gtk4-layer-shell package - just remove them, reinstall them, and try again.

Refer to Issues for a more precise explanation.

### Visualizer not working

1. Ensure PulseAudio is running: `pulseaudio --check`
2. Check that visualizer is enabled in config: `enabled = true` under `[Visualizer]`
3. Visualizer only works in horizontal layouts (top/bottom edges)
4. Check console output for PulseAudio connection errors

### Notifications not appearing

1. Check that notifications are enabled in `~/.config/hyprwave/config.conf`
2. Verify both `enabled = true` and `now_playing = true` under `[Notifications]`
3. Restart HyprWave after config changes

### Keybinds not working

1. Verify `hyprwave-toggle` is installed: `which hyprwave-toggle`
2. Test manually: `hyprwave-toggle visibility` (with HyprWave running)
3. Check your compositor config for syntax errors
4. Reload your compositor config after adding keybinds

### Album art not loading

HyprWave requires the music player to provide album art URLs via MPRIS. Desktop apps work better than web browsers for this.

## Technical Details

- **Language:** C
- **GUI Framework:** GTK4
- **Layer Shell:** gtk4-layer-shell (Wayland overlay)
- **Audio:** PulseAudio (for visualizer)
- **IPC:** D-Bus (MPRIS2 protocol)
- **Memory:** ~80-95MB RAM (base), ~100-110MB with visualizer active
- **CPU:** <0.3% idle, <1% during updates, <2% with visualizer

### File Paths

Resources are searched in this order:
1. Local directory: `./icons/`, `./style.css` (for development)
2. User install: `~/.local/share/hyprwave/`
3. System install: `/usr/share/hyprwave/`

Config: `~/.config/hyprwave/config.conf`

## Roadmap

### v0.9.0 (Current)
- Dot Matrix Display (DMD) with idle mode animation
- Configurable dot matrix display settings
- Improved mouse interaction handling
- Bug fixes and performance improvements

### v1.0.0 (Goals)
- Theming system with pre-built themes
- Custom dimensions and colors via config

## Contributing

Contributions welcome! Feel free to:
- Report bugs via [GitHub Issues](https://github.com/shantanubaddar/hyprwave/issues)
- Submit feature requests
- Create pull requests
- Share your custom themes/icons

## License

Open source. Free to use, modify, and distribute.

## Acknowledgments

- Built with [GTK4](https://gtk.org/)
- Uses [gtk4-layer-shell](https://github.com/wmww/gtk-layer-shell)
- Audio capture via [PulseAudio](https://www.freedesktop.org/wiki/Software/PulseAudio/)
- Inspired by [waybar](https://github.com/Alexays/Waybar)
- MPRIS specification by [freedesktop.org](https://www.freedesktop.org/)

## Support

- **Issues:** [GitHub Issues](https://github.com/shantanubaddar/hyprwave/issues)
- **Discussions:** [GitHub Discussions](https://github.com/shantanubaddar/hyprwave/discussions)

## Stargazers

[![Star History Chart](https://api.star-history.com/svg?repos=shantanubaddar/hyprwave&type=Timeline)](https://star-history.com/#shantanubaddar/hyprwave&Timeline)
