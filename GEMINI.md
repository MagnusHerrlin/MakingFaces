# MakingFaces v1.0.0 — Architecture & Engineering Reference

## 1. Code Formatting & Structural Standards
- **Section & Function Headers:** Organize every `.cpp` and `.h` file into lettered sections (`// Section A. ...`) and sequentially numbered functions (`// 1. ...`) enclosed in `// ------` comment banners.
- **Readable Multi-Line Formatting (No Minification):** Never compress multi-statement loops, conditionals, or functions onto single lines. Use clean multi-line indentation and always leave a blank line after a function's closing `}`.
- **GLOBAL_FACE_Y_OFFSET:** Set to `-50` in `main.h` so full-height hats, crowns, and headwear fit cleanly inside the `600x800` canvas without top clipping.

## 2. AI Collaboration, Chunking & Delivery Rules
- **3–4 Medium Blocks Per Turn:** Deliver code in 3–4 cleanly separated, self-contained blocks per response (~80–150 lines per block) so streaming stays fast without buffer stalls.
- **Always Deliver Complete Functions:** Never send single-line diffs or partial snippets; always state the exact **file**, **Section**, and **function number/name**, and send the entire function ready to paste.
- **Bracket-Safe C++ Indexing:** Avoid raw single-digit numeric array subscripts (`arr[0]`, `arr[1]`, `arr[2]`) in chat code blocks; use `.at(0)`, `.at(1)`, `.at(2)` or `std::array` so Markdown parsers never strip brackets.
- **Strict Response Cap (~220 Lines Total):** Never output more than ~220 lines of code in a single response across all blocks combined. If a function exceeds ~120 lines, deliver only that single function in its own response.

## 3. Asset Index Map & Dynamic Eye Bank
- `assets/face/`: `001..112` Original characters | `113..126` 3D characters | `127+` Side-Profile pairs (**Odd = Left Profile**, **Even = Right Profile**; Dual mode pairs `odd` + `even`).
- `assets/ears/`: `001..080` Original ears | `081..088` 3D ears | `089+` Profile ears (Odd = Left, Even = Right).
- `assets/nose/`: `001..083` Original sculpted noses | `084..095` 3D horns/snouts | `096+` Profile noses (Odd = Left, Even = Right).
- `assets/hat/`: `001..080` Original hats | `081..084` Perched 3D bats | `085+` Profile headwear.
- `assets/brows/`: `001..080` Stranded eyebrow shapes | `081+` Profile eyebrows.
- `assets/beard/`: `001..096` Stranded beards | `097..100` Articulated jaws (track `g_effective_mouth_gap`) | `101..108` 3D beards | `109+` Profile beards.
- `assets/glasses/`: `001..080` Original eyewear | `081+` Profile eyewear.
- `assets/hair/`: `001..096` Stranded hairstyles | `097+` Profile hairstyles.
- `assets/makeup/`: `001..104` Eyeshadows, tribal paint & blush | `105+` Append-only.
- `assets/bg/`: `001..008` 5-layer parallax (`_sky`, `_clouds`, `_far`, `_mid`, `_front`) & static 3D backdrops | `009+` Append-only.
- `assets/fx/`: 17 foreground sprites (`bat`, `fly`, `mosquito`, `bird`, `wasp`, `spider`, `butterfly`, `ufo`, `cobweb`, `fog_1`, `fog_2`, `ghost`, `snake`, `fire_1`, `fire_2`, `lightning_1`, `lightning_2`, `mouse`).
- `graphics/` & `assets/eyes/`: Dynamic Adafruit Eye Bank (`F8` Eye Browser). Supports `*-sclera` + `*-iris` pairs and Adafruit `*Eye/` folders (`catEye`, `goatEye`, `terminatorEye`, `newtEye`, `doeEye`, `naugaEye`, `noScleraEye`) with morphing slit-to-round polar tables (`0x8000` flag for dynamic slit eyes) and full 12-mode eyelid compatibility.

## 4. Runtime Controls, Memory Engine, Audio, Studios & DMX Contract
- **Memory & Animation Engine (`animation_engine.h` / `animation_engine.cpp`):**
  - `Memory #000..#240` (`Bank 00..24`, `PageUp`/`PageDown` + `1..9,0`) stores complete character recipes (`[memory_X]` in `makingfaces.ini`, with backward compatibility for `[macro_combo_X]`).
  - `Animation 0..64` (`A` / `Shift+A`): `0 = AUTO LOOP` cycles all 64 procedural expressions; `1..64` locks a single expression.
- **Stereo Audio & Lip-Sync Engine (`audio_engine.h` / `audio_engine.cpp`):**
  - Uses `SDL2_mixer` (`48 kHz`) for DMX-triggered `.wav` and `.mp3` playback (`001..255`) with post-mix envelope extraction.
  - Captures system audio via PipeWire/PulseAudio `parec` (`24 kHz`, `2ch` stereo) with adaptive AGC and a `600 ms` hold window when DMX audio is idle.
  - In **Dual Side-Profile Mode (`Y`)**, Left and Right stereo channels animate the Left and Right character mouths independently (`getAudioLeftEnvelope()` / `getAudioRightEnvelope()`).
- **3-Layer Seamless Fog & 23 FX Modes:**
  - `B` / `Shift+B` steps `0..23` (`10 = Fog Back`, `21 = Fog Mid`, `22 = Fog Front`, `23 = All 3 Fog Layers`); `Ctrl+Shift+1 / 2 / 3` toggles each fog depth layer independently.
- **4 Selectable DMX Modes (`--mode 1 | 2 | 7 | 39`, `MakingFaces.qxf`):**
  - `1` = 1-Channel Memory; `2` = 2-Channel Memory + Dimmer; `7` = 7-Channel Show; `39` = 39-Channel Full.
  - Inverted PWM Master Dimmer: `0 = 100% Full Brightness`, `255 = Blackout`.
  - Uses `SDL_RENDERER_SOFTWARE` in `run_face_engine` to avoid Raspberry Pi Wayland multi-monitor DRM CMA scanout allocation errors (`DRM_IOCTL_MODE_CREATE_DUMB`).
