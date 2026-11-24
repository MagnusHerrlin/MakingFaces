# MakingFaces v1.0.0 — Installation & Raspberry Pi 5 Setup Guide

This guide covers building **MakingFaces v1.0.0** on Raspberry Pi OS (Bookworm/Trixie with Wayland/labwc) and Debian/Ubuntu Linux desktops.

---

## 1. Install Required System Packages

Install the C++17 compiler, SDL2 graphics/image libraries, OpenCV (for camera capture, background flood-fill, and Telea inpainting), and `zip`/`unzip` for theme pack management:

```bash
sudo apt update
sudo apt install -y \
  build-essential \
  g++ \
  make \
  pkg-config \
  libsdl2-dev \
  libsdl2-image-dev \
  libopencv-dev \
  zip \
  unzip \
  alsa-utils
```

*(Optional)* If you want to generate the built-in vector theme packs from SVG templates, also install `inkscape` or `librsvg2-bin`:

```bash
sudo apt install -y librsvg2-bin
```

---

## 2. Clone & Compile MakingFaces

```bash
git clone https://github.com
cd MakingFaces
make clean && make -j4
```

Verify the build:

```bash
./MakingFaces --version
```

Expected output:
```text
MakingFaces v1.0.0 (Architecture by Magnus Herrlin 2026 - Bodmer TFT_eSPI & Adafruit Uncanny_Eyes inspired)
```
---

## 3. Directory Structure

```text
MakingFaces/
├── MakingFaces          # Compiled C++17 Wayland/SDL2 binary
├── Makefile             # Build rules (-std=c++17 -O3, SDL2, OpenCV)
├── makingfaces.ini      # Active runtime settings, 14-layer profiles & macros
├── assets/              # Active unpacked artwork (face, eyes, bg, fx, etc.)
├── audio/               # Optional WAV audio tracks (001.wav .. 255.wav)
├── import/              # Drop PNG/JPG/WEBP files here for Ctrl+I Studio
│   └── done/            # Processed source images moved here automatically
└── packs/               # Theme pack archive folder
    └── default.zip      # Auto-unpacked on startup & updated on S / F4 save
```

---

## 4. Command-Line Options

| Flag | Description | Default |
| :--- | :--- | :--- |
| `-u, --universe <num>` | Art-Net or sACN DMX universe (`1..63999`) | `1` |
| `-c, --channel <num>` | DMX start channel (`1..512`, uses 38 channels) | `1` |
| `-a, --artnet` | Use Art-Net UDP port `6454` instead of sACN `5568` | `sacn` |
| `-d, --display <idx>` | Target monitor index (`0`, `1`, etc.) | `-1` (cursor) |
| `-F, --fullscreen` | Launch in borderless Wayland desktop fullscreen | `false` |
| `-f, --fps <num>` | Target frame rate (`25..60`) | `50` |
| `-s, --smooth` | Enable linear texture filtering (`nearest` when off) | `false` |
| `--assets-dir <path>` | Custom path to `assets/` directory | `assets` |
| `--audio-dir <path>` | Custom path to `audio/` WAV directory | `audio` |
| `--audio-monitor <dev>` | ALSA playback device (e.g., `plughw:0,0`) | system default |
| `--config <file>` | Path to INI configuration file | `makingfaces.ini` |
| `-v, --version` | Print version and architecture credits | — |

---

## 5. Raspberry Pi 5 Wayland Autostart (Dedicated Portrait Display)

To launch MakingFaces automatically on a dedicated portrait monitor (such as `DSI-2` in `800x1280` portrait mode) under Raspberry Pi OS Wayland (`labwc`):

1. Create `~/.config/labwc/autostart` (if it does not exist):
```bash
mkdir -p ~/.config/labwc
nano ~/.config/labwc/autostart
```

2. Add the startup command:
```bash
cd /home/magnus/Documents/CPP/MakingFaces && ./MakingFaces --display 1 --fullscreen &
```

3. Make it executable:
```bash
chmod +x ~/.config/labwc/autostart
```
