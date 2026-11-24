// ==============================================================================
// MakingFaces - Full Default 3D Character & Props Pack Generator (default.zip)
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
#include <tuple>
#include <vector>

namespace {

struct HairPal { const char* name; const char* deep; const char* dark; const char* mid; const char* hi; };
struct SkinPal { const char* name; const char* hi;   const char* mid;  const char* deep; const char* line; };

constexpr std::array<HairPal, 8> kHairPals = {{
    {"brown",  "#1f1008", "#4d2c18", "#7d4b2d", "#a86d48"},
    {"light",  "#422410", "#7d4c2a", "#b87a50", "#dea378"},
    {"black",  "#050507", "#18181f", "#32323d", "#565666"},
    {"white",  "#6e6e78", "#a8a8b5", "#dcdce6", "#ffffff"},
    {"yellow", "#664900", "#ad8000", "#e6b419", "#ffe875"},
    {"red",    "#3d0902", "#7d1b0a", "#bd361c", "#eb6546"},
    {"green",  "#0a2904", "#1d5c0e", "#39961f", "#75d952"},
    {"purple", "#1c0530", "#451073", "#7624bd", "#b366f5"}
}};

constexpr std::array<SkinPal, 10> kSkinPals = {{
    {"nordic",     "#fff0e6", "#f5c8ad", "#c48866", "#6e4229"},
    {"peach",      "#ffe4d1", "#eab08c", "#b5724d", "#63361e"},
    {"olive",      "#f2c49b", "#c98857", "#854c25", "#42210b"},
    {"latin",      "#e6ad7e", "#b57243", "#733e1b", "#381b08"},
    {"indian",     "#d19264", "#9c5b30", "#5e3012", "#2b1304"},
    {"east_asian", "#fff2dc", "#ebd0a8", "#bd9664", "#6b4d2a"},
    {"west_afr",   "#9e633e", "#693b1f", "#381b0a", "#170902"},
    {"east_afr",   "#754326", "#4a2612", "#240f04", "#0d0401"},
    {"goblin",     "#9eff78", "#42b81d", "#185904", "#0d3304"},
    {"frost",      "#a8ecff", "#3aa8d4", "#104c66", "#082836"}
}};

std::string eyeRimsXml(const std::string& col, const std::string& w = "5")
{
    return svgTag("path", "d=\"M 124,345 C 145,303 230,303 256,347 C 262,379 233,407 190,405 C 145,403 116,379 124,345 Z\" fill=\"none\" stroke=\"#000\" stroke-width=\"11\" opacity=\"0.24\"") +
           svgTag("path", "d=\"M 476,345 C 455,303 370,303 344,347 C 338,379 367,407 410,405 C 455,403 484,379 476,345 Z\" fill=\"none\" stroke=\"#000\" stroke-width=\"11\" opacity=\"0.24\"") +
           svgTag("path", "d=\"M 124,345 C 145,303 230,303 256,347 C 262,379 233,407 190,405 C 145,403 116,379 124,345 Z\" fill=\"none\" stroke=\"" + col + "\" stroke-width=\"" + w + "\"") +
           svgTag("path", "d=\"M 476,345 C 455,303 370,303 344,347 C 338,379 367,407 410,405 C 455,403 484,379 476,345 Z\" fill=\"none\" stroke=\"" + col + "\" stroke-width=\"" + w + "\"");
}

std::string spiderXml(int x, int y, float sc = 1.0f, int rot = 0, const std::string& col = "#18181f", const std::string& eye = "#ff1744")
{
    std::string legs =
        svgTag("path", "d=\"M -6,-4 Q -24,-18 -34,-6\"") + svgTag("path", "d=\"M -8,0 Q -28,-6 -38,8\"") +
        svgTag("path", "d=\"M -8,4 Q -26,10 -34,24\"") + svgTag("path", "d=\"M -5,8 Q -18,22 -24,34\"") +
        svgTag("path", "d=\"M 6,-4 Q 24,-18 34,-6\"") + svgTag("path", "d=\"M 8,0 Q 28,-6 38,8\"") +
        svgTag("path", "d=\"M 8,4 Q 26,10 34,24\"") + svgTag("path", "d=\"M 5,8 Q 18,22 24,34\"");
    char xf[96];
    std::snprintf(xf, sizeof(xf), "transform=\"translate(%d,%d) rotate(%d) scale(%.2f)\"", x, y, rot, sc);
    return svgTag("g", xf, false) +
           svgTag("g", "stroke=\"#111\" stroke-width=\"3.5\" fill=\"none\" stroke-linecap=\"round\"", false) + legs + svgClose("g") +
           svgTag("ellipse", "cx=\"0\" cy=\"10\" rx=\"11\" ry=\"14\" fill=\"" + col + "\"") +
           svgTag("circle", "cx=\"0\" cy=\"-4\" r=\"7\" fill=\"#23232c\"") +
           svgTag("circle", "cx=\"-2.5\" cy=\"-9\" r=\"1.8\" fill=\"" + eye + "\"") +
           svgTag("circle", "cx=\"2.5\" cy=\"-9\" r=\"1.8\" fill=\"" + eye + "\"") + svgClose("g");
}

std::string buildFaceSvg(int idx, const std::string& pathD, const std::string& c1, const std::string& c2, const std::string& c3, const std::string& stroke, const std::string& extra = "")
{
    std::string id = std::to_string(idx);
    std::string defs =
        svgTag("radialGradient", "id=\"df" + id + "\" cx=\"46%\" cy=\"38%\" r=\"65%\"", false) +
        svgTag("stop", "offset=\"0%\" stop-color=\"" + c1 + "\"") + svgTag("stop", "offset=\"52%\" stop-color=\"" + c2 + "\"") +
        svgTag("stop", "offset=\"86%\" stop-color=\"" + c3 + "\"") + svgTag("stop", "offset=\"100%\" stop-color=\"" + stroke + "\"") + svgClose("radialGradient") +
        svgTag("radialGradient", "id=\"dhi" + id + "\" cx=\"50%\" cy=\"24%\" r=\"36%\"", false) +
        svgTag("stop", "offset=\"0%\" stop-color=\"#fff\" stop-opacity=\"0.28\"") + svgTag("stop", "offset=\"100%\" stop-color=\"#fff\" stop-opacity=\"0\"") + svgClose("radialGradient") +
        svgTag("mask", "id=\"dm" + id + "\"", false) + getStandardEyeCutoutMask() + svgClose("mask");
    std::string body =
        svgTag("g", "mask=\"url(#dm" + id + ")\"", false) +
        svgTag("path", "d=\"" + pathD + "\" fill=\"url(#df" + id + ")\" stroke=\"" + stroke + "\" stroke-width=\"6\" stroke-linejoin=\"round\"") +
        svgTag("path", "d=\"" + pathD + "\" fill=\"url(#dhi" + id + ")\"") +
        svgTag("path", "d=\"M 190,702 Q 300,758 410,702 Q 300,730 190,702 Z\" fill=\"#000\" opacity=\"0.22\"") +
        extra + eyeRimsXml(stroke, "5") + svgClose("g");
    return wrapSvgCanvas(body, defs);
}
void generateDefaultFaces(void)
{
    const std::string kOval    = "M 300,205 C 445,202 510,255 515,380 C 520,510 488,655 425,725 C 385,762 338,770 300,770 C 262,770 215,762 175,725 C 112,655 80,510 85,380 C 90,255 155,202 300,205 Z";
    const std::string kSlender = "M 300,195 C 440,195 502,255 500,385 C 496,535 435,695 300,768 C 165,695 104,535 100,385 C 98,255 160,195 300,195 Z";
    const std::string kHeart   = "M 300,238 C 365,182 512,198 518,348 C 524,485 432,642 300,770 C 168,642 76,485 82,348 C 88,198 235,182 300,238 Z";
    const std::string kRound   = "M 300,205 C 465,205 535,275 530,395 C 525,525 485,660 415,720 C 370,758 335,768 300,768 C 265,768 230,758 185,720 C 115,660 75,525 70,395 C 65,275 135,205 300,205 Z";
    const std::string kVChin   = "M 102,215 Q 300,188 498,215 L 490,385 L 300,775 L 110,385 Z";
    const std::string kWide    = "M 300,205 C 485,205 555,275 550,395 C 545,525 495,665 415,725 C 370,760 335,768 300,768 C 265,768 230,760 185,725 C 105,665 55,525 50,395 C 45,275 115,205 300,205 Z";
    const std::string kTotem   = "M 135,185 L 465,185 L 485,375 L 455,735 L 300,768 L 145,735 L 115,375 Z";
    const std::string kAlienG  = "M 300,175 C 475,175 545,260 520,395 C 492,535 375,725 300,778 C 225,725 108,535 80,395 C 55,260 125,175 300,175 Z";
    const std::string kEmoji   = "M 300,185 C 475,185 555,310 555,480 C 555,650 475,775 300,775 C 125,775 45,650 45,480 C 45,310 125,185 300,185 Z";
    const std::string kRat     = "M 300,200 C 435,200 495,270 488,385 C 480,490 420,635 300,768 C 180,635 120,490 112,385 C 105,270 165,200 300,200 Z";
    const std::string kBat     = "M 300,215 C 375,180 495,215 518,340 C 535,435 475,540 420,605 L 365,768 L 300,738 L 235,768 L 180,605 C 125,540 65,435 82,340 C 105,215 225,180 300,215 Z";
    const std::string kBunny   = "M 300,200 C 440,200 500,275 512,395 C 525,515 535,645 435,730 C 385,765 340,770 300,770 C 260,770 215,765 165,730 C 65,645 75,515 88,395 C 100,275 160,200 300,200 Z";

    std::string sp1 = spiderXml(155, 255, 0.85f, 25) + spiderXml(445, 485, 1.05f, -35) + spiderXml(210, 675, 0.75f, 145);
    std::string sp2 = svgTag("line", "x1=\"435\" y1=\"195\" x2=\"435\" y2=\"275\" stroke=\"#fff\" stroke-width=\"2\" opacity=\"0.6\"") + spiderXml(435, 275, 0.95f, 180) + spiderXml(145, 455, 1.15f, 42);
    std::string rib = svgTag("path", "d=\"M 225,210 C 175,350 175,595 225,755 M 375,210 C 425,350 425,595 375,755 M 300,205 L 300,768\" fill=\"none\" stroke=\"#421200\" stroke-width=\"5.5\" opacity=\"0.5\"");
    std::string bandage;
    for (int by = 215; by < 720; by += 38)
        bandage += svgTag("path", "d=\"M 118," + std::to_string(by) + " Q 300," + std::to_string(by + (((by / 38) % 2 == 0) ? 10 : -10)) + " 482," + std::to_string(by - (((by / 38) % 2 == 0) ? 10 : -10)) + "\" fill=\"none\" stroke=\"#57534e\" stroke-width=\"11\" opacity=\"0.6\"");
    using FaceDef = std::tuple<std::string, std::string, std::string, std::string, std::string, std::string>;
    const std::vector<FaceDef> faces = {
        {kOval,"#fff0e6","#f5c8ad","#c48866","#6e4229",""},{kHeart,"#fff0e6","#f5c8ad","#c48866","#6e4229",""},
        {kSlender,"#ffe4d1","#eab08c","#b5724d","#63361e",""},{kRound,"#ffe4d1","#eab08c","#b5724d","#63361e",""},
        {kOval,"#f7d6be","#dfa37c","#a36540","#542e18",""},{kHeart,"#f7d6be","#dfa37c","#a36540","#542e18",""},
        {kOval,"#f2c49b","#c98857","#854c25","#42210b",""},{kSlender,"#f2c49b","#c98857","#854c25","#42210b",""},
        {kRound,"#e6ad7e","#b57243","#733e1b","#381b08",""},{kHeart,"#e6ad7e","#b57243","#733e1b","#381b08",""},
        {kOval,"#d19264","#9c5b30","#5e3012","#2b1304",""},{kHeart,"#d19264","#9c5b30","#5e3012","#2b1304",""},
        {kOval,"#fff2dc","#ebd0a8","#bd9664","#6b4d2a",""},{kHeart,"#fff2dc","#ebd0a8","#bd9664","#6b4d2a",""},
        {kRound,"#fae6c8","#e0be8d","#ad834c","#5c401c",""},{kVChin,"#fae6c8","#e0be8d","#ad834c","#5c401c",""},
        {kOval,"#9e633e","#693b1f","#381b0a","#170902",""},{kSlender,"#9e633e","#693b1f","#381b0a","#170902",""},
        {kRound,"#9e633e","#693b1f","#381b0a","#170902",""},{kHeart,"#8a5130","#592e15","#2b1205","#120601",""},
        {kOval,"#754326","#4a2612","#240f04","#0d0401",""},{kSlender,"#754326","#4a2612","#240f04","#0d0401",""},
        {kRound,"#63361c","#3d1d0b","#1c0a02","#080200",""},{kHeart,"#63361c","#3d1d0b","#1c0a02","#080200",""},
        {kOval,"#d49566","#96582e","#4d270e","#241004",""},{kRound,"#d49566","#96582e","#4d270e","#241004",""},
        {kOval,"#e8c4a2","#b5855c","#6b4528","#331e0e",""},{kHeart,"#ffffff","#faf6f0","#dcd2c4","#8c7a6b",""},
        {kSlender,"#8c5230","#592e15","#291205","#120601",""},{kVChin,"#f7e1d3","#dfb59c","#9e7056","#4f3222",""},
        {kOval,"#eab68a","#b8794b","#6e3e1e","#361b0a",""},{kRound,"#c7895c","#8f5129","#4a240c","#1f0d02",""},
        {kRound,"#ffbe6b","#e08524","#7a3b04","#2b1300",""},{kRound,"#ff9e2c","#d95d00","#6e2500","#1a0700",""},
        {kRound,"#ffd166","#d49a2a","#784b08","#2b1900",""},{kRound,"#4a4e69","#22223b","#0d0d14","#020205",""},
        {kRound,"#f5cb8c","#c98e42","#754712","#2e1902",""},{kRound,"#ffffff","#e8e8ed","#b8b8c2","#2b2b33",""},
        {kVChin,"#ff7b29","#d9480f","#731d02","#260800",""},{kRound,"#ffd6de","#ff9eb3","#c95b73","#6e1e30",""},
        {kRound,"#ffe5ec","#ffb3c6","#cc6b82","#6e1e30",""},{kWide,"#a67c52","#6e4726","#361e0b","#140902",""},
        {kWide,"#ced4da","#8d99ae","#495057","#1d2024",""},{kWide,"#d4a373","#a98467","#5c4330","#261910",""},
        {kSlender,"#b5ff85","#47b825","#165207","#082402",""},{kTotem,"#80b918","#386641","#1b4332","#081c15",""},
        {kVChin,"#ffd166","#d9822b","#6e3808","#240f00",""},{kWide,"#9ef01a","#38b000","#006400","#002900",""},
        {kRound,"#d4a373","#8c5e35","#4a2c11","#1c0e02",""},{kRound,"#a47148","#6f4518","#3d2208","#170b01",""},
        {kRound,"#ffffff","#f0f0f5","#c8c8d4","#18181f",""},{kRound,"#8d5b3c","#58311a","#2b1407","#120601",""},
        {kSlender,"#8c4a27","#5c2c12","#2b1205","#140702",""},{kWide,"#ff4d4d","#cc0000","#660000","#ffd700",""},
        {kWide,"#6a1b9a","#38006b","#120024","#ffab00",""},{kWide,"#ff3d00","#b71c1c","#4a0000","#140000",""},
        {kVChin,"#ffffff","#f5f5f5","#dcdce0","#d50000",""},{kOval,"#ffffff","#e8e8e8","#bdbdbd","#111111",""},
        {kTotem,"#26a69a","#00695c","#00332c","#ffd700",""},{kHeart,"#ffffff","#f5f2eb","#cfc8b8","#1a1a1a",""},
        {kOval,"#fff8e7","#e6d5b8","#a89370","#d4af37",""},{kTotem,"#ffd54f","#ffb300","#b26a00","#0d47a1",""},
        {kWide,"#ffa333","#d95300","#732200","#330e00",rib},{kVChin,"#ffffff","#eceff1","#b0bec5","#00e5ff",""},
        {kEmoji,"#fff59d","#ffd600","#f57f17","#6e3800",""},{kEmoji,"#fff59d","#ffd600","#f57f17","#6e3800",""},
        {kEmoji,"#fff59d","#ffd600","#f57f17","#6e3800",""},{kEmoji,"#ea80fc","#8e24aa","#310840","#14001f",""},
        {kEmoji,"#ccff90","#64dd17","#1b5e20","#08290a",""},{kEmoji,"#b3e5fc","#039be5","#01579b","#002244",""},
        {kEmoji,"#fff9c4","#ffee58","#fbc02d","#6e4f00",""},{kEmoji,"#ffcc80","#fb8c00","#bf360c","#4a1400",""},
        {kOval,"#f7f4f0","#e2dcd5","#a3998e","#423a32",""},{kSlender,"#f2f0ec","#d9d4cc","#999187","#3d3730",""},
        {kHeart,"#fff5f7","#ebd5dc","#ab8593","#4a2c38",""},{kOval,"#f0f2f5","#d4dae0","#8c96a0","#323b42",""},
        {kSlender,"#dcedc8","#9ccc65","#4b7524","#1d3609",""},{kOval,"#e6ee9c","#b4bd46","#666b14","#292b04",""},
        {kHeart,"#f3e5f5","#ce93d8","#7b3b8c","#310d3d",""},{kSlender,"#c8e6c9","#81c784","#38783c","#123314",""},
        {kWide,"#ffb74d","#f57c00","#8c2d00","#330e00",rib},{kWide,"#ffffff","#e0e0e0","#9e9e9e","#424242",rib},
        {kWide,"#ccff90","#64dd17","#1b5e20","#08290a",rib},{kWide,"#ea80fc","#7b1fa2","#310840","#12001c",rib},
        {kWide,"#ffb74d","#f57c00","#8c2d00","#330e00",rib+sp1},{kWide,"#ffcc80","#ef6c00","#7f2800","#260900",rib+sp2},
        {kWide,"#ffa726","#e65100","#6e1d00","#260800",rib+sp1},{kWide,"#ff8a65","#d84315","#5c1300","#1f0400",rib+sp2},
        {kAlienG,"#eceff1","#b0bec5","#546e7a","#1c262b",""},{kAlienG,"#b9f6ca","#00e676","#00695c","#002620",""},
        {kAlienG,"#80d8ff","#0091ea","#01579b","#001e38",""},{kAlienG,"#ff8a80","#e53935","#7f0000","#2b0000",""},
        {kRound,"#ffe082","#ffb300","#8c5100","#381e00",""},{kRound,"#ffffff","#e0e0e0","#9e9e9e","#424242",""},
        {kRound,"#78909c","#37474f","#102027","#000000",""},{kRound,"#ffab40","#ef6c00","#7f2800","#260900",""},
        {kRat,"#cfd8dc","#90a4ae","#455a64","#1c262b",""},{kRat,"#d7ccc8","#8d6e63","#4e342e","#1f120e",""},
        {kRat,"#ffffff","#f5f5f5","#cfd8dc","#78909c",""},{kRat,"#78909c","#37474f","#102027","#000000",""},
        {kBat,"#5c6bc0","#283593","#121638","#050714",""},{kBat,"#a1887f","#6d4c41","#3e2723","#1a0e0b",""},
        {kBat,"#ef5350","#b71c1c","#4a0000","#1a0000",""},{kBat,"#fff8f0","#e6dcd0","#a69888","#4a4036",""},
        {kBunny,"#ffffff","#f5f5fa","#cfd8dc","#78909c",""},{kBunny,"#d7ccc8","#a1887f","#5d4037","#261611",""},
        {kBunny,"#eceff1","#b0bec5","#607d8b","#263238",""},{kBunny,"#fff3e0","#ffcc80","#ef6c00","#5c2600",""},
        {kVChin,"#cfd8dc","#78909c","#37474f","#101820",""},{kRound,"#ffffff","#f5f7fa","#cfd8dc","#546e7a",""},
        {kHeart,"#e6b88a","#b87a46","#6e3e18","#2e1604",""},{kRound,"#525866","#2c303a","#12141a","#050608",""},
        {kTotem,"#8d5524","#5c3310","#2b1504","#f59e0b",""},{kTotem,"#4a2c11","#2b1605","#140801","#ef4444",""},
        {kTotem,"#a16207","#713f12","#3f2005","#fde047",""},{kTotem,"#1e293b","#0f172a","#020617","#06b6d4",""},
        {kTotem,"#84cc16","#4d7c0f","#1a2e05","#0f172a",""},{kTotem,"#a3e635","#65a30d","#273e06","#0f172a",""},
        {kSlender,"#f5f5f4","#d6d3d1","#78716c","#292524",bandage},{kOval,"#e7e5e4","#cbbba0","#78716c","#292524",bandage},
        {kWide,"#784b2a","#4a2c18","#1f1008","#0a0401",""},{kWide,"#52525b","#27272a","#09090b","#000000",""},
        {kAlienG,"#c084fc","#7e22ce","#3b0764","#140024",""},{kAlienG,"#2dd4bf","#0f766e","#042f2e","#011414",""},
        {kWide,"#94a3b8","#475569","#1e293b","#090d16",""},{kWide,"#a8a29e","#57534e","#292524","#0c0a09",""},
        {kHeart,"#ffffff","#fdf4ff","#e879f9","#86198f",""},{kHeart,"#fefce8","#fef08a","#eab308","#854d0e",""}
    };
    for (size_t i = 0; i < faces.size(); ++i) {
        const auto& [pd, c1, c2, c3, st, ext] = faces.at(i);
        saveSvgAssetAt("face", static_cast<int>(i + 1), buildFaceSvg(static_cast<int>(i + 1), pd, c1, c2, c3, st, ext));
    }
}

std::string makeSculptedNoseSvg(int idx, const std::string& shape, const std::string& hi, const std::string& mid, const std::string& deep)
{
    std::string id = std::to_string(idx);
    if (shape == "pig_large") {
        std::string defs = svgTag("radialGradient", "id=\"pg" + id + "\" cx=\"42%\" cy=\"32%\" r=\"65%\"", false) + svgTag("stop", "offset=\"0%\" stop-color=\"" + hi + "\"") + svgTag("stop", "offset=\"58%\" stop-color=\"" + mid + "\"") + svgTag("stop", "offset=\"90%\" stop-color=\"" + deep + "\"") + svgTag("stop", "offset=\"100%\" stop-color=\"#3b0d17\"") + svgClose("radialGradient");
        return wrapSvgCanvas(svgTag("ellipse", "cx=\"300\" cy=\"508\" rx=\"74\" ry=\"28\" fill=\"#000\" opacity=\"0.34\"") + svgTag("ellipse", "cx=\"300\" cy=\"480\" rx=\"76\" ry=\"52\" fill=\"url(#pg" + id + ")\" stroke=\"" + deep + "\" stroke-width=\"3\"") + svgTag("ellipse", "cx=\"272\" cy=\"482\" rx=\"14\" ry=\"23\" fill=\"#24050c\"") + svgTag("ellipse", "cx=\"328\" cy=\"482\" rx=\"14\" ry=\"23\" fill=\"#24050c\""), defs);
    }
    if (shape == "cat_nose") return wrapSvgCanvas(svgTag("path", "d=\"M 262,468 Q 300,456 338,468 Q 332,502 300,512 Q 268,502 262,468 Z\" fill=\"" + mid + "\" stroke=\"" + deep + "\" stroke-width=\"4\"") + svgTag("path", "d=\"M 300,512 L 300,535 M 215,482 L 135,468 M 385,482 L 465,468\" stroke=\"" + deep + "\" stroke-width=\"4\" stroke-linecap=\"round\""));
    if (shape == "dog_nose") return wrapSvgCanvas(svgTag("path", "d=\"M 244,468 C 244,442 356,442 356,468 C 356,502 325,522 300,522 C 275,522 244,502 244,468 Z\" fill=\"" + mid + "\" stroke=\"" + deep + "\" stroke-width=\"4\"") + svgTag("ellipse", "cx=\"272\" cy=\"482\" rx=\"12\" ry=\"16\" fill=\"#050507\"") + svgTag("ellipse", "cx=\"328\" cy=\"482\" rx=\"12\" ry=\"16\" fill=\"#050507\""));
    if (shape == "rat_nose") return wrapSvgCanvas(svgTag("ellipse", "cx=\"300\" cy=\"496\" rx=\"22\" ry=\"15\" fill=\"" + mid + "\" stroke=\"" + deep + "\" stroke-width=\"3.5\"") + svgTag("path", "d=\"M 245,485 L 95,468 M 242,498 L 88,512 M 355,485 L 505,468 M 358,498 L 512,512\" stroke=\"" + deep + "\" stroke-width=\"3\" stroke-linecap=\"round\""));
    if (shape == "bat_nose") return wrapSvgCanvas(svgTag("path", "d=\"M 268,485 Q 275,442 300,415 Q 325,442 332,485 Q 315,505 300,494 Q 285,505 268,485 Z\" fill=\"" + mid + "\" stroke=\"" + deep + "\" stroke-width=\"4\""));
    if (shape == "eagle_beak") return wrapSvgCanvas(svgTag("path", "d=\"M 252,412 Q 300,382 348,412 L 364,492 Q 300,564 236,492 Z\" fill=\"" + mid + "\" stroke=\"" + deep + "\" stroke-width=\"4.5\""));
    if (shape == "witch_crooked") return wrapSvgCanvas(svgTag("path", "d=\"M 282,368 Q 305,365 322,374 L 346,472 Q 354,518 308,528 Q 268,532 264,496 Z\" fill=\"" + mid + "\" stroke=\"" + deep + "\" stroke-width=\"4\"") + svgTag("circle", "cx=\"332\" cy=\"466\" r=\"8\" fill=\"" + deep + "\""));
    if (shape == "clown_ball") return wrapSvgCanvas(svgTag("circle", "cx=\"300\" cy=\"474\" r=\"44\" fill=\"" + mid + "\" stroke=\"" + deep + "\" stroke-width=\"3.5\"") + svgTag("ellipse", "cx=\"284\" cy=\"456\" rx=\"14\" ry=\"8\" fill=\"#fff\" opacity=\"0.65\""));
    if (shape == "skull_nasal") return wrapSvgCanvas(svgTag("path", "d=\"M 300,425 L 332,504 Q 312,516 300,496 Q 288,516 268,504 Z\" fill=\"#12100e\" stroke=\"" + deep + "\" stroke-width=\"4\""));
    if (shape == "bunny_twitch") return wrapSvgCanvas(svgTag("path", "d=\"M 272,474 Q 300,464 328,474 Q 318,498 300,504 Q 282,498 272,474 Z\" fill=\"" + mid + "\" stroke=\"" + deep + "\" stroke-width=\"3.5\""));
    if (shape == "reptile_slits") return wrapSvgCanvas(svgTag("ellipse", "cx=\"274\" cy=\"486\" rx=\"7\" ry=\"18\" fill=\"#081404\" stroke=\"" + mid + "\" stroke-width=\"4\"") + svgTag("ellipse", "cx=\"326\" cy=\"486\" rx=\"7\" ry=\"18\" fill=\"#081404\" stroke=\"" + mid + "\" stroke-width=\"4\""));
    int brRx = 23, brRy = 94, tipRx = 34, tipRy = 26, tipY = 486, wRx = 24, wRy = 20, wDx = 32, nDx = 20, nY = 506;
    if (shape == "broad")     { brRx=32; tipRx=52; tipRy=32; tipY=488; wRx=38; wRy=26; wDx=48; nDx=30; nY=512; }
    if (shape == "button")    { brRx=20; brRy=82; tipRx=36; tipRy=28; tipY=478; wRx=22; wRy=18; wDx=28; nDx=18; nY=496; }
    if (shape == "bulbous")   { brRx=26; tipRx=54; tipRy=38; tipY=490; wRx=32; wRy=24; wDx=44; nDx=26; nY=514; }
    if (shape == "soft_slim") { brRx=18; tipRx=28; tipRy=24; tipY=484; wRx=20; wRy=18; wDx=26; nDx=16; nY=504; }

    std::string defs =
        svgTag("radialGradient", "id=\"nb" + id + "\" cx=\"50%\" cy=\"50%\" r=\"50%\"", false) + svgTag("stop", "offset=\"0%\" stop-color=\"" + mid + "\" stop-opacity=\"0.88\"") + svgTag("stop", "offset=\"100%\" stop-color=\"" + deep + "\" stop-opacity=\"0\"") + svgClose("radialGradient") +
        svgTag("radialGradient", "id=\"nt" + id + "\" cx=\"42%\" cy=\"35%\" r=\"58%\"", false) + svgTag("stop", "offset=\"0%\" stop-color=\"" + hi + "\" stop-opacity=\"0.98\"") + svgTag("stop", "offset=\"55%\" stop-color=\"" + mid + "\" stop-opacity=\"0.78\"") + svgTag("stop", "offset=\"100%\" stop-color=\"" + deep + "\" stop-opacity=\"0\"") + svgClose("radialGradient");
    std::string body =
        svgTag("ellipse", "cx=\"300\" cy=\"" + std::to_string(tipY + 18) + "\" rx=\"" + std::to_string(tipRx + 8) + "\" ry=\"14\" fill=\"#000\" opacity=\"0.24\"") +
        svgTag("ellipse", "cx=\"300\" cy=\"412\" rx=\"" + std::to_string(brRx) + "\" ry=\"" + std::to_string(brRy) + "\" fill=\"url(#nb" + id + ")\"") +
        svgTag("ellipse", "cx=\"" + std::to_string(300 - wDx) + "\" cy=\"" + std::to_string(tipY + 8) + "\" rx=\"" + std::to_string(wRx) + "\" ry=\"" + std::to_string(wRy) + "\" fill=\"url(#nb" + id + ")\"") +
        svgTag("ellipse", "cx=\"" + std::to_string(300 + wDx) + "\" cy=\"" + std::to_string(tipY + 8) + "\" rx=\"" + std::to_string(wRx) + "\" ry=\"" + std::to_string(wRy) + "\" fill=\"url(#nb" + id + ")\"") +
        svgTag("ellipse", "cx=\"300\" cy=\"" + std::to_string(tipY) + "\" rx=\"" + std::to_string(tipRx) + "\" ry=\"" + std::to_string(tipRy) + "\" fill=\"url(#nt" + id + ")\"") +
        svgTag("ellipse", "cx=\"" + std::to_string(300 - nDx) + "\" cy=\"" + std::to_string(nY) + "\" rx=\"10\" ry=\"5.5\" fill=\"#0a0402\" opacity=\"0.88\"") +
        svgTag("ellipse", "cx=\"" + std::to_string(300 + nDx) + "\" cy=\"" + std::to_string(nY) + "\" rx=\"10\" ry=\"5.5\" fill=\"#0a0402\" opacity=\"0.88\"");
    return wrapSvgCanvas(body, defs);
}
void generateDefaultNoses(void)
{
    int idx = 1;
    for (const auto& sh : {"classic", "broad", "button", "bulbous", "soft_slim"})
        for (const auto& sp : kSkinPals) { saveSvgAssetAt("nose", idx, makeSculptedNoseSvg(idx, sh, sp.hi, sp.mid, sp.deep)); ++idx; }

    using SpNose = std::tuple<const char*, const char*, const char*, const char*>;
    const std::vector<SpNose> spNoses = {
        {"pig_large","#ffd6de","#ff859b","#a6384e"},{"pig_large","#f2b891","#c47d54","#6e3d20"},{"pig_large","#c8ff9e","#63bf36","#25590c"},
        {"cat_nose","#ffb3c6","#ff758f","#590d22"},{"cat_nose","#4a4e69","#22223b","#09090d"},{"cat_nose","#e09f3e","#9e2a2b","#330c0c"},
        {"dog_nose","#4a4e69","#22223b","#050507"},{"dog_nose","#7f4f24","#582f0e","#241104"},{"dog_nose","#ffb3c6","#e56b6f","#6d2e46"},
        {"rat_nose","#ffd6e0","#ff8fa3","#752538"},{"rat_nose","#cfd8dc","#78909c","#263238"},{"rat_nose","#d7ccc8","#8d6e63","#3e2723"},
        {"bat_nose","#5c6bc0","#283593","#090d26"},{"bat_nose","#a1887f","#5d4037","#1a0e0b"},{"bat_nose","#ef5350","#b71c1c","#3b0000"},
        {"eagle_beak","#fff59d","#ffb300","#6e4200"},{"eagle_beak","#eceff1","#90a4ae","#263238"},{"eagle_beak","#ffcc80","#e65100","#4a1400"},
        {"witch_crooked","#b9f6ca","#43a047","#143d17"},{"witch_crooked","#ffe0b2","#d99b66","#5e3619"},{"witch_crooked","#e1bee7","#8e24aa","#310840"},
        {"clown_ball","#ff8a80","#e53935","#5c0000"},{"clown_ball","#82b1ff","#1e88e5","#062147"},{"clown_ball","#fff59d","#fdd835","#6e5600"},
        {"skull_nasal","#d7ccc8","#8d6e63","#3e2723"},{"skull_nasal","#b9f6ca","#43a047","#0d3304"},
        {"bunny_twitch","#ffd6e0","#ff80ab","#6e1b3b"},{"bunny_twitch","#fff3e0","#ffb74d","#6e3b00"},{"bunny_twitch","#eceff1","#90a4ae","#37474f"},
        {"reptile_slits","#9eff78","#47b825","#165207"},{"reptile_slits","#ffd166","#e09f3e","#5c3a08"},{"reptile_slits","#ff758f","#c9184a","#4a0417"}
    };
    for (const auto& [sh, hi, mid, dp] : spNoses) { saveSvgAssetAt("nose", idx, makeSculptedNoseSvg(idx, sh, hi, mid, dp)); ++idx; }
    for (int i = 0; i < 6; ++i) saveSvgAssetAt("nose", idx++, wrapSvgCanvas(svgTag("path", "d=\"M 272,487 L 300,355 L 328,487 Z\" fill=\"#f8fafc\" stroke=\"#334155\" stroke-width=\"4\"")));
    for (int i = 0; i < 4; ++i) saveSvgAssetAt("nose", idx++, wrapSvgCanvas(svgTag("polygon", "points=\"282,225 318,225 300,75\" fill=\"#fde047\" stroke=\"#475569\" stroke-width=\"3\"")));
    for (const auto& col : {"#27272a", "#3f2718"}) saveSvgAssetAt("nose", idx++, wrapSvgCanvas(svgTag("ellipse", "cx=\"300\" cy=\"482\" rx=\"36\" ry=\"24\" fill=\"" + std::string(col) + "\" stroke=\"#09090b\" stroke-width=\"4\"")));
}
void generateDefaultEars(void)
{
    auto earWrap = [](int id, const std::string& hi, const std::string& dp, const std::string& c) {
        std::string defs = svgTag("radialGradient", "id=\"eg" + std::to_string(id) + "\" cx=\"45%\" cy=\"38%\" r=\"68%\"", false) +
                           svgTag("stop", "offset=\"0%\" stop-color=\"" + hi + "\"") +
                           svgTag("stop", "offset=\"100%\" stop-color=\"" + dp + "\"") + svgClose("radialGradient");
        return wrapSvgCanvas(c, defs);
    };
    int idx = 1;
    // 001..010: Human skin-tone ears
    for (const auto& sp : kSkinPals) {
        saveSvgAssetAt("ears", idx, earWrap(idx, sp.hi, sp.deep,
            svgTag("path", "d=\"M 118,310 C 22,285 22,510 118,528 Z\" fill=\"url(#eg" + std::to_string(idx) + ")\" stroke=\"" + std::string(sp.deep) + "\" stroke-width=\"5\"") +
            svgTag("path", "d=\"M 482,310 C 578,285 578,510 482,528 Z\" fill=\"url(#eg" + std::to_string(idx) + ")\" stroke=\"" + std::string(sp.deep) + "\" stroke-width=\"5\"")));
        ++idx;
    }
    // 011..026: Gold hoops & diamond/pearl earrings
    for (int si : {0, 1, 3, 6}) {
        const auto& sp = kSkinPals.at(si);
        for (int k = 0; k < 4; ++k) {
            saveSvgAssetAt("ears", idx, earWrap(idx, sp.hi, sp.deep,
                svgTag("ellipse", "cx=\"78\" cy=\"405\" rx=\"34\" ry=\"82\" fill=\"url(#eg" + std::to_string(idx) + ")\" stroke=\"" + std::string(sp.deep) + "\" stroke-width=\"5\"") +
                svgTag("ellipse", "cx=\"522\" cy=\"405\" rx=\"34\" ry=\"82\" fill=\"url(#eg" + std::to_string(idx) + ")\" stroke=\"" + std::string(sp.deep) + "\" stroke-width=\"5\"") +
                svgTag("circle", "cx=\"74\" cy=\"502\" r=\"22\" fill=\"none\" stroke=\"#ffd700\" stroke-width=\"7\"") +
                svgTag("circle", "cx=\"526\" cy=\"502\" r=\"22\" fill=\"none\" stroke=\"#ffd700\" stroke-width=\"7\"")));
            ++idx;
        }
    }
    // 027..034: Cat ears
    for (int i = 0; i < 8; ++i) {
        const char* hi = (i == 2) ? "#3a3a45" : ((i == 3) ? "#ffffff" : "#ffbe6b");
        const char* dp = (i == 2) ? "#08080a" : ((i == 3) ? "#b0b0ba" : "#7a3b04");
        saveSvgAssetAt("ears", idx, earWrap(idx, hi, dp,
            svgTag("path", "d=\"M 112,285 L 64,96 L 218,215 Z\" fill=\"url(#eg" + std::to_string(idx) + ")\" stroke=\"" + std::string(dp) + "\" stroke-width=\"5\"") +
            svgTag("path", "d=\"M 122,256 L 90,136 L 188,215 Z\" fill=\"#ff8fa3\"") +
            svgTag("path", "d=\"M 488,285 L 536,96 L 382,215 Z\" fill=\"url(#eg" + std::to_string(idx) + ")\" stroke=\"" + std::string(dp) + "\" stroke-width=\"5\"") +
            svgTag("path", "d=\"M 478,256 L 510,136 L 412,215 Z\" fill=\"#ff8fa3\"")));
        ++idx;
    }
    // 035..044: Floppy dog & elephant ears
    for (int i = 0; i < 10; ++i) {
        const char* hi = (i >= 6) ? "#ced4da" : "#f5cb8c";
        const char* dp = (i >= 6) ? "#495057" : "#754712";
        saveSvgAssetAt("ears", idx, earWrap(idx, hi, dp,
            svgTag("path", "d=\"M 118,275 C 24,295 16,495 58,535 C 92,555 112,455 112,365 Z\" fill=\"url(#eg" + std::to_string(idx) + ")\" stroke=\"" + std::string(dp) + "\" stroke-width=\"5\"") +
            svgTag("path", "d=\"M 482,275 C 576,295 584,495 542,535 C 508,555 488,455 488,365 Z\" fill=\"url(#eg" + std::to_string(idx) + ")\" stroke=\"" + std::string(dp) + "\" stroke-width=\"5\"")));
        ++idx;
    }
    // 045..064: Special ears (including 054 pale human-elf & 055-057 goblin/frost elf)
    for (int s = 45; s <= 64; ++s) {
        std::string xml;
        if (s == 48 || s == 49) {
            const char* hi = (s == 48) ? "#a47148" : "#3a3a45";
            const char* dp = (s == 48) ? "#3d2208" : "#08080a";
            xml = earWrap(s, hi, dp, svgTag("circle", "cx=\"115\" cy=\"218\" r=\"72\" fill=\"url(#eg" + std::to_string(s) + ")\" stroke=\"" + std::string(dp) + "\" stroke-width=\"6\"") + svgTag("circle", "cx=\"485\" cy=\"218\" r=\"72\" fill=\"url(#eg" + std::to_string(s) + ")\" stroke=\"" + std::string(dp) + "\" stroke-width=\"6\""));
        } else if (s == 50 || s == 51) {
            const char* hi = (s == 50) ? "#ffffff" : "#d4a373";
            const char* dp = (s == 50) ? "#b0b0ba" : "#5c4330";
            xml = earWrap(s, hi, dp, svgTag("ellipse", "cx=\"170\" cy=\"145\" rx=\"36\" ry=\"96\" transform=\"rotate(-14 170 145)\" fill=\"url(#eg" + std::to_string(s) + ")\" stroke=\"" + std::string(dp) + "\" stroke-width=\"5\"") + svgTag("ellipse", "cx=\"430\" cy=\"145\" rx=\"36\" ry=\"96\" transform=\"rotate(14 430 145)\" fill=\"url(#eg" + std::to_string(s) + ")\" stroke=\"" + std::string(dp) + "\" stroke-width=\"5\""));
        } else if (s == 54 || s == 55 || s == 56 || s == 57) {
            const char* hi = (s == 54) ? "#f7f4f0" : ((s == 57) ? "#a8ecff" : "#9eff78");
            const char* dp = (s == 54) ? "#999187" : ((s == 57) ? "#104c66" : "#185904");
            xml = earWrap(s, hi, dp, svgTag("path", "d=\"M 118,312 C 4,155 16,295 112,485 Z\" fill=\"url(#eg" + std::to_string(s) + ")\" stroke=\"" + std::string(dp) + "\" stroke-width=\"5\"") + svgTag("path", "d=\"M 482,312 C 596,155 584,295 488,485 Z\" fill=\"url(#eg" + std::to_string(s) + ")\" stroke=\"" + std::string(dp) + "\" stroke-width=\"5\""));
        } else if (s == 59) {
            xml = earWrap(s, "#ff5252", "#6b0000", svgTag("path", "d=\"M 155,235 Q 85,155 125,95 Q 145,155 200,205 Z\" fill=\"url(#eg59)\" stroke=\"#3b0000\" stroke-width=\"5\"") + svgTag("path", "d=\"M 445,235 Q 515,155 475,95 Q 455,155 400,205 Z\" fill=\"url(#eg59)\" stroke=\"#3b0000\" stroke-width=\"5\""));
        } else if (s == 61) {
            xml = earWrap(s, "#9aa5b4", "#333942", svgTag("rect", "x=\"44\" y=\"330\" width=\"58\" height=\"125\" rx=\"10\" fill=\"url(#eg61)\" stroke=\"#181b1f\" stroke-width=\"5\"") + svgTag("circle", "cx=\"64\" cy=\"222\" r=\"16\" fill=\"#ff2a2a\"") + svgTag("rect", "x=\"498\" y=\"330\" width=\"58\" height=\"125\" rx=\"10\" fill=\"url(#eg61)\" stroke=\"#181b1f\" stroke-width=\"5\"") + svgTag("circle", "cx=\"536\" cy=\"222\" r=\"16\" fill=\"#ff2a2a\""));
        } else {
            xml = earWrap(s, "#ffd6de", "#a6384e", svgTag("path", "d=\"M 118,310 C 22,285 22,510 118,528 Z\" fill=\"url(#eg" + std::to_string(s) + ")\" stroke=\"#6e1e30\" stroke-width=\"5\"") + svgTag("path", "d=\"M 482,310 C 578,285 578,510 482,528 Z\" fill=\"url(#eg" + std::to_string(s) + ")\" stroke=\"#6e1e30\" stroke-width=\"5\""));
        }
        saveSvgAssetAt("ears", idx++, xml);
    }
    // 065 & 066: Pale White Vampire Pointed Ears!
    saveSvgAssetAt("ears", 65, earWrap(65, "#f7f5fa", "#8a8599",
        svgTag("path", "d=\"M 118,330 C 12,175 24,335 112,505 Z\" fill=\"url(#eg65)\" stroke=\"#3b3747\" stroke-width=\"5\"") +
        svgTag("path", "d=\"M 482,330 C 588,175 576,335 488,505 Z\" fill=\"url(#eg65)\" stroke=\"#3b3747\" stroke-width=\"5\"")));
    saveSvgAssetAt("ears", 66, earWrap(66, "#eef2ec", "#6b7866",
        svgTag("path", "d=\"M 118,320 C 4,145 14,315 110,510 Z\" fill=\"url(#eg66)\" stroke=\"#2c3329\" stroke-width=\"5\"") +
        svgTag("path", "d=\"M 482,320 C 596,145 586,315 490,510 Z\" fill=\"url(#eg66)\" stroke=\"#2c3329\" stroke-width=\"5\"")));

    // 067..088: Remaining fantasy, robot, bat & 3D character ears
    for (int k = 67; k <= 88; ++k) {
        const char* hi = (k == 68 || k == 87) ? "#ffffff" : ((k == 75 || k == 76) ? "#b71c1c" : ((k == 72 || k == 85) ? "#cfd8dc" : "#784b2a"));
        const char* dp = (k == 68 || k == 87) ? "#757575" : ((k == 75 || k == 76) ? "#2b0000" : ((k == 72 || k == 85) ? "#455a64" : "#1f1008"));
        saveSvgAssetAt("ears", k, earWrap(k, hi, dp,
            svgTag("path", "d=\"M 140,275 L 72,125 L 210,225 Z\" fill=\"url(#eg" + std::to_string(k) + ")\" stroke=\"" + std::string(dp) + "\" stroke-width=\"4\"") +
            svgTag("path", "d=\"M 460,275 L 528,125 L 390,225 Z\" fill=\"url(#eg" + std::to_string(k) + ")\" stroke=\"" + std::string(dp) + "\" stroke-width=\"4\"")));
    }
}

void generateDefaultStrandedBrows(void)
{
    std::mt19937 rng(2026);
    std::uniform_real_distribution<float> u01(0.0f, 1.0f);
    std::uniform_real_distribution<float> jit(-5.0f, 5.0f);
    int idx = 1;
    for (int st = 0; st < 10; ++st) {
        for (const auto& hp : kHairPals) {
            std::ostringstream ss;
            for (int side : {-1, 1}) {
                for (int k = 0; k < 120; ++k) {
                    float u = u01(rng), v = (u01(rng) - 0.5f) * 2.0f;
                    float xb = (st == 7) ? (302.0f - u * 178.0f) : (270.0f - u * 138.0f);
                    float ay = (st == 4) ? (300.0f - u * 40.0f) : (st == 5) ? (294.0f - u * 44.0f) : (285.0f - std::sin(u * 3.14159f * 0.85f) * 24.0f);
                    float hh = (st == 1 || st == 2) ? 4.2f : ((st == 3 || st == 7) ? 16.5f : 11.5f);
                    float ang = (-78.0f + u * 108.0f + jit(rng)) * 3.14159f / 180.0f;
                    float len = (st == 3) ? 19.0f : 15.0f;
                    float dx = -std::cos(ang) * len, dy = std::sin(ang) * len;
                    float x0 = (side == 1) ? (600.0f - xb) : xb;
                    float x1 = (side == 1) ? (600.0f - (xb + dx)) : (xb + dx);
                    float y0 = ay + v * hh, y1 = y0 + dy;
                    const char* col = (u01(rng) < 0.35f) ? hp.deep : ((u01(rng) < 0.75f) ? hp.dark : hp.mid);
                    char buf[160];
                    std::snprintf(buf, sizeof(buf), "d=\"M %.1f,%.1f Q %.1f,%.1f %.1f,%.1f\" fill=\"none\" stroke=\"%s\" stroke-width=\"1.7\" stroke-linecap=\"round\" opacity=\"0.86\"",
                                  x0, y0, (x0 + x1) * 0.5f, (y0 + y1) * 0.5f - 2.0f, x1, y1, col);
                    ss << svgTag("path", buf);
                }
            }
            saveSvgAssetAt("brows", idx++, wrapSvgCanvas(ss.str()));
        }
    }
}
void generateDefaultBeardsAndJaws(void)
{
    std::mt19937 rng(2026);
    std::uniform_real_distribution<float> u01(0.0f, 1.0f);
    int idx = 1;
    for (int st = 0; st < 12; ++st) {
        for (const auto& hp : kHairPals) {
            std::ostringstream ss;
            int cnt = (st == 7) ? 880 : ((st == 8 || st == 9 || st == 11) ? 240 : 480);
            for (int k = 0; k < cnt; ++k) {
                float side = (u01(rng) < 0.5f) ? -1.0f : 1.0f;
                float u = u01(rng), v = (u01(rng) - 0.5f) * 2.0f;
                float x0 = 300.0f + side * u * ((st == 8 || st == 11) ? 24.0f : 132.0f);
                float y0 = (st == 11) ? (674.0f + u01(rng) * 26.0f) : ((st == 7 && u01(rng) < 0.68f) ? (575.0f + u01(rng) * 145.0f) : (548.0f + u * 22.0f + v * 11.0f));
                if (st == 6) { x0 = 300.0f + side * (135.0f + u * 45.0f); y0 = 435.0f + v * 195.0f; }
                float ang = ((st == 0 || st == 1) ? (18.0f - u * 88.0f) : 82.0f) * 3.14159f / 180.0f;
                float x1 = x0 + side * std::cos(ang) * 16.0f, y1 = y0 + std::sin(ang) * 16.0f;
                const char* col = (u01(rng) < 0.4f) ? hp.deep : ((u01(rng) < 0.8f) ? hp.dark : hp.mid);
                char buf[160];
                std::snprintf(buf, sizeof(buf), "d=\"M %.1f,%.1f L %.1f,%.1f\" stroke=\"%s\" stroke-width=\"1.7\" stroke-linecap=\"round\" opacity=\"0.86\"", x0, y0, x1, y1, col);
                ss << svgTag("path", buf);
            }
            saveSvgAssetAt("beard", idx++, wrapSvgCanvas(ss.str()));
        }
    }
    saveSvgAssetAt("beard", 97,  wrapSvgCanvas(svgTag("path", "d=\"M 186,612 L 204,698 Q 300,746 396,698 L 414,612 L 384,612 L 376,655 Q 300,672 224,655 L 216,612 Z\" fill=\"#e5dec9\" stroke=\"#2b2720\" stroke-width=\"5\"")));
    saveSvgAssetAt("beard", 98,  wrapSvgCanvas(svgTag("path", "d=\"M 182,615 L 210,715 L 390,715 L 418,615 L 382,615 L 368,668 L 232,668 L 218,615 Z\" fill=\"#94a3b8\" stroke=\"#14171c\" stroke-width=\"5\"")));
    saveSvgAssetAt("beard", 99,  wrapSvgCanvas(svgTag("path", "d=\"M 176,612 L 208,712 L 300,736 L 392,712 L 424,612 L 386,612 L 362,662 L 325,642 L 300,668 L 275,642 L 238,662 L 214,612 Z\" fill=\"#ea580c\" stroke=\"#2b0d00\" stroke-width=\"5\"")));
    saveSvgAssetAt("beard", 100, wrapSvgCanvas(svgTag("path", "d=\"M 184,612 L 204,706 Q 300,748 396,706 L 416,612 L 382,612 L 372,660 Q 300,678 228,660 L 218,612 Z\" fill=\"#a16207\" stroke=\"#1f0e03\" stroke-width=\"5\"")));

    const std::array<std::tuple<const char*, const char*, const char*>, 4> vbCols = {{
        {"#d97706","#92400e","#451a03"},{"#e2e8f0","#94a3b8","#334155"},{"#57534e","#292524","#0c0a09"},{"#b91c1c","#7f1d1d","#450a0a"}
    }};
    for (int i = 0; i < 4; ++i) {
        const auto& [hi, mid, dp] = vbCols.at(i);
        std::string id = std::to_string(101 + i);
        std::string defs = svgTag("linearGradient", "id=\"vb" + id + "\" x1=\"0%\" y1=\"0%\" x2=\"0%\" y2=\"100%\"", false) + svgTag("stop", "offset=\"0%\" stop-color=\"" + std::string(hi) + "\"") + svgTag("stop", "offset=\"100%\" stop-color=\"" + std::string(dp) + "\"") + svgClose("linearGradient");
        std::string body = svgTag("path", "d=\"M 145,515 Q 300,565 455,515 L 440,685 L 365,785 L 330,725 L 300,792 L 270,725 L 235,785 L 160,685 Z\" fill=\"url(#vb" + id + ")\" stroke=\"" + std::string(dp) + "\" stroke-width=\"5\"") + svgTag("rect", "x=\"288\" y=\"742\" width=\"24\" height=\"16\" rx=\"4\" fill=\"#fbbf24\"");
        saveSvgAssetAt("beard", 101 + i, wrapSvgCanvas(body, defs));
    }
    const std::array<std::pair<const char*, const char*>, 4> wwCols = {{{"#784b2a","#1f1008"},{"#52525b","#09090b"},{"#4d7c0f","#1a2e05"},{"#0284c7","#082f49"}}};
    for (int i = 0; i < 4; ++i)
        saveSvgAssetAt("beard", 105 + i, wrapSvgCanvas(svgTag("path", "d=\"M 155,555 L 185,695 L 235,745 L 265,710 L 300,758 L 335,710 L 365,745 L 415,695 L 445,555 L 405,635 L 300,665 L 195,635 Z\" fill=\"" + std::string(wwCols.at(i).first) + "\" stroke=\"" + std::string(wwCols.at(i).second) + "\" stroke-width=\"5\"")));
}
void generateDefaultGlasses(void)
{
    int idx = 1;
    const std::array<std::tuple<const char*, const char*, const char*>, 8> pals = {{
        {"#525866","#23262e","#80d8ff"},{"#fff099","#d4af37","#ffb300"},{"#ff8a80","#c62828","#ff1744"},{"#82b1ff","#1565c0","#00b0ff"},
        {"#b9f6ca","#2e7d32","#00e676"},{"#ea80fc","#7b1fa2","#d500f9"},{"#ffcc80","#8d5524","#ff9100"},{"#f5f7fa","#b0bec5","#b0bec5"}
    }};
    for (int st = 0; st < 8; ++st) {
        for (const auto& [hi, mid, lens] : pals) {
            std::string b;
            if (st == 0 || st == 6) b = svgTag("circle", "cx=\"190\" cy=\"356\" r=\"90\" fill=\"" + std::string(lens) + "\" fill-opacity=\"0.32\" stroke=\"" + std::string(mid) + "\" stroke-width=\"" + (st == 6 ? "18" : "13") + "\"") + svgTag("circle", "cx=\"410\" cy=\"356\" r=\"90\" fill=\"" + std::string(lens) + "\" fill-opacity=\"0.32\" stroke=\"" + std::string(mid) + "\" stroke-width=\"" + (st == 6 ? "18" : "13") + "\"") + svgTag("line", "x1=\"278\" y1=\"350\" x2=\"322\" y2=\"350\" stroke=\"" + std::string(mid) + "\" stroke-width=\"12\"");
            else if (st == 1) b = svgTag("path", "d=\"M 185,456 C 66,372 86,248 185,310 C 284,248 304,372 185,456 Z\" fill=\"" + std::string(lens) + "\" fill-opacity=\"0.38\" stroke=\"" + std::string(mid) + "\" stroke-width=\"13\"") + svgTag("path", "d=\"M 415,456 C 296,372 316,248 415,310 C 514,248 534,372 415,456 Z\" fill=\"" + std::string(lens) + "\" fill-opacity=\"0.38\" stroke=\"" + std::string(mid) + "\" stroke-width=\"13\"");
            else if (st == 5) b = svgTag("rect", "x=\"82\" y=\"302\" width=\"436\" height=\"114\" rx=\"28\" fill=\"" + std::string(lens) + "\" fill-opacity=\"0.38\" stroke=\"" + std::string(mid) + "\" stroke-width=\"14\"");
            else b = svgTag("rect", "x=\"92\" y=\"288\" width=\"194\" height=\"132\" rx=\"22\" fill=\"" + std::string(lens) + "\" fill-opacity=\"0.32\" stroke=\"" + std::string(mid) + "\" stroke-width=\"13\"") + svgTag("rect", "x=\"314\" y=\"288\" width=\"194\" height=\"132\" rx=\"22\" fill=\"" + std::string(lens) + "\" fill-opacity=\"0.32\" stroke=\"" + std::string(mid) + "\" stroke-width=\"13\"") + svgTag("rect", "x=\"284\" y=\"330\" width=\"32\" height=\"16\" fill=\"" + std::string(mid) + "\"");
            saveSvgAssetAt("glasses", idx++, wrapSvgCanvas(b));
        }
    }
    for (int i = 0; i < 16; ++i) {
        std::string sp = (i % 2 == 0)
            ? svgTag("path", "d=\"M 112,306 Q 190,290 268,306 Q 262,424 190,424 Q 118,424 112,306 Z\" fill=\"#18181f\" stroke=\"#000\" stroke-width=\"6\"") + svgTag("path", "d=\"M 88,310 L 512,273\" stroke=\"#141418\" stroke-width=\"11\"")
            : svgTag("rect", "x=\"96\" y=\"302\" width=\"188\" height=\"118\" rx=\"12\" fill=\"#ff1744\" fill-opacity=\"0.48\" stroke=\"#f5f5f5\" stroke-width=\"13\"") + svgTag("rect", "x=\"316\" y=\"302\" width=\"188\" height=\"118\" rx=\"12\" fill=\"#00e5ff\" fill-opacity=\"0.48\" stroke=\"#f5f5f5\" stroke-width=\"13\"");
        saveSvgAssetAt("glasses", idx++, wrapSvgCanvas(sp));
    }
}
void generateDefaultHair(void)
{
    std::mt19937 rng(2026);
    std::uniform_real_distribution<float> u01(0.0f, 1.0f);
    int idx = 1;
    for (int st = 0; st < 12; ++st) {
        for (const auto& hp : kHairPals) {
            std::ostringstream ss;
            if (st != 5 && st != 7 && st != 10)
                ss << svgTag("path", std::string("d=\"M 72,445 C 70,245 122,158 300,158 C 478,158 530,245 528,445 L 495,445 C 495,285 455,224 300,224 C 145,224 105,285 105,445 Z\" fill=\"") + hp.deep + "\" opacity=\"0.90\"");
            for (int k = 0; k < 620; ++k) {
                float th = -3.5f + u01(rng) * 3.85f, v = u01(rng);
                float x0 = 300.0f + std::cos(th) * (192.0f + v * ((st == 7) ? 64.0f : 34.0f));
                float y0 = 315.0f + std::sin(th) * (148.0f + v * ((st == 7) ? 58.0f : 32.0f));
                if (st == 5) { x0 = 300.0f + (u01(rng) - 0.5f) * 75.0f; y0 = 120.0f + v * 85.0f; }
                else if (st == 10 && std::abs(th + 1.57f) < 0.55f) continue;
                else if (std::abs(x0 - 300.0f) < 165.0f && y0 > 224.0f) y0 = 175.0f + v * 46.0f;
                float len = (st == 3 || st == 9) ? 62.0f : ((st == 2 || st == 4) ? 44.0f : 32.0f);
                const char* col = (u01(rng) < 0.4f) ? hp.dark : ((u01(rng) < 0.85f) ? hp.mid : hp.hi);
                char buf[140];
                std::snprintf(buf, sizeof(buf), "d=\"M %.1f,%.1f L %.1f,%.1f\" stroke=\"%s\" stroke-width=\"2.1\" stroke-linecap=\"round\" opacity=\"0.86\"",
                              x0, y0, x0 + (x0 - 300.0f) * ((st == 10 || st == 11) ? 0.25f : 0.08f), y0 + ((st == 5) ? -48.0f : len), col);
                ss << svgTag("path", buf);
            }
            saveSvgAssetAt("hair", idx++, wrapSvgCanvas(ss.str()));
        }
    }
}
void generateDefaultHats(void)
{
    int idx = 1;
    const std::array<std::pair<const char*, const char*>, 8> pals = {{
        {"#23262e","#ff4d5a"},{"#1b365d","#ffe875"},{"#4d2c14","#ffaa44"},{"#5e178c","#fff066"},
        {"#1b5e20","#ffee58"},{"#880e4f","#f8bbd0"},{"#90a4ae","#40c4ff"},{"#c65100","#ffffff"}
    }};
    for (int st = 0; st < 8; ++st) {
        for (const auto& [mid, band] : pals) {
            std::string b;
            if (st == 0) b = svgTag("ellipse", "cx=\"300\" cy=\"210\" rx=\"238\" ry=\"34\" fill=\"" + std::string(mid) + "\" stroke=\"#08080a\" stroke-width=\"4\"") + svgTag("path", "d=\"M 168,206 L 186,74 Q 300,58 414,74 L 432,206 Z\" fill=\"" + std::string(mid) + "\" stroke=\"#08080a\" stroke-width=\"4\"") + svgTag("rect", "x=\"172\" y=\"178\" width=\"256\" height=\"26\" fill=\"" + std::string(band) + "\"");
            else if (st == 3) b = svgTag("ellipse", "cx=\"300\" cy=\"218\" rx=\"248\" ry=\"30\" fill=\"" + std::string(mid) + "\" stroke=\"#08080a\" stroke-width=\"4\"") + svgTag("path", "d=\"M 172,214 Q 250,120 345,42 Q 375,135 428,214 Z\" fill=\"" + std::string(mid) + "\" stroke=\"#08080a\" stroke-width=\"4\"");
            else if (st == 5) b = svgTag("polygon", "points=\"155,218 172,102 234,166 300,86 366,166 428,102 445,218\" fill=\"" + std::string(band) + "\" stroke=\"#2b1e00\" stroke-width=\"5\"") + svgTag("rect", "x=\"155\" y=\"196\" width=\"290\" height=\"24\" rx=\"6\" fill=\"" + std::string(mid) + "\"");
            else if (st == 6) b = svgTag("path", "d=\"M 140,218 C 148,104 452,104 460,218 Z\" fill=\"" + std::string(mid) + "\" stroke=\"#14171c\" stroke-width=\"5\"") + svgTag("path", "d=\"M 144,176 Q 68,145 88,82 Q 116,134 158,150 Z\" fill=\"#fffdf5\" stroke=\"#4d4433\" stroke-width=\"4\"") + svgTag("path", "d=\"M 456,176 Q 532,145 512,82 Q 484,134 442,150 Z\" fill=\"#fffdf5\" stroke=\"#4d4433\" stroke-width=\"4\"");
            else b = svgTag("path", "d=\"M 142,212 C 160,114 440,114 458,212 Z\" fill=\"" + std::string(mid) + "\" stroke=\"#08080a\" stroke-width=\"4\"") + svgTag("path", "d=\"M 138,208 Q 300,248 462,208 Q 300,228 138,208 Z\" fill=\"" + std::string(band) + "\"");
            saveSvgAssetAt("hat", idx++, wrapSvgCanvas(b));
        }
    }
    for (int i = 0; i < 20; ++i) {
        std::string sp = (i % 2 == 0)
            ? svgTag("path", "d=\"M 140,210 Q 285,68 468,158 L 430,210 Z\" fill=\"#d50000\" stroke=\"#5c0000\" stroke-width=\"4\"") + svgTag("rect", "x=\"126\" y=\"196\" width=\"348\" height=\"32\" rx=\"16\" fill=\"#ffffff\"") + svgTag("circle", "cx=\"478\" cy=\"162\" r=\"24\" fill=\"#ffffff\"")
            : svgTag("ellipse", "cx=\"300\" cy=\"148\" rx=\"124\" ry=\"24\" fill=\"none\" stroke=\"#ffd700\" stroke-width=\"14\"");
        saveSvgAssetAt("hat", idx++, wrapSvgCanvas(sp));
    }
}
void generateDefaultMakeupAndBg(void)
{
    const std::array<std::pair<const char*, const char*>, 8> shades = {{
        {"#d98a7c","#ff6b8b"},{"#2a2233","#e6456b"},{"#14121a","#9e3b5a"},{"#8c33cc","#c7387a"},
        {"#29b6f6","#ff7096"},{"#ffd54f","#d98245"},{"#2e7d32","#81c784"},{"#b71c1c","#ef5350"}
    }};
    for (int i = 0; i < 104; ++i) {
        const auto& [lid, chk] = shades.at(i % shades.size());
        std::string id = std::to_string(i + 1);
        std::string defs = svgTag("mask", "id=\"dmk" + id + "\"", false) + getStandardEyeCutoutMask() + svgClose("mask");
        std::string ext = (i >= 24 && i < 64) ? (svgTag("circle", "cx=\"190\" cy=\"348\" r=\"86\" fill=\"none\" stroke=\"" + std::string(lid) + "\" stroke-width=\"14\" stroke-dasharray=\"16,10\"") + svgTag("circle", "cx=\"410\" cy=\"348\" r=\"86\" fill=\"none\" stroke=\"" + std::string(lid) + "\" stroke-width=\"14\" stroke-dasharray=\"16,10\"")) : ((i >= 64) ? spiderXml(160, 260, 0.95f, 25) + spiderXml(440, 485, 1.05f, -35) : "");
        std::string body = svgTag("g", "mask=\"url(#dmk" + id + ")\"", false) +
            svgTag("ellipse", "cx=\"190\" cy=\"336\" rx=\"76\" ry=\"42\" fill=\"" + std::string(lid) + "\" opacity=\"0.52\"") +
            svgTag("ellipse", "cx=\"410\" cy=\"336\" rx=\"76\" ry=\"42\" fill=\"" + std::string(lid) + "\" opacity=\"0.52\"") +
            svgTag("ellipse", "cx=\"165\" cy=\"488\" rx=\"38\" ry=\"20\" fill=\"" + std::string(chk) + "\" opacity=\"0.30\"") +
            svgTag("ellipse", "cx=\"435\" cy=\"488\" rx=\"38\" ry=\"20\" fill=\"" + std::string(chk) + "\" opacity=\"0.30\"") + ext + svgClose("g");
        saveSvgAssetAt("makeup", i + 1, wrapSvgCanvas(body, defs));
    }

    const std::array<std::array<const char*, 5>, 10> bgs = {{
        {"#38bdf8","#bae6fd","#15803d","#22c55e","#fde047"},{"#7c2d12","#fb923c","#0369a1","#f59e0b","#fef08a"},
        {"#090d16","#1e293b","#0f172a","#1e293b","#f8fafc"},{"#111827","#374151","#064e3b","#022c22","#fde047"},
        {"#1e3a8a","#93c5fd","#334155","#64748b","#fef9c3"},{"#450a0a","#b91c1c","#1c1917","#ea580c","#f97316"},
        {"#09090b","#311042","#1e1b4b","#06b6d4","#ec4899"},{"#082f49","#0284c7","#0f766e","#14b8a6","#7dd3fc"},
        {"#1e1b4b","#4338ca","#0f172a","#1e293b","#e0e7ff"},{"#14532d","#166534","#052e16","#15803d","#fef08a"}
    }};
    for (int i = 0; i < 10; ++i) {
        std::string id = std::to_string(i + 1);
        std::string defs = svgTag("linearGradient", "id=\"dsky" + id + "\" x1=\"0%\" y1=\"0%\" x2=\"0%\" y2=\"100%\"", false) + svgTag("stop", "offset=\"0%\" stop-color=\"" + std::string(bgs.at(i)[0]) + "\"") + svgTag("stop", "offset=\"100%\" stop-color=\"" + std::string(bgs.at(i)[1]) + "\"") + svgClose("linearGradient");
        std::string body = svgTag("rect", "x=\"0\" y=\"0\" width=\"600\" height=\"800\" fill=\"url(#dsky" + id + ")\"") + svgTag("circle", "cx=\"475\" cy=\"110\" r=\"46\" fill=\"" + std::string(bgs.at(i)[4]) + "\" opacity=\"0.92\"") + svgTag("path", "d=\"M 0,560 Q 170,480 340,550 T 600,530 L 600,800 L 0,800 Z\" fill=\"" + std::string(bgs.at(i)[2]) + "\"") + svgTag("path", "d=\"M 0,640 Q 220,585 410,635 T 600,615 L 600,800 L 0,800 Z\" fill=\"" + std::string(bgs.at(i)[3]) + "\"");
        saveSvgAssetAt("bg", i + 1, wrapSvgCanvas(body, defs));
    }
}
} // namespace
#include <fstream>
#include "default_ini_template.inc"

int main(void)
{
    std::cout << "=== Generating Full MakingFaces Original Catalog (128 faces + 871 props + 241 macros) ===\n";
    ensureBaseIni("makingfaces.ini");
    generateDefaultBrowsBank();
    generateDefaultFxBank();
    generateDefaultFaces();
    generateDefaultNoses();
    generateDefaultEars();
    generateDefaultStrandedBrows();
    generateDefaultBeardsAndJaws();
    generateDefaultGlasses();
    generateDefaultHair();
    generateDefaultHats();
    generateDefaultMakeupAndBg();

    {
        std::ofstream iniOut("makingfaces.ini", std::ios::binary);
        iniOut << kOriginalMakeFaceIni;
    }

    cleanSvgAndSaveZip("starter_default", "packs");
    return 0;
}
