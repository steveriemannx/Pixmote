// Pixmote - colors and widget theme.
#pragma once

#include "pixmote/font.hpp"
#include "pixmote/types.hpp"

namespace pixmote {

struct Theme {
    Color background;
    Color surface;
    Color surface_alt;
    Color primary;
    Color primary_pressed;
    Color on_primary;
    Color text;
    Color text_muted;
    Color border;
    Color focus;
    Color success;
    uint8_t radius;
    uint8_t spacing;
    const Font* font;
};

const Theme& darkTheme();
const Theme& lightTheme();

inline const Theme& defaultTheme() {
    return darkTheme();
}

}  // namespace pixmote
