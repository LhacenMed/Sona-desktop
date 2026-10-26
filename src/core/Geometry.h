#pragma once

// Positions and sizes in device-independent pixels (DIPs): 1 DIP is 1px at 100% display scale,
// so every value here matches the Figma design whatever the monitor's scale.

struct Point
{
    float x = 0;
    float y = 0;
};

struct Size
{
    float width = 0;
    float height = 0;
};

// Space kept inside a view's edges.
struct Insets
{
    float left = 0;
    float top = 0;
    float right = 0;
    float bottom = 0;
};

struct Rect
{
    float x = 0;
    float y = 0;
    float width = 0;
    float height = 0;

    float right() const { return x + width; }
    float bottom() const { return y + height; }
    Size size() const { return {width, height}; }
    bool contains(Point point) const { return point.x >= x && point.x < right() && point.y >= y && point.y < bottom(); }
    Rect inflated(float amount) const { return {x - amount, y - amount, width + 2 * amount, height + 2 * amount}; }
};
