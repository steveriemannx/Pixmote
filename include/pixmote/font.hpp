// Pixmote - fixed-cell bitmap font.
#pragma once

#include <cstdint>

namespace pixmote {

// A compact, fixed-cell font.
//
// Glyph data is column-major: each glyph occupies `width` bytes and bit 0 of
// each byte is the top pixel. Only a contiguous byte range [first, last] is
// stored. This format is intentionally trivial so fonts can live in flash.
struct Font {
    const uint8_t* glyphs;
    uint8_t first;
    uint8_t last;
    uint8_t width;
    uint8_t height;
    uint8_t spacing;
    uint8_t lineGap;
    uint8_t flags;  // reserved, must be 0

    constexpr int advance() const {
        return static_cast<int>(width) + static_cast<int>(spacing);
    }
    constexpr int lineHeight() const {
        return static_cast<int>(height) + static_cast<int>(lineGap);
    }
    constexpr const uint8_t* glyph(uint8_t c) const {
        uint8_t index = c;
        if (c < first || c > last) {
            index = static_cast<uint8_t>('?');
        }
        if (index < first || index > last) {
            index = first;
        }
        return glyphs + static_cast<uint16_t>(index - first) * width;
    }
};

// Built-in 5x7 ASCII font, printable range 0x20..0x7E.
extern const Font font5x7;

// Measurement helpers. They do not need a drawing surface.
[[nodiscard]] int measureTextWidth(const Font& font, const char* text,
                                   uint8_t scale = 1);
[[nodiscard]] int measureTextHeight(const Font& font, const char* text,
                                    uint8_t scale = 1);

}  // namespace pixmote
