#pragma once

#include <string>

#include "widgets/View.h"

// What a list row shows.
struct ListItem
{
    std::wstring name;
    // Trailing detail, such as a duration.
    std::wstring detail;
    // Leads the row in the accent colour, as chapters are numbered; 0 for none.
    int number = 0;

    bool operator==(const ListItem &) const = default;
};

// Figma queue row (463:848): a 24px icon in a 32px box (the queue's drag handle), a number (chapters,
// 6321:14069) or neither, then a name and trailing detail. Hovering tints it, as does being
// selected; the current row (the playing item) has an accent name.
class ListRow : public View
{
public:
    static constexpr float height = 32;

    // `icon` leads the row; empty for none.
    ListRow(std::wstring icon, ListItem item);

    void setItem(ListItem item);
    void setCurrent(bool current);
    void setSelected(bool selected);

    bool takesMouse() const override { return true; }
    void paint(Canvas &canvas) override;

private:
    std::wstring m_icon;
    ListItem m_item;
    bool m_current = false;
    bool m_selected = false;
};
