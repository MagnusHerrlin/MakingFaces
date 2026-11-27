// ==============================================================================
// MakingFaces - Procedural 64-Animation & DMX Memory Show Engine Header
// Architecture designed by Magnus Herrlin 2026
// ==============================================================================
#ifndef ANIMATION_ENGINE_H
#define ANIMATION_ENGINE_H

#include "main.h"
#include <cstdint>
#include <map>
#include <vector>

// ==============================================================================
// Section A. Animation Engine Lifecycle & Query API
// ==============================================================================
void        initAnimationEngine(void);
void        resetAutonomousShow(void);
bool        isDmxMemoryActive(uint8_t rawMemVal);
int         decodeDmxMemorySlot(uint8_t rawMemVal);
int         getActiveMemorySlot(void);
const char* getAnimationName(int animId);

// ==============================================================================
// Section B. Hide-Flag Queries & Animation / Memory Executors
// ==============================================================================
bool isMouthHiddenByMemory(void);
bool isNoseHiddenByMemory(void);
bool isBrowsHiddenByMemory(void);
bool isFaceHiddenByMemory(void);

void runAnimationById(int animId, uint32_t nowMs);
void runAutonomousMemoryShow(uint32_t nowMs, const std::vector<int>& savedSlots, const std::map<int, MemoryPreset>& memories);
void runDmxMemory(uint8_t rawMemVal, uint32_t nowMs, bool allowLiveDmxOverlay);

#endif
