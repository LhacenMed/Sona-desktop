#pragma once

#include "platform/Window.h"

class CommandRegistry;
class Player;
class SettingsStore;

// First-run onboarding (Figma "Onboarding" 1899:3055, 1902:3670, 1902:6770): welcome, theme and
// shortcuts, modal over its owner. Finishing marks onboarding as done; closing it early shows it
// again next launch.
class OnboardingWindow : public Window
{
public:
    OnboardingWindow(GraphicsDevice &graphics, Theme &theme, SettingsStore &settings, Player &player,
                     const CommandRegistry &commands, HWND owner);
};
