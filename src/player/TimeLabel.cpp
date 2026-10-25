#include "player/TimeLabel.h"

#include <algorithm>
#include <format>

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
    if (m_hoverHighlight && isHovered())
        canvas.fillRoundedRect(bounds(), 6, palette.bgB2);
    const long long total = static_cast<long long>(std::max(m_seconds(), 0.0));
    canvas.drawText(std::format(L"{:02}:{:02}:{:02}", total / 3600, total % 3600 / 60, total % 60), bounds(),
                    {.size = 15, .align = TextAlign::Center}, palette.fgA2);
}
