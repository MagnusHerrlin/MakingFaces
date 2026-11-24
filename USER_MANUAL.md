# MakingFaces v1.0.0 — Complete User Manual & 38-Channel DMX Reference

## 1. Function Keys & Studio Shortcuts
- **F1:** Toggle Quick Help overlay
- **F2:** Toggle live Network / DMX / FPS Telemetry bar
- **F3:** Reset & resume Autonomous Macro Show from Slot #000
- **F4 / S:** Unified Save (active macro, all 14 layer offsets/scales, and packs/default.zip)
- **F5:** Toggle On-Screen Display (OSD) notifications
- **F6 / X:** Open ZIP Theme Pack Browser (thumbnails, inspect, append, replace, wipe)
- **F7 / Ctrl+Z:** Undo last pack operation (5-step stack)
- **F8:** Open Adafruit Eye Browser (210x210 live circular previews)
- **F9:** Toggle About & Architecture Credits
- **F11 / Double-Click:** Toggle Wayland borderless fullscreen
- **F12:** Toggle manual Blackout
- **Ctrl+I:** Open Calibration & Import Studio (`import/`)
- **Ctrl+P:** Open Live Camera & USB Microscope Viewfinder
- **Ctrl+X:** Export current theme as `packs/custom_NNN.zip`
- **ESC / Q:** Close overlay or resume Auto Show / Quit

## 2. Character Layer & Expression Keys (`Shift` = backward)
- **F / R / N / W:** Face (`000..255`), Ears, Nose (`Ctrl+N` hide), Eyebrows (`Ctrl+W` hide)
- **J / G / H / C / P:** Beard/Jaw (`097..108` or any beard with `Ctrl+U` animates jaw), Glasses, Hair, Hat, Makeup
- **K:** Background Scene (`000..255`, 5-layer depth parallax)
- **A / O / I / E / L / T:** Animation (`00..64`), Eye Shape (`01..12`), Iris Size (`01..10`), Eyelashes (`00..15`), Lipstick (`00..09`), Teeth (`00..15`)
- **Y:** Toggle Dual Side-Profile Dialogue Mode
- **Ctrl+U:** Hide procedural mouth per favorite (saved per macro slot; animates active beard/jaw)

## 3. Macro Banks & 3-Depth FX / Sky / Weather Shortcuts
- **PageUp / PageDown:** Switch Macro Bank (`00..24`)
- **1..9, 0 / Ctrl+1..9, 0:** Recall slot or copy current look into slot
- **M / Shift+M:** Step macro slots `000..240`
- **V / Shift+V:** Adjust crossfade duration (`0.0s .. 2.0s`)
- **B / Shift+B:** Step all 20 3-depth FX, Sky & Weather effects (`Bat, Fly [lands on nose], Mosquito, Bird [crosses off-screen], Wasp, Spider, Butterfly, UFO, Cobweb, Fog, Ghost, Snake, Fire, Lightning, Mouse [random mid-blur/front], Stars, Comets, Aurora, Rain, Snowfall`)
- **Ctrl+Shift+S / C / A / R / W:** Mix Stars, Comets, Aurora, Rain, and Snowfall independently on any background
- **Ctrl+B / Ctrl+Alt+0:** Clear all active FX & weather overlays

## 4. 14-Layer Live Calibration (`TAB` + Arrows)
- **TAB / Shift+TAB:** Cycle `EYE_LEFT .. FX` (14 layers)
- **Arrows / Shift+Arrows:** Move X/Y (`2 px` / `10 px`)
- **Alt+Up/Down:** Scale `50%..160%`
- **Ctrl+Left/Right:** Adjust symmetrical split distance (`splitOffsetX`)

## 5. 38-Channel Art-Net & sACN DMX Map (Start Channel = 1)
- **Ch 1–4:** Left/Right Eye X & Y (`128` = center)
- **Ch 5–8:** Iris Dilation, Blink, Upper Lid, Lower Lid
- **Ch 9–12:** Eye Style (`0..63` Default, `64..127` Dragon, `+128` Lidless) & Eyelid RGB Tint
- **Ch 13–14:** Eyebrow Tilt (`128` = neutral) & Brow PNG Index (`0..255`)
- **Ch 15–21:** Mouth Gap, Smile/Frown, Width, Teeth (`0..15`), Tongue/Lick, Lip Thickness (`255`=hide), Lipstick (`0..9`)
- **Ch 22–29:** Face, Ears, Nose, Beard/Jaw, Glasses, Hair, Hat, Makeup (`0..255`)
- **Ch 30:** Eyelash Style (`0..255`)
- **Ch 31–32:** Macro Recall (`10..250` = `#000..#240`) & Macro Control (`100..199` edit, `200..255` hold to save)
- **Ch 33–34:** Background Scene (`0..255`) & FX Overlay Trigger (`0..20`)
- **Ch 35–37:** Audio Track (`0..255`), Volume (`0..255`), Trigger (`0..127` once, `128..255` loop)
- **Ch 38:** Master Dimmer (`0` = blackout, `255` = full)
