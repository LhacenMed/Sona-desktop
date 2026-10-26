#include "player/TitleBar.h"

#include <algorithm>

#include "graphics/Canvas.h"
#include "platform/Window.h"
#include "player/BarItems.h"
#include "style/Theme.h"
#include "widgets/Label.h"
#include "widgets/WindowButtons.h"

TitleBar::TitleBar(BarContext &context)
{
    m_leading = &add<Row>();
    m_leading->setFillsHeight(true);
    addBarGroups(*m_leading, defaultBarLayout.titleLeading, context);

    m_titleLabel = &add<Label>(
        L"", TextStyle{.size = 12, .align = TextAlign::Center, .elide = true, .font = TextFont::System}, &Palette::fg1);
    m_titleLabel->setFillsWidth(true);
    m_titleLabel->setFillsHeight(true);

    // The trailing items pushed to the end, then Windows' buttons flush with the window corner.
    m_trailing = &add<Row>();
    m_trailing->setFillsHeight(true);
    m_trailing->add<View>().setFillsWidth(true);
    addBarGroups(*m_trailing, defaultBarLayout.titleTrailing, context);
    m_windowButtons = &m_trailing->add<WindowButtons>();
}

void TitleBar::setTitle(std::wstring title)
{
    m_titleLabel->setText(std::move(title));
}

void TitleBar::layout()
{
    // Maximizing, full screen and DPI changes resize the window, which lays the bar out again.
    if (const Window *host = window())
        m_windowButtons->setVisible(!host->isFullScreen());
    // Both sides as wide, so the title stays centred on the window: at least the trailing buttons;
    // the folder name on the leading side elides in what's left.
    m_trailing->setPreferredSize({});
    const float sideWidth = std::max(240.0f, m_trailing->preferredSize().width);
    m_leading->setPreferredSize({sideWidth, 0});
    m_trailing->setPreferredSize({sideWidth, 0});
    Row::layout();
}

void TitleBar::paint(Canvas &canvas)
{
    canvas.fillRect(bounds(), canvas.palette().bgA3);
    // The title dims with the window, as native titles do.
    const Window *host = window();
    m_titleLabel->setColor(!host || host->isActive() ? &Palette::fg1 : &Palette::fgA2);
}
