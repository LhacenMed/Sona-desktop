#pragma once

#include <string>

#include "core/Color.h"
#include "graphics/TextStyle.h"
#include "style/Theme.h"
#include "widgets/View.h"

class Canvas;

// A line of text in a palette color. Ends that don't fit are elided as the style says, or in
// the middle (setElidesMiddle), which keeps both ends of a path readable.
class Label : public View
{
public:
    Label(std::wstring text, TextStyle style, Color Palette::*color);

    const std::wstring &text() const { return m_text; }
    void setText(std::wstring text);
    // Takes effect at the next paint.
    void setColor(Color Palette::*color) { m_color = color; }
    void setElidesMiddle(bool elides) { m_elidesMiddle = elides; }
    void setPadding(float left, float right)
    {
        m_leftPadding = left;
        m_rightPadding = right;
    }

    Size preferredSize() const override;
    void paint(Canvas &canvas) override;

private:
    const std::wstring &visibleText(const Canvas &canvas, float width) const;

    std::wstring m_text;
    TextStyle m_style;
    Color Palette::*m_color;
    bool m_elidesMiddle = false;
    float m_leftPadding = 0;
    float m_rightPadding = 0;
    // The middle-elided text, kept until the text or width changes.
    mutable std::wstring m_elidedText;
    mutable float m_elidedWidth = -1;
};
