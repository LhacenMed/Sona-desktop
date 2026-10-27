#include "widgets/ListView.h"

#include <algorithm>
#include <cmath>
#include <utility>

namespace {

constexpr float rowGap = 12;
constexpr float rowPitch = ListRow::height + rowGap;
// How far the pointer moves with a row pressed before the row is dragged: Windows' default.
constexpr float dragDistance = 4;

} // namespace

// A row that tells its list how it's clicked and dragged.
class ListView::Row : public ListRow
{
public:
    Row(ListView &list, std::wstring icon, ListMark mark)
        : ListRow(std::move(icon), {}, mark)
        , m_list(list)
    {
    }

    void mousePressed(Point point) override
    {
        m_pressY = point.y;
        if (m_list.m_onRemoved)
            m_list.select(m_list.indexOf(*this));
    }

    void mouseDragged(Point point) override
    {
        if (!m_list.m_onMoved || (!m_dragging && std::abs(point.y - m_pressY) < dragDistance))
            return;
        m_dragging = true;
        m_list.drag(m_list.indexOf(*this), frame().y + point.y);
    }

    void mouseReleased(Point, bool inside) override
    {
        if (std::exchange(m_dragging, false))
            m_list.drop();
        else if (inside && m_list.m_onActivated)
            m_list.m_onActivated(m_list.indexOf(*this));
    }

private:
    ListView &m_list;
    float m_pressY = 0;
    bool m_dragging = false;
};

ListView::ListView(std::wstring icon, ListMark mark)
    : m_icon(std::move(icon))
    , m_mark(mark)
{
}

void ListView::setItems(std::vector<ListItem> items, int current)
{
    const int count = static_cast<int>(items.size());
    while (static_cast<int>(m_rows.size()) > count) {
        remove(*m_rows.back());
        m_rows.pop_back();
    }
    while (static_cast<int>(m_rows.size()) < count)
        m_rows.push_back(&add<Row>(*this, m_icon, m_mark));
    m_selected = std::min(m_selected, count - 1);
    for (int i = 0; i < count; ++i) {
        m_rows[i]->setItem(std::move(items[i]));
        m_rows[i]->setCurrent(i == current);
        m_rows[i]->setSelected(i == m_selected);
    }
    layout();
}

Size ListView::preferredSize() const
{
    return {View::preferredSize().width, m_rows.empty() ? 0 : static_cast<float>(m_rows.size()) * rowPitch - rowGap};
}

void ListView::layout()
{
    for (int i = 0; i < static_cast<int>(m_rows.size()); ++i) {
        // While a row is dragged, it takes the place it would be dropped at and the rows between
        // make way.
        int slot = i;
        if (i == m_dragFrom)
            slot = m_dragTo;
        else if (m_dragFrom < i && i <= m_dragTo)
            slot = i - 1;
        else if (m_dragTo <= i && i < m_dragFrom)
            slot = i + 1;
        m_rows[i]->setFrame({0, slot * rowPitch, frame().width, ListRow::height});
    }
}

bool ListView::keyPressed(KeyChord chord)
{
    if (chord != KeyChord{VK_DELETE} || m_selected < 0 || !m_onRemoved)
        return false;
    m_onRemoved(m_selected);
    return true;
}

int ListView::indexOf(const Row &row) const
{
    return static_cast<int>(std::ranges::find(m_rows, &row) - m_rows.begin());
}

void ListView::select(int index)
{
    m_selected = index;
    for (int i = 0; i < static_cast<int>(m_rows.size()); ++i)
        m_rows[i]->setSelected(i == index);
}

void ListView::drag(int from, float y)
{
    m_dragFrom = from;
    m_dragTo = std::clamp(static_cast<int>(y / rowPitch), 0, static_cast<int>(m_rows.size()) - 1);
    layout();
    invalidate();
}

void ListView::drop()
{
    const int from = std::exchange(m_dragFrom, -1);
    const int to = std::exchange(m_dragTo, -1);
    layout();
    if (from == to)
        return;
    // The selection goes with the row.
    select(to);
    m_onMoved(from, to);
}
