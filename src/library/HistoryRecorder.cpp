#include "library/HistoryRecorder.h"

#include <cmath>

#include "playback/Player.h"

namespace {

// How often the position is saved while playing, in seconds.
constexpr double saveInterval = 5;

} // namespace

HistoryRecorder::HistoryRecorder(Player &player, History &history)
    : m_player(player)
    , m_history(history)
{
    m_changedSubscription = player.changed.connect([this] { playerChanged(); });
    m_endedSubscription = player.ended.connect([this] {
        if (m_playing && m_playing->duration > 0) {
            m_playing->position = m_playing->duration;
            m_history.save(*m_playing);
        }
    });
}

void HistoryRecorder::playerChanged()
{
    const std::wstring &location = m_player.location();
    if (location.empty())
        return;
    if (!m_playing || m_playing->location != location) {
        m_playing = m_history.started(location, m_player.title());
        m_wasPlaying = false;
        return;
    }
    // Until the media plays, the player's state is the previous media's; once stopped, it has none.
    const double position = m_player.position();
    if (m_player.isOpening() || position <= 0)
        return;
    const bool playing = m_player.isPlaying();
    if (playing == m_wasPlaying && std::abs(position - m_playing->position) < saveInterval)
        return;
    m_wasPlaying = playing;
    m_playing->title = m_player.title();
    m_playing->artist = m_player.artist();
    m_playing->audio = !m_player.hasVideo();
    m_playing->position = position;
    m_playing->duration = m_player.duration();
    m_history.save(*m_playing);
}
