// ==============================================================================
// MakingFaces - Concrete Renderable Actors, Shared Video & DMX Particle Emitter
// Architecture designed by Magnus Herrlin 2026
// ==============================================================================
#include "stage_actors.h"

// ==============================================================================
// Section A. SpriteActor, SeamlessFogActor, Special FX Actors & VideoActor
// ==============================================================================

// ------------------------------------------------------------------------------
// 1. Construct SpriteActor and query texture dimensions
// ------------------------------------------------------------------------------
SpriteActor::SpriteActor(SDL_Texture* tex, float px, float py, DepthLayer zLayer)
    : texture(tex)
{
    x     = px;
    y     = py;
    layer = zLayer;
    if (texture) {
        int tw = 0, th = 0;
        SDL_QueryTexture(texture, nullptr, nullptr, &tw, &th);
        w = static_cast<float>(tw);
        h = static_cast<float>(th);
    }
}

// ------------------------------------------------------------------------------
// 2. Render SpriteActor with multi-frame animation, autoFlipHead & wingFlap
// ------------------------------------------------------------------------------
void SpriteActor::render(SDL_Renderer* ren, float camOffsetX, float camOffsetY)
{
    SDL_Texture* activeTex = texture;
    if (!frames.empty()) {
        uint32_t ticks = SDL_GetTicks() + animPhaseMs;
        size_t idx = (ticks / std::max<uint32_t>(20, frameStepMs)) % frames.size();
        if (frames.at(idx)) {
            activeTex = frames.at(idx);
        }
    }
    if (!activeTex || alpha == 0) return;

    SDL_RendererFlip activeFlip = flip;
    double renderAngle = static_cast<double>(rotation);
    if (autoFlipHead) {
        activeFlip = (vx < -2.0f) ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE;
        float pitch = std::max(-16.0f, std::min(16.0f, vy * 0.06f));
        renderAngle = (activeFlip == SDL_FLIP_HORIZONTAL) ? -pitch : pitch;
    }

    float scX = scale;
    float scY = scale;
    if (wingFlap && frames.size() < 2) {
        float t = static_cast<float>(SDL_GetTicks() + animPhaseMs) * 0.022f;
        scY *= (0.72f + 0.32f * std::sin(t));
    }

    int dw = std::max(1, static_cast<int>(std::round(w * scX)));
    int dh = std::max(1, static_cast<int>(std::round(h * scY)));
    SDL_Rect dst{
        static_cast<int>(std::round(x + camOffsetX)) - dw / 2,
        static_cast<int>(std::round(y + camOffsetY)) - dh / 2,
        dw, dh
    };

    SDL_SetTextureColorMod(activeTex, tintR, tintG, tintB);
    SDL_SetTextureAlphaMod(activeTex, alpha);
    SDL_RenderCopyEx(ren, activeTex, nullptr, &dst, renderAngle, nullptr, activeFlip);
    SDL_SetTextureColorMod(activeTex, 255, 255, 255);
    SDL_SetTextureAlphaMod(activeTex, 255);
}

// ------------------------------------------------------------------------------
// 3. Construct SeamlessFogActor for edge-free Perlin-modulated fog & clouds
// ------------------------------------------------------------------------------
SeamlessFogActor::SeamlessFogActor(SDL_Texture* tex, const PerlinNoise* noise, DepthLayer zLayer, float speedX)
    : texture(tex), noiseGen(noise), baseSpeedX(speedX)
{
    layer   = zLayer;
    w       = 600.0f;
    h       = 800.0f;
    maxLife = -1.0f;
    alpha   = 0;
}

// ------------------------------------------------------------------------------
// 4. Smoothly fade fog alpha and modulate drift speed with 3D Perlin gusts
// ------------------------------------------------------------------------------
void SeamlessFogActor::stepFrame(float dt, uint32_t nowMs)
{
    Actor::stepFrame(dt, nowMs);

    float diff = static_cast<float>(targetAlpha) - curAlphaF_;
    curAlphaF_ += diff * std::min(1.0f, dt * 3.5f);
    alpha = static_cast<uint8_t>(std::round(std::max(0.0f, std::min(255.0f, curAlphaF_))));

    float t    = static_cast<float>(nowMs) * 0.00025f + static_cast<float>(layer) * 1.7f;
    float gust = noiseGen ? noiseGen->noise(t, static_cast<float>(layer) * 0.5f, 0.0f) : 0.0f;
    float lift = noiseGen ? noiseGen->noise(0.5f, t * 1.3f, 2.1f) : 0.0f;

    scrollX_ += (baseSpeedX + gust * perlinSwayX) * dt;
    waveOffY_ = lift * perlinSwayY;
}

// ------------------------------------------------------------------------------
// 5. Render seamless periodic tiles across 600x800 canvas (zero overlap seams)
// ------------------------------------------------------------------------------
void SeamlessFogActor::render(SDL_Renderer* ren, float camOffsetX, float camOffsetY)
{
    if (!texture || alpha == 0) return;

    int texW = 0, texH = 0;
    SDL_QueryTexture(texture, nullptr, nullptr, &texW, &texH);
    SDL_Rect src = (!periodicTex && texW > 12 && texH > 8) ? SDL_Rect{4, 2, texW - 8, texH - 4} : SDL_Rect{0, 0, texW, texH};

    int tileW   = static_cast<int>(std::round(600.0f * tileScaleW));
    int periodW = periodicTex ? tileW : (tileW * 2);
    float totalScroll = scrollX_ - camOffsetX;
    float wrapped = std::fmod(totalScroll, static_cast<float>(periodW));
    if (wrapped < 0.0f) wrapped += static_cast<float>(periodW);

    int x0 = -static_cast<int>(std::floor(wrapped));
    int y0 = static_cast<int>(std::round(waveOffY_ + camOffsetY * 0.25f));

    SDL_SetTextureColorMod(texture, tintR, tintG, tintB);
    SDL_SetTextureAlphaMod(texture, alpha);
    for (int i = 0; i < 3; ++i) {
        SDL_Rect dst{x0 + i * tileW, y0 - 12, tileW, 824};
        SDL_RendererFlip f = (!periodicTex && (i % 2) == 1) ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE;
        SDL_RenderCopyEx(ren, texture, &src, &dst, 0.0, nullptr, f);
    }
    SDL_SetTextureColorMod(texture, 255, 255, 255);
    SDL_SetTextureAlphaMod(texture, 255);
}
// ------------------------------------------------------------------------------
// 5.5 UfoActor: sweeps across sky, hovers over head, beams glow & exits
// ------------------------------------------------------------------------------
UfoActor::UfoActor(SDL_Texture* ufoTex, SDL_Texture* glowTex)
    : texture(ufoTex), beamGlow(glowTex)
{
    layer      = DepthLayer::MID;
    gazeTarget = true;
    w          = 136.0f;
    h          = 76.0f;
}

void UfoActor::stepFrame(float dt, uint32_t nowMs)
{
    Actor::stepFrame(dt, nowMs);
    if (depthMask != 0 && !wasActive_) {
        enableMs_ = nowMs;
    }
    wasActive_ = (depthMask != 0);

    if (depthMask == 0 || !texture) {
        visible      = false;
        gazeTarget   = false;
        gazeCrossEye = 0.0f;
        return;
    }

    if (depthMask & 8)      layer = DepthLayer::FRONT;
    else if (depthMask & 4) layer = DepthLayer::MID;
    else if (depthMask & 2) layer = DepthLayer::FAR;
    else                    layer = DepthLayer::SKY;

    const float period = 14.0f;
    float elapsed = static_cast<float>(nowMs - enableMs_) * 0.001f;
    int passIdx = static_cast<int>(std::floor(elapsed / period));
    float u = std::fmod(elapsed, period);
    bool fromLeft = (passIdx % 2) == 0;

    if (u < 9.5f) {
        visible    = true;
        gazeTarget = true;
        float t = static_cast<float>(nowMs) * 0.001f;
        if (u < 2.6f) {
            float p = u / 2.6f;
            float e = 1.0f - (1.0f - p) * (1.0f - p);
            float sx = fromLeft ? -110.0f : 710.0f;
            x = sx + (300.0f - sx) * e;
            y = 105.0f + std::sin(p * 6.28f) * 14.0f;
            rotation = (fromLeft ? 12.0f : -12.0f) * (1.0f - e);
        } else if (u < 6.9f) {
            x = 300.0f + std::sin(t * 2.1f) * 26.0f;
            y = 105.0f + std::cos(t * 3.0f) * 8.0f;
            rotation = std::sin(t * 2.1f) * 5.0f;
        } else {
            float p = (u - 6.9f) / 2.6f;
            float e = p * p;
            float ex = fromLeft ? 720.0f : -120.0f;
            x = 300.0f + (ex - 300.0f) * e;
            y = 105.0f - e * 45.0f;
            rotation = (fromLeft ? 15.0f : -15.0f) * e;
        }
    } else {
        visible    = false;
        gazeTarget = false;
    }
}

void UfoActor::render(SDL_Renderer* ren, float camOffsetX, float camOffsetY)
{
    if (!visible || !texture) return;
    int px = static_cast<int>(std::round(x + camOffsetX));
    int py = static_cast<int>(std::round(y + camOffsetY));

    float t = static_cast<float>(SDL_GetTicks()) * 0.004f;
    float pulse = 0.65f + 0.35f * std::sin(t);
    if (beamGlow) {
        SDL_SetTextureBlendMode(beamGlow, SDL_BLENDMODE_ADD);
        SDL_SetTextureColorMod(beamGlow, 70, 245, 190);
        SDL_SetTextureAlphaMod(beamGlow, static_cast<uint8_t>(std::round(135.0f * pulse)));
        SDL_Rect beamDst{px - 75, py + 12, 150, 240};
        SDL_RenderCopy(ren, beamGlow, nullptr, &beamDst);
        SDL_SetTextureColorMod(beamGlow, 255, 255, 255);
        SDL_SetTextureAlphaMod(beamGlow, 255);
        SDL_SetTextureBlendMode(beamGlow, SDL_BLENDMODE_BLEND);
    }

    int dw = static_cast<int>(std::round(w * scale));
    int dh = static_cast<int>(std::round(h * scale));
    SDL_Rect dst{px - dw / 2, py - dh / 2, dw, dh};
    SDL_RenderCopyEx(ren, texture, nullptr, &dst, static_cast<double>(rotation), nullptr, SDL_FLIP_NONE);
}

// ------------------------------------------------------------------------------
// 6. NoseFlyActor: flutters to nose tip, freezes wings spread & triggers cross-eye
// ------------------------------------------------------------------------------
NoseFlyActor::NoseFlyActor(SDL_Texture* spreadTex, SDL_Texture* foldedTex)
    : texWingsSpread(spreadTex), texWingsFolded(foldedTex)
{
    layer      = DepthLayer::FRONT;
    gazeTarget = true;
    w          = 68.0f;
    h          = 68.0f;
}

void NoseFlyActor::stepFrame(float dt, uint32_t nowMs)
{
    Actor::stepFrame(dt, nowMs);
    if (active && !wasActive_) {
        enableMs_ = nowMs;
    }
    wasActive_ = active;

    if (!active || !texWingsSpread) {
        visible      = false;
        gazeTarget   = false;
        gazeCrossEye = 0.0f;
        return;
    }

    const float period = 13.5f;
    float elapsed = static_cast<float>(nowMs - enableMs_) * 0.001f;
    int passIdx = static_cast<int>(std::floor(elapsed / period));
    float u = std::fmod(elapsed, period);
    bool goingRight = (passIdx % 2) == 0;
    flip_ = goingRight ? SDL_FLIP_NONE : SDL_FLIP_HORIZONTAL;

    if (u < 8.6f) {
        visible    = true;
        gazeTarget = true;
        float t    = static_cast<float>(nowMs) * 0.001f;

        if (u < 2.2f) {
            float p = u / 2.2f;
            float startX = goingRight ? -60.0f : 660.0f;
            x = startX + (noseX - startX) * p + std::sin(p * 12.56f) * 44.0f * (1.0f - p);
            y = 175.0f + (noseY - 175.0f) * p + std::cos(p * 9.42f) * 34.0f * (1.0f - p);
            rotation = std::sin(t * 24.0f) * 14.0f;
            landed_ = false;
            gazeCrossEye = std::max(0.0f, (p - 0.55f) / 0.45f) * 0.85f;
        } else if (u < 6.4f) {
            x = noseX + std::sin(t * 14.0f) * 0.5f;
            y = noseY;
            rotation = goingRight ? -12.0f : 12.0f;
            landed_ = true;
            gazeCrossEye = 1.0f;
        } else {
            float p = (u - 6.4f) / 2.2f;
            float endX = goingRight ? 660.0f : -60.0f;
            x = noseX + (endX - noseX) * p + std::sin(p * 12.56f) * 42.0f * p;
            y = noseY - p * 265.0f + std::cos(p * 9.42f) * 30.0f * p;
            rotation = std::sin(t * 24.0f) * 14.0f;
            landed_ = false;
            gazeCrossEye = std::max(0.0f, 1.0f - (p / 0.35f));
        }
    } else {
        visible      = false;
        gazeTarget   = false;
        gazeCrossEye = 0.0f;
        landed_      = false;
    }
}

void NoseFlyActor::render(SDL_Renderer* ren, float camOffsetX, float camOffsetY)
{
    if (!visible || !texWingsSpread) return;

    SDL_Texture* tex = texWingsSpread;
    if (!landed_ && texWingsFolded && (((SDL_GetTicks() / 38) % 2) == 1)) {
        tex = texWingsFolded;
    }

    int dw = static_cast<int>(std::round(w * scale));
    int dh = static_cast<int>(std::round(h * scale));
    SDL_Rect dst{
        static_cast<int>(std::round(x + camOffsetX)) - dw / 2,
        static_cast<int>(std::round(y + camOffsetY)) - dh / 2,
        dw, dh
    };
    SDL_RenderCopyEx(ren, tex, nullptr, &dst, static_cast<double>(rotation), nullptr, flip_);
}
// ------------------------------------------------------------------------------
// 7. FireflyActor: small glowing insect with green additive lava orb on abdomen
// ------------------------------------------------------------------------------
FireflyActor::FireflyActor(SDL_Texture* bodyTex1, SDL_Texture* bodyTex2, SDL_Texture* additiveGlowTex, float px, float py)
    : SpriteActor(bodyTex1, px, py, DepthLayer::FRONT), glowTex(additiveGlowTex)
{
    if (bodyTex1 && bodyTex2) {
        frames = {bodyTex1, bodyTex2};
        frameStepMs = 65;
    }
    w = 34.0f;
    h = 28.0f;
}

void FireflyActor::render(SDL_Renderer* ren, float camOffsetX, float camOffsetY)
{
    SpriteActor::render(ren, camOffsetX, camOffsetY);
    if (!glowTex || alpha == 0) return;

    float t = static_cast<float>(SDL_GetTicks() + animPhaseMs) * 0.006f + phase;
    float pulse = 0.45f + 0.55f * (0.5f + 0.5f * std::sin(t));
    float dir = (vx < -2.0f) ? 1.0f : -1.0f;
    int gx = static_cast<int>(std::round(x + camOffsetX + dir * (10.0f * scale)));
    int gy = static_cast<int>(std::round(y + camOffsetY + (4.0f * scale)));
    int gSz = std::max(10, static_cast<int>(std::round(34.0f * scale * (0.75f + 0.45f * pulse))));

    SDL_Rect gDst{gx - gSz / 2, gy - gSz / 2, gSz, gSz};
    SDL_SetTextureBlendMode(glowTex, SDL_BLENDMODE_ADD);
    SDL_SetTextureColorMod(glowTex, 140, 255, 70);
    SDL_SetTextureAlphaMod(glowTex, static_cast<uint8_t>(std::round(235.0f * pulse)));
    SDL_RenderCopy(ren, glowTex, nullptr, &gDst);
    SDL_SetTextureColorMod(glowTex, 255, 255, 255);
    SDL_SetTextureAlphaMod(glowTex, 255);
}

// ------------------------------------------------------------------------------
// 8. SpiderActor & SnakeActor: descending spider on thread & slithering snake
// ------------------------------------------------------------------------------
SpiderActor::SpiderActor(SDL_Texture* f1, SDL_Texture* f2, float topAnchorX)
    : tex1(f1), tex2(f2), anchorX(topAnchorX)
{
    layer = DepthLayer::FRONT;
    w = 96.0f;
    h = 96.0f;
}

void SpiderActor::stepFrame(float dt, uint32_t nowMs)
{
    Actor::stepFrame(dt, nowMs);
    if (!active || !tex1) { visible = false; gazeTarget = false; return; }
    float t = static_cast<float>(nowMs) * 0.001f;
    float u = std::fmod(t + 1.5f, 12.0f);
    if (u < 7.8f) {
        visible = true;
        gazeTarget = true;
        float drop = std::sin((u / 7.8f) * 3.1415926f);
        x = anchorX + std::sin(t * 2.2f) * 16.0f;
        y = -40.0f + drop * 355.0f;
        rotation = std::sin(t * 2.2f) * 8.0f;
    } else {
        visible = false;
        gazeTarget = false;
    }
}

void SpiderActor::render(SDL_Renderer* ren, float camOffsetX, float camOffsetY)
{
    if (!visible || !tex1) return;
    SDL_Texture* tex = (((SDL_GetTicks() / 160) % 2) == 1 && tex2) ? tex2 : tex1;
    int px = static_cast<int>(std::round(x + camOffsetX));
    int py = static_cast<int>(std::round(y + camOffsetY));
    SDL_SetRenderDrawColor(ren, 235, 240, 250, 195);
    SDL_RenderDrawLine(ren, static_cast<int>(std::round(anchorX + camOffsetX)), 0, px, py);
    SDL_Rect dst{px - 48, py - 48, 96, 96};
    SDL_RenderCopyEx(ren, tex, nullptr, &dst, static_cast<double>(rotation), nullptr, SDL_FLIP_NONE);
}

SnakeActor::SnakeActor(SDL_Texture* f1, SDL_Texture* f2)
    : tex1(f1), tex2(f2)
{
    layer = DepthLayer::FRONT;
    w = 140.0f;
    h = 360.0f;
}

void SnakeActor::stepFrame(float dt, uint32_t nowMs)
{
    Actor::stepFrame(dt, nowMs);
    if (!active || !tex1) { visible = false; gazeTarget = false; return; }
    float t = static_cast<float>(nowMs) * 0.001f;
    const float period = 11.0f;
    int passIdx = static_cast<int>(std::floor(t / period));
    float u = std::fmod(t, period);
    if (u < 6.4f) {
        visible = true;
        gazeTarget = true;
        float baseX = 110.0f + static_cast<float>((passIdx * 173) % 380);
        float dropP = std::sin((u / 6.4f) * 3.1415926f);
        x = baseX + std::sin(t * 3.8f) * 26.0f;
        y = -190.0f + dropP * 325.0f;
        rotation = std::sin(t * 3.8f) * 8.0f;
        groupId = (passIdx % 2 == 0) ? 1 : 2;
    } else {
        visible = false;
        gazeTarget = false;
    }
}

void SnakeActor::render(SDL_Renderer* ren, float camOffsetX, float camOffsetY)
{
    if (!visible || !tex1) return;
    SDL_Texture* tex = (((SDL_GetTicks() / 180) % 2) == 1 && tex2) ? tex2 : tex1;
    int px = static_cast<int>(std::round(x + camOffsetX));
    int py = static_cast<int>(std::round(y + camOffsetY));
    SDL_Rect dst{px - 70, py - 180, 140, 360};
    SDL_RenderCopyEx(ren, tex, nullptr, &dst, static_cast<double>(rotation), nullptr,
                     (groupId == 1) ? SDL_FLIP_NONE : SDL_FLIP_HORIZONTAL);
}
// ------------------------------------------------------------------------------
// 9. LightningActor, FireActor & StaticPropActor
// ------------------------------------------------------------------------------
LightningActor::LightningActor(SDL_Texture* f1, SDL_Texture* f2)
    : tex1(f1), tex2(f2)
{
    layer = DepthLayer::MID;
    w = 600.0f;
    h = 800.0f;
}

void LightningActor::stepFrame(float dt, uint32_t nowMs)
{
    Actor::stepFrame(dt, nowMs);
    if (!active || !tex1) { visible = false; return; }
    uint32_t strikeIdx = nowMs / 3200;
    uint32_t phase     = nowMs % 3200;
    if (phase < 190) {
        visible = true;
        int d = static_cast<int>((strikeIdx * 5 + 1) % 3);
        layer = (d == 0) ? DepthLayer::FAR : ((d == 1) ? DepthLayer::MID : DepthLayer::FRONT);
        alpha = (d == 0) ? 140 : ((d == 1) ? 195 : 255);
        activeTex_ = ((phase < 95) || !tex2) ? tex1 : tex2;
        shiftX_ = ((strikeIdx % 3) == 0) ? 0 : (((strikeIdx % 3) == 1) ? -90 : 95);
    } else {
        visible = false;
    }
}

void LightningActor::render(SDL_Renderer* ren, float camOffsetX, float camOffsetY)
{
    (void)camOffsetY;
    if (!visible || !activeTex_) return;
    SDL_SetTextureAlphaMod(activeTex_, alpha);
    SDL_Rect dst{shiftX_ + static_cast<int>(std::round(camOffsetX)), 0, 600, 800};
    SDL_RenderCopy(ren, activeTex_, nullptr, &dst);
    SDL_SetTextureAlphaMod(activeTex_, 255);
}

FireActor::FireActor(SDL_Texture* f1, SDL_Texture* f2)
    : tex1(f1), tex2(f2)
{
    layer = DepthLayer::FRONT;
    w = 600.0f;
    h = 800.0f;
}

void FireActor::stepFrame(float dt, uint32_t nowMs)
{
    Actor::stepFrame(dt, nowMs);
    visible = (active && tex1 != nullptr);
    if (visible) {
        float t = static_cast<float>(nowMs) * 0.001f;
        x = std::sin(t * 3.2f) * 15.0f - 24.0f;
    }
}

void FireActor::render(SDL_Renderer* ren, float camOffsetX, float camOffsetY)
{
    if (!visible || !tex1) return;
    SDL_Texture* tex = (((SDL_GetTicks() / 160) % 2) == 0 || !tex2) ? tex1 : tex2;
    SDL_Rect dst{static_cast<int>(std::round(x + camOffsetX)), static_cast<int>(std::round(96.0f + camOffsetY)), 648, 800};
    SDL_RenderCopy(ren, tex, nullptr, &dst);
}

StaticPropActor::StaticPropActor(SDL_Texture* tex, DepthLayer zLayer)
    : texture(tex)
{
    layer = zLayer;
    w = 600.0f;
    h = 800.0f;
}

void StaticPropActor::render(SDL_Renderer* ren, float camOffsetX, float camOffsetY)
{
    if (!active || !texture) return;
    SDL_Rect dst{static_cast<int>(std::round(camOffsetX)), static_cast<int>(std::round(camOffsetY)), 600, 800};
    SDL_RenderCopy(ren, texture, nullptr, &dst);
}

// ------------------------------------------------------------------------------
// 10. SharedVideoClip & VideoActor
// ------------------------------------------------------------------------------
SharedVideoClip::~SharedVideoClip() { close(); }

bool SharedVideoClip::open(SDL_Renderer* ren, const std::string& filePath, bool loopVideo)
{
    close();
    if (!ren || !cap_.open(filePath)) return false;
    loop_   = loopVideo;
    width_  = static_cast<int>(cap_.get(cv::CAP_PROP_FRAME_WIDTH));
    height_ = static_cast<int>(cap_.get(cv::CAP_PROP_FRAME_HEIGHT));
    double fps = cap_.get(cv::CAP_PROP_FPS);
    frameStepMs_ = static_cast<uint32_t>(std::round(1000.0 / ((fps >= 1.0 && fps <= 120.0) ? fps : 30.0)));
    if (width_ <= 0 || height_ <= 0) { close(); return false; }
    tex_ = SDL_CreateTexture(ren, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, width_, height_);
    if (!tex_) { close(); return false; }
    SDL_SetTextureBlendMode(tex_, SDL_BLENDMODE_BLEND);
    return true;
}

void SharedVideoClip::close(void)
{
    if (tex_) { SDL_DestroyTexture(tex_); tex_ = nullptr; }
    if (cap_.isOpened()) cap_.release();
    width_ = height_ = 0;
    lastUpdateMs_ = 0;
}

void SharedVideoClip::updateFrame(uint32_t nowMs)
{
    if (!tex_ || !cap_.isOpened()) return;
    if (lastUpdateMs_ != 0 && (nowMs - lastUpdateMs_) < frameStepMs_) return;
    lastUpdateMs_ = nowMs;
    cv::Mat frame, bgra;
    if (!cap_.read(frame) || frame.empty()) {
        if (!loop_) return;
        cap_.set(cv::CAP_PROP_POS_FRAMES, 0);
        if (!cap_.read(frame) || frame.empty()) return;
    }
    if (frame.channels() == 4) bgra = frame;
    else if (frame.channels() == 3) cv::cvtColor(frame, bgra, cv::COLOR_BGR2BGRA);
    else cv::cvtColor(frame, bgra, cv::COLOR_GRAY2BGRA);
    SDL_UpdateTexture(tex_, nullptr, bgra.data, static_cast<int>(bgra.step));
}

VideoActor::VideoActor(SharedVideoClip* videoClip, float px, float py, DepthLayer zLayer)
    : clip(videoClip)
{
    x = px; y = py; layer = zLayer;
    if (clip && clip->isOpen()) {
        w = static_cast<float>(clip->width());
        h = static_cast<float>(clip->height());
    }
}

void VideoActor::render(SDL_Renderer* ren, float camOffsetX, float camOffsetY)
{
    if (!clip || !clip->texture() || alpha == 0) return;
    SDL_Texture* tex = clip->texture();
    const SDL_Rect* srcPtr = (srcCrop.w > 0 && srcCrop.h > 0) ? &srcCrop : nullptr;
    int dw = std::max(1, static_cast<int>(std::round(w * scale)));
    int dh = std::max(1, static_cast<int>(std::round(h * scale)));
    SDL_Rect dst{static_cast<int>(std::round(x + camOffsetX)) - dw / 2, static_cast<int>(std::round(y + camOffsetY)) - dh / 2, dw, dh};
    SDL_SetTextureBlendMode(tex, additive ? SDL_BLENDMODE_ADD : SDL_BLENDMODE_BLEND);
    SDL_SetTextureColorMod(tex, tintR, tintG, tintB);
    SDL_SetTextureAlphaMod(tex, alpha);
    SDL_RenderCopyEx(ren, tex, srcPtr, &dst, static_cast<double>(rotation), nullptr, SDL_FLIP_NONE);
    SDL_SetTextureColorMod(tex, 255, 255, 255);
    SDL_SetTextureAlphaMod(tex, 255);
    SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND);
}
// ==============================================================================
// Section B. DMX-Controllable ParticleEmitter Implementation
// ==============================================================================

// ------------------------------------------------------------------------------
// 11. Construct ParticleEmitter bound to a PerlinNoise field and swarm group ID
// ------------------------------------------------------------------------------
ParticleEmitter::ParticleEmitter(PerlinNoise& noiseGen, int swarmGroupId)
    : noise_(noiseGen), groupId_(swarmGroupId) {}

void ParticleEmitter::setSpriteSource(SDL_Texture* tex)
{
    spriteTex_ = tex;
    animFrames_.clear();
    videoClip_ = nullptr;
}

void ParticleEmitter::setAnimatedFrames(const std::vector<SDL_Texture*>& animFrames, uint32_t stepMs)
{
    animFrames_  = animFrames;
    frameStepMs_ = stepMs;
    spriteTex_   = animFrames_.empty() ? nullptr : animFrames_.front();
    videoClip_   = nullptr;
}

void ParticleEmitter::setGlowTexture(SDL_Texture* additiveGlowTex)
{
    glowTex_ = additiveGlowTex;
}

void ParticleEmitter::setVideoSource(SharedVideoClip* clip, bool additiveBlend)
{
    videoClip_ = clip;
    videoAdd_  = additiveBlend;
    spriteTex_ = nullptr;
    animFrames_.clear();
}

// ------------------------------------------------------------------------------
// 12. Maintain live particle population & update shared tint/layer properties
// ------------------------------------------------------------------------------
void ParticleEmitter::updateFromDmx(Stage& stage, const EmitterDmxParams& params, float dt)
{
    auto layerForSlot = [&](int slotIdx) -> DepthLayer {
        if (params.depthMask == 0) return params.targetLayer;
        std::array<DepthLayer, 4> activeLayers{};
        int n = 0;
        if (params.depthMask & 1) activeLayers.at(n++) = DepthLayer::SKY;
        if (params.depthMask & 2) activeLayers.at(n++) = DepthLayer::FAR;
        if (params.depthMask & 4) activeLayers.at(n++) = DepthLayer::MID;
        if (params.depthMask & 8) activeLayers.at(n++) = DepthLayer::FRONT;
        return (n > 0) ? activeLayers.at(slotIdx % n) : params.targetLayer;
    };

    if (params.physicsMode == EmitterPhysicsMode::FIREWORKS && params.targetCount > 0 && spriteTex_) {
        burstTimer_ -= dt;
        if (burstTimer_ <= 0.0f) {
            EmitterDmxParams bp = params;
            bp.targetLayer = layerForSlot(static_cast<int>(rng_() % 4));
            spawnFireworkBurst(stage, bp);
            std::uniform_real_distribution<float> uDelay(0.45f, 1.35f);
            burstTimer_ = uDelay(rng_) / std::max(0.3f, params.turbulence);
        }
        return;
    }

    int activeCount = 0;
    for (const auto& actor : stage.actorList()) {
        if (!actor || !actor->alive || actor->groupId != groupId_) continue;
        if (activeCount >= params.targetCount) {
            actor->alive = false;
            continue;
        }
        actor->layer = layerForSlot(activeCount);
        if (params.physicsMode != EmitterPhysicsMode::AUTUMN_LEAVES) {
            actor->tintR = params.tintR;
            actor->tintG = params.tintG;
            actor->tintB = params.tintB;
        }
        ++activeCount;
    }

    if (activeCount >= params.targetCount || (!spriteTex_ && (!videoClip_ || !videoClip_->isOpen()))) return;

    int deficit = params.targetCount - activeCount;
    spawnAccum_ += std::max(12.0f, static_cast<float>(deficit) * 6.0f) * dt;
    int toSpawn = std::min(deficit, static_cast<int>(spawnAccum_));
    spawnAccum_ -= static_cast<float>(toSpawn);

    for (int i = 0; i < toSpawn; ++i) {
        EmitterDmxParams sp = params;
        sp.targetLayer = layerForSlot(activeCount + i);
        spawnOne(stage, sp);
    }
}

// ------------------------------------------------------------------------------
// 13. Spawn one SpriteActor, FireflyActor or VideoActor for the active mode
// ------------------------------------------------------------------------------
void ParticleEmitter::spawnOne(Stage& stage, const EmitterDmxParams& params)
{
    std::uniform_real_distribution<float> u01(0.0f, 1.0f);
    std::uniform_int_distribution<uint32_t> uPhase(0, 1000);
    float sx = (params.emitterX >= 0.0f) ? params.emitterX : (u01(rng_) * 600.0f);
    float sy = (params.emitterY >= 0.0f) ? params.emitterY
               : ((params.physicsMode == EmitterPhysicsMode::RISING_PLUME) ? 790.0f
                 : ((params.physicsMode == EmitterPhysicsMode::GRAVITY_FALL || params.physicsMode == EmitterPhysicsMode::AUTUMN_LEAVES) ? -24.0f : (u01(rng_) * 800.0f)));

    std::unique_ptr<Actor> actor;
    if (videoClip_ && videoClip_->isOpen()) {
        auto va = std::make_unique<VideoActor>(videoClip_, sx, sy, params.targetLayer);
        va->additive = videoAdd_;
        actor = std::move(va);
    } else if (params.physicsMode == EmitterPhysicsMode::FIREFLIES) {
        SDL_Texture* f1 = animFrames_.empty() ? spriteTex_ : animFrames_.at(0);
        SDL_Texture* f2 = (animFrames_.size() > 1) ? animFrames_.at(1) : nullptr;
        auto ff = std::make_unique<FireflyActor>(f1, f2, glowTex_, sx, sy);
        ff->phase      = u01(rng_) * 6.2831853f;
        ff->gazeTarget = params.gazeTarget;
        actor = std::move(ff);
    } else {
        auto sa = std::make_unique<SpriteActor>(spriteTex_, sx, sy, params.targetLayer);
        sa->frames       = animFrames_;
        sa->frameStepMs  = frameStepMs_;
        sa->animPhaseMs  = uPhase(rng_);
        sa->autoFlipHead = params.autoFlipHead;
        sa->wingFlap     = params.wingFlap;
        sa->gazeTarget   = params.gazeTarget;
        actor = std::move(sa);
    }

    actor->scale   = params.particleSize * (0.75f + u01(rng_) * 0.50f);
    actor->tintR   = params.tintR;
    actor->tintG   = params.tintG;
    actor->tintB   = params.tintB;
    actor->alpha   = params.alpha;
    actor->groupId = groupId_;

    float turb = std::max(0.1f, params.turbulence);
    switch (params.physicsMode) {
        case EmitterPhysicsMode::STILL:
            actor->maxLife = 4.0f + u01(rng_) * 3.0f;
            break;
        case EmitterPhysicsMode::GRAVITY_FALL:
            actor->vy = 60.0f + u01(rng_) * 90.0f * turb;
            actor->vx = (u01(rng_) - 0.5f) * 60.0f * turb;
            actor->maxLife = 6.0f;
            actor->addBehavior(std::make_unique<GravityBehavior>(0.0f, 220.0f * turb));
            actor->addBehavior(std::make_unique<PerlinFlowBehavior>(noise_, 0.006f, 0.4f, 95.0f * turb));
            actor->addBehavior(std::make_unique<BoundsBehavior>(600.0f, 800.0f, BoundsPolicy::KILL_OUTSIDE, 48.0f));
            break;
        case EmitterPhysicsMode::AUTUMN_LEAVES: {
            static constexpr std::array<std::array<uint8_t, 3>, 4> kLeafColors = {{
                {{245, 158, 11}}, {{234, 88, 12}}, {{220, 38, 38}}, {{202, 138, 4}}
            }};
            const auto& c = kLeafColors.at(uPhase(rng_) % 4);
            actor->tintR = c.at(0);
            actor->tintG = c.at(1);
            actor->tintB = c.at(2);
            actor->vy = 35.0f + u01(rng_) * 45.0f * turb;
            actor->vx = (u01(rng_) - 0.5f) * 80.0f * turb;
            actor->rotation = (u01(rng_) - 0.5f) * 360.0f;
            actor->drag = 0.85f;
            actor->maxLife = 8.5f;
            actor->addBehavior(std::make_unique<GravityBehavior>(0.0f, 75.0f));
            actor->addBehavior(std::make_unique<PerlinFlowBehavior>(noise_, 0.008f, 0.65f, 210.0f * turb));
            actor->addBehavior(std::make_unique<BoundsBehavior>(600.0f, 800.0f, BoundsPolicy::KILL_OUTSIDE, 48.0f));
            break;
        }
        case EmitterPhysicsMode::RISING_PLUME:
            actor->vy = -(70.0f + u01(rng_) * 90.0f * turb);
            actor->vx = (u01(rng_) - 0.5f) * 45.0f * turb;
            actor->maxLife = 2.4f + u01(rng_) * 1.6f;
            actor->addBehavior(std::make_unique<GravityBehavior>(0.0f, -140.0f * turb));
            actor->addBehavior(std::make_unique<PerlinFlowBehavior>(noise_, 0.009f, 0.65f, 180.0f * turb));
            actor->addBehavior(std::make_unique<LifeFadeBehavior>(params.alpha, 0, actor->scale * 0.6f, actor->scale * 1.45f));
            break;
        case EmitterPhysicsMode::PERLIN_SWIRL:
        case EmitterPhysicsMode::FIREFLIES:
            actor->maxSpeed = 220.0f * turb;
            actor->drag = 0.50f;
            actor->maxLife = 7.0f + u01(rng_) * 5.0f;
            actor->addBehavior(std::make_unique<PerlinFlowBehavior>(noise_, 0.005f, 0.35f, 240.0f * turb));
            actor->addBehavior(std::make_unique<BoundsBehavior>(600.0f, 800.0f, BoundsPolicy::WRAP, 48.0f));
            break;
        case EmitterPhysicsMode::SWARM_FLOCK:
            actor->vx = (u01(rng_) - 0.5f) * 180.0f;
            actor->vy = (u01(rng_) - 0.5f) * 180.0f;
            actor->maxSpeed = 280.0f * turb;
            actor->maxLife = 10.0f + u01(rng_) * 6.0f;
            actor->addBehavior(std::make_unique<SwarmBehavior>(stage, 95.0f, 30.0f, 260.0f * turb, !params.autoFlipHead));
            actor->addBehavior(std::make_unique<BoundsBehavior>(600.0f, 800.0f, BoundsPolicy::WRAP, 40.0f));
            break;
        case EmitterPhysicsMode::GROUND_CRAWL:
            actor->vx = ((u01(rng_) < 0.5f) ? -1.0f : 1.0f) * (80.0f + u01(rng_) * 70.0f * turb);
            actor->maxLife = 9.0f + u01(rng_) * 6.0f;
            actor->addBehavior(std::make_unique<GroundSurfaceBehavior>(755.0f, 110.0f * turb, true));
            actor->addBehavior(std::make_unique<BoundsBehavior>(600.0f, 800.0f, BoundsPolicy::WRAP, 40.0f));
            break;
        case EmitterPhysicsMode::AUDIO_PULSE:
            actor->maxSpeed = 220.0f * turb;
            actor->drag = 1.2f;
            actor->maxLife = 5.0f + u01(rng_) * 3.0f;
            actor->addBehavior(std::make_unique<AudioPulseBehavior>(0.55f, -320.0f * turb));
            actor->addBehavior(std::make_unique<PerlinFlowBehavior>(noise_, 0.007f, 0.4f, 140.0f * turb));
            actor->addBehavior(std::make_unique<BoundsBehavior>(600.0f, 800.0f, BoundsPolicy::BOUNCE, 20.0f));
            break;
        case EmitterPhysicsMode::FIREWORKS:
            break;
    }

    stage.addActor(std::move(actor));
}

// ------------------------------------------------------------------------------
// 15. Spawn a radial firework burst of glowing additive sparks
// ------------------------------------------------------------------------------
void ParticleEmitter::spawnFireworkBurst(Stage& stage, const EmitterDmxParams& params)
{
    static constexpr std::array<std::array<uint8_t, 3>, 6> kPal = {{
        {{255, 80, 90}}, {{80, 220, 255}}, {{255, 225, 75}}, {{190, 110, 255}}, {{90, 255, 145}}, {{255, 150, 60}}
    }};
    std::uniform_real_distribution<float> u01(0.0f, 1.0f);
    std::uniform_int_distribution<size_t> uCol(0, kPal.size() - 1);

    float cx = (params.emitterX >= 0.0f) ? params.emitterX : (90.0f + u01(rng_) * 420.0f);
    float cy = (params.emitterY >= 0.0f) ? params.emitterY : (90.0f + u01(rng_) * 260.0f);
    const auto& col = kPal.at(uCol(rng_));
    int sparks = std::max(16, std::min(64, params.targetCount));

    for (int i = 0; i < sparks; ++i) {
        float ang = (static_cast<float>(i) / static_cast<float>(sparks)) * 6.2831853f + (u01(rng_) - 0.5f) * 0.18f;
        float spd = (120.0f + u01(rng_) * 180.0f) * std::max(0.4f, params.turbulence);
        auto sa = std::make_unique<SpriteActor>(spriteTex_, cx, cy, params.targetLayer);
        sa->vx = std::cos(ang) * spd;
        sa->vy = std::sin(ang) * spd;
        sa->drag = 1.65f;
        sa->tintR = col.at(0);
        sa->tintG = col.at(1);
        sa->tintB = col.at(2);
        sa->groupId = groupId_;
        sa->maxLife = 1.1f + u01(rng_) * 0.65f;
        float sz = params.particleSize * (0.45f + u01(rng_) * 0.45f);
        sa->addBehavior(std::make_unique<GravityBehavior>(0.0f, 180.0f));
        sa->addBehavior(std::make_unique<LifeFadeBehavior>(255, 0, sz, sz * 0.15f));
        stage.addActor(std::move(sa));
    }
}
