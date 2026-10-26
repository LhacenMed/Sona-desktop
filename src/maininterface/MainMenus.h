#pragma once

#include <vector>

#include "widgets/Menu.h"

// The main menu, from the title bar's ⋯ (Figma 6062:13354): opening media, the profile, shortcuts,
// customization, settings and about.
std::vector<MenuItem> mainMenuItems();
// The right-click menu over the player (Figma 6065:13616): playback actions. Figma draws only its
// top level; each submenu holds the existing commands of its kind.
std::vector<MenuItem> contextMenuItems();
