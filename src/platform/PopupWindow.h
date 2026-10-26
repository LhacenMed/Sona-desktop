#pragma once

#include <memory>

#include "platform/Window.h"

struct Surface;

// Where a popup opens against the rectangle it belongs to. Each falls back to the other side when
// there's no room on screen (above instead of below, left instead of right), then stays on screen.
enum class PopupPlacement {
    // Its top-left on the rectangle's (a dropdown's list, laid over the field).
    Over,
    // Under the rectangle (a menu from a button).
    Below,
    // To the rectangle's right, level with its top (a submenu; a context menu, from an empty
    // rectangle at the pointer).
    Beside,
};

// A frameless window over its owner for lists and menus, which may reach past the owner as native
// menus do. It draws its Figma surface and shadow itself, and never takes activation: the owner
// keeps the keyboard and passes it every key while the popup is open. It closes on Escape, on a
// click anywhere else, and when the owner moves or loses activation. A popup can own a popup (a
// submenu), which takes the keys in turn.
class PopupWindow : public Window
{
public:
    // Shows `content` at its preferred size, placed against `anchor` (a rectangle of `owner`'s client
    // area in DIPs). Closes any popup already open over `owner`.
    PopupWindow(Window &owner, Rect anchor, PopupPlacement placement, const Surface &surface, std::unique_ptr<View> content);

    // Whether it opened from another popup (a submenu).
    bool belongsToPopup() const { return m_owner->m_frame == WindowFrame::Popup; }
    // Closes this popup and every popup it belongs to, back to the window they opened over.
    void closeChain();

private:
    bool keyPressed(KeyChord chord) override;

    View *m_content;
};
