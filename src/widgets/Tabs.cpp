#include "widgets/Tabs.h"

#include <algorithm>

#include "graphics/Canvas.h"
#include "style/Theme.h"
#include "widgets/Icon.h"

namespace {

constexpr float sidePadding = 6;
constexpr float tabGap = 1;
constexpr float tabIconSize = 20;
constexpr float iconGap = 8;
constexpr float contentTop = 14;
constexpr float underlineInset = 8;
constexpr float underlineHeight = 2;
constexpr TextStyle labelStyle{.size = 12};

} // namespace

Tabs::Tabs(std::vector<Tab> tabs, int active)
    : m_tabs(std::move(tabs))
    , m_active(active)
{
    setPreferredSize({0, 50});
}

void Tabs::setActive(int index)
{
    if (index == m_active)
        return;
    m_active = index;
    invalidate();
}

Rect Tabs::tabBounds(int index) const
{
    const float count = static_cast<float>(m_tabs.size());
    const float width = (frame().width - 2 * sidePadding - tabGap * (count - 1)) / count;
    return {sidePadding + index * (width + tabGap), 0, width, frame().height};
}

int Tabs::indexAt(float x) const
{
    const int count = static_cast<int>(m_tabs.size());
    return std::clamp(static_cast<int>((x - sidePadding) / (frame().width - 2 * sidePadding) * count), 0, count - 1);
}

void Tabs::paint(Canvas &canvas)
{
    const Palette &palette = canvas.palette();
    canvas.fillRect(bounds(), palette.bgB2);
    for (int i = 0; i < static_cast<int>(m_tabs.size()); ++i) {
        const Tab &tab = m_tabs[i];
        const Rect box = tabBounds(i);
        const bool active = i == m_active;
        const float labelWidth = canvas.measureText(tab.label, labelStyle).width;
        const float x = box.x + (box.width - tabIconSize - iconGap - labelWidth) / 2;
        if (active)
            drawDuoIcon(canvas, tab.icon, {x, contentTop}, tabIconSize);
        else
            drawIcon(canvas, tab.icon, {x, contentTop}, palette.fgA4, tabIconSize);
        canvas.drawText(tab.label, {x + tabIconSize + iconGap, contentTop, labelWidth, tabIconSize}, labelStyle,
                        active ? palette.fgA2 : palette.fgA3);
        if (active) {
            canvas.fillRect({box.x + underlineInset, box.bottom() - underlineHeight, box.width - 2 * underlineInset, underlineHeight},
                            palette.accentMain);
        }
    }
}

void Tabs::mouseReleased(Point point, bool inside)
{
    if (!inside)
        return;
    const int index = indexAt(point.x);
    if (index == m_active)
        return;
    setActive(index);
    if (m_onChanged)
        m_onChanged(index);
}
