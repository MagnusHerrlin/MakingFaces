// ==============================================================================
// MakingFaces - Theme Pack 1 (C++17): 10 3D Apes, 127+ Profiles & Jungle
// Architecture designed by Magnus Herrlin 2026
// ==============================================================================

#include "pack_utils.h"
#include <array>
#include <cmath>
#include <cstdio>
#include <iostream>
#include <random>
#include <sstream>
#include <string>
#include <vector>

// ==============================================================================
// 1. 3D SHADED BANANAS, PALM FRONDS & 5-LAYER JUNGLE BACKDROPS
// ==============================================================================
// ------------------------------------------------------------------------------
// 2. Build 3D gradient-shaded banana bunch with highlights and dark tips
// ------------------------------------------------------------------------------
static std::pair<std::string, std::string> buildBananaBunchSvg(const std::string& uid, float bx, float by, float sc, const std::string& hiC, const std::string& midC, const std::string& deepC)
{
    std::string defs =
        svgTag("linearGradient", "id=\"bg" + uid + "\" x1=\"0%\" y1=\"0%\" x2=\"100%\" y2=\"100%\"", false) +
        svgTag("stop", "offset=\"0%\" stop-color=\"" + hiC + "\"") +
        svgTag("stop", "offset=\"55%\" stop-color=\"" + midC + "\"") +
        svgTag("stop", "offset=\"100%\" stop-color=\"" + deepC + "\"") + svgClose("linearGradient");

    std::ostringstream body;
    body << svgTag("path", "d=\"M " + std::to_string(bx + 18 * sc) + "," + std::to_string(by - 22 * sc) + " Q " + std::to_string(bx + 24 * sc) + "," + std::to_string(by - 8 * sc) + " " + std::to_string(bx + 20 * sc) + "," + std::to_string(by + 6 * sc) + " L " + std::to_string(bx + 32 * sc) + "," + std::to_string(by + 6 * sc) + " Q " + std::to_string(bx + 34 * sc) + "," + std::to_string(by - 8 * sc) + " " + std::to_string(bx + 28 * sc) + "," + std::to_string(by - 22 * sc) + " Z\" fill=\"#451a03\" stroke=\"#1c0a00\" stroke-width=\"2\"");

    struct BSpec { float ang, ox, oy, bsc; };
    static constexpr std::array<BSpec, 5> kSpecs = {{
        {-26.0f, 0.0f, 4.0f, 1.00f}, {-13.0f, 10.0f, 2.0f, 1.06f}, {2.0f, 20.0f, 0.0f, 1.10f},
        {16.0f, 30.0f, 3.0f, 1.04f}, {29.0f, 38.0f, 7.0f, 0.96f}
    }};

    for (const auto& sp : kSpecs) {
        float x0 = bx + sp.ox * sc;
        float y0 = by + sp.oy * sc;
        float s  = sc * sp.bsc;
        std::string bp = "M " + std::to_string(x0) + "," + std::to_string(y0) + " C " + std::to_string(x0 - 18 * s) + "," + std::to_string(y0 + 26 * s) + " " + std::to_string(x0 - 12 * s) + "," + std::to_string(y0 + 64 * s) + " " + std::to_string(x0 + 14 * s) + "," + std::to_string(y0 + 82 * s) + " C " + std::to_string(x0 + 24 * s) + "," + std::to_string(y0 + 60 * s) + " " + std::to_string(x0 + 20 * s) + "," + std::to_string(y0 + 24 * s) + " " + std::to_string(x0 + 9 * s) + "," + std::to_string(y0) + " Z";
        std::string hp = "M " + std::to_string(x0 + 2 * s) + "," + std::to_string(y0 + 8 * s) + " C " + std::to_string(x0 - 8 * s) + "," + std::to_string(y0 + 30 * s) + " " + std::to_string(x0 - 4 * s) + "," + std::to_string(y0 + 56 * s) + " " + std::to_string(x0 + 10 * s) + "," + std::to_string(y0 + 70 * s);
        body << svgTag("g", "transform=\"rotate(" + std::to_string(sp.ang) + " " + std::to_string(x0) + " " + std::to_string(y0) + ")\"", false)
             << svgTag("path", "d=\"" + bp + "\" fill=\"url(#bg" + uid + ")\" stroke=\"#422006\" stroke-width=\"2.2\" stroke-linejoin=\"round\"")
             << svgTag("path", "d=\"" + hp + "\" fill=\"none\" stroke=\"#fef9c3\" stroke-width=\"" + std::to_string(2.4f * s) + "\" stroke-linecap=\"round\" opacity=\"0.65\"")
             << svgTag("circle", "cx=\"" + std::to_string(x0 + 14 * s) + "\" cy=\"" + std::to_string(y0 + 82 * s) + "\" r=\"" + std::to_string(2.8f * s) + "\" fill=\"#1c0a00\"")
             << svgClose("g");
    }
    return {defs, body.str()};
}
// ------------------------------------------------------------------------------
// 3. Build curved tropical palm frond with alternating dark and sunlit leaflets
// ------------------------------------------------------------------------------
static std::string buildPalmFrondSvg(float cx, float cy, float len, float angDeg, const std::string& colDark, const std::string& colHi)
{
    std::ostringstream ss;
    ss << svgTag("g", "transform=\"translate(" + std::to_string(cx) + "," + std::to_string(cy) + ") rotate(" + std::to_string(angDeg) + ")\"", false)
       << svgTag("path", "d=\"M 0,0 Q " + std::to_string(len * 0.5f) + "," + std::to_string(-len * 0.14f) + " " + std::to_string(len) + "," + std::to_string(len * 0.12f) + "\" fill=\"none\" stroke=\"" + colDark + "\" stroke-width=\"6\" stroke-linecap=\"round\"");
    for (int step = 1; step <= 12; ++step) {
        float t = static_cast<float>(step) / 13.0f;
        float bx = t * len;
        float by = -t * (1.0f - t) * len * 0.55f + (t * t) * len * 0.12f;
        float blen = len * 0.38f * std::sin(t * 3.1415926f);
        const std::string& col = (step % 2 == 0) ? colHi : colDark;
        ss << svgTag("path", "d=\"M " + std::to_string(bx) + "," + std::to_string(by) + " Q " + std::to_string(bx + blen * 0.35f) + "," + std::to_string(by - blen * 0.72f) + " " + std::to_string(bx - blen * 0.15f) + "," + std::to_string(by - blen) + "\" fill=\"none\" stroke=\"" + col + "\" stroke-width=\"5.5\" stroke-linecap=\"round\"")
           << svgTag("path", "d=\"M " + std::to_string(bx) + "," + std::to_string(by) + " Q " + std::to_string(bx + blen * 0.35f) + "," + std::to_string(by + blen * 0.72f) + " " + std::to_string(bx - blen * 0.10f) + "," + std::to_string(by + blen) + "\" fill=\"none\" stroke=\"" + col + "\" stroke-width=\"5.5\" stroke-linecap=\"round\"");
    }
    ss << svgClose("g");
    return ss.str();
}

struct JungleTimeSpec {
    const char* s1;
    const char* s2;
    const char* orb;
    int         orbX;
    int         orbY;
    const char* farC;
    const char* midC;
    const char* leafC;
    const char* banC;
    bool        night;
    bool        crescent;
};

// ------------------------------------------------------------------------------
// 4. Render 5 depth-of-field jungle layers (_sky, _clouds, _far, _mid, _front)
// ------------------------------------------------------------------------------
static int addJungleTimeBg(const JungleTimeSpec& t)
{
    int idx = nextAssetIdx("bg");
    std::array<char, 24> prefix{};
    std::snprintf(prefix.data(), prefix.size(), "bg/%03d", idx);
    std::string pfx(prefix.data());
    std::string idS = std::to_string(idx);
    std::string sDefs =
        svgTag("linearGradient", "id=\"bsk" + idS + "\" x1=\"0%\" y1=\"0%\" x2=\"0%\" y2=\"100%\"", false) +
        svgTag("stop", std::string("offset=\"0%\" stop-color=\"") + t.s1 + "\"") +
        svgTag("stop", std::string("offset=\"100%\" stop-color=\"") + t.s2 + "\"") + svgClose("linearGradient") +
        svgTag("radialGradient", "id=\"orb" + idS + "\" cx=\"50%\" cy=\"50%\" r=\"50%\"", false) +
        svgTag("stop", "offset=\"0%\" stop-color=\"#ffffff\" stop-opacity=\"0.95\"") +
        svgTag("stop", std::string("offset=\"45%\" stop-color=\"") + t.orb + "\" stop-opacity=\"0.85\"") +
        svgTag("stop", std::string("offset=\"100%\" stop-color=\"") + t.orb + "\" stop-opacity=\"0.0\"") + svgClose("radialGradient");
    std::ostringstream sky;
    sky << svgTag("rect", "x=\"-10\" y=\"-10\" width=\"620\" height=\"820\" fill=\"url(#bsk" + idS + ")\"");
    if (t.night) {
        static constexpr std::array<int, 20> kStars = {48,62, 125,138, 215,52, 310,118, 528,72, 568,178, 86,238, 382,192, 165,210, 495,230};
        for (size_t i = 0; i < kStars.size(); i += 2) {
            sky << svgTag("circle", "cx=\"" + std::to_string(kStars.at(i)) + "\" cy=\"" + std::to_string(kStars.at(i + 1)) + "\" r=\"2.1\" fill=\"#f8fafc\" opacity=\"0.92\"");
        }
    }
    sky << svgTag("circle", "cx=\"" + std::to_string(t.orbX) + "\" cy=\"" + std::to_string(t.orbY) + "\" r=\"115\" fill=\"url(#orb" + idS + ")\"")
        << svgTag("circle", "cx=\"" + std::to_string(t.orbX) + "\" cy=\"" + std::to_string(t.orbY) + "\" r=\"40\" fill=\"" + t.orb + "\" opacity=\"0.96\"");
    if (t.crescent) sky << svgTag("circle", "cx=\"" + std::to_string(t.orbX + 18) + "\" cy=\"" + std::to_string(t.orbY - 10) + "\" r=\"38\" fill=\"" + t.s1 + "\"");
    exportSvgToPng(pfx + "_sky", wrapSvgCanvas(sky.str(), sDefs), 600, 800, 0.0f);

    std::string cDefs =
        svgTag("linearGradient", "id=\"clg" + idS + "\" x1=\"0%\" y1=\"0%\" x2=\"0%\" y2=\"100%\"", false) +
        svgTag("stop", "offset=\"0%\" stop-color=\"#ffffff\" stop-opacity=\"0.82\"") +
        svgTag("stop", "offset=\"100%\" stop-color=\"#cbd5e1\" stop-opacity=\"0.35\"") + svgClose("linearGradient");
    std::string clouds = t.night ? "" : (
        svgTag("ellipse", "cx=\"165\" cy=\"132\" rx=\"125\" ry=\"26\" fill=\"url(#clg" + idS + ")\"") +
        svgTag("ellipse", "cx=\"215\" cy=\"118\" rx=\"85\" ry=\"28\" fill=\"url(#clg" + idS + ")\"") +
        svgTag("ellipse", "cx=\"455\" cy=\"178\" rx=\"135\" ry=\"28\" fill=\"url(#clg" + idS + ")\"") +
        svgTag("ellipse", "cx=\"500\" cy=\"164\" rx=\"82\" ry=\"24\" fill=\"url(#clg" + idS + ")\""));
    exportSvgToPng(pfx + "_clouds", wrapSvgCanvas(clouds, cDefs), 600, 800, t.night ? 0.0f : 5.5f);

    std::string fDefs =
        svgTag("linearGradient", "id=\"fg" + idS + "\" x1=\"0%\" y1=\"0%\" x2=\"0%\" y2=\"100%\"", false) +
        svgTag("stop", std::string("offset=\"0%\" stop-color=\"") + t.s2 + "\" stop-opacity=\"0.55\"") +
        svgTag("stop", std::string("offset=\"100%\" stop-color=\"") + t.farC + "\" stop-opacity=\"0.92\"") + svgClose("linearGradient");
    std::string far =
        svgTag("path", "d=\"M -20,500 L 115,315 L 255,455 L 385,295 L 520,445 L 620,365 L 620,820 L -20,820 Z\" fill=\"url(#fg" + idS + ")\"") +
        svgTag("rect", std::string("x=\"-20\" y=\"455\" width=\"640\" height=\"55\" fill=\"") + t.s2 + "\" opacity=\"0.32\"");
    exportSvgToPng(pfx + "_far", wrapSvgCanvas(far, fDefs), 600, 800, 9.5f);

    std::string mDefs =
        svgTag("linearGradient", "id=\"tr" + idS + "\" x1=\"0%\" y1=\"0%\" x2=\"100%\" y2=\"0%\"", false) +
        svgTag("stop", "offset=\"0%\" stop-color=\"#78350f\"") +
        svgTag("stop", std::string("offset=\"55%\" stop-color=\"") + t.midC + "\"") +
        svgTag("stop", "offset=\"100%\" stop-color=\"#090d16\"") + svgClose("linearGradient");
    std::ostringstream mid;
    mid << svgTag("path", std::string("d=\"M -20,625 Q 160,555 300,610 T 620,590 L 620,820 L -20,820 Z\" fill=\"") + t.midC + "\"")
        << svgTag("path", "d=\"M 8,820 C 26,520 18,280 28,90 L 82,90 C 74,280 84,520 104,820 Z\" fill=\"url(#tr" + idS + ")\"")
        << svgTag("path", "d=\"M 496,820 C 516,520 526,280 518,80 L 572,80 C 582,280 574,520 592,820 Z\" fill=\"url(#tr" + idS + ")\"");
    for (int cx : {45, 195, 405, 555}) mid << svgTag("ellipse", "cx=\"" + std::to_string(cx) + "\" cy=\"92\" rx=\"145\" ry=\"58\" fill=\"" + t.leafC + "\" opacity=\"0.86\"");
    exportSvgToPng(pfx + "_mid", wrapSvgCanvas(mid.str(), mDefs), 600, 800, 2.8f);

    auto b1 = buildBananaBunchSvg("bl" + idS, 28.0f, 175.0f, 1.05f, "#fef9c3", t.banC, "#92400e");
    auto b2 = buildBananaBunchSvg("br" + idS, 508.0f, 165.0f, 1.05f, "#fef9c3", t.banC, "#92400e");
    std::string front =
        svgTag("path", "d=\"M 46,-10 Q 78,185 38,385 Q 18,525 46,710\" fill=\"none\" stroke=\"#052e16\" stroke-width=\"11\" stroke-linecap=\"round\"") +
        svgTag("path", std::string("d=\"M 46,-10 Q 78,185 38,385 Q 18,525 46,710\" fill=\"none\" stroke=\"") + t.leafC + "\" stroke-width=\"5\" stroke-linecap=\"round\"") +
        svgTag("path", "d=\"M 554,-10 Q 522,175 562,375 Q 582,515 554,700\" fill=\"none\" stroke=\"#052e16\" stroke-width=\"11\" stroke-linecap=\"round\"") +
        svgTag("path", std::string("d=\"M 554,-10 Q 522,175 562,375 Q 582,515 554,700\" fill=\"none\" stroke=\"") + t.leafC + "\" stroke-width=\"5\" stroke-linecap=\"round\"") +
        buildPalmFrondSvg(-15.0f, 32.0f, 185.0f, 22.0f, "#052e16", t.leafC) +
        buildPalmFrondSvg(615.0f, 28.0f, 185.0f, 158.0f, "#052e16", t.leafC) +
        buildPalmFrondSvg(-18.0f, 730.0f, 175.0f, -26.0f, "#052e16", t.leafC) +
        buildPalmFrondSvg(618.0f, 725.0f, 175.0f, 206.0f, "#052e16", t.leafC) +
        b1.second + b2.second;
    exportSvgToPng(pfx + "_front", wrapSvgCanvas(front, b1.first + b2.first), 600, 800, 0.0f);
    return idx;
}

// ==============================================================================
// 5. 3D SILK FUR HAIR, CHIN BEARDS & JUNGLE SNAKE
// ==============================================================================
// ------------------------------------------------------------------------------
// 6. Generate multi-layered silk fur mane or crown tuft strands
// ------------------------------------------------------------------------------
static std::string buildSilkFurHairSvg(const std::string& cDeep, const std::string& cDark, const std::string& cMid, const std::string& cHi, const std::string& style)
{
    std::mt19937 rng(421337);
    std::uniform_real_distribution<float> uni(0.0f, 1.0f);
    std::normal_distribution<float> gauss(0.0f, 0.38f);
    std::ostringstream ss;

    auto pickCol = [&](float v) -> const std::string& {
        if (v < 0.25f) return cDeep;
        if (v < 0.55f) return cDark;
        if (v < 0.85f) return cMid;
        return cHi;
    };

    if (style == "tuft") {
        ss << svgTag("path", "d=\"M 225,218 Q 265,155 300,138 Q 335,155 375,218 Q 300,204 225,218 Z\" fill=\"" + cDeep + "\" opacity=\"0.88\"");
        for (int i = 0; i < 900; ++i) {
            float u = gauss(rng);
            if (std::fabs(u) > 1.0f) continue;
            float v = uni(rng);
            float x0 = 300.0f + u * 88.0f;
            float y0 = 206.0f - (1.0f - u * u) * 32.0f + v * 18.0f;
            float hlen = (28.0f + (1.0f - std::fabs(u)) * 34.0f) * (0.6f + 0.5f * v);
            float x1 = x0 + u * 26.0f + (uni(rng) - 0.5f) * 8.0f;
            float y1 = y0 - hlen;
            ss << svgTag("path", "d=\"M " + std::to_string(x0) + "," + std::to_string(y0) + " Q " + std::to_string((x0 + x1) * 0.5f + u * 10.0f) + "," + std::to_string((y0 + y1) * 0.5f) + " " + std::to_string(x1) + "," + std::to_string(y1) + "\" fill=\"none\" stroke=\"" + pickCol(v) + "\" stroke-width=\"1.8\" stroke-linecap=\"round\" opacity=\"0.82\"");
        }
        return wrapSvgCanvas(ss.str());
    }

    ss << svgTag("path", "d=\"M 56,565 C 24,385 48,175 165,135 C 225,112 375,112 435,135 C 552,175 576,385 544,565 L 492,520 C 508,350 475,215 300,215 C 125,215 92,350 108,520 Z\" fill=\"" + cDeep + "\" opacity=\"0.90\"");
    for (int i = 0; i < 2000; ++i) {
        float theta = -3.70f + uni(rng) * 4.26f;
        float v = uni(rng);
        float x0 = 300.0f + std::cos(theta) * (198.0f + v * 58.0f);
        float y0 = 415.0f + std::sin(theta) * (225.0f + v * 52.0f);
        if (y0 > 228.0f && std::fabs(x0 - 300.0f) < 186.0f) continue;
        float side = (x0 >= 300.0f) ? 1.0f : -1.0f;
        float hlen = ((style == "orang_shag") ? 58.0f : 42.0f) + v * 28.0f;
        float angDeg = (y0 < 210.0f) ? (theta * 57.2958f * 0.45f + 52.0f + (uni(rng) - 0.5f) * 20.0f)
                                       : (90.0f + side * (16.0f - v * 8.0f) + (uni(rng) - 0.5f) * 16.0f);
        float rad = angDeg * 0.0174533f;
        float x1 = x0 + std::cos(rad) * hlen;
        float y1 = y0 + std::sin(rad) * hlen;
        float wave = ((style == "orang_shag") ? 9.0f : 6.0f) * std::sin(v * 11.0f);
        ss << svgTag("path", "d=\"M " + std::to_string(x0) + "," + std::to_string(y0) + " Q " + std::to_string((x0 + x1) * 0.5f + wave) + "," + std::to_string((y0 + y1) * 0.5f) + " " + std::to_string(x1) + "," + std::to_string(y1) + "\" fill=\"none\" stroke=\"" + pickCol(v) + "\" stroke-width=\"1.9\" stroke-linecap=\"round\" opacity=\"0.82\"");
    }
    return wrapSvgCanvas(ss.str());
}
// ------------------------------------------------------------------------------
// 7. Generate chin beard hair strands
// ------------------------------------------------------------------------------
static std::string buildApeBeardSvg(const std::string& cDeep, const std::string& cMid, const std::string& cHi, float length)
{
    std::mt19937 rng(7733);
    std::uniform_real_distribution<float> uni(0.0f, 1.0f);
    std::ostringstream ss;
    for (int i = 0; i < 480; ++i) {
        float ux = uni(rng) * 2.0f - 1.0f;
        float vy = uni(rng);
        float x0 = 300.0f + ux * 115.0f * (1.0f - 0.35f * vy);
        float y0 = 645.0f + vy * 55.0f;
        float hlen = length * (1.0f - 0.45f * std::fabs(ux)) + (uni(rng) * 22.0f - 8.0f);
        float x1 = x0 + ux * 16.0f + (uni(rng) * 10.0f - 5.0f);
        float y1 = y0 + hlen;
        const std::string& col = (vy < 0.35f) ? cDeep : ((vy < 0.75f) ? cMid : cHi);
        ss << svgTag("path", "d=\"M " + std::to_string(x0) + "," + std::to_string(y0) + " Q " + std::to_string((x0 + x1) * 0.5f + 4.0f) + "," + std::to_string((y0 + y1) * 0.5f) + " " + std::to_string(x1) + "," + std::to_string(y1) + "\" fill=\"none\" stroke=\"" + col + "\" stroke-width=\"2.2\" stroke-linecap=\"round\" opacity=\"0.84\"");
    }
    return wrapSvgCanvas(ss.str());
}

// ------------------------------------------------------------------------------
// 8. Build 3D-shaded slithering jungle snake sprite with forked red tongue
// ------------------------------------------------------------------------------
static std::string buildJungleSnakeSvg()
{
    std::string defs =
        svgTag("linearGradient", "id=\"snkGrad\" x1=\"0%\" y1=\"0%\" x2=\"100%\" y2=\"100%\"", false) +
        svgTag("stop", "offset=\"0%\" stop-color=\"#84cc16\"") +
        svgTag("stop", "offset=\"55%\" stop-color=\"#15803d\"") +
        svgTag("stop", "offset=\"100%\" stop-color=\"#052e16\"") + svgClose("linearGradient");

    std::string body =
        svgTag("path", "d=\"M 80,0 C 40,65 130,115 70,175 C 25,220 115,255 82,295\" fill=\"none\" stroke=\"#052e16\" stroke-width=\"24\" stroke-linecap=\"round\"") +
        svgTag("path", "d=\"M 80,0 C 40,65 130,115 70,175 C 25,220 115,255 82,295\" fill=\"none\" stroke=\"url(#snkGrad)\" stroke-width=\"18\" stroke-linecap=\"round\"") +
        svgTag("path", "d=\"M 80,0 C 40,65 130,115 70,175 C 25,220 115,255 82,295\" fill=\"none\" stroke=\"#bef264\" stroke-width=\"5\" stroke-dasharray=\"8 12\" stroke-linecap=\"round\" opacity=\"0.78\"") +
        svgTag("ellipse", "cx=\"80\" cy=\"302\" rx=\"18\" ry=\"24\" fill=\"url(#snkGrad)\" stroke=\"#052e16\" stroke-width=\"3\"") +
        svgTag("circle", "cx=\"72\" cy=\"304\" r=\"4\" fill=\"#facc15\"") +
        svgTag("circle", "cx=\"88\" cy=\"304\" r=\"4\" fill=\"#facc15\"") +
        svgTag("path", "d=\"M 80,324 L 80,348 L 72,358 M 80,348 L 88,358\" fill=\"none\" stroke=\"#ef4444\" stroke-width=\"3\" stroke-linecap=\"round\"");

    return wrapSvgCanvas(body, defs, 160, 370);
}
// ==============================================================================
// 9. 3D SCULPTED APE HEADS, EARS, NOSES & MAKEUP
// ==============================================================================
static std::string build3dApeHeadSvg(const std::string& tag, const std::string& sp, const std::string& f1, const std::string& f2, const std::string& f3, const std::string& mh, const std::string& mm, const std::string& md)
{
    std::string defs =
        svgTag("radialGradient", "id=\"fh" + tag + "\" cx=\"44%\" cy=\"30%\" r=\"70%\"", false) +
        svgTag("stop", "offset=\"0%\" stop-color=\"" + f1 + "\"") +
        svgTag("stop", "offset=\"55%\" stop-color=\"" + f2 + "\"") +
        svgTag("stop", "offset=\"100%\" stop-color=\"" + f3 + "\"") + svgClose("radialGradient") +
        svgTag("radialGradient", "id=\"mz" + tag + "\" cx=\"46%\" cy=\"36%\" r=\"62%\"", false) +
        svgTag("stop", "offset=\"0%\" stop-color=\"" + mh + "\"") +
        svgTag("stop", "offset=\"62%\" stop-color=\"" + mm + "\"") +
        svgTag("stop", "offset=\"100%\" stop-color=\"" + md + "\"") + svgClose("radialGradient") +
        svgTag("filter", "id=\"sb" + tag + "\" x=\"-10%\" y=\"-10%\" width=\"120%\" height=\"120%\"", false) +
        svgTag("feGaussianBlur", "stdDeviation=\"4.5\"") + svgClose("filter") +
        svgTag("mask", "id=\"mk" + tag + "\"", false) + getStandardEyeCutoutMask() + svgClose("mask");

    std::string skull, det;
    if (sp == "gorilla" || sp == "cybergorilla") {
        skull = "M 300,132 C 368,132 418,182 458,232 C 528,295 554,415 544,548 C 534,678 465,758 300,776 C 135,758 66,678 56,548 C 46,415 72,295 142,232 C 182,182 232,132 300,132 Z";
        det = svgTag("ellipse", "cx=\"300\" cy=\"285\" rx=\"195\" ry=\"38\" fill=\"" + f3 + "\" filter=\"url(#sb" + tag + ")\"") +
              svgTag("ellipse", "cx=\"300\" cy=\"568\" rx=\"162\" ry=\"154\" fill=\"url(#mz" + tag + ")\"");
        if (sp == "cybergorilla") det += svgTag("path", "d=\"M 145,245 L 220,275 L 275,275 M 455,245 L 380,275 L 325,275 M 165,640 L 235,680 L 365,680 L 435,640\" fill=\"none\" stroke=\"#00e5ff\" stroke-width=\"5\"");
    } else if (sp == "orangutan") {
        skull = "M 300,185 C 435,185 495,255 495,395 C 495,545 455,685 300,765 C 145,685 105,545 105,395 C 105,255 165,185 300,185 Z";
        det = svgTag("ellipse", "cx=\"92\" cy=\"445\" rx=\"76\" ry=\"172\" fill=\"url(#fh" + tag + ")\"") +
              svgTag("ellipse", "cx=\"508\" cy=\"445\" rx=\"76\" ry=\"172\" fill=\"url(#fh" + tag + ")\"") +
              svgTag("ellipse", "cx=\"300\" cy=\"558\" rx=\"152\" ry=\"155\" fill=\"url(#mz" + tag + ")\"");
    } else if (sp == "mandrill" || sp == "baboon") {
        skull = "M 300,186 C 470,186 538,262 530,398 C 522,504 472,580 432,644 C 398,712 362,774 300,780 C 238,774 202,712 168,644 C 128,580 78,504 70,398 C 62,262 130,186 300,186 Z";
        det = svgTag("path", "d=\"M 192,382 L 408,382 L 380,708 Q 300,758 220,708 Z\" fill=\"url(#mz" + tag + ")\"");
    } else {
        skull = "M 300,202 C 455,202 522,270 516,395 C 510,520 470,655 402,718 C 362,755 330,765 300,765 C 270,765 238,755 198,718 C 130,655 90,520 84,395 C 78,270 145,202 300,202 Z";
        det = svgTag("path", "d=\"M 122,348 C 122,276 264,276 300,324 C 336,276 478,276 478,348 C 478,422 442,452 422,474 C 450,545 412,692 300,692 C 188,692 150,545 178,474 C 158,452 122,422 122,348 Z\" fill=\"url(#mz" + tag + ")\"");
    }

    std::string body =
        svgTag("g", "mask=\"url(#mk" + tag + ")\"", false) +
        svgTag("path", "d=\"" + skull + "\" fill=\"url(#fh" + tag + ")\"") + det +
        svgTag("ellipse", "cx=\"190\" cy=\"350\" rx=\"84\" ry=\"60\" fill=\"#000000\" opacity=\"0.34\" filter=\"url(#sb" + tag + ")\"") +
        svgTag("ellipse", "cx=\"410\" cy=\"350\" rx=\"84\" ry=\"60\" fill=\"#000000\" opacity=\"0.34\" filter=\"url(#sb" + tag + ")\"") +
        svgTag("ellipse", "cx=\"300\" cy=\"238\" rx=\"115\" ry=\"34\" fill=\"#ffffff\" opacity=\"0.16\" filter=\"url(#sb" + tag + ")\"") +
        svgClose("g");
    return wrapSvgCanvas(body, defs);
}
static std::string build3dApeEarsSvg(const std::string& tag, const std::string& hi, const std::string& mid, const std::string& deep, const std::string& inHi, const std::string& inDeep, int rOut)
{
    std::string defs =
        svgTag("radialGradient", "id=\"eo" + tag + "\" cx=\"40%\" cy=\"30%\" r=\"68%\"", false) +
        svgTag("stop", "offset=\"0%\" stop-color=\"" + hi + "\"") +
        svgTag("stop", "offset=\"60%\" stop-color=\"" + mid + "\"") +
        svgTag("stop", "offset=\"100%\" stop-color=\"" + deep + "\"") + svgClose("radialGradient") +
        svgTag("radialGradient", "id=\"ei" + tag + "\" cx=\"58%\" cy=\"58%\" r=\"62%\"", false) +
        svgTag("stop", "offset=\"0%\" stop-color=\"" + inHi + "\"") +
        svgTag("stop", "offset=\"100%\" stop-color=\"" + inDeep + "\"") + svgClose("radialGradient");
    int rIn = static_cast<int>(rOut * 0.64f);
    int cxL = (rOut < 50) ? 92 : 72;
    int cxR = 600 - cxL;
    std::string body =
        svgTag("circle", "cx=\"" + std::to_string(cxL) + "\" cy=\"382\" r=\"" + std::to_string(rOut) + "\" fill=\"url(#eo" + tag + ")\"") +
        svgTag("circle", "cx=\"" + std::to_string(cxL + 8) + "\" cy=\"382\" r=\"" + std::to_string(rIn) + "\" fill=\"url(#ei" + tag + ")\"") +
        svgTag("circle", "cx=\"" + std::to_string(cxR) + "\" cy=\"382\" r=\"" + std::to_string(rOut) + "\" fill=\"url(#eo" + tag + ")\"") +
        svgTag("circle", "cx=\"" + std::to_string(cxR - 8) + "\" cy=\"382\" r=\"" + std::to_string(rIn) + "\" fill=\"url(#ei" + tag + ")\"");
    return wrapSvgCanvas(body, defs);
}

static std::string build3dApeNoseSvg(const std::string& tag, const std::string& hi, const std::string& mid, const std::string& deep, const std::string& style)
{
    std::string defs =
        svgTag("radialGradient", "id=\"nb" + tag + "\" cx=\"50%\" cy=\"50%\" r=\"50%\"", false) +
        svgTag("stop", "offset=\"0%\" stop-color=\"" + mid + "\" stop-opacity=\"0.90\"") +
        svgTag("stop", "offset=\"100%\" stop-color=\"" + deep + "\" stop-opacity=\"0\"") + svgClose("radialGradient") +
        svgTag("radialGradient", "id=\"nt" + tag + "\" cx=\"40%\" cy=\"30%\" r=\"64%\"", false) +
        svgTag("stop", "offset=\"0%\" stop-color=\"" + hi + "\"") +
        svgTag("stop", "offset=\"58%\" stop-color=\"" + mid + "\"") +
        svgTag("stop", "offset=\"100%\" stop-color=\"" + deep + "\"") + svgClose("radialGradient");
    if (style == "proboscis") {
        std::string body =
            svgTag("ellipse", "cx=\"300\" cy=\"552\" rx=\"40\" ry=\"18\" fill=\"#000000\" opacity=\"0.32\"") +
            svgTag("path", "d=\"M 278,408 C 260,455 250,518 272,546 C 292,566 308,566 328,546 C 350,518 340,455 322,408 Z\" fill=\"url(#nt" + tag + ")\"");
        return wrapSvgCanvas(body, defs);
    }
    float wSc = (style == "gorilla") ? 1.40f : ((style == "slender") ? 0.86f : 1.06f);
    int rx = static_cast<int>(42 * wSc);
    int wx = static_cast<int>(38 * wSc);
    std::string body =
        svgTag("ellipse", "cx=\"300\" cy=\"508\" rx=\"" + std::to_string(rx + 12) + "\" ry=\"15\" fill=\"#000000\" opacity=\"0.30\"") +
        svgTag("ellipse", "cx=\"300\" cy=\"415\" rx=\"" + std::to_string(static_cast<int>(26 * wSc)) + "\" ry=\"88\" fill=\"url(#nb" + tag + ")\"") +
        svgTag("ellipse", "cx=\"" + std::to_string(300 - wx) + "\" cy=\"492\" rx=\"" + std::to_string(static_cast<int>(26 * wSc)) + "\" ry=\"21\" fill=\"url(#nt" + tag + ")\"") +
        svgTag("ellipse", "cx=\"" + std::to_string(300 + wx) + "\" cy=\"492\" rx=\"" + std::to_string(static_cast<int>(26 * wSc)) + "\" ry=\"21\" fill=\"url(#nt" + tag + ")\"") +
        svgTag("ellipse", "cx=\"300\" cy=\"482\" rx=\"" + std::to_string(rx) + "\" ry=\"28\" fill=\"url(#nt" + tag + ")\"") +
        svgTag("ellipse", "cx=\"" + std::to_string(300 - static_cast<int>(22 * wSc)) + "\" cy=\"498\" rx=\"11\" ry=\"6\" fill=\"#090605\"") +
        svgTag("ellipse", "cx=\"" + std::to_string(300 + static_cast<int>(22 * wSc)) + "\" cy=\"498\" rx=\"11\" ry=\"6\" fill=\"#090605\"");
    return wrapSvgCanvas(body, defs);
}

static std::string buildApeMakeupSvg(const std::string& tag, const std::string& mode)
{
    std::string defs = svgTag("mask", "id=\"amk" + tag + "\"", false) + getStandardEyeCutoutMask() + svgClose("mask");
    std::string p;
    if (mode == "mandrill") {
        p = svgTag("path", "d=\"M 216,452 L 250,646 M 234,452 L 264,646\" stroke=\"#38bdf8\" stroke-width=\"11\" stroke-linecap=\"round\"") +
            svgTag("path", "d=\"M 225,452 L 257,646\" stroke=\"#ef4444\" stroke-width=\"9\" stroke-linecap=\"round\"") +
            svgTag("path", "d=\"M 384,452 L 350,646 M 366,452 L 336,646\" stroke=\"#38bdf8\" stroke-width=\"11\" stroke-linecap=\"round\"") +
            svgTag("path", "d=\"M 375,452 L 343,646\" stroke=\"#ef4444\" stroke-width=\"9\" stroke-linecap=\"round\"");
    } else if (mode == "rosy") {
        p = svgTag("ellipse", "cx=\"165\" cy=\"482\" rx=\"44\" ry=\"28\" fill=\"#fb7185\" opacity=\"0.42\"") +
            svgTag("ellipse", "cx=\"435\" cy=\"482\" rx=\"44\" ry=\"28\" fill=\"#fb7185\" opacity=\"0.42\"");
    } else {
        p = svgTag("path", "d=\"M 180,236 Q 300,222 420,236 M 192,258 Q 300,246 408,258\" fill=\"none\" stroke=\"#1f0c04\" stroke-width=\"4\" stroke-linecap=\"round\" opacity=\"0.32\"") +
            svgTag("ellipse", "cx=\"165\" cy=\"484\" rx=\"38\" ry=\"24\" fill=\"#ff8a65\" opacity=\"0.28\"") +
            svgTag("ellipse", "cx=\"435\" cy=\"484\" rx=\"38\" ry=\"24\" fill=\"#ff8a65\" opacity=\"0.28\"");
    }
    return wrapSvgCanvas(svgTag("g", "mask=\"url(#amk" + tag + ")\"", false) + p + svgClose("g"), defs);
}
struct ApeSpec {
    const char* name; const char* species;
    const char* f1; const char* f2; const char* f3;
    const char* mh; const char* mm; const char* md;
    int earR; const char* noseStyle;
    int bgIdx; int fxMask; int hatId; int hairId; int makeupId; int beardId; int teethId; int animId;
};

// ------------------------------------------------------------------------------
// 10. Generate 10 jungle backdrops, 10 apes, 50 side-profile pairs & ZIP
// ------------------------------------------------------------------------------
// ------------------------------------------------------------------------------
// 10. Generate 10 jungle backdrops, 10 apes, 50 side-profile pairs & ZIP
// ------------------------------------------------------------------------------
int main(void)
{
    ensureBaseIni();
    generateDefaultFxBank();
    generateDefaultBrowsBank();
    exportSvgToPng("fx/snake", buildJungleSnakeSvg(), 160, 370, 0.0f);

    static constexpr std::array<JungleTimeSpec, 10> kTimes = {{
        {"#1e3a8a", "#f472b6", "#fde047", 140, 250, "#1e293b", "#14532d", "#15803d", "#facc15", false, false},
        {"#0284c7", "#bae6fd", "#fef08a", 210, 195, "#0f766e", "#14532d", "#16a34a", "#facc15", false, false},
        {"#0369a1", "#7dd3fc", "#fef9c3", 310, 145, "#047857", "#14532d", "#16a34a", "#fde047", false, false},
        {"#0284c7", "#e0f2fe", "#ffffff", 410, 105, "#059669", "#065f46", "#22c55e", "#fde047", false, false},
        {"#0369a1", "#93c5fd", "#fef08a", 470, 135, "#0f766e", "#14532d", "#16a34a", "#facc15", false, false},
        {"#1d4ed8", "#fdba74", "#fde047", 500, 190, "#7c2d12", "#3f2005", "#15803d", "#facc15", false, false},
        {"#1e1b4b", "#ea580c", "#fb923c", 475, 255, "#451a03", "#1c1917", "#14532d", "#eab308", false, false},
        {"#0f172a", "#312e81", "#f8fafc", 445, 165, "#1e293b", "#0f172a", "#064e3b", "#ca8a04", true,  true},
        {"#020617", "#1e293b", "#fef9c3", 360, 120, "#0f766e", "#090d16", "#052e16", "#ca8a04", true,  false},
        {"#020617", "#0f172a", "#e2e8f0", 230, 135, "#0f172a", "#020617", "#064e3b", "#a3e635", true,  true}
    }};

    std::vector<int> bgs;
    for (const auto& jt : kTimes) bgs.push_back(addJungleTimeBg(jt));

    int crown  = saveSvgAsset("hat", wrapSvgCanvas(svgTag("polygon", "points=\"155,218 172,102 234,166 300,86 366,166 428,102 445,218\" fill=\"#ffd700\"") + svgTag("rect", "x=\"155\" y=\"196\" width=\"290\" height=\"24\" rx=\"6\" fill=\"#d50000\"")));
    int pirate = saveSvgAsset("hat", wrapSvgCanvas(svgTag("path", "d=\"M 96,248 C 110,112 490,112 504,248 C 442,196 365,186 300,205 C 235,186 158,196 96,248 Z\" fill=\"#1e293b\"") + svgTag("circle", "cx=\"300\" cy=\"148\" r=\"16\" fill=\"#ffd700\"")));

    int hrTuft  = saveSvgAsset("hair", buildSilkFurHairSvg("#1f1008", "#3d2208", "#6e4229", "#9e633e", "tuft"));
    int hrOrang = saveSvgAsset("hair", buildSilkFurHairSvg("#431407", "#7c2d12", "#c2410c", "#fb923c", "orang_shag"));
    int hrMane  = saveSvgAsset("hair", buildSilkFurHairSvg("#451a03", "#92400e", "#d97706", "#fde047", "mane"));
    int hrSnow  = saveSvgAsset("hair", buildSilkFurHairSvg("#64748b", "#94a3b8", "#e2e8f0", "#ffffff", "mane"));

    int mkWrink = saveSvgAsset("makeup", buildApeMakeupSvg("mk1", "wrinkles"));
    int mkMandr = saveSvgAsset("makeup", buildApeMakeupSvg("mk2", "mandrill"));
    int mkRosy  = saveSvgAsset("makeup", buildApeMakeupSvg("mk3", "rosy"));

    int bdOrang = saveSvgAsset("beard", buildApeBeardSvg("#431407", "#9a3412", "#f97316", 115.0f));
    int bdGold  = saveSvgAsset("beard", buildApeBeardSvg("#78350f", "#d97706", "#fde047", 85.0f));

    std::array<ApeSpec, 10> specs = {{
        {"Charlie_Fly",  "chimp",        "#8d5b3c","#58311a","#240f04","#ffe4d1","#dfa37c","#854c25",76,"chimp",    bgs.at(8),2, 0,hrTuft, mkWrink,0,      1,16},
        {"Gorilla",      "gorilla",      "#525866","#27272a","#09090b","#94a3b8","#475569","#1e293b",38,"gorilla",  bgs.at(6),0, 0,0,      mkWrink,0,      8,41},
        {"Orangutan",    "orangutan",    "#ea580c","#9a3412","#431407","#fed7aa","#fb923c","#9a3412",46,"gorilla",  bgs.at(5),0, 0,hrOrang,mkWrink,bdOrang,1,13},
        {"Mandrill",     "mandrill",     "#475569","#1e293b","#090d16","#7dd3fc","#0284c7","#1e3a8a",54,"slender",  bgs.at(3),0, 0,0,      mkMandr,bdGold, 3,8},
        {"Baboon",       "baboon",       "#a8a29e","#57534e","#1c1917","#fecdd3","#fb7185","#9f1239",56,"slender",  bgs.at(2),0, 0,hrTuft, mkWrink,0,      1,18},
        {"LionTamarin",  "chimp",        "#f59e0b","#b45309","#451a03","#fef9c3","#fde047","#ca8a04",48,"chimp",    bgs.at(4),64,0,hrMane, mkRosy, bdGold, 1,24},
        {"SnowMacaque",  "chimp",        "#f8fafc","#cbd5e1","#64748b","#ffe4e6","#fda4af","#be123c",54,"chimp",    bgs.at(9),0, 0,hrSnow, mkRosy, 0,      1,42},
        {"Proboscis",    "chimp",        "#d97706","#92400e","#451a03","#fed7aa","#fdba74","#c2410c",58,"proboscis",bgs.at(1),0, 0,0,      mkWrink,0,      1,28},
        {"SunWukong",    "chimp",        "#b45309","#78350f","#2b1205","#fde68a","#fcd34d","#b45309",64,"chimp",    bgs.at(0),0, crown,0,mkRosy, bdGold, 1,48},
        {"CyberGorilla", "cybergorilla", "#94a3b8","#475569","#0f172a","#bae6fd","#38bdf8","#0369a1",38,"gorilla",  bgs.at(7),0, pirate,0,0,     0,      1,58}
    }};

    std::vector<ProfileThemeSpec> profThemes;
    for (size_t i = 0; i < specs.size(); ++i) {
        const auto& s = specs.at(i);
        std::string tag = "a" + std::to_string(i);
        int fId = saveSvgAsset("face", build3dApeHeadSvg(tag, s.species, s.f1, s.f2, s.f3, s.mh, s.mm, s.md));
        int eId = saveSvgAsset("ears", build3dApeEarsSvg(tag, s.f1, s.f2, s.f3, s.mh, s.md, s.earR));
        int nId = saveSvgAsset("nose", build3dApeNoseSvg(tag, s.mh, s.mm, s.md, s.noseStyle));
        appendFaceOffsets(fId, {{"mouth_y", "14"}, {"brows_y", "-2"}});
        appendMacroCombo({fId, s.makeupId, eId, nId, s.beardId, 0, s.hairId, s.hatId, 1, s.bgIdx, s.fxMask, s.teethId, 0, 0, -1, -1, s.animId, false});

        profThemes.push_back({
            s.f1, s.f2, s.f3, s.mh, s.mm, s.md,
            s.f3, s.f2, s.f1,
            s.earR, s.bgIdx, s.teethId, 0, 0, -1, s.animId, -1,
            (s.makeupId != 0), (s.beardId != 0), false, (s.hatId != 0)
        });
    }

    generateStandardSideProfileSuite(profThemes, 5);

    cleanSvgAndSaveZip("monkeys");
    std::cout << "[OK] 100% C++17 Original Apes + 50 Side-Profile Pairs (127..226) + Snake FX zipped!\n";
    return 0;
}
