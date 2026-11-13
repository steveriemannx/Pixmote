// Pixmote - basic value types. No dynamic allocation, no STL containers.
#pragma once

#include <cstdint>

namespace pixmote {

struct Point {
    int16_t x;
    int16_t y;

    constexpr Point() : x(0), y(0) {}
    constexpr Point(int16_t px, int16_t py) : x(px), y(py) {}

    // C++20 defaulted comparison: defines == and synthesizes !=.
    constexpr bool operator==(const Point&) const = default;
};

struct Rect {
    int16_t x;
    int16_t y;
    uint16_t w;
    uint16_t h;

    constexpr Rect() : x(0), y(0), w(0), h(0) {}
    constexpr Rect(int16_t px, int16_t py, uint16_t pw, uint16_t ph)
        : x(px), y(py), w(pw), h(ph) {}

    // C++20 defaulted comparison: defines == and synthesizes !=.
    constexpr bool operator==(const Rect&) const = default;

    constexpr int right() const {
        return static_cast<int>(x) + static_cast<int>(w);
    }
    constexpr int bottom() const {
        return static_cast<int>(y) + static_cast<int>(h);
    }
    constexpr bool empty() const { return w == 0 || h == 0; }

    constexpr bool contains(int px, int py) const {
        return px >= x && py >= y && px < right() && py < bottom();
    }

    constexpr bool intersects(const Rect& o) const {
        return !empty() && !o.empty() && x < o.right() && o.x < right() &&
               y < o.bottom() && o.y < bottom();
    }

    constexpr Rect intersect(const Rect& o) const {
        const int x0 = x > o.x ? x : o.x;
        const int y0 = y > o.y ? y : o.y;
        const int x1 = right() < o.right() ? right() : o.right();
        const int y1 = bottom() < o.bottom() ? bottom() : o.bottom();
        if (x1 <= x0 || y1 <= y0) {
            return Rect();
        }
        return Rect(static_cast<int16_t>(x0), static_cast<int16_t>(y0),
                    static_cast<uint16_t>(x1 - x0),
                    static_cast<uint16_t>(y1 - y0));
    }

    constexpr Rect unite(const Rect& o) const {
        if (empty()) {
            return o;
        }
        if (o.empty()) {
            return *this;
        }
        const int x0 = x < o.x ? x : o.x;
        const int y0 = y < o.y ? y : o.y;
        const int x1 = right() > o.right() ? right() : o.right();
        const int y1 = bottom() > o.bottom() ? bottom() : o.bottom();
        return Rect(static_cast<int16_t>(x0), static_cast<int16_t>(y0),
                    static_cast<uint16_t>(x1 - x0),
                    static_cast<uint16_t>(y1 - y0));
    }

    constexpr Rect inset(int amount) const {
        const int nw = static_cast<int>(w) - 2 * amount;
        const int nh = static_cast<int>(h) - 2 * amount;
        if (nw <= 0 || nh <= 0) {
            return Rect();
        }
        return Rect(static_cast<int16_t>(x + amount),
                    static_cast<int16_t>(y + amount),
                    static_cast<uint16_t>(nw), static_cast<uint16_t>(nh));
    }
};

// RGB565 color. Keeping this a 16-bit value is important on small MCUs.
class Color {
public:
    constexpr Color() : value_(0) {}
    constexpr explicit Color(uint16_t value) : value_(value) {}

    static constexpr Color rgb(uint8_t r, uint8_t g, uint8_t b) {
        return Color(static_cast<uint16_t>(
            ((static_cast<uint16_t>(r) & 0xF8u) << 8) |
            ((static_cast<uint16_t>(g) & 0xFCu) << 3) |
            (static_cast<uint16_t>(b) >> 3)));
    }

    constexpr uint16_t value() const { return value_; }
    constexpr uint8_t r() const {
        return static_cast<uint8_t>(((value_ >> 11) & 0x1Fu) * 255u / 31u);
    }
    constexpr uint8_t g() const {
        return static_cast<uint8_t>(((value_ >> 5) & 0x3Fu) * 255u / 63u);
    }
    constexpr uint8_t b() const {
        return static_cast<uint8_t>((value_ & 0x1Fu) * 255u / 31u);
    }

    // C++20 defaulted comparison: defines == and synthesizes !=.
    constexpr bool operator==(const Color&) const = default;

private:
    uint16_t value_;
};

// C++20 immediate functions: palette colors can only be produced at compile
// time, so they never cost any code space at run time.
namespace palette {
consteval Color black() { return Color(0x0000); }
consteval Color white() { return Color(0xFFFF); }
consteval Color red() { return Color(0xF800); }
consteval Color green() { return Color(0x07E0); }
consteval Color blue() { return Color(0x001F); }
}  // namespace palette

}  // namespace pixmote
