#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "core/Signal.h"

class Database;

// A media that played: where it was left and how often it plays.
struct HistoryEntry
{
    std::wstring location;
    std::wstring title;
    // Empty when the media names none.
    std::wstring artist;
    // Sound only: no video showed.
    bool audio = false;
    // Seconds; the duration is 0 until it's known.
    double position = 0;
    double duration = 0;
    // Seconds since 1970.
    int64_t lastPlayed = 0;
    int playCount = 0;

    // How much has been played, 0–1; 0 while the duration is unknown.
    double progress() const { return duration > 0 ? position / duration : 0; }
    // Played to its end, or so near it (the credits) that it doesn't continue.
    bool isFinished() const { return progress() >= 0.95; }
};

// Every media played, kept in the database (Figma Home's Continue Watching, Listen Again and
// Frequently Visited Folders come from it).
class History
{
public:
    explicit History(Database &database);

    // Most recently played first.
    const std::vector<HistoryEntry> &entries() const { return m_entries; }
    // `location` started playing now: its entry, with one more play.
    HistoryEntry started(const std::wstring &location, const std::wstring &title);
    void save(const HistoryEntry &entry);

    Signal<> changed;

private:
    Database &m_database;
    std::vector<HistoryEntry> m_entries;
};
