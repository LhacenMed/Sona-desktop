#pragma once

#include <d2d1_3.h>
#include <dcomp.h>
#include <dxgi1_2.h>
#include <windows.h>
#include <wrl/client.h>

#include <functional>

#include "core/Signal.h"
#include "graphics/Canvas.h"

class GraphicsDevice;
class Theme;

// What a window shows: a transparent-capable swap chain that DirectComposition places above the
// window's child windows, so the video child shows through wherever the views leave pixels clear.
class WindowSurface
{
public:
    WindowSurface(GraphicsDevice &graphics, const Theme &theme, HWND hwnd, float dpi);

    // Matches the swap chain to the window's client size.
    void resize();
    void setDpi(float dpi);
    void render(const std::function<void(Canvas &)> &paint);

private:
    void create();
    void createTarget();
    void releaseTarget();

    GraphicsDevice &m_graphics;
    HWND m_hwnd;
    float m_dpi;
    Microsoft::WRL::ComPtr<IDXGISwapChain1> m_swapChain;
    Microsoft::WRL::ComPtr<IDCompositionTarget> m_compositionTarget;
    Microsoft::WRL::ComPtr<IDCompositionVisual> m_visual;
    Microsoft::WRL::ComPtr<ID2D1DeviceContext5> m_context;
    Microsoft::WRL::ComPtr<ID2D1Bitmap1> m_targetBitmap;
    Canvas m_canvas;
    Subscription m_deviceReset;
};
