#include "pixmote/theme.hpp"

namespace pixmote {
namespace {

// C++20 designated initializers keep the palette readable while constexpr
// guarantees the themes are constant-initialized (normally placed in .rodata).
constexpr Theme kDarkTheme{
    .background = Color::rgb(9, 14, 23),
    .surface = Color::rgb(20, 29, 45),
    .surface_alt = Color::rgb(35, 48, 68),
    .primary = Color::rgb(45, 212, 191),
    .primary_pressed = Color::rgb(13, 148, 136),
    .on_primary = Color::rgb(4, 24, 27),
    .text = Color::rgb(232, 240, 248),
    .text_muted = Color::rgb(139, 155, 176),
    .border = Color::rgb(45, 60, 80),
    .focus = Color::rgb(251, 191, 36),
    .success = Color::rgb(74, 222, 128),
    .radius = 10,
    .spacing = 8,
    .font = &font5x7,
};

constexpr Theme kLightTheme{
    .background = Color::rgb(241, 245, 249),
    .surface = Color::rgb(255, 255, 255),
    .surface_alt = Color::rgb(226, 232, 240),
    .primary = Color::rgb(13, 148, 136),
    .primary_pressed = Color::rgb(15, 118, 110),
    .on_primary = Color::rgb(255, 255, 255),
    .text = Color::rgb(15, 23, 42),
    .text_muted = Color::rgb(71, 85, 105),
    .border = Color::rgb(203, 213, 225),
    .focus = Color::rgb(217, 119, 6),
    .success = Color::rgb(22, 163, 74),
    .radius = 10,
    .spacing = 8,
    .font = &font5x7,
};

}  // namespace

const Theme& darkTheme() {
    return kDarkTheme;
}

const Theme& lightTheme() {
    return kLightTheme;
}

}  // namespace pixmote
