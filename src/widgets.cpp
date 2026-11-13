#include "pixmote/widgets.hpp"

#include "pixmote/ui.hpp"

namespace pixmote {
namespace {

constexpr int minInt(int a, int b) {
    return a < b ? a : b;
}

constexpr int maxInt(int a, int b) {
    return a > b ? a : b;
}

const Font& fontOf(const Canvas& canvas) {
    return canvas.theme().font != nullptr ? *canvas.theme().font : font5x7;
}

const Font& fontOf(const Widget& widget) {
    const Ui* ui = widget.ui();
    return ui != nullptr && ui->theme().font != nullptr ? *ui->theme().font
                                                       : font5x7;
}

void drawClippedText(Canvas& canvas, const Rect& box, const char* text,
                     Color color, const Font& font, uint8_t scale, Align align) {
    const Rect old = canvas.clip();
    canvas.setClip(old.intersect(box));
    canvas.drawText(box, text, color, font, scale, align);
    canvas.setClip(old);
}

// Single-line overflow is rendered as "..." instead of cutting a glyph in half.
void drawEllipsizedText(Canvas& canvas, const Rect& box, const char* text,
                        Color color, const Font& font, uint8_t scale,
                        Align align) {
    if (text == nullptr || box.empty() || scale == 0) {
        return;
    }
    const int fullWidth = measureTextWidth(font, text, scale);
    bool singleLine = true;
    for (const char* p = text; *p != '\0'; ++p) {
        if (*p == '\n') {
            singleLine = false;
            break;
        }
    }
    if (fullWidth <= box.w || !singleLine) {
        drawClippedText(canvas, box, text, color, font, scale, align);
        return;
    }

    const int ellipsisWidth = measureTextWidth(font, "...", scale);
    const int advance = font.advance() * scale;
    const int spacing = font.spacing * scale;
    const int maxPrefix = static_cast<int>(box.w) - ellipsisWidth;
    int chars = 0;
    if (advance > 0 && maxPrefix > 0) {
        chars = (maxPrefix + spacing) / advance;
    }
    int length = 0;
    while (text[length] != '\0') {
        ++length;
    }
    if (chars > length) {
        chars = length;
    }
    if (chars < 0) {
        chars = 0;
    }
    const int prefixWidth = chars > 0 ? chars * advance - spacing : 0;
    const int totalWidth = prefixWidth + ellipsisWidth;
    int x = box.x;
    if (align == Align::Center) {
        x = box.x + (static_cast<int>(box.w) - totalWidth) / 2;
    } else if (align == Align::Right) {
        x = box.x + static_cast<int>(box.w) - totalWidth;
    }
    const int y = box.y +
                  (static_cast<int>(box.h) - font.height * scale) / 2;

    const Rect old = canvas.clip();
    canvas.setClip(old.intersect(box));
    char glyph[2] = {0, 0};
    for (int i = 0; i < chars; ++i) {
        glyph[0] = text[i];
        canvas.drawText(x, y, glyph, color, font, scale);
        x += advance;
    }
    canvas.drawText(x, y, "...", color, font, scale);
    canvas.setClip(old);
}

}  // namespace

Panel::Panel()
    : background_(),
      border_(),
      radius_(-1),
      borderWidth_(0),
      hasBackground_(true),
      themedBackground_(true) {}

void Panel::setBackground(Color color) {
    background_ = color;
    hasBackground_ = true;
    themedBackground_ = false;
    invalidate();
}

void Panel::useThemeBackground() {
    themedBackground_ = true;
    hasBackground_ = true;
    invalidate();
}

void Panel::setTransparent() {
    hasBackground_ = false;
    themedBackground_ = false;
    invalidate();
}

void Panel::setBorder(Color color, uint8_t width) {
    border_ = color;
    borderWidth_ = width == 0 ? 1 : width;
    if (borderWidth_ > 4) {
        borderWidth_ = 4;
    }
    invalidate();
}

void Panel::clearBorder() {
    borderWidth_ = 0;
    invalidate();
}

void Panel::setRadius(int16_t radius) {
    radius_ = radius;
    invalidate();
}

void Panel::paint(Canvas& canvas, int x, int y) {
    const Rect r(static_cast<int16_t>(x), static_cast<int16_t>(y), width(),
                 height());
    const int radius = radius_ < 0 ? canvas.theme().radius : radius_;
    const Color background =
        themedBackground_ ? canvas.theme().surface : background_;

    if (borderWidth_ > 0 && hasBackground_) {
        canvas.fillRoundRect(r, static_cast<uint16_t>(radius), border_);
        const Rect inner = r.inset(borderWidth_);
        canvas.fillRoundRect(inner,
                             static_cast<uint16_t>(maxInt(0, radius - borderWidth_)),
                             background);
    } else if (hasBackground_) {
        canvas.fillRoundRect(r, static_cast<uint16_t>(radius), background);
    } else if (borderWidth_ > 0) {
        canvas.drawRect(r, border_, borderWidth_);
    }
}

Label::Label(const char* text)
    : text_(text),
      color_(),
      scale_(1),
      hasColor_(false),
      ellipsis_(true),
      align_(Align::Left) {}

void Label::setText(const char* text) {
    text_ = text;
    invalidate();
}

void Label::setAlign(Align align) {
    align_ = align;
    invalidate();
}

void Label::setScale(uint8_t scale) {
    scale_ = scale == 0 ? 1 : scale;
    invalidate();
}

void Label::setColor(Color color) {
    color_ = color;
    hasColor_ = true;
    invalidate();
}

void Label::useThemeColor() {
    hasColor_ = false;
    invalidate();
}

void Label::setEllipsis(bool enabled) {
    ellipsis_ = enabled;
    invalidate();
}

void Label::fit(const Font& font) {
    setSize(static_cast<uint16_t>(maxInt(0, measureTextWidth(font, text_, scale_))),
            static_cast<uint16_t>(maxInt(0, measureTextHeight(font, text_, scale_))));
}

void Label::paint(Canvas& canvas, int x, int y) {
    if (text_ == nullptr || width() == 0 || height() == 0) {
        return;
    }
    const Rect r(static_cast<int16_t>(x), static_cast<int16_t>(y), width(),
                 height());
    const Color color =
        hasColor_ ? color_ : (enabledInTree() ? canvas.theme().text
                                              : canvas.theme().text_muted);
    if (ellipsis_) {
        drawEllipsizedText(canvas, r, text_, color, fontOf(canvas), scale_,
                           align_);
    } else {
        drawClippedText(canvas, r, text_, color, fontOf(canvas), scale_,
                        align_);
    }
}

Button::Button(const char* text, Action action, void* context)
    : text_(text),
      action_(action),
      context_(context),
      kind_(ButtonKind::Primary),
      radius_(-1),
      scale_(1),
      pressed_(false) {
    setFocusable(true);
}

void Button::setText(const char* text) {
    text_ = text;
    invalidate();
}

void Button::setAction(Action action, void* context) {
    action_ = action;
    context_ = context;
}

void Button::setKind(ButtonKind kind) {
    kind_ = kind;
    invalidate();
}

void Button::setScale(uint8_t scale) {
    scale_ = scale == 0 ? 1 : scale;
    invalidate();
}

void Button::setRadius(int16_t radius) {
    radius_ = radius;
    invalidate();
}

void Button::fit(const Font& font, uint16_t padX, uint16_t padY) {
    const int textW = measureTextWidth(font, text_, scale_);
    const int textH = measureTextHeight(font, text_, scale_);
    setSize(static_cast<uint16_t>(textW + 2 * padX),
            static_cast<uint16_t>(textH + 2 * padY));
}

void Button::activate() {
    if (!enabledInTree()) {
        return;
    }
    if (action_ != nullptr) {
        action_(context_);
    }
}

void Button::paint(Canvas& canvas, int x, int y) {
    const Rect r(static_cast<int16_t>(x), static_cast<int16_t>(y), width(),
                 height());
    if (r.empty()) {
        return;
    }
    const int radius = radius_ < 0 ? canvas.theme().radius : radius_;
    const bool on = enabledInTree();
    Color background;
    Color foreground;
    Color border = canvas.theme().border;

    if (!on) {
        background = canvas.theme().surface_alt;
        foreground = canvas.theme().text_muted;
    } else if (kind_ == ButtonKind::Primary) {
        background = pressed_ ? canvas.theme().primary_pressed
                              : canvas.theme().primary;
        foreground = canvas.theme().on_primary;
    } else {
        background = pressed_ ? canvas.theme().surface
                              : canvas.theme().surface_alt;
        foreground = canvas.theme().text;
        border = pressed_ ? canvas.theme().primary : canvas.theme().border;
    }

    if (kind_ == ButtonKind::Secondary && on && height() > 2) {
        canvas.fillRoundRect(r, static_cast<uint16_t>(radius), border);
        const Rect inner = r.inset(1);
        canvas.fillRoundRect(inner,
                             static_cast<uint16_t>(maxInt(0, radius - 1)),
                             background);
    } else {
        canvas.fillRoundRect(r, static_cast<uint16_t>(radius), background);
    }

    Rect textBox = r;
    if (pressed_ && on) {
        textBox.y = static_cast<int16_t>(textBox.y + 1);
    }
    drawClippedText(canvas, textBox, text_, foreground, fontOf(canvas), scale_,
                    Align::Center);
}

bool Button::onPointerDown([[maybe_unused]] int16_t x,
                           [[maybe_unused]] int16_t y) {
    if (!enabledInTree()) {
        return false;
    }
    if (!pressed_) {
        pressed_ = true;
        invalidate();
    }
    return true;
}

void Button::onPointerUp(int16_t x, int16_t y) {
    if (!pressed_) {
        return;
    }
    pressed_ = false;
    invalidate();
    if (!enabledInTree()) {
        return;
    }
    if (x >= 0 && y >= 0 && x < width() && y < height()) {
        activate();
    }
}

bool Button::onKey(Key key) {
    if (key == Key::Enter || key == Key::Space) {
        activate();
        return true;
    }
    return false;
}

Checkbox::Checkbox(const char* text, bool checked)
    : text_(text),
      callback_(nullptr),
      context_(nullptr),
      scale_(1),
      checked_(checked),
      pressed_(false) {
    setFocusable(true);
}

void Checkbox::setText(const char* text) {
    text_ = text;
    invalidate();
}

void Checkbox::setChecked(bool checked) {
    if (checked_ == checked) {
        return;
    }
    checked_ = checked;
    invalidate();
}

void Checkbox::toggle() {
    checked_ = !checked_;
    invalidate();
}

void Checkbox::setToggled(Toggled callback, void* context) {
    callback_ = callback;
    context_ = context;
}

void Checkbox::setScale(uint8_t scale) {
    scale_ = scale == 0 ? 1 : scale;
    invalidate();
}

void Checkbox::fit(const Font& font) {
    const int box = 18;
    const int textW = measureTextWidth(font, text_, scale_);
    const int textH = measureTextHeight(font, text_, scale_);
    setSize(static_cast<uint16_t>(box + 8 + textW),
            static_cast<uint16_t>(maxInt(box, textH) + 8));
}

void Checkbox::activate() {
    if (!enabledInTree()) {
        return;
    }
    toggle();
    if (callback_ != nullptr) {
        callback_(context_, checked_);
    }
}

void Checkbox::paint(Canvas& canvas, int x, int y) {
    const Rect r(static_cast<int16_t>(x), static_cast<int16_t>(y), width(),
                 height());
    if (r.empty()) {
        return;
    }
    const bool on = enabledInTree();
    int box = minInt(18, static_cast<int>(height()));
    if (box < 8) {
        box = static_cast<int>(height());
    }
    if (box <= 0) {
        return;
    }
    const int bx = x + 1;
    const int by = y + (static_cast<int>(height()) - box) / 2;
    const Rect boxRect(static_cast<int16_t>(bx), static_cast<int16_t>(by),
                       static_cast<uint16_t>(box),
                       static_cast<uint16_t>(box));

    Color border = on ? canvas.theme().border : canvas.theme().text_muted;
    if (checked_) {
        const Color fill = on ? canvas.theme().primary : canvas.theme().border;
        canvas.fillRoundRect(boxRect, 4, fill);
        const Color mark = on ? canvas.theme().on_primary
                              : canvas.theme().text_muted;
        const int pad = box / 5;
        const Rect old = canvas.clip();
        canvas.setClip(old.intersect(boxRect));
        canvas.drawLine(bx + pad, by + box / 2, bx + box / 2, by + box - pad,
                        mark, 2);
        canvas.drawLine(bx + box / 2, by + box - pad, bx + box - pad,
                        by + pad, mark, 2);
        canvas.setClip(old);
    } else {
        canvas.fillRoundRect(boxRect, 4, border);
        const Rect inner = boxRect.inset(1);
        canvas.fillRoundRect(inner, 3,
                             on ? canvas.theme().surface_alt
                                : canvas.theme().surface);
    }

    if (text_ != nullptr && width() > static_cast<uint16_t>(box + 9)) {
        const int textX = bx + box + 8;
        const Rect textBox(static_cast<int16_t>(textX), r.y,
                           static_cast<uint16_t>(r.right() - textX), r.h);
        drawClippedText(canvas, textBox, text_,
                        on ? canvas.theme().text : canvas.theme().text_muted,
                        fontOf(canvas), scale_, Align::Left);
    }
}

bool Checkbox::onPointerDown([[maybe_unused]] int16_t x,
                             [[maybe_unused]] int16_t y) {
    if (!enabledInTree()) {
        return false;
    }
    pressed_ = true;
    return true;
}

void Checkbox::onPointerUp(int16_t x, int16_t y) {
    if (!pressed_) {
        return;
    }
    pressed_ = false;
    if (!enabledInTree()) {
        return;
    }
    if (x >= 0 && y >= 0 && x < width() && y < height()) {
        activate();
    }
}

bool Checkbox::onKey(Key key) {
    if (key == Key::Enter || key == Key::Space) {
        activate();
        return true;
    }
    return false;
}

ProgressBar::ProgressBar(uint8_t value) : value_(value > 100 ? 100 : value) {}

void ProgressBar::setValue(uint8_t value) {
    value = value > 100 ? 100 : value;
    if (value_ == value) {
        return;
    }
    value_ = value;
    invalidate();
}

void ProgressBar::paint(Canvas& canvas, int x, int y) {
    const Rect r(static_cast<int16_t>(x), static_cast<int16_t>(y), width(),
                 height());
    if (r.empty()) {
        return;
    }
    const int radius = static_cast<int>(height()) / 2;
    const Color track = enabledInTree() ? canvas.theme().surface_alt
                                        : canvas.theme().surface;
    canvas.fillRoundRect(r, static_cast<uint16_t>(radius), track);
    if (value_ == 0) {
        return;
    }
    int fillWidth = static_cast<int>(width()) * value_ / 100;
    if (fillWidth < 1) {
        fillWidth = 1;
    }
    const Rect fill(static_cast<int16_t>(x), static_cast<int16_t>(y),
                    static_cast<uint16_t>(fillWidth), height());
    canvas.fillRoundRect(fill, static_cast<uint16_t>(radius),
                         enabledInTree() ? canvas.theme().primary
                                         : canvas.theme().border);
}

Slider::Slider(uint8_t value)
    : value_(value > 100 ? 100 : value),
      step_(5),
      callback_(nullptr),
      context_(nullptr) {
    setFocusable(true);
}

void Slider::setValue(uint8_t value) {
    if (value > 100) {
        value = 100;
    }
    if (value_ == value) {
        return;
    }
    value_ = value;
    invalidate();
    if (callback_ != nullptr) {
        callback_(context_, value_);
    }
}

void Slider::setStep(uint8_t step) {
    step_ = step == 0 ? 1 : step;
    if (step_ > 100) {
        step_ = 100;
    }
}

void Slider::setChanged(Changed callback, void* context) {
    callback_ = callback;
    context_ = context;
}

void Slider::updateFromX(int16_t x) {
    const int span = width() > 1 ? static_cast<int>(width()) - 1 : 1;
    int value = static_cast<int>(x) * 100 / span;
    if (value < 0) {
        value = 0;
    }
    if (value > 100) {
        value = 100;
    }
    setValue(static_cast<uint8_t>(value));
}

void Slider::paint(Canvas& canvas, int x, int y) {
    const Rect r(static_cast<int16_t>(x), static_cast<int16_t>(y), width(),
                 height());
    if (r.empty()) {
        return;
    }
    const bool on = enabledInTree();

    int trackHeight = static_cast<int>(height()) / 3;
    if (trackHeight < 4) {
        trackHeight = 4;
    }
    if (trackHeight > 8) {
        trackHeight = 8;
    }
    if (trackHeight > static_cast<int>(height())) {
        trackHeight = height();
    }
    const int trackY =
        y + (static_cast<int>(height()) - trackHeight) / 2;
    const Rect track(static_cast<int16_t>(x), static_cast<int16_t>(trackY),
                     width(), static_cast<uint16_t>(trackHeight));
    canvas.fillRoundRect(track, static_cast<uint16_t>(trackHeight / 2),
                         on ? canvas.theme().surface_alt
                            : canvas.theme().surface);

    int fillWidth = static_cast<int>(width()) * value_ / 100;
    if (value_ != 0 && fillWidth < 1) {
        fillWidth = 1;
    }
    if (fillWidth > 0) {
        const Rect fill(static_cast<int16_t>(x), static_cast<int16_t>(trackY),
                        static_cast<uint16_t>(fillWidth),
                        static_cast<uint16_t>(trackHeight));
        canvas.fillRoundRect(fill, static_cast<uint16_t>(trackHeight / 2),
                             on ? canvas.theme().primary
                                : canvas.theme().border);
    }

    int knob = static_cast<int>(height());
    if (knob > 20) {
        knob = 20;
    }
    if (knob < 4) {
        knob = 4;
    }
    if (knob > width()) {
        knob = width();
    }
    if (knob > static_cast<int>(height())) {
        knob = height();
    }
    if (knob <= 0) {
        return;
    }
    const int knobX =
        x + (static_cast<int>(width()) - knob) * value_ / 100;
    const Rect knobRect(
        static_cast<int16_t>(knobX),
        static_cast<int16_t>(y + (static_cast<int>(height()) - knob) / 2),
        static_cast<uint16_t>(knob), static_cast<uint16_t>(knob));
    canvas.fillRoundRect(knobRect, static_cast<uint16_t>(knob / 2),
                         on ? canvas.theme().text
                            : canvas.theme().text_muted);
}

bool Slider::onPointerDown(int16_t x, [[maybe_unused]] int16_t y) {
    updateFromX(x);
    return true;
}

void Slider::onPointerMove(int16_t x, [[maybe_unused]] int16_t y) {
    updateFromX(x);
}

bool Slider::onKey(Key key) {
    if (key == Key::Left || key == Key::Down) {
        setValue(static_cast<uint8_t>(value_ > step_ ? value_ - step_ : 0));
        return true;
    }
    if (key == Key::Right || key == Key::Up) {
        const int value = static_cast<int>(value_) + step_;
        setValue(static_cast<uint8_t>(value > 100 ? 100 : value));
        return true;
    }
    return false;
}

Switch::Switch(bool checked)
    : callback_(nullptr),
      context_(nullptr),
      checked_(checked),
      pressed_(false) {
    setFocusable(true);
}

void Switch::setChecked(bool checked) {
    if (checked_ == checked) {
        return;
    }
    checked_ = checked;
    invalidate();
}

void Switch::setToggled(Toggled callback, void* context) {
    callback_ = callback;
    context_ = context;
}

void Switch::toggle() {
    checked_ = !checked_;
    invalidate();
    if (callback_ != nullptr) {
        callback_(context_, checked_);
    }
}

void Switch::paint(Canvas& canvas, int x, int y) {
    const Rect r(static_cast<int16_t>(x), static_cast<int16_t>(y), width(),
                 height());
    if (r.empty()) {
        return;
    }
    const bool on = enabledInTree();
    const Color track = checked_
                            ? (on ? canvas.theme().primary
                                  : canvas.theme().border)
                            : (on ? canvas.theme().surface_alt
                                  : canvas.theme().surface);
    canvas.fillRoundRect(r, static_cast<uint16_t>(height() / 2), track);

    int knob = static_cast<int>(height()) - 4;
    if (knob < 4) {
        knob = 4;
    }
    if (knob > width()) {
        knob = width();
    }
    if (knob > static_cast<int>(height())) {
        knob = height();
    }
    if (knob <= 0) {
        return;
    }
    int knobX = checked_ ? x + static_cast<int>(width()) - knob - 2 : x + 2;
    if (knobX < x) {
        knobX = x;
    }
    if (knobX + knob > x + static_cast<int>(width())) {
        knobX = x + static_cast<int>(width()) - knob;
    }
    const Rect knobRect(
        static_cast<int16_t>(knobX),
        static_cast<int16_t>(y + (static_cast<int>(height()) - knob) / 2),
        static_cast<uint16_t>(knob), static_cast<uint16_t>(knob));
    canvas.fillRoundRect(knobRect, static_cast<uint16_t>(knob / 2),
                         on ? canvas.theme().text
                            : canvas.theme().text_muted);
}

bool Switch::onPointerDown([[maybe_unused]] int16_t x,
                           [[maybe_unused]] int16_t y) {
    if (!enabledInTree()) {
        return false;
    }
    pressed_ = true;
    return true;
}

void Switch::onPointerUp(int16_t x, int16_t y) {
    if (!pressed_) {
        return;
    }
    pressed_ = false;
    if (!enabledInTree()) {
        return;
    }
    if (x >= 0 && y >= 0 && x < width() && y < height()) {
        toggle();
    }
}

bool Switch::onKey(Key key) {
    if (key == Key::Enter || key == Key::Space) {
        if (enabledInTree()) {
            toggle();
        }
        return true;
    }
    return false;
}

ListBox::ListBox(const char* const* items, uint16_t count, uint16_t rowHeight)
    : items_(items),
      count_(count),
      rowHeight_(rowHeight == 0 ? 1 : rowHeight),
      topRow_(0),
      selected_(kNone),
      callback_(nullptr),
      context_(nullptr),
      pressY_(0),
      pressTopRow_(0),
      pressed_(false),
      dragging_(false) {
    setFocusable(true);
}

void ListBox::setItems(const char* const* items, uint16_t count) {
    items_ = items;
    count_ = count;
    if (selected_ >= count_) {
        selected_ = kNone;
    }
    clampScroll();
    invalidate();
}

void ListBox::setRowHeight(uint16_t rowHeight) {
    rowHeight_ = rowHeight == 0 ? 1 : rowHeight;
    clampScroll();
    invalidate();
}

void ListBox::setSelected(uint16_t index) {
    selected_ = index < count_ ? index : kNone;
    ensureVisible();
    invalidate();
}

void ListBox::setSelectedCallback(Selected callback, void* context) {
    callback_ = callback;
    context_ = context;
}

void ListBox::scrollTo(uint16_t index) {
    topRow_ = index < maxTopRow() ? index : maxTopRow();
    invalidate();
}

void ListBox::scrollBy(int16_t rows) {
    scrollByRows(rows);
}

uint16_t ListBox::visibleRows() const {
    if (rowHeight_ == 0 || height() == 0) {
        return 1;
    }
    const uint16_t rows = static_cast<uint16_t>(height() / rowHeight_);
    return rows == 0 ? 1 : rows;
}

uint16_t ListBox::maxTopRow() const {
    const uint16_t visible = visibleRows();
    return count_ > visible ? static_cast<uint16_t>(count_ - visible) : 0;
}

void ListBox::clampScroll() {
    const uint16_t maxTop = maxTopRow();
    if (topRow_ > maxTop) {
        topRow_ = maxTop;
    }
}

void ListBox::ensureVisible() {
    if (selected_ == kNone) {
        return;
    }
    const uint16_t visible = visibleRows();
    if (selected_ < topRow_) {
        topRow_ = selected_;
    } else if (selected_ >= topRow_ + visible) {
        topRow_ = static_cast<uint16_t>(selected_ - visible + 1);
    }
    clampScroll();
}

void ListBox::select(uint16_t index, bool notify) {
    if (index >= count_) {
        return;
    }
    const bool changed = selected_ != index;
    selected_ = index;
    ensureVisible();
    invalidate();
    if (notify && changed && callback_ != nullptr) {
        callback_(context_, index);
    }
}

void ListBox::scrollByRows(int rows) {
    int top = static_cast<int>(topRow_) + rows;
    const int maxTop = maxTopRow();
    if (top < 0) {
        top = 0;
    }
    if (top > maxTop) {
        top = maxTop;
    }
    if (top != topRow_) {
        topRow_ = static_cast<uint16_t>(top);
        invalidate();
    }
}

void ListBox::paint(Canvas& canvas, int x, int y) {
    const Rect r(static_cast<int16_t>(x), static_cast<int16_t>(y), width(),
                 height());
    if (r.empty()) {
        return;
    }
    const Theme& theme = canvas.theme();
    const bool on = enabledInTree();
    canvas.fillRect(r, theme.surface);
    canvas.drawRect(r, theme.border, 1);

    const int textWidth = static_cast<int>(width()) - 12;
    const uint16_t visible = visibleRows();
    const Font& font = fontOf(canvas);
    for (uint16_t row = 0; row < visible; ++row) {
        const uint16_t index = static_cast<uint16_t>(topRow_ + row);
        if (index >= count_) {
            break;
        }
        const Rect rowRect(
            static_cast<int16_t>(x),
            static_cast<int16_t>(y + static_cast<int>(row) * rowHeight_),
            width(), rowHeight_);
        const bool selected = index == selected_;
        if (selected) {
            canvas.fillRect(rowRect, on ? theme.primary : theme.border);
        }
        const char* item = items_ != nullptr ? items_[index] : nullptr;
        if (item != nullptr && textWidth > 0) {
            const Rect textBox(static_cast<int16_t>(x + 6), rowRect.y,
                               static_cast<uint16_t>(textWidth), rowHeight_);
            const Color textColor =
                selected && on ? theme.on_primary
                               : (on ? theme.text : theme.text_muted);
            drawClippedText(canvas, textBox, item, textColor, font, 1,
                            Align::Left);
        }
    }

    if (count_ > visible && width() > 4) {
        const int scrollX = x + static_cast<int>(width()) - 3;
        const int trackHeight = height();
        int thumbHeight =
            static_cast<int>(trackHeight) * visible / count_;
        if (thumbHeight < 8) {
            thumbHeight = 8;
        }
        const uint16_t maxTop = maxTopRow();
        int thumbY = y;
        if (maxTop > 0) {
            thumbY += (trackHeight - thumbHeight) * topRow_ / maxTop;
        }
        canvas.fillRect(
            Rect(static_cast<int16_t>(scrollX), static_cast<int16_t>(y), 3,
                 static_cast<uint16_t>(trackHeight)),
            theme.surface_alt);
        canvas.fillRect(
            Rect(static_cast<int16_t>(scrollX),
                 static_cast<int16_t>(thumbY), 3,
                 static_cast<uint16_t>(thumbHeight)),
            theme.text_muted);
    }
}

bool ListBox::onPointerDown([[maybe_unused]] int16_t x, int16_t y) {
    if (count_ == 0) {
        return false;
    }
    pressed_ = true;
    dragging_ = false;
    pressY_ = y;
    pressTopRow_ = topRow_;
    return true;
}

void ListBox::onPointerMove([[maybe_unused]] int16_t x, int16_t y) {
    if (!pressed_) {
        return;
    }
    const int dy = static_cast<int>(y) - pressY_;
    if (!dragging_ && (dy > 4 || dy < -4)) {
        dragging_ = true;
    }
    if (!dragging_) {
        return;
    }
    const int rows = dy / static_cast<int>(rowHeight_);
    int top = static_cast<int>(pressTopRow_) - rows;
    const int maxTop = maxTopRow();
    if (top < 0) {
        top = 0;
    }
    if (top > maxTop) {
        top = maxTop;
    }
    if (top != topRow_) {
        topRow_ = static_cast<uint16_t>(top);
        invalidate();
    }
}

void ListBox::onPointerUp([[maybe_unused]] int16_t x, int16_t y) {
    if (!pressed_) {
        return;
    }
    pressed_ = false;
    if (dragging_) {
        dragging_ = false;
        return;
    }
    if (y < 0 || y >= height()) {
        return;
    }
    const int row =
        static_cast<int>(topRow_) + static_cast<int>(y) / rowHeight_;
    if (row >= 0 && row < count_) {
        select(static_cast<uint16_t>(row), true);
    }
}

bool ListBox::onKey(Key key) {
    if (count_ == 0) {
        return false;
    }
    if (key == Key::Up || key == Key::Down) {
        int next = selected_ == kNone
                       ? (key == Key::Down ? 0 : count_ - 1)
                       : static_cast<int>(selected_);
        if (key == Key::Down) {
            if (next + 1 < count_) {
                ++next;
            }
        } else if (next > 0) {
            --next;
        }
        select(static_cast<uint16_t>(next), true);
        return true;
    }
    if (key == Key::Enter) {
        if (selected_ != kNone && callback_ != nullptr) {
            callback_(context_, selected_);
        }
        return true;
    }
    return false;
}

EditBox::EditBox(char* buffer, uint16_t capacity, uint16_t maxLength)
    : buffer_(buffer),
      capacity_(capacity),
      maxLength_(maxLength),
      cursor_(0),
      scroll_(0),
      changed_(nullptr),
      submitted_(nullptr),
      context_(nullptr) {
    if (buffer_ == nullptr || capacity_ < 2) {
        buffer_ = nullptr;
        capacity_ = 0;
        maxLength_ = 0;
        return;
    }
    if (maxLength_ == 0 || maxLength_ > capacity_ - 1) {
        maxLength_ = static_cast<uint16_t>(capacity_ - 1);
    }
    buffer_[0] = '\0';
    setFocusable(true);
}

uint16_t EditBox::length() const {
    if (buffer_ == nullptr) {
        return 0;
    }
    uint16_t length = 0;
    while (length < capacity_ - 1 && buffer_[length] != '\0') {
        ++length;
    }
    return length;
}

void EditBox::setText(const char* text) {
    if (buffer_ == nullptr) {
        return;
    }
    const uint16_t limit = maxLength_ < capacity_ - 1 ? maxLength_
                                                     : static_cast<uint16_t>(capacity_ - 1);
    uint16_t length = 0;
    if (text != nullptr) {
        while (length < limit && text[length] != '\0') {
            buffer_[length] = text[length];
            ++length;
        }
    }
    buffer_[length] = '\0';
    cursor_ = length;
    scroll_ = 0;
    invalidate();
}

void EditBox::setMaxLength(uint16_t maxLength) {
    if (buffer_ == nullptr) {
        return;
    }
    const uint16_t limit = static_cast<uint16_t>(capacity_ - 1);
    if (maxLength == 0 || maxLength > limit) {
        maxLength = limit;
    }
    maxLength_ = maxLength;
    uint16_t length = this->length();
    if (length > maxLength_) {
        buffer_[maxLength_] = '\0';
        length = maxLength_;
    }
    if (cursor_ > length) {
        cursor_ = length;
    }
    invalidate();
}

void EditBox::setCursor(uint16_t position) {
    const uint16_t length = this->length();
    cursor_ = position < length ? position : length;
    invalidate();
}

void EditBox::setChanged(Changed callback, void* context) {
    changed_ = callback;
    context_ = context;
}

void EditBox::setSubmitted(Submitted callback, void* context) {
    submitted_ = callback;
    context_ = context;
}

void EditBox::insert(char c) {
    if (buffer_ == nullptr) {
        return;
    }
    const uint16_t length = this->length();
    if (cursor_ > length) {
        cursor_ = length;
    }
    if (length >= maxLength_ || length + 1 >= capacity_) {
        return;
    }
    for (uint16_t i = length; i > cursor_; --i) {
        buffer_[i] = buffer_[i - 1];
    }
    buffer_[cursor_] = c;
    ++cursor_;
    buffer_[length + 1] = '\0';
    invalidate();
    notifyChanged();
}

void EditBox::eraseBefore() {
    if (buffer_ == nullptr) {
        return;
    }
    const uint16_t length = this->length();
    if (cursor_ > length) {
        cursor_ = length;
    }
    if (cursor_ == 0) {
        return;
    }
    for (uint16_t i = static_cast<uint16_t>(cursor_ - 1); i < length; ++i) {
        buffer_[i] = buffer_[i + 1];
    }
    --cursor_;
    invalidate();
    notifyChanged();
}

void EditBox::eraseAfter() {
    if (buffer_ == nullptr) {
        return;
    }
    const uint16_t length = this->length();
    if (cursor_ > length) {
        cursor_ = length;
    }
    if (cursor_ >= length) {
        return;
    }
    for (uint16_t i = cursor_; i < length; ++i) {
        buffer_[i] = buffer_[i + 1];
    }
    invalidate();
    notifyChanged();
}

void EditBox::notifyChanged() {
    if (changed_ != nullptr && buffer_ != nullptr) {
        changed_(context_, buffer_);
    }
}

void EditBox::paint(Canvas& canvas, int x, int y) {
    const Rect r(static_cast<int16_t>(x), static_cast<int16_t>(y), width(),
                 height());
    if (r.empty()) {
        return;
    }
    const Theme& theme = canvas.theme();
    const bool on = enabledInTree();
    const int radius =
        maxInt(0, minInt(theme.radius, static_cast<int>(height()) / 2));
    canvas.fillRoundRect(r, static_cast<uint16_t>(radius),
                         focused() ? theme.focus : theme.border);
    const Rect inner = r.inset(1);
    canvas.fillRoundRect(inner, static_cast<uint16_t>(maxInt(0, radius - 1)),
                         on ? theme.surface_alt : theme.surface);

    const Rect textArea = r.inset(4);
    if (textArea.empty() || buffer_ == nullptr) {
        return;
    }
    const Font& font = fontOf(canvas);
    const uint16_t textLength = length();
    if (cursor_ > textLength) {
        cursor_ = textLength;
    }
    const int advance = font.advance();
    const int cursorX = static_cast<int>(cursor_) * advance;
    if (cursorX < static_cast<int>(scroll_)) {
        scroll_ = static_cast<uint16_t>(cursorX);
    }
    if (cursorX - static_cast<int>(scroll_) >
        static_cast<int>(textArea.w) - 1) {
        scroll_ = static_cast<uint16_t>(
            cursorX - static_cast<int>(textArea.w) + 1);
    }
    const int textY =
        y + (static_cast<int>(height()) - font.height) / 2;

    const Rect old = canvas.clip();
    canvas.setClip(old.intersect(textArea));
    canvas.drawText(textArea.x - static_cast<int>(scroll_), textY, buffer_,
                    on ? theme.text : theme.text_muted, font, 1);
    if (focused()) {
        const int cursorScreenX =
            textArea.x + cursorX - static_cast<int>(scroll_);
        canvas.fillRect(
            Rect(static_cast<int16_t>(cursorScreenX), textArea.y, 1,
                 textArea.h),
            theme.text);
    }
    canvas.setClip(old);
}

bool EditBox::onPointerDown(int16_t x, [[maybe_unused]] int16_t y) {
    if (buffer_ == nullptr) {
        return false;
    }
    const int advance = fontOf(*this).advance();
    if (advance > 0) {
        int position = (static_cast<int>(x) - 4 + scroll_) / advance;
        const uint16_t length = this->length();
        if (position < 0) {
            position = 0;
        }
        if (position > length) {
            position = length;
        }
        cursor_ = static_cast<uint16_t>(position);
        invalidate();
    }
    return true;
}

bool EditBox::onKey(Key key) {
    if (buffer_ == nullptr) {
        return false;
    }
    if (key == Key::Left) {
        if (cursor_ > 0) {
            --cursor_;
            invalidate();
        }
        return true;
    }
    if (key == Key::Right) {
        if (cursor_ < length()) {
            ++cursor_;
            invalidate();
        }
        return true;
    }
    if (key == Key::Backspace) {
        eraseBefore();
        return true;
    }
    if (key == Key::Delete) {
        eraseAfter();
        return true;
    }
    if (key == Key::Space) {
        insert(' ');
        return true;
    }
    if (key == Key::Enter) {
        if (submitted_ != nullptr) {
            submitted_(context_, buffer_);
        }
        return true;
    }
    return false;
}

bool EditBox::onChar(char c) {
    const unsigned char ch = static_cast<unsigned char>(c);
    if (buffer_ == nullptr || ch < 0x20 || ch > 0x7E) {
        return false;
    }
    insert(static_cast<char>(ch));
    return true;
}

Image::Image(const uint16_t* pixels, uint16_t imageWidth,
             uint16_t imageHeight)
    : pixels_(pixels), imageWidth_(imageWidth), imageHeight_(imageHeight) {}

void Image::setBitmap(const uint16_t* pixels, uint16_t imageWidth,
                      uint16_t imageHeight) {
    pixels_ = pixels;
    imageWidth_ = imageWidth;
    imageHeight_ = imageHeight;
    invalidate();
}

void Image::fit() {
    if (pixels_ != nullptr) {
        setSize(imageWidth_, imageHeight_);
    }
}

void Image::paint(Canvas& canvas, int x, int y) {
    if (pixels_ != nullptr) {
        canvas.drawBitmap(x, y, pixels_, imageWidth_, imageHeight_);
    }
}

}  // namespace pixmote
