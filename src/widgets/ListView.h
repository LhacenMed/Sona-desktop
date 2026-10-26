#pragma once

#include <functional>
#include <string>
#include <vector>

#include "widgets/ListRow.h"

// Figma panel list (463:847): ListRows 12 apart, as tall as its rows. A click activates a row (plays
// it, jumps to it) and selects it, and Delete removes the selected one; where the list allows it,
// dragging a row moves it instead, the other rows making way as it goes.
class ListView : public View
{
public:
    // Every row leads with `icon`; empty for none.
    explicit ListView(std::wstring icon = {});

    // Shows `items`, the row at `current` (or none, -1) highlighted. The selection stays where it
    // was, or on the last row once there are fewer.
    void setItems(std::vector<ListItem> items, int current);
    // The selected row, or -1.
    int selected() const { return m_selected; }
    void setOnActivated(std::function<void(int index)> onActivated) { m_onActivated = std::move(onActivated); }
    void setOnRemoved(std::function<void(int index)> onRemoved) { m_onRemoved = std::move(onRemoved); }
    // Lets rows be dragged; `onMoved` gets a dragged row's index and the one it was dropped at.
    void setOnMoved(std::function<void(int from, int to)> onMoved) { m_onMoved = std::move(onMoved); }

    Size preferredSize() const override;
    void layout() override;
    bool acceptsFocus() const override { return true; }
    bool keyPressed(KeyChord chord) override;

private:
    class Row;

    int indexOf(const Row &row) const;
    void select(int index);
    // Row `from` is being dragged with the pointer `y` down the list.
    void drag(int from, float y);
    void drop();

    std::wstring m_icon;
    std::vector<Row *> m_rows;
    int m_selected = -1;
    // While a row is dragged: its index and the one it would be dropped at.
    int m_dragFrom = -1;
    int m_dragTo = -1;
    std::function<void(int)> m_onActivated;
    std::function<void(int)> m_onRemoved;
    std::function<void(int, int)> m_onMoved;
};
