#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "core/Signal.h"

class Database;

struct Bookmark
{
    int64_t id;
    std::wstring name;
    // Seconds from the start.
    double time;
};

// Named times in media, saved per media location (Figma "Controls" bookmarks 6320:13763). It holds
// the playing media's, which the bookmarks tab lists and the seek bar marks.
class Bookmarks
{
public:
    explicit Bookmarks(Database &database) : m_database(database) {}

    // The media whose bookmarks these are: the playing one, or none (empty).
    void setMedia(const std::wstring &media);
    bool hasMedia() const { return !m_media.empty(); }
    // The media's bookmarks, earliest first.
    const std::vector<Bookmark> &list() const { return m_list; }
    // Bookmarks `time` in the media, named by its number there ("Bookmark 3").
    void add(double time);
    void remove(int64_t id);

    Signal<> changed;

private:
    void reload();

    Database &m_database;
    std::wstring m_media;
    std::vector<Bookmark> m_list;
};
