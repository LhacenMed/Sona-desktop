#pragma once

#include <optional>
#include <string>
#include <windows.h>

// The Windows file pickers, opening in `startFolder`. Empty when the user cancels.
std::optional<std::wstring> chooseFile(HWND owner, const std::wstring &startFolder);
std::optional<std::wstring> chooseFolder(HWND owner, const std::wstring &startFolder);
