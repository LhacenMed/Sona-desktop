#pragma once

#include <string>
#include <string_view>
#include <vector>

#include "platform/FileDialogs.h"

// What media is: the locations the player opens, which are file paths or URLs.

// Whether a location is a URL (http://, dvd://) rather than a file path.
bool isUrl(std::wstring_view location);
// What a location is called: a file's name, or the URL itself.
std::wstring displayName(const std::wstring &location);
// Whether `a` sorts before `b` in Explorer's order, where numbers compare by value: "Episode 2"
// before "Episode 10".
bool sortsBefore(const std::wstring &a, const std::wstring &b);
// Media files, for the file pickers.
FileType mediaFileType();
// Subtitle files, for the subtitle file picker.
FileType subtitleFileType();
// The media in what was opened or dropped: files as they are, each folder as the media files in it
// and its subfolders in Explorer's order, URLs as they are. Paths become canonical: absolute, with
// long names in their own case.
std::vector<std::wstring> mediaLocations(const std::vector<std::wstring> &opened);
