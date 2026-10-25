#include "player/PlayButton.h"

#include "graphics/Canvas.h"
#include "playback/Player.h"
#include "style/Theme.h"
#include "widgets/Icon.h"

PlayButton::PlayButton(Player &player)
    : m_player(player)
{
    setPreferredSize({54, 28});
}

void PlayButton::paint(Canvas &canvas)
{
    const Palette &palette = canvas.palette();
    const bool playing = m_player.isPlaying();
    canvas.fillRoundedRect(bounds(), 6, playing ? palette.fgA3 : palette.accentMain);
    // The play glyph sits 1px right of centre, as in the design.
    drawIcon(canvas, playing ? L"pause-dark" : L"play-dark", {playing ? 15.0f : 16.0f, 2}, palette.fgB1);
}

void PlayButton::mouseReleased(Point, bool inside)
{
    if (inside)
        m_player.togglePause();
}
