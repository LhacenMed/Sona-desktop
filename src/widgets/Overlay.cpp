#include "widgets/Overlay.h"

#include <algorithm>
#include <string>

#include "graphics/Canvas.h"
#include "graphics/GraphicsDevice.h"
#include "platform/Window.h"
#include "style/Theme.h"

namespace {

// Space between a popover or tooltip and what it belongs to.
constexpr float gap = 8;

// A short hint under a view. Figma draws none; it takes the dropdown list's surface, smaller.
class Tooltip : public View
{
public:
    explicit Tooltip(std::wstring text) : m_text(std::move(text)) {}

    Size preferredSize() const override
    {
        const Size text = window()->graphics().measureText(m_text, style);
        return {text.width + 2 * horizontalPadding, text.height + 2 * verticalPadding};
    }

    void paint(Canvas &canvas) override
    {
        const Palette &palette = canvas.palette();
        canvas.drawShadow(bounds(), 4, {0, 2}, 8, Color::hex(0x000000, 0.35f));
        canvas.fillRoundedRect(bounds(), 4, palette.bgA4);
        canvas.drawText(m_text, {horizontalPadding, 0, frame().width - 2 * horizontalPadding, frame().height}, style,
                        palette.fgA1);
    }

private:
    static constexpr TextStyle style{.size = 12};
    static constexpr float horizontalPadding = 8;
    static constexpr float verticalPadding = 5;

    std::wstring m_text;
};

} // namespace

Overlay::Overlay()
    : m_tooltipTimer([this] { showTooltip(); })
{
}

void Overlay::closePopovers()
{
    for (View *popover : std::exchange(m_popovers, {}))
        remove(*popover);
}

bool Overlay::closePopoversOutside(const View *target)
{
    if (m_popovers.empty())
        return false;
    for (const View *view = target; view; view = view->parent()) {
        if (std::ranges::find(m_popovers, view) != m_popovers.end())
            return false;
    }
    closePopovers();
    return true;
}

void Overlay::hoverChanged(View *hovered)
{
    hideTooltip();
    if (!hovered || hovered->tooltip().empty())
        return;
    m_tooltipOwner = hovered;
    // Windows' own tooltip delay.
    m_tooltipTimer.start(std::chrono::milliseconds(GetDoubleClickTime()), false);
}

void Overlay::hideTooltip()
{
    m_tooltipTimer.stop();
    m_tooltipOwner = nullptr;
    if (View *tooltip = std::exchange(m_tooltip, nullptr))
        remove(*tooltip);
}

Rect Overlay::boundsOf(const View &view) const
{
    const Rect inWindow = view.mapToWindow(view.bounds());
    const Point origin = mapFromWindow({inWindow.x, inWindow.y});
    return {origin.x, origin.y, inWindow.width, inWindow.height};
}

void Overlay::place(View &popover, const View &anchor)
{
    const Rect target = boundsOf(anchor);
    const Size size = popover.preferredSize();
    float y = target.bottom() + gap;
    if (y + size.height > frame().height && target.y - gap - size.height >= 0)
        y = target.y - gap - size.height;
    const float x = std::clamp(target.x, 0.0f, std::max(0.0f, frame().width - size.width));
    popover.setFrame({x, y, size.width, size.height});
}

void Overlay::showTooltip()
{
    if (!m_tooltipOwner)
        return;
    const Rect target = boundsOf(*m_tooltipOwner);
    m_tooltip = &add<Tooltip>(m_tooltipOwner->tooltip());
    const Size size = m_tooltip->preferredSize();
    // Centred under its view, or over it near the window's bottom edge.
    float y = target.bottom() + gap;
    if (y + size.height > frame().height)
        y = target.y - gap - size.height;
    const float x = std::clamp(target.x + (target.width - size.width) / 2, gap, std::max(gap, frame().width - gap - size.width));
    m_tooltip->setFrame({x, y, size.width, size.height});
    invalidate();
}
