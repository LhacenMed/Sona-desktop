#include "widgets/TextButton.h"

#include "graphics/Canvas.h"
#include "style/Theme.h"

TextButton::TextButton(std::wstring text, Size size, TextStyle style, bool primary)
    : m_text(std::move(text))
    , m_style(style)
    , m_primary(primary)
{
    m_style.align = TextAlign::Center;
    setPreferredSize(size);
}

void TextButton::setText(std::wstring text)
{
    if (text == m_text)
        return;
    m_text = std::move(text);
    invalidate();
}

void TextButton::paint(Canvas &canvas)
{
    const Palette &palette = canvas.palette();
    canvas.fillRoundedRect(bounds(), 6, m_primary ? palette.accentMain : palette.bgA3);
    canvas.drawText(m_text, bounds(), m_style, m_primary ? palette.fgB1 : palette.fgA2);
}

void TextButton::mouseReleased(Point, bool inside)
{
    if (inside && m_onClick)
        m_onClick();
}
