# MakingFaces v1.0.0 — Wayland/SDL2 Animatronic Face Compositor & Art-Net/sACN Engine

![C++17](https://shields.io)
![Platform](https://shields.io)
![SDL2](https://shields.io)
![DMX](https://shields.io)
![Lines of Code](https://shields.io)
![License](https://shields.io)


![C++17](https://shields.io)
![Platform](https://shields.io)
![SDL2](https://shields.io)
![DMX](https://shields.io)
![Lines of Code](https://shields.io)
![License](https://shields.io)


**Architecture & C++17 Engine by Magnus Herrlin (2026)**  
*Inspired by Bodmer `TFT_eSPI` framebuffer sprites and Adafruit `Uncanny_Eyes` polar coordinate eye rendering.*

MakingFaces is a real-time 600x800 (3:4 portrait) animatronic character compositor for Raspberry Pi 5 (Wayland/SDL2) and Linux desktops. It combines procedural dual-eye rendering, 3D anti-aliased lips with 16 tooth presets, 10-layer paper-doll PNG compositing, 5-layer depth parallax backdrops, 17 animated FX overlays, 38-channel Art-Net / sACN DMX control, and built-in camera/microscope capture and calibration tools.

---

## Core Features (v1.0.0)

* **Dual Procedural Eyes & Dynamic Eye Bank (`F8`):** Real-time polar-mapped iris/sclera rendering with 12 eyelid/style modes, 15 eyelash densities, 3D pop-out shading, critter gaze tracking, and hot-loadable custom `*-iris.png` + `*-sclera.png` pairs.
* **3D Anti-Aliased Mouth & Side-Profile Dialogue:** Smooth ARGB8888 lips with 10 lipstick palettes, 16 tooth presets (Dracula, Shark, Grillz, Braces, Zombie, etc.), animated 3D tongue, WAV lip-sync envelope follower, and automatic alternating two-character dialogue in Dual Profile mode (`Y`).
* **14-Layer Live Calibration (`TAB` + Arrows):** Move (`Arrows`), scale 50%–160% (`Alt + Up/Down`), and adjust symmetrical split distance (`Ctrl + Left/Right`) across all 14 layers (`EYE_LEFT`, `EYE_RIGHT`, `MOUTH`, `NOSE`, `GLASSES`, `BROWS`, `BEARD`, `FACE`, `EARS`, `HAIR`, `HAT`, `MAKEUP`, `BG`, `FX`) and save per-face profiles directly into `makingfaces.ini` and `packs/default.zip` (`S` / `F4`).
* **Built-In Calibration & Import Studio (`Ctrl + I`):** Import external `.png`, `.jpg`, `.jpeg`, or `.webp` images from `import/`, automatically strip studio backgrounds with adjustable corner/white tolerance (`B` / `Shift + B`), cut out oval eye sockets (`V`), and erase real noses/mouths with OpenCV Telea inpainting (`N`).
* **Live Camera & USB Microscope Viewfinder (`Ctrl + P`):** Capture 3:4 portrait face photos (`Space`/`Enter`) or close-up iris photos (`E`) directly from USB V4L2 webcams/microscopes or Raspberry Pi CSI cameras (`rpicam-still`) straight into the Import Studio.
* **ZIP Theme Pack Browser, Live Inspector & 5-Step Undo (`F6` / `X`):** Browse `.zip` theme packs with composite thumbnails, preview individual macros live (`I` / `Down`), selectively append or replace active themes, export new packs (`Ctrl + X`), and undo up to 5 changes (`F7` / `Ctrl + Z`).
* **38-Channel Art-Net & sACN (E1.31) DMX Receiver:** Sub-millisecond UDP receiver with 250 macro slots across 25 banks (`00..24`), smooth fade-to-black transitions, and autonomous offline show mode.
---

## How Capture, Import & Live Calibration Work Together

MakingFaces provides a complete three-stage pipeline from raw camera/image input to a saved, DMX-ready animatronic character:

### Stage 1: Capture (`Ctrl + P` — Camera & USB Microscope Viewfinder)
1. Press **`Ctrl + P`** to open the live viewfinder (probes `/dev/video0..3` for USB webcams/microscopes or uses Pi CSI `rpicam-still`).
2. Press **`R`** if your camera is mounted vertically (rotates 90° clockwise) and **`Tab`** to switch between USB V4L2 and Pi CSI.
3. **Face Mode (default):** Align your whole head (including hair and chin) inside the outer 3:4 portrait oval and press **`Space`** or **`Enter`**. MakingFaces crops to a true 3:4 portrait aspect ratio, saves `import/cam_NNN.jpg`, and opens the Calibration Studio.
4. **Microscope Eye Mode (`M` or `E`):** Hold a USB digital microscope over an iris, align it inside the circular reticle, and press **`E`**. MakingFaces saves `import/eye_NNN.jpg`, pre-selects `EYE_IRIS`, and opens the studio ready to export to `assets/eyes/`.

### Stage 2: Import & Cutout (`Ctrl + I` — Calibration & Import Studio)
1. Any image placed in `import/` (or captured via `Ctrl + P`) appears on a checkerboard transparency grid with anatomical alignment guides.
2. **Select Target Layer (`Left` / `Right`):** Choose `FACE`, `EARS`, `NOSE`, `BROWS`, `BEARD`, `GLASSES`, `HAIR`, `HAT`, `MAKEUP`, `BG`, `EYE_IRIS`, or `EYE_SCLERA`.
3. **Strip Background (`W` & `B` / `Shift + B`):** Corner-sampled + white flood-fill removes studio backgrounds while protecting white teeth and eyes inside the face. Press **`B`** to increase tolerance or **`Shift + B`** to decrease it.
4. **Fit & Cut Out (`Mouse Drag`, `Wheel`, `Z`/`X`, `V`, `N`):**
   * Click the canvas (green frame) to drag/zoom (`Wheel`) and rotate (`Z`/`X`) the image into the guide oval.
   * Click any cyan eye/nose/mouth oval (turns **bright green** when active) to drag or resize (`Wheel`) the eye cutouts (**`V`**) and OpenCV Telea nose/mouth erasure (**`N`**).
5. Press **`Enter`** to render the calibrated 600x800 (or 256x256 eye) transparent PNG into `assets/<category>/`, archive the source file to `import/done/`, and update `packs/default.zip`.

### Stage 3: Live Multi-Layer Calibration (`TAB` + Arrows — On-Canvas Alignment)
1. Once your new face or prop is active on screen (e.g., via `F` for Face or `N` for Nose), press **`TAB`** (or **`Shift + TAB`**) to cycle through all **14 live layers**:  
   `EYE_LEFT` -> `EYE_RIGHT` -> `MOUTH` -> `NOSE` -> `GLASSES` -> `BROWS` -> `BEARD` -> `FACE` -> `EARS` -> `HAIR` -> `HAT` -> `MAKEUP` -> `BG` -> `FX`.
2. **Move (`Arrow Keys`):** Nudge the active layer in `X` and `Y` (hold **`Shift`** for 10 px steps).
3. **Scale (`Alt + Up / Down`):** Resize the active layer from **50% to 160%** (hold **`Shift`** for 10% steps) so custom faces, noses, hats, or backgrounds match every other prop seamlessly.
4. **Split (`Ctrl + Left / Right`):** Adjust symmetrical left/right spacing (`splitOffsetX`) for split layers and side-profile masks.
5. **Save (`S` or `F4`):** Saves all 14 layer offsets/scales to `[offsets_face_XXX]` in `makingfaces.ini`, stores the active macro slot, and updates `packs/default.zip`.

---

## Documentation Index

* **[INSTALL.md](INSTALL.md)** — Dependencies, compilation (`make -j4`), Raspberry Pi 5 Wayland autostart, and CLI options.
* **[USER_MANUAL.md](USER_MANUAL.md)** — Complete keyboard shortcut reference, 38-channel DMX map, and browser workflows.
* **[MANUAL.html](MANUAL.html)** — Interactive offline HTML manual with searchable DMX tables and visual guides.
