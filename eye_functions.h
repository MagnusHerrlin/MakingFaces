// ==============================================================================
// MakingFaces - Dedicated Dual-Eye Graphics Pipeline Header
// Architecture designed by Magnus Herrlin 2026
// ==============================================================================
#pragma once

// ==============================================================================
// 1. INCLUDES AND FUNCTION PROTOTYPES
// ==============================================================================
#include "main.h"
#include <string>
#include <vector>

enum EyeLidShapeType {
  LID_SHAPE_SLY    = 0,
  LID_SHAPE_ROUND  = 1,
  LID_SHAPE_ALMOND = 2
};

enum EyelashStyleType {
  LASH_STYLE_NONE     = 0,
  LASH_STYLE_NATURAL  = 1,
  LASH_STYLE_LONG     = 2,
  LASH_STYLE_WINGED   = 3,
  LASH_STYLE_DRAMATIC = 4
};

void initEyes(void);
void reloadDynamicEyeBank(const std::string& assetsDir);
int  getEyeBankCount(void);
std::string getEyeBankName(int idx);
void selectEyeBankIndex(int idx);
int  getSelectedEyeBankIndex(void);
void renderEyePreviewBuffer(int bankIdx, uint32_t* outArgb210x210);

void setEyeStyleForIndex(uint8_t e, uint8_t dmxStyleVal);
void setEyeStyle(uint8_t dmxStyleVal);
void setEyelashStyle(uint8_t dmxLashVal);
void setEyelidFaceActive(bool active);
void setEyelidSkinColor(uint8_t r, uint8_t g, uint8_t b);
void setEyelidMakeupColor(bool enabled, uint8_t r, uint8_t g, uint8_t b);
bool isEyePoppedOut(uint8_t e);
float getEyePopOutFactor(uint8_t e);
void setEyeLidShapeForIndex(uint8_t e, EyeLidShapeType shape);
void setEyeLidShape(EyeLidShapeType shape);
void setRoundEyelidsForIndex(uint8_t e, bool roundMode);
void setRoundEyelids(bool roundMode);
void drawEye(uint8_t e, uint32_t iScale, uint32_t scleraX, uint32_t scleraY, uint32_t uT, uint32_t lT, uint16_t targetSize);
void renderSingleEyeNormalized(uint8_t e, uint8_t rawX, uint8_t rawY, uint8_t rawIris, uint8_t rawBlink, uint8_t rawSize);
void renderEyePair(uint8_t rawX, uint8_t rawY, uint8_t rawIris, uint8_t rawBlink, uint8_t rawSize = 255);
void updateEyeViaDmx(void);
