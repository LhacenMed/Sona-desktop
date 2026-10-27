#pragma once

#include <optional>

#include "core/Signal.h"
#include "data/History.h"

class Player;

// Keeps the history (History) as media plays: each media when it starts, then where it is every few
// seconds, whenever it pauses or plays again, and when it ends.
class HistoryRecorder
{
public:
    HistoryRecorder(Player &player, History &history);

private:
    void playerChanged();

    Player &m_player;
    History &m_history;
    // The playing media's entry, as last saved.
    std::optional<HistoryEntry> m_playing;
    bool m_wasPlaying = false;
    Subscription m_changedSubscription;
    Subscription m_endedSubscription;
};
