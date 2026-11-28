// ==============================================================================
// MakingFaces - Attachable Physics, Flow & Autonomous Agent Behaviors
// Architecture designed by Magnus Herrlin 2026
// ==============================================================================
#include "stage_behaviors.h"
#include "audio_engine.h"

// ==============================================================================
// Section A. Force Field, Perlin Noise & Flocking/Swarm Behaviors
// ==============================================================================

// ------------------------------------------------------------------------------
// 1. Apply constant directional acceleration (e.g., gravity or buoyancy)
// ------------------------------------------------------------------------------
void GravityBehavior::stepFrame(Actor& actor, float dt, uint32_t nowMs)
{
    (void)dt;
    (void)nowMs;
    actor.applyForce(gx_ * actor.mass, gy_ * actor.mass);
}

// ------------------------------------------------------------------------------
// 2. Steer actor along a smooth 3D Perlin noise flow field
// ------------------------------------------------------------------------------
void PerlinFlowBehavior::stepFrame(Actor& actor, float dt, uint32_t nowMs)
{
    (void)dt;
    float z = static_cast<float>(nowMs) * 0.001f * timeFreq_;
    float n = noise_.noise(actor.x * spatialFreq_, actor.y * spatialFreq_, z);
    float angle = n * 6.2831853f;
    actor.applyForce(std::cos(angle) * strength_, std::sin(angle) * strength_);
}

// ------------------------------------------------------------------------------
// 3. Craig Reynolds / Shiffman Boids flocking (separation, alignment, cohesion)
// ------------------------------------------------------------------------------
void SwarmBehavior::stepFrame(Actor& actor, float dt, uint32_t nowMs)
{
    (void)dt;
    (void)nowMs;

    float sepX = 0.0f, sepY = 0.0f;
    float aliX = 0.0f, aliY = 0.0f;
    float cohX = 0.0f, cohY = 0.0f;
    int   neighbors = 0;

    float neighSq = neighborRadius_ * neighborRadius_;
    float sepSq   = separationRadius_ * separationRadius_;

    for (const auto& otherPtr : stage_.actorList()) {
        const Actor* other = otherPtr.get();
        if (!other || other == &actor || !other->alive || other->groupId != actor.groupId) {
            continue;
        }
        float dx = actor.x - other->x;
        float dy = actor.y - other->y;
        float d2 = dx * dx + dy * dy;
        if (d2 > 0.001f && d2 < neighSq) {
            float d = std::sqrt(d2);
            if (d2 < sepSq) {
                sepX += (dx / d) * (separationRadius_ - d);
                sepY += (dy / d) * (separationRadius_ - d);
            }
            aliX += other->vx;
            aliY += other->vy;
            cohX += other->x;
            cohY += other->y;
            ++neighbors;
        }
    }

    float fx = sepX * 2.4f;
    float fy = sepY * 2.4f;

    if (neighbors > 0) {
        float invN = 1.0f / static_cast<float>(neighbors);
        aliX *= invN;
        aliY *= invN;
        cohX = (cohX * invN) - actor.x;
        cohY = (cohY * invN) - actor.y;
        fx += (aliX - actor.vx) * 0.65f + cohX * 0.55f;
        fy += (aliY - actor.vy) * 0.65f + cohY * 0.55f;
    }

    if (hasTarget_) {
        float tx = targetX_ - actor.x;
        float ty = targetY_ - actor.y;
        float td = std::sqrt(tx * tx + ty * ty);
        if (td > 1.0f) {
            fx += (tx / td) * steerForce_ * pullWeight_;
            fy += (ty / td) * steerForce_ * pullWeight_;
        }
    }

    float fMag = std::sqrt(fx * fx + fy * fy);
    if (fMag > steerForce_ && fMag > 0.001f) {
        fx = (fx / fMag) * steerForce_;
        fy = (fy / fMag) * steerForce_;
    }

    actor.applyForce(fx, fy);
    if (rotateToHeading_ && (std::fabs(actor.vx) > 4.0f || std::fabs(actor.vy) > 4.0f)) {
        actor.rotation = std::atan2(actor.vy, actor.vx) * 57.29578f;
    }
}

// ==============================================================================
// Section B. Bounds, Ground Surface, Lifetime Fade & Audio-Reactive Behaviors
// ==============================================================================

// ------------------------------------------------------------------------------
// 4. Wrap, bounce or cull actor at canvas boundaries
// ------------------------------------------------------------------------------
void BoundsBehavior::stepFrame(Actor& actor, float dt, uint32_t nowMs)
{
    (void)dt;
    (void)nowMs;

    if (policy_ == BoundsPolicy::NONE) {
        return;
    }
    if (policy_ == BoundsPolicy::WRAP) {
        if (actor.x < -margin_)          actor.x = width_ + margin_;
        if (actor.x > width_ + margin_)  actor.x = -margin_;
        if (actor.y < -margin_)          actor.y = height_ + margin_;
        if (actor.y > height_ + margin_) actor.y = -margin_;
    } else if (policy_ == BoundsPolicy::BOUNCE) {
        if (actor.x < 0.0f)    { actor.x = 0.0f;    actor.vx = std::fabs(actor.vx); }
        if (actor.x > width_)  { actor.x = width_;  actor.vx = -std::fabs(actor.vx); }
        if (actor.y < 0.0f)    { actor.y = 0.0f;    actor.vy = std::fabs(actor.vy); }
        if (actor.y > height_) { actor.y = height_; actor.vy = -std::fabs(actor.vy); }
    } else if (policy_ == BoundsPolicy::KILL_OUTSIDE) {
        if (actor.x < -margin_ || actor.x > width_ + margin_ ||
            actor.y < -margin_ || actor.y > height_ + margin_) {
            actor.alive = false;
        }
    }
}

// ------------------------------------------------------------------------------
// 5. Keep ground-crawling agents along a floor baseline with subtle scurrying
// ------------------------------------------------------------------------------
void GroundSurfaceBehavior::stepFrame(Actor& actor, float dt, uint32_t nowMs)
{
    (void)dt;
    if (actor.y > groundY_) {
        actor.y  = groundY_;
        actor.vy = 0.0f;
    }
    if (std::fabs(actor.vx) < 10.0f) {
        actor.vx = wanderSpeed_;
    }
    if (alignRotation_) {
        float phase = static_cast<float>(nowMs) * 0.025f + actor.x * 0.1f;
        actor.rotation = std::sin(phase) * 4.0f;
    }
}

// ------------------------------------------------------------------------------
// 6. Interpolate alpha & scale across finite actor lifetime (age / maxLife)
// ------------------------------------------------------------------------------
void LifeFadeBehavior::stepFrame(Actor& actor, float dt, uint32_t nowMs)
{
    (void)dt;
    (void)nowMs;
    if (actor.maxLife <= 0.0f) {
        return;
    }
    float t = std::max(0.0f, std::min(1.0f, actor.age / actor.maxLife));
    actor.alpha = static_cast<uint8_t>(std::round(startAlpha_ + (endAlpha_ - startAlpha_) * t));
    actor.scale = startScale_ + (endScale_ - startScale_) * t;
}

// ------------------------------------------------------------------------------
// 7. Pulse actor scale and apply upward lift from live stereo audio envelope
// ------------------------------------------------------------------------------
void AudioPulseBehavior::stepFrame(Actor& actor, float dt, uint32_t nowMs)
{
    (void)dt;
    (void)nowMs;
    float env = getAudioEnvelope();
    actor.scale = baseScale_ * (1.0f + env * scaleBoost_);
    if (env > 0.12f && liftForce_ != 0.0f) {
        actor.applyForce(0.0f, liftForce_ * env);
    }
}
