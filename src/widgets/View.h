#pragma once

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "core/Geometry.h"
#include "platform/KeyChord.h"
#include "widgets/Animation.h"

class Canvas;
class Window;

// What a point of a window does for the window frame: regular content, or part of the title bar,
// including its minimize, maximize and close buttons (which is what gives the maximize button
// Windows 11's Snap Layouts).
enum class HitRole { Client, Caption, SystemMenu, Minimize, Maximize, Close };

// The pointer's shape over a view.
enum class Cursor { Arrow, Hand, Text };

// A rectangle of a window's content that paints itself, lays out its children and handles the
// mouse and keyboard. Frames are in the parent's coordinates, in DIPs. A parent owns its children.
class View
{
public:
    View() = default;
    virtual ~View() = default;
    View(const View &) = delete;
    View &operator=(const View &) = delete;

    template <typename T, typename... Args>
    T &add(Args &&...args)
    {
        auto child = std::make_unique<T>(std::forward<Args>(args)...);
        T &added = *child;
        add(std::move(child));
        return added;
    }
    View &add(std::unique_ptr<View> child);
    // Destroys a child.
    void remove(View &child);
    View *parent() const { return m_parent; }

    const Rect &frame() const { return m_frame; }
    Rect bounds() const { return {0, 0, m_frame.width, m_frame.height}; }
    // Moves and sizes the view, then lays out its children.
    void setFrame(const Rect &frame);
    // Moves the view without laying it out again (its size is unchanged).
    void moveTo(Point position);
    bool isVisible() const { return m_visible; }
    void setVisible(bool visible);

    // The size a Row or Column gives this view: the one set with setPreferredSize, unless the view
    // measures itself.
    virtual Size preferredSize() const { return m_preferredSize; }
    void setPreferredSize(Size size) { m_preferredSize = size; }
    bool fillsWidth() const { return m_fillsWidth; }
    void setFillsWidth(bool fills) { m_fillsWidth = fills; }
    bool fillsHeight() const { return m_fillsHeight; }
    void setFillsHeight(bool fills) { m_fillsHeight = fills; }

    virtual void layout() {}
    virtual void paint(Canvas &) {}
    // Paints the view, then its children over it.
    virtual void paintTree(Canvas &canvas);

    // The deepest visible view under `point` (in this view's coordinates) that takes the mouse.
    virtual View *hitTest(Point point);
    virtual bool takesMouse() const { return false; }
    virtual HitRole hitRole() const { return HitRole::Client; }
    virtual Cursor cursor() const { return Cursor::Arrow; }
    // Whether a press on it while a popup is open reaches it rather than only closing the popup: it
    // opens, switches and closes popups itself (a panel's button).
    virtual bool actsOnPopups() const { return false; }
    // Shown once the pointer rests on the view.
    const std::wstring &tooltip() const { return m_tooltip; }
    void setTooltip(std::wstring tooltip) { m_tooltip = std::move(tooltip); }

    // Mouse input, in this view's coordinates. The pressed view receives every drag until release.
    virtual void mousePressed(Point) {}
    virtual void mouseDragged(Point) {}
    virtual void mouseReleased(Point, bool /*inside*/) {}
    // The pointer moved over the view while it's hovered and nothing is pressed.
    virtual void mouseMoved(Point) {}
    // A wheel or touchpad scroll over the view, in notches; positive brings content above into
    // view. Returns whether the view scrolled; otherwise its parent is offered the scroll.
    virtual bool wheelScrolled(float /*notches*/) { return false; }
    bool isHovered() const { return m_hovered; }
    virtual void hoverChanged() {}
    // How far the hover tint has faded in, 0–1: hover feedback eases in and out, as Windows' does.
    float hoverAmount() const { return m_hover.value(*this); }
    bool isPressed() const { return m_pressed; }

    // Keyboard input goes to the focused view: one that accepts focus gets it when clicked.
    virtual bool acceptsFocus() const { return false; }
    bool hasFocus() const { return m_focused; }
    virtual void focusChanged() {}
    // Returns whether the view handled the key; unhandled keys go on to the window's shortcuts.
    virtual bool keyPressed(KeyChord) { return false; }
    virtual void charTyped(wchar_t) {}
    // Where typed text goes, in this view's coordinates, so the IME composes there.
    virtual Rect caretBounds() const { return {}; }

    Point mapFromWindow(Point point) const;
    Rect mapToWindow(const Rect &rect) const;
    // Schedules the window to repaint.
    void invalidate() const;
    Window *window() const;

protected:
    const std::vector<std::unique_ptr<View>> &children() const { return m_children; }

private:
    friend class Window;

    View *m_parent = nullptr;
    // Set on a window's root views only.
    Window *m_window = nullptr;
    std::vector<std::unique_ptr<View>> m_children;
    Rect m_frame;
    Size m_preferredSize;
    std::wstring m_tooltip;
    bool m_visible = true;
    bool m_fillsWidth = false;
    bool m_fillsHeight = false;
    bool m_hovered = false;
    Animation m_hover;
    bool m_pressed = false;
    bool m_focused = false;
};
