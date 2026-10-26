#pragma once

#include <vector>

#include "widgets/View.h"

class CommandRegistry;
class Label;
class Player;
class SettingsStore;
class Theme;

// Onboarding step 2 (Figma 1902:3670): theme and accent, previewed live on the real player bars.
class ThemePage : public View
{
public:
    ThemePage(Theme &theme, SettingsStore &settings, Player &player, const CommandRegistry &commands);

    void layout() override;

private:
    View *m_preview;
    Label *m_heading;
    std::vector<View *> m_tiles;
    std::vector<View *> m_swatches;
};
