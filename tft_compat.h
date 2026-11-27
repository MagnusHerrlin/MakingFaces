// ==============================================================================
// MakingFaces - Arduino & TFT_eSPI Software Framebuffer Compatibility Layer
// Architecture designed by Magnus Herrlin 2026
// ==============================================================================
#pragma once

// ==============================================================================
// 1. INCLUDES AND DEFINITIONS
// ==============================================================================
#include <cstdint>
#include <cstring>
#include <cstdlib>
#include <cmath>
#include <algorithm>
#include <cstdio>
#include <chrono>

#ifndef PROGMEM
#define PROGMEM
#endif

#define pgm_read_byte(addr)  (*(const uint8_t *)(addr))
#define pgm_read_word(addr)  (*(const uint16_t *)(addr))
#define pgm_read_dword(addr) (*(const uint32_t *)(addr))

#ifndef PI
#define PI 3.14159265358979323846f
#endif

#define TFT_BLACK 0x0000
#define TFT_WHITE 0xFFFF

// ==============================================================================
// 2. ARDUINO MATH AND TIMING HELPERS
// ==============================================================================
template <typename T>
inline T constrain(T val, T minVal, T maxVal) {
    return (val < minVal) ? minVal : ((val > maxVal) ? maxVal : val);
}

inline int32_t map(int32_t x, int32_t in_min, int32_t in_max, int32_t out_min, int32_t out_max) {
    if (in_max == in_min) return out_min;
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

inline long random(long maxVal) {
    return (maxVal <= 0) ? 0 : (std::rand() % maxVal);
}

inline long random(long minVal, long maxVal) {
    return (maxVal <= minVal) ? minVal : (minVal + (std::rand() % (maxVal - minVal)));
}

inline uint32_t millis(void) {
    static const auto startTime = std::chrono::steady_clock::now();
    auto now = std::chrono::steady_clock::now();
    return static_cast<uint32_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(now - startTime).count()
    );
}

inline void yield(void) {}

// ==============================================================================
// 3. SOFTWARE FRAMEBUFFER EMULATING TFT_eSPI (600x800 RGB565 -> ARGB8888)
// ==============================================================================
class TFT_eSPI {
public:
    static constexpr int FB_WIDTH  = 600;
    static constexpr int FB_HEIGHT = 800;

    uint16_t framebuffer[FB_WIDTH * FB_HEIGHT];
    uint32_t argb_buffer[FB_WIDTH * FB_HEIGHT];
    bool     customArgbReady = false;

    TFT_eSPI() {
        std::memset(framebuffer, 0, sizeof(framebuffer));
        std::memset(argb_buffer, 0, sizeof(argb_buffer));
    }
    void init() { fillScreen(TFT_BLACK); }
    void setRotation(uint8_t) {}
    int16_t width() const { return FB_WIDTH; }
    int16_t height() const { return FB_HEIGHT; }

    void fillScreen(uint16_t color) {
        customArgbReady = false;
        if (color == 0) {
            std::memset(framebuffer, 0, sizeof(framebuffer));
            std::memset(argb_buffer, 0, sizeof(argb_buffer));
        } else {
            std::fill_n(framebuffer, FB_WIDTH * FB_HEIGHT, color);
        }
    }

    uint16_t color565(uint8_t r, uint8_t g, uint8_t b) const {
        return static_cast<uint16_t>(((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3));
    }

    void drawPixel(int32_t x, int32_t y, uint16_t color) {
        if (x >= 0 && x < FB_WIDTH && y >= 0 && y < FB_HEIGHT) {
            framebuffer[y * FB_WIDTH + x] = color;
        }
    }

    void drawFastVLine(int32_t x, int32_t y, int32_t h, uint16_t color) {
        if (x < 0 || x >= FB_WIDTH || h <= 0) return;
        int32_t y0 = std::max<int32_t>(0, y);
        int32_t y1 = std::min<int32_t>(FB_HEIGHT, y + h);
        for (int32_t py = y0; py < y1; ++py) {
            framebuffer[py * FB_WIDTH + x] = color;
        }
    }

    void drawFastHLine(int32_t x, int32_t y, int32_t w, uint16_t color) {
        if (y < 0 || y >= FB_HEIGHT || w <= 0) return;
        int32_t x0 = std::max<int32_t>(0, x);
        int32_t x1 = std::min<int32_t>(FB_WIDTH, x + w);
        std::fill_n(&framebuffer[y * FB_WIDTH + x0], x1 - x0, color);
    }

    void fillRect(int32_t x, int32_t y, int32_t w, int32_t h, uint16_t color) {
        if (w <= 0 || h <= 0) return;
        int32_t x0 = std::max<int32_t>(0, x);
        int32_t y0 = std::max<int32_t>(0, y);
        int32_t x1 = std::min<int32_t>(FB_WIDTH, x + w);
        int32_t y1 = std::min<int32_t>(FB_HEIGHT, y + h);
        if (x0 >= x1 || y0 >= y1) return;
        for (int32_t py = y0; py < y1; ++py) {
            std::fill_n(&framebuffer[py * FB_WIDTH + x0], x1 - x0, color);
        }
    }

    void fillRoundRect(int32_t x, int32_t y, int32_t w, int32_t h, int32_t r, uint16_t color) {
        if (w <= 0 || h <= 0) return;
        int32_t maxR = std::min(w, h) / 2;
        if (r > maxR) r = maxR;
        if (r <= 0) {
            fillRect(x, y, w, h, color);
            return;
        }

        fillRect(x, y + r, w, h - 2 * r, color);

        int32_t f     = 1 - r;
        int32_t ddF_x = 1;
        int32_t ddF_y = -2 * r;
        int32_t cx    = 0;
        int32_t cy    = r;

        while (cx <= cy) {
            drawFastHLine(x + r - cy, y + r - cx,         w - 2 * (r - cy), color);
            drawFastHLine(x + r - cy, y + h - 1 - r + cx, w - 2 * (r - cy), color);
            drawFastHLine(x + r - cx, y + r - cy,         w - 2 * (r - cx), color);
            drawFastHLine(x + r - cx, y + h - 1 - r + cy, w - 2 * (r - cx), color);

            if (f >= 0) {
                cy--;
                ddF_y += 2;
                f += ddF_y;
            }
            cx++;
            ddF_x += 2;
            f += ddF_x;
        }
    }
    void fillTriangle(int32_t x0, int32_t y0, int32_t x1, int32_t y1, int32_t x2, int32_t y2, uint16_t color) {
        if (y0 > y1) { std::swap(y0, y1); std::swap(x0, x1); }
        if (y1 > y2) { std::swap(y1, y2); std::swap(x1, x2); }
        if (y0 > y1) { std::swap(y0, y1); std::swap(x0, x1); }

        if (y0 == y2) {
            int32_t minX = std::min({x0, x1, x2});
            int32_t maxX = std::max({x0, x1, x2});
            drawFastHLine(minX, y0, maxX - minX + 1, color);
            return;
        }

        int32_t dx01 = x1 - x0, dy01 = y1 - y0;
        int32_t dx02 = x2 - x0, dy02 = y2 - y0;
        int32_t dx12 = x2 - x1, dy12 = y2 - y1;
        int32_t sa = 0, sb = 0;
        int32_t last = (y1 == y2) ? y1 : (y1 - 1);

        int32_t y = y0;
        for (; y <= last; ++y) {
            int32_t a = x0 + ((dy01 != 0) ? (sa / dy01) : 0);
            int32_t b = x0 + ((dy02 != 0) ? (sb / dy02) : 0);
            sa += dx01;
            sb += dx02;
            if (a > b) std::swap(a, b);
            drawFastHLine(a, y, b - a + 1, color);
        }

        sa = dx12 * (y - y1);
        sb = dx02 * (y - y0);
        for (; y <= y2; ++y) {
            int32_t a = x1 + ((dy12 != 0) ? (sa / dy12) : 0);
            int32_t b = x0 + ((dy02 != 0) ? (sb / dy02) : 0);
            sa += dx12;
            sb += dx02;
            if (a > b) std::swap(a, b);
            drawFastHLine(a, y, b - a + 1, color);
        }
    }

    void startWrite() {}
    void endWrite() {}

    void setAddrWindow(int32_t x, int32_t y, int32_t w, int32_t h) {
        winX_ = x; winY_ = y; winW_ = w; winH_ = h;
        curX_ = 0; curY_ = 0;
    }

    void pushPixels(const uint16_t* data, uint32_t len) {
        for (uint32_t i = 0; i < len; ++i) {
            if (curY_ >= winH_) break;
            int32_t dstX = winX_ + curX_;
            int32_t dstY = winY_ + curY_;
            if (dstX >= 0 && dstX < FB_WIDTH && dstY >= 0 && dstY < FB_HEIGHT) {
                uint16_t swapped = data[i];
                framebuffer[dstY * FB_WIDTH + dstX] = static_cast<uint16_t>((swapped >> 8) | (swapped << 8));
            }
            if (++curX_ >= winW_) {
                curX_ = 0;
                curY_++;
            }
        }
    }

    const uint32_t* updateArgbBuffer(bool transparentKey = true) {
        if (customArgbReady) return argb_buffer;

        for (int i = 0; i < FB_WIDTH * FB_HEIGHT; ++i) {
            uint16_t c = framebuffer[i];
            if (transparentKey && c == 0) {
                argb_buffer[i] = 0x00000000;
            } else {
                uint8_t r = static_cast<uint8_t>(((c >> 11) & 0x1F) * 255 / 31);
                uint8_t g = static_cast<uint8_t>(((c >> 5)  & 0x3F) * 255 / 63);
                uint8_t b = static_cast<uint8_t>((c         & 0x1F) * 255 / 31);
                argb_buffer[i] = (0xFFu << 24) | (static_cast<uint32_t>(r) << 16) |
                                 (static_cast<uint32_t>(g) << 8) | static_cast<uint32_t>(b);
            }
        }
        return argb_buffer;
    }

private:
    int32_t winX_ = 0, winY_ = 0, winW_ = 0, winH_ = 0;
    int32_t curX_ = 0, curY_ = 0;
};

// ==============================================================================
// 4. SOFTWARE SPRITE EMULATING TFT_eSprite
// ==============================================================================
class TFT_eSprite {
public:
    explicit TFT_eSprite(TFT_eSPI* parent) : parent_(parent) {}
    ~TFT_eSprite() { delete[] buf_; }

    void setColorDepth(int8_t) {}

    void* createSprite(int16_t w, int16_t h) {
        delete[] buf_;
        w_ = w; h_ = h;
        buf_ = new uint16_t[w_ * h_];
        fillSprite(TFT_BLACK);
        return buf_;
    }

    void fillSprite(uint16_t color) {
        if (!buf_) return;
        if (color == 0) std::memset(buf_, 0, w_ * h_ * sizeof(uint16_t));
        else            std::fill_n(buf_, w_ * h_, color);
    }

    void* getPointer() { return buf_; }

    uint16_t color565(uint8_t r, uint8_t g, uint8_t b) const {
        return static_cast<uint16_t>(((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3));
    }

    void pushSprite(int32_t x, int32_t y) {
        if (!buf_ || !parent_) return;
        for (int32_t sy = 0; sy < h_; ++sy) {
            int32_t dy = y + sy;
            if (dy < 0 || dy >= TFT_eSPI::FB_HEIGHT) continue;
            for (int32_t sx = 0; sx < w_; ++sx) {
                int32_t dx = x + sx;
                if (dx < 0 || dx >= TFT_eSPI::FB_WIDTH) continue;
                uint16_t swapped = buf_[sy * w_ + sx];
                parent_->framebuffer[dy * TFT_eSPI::FB_WIDTH + dx] =
                    static_cast<uint16_t>((swapped >> 8) | (swapped << 8));
            }
        }
    }

private:
    TFT_eSPI* parent_ = nullptr;
    uint16_t* buf_    = nullptr;
    int16_t   w_      = 0;
    int16_t   h_      = 0;
};
