#include "widgets/Menu.h"

#include <algorithm>
#include <memory>

#include "commands/CommandRegistry.h"
#include "graphics/Canvas.h"
#include "graphics/GraphicsDevice.h"
#include "platform/PopupWindow.h"
#include "style/Surface.h"
#include "style/Theme.h"
#include "widgets/Icon.h"

namespace {

// Figma 6062:13354: 4 around the rows, 48-tall rows with 14 inside each side, the icon then 10 to
// the name; at least 32 between the name and the shortcut.
constexpr float menuPadding = 4;
constexpr float rowHeight = 48;
constexpr float rowInset = 14;
constexpr float nameX = rowInset + iconSize + 10;
constexpr float nameGap = 32;
constexpr TextStyle nameStyle{.size = 14};
constexpr TextStyle shortcutStyle{.size = 14, .align = TextAlign::Trailing};
// Disabled rows are drawn at this opacity.
constexpr float disabledOpacity = 0.4f;

} // namespace

class MenuRow : public View
{
public:
    MenuRow(Menu &menu, int index) : m_menu(menu), m_index(index) {}

    // A disabled row takes no part in the pointer, as in native menus: no highlight, no click.
    bool takesMouse() const override { return m_menu.isEnabled(m_menu.m_items[m_index]); }

    void hoverChanged() override
    {
        if (isHovered())
            m_menu.rowHovered(m_index);
        else
            m_menu.rowLeft(m_index);
        invalidate();
    }

    void paint(Canvas &canvas) override
    {
        const Palette &palette = canvas.palette();
        const MenuItem &item = m_menu.m_items[m_index];
        canvas.fillRoundedRect(bounds(), 6, palette.bgA2.faded(m_menu.isLit(m_index) ? 1 : hoverAmount()));

        const bool enabled = m_menu.isEnabled(item);
        if (!enabled)
            canvas.pushOpacity(disabledOpacity);
        const std::wstring &icon = item.command ? m_menu.m_commands.command(*item.command).icon : item.icon;
        if (!icon.empty())
            drawDuoIcon(canvas, icon, {rowInset, (rowHeight - iconSize) / 2});
        const float right = frame().width - rowInset;
        canvas.drawText(name(), {nameX, 0, right - nameX, rowHeight}, nameStyle, palette.fgA2);
        if (!item.command)
            drawIcon(canvas, L"chevron-right", {right - iconSize, (rowHeight - iconSize) / 2}, palette.fgA2);
        else
            canvas.drawText(shortcut(), {nameX, 0, right - nameX, rowHeight}, shortcutStyle, palette.fgA3);
        if (!enabled)
            canvas.popOpacity();
    }

    void mouseReleased(Point, bool inside) override
    {
        if (inside)
            m_menu.activate(m_index, false);
    }

    std::wstring name() const
    {
        const MenuItem &item = m_menu.m_items[m_index];
        return item.command ? m_menu.m_commands.label(*item.command) : item.label;
    }

    std::wstring shortcut() const
    {
        const MenuItem &item = m_menu.m_items[m_index];
        const std::vector<KeyChord> &shortcuts = m_menu.m_commands.command(*item.command).shortcuts;
        return shortcuts.empty() ? std::wstring() : shortcutText(shortcuts.front());
    }

private:
    Menu &m_menu;
    int m_index;
};

void Menu::open(Window &owner, Rect anchor, PopupPlacement placement, std::vector<MenuItem> items,
                const CommandRegistry &commands, float minimumWidth, bool fromKeyboard)
{
    owner.openWindow<PopupWindow>(owner, anchor, placement, Surfaces::menu,
                                  std::make_unique<Menu>(std::move(items), commands, owner.graphics(), minimumWidth,
                                                         fromKeyboard ? 0 : -1));
}

Menu::Menu(std::vector<MenuItem> items, const CommandRegistry &commands, const GraphicsDevice &graphics, float minimumWidth,
           int highlighted)
    : m_items(std::move(items))
    , m_commands(commands)
    , m_graphics(graphics)
    , m_minimumWidth(minimumWidth)
    , m_highlighted(highlighted)
    , m_submenuTimer([this] {
        if (m_highlighted >= 0 && !m_items[m_highlighted].command)
            openSubmenu(m_highlighted, false);
        else
            window()->closePopup();
    })
{
    for (int i = 0; i < static_cast<int>(m_items.size()); ++i)
        m_rows.push_back(&add<MenuRow>(*this, i));
}

bool Menu::isEnabled(const MenuItem &item) const
{
    return item.command ? m_commands.canRun(*item.command) : !item.submenu.empty();
}

bool Menu::isLit(int index) const
{
    return index == m_highlighted && std::ranges::none_of(m_rows, [](const MenuRow *row) { return row->isHovered(); });
}

Size Menu::preferredSize() const
{
    float width = m_minimumWidth;
    for (size_t i = 0; i < m_rows.size(); ++i) {
        const MenuRow &row = *m_rows[i];
        const float trailing = m_items[i].command ? m_graphics.measureText(row.shortcut(), shortcutStyle).width : iconSize;
        width = std::max(width, 2 * menuPadding + nameX + m_graphics.measureText(row.name(), nameStyle).width + nameGap
                                    + trailing + rowInset);
    }
    return {width, 2 * menuPadding + rowHeight * static_cast<float>(m_rows.size())};
}

void Menu::layout()
{
    for (size_t i = 0; i < m_rows.size(); ++i)
        m_rows[i]->setFrame({menuPadding, menuPadding + rowHeight * i, frame().width - 2 * menuPadding, rowHeight});
}

bool Menu::keyPressed(KeyChord chord)
{
    const int count = static_cast<int>(m_rows.size());
    switch (chord.key) {
    case VK_UP:
        m_highlighted = enabledRowAfter(m_highlighted < 0 ? 0 : m_highlighted, -1);
        break;
    case VK_DOWN:
        m_highlighted = enabledRowAfter(m_highlighted < 0 ? count - 1 : m_highlighted, 1);
        break;
    case VK_HOME:
        m_highlighted = enabledRowAfter(count - 1, 1);
        break;
    case VK_END:
        m_highlighted = enabledRowAfter(0, -1);
        break;
    case VK_RIGHT:
        if (m_highlighted >= 0 && !m_items[m_highlighted].command)
            openSubmenu(m_highlighted, true);
        return true;
    case VK_LEFT:
        if (isSubmenu())
            window()->close();
        return true;
    case VK_RETURN:
    case VK_SPACE:
        if (m_highlighted >= 0)
            activate(m_highlighted, true);
        return true;
    default:
        return false;
    }
    invalidate();
    return true;
}

void Menu::rowHovered(int index)
{
    m_highlighted = index;
    // Windows' menu delay before a submenu opens, or the open one closes.
    UINT delay = 400;
    SystemParametersInfoW(SPI_GETMENUSHOWDELAY, 0, &delay, 0);
    m_submenuTimer.start(std::chrono::milliseconds(delay), false);
}

void Menu::activate(int index, bool fromKeyboard)
{
    const MenuItem &item = m_items[index];
    if (!isEnabled(item))
        return;
    if (!item.command) {
        openSubmenu(index, fromKeyboard);
        return;
    }
    const CommandId command = *item.command;
    const CommandRegistry &commands = m_commands;
    // Closing the menus destroys this one, so nothing of it is used after.
    static_cast<PopupWindow *>(window())->closeChain();
    commands.run(command);
}

void Menu::rowLeft(int index)
{
    if (index != m_highlighted || (index == m_submenuRow && window()->hasPopup()))
        return;
    m_highlighted = -1;
    m_submenuTimer.stop();
}

int Menu::enabledRowAfter(int from, int direction) const
{
    const int count = static_cast<int>(m_rows.size());
    for (int step = 1; step <= count; ++step) {
        const int index = ((from + direction * step) % count + count) % count;
        if (isEnabled(m_items[index]))
            return index;
    }
    return from;
}

void Menu::openSubmenu(int index, bool fromKeyboard)
{
    m_submenuTimer.stop();
    const MenuItem &item = m_items[index];
    if (!isEnabled(item))
        return;
    // Beside the row, its first row level with this one.
    const Rect row = m_rows[index]->mapToWindow(m_rows[index]->bounds());
    const Rect anchor{row.x - menuPadding, row.y - menuPadding, row.width + 2 * menuPadding, row.height + 2 * menuPadding};
    m_submenuRow = index;
    open(*window(), anchor, PopupPlacement::Beside, item.submenu, m_commands, 0, fromKeyboard);
}

bool Menu::isSubmenu() const
{
    return static_cast<const PopupWindow *>(window())->belongsToPopup();
}
