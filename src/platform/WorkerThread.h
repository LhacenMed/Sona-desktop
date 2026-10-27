#pragma once

#include <windows.h>

#include <condition_variable>
#include <deque>
#include <functional>
#include <mutex>
#include <thread>

// A thread that runs tasks one at a time, in the order they're posted, for work that would hold up
// the UI (libVLC, Windows' thumbnail cache). Its tasks hand their results back to the UI thread, the
// one that created it, with runOnUiThread. Destroying it runs the tasks still queued, then ends the
// thread.
class WorkerThread
{
public:
    WorkerThread();
    ~WorkerThread();
    WorkerThread(const WorkerThread &) = delete;
    WorkerThread &operator=(const WorkerThread &) = delete;

    void post(std::function<void()> task);
    // Runs `task` on the UI thread; callable from any thread.
    void runOnUiThread(std::function<void()> task);

private:
    static LRESULT CALLBACK messageProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
    void run();

    HWND m_messageWindow = nullptr;
    std::mutex m_mutex;
    std::condition_variable m_wake;
    std::deque<std::function<void()>> m_tasks;
    bool m_quit = false;
    std::thread m_thread;
};
