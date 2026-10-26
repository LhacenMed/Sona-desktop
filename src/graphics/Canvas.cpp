#include "graphics/Canvas.h"

#include <d2d1effects.h>
#include <Shlwapi.h>

#include <format>
#include <string>

#include "graphics/GraphicsDevice.h"
#include "platform/HResult.h"
#include "platform/Resources.h"
#include "style/Theme.h"

using Microsoft::WRL::ComPtr;

namespace {

D2D1_RECT_F toD2D(const Rect &rect)
{
    return D2D1::RectF(rect.x, rect.y, rect.right(), rect.bottom());
}

D2D1_COLOR_F toD2D(Color color)
{
    return D2D1::ColorF(color.r, color.g, color.b, color.a);
}

} // namespace

Canvas::Canvas(GraphicsDevice &graphics, const Theme &theme)
    : m_graphics(graphics)
    , m_theme(theme)
{
}

void Canvas::bind(ID2D1DeviceContext5 *context)
{
    m_context = context;
    m_svgDocuments.clear();
    m_brush.Reset();
    check(context->CreateSolidColorBrush(D2D1::ColorF(0, 0), &m_brush));
    m_recorder.Reset();
    check(m_graphics.d2d()->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_NONE, &m_recorder));
}

void Canvas::beginFrame()
{
    m_transform = D2D1::Matrix3x2F::Identity();
    m_savedTransforms.clear();
    m_context->SetTransform(m_transform);
}

const Palette &Canvas::palette() const
{
    return m_theme.palette();
}

Size Canvas::measureText(std::wstring_view text, const TextStyle &style, float maxWidth) const
{
    return m_graphics.measureText(text, style, maxWidth);
}

void Canvas::save()
{
    m_savedTransforms.push_back(m_transform);
}

void Canvas::restore()
{
    m_transform = m_savedTransforms.back();
    m_savedTransforms.pop_back();
    m_context->SetTransform(m_transform);
}

void Canvas::translate(float dx, float dy)
{
    m_transform = D2D1::Matrix3x2F::Translation(dx, dy) * m_transform;
    m_context->SetTransform(m_transform);
}

void Canvas::scale(float factor)
{
    m_transform = D2D1::Matrix3x2F::Scale(factor, factor) * m_transform;
    m_context->SetTransform(m_transform);
}

void Canvas::pushClip(const Rect &rect)
{
    m_context->PushAxisAlignedClip(toD2D(rect), D2D1_ANTIALIAS_MODE_ALIASED);
}

void Canvas::popClip()
{
    m_context->PopAxisAlignedClip();
}

void Canvas::clear(const Rect &rect, Color color)
{
    pushClip(rect);
    m_context->Clear(toD2D(color));
    popClip();
}

void Canvas::fillRect(const Rect &rect, Color color)
{
    m_context->FillRectangle(toD2D(rect), brush(color));
}

void Canvas::fillRoundedRect(const Rect &rect, float radius, Color color)
{
    m_context->FillRoundedRectangle(D2D1::RoundedRect(toD2D(rect), radius, radius), brush(color));
}

void Canvas::fillRoundedRect(const Rect &rect, float radius, Point start, Point end, std::initializer_list<GradientStop> stops)
{
    std::vector<D2D1_GRADIENT_STOP> d2dStops;
    for (const GradientStop &stop : stops)
        d2dStops.push_back({stop.position, toD2D(stop.color)});
    ComPtr<ID2D1GradientStopCollection> collection;
    check(m_context->CreateGradientStopCollection(d2dStops.data(), static_cast<UINT32>(d2dStops.size()), &collection));
    ComPtr<ID2D1LinearGradientBrush> gradient;
    check(m_context->CreateLinearGradientBrush(
        D2D1::LinearGradientBrushProperties(D2D1::Point2F(start.x, start.y), D2D1::Point2F(end.x, end.y)), collection.Get(),
        &gradient));
    m_context->FillRoundedRectangle(D2D1::RoundedRect(toD2D(rect), radius, radius), gradient.Get());
}

void Canvas::strokeRoundedRect(const Rect &rect, float radius, float width, Color color)
{
    m_context->DrawRoundedRectangle(D2D1::RoundedRect(toD2D(rect), radius, radius), brush(color), width);
}

void Canvas::fillCircle(Point center, float radius, Color color)
{
    m_context->FillEllipse(D2D1::Ellipse(D2D1::Point2F(center.x, center.y), radius, radius), brush(color));
}

void Canvas::strokeCircle(Point center, float radius, float width, Color color)
{
    m_context->DrawEllipse(D2D1::Ellipse(D2D1::Point2F(center.x, center.y), radius, radius), brush(color), width);
}

void Canvas::drawShadow(const Rect &rect, float radius, Point offset, float blur, Color color)
{
    // The shape, recorded on a separate context, cast through Direct2D's shadow effect.
    ComPtr<ID2D1CommandList> shape;
    check(m_recorder->CreateCommandList(&shape));
    m_recorder->SetTarget(shape.Get());
    m_recorder->BeginDraw();
    ComPtr<ID2D1SolidColorBrush> black;
    check(m_recorder->CreateSolidColorBrush(D2D1::ColorF(0, 0, 0), &black));
    m_recorder->FillRoundedRectangle(D2D1::RoundedRect(toD2D(rect), radius, radius), black.Get());
    check(m_recorder->EndDraw());
    m_recorder->SetTarget(nullptr);
    check(shape->Close());

    ComPtr<ID2D1Effect> shadow;
    check(m_context->CreateEffect(CLSID_D2D1Shadow, &shadow));
    shadow->SetInput(0, shape.Get());
    // Figma's blur radius is twice the Gaussian's standard deviation, as in CSS.
    check(shadow->SetValue(D2D1_SHADOW_PROP_BLUR_STANDARD_DEVIATION, blur / 2));
    check(shadow->SetValue(D2D1_SHADOW_PROP_COLOR, D2D1::Vector4F(color.r, color.g, color.b, color.a)));
    m_context->DrawImage(shadow.Get(), D2D1::Point2F(offset.x, offset.y));
}

void Canvas::drawText(std::wstring_view text, const Rect &rect, const TextStyle &style, Color color)
{
    if (text.empty())
        return;
    const auto layout = m_graphics.layoutText(text, style, rect.size());
    m_context->DrawTextLayout(D2D1::Point2F(rect.x, rect.y), layout.Get(), brush(color));
}

void Canvas::drawSvg(std::wstring_view assetPath, const Rect &rect, std::initializer_list<ColorSwap> swaps)
{
    std::wstring key = std::format(L"{}|{}x{}", assetPath, rect.width, rect.height);
    for (const ColorSwap &swap : swaps)
        key += std::format(L"|{:06X}>{:06X}", swap.from, swap.to.rgb());
    ComPtr<ID2D1SvgDocument> &document = m_svgDocuments[key];
    if (!document)
        document = createSvg(assetPath, rect.size(), swaps);

    m_context->SetTransform(D2D1::Matrix3x2F::Translation(rect.x, rect.y) * m_transform);
    m_context->DrawSvgDocument(document.Get());
    m_context->SetTransform(m_transform);
}

ComPtr<ID2D1SvgDocument> Canvas::createSvg(std::wstring_view assetPath, Size size, std::initializer_list<ColorSwap> swaps)
{
    const auto data = asset(assetPath);
    std::string svg(reinterpret_cast<const char *>(data.data()), data.size());
    for (const ColorSwap &swap : swaps) {
        const std::string from = std::format("#{:06X}", swap.from);
        const std::string to = std::format("#{:06X}", swap.to.rgb());
        for (size_t at = svg.find(from); at != std::string::npos; at = svg.find(from, at + to.size()))
            svg.replace(at, from.size(), to);
    }

    ComPtr<IStream> stream;
    stream.Attach(SHCreateMemStream(reinterpret_cast<const BYTE *>(svg.data()), static_cast<UINT>(svg.size())));
    ComPtr<ID2D1SvgDocument> document;
    check(m_context->CreateSvgDocument(stream.Get(), D2D1::SizeF(size.width, size.height), &document));

    // Drawn at the requested size whatever size the file declares; its viewBox scales the content.
    ComPtr<ID2D1SvgElement> root;
    document->GetRoot(&root);
    root->SetAttributeValue(L"width", D2D1_SVG_ATTRIBUTE_STRING_TYPE_SVG, std::to_wstring(size.width).c_str());
    root->SetAttributeValue(L"height", D2D1_SVG_ATTRIBUTE_STRING_TYPE_SVG, std::to_wstring(size.height).c_str());
    return document;
}

ID2D1SolidColorBrush *Canvas::brush(Color color)
{
    m_brush->SetColor(toD2D(color));
    return m_brush.Get();
}
