// ==============================================================================
// MakingFaces - Theme Pack 2 (C++17): 60 Horror Icons (6x10) + Side Profiles
// 1. 10 Skulls | 2. 10 Pumpkins | 3. 10 Vampires | 4. 10 Frankensteins
// 5. 10 Werewolves | 6. 10 Mummies, Zombies, Witches & Phantoms
// ==============================================================================

// ==============================================================================
// 1. INCLUDES AND SPOOKY 5-LAYER PARALLAX BACKDROP BUILDERS
// ==============================================================================
#include "pack_utils.h"
#include <array>
#include <cmath>
#include <cstdio>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

struct SpookySceneSpec {
    const char* sTop;
    const char* sBot;
    const char* moonHex;
    const char* farHex;
    const char* midHex;
    const char* frontHex;
    bool        gravestones;
};

// ------------------------------------------------------------------------------
// 2. Render 5-layer depth-of-field graveyard & wrought-iron gate scenes
// ------------------------------------------------------------------------------
static int addSpookyBackdropScene(const SpookySceneSpec& sc)
{
    int idx = nextAssetIdx("bg");
    std::array<char, 24> prefix{};
    std::snprintf(prefix.data(), prefix.size(), "bg/%03d", idx);
    std::string pfx(prefix.data());
    std::string idS = std::to_string(idx);

    std::string sDefs =
        svgTag("linearGradient", "id=\"hsk" + idS + "\" x1=\"0%\" y1=\"0%\" x2=\"0%\" y2=\"100%\"", false) +
        svgTag("stop", std::string("offset=\"0%\" stop-color=\"") + sc.sTop + "\"") +
        svgTag("stop", std::string("offset=\"100%\" stop-color=\"") + sc.sBot + "\"") + svgClose("linearGradient") +
        svgTag("radialGradient", "id=\"hmn" + idS + "\" cx=\"50%\" cy=\"50%\" r=\"50%\"", false) +
        svgTag("stop", "offset=\"0%\" stop-color=\"#ffffff\" stop-opacity=\"0.95\"") +
        svgTag("stop", std::string("offset=\"55%\" stop-color=\"") + sc.moonHex + "\" stop-opacity=\"0.85\"") +
        svgTag("stop", std::string("offset=\"100%\" stop-color=\"") + sc.moonHex + "\" stop-opacity=\"0.0\"") + svgClose("radialGradient");

    std::string sky =
        svgTag("rect", "x=\"-10\" y=\"-10\" width=\"620\" height=\"820\" fill=\"url(#hsk" + idS + ")\"") +
        svgTag("circle", "cx=\"445\" cy=\"155\" r=\"130\" fill=\"url(#hmn" + idS + ")\"") +
        svgTag("circle", std::string("cx=\"445\" cy=\"155\" r=\"48\" fill=\"") + sc.moonHex + "\"");
    exportSvgToPng(pfx + "_sky", wrapSvgCanvas(sky, sDefs), 600, 800, 0.0f);

    std::string clouds =
        svgTag("ellipse", "cx=\"185\" cy=\"165\" rx=\"165\" ry=\"28\" fill=\"#94a3b8\" opacity=\"0.30\"") +
        svgTag("ellipse", "cx=\"435\" cy=\"195\" rx=\"175\" ry=\"32\" fill=\"#64748b\" opacity=\"0.28\"");
    exportSvgToPng(pfx + "_clouds", wrapSvgCanvas(clouds), 600, 800, 6.0f);

    std::string far =
        svgTag("path", std::string("d=\"M -20,545 L 95,545 L 95,380 L 145,320 L 195,380 L 195,515 L 395,515 L 395,355 L 445,295 L 495,355 L 495,530 L 620,530 L 620,820 L -20,820 Z\" fill=\"") + sc.farHex + "\"");
    exportSvgToPng(pfx + "_far", wrapSvgCanvas(far), 600, 800, 9.0f);

    std::ostringstream mid;
    mid << svgTag("path", std::string("d=\"M -20,665 Q 150,595 300,645 Q 450,595 620,665 L 620,820 L -20,820 Z\" fill=\"") + sc.midHex + "\"");
    if (sc.gravestones)
    {
        mid << svgTag("path", std::string("d=\"M 72,645 L 72,545 Q 102,515 132,545 L 132,645 Z\" fill=\"") + sc.midHex + "\"")
            << svgTag("path", std::string("d=\"M 472,645 L 472,535 Q 504,505 536,535 L 536,645 Z\" fill=\"") + sc.midHex + "\"")
            << svgTag("rect", std::string("x=\"498\" y=\"480\" width=\"12\" height=\"42\" fill=\"") + sc.midHex + "\"")
            << svgTag("rect", std::string("x=\"486\" y=\"492\" width=\"36\" height=\"10\" fill=\"") + sc.midHex + "\"");
    }
    exportSvgToPng(pfx + "_mid", wrapSvgCanvas(mid.str()), 600, 800, 2.5f);

    std::ostringstream front;
    front << svgTag("rect", "x=\"-10\" y=\"678\" width=\"620\" height=\"8\" fill=\"#090d16\"")
          << svgTag("rect", "x=\"-10\" y=\"778\" width=\"620\" height=\"26\" fill=\"#090d16\"")
          << svgTag("rect", "x=\"8\" y=\"620\" width=\"24\" height=\"185\" fill=\"#090d16\"")
          << svgTag("polygon", "points=\"4,620 20,596 36,620\" fill=\"#090d16\"")
          << svgTag("rect", "x=\"568\" y=\"620\" width=\"24\" height=\"185\" fill=\"#090d16\"")
          << svgTag("polygon", "points=\"564,620 580,596 596,620\" fill=\"#090d16\"");

    for (int bx = 54; bx <= 546; bx += 38)
    {
        int barTop = 648 - ((bx == 282 || bx == 320) ? 16 : 0);
        front << svgTag("rect", "x=\"" + std::to_string(bx - 4) + "\" y=\"" + std::to_string(barTop) + "\" width=\"8\" height=\"" + std::to_string(805 - barTop) + "\" fill=\"#090d16\"")
              << svgTag("polygon", "points=\"" + std::to_string(bx - 8) + "," + std::to_string(barTop) + " " + std::to_string(bx) + "," + std::to_string(barTop - 16) + " " + std::to_string(bx + 8) + "," + std::to_string(barTop) + "\" fill=\"#090d16\"");
    }

    exportSvgToPng(pfx + "_front", wrapSvgCanvas(front.str()), 600, 800, 0.0f);
    return idx;
}

// ==============================================================================
// 3. 3D SKULLS, JAWS, PUMPKINS, MONSTER HEADS & SPOOKY ACCESSORIES
// ==============================================================================

// ------------------------------------------------------------------------------
// 4. Build smooth 3D skull cranium with upper teeth row
// ------------------------------------------------------------------------------
static std::string build3dSkullCraniumSvg(const std::string& tag, const std::string& style, const std::string& b1, const std::string& b2, const std::string& b3, const std::string& toothHex)
{
    std::string defs =
        svgTag("radialGradient", "id=\"skg" + tag + "\" cx=\"45%\" cy=\"28%\" r=\"70%\"", false) +
        svgTag("stop", "offset=\"0%\" stop-color=\"" + b1 + "\"") +
        svgTag("stop", "offset=\"56%\" stop-color=\"" + b2 + "\"") +
        svgTag("stop", "offset=\"100%\" stop-color=\"" + b3 + "\"") + svgClose("radialGradient") +
        svgTag("mask", "id=\"skm" + tag + "\"", false) + getStandardEyeCutoutMask() + svgClose("mask");

    std::string cranium = "M 300,178 C 448,178 518,262 514,386 C 510,455 478,505 436,534 L 418,596 L 182,596 L 164,534 C 122,505 90,455 86,386 C 82,262 152,178 300,178 Z";
    std::ostringstream det;
    det << svgTag("path", "d=\"M 192,568 L 408,568 L 408,645 L 192,645 Z\" fill=\"#070506\"")
        << svgTag("path", "d=\"M 300,455 L 274,522 Q 300,532 326,522 Z\" fill=\"#090708\"");

    for (int i = 0; i < 8; ++i)
    {
        int tx = 212 + i * 22;
        std::string col = (style == "pirate" && i == 5) ? "#facc15" : toothHex;
        det << svgTag("rect", "x=\"" + std::to_string(tx) + "\" y=\"566\" width=\"19\" height=\"26\" rx=\"5\" fill=\"" + col + "\" stroke=\"" + b3 + "\" stroke-width=\"2.5\"");
    }

    if (style == "calavera")
    {
        det << svgTag("circle", "cx=\"300\" cy=\"244\" r=\"16\" fill=\"#ec4899\"")
            << svgTag("circle", "cx=\"300\" cy=\"244\" r=\"7\" fill=\"#facc15\"")
            << svgTag("path", "d=\"M 135,445 Q 165,425 185,455 Q 155,475 135,445 Z M 465,445 Q 435,425 415,455 Q 445,475 465,445 Z\" fill=\"#06b6d4\"");
    }

    std::string body =
        svgTag("g", "mask=\"url(#skm" + tag + ")\"", false) +
        svgTag("path", "d=\"" + cranium + "\" fill=\"url(#skg" + tag + ")\"") + det.str() +
        svgTag("ellipse", "cx=\"190\" cy=\"352\" rx=\"72\" ry=\"52\" fill=\"#000000\" opacity=\"0.38\"") +
        svgTag("ellipse", "cx=\"410\" cy=\"352\" rx=\"72\" ry=\"52\" fill=\"#000000\" opacity=\"0.38\"") +
        svgClose("g");
    return wrapSvgCanvas(body, defs);
}

// ------------------------------------------------------------------------------
// 5. Build 3D angular articulated mandible jaw (saved to beard/097..106)
// ------------------------------------------------------------------------------
static std::string build3dArticulatedJawSvg(const std::string& tag, const std::string& b1, const std::string& b2, const std::string& b3, const std::string& toothHex)
{
    std::string defs =
        svgTag("radialGradient", "id=\"jw" + tag + "\" cx=\"50%\" cy=\"42%\" r=\"65%\"", false) +
        svgTag("stop", "offset=\"0%\" stop-color=\"" + b1 + "\"") +
        svgTag("stop", "offset=\"58%\" stop-color=\"" + b2 + "\"") +
        svgTag("stop", "offset=\"100%\" stop-color=\"" + b3 + "\"") + svgClose("radialGradient");

    std::ostringstream body;
    body << svgTag("path", "d=\"M 174,522 L 196,522 L 212,616 L 388,616 L 404,522 L 426,522 L 434,638 L 414,678 L 352,746 L 248,746 L 186,678 L 166,638 Z\" fill=\"url(#jw" + tag + ")\" stroke=\"" + b3 + "\" stroke-width=\"4\" stroke-linejoin=\"round\"")
         << svgTag("path", "d=\"M 218,672 L 254,734 L 346,734 L 382,672\" fill=\"none\" stroke=\"" + b3 + "\" stroke-width=\"4\" stroke-linecap=\"round\" opacity=\"0.42\"")
         << svgTag("path", "d=\"M 262,718 L 338,718\" fill=\"none\" stroke=\"" + b1 + "\" stroke-width=\"5\" stroke-linecap=\"round\" opacity=\"0.78\"");

    for (int i = 0; i < 8; ++i)
    {
        int tx = 212 + i * 22;
        body << svgTag("rect", "x=\"" + std::to_string(tx) + "\" y=\"594\" width=\"19\" height=\"24\" rx=\"4\" fill=\"" + toothHex + "\" stroke=\"" + b3 + "\" stroke-width=\"2.5\"");
    }
    return wrapSvgCanvas(body.str(), defs);
}
// ------------------------------------------------------------------------------
// 6. Build 3D pumpkins, furry 3D werewolves, vampires, Franks & 3D horror ears
// ------------------------------------------------------------------------------
static std::string build3dPumpkinHeadSvg(const std::string& tag, const std::string& p1, const std::string& p2, const std::string& p3, const std::string& stemHex, bool withWeb)
{
    std::string defs =
        svgTag("radialGradient", "id=\"pk" + tag + "\" cx=\"45%\" cy=\"32%\" r=\"68%\"", false) +
        svgTag("stop", "offset=\"0%\" stop-color=\"" + p1 + "\"") +
        svgTag("stop", "offset=\"56%\" stop-color=\"" + p2 + "\"") +
        svgTag("stop", "offset=\"100%\" stop-color=\"" + p3 + "\"") + svgClose("radialGradient") +
        svgTag("mask", "id=\"pkm" + tag + "\"", false) + getStandardEyeCutoutMask() + svgClose("mask");

    std::string web = withWeb ? (
        svgTag("path", "d=\"M 90,245 Q 155,285 195,220 M 105,285 Q 160,305 215,255\" fill=\"none\" stroke=\"#ffffff\" stroke-width=\"2.5\" opacity=\"0.65\"") +
        svgTag("circle", "cx=\"155\" cy=\"255\" r=\"9\" fill=\"#18181f\"") +
        svgTag("circle", "cx=\"445\" cy=\"485\" r=\"11\" fill=\"#18181f\"")
    ) : "";

    std::string body =
        svgTag("path", "d=\"M 278,195 C 272,135 312,92 355,105 C 335,135 322,165 325,195 Z\" fill=\"" + stemHex + "\" stroke=\"#052e16\" stroke-width=\"4\"") +
        svgTag("g", "mask=\"url(#pkm" + tag + ")\"", false) +
        svgTag("ellipse", "cx=\"148\" cy=\"468\" rx=\"86\" ry=\"245\" fill=\"url(#pk" + tag + ")\" stroke=\"" + p3 + "\" stroke-width=\"4\"") +
        svgTag("ellipse", "cx=\"452\" cy=\"468\" rx=\"86\" ry=\"245\" fill=\"url(#pk" + tag + ")\" stroke=\"" + p3 + "\" stroke-width=\"4\"") +
        svgTag("ellipse", "cx=\"218\" cy=\"468\" rx=\"92\" ry=\"262\" fill=\"url(#pk" + tag + ")\" stroke=\"" + p3 + "\" stroke-width=\"4\"") +
        svgTag("ellipse", "cx=\"382\" cy=\"468\" rx=\"92\" ry=\"262\" fill=\"url(#pk" + tag + ")\" stroke=\"" + p3 + "\" stroke-width=\"4\"") +
        svgTag("ellipse", "cx=\"300\" cy=\"468\" rx=\"96\" ry=\"272\" fill=\"url(#pk" + tag + ")\" stroke=\"" + p3 + "\" stroke-width=\"4\"") +
        svgTag("polygon", "points=\"300,455 272,512 328,512\" fill=\"#090708\"") +
        web +
        svgTag("ellipse", "cx=\"190\" cy=\"352\" rx=\"72\" ry=\"52\" fill=\"#000000\" opacity=\"0.35\"") +
        svgTag("ellipse", "cx=\"410\" cy=\"352\" rx=\"72\" ry=\"52\" fill=\"#000000\" opacity=\"0.35\"") +
        svgClose("g");
    return wrapSvgCanvas(body, defs);
}

static std::string build3dWerewolfHeadSvg(const std::string& tag, const std::string& fHi, const std::string& fMid, const std::string& fDeep, const std::string& muzzleHex)
{
    std::string defs =
        svgTag("radialGradient", "id=\"wf" + tag + "\" cx=\"48%\" cy=\"34%\" r=\"68%\"", false) +
        svgTag("stop", "offset=\"0%\" stop-color=\"" + fHi + "\"") +
        svgTag("stop", "offset=\"55%\" stop-color=\"" + fMid + "\"") +
        svgTag("stop", "offset=\"100%\" stop-color=\"" + fDeep + "\"") + svgClose("radialGradient") +
        svgTag("radialGradient", "id=\"wfm" + tag + "\" cx=\"50%\" cy=\"45%\" r=\"60%\"", false) +
        svgTag("stop", "offset=\"0%\" stop-color=\"" + muzzleHex + "\"") +
        svgTag("stop", "offset=\"75%\" stop-color=\"" + fMid + "\"") +
        svgTag("stop", "offset=\"100%\" stop-color=\"" + fDeep + "\"") + svgClose("radialGradient") +
        svgTag("mask", "id=\"wfmk" + tag + "\"", false) + getStandardEyeCutoutMask() + svgClose("mask");

    std::ostringstream fur;
    for (int i = 0; i < 160; ++i)
    {
        float t = static_cast<float>(i) / 159.0f;
        float ang = -2.55f + t * 5.10f;
        float radX = (std::fabs(ang) < 1.15f) ? 196.0f : (238.0f + (i % 5) * 6.0f);
        float radY = (ang > 1.3f || ang < -1.3f) ? 285.0f : 230.0f;
        int x1 = 300 + static_cast<int>(std::sin(ang) * (radX - 24.0f));
        int y1 = 445 - static_cast<int>(std::cos(ang) * (radY - 22.0f));
        int x2 = 300 + static_cast<int>(std::sin(ang) * (radX + 18.0f + (i % 4) * 4));
        int y2 = 445 - static_cast<int>(std::cos(ang) * (radY + 18.0f + (i % 4) * 4)) + 14;
        const std::string& col = (i % 3 == 0) ? fHi : ((i % 3 == 1) ? fDeep : "#090d16");
        fur << svgTag("line", "x1=\"" + std::to_string(x1) + "\" y1=\"" + std::to_string(y1) + "\" x2=\"" + std::to_string(x2) + "\" y2=\"" + std::to_string(y2) + "\" stroke=\"" + col + "\" stroke-width=\"2.0\" stroke-linecap=\"round\" opacity=\"0.86\"");
    }

    std::string body =
        svgTag("g", "mask=\"url(#wfmk" + tag + ")\"", false) +
        fur.str() +
        svgTag("path", "d=\"M 300,192 C 405,192 465,248 475,342 L 512,445 L 456,545 L 392,718 Q 300,768 208,718 L 144,545 L 88,445 L 125,342 C 135,248 195,192 300,192 Z\" fill=\"url(#wf" + tag + ")\"") +
        svgTag("path", "d=\"M 192,475 C 218,415 382,415 408,475 C 432,545 405,675 300,712 C 195,675 168,545 192,475 Z\" fill=\"url(#wfm" + tag + ")\"") +
        svgTag("path", "d=\"M 252,332 L 284,388 L 300,372 L 316,388 L 348,332\" fill=\"none\" stroke=\"" + fDeep + "\" stroke-width=\"6\" stroke-linecap=\"round\" opacity=\"0.65\"") +
        svgTag("ellipse", "cx=\"190\" cy=\"352\" rx=\"66\" ry=\"46\" fill=\"#000000\" opacity=\"0.34\"") +
        svgTag("ellipse", "cx=\"410\" cy=\"352\" rx=\"66\" ry=\"46\" fill=\"#000000\" opacity=\"0.34\"") +
        svgClose("g");
    return wrapSvgCanvas(body, defs);
}
static std::string build3dHorrorHeadSvg(const std::string& tag, const std::string& kind, const std::string& c1, const std::string& c2, const std::string& c3, const std::string& stroke)
{
    std::string defs =
        svgTag("radialGradient", "id=\"hh" + tag + "\" cx=\"46%\" cy=\"36%\" r=\"66%\"", false) +
        svgTag("stop", "offset=\"0%\" stop-color=\"" + c1 + "\"") +
        svgTag("stop", "offset=\"52%\" stop-color=\"" + c2 + "\"") +
        svgTag("stop", "offset=\"86%\" stop-color=\"" + c3 + "\"") +
        svgTag("stop", "offset=\"100%\" stop-color=\"" + stroke + "\"") + svgClose("radialGradient") +
        svgTag("mask", "id=\"hhm" + tag + "\"", false) + getStandardEyeCutoutMask() + svgClose("mask");

    std::string shape = "M 300,205 C 445,202 510,255 515,380 C 520,510 488,655 425,725 C 385,762 338,770 300,770 C 262,770 215,762 175,725 C 112,655 80,510 85,380 C 90,255 155,202 300,205 Z";
    if (kind == "vamp_slender")
    {
        shape = "M 300,185 C 415,185 465,250 462,395 C 458,550 412,705 300,775 C 188,705 142,550 138,395 C 135,250 185,185 300,185 Z";
    }
    else if (kind == "frank")
    {
        shape = "M 136,162 L 464,162 L 486,435 L 456,718 L 144,718 L 114,435 Z";
    }

    std::ostringstream det;
    if (kind == "mummy")
    {
        for (int wy = 195; wy <= 705; wy += 42)
        {
            det << svgTag("path", "d=\"M 105," + std::to_string(wy) + " Q 300," + std::to_string(wy + 18) + " 495," + std::to_string(wy - 6) + "\" fill=\"none\" stroke=\"" + stroke + "\" stroke-width=\"9\" opacity=\"0.65\"");
        }
    }

    std::string body =
        svgTag("g", "mask=\"url(#hhm" + tag + ")\"", false) +
        svgTag("path", "d=\"" + shape + "\" fill=\"url(#hh" + tag + ")\" stroke=\"" + stroke + "\" stroke-width=\"6\" stroke-linejoin=\"round\"") +
        det.str() +
        svgTag("path", "d=\"M 190,702 Q 300,758 410,702 Q 300,730 190,702 Z\" fill=\"#000000\" opacity=\"0.22\"") +
        svgTag("ellipse", "cx=\"190\" cy=\"352\" rx=\"68\" ry=\"48\" fill=\"#000000\" opacity=\"0.28\"") +
        svgTag("ellipse", "cx=\"410\" cy=\"352\" rx=\"68\" ry=\"48\" fill=\"#000000\" opacity=\"0.28\"") +
        svgClose("g");
    return wrapSvgCanvas(body, defs);
}

static std::string build3dHorrorEarsSvg(const std::string& tag, const std::string& kind, const std::string& hi, const std::string& mid, const std::string& deep)
{
    std::string defs =
        svgTag("radialGradient", "id=\"eg" + tag + "\" cx=\"45%\" cy=\"35%\" r=\"70%\"", false) +
        svgTag("stop", "offset=\"0%\" stop-color=\"" + hi + "\"") +
        svgTag("stop", "offset=\"55%\" stop-color=\"" + mid + "\"") +
        svgTag("stop", "offset=\"100%\" stop-color=\"" + deep + "\"") + svgClose("radialGradient");

    if (kind == "nosferatu")
    {
        return wrapSvgCanvas(
            svgTag("path", "d=\"M 118,305 L 18,172 L 42,485 L 116,508 Z\" fill=\"url(#eg" + tag + ")\" stroke=\"" + deep + "\" stroke-width=\"4\"") +
            svgTag("path", "d=\"M 102,325 L 38,225 L 55,465 L 102,480 Z\" fill=\"" + deep + "\" opacity=\"0.48\"") +
            svgTag("path", "d=\"M 482,305 L 582,172 L 558,485 L 484,508 Z\" fill=\"url(#eg" + tag + ")\" stroke=\"" + deep + "\" stroke-width=\"4\"") +
            svgTag("path", "d=\"M 498,325 L 562,225 L 545,465 L 498,480 Z\" fill=\"" + deep + "\" opacity=\"0.48\""), defs);
    }
    if (kind == "wolf")
    {
        return wrapSvgCanvas(
            svgTag("path", "d=\"M 118,288 L 42,84 L 204,212 Z\" fill=\"url(#eg" + tag + ")\" stroke=\"" + deep + "\" stroke-width=\"4\"") +
            svgTag("path", "d=\"M 124,264 L 68,124 L 180,214 Z\" fill=\"#090d16\" opacity=\"0.65\"") +
            svgTag("path", "d=\"M 482,288 L 558,84 L 396,212 Z\" fill=\"url(#eg" + tag + ")\" stroke=\"" + deep + "\" stroke-width=\"4\"") +
            svgTag("path", "d=\"M 476,264 L 532,124 L 420,214 Z\" fill=\"#090d16\" opacity=\"0.65\""), defs);
    }
    if (kind == "horns")
    {
        return wrapSvgCanvas(
            svgTag("path", "d=\"M 142,272 Q 52,188 74,78 Q 128,148 186,232 Z\" fill=\"url(#eg" + tag + ")\" stroke=\"" + deep + "\" stroke-width=\"4\"") +
            svgTag("path", "d=\"M 458,272 Q 548,188 526,78 Q 472,148 414,232 Z\" fill=\"url(#eg" + tag + ")\" stroke=\"" + deep + "\" stroke-width=\"4\""), defs);
    }
    return wrapSvgCanvas(
        svgTag("rect", "x=\"36\" y=\"345\" width=\"54\" height=\"96\" rx=\"12\" fill=\"url(#eg" + tag + ")\" stroke=\"" + deep + "\" stroke-width=\"4\"") +
        svgTag("rect", "x=\"510\" y=\"345\" width=\"54\" height=\"96\" rx=\"12\" fill=\"url(#eg" + tag + ")\" stroke=\"" + deep + "\" stroke-width=\"4\""), defs);
}

static std::string buildSculpted3dNoseSvg(const std::string& tag, const std::string& hi, const std::string& mid, const std::string& deep)
{
    std::string defs =
        svgTag("linearGradient", "id=\"ng" + tag + "\" x1=\"20%\" y1=\"10%\" x2=\"80%\" y2=\"90%\"", false) +
        svgTag("stop", "offset=\"0%\" stop-color=\"" + hi + "\"") +
        svgTag("stop", "offset=\"55%\" stop-color=\"" + mid + "\"") +
        svgTag("stop", "offset=\"100%\" stop-color=\"" + deep + "\"") + svgClose("linearGradient");

    std::string body =
        svgTag("path", "d=\"M 284,370 C 272,430 250,475 268,492 C 288,505 312,505 332,492 C 350,475 328,430 316,370 Z\" fill=\"url(#ng" + tag + ")\" stroke=\"" + deep + "\" stroke-width=\"4\"") +
        svgTag("ellipse", "cx=\"282\" cy=\"490\" rx=\"10\" ry=\"6\" fill=\"" + deep + "\"") +
        svgTag("ellipse", "cx=\"318\" cy=\"490\" rx=\"10\" ry=\"6\" fill=\"" + deep + "\"");
    return wrapSvgCanvas(body, defs);
}

static std::string buildHorrorMakeupSvg(const std::string& kind)
{
    std::string defs = svgTag("mask", "id=\"hmk" + kind + "\"", false) + getStandardEyeCutoutMask() + svgClose("mask");
    std::ostringstream body;
    body << svgTag("g", "mask=\"url(#hmk" + kind + ")\"", false);
    if (kind == "vamp")
    {
        body << svgTag("ellipse", "cx=\"190\" cy=\"332\" rx=\"78\" ry=\"44\" fill=\"#2a2233\" opacity=\"0.68\"")
             << svgTag("ellipse", "cx=\"410\" cy=\"332\" rx=\"78\" ry=\"44\" fill=\"#2a2233\" opacity=\"0.68\"")
             << svgTag("path", "d=\"M 225,658 Q 225,705 234,718 Q 242,700 238,658 Z M 375,658 Q 375,702 368,712 Q 360,695 362,658 Z\" fill=\"#b91c1c\"");
    }
    else if (kind == "stitches")
    {
        body << svgTag("path", "d=\"M 135,445 L 235,495 M 155,435 L 145,465 M 185,450 L 175,480 M 215,465 L 205,495\" stroke=\"#0f172a\" stroke-width=\"5\" stroke-linecap=\"round\"")
             << svgTag("path", "d=\"M 215,198 L 238,248 L 222,282 L 246,312 M 392,212 L 368,258 L 384,292\" fill=\"none\" stroke=\"#1e293b\" stroke-width=\"4\" stroke-linecap=\"round\"");
    }
    else
    {
        body << svgTag("circle", "cx=\"155\" cy=\"255\" r=\"11\" fill=\"#18181f\"")
             << svgTag("path", "d=\"M 148,250 L 126,236 M 146,256 L 122,258 M 148,262 L 128,278 M 162,250 L 184,236 M 164,256 L 188,258 M 162,262 L 182,278\" stroke=\"#090d16\" stroke-width=\"3.5\" stroke-linecap=\"round\"")
             << svgTag("circle", "cx=\"152\" cy=\"246\" r=\"2.2\" fill=\"#ef4444\"")
             << svgTag("circle", "cx=\"158\" cy=\"246\" r=\"2.2\" fill=\"#ef4444\"");
    }
    body << svgClose("g");
    return wrapSvgCanvas(body.str(), defs);
}

static std::string buildHorrorAccessorySvg(const std::string& kind)
{
    if (kind == "bandana")
    {
        return wrapSvgCanvas(
            svgTag("path", "d=\"M 86,288 Q 300,142 514,288 L 498,236 Q 300,112 102,236 Z\" fill=\"#dc2626\" stroke=\"#7f1d1d\" stroke-width=\"4\""));
    }
    if (kind == "witchhat")
    {
        return wrapSvgCanvas(
            svgTag("ellipse", "cx=\"300\" cy=\"222\" rx=\"255\" ry=\"26\" fill=\"#1e1b4b\" stroke=\"#08080a\" stroke-width=\"4\"") +
            svgTag("path", "d=\"M 185,216 Q 235,115 265,45 Q 335,120 415,216 Z\" fill=\"#312e81\" stroke=\"#08080a\" stroke-width=\"4\"") +
            svgTag("rect", "x=\"276\" y=\"182\" width=\"48\" height=\"34\" rx=\"4\" fill=\"none\" stroke=\"#facc15\" stroke-width=\"7\""));
    }
    if (kind == "perchedbat")
    {
        return wrapSvgCanvas(
            svgTag("path", "d=\"M 300,136 Q 235,88 192,128 Q 225,152 248,140 Q 272,158 300,148 Z\" fill=\"#3f3f46\" stroke=\"#09090b\" stroke-width=\"3\"") +
            svgTag("path", "d=\"M 300,136 Q 365,88 408,128 Q 375,152 352,140 Q 328,158 300,148 Z\" fill=\"#3f3f46\" stroke=\"#09090b\" stroke-width=\"3\"") +
            svgTag("ellipse", "cx=\"300\" cy=\"140\" rx=\"16\" ry=\"24\" fill=\"#27272a\" stroke=\"#09090b\" stroke-width=\"3\"") +
            svgTag("circle", "cx=\"294\" cy=\"132\" r=\"3.5\" fill=\"#ef4444\"") +
            svgTag("circle", "cx=\"306\" cy=\"132\" r=\"3.5\" fill=\"#ef4444\""));
    }
    if (kind == "eyepatch")
    {
        return wrapSvgCanvas(
            svgTag("path", "d=\"M 118,236 L 502,394\" stroke=\"#090d16\" stroke-width=\"13\" stroke-linecap=\"round\"") +
            svgTag("ellipse", "cx=\"410\" cy=\"352\" rx=\"64\" ry=\"54\" fill=\"#090d16\" stroke=\"#334155\" stroke-width=\"4\""));
    }
    if (kind == "drachair")
    {
        return wrapSvgCanvas(
            svgTag("path", "d=\"M 96,355 C 88,185 185,132 300,224 C 415,132 512,185 504,355 C 492,235 395,215 300,275 C 205,215 108,235 96,355 Z\" fill=\"#090d16\""));
    }
    return wrapSvgCanvas(
        svgTag("path", "d=\"M 122,258 L 122,148 L 478,148 L 478,258 L 444,215 L 408,252 L 372,212 L 336,252 L 300,210 L 264,252 L 228,212 L 192,252 L 156,215 Z\" fill=\"#090d16\""));
}
// ------------------------------------------------------------------------------
// 7. Build 3D side-profile skull cranium & articulated mandible jaw
// ------------------------------------------------------------------------------
static std::string build3dProfileSkullSvg(const std::string& tag, const std::string& side, const std::string& b1, const std::string& b2, const std::string& b3, const std::string& toothHex)
{
    bool isRight = (side == "right");
    std::string defs =
        svgTag("radialGradient", "id=\"skp" + tag + "\" cx=\"55%\" cy=\"30%\" r=\"72%\"", false) +
        svgTag("stop", "offset=\"0%\" stop-color=\"" + b1 + "\"") +
        svgTag("stop", "offset=\"56%\" stop-color=\"" + b2 + "\"") +
        svgTag("stop", "offset=\"100%\" stop-color=\"" + b3 + "\"") + svgClose("radialGradient") +
        svgTag("mask", "id=\"skpm" + tag + "\"", false) +
        svgTag("rect", "x=\"0\" y=\"0\" width=\"600\" height=\"800\" fill=\"white\"") +
        svgTag("ellipse", "cx=\"185\" cy=\"356\" rx=\"52\" ry=\"38\" fill=\"black\"") +
        svgClose("mask");

    std::string xf = isRight ? " transform=\"translate(600,0) scale(-1,1)\"" : "";
    std::ostringstream body;
    body << svgTag("g", "mask=\"url(#skpm" + tag + ")\"" + xf, false)
         << svgTag("path", "d=\"M 0,175 L 92,175 C 192,175 242,248 248,346 C 258,385 288,412 294,444 C 296,464 276,476 262,482 L 274,566 L 166,566 L 154,515 C 115,522 86,568 62,625 L 0,625 Z\" fill=\"url(#skp" + tag + ")\"")
         << svgTag("path", "d=\"M 288,448 L 254,442 L 266,478 Z\" fill=\"#090708\"")
         << svgTag("ellipse", "cx=\"185\" cy=\"352\" rx=\"58\" ry=\"44\" fill=\"#000000\" opacity=\"0.38\"");

    for (int i = 0; i < 4; ++i)
    {
        int tx = 186 + i * 21;
        body << svgTag("rect", "x=\"" + std::to_string(tx) + "\" y=\"560\" width=\"18\" height=\"25\" rx=\"5\" fill=\"" + toothHex + "\" stroke=\"" + b3 + "\" stroke-width=\"2.5\"");
    }
    body << svgClose("g");
    return wrapSvgCanvas(body.str(), defs);
}

static std::string build3dProfileSkullJawSvg(const std::string& tag, const std::string& side, const std::string& b1, const std::string& b2, const std::string& b3, const std::string& toothHex)
{
    bool isRight = (side == "right");
    std::string defs =
        svgTag("linearGradient", "id=\"skpj" + tag + "\" x1=\"0%\" y1=\"0%\" x2=\"100%\" y2=\"100%\"", false) +
        svgTag("stop", "offset=\"0%\" stop-color=\"" + b1 + "\"") +
        svgTag("stop", "offset=\"60%\" stop-color=\"" + b2 + "\"") +
        svgTag("stop", "offset=\"100%\" stop-color=\"" + b3 + "\"") + svgClose("linearGradient");

    std::string xf = isRight ? " transform=\"translate(600,0) scale(-1,1)\"" : "";
    std::ostringstream body;
    body << svgTag("g", xf, false)
         << svgTag("path", "d=\"M 126,524 L 158,524 L 182,612 L 268,612 C 276,652 264,704 234,724 C 194,744 132,732 104,692 C 88,658 106,576 126,524 Z\" fill=\"url(#skpj" + tag + ")\" stroke=\"" + b3 + "\" stroke-width=\"4\" stroke-linejoin=\"round\"");

    for (int i = 0; i < 4; ++i)
    {
        int tx = 186 + i * 21;
        body << svgTag("rect", "x=\"" + std::to_string(tx) + "\" y=\"592\" width=\"18\" height=\"23\" rx=\"5\" fill=\"" + toothHex + "\" stroke=\"" + b3 + "\" stroke-width=\"2.5\"");
    }
    body << svgClose("g");
    return wrapSvgCanvas(body.str(), defs);
}
// ==============================================================================
// 8. MAIN GENERATOR: 60 HORROR PRESETS (6x10) + SIDE PROFILES + ZIP ARCHIVE
// ==============================================================================
struct PaletteTriple {
    const char* c1;
    const char* c2;
    const char* c3;
    const char* tooth;
};

// ------------------------------------------------------------------------------
// 9. Generate 10 Skulls, 10 Pumpkins, 10 Vampires, 10 Franks, 10 Wolves & 10 Mummies
// ------------------------------------------------------------------------------
// ------------------------------------------------------------------------------
// 9. Generate 60 Horror Icons + 50 Complete Side-Profile Pairs (127..226) & ZIP
// ------------------------------------------------------------------------------
int main(void)
{
    ensureBaseIni();
    generateDefaultFxBank();
    generateDefaultBrowsBank();

    static constexpr std::array<SpookySceneSpec, 4> kScenes = {{
        {"#090d16", "#1e1b4b", "#fef08a", "#0f172a", "#1e293b", "#020617", true},
        {"#1e1b4b", "#4c1d95", "#e2e8f0", "#1e1b4b", "#312e81", "#090d16", true},
        {"#450a0a", "#dc2626", "#facc15", "#7f1d1d", "#450a0a", "#1c0a00", false},
        {"#064e3b", "#10b981", "#fef9c3", "#065f46", "#064e3b", "#022c22", true}
    }};

    std::vector<int> bgs;
    for (const auto& sc : kScenes) bgs.push_back(addSpookyBackdropScene(sc));

    int htBandana = saveSvgAsset("hat", buildHorrorAccessorySvg("bandana"));
    int htWitch   = saveSvgAsset("hat", buildHorrorAccessorySvg("witchhat"));
    int htBat     = saveSvgAsset("hat", buildHorrorAccessorySvg("perchedbat"));
    int glPatch   = saveSvgAsset("glasses", buildHorrorAccessorySvg("eyepatch"));
    int erHorns   = saveSvgAsset("ears", build3dHorrorEarsSvg("er1", "horns",     "#fef08a", "#ea580c", "#450a0a"));
    int erBolts   = saveSvgAsset("ears", build3dHorrorEarsSvg("er2", "bolts",     "#f8fafc", "#64748b", "#0f172a"));
    int erWolf1   = saveSvgAsset("ears", build3dHorrorEarsSvg("er3", "wolf",      "#d6b88d", "#78350f", "#1c0a00"));
    int erWolf2   = saveSvgAsset("ears", build3dHorrorEarsSvg("er4", "wolf",      "#cbd5e1", "#475569", "#090d16"));
    int erNosf1   = saveSvgAsset("ears", build3dHorrorEarsSvg("er5", "nosferatu", "#f8fafc", "#cbd5e1", "#334155"));
    int erNosf2   = saveSvgAsset("ears", build3dHorrorEarsSvg("er6", "nosferatu", "#dcfce7", "#4ade80", "#052e16"));
    int hrDrac    = saveSvgAsset("hair", buildHorrorAccessorySvg("drachair"));
    int hrFrank   = saveSvgAsset("hair", buildHorrorAccessorySvg("frankhair"));
    int mkVamp    = saveSvgAsset("makeup", buildHorrorMakeupSvg("vamp"));
    int mkStitch  = saveSvgAsset("makeup", buildHorrorMakeupSvg("stitches"));
    int mkCreepy  = saveSvgAsset("makeup", buildHorrorMakeupSvg("creepy"));
    int nsPale    = saveSvgAsset("nose", buildSculpted3dNoseSvg("ns1", "#f7f4f0", "#d9d4cc", "#423a32"));
    int nsGreen   = saveSvgAsset("nose", buildSculpted3dNoseSvg("ns2", "#dcedc8", "#9ccc65", "#1d3609"));
    int nsTan     = saveSvgAsset("nose", buildSculpted3dNoseSvg("ns3", "#f7c8ad", "#cc8a66", "#6e4229"));
    int nsWolf    = saveSvgAsset("nose", wrapSvgCanvas(svgTag("ellipse", "cx=\"300\" cy=\"468\" rx=\"34\" ry=\"22\" fill=\"#090d16\"")));
    int brVamp    = saveSvgAsset("brows", wrapSvgCanvas(svgTag("path", "d=\"M 122,278 Q 205,232 278,304 Q 202,258 122,292 Z\" fill=\"#090d16\"") + svgTag("path", "d=\"M 478,278 Q 395,232 322,304 Q 398,258 478,292 Z\" fill=\"#090d16\"")));

    static constexpr std::array<PaletteTriple, 10> kSkullPal = {{
        {"#fef9c3","#e2e8f0","#64748b","#ffffff"}, {"#fde68a","#d6b88d","#5c3a21","#fef9c3"},
        {"#ffffff","#f1f5f9","#475569","#ffffff"}, {"#fdba74","#ea580c","#450a0a","#fef08a"},
        {"#f8fafc","#94a3b8","#1e293b","#e2e8f0"}, {"#fef08a","#d97706","#451a03","#fef9c3"},
        {"#dcfce7","#4ade80","#14532d","#fef9c3"}, {"#f3e8ff","#a855f7","#3b0764","#ffffff"},
        {"#ffe4e6","#f43f5e","#881337","#ffffff"}, {"#e0f2fe","#38bdf8","#0c4a6e","#ffffff"}
    }};

    for (int i = 0; i < 10; ++i)
    {
        const auto& p = kSkullPal.at(i);
        const char* st = (i == 1) ? "pirate" : ((i == 2) ? "calavera" : "ivory");
        int fId = saveSvgAsset("face", build3dSkullCraniumSvg("sk" + std::to_string(i), st, p.c1, p.c2, p.c3, p.tooth));
        int jId = 97 + i;
        saveSvgAssetAt("beard", jId, build3dArticulatedJawSvg("sk" + std::to_string(i), p.c1, p.c2, p.c3, p.tooth));
        appendFaceOffsets(fId, {{"mouth_y", "14"}, {"brows_y", "-8"}});
        appendMacroCombo({fId, (i % 2 == 0) ? mkStitch : mkCreepy, (i == 3 || i == 7) ? erHorns : 0, 0, jId, (i == 1) ? glPatch : 0, 0, (i == 1) ? htBandana : ((i == 5) ? htBat : 0), 0, bgs.at(i % 4), (i % 3 == 0) ? 1025 : 512, 0, 0, 0, 160, -1, 16 + (i % 40), false, -1});
    }

    static constexpr std::array<PaletteTriple, 10> kPumpPal = {{
        {"#fdba74","#ea580c","#7c2d12","#15803d"}, {"#ffffff","#cbd5e1","#475569","#365314"},
        {"#bef264","#4d7c0f","#1a2e05","#3f6212"}, {"#fef08a","#eab308","#713f12","#15803d"},
        {"#f472b6","#db2777","#831843","#365314"}, {"#38bdf8","#0284c7","#0c4a6e","#1e293b"},
        {"#c084fc","#7e22ce","#3b0764","#14532d"}, {"#fb7185","#e11d48","#881337","#090d16"},
        {"#fde047","#ca8a04","#422006","#166534"}, {"#e2e8f0","#64748b","#0f172a","#1e293b"}
    }};

    for (int i = 0; i < 10; ++i)
    {
        const auto& p = kPumpPal.at(i);
        int fId = saveSvgAsset("face", build3dPumpkinHeadSvg("pk" + std::to_string(i), p.c1, p.c2, p.c3, p.tooth, (i >= 6)));
        appendFaceOffsets(fId, {{"mouth_y", "16"}, {"brows_y", "-10"}});
        appendMacroCombo({fId, (i >= 7) ? mkCreepy : 0, 0, 0, 0, 0, 0, (i == 6) ? htWitch : ((i == 0) ? htBat : 0), 1, bgs.at((i + 2) % 4), 4097, 11, 7, 0, 202, -1, 18 + i, false, -1});
    }

    static constexpr std::array<PaletteTriple, 10> kVampPal = {{
        {"#f7f4f0","#e2dcd5","#a3998e","#423a32"}, {"#f2f0ec","#d9d4cc","#999187","#3d3730"},
        {"#fff5f7","#ebd5dc","#ab8593","#4a2c38"}, {"#f0f2f5","#d4dae0","#8c96a0","#323b42"},
        {"#dcedc8","#9ccc65","#4b7524","#1d3609"}, {"#e6ee9c","#b4bd46","#666b14","#292b04"},
        {"#f3e5f5","#ce93d8","#7b3b8c","#310d3d"}, {"#c8e6c9","#81c784","#38783c","#123314"},
        {"#f8fafc","#cbd5e1","#64748b","#1e293b"}, {"#ffe4e6","#fda4af","#be123c","#4c0519"}
    }};

    for (int i = 0; i < 10; ++i)
    {
        const auto& p = kVampPal.at(i);
        int fId = saveSvgAsset("face", build3dHorrorHeadSvg("vp" + std::to_string(i), (i % 2 == 1) ? "vamp_slender" : "vamp", p.c1, p.c2, p.c3, p.tooth));
        appendFaceOffsets(fId, {{"mouth_y", "16"}, {"brows_y", "-8"}});
        int er = (i % 2 == 0) ? erNosf1 : erNosf2;
        int ns = (i == 4 || i == 5 || i == 7) ? nsGreen : nsPale;
        appendMacroCombo({fId, mkVamp, er, ns, 0, 0, hrDrac, (i == 3) ? htBat : 0, brVamp, bgs.at(i % 4), 1025, 3, (i == 2) ? 3 : 4, (i == 2 || i == 6) ? 10 : 0, 138, -1, 20 + i, false, -1});
    }

    for (int i = 0; i < 10; ++i)
    {
        int fId = saveSvgAsset("face", build3dHorrorHeadSvg("fk" + std::to_string(i), "frank", (i % 2 == 0) ? "#bef264" : "#86efac", (i % 2 == 0) ? "#4d7c0f" : "#15803d", "#14532d", "#052e16"));
        appendFaceOffsets(fId, {{"mouth_y", "16"}, {"brows_y", "-12"}});
        appendMacroCombo({fId, mkStitch, erBolts, nsGreen, 0, 0, hrFrank, 0, 2, bgs.at((i + 1) % 4), 8192, 5, 7, 0, 160, -1, 30 + i, false, -1});
    }

    for (int i = 0; i < 10; ++i)
    {
        bool brown = (i % 2 == 0);
        int fId = saveSvgAsset("face", build3dWerewolfHeadSvg("wf" + std::to_string(i), brown ? "#a16207" : "#94a3b8", brown ? "#451a03" : "#334155", brown ? "#1c0a00" : "#090d16", brown ? "#d6b88d" : "#cbd5e1"));
        appendFaceOffsets(fId, {{"mouth_y", "18"}, {"brows_y", "-8"}});
        appendMacroCombo({fId, 0, brown ? erWolf1 : erWolf2, nsWolf, 0, 0, 0, 0, 2, bgs.at(i % 4), 512, 10, 7, 0, 202, -1, 40 + i, false, -1});
    }

    for (int i = 0; i < 10; ++i)
    {
        const char* kind = (i < 5) ? "mummy" : "vamp_slender";
        int fId = saveSvgAsset("face", build3dHorrorHeadSvg("mm" + std::to_string(i), kind, (i < 5) ? "#fef3c7" : "#dcfce7", (i < 5) ? "#d6b88d" : "#4ade80", "#451a03", "#1c0a00"));
        appendFaceOffsets(fId, {{"mouth_y", "16"}, {"brows_y", "-8"}});
        appendMacroCombo({fId, (i < 5) ? mkCreepy : mkVamp, (i >= 5) ? erNosf2 : 0, (i < 5) ? nsTan : nsGreen, 0, 0, 0, (i >= 5) ? htWitch : 0, brVamp, bgs.at((i + 3) % 4), 1024, (i < 5) ? 15 : 13, 7, 0, 224, -1, 50 + (i % 14), false, -1});
    }

    std::vector<ProfileThemeSpec> profThemes;
    for (int i = 0; i < 10; ++i)
    {
        const auto& vp = kVampPal.at(i);
        profThemes.push_back({
            vp.c1, vp.c2, vp.c3, vp.c1, vp.c2, vp.c3,
            "#090d16", "#1e1b4b", "#4c1d95",
            48, bgs.at(i % 4), 3, 4, 0, 138, 20 + i, -1,
            true, (i % 2 == 1), (i % 3 == 0), (i % 2 == 0)
        });
    }
    generateStandardSideProfileSuite(profThemes, 5);

    cleanSvgAndSaveZip("halloween");
    std::cout << "[OK] 100% C++17 60 Horror Icons + 50 Complete Side-Profile Pairs (127..226) zipped to packs/halloween.zip!\n";
    return 0;
}
