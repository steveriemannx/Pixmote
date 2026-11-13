// Pixmote - retained widget tree node.
#pragma once

#include "pixmote/canvas.hpp"
#include "pixmote/types.hpp"

namespace pixmote {

class Ui;

enum class Key : uint8_t {
    None,
    Enter,
    Space,
    Left,
    Right,
    Up,
    Down,
    Tab,
    Escape,
    Backspace,
    Delete,
};

// Base class for every retained object. Widgets own no heap memory and are
// normally constructed as globals or members of the application object.
class Widget {
public:
    Widget();
    virtual ~Widget();

    Widget(const Widget&) = delete;
    Widget& operator=(const Widget&) = delete;

    void setPosition(int16_t x, int16_t y);
    void setSize(uint16_t w, uint16_t h);
    void setRect(const Rect& rect);
    void setRect(int16_t x, int16_t y, uint16_t w, uint16_t h);
    Rect rect() const { return Rect(x_, y_, w_, h_); }
    uint16_t width() const { return w_; }
    uint16_t height() const { return h_; }
    Rect screenRect() const;
    int screenX() const;
    int screenY() const;

    void setVisible(bool visible);
    bool visible() const;
    void setEnabled(bool enabled);
    bool enabled() const;
    bool enabledInTree() const;
    void setFocusable(bool focusable);
    bool focusable() const;
    bool focused() const;
    void requestFocus();

    void setClipChildren(bool clip);
    bool clipChildren() const;

    void addChild(Widget& child);
    void removeChild(Widget& child);
    void removeFromParent();
    void bringToFront();

    Widget* parent() const { return parent_; }
    Widget* firstChild() const { return first_; }
    Widget* lastChild() const { return last_; }
    Widget* nextSibling() const { return next_; }
    Widget* previousSibling() const { return prev_; }

    Ui* ui() const { return ui_; }
    void invalidate();

    // Public to allow custom widgets to call base behavior; normally called
    // by Ui.
    virtual bool onPointerDown(int16_t x, int16_t y);
    virtual void onPointerUp(int16_t x, int16_t y);
    virtual void onPointerMove(int16_t x, int16_t y);
    virtual bool onKey(Key key);
    // Printable ASCII (0x20..0x7E); used by editable widgets.
    virtual bool onChar(char c);
    // Internal rendering entry point used by Ui.
    void draw(Canvas& canvas, int originX, int originY);

protected:
    virtual void paint(Canvas& canvas, int x, int y);

private:
    friend class Ui;

    enum Flags : uint8_t {
        kVisible = 1u << 0,
        kEnabled = 1u << 1,
        kFocusable = 1u << 2,
        kFocused = 1u << 3,
        kClipChildren = 1u << 4,
        kShowFocus = 1u << 5,
    };

    void attachUi(Ui* ui);
    bool isAncestorOf(const Widget* other) const;

    Widget* parent_;
    Widget* first_;
    Widget* last_;
    Widget* next_;
    Widget* prev_;
    Ui* ui_;
    int16_t x_;
    int16_t y_;
    uint16_t w_;
    uint16_t h_;
    uint8_t flags_;
};

}  // namespace pixmote
