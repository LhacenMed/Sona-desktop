#include "widgets/IconButton.h"

#include "graphics/Canvas.h"
#include "style/Theme.h"
#include "widgets/Icon.h"

IconButton::IconButton(std::wstring iconName, float horizontalPadding, float verticalPadding)
    : m_iconName(std::move(iconName))
    , m_horizontalPadding(horizontalPadding)
    , m_verticalPadding(verticalPadding)
{
}

void IconButton::setIconName(std::wstring name)
{
    if (name == m_iconName)
        return;
    m_iconName = std::move(name);
    invalidate();
}

Size IconButton::preferredSize() const
{
    return {iconSize + 2 * m_horizontalPadding, iconSize + 2 * m_verticalPadding};
}

void IconButton::paint(Canvas &canvas)
{
    const Palette &palette = canvas.palette();
    if (isPressed() || isHovered())
        canvas.fillRoundedRect(bounds(), 4, isPressed() ? palette.pressedOverlay : palette.hoverOverlay);
    drawIcon(canvas, m_iconName, {(frame().width - iconSize) / 2, (frame().height - iconSize) / 2}, palette.fgA2);
}

void IconButton::mouseReleased(Point, bool inside)
{
    if (inside && m_onClick)
        m_onClick();
}
