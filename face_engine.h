// ==============================================================================
// MakingFaces - Facial Expression Engine Header (Eyebrows, 3D Lips, Lipstick, Teeth)
// Architecture designed by Magnus Herrlin 2026
// ==============================================================================
#pragma once

// ==============================================================================
// 1. INCLUDES AND FUNCTION PROTOTYPES
// ==============================================================================
#include "main.h"

enum LipColorPreset {
  LIP_COLOR_NATURAL     = 0,
  LIP_COLOR_SOFT_PINK   = 1,
  LIP_COLOR_ROSE        = 2,
  LIP_COLOR_CLASSIC_RED = 3,
  LIP_COLOR_CRIMSON     = 4,
  LIP_COLOR_PLUM        = 5,
  LIP_COLOR_CORAL       = 6,
  LIP_COLOR_BLACK       = 7,
  LIP_COLOR_BLUE        = 8,
  LIP_COLOR_GOLD        = 9
};

void initFaceEngine(void);
void setLipColorPreset(uint8_t dmxLipColorVal);
void drawEyebrows(uint8_t rawLeft, uint8_t rawRight);
void drawMouth(uint8_t rawGap, uint8_t rawCurve, uint8_t rawWidth, uint8_t rawTeeth, uint8_t rawTongue, uint8_t rawThickness = 128, bool hideMouth = false);
void updateFaceViaDmx(bool hideMouth = false);
