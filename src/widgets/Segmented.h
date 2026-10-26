#pragma once

#include <functional>
#include <string>
#include <vector>

#include "widgets/View.h"

// Figma "Values" (804:1208, 999:4549): joined options of equal width in a well; the chosen one is
// accent-filled. Its height comes from the call site (44 in panels, 36 in settings).
class Segmented : public View
{
public:
    Segmented(std::vector<std::wstring> options, int selected);

    int selected() const { return m_selected; }
    void setSelected(int index);
    void setOnChanged(std::function<void(int)> onChanged) { m_onChanged = std::move(onChanged); }

    bool takesMouse() const override { return true; }
    Cursor cursor() const override { return Cursor::Hand; }
    void paint(Canvas &canvas) override;
    void mouseReleased(Point point, bool inside) override;

private:
    Rect cell(int index) const;
    int indexAt(float x) const;

    std::vector<std::wstring> m_options;
    int m_selected;
    std::function<void(int)> m_onChanged;
};
