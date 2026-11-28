// ==============================================================================
// MakingFaces - Attachable Physics, Flow & Autonomous Agent Behaviors
// Architecture designed by Magnus Herrlin 2026
// ==============================================================================
#pragma once

#include "stage_engine.h"

// ==============================================================================
// Section A. Force Field, Perlin Noise & Flocking/Swarm Behaviors
// ==============================================================================
class GravityBehavior : public Behavior {
public:
    GravityBehavior(float gx, float gy) : gx_(gx), gy_(gy) {}
    void stepFrame(Actor& actor, float dt, uint32_t nowMs) override;

    void setGravity(float gx, float gy) { gx_ = gx; gy_ = gy; }

private:
    float gx_ = 0.0f;
    float gy_ = 0.0f;
};

class PerlinFlowBehavior : public Behavior {
public:
    PerlinFlowBehavior(const PerlinNoise& noise, float spatialFreq = 0.008f, float timeFreq = 0.35f, float strength = 180.0f)
        : noise_(noise), spatialFreq_(spatialFreq), timeFreq_(timeFreq), strength_(strength) {}

    void stepFrame(Actor& actor, float dt, uint32_t nowMs) override;

    void setParams(float spatialFreq, float timeFreq, float strength) {
        spatialFreq_ = spatialFreq;
        timeFreq_    = timeFreq;
        strength_    = strength;
    }

private:
    const PerlinNoise& noise_;
    float spatialFreq_ = 0.008f;
    float timeFreq_    = 0.35f;
    float strength_    = 180.0f;
};

class SwarmBehavior : public Behavior {
public:
    SwarmBehavior(const Stage& stage, float neighborRadius = 90.0f, float separationRadius = 32.0f, float steerForce = 240.0f, bool rotateToHeading = false)
        : stage_(stage), neighborRadius_(neighborRadius), separationRadius_(separationRadius), steerForce_(steerForce), rotateToHeading_(rotateToHeading) {}

    void setTarget(bool active, float tx, float ty, float pullWeight = 1.2f) {
        hasTarget_  = active;
        targetX_    = tx;
        targetY_    = ty;
        pullWeight_ = pullWeight;
    }

    void stepFrame(Actor& actor, float dt, uint32_t nowMs) override;

private:
    const Stage& stage_;
    float neighborRadius_   = 90.0f;
    float separationRadius_ = 32.0f;
    float steerForce_       = 240.0f;
    bool  rotateToHeading_  = false;
    bool  hasTarget_        = false;
    float targetX_          = 300.0f;
    float targetY_          = 400.0f;
    float pullWeight_       = 1.2f;
};

// ==============================================================================
// Section B. Bounds, Ground Surface, Lifetime Fade & Audio-Reactive Behaviors
// ==============================================================================
class BoundsBehavior : public Behavior {
public:
    BoundsBehavior(float width, float height, BoundsPolicy policy = BoundsPolicy::WRAP, float margin = 64.0f)
        : width_(width), height_(height), policy_(policy), margin_(margin) {}

    void stepFrame(Actor& actor, float dt, uint32_t nowMs) override;

private:
    float        width_  = 600.0f;
    float        height_ = 800.0f;
    BoundsPolicy policy_ = BoundsPolicy::WRAP;
    float        margin_ = 64.0f;
};

class GroundSurfaceBehavior : public Behavior {
public:
    GroundSurfaceBehavior(float groundY, float wanderSpeed = 95.0f, bool alignRotation = true)
        : groundY_(groundY), wanderSpeed_(wanderSpeed), alignRotation_(alignRotation) {}

    void stepFrame(Actor& actor, float dt, uint32_t nowMs) override;

private:
    float groundY_       = 730.0f;
    float wanderSpeed_   = 95.0f;
    bool  alignRotation_ = true;
};

class LifeFadeBehavior : public Behavior {
public:
    LifeFadeBehavior(uint8_t startAlpha = 255, uint8_t endAlpha = 0, float startScale = 1.0f, float endScale = 1.0f)
        : startAlpha_(startAlpha), endAlpha_(endAlpha), startScale_(startScale), endScale_(endScale) {}

    void stepFrame(Actor& actor, float dt, uint32_t nowMs) override;

private:
    uint8_t startAlpha_ = 255;
    uint8_t endAlpha_   = 0;
    float   startScale_ = 1.0f;
    float   endScale_   = 1.0f;
};

class AudioPulseBehavior : public Behavior {
public:
    AudioPulseBehavior(float scaleBoost = 0.45f, float liftForce = -260.0f)
        : scaleBoost_(scaleBoost), liftForce_(liftForce) {}

    void onAttach(Actor& actor) override { baseScale_ = actor.scale; }
    void stepFrame(Actor& actor, float dt, uint32_t nowMs) override;

private:
    float baseScale_  = 1.0f;
    float scaleBoost_ = 0.45f;
    float liftForce_  = -260.0f;
};
