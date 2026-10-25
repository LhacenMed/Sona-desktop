#pragma once

#include <windows.h>

#include <functional>
#include <optional>
#include <string>
#include <vector>

class Player;

// A key, by its Windows virtual-key code, with or without Shift.
struct KeyChord
{
    UINT key;
    bool shift = false;

    bool operator==(const KeyChord &) const = default;
};

// One key or key combination of a shortcut: how the guide shows it and, once a feature is behind
// it, the chord that runs it.
struct ShortcutBinding
{
    std::vector<std::wstring> keyLabels;
    std::optional<KeyChord> chord;
    std::function<void()> run;
};

struct ShortcutGroup
{
    std::wstring title;
    std::vector<ShortcutBinding> bindings;
};

// Every player shortcut, in the order of the onboarding guide (Figma 1902:6770): one list drives
// both the key bindings and the guide. A binding without a chord is listed in the guide but has no
// feature behind it yet.
std::vector<ShortcutGroup> makeKeyboardShortcuts(Player &player, std::function<void()> toggleFullScreen);

// Runs the shortcut bound to `chord`; returns whether there was one.
bool runShortcut(const std::vector<ShortcutGroup> &groups, KeyChord chord);
