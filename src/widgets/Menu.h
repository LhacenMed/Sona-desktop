#pragma once

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

// A line of a menu: a command, whose name, icon, shortcut and state come from the registry, or a
// submenu with its own name, two-tone icon and lines.
struct MenuItem
{
    std::optional<CommandId> command;
    std::wstring label;
    std::wstring icon;
    std::vector<MenuItem> submenu;
};

// Figma menu (6062:13354, 6065:13616): rows of a two-tone icon, a name, and the shortcut or, for a
// submenu, a chevron. A submenu opens once the pointer rests on its row, at once on a click or →.
// The arrow keys move between rows, Enter picks one, ← and Esc close a submenu. Items whose feature
// doesn't exist yet are greyed out.
class Menu : public View
{
public:
    // Opens a menu in a popup over `owner`, placed against `anchor` (client DIPs) and at least
    // `minimumWidth` wide. From the keyboard, its first row starts highlighted.
    static void open(Window &owner, Rect anchor, PopupPlacement placement, std::vector<MenuItem> items,
                     const CommandRegistry &commands, float minimumWidth, bool fromKeyboard = false);

    // It's measured with `graphics` (its owner's), as its popup is sized before it's shown in it.
    Menu(std::vector<MenuItem> items, const CommandRegistry &commands, const GraphicsDevice &graphics, float minimumWidth,
         int highlighted);

    Size preferredSize() const override;
    void layout() override;
    bool keyPressed(KeyChord chord) override;

private:
    friend class MenuRow;

    bool isEnabled(const MenuItem &item) const;
    // Highlighted rows: the hovered one, or the keyboard's while the pointer is on none.
    bool isLit(int index) const;
    void rowHovered(int index);
    // The pointer left the row: it's no longer highlighted, unless its submenu is open.
    void rowLeft(int index);
    // The next enabled row `direction` (1 or -1) rows on from `from`, wrapping; `from` when none.
    int enabledRowAfter(int from, int direction) const;
    // Runs a command row, or opens a submenu row.
    void activate(int index, bool fromKeyboard);
    void openSubmenu(int index, bool fromKeyboard);
    // Whether this menu is a submenu, so ← closes it.
    bool isSubmenu() const;

    std::vector<MenuItem> m_items;
    const CommandRegistry &m_commands;
    const GraphicsDevice &m_graphics;
    float m_minimumWidth;
    std::vector<MenuRow *> m_rows;
    int m_highlighted;
    // The row whose submenu is open (while the window has a popup).
    int m_submenuRow = -1;
    Timer m_submenuTimer;
};
