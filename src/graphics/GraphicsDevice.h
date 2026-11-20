#pragma once

#include <d2d1_3.h>
#include <d3d11.h>
#include <dcomp.h>
#include <dwrite_3.h>
#include <dxgi1_2.h>
#include <wrl/client.h>

#include <future>
#include <optional>
#include <string_view>
#include <utility>
#include <vector>

#include "core/Geometry.h"
#include "core/Signal.h"
#include "graphics/TextStyle.h"

// The GPU device and factories every window shares: Direct3D 11 for swap chains, Direct2D for
// drawing, DirectComposition to put the drawing on screen, and DirectWrite with the app font.
//
// Creating the Direct3D device loads the display driver, the slowest part of starting the app, so
// it's created on a thread of its own from the start while the first window and its views are set
// up (they need only DirectWrite); the first use of the device waits for it.
class GraphicsDevice
{
public:
    GraphicsDevice();
    ~GraphicsDevice();
    GraphicsDevice(const GraphicsDevice &) = delete;
    GraphicsDevice &operator=(const GraphicsDevice &) = delete;

    ID3D11Device *d3d() const { return device().d3d.Get(); }
    IDXGIFactory2 *dxgiFactory() const { return device().dxgiFactory.Get(); }
    ID2D1Device *d2d() const { return device().d2d.Get(); }
    IDCompositionDevice *composition() const { return device().composition.Get(); }

    // Recreates the device after the GPU was reset or removed (driver update, adapter change).
    void reset();
    // Emitted after reset(), so windows rebuild what they made on the old device.
    Signal<> deviceReset;

    Microsoft::WRL::ComPtr<IDWriteTextLayout> layoutText(std::wstring_view text, const TextStyle &style, Size box) const;
    // The size `text` takes, wrapping (when the style wraps) at `maxWidth`.
    Size measureText(std::wstring_view text, const TextStyle &style, float maxWidth = 100000) const;
    // For a line of leading-aligned text: where the caret before character `index` sits, and the
    // caret position nearest to `x`.
    float caretX(std::wstring_view text, const TextStyle &style, size_t index) const;
    size_t caretIndexAt(std::wstring_view text, const TextStyle &style, float x) const;

private:
    struct Device
    {
        Microsoft::WRL::ComPtr<ID3D11Device> d3d;
        Microsoft::WRL::ComPtr<IDXGIFactory2> dxgiFactory;
        Microsoft::WRL::ComPtr<ID2D1Device> d2d;
        Microsoft::WRL::ComPtr<IDCompositionDevice> composition;
    };

    // The device, once the Direct3D one being created is there.
    const Device &device() const;
    Device makeDevice(Microsoft::WRL::ComPtr<ID3D11Device> d3d) const;
    IDWriteTextFormat *textFormat(const TextStyle &style) const;

    Microsoft::WRL::ComPtr<ID2D1Factory1> m_d2dFactory;
    Microsoft::WRL::ComPtr<IDWriteFactory5> m_dwrite;
    Microsoft::WRL::ComPtr<IDWriteInMemoryFontFileLoader> m_fontLoader;
    Microsoft::WRL::ComPtr<IDWriteFontCollection1> m_fonts;
    const wchar_t *m_systemFontFamily = nullptr;
    const wchar_t *m_iconFontFamily = nullptr;
    mutable std::vector<std::pair<TextStyle, Microsoft::WRL::ComPtr<IDWriteTextFormat>>> m_textFormats;

    mutable std::future<Microsoft::WRL::ComPtr<ID3D11Device>> m_creatingD3d;
    mutable std::optional<Device> m_device;
};
