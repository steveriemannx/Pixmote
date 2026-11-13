// Verifies that all UI operations stay allocation-free.
#include "pixmote.hpp"

#include <cstdlib>
#include <new>

using namespace pixmote;

namespace {
constinit unsigned long g_allocations = 0;

constinit uint16_t g_framebuffer[96 * 64];
constinit uint16_t g_band[96 * 8];
const uint16_t g_icon[4] = {0xF800, 0x07E0, 0x001F, 0xFFFF};

void flush(void*, int16_t x, int16_t y, int16_t w, int16_t h,
           const uint16_t* pixels, uint16_t stride) {
    for (int row = 0; row < h; ++row) {
        for (int col = 0; col < w; ++col) {
            g_framebuffer[(y + row) * 96 + x + col] =
                pixels[row * stride + col];
        }
    }
}

void click(void*) {}
void toggled(void*, bool) {}
void valueChanged(void*, uint8_t) {}
void selected(void*, uint16_t) {}
void textChanged(void*, const char*) {}
void submitted(void*, const char*) {}

}  // namespace

void* operator new(std::size_t size) {
    ++g_allocations;
    void* p = std::malloc(size == 0 ? 1 : size);
    if (p == nullptr) {
        std::abort();
    }
    return p;
}

void* operator new[](std::size_t size) {
    ++g_allocations;
    void* p = std::malloc(size == 0 ? 1 : size);
    if (p == nullptr) {
        std::abort();
    }
    return p;
}

void operator delete(void* p) noexcept {
    std::free(p);
}

void operator delete[](void* p) noexcept {
    std::free(p);
}

void operator delete(void* p, std::size_t) noexcept {
    std::free(p);
}

void operator delete[](void* p, std::size_t) noexcept {
    std::free(p);
}

int main() {
    const unsigned long before = g_allocations;
    {
        Display display{
            .width = 96,
            .height = 64,
            .context = nullptr,
            .flush = &flush,
        };

        Panel root;
        root.setBackground(darkTheme().background);
        root.setRadius(0);

        Label label("0 allocations");
        label.setRect(6, 4, 84, 14);
        root.addChild(label);

        Button button("Click");
        button.setRect(6, 22, 84, 20);
        button.setAction(&click, nullptr);
        button.setFocusable(true);
        root.addChild(button);

        Checkbox check("Auto");
        check.setRect(6, 46, 84, 16);
        check.setToggled(&toggled, nullptr);
        root.addChild(check);

        ProgressBar bar(50);
        bar.setRect(6, 16, 84, 4);
        root.addChild(bar);

        Slider slider(30);
        slider.setRect(6, 44, 40, 8);
        slider.setChanged(&valueChanged, nullptr);
        root.addChild(slider);

        Switch toggle(true);
        toggle.setRect(56, 32, 32, 12);
        toggle.setToggled(&toggled, nullptr);
        root.addChild(toggle);

        static const char* const items[] = {"A", "B", "C"};
        ListBox list(items, 3, 6);
        list.setRect(6, 52, 40, 12);
        list.setSelectedCallback(&selected, nullptr);
        root.addChild(list);

        char editBuffer[8];
        EditBox edit(editBuffer, sizeof(editBuffer));
        edit.setRect(50, 52, 42, 12);
        edit.setChanged(&textChanged, nullptr);
        edit.setSubmitted(&submitted, nullptr);
        root.addChild(edit);

        Image image(g_icon, 2, 2);
        image.setRect(92, 2, 2, 2);
        root.addChild(image);

        Ui ui(display, root, g_band, 8);
        ui.render();
        ui.pointerDown(20, 30);
        ui.pointerMove(21, 31);
        ui.pointerUp(22, 32);
        ui.pointerDown(60, 34);
        ui.pointerUp(60, 34);
        ui.pointerDown(20, 48);
        ui.pointerMove(30, 48);
        ui.pointerUp(30, 48);
        ui.pointerDown(10, 56);
        ui.pointerUp(10, 56);
        ui.pointerDown(60, 56);
        ui.pointerUp(60, 56);
        ui.inputChar('x');
        ui.keyDown(Key::Backspace);
        ui.keyDown(Key::Enter);
        ui.keyDown(Key::Tab);
        ui.keyDown(Key::Enter);
        bar.setValue(75);
        label.setText("still no heap");
        ui.render();
        ui.invalidateAll();
        ui.render();
        ui.clearFocus();
    }
    if (g_allocations != before) {
        return 1;
    }
    return 0;
}
