#pragma once

#include <d2d1_3.h>
#include <d3d11.h>
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
struct Surface;

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

// What's behind a window, for its frosted surfaces to blur: `image` (BackdropCapture) has its top-left
// at `origin`, in the window's content DIPs.
struct Backdrop
{
    ID3D11Texture2D *image;
    Point origin;
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
    // Clips to a rounded rectangle, as a Figma frame clips its content, until popRoundedClip.
    void pushRoundedClip(const Rect &rect, float radius);
    void popRoundedClip();
    // Draws everything until popOpacity at `opacity`, as one layer.
    void pushOpacity(float opacity);
    void popOpacity();

    // Replaces the pixels in `rect`, unlike the fills, which blend over them.
    void clear(const Rect &rect, Color color);
    void fillRect(const Rect &rect, Color color);
    void fillRoundedRect(const Rect &rect, float radius, Color color);
    void fillRoundedRect(const Rect &rect, float radius, Point start, Point end, std::initializer_list<GradientStop> stops);
    // The stroke is centred on the outline, as Direct2D draws it.
    void strokeRoundedRect(const Rect &rect, float radius, float width, Color color);
    void fillCircle(Point center, float radius, Color color);
    void strokeCircle(Point center, float radius, float width, Color color);
    // A Figma drop shadow under a rounded rectangle: `blur` is Figma's blur radius.
    void drawShadow(const Rect &rect, float radius, Point offset, float blur, Color color);
    // A Figma surface (style/Surface.h): its shadow, then its fill. A frosted one shows `behind` blurred
    // under its fill, and is opaque where nothing is behind it.
    void fillSurface(const Rect &rect, const Surface &surface, const Backdrop *behind = nullptr);
    void drawText(std::wstring_view text, const Rect &rect, const TextStyle &style, Color color);
    // Draws a picture from memory (32-bit BGR pixels, top row first) stretched to `rect`.
    void drawPixels(const Rect &rect, const std::uint32_t *pixels, int width, int height);
    // Draws an SVG from assets/ stretched to `rect`.
    void drawSvg(std::wstring_view assetPath, const Rect &rect, std::initializer_list<ColorSwap> swaps = {});

private:
    ID2D1SolidColorBrush *brush(Color color);
    // `behind` blurred by `deviation` (DIPs), where it lies under `rect`.
    void drawBlurred(const Backdrop &behind, const Rect &rect, float deviation);
    Microsoft::WRL::ComPtr<ID2D1SvgDocument> createSvg(std::wstring_view assetPath, Size size,
                                                       std::initializer_list<ColorSwap> swaps);

    GraphicsDevice &m_graphics;
    const Theme &m_theme;
    ID2D1DeviceContext5 *m_context = nullptr;
    // Records the shapes that shadows are cast from.
    Microsoft::WRL::ComPtr<ID2D1DeviceContext> m_recorder;
    Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> m_brush;
    D2D1::Matrix3x2F m_transform = D2D1::Matrix3x2F::Identity();
    std::vector<D2D1::Matrix3x2F> m_savedTransforms;
    std::unordered_map<std::wstring, Microsoft::WRL::ComPtr<ID2D1SvgDocument>> m_svgDocuments;
    // Kept across frames: the backdrop as a bitmap (until its image or the DPI changes), and the
    // effects that crop and blur it.
    ID3D11Texture2D *m_backdropImage = nullptr;
    Microsoft::WRL::ComPtr<ID2D1Bitmap1> m_backdropBitmap;
    Microsoft::WRL::ComPtr<ID2D1Effect> m_backdropCrop;
    Microsoft::WRL::ComPtr<ID2D1Effect> m_backdropBlur;
};
