#pragma once

#include <functional>

#include "widgets/View.h"

// A title bar button drawn like Windows' caption buttons: a full-height cell with the system's
// hover and pressed fills, and a glyph from the system icon font that dims while the window is
// inactive. With a caption role (minimize, maximize, close) Windows treats it as its own button; the
// close button turns red under the pointer, as Windows' does.
class CaptionButton : public View
{
public:
    // Windows' caption buttons are 46px wide with 10px glyphs.
    explicit CaptionButton(wchar_t glyph, float width = 46, float glyphSize = 10, HitRole role = HitRole::Client);

    void setGlyph(wchar_t glyph);
    void setEnabled(bool enabled);
    void setOnClick(std::function<void()> onClick) { m_onClick = std::move(onClick); }

    bool takesMouse() const override { return m_enabled; }
    HitRole hitRole() const override { return m_role; }
    void paint(Canvas &canvas) override;
    void mouseReleased(Point point, bool inside) override;

private:
    wchar_t m_glyph;
    float m_glyphSize;
    HitRole m_role;
    bool m_enabled = true;
    std::function<void()> m_onClick;
};
