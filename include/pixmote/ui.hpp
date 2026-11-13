// Pixmote - screen, damage tracking, input routing and band rendering.
#pragma once

#include <concepts>
#include <cstddef>
#include <type_traits>

#include "pixmote/config.hpp"
#include "pixmote/theme.hpp"
#include "pixmote/widget.hpp"

namespace pixmote {

// Display driver contract. `flush` receives an RGB565 rectangle and must
// consume/copy it before returning. The pointer is only valid during the call.
struct Display {
    int16_t width;
    int16_t height;
    void* context;
    void (*flush)(void* context, int16_t x, int16_t y, int16_t w, int16_t h,
                  const uint16_t* pixels, uint16_t stride);
};

// C++20 concept: the array overload only accepts a genuine non-empty uint16_t
// array, never a pointer or a buffer of the wrong pixel type.
template <typename T>
concept BandBuffer = std::is_array_v<T> &&
                     std::same_as<std::remove_extent_t<T>, uint16_t> &&
                     (std::extent_v<T> > 0);

class Ui {
public:
    // `bandBuffer` must hold at least display.width * bandHeight pixels.
    // A band height of 8..32 is usually a good RAM/CPU tradeoff.
    Ui(Display& display, Widget& root, uint16_t* bandBuffer,
       uint32_t bandCapacityPixels, uint16_t bandHeight,
       const Theme& theme = defaultTheme());

    template <BandBuffer Buffer>
    Ui(Display& display, Widget& root, Buffer& bandBuffer,
       uint16_t bandHeight, const Theme& theme = defaultTheme())
        : Ui(display, root, bandBuffer,
             static_cast<uint32_t>(std::extent_v<Buffer>), bandHeight, theme) {}

    ~Ui();

    Ui(const Ui&) = delete;
    Ui& operator=(const Ui&) = delete;

    [[nodiscard("check the band buffer size and display dimensions")]]
    bool valid() const { return valid_; }
    Display& display() { return *display_; }
    const Display& display() const { return *display_; }
    Widget* root() const { return root_; }
    void setRoot(Widget& root);

    void setTheme(const Theme& theme);
    const Theme& theme() const { return *theme_; }

    void invalidate(const Rect& area);
    void invalidateAll();
    void render();
    uint8_t dirtyCount() const { return dirtyCount_; }
    Rect dirtyRect(uint8_t index) const;

    bool pointerDown(int16_t x, int16_t y);
    bool pointerMove(int16_t x, int16_t y);
    bool pointerUp(int16_t x, int16_t y);
    void pointerCancel();
    bool pointerCaptured() const { return pointerTarget_ != nullptr; }

    bool keyDown(Key key);
    // Routes a printable ASCII character to the focused widget.
    bool inputChar(char c);
    void focus(Widget* widget, bool showRing = true);
    void clearFocus();
    void focusNext();
    void focusPrevious();
    Widget* focused() const { return focused_; }

    uint16_t bandHeight() const { return bandHeight_; }

private:
    friend class Widget;

    void widgetDetached(const Widget* widget);
    void attachRoot(Widget& root);
    void renderRegion(const Rect& region);
    Widget* hitTest(Widget* widget, int x, int y) const;
    void setFocusFlags(Widget* widget, bool showRing);

    Display* display_;
    Widget* root_;
    uint16_t* band_;
    uint16_t bandHeight_;
    const Theme* theme_;
    Rect dirty_[PIXMOTE_MAX_DIRTY_RECTS];
    uint8_t dirtyCount_;
    Widget* pointerTarget_;
    Widget* focused_;
    bool valid_;
};

}  // namespace pixmote
