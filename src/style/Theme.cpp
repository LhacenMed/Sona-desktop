#include "style/Theme.h"

#include <algorithm>

#include "platform/SystemTheme.h"

namespace {

Palette makePalette(bool dark, const AccentColors &accent)
{
    return {
        .accentMain = dark ? accent.dark : accent.light,
        .bgAccentBright = dark ? accent.darkDim : accent.lightDim,
        .bgA3 = Color::hex(dark ? 0x1b1b1b : 0xd5d5d5),
        .bgA4 = Color::hex(dark ? 0x191919 : 0xacacac),
        .bgB1 = Color::hex(dark ? 0x0e0e0e : 0xe7e7e7),
        .bgB2 = Color::hex(dark ? 0x090909 : 0xdedede),
        // FG/1, FG/2, FG/A1 and BG/A4 are not defined for Light in Figma; chosen to continue the Light foreground scale.
        .fg1 = Color::hex(dark ? 0xeaeaea : 0x161616),
        .fg2 = Color::hex(dark ? 0xc6c6c6 : 0x2e2e2e),
        .fgA1 = Color::hex(dark ? 0x969696 : 0x3d3d3d),
        .fgA2 = Color::hex(dark ? 0x8f8f8f : 0x2e2e2e),
        .fgA3 = Color::hex(0x6b6b6b),
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
