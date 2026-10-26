#include "widgets/ListRow.h"

#include "graphics/Canvas.h"
#include "style/Theme.h"
#include "widgets/Icon.h"

namespace {

constexpr float iconBox = 32;
constexpr float gap = 10;
constexpr float trailingPadding = 4;
constexpr TextStyle nameStyle{.size = 14, .elide = true};
constexpr TextStyle detailStyle{.size = 14, .align = TextAlign::Trailing};

} // namespace

ListRow::ListRow(std::wstring icon, std::wstring name, std::wstring detail)
    : m_icon(std::move(icon))
    , m_name(std::move(name))
    , m_detail(std::move(detail))
{
    setPreferredSize({0, 32});
}

void ListRow::setSelected(bool selected)
{
    if (selected == m_selected)
        return;
    m_selected = selected;
    invalidate();
}

void ListRow::paint(Canvas &canvas)
{
    const Palette &palette = canvas.palette();
    canvas.fillRoundedRect(bounds(), 4, isPressed() ? palette.pressedOverlay : palette.hoverOverlay.faded(hoverAmount()));
    drawIcon(canvas, m_icon, {(iconBox - iconSize) / 2, (frame().height - iconSize) / 2}, palette.fgA2);
    const float detailWidth = canvas.measureText(m_detail, detailStyle).width;
    const float detailX = frame().width - trailingPadding - detailWidth;
    canvas.drawText(m_detail, {detailX, 0, detailWidth, frame().height}, detailStyle, palette.fgA3);
    const float nameX = iconBox + gap;
    canvas.drawText(m_name, {nameX, 0, detailX - gap - nameX, frame().height}, nameStyle,
                    m_selected ? palette.accentMain : palette.fgA1);
}

void ListRow::mouseReleased(Point, bool inside)
{
    if (inside && m_onClick)
        m_onClick();
}
