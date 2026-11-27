// ==============================================================================
// MakingFaces - SDL2_mixer & Stereo Dialogue Audio Subsystem Header
// Architecture designed by Magnus Herrlin 2026
// ==============================================================================
#pragma once

// ==============================================================================
// Section A. Includes & Function Prototypes
// ==============================================================================
#include <cstdint>
#include <string>

void  initAudioEngine(const std::string& audioDir, const std::string& captureDeviceHint = "Monitor");
void  shutdownAudioEngine(void);
void  updateAudioControlFromDmx(uint8_t trackVal, uint8_t volumeVal, uint8_t triggerVal);
void  resetAudioOnDmxTimeout(void);
void  setSystemAudioMonitorEnabled(bool enable);
float getAudioEnvelope(void);
float getAudioLeftEnvelope(void);
float getAudioRightEnvelope(void);
bool  isAudioPlaying(void);
bool  isAudioLipSyncOverrideActive(void);
int   getActiveDialogueSpeaker(void);
