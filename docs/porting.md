# Porting Guide

This guide explains how to connect Pixmote to a real display, touch panel, and
keyboard, and how to tune RAM and CPU usage.

## 1. Integration checklist

1. Add `include/` to the header search path and compile the six sources under
   `src/`:

   ```
   src/canvas.cpp  src/font5x7.cpp  src/theme.cpp
   src/ui.cpp      src/widget.cpp   src/widgets.cpp
   ```

2. Build with C++20 or later. The library does not depend on STL containers,
   exceptions, RTTI, libm, or any OS. Concepts, `consteval`, `constexpr`, and
   designated initializers are compile-time only and add no runtime or startup
   cost.
3. Provide a `width × bandHeight` `uint16_t` band buffer (`static`, preferably
   4-byte aligned).
4. Implement `Display::flush` to send RGB565 data to the panel.
5. Build the widget tree and `Ui`, then translate touch/key events into
   `pointer*`, `keyDown`, and `inputChar` calls.
6. Call `ui.render()` after processing events.

Minimal skeleton:

```cpp
static uint16_t g_band[LCD_WIDTH * 16];

void lcd_flush(void* /*context*/, int16_t x, int16_t y, int16_t w, int16_t h,
               const uint16_t* pixels, uint16_t stride);

int main() {
    Display display{
        .width = LCD_WIDTH,
        .height = LCD_HEIGHT,
        .context = nullptr,
        .flush = &lcd_flush,
    };

    Panel root;
    root.setBackground(darkTheme().background);
    root.setRadius(0);
    // ... add widgets ...

    Ui ui(display, root, g_band, 16);
    if (!ui.valid()) {
        // Band buffer smaller than one row, or invalid display size.
        for (;;) {}
    }

    ui.invalidateAll();
    ui.render();

    for (;;) {
        // Process events, then:
        ui.render();
    }
}
```

## 2. Display driver

### 2.1 The `flush` contract

```cpp
void (*flush)(void* context, int16_t x, int16_t y, int16_t w, int16_t h,
              const uint16_t* pixels, uint16_t stride);
```

| Parameter | Meaning |
| --- | --- |
| `x, y` | Top-left corner of the damaged area/band in screen coordinates |
| `w, h` | Width and height of this transfer in pixels |
| `pixels` | RGB565 data; row 0 corresponds to screen row `y` |
| `stride` | Pixels per row (currently equal to `w`, but use the contract value) |

**The callback must finish the transfer or copy the data before returning.** The
next band reuses the same band buffer. With asynchronous DMA, wait for completion
inside the callback or provide a second buffer for double buffering.

### 2.2 Blocking implementation

```cpp
struct LcdCtx {
    void (*setWindow)(int16_t x, int16_t y, int16_t w, int16_t h);
    void (*writePixels)(const uint16_t* data, size_t count);
};

void lcd_flush(void* context, int16_t x, int16_t y, int16_t w, int16_t h,
               const uint16_t* pixels, uint16_t stride) {
    LcdCtx* lcd = static_cast<LcdCtx*>(context);
    lcd->setWindow(x, y, w, h);
    for (int row = 0; row < h; ++row) {
        // Write w pixels per row; rows may be separated by `stride`.
        lcd->writePixels(pixels + row * stride, static_cast<size_t>(w));
    }
}
```

Replace `setWindow` / `writePixels` with the HAL, SPI, or 8080-parallel functions
of your platform. If the bus supports a single burst and `stride == w`, the whole
block can be sent in one call.

### 2.3 RGB565 byte order

`Color::value()` and the band buffer use host byte order. Many LCD controllers,
especially SPI ones, expect the high byte first. On a little-endian MCU, swap the
bytes:

```cpp
static inline uint16_t swap_bytes(uint16_t v) {
    return static_cast<uint16_t>((v >> 8) | (v << 8));
}
```

If DMA requires a byte stream, copy into a temporary byte buffer or configure the
controller/DMA to transfer 16-bit halfwords with the correct endianness. Do not
swap the band buffer in place: the next render will read it back as RGB565.

### 2.4 Coordinates, rotation, and offsets

- `Display::width/height` must describe the **logical** screen (the same
  coordinate system as touch input).
- Apply rotation in the driver: `setWindow` maps logical coordinates to panel
  coordinates.
- For panels with an offset (for example, a 320×480 panel showing only a 240×320
  area), add the offset in the driver; `Ui` does not need to know about it.

## 3. Band buffer

Buffer size = `width × bandHeight × 2` bytes. `Ui` clamps `bandHeight` to the
maximum number of rows the buffer allows; if that is less than one row,
`valid()` is `false`.

| Resolution | Band height | Buffer | Flush calls for a full redraw |
| --- | --- | --- | --- |
| 128 × 64 | 8 | 2,048 B | 8 |
| 240 × 320 | 16 | 7,680 B | 20 |
| 320 × 240 | 16 | 10,240 B | 15 |
| 480 × 272 | 24 | 23,040 B | 12 |
| 800 × 480 | 16 | 25,600 B | 30 |

A larger band height:

- reduces the number of `flush` calls and SPI/DMA setup overhead;
- increases peak RAM usage;
- may redraw a few extra rows for small damaged areas (dirty rectangles are split
  along band boundaries but never exceed the region).

Start with 8–32 rows. Reduce the band height when RAM is tight; increase it when
the bus is slow and flush overhead dominates. For DMA, align the band height with
the largest DMA transfer block.

```cpp
static uint16_t g_band[240 * 16];   // 7.5 KB
Ui ui(display, root, g_band, 16);   // the template overload deduces the capacity
```

## 4. Input integration

### 4.1 Touch

Calibrate touch coordinates to screen pixels, then call:

```cpp
enum class TouchAction { Down, Move, Up, Cancel };

void feed_touch(Ui& ui, TouchAction action, int16_t x, int16_t y) {
    switch (action) {
        case TouchAction::Down:   ui.pointerDown(x, y); break;
        case TouchAction::Move:   ui.pointerMove(x, y); break;
        case TouchAction::Up:     ui.pointerUp(x, y);   break;
        case TouchAction::Cancel: ui.pointerCancel();   break;
    }
}
```

- After `pointerDown` hits a widget, subsequent `pointerMove`/`pointerUp` calls go
  to the same capture target.
- Always call `pointerCancel()` when the touch is cancelled (app switch, gesture
  interruption); otherwise a button can remain pressed.
- If you need hover effects (mouse), note that `pointerMove` produces no event when
  there is no capture target. Track hover in the application and invalidate
  manually.
- Put debouncing and long-press/gesture logic in the application; the library only
  provides raw down/move/up.

### 4.2 Keyboard and encoder

```cpp
Key map_key(uint32_t code) {
    switch (code) {
        case KEY_ENTER:     return Key::Enter;
        case KEY_SPACE:     return Key::Space;
        case KEY_LEFT:      return Key::Left;
        case KEY_RIGHT:     return Key::Right;
        case KEY_UP:        return Key::Up;
        case KEY_DOWN:      return Key::Down;
        case KEY_TAB:       return Key::Tab;
        case KEY_ESC:       return Key::Escape;
        case KEY_BACKSPACE: return Key::Backspace;
        case KEY_DELETE:    return Key::Delete;
        default:            return Key::None;
    }
}

const Key key = map_key(code);
if (key == Key::None && code >= 0x20 && code <= 0x7E) {
    ui.inputChar(static_cast<char>(code));  // printable character -> EditBox
} else {
    ui.keyDown(key);
}
```

- `Tab` moves focus to the next focusable widget.
- `Enter` / `Space` activate buttons and toggle checkboxes/switches.
- Arrow keys adjust `Slider` and move the `ListBox` selection; `EditBox` uses
  Left/Right, `Backspace`/`Delete`, and `Enter`.
- Printable characters must go through `ui.inputChar()`, not `keyDown()`.
  `EditBox` consumes ASCII `0x20..0x7E`.
- An encoder can map rotation to Up/Down/Left/Right, press to Enter, and long
  press to Escape.
- For Shift+Tab and similar combinations, call `ui.focusPrevious()` directly.

## 5. RTOS and interrupts

`Ui` is single-threaded and has no internal locking. Recommended pattern:

- **ISR**: enqueue raw events (touch coordinates, key codes, timestamps) only.
- **UI task**: dequeue, translate to `pointer*`, `keyDown`, or `inputChar`, then
  call `ui.render()` once after the batch.

```cpp
void ui_task(void*) {
    for (;;) {
        Event e;
        xQueueReceive(g_events, &e, portMAX_DELAY);
        do {
            dispatch(e);
        } while (xQueueReceive(g_events, &e, 0) == pdTRUE);
        ui.render();   // one render per batch
    }
}
```

If multiple tasks modify widgets, protect "modify + `render()`" with a mutex or
route the modifications through the UI task. Never call `Ui` or widget setters
from an ISR.

## 6. Building and cross-compiling

### 6.1 Make (library only)

```bash
make lib CXX=arm-none-eabi-g++ \
    CXXFLAGS="-std=c++20 -Os -g -mcpu=cortex-m4 -mthumb \
              -ffunction-sections -fdata-sections -fno-exceptions -fno-rtti"
```

The result is `build/libpixmote.a`. A plain `make` also builds the host tests and
demo; embedded projects only need `make lib`.

### 6.2 CMake

```cmake
set(PIXMOTE_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(PIXMOTE_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory(path/to/pixmote)

target_link_libraries(firmware PRIVATE pixmote)
```

Alternatively, list the six sources directly and add `include/`.

### 6.3 Manual compilation

```bash
arm-none-eabi-g++ -std=c++20 -Os -Iinclude \
    src/canvas.cpp src/font5x7.cpp src/theme.cpp \
    src/ui.cpp src/widget.cpp src/widgets.cpp \
    -c
arm-none-eabi-ar rcs libpixmote.a *.o
```

The library itself does not need `-lm`. If `PIXMOTE_USE_LIBM=1` is defined, add
`-lm` when linking the application.

### 6.4 Suggested compiler flags

| Flag | Notes |
| --- | --- |
| `-std=c++20` or later | Required |
| `-Os` / `-O2` | Common embedded choices; try `-O2` for large displays |
| `-fno-exceptions` / `-fno-rtti` | Unused by the library; reduces code size |
| `-ffunction-sections -fdata-sections` | Combine with `--gc-sections` to drop unused widgets |
| `-Wall -Wextra -Wpedantic` | Recommended during development |

## 7. Performance and RAM tuning

### 7.1 RAM

- The band buffer is the largest knob: reduce `bandHeight` first.
- Dirty rectangle table: `PIXMOTE_MAX_DIRTY_RECTS` defaults to 4. 1–2 saves a few
  dozen bytes but degrades to merged redraws sooner.
- Widgets are statically allocated; more widgets means more RAM (about 40–60 B per
  widget on a 32-bit target; `EditBox`/`ListBox` are slightly larger and still
  heap-free).
- Strings are application-managed. Avoid large buffers for every `Label`. The
  `EditBox` buffer includes the terminating `'\0'`; `ListBox` stores only the item
  pointer array and does not copy strings.
- Raw bitmaps cost `width × height × 2` bytes in flash/RAM; 1bpp icons cost
  `width × height / 8` bytes.

### 7.2 CPU and bus

- **Avoid full-screen invalidation**: use `invalidateAll()` only at startup, after
  a theme change, or after switching roots.
- Coalesce application updates: multiple `setText`/`setValue` calls in one pass
  merge into fewer dirty rectangles at render time.
- Band height and flush count are inversely related; increasing the band height
  often helps slow SPI buses.
- Larger corner radii mean more square-root calculations per row; 0–12 pixels is
  plenty on small screens.
- Scaled text cost grows roughly with `scale²`; a 2× title is usually enough.
- `setClipChildren(true)` lets fully off-band subtrees return early.
- Align each `flush` width with the bus burst length (for example, a 32-bit DMA
  boundary).

### 7.3 Flash

- The built-in 5×7 font is about 475 B; a custom font costs
  `glyph count × width` bytes.
- Unused widgets are not removed automatically unless you compile with
  `-ffunction-sections --gc-sections` and genuinely do not reference them.
- Keep the demo and tests out of the firmware link.

## 8. Validation and debugging

### 8.1 Get the host build right first

```bash
make test        # functional and zero-allocation tests
make run-demo    # headless render to build/demo_initial.ppm and friends
python3 tools/ppm2png.py --scale 2 build/demo.ppm /tmp/demo.png
```

Confirm the host screenshot before moving to hardware. This quickly separates UI
bugs from driver bugs.

### 8.2 Board bring-up order

1. Is `flush` being called? Count calls by printing or toggling a pin.
2. Do `x/y/w/h` match the expected damage? The first full-screen render should
   cover the entire display.
3. Are colors correct? Draw three primary-color rectangles to verify RGB565 byte
   order.
4. Only part of the screen updates? Check that `render()` is called after
   `pointerUp` and that the band buffer is valid.
5. Widgets do not respond? Check `visible`, `enabled`, occlusion, and whether
   `onPointerDown` returns `true`.
6. Garbled or missing text? Check string lifetime, the ASCII 0x20–0x7E range, and
   the custom font's `first/last` range.
7. `EditBox` does not accept input? Check focus, that printable characters go
   through `ui.inputChar()`, that the buffer capacity is at least 2, and that
   `maxLength` is not exhausted.
8. A list does not scroll or loses rows? Check widget height, `rowHeight`, and
   `count`; item string pointers must remain valid for the lifetime of the list.

### 8.3 Common symptoms

| Symptom | Likely cause |
| --- | --- |
| Wrong red/blue tint or noise | RGB565 high/low byte order |
| Shifted or skewed image | `stride` not used per row; wrong window rectangle |
| Tearing | DMA reused the band buffer before completion |
| Ghosting while dragging | Custom widget did not invalidate; or the driver only redraws new regions |
| Button does nothing | A transparent widget on top swallowed the press; `onPointerDown` returned `false`; an ancestor is disabled |
| No focus ring | Widget is not `setFocusable(true)`, or focus came from a pointer press (ring intentionally hidden) |
| Band too small | `ui.valid() == false`; all operations are no-ops |
| Text truncated | Label is too small; single-line labels show an ellipsis, multi-line labels are clipped |
| EditBox has no cursor | Not focused, buffer is empty, or `maxLength` is full |
| List selection/scroll is wrong | `rowHeight == 0` is treated as 1; item pointers are invalid; `count` is wrong |
| Garbled image | Data is not RGB565, or `w/h` do not match the actual bitmap |

## 9. Porting checklist

- [ ] Display size and touch coordinates share the same logical orientation
      (rotation handled in the driver)
- [ ] `flush` consumes `pixels` before returning (DMA waited or data copied)
- [ ] RGB565 byte order matches the panel
- [ ] Band buffer holds at least one row and `ui.valid()` is true
- [ ] All widgets, the theme, string buffers, and `Display` outlive `Ui`
- [ ] Touch cancellation calls `pointerCancel()`
- [ ] Printable characters go through `ui.inputChar()`; `Backspace`/`Delete` are
      mapped
- [ ] `EditBox` buffers and `ListBox` item strings outlive their widgets
- [ ] Events and `render()` run in the same task; ISRs only enqueue events
- [ ] Initialization and theme changes call `invalidateAll()` or `render()`
- [ ] Host tests and the demo screenshot have been run
