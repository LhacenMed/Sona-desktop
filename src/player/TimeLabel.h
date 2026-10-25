#pragma once

#include <functional>

#include "widgets/View.h"

// Figma "Current Time" / "Duration" (1904:7238, 1905:7312): a time in hh:mm:ss.
class TimeLabel : public View
{
public:
    TimeLabel(std::function<double()> seconds, bool hoverHighlight);

    bool takesMouse() const override { return m_hoverHighlight; }
    void paint(Canvas &canvas) override;

private:
    std::function<double()> m_seconds;
    bool m_hoverHighlight;
};
