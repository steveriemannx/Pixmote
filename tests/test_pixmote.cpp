#include "pixmote.hpp"

#include <cstdio>
#include <cstring>

using namespace pixmote;

// C++20: value-type geometry is usable in constant expressions, and the
// defaulted operator== (plus synthesized !=) keeps these checks terse.
static_assert(Rect(0, 0, 10, 10).intersect(Rect(5, 5, 10, 10)) ==
                  Rect(5, 5, 5, 5),
              "Rect::intersect must be constexpr");
static_assert(Rect(0, 0, 10, 10).inset(2) == Rect(2, 2, 6, 6),
              "Rect::inset must be constexpr");
static_assert(Rect(0, 0, 10, 10).unite(Rect(8, 8, 4, 4)) ==
                  Rect(0, 0, 12, 12),
              "Rect::unite must be constexpr");
static_assert(Rect(0, 0, 10, 10).contains(9, 9));
static_assert(!Rect(0, 0, 10, 10).contains(10, 10));
static_assert(Point(1, 2) == Point(1, 2));
static_assert(Point(1, 2) != Point(2, 1));
static_assert(Color::rgb(255, 0, 0) == Color(0xF800));
static_assert(Color::rgb(0, 255, 0) == Color(0x07E0));
static_assert(Color::rgb(0, 0, 255) == Color(0x001F));
static_assert(palette::red() != palette::black());

namespace {

int g_failures = 0;

#define CHECK(cond)                                                        \
    do {                                                                   \
        if (!(cond)) {                                                     \
            std::printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);    \
            ++g_failures;                                                  \
        }                                                                  \
    } while (0)

const int kWidth = 160;
const int kHeight = 120;
const uint16_t kBandHeight = 10;
constinit uint16_t g_band[kWidth * kBandHeight];

struct TestDisplay {
    uint16_t fb[kWidth * kHeight];
    Display display;
    int flushes;
    Rect last;
    int minX;
    int minY;
    int maxX;
    int maxY;

    TestDisplay() {
        std::memset(fb, 0, sizeof(fb));
        // C++20 designated initializers: order follows Display's declaration.
        display = Display{
            .width = kWidth,
            .height = kHeight,
            .context = this,
            .flush = &TestDisplay::flush,
        };
        resetStats();
    }

    void resetStats() {
        flushes = 0;
        last = Rect();
        minX = kWidth;
        minY = kHeight;
        maxX = -1;
        maxY = -1;
    }

    uint16_t pixel(int x, int y) const { return fb[y * kWidth + x]; }

    static void flush(void* context, int16_t x, int16_t y, int16_t w, int16_t h,
                      const uint16_t* pixels, uint16_t stride) {
        TestDisplay* self = static_cast<TestDisplay*>(context);
        ++self->flushes;
        self->last = Rect(x, y, static_cast<uint16_t>(w),
                          static_cast<uint16_t>(h));
        if (x < self->minX) {
            self->minX = x;
        }
        if (y < self->minY) {
            self->minY = y;
        }
        if (x + w > self->maxX) {
            self->maxX = x + w;
        }
        if (y + h > self->maxY) {
            self->maxY = y + h;
        }
        for (int row = 0; row < h; ++row) {
            std::memcpy(&self->fb[(y + row) * kWidth + x],
                        pixels + row * stride,
                        static_cast<size_t>(w) * sizeof(uint16_t));
        }
    }
};

struct ClickCounter {
    int clicks;
    int toggles;
    bool lastChecked;

    ClickCounter() : clicks(0), toggles(0), lastChecked(false) {}
};

void onClick(void* context) {
    static_cast<ClickCounter*>(context)->clicks++;
}

void onToggle(void* context, bool checked) {
    ClickCounter* counter = static_cast<ClickCounter*>(context);
    counter->toggles++;
    counter->lastChecked = checked;
}

struct ValueTracker {
    int changes;
    uint8_t last;

    ValueTracker() : changes(0), last(0) {}
};

struct TextTracker {
    int changes;
    int submits;
    const char* last;

    TextTracker() : changes(0), submits(0), last(nullptr) {}
};

struct ListTracker {
    int selections;
    uint16_t last;

    ListTracker() : selections(0), last(0) {}
};

void onSliderChanged(void* context, uint8_t value) {
    ValueTracker* tracker = static_cast<ValueTracker*>(context);
    ++tracker->changes;
    tracker->last = value;
}

void onListSelected(void* context, uint16_t index) {
    ListTracker* tracker = static_cast<ListTracker*>(context);
    ++tracker->selections;
    tracker->last = index;
}

void onEditChanged(void* context, const char* text) {
    TextTracker* tracker = static_cast<TextTracker*>(context);
    ++tracker->changes;
    tracker->last = text;
}

void onEditSubmitted(void* context, const char* text) {
    TextTracker* tracker = static_cast<TextTracker*>(context);
    ++tracker->submits;
    tracker->last = text;
}

void testCanvas() {
    uint16_t pixels[20 * 10];
    Canvas canvas(pixels, 20, 20, 10, 0, 0, Rect(0, 0, 20, 10),
                  darkTheme());

    canvas.fillRect(Rect(-4, -4, 28, 18), palette::red());
    CHECK(pixels[0] == palette::red().value());
    CHECK(pixels[19 * 1 + 19] == palette::red().value());

    canvas.setClip(Rect(2, 2, 5, 4));
    canvas.fillRect(Rect(0, 0, 20, 10), palette::blue());
    CHECK(pixels[2 * 20 + 2] == palette::blue().value());
    CHECK(pixels[2 * 20 + 1] == palette::red().value());
    CHECK(pixels[1 * 20 + 2] == palette::red().value());
    CHECK(pixels[6 * 20 + 6] == palette::red().value());

    canvas.setClip(Rect(0, 0, 20, 10));
    CHECK(canvas.textWidth("AB") == 11);
    CHECK(canvas.textHeight("AB") == 7);
    CHECK(canvas.textWidth("AB", font5x7, 2) == 22);
    CHECK(canvas.textHeight("AB", font5x7, 2) == 14);
    CHECK(canvas.textWidth("A\nBC") == 11);

    canvas.fillRect(Rect(0, 0, 20, 10), palette::black());
    canvas.fillRoundRect(Rect(10, 0, 10, 10), 5, palette::green());
    CHECK(canvas.pixels()[0] == palette::black().value());
    CHECK(canvas.pixels()[5 * 20 + 15] == palette::green().value());

    canvas.fillRect(Rect(0, 0, 20, 10), palette::black());
    canvas.drawRect(Rect(1, 1, 18, 8), palette::white());
    CHECK(canvas.pixels()[1 * 20 + 1] == palette::white().value());
    CHECK(canvas.pixels()[2 * 20 + 2] == palette::black().value());
}

void testLayoutAndDamage() {
    TestDisplay test;

    Panel root;
    root.setBackground(Color::rgb(1, 2, 3));
    root.setRadius(0);
    Label label("Hello");
    label.setRect(10, 10, 60, 20);
    label.setScale(1);
    root.addChild(label);

    Ui ui(test.display, root, g_band, kBandHeight);
    CHECK(ui.valid());

    ui.render();
    CHECK(test.flushes == (kHeight + kBandHeight - 1) / kBandHeight);
    CHECK(test.pixel(0, 0) == Color::rgb(1, 2, 3).value());

    int nonBackground = 0;
    for (int y = 10; y < 30; ++y) {
        for (int x = 10; x < 70; ++x) {
            if (test.pixel(x, y) != Color::rgb(1, 2, 3).value()) {
                ++nonBackground;
            }
        }
    }
    CHECK(nonBackground > 0);

    test.resetStats();
    label.setText("World");
    CHECK(ui.dirtyCount() == 1);
    CHECK(ui.dirtyRect(0).x == 10);
    CHECK(ui.dirtyRect(0).y == 10);
    CHECK(ui.dirtyRect(0).w == 60);
    CHECK(ui.dirtyRect(0).h == 20);
    ui.render();
    CHECK(test.flushes == 2);
    CHECK(test.minX == 10);
    CHECK(test.maxX == 70);
    CHECK(test.minY == 10);
    CHECK(test.maxY == 30);

    test.resetStats();
    ui.invalidate(Rect(0, 0, 5, 5));
    ui.invalidate(Rect(100, 100, 5, 5));
    CHECK(ui.dirtyCount() == 2);
    ui.render();
    CHECK(test.flushes == 2);
    CHECK(test.minX == 0);
    CHECK(test.maxX == 105);

    label.removeFromParent();
    CHECK(label.ui() == nullptr);
    CHECK(root.firstChild() == nullptr);
    root.addChild(label);
    CHECK(label.ui() == &ui);
}

void testPointer() {
    TestDisplay test;
    Panel root;
    root.setBackground(darkTheme().background);
    Button button("Click");
    button.setRect(20, 20, 60, 30);
    ClickCounter counter;
    button.setAction(&onClick, &counter);
    root.addChild(button);
    Ui ui(test.display, root, g_band, kBandHeight);

    CHECK(!ui.pointerDown(2, 2));
    CHECK(!ui.pointerCaptured());

    CHECK(ui.pointerDown(30, 30));
    CHECK(button.pressed());
    CHECK(ui.pointerCaptured());
    CHECK(ui.focused() == &button);
    CHECK(ui.pointerUp(35, 35));
    CHECK(!button.pressed());
    CHECK(counter.clicks == 1);

    CHECK(ui.pointerDown(30, 30));
    CHECK(ui.pointerUp(200, 200));
    CHECK(counter.clicks == 1);
    CHECK(!ui.pointerCaptured());

    CHECK(ui.pointerDown(30, 30));
    ui.pointerCancel();
    CHECK(!button.pressed());
    CHECK(counter.clicks == 1);

    button.setEnabled(false);
    CHECK(!ui.pointerDown(30, 30));
    CHECK(counter.clicks == 1);
    button.setEnabled(true);
}

void testCheckboxAndProgress() {
    TestDisplay test;
    Panel root;
    root.setBackground(darkTheme().background);
    Checkbox check("Auto", false);
    check.setRect(10, 10, 120, 30);
    ClickCounter counter;
    check.setToggled(&onToggle, &counter);
    ProgressBar progress(42);
    progress.setRect(10, 60, 120, 10);
    root.addChild(check);
    root.addChild(progress);
    Ui ui(test.display, root, g_band, kBandHeight);

    CHECK(!check.checked());
    CHECK(ui.pointerDown(20, 20));
    CHECK(ui.pointerUp(20, 20));
    CHECK(check.checked());
    CHECK(counter.toggles == 1);
    CHECK(counter.lastChecked);

    CHECK(ui.pointerDown(20, 20));
    CHECK(ui.pointerUp(20, 20));
    CHECK(!check.checked());
    CHECK(counter.toggles == 2);
    CHECK(!counter.lastChecked);

    CHECK(ui.keyDown(Key::Space));
    CHECK(check.checked());
    CHECK(counter.toggles == 3);

    progress.setValue(200);
    CHECK(progress.value() == 100);
    progress.setValue(0);
    CHECK(progress.value() == 0);
    ui.render();
}

void testKeyboardAndFocusRing() {
    TestDisplay test;
    const Theme& theme = darkTheme();
    Panel root;
    root.setBackground(theme.background);
    Button button("Go");
    button.setRect(40, 40, 80, 30);
    ClickCounter counter;
    button.setAction(&onClick, &counter);
    root.addChild(button);
    Ui ui(test.display, root, g_band, kBandHeight);

    CHECK(ui.focused() == nullptr);
    CHECK(ui.keyDown(Key::Tab));
    CHECK(ui.focused() == &button);
    CHECK(ui.keyDown(Key::Enter));
    CHECK(counter.clicks == 1);

    ui.render();
    CHECK(test.pixel(38, 38) == theme.focus.value());

    test.resetStats();
    CHECK(ui.pointerDown(50, 50));
    CHECK(ui.pointerUp(50, 50));
    CHECK(counter.clicks == 2);
    ui.render();
    CHECK(test.pixel(38, 38) != theme.focus.value());

    ui.clearFocus();
    CHECK(ui.focused() == nullptr);
}

void testSmallBandBuffer() {
    TestDisplay test;
    Panel root;
    uint16_t tiny[4];
    Ui ui(test.display, root, tiny, 4, 10);
    CHECK(!ui.valid());
    ui.invalidateAll();
    ui.render();
    CHECK(test.flushes == 0);
}

void testSlider() {
    TestDisplay test;
    Panel root;
    Slider slider(50);
    slider.setRect(10, 10, 100, 20);
    ValueTracker tracker;
    slider.setChanged(&onSliderChanged, &tracker);
    root.addChild(slider);
    Ui ui(test.display, root, g_band, kBandHeight);

    CHECK(slider.value() == 50);
    CHECK(ui.pointerDown(109, 20));   // local x = 99 -> value 100
    CHECK(ui.focused() == &slider);
    CHECK(slider.value() == 100);
    CHECK(tracker.changes == 1);
    ui.pointerMove(60, 20);           // local x = 50 -> value 50
    CHECK(slider.value() == 50);
    ui.pointerUp(60, 20);
    CHECK(tracker.changes == 2);

    slider.setStep(10);
    CHECK(ui.keyDown(Key::Left));
    CHECK(slider.value() == 40);
    CHECK(ui.keyDown(Key::Right));
    CHECK(slider.value() == 50);
    slider.setValue(200);
    CHECK(slider.value() == 100);
    CHECK(tracker.changes == 5);
    CHECK(ui.keyDown(Key::Right));
    CHECK(slider.value() == 100);
    CHECK(tracker.changes == 5);
    ui.render();
}

void testSwitch() {
    TestDisplay test;
    Panel root;
    Switch toggle(false);
    toggle.setRect(10, 10, 60, 30);
    ClickCounter counter;
    toggle.setToggled(&onToggle, &counter);
    root.addChild(toggle);
    Ui ui(test.display, root, g_band, kBandHeight);

    CHECK(!toggle.checked());
    CHECK(ui.pointerDown(20, 20));
    CHECK(ui.pointerUp(20, 20));
    CHECK(toggle.checked());
    CHECK(counter.toggles == 1);
    CHECK(counter.lastChecked);

    CHECK(ui.pointerDown(20, 20));
    CHECK(ui.pointerUp(20, 20));
    CHECK(!toggle.checked());
    CHECK(counter.toggles == 2);

    CHECK(ui.keyDown(Key::Space));
    CHECK(toggle.checked());
    CHECK(counter.toggles == 3);
    toggle.setChecked(false);
    CHECK(!toggle.checked());
    CHECK(counter.toggles == 3);
    ui.render();
}

void testListBox() {
    static const char* const items[] = {"One", "Two", "Three", "Four", "Five"};
    TestDisplay test;
    Panel root;
    ListBox list(items, 5, 10);
    list.setRect(10, 10, 100, 20);
    ListTracker tracker;
    list.setSelectedCallback(&onListSelected, &tracker);
    root.addChild(list);
    Ui ui(test.display, root, g_band, kBandHeight);

    CHECK(list.count() == 5);
    CHECK(list.selected() == ListBox::kNone);
    CHECK(ui.pointerDown(20, 25));   // local y = 15 -> row 1
    CHECK(ui.pointerUp(20, 25));
    CHECK(list.selected() == 1);
    CHECK(tracker.selections == 1);
    CHECK(tracker.last == 1);

    CHECK(ui.keyDown(Key::Down));
    CHECK(list.selected() == 2);
    CHECK(ui.keyDown(Key::Down));
    CHECK(list.selected() == 3);
    CHECK(list.topRow() == 2);
    CHECK(ui.keyDown(Key::Enter));
    CHECK(tracker.selections == 4);
    CHECK(tracker.last == 3);

    list.scrollTo(4);
    CHECK(list.topRow() == 3);
    list.scrollBy(-1);
    CHECK(list.topRow() == 2);
    list.setSelected(0);
    CHECK(list.topRow() == 0);

    CHECK(ui.pointerDown(20, 25));
    ui.pointerMove(20, 5);           // drag up by 20 px -> scroll 2 rows
    ui.pointerUp(20, 5);
    CHECK(list.topRow() == 2);
    CHECK(list.selected() == 0);
    ui.render();
}

void testEditBox() {
    TestDisplay test;
    Panel root;
    char buffer[8];
    EditBox edit(buffer, sizeof(buffer));
    edit.setRect(10, 10, 80, 24);
    TextTracker tracker;
    edit.setChanged(&onEditChanged, &tracker);
    edit.setSubmitted(&onEditSubmitted, &tracker);
    root.addChild(edit);
    Ui ui(test.display, root, g_band, kBandHeight);

    edit.requestFocus();
    CHECK(ui.focused() == &edit);
    CHECK(ui.inputChar('h'));
    CHECK(ui.inputChar('i'));
    CHECK(std::strcmp(edit.text(), "hi") == 0);
    CHECK(tracker.changes == 2);
    CHECK(edit.cursor() == 2);

    ui.keyDown(Key::Left);
    CHECK(edit.cursor() == 1);
    CHECK(ui.inputChar('X'));
    CHECK(std::strcmp(edit.text(), "hXi") == 0);
    CHECK(edit.cursor() == 2);
    ui.keyDown(Key::Backspace);
    CHECK(std::strcmp(edit.text(), "hi") == 0);
    CHECK(edit.cursor() == 1);
    ui.keyDown(Key::Delete);
    CHECK(std::strcmp(edit.text(), "h") == 0);

    edit.setMaxLength(2);
    edit.setText("abcdef");
    CHECK(std::strcmp(edit.text(), "ab") == 0);
    CHECK(edit.maxLength() == 2);
    CHECK(edit.cursor() == 2);
    ui.keyDown(Key::Enter);
    CHECK(tracker.submits == 1);
    CHECK(std::strcmp(tracker.last, "ab") == 0);

    edit.setMaxLength(0);   // back to capacity - 1
    edit.setText("abcd");
    edit.setCursor(0);
    CHECK(ui.pointerDown(15, 22));   // local x = 5 -> cursor 0
    CHECK(ui.pointerUp(15, 22));
    CHECK(edit.cursor() == 0);

    edit.setRect(10, 10, 30, 24);
    edit.setText("1234567");
    ui.render();
}

void testBitmap() {
    uint16_t pixels[12 * 8];
    Canvas canvas(pixels, 12, 12, 8, 0, 0, Rect(0, 0, 12, 8),
                  darkTheme());
    const uint16_t bitmap[4] = {0xF800, 0x07E0, 0x001F, 0xFFFF};
    canvas.fillRect(Rect(0, 0, 12, 8), palette::black());
    canvas.drawBitmap(2, 1, bitmap, 2, 2);
    CHECK(pixels[1 * 12 + 2] == 0xF800);
    CHECK(pixels[1 * 12 + 3] == 0x07E0);
    CHECK(pixels[2 * 12 + 2] == 0x001F);
    CHECK(pixels[2 * 12 + 3] == 0xFFFF);

    canvas.setClip(Rect(3, 2, 1, 1));
    canvas.drawBitmap(2, 1, bitmap, 2, 2);
    CHECK(pixels[2 * 12 + 3] == 0xFFFF);
    CHECK(pixels[2 * 12 + 2] == 0x001F);

    canvas.setClip(Rect(0, 0, 12, 8));
    canvas.fillRect(Rect(0, 0, 12, 8), palette::black());
    static const uint8_t mask[2] = {0x80, 0x40};
    canvas.drawBitmap1bpp(0, 0, mask, 2, 2, palette::green());
    CHECK(pixels[0] == palette::green().value());
    CHECK(pixels[1] == palette::black().value());
    CHECK(pixels[12] == palette::black().value());
    CHECK(pixels[12 + 1] == palette::green().value());

    TestDisplay test;
    Panel root;
    Image image(bitmap, 2, 2);
    image.setRect(5, 5, 2, 2);
    root.addChild(image);
    Ui ui(test.display, root, g_band, kBandHeight);
    ui.render();
    CHECK(test.pixel(5, 5) == 0xF800);
    CHECK(test.pixel(6, 5) == 0x07E0);
    CHECK(test.pixel(5, 6) == 0x001F);
    CHECK(test.pixel(6, 6) == 0xFFFF);
}

void testLabelEllipsis() {
    const Theme& theme = darkTheme();
    TestDisplay test;
    Panel root;
    root.setBackground(theme.surface);
    Label label("ABCDEFGHIJKLMNOP");
    label.setRect(10, 10, 40, 12);
    root.addChild(label);
    Ui ui(test.display, root, g_band, kBandHeight);

    CHECK(label.ellipsis());
    ui.render();
    int textPixels = 0;
    for (int y = 10; y < 22; ++y) {
        for (int x = 10; x < 50; ++x) {
            if (test.pixel(x, y) != theme.surface.value()) {
                ++textPixels;
            }
        }
    }
    CHECK(textPixels > 0);

    label.setEllipsis(false);
    CHECK(!label.ellipsis());
    ui.render();
}

void testPanelDemo() {
    TestDisplay test;
    const Theme& theme = darkTheme();
    Panel root;
    root.setBackground(theme.background);
    root.setRadius(0);

    Panel card;
    card.setRect(8, 8, 144, 60);
    Label title("Pixmote");
    title.setRect(8, 8, 128, 16);
    title.setScale(2);
    title.setColor(theme.text);
    card.addChild(title);
    root.addChild(card);

    Ui ui(test.display, root, g_band, kBandHeight);
    ui.invalidateAll();
    ui.render();
    CHECK(test.pixel(0, 0) == theme.background.value());
    CHECK(test.flushes > 0);

    test.resetStats();
    ui.render();
    CHECK(test.flushes == 0);
}

}  // namespace

int main() {
    testCanvas();
    testLayoutAndDamage();
    testPointer();
    testCheckboxAndProgress();
    testKeyboardAndFocusRing();
    testSmallBandBuffer();
    testSlider();
    testSwitch();
    testListBox();
    testEditBox();
    testBitmap();
    testLabelEllipsis();
    testPanelDemo();

    if (g_failures != 0) {
        std::printf("%d test(s) failed\n", g_failures);
        return 1;
    }
    std::printf("all Pixmote tests passed\n");
    return 0;
}
