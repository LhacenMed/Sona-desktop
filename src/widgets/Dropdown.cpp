#include "widgets/Dropdown.h"

#include <algorithm>
#include <memory>

#include "graphics/Canvas.h"
#include "platform/PopupWindow.h"
#include "style/Surface.h"
#include "style/Theme.h"
#include "widgets/Icon.h"

namespace {

// The field's well padding, and an option's height, gap and text inset (2372:8307, 2107:5934).
constexpr float inset = 2;
constexpr float optionHeight = 44;
constexpr float textInset = 14;
constexpr TextStyle optionStyle{.size = 14, .elide = true};

class OptionList;

// One option: highlighted when hovered, chosen or reached with the keyboard; the chosen one checked.
class OptionRow : public View
{
public:
    OptionRow(OptionList &list, int index, std::wstring text) : m_list(list), m_index(index), m_text(std::move(text)) {}

    bool takesMouse() const override { return true; }
    void paint(Canvas &canvas) override;
    void mouseReleased(Point point, bool inside) override;

private:
    OptionList &m_list;
    int m_index;
    std::wstring m_text;
};

// The options in the popup.
class OptionList : public View
{
public:
    OptionList(const std::vector<std::wstring> &options, int selected, float width, std::function<void(int)> picked)
        : m_selected(selected)
        , m_highlighted(selected)
        , m_picked(std::move(picked))
    {
        for (int i = 0; i < static_cast<int>(options.size()); ++i)
            m_rows.push_back(&add<OptionRow>(*this, i, options[i]));
        const float count = static_cast<float>(options.size());
        setPreferredSize({width, 2 * inset + count * optionHeight + (count - 1) * inset});
    }

    int selected() const { return m_selected; }

    // The keyboard's option, shown while the pointer is on none.
    bool isHighlighted(int index) const
    {
        return index == m_highlighted && std::ranges::none_of(m_rows, [](const View *row) { return row->isHovered(); });
    }

    void pick(int index)
    {
        m_picked(index);
        window()->close();
    }

    void layout() override
    {
        for (size_t i = 0; i < m_rows.size(); ++i)
            m_rows[i]->setFrame({inset, inset + i * (optionHeight + inset), frame().width - 2 * inset, optionHeight});
    }

    bool keyPressed(KeyChord chord) override
    {
        const int last = static_cast<int>(m_rows.size()) - 1;
        switch (chord.key) {
        case VK_UP:
            m_highlighted = std::max(0, m_highlighted - 1);
            break;
        case VK_DOWN:
            m_highlighted = std::min(last, m_highlighted + 1);
            break;
        case VK_HOME:
            m_highlighted = 0;
            break;
        case VK_END:
            m_highlighted = last;
            break;
        case VK_RETURN:
        case VK_SPACE:
            pick(m_highlighted);
            return true;
        default:
            return false;
        }
        invalidate();
        return true;
    }

private:
    int m_selected;
    int m_highlighted;
    std::function<void(int)> m_picked;
    std::vector<View *> m_rows;
};

void OptionRow::paint(Canvas &canvas)
{
    const Palette &palette = canvas.palette();
    const bool chosen = m_index == m_list.selected();
    const bool lit = chosen || m_list.isHighlighted(m_index);
    canvas.fillRoundedRect(bounds(), 4, palette.hoverOverlay.faded(lit ? 1 : hoverAmount()));
    const float checkX = frame().width - textInset - iconSize;
    canvas.drawText(m_text, {textInset, 0, checkX - 10 - textInset, frame().height}, optionStyle, palette.fgA2);
    if (chosen)
        drawIcon(canvas, L"check", {checkX, (frame().height - iconSize) / 2}, palette.accentMain);
}

void OptionRow::mouseReleased(Point, bool inside)
{
    if (inside)
        m_list.pick(m_index);
}

} // namespace

Dropdown::Dropdown(std::vector<std::wstring> options, int selected)
    : m_options(std::move(options))
    , m_selected(selected)
{
    setPreferredSize({0, 48});
}

void Dropdown::setSelected(int index)
{
    if (index == m_selected)
        return;
    m_selected = index;
    invalidate();
}

void Dropdown::paint(Canvas &canvas)
{
    const Palette &palette = canvas.palette();
    canvas.fillRoundedRect(bounds(), 6, palette.bgB2);
    const Rect field{inset, inset, frame().width - 2 * inset, frame().height - 2 * inset};
    canvas.fillRoundedRect(field, 4, palette.bgA4);
    canvas.fillRoundedRect(field, 4, isPressed() ? palette.pressedOverlay : palette.hoverOverlay.faded(hoverAmount()));
    const float chevronX = field.right() - textInset - iconSize;
    canvas.drawText(m_options[m_selected], {field.x + textInset, 0, chevronX - 10 - field.x - textInset, frame().height},
                    optionStyle, palette.fgA2);
    drawIcon(canvas, L"chevron-down", {chevronX, (frame().height - iconSize) / 2}, palette.fgA2);
}

void Dropdown::mouseReleased(Point, bool inside)
{
    if (!inside)
        return;
    // The list opens over the field, its first option where the field is (Figma's open state).
    Window *host = window();
    const Rect field = mapToWindow(bounds());
    host->openWindow<PopupWindow>(*host, field, PopupPlacement::Over, Surfaces::list,
                                  std::make_unique<OptionList>(m_options, m_selected, field.width, [this](int index) { pick(index); }));
}

void Dropdown::pick(int index)
{
    if (index == m_selected)
        return;
    setSelected(index);
    if (m_onChanged)
        m_onChanged(index);
}
