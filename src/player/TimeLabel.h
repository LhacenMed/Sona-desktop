#pragma once

#include <functional>

#include "widgets/View.h"

// Figma "Current Time" / "Duration" (1904:7238, 1905:7312): a time in hh:mm:ss, or "-hh:mm:ss" for a
// negative one (time left). Clickable when given an action.
class TimeLabel : public View
{
public:
    TimeLabel(std::function<double()> seconds, bool hoverHighlight);

    void setOnClick(std::function<void()> onClick) { m_onClick = std::move(onClick); }

    bool takesMouse() const override { return m_hoverHighlight; }
    Cursor cursor() const override { return m_onClick ? Cursor::Hand : Cursor::Arrow; }
    void paint(Canvas &canvas) override;
    void mouseReleased(Point point, bool inside) override;

private:
    std::function<double()> m_seconds;
    bool m_hoverHighlight;
    std::function<void()> m_onClick;
};
