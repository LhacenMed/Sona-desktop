#include "graphics/WindowSurface.h"

#include <algorithm>

#include "graphics/GraphicsDevice.h"
#include "platform/HResult.h"

using Microsoft::WRL::ComPtr;

namespace {

SIZE clientPixels(HWND hwnd)
{
    RECT client{};
    GetClientRect(hwnd, &client);
    return {std::max(client.right, 1L), std::max(client.bottom, 1L)};
}

bool isDeviceLost(HRESULT result)
{
    return result == D2DERR_RECREATE_TARGET || result == DXGI_ERROR_DEVICE_REMOVED || result == DXGI_ERROR_DEVICE_RESET;
}

} // namespace

WindowSurface::WindowSurface(GraphicsDevice &graphics, const Theme &theme, HWND hwnd, float dpi)
    : m_graphics(graphics)
    , m_hwnd(hwnd)
    , m_dpi(dpi)
    , m_canvas(graphics, theme)
{
    create();
    m_deviceReset = graphics.deviceReset.connect([this] {
        releaseTarget();
        m_context.Reset();
        m_visual.Reset();
        m_compositionTarget.Reset();
        m_swapChain.Reset();
        create();
        InvalidateRect(m_hwnd, nullptr, FALSE);
    });
}

void WindowSurface::create()
{
    const SIZE size = clientPixels(m_hwnd);
    DXGI_SWAP_CHAIN_DESC1 description{};
    description.Width = size.cx;
    description.Height = size.cy;
    description.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    description.SampleDesc.Count = 1;
    description.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    description.BufferCount = 2;
    description.Scaling = DXGI_SCALING_STRETCH;
    description.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
    description.AlphaMode = DXGI_ALPHA_MODE_PREMULTIPLIED;
    check(m_graphics.dxgiFactory()->CreateSwapChainForComposition(m_graphics.d3d(), &description, nullptr, &m_swapChain));

    // Topmost: above the window's child windows (the video).
    IDCompositionDevice *composition = m_graphics.composition();
    check(composition->CreateTargetForHwnd(m_hwnd, TRUE, &m_compositionTarget));
    check(composition->CreateVisual(&m_visual));
    check(m_visual->SetContent(m_swapChain.Get()));
    check(m_compositionTarget->SetRoot(m_visual.Get()));
    check(composition->Commit());

    ComPtr<ID2D1DeviceContext> context;
    check(m_graphics.d2d()->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_NONE, &context));
    check(context.As(&m_context));
    // ClearType needs an opaque target; this one has transparent areas.
    m_context->SetTextAntialiasMode(D2D1_TEXT_ANTIALIAS_MODE_GRAYSCALE);
    createTarget();
    m_canvas.bind(m_context.Get());
}

void WindowSurface::createTarget()
{
    ComPtr<IDXGISurface> surface;
    check(m_swapChain->GetBuffer(0, IID_PPV_ARGS(&surface)));
    const D2D1_BITMAP_PROPERTIES1 properties = D2D1::BitmapProperties1(
        D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW,
        D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED), m_dpi, m_dpi);
    check(m_context->CreateBitmapFromDxgiSurface(surface.Get(), &properties, &m_targetBitmap));
    m_context->SetTarget(m_targetBitmap.Get());
    m_context->SetDpi(m_dpi, m_dpi);
}

void WindowSurface::releaseTarget()
{
    if (m_context)
        m_context->SetTarget(nullptr);
    m_targetBitmap.Reset();
}

void WindowSurface::resize()
{
    releaseTarget();
    const SIZE size = clientPixels(m_hwnd);
    const HRESULT result = m_swapChain->ResizeBuffers(0, size.cx, size.cy, DXGI_FORMAT_UNKNOWN, 0);
    if (isDeviceLost(result)) {
        m_graphics.reset();
        return;
    }
    check(result);
    createTarget();
}

void WindowSurface::setDpi(float dpi)
{
    m_dpi = dpi;
    releaseTarget();
    createTarget();
}

void WindowSurface::render(const std::function<void(Canvas &)> &paint)
{
    m_context->BeginDraw();
    m_canvas.beginFrame();
    paint(m_canvas);
    HRESULT result = m_context->EndDraw();
    if (SUCCEEDED(result))
        result = m_swapChain->Present(1, 0);
    if (isDeviceLost(result)) {
        m_graphics.reset();
        return;
    }
    check(result);
}
