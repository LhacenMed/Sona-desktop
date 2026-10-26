#include "widgets/Stack.h"

#include <algorithm>

Size Stack::preferredSize() const
{
    const bool horizontal = m_axis == Axis::Horizontal;
    float along = horizontal ? m_padding.left + m_padding.right : m_padding.top + m_padding.bottom;
    float across = 0;
    int visibleCount = 0;
    for (const auto &child : children()) {
        if (!child->isVisible())
            continue;
        const Size size = child->preferredSize();
        along += horizontal ? size.width : size.height;
        across = std::max(across, horizontal ? size.height : size.width);
        ++visibleCount;
    }
    if (visibleCount > 1)
        along += m_spacing * (visibleCount - 1);
    across += horizontal ? m_padding.top + m_padding.bottom : m_padding.left + m_padding.right;

    // A size set with setPreferredSize wins.
    const Size fixed = View::preferredSize();
    const Size measured = horizontal ? Size{along, across} : Size{across, along};
    return {fixed.width > 0 ? fixed.width : measured.width, fixed.height > 0 ? fixed.height : measured.height};
}

void Stack::layout()
{
    const bool horizontal = m_axis == Axis::Horizontal;
    const auto lengthOf = [horizontal](Size size) { return horizontal ? size.width : size.height; };
    const auto thicknessOf = [horizontal](Size size) { return horizontal ? size.height : size.width; };
    const auto fillsAlong = [horizontal](const View &view) { return horizontal ? view.fillsWidth() : view.fillsHeight(); };
    const auto fillsAcross = [horizontal](const View &view) { return horizontal ? view.fillsHeight() : view.fillsWidth(); };

    const float start = horizontal ? m_padding.left : m_padding.top;
    const float crossStart = horizontal ? m_padding.top : m_padding.left;
    const float crossEnd = horizontal ? m_padding.bottom : m_padding.right;
    const float crossLength = thicknessOf(frame().size()) - crossStart - crossEnd;

    float used = start + (horizontal ? m_padding.right : m_padding.bottom);
    int visibleCount = 0;
    int fillCount = 0;
    for (const auto &child : children()) {
        if (!child->isVisible())
            continue;
        ++visibleCount;
        if (fillsAlong(*child))
            ++fillCount;
        else
            used += lengthOf(child->preferredSize());
    }
    if (visibleCount > 1)
        used += m_spacing * (visibleCount - 1);
    const float fillLength = fillCount ? std::max(0.0f, (lengthOf(frame().size()) - used) / fillCount) : 0;

    float position = start;
    for (const auto &child : children()) {
        if (!child->isVisible())
            continue;
        const Size size = child->preferredSize();
        const float length = fillsAlong(*child) ? fillLength : lengthOf(size);
        const float thickness = fillsAcross(*child) ? crossLength : thicknessOf(size);
        float offset = crossStart;
        if (m_alignment == Align::Center)
            offset += (crossLength - thickness) / 2;
        else if (m_alignment == Align::End)
            offset += crossLength - thickness;
        child->setFrame(horizontal ? Rect{position, offset, length, thickness} : Rect{offset, position, thickness, length});
        position += length + m_spacing;
    }
}
