# MakingFaces v1.0.0 — Installation & Raspberry Pi 5 Setup Guide

This guide covers building **MakingFaces v1.0.0** on Raspberry Pi OS (Bookworm/Trixie with Wayland/labwc) and Debian/Ubuntu Linux desktops.

---

## 1. Install Required System Packages

```bash
sudo apt update
sudo apt install -y \
  build-essential \
  g++ \
  make \
  pkg-config \
  libsdl2-dev \
  libsdl2-image-dev \
  libsdl2-mixer-dev \
  libopencv-dev \
  pulseaudio-utils \
  zip \
  unzip \
  alsa-utils \
  librsvg2-bin
```

---

## 2. Clone & Compile MakingFaces

```bash
git clone https://github.com
cd MakingFaces
make clean && make -j4
./MakingFaces --version
```

Expected output:
```text
MakingFaces v1.0.0 (Architecture by Magnus Herrlin 2026 - Bodmer TFT_eSPI & Adafruit Uncanny_Eyes inspired)
```

---

## 3. Install QLC+ Fixture Definition (`MakingFaces.qxf`)

Copy `MakingFaces.qxf` into your QLC+ user fixtures folder to unlock all 4 DMX modes (`1-Channel Memory`, `2-Channel Memory + Dimmer`, `7-Channel Show`, `39-Channel Full`):

```bash
mkdir -p ~/.qlcplus/fixtures
rm -f ~/.qlcplus/fixtures/MakingFaces*.qxf
cp MakingFaces.qxf ~/.qlcplus/fixtures/
```

---

## 4. Command-Line Options

| Flag | Description | Default |
| :--- | :--- | :--- |
| `-u, --universe <num>` | Art-Net or sACN DMX universe (`1..63999`) | `1` |
| `-c, --channel <num>` | DMX start channel (`1..512`) | `1` |
| `-m, --mode <1\|2\|7\|39>` | DMX fixture mode (`1`, `2`, `7`, or `39` channels) | `39` |
| `-a, --artnet` | Use Art-Net UDP port `6454` instead of sACN `5568` | `sacn` |
| `-d, --display <idx>` | Target monitor index (`0`, `1`, `2`, etc.) | `-1` (cursor) |
| `-F, --fullscreen` | Launch in borderless Wayland desktop fullscreen | `false` |
| `-f, --fps <num>` | Target frame rate (`25..60`) | `50` |
| `-s, --smooth` | Enable linear texture filtering (`nearest` when off) | `false` |
| `--assets-dir <path>` | Custom path to `assets/` directory | `assets` |
| `--audio-dir <path>` | Custom path to `audio/` `.wav`/`.mp3` directory | `audio` |
| `--audio-monitor <src>` | PipeWire/PulseAudio `.monitor` source hint for stereo lip-sync | `Monitor` (auto-detect) |
| `--config <file>` | Path to INI configuration file | `makingfaces.ini` |
| `-v, --version` | Print version and architecture credits | — |

---

## 5. Raspberry Pi 5 Wayland Autostart (`labwc`)

1. Edit `~/.config/labwc/autostart`:
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
