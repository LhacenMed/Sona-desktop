#include "player/ControlBar.h"

#include "graphics/Canvas.h"
#include "playback/Player.h"
#include "player/PlayButton.h"
#include "player/SeekBar.h"
#include "player/TimeLabel.h"
#include "player/VolumeBar.h"
#include "style/Theme.h"
#include "widgets/IconButton.h"

ControlBar::ControlBar(Player &player)
    : Row(20)
    , m_player(player)
{
    setPadding(20, 20);

    auto &transport = add<Row>(8.0f);
    transport.add<IconButton>(L"previous", 3.0f, 2.0f);
    transport.add<PlayButton>(player);
    transport.add<IconButton>(L"next", 3.0f, 2.0f);

    auto &seek = add<Row>(8.0f);
    seek.setFillsWidth(true);
    seek.add<TimeLabel>([&player] { return player.position(); }, true);
    seek.add<SeekBar>(player).setFillsWidth(true);
    seek.add<TimeLabel>([&player] { return player.duration(); }, false);

    auto &volume = add<Row>(8.0f);
    m_volumeButton = &volume.add<IconButton>(volumeIcon());
    m_volumeButton->setOnClick([&player] { player.toggleMute(); });
    volume.add<VolumeBar>(player);

    auto &panels = add<Row>(12.0f);
    panels.add<IconWrapper>(L"audio");
    panels.add<IconWrapper>(L"subtitles");
    panels.add<IconWrapper>(L"playback-settings");

    add<IconWrapper>(L"lists");

    m_playerSubscription = player.changed.connect([this] {
        m_volumeButton->setIconName(volumeIcon());
        invalidate();
    });
}

void ControlBar::paint(Canvas &canvas)
{
    canvas.fillRect(bounds(), canvas.palette().bgA3);
}

const wchar_t *ControlBar::volumeIcon() const
{
    if (m_player.isMuted() || m_player.volume() == 0)
        return L"volume-mute";
    return m_player.volume() < 50 ? L"volume-medium" : L"volume-high";
}
