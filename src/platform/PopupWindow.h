#pragma once

#include <memory>

#include "platform/Window.h"

// A frameless window over its owner for lists and menus, which may reach past the owner as native
// menus do. It draws Figma's list surface and shadow itself, and never takes activation: the owner
// keeps the keyboard and passes it every key while the popup is open. It closes on Escape, on a
// click anywhere else, and when the owner moves or loses activation.
class PopupWindow : public Window
{
public:
    // Shows `content` at its preferred size with its top-left at `anchor`, a point of `owner`'s
    // client area in DIPs, moved as needed to stay on the monitor. Closes any popup already open.
    PopupWindow(Window &owner, Point anchor, std::unique_ptr<View> content);

private:
    bool keyPressed(KeyChord chord) override;

    View *m_content;
};
