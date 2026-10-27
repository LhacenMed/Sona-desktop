#pragma once

#include <functional>
#include <optional>
#include <string>
#include <vector>

#include "commands/Command.h"
#include "platform/Timer.h"
#include "widgets/View.h"

class CommandRegistry;
class GraphicsDevice;
class MenuRow;
class Window;
enum class PopupPlacement;

// A line of a menu: a command, whose shortcut and state come from the registry (and its name and
// icon, unless the line names its own); a choice (a sort order), which runs `run` and is checked
// while it's the one chosen; or a submenu with its own name, two-tone icon and lines.
struct MenuItem
{
    std::optional<CommandId> command;
    std::wstring label;
    std::wstring icon;
    std::vector<MenuItem> submenu;
    std::function<void()> run;
    bool checked = false;
    // Off for a choice the app can't make yet.
    bool enabled = true;
};

// Figma menu (6062:13354, 6065:13616): rows of a two-tone icon, a name, and the shortcut, a check
// for the chosen choice or, for a submenu, a chevron. A menu (each submenu on its own) keeps the
// icons' column only while one of its rows has an icon; without (Sorting Options 856:2215), the names
// start where the icons would.
//
// A menu and its submenus behave as one, as VS Code's and native menus do. The highlight follows the
// pointer at once; the submenu follows the highlight once the pointer rests on a row (submenuDelay),
// opening that row's or closing the open one, also when the pointer leaves for the menu's edge or
// outside. While the pointer heads from the open submenu's row toward the submenu, the rows it
// crosses wait. The open submenu's row stays highlighted while the pointer is in the submenu. A
// click on a submenu row opens it at once and keeps it. The arrow keys move between rows, Enter picks
// one, → opens a submenu with its first row highlighted, ← and Esc close a submenu. Items whose
// feature doesn't exist yet are greyed out.
class Menu : public View
{
public:
    // Opens a menu in a popup over `owner`, placed against `anchor` (client DIPs) and at least
    // `minimumWidth` wide. From the keyboard, its first row starts highlighted.
    static void open(Window &owner, Rect anchor, PopupPlacement placement, std::vector<MenuItem> items,
                     const CommandRegistry &commands, float minimumWidth, bool fromKeyboard = false);

    // It's measured with `graphics` (its owner's), as its popup is sized before it's shown in it.
    // `parent` is the menu it's a submenu of.
    Menu(std::vector<MenuItem> items, const CommandRegistry &commands, const GraphicsDevice &graphics, float minimumWidth,
         bool fromKeyboard, Menu *parent);

    Size preferredSize() const override;
    void layout() override;
    bool keyPressed(KeyChord chord) override;

private:
    friend class MenuRow;

    static void open(Window &owner, Rect anchor, PopupPlacement placement, std::vector<MenuItem> items,
                     const CommandRegistry &commands, float minimumWidth, bool fromKeyboard, Menu *parent);

    bool isEnabled(const MenuItem &item) const;
    // Its own icon, or its command's.
    const std::wstring &iconOf(const MenuItem &item) const;
    // The pointer is over `row`, or over no row (-1), having moved or come or gone.
    void pointerOver(int row);
    // The pointer is in this menu's submenu (or one of its own): its row stays highlighted.
    void submenuEntered();
    // Whether the pointer, since it left the open submenu's row, is heading into the submenu.
    bool isHeadingToSubmenu(POINT pointer) const;
    // Opens or closes the submenu to match the row the pointer rests on.
    void settleSubmenu();
    // The row whose submenu is open, or -1.
    int submenuRow() const;
    // The next enabled row `direction` (1 or -1) rows on from `from`, wrapping; `from` when none.
    int enabledRowAfter(int from, int direction) const;
    // Runs a command row, or opens a submenu row.
    void activate(int index, bool fromKeyboard);
    void openSubmenu(int index, bool fromKeyboard);

    std::vector<MenuItem> m_items;
    const CommandRegistry &m_commands;
    const GraphicsDevice &m_graphics;
    float m_minimumWidth;
    Menu *m_parent;
    // Where the rows' names start.
    float m_nameLeft;
    std::vector<MenuRow *> m_rows;
    // The highlighted row: the pointer's or the keyboard's.
    int m_highlighted = -1;
    // The row under the pointer.
    int m_pointerRow = -1;
    // The row whose submenu is open (while the window has a popup).
    int m_submenuRow = -1;
    // The pointer's last position over a row, and where it left the open submenu's row while it heads
    // into the submenu (screen pixels).
    POINT m_lastPointer{};
    std::optional<POINT> m_aimStart;
    Timer m_settleTimer;
};
