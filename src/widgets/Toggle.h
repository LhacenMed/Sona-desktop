#pragma once

#include <functional>

#include "widgets/View.h"

// Figma "Toggle" (536:1014): a square switch labelled ON or OFF beside its thumb; on is
// accent-coloured. Clicking slides the thumb and label across. 68×32.
class Toggle : public View
{
public:
    explicit Toggle(bool on);

    bool isOn() const { return m_on; }
    void setOn(bool on);
    void setOnChanged(std::function<void(bool)> onChanged) { m_onChanged = std::move(onChanged); }

    bool takesMouse() const override { return true; }
    Cursor cursor() const override { return Cursor::Hand; }
    void paint(Canvas &canvas) override;
    void mouseReleased(Point point, bool inside) override;

private:
    bool m_on;
    // From off (0) to on (1), as drawn.
    Animation m_position;
    std::function<void(bool)> m_onChanged;
};
