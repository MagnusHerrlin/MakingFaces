// ==============================================================================
// MakingFaces - Wayland/SDL2 Animatronic Face Compositor & Art-Net/sACN Engine
// Architecture designed by Magnus Herrlin 2026
// ==============================================================================

#ifndef MAIN_H
#define MAIN_H

#include <SDL2/SDL.h>
#include <array>
#include <atomic>
#include <cstdint>
#include <string>
#include <map>
#include <vector>

constexpr int      BASE_CANVAS_W            = 600;
constexpr int      BASE_CANVAS_H            = 800;
constexpr int      GLOBAL_FACE_Y_OFFSET     = -50;
constexpr int      PROFILE_FACE_START_INDEX = 127;
constexpr int      JAW_LAYER_START_INDEX    = 97;
constexpr int      ARTNET_PORT              = 6454;
constexpr int      SACN_PORT                = 5568;
constexpr uint32_t DMX_TIMEOUT_MS           = 2000;
constexpr int      EDITABLE_ITEM_COUNT      = 14;
constexpr int      MAX_MEMORY_SLOT          = 240;
constexpr int      MAX_MEMORY_BANK          = 24;
constexpr int      MAX_UNDO_LEVELS          = 5;

constexpr uint32_t FX_BIT_FOG_BACK          = (1u << 9);
constexpr uint32_t FX_BIT_STARS             = (1u << 15);
constexpr uint32_t FX_BIT_COMET             = (1u << 16);
constexpr uint32_t FX_BIT_AURORA            = (1u << 17);
constexpr uint32_t FX_BIT_RAIN              = (1u << 18);
constexpr uint32_t FX_BIT_SNOW              = (1u << 19);
constexpr uint32_t FX_BIT_FOG_MID           = (1u << 20);
constexpr uint32_t FX_BIT_FOG_FRONT         = (1u << 21);

enum DmxChannelOffset {
    DMX_OFFSET_EYE_LEFT_X      = 0,
    DMX_OFFSET_EYE_LEFT_Y      = 1,
    DMX_OFFSET_EYE_LEFT_IRIS   = 2,
    DMX_OFFSET_EYE_LEFT_BLINK  = 3,
    DMX_OFFSET_EYE_LEFT_STYLE  = 4,
    DMX_OFFSET_EYE_LEFT_SIZE   = 5,
    DMX_OFFSET_EYE_RIGHT_X     = 6,
    DMX_OFFSET_EYE_RIGHT_Y     = 7,
    DMX_OFFSET_EYE_RIGHT_IRIS  = 8,
    DMX_OFFSET_EYE_RIGHT_BLINK = 9,
    DMX_OFFSET_EYE_RIGHT_STYLE = 10,
    DMX_OFFSET_EYE_RIGHT_SIZE  = 11,
    DMX_OFFSET_EYE_CONVERGENCE = 12,
    DMX_OFFSET_BROW_TILT       = 13,
    DMX_OFFSET_BROW_MASK       = 14,
    DMX_OFFSET_MOUTH_GAP       = 15,
    DMX_OFFSET_MOUTH_SMILE     = 16,
    DMX_OFFSET_MOUTH_CURVE     = 16,
    DMX_OFFSET_MOUTH_WIDTH     = 17,
    DMX_OFFSET_MOUTH_TEETH     = 18,
    DMX_OFFSET_MOUTH_TONGUE    = 19,
    DMX_OFFSET_MOUTH_THICK     = 20,
    DMX_OFFSET_MOUTH_THICKNESS = 20,
    DMX_OFFSET_LIP_COLOR       = 21,
    DMX_OFFSET_FACE_MASK       = 22,
    DMX_OFFSET_EARS_MASK       = 23,
    DMX_OFFSET_NOSE_MASK       = 24,
    DMX_OFFSET_BEARD_MASK      = 25,
    DMX_OFFSET_GLASSES_MASK    = 26,
    DMX_OFFSET_HAIR_MASK       = 27,
    DMX_OFFSET_HAT_MASK        = 28,
    DMX_OFFSET_MAKEUP_MASK     = 29,
    DMX_OFFSET_EYELASH         = 30,
    DMX_OFFSET_EYELASH_STYLE   = 30,
    DMX_OFFSET_MEMORY          = 31,
    DMX_OFFSET_MACRO           = 31,
    DMX_OFFSET_MEMORY_CTRL     = 32,
    DMX_OFFSET_MACRO_CTRL      = 32,
    DMX_OFFSET_BG_SCENE        = 33,
    DMX_OFFSET_FX_OVERLAY      = 34,
    DMX_OFFSET_AUDIO_TRACK     = 35,
    DMX_OFFSET_AUDIO_VOLUME    = 36,
    DMX_OFFSET_AUDIO_TRIGGER   = 37,
    DMX_OFFSET_MASTER_DIMMER   = 38,
    DMX_CHANNELS_PER_FIXTURE   = 39
};
struct MaskOffsetProfile {
    int   eye_left_x    = 0;
    int   eye_left_y    = 0;
    int   eye_right_x   = 0;
    int   eye_right_y   = 0;
    int   mouth_x       = 0;
    int   mouth_y       = 0;
    int   mouth_split   = 0;
    int   nose_x        = 0;
    int   nose_y        = 0;
    int   nose_split    = 0;
    int   glasses_x     = 0;
    int   glasses_y     = 0;
    int   glasses_split = 0;
    int   brows_x       = 0;
    int   brows_y       = 0;
    int   brows_split   = 0;
    int   beard_x       = 0;
    int   beard_y       = 0;
    int   beard_split   = 0;
    int   face_x        = 0;
    int   face_y        = 0;
    int   face_split    = 0;
    int   ears_x        = 0;
    int   ears_y        = 0;
    int   ears_split    = 0;
    int   hair_x        = 0;
    int   hair_y        = 0;
    int   hat_x         = 0;
    int   hat_y         = 0;
    int   makeup_x      = 0;
    int   makeup_y      = 0;
    int   bg_x          = 0;
    int   bg_y          = 0;
    int   fx_x          = 0;
    int   fx_y          = 0;
    float eye_scale     = 1.0f;
    float mouth_scale   = 1.0f;
    float nose_scale    = 1.0f;
    float glasses_scale = 1.0f;
    float brows_scale   = 1.0f;
    float beard_scale   = 1.0f;
    float face_scale    = 1.0f;
    float ears_scale    = 1.0f;
    float hair_scale    = 1.0f;
    float hat_scale     = 1.0f;
    float makeup_scale  = 1.0f;
    float bg_scale      = 1.0f;
    float fx_scale      = 1.0f;
};

struct FxSpriteBank {
    std::array<SDL_Texture*, 17> sprites{};
    std::array<SDL_Texture*, 17> animSprites{};
    std::array<SDL_Texture*, 17> blurSprites{};
    SDL_Texture*                 bat3            = nullptr;
    SDL_Texture*                 fog2            = nullptr;
    SDL_Texture*                 leafTex         = nullptr;
    SDL_Texture*                 snowflakeTex    = nullptr;
    std::array<SDL_Texture*, 2>  fireflyTex{};
    std::array<SDL_Texture*, 2>  grasshopperTex{};
    std::array<SDL_Texture*, 2>  antTex{};
};

struct MemoryPreset {
    bool     enabled           = false;
    uint8_t  face              = 1;
    uint8_t  makeup            = 0;
    uint8_t  ears              = 0;
    uint8_t  nose              = 1;
    uint8_t  beard             = 0;
    uint8_t  glasses           = 0;
    uint8_t  hair              = 0;
    uint8_t  hat               = 0;
    uint8_t  brows             = 1;
    uint8_t  right_face        = 0;
    uint8_t  right_makeup      = 0;
    uint8_t  right_ears        = 0;
    uint8_t  right_nose        = 0;
    uint8_t  right_beard       = 0;
    uint8_t  right_glasses     = 0;
    uint8_t  right_hair        = 0;
    uint8_t  right_hat         = 0;
    uint8_t  right_brows       = 0;
    int      right_face_x      = 0;
    int      right_face_y      = 0;
    float    right_face_scale  = 1.0f;
    int      right_mouth_x     = 0;
    int      right_mouth_y     = 0;
    float    right_mouth_scale = 1.0f;
    uint8_t  bg_scene          = 0;
    uint32_t fx_mask           = 0;
    int      teeth             = 1;
    int      lip_color         = 3;
    int      eyelash           = 0;
    int      eye_style         = -1;
    int      iris              = -1;
    int      eye_bank          = -1;
    int      eye_bank_right    = -1;
    int      anim_id           = 0;
    bool     dual_profile      = false;
    bool     hide_mouth        = false;
};

struct FaceAppConfig {
    std::string proto         = "sacn";
    uint16_t    universe      = 1;
    uint16_t    start_channel = 1;
    int         dmx_mode      = 39;
    int         fps           = 50;
    int         display       = -1;
    bool        fullscreen    = false;
    bool        smooth_scale  = false;
    int         fade_ms       = 600;
    int         save_hold_ms  = 2000;
    std::string audio_dir     = "audio";
    std::string assets_dir    = "assets";
    std::string audio_monitor = "";
    std::string config_file   = "makingfaces.ini";

    std::map<uint8_t, MaskOffsetProfile> mask_profiles;
    std::map<int, MemoryPreset>          memory_combos;
};
struct PaperDollLayer {
    const char*  folder       = "";
    uint8_t      loadedIndex  = 0;
    uint8_t      testIndex    = 0;
    int          offsetX      = 0;
    int          offsetY      = 0;
    int          splitOffsetX = 0;
    float        scale        = 1.0f;
    SDL_Texture* texture      = nullptr;
    bool         isRightSide  = false;
};

struct ParallaxBackdrop {
    uint8_t      loadedIndex = 0;
    SDL_Texture* sky         = nullptr;
    SDL_Texture* clouds      = nullptr;
    SDL_Texture* farLayer    = nullptr;
    SDL_Texture* midLayer    = nullptr;
    SDL_Texture* frontLayer  = nullptr;
    SDL_Texture* single      = nullptr;
};

struct PackInspectItem {
    int          srcSlot  = 0;
    bool         selected = true;
    std::string  label    = "";
    MemoryPreset preset{};
};

struct PackBrowserEntry {
    std::string  name        = "";
    int          faceCount   = 0;
    int          memoryCount = 0;
    SDL_Texture* thumbTex    = nullptr;
};

struct PackBrowserState {
    bool                          open         = false;
    bool                          inspectMode  = false;
    int                           selectedIdx  = 0;
    int                           inspectIdx   = 0;
    bool                          wipeConfirm  = false;
    std::vector<PackBrowserEntry> entries;
    std::vector<PackInspectItem>  inspectItems;
};

struct EyeBrowserEntry {
    std::string  name     = "";
    int          bankIdx  = -1;
    SDL_Texture* thumbTex = nullptr;
};

struct EyeBrowserState {
    bool                         open        = false;
    int                          selectedIdx = 0;
    std::vector<EyeBrowserEntry> entries;
};

struct FlatImportItem {
    std::string  filePath     = "";
    std::string  fileName     = "";
    int          targetCatIdx = 0;
    bool         selected     = true;
    bool         removeWhite  = true;
    bool         cutEyes      = true;
    bool         doInpaint    = true;
    int          offsetX      = 0;
    int          offsetY      = 0;
    float        scale        = 1.0f;
    float        angle        = 0.0f;
    int          leftEyeX     = 190;
    int          leftEyeY     = 316;
    int          leftEyeRx    = 64;
    int          leftEyeRy    = 45;
    int          rightEyeX    = 410;
    int          rightEyeY    = 316;
    int          rightEyeRx   = 64;
    int          rightEyeRy   = 45;
    int          noseX        = 300;
    int          noseY        = 440;
    int          noseRx       = 54;
    int          noseRy       = 78;
    int          mouthX       = 300;
    int          mouthY       = 565;
    int          mouthRx      = 94;
    int          mouthRy      = 52;
    SDL_Texture* previewTex   = nullptr;
};

struct FlatImportState {
    bool                        open        = false;
    int                         selectedIdx = 0;
    int                         dragTarget  = 0;
    bool                        dragging    = false;
    int                         dragLastX   = 0;
    int                         dragLastY   = 0;
    std::vector<FlatImportItem> items;
};

struct CameraCaptureState {
    bool         open         = false;
    bool         usbAvailable = false;
    int          usbDeviceIdx = 0;
    bool         useCsiMode   = false;
    bool         eyeScopeMode = false;
    bool         rotate90     = false;
    int          texW         = 0;
    int          texH         = 0;
    SDL_Texture* liveTex      = nullptr;
};

struct AppRuntimeState {
    PaperDollLayer bgLayer{"bg"};
    PaperDollLayer faceLayer{"face"};
    PaperDollLayer makeupLayer{"makeup"};
    PaperDollLayer earsLayer{"ears"};
    PaperDollLayer browsLayer{"brows"};
    PaperDollLayer noseLayer{"nose"};
    PaperDollLayer beardLayer{"beard"};
    PaperDollLayer glassesLayer{"glasses"};
    PaperDollLayer hairLayer{"hair"};
    PaperDollLayer hatLayer{"hat"};
    PaperDollLayer fxLayer{"fx"};

    PaperDollLayer dualFaceLayer{"face", 0, 0, 0, 0, 0, 1.0f, nullptr, true};
    PaperDollLayer dualMakeupLayer{"makeup", 0, 0, 0, 0, 0, 1.0f, nullptr, true};
    PaperDollLayer dualEarsLayer{"ears", 0, 0, 0, 0, 0, 1.0f, nullptr, true};
    PaperDollLayer dualBrowsLayer{"brows", 0, 0, 0, 0, 0, 1.0f, nullptr, true};
    PaperDollLayer dualNoseLayer{"nose", 0, 0, 0, 0, 0, 1.0f, nullptr, true};
    PaperDollLayer dualBeardLayer{"beard", 0, 0, 0, 0, 0, 1.0f, nullptr, true};
    PaperDollLayer dualGlassesLayer{"glasses", 0, 0, 0, 0, 0, 1.0f, nullptr, true};
    PaperDollLayer dualHairLayer{"hair", 0, 0, 0, 0, 0, 1.0f, nullptr, true};
    PaperDollLayer dualHatLayer{"hat", 0, 0, 0, 0, 0, 1.0f, nullptr, true};

    ParallaxBackdrop   parallaxBg{};
    FxSpriteBank       fxBank{};
    PackBrowserState   packBrowser{};
    EyeBrowserState    eyeBrowser{};
    FlatImportState    flatImport{};
    CameraCaptureState cameraCapture{};

    bool     isDmxActive         = false;
    bool     wasDmxActive        = false;
    uint32_t dmxPktRate          = 0;
    bool     isFullscreen        = false;
    bool     osdEnabled          = false;
    bool     showHelpOverlay     = false;
    bool     showTelemetry       = false;
    bool     dualProfileMode     = false;
    bool     editingRightProfile = false;
    uint8_t  masterDimmer        = 0;
    bool     manualBlackout      = false;

    int      memoryBank          = 0;
    int      browseMemorySlot    = 0;
    int      manualMemorySlot    = -1;
    int      appliedMemorySlot   = -1;
    bool     dmxMemoryLocked     = false;
    int      manualTestAnim      = 0;

    int      fadePhase           = 0;
    uint32_t fadeStartMs         = 0;
    uint8_t  fadeAlpha           = 0;
    int      pendingMemorySlot   = -99;
    int      pendingApplySlot    = 0;

    uint32_t dmxSaveHoldStart    = 0;
    bool     dmxSaveTriggered    = false;

    uint8_t  activeProfileId     = 1;
    bool     isEditingOffsets    = false;
    int      selectedItemIdx     = 0;
    int      eyeLeftOffsetX      = 0;
    int      eyeLeftOffsetY      = 0;
    int      eyeRightOffsetX     = 0;
    int      eyeRightOffsetY     = 0;
    int      mouthOffsetX        = 0;
    int      mouthOffsetY        = 0;
    int      rightMouthOffsetX   = 0;
    int      rightMouthOffsetY   = 0;
    int      mouthSplitX         = 0;
    float    eyeScale            = 1.0f;
    float    mouthScale          = 1.0f;
    float    rightMouthScale     = 1.0f;

    bool     manualHideMouth     = false;
    bool     manualHideNose      = false;
    bool     manualHideBrows     = false;
    uint32_t manualFxMask        = 0;

    uint8_t  curFace             = 1;
    uint8_t  curMakeup           = 0;
    uint8_t  curEars             = 0;
    uint8_t  curNose             = 1;
    uint8_t  curBeard            = 0;
    uint8_t  curGlasses          = 0;
    uint8_t  curHair             = 0;
    uint8_t  curHat              = 0;
    uint8_t  curBrows            = 1;
    uint8_t  curBgScene          = 0;
    uint32_t curFxMask           = 0;
    int      curTeeth            = -1;
    int      curLipColor         = -1;
    int      curEyelash          = -1;
    int      curEyeStyle         = -1;
};
#include "tft_compat.h"

#ifndef DEBUG_PRINTF
#define DEBUG_PRINTF(...) do {} while (0)
#endif

using dmx_port_t = int;

constexpr int BUFFER_SIZE = 1024;
constexpr uint8_t NOBLINK = 0;
constexpr uint8_t ENBLINK = 1;
constexpr uint8_t DEBLINK = 2;

struct eyeBlink {
    uint8_t  state     = NOBLINK;
    uint32_t duration  = 0;
    uint32_t startTime = 0;
};

struct eye_t {
    TFT_eSprite* sprite        = nullptr;
    int16_t      xposition     = 0;
    uint8_t      wink          = 0;
    int8_t       winkCountdown = 0;
    eyeBlink     blink{};
};

extern TFT_eSPI tft;
extern TFT_eSPI eyeTft;
extern uint16_t pbuffer[BUFFER_SIZE];
extern std::array<eye_t, 2> eye;
extern int      start_address;

extern std::atomic<bool>        g_running;
extern std::atomic<uint8_t>     g_active_face_mask;
extern std::atomic<bool>        g_dual_profile_active;
extern std::atomic<int>         g_profile_face_split_x;
extern std::atomic<int>         g_profile_mouth_split_x;
extern std::atomic<int>         g_dual_mouth_mood_mode;
extern std::atomic<int>         g_override_teeth;
extern std::atomic<int>         g_override_eyelash;
extern std::atomic<int>         g_override_lip_color;
extern std::atomic<int>         g_override_teeth_right;
extern std::atomic<int>         g_override_lip_color_right;
extern std::atomic<int>         g_override_eye_style;
extern std::atomic<int>         g_override_iris;
extern std::atomic<float>       g_eye_scale_factor;
extern std::atomic<float>       g_mouth_scale_factor;
extern std::atomic<float>       g_mouth_scale_factor_right;
extern std::atomic<bool>        g_critter_gaze_active;
extern std::atomic<uint8_t>     g_critter_gaze_x;
extern std::atomic<uint8_t>     g_critter_gaze_y;
extern std::atomic<uint8_t>     g_critter_gaze_converge;
extern std::atomic<uint8_t>     g_brow_left_tilt;
extern std::atomic<uint8_t>     g_brow_right_tilt;
extern std::atomic<uint8_t>     g_effective_mouth_gap;
extern std::array<uint8_t, 512> g_dmx_universe;

static inline void setProgrammaticBrows(uint8_t leftVal, uint8_t rightVal)
{
    g_brow_left_tilt.store(leftVal, std::memory_order_relaxed);
    g_brow_right_tilt.store(rightVal, std::memory_order_relaxed);
}

static inline int getWrappedDmxIndex(uint16_t startCh, int offset)
{
    int idx = (static_cast<int>(startCh) - 1 + offset) % 512;
    return (idx < 0) ? (idx + 512) : idx;
}

static inline uint8_t dmx_read_slot(int, int ch1Based)
{
    if (ch1Based < 1 || ch1Based > 512) return 0;
    return g_dmx_universe.at(static_cast<size_t>(ch1Based - 1));
}

void destroyParallaxBackdrop(ParallaxBackdrop& bg);
void initFxSpriteBank(SDL_Renderer* ren, const std::string& assetsDir, FxSpriteBank& bank);
void destroyFxSpriteBank(FxSpriteBank& bank);

#endif
