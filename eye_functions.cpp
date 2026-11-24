// ==============================================================================
// MakingFaces - Dedicated Dual-Eye Graphics Pipeline (Raspberry Pi / Linux Port)
// Core Engine Adapted by Magnus Herrlin 2026
// Multi-Eye Namespace & Dynamic PNG/JPG Eye Bank (graphics/ + assets/eyes/)
// ==============================================================================

// ==============================================================================
// 1. INCLUDES, CONSTANTS AND EXTERNAL REFERENCES
// ==============================================================================
#include "main.h"
#include "eye_functions.h"
#include <SDL2/SDL_image.h>
#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <filesystem>
#include <map>

namespace fs = std::filesystem;

extern TFT_eSPI   eyeTft;
extern uint16_t   pbuffer[BUFFER_SIZE];
extern dmx_port_t dmxPort;
extern int        start_address;

dmx_port_t dmxPort = 1;

constexpr uint16_t EYE_CANVAS_SIZE     = 210;
constexpr uint16_t EYE_MIN_SCALE_SIZE  = 75;
constexpr uint16_t EYE_NORM_SCALE_SIZE = 152;
constexpr uint16_t EYE_MAX_SCALE_SIZE  = 208;
constexpr uint8_t  EYE_POPOUT_RAW_MIN  = 151;
constexpr uint8_t  EYE_CHANNELS_COUNT  = 6;

// ==============================================================================
// 2. ISOLATED EYE ASSET NAMESPACES, TABLE BINDINGS & DYNAMIC BANK
// ==============================================================================
enum EyeStyleType {
  EYE_STYLE_DEFAULT = 0,
  EYE_STYLE_DRAGON  = 1
};

enum PupilShapeType {
  PUPIL_ROUND      = 0,
  PUPIL_SLIT_VERT  = 1,
  PUPIL_SLIT_HORIZ = 2
};

struct EyeTableSet {
  const uint16_t *sclera;
  const uint16_t *iris;
  const uint16_t *polar;
  const uint8_t  *upperSly;
  const uint8_t  *lowerSly;
  uint16_t        scleraWidth;
  uint16_t        scleraHeight;
  uint16_t        irisWidth;
  uint16_t        irisHeight;
  uint16_t        irisMapWidth;
  uint16_t        irisMapHeight;
  uint16_t        screenWidth;
  uint16_t        screenHeight;
};

struct DynamicEyeEntry {
  std::string           label;
  PupilShapeType        pupilShape = PUPIL_ROUND;
  std::vector<uint16_t> scleraBuf;
  std::vector<uint16_t> irisBuf;
  std::vector<uint16_t> polarBuf;
  EyeTableSet           tables{};
};

namespace EyeDefaultSly {
  #undef SYMMETRICAL_EYELID
  #include "eyes/defaultEye.h"
}

static const EyeTableSet kDefaultEyeSet = {
  EyeDefaultSly::sclera,
  EyeDefaultSly::iris,
  EyeDefaultSly::polar,
  EyeDefaultSly::upper,
  EyeDefaultSly::lower,
  SCLERA_WIDTH,
  SCLERA_HEIGHT,
  IRIS_WIDTH,
  IRIS_HEIGHT,
  IRIS_MAP_WIDTH,
  IRIS_MAP_HEIGHT,
  SCREEN_WIDTH,
  SCREEN_HEIGHT
};

#undef SCLERA_WIDTH
#undef SCLERA_HEIGHT
#undef IRIS_WIDTH
#undef IRIS_HEIGHT
#undef IRIS_MAP_WIDTH
#undef IRIS_MAP_HEIGHT
#undef SCREEN_WIDTH
#undef SCREEN_HEIGHT
namespace EyeDragonSly {
  #undef SYMMETRICAL_EYELID
  #include "eyes/dragonEye.h"
}

static const EyeTableSet kDragonEyeSet = {
  EyeDragonSly::sclera,
  EyeDragonSly::iris,
  EyeDragonSly::polar,
  EyeDragonSly::upper,
  EyeDragonSly::lower,
  SCLERA_WIDTH,
  SCLERA_HEIGHT,
  IRIS_WIDTH,
  IRIS_HEIGHT,
  IRIS_MAP_WIDTH,
  IRIS_MAP_HEIGHT,
  SCREEN_WIDTH,
  SCREEN_HEIGHT
};

#undef SCLERA_WIDTH
#undef SCLERA_HEIGHT
#undef IRIS_WIDTH
#undef IRIS_HEIGHT
#undef IRIS_MAP_WIDTH
#undef IRIS_MAP_HEIGHT
#undef SCREEN_WIDTH
#undef SCREEN_HEIGHT

// ==============================================================================
// 3. ACTIVE EYE STATE, DYNAMIC EYE BANK & EYELID COLOR PALETTE
// ==============================================================================
static std::vector<DynamicEyeEntry>      g_dynamic_eye_bank;
static int                               g_selected_bank_idx = -1;
static std::array<const EyeTableSet*, 2> activeEyeStyle      = { &kDefaultEyeSet, &kDefaultEyeSet };
static std::array<EyeStyleType, 2>       currentStyle        = { EYE_STYLE_DEFAULT, EYE_STYLE_DEFAULT };
static std::array<EyeLidShapeType, 2>    lidShape            = { LID_SHAPE_SLY, LID_SHAPE_SLY };
static std::array<bool, 2>               eyeLidlessMode      = { false, false };
static std::array<bool, 2>               eyePoppedOut        = { false, false };
static std::array<float, 2>              eyePopFactor        = { 0.0f, 0.0f };
static EyelashStyleType                  currentLash         = LASH_STYLE_NONE;
static uint8_t                           currentLashLevel    = 0;

static bool    g_lid_face_active = true;
static uint8_t g_lid_skin_r      = 212;
static uint8_t g_lid_skin_g      = 164;
static uint8_t g_lid_skin_b      = 138;
static bool    g_lid_has_mk      = false;
static uint8_t g_lid_mk_r        = 0;
static uint8_t g_lid_mk_g        = 0;
static uint8_t g_lid_mk_b        = 0;
// ==============================================================================
// 4. ADAFRUIT-COMPATIBLE PNG/JPG TO RGB565 & POLAR TABLE GENERATOR
// ==============================================================================

// ------------------------------------------------------------------------------
// 5. Convert lowercase string to uppercase ASCII label
// ------------------------------------------------------------------------------
static std::string toUpperAscii(const std::string& in)
{
  std::string out = in;
  for (char& ch : out)
  {
    ch = static_cast<char>(std::toupper(static_cast<unsigned char>(ch)));
  }
  return out;
}

// ------------------------------------------------------------------------------
// 6. Load an image file and resample it into a target-sized RGB565 vector
// ------------------------------------------------------------------------------
static bool loadResampledRgb565(const fs::path& imgPath, int dstW, int dstH, std::vector<uint16_t>& outBuf)
{
  SDL_Surface* raw = IMG_Load(imgPath.string().c_str());
  if (!raw)
  {
    return false;
  }

  SDL_Surface* conv = SDL_ConvertSurfaceFormat(raw, SDL_PIXELFORMAT_ARGB8888, 0);
  SDL_FreeSurface(raw);
  if (!conv)
  {
    return false;
  }

  outBuf.resize(static_cast<size_t>(dstW * dstH));
  const uint32_t* px = static_cast<const uint32_t*>(conv->pixels);
  int pitchPx = conv->pitch / 4;
  bool isSclera = (imgPath.filename().string().find("sclera") != std::string::npos);

  for (int y = 0; y < dstH; ++y)
  {
    int sy = std::min(conv->h - 1, (y * conv->h) / dstH);
    for (int x = 0; x < dstW; ++x)
    {
      int sx = std::min(conv->w - 1, (x * conv->w) / dstW);
      uint32_t c = px[sy * pitchPx + sx];
      uint8_t a = static_cast<uint8_t>((c >> 24) & 0xFF);
      uint8_t r = static_cast<uint8_t>((c >> 16) & 0xFF);
      uint8_t g = static_cast<uint8_t>((c >> 8) & 0xFF);
      uint8_t b = static_cast<uint8_t>(c & 0xFF);

      if (isSclera && (a < 200 || (r < 65 && g < 65 && b < 65)))
      {
        float cx = conv->w * 0.5f;
        float cy = conv->h * 0.5f;
        float vx = static_cast<float>(sx) - cx;
        float vy = static_cast<float>(sy) - cy;
        for (int step = 1; step <= 40; ++step)
        {
          float t = 0.72f * (1.0f - (static_cast<float>(step) / 42.0f));
          int nx = std::max(0, std::min(conv->w - 1, static_cast<int>(cx + vx * t)));
          int ny = std::max(0, std::min(conv->h - 1, static_cast<int>(cy + vy * t)));
          uint32_t nc = px[ny * pitchPx + nx];
          uint8_t na = static_cast<uint8_t>((nc >> 24) & 0xFF);
          uint8_t nr = static_cast<uint8_t>((nc >> 16) & 0xFF);
          uint8_t ng = static_cast<uint8_t>((nc >> 8) & 0xFF);
          uint8_t nb = static_cast<uint8_t>(nc & 0xFF);
          if (na >= 200 && (nr >= 65 || ng >= 65 || nb >= 65))
          {
            r = nr;
            g = ng;
            b = nb;
            break;
          }
        }
      }

      uint16_t rgb565 = static_cast<uint16_t>(((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3));
      outBuf[static_cast<size_t>(y * dstW + x)] = static_cast<uint16_t>(rgb565 | 0x0021);
    }
  }

  SDL_FreeSurface(conv);
  return true;
}
// ------------------------------------------------------------------------------
// 7. Compute Adafruit-compatible 16-bit polar lookup table (morphing cat/goat)
// ------------------------------------------------------------------------------
static void buildPolarLookupTable(int irisSize, PupilShapeType shape, std::vector<uint16_t>& outPolar)
{
  outPolar.resize(static_cast<size_t>(irisSize * irisSize));
  float half = static_cast<float>(irisSize - 1) * 0.5f;
  float maxR = static_cast<float>(irisSize) * 0.5f;
  constexpr float kPi = 3.14159265f;

  for (int y = 0; y < irisSize; ++y)
  {
    float dy = (static_cast<float>(y) - half) / maxR;
    for (int x = 0; x < irisSize; ++x)
    {
      float dx = (static_cast<float>(x) - half) / maxR;
      float r = std::sqrt(dx * dx + dy * dy);
      if (r >= 1.0f)
      {
        outPolar[static_cast<size_t>(y * irisSize + x)] = 127;
        continue;
      }

      if (shape == PUPIL_SLIT_VERT || shape == PUPIL_SLIT_HORIZ)
      {
        float slitMetric = (shape == PUPIL_SLIT_VERT)
                           ? std::min(1.0f, std::sqrt((dx * dx * 32.0f) + (dy * dy * 1.15f)))
                           : std::min(1.0f, std::sqrt((dx * dx * 1.30f) + (dy * dy * 20.0f)));
        uint16_t roundBits = static_cast<uint16_t>(std::min(126.0f, std::round((1.0f - r) * 126.0f))) & 0x7F;
        uint16_t slitBits  = static_cast<uint16_t>(std::min(126.0f, std::round((1.0f - slitMetric) * 126.0f))) & 0x7F;
        outPolar[static_cast<size_t>(y * irisSize + x)] = static_cast<uint16_t>(0x8000u | (roundBits << 7) | slitBits);
      }
      else
      {
        float invDist = 0.08f + (1.0f - r) * 0.92f;
        float ang = std::atan2(dy, dx);
        if (ang < 0.0f)
        {
          ang += 2.0f * kPi;
        }
        uint16_t angleBits = static_cast<uint16_t>(std::round((ang / (2.0f * kPi)) * 511.0f)) & 0x1FF;
        uint16_t distBits  = static_cast<uint16_t>(std::min(126.0f, std::round(invDist * 126.0f))) & 0x7F;
        outPolar[static_cast<size_t>(y * irisSize + x)] = static_cast<uint16_t>((angleBits << 7) | distBits);
      }
    }
  }
}
// ------------------------------------------------------------------------------
// 8. Build and register a single dynamic eye entry from sclera and iris files
// ------------------------------------------------------------------------------
static void registerDynamicEyeCandidate(const std::string& rawKey, const fs::path& scl, const fs::path& iri)
{
  DynamicEyeEntry item{};
  item.label = toUpperAscii(rawKey);

  if (item.label.find("GOAT") != std::string::npos)
  {
    item.pupilShape = PUPIL_SLIT_HORIZ;
  }
  else if (item.label.find("DRAGON") != std::string::npos ||
           item.label.find("CAT") != std::string::npos ||
           item.label.find("NEWT") != std::string::npos ||
           item.label.find("SNAKE") != std::string::npos)
  {
    item.pupilShape = PUPIL_SLIT_VERT;
  }

  if (!loadResampledRgb565(scl, 200, 200, item.scleraBuf))
  {
    return;
  }
  if (!loadResampledRgb565(iri, 256, 64, item.irisBuf))
  {
    return;
  }

  if (item.label.find("CAT") != std::string::npos)
  {
    for (int y = 0; y < 200; ++y)
    {
      float dy = (y - 99.5f) / 100.0f;
      for (int x = 0; x < 200; ++x)
      {
        float dx = (x - 99.5f) / 100.0f;
        float r = std::min(1.0f, std::sqrt(dx * dx + dy * dy));
        float ang = std::atan2(dy, dx);
        float streak = 0.93f + 0.07f * std::sin(ang * 44.0f + r * 7.0f);
        float shade = (1.02f - r * 0.16f) * streak;
        uint8_t cr = static_cast<uint8_t>(std::min(255.0f, 255.0f * shade));
        uint8_t cg = static_cast<uint8_t>(std::min(255.0f, 224.0f * shade));
        uint8_t cb = static_cast<uint8_t>(std::min(255.0f, 52.0f * shade));
        uint16_t c565 = static_cast<uint16_t>(((cr & 0xF8) << 8) | ((cg & 0xFC) << 3) | (cb >> 3)) | 0x0021;
        item.scleraBuf[static_cast<size_t>(y * 200 + x)] = c565;
      }
    }
  }

  buildPolarLookupTable(80, item.pupilShape, item.polarBuf);

  item.tables = {
    item.scleraBuf.data(),
    item.irisBuf.data(),
    item.polarBuf.data(),
    kDefaultEyeSet.upperSly,
    kDefaultEyeSet.lowerSly,
    200, 200, 80, 80, 256, 64, 128, 128
  };
  g_dynamic_eye_bank.push_back(std::move(item));
}
// ------------------------------------------------------------------------------
// 9. Scan graphics/ and assets/eyes/ (including subdirs) for sclera + iris pairs
// ------------------------------------------------------------------------------
static void scanDirectoryForEyePairs(const fs::path& dir)
{
  if (!fs::exists(dir) || !fs::is_directory(dir))
  {
    return;
  }

  std::map<std::string, fs::path> scleras;
  std::map<std::string, fs::path> irises;

  for (const auto& entry : fs::directory_iterator(dir))
  {
    if (entry.is_directory())
    {
      fs::path subScl = entry.path() / "sclera.png";
      fs::path subIri = entry.path() / "iris.png";
      if (fs::exists(subScl) && fs::exists(subIri))
      {
        registerDynamicEyeCandidate(entry.path().filename().string(), subScl, subIri);
      }
      continue;
    }

    if (!entry.is_regular_file())
    {
      continue;
    }

    std::string lowExt = entry.path().extension().string();
    for (char& c : lowExt)
    {
      c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    if (lowExt != ".png" && lowExt != ".jpg" && lowExt != ".jpeg")
    {
      continue;
    }

    std::string stem = entry.path().stem().string();
    if (stem == "sclera")
    {
      scleras["default"] = entry.path();
    }
    else if (stem == "iris")
    {
      irises["default"] = entry.path();
    }
    else if (stem.size() > 7 && stem.substr(stem.size() - 7) == "-sclera")
    {
      scleras[stem.substr(0, stem.size() - 7)] = entry.path();
    }
    else if (stem.size() > 7 && stem.substr(stem.size() - 7) == "_sclera")
    {
      scleras[stem.substr(0, stem.size() - 7)] = entry.path();
    }
    else if (stem.size() > 5 && stem.substr(stem.size() - 5) == "-iris")
    {
      irises[stem.substr(0, stem.size() - 5)] = entry.path();
    }
    else if (stem.size() > 5 && stem.substr(stem.size() - 5) == "_iris")
    {
      irises[stem.substr(0, stem.size() - 5)] = entry.path();
    }
  }

  for (const auto& kv : scleras)
  {
    if (irises.count(kv.first))
    {
      registerDynamicEyeCandidate(kv.first, kv.second, irises[kv.first]);
    }
  }
}
// ------------------------------------------------------------------------------
// 10. Rebuild dynamic eye bank from built-in presets, graphics/ and assets/eyes/
// ------------------------------------------------------------------------------
void reloadDynamicEyeBank(const std::string& assetsDir)
{
  g_dynamic_eye_bank.clear();

  DynamicEyeEntry bDef{};
  bDef.label      = "BUILTIN DEFAULT";
  bDef.pupilShape = PUPIL_ROUND;
  bDef.tables     = kDefaultEyeSet;
  g_dynamic_eye_bank.push_back(std::move(bDef));

  DynamicEyeEntry bDrg{};
  bDrg.label      = "BUILTIN DRAGON";
  bDrg.pupilShape = PUPIL_SLIT_VERT;
  bDrg.tables     = kDragonEyeSet;
  g_dynamic_eye_bank.push_back(std::move(bDrg));

  scanDirectoryForEyePairs("graphics");
  scanDirectoryForEyePairs(fs::path(assetsDir) / "eyes");

  if (g_selected_bank_idx >= static_cast<int>(g_dynamic_eye_bank.size()))
  {
    g_selected_bank_idx = -1;
  }
}

// ------------------------------------------------------------------------------
// 11. Return total number of available eyes in the dynamic eye bank
// ------------------------------------------------------------------------------
int getEyeBankCount(void)
{
  return static_cast<int>(g_dynamic_eye_bank.size());
}

// ------------------------------------------------------------------------------
// 12. Return display name for a given dynamic eye bank index
// ------------------------------------------------------------------------------
std::string getEyeBankName(int idx)
{
  if (idx < 0 || idx >= static_cast<int>(g_dynamic_eye_bank.size()))
  {
    return "AUTO";
  }
  return g_dynamic_eye_bank.at(idx).label;
}
// ------------------------------------------------------------------------------
// 13. Activate a specific eye from the dynamic eye bank (-1 restores auto mode)
// ------------------------------------------------------------------------------
void selectEyeBankIndex(int idx)
{
  if (idx < 0 || idx >= static_cast<int>(g_dynamic_eye_bank.size()))
  {
    g_selected_bank_idx = -1;
    return;
  }
  g_selected_bank_idx = idx;
  activeEyeStyle.at(0) = &g_dynamic_eye_bank.at(idx).tables;
  activeEyeStyle.at(1) = &g_dynamic_eye_bank.at(idx).tables;
}

// ------------------------------------------------------------------------------
// 14. Return currently selected eye bank index (-1 when in auto mode)
// ------------------------------------------------------------------------------
int getSelectedEyeBankIndex(void)
{
  return g_selected_bank_idx;
}

// ==============================================================================
// 15. PIXEL SAMPLING, 3D EYELIDS, MOVING EYELASHES AND BUFFER HELPERS
// ==============================================================================

// ------------------------------------------------------------------------------
// 16. Wrap a 1-based DMX slot offset safely within 1..512 universe bounds
// ------------------------------------------------------------------------------
static int getWrappedSlot(int baseAddress, int offset)
{
  int targetSlot = (baseAddress + offset) % 512;
  return (targetSlot == 0) ? 512 : targetSlot;
}

// ------------------------------------------------------------------------------
// 17. Push accumulated RGB565 pixels to the virtual TFT framebuffer
// ------------------------------------------------------------------------------
static inline void flushPixelBuffer(uint32_t &pixels, bool forceFlush)
{
  if (pixels >= BUFFER_SIZE || (forceFlush && pixels > 0))
  {
    yield();
    eyeTft.pushPixels(pbuffer, pixels);
    pixels = 0;
  }
}
// ------------------------------------------------------------------------------
// 18. Pack clamped floating-point RGB channels into non-zero RGB565
// ------------------------------------------------------------------------------
static inline uint16_t packRgb565NonZero(float r, float g, float b)
{
  uint8_t ir = static_cast<uint8_t>(std::max(0.0f, std::min(255.0f, r)));
  uint8_t ig = static_cast<uint8_t>(std::max(0.0f, std::min(255.0f, g)));
  uint8_t ib = static_cast<uint8_t>(std::max(0.0f, std::min(255.0f, b)));
  uint16_t c = static_cast<uint16_t>(((ir & 0xF8) << 8) | ((ig & 0xFC) << 3) | (ib >> 3));
  return static_cast<uint16_t>(c | 0x0021);
}

// ------------------------------------------------------------------------------
// 19. Apply 3D spherical lighting and specular highlight for popped-out eyes
// ------------------------------------------------------------------------------
static inline uint16_t applyPopOutSphereShading(
  uint16_t texCol,
  uint32_t screenX,
  uint32_t screenY,
  uint16_t screenW,
  uint16_t screenH,
  float popAmt)
{
  if (popAmt <= 0.01f)
  {
    return static_cast<uint16_t>(texCol | 0x0021);
  }

  float nx = (static_cast<float>(screenX) / static_cast<float>(screenW - 1)) * 2.0f - 1.0f;
  float ny = (static_cast<float>(screenY) / static_cast<float>(screenH - 1)) * 2.0f - 1.0f;
  float r2 = std::min(1.0f, (nx * nx) + (ny * ny));

  float lx = nx + 0.32f;
  float ly = ny + 0.35f;
  float lightDist2  = std::min(1.8f, (lx * lx) + (ly * ly));
  float sphereShade = 1.16f - (lightDist2 * 0.28f) - (r2 * r2 * 0.34f * popAmt);
  float specular    = std::exp(-lightDist2 * 6.5f) * 42.0f * popAmt;

  float r = static_cast<float>(((texCol >> 11) & 0x1F) * 255 / 31) * sphereShade + specular;
  float g = static_cast<float>(((texCol >> 5)  & 0x3F) * 255 / 63) * sphereShade + specular;
  float b = static_cast<float>(( texCol        & 0x1F) * 255 / 31) * sphereShade + specular;

  return packRgb565NonZero(r, g, b);
}
// ------------------------------------------------------------------------------
// 20. Evaluate upper or lower eyelid opening threshold for Sly, Round & Almond
// ------------------------------------------------------------------------------
static inline float evalLidOpenValue(
  const EyeTableSet *curEye,
  uint8_t e,
  uint32_t screenX,
  uint32_t screenY,
  bool isLower)
{
  const uint16_t screenW = curEye->screenWidth;
  const uint16_t screenH = curEye->screenHeight;
  EyeLidShapeType shape  = lidShape[e];

  if (shape == LID_SHAPE_ROUND || shape == LID_SHAPE_ALMOND)
  {
    float ryFactor = (shape == LID_SHAPE_ROUND) ? 0.49f : 0.34f;
    float halfW    = static_cast<float>(screenW - 1) * 0.5f;
    float halfH    = static_cast<float>(screenH - 1) * 0.5f;
    float nx       = (static_cast<float>(screenX) - halfW) / (static_cast<float>(screenW) * 0.49f);
    float ny       = (static_cast<float>(screenY) - halfH) / (static_cast<float>(screenH) * ryFactor);
    if ((nx * nx) + (ny * ny) > 1.0f)
    {
      return -1.0f;
    }

    float arch = std::sqrt(std::max(0.0f, 1.0f - (nx * nx)));
    if (isLower)
    {
      return 254.0f * (1.0f - (ny > 0.0f ? (ny / std::max(0.05f, arch)) : 0.0f));
    }
    return 254.0f * (1.0f - (ny < 0.0f ? (-ny / std::max(0.05f, arch)) : 0.0f));
  }

  uint16_t lidX      = (e == 0) ? (screenW - 1 - screenX) : screenX;
  uint32_t lidOffset = screenY * screenW + lidX;
  uint8_t  uVal      = pgm_read_byte(curEye->upperSly + lidOffset);
  uint8_t  lVal      = pgm_read_byte(curEye->lowerSly + lidOffset);
  if (uVal == 0 || lVal == 0)
  {
    return -1.0f;
  }

  return isLower ? static_cast<float>(lVal) : static_cast<float>(uVal);
}
// ------------------------------------------------------------------------------
// 21. Find vertical Y coordinate of the upper eyelid rim for eyelash attachment
// ------------------------------------------------------------------------------
static inline float findUpperLidMarginY(
  const EyeTableSet *curEye,
  uint8_t e,
  uint32_t screenX,
  uint32_t uT)
{
  const uint16_t screenH = curEye->screenHeight;
  int topY     = -1;
  int botY     = -1;
  int seamY    = -1;
  int firstOpY = -1;
  bool sawClosedUpper = false;

  for (uint16_t y = 0; y < screenH; ++y)
  {
    float uVal = evalLidOpenValue(curEye, e, screenX, y, false);
    float lVal = evalLidOpenValue(curEye, e, screenX, y, true);
    if (uVal < 0.0f || lVal < 0.0f)
    {
      continue;
    }

    if (topY < 0)
    {
      topY = y;
    }
    botY = y;

    if (uVal <= static_cast<float>(uT))
    {
      sawClosedUpper = true;
    }
    else if (sawClosedUpper && firstOpY < 0 && lVal > static_cast<float>(uT))
    {
      firstOpY = y;
    }

    if (seamY < 0 && lVal <= uVal)
    {
      seamY = y;
    }
  }

  if (topY < 0)
  {
    return -1000.0f;
  }
  if (firstOpY >= 0)
  {
    return static_cast<float>(firstOpY);
  }
  if (seamY >= 0)
  {
    return static_cast<float>(seamY);
  }
  return static_cast<float>(botY);
}
// ------------------------------------------------------------------------------
// 22. Shade 3D skin- and makeup-matched upper or lower eyelid surface
// ------------------------------------------------------------------------------
static inline uint16_t shadeEyelidSurface(
  uint32_t screenX,
  uint32_t screenY,
  uint16_t screenW,
  uint16_t screenH,
  float lidOpenVal,
  float threshold,
  bool isUpper)
{
  if (threshold <= 0.5f)
  {
    return 0x0000;
  }
  if (!g_lid_face_active && !g_lid_has_mk)
  {
    return 0x0000;
  }

  float baseR = static_cast<float>(g_lid_skin_r);
  float baseG = static_cast<float>(g_lid_skin_g);
  float baseB = static_cast<float>(g_lid_skin_b);

  if (g_lid_has_mk)
  {
    float mkMix = isUpper ? 1.00f : 0.85f;
    baseR = baseR * (1.0f - mkMix) + static_cast<float>(g_lid_mk_r) * mkMix;
    baseG = baseG * (1.0f - mkMix) + static_cast<float>(g_lid_mk_g) * mkMix;
    baseB = baseB * (1.0f - mkMix) + static_cast<float>(g_lid_mk_b) * mkMix;
  }

  float nx = (static_cast<float>(screenX) / static_cast<float>(screenW - 1)) * 2.0f - 1.0f;
  float ny = (static_cast<float>(screenY) / static_cast<float>(screenH - 1)) * 2.0f - 1.0f;
  float radial    = std::min(1.0f, (nx * nx * 0.65f) + (ny * ny * 0.65f));
  float domeShade = 1.08f - (radial * 0.36f);

  float distFromRim = std::max(0.0f, threshold - lidOpenVal);
  float rimDarken   = 1.0f;
  if (distFromRim < 10.0f)
  {
    rimDarken = 0.45f + (distFromRim / 10.0f) * 0.55f;
  }
  else if (isUpper && distFromRim > 42.0f && distFromRim < 56.0f)
  {
    float creaseWave = 1.0f - std::fabs(distFromRim - 49.0f) / 7.0f;
    rimDarken = 1.0f - (creaseWave * 0.24f);
  }

  float totalShade = domeShade * rimDarken;
  return packRgb565NonZero(baseR * totalShade, baseG * totalShade, baseB * totalShade);
}
// ------------------------------------------------------------------------------
// 23. Sample eyeliner and tapered eyelash hairs along the upper eyelid margin
// ------------------------------------------------------------------------------
static inline uint16_t sampleEyelashPixel(
  uint8_t e,
  uint32_t screenX,
  uint32_t screenY,
  uint16_t screenW,
  float lidEdgeY,
  uint32_t rawBlink)
{
  bool lidsVisible = g_lid_face_active || g_lid_has_mk;
  if (currentLash == LASH_STYLE_NONE || currentLashLevel == 0 || !lidsVisible || rawBlink == 0 || lidEdgeY < -500.0f)
  {
    return 0x0000;
  }

  float normX = (static_cast<float>(screenX) / static_cast<float>(screenW - 1)) * 2.0f - 1.0f;
  if (e == 0)
  {
    normX = -normX;
  }

  float dyFromLid = static_cast<float>(screenY) - lidEdgeY;
  if (dyFromLid < -3.5f)
  {
    return 0x0000;
  }

  float t             = static_cast<float>(currentLashLevel - 1) / 254.0f;
  float eyelinerAbove = 0.8f + (t * 2.0f);
  float eyelinerBelow = 1.4f + (t * 3.8f);
  float baseReach     = 6.0f + (t * 30.0f);
  float lashFreq      = 14.0f - (t * 6.0f);
  float lashDuty      = 0.42f + (t * 0.26f);
  float peakShift     = 0.0f;
  float curveSpread   = 0.55f;

  if (currentLash == LASH_STYLE_WINGED)
  {
    peakShift   = 0.22f;
    curveSpread = 0.38f;
    if (normX > 0.08f)
    {
      float wingBoost = (normX - 0.08f) * (2.0f + t * 8.0f);
      eyelinerAbove += wingBoost * 0.7f;
      eyelinerBelow += wingBoost;
    }
  }
  else if (currentLash == LASH_STYLE_LONG || currentLash == LASH_STYLE_DRAMATIC)
  {
    peakShift   = 0.14f;
    curveSpread = 0.36f;
  }

  float dxShape   = normX - peakShift;
  float lashReach = baseReach * std::max(0.0f, 1.0f - (dxShape * dxShape * curveSpread));

  if (dyFromLid >= -eyelinerAbove && dyFromLid <= eyelinerBelow && std::fabs(normX) < 0.97f)
  {
    return 0x0861;
  }

  if (dyFromLid > 0.0f && dyFromLid <= lashReach && normX > -0.86f && normX < 0.99f)
  {
    float lashProgress = dyFromLid / std::max(1.0f, lashReach);
    float flareX       = normX - (lashProgress * (0.14f + t * 0.12f));
    float phase        = (flareX + 1.0f) * lashFreq;
    float frac         = phase - std::floor(phase);
    float taperWidth   = lashDuty * (1.0f - (lashProgress * 0.68f));

    if (std::fabs(frac - 0.5f) < (taperWidth * 0.5f))
    {
      return (lashProgress > 0.76f) ? 0x2965 : 0x0861;
    }
  }

  return 0x0000;
}
// ------------------------------------------------------------------------------
// 24. Sample a single composited eye pixel (lids, lashes, iris, pupil, sclera)
// ------------------------------------------------------------------------------
static inline uint16_t sampleEyePixel(
  const EyeTableSet *curEye,
  uint8_t e,
  uint32_t iScale,
  uint32_t curScleraX,
  uint32_t curScleraY,
  int32_t irisX,
  int32_t irisY,
  uint32_t screenX,
  uint32_t screenY,
  float lidEdgeY,
  uint32_t uT,
  uint32_t lT)
{
  const uint16_t screenW = curEye->screenWidth;
  const uint16_t screenH = curEye->screenHeight;
  const uint16_t scleraW = curEye->scleraWidth;
  const uint16_t irisW   = curEye->irisWidth;
  const uint16_t irisH   = curEye->irisHeight;

  float lowerOpen = evalLidOpenValue(curEye, e, screenX, screenY, true);
  float upperOpen = evalLidOpenValue(curEye, e, screenX, screenY, false);

  if (lowerOpen < 0.0f || upperOpen < 0.0f)
  {
    return 0x0000;
  }

  uint16_t lashCol = sampleEyelashPixel(e, screenX, screenY, screenW, lidEdgeY, uT);
  if (lashCol != 0x0000)
  {
    return lashCol;
  }

  if (uT > 0 && upperOpen <= static_cast<float>(uT))
  {
    return shadeEyelidSurface(screenX, screenY, screenW, screenH, upperOpen, static_cast<float>(uT), true);
  }
  if (lT > 0 && lowerOpen <= static_cast<float>(lT))
  {
    return shadeEyelidSurface(screenX, screenY, screenW, screenH, lowerOpen, static_cast<float>(lT), false);
  }

  bool isDynamicPng = (g_selected_bank_idx >= 2 && g_selected_bank_idx < static_cast<int>(g_dynamic_eye_bank.size()));
  bool isMorphSlit  = (isDynamicPng && g_dynamic_eye_bank.at(g_selected_bank_idx).pupilShape != PUPIL_ROUND);
  bool seamlessCat  = (isDynamicPng && g_dynamic_eye_bank.at(g_selected_bank_idx).label.find("CAT") != std::string::npos);

  uint16_t texCol = pgm_read_word(curEye->sclera + curScleraY * scleraW + curScleraX);
  if (irisY >= 0 && irisY < irisH && irisX >= 0 && irisX < irisW)
  {
    uint32_t polarVal = pgm_read_word(curEye->polar + irisY * irisW + irisX);
    uint32_t distRaw  = polarVal & 0x7F;
    if (distRaw != 127)
    {
      if (isMorphSlit && (polarVal & 0x8000u))
      {
        float openT      = std::max(0.0f, std::min(1.0f, (static_cast<float>(iScale) - 85.0f) / 160.0f));
        float slitInv    = static_cast<float>(polarVal & 0x7F);
        float roundInv   = static_cast<float>((polarVal >> 7) & 0x7F);
        float morphedInv = slitInv * (1.0f - openT) + roundInv * openT;
        float thresh     = 98.0f - openT * 62.0f;

        if (morphedInv >= thresh)
        {
          texCol = 0x0021;
        }
        else if (!seamlessCat)
        {
          uint32_t d = std::min<uint32_t>(curEye->irisMapHeight - 1, static_cast<uint32_t>(morphedInv * 0.6f));
          texCol = pgm_read_word(curEye->iris + d * curEye->irisMapWidth);
        }
      }
      else
      {
        uint32_t d = (iScale * distRaw) / 128;
        if (d < curEye->irisMapHeight)
        {
          uint32_t a = (curEye->irisMapWidth * (polarVal >> 7)) / 512;
          texCol = pgm_read_word(curEye->iris + d * curEye->irisMapWidth + a);
        }
        else
        {
          texCol = 0x0021;
        }
      }
    }
  }

  return applyPopOutSphereShading(texCol, screenX, screenY, screenW, screenH, eyePopFactor[e]);
}
// ==============================================================================
// 25. INITIALIZATION, PREVIEW BUFFER, STYLE SELECTION & RENDERING PIPELINE
// ==============================================================================

// ------------------------------------------------------------------------------
// 26. Initialize dual-eye screen coordinates, blink state and eyelid defaults
// ------------------------------------------------------------------------------
void initEyes(void)
{
  eye.at(0).xposition   = 85;
  eye.at(1).xposition   = 305;
  eye.at(0).blink.state = NOBLINK;
  eye.at(1).blink.state = NOBLINK;

  activeEyeStyle.fill(&kDefaultEyeSet);
  currentStyle.fill(EYE_STYLE_DEFAULT);
  lidShape.fill(LID_SHAPE_SLY);
  eyeLidlessMode.fill(false);
  eyePoppedOut.fill(false);
  eyePopFactor.fill(0.0f);
  currentLash       = LASH_STYLE_NONE;
  currentLashLevel  = 0;
  g_lid_face_active = true;
}

// ------------------------------------------------------------------------------
// 27. Render a 210x210 circular ARGB8888 thumbnail for the F8 Eye Browser
// ------------------------------------------------------------------------------
void renderEyePreviewBuffer(int bankIdx, uint32_t* outArgb210x210)
{
  bool isDynamicBank     = (bankIdx >= 0 && bankIdx < static_cast<int>(g_dynamic_eye_bank.size()));
  bool isDynamicPng      = (bankIdx >= 2 && bankIdx < static_cast<int>(g_dynamic_eye_bank.size()));
  bool isMorphSlit       = (isDynamicPng && g_dynamic_eye_bank.at(bankIdx).pupilShape != PUPIL_ROUND);
  const EyeTableSet* curEye = isDynamicBank ? &g_dynamic_eye_bank.at(bankIdx).tables : &kDefaultEyeSet;
  bool seamlessCat       = (isDynamicPng && g_dynamic_eye_bank.at(bankIdx).label.find("CAT") != std::string::npos);

  uint32_t scleraX = (curEye->scleraWidth > curEye->screenWidth) ? (curEye->scleraWidth - curEye->screenWidth) / 2 : 0;
  uint32_t scleraY = (curEye->scleraHeight > curEye->screenHeight) ? (curEye->scleraHeight - curEye->screenHeight) / 2 : 0;
  uint32_t stepFP  = (static_cast<uint32_t>(curEye->screenWidth) << 16) / EYE_CANVAS_SIZE;

  for (uint16_t oy = 0; oy < EYE_CANVAS_SIZE; ++oy)
  {
    uint32_t sy    = std::min<uint32_t>(curEye->screenHeight - 1, (oy * stepFP) >> 16);
    int32_t  irisY = static_cast<int32_t>(scleraY + sy) - (curEye->scleraHeight - curEye->irisHeight) / 2;

    for (uint16_t ox = 0; ox < EYE_CANVAS_SIZE; ++ox)
    {
      uint32_t sx = std::min<uint32_t>(curEye->screenWidth - 1, (ox * stepFP) >> 16);
      float nx    = (static_cast<float>(ox) / 104.5f) - 1.0f;
      float ny    = (static_cast<float>(oy) / 104.5f) - 1.0f;
      if ((nx * nx + ny * ny) > 0.96f)
      {
        outArgb210x210[oy * EYE_CANVAS_SIZE + ox] = 0x00000000;
        continue;
      }

      int32_t  irisX = static_cast<int32_t>(scleraX + sx) - (curEye->scleraWidth - curEye->irisWidth) / 2;
      uint16_t c     = curEye->sclera[(scleraY + sy) * curEye->scleraWidth + (scleraX + sx)];

      if (irisY >= 0 && irisY < curEye->irisHeight && irisX >= 0 && irisX < curEye->irisWidth)
      {
        uint32_t pVal    = curEye->polar[irisY * curEye->irisWidth + irisX];
        uint32_t distRaw = pVal & 0x7F;
        if (distRaw != 127)
        {
          if (isMorphSlit && (pVal & 0x8000u))
          {
            float slitInv = static_cast<float>(pVal & 0x7F);
            if (slitInv >= 94.0f)
            {
              c = 0x0021;
            }
            else if (!seamlessCat)
            {
              uint32_t d = std::min<uint32_t>(curEye->irisMapHeight - 1, static_cast<uint32_t>(slitInv * 0.6f));
              c = curEye->iris[d * curEye->irisMapWidth];
            }
          }
          else
          {
            uint32_t d = (160 * distRaw) / 128;
            if (d < curEye->irisMapHeight)
            {
              uint32_t a = (curEye->irisMapWidth * (pVal >> 7)) / 512;
              c = curEye->iris[d * curEye->irisMapWidth + a];
            }
            else
            {
              c = 0x0021;
            }
          }
        }
      }

      uint8_t r = static_cast<uint8_t>(((c >> 11) & 0x1F) * 255 / 31);
      uint8_t g = static_cast<uint8_t>(((c >> 5)  & 0x3F) * 255 / 63);
      uint8_t b = static_cast<uint8_t>(( c        & 0x1F) * 255 / 31);
      outArgb210x210[oy * EYE_CANVAS_SIZE + ox] = 0xFF000000u | (static_cast<uint32_t>(r) << 16) | (static_cast<uint32_t>(g) << 8) | b;
    }
  }
}
// ------------------------------------------------------------------------------
// 28. Query pop-out state, pop factor and update eyelid colors or shapes
// ------------------------------------------------------------------------------
bool isEyePoppedOut(uint8_t e)
{
  return (e <= 1) ? eyePoppedOut[e] : false;
}

float getEyePopOutFactor(uint8_t e)
{
  return (e <= 1) ? eyePopFactor[e] : 0.0f;
}

void setEyelidFaceActive(bool active)
{
  g_lid_face_active = active;
}

void setEyelidSkinColor(uint8_t r, uint8_t g, uint8_t b)
{
  g_lid_skin_r = r;
  g_lid_skin_g = g;
  g_lid_skin_b = b;
}

void setEyelidMakeupColor(bool enabled, uint8_t r, uint8_t g, uint8_t b)
{
  g_lid_has_mk = enabled;
  g_lid_mk_r   = r;
  g_lid_mk_g   = g;
  g_lid_mk_b   = b;
}

void setEyeLidShapeForIndex(uint8_t e, EyeLidShapeType shape)
{
  if (e <= 1)
  {
    lidShape[e] = shape;
  }
}

void setEyeLidShape(EyeLidShapeType shape)
{
  lidShape.fill(shape);
}

void setRoundEyelidsForIndex(uint8_t e, bool roundMode)
{
  if (g_override_eye_style.load(std::memory_order_relaxed) < 0 && e <= 1)
  {
    lidShape[e] = roundMode ? LID_SHAPE_ROUND : LID_SHAPE_SLY;
  }
}

void setRoundEyelids(bool roundMode)
{
  if (g_override_eye_style.load(std::memory_order_relaxed) < 0)
  {
    lidShape.fill(roundMode ? LID_SHAPE_ROUND : LID_SHAPE_SLY);
  }
}
// ------------------------------------------------------------------------------
// 29. Select active eyelash style and density from DMX channel or override
// ------------------------------------------------------------------------------
void setEyelashStyle(uint8_t dmxLashVal)
{
  int overrideVal      = g_override_eyelash.load(std::memory_order_relaxed);
  uint8_t effectiveVal = dmxLashVal;
  if (overrideVal >= 0)
  {
    int clamped  = std::min(15, overrideVal);
    effectiveVal = (clamped == 0) ? 0 : static_cast<uint8_t>(1 + ((clamped - 1) * 254) / 14);
  }

  currentLashLevel = effectiveVal;
  if (effectiveVal == 0)
  {
    currentLash = LASH_STYLE_NONE;
  }
  else if (effectiveVal < 64)
  {
    currentLash = LASH_STYLE_NATURAL;
  }
  else if (effectiveVal < 128)
  {
    currentLash = LASH_STYLE_LONG;
  }
  else if (effectiveVal < 192)
  {
    currentLash = LASH_STYLE_WINGED;
  }
  else
  {
    currentLash = LASH_STYLE_DRAMATIC;
  }
}

// ------------------------------------------------------------------------------
// 30. Apply 12-mode eye shape & style for a single eye index (0 or 1)
// ------------------------------------------------------------------------------
void setEyeStyleForIndex(uint8_t e, uint8_t dmxStyleVal)
{
  int ov = g_override_eye_style.load(std::memory_order_relaxed);
  if (ov >= 0)
  {
    dmxStyleVal = static_cast<uint8_t>(ov);
  }
  if (e > 1)
  {
    return;
  }

  bool isLidless        = (dmxStyleVal >= 128);
  uint8_t halfVal       = isLidless ? (dmxStyleVal - 128) : dmxStyleVal;
  EyeStyleType newStyle = (halfVal >= 64) ? EYE_STYLE_DRAGON : EYE_STYLE_DEFAULT;
  uint8_t subVal        = (halfVal >= 64) ? (halfVal - 64) : halfVal;

  EyeLidShapeType newShape = LID_SHAPE_SLY;
  if (subVal >= 43)
  {
    newShape = LID_SHAPE_ALMOND;
  }
  else if (subVal >= 22)
  {
    newShape = LID_SHAPE_ROUND;
  }

  eyeLidlessMode[e] = isLidless;
  lidShape[e]       = newShape;

  if (g_selected_bank_idx >= 0 && g_selected_bank_idx < static_cast<int>(g_dynamic_eye_bank.size()))
  {
    activeEyeStyle[e] = &g_dynamic_eye_bank.at(g_selected_bank_idx).tables;
    return;
  }

  if (newStyle != currentStyle[e])
  {
    currentStyle[e]   = newStyle;
    activeEyeStyle[e] = (newStyle == EYE_STYLE_DRAGON) ? &kDragonEyeSet : &kDefaultEyeSet;
  }
}

// ------------------------------------------------------------------------------
// 31. Apply 12-mode eye shape & style across both left and right eyes
// ------------------------------------------------------------------------------
void setEyeStyle(uint8_t dmxStyleVal)
{
  setEyeStyleForIndex(0, dmxStyleVal);
  setEyeStyleForIndex(1, dmxStyleVal);
}
// ------------------------------------------------------------------------------
// 32. Rasterize and scale one 210x210 eye frame into the virtual TFT buffer
// ------------------------------------------------------------------------------
void drawEye(uint8_t e, uint32_t iScale, uint32_t scleraX, uint32_t scleraY, uint32_t uT, uint32_t lT, uint16_t targetSize)
{
  const EyeTableSet *curEye  = activeEyeStyle[e];
  const uint16_t     screenW = curEye->screenWidth;
  const uint16_t     screenH = curEye->screenHeight;
  const uint16_t     scleraW = curEye->scleraWidth;
  const uint16_t     scleraH = curEye->scleraHeight;
  const uint16_t     irisW   = curEye->irisWidth;
  const uint16_t     irisH   = curEye->irisHeight;

  std::array<float, 256> colLidEdgeY{};
  for (uint16_t sx = 0; sx < screenW && sx < 256; ++sx)
  {
    colLidEdgeY.at(sx) = findUpperLidMarginY(curEye, e, sx, uT);
  }

  uint16_t clampedSize = constrain(targetSize, EYE_MIN_SCALE_SIZE, EYE_MAX_SCALE_SIZE);
  uint16_t pad         = (EYE_CANVAS_SIZE - clampedSize) / 2;
  uint16_t endXY       = pad + clampedSize;
  uint32_t stepFP      = (static_cast<uint32_t>(screenW) << 16) / clampedSize;
  uint32_t pixels      = 0;
  uint32_t srcY_FP     = 0;

  eyeTft.startWrite();
  eyeTft.setAddrWindow(eye[e].xposition, 250 + GLOBAL_FACE_Y_OFFSET, EYE_CANVAS_SIZE, EYE_CANVAS_SIZE);

  for (uint16_t outY = 0; outY < EYE_CANVAS_SIZE; outY++)
  {
    if (outY < pad || outY >= endXY)
    {
      for (uint16_t outX = 0; outX < EYE_CANVAS_SIZE; outX++)
      {
        pbuffer[pixels++] = 0;
        flushPixelBuffer(pixels, false);
      }
      continue;
    }

    uint32_t screenY    = std::min<uint32_t>(screenH - 1, srcY_FP >> 16);
    srcY_FP            += stepFP;
    uint32_t curScleraY = scleraY + screenY;
    uint32_t srcX_FP    = 0;
    int32_t  irisY      = static_cast<int32_t>(curScleraY) - (scleraH - irisH) / 2;

    for (uint16_t outX = 0; outX < EYE_CANVAS_SIZE; outX++)
    {
      uint16_t p = 0;
      if (outX >= pad && outX < endXY)
      {
        uint32_t screenX = std::min<uint32_t>(screenW - 1, srcX_FP >> 16);
        srcX_FP         += stepFP;
        float lidEdgeY   = (screenX < 256) ? colLidEdgeY.at(screenX) : -1000.0f;
        p = sampleEyePixel(curEye, e, iScale, scleraX + screenX, curScleraY,
                           static_cast<int32_t>(scleraX + screenX) - (scleraW - irisW) / 2,
                           irisY, screenX, screenY, lidEdgeY, uT, lT);
      }
      pbuffer[pixels++] = (p >> 8) | (p << 8);
      flushPixelBuffer(pixels, false);
    }
  }

  flushPixelBuffer(pixels, true);
  eyeTft.endWrite();
}

// ------------------------------------------------------------------------------
// 33. Normalize gaze, iris dilation, blink & pop-out parameters for one eye
// ------------------------------------------------------------------------------
void renderSingleEyeNormalized(uint8_t e, uint8_t rawX, uint8_t rawY, uint8_t rawIris, uint8_t rawBlink, uint8_t rawSize)
{
  if (e > 1)
  {
    return;
  }
  if (g_override_eye_style.load(std::memory_order_relaxed) >= 0)
  {
    setEyeStyleForIndex(e, 0);
  }
  if (g_override_eyelash.load(std::memory_order_relaxed) >= 0)
  {
    setEyelashStyle(0);
  }

  int irisOv = g_override_iris.load(std::memory_order_relaxed);
  if (irisOv >= 0)
  {
    rawIris = static_cast<uint8_t>(std::min(255, irisOv));
  }

  if (g_critter_gaze_active.load(std::memory_order_relaxed))
  {
    rawX = g_critter_gaze_x.load(std::memory_order_relaxed);
    rawY = g_critter_gaze_y.load(std::memory_order_relaxed);
  }

  bool popped     = (rawSize >= EYE_POPOUT_RAW_MIN);
  eyePoppedOut[e] = popped;
  eyePopFactor[e] = popped ? (static_cast<float>(rawSize - EYE_POPOUT_RAW_MIN) / static_cast<float>(255 - EYE_POPOUT_RAW_MIN)) : 0.0f;

  uint16_t baseSize = (!popped)
                      ? static_cast<uint16_t>(map(rawSize, 0, EYE_POPOUT_RAW_MIN - 1, EYE_MIN_SCALE_SIZE, EYE_NORM_SCALE_SIZE))
                      : static_cast<uint16_t>(map(rawSize, EYE_POPOUT_RAW_MIN, 255, EYE_NORM_SCALE_SIZE + 8, EYE_MAX_SCALE_SIZE));
  float scaleMult     = g_eye_scale_factor.load(std::memory_order_relaxed);
  uint16_t targetSize = static_cast<uint16_t>(std::round(static_cast<float>(baseSize) * scaleMult));

  uint16_t localIris    = map(rawIris, 0, 255, 75, 255);
  uint8_t  lidThreshold = eyeLidlessMode[e] ? 0 : static_cast<uint8_t>(map(rawBlink, 0, 255, 0, 254));

  int32_t dmxXRelative = (static_cast<int32_t>(rawX) - 128) * 4;
  int32_t dmxYRelative = (static_cast<int32_t>(rawY) - 128) * 4;
  const EyeTableSet *curEye = activeEyeStyle[e];
  int32_t maxLimitX = std::max(0, static_cast<int32_t>(curEye->scleraWidth) - static_cast<int32_t>(curEye->screenWidth));
  int32_t maxLimitY = std::max(0, static_cast<int32_t>(curEye->scleraHeight) - static_cast<int32_t>(curEye->screenHeight));
  int32_t centerX   = maxLimitX / 2;
  int32_t centerY   = maxLimitY / 2;
  int32_t localEyeX = constrain(centerX + map(dmxXRelative, -512, 512, centerX, -centerX), 0, maxLimitX);
  int32_t localEyeY = constrain(centerY + map(dmxYRelative, -512, 512, -centerY, centerY), 0, maxLimitY);

  drawEye(e, localIris, static_cast<uint32_t>(localEyeX), static_cast<uint32_t>(localEyeY), lidThreshold, lidThreshold, targetSize);
}

// ------------------------------------------------------------------------------
// 34. Render both left and right eyes synchronously with identical parameters
// ------------------------------------------------------------------------------
void renderEyePair(uint8_t rawX, uint8_t rawY, uint8_t rawIris, uint8_t rawBlink, uint8_t rawSize)
{
  renderSingleEyeNormalized(0, rawX, rawY, rawIris, rawBlink, rawSize);
  renderSingleEyeNormalized(1, rawX, rawY, rawIris, rawBlink, rawSize);
}

// ------------------------------------------------------------------------------
// 35. Read live DMX channels 1..12 and 30 to render both eyes in real time
// ------------------------------------------------------------------------------
void updateEyeViaDmx(void)
{
  setEyelashStyle(dmx_read_slot(dmxPort, getWrappedSlot(start_address, DMX_OFFSET_EYELASH_STYLE)));
  for (uint8_t e = 0; e < 2; e++)
  {
    int eyeBase = start_address + (e * EYE_CHANNELS_COUNT);
    setEyeStyleForIndex(e, dmx_read_slot(dmxPort, getWrappedSlot(eyeBase, 4)));
    renderSingleEyeNormalized(
      e,
      dmx_read_slot(dmxPort, getWrappedSlot(eyeBase, 0)),
      dmx_read_slot(dmxPort, getWrappedSlot(eyeBase, 1)),
      dmx_read_slot(dmxPort, getWrappedSlot(eyeBase, 2)),
      dmx_read_slot(dmxPort, getWrappedSlot(eyeBase, 3)),
      dmx_read_slot(dmxPort, getWrappedSlot(eyeBase, 5)));
  }
}
