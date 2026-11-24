// ==============================================================================
// MakingFaces - Choreographed Halloween, Cartoon & Autonomous Macro Engine
// Architecture designed by Magnus Herrlin 2026
// 64-Preset Non-Blocking Frame-Accurate Animation State Machine
// ==============================================================================
#include "macro_engine.h"
#include "main.h"
#include "eye_functions.h"
#include "face_engine.h"
#include "audio_engine.h"
#include <algorithm>
#include <array>
#include <cmath>

extern dmx_port_t dmxPort;
extern int        start_address;

constexpr uint8_t  MACRO_ACTIVE_THRESHOLD  = 10;
constexpr uint8_t  MACRO_HIDE_THRESHOLD    = 251;
constexpr uint8_t  MACRO_DEFAULT_EYE_STYLE = 62;
constexpr uint8_t  MACRO_DEFAULT_EYE_SIZE  = 124;
constexpr int      TOTAL_ANIMATION_COUNT   = 64;

constexpr uint8_t TOOTH_DMX_NONE    = 0;
constexpr uint8_t TOOTH_DMX_NORMAL  = 35;
constexpr uint8_t TOOTH_DMX_MATER   = 62;
constexpr uint8_t TOOTH_DMX_DRACULA = 88;
constexpr uint8_t TOOTH_DMX_PIRATE  = 114;
constexpr uint8_t TOOTH_DMX_SHARK   = 140;
enum MacroPreset {
  MACRO_NONE = 0,
  MACRO_AUTO_IDLE = 1, MACRO_CREEPY_STALKER, MACRO_SHAPESHIFTER, MACRO_LAUGHING_SHARK,
  MACRO_SMAUG_AWAKENING, MACRO_CROSS_EYED_PIRATE, MACRO_GOLF_PANIC, MACRO_GOOGLY_SPIN,
  MACRO_EVIL_LAUGH, MACRO_COCKY_ROGUE, MACRO_SLY_VILLAIN, MACRO_CHATTY_TALKER,
  MACRO_SLY_LICK, MACRO_POPOUT_SHOCK, MACRO_HYPNOTIC_TRANCE, MACRO_FLIRTY_WINK,
  MACRO_SLEEPY_YAWN, MACRO_ANGRY_RAGE, MACRO_ZOMBIE_GROAN, MACRO_VAMPIRE_HISS,
  MACRO_SUSPICIOUS_SQUINT, MACRO_OPERA_SINGER, MACRO_CONFUSED_PUZZLE, MACRO_EXCITED_BOUNCE,
  MACRO_SHY_BLUSH, MACRO_ROBOT_SCAN, MACRO_DIZZY_STARS, MACRO_CHEWING_GUM,
  MACRO_SNEEZE_BUILDUP, MACRO_EVIL_PLAN, MACRO_CRYING_SOB, MACRO_WHISTLING_TUNE,
  MACRO_SURPRISED_GASP, MACRO_WOLF_HOWL, MACRO_GIGGLE_FIT, MACRO_BORED_SIGH,
  MACRO_ALIEN_SIGNAL, MACRO_CLOWN_GRIN, MACRO_SNORING_SLEEP, MACRO_NERD_BLINK,
  MACRO_MONSTER_ROAR, MACRO_SHIVERING_COLD, MACRO_SPICY_BURN, MACRO_DAYDREAM_FLOAT,
  MACRO_PIRATE_ARRR, MACRO_TICKLED_LAUGH, MACRO_STERN_JUDGE, MACRO_SUPERHERO_POSE,
  MACRO_GHOST_BOO, MACRO_HICCUP_JOLT, MACRO_CAT_PURR, MACRO_DOG_PANT,
  MACRO_FROG_CROAK, MACRO_JACK_O_LANTERN, MACRO_SKELETON_RATTLE, MACRO_WITCH_CACKLE,
  MACRO_MUMMY_CURSE, MACRO_CYBORG_GLITCH, MACRO_ALIEN_PROBE, MACRO_NINJA_FOCUS,
  MACRO_KING_ROYAL, MACRO_BABY_GOO, MACRO_MAD_SCIENTIST, MACRO_DISCO_FEVER,
  MACRO_MASK_MODES = 65
};

struct AutoIdleInternalState {
  bool     initialized;
  uint32_t nextGazeTime, nextBlinkTime, blinkStartTime, nextMoodTime;
  bool     isBlinking;
  float    curEyeX, curEyeY, tgtEyeX, tgtEyeY;
  float    curLid, tgtLid, curBrow, tgtBrow;
  float    curMouthGap, tgtMouthGap, curMouthCurve, tgtMouthCurve;
};

static MacroPreset           activeMacro          = MACRO_NONE;
static int                   activeMacroSlot      = 0;
static uint32_t              macroStartMs         = 0;
static bool                  faceHiddenState      = false;
static bool                  mouthHiddenState     = false;
static bool                  noseHiddenState      = false;
static bool                  browsHiddenState     = false;
static std::array<float, 2>  eyePopOutFactor      = {0.0f, 0.0f};
static AutoIdleInternalState idleState            = {};
static int                   autoShowSlot         = 0;
static size_t                autoShowSeqIdx       = 0;
static uint32_t              autoShowNextSwitchMs = 0;
int decodeDmxMacroSlot(uint8_t macroVal)
{
  if (macroVal < MACRO_ACTIVE_THRESHOLD || macroVal >= MACRO_HIDE_THRESHOLD) return 0;
  return static_cast<int>(macroVal) - 10;
}

static MacroPreset mapSlotToAnimation(int slot)
{
  if (slot <= 0) return MACRO_AUTO_IDLE;
  int idx = ((slot - 1) % TOTAL_ANIMATION_COUNT) + 1;
  return static_cast<MacroPreset>(idx);
}

static inline uint8_t clampByte(float val)
{
  if (val <= 0.0f)   return 0;
  if (val >= 255.0f) return 255;
  return static_cast<uint8_t>(val);
}

static inline bool isMouthSuppressedByDmx(void)
{
  uint8_t rawThick = dmx_read_slot(dmxPort, getWrappedDmxIndex(start_address, DMX_OFFSET_MOUTH_THICKNESS) + 1);
  return (rawThick == 255) || mouthHiddenState;
}

static void resetAutoIdleState(uint32_t nowMs)
{
  idleState.initialized    = true;
  idleState.nextGazeTime   = nowMs + 800UL;
  idleState.nextBlinkTime  = nowMs + 2500UL;
  idleState.blinkStartTime = 0;
  idleState.nextMoodTime   = nowMs + 3500UL;
  idleState.isBlinking     = false;
  idleState.curEyeX        = 128.0f; idleState.curEyeY        = 128.0f;
  idleState.tgtEyeX        = 128.0f; idleState.tgtEyeY        = 128.0f;
  idleState.curLid         = 65.0f;  idleState.tgtLid         = 65.0f;
  idleState.curBrow        = 128.0f; idleState.tgtBrow        = 128.0f;
  idleState.curMouthGap    = 10.0f;  idleState.tgtMouthGap    = 10.0f;
  idleState.curMouthCurve  = 175.0f; idleState.tgtMouthCurve  = 175.0f;
}

static void clearHideFlags(void)
{
  faceHiddenState  = false;
  mouthHiddenState = false;
  noseHiddenState  = false;
  browsHiddenState = false;
  eyePopOutFactor.fill(0.0f);
}

void initMacroEngine(void)
{
  clearHideFlags();
  activeMacro           = MACRO_NONE;
  activeMacroSlot       = 0;
  macroStartMs          = 0;
  idleState.initialized = false;
  autoShowSlot          = 0;
  autoShowSeqIdx        = 0;
  autoShowNextSwitchMs  = 0;
}

bool  isDmxMacroActive(uint8_t macroVal) { return (macroVal >= MACRO_ACTIVE_THRESHOLD); }
bool  isFaceHiddenByMacro(void)          { return faceHiddenState; }
bool  isMouthHiddenByMacro(void)         { return mouthHiddenState; }
bool  isNoseHiddenByMacro(void)          { return noseHiddenState; }
bool  isBrowsHiddenByMacro(void)         { return browsHiddenState; }
bool  isEyePoppedOut(int eyeIdx)         { return (eyeIdx >= 0 && eyeIdx < 2) && (eyePopOutFactor.at(eyeIdx) > 0.02f); }
float getEyePopOutFactor(int eyeIdx)     { return (eyeIdx >= 0 && eyeIdx < 2) ? eyePopOutFactor.at(eyeIdx) : 0.0f; }
int   getActiveMacroSlot(void)           { return activeMacroSlot; }
static void animAutoIdle(uint32_t nowMs)
{
  if (!idleState.initialized) resetAutoIdleState(nowMs);
  if (nowMs >= idleState.nextGazeTime) {
    idleState.tgtEyeX = (float)random(48, 208); idleState.tgtEyeY = (float)random(70, 186);
    idleState.nextGazeTime = nowMs + (uint32_t)random(1400, 4200);
  }
  if (nowMs >= idleState.nextMoodTime) {
    idleState.tgtLid = (float)random(25, 95); idleState.tgtBrow = (float)random(85, 185);
    idleState.tgtMouthGap = (float)random(0, 140); idleState.tgtMouthCurve = (float)random(20, 240);
    idleState.nextMoodTime = nowMs + (uint32_t)random(2800, 6500);
  }
  idleState.curEyeX += (idleState.tgtEyeX - idleState.curEyeX) * 0.14f;
  idleState.curEyeY += (idleState.tgtEyeY - idleState.curEyeY) * 0.14f;
  idleState.curLid += (idleState.tgtLid - idleState.curLid) * 0.06f;
  idleState.curBrow += (idleState.tgtBrow - idleState.curBrow) * 0.08f;
  idleState.curMouthGap += (idleState.tgtMouthGap - idleState.curMouthGap) * 0.08f;
  idleState.curMouthCurve += (idleState.tgtMouthCurve - idleState.curMouthCurve) * 0.08f;

  uint8_t lidThreshold = (uint8_t)idleState.curLid;
  if (!idleState.isBlinking && nowMs >= idleState.nextBlinkTime) { idleState.isBlinking = true; idleState.blinkStartTime = nowMs; }
  if (idleState.isBlinking) {
    uint32_t elapsed = nowMs - idleState.blinkStartTime;
    if (elapsed >= 180UL) { idleState.isBlinking = false; idleState.nextBlinkTime = nowMs + (uint32_t)random(2400, 5800); }
    else { lidThreshold = (uint8_t)(idleState.curLid + sinf(((float)elapsed / 180.0f) * PI) * (254.0f - idleState.curLid)); }
  }
  setEyeStyle(MACRO_DEFAULT_EYE_STYLE);
  renderEyePair((uint8_t)idleState.curEyeX, (uint8_t)idleState.curEyeY, 150, lidThreshold, MACRO_DEFAULT_EYE_SIZE);
  uint8_t browVal = (uint8_t)idleState.curBrow;
  uint8_t idleGap = isAudioPlaying() ? 12 : std::max<uint8_t>(12, (uint8_t)idleState.curMouthGap);
  setProgrammaticBrows(browVal, browVal);
  drawMouth(idleGap, (uint8_t)idleState.curMouthCurve, 150, TOOTH_DMX_NORMAL, 0, 128, isMouthSuppressedByDmx());
}
static void animCreepyStalker(float tSec) { setEyeStyle(MACRO_DEFAULT_EYE_STYLE); uint8_t gx = clampByte(128.0f + sinf(tSec * 0.55f) * 85.0f), gy = clampByte(132.0f + cosf(tSec * 0.35f) * 16.0f); renderEyePair(gx, gy, clampByte(55.0f + sinf(tSec * 0.70f) * 35.0f), clampByte(102.0f + sinf(tSec * 0.50f) * 14.0f), MACRO_DEFAULT_EYE_SIZE); setProgrammaticBrows(clampByte(90.0f + sinf(tSec * 0.65f) * 45.0f), clampByte(90.0f + cosf(tSec * 0.55f) * 45.0f)); drawMouth(clampByte(24.0f + sinf(tSec * 0.85f) * 16.0f), 250, clampByte(245.0f + sinf(tSec * 0.45f) * 10.0f), TOOTH_DMX_DRACULA, 0, 128, isMouthSuppressedByDmx()); }
static void animShapeshifter(float tSec) { float cyclePos = fmodf(tSec, 8.0f); bool isBeast = (cyclePos >= 4.0f); float localT = isBeast ? (cyclePos - 4.0f) : cyclePos; float openEnv = fmaxf(0.0f, fminf(1.0f, (localT < 0.65f) ? (localT / 0.65f) : ((localT < 3.35f) ? 1.0f : 1.0f - ((localT - 3.35f) / 0.65f)))); uint8_t gx = clampByte(128.0f + sinf(tSec * 1.2f) * 55.0f * openEnv); if (isBeast) { setEyeStyle(126); renderEyePair(gx, 136, clampByte(35.0f + sinf(tSec * 6.0f) * 20.0f), clampByte(254.0f - openEnv * 195.0f), MACRO_DEFAULT_EYE_SIZE); setProgrammaticBrows(0, 0); drawMouth(clampByte(openEnv * (175.0f + sinf(tSec * 18.0f) * 35.0f)), 0, clampByte(160.0f + openEnv * 90.0f), TOOTH_DMX_SHARK, 95, 128, isMouthSuppressedByDmx()); } else { setEyeStyle(MACRO_DEFAULT_EYE_STYLE); renderEyePair(gx, 126, 155, clampByte(254.0f - openEnv * 210.0f), MACRO_DEFAULT_EYE_SIZE); setProgrammaticBrows(165, 165); drawMouth(clampByte(openEnv * 38.0f), 160, clampByte(55.0f + openEnv * 25.0f), TOOTH_DMX_NONE, 0, 128, isMouthSuppressedByDmx()); } }
static void animLaughingShark(float tSec) { setEyeStyle(126); float c = fmodf(tSec, 7.5f); float active = (c < 3.2f) ? sinf((c / 3.2f) * PI) : 0.0f; float beat = fabsf(sinf(tSec * 7.5f)) * active; renderEyePair(clampByte(128.0f + sinf(tSec * 0.9f) * 35.0f), 130, clampByte(45.0f + beat * 35.0f), clampByte(75.0f - active * 20.0f), MACRO_DEFAULT_EYE_SIZE); setProgrammaticBrows(clampByte(45.0f - active * 45.0f), clampByte(45.0f - active * 45.0f)); drawMouth(clampByte(18.0f + beat * 230.0f), 210, clampByte(165.0f + beat * 80.0f), TOOTH_DMX_SHARK, clampByte(beat * 185.0f), clampByte(128.0f + sinf(tSec * 12.0f) * 75.0f * active), isMouthSuppressedByDmx()); }
static void animSmaugAwakening(float tSec) { setEyeStyle(126); float br = 0.5f - 0.5f * cosf(tSec * 0.55f); float mo = (br > 0.45f) ? ((br - 0.45f) / 0.55f) : 0.0f; renderEyePair(128, 134, clampByte(85.0f - br * 50.0f), clampByte(115.0f - br * 45.0f), 95); setProgrammaticBrows(15, 15); drawMouth(clampByte(mo * 195.0f), 10, clampByte(35.0f + mo * 95.0f), TOOTH_DMX_DRACULA, clampByte(mo * 140.0f), 128, isMouthSuppressedByDmx()); }
static void animCrossEyedPirate(float tSec) { setEyeStyle(MACRO_DEFAULT_EYE_STYLE); float c = fmodf(tSec, 7.0f); float cw = (c > 2.2f && c < 5.6f) ? sinf(((c - 2.2f) / 3.4f) * PI) : 0.0f; float ch = sinf(tSec * 4.2f) * cw; renderSingleEyeNormalized(0, clampByte(128.0f - cw * 105.0f), clampByte(125.0f + cw * 22.0f), 155, 35, MACRO_DEFAULT_EYE_SIZE); renderSingleEyeNormalized(1, clampByte(128.0f + cw * 105.0f), clampByte(125.0f + cw * 22.0f), 155, 35, MACRO_DEFAULT_EYE_SIZE); setProgrammaticBrows(clampByte(165.0f - cw * 75.0f), clampByte(145.0f + cw * 85.0f)); drawMouth(clampByte(25.0f + fabsf(ch) * 95.0f), clampByte(140.0f + ch * 85.0f), clampByte(125.0f + ch * 55.0f), TOOTH_DMX_MATER, 0, 128, isMouthSuppressedByDmx()); }
static void animGolfPanic(float tSec) { setEyeStyle(32); setRoundEyelids(true); float cyc = fmodf(tSec, 8.5f); float env = (cyc > 3.5f && cyc < 6.8f) ? sinf(((cyc - 3.5f) / 3.3f) * PI) : 0.0f; renderEyePair(clampByte(128.0f + sinf(tSec * 38.0f) * 24.0f * env), clampByte(128.0f + cosf(tSec * 33.0f) * 20.0f * env), clampByte(130.0f - env * 115.0f), clampByte(45.0f - env * 45.0f), clampByte(MACRO_DEFAULT_EYE_SIZE + env * 18.0f)); setProgrammaticBrows(clampByte(128.0f + env * 127.0f), clampByte(128.0f + env * 127.0f)); drawMouth(clampByte(15.0f + env * (230.0f + sinf(tSec * 28.0f) * 15.0f)), clampByte(145.0f - env * 130.0f), clampByte(135.0f - env * 75.0f), TOOTH_DMX_NONE, 0, 128, isMouthSuppressedByDmx()); }
static void animGooglySpin(float tSec) { float cyc = fmodf(tSec, 8.5f); if (cyc < 4.5f || cyc > 7.2f) { setEyeStyle(MACRO_DEFAULT_EYE_STYLE); float ls = sinf(tSec * 1.4f); renderEyePair(clampByte(128.0f + ls * 45.0f), 128, 135, 55, MACRO_DEFAULT_EYE_SIZE); setProgrammaticBrows(135, 135); drawMouth(18, 175, 145, TOOTH_DMX_NORMAL, 0, 128, isMouthSuppressedByDmx()); return; } setEyeStyle(32); setRoundEyelids(true); float st = cyc - 4.5f; renderSingleEyeNormalized(0, clampByte(128.0f + cosf(st * 9.5f) * 115.0f), clampByte(128.0f + sinf(st * 9.5f) * 115.0f), 140, 0, clampByte(MACRO_DEFAULT_EYE_SIZE + sinf(st * 6.0f) * 28.0f)); renderSingleEyeNormalized(1, clampByte(128.0f + cosf(-st * 9.5f) * 115.0f), clampByte(128.0f + sinf(-st * 9.5f) * 115.0f), 140, 0, clampByte(MACRO_DEFAULT_EYE_SIZE - sinf(st * 6.0f) * 28.0f)); setProgrammaticBrows(clampByte(128.0f + sinf(st * 7.0f) * 110.0f), clampByte(128.0f - sinf(st * 7.0f) * 110.0f)); drawMouth(clampByte(130.0f + sinf(st * 16.0f) * 75.0f), clampByte(128.0f + cosf(st * 12.0f) * 105.0f), clampByte(145.0f + sinf(st * 9.0f) * 60.0f), TOOTH_DMX_MATER, 235, clampByte(128.0f + sinf(st * 14.0f) * 110.0f), isMouthSuppressedByDmx()); }
static void animEvilLaugh(float tSec) { setEyeStyle(126); float c = fmodf(tSec, 7.5f); float a = (c < 3.4f) ? sinf((c / 3.4f) * PI) : 0.0f; float l = fabsf(sinf(tSec * 7.0f)) * a; renderEyePair(clampByte(128.0f + sinf(tSec * 1.4f) * 40.0f), 134, clampByte(75.0f + l * 75.0f), clampByte(75.0f - a * 20.0f), MACRO_DEFAULT_EYE_SIZE); setProgrammaticBrows(20, 20); drawMouth(clampByte(16.0f + l * 215.0f), 240, clampByte(185.0f + l * 65.0f), TOOTH_DMX_DRACULA, 0, 95, isMouthSuppressedByDmx()); }
static void animCockyRogue(float tSec) { setEyeStyle(MACRO_DEFAULT_EYE_STYLE); float sw = sinf(tSec * 1.1f), ch = fabsf(sinf(tSec * 4.5f)); renderSingleEyeNormalized(0, clampByte(120.0f + sw * 32.0f), clampByte(115.0f + cosf(tSec * 0.9f) * 14.0f), clampByte(65.0f + sw * 16.0f), clampByte(77.0f + sw * 18.0f), MACRO_DEFAULT_EYE_SIZE); renderSingleEyeNormalized(1, clampByte(115.0f + sw * 32.0f), clampByte(126.0f + cosf(tSec * 0.9f) * 14.0f), clampByte(55.0f - sw * 12.0f), clampByte(62.0f - sw * 18.0f), MACRO_DEFAULT_EYE_SIZE); setProgrammaticBrows(clampByte(109.0f - sw * 35.0f), clampByte(150.0f + sw * 35.0f)); drawMouth(clampByte(20.0f + ch * 45.0f), 235, clampByte(110.0f + ch * 25.0f), TOOTH_DMX_PIRATE, 0, 195, isMouthSuppressedByDmx()); }
static void animSlyVillain(float tSec) { setEyeStyle(MACRO_DEFAULT_EYE_STYLE); uint8_t gx = clampByte(128.0f + sinf(tSec * 0.75f) * 90.0f); renderEyePair(gx, 132, 120, clampByte(105.0f + cosf(tSec * 0.6f) * 15.0f), MACRO_DEFAULT_EYE_SIZE); setProgrammaticBrows(clampByte(110.0f + sinf(tSec * 0.75f) * 55.0f), clampByte(110.0f - sinf(tSec * 0.75f) * 55.0f)); drawMouth(clampByte(8.0f + (0.5f + 0.5f * sinf(tSec * 0.9f)) * 22.0f), 220, 235, TOOTH_DMX_NONE, 0, 100, isMouthSuppressedByDmx()); }
static void animChattyTalker(float tSec) { setEyeStyle(MACRO_DEFAULT_EYE_STYLE); float burst = (fmodf(tSec, 6.5f) < 4.2f) ? 1.0f : 0.0f; float env = fmaxf(0.0f, sinf(tSec * 10.5f) * 0.5f + sinf(tSec * 16.3f) * 0.35f + 0.35f) * burst; uint8_t gap = clampByte(env * 175.0f); uint8_t lid = (fmodf(tSec, 3.8f) > 3.55f) ? 250 : 65; renderEyePair(clampByte(128.0f + sinf(tSec * 0.8f) * 24.0f), 124, 155, lid, MACRO_DEFAULT_EYE_SIZE); uint8_t br = clampByte(140.0f + sinf(tSec * 2.6f) * 28.0f * burst); setProgrammaticBrows(br, br); drawMouth(gap, clampByte(155.0f + sinf(tSec * 4.5f) * 40.0f * burst), clampByte(135.0f + sinf(tSec * 4.5f) * 55.0f * burst), TOOTH_DMX_NORMAL, (gap > 95) ? clampByte((gap - 95) * 1.1f) : 0, 128, isMouthSuppressedByDmx()); }
static void animSlyLick(float tSec) { setEyeStyle(MACRO_DEFAULT_EYE_STYLE); float c = fmodf(tSec, 8.0f); float a = (c > 3.5f && c < 6.5f) ? sinf(((c - 3.5f) / 3.0f) * PI) : 0.0f; float ls = sinf(tSec * 2.4f) * a; renderEyePair(clampByte(128.0f + ls * 72.0f), clampByte(132.0f + a * 32.0f), clampByte(135.0f - a * 45.0f), clampByte(68.0f + a * 36.0f), MACRO_DEFAULT_EYE_SIZE); setProgrammaticBrows(clampByte(135.0f - ls * 35.0f), clampByte(135.0f + ls * 35.0f)); drawMouth(clampByte(14.0f + a * 95.0f), 195, 145, TOOTH_DMX_NORMAL, (a > 0.15f) ? clampByte(165.0f + (0.5f + 0.5f * ls) * 88.0f) : 0, 138, isMouthSuppressedByDmx()); }
static void animPopOutShock(float tSec) { setEyeStyle(32); setRoundEyelids(true); float c = fmodf(tSec, 9.5f); float pop = fmaxf(0.0f, fminf(1.0f, (c < 5.8f) ? 0.0f : ((c < 6.15f) ? ((c - 5.8f) / 0.35f) : ((c < 8.65f) ? 1.0f : 1.0f - (c - 8.65f) / 0.45f)))); float f = fmaxf(0.0f, fminf(1.35f, pop + ((c >= 6.15f && c < 6.85f) ? sinf((c - 6.15f) * 28.0f) * expf(-(c - 6.15f) * 4.5f) * 0.25f : 0.0f))); eyePopOutFactor.fill(f); uint8_t idleBlink = (c > 2.4f && c < 2.6f) ? 245 : 42; renderEyePair(clampByte(128.0f + ((pop > 0.05f) ? sinf(tSec * 38.0f) * 28.0f * pop : sinf(tSec * 1.1f) * 35.0f)), clampByte(128.0f + cosf(tSec * 33.0f) * 24.0f * pop), clampByte(145.0f - pop * 115.0f), (pop > 0.05f) ? 0 : idleBlink, clampByte(MACRO_DEFAULT_EYE_SIZE + f * 95.0f)); setProgrammaticBrows(clampByte(135.0f + pop * 120.0f), clampByte(135.0f + pop * 120.0f)); drawMouth(clampByte(14.0f + pop * 236.0f), clampByte(165.0f - pop * 165.0f), clampByte(150.0f - pop * 35.0f), TOOTH_DMX_NORMAL, clampByte(pop * 110.0f), 128, isMouthSuppressedByDmx()); }
static void animHypnoticTrance(float tSec) { setEyeStyle(MACRO_DEFAULT_EYE_STYLE); float p = sinf(tSec * 0.85f), u = 0.5f + 0.5f * sinf(tSec * 1.7f); renderEyePair(clampByte(128.0f + p * 88.0f), clampByte(120.0f + cosf(tSec * 0.85f) * 18.0f), clampByte(40.0f + u * 195.0f), clampByte(118.0f + u * 24.0f), MACRO_DEFAULT_EYE_SIZE); setProgrammaticBrows(clampByte(155.0f + u * 30.0f), clampByte(155.0f + u * 30.0f)); drawMouth(clampByte(38.0f + u * 32.0f), 135, 95, TOOTH_DMX_NORMAL, 0, 145, isMouthSuppressedByDmx()); }
static void animFlirtyWink(float tSec) { setEyeStyle(MACRO_DEFAULT_EYE_STYLE); float c = fmodf(tSec, 7.2f); float w = (c > 4.2f && c < 4.95f) ? sinf(((c - 4.2f) / 0.75f) * PI) : 0.0f; uint8_t gx = clampByte(128.0f + sinf(tSec * 0.9f) * 42.0f); renderSingleEyeNormalized(0, gx, 124, 150, clampByte(45.0f + w * 209.0f), MACRO_DEFAULT_EYE_SIZE); renderSingleEyeNormalized(1, gx, 124, 150, 42, MACRO_DEFAULT_EYE_SIZE); setProgrammaticBrows(clampByte(140.0f - w * 55.0f), clampByte(145.0f + w * 65.0f)); drawMouth(clampByte(18.0f + w * 45.0f), clampByte(205.0f + w * 45.0f), clampByte(150.0f + w * 35.0f), TOOTH_DMX_NORMAL, 0, 155, isMouthSuppressedByDmx()); }
static void animSleepyYawn(float tSec) { setEyeStyle(MACRO_DEFAULT_EYE_STYLE); float c = fmodf(tSec, 9.0f); float y = (c > 4.5f && c < 7.3f) ? sinf(((c - 4.5f) / 2.8f) * PI) : 0.0f; renderEyePair(128, clampByte(138.0f + y * 35.0f), 110, clampByte(135.0f + y * 105.0f), MACRO_DEFAULT_EYE_SIZE); setProgrammaticBrows(clampByte(155.0f + y * 75.0f), clampByte(155.0f + y * 75.0f)); drawMouth(clampByte(10.0f + y * 235.0f), clampByte(128.0f - y * 65.0f), clampByte(125.0f - y * 45.0f), TOOTH_DMX_NORMAL, clampByte(y * 85.0f), 130, isMouthSuppressedByDmx()); }
static void animAngryRage(float tSec) { setEyeStyle(MACRO_DEFAULT_EYE_STYLE); float c = fmodf(tSec, 7.0f); float a = (c > 2.5f && c < 5.5f) ? sinf(((c - 2.5f) / 3.0f) * PI) : 0.0f; float sx = sinf(tSec * 29.0f) * 16.0f * a; renderEyePair(clampByte(128.0f + sx), 132, clampByte(75.0f - a * 45.0f), clampByte(85.0f - a * 20.0f), MACRO_DEFAULT_EYE_SIZE); setProgrammaticBrows(clampByte(40.0f - a * 40.0f), clampByte(40.0f - a * 40.0f)); drawMouth(clampByte(22.0f + a * 125.0f), 0, clampByte(175.0f + a * 65.0f), TOOTH_DMX_DRACULA, 0, 110, isMouthSuppressedByDmx()); }
static void animZombieGroan(float tSec) { setEyeStyle(MACRO_DEFAULT_EYE_STYLE); float sw = sinf(tSec * 0.65f), gr = 0.5f - 0.5f * cosf(tSec * 1.1f); renderSingleEyeNormalized(0, clampByte(115.0f + sw * 55.0f), 100, 45, clampByte(145.0f + sw * 25.0f), MACRO_DEFAULT_EYE_SIZE); renderSingleEyeNormalized(1, clampByte(145.0f - sw * 45.0f), 150, 190, clampByte(82.0f - sw * 20.0f), MACRO_DEFAULT_EYE_SIZE); setProgrammaticBrows(clampByte(195.0f + sw * 35.0f), clampByte(75.0f - sw * 35.0f)); drawMouth(clampByte(65.0f + gr * 135.0f), 25, 130, 245, clampByte(gr * 90.0f), 115, isMouthSuppressedByDmx()); }
static void animVampireHiss(float tSec) { setEyeStyle(126); float c = fmodf(tSec, 7.5f); float h = (c > 3.8f && c < 5.8f) ? sinf(((c - 3.8f) / 2.0f) * PI) : 0.0f; renderEyePair(128, 130, clampByte(85.0f - h * 65.0f), clampByte(88.0f - h * 42.0f), MACRO_DEFAULT_EYE_SIZE); setProgrammaticBrows(clampByte(85.0f - h * 85.0f), clampByte(85.0f - h * 85.0f)); drawMouth(clampByte(18.0f + h * 155.0f), clampByte(175.0f - h * 155.0f), clampByte(145.0f + h * 95.0f), TOOTH_DMX_DRACULA, clampByte(h * 115.0f), 110, isMouthSuppressedByDmx()); }
static void animSuspiciousSquint(float tSec) { setEyeStyle(MACRO_DEFAULT_EYE_STYLE); float s = sinf(tSec * 0.85f); float sh = (s > 0.35f) ? 1.0f : ((s < -0.35f) ? -1.0f : (s / 0.35f)); renderEyePair(clampByte(128.0f + sh * 85.0f), 128, 115, 118, MACRO_DEFAULT_EYE_SIZE); setProgrammaticBrows(clampByte(75.0f - sh * 35.0f), clampByte(175.0f + sh * 45.0f)); drawMouth(10, clampByte(115.0f + sh * 30.0f), 105, TOOTH_DMX_NONE, 0, 125, isMouthSuppressedByDmx()); }
static void animOperaSinger(float tSec) { setEyeStyle(MACRO_DEFAULT_EYE_STYLE); float ph = 0.5f - 0.5f * cosf(tSec * 1.1f); renderEyePair(128, clampByte(116.0f - ph * 24.0f), 155, clampByte(48.0f - ph * 25.0f), MACRO_DEFAULT_EYE_SIZE); setProgrammaticBrows(clampByte(150.0f + ph * 85.0f), clampByte(150.0f + ph * 85.0f)); drawMouth(clampByte(25.0f + ph * 205.0f + sinf(tSec * 22.0f) * 14.0f * ph), 155, clampByte(105.0f - ph * 30.0f), TOOTH_DMX_NORMAL, 0, 150, isMouthSuppressedByDmx()); }
static void animConfusedPuzzle(float tSec) { setEyeStyle(MACRO_DEFAULT_EYE_STYLE); float w = sinf(tSec * 1.2f); renderSingleEyeNormalized(0, clampByte(128.0f + w * 40.0f), 110, 175, 35, MACRO_DEFAULT_EYE_SIZE); renderSingleEyeNormalized(1, clampByte(128.0f + w * 40.0f), 145, 95, 105, MACRO_DEFAULT_EYE_SIZE); setProgrammaticBrows(clampByte(210.0f + w * 30.0f), clampByte(55.0f - w * 30.0f)); drawMouth(28, clampByte(80.0f + w * 50.0f), 90, TOOTH_DMX_MATER, 0, 130, isMouthSuppressedByDmx()); }
static void animExcitedBounce(float tSec) { setEyeStyle(32); setRoundEyelids(true); float c = fmodf(tSec, 6.5f); float a = (c < 3.0f) ? sinf((c / 3.0f) * PI) : 0.0f; float b = fabsf(sinf(tSec * 6.5f)) * a; renderEyePair(128, clampByte(128.0f - b * 36.0f), clampByte(165.0f + b * 60.0f), 25, clampByte(MACRO_DEFAULT_EYE_SIZE + b * 18.0f)); setProgrammaticBrows(clampByte(160.0f + b * 85.0f), clampByte(160.0f + b * 85.0f)); drawMouth(clampByte(45.0f + b * 175.0f), 245, clampByte(165.0f + b * 55.0f), TOOTH_DMX_NORMAL, 0, 140, isMouthSuppressedByDmx()); }
static void animShyBlush(float tSec) { setEyeStyle(MACRO_DEFAULT_EYE_STYLE); float s = 0.5f + 0.5f * sinf(tSec * 0.85f); renderEyePair(clampByte(165.0f + s * 45.0f), clampByte(165.0f + s * 35.0f), 185, clampByte(95.0f + s * 35.0f), MACRO_DEFAULT_EYE_SIZE); setProgrammaticBrows(195, 195); drawMouth(14, 195, 85, TOOTH_DMX_NONE, 0, 145, isMouthSuppressedByDmx()); }
static void animRobotScan(float tSec) { setEyeStyle(53); float st = floorf(tSec * 1.4f); uint8_t gx = clampByte(128.0f + sinf(st * 1.7f) * 95.0f); renderEyePair(gx, 128, 128, 55, MACRO_DEFAULT_EYE_SIZE); setProgrammaticBrows(128, 128); drawMouth((static_cast<int>(tSec * 6.0f) & 1) ? 85 : 15, 128, 140, TOOTH_DMX_NORMAL, 0, 100, isMouthSuppressedByDmx()); }
static void animDizzyStars(float tSec) { setEyeStyle(32); setRoundEyelids(true); renderSingleEyeNormalized(0, clampByte(128.0f + cosf(tSec * 3.8f) * 95.0f), clampByte(128.0f + sinf(tSec * 3.8f) * 95.0f), clampByte(90.0f + sinf(tSec * 6.0f) * 60.0f), 30, MACRO_DEFAULT_EYE_SIZE); renderSingleEyeNormalized(1, clampByte(128.0f + cosf(tSec * 3.8f + PI) * 95.0f), clampByte(128.0f + sinf(tSec * 3.8f + PI) * 95.0f), clampByte(90.0f - sinf(tSec * 6.0f) * 60.0f), 30, MACRO_DEFAULT_EYE_SIZE); setProgrammaticBrows(clampByte(128.0f + sinf(tSec * 4.0f) * 90.0f), clampByte(128.0f - sinf(tSec * 4.0f) * 90.0f)); drawMouth(clampByte(85.0f + sinf(tSec * 3.0f) * 45.0f), clampByte(128.0f + cosf(tSec * 3.5f) * 85.0f), 115, TOOTH_DMX_MATER, 115, 135, isMouthSuppressedByDmx()); }
static void animChewingGum(float tSec) { setEyeStyle(MACRO_DEFAULT_EYE_STYLE); float ch = sinf(tSec * 4.2f); renderEyePair(clampByte(128.0f + sinf(tSec * 0.65f) * 45.0f), 124, 140, 75, MACRO_DEFAULT_EYE_SIZE); setProgrammaticBrows(145, 120); drawMouth(clampByte(25.0f + fabsf(ch) * 65.0f), clampByte(150.0f + ch * 50.0f), clampByte(115.0f - fabsf(ch) * 30.0f), TOOTH_DMX_NORMAL, 0, 165, isMouthSuppressedByDmx()); }
static void animSneezeBuildup(float tSec) { setEyeStyle(MACRO_DEFAULT_EYE_STYLE); float c = fmodf(tSec, 8.5f); if (c < 4.0f) { renderEyePair(128, 128, 145, 55, MACRO_DEFAULT_EYE_SIZE); setProgrammaticBrows(135, 135); drawMouth(16, 165, 135, TOOTH_DMX_NORMAL, 0, 128, isMouthSuppressedByDmx()); } else if (c < 7.2f) { float b = (c - 4.0f) / 3.2f; renderEyePair(128, clampByte(128.0f - b * 45.0f), 110, clampByte(60.0f + b * 110.0f), MACRO_DEFAULT_EYE_SIZE); setProgrammaticBrows(clampByte(128.0f + b * 110.0f), clampByte(128.0f + b * 110.0f)); drawMouth(clampByte(25.0f + b * 135.0f), 75, clampByte(120.0f - b * 45.0f), TOOTH_DMX_NORMAL, 0, 128, isMouthSuppressedByDmx()); } else { float r = (c - 7.2f) / 1.3f; renderEyePair(128, 165, 80, 250, MACRO_DEFAULT_EYE_SIZE); setProgrammaticBrows(20, 20); drawMouth(clampByte(230.0f * (1.0f - r)), 20, 210, TOOTH_DMX_NORMAL, clampByte(120.0f * (1.0f - r)), 128, isMouthSuppressedByDmx()); } }
static void animEvilPlan(float tSec) { setEyeStyle(126); float f = sinf(tSec * 2.2f); renderEyePair(clampByte(128.0f + f * 65.0f), 115, 75, 95, MACRO_DEFAULT_EYE_SIZE); setProgrammaticBrows(25, 25); drawMouth(clampByte(22.0f + fabsf(f) * 30.0f), 245, 225, TOOTH_DMX_DRACULA, 0, 115, isMouthSuppressedByDmx()); }
static void animCryingSob(float tSec) { setEyeStyle(MACRO_DEFAULT_EYE_STYLE); float sob = fabsf(sinf(tSec * 4.5f)); renderEyePair(128, 155, 175, clampByte(130.0f + sob * 95.0f), MACRO_DEFAULT_EYE_SIZE); setProgrammaticBrows(245, 245); drawMouth(clampByte(55.0f + sob * 115.0f), 0, clampByte(135.0f + sob * 45.0f), TOOTH_DMX_NORMAL, 0, 140, isMouthSuppressedByDmx()); }
static void animWhistlingTune(float tSec) { setEyeStyle(MACRO_DEFAULT_EYE_STYLE); float mel = sinf(tSec * 2.4f); renderEyePair(clampByte(128.0f + sinf(tSec * 0.8f) * 55.0f), 108, 145, 58, MACRO_DEFAULT_EYE_SIZE); setProgrammaticBrows(clampByte(160.0f + mel * 35.0f), clampByte(160.0f - mel * 35.0f)); drawMouth(clampByte(48.0f + mel * 22.0f), 145, clampByte(28.0f + mel * 14.0f), TOOTH_DMX_NONE, 0, 195, isMouthSuppressedByDmx()); }
static void animSurprisedGasp(float tSec) { setEyeStyle(32); setRoundEyelids(true); float c = fmodf(tSec, 7.5f); float p = (c > 3.5f && c < 6.2f) ? sinf(((c - 3.5f) / 2.7f) * PI) : 0.0f; renderEyePair(128, 124, clampByte(150.0f + p * 95.0f), clampByte(45.0f - p * 45.0f), clampByte(MACRO_DEFAULT_EYE_SIZE + p * 16.0f)); setProgrammaticBrows(clampByte(145.0f + p * 105.0f), clampByte(145.0f + p * 105.0f)); drawMouth(clampByte(18.0f + p * 205.0f), 128, clampByte(135.0f - p * 60.0f), TOOTH_DMX_NORMAL, 0, 145, isMouthSuppressedByDmx()); }
static void animWolfHowl(float tSec) { setEyeStyle(126); float c = fmodf(tSec, 8.5f); float h = (c > 3.2f && c < 7.0f) ? sinf(((c - 3.2f) / 3.8f) * PI) : 0.0f; renderEyePair(128, clampByte(128.0f - h * 55.0f), clampByte(55.0f + h * 90.0f), clampByte(65.0f + h * 145.0f), MACRO_DEFAULT_EYE_SIZE); setProgrammaticBrows(clampByte(40.0f + h * 160.0f), clampByte(40.0f + h * 160.0f)); drawMouth(clampByte(25.0f + h * 215.0f), 90, clampByte(140.0f - h * 75.0f), TOOTH_DMX_DRACULA, clampByte(h * 110.0f), 135, isMouthSuppressedByDmx()); }
static void animGiggleFit(float tSec) { setEyeStyle(MACRO_DEFAULT_EYE_STYLE); float c = fmodf(tSec, 6.8f); float a = (c < 3.0f) ? sinf((c / 3.0f) * PI) : 0.0f; float g = fabsf(sinf(tSec * 11.0f)) * a; renderEyePair(128, clampByte(124.0f - g * 14.0f), 160, clampByte(65.0f + g * 105.0f), MACRO_DEFAULT_EYE_SIZE); setProgrammaticBrows(clampByte(155.0f + g * 65.0f), clampByte(155.0f + g * 65.0f)); drawMouth(clampByte(20.0f + g * 130.0f), 245, clampByte(155.0f + g * 50.0f), TOOTH_DMX_NORMAL, 0, 140, isMouthSuppressedByDmx()); }
static void animBoredSigh(float tSec) { setEyeStyle(MACRO_DEFAULT_EYE_STYLE); float c = fmodf(tSec, 7.5f); float s = (c > 3.5f && c < 6.5f) ? sinf(((c - 3.5f) / 3.0f) * PI) : 0.0f; renderEyePair(clampByte(128.0f + sinf(tSec * 0.5f) * 60.0f), 136, 125, clampByte(105.0f + s * 45.0f), MACRO_DEFAULT_EYE_SIZE); setProgrammaticBrows(115, 115); drawMouth(clampByte(8.0f + s * 85.0f), clampByte(125.0f - s * 55.0f), 115, TOOTH_DMX_NONE, 0, 128, isMouthSuppressedByDmx()); }
static void animAlienSignal(float tSec) { setEyeStyle(53); float c = fmodf(tSec, 7.0f); float a = (c > 3.0f && c < 5.5f) ? sinf(((c - 3.0f) / 2.5f) * PI) : 0.0f; float w = sinf(tSec * 6.5f) * a; renderSingleEyeNormalized(0, 128, 128, clampByte(128.0f + fabsf(w) * 115.0f), 25, MACRO_DEFAULT_EYE_SIZE); renderSingleEyeNormalized(1, 128, 128, clampByte(128.0f - fabsf(w) * 105.0f), 25, MACRO_DEFAULT_EYE_SIZE); setProgrammaticBrows(clampByte(128.0f + w * 95.0f), clampByte(128.0f - w * 95.0f)); drawMouth(clampByte(20.0f + fabsf(w) * 115.0f), 128, clampByte(85.0f + fabsf(w) * 65.0f), TOOTH_DMX_NONE, 0, 115, isMouthSuppressedByDmx()); }
static void animClownGrin(float tSec) { setEyeStyle(32); setRoundEyelids(true); float b = sinf(tSec * 2.4f); renderEyePair(clampByte(128.0f + b * 65.0f), 124, 175, 25, MACRO_DEFAULT_EYE_SIZE); setProgrammaticBrows(235, 235); drawMouth(clampByte(65.0f + fabsf(b) * 85.0f), 255, 250, TOOTH_DMX_NORMAL, 0, 185, isMouthSuppressedByDmx()); }
static void animSnoringSleep(float tSec) { setEyeStyle(MACRO_DEFAULT_EYE_STYLE); float sn = 0.5f - 0.5f * cosf(tSec * 1.2f); renderEyePair(128, 145, 110, 248, MACRO_DEFAULT_EYE_SIZE); setProgrammaticBrows(clampByte(135.0f + sn * 35.0f), clampByte(135.0f + sn * 35.0f)); drawMouth(clampByte(20.0f + sn * 165.0f), 95, clampByte(95.0f - sn * 35.0f), TOOTH_DMX_MATER, 0, 140, isMouthSuppressedByDmx()); }
static void animNerdBlink(float tSec) { setEyeStyle(32); setRoundEyelids(true); float c = fmodf(tSec, 4.5f); uint8_t lid = ((c > 2.8f && c < 2.95f) || (c > 3.05f && c < 3.20f)) ? 250 : 25; renderEyePair(128, 128, 170, lid, MACRO_DEFAULT_EYE_SIZE); setProgrammaticBrows(185, 185); drawMouth(32, 205, 130, 100, 0, 120, isMouthSuppressedByDmx()); }
static void animMonsterRoar(float tSec) { setEyeStyle(126); float c = fmodf(tSec, 8.0f); float r = (c > 3.5f && c < 6.8f) ? sinf(((c - 3.5f) / 3.3f) * PI) : 0.0f; renderEyePair(clampByte(128.0f + sinf(tSec * 24.0f) * 14.0f * r), 132, clampByte(80.0f - r * 55.0f), clampByte(70.0f - r * 40.0f), MACRO_DEFAULT_EYE_SIZE); setProgrammaticBrows(0, 0); drawMouth(clampByte(24.0f + r * 225.0f), 0, clampByte(160.0f + r * 90.0f), TOOTH_DMX_SHARK, clampByte(r * 135.0f), 120, isMouthSuppressedByDmx()); }
static void animShiveringCold(float tSec) { setEyeStyle(MACRO_DEFAULT_EYE_STYLE); float sh = sinf(tSec * 32.0f); renderEyePair(clampByte(128.0f + sh * 16.0f), 134, 65, 95, MACRO_DEFAULT_EYE_SIZE); setProgrammaticBrows(225, 225); drawMouth(clampByte(25.0f + fabsf(sh) * 40.0f), 15, clampByte(115.0f + sh * 16.0f), TOOTH_DMX_NORMAL, 0, 110, isMouthSuppressedByDmx()); }
static void animSpicyBurn(float tSec) { setEyeStyle(32); setRoundEyelids(true); float p = fabsf(sinf(tSec * 7.5f)); renderEyePair(clampByte(128.0f + sinf(tSec * 10.0f) * 55.0f), 110, clampByte(45.0f + p * 55.0f), 15, MACRO_DEFAULT_EYE_SIZE); setProgrammaticBrows(240, 240); drawMouth(clampByte(140.0f + p * 95.0f), 40, 165, TOOTH_DMX_NORMAL, clampByte(125.0f + p * 35.0f), 145, isMouthSuppressedByDmx()); }
static void animDaydreamFloat(float tSec) { setEyeStyle(MACRO_DEFAULT_EYE_STYLE); renderEyePair(clampByte(128.0f + sinf(tSec * 0.55f) * 48.0f), clampByte(95.0f + cosf(tSec * 0.45f) * 22.0f), 165, 88, MACRO_DEFAULT_EYE_SIZE); setProgrammaticBrows(175, 175); drawMouth(18, 215, 135, TOOTH_DMX_NONE, 0, 145, isMouthSuppressedByDmx()); }
static void animPirateArrr(float tSec) { setEyeStyle(MACRO_DEFAULT_EYE_STYLE); float gr = 0.5f + 0.5f * sinf(tSec * 1.8f); renderSingleEyeNormalized(0, 128, 128, 135, 235, MACRO_DEFAULT_EYE_SIZE); renderSingleEyeNormalized(1, clampByte(128.0f + sinf(tSec * 1.1f) * 50.0f), 128, 115, 55, MACRO_DEFAULT_EYE_SIZE); setProgrammaticBrows(30, 155); drawMouth(clampByte(45.0f + gr * 95.0f), clampByte(185.0f + gr * 45.0f), 175, TOOTH_DMX_PIRATE, 0, 135, isMouthSuppressedByDmx()); }
static void animTickledLaugh(float tSec) { setEyeStyle(MACRO_DEFAULT_EYE_STYLE); float c = fmodf(tSec, 6.5f); float a = (c < 3.0f) ? sinf((c / 3.0f) * PI) : 0.0f; float tk = fabsf(sinf(tSec * 12.0f)) * a; renderEyePair(clampByte(128.0f + sinf(tSec * 9.0f) * 45.0f * a), clampByte(120.0f + cosf(tSec * 7.0f) * 25.0f * a), 175, clampByte(65.0f + tk * 115.0f), MACRO_DEFAULT_EYE_SIZE); setProgrammaticBrows(clampByte(160.0f + tk * 75.0f), clampByte(160.0f + tk * 75.0f)); drawMouth(clampByte(25.0f + tk * 185.0f), 255, clampByte(165.0f + tk * 65.0f), TOOTH_DMX_NORMAL, clampByte(tk * 70.0f), 140, isMouthSuppressedByDmx()); }
static void animSternJudge(float tSec) { setEyeStyle(MACRO_DEFAULT_EYE_STYLE); float sc = sinf(tSec * 0.65f); renderEyePair(clampByte(128.0f + sc * 70.0f), 136, 95, 92, MACRO_DEFAULT_EYE_SIZE); setProgrammaticBrows(35, 35); drawMouth(0, 25, 130, TOOTH_DMX_NONE, 0, 105, isMouthSuppressedByDmx()); }
static void animSuperheroPose(float tSec) { setEyeStyle(MACRO_DEFAULT_EYE_STYLE); float h = 0.5f + 0.5f * sinf(tSec * 1.0f); renderEyePair(128, 116, 140, 48, MACRO_DEFAULT_EYE_SIZE); setProgrammaticBrows(95, 95); drawMouth(clampByte(22.0f + h * 28.0f), 230, 175, TOOTH_DMX_NORMAL, 0, 140, isMouthSuppressedByDmx()); }
static void animGhostBoo(float tSec) { setEyeStyle(32); setRoundEyelids(true); float c = fmodf(tSec, 8.0f); float boo = (c > 4.2f && c < 6.4f) ? sinf(((c - 4.2f) / 2.2f) * PI) : 0.0f; renderEyePair(128, 128, clampByte(145.0f - boo * 90.0f), clampByte(45.0f - boo * 45.0f), clampByte(MACRO_DEFAULT_EYE_SIZE + boo * 22.0f)); setProgrammaticBrows(clampByte(140.0f + boo * 110.0f), clampByte(140.0f + boo * 110.0f)); drawMouth(clampByte(18.0f + boo * 225.0f), 128, clampByte(130.0f - boo * 65.0f), TOOTH_DMX_NONE, 0, 145, isMouthSuppressedByDmx()); }
static void animHiccupJolt(float tSec) { setEyeStyle(MACRO_DEFAULT_EYE_STYLE); float c = fmodf(tSec, 5.5f); float j = (c > 3.5f && c < 3.85f) ? sinf(((c - 3.5f) / 0.35f) * PI) : 0.0f; renderEyePair(128, clampByte(128.0f - j * 45.0f), clampByte(145.0f - j * 65.0f), clampByte(65.0f - j * 65.0f), MACRO_DEFAULT_EYE_SIZE); setProgrammaticBrows(clampByte(140.0f + j * 115.0f), clampByte(140.0f + j * 115.0f)); drawMouth(clampByte(18.0f + j * 165.0f), 135, clampByte(130.0f - j * 55.0f), TOOTH_DMX_NORMAL, 0, 130, isMouthSuppressedByDmx()); }
static void animCatPurr(float tSec) { setEyeStyle(126); float p = 0.5f + 0.5f * sinf(tSec * 1.4f); renderEyePair(clampByte(128.0f + sinf(tSec * 0.65f) * 35.0f), 132, clampByte(45.0f + p * 75.0f), clampByte(135.0f + p * 45.0f), MACRO_DEFAULT_EYE_SIZE); setProgrammaticBrows(155, 155); drawMouth(12, 225, 110, TOOTH_DMX_DRACULA, 0, 125, isMouthSuppressedByDmx()); }
static void animDogPant(float tSec) { setEyeStyle(32); setRoundEyelids(true); float pt = fabsf(sinf(tSec * 9.5f)); renderEyePair(clampByte(128.0f + sinf(tSec * 1.0f) * 40.0f), 120, 185, 25, MACRO_DEFAULT_EYE_SIZE); setProgrammaticBrows(185, 185); drawMouth(clampByte(115.0f + pt * 75.0f), 220, 165, TOOTH_DMX_NORMAL, clampByte(105.0f + pt * 45.0f), 135, isMouthSuppressedByDmx()); }
static void animFrogCroak(float tSec) { setEyeStyle(32); setRoundEyelids(true); float cr = powf(0.5f - 0.5f * cosf(tSec * 1.4f), 3.0f); renderEyePair(128, 118, 145, clampByte(20.0f + cr * 40.0f), clampByte(MACRO_DEFAULT_EYE_SIZE + cr * 20.0f)); setProgrammaticBrows(165, 165); drawMouth(clampByte(15.0f + cr * 195.0f), 175, clampByte(145.0f + cr * 95.0f), TOOTH_DMX_NONE, clampByte(cr * 85.0f), 155, isMouthSuppressedByDmx()); }
static void animJackOLantern(float tSec) { setEyeStyle(126); float fl = 0.5f + 0.5f * sinf(tSec * 11.0f) * cosf(tSec * 6.3f); renderEyePair(128, 128, clampByte(35.0f + fl * 95.0f), 55, MACRO_DEFAULT_EYE_SIZE); setProgrammaticBrows(15, 15); drawMouth(clampByte(110.0f + fl * 75.0f), 245, 225, 212, 0, 115, isMouthSuppressedByDmx()); }
static void animSkeletonRattle(float tSec) { setEyeStyle(32); setRoundEyelids(true); float c = fmodf(tSec, 6.0f); float a = (c > 2.5f && c < 4.5f) ? 1.0f : 0.0f; float rt = sinf(tSec * 26.0f) * a; renderEyePair(clampByte(128.0f + rt * 22.0f), 128, 25, 10, MACRO_DEFAULT_EYE_SIZE); setProgrammaticBrows(45, 45); drawMouth(clampByte(25.0f + fabsf(rt) * 135.0f), 128, 155, TOOTH_DMX_NORMAL, 0, 75, isMouthSuppressedByDmx()); }
static void animWitchCackle(float tSec) { setEyeStyle(126); float c = fmodf(tSec, 7.5f); float a = (c > 2.5f && c < 5.5f) ? sinf(((c - 2.5f) / 3.0f) * PI) : 0.0f; float ck = fabsf(sinf(tSec * 9.0f)) * a; renderEyePair(clampByte(128.0f + sinf(tSec * 1.8f) * 55.0f), 135, 65, clampByte(85.0f + ck * 30.0f), MACRO_DEFAULT_EYE_SIZE); setProgrammaticBrows(10, 10); drawMouth(clampByte(25.0f + ck * 195.0f), 235, 210, 245, 0, 110, isMouthSuppressedByDmx()); }
static void animMummyCurse(float tSec) { setEyeStyle(MACRO_DEFAULT_EYE_STYLE); float sw = sinf(tSec * 0.75f); renderSingleEyeNormalized(0, clampByte(128.0f + sw * 65.0f), 128, 50, 165, MACRO_DEFAULT_EYE_SIZE); renderSingleEyeNormalized(1, clampByte(128.0f + sw * 65.0f), 128, 50, 115, MACRO_DEFAULT_EYE_SIZE); setProgrammaticBrows(60, 60); drawMouth(clampByte(55.0f + fabsf(sw) * 85.0f), 30, 120, 245, 0, 95, isMouthSuppressedByDmx()); }
static void animCyborgGlitch(float tSec) { setEyeStyle(53); int tick = static_cast<int>(tSec * 10.0f); bool gl = ((tick % 19) == 0); renderSingleEyeNormalized(0, gl ? 40 : 128, gl ? 175 : 128, gl ? 230 : 110, gl ? 180 : 40, MACRO_DEFAULT_EYE_SIZE); renderSingleEyeNormalized(1, gl ? 215 : 128, gl ? 85 : 128, gl ? 30 : 110, 40, MACRO_DEFAULT_EYE_SIZE); setProgrammaticBrows(gl ? 220 : 110, gl ? 35 : 110); drawMouth(gl ? 145 : 22, gl ? 40 : 155, gl ? 195 : 135, TOOTH_DMX_NORMAL, 0, 115, isMouthSuppressedByDmx()); }
static void animAlienProbe(float tSec) { setEyeStyle(53); float p = 0.5f - 0.5f * cosf(tSec * 1.1f); renderEyePair(clampByte(128.0f + sinf(tSec * 1.8f) * 75.0f), clampByte(128.0f - p * 40.0f), clampByte(20.0f + p * 220.0f), 15, clampByte(MACRO_DEFAULT_EYE_SIZE + p * 18.0f)); setProgrammaticBrows(clampByte(128.0f + p * 95.0f), clampByte(128.0f + p * 95.0f)); drawMouth(clampByte(15.0f + p * 110.0f), 128, clampByte(55.0f + p * 35.0f), TOOTH_DMX_NONE, 0, 120, isMouthSuppressedByDmx()); }
static void animNinjaFocus(float tSec) { setEyeStyle(MACRO_DEFAULT_EYE_STYLE); float sn = sinf(tSec * 2.2f); uint8_t gx = (sn > 0.78f) ? 210 : ((sn < -0.78f) ? 45 : 128); renderEyePair(gx, 128, 95, 118, MACRO_DEFAULT_EYE_SIZE); setProgrammaticBrows(20, 20); drawMouth(0, 115, 125, TOOTH_DMX_NONE, 0, 100, isMouthSuppressedByDmx()); }
static void animKingRoyal(float tSec) { setEyeStyle(MACRO_DEFAULT_EYE_STYLE); float nod = sinf(tSec * 0.95f); renderEyePair(clampByte(128.0f + sinf(tSec * 0.5f) * 40.0f), clampByte(124.0f + nod * 16.0f), 135, 68, MACRO_DEFAULT_EYE_SIZE); setProgrammaticBrows(150, 150); drawMouth(clampByte(28.0f + fabsf(nod) * 42.0f), 215, 165, TOOTH_DMX_NORMAL, 0, 145, isMouthSuppressedByDmx()); }
static void animBabyGoo(float tSec) { setEyeStyle(32); setRoundEyelids(true); float b = fabsf(sinf(tSec * 3.2f)); renderEyePair(clampByte(128.0f + sinf(tSec * 1.4f) * 45.0f), 120, 225, 15, clampByte(MACRO_DEFAULT_EYE_SIZE + 14)); setProgrammaticBrows(210, 210); drawMouth(clampByte(45.0f + b * 115.0f), 225, 115, TOOTH_DMX_NONE, clampByte(b * 65.0f), 165, isMouthSuppressedByDmx()); }
static void animMadScientist(float tSec) { setEyeStyle(32); setRoundEyelids(true); float m = sinf(tSec * 4.2f); renderSingleEyeNormalized(0, clampByte(128.0f + m * 55.0f), 115, 45, 0, clampByte(MACRO_DEFAULT_EYE_SIZE + 16)); renderSingleEyeNormalized(1, clampByte(128.0f - m * 45.0f), 140, 195, 95, MACRO_DEFAULT_EYE_SIZE); setProgrammaticBrows(245, 15); drawMouth(clampByte(65.0f + fabsf(m) * 130.0f), 250, 220, TOOTH_DMX_MATER, 0, 125, isMouthSuppressedByDmx()); }
static void animDiscoFever(float tSec) { setEyeStyle(MACRO_DEFAULT_EYE_STYLE); float bt = fabsf(sinf(tSec * 5.2f)); float sd = sinf(tSec * 2.6f); renderEyePair(clampByte(128.0f + sd * 75.0f), clampByte(128.0f - bt * 28.0f), 165, 35, MACRO_DEFAULT_EYE_SIZE); setProgrammaticBrows(clampByte(150.0f + sd * 75.0f), clampByte(150.0f - sd * 75.0f)); drawMouth(clampByte(45.0f + bt * 125.0f), 240, clampByte(155.0f + bt * 55.0f), TOOTH_DMX_NORMAL, 0, 150, isMouthSuppressedByDmx()); }
static void executePresetFrame(MacroPreset preset, uint32_t nowMs) { if (preset != activeMacro) { activeMacro = preset; macroStartMs = nowMs; eyePopOutFactor.fill(0.0f); if (preset == MACRO_AUTO_IDLE) resetAutoIdleState(nowMs); } float tSec = (float)(nowMs - macroStartMs) * 0.001f;
  switch (activeMacro)
  {
    case MACRO_AUTO_IDLE:         animAutoIdle(nowMs);        break;
    case MACRO_CREEPY_STALKER:    animCreepyStalker(tSec);    break;
    case MACRO_SHAPESHIFTER:      animShapeshifter(tSec);     break;
    case MACRO_LAUGHING_SHARK:    animLaughingShark(tSec);    break;
    case MACRO_SMAUG_AWAKENING:   animSmaugAwakening(tSec);   break;
    case MACRO_CROSS_EYED_PIRATE: animCrossEyedPirate(tSec); break;
    case MACRO_GOLF_PANIC:        animGolfPanic(tSec);        break;
    case MACRO_GOOGLY_SPIN:       animGooglySpin(tSec);       break;
    case MACRO_EVIL_LAUGH:        animEvilLaugh(tSec);        break;
    case MACRO_COCKY_ROGUE:       animCockyRogue(tSec);       break;
    case MACRO_SLY_VILLAIN:       animSlyVillain(tSec);       break;
    case MACRO_CHATTY_TALKER:     animChattyTalker(tSec);     break;
    case MACRO_SLY_LICK:          animSlyLick(tSec);          break;
    case MACRO_POPOUT_SHOCK:      animPopOutShock(tSec);      break;
    case MACRO_HYPNOTIC_TRANCE:   animHypnoticTrance(tSec);   break;
    case MACRO_FLIRTY_WINK:       animFlirtyWink(tSec);       break;
    case MACRO_SLEEPY_YAWN:       animSleepyYawn(tSec);       break;
    case MACRO_ANGRY_RAGE:        animAngryRage(tSec);        break;
    case MACRO_ZOMBIE_GROAN:      animZombieGroan(tSec);      break;
    case MACRO_VAMPIRE_HISS:      animVampireHiss(tSec);      break;
    case MACRO_SUSPICIOUS_SQUINT: animSuspiciousSquint(tSec); break;
    case MACRO_OPERA_SINGER:      animOperaSinger(tSec);      break;
    case MACRO_CONFUSED_PUZZLE:   animConfusedPuzzle(tSec);   break;
    case MACRO_EXCITED_BOUNCE:    animExcitedBounce(tSec);    break;
    case MACRO_SHY_BLUSH:         animShyBlush(tSec);         break;
    case MACRO_ROBOT_SCAN:        animRobotScan(tSec);        break;
    case MACRO_DIZZY_STARS:       animDizzyStars(tSec);       break;
    case MACRO_CHEWING_GUM:       animChewingGum(tSec);       break;
    case MACRO_SNEEZE_BUILDUP:    animSneezeBuildup(tSec);    break;
    case MACRO_EVIL_PLAN:         animEvilPlan(tSec);         break;
    case MACRO_CRYING_SOB:        animCryingSob(tSec);        break;
    case MACRO_WHISTLING_TUNE:    animWhistlingTune(tSec);    break;
    case MACRO_SURPRISED_GASP:    animSurprisedGasp(tSec);    break;
    case MACRO_WOLF_HOWL:         animWolfHowl(tSec);         break;
    case MACRO_GIGGLE_FIT:        animGiggleFit(tSec);        break;
    case MACRO_BORED_SIGH:        animBoredSigh(tSec);        break;
    case MACRO_ALIEN_SIGNAL:      animAlienSignal(tSec);      break;
    case MACRO_CLOWN_GRIN:        animClownGrin(tSec);        break;
    case MACRO_SNORING_SLEEP:     animSnoringSleep(tSec);     break;
    case MACRO_NERD_BLINK:        animNerdBlink(tSec);        break;
    case MACRO_MONSTER_ROAR:      animMonsterRoar(tSec);      break;
    case MACRO_SHIVERING_COLD:    animShiveringCold(tSec);    break;
    case MACRO_SPICY_BURN:        animSpicyBurn(tSec);        break;
    case MACRO_DAYDREAM_FLOAT:    animDaydreamFloat(tSec);    break;
    case MACRO_PIRATE_ARRR:       animPirateArrr(tSec);       break;
    case MACRO_TICKLED_LAUGH:     animTickledLaugh(tSec);     break;
    case MACRO_STERN_JUDGE:       animSternJudge(tSec);       break;
    case MACRO_SUPERHERO_POSE:    animSuperheroPose(tSec);    break;
    case MACRO_GHOST_BOO:         animGhostBoo(tSec);         break;
    case MACRO_HICCUP_JOLT:       animHiccupJolt(tSec);       break;
    case MACRO_CAT_PURR:          animCatPurr(tSec);          break;
    case MACRO_DOG_PANT:          animDogPant(tSec);          break;
    case MACRO_FROG_CROAK:        animFrogCroak(tSec);        break;
    case MACRO_JACK_O_LANTERN:    animJackOLantern(tSec);    break;
    case MACRO_SKELETON_RATTLE:   animSkeletonRattle(tSec);   break;
    case MACRO_WITCH_CACKLE:      animWitchCackle(tSec);      break;
    case MACRO_MUMMY_CURSE:       animMummyCurse(tSec);       break;
    case MACRO_CYBORG_GLITCH:     animCyborgGlitch(tSec);     break;
    case MACRO_ALIEN_PROBE:       animAlienProbe(tSec);       break;
    case MACRO_NINJA_FOCUS:       animNinjaFocus(tSec);       break;
    case MACRO_KING_ROYAL:        animKingRoyal(tSec);        break;
    case MACRO_BABY_GOO:          animBabyGoo(tSec);          break;
    case MACRO_MAD_SCIENTIST:     animMadScientist(tSec);     break;
    case MACRO_DISCO_FEVER:       animDiscoFever(tSec);       break;
    case MACRO_MASK_MODES:
    case MACRO_NONE:
    default:
      break;
  }
}

void runDmxMacro(uint8_t macroVal, uint32_t nowMs, bool editOverlay)
{
  (void)editOverlay;
  if (!isDmxMacroActive(macroVal)) { initMacroEngine(); return; }

  if (macroVal >= MACRO_HIDE_THRESHOLD)
  {
    activeMacro      = MACRO_MASK_MODES;
    activeMacroSlot  = 0;
    eyePopOutFactor.fill(0.0f);
    faceHiddenState  = (macroVal == 255);
    mouthHiddenState = (macroVal == 251 || macroVal == 253 || macroVal == 254 || macroVal == 255);
    noseHiddenState  = (macroVal == 252 || macroVal == 253 || macroVal == 254 || macroVal == 255);
    browsHiddenState = (macroVal == 254 || macroVal == 255);

    if (faceHiddenState)
    {
      tft.fillScreen(TFT_BLACK);
      eyeTft.fillScreen(TFT_BLACK);
      return;
    }

    updateEyeViaDmx();
    updateFaceViaDmx(mouthHiddenState);
    return;
  }

  clearHideFlags();
  activeMacroSlot = decodeDmxMacroSlot(macroVal);
  executePresetFrame(mapSlotToAnimation(activeMacroSlot), nowMs);
}
// ------------------------------------------------------------------------------
// 4.1 Reset autonomous show loop index so playback restarts at Bank 00 Slot 0
// ------------------------------------------------------------------------------
void resetAutonomousMacroShow(void)
{
  autoShowSeqIdx       = 0;
  autoShowSlot         = 0;
  activeMacroSlot      = 0;
  autoShowNextSwitchMs = 0;
}

// ------------------------------------------------------------------------------
// 4.2 Advance sequential autonomous macro show across all saved INI slots
// ------------------------------------------------------------------------------
void runAutonomousMacroShow(
    uint32_t nowMs,
    const std::vector<int>& availableSlots,
    const std::map<int, MacroComboPreset>& presets)
{
  clearHideFlags();

  if (availableSlots.empty())
  {
    autoShowSlot = 0;
  }
  else if (autoShowNextSwitchMs == 0 || static_cast<int32_t>(nowMs - autoShowNextSwitchMs) >= 0)
  {
    if (autoShowSeqIdx >= availableSlots.size()) autoShowSeqIdx = 0;
    autoShowSlot         = availableSlots.at(autoShowSeqIdx);
    autoShowSeqIdx       = (autoShowSeqIdx + 1) % availableSlots.size();
    autoShowNextSwitchMs = nowMs + 12000UL;
    DEBUG_PRINTF("[AUTO SHOW] Sequential Macro #%03d\n", autoShowSlot);
  }
  activeMacroSlot = autoShowSlot;

  int savedAnimId = 0;
  auto it = presets.find(autoShowSlot);
  if (it != presets.end() && it->second.enabled && it->second.anim_id > 0)
  {
    savedAnimId = it->second.anim_id;
  }

  MacroPreset targetPreset = (savedAnimId > 0)
    ? static_cast<MacroPreset>(std::max(1, std::min(TOTAL_ANIMATION_COUNT, savedAnimId)))
    : mapSlotToAnimation(autoShowSlot);
  executePresetFrame(targetPreset, nowMs);
}


const char* getMacroAnimationName(int animId)
{
  static const std::array<const char*, 65> kNames = {
    "AUTO SHOW", "AUTO IDLE", "CREEPY STALKER", "SHAPESHIFTER", "LAUGHING SHARK",
    "SMAUG AWAKENING", "CROSS-EYED PIRATE", "GOLF PANIC", "GOOGLY SPIN", "EVIL LAUGH",
    "COCKY ROGUE", "SLY VILLAIN", "CHATTY TALKER", "SLY LICK", "POP-OUT SHOCK",
    "HYPNOTIC TRANCE", "FLIRTY WINK", "SLEEPY YAWN", "ANGRY RAGE", "ZOMBIE GROAN",
    "VAMPIRE HISS", "SUSPICIOUS SQUINT", "OPERA SINGER", "CONFUSED PUZZLE", "EXCITED BOUNCE",
    "SHY BLUSH", "ROBOT SCAN", "DIZZY STARS", "CHEWING GUM", "SNEEZE BUILDUP",
    "EVIL PLAN", "CRYING SOB", "WHISTLING TUNE", "SURPRISED GASP", "WOLF HOWL",
    "GIGGLE FIT", "BORED SIGH", "ALIEN SIGNAL", "CLOWN GRIN", "SNORING SLEEP",
    "NERD BLINK", "MONSTER ROAR", "SHIVERING COLD", "SPICY BURN", "DAYDREAM FLOAT",
    "PIRATE ARRR", "TICKLED LAUGH", "STERN JUDGE", "SUPERHERO POSE", "GHOST BOO",
    "HICCUP JOLT", "CAT PURR", "DOG PANT", "FROG CROAK", "JACK-O-LANTERN",
    "SKELETON RATTLE", "WITCH CACKLE", "MUMMY CURSE", "CYBORG GLITCH", "ALIEN PROBE",
    "NINJA FOCUS", "KING ROYAL", "BABY GOO", "MAD SCIENTIST", "DISCO FEVER"
  };
  if (animId < 0 || animId > TOTAL_ANIMATION_COUNT) return "CUSTOM";
  return kNames.at(animId);
}

void runMacroById(int animId, uint32_t nowMs)
{
  if (animId <= 0)
  {
    clearHideFlags();
    executePresetFrame(MACRO_AUTO_IDLE, nowMs);
    return;
  }
  int clamped = std::max(1, std::min(TOTAL_ANIMATION_COUNT, animId));
  clearHideFlags();
  executePresetFrame(static_cast<MacroPreset>(clamped), nowMs);
}
