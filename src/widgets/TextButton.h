#pragma once

#include <functional>
#include <string>

#include "graphics/TextStyle.h"
#include "widgets/View.h"

// A labelled button (Figma onboarding "Next" / "Back" 1902:3383, 6696:20706, and "Browse"). The
// primary one is filled with the accent.
class TextButton : public View
{
public:
    TextButton(std::wstring text, Size size, TextStyle style, bool primary);

    void setText(std::wstring text);
    void setOnClick(std::function<void()> onClick) { m_onClick = std::move(onClick); }

    bool takesMouse() const override { return true; }
    bool showsHandCursor() const override { return true; }
    void paint(Canvas &canvas) override;
    void mouseReleased(Point point, bool inside) override;

private:
    std::wstring m_text;
    TextStyle m_style;
    bool m_primary;
    std::function<void()> m_onClick;
};
