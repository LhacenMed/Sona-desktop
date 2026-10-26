#include "widgets/Icon.h"

#include "graphics/Canvas.h"
#include "style/Theme.h"

void drawIcon(Canvas &canvas, std::wstring_view name, Point topLeft, Color color, float size)
{
    canvas.drawSvg(L"icons/" + std::wstring(name) + L".svg", {topLeft.x, topLeft.y, size, size}, {{0x8F8F8F, color}});
}

void drawDuoIcon(Canvas &canvas, std::wstring_view name, Point topLeft, float size)
{
    const Palette &palette = canvas.palette();
    canvas.drawSvg(L"icons/" + std::wstring(name) + L"-duo.svg", {topLeft.x, topLeft.y, size, size},
                   {{0x8F8F8F, palette.fgA2}, {0xAB6314, palette.accentDim}, {0xD37918, palette.accentMain}});
}

void drawActiveIcon(Canvas &canvas, std::wstring_view name, Point topLeft, float size)
{
    canvas.drawSvg(L"icons/" + std::wstring(name) + L"-active.svg", {topLeft.x, topLeft.y, size, size},
                   {{0xD37918, canvas.palette().accentMain}});
}

IconView::IconView(std::wstring name, float padding)
    : m_name(std::move(name))
    , m_padding(padding)
{
}

Size IconView::preferredSize() const
{
    return {iconSize + 2 * m_padding, iconSize + 2 * m_padding};
}

void IconView::paint(Canvas &canvas)
{
    drawIcon(canvas, m_name, {(frame().width - iconSize) / 2, (frame().height - iconSize) / 2}, canvas.palette().fgA2);
}
