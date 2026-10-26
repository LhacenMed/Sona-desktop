#include "widgets/CaptionButton.h"

#include <string_view>

#include "graphics/Canvas.h"
#include "platform/Window.h"
#include "style/Theme.h"

CaptionButton::CaptionButton(wchar_t glyph, float width, float glyphSize, HitRole role)
    : m_glyph(glyph)
    , m_glyphSize(glyphSize)
    , m_role(role)
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

void CaptionButton::setEnabled(bool enabled)
{
    if (enabled == m_enabled)
        return;
    m_enabled = enabled;
    invalidate();
}

void CaptionButton::paint(Canvas &canvas)
{
    const Palette &palette = canvas.palette();
    const bool closes = m_role == HitRole::Close;
    const float hover = isPressed() ? 1 : hoverAmount();
    if (closes)
        canvas.fillRect(bounds(), (isPressed() ? palette.closePressed : palette.closeHover).faded(hover));
    else
        canvas.fillRect(bounds(), isPressed() ? palette.pressedOverlay : palette.hoverOverlay.faded(hover));
    const TextStyle glyphStyle{.size = m_glyphSize, .align = TextAlign::Center, .font = TextFont::Icons};
    const Window *host = window();
    Color glyph = m_enabled && (!host || host->isActive()) ? palette.fg1 : palette.fgA3;
    if (closes)
        glyph = Color::mix(glyph, palette.closeForeground, hover);
    canvas.drawText(std::wstring_view(&m_glyph, 1), bounds(), glyphStyle, glyph);
}

void CaptionButton::mouseReleased(Point, bool inside)
{
    if (inside && m_onClick)
        m_onClick();
}
