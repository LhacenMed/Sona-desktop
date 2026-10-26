#include "widgets/Button.h"

#include "graphics/Canvas.h"
#include "style/Theme.h"
#include "widgets/Icon.h"

namespace {

constexpr float buttonIconSize = 20;
constexpr float iconGap = 6;

} // namespace

Button::Button(std::wstring text, ButtonKind kind, TextStyle style, std::wstring icon)
    : m_text(std::move(text))
    , m_kind(kind)
    , m_style(style)
    , m_icon(std::move(icon))
{
    m_style.align = TextAlign::Leading;
}

void Button::setText(std::wstring text)
{
    if (text == m_text)
        return;
    m_text = std::move(text);
    invalidate();
}

void Button::setEnabled(bool enabled)
{
    if (enabled == m_enabled)
        return;
    m_enabled = enabled;
    invalidate();
}

void Button::paint(Canvas &canvas)
{
    const Palette &palette = canvas.palette();
    const float hover = isPressed() ? 1 : hoverAmount();
    float radius = 6;
    Color fill;
    Color foreground;
    switch (m_kind) {
    case ButtonKind::Primary:
        // Disabled, it keeps the accent but dims it (Figma's disabled "Save").
        fill = m_enabled ? palette.accentMain : palette.bgAccentBright;
        foreground = m_enabled ? palette.fgB1 : palette.accentMain;
        break;
    case ButtonKind::Secondary:
        fill = palette.bgA3;
        foreground = palette.fgA2;
        break;
    case ButtonKind::Neutral:
        // Its hover state is Figma's second variant rather than an overlay.
        radius = 4;
        fill = Color::mix(palette.bgB2, palette.bgA4, hover);
        foreground = Color::mix(palette.fgA3, palette.fgA2, hover);
        break;
    }
    if (!m_enabled) {
        fill.a *= 0.5f;
        foreground.a *= 0.5f;
    }
    canvas.fillRoundedRect(bounds(), radius, fill);
    if (m_kind != ButtonKind::Neutral)
        canvas.fillRoundedRect(bounds(), radius, isPressed() ? palette.pressedOverlay : palette.hoverOverlay.faded(hover));

    const float textWidth = canvas.measureText(m_text, m_style).width;
    const float iconWidth = m_icon.empty() ? 0 : buttonIconSize + iconGap;
    float x = (frame().width - iconWidth - textWidth) / 2;
    if (!m_icon.empty()) {
        drawIcon(canvas, m_icon, {x, (frame().height - buttonIconSize) / 2}, foreground, buttonIconSize);
        x += iconWidth;
    }
    canvas.drawText(m_text, {x, 0, textWidth, frame().height}, m_style, foreground);
}

void Button::mouseReleased(Point, bool inside)
{
    if (inside && m_onClick)
        m_onClick();
}
