// ==============================================================================
// MakingFaces - General Object-Oriented Stage, Actor & Physics Engine
// Architecture designed by Magnus Herrlin 2026
// ==============================================================================
#include "stage_engine.h"
#include <numeric>
#include <random>

// ==============================================================================
// Section A. 3D Perlin Noise Generator & Base Actor Lifecycle
// ==============================================================================
PerlinNoise::PerlinNoise(uint32_t seed)
{
    std::array<uint8_t, 256> p{};
    std::iota(p.begin(), p.end(), 0);
    std::mt19937 rng(seed);
    std::shuffle(p.begin(), p.end(), rng);
    for (size_t i = 0; i < 256; ++i) {
        perm_.at(i)       = p.at(i);
        perm_.at(i + 256) = p.at(i);
    }
}

float PerlinNoise::noise(float x, float y, float z) const
{
    auto fade = [](float t) { return t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f); };
    auto lerp = [](float t, float a, float b) { return a + t * (b - a); };
    auto grad = [](int hash, float gx, float gy, float gz) {
        int h = hash & 15;
        float u = (h < 8) ? gx : gy;
        float v = (h < 4) ? gy : ((h == 12 || h == 14) ? gx : gz);
        return (((h & 1) == 0) ? u : -u) + (((h & 2) == 0) ? v : -v);
    };

    int xi = static_cast<int>(std::floor(x)) & 255;
    int yi = static_cast<int>(std::floor(y)) & 255;
    int zi = static_cast<int>(std::floor(z)) & 255;

    float xf = x - std::floor(x);
    float yf = y - std::floor(y);
    float zf = z - std::floor(z);

    float u = fade(xf);
    float v = fade(yf);
    float w = fade(zf);

    int a  = perm_.at(xi) + yi;
    int aa = perm_.at(a) + zi;
    int ab = perm_.at(a + 1) + zi;
    int b  = perm_.at(xi + 1) + yi;
    int ba = perm_.at(b) + zi;
    int bb = perm_.at(b + 1) + zi;

    float x1 = lerp(u, grad(perm_.at(aa), xf, yf, zf), grad(perm_.at(ba), xf - 1.0f, yf, zf));
    float x2 = lerp(u, grad(perm_.at(ab), xf, yf - 1.0f, zf), grad(perm_.at(bb), xf - 1.0f, yf - 1.0f, zf));
    float y1 = lerp(v, x1, x2);

    float x3 = lerp(u, grad(perm_.at(aa + 1), xf, yf, zf - 1.0f), grad(perm_.at(ba + 1), xf - 1.0f, yf, zf - 1.0f));
    float x4 = lerp(u, grad(perm_.at(ab + 1), xf, yf - 1.0f, zf - 1.0f), grad(perm_.at(bb + 1), xf - 1.0f, yf - 1.0f, zf - 1.0f));
    float y2 = lerp(v, x3, x4);

    return lerp(w, y1, y2);
}

void Actor::addBehavior(std::unique_ptr<Behavior> behavior)
{
    if (!behavior) return;
    behavior->onAttach(*this);
    behaviors_.push_back(std::move(behavior));
}

void Actor::clearBehaviors(void)
{
    behaviors_.clear();
}

void Actor::applyForce(float fx, float fy)
{
    float invMass = 1.0f / std::max(0.01f, mass);
    ax += fx * invMass;
    ay += fy * invMass;
}

void Actor::stepFrame(float dt, uint32_t nowMs)
{
    if (!alive) return;

    for (auto& b : behaviors_) {
        b->stepFrame(*this, dt, nowMs);
    }

    vx += ax * dt;
    vy += ay * dt;

    if (drag > 0.0f) {
        float damp = std::max(0.0f, 1.0f - drag * dt);
        vx *= damp;
        vy *= damp;
    }

    float speedSq = vx * vx + vy * vy;
    if (maxSpeed > 0.0f && speedSq > (maxSpeed * maxSpeed)) {
        float inv = maxSpeed / std::sqrt(speedSq);
        vx *= inv;
        vy *= inv;
    }

    x += vx * dt;
    y += vy * dt;
    ax = 0.0f;
    ay = 0.0f;

    age += dt;
    if (maxLife > 0.0f && age >= maxLife) {
        alive = false;
    }
}

// ==============================================================================
// Section B. Stage Camera, actorList Management & Layered Rendering
// ==============================================================================
void Stage::setCameraVelocityFromDmx(uint8_t rawX, uint8_t rawY, float maxPxPerSec)
{
    auto decodeAxis = [maxPxPerSec](uint8_t raw) -> float {
        int delta = static_cast<int>(raw) - 128;
        if (std::abs(delta) <= 2) return 0.0f;
        float norm = (delta < 0)
                   ? (static_cast<float>(delta + 2) / 126.0f)
                   : (static_cast<float>(delta - 2) / 125.0f);
        return std::max(-1.0f, std::min(1.0f, norm)) * maxPxPerSec;
    };

    camVelX = decodeAxis(rawX);
    camVelY = decodeAxis(rawY);
}

Actor* Stage::addActor(std::unique_ptr<Actor> actor)
{
    if (!actor) return nullptr;
    Actor* rawPtr = actor.get();
    actorList_.push_back(std::move(actor));
    return rawPtr;
}

void Stage::clearActors(void)
{
    actorList_.clear();
}

void Stage::stepAll(float dt, uint32_t nowMs)
{
    camX += camVelX * dt;
    camY += camVelY * dt;

    for (size_t i = 0; i < actorList_.size(); ++i) {
        if (actorList_.at(i)->alive) {
            actorList_.at(i)->stepFrame(dt, nowMs);
        }
    }

    actorList_.erase(
        std::remove_if(actorList_.begin(), actorList_.end(),
            [](const std::unique_ptr<Actor>& a) { return !a->alive; }),
        actorList_.end()
    );
}

void Stage::renderLayer(SDL_Renderer* ren, DepthLayer targetLayer, float parallaxFactor)
{
    float offX = -camX * parallaxFactor;
    float offY = -camY * parallaxFactor;

    for (const auto& actor : actorList_) {
        if (actor->alive && actor->visible && actor->layer == targetLayer) {
            actor->render(ren, offX, offY);
        }
    }
}

bool Stage::findPrimaryGazeTarget(float& outX, float& outY, float& outCrossEye) const
{
    bool found = false;
    float bestCross = -1.0f;
    for (const auto& actor : actorList_) {
        if (!actor || !actor->alive || !actor->visible || !actor->gazeTarget) continue;
        if (!found || actor->gazeCrossEye > bestCross) {
            outX        = actor->x;
            outY        = actor->y;
            outCrossEye = actor->gazeCrossEye;
            bestCross   = actor->gazeCrossEye;
            found       = true;
        }
    }
    return found;
}
