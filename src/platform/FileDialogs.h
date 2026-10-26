#pragma once

#include <functional>
#include <string>
#include <vector>

class Window;

// A kind of file a picker lists, as Windows writes it: {L"Media Files", L"*.mkv;*.mp4"}.
struct FileType
{
    std::wstring name;
    std::wstring pattern;
};

// The Windows file pickers, over `owner` and opening in `startFolder`. Each opens once the click or
// key that asked for it has been handled (Window::runLater), then gives what was chosen to `chosen`;
// `chosen` isn't called when the user cancels.

// Files of `type` (or, by choice, of any type).
void chooseFiles(Window &owner, const std::wstring &startFolder, const FileType &type,
                 std::function<void(std::vector<std::wstring> files)> chosen);
// A folder, Windows' folder picker listing folders only.
void chooseFolder(Window &owner, const std::wstring &startFolder, std::function<void(std::wstring folder)> chosen);
