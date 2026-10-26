#include "playback/MediaFiles.h"

#include <windows.h>
#include <shlwapi.h>

#include <algorithm>
#include <cwctype>
#include <filesystem>
#include <iterator>

namespace {

// The audio and video formats a folder is searched for.
constexpr std::wstring_view mediaExtensions[] = {
    L".3g2", L".3gp", L".asf",  L".avi",  L".divx", L".dv",   L".f4v",  L".flv", L".m2t",  L".m2ts", L".m2v",
    L".m4v", L".mkv", L".mov",  L".mp4",  L".mpeg", L".mpg",  L".mts",  L".mxf", L".ogm",  L".ogv",  L".rm",
    L".rmvb", L".ts", L".vob",  L".webm", L".wmv",  L".aac",  L".ac3",  L".aif", L".aiff", L".alac", L".amr",
    L".ape", L".au",  L".dts",  L".flac", L".m4a",  L".m4b",  L".mka",  L".mp2", L".mp3",  L".mpc",  L".oga",
    L".ogg", L".opus", L".ra",  L".tta",  L".voc",  L".wav",  L".wma",  L".wv",  L".weba", L".xm",
};

bool isMediaFile(const std::filesystem::path &path)
{
    std::wstring extension = path.extension().wstring();
    std::ranges::transform(extension, extension.begin(), [](wchar_t c) { return static_cast<wchar_t>(std::towlower(c)); });
    return std::ranges::find(mediaExtensions, extension) != std::end(mediaExtensions);
}

std::vector<std::wstring> mediaFilesIn(const std::filesystem::path &folder)
{
    std::vector<std::wstring> files;
    std::error_code error;
    for (auto entry = std::filesystem::recursive_directory_iterator(
             folder, std::filesystem::directory_options::skip_permission_denied, error);
         !error && entry != std::filesystem::recursive_directory_iterator(); entry.increment(error)) {
        if (entry->is_regular_file(error) && isMediaFile(entry->path()))
            files.push_back(entry->path().wstring());
    }
    std::ranges::sort(files, sortsBefore);
    return files;
}

} // namespace

bool isUrl(std::wstring_view location)
{
    return location.find(L"://") != std::wstring_view::npos;
}

std::wstring displayName(const std::wstring &location)
{
    return isUrl(location) ? location : std::filesystem::path(location).filename().wstring();
}

FileType mediaFileType()
{
    std::wstring pattern;
    for (const std::wstring_view extension : mediaExtensions)
        pattern += (pattern.empty() ? L"*" : L";*") + std::wstring(extension);
    return {L"Media Files", pattern};
}

bool sortsBefore(const std::wstring &a, const std::wstring &b)
{
    return StrCmpLogicalW(a.c_str(), b.c_str()) < 0;
}

std::vector<std::wstring> mediaLocations(const std::vector<std::wstring> &opened)
{
    std::vector<std::wstring> locations;
    for (const std::wstring &location : opened) {
        if (isUrl(location)) {
            locations.push_back(location);
            continue;
        }
        // One spelling per file (long names, its own case), so what's saved for it, such as
        // bookmarks, is found however it's opened.
        std::error_code error;
        std::filesystem::path path = std::filesystem::canonical(location, error);
        if (error)
            path = std::filesystem::absolute(location);
        if (std::filesystem::is_directory(path, error)) {
            const std::vector<std::wstring> files = mediaFilesIn(path);
            locations.insert(locations.end(), files.begin(), files.end());
        } else {
            locations.push_back(path.wstring());
        }
    }
    return locations;
}
