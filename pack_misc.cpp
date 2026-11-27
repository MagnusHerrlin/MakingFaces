// ==============================================================================
// MakingFaces - Default 241-Memory 3D Asset & Character Pack Generator (C++17)
// Architecture & Design by Magnus Herrlin 2026
// ==============================================================================

#include "pack_utils.h"
#include <array>
#include <cmath>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <random>
#include <sstream>
#include <string>
#include <vector>

namespace fs = std::filesystem;

namespace {

struct SkinPal { const char* hi; const char* mid; const char* deep; const char* stroke; };
struct HairPal { const char* deep; const char* dark; const char* mid; const char* hi; };

static constexpr std::array<SkinPal, 8> kSkinPals = {{
    {"#fff0e6", "#f5c8ad", "#c48866", "#6e4229"},
    {"#ffe4d1", "#eab08c", "#b5724d", "#63361e"},
    {"#f7d6be", "#dfa37c", "#a36540", "#542e18"},
    {"#f2c49b", "#c98857", "#854c25", "#42210b"},
    {"#d19264", "#9c5b30", "#5e3012", "#2b1304"},
    {"#fff2dc", "#ebd0a8", "#bd9664", "#6b4d2a"},
    {"#9e633e", "#693b1f", "#381b0a", "#170902"},
    {"#754326", "#4a2612", "#240f04", "#0d0401"}
}};

static constexpr std::array<HairPal, 8> kHairPals = {{
    {"#050507", "#18181f", "#32323d", "#565666"},
    {"#1f1008", "#4d2c18", "#7d4b2d", "#a86d48"},
    {"#3b2212", "#6e4226", "#a16843", "#cc8f66"},
    {"#664900", "#ad8000", "#e6b419", "#ffe875"},
    {"#3d0902", "#7d1b0a", "#bd361c", "#eb6546"},
    {"#27272a", "#52525b", "#94a3b8", "#e2e8f0"},
    {"#064e3b", "#047857", "#10b981", "#6ee7b7"},
    {"#3b0764", "#6b21a8", "#a855f7", "#e9d5ff"}
}};

static std::string wrapFace3d(int idx, const std::string& pathD, const char* c1, const char* c2, const char* c3, const char* st, const std::string& extra = "")
{
    std::string sid = std::to_string(idx);
    std::string defs =
        svgTag("radialGradient", "id=\"df" + sid + "\" cx=\"46%\" cy=\"38%\" r=\"65%\"", false) +
        svgTag("stop", std::string("offset=\"0%\" stop-color=\"") + c1 + "\"") +
        svgTag("stop", std::string("offset=\"52%\" stop-color=\"") + c2 + "\"") +
        svgTag("stop", std::string("offset=\"86%\" stop-color=\"") + c3 + "\"") +
        svgTag("stop", std::string("offset=\"100%\" stop-color=\"") + st + "\"") + svgClose("radialGradient") +
        svgTag("radialGradient", "id=\"dhi" + sid + "\" cx=\"50%\" cy=\"24%\" r=\"36%\"", false) +
        svgTag("stop", "offset=\"0%\" stop-color=\"#ffffff\" stop-opacity=\"0.28\"") +
        svgTag("stop", "offset=\"100%\" stop-color=\"#ffffff\" stop-opacity=\"0\"") + svgClose("radialGradient") +
        svgTag("mask", "id=\"dm" + sid + "\"", false) + getStandardEyeCutoutMask() + svgClose("mask");

    std::string rims =
        svgTag("path", "d=\"M 124,345 C 145,303 230,303 256,347 C 262,379 233,407 190,405 C 145,403 116,379 124,345 Z\" fill=\"none\" stroke=\"" + std::string(st) + "\" stroke-width=\"5\"") +
        svgTag("path", "d=\"M 476,345 C 455,303 370,303 344,347 C 338,379 367,407 410,405 C 455,403 484,379 476,345 Z\" fill=\"none\" stroke=\"" + std::string(st) + "\" stroke-width=\"5\"");

    std::string body =
        svgTag("g", "mask=\"url(#dm" + sid + ")\"", false) +
        svgTag("path", "d=\"" + pathD + "\" fill=\"url(#df" + sid + ")\" stroke=\"" + std::string(st) + "\" stroke-width=\"6\" stroke-linejoin=\"round\"") +
        svgTag("path", "d=\"" + pathD + "\" fill=\"url(#dhi" + sid + ")\"") +
        svgTag("path", "d=\"M 190,702 Q 300,758 410,702 Q 300,730 190,702 Z\" fill=\"#000000\" opacity=\"0.22\"") +
        extra + rims + svgClose("g");
    return wrapSvgCanvas(body, defs);
}

static void genAllFaces(void)
{
    static constexpr std::array<const char*, 8> kShapes = {
        "M 300,205 C 445,202 510,255 515,380 C 520,510 488,655 425,725 C 385,762 338,770 300,770 C 262,770 215,762 175,725 C 112,655 80,510 85,380 C 90,255 155,202 300,205 Z",
        "M 300,185 C 415,185 465,250 462,395 C 458,550 412,705 300,775 C 188,705 142,550 138,395 C 135,250 185,185 300,185 Z",
        "M 300,238 C 365,182 512,198 518,348 C 524,485 432,642 300,770 C 168,642 76,485 82,348 C 88,198 235,182 300,238 Z",
        "M 300,205 C 465,205 535,275 530,395 C 525,525 485,660 415,720 C 370,758 335,768 300,768 C 265,768 230,758 185,720 C 115,660 75,525 70,395 C 65,275 135,205 300,205 Z",
        "M 102,215 Q 300,188 498,215 L 490,385 L 300,775 L 110,385 Z",
        "M 300,205 C 485,205 555,275 550,395 C 545,525 495,665 415,725 C 370,760 335,768 300,768 C 265,768 230,760 185,725 C 105,665 55,525 50,395 C 45,275 115,205 300,205 Z",
        "M 135,185 L 465,185 L 485,375 L 455,735 L 300,768 L 145,735 L 115,375 Z",
        "M 300,175 C 475,175 545,260 520,395 C 492,535 375,725 300,778 C 225,725 108,535 80,395 C 55,260 125,175 300,175 Z"
    };
    static constexpr std::array<SkinPal, 8> kCreaturePals = {{
        {"#ffbe6b", "#e08524", "#7a3b04", "#2b1300"},
        {"#b5ff85", "#47b825", "#165207", "#082402"},
        {"#fff59d", "#ffd600", "#f57f17", "#6e3800"},
        {"#f7f4f0", "#e2dcd5", "#a3998e", "#423a32"},
        {"#ffb74d", "#f57c00", "#8c2d00", "#330e00"},
        {"#eceff1", "#b0bec5", "#546e7a", "#1c262b"},
        {"#784b2a", "#4a2c18", "#1f1008", "#0a0401"},
        {"#c084fc", "#7e22ce", "#3b0764", "#140024"}
    }};

    for (int idx = 1; idx <= 128; ++idx) {
        const auto& pal = (idx <= 32) ? kSkinPals.at((idx - 1) % 8) : kCreaturePals.at((idx - 1) % 8);
        const char* shp = kShapes.at((idx - 1) % 8);
        std::string extra;
        if (idx == 12 || idx == 28) extra = svgTag("circle", "cx=\"300\" cy=\"258\" r=\"8\" fill=\"#d90429\"");
        if (idx == 49 || idx == 50) extra = svgTag("circle", "cx=\"190\" cy=\"348\" r=\"102\" fill=\"none\" stroke=\"#faedcd\" stroke-width=\"13\"") + svgTag("circle", "cx=\"410\" cy=\"348\" r=\"102\" fill=\"none\" stroke=\"#faedcd\" stroke-width=\"13\"");
        if (idx >= 81 && idx <= 88) extra = svgTag("path", "d=\"M 225,210 C 175,350 175,595 225,755 M 375,210 C 425,350 425,595 375,755 M 300,205 L 300,768\" fill=\"none\" stroke=\"#421200\" stroke-width=\"5\" opacity=\"0.48\"");
        if (idx >= 117 && idx <= 120) extra = svgTag("rect", "x=\"78\" y=\"485\" width=\"34\" height=\"28\" rx=\"5\" fill=\"#94a3b8\" stroke=\"#0f172a\" stroke-width=\"3\"") + svgTag("rect", "x=\"488\" y=\"485\" width=\"34\" height=\"28\" rx=\"5\" fill=\"#94a3b8\" stroke=\"#0f172a\" stroke-width=\"3\"");
        saveSvgAssetAt("face", idx, wrapFace3d(idx, shp, pal.hi, pal.mid, pal.deep, pal.stroke, extra));
    }
}

static void genAllEars(void)
{
    for (int idx = 1; idx <= 88; ++idx) {
        const auto& pal = kSkinPals.at((idx - 1) % 8);
        std::string sid = std::to_string(idx);
        std::string defs =
            svgTag("radialGradient", "id=\"deg" + sid + "\" cx=\"45%\" cy=\"38%\" r=\"68%\"", false) +
            svgTag("stop", std::string("offset=\"0%\" stop-color=\"") + pal.hi + "\"") +
            svgTag("stop", std::string("offset=\"100%\" stop-color=\"") + pal.deep + "\"") + svgClose("radialGradient");

        std::string b;
        int kind = (idx - 1) % 6;
        if (kind == 0 || kind == 1) {
            b = svgTag("path", "d=\"M 96,310 C 22,285 22,510 96,528 Z\" fill=\"url(#deg" + sid + ")\" stroke=\"" + std::string(pal.stroke) + "\" stroke-width=\"5\"") +
                svgTag("path", "d=\"M 504,310 C 578,285 578,510 504,528 Z\" fill=\"url(#deg" + sid + ")\" stroke=\"" + std::string(pal.stroke) + "\" stroke-width=\"5\"");
            if (kind == 1) {
                b += svgTag("circle", "cx=\"70\" cy=\"502\" r=\"22\" fill=\"none\" stroke=\"#ffd700\" stroke-width=\"7\"") +
                     svgTag("circle", "cx=\"530\" cy=\"502\" r=\"22\" fill=\"none\" stroke=\"#ffd700\" stroke-width=\"7\"");
            }
        } else if (kind == 2) {
            b = svgTag("path", "d=\"M 110,275 L 64,105 L 215,215 Z\" fill=\"url(#deg" + sid + ")\" stroke=\"" + std::string(pal.stroke) + "\" stroke-width=\"5\"") +
                svgTag("path", "d=\"M 490,275 L 536,105 L 385,215 Z\" fill=\"url(#deg" + sid + ")\" stroke=\"" + std::string(pal.stroke) + "\" stroke-width=\"5\"");
        } else if (kind == 3) {
            b = svgTag("circle", "cx=\"115\" cy=\"222\" r=\"66\" fill=\"url(#deg" + sid + ")\" stroke=\"" + std::string(pal.stroke) + "\" stroke-width=\"5\"") +
                svgTag("circle", "cx=\"485\" cy=\"222\" r=\"66\" fill=\"url(#deg" + sid + ")\" stroke=\"" + std::string(pal.stroke) + "\" stroke-width=\"5\"");
        } else if (kind == 4) {
            b = svgTag("path", "d=\"M 98,320 C 8,185 16,335 90,495 Z\" fill=\"url(#deg" + sid + ")\" stroke=\"" + std::string(pal.stroke) + "\" stroke-width=\"5\"") +
                svgTag("path", "d=\"M 502,320 C 592,185 584,335 510,495 Z\" fill=\"url(#deg" + sid + ")\" stroke=\"" + std::string(pal.stroke) + "\" stroke-width=\"5\"");
        } else {
            b = svgTag("rect", "x=\"44\" y=\"330\" width=\"38\" height=\"120\" rx=\"10\" fill=\"#64748b\" stroke=\"#0f172a\" stroke-width=\"4\"") +
                svgTag("line", "x1=\"63\" y1=\"330\" x2=\"63\" y2=\"230\" stroke=\"#94a3b8\" stroke-width=\"7\"") +
                svgTag("circle", "cx=\"63\" cy=\"222\" r=\"14\" fill=\"#ef4444\"") +
                svgTag("rect", "x=\"518\" y=\"330\" width=\"38\" height=\"120\" rx=\"10\" fill=\"#64748b\" stroke=\"#0f172a\" stroke-width=\"4\"") +
                svgTag("line", "x1=\"537\" y1=\"330\" x2=\"537\" y2=\"230\" stroke=\"#94a3b8\" stroke-width=\"7\"") +
                svgTag("circle", "cx=\"537\" cy=\"222\" r=\"14\" fill=\"#ef4444\"");
        }
        saveSvgAssetAt("ears", idx, wrapSvgCanvas(b, defs));
    }
}
static void genAllNoses(void)
{
    for (int idx = 1; idx <= 95; ++idx) {
        const auto& pal = kSkinPals.at((idx - 1) % 8);
        std::string sid = std::to_string(idx);
        std::string defs =
            svgTag("radialGradient", "id=\"dnt" + sid + "\" cx=\"42%\" cy=\"35%\" r=\"58%\"", false) +
            svgTag("stop", std::string("offset=\"0%\" stop-color=\"") + pal.hi + "\" stop-opacity=\"0.98\"") +
            svgTag("stop", std::string("offset=\"55%\" stop-color=\"") + pal.mid + "\" stop-opacity=\"0.78\"") +
            svgTag("stop", std::string("offset=\"100%\" stop-color=\"") + pal.deep + "\" stop-opacity=\"0\"") + svgClose("radialGradient");

        std::string b;
        if (idx >= 41 && idx <= 43) {
            b = svgTag("ellipse", "cx=\"300\" cy=\"480\" rx=\"74\" ry=\"50\" fill=\"#ff859b\" stroke=\"#a6384e\" stroke-width=\"4\"") +
                svgTag("ellipse", "cx=\"272\" cy=\"482\" rx=\"13\" ry=\"22\" fill=\"#24050c\"") +
                svgTag("ellipse", "cx=\"328\" cy=\"482\" rx=\"13\" ry=\"22\" fill=\"#24050c\"");
        } else if (idx >= 56 && idx <= 58) {
            b = svgTag("path", "d=\"M 252,412 Q 300,382 348,412 L 364,492 Q 300,564 236,492 Z\" fill=\"#ffb300\" stroke=\"#6e4200\" stroke-width=\"4.5\"");
        } else if (idx >= 59 && idx <= 61) {
            b = svgTag("path", "d=\"M 282,368 Q 305,365 322,374 L 346,472 Q 354,518 308,528 Q 268,532 264,496 Q 272,448 282,368 Z\" fill=\"" + std::string(pal.mid) + "\" stroke=\"" + std::string(pal.deep) + "\" stroke-width=\"4\"") +
                svgTag("circle", "cx=\"332\" cy=\"466\" r=\"8\" fill=\"" + std::string(pal.deep) + "\"");
        } else if (idx >= 62 && idx <= 64) {
            b = svgTag("circle", "cx=\"300\" cy=\"474\" r=\"42\" fill=\"#e53935\" stroke=\"#5c0000\" stroke-width=\"3.5\"") +
                svgTag("ellipse", "cx=\"284\" cy=\"456\" rx=\"13\" ry=\"7\" transform=\"rotate(-24 284 456)\" fill=\"#ffffff\" opacity=\"0.65\"");
        } else if (idx == 65 || idx == 66) {
            b = svgTag("path", "d=\"M 300,425 L 332,504 Q 312,516 300,496 Q 288,516 268,504 Z\" fill=\"#12100e\" stroke=\"#3e2723\" stroke-width=\"4\"");
        } else if (idx >= 84 && idx <= 89) {
            b = svgTag("path", "d=\"M 272,487 Q 290,355 302,340 Q 318,395 328,487 Z\" fill=\"#f8fafc\" stroke=\"#334155\" stroke-width=\"4\"");
        } else if (idx >= 90 && idx <= 93) {
            b = svgTag("polygon", "points=\"300,72 276,225 324,225\" fill=\"#fde047\" stroke=\"#854d0e\" stroke-width=\"3\"") +
                svgTag("line", "x1=\"282\" y1=\"190\" x2=\"318\" y2=\"175\" stroke=\"#f472b6\" stroke-width=\"6\"") +
                svgTag("line", "x1=\"288\" y1=\"145\" x2=\"312\" y2=\"132\" stroke=\"#38bdf8\" stroke-width=\"5\"");
        } else {
            int tipRx = (idx % 5 == 1) ? 48 : ((idx % 5 == 2) ? 28 : 34);
            b = svgTag("ellipse", "cx=\"300\" cy=\"504\" rx=\"" + std::to_string(tipRx + 8) + "\" ry=\"13\" fill=\"#000000\" opacity=\"0.22\"") +
                svgTag("ellipse", "cx=\"300\" cy=\"412\" rx=\"22\" ry=\"90\" fill=\"url(#dnt" + sid + ")\"") +
                svgTag("ellipse", "cx=\"300\" cy=\"486\" rx=\"" + std::to_string(tipRx) + "\" ry=\"26\" fill=\"url(#dnt" + sid + ")\"") +
                svgTag("ellipse", "cx=\"280\" cy=\"506\" rx=\"10\" ry=\"5.5\" fill=\"#0a0402\" opacity=\"0.86\"") +
                svgTag("ellipse", "cx=\"320\" cy=\"506\" rx=\"10\" ry=\"5.5\" fill=\"#0a0402\" opacity=\"0.86\"");
        }
        saveSvgAssetAt("nose", idx, wrapSvgCanvas(b, defs));
    }
}

static void genAllBrows(void)
{
    for (int idx = 1; idx <= 80; ++idx) {
        int style = (idx - 1) / 8;
        const auto& pal = kHairPals.at((idx - 1) % 8);
        std::mt19937 rng(7000 + idx * 19);
        std::uniform_real_distribution<float> u01(0.0f, 1.0f);
        std::normal_distribution<float> gauss(0.0f, 0.34f);
        std::ostringstream ss;

        int cnt = (style == 1) ? 60 : ((style == 3 || style == 7) ? 185 : 125);
        for (int side : {-1, 1}) {
            for (int k = 0; k < cnt; ++k) {
                float u = u01(rng);
                float v = std::max(-1.0f, std::min(1.0f, gauss(rng)));
                float xb = (style == 7) ? (302.0f - u * 178.0f) : (268.0f - u * 136.0f);
                float ay = (style == 4) ? (300.0f - u * 40.0f) : ((style == 5) ? (294.0f - u * 44.0f) : (285.0f - std::sin(u * 3.14159f * 0.85f) * 24.0f + u * u * 12.0f));
                float hh = (style == 1) ? 2.5f : ((style == 2) ? 4.8f : ((style == 3 || style == 7) ? 16.0f : 11.0f));
                float len = (style == 1) ? 10.5f : ((style == 3) ? 19.0f : 15.0f);
                float rad = (-76.0f + u * 108.0f + v * 12.0f) * 3.14159f / 180.0f;
                float dx = -std::cos(rad) * len;
                float dy = std::sin(rad) * len;
                float x0 = (side == 1) ? (600.0f - xb) : xb;
                float x1 = (side == 1) ? (600.0f - (xb + dx)) : (xb + dx);
                float y0 = ay + v * hh;
                float y1 = y0 + dy;
                const char* col = (std::fabs(v) > 0.55f) ? pal.deep : ((v < 0.0f) ? pal.dark : ((u01(rng) < 0.72f) ? pal.mid : pal.hi));
                std::string buf(160, '\0');
                int n = std::snprintf(buf.data(), buf.size(), "d=\"M %.1f,%.1f Q %.1f,%.1f %.1f,%.1f\" fill=\"none\" stroke=\"%s\" stroke-width=\"1.05\" stroke-linecap=\"round\" opacity=\"0.80\"",
                                      x0, y0, (x0 + x1) * 0.5f, (y0 + y1) * 0.5f - 2.0f, x1, y1, col);
                if (n > 0) { buf.resize(static_cast<size_t>(n)); ss << svgTag("path", buf); }
            }
        }
        saveSvgAssetAt("brows", idx, wrapSvgCanvas(ss.str()));
    }
}
static void genAllBeards(void)
{
    for (int idx = 1; idx <= 96; ++idx) {
        int style = (idx - 1) / 8;
        const auto& pal = kHairPals.at((idx - 1) % 8);
        std::mt19937 rng(8000 + idx * 23);
        std::uniform_real_distribution<float> u01(0.0f, 1.0f);
        std::normal_distribution<float> gauss(0.0f, 0.36f);
        std::ostringstream ss;

        int cnt = (style == 7) ? 950 : ((style == 8 || style == 9 || style == 11) ? 260 : 520);
        for (int k = 0; k < cnt; ++k) {
            float side = (u01(rng) < 0.5f) ? -1.0f : 1.0f;
            float u = u01(rng);
            float v = std::max(-1.0f, std::min(1.0f, gauss(rng)));
            float x0 = 300.0f + side * (u * 126.0f);
            float y0 = 548.0f + u * 22.0f + v * 12.0f;
            float ang = 68.0f - u * 22.0f;
            float hlen = 15.0f;

            if (style == 0) {
                y0 = 548.0f + std::sin(u * 3.14159f * 0.72f) * 18.0f - std::pow(u, 3.2f) * 28.0f + v * 10.0f;
                ang = 18.0f - std::pow(u, 2.4f) * 95.0f;
            } else if (style == 1) {
                y0 = 552.0f - std::pow(u, 2.6f) * 68.0f + v * 5.0f;
                ang = -12.0f - std::pow(u, 1.8f) * 78.0f;
            } else if (style == 2 || style == 4) {
                if (u >= 0.5f) {
                    float t = (u - 0.5f) / 0.5f;
                    x0 = 300.0f + side * (78.0f + t * 10.0f + v * 9.0f);
                    y0 = 565.0f + t * ((style == 2) ? 140.0f : 86.0f);
                    ang = 88.0f;
                }
            } else if (style == 5) {
                if (u01(rng) >= 0.48f) {
                    x0 = 300.0f + (u01(rng) * 2.0f - 1.0f) * 44.0f;
                    y0 = 668.0f + u01(rng) * 56.0f;
                    ang = 90.0f;
                    side = 1.0f;
                }
            } else if (style == 6) {
                x0 = 300.0f + side * (135.0f + u * 45.0f);
                y0 = 435.0f + v * 195.0f;
                ang = 90.0f - side * 14.0f;
            } else if (style == 7) {
                if (u01(rng) >= 0.32f) {
                    float ja = (u01(rng) * 1.76f - 0.88f) * 1.57079f;
                    x0 = 300.0f + std::sin(ja) * (162.0f + u01(rng) * 28.0f);
                    y0 = 565.0f + std::cos(ja) * (148.0f + u01(rng) * 36.0f);
                    ang = 90.0f;
                    hlen = 20.0f;
                    side = 1.0f;
                }
            } else if (style == 8) {
                x0 = 300.0f + side * (u * 26.0f);
                y0 = 546.0f + v * 11.0f;
                ang = 90.0f;
                hlen = 13.0f;
            } else if (style == 10) {
                x0 = 300.0f + side * (u * 138.0f);
                y0 = 544.0f + std::pow(u, 1.4f) * 32.0f + v * 15.0f;
                hlen = 24.0f;
            } else if (style == 11) {
                x0 = 300.0f + side * (u * 18.0f);
                y0 = 674.0f + u01(rng) * 26.0f;
                ang = 90.0f;
            }

            float rad = ang * 3.14159f / 180.0f;
            float x1 = x0 + side * std::cos(rad) * hlen;
            float y1 = y0 + std::sin(rad) * hlen;
            const char* col = (std::fabs(v) > 0.55f) ? pal.deep : ((v < 0.0f) ? pal.dark : ((u01(rng) < 0.72f) ? pal.mid : pal.hi));
            std::string buf(160, '\0');
            int n = std::snprintf(buf.data(), buf.size(), "d=\"M %.1f,%.1f Q %.1f,%.1f %.1f,%.1f\" fill=\"none\" stroke=\"%s\" stroke-width=\"1.10\" stroke-linecap=\"round\" opacity=\"0.80\"",
                                  x0, y0, (x0 + x1) * 0.5f, (y0 + y1) * 0.5f, x1, y1, col);
            if (n > 0) { buf.resize(static_cast<size_t>(n)); ss << svgTag("path", buf); }
        }
        saveSvgAssetAt("beard", idx, wrapSvgCanvas(ss.str()));
    }

    saveSvgAssetAt("beard", 97, wrapSvgCanvas(svgTag("path", "d=\"M 186,612 L 204,698 Q 300,746 396,698 L 414,612 L 384,612 L 376,655 Q 300,672 224,655 L 216,612 Z\" fill=\"#e2dcd0\" stroke=\"#2b2720\" stroke-width=\"5\"")));
    saveSvgAssetAt("beard", 98, wrapSvgCanvas(svgTag("path", "d=\"M 182,615 L 210,715 L 390,715 L 418,615 L 382,615 L 368,668 L 232,668 L 218,615 Z\" fill=\"#7c8796\" stroke=\"#14171c\" stroke-width=\"5\"")));
    saveSvgAssetAt("beard", 99, wrapSvgCanvas(svgTag("path", "d=\"M 176,612 L 208,712 L 300,736 L 392,712 L 424,612 L 386,612 L 362,662 L 300,668 L 238,662 L 214,612 Z\" fill=\"#ea580c\" stroke=\"#2b0d00\" stroke-width=\"5\"")));
    saveSvgAssetAt("beard", 100, wrapSvgCanvas(svgTag("path", "d=\"M 184,612 L 204,706 Q 300,748 396,706 L 416,612 L 382,612 L 372,660 Q 300,678 228,660 L 218,612 Z\" fill=\"#b45309\" stroke=\"#1f0e03\" stroke-width=\"5\"")));

    for (int i = 0; i < 4; ++i) {
        const auto& pal = kHairPals.at(i + 1);
        std::string b =
            svgTag("path", std::string("d=\"M 145,515 Q 300,565 455,515 L 440,685 L 365,785 L 330,725 L 300,792 L 270,725 L 235,785 L 160,685 Z\" fill=\"") + pal.mid + "\" stroke=\"" + pal.deep + "\" stroke-width=\"5\"") +
            svgTag("rect", "x=\"224\" y=\"728\" width=\"24\" height=\"16\" rx=\"4\" fill=\"#fbbf24\" stroke=\"#78350f\" stroke-width=\"3\"") +
            svgTag("rect", "x=\"352\" y=\"728\" width=\"24\" height=\"16\" rx=\"4\" fill=\"#fbbf24\" stroke=\"#78350f\" stroke-width=\"3\"") +
            svgTag("rect", "x=\"288\" y=\"742\" width=\"24\" height=\"16\" rx=\"4\" fill=\"#fbbf24\" stroke=\"#78350f\" stroke-width=\"3\"");
        saveSvgAssetAt("beard", 101 + i, wrapSvgCanvas(b));
    }
    for (int i = 0; i < 4; ++i) {
        const auto& pal = kHairPals.at(i);
        std::string b = svgTag("path", std::string("d=\"M 155,555 L 185,695 L 235,745 L 265,710 L 300,758 L 335,710 L 365,745 L 415,695 L 445,555 L 405,635 L 300,665 L 195,635 Z\" fill=\"") + pal.dark + "\" stroke=\"" + pal.deep + "\" stroke-width=\"5\"");
        saveSvgAssetAt("beard", 105 + i, wrapSvgCanvas(b));
    }
}

static void genAllHair(void)
{
    for (int idx = 1; idx <= 96; ++idx) {
        int style = (idx - 1) / 8;
        const auto& pal = kHairPals.at((idx - 1) % 8);
        std::mt19937 rng(9000 + idx * 29);
        std::uniform_real_distribution<float> u01(0.0f, 1.0f);
        std::ostringstream ss;

        if (style != 5 && style != 7 && style != 10) {
            int botY = (style == 3 || style == 9) ? 660 : ((style == 2 || style == 4) ? 510 : 435);
            ss << svgTag("path", "d=\"M 74," + std::to_string(botY) + " C 74,245 125,160 300,160 C 475,160 526,245 526," + std::to_string(botY) +
                                  " L 492," + std::to_string(botY - 15) + " C 495,285 455,224 300,224 C 145,224 105,285 108," + std::to_string(botY - 15) + " Z\" fill=\"" + std::string(pal.deep) + "\" opacity=\"0.90\"");
        }
        for (int k = 0; k < 1200; ++k) {
            float th = -3.52f + u01(rng) * 3.90f;
            float v = u01(rng);
            float x0 = 300.0f + std::cos(th) * (192.0f + v * 38.0f);
            float y0 = 318.0f + std::sin(th) * (148.0f + v * 34.0f);
            if (style == 5) {
                float un = (u01(rng) - 0.5f) * 0.5f;
                x0 = 300.0f + un * 170.0f;
                y0 = 120.0f + v * 85.0f;
            } else if (std::fabs(x0 - 300.0f) < 165.0f && y0 > 224.0f) {
                y0 = 175.0f + v * 46.0f;
            }
            float hlen = (style == 3 || style == 9) ? 58.0f : ((style == 5 || style == 6) ? 68.0f : 36.0f);
            float rad = ((style == 5 || style == 6) ? -1.57f : 1.57f) + (u01(rng) - 0.5f) * 0.35f;
            float x1 = x0 + std::cos(rad) * hlen;
            float y1 = y0 + std::sin(rad) * hlen;
            const char* col = (v < 0.35f) ? pal.dark : ((v < 0.80f) ? pal.mid : pal.hi);
            std::string buf(160, '\0');
            int n = std::snprintf(buf.data(), buf.size(), "d=\"M %.1f,%.1f Q %.1f,%.1f %.1f,%.1f\" fill=\"none\" stroke=\"%s\" stroke-width=\"1.4\" stroke-linecap=\"round\" opacity=\"0.80\"",
                                  x0, y0, (x0 + x1) * 0.5f + 4.0f, (y0 + y1) * 0.5f, x1, y1, col);
            if (n > 0) { buf.resize(static_cast<size_t>(n)); ss << svgTag("path", buf); }
        }
        saveSvgAssetAt("hair", idx, wrapSvgCanvas(ss.str()));
    }
}
static void genAllGlassesHatsMakeupAndBg(void)
{
    static constexpr std::array<std::array<const char*, 5>, 8> kPal = {{
        {"#525866", "#23262e", "#08090c", "#ffffff", "#80d8ff"},
        {"#fff099", "#d4af37", "#5c4305", "#fffde7", "#ffb300"},
        {"#ff8a80", "#c62828", "#4a0000", "#ffcdd2", "#ff1744"},
        {"#82b1ff", "#1565c0", "#062147", "#e3f2fd", "#00b0ff"},
        {"#b9f6ca", "#2e7d32", "#0a2e0d", "#e8f5e9", "#00e676"},
        {"#ea80fc", "#7b1fa2", "#28053b", "#f3e5f5", "#d500f9"},
        {"#ffcc80", "#8d5524", "#2b1606", "#fff3e0", "#ff9100"},
        {"#f5f7fa", "#b0bec5", "#37474f", "#ffffff", "#b0bec5"}
    }};

    for (int idx = 1; idx <= 80; ++idx) {
        const auto& c = kPal.at((idx - 1) % 8);
        std::string sid = std::to_string(idx);
        std::string defs =
            svgTag("linearGradient", "id=\"dgf" + sid + "\" x1=\"0%\" y1=\"0%\" x2=\"0%\" y2=\"100%\"", false) +
            svgTag("stop", std::string("offset=\"0%\" stop-color=\"") + c.at(0) + "\"") +
            svgTag("stop", std::string("offset=\"50%\" stop-color=\"") + c.at(1) + "\"") +
            svgTag("stop", std::string("offset=\"100%\" stop-color=\"") + c.at(2) + "\"") + svgClose("linearGradient") +
            svgTag("linearGradient", "id=\"dgl" + sid + "\" x1=\"0%\" y1=\"0%\" x2=\"100%\" y2=\"100%\"", false) +
            svgTag("stop", std::string("offset=\"0%\" stop-color=\"") + c.at(3) + "\" stop-opacity=\"0.50\"") +
            svgTag("stop", std::string("offset=\"100%\" stop-color=\"") + c.at(4) + "\" stop-opacity=\"0.38\"") + svgClose("linearGradient");

        std::string b;
        int shape = (idx - 1) / 8;
        if (idx == 65 || idx == 66) {
            int cx = (idx == 65) ? 190 : 410;
            b = svgTag("ellipse", "cx=\"" + std::to_string(cx) + "\" cy=\"356\" rx=\"78\" ry=\"62\" fill=\"#18181f\" stroke=\"#000000\" stroke-width=\"6\"") +
                svgTag("line", "x1=\"88\" y1=\"285\" x2=\"512\" y2=\"325\" stroke=\"#141418\" stroke-width=\"11\"");
        } else if (idx == 67 || idx == 68) {
            int cx = (idx == 67) ? 410 : 190;
            b = svgTag("circle", "cx=\"" + std::to_string(cx) + "\" cy=\"362\" r=\"86\" fill=\"url(#dgl" + sid + ")\" stroke=\"#d4af37\" stroke-width=\"11\"");
        } else if (shape == 5 || shape == 8) {
            b = svgTag("rect", "x=\"82\" y=\"302\" width=\"436\" height=\"112\" rx=\"28\" fill=\"url(#dgl" + sid + ")\" stroke=\"url(#dgf" + sid + ")\" stroke-width=\"14\"");
        } else if (shape % 2 == 0) {
            b = svgTag("circle", "cx=\"190\" cy=\"356\" r=\"90\" fill=\"url(#dgl" + sid + ")\" stroke=\"url(#dgf" + sid + ")\" stroke-width=\"13\"") +
                svgTag("circle", "cx=\"410\" cy=\"356\" r=\"90\" fill=\"url(#dgl" + sid + ")\" stroke=\"url(#dgf" + sid + ")\" stroke-width=\"13\"") +
                svgTag("path", "d=\"M 278,350 Q 300,332 322,350\" fill=\"none\" stroke=\"url(#dgf" + sid + ")\" stroke-width=\"12\"");
        } else {
            b = svgTag("rect", "x=\"92\" y=\"288\" width=\"194\" height=\"132\" rx=\"22\" fill=\"url(#dgl" + sid + ")\" stroke=\"url(#dgf" + sid + ")\" stroke-width=\"13\"") +
                svgTag("rect", "x=\"314\" y=\"288\" width=\"194\" height=\"132\" rx=\"22\" fill=\"url(#dgl" + sid + ")\" stroke=\"url(#dgf" + sid + ")\" stroke-width=\"13\"") +
                svgTag("rect", "x=\"284\" y=\"330\" width=\"32\" height=\"16\" rx=\"4\" fill=\"url(#dgf" + sid + ")\"");
        }
        saveSvgAssetAt("glasses", idx, wrapSvgCanvas(b, defs));
    }

    for (int idx = 1; idx <= 84; ++idx) {
        const auto& c = kPal.at((idx - 1) % 8);
        std::string sid = std::to_string(idx);
        std::string defs =
            svgTag("linearGradient", "id=\"dhc" + sid + "\" x1=\"15%\" y1=\"0%\" x2=\"85%\" y2=\"100%\"", false) +
            svgTag("stop", std::string("offset=\"0%\" stop-color=\"") + c.at(0) + "\"") +
            svgTag("stop", std::string("offset=\"50%\" stop-color=\"") + c.at(1) + "\"") +
            svgTag("stop", std::string("offset=\"100%\" stop-color=\"") + c.at(2) + "\"") + svgClose("linearGradient");

        int style = (idx - 1) / 8;
        std::string b;
        if (idx >= 81) {
            int bx = 225 + (idx - 81) * 50;
            b = svgTag("path", "d=\"M " + std::to_string(bx) + ",130 Q " + std::to_string(bx - 65) + ",82 " + std::to_string(bx - 108) + ",122 Q " + std::to_string(bx - 52) + ",134 " + std::to_string(bx) + ",142 Z\" fill=\"#3f3f46\" stroke=\"#09090b\" stroke-width=\"3\"") +
                svgTag("path", "d=\"M " + std::to_string(bx) + ",130 Q " + std::to_string(bx + 65) + ",82 " + std::to_string(bx + 108) + ",122 Q " + std::to_string(bx + 52) + ",134 " + std::to_string(bx) + ",142 Z\" fill=\"#3f3f46\" stroke=\"#09090b\" stroke-width=\"3\"") +
                svgTag("ellipse", "cx=\"" + std::to_string(bx) + "\" cy=\"134\" rx=\"16\" ry=\"24\" fill=\"#27272a\"");
        } else if (style == 3 || style == 7) {
            b = svgTag("ellipse", "cx=\"300\" cy=\"218\" rx=\"250\" ry=\"30\" fill=\"url(#dhc" + sid + ")\" stroke=\"#08080a\" stroke-width=\"4\"") +
                svgTag("path", "d=\"M 172,214 Q 250,120 345,42 Q 375,135 428,214 Z\" fill=\"url(#dhc" + sid + ")\" stroke=\"#08080a\" stroke-width=\"4\"");
        } else if (style == 5) {
            b = svgTag("polygon", "points=\"155,218 172,102 234,166 300,86 366,166 428,102 445,218\" fill=\"#facc15\" stroke=\"#78350f\" stroke-width=\"5\"");
        } else if (style == 6) {
            b = svgTag("path", "d=\"M 140,218 C 148,104 452,104 460,218 Z\" fill=\"url(#dhc" + sid + ")\" stroke=\"#14171c\" stroke-width=\"5\"") +
                svgTag("path", "d=\"M 144,176 Q 68,145 88,82 Q 116,134 158,150 Z\" fill=\"#fffdf5\" stroke=\"#4d4433\" stroke-width=\"4\"") +
                svgTag("path", "d=\"M 456,176 Q 532,145 512,82 Q 484,134 442,150 Z\" fill=\"#fffdf5\" stroke=\"#4d4433\" stroke-width=\"4\"");
        } else {
            b = svgTag("ellipse", "cx=\"300\" cy=\"212\" rx=\"238\" ry=\"34\" fill=\"url(#dhc" + sid + ")\" stroke=\"#08080a\" stroke-width=\"4\"") +
                svgTag("path", "d=\"M 168,208 L 186,78 Q 300,62 414,78 L 432,208 Z\" fill=\"url(#dhc" + sid + ")\" stroke=\"#08080a\" stroke-width=\"4\"") +
                svgTag("rect", "x=\"172\" y=\"180\" width=\"256\" height=\"24\" fill=\"" + std::string(c.at(3)) + "\"");
        }
        saveSvgAssetAt("hat", idx, wrapSvgCanvas(b, defs));
    }

    for (int idx = 1; idx <= 104; ++idx) {
        const auto& c = kPal.at((idx - 1) % 8);
        std::string sid = std::to_string(idx);
        std::string defs = svgTag("mask", "id=\"dmk" + sid + "\"", false) + getStandardEyeCutoutMask() + svgClose("mask");
        std::string b =
            svgTag("g", "mask=\"url(#dmk" + sid + ")\"", false) +
            svgTag("ellipse", "cx=\"190\" cy=\"334\" rx=\"76\" ry=\"42\" fill=\"" + std::string(c.at(0)) + "\" opacity=\"0.46\"") +
            svgTag("ellipse", "cx=\"410\" cy=\"334\" rx=\"76\" ry=\"42\" fill=\"" + std::string(c.at(0)) + "\" opacity=\"0.46\"") +
            svgTag("ellipse", "cx=\"164\" cy=\"486\" rx=\"38\" ry=\"22\" fill=\"" + std::string(c.at(3)) + "\" opacity=\"0.32\"") +
            svgTag("ellipse", "cx=\"436\" cy=\"486\" rx=\"38\" ry=\"22\" fill=\"" + std::string(c.at(3)) + "\" opacity=\"0.32\"") +
            svgClose("g");
        saveSvgAssetAt("makeup", idx, wrapSvgCanvas(b, defs));
    }

    static constexpr std::array<std::array<const char*, 5>, 8> kBgScenes = {{
        {"#38bdf8", "#bae6fd", "#15803d", "#22c55e", "#fde047"},
        {"#7c2d12", "#fb923c", "#0369a1", "#f59e0b", "#fef08a"},
        {"#090d16", "#1e293b", "#0f172a", "#1e293b", "#f8fafc"},
        {"#111827", "#374151", "#064e3b", "#022c22", "#fde047"},
        {"#1e3a8a", "#93c5fd", "#334155", "#64748b", "#fef9c3"},
        {"#450a0a", "#b91c1c", "#1c1917", "#ea580c", "#f97316"},
        {"#09090b", "#311042", "#1e1b4b", "#06b6d4", "#ec4899"},
        {"#082f49", "#0284c7", "#0f766e", "#14b8a6", "#7dd3fc"}
    }};
    for (int idx = 1; idx <= 8; ++idx) {
        const auto& s = kBgScenes.at(idx - 1);
        std::string sid = std::to_string(idx);
        std::string defs =
            svgTag("linearGradient", "id=\"dbsk" + sid + "\" x1=\"0%\" y1=\"0%\" x2=\"0%\" y2=\"100%\"", false) +
            svgTag("stop", std::string("offset=\"0%\" stop-color=\"") + s.at(0) + "\"") +
            svgTag("stop", std::string("offset=\"100%\" stop-color=\"") + s.at(1) + "\"") + svgClose("linearGradient");
        std::string sky = svgTag("rect", "x=\"0\" y=\"0\" width=\"600\" height=\"800\" fill=\"url(#dbsk" + sid + ")\"") +
                          svgTag("circle", "cx=\"510\" cy=\"115\" r=\"44\" fill=\"" + std::string(s.at(4)) + "\" opacity=\"0.92\"");
        std::string far = svgTag("path", "d=\"M 0,560 Q 170,480 340,550 T 600,530 L 600,800 L 0,800 Z\" fill=\"" + std::string(s.at(2)) + "\"");
        std::string front = svgTag("path", "d=\"M 0,660 Q 220,615 410,655 T 600,635 L 600,800 L 0,800 Z\" fill=\"" + std::string(s.at(3)) + "\"");
        std::array<char, 24> pfx{};
        std::snprintf(pfx.data(), pfx.size(), "bg/%03d", idx);
        exportSvgToPng(std::string(pfx.data()) + "_sky", wrapSvgCanvas(sky, defs), 600, 800, 0.0f);
        exportSvgToPng(std::string(pfx.data()) + "_far", wrapSvgCanvas(far), 600, 800, 2.5f);
        exportSvgToPng(std::string(pfx.data()) + "_front", wrapSvgCanvas(front), 600, 800, 0.0f);
    }
}

} // namespace

int main(void)
{
    std::cout << "=== Generating MakingFaces Misc Pack (241 Memories + All 3D Asset Banks) ===\n";
    ensureBaseIni("makingfaces.ini");
    generateDefaultFxBank();
    genAllFaces();
    genAllEars();
    genAllNoses();
    genAllBrows();
    genAllBeards();
    genAllHair();
    genAllGlassesHatsMakeupAndBg();

    static constexpr std::array<uint32_t, 12> kFxPool = {
        0, 0, 0,
        (1u << 0),  // Bat
        (1u << 1),  // Fly
        (1u << 3),  // Bird
        (1u << 5),  // Spider
        (1u << 6),  // Butterfly
        (1u << 7),  // UFO
        (1u << 10), // Ghost
        (1u << 18), // Rain
        (1u << 19)  // Snow
    };

    for (int slot = 0; slot <= 240; ++slot) {
        int faceId   = 1 + (slot % 128);
        int mkpId    = (slot % 3 == 0) ? (1 + (slot % 104)) : 0;
        int earId    = 1 + (slot % 88);
        int noseId   = 1 + (slot % 95);
        int beardId  = (slot % 2 == 1) ? (1 + (slot % 108)) : 0;
        int glassId  = (slot % 4 == 1) ? (1 + (slot % 80)) : 0;
        int hairId   = (slot % 5 != 4) ? (1 + (slot % 96)) : 0;
        int hatId    = (slot % 3 == 1) ? (1 + (slot % 84)) : 0;
        int browId   = 1 + (slot % 80);
        int bgId     = 1 + (slot % 8);
        uint32_t fx  = kFxPool.at(slot % kFxPool.size());
        int teeth    = (slot % 16);
        int lipCol   = (slot % 10);
        int lash     = (slot % 3 == 0) ? ((slot % 15) + 1) : 0;
        int eyeStyle = (slot % 4 == 0) ? -1 : (10 + ((slot % 12) * 21));
        int animId   = (slot % 65);

        appendMacroCombo({
            faceId, mkpId, earId, noseId, beardId, glassId, hairId, hatId, browId,
            bgId, fx, teeth, lipCol, lash, eyeStyle, -1, animId, false, 0
        });
    }

    cleanSvgAndSaveZip("misc", "packs");
    std::cout << "[OK] Created packs/misc.zip with 241 Memories!\n";
    return 0;
}
