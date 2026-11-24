// ==============================================================================
// MakingFaces - Choreographed Cartoon & DMX Macro Engine Header
// Architecture designed by Magnus Herrlin 2026
// ==============================================================================
#pragma once

// ==============================================================================
// 1. INCLUDES AND FUNCTION PROTOTYPES
// ==============================================================================
#include "main.h"
#include <vector>

void        initMacroEngine(void);
void        resetAutonomousMacroShow(void);
bool        isDmxMacroActive(uint8_t macroVal);
bool        isFaceHiddenByMacro(void);
bool        isMouthHiddenByMacro(void);
bool        isNoseHiddenByMacro(void);
bool        isBrowsHiddenByMacro(void);
bool        isEyePoppedOut(int eyeIdx);
float       getEyePopOutFactor(int eyeIdx);
int         getActiveMacroSlot(void);
int         decodeDmxMacroSlot(uint8_t macroVal);
const char* getMacroAnimationName(int animId);
void        runMacroById(int animId, uint32_t nowMs);
void        runDmxMacro(uint8_t macroVal, uint32_t nowMs, bool editOverlay = false);
void        runAutonomousMacroShow(uint32_t nowMs, const std::vector<int>& savedSlots, const std::map<int, MacroComboPreset>& presets);
