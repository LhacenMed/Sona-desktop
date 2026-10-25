#pragma once

#include "widgets/View.h"

// Lays its visible children out left to right, like a QML RowLayout: each at its preferred width,
// the ones that fill sharing what's left, vertically centred (or full height when they fill it).
class Row : public View
{
public:
    explicit Row(float spacing = 0) : m_spacing(spacing) {}

    void setPadding(float left, float right)
    {
        m_leftPadding = left;
        m_rightPadding = right;
    }

    Size preferredSize() const override;
    void layout() override;

private:
    float m_spacing;
    float m_leftPadding = 0;
    float m_rightPadding = 0;
};
