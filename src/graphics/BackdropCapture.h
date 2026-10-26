#pragma once

#include <windows.h>
#include <d3d11.h>
#include <wrl/client.h>

#include <memory>

#include "core/Signal.h"

class GraphicsDevice;

// A window as Windows composes it, its child windows included (the video), for the frosted popups
// over it to blur what's behind them (Windows.Graphics.Capture). It's kept warm: captured a couple of
// times a second until a popup opens, then live, so a popup has its backdrop from its first frame.
// Where Windows can't slow a capture down, it captures only while live; where it can't capture
// without outlining the window (before Windows 11), it captures nothing and popups are opaque.
class BackdropCapture
{
public:
    BackdropCapture(GraphicsDevice &graphics, HWND window);
    ~BackdropCapture();
    BackdropCapture(const BackdropCapture &) = delete;
    BackdropCapture &operator=(const BackdropCapture &) = delete;

    // Live while a popup shows the backdrop: every change of the window arrives.
    void setLive(bool live);

    // The latest image of the window, null until there is one; its top-left is at `origin()` on
    // screen, in pixels.
    ID3D11Texture2D *image() const { return m_image.Get(); }
    POINT origin() const { return m_origin; }

    // A new image arrived.
    Signal<> changed;

private:
    struct Session;

    void start();
    void applyRate();
    void frameArrived();

    GraphicsDevice &m_graphics;
    HWND m_window;
    bool m_live = false;
    std::unique_ptr<Session> m_session;
    Microsoft::WRL::ComPtr<ID3D11Texture2D> m_image;
    POINT m_origin{};
    Subscription m_deviceReset;
};
