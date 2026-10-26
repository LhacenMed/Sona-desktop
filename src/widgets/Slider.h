#pragma once

#include <functional>
#include <string>
#include <vector>

#include "widgets/View.h"

// Figma range (the speed slider, 5187:24544): a track with an accent fill up to a thumb. With
// labels, it shows them evenly spaced under the track, from the minimum to the maximum, with a tick
// on the track for each one between. Click or drag to set the value, in steps: while dragging, the
// thumb follows the pointer, and it settles on the step when released.
class Slider : public View
{
public:
    Slider(float minimum, float maximum, float step, float value, std::vector<std::wstring> labels = {});

    float value() const { return m_value; }
    void setValue(float value);
    void setOnChanged(std::function<void(float)> onChanged) { m_onChanged = std::move(onChanged); }

    Size preferredSize() const override;
    bool takesMouse() const override { return true; }
    Cursor cursor() const override { return Cursor::Hand; }
    void paint(Canvas &canvas) override;
    void mousePressed(Point point) override;
    void mouseDragged(Point point) override;
    void mouseReleased(Point point, bool inside) override;

private:
    // Where a value lies in the range, 0–1.
    float fractionOf(float value) const { return (value - m_minimum) / (m_maximum - m_minimum); }
    // Where along the track a fraction of the range lies, for the thumb's left edge.
    float thumbX(float fraction) const;
    void setValueAt(float x);

    float m_minimum;
    float m_maximum;
    float m_step;
    float m_value;
    // Where the thumb is, as a fraction of the range.
    Animation m_thumb;
    std::vector<std::wstring> m_labels;
    std::function<void(float)> m_onChanged;
};
