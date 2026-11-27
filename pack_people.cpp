// ==============================================================================
// MakingFaces - Theme Pack 4 (C++17): 10 People of the World + 10x All Props
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

namespace {

struct PersonSpec {
    const char* name;
    const char* faceShape;
    const char* skinHi;
    const char* skinMid;
    const char* skinDeep;
    const char* skinLine;
    const char* noseShape;
    int         earStyle;
    const char* hairDeep;
    const char* hairDark;
    const char* hairMid;
    const char* hairHi;
    int         hairStyle;
    int         browStyle;
    int         beardStyle;
    int         glassStyle;
    int         hatStyle;
    int         makeupStyle;
    int         lipColor;
    int         eyelash;
    int         eyeBank;
    int         animId;
    bool        wearHat;
    bool        wearGlasses;
    bool        wearBeard;
    bool        wearMakeup;
};

struct WorldSceneSpec {
    const char* skyTop;
    const char* skyBot;
    const char* orbHex;
    int         orbX;
    int         orbY;
    const char* farHex;
    const char* midHex;
    const char* frontHex;
    int         archStyle;
    bool        nightStars;
};

static int addWorldBackdropScene(const WorldSceneSpec& sc)
{
    int idx = nextAssetIdx("bg");
    std::array<char, 24> prefix{};
    std::snprintf(prefix.data(), prefix.size(), "bg/%03d", idx);
    std::string pfx(prefix.data());
    std::string idS = std::to_string(idx);

    auto makeGrad3D = [&](const std::string& gid, const char* cTop, const char* cMid, const char* cBot, bool diag = false) {
        return svgTag("linearGradient", "id=\"" + gid + "\" x1=\"" + (diag ? "15%" : "0%") + "\" y1=\"0%\" x2=\"" + (diag ? "85%" : "0%") + "\" y2=\"100%\"", false) +
               svgTag("stop", std::string("offset=\"0%\" stop-color=\"") + cTop + "\"") +
               svgTag("stop", std::string("offset=\"52%\" stop-color=\"") + cMid + "\"") +
               svgTag("stop", std::string("offset=\"100%\" stop-color=\"") + cBot + "\"") + svgClose("linearGradient");
    };

    std::string sDefs =
        makeGrad3D("wsk" + idS, sc.skyTop, sc.skyBot, "#e2e8f0") +
        svgTag("radialGradient", "id=\"wob" + idS + "\" cx=\"50%\" cy=\"50%\" r=\"50%\"", false) +
        svgTag("stop", "offset=\"0%\" stop-color=\"#ffffff\" stop-opacity=\"0.96\"") +
        svgTag("stop", std::string("offset=\"45%\" stop-color=\"") + sc.orbHex + "\" stop-opacity=\"0.82\"") +
        svgTag("stop", std::string("offset=\"100%\" stop-color=\"") + sc.orbHex + "\" stop-opacity=\"0.0\"") + svgClose("radialGradient") +
        makeGrad3D("wfr" + idS, "#94a3b8", sc.farHex, "#1e293b", true) +
        makeGrad3D("wmd" + idS, "#64748b", sc.midHex, "#0f172a", true) +
        makeGrad3D("wfg" + idS, sc.frontHex, "#14532d", "#052e16") +
        makeGrad3D("wsnd" + idS, "#fde68a", "#d97706", "#78350f") +
        makeGrad3D("wwat" + idS, "#38bdf8", "#0284c7", "#0c4a6e") +
        makeGrad3D("wsnw" + idS, "#ffffff", "#e2e8f0", "#94a3b8", true) +
        makeGrad3D("wstn" + idS, "#cbd5e1", "#64748b", "#1e293b", true);

    int sideOrbX = (sc.orbX < 300) ? 74 : 526;
    std::ostringstream sky;
    sky << svgTag("rect", "x=\"-10\" y=\"-10\" width=\"620\" height=\"820\" fill=\"url(#wsk" + idS + ")\"");
    if (sc.nightStars) {
        static constexpr std::array<int, 20> kStars = {28,58, 88,128, 52,210, 108,82, 512,68, 568,148, 535,225, 488,108, 36,285, 564,295};
        for (size_t i = 0; i < kStars.size(); i += 2) {
            sky << svgTag("circle", "cx=\"" + std::to_string(kStars.at(i)) + "\" cy=\"" + std::to_string(kStars.at(i + 1)) + "\" r=\"2.2\" fill=\"#f8fafc\" opacity=\"0.92\"");
        }
    }
    sky << svgTag("circle", "cx=\"" + std::to_string(sideOrbX) + "\" cy=\"135\" r=\"118\" fill=\"url(#wob" + idS + ")\"")
        << svgTag("circle", "cx=\"" + std::to_string(sideOrbX) + "\" cy=\"135\" r=\"34\" fill=\"#ffffff\" opacity=\"0.88\"");
    if (sc.archStyle == 9) {
        sky << svgTag("path", "d=\"M 12,480 A 288,260 0 0,1 588,480\" fill=\"none\" stroke=\"#ef4444\" stroke-width=\"10\" opacity=\"0.38\"")
            << svgTag("path", "d=\"M 22,480 A 278,250 0 0,1 578,480\" fill=\"none\" stroke=\"#facc15\" stroke-width=\"10\" opacity=\"0.38\"")
            << svgTag("path", "d=\"M 32,480 A 268,240 0 0,1 568,480\" fill=\"none\" stroke=\"#38bdf8\" stroke-width=\"10\" opacity=\"0.38\"");
    }
    exportSvgToPng(pfx + "_sky", wrapSvgCanvas(sky.str(), sDefs), 600, 800, 0.0f);

    std::string clouds =
        svgTag("ellipse", "cx=\"85\" cy=\"112\" rx=\"98\" ry=\"22\" fill=\"#ffffff\" opacity=\"0.42\"") +
        svgTag("ellipse", "cx=\"115\" cy=\"124\" rx=\"72\" ry=\"16\" fill=\"#cbd5e1\" opacity=\"0.30\"") +
        svgTag("ellipse", "cx=\"515\" cy=\"152\" rx=\"102\" ry=\"24\" fill=\"#ffffff\" opacity=\"0.38\"") +
        svgTag("ellipse", "cx=\"485\" cy=\"166\" rx=\"76\" ry=\"16\" fill=\"#94a3b8\" opacity=\"0.28\"");
    exportSvgToPng(pfx + "_clouds", wrapSvgCanvas(clouds), 600, 800, 7.5f);

    std::ostringstream far, mid, front;
    switch (sc.archStyle) {
        case 0: // Nordic: 3D shaded snow mountains, fjord reflection, cabin on left & pines on right
            far << svgTag("path", "d=\"M -20,520 L 65,245 L 155,395 L 300,340 L 445,395 L 535,235 L 620,375 L 620,820 L -20,820 Z\" fill=\"url(#wfr" + idS + ")\"")
                << svgTag("polygon", "points=\"65,245 34,305 65,295\" fill=\"url(#wsnw" + idS + ")\"")
                << svgTag("polygon", "points=\"65,245 65,295 96,305\" fill=\"#94a3b8\"")
                << svgTag("polygon", "points=\"535,235 502,298 535,288\" fill=\"url(#wsnw" + idS + ")\"")
                << svgTag("polygon", "points=\"535,235 535,288 568,298\" fill=\"#94a3b8\"");
            mid << svgTag("rect", "x=\"-10\" y=\"525\" width=\"620\" height=\"160\" fill=\"url(#wwat" + idS + ")\"")
                << svgTag("path", "d=\"M -20,545 Q 110,485 260,545 T 620,515 L 620,665 L -20,665 Z\" fill=\"url(#wmd" + idS + ")\"")
                << svgTag("rect", "x=\"18\" y=\"512\" width=\"64\" height=\"42\" fill=\"#991b1b\"")
                << svgTag("polygon", "points=\"10,512 50,482 90,512\" fill=\"url(#wstn" + idS + ")\"")
                << svgTag("rect", "x=\"34\" y=\"524\" width=\"14\" height=\"14\" fill=\"#fef08a\"");
            front << svgTag("path", "d=\"M -10,708 Q 160,688 320,712 T 610,696 L 610,810 L -10,810 Z\" fill=\"url(#wfg" + idS + ")\"")
                  << svgTag("polygon", "points=\"545,495 510,702 545,702\" fill=\"#15803d\"")
                  << svgTag("polygon", "points=\"545,495 545,702 580,702\" fill=\"#052e16\"");
            break;
        case 1: // Mediterranean: 3D coastal cliffs, deep sea, sunlit villas & cypresses
            far << svgTag("path", "d=\"M -20,515 Q 110,335 280,445 T 620,330 L 620,820 L -20,820 Z\" fill=\"url(#wfr" + idS + ")\"")
                << svgTag("rect", "x=\"-10\" y=\"515\" width=\"620\" height=\"140\" fill=\"url(#wwat" + idS + ")\"");
            mid << svgTag("path", "d=\"M -20,635 Q 180,545 380,605 T 620,550 L 620,820 L -20,820 Z\" fill=\"url(#wmd" + idS + ")\"")
                << svgTag("rect", "x=\"14\" y=\"578\" width=\"50\" height=\"52\" fill=\"url(#wsnw" + idS + ")\"")
                << svgTag("polygon", "points=\"10,578 39,560 68,578\" fill=\"#c2410c\"")
                << svgTag("rect", "x=\"66\" y=\"594\" width=\"44\" height=\"42\" fill=\"#e2e8f0\"")
                << svgTag("polygon", "points=\"62,594 88,578 114,594\" fill=\"#ea580c\"");
            front << svgTag("rect", "x=\"-10\" y=\"712\" width=\"620\" height=\"98\" fill=\"url(#wfg" + idS + ")\"")
                  << svgTag("ellipse", "cx=\"542\" cy=\"585\" rx=\"15\" ry=\"130\" fill=\"#15803d\"")
                  << svgTag("ellipse", "cx=\"548\" cy=\"585\" rx=\"9\" ry=\"128\" fill=\"#052e16\"");
            break;
        case 2: // East Asia: 3D Mount Fuji, shaded pagoda, cherry blossoms & bamboo
            far << svgTag("path", "d=\"M -20,560 L 10,560 L 68,350 L 138,275 L 208,350 L 275,560 L 620,560 L 620,820 L -20,820 Z\" fill=\"url(#wfr" + idS + ")\"")
                << svgTag("path", "d=\"M 102,315 Q 138,334 174,315 L 138,275 Z\" fill=\"url(#wsnw" + idS + ")\"");
            mid << svgTag("path", "d=\"M -20,645 Q 170,590 310,635 T 620,600 L 620,820 L -20,820 Z\" fill=\"url(#wmd" + idS + ")\"")
                << svgTag("rect", "x=\"516\" y=\"550\" width=\"42\" height=\"68\" fill=\"#991b1b\"")
                << svgTag("polygon", "points=\"498,550 537,530 576,550\" fill=\"url(#wstn" + idS + ")\"")
                << svgTag("polygon", "points=\"504,527 537,509 570,527\" fill=\"url(#wstn" + idS + ")\"")
                << svgTag("circle", "cx=\"548\" cy=\"355\" r=\"68\" fill=\"#f472b6\" opacity=\"0.68\"");
            front << svgTag("rect", "x=\"-10\" y=\"715\" width=\"620\" height=\"95\" fill=\"url(#wfg" + idS + ")\"")
                  << svgTag("line", "x1=\"26\" y1=\"800\" x2=\"30\" y2=\"410\" stroke=\"#16a34a\" stroke-width=\"8\"")
                  << svgTag("line", "x1=\"46\" y1=\"800\" x2=\"50\" y2=\"450\" stroke=\"#15803d\" stroke-width=\"7\"")
                  << svgTag("line", "x1=\"576\" y1=\"800\" x2=\"572\" y2=\"425\" stroke=\"#16a34a\" stroke-width=\"8\"");
            break;
        case 3: // South Asia: 3D marble Taj Mahal dome & minarets on right, water reflection & palms
            far << svgTag("path", "d=\"M -20,565 L 435,565 L 460,425 Q 525,315 590,425 L 620,565 L 620,820 L -20,820 Z\" fill=\"url(#wsnw" + idS + ")\"")
                << svgTag("rect", "x=\"425\" y=\"365\" width=\"16\" height=\"200\" fill=\"url(#wsnw" + idS + ")\"")
                << svgTag("rect", "x=\"582\" y=\"365\" width=\"16\" height=\"200\" fill=\"url(#wsnw" + idS + ")\"");
            mid << svgTag("rect", "x=\"-10\" y=\"580\" width=\"620\" height=\"140\" fill=\"url(#wwat" + idS + ")\" opacity=\"0.65\"")
                << svgTag("path", "d=\"M -20,645 Q 180,605 320,645 T 620,625 L 620,820 L -20,820 Z\" fill=\"url(#wmd" + idS + ")\"");
            front << svgTag("rect", "x=\"-10\" y=\"716\" width=\"620\" height=\"94\" fill=\"url(#wfg" + idS + ")\"")
                  << svgTag("ellipse", "cx=\"52\" cy=\"692\" rx=\"62\" ry=\"18\" transform=\"rotate(-24 52 692)\" fill=\"#15803d\"")
                  << svgTag("ellipse", "cx=\"548\" cy=\"692\" rx=\"62\" ry=\"18\" transform=\"rotate(24 548 692)\" fill=\"#15803d\"");
            break;
        case 4: // West Africa: 3D Kilimanjaro peak on right, shaded acacia & lush green savanna
            far << svgTag("path", "d=\"M -20,545 L 365,545 L 445,425 L 495,305 L 575,305 L 620,425 L 620,820 L -20,820 Z\" fill=\"url(#wfr" + idS + ")\"")
                << svgTag("polygon", "points=\"495,305 575,305 590,336 480,336\" fill=\"url(#wsnw" + idS + ")\"");
            mid << svgTag("path", "d=\"M -20,625 Q 220,585 420,615 T 620,605 L 620,820 L -20,820 Z\" fill=\"url(#wfg" + idS + ")\"")
                << svgTag("rect", "x=\"56\" y=\"487\" width=\"12\" height=\"145\" fill=\"#451a03\"")
                << svgTag("ellipse", "cx=\"62\" cy=\"484\" rx=\"76\" ry=\"17\" fill=\"#15803d\"");
            front << svgTag("rect", "x=\"-10\" y=\"714\" width=\"620\" height=\"96\" fill=\"#052e16\"")
                  << svgTag("path", "d=\"M 24,720 L 14,675 M 48,720 L 56,672 M 542,720 L 534,674 M 572,720 L 580,678\" stroke=\"#65a30d\" stroke-width=\"4\"");
            break;
        case 5: // East Africa: 3D Rift Valley cliffs, sunlit lake & baobab on right
            far << svgTag("path", "d=\"M -20,525 L 0,415 L 135,415 L 175,495 L 445,495 L 485,420 L 620,420 L 620,820 L -20,820 Z\" fill=\"url(#wfr" + idS + ")\"");
            mid << svgTag("rect", "x=\"-10\" y=\"535\" width=\"620\" height=\"95\" fill=\"url(#wwat" + idS + ")\" opacity=\"0.78\"")
                << svgTag("path", "d=\"M -20,620 Q 190,575 380,625 T 620,600 L 620,820 L -20,820 Z\" fill=\"url(#wfg" + idS + ")\"")
                << svgTag("rect", "x=\"524\" y=\"492\" width=\"24\" height=\"140\" rx=\"8\" fill=\"#78350f\"")
                << svgTag("ellipse", "cx=\"536\" cy=\"486\" rx=\"64\" ry=\"20\" fill=\"#15803d\"");
            front << svgTag("rect", "x=\"-10\" y=\"715\" width=\"620\" height=\"95\" fill=\"#052e16\"");
            break;
        case 6: // South America: 3D snow-lit Andes peaks & rolling green foothills
            far << svgTag("path", "d=\"M -20,530 L 68,230 L 165,375 L 300,340 L 445,375 L 538,215 L 620,345 L 620,820 L -20,820 Z\" fill=\"url(#wfr" + idS + ")\"");
            mid << svgTag("path", "d=\"M -20,625 Q 150,535 310,605 T 620,575 L 620,820 L -20,820 Z\" fill=\"url(#wfg" + idS + ")\"");
            front << svgTag("rect", "x=\"-10\" y=\"712\" width=\"620\" height=\"98\" fill=\"#052e16\"");
            break;
        case 7: // North America: 3D sandstone Monument Valley mesas & saguaro cacti
            far << svgTag("path", "d=\"M -20,545 L 12,545 L 28,395 L 118,395 L 134,545 L 468,545 L 486,375 L 578,375 L 594,545 L 620,545 L 620,820 L -20,820 Z\" fill=\"url(#wsnd" + idS + ")\"");
            mid << svgTag("path", "d=\"M -20,635 Q 200,595 380,635 T 620,615 L 620,820 L -20,820 Z\" fill=\"url(#wsnd" + idS + ")\"");
            front << svgTag("rect", "x=\"-10\" y=\"714\" width=\"620\" height=\"96\" fill=\"#78350f\"")
                  << svgTag("line", "x1=\"54\" y1=\"720\" x2=\"54\" y2=\"575\" stroke=\"#15803d\" stroke-width=\"14\" stroke-linecap=\"round\"")
                  << svgTag("path", "d=\"M 32,620 L 32,652 L 54,652 M 76,610 L 76,642 L 54,642\" fill=\"none\" stroke=\"#15803d\" stroke-width=\"9\" stroke-linecap=\"round\"")
                  << svgTag("line", "x1=\"552\" y1=\"720\" x2=\"552\" y2=\"590\" stroke=\"#15803d\" stroke-width=\"13\" stroke-linecap=\"round\"");
            break;
        case 8: // Middle East: 3D moonlit desert dunes, mosque dome on left & date palm on right
            far << svgTag("path", "d=\"M -20,555 Q 140,475 310,535 T 620,485 L 620,820 L -20,820 Z\" fill=\"url(#wfr" + idS + ")\"")
                << svgTag("path", "d=\"M 28,535 Q 74,455 120,535 Z\" fill=\"url(#wstn" + idS + ")\"")
                << svgTag("rect", "x=\"12\" y=\"440\" width=\"11\" height=\"105\" fill=\"url(#wstn" + idS + ")\"");
            mid << svgTag("path", "d=\"M -20,645 Q 240,565 460,640 T 620,610 L 620,820 L -20,820 Z\" fill=\"url(#wsnd" + idS + ")\"");
            front << svgTag("rect", "x=\"-10\" y=\"715\" width=\"620\" height=\"95\" fill=\"#451a03\"")
                  << svgTag("path", "d=\"M 556,725 Q 542,595 528,485\" fill=\"none\" stroke=\"#451a03\" stroke-width=\"12\"")
                  << svgTag("path", "d=\"M 528,485 Q 478,470 458,505 M 528,485 Q 574,465 592,500 M 528,485 Q 508,445 478,455\" fill=\"none\" stroke=\"#15803d\" stroke-width=\"7\" stroke-linecap=\"round\"");
            break;
        default: // Celtic: 3D emerald highlands, shaded stone castle tower, rainbow & stone wall
            far << svgTag("path", "d=\"M -20,545 Q 150,415 320,515 T 620,435 L 620,820 L -20,820 Z\" fill=\"url(#wfg" + idS + ")\"")
                << svgTag("rect", "x=\"24\" y=\"452\" width=\"62\" height=\"86\" fill=\"url(#wstn" + idS + ")\"")
                << svgTag("rect", "x=\"24\" y=\"439\" width=\"13\" height=\"16\" fill=\"url(#wstn" + idS + ")\"")
                << svgTag("rect", "x=\"48\" y=\"439\" width=\"13\" height=\"16\" fill=\"url(#wstn" + idS + ")\"")
                << svgTag("rect", "x=\"73\" y=\"439\" width=\"13\" height=\"16\" fill=\"url(#wstn" + idS + ")\"");
            mid << svgTag("path", "d=\"M -20,645 Q 185,565 360,630 T 620,590 L 620,820 L -20,820 Z\" fill=\"url(#wfg" + idS + ")\"");
            front << svgTag("rect", "x=\"-10\" y=\"712\" width=\"620\" height=\"98\" fill=\"#052e16\"")
                  << svgTag("rect", "x=\"-10\" y=\"694\" width=\"620\" height=\"24\" fill=\"url(#wstn" + idS + ")\" stroke=\"#1e293b\" stroke-width=\"3\"");
            break;
    }

    exportSvgToPng(pfx + "_far", wrapSvgCanvas(far.str(), sDefs), 600, 800, 3.0f);
    exportSvgToPng(pfx + "_mid", wrapSvgCanvas(mid.str(), sDefs), 600, 800, 1.2f);
    exportSvgToPng(pfx + "_front", wrapSvgCanvas(front.str(), sDefs), 600, 800, 0.0f);
    return idx;
}


static std::string build3dHumanFaceSvg(int id, const PersonSpec& p)
{
    std::string pathD = "M 300,205 C 445,202 510,255 515,380 C 520,510 488,655 425,725 C 385,762 338,770 300,770 C 262,770 215,762 175,725 C 112,655 80,510 85,380 C 90,255 155,202 300,205 Z";
    if (std::string(p.faceShape) == "heart") {
        pathD = "M 300,224 C 378,186 508,208 514,352 C 519,482 466,628 386,718 C 356,752 330,766 300,766 C 270,766 244,752 214,718 C 134,628 81,482 86,352 C 92,208 222,186 300,224 Z";
    } else if (std::string(p.faceShape) == "slender") {
        pathD = "M 300,198 C 440,198 500,258 498,386 C 495,522 456,658 384,728 C 354,756 328,766 300,766 C 272,766 246,756 216,728 C 144,658 105,522 102,386 C 100,258 160,198 300,198 Z";
    } else if (std::string(p.faceShape) == "round") {
        pathD = "M 300,205 C 465,205 535,275 530,395 C 525,525 485,660 415,720 C 370,758 335,768 300,768 C 265,768 230,758 185,720 C 115,660 75,525 70,395 C 65,275 135,205 300,205 Z";
    }

    std::string sid = std::to_string(id);
    std::string defs =
        svgTag("radialGradient", "id=\"pf" + sid + "\" cx=\"46%\" cy=\"36%\" r=\"66%\"", false) +
        svgTag("stop", std::string("offset=\"0%\" stop-color=\"") + p.skinHi + "\"") +
        svgTag("stop", std::string("offset=\"54%\" stop-color=\"") + p.skinMid + "\"") +
        svgTag("stop", std::string("offset=\"88%\" stop-color=\"") + p.skinDeep + "\"") +
        svgTag("stop", std::string("offset=\"100%\" stop-color=\"") + p.skinLine + "\"") + svgClose("radialGradient") +
        svgTag("radialGradient", "id=\"phi" + sid + "\" cx=\"50%\" cy=\"24%\" r=\"36%\"", false) +
        svgTag("stop", "offset=\"0%\" stop-color=\"#ffffff\" stop-opacity=\"0.26\"") +
        svgTag("stop", "offset=\"100%\" stop-color=\"#ffffff\" stop-opacity=\"0\"") + svgClose("radialGradient") +
        svgTag("mask", "id=\"pmk" + sid + "\"", false) + getStandardEyeCutoutMask() + svgClose("mask");

    std::ostringstream extra;
    if (std::string(p.name) == "Sean_Celtic") {
        for (int fx : {155, 172, 188, 205, 395, 412, 428, 445}) {
            extra << svgTag("circle", "cx=\"" + std::to_string(fx) + "\" cy=\"468\" r=\"2.4\" fill=\"#b45309\" opacity=\"0.48\"")
                  << svgTag("circle", "cx=\"" + std::to_string(fx + 5) + "\" cy=\"482\" r=\"2.0\" fill=\"#b45309\" opacity=\"0.42\"");
        }
    }

    std::string rims =
        svgTag("path", "d=\"M 124,345 C 145,303 230,303 256,347 C 262,379 233,407 190,405 C 145,403 116,379 124,345 Z\" fill=\"none\" stroke=\"" + std::string(p.skinLine) + "\" stroke-width=\"5\"") +
        svgTag("path", "d=\"M 476,345 C 455,303 370,303 344,347 C 338,379 367,407 410,405 C 455,403 484,379 476,345 Z\" fill=\"none\" stroke=\"" + std::string(p.skinLine) + "\" stroke-width=\"5\"");

    std::string body =
        svgTag("g", "mask=\"url(#pmk" + sid + ")\"", false) +
        svgTag("path", "d=\"" + pathD + "\" fill=\"url(#pf" + sid + ")\" stroke=\"" + std::string(p.skinLine) + "\" stroke-width=\"6\" stroke-linejoin=\"round\"") +
        svgTag("path", "d=\"" + pathD + "\" fill=\"url(#phi" + sid + ")\"") +
        extra.str() + rims + svgClose("g");
    return wrapSvgCanvas(body, defs);
}

static std::string build3dHumanEarsSvg(int id, const PersonSpec& p)
{
    std::string sid = std::to_string(id);
    std::string defs =
        svgTag("radialGradient", "id=\"peg" + sid + "\" cx=\"45%\" cy=\"38%\" r=\"68%\"", false) +
        svgTag("stop", std::string("offset=\"0%\" stop-color=\"") + p.skinHi + "\"") +
        svgTag("stop", std::string("offset=\"100%\" stop-color=\"") + p.skinDeep + "\"") + svgClose("radialGradient");

    std::ostringstream b;
    b << svgTag("path", "d=\"M 118,310 C 24,285 24,510 118,528 Z\" fill=\"url(#peg" + sid + ")\" stroke=\"" + std::string(p.skinLine) + "\" stroke-width=\"5\"")
      << svgTag("path", "d=\"M 98,342 C 54,335 54,475 98,488\" fill=\"none\" stroke=\"" + std::string(p.skinDeep) + "\" stroke-width=\"5\" stroke-linecap=\"round\"")
      << svgTag("path", "d=\"M 482,310 C 576,285 576,510 482,528 Z\" fill=\"url(#peg" + sid + ")\" stroke=\"" + std::string(p.skinLine) + "\" stroke-width=\"5\"")
      << svgTag("path", "d=\"M 502,342 C 546,335 546,475 502,488\" fill=\"none\" stroke=\"" + std::string(p.skinDeep) + "\" stroke-width=\"5\" stroke-linecap=\"round\"");

    if (p.earStyle == 1) {
        b << svgTag("circle", "cx=\"72\" cy=\"516\" r=\"24\" fill=\"none\" stroke=\"#facc15\" stroke-width=\"7\"")
          << svgTag("circle", "cx=\"528\" cy=\"516\" r=\"24\" fill=\"none\" stroke=\"#facc15\" stroke-width=\"7\"");
    } else if (p.earStyle == 2) {
        b << svgTag("circle", "cx=\"76\" cy=\"502\" r=\"9\" fill=\"#e0f2fe\" stroke=\"#0284c7\" stroke-width=\"3\"")
          << svgTag("circle", "cx=\"524\" cy=\"502\" r=\"9\" fill=\"#e0f2fe\" stroke=\"#0284c7\" stroke-width=\"3\"");
    } else if (p.earStyle == 3) {
        b << svgTag("circle", "cx=\"74\" cy=\"512\" r=\"16\" fill=\"none\" stroke=\"#f59e0b\" stroke-width=\"6\"")
          << svgTag("circle", "cx=\"74\" cy=\"538\" r=\"8\" fill=\"#ef4444\"")
          << svgTag("circle", "cx=\"526\" cy=\"512\" r=\"16\" fill=\"none\" stroke=\"#f59e0b\" stroke-width=\"6\"")
          << svgTag("circle", "cx=\"526\" cy=\"538\" r=\"8\" fill=\"#ef4444\"");
    } else if (p.earStyle == 4) {
        b << svgTag("path", "d=\"M 74,504 Q 52,555 68,596 Q 88,555 74,504 Z\" fill=\"#38bdf8\" stroke=\"#0369a1\" stroke-width=\"2.5\"")
          << svgTag("path", "d=\"M 526,504 Q 548,555 532,596 Q 512,555 526,504 Z\" fill=\"#38bdf8\" stroke=\"#0369a1\" stroke-width=\"2.5\"");
    }
    return wrapSvgCanvas(b.str(), defs);
}

static std::string build3dHumanNoseSvg(int id, const PersonSpec& p)
{
    std::string sid = std::to_string(id);
    std::string shape(p.noseShape);
    int brRx = 22, brRy = 92, tipRx = 33, tipRy = 25, tipY = 486, wRx = 24, wRy = 19, wDx = 31, nDx = 19, nY = 505;
    if (shape == "broad")    { brRx = 31; tipRx = 48; tipRy = 30; tipY = 488; wRx = 36; wRy = 24; wDx = 45; nDx = 28; nY = 510; }
    if (shape == "button")   { brRx = 19; brRy = 80; tipRx = 34; tipRy = 26; tipY = 478; wRx = 22; wRy = 18; wDx = 27; nDx = 17; nY = 496; }
    if (shape == "aquiline") { brRx = 24; brRy = 98; tipRx = 31; tipRy = 28; tipY = 492; wRx = 23; wRy = 19; wDx = 30; nDx = 18; nY = 510; }
    if (shape == "slim")     { brRx = 18; tipRx = 27; tipRy = 23; tipY = 484; wRx = 20; wRy = 17; wDx = 25; nDx = 16; nY = 503; }

    std::string defs =
        svgTag("radialGradient", "id=\"pnb" + sid + "\" cx=\"50%\" cy=\"50%\" r=\"50%\"", false) +
        svgTag("stop", std::string("offset=\"0%\" stop-color=\"") + p.skinMid + "\" stop-opacity=\"0.88\"") +
        svgTag("stop", std::string("offset=\"100%\" stop-color=\"") + p.skinDeep + "\" stop-opacity=\"0\"") + svgClose("radialGradient") +
        svgTag("radialGradient", "id=\"pnt" + sid + "\" cx=\"42%\" cy=\"35%\" r=\"58%\"", false) +
        svgTag("stop", std::string("offset=\"0%\" stop-color=\"") + p.skinHi + "\" stop-opacity=\"0.98\"") +
        svgTag("stop", std::string("offset=\"55%\" stop-color=\"") + p.skinMid + "\" stop-opacity=\"0.78\"") +
        svgTag("stop", std::string("offset=\"100%\" stop-color=\"") + p.skinDeep + "\" stop-opacity=\"0\"") + svgClose("radialGradient");

    std::string body =
        svgTag("ellipse", "cx=\"300\" cy=\"" + std::to_string(tipY + 18) + "\" rx=\"" + std::to_string(tipRx + 8) + "\" ry=\"13\" fill=\"#000000\" opacity=\"0.22\"") +
        svgTag("ellipse", "cx=\"300\" cy=\"412\" rx=\"" + std::to_string(brRx) + "\" ry=\"" + std::to_string(brRy) + "\" fill=\"url(#pnb" + sid + ")\"") +
        svgTag("ellipse", "cx=\"" + std::to_string(300 - wDx) + "\" cy=\"" + std::to_string(tipY + 8) + "\" rx=\"" + std::to_string(wRx) + "\" ry=\"" + std::to_string(wRy) + "\" fill=\"url(#pnb" + sid + ")\"") +
        svgTag("ellipse", "cx=\"" + std::to_string(300 + wDx) + "\" cy=\"" + std::to_string(tipY + 8) + "\" rx=\"" + std::to_string(wRx) + "\" ry=\"" + std::to_string(wRy) + "\" fill=\"url(#pnb" + sid + ")\"") +
        svgTag("ellipse", "cx=\"300\" cy=\"" + std::to_string(tipY) + "\" rx=\"" + std::to_string(tipRx) + "\" ry=\"" + std::to_string(tipRy) + "\" fill=\"url(#pnt" + sid + ")\"") +
        svgTag("ellipse", "cx=\"" + std::to_string(300 - nDx) + "\" cy=\"" + std::to_string(nY) + "\" rx=\"10\" ry=\"5.5\" fill=\"#0a0402\" opacity=\"0.85\"") +
        svgTag("ellipse", "cx=\"" + std::to_string(300 + nDx) + "\" cy=\"" + std::to_string(nY) + "\" rx=\"10\" ry=\"5.5\" fill=\"#0a0402\" opacity=\"0.85\"");
    return wrapSvgCanvas(body, defs);
}
static std::string buildStrandedBrowSvg(int style, const PersonSpec& p)
{
    std::mt19937 rng(3000 + style * 37);
    std::uniform_real_distribution<float> u01(0.0f, 1.0f);
    std::uniform_real_distribution<float> jit(-4.5f, 4.5f);
    std::ostringstream ss;

    for (int side : {-1, 1}) {
        for (int k = 0; k < 320; ++k) {
            float u = u01(rng), v = (u01(rng) - 0.5f) * 2.0f;
            float xb = (style == 7) ? (298.0f - u * 172.0f) : (270.0f - u * 138.0f);
            float ay = (style == 4) ? (298.0f - u * 38.0f) : ((style == 2) ? (282.0f - std::sin(u * 3.14159f) * 28.0f) : (285.0f - std::sin(u * 3.14159f * 0.85f) * 22.0f));
            float hh = (style == 1 || style == 3) ? 4.5f : ((style == 5 || style == 7) ? 15.5f : 10.5f);
            float ang = (-76.0f + u * 104.0f + jit(rng)) * 3.14159f / 180.0f;
            float len = (style == 5) ? 18.0f : 14.5f;
            float dx = -std::cos(ang) * len, dy = std::sin(ang) * len;
            float x0 = (side == 1) ? (600.0f - xb) : xb;
            float x1 = (side == 1) ? (600.0f - (xb + dx)) : (xb + dx);
            float y0 = ay + v * hh, y1 = y0 + dy;
            const char* col = (u01(rng) < 0.35f) ? p.hairDeep : ((u01(rng) < 0.78f) ? p.hairDark : p.hairMid);

            std::string buf(160, '\0');
            int n = std::snprintf(buf.data(), buf.size(),
                                  "d=\"M %.1f,%.1f Q %.1f,%.1f %.1f,%.1f\" fill=\"none\" stroke=\"%s\" stroke-width=\"0.95\" stroke-linecap=\"round\" opacity=\"0.78\"",
                                  x0, y0, (x0 + x1) * 0.5f, (y0 + y1) * 0.5f - 2.0f, x1, y1, col);
            if (n > 0) { buf.resize(static_cast<size_t>(n)); ss << svgTag("path", buf); }
        }
    }
    return wrapSvgCanvas(ss.str());
}

static std::string buildStrandedBeardSvg(int style, const PersonSpec& p)
{
    std::mt19937 rng(4000 + style * 53);
    std::uniform_real_distribution<float> u01(0.0f, 1.0f);
    std::ostringstream ss;
    int cnt = (style == 1 || style == 4 || style == 6) ? 1800 : ((style == 2 || style == 8) ? 900 : 1450);

    for (int k = 0; k < cnt; ++k) {
        float side = (u01(rng) < 0.5f) ? -1.0f : 1.0f;
        float u = u01(rng), v = (u01(rng) - 0.5f) * 2.0f;
        float x0 = 300.0f + side * u * ((style == 2) ? 34.0f : 112.0f);
        float y0 = 546.0f + u * 16.0f + v * 7.0f;

        if (style == 2) {
            y0 = 672.0f + u01(rng) * 32.0f;
        } else if (style == 1 || style == 4) {
            float zone = u01(rng);
            if (zone < 0.36f) {
                x0 = 300.0f + side * (16.0f + u * 82.0f);
                y0 = 546.0f + u * 14.0f + v * 6.0f;
            } else {
                x0 = 300.0f + side * u * 84.0f;
                y0 = 668.0f + u01(rng) * 58.0f;
            }
        }

        float len = (style == 6) ? 26.0f : ((style == 8) ? 10.0f : 16.0f);
        float ang = ((style == 0 || style == 6) ? (18.0f - u * 86.0f) : 84.0f) * 3.14159f / 180.0f;
        float x1 = x0 + side * std::cos(ang) * len;
        float y1 = y0 + std::sin(ang) * len;

        const char* col = p.hairMid;
        if (style == 4) {
            float rPick = u01(rng);
            col = (rPick < 0.18f) ? "#9a3412" : ((rPick < 0.65f) ? "#ea580c" : "#fb923c");
        } else {
            col = (u01(rng) < 0.38f) ? p.hairDeep : ((u01(rng) < 0.80f) ? p.hairDark : p.hairMid);
        }

        std::string buf(160, '\0');
        int n = std::snprintf(buf.data(), buf.size(),
                              "d=\"M %.1f,%.1f L %.1f,%.1f\" stroke=\"%s\" stroke-width=\"1.0\" stroke-linecap=\"round\" opacity=\"0.82\"",
                              x0, y0, x1, y1, col);
        if (n > 0) { buf.resize(static_cast<size_t>(n)); ss << svgTag("path", buf); }
    }
    return wrapSvgCanvas(ss.str());
}

static std::string build3dGlassesSvg(int style)
{
    static constexpr std::array<std::array<const char*, 3>, 10> kPal = {{
        {"#1e293b", "#0f172a", "#38bdf8"}, {"#d97706", "#78350f", "#f59e0b"},
        {"#dc2626", "#7f1d1d", "#fda4af"}, {"#475569", "#1e293b", "#94a3b8"},
        {"#ca8a04", "#713f12", "#fef08a"}, {"#0284c7", "#0c4a6e", "#7dd3fc"},
        {"#7c3aed", "#3b0764", "#c4b5fd"}, {"#15803d", "#052e16", "#86efac"},
        {"#be123c", "#4c0519", "#fb7185"}, {"#f8fafc", "#64748b", "#e2e8f0"}
    }};
    const auto& c = kPal.at(style % 10);
    std::string b;
    if (style % 3 == 0) {
        b = svgTag("circle", std::string("cx=\"190\" cy=\"356\" r=\"84\" fill=\"") + c[2] + "\" fill-opacity=\"0.28\" stroke=\"" + c[0] + "\" stroke-width=\"12\"") +
            svgTag("circle", std::string("cx=\"410\" cy=\"356\" r=\"84\" fill=\"") + c[2] + "\" fill-opacity=\"0.28\" stroke=\"" + c[0] + "\" stroke-width=\"12\"") +
            svgTag("line", std::string("x1=\"274\" y1=\"350\" x2=\"326\" y2=\"350\" stroke=\"") + c[1] + "\" stroke-width=\"10\"");
    } else if (style % 3 == 1) {
        b = svgTag("rect", std::string("x=\"94\" y=\"292\" width=\"190\" height=\"126\" rx=\"22\" fill=\"") + c[2] + "\" fill-opacity=\"0.34\" stroke=\"" + c[0] + "\" stroke-width=\"13\"") +
            svgTag("rect", std::string("x=\"316\" y=\"292\" width=\"190\" height=\"126\" rx=\"22\" fill=\"") + c[2] + "\" fill-opacity=\"0.34\" stroke=\"" + c[0] + "\" stroke-width=\"13\"") +
            svgTag("rect", std::string("x=\"284\" y=\"332\" width=\"32\" height=\"15\" fill=\"") + c[1] + "\"");
    } else {
        b = svgTag("path", std::string("d=\"M 102,306 Q 190,292 276,312 Q 268,426 190,424 Q 112,422 102,306 Z\" fill=\"") + c[2] + "\" fill-opacity=\"0.42\" stroke=\"" + c[0] + "\" stroke-width=\"11\"") +
            svgTag("path", std::string("d=\"M 498,306 Q 410,292 324,312 Q 332,426 410,424 Q 488,422 498,306 Z\" fill=\"") + c[2] + "\" fill-opacity=\"0.42\" stroke=\"" + c[0] + "\" stroke-width=\"11\"") +
            svgTag("line", std::string("x1=\"274\" y1=\"328\" x2=\"326\" y2=\"328\" stroke=\"") + c[1] + "\" stroke-width=\"9\"");
    }
    return wrapSvgCanvas(b);
}

static std::string buildStrandedHairSvg(int style, const PersonSpec& p)
{
    std::mt19937 rng(5000 + style * 61);
    std::uniform_real_distribution<float> u01(0.0f, 1.0f);
    std::ostringstream ss;

    if (style == 4) {
        int locCount = 28;
        for (int i = 0; i < locCount; ++i) {
            float t = static_cast<float>(i) / static_cast<float>(locCount - 1);
            float ang = -3.05f + t * 2.96f;
            float x0 = 300.0f + std::cos(ang) * 204.0f;
            float y0 = 266.0f + std::sin(ang) * 56.0f;
            float side = (x0 < 300.0f) ? -1.0f : 1.0f;
            float outer = std::fabs(std::cos(ang));
            float len = 195.0f * (0.45f + 0.55f * outer);
            float x1 = x0 + side * (26.0f + outer * 28.0f);
            float y1 = y0 + len;
            float cx = (x0 + x1) * 0.5f + side * 14.0f;
            float cy = (y0 + y1) * 0.5f;

            std::string d = "M " + std::to_string(x0) + "," + std::to_string(y0) +
                            " Q " + std::to_string(cx) + "," + std::to_string(cy) +
                            " " + std::to_string(x1) + "," + std::to_string(y1);
            ss << svgTag("path", "d=\"" + d + "\" fill=\"none\" stroke=\"" + std::string(p.hairDeep) + "\" stroke-width=\"13\" stroke-linecap=\"round\"")
               << svgTag("path", "d=\"" + d + "\" fill=\"none\" stroke=\"" + std::string(p.hairDark) + "\" stroke-width=\"9\" stroke-dasharray=\"10 6\" stroke-linecap=\"round\"")
               << svgTag("path", "d=\"" + d + "\" fill=\"none\" stroke=\"" + std::string(p.hairMid) + "\" stroke-width=\"3.5\" stroke-dasharray=\"5 11\" stroke-linecap=\"round\"");

            if (i % 4 == 1 && outer > 0.55f) {
                ss << svgTag("circle", "cx=\"" + std::to_string(cx) + "\" cy=\"" + std::to_string(cy) + "\" r=\"6.5\" fill=\"#facc15\" stroke=\"#78350f\" stroke-width=\"2\"");
            }
        }
        return wrapSvgCanvas(ss.str());
    }

    if (style == 9) {
        for (int k = 0; k < 2600; ++k) {
            float th = -3.42f + u01(rng) * 3.70f;
            float r = std::sqrt(u01(rng));
            float x0 = 300.0f + std::cos(th) * (174.0f + r * 66.0f);
            float y0 = 289.0f + std::sin(th) * (88.0f + r * 46.0f);
            if (y0 > 258.0f && std::fabs(x0 - 300.0f) < 185.0f) continue;

            float a1 = u01(rng) * 6.28318f;
            float a2 = a1 + 2.2f;
            float rad = 6.0f + u01(rng) * 6.0f;
            float cx = x0 + std::cos(a1) * rad;
            float cy = y0 + std::sin(a1) * rad;
            float x1 = x0 + std::cos(a2) * rad;
            float y1 = y0 + std::sin(a2) * rad;
            const char* col = (u01(rng) < 0.42f) ? p.hairDeep : ((u01(rng) < 0.84f) ? p.hairDark : p.hairMid);

            std::string buf(160, '\0');
            int n = std::snprintf(buf.data(), buf.size(),
                                  "d=\"M %.1f,%.1f Q %.1f,%.1f %.1f,%.1f\" fill=\"none\" stroke=\"%s\" stroke-width=\"1.25\" stroke-linecap=\"round\" opacity=\"0.84\"",
                                  x0, y0, cx, cy, x1, y1, col);
            if (n > 0) { buf.resize(static_cast<size_t>(n)); ss << svgTag("path", buf); }
        }
        return wrapSvgCanvas(ss.str());
    }

    for (int k = 0; k < 2400; ++k) {
        float th = -3.35f + u01(rng) * 3.56f, v = u01(rng);
        float x0 = 300.0f + std::cos(th) * (192.0f + v * 30.0f);
        float y0 = 318.0f + std::sin(th) * (104.0f + v * 20.0f);

        if (style == 5) {
            float side = (k % 2 == 0) ? -1.0f : 1.0f;
            x0 = 300.0f + side * (194.0f + v * 30.0f);
            y0 = 262.0f + u01(rng) * 155.0f;
        } else if (std::fabs(x0 - 300.0f) < 166.0f && y0 > 238.0f) {
            y0 = 212.0f + v * 26.0f;
        }

        float len = (style == 0 || style == 2 || style == 7) ? 74.0f : ((style == 3 || style == 8) ? 46.0f : 26.0f);
        const char* col = (u01(rng) < 0.35f) ? p.hairDark : ((u01(rng) < 0.82f) ? p.hairMid : p.hairHi);
        float dx = (style == 5) ? ((x0 < 300.0f ? -1.0f : 1.0f) * (8.0f + v * 10.0f)) : ((x0 - 300.0f) * 0.09f);
        float dy = (style == 5) ? (22.0f + v * 14.0f) : len;

        std::string buf(160, '\0');
        int n = std::snprintf(buf.data(), buf.size(),
                              "d=\"M %.1f,%.1f L %.1f,%.1f\" stroke=\"%s\" stroke-width=\"1.05\" stroke-linecap=\"round\" opacity=\"0.78\"",
                              x0, y0, x0 + dx, y0 + dy, col);
        if (n > 0) { buf.resize(static_cast<size_t>(n)); ss << svgTag("path", buf); }
    }
    return wrapSvgCanvas(ss.str());
}


static std::string buildCulturalHatSvg(int style)
{
    switch (style) {
        case 0:
            return wrapSvgCanvas(svgTag("path", "d=\"M 118,242 C 132,118 468,118 482,242 Z\" fill=\"#1d4ed8\" stroke=\"#0f172a\" stroke-width=\"4\"") + svgTag("rect", "x=\"112\" y=\"214\" width=\"376\" height=\"34\" rx=\"14\" fill=\"#f8fafc\" stroke=\"#0f172a\" stroke-width=\"4\"") + svgTag("circle", "cx=\"300\" cy=\"112\" r=\"24\" fill=\"#ef4444\""));
        case 1:
            return wrapSvgCanvas(svgTag("ellipse", "cx=\"300\" cy=\"214\" rx=\"245\" ry=\"32\" fill=\"#451a03\" stroke=\"#1c0a00\" stroke-width=\"4\"") + svgTag("path", "d=\"M 168,210 L 186,86 Q 300,68 414,86 L 432,210 Z\" fill=\"#78350f\" stroke=\"#1c0a00\" stroke-width=\"4\"") + svgTag("rect", "x=\"172\" y=\"182\" width=\"256\" height=\"24\" fill=\"#f59e0b\""));
        case 2:
            return wrapSvgCanvas(svgTag("polygon", "points=\"44,232 300,96 556,232\" fill=\"#fde047\" stroke=\"#854d0e\" stroke-width=\"5\" stroke-linejoin=\"round\"") + svgTag("path", "d=\"M 120,218 Q 300,242 480,218\" fill=\"none\" stroke=\"#dc2626\" stroke-width=\"8\""));
        case 3:
            return wrapSvgCanvas(svgTag("path", "d=\"M 110,256 C 102,135 235,98 300,152 C 365,98 498,135 490,256 Q 300,218 110,256 Z\" fill=\"#e11d48\" stroke=\"#4c0519\" stroke-width=\"4\"") + svgTag("circle", "cx=\"300\" cy=\"142\" r=\"18\" fill=\"#facc15\" stroke=\"#92400e\" stroke-width=\"3\""));
        case 4:
            return wrapSvgCanvas(svgTag("path", "d=\"M 134,232 L 152,118 Q 300,96 448,118 L 466,232 Z\" fill=\"#ca8a04\" stroke=\"#422006\" stroke-width=\"4\"") + svgTag("rect", "x=\"140\" y=\"188\" width=\"320\" height=\"36\" fill=\"#15803d\"") + svgTag("path", "d=\"M 155,206 L 445,206\" stroke=\"#ef4444\" stroke-width=\"10\" stroke-dasharray=\"18 12\""));
        case 5:
            return wrapSvgCanvas(svgTag("rect", "x=\"108\" y=\"208\" width=\"384\" height=\"32\" rx=\"10\" fill=\"#1e293b\" stroke=\"#090d16\" stroke-width=\"4\"") + svgTag("path", "d=\"M 122,224 L 478,224\" stroke=\"#38bdf8\" stroke-width=\"12\" stroke-dasharray=\"16 10\"") + svgTag("circle", "cx=\"300\" cy=\"224\" r=\"16\" fill=\"#facc15\""));
        case 6:
            return wrapSvgCanvas(svgTag("ellipse", "cx=\"300\" cy=\"218\" rx=\"272\" ry=\"38\" fill=\"#d97706\" stroke=\"#451a03\" stroke-width=\"5\"") + svgTag("path", "d=\"M 178,212 L 204,82 Q 300,66 396,82 L 422,212 Z\" fill=\"#b45309\" stroke=\"#451a03\" stroke-width=\"4\"") + svgTag("path", "d=\"M 65,218 Q 300,248 535,218\" fill=\"none\" stroke=\"#ef4444\" stroke-width=\"9\" stroke-dasharray=\"14 10\""));
        case 7:
            return wrapSvgCanvas(svgTag("path", "d=\"M 165,212 L 142,84 L 192,208 Z\" fill=\"#f8fafc\" stroke=\"#334155\" stroke-width=\"3\"") + svgTag("path", "d=\"M 435,212 L 458,84 L 408,208 Z\" fill=\"#f8fafc\" stroke=\"#334155\" stroke-width=\"3\"") + svgTag("rect", "x=\"112\" y=\"206\" width=\"376\" height=\"28\" rx=\"8\" fill=\"#92400e\" stroke=\"#451a03\" stroke-width=\"4\""));
        case 8:
            return wrapSvgCanvas(
                svgTag("path", "d=\"M 82,226 C 88,118 512,118 518,226 L 558,545 L 512,545 L 506,246 L 94,246 L 88,545 L 42,545 Z\" fill=\"#f8fafc\" stroke=\"#94a3b8\" stroke-width=\"3\"") +
                svgTag("path", "d=\"M 96,232 Q 300,252 504,232\" fill=\"none\" stroke=\"#dc2626\" stroke-width=\"6\" stroke-dasharray=\"10 8\"") +
                svgTag("rect", "x=\"92\" y=\"188\" width=\"416\" height=\"24\" rx=\"11\" fill=\"#090d16\""));
        default:
            return wrapSvgCanvas(svgTag("path", "d=\"M 122,222 C 138,132 455,126 476,216 L 498,236 Q 300,254 114,228 Z\" fill=\"#15803d\" stroke=\"#052e16\" stroke-width=\"4\""));
    }
}

static std::string buildCulturalMakeupSvg(int style)
{
    std::string sid = std::to_string(style);
    std::string defs = svgTag("mask", "id=\"pmkup" + sid + "\"", false) + getStandardEyeCutoutMask() + svgClose("mask");
    std::ostringstream b;
    b << svgTag("g", "mask=\"url(#pmkup" + sid + ")\"", false);

    switch (style) {
        case 0:
            b << svgTag("path", "d=\"M 125,455 L 185,485 L 145,515 M 475,455 L 415,485 L 455,515\" fill=\"none\" stroke=\"#38bdf8\" stroke-width=\"5\" stroke-linecap=\"round\" opacity=\"0.82\"")
              << svgTag("circle", "cx=\"190\" cy=\"336\" r=\"72\" fill=\"#7dd3fc\" opacity=\"0.24\"")
              << svgTag("circle", "cx=\"410\" cy=\"336\" r=\"72\" fill=\"#7dd3fc\" opacity=\"0.24\"");
            break;
        case 1:
            b << svgTag("path", "d=\"M 118,348 C 135,292 245,292 262,348 C 245,404 135,404 118,348 Z\" fill=\"none\" stroke=\"#facc15\" stroke-width=\"5\" opacity=\"0.85\"")
              << svgTag("path", "d=\"M 482,348 C 465,292 355,292 338,348 C 355,404 465,404 482,348 Z\" fill=\"none\" stroke=\"#facc15\" stroke-width=\"5\" opacity=\"0.85\"")
              << svgTag("ellipse", "cx=\"162\" cy=\"488\" rx=\"38\" ry=\"20\" fill=\"#e11d48\" opacity=\"0.30\"")
              << svgTag("ellipse", "cx=\"438\" cy=\"488\" rx=\"38\" ry=\"20\" fill=\"#e11d48\" opacity=\"0.30\"");
            break;
        case 2:
            b << svgTag("ellipse", "cx=\"186\" cy=\"338\" rx=\"78\" ry=\"42\" fill=\"#fb7185\" opacity=\"0.46\"")
              << svgTag("ellipse", "cx=\"414\" cy=\"338\" rx=\"78\" ry=\"42\" fill=\"#fb7185\" opacity=\"0.46\"")
              << svgTag("path", "d=\"M 122,342 L 94,324 M 478,342 L 506,324\" stroke=\"#e11d48\" stroke-width=\"6\" stroke-linecap=\"round\"")
              << svgTag("circle", "cx=\"300\" cy=\"268\" r=\"7\" fill=\"#e11d48\"")
              << svgTag("circle", "cx=\"290\" cy=\"276\" r=\"4.5\" fill=\"#fb7185\"")
              << svgTag("circle", "cx=\"310\" cy=\"276\" r=\"4.5\" fill=\"#fb7185\"");
            break;
        case 3:
            b << svgTag("circle", "cx=\"300\" cy=\"282\" r=\"11\" fill=\"#dc2626\" stroke=\"#facc15\" stroke-width=\"3\"")
              << svgTag("ellipse", "cx=\"190\" cy=\"348\" rx=\"76\" ry=\"44\" fill=\"#1e1b4b\" opacity=\"0.48\"")
              << svgTag("ellipse", "cx=\"410\" cy=\"348\" rx=\"76\" ry=\"44\" fill=\"#1e1b4b\" opacity=\"0.48\"");
            for (int dx : {-55, -35, -15, 15, 35, 55}) {
                b << svgTag("circle", "cx=\"" + std::to_string(190 + dx) + "\" cy=\"264\" r=\"3.2\" fill=\"#facc15\" opacity=\"0.85\"")
                  << svgTag("circle", "cx=\"" + std::to_string(410 + dx) + "\" cy=\"264\" r=\"3.2\" fill=\"#facc15\" opacity=\"0.85\"");
            }
            break;
        case 4:
            for (int dx : {-32, -16, 0, 16, 32}) {
                b << svgTag("circle", "cx=\"" + std::to_string(158 + dx) + "\" cy=\"482\" r=\"5\" fill=\"#fef9c3\" opacity=\"0.90\"")
                  << svgTag("circle", "cx=\"" + std::to_string(442 + dx) + "\" cy=\"482\" r=\"5\" fill=\"#fef9c3\" opacity=\"0.90\"");
            }
            b << svgTag("path", "d=\"M 128,504 L 192,504 M 408,504 L 472,504\" stroke=\"#facc15\" stroke-width=\"5\" stroke-linecap=\"round\"");
            break;
        case 5:
            b << svgTag("path", "d=\"M 122,470 Q 162,456 202,474 M 398,474 Q 438,456 478,470\" fill=\"none\" stroke=\"#ef4444\" stroke-width=\"8\" stroke-linecap=\"round\"")
              << svgTag("path", "d=\"M 126,488 Q 162,476 198,492 M 402,492 Q 438,476 474,488\" fill=\"none\" stroke=\"#ffffff\" stroke-width=\"4\" stroke-linecap=\"round\"")
              << svgTag("circle", "cx=\"300\" cy=\"286\" r=\"7\" fill=\"#facc15\"");
            break;
        case 6:
            for (int a = 0; a < 360; a += 30) {
                float rad = static_cast<float>(a) * 3.14159f / 180.0f;
                int pxL = 190 + static_cast<int>(std::cos(rad) * 76.0f);
                int pyL = 348 + static_cast<int>(std::sin(rad) * 52.0f);
                int pxR = 410 + static_cast<int>(std::cos(rad) * 76.0f);
                int pyR = 348 + static_cast<int>(std::sin(rad) * 52.0f);
                b << svgTag("circle", "cx=\"" + std::to_string(pxL) + "\" cy=\"" + std::to_string(pyL) + "\" r=\"6\" fill=\"#f59e0b\" opacity=\"0.85\"")
                  << svgTag("circle", "cx=\"" + std::to_string(pxR) + "\" cy=\"" + std::to_string(pyR) + "\" r=\"6\" fill=\"#f59e0b\" opacity=\"0.85\"");
            }
            break;
        case 7:
            b << svgTag("rect", "x=\"122\" y=\"470\" width=\"74\" height=\"10\" rx=\"4\" fill=\"#38bdf8\" opacity=\"0.85\"")
              << svgTag("rect", "x=\"128\" y=\"488\" width=\"64\" height=\"8\" rx=\"4\" fill=\"#ef4444\" opacity=\"0.85\"")
              << svgTag("rect", "x=\"404\" y=\"470\" width=\"74\" height=\"10\" rx=\"4\" fill=\"#38bdf8\" opacity=\"0.85\"")
              << svgTag("rect", "x=\"408\" y=\"488\" width=\"64\" height=\"8\" rx=\"4\" fill=\"#ef4444\" opacity=\"0.85\"");
            break;
        case 8:
            b << svgTag("ellipse", "cx=\"190\" cy=\"352\" rx=\"78\" ry=\"46\" fill=\"#090d16\" opacity=\"0.44\"")
              << svgTag("ellipse", "cx=\"410\" cy=\"352\" rx=\"78\" ry=\"46\" fill=\"#090d16\" opacity=\"0.44\"")
              << svgTag("path", "d=\"M 118,352 L 96,348 M 482,352 L 504,348\" stroke=\"#090d16\" stroke-width=\"6\" stroke-linecap=\"round\"");
            break;
        default:
            b << svgTag("path", "d=\"M 126,462 Q 168,438 196,476 Q 168,514 134,492 Q 152,468 176,482\" fill=\"none\" stroke=\"#0284c7\" stroke-width=\"6\" stroke-linecap=\"round\" opacity=\"0.82\"")
              << svgTag("path", "d=\"M 474,462 Q 432,438 404,476 Q 432,514 466,492 Q 448,468 424,482\" fill=\"none\" stroke=\"#0284c7\" stroke-width=\"6\" stroke-linecap=\"round\" opacity=\"0.82\"");
            break;
    }

    b << svgClose("g");
    return wrapSvgCanvas(b.str(), defs);
}

} // namespace

// ==============================================================================
// 1. MAIN GENERATOR: 10 PEOPLE OF THE WORLD + 5x10=50 SIDE-PROFILE PAIRS (127..226)
// ==============================================================================
int main(void)
{
    std::cout << "=== Generating MakingFaces People of the World Pack (10x Front + 50x Side-Profile Pairs) ===\n";
    ensureBaseIni("makingfaces.ini");
    generateDefaultFxBank();

    static constexpr std::array<WorldSceneSpec, 10> kScenes = {{
        {"#0284c7", "#bae6fd", "#fef08a", 455, 130, "#1e3a8a", "#14532d", "#15803d", 0, false},
        {"#0369a1", "#fed7aa", "#fde047", 470, 185, "#7c2d12", "#b45309", "#15803d", 1, false},
        {"#38bdf8", "#fce7f3", "#ffffff", 445, 145, "#475569", "#f472b6", "#166534", 2, false},
        {"#7c2d12", "#fb923c", "#fef08a", 300, 195, "#9a3412", "#451a03", "#1c0a00", 3, false},
        {"#d97706", "#fef08a", "#ffffff", 460, 155, "#92400e", "#78350f", "#3f6212", 4, false},
        {"#0284c7", "#fde68a", "#fef9c3", 160, 140, "#334155", "#65a30d", "#365314", 5, false},
        {"#1d4ed8", "#f472b6", "#fde047", 450, 175, "#be123c", "#ea580c", "#15803d", 6, false},
        {"#9a3412", "#fdba74", "#fef08a", 180, 150, "#b45309", "#78350f", "#451a03", 7, false},
        {"#090d16", "#312e81", "#fef9c3", 445, 145, "#1e1b4b", "#451a03", "#1c1917", 8, true},
        {"#0284c7", "#86efac", "#fef08a", 165, 135, "#15803d", "#16a34a", "#14532d", 9, false}
    }};

    std::vector<int> bgs;
    for (const auto& sc : kScenes) bgs.push_back(addWorldBackdropScene(sc));

    static constexpr std::array<PersonSpec, 10> kPeople = {{
        {"Astrid_Nordic",     "oval",    "#fff0e6", "#f5c8ad", "#c48866", "#6e4229", "slim",     2, "#664900", "#ad8000", "#e6b419", "#ffe875", 0, 1, 8, 0, 0, 0, 1, 4,  0, 16, false, false, false, true},
        {"Marco_Med",         "oval",    "#f2c49b", "#c98857", "#854c25", "#42210b", "aquiline", 0, "#1f1008", "#4d2c18", "#7d4b2d", "#a86d48", 3, 0, 0, 1, 1, 1, 0, 0,  0, 10, false, true,  true,  false},
        {"Mei_EastAsian",     "heart",   "#fff2dc", "#ebd0a8", "#bd9664", "#6b4d2a", "button",   2, "#050507", "#18181f", "#32323d", "#565666", 2, 1, 8, 2, 2, 2, 3, 6,  0, 25, true,  false, false, true},
        {"Priya_SouthAsian",  "heart",   "#d19264", "#9c5b30", "#5e3012", "#2b1304", "slim",     3, "#050507", "#18181f", "#32323d", "#565666", 7, 2, 8, 3, 3, 3, 4, 8,  0, 22, false, false, false, true},
        {"Kwame_WestAfrican", "round",   "#9e633e", "#693b1f", "#381b0a", "#170902", "broad",    1, "#050507", "#18181f", "#32323d", "#565666", 4, 5, 1, 4, 4, 4, 0, 0,  0, 48, true,  false, true,  true},
        {"Zuri_EastAfrican",  "slender", "#754326", "#4a2612", "#240f04", "#0d0401", "classic",  1, "#050507", "#18181f", "#32323d", "#565666", 9, 2, 8, 5, 5, 5, 6, 7,  0, 12, true,  false, false, true},
        {"Mateo_Latin",       "oval",    "#e6ad7e", "#b57243", "#733e1b", "#381b08", "classic",  0, "#1f1008", "#4d2c18", "#7d4b2d", "#a86d48", 8, 0, 6, 6, 6, 6, 0, 0,  0, 64, true,  false, true,  false},
        {"Aiyana_Indigenous", "heart",   "#d49566", "#96582e", "#4d270e", "#241004", "aquiline", 4, "#050507", "#18181f", "#32323d", "#565666", 0, 0, 8, 7, 7, 7, 2, 4,  0, 44, true,  false, false, true},
        {"Tariq_MiddleEast",  "slender", "#eab68a", "#b8794b", "#6e3e1e", "#361b0a", "aquiline", 0, "#050507", "#18181f", "#32323d", "#565666", 3, 7, 1, 8, 8, 8, 0, 0,  0, 61, true,  false, true,  false},
        {"Sean_Celtic",       "round",   "#ffe4d1", "#eab08c", "#b5724d", "#63361e", "button",   0, "#3d0902", "#7d1b0a", "#bd361c", "#eb6546", 5, 5, 4, 9, 9, 9, 0, 0,  0, 46, true,  true,  true,  false}
    }};

    std::vector<int> fIds, eIds, nIds, brIds, bdIds, glIds, hrIds, htIds, mkIds;
    for (int i = 0; i < 10; ++i) {
        const auto& p = kPeople.at(i);
        fIds.push_back(saveSvgAsset("face", build3dHumanFaceSvg(i + 1, p)));
        eIds.push_back(saveSvgAsset("ears", build3dHumanEarsSvg(i + 1, p)));
        nIds.push_back(saveSvgAsset("nose", build3dHumanNoseSvg(i + 1, p)));
        brIds.push_back(saveSvgAsset("brows", buildStrandedBrowSvg(p.browStyle, p)));
        bdIds.push_back(saveSvgAsset("beard", buildStrandedBeardSvg(p.beardStyle, p)));
        glIds.push_back(saveSvgAsset("glasses", build3dGlassesSvg(i)));
        hrIds.push_back(saveSvgAsset("hair", buildStrandedHairSvg(p.hairStyle, p)));
        htIds.push_back(saveSvgAsset("hat", buildCulturalHatSvg(i)));
        mkIds.push_back(saveSvgAsset("makeup", buildCulturalMakeupSvg(i)));
    }

    for (int i = 0; i < 10; ++i) {
        const auto& p = kPeople.at(i);
        appendFaceOffsets(fIds.at(i), {{"mouth_y", "12"}, {"brows_y", "-4"}});
        appendMacroCombo({
            fIds.at(i),
            p.wearMakeup ? mkIds.at(i) : 0,
            eIds.at(i),
            nIds.at(i),
            p.wearBeard ? bdIds.at(i) : 0,
            p.wearGlasses ? glIds.at(i) : 0,
            hrIds.at(i),
            p.wearHat ? htIds.at(i) : 0,
            brIds.at(i),
            bgs.at(i),
            0, 1, p.lipColor, p.eyelash, -1, -1, p.animId, false, p.eyeBank
        });
    }

    static constexpr std::array<const char*, 5> kFiveChins = {"round", "square", "pointed", "cleft", "double"};
    static constexpr std::array<const char*, 5> kFiveNoses = {"classic", "aquiline", "button", "broad", "long"};

    int nextProfSlot = 127;
    for (int i = 0; i < 10; ++i) {
        const auto& p = kPeople.at(i);
        for (int v = 0; v < 5; ++v) {
            int idL = nextProfSlot++;
            int idR = nextProfSlot++;
            const char* cStyle = kFiveChins.at(v);
            const char* nStyle = kFiveNoses.at(v);
            saveSvgAssetAt("face",    idL, build3dProfileHeadSvg("ppl" + std::to_string(idL), "left",  p.skinHi, p.skinMid, p.skinDeep, p.skinHi, p.skinMid, p.skinDeep, cStyle));
            saveSvgAssetAt("face",    idR, build3dProfileHeadSvg("ppr" + std::to_string(idR), "right", p.skinHi, p.skinMid, p.skinDeep, p.skinHi, p.skinMid, p.skinDeep, cStyle));
            saveSvgAssetAt("makeup",  idL, build3dProfileMakeupSvg("left",  (i + v) % 3));
            saveSvgAssetAt("makeup",  idR, build3dProfileMakeupSvg("right", (i + v) % 3));
            saveSvgAssetAt("ears",    idL, build3dProfileEarsSvg("pel" + std::to_string(idL), "left",  p.skinHi, p.skinMid, p.skinDeep, p.skinMid, p.skinDeep, 44));
            saveSvgAssetAt("ears",    idR, build3dProfileEarsSvg("per" + std::to_string(idR), "right", p.skinHi, p.skinMid, p.skinDeep, p.skinMid, p.skinDeep, 44));
            saveSvgAssetAt("nose",    idL, build3dProfileNoseSvg("pnl" + std::to_string(idL), "left",  p.skinHi, p.skinMid, p.skinDeep, nStyle));
            saveSvgAssetAt("nose",    idR, build3dProfileNoseSvg("pnr" + std::to_string(idR), "right", p.skinHi, p.skinMid, p.skinDeep, nStyle));
            saveSvgAssetAt("brows",   idL, build3dProfileBrowsSvg("left",  p.hairDeep, p.hairMid, (p.browStyle + v) % 6));
            saveSvgAssetAt("brows",   idR, build3dProfileBrowsSvg("right", p.hairDeep, p.hairMid, (p.browStyle + v) % 6));
            saveSvgAssetAt("beard",   idL, build3dProfileBeardSvg("left",  p.hairDeep, p.hairDark, p.hairMid, (i + v) % 3));
            saveSvgAssetAt("beard",   idR, build3dProfileBeardSvg("right", p.hairDeep, p.hairDark, p.hairMid, (i + v) % 3));
            saveSvgAssetAt("glasses", idL, build3dProfileGlassesSvg("left",  (i + v) % 3));
            saveSvgAssetAt("glasses", idR, build3dProfileGlassesSvg("right", (i + v) % 3));
            saveSvgAssetAt("hair",    idL, build3dProfileHairSvg("left",  p.hairDeep, p.hairDark, p.hairMid, (p.hairStyle + v) % 8));
            saveSvgAssetAt("hair",    idR, build3dProfileHairSvg("right", p.hairDeep, p.hairDark, p.hairMid, (p.hairStyle + v) % 8));
            saveSvgAssetAt("hat",     idL, build3dProfileHatSvg("left",  (i + v) % 3));
            saveSvgAssetAt("hat",     idR, build3dProfileHatSvg("right", (i + v) % 3));
            appendFaceOffsets(idL, {{"face_split", "75"}});
            appendFaceOffsets(idR, {{"face_split", "75"}});

            int mkId = p.wearMakeup  ? idL : 0;
            int bdId = p.wearBeard   ? idL : 0;
            int glId = p.wearGlasses ? idL : 0;
            int htId = p.wearHat     ? idL : 0;
            appendMacroCombo({idL, mkId, idL, idL, bdId, glId, idL, htId, idL, bgs.at(i), 0, 1, p.lipColor, p.eyelash, -1, -1, p.animId, (v % 2 == 1), p.eyeBank});
        }
    }

    cleanSvgAndSaveZip("people", "packs");
    std::cout << "[OK] 100% C++17 People of the World (10 Front + 50 Side-Profile Pairs 127..226) zipped to packs/people.zip!\n";
    return 0;
}
