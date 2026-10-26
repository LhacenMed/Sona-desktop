#include "graphics/GraphicsDevice.h"

#include "platform/HResult.h"
#include "platform/Resources.h"
#include "style/Typography.h"

using Microsoft::WRL::ComPtr;

GraphicsDevice::GraphicsDevice()
{
    check(D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, m_d2dFactory.GetAddressOf()));
    check(DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory5),
                              reinterpret_cast<IUnknown **>(m_dwrite.GetAddressOf())));

    // The bundled fonts, read straight from the executable's resources.
    check(m_dwrite->CreateInMemoryFontFileLoader(&m_fontLoader));
    check(m_dwrite->RegisterFontFileLoader(m_fontLoader.Get()));
    ComPtr<IDWriteFontSetBuilder1> builder;
    check(m_dwrite->CreateFontSetBuilder(&builder));
    for (const wchar_t *font : Typography::fontAssets) {
        const auto data = asset(font);
        ComPtr<IDWriteFontFile> file;
        check(m_fontLoader->CreateInMemoryFontFileReference(m_dwrite.Get(), data.data(), static_cast<UINT32>(data.size()),
                                                            nullptr, &file));
        check(builder->AddFontFile(file.Get()));
    }
    ComPtr<IDWriteFontSet> fontSet;
    check(builder->CreateFontSet(&fontSet));
    check(m_dwrite->CreateFontCollectionFromFontSet(fontSet.Get(), &m_fonts));

    // The first installed family of each system font list.
    ComPtr<IDWriteFontCollection> systemFonts;
    check(m_dwrite->GetSystemFontCollection(&systemFonts));
    const auto installed = [&](const auto &families) -> const wchar_t * {
        for (const wchar_t *family : families) {
            UINT32 index = 0;
            BOOL exists = FALSE;
            if (SUCCEEDED(systemFonts->FindFamilyName(family, &index, &exists)) && exists)
                return family;
        }
        return families.back();
    };
    m_systemFontFamily = installed(Typography::systemFontFamilies);
    m_iconFontFamily = installed(Typography::iconFontFamilies);

    createDevice();
}

GraphicsDevice::~GraphicsDevice()
{
    m_textFormats.clear();
    m_fonts.Reset();
    m_dwrite->UnregisterFontFileLoader(m_fontLoader.Get());
}

void GraphicsDevice::createDevice()
{
    // BGRA support is what lets Direct2D draw into Direct3D surfaces.
    constexpr UINT flags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
    if (FAILED(D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, flags, nullptr, 0, D3D11_SDK_VERSION,
                                 &m_d3d, nullptr, nullptr))) {
        // No usable GPU (e.g. the basic display driver): fall back to the software rasterizer.
        check(D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, flags, nullptr, 0, D3D11_SDK_VERSION, &m_d3d,
                                nullptr, nullptr));
    }

    ComPtr<IDXGIDevice1> dxgiDevice;
    check(m_d3d.As(&dxgiDevice));
    // One frame in flight instead of three, so what's drawn follows the pointer without lag.
    check(dxgiDevice->SetMaximumFrameLatency(1));
    ComPtr<IDXGIAdapter> adapter;
    check(dxgiDevice->GetAdapter(&adapter));
    check(adapter->GetParent(IID_PPV_ARGS(&m_dxgiFactory)));
    check(m_d2dFactory->CreateDevice(dxgiDevice.Get(), &m_d2d));
    check(DCompositionCreateDevice(dxgiDevice.Get(), IID_PPV_ARGS(&m_composition)));
}

void GraphicsDevice::reset()
{
    m_composition.Reset();
    m_d2d.Reset();
    m_dxgiFactory.Reset();
    m_d3d.Reset();
    createDevice();
    deviceReset.emit();
}

IDWriteTextFormat *GraphicsDevice::textFormat(const TextStyle &style) const
{
    for (const auto &[cachedStyle, format] : m_textFormats) {
        if (cachedStyle == style)
            return format.Get();
    }

    ComPtr<IDWriteTextFormat> format;
    // System fonts come from the system collection (nullptr), the app font from the bundled one.
    const bool app = style.font == TextFont::App;
    const wchar_t *family = app ? Typography::fontFamily
                            : style.font == TextFont::System ? m_systemFontFamily
                                                             : m_iconFontFamily;
    check(m_dwrite->CreateTextFormat(family, app ? m_fonts.Get() : nullptr, style.weight, DWRITE_FONT_STYLE_NORMAL,
                                     DWRITE_FONT_STRETCH_NORMAL, style.size, L"en-us", &format));
    format->SetTextAlignment(style.align == TextAlign::Center     ? DWRITE_TEXT_ALIGNMENT_CENTER
                             : style.align == TextAlign::Trailing ? DWRITE_TEXT_ALIGNMENT_TRAILING
                                                                  : DWRITE_TEXT_ALIGNMENT_LEADING);
    format->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
    format->SetWordWrapping(style.wrap ? DWRITE_WORD_WRAPPING_WRAP : DWRITE_WORD_WRAPPING_NO_WRAP);
    if (style.elide) {
        const DWRITE_TRIMMING trimming{DWRITE_TRIMMING_GRANULARITY_CHARACTER, 0, 0};
        ComPtr<IDWriteInlineObject> ellipsis;
        check(m_dwrite->CreateEllipsisTrimmingSign(format.Get(), &ellipsis));
        format->SetTrimming(&trimming, ellipsis.Get());
    }
    if (style.lineHeight > 0)
        format->SetLineSpacing(DWRITE_LINE_SPACING_METHOD_UNIFORM, style.lineHeight, style.lineHeight * 0.8f);
    return m_textFormats.emplace_back(style, std::move(format)).second.Get();
}

ComPtr<IDWriteTextLayout> GraphicsDevice::layoutText(std::wstring_view text, const TextStyle &style, Size box) const
{
    ComPtr<IDWriteTextLayout> layout;
    check(m_dwrite->CreateTextLayout(text.data(), static_cast<UINT32>(text.size()), textFormat(style), box.width,
                                     box.height, &layout));
    return layout;
}

Size GraphicsDevice::measureText(std::wstring_view text, const TextStyle &style, float maxWidth) const
{
    // An empty text still takes a line's height, so layouts don't shift when it fills in.
    DWRITE_TEXT_METRICS metrics{};
    check(layoutText(text.empty() ? L" " : text, style, {maxWidth, 100000})->GetMetrics(&metrics));
    return {text.empty() ? 0 : metrics.widthIncludingTrailingWhitespace, metrics.height};
}

float GraphicsDevice::caretX(std::wstring_view text, const TextStyle &style, size_t index) const
{
    if (index == 0)
        return 0;
    // The trailing edge of the character before the caret.
    FLOAT x = 0;
    FLOAT y = 0;
    DWRITE_HIT_TEST_METRICS metrics{};
    check(layoutText(text, style, {100000, 100000})->HitTestTextPosition(static_cast<UINT32>(index - 1), TRUE, &x, &y, &metrics));
    return x;
}

size_t GraphicsDevice::caretIndexAt(std::wstring_view text, const TextStyle &style, float x) const
{
    if (text.empty())
        return 0;
    BOOL trailing = FALSE;
    BOOL inside = FALSE;
    DWRITE_HIT_TEST_METRICS metrics{};
    check(layoutText(text, style, {100000, 100000})->HitTestPoint(x, 0, &trailing, &inside, &metrics));
    return metrics.textPosition + (trailing ? metrics.length : 0);
}
