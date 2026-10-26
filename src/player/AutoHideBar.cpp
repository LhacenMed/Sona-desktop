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
    , m_hideDelay([this] { slide(false); })
{
}

void AutoHideBar::follow(std::optional<float> distanceFromEdge, float size)
{
    const bool shown = m_reveal.target() == 1;
    const bool onBar = distanceFromEdge && *distanceFromEdge < size;
    const bool nearBar = distanceFromEdge && *distanceFromEdge < size + keepMargin;
    if (onBar || (shown && nearBar))
        show();
    else if (shown && !m_hideDelay.isActive())
        m_hideDelay.start(hideDelay, false);
}

void AutoHideBar::show()
{
    m_hideDelay.stop();
    slide(true);
}

void AutoHideBar::slide(bool in)
{
    m_reveal.animateTo(in ? 1.0f : 0.0f, slideDuration);
    m_view.invalidate();
}
