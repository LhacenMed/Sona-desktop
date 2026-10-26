#include "player/AutoHideBar.h"

#include "widgets/View.h"

namespace {

// How far past the bar the pointer can stray before it starts to leave.
constexpr float keepMargin = 24;
// How long the bar waits for the pointer to come back before it leaves (Windows' hover time).
constexpr std::chrono::milliseconds hideDelay{400};
// Windows' fast slide.
constexpr std::chrono::milliseconds slideDuration{167};

} // namespace

AutoHideBar::AutoHideBar(const View &view)
    : m_view(view)
    , m_reveal(1)
{
}

void AutoHideBar::follow(std::optional<float> distanceFromEdge, float size, Clock::time_point now)
{
    const bool shown = m_reveal.target() == 1;
    const bool onBar = distanceFromEdge && *distanceFromEdge < size;
    const bool nearBar = distanceFromEdge && *distanceFromEdge < size + keepMargin;
    if (onBar || (shown && nearBar)) {
        show();
        return;
    }
    if (!shown)
        return;
    if (!m_leavesAt)
        m_leavesAt = now + hideDelay;
    else if (now >= *m_leavesAt)
        slide(false);
}

void AutoHideBar::show()
{
    slide(true);
}

void AutoHideBar::slide(bool in)
{
    m_leavesAt.reset();
    m_reveal.animateTo(in ? 1.0f : 0.0f, slideDuration);
    m_view.invalidate();
}
