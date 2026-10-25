#include "player/SeekBar.h"

#include <algorithm>

#include "graphics/Canvas.h"
#include "playback/Player.h"
#include "style/Theme.h"

namespace {

constexpr float inset = 2;
constexpr float handleWidth = 8;

} // namespace

SeekBar::SeekBar(Player &player)
    : m_player(player)
{
    setPreferredSize({0, 28});
}

bool SeekBar::takesMouse() const
{
    return m_player.duration() > 0;
}

float SeekBar::progress() const
{
    if (isPressed())
        return m_pressedProgress;
    return m_player.duration() > 0 ? static_cast<float>(m_player.position() / m_player.duration()) : 0;
}

void SeekBar::paint(Canvas &canvas)
{
    const Palette &palette = canvas.palette();
    const float handleX = inset + progress() * (frame().width - 2 * inset - handleWidth);
    const float innerHeight = frame().height - 2 * inset;
    canvas.fillRoundedRect(bounds(), 6, palette.bgB2);
    canvas.fillRoundedRect({inset, inset, handleX + handleWidth - inset, innerHeight}, 4, palette.bgAccentBright);
    canvas.fillRoundedRect({handleX, inset, handleWidth, innerHeight}, 4, palette.accentMain);
}

void SeekBar::mousePressed(Point point)
{
    seekTo(point.x);
}

void SeekBar::mouseDragged(Point point)
{
    seekTo(point.x);
}

void SeekBar::seekTo(float x)
{
    const float travel = frame().width - 2 * inset - handleWidth;
    m_pressedProgress = std::clamp((x - inset - handleWidth / 2) / travel, 0.0f, 1.0f);
    m_player.seek(m_pressedProgress * m_player.duration());
}
