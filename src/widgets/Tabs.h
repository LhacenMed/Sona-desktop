#pragma once

#include <functional>
#include <string>
#include <vector>

#include "widgets/View.h"

struct Tab
{
    // An icon with a two-tone variant (assets/icons/<icon>-duo.svg) for the active tab.
    std::wstring icon;
    std::wstring label;
};

// Figma panel tab strip (5187:24309): icon-and-label tabs sharing the width. The active tab's icon
// is two-tone and its label brighter, over an accent underline. 50 tall.
class Tabs : public View
{
public:
    Tabs(std::vector<Tab> tabs, int active);

    int active() const { return m_active; }
    void setActive(int index);
    void setOnChanged(std::function<void(int)> onChanged) { m_onChanged = std::move(onChanged); }

    bool takesMouse() const override { return true; }
    Cursor cursor() const override { return Cursor::Hand; }
    void paint(Canvas &canvas) override;
    void mouseReleased(Point point, bool inside) override;

private:
    Rect tabBounds(int index) const;
    int indexAt(float x) const;

    std::vector<Tab> m_tabs;
    int m_active;
    std::function<void(int)> m_onChanged;
};
