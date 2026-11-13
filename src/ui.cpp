#include "pixmote/ui.hpp"

namespace pixmote {
namespace {

Rect fullScreen(const Display& display) {
    return Rect(0, 0, static_cast<uint16_t>(display.width),
                static_cast<uint16_t>(display.height));
}

Rect focusRect(const Widget* widget) {
    const Rect r = widget->screenRect();
    return Rect(static_cast<int16_t>(r.x - 2), static_cast<int16_t>(r.y - 2),
                static_cast<uint16_t>(r.w + 4),
                static_cast<uint16_t>(r.h + 4));
}

bool isFocusCandidate(const Widget* widget) {
    return widget->visible() && widget->focusable() && widget->enabledInTree();
}

void scanNext(Widget* widget, Widget* current, bool& passed,
              Widget*& candidate, Widget*& first) {
    if (!widget->visible()) {
        return;
    }
    if (isFocusCandidate(widget)) {
        if (first == nullptr) {
            first = widget;
        }
        if (passed && candidate == nullptr && widget != current) {
            candidate = widget;
        }
        if (widget == current) {
            passed = true;
        }
    }
    for (Widget* child = widget->firstChild(); child != nullptr;
         child = child->nextSibling()) {
        scanNext(child, current, passed, candidate, first);
    }
}

void scanPrevious(Widget* widget, Widget* current, bool& seenCurrent,
                  Widget*& before, Widget*& last) {
    if (!widget->visible()) {
        return;
    }
    if (isFocusCandidate(widget)) {
        if (!seenCurrent) {
            before = widget;
        }
        if (widget == current) {
            seenCurrent = true;
        }
        last = widget;
    }
    for (Widget* child = widget->firstChild(); child != nullptr;
         child = child->nextSibling()) {
        scanPrevious(child, current, seenCurrent, before, last);
    }
}

}  // namespace

Ui::Ui(Display& display, Widget& root, uint16_t* bandBuffer,
       uint32_t bandCapacityPixels, uint16_t bandHeight, const Theme& theme)
    : display_(&display),
      root_(nullptr),
      band_(bandBuffer),
      bandHeight_(bandHeight),
      theme_(&theme),
      dirty_(),
      dirtyCount_(0),
      pointerTarget_(nullptr),
      focused_(nullptr),
      valid_(false) {
    const int width = display.width;
    const int height = display.height;
    if (bandBuffer == nullptr || width <= 0 || height <= 0) {
        return;
    }
    const uint32_t maxBands = bandCapacityPixels / static_cast<uint32_t>(width);
    if (bandHeight_ > maxBands) {
        bandHeight_ = static_cast<uint16_t>(maxBands);
    }
    if (bandHeight_ > static_cast<uint16_t>(height)) {
        bandHeight_ = static_cast<uint16_t>(height);
    }
    if (bandHeight_ == 0) {
        return;
    }
    valid_ = true;
    attachRoot(root);
}

Ui::~Ui() {
    pointerTarget_ = nullptr;
    focused_ = nullptr;
    if (root_ != nullptr) {
        root_->attachUi(nullptr);
        root_ = nullptr;
    }
}

void Ui::setRoot(Widget& root) {
    if (root_ == &root) {
        invalidateAll();
        return;
    }
    if (root_ != nullptr) {
        focused_ = nullptr;
        pointerTarget_ = nullptr;
        root_->attachUi(nullptr);
        root_ = nullptr;
    }
    if (valid_) {
        attachRoot(root);
    }
}

void Ui::attachRoot(Widget& root) {
    root_ = &root;
    root.setPosition(0, 0);
    root.setSize(static_cast<uint16_t>(display_->width),
                 static_cast<uint16_t>(display_->height));
    root.attachUi(this);
    dirtyCount_ = 0;
    invalidateAll();
}

void Ui::setTheme(const Theme& theme) {
    theme_ = &theme;
    invalidateAll();
}

void Ui::invalidate(const Rect& area) {
    if (!valid_ || root_ == nullptr) [[unlikely]] {
        return;
    }
    Rect r = area.intersect(fullScreen(*display_));
    if (r.empty()) [[unlikely]] {
        return;
    }

    for (uint8_t i = 0; i < dirtyCount_;) {
        if (dirty_[i].intersects(r)) {
            r = r.unite(dirty_[i]);
            for (uint8_t j = i; static_cast<uint8_t>(j + 1) < dirtyCount_; ++j) {
                dirty_[j] = dirty_[j + 1];
            }
            --dirtyCount_;
            i = 0;
        } else {
            ++i;
        }
    }

    if (dirtyCount_ < PIXMOTE_MAX_DIRTY_RECTS) {
        dirty_[dirtyCount_++] = r;
    } else {
        for (uint8_t i = 0; i < dirtyCount_; ++i) {
            r = r.unite(dirty_[i]);
        }
        dirty_[0] = r;
        dirtyCount_ = 1;
    }
}

void Ui::invalidateAll() {
    if (!valid_) {
        return;
    }
    dirty_[0] = fullScreen(*display_);
    dirtyCount_ = 1;
}

Rect Ui::dirtyRect(uint8_t index) const {
    if (index >= dirtyCount_) {
        return Rect();
    }
    return dirty_[index];
}

void Ui::render() {
    if (!valid_ || root_ == nullptr || dirtyCount_ == 0) [[likely]] {
        return;
    }

    Rect pending[PIXMOTE_MAX_DIRTY_RECTS];
    const uint8_t count = dirtyCount_;
    for (uint8_t i = 0; i < count; ++i) {
        pending[i] = dirty_[i];
    }
    dirtyCount_ = 0;

    for (uint8_t i = 0; i < count; ++i) {
        renderRegion(pending[i]);
    }
}

void Ui::renderRegion(const Rect& region) {
    const Rect area = region.intersect(fullScreen(*display_));
    if (area.empty()) {
        return;
    }
    const int width = area.w;
    const int height = area.h;
    for (int y = area.y; y < area.y + height; y += bandHeight_) {
        int band = area.y + height - y;
        if (band > bandHeight_) {
            band = bandHeight_;
        }
        Canvas canvas(band_, width, width, band, area.x, y,
                      Rect(area.x, static_cast<int16_t>(y),
                           static_cast<uint16_t>(width),
                           static_cast<uint16_t>(band)),
                      *theme_);
        canvas.fillRect(canvas.clip(), theme_->background);
        root_->draw(canvas, 0, 0);
        if (display_->flush != nullptr) {
            display_->flush(display_->context, area.x, static_cast<int16_t>(y),
                            static_cast<int16_t>(width),
                            static_cast<int16_t>(band), band_,
                            static_cast<uint16_t>(width));
        }
    }
}

Widget* Ui::hitTest(Widget* widget, int x, int y) const {
    if (widget == nullptr || !widget->visible()) {
        return nullptr;
    }
    if (!widget->screenRect().contains(x, y)) {
        return nullptr;
    }
    for (Widget* child = widget->lastChild(); child != nullptr;
         child = child->previousSibling()) {
        Widget* hit = hitTest(child, x, y);
        if (hit != nullptr) {
            return hit;
        }
    }
    return widget;
}

bool Ui::pointerDown(int16_t x, int16_t y) {
    if (!valid_ || root_ == nullptr) {
        return false;
    }
    if (pointerTarget_ != nullptr) {
        pointerCancel();
    }

    Widget* target = hitTest(root_, x, y);
    while (target != nullptr) {
        if (target->enabledInTree()) {
            const Rect r = target->screenRect();
            if (target->onPointerDown(static_cast<int16_t>(x - r.x),
                                      static_cast<int16_t>(y - r.y))) {
                pointerTarget_ = target;
                if (target->focusable()) {
                    focus(target, false);
                }
                return true;
            }
        }
        target = target->parent();
    }
    return false;
}

bool Ui::pointerMove(int16_t x, int16_t y) {
    if (pointerTarget_ == nullptr) {
        return false;
    }
    const Rect r = pointerTarget_->screenRect();
    pointerTarget_->onPointerMove(static_cast<int16_t>(x - r.x),
                                  static_cast<int16_t>(y - r.y));
    return true;
}

bool Ui::pointerUp(int16_t x, int16_t y) {
    if (pointerTarget_ == nullptr) {
        return false;
    }
    Widget* target = pointerTarget_;
    pointerTarget_ = nullptr;
    const Rect r = target->screenRect();
    target->onPointerUp(static_cast<int16_t>(x - r.x),
                        static_cast<int16_t>(y - r.y));
    return true;
}

void Ui::pointerCancel() {
    if (pointerTarget_ == nullptr) {
        return;
    }
    Widget* target = pointerTarget_;
    pointerTarget_ = nullptr;
    target->onPointerUp(-1, -1);
}

bool Ui::keyDown(Key key) {
    if (!valid_ || root_ == nullptr || key == Key::None) {
        return false;
    }
    if (key == Key::Tab) {
        focusNext();
        return true;
    }
    if (focused_ == nullptr || !focused_->enabledInTree() ||
        !focused_->visible()) {
        focusNext();
    }
    if (focused_ == nullptr) {
        return false;
    }
    return focused_->onKey(key);
}

bool Ui::inputChar(char c) {
    const unsigned char ch = static_cast<unsigned char>(c);
    if (!valid_ || root_ == nullptr || ch < 0x20 || ch > 0x7E) {
        return false;
    }
    if (focused_ == nullptr || !focused_->enabledInTree() ||
        !focused_->visible()) {
        focusNext();
    }
    if (focused_ == nullptr) {
        return false;
    }
    return focused_->onChar(c);
}

void Ui::setFocusFlags(Widget* widget, bool showRing) {
    if (widget == nullptr) {
        return;
    }
    if (showRing) {
        widget->flags_ =
            static_cast<uint8_t>(widget->flags_ | Widget::kFocused |
                                 Widget::kShowFocus);
    } else {
        widget->flags_ = static_cast<uint8_t>(
            (widget->flags_ | Widget::kFocused) & ~Widget::kShowFocus);
    }
}

void Ui::focus(Widget* widget, bool showRing) {
    if (widget == focused_) {
        if (widget != nullptr) {
            if (showRing) {
                widget->flags_ = static_cast<uint8_t>(widget->flags_ |
                                                      Widget::kShowFocus);
            } else {
                widget->flags_ = static_cast<uint8_t>(widget->flags_ &
                                                      ~Widget::kShowFocus);
            }
            invalidate(focusRect(widget));
        }
        return;
    }
    if (widget != nullptr &&
        (!widget->focusable() || !widget->enabledInTree() ||
         !widget->visible() || widget->ui_ != this)) {
        return;
    }
    if (focused_ != nullptr) {
        const Rect old = focusRect(focused_);
        focused_->flags_ = static_cast<uint8_t>(
            focused_->flags_ & ~(Widget::kFocused | Widget::kShowFocus));
        invalidate(old);
    }
    focused_ = widget;
    if (focused_ != nullptr) {
        setFocusFlags(focused_, showRing);
        invalidate(focusRect(focused_));
    }
}

void Ui::clearFocus() {
    if (focused_ == nullptr) {
        return;
    }
    const Rect old = focusRect(focused_);
    focused_->flags_ = static_cast<uint8_t>(
        focused_->flags_ & ~(Widget::kFocused | Widget::kShowFocus));
    focused_ = nullptr;
    invalidate(old);
}

void Ui::focusNext() {
    if (root_ == nullptr) {
        return;
    }
    bool passed = (focused_ == nullptr);
    Widget* candidate = nullptr;
    Widget* first = nullptr;
    scanNext(root_, focused_, passed, candidate, first);
    Widget* target = candidate != nullptr ? candidate : first;
    if (target != nullptr) {
        focus(target, true);
    }
}

void Ui::focusPrevious() {
    if (root_ == nullptr) {
        return;
    }
    bool seenCurrent = false;
    Widget* before = nullptr;
    Widget* last = nullptr;
    scanPrevious(root_, focused_, seenCurrent, before, last);
    Widget* target = (seenCurrent && before != nullptr) ? before : last;
    if (target != nullptr) {
        focus(target, true);
    }
}

void Ui::widgetDetached(const Widget* widget) {
    if (widget == nullptr) {
        return;
    }
    const Rect r = widget->screenRect();
    const Rect ring(static_cast<int16_t>(r.x - 2),
                    static_cast<int16_t>(r.y - 2),
                    static_cast<uint16_t>(r.w + 4),
                    static_cast<uint16_t>(r.h + 4));
    if (pointerTarget_ == widget) {
        pointerTarget_ = nullptr;
    }
    if (focused_ == widget) {
        focused_ = nullptr;
    }
    invalidate(ring);
}

}  // namespace pixmote
