#include "widgets/Row.h"

#include <algorithm>

Size Row::preferredSize() const
{
    Size content{m_leftPadding + m_rightPadding, 0};
    int visibleCount = 0;
    for (const auto &child : children()) {
        if (!child->isVisible())
            continue;
        const Size size = child->preferredSize();
        content.width += size.width;
        content.height = std::max(content.height, size.height);
        ++visibleCount;
    }
    if (visibleCount > 1)
        content.width += m_spacing * (visibleCount - 1);

    // A size set with setPreferredSize wins.
    const Size fixed = View::preferredSize();
    return {fixed.width > 0 ? fixed.width : content.width, fixed.height > 0 ? fixed.height : content.height};
}

void Row::layout()
{
    float usedWidth = m_leftPadding + m_rightPadding;
    int visibleCount = 0;
    int fillCount = 0;
    for (const auto &child : children()) {
        if (!child->isVisible())
            continue;
        ++visibleCount;
        if (child->fillsWidth())
            ++fillCount;
        else
            usedWidth += child->preferredSize().width;
    }
    if (visibleCount > 1)
        usedWidth += m_spacing * (visibleCount - 1);
    const float fillWidth = fillCount ? std::max(0.0f, (frame().width - usedWidth) / fillCount) : 0;

    float x = m_leftPadding;
    for (const auto &child : children()) {
        if (!child->isVisible())
            continue;
        const Size size = child->preferredSize();
        const float width = child->fillsWidth() ? fillWidth : size.width;
        const float height = child->fillsHeight() ? frame().height : size.height;
        child->setFrame({x, (frame().height - height) / 2, width, height});
        x += width + m_spacing;
    }
}
