#include "widgets/Segmented.h"

#include <algorithm>

#include "graphics/Canvas.h"
#include "style/Theme.h"

namespace {

// The well's padding and the gap between options.
constexpr float inset = 2;
constexpr TextStyle optionStyle{.size = 14, .align = TextAlign::Center, .elide = true};

} // namespace

Segmented::Segmented(std::vector<std::wstring> options, int selected)
    : m_options(std::move(options))
    , m_selected(selected)
{
}

void Segmented::setSelected(int index)
{
    if (index == m_selected)
        return;
    m_selected = index;
    invalidate();
}

Rect Segmented::cell(int index) const
{
    const float count = static_cast<float>(m_options.size());
    const float width = (frame().width - 2 * inset - inset * (count - 1)) / count;
    return {inset + index * (width + inset), inset, width, frame().height - 2 * inset};
}

int Segmented::indexAt(float x) const
{
    const int count = static_cast<int>(m_options.size());
    return std::clamp(static_cast<int>((x - inset) / (frame().width - 2 * inset) * count), 0, count - 1);
}

void Segmented::paint(Canvas &canvas)
{
    const Palette &palette = canvas.palette();
    canvas.fillRoundedRect(bounds(), 6, palette.bgB2);
    for (int i = 0; i < static_cast<int>(m_options.size()); ++i) {
        const Rect box = cell(i);
        const bool selected = i == m_selected;
        canvas.fillRoundedRect(box, 4, selected ? palette.bgAccentBright : palette.bgA4);
        canvas.drawText(m_options[i], box, optionStyle, selected ? palette.accentMain : palette.fgA3);
    }
}

void Segmented::mouseReleased(Point point, bool inside)
{
    if (!inside)
        return;
    const int index = indexAt(point.x);
    if (index == m_selected)
        return;
    setSelected(index);
    if (m_onChanged)
        m_onChanged(index);
}
