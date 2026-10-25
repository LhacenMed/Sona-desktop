#pragma once

#include <windows.h>
#include <shellapi.h>

#include <memory>
#include <string>

#include "core/Geometry.h"
#include "core/Signal.h"
#include "style/Theme.h"

class GraphicsDevice;
class View;
class WindowSurface;

// A top-level window whose content reaches into the title bar while keeping the native frame: resize
// borders, Aero Snap, the system menu, window animations, the DWM shadow and rounded corners, and
// (setNativeCaption) Windows' own caption buttons with Snap Layouts. Its content is a tree of Views
// drawn with Direct2D; views tell the frame which parts act as title bar (HitRole).
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

    // Shows the window after drawing its first frame, so it never appears empty.
    void show(bool maximized);
    void toggleFullScreen();
    void close();
    // Schedules a repaint.
    void invalidate();

protected:
    Window(GraphicsDevice &graphics, Theme &theme);

    // Creates the window centred on the owner's monitor (or the primary one). Sizes are in DIPs.
    void create(const std::wstring &title, Size size, Size minimumSize, bool resizable, HWND owner);
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
    virtual bool keyPressed(UINT /*key*/, bool /*shift*/, bool /*control*/) { return false; }
    virtual void filesDropped(HDROP) {}
    // Handles a message before the window does; returns whether it was handled.
    virtual bool messageReceived(UINT, WPARAM, LPARAM) { return false; }
    virtual void closeRequested();
    virtual void destroyed() {}

private:
    static LRESULT CALLBACK windowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
    LRESULT handleMessage(UINT message, WPARAM wParam, LPARAM lParam);
    LRESULT hitTest(LPARAM screenPoint);
    Point toDips(POINT clientPoint) const;
    // Where the content (the root view) starts below the client area's top, in DIPs.
    float contentTop() const;
    // The view under a point of the client area, in DIPs.
    View *viewAt(Point point) const;
    int frameThickness() const;
    void measureCaptionButtons();
    void applyFrameShape();
    void updateFrameMargins();
    void applyFrameTheme();
    void layoutRoot();
    void render();
    void pointerMoved(Point point);
    void pointerPressed(Point point);
    void pointerReleased(Point point);
    void cancelPress();
    void setHovered(View *view);

    GraphicsDevice &m_graphics;
    Theme &m_theme;
    HWND m_hwnd = nullptr;
    float m_dpi = USER_DEFAULT_SCREEN_DPI;
    bool m_resizable = false;
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
    View *m_hovered = nullptr;
    View *m_pressed = nullptr;
    Subscription m_themeSubscription;
};
