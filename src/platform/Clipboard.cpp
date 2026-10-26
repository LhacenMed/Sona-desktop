#include "platform/Clipboard.h"

#include <cstring>

std::wstring clipboardText(HWND owner)
{
    std::wstring text;
    if (!OpenClipboard(owner))
        return text;
    if (const HANDLE data = GetClipboardData(CF_UNICODETEXT)) {
        if (const auto *characters = static_cast<const wchar_t *>(GlobalLock(data))) {
            text = characters;
            GlobalUnlock(data);
        }
    }
    CloseClipboard();
    return text;
}

void setClipboardText(HWND owner, std::wstring_view text)
{
    if (!OpenClipboard(owner))
        return;
    EmptyClipboard();
    const size_t bytes = (text.size() + 1) * sizeof(wchar_t);
    if (const HGLOBAL data = GlobalAlloc(GMEM_MOVEABLE, bytes)) {
        auto *characters = static_cast<wchar_t *>(GlobalLock(data));
        std::memcpy(characters, text.data(), text.size() * sizeof(wchar_t));
        characters[text.size()] = L'\0';
        GlobalUnlock(data);
        // The clipboard owns the memory once it's set.
        if (!SetClipboardData(CF_UNICODETEXT, data))
            GlobalFree(data);
    }
    CloseClipboard();
}
