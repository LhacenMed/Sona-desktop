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

void IconButton::setEnabled(bool enabled)
{
    if (enabled == m_enabled)
        return;
    m_enabled = enabled;
    invalidate();
}

Size IconButton::preferredSize() const
{
    return {iconSize + 2 * m_horizontalPadding, iconSize + 2 * m_verticalPadding};
}

void IconButton::paint(Canvas &canvas)
{
    const Palette &palette = canvas.palette();
    canvas.fillRoundedRect(bounds(), 4, isPressed() ? palette.pressedOverlay : palette.hoverOverlay.faded(hoverAmount()));
    const Point iconOrigin{(frame().width - iconSize) / 2, (frame().height - iconSize) / 2};
    if (m_isActive && m_isActive())
        drawActiveIcon(canvas, m_iconName, iconOrigin);
    else
        drawIcon(canvas, m_iconName, iconOrigin, m_enabled ? palette.*m_iconColor : palette.fgA4);
}

void IconButton::mouseReleased(Point, bool inside)
{
    if (inside && m_onClick)
        m_onClick();
}
