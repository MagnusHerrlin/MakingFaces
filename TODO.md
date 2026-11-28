# MakingFaces - Project TODO & Completed Milestones (`v1.0.0` -> `v1.1.0`)
**Architecture & Design by Magnus Herrlin 2026**

## Active / Next Up (`v1.1.0` Roadmap)
- [ ] **QLC+ Fixture (`MakingFaces.qxf`) & DMX Channel Verification:**
  - [ ] Audit and align all 4 DMX modes (`1`, `2`, `7`, `39` channels), `MakingFaces.qxf`, and `MakingFaces-Show.qxw` export (`Ctrl+E`) with the 25-slot Stage FX Director and dual-profile channels.
- [ ] **Create curated `pack_default.cpp` (`packs/default.zip`):**
  - [ ] Build a 50-character starter pack (10 People of the World, 10 Apes, 10 Cats, 10 Halloween, 10 Christmas) without the dark vampire hair underlayer, using `GLOBAL_FACE_Y_OFFSET = -50`.
- [ ] **Universal Camera & Pi CSI Live Stream (`Ctrl+P`):**
  - [ ] Broad V4L2 device probing (`/dev/video0..15`) that automatically skips internal Raspberry Pi ISP/metadata nodes without standard capture formats.
  - [ ] Live Pi CSI viewfinder stream via `rpicam-vid` or GStreamer (`libcamerasrc`) directly into `cam.liveTex` so alignment ovals work in real time.
  - [ ] Multi-camera switcher (`TAB`) cycling through all detected USB webcams, microscopes, and CSI cameras.
- [ ] **In-App Audio Recording & Per-Memory Audio Tracks (`Ctrl+R`):**
  - [ ] Record voice clips directly inside MakingFaces via SDL2 audio capture or `parecord` with a live OSD timer (`REC 00:00`), saving automatically to the next free `audio/NNN.wav` slot.
  - [ ] Add `audio_track` (`0 = silent`, `1..255 = audio/NNN.wav`) to `MemoryPreset` and `makingfaces.ini`.
- [ ] **Face Recognition, Presence Wake-Up & Smart Display Sleep:**
  - [ ] Lightweight background OpenCV face detector (`FaceDetectorYN` / Haar cascade at 2-4 FPS) active during Blackout/Idle mode.
  - [ ] Local face recognition (`cv::face::LBPHFaceRecognizer`) trained on user portraits from `Ctrl+P` / `import/` to recognize Magnus.
  - [ ] Smooth wake-up fade-in (`fadePhase`), automatic loading of a designated greeting memory, spoken welcome greeting (`"Hej Magnus!"`), and optional `wlopm --on/--off DSI-2` backlight power management after inactivity.
- [ ] **Live AI Voice Conversation (Gemini API + STT/TTS + Mood Tags):**
  - [ ] Voice capture with Voice Activity Detection (VAD) and local `whisper.cpp` (or cloud STT) speech-to-text transcription.
  - [ ] Gemini API integration over HTTPS (`libcurl`) using per-memory character personas (`ai_prompt=` in `makingfaces.ini`).
  - [ ] Natural TTS synthesis (local **Piper TTS** or cloud TTS) routed through the existing PipeWire/PulseAudio `.monitor` loopback for automatic real-time lip-sync.
  - [ ] Parse hidden `[MOOD:...]` tags in AI responses (e.g., `[MOOD:LAUGH]`, `[MOOD:SURPRISED]`) to trigger matching animations in `animation_engine.cpp`.
- [ ] **Optional Bottom Chat HUD (`F10` / `Ctrl+T`):**
  - [ ] Semi-transparent bottom chat box displaying transcribed user speech and AI replies in real time, toggleable via `F10` / `Ctrl+T` and `show_chat=true/false`.
- [ ] **Additional Modular C++17 Theme Packs (`pack_utils.h` / `pack_utils.cpp`):**
  - [ ] **World Civilizations & Eras Pack (`pack_history.cpp`)** & **Sci-Fi & Cyber Robots Pack (`pack_scifi.cpp`)**.
- [ ] **Calibration Studio Enhancements (`Ctrl+I`):**
  - [ ] Optional side-profile pair generation (`127+`) directly from mirrored/angled camera captures.
- [ ] **WebUI / Mobile Remote Control (Bonus):**
  - [ ] Lightweight embedded HTTP server (port `8080`) to switch memories, toggle AI/chat/face-recognition modes, and send live TTS lines from a phone browser.

## Completed (`v1.0.0` Memory Engine, 3D Stage FX Director, 50 Side-Profile Packs & Studios)
- [x] **25-Slot 3D Stage FX Director & 4-Bit Binary Depth Masking (`stage_engine`, `stage_behaviors`, `stage_actors`):**
  - [x] Added Perlin flow fields, Reynolds Boids flocking swarms, 3-pose bat (`bat_1..3`), realistic nose-landing fly with exaggerated cross-eye convergence, sweeping UFO with tractor beam, fireflies, grasshoppers, ants, autumn leaves, fireworks, periodic 3D Perlin fog, snow, rain, stars, comets & aurora.
  - [x] Implemented unified 2-line FX Director OSD (`B` / `Shift+B`) with 4-bit binary depth toggles (`Alt+1..4` = `SKY/FAR/MID/FRONT`, `Alt+0` = Clear, `Ctrl+B` = All FX Off) and `Ctrl + +` / `Ctrl + -` swarm scaling (`1..50`).
- [x] **Unified Side-Profile Geometry (`face_split=75`, `mouth_split=75`) & 50 Profile Pairs Across Theme Packs:**
  - [x] Updated `pack_cats.cpp`, `pack_monkeys.cpp`, `pack_halloween.cpp`, `pack_people.cpp` (including grounded Nordic red cabin), `pack_misc.cpp`, and `pack_fx.cpp`.
- [x] **Memory & Bank Engine (`animation_engine.h` / `animation_engine.cpp`):**
  - [x] Implemented `Memory #000..#240` (`Bank 00..24`) and all 64 distinct procedural expressions (`Animation 0..64`, `0 = AUTO LOOP`) with 240 ms expression crossfades and backward compatibility for `[macro_combo_X]`.
- [x] **4 Selectable DMX Modes (`1`, `2`, `7`, `39` channels) & QLC+ Fixture (`MakingFaces.qxf`):**
  - [x] Implemented `1-Channel Memory`, `2-Channel Memory + Dimmer`, `7-Channel Show`, and `39-Channel Full` with inverted PWM Master Dimmer (`0 = 100%`, `255 = Blackout`) and QLC+ `.qxw` show export (`Ctrl+E`).
- [x] **Built-In Calibration & Import Studio (`Ctrl+I`) & Live Camera / USB Microscope (`Ctrl+P`):**
  - [x] Integrated OpenCV affine warp, mouse drag & scroll-wheel scaling, `Z`/`X` rotation, white-background flood-fill cutout (`W`), eye-socket cutout (`V`), Telea inpainting (`N`), and V4L2/CSI viewfinder.
- [x] **Theme Pack Browser & Live Inspector (`F6` / `X`), Eye Browser (`F8`) & 5-Step Undo (`F7`):**
  - [x] Implemented live `.zip` pack inspection, selective memory/asset import, append/replace, wipe, and 5-level undo stack.
