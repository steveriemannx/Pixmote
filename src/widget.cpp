#include "pixmote/widget.hpp"

#include "pixmote/ui.hpp"

namespace pixmote {

Widget::Widget()
    : parent_(nullptr),
      first_(nullptr),
      last_(nullptr),
      next_(nullptr),
      prev_(nullptr),
      ui_(nullptr),
      x_(0),
      y_(0),
      w_(0),
      h_(0),
      flags_(kVisible | kEnabled) {}

Widget::~Widget() {
    if (ui_ != nullptr) {
        ui_->widgetDetached(this);
    }
    ui_ = nullptr;
    while (first_ != nullptr) {
        removeChild(*first_);
    }
    if (parent_ != nullptr) {
        parent_->removeChild(*this);
    }
}

void Widget::setPosition(int16_t x, int16_t y) {
    if (x_ == x && y_ == y) {
        return;
    }
    const Rect old = screenRect();
    if (ui_ != nullptr) {
        ui_->invalidate(old);
    }
    x_ = x;
    y_ = y;
    invalidate();
}

void Widget::setSize(uint16_t w, uint16_t h) {
    if (w_ == w && h_ == h) {
        return;
    }
    const Rect old = screenRect();
    if (ui_ != nullptr) {
        ui_->invalidate(old);
    }
    w_ = w;
    h_ = h;
    invalidate();
}

void Widget::setRect(const Rect& rect) {
    setRect(rect.x, rect.y, rect.w, rect.h);
}

void Widget::setRect(int16_t x, int16_t y, uint16_t w, uint16_t h) {
    const Rect old = screenRect();
    if (ui_ != nullptr) {
        ui_->invalidate(old);
    }
    x_ = x;
    y_ = y;
    w_ = w;
    h_ = h;
    invalidate();
}

Rect Widget::screenRect() const {
    int x = x_;
    int y = y_;
    for (const Widget* p = parent_; p != nullptr; p = p->parent_) {
        x += p->x_;
        y += p->y_;
    }
    return Rect(static_cast<int16_t>(x), static_cast<int16_t>(y), w_, h_);
}

int Widget::screenX() const {
    int x = x_;
    for (const Widget* p = parent_; p != nullptr; p = p->parent_) {
        x += p->x_;
    }
    return x;
}

int Widget::screenY() const {
    int y = y_;
    for (const Widget* p = parent_; p != nullptr; p = p->parent_) {
        y += p->y_;
    }
    return y;
}

void Widget::setVisible(bool visible) {
    if (visible == ((flags_ & kVisible) != 0)) {
        return;
    }
    if (!visible) {
        invalidate();
        if ((flags_ & kFocused) != 0 && ui_ != nullptr) {
            ui_->clearFocus();
        }
    }
    if (visible) {
        flags_ = static_cast<uint8_t>(flags_ | kVisible);
        invalidate();
    } else {
        flags_ = static_cast<uint8_t>(flags_ & ~kVisible);
    }
}

bool Widget::visible() const {
    return (flags_ & kVisible) != 0;
}

void Widget::setEnabled(bool enabled) {
    if (enabled == ((flags_ & kEnabled) != 0)) {
        return;
    }
    if (!enabled && (flags_ & kFocused) != 0 && ui_ != nullptr) {
        ui_->clearFocus();
    }
    if (enabled) {
        flags_ = static_cast<uint8_t>(flags_ | kEnabled);
    } else {
        flags_ = static_cast<uint8_t>(flags_ & ~kEnabled);
    }
    invalidate();
}

bool Widget::enabled() const {
    return (flags_ & kEnabled) != 0;
}

bool Widget::enabledInTree() const {
    for (const Widget* p = this; p != nullptr; p = p->parent_) {
        if ((p->flags_ & kEnabled) == 0) {
            return false;
        }
    }
    return true;
}

void Widget::setFocusable(bool focusable) {
    if (focusable == ((flags_ & kFocusable) != 0)) {
        return;
    }
    if (!focusable && (flags_ & kFocused) != 0 && ui_ != nullptr) {
        ui_->clearFocus();
    }
    if (focusable) {
        flags_ = static_cast<uint8_t>(flags_ | kFocusable);
    } else {
        flags_ = static_cast<uint8_t>(flags_ & ~kFocusable);
    }
}

bool Widget::focusable() const {
    return (flags_ & kFocusable) != 0;
}

bool Widget::focused() const {
    return (flags_ & kFocused) != 0;
}

void Widget::requestFocus() {
    if (ui_ != nullptr && focusable() && enabledInTree() && visible()) {
        ui_->focus(this, true);
    }
}

void Widget::setClipChildren(bool clip) {
    if (clip == ((flags_ & kClipChildren) != 0)) {
        return;
    }
    if (clip) {
        flags_ = static_cast<uint8_t>(flags_ | kClipChildren);
    } else {
        flags_ = static_cast<uint8_t>(flags_ & ~kClipChildren);
    }
    invalidate();
}

bool Widget::clipChildren() const {
    return (flags_ & kClipChildren) != 0;
}

void Widget::addChild(Widget& child) {
    if (&child == this || child.isAncestorOf(this)) {
        return;
    }
    if (child.parent_ == this) {
        return;
    }
    if (child.parent_ != nullptr) {
        child.parent_->removeChild(child);
    }
    child.parent_ = this;
    child.prev_ = last_;
    child.next_ = nullptr;
    if (last_ != nullptr) {
        last_->next_ = &child;
    } else {
        first_ = &child;
    }
    last_ = &child;
    if (ui_ != nullptr) {
        child.attachUi(ui_);
    }
    child.invalidate();
}

void Widget::removeChild(Widget& child) {
    if (child.parent_ != this) {
        return;
    }
    child.invalidate();
    if (child.prev_ != nullptr) {
        child.prev_->next_ = child.next_;
    } else {
        first_ = child.next_;
    }
    if (child.next_ != nullptr) {
        child.next_->prev_ = child.prev_;
    } else {
        last_ = child.prev_;
    }
    child.parent_ = nullptr;
    child.next_ = nullptr;
    child.prev_ = nullptr;
    child.attachUi(nullptr);
}

void Widget::removeFromParent() {
    if (parent_ != nullptr) {
        parent_->removeChild(*this);
    }
}

void Widget::bringToFront() {
    if (parent_ == nullptr || parent_->last_ == this) {
        return;
    }
    invalidate();
    if (prev_ != nullptr) {
        prev_->next_ = next_;
    } else {
        parent_->first_ = next_;
    }
    if (next_ != nullptr) {
        next_->prev_ = prev_;
    }
    prev_ = parent_->last_;
    next_ = nullptr;
    if (parent_->last_ != nullptr) {
        parent_->last_->next_ = this;
    }
    parent_->last_ = this;
    invalidate();
}

void Widget::invalidate() {
    if (ui_ == nullptr || !visible()) [[unlikely]] {
        return;
    }
    ui_->invalidate(screenRect());
}

bool Widget::onPointerDown([[maybe_unused]] int16_t x,
                           [[maybe_unused]] int16_t y) {
    return false;
}

void Widget::onPointerUp([[maybe_unused]] int16_t x,
                         [[maybe_unused]] int16_t y) {}

void Widget::onPointerMove([[maybe_unused]] int16_t x,
                           [[maybe_unused]] int16_t y) {}

bool Widget::onKey([[maybe_unused]] Key key) {
    return false;
}

bool Widget::onChar([[maybe_unused]] char c) {
    return false;
}

void Widget::attachUi(Ui* ui) {
    ui_ = ui;
    if (ui == nullptr) {
        flags_ = static_cast<uint8_t>(flags_ & ~(kFocused | kShowFocus));
    }
    for (Widget* child = first_; child != nullptr; child = child->next_) {
        child->attachUi(ui);
    }
}

bool Widget::isAncestorOf(const Widget* other) const {
    for (const Widget* p = this; p != nullptr; p = p->parent_) {
        if (p == other) {
            return true;
        }
    }
    return false;
}

void Widget::draw(Canvas& canvas, int originX, int originY) {
    if (!visible() || w_ == 0 || h_ == 0) [[unlikely]] {
        return;
    }
    const int ax = originX + x_;
    const int ay = originY + y_;
    const Rect self(static_cast<int16_t>(ax), static_cast<int16_t>(ay), w_, h_);
    const bool clipChildren = (flags_ & kClipChildren) != 0;

    if (clipChildren && !self.intersects(canvas.clip())) {
        return;
    }

    const Rect oldClip = canvas.clip();
    if (clipChildren) {
        canvas.setClip(oldClip.intersect(self));
    }

    paint(canvas, ax, ay);
    for (Widget* child = first_; child != nullptr; child = child->next_) {
        child->draw(canvas, ax, ay);
    }

    if (clipChildren) {
        canvas.setClip(oldClip);
    }

    if ((flags_ & kFocused) != 0 && (flags_ & kShowFocus) != 0 &&
        ui_ != nullptr) {
        canvas.drawRect(
            Rect(static_cast<int16_t>(ax - 2), static_cast<int16_t>(ay - 2),
                 static_cast<uint16_t>(w_ + 4), static_cast<uint16_t>(h_ + 4)),
            canvas.theme().focus, 1);
    }
}

void Widget::paint([[maybe_unused]] Canvas& canvas, [[maybe_unused]] int x,
                   [[maybe_unused]] int y) {}

}  // namespace pixmote
