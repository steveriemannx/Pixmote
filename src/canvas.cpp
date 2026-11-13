#include "pixmote/canvas.hpp"

#include "pixmote/config.hpp"

#if PIXMOTE_USE_LIBM
#include <cmath>
#endif

namespace pixmote {
namespace {

constexpr int absInt(int v) {
    return v < 0 ? -v : v;
}

constexpr int minInt(int a, int b) {
    return a < b ? a : b;
}

constexpr int maxInt(int a, int b) {
    return a > b ? a : b;
}

#if PIXMOTE_USE_LIBM
// Optional libm path for rounded corners.
int isqrt(int value) {
    if (value <= 0) {
        return 0;
    }
    return static_cast<int>(std::sqrt(static_cast<double>(value)));
}
#else
// Small integer square root (Newton). Used only for rounded corners.
int isqrt(int value) {
    if (value <= 0) {
        return 0;
    }
    int x = value;
    int y = (x + 1) / 2;
    while (y < x) {
        x = y;
        y = (x + value / x) / 2;
    }
    return x;
}
#endif

}  // namespace

Canvas::Canvas(uint16_t* pixels, int stride, int width, int height,
               int originX, int originY, Rect clip, const Theme& theme)
    : pixels_(pixels),
      stride_(stride),
      width_(width),
      height_(height),
      originX_(originX),
      originY_(originY),
      clip_(),
      theme_(&theme) {
    setClip(clip);
}

void Canvas::setClip(const Rect& clip) {
    const Rect bufferRect(static_cast<int16_t>(originX_),
                          static_cast<int16_t>(originY_),
                          static_cast<uint16_t>(width_),
                          static_cast<uint16_t>(height_));
    clip_ = clip.intersect(bufferRect);
}

void Canvas::putPixelFast(int x, int y, uint16_t color) {
    const int lx = x - originX_;
    const int ly = y - originY_;
    if (lx >= 0 && ly >= 0 && lx < width_ && ly < height_) [[likely]] {
        pixels_[ly * stride_ + lx] = color;
    }
}

void Canvas::drawPixel(int x, int y, Color color) {
    if (!clip_.contains(x, y)) {
        return;
    }
    putPixelFast(x, y, color.value());
}

void Canvas::clear(Color color) {
    fillRect(clip_, color);
}

void Canvas::fillRect(const Rect& rect, Color color) {
    const Rect r = rect.intersect(clip_);
    if (r.empty()) {
        return;
    }
    const int lx = r.x - originX_;
    const int ly = r.y - originY_;
    const uint16_t value = color.value();
    for (int row = 0; row < r.h; ++row) {
        uint16_t* dst = pixels_ + (ly + row) * stride_ + lx;
        for (int col = 0; col < r.w; ++col) {
            dst[col] = value;
        }
    }
}

void Canvas::drawRect(const Rect& rect, Color color, uint8_t thickness) {
    if (rect.empty() || thickness == 0) {
        return;
    }
    int t = thickness;
    const int limit = minInt(rect.w, rect.h) / 2;
    if (t > limit) {
        t = limit;
    }
    if (t <= 0) {
        fillRect(rect, color);
        return;
    }
    const uint16_t right = static_cast<uint16_t>(rect.w - static_cast<uint16_t>(t));
    const uint16_t bottom = static_cast<uint16_t>(rect.h - static_cast<uint16_t>(t));
    fillRect(Rect(rect.x, rect.y, rect.w, static_cast<uint16_t>(t)), color);
    fillRect(Rect(rect.x, static_cast<int16_t>(rect.y + bottom), rect.w,
                  static_cast<uint16_t>(t)),
             color);
    fillRect(Rect(rect.x, static_cast<int16_t>(rect.y + t),
                  static_cast<uint16_t>(t),
                  static_cast<uint16_t>(rect.h - 2 * t)),
             color);
    fillRect(Rect(static_cast<int16_t>(rect.x + right),
                  static_cast<int16_t>(rect.y + t),
                  static_cast<uint16_t>(t),
                  static_cast<uint16_t>(rect.h - 2 * t)),
             color);
}

void Canvas::fillRoundRect(const Rect& rect, uint16_t radius, Color color) {
    if (rect.empty()) {
        return;
    }
    int r = radius;
    const int limit = minInt(rect.w, rect.h) / 2;
    if (r > limit) {
        r = limit;
    }
    if (r <= 0) {
        fillRect(rect, color);
        return;
    }

    // Top and bottom quarter circles, emitted as horizontal spans.
    for (int row = 0; row < r; ++row) {
        const int dy = r - 1 - row;
        const int inset = r - isqrt(r * r - dy * dy);
        const int spanW = static_cast<int>(rect.w) - 2 * inset;
        if (spanW <= 0) {
            continue;
        }
        fillRect(Rect(static_cast<int16_t>(rect.x + inset),
                      static_cast<int16_t>(rect.y + row),
                      static_cast<uint16_t>(spanW), 1),
                 color);
        fillRect(Rect(static_cast<int16_t>(rect.x + inset),
                      static_cast<int16_t>(rect.y + rect.h - 1 - row),
                      static_cast<uint16_t>(spanW), 1),
                 color);
    }

    const int middle = static_cast<int>(rect.h) - 2 * r;
    if (middle > 0) {
        fillRect(Rect(rect.x, static_cast<int16_t>(rect.y + r), rect.w,
                      static_cast<uint16_t>(middle)),
                 color);
    }
}

void Canvas::drawLine(int x0, int y0, int x1, int y1, Color color,
                      uint8_t thickness) {
    if (thickness == 0) {
        return;
    }
    const int t = maxInt(1, static_cast<int>(thickness));
    const int half = t / 2;
    const int dx = absInt(x1 - x0);
    const int dy = -absInt(y1 - y0);
    const int sx = x0 < x1 ? 1 : -1;
    const int sy = y0 < y1 ? 1 : -1;
    int err = dx + dy;

    for (;;) {
        if (t == 1) {
            drawPixel(x0, y0, color);
        } else {
            for (int oy = -half; oy <= half; ++oy) {
                for (int ox = -half; ox <= half; ++ox) {
                    drawPixel(x0 + ox, y0 + oy, color);
                }
            }
        }
        if (x0 == x1 && y0 == y1) {
            break;
        }
        const int e2 = 2 * err;
        if (e2 >= dy) {
            err += dy;
            x0 += sx;
        }
        if (e2 <= dx) {
            err += dx;
            y0 += sy;
        }
    }
}

void Canvas::drawBitmap(int x, int y, const uint16_t* pixels, uint16_t w,
                        uint16_t h) {
    if (pixels == nullptr || w == 0 || h == 0) {
        return;
    }
    const Rect area(static_cast<int16_t>(x), static_cast<int16_t>(y), w, h);
    const Rect r = area.intersect(clip_);
    if (r.empty()) {
        return;
    }
    const int srcX = r.x - x;
    const int srcY = r.y - y;
    const int dstX = r.x - originX_;
    const int dstY = r.y - originY_;
    for (int row = 0; row < r.h; ++row) {
        const uint16_t* src = pixels + (srcY + row) * w + srcX;
        uint16_t* dst = pixels_ + (dstY + row) * stride_ + dstX;
        for (int col = 0; col < r.w; ++col) {
            dst[col] = src[col];
        }
    }
}

void Canvas::drawBitmap1bpp(int x, int y, const uint8_t* bits, uint16_t w,
                            uint16_t h, Color color) {
    if (bits == nullptr || w == 0 || h == 0) {
        return;
    }
    const Rect area(static_cast<int16_t>(x), static_cast<int16_t>(y), w, h);
    const Rect r = area.intersect(clip_);
    if (r.empty()) {
        return;
    }
    const int srcX = r.x - x;
    const int srcY = r.y - y;
    const int dstX = r.x - originX_;
    const int dstY = r.y - originY_;
    const int srcStride = (w + 7) / 8;
    const uint16_t value = color.value();
    for (int row = 0; row < r.h; ++row) {
        const uint8_t* src = bits + (srcY + row) * srcStride;
        uint16_t* dst = pixels_ + (dstY + row) * stride_ + dstX;
        for (int col = 0; col < r.w; ++col) {
            const int sx = srcX + col;
            if ((src[sx >> 3] &
                 static_cast<uint8_t>(0x80u >> (sx & 7))) != 0) {
                dst[col] = value;
            }
        }
    }
}

int Canvas::textWidth(const char* text, const Font& font, uint8_t scale) const {
    return measureTextWidth(font, text, scale);
}

int Canvas::textHeight(const char* text, const Font& font, uint8_t scale) const {
    return measureTextHeight(font, text, scale);
}

void Canvas::drawText(int x, int y, const char* text, Color color,
                      const Font& font, uint8_t scale) {
    if (text == nullptr || scale == 0) {
        return;
    }
    const int advance = font.advance() * scale;
    const int lineHeight = font.lineHeight() * scale;
    int cx = x;
    int cy = y;

    for (const char* p = text; *p != '\0'; ++p) {
        const unsigned char ch = static_cast<unsigned char>(*p);
        if (ch == '\n') {
            cx = x;
            cy += lineHeight;
            continue;
        }
        if (ch == '\r') {
            continue;
        }
        if (ch < 0x20) {
            cx += advance * 4;
            continue;
        }

        const uint8_t* glyph = font.glyph(ch);
        for (int col = 0; col < font.width; ++col) {
            const uint8_t bits = glyph[col];
            if (bits == 0) {
                continue;
            }
            for (int row = 0; row < font.height; ++row) {
                if ((bits & static_cast<uint8_t>(1u << row)) == 0) {
                    continue;
                }
                if (scale == 1) {
                    drawPixel(cx + col, cy + row, color);
                } else {
                    fillRect(Rect(static_cast<int16_t>(cx + col * scale),
                                  static_cast<int16_t>(cy + row * scale),
                                  scale, scale),
                             color);
                }
            }
        }
        cx += advance;
    }
}

void Canvas::drawText(const Rect& box, const char* text, Color color,
                      const Font& font, uint8_t scale, Align align) {
    const int width = textWidth(text, font, scale);
    const int height = textHeight(text, font, scale);
    if (width <= 0 || height <= 0) {
        return;
    }
    int x = box.x;
    if (align == Align::Center) {
        x = box.x + (static_cast<int>(box.w) - width) / 2;
    } else if (align == Align::Right) {
        x = box.x + static_cast<int>(box.w) - width;
    }
    const int y = box.y + (static_cast<int>(box.h) - height) / 2;
    drawText(x, y, text, color, font, scale);
}

}  // namespace pixmote
