#pragma once

#include <windows.h>
#include <shellapi.h>

#include <memory>
#include <string>
#include <vector>

#include "core/Geometry.h"
#include "core/Signal.h"
#include "platform/KeyChord.h"
#include "style/Theme.h"

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
// borders, Aero Snap, the system menu, window animations, the DWM shadow and rounded corners, and
// (setNativeCaption) Windows' own caption buttons with Snap Layouts. Its content is a tree of Views
// drawn with Direct2D, under an Overlay for popovers and tooltips; views tell the frame which parts
// act as title bar (HitRole).
//
// Input goes to views: the mouse to the view under it, keys to the focused view and then to the
// window's shortcuts (keyPressed). An open popup takes the keys first. An open popup or popover
// closes on Escape or on a click anywhere else, and that click does nothing more.
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
    // Where Windows draws the caption buttons, in the root view's DIPs; empty when it draws none.
    Rect captionButtonBounds() const;
    // The title bar height that lines up with the caption buttons, in DIPs. It changes with the
    // window state (a maximized window's buttons are shorter), DPI and system metrics.
    float captionHeight() const;
    Overlay &overlay() const { return *m_overlay; }

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
    // Gives the top of the content (captionHeight) to the native caption: Windows paints it in the
    // `background` colour and draws its minimize, maximize and close buttons there, and the views over
    // it leave their background transparent.
    void setNativeCaption(Color Palette::*background);
    Theme &theme() const { return m_theme; }
    // A rectangle of the root view in client pixels, for child windows.
    RECT contentToClientPixels(const Rect &rect) const;
    float scale() const { return m_dpi / USER_DEFAULT_SCREEN_DPI; }

    // Hooks for the concrete windows.
    virtual void layoutChanged() {}
    // A key that no view handled.
    virtual bool keyPressed(KeyChord) { return false; }
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
    Point toDips(POINT clientPoint) const;
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
    void pointerPressed(Point point);
    void pointerReleased(Point point);
    void wheelScrolled(Point point, float notches);
    // The deepest visible view under `point` (in `view`'s coordinates), whether or not it takes the mouse.
    static View *deepestViewAt(View &view, Point point);
    void cancelPress();
    void setHovered(View *view);
    void closePopup();
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
    bool m_nativeCaption = false;
    // Windows' caption buttons together, in pixels from the window's top edge (measureCaptionButtons).
    SIZE m_captionButtons{};
    int m_frameMarginTop = -1;
    Color Palette::*m_captionBackground = nullptr;
    bool m_active = false;
    bool m_fullScreen = false;
    WINDOWPLACEMENT m_placementBeforeFullScreen{sizeof(WINDOWPLACEMENT)};
    bool m_trackingMouse = false;
    std::unique_ptr<WindowSurface> m_surface;
    std::unique_ptr<View> m_root;
    std::unique_ptr<Overlay> m_overlay;
    View *m_hovered = nullptr;
    View *m_pressed = nullptr;
    View *m_focused = nullptr;
    // The window that owns this one, when it's one of ours, and whether this one is modal over it.
    Window *m_owner = nullptr;
    bool m_modal = false;
    std::vector<std::unique_ptr<Window>> m_ownedWindows;
    // The popup open over this window, which takes its keys.
    Window *m_popup = nullptr;
    Subscription m_themeSubscription;
};
