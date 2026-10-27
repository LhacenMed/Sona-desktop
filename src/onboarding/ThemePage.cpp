#include "onboarding/ThemePage.h"

#include <string>

#include "graphics/Canvas.h"
#include "player/BarItems.h"
#include "player/ControlBar.h"
#include "player/TitleBar.h"
#include "settings/AppSettings.h"
#include "style/Theme.h"
#include "widgets/Label.h"

namespace {

constexpr TextStyle headingStyle{.size = 24, .weight = DWRITE_FONT_WEIGHT_LIGHT, .align = TextAlign::Center};

// The real title bar and control bar on a 1920×1080 player, drawn at 55% (Figma 1902:3671).
// It is a picture only: it takes no clicks.
class PlayerPreview : public View
{
public:
    explicit PlayerPreview(const BarContext &bars)
        : m_context(bars)
        , m_titleBar(add<TitleBar>(m_context, defaultBarLayout))
        , m_controlBar(add<ControlBar>(m_context))
    {
    }

    void layout() override
    {
        // Windows' caption height at 100% scale.
        m_titleBar.setFrame({0, 0, 1920, 30});
        m_controlBar.setFrame({0, 1080 - 54, 1920, 54});
    }

    void paintTree(Canvas &canvas) override
    {
        canvas.save();
        canvas.translate(frame().x, frame().y);
        canvas.pushClip(bounds());
        canvas.fillRect(bounds(), canvas.palette().bgB2);
        canvas.scale(0.55f);
        for (const auto &child : children())
            child->paintTree(canvas);
        canvas.popClip();
        canvas.restore();
    }

    View *hitTest(Point) override { return nullptr; }

private:
    BarContext m_context;
    TitleBar &m_titleBar;
    ControlBar &m_controlBar;
};

// A theme choice: a 100px face, a selection ring and a label.
class ThemeTile : public View
{
public:
    ThemeTile(Theme &theme, SettingsStore &settings, ThemeMode mode, std::wstring label)
        : m_theme(theme)
        , m_settings(settings)
        , m_mode(mode)
        , m_label(std::move(label))
    {
    }

    bool takesMouse() const override { return true; }
    Cursor cursor() const override { return Cursor::Hand; }

    void paint(Canvas &canvas) override
    {
        const Palette &palette = canvas.palette();
        // A 4px inside ring.
        if (m_theme.mode() == m_mode)
            canvas.strokeRoundedRect({2, 2, 120, 120}, 17, 4, palette.accentMain);

        const Rect face{12, 12, 100, 100};
        constexpr Color light = Color::hex(0xafafaf);
        constexpr Color dark = Color::hex(0x222222);
        if (m_mode == ThemeMode::System) {
            // A hard diagonal split: the CSS 238.7° gradient with both stops at 52.4%.
            canvas.fillRoundedRect(face, 12, {face.x + 108.7f, face.y + 14.3f}, {face.x - 8.7f, face.y + 85.7f},
                                   {{0.524f, light}, {0.524f, dark}});
        } else {
            canvas.fillRoundedRect(face, 12, m_mode == ThemeMode::Light ? light : dark);
        }

        constexpr TextStyle labelStyle{.size = 16, .weight = DWRITE_FONT_WEIGHT_LIGHT, .align = TextAlign::Center};
        canvas.drawText(m_label, {0, 142, frame().width, canvas.measureText(m_label, labelStyle).height}, labelStyle,
                        palette.fgA2);
    }

    void mouseReleased(Point, bool inside) override
    {
        if (inside)
            m_settings.set(AppSettings::theme, m_mode);
    }

private:
    Theme &m_theme;
    SettingsStore &m_settings;
    ThemeMode m_mode;
    std::wstring m_label;
};

// An accent choice: a 40px dot, dimmed until selected, then ringed in its own color.
class AccentSwatch : public View
{
public:
    AccentSwatch(Theme &theme, SettingsStore &settings, int index)
        : m_theme(theme)
        , m_settings(settings)
        , m_index(index)
    {
    }

    bool takesMouse() const override { return true; }
    Cursor cursor() const override { return Cursor::Hand; }

    void paint(Canvas &canvas) override
    {
        const AccentColors &accent = accentColors[m_index];
        Color color = m_theme.isDark() ? accent.dark : accent.light;
        const bool selected = m_theme.accentIndex() == m_index;
        const Point center{frame().width / 2, frame().height / 2};
        if (selected) {
            Color ring = color;
            ring.a = 0.3f;
            // A 4px inside ring on the 64px circle.
            canvas.strokeCircle(center, 30, 4, ring);
        }
        color.a = selected ? 1 : 0.5f;
        canvas.fillCircle(center, 20, color);
    }

    void mouseReleased(Point, bool inside) override
    {
        if (inside)
            m_settings.set(AppSettings::accentIndex, m_index);
    }

private:
    Theme &m_theme;
    SettingsStore &m_settings;
    int m_index;
};

} // namespace

ThemePage::ThemePage(Theme &theme, const BarContext &bars)
{
    SettingsStore &settings = bars.settings;
    m_preview = &add<PlayerPreview>(bars);
    m_heading = &add<Label>(L"Select your preferred theme and color.", headingStyle, &Palette::fgA1);
    m_tiles = {&add<ThemeTile>(theme, settings, ThemeMode::Light, L"Light"),
               &add<ThemeTile>(theme, settings, ThemeMode::Dark, L"Dark"),
               &add<ThemeTile>(theme, settings, ThemeMode::System, L"System")};
    for (int index = 0; index < static_cast<int>(accentColors.size()); ++index)
        m_swatches.push_back(&add<AccentSwatch>(theme, settings, index));
}

void ThemePage::layout()
{
    m_preview->setFrame({17, 19, 1056, 594});
    m_heading->setFrame({0, 173, frame().width, m_heading->preferredSize().height});
    for (size_t i = 0; i < m_tiles.size(); ++i)
        m_tiles[i]->setFrame({327 + 156.0f * static_cast<float>(i), 282, 124, 166});
    for (size_t i = 0; i < m_swatches.size(); ++i)
        m_swatches[i]->setFrame({41 + 80.0f * static_cast<float>(i), 655, 64, 64});
}
