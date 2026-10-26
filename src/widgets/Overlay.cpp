#include "widgets/Overlay.h"

#include <algorithm>

#include "graphics/Canvas.h"
#include "graphics/GraphicsDevice.h"
#include "platform/Window.h"
#include "style/Surface.h"
#include "style/Theme.h"
#include "widgets/Animation.h"

namespace {

// Space between a popover or callout and what it belongs to.
constexpr float gap = 8;
// Where on-screen messages sit, below the title bar, and how long they stay.
constexpr float messageTop = 56;
constexpr std::chrono::milliseconds messageDuration{1500};
// Callouts appear at Windows' fast pace and leave a little slower.
constexpr std::chrono::milliseconds fadeIn{83};
constexpr std::chrono::milliseconds fadeOut{167};

// Tooltips are small; values and messages are Figma's "audio value", 37 tall with the text centred
// in 40px each side ("100 %" makes it 121 wide).
constexpr TextStyle tooltipStyle{.size = 12};
constexpr Size tooltipPadding{8, 5};
constexpr TextStyle calloutStyle{.size = 14};
constexpr Size calloutPadding{40, 10};

} // namespace

// A line of text on the callout surface, fading in when shown.
class Callout : public View
{
public:
    Callout(std::wstring text, TextStyle style, Size padding)
        : m_text(std::move(text))
        , m_style(style)
        , m_padding(padding)
    {
        m_style.align = TextAlign::Center;
        m_opacity.animateTo(1, fadeIn);
    }

    void setText(std::wstring text)
    {
        m_text = std::move(text);
        invalidate();
    }

    void fadeOut() { m_opacity.animateTo(0, ::fadeOut); }
    bool hasFaded() const { return m_opacity.target() == 0 && m_opacity.value(*this) == 0; }

    Size preferredSize() const override
    {
        const Size text = window()->graphics().measureText(m_text, m_style);
        return {text.width + 2 * m_padding.width, text.height + 2 * m_padding.height};
    }

    void paintTree(Canvas &canvas) override
    {
        canvas.save();
        canvas.translate(frame().x, frame().y);
        canvas.pushOpacity(m_opacity.value(*this));
        canvas.fillSurface(bounds(), Surfaces::callout);
        canvas.drawText(m_text, bounds(), m_style, canvas.palette().fgA1);
        canvas.popOpacity();
        canvas.restore();
    }

private:
    std::wstring m_text;
    TextStyle m_style;
    Size m_padding;
    Animation m_opacity;
};

Overlay::Overlay()
    : m_tooltipTimer([this] { showTooltip(); })
    , m_messageTimer([this] { dismiss(m_message); })
{
}

Overlay::~Overlay() = default;

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
    dismiss(m_tooltip);
}

void Overlay::showValue(const View &anchor, std::wstring text)
{
    if (m_value)
        m_value->setText(std::move(text));
    else
        m_value = &add<Callout>(std::move(text), calloutStyle, calloutPadding);
    const Rect target = boundsOf(anchor);
    const Size size = m_value->preferredSize();
    m_value->setFrame({target.x + (target.width - size.width) / 2, target.y - gap - size.height, size.width, size.height});
}

void Overlay::hideValue()
{
    dismiss(m_value);
}

void Overlay::showMessage(std::wstring text)
{
    if (m_message)
        m_message->setText(std::move(text));
    else
        m_message = &add<Callout>(std::move(text), calloutStyle, calloutPadding);
    const Size size = m_message->preferredSize();
    m_message->setFrame({(frame().width - size.width) / 2, messageTop, size.width, size.height});
    m_messageTimer.start(messageDuration, false);
}

void Overlay::paintTree(Canvas &canvas)
{
    // Callouts that have faded out go before the frame is drawn.
    std::erase_if(m_fading, [this](Callout *callout) {
        if (!callout->hasFaded())
            return false;
        remove(*callout);
        return true;
    });
    View::paintTree(canvas);
}

void Overlay::dismiss(Callout *&callout)
{
    if (!callout)
        return;
    callout->fadeOut();
    m_fading.push_back(std::exchange(callout, nullptr));
    invalidate();
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
    m_tooltip = &add<Callout>(m_tooltipOwner->tooltip(), tooltipStyle, tooltipPadding);
    const Size size = m_tooltip->preferredSize();
    // Centred under its view, or over it near the window's bottom edge.
    float y = target.bottom() + gap;
    if (y + size.height > frame().height)
        y = target.y - gap - size.height;
    const float x = std::clamp(target.x + (target.width - size.width) / 2, gap, std::max(gap, frame().width - gap - size.width));
    m_tooltip->setFrame({x, y, size.width, size.height});
    // Shown from a timer, with nothing else to repaint the window (paused video, for one).
    m_tooltip->invalidate();
}
