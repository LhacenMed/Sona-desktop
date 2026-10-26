#include "maininterface/CaptionButton.h"

#include <string_view>

#include "graphics/Canvas.h"
#include "platform/Window.h"
#include "style/Theme.h"

CaptionButton::CaptionButton(wchar_t glyph, float width, float glyphSize)
    : m_glyph(glyph)
    , m_glyphSize(glyphSize)
{
    setPreferredSize({width, 0});
    setFillsHeight(true);
}

void CaptionButton::setGlyph(wchar_t glyph)
{
    if (glyph == m_glyph)
        return;
    m_glyph = glyph;
    invalidate();
}

void CaptionButton::paint(Canvas &canvas)
{
    const Palette &palette = canvas.palette();
    canvas.fillRect(bounds(), isPressed() ? palette.pressedOverlay : palette.hoverOverlay.faded(hoverAmount()));
    const TextStyle glyphStyle{.size = m_glyphSize, .align = TextAlign::Center, .font = TextFont::Icons};
    const Window *host = window();
    canvas.drawText(std::wstring_view(&m_glyph, 1), bounds(), glyphStyle,
                    !host || host->isActive() ? palette.fg1 : palette.fgA3);
}

void CaptionButton::mouseReleased(Point, bool inside)
{
    if (inside && m_onClick)
        m_onClick();
}
