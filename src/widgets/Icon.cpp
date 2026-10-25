#include "widgets/Icon.h"

#include "graphics/Canvas.h"
#include "style/Theme.h"

void drawIcon(Canvas &canvas, std::wstring_view name, Point topLeft, Color color)
{
    canvas.drawSvg(L"icons/" + std::wstring(name) + L".svg", {topLeft.x, topLeft.y, iconSize, iconSize},
                   {{0x8F8F8F, color}});
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
