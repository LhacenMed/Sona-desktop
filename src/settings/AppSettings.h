#pragma once

#include <string>

#include "platform/KnownFolders.h"
#include "settings/SettingsStore.h"

enum class ThemeMode { Light, Dark, System };

// How the library lists media (Figma "View Options" 858:3290).
enum class LibraryLayout { Grid, List };

// The library's orders, as Figma's "Sorting Options" (856:2215) lists them.
enum class LibrarySort {
    UnwatchedFirst,
    WatchedFirst,
    NewestFirst,
    OldestFirst,
    TitleAscending,
    TitleDescending,
    ArtistAscending,
    ArtistDescending,
    AlbumAscending,
    AlbumDescending,
    TrackNumberAscending,
    TrackNumberDescending,
    UriAscending,
    UriDescending,
    DiscNumberAscending,
    DiscNumberDescending,
};

// Every persisted preference, with its registry value name and default. Adding a preference is
// adding a line here.
namespace AppSettings {

inline const Setting<bool> onboardingCompleted{L"onboardingCompleted", false};
inline const Setting<ThemeMode> theme{L"theme", ThemeMode::System};
inline const Setting<int> accentIndex{L"accentIndex", 0};
inline const Setting<std::wstring> mediaFolder{L"mediaFolder", knownFolderPath(FOLDERID_Videos)};
// The control bar shows the time left instead of the duration.
inline const Setting<bool> showRemainingTime{L"showRemainingTime", false};
inline const Setting<LibraryLayout> libraryLayout{L"libraryLayout", LibraryLayout::Grid};
inline const Setting<LibrarySort> librarySort{L"librarySort", LibrarySort::NewestFirst};

} // namespace AppSettings
