#pragma once

#include <chrono>

class View;

// How an animation eases, both fast at first and settling gently.
enum class Easing {
    // Ease-out cubic: hover fades, sliding, scrolling.
    OutCubic,
    // Ease-out quint, VS Code's menu motion (cubic-bezier(0.22, 1, 0.36, 1)): popups opening.
    OutQuint,
};

// A number that eases to a new target over a short time (hover fades, sliding, smooth scrolling).
// Reading it while it moves asks the view's window for another frame, so it advances once per
// displayed frame with no timer: presenting a frame waits for the display, which paces them.
class Animation
{
public:
    explicit Animation(float value = 0) : m_from(value), m_to(value) {}

    float target() const { return m_to; }
    // Eases from where the value is now.
    void animateTo(float target, std::chrono::milliseconds duration, Easing easing = Easing::OutCubic);
    void jumpTo(float value);
    // The value for the frame being painted; keeps `view` repainting until it arrives.
    float value(const View &view) const;

private:
    using Clock = std::chrono::steady_clock;

    float valueAt(Clock::time_point time) const;

    float m_from;
    float m_to;
    Clock::time_point m_start;
    std::chrono::milliseconds m_duration{0};
    Easing m_easing = Easing::OutCubic;
};
