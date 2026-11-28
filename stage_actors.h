// ==============================================================================
// MakingFaces - Concrete Renderable Actors, Shared Video & DMX Particle Emitter
// Architecture designed by Magnus Herrlin 2026
// ==============================================================================
#pragma once

#include "stage_behaviors.h"
#include "stage_engine.h"
#include <opencv2/opencv.hpp>
#include <random>
#include <vector>

// ==============================================================================
// Section A. SpriteActor, SeamlessFogActor, Special FX Actors & VideoActor
// ==============================================================================
class SpriteActor : public Actor {
public:
    SDL_Texture*              texture      = nullptr;
    std::vector<SDL_Texture*> frames;
    uint32_t                  frameStepMs  = 110;
    uint32_t                  animPhaseMs  = 0;
    bool                      autoFlipHead = false;
    bool                      wingFlap     = false;
    SDL_RendererFlip          flip         = SDL_FLIP_NONE;

    SpriteActor(SDL_Texture* tex, float px, float py, DepthLayer zLayer = DepthLayer::FRONT);
    void render(SDL_Renderer* ren, float camOffsetX, float camOffsetY) override;
};

class SeamlessFogActor : public Actor {
public:
    SDL_Texture*       texture     = nullptr;
    const PerlinNoise* noiseGen    = nullptr;
    float              baseSpeedX  = 24.0f;
    float              perlinSwayX = 18.0f;
    float              perlinSwayY = 14.0f;
    float              tileScaleW  = 1.0f;
    bool               periodicTex = true;
    uint8_t            targetAlpha = 0;

    SeamlessFogActor(SDL_Texture* tex, const PerlinNoise* noise, DepthLayer zLayer, float speedX);
    void stepFrame(float dt, uint32_t nowMs) override;
    void render(SDL_Renderer* ren, float camOffsetX, float camOffsetY) override;

private:
    float scrollX_   = 0.0f;
    float waveOffY_  = 0.0f;
    float curAlphaF_ = 0.0f;
};

class UfoActor : public Actor {
public:
    SDL_Texture* texture   = nullptr;
    SDL_Texture* beamGlow  = nullptr;
    uint8_t      depthMask = 0;

    UfoActor(SDL_Texture* ufoTex, SDL_Texture* glowTex);
    void stepFrame(float dt, uint32_t nowMs) override;
    void render(SDL_Renderer* ren, float camOffsetX, float camOffsetY) override;

private:
    uint32_t enableMs_  = 0;
    bool     wasActive_ = false;
};

class NoseFlyActor : public Actor {
public:
    SDL_Texture* texWingsSpread = nullptr;
    SDL_Texture* texWingsFolded = nullptr;
    bool         active         = false;
    float        noseX          = 300.0f;
    float        noseY          = 444.0f;

    NoseFlyActor(SDL_Texture* spreadTex, SDL_Texture* foldedTex);
    void stepFrame(float dt, uint32_t nowMs) override;
    void render(SDL_Renderer* ren, float camOffsetX, float camOffsetY) override;

private:
    bool             landed_    = false;
    SDL_RendererFlip flip_      = SDL_FLIP_NONE;
    uint32_t         enableMs_  = 0;
    bool             wasActive_ = false;
};

class FireflyActor : public SpriteActor {
public:
    SDL_Texture* glowTex = nullptr;
    float        phase   = 0.0f;

    FireflyActor(SDL_Texture* bodyTex1, SDL_Texture* bodyTex2, SDL_Texture* additiveGlowTex, float px, float py);
    void render(SDL_Renderer* ren, float camOffsetX, float camOffsetY) override;
};

class SpiderActor : public Actor {
public:
    SDL_Texture* tex1    = nullptr;
    SDL_Texture* tex2    = nullptr;
    float        anchorX = 515.0f;
    bool         active  = false;

    SpiderActor(SDL_Texture* f1, SDL_Texture* f2, float topAnchorX = 515.0f);
    void stepFrame(float dt, uint32_t nowMs) override;
    void render(SDL_Renderer* ren, float camOffsetX, float camOffsetY) override;
};

class SnakeActor : public Actor {
public:
    SDL_Texture* tex1   = nullptr;
    SDL_Texture* tex2   = nullptr;
    bool         active = false;

    SnakeActor(SDL_Texture* f1, SDL_Texture* f2);
    void stepFrame(float dt, uint32_t nowMs) override;
    void render(SDL_Renderer* ren, float camOffsetX, float camOffsetY) override;
};

class LightningActor : public Actor {
public:
    SDL_Texture* tex1   = nullptr;
    SDL_Texture* tex2   = nullptr;
    bool         active = false;

    LightningActor(SDL_Texture* f1, SDL_Texture* f2);
    void stepFrame(float dt, uint32_t nowMs) override;
    void render(SDL_Renderer* ren, float camOffsetX, float camOffsetY) override;

private:
    SDL_Texture* activeTex_ = nullptr;
    int          shiftX_    = 0;
};

class FireActor : public Actor {
public:
    SDL_Texture* tex1   = nullptr;
    SDL_Texture* tex2   = nullptr;
    bool         active = false;

    FireActor(SDL_Texture* f1, SDL_Texture* f2);
    void stepFrame(float dt, uint32_t nowMs) override;
    void render(SDL_Renderer* ren, float camOffsetX, float camOffsetY) override;
};

class StaticPropActor : public Actor {
public:
    SDL_Texture* texture = nullptr;
    bool         active  = false;

    StaticPropActor(SDL_Texture* tex, DepthLayer zLayer = DepthLayer::MID);
    void render(SDL_Renderer* ren, float camOffsetX, float camOffsetY) override;
};

class SharedVideoClip {
public:
    SharedVideoClip() = default;
    ~SharedVideoClip();

    bool open(SDL_Renderer* ren, const std::string& filePath, bool loopVideo = true);
    void close(void);
    void updateFrame(uint32_t nowMs);

    SDL_Texture* texture(void) const { return tex_; }
    int          width(void) const   { return width_; }
    int          height(void) const  { return height_; }
    bool         isOpen(void) const  { return cap_.isOpened(); }

private:
    cv::VideoCapture cap_;
    SDL_Texture*     tex_          = nullptr;
    int              width_        = 0;
    int              height_       = 0;
    bool             loop_         = true;
    uint32_t         frameStepMs_  = 33;
    uint32_t         lastUpdateMs_ = 0;
};

class VideoActor : public Actor {
public:
    SharedVideoClip* clip     = nullptr;
    SDL_Rect         srcCrop  = {0, 0, 0, 0};
    bool             additive = false;

    VideoActor(SharedVideoClip* videoClip, float px, float py, DepthLayer zLayer = DepthLayer::FRONT);
    void render(SDL_Renderer* ren, float camOffsetX, float camOffsetY) override;
};

// ==============================================================================
// Section B. DMX-Controllable ParticleEmitter & 4-Bit Depth Layer Masking
// ==============================================================================
enum class EmitterPhysicsMode : int {
    STILL         = 0,
    GRAVITY_FALL  = 1,
    RISING_PLUME  = 2,
    PERLIN_SWIRL  = 3,
    SWARM_FLOCK   = 4,
    GROUND_CRAWL  = 5,
    AUDIO_PULSE   = 6,
    AUTUMN_LEAVES = 7,
    FIREWORKS     = 8,
    FIREFLIES     = 9
};

struct EmitterDmxParams {
    int                targetCount  = 0;
    float              particleSize = 1.0f;
    EmitterPhysicsMode physicsMode  = EmitterPhysicsMode::PERLIN_SWIRL;
    float              turbulence   = 1.0f;
    DepthLayer         targetLayer  = DepthLayer::FRONT;
    float              emitterX     = -1.0f;
    float              emitterY     = -1.0f;
    uint8_t            tintR        = 255;
    uint8_t            tintG        = 255;
    uint8_t            tintB        = 255;
    uint8_t            alpha        = 255;
    bool               autoFlipHead = false;
    bool               wingFlap     = false;
    bool               gazeTarget   = false;
    uint8_t            depthMask    = 0;
};

class ParticleEmitter {
public:
    explicit ParticleEmitter(PerlinNoise& noiseGen, int swarmGroupId = 100);

    void setSpriteSource(SDL_Texture* tex);
    void setAnimatedFrames(const std::vector<SDL_Texture*>& animFrames, uint32_t stepMs = 110);
    void setGlowTexture(SDL_Texture* additiveGlowTex);
    void setVideoSource(SharedVideoClip* clip, bool additiveBlend = false);
    void updateFromDmx(Stage& stage, const EmitterDmxParams& params, float dt);

private:
    void spawnOne(Stage& stage, const EmitterDmxParams& params);
    void spawnFireworkBurst(Stage& stage, const EmitterDmxParams& params);

    PerlinNoise&              noise_;
    int                       groupId_      = 100;
    SDL_Texture*              spriteTex_    = nullptr;
    std::vector<SDL_Texture*> animFrames_;
    uint32_t                  frameStepMs_  = 110;
    SDL_Texture*              glowTex_      = nullptr;
    SharedVideoClip*          videoClip_    = nullptr;
    bool                      videoAdd_     = false;
    float                     spawnAccum_   = 0.0f;
    float                     burstTimer_   = 0.0f;
    std::mt19937              rng_{2026};
};
