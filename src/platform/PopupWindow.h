#pragma once

#include <memory>
#include <optional>

#include "core/Signal.h"
#include "platform/Window.h"

class BackdropCapture;
struct Backdrop;
struct Surface;

// Where a popup opens against the rectangle it belongs to. Each but Docked falls back to the other
// side when there's no room on screen (above instead of below, left instead of right), then stays on
// screen.
enum class PopupPlacement {
    // Its top-left on the rectangle's (a dropdown's list, laid over the field).
    Over,
    // Under the rectangle (a menu from a button).
    Below,
    // To the rectangle's right, level with its top (a submenu; a context menu, from an empty
    // rectangle at the pointer).
    Beside,
    // Exactly the rectangle, at its size: a panel docked above the control bar's right end, which it
    // grows out of.
    Docked,
};

// What a popup does with the keys it doesn't use: a menu or list keeps them, as native menus do; a
// panel passes them on to the shortcuts of the window it opened over, so playback keys still work.
enum class PopupKeys { Kept, Passed };

// A frameless window over its owner for menus, lists and panels, which may reach past the owner as
// native menus do. It draws its Figma surface itself, shadow included, frosted with what's behind it
// (the window it opened over, BackdropCapture), and opens with VS Code's menu motion: all of it fades
// in while growing from 97% out of the corner nearest what it opened from. It never takes
// activation: the owner keeps the keyboard and passes it every key while the popup is open. It closes
// on Escape, on a click anywhere else (its shadow included), and when the owner moves or loses
// activation. A popup can own
// a popup (a submenu), which takes the keys in turn.
class PopupWindow : public Window
{
public:
    // Shows `content` at its preferred size (or the rectangle's, Docked), placed against `anchor` (a
    // rectangle of `owner`'s client area in DIPs). Closes any popup already open over `owner`.
    PopupWindow(Window &owner, Rect anchor, PopupPlacement placement, const Surface &surface, std::unique_ptr<View> content,
                PopupKeys keys = PopupKeys::Kept);

    View &content() const { return *m_content; }
    // Whether it opened from another popup (a submenu).
    bool belongsToPopup() const { return m_owner->m_frame == WindowFrame::Popup; }
    // Closes this popup and every popup it belongs to, back to the window they opened over.
    void closeChain();
    // What's behind it to blur, in its content's DIPs; none until the capture has an image.
    std::optional<Backdrop> backdrop() const;

private:
    bool keyPressed(KeyChord chord) override;

    View *m_content;
    PopupKeys m_keys;
    BackdropCapture &m_backdropCapture;
    Subscription m_backdropChanged;
};
