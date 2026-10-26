#pragma once

#include <windows.h>

#include <chrono>
#include <functional>

// Runs a callback on the UI thread after an interval, once or repeatedly. Stops when destroyed.
class Timer
{
public:
    explicit Timer(std::function<void()> callback);
    ~Timer();
    Timer(const Timer &) = delete;
    Timer &operator=(const Timer &) = delete;

    // Restarts the timer if it's running.
    void start(std::chrono::milliseconds interval, bool repeating);
    void stop();
    bool isActive() const { return m_id != 0; }

private:
    static void CALLBACK fired(HWND, UINT, UINT_PTR id, DWORD);

    std::function<void()> m_callback;
    UINT_PTR m_id = 0;
    bool m_repeating = false;
};
