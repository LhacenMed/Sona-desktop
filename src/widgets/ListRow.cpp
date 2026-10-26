#include "widgets/ListRow.h"

#include <algorithm>

#include "graphics/Canvas.h"
#include "style/Theme.h"
#include "widgets/Icon.h"

namespace {

constexpr float iconBox = 32;
// Chapter numbers: 34 wide after 4 of padding.
constexpr float numberInset = 4;
constexpr float numberWidth = 34;
constexpr float gap = 10;
constexpr float trailingPadding = 4;
constexpr TextStyle nameStyle{.size = 14, .elide = true};
constexpr TextStyle numberStyle{.size = 14, .weight = DWRITE_FONT_WEIGHT_MEDIUM};
constexpr TextStyle detailStyle{.size = 14, .align = TextAlign::Trailing};

} // namespace

ListRow::ListRow(std::wstring icon, ListItem item)
    : m_icon(std::move(icon))
    , m_item(std::move(item))
{
    setPreferredSize({0, height});
}

void ListRow::setItem(ListItem item)
{
    if (item == m_item)
        return;
    m_item = std::move(item);
    invalidate();
}

void ListRow::setCurrent(bool current)
{
    if (current == m_current)
        return;
    m_current = current;
    invalidate();
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
    const float tint = std::max(hoverAmount(), m_selected ? 1.0f : 0.0f);
    canvas.fillRoundedRect(bounds(), 4, isPressed() ? palette.pressedOverlay : palette.hoverOverlay.faded(tint));
    float nameX = 0;
    if (!m_icon.empty()) {
        drawIcon(canvas, m_icon, {(iconBox - iconSize) / 2, (frame().height - iconSize) / 2}, palette.fgA2);
        nameX = iconBox + gap;
    } else if (m_item.number > 0) {
        canvas.drawText(std::to_wstring(m_item.number), {numberInset, 0, numberWidth, frame().height}, numberStyle,
                        palette.accentMain);
        nameX = numberInset + numberWidth + gap;
    }
    const float detailWidth = canvas.measureText(m_item.detail, detailStyle).width;
    const float detailX = frame().width - trailingPadding - detailWidth;
    canvas.drawText(m_item.detail, {detailX, 0, detailWidth, frame().height}, detailStyle, palette.fgA3);
    canvas.drawText(m_item.name, {nameX, 0, detailX - gap - nameX, frame().height}, nameStyle,
                    m_current ? palette.accentMain : palette.fgA1);
}
