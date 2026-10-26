#include "platform/KeyChord.h"

namespace {

// The key without its modifiers.
std::wstring keyName(UINT key)
{
    switch (key) {
    case VK_SPACE:
        return L"Space";
    case VK_ESCAPE:
        return L"Esc";
    case VK_LEFT:
        return L"←";
    case VK_RIGHT:
        return L"→";
    case VK_UP:
        return L"↑";
    case VK_DOWN:
        return L"↓";
    }
    if (key >= VK_F1 && key <= VK_F24)
        return L"F" + std::to_wstring(key - VK_F1 + 1);
    // Letters, digits and punctuation, as the keyboard layout prints them.
    return std::wstring(1, static_cast<wchar_t>(MapVirtualKeyW(key, MAPVK_VK_TO_CHAR) & 0x7fff));
}

} // namespace

std::vector<std::wstring> keyNames(KeyChord chord)
{
    std::vector<std::wstring> names;
    if (chord.control)
        names.emplace_back(L"CTRL");
    if (chord.shift)
        names.emplace_back(L"SHIFT");
    if (chord.alt)
        names.emplace_back(L"ALT");
    names.push_back(keyName(chord.key));
    return names;
}

std::wstring shortcutText(KeyChord chord)
{
    std::wstring text;
    if (chord.control)
        text += L"Ctrl + ";
    if (chord.shift)
        text += L"Shift + ";
    if (chord.alt)
        text += L"Alt + ";
    return text + keyName(chord.key);
}
