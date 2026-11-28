// ==============================================================================
// Section A. Includes, Constants & Shared Global Instances
// ==============================================================================
#include "main.h"
#include "eye_functions.h"
#include "face_engine.h"
#include "animation_engine.h"
#include "audio_engine.h"
#include "stage_engine.h"
#include "stage_behaviors.h"
#include "stage_actors.h"

#include <SDL2/SDL_image.h>
#include <opencv2/opencv.hpp>
#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <thread>
#include <mutex>
#include <chrono>
#include <cstring>
#include <csignal>
#include <cstdlib>
#include <ctime>
#include <cmath>
#include <cctype>
#include <algorithm>
#include <vector>
#include <queue>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <unistd.h>
#include <getopt.h>

namespace fs = std::filesystem;

constexpr const char* APP_VERSION_STR = "v1.0.0";

std::atomic<bool>        g_running{true};
std::atomic<uint8_t>     g_active_face_mask{1};
std::atomic<bool>        g_dual_profile_active{false};
std::atomic<int>         g_profile_face_split_x{0};
std::atomic<int>         g_profile_mouth_split_x{0};
std::atomic<int>         g_dual_mouth_mood_mode{0};
std::atomic<int>         g_override_teeth{-1};
std::atomic<int>         g_override_eyelash{-1};
std::atomic<int>         g_override_lip_color{-1};
std::atomic<int>         g_override_teeth_right{-1};
std::atomic<int>         g_override_lip_color_right{-1};
std::atomic<int>         g_override_eye_style{-1};
std::atomic<int>         g_override_iris{-1};
std::atomic<float>       g_eye_scale_factor{1.0f};
std::atomic<float>       g_mouth_scale_factor{1.0f};
std::atomic<float>       g_mouth_scale_factor_right{1.0f};
std::atomic<bool>        g_critter_gaze_active{false};
std::atomic<uint8_t>     g_critter_gaze_x{128};
std::atomic<uint8_t>     g_critter_gaze_y{128};
std::atomic<uint8_t>     g_critter_gaze_converge{0};
std::atomic<uint8_t>     g_brow_left_tilt{128};
std::atomic<uint8_t>     g_brow_right_tilt{128};
std::atomic<uint8_t>     g_effective_mouth_gap{0};
std::array<uint8_t, 512> g_dmx_universe{};

TFT_eSPI tft;
TFT_eSPI eyeTft;

uint16_t pbuffer[BUFFER_SIZE];
std::array<eye_t, 2> eye;
int      start_address = 1;

static PerlinNoise       g_stage_noise(2026);
static Stage             g_stage;
static ParticleEmitter   g_fx_emitter(g_stage_noise, 100);
static ParticleEmitter   g_snow_mid_emitter(g_stage_noise, 205);
static ParticleEmitter   g_snow_front_emitter(g_stage_noise, 206);
static ParticleEmitter   g_rain_mid_emitter(g_stage_noise, 207);
static ParticleEmitter   g_rain_front_emitter(g_stage_noise, 208);
static std::array<ParticleEmitter, 17> g_critter_emitters = {{
    ParticleEmitter(g_stage_noise, 300), ParticleEmitter(g_stage_noise, 301),
    ParticleEmitter(g_stage_noise, 302), ParticleEmitter(g_stage_noise, 303),
    ParticleEmitter(g_stage_noise, 304), ParticleEmitter(g_stage_noise, 305),
    ParticleEmitter(g_stage_noise, 306), ParticleEmitter(g_stage_noise, 307),
    ParticleEmitter(g_stage_noise, 308), ParticleEmitter(g_stage_noise, 309),
    ParticleEmitter(g_stage_noise, 310), ParticleEmitter(g_stage_noise, 311),
    ParticleEmitter(g_stage_noise, 312), ParticleEmitter(g_stage_noise, 313),
    ParticleEmitter(g_stage_noise, 314), ParticleEmitter(g_stage_noise, 315),
    ParticleEmitter(g_stage_noise, 316)
}};
static ParticleEmitter   g_leaves_emitter(g_stage_noise, 320);
static ParticleEmitter   g_fireworks_emitter(g_stage_noise, 321);
static ParticleEmitter   g_grasshopper_emitter(g_stage_noise, 322);
static ParticleEmitter   g_ant_emitter(g_stage_noise, 323);
static ParticleEmitter   g_firefly_emitter(g_stage_noise, 324);

static SeamlessFogActor* g_cloud_actor     = nullptr;
static SeamlessFogActor* g_fog_back_actor  = nullptr;
static SeamlessFogActor* g_fog_mid_actor   = nullptr;
static SeamlessFogActor* g_fog_front_actor = nullptr;
static UfoActor*         g_ufo_actor       = nullptr;
static NoseFlyActor*     g_nose_fly_actor  = nullptr;
static SpiderActor*      g_spider_actor    = nullptr;
static SnakeActor*       g_snake_actor     = nullptr;
static LightningActor*   g_lightning_actor = nullptr;
static FireActor*        g_fire_actor      = nullptr;
static StaticPropActor*  g_cobweb_actor    = nullptr;

static std::array<uint8_t, 25> g_fx_depth_masks{}; // 25 FX slots x 4-bit binary depth (1=SKY, 2=FAR, 4=MID, 8=FRONT)
static int                     g_selected_fx_slot = -1;
static bool                    g_fx_osd_active    = false;

static SharedVideoClip   g_fx_video;
static SDL_Texture*      g_default_particle_tex = nullptr;
static SDL_Texture*      g_rain_drop_tex        = nullptr;
static bool              g_stage_fx_enabled     = false;
static int               g_stage_fx_mode        = 3; // 3 = PERLIN_SWIRL
static int               g_stage_fx_count       = 50;
static int               g_critter_swarm_count  = 1;
static uint8_t           g_parallax_vel_x       = 128; // 128 = still
static uint8_t           g_parallax_vel_y       = 128; // 128 = still

// ==============================================================================
// Section B. On-Screen Display (OSD), Help, About & Telemetry Overlays
// ==============================================================================
struct OsdState {
    std::array<char, 96> text{};
    uint32_t             hideAtMs   = 0;
    bool                 persistent = false;
    bool                 enabled    = false;
};

static OsdState osd;
static bool     g_show_about_overlay = false;

// ------------------------------------------------------------------------------
// 1. Display a timed or persistent OSD banner message
// ------------------------------------------------------------------------------
static void showOsd(const char* msg, uint32_t durationMs = 3000, bool persist = false)
{
    if (!osd.enabled && !persist) {
        return;
    }
    std::snprintf(osd.text.data(), osd.text.size(), "%s", msg);
    osd.hideAtMs   = (durationMs == 0) ? 0 : (SDL_GetTicks() + durationMs);
    osd.persistent = persist;
}

// ------------------------------------------------------------------------------
// 2. Clear and hide any currently active OSD banner
// ------------------------------------------------------------------------------
static void hideOsd(void)
{
    osd.text.at(0) = '\0';
    osd.hideAtMs   = 0;
    osd.persistent = false;
}

// ------------------------------------------------------------------------------
// 3. Render a single 5x7 pixel bitmap character onto the SDL renderer
// ------------------------------------------------------------------------------
static void drawChar5x7(SDL_Renderer* ren, char c, int x, int y, int scale)
{
    if (c >= 'a' && c <= 'z') {
        c = static_cast<char>(c - 'a' + 'A');
    }
    static const std::array<std::array<uint8_t, 5>, 26> font = {{
        {{0x7E,0x11,0x11,0x11,0x7E}},{{0x7F,0x49,0x49,0x49,0x36}},{{0x3E,0x41,0x41,0x41,0x22}},{{0x7F,0x41,0x41,0x22,0x1C}},
        {{0x7F,0x49,0x49,0x49,0x41}},{{0x7F,0x09,0x09,0x09,0x01}},{{0x3E,0x41,0x49,0x49,0x7A}},{{0x7F,0x08,0x08,0x08,0x7F}},
        {{0x00,0x41,0x7F,0x41,0x00}},{{0x20,0x40,0x41,0x3F,0x01}},{{0x7F,0x08,0x14,0x22,0x41}},{{0x7F,0x40,0x40,0x40,0x40}},
        {{0x7F,0x02,0x0C,0x02,0x7F}},{{0x7F,0x04,0x08,0x10,0x7F}},{{0x3E,0x41,0x41,0x41,0x3E}},{{0x7F,0x09,0x09,0x09,0x06}},
        {{0x3E,0x41,0x51,0x21,0x5E}},{{0x7F,0x09,0x19,0x29,0x46}},{{0x46,0x49,0x49,0x49,0x31}},{{0x01,0x01,0x7F,0x01,0x01}},
        {{0x3F,0x40,0x40,0x40,0x3F}},{{0x1F,0x20,0x40,0x20,0x1F}},{{0x3F,0x40,0x38,0x40,0x3F}},{{0x63,0x14,0x08,0x14,0x63}},
        {{0x07,0x08,0x70,0x08,0x07}},{{0x61,0x51,0x49,0x45,0x43}}
    }};
    static const std::array<std::array<uint8_t, 5>, 10> digits = {{
        {{0x3E,0x51,0x49,0x45,0x3E}},{{0x00,0x42,0x7F,0x40,0x00}},{{0x42,0x61,0x51,0x49,0x46}},{{0x21,0x41,0x45,0x4B,0x31}},
        {{0x18,0x14,0x12,0x7F,0x10}},{{0x27,0x45,0x45,0x45,0x39}},{{0x3C,0x4A,0x49,0x49,0x30}},{{0x01,0x71,0x09,0x05,0x03}},
        {{0x36,0x49,0x49,0x49,0x36}},{{0x06,0x49,0x49,0x29,0x1E}}
    }};
    std::array<uint8_t, 5> custom = {{0,0,0,0,0}};
    const uint8_t* glyph = custom.data();
    if (c >= 'A' && c <= 'Z') {
        glyph = font.at(c - 'A').data();
    } else if (c >= '0' && c <= '9') {
        glyph = digits.at(c - '0').data();
    } else {
        if (c == ':')      { custom.at(1) = 0x36; custom.at(3) = 0x36; }
        else if (c == '.') { custom.at(2) = 0x60; custom.at(3) = 0x60; }
        else if (c == '-') { custom.at(1) = 0x08; custom.at(2) = 0x08; custom.at(3) = 0x08; }
        else if (c == '+') { custom.at(1) = 0x08; custom.at(2) = 0x1C; custom.at(3) = 0x08; }
        else if (c == '/') { custom.at(0) = 0x20; custom.at(1) = 0x10; custom.at(2) = 0x08; custom.at(3) = 0x04; custom.at(4) = 0x02; }
        else if (c == '[') { custom.at(1) = 0x7F; custom.at(2) = 0x41; }
        else if (c == ']') { custom.at(2) = 0x41; custom.at(3) = 0x7F; }
        else if (c == '(') { custom.at(2) = 0x3E; custom.at(3) = 0x41; }
        else if (c == ')') { custom.at(1) = 0x41; custom.at(2) = 0x3E; }
        else if (c == '!') { custom.at(2) = 0x5F; }
        else if (c == '=') { custom.at(1) = 0x14; custom.at(2) = 0x14; custom.at(3) = 0x14; }
        else if (c == '#') { custom.at(1) = 0x7F; custom.at(2) = 0x14; custom.at(3) = 0x7F; }
        else if (c == '_') { custom.at(0) = 0x40; custom.at(1) = 0x40; custom.at(2) = 0x40; custom.at(3) = 0x40; custom.at(4) = 0x40; }
        else if (c == '%') { custom.at(0) = 0x62; custom.at(1) = 0x64; custom.at(2) = 0x08; custom.at(3) = 0x13; custom.at(4) = 0x23; }
    }
    for (int col = 0; col < 5; ++col) {
        for (int row = 0; row < 7; ++row) {
            if (glyph[col] & (1 << row)) {
                SDL_Rect px{x + col * scale, y + row * scale, scale, scale};
                SDL_RenderFillRect(ren, &px);
            }
        }
    }
}

// ------------------------------------------------------------------------------
// 4. Render a null-terminated ASCII string using 5x7 bitmap glyphs
// ------------------------------------------------------------------------------
static void drawTextLine(SDL_Renderer* ren, const char* str, int x, int y, int scale)
{
    int len = static_cast<int>(std::strlen(str));
    for (int i = 0; i < len; ++i) {
        drawChar5x7(ren, str[i], x + i * (6 * scale), y, scale);
    }
}

// ------------------------------------------------------------------------------
// 5. Retrieve the first non-loopback IPv4 address for telemetry display
// ------------------------------------------------------------------------------
static std::string getLocalIpAddress(void)
{
    struct ifaddrs* ifaddr = nullptr;
    if (getifaddrs(&ifaddr) != 0) {
        return "0.0.0.0";
    }
    std::string res = "127.0.0.1";
    for (auto* ifa = ifaddr; ifa; ifa = ifa->ifa_next) {
        if (!ifa->ifa_addr || ifa->ifa_addr->sa_family != AF_INET) {
            continue;
        }
        char ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &((sockaddr_in*)ifa->ifa_addr)->sin_addr, ip, sizeof(ip));
        if (std::strcmp(ip, "127.0.0.1") != 0) {
            res = ip;
            break;
        }
    }
    freeifaddrs(ifaddr);
    return res;
}

// ------------------------------------------------------------------------------
// 6. Render large 2-line OSD banner with subheader & bottom status bar
// ------------------------------------------------------------------------------
static void renderOsdOverlay(SDL_Renderer* ren, uint32_t nowMs, const AppRuntimeState& state)
{
    if (state.flatImport.open || state.cameraCapture.open || state.packBrowser.open || state.eyeBrowser.open || state.showHelpOverlay || g_show_about_overlay) {
        return;
    }

    const char* subHeader = "MAKINGFACES";
    if (state.isEditingOffsets) {
        subHeader = state.dualProfileMode
                        ? (state.editingRightProfile ? "CALIBRATE [RIGHT CHARACTER]" : "CALIBRATE [LEFT CHARACTER]")
                        : "CALIBRATE [TAB=PART  ALT=SIZE]";
    } else if (g_fx_osd_active && g_selected_fx_slot >= 0) {
        subHeader = "FX DIRECTOR [B/SH+B=FX  ALT+1..4=LAYER]";
    } else if (state.curFace >= PROFILE_FACE_START_INDEX) {
        subHeader = state.dualProfileMode
                        ? (state.editingRightProfile ? "DUAL PROFILE [ACTIVE: RIGHT]" : "DUAL PROFILE [ACTIVE: LEFT]")
                        : "SINGLE PROFILE [Y=DUAL]";
    }

    if (osd.text.at(0) != '\0') {
        if (!osd.persistent && osd.hideAtMs > 0 && nowMs >= osd.hideAtMs) {
            osd.text.at(0) = '\0';
            g_fx_osd_active = false;
        } else if (state.osdEnabled || state.isEditingOffsets || g_fx_osd_active) {
            int len    = static_cast<int>(std::strlen(osd.text.data()));
            int subLen = static_cast<int>(std::strlen(subHeader));
            int scale  = (len > 26) ? 2 : 3;
            int boxW   = std::min(BASE_CANVAS_W - 24, std::max(len * (6 * scale), subLen * 12) + 36);
            int boxH   = 14 + 6 + 7 * scale + 16;
            SDL_Rect bg{(BASE_CANVAS_W - boxW) / 2, 14, boxW, boxH};
            SDL_SetRenderDrawColor(ren, 10, 14, 24, 232);
            SDL_RenderFillRect(ren, &bg);
            SDL_SetRenderDrawColor(ren, 0, 229, 255, 255);
            SDL_RenderDrawRect(ren, &bg);
            SDL_SetRenderDrawColor(ren, 0, 229, 255, 255);
            drawTextLine(ren, subHeader, bg.x + 16, bg.y + 8, 2);
            SDL_SetRenderDrawColor(ren, 255, 255, 255, 255);
            drawTextLine(ren, osd.text.data(), bg.x + 16, bg.y + 28, scale);
        }
    }

    if (state.osdEnabled && !state.isDmxActive) {
        std::array<char, 64> status{};
        if (state.curFace >= PROFILE_FACE_START_INDEX && state.dualProfileMode) {
            uint8_t rId = (state.dualFaceLayer.testIndex >= PROFILE_FACE_START_INDEX) ? state.dualFaceLayer.testIndex : state.curFace;
            std::snprintf(status.data(), status.size(), "DUAL %s | L:%03u R:%03u [CTRL+TAB]",
                          state.editingRightProfile ? "RIGHT" : "LEFT", state.curFace, rId);
        } else if (state.curFace >= PROFILE_FACE_START_INDEX) {
            std::snprintf(status.data(), status.size(), "SINGLE PROFILE [%03u] | Y=DUAL", state.curFace);
        } else {
            std::snprintf(status.data(), status.size(), "FRONT FACE [%03u] | F1=HELP", state.curFace);
        }
        int sw = std::min(BASE_CANVAS_W - 20, static_cast<int>(std::strlen(status.data())) * 12 + 24);
        SDL_Rect sb{(BASE_CANVAS_W - sw) / 2, BASE_CANVAS_H - 36, sw, 26};
        SDL_SetRenderDrawColor(ren, 10, 14, 24, 215);
        SDL_RenderFillRect(ren, &sb);
        SDL_SetRenderDrawColor(ren, 0, 229, 255, 180);
        SDL_RenderDrawRect(ren, &sb);
        SDL_SetRenderDrawColor(ren, 170, 240, 255, 255);
        drawTextLine(ren, status.data(), sb.x + 12, sb.y + 6, 2);
    }
}

// ------------------------------------------------------------------------------
// 7. Render F1 help and F9 about overlays
// ------------------------------------------------------------------------------
static void renderHelpAndAbout(SDL_Renderer* ren, const FaceAppConfig& cfg, const AppRuntimeState& state)
{
    if (state.showHelpOverlay) {
        SDL_Rect box{20, 36, BASE_CANVAS_W - 40, 728};
        SDL_SetRenderDrawColor(ren, 10, 14, 22, 236);
        SDL_RenderFillRect(ren, &box);
        SDL_SetRenderDrawColor(ren, 0, 229, 255, 255);
        SDL_RenderDrawRect(ren, &box);
        static const std::array<const char*, 28> lines = {
            "MAKINGFACES V1.0.0 [F1/ESC=CLOSE Q=QUIT]",
            "-------------------------------------------",
            "F1:HELP  F2:NET  F3:AUTO  F4/S:SAVE+DEFAULT",
            "F5:OSD   F6/X:PACK BROWSER   F7/CTRL+Z:UNDO",
            "F8:EYE BROWSER   F9:ABOUT    F11:FULLSCREEN",
            "F12:BLACKOUT  CTRL+I:IMPORT  CTRL+P:CAMERA",
            "-------------------------------------------",
            "F:FACE [0..255]      R:EARS [0..255]",
            "N:NOSE [0..255]      W:BROWS [0..255]",
            "J:BEARD/JAW [0..255] G:GLASSES [0..255]",
            "H:HAIR [0..255]      C:CAP/HAT [0..255]",
            "P:PAINT/MKP [0..255] T:TEETH [0..15]",
            "A:ANIM [0..64]       O:EYE MODE [1..12]",
            "I:IRIS [1..10]       E:LASHES [0..15]",
            "L:LIPSTICK [0..9]    K:BG SCENE [0..255]",
            "Y:DUAL  CTRL+TAB:SWITCH L/R  V:FADE TIME",
            "-------------------------------------------",
            "PGUP/PGDN: MEMORY BANK [00..24]",
            "1..9,0: LOCK MEMORY IN ACTIVE BANK",
            "CTRL+1..9,0: SAVE MEMORY SLOT (LOCKED)",
            "M / SHIFT+M: STEP MEMORY SLOT",
            "-------------------------------------------",
            "B / SHIFT+B: SELECT FX [01..25]",
            "ALT+1..4: TOGGLE FX DEPTH [SKY/FAR/MID/FRT]",
            "ALT+0: CLEAR FX   CTRL+B: ALL FX OFF",
            "CTRL + / -: SWARM COUNT [1..50]",
            "TAB+ARROWS: MOVE  ALT+ARROWS: SCALE",
            "CTRL+X:ZIP  CTRL+E:QLC+  CTRL+R:REC AUDIO"
        };
        SDL_SetRenderDrawColor(ren, 240, 248, 255, 255);
        for (size_t i = 0; i < lines.size(); ++i) {
            drawTextLine(ren, lines.at(i), 34, 48 + static_cast<int>(i) * 25, 2);
        }
    }
    if (g_show_about_overlay) {
        SDL_Rect box{40, 175, BASE_CANVAS_W - 80, 450};
        SDL_SetRenderDrawColor(ren, 10, 14, 24, 244);
        SDL_RenderFillRect(ren, &box);
        SDL_SetRenderDrawColor(ren, 0, 229, 255, 255);
        SDL_RenderDrawRect(ren, &box);
        SDL_SetRenderDrawColor(ren, 120, 245, 255, 255);
        drawTextLine(ren, "MAKINGFACES ANIMATRONIC ENGINE", 72, 204, 2);
        std::array<char, 64> verLine{};
        std::snprintf(verLine.data(), verLine.size(), "VERSION: %s  (BUILD: %s)", APP_VERSION_STR, __DATE__);
        SDL_SetRenderDrawColor(ren, 255, 255, 255, 255);
        drawTextLine(ren, verLine.data(), 72, 242, 2);
        drawTextLine(ren, "ARCHITECTURE: MAGNUS HERRLIN 2026", 72, 280, 2);
        SDL_SetRenderDrawColor(ren, 200, 225, 245, 255);
        drawTextLine(ren, "CREDITS & CORE INSPIRATION:", 72, 332, 2);
        drawTextLine(ren, "- BODMER TFT_ESPI SPRITE/FB PIPELINE", 72, 366, 2);
        drawTextLine(ren, "- ADAFRUIT UNCANNY EYES POLAR ENGINE", 72, 398, 2);
        drawTextLine(ren, "- SDL2 WAYLAND + 39CH ART-NET / SACN", 72, 430, 2);
        std::array<char, 64> bankInfo{};
        std::snprintf(bankInfo.data(), bankInfo.size(), "EYE BANK: %d EYES   MEMORIES: %d", getEyeBankCount(), static_cast<int>(cfg.memory_combos.size()));
        SDL_SetRenderDrawColor(ren, 255, 230, 120, 255);
        drawTextLine(ren, bankInfo.data(), 72, 486, 2);
        drawTextLine(ren, "PRESS F9 OR ESC TO CLOSE", 72, 558, 2);
    }
}

// ==============================================================================
// Section C. Live Camera & USB Microscope Viewfinder (USB V4L2 + Pi CSI)
// ==============================================================================
static cv::VideoCapture g_usb_cam;

// ------------------------------------------------------------------------------
// 8. Release USB camera capture handle and live viewfinder SDL texture
// ------------------------------------------------------------------------------
static void closeCameraViewfinder(CameraCaptureState& cam)
{
    if (g_usb_cam.isOpened()) {
        g_usb_cam.release();
    }
    if (cam.liveTex) {
        SDL_DestroyTexture(cam.liveTex);
        cam.liveTex = nullptr;
    }
    cam.texW = 0;
    cam.texH = 0;
    cam.open = false;
}

// ------------------------------------------------------------------------------
// 9. Probe /dev/video0..3 for USB webcam/microscope or fall back to Pi CSI
// ------------------------------------------------------------------------------
static void openCameraViewfinder(CameraCaptureState& cam)
{
    closeCameraViewfinder(cam);
    cam.usbAvailable = false;
    for (int idx = 0; idx < 4; ++idx) {
        if (g_usb_cam.open(idx, cv::CAP_V4L2)) {
            g_usb_cam.set(cv::CAP_PROP_FRAME_WIDTH, 640);
            g_usb_cam.set(cv::CAP_PROP_FRAME_HEIGHT, 480);
            cv::Mat testFrame;
            if (g_usb_cam.read(testFrame) && !testFrame.empty()) {
                cam.usbAvailable = true;
                cam.usbDeviceIdx = idx;
                cam.useCsiMode   = false;
                break;
            }
            g_usb_cam.release();
        }
    }
    if (!cam.usbAvailable) {
        cam.useCsiMode = true;
    }
    cam.open = true;
}

// ------------------------------------------------------------------------------
// 10. Generate next unused import/cam_NNN.jpg or import/eye_NNN.jpg path
// ------------------------------------------------------------------------------
static std::string nextCameraImportPath(const std::string& importDir, const char* prefix)
{
    fs::create_directories(importDir);
    for (int i = 1; i <= 999; ++i) {
        std::array<char, 32> buf{};
        std::snprintf(buf.data(), buf.size(), "%s_%03d.jpg", prefix, i);
        fs::path p = fs::path(importDir) / buf.data();
        if (!fs::exists(p)) {
            return p.string();
        }
    }
    return (fs::path(importDir) / (std::string(prefix) + "_999.jpg")).string();
}

// ------------------------------------------------------------------------------
// 11. Capture still frame from USB camera/microscope or Pi CSI into import/
// ------------------------------------------------------------------------------
static std::string captureCameraPhotoToImport(CameraCaptureState& cam, const std::string& importDir, const char* prefix)
{
    std::string outPath = nextCameraImportPath(importDir, prefix);
    if (!cam.useCsiMode && g_usb_cam.isOpened()) {
        cv::Mat frame;
        for (int flush = 0; flush < 3; ++flush) {
            g_usb_cam.read(frame);
        }
        if (!frame.empty()) {
            if (cam.rotate90) {
                cv::rotate(frame, frame, cv::ROTATE_90_CLOCKWISE);
            }
            if (!cam.eyeScopeMode) {
                cv::flip(frame, frame, 1);
            }
            if (!cam.eyeScopeMode && frame.cols > frame.rows) {
                int cropW = std::max(1, (frame.rows * 3) / 4);
                int x0 = std::max(0, (frame.cols - cropW) / 2);
                frame = frame(cv::Rect(x0, 0, cropW, frame.rows)).clone();
            }
            if (cv::imwrite(outPath, frame)) {
                return outPath;
            }
        }
        return "";
    }
    std::string cmd = "rpicam-still -n -t 300 --width 1200 --height 1600 -o \"" + outPath + "\" 2>/dev/null || "
                      "libcamera-still -n -t 300 --width 1200 --height 1600 -o \"" + outPath + "\" 2>/dev/null";
    if (std::system(cmd.c_str()) == 0 && fs::exists(outPath) && fs::file_size(outPath) > 0) {
        return outPath;
    }
    return "";
}

// ------------------------------------------------------------------------------
// 12. Draw ellipse outline for microscope iris guides & face/eye/mouth ovals
// ------------------------------------------------------------------------------
static void drawEllipseOutline(SDL_Renderer* ren, int cx, int cy, int rx, int ry)
{
    int prevX = cx + rx;
    int prevY = cy;
    for (int deg = 4; deg <= 360; deg += 4) {
        double a = deg * 3.14159265 / 180.0;
        int nx = cx + static_cast<int>(std::round(std::cos(a) * rx));
        int ny = cy + static_cast<int>(std::round(std::sin(a) * ry));
        SDL_RenderDrawLine(ren, prevX, prevY, nx, ny);
        prevX = nx;
        prevY = ny;
    }
}

// ------------------------------------------------------------------------------
// 13. Grab live USB frame, upload texture & draw face or microscope eye guides
// ------------------------------------------------------------------------------
static void renderCameraViewfinderOverlay(SDL_Renderer* ren, CameraCaptureState& cam)
{
    if (!cam.open) {
        return;
    }
    if (!cam.useCsiMode && g_usb_cam.isOpened()) {
        cv::Mat bgr, argb;
        if (g_usb_cam.read(bgr) && !bgr.empty()) {
            if (cam.rotate90) {
                cv::rotate(bgr, bgr, cv::ROTATE_90_CLOCKWISE);
            }
            if (!cam.eyeScopeMode) {
                cv::flip(bgr, bgr, 1);
            }
            if (!cam.eyeScopeMode && bgr.cols > bgr.rows) {
                int cropW = std::max(1, (bgr.rows * 3) / 4);
                int x0 = std::max(0, (bgr.cols - cropW) / 2);
                bgr = bgr(cv::Rect(x0, 0, cropW, bgr.rows)).clone();
            }
            cv::cvtColor(bgr, argb, cv::COLOR_BGR2BGRA);
            if (!cam.liveTex || cam.texW != argb.cols || cam.texH != argb.rows) {
                if (cam.liveTex) {
                    SDL_DestroyTexture(cam.liveTex);
                }
                cam.liveTex = SDL_CreateTexture(ren, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, argb.cols, argb.rows);
                cam.texW = argb.cols;
                cam.texH = argb.rows;
            }
            if (cam.liveTex) {
                SDL_UpdateTexture(cam.liveTex, nullptr, argb.data, static_cast<int>(argb.step));
            }
        }
    }

    SDL_Rect box{30, 56, BASE_CANVAS_W - 60, 688};
    SDL_SetRenderDrawColor(ren, 10, 14, 24, 248);
    SDL_RenderFillRect(ren, &box);
    SDL_SetRenderDrawColor(ren, 0, 229, 255, 255);
    SDL_RenderDrawRect(ren, &box);
    SDL_SetRenderDrawColor(ren, 120, 245, 255, 255);
    drawTextLine(ren, cam.useCsiMode ? "CAMERA / MICROSCOPE [PI CSI RPICAM]" : "CAMERA / MICROSCOPE [USB 3:4 PORTRAIT]", 48, 74, 2);

    SDL_Rect view{120, 108, 360, 480};
    SDL_SetRenderDrawColor(ren, 18, 24, 36, 255);
    SDL_RenderFillRect(ren, &view);
    if (!cam.useCsiMode && cam.liveTex) {
        SDL_RenderCopy(ren, cam.liveTex, nullptr, &view);
    } else {
        SDL_SetRenderDrawColor(ren, 220, 235, 250, 255);
        drawTextLine(ren, "PI CSI CAMERA READY", 180, 325, 2);
        drawTextLine(ren, "PRESS SPACE OR E TO SNAP", 156, 360, 2);
    }
    SDL_SetRenderDrawColor(ren, 0, 229, 255, 220);
    SDL_RenderDrawRect(ren, &view);
    if (cam.eyeScopeMode) {
        drawEllipseOutline(ren, 300, 348, 155, 155);
        drawEllipseOutline(ren, 300, 348, 52, 52);
    } else {
        drawEllipseOutline(ren, 300, 368, 125, 175);
        drawEllipseOutline(ren, 252, 322, 30, 20);
        drawEllipseOutline(ren, 348, 322, 30, 20);
        drawEllipseOutline(ren, 300, 445, 44, 22);
    }

    SDL_SetRenderDrawColor(ren, 255, 230, 120, 255);
    drawTextLine(ren, "SPACE/ENTER: SNAP PHOTO -> STUDIO", 48, 608, 2);
    drawTextLine(ren, "E: SNAP EYE IRIS (MICROSCOPE MODE)", 48, 638, 2);
    drawTextLine(ren, "M:GUIDE  R:ROT90  TAB:USB/CSI  ESC:EXIT", 48, 668, 2);
}
// ------------------------------------------------------------------------------
// 14. Render F2 live DMX & network telemetry box
// ------------------------------------------------------------------------------
static int s_bgTolerance = 48;

static void renderTelemetryOverlay(SDL_Renderer* ren, const FaceAppConfig& cfg, const AppRuntimeState& state)
{
    static std::string cachedIp = getLocalIpAddress();
    SDL_Rect box{24, BASE_CANVAS_H - 132, BASE_CANVAS_W - 48, 112};
    SDL_SetRenderDrawColor(ren, 10, 14, 22, 225);
    SDL_RenderFillRect(ren, &box);
    SDL_SetRenderDrawColor(ren, 0, 229, 255, 255);
    SDL_RenderDrawRect(ren, &box);
    std::array<char, 84> l1{}, l2{}, l3{};
    std::snprintf(l1.data(), l1.size(), "IP: %s  PROTO: %s  UNI: %u  CH: %u", cachedIp.c_str(), cfg.proto.c_str(), cfg.universe, cfg.start_channel);
    std::snprintf(l2.data(), l2.size(), "DMX: %s [%u PKT/S]  BANK: %02d  FADE: %.1FS", state.isDmxActive ? "ONLINE" : "AUTO/IDLE", state.dmxPktRate, state.memoryBank, cfg.fade_ms / 1000.0f);
    std::snprintf(l3.data(), l3.size(), "MEMORY: #%03d  ANIM: %s  DUAL: %s", getActiveMemorySlot(), getAnimationName(state.manualTestAnim), state.dualProfileMode ? "ON" : "OFF");
    SDL_SetRenderDrawColor(ren, 120, 245, 255, 255);
    drawTextLine(ren, l1.data(), 38, box.y + 16, 2);
    drawTextLine(ren, l2.data(), 38, box.y + 46, 2);
    drawTextLine(ren, l3.data(), 38, box.y + 76, 2);
}

// ------------------------------------------------------------------------------
// 15. Render F6 Pack Browser & Live ZIP Pack Inspector overlay
// ------------------------------------------------------------------------------
static void renderPackBrowserOverlay(SDL_Renderer* ren, const PackBrowserState& pb)
{
    if (pb.inspectMode) {
        SDL_Rect panel{18, 536, BASE_CANVAS_W - 36, 246};
        SDL_SetRenderDrawColor(ren, 10, 14, 24, 225);
        SDL_RenderFillRect(ren, &panel);
        SDL_SetRenderDrawColor(ren, 0, 229, 255, 255);
        SDL_RenderDrawRect(ren, &panel);
        const std::string& pName = pb.entries.at(pb.selectedIdx).name;
        std::array<char, 84> hdr{};
        std::snprintf(hdr.data(), hdr.size(), "PACK INSPECTOR: %s.ZIP [LIVE PREVIEW]", pName.c_str());
        SDL_SetRenderDrawColor(ren, 120, 245, 255, 255);
        drawTextLine(ren, hdr.data(), 32, 548, 2);
        int total = static_cast<int>(pb.inspectItems.size());
        int start = std::max(0, pb.inspectIdx - 2);
        for (int row = 0; row < 5 && (start + row) < total; ++row) {
            int idx = start + row;
            const auto& it = pb.inspectItems.at(idx);
            bool cur = (idx == pb.inspectIdx);
            std::array<char, 84> line{};
            std::snprintf(line.data(), line.size(), "%s [%c] %s", cur ? ">" : " ", it.selected ? 'X' : ' ', it.label.c_str());
            SDL_SetRenderDrawColor(ren, cur ? 255 : 215, cur ? 235 : 230, cur ? 110 : 245, 255);
            drawTextLine(ren, line.data(), 32, 578 + row * 24, 2);
        }
        SDL_SetRenderDrawColor(ren, 255, 230, 120, 255);
        drawTextLine(ren, "UP/DN:PREVIEW  SPACE:CHECK  A:ALL/NONE", 32, 710, 2);
        drawTextLine(ren, "ENTER:IMPORT CHECKED   ESC/LEFT:BACK", 32, 738, 2);
        return;
    }

    SDL_Rect box{16, 28, BASE_CANVAS_W - 32, 744};
    SDL_SetRenderDrawColor(ren, 10, 14, 24, 246);
    SDL_RenderFillRect(ren, &box);
    SDL_SetRenderDrawColor(ren, 0, 229, 255, 255);
    SDL_RenderDrawRect(ren, &box);
    SDL_SetRenderDrawColor(ren, 255, 255, 255, 255);
    drawTextLine(ren, "THEME PACK BROWSER [F6/ESC=CLOSE]", 34, 46, 2);
    if (pb.entries.empty()) {
        drawTextLine(ren, "NO .ZIP PACKS FOUND IN PACKS/", 92, 360, 2);
    } else {
        const auto& cur = pb.entries.at(pb.selectedIdx);
        std::array<char, 80> hdr{}, stats{};
        std::snprintf(hdr.data(), hdr.size(), "PACK [%02d/%02d]: %s.ZIP", pb.selectedIdx + 1, static_cast<int>(pb.entries.size()), cur.name.c_str());
        SDL_SetRenderDrawColor(ren, 120, 245, 255, 255);
        drawTextLine(ren, hdr.data(), 34, 80, 2);
        SDL_Rect thumbFrame{120, 118, 360, 480};
        SDL_SetRenderDrawColor(ren, 20, 26, 40, 255);
        SDL_RenderFillRect(ren, &thumbFrame);
        if (cur.thumbTex) {
            SDL_RenderCopy(ren, cur.thumbTex, nullptr, &thumbFrame);
        }
        SDL_SetRenderDrawColor(ren, 0, 229, 255, 255);
        SDL_RenderDrawRect(ren, &thumbFrame);
        std::snprintf(stats.data(), stats.size(), "FACES: %d   MEMORIES: %d", cur.faceCount, cur.memoryCount);
        SDL_SetRenderDrawColor(ren, 220, 235, 250, 255);
        drawTextLine(ren, stats.data(), 120, 614, 2);
    }
    SDL_SetRenderDrawColor(ren, 255, 230, 120, 255);
    drawTextLine(ren, "L/R:BROWSE  I/DN:INSPECT  ENTER:APPEND ALL", 34, 656, 2);
    drawTextLine(ren, "R:REPLACE DEFAULT  M:IMPORT/  F7:UNDO", 34, 686, 2);
    drawTextLine(ren, pb.wipeConfirm ? "PRESS DEL AGAIN TO CONFIRM WIPE ALL!" : "DEL:WIPE ALL ASSETS  CTRL+X:EXPORT ZIP", 34, 716, 2);
}

// ------------------------------------------------------------------------------
// 16. Render Ctrl+I Calibration Studio bar & F8 Adafruit Eye Browser
// ------------------------------------------------------------------------------
static void renderImportAndEyeBrowser(SDL_Renderer* ren, const AppRuntimeState& state)
{
    if (state.flatImport.open) {
        static const std::array<const char*, 12> kCatNames = {
            "FACE", "EARS", "NOSE", "BROWS", "BEARD", "GLASSES", "HAIR", "HAT", "MAKEUP", "BG", "EYE_IRIS", "EYE_SCLERA"
        };
        SDL_Rect panel{6, 746, BASE_CANVAS_W - 12, 48};
        SDL_SetRenderDrawColor(ren, 10, 14, 24, 210);
        SDL_RenderFillRect(ren, &panel);
        SDL_SetRenderDrawColor(ren, 0, 229, 255, 200);
        SDL_RenderDrawRect(ren, &panel);
        if (state.flatImport.items.empty()) {
            SDL_SetRenderDrawColor(ren, 120, 245, 255, 255);
            drawTextLine(ren, "IMPORT/ EMPTY - [C] CAMERA   [ESC] EXIT", 18, 766, 1);
        } else {
            const auto& cur = state.flatImport.items.at(state.flatImport.selectedIdx);
            std::array<char, 104> l1{}, l2{};
            std::snprintf(l1.data(), l1.size(), "[%c] %d/%d %s | <%s> %d%% | [V]EYES:%s [N]SUDD:%s [W]BG:%s(%d)",
                          cur.selected ? 'X' : ' ', state.flatImport.selectedIdx + 1, static_cast<int>(state.flatImport.items.size()),
                          cur.fileName.c_str(), kCatNames.at(cur.targetCatIdx), static_cast<int>(std::round(cur.scale * 100.0f)),
                          cur.cutEyes ? "ON" : "OFF", cur.doInpaint ? "ON" : "OFF", cur.removeWhite ? "ON" : "OFF", s_bgTolerance);
            std::snprintf(l2.data(), l2.size(), "DRAG/WHEEL:FIT  B/SH+B:TOL  Z/X:ROT  L/R:TYPE  C:CAM  ENTER:SAVE  ESC:EXIT");
            SDL_SetRenderDrawColor(ren, 255, 255, 255, 255);
            drawTextLine(ren, l1.data(), 14, 755, 1);
            SDL_SetRenderDrawColor(ren, 255, 230, 120, 255);
            drawTextLine(ren, l2.data(), 14, 775, 1);
        }
        return;
    }

    if (state.eyeBrowser.open) {
        SDL_Rect box{40, 125, BASE_CANVAS_W - 80, 555};
        SDL_SetRenderDrawColor(ren, 10, 14, 24, 244);
        SDL_RenderFillRect(ren, &box);
        SDL_SetRenderDrawColor(ren, 0, 229, 255, 255);
        SDL_RenderDrawRect(ren, &box);
        SDL_SetRenderDrawColor(ren, 255, 255, 255, 255);
        drawTextLine(ren, "ADAFRUIT EYE BROWSER [F8/ESC=CLOSE]", 64, 145, 2);
        if (!state.eyeBrowser.entries.empty()) {
            const auto& cur = state.eyeBrowser.entries.at(state.eyeBrowser.selectedIdx);
            std::array<char, 80> hdr{};
            std::snprintf(hdr.data(), hdr.size(), "EYE [%02d/%02d]: %s", state.eyeBrowser.selectedIdx + 1, static_cast<int>(state.eyeBrowser.entries.size()), cur.name.c_str());
            SDL_SetRenderDrawColor(ren, 120, 245, 255, 255);
            drawTextLine(ren, hdr.data(), 64, 180, 2);
            SDL_Rect eyeBox{185, 226, 230, 230};
            SDL_SetRenderDrawColor(ren, 18, 24, 36, 255);
            SDL_RenderFillRect(ren, &eyeBox);
            if (cur.thumbTex) {
                SDL_Rect dst{195, 236, 210, 210};
                SDL_RenderCopy(ren, cur.thumbTex, nullptr, &dst);
            }
            SDL_SetRenderDrawColor(ren, 0, 229, 255, 255);
            SDL_RenderDrawRect(ren, &eyeBox);
        }
        SDL_SetRenderDrawColor(ren, 255, 230, 120, 255);
        drawTextLine(ren, "LEFT/RIGHT: BROWSE   ENTER: ACTIVE/BOTH", 64, 486, 2);
        drawTextLine(ren, "L: LEFT EYE ONLY     R: RIGHT EYE ONLY", 64, 518, 2);
        drawTextLine(ren, "0 / BACKSPACE: AUTO  S/F4: SAVE MEMORY", 64, 550, 2);
        drawTextLine(ren, "DROP *-SCLERA + *-IRIS IN EYES/", 64, 582, 2);
    }
}

// ------------------------------------------------------------------------------
// 17. Orchestrate active UI overlays, camera viewfinder, browsers & telemetry
// ------------------------------------------------------------------------------
static void renderBrowsersAndTelemetry(SDL_Renderer* ren, const FaceAppConfig& cfg, const AppRuntimeState& state)
{
    renderHelpAndAbout(ren, cfg, state);
    if (state.cameraCapture.open) {
        renderCameraViewfinderOverlay(ren, const_cast<CameraCaptureState&>(state.cameraCapture));
        return;
    }
    if (state.showTelemetry) {
        renderTelemetryOverlay(ren, cfg, state);
    }
    if (state.packBrowser.open) {
        renderPackBrowserOverlay(ren, state.packBrowser);
        return;
    }
    renderImportAndEyeBrowser(ren, state);
}
// ==============================================================================
// Section D. Memory Presets, Offset/Scale Profiles & INI Persistence
// ==============================================================================

// ------------------------------------------------------------------------------
// 18. Resolve bank index and digit key (0..9) into clamped memory slot 0..240
// ------------------------------------------------------------------------------
static int resolveBankKeySlot(int bank, int keyDigit)
{
    int slot = (keyDigit == 0) ? (bank * 10) : (bank * 10 + keyDigit);
    return std::max(0, std::min(MAX_MEMORY_SLOT, slot));
}

// ------------------------------------------------------------------------------
// 19. Capture active left & right layers, overrides, eye banks & animation
// ------------------------------------------------------------------------------
static void captureMemoryCombo(FaceAppConfig& cfg, const AppRuntimeState& state, int slot)
{
    int activeAnim = state.manualTestAnim;
    int rawTeeth   = state.dmxMemoryLocked ? state.curTeeth    : g_override_teeth.load();
    int rawLip     = state.dmxMemoryLocked ? state.curLipColor : g_override_lip_color.load();
    int rawLash    = state.dmxMemoryLocked ? state.curEyelash  : g_override_eyelash.load();
    int rawStyle   = state.dmxMemoryLocked ? state.curEyeStyle : g_override_eye_style.load();
    int rawIris    = g_override_iris.load();
    int rawBankL   = getSelectedEyeBankForSide(0);
    int rawBankR   = getSelectedEyeBankForSide(1);

    MemoryPreset mc{};
    mc.enabled           = true;
    mc.face              = state.faceLayer.testIndex;
    mc.makeup            = state.makeupLayer.testIndex;
    mc.ears              = state.earsLayer.testIndex;
    mc.nose              = state.noseLayer.testIndex;
    mc.beard             = state.beardLayer.testIndex;
    mc.glasses           = state.glassesLayer.testIndex;
    mc.hair              = state.hairLayer.testIndex;
    mc.hat               = state.hatLayer.testIndex;
    mc.brows             = state.browsLayer.testIndex;
    mc.right_face        = state.dualFaceLayer.testIndex;
    mc.right_makeup      = state.dualMakeupLayer.testIndex;
    mc.right_ears        = state.dualEarsLayer.testIndex;
    mc.right_nose        = state.dualNoseLayer.testIndex;
    mc.right_beard       = state.dualBeardLayer.testIndex;
    mc.right_glasses     = state.dualGlassesLayer.testIndex;
    mc.right_hair        = state.dualHairLayer.testIndex;
    mc.right_hat         = state.dualHatLayer.testIndex;
    mc.right_brows       = state.dualBrowsLayer.testIndex;
    mc.right_face_x      = state.dualFaceLayer.offsetX;
    mc.right_face_y      = state.dualFaceLayer.offsetY;
    mc.right_face_scale  = state.dualFaceLayer.scale;
    mc.right_mouth_x     = state.rightMouthOffsetX;
    mc.right_mouth_y     = state.rightMouthOffsetY;
    mc.right_mouth_scale = state.rightMouthScale;
    mc.bg_scene          = state.bgLayer.testIndex;
    mc.fx_mask           = state.manualFxMask;
    mc.teeth             = (rawTeeth >= 0) ? rawTeeth : 1;
    mc.lip_color         = (rawLip >= 0) ? rawLip : 3;
    mc.eyelash           = (rawLash >= 0) ? rawLash : 0;
    mc.eye_style         = rawStyle;
    mc.iris              = rawIris;
    mc.eye_bank          = rawBankL;
    mc.eye_bank_right    = rawBankR;
    mc.anim_id           = activeAnim;
    mc.dual_profile      = state.dualProfileMode;
    mc.hide_mouth        = state.manualHideMouth;
    cfg.memory_combos[slot] = mc;
}

// ------------------------------------------------------------------------------
// 20. Apply a single MemoryPreset directly to left & right runtime layers
// ------------------------------------------------------------------------------
static void applyPresetValuesToState(const MemoryPreset& mc, AppRuntimeState& state, int fallbackAnim, bool updateAnim)
{
    state.faceLayer.testIndex        = mc.face;
    state.makeupLayer.testIndex      = mc.makeup;
    state.earsLayer.testIndex        = mc.ears;
    state.noseLayer.testIndex        = mc.nose;
    state.beardLayer.testIndex       = mc.beard;
    state.glassesLayer.testIndex     = mc.glasses;
    state.hairLayer.testIndex        = mc.hair;
    state.hatLayer.testIndex         = mc.hat;
    state.browsLayer.testIndex       = mc.brows;

    uint8_t baseRightFace            = (mc.dual_profile && mc.right_face == 0) ? mc.face : mc.right_face;
    state.dualFaceLayer.testIndex    = baseRightFace;
    state.dualMakeupLayer.testIndex  = mc.right_makeup;
    state.dualEarsLayer.testIndex    = mc.right_ears;
    state.dualNoseLayer.testIndex    = (mc.dual_profile && mc.right_nose == 0) ? baseRightFace : mc.right_nose;
    state.dualBeardLayer.testIndex   = mc.right_beard;
    state.dualGlassesLayer.testIndex = mc.right_glasses;
    state.dualHairLayer.testIndex    = mc.right_hair;
    state.dualHatLayer.testIndex     = mc.right_hat;
    state.dualBrowsLayer.testIndex   = (mc.dual_profile && mc.right_brows == 0) ? baseRightFace : mc.right_brows;
    state.dualFaceLayer.offsetX      = mc.right_face_x;
    state.dualFaceLayer.offsetY      = mc.right_face_y;
    state.dualFaceLayer.scale        = std::max(0.50f, std::min(1.60f, mc.right_face_scale));
    state.rightMouthOffsetX          = mc.right_mouth_x;
    state.rightMouthOffsetY          = mc.right_mouth_y;
    state.rightMouthScale            = std::max(0.50f, std::min(1.60f, mc.right_mouth_scale));
    g_mouth_scale_factor_right.store(state.rightMouthScale, std::memory_order_relaxed);

    state.bgLayer.testIndex          = mc.bg_scene;
    state.manualFxMask               = mc.fx_mask;
    state.dualProfileMode            = mc.dual_profile;
    state.manualHideMouth            = mc.hide_mouth;

    g_override_teeth           = (mc.teeth >= 0)     ? mc.teeth     : 1;
    g_override_teeth_right     = -1;
    g_override_lip_color       = (mc.lip_color >= 0) ? mc.lip_color : 3;
    g_override_lip_color_right = -1;
    g_override_eyelash         = (mc.eyelash >= 0)   ? mc.eyelash   : 0;
    g_override_eye_style       = mc.eye_style;
    g_override_iris            = mc.iris;
    selectEyeBankForSide(0, mc.eye_bank);
    selectEyeBankForSide(1, (mc.eye_bank_right != -1 || mc.eye_bank == -1) ? mc.eye_bank_right : mc.eye_bank);

    if (updateAnim) {
        state.manualTestAnim = (mc.anim_id >= 0) ? mc.anim_id : fallbackAnim;
    }
}

// ------------------------------------------------------------------------------
// 21. Apply a saved memory slot to runtime layer indices and style overrides
// ------------------------------------------------------------------------------
static void applyMemoryComboToState(const FaceAppConfig& cfg, AppRuntimeState& state, int slot, bool updateAnim)
{
    auto it = cfg.memory_combos.find(slot);
    if (it == cfg.memory_combos.end() || !it->second.enabled) {
        if (slot == 0) {
            state.faceLayer.testIndex        = 1;
            state.makeupLayer.testIndex      = 0;
            state.earsLayer.testIndex        = 0;
            state.browsLayer.testIndex       = 1;
            state.noseLayer.testIndex        = 1;
            state.beardLayer.testIndex       = 0;
            state.glassesLayer.testIndex     = 0;
            state.hairLayer.testIndex        = 0;
            state.hatLayer.testIndex         = 0;
            state.dualFaceLayer.testIndex    = 0;
            state.dualMakeupLayer.testIndex  = 0;
            state.dualEarsLayer.testIndex    = 0;
            state.dualBrowsLayer.testIndex   = 0;
            state.dualNoseLayer.testIndex    = 0;
            state.dualBeardLayer.testIndex   = 0;
            state.dualGlassesLayer.testIndex = 0;
            state.dualHairLayer.testIndex    = 0;
            state.dualHatLayer.testIndex     = 0;
            state.dualFaceLayer.offsetX      = 0;
            state.dualFaceLayer.offsetY      = 0;
            state.dualFaceLayer.scale        = 1.0f;
            state.bgLayer.testIndex          = 0;
            state.manualFxMask               = 0;
            state.dualProfileMode            = false;
        }
        state.manualHideMouth      = false;
        g_override_teeth           = 1;
        g_override_teeth_right     = -1;
        g_override_lip_color       = 3;
        g_override_lip_color_right = -1;
        g_override_eyelash         = 0;
        g_override_eye_style       = -1;
        g_override_iris            = -1;
        selectEyeBankIndex(-1);
        if (updateAnim) {
            state.manualTestAnim = 0;
        }
        return;
    }
    applyPresetValuesToState(it->second, state, 0, updateAnim);
}

// ------------------------------------------------------------------------------
// 22. Return sorted vector of all enabled memory slot numbers for auto-show loop
// ------------------------------------------------------------------------------
static std::vector<int> getSavedSequentialMemorySlots(const FaceAppConfig& cfg)
{
    std::vector<int> slots;
    for (const auto& kv : cfg.memory_combos) {
        if (kv.second.enabled) {
            slots.push_back(kv.first);
        }
    }
    std::sort(slots.begin(), slots.end());
    return slots;
}
// ------------------------------------------------------------------------------
// 23. Resolve TAB calibration target index (0..13) for active Left/Right char
// ------------------------------------------------------------------------------
static void getEditableTarget(AppRuntimeState& state, int idx, const char*& outName, int*& outX, int*& outY, float*& outScale)
{
    bool rMode = (state.dualProfileMode && state.editingRightProfile);
    switch (idx) {
        case 0:  outName = "EYE_LEFT";  outX = &state.eyeLeftOffsetX;  outY = &state.eyeLeftOffsetY;  outScale = &state.eyeScale;   break;
        case 1:  outName = "EYE_RIGHT"; outX = &state.eyeRightOffsetX; outY = &state.eyeRightOffsetY; outScale = &state.eyeScale;   break;
        case 2:  outName = rMode ? "R_MOUTH"   : "MOUTH";   outX = rMode ? &state.rightMouthOffsetX        : &state.mouthOffsetX;           outY = rMode ? &state.rightMouthOffsetY        : &state.mouthOffsetY;           outScale = rMode ? &state.rightMouthScale          : &state.mouthScale;          break;
        case 3:  outName = rMode ? "R_NOSE"    : "NOSE";    outX = rMode ? &state.dualNoseLayer.offsetX    : &state.noseLayer.offsetX;    outY = rMode ? &state.dualNoseLayer.offsetY    : &state.noseLayer.offsetY;    outScale = rMode ? &state.dualNoseLayer.scale    : &state.noseLayer.scale;    break;
        case 4:  outName = rMode ? "R_GLASSES" : "GLASSES"; outX = rMode ? &state.dualGlassesLayer.offsetX : &state.glassesLayer.offsetX; outY = rMode ? &state.dualGlassesLayer.offsetY : &state.glassesLayer.offsetY; outScale = rMode ? &state.dualGlassesLayer.scale : &state.glassesLayer.scale; break;
        case 5:  outName = rMode ? "R_BROWS"   : "BROWS";   outX = rMode ? &state.dualBrowsLayer.offsetX   : &state.browsLayer.offsetX;   outY = rMode ? &state.dualBrowsLayer.offsetY   : &state.browsLayer.offsetY;   outScale = rMode ? &state.dualBrowsLayer.scale   : &state.browsLayer.scale;   break;
        case 6:  outName = rMode ? "R_BEARD"   : "BEARD";   outX = rMode ? &state.dualBeardLayer.offsetX   : &state.beardLayer.offsetX;   outY = rMode ? &state.dualBeardLayer.offsetY   : &state.beardLayer.offsetY;   outScale = rMode ? &state.dualBeardLayer.scale   : &state.beardLayer.scale;   break;
        case 7:  outName = rMode ? "R_FACE"    : "FACE";    outX = rMode ? &state.dualFaceLayer.offsetX    : &state.faceLayer.offsetX;    outY = rMode ? &state.dualFaceLayer.offsetY    : &state.faceLayer.offsetY;    outScale = rMode ? &state.dualFaceLayer.scale    : &state.faceLayer.scale;    break;
        case 8:  outName = rMode ? "R_EARS"    : "EARS";    outX = rMode ? &state.dualEarsLayer.offsetX    : &state.earsLayer.offsetX;    outY = rMode ? &state.dualEarsLayer.offsetY    : &state.earsLayer.offsetY;    outScale = rMode ? &state.dualEarsLayer.scale    : &state.earsLayer.scale;    break;
        case 9:  outName = rMode ? "R_HAIR"    : "HAIR";    outX = rMode ? &state.dualHairLayer.offsetX    : &state.hairLayer.offsetX;    outY = rMode ? &state.dualHairLayer.offsetY    : &state.hairLayer.offsetY;    outScale = rMode ? &state.dualHairLayer.scale    : &state.hairLayer.scale;    break;
        case 10: outName = rMode ? "R_HAT"     : "HAT";     outX = rMode ? &state.dualHatLayer.offsetX     : &state.hatLayer.offsetX;     outY = rMode ? &state.dualHatLayer.offsetY     : &state.hatLayer.offsetY;     outScale = rMode ? &state.dualHatLayer.scale     : &state.hatLayer.scale;     break;
        case 11: outName = rMode ? "R_MAKEUP"  : "MAKEUP";  outX = rMode ? &state.dualMakeupLayer.offsetX  : &state.makeupLayer.offsetX;  outY = rMode ? &state.dualMakeupLayer.offsetY  : &state.makeupLayer.offsetY;  outScale = rMode ? &state.dualMakeupLayer.scale  : &state.makeupLayer.scale;  break;
        case 12: outName = "BG";        outX = &state.bgLayer.offsetX; outY = &state.bgLayer.offsetY; outScale = &state.bgLayer.scale; break;
        default: outName = "FX";        outX = &state.fxLayer.offsetX; outY = &state.fxLayer.offsetY; outScale = &state.fxLayer.scale; break;
    }
}

// ------------------------------------------------------------------------------
// 24. Apply per-face alignment profile including splitOffsetX and layer scales
// ------------------------------------------------------------------------------
static void applyMaskOffsetProfile(const FaceAppConfig& cfg, AppRuntimeState& state, uint8_t maskId)
{
    state.activeProfileId = maskId;
    MaskOffsetProfile prof{};
    auto it = cfg.mask_profiles.find(maskId);
    if (it != cfg.mask_profiles.end()) {
        prof = it->second;
    } else if (maskId >= PROFILE_FACE_START_INDEX) {
        prof.face_split  = 75;
        prof.mouth_split = 75;
    } else {
        auto defIt = cfg.mask_profiles.find(0);
        if (defIt != cfg.mask_profiles.end()) {
            prof = defIt->second;
        }
    }

    if (maskId >= PROFILE_FACE_START_INDEX) {
        prof.eye_left_y = prof.eye_right_y = prof.face_y = prof.ears_y = 0;
        prof.nose_y = prof.brows_y = prof.beard_y = prof.glasses_y = 0;
        prof.hair_y = prof.hat_y = prof.makeup_y = prof.mouth_y = 0;
        if (prof.face_split == 0) {
            prof.face_split = 75;
        }
        if (prof.mouth_split == 0) {
            prof.mouth_split = prof.face_split;
        }
    }

    auto clampSc = [](float v) { return std::max(0.50f, std::min(1.60f, v)); };

    state.eyeLeftOffsetX            = prof.eye_left_x;
    state.eyeLeftOffsetY            = prof.eye_left_y;
    state.eyeRightOffsetX           = prof.eye_right_x;
    state.eyeRightOffsetY           = prof.eye_right_y;
    state.mouthOffsetX              = prof.mouth_x;
    state.mouthOffsetY              = prof.mouth_y;
    state.mouthSplitX               = prof.mouth_split;
    state.noseLayer.offsetX         = prof.nose_x;
    state.noseLayer.offsetY         = prof.nose_y;
    state.noseLayer.splitOffsetX    = prof.nose_split;
    state.noseLayer.scale           = clampSc(prof.nose_scale);
    state.glassesLayer.offsetX      = prof.glasses_x;
    state.glassesLayer.offsetY      = prof.glasses_y;
    state.glassesLayer.splitOffsetX = prof.glasses_split;
    state.glassesLayer.scale        = clampSc(prof.glasses_scale);
    state.browsLayer.offsetX        = prof.brows_x;
    state.browsLayer.offsetY        = prof.brows_y;
    state.browsLayer.splitOffsetX   = prof.brows_split;
    state.browsLayer.scale          = clampSc(prof.brows_scale);
    state.beardLayer.offsetX        = prof.beard_x;
    state.beardLayer.offsetY        = prof.beard_y;
    state.beardLayer.splitOffsetX   = prof.beard_split;
    state.beardLayer.scale          = clampSc(prof.beard_scale);
    state.faceLayer.offsetX         = prof.face_x;
    state.faceLayer.offsetY         = prof.face_y;
    state.faceLayer.splitOffsetX    = prof.face_split;
    state.faceLayer.scale           = clampSc(prof.face_scale);
    state.earsLayer.offsetX         = prof.ears_x;
    state.earsLayer.offsetY         = prof.ears_y;
    state.earsLayer.splitOffsetX    = prof.ears_split;
    state.earsLayer.scale           = clampSc(prof.ears_scale);
    state.hairLayer.offsetX         = prof.hair_x;
    state.hairLayer.offsetY         = prof.hair_y;
    state.hairLayer.scale           = clampSc(prof.hair_scale);
    state.hatLayer.offsetX          = prof.hat_x;
    state.hatLayer.offsetY          = prof.hat_y;
    state.hatLayer.scale            = clampSc(prof.hat_scale);
    state.makeupLayer.offsetX       = prof.makeup_x;
    state.makeupLayer.offsetY       = prof.makeup_y;
    state.makeupLayer.scale         = clampSc(prof.makeup_scale);
    state.bgLayer.offsetX           = prof.bg_x;
    state.bgLayer.offsetY           = prof.bg_y;
    state.bgLayer.scale             = clampSc(prof.bg_scale);
    state.fxLayer.offsetX           = prof.fx_x;
    state.fxLayer.offsetY           = prof.fx_y;
    state.fxLayer.scale             = clampSc(prof.fx_scale);
    state.eyeScale                  = clampSc(prof.eye_scale);
    state.mouthScale                = clampSc(prof.mouth_scale);

    g_eye_scale_factor.store(state.eyeScale, std::memory_order_relaxed);
    g_mouth_scale_factor.store(state.mouthScale, std::memory_order_relaxed);
    g_profile_face_split_x.store(state.faceLayer.splitOffsetX, std::memory_order_relaxed);
    g_profile_mouth_split_x.store(state.mouthSplitX, std::memory_order_relaxed);
}

// ------------------------------------------------------------------------------
// 25. Capture active runtime offsets, split distances and scales into a profile
// ------------------------------------------------------------------------------
static MaskOffsetProfile captureCurrentProfile(const AppRuntimeState& state)
{
    return {
        state.eyeLeftOffsetX, state.eyeLeftOffsetY, state.eyeRightOffsetX, state.eyeRightOffsetY,
        state.mouthOffsetX, state.mouthOffsetY, state.mouthSplitX,
        state.noseLayer.offsetX, state.noseLayer.offsetY, state.noseLayer.splitOffsetX,
        state.glassesLayer.offsetX, state.glassesLayer.offsetY, state.glassesLayer.splitOffsetX,
        state.browsLayer.offsetX, state.browsLayer.offsetY, state.browsLayer.splitOffsetX,
        state.beardLayer.offsetX, state.beardLayer.offsetY, state.beardLayer.splitOffsetX,
        state.faceLayer.offsetX, state.faceLayer.offsetY, state.faceLayer.splitOffsetX,
        state.earsLayer.offsetX, state.earsLayer.offsetY, state.earsLayer.splitOffsetX,
        state.hairLayer.offsetX, state.hairLayer.offsetY,
        state.hatLayer.offsetX, state.hatLayer.offsetY,
        state.makeupLayer.offsetX, state.makeupLayer.offsetY,
        state.bgLayer.offsetX, state.bgLayer.offsetY,
        state.fxLayer.offsetX, state.fxLayer.offsetY,
        state.eyeScale, state.mouthScale,
        state.noseLayer.scale, state.glassesLayer.scale, state.browsLayer.scale,
        state.beardLayer.scale, state.faceLayer.scale, state.earsLayer.scale,
        state.hairLayer.scale, state.hatLayer.scale, state.makeupLayer.scale,
        state.bgLayer.scale, state.fxLayer.scale
    };
}
// ------------------------------------------------------------------------------
// 26. Trim leading and trailing whitespace characters from an INI line
// ------------------------------------------------------------------------------
static std::string trimWhitespace(const std::string& s)
{
    size_t f = s.find_first_not_of(" \t\r\n");
    if (f == std::string::npos) {
        return "";
    }
    return s.substr(f, s.find_last_not_of(" \t\r\n") - f + 1);
}

// ------------------------------------------------------------------------------
// 27. Map one key=value pair into a MaskOffsetProfile via lookup tables
// ------------------------------------------------------------------------------
static void applyOffsetProfileKv(MaskOffsetProfile& p, const std::string& k, const std::string& v)
{
    static const std::map<std::string, float MaskOffsetProfile::*> kFloats = {
        {"eye_scale", &MaskOffsetProfile::eye_scale},         {"mouth_scale", &MaskOffsetProfile::mouth_scale},
        {"nose_scale", &MaskOffsetProfile::nose_scale},       {"glasses_scale", &MaskOffsetProfile::glasses_scale},
        {"brows_scale", &MaskOffsetProfile::brows_scale},     {"beard_scale", &MaskOffsetProfile::beard_scale},
        {"face_scale", &MaskOffsetProfile::face_scale},       {"ears_scale", &MaskOffsetProfile::ears_scale},
        {"hair_scale", &MaskOffsetProfile::hair_scale},       {"hat_scale", &MaskOffsetProfile::hat_scale},
        {"makeup_scale", &MaskOffsetProfile::makeup_scale},   {"bg_scale", &MaskOffsetProfile::bg_scale},
        {"fx_scale", &MaskOffsetProfile::fx_scale}
    };
    auto fit = kFloats.find(k);
    if (fit != kFloats.end()) {
        p.*(fit->second) = std::stof(v);
        return;
    }

    static const std::map<std::string, int MaskOffsetProfile::*> kInts = {
        {"eye_left_x", &MaskOffsetProfile::eye_left_x},       {"eye_left_y", &MaskOffsetProfile::eye_left_y},
        {"eye_right_x", &MaskOffsetProfile::eye_right_x},     {"eye_right_y", &MaskOffsetProfile::eye_right_y},
        {"mouth_x", &MaskOffsetProfile::mouth_x},             {"mouth_y", &MaskOffsetProfile::mouth_y},
        {"mouth_split", &MaskOffsetProfile::mouth_split},     {"nose_x", &MaskOffsetProfile::nose_x},
        {"nose_y", &MaskOffsetProfile::nose_y},               {"nose_split", &MaskOffsetProfile::nose_split},
        {"glasses_x", &MaskOffsetProfile::glasses_x},         {"glasses_y", &MaskOffsetProfile::glasses_y},
        {"glasses_split", &MaskOffsetProfile::glasses_split}, {"brows_x", &MaskOffsetProfile::brows_x},
        {"brows_y", &MaskOffsetProfile::brows_y},             {"brows_split", &MaskOffsetProfile::brows_split},
        {"beard_x", &MaskOffsetProfile::beard_x},             {"beard_y", &MaskOffsetProfile::beard_y},
        {"beard_split", &MaskOffsetProfile::beard_split},     {"face_x", &MaskOffsetProfile::face_x},
        {"face_y", &MaskOffsetProfile::face_y},               {"face_split", &MaskOffsetProfile::face_split},
        {"ears_x", &MaskOffsetProfile::ears_x},               {"ears_y", &MaskOffsetProfile::ears_y},
        {"ears_split", &MaskOffsetProfile::ears_split},       {"hair_x", &MaskOffsetProfile::hair_x},
        {"hair_y", &MaskOffsetProfile::hair_y},               {"hat_x", &MaskOffsetProfile::hat_x},
        {"hat_y", &MaskOffsetProfile::hat_y},                 {"makeup_x", &MaskOffsetProfile::makeup_x},
        {"makeup_y", &MaskOffsetProfile::makeup_y},           {"bg_x", &MaskOffsetProfile::bg_x},
        {"bg_y", &MaskOffsetProfile::bg_y},                   {"fx_x", &MaskOffsetProfile::fx_x},
        {"fx_y", &MaskOffsetProfile::fx_y}
    };
    auto iit = kInts.find(k);
    if (iit != kInts.end()) {
        p.*(iit->second) = std::stoi(v);
    }
}

// ------------------------------------------------------------------------------
// 28. Map one key=value pair into a MemoryPreset via member-pointer tables
// ------------------------------------------------------------------------------
static void applyMemoryPresetKv(MemoryPreset& mc, const std::string& k, const std::string& v)
{
    mc.enabled = true;
    if (k == "fx_mask") { mc.fx_mask = static_cast<uint32_t>(std::stoul(v)); return; }
    if (k == "right_face_scale")  { mc.right_face_scale  = std::stof(v); return; }
    if (k == "right_mouth_scale") { mc.right_mouth_scale = std::stof(v); return; }

    int iv = std::stoi(v);
    if (k == "dual_profile") { mc.dual_profile = (iv != 0); return; }
    if (k == "hide_mouth")   { mc.hide_mouth   = (iv != 0); return; }
    if (k == "eye_bank") {
        mc.eye_bank = iv;
        if (mc.eye_bank_right == -1) mc.eye_bank_right = iv;
        return;
    }

    static const std::map<std::string, uint8_t MemoryPreset::*> kU8 = {
        {"face", &MemoryPreset::face},                   {"makeup", &MemoryPreset::makeup},
        {"ears", &MemoryPreset::ears},                   {"nose", &MemoryPreset::nose},
        {"beard", &MemoryPreset::beard},                 {"glasses", &MemoryPreset::glasses},
        {"hair", &MemoryPreset::hair},                   {"hat", &MemoryPreset::hat},
        {"brows", &MemoryPreset::brows},                 {"right_face", &MemoryPreset::right_face},
        {"right_makeup", &MemoryPreset::right_makeup},   {"right_ears", &MemoryPreset::right_ears},
        {"right_nose", &MemoryPreset::right_nose},       {"right_beard", &MemoryPreset::right_beard},
        {"right_glasses", &MemoryPreset::right_glasses}, {"right_hair", &MemoryPreset::right_hair},
        {"right_hat", &MemoryPreset::right_hat},         {"right_brows", &MemoryPreset::right_brows},
        {"bg_scene", &MemoryPreset::bg_scene}
    };
    auto u8It = kU8.find(k);
    if (u8It != kU8.end()) { mc.*(u8It->second) = static_cast<uint8_t>(iv); return; }

    static const std::map<std::string, int MemoryPreset::*> kInts = {
        {"right_face_x", &MemoryPreset::right_face_x},   {"right_face_y", &MemoryPreset::right_face_y},
        {"right_mouth_x", &MemoryPreset::right_mouth_x}, {"right_mouth_y", &MemoryPreset::right_mouth_y},
        {"teeth", &MemoryPreset::teeth},                 {"lip_color", &MemoryPreset::lip_color},
        {"eyelash", &MemoryPreset::eyelash},             {"eye_style", &MemoryPreset::eye_style},
        {"iris", &MemoryPreset::iris},                   {"eye_bank_right", &MemoryPreset::eye_bank_right},
        {"anim_id", &MemoryPreset::anim_id}
    };
    auto iIt = kInts.find(k);
    if (iIt != kInts.end()) { mc.*(iIt->second) = iv; }
}

// ------------------------------------------------------------------------------
// 29. Map one global [network] / [display] / [paths] key=value pair
// ------------------------------------------------------------------------------
static void applyGlobalConfigKv(FaceAppConfig& cfg, const std::string& k, const std::string& v)
{
    if (k == "proto")              cfg.proto = v;
    else if (k == "universe")      cfg.universe = static_cast<uint16_t>(std::stoi(v));
    else if (k == "channel" || k == "start_channel") cfg.start_channel = static_cast<uint16_t>(std::stoi(v));
    else if (k == "mode" || k == "dmx_mode") {
        int m = std::stoi(v);
        if (m == 1 || m == 2 || m == 7 || m == 39) cfg.dmx_mode = m;
    }
    else if (k == "fps")           cfg.fps = std::stoi(v);
    else if (k == "display")       cfg.display = std::stoi(v);
    else if (k == "fullscreen")    cfg.fullscreen = (v == "1" || v == "true");
    else if (k == "smooth" || k == "smooth_scale") cfg.smooth_scale = (v == "1" || v == "true");
    else if (k == "fade_ms")       cfg.fade_ms = std::max(0, std::min(4000, std::stoi(v)));
    else if (k == "save_hold_ms")  cfg.save_hold_ms = std::max(0, std::min(60000, std::stoi(v)));
    else if (k == "audio_dir")     cfg.audio_dir = v;
    else if (k == "assets_dir")    cfg.assets_dir = v;
    else if (k == "audio_monitor") cfg.audio_monitor = v;
}

// ------------------------------------------------------------------------------
// 30. Parse makingfaces.ini into network, display, offset profiles & memories
// ------------------------------------------------------------------------------
static void loadIniConfig(const std::string& path, FaceAppConfig& cfg)
{
    std::ifstream in(path);
    if (!in.is_open()) {
        return;
    }

    std::string sec;
    std::string line;
    uint8_t mSec = 0;
    int memSec   = -1;

    while (std::getline(in, line)) {
        std::string c = trimWhitespace(line);
        if (c.empty() || c.front() == '#' || c.front() == ';') {
            continue;
        }
        if (c.front() == '[' && c.back() == ']') {
            sec    = c.substr(1, c.size() - 2);
            mSec   = (sec.rfind("offsets_face_", 0) == 0) ? static_cast<uint8_t>(std::stoi(sec.substr(13))) : 0;
            memSec = (sec.rfind("memory_", 0) == 0) ? std::stoi(sec.substr(7))
                   : ((sec.rfind("macro_combo_", 0) == 0) ? std::stoi(sec.substr(12)) : -1);
            continue;
        }
        size_t eq = c.find('=');
        if (eq == std::string::npos) {
            continue;
        }
        std::string k = trimWhitespace(c.substr(0, eq));
        std::string v = trimWhitespace(c.substr(eq + 1));

        try {
            if (sec == "offsets" || sec == "offsets_default" || sec.rfind("offsets_face_", 0) == 0) {
                applyOffsetProfileKv(cfg.mask_profiles[mSec], k, v);
            } else if (memSec >= 0 && memSec <= MAX_MEMORY_SLOT) {
                applyMemoryPresetKv(cfg.memory_combos[memSec], k, v);
            } else {
                applyGlobalConfigKv(cfg, k, v);
            }
        } catch (...) {
        }
    }
}

// ------------------------------------------------------------------------------
// 31. Persist network, display, offset/split/scale profiles and memories to INI
// ------------------------------------------------------------------------------
static bool saveIniConfig(SDL_Window* window, FaceAppConfig& cfg, const AppRuntimeState& state)
{
    if (window) {
        int curDisp = SDL_GetWindowDisplayIndex(window);
        if (curDisp >= 0) {
            cfg.display = curDisp;
        }
    }
    cfg.mask_profiles[state.activeProfileId] = captureCurrentProfile(state);

    std::ofstream out(cfg.config_file);
    if (!out.is_open()) {
        return false;
    }

    out << "[network]\nproto=" << cfg.proto << "\nuniverse=" << cfg.universe << "\nstart_channel=" << cfg.start_channel << "\ndmx_mode=" << cfg.dmx_mode
        << "\n\n[display]\ndisplay=" << cfg.display << "\nfps=" << cfg.fps
        << "\nfullscreen=" << (state.isFullscreen ? "true" : "false")
        << "\nsmooth_scale=" << (cfg.smooth_scale ? "true" : "false")
        << "\nfade_ms=" << cfg.fade_ms << "\nsave_hold_ms=" << cfg.save_hold_ms
        << "\n\n[paths]\nassets_dir=" << cfg.assets_dir << "\naudio_dir=" << cfg.audio_dir << "\naudio_monitor=" << cfg.audio_monitor << "\n";

    for (const auto& kv : cfg.mask_profiles) {
        std::array<char, 32> sec{};
        if (kv.first == 0) {
            std::snprintf(sec.data(), sec.size(), "offsets_default");
        } else {
            std::snprintf(sec.data(), sec.size(), "offsets_face_%03u", kv.first);
        }
        const auto& p = kv.second;
        out << "\n[" << sec.data() << "]\neye_left_x=" << p.eye_left_x << "\neye_left_y=" << p.eye_left_y
            << "\neye_right_x=" << p.eye_right_x << "\neye_right_y=" << p.eye_right_y
            << "\nmouth_x=" << p.mouth_x << "\nmouth_y=" << p.mouth_y << "\nmouth_split=" << p.mouth_split
            << "\nnose_x=" << p.nose_x << "\nnose_y=" << p.nose_y << "\nnose_split=" << p.nose_split
            << "\nglasses_x=" << p.glasses_x << "\nglasses_y=" << p.glasses_y << "\nglasses_split=" << p.glasses_split
            << "\nbrows_x=" << p.brows_x << "\nbrows_y=" << p.brows_y << "\nbrows_split=" << p.brows_split
            << "\nbeard_x=" << p.beard_x << "\nbeard_y=" << p.beard_y << "\nbeard_split=" << p.beard_split
            << "\nface_x=" << p.face_x << "\nface_y=" << p.face_y << "\nface_split=" << p.face_split
            << "\nears_x=" << p.ears_x << "\nears_y=" << p.ears_y << "\nears_split=" << p.ears_split
            << "\nhair_x=" << p.hair_x << "\nhair_y=" << p.hair_y << "\nhat_x=" << p.hat_x << "\nhat_y=" << p.hat_y
            << "\nmakeup_x=" << p.makeup_x << "\nmakeup_y=" << p.makeup_y
            << "\nbg_x=" << p.bg_x << "\nbg_y=" << p.bg_y << "\nfx_x=" << p.fx_x << "\nfx_y=" << p.fx_y
            << "\neye_scale=" << p.eye_scale << "\nmouth_scale=" << p.mouth_scale
            << "\nnose_scale=" << p.nose_scale << "\nglasses_scale=" << p.glasses_scale
            << "\nbrows_scale=" << p.brows_scale << "\nbeard_scale=" << p.beard_scale
            << "\nface_scale=" << p.face_scale << "\nears_scale=" << p.ears_scale
            << "\nhair_scale=" << p.hair_scale << "\nhat_scale=" << p.hat_scale
            << "\nmakeup_scale=" << p.makeup_scale << "\nbg_scale=" << p.bg_scale << "\nfx_scale=" << p.fx_scale << "\n";
    }

    for (const auto& kv : cfg.memory_combos) {
        if (!kv.second.enabled) {
            continue;
        }
        const auto& mc = kv.second;
        out << "\n[memory_" << kv.first << "]\nface=" << (int)mc.face << "\nmakeup=" << (int)mc.makeup
            << "\nears=" << (int)mc.ears << "\nnose=" << (int)mc.nose << "\nbeard=" << (int)mc.beard
            << "\nglasses=" << (int)mc.glasses << "\nhair=" << (int)mc.hair << "\nhat=" << (int)mc.hat
            << "\nbrows=" << (int)mc.brows
            << "\nright_face=" << (int)mc.right_face << "\nright_makeup=" << (int)mc.right_makeup
            << "\nright_ears=" << (int)mc.right_ears << "\nright_nose=" << (int)mc.right_nose
            << "\nright_beard=" << (int)mc.right_beard << "\nright_glasses=" << (int)mc.right_glasses
            << "\nright_hair=" << (int)mc.right_hair << "\nright_hat=" << (int)mc.right_hat
            << "\nright_brows=" << (int)mc.right_brows
            << "\nright_face_x=" << mc.right_face_x << "\nright_face_y=" << mc.right_face_y
            << "\nright_face_scale=" << mc.right_face_scale
            << "\nright_mouth_x=" << mc.right_mouth_x << "\nright_mouth_y=" << mc.right_mouth_y
            << "\nright_mouth_scale=" << mc.right_mouth_scale
            << "\nbg_scene=" << (int)mc.bg_scene << "\nfx_mask=" << mc.fx_mask
            << "\nteeth=" << mc.teeth << "\nlip_color=" << mc.lip_color << "\neyelash=" << mc.eyelash
            << "\neye_style=" << mc.eye_style << "\niris=" << mc.iris
            << "\neye_bank=" << mc.eye_bank << "\neye_bank_right=" << mc.eye_bank_right
            << "\nanim_id=" << mc.anim_id << "\ndual_profile=" << (mc.dual_profile ? 1 : 0)
            << "\nhide_mouth=" << (mc.hide_mouth ? 1 : 0) << "\n";
    }
    return true;
}

// ==============================================================================
// Section E. ZIP Pack Browser, Live Inspector, Selective Import & 5-Step Undo
// ==============================================================================

// ------------------------------------------------------------------------------
// 32. Archive current assets/ and makingfaces.ini into packs/<name>.zip
// ------------------------------------------------------------------------------
static bool saveCurrentAssetsToZipPack(const std::string& packName, const std::string& assetsDir, const std::string& iniFile, const std::string& packsDir = "packs")
{
    if (!fs::exists(assetsDir) || !fs::exists(iniFile)) {
        return false;
    }
    fs::create_directories(packsDir);
    fs::path zipPath = fs::path(packsDir) / (packName + ".zip");
    if (fs::exists(zipPath)) {
        fs::remove(zipPath);
    }

    std::string cmd = "zip -rq \"" + zipPath.string() + "\" \"" + assetsDir + "\" \"" + iniFile + "\"";
    int rc = std::system(cmd.c_str());
    return (rc == 0 && fs::exists(zipPath));
}

// ------------------------------------------------------------------------------
// 33. Rotate 5-level undo snapshots (.undo_1..5.zip) before modifying default.zip
// ------------------------------------------------------------------------------
static void pushUndoSnapshot(const std::string& assetsDir, const std::string& iniFile, const std::string& packsDir = "packs")
{
    fs::create_directories(packsDir);
    fs::path oldest = fs::path(packsDir) / (".undo_" + std::to_string(MAX_UNDO_LEVELS) + ".zip");
    if (fs::exists(oldest)) {
        fs::remove(oldest);
    }
    for (int i = MAX_UNDO_LEVELS - 1; i >= 1; --i) {
        fs::path src = fs::path(packsDir) / (".undo_" + std::to_string(i) + ".zip");
        fs::path dst = fs::path(packsDir) / (".undo_" + std::to_string(i + 1) + ".zip");
        if (fs::exists(src)) {
            fs::rename(src, dst);
        }
    }
    fs::path defZip = fs::path(packsDir) / "default.zip";
    fs::path u1 = fs::path(packsDir) / ".undo_1.zip";
    if (fs::exists(defZip)) {
        fs::copy_file(defZip, u1, fs::copy_options::overwrite_existing);
    } else {
        saveCurrentAssetsToZipPack(".undo_1", assetsDir, iniFile, packsDir);
    }
}

// ------------------------------------------------------------------------------
// 34. Generate next unused custom_NNN pack name in packs/ directory
// ------------------------------------------------------------------------------
static std::string nextCustomZipPackName(const std::string& packsDir = "packs")
{
    for (int idx = 1; idx <= 999; ++idx) {
        std::array<char, 32> buf{};
        std::snprintf(buf.data(), buf.size(), "custom_%03d", idx);
        fs::path candidate = fs::path(packsDir) / (std::string(buf.data()) + ".zip");
        if (!fs::exists(candidate)) {
            return std::string(buf.data());
        }
    }
    return "custom_999";
}

// ------------------------------------------------------------------------------
// 35. Free all cached thumbnail textures in the Pack Browser
// ------------------------------------------------------------------------------
static void destroyPackBrowserEntries(PackBrowserState& pb)
{
    for (auto& e : pb.entries) {
        if (e.thumbTex) {
            SDL_DestroyTexture(e.thumbTex);
            e.thumbTex = nullptr;
        }
    }
    pb.entries.clear();
}
// ------------------------------------------------------------------------------
// 36. Scan packs/*.zip and render composite preview thumbnails (bg + full char)
// ------------------------------------------------------------------------------
static void refreshPackBrowserEntries(SDL_Renderer* ren, PackBrowserState& pb, const std::string& packsDir = "packs")
{
    destroyPackBrowserEntries(pb);
    if (!fs::exists(packsDir) || !fs::is_directory(packsDir)) {
        return;
    }

    std::vector<fs::path> zips;
    for (const auto& entry : fs::directory_iterator(packsDir)) {
        if (!entry.is_regular_file() || entry.path().extension() != ".zip") {
            continue;
        }
        std::string stem = entry.path().stem().string();
        if (!stem.empty() && stem.front() == '.') {
            continue;
        }
        zips.push_back(entry.path());
    }
    std::sort(zips.begin(), zips.end());

    fs::path tmpDir = fs::path(packsDir) / ".thumb_tmp";
    static const std::array<const char*, 9> kCharLayers = {
        "ears", "face", "makeup", "nose", "brows", "beard", "glasses", "hair", "hat"
    };

    for (const auto& zp : zips) {
        if (fs::exists(tmpDir)) {
            fs::remove_all(tmpDir);
        }
        fs::create_directories(tmpDir);
        std::string cmd = "unzip -oq \"" + zp.string() + "\" -d \"" + tmpDir.string() + "\"";
        if (std::system(cmd.c_str()) != 0) {
            continue;
        }

        PackBrowserEntry item{};
        item.name = zp.stem().string();
        FaceAppConfig tmpCfg{};
        loadIniConfig((tmpDir / "makingfaces.ini").string(), tmpCfg);
        for (const auto& kv : tmpCfg.memory_combos) {
            if (kv.second.enabled) {
                ++item.memoryCount;
            }
        }

        fs::path faceDir = tmpDir / "assets" / "face";
        if (fs::exists(faceDir)) {
            for (const auto& f : fs::directory_iterator(faceDir)) {
                if (f.is_regular_file() && f.path().extension() == ".png") {
                    ++item.faceCount;
                }
            }
        }

        SDL_Texture* target = SDL_CreateTexture(ren, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_TARGET, 300, 400);
        if (target) {
            SDL_SetRenderTarget(ren, target);
            SDL_SetRenderDrawColor(ren, 16, 22, 34, 255);
            SDL_RenderClear(ren);
            auto blit = [&](const fs::path& p) {
                if (!fs::exists(p)) {
                    return;
                }
                SDL_Texture* t = IMG_LoadTexture(ren, p.string().c_str());
                if (t) {
                    SDL_RenderCopy(ren, t, nullptr, nullptr);
                    SDL_DestroyTexture(t);
                }
            };
            blit(tmpDir / "assets" / "bg" / "001_sky.png");
            blit(tmpDir / "assets" / "bg" / "001_far.png");
            blit(tmpDir / "assets" / "bg" / "001_mid.png");
            blit(tmpDir / "assets" / "bg" / "001.png");
            for (const char* lyr : kCharLayers) {
                blit(tmpDir / "assets" / lyr / "001.png");
            }
            blit(tmpDir / "assets" / "bg" / "001_front.png");
            SDL_SetRenderTarget(ren, nullptr);
            item.thumbTex = target;
        }
        pb.entries.push_back(item);
    }
    if (fs::exists(tmpDir)) {
        fs::remove_all(tmpDir);
    }
    if (pb.selectedIdx >= static_cast<int>(pb.entries.size())) {
        pb.selectedIdx = 0;
    }
}

// ------------------------------------------------------------------------------
// 37. Clear cached SDL layer textures and reset active runtime state to #000
// ------------------------------------------------------------------------------
static void reloadAllTexturesAndState(SDL_Renderer* ren, FaceAppConfig& cfg, AppRuntimeState& state)
{
    std::array<PaperDollLayer*, 19> layers = {
        &state.bgLayer, &state.faceLayer, &state.makeupLayer, &state.earsLayer, &state.browsLayer,
        &state.noseLayer, &state.beardLayer, &state.glassesLayer, &state.hairLayer, &state.hatLayer,
        &state.dualFaceLayer, &state.dualMakeupLayer, &state.dualEarsLayer, &state.dualBrowsLayer,
        &state.dualNoseLayer, &state.dualBeardLayer, &state.dualGlassesLayer, &state.dualHairLayer, &state.dualHatLayer
    };
    for (auto* l : layers) {
        if (l->texture) {
            SDL_DestroyTexture(l->texture);
            l->texture = nullptr;
        }
        l->loadedIndex = 0;
    }
    g_stage.clearActors();
    g_fog_back_actor  = nullptr;
    g_fog_mid_actor   = nullptr;
    g_fog_front_actor = nullptr;
    g_cloud_actor     = nullptr;
    g_ufo_actor       = nullptr;
    g_nose_fly_actor  = nullptr;
    g_spider_actor    = nullptr;
    g_snake_actor     = nullptr;
    g_lightning_actor = nullptr;
    g_fire_actor      = nullptr;
    g_cobweb_actor    = nullptr;
    if (ren) {
        destroyParallaxBackdrop(state.parallaxBg);
        destroyFxSpriteBank(state.fxBank);
        initFxSpriteBank(ren, cfg.assets_dir, state.fxBank);
    }
    reloadDynamicEyeBank(cfg.assets_dir);
    resetAutonomousShow();
    state.manualMemorySlot  = -1;
    state.browseMemorySlot  = 0;
    state.appliedMemorySlot = -1;
    state.manualTestAnim    = 0;
    applyMemoryComboToState(cfg, state, 0, false);
    applyMaskOffsetProfile(cfg, state, state.faceLayer.testIndex);
}

// ------------------------------------------------------------------------------
// 38. Find highest existing index in assets/<folder> for front (<127) or profile
// ------------------------------------------------------------------------------
static int findMaxAssetSlotInDir(const fs::path& dir, bool profileRange)
{
    int maxId = profileRange ? (PROFILE_FACE_START_INDEX - 1) : 0;
    if (!fs::exists(dir) || !fs::is_directory(dir)) {
        return maxId;
    }
    for (const auto& entry : fs::directory_iterator(dir)) {
        std::string stem = entry.path().stem().string();
        if (stem.size() >= 3 && std::isdigit(stem.at(0)) && std::isdigit(stem.at(1)) && std::isdigit(stem.at(2))) {
            int id = std::stoi(stem.substr(0, 3));
            if (!profileRange && id < PROFILE_FACE_START_INDEX) {
                maxId = std::max(maxId, id);
            }
            if (profileRange && id >= PROFILE_FACE_START_INDEX) {
                maxId = std::max(maxId, id);
            }
        }
    }
    return maxId;
}

// ------------------------------------------------------------------------------
// 39. Unpack selected .zip into packs/.inspect_tmp and populate inspectItems list
// ------------------------------------------------------------------------------
static bool openZipPackInspector(const std::string& packName, PackBrowserState& pb, const std::string& packsDir = "packs")
{
    fs::path zipPath = fs::path(packsDir) / (packName + ".zip");
    if (!fs::exists(zipPath)) {
        return false;
    }

    fs::path tmpDir = fs::path(packsDir) / ".inspect_tmp";
    if (fs::exists(tmpDir)) {
        fs::remove_all(tmpDir);
    }
    fs::create_directories(tmpDir);
    std::string cmd = "unzip -oq \"" + zipPath.string() + "\" -d \"" + tmpDir.string() + "\"";
    if (std::system(cmd.c_str()) != 0) {
        return false;
    }

    FaceAppConfig tmpCfg{};
    loadIniConfig((tmpDir / "makingfaces.ini").string(), tmpCfg);
    pb.inspectItems.clear();
    for (const auto& kv : tmpCfg.memory_combos) {
        if (!kv.second.enabled) {
            continue;
        }
        PackInspectItem item{};
        item.srcSlot  = kv.first;
        item.selected = true;
        item.preset   = kv.second;
        std::array<char, 84> lbl{};
        std::snprintf(lbl.data(), lbl.size(), "#%03d %s [F:%03u BG:%03u%s]",
                      kv.first, getAnimationName(kv.second.anim_id),
                      kv.second.face, kv.second.bg_scene,
                      kv.second.dual_profile ? " DUAL" : "");
        item.label = lbl.data();
        pb.inspectItems.push_back(item);
    }
    if (pb.inspectItems.empty()) {
        return false;
    }
    pb.inspectMode = true;
    pb.inspectIdx  = 0;
    return true;
}

// ------------------------------------------------------------------------------
// 40. Copy selected memory items and their referenced PNGs from .inspect_tmp
// ------------------------------------------------------------------------------
static bool appendCheckedItemsFromZipPack(SDL_Window* window, const std::string& packName, const PackBrowserState& pb, FaceAppConfig& cfg, AppRuntimeState& state, const std::string& packsDir = "packs")
{
    fs::path tmpDir = fs::path(packsDir) / ".inspect_tmp";
    if (!fs::exists(tmpDir)) {
        fs::path zipPath = fs::path(packsDir) / (packName + ".zip");
        if (!fs::exists(zipPath)) {
            return false;
        }
        fs::create_directories(tmpDir);
        std::string cmd = "unzip -oq \"" + zipPath.string() + "\" -d \"" + tmpDir.string() + "\"";
        if (std::system(cmd.c_str()) != 0) {
            return false;
        }
    }

    std::vector<MemoryPreset> chosen;
    for (const auto& it : pb.inspectItems) {
        if (it.selected) {
            chosen.push_back(it.preset);
        }
    }
    if (chosen.empty()) {
        return false;
    }

    pushUndoSnapshot(cfg.assets_dir, cfg.config_file, packsDir);
    static const std::array<const char*, 10> kFolders = {"face", "makeup", "ears", "nose", "brows", "beard", "glasses", "hair", "hat", "bg"};
    std::map<std::string, std::map<int, int>> remap;
    std::map<std::string, int> nextFront;
    std::map<std::string, int> nextProfOdd;

    for (const char* f : kFolders) {
        fs::path dstDir = fs::path(cfg.assets_dir) / f;
        fs::create_directories(dstDir);
        nextFront[f] = findMaxAssetSlotInDir(dstDir, false) + 1;
        int mp = findMaxAssetSlotInDir(dstDir, true);
        nextProfOdd[f] = (mp < PROFILE_FACE_START_INDEX) ? PROFILE_FACE_START_INDEX : ((mp % 2 == 1) ? (mp + 2) : (mp + 1));
    }

    auto copyAssetSlot = [&](const char* folder, uint8_t oldId) -> uint8_t {
        if (oldId == 0) {
            return 0;
        }
        if (remap[folder].count(oldId)) {
            return static_cast<uint8_t>(remap[folder][oldId]);
        }

        fs::path srcDir = tmpDir / "assets" / folder;
        fs::path dstDir = fs::path(cfg.assets_dir) / folder;
        bool isProf = (oldId >= PROFILE_FACE_START_INDEX && std::strcmp(folder, "bg") != 0);
        int oddBase = isProf ? ((oldId % 2 == 1) ? oldId : (oldId - 1)) : oldId;

        if (isProf) {
            int dstOdd  = std::min(253, nextProfOdd[folder]);
            int dstEven = std::min(254, dstOdd + 1);
            remap[folder][oddBase]     = dstOdd;
            remap[folder][oddBase + 1] = dstEven;
            if (nextProfOdd[folder] <= 253) nextProfOdd[folder] += 2;
        } else {
            remap[folder][oldId] = std::min(255, nextFront[folder]++);
        }

        if (fs::exists(srcDir)) {
            for (const auto& fp : fs::directory_iterator(srcDir)) {
                if (!fp.is_regular_file()) {
                    continue;
                }
                std::string stem = fp.path().stem().string();
                if (stem.size() < 3 || !std::isdigit(stem.at(0)) || !std::isdigit(stem.at(1)) || !std::isdigit(stem.at(2))) {
                    continue;
                }
                int fid = std::stoi(stem.substr(0, 3));
                if (remap[folder].count(fid)) {
                    std::array<char, 64> nn{};
                    std::snprintf(nn.data(), nn.size(), "%03d%s%s", remap[folder][fid], stem.substr(3).c_str(), fp.path().extension().string().c_str());
                    fs::copy_file(fp.path(), dstDir / nn.data(), fs::copy_options::overwrite_existing);
                }
            }
        }
        return static_cast<uint8_t>(remap[folder][oldId]);
    };

    FaceAppConfig incCfg{};
    loadIniConfig((tmpDir / "makingfaces.ini").string(), incCfg);
    int nextMemory = 0;
    for (const auto& kv : cfg.memory_combos) {
        if (kv.second.enabled && kv.first >= nextMemory) {
            nextMemory = kv.first + 1;
        }
    }

    for (auto mc : chosen) {
        if (nextMemory > MAX_MEMORY_SLOT) {
            break;
        }
        uint8_t origFace = mc.face;
        mc.face          = copyAssetSlot("face", mc.face);
        mc.makeup        = copyAssetSlot("makeup", mc.makeup);
        mc.ears          = copyAssetSlot("ears", mc.ears);
        mc.nose          = copyAssetSlot("nose", mc.nose);
        mc.brows         = copyAssetSlot("brows", mc.brows);
        mc.beard         = copyAssetSlot("beard", mc.beard);
        mc.glasses       = copyAssetSlot("glasses", mc.glasses);
        mc.hair          = copyAssetSlot("hair", mc.hair);
        mc.hat           = copyAssetSlot("hat", mc.hat);
        mc.right_face    = copyAssetSlot("face", mc.right_face);
        mc.right_makeup  = copyAssetSlot("makeup", mc.right_makeup);
        mc.right_ears    = copyAssetSlot("ears", mc.right_ears);
        mc.right_nose    = copyAssetSlot("nose", mc.right_nose);
        mc.right_beard   = copyAssetSlot("beard", mc.right_beard);
        mc.right_glasses = copyAssetSlot("glasses", mc.right_glasses);
        mc.right_hair    = copyAssetSlot("hair", mc.right_hair);
        mc.right_hat     = copyAssetSlot("hat", mc.right_hat);
        mc.right_brows   = copyAssetSlot("brows", mc.right_brows);
        mc.bg_scene      = copyAssetSlot("bg", mc.bg_scene);
        if (origFace > 0 && incCfg.mask_profiles.count(origFace)) {
            cfg.mask_profiles[mc.face] = incCfg.mask_profiles.at(origFace);
        }
        cfg.memory_combos[nextMemory++] = mc;
    }

    if (fs::exists(tmpDir)) {
        fs::remove_all(tmpDir);
    }
    saveIniConfig(window, cfg, state);
    saveCurrentAssetsToZipPack("default", cfg.assets_dir, cfg.config_file, packsDir);
    reloadAllTexturesAndState(SDL_GetRenderer(window), cfg, state);
    return true;
}

// ------------------------------------------------------------------------------
// 41. Append an entire external .zip theme pack onto the end of active assets
// ------------------------------------------------------------------------------
static bool appendZipPackToActive(SDL_Window* window, const std::string& packName, FaceAppConfig& cfg, AppRuntimeState& state, const std::string& packsDir = "packs")
{
    PackBrowserState tmpPb{};
    if (!openZipPackInspector(packName, tmpPb, packsDir)) {
        return false;
    }
    return appendCheckedItemsFromZipPack(window, packName, tmpPb, cfg, state, packsDir);
}

// ------------------------------------------------------------------------------
// 42. Replace active theme with selected .zip pack and update packs/default.zip
// ------------------------------------------------------------------------------
static bool replaceActiveWithZipPack(SDL_Window* window, const std::string& packName, FaceAppConfig& cfg, AppRuntimeState& state, const std::string& packsDir = "packs")
{
    fs::path zipPath = fs::path(packsDir) / (packName + ".zip");
    if (!fs::exists(zipPath) || fs::file_size(zipPath) == 0) {
        return false;
    }

    pushUndoSnapshot(cfg.assets_dir, cfg.config_file, packsDir);
    if (fs::exists(cfg.assets_dir)) {
        fs::remove_all(cfg.assets_dir);
    }
    if (fs::exists(cfg.config_file)) {
        fs::remove(cfg.config_file);
    }

    std::string cmd = "unzip -oq \"" + zipPath.string() + "\" -d .";
    if (std::system(cmd.c_str()) != 0) {
        return false;
    }

    cfg.memory_combos.clear();
    cfg.mask_profiles.clear();
    loadIniConfig(cfg.config_file, cfg);
    saveCurrentAssetsToZipPack("default", cfg.assets_dir, cfg.config_file, packsDir);
    reloadAllTexturesAndState(SDL_GetRenderer(window), cfg, state);
    return true;
}

// ------------------------------------------------------------------------------
// 43. Restore latest undo snapshot (.undo_1.zip) and shift undo stack forward
// ------------------------------------------------------------------------------
static bool restoreUndoSnapshot(SDL_Window* window, FaceAppConfig& cfg, AppRuntimeState& state, const std::string& packsDir = "packs")
{
    fs::path u1 = fs::path(packsDir) / ".undo_1.zip";
    if (!fs::exists(u1) || fs::file_size(u1) == 0) {
        return false;
    }

    if (fs::exists(cfg.assets_dir)) {
        fs::remove_all(cfg.assets_dir);
    }
    if (fs::exists(cfg.config_file)) {
        fs::remove(cfg.config_file);
    }
    std::string cmd = "unzip -oq \"" + u1.string() + "\" -d .";
    if (std::system(cmd.c_str()) != 0) {
        return false;
    }

    fs::remove(u1);
    for (int i = 2; i <= MAX_UNDO_LEVELS; ++i) {
        fs::path src = fs::path(packsDir) / (".undo_" + std::to_string(i) + ".zip");
        fs::path dst = fs::path(packsDir) / (".undo_" + std::to_string(i - 1) + ".zip");
        if (fs::exists(src)) {
            fs::rename(src, dst);
        }
    }

    cfg.memory_combos.clear();
    cfg.mask_profiles.clear();
    loadIniConfig(cfg.config_file, cfg);
    saveCurrentAssetsToZipPack("default", cfg.assets_dir, cfg.config_file, packsDir);
    reloadAllTexturesAndState(SDL_GetRenderer(window), cfg, state);
    return true;
}
// ------------------------------------------------------------------------------
// 44. Wipe all character/background assets and memories while preserving FX
// ------------------------------------------------------------------------------
static void wipeAllAssetsAndMacros(SDL_Window* window, FaceAppConfig& cfg, AppRuntimeState& state, const std::string& packsDir = "packs")
{
    pushUndoSnapshot(cfg.assets_dir, cfg.config_file, packsDir);
    static const std::array<const char*, 10> kClearDirs = {"face", "makeup", "ears", "nose", "brows", "beard", "glasses", "hair", "hat", "bg"};
    for (const char* d : kClearDirs) {
        fs::path p = fs::path(cfg.assets_dir) / d;
        if (fs::exists(p)) {
            fs::remove_all(p);
        }
        fs::create_directories(p);
    }
    cfg.memory_combos.clear();
    MaskOffsetProfile defProf = cfg.mask_profiles.count(0) ? cfg.mask_profiles.at(0) : MaskOffsetProfile{};
    cfg.mask_profiles.clear();
    cfg.mask_profiles[0] = defProf;

    saveIniConfig(window, cfg, state);
    saveCurrentAssetsToZipPack("default", cfg.assets_dir, cfg.config_file, packsDir);
    reloadAllTexturesAndState(SDL_GetRenderer(window), cfg, state);
}

// ==============================================================================
// Section F. Built-In Calibration & Import Studio (Cutout, Telea & Microscope)
// ==============================================================================

// ------------------------------------------------------------------------------
// 45. Free all cached preview textures in Calibration & Import Studio
// ------------------------------------------------------------------------------
static void destroyFlatImportItems(FlatImportState& fi)
{
    for (auto& it : fi.items) {
        if (it.previewTex) {
            SDL_DestroyTexture(it.previewTex);
            it.previewTex = nullptr;
        }
    }
    fi.items.clear();
}

// ------------------------------------------------------------------------------
// 46. Flood-fill corner-sampled & white outer background to transparent BGRA
// ------------------------------------------------------------------------------
static cv::Mat stripWhiteStudioBackgroundCv(const cv::Mat& bgr, int tol)
{
    int h = bgr.rows;
    int w = bgr.cols;
    int bw = std::max(1, std::min(8, w / 8));
    int bh = std::max(1, std::min(8, h / 8));
    cv::Scalar cTL = cv::mean(bgr(cv::Rect(0, 0, bw, bh)));
    cv::Scalar cTR = cv::mean(bgr(cv::Rect(w - bw, 0, bw, bh)));
    cv::Scalar cBL = cv::mean(bgr(cv::Rect(0, h - bh, bw, bh)));
    cv::Scalar cBR = cv::mean(bgr(cv::Rect(w - bw, h - bh, bw, bh)));
    float refB = static_cast<float>((*(cTL.val + 0) + *(cTR.val + 0) + *(cBL.val + 0) + *(cBR.val + 0)) * 0.25);
    float refG = static_cast<float>((*(cTL.val + 1) + *(cTR.val + 1) + *(cBL.val + 1) + *(cBR.val + 1)) * 0.25);
    float refR = static_cast<float>((*(cTL.val + 2) + *(cTR.val + 2) + *(cBL.val + 2) + *(cBR.val + 2)) * 0.25);

    float maxDistSq = static_cast<float>(tol * tol) * 3.0f;
    cv::Mat bgMask(h, w, CV_8UC1, cv::Scalar(0));
    for (int y = 0; y < h; ++y) {
        const cv::Vec3b* row = bgr.ptr<cv::Vec3b>(y);
        uint8_t* mRow = bgMask.ptr<uint8_t>(y);
        for (int x = 0; x < w; ++x) {
            const uint8_t* px = (row + x)->val;
            float pb = static_cast<float>(*(px + 0));
            float pg = static_cast<float>(*(px + 1));
            float pr = static_cast<float>(*(px + 2));
            float db = pb - refB;
            float dg = pg - refG;
            float dr = pr - refR;
            float wb = 255.0f - pb;
            float wg = 255.0f - pg;
            float wr = 255.0f - pr;
            if ((db * db + dg * dg + dr * dr) <= maxDistSq || (wb * wb + wg * wg + wr * wr) <= maxDistSq) {
                *(mRow + x) = 255;
            }
        }
    }

    cv::Mat ffMask = cv::Mat::zeros(h + 2, w + 2, CV_8UC1);
    for (int x = 0; x < w; x += 2) {
        if (bgMask.at<uint8_t>(0, x) == 255) {
            cv::floodFill(bgMask, ffMask, cv::Point(x, 0), cv::Scalar(128));
        }
        if (bgMask.at<uint8_t>(h - 1, x) == 255) {
            cv::floodFill(bgMask, ffMask, cv::Point(x, h - 1), cv::Scalar(128));
        }
    }
    for (int y = 0; y < h; y += 2) {
        if (bgMask.at<uint8_t>(y, 0) == 255) {
            cv::floodFill(bgMask, ffMask, cv::Point(0, y), cv::Scalar(128));
        }
        if (bgMask.at<uint8_t>(y, w - 1) == 255) {
            cv::floodFill(bgMask, ffMask, cv::Point(w - 1, y), cv::Scalar(128));
        }
    }

    cv::Mat alpha(h, w, CV_8UC1);
    for (int y = 0; y < h; ++y) {
        const uint8_t* nw = bgMask.ptr<uint8_t>(y);
        uint8_t* aRow = alpha.ptr<uint8_t>(y);
        for (int x = 0; x < w; ++x) {
            *(aRow + x) = (*(nw + x) == 128) ? 0 : 255;
        }
    }
    cv::GaussianBlur(alpha, alpha, cv::Size(3, 3), 0.8);
    std::vector<cv::Mat> ch;
    cv::split(bgr, ch);
    ch.push_back(alpha);
    cv::Mat bgra;
    cv::merge(ch, bgra);
    return bgra;
}
// ------------------------------------------------------------------------------
// 47. Build calibrated 600x800 (or 256x256 eye) BGRA matrix with Telea & cutouts
// ------------------------------------------------------------------------------
static cv::Mat buildCalibratedCanvasBgra(const FlatImportItem& item, bool fullQualityInpaint)
{
    cv::Mat raw = cv::imread(item.filePath, cv::IMREAD_UNCHANGED);
    if (raw.empty()) {
        return cv::Mat();
    }

    bool doWhite = item.removeWhite && (item.targetCatIdx < 9);
    cv::Mat bgra;
    if (raw.channels() == 4 && !doWhite) {
        bgra = raw.clone();
    } else {
        cv::Mat bgr;
        if (raw.channels() == 4) {
            cv::cvtColor(raw, bgr, cv::COLOR_BGRA2BGR);
        } else if (raw.channels() == 1) {
            cv::cvtColor(raw, bgr, cv::COLOR_GRAY2BGR);
        } else {
            bgr = raw;
        }
        bgra = doWhite ? stripWhiteStudioBackgroundCv(bgr, s_bgTolerance) : cv::Mat();
        if (bgra.empty()) {
            cv::cvtColor(bgr, bgra, cv::COLOR_BGR2BGRA);
        }
    }

    if (item.targetCatIdx >= 10) {
        int sz = 256;
        cv::Mat eyeOut(sz, sz, CV_8UC4, cv::Scalar(0, 0, 0, 0));
        float baseScale = (static_cast<float>(sz) / static_cast<float>(std::max(1, std::min(bgra.cols, bgra.rows)))) * item.scale;
        cv::Point2f srcC(bgra.cols * 0.5f, bgra.rows * 0.5f);
        cv::Mat rotM = cv::getRotationMatrix2D(srcC, item.angle, baseScale);
        rotM.at<double>(0, 2) += (sz * 0.5 + item.offsetX * 0.4 - srcC.x);
        rotM.at<double>(1, 2) += (sz * 0.5 + item.offsetY * 0.4 - srcC.y);
        cv::warpAffine(bgra, eyeOut, rotM, cv::Size(sz, sz), cv::INTER_LINEAR, cv::BORDER_CONSTANT, cv::Scalar(0, 0, 0, 0));
        return eyeOut;
    }

    float fit = std::min(static_cast<float>(BASE_CANVAS_W) / bgra.cols, static_cast<float>(BASE_CANVAS_H) / bgra.rows) * item.scale;
    cv::Point2f srcCenter(bgra.cols * 0.5f, bgra.rows * 0.5f);
    cv::Mat rotMat = cv::getRotationMatrix2D(srcCenter, item.angle, fit);
    rotMat.at<double>(0, 2) += (BASE_CANVAS_W * 0.5 + item.offsetX - srcCenter.x);
    rotMat.at<double>(1, 2) += (BASE_CANVAS_H * 0.5 + item.offsetY - srcCenter.y);

    cv::Mat warped;
    cv::warpAffine(bgra, warped, rotMat, cv::Size(BASE_CANVAS_W, BASE_CANVAS_H), cv::INTER_LINEAR, cv::BORDER_CONSTANT, cv::Scalar(0, 0, 0, 0));

    std::vector<cv::Mat> ch;
    cv::split(warped, ch);
    cv::Mat bgrCanvas;
    cv::Mat alphaCanvas = ch.at(3).clone();
    cv::merge(std::vector<cv::Mat>{ch.at(0), ch.at(1), ch.at(2)}, bgrCanvas);

    bool isFaceOrMakeup = (item.targetCatIdx == 0 || item.targetCatIdx == 8);
    if (isFaceOrMakeup && item.doInpaint && fullQualityInpaint) {
        cv::Mat mask = cv::Mat::zeros(BASE_CANVAS_H, BASE_CANVAS_W, CV_8UC1);
        cv::ellipse(mask, cv::Point(item.noseX, item.noseY), cv::Size(item.noseRx, item.noseRy), 0, 0, 360, cv::Scalar(255), -1);
        cv::ellipse(mask, cv::Point(item.mouthX, item.mouthY), cv::Size(item.mouthRx, item.mouthRy), 0, 0, 360, cv::Scalar(255), -1);
        cv::bitwise_and(mask, alphaCanvas, mask);
        if (cv::countNonZero(mask) > 0) {
            cv::inpaint(bgrCanvas, mask, bgrCanvas, 12.0, cv::INPAINT_TELEA);
        }
    }
    if (isFaceOrMakeup && item.cutEyes) {
        cv::Mat eyeCut(BASE_CANVAS_H, BASE_CANVAS_W, CV_8UC1, cv::Scalar(255));
        cv::ellipse(eyeCut, cv::Point(item.leftEyeX, item.leftEyeY), cv::Size(item.leftEyeRx, item.leftEyeRy), 0, 0, 360, cv::Scalar(0), -1);
        cv::ellipse(eyeCut, cv::Point(item.rightEyeX, item.rightEyeY), cv::Size(item.rightEyeRx, item.rightEyeRy), 0, 0, 360, cv::Scalar(0), -1);
        cv::GaussianBlur(eyeCut, eyeCut, cv::Size(7, 7), 2.2);
        cv::multiply(alphaCanvas, eyeCut, alphaCanvas, 1.0 / 255.0);
    }
    cv::Mat outBgra;
    cv::merge(std::vector<cv::Mat>{bgrCanvas, alphaCanvas}, outBgra);
    return outBgra;
}

// ------------------------------------------------------------------------------
// 48. Update live streaming preview texture for active Calibration Studio item
// ------------------------------------------------------------------------------
static void rebuildFlatImportPreviewTex(SDL_Renderer* ren, FlatImportItem& item, bool fastDrag = false)
{
    cv::Mat bgra = buildCalibratedCanvasBgra(item, !fastDrag);
    if (bgra.empty()) {
        return;
    }
    if (!item.previewTex) {
        item.previewTex = SDL_CreateTexture(ren, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, bgra.cols, bgra.rows);
        if (item.previewTex) {
            SDL_SetTextureBlendMode(item.previewTex, SDL_BLENDMODE_BLEND);
        }
    } else {
        int tw = 0;
        int th = 0;
        SDL_QueryTexture(item.previewTex, nullptr, nullptr, &tw, &th);
        if (tw != bgra.cols || th != bgra.rows) {
            SDL_DestroyTexture(item.previewTex);
            item.previewTex = SDL_CreateTexture(ren, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, bgra.cols, bgra.rows);
            if (item.previewTex) {
                SDL_SetTextureBlendMode(item.previewTex, SDL_BLENDMODE_BLEND);
            }
        }
    }
    if (item.previewTex) {
        SDL_UpdateTexture(item.previewTex, nullptr, bgra.data, static_cast<int>(bgra.step));
    }
}
// ------------------------------------------------------------------------------
// 49. Scan import/ directory for PNG/JPG/WEBP files & guess target category
// ------------------------------------------------------------------------------
static void openFlatImportStudio(SDL_Renderer* ren, FlatImportState& fi, const std::string& importDir = "import")
{
    destroyFlatImportItems(fi);
    fs::create_directories(importDir);
    fs::create_directories(fs::path(importDir) / "done");

    std::vector<fs::path> files;
    for (const auto& entry : fs::directory_iterator(importDir)) {
        if (!entry.is_regular_file()) {
            continue;
        }
        std::string ext = entry.path().extension().string();
        for (char& ch : ext) {
            ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
        }
        if (ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".webp") {
            files.push_back(entry.path());
        }
    }
    std::sort(files.begin(), files.end());

    for (const auto& fp : files) {
        FlatImportItem it{};
        it.filePath = fp.string();
        it.fileName = fp.filename().string();
        std::string low = it.fileName;
        for (char& ch : low) {
            ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
        }
        if (low.find("hat") != std::string::npos || low.find("diadem") != std::string::npos || low.find("tiara") != std::string::npos || low.find("crown") != std::string::npos || low.find("cap") != std::string::npos) {
            it.targetCatIdx = 7;
            it.offsetY      = -210;
            it.scale        = 0.65f;
        } else if (low.find("glass") != std::string::npos) {
            it.targetCatIdx = 5;
        } else if (low.find("hair") != std::string::npos) {
            it.targetCatIdx = 6;
        } else if (low.find("beard") != std::string::npos) {
            it.targetCatIdx = 4;
        } else if (low.find("ear") != std::string::npos) {
            it.targetCatIdx = 1;
        } else if (low.find("nose") != std::string::npos) {
            it.targetCatIdx = 2;
        } else if (low.find("brow") != std::string::npos) {
            it.targetCatIdx = 3;
        } else if (low.find("bg") != std::string::npos) {
            it.targetCatIdx = 9;
            it.removeWhite  = false;
        } else if (low.find("eye") != std::string::npos || low.find("iris") != std::string::npos) {
            it.targetCatIdx = 10;
            it.removeWhite  = false;
        } else if (low.find("sclera") != std::string::npos) {
            it.targetCatIdx = 11;
            it.removeWhite  = false;
        }
        fi.items.push_back(it);
    }
    fi.selectedIdx = 0;
    fi.dragTarget  = 0;
    fi.dragging    = false;
    fi.open        = true;
    if (!fi.items.empty()) {
        rebuildFlatImportPreviewTex(ren, fi.items.at(0), false);
    }
}

// ------------------------------------------------------------------------------
// 50. Export checked Calibration Studio items into assets/ and packs/default.zip
// ------------------------------------------------------------------------------
static int executeFlatImportCommit(SDL_Window* window, FaceAppConfig& cfg, AppRuntimeState& state, const std::string& importDir = "import")
{
    static const std::array<const char*, 10> kFolders = {
        "face", "ears", "nose", "brows", "beard", "glasses", "hair", "hat", "makeup", "bg"
    };
    int importedCount = 0;
    for (const auto& it : state.flatImport.items) {
        if (!it.selected) {
            continue;
        }
        if (importedCount == 0) {
            pushUndoSnapshot(cfg.assets_dir, cfg.config_file, "packs");
        }

        cv::Mat finalBgra = buildCalibratedCanvasBgra(it, true);
        if (finalBgra.empty()) {
            continue;
        }

        if (it.targetCatIdx >= 10) {
            fs::path eyeDir = fs::path(cfg.assets_dir) / "eyes";
            fs::create_directories(eyeDir);
            std::string base = fs::path(it.fileName).stem().string();
            fs::path outPath = eyeDir / (base + (it.targetCatIdx == 10 ? "-iris.png" : "-sclera.png"));
            cv::imwrite(outPath.string(), finalBgra);
            fs::path companion = eyeDir / (base + (it.targetCatIdx == 10 ? "-sclera.png" : "-iris.png"));
            if (!fs::exists(companion)) {
                cv::Mat blankWhite(256, 256, CV_8UC4, cv::Scalar(242, 242, 248, 255));
                cv::imwrite(companion.string(), blankWhite);
            }
        } else {
            const char* folder = kFolders.at(it.targetCatIdx);
            fs::path dstDir = fs::path(cfg.assets_dir) / folder;
            fs::create_directories(dstDir);
            int slot = std::min(255, findMaxAssetSlotInDir(dstDir, false) + 1);
            std::array<char, 16> fn{};
            std::snprintf(fn.data(), fn.size(), "%03d.png", slot);
            cv::imwrite((dstDir / fn.data()).string(), finalBgra);
        }
        fs::rename(it.filePath, fs::path(importDir) / "done" / it.fileName);
        ++importedCount;
    }
    if (importedCount > 0) {
        saveIniConfig(window, cfg, state);
        saveCurrentAssetsToZipPack("default", cfg.assets_dir, cfg.config_file, "packs");
        reloadAllTexturesAndState(SDL_GetRenderer(window), cfg, state);
    }
    return importedCount;
}
// ------------------------------------------------------------------------------
// 51. Handle mouse drag & wheel scaling inside Calibration & Import Studio
// ------------------------------------------------------------------------------
static void handleFlatImportMouse(SDL_Window* win, SDL_Renderer* ren, const SDL_Event& ev, FlatImportState& fi)
{
    if (!fi.open || fi.items.empty()) {
        return;
    }
    auto& cur = fi.items.at(fi.selectedIdx);

    auto toCanvas = [&](int winX, int winY, int& outX, int& outY) {
        int outW = BASE_CANVAS_W;
        int outH = BASE_CANVAS_H;
        if (win) {
            SDL_GetWindowSize(win, &outW, &outH);
        } else {
            SDL_GetRendererOutputSize(ren, &outW, &outH);
        }
        float sc = std::min(static_cast<float>(outW) / BASE_CANVAS_W, static_cast<float>(outH) / BASE_CANVAS_H);
        int vpW = static_cast<int>(std::round(BASE_CANVAS_W * sc));
        int vpH = static_cast<int>(std::round(BASE_CANVAS_H * sc));
        int vpX = (outW - vpW) / 2;
        int vpY = (outH - vpH) / 2;
        outX = static_cast<int>(std::round((winX - vpX) / std::max(0.01f, sc)));
        outY = static_cast<int>(std::round((winY - vpY) / std::max(0.01f, sc)));
    };

    if (ev.type == SDL_MOUSEBUTTONDOWN && ev.button.button == SDL_BUTTON_LEFT) {
        int mx = 0;
        int my = 0;
        toCanvas(ev.button.x, ev.button.y, mx, my);
        auto hit = [&](int cx, int cy, int rx, int ry) {
            float dx = static_cast<float>(mx - cx) / std::max(1, rx);
            float dy = static_cast<float>(my - cy) / std::max(1, ry);
            return (dx * dx + dy * dy) <= 1.35f;
        };
        fi.dragTarget = 0;
        if (cur.targetCatIdx == 0 || cur.targetCatIdx == 8) {
            if (cur.cutEyes && hit(cur.leftEyeX, cur.leftEyeY, cur.leftEyeRx, cur.leftEyeRy)) {
                fi.dragTarget = 1;
            } else if (cur.cutEyes && hit(cur.rightEyeX, cur.rightEyeY, cur.rightEyeRx, cur.rightEyeRy)) {
                fi.dragTarget = 2;
            } else if (cur.doInpaint && hit(cur.noseX, cur.noseY, cur.noseRx, cur.noseRy)) {
                fi.dragTarget = 3;
            } else if (cur.doInpaint && hit(cur.mouthX, cur.mouthY, cur.mouthRx, cur.mouthRy)) {
                fi.dragTarget = 4;
            }
        }
        fi.dragging  = true;
        fi.dragLastX = mx;
        fi.dragLastY = my;
    } else if (ev.type == SDL_MOUSEBUTTONUP && ev.button.button == SDL_BUTTON_LEFT && fi.dragging) {
        fi.dragging = false;
        rebuildFlatImportPreviewTex(ren, cur, false);
    } else if (ev.type == SDL_MOUSEMOTION && fi.dragging) {
        int mx = 0;
        int my = 0;
        toCanvas(ev.motion.x, ev.motion.y, mx, my);
        int dx = mx - fi.dragLastX;
        int dy = my - fi.dragLastY;
        fi.dragLastX = mx;
        fi.dragLastY = my;
        if (fi.dragTarget == 0) {
            cur.offsetX += dx;
            cur.offsetY += dy;
        } else if (fi.dragTarget == 1) {
            cur.leftEyeX += dx;
            cur.leftEyeY += dy;
        } else if (fi.dragTarget == 2) {
            cur.rightEyeX += dx;
            cur.rightEyeY += dy;
        } else if (fi.dragTarget == 3) {
            cur.noseX += dx;
            cur.noseY += dy;
        } else if (fi.dragTarget == 4) {
            cur.mouthX += dx;
            cur.mouthY += dy;
        }
        rebuildFlatImportPreviewTex(ren, cur, true);
    } else if (ev.type == SDL_MOUSEWHEEL) {
        int d = (ev.wheel.y > 0) ? 1 : -1;
        if (fi.dragTarget == 0) {
            cur.scale = std::max(0.15f, std::min(3.0f, cur.scale + d * 0.04f));
        } else if (fi.dragTarget == 1) {
            cur.leftEyeRx = std::max(16, cur.leftEyeRx + d * 3);
            cur.leftEyeRy = std::max(12, cur.leftEyeRy + d * 2);
        } else if (fi.dragTarget == 2) {
            cur.rightEyeRx = std::max(16, cur.rightEyeRx + d * 3);
            cur.rightEyeRy = std::max(12, cur.rightEyeRy + d * 2);
        } else if (fi.dragTarget == 3) {
            cur.noseRx = std::max(14, cur.noseRx + d * 3);
            cur.noseRy = std::max(18, cur.noseRy + d * 3);
        } else if (fi.dragTarget == 4) {
            cur.mouthRx = std::max(20, cur.mouthRx + d * 4);
            cur.mouthRy = std::max(14, cur.mouthRy + d * 3);
        }
        rebuildFlatImportPreviewTex(ren, cur, false);
    }
}

// ------------------------------------------------------------------------------
// 52. Handle keyboard input inside Live Camera & USB Microscope Viewfinder
// ------------------------------------------------------------------------------
static bool handleCameraViewfinderKeys(SDL_Window* window, SDL_Keycode sym, AppRuntimeState& state)
{
    if (!state.cameraCapture.open) {
        return false;
    }
    if (sym == SDLK_ESCAPE) {
        closeCameraViewfinder(state.cameraCapture);
        return true;
    }
    if (sym == SDLK_m) {
        state.cameraCapture.eyeScopeMode = !state.cameraCapture.eyeScopeMode;
        return true;
    }
    if (sym == SDLK_r) {
        state.cameraCapture.rotate90 = !state.cameraCapture.rotate90;
        return true;
    }
    if (sym == SDLK_TAB) {
        state.cameraCapture.useCsiMode = !state.cameraCapture.useCsiMode;
        if (!state.cameraCapture.useCsiMode && !g_usb_cam.isOpened()) {
            openCameraViewfinder(state.cameraCapture);
        }
        return true;
    }
    if (sym == SDLK_SPACE || sym == SDLK_RETURN || sym == SDLK_KP_ENTER || sym == SDLK_e) {
        bool isEye = (sym == SDLK_e || state.cameraCapture.eyeScopeMode);
        std::string saved = captureCameraPhotoToImport(state.cameraCapture, "import", isEye ? "eye" : "cam");
        closeCameraViewfinder(state.cameraCapture);
        if (saved.empty()) {
            showOsd("CAMERA CAPTURE FAILED!", 2400, false);
            return true;
        }
        openFlatImportStudio(SDL_GetRenderer(window), state.flatImport, "import");
        for (size_t i = 0; i < state.flatImport.items.size(); ++i) {
            if (state.flatImport.items.at(i).filePath == saved) {
                state.flatImport.selectedIdx = static_cast<int>(i);
                if (isEye) {
                    state.flatImport.items.at(i).targetCatIdx = 10;
                }
                rebuildFlatImportPreviewTex(SDL_GetRenderer(window), state.flatImport.items.at(i), false);
                break;
            }
        }
        showOsd(isEye ? "MICROSCOPE EYE CAPTURED -> IMPORT STUDIO" : "PHOTO CAPTURED -> IMPORT STUDIO", 2400, false);
        return true;
    }
    return true;
}
// ------------------------------------------------------------------------------
// 53. Handle keyboard controls inside Calibration & Import Studio
// ------------------------------------------------------------------------------
static bool handleFlatImportKeys(SDL_Window* window, SDL_Keycode sym, bool shiftDown, FaceAppConfig& cfg, AppRuntimeState& state)
{
    if (!state.flatImport.open) {
        return false;
    }
    SDL_Renderer* ren = SDL_GetRenderer(window);
    int count = static_cast<int>(state.flatImport.items.size());
    std::array<char, 96> buf{};

    if (sym == SDLK_ESCAPE) {
        destroyFlatImportItems(state.flatImport);
        state.flatImport.open = false;
        return true;
    }
    if (sym == SDLK_c) {
        state.flatImport.open = false;
        openCameraViewfinder(state.cameraCapture);
        return true;
    }
    if (count == 0) {
        return true;
    }

    auto& cur = state.flatImport.items.at(state.flatImport.selectedIdx);
    if (sym == SDLK_UP) {
        state.flatImport.selectedIdx = (state.flatImport.selectedIdx - 1 + count) % count;
        rebuildFlatImportPreviewTex(ren, state.flatImport.items.at(state.flatImport.selectedIdx), false);
        return true;
    }
    if (sym == SDLK_DOWN) {
        state.flatImport.selectedIdx = (state.flatImport.selectedIdx + 1) % count;
        rebuildFlatImportPreviewTex(ren, state.flatImport.items.at(state.flatImport.selectedIdx), false);
        return true;
    }
    if (sym == SDLK_LEFT) {
        cur.targetCatIdx = (cur.targetCatIdx + 11) % 12;
        rebuildFlatImportPreviewTex(ren, cur, false);
        return true;
    }
    if (sym == SDLK_RIGHT) {
        cur.targetCatIdx = (cur.targetCatIdx + 1) % 12;
        rebuildFlatImportPreviewTex(ren, cur, false);
        return true;
    }
    if (sym == SDLK_w) {
        cur.removeWhite = !cur.removeWhite;
        rebuildFlatImportPreviewTex(ren, cur, false);
        return true;
    }
    if (sym == SDLK_b) {
        s_bgTolerance = shiftDown ? std::max(8, s_bgTolerance - 8) : std::min(140, s_bgTolerance + 8);
        cur.removeWhite = true;
        rebuildFlatImportPreviewTex(ren, cur, false);
        return true;
    }
    if (sym == SDLK_v) {
        cur.cutEyes = !cur.cutEyes;
        rebuildFlatImportPreviewTex(ren, cur, false);
        return true;
    }
    if (sym == SDLK_n) {
        cur.doInpaint = !cur.doInpaint;
        rebuildFlatImportPreviewTex(ren, cur, false);
        return true;
    }
    if (sym == SDLK_z) {
        cur.angle -= 1.5f;
        rebuildFlatImportPreviewTex(ren, cur, false);
        return true;
    }
    if (sym == SDLK_x) {
        cur.angle += 1.5f;
        rebuildFlatImportPreviewTex(ren, cur, false);
        return true;
    }
    if (sym == SDLK_EQUALS || sym == SDLK_KP_PLUS) {
        cur.scale = std::min(3.0f, cur.scale + 0.04f);
        rebuildFlatImportPreviewTex(ren, cur, false);
        return true;
    }
    if (sym == SDLK_MINUS || sym == SDLK_KP_MINUS) {
        cur.scale = std::max(0.15f, cur.scale - 0.04f);
        rebuildFlatImportPreviewTex(ren, cur, false);
        return true;
    }
    if (sym == SDLK_SPACE) {
        cur.selected = !cur.selected;
        return true;
    }
    if (sym == SDLK_a) {
        bool anyOff = false;
        for (const auto& it : state.flatImport.items) {
            if (!it.selected) {
                anyOff = true;
            }
        }
        for (auto& it : state.flatImport.items) {
            it.selected = anyOff;
        }
        return true;
    }
    if (sym == SDLK_RETURN || sym == SDLK_KP_ENTER) {
        int n = executeFlatImportCommit(window, cfg, state, "import");
        destroyFlatImportItems(state.flatImport);
        state.flatImport.open = false;
        std::snprintf(buf.data(), buf.size(), "SAVED %d CALIBRATED ITEM(S) -> ASSETS/", n);
        showOsd(buf.data(), 2800, false);
        return true;
    }
    return true;
}

// ==============================================================================
// Section G. Adafruit Eye Browser, Network Socket & Layer Texture Loaders
// ==============================================================================

// ------------------------------------------------------------------------------
// 54. Free all cached 210x210 live eye preview textures in Eye Browser
// ------------------------------------------------------------------------------
static void destroyEyeBrowserEntries(EyeBrowserState& eb)
{
    for (auto& e : eb.entries) {
        if (e.thumbTex) {
            SDL_DestroyTexture(e.thumbTex);
            e.thumbTex = nullptr;
        }
    }
    eb.entries.clear();
}

// ------------------------------------------------------------------------------
// 55. Reload dynamic eye bank and render 210x210 thumbnails for Eye Browser
// ------------------------------------------------------------------------------
static void refreshEyeBrowserEntries(SDL_Renderer* ren, EyeBrowserState& eb, const std::string& assetsDir)
{
    destroyEyeBrowserEntries(eb);
    reloadDynamicEyeBank(assetsDir);

    EyeBrowserEntry autoItem{};
    autoItem.name    = "AUTO (MEMORY / DMX)";
    autoItem.bankIdx = -1;
    eb.entries.push_back(autoItem);

    int total = getEyeBankCount();
    std::vector<uint32_t> argb(210 * 210, 0);
    for (int i = 0; i < total; ++i) {
        EyeBrowserEntry item{};
        item.name    = getEyeBankName(i);
        item.bankIdx = i;
        renderEyePreviewBuffer(i, argb.data());
        SDL_Texture* tex = SDL_CreateTexture(ren, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STATIC, 210, 210);
        if (tex) {
            SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND);
            SDL_UpdateTexture(tex, nullptr, argb.data(), 210 * sizeof(uint32_t));
            item.thumbTex = tex;
        }
        eb.entries.push_back(item);
    }
    if (eb.selectedIdx >= static_cast<int>(eb.entries.size())) {
        eb.selectedIdx = 0;
    }
}
// ------------------------------------------------------------------------------
// 56. Handle keyboard navigation and selection inside Adafruit Eye Browser (F8)
// ------------------------------------------------------------------------------
static bool handleEyeBrowserKeys(SDL_Keycode sym, AppRuntimeState& state)
{
    if (!state.eyeBrowser.open) {
        return false;
    }
    int count = static_cast<int>(state.eyeBrowser.entries.size());
    std::array<char, 96> buf{};

    if (sym == SDLK_ESCAPE || sym == SDLK_F8) {
        state.eyeBrowser.open = false;
        return true;
    }
    if (sym == SDLK_LEFT && count > 0) {
        state.eyeBrowser.selectedIdx = (state.eyeBrowser.selectedIdx - 1 + count) % count;
        return true;
    }
    if (sym == SDLK_RIGHT && count > 0) {
        state.eyeBrowser.selectedIdx = (state.eyeBrowser.selectedIdx + 1) % count;
        return true;
    }
    if (sym == SDLK_0 || sym == SDLK_BACKSPACE) {
        selectEyeBankIndex(-1);
        state.eyeBrowser.open = false;
        showOsd("EYE BANK: AUTO MODE (BOTH)", 2200, false);
        return true;
    }
    if ((sym == SDLK_l || sym == SDLK_1) && count > 0) {
        const auto& sel = state.eyeBrowser.entries.at(state.eyeBrowser.selectedIdx);
        selectEyeBankForSide(0, sel.bankIdx);
        state.eyeBrowser.open = false;
        std::snprintf(buf.data(), buf.size(), "LEFT EYE: %s [S=SAVE]", sel.name.c_str());
        showOsd(buf.data(), 2400, false);
        return true;
    }
    if ((sym == SDLK_r || sym == SDLK_2) && count > 0) {
        const auto& sel = state.eyeBrowser.entries.at(state.eyeBrowser.selectedIdx);
        selectEyeBankForSide(1, sel.bankIdx);
        state.eyeBrowser.open = false;
        std::snprintf(buf.data(), buf.size(), "RIGHT EYE: %s [S=SAVE]", sel.name.c_str());
        showOsd(buf.data(), 2400, false);
        return true;
    }
    if ((sym == SDLK_RETURN || sym == SDLK_KP_ENTER) && count > 0) {
        const auto& sel = state.eyeBrowser.entries.at(state.eyeBrowser.selectedIdx);
        if (state.dualProfileMode) {
            int side = state.editingRightProfile ? 1 : 0;
            selectEyeBankForSide(side, sel.bankIdx);
            std::snprintf(buf.data(), buf.size(), "%s EYE: %s [S=SAVE]", side == 1 ? "RIGHT" : "LEFT", sel.name.c_str());
        } else {
            selectEyeBankIndex(sel.bankIdx);
            std::snprintf(buf.data(), buf.size(), "BOTH EYES: %s [S=SAVE]", sel.name.c_str());
        }
        state.eyeBrowser.open = false;
        showOsd(buf.data(), 2400, false);
        return true;
    }
    return true;
}

// ------------------------------------------------------------------------------
// 57. Handle SIGINT / SIGTERM for clean shutdown
// ------------------------------------------------------------------------------
void sig_handler(int)
{
    g_running = false;
}

// ------------------------------------------------------------------------------
// 58. Initialize non-blocking Art-Net (6454) or sACN multicast (5568) socket
// ------------------------------------------------------------------------------
int init_dmx_socket(const std::string& proto, uint16_t universe)
{
    int port = (proto == "sacn") ? SACN_PORT : ARTNET_PORT;
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        return -1;
    }
    int opt = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in addr{};
    addr.sin_family      = AF_INET;
    addr.sin_port        = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;
    if (bind(sock, (sockaddr*)&addr, sizeof(addr)) < 0) {
        close(sock);
        return -1;
    }

    if (proto == "sacn") {
        struct ifaddrs* ifaddr = nullptr;
        if (getifaddrs(&ifaddr) == 0) {
            std::string ip = "239.255." + std::to_string((universe >> 8) & 0xFF) + "." + std::to_string(universe & 0xFF);
            for (auto* ifa = ifaddr; ifa; ifa = ifa->ifa_next) {
                if (!ifa->ifa_addr || ifa->ifa_addr->sa_family != AF_INET) {
                    continue;
                }
                ip_mreq m{};
                inet_pton(AF_INET, ip.c_str(), &m.imr_multiaddr);
                m.imr_interface = ((sockaddr_in*)ifa->ifa_addr)->sin_addr;
                setsockopt(sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, &m, sizeof(m));
            }
            freeifaddrs(ifaddr);
        }
    }

    timeval tv{0, 100000};
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    return sock;
}

// ------------------------------------------------------------------------------
// 59. Parse incoming Art-Net OpDmx or sACN E1.31 data packet into 512-byte buffer
// ------------------------------------------------------------------------------
static bool parseDmxPacket(const std::string& proto, uint16_t uni, const std::array<uint8_t, 1500>& b, ssize_t len, std::array<uint8_t, 512>& out)
{
    if (proto == "artnet") {
        if (len < 20 || std::memcmp(b.data(), "Art-Net\0", 8) != 0) {
            return false;
        }
        uint16_t opCode = static_cast<uint16_t>(b.at(8) | (b.at(9) << 8));
        uint16_t pktUni = static_cast<uint16_t>(b.at(14) | ((b.at(15) & 0x7F) << 8));
        if (opCode != 0x5000 || pktUni != uni) {
            return false;
        }
        int dmxLen = std::min<int>(512, (b.at(16) << 8) | b.at(17));
        if (dmxLen <= 0 || len < 18 + dmxLen) {
            return false;
        }
        std::memcpy(out.data(), b.data() + 18, static_cast<size_t>(dmxLen));
        return true;
    }

    if (len < 127 || std::memcmp(b.data() + 4, "ASC-E1.17\0\0\0", 12) != 0) {
        return false;
    }
    uint32_t rootVec = (static_cast<uint32_t>(b.at(18)) << 24) |
                       (static_cast<uint32_t>(b.at(19)) << 16) |
                       (static_cast<uint32_t>(b.at(20)) << 8)  |
                        static_cast<uint32_t>(b.at(21));
    uint16_t pktUni  = static_cast<uint16_t>((b.at(113) << 8) | b.at(114));
    if (rootVec != 0x00000004u || b.at(125) != 0x00 || pktUni != uni) {
        return false;
    }
    int dmxLen = std::min<int>(512, ((b.at(123) << 8) | b.at(124)) - 1);
    if (dmxLen <= 0 || len < 126 + dmxLen) {
        return false;
    }
    std::memcpy(out.data(), b.data() + 126, static_cast<size_t>(dmxLen));
    return true;
}

// ------------------------------------------------------------------------------
// 60. Sample average skin or eyeshadow RGB from loaded surface for eyelid tint
// ------------------------------------------------------------------------------
static void sampleSurfaceColorForEyelids(SDL_Surface* raw, bool isMakeup, bool isRightSide)
{
    if (!raw) {
        return;
    }
    SDL_Surface* surf = SDL_ConvertSurfaceFormat(raw, SDL_PIXELFORMAT_ARGB8888, 0);
    if (!surf) {
        return;
    }

    int side = isRightSide ? 1 : 0;
    uint64_t sumR  = 0;
    uint64_t sumG  = 0;
    uint64_t sumB  = 0;
    uint64_t count = 0;
    int yStart = isMakeup ? 295 : 240;
    int yEnd   = isMakeup ? 385 : 420;
    const uint32_t* px = static_cast<const uint32_t*>(surf->pixels);
    int pitchPx = surf->pitch / 4;

    for (int y = yStart; y < yEnd && y < surf->h; y += 4) {
        for (int x = 120; x < 480 && x < surf->w; x += 4) {
            if (isMakeup && !((x >= 135 && x <= 245) || (x >= 355 && x <= 465))) {
                continue;
            }
            uint32_t c = px[y * pitchPx + x];
            if (((c >> 24) & 0xFF) < 140) {
                continue;
            }
            sumR += (c >> 16) & 0xFF;
            sumG += (c >> 8) & 0xFF;
            sumB += c & 0xFF;
            ++count;
        }
    }
    SDL_FreeSurface(surf);

    if (count > 0) {
        uint8_t r = static_cast<uint8_t>(sumR / count);
        uint8_t g = static_cast<uint8_t>(sumG / count);
        uint8_t b = static_cast<uint8_t>(sumB / count);
        if (isMakeup) {
            setEyelidMakeupColorForSide(side, true, r, g, b);
        } else {
            setEyelidSkinColorForSide(side, r, g, b);
        }
    } else if (isMakeup) {
        setEyelidMakeupColorForSide(side, false, 0, 0, 0);
    }
}

// ------------------------------------------------------------------------------
// 61. Load numbered paper-doll PNG texture into layer slot (single disk read)
// ------------------------------------------------------------------------------
static void updatePaperDollLayer(SDL_Renderer* renderer, const std::string& assetsDir, uint8_t requestedIndex, PaperDollLayer& layer)
{
    if (requestedIndex == layer.loadedIndex) {
        return;
    }
    if (layer.texture) {
        SDL_DestroyTexture(layer.texture);
        layer.texture = nullptr;
    }
    layer.loadedIndex = requestedIndex;

    int side      = layer.isRightSide ? 1 : 0;
    bool isFace   = (std::strcmp(layer.folder, "face") == 0);
    bool isMakeup = (std::strcmp(layer.folder, "makeup") == 0);
    if (requestedIndex == 0) {
        if (isFace) {
            setEyelidFaceActiveForSide(side, true);
            setEyelidSkinColorForSide(side, 212, 164, 138);
        }
        if (isMakeup) {
            setEyelidMakeupColorForSide(side, false, 0, 0, 0);
        }
        return;
    }

    std::array<char, 16> fileName{};
    std::snprintf(fileName.data(), fileName.size(), "%03u.png", requestedIndex);
    fs::path fullPath = fs::path(assetsDir) / layer.folder / fileName.data();

    SDL_Surface* surf = IMG_Load(fullPath.string().c_str());
    if (!surf) {
        return;
    }

    if (isFace || isMakeup) {
        setEyelidFaceActiveForSide(side, true);
        sampleSurfaceColorForEyelids(surf, isMakeup, layer.isRightSide);
    }
    layer.texture = SDL_CreateTextureFromSurface(renderer, surf);
    SDL_FreeSurface(surf);
    if (layer.texture) {
        SDL_SetTextureBlendMode(layer.texture, SDL_BLENDMODE_BLEND);
    }
}

// ------------------------------------------------------------------------------
// 62. Free all 19 paper-doll and dual-profile SDL textures on shutdown
// ------------------------------------------------------------------------------
static void destroyPaperDollLayers(AppRuntimeState& state)
{
    std::array<PaperDollLayer*, 19> layers = {
        &state.bgLayer, &state.faceLayer, &state.makeupLayer, &state.earsLayer, &state.browsLayer,
        &state.noseLayer, &state.beardLayer, &state.glassesLayer, &state.hairLayer, &state.hatLayer,
        &state.dualFaceLayer, &state.dualMakeupLayer, &state.dualEarsLayer, &state.dualBrowsLayer,
        &state.dualNoseLayer, &state.dualBeardLayer, &state.dualGlassesLayer, &state.dualHairLayer, &state.dualHatLayer
    };
    for (auto* l : layers) {
        if (l->texture) {
            SDL_DestroyTexture(l->texture);
            l->texture = nullptr;
        }
    }
}

// ------------------------------------------------------------------------------
// 63. Load PNG texture with alpha blending if file exists on disk
// ------------------------------------------------------------------------------
static SDL_Texture* loadPngIfExists(SDL_Renderer* ren, const fs::path& p)
{
    if (!fs::exists(p)) {
        return nullptr;
    }
    SDL_Texture* tex = IMG_LoadTexture(ren, p.string().c_str());
    if (tex) {
        SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND);
    }
    return tex;
}

// ------------------------------------------------------------------------------
// 64. Destroy all 5-layer parallax backdrop textures
// ------------------------------------------------------------------------------
void destroyParallaxBackdrop(ParallaxBackdrop& bg)
{
    SDL_Texture** all[] = {&bg.sky, &bg.clouds, &bg.farLayer, &bg.midLayer, &bg.frontLayer, &bg.single};
    for (auto** t : all) {
        if (*t) {
            SDL_DestroyTexture(*t);
            *t = nullptr;
        }
    }
    bg.loadedIndex = 0;
}

// ------------------------------------------------------------------------------
// 65. Load 5-layer parallax backdrop (_sky.._front) or single static backdrop
// ------------------------------------------------------------------------------
static void updateParallaxBackdrop(SDL_Renderer* ren, const std::string& assetsDir, uint8_t reqIdx, ParallaxBackdrop& bg)
{
    if (reqIdx == bg.loadedIndex) {
        return;
    }
    destroyParallaxBackdrop(bg);
    bg.loadedIndex = reqIdx;
    if (reqIdx == 0) {
        return;
    }

    std::array<char, 32> name{};
    std::snprintf(name.data(), name.size(), "%03u_sky.png", reqIdx);
    bg.sky = loadPngIfExists(ren, fs::path(assetsDir) / "bg" / name.data());
    if (bg.sky) {
        std::snprintf(name.data(), name.size(), "%03u_clouds.png", reqIdx);
        bg.clouds = loadPngIfExists(ren, fs::path(assetsDir) / "bg" / name.data());
        std::snprintf(name.data(), name.size(), "%03u_far.png", reqIdx);
        bg.farLayer = loadPngIfExists(ren, fs::path(assetsDir) / "bg" / name.data());
        std::snprintf(name.data(), name.size(), "%03u_mid.png", reqIdx);
        bg.midLayer = loadPngIfExists(ren, fs::path(assetsDir) / "bg" / name.data());
        std::snprintf(name.data(), name.size(), "%03u_front.png", reqIdx);
        bg.frontLayer = loadPngIfExists(ren, fs::path(assetsDir) / "bg" / name.data());
    } else {
        std::snprintf(name.data(), name.size(), "%03u.png", reqIdx);
        bg.single = loadPngIfExists(ren, fs::path(assetsDir) / "bg" / name.data());
    }
}

// ------------------------------------------------------------------------------
// 66. Load all 17 FX sprites, _2 animation frames, leaf/snow & Gaussian DOF blur
// ------------------------------------------------------------------------------
void initFxSpriteBank(SDL_Renderer* ren, const std::string& assetsDir, FxSpriteBank& bank)
{
    static const std::array<const char*, 17> kFiles = {
        "bat.png", "fly.png", "mosquito.png", "bird.png", "wasp.png",
        "spider.png", "butterfly.png", "ufo.png", "cobweb.png", "fog_1.png",
        "ghost.png", "snake.png", "fire_1.png", "fire_2.png",
        "lightning_1.png", "lightning_2.png", "mouse.png"
    };
    for (size_t i = 0; i < kFiles.size(); ++i) {
        fs::path p = fs::path(assetsDir) / "fx" / kFiles.at(i);
        bank.sprites.at(i) = loadPngIfExists(ren, p);

        std::string stem = fs::path(kFiles.at(i)).stem().string();
        fs::path pFrame2 = fs::path(assetsDir) / "fx" / (stem + "_2.png");
        bank.animSprites.at(i) = loadPngIfExists(ren, pFrame2);

        if (fs::exists(p)) {
            cv::Mat img = cv::imread(p.string(), cv::IMREAD_UNCHANGED);
            if (!img.empty()) {
                cv::Mat bgra;
                if (img.channels() == 4) {
                    bgra = img;
                } else if (img.channels() == 3) {
                    cv::cvtColor(img, bgra, cv::COLOR_BGR2BGRA);
                } else {
                    cv::cvtColor(img, bgra, cv::COLOR_GRAY2BGRA);
                }
                cv::GaussianBlur(bgra, bgra, cv::Size(0, 0), 3.2, 3.2);
                SDL_Texture* bTex = SDL_CreateTexture(ren, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STATIC, bgra.cols, bgra.rows);
                if (bTex) {
                    SDL_SetTextureBlendMode(bTex, SDL_BLENDMODE_BLEND);
                    SDL_UpdateTexture(bTex, nullptr, bgra.data, static_cast<int>(bgra.step));
                    bank.blurSprites.at(i) = bTex;
                }
            }
        }
    }
    bank.fog2                 = loadPngIfExists(ren, fs::path(assetsDir) / "fx" / "fog_2.png");
    bank.leafTex              = loadPngIfExists(ren, fs::path(assetsDir) / "fx" / "leaf.png");
    bank.snowflakeTex         = loadPngIfExists(ren, fs::path(assetsDir) / "fx" / "snowflake.png");
    bank.fireflyTex.at(0)     = loadPngIfExists(ren, fs::path(assetsDir) / "fx" / "firefly_1.png");
    bank.fireflyTex.at(1)     = loadPngIfExists(ren, fs::path(assetsDir) / "fx" / "firefly_2.png");
    bank.grasshopperTex.at(0) = loadPngIfExists(ren, fs::path(assetsDir) / "fx" / "grasshopper_1.png");
    bank.grasshopperTex.at(1) = loadPngIfExists(ren, fs::path(assetsDir) / "fx" / "grasshopper_2.png");
    bank.antTex.at(0)         = loadPngIfExists(ren, fs::path(assetsDir) / "fx" / "ant_1.png");
    bank.antTex.at(1)         = loadPngIfExists(ren, fs::path(assetsDir) / "fx" / "ant_2.png");
}

// ------------------------------------------------------------------------------
// 67. Free all sharp, animated & blurred FX sprite textures on shutdown/reload
// ------------------------------------------------------------------------------
void destroyFxSpriteBank(FxSpriteBank& bank)
{
    for (auto*& t : bank.sprites) {
        if (t) { SDL_DestroyTexture(t); t = nullptr; }
    }
    for (auto*& t : bank.animSprites) {
        if (t) { SDL_DestroyTexture(t); t = nullptr; }
    }
    for (auto*& t : bank.blurSprites) {
        if (t) { SDL_DestroyTexture(t); t = nullptr; }
    }
    SDL_Texture** extras[] = {
        &bank.fog2, &bank.leafTex, &bank.snowflakeTex,
        &bank.fireflyTex.at(0), &bank.fireflyTex.at(1),
        &bank.grasshopperTex.at(0), &bank.grasshopperTex.at(1),
        &bank.antTex.at(0), &bank.antTex.at(1)
    };
    for (auto** t : extras) {
        if (*t) { SDL_DestroyTexture(*t); *t = nullptr; }
    }
}

// ------------------------------------------------------------------------------
// 68. Render split left/right eyebrow PNG halves with tilt, mood and scale
// ------------------------------------------------------------------------------
static void renderDynamicEyebrowsSplit(SDL_Renderer* renderer, const PaperDollLayer& browLayer, uint8_t rawLeft, uint8_t rawRight, bool showLeft, bool showRight, int extraSplit)
{
    if (!browLayer.texture) {
        return;
    }
    int baseY   = browLayer.offsetY + GLOBAL_FACE_Y_OFFSET;
    float moodL = (static_cast<float>(rawLeft) - 128.0f) / 128.0f;
    float moodR = (static_cast<float>(rawRight) - 128.0f) / 128.0f;
    float sc    = browLayer.scale;
    int halfW   = static_cast<int>(std::round((BASE_CANVAS_W / 2) * sc));
    int fullH   = static_cast<int>(std::round(BASE_CANVAS_H * sc));
    int dy      = (BASE_CANVAS_H - fullH) / 2;

    if (showLeft) {
        SDL_Rect srcL{0, 0, BASE_CANVAS_W / 2, BASE_CANVAS_H};
        SDL_Rect dstL{(BASE_CANVAS_W / 2) - halfW + browLayer.offsetX - browLayer.splitOffsetX - extraSplit,
                      baseY + dy + static_cast<int>(std::round(-moodL * 10.0f)), halfW, fullH};
        SDL_Point pivL{static_cast<int>(std::round(205 * sc)), static_cast<int>(std::round(285 * sc))};
        SDL_RenderCopyEx(renderer, browLayer.texture, &srcL, &dstL, static_cast<double>(-moodL * 18.0f), &pivL, SDL_FLIP_NONE);
    }
    if (showRight) {
        SDL_Rect srcR{BASE_CANVAS_W / 2, 0, BASE_CANVAS_W / 2, BASE_CANVAS_H};
        SDL_Rect dstR{(BASE_CANVAS_W / 2) + browLayer.offsetX + browLayer.splitOffsetX + extraSplit,
                      baseY + dy + static_cast<int>(std::round(-moodR * 10.0f)), halfW, fullH};
        SDL_Point pivR{static_cast<int>(std::round(95 * sc)), static_cast<int>(std::round(285 * sc))};
        SDL_RenderCopyEx(renderer, browLayer.texture, &srcR, &dstR, static_cast<double>(moodR * 18.0f), &pivR, SDL_FLIP_NONE);
    }
}
// ==============================================================================
// Section H. Backdrop Parallax, Atmospheric FX & Gaze Tracking Helpers
// ==============================================================================

// ------------------------------------------------------------------------------
// 69. Toggle Wayland desktop fullscreen and cursor visibility safely
// ------------------------------------------------------------------------------
static void toggleWindowFullscreen(SDL_Window* window, bool& isFullscreen)
{
    isFullscreen = !isFullscreen;
    if (isFullscreen) {
        SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN_DESKTOP);
    } else {
        SDL_SetWindowFullscreen(window, 0);
        SDL_SetWindowResizable(window, SDL_TRUE);
        SDL_SetWindowSize(window, BASE_CANVAS_W, BASE_CANVAS_H);
        int dispIdx = SDL_GetWindowDisplayIndex(window);
        if (dispIdx >= 0) {
            SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED_DISPLAY(dispIdx), SDL_WINDOWPOS_CENTERED_DISPLAY(dispIdx));
        }
    }
    SDL_ShowCursor(isFullscreen ? SDL_DISABLE : SDL_ENABLE);
}

// ------------------------------------------------------------------------------
// 70. Step asset index forward or backward across 0..maxCount with wrap-around
// ------------------------------------------------------------------------------
static uint8_t stepAssetIndex(uint8_t current, uint8_t maxCount, bool backward)
{
    if (backward) {
        return (current == 0) ? maxCount : static_cast<uint8_t>(current - 1);
    }
    return (current >= maxCount) ? 0 : static_cast<uint8_t>(current + 1);
}

// ------------------------------------------------------------------------------
// 71. Apply pending memory switch at midpoint of crossfade transition
// ------------------------------------------------------------------------------
static void executePendingMemorySwitch(const FaceAppConfig& cfg, AppRuntimeState& state)
{
    int targetApply = state.pendingApplySlot;
    state.appliedMemorySlot = targetApply;
    if (state.pendingMemorySlot != -99) {
        state.manualMemorySlot  = state.pendingMemorySlot;
        state.pendingMemorySlot = -99;
    }
    if (state.manualMemorySlot < 0) {
        if (targetApply <= 0) {
            state.manualTestAnim   = 0;
            state.isEditingOffsets = false;
            state.manualHideMouth  = false;
            state.manualHideNose   = false;
            state.manualHideBrows  = false;
        }
        applyMemoryComboToState(cfg, state, targetApply, false);
        applyMaskOffsetProfile(cfg, state, state.faceLayer.testIndex);
    } else {
        applyMemoryComboToState(cfg, state, targetApply, true);
        applyMaskOffsetProfile(cfg, state, state.faceLayer.testIndex);
    }
}

// ------------------------------------------------------------------------------
// 72. Initiate smooth fade-to-black crossfade transition into target memory slot
// ------------------------------------------------------------------------------
static void triggerMemoryTransition(AppRuntimeState& state, const FaceAppConfig& cfg, int newManualSlot, int newApplySlot, const char* osdMsg)
{
    if (osdMsg && *osdMsg != '\0') {
        showOsd(osdMsg, 2400, false);
    }
    state.pendingMemorySlot = newManualSlot;
    state.pendingApplySlot  = newApplySlot;
    if (cfg.fade_ms > 0) {
        state.fadePhase   = 1;
        state.fadeStartMs = millis();
    } else {
        executePendingMemorySwitch(cfg, state);
    }
}

// ------------------------------------------------------------------------------
// 73. Draw solid dark oval eye-socket shadow backing behind eyeballs
// ------------------------------------------------------------------------------
static void drawOvalShadow(SDL_Renderer* ren, int cx, int cy, int rx, int ry, uint8_t alpha)
{
    SDL_SetRenderDrawColor(ren, 0, 0, 0, alpha);
    for (int dy = -ry; dy <= ry; ++dy) {
        float normY = static_cast<float>(dy) / static_cast<float>(ry);
        int   halfW = static_cast<int>(rx * std::sqrt(std::max(0.0f, 1.0f - normY * normY)));
        SDL_Rect band{cx - halfW, cy + dy, halfW * 2 + 1, 2};
        SDL_RenderFillRect(ren, &band);
    }
}

// ------------------------------------------------------------------------------
// 76. Render 5-layer depth backdrop + independent stars, comets & aurora FX
// ------------------------------------------------------------------------------
static void renderDepthBackdropLayers(SDL_Renderer* ren, const ParallaxBackdrop& bg, uint32_t fxMask, uint32_t nowMs, int outW, int outH)
{
    bool showStars  = ((fxMask & FX_BIT_STARS) != 0);
    bool showComet  = ((fxMask & FX_BIT_COMET) != 0);
    bool showAurora = ((fxMask & FX_BIT_AURORA) != 0);

    if (bg.loadedIndex == 0 && !showStars && !showComet && !showAurora) {
        return;
    }

    auto copyCroppedFull = [&](SDL_Texture* tex) {
        if (!tex) {
            return;
        }
        int tw = 0;
        int th = 0;
        SDL_QueryTexture(tex, nullptr, nullptr, &tw, &th);
        SDL_Rect src = (tw > 8 && th > 4) ? SDL_Rect{2, 0, tw - 4, th - 2} : SDL_Rect{0, 0, tw, th};
        SDL_Rect full{0, 0, outW, outH + 2};
        SDL_RenderCopy(ren, tex, &src, &full);
    };

    float t = static_cast<float>(nowMs) * 0.001f;
    if (bg.loadedIndex > 0) {
        if (bg.single) {
            copyCroppedFull(bg.single);
        } else {
            copyCroppedFull(bg.sky);
        }
    }
    g_stage.renderLayer(ren, DepthLayer::SKY, 0.15f);

    if (showStars || showComet || showAurora) {
        SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_ADD);
        if (showStars) {
            for (int s = 0; s < 65; ++s) {
                int sx = ((s * 173 + 47) % std::max(1, outW));
                int sy = ((s * 97 + 23) % std::max(1, static_cast<int>(outH * 0.58f)));
                float tw = 0.5f + 0.5f * std::sin(t * (1.8f + (s % 5) * 0.7f) + s * 1.3f);
                uint8_t a = static_cast<uint8_t>(45.0f + tw * 205.0f);
                SDL_SetRenderDrawColor(ren, 235, 245, 255, a);
                SDL_Rect st{sx, sy, (s % 4 == 0) ? 3 : 2, (s % 4 == 0) ? 3 : 2};
                SDL_RenderFillRect(ren, &st);
            }
        }
        if (showAurora) {
            for (int x = 0; x < outW; x += 3) {
                float nx         = static_cast<float>(x) / static_cast<float>(std::max(1, outW));
                float fold       = std::sin(nx * 11.0f + t * 0.85f) * 26.0f + std::cos(nx * 23.0f - t * 1.35f) * 12.0f;
                float shimmer    = std::pow(0.5f + 0.5f * std::sin(nx * 47.0f - t * 3.8f + std::sin(nx * 9.0f + t)), 3.0f);
                float waveBand   = 0.5f + 0.5f * std::sin(nx * 6.5f + t * 0.55f);
                int baseY        = static_cast<int>(outH * 0.25f + fold);
                int rayH         = static_cast<int>(outH * (0.12f + 0.15f * shimmer * waveBand));
                uint8_t alphaLow = static_cast<uint8_t>(22.0f + shimmer * 90.0f);
                uint8_t rLow     = static_cast<uint8_t>(30.0f + 155.0f * std::pow(0.5f + 0.5f * std::sin(nx * 14.0f - t * 1.1f), 2.0f));
                uint8_t gLow     = static_cast<uint8_t>(185.0f + 70.0f * waveBand);
                uint8_t bLow     = static_cast<uint8_t>(135.0f + 115.0f * (1.0f - waveBand));
                SDL_SetRenderDrawColor(ren, rLow, gLow, bLow, alphaLow);
                SDL_Rect lowerRay{x, baseY - rayH, 3, rayH};
                SDL_RenderFillRect(ren, &lowerRay);
                SDL_SetRenderDrawColor(ren, 175, 65, 245, static_cast<uint8_t>(alphaLow * 0.65f));
                SDL_Rect upperCrown{x, baseY - rayH - (rayH / 2), 3, rayH / 2};
                SDL_RenderFillRect(ren, &upperCrown);
            }
        }
        if (showComet) {
            float shootU = std::fmod(t + 1.5f, 5.2f);
            if (shootU < 0.75f) {
                float p     = shootU / 0.75f;
                float headX = outW * (0.18f + p * 0.66f);
                float headY = outH * (0.05f + p * 0.20f);
                float tailX = headX - outW * 0.15f;
                float tailY = headY - outH * 0.045f;
                for (int s = 0; s < 16; ++s) {
                    float f0 = static_cast<float>(s) / 16.0f;
                    float f1 = static_cast<float>(s + 1) / 16.0f;
                    SDL_SetRenderDrawColor(ren, 235, 248, 255, static_cast<uint8_t>((1.0f - f0) * 240.0f));
                    SDL_RenderDrawLine(ren, static_cast<int>(headX + (tailX - headX) * f0), static_cast<int>(headY + (tailY - headY) * f0),
                                       static_cast<int>(headX + (tailX - headX) * f1), static_cast<int>(headY + (tailY - headY) * f1));
                }
            }
            float cometU = std::fmod(t * 0.45f + 0.8f, 6.0f);
            if (cometU < 2.6f) {
                float p     = cometU / 2.6f;
                float headX = outW * (0.85f - p * 0.72f);
                float headY = outH * (0.09f + p * 0.19f);
                float tailX = headX + outW * 0.22f;
                float tailY = headY - outH * 0.06f;
                for (int s = 0; s < 22; ++s) {
                    float f0 = static_cast<float>(s) / 22.0f;
                    float f1 = static_cast<float>(s + 1) / 22.0f;
                    SDL_SetRenderDrawColor(ren, 160, 235, 255, static_cast<uint8_t>((1.0f - f0) * 210.0f));
                    SDL_RenderDrawLine(ren, static_cast<int>(headX + (tailX - headX) * f0), static_cast<int>(headY + (tailY - headY) * f0),
                                       static_cast<int>(headX + (tailX - headX) * f1), static_cast<int>(headY + (tailY - headY) * f1));
                }
                SDL_SetRenderDrawColor(ren, 255, 250, 220, 255);
                SDL_Rect core{static_cast<int>(headX) - 3, static_cast<int>(headY) - 3, 6, 6};
                SDL_RenderFillRect(ren, &core);
            }
        }
        SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);
    }

    if (bg.loadedIndex > 0 && !bg.single) {
        copyCroppedFull(bg.farLayer);
        copyCroppedFull(bg.midLayer);
    }
}

// ------------------------------------------------------------------------------
// 77. Clear active critter eye-tracking target and cross-eye convergence
// ------------------------------------------------------------------------------
static inline void clearGazeTarget(void)
{
    g_critter_gaze_active.store(false, std::memory_order_relaxed);
    g_critter_gaze_converge.store(0, std::memory_order_relaxed);
}

// ------------------------------------------------------------------------------
// 78. Update critter eye-tracking coordinates & cross-eye convergence (0..1)
// ------------------------------------------------------------------------------
static inline void setGazeTarget(float x, float y, float crossEyeAmount = 0.0f)
{
    int gx = std::max(0, std::min(255, static_cast<int>(std::round((x / static_cast<float>(BASE_CANVAS_W)) * 255.0f))));
    int gy = std::max(0, std::min(255, static_cast<int>(std::round(((static_cast<float>(BASE_CANVAS_H) - y) / static_cast<float>(BASE_CANVAS_H)) * 255.0f))));
    int cv = std::max(0, std::min(120, static_cast<int>(std::round(crossEyeAmount * 112.0f))));
    g_critter_gaze_x.store(static_cast<uint8_t>(gx), std::memory_order_relaxed);
    g_critter_gaze_y.store(static_cast<uint8_t>(gy), std::memory_order_relaxed);
    g_critter_gaze_converge.store(static_cast<uint8_t>(cv), std::memory_order_relaxed);
    g_critter_gaze_active.store(true, std::memory_order_relaxed);
}

// ------------------------------------------------------------------------------
// 79. Legacy FX depth hook (all scenic FX now live as Stage Actors)
// ------------------------------------------------------------------------------
static void renderEffectsAtDepth(SDL_Renderer*, const FxSpriteBank&, uint32_t, int, uint32_t, bool&)
{
}

// ==============================================================================
// Section I. Keyboard Input Routers, Frame Compositor & Engine Main Loop
// ==============================================================================

// ------------------------------------------------------------------------------
// 80. Resolve target SDL display index from config or current mouse cursor
// ------------------------------------------------------------------------------
static int resolveTargetDisplayIndex(const FaceAppConfig& cfg)
{
    int numDisplays = SDL_GetNumVideoDisplays();
    if (numDisplays <= 0) {
        return 0;
    }
    if (cfg.display >= 0 && cfg.display < numDisplays) {
        return cfg.display;
    }
    int mx = 0;
    int my = 0;
    SDL_GetGlobalMouseState(&mx, &my);
    for (int i = 0; i < numDisplays; ++i) {
        SDL_Rect b;
        if (SDL_GetDisplayBounds(i, &b) == 0 && mx >= b.x && mx < b.x + b.w && my >= b.y && my < b.y + b.h) {
            return i;
        }
    }
    return 0;
}

// ------------------------------------------------------------------------------
// 81. Check whether numbered PNG asset exists in assets/<folder>/
// ------------------------------------------------------------------------------
static bool assetFileExists(const std::string& assetsDir, const char* folder, uint8_t idx)
{
    if (idx == 0) {
        return true;
    }
    std::array<char, 16> fileName{};
    std::snprintf(fileName.data(), fileName.size(), "%03u.png", idx);
    return fs::exists(fs::path(assetsDir) / folder / fileName.data());
}

// ------------------------------------------------------------------------------
// 82. Show OSD banner indicating stepped asset index or (EMPTY) status
// ------------------------------------------------------------------------------
static void showAssetStepOsd(const std::string& assetsDir, const char* label, const char* folder, uint8_t idx)
{
    std::array<char, 96> buf{};
    bool exists = assetFileExists(assetsDir, folder, idx);
    if (idx == 0) {
        std::snprintf(buf.data(), buf.size(), "%s [000/255] OFF", label);
    } else if (exists) {
        std::snprintf(buf.data(), buf.size(), "%s [%03u/255]", label, idx);
    } else {
        std::snprintf(buf.data(), buf.size(), "%s [%03u/255] (EMPTY)", label, idx);
    }
    showOsd(buf.data(), 2400, false);
}

// ------------------------------------------------------------------------------
// 83. Save active memory slot, overrides & offsets to INI and packs/default.zip
// ------------------------------------------------------------------------------
static void performUnifiedSave(SDL_Window* window, FaceAppConfig& cfg, AppRuntimeState& state)
{
    std::array<char, 96> buf{};
    pushUndoSnapshot(cfg.assets_dir, cfg.config_file, "packs");
    int targetSlot = (state.manualMemorySlot >= 0) ? state.manualMemorySlot : state.browseMemorySlot;
    if ((targetSlot / 10) != state.memoryBank) {
        targetSlot = resolveBankKeySlot(state.memoryBank, targetSlot % 10);
    }
    captureMemoryCombo(cfg, state, targetSlot);
    saveIniConfig(window, cfg, state);
    state.browseMemorySlot  = targetSlot;
    state.manualMemorySlot  = targetSlot;
    state.appliedMemorySlot = targetSlot;
    state.isEditingOffsets  = false;

    bool okZip = saveCurrentAssetsToZipPack("default", cfg.assets_dir, cfg.config_file, "packs");
    if (okZip) {
        std::snprintf(buf.data(), buf.size(), "SAVED MEMORY #%03d + DEFAULT.ZIP [ESC/F3=AUTO]", targetSlot);
    } else {
        std::snprintf(buf.data(), buf.size(), "SAVED MEMORY #%03d [BANK %02d] [ESC/F3=AUTO]", targetSlot, state.memoryBank);
    }
    showOsd(buf.data(), 2400, false);
}

// ------------------------------------------------------------------------------
// 84. Handle keyboard controls inside Live Zip Pack Inspector overlay
// ------------------------------------------------------------------------------
static bool handlePackInspectorKeys(SDL_Window* window, SDL_Keycode sym, FaceAppConfig& cfg, AppRuntimeState& state)
{
    if (!state.packBrowser.open || !state.packBrowser.inspectMode) {
        return false;
    }
    int count = static_cast<int>(state.packBrowser.inspectItems.size());
    std::array<char, 96> buf{};

    if (sym == SDLK_ESCAPE || sym == SDLK_LEFT) {
        state.packBrowser.inspectMode = false;
        reloadAllTexturesAndState(SDL_GetRenderer(window), cfg, state);
        return true;
    }
    if (sym == SDLK_UP && count > 0) {
        state.packBrowser.inspectIdx = (state.packBrowser.inspectIdx - 1 + count) % count;
        return true;
    }
    if (sym == SDLK_DOWN && count > 0) {
        state.packBrowser.inspectIdx = (state.packBrowser.inspectIdx + 1) % count;
        return true;
    }
    if (sym == SDLK_SPACE && count > 0) {
        auto& item = state.packBrowser.inspectItems.at(state.packBrowser.inspectIdx);
        item.selected = !item.selected;
        return true;
    }
    if (sym == SDLK_a && count > 0) {
        bool anyUnchecked = false;
        for (const auto& it : state.packBrowser.inspectItems) {
            if (!it.selected) {
                anyUnchecked = true;
            }
        }
        for (auto& it : state.packBrowser.inspectItems) {
            it.selected = anyUnchecked;
        }
        return true;
    }
    if ((sym == SDLK_RETURN || sym == SDLK_KP_ENTER) && count > 0) {
        std::string pName = state.packBrowser.entries.at(state.packBrowser.selectedIdx).name;
        int checkedCount = 0;
        for (const auto& it : state.packBrowser.inspectItems) {
            if (it.selected) {
                ++checkedCount;
            }
        }
        if (appendCheckedItemsFromZipPack(window, pName, state.packBrowser, cfg, state, "packs")) {
            state.packBrowser.inspectMode = false;
            state.packBrowser.open        = false;
            std::snprintf(buf.data(), buf.size(), "IMPORTED %d ITEMS FROM %s.ZIP", checkedCount, pName.c_str());
            showOsd(buf.data(), 2800, false);
        }
        return true;
    }
    return true;
}
// ------------------------------------------------------------------------------
// 85. Handle keyboard controls inside Thumbnail Pack Browser (F6 / X)
// ------------------------------------------------------------------------------
static bool handlePackBrowserKeys(SDL_Window* window, SDL_Keycode sym, bool ctrlDown, FaceAppConfig& cfg, AppRuntimeState& state)
{
    if (!state.packBrowser.open) {
        return false;
    }
    if (state.packBrowser.inspectMode) {
        return handlePackInspectorKeys(window, sym, cfg, state);
    }

    std::array<char, 96> buf{};
    int count = static_cast<int>(state.packBrowser.entries.size());
    if (sym == SDLK_ESCAPE || sym == SDLK_F6 || sym == SDLK_x) {
        if (ctrlDown && sym == SDLK_x) {
            return false;
        }
        state.packBrowser.open        = false;
        state.packBrowser.wipeConfirm = false;
        return true;
    }
    if (sym == SDLK_LEFT && count > 0) {
        state.packBrowser.selectedIdx = (state.packBrowser.selectedIdx - 1 + count) % count;
        state.packBrowser.wipeConfirm = false;
        return true;
    }
    if (sym == SDLK_RIGHT && count > 0) {
        state.packBrowser.selectedIdx = (state.packBrowser.selectedIdx + 1) % count;
        state.packBrowser.wipeConfirm = false;
        return true;
    }
    if ((sym == SDLK_i || sym == SDLK_DOWN) && count > 0) {
        std::string pName = state.packBrowser.entries.at(state.packBrowser.selectedIdx).name;
        if (openZipPackInspector(pName, state.packBrowser, "packs")) {
            reloadDynamicEyeBank("packs/.inspect_tmp/assets");
        }
        return true;
    }
    if (sym == SDLK_m) {
        state.packBrowser.open = false;
        openFlatImportStudio(SDL_GetRenderer(window), state.flatImport, "import");
        return true;
    }
    if ((sym == SDLK_RETURN || sym == SDLK_KP_ENTER) && count > 0) {
        std::string pName = state.packBrowser.entries.at(state.packBrowser.selectedIdx).name;
        if (appendZipPackToActive(window, pName, cfg, state, "packs")) {
            refreshPackBrowserEntries(SDL_GetRenderer(window), state.packBrowser, "packs");
            std::snprintf(buf.data(), buf.size(), "APPENDED %s.ZIP -> DEFAULT.ZIP", pName.c_str());
            showOsd(buf.data(), 2800, false);
        }
        state.packBrowser.wipeConfirm = false;
        return true;
    }
    if (sym == SDLK_r && count > 0) {
        std::string pName = state.packBrowser.entries.at(state.packBrowser.selectedIdx).name;
        if (replaceActiveWithZipPack(window, pName, cfg, state, "packs")) {
            refreshPackBrowserEntries(SDL_GetRenderer(window), state.packBrowser, "packs");
            std::snprintf(buf.data(), buf.size(), "LOADED %s.ZIP AS DEFAULT", pName.c_str());
            showOsd(buf.data(), 2800, false);
        }
        state.packBrowser.wipeConfirm = false;
        return true;
    }
    if (sym == SDLK_DELETE) {
        if (!state.packBrowser.wipeConfirm) {
            state.packBrowser.wipeConfirm = true;
            return true;
        }
        wipeAllAssetsAndMacros(window, cfg, state, "packs");
        refreshPackBrowserEntries(SDL_GetRenderer(window), state.packBrowser, "packs");
        state.packBrowser.wipeConfirm = false;
        showOsd("WIPED ALL ASSETS -> CLEAN DEFAULT.ZIP", 2800, false);
        return true;
    }
    if (sym == SDLK_F7 || (ctrlDown && sym == SDLK_z)) {
        if (restoreUndoSnapshot(window, cfg, state, "packs")) {
            refreshPackBrowserEntries(SDL_GetRenderer(window), state.packBrowser, "packs");
            showOsd("UNDO: RESTORED PREVIOUS DEFAULT.ZIP", 2600, false);
        } else {
            showOsd("NOTHING TO UNDO", 2000, false);
        }
        return true;
    }
    return true;
}
// ------------------------------------------------------------------------------
// 85.1 Format 2-line FX Director OSD banner for selected FX slot (0..24)
// ------------------------------------------------------------------------------
static void showFxDirectorOsd(int slotIdx, AppRuntimeState& state)
{
    static const std::array<const char*, 25> kFxNames = {
        "BAT", "NOSE FLY", "MOSQUITO", "BIRD", "WASP",
        "SPIDER", "BUTTERFLY", "UFO", "COBWEB", "SMOKE / FOG",
        "GHOST", "SNAKE", "FIRE", "LIGHTNING", "MOUSE",
        "FIREFLIES", "GRASSHOPPER", "ANTS", "AUTUMN LEAVES", "FIREWORKS",
        "SNOWFALL", "RAIN", "STARS", "COMETS", "AURORA"
    };
    if (slotIdx < 0 || slotIdx >= 25) return;
    state.isEditingOffsets = false;
    g_selected_fx_slot     = slotIdx;
    g_fx_osd_active        = true;

    uint8_t m = g_fx_depth_masks.at(slotIdx);
    std::string bits;
    bits.push_back((m & 8) ? '1' : '0');
    bits.push_back((m & 4) ? '1' : '0');
    bits.push_back((m & 2) ? '1' : '0');
    bits.push_back((m & 1) ? '1' : '0');

    std::string activeNames;
    if (m == 0) {
        activeNames = "OFF";
    } else {
        if (m & 1) activeNames += "SKY+";
        if (m & 2) activeNames += "FAR+";
        if (m & 4) activeNames += "MID+";
        if (m & 8) activeNames += "FRT+";
        if (!activeNames.empty()) activeNames.pop_back();
    }

    std::array<char, 96> buf{};
    std::snprintf(buf.data(), buf.size(), "%s [%02d/25] %s [%s]",
                  kFxNames.at(slotIdx), slotIdx + 1, bits.c_str(), activeNames.c_str());
    bool prevOsd = osd.enabled;
    osd.enabled = true;
    showOsd(buf.data(), 0, true);
    osd.enabled = prevOsd;
}

// ------------------------------------------------------------------------------
// 86. Handle memory banks, slot recall/copy, fade timing & Alt+1..4 FX layers
// ------------------------------------------------------------------------------
static bool handleMemoryAndFxKeys(SDL_Window* window, SDL_Keycode sym, bool shiftDown, bool ctrlDown, bool altDown, FaceAppConfig& cfg, AppRuntimeState& state)
{
    std::array<char, 96> buf{};
    if (sym == SDLK_PAGEUP || sym == SDLK_PAGEDOWN) {
        state.memoryBank = (sym == SDLK_PAGEUP) ? ((state.memoryBank >= MAX_MEMORY_BANK) ? 0 : state.memoryBank + 1)
                                                : ((state.memoryBank <= 0) ? MAX_MEMORY_BANK : state.memoryBank - 1);
        std::snprintf(buf.data(), buf.size(), "MEMORY BANK [%02d/24] (SLOTS %03d..%03d)", state.memoryBank, state.memoryBank * 10, std::min(MAX_MEMORY_SLOT, state.memoryBank * 10 + 9));
        showOsd(buf.data(), 2400, false);
        return true;
    }
    if (sym == SDLK_v) {
        cfg.fade_ms = shiftDown ? ((cfg.fade_ms <= 0) ? 2000 : cfg.fade_ms - 200) : ((cfg.fade_ms >= 2000) ? 0 : cfg.fade_ms + 200);
        std::snprintf(buf.data(), buf.size(), "MEMORY FADE: %.1FS [S=SAVE]", cfg.fade_ms / 1000.0f);
        showOsd(buf.data(), 2200, false);
        return true;
    }
    if (sym == SDLK_m) {
        int nextSlot = state.browseMemorySlot + (shiftDown ? -1 : 1);
        if (nextSlot > MAX_MEMORY_SLOT) nextSlot = 0;
        if (nextSlot < 0) nextSlot = MAX_MEMORY_SLOT;
        state.browseMemorySlot = nextSlot;
        state.memoryBank = nextSlot / 10;
        std::snprintf(buf.data(), buf.size(), "MEMORY #%03d [BANK %02d] (LOCKED) [S=SAVE]", nextSlot, state.memoryBank);
        triggerMemoryTransition(state, cfg, nextSlot, nextSlot, buf.data());
        return true;
    }
    if (sym >= SDLK_0 && sym <= SDLK_9) {
        int keyDigit = (sym - SDLK_0);
        if (ctrlDown && altDown && keyDigit == 0) {
            state.manualFxMask = 0;
            g_fx_depth_masks.fill(0);
            g_fx_osd_active = false;
            showOsd("ALL FX & DEPTH LAYERS: OFF", 2000, false);
            return true;
        }
        if (altDown && (keyDigit == 0 || (keyDigit >= 1 && keyDigit <= 4))) {
            int slot = (g_selected_fx_slot >= 0 && g_selected_fx_slot < 25) ? g_selected_fx_slot : 0;
            if (keyDigit == 0) {
                g_fx_depth_masks.at(slot) = 0;
            } else {
                uint8_t bit = static_cast<uint8_t>(1u << (keyDigit - 1));
                g_fx_depth_masks.at(slot) ^= bit;
            }
            showFxDirectorOsd(slot, state);
            return true;
        }
        if (altDown) {
            return false;
        }
        int targetSlot = resolveBankKeySlot(state.memoryBank, keyDigit);
        state.browseMemorySlot = targetSlot;
        if (ctrlDown) {
            pushUndoSnapshot(cfg.assets_dir, cfg.config_file, "packs");
            captureMemoryCombo(cfg, state, targetSlot);
            saveIniConfig(window, cfg, state);
            saveCurrentAssetsToZipPack("default", cfg.assets_dir, cfg.config_file, "packs");
            state.manualMemorySlot  = targetSlot;
            state.appliedMemorySlot = targetSlot;
            std::snprintf(buf.data(), buf.size(), "SAVED MEMORY #%03d [BANK %02d] + DEFAULT.ZIP", targetSlot, state.memoryBank);
            showOsd(buf.data(), 2600, false);
        } else {
            std::snprintf(buf.data(), buf.size(), "MEMORY #%03d [BANK %02d] (LOCKED)", targetSlot, state.memoryBank);
            triggerMemoryTransition(state, cfg, targetSlot, targetSlot, buf.data());
        }
        return true;
    }
    return false;
}

// ------------------------------------------------------------------------------
// 87. Handle Ctrl+TAB (switch Left/Right char), TAB+Arrows (move) & Alt (scale)
// ------------------------------------------------------------------------------
static bool handleOffsetCalibrationKeys(SDL_Keycode sym, bool shiftDown, bool ctrlDown, bool altDown, AppRuntimeState& state)
{
    std::array<char, 96> buf{};
    if (sym == SDLK_TAB && ctrlDown) {
        if (!state.dualProfileMode && state.dualFaceLayer.testIndex == 0) {
            state.dualFaceLayer.testIndex    = state.faceLayer.testIndex;
            state.dualMakeupLayer.testIndex  = state.makeupLayer.testIndex;
            state.dualEarsLayer.testIndex    = state.earsLayer.testIndex;
            state.dualBrowsLayer.testIndex   = state.browsLayer.testIndex;
            state.dualNoseLayer.testIndex    = state.noseLayer.testIndex;
            state.dualBeardLayer.testIndex   = state.beardLayer.testIndex;
            state.dualGlassesLayer.testIndex = state.glassesLayer.testIndex;
            state.dualHairLayer.testIndex    = state.hairLayer.testIndex;
            state.dualHatLayer.testIndex     = state.hatLayer.testIndex;
        }
        state.dualProfileMode     = true;
        state.editingRightProfile = !state.editingRightProfile;
        bool prevOsd = osd.enabled;
        osd.enabled  = true;
        showOsd(state.editingRightProfile ? "ACTIVE: RIGHT CHARACTER" : "ACTIVE: LEFT CHARACTER", 2200, false);
        osd.enabled  = prevOsd;
        return true;
    }
    if (sym == SDLK_TAB) {
        if (!state.isEditingOffsets) {
            state.isEditingOffsets = true;
            if (shiftDown) {
                state.selectedItemIdx = EDITABLE_ITEM_COUNT - 1;
            }
        } else if (shiftDown) {
            state.selectedItemIdx = (state.selectedItemIdx - 1 + EDITABLE_ITEM_COUNT) % EDITABLE_ITEM_COUNT;
        } else {
            state.selectedItemIdx = (state.selectedItemIdx + 1) % EDITABLE_ITEM_COUNT;
        }
        const char* name = "";
        int* ox = nullptr;
        int* oy = nullptr;
        float* sc = nullptr;
        getEditableTarget(state, state.selectedItemIdx, name, ox, oy, sc);
        std::snprintf(buf.data(), buf.size(), "%s [%+d,%+d %d%%] ALT=SIZE", name, *ox, *oy, static_cast<int>(std::round(*sc * 100.0f)));
        showOsd(buf.data(), 0, true);
        return true;
    }
    if (sym == SDLK_UP || sym == SDLK_DOWN || sym == SDLK_LEFT || sym == SDLK_RIGHT) {
        const char* name = "";
        int* ox = nullptr;
        int* oy = nullptr;
        float* sc = nullptr;
        getEditableTarget(state, state.selectedItemIdx, name, ox, oy, sc);
        state.isEditingOffsets = true;
        if (altDown) {
            float d = (shiftDown ? 0.10f : 0.02f) * ((sym == SDLK_UP || sym == SDLK_RIGHT) ? 1.0f : -1.0f);
            *sc = std::max(0.50f, std::min(1.60f, *sc + d));
            g_eye_scale_factor.store(state.eyeScale, std::memory_order_relaxed);
            g_mouth_scale_factor.store(state.mouthScale, std::memory_order_relaxed);
            g_mouth_scale_factor_right.store(state.rightMouthScale, std::memory_order_relaxed);
            std::snprintf(buf.data(), buf.size(), "%s SIZE: %d%% [S=SAVE]", name, static_cast<int>(std::round(*sc * 100.0f)));
        } else {
            int step = shiftDown ? 10 : 2;
            if (sym == SDLK_UP) *oy -= step;
            if (sym == SDLK_DOWN) *oy += step;
            if (sym == SDLK_LEFT) *ox -= step;
            if (sym == SDLK_RIGHT) *ox += step;
            std::snprintf(buf.data(), buf.size(), "%s X:%+d Y:%+d [S=SAVE]", name, *ox, *oy);
        }
        showOsd(buf.data(), 0, true);
        return true;
    }
    return false;
}

// ------------------------------------------------------------------------------
// 88. Handle 0..255 layers for active Left/Right char, backgrounds & animations
// ------------------------------------------------------------------------------
static void handleAssetAndStyleKeys(SDL_Keycode sym, bool shiftDown, bool ctrlDown, const FaceAppConfig& cfg, AppRuntimeState& state)
{
    std::array<char, 96> buf{};
    static int s_irisStep = 0;
    bool rMode = (state.dualProfileMode && state.editingRightProfile);

    auto stepTargetLayer = [&](const char* label, const char* folder, PaperDollLayer& lMain, PaperDollLayer& lRight) {
        PaperDollLayer& target = rMode ? lRight : lMain;
        target.testIndex = stepAssetIndex(target.testIndex, 255, shiftDown);
        std::array<char, 40> fullLbl{};
        std::snprintf(fullLbl.data(), fullLbl.size(), "%s%s", rMode ? "R-" : "", label);
        showAssetStepOsd(cfg.assets_dir, fullLbl.data(), folder, target.testIndex);
    };

    switch (sym) {
        case SDLK_k: {
            state.bgLayer.testIndex = stepAssetIndex(state.bgLayer.testIndex, 255, shiftDown);
            std::array<char, 24> skyFn{};
            std::snprintf(skyFn.data(), skyFn.size(), "%03u_sky.png", state.bgLayer.testIndex);
            bool hasBg = (state.bgLayer.testIndex == 0) ||
                         fs::exists(fs::path(cfg.assets_dir) / "bg" / skyFn.data()) ||
                         assetFileExists(cfg.assets_dir, "bg", state.bgLayer.testIndex);
            if (state.bgLayer.testIndex == 0) std::snprintf(buf.data(), buf.size(), "BACKGROUND [000/255] OFF");
            else if (hasBg) std::snprintf(buf.data(), buf.size(), "BACKGROUND [%03u/255]", state.bgLayer.testIndex);
            else std::snprintf(buf.data(), buf.size(), "BACKGROUND [%03u/255] (EMPTY)", state.bgLayer.testIndex);
            showOsd(buf.data(), 2400, false);
            break;
        }
        case SDLK_b: {
            if (ctrlDown) {
                state.manualFxMask = 0;
                g_fx_depth_masks.fill(0);
                g_fx_osd_active = false;
                showOsd("ALL FX & DEPTH LAYERS: OFF", 2000, false);
                break;
            }
            if (!g_fx_osd_active || g_selected_fx_slot < 0) {
                if (g_selected_fx_slot < 0) g_selected_fx_slot = 0;
            } else {
                g_selected_fx_slot = (g_selected_fx_slot + (shiftDown ? 24 : 1)) % 25;
            }
            showFxDirectorOsd(g_selected_fx_slot, state);
            break;
        }
        case SDLK_a: {
            int nextAnim = state.manualTestAnim + (shiftDown ? -1 : 1);
            if (nextAnim > 64) nextAnim = 0;
            if (nextAnim < 0) nextAnim = 64;
            state.manualTestAnim = nextAnim;
            if (nextAnim == 0) std::snprintf(buf.data(), buf.size(), "ANIM [00/64] AUTO LOOP");
            else std::snprintf(buf.data(), buf.size(), "ANIM [%02d/64] %s", nextAnim, getAnimationName(nextAnim));
            showOsd(buf.data(), 2600, false);
            break;
        }
        case SDLK_l: {
            std::atomic<int>& targetLip = rMode ? g_override_lip_color_right : g_override_lip_color;
            int cur = (targetLip.load() < 0) ? g_override_lip_color.load() : targetLip.load();
            int nl  = (cur + (shiftDown ? 9 : 1)) % 10;
            targetLip = nl;
            std::snprintf(buf.data(), buf.size(), "%sLIP COLOR: %d", rMode ? "R-" : "", nl);
            showOsd(buf.data(), 2600, false);
            break;
        }
        case SDLK_t: {
            std::atomic<int>& targetTeeth = rMode ? g_override_teeth_right : g_override_teeth;
            int cur = (targetTeeth.load() < 0) ? g_override_teeth.load() : targetTeeth.load();
            int nt  = (cur + (shiftDown ? 15 : 1)) % 16;
            targetTeeth = nt;
            std::snprintf(buf.data(), buf.size(), "%sTEETH PRESET: %d", rMode ? "R-" : "", nt);
            showOsd(buf.data(), 2400, false);
            break;
        }
        case SDLK_e: {
            int nl = (g_override_eyelash.load() + (shiftDown ? 15 : 1)) % 16;
            g_override_eyelash = nl;
            std::snprintf(buf.data(), buf.size(), (nl == 0 ? "EYELASHES: OFF" : "EYELASHES: [%02d/15]"), nl);
            showOsd(buf.data(), 2600, false);
            break;
        }
        case SDLK_o: {
            static const std::array<uint8_t, 12> kV = {10, 32, 53, 74, 96, 117, 138, 160, 181, 202, 224, 245};
            static const std::array<const char*, 12> kN = {
                "DEFAULT SLY", "DEFAULT ROUND", "DEFAULT ALMOND", "DRAGON SLY", "DRAGON ROUND", "DRAGON ALMOND",
                "DEF SLY [NO LID]", "DEF ROUND [NO LID]", "DEF ALMOND [NO LID]", "DRG SLY [NO LID]", "DRG ROUND [NO LID]", "DRG ALMOND [NO LID]"
            };
            int curOv = g_override_eye_style.load();
            int curIdx = -1;
            for (int i = 0; i < 12; ++i) {
                if (curOv == static_cast<int>(kV.at(i))) curIdx = i;
            }
            int nextIdx = curIdx + (shiftDown ? -1 : 1);
            if (nextIdx > 11) nextIdx = -1;
            if (nextIdx < -1) nextIdx = 11;
            if (nextIdx < 0) {
                g_override_eye_style = -1;
                std::snprintf(buf.data(), buf.size(), "EYE SHAPE: AUTO");
            } else {
                g_override_eye_style = static_cast<int>(kV.at(nextIdx));
                std::snprintf(buf.data(), buf.size(), "EYE [%02d/12] %s", nextIdx + 1, kN.at(nextIdx));
            }
            showOsd(buf.data(), 2600, false);
            break;
        }
        case SDLK_i: {
            if (state.isDmxActive) { showOsd("DMX ACTIVE - IRIS LOCKED", 2000, false); break; }
            if (g_override_iris.load() < 0) s_irisStep = 0;
            s_irisStep = (s_irisStep + (shiftDown ? 10 : 1)) % 11;
            if (s_irisStep == 0) {
                g_override_iris = -1;
                std::snprintf(buf.data(), buf.size(), "IRIS SIZE: AUTO");
            } else {
                g_override_iris = ((s_irisStep - 1) * 255) / 9;
                std::snprintf(buf.data(), buf.size(), "IRIS SIZE: [%02d/10]", s_irisStep);
            }
            showOsd(buf.data(), 2600, false);
            break;
        }
        case SDLK_y: {
            state.dualProfileMode = !state.dualProfileMode;
            if (state.dualProfileMode && state.dualFaceLayer.testIndex == 0) {
                state.dualFaceLayer.testIndex    = state.faceLayer.testIndex;
                state.dualMakeupLayer.testIndex  = state.makeupLayer.testIndex;
                state.dualEarsLayer.testIndex    = state.earsLayer.testIndex;
                state.dualBrowsLayer.testIndex   = (state.browsLayer.testIndex >= PROFILE_FACE_START_INDEX) ? state.browsLayer.testIndex : state.faceLayer.testIndex;
                state.dualNoseLayer.testIndex    = (state.noseLayer.testIndex >= PROFILE_FACE_START_INDEX) ? state.noseLayer.testIndex : state.faceLayer.testIndex;
                state.dualBeardLayer.testIndex   = state.beardLayer.testIndex;
                state.dualGlassesLayer.testIndex = state.glassesLayer.testIndex;
                state.dualHairLayer.testIndex    = state.hairLayer.testIndex;
                state.dualHatLayer.testIndex     = state.hatLayer.testIndex;
            }
            if (!state.dualProfileMode) state.editingRightProfile = false;
            bool prevOsd = osd.enabled;
            osd.enabled  = true;
            showOsd(state.dualProfileMode ? "DUAL PROFILE: ON [CTRL+TAB=SWITCH]" : "SINGLE PROFILE: ON", 2200, false);
            osd.enabled  = prevOsd;
            break;
        }
        case SDLK_u:
            if (ctrlDown) { state.manualHideMouth = !state.manualHideMouth; showOsd(state.manualHideMouth ? "MOUTH: HIDDEN" : "MOUTH: VISIBLE", 2200, false); }
            break;
        case SDLK_n:
            if (ctrlDown) { state.manualHideNose = !state.manualHideNose; showOsd(state.manualHideNose ? "NOSE: HIDDEN" : "NOSE: VISIBLE", 2200, false); }
            else stepTargetLayer("NOSE", "nose", state.noseLayer, state.dualNoseLayer);
            break;
        case SDLK_w:
            if (ctrlDown) { state.manualHideBrows = !state.manualHideBrows; showOsd(state.manualHideBrows ? "BROWS: HIDDEN" : "BROWS: VISIBLE", 2200, false); }
            else stepTargetLayer("BROWS", "brows", state.browsLayer, state.dualBrowsLayer);
            break;
        case SDLK_f: stepTargetLayer("FACE",    "face",    state.faceLayer,    state.dualFaceLayer);    break;
        case SDLK_r: stepTargetLayer("EARS",    "ears",    state.earsLayer,    state.dualEarsLayer);    break;
        case SDLK_j: stepTargetLayer("BEARD",   "beard",   state.beardLayer,   state.dualBeardLayer);   break;
        case SDLK_g: stepTargetLayer("GLASSES", "glasses", state.glassesLayer, state.dualGlassesLayer); break;
        case SDLK_h: stepTargetLayer("HAIR",    "hair",    state.hairLayer,    state.dualHairLayer);    break;
        case SDLK_c: stepTargetLayer("HAT",     "hat",     state.hatLayer,     state.dualHatLayer);     break;
        case SDLK_p: stepTargetLayer("MAKEUP",  "makeup",  state.makeupLayer,  state.dualMakeupLayer);  break;
        default: break;
    }
}

// ------------------------------------------------------------------------------
// 89. Export complete QLC+ (.qxw) show with 2 Eye XY-Pads, Sliders & Memories
// ------------------------------------------------------------------------------
static bool exportQlcPlusShowFile(const FaceAppConfig& cfg, const std::string& outPath)
{
    std::string mfgStr   = "Magnus Herrlin";
    std::string modStr   = "MakingFaces";
    std::string modeName = "39-Channel Full";
    int chCount          = 39;
    int memChannelIdx    = 31;
    int dimChannelIdx    = 38;

    if (cfg.dmx_mode == 1) {
        modeName = "1-Channel Memory"; chCount = 1; memChannelIdx = 0; dimChannelIdx = -1;
    } else if (cfg.dmx_mode == 2) {
        modeName = "2-Channel Memory + Dimmer"; chCount = 2; memChannelIdx = 0; dimChannelIdx = 1;
    } else if (cfg.dmx_mode == 7) {
        modeName = "7-Channel Show"; chCount = 7; memChannelIdx = 0; dimChannelIdx = 1;
    }

    if (fs::exists("MakingFaces.qxf")) {
        const char* home = std::getenv("HOME");
        if (home && *home) {
            fs::path userFixDir = fs::path(home) / ".qlcplus" / "fixtures";
            std::error_code ec;
            fs::create_directories(userFixDir, ec);
            fs::copy_file("MakingFaces.qxf", userFixDir / "MakingFaces.qxf", fs::copy_options::overwrite_existing, ec);
        }
        std::ifstream qxf("MakingFaces.qxf");
        std::string xml((std::istreambuf_iterator<char>(qxf)), std::istreambuf_iterator<char>());
        auto extractTag = [&](const std::string& tag, std::string& dst) {
            size_t a = xml.find("<" + tag + ">");
            size_t b = (a != std::string::npos) ? xml.find("</" + tag + ">", a) : std::string::npos;
            if (a != std::string::npos && b != std::string::npos) {
                a += tag.size() + 2;
                dst = xml.substr(a, b - a);
            }
        };
        extractTag("Manufacturer", mfgStr);
        extractTag("Model", modStr);

        std::string targetPrefix = std::to_string(chCount) + "-";
        size_t pos = 0;
        while ((pos = xml.find("<Mode Name=\"", pos)) != std::string::npos) {
            pos += 12;
            size_t endQ = xml.find('"', pos);
            if (endQ == std::string::npos) break;
            std::string cand = xml.substr(pos, endQ - pos);
            if (cand.rfind(targetPrefix, 0) == 0 || cand.find(std::to_string(chCount)) != std::string::npos) {
                modeName = cand;
                break;
            }
        }
    }

    std::ofstream out(outPath);
    if (!out.is_open()) {
        return false;
    }

    int uniIdx = (cfg.universe > 0) ? (cfg.universe - 1) : 0;
    int chAddr = (cfg.start_channel > 0) ? (cfg.start_channel - 1) : 0;

    out << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<!DOCTYPE Workspace>\n"
        << "<Workspace xmlns=\"http://qlcplus.org\" CurrentWindow=\"VirtualConsole\">\n"
        << " <Creator><Name>Q Light Controller Plus</Name><Version>4.14.1</Version><Author>Magnus Herrlin</Author></Creator>\n"
        << " <Engine>\n  <InputOutputMap>\n   <Universe Name=\"Universe 1\" ID=\"" << uniIdx << "\">\n"
        << "    <Output Plugin=\"" << (cfg.proto == "sacn" ? "E1.31" : "ArtNet") << "\" Line=\"0\"/>\n   </Universe>\n  </InputOutputMap>\n"
        << "  <Fixture>\n   <Manufacturer>" << mfgStr << "</Manufacturer>\n   <Model>" << modStr << "</Model>\n"
        << "   <Mode>" << modeName << "</Mode>\n   <ID>0</ID>\n   <Name>MakingFaces Head</Name>\n"
        << "   <Universe>" << uniIdx << "</Universe>\n   <Address>" << chAddr << "</Address>\n   <Channels>" << chCount << "</Channels>\n  </Fixture>\n";

    std::vector<std::pair<int, std::string>> sceneButtons;
    out << "  <Function ID=\"0\" Type=\"Scene\" Name=\"Auto Loop (DMX Idle)\">\n"
        << "   <Speed FadeIn=\"0\" FadeOut=\"0\" Duration=\"0\"/>\n   <FixtureVal ID=\"0\">" << memChannelIdx << ",0</FixtureVal>\n  </Function>\n";
    sceneButtons.push_back({0, "AUTO LOOP"});

    int funcId = 1;
    int audioTrigFuncId = -1;
    int fxFireFuncId    = -1;
    int fxFogFuncId     = -1;

    if (cfg.dmx_mode == 39) {
        audioTrigFuncId = funcId++;
        out << "  <Function ID=\"" << audioTrigFuncId << "\" Type=\"Scene\" Name=\"Audio Play Trigger\">\n   <Speed FadeIn=\"0\" FadeOut=\"0\" Duration=\"0\"/>\n   <FixtureVal ID=\"0\">37,255</FixtureVal>\n  </Function>\n";
        fxFireFuncId = funcId++;
        out << "  <Function ID=\"" << fxFireFuncId << "\" Type=\"Scene\" Name=\"FX Fire Overlay\">\n   <Speed FadeIn=\"0\" FadeOut=\"0\" Duration=\"0\"/>\n   <FixtureVal ID=\"0\">34,13</FixtureVal>\n  </Function>\n";
        fxFogFuncId = funcId++;
        out << "  <Function ID=\"" << fxFogFuncId << "\" Type=\"Scene\" Name=\"FX All Fog Layers\">\n   <Speed FadeIn=\"0\" FadeOut=\"0\" Duration=\"0\"/>\n   <FixtureVal ID=\"0\">34,23</FixtureVal>\n  </Function>\n";
    }

    for (const auto& kv : cfg.memory_combos) {
        if (!kv.second.enabled) {
            continue;
        }
        int dmxVal = std::min(250, 10 + kv.first);
        std::array<char, 64> sName{};
        std::snprintf(sName.data(), sName.size(), "Memory #%03d (%s)", kv.first, getAnimationName(kv.second.anim_id));
        out << "  <Function ID=\"" << funcId << "\" Type=\"Scene\" Name=\"" << sName.data() << "\">\n   <Speed FadeIn=\"0\" FadeOut=\"0\" Duration=\"0\"/>\n   <FixtureVal ID=\"0\">" << memChannelIdx << "," << dmxVal << "</FixtureVal>\n  </Function>\n";
        std::array<char, 48> btnLabel{};
        std::snprintf(btnLabel.data(), btnLabel.size(), "#%03d %s", kv.first, getAnimationName(kv.second.anim_id));
        sceneButtons.push_back({funcId, btnLabel.data()});
        ++funcId;
    }
        int cols = 5, btnW = 155, btnH = 42;
    int rows = std::max(1, (static_cast<int>(sceneButtons.size()) + cols - 1) / cols);
    int soloW = 16 + cols * (btnW + 6), soloH = 38 + rows * (btnH + 6);
    int leftColW = (cfg.dmx_mode == 39) ? 540 : 110;
    int frameW = leftColW + soloW + 24, frameH = std::max(460, soloH + 36);

    out << " </Engine>\n <VirtualConsole>\n  <Frame Caption=\"MakingFaces Show Control\">\n"
        << "   <Appearance><FrameStyle>None</FrameStyle><ForegroundColor>Default</ForegroundColor><BackgroundColor>Default</BackgroundColor></Appearance>\n"
        << "   <WindowState Visible=\"True\" X=\"0\" Y=\"0\" Width=\"" << frameW << "\" Height=\"" << frameH << "\"/>\n";

    int widgetId = 0;
    if (cfg.dmx_mode == 39) {
        auto writeEyePad = [&](const char* cap, int px, int py, int chX, int chY) {
            out << "   <XYPad Caption=\"" << cap << "\" ID=\"" << widgetId++ << "\" InvertedAppearance=\"false\">\n"
                << "    <WindowState Visible=\"True\" X=\"" << px << "\" Y=\"" << py << "\" Width=\"168\" Height=\"168\"/>\n"
                << "    <Appearance><FrameStyle>Sunken</FrameStyle><ForegroundColor>Default</ForegroundColor><BackgroundColor>Default</BackgroundColor></Appearance>\n"
                << "    <Fixture ID=\"0\" Head=\"0\"><Axis ID=\"X\" LowLimit=\"0\" HighLimit=\"255\" Reverse=\"False\"/><Axis ID=\"Y\" LowLimit=\"0\" HighLimit=\"255\" Reverse=\"False\"/></Fixture>\n"
                << "    <Channel Fixture=\"0\">" << chX << "</Channel>\n"
                << "    <Channel Fixture=\"0\">" << chY << "</Channel>\n"
                << "    <Pan Position=\"128\"/><Tilt Position=\"128\"/>\n   </XYPad>\n";
        };
        writeEyePad("Left Eye",  12,  32, 0, 1);
        writeEyePad("Right Eye", 186, 32, 6, 7);

        auto writeSlider = [&](const char* cap, int sx, int sy, int sw, int sh, int chIdx, int initVal) {
            out << "   <Slider Caption=\"" << cap << "\" ID=\"" << widgetId++ << "\" WidgetStyle=\"Slider\" InvertedAppearance=\"false\">\n"
                << "    <WindowState Visible=\"True\" X=\"" << sx << "\" Y=\"" << sy << "\" Width=\"" << sw << "\" Height=\"" << sh << "\"/>\n"
                << "    <Appearance><FrameStyle>Sunken</FrameStyle><ForegroundColor>Default</ForegroundColor><BackgroundColor>Default</BackgroundColor></Appearance>\n"
                << "    <SliderMode ValueDisplayStyle=\"Exact\" ClickAndGoType=\"None\" Monitor=\"false\">Level</SliderMode>\n"
                << "    <Level LowLimit=\"0\" HighLimit=\"255\" Value=\"" << initVal << "\"><Channel Fixture=\"0\">" << chIdx << "</Channel></Level>\n   </Slider>\n";
        };
        writeSlider("Skelning", 12, 208, 342, 60, 12, 128);
        writeSlider("Dimmer",   364, 32,  52, 195, 38, 0);
        writeSlider("Mouth",    420, 32,  52, 195, 15, 0);
        writeSlider("Pupil",    476, 32,  52, 195, 2,  128);
        writeSlider("Mood",     364, 234, 52, 195, 16, 128);
        writeSlider("Track",    420, 234, 52, 195, 35, 0);
        writeSlider("Volume",   476, 234, 52, 195, 36, 200);

        auto writeBtn = [&](const char* cap, int bx, int by, int bw, int bh, int fId) {
            if (fId < 0) return;
            out << "   <Button Caption=\"" << cap << "\" ID=\"" << widgetId++ << "\" Icon=\"\">\n"
                << "    <WindowState Visible=\"True\" X=\"" << bx << "\" Y=\"" << by << "\" Width=\"" << bw << "\" Height=\"" << bh << "\"/>\n"
                << "    <Appearance><FrameStyle>None</FrameStyle><ForegroundColor>Default</ForegroundColor><BackgroundColor>Default</BackgroundColor></Appearance>\n"
                << "    <Function ID=\"" << fId << "\"/><Action>Toggle</Action><Intensity Adjust=\"False\">100</Intensity>\n   </Button>\n";
        };
        writeBtn("PLAY AUDIO", 12,  278, 108, 52, audioTrigFuncId);
        writeBtn("FX FIRE",    128, 278, 108, 52, fxFireFuncId);
        writeBtn("FX FOG",     244, 278, 108, 52, fxFogFuncId);
    } else if (dimChannelIdx >= 0) {
        out << "   <Slider Caption=\"Dimmer\" ID=\"" << widgetId++ << "\" WidgetStyle=\"Slider\" InvertedAppearance=\"false\">\n"
            << "    <WindowState Visible=\"True\" X=\"12\" Y=\"32\" Width=\"76\" Height=\"280\"/>\n"
            << "    <Appearance><FrameStyle>Sunken</FrameStyle><ForegroundColor>Default</ForegroundColor><BackgroundColor>Default</BackgroundColor></Appearance>\n"
            << "    <SliderMode ValueDisplayStyle=\"Exact\" ClickAndGoType=\"None\" Monitor=\"false\">Level</SliderMode>\n"
            << "    <Level LowLimit=\"0\" HighLimit=\"255\" Value=\"0\"><Channel Fixture=\"0\">" << dimChannelIdx << "</Channel></Level>\n   </Slider>\n";
    }

    int soloX = leftColW + 12;
    out << "   <SoloFrame Caption=\"Memories (Active Slot)\" ID=\"" << widgetId++ << "\">\n"
        << "    <Appearance><FrameStyle>Sunken</FrameStyle><ForegroundColor>Default</ForegroundColor><BackgroundColor>Default</BackgroundColor></Appearance>\n"
        << "    <WindowState Visible=\"True\" X=\"" << soloX << "\" Y=\"32\" Width=\"" << soloW << "\" Height=\"" << soloH << "\"/>\n"
        << "    <AllowChildren>True</AllowChildren><AllowResize>True</AllowResize><ShowHeader>True</ShowHeader><ShowEnableButton>True</ShowEnableButton><Collapsed>False</Collapsed><Disabled>False</Disabled>\n";
    for (size_t i = 0; i < sceneButtons.size(); ++i) {
        int bx = 8 + (static_cast<int>(i) % cols) * (btnW + 6);
        int by = 32 + (static_cast<int>(i) / cols) * (btnH + 6);
        out << "    <Button Caption=\"" << sceneButtons.at(i).second << "\" ID=\"" << widgetId++ << "\" Icon=\"\">\n"
            << "     <WindowState Visible=\"True\" X=\"" << bx << "\" Y=\"" << by << "\" Width=\"" << btnW << "\" Height=\"" << btnH << "\"/>\n"
            << "     <Appearance><FrameStyle>None</FrameStyle><ForegroundColor>Default</ForegroundColor><BackgroundColor>Default</BackgroundColor></Appearance>\n"
            << "     <Function ID=\"" << sceneButtons.at(i).first << "\"/><Action>Toggle</Action><Intensity Adjust=\"False\">100</Intensity>\n    </Button>\n";
    }
    out << "   </SoloFrame>\n  </Frame>\n </VirtualConsole>\n</Workspace>\n";
    return true;
}

// ------------------------------------------------------------------------------
// 90. Main SDL keyboard router dispatching to camera, browsers & asset keys
// ------------------------------------------------------------------------------
static void handleKeyboardEvent(SDL_Window* window, SDL_Keycode sym, SDL_Keymod mod, FaceAppConfig& cfg, AppRuntimeState& state)
{
    bool shiftDown = (mod & KMOD_SHIFT) != 0;
    bool ctrlDown  = (mod & KMOD_CTRL) != 0;
    bool altDown   = (mod & KMOD_ALT) != 0;
    std::array<char, 96> buf{};

    auto clearFxEmitterParticles = [&]() {
        for (const auto& a : g_stage.actorList()) {
            if (a && a->groupId == 100) {
                a->alive = false;
            }
        }
    };

    if (ctrlDown && (sym == SDLK_EQUALS || sym == SDLK_PLUS || sym == SDLK_KP_PLUS || sym == SDLK_MINUS || sym == SDLK_KP_MINUS)) {
        bool inc = (sym == SDLK_EQUALS || sym == SDLK_PLUS || sym == SDLK_KP_PLUS);
        if (inc) {
            g_critter_swarm_count = (g_critter_swarm_count == 1) ? 3 : std::min(50, g_critter_swarm_count + 4);
        } else {
            g_critter_swarm_count = (g_critter_swarm_count <= 3) ? 1 : std::max(1, g_critter_swarm_count - 4);
        }
        bool prevOsd = osd.enabled;
        osd.enabled = true;
        std::snprintf(buf.data(), buf.size(), "CRITTER / SWARM COUNT: %d", g_critter_swarm_count);
        showOsd(buf.data(), 2200, false);
        osd.enabled = prevOsd;
        return;
    }
    if (ctrlDown && (sym == SDLK_F9 || sym == SDLK_k)) {
        g_stage_fx_enabled = !g_stage_fx_enabled;
        if (!g_stage_fx_enabled) {
            clearFxEmitterParticles();
        }
        bool prevOsd = osd.enabled;
        osd.enabled = true;
        showOsd(g_stage_fx_enabled ? "GLOWING EMBERS / PARTICLES: ON" : "GLOWING EMBERS / PARTICLES: OFF", 2400, false);
        osd.enabled = prevOsd;
        return;
    }
    if (ctrlDown && (sym == SDLK_F10 || sym == SDLK_g)) {
        static const std::array<const char*, 10> kModeNames = {
            "STILL", "GRAVITY FALL", "RISING PLUME", "PERLIN SWIRL", "SWARM FLOCK", "GROUND CRAWL", "AUDIO PULSE", "AUTUMN LEAVES", "FIREWORKS", "FIREFLIES"
        };
        g_stage_fx_enabled = true;
        g_stage_fx_mode = (g_stage_fx_mode + 1) % 10;
        clearFxEmitterParticles();
        bool prevOsd = osd.enabled;
        osd.enabled = true;
        std::snprintf(buf.data(), buf.size(), "PHYSICS MODE: %s", kModeNames.at(g_stage_fx_mode));
        showOsd(buf.data(), 2400, false);
        osd.enabled = prevOsd;
        return;
    }
    if (ctrlDown && (sym == SDLK_LEFT || sym == SDLK_RIGHT || sym == SDLK_DOWN)) {
        if (sym == SDLK_LEFT)       g_parallax_vel_x = 48;
        else if (sym == SDLK_RIGHT) g_parallax_vel_x = 208;
        else { g_parallax_vel_x = 128; g_parallax_vel_y = 128; }
        std::snprintf(buf.data(), buf.size(), "PARALLAX VEL X:%u Y:%u (128=STOP)", g_parallax_vel_x, g_parallax_vel_y);
        showOsd(buf.data(), 2000, false);
        return;
    }

    if (handleCameraViewfinderKeys(window, sym, state)) return;
    if (handleFlatImportKeys(window, sym, shiftDown, cfg, state)) return;
    if (handlePackBrowserKeys(window, sym, ctrlDown, cfg, state)) return;
    if (handleEyeBrowserKeys(sym, state)) return;
    if (handleMemoryAndFxKeys(window, sym, shiftDown, ctrlDown, altDown, cfg, state)) return;
    if (handleOffsetCalibrationKeys(sym, shiftDown, ctrlDown, altDown, state)) return;

    switch (sym) {
        case SDLK_ESCAPE:
            if (g_show_about_overlay) {
                g_show_about_overlay = false;
            } else if (state.showHelpOverlay) {
                state.showHelpOverlay = false;
            } else if (state.showTelemetry) {
                state.showTelemetry = false;
            } else if (g_fx_osd_active) {
                g_fx_osd_active = false;
                hideOsd();
            } else if (state.isEditingOffsets) {
                state.isEditingOffsets = false;
                hideOsd();
            } else if (state.manualMemorySlot >= 0 || state.manualTestAnim > 0) {
                resetAutonomousShow();
                triggerMemoryTransition(state, cfg, -1, 0, "AUTO SHOW RESUMED");
            }
            break;
        case SDLK_q:
            if (!ctrlDown && !altDown) {
                g_running = false;
            }
            break;
        case SDLK_z:
            if (ctrlDown) {
                if (restoreUndoSnapshot(window, cfg, state, "packs")) {
                    showOsd("UNDO: RESTORED PREVIOUS DEFAULT.ZIP", 2600, false);
                } else {
                    showOsd("NOTHING TO UNDO", 2000, false);
                }
            }
            break;
        case SDLK_e:
            if (ctrlDown) {
                saveIniConfig(window, cfg, state);
                bool ok = exportQlcPlusShowFile(cfg, "MakingFaces-Show.qxw");
                std::printf("[QLC+] Exported MakingFaces-Show.qxw: %s\n", ok ? "OK" : "FAILED");
                std::fflush(stdout);
                showOsd(ok ? "EXPORTED QLC+ SHOW: MAKINGFACES-SHOW.QXW" : "ERROR EXPORTING QLC+ SHOW!", 2800, false);
                return;
            }
            handleAssetAndStyleKeys(sym, shiftDown, ctrlDown, cfg, state);
            break;
        case SDLK_i:
            if (ctrlDown) {
                openFlatImportStudio(SDL_GetRenderer(window), state.flatImport, "import");
                return;
            }
            handleAssetAndStyleKeys(sym, shiftDown, ctrlDown, cfg, state);
            break;
        case SDLK_p:
            if (ctrlDown) {
                openCameraViewfinder(state.cameraCapture);
                return;
            }
            handleAssetAndStyleKeys(sym, shiftDown, ctrlDown, cfg, state);
            break;
        case SDLK_F1:
            state.showHelpOverlay = !state.showHelpOverlay;
            g_show_about_overlay  = false;
            break;
        case SDLK_F2:
            state.showTelemetry = !state.showTelemetry;
            showOsd(state.showTelemetry ? "TELEMETRY: ON" : "TELEMETRY: OFF", 1600, false);
            break;
        case SDLK_F3:
            resetAutonomousShow();
            triggerMemoryTransition(state, cfg, -1, 0, "AUTO SHOW RESET -> SLOT #000");
            break;
        case SDLK_F4:
        case SDLK_s:
            performUnifiedSave(window, cfg, state);
            break;
        case SDLK_F5:
            state.osdEnabled = !state.osdEnabled;
            osd.enabled      = state.osdEnabled;
            if (!state.osdEnabled) {
                hideOsd();
            } else {
                showOsd("OSD: ON", 1600, false);
            }
            break;
        case SDLK_F6:
            state.packBrowser.open        = true;
            state.packBrowser.inspectMode = false;
            state.packBrowser.wipeConfirm = false;
            refreshPackBrowserEntries(SDL_GetRenderer(window), state.packBrowser, "packs");
            break;
        case SDLK_F7:
            if (restoreUndoSnapshot(window, cfg, state, "packs")) {
                showOsd("UNDO: RESTORED PREVIOUS DEFAULT.ZIP", 2600, false);
            } else {
                showOsd("NOTHING TO UNDO", 2000, false);
            }
            break;
        case SDLK_F8:
            state.eyeBrowser.open = true;
            refreshEyeBrowserEntries(SDL_GetRenderer(window), state.eyeBrowser, cfg.assets_dir);
            break;
        case SDLK_F9:
            g_show_about_overlay  = !g_show_about_overlay;
            state.showHelpOverlay = false;
            break;
        case SDLK_F11:
            toggleWindowFullscreen(window, state.isFullscreen);
            break;
        case SDLK_F12:
            state.manualBlackout = !state.manualBlackout;
            showOsd(state.manualBlackout ? "BLACKOUT: ON" : "BLACKOUT: OFF", 1800, false);
            break;
        case SDLK_x:
            if (ctrlDown) {
                saveIniConfig(window, cfg, state);
                std::string newName = nextCustomZipPackName("packs");
                if (saveCurrentAssetsToZipPack(newName, cfg.assets_dir, cfg.config_file, "packs")) {
                    std::snprintf(buf.data(), buf.size(), "EXPORTED NEW ZIP: %s.ZIP", newName.c_str());
                    showOsd(buf.data(), 2800, false);
                } else {
                    showOsd("ERROR EXPORTING ZIP PACK!", 2400, false);
                }
            } else {
                state.packBrowser.open        = true;
                state.packBrowser.inspectMode = false;
                state.packBrowser.wipeConfirm = false;
                refreshPackBrowserEntries(SDL_GetRenderer(window), state.packBrowser, "packs");
            }
            break;
        default:
            handleAssetAndStyleKeys(sym, shiftDown, ctrlDown, cfg, state);
            break;
    }
}

// ------------------------------------------------------------------------------
// 91. Render calibrated import overlay item & oval eye/inpaint guides
// ------------------------------------------------------------------------------
static void renderFlatImportLiveLayer(SDL_Renderer* ren, const FlatImportState& fi, int targetCat)
{
    if (!fi.open || fi.items.empty()) {
        return;
    }
    const auto& it = fi.items.at(fi.selectedIdx);
    if ((targetCat >= 0 && it.targetCatIdx != targetCat) || !it.previewTex) {
        return;
    }

    if (it.targetCatIdx >= 10) {
        SDL_Rect box{172, 160, 256, 256};
        SDL_SetRenderDrawColor(ren, 14, 18, 28, 235);
        SDL_RenderFillRect(ren, &box);
        SDL_RenderCopy(ren, it.previewTex, nullptr, &box);
        SDL_SetRenderDrawColor(ren, 0, 255, 110, 255);
        SDL_RenderDrawRect(ren, &box);
        drawEllipseOutline(ren, 300, 288, 108, 108);
        drawEllipseOutline(ren, 300, 288, 36, 36);
        return;
    }

    for (int cy = 0; cy < BASE_CANVAS_H; cy += 24) {
        for (int cx = 0; cx < BASE_CANVAS_W; cx += 24) {
            bool dark = (((cx / 24) + (cy / 24)) % 2) == 0;
            SDL_SetRenderDrawColor(ren, dark ? 18 : 26, dark ? 22 : 31, dark ? 28 : 40, 255);
            SDL_Rect tile{cx, cy, 24, 24};
            SDL_RenderFillRect(ren, &tile);
        }
    }

    SDL_SetRenderDrawColor(ren, 65, 85, 115, 180);
    drawEllipseOutline(ren, 300, 380, 200, 265);
    drawEllipseOutline(ren, 190, 316, 64, 45);
    drawEllipseOutline(ren, 410, 316, 64, 45);
    drawEllipseOutline(ren, 300, 565, 94, 52);
    SDL_RenderCopy(ren, it.previewTex, nullptr, nullptr);

    if (fi.dragTarget == 0) {
        SDL_SetRenderDrawColor(ren, 0, 255, 110, 180);
        SDL_Rect frame{2, 2, BASE_CANVAS_W - 4, BASE_CANVAS_H - 4};
        SDL_RenderDrawRect(ren, &frame);
    }

    auto drawGuideOval = [&](int id, int cx, int cy, int rx, int ry) {
        if (fi.dragTarget == id) {
            SDL_SetRenderDrawColor(ren, 0, 255, 110, 255);
            drawEllipseOutline(ren, cx, cy, rx, ry);
            drawEllipseOutline(ren, cx, cy, rx + 1, ry + 1);
        } else {
            SDL_SetRenderDrawColor(ren, 0, 229, 255, 190);
            drawEllipseOutline(ren, cx, cy, rx, ry);
        }
    };

    if (it.targetCatIdx == 0 || it.targetCatIdx == 8) {
        if (it.cutEyes) {
            drawGuideOval(1, it.leftEyeX, it.leftEyeY, it.leftEyeRx, it.leftEyeRy);
            drawGuideOval(2, it.rightEyeX, it.rightEyeY, it.rightEyeRx, it.rightEyeRy);
        }
        if (it.doInpaint) {
            drawGuideOval(3, it.noseX, it.noseY, it.noseRx, it.noseRy);
            drawGuideOval(4, it.mouthX, it.mouthY, it.mouthRx, it.mouthRy);
        }
    }
}
// ------------------------------------------------------------------------------
// 92. Render eyes, mouth, paper-doll layers and independent Dual characters
// ------------------------------------------------------------------------------
static void renderCharacterLayers(
    SDL_Renderer* renderer, const AppRuntimeState& state,
    SDL_Texture* eyesTexture, SDL_Texture* featuresTexture,
    uint8_t reqFace, uint8_t reqBeard, uint8_t reqHair,
    bool hideMouth, bool hideBrows)
{
    bool isAnyProfile      = (reqFace >= PROFILE_FACE_START_INDEX);
    bool isDualProfile     = (isAnyProfile && state.dualProfileMode);
    bool isSingleLeftProf  = (isAnyProfile && !isDualProfile && (reqFace % 2 == 1));
    bool isSingleRightProf = (isAnyProfile && !isDualProfile && (reqFace % 2 == 0));
    int fy        = GLOBAL_FACE_Y_OFFSET;
    int fSplit    = isAnyProfile ? state.faceLayer.splitOffsetX : 0;
    int eyeShiftL = (isSingleLeftProf || isDualProfile) ? (-5 - fSplit) : 0;
    int eyeShiftR = (isSingleRightProf || isDualProfile) ? (5 + fSplit) : 0;
    bool showEyeL = !isSingleRightProf;
    bool showEyeR = !isSingleLeftProf;

    if (state.faceLayer.texture) {
        int sockRx = isAnyProfile ? 66 : 82;
        int sockRy = isAnyProfile ? 54 : 64;
        if (showEyeL) drawOvalShadow(renderer, 190 + eyeShiftL + state.eyeLeftOffsetX, 355 + fy + state.eyeLeftOffsetY, sockRx, sockRy, 255);
        if (showEyeR) drawOvalShadow(renderer, 410 + eyeShiftR + state.eyeRightOffsetX, 355 + fy + state.eyeRightOffsetY, sockRx, sockRy, 255);
    }

    SDL_Rect eyeSrcL{0, 0, BASE_CANVAS_W / 2, BASE_CANVAS_H};
    SDL_Rect eyeDstL{state.eyeLeftOffsetX + eyeShiftL, state.eyeLeftOffsetY, BASE_CANVAS_W / 2, BASE_CANVAS_H};
    SDL_Rect eyeSrcR{BASE_CANVAS_W / 2, 0, BASE_CANVAS_W / 2, BASE_CANVAS_H};
    SDL_Rect eyeDstR{(BASE_CANVAS_W / 2) + state.eyeRightOffsetX + eyeShiftR, state.eyeRightOffsetY, BASE_CANVAS_W / 2, BASE_CANVAS_H};
    if (showEyeL && !isEyePoppedOut(0)) SDL_RenderCopy(renderer, eyesTexture, &eyeSrcL, &eyeDstL);
    if (showEyeR && !isEyePoppedOut(1)) SDL_RenderCopy(renderer, eyesTexture, &eyeSrcR, &eyeDstR);

    auto drawSplit = [&](const PaperDollLayer& layer, int extraY, int extraSplit) {
        if (!layer.texture) return;
        float sc  = layer.scale;
        int halfW = static_cast<int>(std::round((BASE_CANVAS_W / 2) * sc));
        int fullH = static_cast<int>(std::round(BASE_CANVAS_H * sc));
        int dy    = (BASE_CANVAS_H - fullH) / 2;
        if (showEyeL) {
            SDL_Rect sL{0, 0, BASE_CANVAS_W / 2, BASE_CANVAS_H};
            SDL_Rect dL{(BASE_CANVAS_W / 2) - halfW + layer.offsetX - layer.splitOffsetX - extraSplit, layer.offsetY + fy + extraY + dy, halfW, fullH};
            SDL_RenderCopy(renderer, layer.texture, &sL, &dL);
        }
        if (showEyeR) {
            SDL_Rect sR{BASE_CANVAS_W / 2, 0, BASE_CANVAS_W / 2, BASE_CANVAS_H};
            SDL_Rect dR{(BASE_CANVAS_W / 2) + layer.offsetX + layer.splitOffsetX + extraSplit, layer.offsetY + fy + extraY + dy, halfW, fullH};
            SDL_RenderCopy(renderer, layer.texture, &sR, &dR);
        }
    };

    float rCharSc = state.dualFaceLayer.scale;
    int rCharOx   = state.dualFaceLayer.offsetX;
    int rCharOy   = state.dualFaceLayer.offsetY;

    auto drawProf = [&](const PaperDollLayer& lL, const PaperDollLayer& lR, int extraY, bool isFaceLayer) {
        float scL  = lL.scale;
        int fullWL = static_cast<int>(std::round(BASE_CANVAS_W * scL));
        int fullHL = static_cast<int>(std::round(BASE_CANVAS_H * scL));
        int dxL    = (BASE_CANVAS_W - fullWL) / 2;
        int dyL    = (BASE_CANVAS_H - fullHL) / 2;

        if (isDualProfile) {
            if (lL.texture) {
                SDL_Rect dL{lL.offsetX - fSplit + dxL, lL.offsetY + fy + extraY + dyL, fullWL, fullHL};
                SDL_RenderCopy(renderer, lL.texture, nullptr, &dL);
            }
            if (lR.texture) {
                float scR  = isFaceLayer ? rCharSc : (rCharSc * lR.scale);
                int fullWR = static_cast<int>(std::round(BASE_CANVAS_W * scR));
                int fullHR = static_cast<int>(std::round(BASE_CANVAS_H * scR));
                int dxR    = (BASE_CANVAS_W - fullWR) / 2;
                int dyR    = (BASE_CANVAS_H - fullHR) / 2;
                int oxR    = isFaceLayer ? rCharOx : (rCharOx + lR.offsetX);
                int oyR    = isFaceLayer ? rCharOy : (rCharOy + lR.offsetY);
                SDL_Rect dR{oxR + fSplit + dxR, oyR + fy + extraY + dyR, fullWR, fullHR};
                SDL_RenderCopy(renderer, lR.texture, nullptr, &dR);
            }
        } else if (lL.texture) {
            int sh = isSingleLeftProf ? (lL.offsetX - fSplit) : (lL.offsetX + fSplit);
            SDL_Rect d{sh + dxL, lL.offsetY + fy + extraY + dyL, fullWL, fullHL};
            SDL_RenderCopy(renderer, lL.texture, nullptr, &d);
        }
    };

    bool effHideMouth = hideMouth || (reqBeard >= JAW_LAYER_START_INDEX && reqBeard <= 108);
    bool animateJaw   = (reqBeard >= JAW_LAYER_START_INDEX) || (effHideMouth && reqBeard > 0);
    int jawDrop       = animateJaw ? static_cast<int>(std::round((g_effective_mouth_gap.load() / 255.0f) * 48.0f)) : 0;

    if (!isAnyProfile) drawSplit(state.earsLayer, 0, 0);
    drawProf(state.faceLayer, state.dualFaceLayer, 0, true);
    drawProf(state.makeupLayer, state.dualMakeupLayer, 0, false);
    if (isAnyProfile) drawProf(state.earsLayer, state.dualEarsLayer, 0, false);

    if (!effHideMouth) {
        if (isDualProfile) {
            SDL_Rect mSrcL{0, 0, BASE_CANVAS_W / 2, BASE_CANVAS_H};
            SDL_Rect mDstL{state.mouthOffsetX, state.mouthOffsetY, BASE_CANVAS_W / 2, BASE_CANVAS_H};
            SDL_Rect mSrcR{BASE_CANVAS_W / 2, 0, BASE_CANVAS_W / 2, BASE_CANVAS_H};
            SDL_Rect mDstR{(BASE_CANVAS_W / 2) + state.rightMouthOffsetX, state.rightMouthOffsetY, BASE_CANVAS_W / 2, BASE_CANVAS_H};
            SDL_RenderCopy(renderer, featuresTexture, &mSrcL, &mDstL);
            SDL_RenderCopy(renderer, featuresTexture, &mSrcR, &mDstR);
        } else {
            SDL_Rect d{state.mouthOffsetX, state.mouthOffsetY, BASE_CANVAS_W, BASE_CANVAS_H};
            SDL_RenderCopy(renderer, featuresTexture, nullptr, &d);
        }
    }
    drawProf(state.noseLayer, state.dualNoseLayer, 0, false);

    if (!hideBrows) {
        if (isAnyProfile && state.browsLayer.loadedIndex >= PROFILE_FACE_START_INDEX) {
            drawProf(state.browsLayer, state.dualBrowsLayer, 0, false);
        } else if (state.browsLayer.texture) {
            renderDynamicEyebrowsSplit(renderer, state.browsLayer, g_brow_left_tilt.load(), g_brow_right_tilt.load(), showEyeL, showEyeR, fSplit);
        }
    }

    if (isAnyProfile && state.beardLayer.loadedIndex >= PROFILE_FACE_START_INDEX) drawProf(state.beardLayer, state.dualBeardLayer, jawDrop, false);
    else drawSplit(state.beardLayer, jawDrop, fSplit);

    if (isAnyProfile && state.glassesLayer.loadedIndex >= PROFILE_FACE_START_INDEX) drawProf(state.glassesLayer, state.dualGlassesLayer, 0, false);
    else drawSplit(state.glassesLayer, 0, fSplit);

    drawProf(state.hairLayer, state.dualHairLayer, 0, false);
    drawProf(state.hatLayer, state.dualHatLayer, (reqHair > 0) ? -24 : 0, false);

    if (showEyeL && isEyePoppedOut(0)) {
        float f = getEyePopOutFactor(0);
        drawOvalShadow(renderer, 190 + eyeShiftL + state.eyeLeftOffsetX + 10, 348 + fy + state.eyeLeftOffsetY + 26, static_cast<int>(62 + f * 24), static_cast<int>(54 + f * 20), 135);
        SDL_RenderCopy(renderer, eyesTexture, &eyeSrcL, &eyeDstL);
    }
    if (showEyeR && isEyePoppedOut(1)) {
        float f = getEyePopOutFactor(1);
        drawOvalShadow(renderer, 410 + eyeShiftR + state.eyeRightOffsetX + 10, 348 + fy + state.eyeRightOffsetY + 26, static_cast<int>(62 + f * 24), static_cast<int>(54 + f * 20), 135);
        SDL_RenderCopy(renderer, eyesTexture, &eyeSrcR, &eyeDstR);
    }
}

// ------------------------------------------------------------------------------
// 93. Render foreground parallax/Stage FRONT layer, UI overlays & master blackout
// ------------------------------------------------------------------------------
static void renderForegroundAndOverlays(
    SDL_Renderer* renderer, const FaceAppConfig& cfg, const AppRuntimeState& state,
    const SDL_Rect& bgVp, const SDL_Rect& faceVp, float scale,
    int outW, int outH, uint32_t reqFxMask, bool hideAll, uint8_t transitionFadeAlpha, uint32_t nowMs)
{
    (void)reqFxMask;
    SDL_RenderSetScale(renderer, 1.0f, 1.0f);
    SDL_RenderSetViewport(renderer, &bgVp);
    if (!hideAll && state.parallaxBg.frontLayer) {
        int tw = 0, th = 0;
        SDL_QueryTexture(state.parallaxBg.frontLayer, nullptr, nullptr, &tw, &th);
        SDL_Rect src = (tw > 8 && th > 4) ? SDL_Rect{2, 0, tw - 4, th - 2} : SDL_Rect{0, 0, tw, th};
        SDL_Rect full{0, 0, bgVp.w, bgVp.h + 2};
        SDL_RenderCopy(renderer, state.parallaxBg.frontLayer, &src, &full);
    }

    float fxScaleX = (static_cast<float>(outW) / static_cast<float>(BASE_CANVAS_W)) * state.fxLayer.scale;
    float fxScaleY = (static_cast<float>(outH) / static_cast<float>(BASE_CANVAS_H)) * state.fxLayer.scale;
    int fxW = static_cast<int>(std::round(BASE_CANVAS_W * fxScaleX));
    int fxH = static_cast<int>(std::round(BASE_CANVAS_H * fxScaleY));
    SDL_Rect fxVp{
        (outW - fxW) / 2 + static_cast<int>(std::round(state.fxLayer.offsetX * scale)),
        (outH - fxH) / 2 + static_cast<int>(std::round(state.fxLayer.offsetY * scale)),
        fxW,
        fxH
    };
    SDL_RenderSetScale(renderer, 1.0f, 1.0f);
    SDL_RenderSetViewport(renderer, &fxVp);
    SDL_RenderSetScale(renderer, fxScaleX, fxScaleY);
    if (!hideAll) {
        g_stage.renderLayer(renderer, DepthLayer::FRONT, 1.0f);
    }

    SDL_RenderSetScale(renderer, 1.0f, 1.0f);
    SDL_RenderSetViewport(renderer, &faceVp);
    SDL_RenderSetScale(renderer, scale, scale);
    renderBrowsersAndTelemetry(renderer, cfg, state);
    renderOsdOverlay(renderer, nowMs, state);
    SDL_RenderSetScale(renderer, 1.0f, 1.0f);
    SDL_RenderSetViewport(renderer, nullptr);

    uint8_t totalBlackout = std::max(state.masterDimmer, transitionFadeAlpha);
    if (totalBlackout > 0) {
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, totalBlackout);
        SDL_Rect r{0, 0, outW, outH};
        SDL_RenderFillRect(renderer, &r);
    }
    SDL_RenderPresent(renderer);
}
// ------------------------------------------------------------------------------
// 94. Main frame compositor orchestrating backdrop, character, FX & overlays
// ------------------------------------------------------------------------------
static void renderCompositedFrame(
    SDL_Renderer* renderer, const FaceAppConfig& cfg, const AppRuntimeState& state,
    SDL_Texture* eyesTexture, SDL_Texture* featuresTexture,
    uint8_t reqFace, uint8_t reqBeard, uint8_t reqHair, uint32_t reqFxMask,
    bool hideAll, bool hideMouth, bool hideBrows, uint8_t transitionFadeAlpha, uint32_t nowMs)
{
    int outW = BASE_CANVAS_W, outH = BASE_CANVAS_H;
    SDL_GetRendererOutputSize(renderer, &outW, &outH);
    float scale = std::min(static_cast<float>(outW) / BASE_CANVAS_W, static_cast<float>(outH) / BASE_CANVAS_H);
    int vpW = static_cast<int>(std::round(BASE_CANVAS_W * scale));
    int vpH = static_cast<int>(std::round(BASE_CANVAS_H * scale));
    SDL_Rect faceVp{(outW - vpW) / 2, (outH - vpH) / 2, vpW, vpH};

    SDL_SetRenderTarget(renderer, nullptr);
    SDL_RenderSetScale(renderer, 1.0f, 1.0f);
    SDL_RenderSetViewport(renderer, nullptr);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    if (state.cameraCapture.open || state.flatImport.open) {
        SDL_RenderSetViewport(renderer, &faceVp);
        SDL_RenderSetScale(renderer, scale, scale);
        if (state.flatImport.open) renderFlatImportLiveLayer(renderer, state.flatImport, -1);
        renderBrowsersAndTelemetry(renderer, cfg, state);
        SDL_RenderSetScale(renderer, 1.0f, 1.0f);
        SDL_RenderSetViewport(renderer, nullptr);
        SDL_RenderPresent(renderer);
        return;
    }

    int bgW = static_cast<int>(std::round(outW * state.bgLayer.scale));
    int bgH = static_cast<int>(std::round(outH * state.bgLayer.scale));
    SDL_Rect bgVp{(outW - bgW) / 2 + static_cast<int>(std::round(state.bgLayer.offsetX * scale)),
                  (outH - bgH) / 2 + static_cast<int>(std::round(state.bgLayer.offsetY * scale)), bgW, bgH};
    SDL_RenderSetViewport(renderer, &bgVp);
    if (!hideAll) {
        renderDepthBackdropLayers(renderer, state.parallaxBg, reqFxMask, nowMs, bgW, bgH);
    }

    float fxScaleX = (static_cast<float>(outW) / static_cast<float>(BASE_CANVAS_W)) * state.fxLayer.scale;
    float fxScaleY = (static_cast<float>(outH) / static_cast<float>(BASE_CANVAS_H)) * state.fxLayer.scale;
    int fxW = static_cast<int>(std::round(BASE_CANVAS_W * fxScaleX));
    int fxH = static_cast<int>(std::round(BASE_CANVAS_H * fxScaleY));
    SDL_Rect fxVp{
        (outW - fxW) / 2 + static_cast<int>(std::round(state.fxLayer.offsetX * scale)),
        (outH - fxH) / 2 + static_cast<int>(std::round(state.fxLayer.offsetY * scale)),
        fxW,
        fxH
    };

    if (!hideAll) {
        SDL_RenderSetScale(renderer, 1.0f, 1.0f);
        SDL_RenderSetViewport(renderer, &fxVp);
        SDL_RenderSetScale(renderer, fxScaleX, fxScaleY);
        g_stage.renderLayer(renderer, DepthLayer::FAR, 0.25f);
        g_stage.renderLayer(renderer, DepthLayer::MID, 0.65f);

        SDL_RenderSetScale(renderer, 1.0f, 1.0f);
        SDL_RenderSetViewport(renderer, &faceVp);
        SDL_RenderSetScale(renderer, scale, scale);
        renderCharacterLayers(renderer, state, eyesTexture, featuresTexture, reqFace, reqBeard, reqHair, hideMouth, hideBrows);
    }

    SDL_RenderSetScale(renderer, 1.0f, 1.0f);
    SDL_RenderSetViewport(renderer, nullptr);
    renderForegroundAndOverlays(renderer, cfg, state, bgVp, faceVp, scale, outW, outH, reqFxMask, hideAll, transitionFadeAlpha, nowMs);
}

// ------------------------------------------------------------------------------
// 95. Update DMX connection state, online/offline transitions & crossfade alpha
// ------------------------------------------------------------------------------
static void updateDmxAndFadeState(const FaceAppConfig& cfg, AppRuntimeState& state, uint32_t frameStart, uint32_t lastPktMs)
{
    state.isDmxActive = (lastPktMs > 0 && (frameStart - lastPktMs) <= DMX_TIMEOUT_MS);
    if (state.isDmxActive != state.wasDmxActive) {
        if (state.isDmxActive) {
            state.manualMemorySlot  = -1;
            state.manualTestAnim    = 0;
            state.isEditingOffsets  = false;
            state.manualHideMouth   = false;
            state.manualHideNose    = false;
            state.manualHideBrows   = false;
            g_override_teeth        = -1;
            g_override_eyelash      = -1;
            g_override_lip_color    = -1;
            g_override_eye_style    = -1;
            g_override_iris         = -1;
        } else {
            state.dmxMemoryLocked   = false;
            state.appliedMemorySlot = -1;
            resetAutonomousShow();
            applyMemoryComboToState(cfg, state, 0, false);
            applyMaskOffsetProfile(cfg, state, state.faceLayer.testIndex);
        }
        state.wasDmxActive = state.isDmxActive;
    }

    if (state.fadePhase == 1) {
        uint32_t halfDur = std::max(1, cfg.fade_ms / 2);
        uint32_t elapsed = frameStart - state.fadeStartMs;
        if (elapsed >= halfDur) {
            executePendingMemorySwitch(cfg, state);
            state.fadePhase = 2;
            state.fadeAlpha = 255;
        } else {
            state.fadeAlpha = static_cast<uint8_t>((elapsed * 255U) / halfDur);
        }
    } else if (state.fadePhase == 2) {
        uint32_t halfDur = std::max(1, cfg.fade_ms / 2);
        uint32_t elapsed = (frameStart - state.fadeStartMs) - halfDur;
        if (elapsed >= halfDur) {
            state.fadePhase = 0;
            state.fadeAlpha = 0;
        } else {
            state.fadeAlpha = static_cast<uint8_t>(255U - (elapsed * 255U) / halfDur);
        }
    } else {
        state.fadeAlpha = 0;
    }
}
// ------------------------------------------------------------------------------
// 96. Resolve active DMX/memory/inspector layers and load PNG textures
// ------------------------------------------------------------------------------
static void resolveActiveLayersAndAudio(
    SDL_Window* window, SDL_Renderer* renderer, FaceAppConfig& cfg, AppRuntimeState& state,
    uint32_t frameStart, uint8_t& outRawMem, bool& outDmxEditOverlay, uint8_t& outRawDimmer)
{
    int mode = cfg.dmx_mode;
    int memOffset = (mode == 39) ? DMX_OFFSET_MEMORY : 0;
    outRawMem = 0;
    uint8_t rawMemCtrl = 0;

    if (state.isDmxActive) {
        outRawMem = dmx_read_slot(0, getWrappedDmxIndex(cfg.start_channel, memOffset) + 1);
        if (mode == 39) {
            rawMemCtrl = dmx_read_slot(0, getWrappedDmxIndex(cfg.start_channel, DMX_OFFSET_MEMORY_CTRL) + 1);
        }
    }
    outDmxEditOverlay = state.isDmxActive && (mode == 39) && (rawMemCtrl >= 100 && rawMemCtrl <= 199);

    if (state.isDmxActive && isDmxMemoryActive(outRawMem)) {
        int slot = decodeDmxMemorySlot(outRawMem);
        if (!state.dmxMemoryLocked || slot != state.appliedMemorySlot) {
            state.dmxMemoryLocked = true;
            if (outDmxEditOverlay) {
                state.appliedMemorySlot = slot;
            } else {
                triggerMemoryTransition(state, cfg, -1, slot, nullptr);
            }
        }
        if (mode == 39 && rawMemCtrl >= 200) {
            if (state.dmxSaveHoldStart == 0) {
                state.dmxSaveHoldStart = frameStart;
            } else if (!state.dmxSaveTriggered && (frameStart - state.dmxSaveHoldStart) >= static_cast<uint32_t>(cfg.save_hold_ms)) {
                pushUndoSnapshot(cfg.assets_dir, cfg.config_file, "packs");
                captureMemoryCombo(cfg, state, slot);
                saveIniConfig(window, cfg, state);
                saveCurrentAssetsToZipPack("default", cfg.assets_dir, cfg.config_file, "packs");
                state.dmxSaveTriggered = true;
                std::array<char, 48> sbuf{};
                std::snprintf(sbuf.data(), sbuf.size(), "DMX SAVED MEMORY #%03d", slot);
                showOsd(sbuf.data(), 2600, false);
            }
        } else {
            state.dmxSaveHoldStart = 0;
            state.dmxSaveTriggered = false;
        }
    } else {
        if (state.dmxMemoryLocked) {
            state.dmxMemoryLocked         = false;
            state.appliedMemorySlot       = -1;
            g_override_teeth              = -1;
            g_override_teeth_right        = -1;
            g_override_eyelash            = -1;
            g_override_lip_color          = -1;
            g_override_lip_color_right    = -1;
            g_override_eye_style          = -1;
            g_override_iris               = -1;
        }
        state.dmxSaveHoldStart = 0;
        state.dmxSaveTriggered = false;
    }
    bool inspectingZip = (state.packBrowser.open && state.packBrowser.inspectMode && !state.packBrowser.inspectItems.empty());
    std::string activeAssetsDir = inspectingZip ? "packs/.inspect_tmp/assets" : cfg.assets_dir;
    static bool s_wasInspectingZip = false;
    if (inspectingZip != s_wasInspectingZip) {
        std::array<PaperDollLayer*, 19> allLyrs = {
            &state.bgLayer, &state.faceLayer, &state.makeupLayer, &state.earsLayer, &state.browsLayer,
            &state.noseLayer, &state.beardLayer, &state.glassesLayer, &state.hairLayer, &state.hatLayer,
            &state.dualFaceLayer, &state.dualMakeupLayer, &state.dualEarsLayer, &state.dualBrowsLayer,
            &state.dualNoseLayer, &state.dualBeardLayer, &state.dualGlassesLayer, &state.dualHairLayer, &state.dualHatLayer
        };
        for (auto* l : allLyrs) {
            if (l->texture) {
                SDL_DestroyTexture(l->texture);
                l->texture = nullptr;
            }
            l->loadedIndex = 0;
        }
        destroyParallaxBackdrop(state.parallaxBg);
        reloadDynamicEyeBank(activeAssetsDir);
        s_wasInspectingZip = inspectingZip;
    }

    if (inspectingZip) {
        const auto& item = state.packBrowser.inspectItems.at(state.packBrowser.inspectIdx);
        applyPresetValuesToState(item.preset, state, 0, true);
        FaceAppConfig tmpCfg{};
        loadIniConfig("packs/.inspect_tmp/makingfaces.ini", tmpCfg);
        applyMaskOffsetProfile(tmpCfg, state, state.faceLayer.testIndex);
    }

    bool useMemoryLayer = inspectingZip || state.dmxMemoryLocked || (mode != 39) || (state.manualMemorySlot >= 0) || (!state.isDmxActive && state.manualTestAnim == 0);
    if (state.isDmxActive && !state.dmxMemoryLocked && !inspectingZip && mode == 39) {
        state.curFace    = dmx_read_slot(0, getWrappedDmxIndex(cfg.start_channel, DMX_OFFSET_FACE_MASK) + 1);
        state.curMakeup  = dmx_read_slot(0, getWrappedDmxIndex(cfg.start_channel, DMX_OFFSET_MAKEUP_MASK) + 1);
        state.curEars    = dmx_read_slot(0, getWrappedDmxIndex(cfg.start_channel, DMX_OFFSET_EARS_MASK) + 1);
        state.curNose    = dmx_read_slot(0, getWrappedDmxIndex(cfg.start_channel, DMX_OFFSET_NOSE_MASK) + 1);
        state.curBeard   = dmx_read_slot(0, getWrappedDmxIndex(cfg.start_channel, DMX_OFFSET_BEARD_MASK) + 1);
        state.curGlasses = dmx_read_slot(0, getWrappedDmxIndex(cfg.start_channel, DMX_OFFSET_GLASSES_MASK) + 1);
        state.curHair    = dmx_read_slot(0, getWrappedDmxIndex(cfg.start_channel, DMX_OFFSET_HAIR_MASK) + 1);
        state.curHat     = dmx_read_slot(0, getWrappedDmxIndex(cfg.start_channel, DMX_OFFSET_HAT_MASK) + 1);
        state.curBrows   = dmx_read_slot(0, getWrappedDmxIndex(cfg.start_channel, DMX_OFFSET_BROW_MASK) + 1);
        state.curBgScene = dmx_read_slot(0, getWrappedDmxIndex(cfg.start_channel, DMX_OFFSET_BG_SCENE) + 1);
        uint8_t rawFx    = dmx_read_slot(0, getWrappedDmxIndex(cfg.start_channel, DMX_OFFSET_FX_OVERLAY) + 1);
        if (rawFx == 0) state.curFxMask = 0;
        else if (rawFx == 21) state.curFxMask = FX_BIT_FOG_MID;
        else if (rawFx == 22) state.curFxMask = FX_BIT_FOG_FRONT;
        else if (rawFx == 23) state.curFxMask = FX_BIT_FOG_BACK | FX_BIT_FOG_MID | FX_BIT_FOG_FRONT;
        else state.curFxMask = (1u << ((rawFx - 1) % 20));
        uint8_t rawTeeth  = dmx_read_slot(0, getWrappedDmxIndex(cfg.start_channel, DMX_OFFSET_MOUTH_TEETH) + 1);
        uint8_t rawLip    = dmx_read_slot(0, getWrappedDmxIndex(cfg.start_channel, DMX_OFFSET_LIP_COLOR) + 1);
        uint8_t rawLash   = dmx_read_slot(0, getWrappedDmxIndex(cfg.start_channel, DMX_OFFSET_EYELASH_STYLE) + 1);
        uint8_t rawStyle  = dmx_read_slot(0, getWrappedDmxIndex(cfg.start_channel, DMX_OFFSET_EYE_LEFT_STYLE) + 1);
        state.curTeeth    = std::min<int>(15, rawTeeth / 16);
        state.curLipColor = (rawLip == 0) ? 3 : std::max(1, std::min(9, 1 + ((static_cast<int>(rawLip) - 1) * 9) / 255));
        state.curEyelash  = (rawLash == 0) ? 0 : std::max(1, std::min(15, 1 + ((static_cast<int>(rawLash) - 1) * 14) / 254));
        state.curEyeStyle = static_cast<int>(rawStyle);
    }
    if (useMemoryLayer) {
        state.curFace    = state.faceLayer.testIndex;
        state.curMakeup  = state.makeupLayer.testIndex;
        state.curEars    = state.earsLayer.testIndex;
        state.curNose    = state.noseLayer.testIndex;
        state.curBeard   = state.beardLayer.testIndex;
        state.curGlasses = state.glassesLayer.testIndex;
        state.curHair    = state.hairLayer.testIndex;
        state.curHat     = state.hatLayer.testIndex;
        state.curBrows   = state.browsLayer.testIndex;
        state.curBgScene = state.bgLayer.testIndex;
        state.curFxMask  = state.manualFxMask;
    }
    if (state.isDmxActive && mode == 7 && !inspectingZip) {
        uint8_t dmxBg = dmx_read_slot(0, getWrappedDmxIndex(cfg.start_channel, 2) + 1);
        uint8_t dmxFx = dmx_read_slot(0, getWrappedDmxIndex(cfg.start_channel, 3) + 1);
        if (dmxBg > 0) state.curBgScene = dmxBg;
        if (dmxFx > 0) {
            if (dmxFx == 21) state.curFxMask = FX_BIT_FOG_MID;
            else if (dmxFx == 22) state.curFxMask = FX_BIT_FOG_FRONT;
            else if (dmxFx == 23) state.curFxMask = FX_BIT_FOG_BACK | FX_BIT_FOG_MID | FX_BIT_FOG_FRONT;
            else state.curFxMask = (1u << ((dmxFx - 1) % 20));
        }
    }
    if (!inspectingZip && state.curFace != state.activeProfileId) {
        applyMaskOffsetProfile(cfg, state, state.curFace);
    }

    bool isProf        = (state.curFace >= PROFILE_FACE_START_INDEX);
    bool isDual        = (isProf && state.dualProfileMode);
    bool isSingleRight = (isProf && !isDual && (state.curFace % 2 == 0));
    int  turnSpeaker   = isDual ? getActiveDialogueSpeaker() : 0;
    g_active_face_mask.store(state.curFace, std::memory_order_relaxed);
    g_dual_profile_active.store(isDual, std::memory_order_relaxed);
    g_profile_face_split_x.store(state.faceLayer.splitOffsetX, std::memory_order_relaxed);
    g_profile_mouth_split_x.store(state.mouthSplitX, std::memory_order_relaxed);
    g_dual_mouth_mood_mode.store(turnSpeaker, std::memory_order_relaxed);

    auto toLeftOdd = [](uint8_t idx) -> uint8_t {
        if (idx < PROFILE_FACE_START_INDEX) return idx;
        return (idx % 2 == 1) ? idx : static_cast<uint8_t>(idx - 1);
    };
    auto toRightEven = [](uint8_t idx) -> uint8_t {
        if (idx < PROFILE_FACE_START_INDEX) return idx;
        return (idx % 2 == 0) ? idx : static_cast<uint8_t>(idx + 1);
    };

    auto syncDualPair = [&](uint8_t reqLeft, PaperDollLayer& lMain, PaperDollLayer& lDual, bool isFace) {
        if (isDual) {
            uint8_t leftSlot  = (reqLeft >= PROFILE_FACE_START_INDEX) ? toLeftOdd(reqLeft) : reqLeft;
            uint8_t rawRight  = (isFace && lDual.testIndex == 0) ? reqLeft : lDual.testIndex;
            uint8_t rightSlot = (rawRight >= PROFILE_FACE_START_INDEX) ? toRightEven(rawRight) : rawRight;
            updatePaperDollLayer(renderer, activeAssetsDir, leftSlot, lMain);
            updatePaperDollLayer(renderer, activeAssetsDir, rightSlot, lDual);
        } else if (isProf && reqLeft >= PROFILE_FACE_START_INDEX) {
            uint8_t singleSlot = isSingleRight ? toRightEven(reqLeft) : toLeftOdd(reqLeft);
            updatePaperDollLayer(renderer, activeAssetsDir, singleSlot, lMain);
            updatePaperDollLayer(renderer, activeAssetsDir, 0, lDual);
        } else {
            updatePaperDollLayer(renderer, activeAssetsDir, reqLeft, lMain);
            updatePaperDollLayer(renderer, activeAssetsDir, 0, lDual);
        }
    };

    updateParallaxBackdrop(renderer, activeAssetsDir, state.curBgScene, state.parallaxBg);
    syncDualPair(state.curFace, state.faceLayer, state.dualFaceLayer, true);
    syncDualPair(state.curMakeup, state.makeupLayer, state.dualMakeupLayer, false);
    syncDualPair(state.curEars, state.earsLayer, state.dualEarsLayer, false);
    syncDualPair(state.curBrows, state.browsLayer, state.dualBrowsLayer, false);
    syncDualPair(state.curNose, state.noseLayer, state.dualNoseLayer, false);
    syncDualPair(state.curBeard, state.beardLayer, state.dualBeardLayer, false);
    syncDualPair(state.curGlasses, state.glassesLayer, state.dualGlassesLayer, false);
    syncDualPair(state.curHair, state.hairLayer, state.dualHairLayer, false);
    syncDualPair(state.curHat, state.hatLayer, state.dualHatLayer, false);

    outRawDimmer = 0;
    if (state.isDmxActive && mode != 1) {
        int dimOff = (mode == 39) ? DMX_OFFSET_MASTER_DIMMER : 1;
        outRawDimmer = dmx_read_slot(0, getWrappedDmxIndex(cfg.start_channel, dimOff) + 1);
    }
    state.masterDimmer = state.manualBlackout ? 255 : outRawDimmer;

    setSystemAudioMonitorEnabled(!state.isDmxActive || (mode != 7 && mode != 39));
    if (state.isDmxActive && (mode == 7 || mode == 39)) {
        int audBase = (mode == 39) ? DMX_OFFSET_AUDIO_TRACK : 4;
        updateAudioControlFromDmx(
            dmx_read_slot(0, getWrappedDmxIndex(cfg.start_channel, audBase) + 1),
            dmx_read_slot(0, getWrappedDmxIndex(cfg.start_channel, audBase + 1) + 1),
            dmx_read_slot(0, getWrappedDmxIndex(cfg.start_channel, audBase + 2) + 1)
        );
    } else {
        resetAudioOnDmxTimeout();
    }
}

// ------------------------------------------------------------------------------
// 97. Step procedural eye/mouth animation and upload streaming ARGB textures
// ------------------------------------------------------------------------------
static void stepProceduralAnimation(
    const FaceAppConfig& cfg, AppRuntimeState& state, uint32_t frameStart,
    uint8_t rawMem, bool dmxEditOverlay, SDL_Texture* eyesTexture, SDL_Texture* featuresTexture)
{
    eyeTft.fillScreen(TFT_BLACK);
    tft.fillScreen(TFT_BLACK);
    if (state.packBrowser.open && state.packBrowser.inspectMode) {
        runAnimationById(state.manualTestAnim, frameStart);
    } else if (state.dmxMemoryLocked) {
        int slot = decodeDmxMemorySlot(rawMem);
        int targetAnim = 0;
        auto it = cfg.memory_combos.find(slot);
        if (it != cfg.memory_combos.end() && it->second.enabled) {
            targetAnim = it->second.anim_id;
        }
        if (dmxEditOverlay || slot < 0) {
            runDmxMemory(rawMem, frameStart, dmxEditOverlay);
        } else {
            runAnimationById(targetAnim, frameStart);
        }
    } else if (state.isDmxActive && cfg.dmx_mode == 39) {
        uint8_t browTilt = dmx_read_slot(0, getWrappedDmxIndex(cfg.start_channel, DMX_OFFSET_BROW_TILT) + 1);
        setProgrammaticBrows(browTilt, browTilt);
        updateEyeViaDmx();
        updateFaceViaDmx(isMouthHiddenByMemory() || state.manualHideMouth);
    } else if (state.manualTestAnim > 0) {
        runAnimationById(state.manualTestAnim, frameStart);
    } else if (state.manualMemorySlot >= 0) {
        int targetAnim = 0;
        auto it = cfg.memory_combos.find(state.manualMemorySlot);
        if (it != cfg.memory_combos.end() && it->second.enabled) {
            targetAnim = it->second.anim_id;
        }
        runAnimationById(targetAnim, frameStart);
    } else {
        runAutonomousMemoryShow(frameStart, getSavedSequentialMemorySlots(cfg), cfg.memory_combos);
        int autoSlot = getActiveMemorySlot();
        if (autoSlot != state.appliedMemorySlot && state.fadePhase == 0) {
            if (state.appliedMemorySlot < 0) {
                state.appliedMemorySlot = autoSlot;
                applyMemoryComboToState(cfg, state, autoSlot, false);
                applyMaskOffsetProfile(cfg, state, state.faceLayer.testIndex);
            } else {
                triggerMemoryTransition(state, cfg, -1, autoSlot, nullptr);
            }
        }
    }
    SDL_UpdateTexture(eyesTexture, nullptr, eyeTft.updateArgbBuffer(true), BASE_CANVAS_W * sizeof(uint32_t));
    SDL_UpdateTexture(featuresTexture, nullptr, tft.updateArgbBuffer(true), BASE_CANVAS_W * sizeof(uint32_t));
}
// ------------------------------------------------------------------------------
// 97.5 Update all 25 FX slots, 4-bit depth masks, Stage actors & gaze target
// ------------------------------------------------------------------------------
static uint32_t stepStageAndFxActors(AppRuntimeState& state, float stageDt, uint32_t frameStart)
{
    g_stage.setCameraVelocityFromDmx(g_parallax_vel_x, g_parallax_vel_y, 320.0f);
    if (g_fx_video.isOpen()) {
        g_fx_video.updateFrame(frameStart);
    }

    SDL_Texture* texFar   = state.fxBank.fog2 ? state.fxBank.fog2 : (state.fxBank.blurSprites.at(9) ? state.fxBank.blurSprites.at(9) : state.fxBank.sprites.at(9));
    SDL_Texture* texMid   = state.fxBank.blurSprites.at(9) ? state.fxBank.blurSprites.at(9) : state.fxBank.sprites.at(9);
    SDL_Texture* texFront = state.fxBank.sprites.at(9);

    if (!g_cloud_actor) {
        auto a = std::make_unique<SeamlessFogActor>(state.parallaxBg.clouds, &g_stage_noise, DepthLayer::SKY, 10.0f);
        a->perlinSwayX = 6.0f; a->perlinSwayY = 3.0f; a->tileScaleW = 1.0f; a->periodicTex = false;
        g_cloud_actor = static_cast<SeamlessFogActor*>(g_stage.addActor(std::move(a)));
    }
    if (!g_fog_back_actor) {
        auto a = std::make_unique<SeamlessFogActor>(texFar, &g_stage_noise, DepthLayer::FAR, 14.0f);
        a->perlinSwayX = 12.0f; a->perlinSwayY = 8.0f; a->periodicTex = true;
        g_fog_back_actor = static_cast<SeamlessFogActor*>(g_stage.addActor(std::move(a)));
    }
    if (!g_fog_mid_actor) {
        auto a = std::make_unique<SeamlessFogActor>(texMid, &g_stage_noise, DepthLayer::MID, -26.0f);
        a->perlinSwayX = 24.0f; a->perlinSwayY = 14.0f; a->periodicTex = true;
        g_fog_mid_actor = static_cast<SeamlessFogActor*>(g_stage.addActor(std::move(a)));
    }
    if (!g_fog_front_actor) {
        auto a = std::make_unique<SeamlessFogActor>(texFront, &g_stage_noise, DepthLayer::FRONT, 42.0f);
        a->perlinSwayX = 36.0f; a->perlinSwayY = 18.0f; a->periodicTex = true;
        g_fog_front_actor = static_cast<SeamlessFogActor*>(g_stage.addActor(std::move(a)));
    }
    if (!g_ufo_actor)       g_ufo_actor       = static_cast<UfoActor*>(g_stage.addActor(std::make_unique<UfoActor>(state.fxBank.sprites.at(7), g_default_particle_tex)));
    if (!g_nose_fly_actor)  g_nose_fly_actor  = static_cast<NoseFlyActor*>(g_stage.addActor(std::make_unique<NoseFlyActor>(state.fxBank.sprites.at(1), state.fxBank.animSprites.at(1))));
    if (!g_spider_actor)    g_spider_actor    = static_cast<SpiderActor*>(g_stage.addActor(std::make_unique<SpiderActor>(state.fxBank.sprites.at(5), state.fxBank.animSprites.at(5), 515.0f)));
    if (!g_snake_actor)     g_snake_actor     = static_cast<SnakeActor*>(g_stage.addActor(std::make_unique<SnakeActor>(state.fxBank.sprites.at(11), state.fxBank.animSprites.at(11))));
    if (!g_lightning_actor) g_lightning_actor = static_cast<LightningActor*>(g_stage.addActor(std::make_unique<LightningActor>(state.fxBank.sprites.at(14), state.fxBank.sprites.at(15))));
    if (!g_fire_actor)      g_fire_actor      = static_cast<FireActor*>(g_stage.addActor(std::make_unique<FireActor>(state.fxBank.sprites.at(12), state.fxBank.sprites.at(13))));
    if (!g_cobweb_actor)    g_cobweb_actor    = static_cast<StaticPropActor*>(g_stage.addActor(std::make_unique<StaticPropActor>(state.fxBank.sprites.at(8), DepthLayer::MID)));

    auto topLayer = [](uint8_t m, DepthLayer def) -> DepthLayer {
        if (m & 8) return DepthLayer::FRONT;
        if (m & 4) return DepthLayer::MID;
        if (m & 2) return DepthLayer::FAR;
        if (m & 1) return DepthLayer::SKY;
        return def;
    };

    uint8_t fogMask  = g_fx_depth_masks.at(9);
    bool hasBgClouds = (state.curBgScene > 0 && state.parallaxBg.clouds != nullptr);
    g_cloud_actor->texture         = (fogMask & 1) ? texFar : state.parallaxBg.clouds;
    g_cloud_actor->targetAlpha     = (fogMask & 1) ? 165 : (hasBgClouds ? 255 : 0);
    g_fog_back_actor->texture      = texFar;
    g_fog_back_actor->targetAlpha  = ((fogMask & 2) || (state.curFxMask & FX_BIT_FOG_BACK)) ? 185 : 0;
    g_fog_mid_actor->texture       = texMid;
    g_fog_mid_actor->targetAlpha   = ((fogMask & 4) || (state.curFxMask & FX_BIT_FOG_MID)) ? 205 : 0;
    g_fog_front_actor->texture     = texFront;
    g_fog_front_actor->targetAlpha = ((fogMask & 8) || (state.curFxMask & FX_BIT_FOG_FRONT)) ? 230 : 0;

    g_ufo_actor->texture           = state.fxBank.sprites.at(7);
    g_ufo_actor->beamGlow          = g_default_particle_tex;
    g_ufo_actor->depthMask         = (g_fx_depth_masks.at(7) != 0) ? g_fx_depth_masks.at(7) : ((state.curFxMask & (1u << 7)) ? 4 : 0);

    g_nose_fly_actor->texWingsSpread = state.fxBank.sprites.at(1);
    g_nose_fly_actor->texWingsFolded = state.fxBank.animSprites.at(1);
    g_nose_fly_actor->noseX          = 300.0f + static_cast<float>(state.noseLayer.offsetX);
    g_nose_fly_actor->noseY          = 444.0f + static_cast<float>(state.noseLayer.offsetY);
    g_nose_fly_actor->active         = (g_fx_depth_masks.at(1) != 0) || ((state.curFxMask & (1u << 1)) != 0);
    if (g_fx_depth_masks.at(1)) g_nose_fly_actor->layer = topLayer(g_fx_depth_masks.at(1), DepthLayer::FRONT);

    g_spider_actor->tex1    = state.fxBank.sprites.at(5);
    g_spider_actor->tex2    = state.fxBank.animSprites.at(5);
    g_spider_actor->active  = (g_fx_depth_masks.at(5) != 0) || ((state.curFxMask & (1u << 5)) != 0);
    if (g_fx_depth_masks.at(5)) g_spider_actor->layer = topLayer(g_fx_depth_masks.at(5), DepthLayer::FRONT);

    g_snake_actor->tex1     = state.fxBank.sprites.at(11);
    g_snake_actor->tex2     = state.fxBank.animSprites.at(11);
    g_snake_actor->active   = (g_fx_depth_masks.at(11) != 0) || ((state.curFxMask & (1u << 11)) != 0);
    if (g_fx_depth_masks.at(11)) g_snake_actor->layer = topLayer(g_fx_depth_masks.at(11), DepthLayer::FRONT);

    g_lightning_actor->tex1   = state.fxBank.sprites.at(14);
    g_lightning_actor->tex2   = state.fxBank.sprites.at(15);
    g_lightning_actor->active = (g_fx_depth_masks.at(13) != 0) || ((state.curFxMask & (1u << 13)) != 0);

    g_fire_actor->tex1      = state.fxBank.sprites.at(12);
    g_fire_actor->tex2      = state.fxBank.sprites.at(13);
    g_fire_actor->active    = (g_fx_depth_masks.at(12) != 0) || ((state.curFxMask & (1u << 12)) != 0);
    if (g_fx_depth_masks.at(12)) g_fire_actor->layer = topLayer(g_fx_depth_masks.at(12), DepthLayer::FRONT);

    g_cobweb_actor->texture = state.fxBank.sprites.at(8);
    g_cobweb_actor->active  = (g_fx_depth_masks.at(8) != 0) || ((state.curFxMask & (1u << 8)) != 0);
    if (g_fx_depth_masks.at(8)) g_cobweb_actor->layer = topLayer(g_fx_depth_masks.at(8), DepthLayer::MID);
    struct CritterSpec { int spr; int slot; uint32_t bit; int maxN; float sz; EmitterPhysicsMode mode; float turb; DepthLayer lyr; uint8_t alpha; uint32_t ms; };
    static constexpr std::array<CritterSpec, 7> kSwarms = {{
        {0,  0,  (1u << 0),  16, 0.68f, EmitterPhysicsMode::SWARM_FLOCK,  1.25f, DepthLayer::FRONT, 255, 85},
        {2,  2,  (1u << 2),  30, 0.16f, EmitterPhysicsMode::SWARM_FLOCK,  1.65f, DepthLayer::FRONT, 240, 45},
        {3,  3,  (1u << 3),  12, 0.45f, EmitterPhysicsMode::SWARM_FLOCK,  1.05f, DepthLayer::MID,   235, 140},
        {4,  4,  (1u << 4),  20, 0.26f, EmitterPhysicsMode::SWARM_FLOCK,  1.45f, DepthLayer::FRONT, 255, 50},
        {6,  6,  (1u << 6),  18, 0.36f, EmitterPhysicsMode::PERLIN_SWIRL, 0.90f, DepthLayer::FRONT, 255, 130},
        {10, 10, (1u << 10),  4, 0.55f, EmitterPhysicsMode::PERLIN_SWIRL, 0.65f, DepthLayer::FRONT, 175, 250},
        {16, 14, (1u << 14), 12, 0.48f, EmitterPhysicsMode::GROUND_CRAWL, 1.35f, DepthLayer::FRONT, 255, 95}
    }};

    for (const auto& s : kSwarms) {
        SDL_Texture* f1 = state.fxBank.sprites.at(s.spr);
        SDL_Texture* f2 = state.fxBank.animSprites.at(s.spr);
        if (s.spr == 0 && f1 && f2 && state.fxBank.bat3) g_critter_emitters.at(0).setAnimatedFrames({f1, f2, state.fxBank.bat3, f2}, 85);
        else if (f1 && f2)                               g_critter_emitters.at(s.spr).setAnimatedFrames({f1, f2}, s.ms);
        else                                             g_critter_emitters.at(s.spr).setSpriteSource(f1);

        uint8_t dm = g_fx_depth_masks.at(s.slot);
        bool on    = (dm != 0) || ((state.curFxMask & s.bit) != 0);
        int cnt    = on ? ((g_critter_swarm_count <= 1) ? 1 : std::min(s.maxN, g_critter_swarm_count)) : 0;
        EmitterDmxParams p{cnt, s.sz, s.mode, s.turb, s.lyr, -1.0f, -1.0f, 255, 255, 255, s.alpha, true, (s.spr != 16), true, dm};
        g_critter_emitters.at(s.spr).updateFromDmx(g_stage, p, stageDt);
    }

    if (state.fxBank.fireflyTex.at(0) && state.fxBank.fireflyTex.at(1)) g_firefly_emitter.setAnimatedFrames({state.fxBank.fireflyTex.at(0), state.fxBank.fireflyTex.at(1)}, 65);
    g_firefly_emitter.setGlowTexture(g_default_particle_tex);
    g_firefly_emitter.updateFromDmx(g_stage, {g_fx_depth_masks.at(15) ? std::max(8, g_critter_swarm_count * 2) : 0, 0.34f, EmitterPhysicsMode::FIREFLIES, 0.95f, DepthLayer::FRONT, -1.0f, -1.0f, 255, 255, 255, 255, true, false, true, g_fx_depth_masks.at(15)}, stageDt);

    if (state.fxBank.grasshopperTex.at(0) && state.fxBank.grasshopperTex.at(1)) g_grasshopper_emitter.setAnimatedFrames({state.fxBank.grasshopperTex.at(0), state.fxBank.grasshopperTex.at(1)}, 120);
    g_grasshopper_emitter.updateFromDmx(g_stage, {g_fx_depth_masks.at(16) ? std::max(4, g_critter_swarm_count) : 0, 0.36f, EmitterPhysicsMode::SWARM_FLOCK, 1.35f, DepthLayer::FRONT, -1.0f, -1.0f, 255, 255, 255, 255, true, false, true, g_fx_depth_masks.at(16)}, stageDt);

    if (state.fxBank.antTex.at(0) && state.fxBank.antTex.at(1)) g_ant_emitter.setAnimatedFrames({state.fxBank.antTex.at(0), state.fxBank.antTex.at(1)}, 80);
    g_ant_emitter.updateFromDmx(g_stage, {g_fx_depth_masks.at(17) ? std::max(6, g_critter_swarm_count) : 0, 0.28f, EmitterPhysicsMode::GROUND_CRAWL, 1.15f, DepthLayer::FRONT, -1.0f, -1.0f, 255, 255, 255, 255, true, false, true, g_fx_depth_masks.at(17)}, stageDt);

    g_leaves_emitter.setSpriteSource(state.fxBank.leafTex ? state.fxBank.leafTex : g_default_particle_tex);
    g_leaves_emitter.updateFromDmx(g_stage, {g_fx_depth_masks.at(18) ? 28 : 0, 0.52f, EmitterPhysicsMode::AUTUMN_LEAVES, 1.0f, DepthLayer::FRONT, -1.0f, -1.0f, 255, 255, 255, 240, false, false, false, g_fx_depth_masks.at(18)}, stageDt);

    g_fireworks_emitter.setSpriteSource(g_default_particle_tex);
    g_fireworks_emitter.updateFromDmx(g_stage, {g_fx_depth_masks.at(19) ? 44 : 0, 0.95f, EmitterPhysicsMode::FIREWORKS, 1.0f, DepthLayer::FAR, -1.0f, -1.0f, 255, 255, 255, 255, false, false, false, g_fx_depth_masks.at(19)}, stageDt);

    SDL_Texture* snowTex = state.fxBank.snowflakeTex ? state.fxBank.snowflakeTex : g_default_particle_tex;
    g_snow_mid_emitter.setSpriteSource(snowTex);
    g_snow_front_emitter.setSpriteSource(snowTex);
    uint8_t snM = g_fx_depth_masks.at(20);
    g_snow_mid_emitter.updateFromDmx(g_stage, {(snM & 3) ? 45 : 0, 0.34f, EmitterPhysicsMode::GRAVITY_FALL, 0.45f, DepthLayer::MID, -1.0f, -1.0f, 220, 238, 255, 175}, stageDt);
    g_snow_front_emitter.updateFromDmx(g_stage, {((snM & 12) || (snM == 0 && (state.curFxMask & FX_BIT_SNOW))) ? 40 : 0, 0.62f, EmitterPhysicsMode::GRAVITY_FALL, 0.65f, DepthLayer::FRONT, -1.0f, -1.0f, 255, 255, 255, 240}, stageDt);

    uint8_t rnM = g_fx_depth_masks.at(21);
    g_rain_mid_emitter.updateFromDmx(g_stage, {(rnM & 3) ? 55 : 0, 0.70f, EmitterPhysicsMode::GRAVITY_FALL, 1.8f, DepthLayer::MID, -1.0f, -1.0f, 170, 210, 255, 120}, stageDt);
    g_rain_front_emitter.updateFromDmx(g_stage, {((rnM & 12) || (rnM == 0 && (state.curFxMask & FX_BIT_RAIN))) ? 55 : 0, 1.15f, EmitterPhysicsMode::GRAVITY_FALL, 2.4f, DepthLayer::FRONT, -1.0f, -1.0f, 200, 230, 255, 200}, stageDt);

    if (g_stage_fx_enabled) {
        EmitterPhysicsMode fxMode = static_cast<EmitterPhysicsMode>(g_stage_fx_mode);
        g_fx_emitter.setSpriteSource((fxMode == EmitterPhysicsMode::AUTUMN_LEAVES && state.fxBank.leafTex) ? state.fxBank.leafTex : g_default_particle_tex);
        g_fx_emitter.updateFromDmx(g_stage, {g_stage_fx_count, (fxMode == EmitterPhysicsMode::AUTUMN_LEAVES) ? 0.55f : 1.0f, fxMode, 1.15f, DepthLayer::FRONT}, stageDt);
    }

    g_stage.stepAll(stageDt, frameStart);

    float gx = 300.0f, gy = 400.0f, gc = 0.0f;
    if (g_stage.findPrimaryGazeTarget(gx, gy, gc)) setGazeTarget(gx, gy, gc);
    else clearGazeTarget();

    uint32_t skyMask = state.curFxMask;
    if (g_fx_depth_masks.at(22)) skyMask |= FX_BIT_STARS;
    if (g_fx_depth_masks.at(23)) skyMask |= FX_BIT_COMET;
    if (g_fx_depth_masks.at(24)) skyMask |= FX_BIT_AURORA;
    return skyMask;
}
// ------------------------------------------------------------------------------
// 98. Initialize SDL2 renderer, DMX receiver thread and 50 FPS render loop
// ------------------------------------------------------------------------------
static void run_face_engine(FaceAppConfig& cfg)
{
    start_address = cfg.start_channel;
    SDL_SetHint(SDL_HINT_VIDEODRIVER, "wayland,x11");
    SDL_SetHint(SDL_HINT_RENDER_DRIVER, "opengles2");
    SDL_SetHint(SDL_HINT_VIDEO_WAYLAND_ALLOW_LIBDECOR, "0");

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) return;
    IMG_Init(IMG_INIT_PNG | IMG_INIT_JPG | IMG_INIT_WEBP);

    SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 0);

    int dispIdx = resolveTargetDisplayIndex(cfg);
    SDL_Window* window = SDL_CreateWindow("MakingFaces", SDL_WINDOWPOS_CENTERED_DISPLAY(dispIdx), SDL_WINDOWPOS_CENTERED_DISPLAY(dispIdx),
                                          BASE_CANVAS_W, BASE_CANVAS_H, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
    if (!window) { IMG_Quit(); SDL_Quit(); return; }
    if (cfg.fullscreen) SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN_DESKTOP);
    SDL_ShowCursor(cfg.fullscreen ? SDL_DISABLE : SDL_ENABLE);

    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, cfg.smooth_scale ? "linear" : "nearest");
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer) renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

    SDL_RendererInfo rInfo{};
    bool hasVsync = (SDL_GetRendererInfo(renderer, &rInfo) == 0) && ((rInfo.flags & SDL_RENDERER_PRESENTVSYNC) != 0);
    SDL_Texture* eyesTexture     = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, BASE_CANVAS_W, BASE_CANVAS_H);
    SDL_Texture* featuresTexture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, BASE_CANVAS_W, BASE_CANVAS_H);
    SDL_SetTextureBlendMode(eyesTexture, SDL_BLENDMODE_BLEND);
    SDL_SetTextureBlendMode(featuresTexture, SDL_BLENDMODE_BLEND);

    {
        SDL_Surface* pSurf = SDL_CreateRGBSurfaceWithFormat(0, 48, 48, 32, SDL_PIXELFORMAT_ARGB8888);
        if (pSurf) {
            uint32_t* px = static_cast<uint32_t*>(pSurf->pixels);
            for (int y = 0; y < 48; ++y)
                for (int x = 0; x < 48; ++x) {
                    float a = std::max(0.0f, 1.0f - std::hypot((x - 23.5f) / 23.5f, (y - 23.5f) / 23.5f));
                    px[y * 48 + x] = (static_cast<uint32_t>(std::round(a * a * 255.0f)) << 24) | 0x00FFFFFFu;
                }
            g_default_particle_tex = SDL_CreateTextureFromSurface(renderer, pSurf);
            SDL_FreeSurface(pSurf);
            if (g_default_particle_tex) { SDL_SetTextureBlendMode(g_default_particle_tex, SDL_BLENDMODE_ADD); g_fx_emitter.setSpriteSource(g_default_particle_tex); }
        }
        SDL_Surface* rSurf = SDL_CreateRGBSurfaceWithFormat(0, 6, 28, 32, SDL_PIXELFORMAT_ARGB8888);
        if (rSurf) {
            uint32_t* px = static_cast<uint32_t*>(rSurf->pixels);
            for (int y = 0; y < 28; ++y)
                for (int x = 0; x < 6; ++x) {
                    uint8_t a = static_cast<uint8_t>(std::round((y / 27.0f) * (1.0f - std::fabs((x - 2.5f) / 2.5f)) * 210.0f));
                    px[y * 6 + x] = (static_cast<uint32_t>(a) << 24) | 0x00B8DCFFu;
                }
            g_rain_drop_tex = SDL_CreateTextureFromSurface(renderer, rSurf);
            SDL_FreeSurface(rSurf);
            if (g_rain_drop_tex) { SDL_SetTextureBlendMode(g_rain_drop_tex, SDL_BLENDMODE_BLEND); g_rain_mid_emitter.setSpriteSource(g_rain_drop_tex); g_rain_front_emitter.setSpriteSource(g_rain_drop_tex); }
        }
    }

    initEyes();
    reloadDynamicEyeBank(cfg.assets_dir);
    initFaceEngine();
    initAnimationEngine();
    initAudioEngine(cfg.audio_dir, cfg.audio_monitor);

    AppRuntimeState state{};
    state.isFullscreen = cfg.fullscreen;
    state.osdEnabled   = osd.enabled;
    initFxSpriteBank(renderer, cfg.assets_dir, state.fxBank);
    applyMemoryComboToState(cfg, state, 0, false);
    applyMaskOffsetProfile(cfg, state, state.faceLayer.testIndex);

    int dmxSock = init_dmx_socket(cfg.proto, cfg.universe);
    std::array<uint8_t, 512> netBuf{};
    std::mutex netMutex;
    std::atomic<uint32_t> lastPktMs{0}, pktCounter{0};

    std::thread netThread([&]() {
        std::array<uint8_t, 1500> raw{};
        std::array<uint8_t, 512>  parsed{};
        while (g_running && dmxSock >= 0) {
            ssize_t len = recvfrom(dmxSock, raw.data(), raw.size(), 0, nullptr, nullptr);
            if (len > 0 && parseDmxPacket(cfg.proto, cfg.universe, raw, len, parsed)) {
                { std::lock_guard<std::mutex> lk(netMutex); netBuf = parsed; }
                lastPktMs.store(millis());
                pktCounter.fetch_add(1);
            }
        }
    });
    uint32_t frameDelayMs = 1000 / std::max(1, cfg.fps), lastRateTick = millis(), lastPktCount = 0, prevStageMs = millis();
    while (g_running) {
        uint32_t frameStart = millis();
        float stageDt = std::min(0.05f, (frameStart - prevStageMs) * 0.001f);
        prevStageMs = frameStart;

        SDL_Event ev;
        while (SDL_PollEvent(&ev)) {
            if (ev.type == SDL_QUIT) g_running = false;
            else if (ev.type == SDL_KEYDOWN) handleKeyboardEvent(window, ev.key.keysym.sym, static_cast<SDL_Keymod>(ev.key.keysym.mod), cfg, state);
            else if (ev.type == SDL_MOUSEBUTTONDOWN && ev.button.button == SDL_BUTTON_LEFT && ev.button.clicks == 2 && !state.flatImport.open) toggleWindowFullscreen(window, state.isFullscreen);
            handleFlatImportMouse(window, renderer, ev, state.flatImport);
        }

        if (frameStart - lastRateTick >= 1000) {
            uint32_t cur = pktCounter.load();
            state.dmxPktRate = cur - lastPktCount; lastPktCount = cur; lastRateTick = frameStart;
        }
        updateDmxAndFadeState(cfg, state, frameStart, lastPktMs.load());
        if (state.isDmxActive) { std::lock_guard<std::mutex> lk(netMutex); g_dmx_universe = netBuf; }

        uint8_t rawMem = 0, rawDimmer = 0;
        bool dmxEditOverlay = false;
        resolveActiveLayersAndAudio(window, renderer, cfg, state, frameStart, rawMem, dmxEditOverlay, rawDimmer);
        if (!state.cameraCapture.open && !state.flatImport.open) stepProceduralAnimation(cfg, state, frameStart, rawMem, dmxEditOverlay, eyesTexture, featuresTexture);

        uint32_t effFxMask = stepStageAndFxActors(state, stageDt, frameStart);
        bool hideAll = state.cameraCapture.open || state.flatImport.open || isFaceHiddenByMemory() || (state.isDmxActive && rawDimmer == 255) || state.manualBlackout;
        renderCompositedFrame(renderer, cfg, state, eyesTexture, featuresTexture, state.curFace, state.curBeard, state.curHair, effFxMask, hideAll, isMouthHiddenByMemory() || state.manualHideMouth, isBrowsHiddenByMemory() || state.manualHideBrows, state.fadeAlpha, frameStart);

        if (!hasVsync) {
            uint32_t elapsed = millis() - frameStart;
            if (elapsed < frameDelayMs) SDL_Delay(frameDelayMs - elapsed);
        }
    }

    if (dmxSock >= 0) close(dmxSock);
    if (netThread.joinable()) netThread.join();
    g_stage.clearActors();
    g_fx_video.close();
    if (g_default_particle_tex) { SDL_DestroyTexture(g_default_particle_tex); g_default_particle_tex = nullptr; }
    if (g_rain_drop_tex)        { SDL_DestroyTexture(g_rain_drop_tex);        g_rain_drop_tex = nullptr; }
    shutdownAudioEngine();
    closeCameraViewfinder(state.cameraCapture);
    destroyFlatImportItems(state.flatImport);
    destroyEyeBrowserEntries(state.eyeBrowser);
    destroyPackBrowserEntries(state.packBrowser);
    destroyFxSpriteBank(state.fxBank);
    destroyParallaxBackdrop(state.parallaxBg);
    destroyPaperDollLayers(state);
    SDL_DestroyTexture(featuresTexture);
    SDL_DestroyTexture(eyesTexture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    IMG_Quit();
    SDL_Quit();
}

// ------------------------------------------------------------------------------
// 99. Unpack packs/default.zip if missing, create import/ & launch engine
// ------------------------------------------------------------------------------
int main(int argc, char* argv[])
{
    SDL_SetHint(SDL_HINT_NO_SIGNAL_HANDLERS, "1");
    std::signal(SIGINT, sig_handler);
    std::signal(SIGTERM, sig_handler);

    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        if (a == "--version" || a == "-v") {
            std::printf("MakingFaces %s (Architecture by Magnus Herrlin 2026 - Bodmer TFT_eSPI & Adafruit Uncanny_Eyes inspired)\n", APP_VERSION_STR);
            return 0;
        }
    }

    fs::create_directories("import");
    fs::create_directories("import/done");

    bool needUnpack = (!fs::exists("assets") || !fs::is_directory("assets") ||
                       !fs::exists("makingfaces.ini") || fs::file_size("makingfaces.ini") == 0);
    if (needUnpack && fs::exists("packs/default.zip") && fs::file_size("packs/default.zip") > 0) {
        if (fs::exists("assets")) {
            fs::remove_all("assets");
        }
        if (fs::exists("makingfaces.ini")) {
            fs::remove("makingfaces.ini");
        }
        std::system("unzip -oq \"packs/default.zip\" -d .");
    }

    FaceAppConfig cfg;
    for (int i = 1; i < argc; ++i) {
        if (std::string(argv[i]) == "--config" && (i + 1) < argc) {
            cfg.config_file = argv[i + 1];
        }
    }
    loadIniConfig(cfg.config_file, cfg);

    static struct option longOptions[] = {
        {"universe", required_argument, 0, 'u'},
        {"channel", required_argument, 0, 'c'},
        {"mode", required_argument, 0, 'm'},
        {"display", required_argument, 0, 'd'},
        {"artnet", no_argument, 0, 'a'},
        {"fps", required_argument, 0, 'f'},
        {"fullscreen", no_argument, 0, 'F'},
        {"smooth", no_argument, 0, 's'},
        {"version", no_argument, 0, 'v'},
        {"audio-dir", required_argument, 0, 1001},
        {"assets-dir", required_argument, 0, 1002},
        {"audio-monitor", required_argument, 0, 1003},
        {"config", required_argument, 0, 1004},
        {"help", no_argument, 0, 'h'},
        {0, 0, 0, 0}
    };

    int opt;
    while ((opt = getopt_long(argc, argv, "u:c:m:d:af:Fsvh", longOptions, nullptr)) != -1) {
        switch (opt) {
            case 'u': cfg.universe = std::stoi(optarg); break;
            case 'c': cfg.start_channel = std::stoi(optarg); break;
            case 'm': {
                int m = std::stoi(optarg);
                if (m == 1 || m == 2 || m == 7 || m == 39) {
                    cfg.dmx_mode = m;
                }
                break;
            }
            case 'd': cfg.display = std::stoi(optarg); break;
            case 'a': cfg.proto = "artnet"; break;
            case 'f': cfg.fps = std::stoi(optarg); break;
            case 'F': cfg.fullscreen = true; break;
            case 's': cfg.smooth_scale = true; break;
            case 1001: cfg.audio_dir = optarg; break;
            case 1002: cfg.assets_dir = optarg; break;
            case 1003: cfg.audio_monitor = optarg; break;
            case 1004: cfg.config_file = optarg; break;
            default: return 0;
        }
    }
    run_face_engine(cfg);
    return 0;
}
