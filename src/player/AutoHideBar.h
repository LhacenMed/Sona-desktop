#pragma once

#include <optional>

#include "platform/Timer.h"
#include "widgets/Animation.h"

class View;

// When one auto-hiding bar is in, with the hysteresis mature players use (mpv's OSC, Chromium's PDF
// toolbar, docking panels):
// - it slides in as soon as the pointer is where the bar is when shown (its edge of the window);
// - it stays while the pointer is on it or within a margin past it;
// - once the pointer is farther, it slides out after a short delay, cancelled if the pointer comes
//   back, so brushing past or jittering at its boundary never makes it flicker.
// Distances are measured from the window edge, against the bar's shown size, never its moving
// position, so its own slide can't change the decision.
class AutoHideBar
{
public:
    // `view` repaints while the bar slides.
    explicit AutoHideBar(const View &view);

    // How far in the bar is for the frame being painted, 0–1.
    float reveal() const { return m_reveal.value(m_view); }

    // Follows the pointer: its distance from the bar's edge of the window (none when it's outside
    // the window), for a bar `size` deep when shown.
    void follow(std::optional<float> distanceFromEdge, float size);
    // Shows the bar whatever the pointer does (auto-hide off, nothing to watch).
    void show();

private:
    void slide(bool in);

    const View &m_view;
    Animation m_reveal;
    Timer m_hideDelay;
};
