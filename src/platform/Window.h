#pragma once

#include <windows.h>
#include <shellapi.h>

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "core/Geometry.h"
#include "core/Signal.h"
#include "platform/KeyChord.h"
#include "style/Theme.h"

class BackdropCapture;
class GraphicsDevice;
class Overlay;
class PopupWindow;
class View;
class WindowSurface;

// How a window is framed: the main window (resize borders, snapping, maximizing), a fixed-size
// window (modal over its owner, like onboarding and dialogs), or a frameless popup over its owner
// (lists and menus; see PopupWindow).
enum class WindowFrame { Resizable, Fixed, Popup };

// A top-level window whose content reaches into the title bar while keeping the native frame: resize
// borders, Aero Snap, the system menu, window animations, the DWM shadow and rounded corners. Its
// content is a tree of Views drawn with Direct2D, under an Overlay for callouts. The
// title bar is views too, which tell the frame what they are (HitRole): the caption, the system
// menu, and the minimize, maximize and close buttons, drawn at Windows' own size (captionButtonSize)
// and answering as Windows' own, so Snap Layouts work.
//
// Input goes to views: the mouse to the view under it, keys to the focused view and then to the
// window's shortcuts (keyPressed). An open popup takes the keys first. An open popup closes on
// Escape or on a click anywhere else, and that click does nothing more; until then, the content it
// floats over takes no part in the mouse (no hover, pointer shape, tooltip or scroll).
class Window
{
public:
    virtual ~Window();
    Window(const Window &) = delete;
    Window &operator=(const Window &) = delete;

    HWND hwnd() const { return m_hwnd; }
    GraphicsDevice &graphics() const { return m_graphics; }
    bool isActive() const { return m_active; }
    bool isMaximized() const;
    bool isFullScreen() const { return m_fullScreen; }
    // One of Windows' caption buttons (minimize, maximize, close) in DIPs, measured for this monitor
    // and window state: a maximized window's are shorter.
    Size captionButtonSize() const;
    // The title bar height that matches the caption buttons.
    float captionHeight() const { return captionButtonSize().height; }
    Overlay &overlay() const { return *m_overlay; }

    // Where the pointer is over the client area, in DIPs (as View::mapFromWindow takes it); empty
    // when it's elsewhere.
    std::optional<Point> pointerPosition() const { return m_pointer; }
    // Whether a press is held on a view, and whether a popup is open over this window.
    bool isPressing() const { return m_pressed != nullptr; }
    bool hasPopup() const { return m_popup != nullptr; }
    Window *popup() const { return m_popup; }
    // Closes the popup open over this window, and any popup it owns.
    void closePopup();
    // The window this one opened over, through any popups: itself, unless it's a popup.
    Window &rootWindow();
    // What this window looks like, captured for the frosted popups over it to blur: kept warm from
    // just after it first shows (a resizable window) or its first popup, and live while one is open.
    BackdropCapture &backdropCapture();
    // Hides the pointer over the client area until it next moves.
    void hidePointer();

    // Shows the window after drawing its first frame, so it never appears empty.
    void show(bool maximized);
    void toggleFullScreen();
    void close();
    // Schedules a repaint.
    void invalidate();
    // Gives the keyboard to `view`, or to no view.
    void setFocus(View *view);
    // A rectangle of the client area in DIPs (as View::mapToWindow gives it), on screen in pixels.
    RECT toScreenPixels(const Rect &rect) const;

    // Runs `task` once the message being handled is done, as Windows delivers a menu's command after
    // the menu has closed. Anything that runs its own message loop (a file dialog) starts this way, so
    // that loop never runs inside a click or key handler whose views and windows it may close. The
    // task is dropped if the window is destroyed first.
    void runLater(std::function<void()> task);

    // Opens a window this one owns (a dialog, a popup); it lives until it's closed.
    template <typename T, typename... Args>
    T &openWindow(Args &&...args)
    {
        auto window = std::make_unique<T>(std::forward<Args>(args)...);
        T &opened = *window;
        m_ownedWindows.push_back(std::move(window));
        return opened;
    }

protected:
    Window(GraphicsDevice &graphics, Theme &theme);

    // Creates the window centred on the owner's monitor (or the primary one). Sizes are in DIPs. A
    // fixed window with an owner is modal: the owner is disabled until it closes.
    void create(const std::wstring &title, Size size, Size minimumSize, WindowFrame frame, HWND owner);
    // Creates a popup over `owner` (see PopupWindow), covering `screenPixels`.
    void createPopup(HWND owner, const RECT &screenPixels);
    void setRoot(std::unique_ptr<View> root);
    Theme &theme() const { return m_theme; }
    // A rectangle of the root view in client pixels, for child windows.
    RECT contentToClientPixels(const Rect &rect) const;
    float scale() const { return m_dpi / USER_DEFAULT_SCREEN_DPI; }

    // Hooks for the concrete windows.
    virtual void layoutChanged() {}
    // A key that no view handled.
    virtual bool keyPressed(KeyChord) { return false; }
    // The pointer moved, left, or a press or popup that held it ended (pointerPosition).
    virtual void pointerChanged() {}
    // A right click or the menu key over the content, at a point of the client area in DIPs.
    virtual void contextMenuRequested(Point) {}
    virtual void filesDropped(HDROP) {}
    virtual void closeRequested();
    virtual void destroyed() {}

private:
    friend class PopupWindow;
    friend class View;

    static LRESULT CALLBACK windowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
    void createWindow(const std::wstring &title, DWORD style, DWORD extendedStyle, const RECT &bounds, HWND owner);
    LRESULT handleMessage(UINT message, WPARAM wParam, LPARAM lParam);
    LRESULT hitTest(LPARAM screenPoint);
    // Mouse messages over the caption buttons, which Windows sends as non-client ones.
    bool captionButtonMessage(UINT message, WPARAM hit, LPARAM screenPoint);
    Point toDips(POINT clientPoint) const;
    Point screenToDips(LPARAM screenPoint) const;
    // Where the content (the root view) starts below the client area's top, in DIPs.
    float contentTop() const;
    // The view under a point of the client area, in DIPs: the overlay's first, then the content's.
    View *viewAt(Point point) const;
    int frameThickness() const;
    void measureCaptionButtons();
    void applyFrameShape();
    void updateFrameMargins();
    void applyFrameTheme();
    void layoutRoot();
    void render();
    bool handleKey(KeyChord chord);
    void pointerMoved(Point point);
    void pointerLeft();
    // Shows the pointer the view under it asks for, or none while it's hidden.
    void applyCursor();
    void pointerPressed(Point point);
    // Whether a popup is open over this window's content, which then takes no part in the mouse.
    bool isUnderPopup() const;
    void pointerReleased(Point point);
    void wheelScrolled(Point point, float notches);
    // The deepest visible view under `point` (in `view`'s coordinates), whether or not it takes the mouse.
    static View *deepestViewAt(View &view, Point point);
    void cancelPress();
    void setHovered(View *view);
    // Puts the IME's composition and candidate windows at the focused view's caret.
    void placeImeWindows();
    // Lets go of the views of a subtree that's being removed.
    void viewRemoved(View &removed);

    GraphicsDevice &m_graphics;
    Theme &m_theme;
    HWND m_hwnd = nullptr;
    float m_dpi = USER_DEFAULT_SCREEN_DPI;
    WindowFrame m_frame = WindowFrame::Resizable;
    Size m_minimumSize;
    // Windows' three caption buttons together, in pixels from the window's top edge, restored, and
    // the bottom of a maximized window's (whose top edge lies off screen).
    SIZE m_captionButtons{};
    LONG m_maximizedCaptionBottom = 0;
    int m_frameMarginTop = -1;
    bool m_active = false;
    bool m_fullScreen = false;
    WINDOWPLACEMENT m_placementBeforeFullScreen{sizeof(WINDOWPLACEMENT)};
    bool m_trackingMouse = false;
    bool m_trackingNonClientMouse = false;
    std::optional<Point> m_pointer;
    bool m_pointerHidden = false;
    std::unique_ptr<WindowSurface> m_surface;
    std::unique_ptr<View> m_root;
    std::unique_ptr<Overlay> m_overlay;
    View *m_hovered = nullptr;
    View *m_pressed = nullptr;
    View *m_focused = nullptr;
    // The window that owns this one, when it's one of ours, and whether this one is modal over it.
    Window *m_owner = nullptr;
    bool m_modal = false;
    // Before the owned windows, so the popups subscribed to it go first.
    std::unique_ptr<BackdropCapture> m_backdropCapture;
    std::vector<std::unique_ptr<Window>> m_ownedWindows;
    // The popup open over this window, which takes its keys.
    Window *m_popup = nullptr;
    Subscription m_themeSubscription;
};
