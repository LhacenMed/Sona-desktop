#include "data/History.h"

#include <algorithm>
#include <chrono>
#include <cmath>

#include "data/Database.h"

History::History(Database &database)
    : m_database(database)
{
    auto query = m_database.prepare(
        "SELECT location, title, artist, audio, position, duration, last_played, play_count FROM history ORDER BY last_played DESC");
    while (query.step()) {
        m_entries.push_back({query.text(0), query.text(1), query.text(2), query.integer(3) != 0, query.integer(4) / 1000.0,
                             query.integer(5) / 1000.0, query.integer(6), static_cast<int>(query.integer(7))});
    }
}

HistoryEntry History::started(const std::wstring &location, const std::wstring &title)
{
    const auto known = std::ranges::find(m_entries, location, &HistoryEntry::location);
    HistoryEntry entry = known != m_entries.end() ? *known : HistoryEntry{.location = location, .title = title};
    entry.lastPlayed = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    ++entry.playCount;
    save(entry);
    return entry;
}

void History::save(const HistoryEntry &entry)
{
    m_database
        .prepare("INSERT OR REPLACE INTO history (location, title, artist, audio, position, duration, last_played, play_count) "
                 "VALUES (?, ?, ?, ?, ?, ?, ?, ?)")
        .bind(1, entry.location)
        .bind(2, entry.title)
        .bind(3, entry.artist)
        .bind(4, int64_t{entry.audio})
        .bind(5, std::llround(entry.position * 1000))
        .bind(6, std::llround(entry.duration * 1000))
        .bind(7, entry.lastPlayed)
        .bind(8, int64_t{entry.playCount})
        .step();
    std::erase_if(m_entries, [&entry](const HistoryEntry &kept) { return kept.location == entry.location; });
    const auto later = std::ranges::find_if(m_entries, [&entry](const HistoryEntry &kept) { return kept.lastPlayed <= entry.lastPlayed; });
    m_entries.insert(later, entry);
    changed.emit();
}
