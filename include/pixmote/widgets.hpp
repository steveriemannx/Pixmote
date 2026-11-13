// Pixmote - small set of retained widgets.
#pragma once

#include "pixmote/widget.hpp"

namespace pixmote {

// A container with an optional rounded background and border.
class Panel : public Widget {
public:
    Panel();

    void setBackground(Color color);
    void useThemeBackground();
    void setTransparent();
    void setBorder(Color color, uint8_t width = 1);
    void clearBorder();
    void setRadius(int16_t radius);  // -1 means "use theme radius"

protected:
    void paint(Canvas& canvas, int x, int y) override;

private:
    Color background_;
    Color border_;
    int16_t radius_;
    uint8_t borderWidth_;
    bool hasBackground_;
    bool themedBackground_;
};

class Label : public Widget {
public:
    explicit Label(const char* text = nullptr);

    void setText(const char* text);
    const char* text() const { return text_; }
    void setAlign(Align align);
    Align align() const { return align_; }
    void setScale(uint8_t scale);
    uint8_t scale() const { return scale_; }
    void setColor(Color color);
    void useThemeColor();
    void setEllipsis(bool enabled);   // default: true for single-line text
    bool ellipsis() const { return ellipsis_; }
    void fit(const Font& font = font5x7);

protected:
    void paint(Canvas& canvas, int x, int y) override;

private:
    const char* text_;
    Color color_;
    uint8_t scale_;
    bool hasColor_;
    bool ellipsis_;
    Align align_;
};

enum class ButtonKind : uint8_t {
    Primary,
    Secondary,
};

class Button : public Widget {
public:
    typedef void (*Action)(void* context);

    explicit Button(const char* text = nullptr, Action action = nullptr,
                    void* context = nullptr);

    void setText(const char* text);
    const char* text() const { return text_; }
    void setAction(Action action, void* context = nullptr);
    void setKind(ButtonKind kind);
    void setScale(uint8_t scale);
    void setRadius(int16_t radius);
    void fit(const Font& font = font5x7, uint16_t padX = 12,
             uint16_t padY = 8);
    void activate();
    bool pressed() const { return pressed_; }

protected:
    void paint(Canvas& canvas, int x, int y) override;
    bool onPointerDown(int16_t x, int16_t y) override;
    void onPointerUp(int16_t x, int16_t y) override;
    bool onKey(Key key) override;

private:
    const char* text_;
    Action action_;
    void* context_;
    ButtonKind kind_;
    int16_t radius_;
    uint8_t scale_;
    bool pressed_;
};

class Checkbox : public Widget {
public:
    typedef void (*Toggled)(void* context, bool checked);

    explicit Checkbox(const char* text = nullptr, bool checked = false);

    void setText(const char* text);
    const char* text() const { return text_; }
    void setChecked(bool checked);
    void toggle();
    bool checked() const { return checked_; }
    void setToggled(Toggled callback, void* context = nullptr);
    void setScale(uint8_t scale);
    void fit(const Font& font = font5x7);

protected:
    void paint(Canvas& canvas, int x, int y) override;
    bool onPointerDown(int16_t x, int16_t y) override;
    void onPointerUp(int16_t x, int16_t y) override;
    bool onKey(Key key) override;

private:
    void activate();

    const char* text_;
    Toggled callback_;
    void* context_;
    uint8_t scale_;
    bool checked_;
    bool pressed_;
};

class ProgressBar : public Widget {
public:
    explicit ProgressBar(uint8_t value = 0);

    void setValue(uint8_t value);
    uint8_t value() const { return value_; }

protected:
    void paint(Canvas& canvas, int x, int y) override;

private:
    uint8_t value_;
};

class Slider : public Widget {
public:
    typedef void (*Changed)(void* context, uint8_t value);

    explicit Slider(uint8_t value = 0);

    void setValue(uint8_t value);   // 0..100; calls the changed callback
    uint8_t value() const { return value_; }
    void setStep(uint8_t step);     // keyboard step, default 5
    uint8_t step() const { return step_; }
    void setChanged(Changed callback, void* context = nullptr);

protected:
    void paint(Canvas& canvas, int x, int y) override;
    bool onPointerDown(int16_t x, int16_t y) override;
    void onPointerMove(int16_t x, int16_t y) override;
    bool onKey(Key key) override;

private:
    void updateFromX(int16_t x);
    uint8_t value_;
    uint8_t step_;
    Changed callback_;
    void* context_;
};

class Switch : public Widget {
public:
    typedef void (*Toggled)(void* context, bool checked);

    explicit Switch(bool checked = false);

    void setChecked(bool checked);   // no callback
    bool checked() const { return checked_; }
    void setToggled(Toggled callback, void* context = nullptr);

protected:
    void paint(Canvas& canvas, int x, int y) override;
    bool onPointerDown(int16_t x, int16_t y) override;
    void onPointerUp(int16_t x, int16_t y) override;
    bool onKey(Key key) override;

private:
    void toggle();
    Toggled callback_;
    void* context_;
    bool checked_;
    bool pressed_;
};

class ListBox : public Widget {
public:
    typedef void (*Selected)(void* context, uint16_t index);
    static constexpr uint16_t kNone = 0xFFFFu;

    explicit ListBox(const char* const* items = nullptr, uint16_t count = 0,
                     uint16_t rowHeight = 24);

    void setItems(const char* const* items, uint16_t count);
    void setRowHeight(uint16_t rowHeight);
    uint16_t count() const { return count_; }
    uint16_t rowHeight() const { return rowHeight_; }
    uint16_t selected() const { return selected_; }   // kNone if none
    void setSelected(uint16_t index);                 // kNone clears
    void setSelectedCallback(Selected callback, void* context = nullptr);
    void scrollTo(uint16_t index);
    void scrollBy(int16_t rows);
    uint16_t topRow() const { return topRow_; }

protected:
    void paint(Canvas& canvas, int x, int y) override;
    bool onPointerDown(int16_t x, int16_t y) override;
    void onPointerMove(int16_t x, int16_t y) override;
    void onPointerUp(int16_t x, int16_t y) override;
    bool onKey(Key key) override;

private:
    uint16_t visibleRows() const;
    uint16_t maxTopRow() const;
    void clampScroll();
    void ensureVisible();
    void select(uint16_t index, bool notify);
    void scrollByRows(int rows);

    const char* const* items_;
    uint16_t count_;
    uint16_t rowHeight_;
    uint16_t topRow_;
    uint16_t selected_;
    Selected callback_;
    void* context_;
    int16_t pressY_;
    uint16_t pressTopRow_;
    bool pressed_;
    bool dragging_;
};

class EditBox : public Widget {
public:
    typedef void (*Changed)(void* context, const char* text);
    typedef void (*Submitted)(void* context, const char* text);

    // `buffer` is caller-owned and must live as long as the widget.
    EditBox(char* buffer, uint16_t capacity, uint16_t maxLength = 0);

    const char* text() const { return buffer_ != nullptr ? buffer_ : ""; }
    void setText(const char* text);   // no callback
    void setMaxLength(uint16_t maxLength);
    uint16_t maxLength() const { return maxLength_; }
    void setCursor(uint16_t position);
    uint16_t cursor() const { return cursor_; }
    void setChanged(Changed callback, void* context = nullptr);
    void setSubmitted(Submitted callback, void* context = nullptr);

protected:
    void paint(Canvas& canvas, int x, int y) override;
    bool onPointerDown(int16_t x, int16_t y) override;
    bool onKey(Key key) override;
    bool onChar(char c) override;

private:
    uint16_t length() const;
    void insert(char c);
    void eraseBefore();
    void eraseAfter();
    void notifyChanged();
    char* buffer_;
    uint16_t capacity_;
    uint16_t maxLength_;
    uint16_t cursor_;
    uint16_t scroll_;
    Changed changed_;
    Submitted submitted_;
    void* context_;
};

class Image : public Widget {
public:
    explicit Image(const uint16_t* pixels = nullptr, uint16_t imageWidth = 0,
                   uint16_t imageHeight = 0);

    void setBitmap(const uint16_t* pixels, uint16_t imageWidth,
                   uint16_t imageHeight);
    const uint16_t* bitmap() const { return pixels_; }
    uint16_t imageWidth() const { return imageWidth_; }
    uint16_t imageHeight() const { return imageHeight_; }
    void fit();   // widget size := bitmap size

protected:
    void paint(Canvas& canvas, int x, int y) override;

private:
    const uint16_t* pixels_;
    uint16_t imageWidth_;
    uint16_t imageHeight_;
};

}  // namespace pixmote
