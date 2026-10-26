#include "platform/KeyChord.h"

std::vector<std::wstring> keyNames(KeyChord chord)
{
    std::vector<std::wstring> names;
    if (chord.control)
        names.emplace_back(L"CTRL");
    if (chord.shift)
        names.emplace_back(L"SHIFT");
    if (chord.alt)
        names.emplace_back(L"ALT");

    switch (chord.key) {
    case VK_SPACE:
        names.emplace_back(L"Space");
        break;
    case VK_ESCAPE:
        names.emplace_back(L"Esc");
        break;
    case VK_LEFT:
        names.emplace_back(L"←");
        break;
    case VK_RIGHT:
        names.emplace_back(L"→");
        break;
    case VK_UP:
        names.emplace_back(L"↑");
        break;
    case VK_DOWN:
        names.emplace_back(L"↓");
        break;
    default:
        if (chord.key >= VK_F1 && chord.key <= VK_F24) {
            names.push_back(L"F" + std::to_wstring(chord.key - VK_F1 + 1));
        } else {
            // Letters, digits and punctuation, as the keyboard layout prints them.
            const wchar_t character = static_cast<wchar_t>(MapVirtualKeyW(chord.key, MAPVK_VK_TO_CHAR) & 0x7fff);
            names.emplace_back(1, character);
        }
    }
    return names;
}
