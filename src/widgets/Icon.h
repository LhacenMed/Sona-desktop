#pragma once

#include <string>
#include <string_view>

#include "core/Color.h"
#include "widgets/View.h"

class Canvas;

inline constexpr float iconSize = 24;

// Draws an icon from the Figma "Icon" set (assets/icons/<name>.svg), 24×24 unless `size` says
// otherwise, in `color`, which replaces the icon's Figma foreground (Dark/FG/A2). Divider strokes
// follow the theme's Divider/Bright.
void drawIcon(Canvas &canvas, std::wstring_view name, Point topLeft, Color color, float size = iconSize);
// Draws the two-tone variant of an icon (assets/icons/<name>-duo.svg): its grey in FG/A2 and its
// orange in the accent.
void drawDuoIcon(Canvas &canvas, std::wstring_view name, Point topLeft, float size = iconSize);

// A non-interactive icon, centred in its frame: the icon plus `padding` on each side.
class IconView : public View
{
public:
    explicit IconView(std::wstring name, float padding = 0);

    Size preferredSize() const override;
    void paint(Canvas &canvas) override;

private:
    std::wstring m_name;
    float m_padding;
};
