#pragma once

#include <functional>
#include <string>

#include "widgets/View.h"

// A clickable icon. Each call site sets the padding its Figma frame uses; every icon button shares
// the same hover and pressed feedback.
class IconButton : public View
{
public:
    explicit IconButton(std::wstring iconName, float horizontalPadding = 0, float verticalPadding = 0);

    void setIconName(std::wstring name);
    void setOnClick(std::function<void()> onClick) { m_onClick = std::move(onClick); }

    Size preferredSize() const override;
    bool takesMouse() const override { return true; }
    bool showsHandCursor() const override { return true; }
    void paint(Canvas &canvas) override;
    void mouseReleased(Point point, bool inside) override;

protected:
    const std::wstring &iconName() const { return m_iconName; }

private:
    std::wstring m_iconName;
    float m_horizontalPadding;
    float m_verticalPadding;
    std::function<void()> m_onClick;
};

// Figma "Icon Wrapper" (177:98): a 30×32 icon button.
class IconWrapper : public IconButton
{
public:
    explicit IconWrapper(std::wstring iconName) : IconButton(std::move(iconName), 3, 4) {}
};
