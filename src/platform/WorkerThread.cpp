#include "platform/WorkerThread.h"

#include <memory>

namespace {

constexpr UINT uiTaskMessage = WM_APP + 1;

} // namespace

WorkerThread::WorkerThread()
{
    static const ATOM messageClass = [] {
        WNDCLASSW description{};
        description.lpfnWndProc = messageProc;
        description.hInstance = GetModuleHandleW(nullptr);
        description.lpszClassName = L"SonaWorkerResults";
        return RegisterClassW(&description);
    }();
    m_messageWindow = CreateWindowExW(0, MAKEINTATOM(messageClass), nullptr, 0, 0, 0, 0, 0, HWND_MESSAGE, nullptr,
                                      GetModuleHandleW(nullptr), nullptr);
    m_thread = std::thread([this] { run(); });
}

WorkerThread::~WorkerThread()
{
    {
        const std::lock_guard lock(m_mutex);
        m_quit = true;
    }
    m_wake.notify_one();
    m_thread.join();
    DestroyWindow(m_messageWindow);
}

void WorkerThread::post(std::function<void()> task)
{
    {
        const std::lock_guard lock(m_mutex);
        m_tasks.push_back(std::move(task));
    }
    m_wake.notify_one();
}

void WorkerThread::runOnUiThread(std::function<void()> task)
{
    // messageProc takes ownership of the task on the UI thread.
    auto *posted = new std::function<void()>(std::move(task));
    if (!PostMessageW(m_messageWindow, uiTaskMessage, 0, reinterpret_cast<LPARAM>(posted)))
        delete posted;
}

LRESULT CALLBACK WorkerThread::messageProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    if (message != uiTaskMessage)
        return DefWindowProcW(hwnd, message, wParam, lParam);
    const std::unique_ptr<std::function<void()>> task(reinterpret_cast<std::function<void()> *>(lParam));
    (*task)();
    return 0;
}

void WorkerThread::run()
{
    for (;;) {
        std::function<void()> task;
        {
            std::unique_lock lock(m_mutex);
            m_wake.wait(lock, [this] { return m_quit || !m_tasks.empty(); });
            // Quits only once every task has run.
            if (m_tasks.empty())
                return;
            task = std::move(m_tasks.front());
            m_tasks.pop_front();
        }
        task();
    }
}
