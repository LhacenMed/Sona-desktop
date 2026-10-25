#pragma once

#include <functional>

#include "widgets/View.h"

// A title bar button drawn like the native caption buttons: a full-height cell with the system's
// hover and pressed fills, and a glyph from the system icon font that dims while the window is
// inactive. The window's minimize, maximize and close are Windows' own; this is every other title
// bar button (menu, sidebar, full screen), and the pictures of the native ones in previews.
class CaptionButton : public View
{
public:
    // Windows' caption buttons are 46px wide with 10px glyphs.
    explicit CaptionButton(wchar_t glyph, float width = 46, float glyphSize = 10);

    void setGlyph(wchar_t glyph);
    void setOnClick(std::function<void()> onClick) { m_onClick = std::move(onClick); }

    bool takesMouse() const override { return true; }
    void paint(Canvas &canvas) override;
    void mouseReleased(Point point, bool inside) override;

private:
    wchar_t m_glyph;
    float m_glyphSize;
    std::function<void()> m_onClick;
};
