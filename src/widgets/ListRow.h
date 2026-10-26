#pragma once

#include <functional>
#include <string>

#include "widgets/View.h"

// Figma queue row (463:848): a 24px icon in a 32px box, a name, and trailing detail such as a
// duration. Hovering tints it; the selected row (the current item) has an accent name. 32 tall.
class ListRow : public View
{
public:
    ListRow(std::wstring icon, std::wstring name, std::wstring detail);

    void setSelected(bool selected);
    void setOnClick(std::function<void()> onClick) { m_onClick = std::move(onClick); }

    bool takesMouse() const override { return true; }
    void paint(Canvas &canvas) override;
    void mouseReleased(Point point, bool inside) override;

private:
    std::wstring m_icon;
    std::wstring m_name;
    std::wstring m_detail;
    bool m_selected = false;
    std::function<void()> m_onClick;
};
