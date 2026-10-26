#pragma once

#include <functional>
#include <string>
#include <vector>

#include "widgets/View.h"

// Figma "Dropdown" (2372:8300): the chosen option in a field with a chevron. Clicking it opens the
// options in a popup laid over the field, the chosen one checked; the arrow keys, Enter and the
// mouse pick one. 48 tall; its width comes from the call site.
class Dropdown : public View
{
public:
    Dropdown(std::vector<std::wstring> options, int selected);

    int selected() const { return m_selected; }
    void setSelected(int index);
    void setOnChanged(std::function<void(int)> onChanged) { m_onChanged = std::move(onChanged); }

    bool takesMouse() const override { return true; }
    Cursor cursor() const override { return Cursor::Hand; }
    void paint(Canvas &canvas) override;
    void mouseReleased(Point point, bool inside) override;

private:
    void pick(int index);

    std::vector<std::wstring> m_options;
    int m_selected;
    std::function<void(int)> m_onChanged;
};
