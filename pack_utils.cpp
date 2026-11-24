// ==============================================================================
// MakingFaces - Shared 3D SVG/PNG Asset & Theme Pack Generator Utilities
// Architecture designed by Magnus Herrlin 2026
// ==============================================================================

// ==============================================================================
// 1. INCLUDES AND INTERNAL HELPERS
// ==============================================================================
#include "pack_utils.h"
#include <opencv2/opencv.hpp>
#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>

namespace fs = std::filesystem;

namespace {

int g_nextMacroSlot = 0;

bool runCmd(const std::string& cmd)
{
    return (std::system(cmd.c_str()) == 0);
}

}

// ==============================================================================
// 2. SAFE XML/SVG ELEMENT CONSTRUCTORS
// ==============================================================================
std::string svgTag(const std::string& tag, const std::string& attrs, bool selfClose)
{
    std::string out;
    out.push_back('\x3c');
    out += tag;
    if (!attrs.empty()) {
        out.push_back(' ');
        out += attrs;
    }
    if (selfClose) out += " /";
    out.push_back('\x3e');
    return out;
}

std::string svgClose(const std::string& tag)
{
    std::string out;
    out.push_back('\x3c');
    out.push_back('/');
    out += tag;
    out.push_back('\x3e');
    return out;
}

std::string wrapSvgCanvas(const std::string& body, const std::string& defs, int w, int h)
{
    std::string ns = std::string("http://") + "www.w3.org/2000/svg";
    std::ostringstream ss;
    ss << svgTag("svg", "xmlns=\"" + ns + "\" width=\"" + std::to_string(w) + "\" height=\"" + std::to_string(h) + "\" viewBox=\"0 0 " + std::to_string(w) + " " + std::to_string(h) + "\"", false) << "\n";
    if (!defs.empty()) {
        ss << svgTag("defs", "", false) << "\n" << defs << "\n" << svgClose("defs") << "\n";
    }
    ss << body << "\n" << svgClose("svg") << "\n";
    return ss.str();
}

std::string getStandardEyeCutoutMask(void)
{
    return svgTag("rect", "x=\"0\" y=\"0\" width=\"600\" height=\"800\" fill=\"white\"") + "\n" +
           svgTag("ellipse", "cx=\"190\" cy=\"356\" rx=\"64\" ry=\"45\" fill=\"black\"") + "\n" +
           svgTag("ellipse", "cx=\"410\" cy=\"356\" rx=\"64\" ry=\"45\" fill=\"black\"");
}

// ==============================================================================
// 3. ASSET SLOT INDEXING, RASTERIZATION & GAUSSIAN DOF BLUR
// ==============================================================================
int nextAssetIdx(const std::string& folder)
{
    fs::path dir = fs::path("assets") / folder;
    fs::create_directories(dir);
    int maxId = 0;
    for (const auto& entry : fs::directory_iterator(dir)) {
        if (!entry.is_regular_file()) continue;
        std::string stem = entry.path().stem().string();
        if (stem.size() >= 3 && std::isdigit(stem.at(0)) && std::isdigit(stem.at(1)) && std::isdigit(stem.at(2))) {
            int id = std::stoi(stem.substr(0, 3));
            if (id < 127) maxId = std::max(maxId, id);
        }
    }
    return maxId + 1;
}

bool exportSvgToPng(const std::string& relPathNoExt, const std::string& svgXml, int w, int h, float blurSigma)
{
    fs::path outPng = fs::path("assets") / (relPathNoExt + ".png");
    if (outPng.has_parent_path()) fs::create_directories(outPng.parent_path());

    std::string tmpSvg = "/tmp/mf_pack_gen.svg";
    {
        std::ofstream f(tmpSvg, std::ios::binary);
        if (!f.is_open()) return false;
        f << svgXml;
    }

    std::string cmd = "rsvg-convert -w " + std::to_string(w) + " -h " + std::to_string(h) + " \"" + tmpSvg + "\" -o \"" + outPng.string() + "\"";
    bool ok = runCmd(cmd) && fs::exists(outPng) && fs::file_size(outPng) > 0;
    fs::remove(tmpSvg);
    if (!ok) {
        std::cerr << "[WARN] rsvg-convert failed for " << outPng.string() << "\n";
        return false;
    }

    if (blurSigma > 0.1f) {
        cv::Mat img = cv::imread(outPng.string(), cv::IMREAD_UNCHANGED);
        if (!img.empty()) {
            cv::GaussianBlur(img, img, cv::Size(0, 0), blurSigma, blurSigma);
            cv::imwrite(outPng.string(), img);
        }
    }
    return true;
}

int saveSvgAsset(const std::string& folder, const std::string& svgXml, int w, int h, float blurSigma)
{
    int idx = nextAssetIdx(folder);
    saveSvgAssetAt(folder, idx, svgXml, w, h, blurSigma);
    return idx;
}

void saveSvgAssetAt(const std::string& folder, int idx, const std::string& svgXml, int w, int h, float blurSigma)
{
    std::array<char, 48> rel{};
    std::snprintf(rel.data(), rel.size(), "%s/%03d", folder.c_str(), idx);
    exportSvgToPng(rel.data(), svgXml, w, h, blurSigma);
}

// ==============================================================================
// 4. INI CONFIGURATION, DEFAULT BROWS/FX BANKS & ZIP ARCHIVER
// ==============================================================================
void ensureBaseIni(const std::string& iniPath)
{
    if (fs::exists("assets")) fs::remove_all("assets");
    if (fs::exists(iniPath)) fs::remove(iniPath);
    g_nextMacroSlot = 0;

    static const std::array<const char*, 11> kDirs = {
        "face", "makeup", "ears", "nose", "brows", "beard", "glasses", "hair", "hat", "bg", "fx"
    };
    for (const char* d : kDirs) fs::create_directories(fs::path("assets") / d);

    std::ofstream out(iniPath);
    out << "[network]\nproto=sacn\nuniverse=1\nstart_channel=1\n\n"
        << "[display]\ndisplay=1\nfps=50\nfullscreen=true\nsmooth_scale=false\nfade_ms=1200\nsave_hold_ms=5000\n\n"
        << "[paths]\nassets_dir=assets\naudio_dir=audio\naudio_monitor=Monitor\n\n"
        << "[offsets_default]\neye_left_x=0\neye_left_y=10\neye_right_x=0\neye_right_y=10\n"
        << "mouth_x=0\nmouth_y=0\nmouth_split=0\nnose_x=0\nnose_y=0\n"
        << "glasses_x=0\nglasses_y=0\nglasses_split=0\nbrows_x=0\nbrows_y=-10\nbrows_split=0\n"
        << "beard_x=0\nbeard_y=-4\nbeard_split=0\nface_x=0\nface_y=6\n"
        << "ears_x=0\nears_y=0\nears_split=0\nhair_x=0\nhair_y=0\nhat_x=0\nhat_y=0\n"
        << "eye_scale=1\nmouth_scale=1\n";
}

void appendFaceOffsets(int faceId, const std::map<std::string, std::string>& kv, const std::string& iniPath)
{
    std::ofstream out(iniPath, std::ios::app);
    std::array<char, 32> hdr{};
    std::snprintf(hdr.data(), hdr.size(), "\n[offsets_face_%03d]\n", faceId);
    out << hdr.data();
    for (const auto& p : kv) out << p.first << "=" << p.second << "\n";
}

void appendMacroCombo(const PackMacroSpec& m, const std::string& iniPath)
{
    std::ofstream out(iniPath, std::ios::app);
    out << "\n[macro_combo_" << g_nextMacroSlot++ << "]\n"
        << "face=" << m.face << "\nmakeup=" << m.makeup << "\nears=" << m.ears << "\nnose=" << m.nose
        << "\nbeard=" << m.beard << "\nglasses=" << m.glasses << "\nhair=" << m.hair << "\nhat=" << m.hat
        << "\nbrows=" << m.brows << "\nbg_scene=" << m.bg_scene << "\nfx_mask=" << m.fx_mask
        << "\nteeth=" << m.teeth << "\nlip_color=" << m.lip_color << "\neyelash=" << m.eyelash
        << "\neye_style=" << m.eye_style << "\niris=" << m.iris << "\neye_bank=" << m.eye_bank
        << "\nanim_id=" << m.anim_id << "\ndual_profile=" << (m.dual_profile ? 1 : 0) << "\n";
}

// ------------------------------------------------------------------------------
// 5. Generate default eyebrow bank (001..006)
// ------------------------------------------------------------------------------
void generateDefaultBrowsBank(void)
{
    exportSvgToPng("brows/001", wrapSvgCanvas(
        svgTag("path", "d=\"M 118,284 Q 190,256 266,278 Q 190,276 118,296 Z\" fill=\"#2a180c\"") +
        svgTag("path", "d=\"M 482,284 Q 410,256 334,278 Q 410,276 482,296 Z\" fill=\"#2a180c\"")));
    exportSvgToPng("brows/002", wrapSvgCanvas(
        svgTag("path", "d=\"M 112,288 Q 190,248 272,276 Q 192,274 112,304 Z\" fill=\"#140d08\"") +
        svgTag("path", "d=\"M 488,288 Q 410,248 328,276 Q 408,274 488,304 Z\" fill=\"#140d08\"")));
    exportSvgToPng("brows/003", wrapSvgCanvas(
        svgTag("path", "d=\"M 122,276 Q 192,238 262,272\" fill=\"none\" stroke=\"#3b2212\" stroke-width=\"11\" stroke-linecap=\"round\"") +
        svgTag("path", "d=\"M 478,276 Q 408,238 338,272\" fill=\"none\" stroke=\"#3b2212\" stroke-width=\"11\" stroke-linecap=\"round\"")));
    exportSvgToPng("brows/004", wrapSvgCanvas(
        svgTag("path", "d=\"M 118,266 Q 192,262 270,294 Q 194,280 118,282 Z\" fill=\"#1f1209\"") +
        svgTag("path", "d=\"M 482,266 Q 408,262 330,294 Q 406,280 482,282 Z\" fill=\"#1f1209\"")));
    exportSvgToPng("brows/005", wrapSvgCanvas(
        svgTag("path", "d=\"M 118,284 Q 190,256 266,278 Q 190,276 118,296 Z\" fill=\"#94a3b8\"") +
        svgTag("path", "d=\"M 482,284 Q 410,256 334,278 Q 410,276 482,296 Z\" fill=\"#94a3b8\"")));
    exportSvgToPng("brows/006", wrapSvgCanvas(
        svgTag("path", "d=\"M 118,284 Q 190,256 266,278 Q 190,276 118,296 Z\" fill=\"#c2410c\"") +
        svgTag("path", "d=\"M 482,284 Q 410,256 334,278 Q 410,276 482,296 Z\" fill=\"#c2410c\"")));
}

// ------------------------------------------------------------------------------
// 6. Generate default foreground FX critters, full-height cobweb, fog & fire
// ------------------------------------------------------------------------------
void generateDefaultFxBank(void)
{
    exportSvgToPng("fx/fly", wrapSvgCanvas(
        svgTag("ellipse", "cx=\"48\" cy=\"32\" rx=\"16\" ry=\"9\" transform=\"rotate(-25 48 32)\" fill=\"#e2e8f0\" opacity=\"0.78\"") +
        svgTag("ellipse", "cx=\"72\" cy=\"32\" rx=\"16\" ry=\"9\" transform=\"rotate(25 72 32)\" fill=\"#e2e8f0\" opacity=\"0.78\"") +
        svgTag("ellipse", "cx=\"60\" cy=\"54\" rx=\"14\" ry=\"22\" fill=\"#0f172a\"") +
        svgTag("circle", "cx=\"53\" cy=\"36\" r=\"6\" fill=\"#dc2626\"") +
        svgTag("circle", "cx=\"67\" cy=\"36\" r=\"6\" fill=\"#dc2626\""), "", 120, 120));

    exportSvgToPng("fx/bat", wrapSvgCanvas(
        svgTag("path", "d=\"M 60,48 Q 28,16 4,42 Q 22,58 34,50 Q 46,62 60,55 Z\" fill=\"#3f3f46\" stroke=\"#09090b\" stroke-width=\"2.5\"") +
        svgTag("path", "d=\"M 60,48 Q 92,16 116,42 Q 98,58 86,50 Q 74,62 60,55 Z\" fill=\"#3f3f46\" stroke=\"#09090b\" stroke-width=\"2.5\"") +
        svgTag("ellipse", "cx=\"60\" cy=\"54\" rx=\"10\" ry=\"16\" fill=\"#27272a\" stroke=\"#09090b\" stroke-width=\"2.5\"") +
        svgTag("polygon", "points=\"52,42 55,30 59,41\" fill=\"#27272a\"") +
        svgTag("polygon", "points=\"68,42 65,30 61,41\" fill=\"#27272a\"") +
        svgTag("circle", "cx=\"56\" cy=\"48\" r=\"2.4\" fill=\"#ef4444\"") +
        svgTag("circle", "cx=\"64\" cy=\"48\" r=\"2.4\" fill=\"#ef4444\""), "", 120, 120));

    exportSvgToPng("fx/butterfly", wrapSvgCanvas(
        svgTag("path", "d=\"M 60,50 C 20,10 5,45 35,65 C 10,80 30,98 60,72 Z\" fill=\"#38bdf8\" stroke=\"#0f172a\" stroke-width=\"3\"") +
        svgTag("path", "d=\"M 60,50 C 100,10 115,45 85,65 C 110,80 90,98 60,72 Z\" fill=\"#38bdf8\" stroke=\"#0f172a\" stroke-width=\"3\""), "", 120, 120));

    exportSvgToPng("fx/mosquito", wrapSvgCanvas(
        svgTag("ellipse", "cx=\"48\" cy=\"32\" rx=\"14\" ry=\"8\" fill=\"#ffffff\" opacity=\"0.78\"") +
        svgTag("ellipse", "cx=\"72\" cy=\"32\" rx=\"14\" ry=\"8\" fill=\"#ffffff\" opacity=\"0.78\"") +
        svgTag("ellipse", "cx=\"60\" cy=\"54\" rx=\"18\" ry=\"13\" fill=\"#facc15\" stroke=\"#0f172a\" stroke-width=\"3\""), "", 120, 120));

    exportSvgToPng("fx/bird", wrapSvgCanvas(
        svgTag("path", "d=\"M 60,44 Q 32,16 4,38 Q 22,52 34,68 Q 48,54 60,64 Q 72,54 86,68 Q 98,52 116,38 Q 88,16 60,44 Z\" fill=\"#0284c7\""), "", 120, 120));

    exportSvgToPng("fx/wasp", wrapSvgCanvas(
        svgTag("ellipse", "cx=\"60\" cy=\"54\" rx=\"18\" ry=\"13\" fill=\"#facc15\" stroke=\"#0f172a\" stroke-width=\"3\"") +
        svgTag("line", "x1=\"54\" y1=\"42\" x2=\"54\" y2=\"66\" stroke=\"#0f172a\" stroke-width=\"4\"") +
        svgTag("line", "x1=\"66\" y1=\"42\" x2=\"66\" y2=\"66\" stroke=\"#0f172a\" stroke-width=\"4\""), "", 120, 120));

    exportSvgToPng("fx/spider", wrapSvgCanvas(
        svgTag("path", "d=\"M 46,56 Q 18,32 10,58 M 44,62 Q 14,56 8,82 M 46,68 Q 18,78 16,104 M 50,74 Q 28,94 28,114 M 74,56 Q 102,32 110,58 M 76,62 Q 106,56 112,82 M 74,68 Q 102,78 104,104 M 70,74 Q 92,94 92,114\" fill=\"none\" stroke=\"#090d16\" stroke-width=\"5\" stroke-linecap=\"round\"") +
        svgTag("ellipse", "cx=\"60\" cy=\"54\" rx=\"20\" ry=\"24\" fill=\"#0f172a\" stroke=\"#020617\" stroke-width=\"3\"") +
        svgTag("circle", "cx=\"60\" cy=\"82\" r=\"13\" fill=\"#1e293b\"") +
        svgTag("polygon", "points=\"54,48 66,48 60,56 66,64 54,64 60,56\" fill=\"#ef4444\"") +
        svgTag("circle", "cx=\"55\" cy=\"90\" r=\"2.8\" fill=\"#ef4444\"") +
        svgTag("circle", "cx=\"65\" cy=\"90\" r=\"2.8\" fill=\"#ef4444\""), "", 120, 120));

    exportSvgToPng("fx/ufo", wrapSvgCanvas(
        svgTag("ellipse", "cx=\"60\" cy=\"65\" rx=\"48\" ry=\"14\" fill=\"#64748b\"") +
        svgTag("ellipse", "cx=\"60\" cy=\"54\" rx=\"24\" ry=\"14\" fill=\"#00f5d4\""), "", 120, 120));

    std::ostringstream webSs;
    constexpr int kSpokes = 11;
    for (int s = 0; s < kSpokes; ++s)
    {
        float a = (static_cast<float>(s) / static_cast<float>(kSpokes - 1)) * 1.5707963f;
        int ex = static_cast<int>(std::cos(a) * 820.0f);
        int ey = static_cast<int>(std::sin(a) * 820.0f);
        webSs << svgTag("line", "x1=\"0\" y1=\"0\" x2=\"" + std::to_string(ex) + "\" y2=\"" + std::to_string(ey) + "\" stroke=\"#f1f5f9\" stroke-width=\"1.6\" opacity=\"0.58\"");
    }
    for (int ring = 1; ring <= 14; ++ring)
    {
        float r = ring * 56.0f;
        std::ostringstream d;
        for (int s = 0; s < kSpokes - 1; ++s)
        {
            float a1 = (static_cast<float>(s) / static_cast<float>(kSpokes - 1)) * 1.5707963f;
            float a2 = (static_cast<float>(s + 1) / static_cast<float>(kSpokes - 1)) * 1.5707963f;
            float am = 0.5f * (a1 + a2);
            int x1 = static_cast<int>(std::cos(a1) * r), y1 = static_cast<int>(std::sin(a1) * r);
            int xm = static_cast<int>(std::cos(am) * (r * 0.88f)), ym = static_cast<int>(std::sin(am) * (r * 0.88f));
            int x2 = static_cast<int>(std::cos(a2) * r), y2 = static_cast<int>(std::sin(a2) * r);
            if (s == 0) d << "M " << x1 << "," << y1 << " ";
            d << "Q " << xm << "," << ym << " " << x2 << "," << y2 << " ";
        }
        webSs << svgTag("path", "d=\"" + d.str() + "\" fill=\"none\" stroke=\"#f1f5f9\" stroke-width=\"1.5\" opacity=\"0.52\"");
    }
    exportSvgToPng("fx/cobweb", wrapSvgCanvas(webSs.str()));
    exportSvgToPng("fx/ghost", wrapSvgCanvas(
        svgTag("path", "d=\"M 210,460 Q 210,255 300,255 Q 390,255 390,460 Q 360,435 330,460 Q 300,435 270,460 Q 240,435 210,460 Z\" fill=\"#f8fafc\" opacity=\"0.88\"") +
        svgTag("ellipse", "cx=\"274\" cy=\"336\" rx=\"13\" ry=\"17\" fill=\"#0f172a\"") +
        svgTag("ellipse", "cx=\"326\" cy=\"336\" rx=\"13\" ry=\"17\" fill=\"#0f172a\"") +
        svgTag("circle", "cx=\"277\" cy=\"331\" r=\"4.5\" fill=\"#38bdf8\"") +
        svgTag("circle", "cx=\"329\" cy=\"331\" r=\"4.5\" fill=\"#38bdf8\"") +
        svgTag("ellipse", "cx=\"300\" cy=\"384\" rx=\"15\" ry=\"22\" fill=\"#0f172a\"")));

    exportSvgToPng("fx/fog_1", wrapSvgCanvas(
        svgTag("ellipse", "cx=\"140\" cy=\"520\" rx=\"260\" ry=\"95\" fill=\"#e2e8f0\" opacity=\"0.22\"") +
        svgTag("ellipse", "cx=\"460\" cy=\"620\" rx=\"280\" ry=\"110\" fill=\"#cbd5e1\" opacity=\"0.26\"") +
        svgTag("ellipse", "cx=\"210\" cy=\"745\" rx=\"340\" ry=\"125\" fill=\"#f1f5f9\" opacity=\"0.34\"") +
        svgTag("ellipse", "cx=\"430\" cy=\"825\" rx=\"340\" ry=\"125\" fill=\"#e2e8f0\" opacity=\"0.40\"")), 600, 800, 18.0f);

    exportSvgToPng("fx/fog_2", wrapSvgCanvas(
        svgTag("ellipse", "cx=\"180\" cy=\"310\" rx=\"290\" ry=\"105\" fill=\"#94a3b8\" opacity=\"0.22\"") +
        svgTag("ellipse", "cx=\"440\" cy=\"450\" rx=\"310\" ry=\"120\" fill=\"#cbd5e1\" opacity=\"0.26\"") +
        svgTag("ellipse", "cx=\"160\" cy=\"630\" rx=\"340\" ry=\"130\" fill=\"#94a3b8\" opacity=\"0.30\"") +
        svgTag("ellipse", "cx=\"430\" cy=\"790\" rx=\"350\" ry=\"135\" fill=\"#cbd5e1\" opacity=\"0.36\"")), 600, 800, 22.0f);

    std::string fireDefs =
        svgTag("linearGradient", "id=\"flmOut\" x1=\"0%\" y1=\"100%\" x2=\"0%\" y2=\"0%\"", false) +
        svgTag("stop", "offset=\"0%\" stop-color=\"#991b1b\" stop-opacity=\"0.94\"") +
        svgTag("stop", "offset=\"45%\" stop-color=\"#ea580c\" stop-opacity=\"0.86\"") +
        svgTag("stop", "offset=\"100%\" stop-color=\"#facc15\" stop-opacity=\"0.78\"") + svgClose("linearGradient") +
        svgTag("linearGradient", "id=\"flmIn\" x1=\"0%\" y1=\"100%\" x2=\"0%\" y2=\"0%\"", false) +
        svgTag("stop", "offset=\"0%\" stop-color=\"#f97316\" stop-opacity=\"0.95\"") +
        svgTag("stop", "offset=\"62%\" stop-color=\"#fde047\" stop-opacity=\"0.90\"") +
        svgTag("stop", "offset=\"100%\" stop-color=\"#ffffff\" stop-opacity=\"0.85\"") + svgClose("linearGradient");

    auto makeCurlyFlames = [&](int phase) {
        std::ostringstream fs;
        fs << svgTag("rect", "x=\"-20\" y=\"735\" width=\"640\" height=\"85\" fill=\"url(#flmOut)\"");
        for (int k = 0; k < 15; ++k)
        {
            int bx = -20 + k * 46;
            int h  = 165 + ((k * 53 + phase * 37) % 135);
            int sway1 = ((k + phase) % 2 == 0) ? 34 : -34;
            int sway2 = -sway1;
            int tipX  = bx + sway1 / 2;
            int tipY  = 785 - h;
            fs << svgTag("path", "d=\"M " + std::to_string(bx - 28) + ",810 C " + std::to_string(bx - 38) + "," + std::to_string(785 - h / 3) + " " + std::to_string(bx + sway1) + "," + std::to_string(785 - (h * 2) / 3) + " " + std::to_string(tipX) + "," + std::to_string(tipY) + " C " + std::to_string(bx + sway2) + "," + std::to_string(785 - (h * 2) / 3) + " " + std::to_string(bx + 36) + "," + std::to_string(785 - h / 3) + " " + std::to_string(bx + 28) + ",810 Z\" fill=\"url(#flmOut)\"");
            int ih = (h * 64) / 100;
            fs << svgTag("path", "d=\"M " + std::to_string(bx - 16) + ",810 C " + std::to_string(bx - 22) + "," + std::to_string(795 - ih / 3) + " " + std::to_string(bx + sway1 / 2) + "," + std::to_string(795 - (ih * 2) / 3) + " " + std::to_string(bx + sway1 / 4) + "," + std::to_string(795 - ih) + " C " + std::to_string(bx + sway2 / 2) + "," + std::to_string(795 - (ih * 2) / 3) + " " + std::to_string(bx + 20) + "," + std::to_string(795 - ih / 3) + " " + std::to_string(bx + 16) + ",810 Z\" fill=\"url(#flmIn)\"");
        }
        return fs.str();
    };

    exportSvgToPng("fx/fire_1", wrapSvgCanvas(makeCurlyFlames(0), fireDefs), 600, 800, 2.0f);
    exportSvgToPng("fx/fire_2", wrapSvgCanvas(makeCurlyFlames(1), fireDefs), 600, 800, 2.0f);
    exportSvgToPng("fx/lightning_1", wrapSvgCanvas(
        svgTag("polyline", "points=\"160,0 130,210 195,230 140,480 210,510 155,780\" fill=\"none\" stroke=\"#38bdf8\" stroke-width=\"14\" opacity=\"0.45\"") +
        svgTag("polyline", "points=\"160,0 130,210 195,230 140,480 210,510 155,780\" fill=\"none\" stroke=\"#ffffff\" stroke-width=\"5\"")));

    exportSvgToPng("fx/lightning_2", wrapSvgCanvas(
        svgTag("polyline", "points=\"445,0 475,190 415,220 465,450 405,490 455,770\" fill=\"none\" stroke=\"#a855f7\" stroke-width=\"14\" opacity=\"0.45\"") +
        svgTag("polyline", "points=\"445,0 475,190 415,220 465,450 405,490 455,770\" fill=\"none\" stroke=\"#ffffff\" stroke-width=\"5\"")));

    std::string snakeDefs =
        svgTag("linearGradient", "id=\"snkGrad\" x1=\"0%\" y1=\"0%\" x2=\"100%\" y2=\"100%\"", false) +
        svgTag("stop", "offset=\"0%\" stop-color=\"#84cc16\"") +
        svgTag("stop", "offset=\"55%\" stop-color=\"#15803d\"") +
        svgTag("stop", "offset=\"100%\" stop-color=\"#052e16\"") + svgClose("linearGradient");

    std::string snakeBody =
        svgTag("path", "d=\"M 80,0 C 40,65 130,115 70,175 C 25,220 115,255 82,295\" fill=\"none\" stroke=\"#052e16\" stroke-width=\"24\" stroke-linecap=\"round\"") +
        svgTag("path", "d=\"M 80,0 C 40,65 130,115 70,175 C 25,220 115,255 82,295\" fill=\"none\" stroke=\"url(#snkGrad)\" stroke-width=\"18\" stroke-linecap=\"round\"") +
        svgTag("path", "d=\"M 80,0 C 40,65 130,115 70,175 C 25,220 115,255 82,295\" fill=\"none\" stroke=\"#bef264\" stroke-width=\"5\" stroke-dasharray=\"8 12\" stroke-linecap=\"round\" opacity=\"0.78\"") +
        svgTag("ellipse", "cx=\"80\" cy=\"302\" rx=\"18\" ry=\"24\" fill=\"url(#snkGrad)\" stroke=\"#052e16\" stroke-width=\"3\"") +
        svgTag("circle", "cx=\"72\" cy=\"304\" r=\"4\" fill=\"#facc15\"") +
        svgTag("circle", "cx=\"88\" cy=\"304\" r=\"4\" fill=\"#facc15\"") +
        svgTag("path", "d=\"M 80,324 L 80,348 L 72,358 M 80,348 L 88,358\" fill=\"none\" stroke=\"#ef4444\" stroke-width=\"3\" stroke-linecap=\"round\"");

    exportSvgToPng("fx/snake", wrapSvgCanvas(snakeBody, snakeDefs, 160, 370), 160, 370, 0.0f);

    std::string mouseDefs =
        svgTag("radialGradient", "id=\"msBody\" cx=\"45%\" cy=\"35%\" r=\"65%\"", false) +
        svgTag("stop", "offset=\"0%\" stop-color=\"#cbd5e1\"") +
        svgTag("stop", "offset=\"60%\" stop-color=\"#64748b\"") +
        svgTag("stop", "offset=\"100%\" stop-color=\"#1e293b\"") + svgClose("radialGradient");

    std::string mouseBody =
        svgTag("path", "d=\"M 42,58 Q 14,42 8,24 Q 4,14 16,16\" fill=\"none\" stroke=\"#f472b6\" stroke-width=\"4.5\" stroke-linecap=\"round\"") +
        svgTag("ellipse", "cx=\"82\" cy=\"54\" rx=\"42\" ry=\"24\" fill=\"url(#msBody)\"") +
        svgTag("circle", "cx=\"102\" cy=\"34\" r=\"15\" fill=\"#64748b\"") +
        svgTag("circle", "cx=\"104\" cy=\"34\" r=\"9\" fill=\"#fda4af\"") +
        svgTag("circle", "cx=\"116\" cy=\"38\" r=\"14\" fill=\"#94a3b8\"") +
        svgTag("circle", "cx=\"118\" cy=\"38\" r=\"8\" fill=\"#fda4af\"") +
        svgTag("circle", "cx=\"122\" cy=\"48\" r=\"4.5\" fill=\"#090d16\"") +
        svgTag("circle", "cx=\"123\" cy=\"47\" r=\"1.5\" fill=\"#ffffff\"") +
        svgTag("circle", "cx=\"138\" cy=\"56\" r=\"5.5\" fill=\"#fb7185\"") +
        svgTag("path", "d=\"M 128,54 L 146,46 M 128,58 L 148,58 M 128,62 L 145,68\" stroke=\"#f8fafc\" stroke-width=\"1.8\" stroke-linecap=\"round\"");

    exportSvgToPng("fx/mouse", wrapSvgCanvas(mouseBody, mouseDefs, 150, 90), 150, 90, 0.0f);
}
// ==============================================================================
// 7. 3D SIDE-PROFILE PRIMITIVE BUILDERS (127+ LEFT & RIGHT PROFILES)
// ==============================================================================

// ------------------------------------------------------------------------------
// 8. Build 3D side-profile head with neck at screen edge and rounded 3D jaw
// ------------------------------------------------------------------------------
std::string build3dProfileHeadSvg(const std::string& tag, const std::string& side, const std::string& f1, const std::string& f2, const std::string& f3, const std::string& mh, const std::string& mm, const std::string& md)
{
    bool isRight = (side == "right");
    std::string defs =
        svgTag("radialGradient", "id=\"pf" + tag + "\" cx=\"55%\" cy=\"32%\" r=\"72%\"", false) +
        svgTag("stop", "offset=\"0%\" stop-color=\"" + f1 + "\"") +
        svgTag("stop", "offset=\"55%\" stop-color=\"" + f2 + "\"") +
        svgTag("stop", "offset=\"100%\" stop-color=\"" + f3 + "\"") + svgClose("radialGradient") +
        svgTag("radialGradient", "id=\"pm" + tag + "\" cx=\"52%\" cy=\"42%\" r=\"65%\"", false) +
        svgTag("stop", "offset=\"0%\" stop-color=\"" + mh + "\"") +
        svgTag("stop", "offset=\"60%\" stop-color=\"" + mm + "\"") +
        svgTag("stop", "offset=\"100%\" stop-color=\"" + md + "\"") + svgClose("radialGradient") +
        svgTag("mask", "id=\"pmk" + tag + "\"", false) +
        svgTag("rect", "x=\"0\" y=\"0\" width=\"600\" height=\"800\" fill=\"white\"") +
        svgTag("ellipse", "cx=\"185\" cy=\"356\" rx=\"52\" ry=\"38\" fill=\"black\"") +
        svgClose("mask");

    std::string xf = isRight ? " transform=\"translate(600,0) scale(-1,1)\"" : "";
    std::string body =
        svgTag("g", "mask=\"url(#pmk" + tag + ")\"" + xf, false) +
        svgTag("path", "d=\"M 0,175 L 82,175 C 185,175 235,245 245,345 C 258,385 288,405 298,438 C 308,472 288,508 248,535 C 262,548 268,575 252,595 C 242,618 264,650 256,685 C 248,722 218,754 172,746 C 122,736 78,765 0,795 Z\" fill=\"url(#pf" + tag + ")\"") +
        svgTag("path", "d=\"M 248,385 C 195,355 115,385 98,485 C 82,595 128,718 196,736 C 242,724 260,672 252,595 C 265,555 262,465 248,385 Z\" fill=\"url(#pm" + tag + ")\"") +
        svgTag("ellipse", "cx=\"185\" cy=\"352\" rx=\"56\" ry=\"42\" fill=\"#000000\" opacity=\"0.28\"") +
        svgClose("g");
    return wrapSvgCanvas(body, defs);
}

// ------------------------------------------------------------------------------
// 9. Build 3D side-profile ear aligned near the outer edge of the profile head
// ------------------------------------------------------------------------------
std::string build3dProfileEarsSvg(const std::string& tag, const std::string& side, const std::string& hi, const std::string& mid, const std::string& deep, const std::string& inHi, const std::string& inDeep, int earR)
{
    bool isRight = (side == "right");
    std::string defs =
        svgTag("radialGradient", "id=\"peo" + tag + "\" cx=\"42%\" cy=\"32%\" r=\"68%\"", false) +
        svgTag("stop", "offset=\"0%\" stop-color=\"" + hi + "\"") +
        svgTag("stop", "offset=\"60%\" stop-color=\"" + mid + "\"") +
        svgTag("stop", "offset=\"100%\" stop-color=\"" + deep + "\"") + svgClose("radialGradient") +
        svgTag("radialGradient", "id=\"pei" + tag + "\" cx=\"55%\" cy=\"55%\" r=\"62%\"", false) +
        svgTag("stop", "offset=\"0%\" stop-color=\"" + inHi + "\"") +
        svgTag("stop", "offset=\"100%\" stop-color=\"" + inDeep + "\"") + svgClose("radialGradient");

    int rx   = static_cast<int>(earR * 0.78f);
    int rInX = static_cast<int>(rx * 0.62f);
    int rInY = static_cast<int>(earR * 0.62f);
    int cx   = isRight ? 542 : 58;
    int cxIn = isRight ? 539 : 61;
    std::string body =
        svgTag("ellipse", "cx=\"" + std::to_string(cx) + "\" cy=\"385\" rx=\"" + std::to_string(rx) + "\" ry=\"" + std::to_string(earR) + "\" fill=\"url(#peo" + tag + ")\"") +
        svgTag("ellipse", "cx=\"" + std::to_string(cxIn) + "\" cy=\"385\" rx=\"" + std::to_string(rInX) + "\" ry=\"" + std::to_string(rInY) + "\" fill=\"url(#pei" + tag + ")\"");
    return wrapSvgCanvas(body, defs);
}

// ------------------------------------------------------------------------------
// 10. Build 3D side-profile nose pointing inward toward the screen center
// ------------------------------------------------------------------------------
std::string build3dProfileNoseSvg(const std::string& tag, const std::string& side, const std::string& hi, const std::string& mid, const std::string& deep)
{
    bool isRight = (side == "right");
    std::string defs =
        svgTag("radialGradient", "id=\"pnt" + tag + "\" cx=\"35%\" cy=\"35%\" r=\"65%\"", false) +
        svgTag("stop", "offset=\"0%\" stop-color=\"" + hi + "\"") +
        svgTag("stop", "offset=\"60%\" stop-color=\"" + mid + "\"") +
        svgTag("stop", "offset=\"100%\" stop-color=\"" + deep + "\"") + svgClose("radialGradient");

    int cx = isRight ? 322 : 278;
    int nx = isRight ? 316 : 284;
    std::string body =
        svgTag("ellipse", "cx=\"" + std::to_string(cx) + "\" cy=\"468\" rx=\"34\" ry=\"26\" fill=\"url(#pnt" + tag + ")\"") +
        svgTag("ellipse", "cx=\"" + std::to_string(nx) + "\" cy=\"476\" rx=\"10\" ry=\"7\" fill=\"#090605\"");
    return wrapSvgCanvas(body, defs);
}

// ------------------------------------------------------------------------------
// 11. Remove temporary SVG files and zip assets/ + makingfaces.ini into packs/
// ------------------------------------------------------------------------------
void cleanSvgAndSaveZip(const std::string& packName, const std::string& packsDir)
{
    fs::create_directories(packsDir);
    for (auto it = fs::recursive_directory_iterator("assets"); it != fs::recursive_directory_iterator(); ++it)
    {
        if (it->is_regular_file() && it->path().extension() == ".svg")
        {
            fs::remove(it->path());
        }
    }

    std::string zipOut = packsDir + "/" + packName + ".zip";
    fs::remove(zipOut);
    std::string cmd = "zip -qr \"" + zipOut + "\" assets makingfaces.ini";
    int rc = std::system(cmd.c_str());
    if (rc == 0)
    {
        fs::copy_file(zipOut, packsDir + "/default.zip", fs::copy_options::overwrite_existing);
        std::cout << "[PACKED] Created " << zipOut << " & updated " << packsDir << "/default.zip\n";
    }
}
