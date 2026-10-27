#pragma once

#include <functional>
#include <vector>

#include "player/BarLayout.h"

class ActivePreset;
class Bookmarks;
class CommandRegistry;
class Player;
class Row;
class SettingsStore;
class View;

// What the bars' items work with.
struct BarContext
{
    Player &player;
    // The playing media's, marked on the seek bar.
    Bookmarks &bookmarks;
    SettingsStore &settings;
    // The preset in force, which the preferences panel changes.
    ActivePreset &preset;
    const CommandRegistry &commands;
    // Opens the main menu from the button that asks for it.
    std::function<void(View &button)> openMainMenu;
};

// Builds a bar's groups, in order, into `bar`. Buttons for commands run them, show their name and
// shortcut as a tooltip, and are dimmed while the command has no feature behind it.
void addBarGroups(Row &bar, const std::vector<BarGroup> &groups, BarContext &context);
