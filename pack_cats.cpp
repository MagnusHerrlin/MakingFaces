// ==============================================================================
// MakingFaces - Theme Pack 3 (C++17): 8 3D Cats, Big Cats & Rooftop Nights
// Architecture designed by Magnus Herrlin 2026
// ==============================================================================

// ==============================================================================
// 1. INCLUDES AND FELINE SCENE BACKDROP BUILDERS
// ==============================================================================
#include "pack_utils.h"
#include <array>
#include <cmath>
#include <cstdio>
#include <filesystem>
#include <iostream>
#include <random>
#include <sstream>
#include <string>
#include <vector>

namespace fs = std::filesystem;

struct CatSceneSpec {
    const char* sTop;
    const char* sBot;
    const char* moonHex;
    int         moonX;
    int         moonY;
    const char* farHex;
    const char* midHex;
    const char* frontHex;
    bool        stars;
};

// ------------------------------------------------------------------------------
// 2. Render 5-layer depth-of-field rooftop, window & long-table backdrops
// ------------------------------------------------------------------------------
static int addCatBackdropScene(const CatSceneSpec& sc)
{
    int idx = nextAssetIdx("bg");
    std::array<char, 24> prefix{};
    std::snprintf(prefix.data(), prefix.size(), "bg/%03d", idx);
    std::string pfx(prefix.data());
    std::string idS = std::to_string(idx);

    std::string sDefs =
        svgTag("linearGradient", "id=\"csk" + idS + "\" x1=\"0%\" y1=\"0%\" x2=\"0%\" y2=\"100%\"", false) +
        svgTag("stop", std::string("offset=\"0%\" stop-color=\"") + sc.sTop + "\"") +
        svgTag("stop", std::string("offset=\"100%\" stop-color=\"") + sc.sBot + "\"") + svgClose("linearGradient") +
        svgTag("radialGradient", "id=\"cmn" + idS + "\" cx=\"50%\" cy=\"50%\" r=\"50%\"", false) +
        svgTag("stop", "offset=\"0%\" stop-color=\"#ffffff\" stop-opacity=\"0.95\"") +
        svgTag("stop", std::string("offset=\"50%\" stop-color=\"") + sc.moonHex + "\" stop-opacity=\"0.85\"") +
        svgTag("stop", std::string("offset=\"100%\" stop-color=\"") + sc.moonHex + "\" stop-opacity=\"0.0\"") + svgClose("radialGradient");

    std::ostringstream sky;
    sky << svgTag("rect", "x=\"-10\" y=\"-10\" width=\"620\" height=\"820\" fill=\"url(#csk" + idS + ")\"");
    if (sc.stars)
    {
        static constexpr std::array<int, 16> kSt = {55,70, 140,120, 230,65, 340,110, 510,80, 560,165, 95,215, 420,195};
        for (size_t i = 0; i < kSt.size(); i += 2)
        {
            sky << svgTag("circle", "cx=\"" + std::to_string(kSt.at(i)) + "\" cy=\"" + std::to_string(kSt.at(i + 1)) + "\" r=\"2.2\" fill=\"#f8fafc\" opacity=\"0.90\"");
        }
    }
    sky << svgTag("circle", "cx=\"" + std::to_string(sc.moonX) + "\" cy=\"" + std::to_string(sc.moonY) + "\" r=\"118\" fill=\"url(#cmn" + idS + ")\"")
        << svgTag("circle", "cx=\"" + std::to_string(sc.moonX) + "\" cy=\"" + std::to_string(sc.moonY) + "\" r=\"42\" fill=\"" + sc.moonHex + "\"");
    exportSvgToPng(pfx + "_sky", wrapSvgCanvas(sky.str(), sDefs), 600, 800, 0.0f);

    std::string clouds =
        svgTag("ellipse", "cx=\"175\" cy=\"145\" rx=\"135\" ry=\"24\" fill=\"#e2e8f0\" opacity=\"0.28\"") +
        svgTag("ellipse", "cx=\"445\" cy=\"185\" rx=\"145\" ry=\"26\" fill=\"#cbd5e1\" opacity=\"0.25\"");
    exportSvgToPng(pfx + "_clouds", wrapSvgCanvas(clouds), 600, 800, 5.5f);

    std::string far =
        svgTag("path", std::string("d=\"M -20,540 L 80,540 L 80,410 L 165,375 L 250,410 L 250,510 L 360,510 L 360,360 L 455,325 L 550,360 L 550,520 L 620,520 L 620,820 L -20,820 Z\" fill=\"") + sc.farHex + "\"");
    exportSvgToPng(pfx + "_far", wrapSvgCanvas(far), 600, 800, 8.5f);

    std::string mid =
        svgTag("path", std::string("d=\"M -20,665 L 145,595 L 300,645 L 465,590 L 620,660 L 620,820 L -20,820 Z\" fill=\"") + sc.midHex + "\"") +
        svgTag("rect", std::string("x=\"42\" y=\"495\" width=\"48\" height=\"140\" fill=\"") + sc.midHex + "\"") +
        svgTag("rect", std::string("x=\"36\" y=\"485\" width=\"60\" height=\"14\" fill=\"") + sc.midHex + "\"");
    exportSvgToPng(pfx + "_mid", wrapSvgCanvas(mid), 600, 800, 2.5f);

    std::string tDefs =
        svgTag("linearGradient", "id=\"tblTop" + idS + "\" x1=\"0%\" y1=\"0%\" x2=\"0%\" y2=\"100%\"", false) +
        svgTag("stop", "offset=\"0%\" stop-color=\"#92400e\"") +
        svgTag("stop", "offset=\"100%\" stop-color=\"#451a03\"") + svgClose("linearGradient");

    std::string front =
        svgTag("rect", "x=\"-10\" y=\"712\" width=\"620\" height=\"22\" rx=\"6\" fill=\"url(#tblTop" + idS + ")\" stroke=\"#2b1205\" stroke-width=\"3\"") +
        svgTag("rect", std::string("x=\"-10\" y=\"734\" width=\"620\" height=\"76\" fill=\"") + sc.frontHex + "\"") +
        svgTag("path", "d=\"M 44,712 L 88,684 L 94,712 Z\" fill=\"#facc15\" stroke=\"#ca8a04\" stroke-width=\"2\"") +
        svgTag("circle", "cx=\"72\" cy=\"704\" r=\"3.5\" fill=\"#a16207\"") +
        svgTag("circle", "cx=\"84\" cy=\"700\" r=\"2.8\" fill=\"#a16207\"") +
        svgTag("circle", "cx=\"536\" cy=\"692\" r=\"22\" fill=\"#f43f5e\"") +
        svgTag("path", "d=\"M 518,686 Q 536,674 554,686 M 516,698 Q 536,688 556,698 M 514,706 Q 472,724 428,714\" fill=\"none\" stroke=\"#ffe4e6\" stroke-width=\"3\" stroke-linecap=\"round\"");
    exportSvgToPng(pfx + "_front", wrapSvgCanvas(front, tDefs), 600, 800, 0.0f);
    return idx;
}

// ==============================================================================
// 3. 3D FELINE HEADS, POINTED EARS, SNOUTS & WHISKERS
// ==============================================================================
// ------------------------------------------------------------------------------
// 4. Build 3D radial-shaded feline face mask with stripes/rosettes & eye cutouts
// ------------------------------------------------------------------------------
static std::string build3dCatHeadSvg(const std::string& tag, const std::string& pattern, const std::string& f1, const std::string& f2, const std::string& f3, const std::string& mh, const std::string& mm, const std::string& stripeHex)
{
    std::string defs =
        svgTag("radialGradient", "id=\"cf" + tag + "\" cx=\"45%\" cy=\"30%\" r=\"70%\"", false) +
        svgTag("stop", "offset=\"0%\" stop-color=\"" + f1 + "\"") +
        svgTag("stop", "offset=\"56%\" stop-color=\"" + f2 + "\"") +
        svgTag("stop", "offset=\"100%\" stop-color=\"" + f3 + "\"") + svgClose("radialGradient") +
        svgTag("radialGradient", "id=\"cmz" + tag + "\" cx=\"48%\" cy=\"40%\" r=\"62%\"", false) +
        svgTag("stop", "offset=\"0%\" stop-color=\"" + mh + "\"") +
        svgTag("stop", "offset=\"100%\" stop-color=\"" + mm + "\"") + svgClose("radialGradient") +
        svgTag("mask", "id=\"cmk" + tag + "\"", false) + getStandardEyeCutoutMask() + svgClose("mask");

    std::string skull = "M 300,192 C 445,192 522,272 526,396 C 530,505 478,618 395,688 C 355,722 332,742 300,742 C 268,742 245,722 205,688 C 122,618 70,505 74,396 C 78,272 155,192 300,192 Z";
    std::ostringstream det;
    det << svgTag("ellipse", "cx=\"246\" cy=\"548\" rx=\"72\" ry=\"56\" fill=\"url(#cmz" + tag + ")\"")
        << svgTag("ellipse", "cx=\"354\" cy=\"548\" rx=\"72\" ry=\"56\" fill=\"url(#cmz" + tag + ")\"")
        << svgTag("ellipse", "cx=\"300\" cy=\"625\" rx=\"64\" ry=\"54\" fill=\"url(#cmz" + tag + ")\"");

    if (pattern == "tabby" || pattern == "tiger" || pattern == "cheshire") {
        det << svgTag("path", "d=\"M 255,212 L 274,256 L 300,224 L 326,256 L 345,212\" fill=\"none\" stroke=\"" + stripeHex + "\" stroke-width=\"9\" stroke-linecap=\"round\" stroke-linejoin=\"round\" opacity=\"0.78\"")
            << svgTag("path", "d=\"M 86,415 Q 145,425 182,442 M 82,462 Q 142,468 176,482 M 514,415 Q 455,425 418,442 M 518,462 Q 458,468 424,482\" fill=\"none\" stroke=\"" + stripeHex + "\" stroke-width=\"11\" stroke-linecap=\"round\" opacity=\"0.75\"");
    } else if (pattern == "leopard") {
        det << svgTag("circle", "cx=\"145\" cy=\"265\" r=\"14\" fill=\"none\" stroke=\"" + stripeHex + "\" stroke-width=\"6\" opacity=\"0.72\"")
            << svgTag("circle", "cx=\"455\" cy=\"265\" r=\"14\" fill=\"none\" stroke=\"" + stripeHex + "\" stroke-width=\"6\" opacity=\"0.72\"")
            << svgTag("circle", "cx=\"115\" cy=\"455\" r=\"16\" fill=\"none\" stroke=\"" + stripeHex + "\" stroke-width=\"6\" opacity=\"0.72\"")
            << svgTag("circle", "cx=\"485\" cy=\"455\" r=\"16\" fill=\"none\" stroke=\"" + stripeHex + "\" stroke-width=\"6\" opacity=\"0.72\"");
    }

    std::string body =
        svgTag("g", "mask=\"url(#cmk" + tag + ")\"", false) +
        svgTag("path", "d=\"" + skull + "\" fill=\"url(#cf" + tag + ")\"") + det.str() +
        svgTag("ellipse", "cx=\"190\" cy=\"352\" rx=\"76\" ry=\"54\" fill=\"#000000\" opacity=\"0.28\"") +
        svgTag("ellipse", "cx=\"410\" cy=\"352\" rx=\"76\" ry=\"54\" fill=\"#000000\" opacity=\"0.28\"") +
        svgClose("g");
    return wrapSvgCanvas(body, defs);
}
// ------------------------------------------------------------------------------
// 5. Build 3D pointed feline ears with inner pink/dark cartilage triangles
// ------------------------------------------------------------------------------
static std::string build3dCatEarsSvg(const std::string& tag, const std::string& outHi, const std::string& outDeep, const std::string& inHex)
{
    std::string defs =
        svgTag("linearGradient", "id=\"ceo" + tag + "\" x1=\"0%\" y1=\"0%\" x2=\"100%\" y2=\"100%\"", false) +
        svgTag("stop", "offset=\"0%\" stop-color=\"" + outHi + "\"") +
        svgTag("stop", "offset=\"100%\" stop-color=\"" + outDeep + "\"") + svgClose("linearGradient");

    std::string body =
        svgTag("path", "d=\"M 98,325 L 74,118 L 235,232 Z\" fill=\"url(#ceo" + tag + ")\" stroke=\"" + outDeep + "\" stroke-width=\"4\" stroke-linejoin=\"round\"") +
        svgTag("path", "d=\"M 114,296 L 98,158 L 204,234 Z\" fill=\"" + inHex + "\" opacity=\"0.86\"") +
        svgTag("path", "d=\"M 502,325 L 526,118 L 365,232 Z\" fill=\"url(#ceo" + tag + ")\" stroke=\"" + outDeep + "\" stroke-width=\"4\" stroke-linejoin=\"round\"") +
        svgTag("path", "d=\"M 486,296 L 502,158 L 396,234 Z\" fill=\"" + inHex + "\" opacity=\"0.86\"");
    return wrapSvgCanvas(body, defs);
}

// ------------------------------------------------------------------------------
// 6. Build 3D triangular feline nose, philtrum & sweeping 3D whiskers
// ------------------------------------------------------------------------------
static std::string build3dCatNoseWhiskersSvg(const std::string& tag, const std::string& noseHi, const std::string& noseDeep, const std::string& whiskerHex)
{
    std::string defs =
        svgTag("radialGradient", "id=\"cns" + tag + "\" cx=\"45%\" cy=\"30%\" r=\"68%\"", false) +
        svgTag("stop", "offset=\"0%\" stop-color=\"" + noseHi + "\"") +
        svgTag("stop", "offset=\"100%\" stop-color=\"" + noseDeep + "\"") + svgClose("radialGradient");

    std::ostringstream body;
    body << svgTag("path", "d=\"M 266,472 Q 300,462 334,472 Q 336,496 300,514 Q 264,496 266,472 Z\" fill=\"url(#cns" + tag + ")\"")
         << svgTag("path", "d=\"M 300,514 L 300,542\" stroke=\"" + noseDeep + "\" stroke-width=\"5\" stroke-linecap=\"round\"")
         << svgTag("path", "d=\"M 225,522 Q 130,498 38,512 M 222,542 Q 125,538 32,562 M 228,562 Q 138,576 54,608\" fill=\"none\" stroke=\"" + whiskerHex + "\" stroke-width=\"3.5\" stroke-linecap=\"round\" opacity=\"0.88\"")
         << svgTag("path", "d=\"M 375,522 Q 470,498 562,512 M 378,542 Q 475,538 568,562 M 372,562 Q 462,576 546,608\" fill=\"none\" stroke=\"" + whiskerHex + "\" stroke-width=\"3.5\" stroke-linecap=\"round\" opacity=\"0.88\"");
    return wrapSvgCanvas(body.str(), defs);
}
// ==============================================================================
// 7. 2,000-STRAND SILK LION MANE & FELINE ACCESSORIES
// ==============================================================================
// ------------------------------------------------------------------------------
// 8. Build multi-layered 2,000-strand silk lion mane or lynx ruff
// ------------------------------------------------------------------------------
static std::string buildLionManeSvg(const std::string& cDeep, const std::string& cDark, const std::string& cMid, const std::string& cHi)
{
    std::mt19937 rng(991122);
    std::uniform_real_distribution<float> uni(0.0f, 1.0f);
    std::ostringstream ss;

    auto pickCol = [&](float v) -> const std::string& {
        if (v < 0.25f) return cDeep;
        if (v < 0.55f) return cDark;
        if (v < 0.85f) return cMid;
        return cHi;
    };

    ss << svgTag("path", "d=\"M 42,610 C 8,405 38,145 168,108 C 235,88 365,88 432,108 C 562,145 592,405 558,610 L 496,555 C 514,365 480,205 300,205 C 120,205 86,365 104,555 Z\" fill=\"" + cDeep + "\" opacity=\"0.92\"");

    for (int i = 0; i < 2000; ++i) {
        float theta = -3.85f + uni(rng) * 4.55f;
        float v = uni(rng);
        float x0 = 300.0f + std::cos(theta) * (202.0f + v * 66.0f);
        float y0 = 418.0f + std::sin(theta) * (238.0f + v * 58.0f);
        if (y0 > 218.0f && std::fabs(x0 - 300.0f) < 194.0f) continue;

        float side = (x0 >= 300.0f) ? 1.0f : -1.0f;
        float hlen = 52.0f + v * 32.0f;
        float angDeg = (y0 < 205.0f) ? (theta * 57.2958f * 0.45f + 50.0f + (uni(rng) - 0.5f) * 22.0f)
                                       : (90.0f + side * (18.0f - v * 9.0f) + (uni(rng) - 0.5f) * 18.0f);
        float rad = angDeg * 0.0174533f;
        float x1 = x0 + std::cos(rad) * hlen;
        float y1 = y0 + std::sin(rad) * hlen;
        float wave = 8.0f * std::sin(v * 11.0f);
        ss << svgTag("path", "d=\"M " + std::to_string(x0) + "," + std::to_string(y0) + " Q " + std::to_string((x0 + x1) * 0.5f + wave) + "," + std::to_string((y0 + y1) * 0.5f) + " " + std::to_string(x1) + "," + std::to_string(y1) + "\" fill=\"none\" stroke=\"" + pickCol(v) + "\" stroke-width=\"2.0\" stroke-linecap=\"round\" opacity=\"0.84\"");
    }
    return wrapSvgCanvas(ss.str());
}

// ------------------------------------------------------------------------------
// 9. Build feline crown, witch hat, monocle & cheek blush accessories
// ------------------------------------------------------------------------------
static std::string buildCatAccessorySvg(const std::string& kind)
{
    if (kind == "crown") {
        return wrapSvgCanvas(
            svgTag("polygon", "points=\"185,204 198,108 248,158 300,92 352,158 402,108 415,204\" fill=\"#facc15\" stroke=\"#92400e\" stroke-width=\"4\"") +
            svgTag("rect", "x=\"185\" y=\"186\" width=\"230\" height=\"20\" rx=\"5\" fill=\"#dc2626\""));
    }
    if (kind == "monocle") {
        return wrapSvgCanvas(
            svgTag("circle", "cx=\"410\" cy=\"352\" r=\"62\" fill=\"#38bdf8\" opacity=\"0.22\" stroke=\"#facc15\" stroke-width=\"8\"") +
            svgTag("path", "d=\"M 472,352 Q 515,445 495,585\" fill=\"none\" stroke=\"#facc15\" stroke-width=\"4\" stroke-dasharray=\"6 5\""));
    }
    return wrapSvgCanvas(
        svgTag("ellipse", "cx=\"158\" cy=\"485\" rx=\"42\" ry=\"24\" fill=\"#fb7185\" opacity=\"0.40\"") +
        svgTag("ellipse", "cx=\"442\" cy=\"485\" rx=\"42\" ry=\"24\" fill=\"#fb7185\" opacity=\"0.40\""));
}

// ------------------------------------------------------------------------------
// 9.1 Build 3D-shaded scurrying mouse sprite with pink ears, nose & curling tail
// ------------------------------------------------------------------------------
static std::string buildMouseSpriteSvg(void)
{
    std::string defs =
        svgTag("radialGradient", "id=\"msBody\" cx=\"45%\" cy=\"35%\" r=\"65%\"", false) +
        svgTag("stop", "offset=\"0%\" stop-color=\"#cbd5e1\"") +
        svgTag("stop", "offset=\"60%\" stop-color=\"#64748b\"") +
        svgTag("stop", "offset=\"100%\" stop-color=\"#1e293b\"") + svgClose("radialGradient");

    std::string body =
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

    return wrapSvgCanvas(body, defs, 150, 90);
}

// ==============================================================================
// 10. MAIN GENERATOR: 8 CAT SPECIES, 127+ PROFILES & ZIP ARCHIVE (packs/cats.zip)
// ==============================================================================
struct CatSpeciesSpec {
    const char* name;
    const char* pattern;
    const char* f1;
    const char* f2;
    const char* f3;
    const char* mh;
    const char* mm;
    const char* stripeHex;
    const char* earInHex;
    const char* noseHi;
    const char* noseDeep;
    const char* whiskerHex;
    int         bgIdx;
    int         fxMask;
    int         hairId;
    int         hatId;
    int         glassId;
    int         makeupId;
    int         teethId;
    int         eyeStyle;
    int         animId;
};

// ------------------------------------------------------------------------------
// 11. Generate rooftop/savanna backdrops, 8 cats, 50 side-profile pairs & ZIP
// ------------------------------------------------------------------------------
int main(void)
{
    ensureBaseIni();
    generateDefaultFxBank();
    generateDefaultBrowsBank();
    exportSvgToPng("fx/mouse", buildMouseSpriteSvg(), 150, 90, 0.0f);

    static constexpr std::array<CatSceneSpec, 4> kScenes = {{
        {"#090d16", "#1e1b4b", "#fef08a", 445, 155, "#0f172a", "#1e293b", "#020617", true},
        {"#1e3a8a", "#93c5fd", "#ffffff", 165, 145, "#334155", "#1e293b", "#0f172a", false},
        {"#7c2d12", "#fb923c", "#fef08a", 470, 210, "#9a3412", "#451a03", "#1c0a00", false},
        {"#311042", "#c026d3", "#f0abfc", 300, 140, "#4a044e", "#2e1065", "#090d16", true}
    }};

    std::vector<int> bgs;
    for (const auto& sc : kScenes)
    {
        bgs.push_back(addCatBackdropScene(sc));
    }

    int hrLion  = saveSvgAsset("hair", buildLionManeSvg("#451a03", "#92400e", "#d97706", "#fde047"));
    int hrWhite = saveSvgAsset("hair", buildLionManeSvg("#475569", "#94a3b8", "#e2e8f0", "#ffffff"));
    int htCrown = saveSvgAsset("hat", buildCatAccessorySvg("crown"));
    int glMono  = saveSvgAsset("glasses", buildCatAccessorySvg("monocle"));
    int mkBlush = saveSvgAsset("makeup", buildCatAccessorySvg("blush"));

    std::array<CatSpeciesSpec, 8> cats = {{
        {"Midnight_Cat",  "solid",    "#334155", "#1e293b", "#090d16", "#475569", "#1e293b", "#020617", "#f472b6", "#fb7185", "#be123c", "#f8fafc", bgs.at(0), 16384, 0,       0,       0,      0,       3,  74,  16},
        {"Orange_Tabby",  "tabby",    "#fdba74", "#ea580c", "#7c2d12", "#fff7ed", "#fed7aa", "#9a3412", "#fda4af", "#fb7185", "#be123c", "#ffffff", bgs.at(1), 16384, 0,       0,       0,      mkBlush, 1,  96,  24},
        {"Golden_Lion",   "solid",    "#fde047", "#d97706", "#78350f", "#fef9c3", "#fde68a", "#92400e", "#fdba74", "#78350f", "#2b1205", "#fef9c3", bgs.at(2), 0,     hrLion,  htCrown, 0,      0,       8,  74,  41},
        {"Bengal_Tiger",  "tiger",    "#fb923c", "#ea580c", "#7c2d12", "#ffffff", "#fed7aa", "#090d16", "#fda4af", "#fb7185", "#9f1239", "#ffffff", bgs.at(2), 0,     0,       0,       0,      0,       8,  74,  19},
        {"Snow_Leopard",  "leopard",  "#f8fafc", "#cbd5e1", "#64748b", "#ffffff", "#e2e8f0", "#334155", "#fda4af", "#94a3b8", "#334155", "#ffffff", bgs.at(1), 0,     hrWhite, 0,       0,      mkBlush, 3,  96,  42},
        {"Cheshire_Cat",  "cheshire", "#f472b6", "#c026d3", "#581c87", "#fae8ff", "#f5d0fe", "#4a044e", "#f0abfc", "#e879f9", "#86198f", "#fdf4ff", bgs.at(3), 1024,  0,       0,       glMono, mkBlush, 13, 126, 54},
        {"Siamese_Cat",   "solid",    "#fef3c7", "#d6b88d", "#451a03", "#5c3a21", "#2b1608", "#2b1608", "#fda4af", "#451a03", "#1c0a00", "#fef9c3", bgs.at(1), 0,     0,       0,       glMono, 0,       1,  96,  18},
        {"Panther_King",  "solid",    "#1e1b4b", "#0f172a", "#020617", "#312e81", "#1e1b4b", "#020617", "#a855f7", "#6366f1", "#312e81", "#c4b5fd", bgs.at(0), 512,   0,       htCrown, 0,      0,       3,  126, 48}
    }};

    for (size_t i = 0; i < cats.size(); ++i)
    {
        const auto& c = cats.at(i);
        std::string tag = "c" + std::to_string(i);
        int fId = saveSvgAsset("face", build3dCatHeadSvg(tag, c.pattern, c.f1, c.f2, c.f3, c.mh, c.mm, c.stripeHex));
        int eId = saveSvgAsset("ears", build3dCatEarsSvg(tag, c.f1, c.f3, c.earInHex));
        int nId = saveSvgAsset("nose", build3dCatNoseWhiskersSvg(tag, c.noseHi, c.noseDeep, c.whiskerHex));
        appendFaceOffsets(fId, {{"mouth_y", "12"}, {"brows_y", "-4"}});
        appendMacroCombo({fId, c.makeupId, eId, nId, 0, c.glassId, c.hairId, c.hatId, 1, c.bgIdx, c.fxMask, c.teethId, 0, 0, c.eyeStyle, -1, c.animId, false, -1});
    }

    std::vector<ProfileThemeSpec> profThemes;
    for (size_t i = 0; i < 10; ++i)
    {
        const auto& c = cats.at(i % cats.size());
        profThemes.push_back({
            c.f1, c.f2, c.f3, c.mh, c.mm, c.f3,
            c.stripeHex, c.f3, c.f2,
            52, c.bgIdx, c.teethId, 0, 0, c.eyeStyle, c.animId, -1,
            (c.makeupId != 0), true, (c.glassId != 0), (c.hatId != 0)
        });
    }
    generateStandardSideProfileSuite(profThemes, 5);

    cleanSvgAndSaveZip("cats");
    std::cout << "[OK] 100% C++17 3D Cats & Big Cats + 50 Side-Profile Pairs (127..226) zipped to packs/cats.zip!\n";
    return 0;
}
