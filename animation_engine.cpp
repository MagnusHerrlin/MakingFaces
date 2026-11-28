// ==============================================================================
// MakingFaces - Procedural 64-Animation & DMX Memory Show Engine
// Architecture designed by Magnus Herrlin 2026
// ==============================================================================

// ==============================================================================
// Section A. Includes, Internal State & 64-Animation Name Registry
// ==============================================================================
#include "audio_engine.h"
#include "animation_engine.h"
#include "eye_functions.h"
#include "face_engine.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>

namespace {

constexpr uint32_t AUTO_SHOW_SLOT_DURATION_MS = 12000;

int      g_active_memory_slot   = 0;
uint32_t g_last_slot_switch_ms  = 0;
size_t   g_seq_list_cursor      = 0;
uint8_t  g_hide_flags_mask      = 0;

constexpr std::array<const char*, 65> kAnimationNames = {{
  "AUTO LOOP",
  "WINK & GRIN",
  "LAUGHING SHARK",
  "ANGRY VAMPIRE",
  "SPOOKY GHOST",
  "PIRATE LEER",
  "SLEEPY YAWN",
  "SURPRISED POP",
  "DIZZY ROLL",
  "SHY BLUSH",
  "COCKY ROGUE",
  "EVIL LAUGH",
  "CHATTY TALKER",
  "CURIOUS LOOK",
  "HYPER BLINK",
  "SAD POUT",
  "SLY LICK",
  "DOUBLE WINK",
  "MONSTER ROAR",
  "GOOGLY SPIN",
  "HEROIC SMILE",
  "ZOMBIE GROAN",
  "FLIRTY WINK",
  "SHOCKED GASP",
  "SASSY SMIRK",
  "DREAMY GAZE",
  "FIERCE GROWL",
  "TICKLED LAUGH",
  "SUSPICIOUS SQUINT",
  "JOYFUL BEAM",
  "CRAZY EYE",
  "GRUMPY SCOWL",
  "CHEWING GUM",
  "WHISTLING TUNE",
  "SNEEZE BUILDUP",
  "HYPNOTIC SWIRL",
  "PROUD CHIN",
  "PANIC LOOK",
  "COOL NOD",
  "GIGGLING FIT",
  "BORED SIGH",
  "HUNGRY LICK",
  "STARE DOWN",
  "WOBBLE JAW",
  "SHIVERING COLD",
  "DAYDREAM FLOAT",
  "NERDY BLINK",
  "VAMPIRE BITE",
  "FUNKY GROOVE",
  "DRAMATIC GASP",
  "MISCHIEVOUS GRIN",
  "WIDE WONDER",
  "FROWN TWITCH",
  "TONGUE WAG",
  "SLOW BLINK",
  "EXCITED BOUNCE",
  "SNEAKY GLANCE",
  "BEAMING PRIDE",
  "STARTLED JUMP",
  "RELAXED BREATHE",
  "DISCO FEVER",
  "KING ROYAL",
  "CYBORG GLITCH",
  "JACK-O-LANTERN",
  "SUPERHERO POSE"
}};

} // namespace

// ==============================================================================
// Section B. Animation State Initialization, Slot Decoding & Hide-Flag Queries
// ==============================================================================

// ------------------------------------------------------------------------------
// 1. Initialize animation engine state and clear active hide flags
// ------------------------------------------------------------------------------
void initAnimationEngine(void)
{
  g_active_memory_slot  = 0;
  g_last_slot_switch_ms = 0;
  g_seq_list_cursor     = 0;
  g_hide_flags_mask     = 0;
}

// ------------------------------------------------------------------------------
// 2. Reset autonomous memory/animation show sequence back to slot #000
// ------------------------------------------------------------------------------
void resetAutonomousShow(void)
{
  g_active_memory_slot  = 0;
  g_last_slot_switch_ms = 0;
  g_seq_list_cursor     = 0;
  g_hide_flags_mask     = 0;
}

// ------------------------------------------------------------------------------
// 3. Return true when DMX memory channel (Ch 32) activates a memory or hide mode
// ------------------------------------------------------------------------------
bool isDmxMemoryActive(uint8_t rawMemVal)
{
  return (rawMemVal >= 10);
}

// ------------------------------------------------------------------------------
// 4. Decode 0-based memory slot index (#000..#240) from DMX memory channel value
// ------------------------------------------------------------------------------
int decodeDmxMemorySlot(uint8_t rawMemVal)
{
  if (rawMemVal < 10 || rawMemVal > 250) {
    return -1;
  }
  return static_cast<int>(rawMemVal - 10);
}

// ------------------------------------------------------------------------------
// 5. Return currently active memory slot number
// ------------------------------------------------------------------------------
int getActiveMemorySlot(void)
{
  return g_active_memory_slot;
}

// ------------------------------------------------------------------------------
// 6. Return human-readable display name for animation ID (0..64)
// ------------------------------------------------------------------------------
const char* getAnimationName(int animId)
{
  if (animId < 0 || animId >= static_cast<int>(kAnimationNames.size())) {
    return "CUSTOM ANIM";
  }
  return kAnimationNames.at(static_cast<size_t>(animId));
}

// ------------------------------------------------------------------------------
// 7. Query active hide-part flags set by DMX values 251..255
// ------------------------------------------------------------------------------
bool isMouthHiddenByMemory(void)
{
  return ((g_hide_flags_mask & 0x01u) != 0);
}

bool isNoseHiddenByMemory(void)
{
  return ((g_hide_flags_mask & 0x02u) != 0);
}

bool isBrowsHiddenByMemory(void)
{
  return ((g_hide_flags_mask & 0x04u) != 0);
}

bool isFaceHiddenByMemory(void)
{
  return ((g_hide_flags_mask & 0x08u) != 0);
}
// ==============================================================================
// Section C. Procedural 64-Animation Waveform Generator
// ==============================================================================

// ------------------------------------------------------------------------------
// 8. Clamp floating-point value into 0..255 DMX byte range
// ------------------------------------------------------------------------------
static inline uint8_t clampByte(float v)
{
  int iv = static_cast<int>(std::round(v));
  return static_cast<uint8_t>(std::max(0, std::min(255, iv)));
}

// ------------------------------------------------------------------------------
// 9. Compute natural periodic blink envelope (0 = open, 255 = closed)
// ------------------------------------------------------------------------------
static uint8_t evalPeriodicBlink(uint32_t nowMs, uint32_t periodMs, uint32_t blinkDurMs)
{
  if (periodMs == 0 || blinkDurMs == 0) {
    return 0;
  }
  uint32_t phase = nowMs % periodMs;
  if (phase >= blinkDurMs) {
    return 0;
  }
  float u = static_cast<float>(phase) / static_cast<float>(blinkDurMs);
  return clampByte(std::sin(u * 3.14159265f) * 255.0f);
}

// ------------------------------------------------------------------------------
// 10. Execute procedural eye, eyebrow and mouth animation (0 = auto-cycle 1..64)
// ------------------------------------------------------------------------------
void runAnimationById(int animId, uint32_t nowMs)
{
  int resolvedId = (animId <= 0) ? (static_cast<int>((nowMs / 6000U) % 64U) + 1) : animId;
  int id = ((resolvedId - 1) % 64 + 64) % 64 + 1;

  static int      s_lastId       = -1;
  static uint32_t s_switchMs     = 0;
  static float    s_prevLx       = 128.0f, s_prevLy = 128.0f, s_prevRx = 128.0f, s_prevRy = 128.0f;
  static float    s_prevBrowL    = 128.0f, s_prevBrowR = 128.0f;
  static float    s_prevGap      = 16.0f,  s_prevCurve = 155.0f, s_prevWidth = 136.0f;
  static float    s_curLx        = 128.0f, s_curLy = 128.0f, s_curRx = 128.0f, s_curRy = 128.0f;
  static float    s_curBrowL     = 128.0f, s_curBrowR = 128.0f;
  static float    s_curGap       = 16.0f,  s_curCurve = 155.0f, s_curWidth = 136.0f;

  if (id != s_lastId) {
    if (s_lastId != -1) {
      s_prevLx    = s_curLx;    s_prevLy    = s_curLy;
      s_prevRx    = s_curRx;    s_prevRy    = s_curRy;
      s_prevBrowL = s_curBrowL; s_prevBrowR = s_curBrowR;
      s_prevGap   = s_curGap;   s_prevCurve = s_curCurve;
      s_prevWidth = s_curWidth;
      s_switchMs  = nowMs;
    }
    s_lastId = id;
  }

  float t = static_cast<float>(nowMs) * 0.001f;

  float lx = 128.0f + std::sin(t * 1.15f) * 42.0f;
  float ly = 128.0f + std::cos(t * 0.85f) * 26.0f;
  float rx = lx;
  float ry = ly;

  uint8_t irisL  = 150;
  uint8_t irisR  = 150;
  uint8_t blinkL = evalPeriodicBlink(nowMs, 3800, 170);
  uint8_t blinkR = blinkL;
  uint8_t sizeL  = 124;
  uint8_t sizeR  = 124;

  float browL  = 128.0f + std::sin(t * 1.4f) * 22.0f;
  float browR  = browL;

  float mGap   = 16.0f + std::max(0.0f, std::sin(t * 2.1f)) * 42.0f;
  float mCurve = 155.0f + std::sin(t * 1.3f) * 24.0f;
  float mWidth = 136.0f;
  uint8_t mTeeth = 16;
  uint8_t mTong  = 0;
  uint8_t mThick = 128;

  switch (id) {
    case 1: // WINK & GRIN
      blinkR = evalPeriodicBlink(nowMs, 2600, 340);
      mCurve = 195.0f;
      mGap   = 38.0f;
      break;
    case 2: // LAUGHING SHARK
      mGap   = 90.0f + std::fabs(std::sin(t * 6.5f)) * 155.0f;
      mCurve = 210.0f;
      mTeeth = 80;
      browL  = 92.0f;
      browR  = 92.0f;
      break;
    case 3: // ANGRY VAMPIRE
      browL  = 34.0f;
      browR  = 34.0f;
      mCurve = 75.0f;
      mGap   = 70.0f + std::sin(t * 3.0f) * 45.0f;
      mTeeth = 48;
      irisL  = 105;
      irisR  = 105;
      break;
    case 4: // SPOOKY GHOST
      lx = 128.0f + std::sin(t * 2.4f) * 85.0f; rx = lx;
      ly = 155.0f + std::cos(t * 3.2f) * 45.0f; ry = ly;
      mGap   = 130.0f + std::sin(t * 4.0f) * 80.0f;
      mCurve = 85.0f;
      mWidth = 88.0f;
      break;
    case 5: // PIRATE LEER
      blinkL = 210;
      mCurve = 185.0f;
      mTeeth = 64;
      mGap   = 68.0f;
      break;
    case 6: // SLEEPY YAWN
      blinkL = clampByte(140.0f + std::sin(t * 0.9f) * 95.0f);
      blinkR = blinkL;
      mGap   = std::max(0.0f, std::sin(t * 0.9f)) * 210.0f;
      mWidth = 96.0f;
      break;
    case 7: // SURPRISED POP
      sizeL  = 215; sizeR  = 215;
      blinkL = 0;   blinkR = 0;
      browL  = 225.0f; browR = 225.0f;
      mGap   = 185.0f; mWidth = 84.0f;
      break;
    case 8: // DIZZY ROLL
      lx = 128.0f + std::cos(t * 5.2f) * 95.0f;
      ly = 128.0f + std::sin(t * 5.2f) * 95.0f;
      rx = 128.0f - std::cos(t * 5.2f) * 95.0f;
      ry = 128.0f - std::sin(t * 5.2f) * 95.0f;
      mCurve = 105.0f;
      mGap   = 55.0f;
      break;
    case 9: // SHY BLUSH
      lx = 85.0f; rx = 85.0f; ly = 95.0f; ry = 95.0f;
      browL  = 175.0f; browR = 175.0f;
      mCurve = 178.0f; mGap  = 18.0f;
      break;
    case 10: // COCKY ROGUE
      browL  = 70.0f; browR = 185.0f;
      mCurve = 192.0f; mWidth = 152.0f; mGap = 32.0f;
      break;
    case 11: // EVIL LAUGH
      browL  = 28.0f; browR = 28.0f;
      mGap   = 110.0f + std::fabs(std::sin(t * 7.5f)) * 140.0f;
      mCurve = 215.0f; mTeeth = 160;
      break;
    case 12: // CHATTY TALKER
      mGap   = 25.0f + std::fabs(std::sin(t * 8.2f)) * 165.0f;
      mWidth = 115.0f + std::cos(t * 6.1f) * 35.0f;
      mCurve = 155.0f;
      break;
    case 13: // CURIOUS LOOK
      lx = 128.0f + std::sin(t * 1.6f) * 78.0f; rx = lx;
      ly = 148.0f; ry = 148.0f;
      browL  = 190.0f; browR = 145.0f;
      mGap   = 44.0f; mWidth = 104.0f;
      break;
    case 14: // HYPER BLINK
      blinkL = evalPeriodicBlink(nowMs, 650, 140);
      blinkR = blinkL;
      mCurve = 172.0f;
      break;
    case 15: // SAD POUT
      lx = 128.0f; rx = 128.0f; ly = 92.0f; ry = 92.0f;
      browL  = 198.0f; browR = 198.0f;
      mCurve = 52.0f;  mGap  = 22.0f; mThick = 165;
      break;
    case 16: // SLY LICK
      mGap   = 65.0f;
      mTong  = clampByte(162.0f + (0.5f + 0.5f * std::sin(t * 3.4f)) * 45.0f);
      mCurve = 175.0f;
      break;
    case 17: // DOUBLE WINK
      blinkL = evalPeriodicBlink(nowMs, 2400, 260);
      blinkR = evalPeriodicBlink(nowMs + 1200, 2400, 260);
      mCurve = 190.0f; mGap = 35.0f;
      break;
    case 18: // MONSTER ROAR
      browL  = 20.0f; browR = 20.0f;
      sizeL  = 185;   sizeR = 185;
      mGap   = 160.0f + std::sin(t * 4.5f) * 85.0f;
      mTeeth = 128;   mCurve = 60.0f;
      break;
    case 19: // GOOGLY SPIN
      lx = 128.0f + std::cos(t * 7.0f) * 105.0f;
      ly = 128.0f + std::sin(t * 7.0f) * 105.0f;
      rx = 128.0f + std::cos(t * 7.0f + 3.14f) * 105.0f;
      ry = 128.0f + std::sin(t * 7.0f + 3.14f) * 105.0f;
      mTong  = 135; mGap = 145.0f;
      break;
    case 20: // HEROIC SMILE
      browL  = 148.0f; browR = 148.0f;
      mCurve = 212.0f; mWidth = 168.0f; mGap = 48.0f; mTeeth = 16;
      break;
    case 21: // ZOMBIE GROAN
      lx = 110.0f; rx = 155.0f; ly = 165.0f; ry = 95.0f;
      mTeeth = 240; mGap = 65.0f + std::sin(t * 1.8f) * 45.0f; mCurve = 78.0f;
      break;
    case 22: // FLIRTY WINK
      blinkL = evalPeriodicBlink(nowMs, 2200, 380);
      lx = 152.0f; rx = 152.0f;
      browL  = 168.0f; browR = 148.0f;
      mCurve = 198.0f; mWidth = 126.0f; mGap = 28.0f; mThick = 175;
      break;
    case 23: // SHOCKED GASP
      sizeL  = 196; sizeR = 196; blinkL = 0; blinkR = 0;
      irisL  = 92;  irisR = 92;
      browL  = 236.0f; browR = 236.0f;
      mGap   = 195.0f; mWidth = 76.0f; mCurve = 115.0f;
      break;
    case 24: // SASSY SMIRK
      browL  = 64.0f; browR = 194.0f;
      lx = 165.0f; rx = 165.0f;
      mCurve = 186.0f; mWidth = 144.0f; mGap = 16.0f;
      break;
    case 25: // DREAMY GAZE
      lx = 128.0f + std::sin(t * 0.7f) * 35.0f; rx = lx;
      ly = 168.0f; ry = 168.0f;
      blinkL = 75; blinkR = 75;
      browL  = 160.0f; browR = 160.0f;
      mCurve = 176.0f; mGap = 26.0f;
      break;
    case 26: // FIERCE GROWL
      browL  = 18.0f; browR = 18.0f;
      irisL  = 96;    irisR = 96;
      mGap   = 78.0f + std::sin(t * 9.0f) * 22.0f;
      mCurve = 68.0f; mWidth = 162.0f; mTeeth = 144;
      break;
    case 27: // TICKLED LAUGH
      blinkL = 135; blinkR = 135;
      browL  = 175.0f; browR = 175.0f;
      mGap   = 95.0f + std::fabs(std::sin(t * 8.5f)) * 125.0f;
      mCurve = 224.0f; mWidth = 164.0f;
      break;
    case 28: // SUSPICIOUS SQUINT
      blinkL = 125; blinkR = 125;
      lx = 128.0f + std::sin(t * 1.4f) * 86.0f; rx = lx;
      browL  = 54.0f; browR = 74.0f;
      mCurve = 112.0f; mWidth = 112.0f; mGap = 10.0f;
      break;
    case 29: // JOYFUL BEAM
      sizeL  = 148; sizeR = 148;
      browL  = 182.0f; browR = 182.0f;
      mCurve = 232.0f; mWidth = 172.0f; mGap = 62.0f; mTeeth = 16;
      break;
    case 30: // CRAZY EYE
      irisL  = 230; irisR = 78;
      sizeL  = 185; sizeR = 102;
      lx = 128.0f + std::sin(t * 4.5f) * 80.0f;
      rx = 128.0f - std::cos(t * 3.8f) * 80.0f;
      mGap   = 115.0f; mCurve = 140.0f; mTong = 110;
      break;
    case 31: // GRUMPY SCOWL
      browL  = 24.0f; browR = 24.0f;
      ly = 146.0f; ry = 146.0f;
      mCurve = 44.0f; mWidth = 124.0f; mGap = 12.0f; mThick = 155;
      break;
    case 32: // CHEWING GUM
      mGap   = 20.0f + std::fabs(std::sin(t * 4.8f)) * 55.0f;
      mWidth = 105.0f + std::sin(t * 4.8f) * 38.0f;
      mCurve = 150.0f;
      break;
    case 33: // WHISTLING TUNE
      lx = 128.0f + std::sin(t * 1.8f) * 36.0f; rx = lx;
      ly = 152.0f; ry = 152.0f;
      browL  = 164.0f; browR = 164.0f;
      mWidth = 54.0f;  mGap = 52.0f + std::sin(t * 5.0f) * 16.0f;
      mCurve = 145.0f; mThick = 185;
      break;
    case 34: { // SNEEZE BUILDUP
      float ph = std::fmod(t, 4.0f);
      if (ph < 2.8f) {
        float b = ph / 2.8f;
        blinkL = clampByte(b * 180.0f); blinkR = blinkL;
        browL  = 128.0f + b * 85.0f; browR = browL;
        mGap   = 20.0f + b * 95.0f; mWidth = 96.0f;
      } else {
        blinkL = 245; blinkR = 245;
        browL  = 32.0f; browR = 32.0f;
        mGap   = 185.0f; mWidth = 165.0f; mCurve = 75.0f;
      }
      break;
    }
    case 35: // HYPNOTIC SWIRL
      lx = 128.0f + std::cos(t * 3.6f) * 82.0f;
      ly = 128.0f + std::sin(t * 3.6f) * 82.0f;
      rx = lx; ry = ly;
      irisL = clampByte(140.0f + std::sin(t * 6.0f) * 90.0f);
      irisR = irisL;
      mGap  = 64.0f; mWidth = 98.0f;
      break;
    case 36: // PROUD CHIN
      ly = 104.0f; ry = 104.0f;
      browL  = 152.0f; browR = 152.0f;
      mCurve = 196.0f; mWidth = 154.0f; mGap = 20.0f;
      break;
    case 37: // PANIC LOOK
      lx = 128.0f + std::sin(t * 9.5f) * 98.0f; rx = lx;
      ly = 128.0f + std::cos(t * 7.5f) * 65.0f; ry = ly;
      sizeL  = 195; sizeR = 195; irisL = 85; irisR = 85;
      browL  = 222.0f; browR = 222.0f;
      mGap   = 145.0f; mCurve = 62.0f; mWidth = 112.0f;
      break;
    case 38: // COOL NOD
      ly = 128.0f + std::sin(t * 3.2f) * 38.0f; ry = ly;
      blinkL = 65; blinkR = 65;
      browL  = 96.0f; browR = 148.0f;
      mCurve = 184.0f; mWidth = 148.0f; mGap = 24.0f;
      break;
    case 39: // GIGGLING FIT
      ly = 136.0f + std::sin(t * 14.0f) * 18.0f; ry = ly;
      blinkL = 110; blinkR = 110;
      mGap   = 68.0f + std::fabs(std::sin(t * 12.0f)) * 95.0f;
      mCurve = 216.0f;
      break;
    case 40: // BORED SIGH
      blinkL = 135; blinkR = 135;
      lx = 128.0f + std::sin(t * 0.5f) * 48.0f; rx = lx;
      ly = 148.0f; ry = 148.0f;
      browL  = 112.0f; browR = 112.0f;
      mGap   = std::max(0.0f, std::sin(t * 1.1f)) * 90.0f;
      mCurve = 98.0f; mWidth = 104.0f;
      break;
    case 41: // HUNGRY LICK
      browL  = 148.0f; browR = 148.0f;
      mGap   = 75.0f;
      mTong  = clampByte(209.0f + (0.5f + 0.5f * std::sin(t * 3.2f)) * 45.0f);
      mCurve = 186.0f;
      break;
    case 42: // STARE DOWN
      lx = 128.0f; rx = 128.0f; ly = 128.0f; ry = 128.0f;
      blinkL = 0; blinkR = 0;
      browL  = 42.0f; browR = 42.0f;
      mCurve = 122.0f; mGap = 8.0f;
      break;
    case 43: // WOBBLE JAW
      mGap   = 95.0f + std::sin(t * 6.5f) * 55.0f;
      mWidth = 132.0f + std::sin(t * 9.5f) * 44.0f;
      mCurve = 142.0f + std::cos(t * 7.5f) * 35.0f;
      break;
    case 44: // SHIVERING COLD
      lx = 128.0f + std::sin(t * 26.0f) * 18.0f; rx = lx;
      mGap   = 30.0f + std::fabs(std::sin(t * 24.0f)) * 45.0f;
      mCurve = 72.0f; browL = 185.0f; browR = 185.0f;
      break;
    case 45: // DAYDREAM FLOAT
      lx = 128.0f + std::sin(t * 0.65f) * 58.0f; rx = lx;
      ly = 156.0f + std::cos(t * 0.55f) * 34.0f; ry = ly;
      browL  = 164.0f; browR = 164.0f;
      mCurve = 174.0f; mGap = 28.0f;
      break;
    case 46: // NERDY BLINK
      blinkL = evalPeriodicBlink(nowMs, 1100, 150);
      blinkR = blinkL;
      lx = 138.0f; rx = 118.0f;
      browL  = 176.0f; browR = 176.0f;
      mCurve = 185.0f; mGap = 42.0f; mTeeth = 96;
      break;
    case 47: // VAMPIRE BITE
      browL  = 28.0f; browR = 28.0f;
      irisL  = 185; irisR = 185;
      mGap   = 92.0f + std::sin(t * 3.8f) * 48.0f;
      mCurve = 182.0f; mTeeth = 48;
      break;
    case 48: // FUNKY GROOVE
      lx = 128.0f + std::sin(t * 3.8f) * 64.0f; rx = lx;
      browL  = 128.0f + std::sin(t * 7.6f) * 48.0f;
      browR  = 128.0f - std::sin(t * 7.6f) * 48.0f;
      mGap   = 38.0f + std::fabs(std::sin(t * 3.8f)) * 68.0f;
      mCurve = 188.0f;
      break;
    case 49: // DRAMATIC GASP
      sizeL  = 182; sizeR = 182;
      browL  = 218.0f; browR = 218.0f;
      mGap   = 168.0f; mWidth = 86.0f; mCurve = 96.0f;
      break;
    case 50: // MISCHIEVOUS GRIN
      browL  = 48.0f; browR = 172.0f;
      lx = 168.0f; rx = 168.0f;
      mCurve = 218.0f; mWidth = 166.0f; mGap = 36.0f;
      break;
    case 51: // WIDE WONDER
      sizeL  = 205; sizeR = 205; blinkL = 0; blinkR = 0;
      browL  = 212.0f; browR = 212.0f;
      mGap   = 118.0f; mWidth = 112.0f; mCurve = 175.0f;
      break;
    case 52: // FROWN TWITCH
      browL  = 46.0f + std::sin(t * 11.0f) * 18.0f;
      browR  = 46.0f;
      mCurve = 58.0f; mGap = 18.0f;
      break;
    case 53: // TONGUE WAG
      mGap   = 155.0f;
      mTong  = clampByte(95.0f + std::sin(t * 7.5f) * 55.0f);
      mCurve = 185.0f;
      break;
    case 54: // SLOW BLINK
      blinkL = evalPeriodicBlink(nowMs, 4200, 680);
      blinkR = blinkL;
      mCurve = 168.0f; mGap = 18.0f;
      break;
    case 55: // EXCITED BOUNCE
      ly = 128.0f + std::fabs(std::sin(t * 6.8f)) * 55.0f; ry = ly;
      browL  = 188.0f; browR = 188.0f;
      mGap   = 75.0f + std::fabs(std::sin(t * 6.8f)) * 95.0f;
      mCurve = 222.0f;
      break;
    case 56: // SNEAKY GLANCE
      lx = 128.0f + std::sin(t * 2.2f) * 96.0f; rx = lx;
      ly = 118.0f; ry = 118.0f;
      browL  = 78.0f; browR = 148.0f;
      mCurve = 174.0f; mGap = 14.0f;
      break;
    case 57: // BEAMING PRIDE
      browL  = 168.0f; browR = 168.0f;
      mCurve = 228.0f; mWidth = 172.0f; mGap = 54.0f; mTeeth = 16;
      break;
    case 58: // STARTLED JUMP
      sizeL  = 210; sizeR = 210; blinkL = 0; blinkR = 0;
      ly = 162.0f; ry = 162.0f;
      browL  = 232.0f; browR = 232.0f;
      mGap   = 154.0f; mWidth = 92.0f;
      break;
    case 59: // RELAXED BREATHE
      lx = 128.0f; rx = 128.0f;
      browL  = 134.0f + std::sin(t * 1.2f) * 10.0f; browR = browL;
      mGap   = 14.0f + (0.5f + 0.5f * std::sin(t * 1.2f)) * 26.0f;
      mCurve = 166.0f;
      break;
    case 60: // DISCO FEVER
      lx = 128.0f + std::sin(t * 5.0f) * 85.0f; rx = lx;
      ly = 128.0f + std::cos(t * 5.0f) * 65.0f; ry = ly;
      browL  = 128.0f + std::sin(t * 10.0f) * 64.0f;
      browR  = 128.0f - std::sin(t * 10.0f) * 64.0f;
      mGap   = 65.0f + std::fabs(std::sin(t * 5.0f)) * 115.0f;
      mCurve = 210.0f;
      break;
    case 61: // KING ROYAL
      ly = 108.0f; ry = 108.0f;
      browL  = 144.0f; browR = 144.0f;
      mCurve = 186.0f; mWidth = 152.0f; mGap = 28.0f;
      break;
    case 62: { // CYBORG GLITCH
      uint32_t step = nowMs / 180U;
      lx = 128.0f + static_cast<float>((static_cast<int>(step * 73) % 160) - 80);
      rx = lx;
      ly = 128.0f + static_cast<float>((static_cast<int>(step * 41) % 100) - 50);
      ry = ly;
      mGap   = ((step % 5) == 0) ? 140.0f : 22.0f;
      mWidth = ((step % 3) == 0) ? 88.0f  : 158.0f;
      break;
    }
    case 63: // JACK-O-LANTERN
      mTeeth = 208;
      mGap   = 95.0f + std::sin(t * 2.2f) * 55.0f;
      mCurve = 200.0f;
      irisL  = 195; irisR = 195;
      break;
    case 64: // SUPERHERO POSE
    default:
      browL  = 68.0f; browR = 68.0f;
      mCurve = 216.0f; mWidth = 174.0f; mGap = 42.0f; mTeeth = 16;
      break;
  }

  if (s_switchMs > 0) {
    uint32_t dt = nowMs - s_switchMs;
    if (dt < 240U) {
      float u = static_cast<float>(dt) / 240.0f;
      float e = u * u * (3.0f - 2.0f * u);
      lx    = s_prevLx    + (lx    - s_prevLx)    * e;
      ly    = s_prevLy    + (ly    - s_prevLy)    * e;
      rx    = s_prevRx    + (rx    - s_prevRx)    * e;
      ry    = s_prevRy    + (ry    - s_prevRy)    * e;
      browL = s_prevBrowL + (browL - s_prevBrowL) * e;
      browR = s_prevBrowR + (browR - s_prevBrowR) * e;
      mGap   = s_prevGap   + (mGap   - s_prevGap)   * e;
      mCurve = s_prevCurve + (mCurve - s_prevCurve) * e;
      mWidth = s_prevWidth + (mWidth - s_prevWidth) * e;
      mTong  = 0;
    } else {
      s_switchMs = 0;
    }
  }

  s_curLx    = lx;    s_curLy    = ly;
  s_curRx    = rx;    s_curRy    = ry;
  s_curBrowL = browL; s_curBrowR = browR;
  s_curGap   = mGap;  s_curCurve = mCurve;
  s_curWidth = mWidth;

  setProgrammaticBrows(clampByte(browL), clampByte(browR));
  renderSingleEyeNormalized(0, clampByte(lx), clampByte(ly), irisL, blinkL, sizeL);
  renderSingleEyeNormalized(1, clampByte(rx), clampByte(ry), irisR, blinkR, sizeR);
  drawMouth(clampByte(mGap), clampByte(mCurve), clampByte(mWidth), mTeeth, mTong, mThick, isMouthHiddenByMemory());
}



// ==============================================================================
// Section D. Autonomous Memory Show Loop & 39-Channel Live DMX Memory Executor
// ==============================================================================

// ------------------------------------------------------------------------------
// 11. Step through enabled INI memory slots automatically when DMX is idle
// ------------------------------------------------------------------------------
void runAutonomousMemoryShow(
  uint32_t nowMs,
  const std::vector<int>& savedSlots,
  const std::map<int, MemoryPreset>& memories)
{
  if (savedSlots.empty()) {
    g_active_memory_slot = 0;
    runAnimationById(0, nowMs);
    return;
  }

  if (g_last_slot_switch_ms == 0) {
    g_last_slot_switch_ms = nowMs;
    g_seq_list_cursor     = 0;
    g_active_memory_slot  = savedSlots.at(0);
  } else if ((nowMs - g_last_slot_switch_ms) >= AUTO_SHOW_SLOT_DURATION_MS) {
    g_last_slot_switch_ms = nowMs;
    g_seq_list_cursor     = (g_seq_list_cursor + 1) % savedSlots.size();
    g_active_memory_slot  = savedSlots.at(g_seq_list_cursor);
  }

  int animId = 0;
  auto it = memories.find(g_active_memory_slot);
  if (it != memories.end() && it->second.enabled) {
    animId = it->second.anim_id;
  }

  runAnimationById(animId, nowMs);
}

// ------------------------------------------------------------------------------
// 12. Execute DMX-triggered memory slot (#000..#240) or hide-part mode (251..255)
// ------------------------------------------------------------------------------
void runDmxMemory(uint8_t rawMemVal, uint32_t nowMs, bool allowLiveDmxOverlay)
{
  g_hide_flags_mask = 0;
  if (rawMemVal == 251) {
    g_hide_flags_mask = 0x01u;
  } else if (rawMemVal == 252) {
    g_hide_flags_mask = 0x02u;
  } else if (rawMemVal == 253) {
    g_hide_flags_mask = 0x03u;
  } else if (rawMemVal == 254) {
    g_hide_flags_mask = 0x07u;
  } else if (rawMemVal == 255) {
    g_hide_flags_mask = 0x0Fu;
  }

  int slot = decodeDmxMemorySlot(rawMemVal);
  if (slot >= 0) {
    g_active_memory_slot = slot;
    if (allowLiveDmxOverlay) {
      updateEyeViaDmx();
      updateFaceViaDmx(isMouthHiddenByMemory());
      return;
    }
    runAnimationById(0, nowMs);
    return;
  }

  updateEyeViaDmx();
  updateFaceViaDmx(isMouthHiddenByMemory());
}
