#include "widgets/ScrollView.h"

#include <windows.h>

#include <algorithm>

#include "graphics/Canvas.h"
#include "style/Theme.h"

namespace {

// Figma draws no scrollbar: Windows 11's, a 4px line inside the right edge that widens to 8px under
// the pointer, in a 12px strip that takes the mouse.
constexpr float thinWidth = 4;
constexpr float wideWidth = 8;
constexpr float barInset = 2;
constexpr float stripWidth = 12;
constexpr float minimumThumb = 24;
// How far a wheel line moves, and how long a scroll eases.
constexpr float lineHeight = 20;
constexpr std::chrono::milliseconds scrollDuration{150};

} // namespace

float ScrollView::maxOffset() const
{
    return std::max(0.0f, m_contentHeight - frame().height);
}

Rect ScrollView::thumb(float offset) const
{
    const float height = std::max(minimumThumb, frame().height * frame().height / m_contentHeight);
    const float y = maxOffset() > 0 ? offset / maxOffset() * (frame().height - height) : 0;
    const float width = m_grab >= 0 ? wideWidth : thinWidth + (wideWidth - thinWidth) * hoverAmount();
    return {frame().width - barInset - width, y, width, height};
}

void ScrollView::scrollTo(float offset)
{
    m_offset.animateTo(std::clamp(offset, 0.0f, maxOffset()), scrollDuration);
    invalidate();
}

View *ScrollView::hitTest(Point point)
{
    if (isVisible() && maxOffset() > 0 && bounds().contains(point) && point.x >= frame().width - stripWidth)
        return this;
    return View::hitTest(point);
}

void ScrollView::layout()
{
    if (!m_content)
        return;
    m_contentHeight = m_content->preferredSize().height;
    // A resize can leave the content scrolled past its end.
    const float offset = std::min(m_offset.target(), maxOffset());
    m_offset.jumpTo(offset);
    m_content->setFrame({0, -offset, frame().width, m_contentHeight});
}

void ScrollView::paintTree(Canvas &canvas)
{
    if (!isVisible() || !m_content)
        return;
    // Scrolling moves the content; its layout stays as it is.
    const float offset = m_offset.value(*this);
    m_content->moveTo({0, -offset});
    canvas.save();
    canvas.translate(frame().x, frame().y);
    canvas.pushClip(bounds());
    m_content->paintTree(canvas);
    if (maxOffset() > 0) {
        const Palette &palette = canvas.palette();
        const Rect bar = thumb(offset);
        const float emphasis = m_grab >= 0 ? 1 : hoverAmount();
        canvas.fillRoundedRect(bar, bar.width / 2, Color::mix(palette.fgA4, palette.fgA3, emphasis));
    }
    canvas.popClip();
    canvas.restore();
}

bool ScrollView::wheelScrolled(float notches)
{
    if (maxOffset() <= 0)
        return false;
    // Windows' wheel setting: lines per notch, or a page.
    UINT lines = 3;
    SystemParametersInfoW(SPI_GETWHEELSCROLLLINES, 0, &lines, 0);
    const float distance = lines == WHEEL_PAGESCROLL ? frame().height : lines * lineHeight;
    scrollTo(m_offset.target() - notches * distance);
    return true;
}

void ScrollView::mousePressed(Point point)
{
    const Rect current = thumb(m_offset.target());
    if (point.y >= current.y && point.y < current.bottom()) {
        m_grab = point.y - current.y;
        invalidate();
    } else {
        // The track pages towards the pointer.
        scrollTo(m_offset.target() + (point.y < current.y ? -frame().height : frame().height));
    }
}

void ScrollView::mouseDragged(Point point)
{
    if (m_grab < 0)
        return;
    const float travel = frame().height - thumb(0).height;
    m_offset.jumpTo(travel > 0 ? std::clamp((point.y - m_grab) / travel, 0.0f, 1.0f) * maxOffset() : 0);
    invalidate();
}

void ScrollView::mouseReleased(Point, bool)
{
    m_grab = -1;
    invalidate();
}
