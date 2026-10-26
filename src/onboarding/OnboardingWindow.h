#pragma once

#include "platform/Window.h"

struct BarContext;

// First-run onboarding (Figma "Onboarding" 1899:3055, 1902:3670, 1902:6770): welcome, theme and
// shortcuts, modal over its owner. Finishing marks onboarding as done; closing it early shows it
// again next launch.
class OnboardingWindow : public Window
{
public:
    // `bars` is the player's, which the theme step previews.
    OnboardingWindow(GraphicsDevice &graphics, Theme &theme, const BarContext &bars, HWND owner);
};
