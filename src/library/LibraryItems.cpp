#include "library/LibraryItems.h"

#include <windows.h>
#include <Shlwapi.h>

#include <algorithm>
#include <filesystem>
#include <functional>
#include <unordered_map>

#include "data/History.h"
#include "playback/MediaFiles.h"

namespace {

bool matches(const LibraryItem &item, std::wstring_view search)
{
    if (search.empty())
        return true;
    const std::wstring pattern(search);
    return StrStrIW(item.title.c_str(), pattern.c_str()) || StrStrIW(item.artist.c_str(), pattern.c_str());
}

// The folders that hold what played, the most played first, each as recent as its latest play.
std::vector<LibraryItem> frequentFolders(const std::vector<HistoryEntry> &history)
{
    struct Folder
    {
        LibraryItem item;
        int playCount = 0;
    };
    std::unordered_map<std::wstring, Folder> folders;
    for (const HistoryEntry &entry : history) {
        if (isUrl(entry.location))
            continue;
        const std::filesystem::path path = std::filesystem::path(entry.location).parent_path();
        Folder &folder = folders[path.wstring()];
        if (folder.playCount == 0)
            folder.item = {.kind = LibraryItem::Kind::Folder, .location = path.wstring(), .title = path.filename().wstring()};
        folder.playCount += entry.playCount;
        folder.item.lastPlayed = std::max(folder.item.lastPlayed, entry.lastPlayed);
    }
    std::vector<Folder> sorted;
    for (auto &[location, folder] : folders)
        sorted.push_back(std::move(folder));
    std::ranges::sort(sorted, std::greater{}, &Folder::playCount);
    std::vector<LibraryItem> items;
    for (Folder &folder : sorted)
        items.push_back(std::move(folder.item));
    return items;
}

void sortItems(std::vector<LibraryItem> &items, LibrarySort sort)
{
    const auto byText = [](std::wstring LibraryItem::*text, bool descending) {
        return [text, descending](const LibraryItem &a, const LibraryItem &b) {
            return descending ? sortsBefore(b.*text, a.*text) : sortsBefore(a.*text, b.*text);
        };
    };
    switch (sort) {
    case LibrarySort::UnwatchedFirst:
        std::ranges::stable_sort(items, std::less{}, &LibraryItem::progress);
        break;
    case LibrarySort::WatchedFirst:
        std::ranges::stable_sort(items, std::greater{}, &LibraryItem::progress);
        break;
    case LibrarySort::NewestFirst:
        std::ranges::stable_sort(items, std::greater{}, &LibraryItem::lastPlayed);
        break;
    case LibrarySort::OldestFirst:
        std::ranges::stable_sort(items, std::less{}, &LibraryItem::lastPlayed);
        break;
    case LibrarySort::TitleAscending:
    case LibrarySort::TitleDescending:
        std::ranges::stable_sort(items, byText(&LibraryItem::title, sort == LibrarySort::TitleDescending));
        break;
    case LibrarySort::ArtistAscending:
    case LibrarySort::ArtistDescending:
        std::ranges::stable_sort(items, byText(&LibraryItem::artist, sort == LibrarySort::ArtistDescending));
        break;
    case LibrarySort::UriAscending:
    case LibrarySort::UriDescending:
        std::ranges::stable_sort(items, byText(&LibraryItem::location, sort == LibrarySort::UriDescending));
        break;
    default:
        // Orders by what the library doesn't know yet (sortOptions) keep the section's own.
        break;
    }
}

} // namespace

const wchar_t *sectionTitle(HomeSection section)
{
    switch (section) {
    case HomeSection::ContinueWatching:
        return L"Continue Watching";
    case HomeSection::ListenAgain:
        return L"Listen Again";
    case HomeSection::FrequentFolders:
        return L"Frequently Visited Folders";
    }
    return L"";
}

std::vector<LibraryItem> sectionItems(HomeSection section, const std::vector<HistoryEntry> &history, std::wstring_view search,
                                      LibrarySort sort)
{
    std::vector<LibraryItem> items;
    if (section == HomeSection::FrequentFolders) {
        items = frequentFolders(history);
    } else {
        for (const HistoryEntry &entry : history) {
            if (section == HomeSection::ContinueWatching && !entry.audio && entry.position > 0 && !entry.isFinished())
                items.push_back({.kind = LibraryItem::Kind::Video,
                                 .location = entry.location,
                                 .title = entry.title,
                                 .progress = entry.progress(),
                                 .resumeAt = entry.position,
                                 .lastPlayed = entry.lastPlayed});
            else if (section == HomeSection::ListenAgain && entry.audio)
                items.push_back({.kind = LibraryItem::Kind::Audio,
                                 .location = entry.location,
                                 .title = entry.title,
                                 .artist = entry.artist,
                                 .lastPlayed = entry.lastPlayed});
        }
    }
    std::erase_if(items, [search](const LibraryItem &item) { return !matches(item, search); });
    sortItems(items, sort);
    return items;
}
