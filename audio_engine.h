// ==============================================================================
// MakingFaces - SDL2_mixer Audio Subsystem Header
// Architecture designed by Magnus Herrlin 2026
// ==============================================================================
#pragma once

// ==============================================================================
// 1. INCLUDES AND FUNCTION PROTOTYPES
// ==============================================================================
#include <cstdint>
#include <string>

bool  initAudioEngine(const std::string& audioDir, const std::string& captureDeviceHint = "Monitor");
void  shutdownAudioEngine(void);
void  updateAudioControlFromDmx(uint8_t trackVal, uint8_t volumeVal, uint8_t triggerVal);
void  resetAudioOnDmxTimeout(void);
void  setSystemAudioMonitorEnabled(bool enable);
float getAudioEnvelope(void);
bool  isAudioPlaying(void);
bool  isAudioLipSyncOverrideActive(void);
