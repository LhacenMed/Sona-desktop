#include "style/Theme.h"

#include <algorithm>

#include "platform/SystemTheme.h"

namespace {

Color scaled(Color color, float factor)
{
    return {color.r * factor, color.g * factor, color.b * factor, color.a};
}

Palette makePalette(bool dark, const AccentColors &accent)
{
    const Color accentMain = dark ? accent.dark : accent.light;
    const Color bgB1 = Color::hex(dark ? 0x0e0e0e : 0xe7e7e7);
    return {
        .accentMain = accentMain,
        // Accent/Dim and BG/Accent Dark are defined for orange in Dark only: 81% of the accent's
        // brightness, and the accent at 6% over BG/B1, which reproduce them.
        .accentDim = scaled(accentMain, 0.81f),
        .bgAccentBright = dark ? accent.darkDim : accent.lightDim,
        .bgAccentDark = Color::mix(bgB1, accentMain, 0.06f),
        .bgA2 = Color::hex(dark ? 0x1f1f1f : 0xbdbdbd),
        .bgA3 = Color::hex(dark ? 0x1b1b1b : 0xd5d5d5),
        .bgPopup = dark ? Color::hex(0x191919, 0.92f) : Color::hex(0xd5d5d5, 0.92f),
        .bgPanel = dark ? Color::hex(0x0e0e0e, 0.92f) : Color::hex(0xe7e7e7, 0.92f),
        .bgA4 = Color::hex(dark ? 0x191919 : 0xacacac),
        .bgA5 = Color::hex(dark ? 0x151515 : 0x888888),
        .bgB1 = bgB1,
        .bgB1Transparent = dark ? Color::hex(0x0e0e0e, 0.95f) : Color::hex(0xe7e7e7, 0.8f),
        .bgB2 = Color::hex(dark ? 0x090909 : 0xdedede),
        // FG/1, FG/2, FG/A1, FG/A4, BG/A2 and BG/A4 are not defined for Light in Figma; chosen to
        // continue the Light scales.
        .fg1 = Color::hex(dark ? 0xeaeaea : 0x161616),
        .fg2 = Color::hex(dark ? 0xc6c6c6 : 0x2e2e2e),
        .fgA1 = Color::hex(dark ? 0x969696 : 0x3d3d3d),
        .fgA2 = Color::hex(dark ? 0x8f8f8f : 0x2e2e2e),
        .fgA3 = Color::hex(0x6b6b6b),
        .fgA4 = Color::hex(dark ? 0x404040 : 0x9a9a9a),
        .fgB1 = Color::hex(0x161616),
        .dividerBright = Color::hex(dark ? 0x373737 : 0x9b9b9b),
        // BG/B2 in Dark, BG/A5 in Light.
        .volumeTrack = Color::hex(dark ? 0x090909 : 0x888888),
        // Dark hover is the Figma "Icon Wrapper" fill; the others are the Windows 11 caption button values.
        .hoverOverlay = dark ? Color{1, 1, 1, 0.06f} : Color{0, 0, 0, 0.0373f},
        .pressedOverlay = dark ? Color{1, 1, 1, 0.0419f} : Color{0, 0, 0, 0.0241f},
        .closeHover = Color::hex(0xc42b1c),
        .closePressed = Color::hex(0xc42b1c, 0.9f),
        .closeForeground = Color::hex(0xffffff),
    };
}

} // namespace

Theme::Theme(SettingsStore &settings)
    : m_settings(settings)
{
    resolve();
    m_settingsSubscription = settings.changed.connect([this] {
        if (resolve())
            changed.emit();
    });
}

void Theme::refresh()
{
    if (resolve())
        changed.emit();
}

bool Theme::resolve()
{
    const ThemeMode mode = m_settings.get(AppSettings::theme);
    const bool dark = mode == ThemeMode::Dark || (mode == ThemeMode::System && !systemPrefersLightTheme());
    const int accentIndex = std::clamp(m_settings.get(AppSettings::accentIndex), 0, static_cast<int>(accentColors.size()) - 1);
    if (mode == m_mode && dark == m_dark && accentIndex == m_accentIndex)
        return false;
    m_mode = mode;
    m_dark = dark;
    m_accentIndex = accentIndex;
    m_palette = makePalette(dark, accentColors[accentIndex]);
    return true;
}
