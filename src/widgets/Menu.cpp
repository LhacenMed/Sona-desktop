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
constexpr float iconGap = 10;
constexpr float nameGap = 32;
constexpr TextStyle nameStyle{.size = 14};
constexpr TextStyle shortcutStyle{.size = 14, .align = TextAlign::Trailing};
// Disabled rows are drawn at this opacity.
constexpr float disabledOpacity = 0.4f;
// VS Code's rest on a row before its submenu opens, or the open one closes.
constexpr std::chrono::milliseconds submenuDelay{250};

bool opensSubmenu(const MenuItem &item)
{
    return !item.command && !item.run;
}

} // namespace

class MenuRow : public View
{
public:
    MenuRow(Menu &menu, int index) : m_menu(menu), m_index(index) {}

    // A disabled row takes no part in the pointer, as in native menus: no highlight, no click.
    bool takesMouse() const override { return m_menu.isEnabled(m_menu.m_items[m_index]); }

    void hoverChanged() override { m_menu.pointerOver(isHovered() ? m_index : -1); }

    void mouseMoved(Point) override { m_menu.pointerOver(m_index); }

    void paint(Canvas &canvas) override
    {
        const Palette &palette = canvas.palette();
        const MenuItem &item = m_menu.m_items[m_index];
        // As in native menus, the highlight moves at once rather than fading.
        if (m_index == m_menu.m_highlighted)
            canvas.fillRoundedRect(bounds(), 6, palette.hoverOverlay);

        const bool enabled = m_menu.isEnabled(item);
        if (!enabled)
            canvas.pushOpacity(disabledOpacity);
        const std::wstring &icon = m_menu.iconOf(item);
        if (!icon.empty())
            drawDuoIcon(canvas, icon, {rowInset, (rowHeight - iconSize) / 2});
        const float right = frame().width - rowInset;
        const float nameX = m_menu.m_nameLeft;
        canvas.drawText(name(), {nameX, 0, right - nameX, rowHeight}, nameStyle, palette.fgA2);
        const Point trailing{right - iconSize, (rowHeight - iconSize) / 2};
        if (item.command)
            canvas.drawText(shortcut(), {nameX, 0, right - nameX, rowHeight}, shortcutStyle, palette.fgA3);
        else if (item.checked)
            drawIcon(canvas, L"check", trailing, palette.accentMain);
        else if (opensSubmenu(item))
            drawIcon(canvas, L"chevron-right", trailing, palette.fgA2);
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
        return item.label.empty() && item.command ? m_menu.m_commands.label(*item.command) : item.label;
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
    open(owner, anchor, placement, std::move(items), commands, minimumWidth, fromKeyboard, nullptr);
}

void Menu::open(Window &owner, Rect anchor, PopupPlacement placement, std::vector<MenuItem> items,
                const CommandRegistry &commands, float minimumWidth, bool fromKeyboard, Menu *parent)
{
    owner.openWindow<PopupWindow>(owner, anchor, placement, Surfaces::menu,
                                  std::make_unique<Menu>(std::move(items), commands, owner.graphics(), minimumWidth,
                                                         fromKeyboard, parent));
}

Menu::Menu(std::vector<MenuItem> items, const CommandRegistry &commands, const GraphicsDevice &graphics, float minimumWidth,
           bool fromKeyboard, Menu *parent)
    : m_items(std::move(items))
    , m_commands(commands)
    , m_graphics(graphics)
    , m_minimumWidth(minimumWidth)
    , m_parent(parent)
    , m_nameLeft(std::ranges::any_of(m_items, [this](const MenuItem &item) { return !iconOf(item).empty(); })
                     ? rowInset + iconSize + iconGap
                     : rowInset)
    , m_settleTimer([this] { settleSubmenu(); })
{
    for (int i = 0; i < static_cast<int>(m_items.size()); ++i)
        m_rows.push_back(&add<MenuRow>(*this, i));
    if (fromKeyboard)
        m_highlighted = enabledRowAfter(static_cast<int>(m_rows.size()) - 1, 1);
}

bool Menu::isEnabled(const MenuItem &item) const
{
    if (item.command)
        return m_commands.canRun(*item.command);
    return item.enabled && (item.run || !item.submenu.empty());
}

const std::wstring &Menu::iconOf(const MenuItem &item) const
{
    return item.icon.empty() && item.command ? m_commands.command(*item.command).icon : item.icon;
}

Size Menu::preferredSize() const
{
    float width = m_minimumWidth;
    for (size_t i = 0; i < m_rows.size(); ++i) {
        const MenuRow &row = *m_rows[i];
        const float trailing = m_items[i].command ? m_graphics.measureText(row.shortcut(), shortcutStyle).width : iconSize;
        width = std::max(width, 2 * menuPadding + m_nameLeft + m_graphics.measureText(row.name(), nameStyle).width + nameGap
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
        if (m_highlighted >= 0 && opensSubmenu(m_items[m_highlighted]))
            openSubmenu(m_highlighted, true);
        return true;
    case VK_LEFT:
        if (m_parent)
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
    // The keyboard's row holds until the pointer next moves.
    m_settleTimer.stop();
    m_aimStart.reset();
    invalidate();
    return true;
}

void Menu::pointerOver(int row)
{
    POINT pointer{};
    GetCursorPos(&pointer);
    if (m_parent && row >= 0)
        m_parent->submenuEntered();
    const int openRow = submenuRow();
    if (openRow >= 0 && m_pointerRow == openRow && row != openRow)
        m_aimStart = m_lastPointer;
    const bool rowChanged = row != m_pointerRow;
    m_pointerRow = row;
    if (row >= 0)
        m_lastPointer = pointer;
    if (openRow >= 0 && m_aimStart && isHeadingToSubmenu(pointer)) {
        // The rows it crosses on the way wait until it rests.
        m_settleTimer.start(submenuDelay, false);
        return;
    }
    m_aimStart.reset();
    // Off the rows, the open submenu's row stays highlighted until the submenu closes.
    const int highlighted = row >= 0 ? row : openRow;
    if (highlighted != m_highlighted) {
        m_highlighted = highlighted;
        invalidate();
    }
    if (rowChanged)
        m_settleTimer.start(submenuDelay, false);
}

void Menu::submenuEntered()
{
    m_settleTimer.stop();
    m_aimStart.reset();
    if (m_highlighted != m_submenuRow) {
        m_highlighted = m_submenuRow;
        invalidate();
    }
    if (m_parent)
        m_parent->submenuEntered();
}

bool Menu::isHeadingToSubmenu(POINT pointer) const
{
    const auto &popup = *static_cast<const PopupWindow *>(window()->popup());
    const View &submenu = popup.content();
    const RECT target = popup.toScreenPixels(submenu.mapToWindow(submenu.bounds()));
    if (PtInRect(&target, pointer))
        return true;
    // Within the triangle from where it left the row to the submenu's near edge.
    const POINT start = *m_aimStart;
    const LONG edge = target.left >= start.x ? target.left : target.right;
    const POINT corners[] = {start, {edge, target.top}, {edge, target.bottom}};
    bool clockwise = false;
    bool counterclockwise = false;
    for (int i = 0; i < 3; ++i) {
        const POINT a = corners[i];
        const POINT b = corners[(i + 1) % 3];
        const LONGLONG side = LONGLONG(b.x - a.x) * (pointer.y - a.y) - LONGLONG(b.y - a.y) * (pointer.x - a.x);
        clockwise |= side > 0;
        counterclockwise |= side < 0;
    }
    return !(clockwise && counterclockwise);
}

void Menu::settleSubmenu()
{
    m_aimStart.reset();
    if (submenuRow() >= 0) {
        POINT pointer{};
        GetCursorPos(&pointer);
        for (Window *popup = window()->popup(); popup; popup = popup->popup()) {
            const View &submenu = static_cast<PopupWindow *>(popup)->content();
            const RECT area = popup->toScreenPixels(submenu.mapToWindow(submenu.bounds()));
            if (PtInRect(&area, pointer)) {
                submenuEntered();
                return;
            }
        }
    }
    m_highlighted = m_pointerRow;
    invalidate();
    if (m_pointerRow >= 0 && opensSubmenu(m_items[m_pointerRow]))
        openSubmenu(m_pointerRow, false);
    else
        window()->closePopup();
}

int Menu::submenuRow() const
{
    return window()->hasPopup() ? m_submenuRow : -1;
}

void Menu::activate(int index, bool fromKeyboard)
{
    const MenuItem &item = m_items[index];
    if (!isEnabled(item))
        return;
    if (opensSubmenu(item)) {
        openSubmenu(index, fromKeyboard);
        return;
    }
    const CommandRegistry &commands = m_commands;
    const std::optional<CommandId> command = item.command;
    const std::function<void()> run = item.run;
    // Closing the menus destroys this one, so nothing of it is used after.
    static_cast<PopupWindow *>(window())->closeChain();
    if (command)
        commands.run(*command);
    else
        run();
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
    m_settleTimer.stop();
    m_aimStart.reset();
    const MenuItem &item = m_items[index];
    // A click on the open submenu's row keeps it.
    if (!isEnabled(item) || index == submenuRow())
        return;
    // Beside the row, its first row level with this one.
    const Rect row = m_rows[index]->mapToWindow(m_rows[index]->bounds());
    const Rect anchor{row.x - menuPadding, row.y - menuPadding, row.width + 2 * menuPadding, row.height + 2 * menuPadding};
    m_highlighted = index;
    m_submenuRow = index;
    invalidate();
    open(*window(), anchor, PopupPlacement::Beside, item.submenu, m_commands, 0, fromKeyboard, this);
}
