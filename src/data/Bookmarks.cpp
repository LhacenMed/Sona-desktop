#include "data/Bookmarks.h"

#include <cmath>

#include "data/Database.h"

void Bookmarks::setMedia(const std::wstring &media)
{
    if (media == m_media)
        return;
    m_media = media;
    reload();
}

void Bookmarks::add(double time)
{
    const std::wstring name = L"Bookmark " + std::to_wstring(m_list.size() + 1);
    m_database.prepare("INSERT INTO bookmarks (media, name, time) VALUES (?, ?, ?)")
        .bind(1, m_media)
        .bind(2, name)
        .bind(3, std::llround(time * 1000))
        .step();
    reload();
}

void Bookmarks::remove(int64_t id)
{
    m_database.prepare("DELETE FROM bookmarks WHERE id = ?").bind(1, id).step();
    reload();
}

void Bookmarks::reload()
{
    m_list.clear();
    auto query = m_database.prepare("SELECT id, name, time FROM bookmarks WHERE media = ? ORDER BY time");
    query.bind(1, m_media);
    while (query.step())
        m_list.push_back({query.integer(0), query.text(1), query.integer(2) / 1000.0});
    changed.emit();
}
