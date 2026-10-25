#pragma once

#include <memory>
#include <utility>
#include <vector>

#include "core/Geometry.h"

class Canvas;
class Window;

// What a point of a window does for the window frame: regular content, or part of the title bar.
// The caption buttons are Windows' own (Window::setNativeCaption), so they need no role.
enum class HitRole { Client, Caption, SystemMenu };

// A rectangle of a window's content that paints itself, lays out its children and handles the
// mouse. Frames are in the parent's coordinates, in DIPs. A parent owns its children.
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
        child->m_parent = this;
        m_children.push_back(std::move(child));
        return added;
    }

    const Rect &frame() const { return m_frame; }
    Rect bounds() const { return {0, 0, m_frame.width, m_frame.height}; }
    // Moves and sizes the view, then lays out its children.
    void setFrame(const Rect &frame);
    bool isVisible() const { return m_visible; }
    void setVisible(bool visible);

    // The size a Row gives this view: the one set with setPreferredSize, unless the view measures itself.
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
    virtual bool showsHandCursor() const { return false; }

    // Mouse input, in this view's coordinates. The pressed view receives every drag until release.
    virtual void mousePressed(Point) {}
    virtual void mouseDragged(Point) {}
    virtual void mouseReleased(Point, bool /*inside*/) {}
    bool isHovered() const { return m_hovered; }
    bool isPressed() const { return m_pressed; }

    Point mapFromWindow(Point point) const;
    // Schedules the window to repaint.
    void invalidate();
    Window *window() const;

protected:
    const std::vector<std::unique_ptr<View>> &children() const { return m_children; }

private:
    friend class Window;

    View *m_parent = nullptr;
    // Set on the root view only.
    Window *m_window = nullptr;
    std::vector<std::unique_ptr<View>> m_children;
    Rect m_frame;
    Size m_preferredSize;
    bool m_visible = true;
    bool m_fillsWidth = false;
    bool m_fillsHeight = false;
    bool m_hovered = false;
    bool m_pressed = false;
};
