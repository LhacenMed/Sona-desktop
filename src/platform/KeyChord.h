#pragma once

#include <windows.h>

#include <string>
#include <vector>

// A key, by its Windows virtual-key code, with the modifiers held with it.
struct KeyChord
{
    UINT key = 0;
    bool control = false;
    bool shift = false;
    bool alt = false;

    bool operator==(const KeyChord &) const = default;
};

// The chord's keys as keycaps name them, modifiers first: {"SHIFT", "S"}, {"Space"}, {"←"}.
std::vector<std::wstring> keyNames(KeyChord chord);
// The chord as menus write it: "Ctrl + O", "Shift + F1", "Space".
std::wstring shortcutText(KeyChord chord);
