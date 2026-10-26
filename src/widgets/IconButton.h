#pragma once

#include <functional>
#include <string>

#include "style/Theme.h"
#include "widgets/View.h"

// A clickable icon. Each call site sets the padding its Figma frame uses; every icon button shares
// the same hover and pressed feedback.
class IconButton : public View
{
public:
    explicit IconButton(std::wstring iconName, float horizontalPadding = 0, float verticalPadding = 0);

    void setIconName(std::wstring name);
    // The icon's color; Figma's FG/A2 unless the frame says otherwise. Takes effect at the next paint.
    void setIconColor(Color Palette::*color) { m_iconColor = color; }
    // Disabled, it's dimmed and ignores the mouse.
    void setEnabled(bool enabled);
    // While `isActive` says so, the button shows its icon's active variant (drawActiveIcon), read
    // whenever it paints.
    void setActiveWhen(std::function<bool()> isActive) { m_isActive = std::move(isActive); }
    void setOnClick(std::function<void()> onClick) { m_onClick = std::move(onClick); }

    Size preferredSize() const override;
    bool takesMouse() const override { return m_enabled; }
    Cursor cursor() const override { return Cursor::Hand; }
    void paint(Canvas &canvas) override;
    void mouseReleased(Point point, bool inside) override;

protected:
    const std::wstring &iconName() const { return m_iconName; }

private:
    std::wstring m_iconName;
    float m_horizontalPadding;
    float m_verticalPadding;
    Color Palette::*m_iconColor = &Palette::fgA2;
    bool m_enabled = true;
    std::function<bool()> m_isActive;
    std::function<void()> m_onClick;
};

// Figma "Icon Wrapper" (177:98): a 30×32 icon button.
class IconWrapper : public IconButton
{
public:
    explicit IconWrapper(std::wstring iconName) : IconButton(std::move(iconName), 3, 4) {}
};
