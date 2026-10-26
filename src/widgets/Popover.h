#pragma once

#include "widgets/Stack.h"

// Figma's floating panel (the control-bar panels, 463:831): a rounded, slightly see-through surface
// with its content laid out top to bottom. Open it with Overlay::openPopover.
class Popover : public Column
{
public:
    using Column::Column;

    // Clicks on the panel stay on it.
    bool takesMouse() const override { return true; }
    void paint(Canvas &canvas) override;
};
