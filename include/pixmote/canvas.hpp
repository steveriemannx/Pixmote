// Pixmote - clipped RGB565 drawing surface.
#pragma once

#include "pixmote/font.hpp"
#include "pixmote/theme.hpp"
#include "pixmote/types.hpp"

namespace pixmote {

enum class Align : uint8_t {
    Left,
    Center,
    Right,
};

// A Canvas owns no pixels. It is a small view over a caller-provided band
// buffer, with an origin and a clip rectangle expressed in screen coordinates.
class Canvas {
public:
    Canvas(uint16_t* pixels, int stride, int width, int height, int originX,
           int originY, Rect clip, const Theme& theme);

    uint16_t* pixels() const { return pixels_; }
    int stride() const { return stride_; }
    int width() const { return width_; }
    int height() const { return height_; }
    int originX() const { return originX_; }
    int originY() const { return originY_; }
    Rect clip() const { return clip_; }
    const Theme& theme() const { return *theme_; }

    void setClip(const Rect& clip);
    void clear(Color color);
    void drawPixel(int x, int y, Color color);

    void fillRect(const Rect& rect, Color color);
    void drawRect(const Rect& rect, Color color, uint8_t thickness = 1);
    void fillRoundRect(const Rect& rect, uint16_t radius, Color color);
    void drawLine(int x0, int y0, int x1, int y1, Color color,
                  uint8_t thickness = 1);

    // RGB565 bitmap; each row is `w` pixels wide (no extra stride).
    void drawBitmap(int x, int y, const uint16_t* pixels, uint16_t w,
                    uint16_t h);
    // 1-bit mask, MSB first, rows padded to whole bytes; 0 bits are skipped.
    void drawBitmap1bpp(int x, int y, const uint8_t* bits, uint16_t w,
                        uint16_t h, Color color);

    // Draws top-left aligned text. '\n' starts a new line.
    void drawText(int x, int y, const char* text, Color color,
                  const Font& font = font5x7, uint8_t scale = 1);

    // Draws text vertically centered inside `box`.
    void drawText(const Rect& box, const char* text, Color color,
                  const Font& font = font5x7, uint8_t scale = 1,
                  Align align = Align::Center);

    int textWidth(const char* text, const Font& font = font5x7,
                  uint8_t scale = 1) const;
    int textHeight(const char* text, const Font& font = font5x7,
                   uint8_t scale = 1) const;

private:
    void putPixelFast(int x, int y, uint16_t color);

    uint16_t* pixels_;
    int stride_;
    int width_;
    int height_;
    int originX_;
    int originY_;
    Rect clip_;
    const Theme* theme_;
};

}  // namespace pixmote
