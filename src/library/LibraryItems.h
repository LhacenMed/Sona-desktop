#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include "settings/AppSettings.h"

struct HistoryEntry;

// What a library tile or row shows, and what it opens.
struct LibraryItem
{
    enum class Kind { Video, Audio, Folder };

    Kind kind;
    // A file, or a folder.
    std::wstring location;
    std::wstring title;
    // A song's; empty for the rest.
    std::wstring artist;
    // How much of a video has been watched, 0–1.
    double progress = 0;
    // Where it plays from, in seconds: a partly watched video goes on where it was left.
    double resumeAt = 0;
    // Seconds since 1970.
    int64_t lastPlayed = 0;
};

// A sort order's name in the sort menu, and whether the library knows what it sorts by.
struct SortOption
{
    LibrarySort sort;
    const wchar_t *label;
    bool available;
};

// Every order, in Figma's menu order. Albums, track and disc numbers come with the indexer, which
// reads media metadata.
inline constexpr std::array<SortOption, 16> sortOptions{{
    {LibrarySort::UnwatchedFirst, L"Unwatched First", true},
    {LibrarySort::WatchedFirst, L"Watched First", true},
    {LibrarySort::NewestFirst, L"Newest First", true},
    {LibrarySort::OldestFirst, L"Oldest First", true},
    {LibrarySort::TitleAscending, L"Title A-Z", true},
    {LibrarySort::TitleDescending, L"Title Z-A", true},
    {LibrarySort::ArtistAscending, L"Artist A-Z", true},
    {LibrarySort::ArtistDescending, L"Artist Z-A", true},
    {LibrarySort::AlbumAscending, L"Album A-Z", false},
    {LibrarySort::AlbumDescending, L"Album Z-A", false},
    {LibrarySort::TrackNumberAscending, L"Track Number Ascending", false},
    {LibrarySort::TrackNumberDescending, L"Track Number Descending", false},
    {LibrarySort::UriAscending, L"URI Ascending", true},
    {LibrarySort::UriDescending, L"URI Descending", true},
    {LibrarySort::DiscNumberAscending, L"Disc Number Ascending", false},
    {LibrarySort::DiscNumberDescending, L"Disc Number Descending", false},
}};

// Figma Home (6261:15078), as VLC's home page: what's being watched, what was listened to, and the
// folders played most, from the history.
enum class HomeSection { ContinueWatching, ListenAgain, FrequentFolders };

inline constexpr std::array homeSections{HomeSection::ContinueWatching, HomeSection::ListenAgain, HomeSection::FrequentFolders};

const wchar_t *sectionTitle(HomeSection section);
// The section's items from `history` whose title or artist holds `search` (any case), in `sort`.
std::vector<LibraryItem> sectionItems(HomeSection section, const std::vector<HistoryEntry> &history, std::wstring_view search,
                                      LibrarySort sort);
