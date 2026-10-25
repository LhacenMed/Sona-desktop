#include "maininterface/TitleBar.h"

#include <algorithm>
#include <string_view>

#include "graphics/Canvas.h"
#include "maininterface/CaptionButton.h"
#include "platform/Window.h"
#include "playback/Player.h"
#include "style/Theme.h"
#include "style/Typography.h"
#include "widgets/Label.h"

namespace {

// Window chrome text, as Windows sets the window title.
constexpr TextStyle captionText{.size = 12, .elide = true, .font = TextFont::System};
// The bar's own buttons: narrower than the 46px caption buttons, with slightly larger glyphs, as
// toolbar icons in native title bars are.
constexpr float toolButtonWidth = 40;
constexpr float toolGlyphSize = 12;

// The app logo at the size of a native window icon; it opens the system menu like one.
class WindowIcon : public View
{
public:
    WindowIcon() { setPreferredSize({36, 0}); }

    bool takesMouse() const override { return true; }
    HitRole hitRole() const override { return HitRole::SystemMenu; }
    void paint(Canvas &canvas) override
    {
        constexpr float size = 16;
        canvas.drawSvg(L"logo/sona-logo.svg", {(frame().width - size) / 2, (frame().height - size) / 2, size, size});
    }
};

// A glyph that is a label, not a button.
class Glyph : public View
{
public:
    explicit Glyph(wchar_t glyph) : m_glyph(glyph) { setPreferredSize({toolGlyphSize + 8, 0}); }

    void paint(Canvas &canvas) override
    {
        const TextStyle style{.size = toolGlyphSize, .align = TextAlign::Center, .font = TextFont::Icons};
        canvas.drawText(std::wstring_view(&m_glyph, 1), bounds(), style, canvas.palette().fgA2);
    }

private:
    wchar_t m_glyph;
};

// A thin vertical rule between button groups.
class Divider : public View
{
public:
    Divider() { setPreferredSize({9, 0}); }

    void paint(Canvas &canvas) override
    {
        constexpr float height = 16;
        canvas.fillRect({4, (frame().height - height) / 2, 1, height}, canvas.palette().dividerBright);
    }
};

} // namespace

TitleBar::TitleBar(Player &player, bool preview)
    : m_preview(preview)
{
    m_leading = &add<Row>();
    m_leading->setFillsHeight(true);
    m_leading->add<WindowIcon>().setFillsHeight(true);
    m_leading->add<CaptionButton>(Typography::Glyph::more, toolButtonWidth, toolGlyphSize);
    m_leading->add<CaptionButton>(Typography::Glyph::sidebar, toolButtonWidth, toolGlyphSize);
    m_leading->add<Divider>().setFillsHeight(true);
    m_leading->add<Glyph>(Typography::Glyph::folder).setFillsHeight(true);
    m_folderLabel = &m_leading->add<Label>(player.folderName(), captionText, &Palette::fgA2);
    m_folderLabel->setFillsWidth(true);
    m_folderLabel->setFillsHeight(true);
    m_folderLabel->setPadding(2, 8);

    m_titleLabel = &add<Label>(
        L"", TextStyle{.size = 12, .align = TextAlign::Center, .elide = true, .font = TextFont::System}, &Palette::fg1);
    m_titleLabel->setFillsWidth(true);
    m_titleLabel->setFillsHeight(true);

    // The full-screen button, then Windows' caption buttons flush with the window corner.
    m_trailing = &add<Row>();
    m_trailing->setFillsHeight(true);
    m_trailing->add<View>().setFillsWidth(true);
    m_fullScreenButton = &m_trailing->add<CaptionButton>(Typography::Glyph::fullScreen);
    m_fullScreenButton->setOnClick([this] { window()->toggleFullScreen(); });
    if (preview) {
        for (wchar_t glyph : {Typography::Glyph::minimize, Typography::Glyph::maximize, Typography::Glyph::close})
            m_trailing->add<CaptionButton>(glyph);
    } else {
        // Their spot: Windows draws them there, behind the content.
        m_nativeButtons = &m_trailing->add<View>();
    }

    m_playerSubscription = player.changed.connect([this, &player] { m_folderLabel->setText(player.folderName()); });
}

void TitleBar::setTitle(std::wstring title)
{
    m_titleLabel->setText(std::move(title));
}

void TitleBar::layout()
{
    // Maximizing, full screen and DPI changes resize the window, which lays the bar out again.
    const Window *host = window();
    const Rect native = host && !m_preview ? host->captionButtonBounds() : Rect{};
    m_onNativeCaption = native.width > 0;
    if (m_nativeButtons)
        m_nativeButtons->setPreferredSize({native.width, 0});
    // As wide as each of Windows' three (minimize, maximize, close).
    if (m_onNativeCaption)
        m_fullScreenButton->setPreferredSize({native.width / 3, 0});
    if (host) {
        m_fullScreenButton->setGlyph(host->isFullScreen() ? Typography::Glyph::exitFullScreen
                                                          : Typography::Glyph::fullScreen);
    }
    // Both sides as wide, so the title stays centred on the window.
    const float trailingWidth = m_fullScreenButton->preferredSize().width + (m_preview ? 3 * 46 : native.width);
    const float sideWidth = std::max(240.0f, trailingWidth);
    m_leading->setPreferredSize({sideWidth, 0});
    m_trailing->setPreferredSize({sideWidth, 0});
    Row::layout();
}

void TitleBar::paint(Canvas &canvas)
{
    // Over the native caption, Windows paints the background; elsewhere the bar paints it.
    if (m_onNativeCaption)
        canvas.clear(bounds(), Color{0, 0, 0, 0});
    else
        canvas.fillRect(bounds(), canvas.palette().bgA3);
    // The title dims with the window, as native titles do.
    const Window *host = window();
    m_titleLabel->setColor(!host || host->isActive() ? &Palette::fg1 : &Palette::fgA2);
}
