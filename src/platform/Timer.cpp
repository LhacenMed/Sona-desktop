#include "platform/Timer.h"

#include <unordered_map>

namespace {

// Thread timers carry no context, so each one is found by its id.
std::unordered_map<UINT_PTR, Timer *> &runningTimers()
{
    static std::unordered_map<UINT_PTR, Timer *> timers;
    return timers;
}

} // namespace

Timer::Timer(std::function<void()> callback)
    : m_callback(std::move(callback))
{
}

Timer::~Timer()
{
    stop();
}

void Timer::start(std::chrono::milliseconds interval, bool repeating)
{
    stop();
    m_repeating = repeating;
    m_id = SetTimer(nullptr, 0, static_cast<UINT>(interval.count()), fired);
    runningTimers()[m_id] = this;
}

void Timer::stop()
{
    if (!m_id)
        return;
    KillTimer(nullptr, m_id);
    runningTimers().erase(m_id);
    m_id = 0;
}

void CALLBACK Timer::fired(HWND, UINT, UINT_PTR id, DWORD)
{
    const auto found = runningTimers().find(id);
    if (found == runningTimers().end())
        return;
    Timer *timer = found->second;
    if (!timer->m_repeating)
        timer->stop();
    // Last: the callback may destroy the timer.
    timer->m_callback();
}
