#pragma once

#include "widgets/Stack.h"

struct BarContext;

// Figma "Controls" (491:1779): the layout's control-bar groups, 20 apart.
class ControlBar : public Row
{
public:
    explicit ControlBar(BarContext &context);

    // Clicks between its controls stay on the bar.
    bool takesMouse() const override { return true; }
    void paint(Canvas &canvas) override;
};
