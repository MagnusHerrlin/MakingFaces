// ==============================================================================
// MakingFaces - Shared 3D SVG/PNG Asset & Theme Pack Generator Utilities
// Architecture designed by Magnus Herrlin 2026
// ==============================================================================

#ifndef PACK_UTILS_H
#define PACK_UTILS_H

#include <cstdint>
#include <map>
#include <string>
#include <vector>

struct PackMacroSpec {
    int  face         = 1;
    int  makeup       = 0;
    int  ears         = 0;
    int  nose         = 1;
    int  beard        = 0;
    int  glasses      = 0;
    int  hair         = 0;
    int  hat          = 0;
    int  brows        = 1;
    int  bg_scene     = 0;
    int  fx_mask      = 0;
    int  teeth        = 1;
    int  lip_color    = 3;
    int  eyelash      = 0;
    int  eye_style    = -1;
    int  iris         = -1;
    int  anim_id      = 0;
    bool dual_profile = false;
    int  eye_bank     = -1;
};


// Safe 7-bit ASCII XML/SVG tag builders
std::string svgTag(const std::string& tag, const std::string& attrs = "", bool selfClose = true);
std::string svgClose(const std::string& tag);
std::string wrapSvgCanvas(const std::string& body, const std::string& defs = "", int w = 600, int h = 800);
std::string getStandardEyeCutoutMask(void);

// Asset slot allocation & SVG-to-PNG rasterization
int  nextAssetIdx(const std::string& folder);
bool exportSvgToPng(const std::string& relPathNoExt, const std::string& svgXml, int w = 600, int h = 800, float blurSigma = 0.0f);
int  saveSvgAsset(const std::string& folder, const std::string& svgXml, int w = 600, int h = 800, float blurSigma = 0.0f);
void saveSvgAssetAt(const std::string& folder, int idx, const std::string& svgXml, int w = 600, int h = 800, float blurSigma = 0.0f);

// INI configuration & theme pack ZIP archiver
void ensureBaseIni(const std::string& iniPath = "makingfaces.ini");
void appendFaceOffsets(int faceId, const std::map<std::string, std::string>& kv, const std::string& iniPath = "makingfaces.ini");
void appendMacroCombo(const PackMacroSpec& m, const std::string& iniPath = "makingfaces.ini");
void generateDefaultBrowsBank(void);
void generateDefaultFxBank(void);
void cleanSvgAndSaveZip(const std::string& packName, const std::string& packsDir = "packs");

// 3D Side-profile primitive builders (127+ left & right profiles)
std::string build3dProfileHeadSvg(const std::string& tag, const std::string& side, const std::string& f1, const std::string& f2, const std::string& f3, const std::string& mh, const std::string& mm, const std::string& md);
std::string build3dProfileEarsSvg(const std::string& tag, const std::string& side, const std::string& hi, const std::string& mid, const std::string& deep, const std::string& inHi, const std::string& inDeep, int earR = 46);
std::string build3dProfileNoseSvg(const std::string& tag, const std::string& side, const std::string& hi, const std::string& mid, const std::string& deep);

#endif
