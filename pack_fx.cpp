// ==============================================================================
// MakingFaces - Theme Pack FX (C++17): 3D Animated Critters, Foliage & Weather
// Architecture designed by Magnus Herrlin 2026
// ==============================================================================

#include "pack_utils.h"
#include "stage_engine.h"
#include <opencv2/opencv.hpp>
#include <array>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <sstream>
#include <string>

namespace fs = std::filesystem;

namespace {

// ------------------------------------------------------------------------------
// 1. Build 3D-shaded 3-pose bat (0=wings high, 1=wings level, 2=wings low)
// ------------------------------------------------------------------------------
std::string build3dBatSvg(int phase)
{
    std::string defs =
        svgTag("radialGradient", "id=\"batBody\" cx=\"42%\" cy=\"32%\" r=\"68%\"", false) +
        svgTag("stop", "offset=\"0%\" stop-color=\"#71717a\"") +
        svgTag("stop", "offset=\"55%\" stop-color=\"#3f3f46\"") +
        svgTag("stop", "offset=\"100%\" stop-color=\"#09090b\"") + svgClose("radialGradient") +
        svgTag("linearGradient", "id=\"batWing\" x1=\"0%\" y1=\"0%\" x2=\"100%\" y2=\"100%\"", false) +
        svgTag("stop", "offset=\"0%\" stop-color=\"#52525b\"") +
        svgTag("stop", "offset=\"60%\" stop-color=\"#27272a\"") +
        svgTag("stop", "offset=\"100%\" stop-color=\"#09090b\"") + svgClose("linearGradient");

    std::string wings;
    if (phase == 0) {
        wings = svgTag("path", "d=\"M 72,66 Q 42,12 8,32 Q 26,54 42,48 Q 54,66 72,62 Z\" fill=\"url(#batWing)\" stroke=\"#09090b\" stroke-width=\"2.5\"") +
                svgTag("path", "d=\"M 78,66 Q 108,12 142,32 Q 124,54 108,48 Q 96,66 78,62 Z\" fill=\"url(#batWing)\" stroke=\"#09090b\" stroke-width=\"2.5\"");
    } else if (phase == 1) {
        wings = svgTag("path", "d=\"M 72,66 Q 36,44 6,60 Q 26,72 44,64 Q 56,74 72,68 Z\" fill=\"url(#batWing)\" stroke=\"#09090b\" stroke-width=\"2.5\"") +
                svgTag("path", "d=\"M 78,66 Q 114,44 144,60 Q 124,72 106,64 Q 94,74 78,68 Z\" fill=\"url(#batWing)\" stroke=\"#09090b\" stroke-width=\"2.5\"");
    } else {
        wings = svgTag("path", "d=\"M 72,66 Q 36,68 12,104 Q 34,94 48,82 Q 60,90 72,74 Z\" fill=\"url(#batWing)\" stroke=\"#09090b\" stroke-width=\"2.5\"") +
                svgTag("path", "d=\"M 78,66 Q 114,68 138,104 Q 116,94 102,82 Q 90,90 78,74 Z\" fill=\"url(#batWing)\" stroke=\"#09090b\" stroke-width=\"2.5\"");
    }

    std::string body =
        wings +
        svgTag("ellipse", "cx=\"75\" cy=\"68\" rx=\"13\" ry=\"20\" fill=\"url(#batBody)\" stroke=\"#09090b\" stroke-width=\"2.5\"") +
        svgTag("polygon", "points=\"65,52 68,36 73,50\" fill=\"#27272a\"") +
        svgTag("polygon", "points=\"85,52 82,36 77,50\" fill=\"#27272a\"") +
        svgTag("circle", "cx=\"70\" cy=\"60\" r=\"2.8\" fill=\"#ef4444\"") +
        svgTag("circle", "cx=\"80\" cy=\"60\" r=\"2.8\" fill=\"#ef4444\"") +
        svgTag("polygon", "points=\"71,69 73,74 75,69\" fill=\"#ffffff\"") +
        svgTag("polygon", "points=\"79,69 77,74 75,69\" fill=\"#ffffff\"");
    return wrapSvgCanvas(body, defs, 150, 120);
}

// ------------------------------------------------------------------------------
// 2. Build 3D-shaded animated monarch butterfly (frame 0 = open, frame 1 = folded)
// ------------------------------------------------------------------------------
std::string build3dButterflySvg(int frame)
{
    std::string defs =
        svgTag("radialGradient", "id=\"bfWing\" cx=\"45%\" cy=\"35%\" r=\"65%\"", false) +
        svgTag("stop", "offset=\"0%\" stop-color=\"#fde047\"") +
        svgTag("stop", "offset=\"52%\" stop-color=\"#f97316\"") +
        svgTag("stop", "offset=\"100%\" stop-color=\"#9a3412\"") + svgClose("radialGradient");

    float span = (frame == 0) ? 1.0f : 0.42f;
    int wOff = static_cast<int>(std::round(56.0f * span));
    std::string body =
        svgTag("path", "d=\"M 70,56 C " + std::to_string(70 - wOff) + ",14 " + std::to_string(70 - wOff - 8) + ",58 " + std::to_string(70 - wOff / 2) + ",72 C " + std::to_string(70 - wOff) + ",92 " + std::to_string(70 - wOff / 3) + ",106 70,78 Z\" fill=\"url(#bfWing)\" stroke=\"#090d16\" stroke-width=\"3\"") +
        svgTag("path", "d=\"M 70,56 C " + std::to_string(70 + wOff) + ",14 " + std::to_string(70 + wOff + 8) + ",58 " + std::to_string(70 + wOff / 2) + ",72 C " + std::to_string(70 + wOff) + ",92 " + std::to_string(70 + wOff / 3) + ",106 70,78 Z\" fill=\"url(#bfWing)\" stroke=\"#090d16\" stroke-width=\"3\"") +
        svgTag("ellipse", "cx=\"70\" cy=\"64\" rx=\"5\" ry=\"24\" fill=\"#090d16\"") +
        svgTag("path", "d=\"M 68,42 Q 58,26 52,24 M 72,42 Q 82,26 88,24\" fill=\"none\" stroke=\"#090d16\" stroke-width=\"2\" stroke-linecap=\"round\"");
    return wrapSvgCanvas(body, defs, 140, 120);
}

// ------------------------------------------------------------------------------
// 3. Build 3D-shaded animated bird (frame 0 = soaring up, frame 1 = downstroke)
// ------------------------------------------------------------------------------
std::string build3dBirdSvg(int frame)
{
    std::string defs =
        svgTag("linearGradient", "id=\"brdBody\" x1=\"0%\" y1=\"0%\" x2=\"100%\" y2=\"100%\"", false) +
        svgTag("stop", "offset=\"0%\" stop-color=\"#38bdf8\"") +
        svgTag("stop", "offset=\"60%\" stop-color=\"#0369a1\"") +
        svgTag("stop", "offset=\"100%\" stop-color=\"#0c4a6e\"") + svgClose("linearGradient");

    std::string wing = (frame == 0)
        ? svgTag("path", "d=\"M 68,56 Q 48,14 26,28 Q 42,50 56,58 Z\" fill=\"#0284c7\" stroke=\"#082f49\" stroke-width=\"2\"")
        : svgTag("path", "d=\"M 68,62 Q 46,96 28,88 Q 44,70 56,64 Z\" fill=\"#0284c7\" stroke=\"#082f49\" stroke-width=\"2\"");

    std::string body =
        svgTag("polygon", "points=\"34,58 10,48 16,62 10,72 34,64\" fill=\"#0c4a6e\"") +
        svgTag("ellipse", "cx=\"68\" cy=\"60\" rx=\"32\" ry=\"15\" fill=\"url(#brdBody)\" stroke=\"#082f49\" stroke-width=\"2\"") +
        svgTag("circle", "cx=\"96\" cy=\"54\" r=\"12\" fill=\"url(#brdBody)\" stroke=\"#082f49\" stroke-width=\"2\"") +
        svgTag("polygon", "points=\"107,52 124,56 107,60\" fill=\"#facc15\"") +
        svgTag("circle", "cx=\"99\" cy=\"51\" r=\"2.5\" fill=\"#090d16\"") +
        wing;
    return wrapSvgCanvas(body, defs, 140, 110);
}
// ------------------------------------------------------------------------------
// 4. Build 3D-shaded animated mouse with short black legs & paws (frames 0..1)
// ------------------------------------------------------------------------------
std::string build3dMouseWithFeetSvg(int frame)
{
    std::string defs =
        svgTag("radialGradient", "id=\"ms3d\" cx=\"45%\" cy=\"32%\" r=\"68%\"", false) +
        svgTag("stop", "offset=\"0%\" stop-color=\"#e2e8f0\"") +
        svgTag("stop", "offset=\"55%\" stop-color=\"#64748b\"") +
        svgTag("stop", "offset=\"100%\" stop-color=\"#1e293b\"") + svgClose("radialGradient");

    int stride = (frame == 0) ? -4 : 4;
    std::ostringstream feet;
    feet << svgTag("path", "d=\"M 64,74 L " + std::to_string(61 - stride) + ",79 L " + std::to_string(66 - stride) + ",80\" fill=\"none\" stroke=\"#1e293b\" stroke-width=\"3.5\" stroke-linecap=\"round\"")
         << svgTag("path", "d=\"M 104,74 L " + std::to_string(107 + stride) + ",79 L " + std::to_string(112 + stride) + ",80\" fill=\"none\" stroke=\"#1e293b\" stroke-width=\"3.5\" stroke-linecap=\"round\"")
         << svgTag("path", "d=\"M 70,76 L " + std::to_string(68 + stride) + ",81 L " + std::to_string(74 + stride) + ",82\" fill=\"none\" stroke=\"#090d16\" stroke-width=\"4\" stroke-linecap=\"round\"")
         << svgTag("path", "d=\"M 110,76 L " + std::to_string(108 - stride) + ",81 L " + std::to_string(114 - stride) + ",82\" fill=\"none\" stroke=\"#090d16\" stroke-width=\"4\" stroke-linecap=\"round\"");

    std::string body =
        svgTag("path", "d=\"M 44,58 Q 14,42 8,22 Q 5,12 17,15\" fill=\"none\" stroke=\"#f472b6\" stroke-width=\"4.5\" stroke-linecap=\"round\"") +
        feet.str() +
        svgTag("ellipse", "cx=\"86\" cy=\"55\" rx=\"44\" ry=\"24\" fill=\"url(#ms3d)\" stroke=\"#0f172a\" stroke-width=\"2\"") +
        svgTag("circle", "cx=\"106\" cy=\"34\" r=\"15\" fill=\"#64748b\"") +
        svgTag("circle", "cx=\"108\" cy=\"34\" r=\"9\" fill=\"#fda4af\"") +
        svgTag("circle", "cx=\"120\" cy=\"38\" r=\"14\" fill=\"#94a3b8\"") +
        svgTag("circle", "cx=\"122\" cy=\"38\" r=\"8\" fill=\"#fda4af\"") +
        svgTag("circle", "cx=\"126\" cy=\"48\" r=\"4.5\" fill=\"#090d16\"") +
        svgTag("circle", "cx=\"127\" cy=\"47\" r=\"1.6\" fill=\"#ffffff\"") +
        svgTag("circle", "cx=\"142\" cy=\"56\" r=\"5.5\" fill=\"#fb7185\"") +
        svgTag("path", "d=\"M 132,54 L 150,46 M 132,58 L 152,58 M 132,62 L 149,68\" stroke=\"#f8fafc\" stroke-width=\"1.8\" stroke-linecap=\"round\"");
    return wrapSvgCanvas(body, defs, 160, 90);
}

// ------------------------------------------------------------------------------
// 5. Build 3D-shaded animated grasshopper with hind jumping legs & wings
// ------------------------------------------------------------------------------
std::string build3dGrasshopperSvg(int frame)
{
    std::string defs =
        svgTag("linearGradient", "id=\"gh3d\" x1=\"0%\" y1=\"0%\" x2=\"100%\" y2=\"100%\"", false) +
        svgTag("stop", "offset=\"0%\" stop-color=\"#bef264\"") +
        svgTag("stop", "offset=\"55%\" stop-color=\"#4d7c0f\"") +
        svgTag("stop", "offset=\"100%\" stop-color=\"#1a2e05\"") + svgClose("linearGradient");

    int kneeY = (frame == 0) ? 24 : 38;
    int step  = (frame == 0) ? -6 : 6;
    std::string body =
        svgTag("path", "d=\"M 68,58 L 42," + std::to_string(kneeY) + " L 22,78\" fill=\"none\" stroke=\"#365314\" stroke-width=\"6\" stroke-linecap=\"round\" stroke-linejoin=\"round\"") +
        svgTag("path", "d=\"M 86,62 L " + std::to_string(82 + step) + ",82 M 102,62 L " + std::to_string(106 - step) + ",82\" stroke=\"#1a2e05\" stroke-width=\"3.5\" stroke-linecap=\"round\"") +
        svgTag("ellipse", "cx=\"72\" cy=\"56\" rx=\"38\" ry=\"12\" fill=\"url(#gh3d)\" stroke=\"#1a2e05\" stroke-width=\"2\"") +
        svgTag("ellipse", "cx=\"112\" cy=\"50\" rx=\"13\" ry=\"11\" fill=\"url(#gh3d)\" stroke=\"#1a2e05\" stroke-width=\"2\"") +
        svgTag("circle", "cx=\"116\" cy=\"47\" r=\"3.5\" fill=\"#090d16\"") +
        svgTag("path", "d=\"M 122,42 Q 138,24 148,22 M 122,46 Q 140,32 150,32\" fill=\"none\" stroke=\"#365314\" stroke-width=\"2\" stroke-linecap=\"round\"");
    return wrapSvgCanvas(body, defs, 156, 92);
}

// ------------------------------------------------------------------------------
// 6. Build 3D-shaded animated ant with 6 scurrying legs & antennae
// ------------------------------------------------------------------------------
std::string build3dAntSvg(int frame)
{
    std::string defs =
        svgTag("radialGradient", "id=\"ant3d\" cx=\"40%\" cy=\"30%\" r=\"68%\"", false) +
        svgTag("stop", "offset=\"0%\" stop-color=\"#b91c1c\"") +
        svgTag("stop", "offset=\"60%\" stop-color=\"#450a0a\"") +
        svgTag("stop", "offset=\"100%\" stop-color=\"#090d16\"") + svgClose("radialGradient");

    int s = (frame == 0) ? -5 : 5;
    std::string body =
        svgTag("path", "d=\"M 46,48 L " + std::to_string(38 + s) + ",66 M 62,48 L " + std::to_string(62 - s) + ",66 M 78,48 L " + std::to_string(84 + s) + ",66\" stroke=\"#090d16\" stroke-width=\"3\" stroke-linecap=\"round\"") +
        svgTag("ellipse", "cx=\"34\" cy=\"44\" rx=\"16\" ry=\"12\" fill=\"url(#ant3d)\"") +
        svgTag("ellipse", "cx=\"60\" cy=\"45\" rx=\"11\" ry=\"8\" fill=\"url(#ant3d)\"") +
        svgTag("circle", "cx=\"82\" cy=\"42\" r=\"10\" fill=\"url(#ant3d)\"") +
        svgTag("circle", "cx=\"86\" cy=\"39\" r=\"2.2\" fill=\"#facc15\"") +
        svgTag("path", "d=\"M 90,36 Q 102,24 108,24 M 90,40 Q 104,32 110,34\" fill=\"none\" stroke=\"#090d16\" stroke-width=\"2\" stroke-linecap=\"round\"");
    return wrapSvgCanvas(body, defs, 120, 76);
}
// ------------------------------------------------------------------------------
// 7. Build 3D-shaded wasp, mosquito, realistic fly, firefly, spider & snake
// ------------------------------------------------------------------------------
std::string build3dWaspSvg(int frame)
{
    int wy = (frame == 0) ? 26 : 36;
    std::string body =
        svgTag("ellipse", "cx=\"56\" cy=\"" + std::to_string(wy) + "\" rx=\"18\" ry=\"8\" transform=\"rotate(-22 56 " + std::to_string(wy) + ")\" fill=\"#e0f2fe\" opacity=\"0.82\"") +
        svgTag("ellipse", "cx=\"68\" cy=\"" + std::to_string(wy) + "\" rx=\"18\" ry=\"8\" transform=\"rotate(22 68 " + std::to_string(wy) + ")\" fill=\"#e0f2fe\" opacity=\"0.82\"") +
        svgTag("ellipse", "cx=\"58\" cy=\"54\" rx=\"24\" ry=\"13\" fill=\"#facc15\" stroke=\"#090d16\" stroke-width=\"2.5\"") +
        svgTag("rect", "x=\"46\" y=\"42\" width=\"5\" height=\"24\" fill=\"#090d16\"") +
        svgTag("rect", "x=\"58\" y=\"41\" width=\"5\" height=\"26\" fill=\"#090d16\"") +
        svgTag("rect", "x=\"70\" y=\"43\" width=\"5\" height=\"22\" fill=\"#090d16\"") +
        svgTag("polygon", "points=\"34,54 20,54 34,58\" fill=\"#090d16\"") +
        svgTag("circle", "cx=\"86\" cy=\"52\" r=\"9\" fill=\"#1e293b\"");
    return wrapSvgCanvas(body, "", 120, 96);
}

std::string build3dMosquitoSvg(int frame)
{
    std::string wing = (frame == 0)
        ? svgTag("path", "d=\"M 58,46 Q 42,24 26,30 Q 40,42 54,48 Z\" fill=\"#e2e8f0\" stroke=\"#64748b\" stroke-width=\"1.2\" opacity=\"0.82\"")
        : svgTag("path", "d=\"M 58,48 Q 40,36 28,44 Q 42,50 54,50 Z\" fill=\"#cbd5e1\" stroke=\"#64748b\" stroke-width=\"1.2\" opacity=\"0.72\"");

    std::string body =
        svgTag("path", "d=\"M 46,52 L 30,74 L 22,76 M 58,52 L 54,78 L 48,80 M 70,51 L 84,74 L 92,76\" fill=\"none\" stroke=\"#1e293b\" stroke-width=\"2\" stroke-linecap=\"round\" stroke-linejoin=\"round\"") +
        svgTag("ellipse", "cx=\"44\" cy=\"52\" rx=\"16\" ry=\"4.5\" transform=\"rotate(-10 44 52)\" fill=\"#475569\" stroke=\"#0f172a\" stroke-width=\"1.8\"") +
        svgTag("ellipse", "cx=\"64\" cy=\"48\" rx=\"9\" ry=\"6.5\" fill=\"#334155\" stroke=\"#0f172a\" stroke-width=\"1.8\"") +
        svgTag("circle", "cx=\"76\" cy=\"46\" r=\"5\" fill=\"#1e293b\"") +
        svgTag("circle", "cx=\"78\" cy=\"44\" r=\"2.2\" fill=\"#dc2626\"") +
        svgTag("line", "x1=\"81\" y1=\"47\" x2=\"104\" y2=\"54\" stroke=\"#0f172a\" stroke-width=\"2\" stroke-linecap=\"round\"") +
        wing;
    return wrapSvgCanvas(body, "", 120, 92);
}

std::string build3dRealisticFlySvg(int frame)
{
    std::string defs =
        svgTag("radialGradient", "id=\"flyAbd\" cx=\"40%\" cy=\"35%\" r=\"68%\"", false) +
        svgTag("stop", "offset=\"0%\" stop-color=\"#38bdf8\"") +
        svgTag("stop", "offset=\"48%\" stop-color=\"#1e3a8a\"") +
        svgTag("stop", "offset=\"100%\" stop-color=\"#090d16\"") + svgClose("radialGradient");

    std::string wings = (frame == 0)
        ? (svgTag("path", "d=\"M 56,52 C 30,22 10,34 22,52 C 32,64 46,60 56,54 Z\" fill=\"#e2e8f0\" fill-opacity=\"0.55\" stroke=\"#475569\" stroke-width=\"1.5\"") +
           svgTag("path", "d=\"M 52,50 L 24,38 M 50,53 L 26,48\" stroke=\"#334155\" stroke-width=\"1.0\" opacity=\"0.7\"") +
           svgTag("path", "d=\"M 62,52 C 88,22 108,34 96,52 C 86,64 72,60 62,54 Z\" fill=\"#e2e8f0\" fill-opacity=\"0.55\" stroke=\"#475569\" stroke-width=\"1.5\"") +
           svgTag("path", "d=\"M 66,50 L 94,38 M 68,53 L 92,48\" stroke=\"#334155\" stroke-width=\"1.0\" opacity=\"0.7\""))
        : (svgTag("path", "d=\"M 56,52 C 38,38 24,52 34,66 C 42,74 52,66 56,56 Z\" fill=\"#cbd5e1\" fill-opacity=\"0.48\" stroke=\"#475569\" stroke-width=\"1.4\"") +
           svgTag("path", "d=\"M 62,52 C 80,38 94,52 84,66 C 76,74 66,66 62,56 Z\" fill=\"#cbd5e1\" fill-opacity=\"0.48\" stroke=\"#475569\" stroke-width=\"1.4\""));

    std::string body =
        svgTag("path", "d=\"M 46,56 L 32,74 M 50,62 L 38,82 M 54,68 L 46,88 M 72,56 L 86,74 M 68,62 L 80,82 M 64,68 L 72,88\" stroke=\"#090d16\" stroke-width=\"2.2\" stroke-linecap=\"round\"") +
        svgTag("ellipse", "cx=\"59\" cy=\"66\" rx=\"14\" ry=\"18\" fill=\"url(#flyAbd)\" stroke=\"#090d16\" stroke-width=\"2\"") +
        svgTag("ellipse", "cx=\"59\" cy=\"50\" rx=\"11\" ry=\"10\" fill=\"#1e293b\" stroke=\"#090d16\" stroke-width=\"2\"") +
        wings +
        svgTag("ellipse", "cx=\"53\" cy=\"42\" rx=\"5.5\" ry=\"7\" fill=\"#991b1b\" stroke=\"#090d16\" stroke-width=\"1.5\"") +
        svgTag("ellipse", "cx=\"65\" cy=\"42\" rx=\"5.5\" ry=\"7\" fill=\"#991b1b\" stroke=\"#090d16\" stroke-width=\"1.5\"") +
        svgTag("circle", "cx=\"52\" cy=\"40\" r=\"1.6\" fill=\"#fca5a5\"") +
        svgTag("circle", "cx=\"64\" cy=\"40\" r=\"1.6\" fill=\"#fca5a5\"");
    return wrapSvgCanvas(body, defs, 120, 100);
}

std::string build3dFireflySvg(int frame)
{
    int wy = (frame == 0) ? 24 : 36;
    std::string body =
        svgTag("ellipse", "cx=\"52\" cy=\"" + std::to_string(wy) + "\" rx=\"16\" ry=\"7\" transform=\"rotate(-24 52 " + std::to_string(wy) + ")\" fill=\"#fef9c3\" opacity=\"0.78\"") +
        svgTag("ellipse", "cx=\"44\" cy=\"52\" rx=\"15\" ry=\"9\" fill=\"#3f6212\" stroke=\"#1a2e05\" stroke-width=\"2\"") +
        svgTag("ellipse", "cx=\"34\" cy=\"54\" rx=\"8\" ry=\"7\" fill=\"#bef264\"") +
        svgTag("circle", "cx=\"63\" cy=\"48\" r=\"7\" fill=\"#1e293b\"") +
        svgTag("path", "d=\"M 68,43 L 78,35 M 69,46 L 80,41\" stroke=\"#1e293b\" stroke-width=\"1.8\" stroke-linecap=\"round\"");
    return wrapSvgCanvas(body, "", 100, 80);
}

std::string build3dSpiderSvg(int frame)
{
    int leg = (frame == 0) ? -5 : 5;
    std::string body =
        svgTag("path", "d=\"M 46,56 Q 18," + std::to_string(32 + leg) + " 10,58 M 44,62 Q 14," + std::to_string(56 - leg) + " 8,82 M 46,68 Q 18," + std::to_string(78 + leg) + " 16,104 M 50,74 Q 28,94 28,114 M 74,56 Q 102," + std::to_string(32 - leg) + " 110,58 M 76,62 Q 106," + std::to_string(56 + leg) + " 112,82 M 74,68 Q 102," + std::to_string(78 - leg) + " 104,104 M 70,74 Q 92,94 92,114\" fill=\"none\" stroke=\"#090d16\" stroke-width=\"5\" stroke-linecap=\"round\"") +
        svgTag("ellipse", "cx=\"60\" cy=\"54\" rx=\"20\" ry=\"24\" fill=\"#0f172a\" stroke=\"#020617\" stroke-width=\"3\"") +
        svgTag("circle", "cx=\"60\" cy=\"82\" r=\"13\" fill=\"#1e293b\"") +
        svgTag("polygon", "points=\"54,48 66,48 60,56 66,64 54,64 60,56\" fill=\"#ef4444\"") +
        svgTag("circle", "cx=\"55\" cy=\"90\" r=\"2.8\" fill=\"#ef4444\"") +
        svgTag("circle", "cx=\"65\" cy=\"90\" r=\"2.8\" fill=\"#ef4444\"");
    return wrapSvgCanvas(body, "", 120, 120);
}

std::string build3dSnakeSvg(int frame)
{
    std::string defs =
        svgTag("linearGradient", "id=\"snk3d\" x1=\"0%\" y1=\"0%\" x2=\"100%\" y2=\"100%\"", false) +
        svgTag("stop", "offset=\"0%\" stop-color=\"#84cc16\"") +
        svgTag("stop", "offset=\"55%\" stop-color=\"#15803d\"") +
        svgTag("stop", "offset=\"100%\" stop-color=\"#052e16\"") + svgClose("linearGradient");

    int tongueLen = (frame == 0) ? 358 : 340;
    std::string body =
        svgTag("path", "d=\"M 80,0 C 40,65 130,115 70,175 C 25,220 115,255 82,295\" fill=\"none\" stroke=\"#052e16\" stroke-width=\"24\" stroke-linecap=\"round\"") +
        svgTag("path", "d=\"M 80,0 C 40,65 130,115 70,175 C 25,220 115,255 82,295\" fill=\"none\" stroke=\"url(#snk3d)\" stroke-width=\"18\" stroke-linecap=\"round\"") +
        svgTag("path", "d=\"M 80,0 C 40,65 130,115 70,175 C 25,220 115,255 82,295\" fill=\"none\" stroke=\"#bef264\" stroke-width=\"5\" stroke-dasharray=\"8 12\" stroke-linecap=\"round\" opacity=\"0.78\"") +
        svgTag("ellipse", "cx=\"80\" cy=\"302\" rx=\"18\" ry=\"24\" fill=\"url(#snk3d)\" stroke=\"#052e16\" stroke-width=\"3\"") +
        svgTag("circle", "cx=\"72\" cy=\"304\" r=\"4\" fill=\"#facc15\"") +
        svgTag("circle", "cx=\"88\" cy=\"304\" r=\"4\" fill=\"#facc15\"") +
        svgTag("path", "d=\"M 80,324 L 80," + std::to_string(tongueLen - 10) + " L 72," + std::to_string(tongueLen) + " M 80," + std::to_string(tongueLen - 10) + " L 88," + std::to_string(tongueLen) + "\" fill=\"none\" stroke=\"#ef4444\" stroke-width=\"3\" stroke-linecap=\"round\"");
    return wrapSvgCanvas(body, defs, 160, 370);
}

// ------------------------------------------------------------------------------
// 8. Build 3D-shaded maple autumn leaf & 6-pointed snowflake crystal
// ------------------------------------------------------------------------------
std::string build3dAutumnLeafSvg(void)
{
    std::string defs =
        svgTag("radialGradient", "id=\"lf3d\" cx=\"45%\" cy=\"40%\" r=\"65%\"", false) +
        svgTag("stop", "offset=\"0%\" stop-color=\"#fef08a\"") +
        svgTag("stop", "offset=\"55%\" stop-color=\"#ffffff\"") +
        svgTag("stop", "offset=\"100%\" stop-color=\"#cbd5e1\"") + svgClose("radialGradient");

    std::string body =
        svgTag("path", "d=\"M 45,10 L 56,32 L 80,26 L 68,48 L 84,62 L 54,66 L 48,84 L 42,84 L 36,66 L 6,62 L 22,48 L 10,26 L 34,32 Z\" fill=\"url(#lf3d)\" stroke=\"#64748b\" stroke-width=\"2\" stroke-linejoin=\"round\"") +
        svgTag("path", "d=\"M 45,22 L 45,86 M 45,48 L 22,36 M 45,48 L 68,36\" stroke=\"#64748b\" stroke-width=\"2\" stroke-linecap=\"round\"");
    return wrapSvgCanvas(body, defs, 90, 92);
}

std::string buildSnowflakeCrystalSvg(void)
{
    std::ostringstream ss;
    for (int k = 0; k < 6; ++k) {
        double a = k * 3.14159265 / 3.0;
        int x2 = 24 + static_cast<int>(std::round(std::cos(a) * 19.0));
        int y2 = 24 + static_cast<int>(std::round(std::sin(a) * 19.0));
        ss << svgTag("line", "x1=\"24\" y1=\"24\" x2=\"" + std::to_string(x2) + "\" y2=\"" + std::to_string(y2) + "\" stroke=\"#ffffff\" stroke-width=\"2.4\" stroke-linecap=\"round\"");
    }
    ss << svgTag("circle", "cx=\"24\" cy=\"24\" r=\"4.5\" fill=\"#e0f2fe\"");
    return wrapSvgCanvas(ss.str(), "", 48, 48);
}

// ------------------------------------------------------------------------------
// 9. Generate mathematically periodic 600x800 3D Perlin fog PNG (zero seams!)
// ------------------------------------------------------------------------------
void exportPeriodicPerlinFogPng(const std::string& relPathNoExt, uint32_t seed, float scaleX, float scaleY, uint8_t maxAlpha)
{
    fs::path outPng = fs::path("assets") / (relPathNoExt + ".png");
    if (outPng.has_parent_path()) fs::create_directories(outPng.parent_path());

    PerlinNoise pn(seed);
    constexpr int W = 600;
    constexpr int H = 800;
    cv::Mat bgra(H, W, CV_8UC4);

    for (int y = 0; y < H; ++y) {
        float ny = static_cast<float>(y) / static_cast<float>(H);
        float envY = std::sin(ny * 3.14159265f);
        cv::Vec4b* row = bgra.ptr<cv::Vec4b>(y);
        for (int x = 0; x < W; ++x) {
            float ang = (static_cast<float>(x) / static_cast<float>(W)) * 6.2831853f;
            float cx  = std::cos(ang) * scaleX;
            float cz  = std::sin(ang) * scaleX;
            float cy  = ny * scaleY;

            float f1 = pn.noise(cx, cy, cz);
            float f2 = pn.noise(cx * 2.1f + 3.7f, cy * 2.1f, cz * 2.1f - 1.9f) * 0.5f;
            float v  = std::max(0.0f, std::min(1.0f, (f1 + f2) * 0.55f + 0.45f));
            float w  = v * v * (3.0f - 2.0f * v) * envY;
            uint8_t a = static_cast<uint8_t>(std::round(w * static_cast<float>(maxAlpha)));
            row[x] = cv::Vec4b(245, 240, 235, a);
        }
    }
    cv::imwrite(outPng.string(), bgra);
}

} // namespace

// ==============================================================================
// 10. MAIN GENERATOR: Build 3D Animated FX Bank & Archive packs/pack_fx.zip
// ==============================================================================
int main(int argc, char* argv[])
{
    bool installFxOnly = false;
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--install-fx") == 0) {
            installFxOnly = true;
        }
    }

    if (!installFxOnly) {
        std::cout << "=== Generating MakingFaces Pack FX (Full Standalone Pack) ===\n";
        ensureBaseIni("makingfaces.ini");
        generateDefaultBrowsBank();
    } else {
        std::cout << "=== Installing 3D FX Sprites into active assets/fx/ (preserving characters) ===\n";
        fs::create_directories("assets/fx");
    }

    generateDefaultFxBank();

    exportSvgToPng("fx/bat",           build3dBatSvg(0), 150, 120, 0.0f);
    exportSvgToPng("fx/bat_1",         build3dBatSvg(0), 150, 120, 0.0f);
    exportSvgToPng("fx/bat_2",         build3dBatSvg(1), 150, 120, 0.0f);
    exportSvgToPng("fx/bat_3",         build3dBatSvg(2), 150, 120, 0.0f);

    exportSvgToPng("fx/butterfly",     build3dButterflySvg(0), 140, 120, 0.0f);
    exportSvgToPng("fx/butterfly_1",   build3dButterflySvg(0), 140, 120, 0.0f);
    exportSvgToPng("fx/butterfly_2",   build3dButterflySvg(1), 140, 120, 0.0f);

    exportSvgToPng("fx/bird",          build3dBirdSvg(0), 140, 110, 0.0f);
    exportSvgToPng("fx/bird_1",        build3dBirdSvg(0), 140, 110, 0.0f);
    exportSvgToPng("fx/bird_2",        build3dBirdSvg(1), 140, 110, 0.0f);

    exportSvgToPng("fx/wasp",          build3dWaspSvg(0), 120, 96, 0.0f);
    exportSvgToPng("fx/wasp_1",        build3dWaspSvg(0), 120, 96, 0.0f);
    exportSvgToPng("fx/wasp_2",        build3dWaspSvg(1), 120, 96, 0.0f);

    exportSvgToPng("fx/mosquito",      build3dMosquitoSvg(0), 120, 92, 0.0f);
    exportSvgToPng("fx/mosquito_1",    build3dMosquitoSvg(0), 120, 92, 0.0f);
    exportSvgToPng("fx/mosquito_2",    build3dMosquitoSvg(1), 120, 92, 0.0f);

    exportSvgToPng("fx/fly",           build3dRealisticFlySvg(0), 120, 100, 0.0f);
    exportSvgToPng("fx/fly_1",         build3dRealisticFlySvg(0), 120, 100, 0.0f);
    exportSvgToPng("fx/fly_2",         build3dRealisticFlySvg(1), 120, 100, 0.0f);

    exportSvgToPng("fx/firefly",       build3dFireflySvg(0), 100, 80, 0.0f);
    exportSvgToPng("fx/firefly_1",     build3dFireflySvg(0), 100, 80, 0.0f);
    exportSvgToPng("fx/firefly_2",     build3dFireflySvg(1), 100, 80, 0.0f);

    exportSvgToPng("fx/spider",        build3dSpiderSvg(0), 120, 120, 0.0f);
    exportSvgToPng("fx/spider_1",      build3dSpiderSvg(0), 120, 120, 0.0f);
    exportSvgToPng("fx/spider_2",      build3dSpiderSvg(1), 120, 120, 0.0f);

    exportSvgToPng("fx/snake",         build3dSnakeSvg(0), 160, 370, 0.0f);
    exportSvgToPng("fx/snake_1",       build3dSnakeSvg(0), 160, 370, 0.0f);
    exportSvgToPng("fx/snake_2",       build3dSnakeSvg(1), 160, 370, 0.0f);

    exportSvgToPng("fx/mouse",         build3dMouseWithFeetSvg(0), 160, 90, 0.0f);
    exportSvgToPng("fx/mouse_1",       build3dMouseWithFeetSvg(0), 160, 90, 0.0f);
    exportSvgToPng("fx/mouse_2",       build3dMouseWithFeetSvg(1), 160, 90, 0.0f);

    exportSvgToPng("fx/grasshopper",   build3dGrasshopperSvg(0), 156, 92, 0.0f);
    exportSvgToPng("fx/grasshopper_1", build3dGrasshopperSvg(0), 156, 92, 0.0f);
    exportSvgToPng("fx/grasshopper_2", build3dGrasshopperSvg(1), 156, 92, 0.0f);

    exportSvgToPng("fx/ant",           build3dAntSvg(0), 120, 76, 0.0f);
    exportSvgToPng("fx/ant_1",         build3dAntSvg(0), 120, 76, 0.0f);
    exportSvgToPng("fx/ant_2",         build3dAntSvg(1), 120, 76, 0.0f);

    exportSvgToPng("fx/leaf",          build3dAutumnLeafSvg(), 90, 92, 0.0f);
    exportSvgToPng("fx/snowflake",     buildSnowflakeCrystalSvg(), 48, 48, 0.0f);

    exportPeriodicPerlinFogPng("fx/fog_1", 2026, 2.2f, 4.5f, 185);
    exportPeriodicPerlinFogPng("fx/fog_2", 7077, 1.4f, 3.0f, 165);

    if (!installFxOnly) {
        cleanSvgAndSaveZip("pack_fx", "packs");
        std::cout << "[OK] 100% C++17 Pack FX built and archived to packs/pack_fx.zip!\n";
    } else {
        if (fs::exists("packs/default.zip")) {
            std::system("zip -rq \"packs/default.zip\" assets/fx");
        }
        std::cout << "[OK] Installed 3D FX into active assets/fx/ and packs/default.zip!\n";
    }
    return 0;
}
