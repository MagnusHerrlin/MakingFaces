# MakingFaces v1.0.0 — Complete User Manual & 38-Channel DMX Reference

---

## 1. Function Keys & Studio Shortcuts

| Key | Action |
| :--- | :--- |
| **`F1`** | Toggle built-in Quick Help overlay |
| **`F2`** | Toggle live Network / DMX / FPS Telemetry bar |
| **`F3`** | Reset & resume Autonomous Macro Show from Slot `#000` |
| **`F4` / `S`** | **Unified Save:** Save active macro slot, all 14 layer offsets/scales, and update `packs/default.zip` |
| **`F5`** | Toggle On-Screen Display (`OSD`) notifications |
| **`F6` / `X`** | Open **ZIP Theme Pack Browser** (thumbnails, inspect, append, replace, wipe) |
| **`F7` / `Ctrl + Z`** | **Undo:** Restore previous `packs/default.zip` snapshot (up to 5 undo levels) |
| **`F8`** | Open **Adafruit Eye Browser** (live 210x210 circular previews of built-in & custom eyes) |
| **`F9`** | Toggle About & Architecture Credits overlay |
| **`F11` / Double-Click** | Toggle Wayland borderless fullscreen |
| **`F12`** | Toggle manual Blackout |
| **`Ctrl + I`** | Open **Calibration & Import Studio** (`import/` folder) |
| **`Ctrl + P`** | Open **Live Camera & USB Microscope Viewfinder** |
| **`Ctrl + X`** | Export current `assets/` + `makingfaces.ini` as `packs/custom_NNN.zip` |
| **`ESC`** | Close active overlay/studio, exit `TAB` calibration, or resume Auto Show |
| **`Q`** | Quit MakingFaces |

---

## 2. Character Layer & Expression Keys

Hold **`Shift`** with any stepping key below to step **backward**.

| Key | Layer / Feature | Range |
| :--- | :--- | :--- |
| **`F`** | Face mask (`001..126` front, `127..255` left/right side profiles) | `000..255` |
| **`R`** | Ears | `000..255` |
| **`N`** | Nose (`Ctrl + N` toggles nose visibility) | `000..255` |
| **`W`** | Eyebrows (`Ctrl + W` toggles brow visibility) | `000..255` |
| **`J`** | Beard / Articulated Jaw (`097..108` syncs jaw drop to mouth gap) | `000..255` |
| **`G`** | Glasses / Eyewear | `000..255` |
| **`H`** | Hair | `000..255` |
| **`C`** | Cap / Hat / Crown | `000..255` |
| **`P`** | Face Paint / Makeup (auto-samples eyeshadow tint for eyelids) | `000..255` |
| **`K`** | 5-Layer Parallax Background Scene (`_sky`, `_clouds`, `_far`, `_mid`, `_front`) | `000..255` |
| **`A`** | Procedural Eye & Mouth Animation Preset (`00` = Auto Show) | `00..64` |
| **`O`** | Eye Shape & Eyelid Mode (Default/Dragon × Sly/Round/Almond + Lidless) | `01..12` / Auto |
| **`I`** | Iris Dilation Size (`Auto` when DMX is active) | `01..10` / Auto |
| **`E`** | Eyelash Style & Density | `00..15` |
| **`L`** | Lipstick Color Palette | `00..09` |
| **`T`** | 3D Tooth Preset (Normal, Mater, Dracula, Pirate, Shark, Grillz, Zombie, etc.) | `00..15` |
| **`Y`** | Toggle **Dual Side-Profile Dialogue Mode** (two inward-facing profiles take turns speaking) | `ON / OFF` |
| **`Ctrl + U`** | Toggle Procedural Mouth Visibility | `ON / OFF` |
---

## 3. Macro Banks & FX Overlay Shortcuts

| Key | Action |
| :--- | :--- |
| **`PageUp` / `PageDown`** | Switch active Macro Bank (`00..24`, covering slots `000..240`) |
| **`1..9`, `0`** | Recall & lock macro slot in the active bank with smooth crossfade |
| **`Ctrl + 1..9`, `0`** | Copy current character/animation directly into slot (`0..9`) of active bank & save |
| **`M` / `Shift + M`** | Step forward/backward through macro slots `000..240` |
| **`V` / `Shift + V`** | Increase/decrease macro crossfade duration (`0.0s .. 2.0s`) |
| **`Alt + 1..9`, `0`** | Toggle individual FX overlays (`1:Bat`, `2:Fly`, `3:Mosquito`, `4:Bird`, `5:Wasp`, `6:Spider`, `7:Butterfly`, `8:UFO`, `9:Cobweb`, `0:Fog`) |
| **`Ctrl + Alt + 0`** | Clear all active FX overlays |
| **`B` / `Shift + B`** | Cycle single FX overlay (`01..15` incl. Ghost, Snake, Fire, Lightning, Mouse) / Clear |

---

## 4. 14-Layer Live Calibration (`TAB` + Arrow Keys)

You can adjust position, scale, and symmetrical left/right split distance for **every single layer** live on screen and save the calibration per face mask (`[offsets_face_XXX]` in `makingfaces.ini`):

1. Press **`TAB`** (or **`Shift + TAB`**) to cycle through all **14 editable layers**:
   `EYE_LEFT` -> `EYE_RIGHT` -> `MOUTH` -> `NOSE` -> `GLASSES` -> `BROWS` -> `BEARD` -> `FACE` -> `EARS` -> `HAIR` -> `HAT` -> `MAKEUP` -> `BG` -> `FX`.
2. **Move (`Arrow Keys`):** Move the selected layer in `X` and `Y` (`2 px` per step, or `10 px` with **`Shift`**).
3. **Scale (`Alt + Up / Down`):** Resize the selected layer from **`50%` to `160%`** (`2%` per step, or `10%` with **`Shift`**).
4. **Split (`Ctrl + Left / Right`):** Increase or decrease symmetrical horizontal separation (`splitOffsetX`) on split layers (`MOUTH`, `NOSE`, `GLASSES`, `BROWS`, `BEARD`, `FACE`, `EARS`).
5. **Save (`S` or `F4`):** Writes all 14 offsets, splits, and scales into `makingfaces.ini` and updates `packs/default.zip`.

---

## 5. Camera (`Ctrl + P`), Import Studio (`Ctrl + I`) & Pack Browser (`F6`)

### Live Camera & USB Microscope (`Ctrl + P`)
* **`Space` / `Enter`:** Capture a 3:4 portrait photo (`import/cam_NNN.jpg`) and open it in the Calibration Studio.
* **`E`:** Capture a close-up eye iris photo (`import/eye_NNN.jpg`) and open it pre-set to `EYE_IRIS`.
* **`M`:** Toggle between full-head 3:4 portrait oval guide and circular microscope iris guide.
* **`R`:** Rotate live camera feed 90° clockwise (for vertically mounted webcams).
* **`Tab`:** Toggle between USB V4L2 camera (`/dev/video0..3`) and Raspberry Pi CSI (`rpicam-still`).

### Calibration & Import Studio (`Ctrl + I`)
* **Mouse Left-Click & Drag:** Click the canvas (green border) to position the image, or click any cyan eye/nose/mouth oval (turns **bright green**) to move that cutout/inpaint zone.
* **Mouse Wheel / `+` / `-`:** Scale the active image or the selected green oval guide.
* **`Z` / `X`:** Rotate image `-1.5°` / `+1.5°`.
* **`Left` / `Right`:** Select target folder (`FACE`, `EARS`, `NOSE`, `BROWS`, `BEARD`, `GLASSES`, `HAIR`, `HAT`, `MAKEUP`, `BG`, `EYE_IRIS`, `EYE_SCLERA`).
* **`W`:** Toggle automatic studio background removal.
* **`B` / `Shift + B`:** Increase / decrease background removal color tolerance (`8..140`).
* **`V`:** Toggle oval eye-socket transparency cutouts (`FACE` / `MAKEUP`).
* **`N`:** Toggle OpenCV Telea nose & mouth removal (`FACE` / `MAKEUP`).
* **`Space` / `A`:** Check/uncheck current item or toggle all items.
* **`Enter`:** Export checked items to `assets/`, move originals to `import/done/`, and update `packs/default.zip`.

### ZIP Theme Pack Browser (`F6` / `X`)
* **`Left` / `Right`:** Browse `.zip` theme packs in `packs/` with rendered composite thumbnails.
* **`I` / `Down`:** Open **Live Pack Inspector** to preview individual macros on stage (`Up`/`Down`), check items (`Space` / `A`), and import selected presets (`Enter`).
* **`Enter`:** Append all macros and assets from the selected `.zip` pack onto the end of your active library.
* **`R`:** Replace active theme with the selected `.zip` pack (creates automatic undo snapshot).
* **`Del` (twice):** Wipe all character/background assets and macros to start from a clean slate (preserves `assets/fx/`).
* **`F7` / `Ctrl + Z`:** Undo last pack import, replace, or wipe.
---

## 6. 38-Channel Art-Net & sACN DMX Channel Map

Set `--channel <1..512>` (default `1`). All offsets wrap cleanly within the 512-channel DMX universe.

| Offset | Ch (at 1) | Parameter | Value Mapping |
| :--- | :--- | :--- | :--- |
| `+0` | `1` | Left Eye X | `0..255` (`128` = center) |
| `+1` | `2` | Left Eye Y | `0..255` (`128` = center) |
| `+2` | `3` | Right Eye X | `0..255` (`128` = center) |
| `+3` | `4` | Right Eye Y | `0..255` (`128` = center) |
| `+4` | `5` | Iris Dilation | `0..255` (pinpoint to dilated) |
| `+5` | `6` | Eyelid Blink | `0` = open, `255` = closed |
| `+6` | `7` | Upper Eyelid | `0..255` manual upper lid position |
| `+7` | `8` | Lower Eyelid | `0..255` manual lower lid position |
| `+8` | `9` | Eye Style & Lid Shape | `0..63` Default, `64..127` Dragon (`+128` = lidless) |
| `+9` | `10` | Eyelid Custom Red | `0..255` |
| `+10` | `11` | Eyelid Custom Green | `0..255` |
| `+11` | `12` | Eyelid Custom Blue | `0..255` |
| `+12` | `13` | Eyebrow Tilt / Mood | `0` = angry, `128` = neutral, `255` = surprised |
| `+13` | `14` | Eyebrow PNG Index | `0` = off, `1..255` = `assets/brows/NNN.png` |
| `+14` | `15` | Mouth Open Gap | `0` = closed, `255` = wide open |
| `+15` | `16` | Mouth Smile / Frown | `0` = frown, `128` = neutral, `255` = big smile |
| `+16` | `17` | Mouth Width / Pucker | `0` = narrow pucker, `128` = normal, `255` = wide |
| `+17` | `18` | Tooth Style Preset | `0..255` divided into 16 presets (`0..15`) |
| `+18` | `19` | Tongue Protrusion / Lick | `0..160` stick out, `161..208` upper lip lick, `209..255` lower lip lick |
| `+19` | `20` | Lip Thickness / Hide | `0..254` fullness, `255` = hide procedural mouth |
| `+20` | `21` | Lipstick Color Preset | `0..255` mapped to 10 palettes (`0..9`) |
| `+21` | `22` | Face Mask Index | `0` = off, `1..126` front, `127..255` side profiles |
| `+22` | `23` | Ears Index | `0` = off, `1..255` = `assets/ears/NNN.png` |
| `+23` | `24` | Nose Index | `0` = off, `1..255` = `assets/nose/NNN.png` |
| `+24` | `25` | Beard / Articulated Jaw | `0` = off, `1..255` (`97..108` articulated jaw) |
| `+25` | `26` | Glasses Index | `0` = off, `1..255` = `assets/glasses/NNN.png` |
| `+26` | `27` | Hair Index | `0` = off, `1..255` = `assets/hair/NNN.png` |
| `+27` | `28` | Hat / Crown Index | `0` = off, `1..255` = `assets/hat/NNN.png` |
| `+28` | `29` | Face Paint / Makeup | `0` = off, `1..255` = `assets/makeup/NNN.png` |
| `+29` | `30` | Eyelash Style & Density | `0` = off, `1..255` Natural / Long / Winged / Dramatic |
| `+30` | `31` | Macro Recall Slot | `0..9` manual DMX, `10..250` = Macro `#000..#240`, `251..255` hide modes |
| `+31` | `32` | Macro Control / Save | `0..99` play, `100..199` live edit, `200..255` hold to save macro |
| `+32` | `33` | Background Scene Index | `0` = black, `1..255` = `assets/bg/NNN*.png` |
| `+33` | `34` | FX Overlay Trigger | `0` = off, `1..15` animated FX sprite |
| `+34` | `35` | Audio Track Index | `0` = stop, `1..255` = `audio/NNN.wav` |
| `+35` | `36` | Audio Volume | `0..255` |
| `+36` | `37` | Audio Trigger Mode | `0..127` one-shot on change, `128..255` loop |
| `+37` | `38` | Master Dimmer | `0` = blackout, `255` = full brightness |
