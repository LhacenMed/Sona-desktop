#include "widgets/Animation.h"

#include "widgets/View.h"

void Animation::animateTo(float target, std::chrono::milliseconds duration, Easing easing)
{
    if (target == m_to)
        return;
    const Clock::time_point now = Clock::now();
    m_from = valueAt(now);
    m_to = target;
    m_start = now;
    m_duration = duration;
    m_easing = easing;
}

void Animation::jumpTo(float value)
{
    m_from = value;
    m_to = value;
    m_duration = {};
}

float Animation::value(const View &view) const
{
    const Clock::time_point now = Clock::now();
    if (now < m_start + m_duration)
        view.invalidate();
    return valueAt(now);
}

float Animation::valueAt(Clock::time_point time) const
{
    if (time >= m_start + m_duration)
        return m_to;
    const float remaining = 1 - std::chrono::duration<float>(time - m_start) / m_duration;
    const float left = remaining * remaining * remaining * (m_easing == Easing::OutQuint ? remaining * remaining : 1);
    return m_from + (m_to - m_from) * (1 - left);
}
