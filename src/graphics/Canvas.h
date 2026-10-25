#pragma once

#include <d2d1_3.h>
#include <wrl/client.h>

#include <cstdint>
#include <initializer_list>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "core/Color.h"
#include "core/Geometry.h"
#include "graphics/TextStyle.h"

class GraphicsDevice;
class Theme;
struct Palette;

// Replaces one color of an SVG asset, as Figma wrote it (0xRRGGBB), when drawing it.
struct ColorSwap
{
    std::uint32_t from;
    Color to;
};

struct GradientStop
{
    float position;
    Color color;
};

// The drawing API views paint with, in DIPs. Wraps a window's Direct2D context and caches what
// drawing creates for it (the brush, parsed SVGs).
class Canvas
{
public:
    Canvas(GraphicsDevice &graphics, const Theme &theme);

    // Draws into `context` from now on (after the window's device was created or recreated).
    void bind(ID2D1DeviceContext5 *context);
    void beginFrame();

    const Palette &palette() const;
    Size measureText(std::wstring_view text, const TextStyle &style, float maxWidth = 100000) const;

    void save();
    void restore();
    void translate(float dx, float dy);
    void scale(float factor);
    void pushClip(const Rect &rect);
    void popClip();

    // Replaces the pixels in `rect`, unlike the fills, which blend over them.
    void clear(const Rect &rect, Color color);
    void fillRect(const Rect &rect, Color color);
    void fillRoundedRect(const Rect &rect, float radius, Color color);
    void fillRoundedRect(const Rect &rect, float radius, Point start, Point end, std::initializer_list<GradientStop> stops);
    // The stroke is centred on the outline, as Direct2D draws it.
    void strokeRoundedRect(const Rect &rect, float radius, float width, Color color);
    void fillCircle(Point center, float radius, Color color);
    void strokeCircle(Point center, float radius, float width, Color color);
    void drawText(std::wstring_view text, const Rect &rect, const TextStyle &style, Color color);
    // Draws an SVG from assets/ stretched to `rect`.
    void drawSvg(std::wstring_view assetPath, const Rect &rect, std::initializer_list<ColorSwap> swaps = {});

private:
    ID2D1SolidColorBrush *brush(Color color);
    Microsoft::WRL::ComPtr<ID2D1SvgDocument> createSvg(std::wstring_view assetPath, Size size,
                                                       std::initializer_list<ColorSwap> swaps);

    GraphicsDevice &m_graphics;
    const Theme &m_theme;
    ID2D1DeviceContext5 *m_context = nullptr;
    Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> m_brush;
    D2D1::Matrix3x2F m_transform = D2D1::Matrix3x2F::Identity();
    std::vector<D2D1::Matrix3x2F> m_savedTransforms;
    std::unordered_map<std::wstring, Microsoft::WRL::ComPtr<ID2D1SvgDocument>> m_svgDocuments;
};
