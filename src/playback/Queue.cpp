#include "playback/Queue.h"

#include <algorithm>

#include "playback/MediaFiles.h"
#include "playback/Player.h"

Queue::Queue(Player &player)
    : m_player(player)
{
    // What reading ahead couldn't tell (a URL's duration, some MP3s') is known once the item plays.
    m_playerSubscription = player.changed.connect([this] {
        if (m_current >= 0 && m_items[m_current].duration == 0 && m_player.duration() > 0) {
            m_items[m_current].duration = m_player.duration();
            changed.emit();
        }
    });
    m_endedSubscription = player.ended.connect([this] { playedToEnd(); });
    // An item that can't play is skipped, never repeated.
    m_failedSubscription = player.failed.connect([this] { playOrStop(step(1, false)); });
}

double Queue::totalDuration() const
{
    double total = 0;
    for (const QueueItem &item : m_items)
        total += item.duration;
    return total;
}

void Queue::cycleRepeat()
{
    m_repeat = m_repeat == Repeat::Off ? Repeat::All : m_repeat == Repeat::All ? Repeat::One : Repeat::Off;
    changed.emit();
}

void Queue::toggleShuffle()
{
    m_shuffles = !m_shuffles;
    if (m_shuffles)
        shuffle();
    else
        m_shuffleOrder.clear();
    changed.emit();
}

void Queue::open(const std::vector<std::wstring> &opened, double startTime)
{
    const std::vector<std::wstring> locations = mediaLocations(opened);
    if (locations.empty())
        return;
    m_items.clear();
    m_shuffleOrder.clear();
    m_current = -1;
    add(locations);
    play(m_shuffles ? indexOf(m_shuffleOrder.front()) : 0, startTime);
    readDurations(0);
}

void Queue::append(const std::vector<std::wstring> &opened)
{
    const std::vector<std::wstring> locations = mediaLocations(opened);
    if (locations.empty())
        return;
    const int first = static_cast<int>(m_items.size());
    add(locations);
    if (m_current < 0)
        play(first);
    else
        changed.emit();
    readDurations(first);
}

void Queue::add(const std::vector<std::wstring> &locations)
{
    for (const std::wstring &location : locations) {
        const int id = ++m_lastId;
        m_items.push_back({id, location, displayName(location)});
        if (m_shuffles) {
            // Somewhere among the items still to play.
            const auto unplayed =
                m_current >= 0 ? std::ranges::find(m_shuffleOrder, m_items[m_current].id) + 1 : m_shuffleOrder.begin();
            std::uniform_int_distribution<std::ptrdiff_t> position(0, m_shuffleOrder.end() - unplayed);
            m_shuffleOrder.insert(unplayed + position(m_random), id);
        }
    }
}

void Queue::readDurations(int first)
{
    for (size_t i = first; i < m_items.size(); ++i) {
        m_player.readDuration(m_items[i].location, [this, id = m_items[i].id](double seconds) {
            if (const int index = indexOf(id); index >= 0) {
                m_items[index].duration = seconds;
                changed.emit();
            }
        });
    }
}

void Queue::play(int index, double startTime)
{
    if (index < 0 || index >= static_cast<int>(m_items.size()))
        return;
    m_current = index;
    m_player.open(m_items[index].location, startTime);
    changed.emit();
}

void Queue::stop()
{
    m_current = -1;
    m_player.stop([] {});
    changed.emit();
}

void Queue::playOrStop(int index)
{
    if (index < 0)
        stop();
    else
        play(index);
}

void Queue::previous()
{
    play(step(-1, m_repeat == Repeat::All));
}

void Queue::next()
{
    play(step(1, m_repeat == Repeat::All));
}

void Queue::move(int from, int to)
{
    const int count = static_cast<int>(m_items.size());
    if (from == to || from < 0 || from >= count || to < 0 || to >= count)
        return;
    const int currentId = m_current >= 0 ? m_items[m_current].id : -1;
    const auto begin = m_items.begin();
    if (from < to)
        std::rotate(begin + from, begin + from + 1, begin + to + 1);
    else
        std::rotate(begin + to, begin + from, begin + from + 1);
    m_current = indexOf(currentId);
    changed.emit();
}

void Queue::remove(int index)
{
    if (index < 0 || index >= static_cast<int>(m_items.size()))
        return;
    std::erase(m_shuffleOrder, m_items[index].id);
    m_items.erase(m_items.begin() + index);
    if (index == m_current) {
        stop();
        return;
    }
    if (index < m_current)
        --m_current;
    changed.emit();
}

void Queue::sortByName()
{
    const auto byName = [](const QueueItem &a, const QueueItem &b) { return sortsBefore(a.name, b.name); };
    const int currentId = m_current >= 0 ? m_items[m_current].id : -1;
    if (std::ranges::is_sorted(m_items, byName))
        std::ranges::reverse(m_items);
    else
        std::ranges::stable_sort(m_items, byName);
    m_current = indexOf(currentId);
    changed.emit();
}

int Queue::step(int direction, bool wraps) const
{
    if (m_current < 0)
        return -1;
    const int count = static_cast<int>(m_items.size());
    const int from = m_shuffles ? static_cast<int>(std::ranges::find(m_shuffleOrder, m_items[m_current].id) - m_shuffleOrder.begin())
                                : m_current;
    int to = from + direction;
    if (to < 0 || to >= count) {
        if (!wraps)
            return -1;
        to = (to + count) % count;
    }
    return m_shuffles ? indexOf(m_shuffleOrder[to]) : to;
}

int Queue::indexOf(int id) const
{
    const auto found = std::ranges::find(m_items, id, &QueueItem::id);
    return found == m_items.end() ? -1 : static_cast<int>(found - m_items.begin());
}

void Queue::shuffle()
{
    m_shuffleOrder.clear();
    for (const QueueItem &item : m_items)
        m_shuffleOrder.push_back(item.id);
    std::ranges::shuffle(m_shuffleOrder, m_random);
    if (m_current >= 0)
        std::iter_swap(m_shuffleOrder.begin(), std::ranges::find(m_shuffleOrder, m_items[m_current].id));
}

void Queue::playedToEnd()
{
    playOrStop(m_repeat == Repeat::One ? m_current : step(1, m_repeat == Repeat::All));
}
