#include "widgets/View.h"

#include "graphics/Canvas.h"
#include "platform/Window.h"

void View::setFrame(const Rect &frame)
{
    m_frame = frame;
    layout();
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

void View::invalidate()
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
