# MakingFaces v1.0.0 — Complete User Manual & DMX Reference

## 1. Function Keys & Studio Shortcuts
- **F1:** Toggle Quick Help overlay
- **F2:** Toggle live Network / DMX / FPS Telemetry bar
- **F3:** Reset & resume Autonomous Memory Show from Slot #000
- **F4 / S:** Unified Save (active memory slot, all 14 layer offsets/scales, and `packs/default.zip`)
- **F5:** Toggle On-Screen Display (OSD) notifications
- **F6 / X:** Open ZIP Theme Pack Browser (thumbnails, inspect, append, replace, wipe)
- **F7 / Ctrl+Z:** Undo last pack operation (5-step stack)
- **F8:** Open Adafruit Eye Browser (210x210 live circular previews)
- **F9:** Toggle About & Architecture Credits
- **F11 / Double-Click:** Toggle Wayland borderless fullscreen
- **F12:** Toggle manual Blackout
- **Ctrl+I:** Open Calibration & Import Studio (`import/`)
- **Ctrl+P:** Open Live Camera & USB Microscope Viewfinder
- **Ctrl+E:** Export complete QLC+ `.qxw` show & virtual console workspace (`MakingFaces-Show.qxw`)
- **Ctrl+X:** Export current theme as `packs/custom_NNN.zip`
- **ESC / Q:** Close overlay or resume Auto Show / Quit

## 2. Character Layer & Expression Keys (`Shift` = backward)
- **F / R / N / W:** Face (`000..255`), Ears, Nose (`Ctrl+N` hide), Eyebrows (`Ctrl+W` hide)
- **J / G / H / C / P:** Beard/Jaw (`097..108` or any beard with `Ctrl+U` animates jaw), Glasses, Hair, Hat, Makeup
- **K:** Background Scene (`000..255`, 5-layer depth parallax)
- **A / O / I / E / L / T:** Animation (`00..64`, `00 = AUTO LOOP`), Eye Shape (`01..12`), Iris Size (`01..10`), Eyelashes (`00..15`), Lipstick (`00..09`), Teeth (`00..15`)
- **Y:** Toggle Dual Side-Profile Dialogue Mode (`Ctrl+TAB` switches active Left/Right character)
- **Ctrl+U:** Hide procedural mouth per favorite (saved per memory slot; animates active beard/jaw)

## 3. Memory Banks & 3-Depth FX / Sky / Fog / Weather Shortcuts
- **PageUp / PageDown:** Switch Memory Bank (`00..24`)
- **1..9, 0 / Ctrl+1..9, 0:** Recall memory slot (`#000..#240`) or save current look into slot
- **M / Shift+M:** Step memory slots `000..240`
- **V / Shift+V:** Adjust crossfade duration (`0.0s .. 2.0s`)
- **B / Shift+B:** Step all 23 3-depth FX, Sky, Fog & Weather effects (`1: Bat, 2: Fly, 3: Mosquito, 4: Bird, 5: Wasp, 6: Spider, 7: Butterfly, 8: UFO, 9: Cobweb, 10: Fog Back, 11: Ghost, 12: Snake, 13: Fire, 14: Lightning, 15: Mouse, 16: Stars, 17: Comets, 18: Aurora, 19: Rain, 20: Snowfall, 21: Fog Mid, 22: Fog Front, 23: All 3 Fog Layers`)
- **Ctrl+Shift+1 / 2 / 3:** Toggle `Fog Back`, `Fog Mid`, and `Fog Front` independently on any scene
- **Ctrl+Shift+S / C / A / R / W:** Mix Stars, Comets, Aurora, Rain, and Snowfall independently
- **Ctrl+B / Ctrl+Alt+0:** Clear all active FX, fog & weather overlays

## 4. 14-Layer Live Calibration (`TAB` + Arrows)
- **TAB / Shift+TAB:** Cycle `EYE_LEFT .. FX` (14 layers)
- **Arrows / Shift+Arrows:** Move X/Y (`2 px` / `10 px`)
- **Alt+Up/Down:** Scale `50%..160%`
- **Ctrl+Left/Right:** Adjust symmetrical split distance (`splitOffsetX`)

## 5. Art-Net & sACN DMX Modes (`--mode 1 | 2 | 7 | 39`)
- **Mode 1 (`1-Channel Memory`):** Ch 1 = Memory Recall / Hide (`0..9` Manual, `10..250` Memory `#000..#240`, `251..255` Hide flags).
- **Mode 2 (`2-Channel Memory + Dimmer`):** Ch 1 = Memory Recall / Hide, Ch 2 = Master Dimmer (`0 = 100% Full`, `255 = Blackout`).
- **Mode 3 (`7-Channel Show`):** Ch 1 = Memory Recall / Hide, Ch 2 = Master Dimmer, Ch 3 = Background Scene, Ch 4 = FX / Fog Overlay (`0..23`), Ch 5 = Audio Track, Ch 6 = Audio Volume, Ch 7 = Audio Trigger.
- **Mode 4 (`39-Channel Full`):**
  - **Ch 1–6:** Left Eye X, Y, Iris Size, Blink, Style (`0..63` Default, `64..127` Dragon, `+128` Lidless), Eye Size / 3D Pop-Out
  - **Ch 7–12:** Right Eye X, Y, Iris Size, Blink, Style, Eye Size / 3D Pop-Out
  - **Ch 13–15:** Eye Convergence (`128` = neutral), Eyebrow Tilt (`128` = neutral), Eyebrow PNG (`0..255`)
  - **Ch 16–22:** Mouth Gap, Smile/Frown, Width, Teeth (`0..15`), Tongue/Lick, Lip Thickness (`255`=hide), Lipstick (`0..9`)
  - **Ch 23–31:** Face, Ears, Nose, Beard/Jaw, Glasses, Hair, Hat, Makeup (`0..255`), Eyelash Style
  - **Ch 32–33:** Memory Recall / Hide (`10..250` = `#000..#240`) & Memory Control (`100..199` Live Edit, `200..255` Hold to Save)
  - **Ch 34–35:** Background Scene (`0..255`) & FX / Fog Overlay (`0..23`, `24..255` Loop)
  - **Ch 36–38:** Audio Track (`0..255`), Audio Volume (`0..255`), Audio Trigger (`128..255` Play)
  - **Ch 39:** Master Dimmer (`0 = 100% Full`, `255 = Blackout`)

## 6. Stereo System Audio & Dual-Profile Lip-Sync
- **Automatic System Loopback:** Whenever DMX audio is inactive, MakingFaces monitors the active PipeWire/PulseAudio `.monitor` source in **2-channel stereo** with automatic gain control (AGC) and a `600 ms` hold window.
- **Independent Left/Right Stereo Mouths:** In Dual Side-Profile Mode (`Y`), the **Left stereo channel** drives the **Left character's mouth** and the **Right stereo channel** drives the **Right character's mouth** independently.
- **DMX Audio Tracks (`001..255`):** Place `.wav` or `.mp3` files named `001.wav` / `001.mp3` inside `audio/` to trigger them via DMX (Mode 7 Ch 5–7 or Mode 39 Ch 36–38).
