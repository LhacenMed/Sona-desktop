#include "player/VideoView.h"

#include "graphics/Canvas.h"
#include "playback/Player.h"
#include "style/Theme.h"

VideoView::VideoView(Player &player)
    : m_player(player)
    , m_playerSubscription(player.changed.connect([this] { invalidate(); }))
{
}

void VideoView::paint(Canvas &canvas)
{
    const Palette &palette = canvas.palette();
    if (m_player.hasVideo())
        canvas.clear(bounds(), Color{});
    else
        canvas.fillRect(bounds(), palette.bgB2);
    const Rect area = m_visibleArea.width > 0 ? m_visibleArea : bounds();
    canvas.strokeRoundedRect(area.inflated(10), 19, 20, palette.bgA3);
}
