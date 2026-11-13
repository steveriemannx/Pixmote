// Headless desktop demo. Renders a 320x340 RGB565 UI, drives it with synthetic
// pointer/keyboard input, and writes PPM screenshots.
#include "pixmote.hpp"

#include <cstdio>
#include <cstring>

using namespace pixmote;

namespace {

const int kScreenW = 320;
const int kScreenH = 340;
const uint16_t kBandHeight = 16;

constinit uint16_t g_framebuffer[kScreenW * kScreenH];
constinit uint16_t g_band[kScreenW * kBandHeight];

const char* const g_items[] = {"Brightness", "Contrast", "Language", "Reset",
                               "About"};

struct App {
    Label* cpuValue;
    Label* ledValue;
    Label* clicks;
    ProgressBar* progress;
    Label* levelValue;
    Label* selectedValue;
    Label* autoValue;
    int cpu;
    int clickCount;
    bool ledOn;
};

constinit App g_app = {nullptr, nullptr, nullptr, nullptr,
                       nullptr, nullptr, nullptr, 42, 0, false};

// Labels keep the pointer they are given, so the backing buffers must outlive
// the UI. Never pass a temporary or a stack buffer to Label::setText().
constinit char g_cpuText[8] = "42";
constinit char g_clicksText[24] = "CLICKS 0";
constinit char g_levelText[16] = "42%";
constinit char g_selectedText[32] = "SELECTED -";
constinit char g_switchText[12] = "AUTO OFF";
constinit char g_nameText[16] = "guest";

void flush(void*, int16_t x, int16_t y, int16_t w, int16_t h,
           const uint16_t* pixels, uint16_t stride) {
    for (int row = 0; row < h; ++row) {
        std::memcpy(&g_framebuffer[(y + row) * kScreenW + x],
                    pixels + row * stride,
                    static_cast<size_t>(w) * sizeof(uint16_t));
    }
}

void onToggleLed(void*) {
    App& app = g_app;
    app.ledOn = !app.ledOn;
    ++app.clickCount;
    app.cpu += 35;
    if (app.cpu > 99) {
        app.cpu = 9;
    }

    std::snprintf(g_cpuText, sizeof(g_cpuText), "%u",
                  static_cast<unsigned>(app.cpu));
    app.cpuValue->setText(g_cpuText);
    app.progress->setValue(static_cast<uint8_t>(app.cpu));

    app.ledValue->setText(app.ledOn ? "ON" : "OFF");
    app.ledValue->setColor(app.ledOn ? darkTheme().success
                                     : darkTheme().text_muted);

    std::snprintf(g_clicksText, sizeof(g_clicksText), "CLICKS %d",
                  app.clickCount);
    app.clicks->setText(g_clicksText);
}

void onLevel(void*, uint8_t value) {
    std::snprintf(g_levelText, sizeof(g_levelText), "%u%%",
                  static_cast<unsigned>(value));
    g_app.levelValue->setText(g_levelText);
}

void onSelected(void*, uint16_t index) {
    const unsigned count =
        static_cast<unsigned>(sizeof(g_items) / sizeof(g_items[0]));
    if (index < count) {
        std::snprintf(g_selectedText, sizeof(g_selectedText), "SELECTED %s",
                      g_items[index]);
        g_app.selectedValue->setText(g_selectedText);
    }
}

void onAuto(void*, bool checked) {
    std::snprintf(g_switchText, sizeof(g_switchText), "AUTO %s",
                  checked ? "ON" : "OFF");
    g_app.autoValue->setText(g_switchText);
}

bool writePpm(const char* path) {
    std::FILE* file = std::fopen(path, "wb");
    if (file == nullptr) {
        return false;
    }
    std::fprintf(file, "P6\n%d %d\n255\n", kScreenW, kScreenH);
    for (int y = 0; y < kScreenH; ++y) {
        for (int x = 0; x < kScreenW; ++x) {
            const Color c(g_framebuffer[y * kScreenW + x]);
            const unsigned char rgb[3] = {c.r(), c.g(), c.b()};
            std::fwrite(rgb, 1, 3, file);
        }
    }
    const bool ok = std::ferror(file) == 0;
    std::fclose(file);
    return ok;
}

}  // namespace

int main(int argc, char** argv) {
    const char* prefix = argc > 1 ? argv[1] : "demo";

    const Theme& theme = darkTheme();
    Display display{
        .width = kScreenW,
        .height = kScreenH,
        .context = nullptr,
        .flush = &flush,
    };

    Panel root;
    root.setBackground(theme.background);
    root.setRadius(0);

    Label brand("Pixmote");
    brand.setRect(16, 12, 180, 24);
    brand.setScale(2);
    brand.setColor(theme.text);
    root.addChild(brand);

    Label tagline("RETAINED / NO HEAP / C++20");
    tagline.setRect(16, 38, 210, 10);
    tagline.setColor(theme.text_muted);
    root.addChild(tagline);

    Panel pill;
    pill.setRect(238, 16, 66, 24);
    pill.setBackground(theme.primary);
    pill.setRadius(12);
    root.addChild(pill);

    Label online("ONLINE");
    online.setRect(238, 16, 66, 24);
    online.setAlign(Align::Center);
    online.setColor(theme.on_primary);
    root.addChild(online);

    Panel card;
    card.setRect(16, 62, 288, 94);
    card.setBackground(theme.surface);
    card.setBorder(theme.border, 1);
    card.setRadius(12);
    root.addChild(card);

    // Coordinates are relative to the parent: card content coordinates are
    // card-local, so a child at (16, 12) renders at screen (32, 74).
    Label caption("CPU LOAD");
    caption.setRect(16, 12, 120, 10);
    caption.setColor(theme.text_muted);
    card.addChild(caption);

    Label cpuValue(g_cpuText);
    cpuValue.setRect(14, 24, 64, 36);
    cpuValue.setScale(4);
    cpuValue.setColor(theme.text);
    card.addChild(cpuValue);

    Label percent("%");
    percent.setRect(66, 41, 24, 16);
    percent.setScale(2);
    percent.setColor(theme.primary);
    card.addChild(percent);

    Label ledCaption("LED");
    ledCaption.setRect(216, 12, 56, 10);
    ledCaption.setAlign(Align::Right);
    ledCaption.setColor(theme.text_muted);
    card.addChild(ledCaption);

    Label ledValue("OFF");
    ledValue.setRect(192, 24, 80, 20);
    ledValue.setAlign(Align::Right);
    ledValue.setScale(2);
    ledValue.setColor(theme.text_muted);
    card.addChild(ledValue);

    ProgressBar progress(42);
    progress.setRect(16, 64, 256, 8);
    card.addChild(progress);

    Label clicks(g_clicksText);
    clicks.setRect(16, 78, 140, 8);
    clicks.setColor(theme.text_muted);
    card.addChild(clicks);

    Label bandInfo("BAND 16 ROWS / 4 DIRTY RECTS");
    bandInfo.setRect(80, 78, 192, 8);
    bandInfo.setAlign(Align::Right);
    bandInfo.setColor(theme.text_muted);
    card.addChild(bandInfo);

    Button toggle("Toggle LED");
    toggle.setRect(16, 170, 152, 44);
    toggle.setScale(2);
    toggle.setAction(&onToggleLed);
    root.addChild(toggle);

    Switch autoRefresh(false);
    autoRefresh.setRect(184, 178, 40, 24);
    autoRefresh.setToggled(&onAuto);
    root.addChild(autoRefresh);

    Label autoValue(g_switchText);
    autoValue.setRect(232, 178, 88, 24);
    autoValue.setColor(theme.text_muted);
    root.addChild(autoValue);

    Label levelCaption("LEVEL");
    levelCaption.setRect(16, 224, 80, 8);
    levelCaption.setColor(theme.text_muted);
    root.addChild(levelCaption);

    Label levelValue(g_levelText);
    levelValue.setRect(200, 224, 104, 8);
    levelValue.setAlign(Align::Right);
    levelValue.setColor(theme.text_muted);
    root.addChild(levelValue);

    Slider level(42);
    level.setRect(16, 236, 288, 16);
    level.setChanged(&onLevel);
    root.addChild(level);

    ListBox list(g_items, 5, 18);
    list.setRect(16, 262, 140, 54);
    list.setSelectedCallback(&onSelected);
    root.addChild(list);

    EditBox name(g_nameText, sizeof(g_nameText));
    name.setRect(168, 262, 136, 24);
    name.setText("guest");
    root.addChild(name);

    Label nameCaption("NAME");
    nameCaption.setRect(168, 290, 60, 8);
    nameCaption.setColor(theme.text_muted);
    root.addChild(nameCaption);

    Label selectedValue(g_selectedText);
    selectedValue.setRect(168, 302, 136, 8);
    selectedValue.setColor(theme.text_muted);
    root.addChild(selectedValue);

    Label hint("TAB focus   DRAG slider   TYPE in NAME");
    hint.setRect(16, 320, 288, 8);
    hint.setColor(theme.text_muted);
    root.addChild(hint);

    g_app.cpuValue = &cpuValue;
    g_app.ledValue = &ledValue;
    g_app.clicks = &clicks;
    g_app.progress = &progress;
    g_app.levelValue = &levelValue;
    g_app.selectedValue = &selectedValue;
    g_app.autoValue = &autoValue;

    Ui ui(display, root, g_band, kBandHeight);
    if (!ui.valid()) {
        std::fprintf(stderr, "invalid display or band buffer\n");
        return 1;
    }

    ui.render();
    char path[256];
    std::snprintf(path, sizeof(path), "%s_initial.ppm", prefix);
    if (!writePpm(path)) {
        std::fprintf(stderr, "cannot write %s\n", path);
        return 1;
    }

    // Simulate a touch tap on the button.
    const int bx = toggle.screenX() + toggle.width() / 2;
    const int by = toggle.screenY() + toggle.height() / 2;
    ui.pointerDown(static_cast<int16_t>(bx), static_cast<int16_t>(by));
    ui.pointerUp(static_cast<int16_t>(bx), static_cast<int16_t>(by));

    // Toggle the switch.
    const int sx = autoRefresh.screenX() + autoRefresh.width() / 2;
    const int sy = autoRefresh.screenY() + autoRefresh.height() / 2;
    ui.pointerDown(static_cast<int16_t>(sx), static_cast<int16_t>(sy));
    ui.pointerUp(static_cast<int16_t>(sx), static_cast<int16_t>(sy));

    // Drag the slider to about 75%.
    const int lx = level.screenX() + level.width() * 3 / 4;
    const int ly = level.screenY() + level.height() / 2;
    ui.pointerDown(static_cast<int16_t>(lx), static_cast<int16_t>(ly));
    ui.pointerUp(static_cast<int16_t>(lx), static_cast<int16_t>(ly));

    // Select the second list row.
    const int rowY = list.screenY() + list.rowHeight() +
                     static_cast<int>(list.rowHeight()) / 2;
    ui.pointerDown(static_cast<int16_t>(list.screenX() + 20),
                   static_cast<int16_t>(rowY));
    ui.pointerUp(static_cast<int16_t>(list.screenX() + 20),
                 static_cast<int16_t>(rowY));

    // Focus the edit box and append two characters.
    const int ex = name.screenX() + static_cast<int>(name.width()) - 8;
    const int ey = name.screenY() + name.height() / 2;
    ui.pointerDown(static_cast<int16_t>(ex), static_cast<int16_t>(ey));
    ui.pointerUp(static_cast<int16_t>(ex), static_cast<int16_t>(ey));
    ui.inputChar('T');
    ui.inputChar('V');
    ui.render();

    std::snprintf(path, sizeof(path), "%s.ppm", prefix);
    if (!writePpm(path)) {
        std::fprintf(stderr, "cannot write %s\n", path);
        return 1;
    }

    // Show the focus ring on the list box.
    ui.focus(&list);
    ui.render();
    std::snprintf(path, sizeof(path), "%s_focus.ppm", prefix);
    if (!writePpm(path)) {
        std::fprintf(stderr, "cannot write %s\n", path);
        return 1;
    }

    std::printf("wrote %s_initial.ppm, %s.ppm, %s_focus.ppm\n", prefix, prefix,
                prefix);
    std::printf("flush bands: %u rows, dirty capacity: %u rects\n",
                static_cast<unsigned>(ui.bandHeight()),
                static_cast<unsigned>(PIXMOTE_MAX_DIRTY_RECTS));
    return 0;
}
