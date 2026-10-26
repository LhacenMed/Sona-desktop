#pragma once

#include <random>
#include <string>
#include <vector>

#include "core/Signal.h"

class Player;

// How the queue repeats: not at all, all of it, or the playing item.
enum class Repeat { Off, All, One };

struct QueueItem
{
    // Stays with the item as the queue is reordered.
    int id;
    std::wstring location;
    std::wstring name;
    // Seconds; 0 until it's known.
    double duration = 0;
};

// What plays next (ROADMAP 5.3): the items, which one is playing, and how the queue moves on. The
// queue is the one place media is opened from. It plays one item at a time through the Player and
// moves to the next as each one ends, by the repeat mode and in shuffled order when shuffling.
class Queue
{
public:
    explicit Queue(Player &player);

    const std::vector<QueueItem> &items() const { return m_items; }
    // The playing item's index, or -1 before anything plays.
    int current() const { return m_current; }
    const QueueItem *currentItem() const { return m_current >= 0 ? &m_items[m_current] : nullptr; }
    // The sum of the durations known so far.
    double totalDuration() const;
    Repeat repeat() const { return m_repeat; }
    // Off, then All, then One.
    void cycleRepeat();
    bool shuffles() const { return m_shuffles; }
    void toggleShuffle();

    // Replaces the queue with the media in `opened` (see mediaLocations) and plays the first.
    void open(const std::vector<std::wstring> &opened);
    // Adds the media in `opened` to the end; with nothing playing, the first added plays.
    void append(const std::vector<std::wstring> &opened);
    void play(int index);
    bool hasPrevious() const { return step(-1, m_repeat == Repeat::All) >= 0; }
    bool hasNext() const { return step(1, m_repeat == Repeat::All) >= 0; }
    void previous();
    void next();
    // Moves an item to `to`, its index once moved.
    void move(int from, int to);
    // Removing the playing item stops it.
    void remove(int index);
    // By name, in Explorer's order; sorted already, it reverses.
    void sortByName();

    Signal<> changed;

private:
    // The index of the item before (-1) or after (1) the playing one in play order, wrapping past
    // the ends if `wraps`; -1 when there's none.
    int step(int direction, bool wraps) const;
    int indexOf(int id) const;
    // Adds media locations to the end, reading their durations.
    void add(const std::vector<std::wstring> &locations);
    // A new play order for shuffling, which starts with the playing item.
    void shuffle();
    void playedToEnd();

    Player &m_player;
    std::vector<QueueItem> m_items;
    int m_current = -1;
    int m_lastId = 0;
    Repeat m_repeat = Repeat::Off;
    bool m_shuffles = false;
    // While shuffling, every item's id in the order they play.
    std::vector<int> m_shuffleOrder;
    std::mt19937 m_random{std::random_device{}()};
    Subscription m_playerSubscription;
    Subscription m_endedSubscription;
    Subscription m_failedSubscription;
};
