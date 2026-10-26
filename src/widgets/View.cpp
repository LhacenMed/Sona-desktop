#include "widgets/View.h"

#include <algorithm>

#include "graphics/Canvas.h"
#include "platform/Window.h"

View &View::add(std::unique_ptr<View> child)
{
    child->m_parent = this;
    m_children.push_back(std::move(child));
    return *m_children.back();
}

void View::remove(View &child)
{
    // The window lets go of the child's views first (hover, press, focus).
    if (Window *host = window())
        host->viewRemoved(child);
    std::erase_if(m_children, [&child](const auto &entry) { return entry.get() == &child; });
    invalidate();
}

void View::setFrame(const Rect &frame)
{
    m_frame = frame;
    layout();
}

void View::moveTo(Point position)
{
    m_frame.x = position.x;
    m_frame.y = position.y;
}

void View::setVisible(bool visible)
{
    if (m_visible == visible)
        return;
    m_visible = visible;
    invalidate();
}

void View::paintTree(Canvas &canvas)
{
    if (!m_visible)
        return;
    canvas.save();
    canvas.translate(m_frame.x, m_frame.y);
    paint(canvas);
    for (const auto &child : m_children)
        child->paintTree(canvas);
    canvas.restore();
}

View *View::hitTest(Point point)
{
    if (!m_visible || !bounds().contains(point))
        return nullptr;
    // Later children paint on top, so they are hit first.
    for (auto child = m_children.rbegin(); child != m_children.rend(); ++child) {
        const Rect &frame = (*child)->m_frame;
        if (View *hit = (*child)->hitTest({point.x - frame.x, point.y - frame.y}))
            return hit;
    }
    return takesMouse() ? this : nullptr;
}

Point View::mapFromWindow(Point point) const
{
    for (const View *view = this; view; view = view->m_parent) {
        point.x -= view->m_frame.x;
        point.y -= view->m_frame.y;
    }
    return point;
}

Rect View::mapToWindow(const Rect &rect) const
{
    const Point origin = mapFromWindow({0, 0});
    return {rect.x - origin.x, rect.y - origin.y, rect.width, rect.height};
}

void View::invalidate() const
{
    if (Window *host = window())
        host->invalidate();
}

Window *View::window() const
{
    const View *view = this;
    while (view->m_parent)
        view = view->m_parent;
    return view->m_window;
}
