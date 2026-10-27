#include "widgets/Expander.h"

#include "graphics/Canvas.h"
#include "style/Theme.h"
#include "widgets/Icon.h"

namespace {

// The title 16 in, the chevron 10 from the end (536:1857).
constexpr float headerHeight = 48;
constexpr float titleInset = 16;
constexpr float chevronInset = 10;
constexpr TextStyle titleStyle{.size = 14};

} // namespace

// The title and chevron; a click opens or closes the group.
class Expander::Header : public View
{
public:
    explicit Header(Expander &expander) : m_expander(expander) { setPreferredSize({0, headerHeight}); }

    bool takesMouse() const override { return true; }
    Cursor cursor() const override { return Cursor::Hand; }

    void paint(Canvas &canvas) override
    {
        const Palette &palette = canvas.palette();
        const Color color = Color::mix(palette.fgA3, palette.fgA2, hoverAmount());
        const bool expanded = m_expander.isExpanded();
        canvas.drawText(expanded ? L"Collapse Settings" : L"Expand Settings",
                        {titleInset, 0, frame().width - 2 * titleInset, frame().height}, titleStyle, color);
        drawIcon(canvas, expanded ? L"chevron-down" : L"chevron-right",
                 {frame().width - chevronInset - iconSize, (frame().height - iconSize) / 2}, color);
    }

    void mouseReleased(Point, bool inside) override
    {
        if (!inside)
            return;
        m_expander.setExpanded(!m_expander.isExpanded());
        if (m_expander.m_onToggled)
            m_expander.m_onToggled();
    }

private:
    Expander &m_expander;
};

Expander::Expander()
{
    add<Header>(*this).setFillsWidth(true);
    m_content = &add<Column>();
    m_content->setPadding({.left = titleInset, .right = titleInset, .bottom = titleInset});
    m_content->setFillsWidth(true);
    m_content->setVisible(false);
}

void Expander::setExpanded(bool expanded)
{
    m_content->setVisible(expanded);
    invalidate();
}

void Expander::paint(Canvas &canvas)
{
    // Closed it's a well; open, a raised area (6114:13615).
    const Palette &palette = canvas.palette();
    canvas.fillRoundedRect(bounds(), 6, isExpanded() ? palette.bgA3 : palette.bgB2);
}
