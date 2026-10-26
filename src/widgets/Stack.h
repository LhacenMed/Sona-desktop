#pragma once

#include "widgets/View.h"

enum class Axis { Horizontal, Vertical };

// Where children sit across a stack's line: at its start, centred or at its end.
enum class Align { Start, Center, End };

// Lays its visible children out in a line, like QML's RowLayout and ColumnLayout: each at its
// preferred length, the ones that fill along the line sharing what's left, and across the line
// aligned as the stack says, or stretched when they fill that way.
class Stack : public View
{
public:
    Stack(Axis axis, float spacing) : m_axis(axis), m_spacing(spacing) {}

    void setPadding(Insets padding) { m_padding = padding; }
    void setAlignment(Align alignment) { m_alignment = alignment; }

    Size preferredSize() const override;
    void layout() override;

private:
    Axis m_axis;
    float m_spacing;
    Insets m_padding;
    Align m_alignment = Align::Center;
};

// Left to right.
class Row : public Stack
{
public:
    explicit Row(float spacing = 0) : Stack(Axis::Horizontal, spacing) {}
};

// Top to bottom.
class Column : public Stack
{
public:
    explicit Column(float spacing = 0) : Stack(Axis::Vertical, spacing) {}
};
