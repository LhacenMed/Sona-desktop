#include "player/TimeLabel.h"

#include "core/TimeText.h"
#include "graphics/Canvas.h"
#include "style/Theme.h"

TimeLabel::TimeLabel(std::function<double()> seconds, bool hoverHighlight)
    : m_seconds(std::move(seconds))
    , m_hoverHighlight(hoverHighlight)
{
    setPreferredSize({76, 28});
}

void TimeLabel::paint(Canvas &canvas)
{
    const Palette &palette = canvas.palette();
    if (m_hoverHighlight)
        canvas.fillRoundedRect(bounds(), 6, palette.bgB2.faded(hoverAmount()));
    canvas.drawText(clockText(m_seconds()), bounds(), {.size = 15, .align = TextAlign::Center}, palette.fgA2);
}

void TimeLabel::mouseReleased(Point, bool inside)
{
    if (inside && m_onClick)
        m_onClick();
}
