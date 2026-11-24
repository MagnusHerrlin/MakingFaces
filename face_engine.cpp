// ==============================================================================
// MakingFaces - Procedural 3D Mouth, Lipstick & Eyebrow Engine
// Architecture designed by Magnus Herrlin 2026
// True ARGB8888 Anti-Aliased 3D Lips, 16 Teeth, Side-Profile Mouths & Tongue
// ==============================================================================

// ==============================================================================
// 1. INCLUDES, CONSTANTS AND INTERNAL STATE
// ==============================================================================
#include "face_engine.h"
#include "audio_engine.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <SDL.h>

extern TFT_eSPI   tft;
extern dmx_port_t dmxPort;
extern int        start_address;

struct LipColorPalette {
  uint16_t baseColor;
  uint16_t shadowColor;
  uint16_t highlightColor;
  uint16_t outlineColor;
};

enum ToothPresetMode {
  TOOTH_MODE_NONE           = 0,
  TOOTH_MODE_NORMAL         = 1,
  TOOTH_MODE_MATER          = 2,
  TOOTH_MODE_DRACULA        = 3,
  TOOTH_MODE_PIRATE         = 4,
  TOOTH_MODE_SHARK          = 5,
  TOOTH_MODE_BUNNY          = 6,
  TOOTH_MODE_GAP            = 7,
  TOOTH_MODE_TUSKS          = 8,
  TOOTH_MODE_UNDERBITE      = 9,
  TOOTH_MODE_VAMPIRE_QUAD   = 10,
  TOOTH_MODE_GRILLZ_GOLD    = 11,
  TOOTH_MODE_GRILLZ_DIAMOND = 12,
  TOOTH_MODE_JACK_O_LANTERN = 13,
  TOOTH_MODE_BRACES         = 14,
  TOOTH_MODE_ZOMBIE         = 15
};
enum ToothColorType {
  TOOTH_COLOR_NONE = 0,
  TOOTH_COLOR_IVORY,
  TOOTH_COLOR_GOLD,
  TOOTH_COLOR_DECAY,
  TOOTH_COLOR_DIAMOND,
  TOOTH_COLOR_PUMPKIN,
  TOOTH_COLOR_WIRE,
  TOOTH_COLOR_BRACKET,
  TOOTH_COLOR_ZOMBIE
};

static LipColorPreset currentLipColor = LIP_COLOR_CLASSIC_RED;

static constexpr LipColorPalette kLipPalettes[10] = {
  { 0xC36D, 0x7987, 0xECBA, 0x48E4 },
  { 0xEB54, 0x998B, 0xFD9B, 0x58A6 },
  { 0xD1AE, 0x8087, 0xF455, 0x4824 },
  { 0xE802, 0x8000, 0xFB4B, 0x4800 },
  { 0x9002, 0x4801, 0xD885, 0x2800 },
  { 0x608B, 0x3006, 0xA252, 0x1803 },
  { 0xE2A8, 0x8923, 0xFCEF, 0x4881 },
  { 0x18C3, 0x0841, 0x52AA, 0x0000 },
  { 0x1A59, 0x08ED, 0x6D5F, 0x0066 },
  { 0xCCA3, 0x72E0, 0xFEA9, 0x3960 }
};

// ==============================================================================
// 2. COLOR BLENDING & HELPER FUNCTIONS
// ==============================================================================
// ------------------------------------------------------------------------------
// 3. Unpack RGB565 value into 0..255 floating-point color channels
// ------------------------------------------------------------------------------
static inline void unpackColor565(uint16_t c, float &r, float &g, float &b)
{
  r = static_cast<float>(((c >> 11) & 0x1F) * 255 / 31);
  g = static_cast<float>(((c >> 5)  & 0x3F) * 255 / 63);
  b = static_cast<float>(( c        & 0x1F) * 255 / 31);
}

// ------------------------------------------------------------------------------
// 4. Linearly interpolate between two RGB565 colors
// ------------------------------------------------------------------------------
static inline void lerpRgb565(uint16_t c1, uint16_t c2, float t, float &outR, float &outG, float &outB)
{
  t = std::max(0.0f, std::min(1.0f, t));
  float r1 = 0.0f, g1 = 0.0f, b1 = 0.0f;
  float r2 = 0.0f, g2 = 0.0f, b2 = 0.0f;
  unpackColor565(c1, r1, g1, b1);
  unpackColor565(c2, r2, g2, b2);
  outR = r1 + (r2 - r1) * t;
  outG = g1 + (g2 - g1) * t;
  outB = b1 + (b2 - b1) * t;
}
// ------------------------------------------------------------------------------
// 5. Alpha-blend a shaded sub-pixel into the ARGB8888 mouth framebuffer
// ------------------------------------------------------------------------------
static inline void blendPixelArgb(int x, int y, float srcR, float srcG, float srcB, float alpha)
{
  if (x < 0 || x >= TFT_eSPI::FB_WIDTH || y < 0 || y >= TFT_eSPI::FB_HEIGHT) return;
  alpha = std::max(0.0f, std::min(1.0f, alpha));
  if (alpha <= 0.005f) return;

  uint32_t &dst = tft.argb_buffer[y * TFT_eSPI::FB_WIDTH + x];
  uint8_t   dstA = static_cast<uint8_t>((dst >> 24) & 0xFF);
  uint8_t   sA   = static_cast<uint8_t>(alpha * 255.0f + 0.5f);
  uint8_t   sR   = static_cast<uint8_t>(std::max(0.0f, std::min(255.0f, srcR + 0.5f)));
  uint8_t   sG   = static_cast<uint8_t>(std::max(0.0f, std::min(255.0f, srcG + 0.5f)));
  uint8_t   sB   = static_cast<uint8_t>(std::max(0.0f, std::min(255.0f, srcB + 0.5f)));

  if (dstA == 0 || sA == 255) {
    dst = (static_cast<uint32_t>(sA) << 24) |
          (static_cast<uint32_t>(sR) << 16) |
          (static_cast<uint32_t>(sG) << 8)  |
           static_cast<uint32_t>(sB);
    return;
  }

  uint8_t dR = static_cast<uint8_t>((dst >> 16) & 0xFF);
  uint8_t dG = static_cast<uint8_t>((dst >> 8)  & 0xFF);
  uint8_t dB = static_cast<uint8_t>( dst        & 0xFF);

  float invA = 1.0f - alpha;
  uint8_t oR = static_cast<uint8_t>(sR * alpha + dR * invA + 0.5f);
  uint8_t oG = static_cast<uint8_t>(sG * alpha + dG * invA + 0.5f);
  uint8_t oB = static_cast<uint8_t>(sB * alpha + dB * invA + 0.5f);
  uint8_t oA = std::max(dstA, sA);

  dst = (static_cast<uint32_t>(oA) << 24) |
        (static_cast<uint32_t>(oR) << 16) |
        (static_cast<uint32_t>(oG) << 8)  |
         static_cast<uint32_t>(oB);
}

// ==============================================================================
// 6. INITIALIZATION & STATE SETTERS
// ==============================================================================
// ------------------------------------------------------------------------------
// 7. Reset lip color preset and brow/mouth atomic parameters on startup
// ------------------------------------------------------------------------------
void initFaceEngine(void)
{
  currentLipColor = LIP_COLOR_CLASSIC_RED;
  g_brow_left_tilt.store(128, std::memory_order_relaxed);
  g_brow_right_tilt.store(128, std::memory_order_relaxed);
  g_effective_mouth_gap.store(0, std::memory_order_relaxed);
}
// ------------------------------------------------------------------------------
// 8. Select active lipstick palette from DMX value or macro/keyboard override
// ------------------------------------------------------------------------------
void setLipColorPreset(uint8_t dmxLipColorVal)
{
  int overrideVal = g_override_lip_color.load(std::memory_order_relaxed);
  if (overrideVal >= 0) {
    int clamped = std::min(9, overrideVal);
    currentLipColor = static_cast<LipColorPreset>(clamped);
    return;
  }
  if (dmxLipColorVal == 0) {
    currentLipColor = LIP_COLOR_CLASSIC_RED;
    return;
  }

  int idx = 1 + ((static_cast<int>(dmxLipColorVal) - 1) * 9) / 255;
  idx = std::max(1, std::min(9, idx));
  currentLipColor = static_cast<LipColorPreset>(idx);
}

// ------------------------------------------------------------------------------
// 9. Store left and right eyebrow tilt targets for the SDL compositor
// ------------------------------------------------------------------------------
void drawEyebrows(uint8_t rawLeft, uint8_t rawRight)
{
  g_brow_left_tilt.store(rawLeft, std::memory_order_relaxed);
  g_brow_right_tilt.store(rawRight, std::memory_order_relaxed);
}

// ==============================================================================
// 10. 3D-SHADED PER-PIXEL TEETH EVALUATOR (ALL 16 PRESETS)
// ==============================================================================
// ------------------------------------------------------------------------------
// 11. Convert tooth material type and shading factor into RGB channel values
// ------------------------------------------------------------------------------
static bool applyToothColor(ToothColorType colorType, float shade, float &r, float &g, float &b)
{
  switch (colorType) {
    case TOOTH_COLOR_IVORY:   r = 238.0f * shade; g = 231.0f * shade; b = 218.0f * shade; return true;
    case TOOTH_COLOR_GOLD:    r = 248.0f * shade; g = 198.0f * shade; b = 34.0f  * shade; return true;
    case TOOTH_COLOR_DECAY:   r = 28.0f  * shade; g = 24.0f  * shade; b = 18.0f  * shade; return true;
    case TOOTH_COLOR_DIAMOND: r = 215.0f + 40.0f * shade; g = 245.0f + 10.0f * shade; b = 255.0f; return true;
    case TOOTH_COLOR_PUMPKIN: r = 255.0f * shade; g = 165.0f * shade; b = 35.0f  * shade; return true;
    case TOOTH_COLOR_WIRE:    r = 165.0f * shade; g = 175.0f * shade; b = 188.0f * shade; return true;
    case TOOTH_COLOR_BRACKET: r = 25.0f  * shade; g = 155.0f * shade; b = 245.0f * shade; return true;
    case TOOTH_COLOR_ZOMBIE:  r = 178.0f * shade; g = 188.0f * shade; b = 115.0f * shade; return true;
    case TOOTH_COLOR_NONE:
    default: return false;
  }
}
// ------------------------------------------------------------------------------
// 12. Evaluate pirate tooth layout with single gold tooth and decayed lower tooth
// ------------------------------------------------------------------------------
static ToothColorType evalPirateTooth(float normX, float absX, float cavPos)
{
  if (cavPos < 0.46f && absX < 0.58f) {
    int   toothIdx    = -1;
    float toothCenter = 0.0f;
    if      (normX >= -0.58f && normX <= -0.32f) { toothIdx = 0; toothCenter = -0.45f; }
    else if (normX >= -0.28f && normX <= -0.02f) { toothIdx = 1; toothCenter = -0.15f; }
    else if (normX >=  0.02f && normX <=  0.28f) { toothIdx = 2; toothCenter =  0.15f; }
    else if (normX >=  0.32f && normX <=  0.58f) { toothIdx = 3; toothCenter =  0.45f; }
    if (toothIdx >= 0) {
      float u = (normX - toothCenter) / 0.13f;
      if (cavPos < 0.45f - (u * u * 0.06f)) return (toothIdx == 2) ? TOOTH_COLOR_GOLD : TOOTH_COLOR_IVORY;
    }
  } else if (cavPos > 0.60f && absX < 0.52f) {
    int   toothIdx    = -1;
    float toothCenter = 0.0f;
    if      (normX >= -0.52f && normX <= -0.29f) { toothIdx = 0; toothCenter = -0.405f; }
    else if (normX >= -0.25f && normX <= -0.02f) { toothIdx = 1; toothCenter = -0.135f; }
    else if (normX >=  0.02f && normX <=  0.25f) { toothIdx = 2; toothCenter =  0.135f; }
    else if (normX >=  0.29f && normX <=  0.52f) { toothIdx = 3; toothCenter =  0.405f; }
    if (toothIdx >= 0) {
      float u = (normX - toothCenter) / 0.115f;
      if (cavPos > 0.62f + (u * u * 0.05f)) return (toothIdx == 1) ? TOOTH_COLOR_DECAY : TOOTH_COLOR_IVORY;
    }
  }
  return TOOTH_COLOR_NONE;
}

// ------------------------------------------------------------------------------
// 13. Evaluate tooth coverage and shading for any of the 16 tooth style presets
// ------------------------------------------------------------------------------
static bool evalToothPixel(ToothPresetMode mode, float normX, float absX, float cavPos, float &r, float &g, float &b)
{
  if (mode == TOOTH_MODE_NONE || absX >= 0.84f) return false;
  ToothColorType colorType = TOOTH_COLOR_NONE;
  float          shade     = 1.0f - (absX * 0.35f);

  switch (mode) {
    case TOOTH_MODE_NORMAL: {
      float inTooth = fabsf((normX * 4.5f) - roundf(normX * 4.5f));
      if (inTooth < 0.40f && (cavPos < 0.28f || cavPos > 0.76f)) colorType = TOOTH_COLOR_IVORY;
      break;
    }
    case TOOTH_MODE_MATER:
      if (absX > 0.03f && absX < 0.28f && cavPos < 0.56f) colorType = TOOTH_COLOR_IVORY;
      break;
    case TOOTH_MODE_DRACULA: {
      float fangDist = fabsf(absX - 0.44f);
      if (fangDist < 0.13f && cavPos < 0.62f * (1.0f - (fangDist / 0.13f))) colorType = TOOTH_COLOR_IVORY;
      break;
    }
    case TOOTH_MODE_PIRATE: colorType = evalPirateTooth(normX, absX, cavPos); break;
    case TOOTH_MODE_SHARK: {
      float saw = fabsf((normX * 5.5f) - roundf(normX * 5.5f)) * 2.0f;
      if (cavPos < 0.46f * (1.0f - saw) || cavPos > 1.0f - (0.42f * (1.0f - saw))) colorType = TOOTH_COLOR_IVORY;
      break;
    }
    case TOOTH_MODE_BUNNY:
      if (absX < 0.22f && fabsf(normX) > 0.015f && cavPos < 0.64f) colorType = TOOTH_COLOR_IVORY;
      break;
    case TOOTH_MODE_GAP: {
      float inTooth = fabsf((normX * 4.5f) - roundf(normX * 4.5f));
      bool missing  = (normX > 0.02f && normX < 0.24f && cavPos < 0.5f);
      if (!missing && inTooth < 0.40f && (cavPos < 0.30f || cavPos > 0.74f)) colorType = TOOTH_COLOR_IVORY;
      break;
    }
    case TOOTH_MODE_TUSKS: {
      float tuskDist = fabsf(absX - 0.50f);
      if (tuskDist < 0.14f && cavPos > 0.35f + (tuskDist * 3.5f)) colorType = TOOTH_COLOR_IVORY;
      break;
    }
    case TOOTH_MODE_UNDERBITE: {
      float inTooth = fabsf((normX * 4.0f) - roundf(normX * 4.0f));
      if (inTooth < 0.38f && cavPos > 0.64f) colorType = TOOTH_COLOR_IVORY;
      break;
    }
    case TOOTH_MODE_VAMPIRE_QUAD: {
      float fangDist = fabsf(absX - 0.44f);
      if (fangDist < 0.12f) {
        float taper = 1.0f - (fangDist / 0.12f);
        if (cavPos < 0.56f * taper || cavPos > 1.0f - (0.46f * taper)) colorType = TOOTH_COLOR_IVORY;
      }
      break;
    }
    case TOOTH_MODE_GRILLZ_GOLD: {
      float inTooth = fabsf((normX * 4.5f) - roundf(normX * 4.5f));
      if (inTooth < 0.42f && (cavPos < 0.30f || cavPos > 0.74f)) colorType = TOOTH_COLOR_GOLD;
      break;
    }
    case TOOTH_MODE_GRILLZ_DIAMOND: {
      float cell = normX * 4.5f, inTooth = fabsf(cell - roundf(cell));
      if (inTooth < 0.42f && (cavPos < 0.30f || cavPos > 0.74f)) {
        int sparkle = (static_cast<int>(roundf(cell * 6.0f)) + static_cast<int>(cavPos * 20.0f)) & 1;
        colorType   = sparkle ? TOOTH_COLOR_DIAMOND : TOOTH_COLOR_IVORY;
      }
      break;
    }
    case TOOTH_MODE_JACK_O_LANTERN: {
      float cell = normX * 3.2f, inTooth = fabsf(cell - roundf(cell));
      int colIdx = static_cast<int>(roundf(cell)) + 3;
      if (inTooth < 0.32f) {
        if ((colIdx % 2 == 1) && cavPos < 0.36f) colorType = TOOTH_COLOR_PUMPKIN;
        if ((colIdx % 2 == 0) && cavPos > 0.66f) colorType = TOOTH_COLOR_PUMPKIN;
      }
      break;
    }
    case TOOTH_MODE_BRACES: {
      float cell = normX * 4.5f, inTooth = fabsf(cell - roundf(cell));
      if (inTooth < 0.40f && (cavPos < 0.30f || cavPos > 0.74f)) {
        colorType = TOOTH_COLOR_IVORY;
        if (fabsf(cavPos - 0.16f) < 0.025f || fabsf(cavPos - 0.86f) < 0.025f)
          colorType = (inTooth < 0.14f) ? TOOTH_COLOR_BRACKET : TOOTH_COLOR_WIRE;
      } else if (fabsf(cavPos - 0.16f) < 0.018f || fabsf(cavPos - 0.86f) < 0.018f) {
        colorType = TOOTH_COLOR_WIRE;
      }
      break;
    }
    case TOOTH_MODE_ZOMBIE: {
      float cell = normX * 4.5f, inTooth = fabsf(cell - roundf(cell));
      int colIdx = static_cast<int>(roundf(cell)) + 4;
      if (colIdx == 2 || colIdx == 6) break;
      float maxTop = (colIdx % 2 == 0) ? 0.32f : 0.22f;
      if (inTooth < 0.36f && (cavPos < maxTop || cavPos > 0.76f))
        colorType = (colIdx % 3 == 0) ? TOOTH_COLOR_DECAY : TOOTH_COLOR_ZOMBIE;
      break;
    }
    case TOOTH_MODE_NONE:
    default: break;
  }
  return applyToothColor(colorType, shade, r, g, b);
}

// ==============================================================================
// 14. PROCEDURAL 3D MOUTH, SIDE-PROFILE MOUTHS & TONGUE RENDERER
// ==============================================================================
struct MouthColumnState {
  float normX, absX, arch, upperOuterY, upperInnerY, lowerInnerY, lowerOuterY, invUpperH, invLowerH, invCavH, cornerAlpha;
};

// ------------------------------------------------------------------------------
// 15. Shade interior mouth cavity, tongue floor and lower teeth
// ------------------------------------------------------------------------------
static void shadeMouthCavity(const MouthColumnState &col, float cavPos, float gapHeight, bool showTongue, ToothPresetMode teethMode, float &r, float &g, float &b)
{
  float cd = cavPos - 0.5f, shade = fmaxf(0.0f, (1.0f - (cd * cd * 3.6f)) * col.arch);
  r = 34.0f * shade; g = 3.0f * shade; b = 5.0f * shade;
  if (!showTongue && gapHeight > 7.5f && col.absX < 0.74f) {
    float tongueTop = 0.54f + (col.absX * col.absX * 0.42f);
    if (cavPos > tongueTop) {
      float tDepth = fminf(1.0f, (cavPos - tongueTop) * 6.5f) * (1.0f - (col.absX * 0.62f));
      float crease = 1.0f - (0.34f * fmaxf(0.0f, 1.0f - (col.normX * col.normX * 22.0f)));
      r = (165.0f * tDepth + 35.0f) * crease; g = (34.0f * tDepth + 6.0f) * crease; b = (46.0f * tDepth + 8.0f) * crease;
    }
  }
  if (gapHeight > 6.5f && cavPos > 0.5f) evalToothPixel(teethMode, col.normX, col.absX, cavPos, r, g, b);
}
// ------------------------------------------------------------------------------
// 16. Shade 3D upper front lip with specular highlight
// ------------------------------------------------------------------------------
static bool shadeUpperLip(const MouthColumnState &col, float fy, const LipColorPalette &pal, float fullness, float &r, float &g, float &b, float &alpha)
{
  float v = (fy - col.upperOuterY) * col.invUpperH;
  float eA = (v < 0.0f) ? fmaxf(0.0f, 1.0f + (v * 2.4f)) : ((v > 1.0f) ? fmaxf(0.0f, 1.0f - ((v - 1.0f) * 3.5f)) : 1.0f);
  alpha = eA * col.cornerAlpha;
  if (alpha <= 0.01f) return false;
  float vc = fmaxf(0.0f, fminf(1.0f, v)), cR = 0.0f, cG = 0.0f, cB = 0.0f;
  if (vc < 0.20f) lerpRgb565(pal.outlineColor, pal.highlightColor, vc / 0.20f, cR, cG, cB);
  else if (vc < 0.56f) lerpRgb565(pal.highlightColor, pal.baseColor, (vc - 0.20f) / 0.36f, cR, cG, cB);
  else lerpRgb565(pal.baseColor, pal.shadowColor, (vc - 0.56f) / 0.44f, cR, cG, cB);

  float highlightBoost = (0.45f + 0.55f * fullness) * (1.0f - col.absX * 0.45f);
  float ld = (vc - 0.24f) * 2.5f, light = fmaxf(0.0f, 1.0f - ld * ld) * highlightBoost;
  if (light > 0.01f) {
    float hR = 0.0f, hG = 0.0f, hB = 0.0f; unpackColor565(pal.highlightColor, hR, hG, hB);
    float amt = std::min(0.65f, light * 0.55f);
    cR += (hR - cR) * amt; cG += (hG - cG) * amt; cB += (hB - cB) * amt;
  }
  r = cR; g = cG; b = cB; return true;
}

// ------------------------------------------------------------------------------
// 17. Shade 3D lower front lip with glossy highlight
// ------------------------------------------------------------------------------
static bool shadeLowerLip(const MouthColumnState &col, float fy, const LipColorPalette &pal, float fullness, float &r, float &g, float &b, float &alpha)
{
  float v = (fy - col.lowerInnerY) * col.invLowerH;
  float eA = (v < 0.0f) ? fmaxf(0.0f, 1.0f + (v * 3.2f)) : ((v > 1.0f) ? fmaxf(0.0f, 1.0f - ((v - 1.0f) * 2.2f)) : 1.0f);
  alpha = eA * col.cornerAlpha;
  if (alpha <= 0.01f) return false;
  float vc = fmaxf(0.0f, fminf(1.0f, v)), cR = 0.0f, cG = 0.0f, cB = 0.0f;
  if (vc < 0.24f) lerpRgb565(pal.shadowColor, pal.baseColor, vc / 0.24f, cR, cG, cB);
  else if (vc < 0.78f) lerpRgb565(pal.baseColor, pal.shadowColor, ((vc - 0.24f) / 0.54f) * 0.35f, cR, cG, cB);
  else lerpRgb565(pal.baseColor, pal.outlineColor, (vc - 0.78f) / 0.22f, cR, cG, cB);

  float gloss = std::exp(-std::pow(col.absX / 0.42f, 2.0f)) * std::exp(-std::pow((vc - 0.46f) / 0.20f, 2.0f)) * (0.45f + 0.55f * fullness);
  if (gloss > 0.01f) {
    float hR = 0.0f, hG = 0.0f, hB = 0.0f; unpackColor565(pal.highlightColor, hR, hG, hB);
    float gAmt = std::min(0.90f, gloss * 0.85f);
    cR += (hR - cR) * gAmt; cG += (hG - cG) * gAmt; cB += (hB - cB) * gAmt;
  }
  r = cR; g = cG; b = cB; return true;
}
// ------------------------------------------------------------------------------
// 18. Render protruding 3D tongue variant for front mouths
// ------------------------------------------------------------------------------
static void renderRollingStonesTongue(float cx, float rootY, float tongueW, float tongueLen, float tongueOut)
{
  if (tongueLen <= 1.5f || tongueW <= 4.0f) return;
  float sizeRamp = std::min(1.0f, tongueLen / std::max(18.0f, tongueW * 0.85f));
  float effWidth = 6.0f + (tongueW - 6.0f) * sizeRamp;
  float bendFactor = std::max(0.0f, (tongueOut - 0.25f) / 0.75f);
  float sideSweep = 16.0f * bendFactor * bendFactor, archLift = 18.0f * bendFactor;
  constexpr int kSamples = 36;
  float spX[kSamples + 1], spY[kSamples + 1], spR[kSamples + 1];
  for (int i = 0; i <= kSamples; ++i) {
    float t = static_cast<float>(i) / static_cast<float>(kSamples);
    spX[i] = cx + sideSweep * std::pow(t, 1.85f);
    spY[i] = rootY + (t * tongueLen) - (archLift * std::sin(t * 3.14159f));
    spR[i] = effWidth * (0.98f + 0.12f * std::sin(t * 3.14159f)) * (1.0f - 0.18f * t * t);
  }
  int32_t xMin = std::max(0, static_cast<int32_t>(cx - effWidth - 14.0f)), xMax = std::min(BASE_CANVAS_W - 1, static_cast<int32_t>(cx + effWidth + sideSweep + 14.0f));
  int32_t yMin = std::max(0, static_cast<int32_t>(rootY - archLift - 10.0f)), yMax = std::min(BASE_CANVAS_H - 1, static_cast<int32_t>(rootY + tongueLen + effWidth + 10.0f));
  for (int32_t y = yMin; y <= yMax; ++y) {
    float fy = static_cast<float>(y) + 0.5f; if (fy < rootY - 1.5f) continue;
    for (int32_t x = xMin; x <= xMax; ++x) {
      float fx = static_cast<float>(x) + 0.5f, bestSignedDist = -1000.0f, bestT = 0.0f, bestLat = 0.0f;
      for (int i = 0; i <= kSamples; ++i) {
        float dx = fx - spX[i], dy = fy - spY[i], sd = spR[i] - std::sqrt(dx * dx + dy * dy);
        if (sd > bestSignedDist) { bestSignedDist = sd; bestT = static_cast<float>(i) / static_cast<float>(kSamples); bestLat = (spR[i] > 0.1f) ? (dx / spR[i]) : 0.0f; }
      }
      if (bestSignedDist < -1.2f) continue;
      float alpha = std::max(0.0f, std::min(1.0f, (bestSignedDist + 0.9f) / 1.8f)) * std::max(0.0f, std::min(1.0f, (fy - rootY + 1.0f) / 2.2f));
      if (alpha <= 0.01f) continue;
      float grooveMask = std::exp(-std::pow(bestLat / 0.18f, 2.0f)) * std::sin(std::min(1.0f, bestT * 1.15f) * 2.85f);
      float shade = (1.0f - 0.50f * std::max(0.0f, grooveMask)) * ((bestSignedDist < 3.2f) ? (0.60f + 0.40f * (std::max(0.0f, bestSignedDist) / 3.2f)) : 1.0f) * ((bestT < 0.20f) ? (0.66f + 0.34f * (bestT / 0.20f)) : 1.0f) * (1.0f + 0.14f * std::sin(bestT * 3.14159f) * bendFactor);
      float gx = (bestLat + 0.34f) * 2.5f, gy = (bestT - 0.44f) * 2.3f, gloss = std::max(0.0f, 1.0f - (gx * gx + gy * gy));
      blendPixelArgb(x, y, 238.0f * shade + 60.0f * gloss, 34.0f * shade + 44.0f * gloss, 52.0f * shade + 48.0f * gloss, alpha);
    }
  }
}

// ------------------------------------------------------------------------------
// 19. Render lip-licking 3D tongue variant sweeping across upper or lower lip
// ------------------------------------------------------------------------------
static void renderLipLickingTongue(float cx, float cy, float rootY, float tongueW, float halfWidth, float smileBend, float gapHeight, float sweepNorm, bool lickUpperLip)
{
  float maxSweepX = std::min(halfWidth * 0.68f, 72.0f), tipX = cx + sweepNorm * maxSweepX;
  float arch = std::max(0.0f, 1.0f - (sweepNorm * sweepNorm * 0.65f)), lipLineY = cy - arch * smileBend;
  float tipY = lipLineY + (lickUpperLip ? (-gapHeight * 0.36f - 8.5f * arch) : (gapHeight * 0.78f + 10.5f * arch));
  float startY = rootY + (lickUpperLip ? 4.0f : 6.0f), spanY = tipY - startY;
  if (std::fabs(spanY) < 4.0f) return;
  float baseHalfW = std::min(tongueW * 0.42f, 22.0f), tipHalfW = std::max(6.5f, baseHalfW * 0.38f);
  int32_t xMin = std::max(0, static_cast<int32_t>(std::min(cx - baseHalfW, tipX - tipHalfW) - 8.0f)), xMax = std::min(BASE_CANVAS_W - 1, static_cast<int32_t>(std::max(cx + baseHalfW, tipX + tipHalfW) + 8.0f));
  int32_t yMin = std::max(0, static_cast<int32_t>(std::min(startY, tipY) - tipHalfW - 4.0f)), yMax = std::min(BASE_CANVAS_H - 1, static_cast<int32_t>(std::max(startY, tipY) + tipHalfW + 4.0f));
  for (int32_t y = yMin; y <= yMax; ++y) {
    float fy = static_cast<float>(y) + 0.5f, t = (fy - startY) / spanY;
    if (t < -0.05f || t > 1.18f) continue;
    float tc = std::max(0.0f, std::min(1.0f, t)), spineX = cx + (tipX - cx) * (tc * tc * (3.0f - 2.0f * tc)), halfSpan = baseHalfW + (tipHalfW - baseHalfW) * tc;
    if (t > 1.0f) { float tipOver = (t - 1.0f) / 0.18f; if (tipOver >= 1.0f) continue; halfSpan *= std::sqrt(std::max(0.0f, 1.0f - tipOver * tipOver)); }
    if (halfSpan <= 0.5f) continue;
    for (int32_t x = xMin; x <= xMax; ++x) {
      float fx = static_cast<float>(x) + 0.5f, dx = fx - spineX, distPx = halfSpan - std::fabs(dx);
      if (distPx < -1.2f) continue;
      float alpha = std::max(0.0f, std::min(1.0f, (distPx + 0.9f) / 1.8f)) * std::max(0.0f, std::min(1.0f, (t + 0.04f) / 0.16f));
      if (alpha <= 0.01f) continue;
      float lat = dx / std::max(1.0f, halfSpan), grooveMask = std::exp(-std::pow(lat / 0.22f, 2.0f)) * std::sin(tc * 2.85f);
      float shade = (1.0f - 0.45f * std::max(0.0f, grooveMask)) * ((distPx < 3.0f) ? (0.62f + 0.38f * (std::max(0.0f, distPx) / 3.0f)) : 1.0f) * ((tc < 0.25f) ? (0.58f + 0.42f * (tc / 0.25f)) : 1.0f);
      float gx = (lat + 0.32f) * 2.2f, gy = (tc - 0.62f) * 2.2f, gloss = std::max(0.0f, 1.0f - (gx * gx + gy * gy));
      blendPixelArgb(x, y, 238.0f * shade + 62.0f * gloss, 34.0f * shade + 44.0f * gloss, 52.0f * shade + 48.0f * gloss, alpha);
    }
  }
}
// ------------------------------------------------------------------------------
// 20. Render single 3D side-profile mouth with rounded lips, teeth & tongue
// ------------------------------------------------------------------------------
static void renderSingleSideProfileMouth(
    float cornerX, float tipX, float baseY, float gapHeight, float smileBend,
    float fullness, float scaleMult, ToothPresetMode teethMode, float tongueOut,
    const LipColorPalette &pal)
{
  float dir = (tipX >= cornerX) ? 1.0f : -1.0f;
  float spanX = std::fabs(tipX - cornerX);
  if (spanX < 6.0f) return;

  float upperScale = (0.52f + fullness * 1.35f) * scaleMult;
  float lowerScale = (0.56f + fullness * 1.48f) * scaleMult;
  float openHalf   = gapHeight * 0.56f;

  auto evalProfileGeom = [&](float uc, float& outUpperInner, float& outLowerInner, float& outUpperOuter, float& outLowerOuter) {
    float env = std::sin(uc * 1.570796f);
    float cornerShift = -smileBend * 0.72f * (1.0f - uc) * (1.0f - uc);
    float centerLineY = baseY + cornerShift;
    outUpperInner = centerLineY - env * openHalf * 0.88f;
    outLowerInner = centerLineY + env * openHalf * 1.08f;

    float tipT = std::max(0.0f, (uc - 0.72f) / 0.28f);
    float tipRound = std::sqrt(std::max(0.0f, 1.0f - tipT * tipT));
    float cornerTaper = std::pow(std::sin(uc * 3.14159f), 0.55f) * 0.55f + env * tipRound * 0.65f;
    outUpperOuter = outUpperInner - (1.8f + 17.5f * cornerTaper * upperScale);
    outLowerOuter = outLowerInner + (1.8f + 20.0f * cornerTaper * lowerScale);
  };

  int32_t xMin = std::max(0, static_cast<int32_t>(std::min(cornerX, tipX) - 8.0f));
  int32_t xMax = std::min(BASE_CANVAS_W - 1, static_cast<int32_t>(std::max(cornerX, tipX) + 10.0f));

  for (int32_t x = xMin; x <= xMax; ++x) {
    float fx = static_cast<float>(x) + 0.5f;
    float u = ((fx - cornerX) * dir) / spanX;
    if (u < -0.04f || u > 1.02f) continue;
    float uc = std::max(0.0f, std::min(1.0f, u));
    float cornerAlpha = std::max(0.0f, std::min(1.0f, (u + 0.04f) * 10.0f)) *
                        std::max(0.0f, std::min(1.0f, (1.02f - u) * 14.0f));
    float upperInnerY = 0, lowerInnerY = 0, upperOuterY = 0, lowerOuterY = 0;
    evalProfileGeom(uc, upperInnerY, lowerInnerY, upperOuterY, lowerOuterY);
    float cavH = lowerInnerY - upperInnerY;
    if (cavH <= 2.0f) continue;

    int32_t yMin = std::max(0, static_cast<int32_t>(upperInnerY));
    int32_t yMax = std::min(BASE_CANVAS_H - 1, static_cast<int32_t>(lowerInnerY));
    for (int32_t y = yMin; y <= yMax; ++y) {
      float fy = static_cast<float>(y) + 0.5f;
      if (fy <= upperInnerY || fy >= lowerInnerY) continue;
      float cavPos = (fy - upperInnerY) / cavH;
      float shade = 0.52f + 0.48f * uc;
      float r = 88.0f * shade, g = 10.0f * shade, b = 16.0f * shade;
      if (tongueOut <= 0.05f && cavH > 5.5f && cavPos > 0.52f && uc > 0.25f) {
        float tDepth = std::min(1.0f, (cavPos - 0.52f) * 4.5f) * std::min(1.0f, (uc - 0.25f) * 2.5f);
        r = (175.0f * tDepth + 55.0f) * shade;
        g = (30.0f * tDepth + 8.0f) * shade;
        b = (42.0f * tDepth + 12.0f) * shade;
      }
      blendPixelArgb(x, y, r, g, b, cornerAlpha);
    }
  }
  if (tongueOut > 0.05f && openHalf > 2.5f) {
    float tLen = tongueOut * 56.0f * scaleMult;
    float tRad = std::min(openHalf * 0.72f, 12.5f * scaleMult);
    float tStartX = cornerX + dir * (spanX * 0.45f);
    float tEndX   = tipX + dir * tLen;
    float tStartY = baseY + openHalf * 0.22f;
    float tDropY  = baseY + 5.5f + tongueOut * 14.0f * scaleMult;
    int32_t txMin = std::max(0, static_cast<int32_t>(std::min(tStartX, tEndX) - tRad - 4.0f));
    int32_t txMax = std::min(BASE_CANVAS_W - 1, static_cast<int32_t>(std::max(tStartX, tEndX) + tRad + 4.0f));
    int32_t tyMin = std::max(0, static_cast<int32_t>(std::min(tStartY, tDropY) - tRad - 4.0f));
    int32_t tyMax = std::min(BASE_CANVAS_H - 1, static_cast<int32_t>(std::max(tStartY, tDropY) + tRad + 4.0f));
    for (int32_t y = tyMin; y <= tyMax; ++y) {
      float fy = static_cast<float>(y) + 0.5f;
      for (int32_t x = txMin; x <= txMax; ++x) {
        float fx = static_cast<float>(x) + 0.5f;
        if ((fx - tStartX) * dir < 0.0f) continue;
        float t = std::max(0.0f, std::min(1.0f, (fx - tStartX) / (tEndX - tStartX)));
        float spineX = tStartX + (tEndX - tStartX) * t;
        float spineY = tStartY + (tDropY - tStartY) * (t * t);
        float rad = tRad * (1.0f - 0.25f * t);
        float dist = rad - std::sqrt((fx - spineX) * (fx - spineX) + (fy - spineY) * (fy - spineY));
        if (dist < -1.0f) continue;
        float rootFade = std::min(1.0f, t * 4.5f);
        float alpha = std::max(0.0f, std::min(1.0f, (dist + 0.8f) / 1.6f)) * rootFade;
        float groove = 1.0f - 0.35f * std::exp(-std::pow((fy - spineY) / (rad * 0.28f), 2.0f));
        float shade = (0.72f + 0.28f * (std::max(0.0f, dist) / std::max(1.0f, rad))) * groove;
        blendPixelArgb(x, y, 236.0f * shade, 36.0f * shade, 54.0f * shade, alpha);
      }
    }
  }

  if (teethMode != TOOTH_MODE_NONE) {
    for (int32_t x = xMin; x <= xMax; ++x) {
      float fx = static_cast<float>(x) + 0.5f;
      float u = ((fx - cornerX) * dir) / spanX;
      if (u <= 0.22f || u >= 0.94f) continue;
      float upperInnerY = 0, lowerInnerY = 0, upperOuterY = 0, lowerOuterY = 0;
      evalProfileGeom(u, upperInnerY, lowerInnerY, upperOuterY, lowerOuterY);
      float cavH = lowerInnerY - upperInnerY;
      if (cavH <= 5.0f) continue;

      float toothCell = (u - 0.22f) * 6.5f;
      float inTooth   = fabsf(toothCell - roundf(toothCell));
      int   toothIdx  = static_cast<int>(roundf(toothCell));
      bool  isFang    = (teethMode == TOOTH_MODE_DRACULA || teethMode == TOOTH_MODE_VAMPIRE_QUAD) && (toothIdx == 2);
      float topLimit  = isFang ? 0.62f * (1.0f - inTooth * 1.4f) : 0.32f;
      float botLimit  = (teethMode == TOOTH_MODE_VAMPIRE_QUAD && toothIdx == 2) ? (1.0f - 0.52f * (1.0f - inTooth * 1.4f)) : 0.72f;
      float tShade    = 0.78f + 0.22f * u;
      ToothColorType tCol = (teethMode == TOOTH_MODE_GRILLZ_GOLD) ? TOOTH_COLOR_GOLD
                          : (teethMode == TOOTH_MODE_ZOMBIE)      ? TOOTH_COLOR_ZOMBIE : TOOTH_COLOR_IVORY;

      int32_t yMin = std::max(0, static_cast<int32_t>(upperInnerY));
      int32_t yMax = std::min(BASE_CANVAS_H - 1, static_cast<int32_t>(lowerInnerY));
      for (int32_t y = yMin; y <= yMax; ++y) {
        float fy = static_cast<float>(y) + 0.5f;
        if (fy <= upperInnerY || fy >= lowerInnerY) continue;
        float cavPos = (fy - upperInnerY) / cavH;
        bool drawSideTooth = isFang
          ? ((cavPos < topLimit) || (teethMode == TOOTH_MODE_VAMPIRE_QUAD && cavPos > botLimit))
          : ((inTooth < 0.38f) && ((cavPos < topLimit) || (cavPos > botLimit)));
        if (drawSideTooth) {
          float r = 0, g = 0, b = 0;
          if (applyToothColor(tCol, tShade, r, g, b)) blendPixelArgb(x, y, r, g, b, 1.0f);
        }
      }
    }
  }
  for (int32_t x = xMin; x <= xMax; ++x) {
    float fx = static_cast<float>(x) + 0.5f;
    float u = ((fx - cornerX) * dir) / spanX;
    if (u < -0.04f || u > 1.03f) continue;
    float uc = std::max(0.0f, std::min(1.0f, u));
    float cornerAlpha = std::max(0.0f, std::min(1.0f, (u + 0.04f) * 10.0f)) *
                        std::max(0.0f, std::min(1.0f, (1.03f - u) * 14.0f));

    float upperInnerY = 0, lowerInnerY = 0, upperOuterY = 0, lowerOuterY = 0;
    evalProfileGeom(uc, upperInnerY, lowerInnerY, upperOuterY, lowerOuterY);
    float upH = std::max(2.0f, upperInnerY - upperOuterY);
    float loH = std::max(2.0f, lowerOuterY - lowerInnerY);

    int32_t yMin = std::max(0, static_cast<int32_t>(upperOuterY - 4.0f));
    int32_t yMax = std::min(BASE_CANVAS_H - 1, static_cast<int32_t>(lowerOuterY + 4.0f));
    for (int32_t y = yMin; y <= yMax; ++y) {
      float fy = static_cast<float>(y) + 0.5f;
      if (fy >= upperOuterY - 3.0f && fy <= upperInnerY + 1.5f) {
        float v = (fy - upperOuterY) / upH;
        float eA = (v < 0.0f) ? fmaxf(0.0f, 1.0f + v * 2.5f) : ((v > 1.0f) ? fmaxf(0.0f, 1.0f - (v - 1.0f) * 3.5f) : 1.0f);
        float a = eA * cornerAlpha;
        if (a > 0.01f) {
          float vc = std::max(0.0f, std::min(1.0f, v)), cR = 0, cG = 0, cB = 0;
          if (vc < 0.22f) lerpRgb565(pal.outlineColor, pal.highlightColor, vc / 0.22f, cR, cG, cB);
          else if (vc < 0.60f) lerpRgb565(pal.highlightColor, pal.baseColor, (vc - 0.22f) / 0.38f, cR, cG, cB);
          else lerpRgb565(pal.baseColor, pal.shadowColor, (vc - 0.60f) / 0.40f, cR, cG, cB);
          blendPixelArgb(x, y, cR, cG, cB, a);
        }
      }
      if (fy >= lowerInnerY - 1.5f && fy <= lowerOuterY + 3.5f) {
        float v = (fy - lowerInnerY) / loH;
        float eA = (v < 0.0f) ? fmaxf(0.0f, 1.0f + v * 3.2f) : ((v > 1.0f) ? fmaxf(0.0f, 1.0f - (v - 1.0f) * 2.3f) : 1.0f);
        float a = eA * cornerAlpha;
        if (a > 0.01f) {
          float vc = std::max(0.0f, std::min(1.0f, v)), cR = 0, cG = 0, cB = 0;
          if (vc < 0.25f) lerpRgb565(pal.shadowColor, pal.baseColor, vc / 0.25f, cR, cG, cB);
          else if (vc < 0.78f) lerpRgb565(pal.baseColor, pal.highlightColor, ((vc - 0.25f) / 0.53f) * 0.45f, cR, cG, cB);
          else lerpRgb565(pal.baseColor, pal.outlineColor, (vc - 0.78f) / 0.22f, cR, cG, cB);
          blendPixelArgb(x, y, cR, cG, cB, a);
        }
      }
    }
  }
}
// ------------------------------------------------------------------------------
// 21. Render single or dual 3D side-profile mouths facing inward toward center
// ------------------------------------------------------------------------------
static void renderSideProfileMouths(
    uint8_t faceId, bool dualMode, float gapHeight, float smileBend,
    float widthNorm, float fullness, float scaleMult, uint8_t rawWidth,
    uint8_t rawGap, ToothPresetMode teethMode, bool showTongue, float tongueOut,
    const LipColorPalette &pal)
{
  static int      s_speakerSide  = 0;
  static uint32_t s_lastSpeechMs = 0;
  static uint32_t s_turnStartMs  = 0;
  static bool     s_wasSpeaking  = false;
  uint32_t nowTicks = SDL_GetTicks();
  bool speaking = (rawGap > 34);
  if (speaking) {
    if (!s_wasSpeaking) {
      if (nowTicks - s_lastSpeechMs > 220u) s_speakerSide = 1 - s_speakerSide;
      s_turnStartMs = nowTicks;
    } else if (nowTicks - s_turnStartMs > 2600u) {
      s_speakerSide = 1 - s_speakerSide;
      s_turnStartMs = nowTicks;
    }
    s_lastSpeechMs = nowTicks;
  }
  s_wasSpeaking = speaking;

  bool isSingleLeftProf  = (!dualMode && (faceId % 2 == 1));
  bool isSingleRightProf = (!dualMode && (faceId % 2 == 0));
  bool isDualProfile     = dualMode;

  float totalSplit = static_cast<float>(g_profile_face_split_x.load(std::memory_order_relaxed) +
                                        g_profile_mouth_split_x.load(std::memory_order_relaxed));
  float puckerProtrude = std::max(0.0f, (128.0f - static_cast<float>(rawWidth)) / 128.0f) * 18.0f;
  float baseSpan = isDualProfile ? (46.0f + widthNorm * 22.0f) : (56.0f + widthNorm * 28.0f);
  float profSpan = (baseSpan + puckerProtrude) * scaleMult;
  float profY    = 572.0f + static_cast<float>(GLOBAL_FACE_Y_OFFSET);

  float gapL = (isDualProfile && s_speakerSide != 0) ? (gapHeight * 0.08f) : gapHeight;
  float gapR = (isDualProfile && s_speakerSide != 1) ? (gapHeight * 0.08f) : gapHeight;
  float tL   = (isDualProfile && s_speakerSide != 0) ? 0.0f : (showTongue ? tongueOut : 0.0f);
  float tR   = (isDualProfile && s_speakerSide != 1) ? 0.0f : (showTongue ? tongueOut : 0.0f);

  float tipL = 282.0f - totalSplit;
  float tipR = 318.0f + totalSplit;

  if (isSingleLeftProf) {
    renderSingleSideProfileMouth(tipL - profSpan, tipL, profY, gapL, smileBend, fullness, scaleMult, teethMode, tL, pal);
  } else if (isSingleRightProf) {
    renderSingleSideProfileMouth(tipR + profSpan, tipR, profY, gapR, smileBend, fullness, scaleMult, teethMode, tR, pal);
  } else {
    renderSingleSideProfileMouth(tipL - profSpan, tipL, profY, gapL, smileBend, fullness, scaleMult, teethMode, tL, pal);
    renderSingleSideProfileMouth(tipR + profSpan, tipR, profY, gapR, smileBend, fullness, scaleMult, teethMode, tR, pal);
  }
}

// ------------------------------------------------------------------------------
// 22. Render procedural 3D front or alternating-dialogue side-profile mouth
// ------------------------------------------------------------------------------
void drawMouth(uint8_t rawGap, uint8_t rawCurve, uint8_t rawWidth, uint8_t rawTeeth, uint8_t rawTongue, uint8_t rawThickness, bool hideMouth)
{
  tft.fillScreen(TFT_BLACK);
  std::memset(tft.argb_buffer, 0, sizeof(tft.argb_buffer));
  tft.customArgbReady = true;

  int ovLip = g_override_lip_color.load(std::memory_order_relaxed);
  if (ovLip >= 0) setLipColorPreset(static_cast<uint8_t>(ovLip));
  else currentLipColor = LIP_COLOR_CLASSIC_RED;

  static float s_smoothMouthEnv = 0.0f;
  if (isAudioLipSyncOverrideActive()) {
    float rawEnv = getAudioEnvelope();
    s_smoothMouthEnv += 0.35f * (rawEnv - s_smoothMouthEnv);
    float boostedEnv = std::min(1.0f, std::sqrt(std::max(0.0f, s_smoothMouthEnv)) * 1.15f);
    rawGap    = static_cast<uint8_t>(std::min(255, static_cast<int>(std::round(boostedEnv * 255.0f))));
    rawWidth  = static_cast<uint8_t>(std::max(0, std::min(255, static_cast<int>(rawWidth) - static_cast<int>(std::round(boostedEnv * 28.0f)))));
    rawTongue = 0;
  } else s_smoothMouthEnv = 0.0f;
  g_effective_mouth_gap.store(rawGap, std::memory_order_relaxed);
  if (hideMouth || rawThickness >= 255) return;

  float scaleMult = g_mouth_scale_factor.load(std::memory_order_relaxed);
  float fullness  = std::max(0.0f, std::min(1.0f, static_cast<float>(rawThickness) / 254.0f));
  float widthNorm = static_cast<float>(rawWidth) / 255.0f;
  float halfWidth = (48.0f + widthNorm * 117.0f) * scaleMult;
  float gapHeight = static_cast<float>(map(rawGap, 0, 255, 0, 64)) * scaleMult;
  float smileBend = static_cast<float>(map(rawCurve, 0, 255, -22, 34)) * scaleMult;
  int toothOverride = g_override_teeth.load(std::memory_order_relaxed);
  ToothPresetMode teethMode = static_cast<ToothPresetMode>(std::min<uint8_t>(15, (toothOverride >= 0) ? static_cast<uint8_t>(toothOverride) : static_cast<uint8_t>(rawTeeth / 16)));
  const LipColorPalette& pal = kLipPalettes[static_cast<int>(currentLipColor)];
  bool isLipLickMode = (rawTongue >= 161), lickUpperLip = (rawTongue >= 161 && rawTongue <= 208);
  float lickSweepNorm = isLipLickMode ? (lickUpperLip ? ((static_cast<float>(rawTongue - 161) / 47.0f) * 2.0f - 1.0f) : ((static_cast<float>(rawTongue - 209) / 46.0f) * 2.0f - 1.0f)) : 0.0f;
  if (isLipLickMode) gapHeight = std::max(gapHeight, 12.0f * scaleMult);
  float tongueOut = isLipLickMode ? 0.0f : (static_cast<float>(rawTongue) / 160.0f);
  float fineCurve = tongueOut * (0.35f + 0.65f * tongueOut);
  bool showTongue = (!isLipLickMode && rawTongue >= 2 && gapHeight > 3.5f);

  uint8_t faceId = g_active_face_mask.load(std::memory_order_relaxed);
  if (faceId >= PROFILE_FACE_START_INDEX) {
    renderSideProfileMouths(faceId, g_dual_profile_active.load(std::memory_order_relaxed), gapHeight, smileBend, widthNorm, fullness, scaleMult, rawWidth, rawGap, teethMode, showTongue, tongueOut, pal);
    return;
  }

  float upperScale = (0.28f + fullness * 1.28f) * scaleMult, lowerScale = (0.30f + fullness * 1.45f) * scaleMult, cupidAmount = 0.08f + fullness * fullness * 1.35f;
  float tongueW = fminf(64.0f * scaleMult, halfWidth * 0.56f), tongueLen = fineCurve * (78.0f * scaleMult + gapHeight * 0.95f);
  float cx = static_cast<float>(BASE_CANVAS_W / 2), cy = 618.0f + static_cast<float>(GLOBAL_FACE_Y_OFFSET);
  int32_t xMin = std::max(0, static_cast<int32_t>(cx - halfWidth - 18.0f)), xMax = std::min(BASE_CANVAS_W - 1, static_cast<int32_t>(cx + halfWidth + 18.0f));
  for (int32_t x = xMin; x <= xMax; ++x) {
    MouthColumnState col{}; col.normX = (static_cast<float>(x) - cx) / halfWidth; col.absX = fabsf(col.normX); if (col.absX > 1.06f) continue;
    col.arch = fmaxf(0.0f, 1.0f - col.normX * col.normX);
    float taper = std::pow(col.arch, 0.76f), cupidMod = 1.0f + cupidAmount * (0.42f * std::exp(-std::pow((col.absX - 0.22f) / 0.15f, 2.0f)) - 0.34f * std::exp(-std::pow(col.absX / 0.095f, 2.0f)));
    float baseLineY = cy - col.arch * smileBend;
    col.upperInnerY = baseLineY - col.arch * gapHeight * 0.36f; col.lowerInnerY = baseLineY + col.arch * gapHeight * 0.78f;
    col.upperOuterY = col.upperInnerY - 14.5f * taper * upperScale * cupidMod; col.lowerOuterY = col.lowerInnerY + 17.5f * taper * lowerScale;
    col.cornerAlpha = fmaxf(0.0f, fminf(1.0f, (1.05f - col.absX) * 5.5f)); col.invUpperH = 1.0f / fmaxf(2.4f, col.upperInnerY - col.upperOuterY); col.invLowerH = 1.0f / fmaxf(2.4f, col.lowerOuterY - col.lowerInnerY);
    col.invCavH = (col.lowerInnerY > col.upperInnerY) ? (1.0f / (col.lowerInnerY - col.upperInnerY)) : 0.0f;
    int32_t yMin = std::max(0, static_cast<int32_t>(col.upperOuterY - 5.0f)), yMax = std::min(BASE_CANVAS_H - 1, static_cast<int32_t>(col.lowerOuterY + 6.0f));
    for (int32_t y = yMin; y <= yMax; ++y) {
      float fy = static_cast<float>(y) + 0.5f; bool inCavity = (fy > col.upperInnerY && fy < col.lowerInnerY && gapHeight > 2.8f); float cavPos = inCavity ? ((fy - col.upperInnerY) * col.invCavH) : 0.0f;
      if (inCavity) { float r = 0, g = 0, b = 0; shadeMouthCavity(col, cavPos, gapHeight, showTongue || isLipLickMode, teethMode, r, g, b); if (gapHeight > 7.5f && cavPos <= 0.5f) evalToothPixel(teethMode, col.normX, col.absX, cavPos, r, g, b); blendPixelArgb(x, y, r, g, b, 1.0f); }
      if (fy >= col.upperOuterY - 3.5f && fy <= col.upperInnerY + 2.0f) { float r = 0, g = 0, b = 0, a = 0; if (shadeUpperLip(col, fy, pal, fullness, r, g, b, a)) blendPixelArgb(x, y, r, g, b, a); }
      if (fy >= col.lowerInnerY - 2.0f && fy <= col.lowerOuterY + 4.5f) { float r = 0, g = 0, b = 0, a = 0; if (shadeLowerLip(col, fy, pal, fullness, r, g, b, a)) blendPixelArgb(x, y, r, g, b, a); }
    }
  }
  float tongueRootY = (cy - smileBend) + gapHeight * 0.06f;
  if (isLipLickMode) renderLipLickingTongue(cx, cy, tongueRootY, tongueW, halfWidth, smileBend, gapHeight, lickSweepNorm, lickUpperLip);
  else if (showTongue) renderRollingStonesTongue(cx, tongueRootY, tongueW, tongueLen, tongueOut);
}

// ------------------------------------------------------------------------------
// 23. Read DMX face/mouth slots and update lip palette and 3D mouth rendering
// ------------------------------------------------------------------------------
void updateFaceViaDmx(bool hideMouth)
{
  setLipColorPreset(dmx_read_slot(dmxPort, getWrappedDmxIndex(start_address, DMX_OFFSET_LIP_COLOR) + 1));
  drawMouth(dmx_read_slot(dmxPort, getWrappedDmxIndex(start_address, DMX_OFFSET_MOUTH_GAP) + 1), dmx_read_slot(dmxPort, getWrappedDmxIndex(start_address, DMX_OFFSET_MOUTH_CURVE) + 1), dmx_read_slot(dmxPort, getWrappedDmxIndex(start_address, DMX_OFFSET_MOUTH_WIDTH) + 1), dmx_read_slot(dmxPort, getWrappedDmxIndex(start_address, DMX_OFFSET_MOUTH_TEETH) + 1), dmx_read_slot(dmxPort, getWrappedDmxIndex(start_address, DMX_OFFSET_MOUTH_TONGUE) + 1), dmx_read_slot(dmxPort, getWrappedDmxIndex(start_address, DMX_OFFSET_MOUTH_THICKNESS) + 1), hideMouth);
}
