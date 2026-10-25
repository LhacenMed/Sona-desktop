#pragma once

#include <array>

#include "core/Color.h"
#include "core/Signal.h"
#include "settings/AppSettings.h"

// An accent choice from the onboarding swatches (Figma 1902:3670), with its dimmed variant (the
// elapsed seek fill, BG/Accent Bright) in each theme. Orange is the Figma token pair; the others
// use the 24% blend over BG/B2 measured on the red variant, and in Light a 35% blend over
// Light/BG/B2 (Figma defines no Light values for them).
struct AccentColors
{
    Color dark;
    Color darkDim;
    Color light;
    Color lightDim;
};

inline constexpr std::array<AccentColors, 9> accentColors{{
    {Color::hex(0xd37918), Color::hex(0x371f06), Color::hex(0xe8851a), Color::hex(0xd8ba9a)},
    {Color::hex(0xe15032), Color::hex(0x3d1a13), Color::hex(0xe15032), Color::hex(0xdfaca2)},
    {Color::hex(0xc49e23), Color::hex(0x362d0f), Color::hex(0xc49e23), Color::hex(0xd5c89d)},
    {Color::hex(0x19a234), Color::hex(0x0d2e13), Color::hex(0x19a234), Color::hex(0x99c9a2)},
    {Color::hex(0x18aad3), Color::hex(0x0d3039), Color::hex(0x18aad3), Color::hex(0x99ccda)},
    {Color::hex(0x185dd3), Color::hex(0x0d1d39), Color::hex(0x185dd3), Color::hex(0x99b1da)},
    {Color::hex(0x8e42ff), Color::hex(0x291744), Color::hex(0x8e42ff), Color::hex(0xc2a7ea)},
    {Color::hex(0xf052b3), Color::hex(0x401b32), Color::hex(0xf052b3), Color::hex(0xe4adcf)},
    {Color::hex(0xafafaf), Color::hex(0x313131), Color::hex(0xafafaf), Color::hex(0xcecece)},
}};

// Design tokens from the Figma "VLC Redesign" file, resolved for the current theme and accent.
// Names follow the Figma variables (Dark/… and Light/…).
struct Palette
{
    Color accentMain;
    Color bgAccentBright;
    Color bgA3;
    Color bgA4;
    Color bgB1;
    Color bgB2;
    Color fg1;
    Color fg2;
    Color fgA1;
    Color fgA2;
    Color fgA3;
    Color fgB1;
    Color dividerBright;
    Color volumeTrack;
    // Button states shared by every button.
    Color hoverOverlay;
    Color pressedOverlay;
    Color closeHover;
    Color closePressed;
    Color closeForeground;
};

// The active palette, following the theme and accent settings and, for the System theme, the
// Windows app mode.
class Theme
{
public:
    explicit Theme(SettingsStore &settings);

    const Palette &palette() const { return m_palette; }
    bool isDark() const { return m_dark; }
    ThemeMode mode() const { return m_mode; }
    int accentIndex() const { return m_accentIndex; }

    // Re-reads the Windows app mode after it changed.
    void refresh();

    Signal<> changed;

private:
    // Resolves the palette; returns whether anything changed.
    bool resolve();

    SettingsStore &m_settings;
    Subscription m_settingsSubscription;
    Palette m_palette{};
    ThemeMode m_mode = ThemeMode::System;
    bool m_dark = true;
    int m_accentIndex = -1;
};
