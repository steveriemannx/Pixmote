# API Reference

All symbols live in `namespace pixmote` and are available through the umbrella
header:

```cpp
#include "pixmote.hpp"
using namespace pixmote;
```

---

## 1. Basic types

### 1.1 `Point`

```cpp
struct Point {
    int16_t x;
    int16_t y;

    constexpr Point();
    constexpr Point(int16_t px, int16_t py);
    constexpr bool operator==(const Point&) const = default;  // != synthesized
};
```

### 1.2 `Rect`

```cpp
struct Rect {
    int16_t x;
    int16_t y;
    uint16_t w;
    uint16_t h;

    constexpr Rect();
    constexpr Rect(int16_t px, int16_t py, uint16_t pw, uint16_t ph);
    constexpr bool operator==(const Rect&) const = default;  // != synthesized

    constexpr int right() const;                   // x + w
    constexpr int bottom() const;                  // y + h
    constexpr bool empty() const;                  // w == 0 || h == 0
    constexpr bool contains(int px, int py) const; // [left, right)
    constexpr bool intersects(const Rect&) const;
    constexpr Rect intersect(const Rect&) const;   // empty if disjoint
    constexpr Rect unite(const Rect&) const;       // empty is the identity
    constexpr Rect inset(int amount) const;        // empty if non-positive
};
```

All geometry methods are `constexpr`, so layouts can be checked with
`static_assert`.

### 1.3 `Color` (RGB565)

```cpp
class Color {
public:
    constexpr Color();                         // 0x0000
    constexpr explicit Color(uint16_t value);

    static constexpr Color rgb(uint8_t r, uint8_t g, uint8_t b);

    constexpr uint16_t value() const;
    constexpr uint8_t r() const;               // expanded to 0..255
    constexpr uint8_t g() const;
    constexpr uint8_t b() const;

    constexpr bool operator==(const Color&) const = default;  // != synthesized

private:
    uint16_t value_;
};
```

The `palette` namespace provides `black()`, `white()`, `red()`, `green()`, and
`blue()`. These are `consteval` functions: every call must be evaluated at compile
time, so they generate no runtime code.

```cpp
const Color accent = Color::rgb(45, 212, 191);
```

---

## 2. Fonts

### 2.1 `Font`

```cpp
struct Font {
    const uint8_t* glyphs;  // glyph bitmap, stored by column
    uint8_t first;          // first character code
    uint8_t last;           // last character code
    uint8_t width;          // glyph width in bytes/columns
    uint8_t height;         // glyph height in pixels/rows
    uint8_t spacing;        // inter-character spacing in pixels
    uint8_t lineGap;        // inter-line spacing in pixels
    uint8_t flags;          // reserved, must be 0

    constexpr int advance() const;     // width + spacing
    constexpr int lineHeight() const;  // height + lineGap
    constexpr const uint8_t* glyph(uint8_t c) const;
};
```

Glyph format:

- Each character uses `width` bytes; byte `col` is column `col`.
- Bit 0 of each byte is the top pixel, bit 1 is below it, and so on.
- A character outside `[first, last]` falls back to `'?'`; if that is not present,
  it falls back to `first`.

Built-in font:

```cpp
extern const Font font5x7;  // 5×7, ASCII 0x20..0x7E, spacing 1, line gap 2
```

### 2.2 Measurement helpers

```cpp
[[nodiscard]] int measureTextWidth(const Font& font, const char* text,
                                   uint8_t scale = 1);
[[nodiscard]] int measureTextHeight(const Font& font, const char* text,
                                    uint8_t scale = 1);
```

- Width is `(longest line glyph count × advance − spacing) × scale`. `\n` starts a
  new line, `\r` is ignored, and other control characters count as 4 cells.
- Height is `(line count × lineHeight − lineGap) × scale`.
- `text == nullptr` or `scale == 0` returns 0.
- These helpers do not need a drawing surface; `Label::fit()` and `Button::fit()`
  use them.

---

## 3. Theme

```cpp
struct Theme {
    Color background;       // screen/band background
    Color surface;          // cards and primary surfaces
    Color surface_alt;      // secondary surfaces and tracks
    Color primary;          // primary accent (buttons, progress, checks)
    Color primary_pressed;  // pressed primary color
    Color on_primary;       // text/graphics on the primary color
    Color text;             // body text
    Color text_muted;       // secondary and disabled text
    Color border;           // borders
    Color focus;            // focus ring
    Color success;          // semantic success/online color
    uint8_t radius;         // default radius (used when a widget has -1)
    uint8_t spacing;        // reserved; built-in widgets do not use it yet
    const Font* font;       // default font; nullptr falls back to font5x7
};

const Theme& darkTheme();
const Theme& lightTheme();
const Theme& defaultTheme();  // returns darkTheme()
```

`Ui` stores a `const Theme*`. The theme object must outlive the `Ui`; use the
static themes returned by these functions or your own global/static theme.

---

## 4. Canvas

`Canvas` is a viewport onto a band buffer. It owns no pixels and all coordinates
are screen coordinates. Application code normally only sees it inside a custom
widget's `paint()`.

```cpp
Canvas(uint16_t* pixels, int stride, int width, int height, int originX,
       int originY, Rect clip, const Theme& theme);

uint16_t* pixels() const; int stride() const;
int width() const; int height() const;    // buffer dimensions in pixels
int originX() const; int originY() const; // screen coordinate of buffer (0,0)
Rect clip() const;
const Theme& theme() const;

void setClip(const Rect& clip);  // intersected with the buffer; not with current clip
void clear(Color color);         // fills the current clip
void drawPixel(int x, int y, Color color);

void fillRect(const Rect& rect, Color color);
void drawRect(const Rect& rect, Color color, uint8_t thickness = 1);
void fillRoundRect(const Rect& rect, uint16_t radius, Color color);
void drawLine(int x0, int y0, int x1, int y1, Color color,
              uint8_t thickness = 1);
void drawBitmap(int x, int y, const uint16_t* pixels, uint16_t w,
                uint16_t h);   // RGB565, row width = w
void drawBitmap1bpp(int x, int y, const uint8_t* bits, uint16_t w,
                    uint16_t h, Color color);  // MSB first, 0 bits are transparent

void drawText(int x, int y, const char* text, Color color,
              const Font& font = font5x7, uint8_t scale = 1);
void drawText(const Rect& box, const char* text, Color color,
              const Font& font = font5x7, uint8_t scale = 1,
              Align align = Align::Center);

int textWidth(const char* text, const Font& font = font5x7,
              uint8_t scale = 1) const;
int textHeight(const char* text, const Font& font = font5x7,
               uint8_t scale = 1) const;
```

Behavior:

- All clipping is in screen coordinates; pixels outside `clip` are discarded.
- `drawRect` clamps its thickness to half the short side; thickness 0 has no
  effect.
- `fillRoundRect` clamps the radius to half the short side; radius 0 degenerates
  to `fillRect`.
- `drawBitmap` performs no scaling or color conversion. `drawBitmap1bpp` pads
  each row to `(w + 7) / 8` bytes, treats the most significant bit as the leftmost
  pixel, draws set bits with `color`, and leaves zero bits untouched.
- The first `drawText` uses `(x, y)` as the top-left corner. The second centers
  vertically inside `box` and aligns horizontally according to
  `Align::Left/Center/Right`.
- `setClip` only intersects with the buffer rectangle. Nested clipping therefore
  requires saving and restoring manually:
  `Rect old = canvas.clip(); ...; canvas.setClip(old);`.
- There is no alpha or blending; later pixels simply overwrite earlier ones.

`Align`:

```cpp
enum class Align : uint8_t { Left, Center, Right };
```

---

## 5. Widget base class

### 5.1 Geometry

```cpp
void setPosition(int16_t x, int16_t y);   // relative to the parent
void setSize(uint16_t w, uint16_t h);
void setRect(const Rect& rect);
void setRect(int16_t x, int16_t y, uint16_t w, uint16_t h);

Rect rect() const;                        // local rectangle
uint16_t width() const;
uint16_t height() const;
Rect screenRect() const;                  // after summing parent offsets
int screenX() const;
int screenY() const;
```

### 5.2 Visibility, enablement, and focus

```cpp
void setVisible(bool visible);
bool visible() const;
void setEnabled(bool enabled);
bool enabled() const;
bool enabledInTree() const;               // self and every ancestor enabled

void setFocusable(bool focusable);
bool focusable() const;
bool focused() const;
void requestFocus();                      // equivalent to Ui::focus(this, true)
```

Hiding a widget, disabling it, or clearing focusability also clears the current
focus when needed.

### 5.3 Tree operations

```cpp
void addChild(Widget& child);             // removes it from a previous parent; rejects cycles
void removeChild(Widget& child);
void removeFromParent();
void bringToFront();                      // moves to the end of the sibling list (top Z-order)

Widget* parent() const;
Widget* firstChild() const;
Widget* lastChild() const;
Widget* nextSibling() const;
Widget* previousSibling() const;
```

`setClipChildren(bool)` shrinks the clip rectangle to the widget when drawing
children. `ui()` returns the attached runtime (or `nullptr`), and `invalidate()`
manually damages the widget's screen rectangle.

### 5.4 Event and paint hooks

```cpp
virtual bool onPointerDown(int16_t x, int16_t y); // return true to capture
virtual void onPointerUp(int16_t x, int16_t y);
virtual void onPointerMove(int16_t x, int16_t y);
virtual bool onKey(Key key);                      // return true if handled
virtual bool onChar(char c);                      // printable ASCII, true if consumed

protected:
virtual void paint(Canvas& canvas, int x, int y); // x, y are screen coordinates
```

- Pointer coordinates are local to the widget. `pointerCancel()` calls
  `onPointerUp(-1, -1)`.
- `onPointerUp()` may receive coordinates outside the widget; decide there whether
  to activate.
- `onChar()` only receives bytes in `0x20..0x7E`, routed by `Ui::inputChar()` to
  the focused widget.
- `paint()` draws only the widget itself. The base class draws children in
  Z-order and then the focus ring.
- `Widget::draw()` is the internal entry point used by `Ui`; applications normally
  do not call it.

`Key`:

```cpp
enum class Key : uint8_t {
    None, Enter, Space, Left, Right, Up, Down, Tab, Escape,
    Backspace, Delete
};
```

---

## 6. Built-in widgets

### 6.1 `Panel`

A container with an optional background, border, and rounded corners.

```cpp
Panel();

void setBackground(Color color);   // fixed color
void useThemeBackground();         // use theme.surface (default)
void setTransparent();             // draw no background
void setBorder(Color color, uint8_t width = 1);  // width clamped to 1..4
void clearBorder();
void setRadius(int16_t radius);    // -1 means theme.radius (default)
```

Defaults: themed background, no border, `radius = -1`. With both a background and
a border, the border color is painted first and the background is inset by the
border width. A transparent panel with a border uses a rectangular outline.

### 6.2 `Label`

```cpp
explicit Label(const char* text = nullptr);

void setText(const char* text);    // stores the pointer; does not copy
const char* text() const;
void setAlign(Align align);        // default Left
Align align() const;
void setScale(uint8_t scale);      // 0 is treated as 1
uint8_t scale() const;
void setColor(Color color);        // fixed color
void useThemeColor();              // restore the themed color (default)
void setEllipsis(bool enabled);    // show "..." when single-line text overflows (default true)
bool ellipsis() const;
void fit(const Font& font = font5x7);  // size the widget to the text
```

Default color: `theme.text` when enabled and `theme.text_muted` when disabled. If
`setColor()` has been called, disabling does not automatically mute the color.
Text is clipped to the widget rectangle. Single-line text that does not fit is
truncated with a `"..."` suffix according to `Align`; multi-line text is clipped
as before.

### 6.3 `Button`

```cpp
typedef void (*Action)(void* context);

explicit Button(const char* text = nullptr, Action action = nullptr,
                void* context = nullptr);

void setText(const char* text);
const char* text() const;
void setAction(Action action, void* context = nullptr);
void setKind(ButtonKind kind);     // Primary (default) / Secondary
void setScale(uint8_t scale);
void setRadius(int16_t radius);    // -1 means theme.radius
void fit(const Font& font = font5x7, uint16_t padX = 12, uint16_t padY = 8);
void activate();                   // invoke the action now (checks enabledInTree)
bool pressed() const;
```

- The constructor makes the button focusable.
- Pressing shows the pressed state; the action fires only when the pointer is
  released inside the button.
- `Enter` and `Space` activate it.
- `Primary` uses `primary`/`primary_pressed`/`on_primary`; `Secondary` uses
  `surface_alt` and a border.
- The text pointer is not copied.

### 6.4 `Checkbox`

```cpp
typedef void (*Toggled)(void* context, bool checked);

explicit Checkbox(const char* text = nullptr, bool checked = false);

void setText(const char* text);
const char* text() const;
void setChecked(bool checked);     // does not fire the callback
void toggle();                     // flips the state only; no callback
bool checked() const;
void setToggled(Toggled callback, void* context = nullptr);
void setScale(uint8_t scale);
void fit(const Font& font = font5x7);
```

- Focusable by default.
- Clicking/tapping or `Enter`/`Space` toggles the state and invokes the callback.
- The box is 18 pixels; `fit()` sizes the widget to the box plus an 8-pixel gap
  and the text.

### 6.5 `ProgressBar`

```cpp
explicit ProgressBar(uint8_t value = 0);

void setValue(uint8_t value);   // clamped to 0..100
uint8_t value() const;
```

Not focusable. The track uses `surface_alt` (`surface` when disabled), and the
fill uses `primary` (`border` when disabled). The fill is an integer percentage
and any non-zero value paints at least one pixel.

### 6.6 `Slider`

```cpp
typedef void (*Changed)(void* context, uint8_t value);

explicit Slider(uint8_t value = 0);

void setValue(uint8_t value);   // 0..100; invokes the callback on change
uint8_t value() const;
void setStep(uint8_t step);     // arrow-key step, default 5; 0 is treated as 1
uint8_t step() const;
void setChanged(Changed callback, void* context = nullptr);
```

- Focusable by default.
- Pressing/dragging maps the horizontal coordinate to 0..100. `Left`/`Down`
  subtract the step, `Right`/`Up` add it.
- Unlike `Checkbox`, `setValue()` invokes the callback whenever the value changes.
  Avoid calling `setValue()` from inside the callback unless you handle the
  feedback loop.
- The track and fill are rounded rectangles; the knob is a small rounded square
  and does not require a circle primitive.

### 6.7 `Switch`

```cpp
typedef void (*Toggled)(void* context, bool checked);

explicit Switch(bool checked = false);

void setChecked(bool checked);   // does not fire the callback
bool checked() const;
void setToggled(Toggled callback, void* context = nullptr);
```

- Focusable by default.
- A click/tap released inside the widget, or `Enter`/`Space`, toggles the state and
  invokes the callback.
- `setChecked()` only updates the display and does not fire the callback, matching
  `Checkbox`.

### 6.8 `ListBox`

```cpp
typedef void (*Selected)(void* context, uint16_t index);
static constexpr uint16_t kNone = 0xFFFFu;

explicit ListBox(const char* const* items = nullptr, uint16_t count = 0,
                 uint16_t rowHeight = 24);

void setItems(const char* const* items, uint16_t count);
void setRowHeight(uint16_t rowHeight);
uint16_t count() const;
uint16_t rowHeight() const;
uint16_t selected() const;                  // kNone when nothing is selected
void setSelected(uint16_t index);           // kNone clears; does not fire the callback
void setSelectedCallback(Selected callback, void* context = nullptr);
void scrollTo(uint16_t index);
void scrollBy(int16_t rows);
uint16_t topRow() const;                    // first visible row
```

- Focusable by default.
- `items` is a caller-owned array of string pointers; the widget never copies the
  strings.
- A scrollbar appears when the content is taller than the widget. A pointer drag
  of more than 4 pixels scrolls; a release without a drag selects the row under
  the pointer. `Up`/`Down` move the selection and keep it visible; `Enter` fires
  the callback again.
- The callback fires when the selection changes via pointer or arrow keys.
  `setSelected()` and `setItems()` do not fire it.
- `setSelected()` scrolls the selection into view. `scrollTo()` takes the index of
  the first visible row.

### 6.9 `EditBox`

```cpp
typedef void (*Changed)(void* context, const char* text);
typedef void (*Submitted)(void* context, const char* text);

EditBox(char* buffer, uint16_t capacity, uint16_t maxLength = 0);

const char* text() const;
void setText(const char* text);             // does not fire Changed
void setMaxLength(uint16_t maxLength);      // 0 means capacity - 1
uint16_t maxLength() const;
void setCursor(uint16_t position);
uint16_t cursor() const;
void setChanged(Changed callback, void* context = nullptr);
void setSubmitted(Submitted callback, void* context = nullptr);
```

- Focusable by default when editing is possible (`buffer != nullptr` and
  `capacity >= 2`).
- **The buffer is caller-owned.** `capacity` includes the terminating `'\0'`; the
  widget never allocates or copies to another buffer.
- `Changed` fires when the user inserts/deletes a character or inserts a space.
  `Enter` fires `Submitted`. `setText()` and `setMaxLength()` do not fire
  `Changed`.
- Editing keys: `Left`/`Right` move the cursor, `Backspace` deletes before the
  cursor, `Delete` deletes at the cursor, `Space` inserts a space, and `Enter`
  submits. Printable characters must be sent through `Ui::inputChar()`.
- A click places the cursor at the corresponding character. The text scrolls
  horizontally to keep the cursor visible. The cursor is drawn only while the
  widget has focus (static, no blinking).

### 6.10 `Image`

```cpp
explicit Image(const uint16_t* pixels = nullptr, uint16_t imageWidth = 0,
               uint16_t imageHeight = 0);

void setBitmap(const uint16_t* pixels, uint16_t imageWidth,
               uint16_t imageHeight);
const uint16_t* bitmap() const;
uint16_t imageWidth() const;
uint16_t imageHeight() const;
void fit();   // widget size := bitmap size
```

Draws raw RGB565 bitmaps only; no scaling, color conversion, or decoding. The
bitmap data is caller-owned. For 1bpp icons, call `Canvas::drawBitmap1bpp()` from a
custom widget's `paint()`.

---

## 7. Runtime `Ui`

### 7.1 Display driver and constructors

```cpp
struct Display {
    int16_t width;
    int16_t height;
    void* context;
    void (*flush)(void* context, int16_t x, int16_t y, int16_t w, int16_t h,
                  const uint16_t* pixels, uint16_t stride);
};
```

```cpp
Ui(Display& display, Widget& root, uint16_t* bandBuffer,
   uint32_t bandCapacityPixels, uint16_t bandHeight,
   const Theme& theme = defaultTheme());

template <BandBuffer Buffer>
Ui(Display& display, Widget& root, Buffer& bandBuffer,
   uint16_t bandHeight, const Theme& theme = defaultTheme());
```

`BandBuffer` is a C++20 concept that accepts only non-empty `uint16_t[N]` arrays;
pointers and other pixel types fail overload resolution. The template overload
deduces the capacity and is the recommended form:

```cpp
static uint16_t band[240 * 16];
Ui ui(display, root, band, 16);   // capacity = 3840 pixels
```

On construction:

- The root is forced to `(0, 0, display.width, display.height)` and the whole tree
  is attached.
- `bandHeight` is clamped to `bandCapacityPixels / display.width` and the display
  height.
- If the buffer is null, the display size is non-positive, or the clamped band
  height is zero, `valid()` is `false` and every operation is a safe no-op.
- `display`, `bandBuffer`, `root`, and `theme` must outlive the `Ui`.

`Ui` is non-copyable.

### 7.2 State and queries

```cpp
[[nodiscard("check the band buffer size and display dimensions")]]
bool valid() const;                 // ignoring it produces a diagnostic
Display& display();
const Display& display() const;
Widget* root() const;
void setRoot(Widget& root);        // switch roots and invalidate the whole screen
void setTheme(const Theme& theme); // switch themes and invalidate the whole screen
const Theme& theme() const;
uint16_t bandHeight() const;
```

### 7.3 Invalidation, rendering, and dirty rectangles

```cpp
void invalidate(const Rect& area);  // screen coordinates; intersected with the display
void invalidateAll();
void render();                      // draw and clear the dirty list
uint8_t dirtyCount() const;
Rect dirtyRect(uint8_t index) const; // out-of-range returns an empty rectangle
```

`render()` returns immediately when there is no dirty rectangle. The dirty list can
be queried until the next render (for example for statistics). Merging rules are
described in [Architecture](architecture.md#41-invalidation-and-dirty-rectangles).

### 7.4 Pointer input

```cpp
bool pointerDown(int16_t x, int16_t y);  // true if a widget was hit and captured
bool pointerMove(int16_t x, int16_t y);  // true while a capture target exists
bool pointerUp(int16_t x, int16_t y);    // true while a capture target exists
void pointerCancel();
bool pointerCaptured() const;
```

- Coordinates are screen coordinates.
- If a capture target already exists, `pointerDown()` cancels it first.
- On a hit, `onPointerDown()` is called from the target up through its ancestors.
  A focusable target also receives focus without showing the focus ring.
- `pointerMove`/`pointerUp` are delivered only to the capture target, even when the
  pointer leaves the widget.
- `pointerCancel()` reports `(-1, -1)` to the target and never activates a button.

### 7.5 Keyboard and focus

```cpp
bool keyDown(Key key);
bool inputChar(char c);   // printable ASCII -> onChar() of the focused widget
void focus(Widget* widget, bool showRing = true);
void clearFocus();
void focusNext();
void focusPrevious();
Widget* focused() const;
```

- `keyDown(Key::None)` returns `false`.
- `inputChar()` accepts only `0x20..0x7E`. If nothing is focused it first focuses a
  candidate, just like `keyDown()`, and returns whether a widget consumed the
  character (`EditBox` consumes printable characters).
- `Tab` is handled by `Ui` as `focusNext()` and returns `true`.
- Other keys first auto-focus the first candidate when nothing is focused.
- `focus()` only accepts a widget belonging to this `Ui` that is visible, enabled,
  and focusable; passing `nullptr` clears focus.
- Traversal is a cyclic pre-order walk (parent → child → sibling), skipping
  invisible, disabled, and non-focusable widgets.

---

## 8. Configuration macros

Define these before including `pixmote.hpp`:

| Macro | Default | Range | Description |
| --- | --- | --- | --- |
| `PIXMOTE_MAX_DIRTY_RECTS` | `4` | 1..16 | Dirty-rectangle table size; overflow merges into one rectangle |
| `PIXMOTE_USE_LIBM` | `0` | 0/1 | Use libm `sqrt()` for rounded corners; link with `-lm` |

```cpp
#define PIXMOTE_MAX_DIRTY_RECTS 8
#include "pixmote.hpp"
```

Or pass them on the command line:

```bash
g++ -DPIXMOTE_MAX_DIRTY_RECTS=8 -Iinclude ...
```

Version macros: `PIXMOTE_VERSION_MAJOR/MINOR/PATCH` (currently `0.1.0`).

Pixmote requires C++20. Concepts, `consteval`, and `constexpr` geometry exist only
at compile time and add no heap, exception, RTTI, or runtime-library dependencies.
Only zero-runtime headers such as `<concepts>` and `<type_traits>` are used.

---

## 9. Usage notes

- **Text ownership**: `Label`/`Button`/`Checkbox`/`ListBox` store only a
  `const char*`. String literals, static buffers, and long-lived member arrays are
  safe; a local `char[]` on the stack creates a dangling pointer. The `EditBox`
  buffer is caller-owned and must outlive the widget (and any still-used `text()`
  pointer).
- **Object lifetime**: the `Display`, root widget, band buffer, and theme must all
  outlive the `Ui`.
- **Relative coordinates**: every `setRect()` uses the parent's coordinate system;
  only `screenRect()` is in screen coordinates.
- **ASCII only**: characters are parsed byte by byte; UTF-8 is not supported.
  Unsupported characters are drawn as `?`. `EditBox` is single-line ASCII and the
  library has no on-screen keyboard; touch-only products should provide a keypad
  and call `Ui::inputChar()`.
- **Bitmaps**: `Image` accepts raw RGB565 data only; PNG/JPEG/GIF decoders are not
  included.
- **No blending**: drawing is simple overwrite. For "transparent" panels, use a
  `Panel` without a background so previously drawn ancestors/siblings show
  through.
- **Single-threaded**: input and rendering must run on the same thread; do not call
  `Ui` from an ISR.
- **Every dirty region redraws the whole tree**: performance depends on the area,
  band height, and widget count; avoid frequent full-screen invalidations.
