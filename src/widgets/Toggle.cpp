#include "widgets/Toggle.h"

#include "graphics/Canvas.h"
#include "style/Theme.h"

namespace {

constexpr TextStyle labelStyle{.size = 12, .align = TextAlign::Center};
// Windows' toggle switch slide.
constexpr std::chrono::milliseconds slideDuration{167};

float between(float from, float to, float amount)
{
    return from + (to - from) * amount;
}

} // namespace

Toggle::Toggle(bool on)
    : m_on(on)
    , m_position(on ? 1.0f : 0.0f)
{
    setPreferredSize({68, 32});
}

void Toggle::setOn(bool on)
{
    if (on == m_on)
        return;
    m_on = on;
    m_position.jumpTo(on ? 1.0f : 0.0f);
    invalidate();
}

void Toggle::paint(Canvas &canvas)
{
    const Palette &palette = canvas.palette();
    const float on = m_position.value(*this);
    canvas.fillRoundedRect(bounds(), 6, palette.bgB2);
    // The label fills one side and the thumb the other: label then thumb when on, the reverse when
    // off. Both slide across, their colors turning as they go.
    const Rect label{between(32, 3, on), 3, 33, 26};
    const Rect thumb{between(3, 39, on), 3, 26, 26};
    canvas.fillRoundedRect(label, 3, Color::mix(palette.bgA4, palette.bgAccentBright, on));
    canvas.fillRoundedRect(thumb, 3, Color::mix(palette.fgA4, palette.accentDim, on));
    canvas.drawText(on >= 0.5f ? L"ON" : L"OFF", label, labelStyle, Color::mix(palette.fgA3, palette.accentMain, on));
    canvas.fillRoundedRect(bounds(), 6, palette.hoverOverlay.faded(hoverAmount()));
}

void Toggle::mouseReleased(Point, bool inside)
{
    if (!inside)
        return;
    m_on = !m_on;
    m_position.animateTo(m_on ? 1.0f : 0.0f, slideDuration);
    invalidate();
    if (m_onChanged)
        m_onChanged(m_on);
}
