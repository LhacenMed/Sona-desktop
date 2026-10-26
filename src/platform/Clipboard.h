#pragma once

#include <windows.h>

#include <string>
#include <string_view>

// The Windows clipboard's text; empty when it holds none.
std::wstring clipboardText(HWND owner);
void setClipboardText(HWND owner, std::wstring_view text);
