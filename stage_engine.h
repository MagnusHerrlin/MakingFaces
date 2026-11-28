// ==============================================================================
// MakingFaces - General Object-Oriented Stage, Actor & Physics Engine
// Architecture designed by Magnus Herrlin 2026
// ==============================================================================
#pragma once

#include <SDL2/SDL.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

// ==============================================================================
// Section A. Depth Layers, Bounds Mode & 3D Perlin Noise Generator
// ==============================================================================
enum class DepthLayer : int {
    SKY       = 0,
    FAR       = 1,
    MID       = 2,
    CHARACTER = 3,
    FRONT     = 4,
    OVERLAY   = 5
};

enum class BoundsPolicy {
    NONE,
    WRAP,
    BOUNCE,
    KILL_OUTSIDE
};

class PerlinNoise {
public:
    explicit PerlinNoise(uint32_t seed = 2026);
    float noise(float x, float y, float z = 0.0f) const;

private:
    std::array<uint8_t, 512> perm_{};
};

// ==============================================================================
// Section B. Base Actor, Attachable Behaviors & Director-Style Stage
// ==============================================================================
class Actor;

class Behavior {
public:
    virtual ~Behavior() = default;
    virtual void onAttach(Actor& actor) { (void)actor; }
    virtual void stepFrame(Actor& actor, float dt, uint32_t nowMs) = 0;
};

class Actor {
public:
    virtual ~Actor() = default;

    // Spatial, visual & layer properties
    float        x            = 0.0f;
    float        y            = 0.0f;
    float        w            = 64.0f;
    float        h            = 64.0f;
    float        scale        = 1.0f;
    float        rotation     = 0.0f;
    uint8_t      alpha        = 255;
    uint8_t      tintR        = 255;
    uint8_t      tintG        = 255;
    uint8_t      tintB        = 255;
    DepthLayer   layer        = DepthLayer::FRONT;
    bool         visible      = true;
    bool         alive        = true;
    bool         gazeTarget   = false;
    float        gazeCrossEye = 0.0f; // 0.0 = normal tracking, 1.0 = full cross-eyed on nose

    // Physics state (Coding Train / Nature of Code)
    float        vx           = 0.0f;
    float        vy           = 0.0f;
    float        ax           = 0.0f;
    float        ay           = 0.0f;
    float        mass         = 1.0f;
    float        drag         = 0.0f;
    float        maxSpeed     = 600.0f;
    float        age          = 0.0f;
    float        maxLife      = -1.0f; // < 0 means immortal
    int          groupId      = 0;     // Optional tag for swarm/flock grouping

    void addBehavior(std::unique_ptr<Behavior> behavior);
    void clearBehaviors(void);
    void applyForce(float fx, float fy);

    virtual void stepFrame(float dt, uint32_t nowMs);
    virtual void render(SDL_Renderer* ren, float camOffsetX, float camOffsetY) = 0;

protected:
    std::vector<std::unique_ptr<Behavior>> behaviors_;
};

class Stage {
public:
    // 2-axis parallax camera (DMX 128 = 0 speed, 0 = reverse, 255 = forward)
    float camX    = 0.0f;
    float camY    = 0.0f;
    float camVelX = 0.0f;
    float camVelY = 0.0f;

    void setCameraVelocityFromDmx(uint8_t rawX, uint8_t rawY, float maxPxPerSec = 320.0f);
    Actor* addActor(std::unique_ptr<Actor> actor);
    void clearActors(void);
    void stepAll(float dt, uint32_t nowMs);
    void renderLayer(SDL_Renderer* ren, DepthLayer targetLayer, float parallaxFactor = 1.0f);
    bool findPrimaryGazeTarget(float& outX, float& outY, float& outCrossEye) const;

    const std::vector<std::unique_ptr<Actor>>& actorList(void) const { return actorList_; }

private:
    std::vector<std::unique_ptr<Actor>> actorList_;
};
