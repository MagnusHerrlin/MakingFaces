// ==============================================================================
// MakingFaces - Wayland/SDL2 Animatronic Face Compositor & Art-Net/sACN Engine
// Architecture designed by Magnus Herrlin 2026
// ==============================================================================

// ==============================================================================
// Section A. Includes, Constants & Shared Global Instances
// ==============================================================================
#include "main.h"
#include "eye_functions.h"
#include "face_engine.h"
#include "macro_engine.h"
#include "audio_engine.h"

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
std::atomic<int>         g_override_eye_style{-1};
std::atomic<int>         g_override_iris{-1};
std::atomic<float>       g_eye_scale_factor{1.0f};
std::atomic<float>       g_mouth_scale_factor{1.0f};
std::atomic<bool>        g_critter_gaze_active{false};
std::atomic<uint8_t>     g_critter_gaze_x{128};
std::atomic<uint8_t>     g_critter_gaze_y{128};
std::atomic<uint8_t>     g_brow_left_tilt{128};
std::atomic<uint8_t>     g_brow_right_tilt{128};
std::atomic<uint8_t>     g_effective_mouth_gap{0};
std::array<uint8_t, 512> g_dmx_universe{};

TFT_eSPI tft;
TFT_eSPI eyeTft;

uint16_t pbuffer[BUFFER_SIZE];
std::array<eye_t, 2> eye;
int      start_address = 1;

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
    if (!osd.enabled && !persist) return;
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
    if (c >= 'a' && c <= 'z') c = static_cast<char>(c - 'a' + 'A');
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
    if (c >= 'A' && c <= 'Z')      glyph = font.at(c - 'A').data();
    else if (c >= '0' && c <= '9') glyph = digits.at(c - '0').data();
    else {
        if (c == ':')      { custom.at(1)=0x36; custom.at(3)=0x36; }
        else if (c == '.') { custom.at(2)=0x60; custom.at(3)=0x60; }
        else if (c == '-') { custom.at(1)=0x08; custom.at(2)=0x08; custom.at(3)=0x08; }
        else if (c == '+') { custom.at(1)=0x08; custom.at(2)=0x1C; custom.at(3)=0x08; }
        else if (c == '/') { custom.at(0)=0x20; custom.at(1)=0x10; custom.at(2)=0x08; custom.at(3)=0x04; custom.at(4)=0x02; }
        else if (c == '[') { custom.at(1)=0x7F; custom.at(2)=0x41; }
        else if (c == ']') { custom.at(2)=0x41; custom.at(3)=0x7F; }
        else if (c == '(') { custom.at(2)=0x3E; custom.at(3)=0x41; }
        else if (c == ')') { custom.at(1)=0x41; custom.at(2)=0x3E; }
        else if (c == '!') { custom.at(2)=0x5F; }
        else if (c == '=') { custom.at(1)=0x14; custom.at(2)=0x14; custom.at(3)=0x14; }
        else if (c == '#') { custom.at(1)=0x7F; custom.at(2)=0x14; custom.at(3)=0x7F; }
        else if (c == '_') { custom.at(0)=0x40; custom.at(1)=0x40; custom.at(2)=0x40; custom.at(3)=0x40; custom.at(4)=0x40; }
        else if (c == '%') { custom.at(0)=0x62; custom.at(1)=0x64; custom.at(2)=0x08; custom.at(3)=0x13; custom.at(4)=0x23; }
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
    for (int i = 0; i < len; ++i) drawChar5x7(ren, str[i], x + i * (6 * scale), y, scale);
}

// ------------------------------------------------------------------------------
// 5. Retrieve the first non-loopback IPv4 address for telemetry display
// ------------------------------------------------------------------------------
static std::string getLocalIpAddress(void)
{
    struct ifaddrs* ifaddr = nullptr;
    if (getifaddrs(&ifaddr) != 0) return "0.0.0.0";
    std::string res = "127.0.0.1";
    for (auto* ifa = ifaddr; ifa; ifa = ifa->ifa_next) {
        if (!ifa->ifa_addr || ifa->ifa_addr->sa_family != AF_INET) continue;
        char ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &((sockaddr_in*)ifa->ifa_addr)->sin_addr, ip, sizeof(ip));
        if (std::strcmp(ip, "127.0.0.1") != 0) { res = ip; break; }
    }
    freeifaddrs(ifaddr);
    return res;
}

// ------------------------------------------------------------------------------
// 6. Render top OSD banner when enabled and no modal studio/overlay is open
// ------------------------------------------------------------------------------
static void renderOsdOverlay(SDL_Renderer* ren, uint32_t nowMs, const AppRuntimeState& state)
{
    if (osd.text.at(0) == '\0') return;
    if (!osd.persistent && osd.hideAtMs > 0 && nowMs >= osd.hideAtMs) { osd.text.at(0) = '\0'; return; }
    if (state.flatImport.open || state.cameraCapture.open || state.packBrowser.open || state.eyeBrowser.open || state.showHelpOverlay || g_show_about_overlay) return;
    if (!state.osdEnabled && !state.isEditingOffsets) return;

    int len = static_cast<int>(std::strlen(osd.text.data()));
    int scale = (len > 30) ? 2 : 3;
    int boxW = len * (6 * scale) + 28, boxH = 7 * scale + 22;
    SDL_Rect bg{(BASE_CANVAS_W - boxW) / 2, 16, boxW, boxH};
    SDL_SetRenderDrawColor(ren, 12, 16, 24, 215); SDL_RenderFillRect(ren, &bg);
    SDL_SetRenderDrawColor(ren, 0, 229, 255, 255); SDL_RenderDrawRect(ren, &bg);
    SDL_SetRenderDrawColor(ren, 255, 255, 255, 255);
    drawTextLine(ren, osd.text.data(), bg.x + 14, bg.y + 11, scale);
}
// ------------------------------------------------------------------------------
// 7. Render F1 help and F9 about overlays
// ------------------------------------------------------------------------------
static void renderHelpAndAbout(SDL_Renderer* ren, const FaceAppConfig& cfg, const AppRuntimeState& state)
{
    if (state.showHelpOverlay) {
        SDL_Rect box{20, 36, BASE_CANVAS_W - 40, 728};
        SDL_SetRenderDrawColor(ren, 10, 14, 22, 236); SDL_RenderFillRect(ren, &box);
        SDL_SetRenderDrawColor(ren, 0, 229, 255, 255); SDL_RenderDrawRect(ren, &box);
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
            "Y:DUAL PROFILE       V/SHIFT+V: FADE TIME",
            "-------------------------------------------",
            "PGUP/PGDN: MACRO BANK [00..24]",
            "1..9,0: LOCK MACRO IN ACTIVE BANK",
            "CTRL+1..9,0: COPY TO SLOT (LOCKED)",
            "M / SHIFT+M: STEP MACRO SLOT",
            "-------------------------------------------",
            "B/SH+B: STEP 20 FX [CRITTERS/SKY/WEATHER]",
            "CTRL+SHIFT+S/C/A/R/W: MIX STARS/COMET/",
            "  AURORA/RAIN/SNOW   CTRL+B: CLEAR ALL FX",
            "CTRL+U/N/W: HIDE MOUTH/NOSE/BROWS",
            "TAB+ARROWS: MOVE/SCALE EYE & MOUTH",
            "CTRL+X: EXPORT ZIP  CTRL+L/R: SPLIT OFFSET"
        };
        SDL_SetRenderDrawColor(ren, 240, 248, 255, 255);
        for (size_t i = 0; i < lines.size(); ++i)
            drawTextLine(ren, lines[i], 34, 48 + static_cast<int>(i) * 25, 2);
    }
    if (g_show_about_overlay) {
        SDL_Rect box{40, 175, BASE_CANVAS_W - 80, 450};
        SDL_SetRenderDrawColor(ren, 10, 14, 24, 244); SDL_RenderFillRect(ren, &box);
        SDL_SetRenderDrawColor(ren, 0, 229, 255, 255); SDL_RenderDrawRect(ren, &box);
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
        drawTextLine(ren, "- SDL2 WAYLAND + 38CH ART-NET / SACN", 72, 430, 2);
        std::array<char, 64> bankInfo{};
        std::snprintf(bankInfo.data(), bankInfo.size(), "EYE BANK: %d EYES   SAVED MACROS: %d", getEyeBankCount(), static_cast<int>(cfg.macro_combos.size()));
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
    if (g_usb_cam.isOpened()) g_usb_cam.release();
    if (cam.liveTex) { SDL_DestroyTexture(cam.liveTex); cam.liveTex = nullptr; }
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
    if (!cam.usbAvailable) cam.useCsiMode = true;
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
        if (!fs::exists(p)) return p.string();
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
        for (int flush = 0; flush < 3; ++flush) g_usb_cam.read(frame);
        if (!frame.empty()) {
            if (cam.rotate90) cv::rotate(frame, frame, cv::ROTATE_90_CLOCKWISE);
            if (!cam.eyeScopeMode) cv::flip(frame, frame, 1);
            if (!cam.eyeScopeMode && frame.cols > frame.rows) {
                int cropW = std::max(1, (frame.rows * 3) / 4);
                int x0 = std::max(0, (frame.cols - cropW) / 2);
                frame = frame(cv::Rect(x0, 0, cropW, frame.rows)).clone();
            }
            if (cv::imwrite(outPath, frame)) return outPath;
        }
        return "";
    }
    std::string cmd = "rpicam-still -n -t 300 --width 1200 --height 1600 -o \"" + outPath + "\" 2>/dev/null || "
                      "libcamera-still -n -t 300 --width 1200 --height 1600 -o \"" + outPath + "\" 2>/dev/null";
    if (std::system(cmd.c_str()) == 0 && fs::exists(outPath) && fs::file_size(outPath) > 0) return outPath;
    return "";
}

// ------------------------------------------------------------------------------
// 12. Draw ellipse outline for microscope iris guides & face/eye/mouth ovals
// ------------------------------------------------------------------------------
static void drawEllipseOutline(SDL_Renderer* ren, int cx, int cy, int rx, int ry)
{
    int prevX = cx + rx, prevY = cy;
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
    if (!cam.open) return;
    if (!cam.useCsiMode && g_usb_cam.isOpened()) {
        cv::Mat bgr, argb;
        if (g_usb_cam.read(bgr) && !bgr.empty()) {
            if (cam.rotate90) cv::rotate(bgr, bgr, cv::ROTATE_90_CLOCKWISE);
            if (!cam.eyeScopeMode) cv::flip(bgr, bgr, 1);
            if (!cam.eyeScopeMode && bgr.cols > bgr.rows) {
                int cropW = std::max(1, (bgr.rows * 3) / 4);
                int x0 = std::max(0, (bgr.cols - cropW) / 2);
                bgr = bgr(cv::Rect(x0, 0, cropW, bgr.rows)).clone();
            }
            cv::cvtColor(bgr, argb, cv::COLOR_BGR2BGRA);
            if (!cam.liveTex || cam.texW != argb.cols || cam.texH != argb.rows) {
                if (cam.liveTex) SDL_DestroyTexture(cam.liveTex);
                cam.liveTex = SDL_CreateTexture(ren, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, argb.cols, argb.rows);
                cam.texW = argb.cols;
                cam.texH = argb.rows;
            }
            if (cam.liveTex) SDL_UpdateTexture(cam.liveTex, nullptr, argb.data, static_cast<int>(argb.step));
        }
    }

    SDL_Rect box{30, 56, BASE_CANVAS_W - 60, 688};
    SDL_SetRenderDrawColor(ren, 10, 14, 24, 248); SDL_RenderFillRect(ren, &box);
    SDL_SetRenderDrawColor(ren, 0, 229, 255, 255); SDL_RenderDrawRect(ren, &box);
    SDL_SetRenderDrawColor(ren, 120, 245, 255, 255);
    drawTextLine(ren, cam.useCsiMode ? "CAMERA / MICROSCOPE [PI CSI RPICAM]" : "CAMERA / MICROSCOPE [USB 3:4 PORTRAIT]", 48, 74, 2);

    SDL_Rect view{120, 108, 360, 480};
    SDL_SetRenderDrawColor(ren, 18, 24, 36, 255); SDL_RenderFillRect(ren, &view);
    if (!cam.useCsiMode && cam.liveTex) SDL_RenderCopy(ren, cam.liveTex, nullptr, &view);
    else {
        SDL_SetRenderDrawColor(ren, 220, 235, 250, 255);
        drawTextLine(ren, "PI CSI CAMERA READY", 180, 325, 2);
        drawTextLine(ren, "PRESS SPACE OR E TO SNAP", 156, 360, 2);
    }
    SDL_SetRenderDrawColor(ren, 0, 229, 255, 220); SDL_RenderDrawRect(ren, &view);
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
// 14. Render Camera, Pack Browser, Live Inspector, Import Studio & Eye Browser
// ------------------------------------------------------------------------------
static int s_bgTolerance = 48;

static void renderBrowsersAndTelemetry(SDL_Renderer* ren, const FaceAppConfig& cfg, const AppRuntimeState& state)
{
    renderHelpAndAbout(ren, cfg, state);
    if (state.cameraCapture.open) {
        renderCameraViewfinderOverlay(ren, const_cast<CameraCaptureState&>(state.cameraCapture));
        return;
    }
    if (state.showTelemetry) {
        static std::string cachedIp = getLocalIpAddress();
        SDL_Rect box{24, BASE_CANVAS_H - 132, BASE_CANVAS_W - 48, 112};
        SDL_SetRenderDrawColor(ren, 10, 14, 22, 225); SDL_RenderFillRect(ren, &box);
        SDL_SetRenderDrawColor(ren, 0, 229, 255, 255); SDL_RenderDrawRect(ren, &box);
        std::array<char, 84> l1{}, l2{}, l3{};
        std::snprintf(l1.data(), l1.size(), "IP: %s  PROTO: %s  UNI: %u  CH: %u", cachedIp.c_str(), cfg.proto.c_str(), cfg.universe, cfg.start_channel);
        std::snprintf(l2.data(), l2.size(), "DMX: %s [%u PKT/S]  BANK: %02d  FADE: %.1FS", state.isDmxActive ? "ONLINE" : "AUTO/IDLE", state.dmxPktRate, state.macroBank, cfg.fade_ms / 1000.0f);
        std::snprintf(l3.data(), l3.size(), "MACRO: #%03d  ANIM: %s  DUAL: %s", getActiveMacroSlot(), getMacroAnimationName(state.manualTestAnim), state.dualProfileMode ? "ON" : "OFF");
        SDL_SetRenderDrawColor(ren, 120, 245, 255, 255);
        drawTextLine(ren, l1.data(), 38, box.y + 16, 2);
        drawTextLine(ren, l2.data(), 38, box.y + 46, 2);
        drawTextLine(ren, l3.data(), 38, box.y + 76, 2);
    }
    if (state.packBrowser.open && state.packBrowser.inspectMode) {
        SDL_Rect panel{18, 536, BASE_CANVAS_W - 36, 246};
        SDL_SetRenderDrawColor(ren, 10, 14, 24, 225); SDL_RenderFillRect(ren, &panel);
        SDL_SetRenderDrawColor(ren, 0, 229, 255, 255); SDL_RenderDrawRect(ren, &panel);
        const std::string& pName = state.packBrowser.entries.at(state.packBrowser.selectedIdx).name;
        std::array<char, 84> hdr{};
        std::snprintf(hdr.data(), hdr.size(), "PACK INSPECTOR: %s.ZIP [LIVE PREVIEW]", pName.c_str());
        SDL_SetRenderDrawColor(ren, 120, 245, 255, 255);
        drawTextLine(ren, hdr.data(), 32, 548, 2);
        int total = static_cast<int>(state.packBrowser.inspectItems.size());
        int start = std::max(0, state.packBrowser.inspectIdx - 2);
        for (int row = 0; row < 5 && (start + row) < total; ++row) {
            int idx = start + row;
            const auto& it = state.packBrowser.inspectItems.at(idx);
            bool cur = (idx == state.packBrowser.inspectIdx);
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
    if (state.flatImport.open) {
        static const std::array<const char*, 12> kCatNames = {
            "FACE", "EARS", "NOSE", "BROWS", "BEARD", "GLASSES", "HAIR", "HAT", "MAKEUP", "BG", "EYE_IRIS", "EYE_SCLERA"
        };
        SDL_Rect panel{6, 746, BASE_CANVAS_W - 12, 48};
        SDL_SetRenderDrawColor(ren, 10, 14, 24, 210); SDL_RenderFillRect(ren, &panel);
        SDL_SetRenderDrawColor(ren, 0, 229, 255, 200); SDL_RenderDrawRect(ren, &panel);
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
            SDL_SetRenderDrawColor(ren, 255, 255, 255, 255); drawTextLine(ren, l1.data(), 14, 755, 1);
            SDL_SetRenderDrawColor(ren, 255, 230, 120, 255); drawTextLine(ren, l2.data(), 14, 775, 1);
        }
        return;
    }
    if (state.packBrowser.open) {
        SDL_Rect box{36, 95, BASE_CANVAS_W - 72, 610};
        SDL_SetRenderDrawColor(ren, 10, 14, 24, 242); SDL_RenderFillRect(ren, &box);
        SDL_SetRenderDrawColor(ren, 0, 229, 255, 255); SDL_RenderDrawRect(ren, &box);
        SDL_SetRenderDrawColor(ren, 255, 255, 255, 255);
        drawTextLine(ren, "THEME PACK BROWSER [F6/ESC=CLOSE]", 56, 114, 2);
        if (state.packBrowser.entries.empty()) drawTextLine(ren, "NO .ZIP PACKS FOUND IN PACKS/", 92, 340, 2);
        else {
            const auto& cur = state.packBrowser.entries.at(state.packBrowser.selectedIdx);
            std::array<char, 80> hdr{}, stats{};
            std::snprintf(hdr.data(), hdr.size(), "PACK [%02d/%02d]: %s.ZIP", state.packBrowser.selectedIdx + 1, static_cast<int>(state.packBrowser.entries.size()), cur.name.c_str());
            SDL_SetRenderDrawColor(ren, 120, 245, 255, 255); drawTextLine(ren, hdr.data(), 56, 148, 2);
            SDL_Rect thumbFrame{165, 188, 270, 360};
            SDL_SetRenderDrawColor(ren, 20, 26, 40, 255); SDL_RenderFillRect(ren, &thumbFrame);
            if (cur.thumbTex) SDL_RenderCopy(ren, cur.thumbTex, nullptr, &thumbFrame);
            SDL_SetRenderDrawColor(ren, 0, 229, 255, 255); SDL_RenderDrawRect(ren, &thumbFrame);
            std::snprintf(stats.data(), stats.size(), "FACES: %d   MACROS: %d", cur.faceCount, cur.macroCount);
            SDL_SetRenderDrawColor(ren, 220, 235, 250, 255); drawTextLine(ren, stats.data(), 165, 562, 2);
        }
        SDL_SetRenderDrawColor(ren, 255, 230, 120, 255);
        drawTextLine(ren, "L/R:BROWSE  I/DN:INSPECT  ENTER:APPEND ALL", 56, 602, 2);
        drawTextLine(ren, "R:REPLACE DEFAULT  M:IMPORT/  F7:UNDO", 56, 630, 2);
        drawTextLine(ren, state.packBrowser.wipeConfirm ? "PRESS DEL AGAIN TO CONFIRM WIPE ALL!" : "DEL:WIPE ALL ASSETS  CTRL+X:EXPORT ZIP", 56, 658, 2);
    }
    if (state.eyeBrowser.open) {
        SDL_Rect box{48, 130, BASE_CANVAS_W - 96, 540};
        SDL_SetRenderDrawColor(ren, 10, 14, 24, 244); SDL_RenderFillRect(ren, &box);
        SDL_SetRenderDrawColor(ren, 0, 229, 255, 255); SDL_RenderDrawRect(ren, &box);
        SDL_SetRenderDrawColor(ren, 255, 255, 255, 255);
        drawTextLine(ren, "ADAFRUIT EYE BROWSER [F8/ESC=CLOSE]", 72, 150, 2);
        if (!state.eyeBrowser.entries.empty()) {
            const auto& cur = state.eyeBrowser.entries.at(state.eyeBrowser.selectedIdx);
            std::array<char, 80> hdr{};
            std::snprintf(hdr.data(), hdr.size(), "EYE [%02d/%02d]: %s", state.eyeBrowser.selectedIdx + 1, static_cast<int>(state.eyeBrowser.entries.size()), cur.name.c_str());
            SDL_SetRenderDrawColor(ren, 120, 245, 255, 255); drawTextLine(ren, hdr.data(), 72, 186, 2);
            SDL_Rect eyeBox{185, 236, 230, 230};
            SDL_SetRenderDrawColor(ren, 18, 24, 36, 255); SDL_RenderFillRect(ren, &eyeBox);
            if (cur.thumbTex) { SDL_Rect dst{195, 246, 210, 210}; SDL_RenderCopy(ren, cur.thumbTex, nullptr, &dst); }
            SDL_SetRenderDrawColor(ren, 0, 229, 255, 255); SDL_RenderDrawRect(ren, &eyeBox);
        }
        SDL_SetRenderDrawColor(ren, 255, 230, 120, 255);
        drawTextLine(ren, "LEFT/RIGHT: BROWSE   ENTER: SELECT EYE", 72, 502, 2);
        drawTextLine(ren, "0 / BACKSPACE: AUTO EYE   S/F4: SAVE MACRO", 72, 534, 2);
        drawTextLine(ren, "DROP *-SCLERA + *-IRIS IN GRAPHICS/ OR EYES/", 72, 566, 2);
    }
}

// ==============================================================================
// Section D. Macro Presets, Offset/Scale Profiles & INI Persistence
// ==============================================================================

// ------------------------------------------------------------------------------
// 15. Resolve bank index and digit key (0..9) into clamped macro slot 0..240
// ------------------------------------------------------------------------------
static int resolveBankKeySlot(int bank, int keyDigit)
{
    int slot = (keyDigit == 0) ? (bank * 10) : (bank * 10 + keyDigit);
    return std::max(0, std::min(MAX_MACRO_SLOT, slot));
}

// ------------------------------------------------------------------------------
// 16. Capture active layers, overrides, eye bank, animation and dual mode
// ------------------------------------------------------------------------------
static void captureMacroCombo(FaceAppConfig& cfg, const AppRuntimeState& state, int slot)
{
    int activeAnim = (state.manualTestAnim > 0) ? state.manualTestAnim : ((slot == 0) ? 1 : ((slot - 1) % 64) + 1);
    int rawTeeth   = state.dmxMacroLocked ? state.curTeeth    : g_override_teeth.load();
    int rawLip     = state.dmxMacroLocked ? state.curLipColor : g_override_lip_color.load();
    int rawLash    = state.dmxMacroLocked ? state.curEyelash  : g_override_eyelash.load();
    int rawStyle   = state.dmxMacroLocked ? state.curEyeStyle : g_override_eye_style.load();
    int rawIris    = g_override_iris.load();
    int rawBank    = getSelectedEyeBankIndex();
    cfg.macro_combos[slot] = {true, state.faceLayer.testIndex, state.makeupLayer.testIndex, state.earsLayer.testIndex,
        state.noseLayer.testIndex, state.beardLayer.testIndex, state.glassesLayer.testIndex,
        state.hairLayer.testIndex, state.hatLayer.testIndex, state.browsLayer.testIndex,
        state.bgLayer.testIndex, state.manualFxMask, (rawTeeth >= 0) ? rawTeeth : 1,
        (rawLip >= 0) ? rawLip : 3, (rawLash >= 0) ? rawLash : 0, rawStyle, rawIris, rawBank, activeAnim,
        state.dualProfileMode, state.manualHideMouth};
}

// ------------------------------------------------------------------------------
// 17. Apply a single MacroComboPreset directly to active runtime layers & overrides
// ------------------------------------------------------------------------------
static void applyPresetValuesToState(const MacroComboPreset& mc, AppRuntimeState& state, int fallbackAnim, bool updateAnim)
{
    state.faceLayer.testIndex = mc.face; state.makeupLayer.testIndex = mc.makeup; state.earsLayer.testIndex = mc.ears;
    state.noseLayer.testIndex = mc.nose; state.beardLayer.testIndex = mc.beard; state.glassesLayer.testIndex = mc.glasses;
    state.hairLayer.testIndex = mc.hair; state.hatLayer.testIndex = mc.hat; state.browsLayer.testIndex = mc.brows;
    state.bgLayer.testIndex   = mc.bg_scene; state.manualFxMask = mc.fx_mask;
    state.dualProfileMode     = mc.dual_profile;
    state.manualHideMouth     = mc.hide_mouth;
    g_override_teeth     = (mc.teeth >= 0)     ? mc.teeth     : 1;
    g_override_lip_color = (mc.lip_color >= 0) ? mc.lip_color : 3;
    g_override_eyelash   = (mc.eyelash >= 0)   ? mc.eyelash   : 0;
    g_override_eye_style = mc.eye_style;
    g_override_iris      = mc.iris;
    selectEyeBankIndex(mc.eye_bank);
    if (updateAnim) state.manualTestAnim = (mc.anim_id > 0) ? mc.anim_id : fallbackAnim;
}

// ------------------------------------------------------------------------------
// 18. Apply a saved macro preset slot to runtime layer indices and style overrides
// ------------------------------------------------------------------------------
static void applyMacroComboToState(const FaceAppConfig& cfg, AppRuntimeState& state, int slot, bool updateAnim)
{
    int fallbackAnim = (slot == 0) ? 1 : ((slot - 1) % 64) + 1;
    auto it = cfg.macro_combos.find(slot);
    if (it == cfg.macro_combos.end() || !it->second.enabled)
    {
        if (slot == 0)
        {
            state.faceLayer.testIndex = 1; state.makeupLayer.testIndex = 0; state.earsLayer.testIndex = 0;
            state.browsLayer.testIndex = 1; state.noseLayer.testIndex = 1; state.beardLayer.testIndex = 0;
            state.glassesLayer.testIndex = 0; state.hairLayer.testIndex = 0; state.hatLayer.testIndex = 0;
            state.bgLayer.testIndex = 0; state.manualFxMask = 0; state.dualProfileMode = false;
        }
        state.manualHideMouth = false;
        g_override_teeth = 1; g_override_lip_color = 3; g_override_eyelash = 0;
        g_override_eye_style = -1; g_override_iris = -1;
        selectEyeBankIndex(-1);
        if (updateAnim) state.manualTestAnim = fallbackAnim;
        return;
    }
    applyPresetValuesToState(it->second, state, fallbackAnim, updateAnim);
}

// ------------------------------------------------------------------------------
// 19. Return sorted vector of all enabled macro slot numbers for auto-show loop
// ------------------------------------------------------------------------------
static std::vector<int> getSavedSequentialMacroSlots(const FaceAppConfig& cfg)
{
    std::vector<int> slots;
    for (const auto& kv : cfg.macro_combos)
        if (kv.second.enabled) slots.push_back(kv.first);
    std::sort(slots.begin(), slots.end());
    return slots;
}
// ------------------------------------------------------------------------------
// 20. Resolve TAB calibration target index (0..13) to label, offset & scale ptrs
// ------------------------------------------------------------------------------
static void getEditableTarget(AppRuntimeState& state, int idx, const char*& outName, int*& outX, int*& outY, float*& outScale)
{
    outX = nullptr; outY = nullptr; outScale = nullptr;
    switch (idx) {
        case 0:  outName="EYE_LEFT";  outX=&state.eyeLeftOffsetX;       outY=&state.eyeLeftOffsetY;       outScale=&state.eyeScale;           break;
        case 1:  outName="EYE_RIGHT"; outX=&state.eyeRightOffsetX;      outY=&state.eyeRightOffsetY;      outScale=&state.eyeScale;           break;
        case 2:  outName="MOUTH";     outX=&state.mouthOffsetX;         outY=&state.mouthOffsetY;         outScale=&state.mouthScale;         break;
        case 3:  outName="NOSE";      outX=&state.noseLayer.offsetX;    outY=&state.noseLayer.offsetY;    outScale=&state.noseLayer.scale;    break;
        case 4:  outName="GLASSES";   outX=&state.glassesLayer.offsetX; outY=&state.glassesLayer.offsetY; outScale=&state.glassesLayer.scale; break;
        case 5:  outName="BROWS";     outX=&state.browsLayer.offsetX;   outY=&state.browsLayer.offsetY;   outScale=&state.browsLayer.scale;   break;
        case 6:  outName="BEARD";     outX=&state.beardLayer.offsetX;   outY=&state.beardLayer.offsetY;   outScale=&state.beardLayer.scale;   break;
        case 7:  outName="FACE";      outX=&state.faceLayer.offsetX;    outY=&state.faceLayer.offsetY;    outScale=&state.faceLayer.scale;    break;
        case 8:  outName="EARS";      outX=&state.earsLayer.offsetX;    outY=&state.earsLayer.offsetY;    outScale=&state.earsLayer.scale;    break;
        case 9:  outName="HAIR";      outX=&state.hairLayer.offsetX;    outY=&state.hairLayer.offsetY;    outScale=&state.hairLayer.scale;    break;
        case 10: outName="HAT";       outX=&state.hatLayer.offsetX;     outY=&state.hatLayer.offsetY;     outScale=&state.hatLayer.scale;     break;
        case 11: outName="MAKEUP";    outX=&state.makeupLayer.offsetX;  outY=&state.makeupLayer.offsetY;  outScale=&state.makeupLayer.scale;  break;
        case 12: outName="BG";        outX=&state.bgLayer.offsetX;      outY=&state.bgLayer.offsetY;      outScale=&state.bgLayer.scale;      break;
        default: outName="FX";        outX=&state.fxLayer.offsetX;      outY=&state.fxLayer.offsetY;      outScale=&state.fxLayer.scale;      break;
    }
}

// ------------------------------------------------------------------------------
// 21. Apply per-face alignment profile including splitOffsetX and layer scales
// ------------------------------------------------------------------------------
static void applyMaskOffsetProfile(const FaceAppConfig& cfg, AppRuntimeState& state, uint8_t maskId)
{
    state.activeProfileId = maskId;
    MaskOffsetProfile prof{};
    auto it = cfg.mask_profiles.find(maskId);
    if (it != cfg.mask_profiles.end()) prof = it->second;
    else {
        auto defIt = cfg.mask_profiles.find(0);
        if (defIt != cfg.mask_profiles.end()) prof = defIt->second;
        if (maskId >= PROFILE_FACE_START_INDEX) prof.face_split = 75;
    }

    auto clampSc = [](float v) { return std::max(0.50f, std::min(1.60f, v)); };
    state.eyeLeftOffsetX = prof.eye_left_x; state.eyeLeftOffsetY = prof.eye_left_y;
    state.eyeRightOffsetX = prof.eye_right_x; state.eyeRightOffsetY = prof.eye_right_y;
    state.mouthOffsetX = prof.mouth_x; state.mouthOffsetY = prof.mouth_y; state.mouthSplitX = prof.mouth_split;
    state.noseLayer.offsetX = prof.nose_x; state.noseLayer.offsetY = prof.nose_y; state.noseLayer.splitOffsetX = prof.nose_split; state.noseLayer.scale = clampSc(prof.nose_scale);
    state.glassesLayer.offsetX = prof.glasses_x; state.glassesLayer.offsetY = prof.glasses_y; state.glassesLayer.splitOffsetX = prof.glasses_split; state.glassesLayer.scale = clampSc(prof.glasses_scale);
    state.browsLayer.offsetX = prof.brows_x; state.browsLayer.offsetY = prof.brows_y; state.browsLayer.splitOffsetX = prof.brows_split; state.browsLayer.scale = clampSc(prof.brows_scale);
    state.beardLayer.offsetX = prof.beard_x; state.beardLayer.offsetY = prof.beard_y; state.beardLayer.splitOffsetX = prof.beard_split; state.beardLayer.scale = clampSc(prof.beard_scale);
    state.faceLayer.offsetX = prof.face_x; state.faceLayer.offsetY = prof.face_y; state.faceLayer.splitOffsetX = prof.face_split; state.faceLayer.scale = clampSc(prof.face_scale);
    state.earsLayer.offsetX = prof.ears_x; state.earsLayer.offsetY = prof.ears_y; state.earsLayer.splitOffsetX = prof.ears_split; state.earsLayer.scale = clampSc(prof.ears_scale);
    state.hairLayer.offsetX = prof.hair_x; state.hairLayer.offsetY = prof.hair_y; state.hairLayer.scale = clampSc(prof.hair_scale);
    state.hatLayer.offsetX = prof.hat_x; state.hatLayer.offsetY = prof.hat_y; state.hatLayer.scale = clampSc(prof.hat_scale);
    state.makeupLayer.offsetX = prof.makeup_x; state.makeupLayer.offsetY = prof.makeup_y; state.makeupLayer.scale = clampSc(prof.makeup_scale);
    state.bgLayer.offsetX = prof.bg_x; state.bgLayer.offsetY = prof.bg_y; state.bgLayer.scale = clampSc(prof.bg_scale);
    state.fxLayer.offsetX = prof.fx_x; state.fxLayer.offsetY = prof.fx_y; state.fxLayer.scale = clampSc(prof.fx_scale);
    state.eyeScale   = clampSc(prof.eye_scale);
    state.mouthScale = clampSc(prof.mouth_scale);
    g_eye_scale_factor.store(state.eyeScale, std::memory_order_relaxed);
    g_mouth_scale_factor.store(state.mouthScale, std::memory_order_relaxed);
    g_profile_face_split_x.store(state.faceLayer.splitOffsetX, std::memory_order_relaxed);
    g_profile_mouth_split_x.store(state.mouthSplitX, std::memory_order_relaxed);
}

// ------------------------------------------------------------------------------
// 22. Capture active runtime offsets, split distances and scales into a profile
// ------------------------------------------------------------------------------
static MaskOffsetProfile captureCurrentProfile(const AppRuntimeState& state)
{
    return {state.eyeLeftOffsetX, state.eyeLeftOffsetY, state.eyeRightOffsetX, state.eyeRightOffsetY,
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
            state.bgLayer.scale, state.fxLayer.scale};
}

// ------------------------------------------------------------------------------
// 23. Trim leading and trailing whitespace characters from an INI line
// ------------------------------------------------------------------------------
static std::string trimWhitespace(const std::string& s)
{
    size_t f = s.find_first_not_of(" \t\r\n");
    return (f == std::string::npos) ? "" : s.substr(f, s.find_last_not_of(" \t\r\n") - f + 1);
}

// ------------------------------------------------------------------------------
// 24. Parse makingfaces.ini into network, display, offset profiles and macros
// ------------------------------------------------------------------------------
static void loadIniConfig(const std::string& path, FaceAppConfig& cfg)
{
    std::ifstream in(path);
    if (!in.is_open()) return;
    std::string sec = "", line;
    uint8_t mSec = 0;
    int mcSec = -1;
    while (std::getline(in, line))
    {
        std::string c = trimWhitespace(line);
        if (c.empty() || c.front() == '#' || c.front() == ';') continue;
        if (c.front() == '[' && c.back() == ']')
        {
            sec = c.substr(1, c.size() - 2);
            if (sec == "offsets" || sec == "offsets_default") mSec = 0;
            else if (sec.rfind("offsets_face_", 0) == 0) mSec = static_cast<uint8_t>(std::stoi(sec.substr(13)));
            else if (sec.rfind("macro_combo_", 0) == 0) mcSec = std::stoi(sec.substr(12));
            else mcSec = -1;
            continue;
        }
        size_t eq = c.find('=');
        if (eq == std::string::npos) continue;
        std::string k = trimWhitespace(c.substr(0, eq)), v = trimWhitespace(c.substr(eq + 1));
        try
        {
            if (sec == "offsets" || sec == "offsets_default" || sec.rfind("offsets_face_", 0) == 0)
            {
                auto& p = cfg.mask_profiles[mSec];
                if (k == "eye_scale") p.eye_scale = std::stof(v);
                else if (k == "mouth_scale") p.mouth_scale = std::stof(v);
                else if (k == "nose_scale") p.nose_scale = std::stof(v);
                else if (k == "glasses_scale") p.glasses_scale = std::stof(v);
                else if (k == "brows_scale") p.brows_scale = std::stof(v);
                else if (k == "beard_scale") p.beard_scale = std::stof(v);
                else if (k == "face_scale") p.face_scale = std::stof(v);
                else if (k == "ears_scale") p.ears_scale = std::stof(v);
                else if (k == "hair_scale") p.hair_scale = std::stof(v);
                else if (k == "hat_scale") p.hat_scale = std::stof(v);
                else if (k == "makeup_scale") p.makeup_scale = std::stof(v);
                else if (k == "bg_scale") p.bg_scale = std::stof(v);
                else if (k == "fx_scale") p.fx_scale = std::stof(v);
                else {
                    int iv = std::stoi(v);
                    if (k == "eye_left_x") p.eye_left_x = iv; else if (k == "eye_left_y") p.eye_left_y = iv;
                    else if (k == "eye_right_x") p.eye_right_x = iv; else if (k == "eye_right_y") p.eye_right_y = iv;
                    else if (k == "mouth_x") p.mouth_x = iv; else if (k == "mouth_y") p.mouth_y = iv;
                    else if (k == "mouth_split") p.mouth_split = iv;
                    else if (k == "nose_x") p.nose_x = iv; else if (k == "nose_y") p.nose_y = iv;
                    else if (k == "nose_split") p.nose_split = iv;
                    else if (k == "glasses_x") p.glasses_x = iv; else if (k == "glasses_y") p.glasses_y = iv;
                    else if (k == "glasses_split") p.glasses_split = iv;
                    else if (k == "brows_x") p.brows_x = iv; else if (k == "brows_y") p.brows_y = iv;
                    else if (k == "brows_split") p.brows_split = iv;
                    else if (k == "beard_x") p.beard_x = iv; else if (k == "beard_y") p.beard_y = iv;
                    else if (k == "beard_split") p.beard_split = iv;
                    else if (k == "face_x") p.face_x = iv; else if (k == "face_y") p.face_y = iv;
                    else if (k == "face_split") p.face_split = iv;
                    else if (k == "ears_x") p.ears_x = iv; else if (k == "ears_y") p.ears_y = iv;
                    else if (k == "ears_split") p.ears_split = iv;
                    else if (k == "hair_x") p.hair_x = iv; else if (k == "hair_y") p.hair_y = iv;
                    else if (k == "hat_x") p.hat_x = iv; else if (k == "hat_y") p.hat_y = iv;
                    else if (k == "makeup_x") p.makeup_x = iv; else if (k == "makeup_y") p.makeup_y = iv;
                    else if (k == "bg_x") p.bg_x = iv; else if (k == "bg_y") p.bg_y = iv;
                    else if (k == "fx_x") p.fx_x = iv; else if (k == "fx_y") p.fx_y = iv;
                }
            }
            else if (sec.rfind("macro_combo_", 0) == 0 && mcSec >= 0 && mcSec <= MAX_MACRO_SLOT)
            {
                auto& mc = cfg.macro_combos[mcSec];
                mc.enabled = true;
                if (k == "fx_mask") {
                    mc.fx_mask = static_cast<uint32_t>(std::stoul(v));
                } else {
                    int iv = std::stoi(v);
                    if (k == "face") mc.face = iv; else if (k == "makeup") mc.makeup = iv; else if (k == "ears") mc.ears = iv;
                    else if (k == "nose") mc.nose = iv; else if (k == "beard") mc.beard = iv; else if (k == "glasses") mc.glasses = iv;
                    else if (k == "hair") mc.hair = iv; else if (k == "hat") mc.hat = iv; else if (k == "brows") mc.brows = iv;
                    else if (k == "bg_scene") mc.bg_scene = iv;
                    else if (k == "teeth") mc.teeth = iv; else if (k == "lip_color") mc.lip_color = iv; else if (k == "eyelash") mc.eyelash = iv;
                    else if (k == "eye_style") mc.eye_style = iv; else if (k == "iris") mc.iris = iv; else if (k == "eye_bank") mc.eye_bank = iv;
                    else if (k == "anim_id") mc.anim_id = iv; else if (k == "dual_profile") mc.dual_profile = (iv != 0);
                    else if (k == "hide_mouth") mc.hide_mouth = (iv != 0);
                }
            }
            else if (k == "proto") cfg.proto = v; else if (k == "universe") cfg.universe = std::stoi(v);
            else if (k == "channel" || k == "start_channel") cfg.start_channel = std::stoi(v);
            else if (k == "fps") cfg.fps = std::stoi(v); else if (k == "display") cfg.display = std::stoi(v);
            else if (k == "fullscreen") cfg.fullscreen = (v == "1" || v == "true");
            else if (k == "smooth" || k == "smooth_scale") cfg.smooth_scale = (v == "1" || v == "true");
            else if (k == "fade_ms") cfg.fade_ms = std::max(0, std::min(4000, std::stoi(v)));
            else if (k == "save_hold_ms") cfg.save_hold_ms = std::max(0, std::min(60000, std::stoi(v)));
            else if (k == "audio_dir") cfg.audio_dir = v; else if (k == "assets_dir") cfg.assets_dir = v;
            else if (k == "audio_monitor") cfg.audio_monitor = v;
        }
        catch (...) {}
    }
}


// ------------------------------------------------------------------------------
// 25. Persist network, display, offset/split/scale profiles and macros to INI
// ------------------------------------------------------------------------------
static bool saveIniConfig(SDL_Window* window, FaceAppConfig& cfg, const AppRuntimeState& state)
{
    if (window) {
        int curDisp = SDL_GetWindowDisplayIndex(window);
        if (curDisp >= 0) cfg.display = curDisp;
    }
    cfg.mask_profiles[state.activeProfileId] = captureCurrentProfile(state);
    std::ofstream out(cfg.config_file);
    if (!out.is_open()) return false;
    out << "[network]\nproto=" << cfg.proto << "\nuniverse=" << cfg.universe << "\nstart_channel=" << cfg.start_channel
        << "\n\n[display]\ndisplay=" << cfg.display << "\nfps=" << cfg.fps
        << "\nfullscreen=" << (state.isFullscreen ? "true" : "false")
        << "\nsmooth_scale=" << (cfg.smooth_scale ? "true" : "false")
        << "\nfade_ms=" << cfg.fade_ms << "\nsave_hold_ms=" << cfg.save_hold_ms
        << "\n\n[paths]\nassets_dir=" << cfg.assets_dir << "\naudio_dir=" << cfg.audio_dir << "\naudio_monitor=" << cfg.audio_monitor << "\n";
    for (const auto& kv : cfg.mask_profiles)
    {
        std::array<char, 32> sec{};
        if (kv.first == 0) std::snprintf(sec.data(), sec.size(), "offsets_default");
        else std::snprintf(sec.data(), sec.size(), "offsets_face_%03u", kv.first);
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
    for (const auto& kv : cfg.macro_combos)
    {
        if (!kv.second.enabled) continue;
        const auto& mc = kv.second;
        out << "\n[macro_combo_" << kv.first << "]\nface=" << (int)mc.face << "\nmakeup=" << (int)mc.makeup
            << "\nears=" << (int)mc.ears << "\nnose=" << (int)mc.nose << "\nbeard=" << (int)mc.beard
            << "\nglasses=" << (int)mc.glasses << "\nhair=" << (int)mc.hair << "\nhat=" << (int)mc.hat
            << "\nbrows=" << (int)mc.brows << "\nbg_scene=" << (int)mc.bg_scene << "\nfx_mask=" << mc.fx_mask
            << "\nteeth=" << mc.teeth << "\nlip_color=" << mc.lip_color << "\neyelash=" << mc.eyelash
            << "\neye_style=" << mc.eye_style << "\niris=" << mc.iris << "\neye_bank=" << mc.eye_bank
            << "\nanim_id=" << mc.anim_id << "\ndual_profile=" << (mc.dual_profile ? 1 : 0)
            << "\nhide_mouth=" << (mc.hide_mouth ? 1 : 0) << "\n";
    }
    return true;
}

// ==============================================================================
// Section E. ZIP Pack Browser, Live Inspector, Selective Import & 5-Step Undo
// ==============================================================================

// ------------------------------------------------------------------------------
// 26. Archive current assets/ and makingfaces.ini into packs/<name>.zip
// ------------------------------------------------------------------------------
static bool saveCurrentAssetsToZipPack(const std::string& packName, const std::string& assetsDir, const std::string& iniFile, const std::string& packsDir = "packs")
{
    if (!fs::exists(assetsDir) || !fs::exists(iniFile)) return false;
    fs::create_directories(packsDir);
    fs::path zipPath = fs::path(packsDir) / (packName + ".zip");
    if (fs::exists(zipPath)) fs::remove(zipPath);

    std::string cmd = "zip -rq \"" + zipPath.string() + "\" \"" + assetsDir + "\" \"" + iniFile + "\"";
    int rc = std::system(cmd.c_str());
    return (rc == 0 && fs::exists(zipPath));
}
// ------------------------------------------------------------------------------
// 27. Rotate 5-level undo snapshots (.undo_1..5.zip) before modifying default.zip
// ------------------------------------------------------------------------------
static void pushUndoSnapshot(const std::string& assetsDir, const std::string& iniFile, const std::string& packsDir = "packs")
{
    fs::create_directories(packsDir);
    fs::path oldest = fs::path(packsDir) / (".undo_" + std::to_string(MAX_UNDO_LEVELS) + ".zip");
    if (fs::exists(oldest)) fs::remove(oldest);
    for (int i = MAX_UNDO_LEVELS - 1; i >= 1; --i) {
        fs::path src = fs::path(packsDir) / (".undo_" + std::to_string(i) + ".zip");
        fs::path dst = fs::path(packsDir) / (".undo_" + std::to_string(i + 1) + ".zip");
        if (fs::exists(src)) fs::rename(src, dst);
    }
    fs::path defZip = fs::path(packsDir) / "default.zip";
    fs::path u1 = fs::path(packsDir) / ".undo_1.zip";
    if (fs::exists(defZip)) fs::copy_file(defZip, u1, fs::copy_options::overwrite_existing);
    else saveCurrentAssetsToZipPack(".undo_1", assetsDir, iniFile, packsDir);
}

// ------------------------------------------------------------------------------
// 28. Generate next unused custom_NNN pack name in packs/ directory
// ------------------------------------------------------------------------------
static std::string nextCustomZipPackName(const std::string& packsDir = "packs")
{
    for (int idx = 1; idx <= 999; ++idx) {
        std::array<char, 32> buf{};
        std::snprintf(buf.data(), buf.size(), "custom_%03d", idx);
        fs::path candidate = fs::path(packsDir) / (std::string(buf.data()) + ".zip");
        if (!fs::exists(candidate)) return std::string(buf.data());
    }
    return "custom_999";
}

// ------------------------------------------------------------------------------
// 29. Free all cached thumbnail textures in the Pack Browser
// ------------------------------------------------------------------------------
static void destroyPackBrowserEntries(PackBrowserState& pb)
{
    for (auto& e : pb.entries) {
        if (e.thumbTex) { SDL_DestroyTexture(e.thumbTex); e.thumbTex = nullptr; }
    }
    pb.entries.clear();
}

// ------------------------------------------------------------------------------
// 30. Scan packs/*.zip and render composite preview thumbnails (bg + full char)
// ------------------------------------------------------------------------------
static void refreshPackBrowserEntries(SDL_Renderer* ren, PackBrowserState& pb, const std::string& packsDir = "packs")
{
    destroyPackBrowserEntries(pb);
    if (!fs::exists(packsDir) || !fs::is_directory(packsDir)) return;

    std::vector<fs::path> zips;
    for (const auto& entry : fs::directory_iterator(packsDir)) {
        if (!entry.is_regular_file() || entry.path().extension() != ".zip") continue;
        std::string stem = entry.path().stem().string();
        if (!stem.empty() && stem.front() == '.') continue;
        zips.push_back(entry.path());
    }
    std::sort(zips.begin(), zips.end());

    fs::path tmpDir = fs::path(packsDir) / ".thumb_tmp";
    static const std::array<const char*, 9> kCharLayers = {
        "ears", "face", "makeup", "nose", "brows", "beard", "glasses", "hair", "hat"
    };

    for (const auto& zp : zips) {
        if (fs::exists(tmpDir)) fs::remove_all(tmpDir);
        fs::create_directories(tmpDir);
        std::string cmd = "unzip -oq \"" + zp.string() + "\" -d \"" + tmpDir.string() + "\"";
        if (std::system(cmd.c_str()) != 0) continue;

        PackBrowserEntry item{};
        item.name = zp.stem().string();
        FaceAppConfig tmpCfg{};
        loadIniConfig((tmpDir / "makingfaces.ini").string(), tmpCfg);
        for (const auto& kv : tmpCfg.macro_combos) if (kv.second.enabled) ++item.macroCount;

        fs::path faceDir = tmpDir / "assets" / "face";
        if (fs::exists(faceDir)) {
            for (const auto& f : fs::directory_iterator(faceDir))
                if (f.is_regular_file() && f.path().extension() == ".png") ++item.faceCount;
        }

        SDL_Texture* target = SDL_CreateTexture(ren, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_TARGET, 300, 400);
        if (target) {
            SDL_SetRenderTarget(ren, target);
            SDL_SetRenderDrawColor(ren, 16, 22, 34, 255);
            SDL_RenderClear(ren);
            auto blit = [&](const fs::path& p) {
                if (!fs::exists(p)) return;
                SDL_Texture* t = IMG_LoadTexture(ren, p.string().c_str());
                if (t) { SDL_RenderCopy(ren, t, nullptr, nullptr); SDL_DestroyTexture(t); }
            };
            blit(tmpDir / "assets" / "bg" / "001_sky.png");
            blit(tmpDir / "assets" / "bg" / "001_far.png");
            blit(tmpDir / "assets" / "bg" / "001_mid.png");
            blit(tmpDir / "assets" / "bg" / "001.png");
            for (const char* lyr : kCharLayers) blit(tmpDir / "assets" / lyr / "001.png");
            blit(tmpDir / "assets" / "bg" / "001_front.png");
            SDL_SetRenderTarget(ren, nullptr);
            item.thumbTex = target;
        }
        pb.entries.push_back(item);
    }
    if (fs::exists(tmpDir)) fs::remove_all(tmpDir);
    if (pb.selectedIdx >= static_cast<int>(pb.entries.size())) pb.selectedIdx = 0;
}
// ------------------------------------------------------------------------------
// 31. Clear cached SDL layer textures and reset active runtime state to #000
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
        if (l->texture) { SDL_DestroyTexture(l->texture); l->texture = nullptr; }
        l->loadedIndex = 0;
    }
    if (ren) {
        destroyParallaxBackdrop(state.parallaxBg);
        destroyFxSpriteBank(state.fxBank);
        initFxSpriteBank(ren, cfg.assets_dir, state.fxBank);
    }
    reloadDynamicEyeBank(cfg.assets_dir);
    resetAutonomousMacroShow();
    state.manualMacroSlot = -1; state.browseMacroSlot = 0;
    state.appliedMacroSlot = -1; state.manualTestAnim = 0;
    applyMacroComboToState(cfg, state, 0, false);
    applyMaskOffsetProfile(cfg, state, state.faceLayer.testIndex);
}

// ------------------------------------------------------------------------------
// 32. Find highest existing index in assets/<folder> for front (<127) or profile
// ------------------------------------------------------------------------------
static int findMaxAssetSlotInDir(const fs::path& dir, bool profileRange)
{
    int maxId = profileRange ? (PROFILE_FACE_START_INDEX - 1) : 0;
    if (!fs::exists(dir) || !fs::is_directory(dir)) return maxId;
    for (const auto& entry : fs::directory_iterator(dir)) {
        std::string stem = entry.path().stem().string();
        if (stem.size() >= 3 && std::isdigit(stem.at(0)) && std::isdigit(stem.at(1)) && std::isdigit(stem.at(2))) {
            int id = std::stoi(stem.substr(0, 3));
            if (!profileRange && id < PROFILE_FACE_START_INDEX) maxId = std::max(maxId, id);
            if (profileRange && id >= PROFILE_FACE_START_INDEX) maxId = std::max(maxId, id);
        }
    }
    return maxId;
}

// ------------------------------------------------------------------------------
// 33. Unpack selected .zip into packs/.inspect_tmp and populate inspectItems list
// ------------------------------------------------------------------------------
static bool openZipPackInspector(const std::string& packName, PackBrowserState& pb, const std::string& packsDir = "packs")
{
    fs::path zipPath = fs::path(packsDir) / (packName + ".zip");
    if (!fs::exists(zipPath)) return false;

    fs::path tmpDir = fs::path(packsDir) / ".inspect_tmp";
    if (fs::exists(tmpDir)) fs::remove_all(tmpDir);
    fs::create_directories(tmpDir);
    std::string cmd = "unzip -oq \"" + zipPath.string() + "\" -d \"" + tmpDir.string() + "\"";
    if (std::system(cmd.c_str()) != 0) return false;

    FaceAppConfig tmpCfg{};
    loadIniConfig((tmpDir / "makingfaces.ini").string(), tmpCfg);
    pb.inspectItems.clear();
    for (const auto& kv : tmpCfg.macro_combos) {
        if (!kv.second.enabled) continue;
        PackInspectItem item{};
        item.srcSlot = kv.first;
        item.selected = true;
        item.preset = kv.second;
        std::array<char, 84> lbl{};
        std::snprintf(lbl.data(), lbl.size(), "#%03d %s [F:%03u BG:%03u%s]",
                      kv.first, getMacroAnimationName(kv.second.anim_id),
                      kv.second.face, kv.second.bg_scene,
                      kv.second.dual_profile ? " DUAL" : "");
        item.label = lbl.data();
        pb.inspectItems.push_back(item);
    }
    if (pb.inspectItems.empty()) return false;
    pb.inspectMode = true;
    pb.inspectIdx = 0;
    return true;
}
// ------------------------------------------------------------------------------
// 34. Copy selected macro items and their referenced PNGs from .inspect_tmp
// ------------------------------------------------------------------------------
static bool appendCheckedItemsFromZipPack(SDL_Window* window, const std::string& packName, const PackBrowserState& pb, FaceAppConfig& cfg, AppRuntimeState& state, const std::string& packsDir = "packs")
{
    fs::path tmpDir = fs::path(packsDir) / ".inspect_tmp";
    if (!fs::exists(tmpDir)) {
        fs::path zipPath = fs::path(packsDir) / (packName + ".zip");
        if (!fs::exists(zipPath)) return false;
        fs::create_directories(tmpDir);
        std::string cmd = "unzip -oq \"" + zipPath.string() + "\" -d \"" + tmpDir.string() + "\"";
        if (std::system(cmd.c_str()) != 0) return false;
    }

    std::vector<MacroComboPreset> chosen;
    for (const auto& it : pb.inspectItems) if (it.selected) chosen.push_back(it.preset);
    if (chosen.empty()) return false;

    pushUndoSnapshot(cfg.assets_dir, cfg.config_file, packsDir);
    static const std::array<const char*, 10> kFolders = {"face", "makeup", "ears", "nose", "brows", "beard", "glasses", "hair", "hat", "bg"};
    std::map<std::string, std::map<int, int>> remap;
    std::map<std::string, int> nextFront, nextProfOdd;

    for (const char* f : kFolders) {
        fs::path dstDir = fs::path(cfg.assets_dir) / f;
        fs::create_directories(dstDir);
        nextFront[f] = findMaxAssetSlotInDir(dstDir, false) + 1;
        int mp = findMaxAssetSlotInDir(dstDir, true);
        nextProfOdd[f] = (mp < PROFILE_FACE_START_INDEX) ? PROFILE_FACE_START_INDEX : ((mp % 2 == 1) ? (mp + 2) : (mp + 1));
    }

    auto copyAssetSlot = [&](const char* folder, uint8_t oldId) -> uint8_t {
        if (oldId == 0) return 0;
        if (remap[folder].count(oldId)) return static_cast<uint8_t>(remap[folder][oldId]);

        fs::path srcDir = tmpDir / "assets" / folder;
        fs::path dstDir = fs::path(cfg.assets_dir) / folder;
        bool isProf = (oldId >= PROFILE_FACE_START_INDEX && std::strcmp(folder, "bg") != 0);
        int oddBase = isProf ? ((oldId % 2 == 1) ? oldId : (oldId - 1)) : oldId;

        if (isProf) {
            remap[folder][oddBase] = nextProfOdd[folder];
            remap[folder][oddBase + 1] = nextProfOdd[folder] + 1;
            nextProfOdd[folder] += 2;
        } else {
            remap[folder][oldId] = std::min(255, nextFront[folder]++);
        }

        if (fs::exists(srcDir)) {
            for (const auto& fp : fs::directory_iterator(srcDir)) {
                if (!fp.is_regular_file()) continue;
                std::string stem = fp.path().stem().string();
                if (stem.size() < 3 || !std::isdigit(stem.at(0)) || !std::isdigit(stem.at(1)) || !std::isdigit(stem.at(2))) continue;
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
    int nextMacro = 0;
    for (const auto& kv : cfg.macro_combos) if (kv.second.enabled && kv.first >= nextMacro) nextMacro = kv.first + 1;

    for (auto mc : chosen) {
        if (nextMacro > MAX_MACRO_SLOT) break;
        uint8_t origFace = mc.face;
        mc.face = copyAssetSlot("face", mc.face);
        mc.makeup = copyAssetSlot("makeup", mc.makeup);
        mc.ears = copyAssetSlot("ears", mc.ears);
        mc.nose = copyAssetSlot("nose", mc.nose);
        mc.brows = copyAssetSlot("brows", mc.brows);
        mc.beard = copyAssetSlot("beard", mc.beard);
        mc.glasses = copyAssetSlot("glasses", mc.glasses);
        mc.hair = copyAssetSlot("hair", mc.hair);
        mc.hat = copyAssetSlot("hat", mc.hat);
        mc.bg_scene = copyAssetSlot("bg", mc.bg_scene);
        if (origFace > 0 && incCfg.mask_profiles.count(origFace)) cfg.mask_profiles[mc.face] = incCfg.mask_profiles.at(origFace);
        cfg.macro_combos[nextMacro++] = mc;
    }

    if (fs::exists(tmpDir)) fs::remove_all(tmpDir);
    saveIniConfig(window, cfg, state);
    saveCurrentAssetsToZipPack("default", cfg.assets_dir, cfg.config_file, packsDir);
    reloadAllTexturesAndState(SDL_GetRenderer(window), cfg, state);
    return true;
}
// ------------------------------------------------------------------------------
// 35. Append an entire external .zip theme pack onto the end of active assets
// ------------------------------------------------------------------------------
static bool appendZipPackToActive(SDL_Window* window, const std::string& packName, FaceAppConfig& cfg, AppRuntimeState& state, const std::string& packsDir = "packs")
{
    PackBrowserState tmpPb{};
    if (!openZipPackInspector(packName, tmpPb, packsDir)) return false;
    bool ok = appendCheckedItemsFromZipPack(window, packName, tmpPb, cfg, state, packsDir);
    return ok;
}

// ------------------------------------------------------------------------------
// 36. Replace active theme with selected .zip pack and update packs/default.zip
// ------------------------------------------------------------------------------
static bool replaceActiveWithZipPack(SDL_Window* window, const std::string& packName, FaceAppConfig& cfg, AppRuntimeState& state, const std::string& packsDir = "packs")
{
    fs::path zipPath = fs::path(packsDir) / (packName + ".zip");
    if (!fs::exists(zipPath) || fs::file_size(zipPath) == 0) return false;

    pushUndoSnapshot(cfg.assets_dir, cfg.config_file, packsDir);
    if (fs::exists(cfg.assets_dir)) fs::remove_all(cfg.assets_dir);
    if (fs::exists(cfg.config_file)) fs::remove(cfg.config_file);

    std::string cmd = "unzip -oq \"" + zipPath.string() + "\" -d .";
    if (std::system(cmd.c_str()) != 0) return false;

    cfg.macro_combos.clear();
    cfg.mask_profiles.clear();
    loadIniConfig(cfg.config_file, cfg);
    saveCurrentAssetsToZipPack("default", cfg.assets_dir, cfg.config_file, packsDir);
    reloadAllTexturesAndState(SDL_GetRenderer(window), cfg, state);
    return true;
}

// ------------------------------------------------------------------------------
// 37. Restore latest undo snapshot (.undo_1.zip) and shift undo stack forward
// ------------------------------------------------------------------------------
static bool restoreUndoSnapshot(SDL_Window* window, FaceAppConfig& cfg, AppRuntimeState& state, const std::string& packsDir = "packs")
{
    fs::path u1 = fs::path(packsDir) / ".undo_1.zip";
    if (!fs::exists(u1) || fs::file_size(u1) == 0) return false;

    if (fs::exists(cfg.assets_dir)) fs::remove_all(cfg.assets_dir);
    if (fs::exists(cfg.config_file)) fs::remove(cfg.config_file);
    std::string cmd = "unzip -oq \"" + u1.string() + "\" -d .";
    if (std::system(cmd.c_str()) != 0) return false;

    fs::remove(u1);
    for (int i = 2; i <= MAX_UNDO_LEVELS; ++i) {
        fs::path src = fs::path(packsDir) / (".undo_" + std::to_string(i) + ".zip");
        fs::path dst = fs::path(packsDir) / (".undo_" + std::to_string(i - 1) + ".zip");
        if (fs::exists(src)) fs::rename(src, dst);
    }

    cfg.macro_combos.clear();
    cfg.mask_profiles.clear();
    loadIniConfig(cfg.config_file, cfg);
    saveCurrentAssetsToZipPack("default", cfg.assets_dir, cfg.config_file, packsDir);
    reloadAllTexturesAndState(SDL_GetRenderer(window), cfg, state);
    return true;
}

// ------------------------------------------------------------------------------
// 38. Wipe all character/background assets and macros while preserving FX sprites
// ------------------------------------------------------------------------------
static void wipeAllAssetsAndMacros(SDL_Window* window, FaceAppConfig& cfg, AppRuntimeState& state, const std::string& packsDir = "packs")
{
    pushUndoSnapshot(cfg.assets_dir, cfg.config_file, packsDir);
    static const std::array<const char*, 10> kClearDirs = {"face", "makeup", "ears", "nose", "brows", "beard", "glasses", "hair", "hat", "bg"};
    for (const char* d : kClearDirs) {
        fs::path p = fs::path(cfg.assets_dir) / d;
        if (fs::exists(p)) fs::remove_all(p);
        fs::create_directories(p);
    }
    cfg.macro_combos.clear();
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
// 39. Free all cached preview textures in Calibration & Import Studio
// ------------------------------------------------------------------------------
static void destroyFlatImportItems(FlatImportState& fi)
{
    for (auto& it : fi.items) {
        if (it.previewTex) { SDL_DestroyTexture(it.previewTex); it.previewTex = nullptr; }
    }
    fi.items.clear();
}

// ------------------------------------------------------------------------------
// 40. Flood-fill corner-sampled & white outer background to transparent BGRA
// ------------------------------------------------------------------------------
static cv::Mat stripWhiteStudioBackgroundCv(const cv::Mat& bgr, int tol)
{
    int h = bgr.rows, w = bgr.cols;
    int bw = std::max(1, std::min(8, w / 8)), bh = std::max(1, std::min(8, h / 8));
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
            float db = pb - refB, dg = pg - refG, dr = pr - refR;
            float wb = 255.0f - pb, wg = 255.0f - pg, wr = 255.0f - pr;
            if ((db * db + dg * dg + dr * dr) <= maxDistSq || (wb * wb + wg * wg + wr * wr) <= maxDistSq) *(mRow + x) = 255;
        }
    }

    cv::Mat ffMask = cv::Mat::zeros(h + 2, w + 2, CV_8UC1);
    for (int x = 0; x < w; x += 2) {
        if (bgMask.at<uint8_t>(0, x) == 255) cv::floodFill(bgMask, ffMask, cv::Point(x, 0), cv::Scalar(128));
        if (bgMask.at<uint8_t>(h - 1, x) == 255) cv::floodFill(bgMask, ffMask, cv::Point(x, h - 1), cv::Scalar(128));
    }
    for (int y = 0; y < h; y += 2) {
        if (bgMask.at<uint8_t>(y, 0) == 255) cv::floodFill(bgMask, ffMask, cv::Point(0, y), cv::Scalar(128));
        if (bgMask.at<uint8_t>(y, w - 1) == 255) cv::floodFill(bgMask, ffMask, cv::Point(w - 1, y), cv::Scalar(128));
    }

    cv::Mat alpha(h, w, CV_8UC1);
    for (int y = 0; y < h; ++y) {
        const uint8_t* nw = bgMask.ptr<uint8_t>(y);
        uint8_t* aRow = alpha.ptr<uint8_t>(y);
        for (int x = 0; x < w; ++x) *(aRow + x) = (*(nw + x) == 128) ? 0 : 255;
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
// 41. Build calibrated 600x800 (or 256x256 eye) BGRA matrix with Telea & cutouts
// ------------------------------------------------------------------------------
static cv::Mat buildCalibratedCanvasBgra(const FlatImportItem& item, bool fullQualityInpaint)
{
    cv::Mat raw = cv::imread(item.filePath, cv::IMREAD_UNCHANGED);
    if (raw.empty()) return cv::Mat();

    bool doWhite = item.removeWhite && (item.targetCatIdx < 9);
    cv::Mat bgra;
    if (raw.channels() == 4 && !doWhite) {
        bgra = raw.clone();
    } else {
        cv::Mat bgr;
        if (raw.channels() == 4) cv::cvtColor(raw, bgr, cv::COLOR_BGRA2BGR);
        else if (raw.channels() == 1) cv::cvtColor(raw, bgr, cv::COLOR_GRAY2BGR);
        else bgr = raw;
        bgra = doWhite ? stripWhiteStudioBackgroundCv(bgr, s_bgTolerance) : cv::Mat();
        if (bgra.empty()) cv::cvtColor(bgr, bgra, cv::COLOR_BGR2BGRA);
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
    cv::Mat bgrCanvas, alphaCanvas = ch.at(3).clone();
    cv::merge(std::vector<cv::Mat>{ch.at(0), ch.at(1), ch.at(2)}, bgrCanvas);

    bool isFaceOrMakeup = (item.targetCatIdx == 0 || item.targetCatIdx == 8);
    if (isFaceOrMakeup && item.doInpaint && fullQualityInpaint) {
        cv::Mat mask = cv::Mat::zeros(BASE_CANVAS_H, BASE_CANVAS_W, CV_8UC1);
        cv::ellipse(mask, cv::Point(item.noseX, item.noseY), cv::Size(item.noseRx, item.noseRy), 0, 0, 360, cv::Scalar(255), -1);
        cv::ellipse(mask, cv::Point(item.mouthX, item.mouthY), cv::Size(item.mouthRx, item.mouthRy), 0, 0, 360, cv::Scalar(255), -1);
        cv::bitwise_and(mask, alphaCanvas, mask);
        if (cv::countNonZero(mask) > 0) cv::inpaint(bgrCanvas, mask, bgrCanvas, 12.0, cv::INPAINT_TELEA);
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
// 42. Update live streaming preview texture for active Calibration Studio item
// ------------------------------------------------------------------------------
static void rebuildFlatImportPreviewTex(SDL_Renderer* ren, FlatImportItem& item, bool fastDrag = false)
{
    cv::Mat bgra = buildCalibratedCanvasBgra(item, !fastDrag);
    if (bgra.empty()) return;
    if (!item.previewTex) {
        item.previewTex = SDL_CreateTexture(ren, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, bgra.cols, bgra.rows);
        if (item.previewTex) SDL_SetTextureBlendMode(item.previewTex, SDL_BLENDMODE_BLEND);
    } else {
        int tw = 0, th = 0;
        SDL_QueryTexture(item.previewTex, nullptr, nullptr, &tw, &th);
        if (tw != bgra.cols || th != bgra.rows) {
            SDL_DestroyTexture(item.previewTex);
            item.previewTex = SDL_CreateTexture(ren, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, bgra.cols, bgra.rows);
            if (item.previewTex) SDL_SetTextureBlendMode(item.previewTex, SDL_BLENDMODE_BLEND);
        }
    }
    if (item.previewTex) SDL_UpdateTexture(item.previewTex, nullptr, bgra.data, static_cast<int>(bgra.step));
}

// ------------------------------------------------------------------------------
// 43. Scan import/ directory for PNG/JPG/WEBP files & guess target category
// ------------------------------------------------------------------------------
static void openFlatImportStudio(SDL_Renderer* ren, FlatImportState& fi, const std::string& importDir = "import")
{
    destroyFlatImportItems(fi);
    fs::create_directories(importDir);
    fs::create_directories(fs::path(importDir) / "done");

    std::vector<fs::path> files;
    for (const auto& entry : fs::directory_iterator(importDir)) {
        if (!entry.is_regular_file()) continue;
        std::string ext = entry.path().extension().string();
        for (char& ch : ext) ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
        if (ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".webp") files.push_back(entry.path());
    }
    std::sort(files.begin(), files.end());

    for (const auto& fp : files) {
        FlatImportItem it{};
        it.filePath = fp.string();
        it.fileName = fp.filename().string();
        std::string low = it.fileName;
        for (char& ch : low) ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
        if (low.find("hat") != std::string::npos || low.find("diadem") != std::string::npos || low.find("tiara") != std::string::npos || low.find("crown") != std::string::npos || low.find("cap") != std::string::npos) {
            it.targetCatIdx = 7; it.offsetY = -210; it.scale = 0.65f;
        }
        else if (low.find("glass") != std::string::npos) it.targetCatIdx = 5;
        else if (low.find("hair") != std::string::npos) it.targetCatIdx = 6;
        else if (low.find("beard") != std::string::npos) it.targetCatIdx = 4;
        else if (low.find("ear") != std::string::npos) it.targetCatIdx = 1;
        else if (low.find("nose") != std::string::npos) it.targetCatIdx = 2;
        else if (low.find("brow") != std::string::npos) it.targetCatIdx = 3;
        else if (low.find("bg") != std::string::npos) { it.targetCatIdx = 9; it.removeWhite = false; }
        else if (low.find("eye") != std::string::npos || low.find("iris") != std::string::npos) { it.targetCatIdx = 10; it.removeWhite = false; }
        else if (low.find("sclera") != std::string::npos) { it.targetCatIdx = 11; it.removeWhite = false; }
        fi.items.push_back(it);
    }
    fi.selectedIdx = 0;
    fi.dragTarget = 0;
    fi.dragging = false;
    fi.open = true;
    if (!fi.items.empty()) rebuildFlatImportPreviewTex(ren, fi.items.at(0), false);
}

// ------------------------------------------------------------------------------
// 44. Export checked Calibration Studio items into assets/ and packs/default.zip
// ------------------------------------------------------------------------------
static int executeFlatImportCommit(SDL_Window* window, FaceAppConfig& cfg, AppRuntimeState& state, const std::string& importDir = "import")
{
    static const std::array<const char*, 10> kFolders = {
        "face", "ears", "nose", "brows", "beard", "glasses", "hair", "hat", "makeup", "bg"
    };
    int importedCount = 0;
    for (const auto& it : state.flatImport.items) {
        if (!it.selected) continue;
        if (importedCount == 0) pushUndoSnapshot(cfg.assets_dir, cfg.config_file, "packs");

        cv::Mat finalBgra = buildCalibratedCanvasBgra(it, true);
        if (finalBgra.empty()) continue;

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
            std::array<char, 16> fn{}; std::snprintf(fn.data(), fn.size(), "%03d.png", slot);
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
// 45. Handle mouse drag & wheel scaling inside Calibration & Import Studio
// ------------------------------------------------------------------------------
static void handleFlatImportMouse(SDL_Window* win, SDL_Renderer* ren, const SDL_Event& ev, FlatImportState& fi)
{
    if (!fi.open || fi.items.empty()) return;
    auto& cur = fi.items.at(fi.selectedIdx);

    auto toCanvas = [&](int winX, int winY, int& outX, int& outY) {
        int outW = BASE_CANVAS_W, outH = BASE_CANVAS_H;
        if (win) SDL_GetWindowSize(win, &outW, &outH);
        else SDL_GetRendererOutputSize(ren, &outW, &outH);
        float sc = std::min(static_cast<float>(outW) / BASE_CANVAS_W, static_cast<float>(outH) / BASE_CANVAS_H);
        int vpW = static_cast<int>(std::round(BASE_CANVAS_W * sc));
        int vpH = static_cast<int>(std::round(BASE_CANVAS_H * sc));
        int vpX = (outW - vpW) / 2;
        int vpY = (outH - vpH) / 2;
        outX = static_cast<int>(std::round((winX - vpX) / std::max(0.01f, sc)));
        outY = static_cast<int>(std::round((winY - vpY) / std::max(0.01f, sc)));
    };

    if (ev.type == SDL_MOUSEBUTTONDOWN && ev.button.button == SDL_BUTTON_LEFT) {
        int mx = 0, my = 0;
        toCanvas(ev.button.x, ev.button.y, mx, my);
        auto hit = [&](int cx, int cy, int rx, int ry) {
            float dx = static_cast<float>(mx - cx) / std::max(1, rx);
            float dy = static_cast<float>(my - cy) / std::max(1, ry);
            return (dx * dx + dy * dy) <= 1.35f;
        };
        fi.dragTarget = 0;
        if (cur.targetCatIdx == 0 || cur.targetCatIdx == 8) {
            if (cur.cutEyes && hit(cur.leftEyeX, cur.leftEyeY, cur.leftEyeRx, cur.leftEyeRy)) fi.dragTarget = 1;
            else if (cur.cutEyes && hit(cur.rightEyeX, cur.rightEyeY, cur.rightEyeRx, cur.rightEyeRy)) fi.dragTarget = 2;
            else if (cur.doInpaint && hit(cur.noseX, cur.noseY, cur.noseRx, cur.noseRy)) fi.dragTarget = 3;
            else if (cur.doInpaint && hit(cur.mouthX, cur.mouthY, cur.mouthRx, cur.mouthRy)) fi.dragTarget = 4;
        }
        fi.dragging = true;
        fi.dragLastX = mx;
        fi.dragLastY = my;
    } else if (ev.type == SDL_MOUSEBUTTONUP && ev.button.button == SDL_BUTTON_LEFT && fi.dragging) {
        fi.dragging = false;
        rebuildFlatImportPreviewTex(ren, cur, false);
    } else if (ev.type == SDL_MOUSEMOTION && fi.dragging) {
        int mx = 0, my = 0;
        toCanvas(ev.motion.x, ev.motion.y, mx, my);
        int dx = mx - fi.dragLastX, dy = my - fi.dragLastY;
        fi.dragLastX = mx; fi.dragLastY = my;
        if (fi.dragTarget == 0)      { cur.offsetX += dx; cur.offsetY += dy; }
        else if (fi.dragTarget == 1) { cur.leftEyeX += dx; cur.leftEyeY += dy; }
        else if (fi.dragTarget == 2) { cur.rightEyeX += dx; cur.rightEyeY += dy; }
        else if (fi.dragTarget == 3) { cur.noseX += dx; cur.noseY += dy; }
        else if (fi.dragTarget == 4) { cur.mouthX += dx; cur.mouthY += dy; }
        rebuildFlatImportPreviewTex(ren, cur, true);
    } else if (ev.type == SDL_MOUSEWHEEL) {
        int d = (ev.wheel.y > 0) ? 1 : -1;
        if (fi.dragTarget == 0)      cur.scale = std::max(0.15f, std::min(3.0f, cur.scale + d * 0.04f));
        else if (fi.dragTarget == 1) { cur.leftEyeRx = std::max(16, cur.leftEyeRx + d * 3); cur.leftEyeRy = std::max(12, cur.leftEyeRy + d * 2); }
        else if (fi.dragTarget == 2) { cur.rightEyeRx = std::max(16, cur.rightEyeRx + d * 3); cur.rightEyeRy = std::max(12, cur.rightEyeRy + d * 2); }
        else if (fi.dragTarget == 3) { cur.noseRx = std::max(14, cur.noseRx + d * 3); cur.noseRy = std::max(18, cur.noseRy + d * 3); }
        else if (fi.dragTarget == 4) { cur.mouthRx = std::max(20, cur.mouthRx + d * 4); cur.mouthRy = std::max(14, cur.mouthRy + d * 3); }
        rebuildFlatImportPreviewTex(ren, cur, false);
    }
}

// ------------------------------------------------------------------------------
// 46. Handle keyboard input inside Live Camera & USB Microscope Viewfinder
// ------------------------------------------------------------------------------
static bool handleCameraViewfinderKeys(SDL_Window* window, SDL_Keycode sym, AppRuntimeState& state)
{
    if (!state.cameraCapture.open) return false;
    if (sym == SDLK_ESCAPE) { closeCameraViewfinder(state.cameraCapture); return true; }
    if (sym == SDLK_m) { state.cameraCapture.eyeScopeMode = !state.cameraCapture.eyeScopeMode; return true; }
    if (sym == SDLK_r) { state.cameraCapture.rotate90 = !state.cameraCapture.rotate90; return true; }
    if (sym == SDLK_TAB) {
        state.cameraCapture.useCsiMode = !state.cameraCapture.useCsiMode;
        if (!state.cameraCapture.useCsiMode && !g_usb_cam.isOpened()) openCameraViewfinder(state.cameraCapture);
        return true;
    }
    if (sym == SDLK_SPACE || sym == SDLK_RETURN || sym == SDLK_KP_ENTER || sym == SDLK_e) {
        bool isEye = (sym == SDLK_e || state.cameraCapture.eyeScopeMode);
        std::string saved = captureCameraPhotoToImport(state.cameraCapture, "import", isEye ? "eye" : "cam");
        closeCameraViewfinder(state.cameraCapture);
        if (saved.empty()) { showOsd("CAMERA CAPTURE FAILED!", 2400, false); return true; }
        openFlatImportStudio(SDL_GetRenderer(window), state.flatImport, "import");
        for (size_t i = 0; i < state.flatImport.items.size(); ++i) {
            if (state.flatImport.items.at(i).filePath == saved) {
                state.flatImport.selectedIdx = static_cast<int>(i);
                if (isEye) state.flatImport.items.at(i).targetCatIdx = 10;
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
// 47. Handle keyboard controls inside Calibration & Import Studio
// ------------------------------------------------------------------------------
static bool handleFlatImportKeys(SDL_Window* window, SDL_Keycode sym, bool shiftDown, FaceAppConfig& cfg, AppRuntimeState& state)
{
    if (!state.flatImport.open) return false;
    SDL_Renderer* ren = SDL_GetRenderer(window);
    int count = static_cast<int>(state.flatImport.items.size());
    std::array<char, 96> buf{};

    if (sym == SDLK_ESCAPE) {
        destroyFlatImportItems(state.flatImport);
        state.flatImport.open = false;
        return true;
    }
    if (sym == SDLK_c) { state.flatImport.open = false; openCameraViewfinder(state.cameraCapture); return true; }
    if (count == 0) return true;

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
    if (sym == SDLK_w) { cur.removeWhite = !cur.removeWhite; rebuildFlatImportPreviewTex(ren, cur, false); return true; }
    if (sym == SDLK_b) {
        s_bgTolerance = shiftDown ? std::max(8, s_bgTolerance - 8) : std::min(140, s_bgTolerance + 8);
        cur.removeWhite = true;
        rebuildFlatImportPreviewTex(ren, cur, false);
        return true;
    }
    if (sym == SDLK_v) { cur.cutEyes = !cur.cutEyes; rebuildFlatImportPreviewTex(ren, cur, false); return true; }
    if (sym == SDLK_n) { cur.doInpaint = !cur.doInpaint; rebuildFlatImportPreviewTex(ren, cur, false); return true; }
    if (sym == SDLK_z) { cur.angle -= 1.5f; rebuildFlatImportPreviewTex(ren, cur, false); return true; }
    if (sym == SDLK_x) { cur.angle += 1.5f; rebuildFlatImportPreviewTex(ren, cur, false); return true; }
    if (sym == SDLK_EQUALS || sym == SDLK_KP_PLUS)  { cur.scale = std::min(3.0f, cur.scale + 0.04f); rebuildFlatImportPreviewTex(ren, cur, false); return true; }
    if (sym == SDLK_MINUS  || sym == SDLK_KP_MINUS) { cur.scale = std::max(0.15f, cur.scale - 0.04f); rebuildFlatImportPreviewTex(ren, cur, false); return true; }
    if (sym == SDLK_SPACE) { cur.selected = !cur.selected; return true; }
    if (sym == SDLK_a) {
        bool anyOff = false;
        for (const auto& it : state.flatImport.items) if (!it.selected) anyOff = true;
        for (auto& it : state.flatImport.items) it.selected = anyOff;
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
// 48. Free all cached 210x210 live eye preview textures in Eye Browser
// ------------------------------------------------------------------------------
static void destroyEyeBrowserEntries(EyeBrowserState& eb)
{
    for (auto& e : eb.entries) {
        if (e.thumbTex) { SDL_DestroyTexture(e.thumbTex); e.thumbTex = nullptr; }
    }
    eb.entries.clear();
}

// ------------------------------------------------------------------------------
// 49. Reload dynamic eye bank and render 210x210 thumbnails for Eye Browser
// ------------------------------------------------------------------------------
static void refreshEyeBrowserEntries(SDL_Renderer* ren, EyeBrowserState& eb, const std::string& assetsDir)
{
    destroyEyeBrowserEntries(eb);
    reloadDynamicEyeBank(assetsDir);

    EyeBrowserEntry autoItem{};
    autoItem.name = "AUTO (MACRO / DMX)";
    autoItem.bankIdx = -1;
    eb.entries.push_back(autoItem);

    int total = getEyeBankCount();
    std::vector<uint32_t> argb(210 * 210, 0);
    for (int i = 0; i < total; ++i) {
        EyeBrowserEntry item{};
        item.name = getEyeBankName(i);
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
    if (eb.selectedIdx >= static_cast<int>(eb.entries.size())) eb.selectedIdx = 0;
}

// ------------------------------------------------------------------------------
// 50. Handle keyboard navigation and selection inside Adafruit Eye Browser (F8)
// ------------------------------------------------------------------------------
static bool handleEyeBrowserKeys(SDL_Keycode sym, AppRuntimeState& state)
{
    if (!state.eyeBrowser.open) return false;
    int count = static_cast<int>(state.eyeBrowser.entries.size());
    std::array<char, 96> buf{};

    if (sym == SDLK_ESCAPE || sym == SDLK_F8) { state.eyeBrowser.open = false; return true; }
    if (sym == SDLK_LEFT && count > 0) { state.eyeBrowser.selectedIdx = (state.eyeBrowser.selectedIdx - 1 + count) % count; return true; }
    if (sym == SDLK_RIGHT && count > 0) { state.eyeBrowser.selectedIdx = (state.eyeBrowser.selectedIdx + 1) % count; return true; }
    if (sym == SDLK_0 || sym == SDLK_BACKSPACE) {
        selectEyeBankIndex(-1); state.eyeBrowser.open = false;
        showOsd("EYE BANK: AUTO MODE", 2200, false); return true;
    }
    if ((sym == SDLK_RETURN || sym == SDLK_KP_ENTER) && count > 0) {
        const auto& sel = state.eyeBrowser.entries.at(state.eyeBrowser.selectedIdx);
        selectEyeBankIndex(sel.bankIdx); state.eyeBrowser.open = false;
        std::snprintf(buf.data(), buf.size(), "EYE SELECTED: %s [S=SAVE]", sel.name.c_str());
        showOsd(buf.data(), 2400, false); return true;
    }
    return true;
}
// ------------------------------------------------------------------------------
// 51. Handle SIGINT / SIGTERM for clean shutdown
// ------------------------------------------------------------------------------
void sig_handler(int) { g_running = false; }

// ------------------------------------------------------------------------------
// 52. Initialize non-blocking Art-Net (6454) or sACN multicast (5568) socket
// ------------------------------------------------------------------------------
int init_dmx_socket(const std::string& proto, uint16_t universe)
{
    int port = (proto == "sacn") ? SACN_PORT : ARTNET_PORT;
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) return -1;
    int opt = 1; setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    sockaddr_in addr{}; addr.sin_family = AF_INET; addr.sin_port = htons(port); addr.sin_addr.s_addr = INADDR_ANY;
    if (bind(sock, (sockaddr*)&addr, sizeof(addr)) < 0) { close(sock); return -1; }
    if (proto == "sacn") {
        struct ifaddrs* ifaddr = nullptr;
        if (getifaddrs(&ifaddr) == 0) {
            std::string ip = "239.255." + std::to_string((universe >> 8) & 0xFF) + "." + std::to_string(universe & 0xFF);
            for (auto* ifa = ifaddr; ifa; ifa = ifa->ifa_next) {
                if (!ifa->ifa_addr || ifa->ifa_addr->sa_family != AF_INET) continue;
                ip_mreq m{}; inet_pton(AF_INET, ip.c_str(), &m.imr_multiaddr);
                m.imr_interface = ((sockaddr_in*)ifa->ifa_addr)->sin_addr;
                setsockopt(sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, &m, sizeof(m));
            }
            freeifaddrs(ifaddr);
        }
    }
    timeval tv{0, 100000}; setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    return sock;
}

// ------------------------------------------------------------------------------
// 53. Parse incoming Art-Net OpDmx or sACN E1.31 packet into 512-byte buffer
// ------------------------------------------------------------------------------
static bool parseDmxPacket(const std::string& proto, uint16_t uni, const std::array<uint8_t, 1500>& b, ssize_t len, std::array<uint8_t, 512>& out)
{
    if (proto == "artnet") {
        if (len < 20 || std::memcmp(b.data(), "Art-Net\0", 8) != 0) return false;
        if ((b.at(8) | (b.at(9) << 8)) != 0x5000 || (b.at(14) | ((b.at(15) & 0x7F) << 8)) != uni) return false;
        std::memcpy(out.data(), b.data() + 18, std::min<uint16_t>(512, (b.at(16) << 8) | b.at(17)));
        return true;
    }
    if (len < 127 || std::memcmp(b.data() + 4, "ASC-E1.17\0\0\0", 12) != 0 || b.at(125) != 0 || ((b.at(113) << 8) | b.at(114)) != uni) return false;
    int dmxLen = std::min(512, static_cast<int>((b.at(123) << 8) | b.at(124)) - 1);
    if (dmxLen <= 0 || len < 126 + dmxLen) return false;
    std::memcpy(out.data(), b.data() + 126, dmxLen);
    return true;
}
// ------------------------------------------------------------------------------
// 54. Sample average skin or eyeshadow RGB from face/makeup PNG for eyelid tint
// ------------------------------------------------------------------------------
static void sampleLayerColorForEyelids(const fs::path& fullPath, bool isMakeup)
{
    SDL_Surface* raw = IMG_Load(fullPath.string().c_str());
    if (!raw) return;
    SDL_Surface* surf = SDL_ConvertSurfaceFormat(raw, SDL_PIXELFORMAT_ARGB8888, 0);
    SDL_FreeSurface(raw);
    if (!surf) return;
    uint64_t sumR = 0, sumG = 0, sumB = 0, count = 0;
    int yStart = isMakeup ? 295 : 240, yEnd = isMakeup ? 385 : 420;
    const uint32_t* px = static_cast<const uint32_t*>(surf->pixels);
    int pitchPx = surf->pitch / 4;

    for (int y = yStart; y < yEnd && y < surf->h; y += 2) {
        for (int x = 120; x < 480 && x < surf->w; x += 2) {
            if (isMakeup && !((x >= 135 && x <= 245) || (x >= 355 && x <= 465))) continue;
            uint32_t c = px[y * pitchPx + x];
            if (((c >> 24) & 0xFF) < 140) continue;
            sumR += (c >> 16) & 0xFF; sumG += (c >> 8) & 0xFF; sumB += c & 0xFF;
            ++count;
        }
    }
    SDL_FreeSurface(surf);
    if (count > 0) {
        uint8_t r = static_cast<uint8_t>(sumR / count), g = static_cast<uint8_t>(sumG / count), b = static_cast<uint8_t>(sumB / count);
        if (isMakeup) setEyelidMakeupColor(true, r, g, b);
        else          setEyelidSkinColor(r, g, b);
    } else if (isMakeup) setEyelidMakeupColor(false, 0, 0, 0);
}

// ------------------------------------------------------------------------------
// 55. Load numbered paper-doll PNG texture into layer slot when index changes
// ------------------------------------------------------------------------------
static void updatePaperDollLayer(SDL_Renderer* renderer, const std::string& assetsDir, uint8_t requestedIndex, PaperDollLayer& layer)
{
    if (requestedIndex == layer.loadedIndex) return;
    if (layer.texture) { SDL_DestroyTexture(layer.texture); layer.texture = nullptr; }
    layer.loadedIndex = requestedIndex;

    bool isFace = (std::strcmp(layer.folder, "face") == 0), isMakeup = (std::strcmp(layer.folder, "makeup") == 0);
    if (requestedIndex == 0) {
        if (isFace)   { setEyelidFaceActive(true); setEyelidSkinColor(212, 164, 138); }
        if (isMakeup) { setEyelidMakeupColor(false, 0, 0, 0); }
        return;
    }
    std::array<char, 16> fileName{};
    std::snprintf(fileName.data(), fileName.size(), "%03u.png", requestedIndex);
    fs::path fullPath = fs::path(assetsDir) / layer.folder / fileName.data();
    if (!fs::exists(fullPath)) return;

    if (isFace || isMakeup) { setEyelidFaceActive(true); sampleLayerColorForEyelids(fullPath, isMakeup); }
    layer.texture = IMG_LoadTexture(renderer, fullPath.string().c_str());
    if (layer.texture) SDL_SetTextureBlendMode(layer.texture, SDL_BLENDMODE_BLEND);
}
// ------------------------------------------------------------------------------
// 56. Free all 19 paper-doll and dual-profile SDL textures on shutdown
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
        if (l->texture) { SDL_DestroyTexture(l->texture); l->texture = nullptr; }
    }
}

// ------------------------------------------------------------------------------
// 57. Load PNG texture with alpha blending if file exists on disk
// ------------------------------------------------------------------------------
static SDL_Texture* loadPngIfExists(SDL_Renderer* ren, const fs::path& p)
{
    if (!fs::exists(p)) return nullptr;
    SDL_Texture* tex = IMG_LoadTexture(ren, p.string().c_str());
    if (tex) SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND);
    return tex;
}

// ------------------------------------------------------------------------------
// 58. Destroy all 5-layer parallax backdrop textures
// ------------------------------------------------------------------------------
void destroyParallaxBackdrop(ParallaxBackdrop& bg)
{
    SDL_Texture** all[] = {&bg.sky, &bg.clouds, &bg.farLayer, &bg.midLayer, &bg.frontLayer, &bg.single};
    for (auto** t : all) if (*t) { SDL_DestroyTexture(*t); *t = nullptr; }
    bg.loadedIndex = 0;
}

// ------------------------------------------------------------------------------
// 59. Load 5-layer parallax backdrop (_sky.._front) or single static backdrop
// ------------------------------------------------------------------------------
static void updateParallaxBackdrop(SDL_Renderer* ren, const std::string& assetsDir, uint8_t reqIdx, ParallaxBackdrop& bg)
{
    if (reqIdx == bg.loadedIndex) return;
    destroyParallaxBackdrop(bg);
    bg.loadedIndex = reqIdx;
    if (reqIdx == 0) return;

    std::array<char, 32> name{};
    std::snprintf(name.data(), name.size(), "%03u_sky.png", reqIdx);
    bg.sky = loadPngIfExists(ren, fs::path(assetsDir) / "bg" / name.data());
    if (bg.sky) {
        std::snprintf(name.data(), name.size(), "%03u_clouds.png", reqIdx); bg.clouds = loadPngIfExists(ren, fs::path(assetsDir) / "bg" / name.data());
        std::snprintf(name.data(), name.size(), "%03u_far.png", reqIdx);    bg.farLayer = loadPngIfExists(ren, fs::path(assetsDir) / "bg" / name.data());
        std::snprintf(name.data(), name.size(), "%03u_mid.png", reqIdx);    bg.midLayer = loadPngIfExists(ren, fs::path(assetsDir) / "bg" / name.data());
        std::snprintf(name.data(), name.size(), "%03u_front.png", reqIdx);  bg.frontLayer = loadPngIfExists(ren, fs::path(assetsDir) / "bg" / name.data());
    } else {
        std::snprintf(name.data(), name.size(), "%03u.png", reqIdx);
        bg.single = loadPngIfExists(ren, fs::path(assetsDir) / "bg" / name.data());
    }
}
// ------------------------------------------------------------------------------
// 60. Load all 17 FX sprites + auto-generate OpenCV GaussianBlur depth textures
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
        if (fs::exists(p)) {
            cv::Mat img = cv::imread(p.string(), cv::IMREAD_UNCHANGED);
            if (!img.empty()) {
                cv::Mat bgra;
                if (img.channels() == 4) bgra = img;
                else if (img.channels() == 3) cv::cvtColor(img, bgra, cv::COLOR_BGR2BGRA);
                else cv::cvtColor(img, bgra, cv::COLOR_GRAY2BGRA);
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
    bank.fog2 = loadPngIfExists(ren, fs::path(assetsDir) / "fx" / "fog_2.png");
}

// ------------------------------------------------------------------------------
// 61. Free all sharp & blurred FX sprite textures on shutdown or theme reload
// ------------------------------------------------------------------------------
void destroyFxSpriteBank(FxSpriteBank& bank)
{
    for (auto*& t : bank.sprites)     if (t) { SDL_DestroyTexture(t); t = nullptr; }
    for (auto*& t : bank.blurSprites) if (t) { SDL_DestroyTexture(t); t = nullptr; }
    if (bank.fog2) { SDL_DestroyTexture(bank.fog2); bank.fog2 = nullptr; }
}


// ------------------------------------------------------------------------------
// 62. Render split left/right eyebrow PNG halves with tilt, mood and scale
// ------------------------------------------------------------------------------
static void renderDynamicEyebrowsSplit(SDL_Renderer* renderer, const PaperDollLayer& browLayer, uint8_t rawLeft, uint8_t rawRight, bool showLeft, bool showRight, int extraSplit)
{
    if (!browLayer.texture) return;
    int baseY = browLayer.offsetY + GLOBAL_FACE_Y_OFFSET;
    float moodL = (static_cast<float>(rawLeft) - 128.0f) / 128.0f;
    float moodR = (static_cast<float>(rawRight) - 128.0f) / 128.0f;
    float sc = browLayer.scale;
    int halfW = static_cast<int>(std::round((BASE_CANVAS_W / 2) * sc));
    int fullH = static_cast<int>(std::round(BASE_CANVAS_H * sc));
    int dy = (BASE_CANVAS_H - fullH) / 2;

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

// ------------------------------------------------------------------------------
// 63. Toggle Wayland desktop fullscreen and cursor visibility
// ------------------------------------------------------------------------------
static void toggleWindowFullscreen(SDL_Window* window, bool& isFullscreen)
{
    isFullscreen = !isFullscreen;
    if (isFullscreen) { SDL_SetWindowResizable(window, SDL_FALSE); SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN_DESKTOP); }
    else { SDL_SetWindowFullscreen(window, 0); SDL_SetWindowResizable(window, SDL_TRUE); SDL_SetWindowSize(window, BASE_CANVAS_W, BASE_CANVAS_H); }
    SDL_ShowCursor(isFullscreen ? SDL_DISABLE : SDL_ENABLE);
}

// ------------------------------------------------------------------------------
// 64. Step asset index forward or backward across 0..maxCount with wrap-around
// ------------------------------------------------------------------------------
static uint8_t stepAssetIndex(uint8_t current, uint8_t maxCount, bool backward)
{
    if (backward) return (current == 0) ? maxCount : static_cast<uint8_t>(current - 1);
    return (current >= maxCount) ? 0 : static_cast<uint8_t>(current + 1);
}

// ------------------------------------------------------------------------------
// 65. Apply pending macro switch at midpoint of crossfade transition
// ------------------------------------------------------------------------------
static void executePendingMacroSwitch(const FaceAppConfig& cfg, AppRuntimeState& state)
{
    int targetApply = state.pendingApplySlot;
    state.appliedMacroSlot = targetApply;
    if (state.pendingMacroSlot != -99) { state.manualMacroSlot = state.pendingMacroSlot; state.pendingMacroSlot = -99; }
    if (state.manualMacroSlot < 0)
    {
        if (targetApply <= 0) {
            state.manualTestAnim = 0; state.isEditingOffsets = false;
            state.manualHideMouth = state.manualHideNose = state.manualHideBrows = false;
        }
        applyMacroComboToState(cfg, state, targetApply, false);
        applyMaskOffsetProfile(cfg, state, state.faceLayer.testIndex);
    }
    else
    {
        applyMacroComboToState(cfg, state, targetApply, true);
        applyMaskOffsetProfile(cfg, state, state.faceLayer.testIndex);
    }
}

// ------------------------------------------------------------------------------
// 66. Initiate smooth fade-to-black crossfade transition into target macro slot
// ------------------------------------------------------------------------------
static void triggerMacroTransition(AppRuntimeState& state, const FaceAppConfig& cfg, int newManualSlot, int newApplySlot, const char* osdMsg)
{
    if (osdMsg && *osdMsg != '\0') showOsd(osdMsg, 2400, false);
    state.pendingMacroSlot = newManualSlot;
    state.pendingApplySlot  = newApplySlot;
    if (cfg.fade_ms > 0) { state.fadePhase = 1; state.fadeStartMs = millis(); }
    else executePendingMacroSwitch(cfg, state);
}
// ------------------------------------------------------------------------------
// 67. Draw solid dark oval eye-socket shadow backing behind eyeballs
// ------------------------------------------------------------------------------
static void drawOvalShadow(SDL_Renderer* ren, int cx, int cy, int rx, int ry, uint8_t alpha)
{
    SDL_SetRenderDrawColor(ren, 0, 0, 0, alpha);
    for (int dy = -ry; dy <= ry; ++dy) {
        float normY = static_cast<float>(dy) / static_cast<float>(ry);
        int   halfW = static_cast<int>(rx * std::sqrt(std::max(0.0f, 1.0f - normY * normY)));
        SDL_RenderDrawLine(ren, cx - halfW, cy + dy, cx + halfW, cy + dy);
    }
}

// ------------------------------------------------------------------------------
// 68. Render seamlessly mirrored horizontal scrolling cloud layer
// ------------------------------------------------------------------------------
static void renderDriftingCloudLayer(SDL_Renderer* ren, SDL_Texture* tex, float scrollPx, int outW, int outH)
{
    if (!tex) return;
    int texW = 0, texH = 0;
    SDL_QueryTexture(tex, nullptr, nullptr, &texW, &texH);
    SDL_Rect src = (texW > 8 && texH > 4) ? SDL_Rect{2, 0, texW - 4, texH - 2} : SDL_Rect{0, 0, texW, texH};

    int periodW = outW * 2;
    float wrapped = std::fmod(scrollPx, static_cast<float>(periodW));
    if (wrapped < 0.0f) wrapped += static_cast<float>(periodW);
    int x0 = -static_cast<int>(std::floor(wrapped));

    for (int i = 0; i < 3; ++i) {
        SDL_Rect dst{x0 + i * outW, 0, outW + 1, outH + 2};
        SDL_RendererFlip flip = ((i % 2) == 1) ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE;
        SDL_RenderCopyEx(ren, tex, &src, &dst, 0.0, nullptr, flip);
    }
}

// ------------------------------------------------------------------------------
// 69. Render 5-layer depth backdrop + independent stars, comets & aurora FX
// ------------------------------------------------------------------------------
static void renderDepthBackdropLayers(SDL_Renderer* ren, const ParallaxBackdrop& bg, uint32_t fxMask, uint32_t nowMs, int outW, int outH)
{
    bool showStars  = ((fxMask & (1u << 15)) != 0);
    bool showComet  = ((fxMask & (1u << 16)) != 0);
    bool showAurora = ((fxMask & (1u << 17)) != 0);

    if (bg.loadedIndex == 0 && !showStars && !showComet && !showAurora) return;

    auto copyCroppedFull = [&](SDL_Texture* tex) {
        if (!tex) return;
        int tw = 0, th = 0;
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
            renderDriftingCloudLayer(ren, bg.clouds, t * 10.0f, outW, outH);
        }
    }

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
                float nx = static_cast<float>(x) / static_cast<float>(std::max(1, outW));
                float fold = std::sin(nx * 11.0f + t * 0.85f) * 26.0f + std::cos(nx * 23.0f - t * 1.35f) * 12.0f;
                float shimmer = std::pow(0.5f + 0.5f * std::sin(nx * 47.0f - t * 3.8f + std::sin(nx * 9.0f + t)), 3.0f);
                float waveBand = 0.5f + 0.5f * std::sin(nx * 6.5f + t * 0.55f);
                int baseY = static_cast<int>(outH * 0.25f + fold);
                int rayH = static_cast<int>(outH * (0.12f + 0.15f * shimmer * waveBand));
                uint8_t alphaLow = static_cast<uint8_t>(22.0f + shimmer * 90.0f);
                uint8_t rLow = static_cast<uint8_t>(30.0f + 155.0f * std::pow(0.5f + 0.5f * std::sin(nx * 14.0f - t * 1.1f), 2.0f));
                uint8_t gLow = static_cast<uint8_t>(185.0f + 70.0f * waveBand);
                uint8_t bLow = static_cast<uint8_t>(135.0f + 115.0f * (1.0f - waveBand));
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
                float p = shootU / 0.75f;
                float headX = outW * (0.18f + p * 0.66f), headY = outH * (0.05f + p * 0.20f);
                float tailX = headX - outW * 0.15f, tailY = headY - outH * 0.045f;
                for (int s = 0; s < 16; ++s) {
                    float f0 = static_cast<float>(s) / 16.0f, f1 = static_cast<float>(s + 1) / 16.0f;
                    SDL_SetRenderDrawColor(ren, 235, 248, 255, static_cast<uint8_t>((1.0f - f0) * 240.0f));
                    SDL_RenderDrawLine(ren, static_cast<int>(headX + (tailX - headX) * f0), static_cast<int>(headY + (tailY - headY) * f0),
                                       static_cast<int>(headX + (tailX - headX) * f1), static_cast<int>(headY + (tailY - headY) * f1));
                }
            }
            float cometU = std::fmod(t * 0.45f + 0.8f, 6.0f);
            if (cometU < 2.6f) {
                float p = cometU / 2.6f;
                float headX = outW * (0.85f - p * 0.72f), headY = outH * (0.09f + p * 0.19f);
                float tailX = headX + outW * 0.22f, tailY = headY - outH * 0.06f;
                for (int s = 0; s < 22; ++s) {
                    float f0 = static_cast<float>(s) / 22.0f, f1 = static_cast<float>(s + 1) / 22.0f;
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
// 70. Clear active critter eye-tracking target
// ------------------------------------------------------------------------------
static inline void clearGazeTarget(void) { g_critter_gaze_active.store(false, std::memory_order_relaxed); }

// ------------------------------------------------------------------------------
// 71. Update critter eye-tracking target coordinates (with inverted Y axis)
// ------------------------------------------------------------------------------
static inline void setGazeTarget(float x, float y)
{
    int gx = std::max(0, std::min(255, static_cast<int>(std::round((x / static_cast<float>(BASE_CANVAS_W)) * 255.0f))));
    int gy = std::max(0, std::min(255, static_cast<int>(std::round(((static_cast<float>(BASE_CANVAS_H) - y) / static_cast<float>(BASE_CANVAS_H)) * 255.0f))));
    g_critter_gaze_x.store(static_cast<uint8_t>(gx), std::memory_order_relaxed);
    g_critter_gaze_y.store(static_cast<uint8_t>(gy), std::memory_order_relaxed);
    g_critter_gaze_active.store(true, std::memory_order_relaxed);
}

// ------------------------------------------------------------------------------
// 72. Render FX at depthPass: 0=far horizon, 1=mid behind face, 2=front sharp
// ------------------------------------------------------------------------------
static void renderEffectsAtDepth(SDL_Renderer* ren, const FxSpriteBank& bank, uint32_t fxMask, int depthPass, uint32_t nowMs, bool& ioTrackedGaze)
{
    static uint32_t s_mouseEnableMs = 0;
    static bool     s_mouseWasOn    = false;
    bool mouseOn = ((fxMask & (1u << 14)) != 0);
    if (depthPass == 0) {
        if (mouseOn && !s_mouseWasOn) s_mouseEnableMs = nowMs;
        s_mouseWasOn = mouseOn;
    }

    if (fxMask == 0) return;

    float t = static_cast<float>(nowMs) * 0.001f;
    auto setGaze = [&](float x, float y) { if (!ioTrackedGaze) { setGazeTarget(x, y); ioTrackedGaze = true; } };
    auto getTex = [&](int idx, bool farBlur) -> SDL_Texture* {
        if (farBlur && bank.blurSprites.at(idx)) return bank.blurSprites.at(idx);
        return bank.sprites.at(idx);
    };

    // Cobweb (bit 8) -> full-height corner-to-corner web behind face in depthPass 1
    if (depthPass == 1 && (fxMask & (1u << 8)) && bank.sprites.at(8)) {
        SDL_Rect web{0, 0, BASE_CANVAS_W, BASE_CANVAS_H};
        SDL_RenderCopy(ren, bank.sprites.at(8), nullptr, &web);
    }

    // Ghost (bit 10) -> alternates between mid (behind face) & front pass
    if ((fxMask & (1u << 10)) && bank.sprites.at(10)) {
        int pass = static_cast<int>(std::floor(t / 14.0f));
        int targetPass = ((pass * 3 + 1) % 2 == 0) ? 1 : 2;
        if (depthPass == targetPass) {
            bool isMid = (depthPass == 1);
            float sc = isMid ? 0.65f : 1.0f;
            float ghX = 300.0f + std::sin(t * 0.65f) * 210.0f, ghY = (isMid ? 330.0f : 390.0f) + std::sin(t * 1.30f) * 75.0f;
            setGaze(ghX, ghY);
            SDL_Texture* tex = getTex(10, isMid);
            SDL_SetTextureAlphaMod(tex, isMid ? 130 : 175);
            int w = static_cast<int>(220 * sc), h = static_cast<int>(280 * sc);
            SDL_Rect ghDst{static_cast<int>(ghX) - w / 2, static_cast<int>(ghY) - h / 2, w, h};
            SDL_RenderCopyEx(ren, tex, nullptr, &ghDst, std::sin(t * 1.1f) * 6.0, nullptr,
                             (std::cos(t * 0.65f) >= 0.0f) ? SDL_FLIP_NONE : SDL_FLIP_HORIZONTAL);
            SDL_SetTextureAlphaMod(tex, 255);
        }
    }

    // Hanging Snake (bit 11) -> front pass
    if (depthPass == 2 && (fxMask & (1u << 11)) && bank.sprites.at(11)) {
        const float period = 11.0f;
        int passIdx = static_cast<int>(std::floor(t / period));
        float u = std::fmod(t, period);
        if (u < 6.2f) {
            float baseX = 110.0f + static_cast<float>((passIdx * 173) % 380);
            float dropP = std::sin((u / 6.2f) * 3.1415926f);
            float snkX = baseX + std::sin(t * 3.8f) * 26.0f, snkY = -190.0f + dropP * 320.0f;
            setGaze(snkX, snkY + 135.0f);
            SDL_Rect snkDst{static_cast<int>(snkX) - 70, static_cast<int>(snkY) - 180, 140, 360};
            SDL_RenderCopyEx(ren, bank.sprites.at(11), nullptr, &snkDst, std::sin(t * 3.8f) * 8.0, nullptr,
                             ((passIdx % 2) == 0) ? SDL_FLIP_NONE : SDL_FLIP_HORIZONTAL);
        }
    }

    // Mouse (bit 14) -> randomly picks mid-layer (scaled down + blurred behind face) or front-layer
    if (mouseOn && bank.sprites.at(16)) {
        const float period = 14.0f;
        float mouseT = static_cast<float>(nowMs - s_mouseEnableMs) * 0.001f;
        int passIdx = static_cast<int>(std::floor(mouseT / period));
        int targetPass = ((passIdx * 7 + 3) % 2 == 0) ? 1 : 2;
        float u = std::fmod(mouseT, period);
        if (depthPass == targetPass && u < 5.4f) {
            bool isMid = (depthPass == 1);
            float sc = isMid ? 0.56f : 1.0f;
            bool leftToRight = (passIdx % 2) == 0;
            float prog = (u < 2.1f) ? ((u / 2.1f) * 0.48f) : ((u < 3.3f) ? 0.48f : (0.48f + ((u - 3.3f) / 2.1f) * 0.52f));
            float cx = leftToRight ? (-75.0f + prog * 750.0f) : (675.0f - prog * 750.0f);
            float scurryBob = (u >= 2.1f && u < 3.3f) ? std::sin(t * 28.0f) * 1.5f : std::fabs(std::sin(t * 34.0f)) * 4.5f;
            float cy = (isMid ? 688.0f : 764.0f) - scurryBob * sc;
            setGaze(cx, cy);
            SDL_Texture* tex = getTex(16, isMid);
            SDL_SetTextureAlphaMod(tex, isMid ? 185 : 255);
            int w = static_cast<int>(140 * sc), h = static_cast<int>(84 * sc);
            SDL_Rect dst{static_cast<int>(cx) - w / 2, static_cast<int>(cy) - h / 2, w, h};
            SDL_RenderCopyEx(ren, tex, nullptr, &dst, std::sin(t * 34.0f) * 3.0, nullptr,
                             leftToRight ? SDL_FLIP_NONE : SDL_FLIP_HORIZONTAL);
            SDL_SetTextureAlphaMod(tex, 255);
        }
    }
    // Critters (bits 0..7): random depth pass per visit (0=far blurred, 1=mid blurred behind face, 2=front sharp)
    for (int i = 0; i < 8; ++i) {
        if (!(fxMask & (1u << i)) || !bank.sprites.at(i)) continue;

        if (i == 5) {
            const float period = 12.0f;
            float u = std::fmod(t + 1.5f, period);
            if (depthPass == 2 && u < 7.5f) {
                float drop = std::sin((u / 7.5f) * 3.1415926f);
                float cx = 515.0f + std::sin(t * 2.2f) * 14.0f, cy = -40.0f + drop * 340.0f;
                setGaze(cx, cy);
                SDL_SetRenderDrawColor(ren, 230, 235, 245, 190);
                SDL_RenderDrawLine(ren, 515, 0, static_cast<int>(cx), static_cast<int>(cy));
                SDL_Rect dst{static_cast<int>(cx) - 44, static_cast<int>(cy) - 44, 88, 88};
                SDL_RenderCopyEx(ren, bank.sprites.at(i), nullptr, &dst, std::sin(t * 2.2f) * 8.0, nullptr, SDL_FLIP_NONE);
            }
            continue;
        }

        if (i == 3) {
            const float period = 10.5f;
            int passIdx = static_cast<int>(std::floor(t / period));
            int targetPass = ((passIdx * 5 + 2) % 3);
            float u = std::fmod(t, period);
            if (depthPass == targetPass && u < 4.2f) {
                bool isBack = (depthPass < 2);
                float sc = (depthPass == 0) ? 0.44f : ((depthPass == 1) ? 0.64f : 1.0f);
                float p = u / 4.2f;
                bool leftToRight = (passIdx % 2) == 0;
                float cx = leftToRight ? (-90.0f + p * 780.0f) : (690.0f - p * 780.0f);
                float cy = (depthPass == 0 ? 115.0f : 155.0f) + std::sin(p * 3.1415926f) * 45.0f * sc + std::sin(t * 9.0f) * 10.0f * sc;
                setGaze(cx, cy);
                SDL_Texture* tex = getTex(i, isBack);
                SDL_SetTextureAlphaMod(tex, (depthPass == 0) ? 165 : ((depthPass == 1) ? 205 : 255));
                int sz = static_cast<int>(92 * sc);
                SDL_Rect dst{static_cast<int>(cx) - sz / 2, static_cast<int>(cy) - sz / 2, sz, sz};
                SDL_RenderCopyEx(ren, tex, nullptr, &dst, std::sin(t * 9.0f) * 6.0, nullptr, leftToRight ? SDL_FLIP_NONE : SDL_FLIP_HORIZONTAL);
                SDL_SetTextureAlphaMod(tex, 255);
            }
            continue;
        }

        if (i == 1) {
            const float period = 13.0f;
            int passIdx = static_cast<int>(std::floor(t / period));
            float u = std::fmod(t, period);
            const float noseX = 300.0f, noseY = 448.0f;
            bool goingRight = (passIdx % 2) == 0;
            if (depthPass == 2 && u < 7.8f) {
                float cx = noseX, cy = noseY, angleDeg = 0.0f;
                if (u < 2.2f) {
                    float p = u / 2.2f, startX = goingRight ? -70.0f : 670.0f;
                    cx = startX + (noseX - startX) * p + std::sin(p * 12.56f) * 48.0f * (1.0f - p);
                    cy = 180.0f + (noseY - 180.0f) * p + std::cos(p * 9.42f) * 38.0f * (1.0f - p);
                    angleDeg = std::sin(t * 22.0f) * 14.0f;
                } else if (u < 5.6f) {
                    cx = noseX + std::sin(t * 18.0f) * 0.8f; cy = noseY; angleDeg = std::sin(t * 12.0f) * 3.0f;
                } else {
                    float p = (u - 5.6f) / 2.2f, endX = goingRight ? 670.0f : -70.0f;
                    cx = noseX + (endX - noseX) * p + std::sin(p * 12.56f) * 44.0f * p;
                    cy = noseY - p * 260.0f + std::cos(p * 9.42f) * 32.0f * p;
                    angleDeg = std::sin(t * 22.0f) * 14.0f;
                }
                setGaze(cx, cy);
                SDL_Rect dst{static_cast<int>(cx) - 38, static_cast<int>(cy) - 38, 76, 76};
                SDL_RenderCopyEx(ren, bank.sprites.at(i), nullptr, &dst, angleDeg, nullptr, goingRight ? SDL_FLIP_NONE : SDL_FLIP_HORIZONTAL);
            }
            continue;
        }

        const float period = 11.5f + i * 1.4f;
        float localT = t + i * 2.3f;
        int passIdx = static_cast<int>(std::floor(localT / period));
        int targetPass = ((passIdx * 7 + i * 3) % 3);
        float u = std::fmod(localT, period);
        if (depthPass == targetPass && u < 5.6f) {
            bool isBack = (depthPass < 2);
            float sc = (depthPass == 0) ? 0.45f : ((depthPass == 1) ? 0.65f : 1.0f);
            float p = u / 5.6f;
            bool leftToRight = (passIdx % 2) == 0;
            float baseX = leftToRight ? (-80.0f + p * 760.0f) : (680.0f - p * 760.0f);
            float loopEnv = std::sin(p * 3.1415926f);
            float cx = baseX + std::sin(p * 9.42f) * 85.0f * loopEnv;
            float cy = (190.0f + (i % 3) * 65.0f) + std::cos(p * 6.28f) * 70.0f * loopEnv;
            setGaze(cx, cy);
            SDL_Texture* tex = getTex(i, isBack);
            SDL_SetTextureAlphaMod(tex, (depthPass == 0) ? 160 : ((depthPass == 1) ? 205 : 255));
            int sz = static_cast<int>(((i == 7) ? 128 : 88) * sc);
            bool faceLeft = (std::cos(p * 9.42f) * 85.0f + (leftToRight ? 120.0f : -120.0f)) < 0.0f;
            SDL_Rect dst{static_cast<int>(cx) - sz / 2, static_cast<int>(cy) - sz / 2, sz, sz};
            SDL_RenderCopyEx(ren, tex, nullptr, &dst, std::sin(t * 6.0f + i) * 10.0, nullptr, faceLeft ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE);
            SDL_SetTextureAlphaMod(tex, 255);
        }
    }

    // Lightning (bit 13) -> randomly strikes in depthPass 0 (horizon), 1 (behind head) or 2 (front)
    if ((fxMask & (1u << 13)) && bank.sprites.at(14)) {
        uint32_t strikeIdx = nowMs / 3200;
        uint32_t phase     = nowMs % 3200;
        int targetPass     = static_cast<int>((strikeIdx * 5 + 1) % 3);
        if (depthPass == targetPass && phase < 190) {
            bool isBack = (depthPass < 2);
            SDL_Texture* lTex = ((phase < 95) || !bank.sprites.at(15)) ? getTex(14, isBack) : getTex(15, isBack);
            SDL_SetTextureAlphaMod(lTex, (depthPass == 0) ? 135 : ((depthPass == 1) ? 190 : 255));
            int shiftX = ((strikeIdx % 3) == 0) ? 0 : (((strikeIdx % 3) == 1) ? -90 : 95);
            SDL_Rect dst{shiftX, 0, BASE_CANVAS_W, BASE_CANVAS_H};
            SDL_RenderCopy(ren, lTex, nullptr, &dst);
            SDL_SetTextureAlphaMod(lTex, 255);
        }
    }

    // Multi-depth Slanted Rain (bit 18 - Alt+R): pass 1 = distant fine rain behind head, pass 2 = close rain
    if ((fxMask & (1u << 18)) && (depthPass == 1 || depthPass == 2)) {
        bool isMid = (depthPass == 1);
        SDL_SetRenderDrawColor(ren, 175, 215, 255, isMid ? 95 : 175);
        int count = isMid ? 65 : 75;
        float speedBase = isMid ? 420.0f : 690.0f;
        int lenY = isMid ? 13 : 24;
        for (int r = 0; r < count; ++r) {
            int seed = r + (isMid ? 300 : 0);
            float speed = speedBase + (seed % 5) * 65.0f;
            int ry = static_cast<int>(std::fmod(t * speed + seed * 73.0f, static_cast<float>(BASE_CANVAS_H + 60))) - 30;
            int rx = ((seed * 137 + static_cast<int>(ry * 0.22f)) % BASE_CANVAS_W);
            SDL_RenderDrawLine(ren, rx, ry, rx + (isMid ? 3 : 6), ry + lenY);
        }
    }

    // Multi-depth 3D Snowfall (bit 19 - Alt+W): pass 1 = small slow distant flakes behind face, pass 2 = large front flakes
    if ((fxMask & (1u << 19)) && (depthPass == 1 || depthPass == 2)) {
        bool isMid = (depthPass == 1);
        int count = isMid ? 60 : 55;
        for (int f = 0; f < count; ++f) {
            int seed = f + (isMid ? 500 : 0);
            float speed = (isMid ? 34.0f : 68.0f) + (seed % 6) * (isMid ? 9.0f : 18.0f);
            int fy = static_cast<int>(std::fmod(t * speed + seed * 61.0f, static_cast<float>(BASE_CANVAS_H + 30))) - 15;
            int fx = ((seed * 149) % BASE_CANVAS_W) + static_cast<int>(std::sin(t * (1.1f + (seed % 4) * 0.35f) + seed) * (isMid ? 16.0f : 28.0f));
            int rad = isMid ? ((seed % 3 == 0) ? 3 : 2) : ((seed % 4 == 0) ? 5 : 4);
            SDL_SetRenderDrawColor(ren, 245, 250, 255, isMid ? 145 : 235);
            SDL_Rect flk{fx - rad / 2, fy - rad / 2, rad, rad};
            SDL_RenderFillRect(ren, &flk);
        }
    }
}

// ==============================================================================
// Section H. Keyboard Input Routers & Unified Save Helpers
// ==============================================================================

// ------------------------------------------------------------------------------
// 73. Resolve target SDL display index from config or current mouse cursor
// ------------------------------------------------------------------------------
static int resolveTargetDisplayIndex(const FaceAppConfig& cfg)
{
    int numDisplays = SDL_GetNumVideoDisplays();
    if (numDisplays <= 0) return 0;
    if (cfg.display >= 0 && cfg.display < numDisplays) return cfg.display;
    int mx = 0, my = 0; SDL_GetGlobalMouseState(&mx, &my);
    for (int i = 0; i < numDisplays; ++i) { SDL_Rect b; if (SDL_GetDisplayBounds(i, &b) == 0 && mx >= b.x && mx < b.x + b.w && my >= b.y && my < b.y + b.h) return i; }
    return 0;
}

// ------------------------------------------------------------------------------
// 74. Check whether numbered PNG asset exists in assets/<folder>/
// ------------------------------------------------------------------------------
static bool assetFileExists(const std::string& assetsDir, const char* folder, uint8_t idx)
{
    if (idx == 0) return true;
    std::array<char, 16> fileName{};
    std::snprintf(fileName.data(), fileName.size(), "%03u.png", idx);
    return fs::exists(fs::path(assetsDir) / folder / fileName.data());
}

// ------------------------------------------------------------------------------
// 75. Show OSD banner indicating stepped asset index or (EMPTY) status
// ------------------------------------------------------------------------------
static void showAssetStepOsd(const std::string& assetsDir, const char* label, const char* folder, uint8_t idx)
{
    std::array<char, 96> buf{};
    bool exists = assetFileExists(assetsDir, folder, idx);
    if (idx == 0) std::snprintf(buf.data(), buf.size(), "%s [000/255] OFF", label);
    else if (exists) std::snprintf(buf.data(), buf.size(), "%s [%03u/255]", label, idx);
    else std::snprintf(buf.data(), buf.size(), "%s [%03u/255] (EMPTY)", label, idx);
    showOsd(buf.data(), 2400, false);
}

// ------------------------------------------------------------------------------
// 76. Save active macro slot, overrides & offsets to INI and packs/default.zip
// ------------------------------------------------------------------------------
static void performUnifiedSave(SDL_Window* window, FaceAppConfig& cfg, AppRuntimeState& state)
{
    std::array<char, 96> buf{};
    pushUndoSnapshot(cfg.assets_dir, cfg.config_file, "packs");
    int targetSlot = (state.manualMacroSlot >= 0) ? state.manualMacroSlot : state.browseMacroSlot;
    if ((targetSlot / 10) != state.macroBank) targetSlot = resolveBankKeySlot(state.macroBank, targetSlot % 10);
    captureMacroCombo(cfg, state, targetSlot);
    saveIniConfig(window, cfg, state);
    state.browseMacroSlot = state.manualMacroSlot = state.appliedMacroSlot = targetSlot;
    state.isEditingOffsets = false;

    bool okZip = saveCurrentAssetsToZipPack("default", cfg.assets_dir, cfg.config_file, "packs");
    if (okZip) std::snprintf(buf.data(), buf.size(), "SAVED #%03d + DEFAULT.ZIP [ESC/F3=AUTO]", targetSlot);
    else       std::snprintf(buf.data(), buf.size(), "SAVED #%03d [BANK %02d] [ESC/F3=AUTO]", targetSlot, state.macroBank);
    showOsd(buf.data(), 2400, false);
}
// ------------------------------------------------------------------------------
// 77. Handle keyboard controls inside Live Zip Pack Inspector overlay
// ------------------------------------------------------------------------------
static bool handlePackInspectorKeys(SDL_Window* window, SDL_Keycode sym, FaceAppConfig& cfg, AppRuntimeState& state)
{
    if (!state.packBrowser.open || !state.packBrowser.inspectMode) return false;
    int count = static_cast<int>(state.packBrowser.inspectItems.size());
    std::array<char, 96> buf{};

    if (sym == SDLK_ESCAPE || sym == SDLK_LEFT) { state.packBrowser.inspectMode = false; reloadAllTexturesAndState(SDL_GetRenderer(window), cfg, state); return true; }
    if (sym == SDLK_UP && count > 0) { state.packBrowser.inspectIdx = (state.packBrowser.inspectIdx - 1 + count) % count; return true; }
    if (sym == SDLK_DOWN && count > 0) { state.packBrowser.inspectIdx = (state.packBrowser.inspectIdx + 1) % count; return true; }
    if (sym == SDLK_SPACE && count > 0) { auto& item = state.packBrowser.inspectItems.at(state.packBrowser.inspectIdx); item.selected = !item.selected; return true; }
    if (sym == SDLK_a && count > 0) { bool anyUnchecked = false; for (const auto& it : state.packBrowser.inspectItems) if (!it.selected) anyUnchecked = true; for (auto& it : state.packBrowser.inspectItems) it.selected = anyUnchecked; return true; }
    if ((sym == SDLK_RETURN || sym == SDLK_KP_ENTER) && count > 0) {
        std::string pName = state.packBrowser.entries.at(state.packBrowser.selectedIdx).name;
        int checkedCount = 0;
        for (const auto& it : state.packBrowser.inspectItems) if (it.selected) ++checkedCount;
        if (appendCheckedItemsFromZipPack(window, pName, state.packBrowser, cfg, state, "packs")) {
            state.packBrowser.inspectMode = false; state.packBrowser.open = false;
            std::snprintf(buf.data(), buf.size(), "IMPORTED %d ITEMS FROM %s.ZIP", checkedCount, pName.c_str());
            showOsd(buf.data(), 2800, false);
        }
        return true;
    }
    return true;
}

// ------------------------------------------------------------------------------
// 78. Handle keyboard controls inside Thumbnail Pack Browser (F6 / X)
// ------------------------------------------------------------------------------
static bool handlePackBrowserKeys(SDL_Window* window, SDL_Keycode sym, bool ctrlDown, FaceAppConfig& cfg, AppRuntimeState& state)
{
    if (!state.packBrowser.open) return false;
    if (state.packBrowser.inspectMode) return handlePackInspectorKeys(window, sym, cfg, state);

    std::array<char, 96> buf{};
    int count = static_cast<int>(state.packBrowser.entries.size());
    if (sym == SDLK_ESCAPE || sym == SDLK_F6 || sym == SDLK_x) { if (ctrlDown && sym == SDLK_x) return false; state.packBrowser.open = false; state.packBrowser.wipeConfirm = false; return true; }
    if (sym == SDLK_LEFT && count > 0) { state.packBrowser.selectedIdx = (state.packBrowser.selectedIdx - 1 + count) % count; state.packBrowser.wipeConfirm = false; return true; }
    if (sym == SDLK_RIGHT && count > 0) { state.packBrowser.selectedIdx = (state.packBrowser.selectedIdx + 1) % count; state.packBrowser.wipeConfirm = false; return true; }
    if ((sym == SDLK_i || sym == SDLK_DOWN) && count > 0) { std::string pName = state.packBrowser.entries.at(state.packBrowser.selectedIdx).name; if (openZipPackInspector(pName, state.packBrowser, "packs")) reloadDynamicEyeBank("packs/.inspect_tmp/assets"); return true; }
    if (sym == SDLK_m) { state.packBrowser.open = false; openFlatImportStudio(SDL_GetRenderer(window), state.flatImport, "import"); return true; }
    if ((sym == SDLK_RETURN || sym == SDLK_KP_ENTER) && count > 0) { std::string pName = state.packBrowser.entries.at(state.packBrowser.selectedIdx).name; if (appendZipPackToActive(window, pName, cfg, state, "packs")) { refreshPackBrowserEntries(SDL_GetRenderer(window), state.packBrowser, "packs"); std::snprintf(buf.data(), buf.size(), "APPENDED %s.ZIP -> DEFAULT.ZIP", pName.c_str()); showOsd(buf.data(), 2800, false); } state.packBrowser.wipeConfirm = false; return true; }
    if (sym == SDLK_r && count > 0) { std::string pName = state.packBrowser.entries.at(state.packBrowser.selectedIdx).name; if (replaceActiveWithZipPack(window, pName, cfg, state, "packs")) { refreshPackBrowserEntries(SDL_GetRenderer(window), state.packBrowser, "packs"); std::snprintf(buf.data(), buf.size(), "LOADED %s.ZIP AS DEFAULT", pName.c_str()); showOsd(buf.data(), 2800, false); } state.packBrowser.wipeConfirm = false; return true; }
    if (sym == SDLK_DELETE) { if (!state.packBrowser.wipeConfirm) { state.packBrowser.wipeConfirm = true; return true; } wipeAllAssetsAndMacros(window, cfg, state, "packs"); refreshPackBrowserEntries(SDL_GetRenderer(window), state.packBrowser, "packs"); state.packBrowser.wipeConfirm = false; showOsd("WIPED ALL ASSETS -> CLEAN DEFAULT.ZIP", 2800, false); return true; }
    if (sym == SDLK_F7 || (ctrlDown && sym == SDLK_z)) { if (restoreUndoSnapshot(window, cfg, state, "packs")) { refreshPackBrowserEntries(SDL_GetRenderer(window), state.packBrowser, "packs"); showOsd("UNDO: RESTORED PREVIOUS DEFAULT.ZIP", 2600, false); } else showOsd("NOTHING TO UNDO", 2000, false); return true; }
    return true;
}
// ------------------------------------------------------------------------------
// 79. Handle macro banks, slot recall/copy, fade timing & FX/sky/weather toggles
// ------------------------------------------------------------------------------
static bool handleMacroAndFxKeys(SDL_Window* window, SDL_Keycode sym, bool shiftDown, bool ctrlDown, bool altDown, FaceAppConfig& cfg, AppRuntimeState& state)
{
    std::array<char, 96> buf{};
    if ((altDown || (ctrlDown && shiftDown)) && (sym == SDLK_s || sym == SDLK_c || sym == SDLK_a || sym == SDLK_r || sym == SDLK_w)) {
        uint32_t bit = 0;
        const char* lbl = "";
        if (sym == SDLK_s)      { bit = (1u << 15); lbl = "STARS"; }
        else if (sym == SDLK_c) { bit = (1u << 16); lbl = "COMETS & SHOOTING STARS"; }
        else if (sym == SDLK_a) { bit = (1u << 17); lbl = "NORTHERN LIGHTS (AURORA)"; }
        else if (sym == SDLK_r) { bit = (1u << 18); lbl = "RAIN"; }
        else if (sym == SDLK_w) { bit = (1u << 19); lbl = "SNOWFALL"; }
        state.manualFxMask ^= bit;
        std::snprintf(buf.data(), buf.size(), "WEATHER/SKY %s: %s [S=SAVE]", lbl, (state.manualFxMask & bit) ? "ON" : "OFF");
        showOsd(buf.data(), 2200, false);
        return true;
    }
    if (sym == SDLK_PAGEUP || sym == SDLK_PAGEDOWN) {
        state.macroBank = (sym == SDLK_PAGEUP) ? ((state.macroBank >= MAX_MACRO_BANK) ? 0 : state.macroBank + 1) : ((state.macroBank <= 0) ? MAX_MACRO_BANK : state.macroBank - 1);
        std::snprintf(buf.data(), buf.size(), "MACRO BANK [%02d/24] (SLOTS %03d..%03d)", state.macroBank, state.macroBank * 10, std::min(MAX_MACRO_SLOT, state.macroBank * 10 + 9));
        showOsd(buf.data(), 2400, false);
        return true;
    }
    if (sym == SDLK_v) {
        cfg.fade_ms = shiftDown ? ((cfg.fade_ms <= 0) ? 2000 : cfg.fade_ms - 200) : ((cfg.fade_ms >= 2000) ? 0 : cfg.fade_ms + 200);
        std::snprintf(buf.data(), buf.size(), "MACRO FADE: %.1FS [S=SAVE]", cfg.fade_ms / 1000.0f);
        showOsd(buf.data(), 2200, false);
        return true;
    }
    if (sym == SDLK_m) {
        int nextSlot = state.browseMacroSlot + (shiftDown ? -1 : 1);
        if (nextSlot > MAX_MACRO_SLOT) nextSlot = 0;
        if (nextSlot < 0) nextSlot = MAX_MACRO_SLOT;
        state.browseMacroSlot = nextSlot;
        state.macroBank = nextSlot / 10;
        std::snprintf(buf.data(), buf.size(), "MACRO #%03d [BANK %02d] (LOCKED) [S=SAVE]", nextSlot, state.macroBank);
        triggerMacroTransition(state, cfg, nextSlot, nextSlot, buf.data());
        return true;
    }
    if (sym >= SDLK_0 && sym <= SDLK_9) {
        int keyDigit = (sym - SDLK_0);
        if (ctrlDown && altDown && keyDigit == 0) {
            state.manualFxMask = 0;
            showOsd("FX, SKY & WEATHER OVERLAYS: ALL OFF", 2000, false);
            return true;
        }
        if (altDown) {
            int fxIdx = (keyDigit == 0) ? 9 : (keyDigit - 1);
            static const std::array<const char*, 10> kFxNames = {"BAT", "FLY", "MOSQUITO", "BIRD", "WASP", "SPIDER", "BUTTERFLY", "UFO", "COBWEB", "FOG"};
            state.manualFxMask ^= (1u << fxIdx);
            std::snprintf(buf.data(), buf.size(), "FX %s: %s", kFxNames.at(fxIdx), (state.manualFxMask & (1u << fxIdx)) ? "ON" : "OFF");
            showOsd(buf.data(), 2000, false);
            return true;
        }
        int targetSlot = resolveBankKeySlot(state.macroBank, keyDigit);
        state.browseMacroSlot = targetSlot;
        if (ctrlDown) {
            pushUndoSnapshot(cfg.assets_dir, cfg.config_file, "packs");
            captureMacroCombo(cfg, state, targetSlot);
            saveIniConfig(window, cfg, state);
            saveCurrentAssetsToZipPack("default", cfg.assets_dir, cfg.config_file, "packs");
            state.manualMacroSlot = state.appliedMacroSlot = targetSlot;
            std::snprintf(buf.data(), buf.size(), "COPIED TO #%03d [BANK %02d] + DEFAULT.ZIP", targetSlot, state.macroBank);
            showOsd(buf.data(), 2600, false);
        } else {
            std::snprintf(buf.data(), buf.size(), "MACRO #%03d [BANK %02d] (LOCKED)", targetSlot, state.macroBank);
            triggerMacroTransition(state, cfg, targetSlot, targetSlot, buf.data());
        }
        return true;
    }
    return false;
}
// ------------------------------------------------------------------------------
// 80. Handle TAB + Arrows (move), Alt+Arrows (scale) & Ctrl+Arrows (split)
// ------------------------------------------------------------------------------
static bool handleOffsetCalibrationKeys(SDL_Keycode sym, bool shiftDown, bool ctrlDown, bool altDown, AppRuntimeState& state)
{
    std::array<char, 96> buf{};
    if (sym == SDLK_TAB) {
        if (!state.isEditingOffsets) { state.isEditingOffsets = true; if (shiftDown) state.selectedItemIdx = EDITABLE_ITEM_COUNT - 1; }
        else if (shiftDown) state.selectedItemIdx = (state.selectedItemIdx - 1 + EDITABLE_ITEM_COUNT) % EDITABLE_ITEM_COUNT;
        else state.selectedItemIdx = (state.selectedItemIdx + 1) % EDITABLE_ITEM_COUNT;
        const char* name = ""; int* ox = nullptr; int* oy = nullptr; float* sc = nullptr;
        getEditableTarget(state, state.selectedItemIdx, name, ox, oy, sc);
        std::snprintf(buf.data(), buf.size(), "EDIT %s [%+d,%+d %d%%] ALT+UP/DN=SIZE S=SAVE", name, *ox, *oy, static_cast<int>(std::round(*sc * 100.0f)));
        showOsd(buf.data(), 0, true);
        return true;
    }
    if (sym == SDLK_UP || sym == SDLK_DOWN || sym == SDLK_LEFT || sym == SDLK_RIGHT) {
        const char* name = ""; int* ox = nullptr; int* oy = nullptr; float* sc = nullptr;
        getEditableTarget(state, state.selectedItemIdx, name, ox, oy, sc);
        state.isEditingOffsets = true;
        if (altDown) {
            float d = (shiftDown ? 0.10f : 0.02f) * ((sym == SDLK_UP || sym == SDLK_RIGHT) ? 1.0f : -1.0f);
            *sc = std::max(0.50f, std::min(1.60f, *sc + d));
            g_eye_scale_factor.store(state.eyeScale, std::memory_order_relaxed);
            g_mouth_scale_factor.store(state.mouthScale, std::memory_order_relaxed);
            std::snprintf(buf.data(), buf.size(), "SCALE %s: %d%%  [S=SAVE]", name, static_cast<int>(std::round(*sc * 100.0f)));
        } else {
            int step = shiftDown ? 10 : 2;
            int* splitPtr = (state.selectedItemIdx == 2) ? &state.mouthSplitX : (state.selectedItemIdx == 3) ? &state.noseLayer.splitOffsetX : (state.selectedItemIdx == 4) ? &state.glassesLayer.splitOffsetX : (state.selectedItemIdx == 5) ? &state.browsLayer.splitOffsetX : (state.selectedItemIdx == 6) ? &state.beardLayer.splitOffsetX : (state.selectedItemIdx == 7) ? &state.faceLayer.splitOffsetX : (state.selectedItemIdx == 8) ? &state.earsLayer.splitOffsetX : nullptr;
            if (ctrlDown && splitPtr && (sym == SDLK_LEFT || sym == SDLK_RIGHT)) {
                *splitPtr += (sym == SDLK_RIGHT) ? step : -step;
                g_profile_face_split_x.store(state.faceLayer.splitOffsetX, std::memory_order_relaxed);
                g_profile_mouth_split_x.store(state.mouthSplitX, std::memory_order_relaxed);
                std::snprintf(buf.data(), buf.size(), "SPLIT %s: %+d PX  [S=SAVE]", name, *splitPtr);
            } else {
                if (sym == SDLK_UP) *oy -= step;
                if (sym == SDLK_DOWN) *oy += step;
                if (sym == SDLK_LEFT) *ox -= step;
                if (sym == SDLK_RIGHT) *ox += step;
                std::snprintf(buf.data(), buf.size(), "MOVE %s  X:%+d  Y:%+d  [S=SAVE]", name, *ox, *oy);
            }
        }
        showOsd(buf.data(), 0, true);
        return true;
    }
    return false;
}

// ------------------------------------------------------------------------------
// 81. Handle 0..255 paper-doll layers, backgrounds, animations & style presets
// ------------------------------------------------------------------------------
static void handleAssetAndStyleKeys(SDL_Keycode sym, bool shiftDown, bool ctrlDown, const FaceAppConfig& cfg, AppRuntimeState& state)
{
    std::array<char, 96> buf{};
    static int s_irisStep = 0;

    switch (sym) {
        case SDLK_k: {
            state.bgLayer.testIndex = stepAssetIndex(state.bgLayer.testIndex, 255, shiftDown);
            std::array<char, 24> skyFn{}; std::snprintf(skyFn.data(), skyFn.size(), "%03u_sky.png", state.bgLayer.testIndex);
            bool hasBg = (state.bgLayer.testIndex == 0) || fs::exists(fs::path(cfg.assets_dir) / "bg" / skyFn.data()) || assetFileExists(cfg.assets_dir, "bg", state.bgLayer.testIndex);
            if (state.bgLayer.testIndex == 0) std::snprintf(buf.data(), buf.size(), "BACKGROUND [000/255] OFF");
            else if (hasBg)                   std::snprintf(buf.data(), buf.size(), "BACKGROUND [%03u/255]", state.bgLayer.testIndex);
            else                              std::snprintf(buf.data(), buf.size(), "BACKGROUND [%03u/255] (EMPTY)", state.bgLayer.testIndex);
            showOsd(buf.data(), 2400, false);
            break;
        }
        case SDLK_b: {
            if (ctrlDown) { state.manualFxMask = 0; showOsd("FX & WEATHER OVERLAYS: ALL OFF", 2000, false); break; }
            int curBit = -1;
            for (int i = 0; i < 20; ++i) if (state.manualFxMask == (1u << i)) curBit = i;
            int nextBit = curBit + (shiftDown ? -1 : 1);
            if (nextBit > 19 || nextBit < 0) { state.manualFxMask = 0; showOsd("FX & WEATHER OVERLAY: OFF [00/20]", 2000, false); }
            else {
                state.manualFxMask = (1u << nextBit);
                static const std::array<const char*, 20> kAllFx = {
                    "BAT", "FLY", "MOSQUITO", "BIRD", "WASP", "SPIDER", "BUTTERFLY", "UFO", "COBWEB", "FOG",
                    "GHOST", "SNAKE", "FIRE", "LIGHTNING", "MOUSE", "STARS", "COMETS", "AURORA", "RAIN", "SNOWFALL"
                };
                std::snprintf(buf.data(), buf.size(), "FX [%02d/20]: %s [CTRL+SHIFT+S/C/A/R/W=MIX]", nextBit + 1, kAllFx.at(nextBit));
                showOsd(buf.data(), 2400, false);
            }
            break;
        }
        case SDLK_a: {
            int nextAnim = state.manualTestAnim + (shiftDown ? -1 : 1);
            if (nextAnim > 64) nextAnim = 0;
            if (nextAnim < 0) nextAnim = 64;
            state.manualTestAnim = nextAnim;
            if (nextAnim == 0 && state.manualMacroSlot < 0) std::snprintf(buf.data(), buf.size(), "ANIM [00/64] AUTO SHOW");
            else std::snprintf(buf.data(), buf.size(), "ANIM [%02d/64] %s", nextAnim, getMacroAnimationName(nextAnim));
            showOsd(buf.data(), 2600, false);
            break;
        }
        case SDLK_l: { int nl = g_override_lip_color.load() + (shiftDown ? -1 : 1); if (nl > 9) nl = 0; if (nl < 0) nl = 9; g_override_lip_color = nl; std::snprintf(buf.data(), buf.size(), "LIP COLOR PRESET: %d", nl); showOsd(buf.data(), 2600, false); break; }
        case SDLK_e: { int nl = g_override_eyelash.load() + (shiftDown ? -1 : 1); if (nl > 15) nl = 0; if (nl < 0) nl = 15; g_override_eyelash = nl; std::snprintf(buf.data(), buf.size(), (nl == 0 ? "EYELASHES: OFF [00/15]" : "EYELASHES: [%02d/15]"), nl); showOsd(buf.data(), 2600, false); break; }
        case SDLK_o: {
            static const std::array<uint8_t, 12> kV = {10, 32, 53, 74, 96, 117, 138, 160, 181, 202, 224, 245};
            static const std::array<const char*, 12> kN = {"DEFAULT SLY", "DEFAULT ROUND", "DEFAULT ALMOND", "DRAGON SLY", "DRAGON ROUND", "DRAGON ALMOND", "DEF SLY [NO LID]", "DEF ROUND [NO LID]", "DEF ALMOND [NO LID]", "DRG SLY [NO LID]", "DRG ROUND [NO LID]", "DRG ALMOND [NO LID]"};
            int curOv = g_override_eye_style.load(), curIdx = -1;
            for (int i = 0; i < 12; ++i) if (curOv == static_cast<int>(kV.at(i))) curIdx = i;
            int nextIdx = curIdx + (shiftDown ? -1 : 1);
            if (nextIdx > 11) nextIdx = -1;
            if (nextIdx < -1) nextIdx = 11;
            if (nextIdx < 0) { g_override_eye_style = -1; std::snprintf(buf.data(), buf.size(), "EYE SHAPE: AUTO"); }
            else { g_override_eye_style = static_cast<int>(kV.at(nextIdx)); std::snprintf(buf.data(), buf.size(), "EYE [%02d/12] %s", nextIdx + 1, kN.at(nextIdx)); }
            showOsd(buf.data(), 2600, false);
            break;
        }
        case SDLK_i: {
            if (state.isDmxActive) { showOsd("DMX ACTIVE - IRIS LOCKED TO DMX", 2000, false); break; }
            if (g_override_iris.load() < 0) s_irisStep = 0;
            s_irisStep += (shiftDown ? -1 : 1);
            if (s_irisStep > 10) s_irisStep = 0;
            if (s_irisStep < 0) s_irisStep = 10;
            if (s_irisStep == 0) { g_override_iris = -1; std::snprintf(buf.data(), buf.size(), "IRIS SIZE: AUTO"); }
            else { g_override_iris = ((s_irisStep - 1) * 255) / 9; std::snprintf(buf.data(), buf.size(), "IRIS SIZE: [%02d/10]", s_irisStep); }
            showOsd(buf.data(), 2600, false);
            break;
        }
        case SDLK_t: { int nt = g_override_teeth.load() + (shiftDown ? -1 : 1); if (nt > 15) nt = 0; if (nt < 0) nt = 15; g_override_teeth = nt; std::snprintf(buf.data(), buf.size(), "TEETH PRESET: %d", nt); showOsd(buf.data(), 2400, false); break; }
        case SDLK_y: state.dualProfileMode = !state.dualProfileMode; showOsd(state.dualProfileMode ? "DUAL SIDE PROFILE: ON" : "DUAL SIDE PROFILE: OFF", 2200, false); break;
        case SDLK_u: if (ctrlDown) { state.manualHideMouth = !state.manualHideMouth; showOsd(state.manualHideMouth ? "MOUTH: HIDDEN" : "MOUTH: VISIBLE", 2200, false); } break;
        case SDLK_n: if (ctrlDown) { state.manualHideNose = !state.manualHideNose; showOsd(state.manualHideNose ? "NOSE: HIDDEN" : "NOSE: VISIBLE", 2200, false); } else { state.noseLayer.testIndex = stepAssetIndex(state.noseLayer.testIndex, 255, shiftDown); showAssetStepOsd(cfg.assets_dir, "NOSE", "nose", state.noseLayer.testIndex); } break;
        case SDLK_w: if (ctrlDown) { state.manualHideBrows = !state.manualHideBrows; showOsd(state.manualHideBrows ? "BROWS: HIDDEN" : "BROWS: VISIBLE", 2200, false); } else { state.browsLayer.testIndex = stepAssetIndex(state.browsLayer.testIndex, 255, shiftDown); showAssetStepOsd(cfg.assets_dir, "BROWS", "brows", state.browsLayer.testIndex); } break;
        case SDLK_f: state.faceLayer.testIndex = stepAssetIndex(state.faceLayer.testIndex, 255, shiftDown); showAssetStepOsd(cfg.assets_dir, "FACE", "face", state.faceLayer.testIndex); break;
        case SDLK_r: state.earsLayer.testIndex = stepAssetIndex(state.earsLayer.testIndex, 255, shiftDown); showAssetStepOsd(cfg.assets_dir, "EARS", "ears", state.earsLayer.testIndex); break;
        case SDLK_j: state.beardLayer.testIndex = stepAssetIndex(state.beardLayer.testIndex, 255, shiftDown); showAssetStepOsd(cfg.assets_dir, "BEARD/JAW", "beard", state.beardLayer.testIndex); break;
        case SDLK_g: state.glassesLayer.testIndex = stepAssetIndex(state.glassesLayer.testIndex, 255, shiftDown); showAssetStepOsd(cfg.assets_dir, "GLASSES", "glasses", state.glassesLayer.testIndex); break;
        case SDLK_h: state.hairLayer.testIndex = stepAssetIndex(state.hairLayer.testIndex, 255, shiftDown); showAssetStepOsd(cfg.assets_dir, "HAIR", "hair", state.hairLayer.testIndex); break;
        case SDLK_c: state.hatLayer.testIndex = stepAssetIndex(state.hatLayer.testIndex, 255, shiftDown); showAssetStepOsd(cfg.assets_dir, "CAP/HAT", "hat", state.hatLayer.testIndex); break;
        case SDLK_p: state.makeupLayer.testIndex = stepAssetIndex(state.makeupLayer.testIndex, 255, shiftDown); showAssetStepOsd(cfg.assets_dir, "PAINT/MAKEUP", "makeup", state.makeupLayer.testIndex); break;
        default: break;
    }
}

// ------------------------------------------------------------------------------
// 82. Main SDL keyboard router dispatching to camera, browsers & asset keys
// ------------------------------------------------------------------------------
static void handleKeyboardEvent(SDL_Window* window, SDL_Keycode sym, SDL_Keymod mod, FaceAppConfig& cfg, AppRuntimeState& state)
{
    bool shiftDown = (mod & KMOD_SHIFT) != 0, ctrlDown = (mod & KMOD_CTRL) != 0, altDown = (mod & KMOD_ALT) != 0;
    std::array<char, 96> buf{};

    if (handleCameraViewfinderKeys(window, sym, state)) return;
    if (handleFlatImportKeys(window, sym, shiftDown, cfg, state)) return;
    if (handlePackBrowserKeys(window, sym, ctrlDown, cfg, state)) return;
    if (handleEyeBrowserKeys(sym, state)) return;
    if (handleMacroAndFxKeys(window, sym, shiftDown, ctrlDown, altDown, cfg, state)) return;
    if (handleOffsetCalibrationKeys(sym, shiftDown, ctrlDown, altDown, state)) return;

    switch (sym) {
        case SDLK_ESCAPE:
            if (g_show_about_overlay) g_show_about_overlay = false;
            else if (state.showHelpOverlay) state.showHelpOverlay = false;
            else if (state.showTelemetry) state.showTelemetry = false;
            else if (state.isEditingOffsets) { state.isEditingOffsets = false; hideOsd(); }
            else if (state.manualMacroSlot >= 0 || state.manualTestAnim > 0) { resetAutonomousMacroShow(); triggerMacroTransition(state, cfg, -1, 0, "AUTO SHOW RESUMED"); }
            break;
        case SDLK_q:   if (!ctrlDown && !altDown) g_running = false; break;
        case SDLK_z:   if (ctrlDown) { if (restoreUndoSnapshot(window, cfg, state, "packs")) showOsd("UNDO: RESTORED PREVIOUS DEFAULT.ZIP", 2600, false); else showOsd("NOTHING TO UNDO", 2000, false); } break;
        case SDLK_i:   if (ctrlDown) { openFlatImportStudio(SDL_GetRenderer(window), state.flatImport, "import"); return; } handleAssetAndStyleKeys(sym, shiftDown, ctrlDown, cfg, state); break;
        case SDLK_p:   if (ctrlDown) { openCameraViewfinder(state.cameraCapture); return; } handleAssetAndStyleKeys(sym, shiftDown, ctrlDown, cfg, state); break;
        case SDLK_F1:  state.showHelpOverlay = !state.showHelpOverlay; g_show_about_overlay = false; break;
        case SDLK_F2:  state.showTelemetry = !state.showTelemetry; showOsd(state.showTelemetry ? "TELEMETRY: ON" : "TELEMETRY: OFF", 1600, false); break;
        case SDLK_F3:  resetAutonomousMacroShow(); triggerMacroTransition(state, cfg, -1, 0, "AUTO SHOW RESET -> SLOT #000"); break;
        case SDLK_F4:
        case SDLK_s:   performUnifiedSave(window, cfg, state); break;
        case SDLK_F5:  state.osdEnabled = !state.osdEnabled; osd.enabled = state.osdEnabled; if (!state.osdEnabled) hideOsd(); else showOsd("OSD: ON", 1600, false); break;
        case SDLK_F6:  state.packBrowser.open = true; state.packBrowser.inspectMode = false; state.packBrowser.wipeConfirm = false; refreshPackBrowserEntries(SDL_GetRenderer(window), state.packBrowser, "packs"); break;
        case SDLK_F7:  if (restoreUndoSnapshot(window, cfg, state, "packs")) showOsd("UNDO: RESTORED PREVIOUS DEFAULT.ZIP", 2600, false); else showOsd("NOTHING TO UNDO", 2000, false); break;
        case SDLK_F8:  state.eyeBrowser.open = true; refreshEyeBrowserEntries(SDL_GetRenderer(window), state.eyeBrowser, cfg.assets_dir); break;
        case SDLK_F9:  g_show_about_overlay = !g_show_about_overlay; state.showHelpOverlay = false; break;
        case SDLK_F11: toggleWindowFullscreen(window, state.isFullscreen); break;
        case SDLK_F12: state.manualBlackout = !state.manualBlackout; showOsd(state.manualBlackout ? "BLACKOUT: ON" : "BLACKOUT: OFF", 1800, false); break;
        case SDLK_x:
            if (ctrlDown) {
                saveIniConfig(window, cfg, state);
                std::string newName = nextCustomZipPackName("packs");
                if (saveCurrentAssetsToZipPack(newName, cfg.assets_dir, cfg.config_file, "packs")) {
                    std::snprintf(buf.data(), buf.size(), "EXPORTED NEW ZIP: %s.ZIP", newName.c_str());
                    showOsd(buf.data(), 2800, false);
                } else showOsd("ERROR EXPORTING ZIP PACK!", 2400, false);
            } else {
                state.packBrowser.open = true;
                state.packBrowser.inspectMode = false;
                state.packBrowser.wipeConfirm = false;
                refreshPackBrowserEntries(SDL_GetRenderer(window), state.packBrowser, "packs");
            }
            break;
        default: handleAssetAndStyleKeys(sym, shiftDown, ctrlDown, cfg, state); break;
    }
}

// ------------------------------------------------------------------------------
// 83. Render calibrated import overlay item & oval eye/inpaint guides
// ------------------------------------------------------------------------------
static void renderFlatImportLiveLayer(SDL_Renderer* ren, const FlatImportState& fi, int targetCat)
{
    if (!fi.open || fi.items.empty()) return;
    const auto& it = fi.items.at(fi.selectedIdx);
    if (targetCat >= 0 && it.targetCatIdx != targetCat) return;
    if (!it.previewTex) return;

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
// 84. Render 5-layer depth backdrop, 3-depth FX/critters, character & overlays
// ------------------------------------------------------------------------------
static void renderCompositedFrame(
    SDL_Renderer* renderer, const FaceAppConfig& cfg, const AppRuntimeState& state,
    SDL_Texture* eyesTexture, SDL_Texture* featuresTexture,
    uint8_t reqFace, uint8_t reqBeard, uint8_t reqHair, uint32_t reqFxMask,
    bool hideAll, bool hideMouth, bool hideBrows, uint8_t transitionFadeAlpha, uint32_t nowMs)
{
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    int outW = BASE_CANVAS_W, outH = BASE_CANVAS_H;
    SDL_GetRendererOutputSize(renderer, &outW, &outH);
    float scale = std::min(static_cast<float>(outW) / BASE_CANVAS_W, static_cast<float>(outH) / BASE_CANVAS_H);
    int vpW = static_cast<int>(std::round(BASE_CANVAS_W * scale));
    int vpH = static_cast<int>(std::round(BASE_CANVAS_H * scale));
    SDL_Rect faceVp{(outW - vpW) / 2, (outH - vpH) / 2, vpW, vpH};

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

    bool trackedGaze = false;
    int bgW = static_cast<int>(std::round(outW * state.bgLayer.scale));
    int bgH = static_cast<int>(std::round(outH * state.bgLayer.scale));
    SDL_Rect bgVp{(outW - bgW) / 2 + static_cast<int>(std::round(state.bgLayer.offsetX * scale)),
                  (outH - bgH) / 2 + static_cast<int>(std::round(state.bgLayer.offsetY * scale)), bgW, bgH};
    SDL_RenderSetViewport(renderer, &bgVp);
    if (!hideAll)
    {
        renderDepthBackdropLayers(renderer, state.parallaxBg, reqFxMask, nowMs, bgW, bgH);
        if ((reqFxMask & (1u << 9)) && state.fxBank.fog2)
        {
            float t = static_cast<float>(nowMs) * 0.001f;
            int bx = static_cast<int>(std::cos(t * 0.14f) * (bgW * 0.12f)) - static_cast<int>(bgW * 0.10f);
            SDL_Rect backFog{bx, static_cast<int>(bgH * 0.08f), static_cast<int>(bgW * 1.24f), bgH};
            SDL_RenderCopy(renderer, state.fxBank.fog2, nullptr, &backFog);
        }
    }

    int fxW = static_cast<int>(std::round(vpW * state.fxLayer.scale));
    int fxH = static_cast<int>(std::round(vpH * state.fxLayer.scale));
    SDL_Rect fxVp{faceVp.x + (vpW - fxW) / 2 + static_cast<int>(std::round(state.fxLayer.offsetX * scale)),
                  faceVp.y + (vpH - fxH) / 2 + static_cast<int>(std::round(state.fxLayer.offsetY * scale)), fxW, fxH};

    if (!hideAll && reqFxMask != 0) {
        SDL_RenderSetViewport(renderer, &fxVp);
        SDL_RenderSetScale(renderer, scale * state.fxLayer.scale, scale * state.fxLayer.scale);
        renderEffectsAtDepth(renderer, state.fxBank, reqFxMask, 0, nowMs, trackedGaze);
        renderEffectsAtDepth(renderer, state.fxBank, reqFxMask, 1, nowMs, trackedGaze);
        SDL_RenderSetScale(renderer, 1.0f, 1.0f);
    }
    SDL_RenderSetViewport(renderer, &faceVp);
    SDL_RenderSetScale(renderer, scale, scale);
    bool isAnyProfile      = (reqFace >= PROFILE_FACE_START_INDEX);
    bool isDualProfile     = (isAnyProfile && state.dualProfileMode);
    bool isSingleLeftProf  = (isAnyProfile && !isDualProfile && (reqFace % 2 == 1));
    bool isSingleRightProf = (isAnyProfile && !isDualProfile && (reqFace % 2 == 0));
    int fy        = GLOBAL_FACE_Y_OFFSET;
    int fSplit    = isAnyProfile ? state.faceLayer.splitOffsetX : 0;
    int eyeShiftL = (isSingleLeftProf || isDualProfile) ? (-5 - fSplit) : 0;
    int eyeShiftR = (isSingleRightProf || isDualProfile) ? (5 + fSplit) : 0;
    bool showEyeL = !isSingleRightProf, showEyeR = !isSingleLeftProf;

    if (!hideAll && state.faceLayer.texture) {
        int sockRx = isAnyProfile ? 66 : 82;
        int sockRy = isAnyProfile ? 54 : 64;
        if (showEyeL) drawOvalShadow(renderer, 190 + eyeShiftL + state.eyeLeftOffsetX, 355 + fy + state.eyeLeftOffsetY, sockRx, sockRy, 255);
        if (showEyeR) drawOvalShadow(renderer, 410 + eyeShiftR + state.eyeRightOffsetX, 355 + fy + state.eyeRightOffsetY, sockRx, sockRy, 255);
    }

    SDL_Rect eyeSrcL{0, 0, BASE_CANVAS_W / 2, BASE_CANVAS_H}, eyeDstL{state.eyeLeftOffsetX + eyeShiftL, state.eyeLeftOffsetY, BASE_CANVAS_W / 2, BASE_CANVAS_H};
    SDL_Rect eyeSrcR{BASE_CANVAS_W / 2, 0, BASE_CANVAS_W / 2, BASE_CANVAS_H}, eyeDstR{(BASE_CANVAS_W / 2) + state.eyeRightOffsetX + eyeShiftR, state.eyeRightOffsetY, BASE_CANVAS_W / 2, BASE_CANVAS_H};
    if (!hideAll && showEyeL && !isEyePoppedOut(0)) SDL_RenderCopy(renderer, eyesTexture, &eyeSrcL, &eyeDstL);
    if (!hideAll && showEyeR && !isEyePoppedOut(1)) SDL_RenderCopy(renderer, eyesTexture, &eyeSrcR, &eyeDstR);

    if (!hideAll) {
        auto renderSplitLayer = [&](const PaperDollLayer& layer, int extraY, int extraSplit) {
            if (!layer.texture) return;
            float sc = layer.scale;
            int halfW = static_cast<int>(std::round((BASE_CANVAS_W / 2) * sc));
            int fullH = static_cast<int>(std::round(BASE_CANVAS_H * sc));
            int dy = (BASE_CANVAS_H - fullH) / 2;
            if (showEyeL) { SDL_Rect sL{0, 0, BASE_CANVAS_W / 2, BASE_CANVAS_H}, dL{(BASE_CANVAS_W / 2) - halfW + layer.offsetX - layer.splitOffsetX - extraSplit, layer.offsetY + fy + extraY + dy, halfW, fullH}; SDL_RenderCopy(renderer, layer.texture, &sL, &dL); }
            if (showEyeR) { SDL_Rect sR{BASE_CANVAS_W / 2, 0, BASE_CANVAS_W / 2, BASE_CANVAS_H}, dR{(BASE_CANVAS_W / 2) + layer.offsetX + layer.splitOffsetX + extraSplit, layer.offsetY + fy + extraY + dy, halfW, fullH}; SDL_RenderCopy(renderer, layer.texture, &sR, &dR); }
        };
        auto renderProfPair = [&](const PaperDollLayer& lL, const PaperDollLayer& lR, int extraY, int extraSplit) {
            float sc = lL.scale;
            int fullW = static_cast<int>(std::round(BASE_CANVAS_W * sc));
            int fullH = static_cast<int>(std::round(BASE_CANVAS_H * sc));
            int dx = (BASE_CANVAS_W - fullW) / 2;
            int dy = (BASE_CANVAS_H - fullH) / 2;
            if (isDualProfile) {
                if (lL.texture) { SDL_Rect dL{lL.offsetX - fSplit - extraSplit + dx, lL.offsetY + fy + extraY + dy, fullW, fullH}; SDL_RenderCopy(renderer, lL.texture, nullptr, &dL); }
                if (lR.texture) { SDL_Rect dR{lR.offsetX + fSplit + extraSplit + dx, lR.offsetY + fy + extraY + dy, fullW, fullH}; SDL_RenderCopy(renderer, lR.texture, nullptr, &dR); }
            } else if (lL.texture) {
                int sh = isSingleLeftProf ? (-fSplit - extraSplit) : (fSplit + extraSplit);
                SDL_Rect d{lL.offsetX + sh + dx, lL.offsetY + fy + extraY + dy, fullW, fullH};
                SDL_RenderCopy(renderer, lL.texture, nullptr, &d);
            }
        };

        bool effHideMouth = hideMouth || (reqBeard >= JAW_LAYER_START_INDEX && reqBeard <= 108);
        bool animateJaw   = (reqBeard >= JAW_LAYER_START_INDEX) || (effHideMouth && reqBeard > 0);
        int jawDrop = animateJaw ? static_cast<int>(std::round((g_effective_mouth_gap.load() / 255.0f) * 48.0f)) : 0;

        if (!isAnyProfile) renderSplitLayer(state.earsLayer, 0, 0);
        renderProfPair(state.faceLayer, state.dualFaceLayer, 0, 0);
        renderProfPair(state.makeupLayer, state.dualMakeupLayer, 0, 0);
        if (isAnyProfile) renderProfPair(state.earsLayer, state.dualEarsLayer, 0, state.earsLayer.splitOffsetX);
        if (!effHideMouth) { SDL_Rect d{state.mouthOffsetX, state.mouthOffsetY, BASE_CANVAS_W, BASE_CANVAS_H}; SDL_RenderCopy(renderer, featuresTexture, nullptr, &d); }
        renderProfPair(state.noseLayer, state.dualNoseLayer, 0, state.noseLayer.splitOffsetX);
        if (!hideBrows) {
            if (isAnyProfile && state.browsLayer.loadedIndex >= PROFILE_FACE_START_INDEX) renderProfPair(state.browsLayer, state.dualBrowsLayer, 0, state.browsLayer.splitOffsetX);
            else if (state.browsLayer.texture) renderDynamicEyebrowsSplit(renderer, state.browsLayer, g_brow_left_tilt.load(), g_brow_right_tilt.load(), showEyeL, showEyeR, fSplit);
        }
        if (isAnyProfile && state.beardLayer.loadedIndex >= PROFILE_FACE_START_INDEX) renderProfPair(state.beardLayer, state.dualBeardLayer, jawDrop, state.beardLayer.splitOffsetX);
        else renderSplitLayer(state.beardLayer, jawDrop, fSplit);

        if (isAnyProfile && state.glassesLayer.loadedIndex >= PROFILE_FACE_START_INDEX) renderProfPair(state.glassesLayer, state.dualGlassesLayer, 0, state.glassesLayer.splitOffsetX);
        else renderSplitLayer(state.glassesLayer, 0, fSplit);

        renderProfPair(state.hairLayer, state.dualHairLayer, 0, 0);
        int hatLift = (reqHair > 0) ? -24 : 0;
        renderProfPair(state.hatLayer, state.dualHatLayer, hatLift, 0);

        if (showEyeL && isEyePoppedOut(0)) { float f = getEyePopOutFactor(0); drawOvalShadow(renderer, 190 + eyeShiftL + state.eyeLeftOffsetX + 10, 348 + fy + state.eyeLeftOffsetY + 26, static_cast<int>(62 + f * 24), static_cast<int>(54 + f * 20), 135); SDL_RenderCopy(renderer, eyesTexture, &eyeSrcL, &eyeDstL); }
        if (showEyeR && isEyePoppedOut(1)) { float f = getEyePopOutFactor(1); drawOvalShadow(renderer, 410 + eyeShiftR + state.eyeRightOffsetX + 10, 348 + fy + state.eyeRightOffsetY + 26, static_cast<int>(62 + f * 24), static_cast<int>(54 + f * 20), 135); SDL_RenderCopy(renderer, eyesTexture, &eyeSrcR, &eyeDstR); }
    }
    SDL_RenderSetScale(renderer, 1.0f, 1.0f);
    SDL_RenderSetViewport(renderer, &bgVp);
    if (!hideAll && (reqFxMask & (1u << 12)) && state.fxBank.sprites.at(12)) {
        float t = static_cast<float>(nowMs) * 0.001f;
        SDL_Texture* fTex = (((nowMs / 160) % 2) == 0 || !state.fxBank.sprites.at(13)) ? state.fxBank.sprites.at(12) : state.fxBank.sprites.at(13);
        int swayX = static_cast<int>(std::sin(t * 3.2f) * (bgW * 0.025f)) - static_cast<int>(bgW * 0.04f);
        SDL_Rect fireDst{swayX, static_cast<int>(bgH * 0.12f), static_cast<int>(bgW * 1.08f), bgH};
        SDL_RenderCopy(renderer, fTex, nullptr, &fireDst);
    }
    if (!hideAll && state.parallaxBg.frontLayer) {
        int tw = 0, th = 0; SDL_QueryTexture(state.parallaxBg.frontLayer, nullptr, nullptr, &tw, &th);
        SDL_Rect src = (tw > 8 && th > 4) ? SDL_Rect{2, 0, tw - 4, th - 2} : SDL_Rect{0, 0, tw, th};
        SDL_Rect full{0, 0, bgW, bgH + 2}; SDL_RenderCopy(renderer, state.parallaxBg.frontLayer, &src, &full);
    }
    if (!hideAll && (reqFxMask & (1u << 9)) && state.fxBank.sprites.at(9)) {
        float t = static_cast<float>(nowMs) * 0.001f;
        int fx = static_cast<int>(std::sin(t * 0.25f) * (bgW * 0.14f)) - static_cast<int>(bgW * 0.10f);
        SDL_Rect frontFog{fx, static_cast<int>(bgH * 0.18f), static_cast<int>(bgW * 1.24f), bgH};
        SDL_RenderCopy(renderer, state.fxBank.sprites.at(9), nullptr, &frontFog);
    }

    SDL_RenderSetViewport(renderer, &fxVp);
    SDL_RenderSetScale(renderer, scale * state.fxLayer.scale, scale * state.fxLayer.scale);
    if (!hideAll) renderEffectsAtDepth(renderer, state.fxBank, reqFxMask, 2, nowMs, trackedGaze);
    if (!trackedGaze) clearGazeTarget();

    SDL_RenderSetViewport(renderer, &faceVp);
    SDL_RenderSetScale(renderer, scale, scale);
    renderBrowsersAndTelemetry(renderer, cfg, state);
    renderOsdOverlay(renderer, nowMs, state);
    SDL_RenderSetScale(renderer, 1.0f, 1.0f);
    SDL_RenderSetViewport(renderer, nullptr);
    uint8_t totalBlackout = std::max(state.masterDimmer, transitionFadeAlpha);
    if (totalBlackout > 0) { SDL_SetRenderDrawColor(renderer, 0, 0, 0, totalBlackout); SDL_Rect r{0, 0, outW, outH}; SDL_RenderFillRect(renderer, &r); }
    SDL_RenderPresent(renderer);
}

// ------------------------------------------------------------------------------
// 85. Update DMX connection state, online/offline transitions & crossfade alpha
// ------------------------------------------------------------------------------
static void updateDmxAndFadeState(const FaceAppConfig& cfg, AppRuntimeState& state, uint32_t frameStart, uint32_t lastPktMs)
{
    state.isDmxActive = (lastPktMs > 0 && (frameStart - lastPktMs) <= DMX_TIMEOUT_MS);
    if (state.isDmxActive != state.wasDmxActive) {
        if (state.isDmxActive) {
            state.manualMacroSlot = -1; state.manualTestAnim = 0; state.isEditingOffsets = false;
            state.manualHideMouth = state.manualHideNose = state.manualHideBrows = false;
            g_override_teeth = g_override_eyelash = g_override_lip_color = g_override_eye_style = g_override_iris = -1;
        } else {
            state.dmxMacroLocked = false; state.appliedMacroSlot = -1;
            resetAutonomousMacroShow(); applyMacroComboToState(cfg, state, 0, false); applyMaskOffsetProfile(cfg, state, state.faceLayer.testIndex);
        }
        state.wasDmxActive = state.isDmxActive;
    }
    if (state.fadePhase == 1) {
        uint32_t halfDur = std::max(1, cfg.fade_ms / 2), elapsed = frameStart - state.fadeStartMs;
        if (elapsed >= halfDur) { executePendingMacroSwitch(cfg, state); state.fadePhase = 2; state.fadeAlpha = 255; }
        else state.fadeAlpha = static_cast<uint8_t>((elapsed * 255U) / halfDur);
    } else if (state.fadePhase == 2) {
        uint32_t halfDur = std::max(1, cfg.fade_ms / 2), elapsed = (frameStart - state.fadeStartMs) - halfDur;
        if (elapsed >= halfDur) { state.fadePhase = 0; state.fadeAlpha = 0; }
        else state.fadeAlpha = static_cast<uint8_t>(255U - (elapsed * 255U) / halfDur);
    } else state.fadeAlpha = 0;
}

// ------------------------------------------------------------------------------
// 86. Resolve active DMX/macro/inspector layers and load PNG textures
// ------------------------------------------------------------------------------
static void resolveActiveLayersAndAudio(
    SDL_Window* window, SDL_Renderer* renderer, FaceAppConfig& cfg, AppRuntimeState& state,
    uint32_t frameStart, uint8_t& outRawMacro, bool& outDmxEditOverlay, uint8_t& outRawDimmer)
{
    outRawMacro = 0; uint8_t rawMacroCtrl = 0;
    if (state.isDmxActive) {
        outRawMacro  = dmx_read_slot(0, getWrappedDmxIndex(cfg.start_channel, DMX_OFFSET_MACRO) + 1);
        rawMacroCtrl = dmx_read_slot(0, getWrappedDmxIndex(cfg.start_channel, DMX_OFFSET_MACRO_CTRL) + 1);
    }
    outDmxEditOverlay = state.isDmxActive && (rawMacroCtrl >= 100 && rawMacroCtrl <= 199);

    if (state.isDmxActive && isDmxMacroActive(outRawMacro)) {
        int slot = decodeDmxMacroSlot(outRawMacro);
        if (!state.dmxMacroLocked || slot != state.appliedMacroSlot) {
            state.dmxMacroLocked = true;
            if (outDmxEditOverlay) state.appliedMacroSlot = slot;
            else triggerMacroTransition(state, cfg, -1, slot, nullptr);
        }
        if (rawMacroCtrl >= 200) {
            if (state.dmxSaveHoldStart == 0) state.dmxSaveHoldStart = frameStart;
            else if (!state.dmxSaveTriggered && (frameStart - state.dmxSaveHoldStart) >= static_cast<uint32_t>(cfg.save_hold_ms)) {
                pushUndoSnapshot(cfg.assets_dir, cfg.config_file, "packs");
                captureMacroCombo(cfg, state, slot); saveIniConfig(window, cfg, state);
                saveCurrentAssetsToZipPack("default", cfg.assets_dir, cfg.config_file, "packs");
                state.dmxSaveTriggered = true;
                std::array<char, 48> sbuf{}; std::snprintf(sbuf.data(), sbuf.size(), "DMX SAVED MACRO #%03d", slot);
                showOsd(sbuf.data(), 2600, false);
            }
        } else { state.dmxSaveHoldStart = 0; state.dmxSaveTriggered = false; }
    } else {
        if (state.dmxMacroLocked) { state.dmxMacroLocked = false; state.appliedMacroSlot = -1; g_override_teeth = g_override_eyelash = g_override_lip_color = g_override_eye_style = g_override_iris = -1; }
        state.dmxSaveHoldStart = 0; state.dmxSaveTriggered = false;
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
        for (auto* l : allLyrs) { if (l->texture) { SDL_DestroyTexture(l->texture); l->texture = nullptr; } l->loadedIndex = 0; }
        destroyParallaxBackdrop(state.parallaxBg);
        reloadDynamicEyeBank(activeAssetsDir);
        s_wasInspectingZip = inspectingZip;
    }

    if (inspectingZip) {
        const auto& item = state.packBrowser.inspectItems.at(state.packBrowser.inspectIdx);
        applyPresetValuesToState(item.preset, state, 1, true);
        FaceAppConfig tmpCfg{};
        loadIniConfig("packs/.inspect_tmp/makingfaces.ini", tmpCfg);
        applyMaskOffsetProfile(tmpCfg, state, state.faceLayer.testIndex);
    }

    bool useMacroLayer = inspectingZip || state.dmxMacroLocked || (state.manualMacroSlot >= 0) || (!state.isDmxActive && state.manualTestAnim == 0);
    if (state.isDmxActive && !state.dmxMacroLocked && !inspectingZip) {
        state.curFace = dmx_read_slot(0, getWrappedDmxIndex(cfg.start_channel, DMX_OFFSET_FACE_MASK) + 1);
        state.curMakeup = dmx_read_slot(0, getWrappedDmxIndex(cfg.start_channel, DMX_OFFSET_MAKEUP_MASK) + 1);
        state.curEars = dmx_read_slot(0, getWrappedDmxIndex(cfg.start_channel, DMX_OFFSET_EARS_MASK) + 1);
        state.curNose = dmx_read_slot(0, getWrappedDmxIndex(cfg.start_channel, DMX_OFFSET_NOSE_MASK) + 1);
        state.curBeard = dmx_read_slot(0, getWrappedDmxIndex(cfg.start_channel, DMX_OFFSET_BEARD_MASK) + 1);
        state.curGlasses = dmx_read_slot(0, getWrappedDmxIndex(cfg.start_channel, DMX_OFFSET_GLASSES_MASK) + 1);
        state.curHair = dmx_read_slot(0, getWrappedDmxIndex(cfg.start_channel, DMX_OFFSET_HAIR_MASK) + 1);
        state.curHat = dmx_read_slot(0, getWrappedDmxIndex(cfg.start_channel, DMX_OFFSET_HAT_MASK) + 1);
        state.curBrows = dmx_read_slot(0, getWrappedDmxIndex(cfg.start_channel, DMX_OFFSET_BROW_MASK) + 1);
        state.curBgScene = dmx_read_slot(0, getWrappedDmxIndex(cfg.start_channel, DMX_OFFSET_BG_SCENE) + 1);
        uint8_t rawFx = dmx_read_slot(0, getWrappedDmxIndex(cfg.start_channel, DMX_OFFSET_FX_OVERLAY) + 1);
        state.curFxMask = (rawFx == 0) ? 0 : static_cast<uint16_t>(1u << ((rawFx - 1) % 14));
        state.curTeeth = state.curLipColor = state.curEyelash = state.curEyeStyle = -1;
    }
    if (useMacroLayer) {
        state.curFace = state.faceLayer.testIndex; state.curMakeup = state.makeupLayer.testIndex;
        state.curEars = state.earsLayer.testIndex; state.curNose = state.noseLayer.testIndex;
        state.curBeard = state.beardLayer.testIndex; state.curGlasses = state.glassesLayer.testIndex;
        state.curHair = state.hairLayer.testIndex; state.curHat = state.hatLayer.testIndex;
        state.curBrows = state.browsLayer.testIndex; state.curBgScene = state.bgLayer.testIndex;
        state.curFxMask = state.manualFxMask;
    }
    if (!inspectingZip && state.curFace != state.activeProfileId) applyMaskOffsetProfile(cfg, state, state.curFace);
    bool isProf = (state.curFace >= PROFILE_FACE_START_INDEX);
    bool isDual = (isProf && state.dualProfileMode);
    g_active_face_mask.store(state.curFace, std::memory_order_relaxed);
    g_dual_profile_active.store(isDual, std::memory_order_relaxed);
    g_profile_face_split_x.store(state.faceLayer.splitOffsetX, std::memory_order_relaxed);
    g_profile_mouth_split_x.store(state.mouthSplitX, std::memory_order_relaxed);
    g_dual_mouth_mood_mode.store(0, std::memory_order_relaxed);

    auto syncDualPair = [&](uint8_t reqIdx, PaperDollLayer& lMain, PaperDollLayer& lDual) {
        if (isDual && reqIdx >= PROFILE_FACE_START_INDEX) {
            uint8_t oddIdx = (reqIdx % 2 == 1) ? reqIdx : static_cast<uint8_t>(reqIdx - 1);
            updatePaperDollLayer(renderer, activeAssetsDir, oddIdx, lMain);
            updatePaperDollLayer(renderer, activeAssetsDir, static_cast<uint8_t>(oddIdx + 1), lDual);
        } else {
            updatePaperDollLayer(renderer, activeAssetsDir, reqIdx, lMain);
            updatePaperDollLayer(renderer, activeAssetsDir, 0, lDual);
        }
    };

    updateParallaxBackdrop(renderer, activeAssetsDir, state.curBgScene, state.parallaxBg);
    syncDualPair(state.curFace, state.faceLayer, state.dualFaceLayer);
    syncDualPair(state.curMakeup, state.makeupLayer, state.dualMakeupLayer);
    syncDualPair(state.curEars, state.earsLayer, state.dualEarsLayer);
    syncDualPair(state.curBrows, state.browsLayer, state.dualBrowsLayer);
    syncDualPair(state.curNose, state.noseLayer, state.dualNoseLayer);
    syncDualPair(state.curBeard, state.beardLayer, state.dualBeardLayer);
    syncDualPair(state.curGlasses, state.glassesLayer, state.dualGlassesLayer);
    syncDualPair(state.curHair, state.hairLayer, state.dualHairLayer);
    syncDualPair(state.curHat, state.hatLayer, state.dualHatLayer);

    outRawDimmer = state.isDmxActive ? dmx_read_slot(0, getWrappedDmxIndex(cfg.start_channel, DMX_OFFSET_MASTER_DIMMER) + 1) : 255;
    state.masterDimmer = state.manualBlackout ? 255 : static_cast<uint8_t>(255 - outRawDimmer);

    setSystemAudioMonitorEnabled(!state.isDmxActive);
    if (state.isDmxActive) {
        updateAudioControlFromDmx(dmx_read_slot(0, getWrappedDmxIndex(cfg.start_channel, DMX_OFFSET_AUDIO_TRACK) + 1),
                                  dmx_read_slot(0, getWrappedDmxIndex(cfg.start_channel, DMX_OFFSET_AUDIO_VOLUME) + 1),
                                  dmx_read_slot(0, getWrappedDmxIndex(cfg.start_channel, DMX_OFFSET_AUDIO_TRIGGER) + 1));
    } else resetAudioOnDmxTimeout();
}
// ------------------------------------------------------------------------------
// 87. Step procedural eye/mouth animation and upload streaming ARGB textures
// ------------------------------------------------------------------------------
static void stepProceduralAnimation(
    const FaceAppConfig& cfg, AppRuntimeState& state, uint32_t frameStart,
    uint8_t rawMacro, bool dmxEditOverlay, SDL_Texture* eyesTexture, SDL_Texture* featuresTexture)
{
    eyeTft.fillScreen(TFT_BLACK);
    if (state.packBrowser.open && state.packBrowser.inspectMode) {
        runMacroById(std::max(1, state.manualTestAnim), frameStart);
    } else if (state.dmxMacroLocked) {
        runDmxMacro(rawMacro, frameStart, dmxEditOverlay);
    } else if (state.manualTestAnim > 0) {
        runMacroById(state.manualTestAnim, frameStart);
    } else if (state.manualMacroSlot >= 0) {
        int targetAnim = (state.manualMacroSlot == 0) ? 1 : ((state.manualMacroSlot - 1) % 64) + 1;
        auto it = cfg.macro_combos.find(state.manualMacroSlot);
        if (it != cfg.macro_combos.end() && it->second.enabled && it->second.anim_id > 0) targetAnim = it->second.anim_id;
        runMacroById(targetAnim, frameStart);
    } else {
        runAutonomousMacroShow(frameStart, getSavedSequentialMacroSlots(cfg), cfg.macro_combos);
        int autoSlot = getActiveMacroSlot();
        if (autoSlot != state.appliedMacroSlot && state.fadePhase == 0) {
            if (state.appliedMacroSlot < 0) {
                state.appliedMacroSlot = autoSlot;
                applyMacroComboToState(cfg, state, autoSlot, false);
                applyMaskOffsetProfile(cfg, state, state.faceLayer.testIndex);
            } else triggerMacroTransition(state, cfg, -1, autoSlot, nullptr);
        }
    }
    SDL_UpdateTexture(eyesTexture, nullptr, eyeTft.updateArgbBuffer(true), BASE_CANVAS_W * sizeof(uint32_t));
    SDL_UpdateTexture(featuresTexture, nullptr, tft.updateArgbBuffer(true), BASE_CANVAS_W * sizeof(uint32_t));
}

// ------------------------------------------------------------------------------
// 88. Initialize SDL2 renderer, DMX receiver thread and 50 FPS render loop
// ------------------------------------------------------------------------------
static void run_face_engine(FaceAppConfig& cfg)
{
    start_address = cfg.start_channel;
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) return;
    IMG_Init(IMG_INIT_PNG | IMG_INIT_JPG | IMG_INIT_WEBP);

    int dispIdx = resolveTargetDisplayIndex(cfg);
    SDL_Rect bounds{0, 0, BASE_CANVAS_W, BASE_CANVAS_H};
    SDL_GetDisplayBounds(dispIdx, &bounds);
    uint32_t flags = SDL_WINDOW_SHOWN | (cfg.fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : SDL_WINDOW_RESIZABLE);
    SDL_Window* window = SDL_CreateWindow("MakingFaces", SDL_WINDOWPOS_CENTERED_DISPLAY(dispIdx), SDL_WINDOWPOS_CENTERED_DISPLAY(dispIdx), BASE_CANVAS_W, BASE_CANVAS_H, flags);
    if (!window) { IMG_Quit(); SDL_Quit(); return; }
    SDL_ShowCursor(cfg.fullscreen ? SDL_DISABLE : SDL_ENABLE);

    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, cfg.smooth_scale ? "linear" : "nearest");
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer) renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

    SDL_Texture* eyesTexture     = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, BASE_CANVAS_W, BASE_CANVAS_H);
    SDL_Texture* featuresTexture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, BASE_CANVAS_W, BASE_CANVAS_H);
    SDL_SetTextureBlendMode(eyesTexture, SDL_BLENDMODE_BLEND);
    SDL_SetTextureBlendMode(featuresTexture, SDL_BLENDMODE_BLEND);
    initEyes();
    reloadDynamicEyeBank(cfg.assets_dir);
    initFaceEngine();
    initMacroEngine();
    initAudioEngine(cfg.audio_dir, cfg.audio_monitor);

    AppRuntimeState state{};
    state.isFullscreen = cfg.fullscreen;
    state.osdEnabled   = osd.enabled;
    initFxSpriteBank(renderer, cfg.assets_dir, state.fxBank);
    applyMacroComboToState(cfg, state, 0, false);
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
                lastPktMs.store(millis()); pktCounter.fetch_add(1);
            }
        }
    });

    uint32_t frameDelayMs = 1000 / std::max(1, cfg.fps), lastRateTick = millis(), lastPktCount = 0;

    while (g_running)
    {
        uint32_t frameStart = millis();
        SDL_Event ev;
        while (SDL_PollEvent(&ev)) {
            if (ev.type == SDL_QUIT) g_running = false;
            else if (ev.type == SDL_KEYDOWN) handleKeyboardEvent(window, ev.key.keysym.sym, static_cast<SDL_Keymod>(ev.key.keysym.mod), cfg, state);
            else if (ev.type == SDL_MOUSEBUTTONDOWN && ev.button.button == SDL_BUTTON_LEFT && ev.button.clicks == 2 && !state.flatImport.open) toggleWindowFullscreen(window, state.isFullscreen);
            handleFlatImportMouse(window, renderer, ev, state.flatImport);
        }

        SDL_ShowCursor((state.flatImport.open || !state.isFullscreen) ? SDL_ENABLE : SDL_DISABLE);

        if (frameStart - lastRateTick >= 1000) { uint32_t cur = pktCounter.load(); state.dmxPktRate = cur - lastPktCount; lastPktCount = cur; lastRateTick = frameStart; }
        updateDmxAndFadeState(cfg, state, frameStart, lastPktMs.load());
        if (state.isDmxActive) { std::lock_guard<std::mutex> lk(netMutex); g_dmx_universe = netBuf; }

        uint8_t rawMacro = 0, rawDimmer = 255; bool dmxEditOverlay = false;
        resolveActiveLayersAndAudio(window, renderer, cfg, state, frameStart, rawMacro, dmxEditOverlay, rawDimmer);
        if (!state.cameraCapture.open && !state.flatImport.open) {
            stepProceduralAnimation(cfg, state, frameStart, rawMacro, dmxEditOverlay, eyesTexture, featuresTexture);
        }

        bool hideAll   = state.cameraCapture.open || state.flatImport.open || isFaceHiddenByMacro() || (state.isDmxActive && rawDimmer == 0) || state.manualBlackout;
        bool hideMouth = isMouthHiddenByMacro() || state.manualHideMouth;
        bool hideBrows = isBrowsHiddenByMacro() || state.manualHideBrows;
        renderCompositedFrame(renderer, cfg, state, eyesTexture, featuresTexture, state.curFace, state.curBeard, state.curHair, state.curFxMask, hideAll, hideMouth, hideBrows, state.fadeAlpha, frameStart);

        uint32_t elapsed = millis() - frameStart;
        if (elapsed < frameDelayMs) SDL_Delay(frameDelayMs - elapsed);
    }

    if (dmxSock >= 0) close(dmxSock);
    if (netThread.joinable()) netThread.join();
    shutdownAudioEngine();
    closeCameraViewfinder(state.cameraCapture);
    destroyFlatImportItems(state.flatImport);
    destroyEyeBrowserEntries(state.eyeBrowser);
    destroyPackBrowserEntries(state.packBrowser);
    destroyFxSpriteBank(state.fxBank); destroyParallaxBackdrop(state.parallaxBg); destroyPaperDollLayers(state);
    SDL_DestroyTexture(featuresTexture); SDL_DestroyTexture(eyesTexture); SDL_DestroyRenderer(renderer); SDL_DestroyWindow(window);
    IMG_Quit(); SDL_Quit();
}

// ------------------------------------------------------------------------------
// 89. Unpack packs/default.zip if missing, create import/ & launch engine
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
        if (fs::exists("assets")) fs::remove_all("assets");
        if (fs::exists("makingfaces.ini")) fs::remove("makingfaces.ini");
        std::system("unzip -oq \"packs/default.zip\" -d .");
    }

    FaceAppConfig cfg;
    for (int i = 1; i < argc; ++i) {
        if (std::string(argv[i]) == "--config" && (i + 1) < argc) cfg.config_file = argv[i + 1];
    }
    loadIniConfig(cfg.config_file, cfg);

    static struct option longOptions[] = {
        {"universe", required_argument, 0, 'u'}, {"channel", required_argument, 0, 'c'}, {"display", required_argument, 0, 'd'},
        {"artnet", no_argument, 0, 'a'}, {"fps", required_argument, 0, 'f'}, {"fullscreen", no_argument, 0, 'F'},
        {"smooth", no_argument, 0, 's'}, {"version", no_argument, 0, 'v'}, {"audio-dir", required_argument, 0, 1001},
        {"assets-dir", required_argument, 0, 1002}, {"audio-monitor", required_argument, 0, 1003},
        {"config", required_argument, 0, 1004}, {"help", no_argument, 0, 'h'}, {0, 0, 0, 0}
    };

    int opt;
    while ((opt = getopt_long(argc, argv, "u:c:d:af:Fsvh", longOptions, nullptr)) != -1) {
        switch (opt) {
            case 'u': cfg.universe = std::stoi(optarg); break; case 'c': cfg.start_channel = std::stoi(optarg); break;
            case 'd': cfg.display = std::stoi(optarg); break; case 'a': cfg.proto = "artnet"; break;
            case 'f': cfg.fps = std::stoi(optarg); break; case 'F': cfg.fullscreen = true; break;
            case 's': cfg.smooth_scale = true; break;
            case 1001: cfg.audio_dir = optarg; break; case 1002: cfg.assets_dir = optarg; break;
            case 1003: cfg.audio_monitor = optarg; break; case 1004: cfg.config_file = optarg; break;
            default: return 0;
        }
    }
    run_face_engine(cfg);
    return 0;
}
