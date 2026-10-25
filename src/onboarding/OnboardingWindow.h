#pragma once

#include <functional>
#include <vector>

#include "maininterface/KeyboardShortcuts.h"
#include "platform/Window.h"

class Player;
class SettingsStore;

// First-run onboarding (Figma "Onboarding" 1899:3055, 1902:3670, 1902:6770): welcome, theme and
// shortcuts. Finishing marks onboarding as done; closing it early shows it again next launch.
class OnboardingWindow : public Window
{
public:
    OnboardingWindow(GraphicsDevice &graphics, Theme &theme, SettingsStore &settings, Player &player,
                     const std::vector<ShortcutGroup> &shortcuts, HWND owner, std::function<void()> closed);

private:
    void closeRequested() override;

    std::function<void()> m_closed;
};
